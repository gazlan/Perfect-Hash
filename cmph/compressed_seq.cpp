#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "select.h"
#include "compressed_seq.h"
#include "bitbool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static inline DWORD compressed_seq_i_log2(DWORD x)
{
   DWORD res   = 0;

   while (x > 1)
   {
      x >>= 1;
      res++;
   }

   return res;
}

void compressed_seq_init(compressed_seq_t* cs)
{
   select_init(&cs->sel);

   cs->n = 0;
   cs->rem_r = 0;
   cs->length_rems = 0;
   cs->total_length = 0;
   cs->store_table = 0;
}

void compressed_seq_destroy(compressed_seq_t* cs)
{
   free(cs->store_table);

   cs->store_table = 0;

   free(cs->length_rems);

   cs->length_rems = 0;

   select_destroy(&cs->sel);
}

void compressed_seq_generate(compressed_seq_t* cs,DWORD* vals_table,DWORD n)
{
   DWORD    i;
   // lengths: represents lengths of encoded values   
   DWORD*   lengths  = (DWORD*) calloc(n,sizeof(DWORD));
   DWORD    rems_mask;
   DWORD    stored_value;

   cs->n = n;
   cs->total_length = 0;

   for (i = 0; i < cs->n; i++)
   {
      if (vals_table[i] == 0)
      {
         lengths[i] = 0;
      }
      else
      {
         lengths[i] = compressed_seq_i_log2(vals_table[i] + 1);
         cs->total_length += lengths[i];
      };
   };

   if (cs->store_table)
   {
      free(cs->store_table);
   }
   cs->store_table = (DWORD *) calloc(((cs->total_length + 31) >> 5),sizeof(DWORD));
   cs->total_length = 0;

   for (i = 0; i < cs->n; i++)
   {
      if (vals_table[i] == 0)
      {
         continue;
      }
      stored_value = vals_table[i] - ((1U << lengths[i]) - 1U);
      set_bits_at_pos(cs->store_table,cs->total_length,stored_value,lengths[i]);
      cs->total_length += lengths[i];
   };

   cs->rem_r = compressed_seq_i_log2(cs->total_length / cs->n);

   if (cs->rem_r == 0)
   {
      cs->rem_r = 1;
   }

   if (cs->length_rems)
   {
      free(cs->length_rems);
   }

   cs->length_rems = (DWORD *) calloc(BITS_TABLE_SIZE(cs->n,cs->rem_r),sizeof(DWORD));

   rems_mask = (1U << cs->rem_r) - 1U;
   cs->total_length = 0;

   for (i = 0; i < cs->n; i++)
   {
      cs->total_length += lengths[i];
      set_bits_value(cs->length_rems,i,cs->total_length & rems_mask,cs->rem_r,rems_mask);
      lengths[i] = cs->total_length >> cs->rem_r;
   }

   select_init(&cs->sel);

   // FABIANO: before it was (cs->total_length >> cs->rem_r) + 1. But I wiped out the + 1 because
   // I changed the select structure to work up to m, instead of up to m - 1.
   select_generate(&cs->sel,lengths,cs->n,(cs->total_length >> cs->rem_r));

   free(lengths);
}

DWORD compressed_seq_get_space_usage(compressed_seq_t* cs)
{
   DWORD space_usage = select_get_space_usage(&cs->sel);
   space_usage += ((cs->total_length + 31) >> 5) * (DWORD)sizeof(DWORD) * 8;
   space_usage += BITS_TABLE_SIZE(cs->n,cs->rem_r) * (DWORD)sizeof(DWORD) * 8;
   return  4 * (DWORD)sizeof(DWORD) * 8 + space_usage;
}

