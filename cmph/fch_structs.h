#ifndef __CMPH_FCH_STRUCTS_H__
#define __CMPH_FCH_STRUCTS_H__

struct __fch_data_t
{
   DWORD          m;       // words count
   double         c;      // constant c
   DWORD          b;      // parameter b = ceil(c*m/(log(m)/log(2) + 1)). Don't need to be stored 
   double         p1;     // constant p1 = ceil(0.6*m). Don't need to be stored 
   double         p2;     // constant p2 = ceil(0.3*b). Don't need to be stored 
   DWORD*         g;      // g function. 
   hash_state_t*  h1;    // h10 function. 
   hash_state_t*  h2;    // h20 function.
};

struct __fch_config_data_t
{
   CMPH_HASH      hashfuncs[2];
   DWORD          m;       // words count
   double         c;      // constant c
   DWORD          b;      // parameter b = ceil(c*m/(log(m)/log(2) + 1)). Don't need to be stored 
   double         p1;     // constant p1 = ceil(0.6*m). Don't need to be stored 
   double         p2;     // constant p2 = ceil(0.3*b). Don't need to be stored 
   DWORD*         g;      // g function. 
   hash_state_t*  h1;    // h10 function. 
   hash_state_t*  h2;    // h20 function.
};
#endif
