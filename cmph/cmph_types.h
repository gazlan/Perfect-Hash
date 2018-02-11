#ifndef __CMPH_TYPES_H__
#define __CMPH_TYPES_H__

enum CMPH_HASH
{
   CMPH_HASH_JENKINS,
   CMPH_HASH_COUNT
};

extern char*cmph_hash_names[];

enum CMPH_ALGO
{
   CMPH_BMZ,
   CMPH_BMZ8,
   CMPH_CHM,
   CMPH_BRZ,
   CMPH_FCH,
   CMPH_BDZ,
   CMPH_BDZ_PH,
   CMPH_CHD_PH,
   CMPH_CHD,
   CMPH_COUNT
};

extern const char*cmph_names[];

#endif
