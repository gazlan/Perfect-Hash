#include "stdafx.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<limits.h>

#include "cmph_types.h"
#include "cmph.h"
#include "select.h"
#include "compressed_seq.h"
#include "compressed_rank.h"
#include "jenkins_hash.h"
#include "hash_state.h"
#include "chd_ph.h"
#include "hash.h"
#include "chd_structs_ph.h"
#include "cmph_structs.h"
#include"miller_rabin.h"
#include"bitbool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// NO_ELEMENT is equivalent to null pointer
#ifndef NO_ELEMENT
#define NO_ELEMENT UINT_MAX
#endif

// struct used to represent items at mapping, ordering and searching phases
struct _chd_ph_item_t
{
   DWORD                         f;
   DWORD                         h;
};

typedef struct _chd_ph_item_t chd_ph_item_t;

// struct to represent the items at mapping phase only.
struct _chd_ph_map_item_t
{
   DWORD                               f;
   DWORD                               h;
   DWORD                               bucket_num;
};

typedef struct _chd_ph_map_item_t   chd_ph_map_item_t;

// struct to represent a bucket
struct _chd_ph_bucket_t
{
   DWORD items_list; // offset
   union
   {
      DWORD                            size;
      DWORD                            bucket_id;
   };
};

typedef struct _chd_ph_bucket_t  chd_ph_bucket_t;

struct _chd_ph_sorted_list_t
{
   DWORD                                  buckets_list;
   DWORD                                  size;
};

typedef struct _chd_ph_sorted_list_t   chd_ph_sorted_list_t;

static inline chd_ph_bucket_t*         chd_ph_bucket_new(DWORD nbuckets);
static inline void                     chd_ph_bucket_clean(chd_ph_bucket_t* buckets,DWORD nbuckets);
static inline void                     chd_ph_bucket_destroy(chd_ph_bucket_t* buckets);

chd_ph_bucket_t* chd_ph_bucket_new(DWORD nbuckets)
{
   chd_ph_bucket_t*  buckets  = (chd_ph_bucket_t*) calloc(nbuckets,sizeof(chd_ph_bucket_t));
   return buckets;
}

void chd_ph_bucket_clean(chd_ph_bucket_t* buckets,DWORD nbuckets)
{
   DWORD i  = 0;

   ASSERT(buckets);

   for (i = 0; i < nbuckets; i++)
   {
      buckets[i].size = 0;
   }
}

static BYTE chd_ph_bucket_insert(chd_ph_bucket_t* buckets,chd_ph_map_item_t* map_items,chd_ph_item_t* items,DWORD nbuckets,DWORD item_idx)
{
   DWORD             i              = 0;
   chd_ph_item_t*    tmp_item;
   chd_ph_map_item_t*tmp_map_item   = map_items + item_idx;
   chd_ph_bucket_t*  bucket         = buckets + tmp_map_item->bucket_num;

   tmp_item = items + bucket->items_list;

   for (i = 0; i < bucket->size; i++)
   {
      if (tmp_item->f == tmp_map_item->f && tmp_item->h == tmp_map_item->h)
      {
         TRACE("Item not added\n");
         return 0;
      }

      tmp_item++;
   }

   tmp_item->f = tmp_map_item->f;
   tmp_item->h = tmp_map_item->h;

   bucket->size++;

   return 1;
}

void chd_ph_bucket_destroy(chd_ph_bucket_t* buckets)
{
   free(buckets);
}

static inline BYTE            chd_ph_mapping(cmph_config_t* mph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD* max_bucket_size);

static chd_ph_sorted_list_t*  chd_ph_ordering(chd_ph_bucket_t** _buckets,chd_ph_item_t** items,DWORD nbuckets,DWORD nitems,DWORD max_bucket_size);

static BYTE                   chd_ph_searching(chd_ph_config_data_t* chd_ph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD max_bucket_size,chd_ph_sorted_list_t* sorted_lists,DWORD max_probes,DWORD* disp_table);

static inline double chd_ph_space_lower_bound(DWORD _n,DWORD _r)
{
   double r =  _r, n = _n;
   return (1 + (r / n - 1.0 + 1.0 / (2.0 * n)) * log(1 - n / r)) / log(2);
};

