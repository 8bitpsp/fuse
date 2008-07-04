#include <config.h>

#include <stdlib.h>

#include "compat.h"

const char*
compat_get_temp_path( void )
{
  return ".";
}

const char*
compat_get_home_path( void )
{
  return ".";
}

int
compat_is_absolute_path( const char *path )
{
  return path[0] == '/';
}
