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
#include "tape.h"
#include "input.h"
#include "joystick.h"
#include "keyboard.h"
#include "string.h"
#include "if1.h"
#include "if2.h"

#include "psp_aux/libpsp/ui.h"
#include "psp_aux/libpsp/font.h"
#include "psp_aux/libpsp/ctrl.h"
#include "pl_snd.h"
#include "pl_file.h"
#include "pl_psp.h"
#include "pl_ini.h"
#include "pl_util.h"
#include "pl_vk.h"

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

#define OPTION_DISPLAY_MODE  0x01
#define OPTION_FRAME_LIMITER 0x02
#define OPTION_CLOCK_FREQ    0x03
#define OPTION_SHOW_FPS      0x04
#define OPTION_CONTROL_MODE  0x05
#define OPTION_ANIMATE       0x06
#define OPTION_AUTOLOAD      0x07
#define OPTION_TOGGLE_VK     0x08
#define OPTION_SHOW_OSI      0x09
#define OPTION_SHOW_PC       0x0A
#define OPTION_SHOW_BORDER   0x0B

#define SYSTEM_SCRNSHOT     0x11
#define SYSTEM_RESET        0x12
#define SYSTEM_TYPE         0x13
#define SYSTEM_MONITOR      0x14
#define SYSTEM_FASTLOAD     0x15
#define SYSTEM_TAPE_TRAPS   0x16
#define SYSTEM_AUTOLOAD     0x17
#define SYSTEM_ISSUE2       0x18
#define SYSTEM_SOUND_LOAD   0x19
#define SYSTEM_TAPE_BROWSER 0x1A
#define SYSTEM_TAPE_PLAY    0x1B
#define SYSTEM_TAPE_REWIND  0x1C

#define SPC_MENU     1
#define SPC_KYBD     2
#define SPC_2X_SPD   3
#define SPC_HALF_SPD 4

#define CURRENT_GAME (psp_current_game)
#define GAME_LOADED (psp_current_game[0] != '\0')
#define SET_AS_CURRENT_GAME(filename) \
  strncpy(psp_current_game, filename, sizeof(psp_current_game) - 1)

static const char 
  PresentSlotText[] = "\026\244\020 Save\t\026\001\020 Load\t\026\243\020 Delete\t\026"PSP_CHAR_START"\020 Export",
  EmptySlotText[]   = "\026\244\020 Save\t\026"PSP_CHAR_START"\020 Export",
  ControlHelpText[] = "\026\250\020 Change mapping\t"
                      "\026\244\020 Set as default\t\026\243\020 Load defaults";

static const char *QuickloadFilter[] =
      { "ZIP", "DCK", "ROM", "MDR", "TAP", "SPC", "STA", "LTP", "TZX",
        "DSK",
        "SCL", "TRD", "HDF", "BZ2", "GZ", "RAW", "CSW",
        "WAV", "MGT", "IMG", "Z80", "RZX", "SNA", "SNP", "SP", "SZX",
        "SLT", "ZXS", "MGTSNP", '\0' };

static int         OnSplashButtonPress(const struct PspUiSplash *splash,
                                       u32 button_mask);
static void        OnSplashRender(const void *uiobject, const void *null);
static const char* OnSplashGetStatusBarText(const struct PspUiSplash *splash);

static int  OnGenericCancel(const void *uiobject, const void* param);
static void OnGenericRender(const void *uiobject, const void *item_obj);
static int  OnGenericButtonPress(const PspUiFileBrowser *browser, const char *path,
                                 u32 button_mask);

static int OnMenuOk(const void *menu, const void *item);
static int OnMenuButtonPress(const struct PspUiMenu *uimenu, pl_menu_item* item,
                             u32 button_mask);
static int OnMenuItemChanged(const struct PspUiMenu *uimenu, pl_menu_item* item,
                             const pl_menu_option* option);

static void OnSystemRender(const void *uiobject, const void *item_obj);

static int OnQuickloadOk(const void *browser, const void *path);

static int OnSaveStateOk(const void *gallery, const void *item);
static int OnSaveStateButtonPress(const PspUiGallery *gallery,
                                  pl_menu_item *sel,
                                  u32 button_mask);

void psp_sound_pause();
void psp_sound_resume();

int snapshot_read_file(const char *filename, FILE *fptr);
int snapshot_write_file(const char *filename, FILE *fptr);

static void psp_display_menu();
static void psp_display_state_tab();
static void psp_display_control_tab();
static void psp_display_system_tab();

static void psp_load_options();
static int  psp_save_options();

static int  psp_load_game(const char *path);

static void psp_init_controls(psp_ctrl_map_t *config);
static int  psp_load_controls(const char *filename, psp_ctrl_map_t *config);
static int  psp_save_controls(const char *filename, const psp_ctrl_map_t *config);

static inline void psp_keyboard_toggle(unsigned int code, int on);
static inline void psp_joystick_toggle(unsigned int code, int on);

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
    OnGenericRender,       /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  },
  SystemUiMenu =
  {
    OnSystemRender,        /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  },
  ControlUiMenu =
  {
    OnGenericRender,       /* OnRender() */
    OnMenuOk,              /* OnOk() */
    OnGenericCancel,       /* OnCancel() */
    OnMenuButtonPress,     /* OnButtonPress() */
    OnMenuItemChanged,     /* OnItemChanged() */
  };

