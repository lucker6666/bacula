/*
 *
 *  Higher Level Device routines. 
 *  Knows about Bacula tape labels and such  
 *
 *  NOTE! In general, subroutines that have the word
 *        "device" in the name do locking.  Subroutines
 *        that have the word "dev" in the name do not
 *	  do locking.  Thus if xxx_device() calls
 *	  yyy_dev(), all is OK, but if xxx_device()
 *	  calls yyy_device(), everything will hang.
 *	  Obviously, no zzz_dev() is allowed to call
 *	  a www_device() or everything falls apart. 
 *
 * Concerning the routines lock_device() and block_device()
 *  see the end of this module for details.  In general,
 *  blocking a device leaves it in a state where all threads
 *  other than the current thread block when they attempt to 
 *  lock the device. They remain suspended (blocked) until the device
 *  is unblocked. So, a device is blocked during an operation
 *  that takes a long time (initialization, mounting a new
 *  volume, ...) locking a device is done for an operation
 *  that takes a short time such as writing data to the   
 *  device.
 *
 *
 *   Kern Sibbald, MM, MMI
 *			      
 *   Version $Id$
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

/* Forward referenced functions */

extern char my_name[];
extern int debug_level;

/*
 * This is the dreaded moment. We either have an end of
 * medium condition or worse, and error condition.
 * Attempt to "recover" by obtaining a new Volume.
 *
 * Here are a few things to know:
 *  jcr->VolCatInfo contains the info on the "current" tape for this job.
 *  dev->VolCatInfo contains the info on the tape in the drive.
 *    The tape in the drive could have changed several times since 
 *    the last time the job used it (jcr->VolCatInfo).
 *  jcr->VolumeName is the name of the current/desired tape in the drive.
 *
 * We enter with device locked, and 
 *     exit with device locked.
 *
 * Note, we are called only from one place in block.c
 *
 *  Returns: 1 on success
 *	     0 on failure
 */
int fixup_device_block_write_error(JCR *jcr, DEVICE *dev, DEV_BLOCK *block)
{
   uint32_t stat;
   char PrevVolName[MAX_NAME_LENGTH];
   DEV_BLOCK *label_blk;
   char b1[30], b2[30];
   time_t wait_time;
   char dt[MAX_TIME_LENGTH];

   wait_time = time(NULL);
   stat = status_dev(dev);
   if (!(stat & BMT_EOD)) {
      return 0;                       /* this really shouldn't happen */
   }

   Dmsg0(100, "======= Got EOD ========\n");

   block_device(dev, BST_DOING_ACQUIRE);
   /* Unlock, but leave BLOCKED */
   unlock_device(dev);

   /* Create a jobmedia record for this job */
   if (!dir_create_jobmedia_record(jcr)) {
       Jmsg(jcr, M_ERROR, 0, _("Could not create JobMedia record for Volume=\"%s\" Job=%s\n"),
	    jcr->VolCatInfo.VolCatName, jcr->Job);
       P(dev->mutex);
       unblock_device(dev);
       return 0;
   }

   bstrncpy(dev->VolCatInfo.VolCatStatus, "Full", sizeof(dev->VolCatInfo.VolCatStatus));
   Dmsg2(100, "Call update_vol_info Stat=%s Vol=%s\n", 
      dev->VolCatInfo.VolCatStatus, dev->VolCatInfo.VolCatName);
   dev->VolCatInfo.VolCatFiles = dev->file;   /* set number of files */
   dev->VolCatInfo.VolCatJobs++;	      /* increment number of jobs */
   if (!dir_update_volume_info(jcr, dev, 0)) {	  /* send Volume info to Director */
      P(dev->mutex);
      unblock_device(dev);
      return 0; 		   /* device locked */
   }
   Dmsg0(100, "Back from update_vol_info\n");

   bstrncpy(PrevVolName, dev->VolCatInfo.VolCatName, sizeof(PrevVolName));
   bstrncpy(dev->VolHdr.PrevVolName, PrevVolName, sizeof(dev->VolHdr.PrevVolName));

   label_blk = new_block(dev);

   /* Inform User about end of medium */
   Jmsg(jcr, M_INFO, 0, _("End of medium on Volume \"%s\" Bytes=%s Blocks=%s at %s.\n"), 
	PrevVolName, edit_uint64_with_commas(dev->VolCatInfo.VolCatBytes, b1),
	edit_uint64_with_commas(dev->VolCatInfo.VolCatBlocks, b2),
	bstrftime(dt, sizeof(dt), time(NULL)));

   if (!mount_next_write_volume(jcr, dev, label_blk, 1)) {
      free_block(label_blk);
      P(dev->mutex);
      unblock_device(dev);
      return 0; 		   /* device locked */
   }
   P(dev->mutex);		   /* lock again */

   Jmsg(jcr, M_INFO, 0, _("New volume \"%s\" mounted on device %s at %s.\n"),
      jcr->VolumeName, dev_name(dev), bstrftime(dt, sizeof(dt), time(NULL)));

   /* 
    * If this is a new tape, the label_blk will contain the
    *  label, so write it now. If this is a previously
    *  used tape, mount_next_write_volume() will return an
    *  empty label_blk, and nothing will be written.
    */
   Dmsg0(190, "write label block to dev\n");
   if (!write_block_to_dev(jcr, dev, label_blk)) {
      Pmsg1(0, "write_block_to_device Volume label failed. ERR=%s",
	strerror_dev(dev));
      free_block(label_blk);
      unblock_device(dev);
      return 0; 		   /* device locked */
   }
   free_block(label_blk);

   /* 
    * Walk through all attached jcrs indicating the volume has changed	 
    */
   Dmsg1(100, "Walk attached jcrs. Volume=%s\n", dev->VolCatInfo.VolCatName);
   for (JCR *mjcr=NULL; (mjcr=next_attached_jcr(dev, mjcr)); ) {
      if (mjcr->JobId == 0) {
	 continue;		   /* ignore console */
      }
      mjcr->NewVol = true;
      if (jcr != mjcr) {
	 pm_strcpy(&mjcr->VolumeName, jcr->VolumeName);  /* get a copy of the new volume */
      }
   }

   /* Clear NewVol now because dir_get_volume_info() already done */
   jcr->NewVol = false;
   set_new_volume_parameters(jcr, dev);

   jcr->run_time += time(NULL) - wait_time; /* correct run time for mount wait */

   /* Write overflow block to device */
   Dmsg0(190, "Write overflow block to dev\n");
   if (!write_block_to_dev(jcr, dev, block)) {
      Pmsg1(0, "write_block_to_device overflow block failed. ERR=%s",
	strerror_dev(dev));
      unblock_device(dev);
      return 0; 		   /* device locked */
   }

   unblock_device(dev);
   return 1;				    /* device locked */
}

