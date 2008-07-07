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

#include "psplib/ui.h"
#include "psplib/font.h"
#include "psplib/psp.h"
#include "psplib/ctrl.h"
#include "psplib/util.h"
#include "psplib/file.h"

#include "pspui.h"

#define TAB_OPTIONS   0
#define TAB_SYSTEM    1
#define TAB_MAX       TAB_SYSTEM
#define TAB_ABOUT     2

#define OPTION_DISPLAY_MODE  1
#define OPTION_FRAME_LIMITER 2
#define OPTION_CLOCK_FREQ    3
#define OPTION_SHOW_FPS      4
#define OPTION_CONTROL_MODE  5
#define OPTION_ANIMATE       6

#define SYSTEM_SCRNSHOT    1
#define SYSTEM_RESET       2

static int         OnSplashButtonPress(const struct PspUiSplash *splash,
                                       u32 button_mask);
static void        OnSplashRender(const void *uiobject, const void *null);
static const char* OnSplashGetStatusBarText(const struct PspUiSplash *splash);

static int    OnGenericCancel(const void *uiobject, const void* param);
static void   OnGenericRender(const void *uiobject, const void *item_obj);
static int    OnGenericButtonPress(const void *browser, const char *path,
                            u32 button_mask);

static int OnMenuOk(const void *menu, const void *item);
static int OnMenuButtonPress(const struct PspUiMenu *uimenu, PspMenuItem* item, 
                             u32 button_mask);
static int OnMenuItemChanged(const struct PspUiMenu *uimenu, PspMenuItem* item, 
                             const PspMenuOption* option);

static void OnSystemRender(const void *uiobject, const void *item_obj);

void psp_display_menu();

PspUiSplash SplashScreen = 
{
  OnSplashRender,
  OnGenericCancel,
  OnSplashButtonPress,
  OnSplashGetStatusBarText
};

PspUiMenu
  OptionUiMenu =
  {
    NULL,                  /* PspMenu */
    OnGenericRender,       /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  },
  SystemUiMenu =
  {
    NULL,                  /* PspMenu */
    OnSystemRender,        /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  };

/* Tab labels */
static const char *TabLabel[] = 
{
  "Options",
  "System",
  "About"
};

static const PspMenuOptionDef
  ToggleOptions[] = {
    MENU_OPTION("Disabled", 0),
    MENU_OPTION("Enabled",  1),
    MENU_END_OPTIONS
  },
  ScreenSizeOptions[] = {
    MENU_OPTION("Actual size", DISPLAY_MODE_UNSCALED),
    MENU_OPTION("4:3 scaled (fit height)", DISPLAY_MODE_FIT_HEIGHT),
    MENU_OPTION("16:9 scaled (fit screen)", DISPLAY_MODE_FILL_SCREEN),
    MENU_END_OPTIONS
  },
  PspClockFreqOptions[] = {
    MENU_OPTION("222 MHz", 222),
    MENU_OPTION("266 MHz", 266),
    MENU_OPTION("300 MHz", 300),
    MENU_OPTION("333 MHz", 333),
    MENU_END_OPTIONS
  },
  ControlModeOptions[] = {
    MENU_OPTION("\026\242\020 cancels, \026\241\020 confirms (US)", 0),
    MENU_OPTION("\026\241\020 cancels, \026\242\020 confirms (Japan)", 1),
    MENU_END_OPTIONS
  };

