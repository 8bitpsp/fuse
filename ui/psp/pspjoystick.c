#include <psptypes.h>
#include <pspkernel.h>
#include <psprtc.h>

#include "ui/psp/lib/ctrl.h"

#include "pspui.h"
#include "input.h"

extern SceCtrlData psp_pad_status;

static inline void psp_joystick_toggle(unsigned int code, int on);

static inline void psp_joystick_toggle(unsigned int code, int on)
{
  input_event_t fuse_event;

  fuse_event.type = (on)
    ? INPUT_EVENT_JOYSTICK_PRESS : INPUT_EVENT_JOYSTICK_RELEASE;
  fuse_event.types.joystick.which = 0;
  fuse_event.types.joystick.button = code;

  input_event(&fuse_event);
}

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
  /* Check the input */
  pspCtrlPollControls(&psp_pad_status);

  /* Parse input */
  psp_ctrl_mask_to_index_map_t *current_mapping = physical_to_emulated_button_map;
  for (; current_mapping->mask; current_mapping++)
  {
    u32 code = current_map.button_map[current_mapping->index];
    u8  on = (psp_pad_status.Buttons & current_mapping->mask) == current_mapping->mask;

    if (code & JST)
    {
      psp_joystick_toggle(CODE_MASK(code), on);

      /* Check to see if a button set is pressed. If so, unset it, so it */
      /* doesn't trigger any other combination presses. */
      if (on) psp_pad_status.Buttons &= ~current_mapping->mask;
    }
  }
}
