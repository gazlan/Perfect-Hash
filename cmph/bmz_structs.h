#ifndef __CMPH_BMZ_STRUCTS_H__
#define __CMPH_BMZ_STRUCTS_H__

struct __bmz_data_t
{
   DWORD          m; //edges (words) count
   DWORD          n; //vertex count
   DWORD*         g;
   hash_state_t** hashes;
};

struct __bmz_config_data_t
{
   CMPH_HASH      hashfuncs[2];
   DWORD          m; //edges (words) count
   DWORD          n; //vertex count
   graph_t*       graph;
   DWORD*         g;
   hash_state_t** hashes;
};

#endif
