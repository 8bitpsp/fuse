#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "ui/ui.h"

int compat_osname( char *buffer, size_t length )
{
  snprintf(buffer, length, "Sony PSP");
  return 0;
}
