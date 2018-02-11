#include "stdafx.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmph_types.h"
#include "cmph.h"
#include "graph.h"
#include "fch.h"
#include "jenkins_hash.h"
#include "hash_state.h"
#include "hash.h"
#include "fch_structs.h"
#include "bmz8.h"
#include "bmz8_structs.h"
#include "brz.h"
#include "cmph_structs.h"
#include "brz_structs.h"
#include "buffer_manager.h"
#include "cmph.h"
#include "hash.h"
#include "bitbool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_BUCKET_SIZE 255

static int     brz_gen_mphf(cmph_config_t* mph);
static DWORD   brz_min_index(DWORD* vector,DWORD n);
static void    brz_destroy_keys_vd(BYTE** keys_vd,DWORD nkeys);
static char*   brz_copy_partial_fch_mphf(brz_config_data_t* brz,fch_data_t* fchf,DWORD index,DWORD* buflen);
static char*   brz_copy_partial_bmz8_mphf(brz_config_data_t* brz,bmz8_data_t* bmzf,DWORD index,DWORD* buflen);
brz_config_data_t* brz_config_new(void)
{
   brz_config_data_t*   brz   = NULL;

   brz = (brz_config_data_t *) malloc(sizeof(brz_config_data_t));

   if (!brz)
   {
      return NULL;
   }

   brz->algo = CMPH_FCH;
   brz->b = 128;
   brz->hashfuncs[0] = CMPH_HASH_JENKINS;
   brz->hashfuncs[1] = CMPH_HASH_JENKINS;
   brz->hashfuncs[2] = CMPH_HASH_JENKINS;
   brz->size = NULL;
   brz->offset = NULL;
   brz->g = NULL;
   brz->h1 = NULL;
   brz->h2 = NULL;
   brz->h0 = NULL;
   brz->memory_availability = 1024 * 1024;
   brz->tmp_dir = (BYTE *) calloc((size_t) 10,sizeof(BYTE));
   brz->mphf_fd = NULL;

   strcpy((char *) (brz->tmp_dir),"/var/tmp/");

   ASSERT(brz);
   return brz;
}

void brz_config_destroy(cmph_config_t* mph)
{
   brz_config_data_t*   data  = (brz_config_data_t*) mph->data;
   free(data->tmp_dir);

   TRACE("Destroying algorithm dependent data\n");
   free(data);
}

void brz_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs)
{
   brz_config_data_t*   brz      = (brz_config_data_t*) mph->data;
   CMPH_HASH*           hashptr  = hashfuncs;
   DWORD                i        = 0;
   while (*hashptr != CMPH_HASH_COUNT)
   {
      if (i >= 3)
      {
         break;
      } //brz only uses three hash functions
      brz->hashfuncs[i] = *hashptr;
      ++i, ++hashptr;
   }
}

void brz_config_set_memory_availability(cmph_config_t* mph,DWORD memory_availability)
{
   brz_config_data_t*   brz   = (brz_config_data_t*) mph->data;
   if (memory_availability > 0)
   {
      brz->memory_availability = memory_availability * 1024 * 1024;
   }
}

void brz_config_set_tmp_dir(cmph_config_t* mph,BYTE* tmp_dir)
{
   brz_config_data_t*   brz   = (brz_config_data_t*) mph->data;
   if (tmp_dir)
   {
      size_t   len   = strlen((char*) tmp_dir);
      free(brz->tmp_dir);
      if (tmp_dir[len - 1] != '/')
      {
         brz->tmp_dir = (BYTE *) calloc((size_t) len + 2,sizeof(BYTE));
         sprintf((char *) (brz->tmp_dir),"%s/",(char *) tmp_dir);
      }
      else
      {
         brz->tmp_dir = (BYTE *) calloc((size_t) len + 1,sizeof(BYTE));
         sprintf((char *) (brz->tmp_dir),"%s",(char *) tmp_dir);
      }
   }
}

void brz_config_set_mphf_fd(cmph_config_t* mph,FILE* mphf_fd)
{
   brz_config_data_t*   brz   = (brz_config_data_t*) mph->data;
   brz->mphf_fd = mphf_fd;

   ASSERT(brz->mphf_fd);
}

void brz_config_set_b(cmph_config_t* mph,DWORD b)
{
   brz_config_data_t*   brz   = (brz_config_data_t*) mph->data;
   if (b <= 64 || b >= 175)
   {
      b = 128;
   }
   brz->b = (BYTE) b;
}

void brz_config_set_algo(cmph_config_t* mph,CMPH_ALGO algo)
{
   if (algo == CMPH_BMZ8 || algo == CMPH_FCH) // supported algorithms
   {
      brz_config_data_t*   brz   = (brz_config_data_t*) mph->data;
      brz->algo = algo;
   }
}

