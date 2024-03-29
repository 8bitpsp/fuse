/* snap_accessors.c: simple accessor functions for libspectrum_snap
   Copyright (c) 2003-2008 Philip Kendall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

/* NB: this file is autogenerated from snap_accessors.txt by accessor.pl */

#include <config.h>

#include "internals.h"

struct libspectrum_snap {

  /* Which machine are we using here? */

  libspectrum_machine machine;

  /* Registers and the like */

  libspectrum_byte a , f ; libspectrum_word bc , de , hl ;
  libspectrum_byte a_, f_; libspectrum_word bc_, de_, hl_;

  libspectrum_word ix, iy; libspectrum_byte i, r;
  libspectrum_word sp, pc;

  libspectrum_byte iff1, iff2, im;

  int halted;			/* Is the Z80 currently HALTed? */
  int last_instruction_ei;	/* Was the last instruction an EI? */

  /* Custom ROM */
  int custom_rom;
  size_t custom_rom_pages;
  libspectrum_byte* roms[ 4 ];
  size_t rom_length[ 4 ];

  /* RAM */

  libspectrum_byte *pages[ SNAPSHOT_RAM_PAGES ];

  /* Data from .slt files */

  libspectrum_byte *slt[ SNAPSHOT_SLT_PAGES ];	/* Level data */
  size_t slt_length[ SNAPSHOT_SLT_PAGES ];	/* Length of each level */

  libspectrum_byte *slt_screen;	/* Loading screen */
  int slt_screen_level;		/* The id of the loading screen. Not used
				   for anything AFAIK, but I'll copy it
				   around just in case */

  /* Peripheral status */

  libspectrum_byte out_ula; libspectrum_dword tstates;

  libspectrum_byte out_128_memoryport;

  libspectrum_byte out_ay_registerport, ay_registers[16];

  /* Used for both the +3's and the Scorpion's 0x1ffd port */
  libspectrum_byte out_plus3_memoryport;

  /* Timex-specific bits */
  libspectrum_byte out_scld_hsr, out_scld_dec;

  /* Interface 1 status */
  int interface1_active;
  int interface1_paged;
  int interface1_drive_count;
  int interface1_custom_rom;
  libspectrum_byte* interface1_rom[1];
  size_t interface1_rom_length[1];	/* Length of the ROM */

  /* Betadisk status */
  int beta_active;
  int beta_paged;
  int beta_custom_rom;
  int beta_direction;	/* FDC seek direction:
			      zero => towards lower cylinders (hubwards)
			  non-zero => towards higher cylinders (rimwards) */
  libspectrum_byte beta_system, beta_track, beta_sector, beta_data,
    beta_status;
  libspectrum_byte *beta_rom[1];

  /* Plus D status */
  int plusd_active;
  int plusd_paged;
  int plusd_custom_rom;
  int plusd_direction;	/* FDC seek direction:
			      zero => towards lower cylinders (hubwards)
			  non-zero => towards higher cylinders (rimwards) */
  libspectrum_byte plusd_control, plusd_track, plusd_sector, plusd_data,
    plusd_status;
  libspectrum_byte *plusd_rom[1];
  libspectrum_byte *plusd_ram[1];

  /* ZXATASP status */
  int zxatasp_active;
  int zxatasp_upload;
  int zxatasp_writeprotect;
  libspectrum_byte zxatasp_port_a, zxatasp_port_b, zxatasp_port_c;
  libspectrum_byte zxatasp_control;
  size_t zxatasp_pages;
  size_t zxatasp_current_page;
  libspectrum_byte *zxatasp_ram[ SNAPSHOT_ZXATASP_PAGES ];

  /* ZXCF status */
  int zxcf_active;
  int zxcf_upload;
  libspectrum_byte zxcf_memctl;
  size_t zxcf_pages;
  libspectrum_byte *zxcf_ram[ SNAPSHOT_ZXCF_PAGES ];

  /* Interface II cartridge */
  int interface2_active;
  libspectrum_byte *interface2_rom[1];

  /* Timex Dock cartridge */
  int dock_active;
  libspectrum_byte exrom_ram[ SNAPSHOT_DOCK_EXROM_PAGES ];
  libspectrum_byte *exrom_cart[ SNAPSHOT_DOCK_EXROM_PAGES ];
  libspectrum_byte dock_ram[ SNAPSHOT_DOCK_EXROM_PAGES ];
  libspectrum_byte *dock_cart[ SNAPSHOT_DOCK_EXROM_PAGES ];

