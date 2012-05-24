// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#ifndef SC_CLASS_MODULES
#define SC_CLASS_MODULES

// Enabled classes
#define SC_DEATH_KNIGHT 1
#define SC_DRUID        1
#define SC_HUNTER       1
#define SC_MAGE         1
#define SC_MONK         1
#define SC_PALADIN      1
#define SC_PRIEST       1
#define SC_ROGUE        0
#define SC_SHAMAN       1
#define SC_WARLOCK      1
#define SC_WARRIOR      0

// Forward Declaration of class register functions, which the class modules are obliged to define.

namespace class_modules {

namespace create {

player_t* death_knight( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* druid       ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* hunter      ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* mage        ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* monk        ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* paladin     ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* priest      ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* rogue       ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* shaman      ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* warlock     ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* warrior     ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
player_t* enemy       ( sim_t* sim, const std::string& name, race_e r = RACE_NONE );
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

} // END class_modules NAMESPACE

// Class Create helper functions

// Needs to be a class template with overloaded () operator, because partial function template specialization
// is not allowed in C++98.
template <typename Class, bool X>
struct sc_create_class
{
  player_t* operator() ( std::string /* class_name */, sim_t* s, std::string name, race_e rt )
  {
    return new Class( s, name, rt );
  }
};

// Template specialization if class is disabled
template <typename Class>
struct sc_create_class<Class,false>
{
  player_t* operator() ( std::string class_name, sim_t* s, std::string name, race_e )
  {
    s -> errorf( "\n%s class module for player %s is currently not available.\n", class_name.c_str(), name.c_str() );
    return 0;
  }
};

#endif // SC_CLASS_MODULES
