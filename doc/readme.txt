Castaway Atari ST Emulator
==========================

CONTENTS
--------

1.  Introduction

2.  Copyright Notes

3.  Description and Usage
3.1 Compiling MOST
3.2 Using MOST
3.3 Command Line Options
3.4 Floppy Disks
3.5 ROMs and ROM troubles
3.6 Compatibility and Emulation Speed

4.  Future Work

Appendices:
A   I/O and Device Emulation Summary
B   Bugs


1. INTRODUCTION
---------------

Welcome to the Castaway Atari ST Emulator.

Q: Why another Atari ST emulator?
A: Why indeed. Most of the ST emulator code is ancient (from about 1994)
   and there weren't that many emulators around then. However, its still
   working, and it is being constantly improved.

Q: How fast is it?
A: On fast computers it is (see 3.6). On today's PC systems, the emulator
   is about fifteen times as fast as the original Atari ST.

Q: What do I need it for?
A: Uh, good question.  I asked myself this question many times during long
   nights of coding and optimizing and experimenting.  If you come up with
   a satisfying answer, do let me know.

Q: Does it work properly?
A: It looks very much as if it indeed does.  Don't expect miracles, though.
   There are some incompatibilities, but we are working on them.

Q: What do I need to run Castaway?
A: The real thing, to copy the ROMs.  Next, a PC running Linux or Win32.

Q: Is it all for free?
A: Quite. Read the next section `COPYRIGHT'.


2. COPYRIGHT
------------

Castaway itself may be distributed under the terms of the GNU General Public
License (GPL) version 2 or later (http://www.gnu.org/licenses/licenses.html#GPL).

The Castaway distribution does not include Atari ROM code.

The Castaway Atari ST emulator uses the Simple DirectMedia Layer library, currently
available under the GNU Lesser General Public License (LGPL) version 2 or newer
(http://www.gnu.org/copyleft/lgpl.html). The Simple DirectMedia Layer source code
and libraries are available for download at http://www.libsdl.org.

The Win32 binary release of the Castaway Atari ST emulator uses the cygwin library
(http://cygwin.com).

E-mail:
            jhoenig@users.sourceforge.net
            doeringm@gmx.de

Castaway home:
            http://castaway.sourceforge.net



3. DESCRIPTION AND USAGE
------------------------

3.1 Compiling

Castaway has been compiled and tried on PC systems (Linux or Win32), and
on MIPS R3000 machines.

There's a Makefile.  You will probably need to edit paths and flags
according to your needs.  Use of gcc -O2 ist strongly encouraged, as it
REALLY makes a difference.
Most of the configuration and the setting of default values is done in
the file config.h.
Additional configuration switches of the 68000 emulator are loacted in
cpu/68000.h.

Note: Some problems have been encountered with certain gcc versions
and optimization.  If your build does not work, cut back on optimization.


3.2 Using Castaway

After compiling, you should find a program named castaway.exe or simply
castaway in the build directory.  This program expects to find the rom
image in a file named `rom' (see 3.5 ROMs and ROM troubles), the floppy
disk images in files named `diska' and `diskb´ (see 3.4 Floppy Disks).
You will find a default boot disk (diska, drive A) image included in the
distribution. Create a symbolic link named `diskb´ (drive B) pointing to
your physical floppy disk device (ln -s /dev/fd0 diskb).
After you have provided the rom image you are all set to go.

Note: Win32 users need to install cygwin (http://www.cygwin.com/) to
provide the /dev/fd0 device, and to have the ability to create symbolic
links. Without cygwin, you need to provide a second floppy disk image file.

After starting castaway, MOST will print some status messages to the terminal
window and open a 640x400 monochrome window.

Maybe you will be trying to find the button labelled `EXIT'. Don't
worry, there is no such button. To terminate MOST, hold down the
SHIFT key and then press PAUSE in the Castaway window, or CTRL-C in the
terminal window. Depending on your window manager, exiting the Castaway
window is another option.

The Mouse generally works as expected (surprise). You can confine the mouse
to the Castaway window by pressing the middle mouse button (this "grabs" the
mouse). The mouse will not leave the Castaway window until you toggle mouse
grabbing by pressing the middle button again.


3.3 Command Line Options

   -a <file>    set disk A file name. E.g. -a /dev/fd0 sets the physical
                floppy drive to be drive A. The default name is "diska".
   -b <file>    set disk B file name. Using the same name for both drives
        is discouraged. The default is "diskb".
   -c <file>    name of the cartridge image file (not yet supported)
   -r <file>    name of the rom image file. The default is "rom".
   -m <kbyte>   RAM size (e.g 256, 512, 1024 etc.). The default is 512k.
   -4           Start in 4bpp color mode (320x200, 16 colors)
   -h           (discontinued)
   -f           (reserved for fullscreen mode)
   -i <ips>     slow down execution to <ips> instructions per second.


