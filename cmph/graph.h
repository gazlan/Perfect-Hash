#ifndef _CMPH_GRAPH_H__
#define _CMPH_GRAPH_H__

#define GRAPH_NO_NEIGHBOR UINT_MAX

typedef struct __graph_t            graph_t;
typedef struct __graph_iterator_t   graph_iterator_t;
struct __graph_iterator_t
{
   DWORD             vertex;
   DWORD             edge;
};

graph_t*          graph_new(DWORD nnodes,DWORD nedges);
void              graph_destroy(graph_t* graph);

void              graph_add_edge(graph_t* g,DWORD v1,DWORD v2);
void              graph_del_edge(graph_t* g,DWORD v1,DWORD v2);
void              graph_clear_edges(graph_t* g);
DWORD             graph_edge_id(graph_t* g,DWORD v1,DWORD v2);
BYTE              graph_contains_edge(graph_t* g,DWORD v1,DWORD v2);

graph_iterator_t  graph_neighbors_it(graph_t* g,DWORD v);
DWORD             graph_next_neighbor(graph_t* g,graph_iterator_t* it);

void              graph_obtain_critical_nodes(graph_t* g);            /* included -- Fabiano*/
BYTE              graph_node_is_critical(graph_t* g,DWORD v);     /* included -- Fabiano */
DWORD             graph_ncritical_nodes(graph_t* g);                /* included -- Fabiano*/
DWORD             graph_vertex_id(graph_t* g,DWORD e,DWORD id); /* included -- Fabiano*/

int               graph_is_cyclic(graph_t* g);

void              graph_print(graph_t*);

#endif
