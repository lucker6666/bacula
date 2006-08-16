/*
 *
 *   Bacula Director -- migrate.c -- responsible for doing
 *     migration jobs.
 *
 *     Kern Sibbald, September MMIV
 *
 *  Basic tasks done here:
 *     Open DB and create records for this job.
 *     Open Message Channel with Storage daemon to tell him a job will be starting.
 *     Open connection with Storage daemon and pass him commands
 *       to do the backup.
 *     When the Storage daemon finishes the job, update the DB.
 *
 *   Version $Id$
 */
/*
   Copyright (C) 2004-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "dird.h"
#include "ua.h"
#ifndef HAVE_REGEX_H
#include "lib/bregex.h"
#else
#include <regex.h>
#endif

static const int dbglevel = 100;

static char OKbootstrap[] = "3000 OK bootstrap\n";
static bool get_job_to_migrate(JCR *jcr);
struct idpkt;
static bool regex_find_jobids(JCR *jcr, idpkt *ids, const char *query1,
                 const char *query2, const char *type);
static bool find_mediaid_then_jobids(JCR *jcr, idpkt *ids, const char *query1,
                 const char *type);
static bool find_jobids_from_mediaid_list(JCR *jcr, idpkt *ids, const char *type);
static void start_migration_job(JCR *jcr);

/* 
 * Called here before the job is run to do the job
 *   specific setup.
 */
bool do_migration_init(JCR *jcr)
{
   /* If we find a job to migrate it is previous_jr.JobId */
   if (!get_job_to_migrate(jcr)) {
      return false;
   }

   if (jcr->previous_jr.JobId == 0) {
      return true;                    /* no work */
   }

   if (!get_or_create_fileset_record(jcr)) {
      return false;
   }

   apply_pool_overrides(jcr);

   jcr->jr.PoolId = get_or_create_pool_record(jcr, jcr->pool->hdr.name);
   if (jcr->jr.PoolId == 0) {
      return false;
   }

   /* If pool storage specified, use it instead of job storage */
   copy_wstorage(jcr, jcr->pool->storage, _("Pool resource"));

   if (!jcr->wstorage) {
      Jmsg(jcr, M_FATAL, 0, _("No Storage specification found in Job or Pool.\n"));
      return false;
   }

   create_restore_bootstrap_file(jcr);
   return true;
}

/*
 * Do a Migration of a previous job
 *
 *  Returns:  false on failure
 *            true  on success
 */
