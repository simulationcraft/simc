// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/talent.hpp"
#include "player/player.hpp"

player_talent_t::player_talent_t() :
  m_player( nullptr ), m_trait( &( trait_data_t::nil() ) ), m_spell( spell_data_t::not_found() ),
  m_rank( 0 )
{ }

player_talent_t::player_talent_t( const player_t* player ) :
  m_player( player ), m_trait( &( trait_data_t::nil() ) ), m_spell( spell_data_t::not_found() ),
  m_rank( 0 )
{ }

player_talent_t::player_talent_t( const player_t* player, const trait_data_t* trait, unsigned rank ) :
  m_player( player ), m_trait( trait ), m_spell( m_player->find_spell( trait->id_spell ) ),
  m_rank ( rank )
{ }

const spell_data_t* player_talent_t::find_override_spell() const
{
  return m_player->find_spell( m_trait->id_override_spell );
}
