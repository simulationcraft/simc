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

OS         = UNIX
OPENSSL_PATH =

CXX        = g++
CPP_FLAGS  = -Wall -Woverloaded-virtual -W -Wno-strict-aliasing
OPTS       = -O3 -fomit-frame-pointer -ffast-math
BITS       = 32

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
LINK_FLAGS += -static
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
	simulationcraft.h \
	data_definitions.hh \
	data_enums.hh \
	utf8.h \
	sc_action.cpp \
	sc_armory.cpp \
	sc_attack.cpp \
	sc_battle_net.cpp \
	sc_bcp_api.cpp \
	sc_buff.cpp \
	sc_chardev.cpp \
	sc_chart.cpp \
	sc_const_data.cpp \
	sc_consumable.cpp \
	sc_data.cpp \
	sc_death_knight.cpp \
	sc_dot.cpp \
	sc_druid.cpp \
	sc_enchant.cpp \
	sc_expressions.cpp \
	sc_event.cpp \
	sc_gear_stats.cpp \
	sc_heal.cpp \
	sc_hunter.cpp \
	sc_http.cpp \
	sc_item.cpp \
	sc_item_data.cpp \
	sc_js.cpp \
	sc_log.cpp \
	sc_mage.cpp \
	sc_main.cpp \
	sc_mmo_champion.cpp \
	sc_monk.cpp \
	sc_option.cpp \
	sc_paladin.cpp \
	sc_pet.cpp \
	sc_player.cpp \
	sc_plot.cpp \
	sc_priest.cpp \
	sc_raid_event.cpp \
	sc_rating.cpp \
	sc_rawr.cpp \
	sc_reforge_plot.cpp \
	sc_report_html_player.cpp \
	sc_report_html_sim.cpp \
	sc_report_text.cpp \
	sc_report_xml.cpp \
	sc_report.cpp \
	sc_rng.cpp \
	sc_rogue.cpp \
	sc_sample_data.cpp \
	sc_shaman.cpp \
	sc_scaling.cpp \
	sc_sequence.cpp \
	sc_set_bonus.cpp \
	sc_sim.cpp \
	sc_spell.cpp \
	sc_spell_data.cpp \
	sc_spell_info.cpp \
	sc_stats.cpp \
	sc_talent.cpp \
	sc_target.cpp \
	sc_thread.cpp \
	sc_timespan.cpp \
	sc_unique_gear.cpp \
	sc_util.cpp \
	sc_warlock.cpp \
	sc_warrior.cpp \
	sc_weapon.cpp \
	sc_wowhead.cpp \
	sc_xml.cpp

SRC_H   := $(filter %.h, $(SRC)) $(filter %.hh, $(SRC))
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

sc_http$(MODULE_EXT): sc_http.cpp sc_util.cpp
	-@echo [$@] Linking
	@$(CXX) $(CPPFLAGS) -DUNIT_TEST $(OPTS) $(LINK_FLAGS) $^ $(LINK_LIBS) -o $@

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
