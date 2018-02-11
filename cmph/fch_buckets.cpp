#include "stdafx.h"

#include "vqueue.h"
#include "fch_buckets.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct fch_bucket_entry_t
{
   char* value;
   DWORD length;
};

struct fch_bucket_t
{
   fch_bucket_entry_t*  entries;
   DWORD                capacity, size;
};

static void fch_bucket_new(fch_bucket_t* bucket)
{
   ASSERT(bucket);

   bucket->size = 0;
   bucket->entries = NULL;
   bucket->capacity = 0;
}

static void fch_bucket_destroy(fch_bucket_t* bucket)
{
   DWORD i;

   ASSERT(bucket);

   for (i = 0; i < bucket->size; i++)
   {
      free((bucket->entries + i)->value);
   }
   free(bucket->entries);
}

static void fch_bucket_reserve(fch_bucket_t* bucket,DWORD size)
{
   ASSERT(bucket);

   if (bucket->capacity < size)
   {
      DWORD new_capacity   = bucket->capacity + 1;

      TRACE("Increasing current capacity %u to %u\n",bucket->capacity,size);

      while (new_capacity < size)
      {
         new_capacity *= 2;
      }

      bucket->entries = (fch_bucket_entry_t *) realloc(bucket->entries,sizeof(fch_bucket_entry_t) * new_capacity);

      ASSERT(bucket->entries);

      bucket->capacity = new_capacity;
      TRACE("Increased\n");
   }
}

static void fch_bucket_insert(fch_bucket_t* bucket,char* val,DWORD val_length)
{
   ASSERT(bucket);

   fch_bucket_reserve(bucket,bucket->size + 1);

   (bucket->entries + bucket->size)->value = val;
   (bucket->entries + bucket->size)->length = val_length;

   ++(bucket->size);
}

static BYTE fch_bucket_is_empty(fch_bucket_t* bucket)
{
   ASSERT(bucket);

   return (BYTE) (bucket->size == 0);
}

static DWORD fch_bucket_size(fch_bucket_t* bucket)
{
   ASSERT(bucket);
   return bucket->size;
}

static char* fch_bucket_get_key(fch_bucket_t* bucket,DWORD index_key)
{
   ASSERT(bucket); 
   ASSERT(index_key < bucket->size);
   return (bucket->entries + index_key)->value;
}

static DWORD fch_bucket_get_length(fch_bucket_t* bucket,DWORD index_key)
{
   ASSERT(bucket); 
   ASSERT(index_key < bucket->size);
   return (bucket->entries + index_key)->length;
}

static void fch_bucket_print(fch_bucket_t* bucket,DWORD index)
{
   DWORD i;

   ASSERT(bucket);

   fprintf(stderr,"Printing bucket %u ...\n",index);

   for (i = 0; i < bucket->size; i++)
   {
      fprintf(stderr,"  key: %s\n",(bucket->entries + i)->value);
   }
}

struct __fch_buckets_t
{
   fch_bucket_t*  values;
   DWORD          nbuckets, max_size;
};

fch_buckets_t* fch_buckets_new(DWORD nbuckets)
{
   DWORD          i;

   fch_buckets_t* buckets  = (fch_buckets_t*) malloc(sizeof(fch_buckets_t));

   if (!buckets)
   {
      return NULL;
   }

   buckets->values = (fch_bucket_t *) calloc((size_t) nbuckets,sizeof(fch_bucket_t));

   for (i = 0; i < nbuckets; i++)
   {
      fch_bucket_new(buckets->values + i);
   }

   ASSERT(buckets->values);

   buckets->nbuckets = nbuckets;
   buckets->max_size = 0;

   return buckets;
}

BYTE fch_buckets_is_empty(fch_buckets_t* buckets,DWORD index)
{
   ASSERT(index < buckets->nbuckets);
   return fch_bucket_is_empty(buckets->values + index);
}

void fch_buckets_insert(fch_buckets_t* buckets,DWORD index,char* key,DWORD length)
{
   ASSERT(index < buckets->nbuckets);

   fch_bucket_insert(buckets->values + index,key,length);

   if (fch_bucket_size(buckets->values + index) > buckets->max_size)
   {
      buckets->max_size = fch_bucket_size(buckets->values + index);
   }
}

DWORD fch_buckets_get_size(fch_buckets_t* buckets,DWORD index)
{
   ASSERT(index < buckets->nbuckets);
   return fch_bucket_size(buckets->values + index);
}

char* fch_buckets_get_key(fch_buckets_t* buckets,DWORD index,DWORD index_key)
{
   ASSERT(index < buckets->nbuckets);
   return fch_bucket_get_key(buckets->values + index,index_key);
}

DWORD fch_buckets_get_keylength(fch_buckets_t* buckets,DWORD index,DWORD index_key)
{
   ASSERT(index < buckets->nbuckets);
   return fch_bucket_get_length(buckets->values + index,index_key);
}

DWORD fch_buckets_get_max_size(fch_buckets_t* buckets)
{
   return buckets->max_size;
}

DWORD fch_buckets_get_nbuckets(fch_buckets_t* buckets)
{
   return buckets->nbuckets;
}

DWORD* fch_buckets_get_indexes_sorted_by_size(fch_buckets_t* buckets)
{
   int      i              = 0;
   DWORD    sum = 0, value;
   DWORD*   nbuckets_size  = (DWORD*) calloc((size_t) buckets->max_size + 1,sizeof(DWORD));
   DWORD*   sorted_indexes = (DWORD*) calloc((size_t) buckets->nbuckets,sizeof(DWORD));

   // collect how many buckets for each size.
   for (i = 0; i < (int) buckets->nbuckets; i++)
   {
      nbuckets_size[fch_bucket_size(buckets->values + i)] ++;
   }

   // calculating offset considering a decreasing order of buckets size.
   value = nbuckets_size[buckets->max_size];

   nbuckets_size[buckets->max_size] = sum;

   for (i = (int) buckets->max_size - 1; i >= 0; i--)
   {
      sum += value;
      value = nbuckets_size[i];
      nbuckets_size[i] = sum;
   }

   for (i = 0; i < (int) buckets->nbuckets; i++)
   {
      sorted_indexes[nbuckets_size[fch_bucket_size(buckets->values + i)]] = (DWORD) i;
      nbuckets_size[fch_bucket_size(buckets->values + i)] ++;
   }

   free(nbuckets_size);

   return sorted_indexes;
}

void fch_buckets_print(fch_buckets_t* buckets)
{
   DWORD i;

   for (i = 0; i < buckets->nbuckets; i++)
   {
      fch_bucket_print(buckets->values + i,i);
   }
}

void fch_buckets_destroy(fch_buckets_t* buckets)
{
   DWORD i;

   for (i = 0; i < buckets->nbuckets; i++)
   {
      fch_bucket_destroy(buckets->values + i);
   }

   free(buckets->values);
   free(buckets);
}
