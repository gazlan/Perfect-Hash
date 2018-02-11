#ifndef __CMPH_SELECT_H__
#define __CMPH_SELECT_H__

struct _select_t
{
   DWORD                      n, m;
   DWORD*                     bits_vec;
   DWORD*                     select_table;
};

typedef struct _select_t   select_t;

void                       select_init(select_t* sel);
void                       select_destroy(select_t* sel);
void                       select_generate(select_t* sel,DWORD* keys_vec,DWORD n,DWORD m);
DWORD                      select_query(select_t* sel,DWORD one_idx);
DWORD                      select_next_query(select_t* sel,DWORD vec_bit_idx);
DWORD                      select_get_space_usage(select_t* sel);
void                       select_dump(select_t* sel,char** buf,DWORD* buflen);
void                       select_load(select_t* sel,const char* buf,DWORD buflen);

/** \fn void select_pack(select_t *sel, void *sel_packed);
 *  \brief Support the ability to pack a select structure into a preallocated contiguous memory space pointed by sel_packed.
 *  \param sel points to the select structure
 *  \param sel_packed pointer to the contiguous memory area used to store the select structure. The size of sel_packed must be at least @see select_packed_size 
 */
void                       select_pack(select_t* sel,void* sel_packed);

/** \fn DWORD select_packed_size(select_t *sel);
 *  \brief Return the amount of space needed to pack a select structure.
 *  \return the size of the packed select structure or zero for failures
 */ 
DWORD                      select_packed_size(select_t* sel);


/** \fn DWORD select_query_packed(void * sel_packed, DWORD one_idx);
 *  \param sel_packed is a pointer to a contiguous memory area
 *  \param one_idx is the rank for which we want to calculate the inverse function select
 *  \return an integer that represents the select value of rank idx.
 */
DWORD                      select_query_packed(void* sel_packed,DWORD one_idx);


/** \fn DWORD select_next_query_packed(void * sel_packed, DWORD vec_bit_idx);
 *  \param sel_packed is a pointer to a contiguous memory area
 *  \param vec_bit_idx is a value prior computed by @see select_query_packed
 *  \return an integer that represents the next select value greater than @see vec_bit_idx.
 */
DWORD                      select_next_query_packed(void* sel_packed,DWORD vec_bit_idx);

#endif
