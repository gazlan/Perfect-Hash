#ifndef __CMPH_FCH_H__
#define __CMPH_FCH_H__

typedef struct __fch_data_t         fch_data_t;
typedef struct __fch_config_data_t  fch_config_data_t;

/* Parameters calculation */
DWORD                fch_calc_b(double c,DWORD m);
double               fch_calc_p1(DWORD m);
double               fch_calc_p2(DWORD b);
DWORD                mixh10h11h12(DWORD b,double p1,double p2,DWORD initial_index);

fch_config_data_t*   fch_config_new();
void                 fch_config_set_hashfuncs(cmph_config_t* mph,CMPH_HASH* hashfuncs);
void                 fch_config_destroy(cmph_config_t* mph);
cmph_t*              fch_new(cmph_config_t* mph,double c);

void                 fch_load(FILE* f,cmph_t* mphf);
int                  fch_dump(cmph_t* mphf,FILE* f);
void                 fch_destroy(cmph_t* mphf);
DWORD                fch_search(cmph_t* mphf,const char* key,DWORD keylen);

/** \fn void fch_pack(cmph_t *mphf, void *packed_mphf);
 *  \brief Support the ability to pack a perfect hash function into a preallocated contiguous memory space pointed by packed_mphf.
 *  \param mphf pointer to the resulting mphf
 *  \param packed_mphf pointer to the contiguous memory area used to store the resulting mphf. The size of packed_mphf must be at least cmph_packed_size() 
 */
void                 fch_pack(cmph_t* mphf,void* packed_mphf);

/** \fn DWORD fch_packed_size(cmph_t *mphf);
 *  \brief Return the amount of space needed to pack mphf.
 *  \param mphf pointer to a mphf
 *  \return the size of the packed function or zero for failures
 */ 
DWORD                fch_packed_size(cmph_t* mphf);

/** DWORD fch_search(void *packed_mphf, const char *key, DWORD keylen);
 *  \brief Use the packed mphf to do a search. 
 *  \param  packed_mphf pointer to the packed mphf
 *  \param key key to be hashed
 *  \param keylen key legth in bytes
 *  \return The mphf value
 */
DWORD                fch_search_packed(void* packed_mphf,const char* key,DWORD keylen);

#endif
