## Makefile for psplib library
## Copyright (C) 2008 Akop Karapetyan

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Author contact information: pspdev@akop.org

ifndef PSP_FW_VERSION
PSP_FW_VERSION=200
endif

CC=psp-gcc
AR=psp-ar
RANLIB=psp-ranlib
RM=rm -rf
PSPSDK=$(shell psp-config --pspsdk-path)
CFLAGS=-O2 -G0 -Wall -I$(PSPSDK)/include
DEFINES=-D_PSP_FW_VERSION=$(PSP_FW_VERSION)

all: libmypsp.a

libmypsp.a: adhoc.o font.o file.o audio.o \
            ctrl.o video.o image.o ui.o \
            pl_ini.o pl_perf.o pl_vk.o pl_util.o \
            pl_psp.o pl_menu.o
	$(AR) cru $@ $?
	$(RANLIB) $@
	@echo Compiled for firmware $(PSP_FW_VERSION)

clean:
	$(RM) *.o genfont stockfont.h *.a

font.o: font.c font.h stockfont.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

image.o: image.c image.h video.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

video.o: video.c video.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

audio.o: audio.c audio.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

ctrl.o: ctrl.c ctrl.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

file.o: file.c file.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

adhoc.o: adhoc.c adhoc.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

ui.o: ui.c ui.h file.o pl_psp.o ctrl.o font.o pl_menu.o video.o adhoc.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

pl_psp.o: pl_psp.c pl_psp.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

pl_ini.o: pl_ini.c pl_ini.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

pl_vk.o: pl_vk.c pl_vk.h video.o font.o ctrl.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

pl_util.o: pl_util.c pl_util.h image.o file.o video.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

pl_perf.o: pl_perf.c pl_perf.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

pl_menu.o: pl_menu.c pl_menu.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

stockfont.h: stockfont.fd genfont
	./genfont < $< > $@

genfont: genfont.c
	cc $< -o $@
