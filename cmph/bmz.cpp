#include "stdafx.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmph_types.h"
#include "cmph.h"
#include "graph.h"
#include "bmz.h"
#include "jenkins_hash.h"
#include "hash_state.h"
#include "cmph_structs.h"
#include "hash.h"
#include "bmz_structs.h"
#include "hash.h"
#include "vqueue.h"
#include "bitbool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int  bmz_gen_edges(cmph_config_t* mph);
static BYTE bmz_traverse_critical_nodes(bmz_config_data_t* bmz,DWORD v,DWORD* biggest_g_value,DWORD* biggest_edge_value,BYTE* used_edges,BYTE* visited);
static BYTE bmz_traverse_critical_nodes_heuristic(bmz_config_data_t* bmz,DWORD v,DWORD* biggest_g_value,DWORD* biggest_edge_value,BYTE* used_edges,BYTE* visited);
static void bmz_traverse_non_critical_nodes(bmz_config_data_t* bmz,BYTE* used_edges,BYTE* visited);

bmz_config_data_t* bmz_config_new(void)
{
   bmz_config_data_t*   bmz   = NULL;
   bmz = (bmz_config_data_t *) malloc(sizeof(bmz_config_data_t));
   if (!bmz)
   {
      return NULL;
   }
   memset(bmz,0,sizeof(bmz_config_data_t));
   bmz->hashfuncs[0] = CMPH_HASH_JENKINS;
   bmz->hashfuncs[1] = CMPH_HASH_JENKINS;
   bmz->g = NULL;
   bmz->graph = NULL;
   bmz->hashes = NULL;
   return bmz;
}

void bmz_config_destroy(cmph_config_t* mph)
{
   bmz_config_data_t*   data  = (bmz_config_data_t*) mph->data;

   TRACE("Destroying algorithm dependent data\n");
   free(data);
}

void bmz_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs)
{
   bmz_config_data_t*   bmz      = (bmz_config_data_t*) mph->data;
   CMPH_HASH*           hashptr  = hashfuncs;
   DWORD                i        = 0;
   while (*hashptr != CMPH_HASH_COUNT)
   {
      if (i >= 2)
      {
         break;
      } //bmz only uses two hash functions
      bmz->hashfuncs[i] = *hashptr;
      ++i, ++hashptr;
   }
}

