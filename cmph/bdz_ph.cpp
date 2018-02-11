#include "stdafx.h"

#include "cmph_types.h"
#include "cmph.h"
#include "jenkins_hash.h"
#include "hash_state.h"
#include "bdz_ph.h"
#include "cmph_structs.h"
#include "hash.h"
#include "bdz_structs_ph.h"
#include "hash.h"
#include "bitbool.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UNASSIGNED      (3)
#define NULL_EDGE       (0xFFFFFFFF)

static BYTE pow3_table[5] = { 1, 3, 9, 27, 81 };

static BYTE lookup_table[5][256] =
{
   { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0}, {0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 
};

struct bdz_ph_edge_t
{
   DWORD          vertices[3];
   DWORD          next_edges[3];
};

struct bdz_ph_graph3_t
{
   DWORD             nedges;
   bdz_ph_edge_t*    edges;
   DWORD*            first_edge;
   BYTE*             vert_degree;
};

static int  bdz_ph_mapping(cmph_config_t* mph,bdz_ph_graph3_t* graph3,DWORD* queue);
static void assigning(bdz_ph_config_data_t* bdz_ph,bdz_ph_graph3_t* graph3,DWORD* queue);
static void bdz_ph_optimization(bdz_ph_config_data_t* bdz_ph);

static void bdz_ph_alloc_queue(DWORD** queuep,DWORD nedges)
{  
   *queuep = (DWORD*)malloc(nedges * sizeof(DWORD));
}

static void bdz_ph_free_queue(DWORD** queue)
{
   free(*queue);
}

static void bdz_ph_alloc_graph3(bdz_ph_graph3_t* graph3,DWORD nedges,DWORD nvertices)
{
   graph3->edges = (bdz_ph_edge_t *) malloc(nedges * sizeof(bdz_ph_edge_t));
   graph3->first_edge = (DWORD *) malloc(nvertices * sizeof(DWORD));
   graph3->vert_degree = (BYTE *) malloc((size_t) nvertices);
}

static void bdz_ph_init_graph3(bdz_ph_graph3_t* graph3,DWORD nedges,DWORD nvertices)
{
   memset(graph3->first_edge,0xff,nvertices * sizeof(DWORD));
   memset(graph3->vert_degree,0,(size_t) nvertices);
   graph3->nedges = 0;
}

static void bdz_ph_free_graph3(bdz_ph_graph3_t* graph3)
{
   free(graph3->edges);
   free(graph3->first_edge);
   free(graph3->vert_degree);
}

static void bdz_ph_partial_free_graph3(bdz_ph_graph3_t* graph3)
{
   free(graph3->first_edge);
   free(graph3->vert_degree);
   graph3->first_edge = NULL;
   graph3->vert_degree = NULL;
}

static void bdz_ph_add_edge(bdz_ph_graph3_t* graph3,DWORD v0,DWORD v1,DWORD v2)
{
   graph3->edges[graph3->nedges].vertices[0] = v0;
   graph3->edges[graph3->nedges].vertices[1] = v1;
   graph3->edges[graph3->nedges].vertices[2] = v2;

   graph3->edges[graph3->nedges].next_edges[0] = graph3->first_edge[v0];
   graph3->edges[graph3->nedges].next_edges[1] = graph3->first_edge[v1];
   graph3->edges[graph3->nedges].next_edges[2] = graph3->first_edge[v2];
 
   graph3->first_edge[v0] = graph3->first_edge[v1] = graph3->first_edge[v2] = graph3->nedges;
   
   graph3->vert_degree[v0]++;
   graph3->vert_degree[v1]++;
   graph3->vert_degree[v2]++;
   
   graph3->nedges++;
}

static void bdz_ph_dump_graph(bdz_ph_graph3_t* graph3,DWORD nedges,DWORD nvertices)
{
   for (DWORD ii = 0; ii < nedges; ++ii)
   {
      printf("\nedge %d %d %d %d ",ii,graph3->edges[ii].vertices[0],graph3->edges[ii].vertices[1],graph3->edges[ii].vertices[2]);
      printf(" nexts %d %d %d",graph3->edges[ii].next_edges[0],graph3->edges[ii].next_edges[1],graph3->edges[ii].next_edges[2]);
   }

   for (ii = 0; ii < nvertices; ++ii)
   {
      printf("\nfirst for vertice %d %d ",ii,graph3->first_edge[ii]);
   }
}

static void bdz_ph_remove_edge(bdz_ph_graph3_t* graph3,DWORD curr_edge)
{
   DWORD i, 
   j = 0, 
   vert, 
   edge1, 
   edge2;

   for (i = 0; i < 3; i++)
   {
      vert = graph3->edges[curr_edge].vertices[i];
      edge1 = graph3->first_edge[vert];
      edge2 = NULL_EDGE;

      while (edge1 != curr_edge && edge1 != NULL_EDGE)
      {
         edge2 = edge1;

         if (graph3->edges[edge1].vertices[0] == vert)
         {
            j = 0;
         }
         else if (graph3->edges[edge1].vertices[1] == vert)
         {
            j = 1;
         }
         else
         {
            j = 2;
         }

         edge1 = graph3->edges[edge1].next_edges[j];
      }

      if (edge1 == NULL_EDGE)
      {
         printf("\nerror remove edge %d dump graph",curr_edge);
         bdz_ph_dump_graph(graph3,graph3->nedges,graph3->nedges + graph3->nedges / 4);
         exit(-1);
      }

      if (edge2 != NULL_EDGE)
      {
         graph3->edges[edge2].next_edges[j] = graph3->edges[edge1].next_edges[i];
      }
      else
      {
         graph3->first_edge[vert] = graph3->edges[edge1].next_edges[i];
      }

      graph3->vert_degree[vert]--;
   }
}

static int bdz_ph_generate_queue(DWORD nedges,DWORD nvertices,DWORD* queue,bdz_ph_graph3_t* graph3)
{
   DWORD i, 
   v0, 
   v1, 
   v2;
   DWORD queue_head = 0, 
   queue_tail = 0;
   DWORD curr_edge;
   DWORD tmp_edge;

   BYTE*    marked_edge = (BYTE*)malloc((size_t) (nedges >> 3) + 1);

   memset(marked_edge,0,(size_t)(nedges >> 3) + 1);

   for (i = 0; i < nedges; i++)
   {
      v0 = graph3->edges[i].vertices[0];
      v1 = graph3->edges[i].vertices[1];
      v2 = graph3->edges[i].vertices[2];

      if (graph3->vert_degree[v0] == 1 || graph3->vert_degree[v1] == 1 || graph3->vert_degree[v2] == 1)
      {
         if (!GETBIT(marked_edge,i))
         {
            queue[queue_head++] = i;

            SETBIT(marked_edge,i);
         }
      }
   }

   while (queue_tail != queue_head)
   {
      curr_edge = queue[queue_tail++];

      bdz_ph_remove_edge(graph3,curr_edge);
      
      v0 = graph3->edges[curr_edge].vertices[0];
      v1 = graph3->edges[curr_edge].vertices[1];
      v2 = graph3->edges[curr_edge].vertices[2];

      if (graph3->vert_degree[v0] == 1)
      {
         tmp_edge = graph3->first_edge[v0];

         if (!GETBIT(marked_edge,tmp_edge))
         {
            queue[queue_head++] = tmp_edge;
            SETBIT(marked_edge,tmp_edge);
         }
      }

      if (graph3->vert_degree[v1] == 1)
      {
         tmp_edge = graph3->first_edge[v1];

         if (!GETBIT(marked_edge,tmp_edge))
         {
            queue[queue_head++] = tmp_edge;

            SETBIT(marked_edge,tmp_edge);
         }
      }

      if (graph3->vert_degree[v2] == 1)
      {
         tmp_edge = graph3->first_edge[v2];

         if (!GETBIT(marked_edge,tmp_edge))
         {
            queue[queue_head++] = tmp_edge;

            SETBIT(marked_edge,tmp_edge);
         }
      }
   }

   free(marked_edge);

   return (int) queue_head - (int) nedges;/* returns 0 if successful otherwies return negative number*/
}

bdz_ph_config_data_t* bdz_ph_config_new(void)
{
   bdz_ph_config_data_t*   bdz_ph;

   bdz_ph = (bdz_ph_config_data_t*)malloc(sizeof(bdz_ph_config_data_t));

   ASSERT(bdz_ph);
   
   memset(bdz_ph,0,sizeof(bdz_ph_config_data_t));
   
   bdz_ph->hashfunc = CMPH_HASH_JENKINS;
   bdz_ph->g        = NULL;
   bdz_ph->hl       = NULL;
   
   return bdz_ph;
}

void bdz_ph_config_destroy(cmph_config_t* mph)
{
   bdz_ph_config_data_t*   data = (bdz_ph_config_data_t*) mph->data;

   TRACE("Destroying algorithm dependent data\n");

   free(data);
}

void bdz_ph_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs)
{
   bdz_ph_config_data_t*   bdz_ph = (bdz_ph_config_data_t*) mph->data;

   CMPH_HASH*     hashptr = hashfuncs;
   
   DWORD i = 0;

   while (*hashptr != CMPH_HASH_COUNT)
   {
      if (i >= 1)
      {
         break;
      } //bdz_ph only uses one linear hash function

      bdz_ph->hashfunc = *hashptr;

      ++i; 
      ++hashptr;
   }
}

