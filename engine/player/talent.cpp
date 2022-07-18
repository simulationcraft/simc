// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/talent.hpp"
#include "dbc/dbc.hpp"
#include "dbc/item_database.hpp"
#include "player/player.hpp"

player_talent_t::player_talent_t( const player_t* player ) :
  m_player( player ), m_trait( &( trait_data_t::nil() ) ), m_spell( spell_data_t::not_found() ),
  m_rank( 0 )
{ }

player_talent_t::player_talent_t( const player_t* player, const trait_data_t* trait, unsigned rank ) :
  m_player( player ), m_trait( trait ), m_spell( player->find_spell( trait->id_spell ) ),
  m_rank ( rank )
{
  const auto effect_points = trait_definition_effect_entry_t::find( m_trait->id_trait_definition,
      m_player->dbc->ptr );

  if ( effect_points.size() > 0 )
  {
    m_values.resize( effect_points.back().effect_index + 1, 0.0 );
  }

  for ( const auto& effect_point : effect_points )
  {
    auto curve_data = curve_point_t::find( effect_point.id_curve, m_player->dbc->ptr );
    auto value = 0.0f;
    auto it = range::find_if( curve_data, [this]( const auto& point ) {
      return point.primary1 == as<float>( m_rank );
    } );

    if ( it != curve_data.end() )
    {
      value = it->primary2;
    }

    switch ( effect_point.operation )
    {
      case trait_definition_op::TRAIT_OP_SET:
        m_values[ effect_point.effect_index ] = value;
        break;
      case trait_definition_op::TRAIT_OP_MUL:
        m_values[ effect_point.effect_index ] = value *
          m_spell->effectN( effect_point.effect_index + 1U ).base_value();
        break;
      default:
        assert( 0 );
        break;
    }
  }
}