cmph_t* brz_new(cmph_config_t* mph,double c)
{
   cmph_t*     mphf        = NULL;
   brz_data_t* brzf        = NULL;
   DWORD       i;
   DWORD       iterations  = 20;

   TRACE("c: %f\n",c);

   brz_config_data_t*   brz   = (brz_config_data_t*) mph->data;

   switch (brz->algo) // validating restrictions over parameter c.
   {
      case CMPH_BMZ8:
         if (c == 0 || c >= 2.0)
         {
            c = 1;
         }

         break;
      case CMPH_FCH:
         if (c <= 2.0)
         {
            c = 2.6;
         }

         break;
      default:
         ASSERT(0);
   }

   brz->c = c;
   brz->m = mph->key_source->nkeys;

   TRACE("m: %u\n",brz->m);

   brz->k = (DWORD) ceil(brz->m / ((double) brz->b));

   TRACE("k: %u\n",brz->k);

   brz->size = (BYTE *) calloc((size_t) brz->k,sizeof(BYTE));

   // Clustering the keys by graph id.
   if (mph->verbosity)
   {
      fprintf(stderr,"Partioning the set of keys.\n");
   }

   while (true)
   {
      int   ok;

      TRACE("hash function 3\n");

      brz->h0 = hash_state_new(brz->hashfuncs[2],brz->k);

      TRACE("Generating graphs\n");

      ok = brz_gen_mphf(mph);

      if (!ok)
      {
         --iterations;
         hash_state_destroy(brz->h0);
         brz->h0 = NULL;

         TRACE("%u iterations remaining to create the graphs in a external file\n",iterations);

         if (mph->verbosity)
         {
            fprintf(stderr,"Failure: A graph with more than 255 keys was created - %u iterations remaining\n",iterations);
         }

         if (iterations == 0)
         {
            break;
         }
      }
      else
      {
         break;
      }
   }

   if (iterations == 0)
   {
      TRACE("Graphs with more than 255 keys were created in all 20 iterations\n");

      free(brz->size);

      return NULL;
   }

   TRACE("Graphs generated\n");

   brz->offset = (DWORD *) calloc((size_t) brz->k,sizeof(DWORD));

   for (i = 1; i < brz->k; ++i)
   {
      brz->offset[i] = brz->size[i - 1] + brz->offset[i - 1];
   }

   // Generating a mphf
   mphf = (cmph_t *) malloc(sizeof(cmph_t));
   mphf->algo = mph->algo;

   brzf = (brz_data_t *) malloc(sizeof(brz_data_t));
   brzf->g = brz->g;
   brz->g = NULL; //transfer memory ownership
   brzf->h1 = brz->h1;
   brz->h1 = NULL; //transfer memory ownership
   brzf->h2 = brz->h2;
   brz->h2 = NULL; //transfer memory ownership
   brzf->h0 = brz->h0;
   brz->h0 = NULL; //transfer memory ownership
   brzf->size = brz->size;
   brz->size = NULL; //transfer memory ownership
   brzf->offset = brz->offset;
   brz->offset = NULL; //transfer memory ownership
   brzf->k = brz->k;
   brzf->c = brz->c;
   brzf->m = brz->m;
   brzf->algo = brz->algo;

   mphf->data = brzf;
   mphf->size = brz->m;

   TRACE("Successfully generated minimal perfect hash\n");

   if (mph->verbosity)
   {
      fprintf(stderr,"Successfully generated minimal perfect hash function\n");
   }

   return mphf;
}

