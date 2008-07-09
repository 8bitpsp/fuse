#include <config.h>

#include <audio.h>
#include <pspkernel.h>

#include "fuse.h"
#include "settings.h"
#include "sound/sfifo.h"
#include "sound.h"
#include "ui/ui.h"

sfifo_t sound_fifo;
#define NUM_FRAMES 2

void psp_sound_pause();
void psp_sound_resume();

void AudioCallback(void* buf, unsigned int *length, void *userdata)
{
  sfifo_read( &sound_fifo, buf, (*length << 1));
}

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  float hz = (float)machine_current->timings.processor_speed /
                    machine_current->timings.tstates_per_frame;
  int sound_framesiz = *freqptr / hz;
  int channels = (*stereoptr) ? 2 : 1;

  if (sfifo_init(&sound_fifo, NUM_FRAMES * channels * sound_framesiz + 1))
    return 1;

  return 0;
}

void
sound_lowlevel_end( void )
{
  psp_sound_pause();
  sfifo_flush( &sound_fifo );
  sfifo_close( &sound_fifo );
}

void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  int i = 0;

  /* Convert to bytes */
  libspectrum_signed_byte* bytes = (libspectrum_signed_byte*)data;
  len <<= 1;

  while( len ) {
    if( ( i = sfifo_write( &sound_fifo, bytes, len ) ) < 0 ) {
      break;
    } else if (!i) {
      sceKernelDelayThread(10);
    }
    bytes += i;
    len -= i;
  }
}

void psp_sound_pause()
{
  pspAudioSetChannelCallback(0, NULL, 0);
}

void psp_sound_resume()
{
  pspAudioSetChannelCallback(0, AudioCallback, 0);
}
