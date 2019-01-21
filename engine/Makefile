# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# To build on Unix/Mac/Windows: make

# To build debuggable executable, add OPTS=-g to cmd-line invocation
# By default, 32-bit binary is built.  To build a 64-bit binary, add BITS=64 to the cmd-line invocation
# Override MODULE on the cmd-line invocation if you want to build a custom named executable, e.g. 'simc64'
# Override OBJ_DIR if you want your object files built somewhere other than the local directory


FLAVOR     =
DESTDIR    =
PREFIX    ?= ..

# Build base directory
BASE_DIR = $(DESTDIR)$(PREFIX)

# Base "shared data" search path. For the command line client, profiles are
# searched from $PREFIX, or $PREFIX/share/SimulationCraft if $PREFIX is not the
# default ("..")
SHARED_DATA = $(PREFIX)
# Base installation path for the command line client binary, by default "..",
# if $DESTDIR or $PREFIX is given, set to $DESTDIR$PREFIX/bin.
BIN_INSTALL_PATH = $(PREFIX)
# Installation path for the shared files. By default it's not used; if $PREFIX
# is set to something else than "..", set to
# $DESTDIR$PREFIX/share/SimulationCraft
SHARE_INSTALL_PATH =

ifneq (${PREFIX},..)
  ORG_NAME = SimulationCraft
  APP_NAME = SimulationCraft
  SHARED_DATA = $(PREFIX)/share/$(ORG_NAME)/$(APP_NAME)

  BIN_INSTALL_PATH = $(BASE_DIR)/bin
  SHARE_INSTALL_PATH = $(BASE_DIR)/share/$(ORG_NAME)/$(APP_NAME)
endif

ifeq (${OS},Windows_NT)
  OS         = WINDOWS
endif

ifeq (${OS},)
  ifdef SystemRoot
    OS         = WINDOWS
  else
    OS         = UNIX
    FLAVOR     = $(shell uname)
  endif
endif


MKDIR      = mkdir
CXX        = c++
CPP_FLAGS  = -Wall -Wextra -W -I. -I./util -DSC_SHARED_DATA=\"$(SHARED_DATA)\" --std=c++14 -O3 -MMD -MP \
-Wpedantic \
-Wcast-qual \
-Wconversion-null

OPTS       =
GIT        = $(shell [ -d ../.git ] && which git)

LINK_LIBS  =

ifeq (${SC_NO_NETWORKING},)
  LINK_LIBS += -lcurl
else
  OPTS += -DSC_NO_NETWORKING
endif

# Include build setting from user configuration file
-include ../build.conf

# Enable SSE2
DUMPMACHINE =$(shell $(CXX) -dumpmachine)
# SSE2 is explicitly disabled for 32bit MINGW.
ifeq ($(findstring i686-w64-mingw32,${DUMPMACHINE}),)
  OPTS += -msse2 -mfpmath=sse
endif

# OSX
ifeq (${FLAVOR},Darwin)
ifeq (${OPENSSL},)
  LINK_LIBS += -framework Security -framework CoreFoundation
endif
endif

ifneq (${DEBUG},)
  CPP_FLAGS += -g
endif
ifneq (${EVENT_QUEUE_DEBUG},)
  CPP_FLAGS += -DEVENT_QUEUE_DEBUG
endif
ifneq (${NO_DEBUG},)
  CPP_FLAGS += -DNDEBUG
endif
ifneq (${C++17},)
  CPP_FLAGS += --std=c++17
endif
ifneq (${LTO},)
  OPTS += -flto
endif

ifneq (${LTO_THIN},)
  OPTS += -flto=thin
  -fuse-ld=lld
endif
ifneq (${MARCH_NATIVE},)
  OPTS += -march=native
endif
ifneq (${FFAST-MATH},)
  OPTS += -ffast-math
endif

ifeq (32,${BITS})
  OPTS += -m32
endif
ifeq (64,${BITS})
  OPTS += -m64
endif

ifneq (${SANITIZE},)
  OPTS += -fsanitize=address
endif

ifneq (${LLVM_PGO_GENERATE},)
  OPTS += -fprofile-instr-generate
endif

ifneq (${LLVM_PGO_USE},)
  OPTS += -fprofile-instr-use=${LLVM_PGO_USE}
endif

ifneq (${SC_DEFAULT_APIKEY},)
  CPP_FLAGS += -DSC_DEFAULT_APIKEY=\"${SC_DEFAULT_APIKEY}\"
endif

ifneq (${OPENSSL},)
  ifneq (${OPENSSL_PATH},)
    CPP_FLAGS += -I${OPENSSL_PATH}/include
    LINK_LIBS += -L${OPENSSL_PATH}/lib
  endif
  OPTS += -DSC_USE_OPENSSL
