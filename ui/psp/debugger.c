#include <config.h>

#include <libspectrum.h>

#include "debugger/debugger.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

int
ui_debugger_activate( void )
{
  return 0;
}

int
ui_debugger_disassemble( libspectrum_word address )
{
  return 0;
}

int
ui_debugger_update( void )
{
  return 0;
}

int
ui_debugger_deactivate( int interruptable )
{
  return 0;
}
