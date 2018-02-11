#ifndef __CMPH_H__
#define __CMPH_H__

typedef struct __config_t  cmph_config_t;
typedef struct __cmph_t    cmph_t;

struct cmph_io_adapter_t
{
   void*                data;
   DWORD                nkeys;
   int (*read)(void*,char**,DWORD*);
   void (*dispose)(void*,char*,DWORD);
   void (*rewind)(void*);
};

/** Adapter pattern API **/
/* please call free() in the created adapters */
cmph_io_adapter_t*   cmph_io_nlfile_adapter(FILE* keys_fd);
void                 cmph_io_nlfile_adapter_destroy(cmph_io_adapter_t* key_source);

cmph_io_adapter_t*   cmph_io_nlnkfile_adapter(FILE* keys_fd,DWORD nkeys);
void                 cmph_io_nlnkfile_adapter_destroy(cmph_io_adapter_t* key_source);

cmph_io_adapter_t*   cmph_io_vector_adapter(char** vector,DWORD nkeys);
void                 cmph_io_vector_adapter_destroy(cmph_io_adapter_t* key_source);

cmph_io_adapter_t*   cmph_io_byte_vector_adapter(BYTE** vector,DWORD nkeys);
void                 cmph_io_byte_vector_adapter_destroy(cmph_io_adapter_t* key_source);

cmph_io_adapter_t*   cmph_io_struct_vector_adapter(void* vector,DWORD struct_size,DWORD key_offset,DWORD key_len,DWORD nkeys);

void                 cmph_io_struct_vector_adapter_destroy(cmph_io_adapter_t* key_source);

/** Hash configuration API **/
cmph_config_t*       cmph_config_new(cmph_io_adapter_t* key_source);
void                 cmph_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs);
void                 cmph_config_set_verbosity(cmph_config_t* mph,DWORD verbosity);
void                 cmph_config_set_graphsize(cmph_config_t* mph,double c);
void                 cmph_config_set_algo(cmph_config_t* mph,CMPH_ALGO algo);
void                 cmph_config_set_tmp_dir(cmph_config_t* mph,BYTE* tmp_dir);
void                 cmph_config_set_mphf_fd(cmph_config_t* mph,FILE* mphf_fd);
void                 cmph_config_set_b(cmph_config_t* mph,DWORD b);
void                 cmph_config_set_keys_per_bin(cmph_config_t* mph,DWORD keys_per_bin);
void                 cmph_config_set_memory_availability(cmph_config_t* mph,DWORD memory_availability);
void                 cmph_config_destroy(cmph_config_t* mph);

/** Hash API **/
cmph_t*              cmph_new(cmph_config_t* mph);

/** DWORD cmph_search(cmph_t *mphf, const char *key, DWORD keylen);
 *  \brief Computes the mphf value. 
 *  \param mphf pointer to the resulting function
 *  \param key is the key to be hashed
 *  \param keylen is the key legth in bytes
 *  \return The mphf value
 */
DWORD                cmph_search(cmph_t* mphf,const char* key,DWORD keylen);

DWORD                cmph_size(cmph_t* mphf);
void                 cmph_destroy(cmph_t* mphf);

/** Hash serialization/deserialization */
int                  cmph_dump(cmph_t* mphf,FILE* f);
cmph_t*              cmph_load(FILE* f);

/** \fn void cmph_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the 
 *  \param resulting mphf. The size of packed_mphf must be at least cmph_packed_size() 
 */
void                 cmph_pack(cmph_t* mphf,void* packed_mphf);

/** \fn DWORD cmph_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */
DWORD                cmph_packed_size(cmph_t* mphf);

/** DWORD cmph_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search. 
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD                cmph_search_packed(void* packed_mphf,const char* key,DWORD keylen);

#endif
