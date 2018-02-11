#ifndef __CMPH_BDZ_STRUCTS_H__
#define __CMPH_BDZ_STRUCTS_H__

struct bdz_data_t
{
   DWORD          m; //edges (words) count
   DWORD          n; //vertex count
   DWORD          r; //partition vertex count
   BYTE*          g;
   hash_state_t*  hl; // linear hashing

   DWORD          k; //kth index in ranktable, $k = log_2(n=3r)/\varepsilon$
   BYTE           b; // number of bits of k
   DWORD          ranktablesize; //number of entries in ranktable, $n/k +1$
   DWORD*         ranktable; // rank table
};

struct bdz_config_data_t
{
   DWORD          m; //edges (words) count
   DWORD          n; //vertex count
   DWORD          r; //partition vertex count
   BYTE*          g;
   hash_state_t*  hl; // linear hashing

   DWORD          k; //kth index in ranktable, $k = log_2(n=3r)/\varepsilon$
   BYTE           b; // number of bits of k
   DWORD          ranktablesize; //number of entries in ranktable, $n/k +1$
   DWORD*         ranktable; // rank table
   CMPH_HASH      hashfunc;
};

#endif