  /* Keyboard emulation */
  int issue2;

  /* Joystick emulation */
  size_t joystick_active_count;
  libspectrum_joystick joystick_list[ SNAPSHOT_JOYSTICKS ];
  int joystick_inputs[ SNAPSHOT_JOYSTICKS ];

  /* Kempston mouse status */
  int kempston_mouse_active;

  /* Simple 8-bit IDE status */
  int simpleide_active;

  /* DivIDE status */
  int divide_active;
  int divide_eprom_writeprotect;
  int divide_paged;
  libspectrum_byte divide_control;
  size_t divide_pages;
  libspectrum_byte* divide_eprom[ 1 ];
  libspectrum_byte* divide_ram[ SNAPSHOT_DIVIDE_PAGES ];
};

/* Initialise a libspectrum_snap structure */
libspectrum_snap*
libspectrum_snap_alloc_internal( void )
{
  return libspectrum_malloc( sizeof( libspectrum_snap ) );
}

libspectrum_machine
libspectrum_snap_machine( libspectrum_snap *snap )
{
  return snap->machine;
}

void
libspectrum_snap_set_machine( libspectrum_snap *snap, libspectrum_machine machine )
{
  snap->machine = machine;
}

libspectrum_byte
libspectrum_snap_a( libspectrum_snap *snap )
{
  return snap->a;
}

void
libspectrum_snap_set_a( libspectrum_snap *snap, libspectrum_byte a )
{
  snap->a = a;
}

libspectrum_byte
libspectrum_snap_f( libspectrum_snap *snap )
{
  return snap->f;
}

void
libspectrum_snap_set_f( libspectrum_snap *snap, libspectrum_byte f )
{
  snap->f = f;
}

libspectrum_word
libspectrum_snap_bc( libspectrum_snap *snap )
{
  return snap->bc;
}

void
libspectrum_snap_set_bc( libspectrum_snap *snap, libspectrum_word bc )
{
  snap->bc = bc;
}

libspectrum_word
libspectrum_snap_de( libspectrum_snap *snap )
{
  return snap->de;
}

void
libspectrum_snap_set_de( libspectrum_snap *snap, libspectrum_word de )
{
  snap->de = de;
}

libspectrum_word
libspectrum_snap_hl( libspectrum_snap *snap )
{
  return snap->hl;
}

void
libspectrum_snap_set_hl( libspectrum_snap *snap, libspectrum_word hl )
{
  snap->hl = hl;
}

libspectrum_byte
libspectrum_snap_a_( libspectrum_snap *snap )
{
  return snap->a_;
}

void
libspectrum_snap_set_a_( libspectrum_snap *snap, libspectrum_byte a_ )
{
  snap->a_ = a_;
}

libspectrum_byte
libspectrum_snap_f_( libspectrum_snap *snap )
{
  return snap->f_;
}

void
libspectrum_snap_set_f_( libspectrum_snap *snap, libspectrum_byte f_ )
{
  snap->f_ = f_;
}

libspectrum_word
libspectrum_snap_bc_( libspectrum_snap *snap )
{
  return snap->bc_;
}

void
libspectrum_snap_set_bc_( libspectrum_snap *snap, libspectrum_word bc_ )
{
  snap->bc_ = bc_;
}

libspectrum_word
libspectrum_snap_de_( libspectrum_snap *snap )
{
  return snap->de_;
}

void
libspectrum_snap_set_de_( libspectrum_snap *snap, libspectrum_word de_ )
{
  snap->de_ = de_;
}

libspectrum_word
libspectrum_snap_hl_( libspectrum_snap *snap )
{
  return snap->hl_;
}

void
libspectrum_snap_set_hl_( libspectrum_snap *snap, libspectrum_word hl_ )
{
  snap->hl_ = hl_;
}

libspectrum_word
libspectrum_snap_ix( libspectrum_snap *snap )
{
  return snap->ix;
}

void
libspectrum_snap_set_ix( libspectrum_snap *snap, libspectrum_word ix )
{
  snap->ix = ix;
}

libspectrum_word
libspectrum_snap_iy( libspectrum_snap *snap )
{
  return snap->iy;
}

void
libspectrum_snap_set_iy( libspectrum_snap *snap, libspectrum_word iy )
{
  snap->iy = iy;
}

libspectrum_byte
libspectrum_snap_i( libspectrum_snap *snap )
{
  return snap->i;
}

void
libspectrum_snap_set_i( libspectrum_snap *snap, libspectrum_byte i )
{
  snap->i = i;
}

