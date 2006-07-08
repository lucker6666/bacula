/*
 *   Version $Id$
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */


/*  General purpose queue  */

struct b_queue {
        struct b_queue *qnext,     /* Next item in queue */
                     *qprev;       /* Previous item in queue */
};

typedef struct b_queue BQUEUE;

/*  Queue functions  */

void    qinsert(BQUEUE *qhead, BQUEUE *object);
BQUEUE *qnext(BQUEUE *qhead, BQUEUE *qitem);
BQUEUE *qdchain(BQUEUE *qitem);
BQUEUE *qremove(BQUEUE *qhead);
