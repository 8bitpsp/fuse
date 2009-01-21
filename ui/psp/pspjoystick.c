#include <psptypes.h>
#include <pspkernel.h>
#include <psprtc.h>

#include "psp_aux/libpsp/ctrl.h"

#include "pspui.h"
#include "input.h"

int
ui_joystick_init( void )
{
  return 1;
}

void
ui_joystick_end( void )
{
}

void
ui_joystick_poll( void )
{
  /* Polling is done in ui_event */
}
