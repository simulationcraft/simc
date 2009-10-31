# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# To build on Unix/Mac: make 
# To build on Windows with MinGW: c:\mingw\bin\mingw32-make OS=WINDOWS

# To build debuggable executable, add OPTS=-g to cmd-line invocation
# By default, 32-bit binary is built.  To build a 64-bit binary, add BITS=64 to the cmd-line invocation
# Override MODULE on the cmd-line invocation if you want to build a custom named executable, e.g. 'simc64'
# Override OBJ_DIR if you want your object files built somewhere other than the local directory

MODULE     = simc
LIBRARY    = libsimc.a
OS         = UNIX

# Any UNIX platform
ifeq (UNIX,${OS})
CXX        = g++
CPP_FLAGS  = -Wall -Woverloaded-virtual -Wno-unused-parameter -Wswitch-enum
LINK_FLAGS = 
LINK_LIBS  = -lpthread
OPTS       = -O3 -ffast-math
BITS       = 32
AR         = ar
COPY       = cp
ifeq (64,${BITS})
CPP_FLAGS  += -m64
LINK_FLAGS += -m64
else
ifeq (i386,${HOSTTYPE})
CPP_FLAGS  += -malign-double
endif
ifeq (i686,${HOSTTYPE})
CPP_FLAGS  += -malign-double
endif
endif
endif

ifeq (yes,${USE_CURL})
CPP_FLAGS += -DUSE_CURL
LINK_LIBS += -lcurl
endif

# Windows platform with MinGW32
ifeq (WINDOWS,${OS})
MODULE     = simc.exe
CXX        = g++
CPP_FLAGS  = -Wall -Woverloaded-virtual -malign-double
LINK_FLAGS = 
LINK_LIBS  = -lwsock32
OPTS       = -O3
BITS       = 32
COPY       = copy
AR         = ar
endif

SRC =\
	simulationcraft.h	\
	sc_action.cpp		\
	sc_armory.cpp		\
	sc_attack.cpp		\
	sc_buff.cpp		\
	sc_chart.cpp		\
	sc_consumable.cpp	\
	sc_death_knight.cpp	\
	sc_druid.cpp		\
	sc_enchant.cpp		\
	sc_expressions.cpp	\
	sc_event.cpp		\
	sc_gear_stats.cpp	\
	sc_hunter.cpp		\
	sc_http.cpp		\
	sc_item.cpp		\
	sc_js.cpp		\
	sc_log.cpp		\
	sc_mage.cpp		\
	sc_main.cpp		\
	sc_mmo_champion.cpp	\
	sc_option.cpp		\
	sc_paladin.cpp          \
	sc_pet.cpp		\
	sc_player.cpp		\
	sc_plot.cpp		\
	sc_priest.cpp		\
	sc_raid_event.cpp	\
	sc_rating.cpp		\
	sc_rawr.cpp		\
	sc_report.cpp		\
	sc_rng.cpp		\
	sc_rogue.cpp		\
	sc_shaman.cpp		\
	sc_scaling.cpp		\
	sc_sequence.cpp		\
	sc_set_bonus.cpp	\
	sc_sim.cpp		\
	sc_spell.cpp		\
	sc_stats.cpp		\
	sc_target.cpp		\
	sc_thread.cpp		\
	sc_unique_gear.cpp	\
	sc_util.cpp		\
	sc_warlock.cpp		\
	sc_warrior.cpp		\
	sc_weapon.cpp		\
	sc_wowhead.cpp		\
	sc_xml.cpp

SRC_H   := $(filter %.h,   $(SRC))
SRC_CPP := $(filter %.cpp, $(SRC))
OBJ_DIR = .
OBJ_EXT = o
SRC_OBJ	:= $(SRC_CPP:%.cpp=$(OBJ_DIR)/%.$(OBJ_EXT))

.PHONY:	all mostlyclean clean

all: $(MODULE) $(LIBRARY)

install: all
	$(COPY) $(MODULE) ..
	$(COPY) $(LIBRARY) ..

$(MODULE): $(SRC_OBJ)
	-@echo [$(MODULE)] Linking $@
	@$(CXX) $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

$(LIBRARY): $(SRC_OBJ)
	-@echo [$(MODULE)] Archiving $@
	@$(AR) rv $@ $^

$(OBJ_DIR)/%.$(OBJ_EXT): %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $(notdir $<)
	@$(CXX) $(CPP_FLAGS) $(OPTS) -c $< -o $@

# Release targets (may use platform-specific cmds)

REV=0
tarball:
	tar -cvf simc-r$(REV).tar $(SRC) Makefile *.simc
	gzip simc-r$(REV).tar

mostlyclean:
	-@echo [$(MODULE)] Cleaning intermediate files
	@rm -f $(SRC_OBJ)

clean: mostlyclean
	-@echo [$(MODULE)] Cleaning target files
	@rm -f $(MODULE)

# Deprecated targets

unix windows mac:
	$(error unix/windows/mac are no longer a valid targets, please read the docs at the top of Makefile)
