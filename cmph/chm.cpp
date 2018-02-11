#include "stdafx.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmph_types.h"
#include "jenkins_hash.h"
#include "hash_state.h"
#include "graph.h"
#include "cmph.h"
#include "hash.h"
#include "chm_structs.h"
#include "cmph_structs.h"
#include "hash.h"
#include "bitbool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int  chm_gen_edges(cmph_config_t* mph);
static void chm_traverse(chm_config_data_t* chm,BYTE* visited,DWORD v);

chm_config_data_t* chm_config_new(void)
{
   chm_config_data_t*chm   = NULL;

   chm = (chm_config_data_t *) malloc(sizeof(chm_config_data_t));

   if (!chm)
   {
      return NULL;
   }

   memset(chm,0,sizeof(chm_config_data_t));

   chm->hashfuncs[0] = CMPH_HASH_JENKINS;
   chm->hashfuncs[1] = CMPH_HASH_JENKINS;
   chm->g = NULL;
   chm->graph = NULL;
   chm->hashes = NULL;

   return chm;
}

void chm_config_destroy(cmph_config_t* mph)
{
   chm_config_data_t*data  = (chm_config_data_t*) mph->data;

   TRACE("Destroying algorithm dependent data\n");

   free(data);
}

void chm_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs)
{
   chm_config_data_t*chm      = (chm_config_data_t*) mph->data;

   CMPH_HASH*        hashptr  = hashfuncs;

   DWORD             i        = 0;

   while (*hashptr != CMPH_HASH_COUNT)
   {
      if (i >= 2)
      {
         break;
      } //chm only uses two hash functions

      chm->hashfuncs[i] = *hashptr;

      ++i; 
      ++hashptr;
   }
}

