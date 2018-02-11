#include "stdafx.h"

#include<stdlib.h>
#include<stdio.h>
#include <string.h>
#include <limits.h>
#include "select_lookup_tables.h"
#include "select.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef STEP_SELECT_TABLE
#define STEP_SELECT_TABLE 128
#endif

#ifndef NBITS_STEP_SELECT_TABLE
#define NBITS_STEP_SELECT_TABLE 7
#endif

#ifndef MASK_STEP_SELECT_TABLE
#define MASK_STEP_SELECT_TABLE 0x7f // 0x7f = 127
#endif

static inline void select_insert_0(DWORD* buffer)
{
   (*buffer) >>= 1;
}

static inline void select_insert_1(DWORD* buffer)
{
   (*buffer) >>= 1;
   (*buffer) |= 0x80000000;
}

void select_init(select_t* sel)
{
   sel->n = 0;
   sel->m = 0;
   sel->bits_vec = 0;
   sel->select_table = 0;
}

DWORD select_get_space_usage(select_t* sel)
{
   DWORD nbits;
   DWORD vec_size;
   DWORD sel_table_size;
   DWORD space_usage;

   nbits = sel->n + sel->m;
   vec_size = (nbits + 31) >> 5;
   sel_table_size = (sel->n >> NBITS_STEP_SELECT_TABLE) + 1; // (sel->n >> NBITS_STEP_SELECT_TABLE) = (sel->n/STEP_SELECT_TABLE)

   space_usage = 2 * sizeof(DWORD) * 8; // n and m
   space_usage += vec_size * (DWORD) sizeof(DWORD) * 8;
   space_usage += sel_table_size * (DWORD)sizeof(DWORD) * 8;
   return space_usage;
}

void select_destroy(select_t* sel)
{
   free(sel->bits_vec);
   free(sel->select_table);
   sel->bits_vec = 0;
   sel->select_table = 0;
}

static inline void select_generate_sel_table(select_t* sel)
{
   BYTE* bits_table  = (BYTE*) sel->bits_vec;
   DWORD part_sum, old_part_sum;
   DWORD vec_idx, one_idx, sel_table_idx;

   part_sum = vec_idx = one_idx = sel_table_idx = 0;

   for (; ;)
   {
      // FABIANO: Should'n it be one_idx >= sel->n
      if (one_idx >= sel->n)
      {
         break;
      }
      do
      {
         old_part_sum = part_sum; 
         part_sum += rank_lookup_table[bits_table[vec_idx]];
         vec_idx++;
      }
      while (part_sum <= one_idx);

      sel->select_table[sel_table_idx] = select_lookup_table[bits_table[vec_idx - 1]][one_idx - old_part_sum] + ((vec_idx - 1) << 3); // ((vec_idx - 1) << 3) = ((vec_idx - 1) * 8)
      one_idx += STEP_SELECT_TABLE ;
      sel_table_idx++;
   }
}

void select_generate(select_t* sel,DWORD* keys_vec,DWORD n,DWORD m)
{
   DWORD i, j, idx;
   DWORD buffer   = 0;
   DWORD nbits;
   DWORD vec_size;
   DWORD sel_table_size;

   sel->n = n;
   sel->m = m; // n values in the range [0,m-1]

   nbits = sel->n + sel->m; 
   vec_size = (nbits + 31) >> 5; // (nbits + 31) >> 5 = (nbits + 31)/32

   sel_table_size = (sel->n >> NBITS_STEP_SELECT_TABLE) + 1; // (sel->n >> NBITS_STEP_SELECT_TABLE) = (sel->n/STEP_SELECT_TABLE)

   if (sel->bits_vec)
   {
      free(sel->bits_vec);
   }   

   sel->bits_vec = (DWORD *) calloc(vec_size,sizeof(DWORD));

   if (sel->select_table)
   {
      free(sel->select_table);
   }

   sel->select_table = (DWORD *) calloc(sel_table_size,sizeof(DWORD));

   idx = i = j = 0;

   for (; ;)
   {
      while (keys_vec[j] == i)
      {
         select_insert_1(&buffer);
         idx++;

         if ((idx & 0x1f) == 0) // (idx & 0x1f) = idx % 32
         {
            sel->bits_vec[(idx >> 5) - 1] = buffer;
         } // (idx >> 5) = idx/32
         j++;

         if (j == sel->n)
         {
            goto LOOP_END;
         }
      }

      if (i == sel->m)
      {
         break;
      }

      while (keys_vec[j] > i)
      {
         select_insert_0(&buffer);
         idx++;

         if (!(idx & 0x1f)) // (idx & 0x1f) = idx % 32
         {
            sel->bits_vec[(idx >> 5) - 1] = buffer;
         } // (idx >> 5) = idx/32

         i++;
      }
   }

   LOOP_END:

   if ((idx & 0x1f) != 0) // (idx & 0x1f) = idx % 32
   {
      buffer >>= 32 - (idx & 0x1f);
      sel->bits_vec[(idx - 1) >> 5] = buffer;
   };

   select_generate_sel_table(sel);
}

