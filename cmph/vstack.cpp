#include "stdafx.h"

#include "vstack.h"
#include <stdlib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct __vstack_t
{
   DWORD pointer;
   DWORD*values;
   DWORD capacity;
};

vstack_t* vstack_new()
{
   vstack_t*   stack = (vstack_t*) malloc(sizeof(vstack_t));

   ASSERT(stack);

   stack->pointer = 0;
   stack->values = NULL;
   stack->capacity = 0;

   return stack;
}

void vstack_destroy(vstack_t* stack)
{
   ASSERT(stack);

   free(stack->values);
   free(stack);
}

void vstack_push(vstack_t* stack,DWORD val)
{
   ASSERT(stack);

   vstack_reserve(stack,stack->pointer + 1);
   stack->values[stack->pointer] = val;
   ++(stack->pointer);
}

void vstack_pop(vstack_t* stack)
{
   ASSERT(stack);
   ASSERT(stack->pointer > 0);

   --(stack->pointer);
}

DWORD vstack_top(vstack_t* stack)
{
   ASSERT(stack);
   ASSERT(stack->pointer > 0);
   return stack->values[(stack->pointer - 1)];
}

int vstack_empty(vstack_t* stack)
{
   ASSERT(stack);
   return stack->pointer == 0;
}

DWORD vstack_size(vstack_t* stack)
{
   return stack->pointer;
}

void vstack_reserve(vstack_t* stack,DWORD size)
{
   ASSERT(stack);

   if (stack->capacity < size)
   {
      DWORD new_capacity   = stack->capacity + 1;

      TRACE("Increasing current capacity %u to %u\n",stack->capacity,size);

      while (new_capacity < size)
      {
         new_capacity *= 2;
      }

      stack->values = (DWORD *) realloc(stack->values,sizeof(DWORD) * new_capacity);

      ASSERT(stack->values);

      stack->capacity = new_capacity;
      TRACE("Increased\n");
   }
}
