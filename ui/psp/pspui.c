#include <psptypes.h>
#include <pspkernel.h>
#include <psprtc.h>

#include "config.h"
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
#include "input.h"
#include "joystick.h"
#include "keyboard.h"
#include "string.h"

#include "psplib/ui.h"
#include "psplib/font.h"
#include "psplib/psp.h"
#include "psplib/ctrl.h"
#include "psplib/util.h"
#include "psplib/file.h"

#include "pspui.h"

#define TAB_QUICKLOAD 0
#define TAB_STATE     1
#define TAB_OPTIONS   2
#define TAB_SYSTEM    3
#define TAB_MAX       TAB_SYSTEM
#define TAB_ABOUT     4

#define OPTION_DISPLAY_MODE  1
#define OPTION_FRAME_LIMITER 2
#define OPTION_CLOCK_FREQ    3
#define OPTION_SHOW_FPS      4
#define OPTION_CONTROL_MODE  5
#define OPTION_ANIMATE       6

#define SYSTEM_SCRNSHOT    1
#define SYSTEM_RESET       2
#define SYSTEM_TYPE        3

#define KBD 0x100
#define JST 0x200
#define SPC 0x400

#define MAP_BUTTONS 20

#define CODE_MASK(x) (x & 0xff)

#define SPC_MENU  1
#define SPC_KYBD  2

#define CURRENT_GAME (psp_current_game)
#define GAME_LOADED (psp_current_game[0] != '\0')
#define SET_AS_CURRENT_GAME(filename) \
  strncpy(psp_current_game, filename, PSP_FILE_MAX_PATH_LEN - 1)

static const char 
  PresentSlotText[] = "\026\244\020 Save\t\026\001\020 Load\t\026\243\020 Delete",
  EmptySlotText[] = "\026\244\020 Save";

typedef struct psp_ctrl_map_t
{
  u32 button_map[MAP_BUTTONS];
} psp_ctrl_map_t;

typedef struct psp_ctrl_mask_to_index_map_t
{
  u64 mask;
  u8  index;
} psp_ctrl_mask_to_index_map_t;

static const char *QuickloadFilter[] =
      { "DCK", "ROM", "MDR", "TAP", "SPC", "STA", "LTP", "TZX",
        "DSK", "SCL", "TRD", "HDF", "BZ2", "GZ", "RAW", "CSW",
        "WAV", "MGT", "IMG", '\0' };

static int         OnSplashButtonPress(const struct PspUiSplash *splash,
                                       u32 button_mask);
static void        OnSplashRender(const void *uiobject, const void *null);
static const char* OnSplashGetStatusBarText(const struct PspUiSplash *splash);

static int  OnGenericCancel(const void *uiobject, const void* param);
static void OnGenericRender(const void *uiobject, const void *item_obj);
static int  OnGenericButtonPress(const PspUiFileBrowser *browser, const char *path,
                                 u32 button_mask);

static int OnMenuOk(const void *menu, const void *item);
static int OnMenuButtonPress(const struct PspUiMenu *uimenu, PspMenuItem* item, 
                             u32 button_mask);
static int OnMenuItemChanged(const struct PspUiMenu *uimenu, PspMenuItem* item, 
                             const PspMenuOption* option);

static void OnSystemRender(const void *uiobject, const void *item_obj);

static int OnQuickloadOk(const void *browser, const void *path);

static int OnSaveStateOk(const void *gallery, const void *item);
static int OnSaveStateButtonPress(const PspUiGallery *gallery,
                                  PspMenuItem *sel,
                                  u32 button_mask);

void psp_display_menu();
void psp_sound_pause();
void psp_sound_resume();

int snapshot_read_file(const char *filename, FILE *fptr);
int snapshot_write_file(const char *filename, FILE *fptr);

static void psp_display_state_tab();
static inline void psp_keyboard_toggle(unsigned int code, int on);
static inline void psp_joystick_toggle(unsigned int code, int on);

static PspImage* psp_load_state_icon(const char *path);
static int psp_load_state(const char *path);
static PspImage* psp_save_state(const char *path, PspImage *icon);

PspUiSplash SplashScreen = 
{
  OnSplashRender,
  OnGenericCancel,
  OnSplashButtonPress,
  OnSplashGetStatusBarText
};