3.4 Floppy Disks

In most applications, your floppy disks will simply be image files.  Use
dd to create a copy of a physical disk.  The proper disk devices with Linux
are /dev/fd?D360 for single-sided disks and /dev/fd?D720 for double sided
disks (e.g. dd if=/dev/fd0D720 of=diska).

You will find that Atari's operating system has no trouble with HD disk
images as well (/dev/fd?, 1440k).  In fact, TOS supports almost any disk
size and layout (track and sector numbers, cluster size).  A small
program is provided (tools/mkdisk.c), to create large disk images (4 MB
or more, depending on TOS version) for use as a hard disk substitute.
Mkdisk does not support any command line switches other than the file
name of the image file to create, simply edit the program according to
your tastes.  As is, mkdisk is configured to create 4000k disk images.
To build mkdisk, type `make tools' in the Castaway directory.

You may access floppy disks directly by specifying the proper device
inode in the command line (e.g. st.i386 -a /dev/fd0).  Note that you
cannot format disks, as the write track command of the FDC chip is not
supported.  Formatting disks will simply not work.  Note, moreover, that
due to neglect and laziness on my part, disks may not be write-protected.

On some platforms, you may have trouble reading single-sided disks, simply
because the proper device inodes or drivers are lacking.
Therefore a small programm is provided (tools/rdsingle.c), to read
single-sided disks from device /dev/rfd0 to a disk image file.  Rdsingle
expects the name of the image file as an argument.


3.5 ROMs and ROM troubles

The TOS power-up sequence has a timing loop test, which usually fails if
you are not using an 8 MHz CPU. To cut a long story short, the simplest
workaround for this problem is having the Timer B of the MFP always return
1 as the raster line count, no harm done. Add this to the incompatibility
list.

The TOS ROM binaries are NOT included within this distribution.


3.6 Compatibility and Emulation Speed

The only known bugs of the ST emulator are:

- A few shortcuts were taken on the subject of timers and interrupts.  The
  horizontal blanking interrupt (every 50us) was scrapped, likewise the
  raster line counter.

- The system timer is not programmable, instead it will appear 200 times
  per second, as supposed.  The system timer is driven by an interval timer,
  whose shortest interval is 10ms (on some platforms), 33ms on others.
  The emulator doubles or triples each system timer interrupt, resulting
  in a pseudo-5ms interval matching the ST's 200Hz timer speed.

- The vertical blanking interrupt is raised every 20ms, resulting in VBL
  frequencies of about 50Hz (50 frames per second).  On platforms with a
  60 Hz (33ms) system timer the spacing of individual VBLs gets weird.

- The Blitter may crash the emulator when its programmed to bitblt to or
  from unmapped I/O memory. If you encounter crashes or unexpected bus
  errors, try disabling the Blitter (Blitter support requires TOS 1.2 or
  later, there is a system menu entry to disable/enable the Blitter).

Emulation speed benchmarks are rather infrequently updated.  For your
enjoyment a Dhrystone benchmark program is provided on the boot disk (diska).
Expect about 50,000 Dhps for Castaway running on an Athlon XP1700+.
The Atari ST (MC68000 clocked at 8MHz) typically executes at about 1600 Dhps.


4. Future Work

If you run into problems drop me a line (see e-mail address above).
Do not expect to get an answer soon, I am often away from my mail
for several days.

If you want to contribute to Castaway, your work will be very much
appreciated.  Just pick any item from the TODO list.


Appendix A: I/O and device emulation status summary

Devices and Interfaces:

serial port     not supported (yet)
parallel port   not supported (yet)
mouse           supported
keyboard        German keyboard layout only (yet)
floppy disk     2 drives, either physical (device inode) or file.
                Supports almost any disk format using 12bit FATs.
hard disk       not supported (yet). As of now, use of large floppy
                disk files should be sufficient (12bit FAT is the limit).
midi port       not supported
DMA port        uggh!
cartridge       not supported (yet)
display modes   640x400 (monochrome), 320x200 (16 colors), 640x200 (4 colors)


Chips and Interrupts

68000           complete
blitter         complete
WD1772          no read/write track commands
IKBD            no controller execute
RTC             no emulation (yet)
ACIAs           keyboard only, midi dead
YM... (sound)   no emulation (yet)
68901           Software EOI mode only
Interrupts      VBL at 50Hz, no HBL.
                System timer at 200 Hz.
Memory Size     any size TOS supports
ROM             Versions 1.x

(yet) means item included in TODO list.


Appendix B:  Bugs

