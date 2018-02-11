#include "stdafx.h"

#include<stdlib.h>
#include<stdio.h>
#include<limits.h>
#include<string.h>

#include "select.h"
#include"compressed_rank.h"
#include"bitbool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static inline DWORD compressed_rank_i_log2(DWORD x)
{
   DWORD res   = 0;

   while (x > 1)
   {
      x >>= 1;
      res++;
   }

   return res;
};

void compressed_rank_init(compressed_rank_t* cr)
{
   cr->max_val = 0;
   cr->n = 0;
   cr->rem_r = 0;

   select_init(&cr->sel);

   cr->vals_rems = 0;
}

void compressed_rank_destroy(compressed_rank_t* cr)
{
   free(cr->vals_rems);

   cr->vals_rems = 0;

   select_destroy(&cr->sel);
}

void compressed_rank_generate(compressed_rank_t* cr,DWORD* vals_table,DWORD n)
{
   DWORD    i, j;
   DWORD    rems_mask;
   DWORD*   select_vec  = 0;

   cr->n = n;
   cr->max_val = vals_table[cr->n - 1];
   cr->rem_r = compressed_rank_i_log2(cr->max_val / cr->n);

   if (!cr->rem_r)
   {
      cr->rem_r = 1;
   }

   select_vec = (DWORD *) calloc(cr->max_val >> cr->rem_r,sizeof(DWORD));

   cr->vals_rems = (DWORD *) calloc(BITS_TABLE_SIZE(cr->n,cr->rem_r),sizeof(DWORD));

   rems_mask = (1 << cr->rem_r) - 1;

   for (i = 0; i < cr->n; i++)
   {
      set_bits_value(cr->vals_rems,i,vals_table[i] & rems_mask,cr->rem_r,rems_mask);
   }

   for (i = 1, j = 0; i <= cr->max_val >> cr->rem_r; i++)
   {
      while (i > (vals_table[j] >> cr->rem_r))
      {
         j++;
      }

      select_vec[i - 1] = j;
   };


   // FABIANO: before it was (cr->total_length >> cr->rem_r) + 1. But I wiped out the + 1 because
   // I changed the select structure to work up to m, instead of up to m - 1.
   select_generate(&cr->sel,select_vec,cr->max_val >> cr->rem_r,cr->n);

   free(select_vec);
}

DWORD compressed_rank_query(compressed_rank_t* cr,DWORD idx)
{
   DWORD rems_mask;
   DWORD val_quot, val_rem;
   DWORD sel_res, rank;

   if (idx > cr->max_val)
   {
      return cr->n;
   }

   val_quot = idx >> cr->rem_r;  

   rems_mask = (1U << cr->rem_r) - 1U; 

   val_rem = idx & rems_mask; 

   if (!val_quot)
   {
      rank = sel_res = 0;
   }
   else
   {
      sel_res = select_query(&cr->sel,val_quot - 1) + 1;
      rank = sel_res - val_quot;
   }

   do
   {
      if (GETBIT32(cr->sel.bits_vec,sel_res))
      {
         break;
      }
      if (get_bits_value(cr->vals_rems,rank,cr->rem_r,rems_mask) >= val_rem)
      {
         break;
      }

      sel_res++;
      rank++;
   }
   while (1); 

   return rank;
}

DWORD compressed_rank_get_space_usage(compressed_rank_t* cr)
{
   DWORD space_usage = select_get_space_usage(&cr->sel);

   space_usage += BITS_TABLE_SIZE(cr->n,cr->rem_r) * (DWORD)sizeof(DWORD) * 8;
   space_usage += 3 * (DWORD)sizeof(DWORD) * 8;

   return space_usage;
}

void compressed_rank_dump(compressed_rank_t* cr,char** buf,DWORD* buflen)
{
   DWORD sel_size       = select_packed_size(&(cr->sel));
   DWORD vals_rems_size = BITS_TABLE_SIZE(cr->n,cr->rem_r) * (DWORD)sizeof(DWORD);
   DWORD pos            = 0;

   char* buf_sel        = 0;
   DWORD buflen_sel     = 0;

   *buflen = 4 * (DWORD)sizeof(DWORD) + sel_size + vals_rems_size;

   TRACE("sel_size = %u\n",sel_size);
   TRACE("vals_rems_size = %u\n",vals_rems_size);

   *buf = (char *) calloc(*buflen,sizeof(char));

   if (!*buf)
   {
      *buflen = UINT_MAX;
      return;
   }

   // dumping max_val, n and rem_r
   memcpy(*buf,&(cr->max_val),sizeof(DWORD));

   pos += (DWORD)sizeof(DWORD);

   TRACE("max_val = %u\n",cr->max_val);

   memcpy(*buf + pos,&(cr->n),sizeof(DWORD));

   pos += (DWORD)sizeof(DWORD);

   TRACE("n = %u\n",cr->n);

   memcpy(*buf + pos,&(cr->rem_r),sizeof(DWORD));

   pos += (DWORD)sizeof(DWORD);

   TRACE("rem_r = %u\n",cr->rem_r);

   // dumping sel
   select_dump(&cr->sel,&buf_sel,&buflen_sel);

   memcpy(*buf + pos,&buflen_sel,sizeof(DWORD));

   pos += (DWORD)sizeof(DWORD);

   TRACE("buflen_sel = %u\n",buflen_sel);

   memcpy(*buf + pos,buf_sel,buflen_sel);

   #ifdef DEBUG   
   DWORD i  = 0; 

   for (i = 0; i < buflen_sel; i++)
   {
      TRACE("pos = %u  -- buf_sel[%u] = %u\n",pos,i,*(*buf + pos + i));
   }
   #endif

   pos += buflen_sel;

   free(buf_sel);

   // dumping vals_rems
   memcpy(*buf + pos,cr->vals_rems,vals_rems_size);

   #ifdef DEBUG   
   for (i = 0; i < vals_rems_size; i++)
   {
      TRACE("pos = %u -- vals_rems_size = %u  -- vals_rems[%u] = %u\n",pos,vals_rems_size,i,*(*buf + pos + i));
   }
   #endif

   pos += vals_rems_size;

   TRACE("Dumped compressed rank structure with size %u bytes\n",*buflen);
}

