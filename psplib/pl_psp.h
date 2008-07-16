/** PSP helper library ***************************************/
/**                                                         **/
/**                          psp.h                          **/
/**                                                         **/
/** This file contains declarations for core PSP management **/
/** routines, such as program startup/shutdown, callbacks,  **/
/** etc...                                                  **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PL_PSP_H
#define _PL_PSP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  PSP_EXIT_CALLBACK
} pl_callback_type;

extern int ExitPSP;

int  pl_psp_init(const char *app_path);
void pl_psp_shutdown();
void pl_psp_set_clock_freq(int freq);
const char* pl_psp_get_app_directory();

int pl_psp_register_callback(pl_callback_type type,
                             void (*func)(void *param),
                             void *param);
int pl_psp_start_callback_thread();

#ifdef __cplusplus
}
#endif

#endif // _PL_PSP_H
