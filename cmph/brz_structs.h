#ifndef __CMPH_BRZ_STRUCTS_H__
#define __CMPH_BRZ_STRUCTS_H__

struct __brz_data_t
{
   CMPH_ALGO      algo;      // CMPH algo for generating the MPHFs for the buckets (Just CMPH_FCH and CMPH_BMZ8)
   DWORD          m;       // edges (words) count
   double         c;      // constant c
   BYTE*          size;   // size[i] stores the number of edges represented by g[i][...]. 
   DWORD*         offset; // offset[i] stores the sum: size[0] + size[1] + ... size[i-1].
   BYTE**         g;      // g function. 
   DWORD          k;       // number of components
   hash_state_t** h1;
   hash_state_t** h2;
   hash_state_t*  h0;
};

struct __brz_config_data_t
{
   CMPH_HASH      hashfuncs[3];
   CMPH_ALGO      algo;      // CMPH algo for generating the MPHFs for the buckets (Just CMPH_FCH and CMPH_BMZ8)
   double         c;      // constant c
   DWORD          m;       // edges (words) count
   BYTE*          size;   // size[i] stores the number of edges represented by g[i][...]. 
   DWORD*         offset; // offset[i] stores the sum: size[0] + size[1] + ... size[i-1].
   BYTE**         g;      // g function. 
   BYTE           b;       // parameter b. 
   DWORD          k;       // number of components
   hash_state_t** h1;
   hash_state_t** h2;
   hash_state_t*  h0;    
   DWORD          memory_availability; 
   BYTE*          tmp_dir; // temporary directory 
   FILE*          mphf_fd; // mphf file
};

#endif
