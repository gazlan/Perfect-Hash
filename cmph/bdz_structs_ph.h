#ifndef __CMPH_BDZ_STRUCTS_PH_H__
#define __CMPH_BDZ_STRUCTS_PH_H__

struct __bdz_ph_data_t
{
   DWORD          m; //edges (words) count
   DWORD          n; //vertex count
   DWORD          r; //partition vertex count
   BYTE*          g;
   hash_state_t*  hl; // linear hashing
};

struct __bdz_ph_config_data_t
{
   CMPH_HASH      hashfunc;
   DWORD          m; //edges (words) count
   DWORD          n; //vertex count
   DWORD          r; //partition vertex count
   BYTE*          g;
   hash_state_t*  hl; // linear hashing
};

#endif
