#include "stdafx.h"

#include "cmph_types.h"
#include "cmph.h"
#include "jenkins_hash.h"
#include "hash_state.h"
#include "bdz.h"
#include "cmph_structs.h"
#include "hash.h"
#include "bdz_structs.h"
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

#define UNASSIGNED         (3)
#define NULL_EDGE          (0xFFFFFFFF)

//DWORD ngrafos = 0;
//DWORD ngrafos_aciclicos = 0;
// table used for looking up the number of assigned vertices a 8-bit integer
const BYTE  bdz_lookup_table[] =
{
   4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 0
};

struct bdz_edge_t
{              
   DWORD       vertices[3];
   DWORD       next_edges[3];
};

static void bdz_alloc_queue(DWORD** queuep,DWORD nedges)
{
   (*queuep) = (DWORD*)malloc(nedges* sizeof(DWORD));
}

static void bdz_free_queue(DWORD** queue)
{
   free(*queue);
}

struct bdz_graph3_t
{
   DWORD          nedges;
   bdz_edge_t*    edges;
   DWORD*         first_edge;
   BYTE*          vert_degree;
};

static void bdz_alloc_graph3(bdz_graph3_t* graph3,DWORD nedges,DWORD nvertices)
{
   graph3->edges = (bdz_edge_t *) malloc(nedges * sizeof(bdz_edge_t));
   graph3->first_edge = (DWORD *) malloc(nvertices * sizeof(DWORD));
   graph3->vert_degree = (BYTE *) malloc((size_t) nvertices);
}

static void bdz_init_graph3(bdz_graph3_t* graph3,DWORD nedges,DWORD nvertices)
{
   memset(graph3->first_edge,0xff,nvertices * sizeof(DWORD));
   memset(graph3->vert_degree,0,(size_t) nvertices);
   graph3->nedges = 0;
}

static void bdz_free_graph3(bdz_graph3_t* graph3)
{
   free(graph3->edges);
   free(graph3->first_edge);
   free(graph3->vert_degree);
}

static void bdz_partial_free_graph3(bdz_graph3_t* graph3)
{
   free(graph3->first_edge);
   free(graph3->vert_degree);
   graph3->first_edge = NULL;
   graph3->vert_degree = NULL;
}

static void bdz_add_edge(bdz_graph3_t* graph3,DWORD v0,DWORD v1,DWORD v2)
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

static void bdz_dump_graph(bdz_graph3_t* graph3,DWORD nedges,DWORD nvertices)
{
   DWORD i;
 
   for (i = 0; i < nedges; i++)
   {
      printf("\nedge %d %d %d %d ",i,graph3->edges[i].vertices[0],graph3->edges[i].vertices[1],graph3->edges[i].vertices[2]);
      printf(" nexts %d %d %d",graph3->edges[i].next_edges[0],graph3->edges[i].next_edges[1],graph3->edges[i].next_edges[2]);
   };

   #ifdef DEBUG
   for (i = 0; i < nvertices; i++)
   {
      printf("\nfirst for vertice %d %d ",i,graph3->first_edge[i]);
   };
   #endif
}