bool do_migration(JCR *jcr)
{
   POOL_DBR pr;
   POOL *pool;
   char ed1[100];
   BSOCK *sd;
   JOB *job, *prev_job;
   JCR *mig_jcr;                   /* newly migrated job */

   /* 
    *  previous_jr refers to the job DB record of the Job that is
    *    going to be migrated.
    *  prev_job refers to the job resource of the Job that is
    *    going to be migrated.
    *  jcr is the jcr for the current "migration" job.  It is a
    *    control job that is put in the DB as a migration job, which
    *    means that this job migrated a previous job to a new job.
    *    No Volume or File data is associated with this control
    *    job.
    *  mig_jcr refers to the newly migrated job that is run by
    *    the current jcr.  It is a backup job that moves (migrates) the
    *    data written for the previous_jr into the new pool.  This
    *    job (mig_jcr) becomes the new backup job that replaces
    *    the original backup job.
    */
   if (jcr->previous_jr.JobId == 0 || jcr->ExpectedFiles == 0) {
      set_jcr_job_status(jcr, JS_Terminated);
      migration_cleanup(jcr, jcr->JobStatus);
      return true;                    /* no work */
   }

   Dmsg4(dbglevel, "Previous: Name=%s JobId=%d Type=%c Level=%c\n",
      jcr->previous_jr.Name, jcr->previous_jr.JobId, 
      jcr->previous_jr.JobType, jcr->previous_jr.JobLevel);

   Dmsg4(dbglevel, "Current: Name=%s JobId=%d Type=%c Level=%c\n",
      jcr->jr.Name, jcr->jr.JobId, 
      jcr->jr.JobType, jcr->jr.JobLevel);

   LockRes();
   job = (JOB *)GetResWithName(R_JOB, jcr->jr.Name);
   prev_job = (JOB *)GetResWithName(R_JOB, jcr->previous_jr.Name);
   UnlockRes();
   if (!job || !prev_job) {
      return false;
   }

   /* Create a migation jcr */
   mig_jcr = jcr->mig_jcr = new_jcr(sizeof(JCR), dird_free_jcr);
   memcpy(&mig_jcr->previous_jr, &jcr->previous_jr, sizeof(mig_jcr->previous_jr));

   /*
    * Turn the mig_jcr into a "real" job that takes on the aspects of
    *   the previous backup job "prev_job".
    */
   set_jcr_defaults(mig_jcr, prev_job);
   if (!setup_job(mig_jcr)) {
      return false;
   }

   /* Now reset the job record from the previous job */
   memcpy(&mig_jcr->jr, &jcr->previous_jr, sizeof(mig_jcr->jr));
   /* Update the jr to reflect the new values of PoolId, FileSetId, and JobId. */
   mig_jcr->jr.PoolId = jcr->jr.PoolId;
   mig_jcr->jr.FileSetId = jcr->jr.FileSetId;
   mig_jcr->jr.JobId = mig_jcr->JobId;

   Dmsg4(dbglevel, "mig_jcr: Name=%s JobId=%d Type=%c Level=%c\n",
      mig_jcr->jr.Name, mig_jcr->jr.JobId, 
      mig_jcr->jr.JobType, mig_jcr->jr.JobLevel);

   /*
    * Get the PoolId used with the original job. Then
    *  find the pool name from the database record.
    */
   memset(&pr, 0, sizeof(pr));
   pr.PoolId = mig_jcr->previous_jr.PoolId;
   if (!db_get_pool_record(jcr, jcr->db, &pr)) {
      Jmsg(jcr, M_FATAL, 0, _("Pool for JobId %s not in database. ERR=%s\n"),
            edit_int64(pr.PoolId, ed1), db_strerror(jcr->db));
         return false;
   }
   /* Get the pool resource corresponding to the original job */
   pool = (POOL *)GetResWithName(R_POOL, pr.Name);
   if (!pool) {
      Jmsg(jcr, M_FATAL, 0, _("Pool resource \"%s\" not found.\n"), pr.Name);
      return false;
   }

   /* If pool storage specified, use it for restore */
   copy_rstorage(mig_jcr, pool->storage, _("Pool resource"));
   copy_rstorage(jcr, pool->storage, _("Pool resource"));

   /*
    * If the original backup pool has a NextPool, make sure a 
    *  record exists in the database. Note, in this case, we
    *  will be migrating from pool to pool->NextPool.
    */
   if (pool->NextPool) {
      jcr->jr.PoolId = get_or_create_pool_record(jcr, pool->NextPool->hdr.name);
      if (jcr->jr.PoolId == 0) {
         return false;
      }
      /*
       * put the "NextPool" resource pointer in our jcr so that we
       * can pull the Storage reference from it.
       */
      mig_jcr->pool = jcr->pool = pool->NextPool;
      mig_jcr->jr.PoolId = jcr->jr.PoolId;
      pm_strcpy(jcr->pool_source, _("NextPool in Pool resource"));
   }

   /* If pool storage specified, use it instead of job storage for backup */
   copy_wstorage(jcr, jcr->pool->storage, _("Pool resource"));

   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Migration JobId %s, Job=%s\n"),
        edit_uint64(jcr->JobId, ed1), jcr->Job);

   set_jcr_job_status(jcr, JS_Running);
   set_jcr_job_status(mig_jcr, JS_Running);
   Dmsg2(dbglevel, "JobId=%d JobLevel=%c\n", jcr->jr.JobId, jcr->jr.JobLevel);

   /* Update job start record for this migration control job */
   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      return false;
   }

   Dmsg4(dbglevel, "mig_jcr: Name=%s JobId=%d Type=%c Level=%c\n",
      mig_jcr->jr.Name, mig_jcr->jr.JobId, 
      mig_jcr->jr.JobType, mig_jcr->jr.JobLevel);

   /* Update job start record for the real migration backup job */
   if (!db_update_job_start_record(mig_jcr, mig_jcr->db, &mig_jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(mig_jcr->db));
      return false;
   }


   /*
    * Open a message channel connection with the Storage
    * daemon. This is to let him know that our client
    * will be contacting him for a backup  session.
    *
    */
   Dmsg0(110, "Open connection with storage daemon\n");
   set_jcr_job_status(jcr, JS_WaitSD);
   set_jcr_job_status(mig_jcr, JS_WaitSD);
   /*
    * Start conversation with Storage daemon
    */
   if (!connect_to_storage_daemon(jcr, 10, SDConnectTimeout, 1)) {
      return false;
   }
   sd = jcr->store_bsock;
   /*
    * Now start a job with the Storage daemon
    */
   Dmsg2(dbglevel, "Read store=%s, write store=%s\n", 
      ((STORE *)jcr->rstorage->first())->name(),
      ((STORE *)jcr->wstorage->first())->name());
   if (!start_storage_daemon_job(jcr, jcr->rstorage, jcr->wstorage)) {
      return false;
   }
   Dmsg0(150, "Storage daemon connection OK\n");

   if (!send_bootstrap_file(jcr, sd) ||
       !response(jcr, sd, OKbootstrap, "Bootstrap", DISPLAY_ERROR)) {
      return false;
   }

   if (!bnet_fsend(sd, "run")) {
      return false;
   }

   /*
    * Now start a Storage daemon message thread
    */
   if (!start_storage_daemon_message_thread(jcr)) {
      return false;
   }


   set_jcr_job_status(jcr, JS_Running);
   set_jcr_job_status(mig_jcr, JS_Running);

   /* Pickup Job termination data */
   /* Note, the SD stores in jcr->JobFiles/ReadBytes/JobBytes/Errors */
   wait_for_storage_daemon_termination(jcr);

   set_jcr_job_status(jcr, jcr->SDJobStatus);
   if (jcr->JobStatus != JS_Terminated) {
      return false;
   }
   migration_cleanup(jcr, jcr->JobStatus);
   if (mig_jcr) {
      UAContext *ua = new_ua_context(jcr);
      purge_files_from_job(ua, jcr->previous_jr.JobId);
      free_ua_context(ua);
   }
   return true;
}

