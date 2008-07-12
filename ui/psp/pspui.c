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
#include "utils.h"
#include "input.h"
#include "joystick.h"
#include "keyboard.h"
#include "string.h"

#include "psplib/ui.h"
#include "psplib/audio.h"
#include "psplib/font.h"
#include "psplib/psp.h"
#include "psplib/ctrl.h"
#include "psplib/util.h"
#include "psplib/file.h"
#include "psplib/init.h"

#include "unzip.h"

#include "pspui.h"

PSP_MODULE_INFO(PSP_APP_NAME, 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

#define TAB_QUICKLOAD 0
#define TAB_STATE     1
#define TAB_CONTROLS  2
#define TAB_OPTIONS   3
#define TAB_SYSTEM    4
#define TAB_MAX       TAB_SYSTEM
#define TAB_ABOUT     5

#define OPTION_DISPLAY_MODE  1
#define OPTION_FRAME_LIMITER 2
#define OPTION_CLOCK_FREQ    3
#define OPTION_SHOW_FPS      4
#define OPTION_SHOW_BORDER   5
#define OPTION_CONTROL_MODE  6
#define OPTION_ANIMATE       7

#define SYSTEM_SCRNSHOT    1
#define SYSTEM_RESET       2
#define SYSTEM_TYPE        3

#define SPC_MENU  1
#define SPC_KYBD  2

#define CURRENT_GAME (psp_current_game)
#define GAME_LOADED (psp_current_game[0] != '\0')
#define SET_AS_CURRENT_GAME(filename) \
  strncpy(psp_current_game, filename, PSP_FILE_MAX_PATH_LEN - 1)

static const char 
  PresentSlotText[] = "\026\244\020 Save\t\026\001\020 Load\t\026\243\020 Delete",
  EmptySlotText[]   = "\026\244\020 Save",
  ControlHelpText[] = "\026\250\020 Change mapping\t\026\001\020 Save to \271\t"
                      "\026\244\020 Set as default\t\026\243\020 Load defaults";

SceCtrlData psp_pad_status;

static const char *QuickloadFilter[] =
      { "ZIP", "DCK", "ROM", "MDR", "TAP", "SPC", "STA", "LTP", "TZX",
        "DSK", "SCL", "TRD", "HDF", "BZ2", "GZ", "RAW", "CSW",
        "WAV", "MGT", "IMG", "Z80", '\0' };

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

void psp_sound_pause();
void psp_sound_resume();

int snapshot_read_file(const char *filename, FILE *fptr);
int snapshot_write_file(const char *filename, FILE *fptr);

static void psp_display_menu();
static void psp_display_state_tab();
static void psp_display_control_tab();

static void psp_load_options();
static int  psp_save_options();

static int  psp_load_game(const char *path);

static void psp_init_controls(psp_ctrl_map_t *config);
static int  psp_load_controls(const char *filename, psp_ctrl_map_t *config);
static int  psp_save_controls(const char *filename, const psp_ctrl_map_t *config);

static inline void psp_keyboard_toggle(unsigned int code, int on);
static void psp_exit_callback(void* arg);

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
  },
  ControlUiMenu =
  {
    NULL,                  /* PspMenu */
    OnGenericRender,       /* OnRender() */
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
  "Controls",
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
  MappableButtons[] = {
    /* Unmapped */
    MENU_OPTION("None", 0),
    /* Special */
    MENU_OPTION("Special: Open Menu",       (SPC|SPC_MENU)),
    MENU_OPTION("Special: Show keyboard",   (SPC|SPC_KYBD)),
    /* Joystick */
    MENU_OPTION("Joystick Up",    (JST|INPUT_JOYSTICK_UP)),
    MENU_OPTION("Joystick Down",  (JST|INPUT_JOYSTICK_DOWN)),
    MENU_OPTION("Joystick Left",  (JST|INPUT_JOYSTICK_LEFT)),
    MENU_OPTION("Joystick Right", (JST|INPUT_JOYSTICK_RIGHT)),
    /* Etc... */
    MENU_OPTION("Space",     (KBD|INPUT_KEY_space)),
    MENU_OPTION("Enter",     (KBD|INPUT_KEY_Return)),
    /* Numbers */
    MENU_OPTION("0", (KBD|INPUT_KEY_0)), MENU_OPTION("1", (KBD|INPUT_KEY_1)),
    MENU_OPTION("2", (KBD|INPUT_KEY_2)), MENU_OPTION("3", (KBD|INPUT_KEY_3)),
    MENU_OPTION("4", (KBD|INPUT_KEY_4)), MENU_OPTION("5 / Left", (KBD|INPUT_KEY_5)),
    MENU_OPTION("6 / Down", (KBD|INPUT_KEY_6)), MENU_OPTION("7 / Up", (KBD|INPUT_KEY_7)),
    MENU_OPTION("8 / Right", (KBD|INPUT_KEY_8)), MENU_OPTION("9", (KBD|INPUT_KEY_9)),
    /* Alphabet */
    MENU_OPTION("A", (KBD|INPUT_KEY_a)), MENU_OPTION("B", (KBD|INPUT_KEY_b)),
    MENU_OPTION("C", (KBD|INPUT_KEY_c)), MENU_OPTION("D", (KBD|INPUT_KEY_d)),
    MENU_OPTION("E", (KBD|INPUT_KEY_e)), MENU_OPTION("F", (KBD|INPUT_KEY_f)),
    MENU_OPTION("G", (KBD|INPUT_KEY_g)), MENU_OPTION("H", (KBD|INPUT_KEY_h)),
    MENU_OPTION("I", (KBD|INPUT_KEY_i)), MENU_OPTION("J", (KBD|INPUT_KEY_j)),
    MENU_OPTION("K", (KBD|INPUT_KEY_k)), MENU_OPTION("L", (KBD|INPUT_KEY_l)),
    MENU_OPTION("M", (KBD|INPUT_KEY_m)), MENU_OPTION("N", (KBD|INPUT_KEY_n)),
    MENU_OPTION("O", (KBD|INPUT_KEY_o)), MENU_OPTION("P", (KBD|INPUT_KEY_p)),
    MENU_OPTION("Q", (KBD|INPUT_KEY_q)), MENU_OPTION("R", (KBD|INPUT_KEY_r)),
    MENU_OPTION("S", (KBD|INPUT_KEY_s)), MENU_OPTION("T", (KBD|INPUT_KEY_t)),
    MENU_OPTION("U", (KBD|INPUT_KEY_u)), MENU_OPTION("V", (KBD|INPUT_KEY_v)),
    MENU_OPTION("W", (KBD|INPUT_KEY_w)), MENU_OPTION("X", (KBD|INPUT_KEY_x)),
    MENU_OPTION("Y", (KBD|INPUT_KEY_y)), MENU_OPTION("Z", (KBD|INPUT_KEY_z)),
    /* State keys */
    MENU_OPTION("Caps Shift",  (KBD|INPUT_KEY_Shift_R)),
    MENU_OPTION("Symbol Shift",(KBD|INPUT_KEY_Super_R)),
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
    MENU_ITEM("Screen border", OPTION_SHOW_BORDER, ToggleOptions, -1, 
      "\026\250\020 Show/hide border surrounding the main display"),
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
  },
  ControlMenuDef[] = {
    MENU_ITEM(PSP_CHAR_ANALUP,   0, MappableButtons, -1,  ControlHelpText),
    MENU_ITEM(PSP_CHAR_ANALDOWN, 1, MappableButtons, -1,  ControlHelpText),
    MENU_ITEM(PSP_CHAR_ANALLEFT, 2, MappableButtons, -1,  ControlHelpText),
    MENU_ITEM(PSP_CHAR_ANALRIGHT, 3, MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_UP, 4, MappableButtons, -1,    ControlHelpText),
    MENU_ITEM(PSP_CHAR_DOWN, 5, MappableButtons, -1,  ControlHelpText),
    MENU_ITEM(PSP_CHAR_LEFT, 6, MappableButtons, -1,  ControlHelpText),
    MENU_ITEM(PSP_CHAR_RIGHT, 7, MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_SQUARE, 8, MappableButtons, -1,    ControlHelpText),
    MENU_ITEM(PSP_CHAR_CROSS, 9, MappableButtons, -1,     ControlHelpText),
    MENU_ITEM(PSP_CHAR_CIRCLE, 10, MappableButtons, -1,   ControlHelpText),
    MENU_ITEM(PSP_CHAR_TRIANGLE, 11, MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_LTRIGGER, 12, MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_RTRIGGER, 13, MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_SELECT, 14, MappableButtons, -1,   ControlHelpText),
    MENU_ITEM(PSP_CHAR_START, 15, MappableButtons, -1,    ControlHelpText),
    MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_RTRIGGER, 16,
      MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_START"+"PSP_CHAR_SELECT, 17,
      MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_SELECT"+"PSP_CHAR_LTRIGGER, 18,
      MappableButtons, -1, ControlHelpText),
    MENU_ITEM(PSP_CHAR_SELECT"+"PSP_CHAR_RTRIGGER, 19,
      MappableButtons, -1, ControlHelpText),
    MENU_END_ITEMS
  };