PspUiFileBrowser QuickloadBrowser = 
{
  OnGenericRender,
  OnQuickloadOk,
  OnGenericCancel,
  OnGenericButtonPress,
  QuickloadFilter,
  0
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

PspUiGallery SaveStateGallery = 
{
  NULL,                        /* PspMenu */
  OnGenericRender,             /* OnRender() */
  OnSaveStateOk,               /* OnOk() */
  OnGenericCancel,             /* OnCancel() */
  OnSaveStateButtonPress,      /* OnButtonPress() */
  NULL                         /* Userdata */
};


/* Tab labels */
static const char *TabLabel[] = 
{
  "Game",
  "Save/Load",
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
  MachineTypes[] = {
    MENU_OPTION("Spectrum 16K",    LIBSPECTRUM_MACHINE_16),
    MENU_OPTION("Spectrum 48K",    LIBSPECTRUM_MACHINE_48),
    MENU_OPTION("Spectrum 128K",   LIBSPECTRUM_MACHINE_128),
    MENU_OPTION("Spectrum +2",     LIBSPECTRUM_MACHINE_PLUS2),
    MENU_OPTION("Spectrum +2A",    LIBSPECTRUM_MACHINE_PLUS2A),
    MENU_OPTION("Spectrum +3",     LIBSPECTRUM_MACHINE_PLUS3),
    MENU_OPTION("Spectrum +3e",    LIBSPECTRUM_MACHINE_PLUS3E),
    MENU_OPTION("Timex TC2048",    LIBSPECTRUM_MACHINE_TC2048),
    MENU_OPTION("Timex TC2068",    LIBSPECTRUM_MACHINE_TC2068),
    MENU_OPTION("Timex TS2068",    LIBSPECTRUM_MACHINE_TS2068),
    MENU_OPTION("Pentagon 128K",   LIBSPECTRUM_MACHINE_PENT),
    MENU_OPTION("Pentagon 512K",   LIBSPECTRUM_MACHINE_PENT512),
    MENU_OPTION("Pentagon 1024K",  LIBSPECTRUM_MACHINE_PENT1024),
    MENU_OPTION("Scorpion ZS 256", LIBSPECTRUM_MACHINE_SCORP),
    MENU_OPTION("Spectrum SE",     LIBSPECTRUM_MACHINE_SE),
    MENU_END_OPTIONS
  },
  ControlModeOptions[] = {
    MENU_OPTION("\026\242\020 cancels, \026\241\020 confirms (US)", 0),
    MENU_OPTION("\026\241\020 cancels, \026\242\020 confirms (Japan)", 1),
    MENU_END_OPTIONS
  };

static const PspMenuItemDef 
  SystemMenuDef[] = {
    MENU_HEADER("System"),
    MENU_ITEM("Machine type", SYSTEM_TYPE, MachineTypes, -1, 
      "\026\250\020 Select emulated system"),
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

/* Default configuration */
static psp_ctrl_map_t default_map =
{
  {
    JST|INPUT_JOYSTICK_UP,    /* Analog Up    */
    JST|INPUT_JOYSTICK_DOWN,  /* Analog Down  */
    JST|INPUT_JOYSTICK_LEFT,  /* Analog Left  */
    JST|INPUT_JOYSTICK_RIGHT, /* Analog Right */
    KBD|INPUT_KEY_p,         /* D-pad Up     */
    KBD|INPUT_KEY_l,       /* D-pad Down   */
    KBD|INPUT_KEY_q,       /* D-pad Left   */
    KBD|INPUT_KEY_w,      /* D-pad Right  */
    JST|INPUT_JOYSTICK_FIRE_1,/* Square       */
    0,                        /* Cross        */
    KBD|INPUT_KEY_space,      /* Circle       */
    KBD|INPUT_KEY_Return,     /* Triangle     */
    0,                        /* L Trigger    */
    SPC|SPC_KYBD,             /* R Trigger    */
    0,                        /* Select       */
/* TODO */
    KBD|INPUT_KEY_0,          /* Start        */
    SPC|SPC_MENU,             /* L+R Triggers */
    0,                        /* Start+Select */
    0,                        /* Select + L   */
    0,                        /* Select + R   */
  }
};

psp_options_t psp_options;
static psp_ctrl_map_t current_map;
static u8 psp_exit_menu;
static u8 show_kybd_held;
static int TabIndex;
static PspImage *Background;
static PspImage *NoSaveIcon;

extern PspImage *Screen;
extern int clear_screen;

static char psp_current_game[PSP_FILE_MAX_PATH_LEN] = {'\0'};
static char SaveStatePath[PSP_FILE_MAX_PATH_LEN],
            ScreenshotPath[PSP_FILE_MAX_PATH_LEN],
            ConfigPath[PSP_FILE_MAX_PATH_LEN];

psp_ctrl_mask_to_index_map_t physical_to_emulated_button_map[] =
{
  { PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER, 16 },
  { PSP_CTRL_START    | PSP_CTRL_SELECT,   17 },
  { PSP_CTRL_SELECT   | PSP_CTRL_LTRIGGER, 18 },
  { PSP_CTRL_SELECT   | PSP_CTRL_RTRIGGER, 19 },
  { PSP_CTRL_ANALUP,   0 }, { PSP_CTRL_ANALDOWN,  1 },
  { PSP_CTRL_ANALLEFT, 2 }, { PSP_CTRL_ANALRIGHT, 3 },
  { PSP_CTRL_UP,   4 }, { PSP_CTRL_DOWN,  5 },
  { PSP_CTRL_LEFT, 6 }, { PSP_CTRL_RIGHT, 7 },
  { PSP_CTRL_SQUARE, 8 },  { PSP_CTRL_CROSS,     9 },
  { PSP_CTRL_CIRCLE, 10 }, { PSP_CTRL_TRIANGLE, 11 },
  { PSP_CTRL_LTRIGGER, 12 }, { PSP_CTRL_RTRIGGER, 13 },
  { PSP_CTRL_SELECT, 14 }, { PSP_CTRL_START, 15 },
  { 0, -1 }
};

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

  /* Init NoSaveState icon image */
  NoSaveIcon = pspImageCreate(256, 192, PSP_IMAGE_16BPP);
  pspImageClear(NoSaveIcon, RGB(0xdd,0xdd,0xdd));

  /* Initialize state menu */
  int i;
  PspMenuItem *item;
  SaveStateGallery.Menu = pspMenuCreate();
  for (i = 0; i < 10; i++)
  {
    item = pspMenuAppendItem(SaveStateGallery.Menu, NULL, i);
    pspMenuSetHelpText(item, EmptySlotText);
  }

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

  show_kybd_held = 0;
  clear_screen = 1;

/* TODO */
  memcpy(&current_map, &default_map, sizeof(default_map));

  return 0;
}

void psp_display_menu()
{
  PspMenuItem *item;
  psp_exit_menu = 0;

  psp_sound_pause();

  /* Set normal clock frequency */
  pspSetClockFrequency(222);
  /* Set buttons to autorepeat */
  pspCtrlSetPollingMode(PSP_CTRL_AUTOREPEAT);

  /* Menu loop */
  while (!ExitPSP && !psp_exit_menu)
  {
    /* Display appropriate tab */
    switch (TabIndex)
    {
    case TAB_QUICKLOAD:
      pspUiOpenBrowser(&QuickloadBrowser, (GAME_LOADED)
                       ? pspFileGetFilename(psp_current_game) : NULL);
      break;
    case TAB_OPTIONS:
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
    case TAB_STATE:
      psp_display_state_tab();
      break;
    case TAB_SYSTEM:
      item = pspMenuFindItemById(SystemUiMenu.Menu, SYSTEM_TYPE);
      pspMenuSelectOptionByValue(item, (void*)(machine_current->machine));

      pspUiOpenMenu(&SystemUiMenu, NULL);
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

    show_kybd_held = 0;
    clear_screen = 1;
    keyboard_release_all();
    psp_uidisplay_reinit();
    psp_sound_resume();
  }
}

int ui_event( void )
{
  static SceCtrlData pad;

  /* Check the input */
  if (pspCtrlPollControls(&pad))
  {
#ifdef PSP_DEBUG
    if ((pad.Buttons & (PSP_CTRL_SELECT | PSP_CTRL_START))
      == (PSP_CTRL_SELECT | PSP_CTRL_START))
        pspUtilSaveVramSeq(ScreenshotPath, "game");
#endif
    /* Parse input */
    psp_ctrl_mask_to_index_map_t *current_mapping = physical_to_emulated_button_map;
    for (; current_mapping->mask; current_mapping++)
    {
      u32 code = current_map.button_map[current_mapping->index];
      u8  on = (pad.Buttons & current_mapping->mask) == current_mapping->mask;

      /* Check to see if a button set is pressed. If so, unset it, so it */
      /* doesn't trigger any other combination presses. */
      if (on) pad.Buttons &= ~current_mapping->mask;

      if ((code & KBD) && !show_kybd_held)
        psp_keyboard_toggle(CODE_MASK(code), on);
      else if ((code & JST) && !show_kybd_held)
        psp_joystick_toggle(CODE_MASK(code), on);
      else if (code & SPC)
      {
        switch (CODE_MASK(code))
        {
        case SPC_MENU:
          if (on) psp_display_menu();
          break;
        case SPC_KYBD:
          if (show_kybd_held != on)
          {
/* TODO */
            if (on) ; /* pspKybdReinit(KeyLayout); */
            else
            {
              clear_screen = 1;
              keyboard_release_all();
            }
          }

          show_kybd_held = on;
          break;
        }
      }
    }
  }

  return 0;
}

int ui_end( void )
{
  if (Background) pspImageDestroy(Background);
  if (NoSaveIcon) pspImageDestroy(NoSaveIcon);

  pspMenuDestroy(OptionUiMenu.Menu);
  pspMenuDestroy(SystemUiMenu.Menu);
  pspMenuDestroy(SaveStateGallery.Menu);

  return 0;
}

static inline void psp_joystick_toggle(unsigned int code, int on)
{
  input_event_t fuse_event;

  fuse_event.type = (on) 
    ? INPUT_EVENT_JOYSTICK_PRESS : INPUT_EVENT_JOYSTICK_RELEASE;
  fuse_event.types.joystick.which = 0;
  fuse_event.types.joystick.button = code;

  input_event(&fuse_event);
}

static inline void psp_keyboard_toggle(unsigned int code, int on)
{
  input_event_t fuse_event;

  fuse_event.type = (on) ? INPUT_EVENT_KEYPRESS : INPUT_EVENT_KEYRELEASE;
  fuse_event.types.key.native_key = code;
  fuse_event.types.key.spectrum_key = code;

  input_event(&fuse_event);
}

static void psp_display_state_tab()
{
  PspMenuItem *item, *sel = NULL;
  SceIoStat stat;
  ScePspDateTime latest;
  char caption[32];
  const char *config_name = (GAME_LOADED)
    ? pspFileGetFilename(psp_current_game) : "BASIC";
  char *path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
  char *game_name = strdup(config_name);
  char *dot = strrchr(game_name, '.');
  if (dot) *dot='\0';

  memset(&latest,0,sizeof(latest));

  /* Initialize icons */
  for (item = SaveStateGallery.Menu->First; item; item = item->Next)
  {
    sprintf(path, "%s%s_%02i.sta", SaveStatePath, config_name, item->ID);

    if (pspFileCheckIfExists(path))
    {
      if (sceIoGetstat(path, &stat) < 0)
        sprintf(caption, "ERROR");
      else
      {
        /* Determine the latest save state */
        if (pspUtilCompareDates(&latest, &stat.st_mtime) < 0)
        {
          sel = item;
          latest = stat.st_mtime;
        }

        sprintf(caption, "%02i/%02i/%02i %02i:%02i", 
          stat.st_mtime.month,
          stat.st_mtime.day,
          stat.st_mtime.year - (stat.st_mtime.year / 100) * 100,
          stat.st_mtime.hour,
          stat.st_mtime.minute);
      }

      pspMenuSetCaption(item, caption);
      item->Icon = psp_load_state_icon(path);
      pspMenuSetHelpText(item, PresentSlotText);
    }
    else
    {
      pspMenuSetCaption(item, "Empty");
      item->Icon = NoSaveIcon;
      pspMenuSetHelpText(item, EmptySlotText);
    }
  }

  free(path);

  /* Highlight the latest save state if none are selected */
  if (SaveStateGallery.Menu->Selected == NULL)
    SaveStateGallery.Menu->Selected = sel;

  pspUiOpenGallery(&SaveStateGallery, game_name);
  free(game_name);

  /* Destroy any icons */
  for (item = SaveStateGallery.Menu->First; item; item = item->Next)
    if (item->Icon != NULL && item->Icon != NoSaveIcon)
      pspImageDestroy((PspImage*)item->Icon);
}

/* Load state icon */
static PspImage* psp_load_state_icon(const char *path)
{
  /* Open file for reading */
  FILE *f = fopen(path, "r");
  if (!f) return NULL;

  /* Load image */
  PspImage *image = pspImageLoadPngFd(f);
  fclose(f);

  return image;
}

/* Load state */
static int psp_load_state(const char *path)
{
  /* Open file for reading */
  FILE *f = fopen(path, "r");
  if (!f) return 0;

  /* Load image into temporary object */
  PspImage *image = pspImageLoadPngFd(f);
  pspImageDestroy(image);

  /* Load the state data */
  int error = !snapshot_read_file(path, f);
  fclose(f);

  return error;
}

/* Save state */
static PspImage* psp_save_state(const char *path, PspImage *icon)
{
  /* Open file for writing */
  FILE *f;
pspUiAlert(path);
  if (!(f = fopen(path, "w")))
    return NULL;

  /* Create thumbnail */
  PspImage *thumb;
  thumb = (icon->Viewport.Width <= 256)
    ? pspImageCreateCopy(icon) : pspImageCreateThumbnail(icon);
  if (!thumb) { fclose(f); return NULL; }

  /* Write the thumbnail */
  if (!pspImageSavePngFd(f, thumb))
  {
    pspImageDestroy(thumb);
    fclose(f);
    return NULL;
  }

  /* Write the state */
  if (snapshot_write_file(path, f))
  {
    pspImageDestroy(thumb);
    thumb = NULL;
  }

  fclose(f);
  return thumb;
}

/**************************/
/* psplib event callbacks */
/**************************/
static int OnGenericCancel(const void *uiobject, const void* param)
{
  psp_exit_menu = 1;
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

    /* Draw background of active tab */
    if (i == TabIndex) 
      pspVideoFillRect(x - 5, 0, x + width + 5, height + 1, 
        UiMetric.TabBgColor);

    /* Draw name of tab */
    pspVideoPrint(UiMetric.Font, x, 0, TabLabel[i], PSP_COLOR_WHITE);
  }
}

static int OnGenericButtonPress(const PspUiFileBrowser *browser,
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
    if (pspUtilSaveVramSeq(ScreenshotPath, "UI"))
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
        psp_exit_menu = 1;
        machine_reset(0);
        return 1;
      }
      break;
    case SYSTEM_SCRNSHOT:
      /* Save screenshot */
      if (!pspUtilSavePngSeq(ScreenshotPath, (GAME_LOADED)
                               ? pspFileGetFilename(psp_current_game) : "BASIC",
                             Screen))
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
  else if (uimenu == &SystemUiMenu)
  {
    switch((int)item->ID)
    {
    case SYSTEM_TYPE:
      if (machine_current->machine == (int)option->Value
          || !pspUiConfirm("This will reset the system. Proceed?"))
        return 0;
      machine_select((int)option->Value);
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

static int OnQuickloadOk(const void *browser, const void *path)
{
  fuse_emulation_pause();

  int error;
  if ((error = utils_open_file(path, 0, NULL)))
  {
    pspUiAlert("Error loading file");
    fuse_emulation_unpause();
    return 0;
  }

  psp_exit_menu = 1;
  display_refresh_all();
  fuse_emulation_unpause();
  SET_AS_CURRENT_GAME(path);

  return 1;
}

static int OnSaveStateOk(const void *gallery, const void *item)
{
  char *path;
  const char *config_name = (GAME_LOADED) 
    ? pspFileGetFilename(psp_current_game) : "BASIC";

  path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
  sprintf(path, "%s%s_%02i.sta", SaveStatePath, config_name, 
    ((const PspMenuItem*)item)->ID);

  if (pspFileCheckIfExists(path) && pspUiConfirm("Load state?"))
  {
    if (psp_load_state(path))
    {
      psp_exit_menu = 1;
      pspMenuFindItemById(((PspUiGallery*)gallery)->Menu, 
        ((PspMenuItem*)item)->ID);
      free(path);

      return 1;
    }

    pspUiAlert("ERROR: State failed to load\nSee documentation for possible reasons");
  }

  free(path);
  return 0;
}

static int OnSaveStateButtonPress(const PspUiGallery *gallery, 
                                  PspMenuItem *sel,
                                  u32 button_mask)
{
  if (button_mask & PSP_CTRL_SQUARE 
    || button_mask & PSP_CTRL_TRIANGLE)
  {
    char *path;
    char caption[32];
    const char *config_name = (GAME_LOADED) 
      ? pspFileGetFilename(psp_current_game) : "BASIC";
    PspMenuItem *item = pspMenuFindItemById(gallery->Menu, sel->ID);

    path = (char*)malloc(strlen(SaveStatePath) + strlen(config_name) + 8);
    sprintf(path, "%s%s_%02i.sta", SaveStatePath, config_name, item->ID);

    do /* not a real loop; flow control construct */
    {
      if (button_mask & PSP_CTRL_SQUARE)
      {
        if (pspFileCheckIfExists(path) && !pspUiConfirm("Overwrite existing state?"))
          break;

        pspUiFlashMessage("Saving, please wait ...");

        PspImage *icon;
        if (!(icon = psp_save_state(path, Screen)))
        {
          pspUiAlert("ERROR: State not saved");
          break;
        }

        SceIoStat stat;

        /* Trash the old icon (if any) */
        if (item->Icon && item->Icon != NoSaveIcon)
          pspImageDestroy((PspImage*)item->Icon);

        /* Update icon, help text */
        item->Icon = icon;
        pspMenuSetHelpText(item, PresentSlotText);

        /* Get file modification time/date */
        if (sceIoGetstat(path, &stat) < 0)
          sprintf(caption, "ERROR");
        else
          sprintf(caption, "%02i/%02i/%02i %02i:%02i", 
            stat.st_mtime.month,
            stat.st_mtime.day,
            stat.st_mtime.year - (stat.st_mtime.year / 100) * 100,
            stat.st_mtime.hour,
            stat.st_mtime.minute);

        pspMenuSetCaption(item, caption);
      }
      else if (button_mask & PSP_CTRL_TRIANGLE)
      {
        if (!pspFileCheckIfExists(path) || !pspUiConfirm("Delete state?"))
          break;

        if (!pspFileDelete(path))
        {
          pspUiAlert("ERROR: State not deleted");
          break;
        }

        /* Trash the old icon (if any) */
        if (item->Icon && item->Icon != NoSaveIcon)
          pspImageDestroy((PspImage*)item->Icon);

        /* Update icon, caption */
        item->Icon = NoSaveIcon;
        pspMenuSetHelpText(item, EmptySlotText);
        pspMenuSetCaption(item, "Empty");
      }
    } while (0);

    if (path) free(path);
    return 0;
  }

  return OnGenericButtonPress(NULL, NULL, button_mask);
}

/**************************/
/* STUB functions         */
/**************************/
void menu_machine_pause( int action )
{
}

char *ui_get_save_filename(const char *title)
{
  return NULL;
}

int ui_menu_item_set_active(const char *path, int active)
{
  return 1;
}

#ifdef USE_LIBPNG
void menu_file_movies_recordmovieaspng( int action )
{
}

void menu_file_savescreenaspng( int action )
{
}
#endif

void menu_file_loadbinarydata( int action )
{
}

void menu_file_savebinarydata( int action )
{
}

int ui_mouse_grab( int startup )
{
  return 0;
}

int ui_mouse_release( int suspend )
{
  return 0;
}

int ui_tape_browser_update(ui_tape_browser_update_type change,
                           libspectrum_tape_block *block)
{
  return 0;
}

ui_confirm_save_t ui_confirm_save_specific(const char *message)
{
  return UI_CONFIRM_SAVE_CANCEL;
}

ui_confirm_joystick_t ui_confirm_joystick(libspectrum_joystick libspectrum_type, int inputs)
{
  return UI_CONFIRM_JOYSTICK_NONE;
}

int ui_widgets_reset()
{
  return 0;
}

int ui_get_rollback_point(GSList *points)
{
  return -1;
}

int ui_error_specific(ui_error_level severity, const char *message) 
{
#ifdef PSP_DEBUG
  FILE *err_file = fopen("errors.txt", "a");
  switch( severity ) 
  {
  case UI_ERROR_INFO: break;    /* Shouldn't happen */
  case UI_ERROR_WARNING: fprintf( err_file, "warning: " ); break;
  case UI_ERROR_ERROR: fprintf( err_file, "error: " ); break;
  }
  fprintf(err_file, message);
  fclose(err_file);
#endif
  return 0;
}