static int brz_gen_mphf(cmph_config_t* mph)
{
   DWORD                i, e, error;

   brz_config_data_t*   brz               = (brz_config_data_t*) mph->data;

   DWORD                memory_usage      = 0;
   DWORD                nkeys_in_buffer   = 0;

   BYTE*                buffer            = (BYTE*) malloc((size_t) brz->memory_availability);

   DWORD*               buckets_size      = (DWORD*) calloc((size_t) brz->k,sizeof(DWORD));
   DWORD*               keys_index        = NULL;

   BYTE**               buffer_merge      = NULL;
   DWORD*               buffer_h0         = NULL;
   DWORD                nflushes          = 0;
   DWORD                h0;

   size_t               nbytes;

   FILE*                tmp_fd            = NULL;

   buffer_manager_t*    buff_manager      = NULL;

   char*                filename          = NULL;
   char*                key               = NULL;

   DWORD                keylen;
   DWORD                cur_bucket        = 0;

   BYTE                 nkeys_vd          = 0;
   BYTE**               keys_vd           = NULL;

   mph->key_source->rewind(mph->key_source->data);

   TRACE("Generating graphs from %u keys\n",brz->m);

   // Partitioning
   for (e = 0; e < brz->m; ++e)
   {
      mph->key_source->read(mph->key_source->data,&key,&keylen);

      /* Buffers management */
      if (memory_usage + keylen + sizeof(keylen) > brz->memory_availability) // flush buffers
      {
         if (mph->verbosity)
         {
            fprintf(stderr,"Flushing  %u\n",nkeys_in_buffer);
         }

         DWORD value    = buckets_size[0];
         DWORD sum      = 0;
         DWORD keylen1  = 0;

         buckets_size[0] = 0;

         for (i = 1; i < brz->k; i++)
         {
            if (buckets_size[i] == 0)
            {
               continue;
            }

            sum += value;

            value = buckets_size[i];

            buckets_size[i] = sum;
         }

         memory_usage = 0;

         keys_index = (DWORD *) calloc((size_t) nkeys_in_buffer,sizeof(DWORD));

         for (i = 0; i < nkeys_in_buffer; i++)
         {
            memcpy(&keylen1,buffer + memory_usage,sizeof(keylen1));

            h0 = hash(brz->h0,(char *) (buffer + memory_usage + sizeof(keylen1)),keylen1) % brz->k;

            keys_index[buckets_size[h0]] = memory_usage;

            buckets_size[h0]++;

            memory_usage += keylen1 + (DWORD)sizeof(keylen1);
         }

         filename = (char *) calloc(strlen((char *) (brz->tmp_dir)) + 11,sizeof(char));

         sprintf(filename,"%s%u.cmph",brz->tmp_dir,nflushes);

         tmp_fd = fopen(filename,"wb");

         free(filename);

         filename = NULL;

         for (i = 0; i < nkeys_in_buffer; i++)
         {
            memcpy(&keylen1,buffer + keys_index[i],sizeof(keylen1));
            nbytes = fwrite(buffer + keys_index[i],(size_t) 1,keylen1 + sizeof(keylen1),tmp_fd);
         }

         nkeys_in_buffer = 0;

         memory_usage = 0;

         memset((void *) buckets_size,0,brz->k * sizeof(DWORD));

         nflushes++;

         free(keys_index);

         fclose(tmp_fd);
      }

      memcpy(buffer + memory_usage,&keylen,sizeof(keylen));
      memcpy(buffer + memory_usage + sizeof(keylen),key,(size_t) keylen);

      memory_usage += keylen + (DWORD)sizeof(keylen);

      h0 = hash(brz->h0,key,keylen) % brz->k;

      if ((brz->size[h0] == MAX_BUCKET_SIZE) || (brz->algo == CMPH_BMZ8 && ((brz->c >= 1.0) && (BYTE) (brz->c * brz->size[h0]) < brz->size[h0])))
      {
         free(buffer);
         free(buckets_size);
         return 0;
      }

      brz->size[h0] = (BYTE) (brz->size[h0] + 1U);

      buckets_size[h0] ++;

      nkeys_in_buffer++;

      mph->key_source->dispose(mph->key_source->data,key,keylen);
   }

   if (memory_usage != 0) // flush buffers
   {
      if (mph->verbosity)
      {
         fprintf(stderr,"Flushing  %u\n",nkeys_in_buffer);
      }

      DWORD value    = buckets_size[0];
      DWORD sum      = 0;
      DWORD keylen1  = 0;

      buckets_size[0] = 0;

      for (i = 1; i < brz->k; i++)
      {
         if (buckets_size[i] == 0)
         {
            continue;
         }
         sum += value;
         value = buckets_size[i];
         buckets_size[i] = sum;
      }

      memory_usage = 0;

      keys_index = (DWORD *) calloc((size_t) nkeys_in_buffer,sizeof(DWORD));

      for (i = 0; i < nkeys_in_buffer; i++)
      {
         memcpy(&keylen1,buffer + memory_usage,sizeof(keylen1));
         h0 = hash(brz->h0,(char *) (buffer + memory_usage + sizeof(keylen1)),keylen1) % brz->k;
         keys_index[buckets_size[h0]] = memory_usage;
         buckets_size[h0]++;
         memory_usage += keylen1 + (DWORD)sizeof(keylen1);
      }

      filename = (char *) calloc(strlen((char *) (brz->tmp_dir)) + 11,sizeof(char));

      sprintf(filename,"%s%u.cmph",brz->tmp_dir,nflushes);

      tmp_fd = fopen(filename,"wb");

      free(filename);

      filename = NULL;

      for (i = 0; i < nkeys_in_buffer; i++)
      {
         memcpy(&keylen1,buffer + keys_index[i],sizeof(keylen1));
         nbytes = fwrite(buffer + keys_index[i],(size_t) 1,keylen1 + sizeof(keylen1),tmp_fd);
      }

      nkeys_in_buffer = 0;

      memory_usage = 0;

      memset((void *) buckets_size,0,brz->k * sizeof(DWORD));

      nflushes++;

      free(keys_index);

      fclose(tmp_fd);
   }

   free(buffer);
   free(buckets_size);

   if (nflushes > 1024)
   {
      return 0;
   } // Too many files generated.

   // mphf generation

   if (mph->verbosity)
   {
      fprintf(stderr,"\nMPHF generation \n");
   }

   /* Starting to dump to disk the resultant MPHF: __cmph_dump function */
   nbytes = fwrite(cmph_names[CMPH_BRZ],(size_t) (strlen(cmph_names[CMPH_BRZ]) + 1),(size_t) 1,brz->mphf_fd);
   nbytes = fwrite(&(brz->m),sizeof(brz->m),(size_t) 1,brz->mphf_fd);
   nbytes = fwrite(&(brz->c),sizeof(double),(size_t) 1,brz->mphf_fd);
   nbytes = fwrite(&(brz->algo),sizeof(brz->algo),(size_t) 1,brz->mphf_fd);
   nbytes = fwrite(&(brz->k),sizeof(DWORD),(size_t) 1,brz->mphf_fd); // number of MPHFs
   nbytes = fwrite(brz->size,sizeof(BYTE) * (brz->k),(size_t) 1,brz->mphf_fd);

   //tmp_fds = (FILE **)calloc(nflushes, sizeof(FILE *));
   buff_manager = buffer_manager_new(brz->memory_availability,nflushes);
   buffer_merge = (BYTE * *) calloc((size_t) nflushes,sizeof(BYTE *));
   buffer_h0 = (DWORD *) calloc((size_t) nflushes,sizeof(DWORD));

   memory_usage = 0;

   for (i = 0; i < nflushes; i++)
   {
      filename = (char *) calloc(strlen((char *) (brz->tmp_dir)) + 11,sizeof(char));

      sprintf(filename,"%s%u.cmph",brz->tmp_dir,i);

      buffer_manager_open(buff_manager,i,filename);

      free(filename);

      filename = NULL;

      key = (char *) buffer_manager_read_key(buff_manager,i,&keylen);

      h0 = hash(brz->h0,key + sizeof(keylen),keylen) % brz->k;

      buffer_h0[i] = h0;
      buffer_merge[i] = (BYTE *) key;
      key = NULL; //transfer memory ownership
   }

   e = 0;

   keys_vd = (BYTE * *) calloc((size_t) MAX_BUCKET_SIZE,sizeof(BYTE *));

   nkeys_vd = 0;

   error = 0;

   while (e < brz->m)
   {
      i = brz_min_index(buffer_h0,nflushes);

      cur_bucket = buffer_h0[i];

      key = (char *) buffer_manager_read_key(buff_manager,i,&keylen);

      if (key)
      {
         while (key)
         {
            //keylen = strlen(key);
            h0 = hash(brz->h0,key + sizeof(keylen),keylen) % brz->k;

            if (h0 != buffer_h0[i])
            {
               break;
            }
            keys_vd[nkeys_vd++] = (BYTE *) key;
            key = NULL; //transfer memory ownership
            e++;

            key = (char *) buffer_manager_read_key(buff_manager,i,&keylen);
         }

         if (key)
         {
            ASSERT(nkeys_vd < brz->size[cur_bucket]);

            keys_vd[nkeys_vd++] = buffer_merge[i];
            buffer_merge[i] = NULL; //transfer memory ownership
            e++;
            buffer_h0[i] = h0;
            buffer_merge[i] = (BYTE *) key;
         }
      }

      if (!key)
      {
         ASSERT(nkeys_vd < brz->size[cur_bucket]);

         keys_vd[nkeys_vd++] = buffer_merge[i];
         buffer_merge[i] = NULL; //transfer memory ownership
         e++;
         buffer_h0[i] = UINT_MAX;
      }

      if (nkeys_vd == brz->size[cur_bucket]) // Generating mphf for each bucket.
      {
         cmph_io_adapter_t*   source      = NULL;
         cmph_config_t*       config      = NULL;
         cmph_t*              mphf_tmp    = NULL;
         char*                bufmphf     = NULL;
         DWORD                buflenmphf  = 0;
         // Source of keys
         source = cmph_io_byte_vector_adapter(keys_vd,(DWORD) nkeys_vd);
         config = cmph_config_new(source);
         cmph_config_set_algo(config,brz->algo);
         //cmph_config_set_algo(config, CMPH_BMZ8);
         cmph_config_set_graphsize(config,brz->c);
         mphf_tmp = cmph_new(config);

         if (mphf_tmp == NULL)
         {
            if (mph->verbosity)
            {
               fprintf(stderr,"ERROR: Can't generate MPHF for bucket %u out of %u\n",cur_bucket + 1,brz->k);
            }
            error = 1;
            cmph_config_destroy(config);
            brz_destroy_keys_vd(keys_vd,nkeys_vd);
            cmph_io_byte_vector_adapter_destroy(source);
            break;
         }

         if (mph->verbosity)
         {
            if (cur_bucket % 1000 == 0)
            {
               fprintf(stderr,"MPHF for bucket %u out of %u was generated.\n",cur_bucket + 1,brz->k);
            }
         }

         switch (brz->algo)
         {
            case CMPH_FCH:
               {
                  fch_data_t* fchf  = NULL;
                  fchf = (fch_data_t *) mphf_tmp->data;
                  bufmphf = brz_copy_partial_fch_mphf(brz,fchf,cur_bucket,&buflenmphf);
               }
               break;
            case CMPH_BMZ8:
               {
                  bmz8_data_t*   bmzf  = NULL;
                  bmzf = (bmz8_data_t *) mphf_tmp->data;
                  bufmphf = brz_copy_partial_bmz8_mphf(brz,bmzf,cur_bucket,&buflenmphf);
               }
               break;
            default:
               ASSERT(0);
         }

         nbytes = fwrite(bufmphf,(size_t) buflenmphf,(size_t) 1,brz->mphf_fd);

         free(bufmphf);
         bufmphf = NULL;

         cmph_config_destroy(config);
         brz_destroy_keys_vd(keys_vd,nkeys_vd);
         cmph_destroy(mphf_tmp);
         cmph_io_byte_vector_adapter_destroy(source);
         nkeys_vd = 0;
      }
   }

   buffer_manager_destroy(buff_manager);

   free(keys_vd);
   free(buffer_merge);
   free(buffer_h0);

   if (error)
   {
      return 0;
   }

   return 1;
}

