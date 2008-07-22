
# WARNING!  WARNING!  WARNING!
#
# THESE OPTIMIZATION FLAGS ARE VALID ONLY FOR PENTIUM-M BASED SYSTEMS!
#
# WARNING!  WARNING!  WARNING!

OPTS = -mtune=pentium-m -malign-double -msse -msse2 -mfpmath=sse -maccumulate-outgoing-args -O3

SRC =\
	sc_action.cpp		\
	sc_attack.cpp		\
	sc_consumable.cpp	\
	sc_druid.cpp		\
	sc_enchant.cpp		\
	sc_event.cpp		\
	sc_option.cpp		\
	sc_pet.cpp		\
	sc_player.cpp		\
	sc_priest.cpp		\
	sc_rating.cpp		\
	sc_report.cpp		\
	sc_shaman.cpp		\
	sc_sim.cpp		\
	sc_spell.cpp		\
	sc_stats.cpp		\
	sc_target.cpp		\
	sc_unique_gear.cpp	\
	sc_util.cpp		\
	sc_weapon.cpp

simcraft build:
	g++ -I. $(OPTS) -Wall $(SRC) -o simcraft

debug:
	g++ -DDEBUG -g -I. -Wall $(SRC) -o simcraft

REV=0
tarball:
	tar -cvf simcraft-$(REV).tar $(SRC) Makefile profiles/* 
	gzip simcraft-$(REV).tar
