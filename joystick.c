/* joystick.c: Joystick emulation support
   Copyright (c) 2001-2004 Russell Marks, Philip Kendall
   Copyright (c) 2003 Darren Salt

   $Id: joystick.c 3389 2007-12-03 12:54:17Z fredm $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <libspectrum.h>

#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "module.h"
#include "periph.h"
#include "rzx.h"
#include "settings.h"
#include "spectrum.h"
#include "machine.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"

/* Number of joysticks known about & initialised */
int joysticks_supported = 0;

/* The bit masks used by the various joysticks. The order is the same
   as the ordering of buttons in joystick.h:joystick_button (left,
   right, up, down, fire ) */
static const libspectrum_byte kempston_mask[5] =
  { 0x02, 0x01, 0x08, 0x04, 0x10 };
static const libspectrum_byte timex_mask[5] =
  { 0x04, 0x08, 0x01, 0x02, 0x80 };

/* The keys used by the Cursor joystick */
static const keyboard_key_name cursor_key[5] =
  { KEYBOARD_5, KEYBOARD_8, KEYBOARD_7, KEYBOARD_6, KEYBOARD_0 };

/* The keys used by the two Sinclair joysticks */
static const keyboard_key_name sinclair1_key[5] =
  { KEYBOARD_6, KEYBOARD_7, KEYBOARD_9, KEYBOARD_8, KEYBOARD_0 };
static const keyboard_key_name sinclair2_key[5] =
  { KEYBOARD_1, KEYBOARD_2, KEYBOARD_4, KEYBOARD_3, KEYBOARD_5 };

/* The current values for the joysticks we can emulate */
static libspectrum_byte kempston_value;
static libspectrum_byte timex1_value;
static libspectrum_byte timex2_value;

/* The names of the joysticks we can emulate. Order must correspond to
   that of joystick.h:joystick_type_t */
const char *joystick_name[ JOYSTICK_TYPE_COUNT ] = {
  "None",
  "Cursor",
  "Kempston",
  "Sinclair 1", "Sinclair 2",
  "Timex 1", "Timex 2"
};

static void joystick_from_snapshot( libspectrum_snap *snap );
static void joystick_to_snapshot( libspectrum_snap *snap );

static module_info_t joystick_module_info = {

  NULL,
  NULL,
  NULL,
  joystick_from_snapshot,
  joystick_to_snapshot,

};

/* Init/shutdown functions. Errors aren't important here */

void
fuse_joystick_init (void)
{
  joysticks_supported = ui_joystick_init();
  kempston_value = timex1_value = timex2_value = 0x00;

  module_register( &joystick_module_info );
}

void
fuse_joystick_end (void)
{
  ui_joystick_end();
}

int
joystick_release_all( int which )
{
  joystick_type_t type;

  switch( which ) {
  case 0: type = settings_current.joystick_1_output; break;
  case 1: type = settings_current.joystick_2_output; break;

  case JOYSTICK_KEYBOARD:
    type = settings_current.joystick_keyboard_output; break;

  default:
    return 0;
  }

  switch( type ) {
  case JOYSTICK_TYPE_KEMPSTON:
    kempston_value = 0x00;
    return 1;

  case JOYSTICK_TYPE_TIMEX_1:
    timex1_value = 0x00;
    return 1;

  case JOYSTICK_TYPE_TIMEX_2:
    timex2_value = 0x00;
    return 1;

  default: return 0;
  }
}

