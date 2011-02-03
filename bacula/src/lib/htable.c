/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2003-2011 Free Software Foundation Europe e.V.

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
 *  Bacula hash table routines
 *
 *  htable is a hash table of items (pointers). This code is
 *    adapted and enhanced from code I wrote in 1982 for a
 *    relocatable linker.  At that time, the hash table size
 *    was fixed and a primary number, which essentially provides
 *    the randomness. In this program, the hash table can grow when
 *    it gets too full, so the table size here is a binary number. The
 *    hashing is provided using an idea from Tcl where the initial
 *    hash code is "randomized" using a simple calculation from
 *    a random number generator that multiplies by a big number
 *    (I multiply by a prime number, while Tcl did not)
 *    then shifts the result down and does the binary division
 *    by masking.  Increasing the size of the hash table is simple.
 *    Just create a new larger table, walk the old table and
 *    re-hash insert each entry into the new table.
 *
 *
 *   Kern Sibbald, July MMIII
 *
 */

#include "bacula.h"

#define PAGE_SIZE 4096
#define MAX_PAGES 2400
#define MAX_BUF_SIZE (MAX_PAGES * PAGE_SIZE)  /* approx 10MB */

static const int dbglvl = 500;

/* ===================================================================
 *    htable
 */

/*
 * This subroutine gets a big buffer.
 */
void htable::malloc_big_buf(int size)
{
   struct h_mem *hmem;

   hmem = (struct h_mem *)malloc(size);
   total_size += size;
   blocks++;
   hmem->next = mem_block;
   mem_block = hmem;
   hmem->mem = mem_block->first;
   hmem->rem = (char *)hmem + size - hmem->mem;
   Dmsg3(100, "malloc buf=%p size=%d rem=%d\n", hmem, size, hmem->rem);
}

/* This routine frees the whole tree */
void htable::hash_big_free()
{
   struct h_mem *hmem, *rel;

   for (hmem=mem_block; hmem; ) {
      rel = hmem;
      hmem = hmem->next;
      Dmsg1(100, "free malloc buf=%p\n", rel);
      free(rel);
   }
}

/*
 * Normal hash malloc routine that gets a 
 *  "small" buffer from the big buffer
 */
char *htable::hash_malloc(int size)
{
   char *buf;
   int asize = BALIGN(size);

   if (mem_block->rem < asize) {
      int mb_size;
      if (total_size >= (MAX_BUF_SIZE / 2)) {
         mb_size = MAX_BUF_SIZE;
      } else {
         mb_size = MAX_BUF_SIZE / 2;
      }
      malloc_big_buf(mb_size);
   }
   mem_block->rem -= asize;
   buf = mem_block->mem;
   mem_block->mem += asize;
   return buf;
}

/*
 * Create hash of key, stored in hash then
 *  create and return the pseudo random bucket index
 */
void htable::hash_index(char *key)
{
   hash = 0;
   for (char *p=key; *p; p++) {
      hash +=  ((hash << 5) | (hash >> (sizeof(hash)*8-5))) + (uint32_t)*p;
   }
   /* Multiply by large prime number, take top bits, mask for remainder */
   index = ((hash * 1103515249) >> rshift) & mask;
   Dmsg2(dbglvl, "Leave hash_index hash=0x%llx index=%d\n", hash, index);
}

void htable::hash_index(uint32_t key)
{
   hash = key;
   /* Multiply by large prime number, take top bits, mask for remainder */
   index = ((hash * 1103515249) >> rshift) & mask;
   Dmsg2(dbglvl, "Leave hash_index hash=0x%llx index=%d\n", hash, index);
}

void htable::hash_index(uint64_t key)
{
   hash = key;
   /* Multiply by large prime number, take top bits, mask for remainder */
   index = ((hash * 1103515249) >> rshift) & mask;
   Dmsg2(dbglvl, "Leave hash_index hash=0x%llx index=%d\n", hash, index);
}

/*
 * tsize is the estimated number of entries in the hash table
 */
htable::htable(void *item, void *link, int tsize)
{
   init(item, link, tsize);
}

void htable::init(void *item, void *link, int tsize)
{
   int pwr;

   memset(this, 0, sizeof(htable));
   if (tsize < 31) {
      tsize = 31;
   }
   tsize >>= 2;
   for (pwr=0; tsize; pwr++) {
      tsize >>= 1;
   }
   loffset = (char *)link - (char *)item;
   mask = ~((~0)<<pwr);               /* 3 bits => table size = 8 */
   rshift = 30 - pwr;                 /* start using bits 28, 29, 30 */
   buckets = 1<<pwr;                  /* hash table size -- power of two */
   max_items = buckets * 4;           /* allow average 4 entries per chain */
   table = (hlink **)malloc(buckets * sizeof(hlink *));
   memset(table, 0, buckets * sizeof(hlink *));
   malloc_big_buf(MAX_BUF_SIZE);   /* ***FIXME*** need variable or some estimate */
}

uint32_t htable::size()
{
   return num_items;
}