struct idpkt {
   POOLMEM *list;
   uint32_t count;
};

/*
 * Callback handler make list of DB Ids
 */
static int dbid_handler(void *ctx, int num_fields, char **row)
{
   idpkt *ids = (idpkt *)ctx;

   Dmsg3(dbglevel, "count=%d Ids=%p %s\n", ids->count, ids->list, ids->list);
   if (ids->count == 0) {
      ids->list[0] = 0;
   } else {
      pm_strcat(ids->list, ",");
   }
   pm_strcat(ids->list, row[0]);
   ids->count++;
   return 0;
}


struct uitem {
   dlink link;   
   char *item;
};

static int item_compare(void *item1, void *item2)
{
   uitem *i1 = (uitem *)item1;
   uitem *i2 = (uitem *)item2;
   return strcmp(i1->item, i2->item);
}

static int unique_name_handler(void *ctx, int num_fields, char **row)
{
   dlist *list = (dlist *)ctx;

   uitem *new_item = (uitem *)malloc(sizeof(uitem));
   uitem *item;
   
   memset(new_item, 0, sizeof(uitem));
   new_item->item = bstrdup(row[0]);
   Dmsg1(dbglevel, "Item=%s\n", row[0]);
   item = (uitem *)list->binary_insert((void *)new_item, item_compare);
   if (item != new_item) {            /* already in list */
      free(new_item->item);
      free((char *)new_item);
      return 0;
   }
   return 0;
}

/* Get Job names in Pool */
const char *sql_job =
   "SELECT DISTINCT Job.Name from Job,Pool"
   " WHERE Pool.Name='%s' AND Job.PoolId=Pool.PoolId";

/* Get JobIds from regex'ed Job names */
const char *sql_jobids_from_job =
   "SELECT DISTINCT Job.JobId,Job.StartTime FROM Job,Pool"
   " WHERE Job.Name='%s' AND Pool.Name='%s' AND Job.PoolId=Pool.PoolId"
   " ORDER by Job.StartTime";

/* Get Client names in Pool */
const char *sql_client =
   "SELECT DISTINCT Client.Name from Client,Pool,Job"
   " WHERE Pool.Name='%s' AND Job.ClientId=Client.ClientId AND"
   " Job.PoolId=Pool.PoolId";

/* Get JobIds from regex'ed Client names */
const char *sql_jobids_from_client =
   "SELECT DISTINCT Job.JobId,Job.StartTime FROM Job,Pool"
   " WHERE Client.Name='%s' AND Pool.Name='%s' AND Job.PoolId=Pool.PoolId"
   " AND Job.ClientId=Client.ClientId "
   " ORDER by Job.StartTime";

/* Get Volume names in Pool */
const char *sql_vol = 
   "SELECT DISTINCT VolumeName FROM Media,Pool WHERE"
   " VolStatus in ('Full','Used','Error') AND Media.Enabled=1 AND"
   " Media.PoolId=Pool.PoolId AND Pool.Name='%s'";