static DWORD brz_min_index(DWORD* vector,DWORD n)
{
   DWORD i, min_index = 0;

   for (i = 1; i < n; i++)
   {
      if (vector[i] < vector[min_index])
      {
         min_index = i;
      }
   }

   return min_index;
}

static void brz_destroy_keys_vd(BYTE** keys_vd,DWORD nkeys)
{
   BYTE  i;

   for (i = 0; i < nkeys; i++)
   {
      free(keys_vd[i]); keys_vd[i] = NULL;
   }
}

static char* brz_copy_partial_fch_mphf(brz_config_data_t* brz,fch_data_t* fchf,DWORD index,DWORD* buflen)
{
   DWORD i        = 0;
   DWORD buflenh1 = 0;
   DWORD buflenh2 = 0;
   char* bufh1    = NULL;
   char* bufh2    = NULL;
   char* buf      = NULL;

   DWORD n        = fchf->b;//brz->size[index];

   hash_state_dump(fchf->h1,&bufh1,&buflenh1);
   hash_state_dump(fchf->h2,&bufh2,&buflenh2);

   *buflen = buflenh1 + buflenh2 + n + 2U * (DWORD)sizeof(DWORD);

   buf = (char *) malloc((size_t) (*buflen));

   memcpy(buf,&buflenh1,sizeof(DWORD));
   memcpy(buf + sizeof(DWORD),bufh1,(size_t) buflenh1);
   memcpy(buf + sizeof(DWORD) + buflenh1,&buflenh2,sizeof(DWORD));
   memcpy(buf + 2 * sizeof(DWORD) + buflenh1,bufh2,(size_t) buflenh2);

   for (i = 0; i < n; i++)
   {
      memcpy(buf + 2 * sizeof(DWORD) + buflenh1 + buflenh2 + i,(fchf->g + i),(size_t) 1);
   }

   free(bufh1);
   free(bufh2);

   return buf;
}
static char* brz_copy_partial_bmz8_mphf(brz_config_data_t* brz,bmz8_data_t* bmzf,DWORD index,DWORD* buflen)
{
   DWORD buflenh1 = 0;
   DWORD buflenh2 = 0;

   char* bufh1    = NULL;
   char* bufh2    = NULL;
   char* buf      = NULL;

   DWORD n        = (DWORD) ceil(brz->c* brz->size[index]);

   hash_state_dump(bmzf->hashes[0],&bufh1,&buflenh1);
   hash_state_dump(bmzf->hashes[1],&bufh2,&buflenh2);

   *buflen = buflenh1 + buflenh2 + n + 2U * (DWORD)sizeof(DWORD);

   buf = (char *) malloc((size_t) (*buflen));

   memcpy(buf,&buflenh1,sizeof(DWORD));
   memcpy(buf + sizeof(DWORD),bufh1,(size_t) buflenh1);
   memcpy(buf + sizeof(DWORD) + buflenh1,&buflenh2,sizeof(DWORD));
   memcpy(buf + 2 * sizeof(DWORD) + buflenh1,bufh2,(size_t) buflenh2);
   memcpy(buf + 2 * sizeof(DWORD) + buflenh1 + buflenh2,bmzf->g,(size_t) n);

   free(bufh1);
   free(bufh2);

   return buf;
}

