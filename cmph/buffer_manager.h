#ifndef __CMPH_BUFFER_MANAGE_H__
#define __CMPH_BUFFER_MANAGE_H__

typedef struct __buffer_manager_t   buffer_manager_t;

buffer_manager_t* buffer_manager_new(DWORD memory_avail,DWORD nentries);
void              buffer_manager_open(buffer_manager_t* buffer_manager,DWORD index,char* filename);
BYTE*             buffer_manager_read_key(buffer_manager_t* buffer_manager,DWORD index,DWORD* keylen);
void              buffer_manager_destroy(buffer_manager_t* buffer_manager);
#endif
