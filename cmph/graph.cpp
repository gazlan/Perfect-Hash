#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "vstack.h"
#include "bitbool.h"

#include <limits.h>
#include "cmph_types.h"
#include "graph.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define abs_edge(e,i) (e % g->nedges + i * g->nedges)

struct __graph_t
{
   DWORD          nnodes;
   DWORD          nedges;
   DWORD*         edges;
   DWORD*         first;
   DWORD*         next;
   BYTE*          critical_nodes;   /* included -- Fabiano*/
   DWORD          ncritical_nodes;   /* included -- Fabiano*/
   DWORD          cedges;
   int            shrinking;
};

static DWORD   EMPTY = UINT_MAX;

graph_t* graph_new(DWORD nnodes,DWORD nedges)
{
   graph_t* graph = (graph_t*) malloc(sizeof(graph_t));
   if (!graph)
   {
      return NULL;
   }

   graph->edges = (DWORD *) malloc(sizeof(DWORD) * 2 * nedges);
   graph->next = (DWORD *) malloc(sizeof(DWORD) * 2 * nedges);
   graph->first = (DWORD *) malloc(sizeof(DWORD) * nnodes);
   graph->critical_nodes = NULL; /* included -- Fabiano*/
   graph->ncritical_nodes = 0;   /* included -- Fabiano*/
   graph->nnodes = nnodes;
   graph->nedges = nedges;

   graph_clear_edges(graph);
   return graph;
}


void graph_destroy(graph_t* graph)
{
   TRACE("Destroying graph\n");

   free(graph->edges);
   free(graph->first);
   free(graph->next);
   free(graph->critical_nodes); /* included -- Fabiano*/
   free(graph);

   return;
}

void graph_print(graph_t* g)
{
   DWORD i, e;
   for (i = 0; i < g->nnodes; ++i)
   {
      TRACE("Printing edges connected to %u\n",i);

      e = g->first[i];

      if (e != EMPTY)
      {
         printf("%u -> %u\n",g->edges[abs_edge(e,0)],g->edges[abs_edge(e,1)]);

         while ((e = g->next[e]) != EMPTY)
         {
            printf("%u -> %u\n",g->edges[abs_edge(e,0)],g->edges[abs_edge(e,1)]);
         }
      }
   }
}

void graph_add_edge(graph_t* g,DWORD v1,DWORD v2)
{
   DWORD e  = g->cedges;

   ASSERT(v1 < g->nnodes);
   ASSERT(v2 < g->nnodes);
   ASSERT(e < g->nedges);
   ASSERT(!g->shrinking);

   g->next[e] = g->first[v1];
   g->first[v1] = e;
   g->edges[e] = v2;

   g->next[e + g->nedges] = g->first[v2];
   g->first[v2] = e + g->nedges;
   g->edges[e + g->nedges] = v1;

   ++(g->cedges);
}

static int check_edge(graph_t* g,DWORD e,DWORD v1,DWORD v2)
{
   TRACE("Checking edge %u %u looking for %u %u\n",g->edges[abs_edge(e,0)],g->edges[abs_edge(e,1)],v1,v2);

   if (g->edges[abs_edge(e,0)] == v1 && g->edges[abs_edge(e,1)] == v2)
   {
      return 1;
   }

   if (g->edges[abs_edge(e,0)] == v2 && g->edges[abs_edge(e,1)] == v1)
   {
      return 1;
   }

   return 0;
}

DWORD graph_edge_id(graph_t* g,DWORD v1,DWORD v2)
{
   DWORD e;

   e = g->first[v1];

   ASSERT(e != EMPTY);

   if (check_edge(g,e,v1,v2))
   {
      return abs_edge(e,0);
   }

   do
   {
      e = g->next[e];
      ASSERT(e != EMPTY);
   }
   while (!check_edge(g,e,v1,v2));

   return abs_edge(e,0);
}

static void del_edge_point(graph_t* g,DWORD v1,DWORD v2)
{
   DWORD e, prev;

   TRACE("Deleting edge point %u %u\n",v1,v2);

   e = g->first[v1];

   if (check_edge(g,e,v1,v2))
   {
      g->first[v1] = g->next[e];

      TRACE("Deleted\n");
      return;
   }

   TRACE("Checking linked list\n");

   do
   {
      prev = e;
      e = g->next[e];
      ASSERT(e != EMPTY);
   }
   while (!check_edge(g,e,v1,v2));

   g->next[prev] = g->next[e];
   TRACE("Deleted\n");
}

void graph_del_edge(graph_t* g,DWORD v1,DWORD v2)
{
   g->shrinking = 1;
   del_edge_point(g,v1,v2);
   del_edge_point(g,v2,v1);
}

void graph_clear_edges(graph_t* g)
{
   DWORD i;

   for (i = 0; i < g->nnodes; ++i)
   {
      g->first[i] = EMPTY;
   }

   for (i = 0; i < g->nedges*2; ++i)
   {
      g->edges[i] = EMPTY;
      g->next[i] = EMPTY;
   }

   g->cedges = 0;
   g->shrinking = 0;
}

static BYTE find_degree1_edge(graph_t* g,DWORD v,BYTE* deleted,DWORD* e)
{
   DWORD edge  = g->first[v];
   BYTE  found = 0;

   TRACE("Checking degree of vertex %u connected to edge %u\n",v,edge);

   if (edge == EMPTY)
   {
      return 0;
   }
   else if (!(GETBIT(deleted,abs_edge(edge,0))))
   {
      found = 1;
      *e = edge;
   }

   while (true)
   {
      edge = g->next[edge];

      if (edge == EMPTY)
      {
         break;
      }

      if (GETBIT(deleted,abs_edge(edge,0)))
      {
         continue;
      }

      if (found)
      {
         return 0;
      }

      TRACE("Found first edge\n");

      *e = edge;

      found = 1;
   }

   return found;
}

