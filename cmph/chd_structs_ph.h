#ifndef __CMPH_CHD_PH_STRUCTS_H__
#define __CMPH_CHD_PH_STRUCTS_H__

struct __chd_ph_data_t
{
   compressed_seq_t* cs;  // compressed displacement values
   DWORD             nbuckets;   // number of buckets
   DWORD             n;    // number of bins
   hash_state_t*     hl; // linear hash function
};

struct __chd_ph_config_data_t
{
   CMPH_HASH         hashfunc;  // linear hash function to be used
   compressed_seq_t* cs;  // compressed displacement values
   DWORD             nbuckets;   // number of buckets
   DWORD             n;    // number of bins
   hash_state_t*     hl; // linear hash function

   DWORD             m;    // number of keys
   BYTE              use_h; // flag to indicate the of use of a heuristic (use_h = 1)
   DWORD             keys_per_bin;//maximum number of keys per bin 
   DWORD             keys_per_bucket; // average number of keys per bucket
   BYTE*             occup_table;     // table that indicates occupied positions 
};

#endif
