#include <config.h>

#include <pspkernel.h>

#include "timer/timer.h"

int
timer_get_real_time( timer_type *real_time )
{
  sceRtcGetCurrentTick(real_time);
  return 0;
}

float
timer_get_time_difference( timer_type *a, timer_type *b )
{
  return ( *a - *b ) / (float)sceRtcGetTickResolution();
}

void
timer_add_time_difference( timer_type *a, long msec )
{
  *a += (msec * (sceRtcGetTickResolution() / 1000));
}

void
timer_sleep_ms( int ms )
{
  sceKernelDelayThread(ms * (sceRtcGetTickResolution() / 1000));
}
