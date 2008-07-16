/* psplib/audio.h
   Simple sound playback library

   Copyright (C) 2007-2008 Akop Karapetyan

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: pspdev@akop.org
*/

#ifndef _PSP_AUDIO_H
#define _PSP_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_AUDIO_SAMPLE_ALIGN(s) (((s) + 63) & ~63)
#define PSP_AUDIO_SAMPLE_TRUNCATE(s) ((s) & ~63)
#define PSP_AUDIO_MAX_VOLUME      0x8000

typedef struct 
{
  short Left;
  short Right;
} PspStereoSample;

typedef struct 
{
  short Channel;
} PspMonoSample;

typedef void (*pspAudioCallback)(void *buffer, unsigned int *sample_count, void *userdata);

int  pspAudioInit(int sample_count, int stereo);
void pspAudioSetVolume(int channel, int left, int right);
void pspAudioSetChannelCallback(int channel, pspAudioCallback callback, void *userdata);
void pspAudioShutdown();
int  pspAudioOutputBlocking(void *buf, unsigned int length);
int  pspAudioOutput(void *buf, unsigned int length);
int  pspAudioGetSampleCount();

#ifdef __cplusplus
}
#endif

#endif // _PSP_AUDIO_H