static const PspMenuItemDef 
  SystemMenuDef[] = {
    MENU_HEADER("Options"),
    MENU_ITEM("Reset", SYSTEM_RESET, NULL, -1, "\026\001\020 Reset system" ),
    MENU_ITEM("Save screenshot",  SYSTEM_SCRNSHOT, NULL,  -1, 
      "\026\001\020 Save screenshot"),
    MENU_END_ITEMS
  },
  OptionMenuDef[] = {
    MENU_HEADER("Video"),
    MENU_ITEM("Screen size", OPTION_DISPLAY_MODE, ScreenSizeOptions, -1, 
      "\026\250\020 Change screen size"),
    MENU_HEADER("Performance"),
/*
    MENU_ITEM("Frame limiter", OPTION_FRAME_LIMITER, ToggleOptions, -1,
      "\026\250\020 Enable/disable correct FPS emulation"),
*/
    MENU_ITEM("PSP clock frequency", OPTION_CLOCK_FREQ, PspClockFreqOptions,
      -1, "\026\250\020 Larger values: faster emulation, faster battery depletion (default: 222MHz)"),
    MENU_ITEM("Show FPS counter", OPTION_SHOW_FPS, ToggleOptions, -1, 
      "\026\250\020 Show/hide the frames-per-second counter"),
    MENU_HEADER("Menu"),
    MENU_ITEM("Button mode", OPTION_CONTROL_MODE, ControlModeOptions, -1,
      "\026\250\020 Change OK and Cancel button mapping"),
    MENU_ITEM("Animations", OPTION_ANIMATE, ToggleOptions, -1,
      "\026\250\020 Enable/disable menu animations"),
    MENU_END_ITEMS
  };

psp_options_t psp_options;
static u8 ExitMenu;
static int TabIndex;
static PspImage *Background;
extern PspImage *Screen;

static char SaveStatePath[PSP_FILE_MAX_PATH_LEN],
            ScreenshotPath[PSP_FILE_MAX_PATH_LEN],
            ConfigPath[PSP_FILE_MAX_PATH_LEN];

int ui_init(int *argc, char ***argv)
{
  psp_options.show_fps     = 0;
  psp_options.display_mode = DISPLAY_MODE_FIT_HEIGHT;
  psp_options.clock_freq   = 222;
  psp_options.control_mode = 0;
  psp_options.limit_frames = 1;
  psp_options.animate_menu = 1;

  /* Initialize paths */
  sprintf(SaveStatePath, "%sstates/", pspGetAppDirectory());
  sprintf(ScreenshotPath, "%sscreens/", pspGetAppDirectory());
  sprintf(ConfigPath, "%sconfig/", pspGetAppDirectory());

  /* Initialize options menu */
  OptionUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(OptionUiMenu.Menu, OptionMenuDef);

  /* Initialize system menu */
  SystemUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(SystemUiMenu.Menu, SystemMenuDef);

  /* Load the background image */
  Background = pspImageLoadPng("background.png");

  /* Initialize UI components */
  UiMetric.Background = Background;
  UiMetric.Font = &PspStockFont;
  UiMetric.Left = 8;
  UiMetric.Top = 24;
  UiMetric.Right = 472;
  UiMetric.Bottom = 240;
  UiMetric.OkButton = (!psp_options.control_mode)
    ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
  UiMetric.CancelButton = (!psp_options.control_mode)
    ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
  UiMetric.ScrollbarColor = PSP_COLOR_GRAY;
  UiMetric.ScrollbarBgColor = 0x44ffffff;
  UiMetric.ScrollbarWidth = 10;
  UiMetric.TextColor = PSP_COLOR_GRAY;
  UiMetric.SelectedColor = COLOR(0xff,0xff,0,0xff);
  UiMetric.SelectedBgColor = COLOR(0,0,0,0x55);
  UiMetric.StatusBarColor = PSP_COLOR_WHITE;
  UiMetric.BrowserFileColor = PSP_COLOR_GRAY;
  UiMetric.BrowserDirectoryColor = PSP_COLOR_YELLOW;
  UiMetric.GalleryIconsPerRow = 5;
  UiMetric.GalleryIconMarginWidth = 16;
  UiMetric.MenuItemMargin = 20;
  UiMetric.MenuSelOptionBg = PSP_COLOR_BLACK;
  UiMetric.MenuOptionBoxColor = PSP_COLOR_GRAY;
  UiMetric.MenuOptionBoxBg = COLOR(0, 0, 0, 0xBB);
  UiMetric.MenuDecorColor = UiMetric.SelectedColor;
  UiMetric.DialogFogColor = COLOR(0, 0, 0, 88);
  UiMetric.TitlePadding = 4;
  UiMetric.TitleColor = PSP_COLOR_WHITE;
  UiMetric.MenuFps = 30;
  UiMetric.TabBgColor = PSP_COLOR_WHITE;
  UiMetric.Animate = psp_options.animate_menu;

  TabIndex = TAB_ABOUT;

  return 0;
}