cmph_t* chm_new(cmph_config_t* mph,double c)
{
   cmph_t*           mphf        = NULL;
   chm_data_t*       chmf        = NULL;

   DWORD             i;
   DWORD             iterations  = 20;
   BYTE*             visited     = NULL;

   chm_config_data_t*chm         = (chm_config_data_t*) mph->data;

   chm->m = mph->key_source->nkeys;

   if (c == 0)
   {
      c = 2.09;
   }

   chm->n = (DWORD) ceil(c * mph->key_source->nkeys);

   TRACE("m (edges): %u n (vertices): %u c: %f\n",chm->m,chm->n,c);

   chm->graph = graph_new(chm->n,chm->m);

   TRACE("Created graph\n");

   chm->hashes = (hash_state_t * *) malloc(sizeof(hash_state_t *) * 3);

   for (i = 0; i < 3; ++i)
   {
      chm->hashes[i] = NULL;
   }

   //Mapping step
   if (mph->verbosity)
   {
      fprintf(stderr,"Entering mapping step for mph creation of %u keys with graph sized %u\n",chm->m,chm->n);
   }

   while (true)
   {
      int   ok;
      chm->hashes[0] = hash_state_new(chm->hashfuncs[0],chm->n);
      chm->hashes[1] = hash_state_new(chm->hashfuncs[1],chm->n);
      ok = chm_gen_edges(mph);

      if (!ok)
      {
         --iterations;
         hash_state_destroy(chm->hashes[0]);
         chm->hashes[0] = NULL;
         hash_state_destroy(chm->hashes[1]);
         chm->hashes[1] = NULL;

         TRACE("%u iterations remaining\n",iterations);

         if (mph->verbosity)
         {
            fprintf(stderr,"Acyclic graph creation failure - %u iterations remaining\n",iterations);
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
      graph_destroy(chm->graph);
      return NULL;
   }

   //Assignment step
   if (mph->verbosity)
   {
      fprintf(stderr,"Starting assignment step\n");
   }

   TRACE("Assignment step\n");

   visited = (BYTE *) malloc((size_t) (chm->n / 8 + 1));

   memset(visited,0,(size_t) (chm->n / 8 + 1));

   free(chm->g);

   chm->g = (DWORD *) malloc(chm->n * sizeof(DWORD));

   ASSERT(chm->g);

   for (i = 0; i < chm->n; ++i)
   {
      if (!GETBIT(visited,i))
      {
         chm->g[i] = 0;
         chm_traverse(chm,visited,i);
      }
   }

   graph_destroy(chm->graph);

   free(visited);

   chm->graph = NULL;

   mphf = (cmph_t *) malloc(sizeof(cmph_t));
   mphf->algo = mph->algo;

   chmf = (chm_data_t *) malloc(sizeof(chm_data_t));
   chmf->g = chm->g;
   chm->g = NULL; //transfer memory ownership
   chmf->hashes = chm->hashes;
   chm->hashes = NULL; //transfer memory ownership
   chmf->n = chm->n;
   chmf->m = chm->m;

   mphf->data = chmf;
   mphf->size = chm->m;

   TRACE("Successfully generated minimal perfect hash\n");

   if (mph->verbosity)
   {
      fprintf(stderr,"Successfully generated minimal perfect hash function\n");
   }

   return mphf;
}

static void chm_traverse(chm_config_data_t* chm,BYTE* visited,DWORD v)
{
   graph_iterator_t  it       = graph_neighbors_it(chm->graph,v);

   DWORD             neighbor = 0;

   SETBIT(visited,v);

   TRACE("Visiting vertex %u\n",v);

   while ((neighbor = graph_next_neighbor(chm->graph,&it)) != GRAPH_NO_NEIGHBOR)
   {
      TRACE("Visiting neighbor %u\n",neighbor);

      if (GETBIT(visited,neighbor))
      {
         continue;
      }

      TRACE("Visiting neighbor %u\n",neighbor);
      TRACE("Visiting edge %u->%u with id %u\n",v,neighbor,graph_edge_id(chm->graph,v,neighbor));

      chm->g[neighbor] = graph_edge_id(chm->graph,v,neighbor) - chm->g[v];

      TRACE("g is %u (%u - %u mod %u)\n",chm->g[neighbor],graph_edge_id(chm->graph,v,neighbor),chm->g[v],chm->m);

      chm_traverse(chm,visited,neighbor);
   }
}

static int chm_gen_edges(cmph_config_t* mph)
{
   DWORD             e;

   chm_config_data_t*chm      = (chm_config_data_t*) mph->data;

   int               cycles   = 0;

   TRACE("Generating edges for %u vertices with hash functions %s and %s\n",chm->n,cmph_hash_names[chm->hashfuncs[0]],cmph_hash_names[chm->hashfuncs[1]]);

   graph_clear_edges(chm->graph);

   mph->key_source->rewind(mph->key_source->data);

   for (e = 0; e < mph->key_source->nkeys; ++e)
   {
      DWORD h1, h2;
      DWORD keylen;
      char* key;

      mph->key_source->read(mph->key_source->data,&key,&keylen);

      h1 = hash(chm->hashes[0],key,keylen) % chm->n;
      h2 = hash(chm->hashes[1],key,keylen) % chm->n;

      if (h1 == h2)
      {
         if (++h2 >= chm->n)
         {
            h2 = 0;
         }
      }

      if (h1 == h2)
      {
         if (mph->verbosity)
         {
            fprintf(stderr,"Self loop for key %u\n",e);
         }
         mph->key_source->dispose(mph->key_source->data,key,keylen);
         return 0;
      }

      TRACE("Adding edge: %u -> %u for key %s\n",h1,h2,key);

      mph->key_source->dispose(mph->key_source->data,key,keylen);
      graph_add_edge(chm->graph,h1,h2);
   }

   cycles = graph_is_cyclic(chm->graph);

   if (mph->verbosity && cycles)
   {
      fprintf(stderr,"Cyclic graph generated\n");
   }

   TRACE("Looking for cycles: %u\n",cycles);

   return !cycles;
}

int chm_dump(cmph_t* mphf,FILE* fd)
{
   char*       buf   = NULL;
   DWORD       buflen;
   DWORD       two   = 2; //number of hash functions
   chm_data_t* data  = (chm_data_t*) mphf->data;
   size_t      nbytes;

   __cmph_dump(mphf,fd);

   nbytes = fwrite(&two,sizeof(DWORD),(size_t) 1,fd);

   hash_state_dump(data->hashes[0],&buf,&buflen);

   TRACE("Dumping hash state with %u bytes to disk\n",buflen);

   nbytes = fwrite(&buflen,sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(buf,(size_t) buflen,(size_t) 1,fd);

   free(buf);

   hash_state_dump(data->hashes[1],&buf,&buflen);

   TRACE("Dumping hash state with %u bytes to disk\n",buflen);

   nbytes = fwrite(&buflen,sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(buf,(size_t) buflen,(size_t) 1,fd);

   free(buf);

   nbytes = fwrite(&(data->n),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(&(data->m),sizeof(DWORD),(size_t) 1,fd);

   nbytes = fwrite(data->g,sizeof(DWORD) * data->n,(size_t) 1,fd);

   return 1;
}

void chm_load(FILE* f,cmph_t* mphf)
{
   DWORD       nhashes;
   char*       buf   = NULL;
   DWORD       buflen;
   DWORD       i;
   chm_data_t* chm   = (chm_data_t*) malloc(sizeof(chm_data_t));
   size_t      nbytes;

   TRACE("Loading chm mphf\n");

   mphf->data = chm;
   nbytes = fread(&nhashes,sizeof(DWORD),(size_t) 1,f);
   chm->hashes = (hash_state_t * *) malloc(sizeof(hash_state_t *) * (nhashes + 1));
   chm->hashes[nhashes] = NULL;

   TRACE("Reading %u hashes\n",nhashes);

   for (i = 0; i < nhashes; ++i)
   {
      hash_state_t*  state = NULL;
      nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,f);

      TRACE("Hash state has %u bytes\n",buflen);

      buf = (char *) malloc((size_t) buflen);
      nbytes = fread(buf,(size_t) buflen,(size_t) 1,f);
      state = hash_state_load(buf,buflen);
      chm->hashes[i] = state;

      free(buf);
   }

   TRACE("Reading m and n\n");

   nbytes = fread(&(chm->n),sizeof(DWORD),(size_t) 1,f);
   nbytes = fread(&(chm->m),sizeof(DWORD),(size_t) 1,f);

   chm->g = (DWORD *) malloc(sizeof(DWORD) * chm->n);
   nbytes = fread(chm->g,chm->n * sizeof(DWORD),(size_t) 1,f);

   #ifdef DEBUG
   fprintf(stderr,"G: ");
   for (i = 0; i < chm->n; ++i)
   {
      fprintf(stderr,"%u ",chm->g[i]);
   }
   fprintf(stderr,"\n");
   #endif

   return;
}


DWORD chm_search(cmph_t* mphf,const char* key,DWORD keylen)
{
   chm_data_t* chm   = (chm_data_t*) mphf->data;
   DWORD       h1    = hash(chm->hashes[0],key,keylen) % chm->n;
   DWORD       h2    = hash(chm->hashes[1],key,keylen) % chm->n;

   TRACE("key: %s h1: %u h2: %u\n",key,h1,h2);

   if (h1 == h2 && ++h2 >= chm->n)
   {
      h2 = 0;
   }

   TRACE("key: %s g[h1]: %u g[h2]: %u edges: %u\n",key,chm->g[h1],chm->g[h2],chm->m);

   return (chm->g[h1] + chm->g[h2]) % chm->m;
}
void chm_destroy(cmph_t* mphf)
{
   chm_data_t* data  = (chm_data_t*) mphf->data;

   free(data->g);

   hash_state_destroy(data->hashes[0]);
   hash_state_destroy(data->hashes[1]);

   free(data->hashes);
   free(data);
   free(mphf);
}

/** \fn void chm_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the resulting mphf. The size of packed_mphf must be at least cmph_packed_size()
 */
void chm_pack(cmph_t* mphf,void* packed_mphf)
{
   chm_data_t* data     = (chm_data_t*) mphf->data;
   BYTE*       ptr      = (BYTE*) packed_mphf;

   // packing h1 type
   CMPH_HASH   h1_type  = hash_get_type(data->hashes[0]);
   *((DWORD *) ptr) = h1_type;
   ptr += sizeof(DWORD);

   // packing h1
   hash_state_pack(data->hashes[0],ptr);
   ptr += hash_state_packed_size(h1_type);

   // packing h2 type
   CMPH_HASH   h2_type  = hash_get_type(data->hashes[1]);
   *((DWORD *) ptr) = h2_type;
   ptr += sizeof(DWORD);

   // packing h2
   hash_state_pack(data->hashes[1],ptr);
   ptr += hash_state_packed_size(h2_type);

   // packing n
   *((DWORD *) ptr) = data->n;
   ptr += sizeof(data->n);

   // packing m
   *((DWORD *) ptr) = data->m;
   ptr += sizeof(data->m);

   // packing g
   memcpy(ptr,data->g,sizeof(DWORD) * data->n);
}

/** \fn DWORD chm_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */
DWORD chm_packed_size(cmph_t* mphf)
{
   chm_data_t* data     = (chm_data_t*) mphf->data;
   CMPH_HASH   h1_type  = hash_get_type(data->hashes[0]);
   CMPH_HASH   h2_type  = hash_get_type(data->hashes[1]);

   return (DWORD) (sizeof(CMPH_ALGO) + hash_state_packed_size(h1_type) + hash_state_packed_size(h2_type) + 4 * sizeof(DWORD) + sizeof(DWORD) * data->n);
}

/** DWORD chm_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search.
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD chm_search_packed(void* packed_mphf,const char* key,DWORD keylen)
{
   BYTE*       h1_ptr   = (BYTE*) packed_mphf;
   CMPH_HASH   h1_type  = (CMPH_HASH) (*((DWORD*) h1_ptr));
   h1_ptr += 4;

   BYTE*       h2_ptr   = h1_ptr + hash_state_packed_size(h1_type);
   CMPH_HASH   h2_type  = (CMPH_HASH) (*((DWORD*) h2_ptr));
   h2_ptr += 4;

   DWORD*   g_ptr = (DWORD*) (h2_ptr + hash_state_packed_size(h2_type));

   DWORD    n     = *g_ptr++;
   DWORD    m     = *g_ptr++;

   DWORD    h1    = hash_packed(h1_ptr,h1_type,key,keylen) % n;
   DWORD    h2    = hash_packed(h2_ptr,h2_type,key,keylen) % n;

   TRACE("key: %s h1: %u h2: %u\n",key,h1,h2);

   if (h1 == h2 && ++h2 >= n)
   {
      h2 = 0;
   }

   TRACE("key: %s g[h1]: %u g[h2]: %u edges: %u\n",key,g_ptr[h1],g_ptr[h2],m);

   return (g_ptr[h1] + g_ptr[h2]) % m;
}