/*
 * Take each hash link and walk down the chain of items
 *  that hash there counting them (i.e. the hits), 
 *  then report that number.
 * Obiously, the more hits in a chain, the more time
 *  it takes to reference them. Empty chains are not so
 *  hot either -- as it means unused or wasted space.
 */
#define MAX_COUNT 20
void htable::stats()
{
   int hits[MAX_COUNT];
   int max = 0;
   int i, j;
   hlink *p;
   printf("\n\nNumItems=%d\nTotal buckets=%d\n", num_items, buckets);
   printf("Hits/bucket: buckets\n");
   for (i=0; i < MAX_COUNT; i++) {
      hits[i] = 0;
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
      if (j < MAX_COUNT) {
         hits[j]++;
      }
   }
   for (i=0; i < MAX_COUNT; i++) {
      printf("%2d:           %d\n",i, hits[i]);
   }
   printf("buckets=%d num_items=%d max_items=%d\n", buckets, num_items, max_items);
   printf("max hits in a bucket = %d\n", max);
   printf("total bytes malloced = %lld\n", (long long int)total_size);
   printf("total blocks malloced = %d\n", blocks);
}

void htable::grow_table()
{
   htable *big;
   hlink *cur;
   void *ni;

   Dmsg1(100, "Grow called old size = %d\n", buckets);
   /* Setup a bigger table */
   big = (htable *)malloc(sizeof(htable));
   memcpy(big, this, sizeof(htable));  /* start with original class data */
   big->loffset = loffset;
   big->mask = mask<<1 | 1;
   big->rshift = rshift - 1;
   big->num_items = 0;
   big->buckets = buckets * 2;
   big->max_items = big->buckets * 4;
   /* Create a bigger hash table */
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
      cur = (hlink *)((char *)item+loffset);
      ni = cur->next; /* save link overwritten by insert */
      switch (cur->key_type) {
      case KEY_TYPE_CHAR:
         Dmsg1(100, "Grow insert: %s\n", cur->key.char_key);
         big->insert(cur->key.char_key, item);
         break;
      case KEY_TYPE_UINT32:
         Dmsg1(100, "Grow insert: %ld\n", cur->key.uint32_key);
         big->insert(cur->key.uint32_key, item);
         break;
      case KEY_TYPE_UINT64:
         Dmsg1(100, "Grow insert: %ld\n", cur->key.uint64_key);
         big->insert(cur->key.uint64_key, item);
         break;
      }
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
      return false;                   /* already exists */
   }
   ASSERT(index < buckets);
   Dmsg2(dbglvl, "Insert: hash=%p index=%d\n", hash, index);
   hp = (hlink *)(((char *)item)+loffset);
   Dmsg4(dbglvl, "Insert hp=%p index=%d item=%p offset=%u\n", hp,
      index, item, loffset);
   hp->next = table[index];
   hp->hash = hash;
   hp->key_type = KEY_TYPE_CHAR;
   hp->key.char_key = key;
   table[index] = hp;
   Dmsg3(dbglvl, "Insert hp->next=%p hp->hash=0x%llx hp->key=%s\n",
      hp->next, hp->hash, hp->key.char_key);

   if (++num_items >= max_items) {
      Dmsg2(dbglvl, "num_items=%d max_items=%d\n", num_items, max_items);
      grow_table();
   }
   Dmsg3(dbglvl, "Leave insert index=%d num_items=%d key=%s\n", index, num_items, key);
   return true;
}

bool htable::insert(uint32_t key, void *item)
{
   hlink *hp;

   if (lookup(key)) {
      return false;                   /* already exists */
   }
   ASSERT(index < buckets);
   Dmsg2(dbglvl, "Insert: hash=%p index=%d\n", hash, index);
   hp = (hlink *)(((char *)item)+loffset);
   Dmsg4(dbglvl, "Insert hp=%p index=%d item=%p offset=%u\n", hp,
      index, item, loffset);
   hp->next = table[index];
   hp->hash = hash;
   hp->key_type = KEY_TYPE_UINT32;
   hp->key.uint32_key = key;
   table[index] = hp;
   Dmsg3(dbglvl, "Insert hp->next=%p hp->hash=0x%llx hp->key=%d\n",
      hp->next, hp->hash, hp->key.uint32_key);

   if (++num_items >= max_items) {
      Dmsg2(dbglvl, "num_items=%d max_items=%d\n", num_items, max_items);
      grow_table();
   }
   Dmsg3(dbglvl, "Leave insert index=%d num_items=%d key=%d\n", index, num_items, key);
   return true;
}

bool htable::insert(uint64_t key, void *item)
{
   hlink *hp;

   if (lookup(key)) {
      return false;                   /* already exists */
   }
   ASSERT(index < buckets);
   Dmsg2(dbglvl, "Insert: hash=%p index=%d\n", hash, index);
   hp = (hlink *)(((char *)item)+loffset);
   Dmsg4(dbglvl, "Insert hp=%p index=%d item=%p offset=%u\n", hp,
      index, item, loffset);
   hp->next = table[index];
   hp->hash = hash;
   hp->key_type = KEY_TYPE_UINT64;
   hp->key.uint64_key = key;
   table[index] = hp;
   Dmsg3(dbglvl, "Insert hp->next=%p hp->hash=0x%llx hp->key=%ld\n",
      hp->next, hp->hash, hp->key.uint64_key);

   if (++num_items >= max_items) {
      Dmsg2(dbglvl, "num_items=%d max_items=%d\n", num_items, max_items);
      grow_table();
   }
   Dmsg3(dbglvl, "Leave insert index=%d num_items=%d key=%lld\n", index, num_items, key);
   return true;
}

