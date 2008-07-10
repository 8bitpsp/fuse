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

libmypsp.a: adhoc.o menu.o  font.o  init.o perf.o file.o psp.o audio.o \
            ctrl.o  video.o image.o kybd.o ui.o   util.o
	$(AR) cru $@ $?
	$(RANLIB) $@
	@echo Compiled for firmware $(PSP_FW_VERSION)

clean:
	$(RM) *.o genfont stockfont.h *.a

menu.o: menu.c menu.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

font.o: font.c font.h stockfont.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

init.o: init.c init.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

image.o: image.c image.h video.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

kybd.o: kybd.c kybd.h video.o font.o ctrl.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

video.o: video.c video.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

audio.o: audio.c audio.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

ctrl.o: ctrl.c ctrl.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

psp.o: psp.c psp.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

perf.o: perf.c perf.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

file.o: file.c file.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

adhoc.o: adhoc.c adhoc.h
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

ui.o: ui.c ui.h psp.o file.o ctrl.o font.o menu.o video.o adhoc.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

util.o: util.c util.h image.o file.o video.o
	$(CC) $(DEFINES) $(CFLAGS) -c -o $@ $<

stockfont.h: stockfont.fd genfont
	./genfont < $< > $@

genfont: genfont.c
	cc $< -o $@
