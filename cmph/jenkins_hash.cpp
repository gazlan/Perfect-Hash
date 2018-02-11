#include "stdafx.h"

#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>

#include "cmph_types.h"
#include "jenkins_hash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define hashsize(n)  ((DWORD)1 << (n))
#define hashmask(n)  (hashsize(n) - 1)

//#define NM2 /* Define this if you do not want power of 2 table sizes*/

/*
   --------------------------------------------------------------------
   mix -- mix 3 32-bit values reversibly.
   For every delta with one or two bits set, and the deltas of all three
   high bits or all three low bits, whether the original value of a,b,c
   is almost all zero or is uniformly distributed,
 * If mix() is run forward or backward, at least 32 bits in a,b,c
 have at least 1/4 probability of changing.
 * If mix() is run forward, every bit of c will change between 1/3 and
 2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
 mix() was built out of 36 single-cycle latency instructions in a
 structure that could supported 2x parallelism, like so:
 a -= b;
 a -= c; x = (c>>13);
 b -= c; a ^= x;
 b -= a; x = (a<<8);
 c -= a; b ^= x;
 c -= b; x = (b>>13);
 ...
 Unfortunately, superscalar Pentiums and Sparcs can't take advantage
 of that parallelism.  They've also turned some of those single-cycle
 latency instructions into multi-cycle latency instructions.  Still,
 this is the fastest good hash I could find.  There were about 2^^68
 to choose from.  I only looked at a billion or so.
 --------------------------------------------------------------------
 */
#define mix(a,b,c) \
{ \
   a -= b; a -= c; a ^= (c>>13); \
   b -= c; b -= a; b ^= (a<<8); \
   c -= a; c -= b; c ^= (b>>13); \
   a -= b; a -= c; a ^= (c>>12);  \
   b -= c; b -= a; b ^= (a<<16); \
   c -= a; c -= b; c ^= (b>>5); \
   a -= b; a -= c; a ^= (c>>3);  \
   b -= c; b -= a; b ^= (a<<10); \
   c -= a; c -= b; c ^= (b>>15); \
}

/*
   --------------------------------------------------------------------
   hash() -- hash a variable-length key into a 32-bit value
k       : the key (the unaligned variable-length array of bytes)
len     : the length of the key, counting by bytes
initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 6*len+35 instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (BYTE**)k, do it like this:
for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------
 */
jenkins_state_t* jenkins_state_new(DWORD size) //size of hash table
{
   jenkins_state_t*  state = (jenkins_state_t*) malloc(sizeof(jenkins_state_t));

   if (!state)
   {
      return NULL;
   }

   TRACE("Initializing jenkins hash\n");

   state->seed = ((DWORD) rand() % size);

   return state;
}
void jenkins_state_destroy(jenkins_state_t* state)
{
   free(state);
}

static inline void __jenkins_hash_vector(DWORD seed,const char* k,DWORD keylen,DWORD* hashes)
{
   DWORD len, length;

   /* Set up the internal state */
   length = keylen;
   len = length;

   hashes[0] = hashes[1] = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   hashes[2] = seed;   /* the previous hash value - seed in our case */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      hashes[0] += ((DWORD) k[0] + ((DWORD) k[1] << 8) + ((DWORD) k[2] << 16) + ((DWORD) k[3] << 24));
      hashes[1] += ((DWORD) k[4] + ((DWORD) k[5] << 8) + ((DWORD) k[6] << 16) + ((DWORD) k[7] << 24));
      hashes[2] += ((DWORD) k[8] + ((DWORD) k[9] << 8) + ((DWORD) k[10] << 16) + ((DWORD) k[11] << 24));

      mix(hashes[0],hashes[1],hashes[2]);

      k += 12; 
      len -= 12;
   }

   /*------------------------------------- handle the last 11 bytes */
   hashes[2] += length;

   switch (len)              /* all the case statements fall through */
   {
      case 11:
         hashes[2] += ((DWORD) k[10] << 24);
      case 10:
         hashes[2] += ((DWORD) k[9] << 16);
      case 9 :
         hashes[2] += ((DWORD) k[8] << 8);
         /* the first byte of hashes[2] is reserved for the length */
      case 8 :
         hashes[1] += ((DWORD) k[7] << 24);
      case 7 :
         hashes[1] += ((DWORD) k[6] << 16);
      case 6 :
         hashes[1] += ((DWORD) k[5] << 8);
      case 5 :
         hashes[1] += (BYTE) k[4];
      case 4 :
         hashes[0] += ((DWORD) k[3] << 24);
      case 3 :
         hashes[0] += ((DWORD) k[2] << 16);
      case 2 :
         hashes[0] += ((DWORD) k[1] << 8);
      case 1 :
         hashes[0] += (BYTE) k[0];
         /* case 0: nothing left to add */
   }

   mix(hashes[0],hashes[1],hashes[2]);
}