/*
 * We have a new Volume mounted, so reset the Volume parameters
 *  concerning this job.  The global changes were made earlier
 *  in the dev structure.
 */
void set_new_volume_parameters(JCR *jcr, DEVICE *dev) 
{
   if (jcr->NewVol && !dir_get_volume_info(jcr, GET_VOL_INFO_FOR_WRITE)) {
      Jmsg1(jcr, M_ERROR, 0, "%s", jcr->errmsg);
   }
   /* Set new start/end positions */
   if (dev_state(dev, ST_TAPE)) {
      jcr->StartBlock = dev->block_num;
      jcr->StartFile = dev->file;
   } else {
      jcr->StartBlock = (uint32_t)dev->file_addr;
      jcr->StartFile  = (uint32_t)(dev->file_addr >> 32);
   }
   /* Reset indicies */
   jcr->VolFirstIndex = 0;
   jcr->VolLastIndex = 0;
   jcr->NumVolumes++;
   jcr->NewVol = false;
   jcr->WroteVol = false;
}

/*
 * We are now in a new Volume file, so reset the Volume parameters
 *  concerning this job.  The global changes were made earlier
 *  in the dev structure.
 */
void set_new_file_parameters(JCR *jcr, DEVICE *dev) 
{
   /* Set new start/end positions */
   if (dev_state(dev, ST_TAPE)) {
      jcr->StartBlock = dev->block_num;
      jcr->StartFile = dev->file;
   } else {
      jcr->StartBlock = (uint32_t)dev->file_addr;
      jcr->StartFile  = (uint32_t)(dev->file_addr >> 32);
   }
   /* Reset indicies */
   jcr->VolFirstIndex = 0;
   jcr->VolLastIndex = 0;
   jcr->NewFile = false;
   jcr->WroteVol = false;
}



/*
 *   First Open of the device. Expect dev to already be initialized.  
 *
 *   This routine is used only when the Storage daemon starts 
 *   and always_open is set, and in the stand-alone utility
 *   routines such as bextract.
 *
 *   Note, opening of a normal file is deferred to later so
 *    that we can get the filename; the device_name for
 *    a file is the directory only. 
 *
 *   Retuns: 0 on failure
 *	     1 on success
 */
int first_open_device(DEVICE *dev)
{
   Dmsg0(120, "start open_output_device()\n");
   if (!dev) {
      return 0;
   }

   lock_device(dev);

   /* Defer opening files */
   if (!dev_is_tape(dev)) {
      Dmsg0(129, "Device is file, deferring open.\n");
      unlock_device(dev);
      return 1;
   }

   if (!(dev->state & ST_OPENED)) {
       int mode;
       if (dev_cap(dev, CAP_STREAM)) {
	  mode = OPEN_WRITE_ONLY;
       } else {
	  mode = OPEN_READ_WRITE;
       }
      Dmsg0(129, "Opening device.\n");
      if (open_dev(dev, NULL, mode) < 0) {
         Emsg1(M_FATAL, 0, _("dev open failed: %s\n"), dev->errmsg);
	 unlock_device(dev);
	 return 0;
      }
   }
   Dmsg1(129, "open_dev %s OK\n", dev_name(dev));

   unlock_device(dev);
   return 1;
}

