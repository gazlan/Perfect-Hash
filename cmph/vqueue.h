#ifndef __CMPH_VQUEUE_H__
#define __CMPH_VQUEUE_H__

typedef struct __vqueue_t  vqueue_t;

vqueue_t*vqueue_new(DWORD capacity);
BYTE     vqueue_is_empty(vqueue_t* q);
void     vqueue_insert(vqueue_t* q,DWORD val);
DWORD    vqueue_remove(vqueue_t* q);
void     vqueue_print(vqueue_t* q);
void     vqueue_destroy(vqueue_t* q);

#endif
