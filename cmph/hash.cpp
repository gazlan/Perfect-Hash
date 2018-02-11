#include "stdafx.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "cmph_types.h"
#include "jenkins_hash.h"
#include "hash_state.h"
#include "hash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]       = __FILE__;
#endif

char*       cmph_hash_names[] =
{
   "jenkins", NULL
};

hash_state_t* hash_state_new(CMPH_HASH hashfunc,DWORD hashsize)
{
   hash_state_t*  state = NULL;

   switch (hashfunc)
   {
      case CMPH_HASH_JENKINS:
         TRACE("Jenkins function - %u\n",hashsize);
         state = (hash_state_t *) jenkins_state_new(hashsize);
         TRACE("Jenkins function created\n");
         break;
      default:
         ASSERT(0);
   }

   state->hashfunc = hashfunc;
   return state;
}

DWORD hash(hash_state_t* state,const char* key,DWORD keylen)
{
   switch (state->hashfunc)
   {
      case CMPH_HASH_JENKINS:
         return jenkins_hash((jenkins_state_t *) state,key,keylen);
      default:
         ASSERT(0);
   }

   ASSERT(0);
   return 0;
}

void hash_vector(hash_state_t* state,const char* key,DWORD keylen,DWORD* hashes)
{
   switch (state->hashfunc)
   {
      case CMPH_HASH_JENKINS:
         jenkins_hash_vector_((jenkins_state_t *) state,key,keylen,hashes);
         break;
      default:
         ASSERT(0);
   }
}

void hash_state_dump(hash_state_t* state,char** buf,DWORD* buflen)
{
   char*    algobuf  = NULL;
   size_t   len;

   switch (state->hashfunc)
   {
      case CMPH_HASH_JENKINS:
         {
            jenkins_state_dump((jenkins_state_t *) state,&algobuf,buflen);

            if (*buflen == UINT_MAX)
            {
               goto cmph_cleanup;
            }

            break;
         }
      default:
         {
            ASSERT(0);
         }
   }

   *buf = (char *) malloc(strlen(cmph_hash_names[state->hashfunc]) + 1 + *buflen);

   memcpy(*buf,cmph_hash_names[state->hashfunc],strlen(cmph_hash_names[state->hashfunc]) + 1);

   TRACE("Algobuf is %u\n",*(DWORD *) algobuf);

   len = *buflen;

   memcpy(*buf + strlen(cmph_hash_names[state->hashfunc]) + 1,algobuf,len);

   *buflen = (DWORD) strlen(cmph_hash_names[state->hashfunc]) + 1 + *buflen;

   cmph_cleanup:

   free(algobuf);
   algobuf = NULL;
}

hash_state_t* hash_state_copy(hash_state_t* src_state)
{
   hash_state_t*  dest_state  = NULL;

   switch (src_state->hashfunc)
   {
      case CMPH_HASH_JENKINS:
         dest_state = (hash_state_t *) jenkins_state_copy((jenkins_state_t *) src_state);
         break;
      default:
         ASSERT(0);
   }

   dest_state->hashfunc = src_state->hashfunc;
   return dest_state;
}

hash_state_t* hash_state_load(const char* buf,DWORD buflen)
{
   DWORD       i;
   DWORD       offset;
   CMPH_HASH   hashfunc = CMPH_HASH_COUNT;

   for (i = 0; i < CMPH_HASH_COUNT; ++i)
   {
      if (strcmp(buf,cmph_hash_names[i]) == 0)
      {
         hashfunc = (CMPH_HASH) (i);
         break;
      }
   }

   if (hashfunc == CMPH_HASH_COUNT)
   {
      return NULL;
   }

   offset = (DWORD) strlen(cmph_hash_names[hashfunc]) + 1;

   switch (hashfunc)
   {
      case CMPH_HASH_JENKINS:
         return (hash_state_t *) jenkins_state_load(buf + offset,buflen - offset);
      default:
         return NULL;
   }
   return NULL;
}

void hash_state_destroy(hash_state_t* state)
{
   switch (state->hashfunc)
   {
      case CMPH_HASH_JENKINS:
         jenkins_state_destroy((jenkins_state_t *) state);
         break;
      default:
         ASSERT(0);
   }
}

/** \fn void hash_state_pack(hash_state_t *state, void *hash_packed)
 *  \brief Support the ability to pack a hash function into a preallocated contiguous memory space pointed by hash_packed.
 *  \param state points to the hash function
 *  \param hash_packed pointer to the contiguous memory area used to store the hash function. The size of hash_packed must be at least hash_state_packed_size()
 *
 * Support the ability to pack a hash function into a preallocated contiguous memory space pointed by hash_packed.
 * However, the hash function type must be packed outside.
 */
void hash_state_pack(hash_state_t* state,void* hash_packed)
{
   switch (state->hashfunc)
   {
      case CMPH_HASH_JENKINS:
         // pack the jenkins hash function
         jenkins_state_pack((jenkins_state_t *) state,hash_packed);
         break;
      default:
         ASSERT(0);
   }
}

/** \fn DWORD hash_state_packed_size(CMPH_HASH hashfunc)
 *  \brief Return the amount of space needed to pack a hash function.
 *  \param hashfunc function type
 *  \return the size of the packed function or zero for failures
 */
DWORD hash_state_packed_size(CMPH_HASH hashfunc)
{
   DWORD size  = 0;
   switch (hashfunc)
   {
      case CMPH_HASH_JENKINS:
         size += jenkins_state_packed_size();
         break;
      default:
         ASSERT(0);
   }

   return size;
}

/** \fn DWORD hash_packed(void *hash_packed, CMPH_HASH hashfunc, const char *k, DWORD keylen)
 *  \param hash_packed is a pointer to a contiguous memory area
 *  \param hashfunc is the type of the hash function packed in hash_packed
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
DWORD hash_packed(void* hash_packed,CMPH_HASH hashfunc,const char* k,DWORD keylen)
{
   switch (hashfunc)
   {
      case CMPH_HASH_JENKINS:
         return jenkins_hash_packed(hash_packed,k,keylen);
      default:
         ASSERT(0);
   }

   ASSERT(0);
   return 0;
}

/** \fn hash_vector_packed(void *hash_packed, CMPH_HASH hashfunc, const char *k, DWORD keylen, DWORD * hashes)
 *  \param hash_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void hash_vector_packed(void* hash_packed,CMPH_HASH hashfunc,const char* k,DWORD keylen,DWORD* hashes)
{
   switch (hashfunc)
   {
      case CMPH_HASH_JENKINS:
         jenkins_hash_vector_packed(hash_packed,k,keylen,hashes);
         break;
      default:
         ASSERT(0);
   }
}

/** \fn CMPH_HASH hash_get_type(hash_state_t *state);
 *  \param state is a pointer to a hash_state_t structure
 *  \return the hash function type pointed by state
 */
CMPH_HASH hash_get_type(hash_state_t* state)
{
   return state->hashfunc;
}