DWORD jenkins_hash(jenkins_state_t* state,const char* k,DWORD keylen)
{
   DWORD hashes[3];

   __jenkins_hash_vector(state->seed,k,keylen,hashes);

   return hashes[2];

   /* DWORD a, b, c;
      DWORD len, length;

      // Set up the internal state
      length = keylen;
      len = length;
      a = b = 0x9e3779b9;  // the golden ratio; an arbitrary value
      c = state->seed;   // the previous hash value - seed in our case

      // handle most of the key
      while (len >= 12)
      {
         a += (k[0] +((DWORD)k[1]<<8) +((DWORD)k[2]<<16) +((DWORD)k[3]<<24));
         b += (k[4] +((DWORD)k[5]<<8) +((DWORD)k[6]<<16) +((DWORD)k[7]<<24));
         c += (k[8] +((DWORD)k[9]<<8) +((DWORD)k[10]<<16)+((DWORD)k[11]<<24));
         mix(a,b,c);
         k += 12; len -= 12;
      }

      // handle the last 11 bytes
      c  += length;
      switch(len)              /// all the case statements fall through
      {
         case 11:
            c +=((DWORD)k[10]<<24);
         case 10:
            c +=((DWORD)k[9]<<16);
         case 9 :
            c +=((DWORD)k[8]<<8);
            // the first byte of c is reserved for the length
         case 8 :
            b +=((DWORD)k[7]<<24);
         case 7 :
            b +=((DWORD)k[6]<<16);
         case 6 :
            b +=((DWORD)k[5]<<8);
         case 5 :
            b +=k[4];
         case 4 :
            a +=((DWORD)k[3]<<24);
         case 3 :
            a +=((DWORD)k[2]<<16);
         case 2 :
            a +=((DWORD)k[1]<<8);
         case 1 :
            a +=k[0];
         // case 0: nothing left to add
      }

      mix(a,b,c);

      /// report the result

      return c;
      */
}

void jenkins_hash_vector_(jenkins_state_t* state,const char* k,DWORD keylen,DWORD* hashes)
{
   __jenkins_hash_vector(state->seed,k,keylen,hashes);
}

void jenkins_state_dump(jenkins_state_t* state,char** buf,DWORD* buflen)
{
   *buflen = sizeof(DWORD);

   *buf = (char *) malloc(sizeof(DWORD));

   if (!*buf)
   {
      *buflen = UINT_MAX;
      return;
   }

   memcpy(*buf,&(state->seed),sizeof(DWORD));

   TRACE("Dumped jenkins state with seed %u\n",state->seed);
}

jenkins_state_t* jenkins_state_copy(jenkins_state_t* src_state)
{
   jenkins_state_t*  dest_state  = (jenkins_state_t*) malloc(sizeof(jenkins_state_t));
   dest_state->hashfunc = src_state->hashfunc;
   dest_state->seed = src_state->seed;
   return dest_state;
}

jenkins_state_t* jenkins_state_load(const char* buf,DWORD buflen)
{
   jenkins_state_t*  state = (jenkins_state_t*) malloc(sizeof(jenkins_state_t));

   state->seed = *(DWORD *) buf;
   state->hashfunc = CMPH_HASH_JENKINS;

   TRACE("Loaded jenkins state with seed %u\n",state->seed);

   return state;
}

/** \fn void jenkins_state_pack(jenkins_state_t *state, void *jenkins_packed);
 *  \brief Support the ability to pack a jenkins function into a preallocated contiguous memory space pointed by jenkins_packed.
 *  \param state points to the jenkins function
 *  \param jenkins_packed pointer to the contiguous memory area used to store the jenkins function. The size of jenkins_packed must be at least jenkins_state_packed_size()
 */
void jenkins_state_pack(jenkins_state_t* state,void* jenkins_packed)
{
   if (state && jenkins_packed)
   {
      memcpy(jenkins_packed,&(state->seed),sizeof(DWORD));
   }
}

/** \fn DWORD jenkins_state_packed_size(jenkins_state_t *state);
 *  \brief Return the amount of space needed to pack a jenkins function.
 *  \return the size of the packed function or zero for failures
 */
DWORD jenkins_state_packed_size(void)
{
   return sizeof(DWORD);
}

/** \fn DWORD jenkins_hash_packed(void *jenkins_packed, const char *k, DWORD keylen);
 *  \param jenkins_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
DWORD jenkins_hash_packed(void* jenkins_packed,const char* k,DWORD keylen)
{
   DWORD hashes[3];

   __jenkins_hash_vector(*((DWORD *) jenkins_packed),k,keylen,hashes);

   return hashes[2];
}

/** \fn jenkins_hash_vector_packed(void *jenkins_packed, const char *k, DWORD keylen, DWORD * hashes);
 *  \param jenkins_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void jenkins_hash_vector_packed(void* jenkins_packed,const char* k,DWORD keylen,DWORD* hashes)
{
   __jenkins_hash_vector(*((DWORD *) jenkins_packed),k,keylen,hashes);
}
