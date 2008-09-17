// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Pet
// ==========================================================================

// pet_t::pet_t =============================================================

pet_t::pet_t( sim_t*             s, 
	      player_t*          o,
	      const std::string& n ) :
  player_t( s, PLAYER_PET, n ), owner(o), next_pet(0)
{
  level = owner -> level;
  full_name_str = owner -> name_str + "_" + name_str;
  next_pet = owner -> pet_list;
  owner -> pet_list = this;
}

// pet_t::stamina ===========================================================

double pet_t::stamina()
{
  return player_t::stamina() + 0.30 * owner -> stamina();
}

// pet_t::intellect =========================================================

double pet_t::intellect()
{
  return player_t::intellect() + 0.30 * owner -> intellect();
}

// pet_t::summon ============================================================

void pet_t::summon()
{
  if( sim -> log ) report_t::log( sim, "%s summons %s.", owner -> name(), name() );
  init_resources( true );
}

// pet_t::dismiss ===========================================================

void pet_t::dismiss()
{
  if( sim -> log ) report_t::log( sim, "%s dismisses %s", owner -> name(), name() );

  for( action_t* a = action_list; a; a = a -> next )
  {
    a -> cancel();
  }

  sim -> cancel_events( this );
}
