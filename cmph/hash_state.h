#ifndef __HASH_STATE_H__
#define __HASH_STATE_H__

union __hash_state_t
{
   CMPH_HASH         hashfunc;
   jenkins_state_t   jenkins;
};

#endif