PspUiGallery SaveStateGallery = 
{
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

PL_MENU_OPTIONS_BEGIN(ToggleOptions)
  PL_MENU_OPTION("Disabled", 0)
  PL_MENU_OPTION("Enabled", 1)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(MonitorTypes)
  PL_MENU_OPTION("Color", 0)
  PL_MENU_OPTION("Grayscale", 1)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(ScreenSizeOptions)
  PL_MENU_OPTION("Actual size", DISPLAY_MODE_UNSCALED)
  PL_MENU_OPTION("4:3 scaled (fit height)", DISPLAY_MODE_FIT_HEIGHT)
  PL_MENU_OPTION("16:9 scaled (fit screen)", DISPLAY_MODE_FILL_SCREEN)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(PspClockFreqOptions)
  PL_MENU_OPTION("222 MHz", 222)
  PL_MENU_OPTION("266 MHz", 266)
  PL_MENU_OPTION("300 MHz", 300)
  PL_MENU_OPTION("333 MHz", 333)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(MachineTypes)
  PL_MENU_OPTION("Spectrum 16K",    LIBSPECTRUM_MACHINE_16)
  PL_MENU_OPTION("Spectrum 48K",    LIBSPECTRUM_MACHINE_48)
  PL_MENU_OPTION("Spectrum 128K",   LIBSPECTRUM_MACHINE_128)
  PL_MENU_OPTION("Spectrum +2",     LIBSPECTRUM_MACHINE_PLUS2)
  PL_MENU_OPTION("Spectrum +2A",    LIBSPECTRUM_MACHINE_PLUS2A)
  PL_MENU_OPTION("Spectrum +3",     LIBSPECTRUM_MACHINE_PLUS3)
  PL_MENU_OPTION("Spectrum +3e",    LIBSPECTRUM_MACHINE_PLUS3E)
  PL_MENU_OPTION("Timex TC2048",    LIBSPECTRUM_MACHINE_TC2048)
  PL_MENU_OPTION("Timex TC2068",    LIBSPECTRUM_MACHINE_TC2068)
  PL_MENU_OPTION("Timex TS2068",    LIBSPECTRUM_MACHINE_TS2068)
  PL_MENU_OPTION("Pentagon 128K",   LIBSPECTRUM_MACHINE_PENT)
  PL_MENU_OPTION("Pentagon 512K",   LIBSPECTRUM_MACHINE_PENT512)
  PL_MENU_OPTION("Pentagon 1024K",  LIBSPECTRUM_MACHINE_PENT1024)
  PL_MENU_OPTION("Scorpion ZS 256", LIBSPECTRUM_MACHINE_SCORP)
  PL_MENU_OPTION("Spectrum SE",     LIBSPECTRUM_MACHINE_SE)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(AutoloadSlots)
  PL_MENU_OPTION("Disabled", -1)
  PL_MENU_OPTION("1", 0)
  PL_MENU_OPTION("2", 1)
  PL_MENU_OPTION("3", 2)
  PL_MENU_OPTION("4", 3)
  PL_MENU_OPTION("5", 4)
  PL_MENU_OPTION("6", 5)
  PL_MENU_OPTION("7", 6)
  PL_MENU_OPTION("8", 7)
  PL_MENU_OPTION("9", 8)
  PL_MENU_OPTION("10",9)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(MappableButtons)
  /* Unmapped */
  PL_MENU_OPTION("None", 0)
  /* Special */
  PL_MENU_OPTION("Special: Open Menu",     (SPC|SPC_MENU))
  PL_MENU_OPTION("Special: Show keyboard", (SPC|SPC_KYBD))
/*
  PL_MENU_OPTION("Special: Run at 50% speed", (SPC|SPC_HALF_SPD))
  PL_MENU_OPTION("Special: Run at 200% speed", (SPC|SPC_2X_SPD))
*/
  /* Hack cursors */
  PL_MENU_OPTION("Up",    (KBD|INPUT_KEY_Up))
  PL_MENU_OPTION("Down",  (KBD|INPUT_KEY_Down))
  PL_MENU_OPTION("Left",  (KBD|INPUT_KEY_Left))
  PL_MENU_OPTION("Right", (KBD|INPUT_KEY_Right))
  /* Joystick */
  PL_MENU_OPTION("Joystick Up",    (JST|INPUT_JOYSTICK_UP))
  PL_MENU_OPTION("Joystick Down",  (JST|INPUT_JOYSTICK_DOWN))
  PL_MENU_OPTION("Joystick Left",  (JST|INPUT_JOYSTICK_LEFT))
  PL_MENU_OPTION("Joystick Right", (JST|INPUT_JOYSTICK_RIGHT))
  PL_MENU_OPTION("Joystick Fire 1", (JST|INPUT_JOYSTICK_FIRE_1))
  PL_MENU_OPTION("Joystick Fire 2", (JST|INPUT_JOYSTICK_FIRE_2))
  PL_MENU_OPTION("Joystick Fire 3", (JST|INPUT_JOYSTICK_FIRE_3))
  PL_MENU_OPTION("Joystick Fire 4", (JST|INPUT_JOYSTICK_FIRE_4))
  PL_MENU_OPTION("Joystick Fire 5", (JST|INPUT_JOYSTICK_FIRE_5))
  PL_MENU_OPTION("Joystick Fire 6", (JST|INPUT_JOYSTICK_FIRE_6))
  PL_MENU_OPTION("Joystick Fire 7", (JST|INPUT_JOYSTICK_FIRE_7))
  PL_MENU_OPTION("Joystick Fire 8", (JST|INPUT_JOYSTICK_FIRE_8))
  PL_MENU_OPTION("Joystick Fire 9", (JST|INPUT_JOYSTICK_FIRE_9))
  PL_MENU_OPTION("Joystick Fire 10",(JST|INPUT_JOYSTICK_FIRE_10))
  /* Etc... */
  PL_MENU_OPTION("Space",     (KBD|INPUT_KEY_space))
  PL_MENU_OPTION("Enter",     (KBD|INPUT_KEY_Return))
  /* Numbers */
  PL_MENU_OPTION("0", (KBD|INPUT_KEY_0))
  PL_MENU_OPTION("1", (KBD|INPUT_KEY_1))
  PL_MENU_OPTION("2", (KBD|INPUT_KEY_2))
  PL_MENU_OPTION("3", (KBD|INPUT_KEY_3))
  PL_MENU_OPTION("4", (KBD|INPUT_KEY_4))
  PL_MENU_OPTION("5 / Left", (KBD|INPUT_KEY_5))
  PL_MENU_OPTION("6 / Down", (KBD|INPUT_KEY_6))
  PL_MENU_OPTION("7 / Up", (KBD|INPUT_KEY_7))
  PL_MENU_OPTION("8 / Right", (KBD|INPUT_KEY_8))
  PL_MENU_OPTION("9", (KBD|INPUT_KEY_9))
  /* Alphabet */
  PL_MENU_OPTION("A", (KBD|INPUT_KEY_a))
  PL_MENU_OPTION("B", (KBD|INPUT_KEY_b))
  PL_MENU_OPTION("C", (KBD|INPUT_KEY_c))
  PL_MENU_OPTION("D", (KBD|INPUT_KEY_d))
  PL_MENU_OPTION("E", (KBD|INPUT_KEY_e))
  PL_MENU_OPTION("F", (KBD|INPUT_KEY_f))
  PL_MENU_OPTION("G", (KBD|INPUT_KEY_g))
  PL_MENU_OPTION("H", (KBD|INPUT_KEY_h))
  PL_MENU_OPTION("I", (KBD|INPUT_KEY_i))
  PL_MENU_OPTION("J", (KBD|INPUT_KEY_j))
  PL_MENU_OPTION("K", (KBD|INPUT_KEY_k))
  PL_MENU_OPTION("L", (KBD|INPUT_KEY_l))
  PL_MENU_OPTION("M", (KBD|INPUT_KEY_m))
  PL_MENU_OPTION("N", (KBD|INPUT_KEY_n))
  PL_MENU_OPTION("O", (KBD|INPUT_KEY_o))
  PL_MENU_OPTION("P", (KBD|INPUT_KEY_p))
  PL_MENU_OPTION("Q", (KBD|INPUT_KEY_q))
  PL_MENU_OPTION("R", (KBD|INPUT_KEY_r))
  PL_MENU_OPTION("S", (KBD|INPUT_KEY_s))
  PL_MENU_OPTION("T", (KBD|INPUT_KEY_t))
  PL_MENU_OPTION("U", (KBD|INPUT_KEY_u))
  PL_MENU_OPTION("V", (KBD|INPUT_KEY_v))
  PL_MENU_OPTION("W", (KBD|INPUT_KEY_w))
  PL_MENU_OPTION("X", (KBD|INPUT_KEY_x))
  PL_MENU_OPTION("Y", (KBD|INPUT_KEY_y))
  PL_MENU_OPTION("Z", (KBD|INPUT_KEY_z))
  /* State keys */
  PL_MENU_OPTION("Caps Shift",  (KBD|INPUT_KEY_Shift_R))
  PL_MENU_OPTION("Symbol Shift",(KBD|INPUT_KEY_Super_R))
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(ControlModeOptions)
  PL_MENU_OPTION("\026\242\020 cancels, \026\241\020 confirms (US)", 0)
  PL_MENU_OPTION("\026\241\020 cancels, \026\242\020 confirms (Japan)", 1)
PL_MENU_OPTIONS_END
PL_MENU_OPTIONS_BEGIN(VkModeOptions)
  PL_MENU_OPTION("Display when button is held down (classic mode)", 0)
  PL_MENU_OPTION("Toggle display on and off when button is pressed", 1)
PL_MENU_OPTIONS_END

PL_MENU_ITEMS_BEGIN(SystemMenuDef)
  PL_MENU_HEADER("Video")
  PL_MENU_ITEM("Monitor type",SYSTEM_MONITOR,MonitorTypes,
               "\026\250\020 Select type of monitor (color/grayscale)")
  PL_MENU_HEADER("Tape")
  PL_MENU_ITEM("Play", SYSTEM_TAPE_PLAY, NULL,
               "\026\001\020 Toggle tape playback")
  PL_MENU_ITEM("Rewind", SYSTEM_TAPE_REWIND, NULL,
               "\026\001\020 Rewind tape")
  PL_MENU_ITEM("Browse", SYSTEM_TAPE_BROWSER, NULL,
               "\026\250\020 View/move current tape position (if loaded)")
  PL_MENU_ITEM("Tape traps", SYSTEM_TAPE_TRAPS, ToggleOptions,
               "\026\250\020 Enable/disable tape traps")
  PL_MENU_ITEM("Autoloading",SYSTEM_AUTOLOAD,ToggleOptions,
               "\026\250\020 When enabled, emulator will immediately load and run tape")
  PL_MENU_ITEM("Fastloading",SYSTEM_FASTLOAD,ToggleOptions,
               "\026\250\020 Run at max. speed while loading tapes (does not work with loading sounds)")
  PL_MENU_ITEM("Loading sounds",SYSTEM_SOUND_LOAD,ToggleOptions,
               "\026\250\020 Toogle tape loading sounds (does not work with fastloading)")
  PL_MENU_HEADER("System")
  PL_MENU_ITEM("Machine type",SYSTEM_TYPE,MachineTypes,
               "\026\250\020 Select emulated system")
  PL_MENU_ITEM("Issue 2 keyboard support",SYSTEM_ISSUE2,ToggleOptions,
               "\026\250\020 Enable/disable older keyboard model support")
  PL_MENU_HEADER("Options")
  PL_MENU_ITEM("Reset",SYSTEM_RESET,NULL,
               "\026\001\020 Reset system")
  PL_MENU_ITEM("Save screenshot",SYSTEM_SCRNSHOT,NULL,
               "\026\001\020 Save screenshot")
PL_MENU_ITEMS_END
PL_MENU_ITEMS_BEGIN(OptionMenuDef)
  PL_MENU_HEADER("Video")
  PL_MENU_ITEM("Screen size",OPTION_DISPLAY_MODE,ScreenSizeOptions,
               "\026\250\020 Change screen size")
  PL_MENU_ITEM("Border",OPTION_SHOW_BORDER,ToggleOptions,
               "\026\250\020 Show/hide border surrounding the main display area")
  PL_MENU_HEADER("Input")
  PL_MENU_ITEM("Virtual keyboard mode",OPTION_TOGGLE_VK,VkModeOptions,
               "\026\250\020 Select virtual keyboard mode")
  PL_MENU_HEADER("Enhancements")
  PL_MENU_ITEM("Autoload slot",OPTION_AUTOLOAD,AutoloadSlots,
               "\026\250\020 Select save state to be loaded automatically")
  PL_MENU_HEADER("Performance")
  PL_MENU_ITEM("PSP clock frequency",OPTION_CLOCK_FREQ,PspClockFreqOptions,
               "\026\250\020 Larger values: faster emulation, faster battery depletion (default: 222MHz)")
  PL_MENU_ITEM("Show FPS counter",OPTION_SHOW_FPS,ToggleOptions,
               "\026\250\020 Show/hide the frames-per-second counter")
  PL_MENU_ITEM("Peripheral status",OPTION_SHOW_OSI,ToggleOptions, 
               "\026\250\020 Show/hide floppy, disk drive status indicators")
  PL_MENU_ITEM("Show program counter",OPTION_SHOW_PC,ToggleOptions, 
               "\026\250\020 Show/hide program counter (PC)")
  PL_MENU_HEADER("Menu")
  PL_MENU_ITEM("Button mode",OPTION_CONTROL_MODE,ControlModeOptions,
               "\026\250\020 Change OK and Cancel button mapping")
  PL_MENU_ITEM("Animations",OPTION_ANIMATE,ToggleOptions,
               "\026\250\020 Enable/disable menu animations")
PL_MENU_ITEMS_END
PL_MENU_ITEMS_BEGIN(ControlMenuDef)
  PL_MENU_ITEM(PSP_CHAR_ANALUP,0,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_ANALDOWN,1,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_ANALLEFT,2,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_ANALRIGHT,3,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_UP,4,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_DOWN,5,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LEFT,6,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RIGHT,7,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_SQUARE,8,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_CROSS,9,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_CIRCLE,10,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_TRIANGLE,11,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER,12,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RTRIGGER,13,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_SELECT,14,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_START,15,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_RTRIGGER,16,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_START"+"PSP_CHAR_SELECT,17,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_SELECT,18,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RTRIGGER"+"PSP_CHAR_SELECT,19,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_SQUARE,20,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_CROSS,21,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_CIRCLE,22,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_LTRIGGER"+"PSP_CHAR_TRIANGLE,23,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RTRIGGER"+"PSP_CHAR_SQUARE,24,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RTRIGGER"+"PSP_CHAR_CROSS,25,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RTRIGGER"+"PSP_CHAR_CIRCLE,26,MappableButtons,ControlHelpText)
  PL_MENU_ITEM(PSP_CHAR_RTRIGGER"+"PSP_CHAR_TRIANGLE,27,MappableButtons,ControlHelpText)
PL_MENU_ITEMS_END

/* Default configuration */
static psp_ctrl_map_t default_map =
{
  {
    JST|INPUT_JOYSTICK_UP,    /* Analog Up    */
    JST|INPUT_JOYSTICK_DOWN,  /* Analog Down  */
    JST|INPUT_JOYSTICK_LEFT,  /* Analog Left  */
    JST|INPUT_JOYSTICK_RIGHT, /* Analog Right */
    KBD|INPUT_KEY_Up,    /* D-pad Up     */
    KBD|INPUT_KEY_Down,  /* D-pad Down   */
    KBD|INPUT_KEY_Left,  /* D-pad Left   */
    KBD|INPUT_KEY_Right, /* D-pad Right  */
    KBD|INPUT_KEY_Return,      /* Square       */
    JST|INPUT_JOYSTICK_FIRE_1, /* Cross        */
    KBD|INPUT_KEY_space,   /* Circle       */
    0,                     /* Triangle     */
    0,                     /* L Trigger    */
    SPC|SPC_KYBD,          /* R Trigger    */
    KBD|INPUT_KEY_3,       /* Select       */
    KBD|INPUT_KEY_0,       /* Start        */
    SPC|SPC_MENU,          /* L+R Triggers */
    0,                     /* Start+Select */
    0,                     /* L + Select   */
    0,                     /* R + Select   */
    0,                     /* L + Square   */
    0,                     /* L + Cross    */
    0,                     /* L + Circle   */
    0,                     /* L + Triangle */
    0,                     /* R + Square   */
    0,                     /* R + Cross    */
    0,                     /* R + Circle   */
    0,                     /* R + Triangle */
  }
};

psp_ctrl_map_t current_map;
psp_options_t psp_options;
u8 psp_menu_active;
pl_vk_layout vk_spectrum;
u8 show_kybd_held;
u8 run_full_spd_held;
u8 run_half_spd_held;
u8 keyboard_visible;

static u8 psp_exit_menu;
static int TabIndex;
static PspImage *Background;
static PspImage *NoSaveIcon;
static int psp_controls_changed;

extern PspImage *Screen;
extern int clear_screen;
extern int fuse_exiting;

pl_file_path psp_current_game = {'\0'},
             psp_game_path = {'\0'},
             psp_save_state_path,
             psp_screenshot_path,
             psp_config_path,
             psp_temporary_filename = "";

const psp_ctrl_mask_to_index_map_t physical_to_emulated_button_map[] =
{
  /* These are shift-based (e.g. L/R are not unset when a button pressed) */
  { PSP_CTRL_LTRIGGER | PSP_CTRL_SELECT,   18 },
  { PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT,   19 },
  { PSP_CTRL_LTRIGGER | PSP_CTRL_SQUARE,   20 },
  { PSP_CTRL_LTRIGGER | PSP_CTRL_CROSS,    21 },
  { PSP_CTRL_LTRIGGER | PSP_CTRL_CIRCLE,   22 },
  { PSP_CTRL_LTRIGGER | PSP_CTRL_TRIANGLE, 23 },
  { PSP_CTRL_RTRIGGER | PSP_CTRL_SQUARE,   24 },
  { PSP_CTRL_RTRIGGER | PSP_CTRL_CROSS,    25 },
  { PSP_CTRL_RTRIGGER | PSP_CTRL_CIRCLE,   26 },
  { PSP_CTRL_RTRIGGER | PSP_CTRL_TRIANGLE, 27 },

  /* These are normal */
  { PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER, 16 },
  { PSP_CTRL_START    | PSP_CTRL_SELECT,   17 },
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
  /* Init joystick */
  settings_current.joy_kempston = 1;
  settings_current.joystick_1_output = JOYSTICK_TYPE_KEMPSTON;
  settings_current.sound_freq = 44100;
  settings_current.late_timings = 0;
  settings_current.accelerate_loader = 1;

  /* Initialize callbacks */
  pl_psp_register_callback(PSP_EXIT_CALLBACK,
                           psp_exit_callback,
                           NULL);
  pl_psp_start_callback_thread();

  if (!pl_vk_load(&vk_spectrum,
                  "spectrum.l2",
                  "speckeys.png",
                  NULL,
                  psp_keyboard_toggle))
    return 1;

  /* Initialize paths */
  sprintf(psp_save_state_path, "%sstates/", pl_psp_get_app_directory());
  sprintf(psp_screenshot_path, "ms0:/PSP/PHOTO/%s/", PSP_APP_NAME);
  sprintf(psp_config_path, "%sconfig/", pl_psp_get_app_directory());

  /* Initialize menus */
  pl_menu_create(&OptionUiMenu.Menu, OptionMenuDef);
  pl_menu_create(&SystemUiMenu.Menu, SystemMenuDef);
  pl_menu_create(&ControlUiMenu.Menu, ControlMenuDef);

  /* Load the background image */
  Background = pspImageLoadPng("background.png");

  /* Init NoSaveState icon image */
  NoSaveIcon = pspImageCreate(256, 192, PSP_IMAGE_16BPP);
  pspImageClear(NoSaveIcon, RGB(0x66,0x66,0x66));

  /* Initialize state menu */
  int i;
  pl_menu_item *item;
  for (i = 0; i < 10; i++)
  {
    item = pl_menu_append_item(&SaveStateGallery.Menu, i, NULL);
    pl_menu_set_item_help_text(item, EmptySlotText);
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
  UiMetric.BrowserScreenshotPath = psp_screenshot_path;
  UiMetric.BrowserScreenshotDelay = 30;

  TabIndex = TAB_ABOUT;

  show_kybd_held = 0;
  keyboard_visible = 0;
  run_full_spd_held = 0;
  run_half_spd_held = 0;
  clear_screen = 1;
  psp_menu_active = 1;

  /* Load default configuration */
  psp_load_controls("DEFAULT", &default_map);
  psp_init_controls(&current_map);

  return 0;
}

static void psp_display_menu()
{
  pl_menu_item *item;
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
  pl_psp_set_clock_freq(222);
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
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_DISPLAY_MODE);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.display_mode);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_CLOCK_FREQ);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.clock_freq);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_SHOW_FPS);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.show_fps);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_SHOW_OSI);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.show_osi);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_SHOW_PC);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.show_pc);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_CONTROL_MODE);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.control_mode);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_ANIMATE);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.animate_menu);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_AUTOLOAD);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.autoload_slot);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_TOGGLE_VK);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.toggle_vk);
      if ((item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_FRAME_LIMITER)))
        pl_menu_select_option_by_value(item, (void*)(int)psp_options.limit_frames);
      item = pl_menu_find_item_by_id(&OptionUiMenu.Menu, OPTION_SHOW_BORDER);
      pl_menu_select_option_by_value(item, (void*)(int)psp_options.show_border);

      pspUiOpenMenu(&OptionUiMenu, NULL);
      break;
    case TAB_STATE:
      psp_display_state_tab();
      break;
    case TAB_SYSTEM:
      psp_display_system_tab();
      break;
    case TAB_ABOUT:
      pspUiSplashScreen(&SplashScreen);
      break;
    }
  }

  if (!ExitPSP)
  {
    /* Set clock frequency during emulation */
    pl_psp_set_clock_freq(psp_options.clock_freq);
    /* Set buttons to normal mode */
    pspCtrlSetPollingMode(PSP_CTRL_NORMAL);

    psp_menu_active = 0;
    show_kybd_held = 0;
    keyboard_visible = 0;
    run_full_spd_held = 0;
    run_half_spd_held = 0;
    clear_screen = 1;

    keyboard_release_all();
    psp_uidisplay_reinit();
    psp_sound_resume();
    fuse_emulation_unpause();

    if (psp_options.animate_menu)
      pspUiFadeout();
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

  if (!keyboard_visible)
    keyboard_release_all();
  joystick_release_all(0);

  /* Parse input */
  static SceCtrlData pad;
  if (pspCtrlPollControls(&pad))
  {
    if (keyboard_visible)
      pl_vk_navigate(&vk_spectrum, &pad);

    const psp_ctrl_mask_to_index_map_t *current_mapping = physical_to_emulated_button_map;
    for (; current_mapping->mask; current_mapping++)
    {
      u32 code = current_map.button_map[current_mapping->index];
      u8  on = (pad.Buttons & current_mapping->mask) == current_mapping->mask;

      if (!keyboard_visible)
      {
        if (on)
        {
          if (current_mapping->index < MAP_SHIFT_START_POS)
            /* If a button set is pressed, unset it, so it */
            /* doesn't trigger any other combination presses. */
            pad.Buttons &= ~current_mapping->mask;
          else
            /* Shift mode: Don't unset the L/R; just the rest */
            pad.Buttons &= ~(current_mapping->mask &
                             ~(PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER));
        }

        if (code & KBD)
        {
          if (on) psp_keyboard_toggle(CODE_MASK(code), on);
          continue;
        }
        else if (code & JST)
        {
          if (on) psp_joystick_toggle(CODE_MASK(code), on);
          continue;
        }
      }

      if (code & SPC)
      {
        switch (CODE_MASK(code))
        {
        case SPC_MENU:
          if (on) { psp_display_menu(); return 0; }
          break;
        case SPC_KYBD:
          if (psp_options.toggle_vk)
          {
            if (show_kybd_held != on && on)
            {
              keyboard_visible = !keyboard_visible;
              keyboard_release_all();

              if (keyboard_visible) 
                pl_vk_reinit(&vk_spectrum);
              else clear_screen = 1;
            }
          }
          else
          {
            if (show_kybd_held != on)
            {
              keyboard_visible = on;
              if (on) 
                pl_vk_reinit(&vk_spectrum);
              else
              {
                clear_screen = 1;
                keyboard_release_all();
              }
            }
          }

          show_kybd_held = on;
          break;
        case SPC_HALF_SPD:
          if (run_half_spd_held != on)
            settings_current.emulation_speed = (on) ? 50 : 100;

          run_half_spd_held = on;
          break;
        case SPC_2X_SPD:
          if (run_full_spd_held != on)
            settings_current.emulation_speed = (on) ? 200 : 100;

          run_full_spd_held = on;
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

  pl_menu_destroy(&OptionUiMenu.Menu);
  pl_menu_destroy(&SystemUiMenu.Menu);
  pl_menu_destroy(&SaveStateGallery.Menu);
  pl_menu_destroy(&ControlUiMenu.Menu);

  /* Destroy keyboard */
  pl_vk_destroy(&vk_spectrum);

  if (psp_temporary_filename[0] && pl_file_exists(psp_temporary_filename))
    pl_file_rm(psp_temporary_filename);

  psp_save_options();

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

  fuse_event.type = (on) 
    ? INPUT_EVENT_KEYPRESS : INPUT_EVENT_KEYRELEASE;
  fuse_event.types.key.native_key = code;
  fuse_event.types.key.spectrum_key = code;

  input_event(&fuse_event);
}

static void psp_display_control_tab()
{
  pl_menu_item *item;
  const char *config_name = (GAME_LOADED)
    ? pl_file_get_filename(psp_current_game) : "BASIC";
  char *game_name = strdup(config_name);
  char *dot = strrchr(game_name, '.');
  int i;
  if (dot) *dot='\0';

  /* Load current button mappings */
  for (item = ControlUiMenu.Menu.items, i = 0; item; item = item->next, i++)
    pl_menu_select_option_by_value(item, (void*)current_map.button_map[i]);

  psp_controls_changed = 0;

  pspUiOpenMenu(&ControlUiMenu, game_name);
  free(game_name);

  /* Save controls, if any changes were made */
  if (psp_controls_changed && !psp_save_controls(config_name, &current_map))
    pspUiAlert("ERROR: Changes not saved");
}

static void add_block_details(libspectrum_tape_block *block, void *user_data)
{
  /* Get tape position data */
  char caption[256];
  tape_block_details(caption, 255, block);

  /* Add option */
  pl_menu_item *item = (pl_menu_item*)user_data;
  pl_menu_append_option(item, caption, 
                        (void*)pl_menu_get_option_count(item), 0);
}

static void psp_display_system_tab()
{
  pl_menu_item *item;

  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_MONITOR);
  pl_menu_select_option_by_value(item, (void*)(int)psp_options.enable_bw);
  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_TYPE);
  pl_menu_select_option_by_value(item, (void*)(machine_current->machine));
  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_FASTLOAD);
  pl_menu_select_option_by_value(item, (void*)(settings_current.fastload));
  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_AUTOLOAD);
  pl_menu_select_option_by_value(item, (void*)(settings_current.auto_load));
  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_ISSUE2);
  pl_menu_select_option_by_value(item, (void*)(settings_current.issue2));
  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_SOUND_LOAD);
  pl_menu_select_option_by_value(item, (void*)(settings_current.sound_load));
  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_TAPE_TRAPS);
  pl_menu_select_option_by_value(item, (void*)(settings_current.tape_traps));

  /* Initialize tape browser information */
  item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_TAPE_BROWSER);
  tape_foreach(add_block_details, item);

  /* Select currently selected block */
  int current_block = tape_get_current_block();
  if (current_block != -1)
    pl_menu_select_option_by_index(item, current_block);

  pspUiOpenMenu(&SystemUiMenu, NULL);

  /* Clear list of options */
  pl_menu_clear_options(item);
}

