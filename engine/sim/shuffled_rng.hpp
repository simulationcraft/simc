// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <string>

namespace rng {
  struct rng_t;
}

// "Deck of Cards" randomizer helper class ====================================
// Described at https://www.reddit.com/r/wow/comments/6j2wwk/wow_class_design_ama_june_2017/djb8z68/

class shuffled_rng_t
{
private:
  rng::rng_t& rng;
  std::string  name_str;
  int          success_entries;
  int          total_entries;
  int          success_entries_remaining;
  int          total_entries_remaining;
  
public:
  
  shuffled_rng_t(const std::string& name, rng::rng_t& rng, int success_entries = 0, int total_entries = 0) :
    rng(rng),
    name_str(name),
    success_entries(success_entries),
    total_entries(total_entries),
    success_entries_remaining(success_entries),
    total_entries_remaining(total_entries)
  { }

  const std::string& name() const
  {
    return name_str;
  }

  int get_success_entries() const
  {
    return success_entries;
  }

  int get_success_entries_remaining() const
  {
    return success_entries_remaining;
  }

  int get_total_entries() const
  {
    return total_entries;
  }

  int get_total_entries_remaining() const
  {
    return total_entries_remaining;
  }

  double get_remaining_success_chance() const
  {
    return (double)success_entries_remaining / (double)total_entries_remaining;
  }

  void reset()
  {
    success_entries_remaining = success_entries;
    total_entries_remaining = total_entries;
  }

  bool trigger();
};