cmph_t* bmz_new(cmph_config_t* mph,double c)
{
   cmph_t*              mphf              = NULL;
   bmz_data_t*          bmzf              = NULL;
   DWORD                i;
   DWORD                iterations;
   DWORD                iterations_map    = 20;
   BYTE*                used_edges        = NULL;
   BYTE                 restart_mapping   = 0;
   BYTE*                visited           = NULL;

   bmz_config_data_t*   bmz               = (bmz_config_data_t*) mph->data;

   if (c == 0)
   {
      c = 1.15;
   } // validating restrictions over parameter c.

   TRACE("c: %f\n",c);

   bmz->m = mph->key_source->nkeys;
   bmz->n = (DWORD) ceil(c * mph->key_source->nkeys);

   TRACE("m (edges): %u n (vertices): %u c: %f\n",bmz->m,bmz->n,c);

   bmz->graph = graph_new(bmz->n,bmz->m);

   TRACE("Created graph\n");

   bmz->hashes = (hash_state_t * *) malloc(sizeof(hash_state_t *) * 3);

   for (i = 0; i < 3; ++i)
   {
      bmz->hashes[i] = NULL;
   }

   do
   {
      // Mapping step
      DWORD biggest_g_value      = 0;
      DWORD biggest_edge_value   = 1;

      iterations = 100;

      if (mph->verbosity)
      {
         fprintf(stderr,"Entering mapping step for mph creation of %u keys with graph sized %u\n",bmz->m,bmz->n);
      }

      while (true)
      {
         int   ok;

         TRACE("hash function 1\n");

         bmz->hashes[0] = hash_state_new(bmz->hashfuncs[0],bmz->n);

         TRACE("hash function 2\n");

         bmz->hashes[1] = hash_state_new(bmz->hashfuncs[1],bmz->n);

         TRACE("Generating edges\n");

         ok = bmz_gen_edges(mph);

         if (!ok)
         {
            --iterations;
            hash_state_destroy(bmz->hashes[0]);
            bmz->hashes[0] = NULL;
            hash_state_destroy(bmz->hashes[1]);
            bmz->hashes[1] = NULL;

            TRACE("%u iterations remaining\n",iterations);

            if (mph->verbosity)
            {
               fprintf(stderr,"simple graph creation failure - %u iterations remaining\n",iterations);
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
         graph_destroy(bmz->graph);
         return NULL;
      }

      // Ordering step
      if (mph->verbosity)
      {
         fprintf(stderr,"Starting ordering step\n");
      }

      graph_obtain_critical_nodes(bmz->graph);

      // Searching step
      if (mph->verbosity)
      {
         fprintf(stderr,"Starting Searching step.\n");
         fprintf(stderr,"\tTraversing critical vertices.\n");
      }

      TRACE("Searching step\n");

      visited = (BYTE *) malloc((size_t) bmz->n / 8 + 1);

      memset(visited,0,(size_t) bmz->n / 8 + 1);

      used_edges = (BYTE *) malloc((size_t) bmz->m / 8 + 1);

      memset(used_edges,0,(size_t) bmz->m / 8 + 1);

      free(bmz->g);

      bmz->g = (DWORD *) calloc((size_t) bmz->n,sizeof(DWORD));

      ASSERT(bmz->g);

      for (i = 0; i < bmz->n; ++i) // critical nodes
      {
         if (graph_node_is_critical(bmz->graph,i) && (!GETBIT(visited,i)))
         {
            if (c > 1.14)
            {
               restart_mapping = bmz_traverse_critical_nodes(bmz,i,&biggest_g_value,&biggest_edge_value,used_edges,visited);
            }
            else
            {
               restart_mapping = bmz_traverse_critical_nodes_heuristic(bmz,i,&biggest_g_value,&biggest_edge_value,used_edges,visited);
            }

            if (restart_mapping)
            {
               break;
            }
         }
      }

      if (!restart_mapping)
      {
         if (mph->verbosity)
         {
            fprintf(stderr,"\tTraversing non critical vertices.\n");
         }
         bmz_traverse_non_critical_nodes(bmz,used_edges,visited); // non_critical_nodes
      }
      else
      {
         iterations_map--;
         if (mph->verbosity)
         {
            fprintf(stderr,"Restarting mapping step. %u iterations remaining.\n",iterations_map);
         }
      }

      free(used_edges);
      free(visited);
   }
   while (restart_mapping && iterations_map > 0);

   graph_destroy(bmz->graph);

   bmz->graph = NULL;

   if (iterations_map == 0)
   {
      return NULL;
   }

   #ifdef DEBUG
   fprintf(stderr,"G: ");
   for (i = 0; i < bmz->n; ++i)
   {
      fprintf(stderr,"%u ",bmz->g[i]);
   }
   fprintf(stderr,"\n");
   #endif

   mphf = (cmph_t *) malloc(sizeof(cmph_t));
   mphf->algo = mph->algo;

   bmzf = (bmz_data_t *) malloc(sizeof(bmz_data_t));
   bmzf->g = bmz->g;
   bmz->g = NULL; //transfer memory ownership
   bmzf->hashes = bmz->hashes;
   bmz->hashes = NULL; //transfer memory ownership
   bmzf->n = bmz->n;
   bmzf->m = bmz->m;

   mphf->data = bmzf;
   mphf->size = bmz->m;

   TRACE("Successfully generated minimal perfect hash\n");

   if (mph->verbosity)
   {
      fprintf(stderr,"Successfully generated minimal perfect hash function\n");
   }

   return mphf;
}

static BYTE bmz_traverse_critical_nodes(bmz_config_data_t* bmz,DWORD v,DWORD* biggest_g_value,DWORD* biggest_edge_value,BYTE* used_edges,BYTE* visited)
{
   DWORD             next_g;
   DWORD             u;   /* Auxiliary vertex */
   DWORD             lav; /* lookahead vertex */
   BYTE              collision;
   vqueue_t*         q  = vqueue_new((DWORD) (graph_ncritical_nodes(bmz->graph)) + 1);
   graph_iterator_t  it, it1;

   TRACE("Labelling critical vertices\n");

   bmz->g[v] = (DWORD) ceil((double) (*biggest_edge_value) / 2) - 1;

   SETBIT(visited,v);

   next_g = (DWORD) floor((double) (*biggest_edge_value / 2)); /* next_g is incremented in the do..while statement*/

   vqueue_insert(q,v);

   while (!vqueue_is_empty(q))
   {
      v = vqueue_remove(q);
      it = graph_neighbors_it(bmz->graph,v);

      while ((u = graph_next_neighbor(bmz->graph,&it)) != GRAPH_NO_NEIGHBOR)
      {
         if (graph_node_is_critical(bmz->graph,u) && (!GETBIT(visited,u)))
         {
            collision = 1;
            while (collision) // lookahead to resolve collisions
            {
               next_g = *biggest_g_value + 1;
               it1 = graph_neighbors_it(bmz->graph,u);

               collision = 0;

               while ((lav = graph_next_neighbor(bmz->graph,&it1)) != GRAPH_NO_NEIGHBOR)
               {
                  if (graph_node_is_critical(bmz->graph,lav) && GETBIT(visited,lav))
                  {
                     if (next_g + bmz->g[lav] >= bmz->m)
                     {
                        vqueue_destroy(q);
                        return 1; // restart mapping step.
                     }

                     if (GETBIT(used_edges,(next_g + bmz->g[lav])))
                     {
                        collision = 1;
                        break;
                     }
                  }
               }

               if (next_g > *biggest_g_value)
               {
                  *biggest_g_value = next_g;
               }
            }

            // Marking used edges...
            it1 = graph_neighbors_it(bmz->graph,u);

            while ((lav = graph_next_neighbor(bmz->graph,&it1)) != GRAPH_NO_NEIGHBOR)
            {
               if (graph_node_is_critical(bmz->graph,lav) && GETBIT(visited,lav))
               {
                  SETBIT(used_edges,(next_g + bmz->g[lav]));
                  if (next_g + bmz->g[lav] > *biggest_edge_value)
                  {
                     *biggest_edge_value = next_g + bmz->g[lav];
                  }
               }
            }

            bmz->g[u] = next_g; // Labelling vertex u.
            SETBIT(visited,u);
            vqueue_insert(q,u);
         }
      }
   }

   vqueue_destroy(q);
   return 0;
}

static BYTE bmz_traverse_critical_nodes_heuristic(bmz_config_data_t* bmz,DWORD v,DWORD* biggest_g_value,DWORD* biggest_edge_value,BYTE* used_edges,BYTE* visited)
{
   DWORD             next_g;
   DWORD             u;   /* Auxiliary vertex */
   DWORD             lav; /* lookahead vertex */
   BYTE              collision;
   DWORD*            unused_g_values            = NULL;
   DWORD             unused_g_values_capacity   = 0;
   DWORD             nunused_g_values           = 0;
   vqueue_t*         q                          = vqueue_new((DWORD) (0.5 * graph_ncritical_nodes(bmz->graph)) + 1);
   graph_iterator_t  it, it1;

   TRACE("Labelling critical vertices\n");

   bmz->g[v] = (DWORD) ceil((double) (*biggest_edge_value) / 2) - 1;

   SETBIT(visited,v);

   next_g = (DWORD) floor((double) (*biggest_edge_value / 2)); /* next_g is incremented in the do..while statement*/

   vqueue_insert(q,v);

   while (!vqueue_is_empty(q))
   {
      v = vqueue_remove(q);

      it = graph_neighbors_it(bmz->graph,v);

      while ((u = graph_next_neighbor(bmz->graph,&it)) != GRAPH_NO_NEIGHBOR)
      {
         if (graph_node_is_critical(bmz->graph,u) && (!GETBIT(visited,u)))
         {
            DWORD next_g_index   = 0;
            collision = 1;

            while (collision) // lookahead to resolve collisions
            {
               if (next_g_index < nunused_g_values)
               {
                  next_g = unused_g_values[next_g_index++];
               }
               else
               {
                  next_g = *biggest_g_value + 1;
                  next_g_index = UINT_MAX;
               }

               it1 = graph_neighbors_it(bmz->graph,u);

               collision = 0;

               while ((lav = graph_next_neighbor(bmz->graph,&it1)) != GRAPH_NO_NEIGHBOR)
               {
                  if (graph_node_is_critical(bmz->graph,lav) && GETBIT(visited,lav))
                  {
                     if (next_g + bmz->g[lav] >= bmz->m)
                     {
                        vqueue_destroy(q);
                        free(unused_g_values);
                        return 1; // restart mapping step.
                     }

                     if (GETBIT(used_edges,(next_g + bmz->g[lav])))
                     {
                        collision = 1;
                        break;
                     }
                  }
               }

               if (collision && (next_g > *biggest_g_value)) // saving the current g value stored in next_g.
               {
                  if (nunused_g_values == unused_g_values_capacity)
                  {
                     unused_g_values = (DWORD *) realloc(unused_g_values,(unused_g_values_capacity + BUFSIZ) * sizeof(DWORD));
                     unused_g_values_capacity += BUFSIZ;
                  }
                  unused_g_values[nunused_g_values++] = next_g;
               }

               if (next_g > *biggest_g_value)
               {
                  *biggest_g_value = next_g;
               }
            }

            next_g_index--;

            if (next_g_index < nunused_g_values)
            {
               unused_g_values[next_g_index] = unused_g_values[--nunused_g_values];
            }

            // Marking used edges...
            it1 = graph_neighbors_it(bmz->graph,u);

            while ((lav = graph_next_neighbor(bmz->graph,&it1)) != GRAPH_NO_NEIGHBOR)
            {
               if (graph_node_is_critical(bmz->graph,lav) && GETBIT(visited,lav))
               {
                  SETBIT(used_edges,(next_g + bmz->g[lav]));
                  if (next_g + bmz->g[lav] > *biggest_edge_value)
                  {
                     *biggest_edge_value = next_g + bmz->g[lav];
                  }
               }
            }

            bmz->g[u] = next_g; // Labelling vertex u.

            SETBIT(visited,u);

            vqueue_insert(q,u);
         }
      }
   }

   vqueue_destroy(q);

   free(unused_g_values);

   return 0;
}

static DWORD next_unused_edge(bmz_config_data_t* bmz,BYTE* used_edges,DWORD unused_edge_index)
{
   while (true)
   {
      ASSERT(unused_edge_index < bmz->m);

      if (GETBIT(used_edges,unused_edge_index))
      {
         unused_edge_index ++;
      }
      else
      {
         break;
      }
   }
   return unused_edge_index;
}

static void bmz_traverse(bmz_config_data_t* bmz,BYTE* used_edges,DWORD v,DWORD* unused_edge_index,BYTE* visited)
{
   graph_iterator_t  it       = graph_neighbors_it(bmz->graph,v);
   DWORD             neighbor = 0;
   while ((neighbor = graph_next_neighbor(bmz->graph,&it)) != GRAPH_NO_NEIGHBOR)
   {
      if (GETBIT(visited,neighbor))
      {
         continue;
      }
      //DEBUGP("Visiting neighbor %u\n", neighbor);
      *unused_edge_index = next_unused_edge(bmz,used_edges,*unused_edge_index);
      bmz->g[neighbor] = *unused_edge_index - bmz->g[v];
      //if (bmz->g[neighbor] >= bmz->m) bmz->g[neighbor] += bmz->m;
      SETBIT(visited,neighbor);
      (*unused_edge_index)++;
      bmz_traverse(bmz,used_edges,neighbor,unused_edge_index,visited);
   }
}

static void bmz_traverse_non_critical_nodes(bmz_config_data_t* bmz,BYTE* used_edges,BYTE* visited)
{
   DWORD i, v1, v2, unused_edge_index = 0;

   TRACE("Labelling non critical vertices\n");

   for (i = 0; i < bmz->m; i++)
   {
      v1 = graph_vertex_id(bmz->graph,i,0);
      v2 = graph_vertex_id(bmz->graph,i,1);

      if ((GETBIT(visited,v1) && GETBIT(visited,v2)) || (!GETBIT(visited,v1) && !GETBIT(visited,v2)))
      {
         continue;
      }            

      if (GETBIT(visited,v1))
      {
         bmz_traverse(bmz,used_edges,v1,&unused_edge_index,visited);
      }
      else
      {
         bmz_traverse(bmz,used_edges,v2,&unused_edge_index,visited);
      }
   }

   for (i = 0; i < bmz->n; i++)
   {
      if (!GETBIT(visited,i))
      {
         bmz->g[i] = 0;
         SETBIT(visited,i);
         bmz_traverse(bmz,used_edges,i,&unused_edge_index,visited);
      }
   }
}

static int bmz_gen_edges(cmph_config_t* mph)
{
   DWORD                e;
   bmz_config_data_t*   bmz            = (bmz_config_data_t*) mph->data;
   BYTE                 multiple_edges = 0;

   TRACE("Generating edges for %u vertices\n",bmz->n);

   graph_clear_edges(bmz->graph);

   mph->key_source->rewind(mph->key_source->data);

   for (e = 0; e < mph->key_source->nkeys; ++e)
   {
      DWORD h1, h2;
      DWORD keylen;
      char* key   = NULL;

      mph->key_source->read(mph->key_source->data,&key,&keylen);

      h1 = hash(bmz->hashes[0],key,keylen) % bmz->n;
      h2 = hash(bmz->hashes[1],key,keylen) % bmz->n;

      if (h1 == h2)
      {
         if (++h2 >= bmz->n)
         {
            h2 = 0;
         }
      }

      TRACE("key: %.*s h1: %u h2: %u\n",keylen,key,h1,h2);

      if (h1 == h2)
      {
         if (mph->verbosity)
         {
            fprintf(stderr,"Self loop for key %u\n",e);
         }
         mph->key_source->dispose(mph->key_source->data,key,keylen);
         return 0;
      }

      TRACE("Adding edge: %u -> %u for key %.*s\n",h1,h2,keylen,key);

      mph->key_source->dispose(mph->key_source->data,key,keylen);
      multiple_edges = graph_contains_edge(bmz->graph,h1,h2);

      if (mph->verbosity && multiple_edges)
      {
         fprintf(stderr,"A non simple graph was generated\n");
      }
      if (multiple_edges)
      {
         return 0;
      } // checking multiple edge restriction.

      graph_add_edge(bmz->graph,h1,h2);
   }
   return !multiple_edges;
}

int bmz_dump(cmph_t* mphf,FILE* fd)
{
   char*       buf   = NULL;
   DWORD       buflen;
   DWORD       two   = 2; //number of hash functions

   bmz_data_t* data  = (bmz_data_t*) mphf->data;

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

   nbytes = fwrite(data->g,sizeof(DWORD) * (data->n),(size_t) 1,fd);

   #ifdef DEBUG
   DWORD i;
   fprintf(stderr,"G: ");
   for (i = 0; i < data->n; ++i)
   {
      fprintf(stderr,"%u ",data->g[i]);
   }
   fprintf(stderr,"\n");
   #endif

   return 1;
}

void bmz_load(FILE* f,cmph_t* mphf)
{
   DWORD       nhashes;
   char*       buf   = NULL;
   DWORD       buflen;
   DWORD       i;

   bmz_data_t* bmz   = (bmz_data_t*) malloc(sizeof(bmz_data_t));
   size_t      nbytes;

   TRACE("Loading bmz mphf\n");

   mphf->data = bmz;
   nbytes = fread(&nhashes,sizeof(DWORD),(size_t) 1,f);

   bmz->hashes = (hash_state_t * *) malloc(sizeof(hash_state_t *) * (nhashes + 1));
   bmz->hashes[nhashes] = NULL;

   TRACE("Reading %u hashes\n",nhashes);

   for (i = 0; i < nhashes; ++i)
   {
      hash_state_t*  state = NULL;
      nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,f);

      TRACE("Hash state has %u bytes\n",buflen);

      buf = (char *) malloc((size_t) buflen);
      nbytes = fread(buf,(size_t) buflen,(size_t) 1,f);
      state = hash_state_load(buf,buflen);
      bmz->hashes[i] = state;
      free(buf);
   }

   TRACE("Reading m and n\n");

   nbytes = fread(&(bmz->n),sizeof(DWORD),(size_t) 1,f);
   nbytes = fread(&(bmz->m),sizeof(DWORD),(size_t) 1,f);

   bmz->g = (DWORD *) malloc(sizeof(DWORD) * bmz->n);
   nbytes = fread(bmz->g,bmz->n * sizeof(DWORD),(size_t) 1,f);

   #ifdef DEBUG
   fprintf(stderr,"G: ");
   for (i = 0; i < bmz->n; ++i)
   {
      fprintf(stderr,"%u ",bmz->g[i]);
   }
   fprintf(stderr,"\n");
   #endif
}

DWORD bmz_search(cmph_t* mphf,const char* key,DWORD keylen)
{
   bmz_data_t* bmz   = (bmz_data_t*) mphf->data;
   DWORD       h1    = hash(bmz->hashes[0],key,keylen) % bmz->n;
   DWORD       h2    = hash(bmz->hashes[1],key,keylen) % bmz->n;

   TRACE("key: %.*s h1: %u h2: %u\n",keylen,key,h1,h2);

   if (h1 == h2 && ++h2 > bmz->n)
   {
      h2 = 0;
   }

   TRACE("key: %.*s g[h1]: %u g[h2]: %u edges: %u\n",keylen,key,bmz->g[h1],bmz->g[h2],bmz->m);

   return bmz->g[h1] + bmz->g[h2];
}

void bmz_destroy(cmph_t* mphf)
{
   bmz_data_t* data  = (bmz_data_t*) mphf->data;

   free(data->g);

   hash_state_destroy(data->hashes[0]);
   hash_state_destroy(data->hashes[1]);

   free(data->hashes);
   free(data);
   free(mphf);
}

/** \fn void bmz_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the resulting mphf. The size of packed_mphf must be at least cmph_packed_size()
 */
void bmz_pack(cmph_t* mphf,void* packed_mphf)
{
   bmz_data_t* data     = (bmz_data_t*) mphf->data;
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

   // packing g
   memcpy(ptr,data->g,sizeof(DWORD) * data->n);
}

/** \fn DWORD bmz_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */
DWORD bmz_packed_size(cmph_t* mphf)
{
   bmz_data_t* data     = (bmz_data_t*) mphf->data;
   CMPH_HASH   h1_type  = hash_get_type(data->hashes[0]);
   CMPH_HASH   h2_type  = hash_get_type(data->hashes[1]);

   return (DWORD) (sizeof(CMPH_ALGO) + hash_state_packed_size(h1_type) + hash_state_packed_size(h2_type) + 3 * sizeof(DWORD) + sizeof(DWORD) * data->n);
}

/** DWORD bmz_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search.
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD bmz_search_packed(void* packed_mphf,const char* key,DWORD keylen)
{
   BYTE*       h1_ptr   = (BYTE*) packed_mphf;

   CMPH_HASH   h1_type  = (CMPH_HASH) (*((DWORD*) h1_ptr));

   h1_ptr += 4;

   BYTE*       h2_ptr   = h1_ptr + hash_state_packed_size(h1_type);

   CMPH_HASH   h2_type  = (CMPH_HASH) (*((DWORD*) h2_ptr));

   h2_ptr += 4;

   DWORD*   g_ptr = (DWORD*) (h2_ptr + hash_state_packed_size(h2_type));
   DWORD    n     = *g_ptr++;
   DWORD    h1    = hash_packed(h1_ptr,h1_type,key,keylen) % n;
   DWORD    h2    = hash_packed(h2_ptr,h2_type,key,keylen) % n;

   if (h1 == h2 && ++h2 > n)
   {
      h2 = 0;
   }

   return g_ptr[h1] + g_ptr[h2];
}
