#ifndef __CMPH_CHM_H__
#define __CMPH_CHM_H__

chm_config_data_t*chm_config_new();

void              chm_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs);
void              chm_config_destroy(cmph_config_t* mph);
cmph_t*           chm_new(cmph_config_t* mph,double c);
void              chm_load(FILE* f,cmph_t* mphf);
int               chm_dump(cmph_t* mphf,FILE* f);
void              chm_destroy(cmph_t* mphf);
DWORD             chm_search(cmph_t* mphf,const char* key,DWORD keylen);

/** \fn void chm_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the resulting mphf. The size of packed_mphf must be at least cmph_packed_size() 
 */
void              chm_pack(cmph_t* mphf,void* packed_mphf);

/** \fn DWORD chm_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */ 
DWORD             chm_packed_size(cmph_t* mphf);

/** DWORD chm_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search. 
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD             chm_search_packed(void* packed_mphf,const char* key,DWORD keylen);

#endif
