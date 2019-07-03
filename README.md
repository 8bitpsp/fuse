Fuse PSP
========

Sinclair Spectrum emulator

&copy; 2008-2009 Akop Karapetyan  
&copy; 2003-2008 Philip Kendall and Fuse team

New Features
------------

#### Version 0.10.0.21 (April 07 2009)

*   Bugfix: Tape/disk status icons should no longer clear when switching between emulator and menu
*   Added tape browser: can now reposition tape location from the System tab (if a tape is loaded)
*   Added tape rewinding, playback (System tab)
*   Added an option to enable/disable tape traps (fixes games like splATTR)
*   Added program counter (PC) display – enable/disable in the Options tab

Installation
------------

Unzip `fuse.zip` into `/PSP/GAME/` folder on the memory stick.

Controls
--------

During emulation:

| PSP controls                    | Emulated controls            |
| ------------------------------- | ---------------------------- |
| D-pad up/down/left/right        | Cursor keys (CAPS + 7/6/5/8) |
| Analog stick up/down/left/right | Kempston joystick up/down/left/right |
| X (cross)                       | Joystick Fire 1              |
| [ ] (square)                    | Enter                        |
| O (circle)                      | Spacebar                     |
| Select                          | 3                            |
| Start                           | 0                            |
| [R]                             | Show virtual keyboard        |
| [L] + [R]                       | Return to the emulator menu  |

When the virtual keyboard is on:

| PSP controls                    | Function                 |
| ------------------------------- | ------------------------ |
| Directional pad                 | select key               |
| [ ] (square)                    | press key                |
| O (circle)                      | hold down/release key    |
| ^ (triangle)                    | release all held keys    |

Only certain keys (e.g. SHIFT) can be held down.

Keyboard mappings can be modified for each game. Button configuration changes are not automatically retained after a mapping is modified. To save changes, press X (cross) after desired mapping is configured. To set the mapping as the default mapping, press [ ] (square).

Compressed Disks
----------------

As of version 0.10, Fuse PSP will load compressed disk (and TRD) images by extracting them to a temporary location, then opening the resulting file. This means that any changes made to a compressed disk will be lost if you either switch disks, or exit the emulator.

Save State Autoloading
----------------------

Fuse PSP can be configured to automatically load a saved game whenever a new game is loaded via the Game tab. To do this, specify one of the save state slots as the Autoload slot (Options tab). If a saved game exists in the specified slot, it will be automatically loaded.

Known Issues
------------

**Fastloading**

When fastloading is enabled, Fuse automatically turns off speed limiting when tape loading is in effect. This speeds up the loading process, but it may prevent certain TZX files from loading. If a TZX/TAP file does not load, try disabling fastloading.

Compiling
---------

To compile, ensure that [zlib](svn://svn.pspdev.org/psp/trunk/zlib) and [libpng](svn://svn.pspdev.org/psp/trunk/libpng) are installed, and run make:

`make -f Makefile.psp`

Version History
---------------

#### 0.10.0.2 (April 05 2009)

*   Added on-screen indicators for floppy and tape drives – enable in the Options tab
*   Fuse updated to 0.10.0.2; libspectrum updated to 0.5.0.1
*   Source: ‘aux’ directory has been renamed, and source code can now be checked out under Windows
*   Bugfix: held keys will be automatically released when the virtual keyboard is displayed
*   Bugfix: Older (pre-0.10) will now load
*   Bugfix: Multiple simultaneous L+\[button\] and R+\[button\] combination presses will now register correctly

#### 0.10.0.1 (January 01 2009)

*   Fuse updated to version 0.10.0.1
*   Miscellaneous bug fixes
*   Support for compressed disks (see documentation)
*   Option to enable/disable sound while tape is loading (only works with Fastloading disabled)
*   Option to enable/disable “Issue 2” keyboard support
*   Extra button options – Square/Cross/Circle/Triangle with L or R trigger held down
*   Option to enable/disable tape fastloading (some games do not load correctly when enabled)
*   Virtual keyboard: emulation should not lag as much when virtual keyboard is displayed
*   Added state exporting to other versions of Fuse – press Start while the ‘Save/Load’ tab is selected — an SNA file will be written to the snapshot directory
*   Virtual keyboard: classic display mode (show when R held down), or toggle mode (show/hide when R is pressed)
*   Tape autoloading
*   Snapshots are now saved into PSP’s own PHOTO directory (/PSP/PHOTO), and can be viewed in PSP’s image viewer
*   File selector snapshots – while browsing for games with the file selector, pause momentarily to display the first snapshot for the game

#### 0.9.0.26 (August 13 2008)

*   A Spectrum joystick button/keyboard key can now be assigned to multiple PSP buttons
*   Sticky key status will now be correctly reset when the virtual keyboard is toggled on/off

#### 0.9.0.25 (July 28 2008)

*   Fixed a bug in the sound rendering engine that would cause occasional slowdowns
*   Added various game image types missing from the initial release
*   New virtual keyboard
*   Added monitor type selection (color/grayscale)
*   Fixed a serious memory leak, which would manifest itself when opening ZIP files
*   Fixed the “tape loading hang” bug
*   Added +3 support
*   Added Kempston joystick support
*   Added “hack cursor” support (keyboard up/down/left/right)
*   Lots of code reshuffling

#### 0.9.0.1 (July 13 2008)

*   Initial release

Credits
-------

Philip Kendall and the Fuse team (Fuse)
