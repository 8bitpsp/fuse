#ifndef PSPUI_H
#define PSPUI_H

#define DISPLAY_MODE_UNSCALED    0
#define DISPLAY_MODE_FIT_HEIGHT  1
#define DISPLAY_MODE_FILL_SCREEN 2

#define MAP_BUTTONS 20

#define KBD 0x10000
#define JST 0x20000
#define SPC 0x40000

#define CODE_MASK(x) (x & 0xffff)

typedef struct psp_options_t
{
  uint8_t  display_mode;
  uint8_t  show_fps;
  uint8_t  limit_frames;
  uint8_t  control_mode;
  uint8_t  animate_menu;
  uint8_t  show_border;
  uint16_t clock_freq;
  uint16_t machine;
} psp_options_t;

typedef struct psp_ctrl_map_t
{
  uint32_t button_map[MAP_BUTTONS];
} psp_ctrl_map_t;

typedef struct psp_ctrl_mask_to_index_map_t
{
  uint64_t mask;
  uint8_t  index;
} psp_ctrl_mask_to_index_map_t;

extern psp_ctrl_map_t current_map;
extern psp_options_t psp_options;
extern uint8_t psp_menu_active;
extern psp_ctrl_mask_to_index_map_t physical_to_emulated_button_map[];

void psp_uidisplay_reinit();

#endif
