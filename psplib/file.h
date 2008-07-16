/* psplib/file.h
   File and directory query routines

   Copyright (C) 2007-2008 Akop Karapetyan

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: pspdev@akop.org
*/

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
void         pspFileGetParentDirectoryCopy(const char *path, 
                                           char *parent);
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
