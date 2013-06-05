# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# To build on Unix/Mac/Windows: make

# To build debuggable executable, add OPTS=-g to cmd-line invocation
# By default, 32-bit binary is built.  To build a 64-bit binary, add BITS=64 to the cmd-line invocation
# Override MODULE on the cmd-line invocation if you want to build a custom named executable, e.g. 'simc64'
# Override OBJ_DIR if you want your object files built somewhere other than the local directory


# Detect operating system
FLAVOR     =
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


OPENSSL_PATH =

CXX        = g++
CPP_FLAGS  = -Wall -Woverloaded-virtual -W -I.
OPTS       = -O3
# Force on SSE2 to avoid problems with older GCC versions
OPTS += -msse2 -mfpmath=sse

ifneq (${NO_DEBUG},)
  CPP_FLAGS += -DNDEBUG
endif
ifneq (${C++11},)
  CPP_FLAGS += -std=c++0x
endif
ifneq (${LTO},)
  OPTS += -flto
endif
ifneq (${MARCH_NATIVE},)
  OPTS += -march=native
endif
ifneq (${FFAST-MATH},)
  OPTS += -ffast-math
endif

# Include build setting from user configuration file
-include ../build.conf

ifeq (32,${BITS})
  OPTS += -m32
endif
ifeq (64,${BITS})
  OPTS += -m64
endif

# Any UNIX platform
ifeq (UNIX,${OS})
  LINK_LIBS += -lpthread
  AR         = ar
  COPY       = cp
  REMOVE     = rm -f
  PATHSEP    = /
    ifeq (i386,${HOSTTYPE})
        optimized::CPP_FLAGS  += -malign-double
    endif
    ifeq (i686,${HOSTTYPE})
        optimized::CPP_FLAGS  += -malign-double
    endif
endif

# Windows platform with MinGW32
ifeq (WINDOWS,${OS})
  MODULE_EXT = .exe
  CPP_FLAGS += -DUNICODE
  LINK_LIBS  = -lws2_32 -lwininet
  ifeq (${NO_STATIC},)
    LINK_FLAGS += -static
  endif
  COPY       = copy
  REMOVE     = del /f
  PATHSEP    = \\
  AR         = ar
  
  optimized::CPP_FLAGS += -malign-double
endif

MODULE     = simc$(MODULE_EXT)

ifneq ($(OPENSSL_PATH),)
CPP_FLAGS += -DUSE_OPENSSL -I$(OPENSSL_PATH)/include
LINK_LIBS += -L$(OPENSSL_PATH)/lib -lssleay32
endif