cmph_t* bdz_ph_new(cmph_config_t* mph,double c)
{
   cmph_t*                 mphf = NULL;
   
   bdz_ph_data_t*          bdz_phf = NULL;

   DWORD                   iterations;
   
   DWORD*          edges;
   bdz_ph_graph3_t         graph3;

   bdz_ph_config_data_t*   bdz_ph = (bdz_ph_config_data_t*) mph->data;

   #ifdef CMPH_TIMING
   double                  construction_time_begin = 0.0;
   double                  construction_time       = 0.0;

   ELAPSED_TIME_IN_SECONDS(&construction_time_begin);
   #endif

   if (c == 0)
   {
      c = 1.23;
   } // validating restrictions over parameter c.

   TRACE("c: %f\n",c);

   bdz_ph->m = mph->key_source->nkeys;
   bdz_ph->r = (DWORD) ceil((c * mph->key_source->nkeys) / 3);

   if ((bdz_ph->r % 2) == 0)
   {
      bdz_ph->r += 1;
   }

   bdz_ph->n = 3 * bdz_ph->r;

   bdz_ph_alloc_graph3(&graph3,bdz_ph->m,bdz_ph->n);
   bdz_ph_alloc_queue(&edges,bdz_ph->m);

   TRACE("Created hypergraph\n");
   TRACE("m (edges): %u n (vertices): %u  r: %u c: %f \n",bdz_ph->m,bdz_ph->n,bdz_ph->r,c);

   // Mapping step
   iterations = 100;

   if (mph->verbosity)
   {
      fprintf(stderr,"Entering mapping step for mph creation of %u keys with graph sized %u\n",bdz_ph->m,bdz_ph->n);
   }

   while (true)
   {
      int   ok;

      TRACE("linear hash function \n");

      bdz_ph->hl = hash_state_new(bdz_ph->hashfunc,15);

      ok = bdz_ph_mapping(mph,&graph3,edges);

      if (!ok)
      {
         --iterations;
         hash_state_destroy(bdz_ph->hl);
         bdz_ph->hl = NULL;

         TRACE("%u iterations remaining\n",iterations);

         if (mph->verbosity)
         {
            fprintf(stderr,"acyclic graph creation failure - %u iterations remaining\n",iterations);
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
      bdz_ph_free_queue(&edges);
      bdz_ph_free_graph3(&graph3);
      return NULL;
   }

   bdz_ph_partial_free_graph3(&graph3);

   // Assigning step
   if (mph->verbosity)
   {
      fprintf(stderr,"Entering assigning step for mph creation of %u keys with graph sized %u\n",bdz_ph->m,bdz_ph->n);
   }

   assigning(bdz_ph,&graph3,edges);

   bdz_ph_free_queue(&edges);
   bdz_ph_free_graph3(&graph3);

   if (mph->verbosity)
   {
      fprintf(stderr,"Starting optimization step\n");
   }

   bdz_ph_optimization(bdz_ph);

   #ifdef CMPH_TIMING
   ELAPSED_TIME_IN_SECONDS(&construction_time);
   #endif

   mphf = (cmph_t*) malloc(sizeof(cmph_t));

   mphf->algo = mph->algo;

   bdz_phf = (bdz_ph_data_t*)malloc(sizeof(bdz_ph_data_t));

   bdz_phf->g  = bdz_ph->g;
   bdz_ph->g   = NULL; //transfer memory ownership
   
   bdz_phf->hl = bdz_ph->hl;
   
   bdz_ph->hl  = NULL; //transfer memory ownership
   bdz_phf->n  = bdz_ph->n;
   bdz_phf->m  = bdz_ph->m;
   bdz_phf->r  = bdz_ph->r;

   mphf->data = bdz_phf;
   mphf->size = bdz_ph->n;

   TRACE("Successfully generated minimal perfect hash\n");

   if (mph->verbosity)
   {
      fprintf(stderr,"Successfully generated minimal perfect hash function\n");
   }

   #ifdef CMPH_TIMING
   DWORD    space_usage     = bdz_ph_packed_size(mphf) * 8;
   DWORD    keys_per_bucket = 1;

   construction_time = construction_time - construction_time_begin;

   fprintf(stdout,"%u\t%.2f\t%u\t%.4f\t%.4f\n",bdz_ph->m,bdz_ph->m / (double) bdz_ph->n,keys_per_bucket,construction_time,space_usage / (double) bdz_ph->m);
   #endif

   return mphf;
}

static int bdz_ph_mapping(cmph_config_t* mph,bdz_ph_graph3_t* graph3,DWORD* queue)
{              
   DWORD       e;
   
   int         cycles = 0;
   
   DWORD       hl[3];

   bdz_ph_config_data_t*   bdz_ph = (bdz_ph_config_data_t*) mph->data;

   bdz_ph_init_graph3(graph3,bdz_ph->m,bdz_ph->n);

   mph->key_source->rewind(mph->key_source->data);

   for (e = 0; e < mph->key_source->nkeys; ++e)
   {
      DWORD    keylen = 0;

      char*    key = NULL;

      mph->key_source->read(mph->key_source->data,&key,&keylen);

      hash_vector(bdz_ph->hl,key,keylen,hl);

      DWORD h0 = hl[0] % bdz_ph->r;
      DWORD h1 = hl[1] % bdz_ph->r + bdz_ph->r;
      DWORD h2 = hl[2] % bdz_ph->r + (bdz_ph->r << 1);

      mph->key_source->dispose(mph->key_source->data,key,keylen);

      bdz_ph_add_edge(graph3,h0,h1,h2);
   }

   cycles = bdz_ph_generate_queue(bdz_ph->m,bdz_ph->n,queue,graph3);

   return cycles == 0;
}

static void assigning(bdz_ph_config_data_t* bdz_ph,bdz_ph_graph3_t* graph3,DWORD* queue)
{
   DWORD    i;
   DWORD    nedges            = graph3->nedges;
   DWORD    curr_edge;
   DWORD    v0, 
            v1, 
            v2;

   BYTE*    marked_vertices = (BYTE*)malloc((size_t) (bdz_ph->n >> 3) + 1);

   DWORD    sizeg = (DWORD) ceil(bdz_ph->n / 4.0);

   bdz_ph->g = (BYTE*)calloc((size_t)sizeg,sizeof(BYTE));

   memset(marked_vertices,0,(size_t)(bdz_ph->n >> 3) + 1);

   for (i = nedges - 1; i + 1 >= 1; i--)
   {
      curr_edge = queue[i];

      v0 = graph3->edges[curr_edge].vertices[0];
      v1 = graph3->edges[curr_edge].vertices[1];
      v2 = graph3->edges[curr_edge].vertices[2];

      TRACE("B:%u %u %u -- %u %u %u\n",v0,v1,v2,GETVALUE(bdz_ph->g,v0),GETVALUE(bdz_ph->g,v1),GETVALUE(bdz_ph->g,v2));

      if (!GETBIT(marked_vertices,v0))
      {
         if (!GETBIT(marked_vertices,v1))
         {
            //SETVALUE(bdz_ph->g, v1, UNASSIGNED);
            SETBIT(marked_vertices,v1);
         }

         if (!GETBIT(marked_vertices,v2))
         {
            //SETVALUE(bdz_ph->g, v2, UNASSIGNED);
            SETBIT(marked_vertices,v2);
         }

         SETVALUE0(bdz_ph->g,v0,(6 - (GETVALUE(bdz_ph->g,v1) + GETVALUE(bdz_ph->g,v2))) % 3);
         SETBIT(marked_vertices,v0);
      }
      else if (!GETBIT(marked_vertices,v1))
      {
         if (!GETBIT(marked_vertices,v2))
         {
            //SETVALUE(bdz_ph->g, v2, UNASSIGNED);
            SETBIT(marked_vertices,v2);
         }

         SETVALUE0(bdz_ph->g,v1,(7 - (GETVALUE(bdz_ph->g,v0) + GETVALUE(bdz_ph->g,v2))) % 3);
         SETBIT(marked_vertices,v1);
      }
      else
      {
         SETVALUE0(bdz_ph->g,v2,(8 - (GETVALUE(bdz_ph->g,v0) + GETVALUE(bdz_ph->g,v1))) % 3);
         SETBIT(marked_vertices,v2);
      }

      TRACE("A:%u %u %u -- %u %u %u\n",v0,v1,v2,GETVALUE(bdz_ph->g,v0),GETVALUE(bdz_ph->g,v1),GETVALUE(bdz_ph->g,v2));
   }

   free(marked_vertices);
}

static void bdz_ph_optimization(bdz_ph_config_data_t* bdz_ph)
{
   DWORD    i;
   BYTE     byte  = 0;
   DWORD    sizeg = (DWORD) ceil(bdz_ph->n / 5.0);
   BYTE*    new_g = (BYTE*) calloc((size_t) sizeg,sizeof(BYTE));
   BYTE     value;
   DWORD    idx;

   for (i = 0; i < bdz_ph->n; i++)
   {
      idx = i / 5;

      byte = new_g[idx];
      
      value = GETVALUE(bdz_ph->g,i);
      
      byte = (BYTE) (byte + value * pow3_table[i % 5U]);
      
      new_g[idx] = byte;
   }

   free(bdz_ph->g);

   bdz_ph->g = new_g;
}

int bdz_ph_dump(cmph_t* mphf,FILE* fd)
{
   char*          buf   = NULL;
   DWORD          buflen;
   DWORD          sizeg = 0;
   size_t         nbytes;

   bdz_ph_data_t* data  = (bdz_ph_data_t*) mphf->data;

   __cmph_dump(mphf,fd);

   hash_state_dump(data->hl,&buf,&buflen);

   TRACE("Dumping hash state with %u bytes to disk\n",buflen);

   nbytes = fwrite(&buflen,sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(buf,(size_t) buflen,(size_t) 1,fd);

   free(buf);

   nbytes = fwrite(&(data->n),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(&(data->m),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(&(data->r),sizeof(DWORD),(size_t) 1,fd);

   sizeg = (DWORD) ceil(data->n / 5.0);

   nbytes = fwrite(data->g,sizeof(BYTE) * sizeg,(size_t) 1,fd);

   #ifdef DEBUG
   DWORD i;

   fprintf(stderr,"G: ");
   
   for (i = 0; i < data->n; ++i)
   {
      fprintf(stderr,"%u ",GETVALUE(data->g,i));
   }
   
   fprintf(stderr,"\n");
   #endif

   return 1;
}

void bdz_ph_load(FILE* f,cmph_t* mphf)
{
   char*          buf      = NULL;
   DWORD          buflen;
   DWORD          sizeg    = 0;
   size_t         nbytes;

   bdz_ph_data_t*    bdz_ph = (bdz_ph_data_t*) malloc(sizeof(bdz_ph_data_t));

   TRACE("Loading bdz_ph mphf\n");

   mphf->data = bdz_ph;

   nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,f);

   TRACE("Hash state has %u bytes\n",buflen);

   buf = (char *) malloc((size_t) buflen);

   nbytes = fread(buf,(size_t) buflen,(size_t) 1,f);

   bdz_ph->hl = hash_state_load(buf,buflen);

   free(buf);

   TRACE("Reading m and n\n");

   nbytes = fread(&(bdz_ph->n),sizeof(DWORD),(size_t) 1,f);
   nbytes = fread(&(bdz_ph->m),sizeof(DWORD),(size_t) 1,f);
   nbytes = fread(&(bdz_ph->r),sizeof(DWORD),(size_t) 1,f);

   sizeg = (DWORD) ceil(bdz_ph->n / 5.0);

   bdz_ph->g = (BYTE *) calloc((size_t) sizeg,sizeof(BYTE));

   nbytes = fread(bdz_ph->g,sizeg * sizeof(BYTE),(size_t) 1,f);
}

DWORD bdz_ph_search(cmph_t* mphf,const char* key,DWORD keylen)
{
   bdz_ph_data_t*    bdz_ph = (bdz_ph_data_t*) mphf->data;

   DWORD          hl[3];
   BYTE           byte0, 
   byte1, 
   byte2;
   DWORD          vertex;

   hash_vector(bdz_ph->hl,key,keylen,hl);

   hl[0] = hl[0] % bdz_ph->r;
   hl[1] = hl[1] % bdz_ph->r + bdz_ph->r;
   hl[2] = hl[2] % bdz_ph->r + (bdz_ph->r << 1);

   byte0 = bdz_ph->g[hl[0] / 5];
   byte1 = bdz_ph->g[hl[1] / 5];
   byte2 = bdz_ph->g[hl[2] / 5];

   byte0 = lookup_table[hl[0] % 5U][byte0];
   byte1 = lookup_table[hl[1] % 5U][byte1];
   byte2 = lookup_table[hl[2] % 5U][byte2];

   vertex = hl[(byte0 + byte1 + byte2) % 3];

   return vertex;
}

void bdz_ph_destroy(cmph_t* mphf)
{
   bdz_ph_data_t* data  = (bdz_ph_data_t*) mphf->data;

   free(data->g);

   hash_state_destroy(data->hl);

   free(data);
   free(mphf);
}

/** \fn void bdz_ph_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the resulting mphf. The size of packed_mphf must be at least cmph_packed_size() 
 */
void bdz_ph_pack(cmph_t* mphf,void* packed_mphf)
{
   bdz_ph_data_t*    data = (bdz_ph_data_t*)mphf->data;
   BYTE*             ptr  = (BYTE*)packed_mphf;

   // packing hl type
   CMPH_HASH      hl_type  = hash_get_type(data->hl);

   *((DWORD *) ptr) = hl_type;
   
   ptr += sizeof(DWORD);

   // packing hl
   hash_state_pack(data->hl,ptr);
   
   ptr += hash_state_packed_size(hl_type);

   // packing r
   *((DWORD *) ptr) = data->r;
   
   ptr += sizeof(data->r);

   // packing g
   DWORD    sizeg = (DWORD)ceil(data->n / 5.0);
   
   memcpy(ptr,data->g,sizeof(BYTE) * sizeg);
}

/** \fn DWORD bdz_ph_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */
DWORD bdz_ph_packed_size(cmph_t* mphf)
{
   bdz_ph_data_t*    data = (bdz_ph_data_t*) mphf->data;

   CMPH_HASH   hl_type = hash_get_type(data->hl);

   DWORD    sizeg = (DWORD) ceil(data->n / 5.0);
   
   return (DWORD)(sizeof(CMPH_ALGO) + hash_state_packed_size(hl_type) + 2 * sizeof(DWORD) + sizeof(BYTE) * sizeg);
}

/** DWORD bdz_ph_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search.
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD bdz_ph_search_packed(void* packed_mphf,const char* key,DWORD keylen)
{
   CMPH_HASH   hl_type  = (CMPH_HASH) * (DWORD*) packed_mphf;

   BYTE*    hl_ptr = (BYTE*)(packed_mphf) + 4;

   BYTE*    ptr = hl_ptr + hash_state_packed_size(hl_type);

   DWORD       r = *(DWORD*)ptr;

   BYTE*       g = ptr + 4;

   DWORD       hl[3];

   BYTE        byte0, 
               byte1, 
               byte2;

   DWORD       vertex;

   hash_vector_packed(hl_ptr,hl_type,key,keylen,hl);

   hl[0] = hl[0] % r;
   hl[1] = hl[1] % r + r;
   hl[2] = hl[2] % r + (r << 1);

   byte0 = g[hl[0] / 5];
   byte1 = g[hl[1] / 5];
   byte2 = g[hl[2] / 5];

   byte0 = lookup_table[hl[0] % 5][byte0];
   byte1 = lookup_table[hl[1] % 5][byte1];
   byte2 = lookup_table[hl[2] % 5][byte2];

   vertex = hl[(byte0 + byte1 + byte2) % 3];

   return vertex;
}