libspectrum_byte
libspectrum_snap_r( libspectrum_snap *snap )
{
  return snap->r;
}

void
libspectrum_snap_set_r( libspectrum_snap *snap, libspectrum_byte r )
{
  snap->r = r;
}

libspectrum_word
libspectrum_snap_sp( libspectrum_snap *snap )
{
  return snap->sp;
}

void
libspectrum_snap_set_sp( libspectrum_snap *snap, libspectrum_word sp )
{
  snap->sp = sp;
}

libspectrum_word
libspectrum_snap_pc( libspectrum_snap *snap )
{
  return snap->pc;
}

void
libspectrum_snap_set_pc( libspectrum_snap *snap, libspectrum_word pc )
{
  snap->pc = pc;
}

libspectrum_byte
libspectrum_snap_iff1( libspectrum_snap *snap )
{
  return snap->iff1;
}

void
libspectrum_snap_set_iff1( libspectrum_snap *snap, libspectrum_byte iff1 )
{
  snap->iff1 = iff1;
}

libspectrum_byte
libspectrum_snap_iff2( libspectrum_snap *snap )
{
  return snap->iff2;
}

void
libspectrum_snap_set_iff2( libspectrum_snap *snap, libspectrum_byte iff2 )
{
  snap->iff2 = iff2;
}

libspectrum_byte
libspectrum_snap_im( libspectrum_snap *snap )
{
  return snap->im;
}

void
libspectrum_snap_set_im( libspectrum_snap *snap, libspectrum_byte im )
{
  snap->im = im;
}

libspectrum_dword
libspectrum_snap_tstates( libspectrum_snap *snap )
{
  return snap->tstates;
}

void
libspectrum_snap_set_tstates( libspectrum_snap *snap, libspectrum_dword tstates )
{
  snap->tstates = tstates;
}

int
libspectrum_snap_halted( libspectrum_snap *snap )
{
  return snap->halted;
}

void
libspectrum_snap_set_halted( libspectrum_snap *snap, int halted )
{
  snap->halted = halted;
}

int
libspectrum_snap_last_instruction_ei( libspectrum_snap *snap )
{
  return snap->last_instruction_ei;
}

void
libspectrum_snap_set_last_instruction_ei( libspectrum_snap *snap, int last_instruction_ei )
{
  snap->last_instruction_ei = last_instruction_ei;
}

libspectrum_byte
libspectrum_snap_out_ula( libspectrum_snap *snap )
{
  return snap->out_ula;
}

void
libspectrum_snap_set_out_ula( libspectrum_snap *snap, libspectrum_byte out_ula )
{
  snap->out_ula = out_ula;
}

libspectrum_byte
libspectrum_snap_out_128_memoryport( libspectrum_snap *snap )
{
  return snap->out_128_memoryport;
}

void
libspectrum_snap_set_out_128_memoryport( libspectrum_snap *snap, libspectrum_byte out_128_memoryport )
{
  snap->out_128_memoryport = out_128_memoryport;
}

libspectrum_byte
libspectrum_snap_out_plus3_memoryport( libspectrum_snap *snap )
{
  return snap->out_plus3_memoryport;
}

void
libspectrum_snap_set_out_plus3_memoryport( libspectrum_snap *snap, libspectrum_byte out_plus3_memoryport )
{
  snap->out_plus3_memoryport = out_plus3_memoryport;
}

libspectrum_byte
libspectrum_snap_out_ay_registerport( libspectrum_snap *snap )
{
  return snap->out_ay_registerport;
}

void
libspectrum_snap_set_out_ay_registerport( libspectrum_snap *snap, libspectrum_byte out_ay_registerport )
{
  snap->out_ay_registerport = out_ay_registerport;
}

libspectrum_byte
libspectrum_snap_ay_registers( libspectrum_snap *snap, int idx )
{
  return snap->ay_registers[idx];
}

void
libspectrum_snap_set_ay_registers( libspectrum_snap *snap, int idx, libspectrum_byte ay_registers )
{
  snap->ay_registers[idx] = ay_registers;
}

libspectrum_byte
libspectrum_snap_out_scld_hsr( libspectrum_snap *snap )
{
  return snap->out_scld_hsr;
}

void
libspectrum_snap_set_out_scld_hsr( libspectrum_snap *snap, libspectrum_byte out_scld_hsr )
{
  snap->out_scld_hsr = out_scld_hsr;
}

libspectrum_byte
libspectrum_snap_out_scld_dec( libspectrum_snap *snap )
{
  return snap->out_scld_dec;
}

