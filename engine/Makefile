# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# To build on Unix/Mac/Windows: make

# To build debuggable executable, add DEBUG=1 to cmd-line invocation
# By default, 32-bit binary is built.  To build a 64-bit binary, add BITS=64 to the cmd-line invocation
# Override MODULE on the cmd-line invocation if you want to build a custom named executable, e.g. 'simc64'
# Override OBJ_DIR if you want your object files built somewhere other than the local directory


FLAVOR     =
ARCH       =
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
    ARCH       = $(shell uname -m)
  endif
endif


MKDIR      = mkdir
CXX        = c++
CPP_FLAGS  = -Wall -Wextra -W -I. -I./include -I./lib -DSC_SHARED_DATA=\"$(SHARED_DATA)\" --std=c++17 -O3 -MMD -MP \
-Wpedantic \
-Wcast-qual \
-Wconversion-null

OPTS_INTERNAL       =
GIT        = $(shell [ -d ../.git ] && which git)

LINK_LIBS  =

ifeq (${SC_NO_NETWORKING},)
  LINK_LIBS += -lcurl
else
  OPTS_INTERNAL += -DSC_NO_NETWORKING
endif

# Enable universal binary mode for MacOS
ifneq (${UNIVERSAL},)
  MACOS_ARCHS=x86_64 arm64
endif

# Include build setting from user configuration file
-include ../build.conf

# OSX
ifeq (${FLAVOR},Darwin)
  # Explicitly state architectures to compile with
  ifneq ($(MACOS_ARCHS),)
    OPTS_INTERNAL += $(addprefix -arch ,$(MACOS_ARCHS))
  endif

  ifeq (${OPENSSL},)
    LINK_LIBS += -framework Security -framework CoreFoundation
  endif
endif

ifneq (${DEBUG},)
  CPP_FLAGS += -g
endif
ifneq (${CURL_DEBUG},)
  CPP_FLAGS += -DCURL_DEBUG
endif
ifneq (${EVENT_QUEUE_DEBUG},)
  CPP_FLAGS += -DEVENT_QUEUE_DEBUG
endif
ifneq (${NO_DEBUG},)
  CPP_FLAGS += -DNDEBUG
endif

# Variant builds: 'make debug|release|optimized' recurse with VARIANT set and
# OBJ_DIR redirected to build/<variant>, so the three configurations get their
# own object/binary directories and never share (and corrupt) each other's
# objects.
ifeq (debug,${VARIANT})
  OPTS_INTERNAL += -g -fno-omit-frame-pointer -O0 -fno-optimize-sibling-calls
endif
ifeq (release,${VARIANT})
  CPP_FLAGS += -DNDEBUG
endif
ifeq (optimized,${VARIANT})
  CPP_FLAGS += -DNDEBUG
  OPTS_INTERNAL += -fomit-frame-pointer
endif
ifneq (${C++20},)
  CPP_FLAGS += --std=c++20
endif
ifneq (${LTO},)
  OPTS_INTERNAL += -flto
endif
ifneq (${LTO_AUTO},)
  OPTS_INTERNAL += -flto=auto
endif
ifneq (${LTO_THIN},)
  OPTS_INTERNAL += -flto=thin
endif
ifneq (${MARCH},)
  OPTS_INTERNAL += -march=${MARCH}
endif
ifneq (${MCPU},)
  OPTS_INTERNAL += -mcpu=${MCPU}
endif

ifeq (32,${BITS})
  OPTS_INTERNAL += -m32
endif
ifeq (64,${BITS})
  OPTS_INTERNAL += -m64
endif

ifneq (${SANITIZE},)
  OPTS_INTERNAL += -fsanitize=address
endif

ifneq (${LLVM_PGO_GENERATE},)
  OPTS_INTERNAL += -fprofile-instr-generate
endif

ifneq (${LLVM_PGO_USE},)
  OPTS_INTERNAL += -fprofile-instr-use=${LLVM_PGO_USE}
endif

ifneq (${SC_DEFAULT_APIKEY},)
  CPP_FLAGS += -DSC_DEFAULT_APIKEY=\"${SC_DEFAULT_APIKEY}\"
endif

ifneq (${OPENSSL},)
  ifneq (${OPENSSL_PATH},)
    CPP_FLAGS += -I${OPENSSL_PATH}/include
    LINK_LIBS += -L${OPENSSL_PATH}/lib
  endif
  OPTS_INTERNAL += -DSC_USE_OPENSSL
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
  OPTS_INTERNAL += -DSC_GIT_REV="\"$(shell ${GIT} rev-parse --short HEAD)\""
  OPTS_INTERNAL += -DSC_GIT_BRANCH="\"$(shell ${GIT} rev-parse --abbrev-ref HEAD)\""
