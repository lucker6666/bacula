/*
 * Manipulation routines for Job Control Records
 *
 *  Kern E. Sibbald, December 2000
 *
 *  Version $Id$
 *
 *  These routines are thread safe.
 *
 */
/*
   Copyright (C) 2000-2003 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"
#include "jcr.h"

/* External variables we reference */
extern time_t watchdog_time;

/* Forward referenced functions */
static void timeout_handler(int sig);
static void jcr_timeout_check(watchdog_t *self);

struct s_last_job last_job;    /* last job run by this daemon */
dlist *last_jobs = NULL;
#define MAX_LAST_JOBS 10

static JCR *jobs = NULL;	      /* pointer to JCR chain */

static brwlock_t lock;		      /* lock for last jobs and JCR chain */

void init_last_jobs_list()
{
   int errstat;
   struct s_last_job *job_entry;
   if (!last_jobs) {
      last_jobs = new dlist(job_entry,	&job_entry->link);
      memset(&last_job, 0, sizeof(last_job));
   }
   if ((errstat=rwl_init(&lock)) != 0) {
      Emsg1(M_ABORT, 0, _("Unable to initialize jcr_chain lock. ERR=%s\n"), 
	    strerror(errstat));
   }

}

void term_last_jobs_list()
{
   char *je;
   if (last_jobs) {
      foreach_dlist(je, last_jobs) {
	 free(je);		       
      }
      delete last_jobs;
      last_jobs = NULL;
   }
   rwl_destroy(&lock);
}

void lock_last_jobs_list() 
{
   /* Use jcr chain mutex */
   lock_jcr_chain();
}

void unlock_last_jobs_list() 
{
   /* Use jcr chain mutex */
   unlock_jcr_chain();
}

/*
 * Create a Job Control Record and link it into JCR chain
 * Returns newly allocated JCR
 * Note, since each daemon has a different JCR, he passes
 *  us the size.
 */
JCR *new_jcr(int size, JCR_free_HANDLER *daemon_free_jcr)
{
   JCR *jcr;
   struct sigaction sigtimer;

   Dmsg0(200, "Enter new_jcr\n");
   jcr = (JCR *)malloc(size);
   memset(jcr, 0, size);
   jcr->my_thread_id = pthread_self();
   jcr->sched_time = time(NULL);
   jcr->daemon_free_jcr = daemon_free_jcr;    /* plug daemon free routine */
   jcr->use_count = 1;
   pthread_mutex_init(&(jcr->mutex), NULL);
   jcr->JobStatus = JS_Created;       /* ready to run */
   jcr->VolumeName = get_pool_memory(PM_FNAME);
   jcr->VolumeName[0] = 0;
   jcr->errmsg = get_pool_memory(PM_MESSAGE);
   jcr->errmsg[0] = 0;
   strcpy(jcr->Job, "*Console*");     /* default */

   sigtimer.sa_flags = 0;
   sigtimer.sa_handler = timeout_handler;
   sigfillset(&sigtimer.sa_mask);
   sigaction(TIMEOUT_SIGNAL, &sigtimer, NULL);

   lock_jcr_chain();
   jcr->prev = NULL;
   jcr->next = jobs;
   if (jobs) {
      jobs->prev = jcr;
   }
   jobs = jcr;
   unlock_jcr_chain();
   return jcr;
}


/*
 * Remove a JCR from the chain
 * NOTE! The chain must be locked prior to calling
 *	 this routine.
 */
static void remove_jcr(JCR *jcr)
{
   Dmsg0(150, "Enter remove_jcr\n");
   if (!jcr) {
      Emsg0(M_ABORT, 0, "NULL jcr.\n");
   }
   if (!jcr->prev) {		      /* if no prev */
      jobs = jcr->next; 	      /* set new head */
   } else {
      jcr->prev->next = jcr->next;    /* update prev */
   }
   if (jcr->next) {
      jcr->next->prev = jcr->prev;
   }
   Dmsg0(150, "Leave remove_jcr\n");
}

