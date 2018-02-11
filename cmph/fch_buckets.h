#ifndef __CMPH_FCH_BUCKETS_H__
#define __CMPH_FCH_BUCKETS_H__

typedef struct __fch_buckets_t   fch_buckets_t;

fch_buckets_t* fch_buckets_new(DWORD nbuckets);

BYTE           fch_buckets_is_empty(fch_buckets_t* buckets,DWORD index);

void           fch_buckets_insert(fch_buckets_t* buckets,DWORD index,char* key,DWORD length);

DWORD          fch_buckets_get_size(fch_buckets_t* buckets,DWORD index);

char*          fch_buckets_get_key(fch_buckets_t* buckets,DWORD index,DWORD index_key);

DWORD          fch_buckets_get_keylength(fch_buckets_t* buckets,DWORD index,DWORD index_key);

// returns the size of biggest bucket.
DWORD          fch_buckets_get_max_size(fch_buckets_t* buckets);

// returns the number of buckets.
DWORD          fch_buckets_get_nbuckets(fch_buckets_t* buckets);

DWORD*         fch_buckets_get_indexes_sorted_by_size(fch_buckets_t* buckets);

void           fch_buckets_print(fch_buckets_t* buckets);

void           fch_buckets_destroy(fch_buckets_t* buckets);
#endif