/* computes the entropy of non empty buckets.*/
static inline double chd_ph_get_entropy(DWORD* disp_table,DWORD n,DWORD max_probes)
{
   DWORD*   probe_counts   = (DWORD*) calloc(max_probes,sizeof(DWORD));
   DWORD    i;
   double   entropy        = 0;

   for (i = 0; i < n; i++)
   {
      probe_counts[disp_table[i]]++;
   };

   for (i = 0; i < max_probes; i++)
   {
      if (probe_counts[i] > 0)
      {
         entropy -= probe_counts[i] * log((double) probe_counts[i] / (double) n) / log(2);
      }
   };
   free(probe_counts);
   return entropy;
};

chd_ph_config_data_t* chd_ph_config_new(void)
{
   chd_ph_config_data_t*   chd_ph;
   chd_ph = (chd_ph_config_data_t *) malloc(sizeof(chd_ph_config_data_t));
   if (!chd_ph)
   {
      return NULL;
   }
   memset(chd_ph,0,sizeof(chd_ph_config_data_t));

   chd_ph->hashfunc = CMPH_HASH_JENKINS;
   chd_ph->cs = NULL;
   chd_ph->nbuckets = 0;
   chd_ph->n = 0;
   chd_ph->hl = NULL;

   chd_ph->m = 0;
   chd_ph->use_h = 1;
   chd_ph->keys_per_bin = 1;
   chd_ph->keys_per_bucket = 4;
   chd_ph->occup_table = 0;

   return chd_ph;
}

void chd_ph_config_destroy(cmph_config_t* mph)
{
   chd_ph_config_data_t*   data  = (chd_ph_config_data_t*) mph->data;

   TRACE("Destroying algorithm dependent data\n");

   if (data->occup_table)
   {
      free(data->occup_table);
      data->occup_table = NULL;
   }

   free(data);
}

void chd_ph_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs)
{
   chd_ph_config_data_t*   chd_ph   = (chd_ph_config_data_t*) mph->data;
   CMPH_HASH*              hashptr  = hashfuncs;
   DWORD                   i        = 0;

   while (*hashptr != CMPH_HASH_COUNT)
   {
      if (i >= 1)
      {
         break;
      } //chd_ph only uses one linear hash function
      chd_ph->hashfunc = *hashptr;
      ++i, ++hashptr;
   }
}

void chd_ph_config_set_b(cmph_config_t* mph,DWORD keys_per_bucket)
{
   ASSERT(mph);

   chd_ph_config_data_t*   chd_ph   = (chd_ph_config_data_t*) mph->data;

   if (keys_per_bucket < 1 || keys_per_bucket >= 15)
   {
      keys_per_bucket = 4;
   }
   chd_ph->keys_per_bucket = keys_per_bucket;
}

void chd_ph_config_set_keys_per_bin(cmph_config_t* mph,DWORD keys_per_bin)
{
   ASSERT(mph);

   chd_ph_config_data_t*   chd_ph   = (chd_ph_config_data_t*) mph->data;

   if (keys_per_bin <= 1 || keys_per_bin >= 128)
   {
      keys_per_bin = 1;
   }

   chd_ph->keys_per_bin = keys_per_bin;
}

BYTE chd_ph_mapping(cmph_config_t* mph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD* max_bucket_size)
{
   DWORD                   i = 0, g = 0;
   DWORD                   hl[3];
   chd_ph_config_data_t*   chd_ph               = (chd_ph_config_data_t*) mph->data;
   char*                   key                  = NULL;
   DWORD                   keylen               = 0;
   chd_ph_map_item_t*      map_item;
   chd_ph_map_item_t*      map_items            = (chd_ph_map_item_t*) malloc(chd_ph->m*sizeof(chd_ph_map_item_t));
   DWORD                   mapping_iterations   = 1000;
   *max_bucket_size = 0;

   while (true)
   {
      mapping_iterations--;

      if (chd_ph->hl)
      {
         hash_state_destroy(chd_ph->hl);
      }
      chd_ph->hl = hash_state_new(chd_ph->hashfunc,chd_ph->m);

      chd_ph_bucket_clean(buckets,chd_ph->nbuckets);

      mph->key_source->rewind(mph->key_source->data);

      for (i = 0; i < chd_ph->m; i++)
      {
         mph->key_source->read(mph->key_source->data,&key,&keylen);
         hash_vector(chd_ph->hl,key,keylen,hl);

         map_item = (map_items + i);

         g = hl[0] % chd_ph->nbuckets;
         map_item->f = hl[1] % chd_ph->n;
         map_item->h = hl[2] % (chd_ph->n - 1) + 1;
         map_item->bucket_num = g;
         mph->key_source->dispose(mph->key_source->data,key,keylen);

         buckets[g].size++;

         if (buckets[g].size > *max_bucket_size)
         {
            *max_bucket_size = buckets[g].size;
         }
      }

      buckets[0].items_list = 0;

      for (i = 1; i < chd_ph->nbuckets; i++)
      {
         buckets[i].items_list = buckets[i - 1].items_list + buckets[i - 1].size;
         buckets[i - 1].size = 0;
      }

      buckets[i - 1].size = 0;

      for (i = 0; i < chd_ph->m; i++)
      {
         map_item = (map_items + i);
         if (!chd_ph_bucket_insert(buckets,map_items,items,chd_ph->nbuckets,i))
         {
            break;
         }
      }

      if (i == chd_ph->m)
      {
         free(map_items);
         return 1; // SUCCESS
      }

      if (mapping_iterations == 0)
      {
         goto error;
      }
   }

   error:

   free(map_items);

   hash_state_destroy(chd_ph->hl);

   chd_ph->hl = NULL;

   return 0; // FAILURE
}