void psp_display_menu()
{
  PspMenuItem *item;
  ExitMenu = 0;

  /* Set normal clock frequency */
  pspSetClockFrequency(222);
  /* Set buttons to autorepeat */
  pspCtrlSetPollingMode(PSP_CTRL_AUTOREPEAT);

  /* Menu loop */
  while (!ExitPSP && !ExitMenu)
  {
    /* Display appropriate tab */
    switch (TabIndex)
    {
    case TAB_OPTIONS:
      /* Init menu options */
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_DISPLAY_MODE);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.display_mode);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_CLOCK_FREQ);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.clock_freq);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_SHOW_FPS);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.show_fps);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_CONTROL_MODE);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.control_mode);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_ANIMATE);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.animate_menu);
      if ((item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_FRAME_LIMITER)))
        pspMenuSelectOptionByValue(item, (void*)(int)psp_options.limit_frames);

      pspUiOpenMenu(&OptionUiMenu, NULL);
      break;
    case TAB_ABOUT:
      pspUiSplashScreen(&SplashScreen);
      break;
    }
  }

  if (!ExitPSP)
  {
    /* Set clock frequency during emulation */
    pspSetClockFrequency(psp_options.clock_freq);
    /* Set buttons to normal mode */
    pspCtrlSetPollingMode(PSP_CTRL_NORMAL);
  }
}

int ui_event( void )
{
  static u8 first_time_run = 1;
  if (first_time_run)
  {
    /* Show menu */
    psp_display_menu();
    first_time_run = 0;

    return 0;
  }

  return 0;
}

int ui_end( void )
{
  if (Background) pspImageDestroy(Background);

  pspMenuDestroy(OptionUiMenu.Menu);
  pspMenuDestroy(SystemUiMenu.Menu);

  return 0;
}

/* psplib Event callbacks */
static int OnGenericCancel(const void *uiobject, const void* param)
{
  ExitMenu = 1;
  return 1;
}

static void OnGenericRender(const void *uiobject, const void *item_obj)
{
  int height = pspFontGetLineHeight(UiMetric.Font);
  int width;

  /* Draw tabs */
  int i, x;
  for (i = 0, x = 5; i <= TAB_MAX; i++, x += width + 10)
  {
    /* Determine width of text */
    width = pspFontGetTextWidth(UiMetric.Font, TabLabel[i]);

    if (i == TabIndex)
    {
      /* Draw background of active tab */
      if (i == TabIndex) 
        pspVideoFillRect(x - 5, 0, x + width + 5, height + 1, 
          UiMetric.TabBgColor);

      /* Draw name of tab */
      pspVideoPrint(UiMetric.Font, x, 0, TabLabel[i], PSP_COLOR_WHITE);
    }
  }
}

static int OnGenericButtonPress(const void *browser, 
  const char *path, u32 button_mask)
{
  /* If L or R are pressed, switch tabs */
  if (button_mask & PSP_CTRL_LTRIGGER)
  { if (--TabIndex < 0) TabIndex=TAB_MAX; }
  else if (button_mask & PSP_CTRL_RTRIGGER)
  { if (++TabIndex > TAB_MAX) TabIndex=0; }
  else if ((button_mask & (PSP_CTRL_START | PSP_CTRL_SELECT)) 
    == (PSP_CTRL_START | PSP_CTRL_SELECT))
  {
    if (pspUtilSaveVramSeq(pspGetAppDirectory(), "UI"))
      pspUiAlert("Saved successfully");
    else
      pspUiAlert("ERROR: Not saved");
    return 0;
  }
  else return 0;

  return 1;
}