int brz_dump(cmph_t* mphf,FILE* fd)
{
   brz_data_t* data  = (brz_data_t*) mphf->data;

   char*       buf   = NULL;
   DWORD       buflen;
   size_t      nbytes;

   TRACE("Dumping brzf\n");

   // The initial part of the MPHF have already been dumped to disk during construction
   // Dumping h0
   hash_state_dump(data->h0,&buf,&buflen);

   TRACE("Dumping hash state with %u bytes to disk\n",buflen);

   nbytes = fwrite(&buflen,sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(buf,(size_t) buflen,(size_t) 1,fd);

   free(buf);

   // Dumping m and the vector offset.
   nbytes = fwrite(&(data->m),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(data->offset,sizeof(DWORD) * (data->k),(size_t) 1,fd);

   return 1;
}

void brz_load(FILE* f,cmph_t* mphf)
{
   char*       buf   = NULL;
   DWORD       buflen;

   size_t      nbytes;
   DWORD       i, n;

   brz_data_t* brz   = (brz_data_t*) malloc(sizeof(brz_data_t));

   TRACE("Loading brz mphf\n");

   mphf->data = brz;

   nbytes = fread(&(brz->c),sizeof(double),(size_t) 1,f);
   nbytes = fread(&(brz->algo),sizeof(brz->algo),(size_t) 1,f); // Reading algo.
   nbytes = fread(&(brz->k),sizeof(DWORD),(size_t) 1,f);
   brz->size = (BYTE *) malloc(sizeof(BYTE) * brz->k);
   nbytes = fread(brz->size,sizeof(BYTE) * (brz->k),(size_t) 1,f);
   brz->h1 = (hash_state_t * *) malloc(sizeof(hash_state_t *) * brz->k);
   brz->h2 = (hash_state_t * *) malloc(sizeof(hash_state_t *) * brz->k);
   brz->g = (BYTE * *) calloc((size_t) brz->k,sizeof(BYTE *));

   TRACE("Reading c = %f   k = %u   algo = %u \n",brz->c,brz->k,brz->algo);

   //loading h_i1, h_i2 and g_i.
   for (i = 0; i < brz->k; i++)
   {
      // h1
      nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,f);

      TRACE("Hash state 1 has %u bytes\n",buflen);

      buf = (char *) malloc((size_t) buflen);
      nbytes = fread(buf,(size_t) buflen,(size_t) 1,f);
      brz->h1[i] = hash_state_load(buf,buflen);

      free(buf);

      //h2
      nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,f);

      TRACE("Hash state 2 has %u bytes\n",buflen);

      buf = (char *) malloc((size_t) buflen);
      nbytes = fread(buf,(size_t) buflen,(size_t) 1,f);
      brz->h2[i] = hash_state_load(buf,buflen);

      free(buf);

      switch (brz->algo)
      {
         case CMPH_FCH:
            n = fch_calc_b(brz->c,brz->size[i]);
            break;
         case CMPH_BMZ8:
            n = (DWORD) ceil(brz->c * brz->size[i]);
            break;
         default:
            ASSERT(0);
      }

      TRACE("g_i has %u bytes\n",n);

      brz->g[i] = (BYTE *) calloc((size_t) n,sizeof(BYTE));
      nbytes = fread(brz->g[i],sizeof(BYTE) * n,(size_t) 1,f);
   }

   //loading h0
   nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,f);

   TRACE("Hash state has %u bytes\n",buflen);

   buf = (char *) malloc((size_t) buflen);
   nbytes = fread(buf,(size_t) buflen,(size_t) 1,f);
   brz->h0 = hash_state_load(buf,buflen);

   free(buf);

   //loading c, m, and the vector offset.
   nbytes = fread(&(brz->m),sizeof(DWORD),(size_t) 1,f);
   brz->offset = (DWORD *) malloc(sizeof(DWORD) * brz->k);
   nbytes = fread(brz->offset,sizeof(DWORD) * (brz->k),(size_t) 1,f);
}

