#include <config.h>

#include "debugger/debugger.h"
#include "display.h"
#include "fuse.h"
#include "menu.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "timer/timer.h"
#include "ui/ui.h"
#include "ui/uijoystick.h"
#include "utils.h"

#include "video.h"

PspImage *Screen = NULL;

int
ui_init( int *argc, char ***argv )
{
  /* Initialize screen buffer */
  if (!(Screen = pspImageCreateVram(512, DISPLAY_SCREEN_HEIGHT,
    PSP_IMAGE_INDEXED)))
      return 1;
  Screen->Viewport.Width = DISPLAY_SCREEN_WIDTH;
  Screen->Viewport.Height = DISPLAY_SCREEN_HEIGHT;

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
  Screen->PalSize = 16;

  return 0;
}

int
ui_event( void )
{
  return 0;
}

int
ui_end( void )
{
  if (Screen) pspImageDestroy(Screen);
  return 0;
}

int
ui_mouse_grab( int startup )
{
  return 0;
}

int
ui_mouse_release( int suspend )
{
  return 0;
}

void menu_file_loadbinarydata( int action ) 
{
}

void menu_file_savebinarydata( int action ) 
{
}

void menu_file_savescreenaspng( int action )
{
}

void menu_file_movies_recordmovieaspng( int action )
{
}

void menu_machine_pause( int action )
{
}

/*
int
menu_select_roms_with_title( const char *title, size_t start, size_t count )
{
  STUB;
  return 0;
}

scaler_type
menu_get_scaler( scaler_available_fn selector )
{
  STUB;
  return SCALER_NORMAL;
}

char *
win32ui_get_filename( const char *title, int is_saving )
{
  OPENFILENAME ofn;
  char szFile[512];
  int result;

  memset( &ofn, 0, sizeof( ofn ) );
  szFile[0] = '\0';

  ofn.lStructSize = sizeof( ofn );
  ofn.hwndOwner = fuse_hWnd;
  ofn.lpstrFilter = "All Files\0*.*\0\0";
  ofn.lpstrCustomFilter = NULL;
  ofn.nFilterIndex = 0;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof( szFile );
  ofn.lpstrFileTitle = NULL;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = /* OFN_DONTADDTORECENT | * OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  if( is_saving ) {
    ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
  } else {
    ofn.Flags |= OFN_FILEMUSTEXIST;
  }
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
/* ofn.pvReserved = NULL; *
/* ofn.FlagsEx = 0; *

  if( is_saving ) {
    result = GetSaveFileName( &ofn );
  } else {
    result = GetOpenFileName( &ofn );
  }

  if( !result ) {
    return NULL;
  } else {
    return strdup( ofn.lpstrFile );
  }
}

char *
ui_get_open_filename( const char *title )
{
  return win32ui_get_filename( title, 0 );
}

char *
ui_get_save_filename( const char *title )
{
  return win32ui_get_filename( title, 1 );
}

ui_confirm_save_t
ui_confirm_save_specific( const char *message )
{
  STUB;
  return UI_CONFIRM_SAVE_CANCEL;
}

ui_confirm_joystick_t
ui_confirm_joystick( libspectrum_joystick libspectrum_type, int inputs )
{
  STUB;
  return UI_CONFIRM_JOYSTICK_NONE;
}

int
ui_tape_browser_update( ui_tape_browser_update_type change,
			libspectrum_tape_block *block )
{
  STUB;
  return 0;
}

int
ui_get_rollback_point( GSList *points )
{
  STUB;
  return -1;
}

int
ui_menu_item_set_active( const char *path, int active )
{
  /* STUB; *
  return 1;
}

#ifdef USE_LIBPNG
void
menu_file_savescreenaspng( int action )
{
  scaler_type scaler;
  char *filename;

  fuse_emulation_pause();

  scaler = menu_get_scaler( screenshot_available_scalers );
  if( scaler == SCALER_NUM ) {
    fuse_emulation_unpause();
    return;
  }

  filename =
    ui_get_save_filename( "Fuse - Save Screenshot as PNG" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_write( filename, scaler );

  free( filename );

  fuse_emulation_unpause();
}
#endif

void
menu_machine_reset( int action )
{
  if( win32ui_confirm( "Reset?" ) ) {
    if( machine_reset( 0 ) ) {
      ui_error( UI_ERROR_ERROR, "couldn't reset machine: giving up!" );

      /* FIXME: abort() seems a bit extreme here, but it'll do for now *
      fuse_abort();
    }
  }
}
*/