/* Get JobIds from regex'ed Volume names */
const char *sql_jobids_from_vol =
   "SELECT DISTINCT Job.JobId,Job.StartTime FROM Media,JobMedia,Job"
   " WHERE Media.VolumeName='%s' AND Media.MediaId=JobMedia.MediaId"
   " AND JobMedia.JobId=Job.JobId" 
   " ORDER by Job.StartTime";


const char *sql_smallest_vol = 
   "SELECT MediaId FROM Media,Pool WHERE"
   " VolStatus in ('Full','Used','Error') AND Media.Enabled=1 AND"
   " Media.PoolId=Pool.PoolId AND Pool.Name='%s'"
   " ORDER BY VolBytes ASC LIMIT 1";

const char *sql_oldest_vol = 
   "SELECT MediaId FROM Media,Pool WHERE"
   " VolStatus in ('Full','Used','Error') AND Media.Enabled=1 AND"
   " Media.PoolId=Pool.PoolId AND Pool.Name='%s'"
   " ORDER BY LastWritten ASC LIMIT 1";

/* Get JobIds when we have selected MediaId */
const char *sql_jobids_from_mediaid =
   "SELECT DISTINCT Job.JobId,Job.StartTime FROM JobMedia,Job"
   " WHERE JobMedia.JobId=Job.JobId AND JobMedia.MediaId=%s"
   " ORDER by Job.StartTime";

const char *sql_pool_bytes =
   "SELECT SUM(VolBytes) FROM Media,Pool WHERE"
   " VolStatus in ('Full','Used','Error','Append') AND Media.Enabled=1 AND"
   " Media.PoolId=Pool.PoolId AND Pool.Name='%s'";

const char *sql_vol_bytes =
   "SELECT MediaId FROM Media,Pool WHERE"
   " VolStatus in ('Full','Used','Error') AND Media.Enabled=1 AND"
   " Media.PoolId=Pool.PoolId AND Pool.Name='%s' AND"
   " VolBytes<%s ORDER BY LastWritten ASC LIMIT 1";


const char *sql_ujobid =
   "SELECT DISTINCT Job.Job from Client,Pool,Media,Job,JobMedia "
   " WHERE Media.PoolId=Pool.PoolId AND Pool.Name='%s' AND"
   " JobMedia.JobId=Job.JobId AND Job.PoolId=Media.PoolId";



/*
 * Returns: false on error
 *          true  if OK and jcr->previous_jr filled in
 */
