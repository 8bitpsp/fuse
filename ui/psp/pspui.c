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

#include "pspui.h"

psp_options_t psp_options;

int ui_init( int *argc, char ***argv )
{
  psp_options.show_fps     = 0;
  psp_options.display_mode = DISPLAY_MODE_FIT_HEIGHT;

  return 0;
}

int ui_end( void )
{
  return 0;
}

/* These are stub functions */
void menu_file_movies_recordmovieaspng( int action ) { }
void menu_file_savescreenaspng( int action ) {  }
void menu_file_loadbinarydata( int action ) {  }
void menu_file_savebinarydata( int action ) {  }
void menu_machine_pause( int action ) {  }
int  ui_mouse_grab( int startup ) {  return 0; }
int  ui_mouse_release( int suspend ) {  return 0; }
int  ui_event( void ) {  return 0; }

/* Widget stubs */
int ui_debugger_activate() { return 1; }
int ui_debugger_deactivate(int interruptable) { return 1; }
int ui_debugger_update() { return 1; }
int ui_debugger_disassemble(libspectrum_word address) { return 1; }
int ui_statusbar_update(ui_statusbar_item item, ui_statusbar_state state) { return 1; }
int ui_statusbar_update_speed(float speed) { return 1; }
int ui_tape_browser_update(ui_tape_browser_update_type change,
                           libspectrum_tape_block *block) { return 1; }
int ui_menu_item_set_active(const char *path, int active) { return 1; }
char *ui_get_save_filename(const char *title) { return NULL; }
ui_confirm_save_t 
  ui_confirm_save_specific(const char *message) { return UI_CONFIRM_SAVE_CANCEL; }
ui_confirm_joystick_t 
  ui_confirm_joystick(libspectrum_joystick libspectrum_type, int inputs) 
  { return UI_CONFIRM_JOYSTICK_NONE; }
int ui_widgets_reset() { return 1; }
int ui_get_rollback_point(GSList *points) { return 1; }
int ui_error_specific(ui_error_level severity, const char *message) { return 1; }
