/** PSP helper library ***************************************/
/**                                                         **/
/**                       pl_perf.c                         **/
/**                                                         **/
/** This file contains performance-related PSP routines     **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include <pspkernel.h>
#include <psprtc.h>

#include "pl_perf.h"

void  pl_perf_init_counter(pl_perf_counter *counter)
{
  counter->fps = 0;
  counter->frame_count = 0;
  counter->ticks_per_second = (float)sceRtcGetTickResolution();
  sceRtcGetCurrentTick(&counter->last_tick);
}

float pl_perf_update_counter(pl_perf_counter *counter)
{
  u64 current_tick;
  sceRtcGetCurrentTick(&current_tick);

  counter->frame_count++;
  if (current_tick - counter->last_tick >= 
      counter->ticks_per_second)
  {
    /* A second elapsed; recompute FPS */
    counter->fps = (float)counter->frame_count 
      / (float)((current_tick - counter->last_tick) / counter->ticks_per_second);
    counter->last_tick = current_tick;
    counter->frame_count = 0;
  }

  return counter->fps;
}