void
libspectrum_snap_set_out_scld_dec( libspectrum_snap *snap, libspectrum_byte out_scld_dec )
{
  snap->out_scld_dec = out_scld_dec;
}

int
libspectrum_snap_interface1_active( libspectrum_snap *snap )
{
  return snap->interface1_active;
}

void
libspectrum_snap_set_interface1_active( libspectrum_snap *snap, int interface1_active )
{
  snap->interface1_active = interface1_active;
}

int
libspectrum_snap_interface1_paged( libspectrum_snap *snap )
{
  return snap->interface1_paged;
}

void
libspectrum_snap_set_interface1_paged( libspectrum_snap *snap, int interface1_paged )
{
  snap->interface1_paged = interface1_paged;
}

int
libspectrum_snap_interface1_drive_count( libspectrum_snap *snap )
{
  return snap->interface1_drive_count;
}

void
libspectrum_snap_set_interface1_drive_count( libspectrum_snap *snap, int interface1_drive_count )
{
  snap->interface1_drive_count = interface1_drive_count;
}

int
libspectrum_snap_interface1_custom_rom( libspectrum_snap *snap )
{
  return snap->interface1_custom_rom;
}

void
libspectrum_snap_set_interface1_custom_rom( libspectrum_snap *snap, int interface1_custom_rom )
{
  snap->interface1_custom_rom = interface1_custom_rom;
}

libspectrum_byte*
libspectrum_snap_interface1_rom( libspectrum_snap *snap, int idx )
{
  return snap->interface1_rom[idx];
}

void
libspectrum_snap_set_interface1_rom( libspectrum_snap *snap, int idx, libspectrum_byte* interface1_rom )
{
  snap->interface1_rom[idx] = interface1_rom;
}

size_t
libspectrum_snap_interface1_rom_length( libspectrum_snap *snap, int idx )
{
  return snap->interface1_rom_length[idx];
}

void
libspectrum_snap_set_interface1_rom_length( libspectrum_snap *snap, int idx, size_t interface1_rom_length )
{
  snap->interface1_rom_length[idx] = interface1_rom_length;
}

int
libspectrum_snap_beta_active( libspectrum_snap *snap )
{
  return snap->beta_active;
}

void
libspectrum_snap_set_beta_active( libspectrum_snap *snap, int beta_active )
{
  snap->beta_active = beta_active;
}

int
libspectrum_snap_beta_paged( libspectrum_snap *snap )
{
  return snap->beta_paged;
}

void
libspectrum_snap_set_beta_paged( libspectrum_snap *snap, int beta_paged )
{
  snap->beta_paged = beta_paged;
}

int
libspectrum_snap_beta_custom_rom( libspectrum_snap *snap )
{
  return snap->beta_custom_rom;
}

void
libspectrum_snap_set_beta_custom_rom( libspectrum_snap *snap, int beta_custom_rom )
{
  snap->beta_custom_rom = beta_custom_rom;
}

int
libspectrum_snap_beta_direction( libspectrum_snap *snap )
{
  return snap->beta_direction;
}

void
libspectrum_snap_set_beta_direction( libspectrum_snap *snap, int beta_direction )
{
  snap->beta_direction = beta_direction;
}

libspectrum_byte
libspectrum_snap_beta_system( libspectrum_snap *snap )
{
  return snap->beta_system;
}

void
libspectrum_snap_set_beta_system( libspectrum_snap *snap, libspectrum_byte beta_system )
{
  snap->beta_system = beta_system;
}

libspectrum_byte
libspectrum_snap_beta_track( libspectrum_snap *snap )
{
  return snap->beta_track;
}

void
libspectrum_snap_set_beta_track( libspectrum_snap *snap, libspectrum_byte beta_track )
{
  snap->beta_track = beta_track;
}

libspectrum_byte
libspectrum_snap_beta_sector( libspectrum_snap *snap )
{
  return snap->beta_sector;
}

void
libspectrum_snap_set_beta_sector( libspectrum_snap *snap, libspectrum_byte beta_sector )
{
  snap->beta_sector = beta_sector;
}

libspectrum_byte
libspectrum_snap_beta_data( libspectrum_snap *snap )
{
  return snap->beta_data;
}

void
libspectrum_snap_set_beta_data( libspectrum_snap *snap, libspectrum_byte beta_data )
{
  snap->beta_data = beta_data;
}

libspectrum_byte
libspectrum_snap_beta_status( libspectrum_snap *snap )
{
  return snap->beta_status;
}