chd_ph_sorted_list_t* chd_ph_ordering(chd_ph_bucket_t** _buckets,chd_ph_item_t** _items,DWORD nbuckets,DWORD nitems,DWORD max_bucket_size)
{
   chd_ph_sorted_list_t*   sorted_lists   = (chd_ph_sorted_list_t*) calloc(max_bucket_size + 1,sizeof(chd_ph_sorted_list_t));

   chd_ph_bucket_t*        input_buckets  = (*_buckets);
   chd_ph_bucket_t*        output_buckets;
   chd_ph_item_t*          input_items    = (*_items);
   chd_ph_item_t*          output_items;
   DWORD                   i, j, bucket_size, position, position2;
   //    DWORD non_empty_buckets;

   TRACE("MAX BUCKET SIZE = %u\n",max_bucket_size);

   // Determine size of each list of buckets
   for (i = 0; i < nbuckets; i++)
   {
      bucket_size = input_buckets[i].size;
      if (bucket_size == 0)
      {
         continue;
      }
      sorted_lists[bucket_size].size++;
   }

   sorted_lists[1].buckets_list = 0;

   // Determine final position of list of buckets into the contiguous array that will store all the buckets
   for (i = 2; i <= max_bucket_size; i++)
   {
      sorted_lists[i].buckets_list = sorted_lists[i - 1].buckets_list + sorted_lists[i - 1].size;
      sorted_lists[i - 1].size = 0;
   }

   sorted_lists[i - 1].size = 0;

   // Store the buckets in a new array which is sorted by bucket sizes
   output_buckets = (chd_ph_bucket_t *) calloc(nbuckets,sizeof(chd_ph_bucket_t)); // everything is initialized with zero
   //    non_empty_buckets = nbuckets;

   for (i = 0; i < nbuckets; i++)
   {
      bucket_size = input_buckets[i].size;
      if (bucket_size == 0)
      {
         //          non_empty_buckets--;
         continue;
      }

      position = sorted_lists[bucket_size].buckets_list + sorted_lists[bucket_size].size;
      output_buckets[position].bucket_id = i;
      output_buckets[position].items_list = input_buckets[i].items_list;
      sorted_lists[bucket_size].size++;
   }

   /* for(i = non_empty_buckets; i < nbuckets; i++)
         output_buckets[i].size=0;*/
   // Return the buckets sorted in new order and free the old buckets sorted in old order
   free(input_buckets);

   (*_buckets) = output_buckets;

   // Store the items according to the new order of buckets.
   output_items = (chd_ph_item_t *) calloc(nitems,sizeof(chd_ph_item_t));
   position = 0;
   i = 0;

   for (bucket_size = 1; bucket_size <= max_bucket_size; bucket_size++)
   {
      for (i = sorted_lists[bucket_size].buckets_list; i < sorted_lists[bucket_size].size + sorted_lists[bucket_size].buckets_list; i++)
      {
         position2 = output_buckets[i].items_list;
         output_buckets[i].items_list = position;

         for (j = 0; j < bucket_size; j++)
         {
            output_items[position].f = input_items[position2].f;
            output_items[position].h = input_items[position2].h;
            position++;
            position2++;
         }
      }
   }

   //Return the items sorted in new order and free the old items sorted in old order
   free(input_items);

   (*_items) = output_items;

   return sorted_lists;
}

