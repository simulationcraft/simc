#include "runeforge_data.hpp"

#include "item/item.hpp"
#include "player/sc_player.hpp"
#include "dbc/dbc.hpp"
#include "dbc/spell_data.hpp"

#include "sim/sc_expressions.hpp"

const item_runeforge_t& item_runeforge_t::nil()
{
  static item_runeforge_t __nil { spell_data_t::nil() };
  return __nil;
}

const item_runeforge_t& item_runeforge_t::not_found()
{
  static item_runeforge_t __not_found { spell_data_t::not_found() };
  return __not_found;
}

item_runeforge_t::item_runeforge_t() :
  m_entry( &runeforge_legendary_entry_t::nil() ), m_item( nullptr ),
  m_spell( spell_data_t::nil() )
{
}

item_runeforge_t::item_runeforge_t( const spell_data_t* spell ) :
  m_entry( &runeforge_legendary_entry_t::nil() ), m_item( nullptr ),
  m_spell( spell )
{
}

item_runeforge_t::item_runeforge_t( const runeforge_legendary_entry_t& entry, const item_t* item ) :
  m_entry( &entry ), m_item( item ), m_spell( nullptr )
{
  const auto& effect = item_effect();
  if ( effect.id != 0 )
  {
    m_spell = dbc::find_spell( m_item->player, effect.spell_id );
  }
  else
  {
    m_spell = spell_data_t::not_found();
  }
}

bool item_runeforge_t::enabled() const
{
  return m_entry->bonus_id != 0 && m_item;
}

const runeforge_legendary_entry_t& item_runeforge_t::runeforge() const
{
  return *m_entry;
}

const item_effect_t& item_runeforge_t::item_effect() const
{
  if ( !enabled() )
  {
    return item_effect_t::nil();
  }

  auto bonus = m_item->player->dbc->item_bonus( m_entry->bonus_id );
  if ( bonus.size() == 0 )
  {
    return item_effect_t::nil();
  }

  // Find the correct effect application by going over the item bonuses, and comparing
  // spell ids and item effect spell ids
  for ( const auto& entry : bonus )
  {
    if ( entry.type != ITEM_BONUS_ADD_ITEM_EFFECT )
    {
      continue;
    }

    const auto& effect = item_effect_t::find( entry.value_1, m_item->player->dbc->ptr );
    if ( !effect.id || effect.spell_id != m_entry->spell_id )
    {
      continue;
    }

    return effect;
  }

  return item_effect_t::nil();
}

namespace runeforge
{
std::unique_ptr<expr_t> create_expression( const player_t* player,
                                           util::span<const util::string_view> expr_str )
{
  if ( expr_str.size() != 3 )
  {
    return nullptr;
  }

  if ( !util::str_compare_ci( expr_str.front(), "runeforge" ) )
  {
    return nullptr;
  }

  const auto runeforge = player->find_runeforge_legendary( expr_str[ 1 ], true );
  if ( runeforge == item_runeforge_t::nil() )
  {
    throw std::invalid_argument(
        fmt::format( "Unknown runeforge legendary name '{}'", expr_str[ 1 ] ) );
  }

  if ( util::str_compare_ci( expr_str[ 2 ], "equipped" ) )
  {
    return expr_t::create_constant( "runeforge_equipped",
        static_cast<double>( runeforge->ok() ) );
  }
  else
  {
    throw std::invalid_argument(
        fmt::format( "Invalid runeforge legendary expression '{}', allowed values are 'equipped'.",
          expr_str[ 2 ] ) );
  }

  return nullptr;
}
} // Namespace runeforge enas
