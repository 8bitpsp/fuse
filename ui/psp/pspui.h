#ifndef PSPUI_H
#define PSPUI_H

#define DISPLAY_MODE_UNSCALED    0
#define DISPLAY_MODE_FIT_HEIGHT  1
#define DISPLAY_MODE_FILL_SCREEN 2

typedef struct psp_options_t
{
  unsigned char  display_mode;
  unsigned char  show_fps;
  unsigned char  limit_frames;
  unsigned char  control_mode;
  unsigned char  animate_menu;
  unsigned char  show_border;
  unsigned short clock_freq;
  unsigned short machine;
} psp_options_t;

extern psp_options_t psp_options;
extern unsigned char psp_menu_active;

void psp_uidisplay_reinit();

#endif
