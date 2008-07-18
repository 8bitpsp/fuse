/* psplib/pl_file.c
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

#include <pspkernel.h>
#include <psptypes.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pl_file.h"

static void sort_file_list(pl_file_list *list);
static int  compare_files_by_name(const void *s1, const void *s2);

void pl_file_get_parent_directory(const char *path,
                                  pl_file_path parent)
{
  if (strlen(path) >= PL_FILE_MAX_PATH_LEN)
    return;

  char *pos;
  if (!(pos = strrchr(path, '/')))
  {
    strcpy(parent, path);
    return;
  }

  strncpy(parent, path, pos - path + 1);
  parent[pos - path + 1] = '\0';
}

const char* pl_file_get_filename(const char *path)
{
  const char *filename;
  if (!(filename = strrchr(path, '/')))
    return path;
  return filename + 1;
}

const char* pl_file_get_extension(const char *path)
{
  const char *filename = pl_file_get_filename(path);
  const char *ext;
  if (!(ext = strrchr(filename, '.')))
    return filename + strlen(filename);
  return ext + 1;
}

int pl_file_rm(const char *path)
{
  return sceIoRemove(path) >= 0;
}

int pl_file_get_file_size(const char *path)
{
  /* TODO: there's probably a better way to do this */
  FILE *file; 
  if (!(file = fopen(path, "r")))
    return 0;

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  fclose(file);

  return size;
}

int pl_file_is_root_directory(const char *path)
{
  const char *pos = strchr(path, '/');
  return !pos || !(*(pos + 1));
}

int pl_file_exists(const char *path)
{
  SceIoStat stat;
  memset(&stat, 0, sizeof(stat));
  if (sceIoGetstat(path, &stat) < 0)
    return 0;

  return 1;
}

int pl_file_is_of_type(const char *path,
                       const char *extension)
{
  int fn_len, ext_len;
  const char *file_ext;

  fn_len = strlen(path);
  ext_len = strlen(extension);

  /* Filename must be at least 2 chars longer (period + a char) */
  if (fn_len < ext_len + 2)
    return 0;

  file_ext = path + (fn_len - ext_len);
  if (*(file_ext - 1) == '.' && strcasecmp(file_ext, extension) == 0)
    return 1;

  return 0;
}

void pl_file_destroy_file_list(pl_file_list *list)
{
  pl_file *file, *next;

  for (file = list->files; file; file = next)
  {
    next = file->next;
    free(file->name);
    free(file);
  }
}

int pl_file_get_file_list_count(const pl_file_list *list)
{
  int count = 0;
  pl_file *file;

  for (file = list->files; file; file = file->next)
    count++;
  return count;
}

/* Returns number of files successfully read; negative number if error */
int pl_file_get_file_list(pl_file_list *list,
                          const char *path,
                          const char **filter)
{
  SceUID fd = sceIoDopen(path);

  if (fd < 0) return 0;

  SceIoDirent dir;
  memset(&dir, 0, sizeof(dir));

  PspFile *file = 0;
  PspFileList *list = (PspFileList*)malloc(sizeof(PspFileList));
  const char **pext;
  int loop;

  list->Count = 0;
  list->First = 0;
  list->Last = 0;

  while (sceIoDread(fd, &dir) > 0)
  {
    if (filter && !(dir.d_stat.st_attr & FIO_SO_IFDIR))
    {
      /* Loop through the list of allowed extensions and compare */
      for (pext = filter, loop = 1; *pext; pext++)
      {
        if (pspFileEndsWith(dir.d_name, *pext))
        {
          loop = 0;
          break;
        }
      }

      if (loop) continue;
    }

    // Create a new file entry

    file = (PspFile*)malloc(sizeof(PspFile));
    file->Name = strdup(dir.d_name);
    file->Next = 0;
    file->Attrs = (dir.d_stat.st_attr & FIO_SO_IFDIR) ? PSP_FILE_DIR : 0;

    list->Count++;

    // Update preceding element

    if (list->Last)
    {
      list->Last->Next = file;
      list->Last = file;
    }
    else
    {
      list->First = file;
      list->Last = file;
    }
  }

  sceIoDclose(fd);

  // Sort the files by name

  _pspFileSortFileList(list);

  return list;
}

static void sort_file_list(pl_file_list *list)
{
  pl_file **files, *file, *fp;
  int i, count;

  count = pl_file_get_file_list_count(list);
  if (count < 1)
    return;

  /* Copy the file entries to an array */
  files = (pl_file**)malloc(sizeof(pl_file*) * count);
  for (file = list->files, fp = *files; file; file = file->next, i++, fp++)
    fp = file;

  /* Sort the array */
  qsort((void*)files, count, sizeof(pl_file*), compare_files_by_name);

  /* Rearrange the file entries in the list */
  list->files = files[0];
  list->files->next = NULL;

  for (i = 1; i < count; i++)
    files[i - 1]->next = files[i];

  pl_file *last = files[count - 1];
  last->next = NULL;
  free(files);
}

static int compare_files_by_name(const void *s1, const void *s2)
{
  pl_file *f1 = *(pl_file**)s1, *f2 = *(pl_file**)s2;
  if ((f1->attrs & PL_FILE_DIRECTORY) == (f2->attrs & PL_FILE_DIRECTORY))
    return strcasecmp(f1->name, f2->name);
  else if (f1->attrs & PL_FILE_DIRECTORY)
    return -1;
  else return 1;
}


/*

/*
// Returns a list of all files in a directory
// path: path containing the files
// filter: array of strings, each containing an extension. last element contains NULL ('\0')

PspFileList* pspFileGetFileList(const char *path, const char **filter)
{
}

void pspFileEnterDirectory(char **cur_path, char *dir)
{
  /* Same directory *
  if (strcmp(dir, ".") == 0)
    return;

  int len, pos;
  len = strlen(*cur_path);
  pos = len - 1;

  /* Ascend one level *
  if (strcmp(dir, "..") == 0)
  {
    for (; pos >= 0 && (*cur_path)[pos] == '/'; pos--);
    for (; pos >= 0 && (*cur_path)[pos] != '/'; pos--);

    if (pos >= 0)
      (*cur_path)[pos + 1] = '\0';

    return;
  }

  /* Descend one level *
  char *new_path = (char*)malloc(sizeof(char) * (strlen(dir) + len + 2)); // 2: '\0' + '/'
  sprintf(new_path, "%s%s/", *cur_path, dir);

  free(*cur_path);
  *cur_path = new_path;
}

*/