static void cyclic_del_edge(graph_t* g,DWORD v,BYTE* deleted)
{
   DWORD e  = 0;
   BYTE  degree1;
   DWORD v1 = v;
   DWORD v2 = 0;

   degree1 = find_degree1_edge(g,v1,deleted,&e);

   if (!degree1)
   {
      return;
   }

   while (true)
   {
      TRACE("Deleting edge %u (%u->%u)\n",e,g->edges[abs_edge(e,0)],g->edges[abs_edge(e,1)]);

      SETBIT(deleted,abs_edge(e,0));

      v2 = g->edges[abs_edge(e,0)];
      if (v2 == v1)
      {
         v2 = g->edges[abs_edge(e,1)];
      }

      TRACE("Checking if second endpoint %u has degree 1\n",v2);

      degree1 = find_degree1_edge(g,v2,deleted,&e);

      if (degree1)
      {
         TRACE("Inspecting vertex %u\n",v2);
         v1 = v2;
      }
      else
      {
         break;
      }
   }
}

int graph_is_cyclic(graph_t* g)
{
   DWORD    i;
   DWORD    v;

   BYTE*    deleted     = (BYTE*) malloc((g->nedges*sizeof(BYTE)) / 8 + 1);

   size_t   deleted_len = g->nedges / 8 + 1;

   memset(deleted,0,deleted_len);

   TRACE("Looking for cycles in graph with %u vertices and %u edges\n",g->nnodes,g->nedges);

   for (v = 0; v < g->nnodes; ++v)
   {
      cyclic_del_edge(g,v,deleted);
   }

   for (i = 0; i < g->nedges; ++i)
   {
      if (!(GETBIT(deleted,i)))
      {
         TRACE("Edge %u %u->%u was not deleted\n",i,g->edges[i],g->edges[i + g->nedges]);
         free(deleted);
         return 1;
      }
   }

   free(deleted);

   return 0;
}

BYTE graph_node_is_critical(graph_t* g,DWORD v) /* included -- Fabiano */
{
   return (BYTE) GETBIT(g->critical_nodes,v);
}

void graph_obtain_critical_nodes(graph_t* g) /* included -- Fabiano*/
{
   DWORD    i;
   DWORD    v;

   BYTE*    deleted     = (BYTE*) malloc((g->nedges*sizeof(BYTE)) / 8 + 1);

   size_t   deleted_len = g->nedges / 8 + 1;

   memset(deleted,0,deleted_len);

   free(g->critical_nodes);

   g->critical_nodes = (BYTE *) malloc((g->nnodes * sizeof(BYTE)) / 8 + 1);
   g->ncritical_nodes = 0;

   memset(g->critical_nodes,0,(g->nnodes * sizeof(BYTE)) / 8 + 1);

   TRACE("Looking for the 2-core in graph with %u vertices and %u edges\n",g->nnodes,g->nedges);

   for (v = 0; v < g->nnodes; ++v)
   {
      cyclic_del_edge(g,v,deleted);
   }

   for (i = 0; i < g->nedges; ++i)
   {
      if (!(GETBIT(deleted,i)))
      {
         TRACE("Edge %u %u->%u belongs to the 2-core\n",i,g->edges[i],g->edges[i + g->nedges]);

         if (!(GETBIT(g->critical_nodes,g->edges[i])))
         {
            g->ncritical_nodes ++;
            SETBIT(g->critical_nodes,g->edges[i]);
         }

         if (!(GETBIT(g->critical_nodes,g->edges[i + g->nedges])))
         {
            g->ncritical_nodes ++;
            SETBIT(g->critical_nodes,g->edges[i + g->nedges]);
         }
      }
   }

   free(deleted);
}

BYTE graph_contains_edge(graph_t* g,DWORD v1,DWORD v2) /* included -- Fabiano*/
{
   DWORD e;

   e = g->first[v1];

   if (e == EMPTY)
   {
      return 0;
   }
   if (check_edge(g,e,v1,v2))
   {
      return 1;
   }

   do
   {
      e = g->next[e];
      if (e == EMPTY)
      {
         return 0;
      }
   }
   while (!check_edge(g,e,v1,v2));

   return 1;
}

DWORD graph_vertex_id(graph_t* g,DWORD e,DWORD id) /* included -- Fabiano*/
{
   return (g->edges[e + id * g->nedges]);
}

DWORD graph_ncritical_nodes(graph_t* g) /* included -- Fabiano*/
{
   return g->ncritical_nodes;
}

graph_iterator_t graph_neighbors_it(graph_t* g,DWORD v)
{
   graph_iterator_t  it;
   it.vertex = v;
   it.edge = g->first[v];

   return it;
}
DWORD graph_next_neighbor(graph_t* g,graph_iterator_t* it)
{
   DWORD ret;


   if (it->edge == EMPTY)
   {
      return GRAPH_NO_NEIGHBOR;
   }

   if (g->edges[it->edge] == it->vertex)
   {
      ret = g->edges[it->edge + g->nedges];
   }
   else
   {
      ret = g->edges[it->edge];
   }

   it->edge = g->next[it->edge];

   return ret;
}