void
libspectrum_snap_set_beta_status( libspectrum_snap *snap, libspectrum_byte beta_status )
{
  snap->beta_status = beta_status;
}

libspectrum_byte*
libspectrum_snap_beta_rom( libspectrum_snap *snap, int idx )
{
  return snap->beta_rom[idx];
}

void
libspectrum_snap_set_beta_rom( libspectrum_snap *snap, int idx, libspectrum_byte* beta_rom )
{
  snap->beta_rom[idx] = beta_rom;
}

int
libspectrum_snap_plusd_active( libspectrum_snap *snap )
{
  return snap->plusd_active;
}

void
libspectrum_snap_set_plusd_active( libspectrum_snap *snap, int plusd_active )
{
  snap->plusd_active = plusd_active;
}

int
libspectrum_snap_plusd_paged( libspectrum_snap *snap )
{
  return snap->plusd_paged;
}

void
libspectrum_snap_set_plusd_paged( libspectrum_snap *snap, int plusd_paged )
{
  snap->plusd_paged = plusd_paged;
}

int
libspectrum_snap_plusd_custom_rom( libspectrum_snap *snap )
{
  return snap->plusd_custom_rom;
}

void
libspectrum_snap_set_plusd_custom_rom( libspectrum_snap *snap, int plusd_custom_rom )
{
  snap->plusd_custom_rom = plusd_custom_rom;
}

int
libspectrum_snap_plusd_direction( libspectrum_snap *snap )
{
  return snap->plusd_direction;
}

void
libspectrum_snap_set_plusd_direction( libspectrum_snap *snap, int plusd_direction )
{
  snap->plusd_direction = plusd_direction;
}

libspectrum_byte
libspectrum_snap_plusd_control( libspectrum_snap *snap )
{
  return snap->plusd_control;
}

void
libspectrum_snap_set_plusd_control( libspectrum_snap *snap, libspectrum_byte plusd_control )
{
  snap->plusd_control = plusd_control;
}

libspectrum_byte
libspectrum_snap_plusd_track( libspectrum_snap *snap )
{
  return snap->plusd_track;
}

void
libspectrum_snap_set_plusd_track( libspectrum_snap *snap, libspectrum_byte plusd_track )
{
  snap->plusd_track = plusd_track;
}

libspectrum_byte
libspectrum_snap_plusd_sector( libspectrum_snap *snap )
{
  return snap->plusd_sector;
}

void
libspectrum_snap_set_plusd_sector( libspectrum_snap *snap, libspectrum_byte plusd_sector )
{
  snap->plusd_sector = plusd_sector;
}

libspectrum_byte
libspectrum_snap_plusd_data( libspectrum_snap *snap )
{
  return snap->plusd_data;
}

void
libspectrum_snap_set_plusd_data( libspectrum_snap *snap, libspectrum_byte plusd_data )
{
  snap->plusd_data = plusd_data;
}

libspectrum_byte
libspectrum_snap_plusd_status( libspectrum_snap *snap )
{
  return snap->plusd_status;
}

void
libspectrum_snap_set_plusd_status( libspectrum_snap *snap, libspectrum_byte plusd_status )
{
  snap->plusd_status = plusd_status;
}

libspectrum_byte*
libspectrum_snap_plusd_rom( libspectrum_snap *snap, int idx )
{
  return snap->plusd_rom[idx];
}

void
libspectrum_snap_set_plusd_rom( libspectrum_snap *snap, int idx, libspectrum_byte* plusd_rom )
{
  snap->plusd_rom[idx] = plusd_rom;
}

libspectrum_byte*
libspectrum_snap_plusd_ram( libspectrum_snap *snap, int idx )
{
  return snap->plusd_ram[idx];
}

void
libspectrum_snap_set_plusd_ram( libspectrum_snap *snap, int idx, libspectrum_byte* plusd_ram )
{
  snap->plusd_ram[idx] = plusd_ram;
}

int
libspectrum_snap_custom_rom( libspectrum_snap *snap )
{
  return snap->custom_rom;
}

void
libspectrum_snap_set_custom_rom( libspectrum_snap *snap, int custom_rom )
{
  snap->custom_rom = custom_rom;
}

size_t
libspectrum_snap_custom_rom_pages( libspectrum_snap *snap )
{
  return snap->custom_rom_pages;
}

void
libspectrum_snap_set_custom_rom_pages( libspectrum_snap *snap, size_t custom_rom_pages )
{
  snap->custom_rom_pages = custom_rom_pages;
}

