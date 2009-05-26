#include <config.h>

#include "display.h"
#include "machine.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#include "pspui.h"

#include "video.h"
#include "pl_vk.h"
#include "pl_perf.h"

/* raw canvas width is 1 << 9 (512) */
#define CANVAS_WIDTH_SHIFTBY 9
#define CANVAS_WIDTH         (1 << CANVAS_WIDTH_SHIFTBY)

extern pl_vk_layout vk_spectrum;
PspImage *Screen = NULL;
static pl_perf_counter perf_counter;
static int disk_status, tape_status, mdr_status;
static int disk_icon_offset, tape_icon_offset, pc_offset, 
           pc_text_width, line_height;
int clear_screen;

static int ScreenX, ScreenY, ScreenW, ScreenH;

int uidisplay_init( int width, int height )
{
  /* Initialize screen buffer */
  if (!(Screen = pspImageCreateVram(CANVAS_WIDTH,
                                    DISPLAY_SCREEN_HEIGHT,
                                    PSP_IMAGE_INDEXED)))
      return 1;

  Screen->PalSize = 16;

  /* Reset status indicators */
  disk_status = tape_status = mdr_status = 0;
  disk_icon_offset = 0;
  tape_icon_offset = disk_icon_offset + 
    pspFontGetTextWidth(&PspStockFont, PSP_CHAR_FLOPPY);
  pc_offset = tape_icon_offset + 
    pspFontGetTextWidth(&PspStockFont, PSP_CHAR_TAPE);

  psp_uidisplay_reinit();

  return 0;
}

void psp_uidisplay_reinit()
{
  /* Reinitialize color palette */
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
  for (i = 0; i < Screen->PalSize; i++) 
  {
    unsigned char red, green, blue, grey;

    red   = rgb_colours[i][0];
    green = rgb_colours[i][1];
    blue  = rgb_colours[i][2];

    /* Addition of 0.5 is to avoid rounding errors */
    grey = ( 0.299 * red + 0.587 * green + 0.114 * blue ) + 0.5;

    Screen->Palette[i] = (!psp_options.enable_bw)
                           ? RGB(red, green, blue)
                           : RGB(grey, grey, grey);
  }

  if (!psp_menu_active)
  {
    if (psp_options.show_border)
    {
      Screen->Viewport.X = 0;
      Screen->Viewport.Y = 0;
      Screen->Viewport.Width = DISPLAY_SCREEN_WIDTH / 2;
      Screen->Viewport.Height = DISPLAY_SCREEN_HEIGHT;
    }
    else
    {
      Screen->Viewport.X = DISPLAY_BORDER_WIDTH / 2;
      Screen->Viewport.Y = DISPLAY_BORDER_HEIGHT;
      Screen->Viewport.Width = DISPLAY_WIDTH / 2;
      Screen->Viewport.Height = DISPLAY_HEIGHT;
    }
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

  /* Reset FPS counter */
  pl_perf_init_counter(&perf_counter);
  display_refresh_all();

  line_height = pspFontGetLineHeight(&PspStockFont);
  pc_text_width = 0;
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

  /* Black out the status line */
  if (psp_options.show_pc || psp_options.show_fps || psp_options.show_osi)
    pspVideoFillRect(pc_offset, 0, SCR_WIDTH, line_height, PSP_COLOR_BLACK);

  /* Render screen */
  pspVideoPutImage(Screen, ScreenX, ScreenY, ScreenW, ScreenH);

  /* Draw keyboard */
  if (keyboard_visible)
    pl_vk_render(&vk_spectrum);

  /* Show frames-per-second */
  if (psp_options.show_fps)
  {
    static char fps_display[32];
    sprintf(fps_display, " %3.02f", pl_perf_update_counter(&perf_counter));

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);
    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
  }

  /* Display any status indicators */
  if (psp_options.show_osi)
  {
    /* "Disk busy" indicator */
    if (disk_status)
      pspVideoPrint(&PspStockFont, disk_icon_offset, 0, PSP_CHAR_FLOPPY, PSP_COLOR_GREEN);
    if (tape_status)
      pspVideoPrint(&PspStockFont, tape_icon_offset, 0, PSP_CHAR_TAPE, PSP_COLOR_GREEN);
  }

  /* Display program counter */
  if (psp_options.show_pc)
  {
    static char pc_display[32];
    sprintf(pc_display, " %04X", PC);
    pc_text_width = pspVideoPrint(&PspStockFont, pc_offset, 0, pc_display, PSP_COLOR_RED);
  }

  pspVideoEnd();
  pspVideoSwapBuffers();
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
  u8 *screen_start = (u8*)Screen->Pixels;
  screen_start[(y << CANVAS_WIDTH_SHIFTBY) + x] = colour;
}

/* Print the 8 pixels in `data' using ink colour `ink' and paper
   colour `paper' to the screen at ( (8*x) , y ) */
void uidisplay_plot8(int x, int y, libspectrum_byte data,
                     libspectrum_byte ink, libspectrum_byte paper)
{
  x <<= 3;
  u8 *line_start = (u8*)Screen->Pixels +
                    ((y << CANVAS_WIDTH_SHIFTBY) + x);

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
  u8 *line_start = (u8*)Screen->Pixels +
                   ((y << CANVAS_WIDTH_SHIFTBY) + x);

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

int
ui_statusbar_update( ui_statusbar_item item, ui_statusbar_state state )
{
  switch (item) 
  {
  case UI_STATUSBAR_ITEM_DISK:
    disk_status = (state == UI_STATUSBAR_STATE_ACTIVE);
    return 0;
  case UI_STATUSBAR_ITEM_TAPE:
    tape_status = (state == UI_STATUSBAR_STATE_ACTIVE);
    return 0;
  case UI_STATUSBAR_ITEM_MICRODRIVE:
    mdr_status = (state == UI_STATUSBAR_STATE_ACTIVE);
    return 0;
  default: 
    return 0;
  }

  return 0;
}

int
ui_statusbar_update_speed( float speed )
{
  return 0;
}