static void OnSplashRender(const void *splash, const void *null)
{
  int fh, i, x, y, height;
  const char *lines[] = 
  { 
    PSP_APP_NAME" version "PSP_APP_VER" ("__DATE__")",
    "\026http://psp.akop.org/fuse",
    " ",
    "2008 Akop Karapetyan (port)",
    "2003-2008 Philip Kendall et al. (emulation)",
    NULL
  };

  fh = pspFontGetLineHeight(UiMetric.Font);

  for (i = 0; lines[i]; i++);
  height = fh * (i - 1);

  /* Render lines */
  for (i = 0, y = SCR_HEIGHT / 2 - height / 2; lines[i]; i++, y += fh)
  {
    x = SCR_WIDTH / 2 - pspFontGetTextWidth(UiMetric.Font, lines[i]) / 2;
    pspVideoPrint(UiMetric.Font, x, y, lines[i], PSP_COLOR_GRAY);
  }

  /* Render PSP status */
  OnGenericRender(splash, null);
}

static int OnSplashButtonPress(const struct PspUiSplash *splash, 
                               u32 button_mask)
{
  return OnGenericButtonPress(NULL, NULL, button_mask);
}

static const char* OnSplashGetStatusBarText(const struct PspUiSplash *splash)
{
  return "\026\255\020/\026\256\020 Switch tabs";
}

static int OnMenuOk(const void *uimenu, const void* sel_item)
{
  if (uimenu == &SystemUiMenu)
  {
    switch (((const PspMenuItem*)sel_item)->ID)
    {
    case SYSTEM_RESET:
      if (pspUiConfirm("Reset the system?"))
      {
        ExitMenu = 1;
        machine_reset(0);
        return 1;
      }
      break;
    case SYSTEM_SCRNSHOT:
      /* Save screenshot */
/* TODO: name of game */
      if (!pspUtilSavePngSeq(ScreenshotPath, "GAME", Screen))
        pspUiAlert("ERROR: Screenshot not saved");
      else
        pspUiAlert("Screenshot saved successfully");
      break;
    }
  }

  return 0;
}

static int OnMenuButtonPress(const struct PspUiMenu *uimenu,
                             PspMenuItem* sel_item,
                             u32 button_mask)
{
  return OnGenericButtonPress(NULL, NULL, button_mask);
}

static int OnMenuItemChanged(const struct PspUiMenu *uimenu,
                             PspMenuItem* item,
                             const PspMenuOption* option)
{
  if (uimenu == &OptionUiMenu)
  {
    switch((int)item->ID)
    {
    case OPTION_DISPLAY_MODE:
      psp_options.display_mode = (int)option->Value;
      break;
    case OPTION_FRAME_LIMITER:
      psp_options.limit_frames = (int)option->Value;
      break;
    case OPTION_CLOCK_FREQ:
      psp_options.clock_freq = (int)option->Value;
      break;
    case OPTION_SHOW_FPS:
      psp_options.show_fps = (int)option->Value;
      break;
    case OPTION_CONTROL_MODE:
      psp_options.control_mode = (int)option->Value;
      UiMetric.OkButton = (!psp_options.control_mode) ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
      UiMetric.CancelButton = (!psp_options.control_mode) ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
      break;
    case OPTION_ANIMATE:
      psp_options.animate_menu = (int)option->Value;
      UiMetric.Animate = psp_options.animate_menu;
      break;
    }
  }

  return 1;
}

static void OnSystemRender(const void *uiobject, const void *item_obj)
{
  int w, h, x, y;
  w = Screen->Viewport.Width >> 1;
  h = Screen->Viewport.Height >> 1;
  x = UiMetric.Right - w - UiMetric.ScrollbarWidth;
  y = SCR_HEIGHT - h - 56;

  /* Draw a small representation of the screen */
  pspVideoShadowRect(x, y, x + w - 1, y + h - 1, PSP_COLOR_BLACK, 3);
  pspVideoPutImage(Screen, x, y, w, h);
  pspVideoDrawRect(x, y, x + w - 1, y + h - 1, PSP_COLOR_GRAY);

  OnGenericRender(uiobject, item_obj);
}

/* These are stub functions */
void menu_file_movies_recordmovieaspng( int action ) { }
void menu_file_savescreenaspng( int action ) {  }
void menu_file_loadbinarydata( int action ) {  }
void menu_file_savebinarydata( int action ) {  }
void menu_machine_pause( int action ) {  }
int  ui_mouse_grab( int startup ) {  return 0; }
int  ui_mouse_release( int suspend ) {  return 0; }

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
