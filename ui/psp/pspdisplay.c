#include <config.h>

#include "display.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "input.h" /* TODO: TEMP */

/* TODO */
#include "pspui.h"
#include "ctrl.h"
#include "util.h"
#include "perf.h"
#include "video.h"

PspImage *Screen = NULL;

static int ScreenX, ScreenY, ScreenW, ScreenH;
static PspFpsCounter FpsCounter;

int uidisplay_init( int width, int height )
{
  /* Initialize screen buffer */
  if (!(Screen = pspImageCreateVram(512, DISPLAY_SCREEN_HEIGHT,
    PSP_IMAGE_INDEXED)))
      return 1;

  Screen->Viewport.X = DISPLAY_BORDER_WIDTH / 2;
  Screen->Viewport.Y = DISPLAY_BORDER_HEIGHT;
  Screen->Viewport.Width = DISPLAY_WIDTH / 2;
  Screen->Viewport.Height = DISPLAY_HEIGHT;
  Screen->PalSize = 16;

  /* Initialize color palette */
  const unsigned char rgb_colours[16][3] = 
  {
    {   0,   0,   0 },
    {   0,   0, 192 },
    { 192,   0,   0 },
    { 192,   0, 192 },
    {   0, 192,   0 },
    {   0, 192, 192 },
    { 192, 192,   0 },
    { 192, 192, 192 },
    {   0,   0,   0 },
    {   0,   0, 255 },
    { 255,   0,   0 },
    { 255,   0, 255 },
    {   0, 255,   0 },
    {   0, 255, 255 },
    { 255, 255,   0 },
    { 255, 255, 255 },
  };

  int i;
  for (i = 0; i < 16; i++) 
  {
    unsigned char red, green, blue, grey;

    red   = rgb_colours[i][0];
    green = rgb_colours[i][1];
    blue  = rgb_colours[i][2];

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

    Screen->Palette[i] = RGB(red, green, blue);
  }

  /* Set up viewing ratios */
  float ratio;
  switch (psp_options.display_mode)
  {
  default:
  case DISPLAY_MODE_UNSCALED:
    ScreenW = Screen->Viewport.Width;
    ScreenH = Screen->Viewport.Height;
    break;
  case DISPLAY_MODE_FIT_HEIGHT:
    ratio = (float)SCR_HEIGHT / (float)Screen->Viewport.Height;
    ScreenW = (float)Screen->Viewport.Width * ratio - 2;
    ScreenH = SCR_HEIGHT;
    break;
  case DISPLAY_MODE_FILL_SCREEN:
    ScreenW = SCR_WIDTH - 3;
    ScreenH = SCR_HEIGHT;
    break;
  }

  ScreenX = (SCR_WIDTH / 2) - (ScreenW / 2);
  ScreenY = (SCR_HEIGHT / 2) - (ScreenH / 2);

  /* Init performance counter */
  pspPerfInitFps(&FpsCounter);

  display_refresh_all();
  return 0;
}

void uidisplay_frame_end()
{
  pspVideoBegin();
  pspVideoPutImage(Screen, ScreenX, ScreenY, ScreenW, ScreenH);

  if (psp_options.show_fps)
  {
    static char fps_display[32];
    sprintf(fps_display, " %3.02f", pspPerfGetFps(&FpsCounter));

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    int height = pspFontGetLineHeight(&PspStockFont);

    pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_COLOR_BLACK);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
  }

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
    if (pad.Buttons & PSP_CTRL_START)
      keyboard_press(INPUT_KEY_0);
    else
      keyboard_release(INPUT_KEY_0);
    if (pad.Buttons & PSP_CTRL_UP)
      keyboard_press(INPUT_KEY_p);
    else
      keyboard_release(INPUT_KEY_p);
    if (pad.Buttons & PSP_CTRL_DOWN)
      keyboard_press(INPUT_KEY_l);
    else
      keyboard_release(INPUT_KEY_l);
    if (pad.Buttons & PSP_CTRL_LEFT)
      keyboard_press(INPUT_KEY_q);
    else
      keyboard_release(INPUT_KEY_q);
    if (pad.Buttons & PSP_CTRL_RIGHT)
      keyboard_press(INPUT_KEY_w);
    else
      keyboard_release(INPUT_KEY_w);
    if (pad.Buttons & PSP_CTRL_SQUARE)
      keyboard_press(INPUT_KEY_space);
    else
      keyboard_release(INPUT_KEY_space);
  }
}

int uidisplay_end()
{
  if (Screen) 
  {
    pspImageDestroy(Screen);
    Screen = NULL;
  }

  return 0;
}

/* Set one pixel in the display */
void uidisplay_putpixel(int x, int y, int colour)
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
void uidisplay_plot8(int x, int y, libspectrum_byte data,
                     libspectrum_byte ink, libspectrum_byte paper)
{
  x <<= 3;

  if (machine_current->timex) {
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
void uidisplay_plot16(int x, int y, libspectrum_word data,
                      libspectrum_byte ink, libspectrum_byte paper)
{
  int i;
  x <<= 4; y <<= 1;

  u8 *line_start;

  for (i = 0; i < 2; i++, y++)
  {
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

void uidisplay_area(int x, int y, int w, int h)
{
}

int uidisplay_hotswap_gfx_mode()
{
  display_refresh_all();
  return 0;
}