/*
 * Free stuff common to all JCRs.  N.B. Be careful to include only
 *  generic stuff in the common part of the jcr. 
 */
static void free_common_jcr(JCR *jcr)
{
   /* Keep some statistics */
   switch (jcr->JobType) {
   case JT_BACKUP:
   case JT_VERIFY:
   case JT_RESTORE:
   case JT_ADMIN:
      last_job.NumJobs++;
      last_job.JobType = jcr->JobType;
      last_job.JobId = jcr->JobId;
      last_job.VolSessionId = jcr->VolSessionId;
      last_job.VolSessionTime = jcr->VolSessionTime;
      bstrncpy(last_job.Job, jcr->Job, sizeof(last_job.Job));
      last_job.JobFiles = jcr->JobFiles;
      last_job.JobBytes = jcr->JobBytes;
      last_job.JobStatus = jcr->JobStatus;
      last_job.JobLevel = jcr->JobLevel;
      last_job.start_time = jcr->start_time;
      last_job.end_time = time(NULL);
      break;
   default:
      break;
   }
   pthread_mutex_destroy(&jcr->mutex);

   close_msg(jcr);		      /* close messages for this job */

   /* do this after closing messages */
   if (jcr->client_name) {
      free_pool_memory(jcr->client_name);
      jcr->client_name = NULL;
   }

   if (jcr->sd_auth_key) {
      free(jcr->sd_auth_key);
      jcr->sd_auth_key = NULL;
   }
   if (jcr->VolumeName) {
      free_pool_memory(jcr->VolumeName);
      jcr->VolumeName = NULL;
   }

   if (jcr->dir_bsock) {
      bnet_close(jcr->dir_bsock);
      jcr->dir_bsock = NULL;
   }
   if (jcr->errmsg) {
      free_pool_memory(jcr->errmsg);
      jcr->errmsg = NULL;
   }
   if (jcr->where) {
      free(jcr->where);
      jcr->where = NULL;
   }
   if (jcr->cached_path) {
      free_pool_memory(jcr->cached_path);
      jcr->cached_path = NULL;
      jcr->cached_pnl = 0;
   }
   free_getuser_cache();
   free_getgroup_cache();
   free(jcr);
}

/* 
 * Global routine to free a jcr
 */