void compressed_rank_load(compressed_rank_t* cr,const char* buf,DWORD buflen)
{
   DWORD pos            = 0;
   DWORD buflen_sel     = 0;
   DWORD vals_rems_size = 0;

   // loading max_val, n, and rem_r
   memcpy(&(cr->max_val),buf,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("max_val = %u\n",cr->max_val);

   memcpy(&(cr->n),buf + pos,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("n = %u\n",cr->n);

   memcpy(&(cr->rem_r),buf + pos,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("rem_r = %u\n",cr->rem_r);

   // loading sel
   memcpy(&buflen_sel,buf + pos,sizeof(DWORD));
   pos += (DWORD)sizeof(DWORD);

   TRACE("buflen_sel = %u\n",buflen_sel);

   select_load(&cr->sel,buf + pos,buflen_sel);

   #ifdef DEBUG   
   DWORD i  = 0;  

   for (i = 0; i < buflen_sel; i++)
   {
      DEBUGP("pos = %u  -- buf_sel[%u] = %u\n",pos,i,*(buf + pos + i));
   }
   #endif

   pos += buflen_sel;

   // loading vals_rems
   if (cr->vals_rems)
   {
      free(cr->vals_rems);
   }

   vals_rems_size = BITS_TABLE_SIZE(cr->n,cr->rem_r);

   cr->vals_rems = (DWORD *) calloc(vals_rems_size,sizeof(DWORD));

   vals_rems_size *= 4;

   memcpy(cr->vals_rems,buf + pos,vals_rems_size);

   #ifdef DEBUG   
   for (i = 0; i < vals_rems_size; i++)
   {
      TRACE("pos = %u -- vals_rems_size = %u  -- vals_rems[%u] = %u\n",pos,vals_rems_size,i,*(buf + pos + i));
   }
   #endif

   pos += vals_rems_size;

   TRACE("Loaded compressed rank structure with size %u bytes\n",buflen);
}

void compressed_rank_pack(compressed_rank_t* cr,void* cr_packed)
{
   if (cr && cr_packed)
   {
      char* buf      = NULL;
      DWORD buflen   = 0;

      compressed_rank_dump(cr,&buf,&buflen);

      memcpy(cr_packed,buf,buflen);

      free(buf);
   }
}

DWORD compressed_rank_packed_size(compressed_rank_t* cr)
{
   DWORD sel_size       = select_packed_size(&cr->sel);
   DWORD vals_rems_size = BITS_TABLE_SIZE(cr->n,cr->rem_r) * (DWORD)sizeof(DWORD); 

   return 4 * (DWORD)sizeof(DWORD) + sel_size + vals_rems_size;
}

DWORD compressed_rank_query_packed(void* cr_packed,DWORD idx)
{
   // unpacking cr_packed
   DWORD*   ptr         = (DWORD*) cr_packed;
   DWORD    max_val     = *ptr++;
   DWORD    n           = *ptr++;
   DWORD    rem_r       = *ptr++;
   DWORD    buflen_sel  = *ptr++;
   DWORD*   sel_packed  = ptr;

   DWORD*   bits_vec    = sel_packed + 2; // skipping n and m

   DWORD*   vals_rems   = (ptr += (buflen_sel >> 2)); 

   // compressed sequence query computation
   DWORD    rems_mask;
   DWORD    val_quot, val_rem;
   DWORD    sel_res, rank;

   if (idx > max_val)
   {
      return n;
   }

   val_quot = idx >> rem_r;   
   rems_mask = (1U << rem_r) - 1U; 

   val_rem = idx & rems_mask; 

   if (val_quot == 0)
   {
      rank = sel_res = 0;
   }
   else
   {
      sel_res = select_query_packed(sel_packed,val_quot - 1) + 1;
      rank = sel_res - val_quot;
   }

   do
   {
      if (GETBIT32(bits_vec,sel_res))
      {
         break;
      }

      if (get_bits_value(vals_rems,rank,rem_r,rems_mask) >= val_rem)
      {
         break;
      }

      sel_res++;
      rank++;
   }
   while (true);   

   return rank;
}



