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

// pet_t::summon ============================================================

void pet_t::summon()
{
  schedule_ready();
}

// pet_t::dismiss ===========================================================

void pet_t::dismiss()
{
  // This will not cancel actions in progress, but probably good enough.....
  sleeping = true;
}
