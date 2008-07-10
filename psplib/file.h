/** PSP helper library ***************************************/
/**                                                         **/
/**                        file.h                         **/
/**                                                         **/
/** This file contains declarations for the file management **/
/** routines                                                **/
/**                                                         **/
/** Copyright (C) Akop Karapetyan 2007                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef _PSP_FILE_H
#define _PSP_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_FILE_DIR          0x01
#define PSP_FILE_MAX_PATH_LEN 1024

struct PspFileFile
{
  char *Name;
  struct PspFileFile *Next;
  unsigned int Attrs;
};

typedef struct PspFileFile PspFile;

struct PspFileFileList
{
  int Count;
  struct PspFileFile *First;
  struct PspFileFile *Last;
};

typedef struct PspFileFileList PspFileList;

PspFileList* pspFileGetFileList(const char* path, const char **filter);
void         pspFileDestroyFileList(PspFileList* list);
void         pspFileEnterDirectory(char **cur_path, char *dir);
char*        pspFileGetParentDirectory(const char *path);
const char*  pspFileGetFilename(const char *path);
const char*  pspFileGetFileExtension(const char *path);
int          pspFileGetFileSize(const char *path);
int          pspFileIsRootDirectory(const char *path);
int          pspFileCheckIfExists(const char *path);
int          pspFileEndsWith(const char *filename, const char *ext);
int          pspFileDelete(const char *path);

#ifdef __cplusplus
}
#endif

#endif // _PSP_FILE_H