DWORD compressed_seq_query(compressed_seq_t* cs,DWORD idx)
{
   DWORD enc_idx, enc_length;
   DWORD rems_mask;
   DWORD stored_value;
   DWORD sel_res;

   ASSERT(idx < cs->n); // FABIANO ADDED

   rems_mask = (1U << cs->rem_r) - 1U;

   if (idx == 0)
   {
      enc_idx = 0;
      sel_res = select_query(&cs->sel,idx);
   }
   else
   {
      sel_res = select_query(&cs->sel,idx - 1);

      enc_idx = (sel_res - (idx - 1)) << cs->rem_r;
      enc_idx += get_bits_value(cs->length_rems,idx - 1,cs->rem_r,rems_mask);

      sel_res = select_next_query(&cs->sel,sel_res);
   };

   enc_length = (sel_res - idx) << cs->rem_r;
   enc_length += get_bits_value(cs->length_rems,idx,cs->rem_r,rems_mask);
   enc_length -= enc_idx;
   if (enc_length == 0)
   {
      return 0;
   }

   stored_value = get_bits_at_pos(cs->store_table,enc_idx,enc_length);
   return stored_value + ((1U << enc_length) - 1U);
}

void compressed_seq_dump(compressed_seq_t* cs,char** buf,DWORD* buflen)
{
   DWORD sel_size          = select_packed_size(&(cs->sel));
   DWORD length_rems_size  = BITS_TABLE_SIZE(cs->n,cs->rem_r) * 4;
   DWORD store_table_size  = ((cs->total_length + 31) >> 5) * 4;
   DWORD pos               = 0;
   char* buf_sel           = 0;
   DWORD buflen_sel        = 0;

   *buflen = 4 * (DWORD)sizeof(DWORD) + sel_size + length_rems_size + store_table_size;

   TRACE("sel_size = %u\n",sel_size);
   TRACE("length_rems_size = %u\n",length_rems_size);
   TRACE("store_table_size = %u\n",store_table_size);

   *buf = (char *) calloc(*buflen,sizeof(char));

   if (!*buf)
   {
      *buflen = UINT_MAX;
      return;
   }

   // dumping n, rem_r and total_length
   memcpy(*buf,&(cs->n),sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("n = %u\n",cs->n);

   memcpy(*buf + pos,&(cs->rem_r),sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("rem_r = %u\n",cs->rem_r);

   memcpy(*buf + pos,&(cs->total_length),sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("total_length = %u\n",cs->total_length);

   // dumping sel
   select_dump(&cs->sel,&buf_sel,&buflen_sel);
   memcpy(*buf + pos,&buflen_sel,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("buflen_sel = %u\n",buflen_sel);

   memcpy(*buf + pos,buf_sel,buflen_sel);

   #ifdef DEBUG   
   DWORD i  = 0; 
   for (i = 0; i < buflen_sel; i++)
   {
      DEBUGP("pos = %u  -- buf_sel[%u] = %u\n",pos,i,*(*buf + pos + i));
   }
   #endif

   pos += buflen_sel;

   free(buf_sel);

   // dumping length_rems
   memcpy(*buf + pos,cs->length_rems,length_rems_size);

   #ifdef DEBUG   
   for (i = 0; i < length_rems_size; i++)
   {
      TRACE("pos = %u -- length_rems_size = %u  -- length_rems[%u] = %u\n",pos,length_rems_size,i,*(*buf + pos + i));
   }
   #endif

   pos += length_rems_size;

   // dumping store_table
   memcpy(*buf + pos,cs->store_table,store_table_size);

   #ifdef DEBUG   
   for (i = 0; i < store_table_size; i++)
   {
      TRACE("pos = %u -- store_table_size = %u  -- store_table[%u] = %u\n",pos,store_table_size,i,*(*buf + pos + i));
   }
   #endif

   TRACE("Dumped compressed sequence structure with size %u bytes\n",*buflen);
}

void compressed_seq_load(compressed_seq_t* cs,const char* buf,DWORD buflen)
{
   DWORD pos               = 0;
   DWORD buflen_sel        = 0;
   DWORD length_rems_size  = 0;
   DWORD store_table_size  = 0;

   // loading n, rem_r and total_length
   memcpy(&(cs->n),buf,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("n = %u\n",cs->n);

   memcpy(&(cs->rem_r),buf + pos,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("rem_r = %u\n",cs->rem_r);

   memcpy(&(cs->total_length),buf + pos,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("total_length = %u\n",cs->total_length);

   // loading sel
   memcpy(&buflen_sel,buf + pos,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("buflen_sel = %u\n",buflen_sel);

   select_load(&cs->sel,buf + pos,buflen_sel);

   #ifdef DEBUG   
   DWORD i  = 0;  

   for (i = 0; i < buflen_sel; i++)
   {
      TRACE("pos = %u  -- buf_sel[%u] = %u\n",pos,i,*(buf + pos + i));
   }
   #endif

   pos += buflen_sel;

   // loading length_rems
   if (cs->length_rems)
   {
      free(cs->length_rems);
   }

   length_rems_size = BITS_TABLE_SIZE(cs->n,cs->rem_r);

   cs->length_rems = (DWORD *) calloc(length_rems_size,sizeof(DWORD));

   length_rems_size *= 4;

   memcpy(cs->length_rems,buf + pos,length_rems_size);

   #ifdef DEBUG   
   for (i = 0; i < length_rems_size; i++)
   {
      TRACE("pos = %u -- length_rems_size = %u  -- length_rems[%u] = %u\n",pos,length_rems_size,i,*(buf + pos + i));
   }
   #endif

   pos += length_rems_size;

   // loading store_table
   store_table_size = ((cs->total_length + 31) >> 5);

   if (cs->store_table)
   {
      free(cs->store_table);
   }

   cs->store_table = (DWORD *) calloc(store_table_size,sizeof(DWORD));
   store_table_size *= 4;

   memcpy(cs->store_table,buf + pos,store_table_size);

   #ifdef DEBUG   
   for (i = 0; i < store_table_size; i++)
   {
      TRACE("pos = %u -- store_table_size = %u  -- store_table[%u] = %u\n",pos,store_table_size,i,*(buf + pos + i));
   }
   #endif

   TRACE("Loaded compressed sequence structure with size %u bytes\n",buflen);
}

void compressed_seq_pack(compressed_seq_t* cs,void* cs_packed)
{
   if (cs && cs_packed)
   {
      char* buf      = NULL;
      DWORD buflen   = 0;
      compressed_seq_dump(cs,&buf,&buflen);
      memcpy(cs_packed,buf,buflen);
      free(buf);
   }
}

DWORD compressed_seq_packed_size(compressed_seq_t* cs)
{
   DWORD sel_size          = select_packed_size(&cs->sel);
   DWORD store_table_size  = ((cs->total_length + 31) >> 5) * (DWORD)sizeof(DWORD);
   DWORD length_rems_size  = BITS_TABLE_SIZE(cs->n,cs->rem_r) * (DWORD)sizeof(DWORD);
   return 4 * (DWORD)sizeof(DWORD) + sel_size + store_table_size + length_rems_size;
}

DWORD compressed_seq_query_packed(void* cs_packed,DWORD idx)
{
   // unpacking cs_packed
   DWORD*   ptr   = (DWORD*) cs_packed;
   DWORD    n     = *ptr++;
   DWORD    rem_r = *ptr++;
   ptr++; // skipping total_length 

   DWORD    buflen_sel        = *ptr++;
   DWORD*   sel_packed        = ptr;
   DWORD*   length_rems       = (ptr += (buflen_sel >> 2)); 
   DWORD    length_rems_size  = BITS_TABLE_SIZE(n,rem_r);
   DWORD*   store_table       = (ptr += length_rems_size);

   // compressed sequence query computation
   DWORD    enc_idx, enc_length;
   DWORD    rems_mask;
   DWORD    stored_value;
   DWORD    sel_res;

   rems_mask = (1U << rem_r) - 1U;

   if (idx == 0)
   {
      enc_idx = 0;
      sel_res = select_query_packed(sel_packed,idx);
   }
   else
   {
      sel_res = select_query_packed(sel_packed,idx - 1);

      enc_idx = (sel_res - (idx - 1)) << rem_r;
      enc_idx += get_bits_value(length_rems,idx - 1,rem_r,rems_mask);

      sel_res = select_next_query_packed(sel_packed,sel_res);
   };

   enc_length = (sel_res - idx) << rem_r;
   enc_length += get_bits_value(length_rems,idx,rem_r,rems_mask);
   enc_length -= enc_idx;

   if (enc_length == 0)
   {
      return 0;
   }

   stored_value = get_bits_at_pos(store_table,enc_idx,enc_length);
   return stored_value + ((1U << enc_length) - 1U);
}
