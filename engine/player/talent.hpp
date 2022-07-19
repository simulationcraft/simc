// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef TALENT_HPP
#define TALENT_HPP

#include "dbc/spell_data.hpp"
#include "dbc/trait_data.hpp"

struct player_t;

class player_talent_t
{
  const player_t*     m_player;
  const trait_data_t* m_trait;
  const spell_data_t* m_spell;
  unsigned            m_rank;

public:
  player_talent_t();
  player_talent_t( const player_t* );
  player_talent_t( const player_t*, const trait_data_t*, unsigned rank );

  bool enabled() const
  { return m_rank > 0 && m_spell->ok(); }

  bool ok() const
  { return enabled(); }

  unsigned rank() const
  { return m_rank; }

  const trait_data_t* trait() const
  { return m_trait; }

  const spell_data_t* spell() const
  { return m_spell; }

  const spell_data_t* operator->() const
  { return m_spell; }
};

#endif /* TALENT_HPP */

