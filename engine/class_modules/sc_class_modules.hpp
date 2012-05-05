// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#ifndef SC_CLASS_MODULES
#define SC_CLASS_MODULES

// Forward Declaration of class register functions, which the class modules are obliged to define.

namespace class_modules {

namespace create {

player_t* death_knight( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* druid       ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* hunter      ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* mage        ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* monk        ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* paladin     ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* priest      ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* rogue       ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* shaman      ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* warlock     ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* warrior     ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );
player_t* enemy       ( sim_t* sim, const std::string& name, race_type_e r = RACE_NONE );

}
namespace register_targetdata {
void death_knight ( sim_t* );
void druid        ( sim_t* );
void hunter       ( sim_t* );
void mage         ( sim_t* );
void monk         ( sim_t* );
void paladin      ( sim_t* );
void priest       ( sim_t* );
void rogue        ( sim_t* );
void shaman       ( sim_t* );
void warlock      ( sim_t* );
void warrior      ( sim_t* );
}

namespace combat_begin {

void death_knight ( sim_t* );
void druid        ( sim_t* );
void hunter       ( sim_t* );
void mage         ( sim_t* );
void monk         ( sim_t* );
void paladin      ( sim_t* );
void priest       ( sim_t* );
void rogue        ( sim_t* );
void shaman       ( sim_t* );
void warlock      ( sim_t* );
void warrior      ( sim_t* );
void enemy        ( sim_t* );
}
namespace combat_end {

void death_knight ( sim_t* );
void druid        ( sim_t* );
void hunter       ( sim_t* );
void mage         ( sim_t* );
void monk         ( sim_t* );
void paladin      ( sim_t* );
void priest       ( sim_t* );
void rogue        ( sim_t* );
void shaman       ( sim_t* );
void warlock      ( sim_t* );
void warrior      ( sim_t* );
void enemy        ( sim_t* );
}
namespace init {
// Raid-wide Death Knight buff maintenance
void death_knight ( sim_t* );
void druid        ( sim_t* );
void hunter       ( sim_t* );
void mage         ( sim_t* );
void monk         ( sim_t* );
void paladin      ( sim_t* );
void priest       ( sim_t* );
void rogue        ( sim_t* );
void shaman       ( sim_t* );
void warlock      ( sim_t* );
void warrior      ( sim_t* );
void enemy        ( sim_t* );
}
}

#endif // SC_CLASS_MODULES
