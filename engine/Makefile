# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# To build on Unix/Mac/Windows: make

# To build debuggable executable, add OPTS=-g to cmd-line invocation
# By default, 32-bit binary is built.  To build a 64-bit binary, add BITS=64 to the cmd-line invocation
# Override MODULE on the cmd-line invocation if you want to build a custom named executable, e.g. 'simc64'
# Override OBJ_DIR if you want your object files built somewhere other than the local directory

ifeq (${OS},)
ifdef SystemRoot
OS         = WINDOWS
else
OS         = UNIX
endif
endif

OPENSSL_PATH =

CXX        = g++
CPP_FLAGS  = -Wall -Woverloaded-virtual -W -I.
ifneq (${NO_DEBUG},)
CPP_FLAGS += -DNDEBUG
endif
OPTS       = -O3 -fomit-frame-pointer -ffast-math
BITS       = 32

-include ../build.conf

ifeq (64,${BITS})
CPP_FLAGS += -m64
LINK_FLAGS+= -m64
endif

# Any UNIX platform
ifeq (UNIX,${OS})
LINK_LIBS += -lpthread
AR         = ar
COPY       = cp
REMOVE     = rm -f
PATHSEP    = /
ifneq (64,${BITS})
ifeq (i386,${HOSTTYPE})
CPP_FLAGS  += -malign-double
endif
ifeq (i686,${HOSTTYPE})
CPP_FLAGS  += -malign-double
endif
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
# MinGW W32
ifeq (32,${BITS})
CPP_FLAGS += -malign-double
endif
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
	sc_action_state.cpp \
	sc_action.cpp \
	sc_attack.cpp \
	sc_buff.cpp \
	sc_consumable.cpp \
	sc_cooldown.cpp \
	sc_dot.cpp \
	sc_enchant.cpp \
	sc_expressions.cpp \
	sc_event.cpp \
	sc_gear_stats.cpp \
	sc_heal.cpp \
	sc_item.cpp \
	sc_main.cpp \
	sc_option.cpp \
	sc_pet.cpp \
	sc_player.cpp \
	sc_plot.cpp \
	sc_raid_event.cpp \
	sc_rating.cpp \
	sc_reforge_plot.cpp \
	sc_rng.cpp \
	sc_sample_data.cpp \
	sc_scaling.cpp \
	sc_sequence.cpp \
	sc_set_bonus.cpp \
	sc_sim.cpp \
	sc_spell_base.cpp \
	sc_harmful_spell.cpp \
	sc_absorb.cpp \
	sc_stats.cpp \
	sc_thread.cpp \
	sc_unique_gear.cpp \
	sc_util.cpp \
	sc_weapon.cpp \
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
	interfaces$(PATHSEP)sc_mmo_champion.cpp \
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

install: all
	$(COPY) $(MODULE) ..

$(MODULE): $(SRC_OBJ)
	-@echo [$(MODULE)] Linking $@
	@$(CXX) $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

sc_http$(MODULE_EXT): interfaces$(PATHSEP)sc_http.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -std=c++0x -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

sc_rng$(MODULE_EXT): sc_rng.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@	

sc_expressions$(MODULE_EXT): sc_expressions.cpp sc_util.cpp
	-@echo [$@] Linking
	$(CXX) $(CPP_FLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@	

$(OBJ_DIR)$(PATHSEP)%.$(OBJ_EXT): %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $(notdir $<)
	@$(CXX) $(CPP_FLAGS) $(OPTS) -c $< -o $@

%.s: %.cpp $(SRC_H)
	-@echo [$(MODULE)] Compiling $(notdir $<)
	@$(CXX) $(CPP_FLAGS) $(OPTS) -S $< -o $@


mostlyclean:
	-@echo [$(MODULE)] Cleaning intermediate files
	@$(REMOVE) $(SRC_OBJ)

clean: mostlyclean
	-@echo [$(MODULE)] Cleaning target files
	@$(REMOVE) $(MODULE) sc_http$(MODULE_EXT)

# Deprecated targets

unix windows mac:
	$(error unix/windows/mac are no longer a valid targets, please read the docs at the top of Makefile)
