EXT = pd_linux
DEFS = 
CC = gcc
CXX = c++
LD = ld
AFLAGS = 
LFLAGS = -export_dynamic  -shared -O2 -Wall -fPIC -DPD
WFLAGS =
IFLAGS = -I./include -I../../pd/src
INSTALL_PREFIX=/usr/lib

.SUFFIXES: .$(EXT)

PDCFLAGS = -g -O2 $(DEFS) $(IFLAGS) $(WFLAGS) $(LFLAGS) $(AFLAGS) -DVERSION=$(VERSION)
CFLAGS = -g -O2 $(DEFS) $(IFLAGS) $(WFLAGS) -DVERSION=$(VERSION)
CXXFLAGS = $(CFLAGS)

LIBS = -lm -lc 
SOURCES =  sgui.c sbng.c svsl.c shsl.c stgl.c sknb.c
TARGETS = $(SOURCES:.c=.$(EXT)) 

all: $(TARGETS) 

sbng:
	cc -c $(CFLAGS) -DPD sbng.c
	$(LD) -export_dynamic  -shared -o sbng.pd_linux *.o $(LIBS)
	strip --strip-unneeded sbng.pd_linux

sknb:
	cc -c $(CFLAGS) -DPD sknb.c
	$(LD) -export_dynamic  -shared -o sknb.pd_linux *.o $(LIBS)
	strip --strip-unneeded sknb.pd_linux

svsl:
	cc -c $(CFLAGS) -DPD svsl.c
	$(LD) -export_dynamic  -shared -o svsl.pd_linux *.o $(LIBS)
	strip --strip-unneeded svsl.pd_linux

shsl:
	cc -c $(CFLAGS) -DPD sshl.c
	$(LD) -export_dynamic  -shared -o shsl.pd_linux *.o $(LIBS)
	strip --strip-unneeded shsl.pd_linux

stgl:
	cc -c $(CFLAGS) -DPD stgl.c
	$(LD) -export_dynamic  -shared -o stgl.pd_linux *.o $(LIBS)
	strip --strip-unneeded stgl.pd_linux

clean::
	-rm *.$(EXT) *.o 

.c.o:
	$(CC) -c -o $@ $(CFLAGS) -DPD $*.c

# cp $@ $*_stat.o

.o.pd_linux:
	$(CC) -o $@ $(PDCFLAGS) -DPD $*.o

install::
	install -d $(INSTALL_PREFIX)/pd/extra
	install -m 644 *.$(EXT) $(INSTALL_PREFIX)/pd/extra
	install -m 644 sgui.pd_linux $(INSTALL_PREFIX)/pd/extra
	install -m 644 help/*.pd $(INSTALL_PREFIX)/pd/doc/5.reference