static inline BYTE place_bucket_probe(chd_ph_config_data_t* chd_ph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD probe0_num,DWORD probe1_num,DWORD bucket_num,DWORD size)
{
   DWORD          i;
   chd_ph_item_t* item;
   DWORD          position;

   item = items + buckets[bucket_num].items_list;

   // try place bucket with probe_num
   if (chd_ph->keys_per_bin > 1)
   {
      for (i = 0; i < size; i++) // placement
      {
         position = (DWORD) ((item->f + ((unsigned __int64) item->h) * probe0_num + probe1_num) % chd_ph->n);

         if (chd_ph->occup_table[position] >= chd_ph->keys_per_bin)
         {
            break;
         }

         (chd_ph->occup_table[position])++;

         item++;
      }
   }
   else
   {
      for (i = 0; i < size; i++) // placement
      {
         position = (DWORD) ((item->f + ((unsigned __int64) item->h) * probe0_num + probe1_num) % chd_ph->n);

         if (GETBIT32(((DWORD *) chd_ph->occup_table),position))
         {
            break;
         }

         SETBIT32(((DWORD *) chd_ph->occup_table),position);
         item++;
      }
   }

   if (i != size) // Undo the placement
   {
      item = items + buckets[bucket_num].items_list;

      if (chd_ph->keys_per_bin > 1)
      {
         while (true)
         {
            if (i == 0)
            {
               break;
            }
            position = (DWORD) ((item->f + ((unsigned __int64) item->h) * probe0_num + probe1_num) % chd_ph->n);
            (chd_ph->occup_table[position])--;
            item++;
            i--;
         }
      }
      else
      {
         while (true)
         {
            if (i == 0)
            {
               break;
            }

            position = (DWORD) ((item->f + ((unsigned __int64) item->h) * probe0_num + probe1_num) % chd_ph->n);

            UNSETBIT32(((DWORD *) chd_ph->occup_table),position);

            //             ([position/32]^=(1<<(position%32));
            item++;
            i--;
         }
      }

      return 0;
   } 

   return 1;
}

static inline BYTE place_bucket(chd_ph_config_data_t* chd_ph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD max_probes,DWORD* disp_table,DWORD bucket_num,DWORD size)
{
   DWORD probe0_num, probe1_num, probe_num;
   probe0_num = 0;
   probe1_num = 0;
   probe_num = 0;

   while (true)
   {
      if (place_bucket_probe(chd_ph,buckets,items,probe0_num,probe1_num,bucket_num,size))
      {
         disp_table[buckets[bucket_num].bucket_id] = probe0_num + probe1_num * chd_ph->n;
         return 1;
      }

      probe0_num++;

      if (probe0_num >= chd_ph->n)
      {
         probe0_num -= chd_ph->n;
         probe1_num++;
      }

      probe_num++;

      if (probe_num >= max_probes || probe1_num >= chd_ph->n)
      {
         return 0;
      }
   }

   return 0;
}

static inline BYTE place_buckets1(chd_ph_config_data_t* chd_ph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD max_bucket_size,chd_ph_sorted_list_t* sorted_lists,DWORD max_probes,DWORD* disp_table)
{
   DWORD i           = 0;
   DWORD curr_bucket = 0;

   for (i = max_bucket_size; i > 0; i--)
   {
      curr_bucket = sorted_lists[i].buckets_list;

      while (curr_bucket < sorted_lists[i].size + sorted_lists[i].buckets_list)
      {
         if (!place_bucket(chd_ph,buckets,items,max_probes,disp_table,curr_bucket,i))
         {
            return 0;
         }
         curr_bucket++;
      }
   }

   return 1;
}

