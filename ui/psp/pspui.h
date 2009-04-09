#ifndef PSPUI_H
#define PSPUI_H

#define DISPLAY_MODE_UNSCALED    0
#define DISPLAY_MODE_FIT_HEIGHT  1
#define DISPLAY_MODE_FILL_SCREEN 2

#define MAP_BUTTONS         28
#define MAP_SHIFT_START_POS 18 /* Shift buttons start here */

#define KBD 0x10000
#define JST 0x20000
#define SPC 0x40000

#define CODE_MASK(x) (x & 0xffff)

typedef struct psp_options
{
  uint8_t  display_mode;
  uint8_t  show_fps;
  uint8_t  limit_frames;
  uint8_t  control_mode;
  uint8_t  animate_menu;
  uint8_t  enable_bw;
  uint8_t  show_border;
  uint8_t  toggle_vk;
   int8_t  autoload_slot;
  uint16_t clock_freq;
  uint16_t machine;
  uint8_t  show_osi;
  uint8_t  show_pc;
} psp_options_t;

typedef struct psp_ctrl_map
{
  uint32_t button_map[MAP_BUTTONS];
} psp_ctrl_map_t;

typedef struct psp_ctrl_mask_to_index_map
{
  uint64_t mask;
  uint8_t  index;
} psp_ctrl_mask_to_index_map_t;

extern uint8_t keyboard_visible;
extern psp_ctrl_map_t current_map;
extern psp_options_t psp_options;
extern uint8_t psp_menu_active;
extern const psp_ctrl_mask_to_index_map_t physical_to_emulated_button_map[];

void psp_uidisplay_reinit();

#endif