/* Default configuration */
static psp_ctrl_map_t default_map =
{
  {
    KBD|INPUT_KEY_p,       /* Analog Up    */
    KBD|INPUT_KEY_l,       /* Analog Down  */
    KBD|INPUT_KEY_z,       /* Analog Left  */
    KBD|INPUT_KEY_x,       /* Analog Right */
    KBD|INPUT_KEY_7,       /* D-pad Up     */
    KBD|INPUT_KEY_6,       /* D-pad Down   */
    KBD|INPUT_KEY_5,       /* D-pad Left   */
    KBD|INPUT_KEY_8,       /* D-pad Right  */
    KBD|INPUT_KEY_Return,  /* Square       */
    KBD|INPUT_KEY_0,       /* Cross        */
    KBD|INPUT_KEY_space,   /* Circle       */
    KBD|INPUT_KEY_Shift_R, /* Triangle     */
    0,                     /* L Trigger    */
    SPC|SPC_KYBD,          /* R Trigger    */
    KBD|INPUT_KEY_3,       /* Select       */
    KBD|INPUT_KEY_0,       /* Start        */
    SPC|SPC_MENU,          /* L+R Triggers */
    0,                     /* Start+Select */
    0,                     /* Select + L   */
    0,                     /* Select + R   */
  }
};

