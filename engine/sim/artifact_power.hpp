// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_timespan.hpp"
#include <cstdint>

struct spell_data_t;
struct artifact_power_data_t;
struct artifact_power_rank_t;


struct artifact_power_t
{
  artifact_power_t(unsigned rank, const spell_data_t* spell, const artifact_power_data_t* power,
      const artifact_power_rank_t* rank_data);
  artifact_power_t();

  unsigned rank_;
  const spell_data_t* spell_;
  const artifact_power_rank_t* rank_data_;
  const artifact_power_data_t* power_;

  operator const spell_data_t*() const
  { return spell_; }

  const artifact_power_data_t* power() const
  { return power_; }

  double value( std::size_t idx = 1 ) const;

  timespan_t time_value( std::size_t idx = 1 ) const;

  double percent( std::size_t idx = 1 ) const
  { return value( idx ) * .01; }

  const spell_data_t& data() const
  { return *spell_; }

  unsigned rank() const
  { return rank_; }
};
