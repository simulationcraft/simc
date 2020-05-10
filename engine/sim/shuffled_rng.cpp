// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "shuffled_rng.hpp"
#include "util/rng.hpp"


bool shuffled_rng_t::trigger()
{
  if (total_entries <= 0 || success_entries <= 0)
  {
    return false;
  }

  if (total_entries_remaining <= 0)
  {
    reset(); // Re-Shuffle the "Deck"
  }

  bool result = false;
  if (success_entries_remaining > 0)
  {
    result = rng.roll(get_remaining_success_chance());
    if (result)
    {
      success_entries_remaining--;
    }
  }

  total_entries_remaining--;

  if (total_entries_remaining <= 0)
  {
    reset(); // Re-Shuffle the "Deck"
  }

  return result;
}