// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_class_modules.hpp"

// This file contains the interface between the class modules and the simulation core

// player_t::create =========================================================

player_t* player_t::create( sim_t*             sim,
                            const std::string& type,
                            const std::string& name,
                            race_type_e r )
{
  using namespace class_modules;

  if ( type == "death_knight" || type == "deathknight" )
  {
    return create::death_knight( sim, name, r );
  }
  else if ( type == "druid" )
  {
    return create::druid( sim, name, r );
  }
  else if ( type == "hunter" )
  {
    return create::hunter( sim, name, r );
  }
  else if ( type == "mage" )
  {
    return create::mage( sim, name, r );
  }
  else if ( type == "monk" )
  {
    return create::monk( sim, name, r );
  }
  else if ( type == "priest" )
  {
    return create::priest( sim, name, r );
  }
  else if ( type == "paladin" )
  {
    return create::paladin( sim, name, r );
  }
  else if ( type == "rogue" )
  {
    return create::rogue( sim, name, r );
  }
  else if ( type == "shaman" )
  {
    return create::shaman( sim, name, r );
  }
  else if ( type == "warlock" )
  {
    return create::warlock( sim, name, r );
  }
  else if ( type == "warrior" )
  {
    return create::warrior( sim, name, r );
  }
  else if ( type == "enemy" )
  {
    return create::enemy( sim, name, r );
  }
  return 0;
}

void player_t::init_class_modules( sim_t* sim )
{
  using namespace class_modules;
  init::death_knight( sim );
  init::druid       ( sim );
  init::hunter      ( sim );
  init::mage        ( sim );
  init::monk        ( sim );
  init::paladin     ( sim );
  init::priest      ( sim );
  init::rogue       ( sim );
  init::shaman      ( sim );
  init::warlock     ( sim );
  init::warrior     ( sim );
  init::enemy       ( sim );
}

// player_t::combat_begin ===================================================

void player_t::combat_begin( sim_t* sim )
{
  using namespace class_modules;
  combat_begin::death_knight( sim );
  combat_begin::druid       ( sim );
  combat_begin::hunter      ( sim );
  combat_begin::mage        ( sim );
  combat_begin::monk        ( sim );
  combat_begin::paladin     ( sim );
  combat_begin::priest      ( sim );
  combat_begin::rogue       ( sim );
  combat_begin::shaman      ( sim );
  combat_begin::warlock     ( sim );
  combat_begin::warrior     ( sim );
  combat_begin::enemy       ( sim );
}

// player_t::combat_end =====================================================

void player_t::combat_end( sim_t* sim )
{
  using namespace class_modules;
  combat_end::death_knight( sim );
  combat_end::druid       ( sim );
  combat_end::hunter      ( sim );
  combat_end::mage        ( sim );
  combat_end::monk        ( sim );
  combat_end::paladin     ( sim );
  combat_end::priest      ( sim );
  combat_end::rogue       ( sim );
  combat_end::shaman      ( sim );
  combat_end::warlock     ( sim );
  combat_end::warrior     ( sim );
  combat_end::enemy       ( sim );
}

option_t* sim_t::get_class_option( void* parse_player_func )
{
  static option_t opt[] =
  {
      // Character Creation
      { "death_knight", OPT_FUNC,   parse_player_func },
      { "deathknight",  OPT_FUNC,   parse_player_func },
      { "druid",        OPT_FUNC,   parse_player_func },
      { "hunter",       OPT_FUNC,   parse_player_func },
      { "mage",         OPT_FUNC,   parse_player_func },
      { "monk",         OPT_FUNC,   parse_player_func },
      { "priest",       OPT_FUNC,   parse_player_func },
      { "paladin",      OPT_FUNC,   parse_player_func },
      { "rogue",        OPT_FUNC,   parse_player_func },
      { "shaman",       OPT_FUNC,   parse_player_func },
      { "warlock",      OPT_FUNC,   parse_player_func },
      { "warrior",      OPT_FUNC,   parse_player_func },
      { "enemy",        OPT_FUNC,   parse_player_func },
      { NULL, OPT_UNKNOWN, NULL }
  };

  return opt;
}