static void psp_display_state_tab()
{
  pl_menu_item *item, *sel = NULL;
  SceIoStat stat;
  ScePspDateTime latest;
  char caption[32];
  const char *config_name = (GAME_LOADED)
    ? pl_file_get_filename(psp_current_game) : "BASIC";
  char *path = (char*)malloc(strlen(psp_save_state_path) + strlen(config_name) + 8);
  char *game_name = strdup(config_name);
  char *dot = strrchr(game_name, '.');
  if (dot) *dot='\0';

  memset(&latest,0,sizeof(latest));

  /* Initialize icons */
  for (item = SaveStateGallery.Menu.items; item; item = item->next)
  {
    sprintf(path, "%s%s_%02i.sta", psp_save_state_path, config_name, item->id);

    if (pl_file_exists(path))
    {
      if (sceIoGetstat(path, &stat) < 0)
        sprintf(caption, "ERROR");
      else
      {
        /* Determine the latest save state */
        if (pl_util_date_compare(&latest, &stat.st_mtime) < 0)
        {
          sel = item;
          latest = stat.st_mtime;
        }

        sprintf(caption, "%02i/%02i/%02i %02i:%02i%s", 
          stat.st_mtime.month,
          stat.st_mtime.day,
          stat.st_mtime.year - (stat.st_mtime.year / 100) * 100,
          stat.st_mtime.hour,
          stat.st_mtime.minute,
          ((int)item->id == psp_options.autoload_slot) ? "*" : "");
      }

      pl_menu_set_item_caption(item, caption);
      item->param = psp_load_state_icon(path);
      pl_menu_set_item_help_text(item, PresentSlotText);
    }
    else
    {
      pl_menu_set_item_caption(item, ((int)item->id == psp_options.autoload_slot)
          ? "Autoload" : "Empty");
      item->param = NoSaveIcon;
      pl_menu_set_item_help_text(item, EmptySlotText);
    }
  }

  free(path);

  /* Highlight the latest save state if none are selected */
  if (SaveStateGallery.Menu.selected == NULL)
    SaveStateGallery.Menu.selected = sel;

  pspUiOpenGallery(&SaveStateGallery, game_name);
  free(game_name);

  /* Destroy any icons */
  for (item = SaveStateGallery.Menu.items; item; item = item->next)
    if (item->param != NULL && item->param != NoSaveIcon)
      pspImageDestroy((PspImage*)item->param);
}