/* 
 * Make sure device is open, if not do so 
 */
int open_device(JCR *jcr, DEVICE *dev)
{
   /* Open device */
   if  (!(dev_state(dev, ST_OPENED))) {
       int mode;
       if (dev_cap(dev, CAP_STREAM)) {
	  mode = OPEN_WRITE_ONLY;
       } else {
	  mode = OPEN_READ_WRITE;
       }
       if (open_dev(dev, jcr->VolCatInfo.VolCatName, mode) < 0) {
          Jmsg2(jcr, M_FATAL, 0, _("Unable to open device %s. ERR=%s\n"), 
	     dev_name(dev), strerror_dev(dev));
	  return 0;
       }
   }
   return 1;
}

/* 
 * When dev_blocked is set, all threads EXCEPT thread with id no_wait_id
 * must wait. The no_wait_id thread is out obtaining a new volume
 * and preparing the label.
 */
void _lock_device(char *file, int line, DEVICE *dev)
{
   int stat;
   Dmsg3(100, "lock %d from %s:%d\n", dev->dev_blocked, file, line);
   P(dev->mutex);
   if (dev->dev_blocked && !pthread_equal(dev->no_wait_id, pthread_self())) {
      dev->num_waiting++;	      /* indicate that I am waiting */
      while (dev->dev_blocked) {
	 if ((stat = pthread_cond_wait(&dev->wait, &dev->mutex)) != 0) {
	    V(dev->mutex);
            Emsg1(M_ABORT, 0, _("pthread_cond_wait failure. ERR=%s\n"),
	       strerror(stat));
	 }
      }
      dev->num_waiting--;	      /* no longer waiting */
   }
}

/*
 * Check if the device is blocked or not
 */
int device_is_unmounted(DEVICE *dev)
{
   int stat;
   P(dev->mutex);
   stat = (dev->dev_blocked == BST_UNMOUNTED) ||
	  (dev->dev_blocked == BST_UNMOUNTED_WAITING_FOR_SYSOP);
   V(dev->mutex);
   return stat;
}

void _unlock_device(char *file, int line, DEVICE *dev) 
{
   Dmsg2(100, "unlock from %s:%d\n", file, line);
   V(dev->mutex);
}

/* 
 * Block all other threads from using the device
 *  Device must already be locked.  After this call,
 *  the device is blocked to any thread calling lock_device(),
 *  but the device is not locked (i.e. no P on device).  Also,
 *  the current thread can do slip through the lock_device()
 *  calls without blocking.
 */
void _block_device(char *file, int line, DEVICE *dev, int state)
{
   Dmsg3(100, "block set %d from %s:%d\n", state, file, line);
   ASSERT(dev->dev_blocked == BST_NOT_BLOCKED);
   dev->dev_blocked = state;	      /* make other threads wait */
   dev->no_wait_id = pthread_self();  /* allow us to continue */
}



/*
 * Unblock the device, and wake up anyone who went to sleep.
 */
void _unblock_device(char *file, int line, DEVICE *dev)
{
   Dmsg3(100, "unblock %d from %s:%d\n", dev->dev_blocked, file, line);
   ASSERT(dev->dev_blocked);
   dev->dev_blocked = BST_NOT_BLOCKED;
   dev->no_wait_id = 0;
   if (dev->num_waiting > 0) {
      pthread_cond_broadcast(&dev->wait); /* wake them up */
   }
}

/*
 * Enter with device locked and blocked
 * Exit with device unlocked and blocked by us.
 */
void _steal_device_lock(char *file, int line, DEVICE *dev, bsteal_lock_t *hold, int state)
{
   Dmsg4(100, "steal lock. old=%d new=%d from %s:%d\n", dev->dev_blocked, state,
      file, line);
   hold->dev_blocked = dev->dev_blocked;
   hold->dev_prev_blocked = dev->dev_prev_blocked;
   hold->no_wait_id = dev->no_wait_id;
   dev->dev_blocked = state;
   dev->no_wait_id = pthread_self();
   V(dev->mutex);
}

/*
 * Enter with device blocked by us but not locked
 * Exit with device locked, and blocked by previous owner 
 */
void _give_back_device_lock(char *file, int line, DEVICE *dev, bsteal_lock_t *hold)	      
{
   Dmsg4(100, "return lock. old=%d new=%d from %s:%d\n", 
      dev->dev_blocked, hold->dev_blocked, file, line);
   P(dev->mutex);
   dev->dev_blocked = hold->dev_blocked;
   dev->dev_prev_blocked = hold->dev_prev_blocked;
   dev->no_wait_id = hold->no_wait_id;
}
