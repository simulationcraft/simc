// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Pet
// ==========================================================================

// pet_t::pet_t =============================================================

pet_t::pet_t( sim_t*             s, 
	      player_t*          o,
	      const std::string& n,
	      bool               g ) :
  player_t( s, g ? PLAYER_GUARDIAN : PLAYER_PET, n ), owner(o), next_pet(0)
{
  level = owner -> level;
  full_name_str = owner -> name_str + "_" + name_str;
  next_pet = owner -> pet_list;
  owner -> pet_list = this;

  // Pets have inherent 5% critical strike chance
  base_spell_crit  = 0.05;
  base_attack_crit = 0.05;

    stamina_per_owner = 0.75;
  intellect_per_owner = 0.30;

  // By default, only report statistics in the context of the owner
  quiet = 1;
}

// pet_t::create_action =====================================================

action_t* pet_t::create_action( const std::string& name,
				const std::string& options_str )
{
  return player_t::create_action( name, options_str );
}

// pet_t::stamina ===========================================================

double pet_t::stamina()
{
  return composite_attribute_multiplier( ATTR_STAMINA ) * ( attribute[ ATTR_STAMINA ] + stamina_per_owner * owner -> stamina() );
}

// pet_t::intellect =========================================================

double pet_t::intellect()
{
  return composite_attribute_multiplier( ATTR_INTELLECT ) * ( attribute[ ATTR_INTELLECT ] + intellect_per_owner * owner -> intellect() );
}

// pet_t::init ==============================================================

void pet_t::init()
{
  player_t::init();
  level = owner -> level;
}

// pet_t::reset =============================================================

void pet_t::reset()
{
  player_t::reset();
  sleeping = 1;
  summon_time = 0;
}

// pet_t::summon ============================================================

void pet_t::summon()
{
  if( sim -> log ) report_t::log( sim, "%s summons %s.", owner -> name(), name() );
  sleeping = 0;
  init_resources( true );
  summon_time = sim -> current_time;
}

// pet_t::dismiss ===========================================================

void pet_t::dismiss()
{
  if( sim -> log ) report_t::log( sim, "%s dismisses %s", owner -> name(), name() );

  sleeping = 1;

  for( action_t* a = action_list; a; a = a -> next )
  {
    a -> cancel();
  }

  sim -> cancel_events( this );
}