psp_ctrl_map_t current_map;
psp_options_t psp_options;
u8 psp_menu_active;

static u8 psp_exit_menu;
static u8 show_kybd_held;
static int TabIndex;
static PspImage *Background;
static PspImage *NoSaveIcon;

extern PspImage *Screen;
extern int clear_screen;
extern int fuse_exiting;

static char psp_current_game[PSP_FILE_MAX_PATH_LEN] = {'\0'},
            psp_game_path[PSP_FILE_MAX_PATH_LEN] = {'\0'},
            psp_save_state_path[PSP_FILE_MAX_PATH_LEN],
            psp_screenshot_path[PSP_FILE_MAX_PATH_LEN],
            psp_config_path[PSP_FILE_MAX_PATH_LEN];

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

static void psp_exit_callback(void* arg)
{
  fuse_exiting = 1;
  ExitPSP = 1;
}

int ui_init(int *argc, char ***argv)
{
  pspInit(*argv[0]);
  pspCtrlInit();

  /* Initialize callbacks */
  pspRegisterCallback(PSP_EXIT_CALLBACK, psp_exit_callback, NULL);
  pspStartCallbackThread();

  /* Initialize paths */
  sprintf(psp_save_state_path, "%sstates/", pspGetAppDirectory());
  sprintf(psp_screenshot_path, "%sscreens/", pspGetAppDirectory());
  sprintf(psp_config_path, "%sconfig/", pspGetAppDirectory());

  /* Initialize options menu */
  OptionUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(OptionUiMenu.Menu, OptionMenuDef);

  /* Initialize system menu */
  SystemUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(SystemUiMenu.Menu, SystemMenuDef);

  /* Initialize control menu */
  ControlUiMenu.Menu = pspMenuCreate();
  pspMenuLoad(ControlUiMenu.Menu, ControlMenuDef);

  /* Load the background image */
  Background = pspImageLoadPng("background.png");

  /* Init NoSaveState icon image */
  NoSaveIcon = pspImageCreate(256, 192, PSP_IMAGE_16BPP);
  pspImageClear(NoSaveIcon, RGB(0x66,0x66,0x66));

  /* Initialize state menu */
  int i;
  PspMenuItem *item;
  SaveStateGallery.Menu = pspMenuCreate();
  for (i = 0; i < 10; i++)
  {
    item = pspMenuAppendItem(SaveStateGallery.Menu, NULL, i);
    pspMenuSetHelpText(item, EmptySlotText);
  }

  psp_load_options();

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
  UiMetric.SelectedColor = COLOR(0xf7,0xc2,0x50,0xFF);
  UiMetric.SelectedBgColor = COLOR(0x46,0x98,0xce,0x99);
  UiMetric.StatusBarColor = PSP_COLOR_WHITE;
  UiMetric.BrowserFileColor = PSP_COLOR_GRAY;
  UiMetric.BrowserDirectoryColor = PSP_COLOR_YELLOW;
  UiMetric.GalleryIconsPerRow = 5;
  UiMetric.GalleryIconMarginWidth = 8;
  UiMetric.MenuItemMargin = 20;
  UiMetric.MenuSelOptionBg = PSP_COLOR_BLACK;
  UiMetric.MenuOptionBoxColor = PSP_COLOR_GRAY;
  UiMetric.MenuOptionBoxBg = COLOR(0x46,0x98,0xce,0xCC);
  UiMetric.MenuDecorColor = UiMetric.SelectedColor;
  UiMetric.DialogFogColor = COLOR(0x59,0x91,0x38,0xBB);
  UiMetric.TitlePadding = 4;
  UiMetric.TitleColor = PSP_COLOR_WHITE;
  UiMetric.MenuFps = 30;
  UiMetric.TabBgColor = COLOR(0xa4,0xa4,0xa4,0xff);
  UiMetric.Animate = psp_options.animate_menu;

  TabIndex = TAB_ABOUT;

  show_kybd_held = 0;
  clear_screen = 1;
  psp_menu_active = 1;

  /* Load default configuration */
  psp_load_controls("DEFAULT", &default_map);
  psp_init_controls(&current_map);

  return 0;
}

