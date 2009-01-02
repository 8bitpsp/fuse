#include <config.h>
#include <string.h>
#include <stdlib.h>

#include "pl_psp.h"
#include "pl_file.h"

#include "compat.h"

static pl_file_path temp_app_path;

const char*
compat_get_temp_path( void )
{
  strncpy(temp_app_path,
         pl_psp_get_app_directory(),
         strlen(pl_psp_get_app_directory()) - 1);
  return temp_app_path + 4;
}

const char*
compat_get_home_path( void )
{
  strncpy(temp_app_path,
         pl_psp_get_app_directory(),
         strlen(pl_psp_get_app_directory()) - 1);
  return temp_app_path + 4;
}

int
compat_is_absolute_path( const char *path )
{
  return (path[0] == '/') || (strcasestr(path, "ms0:/") == path);
}
