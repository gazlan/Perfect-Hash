#ifndef __CMPH_VSTACK_H__
#define __CMPH_VSTACK_H__

typedef struct __vstack_t  vstack_t;

vstack_t*   vstack_new();

void        vstack_destroy(vstack_t* stack);
void        vstack_push(vstack_t* stack,DWORD val);
DWORD       vstack_top(vstack_t* stack);
void        vstack_pop(vstack_t* stack);
int         vstack_empty(vstack_t* stack);
DWORD       vstack_size(vstack_t* stack);
void        vstack_reserve(vstack_t* stack,DWORD size);

#endif
