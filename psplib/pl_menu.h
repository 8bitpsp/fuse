/* psplib/pl_menu.h
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

#ifndef _PL_MENU_H
#define _PL_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pl_menu_option_t
{
  char *text;
  const void *value;
} pl_menu_option;

typedef struct pl_menu_item_t
{
  unsigned int id;
  char *caption;
  const void *icon;
  const void *param;
  char *help_text;
  struct pl_menu_option_t *selected;
  struct pl_menu_option_t *options;
} pl_menu_item;

typedef struct pl_menu_t
{
  struct pl_menu_item_t *selected;
  struct pl_menu_item_t *items;
} pl_menu;

typedef struct pl_menu_option_def_t
{
  const char *text;
  const void *value;
} pl_menu_option_def;

typedef struct pl_menu_def_t
{
  unsigned int id;
  const char *caption;
  const char *help_text;
  pl_menu_option_def *options;
} pl_menu_def;

#define PL_MENU_BEGIN(ident)  pl_menu_def ident[] = {
#define PL_MENU_HEADER(text)  {0,"\t"text,NULL,NULL},
#define PL_MENU_ITEM(id,caption,help,option_list) \
                              {id,caption,help,option_list},
#define PL_MENU_END           {0,NULL,NULL,NULL}}

#define PL_MENU_BEGIN_OPTION(ident) pl_menu_option_def ident[] = {
#define PL_MENU_OPTION(text,param) {text,(const void*)(param)},
#define PL_MENU_END_OPTION {NULL,NULL}}



PL_MENU_BEGIN_OPTION(options)
PL_MENU_OPTION("foo", NULL)
PL_MENU_OPTION("bar", NULL)
PL_MENU_END_OPTION;

PL_MENU_BEGIN(menu)
PL_MENU_ITEM(1,"caption","tooltip",options)
PL_MENU_END;
PL_MENU_BEGIN(menu2)
PL_MENU_ITEM(1,"caption","tooltip",options)
PL_MENU_END;

/*
#define PL_MENU_HEADER(text)        { "\t"text, 0, NULL, -1, NULL }
#define PL_MENU_ITEM(text, id, option_list, sel_index, help_text) \
  { (text), (id), (option_list), (sel_index), (help_text) }
#define PL_MENU_OPTION(text, value) { (text), (void*)(value) }
#define PL_MENU_END_ITEMS           { NULL, 0 }
#define PL_MENU_END_OPTIONS         { NULL, NULL }
*/
/*
typedef struct pl_menu_option_t
{
  const void *value;
  char *text;
  struct pl_menu_option_t *next;
  struct pl_menu_option_t *prev;
} pl_menu_option;

typedef struct pl_menu_item_t
{
  unsigned int id;
  char *caption;
  const void *icon;
  const void *param;
  struct pl_menu_option_t *options;
  struct pl_menu_option_t *selected;
  struct pl_menu_item_t *next;
  struct pl_menu_item_t *prev;
  char *help_text;
} pl_menu_item;

typedef struct pl_menu_t
{
  struct pl_menu_item_t *first;
  struct pl_menu_item_t *last;
  struct pl_menu_item_t *selected;
  int count;
} pl_menu;

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

*/

#ifdef __cplusplus
}
#endif

#endif  // _PL_MENU_H