static inline BYTE place_buckets2(chd_ph_config_data_t* chd_ph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD max_bucket_size,chd_ph_sorted_list_t* sorted_lists,DWORD max_probes,DWORD* disp_table)
{
   DWORD i, j, non_placed_bucket;
   DWORD curr_bucket;
   DWORD probe_num, probe0_num, probe1_num;
   DWORD sorted_list_size;

   #ifdef DEBUG
   DWORD items_list;
   DWORD bucket_id;
   #endif

   TRACE("USING HEURISTIC TO PLACE BUCKETS\n");

   for (i = max_bucket_size; i > 0; i--)
   {
      probe_num = 0;
      probe0_num = 0;
      probe1_num = 0;
      sorted_list_size = sorted_lists[i].size;

      while (sorted_lists[i].size != 0)
      {
         curr_bucket = sorted_lists[i].buckets_list;

         for (j = 0, non_placed_bucket = 0; j < sorted_lists[i].size; j++)
         {
            // if bucket is successfully placed remove it from list
            if (place_bucket_probe(chd_ph,buckets,items,probe0_num,probe1_num,curr_bucket,i))
            {
               disp_table[buckets[curr_bucket].bucket_id] = probe0_num + probe1_num * chd_ph->n;
               //                TRACE("BUCKET %u PLACED --- DISPLACEMENT = %u\n", curr_bucket, disp_table[curr_bucket]);
            }
            else
            {
               //                TRACE("BUCKET %u NOT PLACED\n", curr_bucket);
               #ifdef DEBUG
               items_list = buckets[non_placed_bucket + sorted_lists[i].buckets_list].items_list;
               bucket_id = buckets[non_placed_bucket + sorted_lists[i].buckets_list].bucket_id;
               #endif
               buckets[non_placed_bucket + sorted_lists[i].buckets_list].items_list = buckets[curr_bucket].items_list;
               buckets[non_placed_bucket + sorted_lists[i].buckets_list].bucket_id = buckets[curr_bucket].bucket_id;
               #ifdef DEBUG
               buckets[curr_bucket].items_list = items_list;
               buckets[curr_bucket].bucket_id = bucket_id;
               #endif
               non_placed_bucket++;
            }

            curr_bucket++;
         }

         sorted_lists[i].size = non_placed_bucket;
         probe0_num++;

         if (probe0_num >= chd_ph->n)
         {
            probe0_num -= chd_ph->n;
            probe1_num++;
         }

         probe_num++;
         if (probe_num >= max_probes || probe1_num >= chd_ph->n)
         {
            sorted_lists[i].size = sorted_list_size;
            return 0;
         }
      }

      sorted_lists[i].size = sorted_list_size;
   }

   return 1;
}

BYTE chd_ph_searching(chd_ph_config_data_t* chd_ph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD max_bucket_size,chd_ph_sorted_list_t* sorted_lists,DWORD max_probes,DWORD* disp_table)
{
   if (chd_ph->use_h)
   {
      return place_buckets2(chd_ph,buckets,items,max_bucket_size,sorted_lists,max_probes,disp_table);
   }
   else
   {
      return place_buckets1(chd_ph,buckets,items,max_bucket_size,sorted_lists,max_probes,disp_table);
   }
}

static inline BYTE chd_ph_check_bin_hashing(chd_ph_config_data_t* chd_ph,chd_ph_bucket_t* buckets,chd_ph_item_t* items,DWORD* disp_table,chd_ph_sorted_list_t* sorted_lists,DWORD max_bucket_size)
{
   DWORD          bucket_size, i, j;
   DWORD          position, probe0_num, probe1_num;
   DWORD          m  = 0;
   chd_ph_item_t* item;

   if (chd_ph->keys_per_bin > 1)
   {
      memset(chd_ph->occup_table,0,chd_ph->n);
   }
   else
   {
      memset(chd_ph->occup_table,0,((chd_ph->n + 31) / 32) * sizeof(DWORD));
   }

   for (bucket_size = 1; bucket_size <= max_bucket_size; bucket_size++)
   {
      for (i = sorted_lists[bucket_size].buckets_list; i < sorted_lists[bucket_size].size + sorted_lists[bucket_size].buckets_list; i++)
      {
         j = bucket_size;
         item = items + buckets[i].items_list;
         probe0_num = disp_table[buckets[i].bucket_id] % chd_ph->n;
         probe1_num = disp_table[buckets[i].bucket_id] / chd_ph->n;

         for (; j > 0; j--)
         {
            m++;
            position = (DWORD) ((item->f + ((unsigned __int64) item->h) * probe0_num + probe1_num) % chd_ph->n);

            if (chd_ph->keys_per_bin > 1)
            {
               if (chd_ph->occup_table[position] >= chd_ph->keys_per_bin)
               {
                  return 0;
               }

               (chd_ph->occup_table[position])++;
            }
            else
            {
               if (GETBIT32(((DWORD *) chd_ph->occup_table),position))
               {
                  return 0;
               }

               SETBIT32(((DWORD *) chd_ph->occup_table),position);
            }
            item++;
         }
      }
   }

   TRACE("We were able to place m = %u keys\n",m);
   return 1;
}

