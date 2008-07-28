#include <config.h>

#include "display.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

#include <pspkernel.h>

#include "pspui.h"

#include "video.h"
#include "pl_vk.h"
#include "pl_perf.h"

extern pl_vk_layout vk_spectrum;
pl_image plScreen;
static pl_perf_counter perf_counter;
int clear_screen;
static int ScreenX, ScreenY, ScreenW, ScreenH;
static const unsigned char spectrum_palette[16][3] = 
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


int uidisplay_init( int width, int height )
{
  /* Initialize screen buffer */
  if (!pl_image_create(&plScreen,
                       DISPLAY_SCREEN_WIDTH / 2,
                       DISPLAY_SCREEN_HEIGHT,
                       pl_image_indexed,
                       PL_IMAGE_USE_VRAM))
      return 1;

  if (!pl_image_palettize(&plScreen,
                          pl_image_5551,
                          16))
  {
    pl_image_destroy(&plScreen);
    return 1;
  }

  psp_uidisplay_reinit();

  return 0;
}

void psp_uidisplay_reinit()
{
  /* Reset palette (color/gray) */
  int i;
  for (i = 0; i < 16; i++) 
  {
    unsigned char red, green, blue, grey;

    red   = spectrum_palette[i][0];
    green = spectrum_palette[i][1];
    blue  = spectrum_palette[i][2];

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

    if (psp_options.enable_bw)
      red = green = blue = grey;

    pl_image_set_palette_color(&plScreen,
                               i,
                               RGB(red, green, blue));
  }

  /* Reinitialize view */
  if (psp_options.show_border)
  {
    plScreen.view.x = 0;
    plScreen.view.y = 0;
    plScreen.view.w = DISPLAY_SCREEN_WIDTH / 2;
    plScreen.view.h = DISPLAY_SCREEN_HEIGHT;
  }
  else
  {
    plScreen.view.x = DISPLAY_BORDER_WIDTH / 2;
    plScreen.view.y = DISPLAY_BORDER_HEIGHT;
    plScreen.view.w = DISPLAY_WIDTH / 2;
    plScreen.view.h = DISPLAY_HEIGHT;
  }

  /* Set up viewing ratios */
  float ratio;
  switch (psp_options.display_mode)
  {
  default:
  case DISPLAY_MODE_UNSCALED:
    ScreenW = plScreen.view.w;
    ScreenH = plScreen.view.h;
    break;
  case DISPLAY_MODE_FIT_HEIGHT:
    ratio = (float)SCR_HEIGHT / (float)plScreen.view.h;
    ScreenW = (float)plScreen.view.w * ratio - 2;
    ScreenH = SCR_HEIGHT;
    break;
  case DISPLAY_MODE_FILL_SCREEN:
    ScreenW = SCR_WIDTH - 3;
    ScreenH = SCR_HEIGHT;
    break;
  }

  ScreenX = (SCR_WIDTH / 2) - (ScreenW / 2);
  ScreenY = (SCR_HEIGHT / 2) - (ScreenH / 2);

  /* Reset FPS counter */
  pl_perf_init_counter(&perf_counter);

  display_refresh_all();
}

void uidisplay_frame_end()
{
  /* No drawing if the menu is currently active */
  if (psp_menu_active)
    return;

  pspVideoBegin();

  /* Clear the buffer first, if necessary */
  if (clear_screen >= 0)
  {
    clear_screen--;
    pspVideoClearScreen();
  }

  pl_video_put_image(&plScreen, ScreenX, ScreenY, ScreenW, ScreenH);

  /* Draw keyboard */
  if (show_kybd_held)
    pl_vk_render(&vk_spectrum);

  if (psp_options.show_fps)
  {
    static char fps_display[32];
    sprintf(fps_display, " %3.02f", pl_perf_update_counter(&perf_counter));

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    int height = pspFontGetLineHeight(&PspStockFont);

    pspVideoFillRect(SCR_WIDTH - width, 0, SCR_WIDTH, height, PSP_COLOR_BLACK);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
  }

  pspVideoEnd();
  pspVideoSwapBuffers();
}

int uidisplay_end()
{
  pl_image_destroy(&plScreen);
  return 0;
}

/* Set one pixel in the display */
void uidisplay_putpixel(int x, int y, int colour)
{
  u8 *pixel = (u8*)plScreen.bitmap +
    (y * plScreen.pitch + x);
  *pixel = colour;
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void uidisplay_plot8(int x, int y, libspectrum_byte data,
                     libspectrum_byte ink, libspectrum_byte paper)
{
  x <<= 3;
  u8 *line_start = (u8*)plScreen.bitmap +
                    (y * plScreen.pitch + x);

  *line_start++ = ( data & 0x80 ) ? ink : paper;
  *line_start++ = ( data & 0x40 ) ? ink : paper;
  *line_start++ = ( data & 0x20 ) ? ink : paper;
  *line_start++ = ( data & 0x10 ) ? ink : paper;
  *line_start++ = ( data & 0x08 ) ? ink : paper;
  *line_start++ = ( data & 0x04 ) ? ink : paper;
  *line_start++ = ( data & 0x02 ) ? ink : paper;
  *line_start++ = ( data & 0x01 ) ? ink : paper;
}

/* Print the 16 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (16*x) , y ) */
void uidisplay_plot16(int x, int y, libspectrum_word data,
                      libspectrum_byte ink, libspectrum_byte paper)
{
  /* Forces a low-res render, discarding every other pixel */
  x <<= 4;
  u8 *line_start = (u8*)plScreen.bitmap +
                   (y * plScreen.pitch + x);

  *line_start++ = ( data & 0x8000 ) ? ink : paper;
  *line_start++ = ( data & 0x2000 ) ? ink : paper;
  *line_start++ = ( data & 0x0800 ) ? ink : paper;
  *line_start++ = ( data & 0x0200 ) ? ink : paper;
  *line_start++ = ( data & 0x0080 ) ? ink : paper;
  *line_start++ = ( data & 0x0020 ) ? ink : paper;
  *line_start++ = ( data & 0x0008 ) ? ink : paper;
  *line_start++ = ( data & 0x0002 ) ? ink : paper;
}

void uidisplay_area(int x, int y, int w, int h)
{
}

int uidisplay_hotswap_gfx_mode()
{
  display_refresh_all();
  return 0;
}