libspectrum_byte*
libspectrum_snap_roms( libspectrum_snap *snap, int idx )
{
  return snap->roms[idx];
}

void
libspectrum_snap_set_roms( libspectrum_snap *snap, int idx, libspectrum_byte* roms )
{
  snap->roms[idx] = roms;
}

size_t
libspectrum_snap_rom_length( libspectrum_snap *snap, int idx )
{
  return snap->rom_length[idx];
}

void
libspectrum_snap_set_rom_length( libspectrum_snap *snap, int idx, size_t rom_length )
{
  snap->rom_length[idx] = rom_length;
}

libspectrum_byte*
libspectrum_snap_pages( libspectrum_snap *snap, int idx )
{
  return snap->pages[idx];
}

void
libspectrum_snap_set_pages( libspectrum_snap *snap, int idx, libspectrum_byte* pages )
{
  snap->pages[idx] = pages;
}

libspectrum_byte*
libspectrum_snap_slt( libspectrum_snap *snap, int idx )
{
  return snap->slt[idx];
}

void
libspectrum_snap_set_slt( libspectrum_snap *snap, int idx, libspectrum_byte* slt )
{
  snap->slt[idx] = slt;
}

size_t
libspectrum_snap_slt_length( libspectrum_snap *snap, int idx )
{
  return snap->slt_length[idx];
}

void
libspectrum_snap_set_slt_length( libspectrum_snap *snap, int idx, size_t slt_length )
{
  snap->slt_length[idx] = slt_length;
}

libspectrum_byte*
libspectrum_snap_slt_screen( libspectrum_snap *snap )
{
  return snap->slt_screen;
}

void
libspectrum_snap_set_slt_screen( libspectrum_snap *snap, libspectrum_byte* slt_screen )
{
  snap->slt_screen = slt_screen;
}

int
libspectrum_snap_slt_screen_level( libspectrum_snap *snap )
{
  return snap->slt_screen_level;
}

void
libspectrum_snap_set_slt_screen_level( libspectrum_snap *snap, int slt_screen_level )
{
  snap->slt_screen_level = slt_screen_level;
}

int
libspectrum_snap_zxatasp_active( libspectrum_snap *snap )
{
  return snap->zxatasp_active;
}

void
libspectrum_snap_set_zxatasp_active( libspectrum_snap *snap, int zxatasp_active )
{
  snap->zxatasp_active = zxatasp_active;
}

int
libspectrum_snap_zxatasp_upload( libspectrum_snap *snap )
{
  return snap->zxatasp_upload;
}

void
libspectrum_snap_set_zxatasp_upload( libspectrum_snap *snap, int zxatasp_upload )
{
  snap->zxatasp_upload = zxatasp_upload;
}

int
libspectrum_snap_zxatasp_writeprotect( libspectrum_snap *snap )
{
  return snap->zxatasp_writeprotect;
}

void
libspectrum_snap_set_zxatasp_writeprotect( libspectrum_snap *snap, int zxatasp_writeprotect )
{
  snap->zxatasp_writeprotect = zxatasp_writeprotect;
}

libspectrum_byte
libspectrum_snap_zxatasp_port_a( libspectrum_snap *snap )
{
  return snap->zxatasp_port_a;
}

void
libspectrum_snap_set_zxatasp_port_a( libspectrum_snap *snap, libspectrum_byte zxatasp_port_a )
{
  snap->zxatasp_port_a = zxatasp_port_a;
}

libspectrum_byte
libspectrum_snap_zxatasp_port_b( libspectrum_snap *snap )
{
  return snap->zxatasp_port_b;
}

void
libspectrum_snap_set_zxatasp_port_b( libspectrum_snap *snap, libspectrum_byte zxatasp_port_b )
{
  snap->zxatasp_port_b = zxatasp_port_b;
}

libspectrum_byte
libspectrum_snap_zxatasp_port_c( libspectrum_snap *snap )
{
  return snap->zxatasp_port_c;
}

void
libspectrum_snap_set_zxatasp_port_c( libspectrum_snap *snap, libspectrum_byte zxatasp_port_c )
{
  snap->zxatasp_port_c = zxatasp_port_c;
}

libspectrum_byte
libspectrum_snap_zxatasp_control( libspectrum_snap *snap )
{
  return snap->zxatasp_control;
}

void
libspectrum_snap_set_zxatasp_control( libspectrum_snap *snap, libspectrum_byte zxatasp_control )
{
  snap->zxatasp_control = zxatasp_control;
}