SRC = \
	simulationcraft.hpp \
	sc_generic.hpp \
	sc_timespan.hpp \
	utf8.h \
	sc_io.cpp \
	sc_main.cpp \
	sc_thread.cpp \
	sc_util.cpp \
	util$(PATHSEP)sample_data.hpp \
	util$(PATHSEP)timeline.hpp \
	util$(PATHSEP)rng.hpp \
	util$(PATHSEP)rng.cpp \
	sim$(PATHSEP)sc_cooldown.cpp \
	sim$(PATHSEP)sc_event.cpp \
	sim$(PATHSEP)sc_expressions.cpp \
	sim$(PATHSEP)sc_gear_stats.cpp \
	sim$(PATHSEP)sc_option.cpp \
	sim$(PATHSEP)sc_plot.cpp \
	sim$(PATHSEP)sc_raid_event.cpp \
	sim$(PATHSEP)sc_reforge_plot.cpp \
	sim$(PATHSEP)sc_scaling.cpp \
	sim$(PATHSEP)sc_sim.cpp \
	buff$(PATHSEP)sc_buff.cpp \
	player$(PATHSEP)sc_consumable.cpp \
	player$(PATHSEP)sc_item.cpp \
	player$(PATHSEP)sc_pet.cpp \
	player$(PATHSEP)sc_player.cpp \
	player$(PATHSEP)sc_set_bonus.cpp \
	player$(PATHSEP)sc_unique_gear.cpp \
	action$(PATHSEP)sc_absorb.cpp \
	action$(PATHSEP)sc_action_state.cpp \
	action$(PATHSEP)sc_action.cpp \
	action$(PATHSEP)sc_attack.cpp \
	action$(PATHSEP)sc_dot.cpp \
	action$(PATHSEP)sc_harmful_spell.cpp \
	action$(PATHSEP)sc_heal.cpp \
	action$(PATHSEP)sc_sequence.cpp \
	action$(PATHSEP)sc_spell_base.cpp \
	action$(PATHSEP)sc_stats.cpp \
	report$(PATHSEP)sc_report.hpp \
	report$(PATHSEP)sc_chart.cpp \
	report$(PATHSEP)sc_report_html_player.cpp \
	report$(PATHSEP)sc_report_html_sim.cpp \
	report$(PATHSEP)sc_report_text.cpp \
	report$(PATHSEP)sc_report_xml.cpp \
	report$(PATHSEP)sc_report.cpp \
	interfaces$(PATHSEP)sc_bcp_api.cpp \
	interfaces$(PATHSEP)sc_chardev.cpp \
	interfaces$(PATHSEP)sc_http.cpp \
	interfaces$(PATHSEP)sc_js.cpp \
	interfaces$(PATHSEP)sc_rawr.cpp \
	interfaces$(PATHSEP)sc_wowhead.cpp \
	interfaces$(PATHSEP)sc_xml.cpp \
	dbc$(PATHSEP)data_definitions.hh \
	dbc$(PATHSEP)data_enums.hh \
	dbc$(PATHSEP)dbc.hpp \
	dbc$(PATHSEP)sc_const_data.cpp \
	dbc$(PATHSEP)sc_item_data.cpp \
	dbc$(PATHSEP)sc_spell_data.cpp \
	dbc$(PATHSEP)sc_spell_info.cpp \
	dbc$(PATHSEP)sc_data.cpp \
	dbc$(PATHSEP)sc_extra_data.inc \
	dbc$(PATHSEP)sc_extra_data_ptr.inc \
	dbc$(PATHSEP)sc_item_data.inc \
	dbc$(PATHSEP)sc_item_data_ptr.inc \
	dbc$(PATHSEP)sc_scale_data.inc \
	dbc$(PATHSEP)sc_scale_data_ptr.inc \
	dbc$(PATHSEP)sc_spec_list.inc \
	dbc$(PATHSEP)sc_spell_data.inc \
	dbc$(PATHSEP)sc_spell_data_ptr.inc \
	dbc$(PATHSEP)sc_spell_lists.inc \
	dbc$(PATHSEP)sc_spell_lists_ptr.inc \
	dbc$(PATHSEP)sc_talent_data.inc \
	dbc$(PATHSEP)sc_talent_data_ptr.inc \
	class_modules$(PATHSEP)sc_death_knight.cpp \
	class_modules$(PATHSEP)sc_druid.cpp \
	class_modules$(PATHSEP)sc_enemy.cpp \
	class_modules$(PATHSEP)sc_hunter.cpp \
	class_modules$(PATHSEP)sc_mage.cpp \
	class_modules$(PATHSEP)sc_monk.cpp \
	class_modules$(PATHSEP)sc_paladin.cpp \
	class_modules$(PATHSEP)sc_priest.cpp \
	class_modules$(PATHSEP)sc_rogue.cpp \
	class_modules$(PATHSEP)sc_shaman.cpp \
	class_modules$(PATHSEP)sc_warlock.cpp \
	class_modules$(PATHSEP)sc_warrior.cpp
SRC_H   := $(filter %.h, $(SRC)) $(filter %.hh, $(SRC)) $(filter %.hpp, $(SRC)) $(filter %.inc, $(SRC))
SRC_CPP := $(filter %.cpp, $(SRC))
OBJ_DIR = .
OBJ_EXT = o
SRC_OBJ	:= $(SRC_CPP:%.cpp=$(OBJ_DIR)$(PATHSEP)%.$(OBJ_EXT))

.PHONY:	all mostlyclean clean

all: $(MODULE)

debug:OPTS += -g -fno-omit-frame-pointer -O2
debug: $(MODULE)

release:CPP_FLAGS += -DNDEBUG
release: $(MODULE)
	
optimized:CPP_FLAGS += -DNDEBUG -std=c++0x
optimized:OPTS += -march=native -ffast-math -fomit-frame-pointer
optimized: $(MODULE)

install: all
	$(COPY) $(MODULE) ..

$(MODULE): $(SRC_OBJ)
	-@echo [$(MODULE)] Linking $@
	@$(CXX) $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

$(OBJ_DIR)$(PATHSEP)%.$(OBJ_EXT): %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $(notdir $<)
	@$(CXX) $(CPP_FLAGS) $(OPTS) -c $< -o $@

%.s: %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $(notdir $<)
	$(CXX) $(CPP_FLAGS) $(OPTS) -S $< -o $@

# cleanup targets
mostlyclean:
	-@echo [$(MODULE)] Cleaning intermediate files
	@$(REMOVE) $(SRC_OBJ)

clean: mostlyclean
	-@echo [$(MODULE)] Cleaning target files
	@$(REMOVE) $(MODULE) sc_http$(MODULE_EXT)

# Unit Tests
sc_http$(MODULE_EXT): interfaces$(PATHSEP)sc_http.cpp sc_io.cpp sc_thread.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

rng$(MODULE_EXT): util$(PATHSEP)rng.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@
	
timeline$(MODULE_EXT): util$(PATHSEP)timeline.hpp util$(PATHSEP)timeline.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

sample_data$(MODULE_EXT): util$(PATHSEP)sample_data.hpp util$(PATHSEP)sample_data.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

sc_expressions$(MODULE_EXT): sim$(PATHSEP)sc_expressions.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

# Deprecated targets

unix windows mac:
	$(error unix/windows/mac are no longer a valid targets, please read the docs at the top of Makefile)
