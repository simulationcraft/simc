# ==========================================================================
# Dedmonwakeen's Raid DPS/TPS Simulator.
# Send questions to natehieter@gmail.com
# ==========================================================================

# To build debuggable executable, add OPTS=-g to make invocation

OPTS = -O3
BITS = 32

ifneq (64,${BITS})
	OPTS += -malign-double 
endif

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
	sc_rating.cpp		\
	sc_report.cpp		\
	sc_rng.cpp		\
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

simcraft unix: $(SRC) simcraft.h Makefile
	g++ $(OPTS) -Wall $(SRC) -lpthread -o simcraft

# For Windows platform... (using MinGW)

windows:
	g++ $(OPTS) -Wall $(SRC) -o simcraft

# For MAC platform...

mac:
	g++ -arch ppc -arch i386 $(OPTS) -Wall $(SRC) -lpthread -o simcraft

REV=0
tarball:
	tar -cvf simcraft-r$(REV).tar $(SRC) simcraft.h Makefile raid_70.txt raid_80.txt
	gzip simcraft-r$(REV).tar

clean:
	rm -f simcraft