static inline DWORD _select_query(BYTE* bits_table,DWORD* select_table,DWORD one_idx)
{
   DWORD vec_bit_idx, vec_byte_idx;
   DWORD part_sum, old_part_sum;

   vec_bit_idx = select_table[one_idx >> NBITS_STEP_SELECT_TABLE]; // one_idx >> NBITS_STEP_SELECT_TABLE = one_idx/STEP_SELECT_TABLE
   vec_byte_idx = vec_bit_idx >> 3; // vec_bit_idx / 8

   one_idx &= MASK_STEP_SELECT_TABLE; // one_idx %= STEP_SELECT_TABLE == one_idx &= MASK_STEP_SELECT_TABLE
   one_idx += rank_lookup_table[bits_table[vec_byte_idx] & ((1 << (vec_bit_idx & 0x7)) - 1)];
   part_sum = 0;

   do
   {
      old_part_sum = part_sum; 
      part_sum += rank_lookup_table[bits_table[vec_byte_idx]];
      vec_byte_idx++;
   }
   while (part_sum <= one_idx);

   return select_lookup_table[bits_table[vec_byte_idx - 1]][one_idx - old_part_sum] + ((vec_byte_idx - 1) << 3);
}

DWORD select_query(select_t* sel,DWORD one_idx)
{
   return _select_query((BYTE *) sel->bits_vec,sel->select_table,one_idx);
};


static inline DWORD _select_next_query(BYTE* bits_table,DWORD vec_bit_idx)
{
   DWORD vec_byte_idx, one_idx;
   DWORD part_sum, old_part_sum;

   vec_byte_idx = vec_bit_idx >> 3;

   one_idx = rank_lookup_table[bits_table[vec_byte_idx] & ((1U << (vec_bit_idx & 0x7)) - 1U)] + 1U;
   part_sum = 0;

   do
   {
      old_part_sum = part_sum; 
      part_sum += rank_lookup_table[bits_table[vec_byte_idx]];
      vec_byte_idx++;
   }
   while (part_sum <= one_idx);

   return select_lookup_table[bits_table[(vec_byte_idx - 1)]][(one_idx - old_part_sum)] + ((vec_byte_idx - 1) << 3);
}

DWORD select_next_query(select_t* sel,DWORD vec_bit_idx)
{
   return _select_next_query((BYTE *) sel->bits_vec,vec_bit_idx);
};

void select_dump(select_t* sel,char** buf,DWORD* buflen)
{
   DWORD nbits          = sel->n + sel->m;
   DWORD vec_size       = ((nbits + 31) >> 5) * (DWORD)sizeof(DWORD); // (nbits + 31) >> 5 = (nbits + 31)/32
   DWORD sel_table_size = ((sel->n >> NBITS_STEP_SELECT_TABLE) + 1) * (DWORD)sizeof(DWORD); // (sel->n >> NBITS_STEP_SELECT_TABLE) = (sel->n/STEP_SELECT_TABLE)
   DWORD pos            = 0;

   *buflen = 2 * (DWORD)sizeof(DWORD) + vec_size + sel_table_size;

   *buf = (char *) calloc(*buflen,sizeof(char));

   if (!*buf)
   {
      *buflen = UINT_MAX;
      return;
   }

   memcpy(*buf,&(sel->n),sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);
   memcpy(*buf + pos,&(sel->m),sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);
   memcpy(*buf + pos,sel->bits_vec,vec_size);
   pos += vec_size;
   memcpy(*buf + pos,sel->select_table,sel_table_size);

   TRACE("Dumped select structure with size %u bytes\n",*buflen);
}