/* Load state icon */
static PspImage* psp_load_state_icon(const char *path)
{
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

/* Save state */
static int psp_export_state(const char *path)
{
  /* Open file for writing */
  FILE *f;
  if (!(f = fopen(path, "w")))
    return 0;

  /* Write the state */
  int status = 0;
  if (snapshot_write_file(path, f) == 0)
    status = 1;

  fclose(f);
  return status;
}

static void psp_load_options()
{
  pl_file_path path;
  snprintf(path, sizeof(path) - 1, "%s%s", pl_psp_get_app_directory(), "options.ini");

  /* Load INI */
  pl_ini_file file;
  pl_ini_load(&file, path);

  psp_options.display_mode = pl_ini_get_int(&file, "Video", "Display Mode", 
                                            DISPLAY_MODE_UNSCALED);
  psp_options.limit_frames = pl_ini_get_int(&file, "Video", "Frame Limiter", 1);
  psp_options.clock_freq = pl_ini_get_int(&file, "Video", "PSP Clock Frequency", 222);
  psp_options.show_fps = pl_ini_get_int(&file, "Video", "Show FPS", 0);
  psp_options.show_osi = pl_ini_get_int(&file, "Video", "Show Peripheral Status", 0);
  psp_options.show_pc = pl_ini_get_int(&file, "Options", "Show PC", 0);
  psp_options.show_border = pl_ini_get_int(&file, "Video", "Show Border", 1);
  psp_options.enable_bw = pl_ini_get_int(&file, "Video", "Enable B&W", 0);
  psp_options.control_mode = pl_ini_get_int(&file, "Menu", "Control Mode", 0);
  psp_options.animate_menu = pl_ini_get_int(&file, "Menu", "Animate", 1);
  psp_options.toggle_vk = pl_ini_get_int(&file, "Input", "VK Mode", 0);
  psp_options.machine = pl_ini_get_int(&file, "System", "Current Machine", 
                                      LIBSPECTRUM_MACHINE_48);
  psp_options.autoload_slot = pl_ini_get_int(&file, "System", "Autoload Slot", 9);
  pl_ini_get_string(&file, "File", "Game Path", NULL, 
                    psp_game_path, sizeof(psp_game_path));

  settings_current.fastload = pl_ini_get_int(&file, "System", "Fastload", 1);
  settings_current.auto_load = pl_ini_get_int(&file, "System", "Autoload", 1);
  settings_current.issue2 = pl_ini_get_int(&file, "System", "Issue2", 0);
  settings_current.sound_load = pl_ini_get_int(&file, "System", "Loading Sound", 1);
  settings_current.tape_traps = pl_ini_get_int(&file, "System", "Tape Traps", 1);

  /* Clean up */
  pl_ini_destroy(&file);
}

static int psp_save_options()
{
  pl_file_path path;
  snprintf(path, sizeof(path)-1, "%s%s", pl_psp_get_app_directory(), "options.ini");

  /* Initialize INI structure */
  pl_ini_file file;
  pl_ini_create(&file);
  pl_ini_set_int(&file, "Video", "Display Mode", psp_options.display_mode);
  pl_ini_set_int(&file, "Video", "Frame Limiter", psp_options.limit_frames);
  pl_ini_set_int(&file, "Video", "PSP Clock Frequency", psp_options.clock_freq);
  pl_ini_set_int(&file, "Video", "Show FPS", psp_options.show_fps);
  pl_ini_set_int(&file, "Video", "Show Peripheral Status", psp_options.show_osi);
  pl_ini_set_int(&file, "Video", "Show Border", psp_options.show_border);
  pl_ini_set_int(&file, "Video", "Enable B&W", psp_options.enable_bw);
  pl_ini_set_int(&file, "Options", "Show PC", psp_options.show_pc);
  pl_ini_set_int(&file, "Menu", "Control Mode", psp_options.control_mode);
  pl_ini_set_int(&file, "Menu", "Animate", psp_options.animate_menu);
  pl_ini_set_int(&file, "Input", "VK Mode", psp_options.toggle_vk);
  pl_ini_set_int(&file, "System", "Current Machine", machine_current->machine);
  pl_ini_set_int(&file, "System", "Autoload Slot", psp_options.autoload_slot);
  pl_ini_set_int(&file, "System", "Fastload", settings_current.fastload);
  pl_ini_set_int(&file, "System", "Autoload", settings_current.auto_load);
  pl_ini_set_int(&file, "System", "Issue2", settings_current.issue2);
  pl_ini_set_int(&file, "System", "Loading Sound", settings_current.sound_load);
  pl_ini_set_int(&file, "System", "Tape Traps", settings_current.tape_traps);
  pl_ini_set_string(&file, "File", "Game Path", psp_game_path);

  int status = pl_ini_save(&file, path);
  pl_ini_destroy(&file);

  return status;
}

static int psp_load_game(const char *path)
{
  libspectrum_id_t type;
  libspectrum_class_t class;
  utils_file file;
  const char *game_path = path;
  int is_compressed = 0;

  /* Eject disks/tapes */
  if (tape_present()) tape_close();
  specplus3_disk_eject(SPECPLUS3_DRIVE_A, 0);
  beta_disk_eject(BETA_DRIVE_A, 0);
  plusd_disk_eject(PLUSD_DRIVE_1, 0);

  if (pl_file_is_of_type(path, "ZIP"))
  {
    is_compressed = 1;
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

      extension = pl_file_get_extension(archived_file);
      for (j = 1; QuickloadFilter[j]; j++)
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

    /* No eligible files */
    return 0;

close_archive:
    unzClose(zipfile);

    game_path = archived_file;
  }

  int error = 1;
  if (!is_compressed)
  {
    /* Uncompressed; business as usual */
    error = utils_open_file(game_path, 
                            tape_can_autoload(),
                            NULL);
  }
  else
  {
    /* Compressed; first, identify file type */
    error = libspectrum_identify_file_with_class(&type, 
                                                 &class, 
                                                 game_path,
                                                 file.buffer, 
                                                 file.length);

    if (error == 0)
    {
      if (class == LIBSPECTRUM_CLASS_DISK_TRDOS
       || class == LIBSPECTRUM_CLASS_DISK_PLUS3
       || class == LIBSPECTRUM_CLASS_DISK_PLUSD
       || class == LIBSPECTRUM_CLASS_DISK_GENERIC)
      {
        if (psp_temporary_filename[0] 
         && pl_file_exists(psp_temporary_filename))
        {
          /* Remove earlier temporary file */
          pl_file_rm(psp_temporary_filename);
          psp_temporary_filename[0] = '\0';
        }

        /* Define temp filename */
        sprintf(psp_temporary_filename, "%st%04i_%s", 
                pl_psp_get_app_directory(),
                rand() % 10000,
                game_path);

        /* Disk image; extract to a temporary location and open */
        FILE *temp_file = fopen(psp_temporary_filename, "w");

        if (!temp_file)
          error = 1;
        else
        {
          if (!fwrite(file.buffer, file.length, 1, temp_file))
            error = 1;
          fclose(temp_file);

          if (error == 0)
            error = utils_open_file(psp_temporary_filename,
                                    tape_can_autoload(),
                                    NULL);
        }
      }
      else
      {
        /* Compressed, but basic; open from buffer */
        error = utils_open_file_buffer(file, 
                                       game_path, 
                                       tape_can_autoload(),
                                       NULL);
      }
    }

    /* Free file resource */
    utils_close_file(&file);
  }

  if (error)
    return 0;

  if (class != LIBSPECTRUM_CLASS_SNAPSHOT && !tape_can_autoload())
    machine_reset(0);

  return 1;
}

static void psp_init_controls(psp_ctrl_map_t *config)
{
  /* Initialize to default configuration */
  if (config != &default_map)
    memcpy(config, &default_map, sizeof(psp_ctrl_map_t));
}

static int psp_load_controls(const char *filename, psp_ctrl_map_t *config)
{
  pl_file_path path;
  snprintf(path, sizeof(path), "%s%s.cnf", psp_config_path, filename);

  /* Initialize default controls */
  psp_init_controls(config);

  /* No configuration; defaults are fine */
  if (!pl_file_exists(path))
    return 1;

  /* Open file for reading */
  FILE *file = fopen(path, "r");
  if (!file) return 0;

  /* Load defaults; attempt to read controls from file */
  psp_init_controls(config);
  int nread = fread(config, sizeof(uint32_t), MAP_BUTTONS, file);

  fclose(file);

  /* Reading less than MAP_BUTTONS is ok; may be an older config file */
  if (nread < 1)
  {
    psp_init_controls(config);
    return 0;
  }

  return 1;
}

static int psp_save_controls(const char *filename, const psp_ctrl_map_t *config)
{
  pl_file_path path;
  snprintf(path, sizeof(path)-1, "%s%s.cnf", psp_config_path, filename);

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
    if (pl_util_save_vram_seq(psp_screenshot_path, "UI"))
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
    "2008-2009 Akop Karapetyan",
    "2003-2008 Philip Kendall & Fuse team",
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
  switch (((const pl_menu_item*)sel_item)->id)
  {
  case SYSTEM_TAPE_REWIND:
    tape_select_block(0);
    break;
  case SYSTEM_TAPE_PLAY:
    tape_toggle_play(0);
    break;
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
    if (!pl_util_save_image_seq(psp_screenshot_path, (GAME_LOADED)
                                ? pl_file_get_filename(psp_current_game) : "BASIC",
                                Screen))
      pspUiAlert("ERROR: Screenshot not saved");
    else
      pspUiAlert("Screenshot saved successfully");
    break;
  }

  return 0;
}

static int OnMenuButtonPress(const struct PspUiMenu *uimenu,
                             pl_menu_item* sel_item,
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
      pl_menu_item *item;
      int i;

      /* Load default mapping */
      memcpy(&current_map, &default_map, sizeof(psp_ctrl_map_t));

      /* Modify the menu */
      for (item = ControlUiMenu.Menu.items, i = 0; item; item = item->next, i++)
        pl_menu_select_option_by_value(item, (void*)default_map.button_map[i]);

      return 0;
    }
  }

  return OnGenericButtonPress(NULL, NULL, button_mask);
}