cmph_t* chd_ph_new(cmph_config_t* mph,double c)
{
   cmph_t*                 mphf                    = NULL;
   chd_ph_data_t*          chd_phf                 = NULL;
   chd_ph_config_data_t*   chd_ph                  = (chd_ph_config_data_t*) mph->data;

   double                  load_factor             = c;
   BYTE                    searching_success       = 0;
   DWORD                   max_probes              = 1 << 20; // default value for max_probes
   DWORD                   iterations              = 100;
   chd_ph_bucket_t*        buckets                 = NULL;
   chd_ph_item_t*          items                   = NULL;
   BYTE                    failure                 = 0;
   DWORD                   max_bucket_size         = 0;
   chd_ph_sorted_list_t*   sorted_lists            = NULL;
   DWORD*                  disp_table              = NULL;
   double                  space_lower_bound       = 0;
   #ifdef CMPH_TIMING
   double                  construction_time_begin = 0.0;
   double                  construction_time       = 0.0;
   ELAPSED_TIME_IN_SECONDS(&construction_time_begin);
   #endif

   chd_ph->m = mph->key_source->nkeys;

   TRACE("m = %u\n",chd_ph->m);

   chd_ph->nbuckets = (DWORD) (chd_ph->m / chd_ph->keys_per_bucket) + 1;

   TRACE("nbuckets = %u\n",chd_ph->nbuckets);

   if (load_factor < 0.5)
   {
      load_factor = 0.5;
   }

   if (load_factor >= 0.99)
   {
      load_factor = 0.99;
   }

   TRACE("load_factor = %.3f\n",load_factor);

   chd_ph->n = (DWORD) (chd_ph->m / (chd_ph->keys_per_bin * load_factor)) + 1;

   //Round the number of bins to the prime immediately above
   if (chd_ph->n % 2 == 0)
   {
      chd_ph->n++;
   }

   for (; ;)
   {
      if (check_primality(chd_ph->n) == 1)
      {
         break;
      }
      chd_ph->n += 2; // just odd numbers can be primes for n > 2
   }

   TRACE("n = %u \n",chd_ph->n);

   if (chd_ph->keys_per_bin == 1)
   {
      space_lower_bound = chd_ph_space_lower_bound(chd_ph->m,chd_ph->n);
   }

   if (mph->verbosity)
   {
      fprintf(stderr,"space lower bound is %.3f bits per key\n",space_lower_bound);
   }

   // We allocate the working tables
   buckets = chd_ph_bucket_new(chd_ph->nbuckets);
   items = (chd_ph_item_t *) calloc(chd_ph->m,sizeof(chd_ph_item_t));

   max_probes = (DWORD) (((log(chd_ph->m) / log(2)) / 20) * max_probes);

   if (chd_ph->keys_per_bin == 1)
   {
      chd_ph->occup_table = (BYTE *) calloc(((chd_ph->n + 31) / 32),sizeof(DWORD));
   }
   else
   {
      chd_ph->occup_table = (BYTE *) calloc(chd_ph->n,sizeof(BYTE));
   }

   disp_table = (DWORD *) calloc(chd_ph->nbuckets,sizeof(DWORD));

   while (true)
   {
      iterations --;

      if (mph->verbosity)
      {
         fprintf(stderr,"Starting mapping step for mph creation of %u keys with %u bins\n",chd_ph->m,chd_ph->n);
      }

      if (!chd_ph_mapping(mph,buckets,items,&max_bucket_size))
      {
         if (mph->verbosity)
         {
            fprintf(stderr,"Failure in mapping step\n");
         }

         failure = 1;
         goto cleanup;
      }

      if (mph->verbosity)
      {
         fprintf(stderr,"Starting ordering step\n");
      }

      if (sorted_lists)
      {
         free(sorted_lists);
      }

      sorted_lists = chd_ph_ordering(&buckets,&items,chd_ph->nbuckets,chd_ph->m,max_bucket_size);

      if (mph->verbosity)
      {
         fprintf(stderr,"Starting searching step\n");
      }

      searching_success = chd_ph_searching(chd_ph,buckets,items,max_bucket_size,sorted_lists,max_probes,disp_table);

      if (searching_success)
      {
         break;
      }

      // reset occup_table
      if (chd_ph->keys_per_bin > 1)
      {
         memset(chd_ph->occup_table,0,chd_ph->n);
      }
      else
      {
         memset(chd_ph->occup_table,0,((chd_ph->n + 31) / 32) * sizeof(DWORD));
      }

      if (iterations == 0)
      {
         // Cleanup memory
         if (mph->verbosity)
         {
            fprintf(stderr,"Failure because the max trials was exceeded\n");
         }

         failure = 1;
         goto cleanup;
      }
   }

   #ifdef DEBUG
   {
      if (!chd_ph_check_bin_hashing(chd_ph,buckets,items,disp_table,sorted_lists,max_bucket_size))
      {
         TRACE("Error for bin packing generation");
         failure = 1;
         goto cleanup;
      }
   }
   #endif

   if (mph->verbosity)
   {
      fprintf(stderr,"Starting compressing step\n");
   }

   if (chd_ph->cs)
   {
      free(chd_ph->cs);
   }

   chd_ph->cs = (compressed_seq_t *) calloc(1,sizeof(compressed_seq_t));

   compressed_seq_init(chd_ph->cs);
   compressed_seq_generate(chd_ph->cs,disp_table,chd_ph->nbuckets);

   #ifdef CMPH_TIMING
   ELAPSED_TIME_IN_SECONDS(&construction_time);
   double   entropy  = chd_ph_get_entropy(disp_table,chd_ph->nbuckets,max_probes);
   DEBUGP("Entropy = %.4f\n",entropy / chd_ph->m);
   #endif

   cleanup:

   chd_ph_bucket_destroy(buckets);

   free(items);
   free(sorted_lists);
   free(disp_table);

   if (failure)
   {
      if (chd_ph->hl)
      {
         hash_state_destroy(chd_ph->hl);
      }

      chd_ph->hl = NULL;
      return NULL;
   }

   mphf = (cmph_t *) malloc(sizeof(cmph_t));
   mphf->algo = mph->algo;
   chd_phf = (chd_ph_data_t *) malloc(sizeof(chd_ph_data_t));

   chd_phf->cs = chd_ph->cs;
   chd_ph->cs = NULL; //transfer memory ownership
   chd_phf->hl = chd_ph->hl;
   chd_ph->hl = NULL; //transfer memory ownership
   chd_phf->n = chd_ph->n;
   chd_phf->nbuckets = chd_ph->nbuckets;

   mphf->data = chd_phf;
   mphf->size = chd_ph->n;

   TRACE("Successfully generated minimal perfect hash\n");

   if (mph->verbosity)
   {
      fprintf(stderr,"Successfully generated minimal perfect hash function\n");
   }

   #ifdef CMPH_TIMING
   DWORD space_usage = chd_ph_packed_size(mphf) * 8;
   construction_time = construction_time - construction_time_begin;
   fprintf(stdout,"%u\t%.2f\t%u\t%.4f\t%.4f\t%.4f\t%.4f\n",chd_ph->m,load_factor,chd_ph->keys_per_bucket,construction_time,space_usage / (double) chd_ph->m,space_lower_bound,entropy / chd_ph->m);
   #endif

   return mphf;
}