static DWORD brz_bmz8_search(brz_data_t* brz,const char* key,DWORD keylen,DWORD* fingerprint)
{
   DWORD h0;

   hash_vector(brz->h0,key,keylen,fingerprint);
   h0 = fingerprint[2] % brz->k;

   DWORD m  = brz->size[h0];
   DWORD n  = (DWORD) ceil(brz->c* m);
   DWORD h1 = hash(brz->h1[h0],key,keylen) % n;
   DWORD h2 = hash(brz->h2[h0],key,keylen) % n;
   BYTE  mphf_bucket;

   if (h1 == h2 && ++h2 >= n)
   {
      h2 = 0;
   }

   mphf_bucket = (BYTE) (brz->g[h0][h1] + brz->g[h0][h2]);

   TRACE("key: %s h1: %u h2: %u h0: %u\n",key,h1,h2,h0);
   TRACE("key: %s g[h1]: %u g[h2]: %u offset[h0]: %u edges: %u\n",key,brz->g[h0][h1],brz->g[h0][h2],brz->offset[h0],brz->m);
   TRACE("Address: %u\n",mphf_bucket + brz->offset[h0]);

   return (mphf_bucket + brz->offset[h0]);
}

static DWORD brz_fch_search(brz_data_t* brz,const char* key,DWORD keylen,DWORD* fingerprint)
{
   DWORD h0;

   hash_vector(brz->h0,key,keylen,fingerprint);
   h0 = fingerprint[2] % brz->k;

   DWORD    m           = brz->size[h0];
   DWORD    b           = fch_calc_b(brz->c,m);
   double   p1          = fch_calc_p1(m);
   double   p2          = fch_calc_p2(b);
   DWORD    h1          = hash(brz->h1[h0],key,keylen) % m;
   DWORD    h2          = hash(brz->h2[h0],key,keylen) % m;
   BYTE     mphf_bucket = 0;

   h1 = mixh10h11h12(b,p1,p2,h1);

   mphf_bucket = (BYTE) ((h2 + brz->g[h0][h1]) % m);

   return (mphf_bucket + brz->offset[h0]);
}

DWORD brz_search(cmph_t* mphf,const char* key,DWORD keylen)
{
   brz_data_t* brz   = (brz_data_t*) mphf->data;

   DWORD       fingerprint[3];

   switch (brz->algo)
   {
      case CMPH_FCH:
         return brz_fch_search(brz,key,keylen,fingerprint);
      case CMPH_BMZ8:
         return brz_bmz8_search(brz,key,keylen,fingerprint);
      default:
         ASSERT(0);
   }

   return 0;
}

