#
# Makefile for GNU make
#
# TODO: add dependency checking

CC = gcc

# SDL-Library configuration:
SDLLIBS  = $(shell sdl2-config --libs 2>/dev/null || sdl-config --libs)
SDLFLAGS = $(shell sdl2-config --cflags 2>/dev/null || sdl-config --cflags)
#SDLLIBS  = -L. -lSDLmain -lsdl

#INCS = -I. -Icpu -Ist -I/usr/X11R6/include/X11
INCS = -I. -Icpu -Ist -I/usr/X11R6/include/X11 -Isdl/include
OPTS = -O3 -march=native 
WRNS = -Wall -Wno-unused
LIBS =  $(SDLLIBS) #-lc -L/usr/X11R6/lib -lX11
DEFS = -Di386 # or -Dsun4

CFLAGS = $(OPTS) $(DEFS) $(WRNS) $(INCS) $(SDLFLAGS)

#
# source code 
#

CPU_CSRC = 68000.c debug.c savestate_stub.c op68kadd.c op68karith.c op68ksub.c op68klogop.c \
           op68kmisc.c op68kmove.c op68kshift.c

EMU_CSRC = init.c st.c mem.c ikbd.c blitter.c fdc.c sound.c

GUI_CSRC = main.c  # or x11.c for not just mono blitting

#
# everything should work fine below (P for PATH).
#

PCPU_CSRC = $(CPU_CSRC:%=cpu/%)
PEMU_CSRC = $(EMU_CSRC:%=st/%)
PGUI_CSRC = $(GUI_CSRC:%=sdl/%)

CSRC = $(PEMU_CSRC) $(PCPU_CSRC) $(PGUI_CSRC)

CPU_COBJ = $(CPU_CSRC:%.c=obj/%.o)
EMU_COBJ = $(EMU_CSRC:%.c=obj/%.o)
GUI_COBJ = $(GUI_CSRC:%.c=obj/%.o)

COBJ = $(EMU_COBJ) $(CPU_COBJ) $(GUI_COBJ)

OBJECTS = $(COBJ) obj/emu2149.o obj/cycles68000.o
.PHONY: default
default: castaway

obj/emu2149.o: emu2149.c emu2149.h
	$(CC) $(CFLAGS) -c emu2149.c -o obj/emu2149.o

obj/cycles68000.o: cpu/cycles68000.c cpu/cycles68000.h
	$(CC) $(CFLAGS) -c cpu/cycles68000.c -o obj/cycles68000.o
#
# production targets 
# 

castaway: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

obj/%.o : cpu/%.c config.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o : st/%.c config.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o : sdl/%.c config.h
	$(CC) $(CFLAGS) -c $< -o $@

obj/emu2149.o: emu2149.c emu2149.h
	$(CC) $(CFLAGS) -c emu2149.c -o obj/emu2149.o

obj/emu2149.o: emu2149.c emu2149.h
	$(CC) $(CFLAGS) -c emu2149.c -o obj/emu2149.o


all:	clean castaway

tools:		mkdisk rdsingle

mkdisk:		tools/mkdisk.c
		$(CC) $(CFLAGS) -o mkdisk tools/mkdisk.c

rdsingle:	tools/rdsingle.c
		$(CC) $(CFLAGS) -o rdsingle tools/rdsingle.c

#
# create a tgz archive named most-nnnnnn.tgz,
# where nnnnnn is the date.
#

HERE = $(shell pwd)
HEREDIR = $(shell basename $(HERE))
TGZ = $(shell echo $(HEREDIR)-`date +%y%m%d`|tr A-Z a-z).tgz
#TGZEXCL = --exclude aes --exclude vdi
TGZEXCL = 

tgz:	clean
	cd ..;\
	tar -cf - --exclude '*CVS' $(TGZEXCL) $(HEREDIR) | gzip -c -9 >$(TGZ)



clean:
	rm -f obj/*.o st/*~ cpu/*~ sdl/*~ doc/*~ *~ castaway core