void chd_ph_load(FILE* fd,cmph_t* mphf)
{
   char*          buf      = NULL;
   DWORD          buflen;
   size_t         nbytes;
   chd_ph_data_t* chd_ph   = (chd_ph_data_t*) malloc(sizeof(chd_ph_data_t));

   TRACE("Loading chd_ph mphf\n");
   mphf->data = chd_ph;

   nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,fd);

   TRACE("Hash state has %u bytes\n",buflen);

   buf = (char *) malloc((size_t) buflen);
   nbytes = fread(buf,(size_t) buflen,(size_t) 1,fd);
   chd_ph->hl = hash_state_load(buf,buflen);

   free(buf);

   nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,fd);

   TRACE("Compressed sequence structure has %u bytes\n",buflen);

   buf = (char *) malloc((size_t) buflen);
   nbytes = fread(buf,(size_t) buflen,(size_t) 1,fd);
   chd_ph->cs = (compressed_seq_t *) calloc(1,sizeof(compressed_seq_t));
   compressed_seq_load(chd_ph->cs,buf,buflen);

   free(buf);

   // loading n and nbuckets
   TRACE("Reading n and nbuckets\n");

   nbytes = fread(&(chd_ph->n),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fread(&(chd_ph->nbuckets),sizeof(DWORD),(size_t) 1,fd);
}

