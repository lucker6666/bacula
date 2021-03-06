/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2010 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *   Bacula Director -- verify.c -- responsible for running file verification
 *
 *     Kern Sibbald, October MM
 *
 *  Basic tasks done here:
 *     Open DB
 *     Open connection with File daemon and pass him commands
 *       to do the verify.
 *     When the File daemon sends the attributes, compare them to
 *       what is in the DB.
 *
 */


#include "bacula.h"
#include "dird.h"
#include "findlib/find.h"

/* Commands sent to File daemon */
static char verifycmd[] = "verify level=%s\n";
static char storaddr[]  = "storage address=%s port=%d ssl=0 Authorization=%s\n";

/* Responses received from File daemon */
static char OKverify[]  = "2000 OK verify\n";
static char OKstore[]   = "2000 OK storage\n";

/* Responses received from the Storage daemon */
static char OKbootstrap[] = "3000 OK bootstrap\n";

/* Forward referenced functions */
static void prt_fname(JCR *jcr);
static int missing_handler(void *ctx, int num_fields, char **row);


/* 
 * Called here before the job is run to do the job
 *   specific setup.
 */
bool do_verify_init(JCR *jcr) 
{
   if (!allow_duplicate_job(jcr)) {
      return false;
   }
   switch (jcr->getJobLevel()) {
   case L_VERIFY_INIT:
   case L_VERIFY_CATALOG:
   case L_VERIFY_DISK_TO_CATALOG:
      free_rstorage(jcr);
      free_wstorage(jcr);
      break;
   case L_VERIFY_VOLUME_TO_CATALOG:
      free_wstorage(jcr);
      break;
   case L_VERIFY_DATA:
      break;
   default:
      Jmsg2(jcr, M_FATAL, 0, _("Unimplemented Verify level %d(%c)\n"), jcr->getJobLevel(),
         jcr->getJobLevel());
      return false;
   }
   return true;
}


/*
 * Do a verification of the specified files against the Catlaog
 *
 *  Returns:  false on failure
 *            true  on success
 */