void brz_destroy(cmph_t* mphf)
{
   DWORD       i;

   brz_data_t* data  = (brz_data_t*) mphf->data;

   if (data->g)
   {
      for (i = 0; i < data->k; i++)
      {
         free(data->g[i]);
         hash_state_destroy(data->h1[i]);
         hash_state_destroy(data->h2[i]);
      }

      free(data->g);
      free(data->h1);
      free(data->h2);
   }

   hash_state_destroy(data->h0);

   free(data->size);
   free(data->offset);
   free(data);
   free(mphf);
}

/** \fn void brz_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the resulting mphf. The size of packed_mphf must be at least cmph_packed_size()
 */
void brz_pack(cmph_t* mphf,void* packed_mphf)
{
   brz_data_t* data  = (brz_data_t*) mphf->data;
   BYTE*       ptr   = (BYTE*) packed_mphf;
   DWORD       i, n;

   // packing internal algo type
   memcpy(ptr,&(data->algo),sizeof(data->algo));
   ptr += sizeof(data->algo);

   // packing h0 type
   CMPH_HASH   h0_type  = hash_get_type(data->h0);
   memcpy(ptr,&h0_type,sizeof(h0_type));
   ptr += sizeof(h0_type);

   // packing h0
   hash_state_pack(data->h0,ptr);
   ptr += hash_state_packed_size(h0_type);

   // packing k
   memcpy(ptr,&(data->k),sizeof(data->k));
   ptr += sizeof(data->k);

   // packing c
   *((unsigned __int64 *) ptr) = (unsigned __int64) data->c;
   ptr += sizeof(data->c);

   // packing h1 type
   CMPH_HASH   h1_type  = hash_get_type(data->h1[0]);

   memcpy(ptr,&h1_type,sizeof(h1_type));

   ptr += sizeof(h1_type);

   // packing h2 type
   CMPH_HASH   h2_type  = hash_get_type(data->h2[0]);

   memcpy(ptr,&h2_type,sizeof(h2_type));

   ptr += sizeof(h2_type);

   // packing size
   memcpy(ptr,data->size,sizeof(BYTE) * data->k);
   ptr += data->k;

   // packing offset
   memcpy(ptr,data->offset,sizeof(DWORD) * data->k);

   ptr += sizeof(DWORD) * data->k;

   #if defined (__ia64) || defined (__x86_64__)
   cmph_uint64*   g_is_ptr = (cmph_uint64*) ptr;
   #else
   DWORD*         g_is_ptr = (DWORD*) ptr;
   #endif

   BYTE*          g_i      = (BYTE*) (g_is_ptr + data->k);

   for (i = 0; i < data->k; i++)
   {
      #if defined (__ia64) || defined (__x86_64__)
      *g_is_ptr++ = (cmph_uint64) g_i;
      #else
      *g_is_ptr++ = (DWORD) g_i;
      #endif
      // packing h1[i]
      hash_state_pack(data->h1[i],g_i);
      g_i += hash_state_packed_size(h1_type);

      // packing h2[i]
      hash_state_pack(data->h2[i],g_i);
      g_i += hash_state_packed_size(h2_type);

      // packing g_i
      switch (data->algo)
      {
         case CMPH_FCH:
            n = fch_calc_b(data->c,data->size[i]);
            break;
         case CMPH_BMZ8:
            n = (DWORD) ceil(data->c * data->size[i]);
            break;
         default:
            ASSERT(0);
      }
      memcpy(g_i,data->g[i],sizeof(BYTE) * n);
      g_i += n;
   }
}

/** \fn DWORD brz_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */
DWORD brz_packed_size(cmph_t* mphf)
{
   DWORD       i;
   DWORD       size     = 0;

   brz_data_t* data     = (brz_data_t*) mphf->data;

   CMPH_HASH   h0_type  = hash_get_type(data->h0);
   CMPH_HASH   h1_type  = hash_get_type(data->h1[0]);
   CMPH_HASH   h2_type  = hash_get_type(data->h2[0]);

   size = (DWORD) (2 * sizeof(CMPH_ALGO) + 3 * sizeof(CMPH_HASH) + hash_state_packed_size(h0_type) + sizeof(DWORD) + sizeof(double) + sizeof(BYTE) * data->k + sizeof(DWORD) * data->k);

   // pointers to g_is
   #if defined (__ia64) || defined (__x86_64__)
   size += (DWORD) sizeof(cmph_uint64) * data->k;
   #else
   size += (DWORD) sizeof(DWORD) * data->k;
   #endif

   size += hash_state_packed_size(h1_type) * data->k;
   size += hash_state_packed_size(h2_type) * data->k;

   DWORD n  = 0;
   for (i = 0; i < data->k; i++)
   {
      switch (data->algo)
      {
         case CMPH_FCH:
            n = fch_calc_b(data->c,data->size[i]);
            break;
         case CMPH_BMZ8:
            n = (DWORD) ceil(data->c * data->size[i]);
            break;
         default:
            ASSERT(0);
      }
      size += n;
   }

   return size;
}