int
joystick_press( int which, joystick_button button, int press )
{
  joystick_type_t type;

  switch( which ) {
  case 0: type = settings_current.joystick_1_output; break;
  case 1: type = settings_current.joystick_2_output; break;

  case JOYSTICK_KEYBOARD:
    type = settings_current.joystick_keyboard_output; break;

  default:
    return 0;
  }

  switch( type ) {

  case JOYSTICK_TYPE_CURSOR:
    if( press ) {
      keyboard_press( cursor_key[ button ] );
    } else {
      keyboard_release( cursor_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_KEMPSTON:
    if( press ) {
      kempston_value |=  kempston_mask[ button ];
    } else {
      kempston_value &= ~kempston_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_SINCLAIR_1:
    if( press ) {
      keyboard_press( sinclair1_key[ button ] );
    } else {
      keyboard_release( sinclair1_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_SINCLAIR_2:
    if( press ) {
      keyboard_press( sinclair2_key[ button ] );
    } else {
      keyboard_release( sinclair2_key[ button ] );
    }
    return 1;

  case JOYSTICK_TYPE_TIMEX_1:
    if( press ) {
      timex1_value |=  timex_mask[ button ];
    } else {
      timex1_value &= ~timex_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_TIMEX_2:
    if( press ) {
      timex2_value |=  timex_mask[ button ];
    } else {
      timex2_value &= ~timex_mask[ button ];
    }
    return 1;

  case JOYSTICK_TYPE_NONE: return 0;
  }

  ui_error( UI_ERROR_ERROR, "%s:joystick_press:unknown joystick type %d",
	    __FILE__, type );
  fuse_abort();
}

/* Read functions for specific interfaces */

libspectrum_byte
joystick_kempston_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !periph_kempston_active ) return 0xff;

  *attached = 1;

  return kempston_value;
}

libspectrum_byte
joystick_timex_read( libspectrum_word port GCC_UNUSED, libspectrum_byte which )
{
  return which ? timex2_value : timex1_value;
}

static void
joystick_from_snapshot( libspectrum_snap *snap )
{
  size_t i;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  joystick_type_t fuse_type;

  for( i = 0; i < num_joysticks; i++ ) {
    switch( libspectrum_snap_joystick_list( snap, i ) ) {
    case LIBSPECTRUM_JOYSTICK_CURSOR:
      fuse_type = JOYSTICK_TYPE_CURSOR;
      break;
    case LIBSPECTRUM_JOYSTICK_KEMPSTON:            
      fuse_type = JOYSTICK_TYPE_KEMPSTON;
      break;
    case LIBSPECTRUM_JOYSTICK_SINCLAIR_1:
      fuse_type = JOYSTICK_TYPE_SINCLAIR_1;
      break;
    case LIBSPECTRUM_JOYSTICK_SINCLAIR_2:           
      fuse_type = JOYSTICK_TYPE_SINCLAIR_2;
      break;
    case LIBSPECTRUM_JOYSTICK_TIMEX_1:
      fuse_type = JOYSTICK_TYPE_TIMEX_1;
      break;
    case LIBSPECTRUM_JOYSTICK_TIMEX_2:
      fuse_type = JOYSTICK_TYPE_TIMEX_2;
      break;
    case LIBSPECTRUM_JOYSTICK_FULLER:
    default:
      ui_error( UI_ERROR_INFO, "Ignoring unsupported joystick in snapshot %s", 
        libspectrum_joystick_name( libspectrum_snap_joystick_list( snap, i ) ));
      continue;
    };

    if( settings_current.joystick_keyboard_output != fuse_type &&
        settings_current.joystick_1_output != fuse_type &&
        settings_current.joystick_2_output != fuse_type &&
        !rzx_playback ) {
      switch( ui_confirm_joystick( libspectrum_snap_joystick_list(snap,i),
                                   libspectrum_snap_joystick_inputs(snap,i)) ) {
      case UI_CONFIRM_JOYSTICK_KEYBOARD:
        settings_current.joystick_keyboard_output = fuse_type;
        break;
      case UI_CONFIRM_JOYSTICK_JOYSTICK_1:
        settings_current.joystick_1_output = fuse_type;
        break;
      case UI_CONFIRM_JOYSTICK_JOYSTICK_2:
        settings_current.joystick_2_output = fuse_type;
        break;
      case UI_CONFIRM_JOYSTICK_NONE:
        break;
      }
    }

    /* If the snap was configured for a Kempston joystick, enable
       our Kempston emulation in case the snap was reading from
       the joystick to prevent things going haywire */
    if( fuse_type == JOYSTICK_TYPE_KEMPSTON )
      settings_current.joy_kempston = 1;
  }
}

static void
add_joystick( libspectrum_snap *snap, joystick_type_t fuse_type, int inputs )
{
  size_t i;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  libspectrum_joystick libspectrum_type;

  switch( fuse_type ) {
  case JOYSTICK_TYPE_CURSOR:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_CURSOR;
    break;
  case JOYSTICK_TYPE_KEMPSTON:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_KEMPSTON;
    break;
  case JOYSTICK_TYPE_SINCLAIR_1:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_SINCLAIR_1;
    break;
  case JOYSTICK_TYPE_SINCLAIR_2:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_SINCLAIR_2;
    break;
  case JOYSTICK_TYPE_TIMEX_1:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_TIMEX_1;
    break;
  case JOYSTICK_TYPE_TIMEX_2:
    libspectrum_type = LIBSPECTRUM_JOYSTICK_TIMEX_2;
    break;
  
  case JOYSTICK_TYPE_NONE:
  default:
    return;
    break;
  }

  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_list( snap, i ) == libspectrum_type ) {
      libspectrum_snap_set_joystick_inputs( snap, i, inputs |
                                  libspectrum_snap_joystick_inputs( snap, i ) );
      return;
    }
  }

  libspectrum_snap_set_joystick_list( snap, num_joysticks, libspectrum_type );
  libspectrum_snap_set_joystick_inputs( snap, num_joysticks, inputs );
  libspectrum_snap_set_joystick_active_count( snap, num_joysticks + 1 );
}

static void
joystick_to_snapshot( libspectrum_snap *snap )
{
  if( settings_current.joy_kempston ) {
    add_joystick( snap, JOYSTICK_TYPE_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_NONE );
  }
  add_joystick( snap, settings_current.joystick_keyboard_output,
                LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
  add_joystick( snap, settings_current.joystick_1_output,
                LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
  add_joystick( snap, settings_current.joystick_2_output,
                LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
}
