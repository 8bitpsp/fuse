/* psplib/pl_gfx.h
   Graphics routines

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

#ifndef _PL_GFX_H
#define _PL_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

#define PL_GFX_SCREEN_WIDTH  480
#define PL_GFX_SCREEN_HEIGHT 272

int   pl_gfx_init();
void  pl_gfx_shutdown();

void* pl_gfx_vram_alloc(unsigned int bytes);

#ifdef __cplusplus
}
#endif

#endif  // PL_GFX_H