static void bdz_remove_edge(bdz_graph3_t* graph3,DWORD curr_edge)
{
   DWORD i, j = 0, vert, edge1, edge2;
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
         bdz_dump_graph(graph3,graph3->nedges,graph3->nedges + graph3->nedges / 4);
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

static int bdz_generate_queue(DWORD nedges,DWORD nvertices,DWORD* queue,bdz_graph3_t* graph3)
{
   DWORD i, v0, v1, v2;
   DWORD queue_head = 0, queue_tail = 0;
   DWORD curr_edge;
   DWORD tmp_edge;
   BYTE* marked_edge = (BYTE*) malloc((size_t) (nedges >> 3) + 1);
 
   memset(marked_edge,0,(size_t) (nedges >> 3) + 1);

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

   TRACE("Queue head %d Queue tail %d\n",queue_head,queue_tail);
   
   #ifdef DEBUG
   bdz_dump_graph(graph3,graph3->nedges,graph3->nedges + graph3->nedges / 4);
   #endif
   
   while (queue_tail != queue_head)
   {
      curr_edge = queue[queue_tail++];
      bdz_remove_edge(graph3,curr_edge);
   
      TRACE("Removing edge %d\n",curr_edge);
   
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
   
   return (int) (queue_head - nedges);/* returns 0 if successful otherwies return negative number*/
}

static int     bdz_mapping(cmph_config_t* mph,bdz_graph3_t* graph3,DWORD* queue);
static void    assigning(bdz_config_data_t* bdz,bdz_graph3_t* graph3,DWORD* queue);
static void    ranking(bdz_config_data_t* bdz);
static DWORD   rank(DWORD b,DWORD* ranktable,BYTE* g,DWORD vertex);

bdz_config_data_t* bdz_config_new()
{
   bdz_config_data_t*   bdz;

   bdz = (bdz_config_data_t *) malloc(sizeof(bdz_config_data_t));
   
   if (!bdz)
   {
      return NULL;
   }

   memset(bdz,0,sizeof(bdz_config_data_t));
   
   bdz->hashfunc = CMPH_HASH_JENKINS;
   bdz->g = NULL;
   bdz->hl = NULL;
   bdz->k = 0; //kth index in ranktable, $k = log_2(n=3r)/\varepsilon$
   bdz->b = 7; // number of bits of k
   bdz->ranktablesize = 0; //number of entries in ranktable, $n/k +1$
   bdz->ranktable = NULL; // rank table
   
   return bdz;
}

void bdz_config_destroy(cmph_config_t* mph)
{
   bdz_config_data_t*   data  = (bdz_config_data_t*) mph->data;

   TRACE("Destroying algorithm dependent data\n");

   free(data);
}

void bdz_config_set_b(cmph_config_t* mph,DWORD b)
{
   bdz_config_data_t*   bdz = (bdz_config_data_t*) mph->data;

   if (b <= 2 || b > 10)
   {
      b = 7;
   } // validating restrictions over parameter b.

   bdz->b = (BYTE)b;

   TRACE("b: %u\n",b);
}

void bdz_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs)
{
   bdz_config_data_t*   bdz = (bdz_config_data_t*) mph->data;

   CMPH_HASH*           hashptr = hashfuncs;

   DWORD                i = 0;

   while (*hashptr != CMPH_HASH_COUNT)
   {
      if (i >= 1)
      {
         break;
      } //bdz only uses one linear hash function

      bdz->hashfunc = *hashptr;  
      
      ++i; 
      ++hashptr;
   }
}

cmph_t* bdz_new(cmph_config_t* mph,double c)
{
   cmph_t*              mphf = NULL;
   bdz_data_t*          bdzf = NULL;

   DWORD                iterations;
   
   DWORD*          edges;
   
   bdz_graph3_t         graph3;

   bdz_config_data_t*   bdz                     = (bdz_config_data_t*) mph->data;

   #ifdef CMPH_TIMING
   double   construction_time_begin = 0.0;
   double   construction_time       = 0.0;

   ELAPSED_TIME_IN_SECONDS(&construction_time_begin);
   #endif

   if (c == 0)
   {
      c = 1.23;
   } // validating restrictions over parameter c.

   TRACE("c: %f\n",c);

   bdz->m = mph->key_source->nkeys; 
   bdz->r = (DWORD) ceil((c * mph->key_source->nkeys) / 3);

   if ((bdz->r % 2) == 0)
   {
      bdz->r += 1;
   }
   
   bdz->n = 3 * bdz->r;

   bdz->k = (1U << bdz->b);

   TRACE("b: %u -- k: %u\n",bdz->b,bdz->k);

   bdz->ranktablesize = (DWORD) ceil(bdz->n / (double) bdz->k);

   TRACE("ranktablesize: %u\n",bdz->ranktablesize);

   bdz_alloc_graph3(&graph3,bdz->m,bdz->n);
   
   bdz_alloc_queue(&edges,bdz->m);

   TRACE("Created hypergraph\n");
   TRACE("m (edges): %u n (vertices): %u  r: %u c: %f \n",bdz->m,bdz->n,bdz->r,c);

   // Mapping step
   iterations = 1000;

   if (mph->verbosity)
   {
      fprintf(stderr,"Entering mapping step for mph creation of %u keys with graph sized %u\n",bdz->m,bdz->n);
   }

   while (true)
   {
      int   ok;

      TRACE("linear hash function \n");

      bdz->hl = hash_state_new(bdz->hashfunc,15);

      ok = bdz_mapping(mph,&graph3,edges);

      if (!ok)
      {
         --iterations;
         hash_state_destroy(bdz->hl);
         bdz->hl = NULL;

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

   if (!iterations)
   {
      bdz_free_queue(&edges);
      bdz_free_graph3(&graph3);

      return NULL;
   }

   bdz_partial_free_graph3(&graph3);

   // Assigning step
   if (mph->verbosity)
   {
      fprintf(stderr,"Entering assigning step for mph creation of %u keys with graph sized %u\n",bdz->m,bdz->n);
   }

   assigning(bdz,&graph3,edges);

   bdz_free_queue(&edges);
   bdz_free_graph3(&graph3);

   if (mph->verbosity)
   {
      fprintf(stderr,"Entering ranking step for mph creation of %u keys with graph sized %u\n",bdz->m,bdz->n);
   }

   ranking(bdz);

   #ifdef CMPH_TIMING
   ELAPSED_TIME_IN_SECONDS(&construction_time);
   #endif

   mphf = (cmph_t *) malloc(sizeof(cmph_t));
   mphf->algo = mph->algo;

   bdzf = (bdz_data_t *) malloc(sizeof(bdz_data_t));
   bdzf->g = bdz->g;
   
   bdz->g = NULL; //transfer memory ownership
   
   bdzf->hl = bdz->hl;
   
   bdz->hl = NULL; //transfer memory ownership
   
   bdzf->ranktable = bdz->ranktable;
   
   bdz->ranktable = NULL; //transfer memory ownership
   
   bdzf->ranktablesize = bdz->ranktablesize;
   bdzf->k = bdz->k;
   bdzf->b = bdz->b;
   bdzf->n = bdz->n;
   bdzf->m = bdz->m;
   bdzf->r = bdz->r;

   mphf->data = bdzf;
   mphf->size = bdz->m;

   TRACE("Successfully generated minimal perfect hash\n");

   if (mph->verbosity)
   {
      fprintf(stderr,"Successfully generated minimal perfect hash function\n");
   }

   #ifdef CMPH_TIMING
   DWORD space_usage       = bdz_packed_size(mphf) * 8;
   DWORD keys_per_bucket   = 1;
   
   construction_time = construction_time - construction_time_begin;
   
   fprintf(stdout,"%u\t%.2f\t%u\t%.4f\t%.4f\n",bdz->m,bdz->m / (double) bdz->n,keys_per_bucket,construction_time,space_usage / (double) bdz->m);
   #endif

   return mphf;
}

static int bdz_mapping(cmph_config_t* mph,bdz_graph3_t* graph3,DWORD* queue)
{
   DWORD                e;
   int                  cycles   = 0;
   DWORD                hl[3];

   bdz_config_data_t*   bdz = (bdz_config_data_t*) mph->data;
   
   bdz_init_graph3(graph3,bdz->m,bdz->n);

   mph->key_source->rewind(mph->key_source->data);

   for (e = 0; e < mph->key_source->nkeys; ++e)
   {
      DWORD h0, 
      h1, 
      h2;
      DWORD keylen;

      char* key = NULL;

      mph->key_source->read(mph->key_source->data,&key,&keylen);

      hash_vector(bdz->hl,key,keylen,hl);

      h0 = hl[0] % bdz->r;
      h1 = hl[1] % bdz->r + bdz->r;
      h2 = hl[2] % bdz->r + (bdz->r << 1);

      TRACE("Key: %.*s (%u %u %u)\n",keylen,key,h0,h1,h2);
      
      mph->key_source->dispose(mph->key_source->data,key,keylen);
      
      bdz_add_edge(graph3,h0,h1,h2);
   }

   cycles = bdz_generate_queue(bdz->m,bdz->n,queue,graph3);

   return (cycles == 0);
}

static void assigning(bdz_config_data_t* bdz,bdz_graph3_t* graph3,DWORD* queue)
{
   DWORD i;
   DWORD nedges = graph3->nedges;
   DWORD curr_edge;
   DWORD v0, 
   v1, 
   v2;

   BYTE*    marked_vertices = (BYTE*) malloc((size_t) (bdz->n >> 3) + 1);

   DWORD    sizeg = (DWORD) ceil(bdz->n / 4.0);

   bdz->g = (BYTE *) calloc((size_t) (sizeg),sizeof(BYTE));

   memset(marked_vertices,0,(size_t) (bdz->n >> 3) + 1);
   memset(bdz->g,0xff,(size_t) (sizeg));

   for (i = nedges - 1; i + 1 >= 1; i--)
   {
      curr_edge = queue[i];

      v0 = graph3->edges[curr_edge].vertices[0];
      v1 = graph3->edges[curr_edge].vertices[1];
      v2 = graph3->edges[curr_edge].vertices[2];

      TRACE("B:%u %u %u -- %u %u %u edge %u\n",v0,v1,v2,GETVALUE(bdz->g,v0),GETVALUE(bdz->g,v1),GETVALUE(bdz->g,v2),curr_edge);

      if (!GETBIT(marked_vertices,v0))
      {
         if (!GETBIT(marked_vertices,v1))
         {
            SETVALUE1(bdz->g,v1,UNASSIGNED);
            SETBIT(marked_vertices,v1);
         }

         if (!GETBIT(marked_vertices,v2))
         {
            SETVALUE1(bdz->g,v2,UNASSIGNED);
            SETBIT(marked_vertices,v2);
         }

         SETVALUE1(bdz->g,v0,(6 - (GETVALUE(bdz->g,v1) + GETVALUE(bdz->g,v2))) % 3);
         SETBIT(marked_vertices,v0);
      }
      else if (!GETBIT(marked_vertices,v1))
      {
         if (!GETBIT(marked_vertices,v2))
         {
            SETVALUE1(bdz->g,v2,UNASSIGNED);
            SETBIT(marked_vertices,v2);
         }

         SETVALUE1(bdz->g,v1,(7 - (GETVALUE(bdz->g,v0) + GETVALUE(bdz->g,v2))) % 3);
         SETBIT(marked_vertices,v1);
      }
      else
      {
         SETVALUE1(bdz->g,v2,(8 - (GETVALUE(bdz->g,v0) + GETVALUE(bdz->g,v1))) % 3);
         SETBIT(marked_vertices,v2);
      }     

      TRACE("A:%u %u %u -- %u %u %u\n",v0,v1,v2,GETVALUE(bdz->g,v0),GETVALUE(bdz->g,v1),GETVALUE(bdz->g,v2));
   }

   free(marked_vertices);
}

static void ranking(bdz_config_data_t* bdz)
{
   DWORD i, 
   j, 
   offset = 0U, 
   count = 0U, 
   size = (bdz->k >> 2U), 
   nbytes_total = (DWORD)ceil(bdz->n / 4.0), 
   nbytes;

   bdz->ranktable = (DWORD*)calloc((size_t) bdz->ranktablesize,sizeof(DWORD));

   // ranktable computation
   bdz->ranktable[0] = 0;  

   i = 1;

   while (true)
   {
      if (i == bdz->ranktablesize)
      {
         break;
      }

      nbytes = size < nbytes_total ? size : nbytes_total;

      for (j = 0; j < nbytes; j++)
      {
         count += bdz_lookup_table[*(bdz->g + offset + j)];
      }

      bdz->ranktable[i] = count;
      
      offset += nbytes;
      
      nbytes_total -= size;
      
      i++;
   }
}

int bdz_dump(cmph_t* mphf,FILE* fd)
{
   char*       buf = NULL;
   DWORD       buflen;
   size_t      nbytes;

   bdz_data_t* data = (bdz_data_t*) mphf->data;

   __cmph_dump(mphf,fd);

   hash_state_dump(data->hl,&buf,&buflen);

   TRACE("Dumping hash state with %u bytes to disk\n",buflen);

   nbytes = fwrite(&buflen,sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(buf,(size_t) buflen,(size_t) 1,fd);

   free(buf);

   nbytes = fwrite(&(data->n),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(&(data->m),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(&(data->r),sizeof(DWORD),(size_t) 1,fd);

   DWORD    sizeg = (DWORD) ceil(data->n / 4.0);

   nbytes = fwrite(data->g,sizeof(BYTE) * sizeg,(size_t) 1,fd);

   nbytes = fwrite(&(data->k),sizeof(DWORD),(size_t) 1,fd);
   nbytes = fwrite(&(data->b),sizeof(BYTE),(size_t) 1,fd);
   nbytes = fwrite(&(data->ranktablesize),sizeof(DWORD),(size_t) 1,fd);

   nbytes = fwrite(data->ranktable,sizeof(DWORD) * (data->ranktablesize),(size_t) 1,fd);

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

void bdz_load(FILE* f,cmph_t* mphf)
{
   char*       buf   = NULL;

   DWORD       buflen, sizeg;

   size_t      nbytes;

   bdz_data_t*    bdz = (bdz_data_t*) malloc(sizeof(bdz_data_t));

   TRACE("Loading bdz mphf\n");

   mphf->data = bdz;

   nbytes = fread(&buflen,sizeof(DWORD),(size_t) 1,f);

   TRACE("Hash state has %u bytes\n",buflen);

   buf = (char*)malloc((size_t)buflen);

   nbytes = fread(buf,(size_t) buflen,(size_t)1,f);

   bdz->hl = hash_state_load(buf,buflen);

   free(buf);

   TRACE("Reading m and n\n");

   nbytes = fread(&(bdz->n),sizeof(DWORD),(size_t) 1,f);
   nbytes = fread(&(bdz->m),sizeof(DWORD),(size_t) 1,f);
   nbytes = fread(&(bdz->r),sizeof(DWORD),(size_t) 1,f);

   sizeg = (DWORD) ceil(bdz->n / 4.0);

   bdz->g = (BYTE*)calloc((size_t)(sizeg),sizeof(BYTE));

   nbytes = fread(bdz->g,sizeg * sizeof(BYTE),(size_t) 1,f);

   nbytes = fread(&(bdz->k),sizeof(DWORD),(size_t)1,f);
   nbytes = fread(&(bdz->b),sizeof(BYTE),(size_t)1,f);
   nbytes = fread(&(bdz->ranktablesize),sizeof(DWORD),(size_t) 1,f);

   bdz->ranktable = (DWORD *) calloc((size_t) bdz->ranktablesize,sizeof(DWORD));

   nbytes = fread(bdz->ranktable,sizeof(DWORD) * (bdz->ranktablesize),(size_t) 1,f);

   #ifdef DEBUG
   DWORD i  = 0;
   
   fprintf(stderr,"G: ");
   
   for (i = 0; i < bdz->n; ++i)
   {
      fprintf(stderr,"%u ",GETVALUE(bdz->g,i));
   }
   
   fprintf(stderr,"\n");
   #endif
}

static inline DWORD rank(DWORD b,DWORD* ranktable,BYTE* g,DWORD vertex)
{
   DWORD    index     = vertex >> b;
   DWORD    base_rank = ranktable[index];
   DWORD    beg_idx_v = index << b;
   DWORD    beg_idx_b = beg_idx_v >> 2;
   DWORD    end_idx_b = vertex >> 2;

   while (beg_idx_b < end_idx_b)
   {
      base_rank += bdz_lookup_table[*(g + beg_idx_b++)];
   }

   TRACE("base rank %u\n",base_rank);

   beg_idx_v = beg_idx_b << 2;

   TRACE("beg_idx_v %u\n",beg_idx_v);

   while (beg_idx_v < vertex)
   {
      if (GETVALUE(g,beg_idx_v) != UNASSIGNED)
      {
         base_rank++;
      }
   
      beg_idx_v++;
   }

   return base_rank;
}

DWORD bdz_search(cmph_t* mphf,const char* key,DWORD keylen)
{
   DWORD       vertex;

   bdz_data_t* bdz   = (bdz_data_t*) mphf->data;

   DWORD       hl[3];

   hash_vector(bdz->hl,key,keylen,hl);

   hl[0] = hl[0] % bdz->r;
   hl[1] = hl[1] % bdz->r + bdz->r;
   hl[2] = hl[2] % bdz->r + (bdz->r << 1);

   vertex = hl[(GETVALUE(bdz->g,hl[0]) + GETVALUE(bdz->g,hl[1]) + GETVALUE(bdz->g,hl[2])) % 3];

   TRACE("Search found vertex %u\n",vertex);

   return rank(bdz->b,bdz->ranktable,bdz->g,vertex);
}

void bdz_destroy(cmph_t* mphf)
{
   bdz_data_t* data  = (bdz_data_t*) mphf->data;

   free(data->g);

   hash_state_destroy(data->hl);

   free(data->ranktable);
   free(data);
   free(mphf);
}

/** \fn void bdz_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the resulting mphf. The size of packed_mphf must be at least cmph_packed_size() 
 */
void bdz_pack(cmph_t* mphf,void* packed_mphf)
{
   bdz_data_t*    data = (bdz_data_t*) mphf->data;

   BYTE*    ptr = (BYTE*)packed_mphf;

   // packing hl type
   CMPH_HASH   hl_type = hash_get_type(data->hl);

   *((DWORD*) ptr) = hl_type;
   
   ptr += sizeof(DWORD);

   // packing hl
   hash_state_pack(data->hl,ptr);

   ptr += hash_state_packed_size(hl_type);

   // packing r
   *((DWORD*) ptr) = data->r;

   ptr += sizeof(data->r);

   // packing ranktablesize
   *((DWORD *) ptr) = data->ranktablesize;

   ptr += sizeof(data->ranktablesize);

   // packing ranktable
   memcpy(ptr,data->ranktable,sizeof(DWORD) * (data->ranktablesize));
   
   ptr += sizeof(DWORD) * (data->ranktablesize);

   // packing b
   *ptr++ = data->b;

   // packing g
   DWORD    sizeg = (DWORD)ceil(data->n / 4.0);
   
   memcpy(ptr,data->g,sizeof(BYTE) * sizeg);
}

/** \fn DWORD bdz_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */
DWORD bdz_packed_size(cmph_t* mphf)
{
   bdz_data_t*    data = (bdz_data_t*) mphf->data;

   CMPH_HASH   hl_type = hash_get_type(data->hl);

   return (DWORD) (sizeof(CMPH_ALGO) + hash_state_packed_size(hl_type) + 3 * sizeof(DWORD) + sizeof(DWORD) * (data->ranktablesize) + sizeof(BYTE) + sizeof(BYTE) * (DWORD) (ceil(data->n / 4.0)));
}

/** DWORD bdz_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search.
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD bdz_search_packed(void* packed_mphf,const char* key,DWORD keylen)
{
   DWORD       vertex;

   CMPH_HASH   hl_type = (CMPH_HASH) (*(DWORD*) packed_mphf);

   BYTE*       hl_ptr = (BYTE*) (packed_mphf) + 4;

   DWORD*      ranktable = (DWORD*) (hl_ptr + hash_state_packed_size(hl_type));

   DWORD       r = *ranktable++;

   DWORD       ranktablesize = *ranktable++;

   BYTE*       g = (BYTE*) (ranktable + ranktablesize);
   BYTE        b = *g++;

   DWORD       hl[3];

   hash_vector_packed(hl_ptr,hl_type,key,keylen,hl);

   hl[0] = hl[0] % r;
   hl[1] = hl[1] % r + r;
   hl[2] = hl[2] % r + (r << 1);

   vertex = hl[(GETVALUE(g,hl[0]) + GETVALUE(g,hl[1]) + GETVALUE(g,hl[2])) % 3];

   return rank(b,ranktable,g,vertex);
}
