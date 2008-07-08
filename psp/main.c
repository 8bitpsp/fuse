#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "fuse.h"
#include "audio.h"
#include "video.h"
#include "psp.h"
#include "ctrl.h"
#include "adhoc.h"

PSP_MODULE_INFO(PSP_APP_NAME, 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

extern int fuse_exiting;

static void ExitCallback(void* arg)
{
  fuse_exiting = 1;
  ExitPSP = 1;
}

int main(int argc, char *argv[])
{
  pspAdhocLoadDrivers();

  /* Initialize PSP */
  pspInit(argv[0]);
  pspAudioInit(SOUND_BUFFER_SIZE, 0);
  pspCtrlInit();
  pspVideoInit();

  /* Initialize callbacks */
  pspRegisterCallback(PSP_EXIT_CALLBACK, ExitCallback, NULL);
  pspStartCallbackThread();

  fuse_main(argc, argv);

  /* Start emulation */
/*
  InitMenu();
  DisplayMenu();
  TrashMenu();
*/
  /* Release PSP resources */
  pspAudioShutdown();
  pspVideoShutdown();
  pspAdhocShutdown();
  pspShutdown();

  return(0);
}
