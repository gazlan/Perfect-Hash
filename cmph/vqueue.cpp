#include "stdafx.h"

#include "vqueue.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct __vqueue_t
{
   DWORD*values;
   DWORD beg, end, capacity;
};

vqueue_t* vqueue_new(DWORD capacity)
{
   size_t      capacity_plus_one = capacity + 1;

   vqueue_t*   q                 = (vqueue_t*) malloc(sizeof(vqueue_t));

   if (!q)
   {
      return NULL;
   }

   q->values = (DWORD *) calloc(capacity_plus_one,sizeof(DWORD));
   q->beg = q->end = 0;
   q->capacity = (DWORD) capacity_plus_one;

   return q;
}

BYTE vqueue_is_empty(vqueue_t* q)
{
   return (BYTE) (q->beg == q->end);
}

void vqueue_insert(vqueue_t* q,DWORD val)
{
   ASSERT((q->end + 1) % q->capacity != q->beg); // Is queue full?

   q->end = (q->end + 1) % q->capacity;
   q->values[q->end] = val;
}

DWORD vqueue_remove(vqueue_t* q)
{
   ASSERT(!vqueue_is_empty(q)); // Is queue empty?

   q->beg = (q->beg + 1) % q->capacity;

   return q->values[q->beg];
}

void vqueue_print(vqueue_t* q)
{
   DWORD i;

   for (i = q->beg; i != q->end; i = (i + 1) % q->capacity)
   {
      fprintf(stderr,"%u\n",q->values[(i + 1) % q->capacity]);
   }
}

void vqueue_destroy(vqueue_t* q)
{
   free(q->values); 
   q->values = NULL; 

   free(q);
}