static DWORD brz_bmz8_search_packed(DWORD* packed_mphf,const char* key,DWORD keylen,DWORD* fingerprint)
{
   CMPH_HASH   h0_type  = (CMPH_HASH) * packed_mphf++;
   DWORD*      h0_ptr   = packed_mphf;

   packed_mphf = (DWORD *) (((BYTE *) packed_mphf) + hash_state_packed_size(h0_type));

   DWORD    k  = *packed_mphf++;

   double   c  = (double) (__int64) (*((unsigned __int64*) packed_mphf));
   packed_mphf += 2;

   CMPH_HASH   h1_type  = (CMPH_HASH) * packed_mphf++;

   CMPH_HASH   h2_type  = (CMPH_HASH) * packed_mphf++;

   BYTE*       size     = (BYTE*) packed_mphf;
   packed_mphf = (DWORD *) (size + k);

   DWORD*   offset   = packed_mphf;
   packed_mphf += k;

   DWORD h0;

   hash_vector_packed(h0_ptr,h0_type,key,keylen,fingerprint);
   h0 = fingerprint[2] % k;

   DWORD          m        = size[h0];
   DWORD          n        = (DWORD) ceil(c* m);

   #if defined (__ia64) || defined (__x86_64__)
   cmph_uint64*   g_is_ptr = (cmph_uint64*) packed_mphf;
   #else
   DWORD*         g_is_ptr = packed_mphf;
   #endif

   BYTE*          h1_ptr   = (BYTE*) g_is_ptr[h0];
   BYTE*          h2_ptr   = h1_ptr + hash_state_packed_size(h1_type);
   BYTE*          g        = h2_ptr + hash_state_packed_size(h2_type);

   DWORD          h1       = hash_packed(h1_ptr,h1_type,key,keylen) % n;
   DWORD          h2       = hash_packed(h2_ptr,h2_type,key,keylen) % n;

   BYTE           mphf_bucket;

   if (h1 == h2 && ++h2 >= n)
   {
      h2 = 0;
   }

   mphf_bucket = (BYTE) (g[h1] + g[h2]);

   TRACE("key: %s h1: %u h2: %u h0: %u\n",key,h1,h2,h0);
   TRACE("Address: %u\n",mphf_bucket + offset[h0]);

   return (mphf_bucket + offset[h0]);
}

static DWORD brz_fch_search_packed(DWORD* packed_mphf,const char* key,DWORD keylen,DWORD* fingerprint)
{
   CMPH_HASH   h0_type  = (CMPH_HASH) * packed_mphf++;

   DWORD*      h0_ptr   = packed_mphf;
   packed_mphf = (DWORD *) (((BYTE *) packed_mphf) + hash_state_packed_size(h0_type));

   DWORD    k  = *packed_mphf++;

   double   c  = (double) (__int64) (*((unsigned __int64*) packed_mphf));
   packed_mphf += 2;

   CMPH_HASH   h1_type  = (CMPH_HASH) * packed_mphf++;

   CMPH_HASH   h2_type  = (CMPH_HASH) * packed_mphf++;

   BYTE*       size     = (BYTE*) packed_mphf;
   packed_mphf = (DWORD *) (size + k);

   DWORD*   offset   = packed_mphf;
   packed_mphf += k;

   DWORD h0;

   hash_vector_packed(h0_ptr,h0_type,key,keylen,fingerprint);
   h0 = fingerprint[2] % k;

   DWORD          m           = size[h0];
   DWORD          b           = fch_calc_b(c,m);

   double         p1          = fch_calc_p1(m);
   double         p2          = fch_calc_p2(b);

   #if defined (__ia64) || defined (__x86_64__)
   cmph_uint64*   g_is_ptr    = (cmph_uint64*) packed_mphf;
   #else
   DWORD*         g_is_ptr    = packed_mphf;
   #endif

   BYTE*          h1_ptr      = (BYTE*) g_is_ptr[h0];
   BYTE*          h2_ptr      = h1_ptr + hash_state_packed_size(h1_type);
   BYTE*          g           = h2_ptr + hash_state_packed_size(h2_type);

   DWORD          h1          = hash_packed(h1_ptr,h1_type,key,keylen) % m;
   DWORD          h2          = hash_packed(h2_ptr,h2_type,key,keylen) % m;

   BYTE           mphf_bucket = 0;

   h1 = mixh10h11h12(b,p1,p2,h1);

   mphf_bucket = (BYTE) ((h2 + g[h1]) % m);

   return (mphf_bucket + offset[h0]);
}

/** DWORD brz_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search.
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD brz_search_packed(void* packed_mphf,const char* key,DWORD keylen)
{
   DWORD*      ptr   = (DWORD*) packed_mphf;
   CMPH_ALGO   algo  = (CMPH_ALGO) * ptr++;

   DWORD       fingerprint[3];

   switch (algo)
   {
      case CMPH_FCH:
         return brz_fch_search_packed(ptr,key,keylen,fingerprint);
      case CMPH_BMZ8:
         return brz_bmz8_search_packed(ptr,key,keylen,fingerprint);
      default:
         ASSERT(0);
         return 0;
   }
}
