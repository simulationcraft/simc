
OPTS = -mtune=pentium-m -malign-double -msse -msse2 -mfpmath=sse -maccumulate-outgoing-args -O3

SRC =\
	sc_action.cpp		\
	sc_attack.cpp		\
	sc_consumable.cpp	\
	sc_enchant.cpp		\
	sc_event.cpp		\
	sc_option.cpp		\
	sc_pet.cpp		\
	sc_player.cpp		\
	sc_priest.cpp		\
	sc_rating.cpp		\
	sc_report.cpp		\
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
	g++ -DDEBUG -I. $(OPTS) -Wall $(SRC) -o simcraft