int chd_ph_dump(cmph_t* mphf,FILE* fd)
{
   char*          buf   = NULL;
   DWORD          buflen;
   size_t         nbytes;
   chd_ph_data_t* data  = (chd_ph_data_t*) mphf->data;

   __cmph_dump(mphf,fd);

   hash_state_dump(data->hl,&buf,&buflen);

   TRACE("Dumping hash state with %u bytes to disk\n",buflen);

   nbytes = fwrite(&buflen,sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(buf,(size_t) buflen,(size_t) 1,fd);

   free(buf);

   compressed_seq_dump(data->cs,&buf,&buflen);

   TRACE("Dumping compressed sequence structure with %u bytes to disk\n",buflen);

   nbytes = fwrite(&buflen,sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(buf,(size_t) buflen,(size_t) 1,fd);

   free(buf);

   // dumping n and nbuckets
   nbytes = fwrite(&(data->n),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(&(data->nbuckets),sizeof(DWORD),(size_t) 1,fd);

   return 1;
}

void chd_ph_destroy(cmph_t* mphf)
{
   chd_ph_data_t* data  = (chd_ph_data_t*) mphf->data;
   compressed_seq_destroy(data->cs);

   free(data->cs);

   hash_state_destroy(data->hl);

   free(data);
   free(mphf);
}

DWORD chd_ph_search(cmph_t* mphf,const char* key,DWORD keylen)
{
   chd_ph_data_t* chd_ph   = (chd_ph_data_t*) mphf->data;
   DWORD          hl[3];
   DWORD          disp, position;
   DWORD          probe0_num, probe1_num;
   DWORD          f, g, h;

   hash_vector(chd_ph->hl,key,keylen,hl);

   g = hl[0] % chd_ph->nbuckets;
   f = hl[1] % chd_ph->n;
   h = hl[2] % (chd_ph->n - 1) + 1;

   disp = compressed_seq_query(chd_ph->cs,g);

   probe0_num = disp % chd_ph->n;
   probe1_num = disp / chd_ph->n;

   position = (DWORD) ((f + ((unsigned __int64) h) * probe0_num + probe1_num) % chd_ph->n);

   return position;
}

void chd_ph_pack(cmph_t* mphf,void* packed_mphf)
{
   chd_ph_data_t* data     = (chd_ph_data_t*) mphf->data;
   BYTE*          ptr      = (BYTE*) packed_mphf;

   // packing hl type
   CMPH_HASH      hl_type  = hash_get_type(data->hl);
   *((DWORD *) ptr) = hl_type;
   ptr += sizeof(DWORD);

   // packing hl
   hash_state_pack(data->hl,ptr);
   ptr += hash_state_packed_size(hl_type);

   // packing n
   *((DWORD *) ptr) = data->n;
   ptr += sizeof(data->n);

   // packing nbuckets
   *((DWORD *) ptr) = data->nbuckets;
   ptr += sizeof(data->nbuckets);

   // packing cs
   compressed_seq_pack(data->cs,ptr);
   //ptr += compressed_seq_packed_size(data->cs);
}

DWORD chd_ph_packed_size(cmph_t* mphf)
{
   chd_ph_data_t* data                 = (chd_ph_data_t*) mphf->data;
   CMPH_HASH      hl_type              = hash_get_type(data->hl);
   DWORD          hash_state_pack_size = hash_state_packed_size(hl_type);
   DWORD          cs_pack_size         = compressed_seq_packed_size(data->cs);

   return (DWORD) (sizeof(CMPH_ALGO) + hash_state_pack_size + cs_pack_size + 3 * sizeof(DWORD));
}

DWORD chd_ph_search_packed(void* packed_mphf,const char* key,DWORD keylen)
{
   CMPH_HASH   hl_type  = (CMPH_HASH) * (DWORD*) packed_mphf;
   BYTE*       hl_ptr   = (BYTE*) (packed_mphf) + 4;

   DWORD*      ptr      = (DWORD*) (hl_ptr + hash_state_packed_size(hl_type));
   DWORD       n        = *ptr++;
   DWORD       nbuckets = *ptr++;
   DWORD       hl[3];

   DWORD       disp, position;
   DWORD       probe0_num, probe1_num;
   DWORD       f, g, h;

   hash_vector_packed(hl_ptr,hl_type,key,keylen,hl);

   g = hl[0] % nbuckets;
   f = hl[1] % n;
   h = hl[2] % (n - 1) + 1;

   disp = compressed_seq_query_packed(ptr,g);

   probe0_num = disp % n;
   probe1_num = disp / n;

   position = (DWORD) ((f + ((unsigned __int64) h) * probe0_num + probe1_num) % n);

   return position;
}