void *htable::lookup(char *key)
{
   hash_index(key);
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next) {
      ASSERT(hp->key_type == KEY_TYPE_CHAR);
//    Dmsg2(100, "hp=%p key=%s\n", hp, hp->key.char_key);
      if (hash == hp->hash && strcmp(key, hp->key.char_key) == 0) {
         Dmsg1(dbglvl, "lookup return %p\n", ((char *)hp)-loffset);
         return ((char *)hp)-loffset;
      }
   }
   return NULL;
}

void *htable::lookup(uint32_t key)
{
   hash_index(key);
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next) {
      ASSERT(hp->key_type == KEY_TYPE_UINT32);
      if (hash == hp->hash && key == hp->key.uint32_key) {
         Dmsg1(dbglvl, "lookup return %p\n", ((char *)hp)-loffset);
         return ((char *)hp)-loffset;
      }
   }
   return NULL;
}

void *htable::lookup(uint64_t key)
{
   hash_index(key);
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next) {
      ASSERT(hp->key_type == KEY_TYPE_UINT64);
      if (hash == hp->hash && key == hp->key.uint64_key) {
         Dmsg1(dbglvl, "lookup return %p\n", ((char *)hp)-loffset);
         return ((char *)hp)-loffset;
      }
   }
   return NULL;
}

void *htable::next()
{
   Dmsg1(dbglvl, "Enter next: walkptr=%p\n", walkptr);
   if (walkptr) {
      walkptr = (hlink *)(walkptr->next);
   }
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];
      if (walkptr) {
         Dmsg3(dbglvl, "new walkptr=%p next=%p inx=%d\n", walkptr,
            walkptr->next, walk_index-1);
      }
   }
   if (walkptr) {
      Dmsg2(dbglvl, "next: rtn %p walk_index=%d\n",
         ((char *)walkptr)-loffset, walk_index);
      return ((char *)walkptr)-loffset;
   }
   Dmsg0(dbglvl, "next: return NULL\n");
   return NULL;
}

void *htable::first()
{
   Dmsg0(dbglvl, "Enter first\n");
   walkptr = table[0];                /* get first bucket */
   walk_index = 1;                    /* Point to next index */
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];  /* go to next bucket */
      if (walkptr) {
         Dmsg3(dbglvl, "first new walkptr=%p next=%p inx=%d\n", walkptr,
            walkptr->next, walk_index-1);
      }
   }
   if (walkptr) {
      Dmsg1(dbglvl, "Leave first walkptr=%p\n", walkptr);
      return ((char *)walkptr)-loffset;
   }
   Dmsg0(dbglvl, "Leave first walkptr=NULL\n");
   return NULL;
}

/* Destroy the table and its contents */
void htable::destroy()
{
   hash_big_free();

   free(table);
   table = NULL;
   garbage_collect_memory();
   Dmsg0(100, "Done destroy.\n");
}



#ifdef TEST_PROGRAM

struct MYJCR {
#ifndef TEST_NON_CHAR
   char *key;
#else
   uint32_t key;
#endif
   hlink link;
};

#define NITEMS 5000000

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
#ifndef TEST_NON_CHAR
      int len;
      len = sprintf(mkey, "This is htable item %d", i) + 1;

      jcr = (MYJCR *)jcrtbl->hash_malloc(sizeof(MYJCR));
      jcr->key = (char *)jcrtbl->hash_malloc(len);
      memcpy(jcr->key, mkey, len);
#else
      jcr = (MYJCR *)jcrtbl->hash_malloc(sizeof(MYJCR));
      jcr->key = i;
#endif
      Dmsg2(100, "link=%p jcr=%p\n", jcr->link, jcr);

      jcrtbl->insert(jcr->key, jcr);
      if (i == 10) {
         save_jcr = jcr;
      }
   }
   if (!(item = (MYJCR *)jcrtbl->lookup(save_jcr->key))) {
      printf("Bad news: %s not found.\n", save_jcr->key);
   } else {
#ifndef TEST_NON_CHAR
      printf("Item 10's key is: %s\n", item->key);
#else
      printf("Item 10's key is: %ld\n", item->key);
#endif
   }

   jcrtbl->stats();
   printf("Walk the hash table:\n");
   foreach_htable (jcr, jcrtbl) {
#ifndef TEST_NON_CHAR
//    printf("htable item = %s\n", jcr->key);
#ifndef BIG_MALLOC
      free(jcr->key);
#endif
#endif
      count++;
   }
   printf("Got %d items -- %s\n", count, count==NITEMS?"OK":"***ERROR***");
   printf("Calling destroy\n");
   jcrtbl->destroy();

   free(jcrtbl);
   printf("Freed jcrtbl\n");

   sm_dump(false);   /* unit test */

}
#endif