size_t
libspectrum_snap_zxatasp_pages( libspectrum_snap *snap )
{
  return snap->zxatasp_pages;
}

void
libspectrum_snap_set_zxatasp_pages( libspectrum_snap *snap, size_t zxatasp_pages )
{
  snap->zxatasp_pages = zxatasp_pages;
}

size_t
libspectrum_snap_zxatasp_current_page( libspectrum_snap *snap )
{
  return snap->zxatasp_current_page;
}

void
libspectrum_snap_set_zxatasp_current_page( libspectrum_snap *snap, size_t zxatasp_current_page )
{
  snap->zxatasp_current_page = zxatasp_current_page;
}

libspectrum_byte*
libspectrum_snap_zxatasp_ram( libspectrum_snap *snap, int idx )
{
  return snap->zxatasp_ram[idx];
}

void
libspectrum_snap_set_zxatasp_ram( libspectrum_snap *snap, int idx, libspectrum_byte* zxatasp_ram )
{
  snap->zxatasp_ram[idx] = zxatasp_ram;
}

int
libspectrum_snap_zxcf_active( libspectrum_snap *snap )
{
  return snap->zxcf_active;
}

void
libspectrum_snap_set_zxcf_active( libspectrum_snap *snap, int zxcf_active )
{
  snap->zxcf_active = zxcf_active;
}

int
libspectrum_snap_zxcf_upload( libspectrum_snap *snap )
{
  return snap->zxcf_upload;
}

void
libspectrum_snap_set_zxcf_upload( libspectrum_snap *snap, int zxcf_upload )
{
  snap->zxcf_upload = zxcf_upload;
}

libspectrum_byte
libspectrum_snap_zxcf_memctl( libspectrum_snap *snap )
{
  return snap->zxcf_memctl;
}

void
libspectrum_snap_set_zxcf_memctl( libspectrum_snap *snap, libspectrum_byte zxcf_memctl )
{
  snap->zxcf_memctl = zxcf_memctl;
}

size_t
libspectrum_snap_zxcf_pages( libspectrum_snap *snap )
{
  return snap->zxcf_pages;
}

void
libspectrum_snap_set_zxcf_pages( libspectrum_snap *snap, size_t zxcf_pages )
{
  snap->zxcf_pages = zxcf_pages;
}

libspectrum_byte*
libspectrum_snap_zxcf_ram( libspectrum_snap *snap, int idx )
{
  return snap->zxcf_ram[idx];
}

void
libspectrum_snap_set_zxcf_ram( libspectrum_snap *snap, int idx, libspectrum_byte* zxcf_ram )
{
  snap->zxcf_ram[idx] = zxcf_ram;
}

int
libspectrum_snap_interface2_active( libspectrum_snap *snap )
{
  return snap->interface2_active;
}

void
libspectrum_snap_set_interface2_active( libspectrum_snap *snap, int interface2_active )
{
  snap->interface2_active = interface2_active;
}

libspectrum_byte*
libspectrum_snap_interface2_rom( libspectrum_snap *snap, int idx )
{
  return snap->interface2_rom[idx];
}

void
libspectrum_snap_set_interface2_rom( libspectrum_snap *snap, int idx, libspectrum_byte* interface2_rom )
{
  snap->interface2_rom[idx] = interface2_rom;
}

int
libspectrum_snap_dock_active( libspectrum_snap *snap )
{
  return snap->dock_active;
}

void
libspectrum_snap_set_dock_active( libspectrum_snap *snap, int dock_active )
{
  snap->dock_active = dock_active;
}

libspectrum_byte
libspectrum_snap_exrom_ram( libspectrum_snap *snap, int idx )
{
  return snap->exrom_ram[idx];
}

void
libspectrum_snap_set_exrom_ram( libspectrum_snap *snap, int idx, libspectrum_byte exrom_ram )
{
  snap->exrom_ram[idx] = exrom_ram;
}

libspectrum_byte*
libspectrum_snap_exrom_cart( libspectrum_snap *snap, int idx )
{
  return snap->exrom_cart[idx];
}

void
libspectrum_snap_set_exrom_cart( libspectrum_snap *snap, int idx, libspectrum_byte* exrom_cart )
{
  snap->exrom_cart[idx] = exrom_cart;
}

libspectrum_byte
libspectrum_snap_dock_ram( libspectrum_snap *snap, int idx )
{
  return snap->dock_ram[idx];
}

