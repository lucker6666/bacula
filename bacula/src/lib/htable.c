/*
 *  Bacula hash table routines
 *
 *  htable is a hash table of items (pointers). This code is
 *    adapted and enhanced from code I wrote in 1982 for a
 *    relocatable linker.  At that time, the hash table size
 *    was fixed and a primary number, which essentially provides
 *    the randomness. In this program, the hash table can grow when
 *    it gets too full, so the table size here is a binary number. The
 *    hashing is provided using an idea from Tcl where the initial
 *    hash code is then "randomized" using a simple calculation from
 *    a random number generator that multiplies by a big number
 *    (I multiply by a prime number, while Tcl did not)
 *    then shifts the results down and does the binary division
 *    by masking.  Increasing the size of the hash table is simple.
 *    Just create a new larger table, walk the old table and
 *    re-hash insert each entry into the new table.
 *
 *		  
 *   Kern Sibbald, July MMIII
 *
 *   Version $Id$
 *
 */
/*
   Copyright (C) 2003-2004 Kern Sibbald and John Walker

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

#include "htable.h"

/* ===================================================================
 *    htable
 */

/*
 * Create hash of key, stored in hash then
 *  create and return the pseudo random bucket index
 */
void htable::hash_index(char *key)
{
   hash = 0;
   for (char *p=key; *p; p++) {
      hash += (hash << 3) + (uint32_t)*p;
   }
   /* Multiply by large prime number, take top bits, mask for remainder */
   index = ((hash * 1103515249) >> rshift) & mask;
   Dmsg2(100, "Leave hash_index hash=0x%x index=%d\n", hash, index);
   return;
}

htable::htable(void *item, void *link, int tsize)
{
   init(item, link, tsize);
}

void htable::init(void *item, void *link, int tsize)
{
   int pwr;
   tsize >>= 2;
   for (pwr=0; tsize; pwr++) {
      tsize >>= 1;
   }
   loffset = (char *)link - (char *)item;
   mask = ~((~0)<<pwr); 	      /* 3 bits => table size = 8 */
   rshift = 30 - pwr;		      /* start using bits 28, 29, 30 */
   num_items = 0;		      /* number of entries in table */
   buckets = 1<<pwr;		      /* hash table size -- power of two */
   max_items = buckets * 4;	      /* allow average 4 entries per chain */
   table = (hlink **)malloc(buckets * sizeof(hlink *));
   memset(table, 0, buckets * sizeof(hlink *));
   walkptr = NULL;
   walk_index = 0;
}

void * htable::operator new(size_t)
{
   return malloc(sizeof(htable));
}

void htable::operator delete(void *tbl) 
{
   ((htable *)tbl)->destroy();
   free(tbl);
}

uint32_t htable::size()
{
   return num_items;
}

void htable::stats() 
{
   int count[10];
   int max = 0;
   int i, j;
   hlink *p;
   printf("\n\nNumItems=%d\nBuckets=%d\n", num_items, buckets);
   for (i=0; i<10; i++) {
      count[i] = 0;
   }
   for (i=0; i<(int)buckets; i++) {
      p = table[i];
      j = 0;	 
      while (p) {
	 p = (hlink *)(p->next);
	 j++;
      }
      if (j > max) {
	 max = j;
      }
      if (j < 10) {
	 count[j]++;
      }
   }
   for (i=0; i<10; i++) {
      printf("%2d: %d\n",i, count[i]);
   }
   printf("max = %d\n", max);
}

void htable::grow_table()
{
   Dmsg1(100, "Grow called old size = %d\n", buckets);
   /* Setup a bigger table */
   htable *big = (htable *)malloc(sizeof(htable));
   big->loffset = loffset;
   big->mask = mask<<1 | 1;
   big->rshift = rshift - 1;
   big->num_items = 0;
   big->buckets = buckets * 2;
   big->max_items = big->buckets * 4;
   big->table = (hlink **)malloc(big->buckets * sizeof(hlink *));
   memset(big->table, 0, big->buckets * sizeof(hlink *));
   big->walkptr = NULL;
   big->walk_index = 0;
   /* Insert all the items in the new hash table */
   Dmsg1(100, "Before copy num_items=%d\n", num_items);
   /* 
    * We walk through the old smaller tree getting items,
    * but since we are overwriting the colision links, we must
    * explicitly save the item->next pointer and walk each
    * colision chain ourselves.  We do use next() for getting
    * to the next bucket.
    */
   for (void *item=first(); item; ) {
      void *ni = ((hlink *)((char *)item+loffset))->next;  /* save link overwritten by insert */
      Dmsg1(100, "Grow insert: %s\n", ((hlink *)((char *)item+loffset))->key);
      big->insert(((hlink *)((char *)item+loffset))->key, item);
      if (ni) {
	 item = (void *)((char *)ni-loffset);
      } else {
	 walkptr = NULL;       
	 item = next();
      }
   }
   Dmsg1(100, "After copy new num_items=%d\n", big->num_items);
   if (num_items != big->num_items) {
      Dmsg0(000, "****** Big problems num_items mismatch ******\n");
   }
   free(table);
   memcpy(this, big, sizeof(htable));  /* move everything across */
   free(big);
   Dmsg0(100, "Exit grow.\n");
}

