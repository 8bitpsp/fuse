# psplib library 
# Akop Karapetyan, 2007-2008

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

libmypsp.a: adhoc.o menu.o font.o \
            file.o audio.o \
            ctrl.o video.o image.o ui.o \
            pl_ini.o pl_perf.o pl_vk.o pl_util.o \
            pl_psp.o
	$(AR) cru $@ $?
	$(RANLIB) $@
	@echo Compiled for firmware $(PSP_FW_VERSION)

clean:
	$(RM) *.o genfont stockfont.h *.a

menu.o: menu.c menu.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

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

ui.o: ui.c ui.h file.o pl_psp.o ctrl.o font.o menu.o video.o adhoc.o
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

stockfont.h: stockfont.fd genfont
	./genfont < $< > $@

genfont: genfont.c
	cc $< -o $@
