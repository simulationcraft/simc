# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# To build on Unix/Mac: make 
# To build on Windows with MinGW: c:\mingw\bin\mingw32-make OS=WINDOWS

# To build debuggable executable, add OPTS=-g to cmd-line invocation
# By default, 32-bit binary is built.  To build a 64-bit binary, add BITS=64 to the cmd-line invocation


OS = UNIX

# Any UNIX platform
ifeq (UNIX,${OS})
CXX        = g++
CPP_FLAGS  = -Wall
LINK_FLAGS = 
LINK_LIBS  = -lpthread
OPTS       = -O3
BITS       = 32
ifeq (64,${BITS})
CPP_FLAGS  += -m64
LINK_FLAGS += -m64
else
CPP_FLAGS  += -malign-double 
endif
endif

# Windows platform with MinGW32
ifeq (WINDOWS,${OS})
CXX        = g++
CPP_FLAGS  = -Wall -malign-double
LINK_FLAGS = 
LINK_LIBS  =
OPTS       = -O3
BITS       = 32
endif

SRC =\
	simcraft.h		\
	sc_action.cpp		\
	sc_attack.cpp		\
	sc_consumable.cpp	\
	sc_druid.cpp		\
	sc_enchant.cpp		\
	sc_event.cpp		\
	sc_mage.cpp		\
	sc_option.cpp		\
	sc_pet.cpp		\
	sc_player.cpp		\
	sc_priest.cpp		\
	sc_rating.cpp		\
	sc_report.cpp		\
	sc_rng.cpp		\
	sc_rogue.cpp		\
	sc_shaman.cpp		\
	sc_scaling.cpp		\
	sc_sim.cpp		\
	sc_spell.cpp		\
	sc_stats.cpp		\
	sc_target.cpp		\
	sc_thread.cpp		\
	sc_unique_gear.cpp	\
	sc_util.cpp		\
	sc_warlock.cpp		\
	sc_weapon.cpp

SRC_H   := $(filter %.h,   $(SRC))
SRC_CPP := $(filter %.cpp, $(SRC))
SRC_OBJ := $(SRC_CPP:.cpp=.o)

simcraft: $(SRC_OBJ)
	$(CXX) $(LINK_FLAGS) $(SRC_OBJ) $(LINK_LIBS) -o simcraft

$(SRC_OBJ): %.o: %.cpp $(SRC_H) Makefile
	$(CXX) $(CPP_FLAGS) $(OPTS) -c $< -o $@

# Release targets (may use platform-specific cmds)

REV=0
tarball:
	tar -cvf simcraft-r$(REV).tar $(SRC) Makefile raid_70.txt raid_80.txt
	gzip simcraft-r$(REV).tar

clean:
	rm -f simcraft *.o *.d *~

# Deprecated targets

unix windows mac:
	$(error unix/windows/mac are no longer a valid targets, please read the docs at the top of Makefile)
