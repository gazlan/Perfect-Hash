#ifndef __CMPH_CHD_STRUCTS_H__
#define __CMPH_CHD_STRUCTS_H__

struct __chd_data_t
{
   DWORD packed_cr_size;
   BYTE* packed_cr; // packed compressed rank structure to control the number of zeros in a bit vector

   DWORD packed_chd_phf_size;
   BYTE* packed_chd_phf;
};

struct __chd_config_data_t
{
   cmph_config_t* chd_ph;     // chd_ph algorithm must be used here
};

#endif
