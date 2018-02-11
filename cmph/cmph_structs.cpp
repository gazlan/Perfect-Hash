#include "stdafx.h"

#include <string.h>

#include "cmph_types.h"
#include "cmph.h"
#include "cmph_structs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

cmph_config_t* __config_new(cmph_io_adapter_t* key_source)
{
   cmph_config_t* mph   = (cmph_config_t*) malloc(sizeof(cmph_config_t));

   if (!mph)
   {
      return NULL;
   }

   memset(mph,0,sizeof(cmph_config_t));

   mph->key_source = key_source;
   mph->verbosity = 0;
   mph->data = NULL;
   mph->c = 0;

   return mph;
}

void __config_destroy(cmph_config_t* mph)
{
   free(mph);
}

void __cmph_dump(cmph_t* mphf,FILE* fd)
{
   size_t   nbytes;

   nbytes = fwrite(cmph_names[mphf->algo],(size_t) (strlen(cmph_names[mphf->algo]) + 1),(size_t) 1,fd);
   nbytes = fwrite(&(mphf->size),sizeof(mphf->size),(size_t) 1,fd);
}
cmph_t* __cmph_load(FILE* f)
{
   cmph_t*     mphf  = NULL;

   DWORD       i;

   char        algo_name[BUFSIZ];
   char*       ptr   = algo_name;

   CMPH_ALGO   algo  = CMPH_COUNT;

   size_t      nbytes;

   TRACE("Loading mphf\n");

   while (true)
   {
      size_t   c  = fread(ptr,(size_t) 1,(size_t) 1,f);

      if (c != 1)
      {
         return NULL;
      }

      if (!*ptr)
      {
         break;
      }

      ++ptr;
   }

   for (i = 0; i < CMPH_COUNT; ++i)
   {
      if (!strcmp(algo_name,cmph_names[i]))
      {
         algo = (CMPH_ALGO) (i);
      }
   }

   if (algo == CMPH_COUNT)
   {
      TRACE("Algorithm %s not found\n",algo_name);
      return NULL;
   }

   mphf = (cmph_t *) malloc(sizeof(cmph_t));
   mphf->algo = algo;
   nbytes = fread(&(mphf->size),sizeof(mphf->size),(size_t) 1,f);
   mphf->data = NULL;

   TRACE("Algorithm is %s and mphf is sized %u\n",cmph_names[algo],mphf->size);
   return mphf;
}