bool htable::insert(char *key, void *item)
{
   hlink *hp;
   if (lookup(key)) {
      return false;		      /* already exists */
   }
   sm_check(__FILE__, __LINE__, false);
   ASSERT(index < buckets);
   Dmsg2(100, "Insert: hash=0x%x index=%d\n", (unsigned)hash, index);
   hp = (hlink *)(((char *)item)+loffset);
   Dmsg4(100, "Insert hp=0x%x index=%d item=0x%x offset=%u\n", (unsigned)hp,
      index, (unsigned)item, loffset);
   hp->next = table[index];
   hp->hash = hash;
   hp->key = key;
   table[index] = hp;
   Dmsg3(100, "Insert hp->next=0x%x hp->hash=0x%x hp->key=%s\n",
      (unsigned)hp->next, hp->hash, hp->key);

   if (++num_items >= max_items) {
      Dmsg2(100, "num_items=%d max_items=%d\n", num_items, max_items);
      grow_table();
   }
   sm_check(__FILE__, __LINE__, false);
   Dmsg3(100, "Leave insert index=%d num_items=%d key=%s\n", index, num_items, key);
   return true;
}

void *htable::lookup(char *key)
{
   hash_index(key);
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next) {
//    Dmsg2(100, "hp=0x%x key=%s\n", (long)hp, hp->key);
      if (hash == hp->hash && strcmp(key, hp->key) == 0) {
         Dmsg1(100, "lookup return %x\n", ((char *)hp)-loffset);
	 return ((char *)hp)-loffset;
      }
   }
   return NULL;
}

void *htable::next()
{
   Dmsg1(100, "Enter next: walkptr=0x%x\n", (unsigned)walkptr);
   if (walkptr) {
      walkptr = (hlink *)(walkptr->next);
   }
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];
      if (walkptr) {
         Dmsg3(100, "new walkptr=0x%x next=0x%x inx=%d\n", (unsigned)walkptr,
	    (unsigned)(walkptr->next), walk_index-1);
      }
   }
   if (walkptr) {
      Dmsg2(100, "next: rtn 0x%x walk_index=%d\n", 
	 (unsigned)(((char *)walkptr)-loffset), walk_index);
      return ((char *)walkptr)-loffset;
   } 
   Dmsg0(100, "next: return NULL\n");
   return NULL;
}

void *htable::first()
{
   Dmsg0(100, "Enter first\n");
   walkptr = table[0];		      /* get first bucket */
   walk_index = 1;		      /* Point to next index */
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];  /* go to next bucket */
      if (walkptr) {
         Dmsg3(100, "first new walkptr=0x%x next=0x%x inx=%d\n", (unsigned)walkptr,
	    (unsigned)(walkptr->next), walk_index-1);
      }
   }
   if (walkptr) {
      Dmsg1(100, "Leave first walkptr=0x%x\n", (unsigned)walkptr);
      return ((char *)walkptr)-loffset;
   } 
   Dmsg0(100, "Leave first walkptr=NULL\n");
   return NULL;
}

/* Destroy the table and its contents */
void htable::destroy()
{
   void *ni;
   void *li = first();
   do {
      ni = next();
      free(li);
      li = ni;
   } while (ni);

   free(table);
   table = NULL;
   Dmsg0(100, "Done destroy.\n");
}



#ifdef TEST_PROGRAM

struct MYJCR {
   char *key;
   hlink link;
};

#define NITEMS 10000

int main()
{
   char mkey[30];
   htable *jcrtbl;
   MYJCR *save_jcr = NULL, *item;
   MYJCR *jcr = NULL;
   int count = 0;
    
   jcrtbl = (htable *)malloc(sizeof(htable));
   jcrtbl->init(jcr, &jcr->link, NITEMS);
    
   Dmsg1(000, "Inserting %d items\n", NITEMS);
   for (int i=0; i<NITEMS; i++) {
      sprintf(mkey, "This is htable item %d", i);
      jcr = (MYJCR *)malloc(sizeof(MYJCR));
      Dmsg2(100, "link=0x%x jcr=0x%x\n", (unsigned)&jcr->link, (unsigned)jcr);
      jcr->key = bstrdup(mkey);

      jcrtbl->insert(jcr->key, jcr);
      if (i == 10) {
	 save_jcr = jcr;
      }
   }
   if (!(item = (MYJCR *)jcrtbl->lookup(save_jcr->key))) {
      printf("Bad news: %s not found.\n", save_jcr->key);
   } else {
      printf("Item 10's key is: %s\n", item->key);
   }

   jcrtbl->stats();
   printf("Walk the hash table:\n");
   foreach_htable (jcr, jcrtbl) {
//    printf("htable item = %s\n", jcr->key);
      free(jcr->key);
      count++;
   }
   printf("Got %d items -- %s\n", count, count==NITEMS?"OK":"***ERROR***");
   printf("Calling destroy\n");
   jcrtbl->destroy();

   free(jcrtbl);
   printf("Freed jcrtbl\n");

   sm_dump(false);

}
#endif