static bool get_job_to_migrate(JCR *jcr)
{
   char ed1[30];
   POOL_MEM query(PM_MESSAGE);
   JobId_t JobId;
   int stat;
   char *p;
   idpkt ids;

   ids.list = get_pool_memory(PM_MESSAGE);
   Dmsg1(dbglevel, "list=%p\n", ids.list);
   ids.list[0] = 0;
   ids.count = 0;

   /*
    * If MigrateJobId is set, then we migrate only that Job,
    *  otherwise, we go through the full selection of jobs to
    *  migrate.
    */
   if (jcr->MigrateJobId != 0) {
      Dmsg1(dbglevel, "At Job start previous jobid=%u\n", jcr->MigrateJobId);
      edit_uint64(jcr->MigrateJobId, ids.list);
      ids.count = 1;
   } else {
      switch (jcr->job->selection_type) {
      case MT_JOB:
         if (!regex_find_jobids(jcr, &ids, sql_job, sql_jobids_from_job, "Job")) {
            goto bail_out;
         } 
         break;
      case MT_CLIENT:
         if (!regex_find_jobids(jcr, &ids, sql_client, sql_jobids_from_client, "Client")) {
            goto bail_out;
         } 
         break;
      case MT_VOLUME:
         if (!regex_find_jobids(jcr, &ids, sql_vol, sql_jobids_from_vol, "Volume")) {
            goto bail_out;
         } 
         break;
      case MT_SQLQUERY:
         if (!jcr->job->selection_pattern) {
            Jmsg(jcr, M_FATAL, 0, _("No Migration SQL selection pattern specified.\n"));
            goto bail_out;
         }
         Dmsg1(dbglevel, "SQL=%s\n", jcr->job->selection_pattern);
         if (!db_sql_query(jcr->db, jcr->job->selection_pattern,
              dbid_handler, (void *)&ids)) {
            Jmsg(jcr, M_FATAL, 0,
                 _("SQL failed. ERR=%s\n"), db_strerror(jcr->db));
            goto bail_out;
         }
         break;
      case MT_SMALLEST_VOL:
         if (!find_mediaid_then_jobids(jcr, &ids, sql_smallest_vol, "Smallest Volume")) {
            goto bail_out;
         }
         break;
      case MT_OLDEST_VOL:
         if (!find_mediaid_then_jobids(jcr, &ids, sql_oldest_vol, "Oldest Volume")) {
            goto bail_out;
         }
         break;

/***** Below not implemented yet *********/
      case MT_POOL_OCCUPANCY:
         db_int64_ctx ctx;

         ctx.count = 0;
         Mmsg(query, sql_pool_bytes, jcr->pool->hdr.name);
         if (!db_sql_query(jcr->db, query.c_str(), db_int64_handler, (void *)&ctx)) {
            Jmsg(jcr, M_FATAL, 0, _("SQL failed. ERR=%s\n"), db_strerror(jcr->db));
            goto bail_out;
         }
         if (ctx.count == 0) {
            Jmsg(jcr, M_INFO, 0, _("No Volumes found to migrate.\n"));
            goto bail_out;
         }
         if (ctx.value > (int64_t)jcr->pool->MigrationHighBytes) {
            Dmsg2(dbglevel, "highbytes=%d pool=%d\n", (int)jcr->pool->MigrationHighBytes,
               (int)ctx.value);
         }
         goto bail_out;
         break;
      case MT_POOL_TIME:
         Dmsg0(dbglevel, "Pool time not implemented\n");
         break;
      default:
         Jmsg(jcr, M_FATAL, 0, _("Unknown Migration Selection Type.\n"));
         goto bail_out;
      }
   }

   /*
    * Loop over all jobids except the last one, sending
    *  them to start_migration_job(), which will start a job
    *  for each of them.  For the last JobId, we handle it below.
    */
   p = ids.list;
   for (int i=1; i < (int)ids.count; i++) {
      JobId = 0;
      stat = get_next_jobid_from_list(&p, &JobId);
      Dmsg2(dbglevel, "get_next_jobid stat=%d JobId=%u\n", stat, JobId);
      jcr->MigrateJobId = JobId;
      start_migration_job(jcr);
      if (stat < 0) {
         Jmsg(jcr, M_FATAL, 0, _("Invalid JobId found.\n"));
         goto bail_out;
      } else if (stat == 0) {
         Jmsg(jcr, M_INFO, 0, _("No JobIds found to migrate.\n"));
         goto ok_out;
      }
   }
   
   /* Now get the last JobId and handle it in the current job */
   JobId = 0;
   stat = get_next_jobid_from_list(&p, &JobId);
   Dmsg2(dbglevel, "Last get_next_jobid stat=%d JobId=%u\n", stat, JobId);
   if (stat < 0) {
      Jmsg(jcr, M_FATAL, 0, _("Invalid JobId found.\n"));
      goto bail_out;
   } else if (stat == 0) {
      Jmsg(jcr, M_INFO, 0, _("No JobIds found to migrate.\n"));
      goto ok_out;
   }

   jcr->previous_jr.JobId = JobId;
   Dmsg1(100, "Previous jobid=%d\n", jcr->previous_jr.JobId);

   if (!db_get_job_record(jcr, jcr->db, &jcr->previous_jr)) {
      Jmsg(jcr, M_FATAL, 0, _("Could not get job record for JobId %s to migrate. ERR=%s"),
           edit_int64(jcr->previous_jr.JobId, ed1),
           db_strerror(jcr->db));
      goto bail_out;
   }
   Jmsg(jcr, M_INFO, 0, _("Migration using JobId=%d Job=%s\n"),
      jcr->previous_jr.JobId, jcr->previous_jr.Job);

ok_out:
   free_pool_memory(ids.list);
   return true;

bail_out:
   free_pool_memory(ids.list);
   return false;
}

static void start_migration_job(JCR *jcr)
{
   UAContext *ua = new_ua_context(jcr);
   char ed1[50];
   ua->batch = true;
   Mmsg(ua->cmd, "run %s jobid=%s", jcr->job->hdr.name, 
        edit_uint64(jcr->MigrateJobId, ed1));
   Dmsg1(dbglevel, "=============== Migration cmd=%s\n", ua->cmd);
   parse_ua_args(ua);                 /* parse command */
   int stat = run_cmd(ua, ua->cmd);
   if (stat == 0) {
      Jmsg(jcr, M_ERROR, 0, _("Could not start migration job.\n"));
   } else {
      Jmsg(jcr, M_INFO, 0, _("Migration JobId %d started.\n"), stat);
   }
   free_ua_context(ua);
}