static int OnMenuItemChanged(const struct PspUiMenu *uimenu,
                             pl_menu_item* item,
                             const pl_menu_option* option)
{
  if (uimenu == &ControlUiMenu)
  {
    current_map.button_map[item->id] = (unsigned int)option->value;
    psp_controls_changed = 1;
  }
  else
  {
    switch((int)item->id)
    {
    case OPTION_DISPLAY_MODE:
      psp_options.display_mode = (int)option->value;
      break;
    case OPTION_FRAME_LIMITER:
      psp_options.limit_frames = (int)option->value;
      break;
    case OPTION_CLOCK_FREQ:
      psp_options.clock_freq = (int)option->value;
      break;
    case OPTION_SHOW_FPS:
      psp_options.show_fps = (int)option->value;
      break;
    case OPTION_SHOW_OSI:
      psp_options.show_osi = (int)option->value;
      break;
    case OPTION_SHOW_PC:
      psp_options.show_pc = (int)option->value;
      break;
    case OPTION_SHOW_BORDER:
      psp_options.show_border = (int)option->value;
      break;
    case OPTION_TOGGLE_VK:
      psp_options.toggle_vk = (int)option->value;
      break;
    case OPTION_CONTROL_MODE:
      psp_options.control_mode = (int)option->value;
      UiMetric.OkButton = (!psp_options.control_mode) ? PSP_CTRL_CROSS : PSP_CTRL_CIRCLE;
      UiMetric.CancelButton = (!psp_options.control_mode) ? PSP_CTRL_CIRCLE : PSP_CTRL_CROSS;
      break;
    case OPTION_ANIMATE:
      psp_options.animate_menu = (int)option->value;
      UiMetric.Animate = psp_options.animate_menu;
      break;
    case OPTION_AUTOLOAD:
      psp_options.autoload_slot = (int)option->value;
      break;
    case SYSTEM_MONITOR:
      psp_options.enable_bw = (int)option->value;
      psp_uidisplay_reinit();
      break;
    case SYSTEM_TYPE:
      if (machine_current->machine == (int)option->value
          || !pspUiConfirm("This will reset the system. Proceed?"))
        return 0;
      machine_select((int)option->value);
      break;
    case SYSTEM_FASTLOAD:
      settings_current.fastload = (int)option->value;
      if (settings_current.sound_load && settings_current.fastload)
      {
        settings_current.sound_load = 0;
        pl_menu_item *item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, 
                                                     SYSTEM_SOUND_LOAD);
        pl_menu_select_option_by_value(item, 
                                       (void*)(settings_current.sound_load));
      }
      break;
    case SYSTEM_AUTOLOAD:
      settings_current.auto_load = (int)option->value;
      break;
    case SYSTEM_ISSUE2:
      settings_current.issue2 = (int)option->value;
      break;
    case SYSTEM_TAPE_TRAPS:
      settings_current.tape_traps = (int)option->value;
      break;
    case SYSTEM_SOUND_LOAD:
      settings_current.sound_load = (int)option->value;
      if (settings_current.sound_load && settings_current.fastload)
      {
        settings_current.fastload = 0;
        pl_menu_item *item = pl_menu_find_item_by_id(&SystemUiMenu.Menu, SYSTEM_FASTLOAD);
        pl_menu_select_option_by_value(item, (void*)(settings_current.fastload));
      }
      break;
    case SYSTEM_TAPE_BROWSER:
      /* Reposition tape */
      if ((int)option->value != tape_get_current_block())
        tape_select_block_no_update((int)option->value);
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
  pl_file_get_parent_directory(path,
                               psp_game_path,
                               sizeof(psp_game_path));

  if (!psp_load_controls((GAME_LOADED)
                           ? pl_file_get_filename(psp_current_game) : "BASIC",
                         &current_map));

  /* Autoload saved state */
  if (psp_options.autoload_slot >= 0)
  {
    const char *config_name = (GAME_LOADED)
                              ? pl_file_get_filename(psp_current_game) : "BASIC";
    pl_file_path state_file;
    snprintf(state_file, sizeof(state_file) - 1, 
             "%s%s_%02i.sta", psp_save_state_path, config_name, 
             psp_options.autoload_slot);

    /* Attempt loading saved state (don't care if fails) */
    psp_load_state(state_file);
  }

  /* Reset selected state */
  SaveStateGallery.Menu.selected = NULL;

  return 1;
}

static int OnSaveStateOk(const void *gallery, const void *item)
{
  char *path;
  const char *config_name = (GAME_LOADED) 
    ? pl_file_get_filename(psp_current_game) : "BASIC";

  path = (char*)malloc(strlen(psp_save_state_path) + strlen(config_name) + 8);
  sprintf(path, "%s%s_%02i.sta", psp_save_state_path, config_name,
    ((const pl_menu_item*)item)->id);

  if (pl_file_exists(path) && pspUiConfirm("Load state?"))
  {
    if (psp_load_state(path))
    {
      psp_exit_menu = 1;
      pl_menu_find_item_by_id(&((PspUiGallery*)gallery)->Menu,
        ((pl_menu_item*)item)->id);
      free(path);

      return 1;
    }

    pspUiAlert("ERROR: State failed to load\nSee documentation for possible reasons");
  }

  free(path);
  return 0;
}

static int OnSaveStateButtonPress(const PspUiGallery *gallery, 
                                  pl_menu_item *sel,
                                  u32 button_mask)
{
  if (button_mask & PSP_CTRL_SQUARE 
    || button_mask & PSP_CTRL_TRIANGLE
    || button_mask & PSP_CTRL_START)
  {
    char *path;
    char caption[32];
    const char *config_name = (GAME_LOADED) 
      ? pl_file_get_filename(psp_current_game) : "BASIC";
    pl_menu_item *item = pl_menu_find_item_by_id(&gallery->Menu, sel->id);

    path = (char*)malloc(strlen(psp_save_state_path) + strlen(config_name) + 8);
    sprintf(path, "%s%s_%02i.sta", psp_save_state_path, config_name, item->id);

    do /* not a real loop; flow control construct */
    {
      if (button_mask & PSP_CTRL_SQUARE)
      {
        if (pl_file_exists(path) && !pspUiConfirm("Overwrite existing state?"))
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
        if (item->param && item->param != NoSaveIcon)
          pspImageDestroy((PspImage*)item->param);

        /* Update icon, help text */
        item->param = icon;
        pl_menu_set_item_help_text(item, PresentSlotText);

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

        pl_menu_set_item_caption(item, caption);
      }
      else if (button_mask & PSP_CTRL_START)
      {
        /* Set up path */
        pl_file_path plain_state_path;
        snprintf(plain_state_path, sizeof(plain_state_path) - 1,
          "%s%s_%02i.sna", psp_save_state_path, config_name, item->id);

        /* Export state */
        if (!psp_export_state(plain_state_path))
          pspUiAlert("ERROR: State not exported");
        else pspUiAlert("State exported successfully");
      }
      else if (button_mask & PSP_CTRL_TRIANGLE)
      {
        if (!pl_file_exists(path) || !pspUiConfirm("Delete state?"))
          break;

        if (!pl_file_rm(path))
        {
          pspUiAlert("ERROR: State not deleted");
          break;
        }

        /* Trash the old icon (if any) */
        if (item->param && item->param != NoSaveIcon)
          pspImageDestroy((PspImage*)item->param);

        /* Update icon, caption */
        item->param = NoSaveIcon;
        pl_menu_set_item_help_text(item, EmptySlotText);
        pl_menu_set_item_caption(item, ((int)item->id == psp_options.autoload_slot)
            ? "Autoload" : "Empty");
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

int fuse_main(int argc, char *argv[]);

/**************************/
/* PSP main()             */
/**************************/
int main(int argc, char *argv[])
{
  /* Initialize PSP */
  pl_psp_init(argv[0]);
  pspCtrlInit();
  pspVideoInit();
  pl_snd_init(SOUND_BUFFER_SIZE, 0);

  /* Main emulation loop */
  fuse_main(argc, argv);

  /* Release PSP resources */
  pl_snd_shutdown();
  pspVideoShutdown();
  pl_psp_shutdown();

  return(0);
}
