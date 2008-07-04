#include <config.h>

#include "display.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

#include "video.h"

/* TODO */
#include "ctrl.h"
#include "util.h"

extern PspImage *Screen;

/* The size of a 1x1 image in units of
   DISPLAY_ASPECT WIDTH x DISPLAY_SCREEN_HEIGHT */
int image_scale;

/* The height and width of a 1x1 image in pixels */
int image_width, image_height;

int
uidisplay_init( int width, int height )
{
  image_width = width; image_height = height;
  image_scale = width / DISPLAY_ASPECT_WIDTH;

  display_refresh_all();
  return 0;
}

void uidisplay_frame_end()
{
  pspVideoBegin();
  pspVideoPutImage(Screen, 0, 0, Screen->Viewport.Width, Screen->Viewport.Height);
  pspVideoEnd();
  pspVideoSwapBuffers();
  static SceCtrlData pad;

  /* Check the input */
  if (pspCtrlPollControls(&pad))
  {
#ifdef PSP_DEBUG
    if ((pad.Buttons & (PSP_CTRL_SELECT | PSP_CTRL_START))
      == (PSP_CTRL_SELECT | PSP_CTRL_START))
        pspUtilSaveVramSeq("", "game");
#endif
  }

}

void
uidisplay_area( int x, int y, int w, int h )
{
/*
  Screen->Viewport.X = x;
  Screen->Viewport.Y = y;
  Screen->Viewport.Width = w;
  Screen->Viewport.Height = h;
*/
}

int
uidisplay_hotswap_gfx_mode( void )
{
  /* Redraw the entire screen... */
  display_refresh_all();

  return 0;
}

int
uidisplay_end( void )
{
  return 0;
}

/* Set one pixel in the display */
void
uidisplay_putpixel( int x, int y, int colour )
{
  if( machine_current->timex ) {
/* TODO 
    x <<= 1; y <<= 1;
    win32display_image[y  ][x  ] = colour;
    win32display_image[y  ][x+1] = colour;
    win32display_image[y+1][x  ] = colour;
    win32display_image[y+1][x+1] = colour;
*/
  } else {
    u8 *screen_start = (u8*)Screen->Pixels;
    screen_start[y * Screen->Width + x] = colour;
  }
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void
uidisplay_plot8( int x, int y, libspectrum_byte data,
		 libspectrum_byte ink, libspectrum_byte paper )
{
  x <<= 3;

  if( machine_current->timex ) {
/* TODO
    int i;

    x <<= 1; y <<= 1;
    for( i=0; i<2; i++,y++ ) {
      win32display_image[y][x+ 0] = ( data & 0x80 ) ? ink : paper;
      win32display_image[y][x+ 1] = ( data & 0x80 ) ? ink : paper;
      win32display_image[y][x+ 2] = ( data & 0x40 ) ? ink : paper;
      win32display_image[y][x+ 3] = ( data & 0x40 ) ? ink : paper;
      win32display_image[y][x+ 4] = ( data & 0x20 ) ? ink : paper;
      win32display_image[y][x+ 5] = ( data & 0x20 ) ? ink : paper;
      win32display_image[y][x+ 6] = ( data & 0x10 ) ? ink : paper;
      win32display_image[y][x+ 7] = ( data & 0x10 ) ? ink : paper;
      win32display_image[y][x+ 8] = ( data & 0x08 ) ? ink : paper;
      win32display_image[y][x+ 9] = ( data & 0x08 ) ? ink : paper;
      win32display_image[y][x+10] = ( data & 0x04 ) ? ink : paper;
      win32display_image[y][x+11] = ( data & 0x04 ) ? ink : paper;
      win32display_image[y][x+12] = ( data & 0x02 ) ? ink : paper;
      win32display_image[y][x+13] = ( data & 0x02 ) ? ink : paper;
      win32display_image[y][x+14] = ( data & 0x01 ) ? ink : paper;
      win32display_image[y][x+15] = ( data & 0x01 ) ? ink : paper;
    }
*/
  } else {
    u8 *line_start = (u8*)Screen->Pixels;
    line_start += y * Screen->Width + x;

    *line_start++ = ( data & 0x80 ) ? ink : paper;
    *line_start++ = ( data & 0x40 ) ? ink : paper;
    *line_start++ = ( data & 0x20 ) ? ink : paper;
    *line_start++ = ( data & 0x10 ) ? ink : paper;
    *line_start++ = ( data & 0x08 ) ? ink : paper;
    *line_start++ = ( data & 0x04 ) ? ink : paper;
    *line_start++ = ( data & 0x02 ) ? ink : paper;
    *line_start++ = ( data & 0x01 ) ? ink : paper;
  }
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void
uidisplay_plot16( int x, int y, libspectrum_word data,
		  libspectrum_byte ink, libspectrum_byte paper )
{
  int i;
  x <<= 4; y <<= 1;

  u8 *line_start;

  for( i=0; i<2; i++,y++ ) {
    line_start = (u8*)Screen->Pixels;
    line_start += y * Screen->Width + x;

    *line_start++ = ( data & 0x8000 ) ? ink : paper;
    *line_start++ = ( data & 0x4000 ) ? ink : paper;
    *line_start++ = ( data & 0x2000 ) ? ink : paper;
    *line_start++ = ( data & 0x1000 ) ? ink : paper;
    *line_start++ = ( data & 0x0800 ) ? ink : paper;
    *line_start++ = ( data & 0x0400 ) ? ink : paper;
    *line_start++ = ( data & 0x0200 ) ? ink : paper;
    *line_start++ = ( data & 0x0100 ) ? ink : paper;
    *line_start++ = ( data & 0x0080 ) ? ink : paper;
    *line_start++ = ( data & 0x0040 ) ? ink : paper;
    *line_start++ = ( data & 0x0020 ) ? ink : paper;
    *line_start++ = ( data & 0x0010 ) ? ink : paper;
    *line_start++ = ( data & 0x0008 ) ? ink : paper;
    *line_start++ = ( data & 0x0004 ) ? ink : paper;
    *line_start++ = ( data & 0x0002 ) ? ink : paper;
    *line_start++ = ( data & 0x0001 ) ? ink : paper;
  }
}