static bool find_mediaid_then_jobids(JCR *jcr, idpkt *ids, const char *query1,
                 const char *type) 
{
   bool ok = false;
   POOL_MEM query(PM_MESSAGE);

   ids->count = 0;
   /* Basic query for MediaId */
   Mmsg(query, query1, jcr->pool->hdr.name);
   if (!db_sql_query(jcr->db, query.c_str(), dbid_handler, (void *)ids)) {
      Jmsg(jcr, M_FATAL, 0, _("SQL failed. ERR=%s\n"), db_strerror(jcr->db));
      goto bail_out;
   }
   if (ids->count == 0) {
      Jmsg(jcr, M_INFO, 0, _("No %ss found to migrate.\n"), type);
   }
   if (ids->count != 1) {
      Jmsg(jcr, M_FATAL, 0, _("SQL logic error. Count should be 1 but is %d\n"), 
         ids->count);
      goto bail_out;
   }
   Dmsg1(dbglevel, "Smallest Vol Jobids=%s\n", ids->list);

   ok = find_jobids_from_mediaid_list(jcr, ids, type);

bail_out:
   return ok;
}

static bool find_jobids_from_mediaid_list(JCR *jcr, idpkt *ids, const char *type) 
{
   bool ok = false;
   POOL_MEM query(PM_MESSAGE);

   Mmsg(query, sql_jobids_from_mediaid, ids->list);
   ids->count = 0;
   if (!db_sql_query(jcr->db, query.c_str(), dbid_handler, (void *)ids)) {
      Jmsg(jcr, M_FATAL, 0, _("SQL failed. ERR=%s\n"), db_strerror(jcr->db));
      goto bail_out;
   }
   if (ids->count == 0) {
      Jmsg(jcr, M_INFO, 0, _("No %ss found to migrate.\n"), type);
   }
   ok = true;
bail_out:
   return ok;
}

static bool regex_find_jobids(JCR *jcr, idpkt *ids, const char *query1,
                 const char *query2, const char *type) 
{
   dlist *item_chain;
   uitem *item = NULL;
   uitem *last_item = NULL;
   regex_t preg;
   char prbuf[500];
   int rc;
   bool ok = false;
   POOL_MEM query(PM_MESSAGE);

   item_chain = New(dlist(item, &item->link));
   if (!jcr->job->selection_pattern) {
      Jmsg(jcr, M_FATAL, 0, _("No Migration %s selection pattern specified.\n"),
         type);
      goto bail_out;
   }
   Dmsg1(dbglevel, "regex=%s\n", jcr->job->selection_pattern);
   /* Compile regex expression */
   rc = regcomp(&preg, jcr->job->selection_pattern, REG_EXTENDED);
   if (rc != 0) {
      regerror(rc, &preg, prbuf, sizeof(prbuf));
      Jmsg(jcr, M_FATAL, 0, _("Could not compile regex pattern \"%s\" ERR=%s\n"),
           jcr->job->selection_pattern, prbuf);
      goto bail_out;
   }
   /* Basic query for names */
   Mmsg(query, query1, jcr->pool->hdr.name);
   Dmsg1(dbglevel, "query1=%s\n", query.c_str());
   if (!db_sql_query(jcr->db, query.c_str(), unique_name_handler, 
        (void *)item_chain)) {
      Jmsg(jcr, M_FATAL, 0,
           _("SQL to get %s failed. ERR=%s\n"), type, db_strerror(jcr->db));
      goto bail_out;
   }
   /* Now apply the regex to the names and remove any item not matched */
   foreach_dlist(item, item_chain) {
      const int nmatch = 30;
      regmatch_t pmatch[nmatch];
      if (last_item) {
         Dmsg1(dbglevel, "Remove item %s\n", last_item->item);
         free(last_item->item);
         item_chain->remove(last_item);
      }
      Dmsg1(dbglevel, "Item=%s\n", item->item);
      rc = regexec(&preg, item->item, nmatch, pmatch,  0);
      if (rc == 0) {
         last_item = NULL;   /* keep this one */
      } else {   
         last_item = item;
      }
   }
   if (last_item) {
      free(last_item->item);
      Dmsg1(dbglevel, "Remove item %s\n", last_item->item);
      item_chain->remove(last_item);
   }
   regfree(&preg);
   /* 
    * At this point, we have a list of items in item_chain
    *  that have been matched by the regex, so now we need
    *  to look up their jobids.
    */
   ids->count = 0;
   foreach_dlist(item, item_chain) {
      Dmsg2(dbglevel, "Got %s: %s\n", type, item->item);
      Mmsg(query, query2, item->item, jcr->pool->hdr.name);
      Dmsg1(dbglevel, "query2=%s\n", query.c_str());
      if (!db_sql_query(jcr->db, query.c_str(), dbid_handler, (void *)ids)) {
         Jmsg(jcr, M_FATAL, 0,
              _("SQL failed. ERR=%s\n"), db_strerror(jcr->db));
         goto bail_out;
      }
   }
   if (ids->count == 0) {
      Jmsg(jcr, M_INFO, 0, _("No %ss found to migrate.\n"), type);
   }
   ok = true;
bail_out:
   Dmsg2(dbglevel, "Count=%d Jobids=%s\n", ids->count, ids->list);
   delete item_chain;
   Dmsg0(dbglevel, "After delete item_chain\n");
   return ok;
}


