/* joystick.h: Joystick emulation support
   Copyright (c) 2001-2004 Russell Marks, Philip Kendall
   Copyright (c) 2003 Darren Salt

   $Id: joystick.h 2993 2007-06-17 13:54:49Z pak21 $

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

#ifndef FUSE_JOYSTICK_H
#define FUSE_JOYSTICK_H

#include <libspectrum.h>

/* Number of joysticks known about & initialised */
extern int joysticks_supported;

/* Init/shutdown functions. Errors aren't important here */
void fuse_joystick_init( void );
void fuse_joystick_end( void );

/* A constant to identify the joystick emulated via the keyboard */
#define JOYSTICK_KEYBOARD 2

typedef enum joystick_type_t {

  JOYSTICK_TYPE_NONE = 0,
  
  JOYSTICK_TYPE_CURSOR,
  JOYSTICK_TYPE_KEMPSTON,
  JOYSTICK_TYPE_SINCLAIR_1,
  JOYSTICK_TYPE_SINCLAIR_2,
  JOYSTICK_TYPE_TIMEX_1,
  JOYSTICK_TYPE_TIMEX_2,

} joystick_type_t;

#define JOYSTICK_TYPE_COUNT 7

extern const char *joystick_name[];

typedef enum joystick_button {

  JOYSTICK_BUTTON_LEFT = 0,
  JOYSTICK_BUTTON_RIGHT,
  JOYSTICK_BUTTON_UP,
  JOYSTICK_BUTTON_DOWN,
  JOYSTICK_BUTTON_FIRE,

} joystick_button;

/* Called whenever the (Spectrum) joystick is moved or the fire button
   pressed */
int joystick_press( int which, joystick_button button, int press );
/* Resets joystick state */
int joystick_release_all( int which );

/* Interface-specific read functions */
libspectrum_byte joystick_kempston_read ( libspectrum_word port,
					  int *attached );
libspectrum_byte joystick_timex_read ( libspectrum_word port,
				       libspectrum_byte which );

#endif			/* #ifndef FUSE_JOYSTICK_H */