#ifdef DEBUG
void b_free_jcr(char *file, int line, JCR *jcr)
{
   Dmsg3(200, "Enter free_jcr 0x%x from %s:%d\n", jcr, file, line);

#else

void free_jcr(JCR *jcr)
{

   Dmsg1(200, "Enter free_jcr 0x%x\n", jcr);

#endif
   struct s_last_job *je;

   lock_jcr_chain();
   jcr->use_count--;		      /* decrement use count */
   Dmsg3(200, "Dec free_jcr 0x%x use_count=%d jobid=%d\n", jcr, jcr->use_count, jcr->JobId);
   if (jcr->use_count > 0) {	      /* if in use */
      unlock_jcr_chain();
      Dmsg2(200, "free_jcr 0x%x use_count=%d\n", jcr, jcr->use_count);
      return;
   }
   remove_jcr(jcr);

   Dmsg1(200, "End job=%d\n", jcr->JobId);
   if (jcr->daemon_free_jcr) {
      jcr->daemon_free_jcr(jcr);      /* call daemon free routine */
   }

   free_common_jcr(jcr);

   /* Keep list of last jobs, but not Console where JobId==0 */
   if (last_job.JobId > 0) {
      je = (struct s_last_job *)malloc(sizeof(struct s_last_job));
      memcpy((char *)je, (char *)&last_job, sizeof(last_job));
      if (!last_jobs) {
	 init_last_jobs_list();
      }
      last_jobs->append(je);
      if (last_jobs->size() > MAX_LAST_JOBS) {
	 last_jobs->remove(last_jobs->first());
      }
      last_job.JobId = 0;	      /* zap last job */
   }
   close_msg(NULL);		      /* flush any daemon messages */
   unlock_jcr_chain();
   Dmsg0(200, "Exit free_jcr\n");
}


/* 
 * Global routine to free a jcr
 *  JCR chain is already locked
 */
void free_locked_jcr(JCR *jcr)
{
   jcr->use_count--;		      /* decrement use count */
   Dmsg2(200, "Dec free_locked_jcr 0x%x use_count=%d\n", jcr, jcr->use_count);
   if (jcr->use_count > 0) {	      /* if in use */
      return;
   }
   remove_jcr(jcr);
   jcr->daemon_free_jcr(jcr);	      /* call daemon free routine */
   free_common_jcr(jcr);
}




/*
 * Given a JobId, find the JCR	    
 *   Returns: jcr on success
 *	      NULL on failure
 */
JCR *get_jcr_by_id(uint32_t JobId)
{
   JCR *jcr;	   

   lock_jcr_chain();			/* lock chain */
   for (jcr = jobs; jcr; jcr=jcr->next) {
      if (jcr->JobId == JobId) {
	 P(jcr->mutex);
	 jcr->use_count++;
	 V(jcr->mutex);
         Dmsg2(200, "Inc get_jcr 0x%x use_count=%d\n", jcr, jcr->use_count);
	 break;
      }
   }
   unlock_jcr_chain();
   return jcr; 
}

/*
 * Given a SessionId and SessionTime, find the JCR	
 *   Returns: jcr on success
 *	      NULL on failure
 */
JCR *get_jcr_by_session(uint32_t SessionId, uint32_t SessionTime)
{
   JCR *jcr;	   

   lock_jcr_chain();
   for (jcr = jobs; jcr; jcr=jcr->next) {
      if (jcr->VolSessionId == SessionId && 
	  jcr->VolSessionTime == SessionTime) {
	 P(jcr->mutex);
	 jcr->use_count++;
	 V(jcr->mutex);
         Dmsg2(200, "Inc get_jcr 0x%x use_count=%d\n", jcr, jcr->use_count);
	 break;
      }
   }
   unlock_jcr_chain();
   return jcr; 
}


/*
 * Given a Job, find the JCR	  
 *  compares on the number of characters in Job
 *  thus allowing partial matches.
 *   Returns: jcr on success
 *	      NULL on failure
 */
JCR *get_jcr_by_partial_name(char *Job)
{
   JCR *jcr;	   
   int len;

   if (!Job) {
      return NULL;
   }
   lock_jcr_chain();
   len = strlen(Job);
   for (jcr = jobs; jcr; jcr=jcr->next) {
      if (strncmp(Job, jcr->Job, len) == 0) {
	 P(jcr->mutex);
	 jcr->use_count++;
	 V(jcr->mutex);
         Dmsg2(200, "Inc get_jcr 0x%x use_count=%d\n", jcr, jcr->use_count);
	 break;
      }
   }
   unlock_jcr_chain();
   return jcr; 
}


/*
 * Given a Job, find the JCR	  
 *  requires an exact match of names.
 *   Returns: jcr on success
 *	      NULL on failure
 */
JCR *get_jcr_by_full_name(char *Job)
{
   JCR *jcr;	   

   if (!Job) {
      return NULL;
   }
   lock_jcr_chain();
   for (jcr = jobs; jcr; jcr=jcr->next) {
      if (strcmp(jcr->Job, Job) == 0) {
	 P(jcr->mutex);
	 jcr->use_count++;
	 V(jcr->mutex);
         Dmsg2(200, "Inc get_jcr 0x%x use_count=%d\n", jcr, jcr->use_count);
	 break;
      }
   }
   unlock_jcr_chain();
   return jcr; 
}

void set_jcr_job_status(JCR *jcr, int JobStatus)
{
   /*
    * For a set of errors, ... keep the current status
    *   so it isn't lost. For all others, set it.
    */
   switch (jcr->JobStatus) {
   case JS_ErrorTerminated:
   case JS_Error:
   case JS_FatalError:
   case JS_Differences:
   case JS_Canceled:
      break;
   default:
      jcr->JobStatus = JobStatus;
   }
}

/* 
 * Lock the chain
 */
void lock_jcr_chain()
{
   int errstat;
   if ((errstat=rwl_writelock(&lock)) != 0) {
      Emsg1(M_ABORT, 0, "rwl_writelock failure. ERR=%s\n",
	   strerror(errstat));
   }
}

/*
 * Unlock the chain
 */
void unlock_jcr_chain()
{
   int errstat;
   if ((errstat=rwl_writeunlock(&lock)) != 0) {
      Emsg1(M_ABORT, 0, "rwl_writeunlock failure. ERR=%s\n",
	   strerror(errstat));
   }
}


JCR *get_next_jcr(JCR *prev_jcr)
{
   JCR *jcr;

   if (prev_jcr == NULL) {
      jcr = jobs;
   } else {
      jcr = prev_jcr->next;
   }
   if (jcr) {
      P(jcr->mutex);
      jcr->use_count++;
      V(jcr->mutex);
      Dmsg2(200, "Inc get_next_jcr 0x%x use_count=%d\n", jcr, jcr->use_count);
   }
   return jcr;
}

bool init_jcr_subsystem(void)
{
   watchdog_t *wd = new_watchdog();

   wd->one_shot = false;
   wd->interval = 30;	/* FIXME: should be configurable somewhere, even
			 if only with a #define */
   wd->callback = jcr_timeout_check;

   register_watchdog(wd);

   return true;
}

static void jcr_timeout_check(watchdog_t *self)
{
   JCR *jcr;
   BSOCK *fd;
   time_t timer_start;

   Dmsg0(400, "Start JCR timeout checks\n");

   /* Walk through all JCRs checking if any one is 
    * blocked for more than specified max time.
    */
   lock_jcr_chain();
   foreach_jcr(jcr) {
      free_locked_jcr(jcr);	      /* OK to free now cuz chain is locked */
      if (jcr->JobId == 0) {
	 continue;
      }
      fd = jcr->store_bsock;
      if (fd) {
	 timer_start = fd->timer_start;
	 if (timer_start && (watchdog_time - timer_start) > fd->timeout) {
	    fd->timer_start = 0;      /* turn off timer */
	    fd->timed_out = TRUE;
	    Jmsg(jcr, M_ERROR, 0, _(
"Watchdog sending kill after %d secs to thread stalled reading Storage daemon.\n"),
		 watchdog_time - timer_start);
	    pthread_kill(jcr->my_thread_id, TIMEOUT_SIGNAL);
	 }
      }
      fd = jcr->file_bsock;
      if (fd) {
	 timer_start = fd->timer_start;
	 if (timer_start && (watchdog_time - timer_start) > fd->timeout) {
	    fd->timer_start = 0;      /* turn off timer */
	    fd->timed_out = TRUE;
	    Jmsg(jcr, M_ERROR, 0, _(
"Watchdog sending kill after %d secs to thread stalled reading File daemon.\n"),
		 watchdog_time - timer_start);
	    pthread_kill(jcr->my_thread_id, TIMEOUT_SIGNAL);
	 }
      }
      fd = jcr->dir_bsock;
      if (fd) {
	 timer_start = fd->timer_start;
	 if (timer_start && (watchdog_time - timer_start) > fd->timeout) {
	    fd->timer_start = 0;      /* turn off timer */
	    fd->timed_out = TRUE;
	    Jmsg(jcr, M_ERROR, 0, _(
"Watchdog sending kill after %d secs to thread stalled reading Director.\n"),
		 watchdog_time - timer_start);
	    pthread_kill(jcr->my_thread_id, TIMEOUT_SIGNAL);
	 }
      }

   }
   unlock_jcr_chain();

   Dmsg0(200, "Finished JCR timeout checks\n");
}

/*
 * Timeout signal comes here
 */
void timeout_handler(int sig)
{
   return;			      /* thus interrupting the function */
}
