
# WARNING!  WARNING!  WARNING!
#
# THESE OPTIMIZATION FLAGS ARE VALID ONLY FOR SSE-ENABLED SYSTEMS!
# MOST MODERN PROCESSORS SHOULD SUPPORT SSE.
#
# WARNING!  WARNING!  WARNING!

OPTS = -malign-double -msse -msse2 -mfpmath=sse -maccumulate-outgoing-args -O3 

SFMT = -I./sfmt -DUSE_SFMT -DHAVE_SSE2

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
	sc_rand.cpp		\
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

simcraft opt:
	g++ -I. $(OPTS) $(SFMT) -Wall $(SRC) -o simcraft

debug:
	g++ -DDEBUG -g -I. $(SFMT) -Wall $(SRC) -o simcraft

REV=0
tarball:
	tar -cvf simcraft-r$(REV).tar $(SRC) Makefile profiles/* 
	gzip simcraft-$(REV).tar