endif

SRC_H   := $(filter %.h, $(SRC)) $(filter %.hh, $(SRC)) $(filter %.hpp, $(SRC)) $(filter %.inc, $(SRC))
SRC_CPP := $(filter %.cpp, $(SRC))
# Variant builds land under build/<variant>; the default build still goes
# in-tree (OBJ_DIR = .). The binary follows OBJ_DIR via TARGET_BIN.
BUILD_DIR = build
OBJ_DIR = .
OBJ_EXT = o
DEP_EXT = d
SRC_OBJ := $(SRC_CPP:%.cpp=$(OBJ_DIR)$(PATHSEP)%.$(OBJ_EXT))
SRC_DEPS := $(SRC_CPP:%.cpp=$(OBJ_DIR)$(PATHSEP)%.$(DEP_EXT))
TARGET_BIN = $(OBJ_DIR)$(PATHSEP)$(MODULE)

.PHONY: .FORCE all mostlyclean clean
.FORCE:

all: $(TARGET_BIN)

-include $(SRC_DEPS)

# Build each variant in its own directory by recursing with OBJ_DIR redirected;
# VARIANT (read in the flags section above) selects the extra compile flags.
# 'make debug && make release' leaves both binaries side by side under build/.
.PHONY: debug release optimized
debug release optimized:
	@$(MAKE) all OBJ_DIR=$(BUILD_DIR)$(PATHSEP)$@ VARIANT=$@

install: all
ifneq (${PREFIX},..)
	$(MKDIR) -p $(BIN_INSTALL_PATH)
endif
	$(COPY) $(MODULE) $(BIN_INSTALL_PATH)
ifneq (${PREFIX},..)
	$(MKDIR) -p $(SHARE_INSTALL_PATH)
	$(COPY) -r $(wildcard ../profiles/*) $(SHARE_INSTALL_PATH)
endif

$(TARGET_BIN): $(SRC_OBJ)
	-@echo [$(MODULE)] Linking $@
	@$(MKDIR) -p $(dir $@)
	@$(CXX) $(OPTS_INTERNAL) $(OPTS) $(LINK_FLAGS) $^ -o $@ $(LINK_LIBS)

$(OBJ_DIR)$(PATHSEP)%.$(OBJ_EXT): %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $<
	@$(MKDIR) -p $(dir $@)
	@$(CXX) $(CPP_FLAGS) $(OPTS_INTERNAL) $(OPTS) -c $< -o $@

%.s: %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $<
	$(CXX) $(CPP_FLAGS) $(OPTS_INTERNAL) $(OPTS) -S $< -o $@

# Force regeneration of git_info.o on every recompilation to potntially get the
# changed GIT shorthash into the binary
$(OBJ_DIR)$(PATHSEP)util/git_info.$(OBJ_EXT): .FORCE

# cleanup targets
mostlyclean:
	-@echo [$(MODULE)] Cleaning intermediate files
	@$(REMOVE) $(SRC_OBJ) $(SRC_DEPS)

clean: mostlyclean
	-@echo [$(MODULE)] Cleaning target files
	@$(REMOVE) $(MODULE) sc_http$(MODULE_EXT)
	-@$(REMOVE) -r $(BUILD_DIR)

# Unit Tests
sc_http$(MODULE_EXT): interfaces$(PATHSEP)sc_http.cpp util$(PATHSEP)sc_io.cpp sc_thread.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS_INTERNAL) $(OPTS) $(LINK_FLAGS) $^ -o $@ $(LINK_LIBS)

rng$(MODULE_EXT): util$(PATHSEP)rng.cpp lib$(PATHSEP)fmt$(PATHSEP)format.cpp util$(PATHSEP)chrono.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS_INTERNAL) $(OPTS) $(LINK_FLAGS) $^ -o $@ $(LINK_LIBS)

timeline$(MODULE_EXT): util$(PATHSEP)timeline.hpp util$(PATHSEP)timeline.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS_INTERNAL) $(OPTS) $(LINK_FLAGS) $^ -o $@ $(LINK_LIBS)

sample_data$(MODULE_EXT): util$(PATHSEP)sample_data.hpp util$(PATHSEP)sample_data.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS_INTERNAL) $(OPTS) $(LINK_FLAGS) $^ -o $@ $(LINK_LIBS)

sc_expressions$(MODULE_EXT): sim$(PATHSEP)sc_expressions.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS_INTERNAL) $(OPTS) $(LINK_FLAGS) $^ -o $@ $(LINK_LIBS)

# Deprecated targets

unix windows mac:
	$(error unix/windows/mac are no longer a valid targets, please read the docs at the top of Makefile)
