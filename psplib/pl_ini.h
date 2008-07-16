/** PSP helper library ***************************************/
/**                                                         **/
/**                      pl_ini.h                           **/
/**                                                         **/
/** This file contains declarations for INI file loading/   **/
/** saving                                                  **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PL_INI_H
#define _PL_INI_H

#ifdef __cplusplus
extern "C" {
#endif

struct pl_ini_section_t;

typedef struct pl_ini_file_t
{
  struct pl_ini_section_t *head;
} pl_ini_file;

int  pl_ini_create(pl_ini_file *file);
void pl_ini_destroy(pl_ini_file *file);
int  pl_ini_load(pl_ini_file *file,
                 const char *path);
int  pl_ini_save(const pl_ini_file *file,
                 const char *path);
int  pl_ini_get_int(const pl_ini_file *file,
                    const char *section,
                    const char *key,
                    const int default_value);
void pl_ini_set_int(pl_ini_file *file,
                    const char *section,
                    const char *key,
                    int value);
int  pl_ini_get_string(const pl_ini_file *file,
                       const char *section,
                       const char *key,
                       const char *default_value,
                       char *copy_to,
                       int dest_len);
void pl_ini_set_string(pl_ini_file *file,
                       const char *section,
                       const char *key,
                       const char *string);

#ifdef __cplusplus
}
#endif

#endif // _PL_INI_H