ifeq (WINDOWS,${OS})
  LINK_LIBS += -lssleay32
else
  LINK_LIBS += -lssl
endif
endif

# Any UNIX platform
ifeq (UNIX,${OS})
  LINK_LIBS += -lpthread
  AR         = ar
  COPY       = cp
  REMOVE     = rm -f
  PATHSEP    = /
endif

# Additional include paths for FreeBSD
ifeq (FreeBSD,${FLAVOR})
  CPP_FLAGS += -I/usr/local/include
  LINK_FLAGS += -L/usr/local/lib
endif

# Windows platform with MinGW32
ifeq (WINDOWS,${OS})
  MODULE_EXT = .exe
  CPP_FLAGS += -DUNICODE
  LINK_LIBS  += -lws2_32 -lwininet
  ifeq (${NO_STATIC},)
    LINK_FLAGS += -static
  endif
  COPY       = copy
  REMOVE     = del /f
  PATHSEP    = \\
  AR         = ar
endif

MODULE     = simc$(MODULE_EXT)

include ../source_files/engine_make
include ../source_files/engine_main_make

ifneq (,${GIT})
  OPTS += -DSC_GIT_REV="\"$(shell ${GIT} rev-parse --short HEAD)\""
  OPTS += -DSC_GIT_BRANCH="\"$(shell ${GIT} rev-parse --abbrev-ref HEAD)\""
endif

SRC_H   := $(filter %.h, $(SRC)) $(filter %.hh, $(SRC)) $(filter %.hpp, $(SRC)) $(filter %.inc, $(SRC))
SRC_CPP := $(filter %.cpp, $(SRC))
OBJ_DIR = .
OBJ_EXT = o
DEP_EXT = d
SRC_OBJ := $(SRC_CPP:%.cpp=$(OBJ_DIR)$(PATHSEP)%.$(OBJ_EXT))
SRC_DEPS := $(SRC_CPP:%.cpp=$(OBJ_DIR)$(PATHSEP)%.$(DEP_EXT))

.PHONY: .FORCE all mostlyclean clean
.FORCE:

all: $(MODULE)

-include $(SRC_DEPS)

debug:OPTS += -g -fno-omit-frame-pointer -O0 -fno-optimize-sibling-calls
debug: $(MODULE)

release:CPP_FLAGS += -DNDEBUG
release: $(MODULE)

optimized:CPP_FLAGS += -DNDEBUG
optimized:OPTS += -march=native -ffast-math -fomit-frame-pointer
optimized: $(MODULE)

install: all
ifneq (${PREFIX},..)
	$(MKDIR) -p $(BIN_INSTALL_PATH)
endif
	$(COPY) $(MODULE) $(BIN_INSTALL_PATH)
ifneq (${PREFIX},..)
	$(MKDIR) -p $(SHARE_INSTALL_PATH)
	$(COPY) -r $(wildcard ../profiles/*) $(SHARE_INSTALL_PATH)
endif

$(MODULE): $(SRC_OBJ)
	-@echo [$(MODULE)] Linking $@
	@$(CXX) $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

$(OBJ_DIR)$(PATHSEP)%.$(OBJ_EXT): %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $(notdir $<)
	@$(CXX) $(CPP_FLAGS) $(OPTS) -c $< -o $@

%.s: %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $(notdir $<)
	$(CXX) $(CPP_FLAGS) $(OPTS) -S $< -o $@

# Force regeneration of git_info.o on every recompilation to potntially get the
# changed GIT shorthash into the binary
util/git_info.o: .FORCE

# cleanup targets
mostlyclean:
	-@echo [$(MODULE)] Cleaning intermediate files
	@$(REMOVE) $(SRC_OBJ) $(SRC_DEPS)

clean: mostlyclean
	-@echo [$(MODULE)] Cleaning target files
	@$(REMOVE) $(MODULE) sc_http$(MODULE_EXT)

# Unit Tests
sc_http$(MODULE_EXT): interfaces$(PATHSEP)sc_http.cpp util$(PATHSEP)sc_io.cpp sc_thread.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

rng$(MODULE_EXT): util$(PATHSEP)rng.cpp util$(PATHSEP)fmt$(PATHSEP)format.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

timeline$(MODULE_EXT): util$(PATHSEP)timeline.hpp util$(PATHSEP)timeline.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

sample_data$(MODULE_EXT): util$(PATHSEP)sample_data.hpp util$(PATHSEP)sample_data.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

sc_expressions$(MODULE_EXT): sim$(PATHSEP)sc_expressions.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

# Deprecated targets

unix windows mac:
	$(error unix/windows/mac are no longer a valid targets, please read the docs at the top of Makefile)