void
libspectrum_snap_set_dock_ram( libspectrum_snap *snap, int idx, libspectrum_byte dock_ram )
{
  snap->dock_ram[idx] = dock_ram;
}

libspectrum_byte*
libspectrum_snap_dock_cart( libspectrum_snap *snap, int idx )
{
  return snap->dock_cart[idx];
}

void
libspectrum_snap_set_dock_cart( libspectrum_snap *snap, int idx, libspectrum_byte* dock_cart )
{
  snap->dock_cart[idx] = dock_cart;
}

int
libspectrum_snap_issue2( libspectrum_snap *snap )
{
  return snap->issue2;
}

void
libspectrum_snap_set_issue2( libspectrum_snap *snap, int issue2 )
{
  snap->issue2 = issue2;
}

size_t
libspectrum_snap_joystick_active_count( libspectrum_snap *snap )
{
  return snap->joystick_active_count;
}

void
libspectrum_snap_set_joystick_active_count( libspectrum_snap *snap, size_t joystick_active_count )
{
  snap->joystick_active_count = joystick_active_count;
}

libspectrum_joystick
libspectrum_snap_joystick_list( libspectrum_snap *snap, int idx )
{
  return snap->joystick_list[idx];
}

void
libspectrum_snap_set_joystick_list( libspectrum_snap *snap, int idx, libspectrum_joystick joystick_list )
{
  snap->joystick_list[idx] = joystick_list;
}

int
libspectrum_snap_joystick_inputs( libspectrum_snap *snap, int idx )
{
  return snap->joystick_inputs[idx];
}

void
libspectrum_snap_set_joystick_inputs( libspectrum_snap *snap, int idx, int joystick_inputs )
{
  snap->joystick_inputs[idx] = joystick_inputs;
}

int
libspectrum_snap_kempston_mouse_active( libspectrum_snap *snap )
{
  return snap->kempston_mouse_active;
}

void
libspectrum_snap_set_kempston_mouse_active( libspectrum_snap *snap, int kempston_mouse_active )
{
  snap->kempston_mouse_active = kempston_mouse_active;
}

int
libspectrum_snap_simpleide_active( libspectrum_snap *snap )
{
  return snap->simpleide_active;
}

void
libspectrum_snap_set_simpleide_active( libspectrum_snap *snap, int simpleide_active )
{
  snap->simpleide_active = simpleide_active;
}

int
libspectrum_snap_divide_active( libspectrum_snap *snap )
{
  return snap->divide_active;
}

void
libspectrum_snap_set_divide_active( libspectrum_snap *snap, int divide_active )
{
  snap->divide_active = divide_active;
}

int
libspectrum_snap_divide_eprom_writeprotect( libspectrum_snap *snap )
{
  return snap->divide_eprom_writeprotect;
}

void
libspectrum_snap_set_divide_eprom_writeprotect( libspectrum_snap *snap, int divide_eprom_writeprotect )
{
  snap->divide_eprom_writeprotect = divide_eprom_writeprotect;
}

int
libspectrum_snap_divide_paged( libspectrum_snap *snap )
{
  return snap->divide_paged;
}

void
libspectrum_snap_set_divide_paged( libspectrum_snap *snap, int divide_paged )
{
  snap->divide_paged = divide_paged;
}

libspectrum_byte
libspectrum_snap_divide_control( libspectrum_snap *snap )
{
  return snap->divide_control;
}

void
libspectrum_snap_set_divide_control( libspectrum_snap *snap, libspectrum_byte divide_control )
{
  snap->divide_control = divide_control;
}

size_t
libspectrum_snap_divide_pages( libspectrum_snap *snap )
{
  return snap->divide_pages;
}

void
libspectrum_snap_set_divide_pages( libspectrum_snap *snap, size_t divide_pages )
{
  snap->divide_pages = divide_pages;
}

libspectrum_byte*
libspectrum_snap_divide_eprom( libspectrum_snap *snap, int idx )
{
  return snap->divide_eprom[idx];
}

void
libspectrum_snap_set_divide_eprom( libspectrum_snap *snap, int idx, libspectrum_byte* divide_eprom )
{
  snap->divide_eprom[idx] = divide_eprom;
}

libspectrum_byte*
libspectrum_snap_divide_ram( libspectrum_snap *snap, int idx )
{
  return snap->divide_ram[idx];
}

void
libspectrum_snap_set_divide_ram( libspectrum_snap *snap, int idx, libspectrum_byte* divide_ram )
{
  snap->divide_ram[idx] = divide_ram;
}