bool do_verify(JCR *jcr)
{
   const char *level;
   BSOCK   *fd;
   int stat;
   char ed1[100];
   JOB_DBR jr;
   JobId_t verify_jobid = 0;
   const char *Name;

   free_wstorage(jcr);                   /* we don't write */

   memset(&jcr->previous_jr, 0, sizeof(jcr->previous_jr));

   /*
    * Find JobId of last job that ran. Note, we do this when
    *   the job actually starts running, not at schedule time,
    *   so that we find the last job that terminated before
    *   this job runs rather than before it is scheduled. This
    *   permits scheduling a Backup and Verify at the same time,
    *   but with the Verify at a lower priority.
    *
    *   For VERIFY_CATALOG we want the JobId of the last INIT.
    *   For VERIFY_VOLUME_TO_CATALOG, we want the JobId of the
    *       last backup Job.
    */
   if (jcr->getJobLevel() == L_VERIFY_CATALOG ||
       jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG ||
       jcr->getJobLevel() == L_VERIFY_DISK_TO_CATALOG) {
      memcpy(&jr, &jcr->jr, sizeof(jr));
      if (jcr->verify_job &&
          (jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG ||
           jcr->getJobLevel() == L_VERIFY_DISK_TO_CATALOG)) {
         Name = jcr->verify_job->name();  
      } else {
         Name = NULL;
      }
      Dmsg1(100, "find last jobid for: %s\n", NPRT(Name));
      if (!db_find_last_jobid(jcr, jcr->db, Name, &jr)) {
         if (jcr->getJobLevel() == L_VERIFY_CATALOG) {
            Jmsg(jcr, M_FATAL, 0, _(
                 "Unable to find JobId of previous InitCatalog Job.\n"
                 "Please run a Verify with Level=InitCatalog before\n"
                 "running the current Job.\n"));
          } else {
            Jmsg(jcr, M_FATAL, 0, _(
                 "Unable to find JobId of previous Job for this client.\n"));
         }
         return false;
      }
      verify_jobid = jr.JobId;
      Dmsg1(100, "Last full jobid=%d\n", verify_jobid);
   }
   /*
    * Now get the job record for the previous backup that interests
    *   us. We use the verify_jobid that we found above.
    */
   if (jcr->getJobLevel() == L_VERIFY_CATALOG ||
       jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG ||
       jcr->getJobLevel() == L_VERIFY_DISK_TO_CATALOG) {
      jcr->previous_jr.JobId = verify_jobid;
      if (!db_get_job_record(jcr, jcr->db, &jcr->previous_jr)) {
         Jmsg(jcr, M_FATAL, 0, _("Could not get job record for previous Job. ERR=%s"),
              db_strerror(jcr->db));
         return false;
      }
      if (!(jcr->previous_jr.JobStatus == JS_Terminated ||
            jcr->previous_jr.JobStatus == JS_Warnings)) {
         Jmsg(jcr, M_FATAL, 0, _("Last Job %d did not terminate normally. JobStatus=%c\n"),
            verify_jobid, jcr->previous_jr.JobStatus);
         return false;
      }
      Jmsg(jcr, M_INFO, 0, _("Verifying against JobId=%d Job=%s\n"),
         jcr->previous_jr.JobId, jcr->previous_jr.Job);
   }

   /*
    * If we are verifying a Volume, we need the Storage
    *   daemon, so open a connection, otherwise, just
    *   create a dummy authorization key (passed to
    *   File daemon but not used).
    */
   if (jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG) {
      int stat;
      /*
       * Note: negative status is an error, zero status, means
       *  no files were backed up, so skip calling SD and
       *  client.
       */
      stat = create_restore_bootstrap_file(jcr);
      if (stat < 0) {                      /* error */
         return false;
      } else if (stat == 0) {              /* No files, nothing to do */
         verify_cleanup(jcr, JS_Terminated); /* clean up */
         return true;                      /* get out */
      }
   } else {
      jcr->sd_auth_key = bstrdup("dummy");    /* dummy Storage daemon key */
   }

   if (jcr->getJobLevel() == L_VERIFY_DISK_TO_CATALOG && jcr->verify_job) {
      jcr->fileset = jcr->verify_job->fileset;
   }
   Dmsg2(100, "ClientId=%u JobLevel=%c\n", jcr->previous_jr.ClientId, jcr->getJobLevel());

   if (!db_update_job_start_record(jcr, jcr->db, &jcr->jr)) {
      Jmsg(jcr, M_FATAL, 0, "%s", db_strerror(jcr->db));
      return false;
   }

   /* Print Job Start message */
   Jmsg(jcr, M_INFO, 0, _("Start Verify JobId=%s Level=%s Job=%s\n"),
      edit_uint64(jcr->JobId, ed1), level_to_str(jcr->getJobLevel()), jcr->Job);

   if (jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG) {
      BSOCK *sd;
      /*
       * Start conversation with Storage daemon
       */
      set_jcr_job_status(jcr, JS_Blocked);
      if (!connect_to_storage_daemon(jcr, 10, SDConnectTimeout, 1)) {
         return false;
      }
      /*
       * Now start a job with the Storage daemon
       */
      if (!start_storage_daemon_job(jcr, jcr->rstorage, NULL)) {
         return false;
      }
      sd = jcr->store_bsock;
      /*
       * Send the bootstrap file -- what Volumes/files to restore
       */
      if (!send_bootstrap_file(jcr, sd) ||
          !response(jcr, sd, OKbootstrap, "Bootstrap", DISPLAY_ERROR)) {
         goto bail_out;
      }
      if (!sd->fsend("run")) {
         return false;
      }
      /*
       * Now start a Storage daemon message thread
       */
      if (!start_storage_daemon_message_thread(jcr)) {
         return false;
      }
      Dmsg0(50, "Storage daemon connection OK\n");

   }
   /*
    * OK, now connect to the File daemon
    *  and ask him for the files.
    */
   set_jcr_job_status(jcr, JS_Blocked);
   if (!connect_to_file_daemon(jcr, 10, FDConnectTimeout, 1)) {
      goto bail_out;
   }

   set_jcr_job_status(jcr, JS_Running);
   fd = jcr->file_bsock;


   Dmsg0(30, ">filed: Send include list\n");
   if (!send_include_list(jcr)) {
      goto bail_out;
   }

   Dmsg0(30, ">filed: Send exclude list\n");
   if (!send_exclude_list(jcr)) {
      goto bail_out;
   }

   /*
    * Send Level command to File daemon, as well
    *   as the Storage address if appropriate.
    */
   switch (jcr->getJobLevel()) {
   case L_VERIFY_INIT:
      level = "init";
      break;
   case L_VERIFY_CATALOG:
      level = "catalog";
      break;
   case L_VERIFY_VOLUME_TO_CATALOG:
      /*
       * send Storage daemon address to the File daemon
       */
      if (jcr->rstore->SDDport == 0) {
         jcr->rstore->SDDport = jcr->rstore->SDport;
      }
      bnet_fsend(fd, storaddr, jcr->rstore->address, 
                 jcr->rstore->SDDport, jcr->sd_auth_key);
      if (!response(jcr, fd, OKstore, "Storage", DISPLAY_ERROR)) {
         goto bail_out;
      }

      if (!jcr->RestoreBootstrap) {
         Jmsg0(jcr, M_FATAL, 0, _("Deprecated feature ... use bootstrap.\n"));
         goto bail_out;
      }

      level = "volume";
      break;
   case L_VERIFY_DATA:
      level = "data";
      break;
   case L_VERIFY_DISK_TO_CATALOG:
      level="disk_to_catalog";
      break;
   default:
      Jmsg2(jcr, M_FATAL, 0, _("Unimplemented Verify level %d(%c)\n"), jcr->getJobLevel(),
         jcr->getJobLevel());
      goto bail_out;
   }

   if (!send_runscripts_commands(jcr)) {
      goto bail_out;
   }

   /*
    * Send verify command/level to File daemon
    */
   fd->fsend(verifycmd, level);
   if (!response(jcr, fd, OKverify, "Verify", DISPLAY_ERROR)) {
      goto bail_out;
   }

   /*
    * Now get data back from File daemon and
    *  compare it to the catalog or store it in the
    *  catalog depending on the run type.
    */
   /* Compare to catalog */
   switch (jcr->getJobLevel()) {
   case L_VERIFY_CATALOG:
      Dmsg0(10, "Verify level=catalog\n");
      jcr->sd_msg_thread_done = true;   /* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_compare_to_catalog(jcr, jcr->previous_jr.JobId);
      break;

   case L_VERIFY_VOLUME_TO_CATALOG:
      Dmsg0(10, "Verify level=volume\n");
      get_attributes_and_compare_to_catalog(jcr, jcr->previous_jr.JobId);
      break;

   case L_VERIFY_DISK_TO_CATALOG:
      Dmsg0(10, "Verify level=disk_to_catalog\n");
      jcr->sd_msg_thread_done = true;   /* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_compare_to_catalog(jcr, jcr->previous_jr.JobId);
      break;

   case L_VERIFY_INIT:
      /* Build catalog */
      Dmsg0(10, "Verify level=init\n");
      jcr->sd_msg_thread_done = true;   /* no SD msg thread, so it is done */
      jcr->SDJobStatus = JS_Terminated;
      get_attributes_and_put_in_catalog(jcr);
      db_end_transaction(jcr, jcr->db);   /* terminate any open transaction */
      db_write_batch_file_records(jcr);
      break;

   default:
      Jmsg1(jcr, M_FATAL, 0, _("Unimplemented verify level %d\n"), jcr->getJobLevel());
      goto bail_out;
   }

   stat = wait_for_job_termination(jcr);
   verify_cleanup(jcr, stat);
   return true;

bail_out:
   return false;
}


