#ifndef __CMPH_BMZ8_STRUCTS_H__
#define __CMPH_BMZ8_STRUCTS_H__

struct __bmz8_data_t
{
   BYTE           m; //edges (words) count
   BYTE           n; //vertex count
   BYTE*          g;
   hash_state_t** hashes;
};

struct __bmz8_config_data_t
{
   CMPH_HASH      hashfuncs[2];
   BYTE           m; //edges (words) count
   BYTE           n; //vertex count
   graph_t*       graph;
   BYTE*          g;
   hash_state_t** hashes;
};

#endif