static void psp_display_menu()
{
  PspMenuItem *item;
  psp_exit_menu = 0;
  psp_menu_active = 1;

  fuse_emulation_pause();
  psp_sound_pause();

  /* For purposes of the menu, the screen excludes the border */
  Screen->Viewport.X = DISPLAY_BORDER_WIDTH / 2;
  Screen->Viewport.Y = DISPLAY_BORDER_HEIGHT;
  Screen->Viewport.Width = DISPLAY_WIDTH / 2;
  Screen->Viewport.Height = DISPLAY_HEIGHT;

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
      pspUiOpenBrowser(&QuickloadBrowser, 
        (GAME_LOADED) ? psp_current_game 
          : ((psp_game_path[0]) ? psp_game_path : NULL));
      break;
    case TAB_CONTROLS:
      psp_display_control_tab();
      break;
    case TAB_OPTIONS:
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_DISPLAY_MODE);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.display_mode);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_CLOCK_FREQ);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.clock_freq);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_SHOW_FPS);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.show_fps);
      item = pspMenuFindItemById(OptionUiMenu.Menu, OPTION_SHOW_BORDER);
      pspMenuSelectOptionByValue(item, (void*)(int)psp_options.show_border);
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

  psp_menu_active = 0;

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
    fuse_emulation_unpause();
  }
}