/*
 * Release resources allocated during backup.
 *
 */
void verify_cleanup(JCR *jcr, int TermCode)
{
   char sdt[50], edt[50];
   char ec1[30], ec2[30];
   char term_code[100], fd_term_msg[100], sd_term_msg[100];
   const char *term_msg;
   int msg_type;
   JobId_t JobId;
   const char *Name;

// Dmsg1(100, "Enter verify_cleanup() TermCod=%d\n", TermCode);

   Dmsg3(900, "JobLevel=%c Expected=%u JobFiles=%u\n", jcr->getJobLevel(),
      jcr->ExpectedFiles, jcr->JobFiles);
   if (jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG &&
       jcr->ExpectedFiles != jcr->JobFiles) {
      TermCode = JS_ErrorTerminated;
   }

   JobId = jcr->jr.JobId;

   update_job_end(jcr, TermCode);

   if (job_canceled(jcr)) {
      cancel_storage_daemon_job(jcr);
   }

   if (jcr->unlink_bsr && jcr->RestoreBootstrap) {
      unlink(jcr->RestoreBootstrap);
      jcr->unlink_bsr = false;
   }

   msg_type = M_INFO;                 /* by default INFO message */
   switch (TermCode) {
   case JS_Terminated:
      term_msg = _("Verify OK");
      break;
   case JS_FatalError:
   case JS_ErrorTerminated:
      term_msg = _("*** Verify Error ***");
      msg_type = M_ERROR;          /* Generate error message */
      break;
   case JS_Error:
      term_msg = _("Verify warnings");
      break;
   case JS_Canceled:
      term_msg = _("Verify Canceled");
      break;
   case JS_Differences:
      term_msg = _("Verify Differences");
      break;
   default:
      term_msg = term_code;
      bsnprintf(term_code, sizeof(term_code),
                _("Inappropriate term code: %d %c\n"), TermCode, TermCode);
      break;
   }
   bstrftimes(sdt, sizeof(sdt), jcr->jr.StartTime);
   bstrftimes(edt, sizeof(edt), jcr->jr.EndTime);
   if (jcr->verify_job) {
      Name = jcr->verify_job->hdr.name;
   } else {
      Name = "";
   }

   jobstatus_to_ascii(jcr->FDJobStatus, fd_term_msg, sizeof(fd_term_msg));
   if (jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG) {
      jobstatus_to_ascii(jcr->SDJobStatus, sd_term_msg, sizeof(sd_term_msg));
      Jmsg(jcr, msg_type, 0, _("%s %s %s (%s): %s\n"
"  Build OS:               %s %s %s\n"
"  JobId:                  %d\n"
"  Job:                    %s\n"
"  FileSet:                %s\n"
"  Verify Level:           %s\n"
"  Client:                 %s\n"
"  Verify JobId:           %d\n"
"  Verify Job:             %s\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Files Expected:         %s\n"
"  Files Examined:         %s\n"
"  Non-fatal FD errors:    %d\n"
"  FD termination status:  %s\n"
"  SD termination status:  %s\n"
"  Termination:            %s\n\n"),
           BACULA, my_name, VERSION, LSMDATE, edt,
           HOST_OS, DISTNAME, DISTVER,
           jcr->jr.JobId,
           jcr->jr.Job,
           jcr->fileset->hdr.name,
           level_to_str(jcr->getJobLevel()),
           jcr->client->hdr.name,
           jcr->previous_jr.JobId,
           Name,
           sdt,
           edt,
           edit_uint64_with_commas(jcr->ExpectedFiles, ec1),
           edit_uint64_with_commas(jcr->JobFiles, ec2),
           jcr->JobErrors,
           fd_term_msg,
           sd_term_msg,
           term_msg);
   } else {
      Jmsg(jcr, msg_type, 0, _("%s %s %s (%s): %s\n"
"  Build:                  %s %s %s\n"
"  JobId:                  %d\n"
"  Job:                    %s\n"
"  FileSet:                %s\n"
"  Verify Level:           %s\n"
"  Client:                 %s\n"
"  Verify JobId:           %d\n"
"  Verify Job:             %s\n"
"  Start time:             %s\n"
"  End time:               %s\n"
"  Files Examined:         %s\n"
"  Non-fatal FD errors:    %d\n"
"  FD termination status:  %s\n"
"  Termination:            %s\n\n"),
           BACULA, my_name, VERSION, LSMDATE, edt,
           HOST_OS, DISTNAME, DISTVER,
           jcr->jr.JobId,
           jcr->jr.Job,
           jcr->fileset->hdr.name,
           level_to_str(jcr->getJobLevel()),
           jcr->client->name(),
           jcr->previous_jr.JobId,
           Name,
           sdt,
           edt,
           edit_uint64_with_commas(jcr->JobFiles, ec1),
           jcr->JobErrors,
           fd_term_msg,
           term_msg);
   }
   Dmsg0(100, "Leave verify_cleanup()\n");
}

