/** PSP helper library ***************************************/
/**                                                         **/
/**                       pl_perf.h                         **/
/**                                                         **/
/** This file contains declarations for performance-related **/
/** PSP routines                                            **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PL_PERF_H
#define _PL_PERF_H

#include <psptypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pl_perf_counter_t
{
  float ticks_per_second;
  int frame_count;
  u64 last_tick;
  float fps;
} pl_perf_counter;

void  pl_perf_init_counter(pl_perf_counter *counter);
float pl_perf_update_counter(pl_perf_counter *counter);

#ifdef __cplusplus
}
#endif

#endif // _PL_PERF_H
