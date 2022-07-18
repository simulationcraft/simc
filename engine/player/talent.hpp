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
  std::vector<double> m_values;

public:
  player_talent_t();
  player_talent_t( const player_t* );
  player_talent_t( const player_t*, const trait_data_t*, unsigned rank );

  bool enabled() const
  { return m_rank > 0; }

  bool ok() const
  { return enabled(); }

  unsigned rank() const
  { return m_rank; }

  const trait_data_t* trait() const
  { return m_trait; }

  const spell_data_t* spell() const
  { return m_spell; }

  double base_value( unsigned effect_index ) const
  {
    assert( effect_index > 0 );

    if ( !enabled() )
    {
      return 0.0;
    }

    if ( effect_index > m_values.size() )
    {
      return 0.0;
    }

    return m_values[ effect_index - 1U ];
  }

  double percent( unsigned effect_index ) const
  { return base_value( effect_index ) * ( 1 / 100.0 ); }
};

#endif /* TALENT_HPP */

