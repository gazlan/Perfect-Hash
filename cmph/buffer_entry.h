#ifndef __CMPH_BUFFER_ENTRY_H__
#define __CMPH_BUFFER_ENTRY_H__

typedef struct __buffer_entry_t  buffer_entry_t;

buffer_entry_t*buffer_entry_new(DWORD capacity);
void           buffer_entry_set_capacity(buffer_entry_t* buffer_entry,DWORD capacity);
DWORD          buffer_entry_get_capacity(buffer_entry_t* buffer_entry);
void           buffer_entry_open(buffer_entry_t* buffer_entry,char* filename);
BYTE*          buffer_entry_read_key(buffer_entry_t* buffer_entry,DWORD* keylen);
void           buffer_entry_destroy(buffer_entry_t* buffer_entry);
#endif
