/* psplib/menu.h
   Simple menu implementation

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

#ifndef _PSP_MENU_H
#define _PSP_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

#define MENU_HEADER(text)        { "\t"text, 0, NULL, -1, NULL }
#define MENU_ITEM(text, id, option_list, sel_index, help_text) \
  { (text), (id), (option_list), (sel_index), (help_text) }
#define MENU_OPTION(text, value) { (text), (void*)(value) }
#define MENU_END_ITEMS           { NULL, 0 }
#define MENU_END_OPTIONS         { NULL, NULL }

typedef struct PspMenuOption
{
  const void* Value;
  char* Text;
  struct PspMenuOption *Next;
  struct PspMenuOption *Prev;
} PspMenuOption;

typedef struct PspMenuItem
{
  unsigned int ID;
  char* Caption;
  const void *Icon;
  const void *Param;
  PspMenuOption *Options;
  const PspMenuOption *Selected;
  struct PspMenuItem *Next;
  struct PspMenuItem *Prev;
  char *HelpText;
} PspMenuItem;

typedef struct PspMenu
{
  PspMenuItem* First;
  PspMenuItem* Last;
  PspMenuItem* Selected;
  int Count;
} PspMenu;

typedef struct PspMenuOptionDef
{
  const char *Text;
  void *Value;
} PspMenuOptionDef;

typedef struct PspMenuItemDef
{
  const char *Caption;
  unsigned int ID;
  const PspMenuOptionDef *OptionList;
  int   SelectedIndex;
  const char *HelpText;
} PspMenuItemDef;


PspMenu*       pspMenuCreate();
void           pspMenuLoad(PspMenu *menu, const PspMenuItemDef *def);
void           pspMenuClear(PspMenu* menu);
void           pspMenuDestroy(PspMenu* menu);
PspMenuItem*   pspMenuAppendItem(PspMenu* menu, const char* caption, 
  unsigned int id);
int            pspMenuDestroyItem(PspMenu *menu, PspMenuItem *which);
PspMenuOption* pspMenuAppendOption(PspMenuItem *item, const char *text, 
  const void *value, int select);
void           pspMenuSelectOptionByIndex(PspMenuItem *item, int index);
void           pspMenuSelectOptionByValue(PspMenuItem *item, const void *value);
void           pspMenuModifyOption(PspMenuOption *option, const char *text, 
  const void *value);
void           pspMenuClearOptions(PspMenuItem* item);
PspMenuItem*   pspMenuGetNthItem(PspMenu *menu, int index);
PspMenuItem*   pspMenuFindItemById(PspMenu *menu, unsigned int id);
void           pspMenuSetCaption(PspMenuItem *item, const char *caption);
void           pspMenuSetHelpText(PspMenuItem *item, const char *helptext);

#ifdef __cplusplus
}
#endif

#endif  // _PSP_MENU_H
