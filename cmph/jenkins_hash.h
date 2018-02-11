#ifndef __JEKINS_HASH_H__
#define __JEKINS_HASH_H__

struct jenkins_state_t
{
   CMPH_HASH         hashfunc;
   DWORD             seed;
};

jenkins_state_t*  jenkins_state_new(DWORD size); //size of hash table

/** \fn DWORD jenkins_hash(jenkins_state_t *state, const char *k, DWORD keylen);
 *  \param state is a pointer to a jenkins_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
DWORD             jenkins_hash(jenkins_state_t* state,const char* k,DWORD keylen);

/** \fn void jenkins_hash_vector_(jenkins_state_t *state, const char *k, DWORD keylen, DWORD * hashes);
 *  \param state is a pointer to a jenkins_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void              jenkins_hash_vector_(jenkins_state_t* state,const char* k,DWORD keylen,DWORD* hashes);

void              jenkins_state_dump(jenkins_state_t* state,char** buf,DWORD* buflen);
jenkins_state_t*  jenkins_state_copy(jenkins_state_t* src_state);
jenkins_state_t*  jenkins_state_load(const char* buf,DWORD buflen);
void              jenkins_state_destroy(jenkins_state_t* state);

/** \fn void jenkins_state_pack(jenkins_state_t *state, void *jenkins_packed);
 *  \brief Support the ability to pack a jenkins function into a preallocated contiguous memory space pointed by jenkins_packed.
 *  \param state points to the jenkins function
 *  \param jenkins_packed pointer to the contiguous memory area used to store the jenkins function. The size of jenkins_packed must be at least jenkins_state_packed_size() 
 */
void              jenkins_state_pack(jenkins_state_t* state,void* jenkins_packed);

/** \fn DWORD jenkins_state_packed_size();
 *  \brief Return the amount of space needed to pack a jenkins function.
 *  \return the size of the packed function or zero for failures
 */ 
DWORD             jenkins_state_packed_size(void);


/** \fn DWORD jenkins_hash_packed(void *jenkins_packed, const char *k, DWORD keylen);
 *  \param jenkins_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
DWORD             jenkins_hash_packed(void* jenkins_packed,const char* k,DWORD keylen);

/** \fn jenkins_hash_vector_packed(void *jenkins_packed, const char *k, DWORD keylen, DWORD * hashes);
 *  \param jenkins_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void              jenkins_hash_vector_packed(void* jenkins_packed,const char* k,DWORD keylen,DWORD* hashes);

#endif