int ui_event( void )
{
  static u8 first_time_run = 1;

  if (first_time_run)
  {
    machine_select(psp_options.machine); /* Change the default machine */
    psp_display_menu(); /* Show menu */

    first_time_run = 0;
    return 0;
  }

#ifdef PSP_DEBUG
  if ((psp_pad_status.Buttons & (PSP_CTRL_SELECT | PSP_CTRL_START))
    == (PSP_CTRL_SELECT | PSP_CTRL_START))
      pspUtilSaveVramSeq(psp_screenshot_path, "game");
#endif
  /* Parse input */
  psp_ctrl_mask_to_index_map_t *current_mapping = physical_to_emulated_button_map;
  for (; current_mapping->mask; current_mapping++)
  {
    u32 code = current_map.button_map[current_mapping->index];
    u8  on = (psp_pad_status.Buttons & current_mapping->mask) == current_mapping->mask;

    /* Check to see if a button set is pressed. If so, unset it, so it */
    /* doesn't trigger any other combination presses. */
    if (on) psp_pad_status.Buttons &= ~current_mapping->mask;

    if ((code & KBD) && !show_kybd_held)
      psp_keyboard_toggle(CODE_MASK(code), on);
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

  return 0;
}

int ui_end( void )
{
  if (Background) pspImageDestroy(Background);
  if (NoSaveIcon) pspImageDestroy(NoSaveIcon);

  pspMenuDestroy(OptionUiMenu.Menu);
  pspMenuDestroy(SystemUiMenu.Menu);
  pspMenuDestroy(SaveStateGallery.Menu);
  pspMenuDestroy(ControlUiMenu.Menu);

  psp_save_options();

  return 0;
}

static inline void psp_keyboard_toggle(unsigned int code, int on)
{
  input_event_t fuse_event;

  fuse_event.type = (on) 
    ? INPUT_EVENT_KEYPRESS : INPUT_EVENT_KEYRELEASE;
  fuse_event.types.key.native_key = code;
  fuse_event.types.key.spectrum_key = code;

  input_event(&fuse_event);
}

static void psp_display_control_tab()
{
  PspMenuItem *item;
  const char *config_name = (GAME_LOADED)
    ? pspFileGetFilename(psp_current_game) : "BASIC";
  char *game_name = strdup(config_name);
  char *dot = strrchr(game_name, '.');
  int i;
  if (dot) *dot='\0';

  /* Load current button mappings */
  for (item = ControlUiMenu.Menu->First, i = 0; item; item = item->Next, i++)
    pspMenuSelectOptionByValue(item, (void*)current_map.button_map[i]);

  pspUiOpenMenu(&ControlUiMenu, game_name);
  free(game_name);
}

static void psp_display_state_tab()
{
  PspMenuItem *item, *sel = NULL;
  SceIoStat stat;
  ScePspDateTime latest;
  char caption[32];
  const char *config_name = (GAME_LOADED)
    ? pspFileGetFilename(psp_current_game) : "BASIC";
  char *path = (char*)malloc(strlen(psp_save_state_path) + strlen(config_name) + 8);
  char *game_name = strdup(config_name);
  char *dot = strrchr(game_name, '.');
  if (dot) *dot='\0';

  memset(&latest,0,sizeof(latest));

  /* Initialize icons */
  for (item = SaveStateGallery.Menu->First; item; item = item->Next)
  {
    sprintf(path, "%s%s_%02i.sta", psp_save_state_path, config_name, item->ID);

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

static void psp_load_options()
{
  char path[PSP_FILE_MAX_PATH_LEN];
  snprintf(path, PSP_FILE_MAX_PATH_LEN-1, "%s%s", pspGetAppDirectory(), "options.ini");

  /* Initialize INI structure */
  PspInit *init = pspInitCreate();

  /* Read the file */
  pspInitLoad(init, path);

  /* Load values */
  psp_options.display_mode = pspInitGetInt(init, "Video", "Display Mode", 
                                           DISPLAY_MODE_UNSCALED);
  psp_options.limit_frames = pspInitGetInt(init, "Video", "Frame Limiter", 1);
  psp_options.clock_freq = pspInitGetInt(init, "Video", "PSP Clock Frequency", 222);
  psp_options.show_fps = pspInitGetInt(init, "Video", "Show FPS", 0);
  psp_options.show_border = pspInitGetInt(init, "Video", "Show Border", 1);
  psp_options.control_mode = pspInitGetInt(init, "Menu", "Control Mode", 0);
  psp_options.animate_menu = pspInitGetInt(init, "Menu", "Animate", 1);
  psp_options.machine = pspInitGetInt(init, "System", "Current Machine", 
                                      LIBSPECTRUM_MACHINE_48);
  pspInitGetStringCopy(init, "File", "Game Path", NULL, 
                       psp_game_path, PSP_FILE_MAX_PATH_LEN-1);

  /* Clean up */
  pspInitDestroy(init);
}

static int psp_save_options()
{
  char path[PSP_FILE_MAX_PATH_LEN];
  snprintf(path, PSP_FILE_MAX_PATH_LEN-1, "%s%s", pspGetAppDirectory(), "options.ini");

  /* Initialize INI structure */
  PspInit *init = pspInitCreate();

  /* Set values */
  pspInitSetInt(init, "Video", "Display Mode", 
                      psp_options.display_mode);
  pspInitSetInt(init, "Video", "Frame Limiter", 
                      psp_options.limit_frames);
  pspInitSetInt(init, "Video", "PSP Clock Frequency", 
                      psp_options.clock_freq);
  pspInitSetInt(init, "Video", "Show FPS", 
                      psp_options.show_fps);
  pspInitSetInt(init, "Video", "Show Border", 
                      psp_options.show_border);
  pspInitSetInt(init, "Menu", "Control Mode", 
                      psp_options.control_mode);
  pspInitSetInt(init, "Menu", "Animate", 
                      psp_options.animate_menu);
  pspInitSetString(init, "File", "Game Path", psp_game_path);
  pspInitSetInt(init, "System", "Current Machine", 
                      machine_current->machine);

  /* Save INI file */
  int status = pspInitSave(init, path);

  /* Clean up */
  pspInitDestroy(init);

  return status;
}

static int psp_load_game(const char *path)
{
  int success, reboot;
  if (pspFileEndsWith(path, "ZIP"))
  {
    char archived_file[512];
    unzFile zipfile = NULL;
    unz_global_info gi;
    unz_file_info fi;

    /* Open archive for reading */
    if (!(zipfile = unzOpen(path)))
      return 0;

    /* Get global ZIP file information */
    if (unzGetGlobalInfo(zipfile, &gi) != UNZ_OK)
    {
      unzClose(zipfile);
      return 0;
    }

    const char *extension;
    utils_file file;
    int i, j;

    for (i = 0; i < (int)gi.number_entry; i++)
    {
      /* Get name of the archived file */
      if (unzGetCurrentFileInfo(zipfile, &fi, archived_file, 
          sizeof(archived_file), NULL, 0, NULL, 0) != UNZ_OK)
      {
        unzClose(zipfile);
        return 0;
      }

      extension = pspFileGetFileExtension(archived_file);
      for (j = 1; *QuickloadFilter[j]; j++)
      {
        if (strcasecmp(QuickloadFilter[j], extension) == 0)
        {
          file.length = fi.uncompressed_size;

          /* Open archived file for reading */
          if(unzOpenCurrentFile(zipfile) != UNZ_OK)
          {
            unzClose(zipfile); 
            return 0;
          }

          if (!(file.buffer = malloc(file.length)))
          {
            unzCloseCurrentFile(zipfile);
            unzClose(zipfile); 
            return 0;
          }

          unzReadCurrentFile(zipfile, file.buffer, file.length);
          unzCloseCurrentFile(zipfile);

          goto close_archive;
        }
      }

      /* Go to the next file in the archive */
      if (i + 1 < (int)gi.number_entry)
      {
        if (unzGoToNextFile(zipfile) != UNZ_OK)
        {
          unzClose(zipfile);
          return 0;
        }
      }
    }

close_archive:
    unzClose(zipfile);

    success = (utils_open_file_buffer(file, archived_file, 0, NULL) == 0);
    reboot = !pspFileEndsWith(archived_file, "Z80");
  }
  else
  {
    success = (utils_open_file(path, 0, NULL) == 0);
    reboot = !pspFileEndsWith(path, "Z80");
  }

  if (reboot) machine_reset(0);
  return success;
}

static void psp_init_controls(psp_ctrl_map_t *config)
{
  /* Initialize to default configuration */
  if (config != &default_map)
    memcpy(config, &default_map, sizeof(psp_ctrl_map_t));
}

static int psp_load_controls(const char *filename, psp_ctrl_map_t *config)
{
  char path[PSP_FILE_MAX_PATH_LEN];
  snprintf(path, PSP_FILE_MAX_PATH_LEN-1, "%s%s.cnf", psp_config_path, filename);

  /* If no configuration, load defaults */
  if (!pspFileCheckIfExists(path))
  {
    psp_init_controls(config);
    return 1;
  }

  /* Open file for reading */
  FILE *file = fopen(path, "r");
  if (!file) return 0;

  /* Read contents of struct */
  int nread = fread(config, sizeof(psp_ctrl_map_t), 1, file);
  fclose(file);

  if (nread != 1)
  {
    psp_init_controls(config);
    return 0;
  }

  return 1;
}

static int psp_save_controls(const char *filename, const psp_ctrl_map_t *config)
{
  char path[PSP_FILE_MAX_PATH_LEN];
  snprintf(path, PSP_FILE_MAX_PATH_LEN-1, "%s%s.cnf", psp_config_path, filename);

  /* Open file for writing */
  FILE *file = fopen(path, "w");
  if (!file) return 0;

  /* Write contents of struct */
  int nwritten = fwrite(config, sizeof(psp_ctrl_map_t), 1, file);
  fclose(file);

  return (nwritten == 1);
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
    if (pspUtilSaveVramSeq(psp_screenshot_path, "UI"))
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
  if (uimenu == &ControlUiMenu)
  {
    /* Save to MS */
    if (psp_save_controls((GAME_LOADED)
                          ? pspFileGetFilename(psp_current_game) : "BASIC", 
                          &current_map))
      pspUiAlert("Changes saved");
    else
      pspUiAlert("ERROR: Changes not saved");
  }
  else if (uimenu == &SystemUiMenu)
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
      if (!pspUtilSavePngSeq(psp_screenshot_path, (GAME_LOADED)
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
  if (uimenu == &ControlUiMenu)
  {
    if (button_mask & PSP_CTRL_SQUARE)
    {
      /* Save to MS as default mapping */
      if (psp_save_controls("DEFAULT", &current_map))
      {
        /* Modify in-memory defaults */
        memcpy(&default_map, &current_map, sizeof(psp_ctrl_map_t));
        pspUiAlert("Changes saved");
      }
      else
        pspUiAlert("ERROR: Changes not saved");

      return 0;
    }
    else if (button_mask & PSP_CTRL_TRIANGLE)
    {
      PspMenuItem *item;
      int i;

      /* Load default mapping */
      memcpy(&current_map, &default_map, sizeof(psp_ctrl_map_t));

      /* Modify the menu */
      for (item = ControlUiMenu.Menu->First, i = 0; item; item = item->Next, i++)
        pspMenuSelectOptionByValue(item, (void*)default_map.button_map[i]);

      return 0;
    }
  }

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
    case OPTION_SHOW_BORDER:
      psp_options.show_border = (int)option->Value;
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
  else if (uimenu == &ControlUiMenu)
  {
    current_map.button_map[item->ID] = (unsigned int)option->Value;
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
  if (!psp_load_game(path))
  {
    pspUiAlert("Error loading file");
    return 0;
  }

  psp_exit_menu = 1;
  display_refresh_all();

  SET_AS_CURRENT_GAME(path);
  pspFileGetParentDirectoryCopy(path, psp_game_path);

  if (!psp_load_controls((GAME_LOADED)
                           ? pspFileGetFilename(psp_current_game) : "BASIC",
                         &current_map));

  /* Reset selected state */
  SaveStateGallery.Menu->Selected = NULL;

  return 1;
}

static int OnSaveStateOk(const void *gallery, const void *item)
{
  char *path;
  const char *config_name = (GAME_LOADED) 
    ? pspFileGetFilename(psp_current_game) : "BASIC";

  path = (char*)malloc(strlen(psp_save_state_path) + strlen(config_name) + 8);
  sprintf(path, "%s%s_%02i.sta", psp_save_state_path, config_name,
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

    path = (char*)malloc(strlen(psp_save_state_path) + strlen(config_name) + 8);
    sprintf(path, "%s%s_%02i.sta", psp_save_state_path, config_name, item->ID);

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

/**************************/
/* PSP main()             */
/**************************/
int main(int argc, char *argv[])
{
  /* Initialize PSP */
  pspAudioInit(SOUND_BUFFER_SIZE, 0);

  /* Main emulation loop */
  fuse_main(argc, argv);

  /* Release PSP resources */
  pspAudioShutdown();
  pspShutdown();

  return(0);
}