void select_load(select_t* sel,const char* buf,DWORD buflen)
{
   DWORD pos            = 0;
   DWORD nbits          = 0;
   DWORD vec_size       = 0;
   DWORD sel_table_size = 0;

   memcpy(&(sel->n),buf,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);
   memcpy(&(sel->m),buf + pos,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   nbits = sel->n + sel->m;
   vec_size = ((nbits + 31) >> 5) * (DWORD)sizeof(DWORD); // (nbits + 31) >> 5 = (nbits + 31)/32
   sel_table_size = ((sel->n >> NBITS_STEP_SELECT_TABLE) + 1) * (DWORD)sizeof(DWORD); // (sel->n >> NBITS_STEP_SELECT_TABLE) = (sel->n/STEP_SELECT_TABLE)

   if (sel->bits_vec)
   {
      free(sel->bits_vec);
   }
   sel->bits_vec = (DWORD *) calloc(vec_size / sizeof(DWORD),sizeof(DWORD));

   if (sel->select_table)
   {
      free(sel->select_table);
   }
   sel->select_table = (DWORD *) calloc(sel_table_size / sizeof(DWORD),sizeof(DWORD));

   memcpy(sel->bits_vec,buf + pos,vec_size);
   pos += vec_size;
   memcpy(sel->select_table,buf + pos,sel_table_size);

   TRACE("Loaded select structure with size %u bytes\n",buflen);
}

/** \fn void select_pack(select_t *sel, void *sel_packed);
 *  \brief Support the ability to pack a select structure function into a preallocated contiguous memory space pointed by sel_packed.
 *  \param sel points to the select structure
 *  \param sel_packed pointer to the contiguous memory area used to store the select structure. The size of sel_packed must be at least @see select_packed_size 
 */
void select_pack(select_t* sel,void* sel_packed)
{
   if (sel && sel_packed)
   {
      char* buf      = NULL;
      DWORD buflen   = 0;
      select_dump(sel,&buf,&buflen);
      memcpy(sel_packed,buf,buflen);
      free(buf);
   }
}


/** \fn DWORD select_packed_size(select_t *sel);
 *  \brief Return the amount of space needed to pack a select structure.
 *  \return the size of the packed select structure or zero for failures
 */ 
DWORD select_packed_size(select_t* sel)
{
   DWORD nbits          = sel->n + sel->m;
   DWORD vec_size       = ((nbits + 31) >> 5) * (DWORD)sizeof(DWORD); // (nbits + 31) >> 5 = (nbits + 31)/32
   DWORD sel_table_size = ((sel->n >> NBITS_STEP_SELECT_TABLE) + 1) * (DWORD)sizeof(DWORD); // (sel->n >> NBITS_STEP_SELECT_TABLE) = (sel->n/STEP_SELECT_TABLE)
   return 2 * (DWORD)sizeof(DWORD) + vec_size + sel_table_size;
}



DWORD select_query_packed(void* sel_packed,DWORD one_idx)
{
   DWORD*   ptr            = (DWORD*) sel_packed;
   DWORD    n              = *ptr++;
   DWORD    m              = *ptr++;
   DWORD    nbits          = n + m;
   DWORD    vec_size       = (nbits + 31) >> 5; // (nbits + 31) >> 5 = (nbits + 31)/32
   BYTE*    bits_vec       = (BYTE*) ptr;
   DWORD*   select_table   = ptr + vec_size;

   return _select_query(bits_vec,select_table,one_idx);
}


DWORD select_next_query_packed(void* sel_packed,DWORD vec_bit_idx)
{
   BYTE* bits_vec = (BYTE*) sel_packed;
   bits_vec += 8; // skipping n and m
   return _select_next_query(bits_vec,vec_bit_idx);
}