/*
 * Release resources allocated during backup.
 */
void migration_cleanup(JCR *jcr, int TermCode)
{
   char sdt[MAX_TIME_LENGTH], edt[MAX_TIME_LENGTH];
   char ec1[30], ec2[30], ec3[30], ec4[30], ec5[30], elapsed[50];
   char ec6[50], ec7[50], ec8[50];
   char term_code[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   MEDIA_DBR mr;
   double kbps;
   utime_t RunTime;
   JCR *mig_jcr = jcr->mig_jcr;
   POOL_MEM query(PM_MESSAGE);

   Dmsg2(100, "Enter migrate_cleanup %d %c\n", TermCode, TermCode);
   dequeue_messages(jcr);             /* display any queued messages */
   memset(&mr, 0, sizeof(mr));
   set_jcr_job_status(jcr, TermCode);
   update_job_end_record(jcr);        /* update database */

   /* 
    * Check if we actually did something.  
    *  mig_jcr is jcr of the newly migrated job.
    */
   if (mig_jcr) {
      mig_jcr->JobFiles = jcr->JobFiles = jcr->SDJobFiles;
      mig_jcr->JobBytes = jcr->JobBytes = jcr->SDJobBytes;
      mig_jcr->VolSessionId = jcr->VolSessionId;
      mig_jcr->VolSessionTime = jcr->VolSessionTime;
      mig_jcr->jr.RealEndTime = 0; 
      mig_jcr->jr.PriorJobId = jcr->previous_jr.JobId;

      set_jcr_job_status(mig_jcr, TermCode);

  
      update_job_end_record(mig_jcr);
     
      /* Update final items to set them to the previous job's values */
      Mmsg(query, "UPDATE Job SET StartTime='%s',EndTime='%s',"
                  "JobTDate=%s WHERE JobId=%s", 
         jcr->previous_jr.cStartTime, jcr->previous_jr.cEndTime, 
         edit_uint64(jcr->previous_jr.JobTDate, ec1),
         edit_uint64(mig_jcr->jr.JobId, ec2));
      db_sql_query(mig_jcr->db, query.c_str(), NULL, NULL);

      /* Now marke the previous job as migrated */
      Mmsg(query, "UPDATE Job SET Type='%c' WHERE JobId=%s",
           (char)JT_MIGRATED_JOB, edit_uint64(jcr->previous_jr.JobId, ec1));
      db_sql_query(mig_jcr->db, query.c_str(), NULL, NULL);

      if (!db_get_job_record(jcr, jcr->db, &jcr->jr)) {
         Jmsg(jcr, M_WARNING, 0, _("Error getting job record for stats: %s"),
            db_strerror(jcr->db));
         set_jcr_job_status(jcr, JS_ErrorTerminated);
      }

      bstrncpy(mr.VolumeName, jcr->VolumeName, sizeof(mr.VolumeName));
      if (!db_get_media_record(jcr, jcr->db, &mr)) {
         Jmsg(jcr, M_WARNING, 0, _("Error getting Media record for Volume \"%s\": ERR=%s"),
            mr.VolumeName, db_strerror(jcr->db));
         set_jcr_job_status(jcr, JS_ErrorTerminated);
      }

      update_bootstrap_file(mig_jcr);

      if (!db_get_job_volume_names(mig_jcr, mig_jcr->db, mig_jcr->jr.JobId, &mig_jcr->VolumeName)) {
         /*
          * Note, if the job has erred, most likely it did not write any
          *  tape, so suppress this "error" message since in that case
          *  it is normal.  Or look at it the other way, only for a
          *  normal exit should we complain about this error.
          */
         if (jcr->JobStatus == JS_Terminated && jcr->jr.JobBytes) {
            Jmsg(jcr, M_ERROR, 0, "%s", db_strerror(mig_jcr->db));
         }
         mig_jcr->VolumeName[0] = 0;         /* none */
      }
  }

   msg_type = M_INFO;                 /* by default INFO message */
   switch (jcr->JobStatus) {
   case JS_Terminated:
      if (jcr->Errors || jcr->SDErrors) {
         term_msg = _("%s OK -- with warnings");
      } else {
         term_msg = _("%s OK");
      }
      break;
   case JS_FatalError:
   case JS_ErrorTerminated:
      term_msg = _("*** %s Error ***");
      msg_type = M_ERROR;          /* Generate error message */
      if (jcr->store_bsock) {
         bnet_sig(jcr->store_bsock, BNET_TERMINATE);
         if (jcr->SD_msg_chan) {
            pthread_cancel(jcr->SD_msg_chan);
         }
      }
      break;
   case JS_Canceled:
      term_msg = _("%s Canceled");
      if (jcr->store_bsock) {
         bnet_sig(jcr->store_bsock, BNET_TERMINATE);
         if (jcr->SD_msg_chan) {
            pthread_cancel(jcr->SD_msg_chan);
         }
      }
      break;
   default:
      term_msg = _("Inappropriate %s term code");
      break;
   }
   bsnprintf(term_code, sizeof(term_code), term_msg, "Migration");
   bstrftimes(sdt, sizeof(sdt), jcr->jr.StartTime);
   bstrftimes(edt, sizeof(edt), jcr->jr.EndTime);
   RunTime = jcr->jr.EndTime - jcr->jr.StartTime;
   if (RunTime <= 0) {
      kbps = 0;
   } else {
      kbps = (double)jcr->SDJobBytes / (1000 * RunTime);
   }


   jobstatus_to_ascii(jcr->SDJobStatus, sd_term_msg, sizeof(sd_term_msg));

   Jmsg(jcr, msg_type, 0, _("Bacula %s (%s): %s\n"
"  Prev Backup JobId:      %s\n"
"  New Backup JobId:       %s\n"
"  Migration JobId:        %s\n"
"  Migration Job:          %s\n"
"  Backup Level:           %s%s\n"
"  Client:                 %s\n"
"  FileSet:                \"%s\" %s\n"
"  Pool:                   \"%s\" (From %s)\n"
"  Storage:                \"%s\" (From %s)\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Elapsed time:           %s\n"
"  Priority:               %d\n"
"  SD Files Written:       %s\n"
"  SD Bytes Written:       %s (%sB)\n"
"  Rate:                   %.1f KB/s\n"
"  Volume name(s):         %s\n"
"  Volume Session Id:      %d\n"
"  Volume Session Time:    %d\n"
"  Last Volume Bytes:      %s (%sB)\n"
"  SD Errors:              %d\n"
"  SD termination status:  %s\n"
"  Termination:            %s\n\n"),
   VERSION,
   LSMDATE,
        edt, 
        mig_jcr ? edit_uint64(jcr->previous_jr.JobId, ec6) : "0", 
        mig_jcr ? edit_uint64(mig_jcr->jr.JobId, ec7) : "0",
        edit_uint64(jcr->jr.JobId, ec8),
        jcr->jr.Job,
        level_to_str(jcr->JobLevel), jcr->since,
        jcr->client->name(),
        jcr->fileset->name(), jcr->FSCreateTime,
        jcr->pool->name(), jcr->pool_source,
        jcr->wstore->name(), jcr->storage_source,
        sdt,
        edt,
        edit_utime(RunTime, elapsed, sizeof(elapsed)),
        jcr->JobPriority,
        edit_uint64_with_commas(jcr->SDJobFiles, ec1),
        edit_uint64_with_commas(jcr->SDJobBytes, ec2),
        edit_uint64_with_suffix(jcr->SDJobBytes, ec3),
        (float)kbps,
        mig_jcr ? mig_jcr->VolumeName : "",
        jcr->VolSessionId,
        jcr->VolSessionTime,
        edit_uint64_with_commas(mr.VolBytes, ec4),
        edit_uint64_with_suffix(mr.VolBytes, ec5),
        jcr->SDErrors,
        sd_term_msg,
        term_code);

   Dmsg1(100, "migrate_cleanup() mig_jcr=0x%x\n", jcr->mig_jcr);
   if (jcr->mig_jcr) {
      free_jcr(jcr->mig_jcr);
      jcr->mig_jcr = NULL;
   }
   Dmsg0(100, "Leave migrate_cleanup()\n");
}
