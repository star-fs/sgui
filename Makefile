# sgui - replacement IEM-style GUI objects for Pure Data
#
# Builds self-contained externals against the *public* m_pd.h.  No pd
# internal headers are needed.
#
# Targets:
#   sgui.pd_linux  - all objects in one file (load explicitly:
#                    "pd -lib sgui" or a "lib" message; setup = sgui_setup)
#   sbng/svsl/shsl/stgl/sknb.pd_linux - one file per object name.  pd loads
#                    these on demand when it meets the object in a patch
#                    (each entry point registers its own object; svsl also
#                    shadows the built-in "vsl", shsl "hsl", stgl "tgl").
#
#   make            build all
#   make install    install to ~/.pd/sgui (plus help files)
#   make install-plugdata
#                   install to PlugData's externals folder, with the pdlua
#                   versions (sbng.pd_lua, sknb.pd_lua) in place of
#                   sbng.$(EXT) and sknb.$(EXT)
#   make clean
#
# m_pd.h is found in this order:
#   1. $(PD_SRC)    (environment or make variable: path to a pd source tree)
#   2. /usr/include/pd
#   3. /usr/local/include/pd
#   4. ./pd/src
# You can also override the include dir directly: make PDINC=/path/to/dir
# (PDINC must be the directory that *contains* m_pd.h)

EXT = pd_linux
CC = cc
# -fvisibility=hidden: keep the vendored iemgui_* functions out of the
# exported symbol table (vanilla pd exports its own iemgui_* with a
# different struct layout; exported duplicates would interpose on ours)
CFLAGS = -O2 -g -Wall -fPIC -fvisibility=hidden -DPD -I$(PDINC)
LIBS = -lm
SOURCES = sgui.c sgui_iem.c sbng.c svsl.c shsl.c stgl.c sknb.c
OBJECTS = $(SOURCES:.c=.o)
ONDEMAND = sbng svsl shsl stgl sknb
TARGETS = sgui.$(EXT) $(addsuffix .$(EXT),$(ONDEMAND))

UNAME_S := $(shell uname -s)
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
EXT = pd_win.dll
LDFLAGS = -shared
else ifeq ($(UNAME_S),Darwin)
EXT = pd_darwin
LDFLAGS = -bundle -undefined dynamic_lookup
else
LDFLAGS = -shared
endif

ifeq ($(origin PDINC),undefined)
  ifeq ($(PD_SRC),)
    ifeq ($(wildcard /usr/include/pd/m_pd.h),)
      ifeq ($(wildcard /usr/local/include/pd/m_pd.h),)
        ifeq ($(wildcard ./pd/src/m_pd.h),)
          PDINC :=
        else
          PDINC := ./pd/src
        endif
      else
        PDINC := /usr/local/include/pd
      endif
    else
      PDINC := /usr/include/pd
    endif
  else
    PDINC := $(PD_SRC)
  endif
endif

ifeq ($(wildcard $(PDINC)/m_pd.h),)
  $(error could not find m_pd.h - set PD_SRC to your pd source tree, or PDINC to the dir containing m_pd.h)
endif

all: $(TARGETS)

sgui.$(EXT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

# per-object externals: pd dlopens <name>.pd_linux and calls <name>_setup,
# so each file only needs its own object plus the shared iemgui helpers
# (the explicit sgui.$(EXT) rule above takes precedence for that target)
%.$(EXT): %.o sgui_iem.o
	$(CC) $(LDFLAGS) -o $@ $< sgui_iem.o $(LIBS)

%.o: %.c sgui.h
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGETS)
	install -d $(HOME)/.pd/sgui
	install -m 644 $(TARGETS) $(HOME)/.pd/sgui
	install -m 644 help/*.pd $(HOME)/.pd/sgui

# PlugData draws pdlua GUIs natively but not the Tk-drawn C bang and knob,
# and pd's native loader runs before pdlua's, so leave those out here
PLUGDATA_DIR = $(HOME)/Documents/plugdata/Externals/sgui
PLUGDATA_LUA = sbng sknb
install-plugdata: $(TARGETS)
	install -d $(PLUGDATA_DIR)
	install -m 644 $(filter-out $(addsuffix .$(EXT),$(PLUGDATA_LUA)),$(TARGETS)) \
		$(addsuffix .pd_lua,$(PLUGDATA_LUA)) $(PLUGDATA_DIR)
	rm -f $(addprefix $(PLUGDATA_DIR)/,$(addsuffix .$(EXT),$(PLUGDATA_LUA)))
	install -m 644 help/*.pd $(PLUGDATA_DIR)

clean:
	rm -f $(OBJECTS) $(TARGETS) sgui.pd_linux sgui.pd_darwin sgui.pd_win.dll