/*
 * This routine is called only during a Verify
 */
void get_attributes_and_compare_to_catalog(JCR *jcr, JobId_t JobId)
{
   BSOCK   *fd;
   int n, len;
   FILE_DBR fdbr;
   struct stat statf;                 /* file stat */
   struct stat statc;                 /* catalog stat */
   char buf[MAXSTRING];
   POOLMEM *fname = get_pool_memory(PM_MESSAGE);
   int do_Digest = CRYPTO_DIGEST_NONE;
   int32_t file_index = 0;

   memset(&fdbr, 0, sizeof(FILE_DBR));
   fd = jcr->file_bsock;
   fdbr.JobId = JobId;
   jcr->FileIndex = 0;

   Dmsg0(20, "bdird: waiting to receive file attributes\n");
   /*
    * Get Attributes and Signature from File daemon
    * We expect:
    *   FileIndex
    *   Stream
    *   Options or Digest (MD5/SHA1)
    *   Filename
    *   Attributes
    *   Link name  ???
    */
   while ((n=bget_dirmsg(fd)) >= 0 && !job_canceled(jcr)) {
      int stream;
      char *attr, *p, *fn;
      char Opts_Digest[MAXSTRING];        /* Verify Opts or MD5/SHA1 digest */

      if (job_canceled(jcr)) {
         return;
      }
      fname = check_pool_memory_size(fname, fd->msglen);
      jcr->fname = check_pool_memory_size(jcr->fname, fd->msglen);
      Dmsg1(200, "Atts+Digest=%s\n", fd->msg);
      if ((len = sscanf(fd->msg, "%ld %d %100s", &file_index, &stream,
            fname)) != 3) {
         Jmsg3(jcr, M_FATAL, 0, _("bird<filed: bad attributes, expected 3 fields got %d\n"
" mslen=%d msg=%s\n"), len, fd->msglen, fd->msg);
         return;
      }
      /*
       * We read the Options or Signature into fname
       *  to prevent overrun, now copy it to proper location.
       */
      bstrncpy(Opts_Digest, fname, sizeof(Opts_Digest));
      p = fd->msg;
      skip_nonspaces(&p);             /* skip FileIndex */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Stream */
      skip_spaces(&p);
      skip_nonspaces(&p);             /* skip Opts_Digest */
      p++;                            /* skip space */
      fn = fname;
      while (*p != 0) {
         *fn++ = *p++;                /* copy filename */
      }
      *fn = *p++;                     /* term filename and point to attribs */
      attr = p;
      /*
       * Got attributes stream, decode it
       */
      if (stream == STREAM_UNIX_ATTRIBUTES || stream == STREAM_UNIX_ATTRIBUTES_EX) {
         int32_t LinkFIf, LinkFIc;
         Dmsg2(400, "file_index=%d attr=%s\n", file_index, attr);
         jcr->JobFiles++;
         jcr->FileIndex = file_index;    /* remember attribute file_index */
         jcr->previous_jr.FileIndex = file_index;
         decode_stat(attr, &statf, &LinkFIf);  /* decode file stat packet */
         do_Digest = CRYPTO_DIGEST_NONE;
         jcr->fn_printed = false;
         pm_strcpy(jcr->fname, fname);  /* move filename into JCR */

         Dmsg2(040, "dird<filed: stream=%d %s\n", stream, jcr->fname);
         Dmsg1(020, "dird<filed: attr=%s\n", attr);

         /*
          * Find equivalent record in the database
          */
         fdbr.FileId = 0;
         if (!db_get_file_attributes_record(jcr, jcr->db, jcr->fname,
              &jcr->previous_jr, &fdbr)) {
            Jmsg(jcr, M_INFO, 0, _("New file: %s\n"), jcr->fname);
            Dmsg1(020, _("File not in catalog: %s\n"), jcr->fname);
            set_jcr_job_status(jcr, JS_Differences);
            continue;
         } else {
            /*
             * mark file record as visited by stuffing the
             * current JobId, which is unique, into the MarkId field.
             */
            db_mark_file_record(jcr, jcr->db, fdbr.FileId, jcr->JobId);
         }

         Dmsg3(400, "Found %s in catalog. inx=%d Opts=%s\n", jcr->fname,
            file_index, Opts_Digest);
         decode_stat(fdbr.LStat, &statc, &LinkFIc); /* decode catalog stat */
         /*
          * Loop over options supplied by user and verify the
          * fields he requests.
          */
         for (p=Opts_Digest; *p; p++) {
            char ed1[30], ed2[30];
            switch (*p) {
            case 'i':                /* compare INODEs */
               if (statc.st_ino != statf.st_ino) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_ino   differ. Cat: %s File: %s\n"),
                     edit_uint64((uint64_t)statc.st_ino, ed1),
                     edit_uint64((uint64_t)statf.st_ino, ed2));
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'p':                /* permissions bits */
               if (statc.st_mode != statf.st_mode) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_mode  differ. Cat: %x File: %x\n"),
                     (uint32_t)statc.st_mode, (uint32_t)statf.st_mode);
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'n':                /* number of links */
               if (statc.st_nlink != statf.st_nlink) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_nlink differ. Cat: %d File: %d\n"),
                     (uint32_t)statc.st_nlink, (uint32_t)statf.st_nlink);
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'u':                /* user id */
               if (statc.st_uid != statf.st_uid) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_uid   differ. Cat: %u File: %u\n"),
                     (uint32_t)statc.st_uid, (uint32_t)statf.st_uid);
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'g':                /* group id */
               if (statc.st_gid != statf.st_gid) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_gid   differ. Cat: %u File: %u\n"),
                     (uint32_t)statc.st_gid, (uint32_t)statf.st_gid);
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 's':                /* size */
               if (statc.st_size != statf.st_size) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_size  differ. Cat: %s File: %s\n"),
                     edit_uint64((uint64_t)statc.st_size, ed1),
                     edit_uint64((uint64_t)statf.st_size, ed2));
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'a':                /* access time */
               if (statc.st_atime != statf.st_atime) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_atime differs\n"));
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'm':
               if (statc.st_mtime != statf.st_mtime) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_mtime differs\n"));
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'c':                /* ctime */
               if (statc.st_ctime != statf.st_ctime) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_ctime differs\n"));
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case 'd':                /* file size decrease */
               if (statc.st_size > statf.st_size) {
                  prt_fname(jcr);
                  Jmsg(jcr, M_INFO, 0, _("      st_size  decrease. Cat: %s File: %s\n"),
                     edit_uint64((uint64_t)statc.st_size, ed1),
                     edit_uint64((uint64_t)statf.st_size, ed2));
                  set_jcr_job_status(jcr, JS_Differences);
               }
               break;
            case '5':                /* compare MD5 */
               Dmsg1(500, "set Do_MD5 for %s\n", jcr->fname);
               do_Digest = CRYPTO_DIGEST_MD5;
               break;
            case '1':                 /* compare SHA1 */
               do_Digest = CRYPTO_DIGEST_SHA1;
               break;
            case ':':
            case 'V':
            default:
               break;
            }
         }
      /*
       * Got Digest Signature from Storage daemon
       *  It came across in the Opts_Digest field.
       */
      } else if (crypto_digest_stream_type(stream) != CRYPTO_DIGEST_NONE) {
         Dmsg2(400, "stream=Digest inx=%d Digest=%s\n", file_index, Opts_Digest);
         /*
          * When ever we get a digest it MUST have been
          * preceded by an attributes record, which sets attr_file_index
          */
         if (jcr->FileIndex != (uint32_t)file_index) {
            Jmsg2(jcr, M_FATAL, 0, _("MD5/SHA1 index %d not same as attributes %d\n"),
               file_index, jcr->FileIndex);
            return;
         }
         if (do_Digest != CRYPTO_DIGEST_NONE) {
            db_escape_string(jcr, jcr->db, buf, Opts_Digest, strlen(Opts_Digest));
            if (strcmp(buf, fdbr.Digest) != 0) {
               prt_fname(jcr);
               Jmsg(jcr, M_INFO, 0, _("      %s differs. File=%s Cat=%s\n"),
                    stream_to_ascii(stream), buf, fdbr.Digest);
               set_jcr_job_status(jcr, JS_Differences);
            }
            do_Digest = CRYPTO_DIGEST_NONE;
         }
      }
      jcr->JobFiles = file_index;
   }
   if (is_bnet_error(fd)) {
      berrno be;
      Jmsg2(jcr, M_FATAL, 0, _("bdird<filed: bad attributes from filed n=%d : %s\n"),
                        n, be.bstrerror());
      return;
   }

   /* Now find all the files that are missing -- i.e. all files in
    *  the database where the MarkId != current JobId
    */
   jcr->fn_printed = false;
   bsnprintf(buf, sizeof(buf),
      "SELECT Path.Path,Filename.Name FROM File,Path,Filename "
      "WHERE File.JobId=%d AND File.FileIndex > 0 "
      "AND File.MarkId!=%d AND File.PathId=Path.PathId "
      "AND File.FilenameId=Filename.FilenameId",
         JobId, jcr->JobId);
   /* missing_handler is called for each file found */
   db_sql_query(jcr->db, buf, missing_handler, (void *)jcr);
   if (jcr->fn_printed) {
      set_jcr_job_status(jcr, JS_Differences);
   }
   free_pool_memory(fname);
}

/*
 * We are called here for each record that matches the above
 *  SQL query -- that is for each file contained in the Catalog
 *  that was not marked earlier. This means that the file in
 *  question is a missing file (in the Catalog but not on Disk).
 */
static int missing_handler(void *ctx, int num_fields, char **row)
{
   JCR *jcr = (JCR *)ctx;

   if (job_canceled(jcr)) {
      return 1;
   }
   if (!jcr->fn_printed) {
      Qmsg(jcr, M_WARNING, 0, _("The following files are in the Catalog but not on %s:\n"),
          jcr->getJobLevel() == L_VERIFY_VOLUME_TO_CATALOG ? "the Volume(s)" : "disk");
      jcr->fn_printed = true;
   }
   Qmsg(jcr, M_INFO, 0, "      %s%s\n", row[0]?row[0]:"", row[1]?row[1]:"");
   return 0;
}


/*
 * Print filename for verify
 */
static void prt_fname(JCR *jcr)
{
   if (!jcr->fn_printed) {
      Jmsg(jcr, M_INFO, 0, _("File: %s\n"), jcr->fname);
      jcr->fn_printed = true;
   }
}
