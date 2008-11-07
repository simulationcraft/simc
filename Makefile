
# WARNING!  WARNING!  WARNING!
#
# THESE OPTIMIZATION FLAGS ARE VALID ONLY FOR SSE-ENABLED SYSTEMS!
# MOST MODERN PROCESSORS SHOULD SUPPORT SSE.
#
# WARNING!  WARNING!  WARNING!

PG   =
BITS = 32
MCP  = -DHAVE_SSE2 -msse -msse2 -mfpmath=sse
OPTS = -maccumulate-outgoing-args -O3 

ifneq (64,${BITS})
	OPTS += -malign-double 
endif

INC = -I. -I./sfmt

SRC =\
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
	sc_rand.cpp		\
	sc_rating.cpp		\
	sc_report.cpp		\
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

# For POSIX-compiant platforms...

simcraft: $(SRC) Makefile
	g++ $(PG) $(MCP) $(OPTS) -Wall $(INC) $(SRC) -lpthread -o simcraft

debug:
	g++ $(PG) $(MCP) -g -Wall $(INC) $(SRC) -lpthread -o simcraft

# For Windows platform... (using MinGW)
# No SSE currently, since it crashes simcraft with SFMT and multiple threads

windows:
	g++ $(OPTS) -Wall $(INC) $(SRC) -o simcraft

windows-debug:
	g++ -g -Wall $(INC) $(SRC) -o simcraft

# For MAC platform...

mac:
	g++ -arch ppc -arch i386 $(MCP) -O3 -Wall $(INC) $(SRC) -lpthread -o simcraft

mac-debug:
	g++ -arch ppc -arch i386 $(MCP) -g -Wall $(INC) $(SRC) -lpthread -o simcraft

REV=0
tarball:
	tar -cvf simcraft-r$(REV).tar $(SRC) simcraft.h sfmt/* Makefile raid_70.txt raid_80.txt
	gzip simcraft-r$(REV).tar

clean:
	rm -f simcraft
