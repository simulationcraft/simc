#include "runeforge_data.hpp"

#include "item/item.hpp"
#include "player/player.hpp"
#include "player/covenant.hpp"
#include "dbc/dbc.hpp"
#include "dbc/spell_data.hpp"

#include "sim/expressions.hpp"
#include "report/decorators.hpp"
#include "util/io.hpp"


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
    m_spell = dbc::find_spell( m_item->player, m_entry->spell_id );
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
  if ( bonus.empty() )
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
      // 9.2 has changed item_effect.spell_id on some legendaries to a new spell which triggers the spell
      // corresponding to runeforge_legendary_entry.spell_id
      if ( m_item->player->find_spell( effect.spell_id )->effectN( 1 ).trigger_spell_id() != m_entry->spell_id )
      {
        continue;
      }
    }

    return effect;
  }

  return item_effect_t::nil();
}

namespace runeforge
{
std::unique_ptr<expr_t> create_expression( const player_t* player,
                                           util::span<const util::string_view> expr_str, util::string_view full_expression_str )
{
  if ( expr_str.size() < 2 || !util::str_compare_ci( expr_str.front(), "runeforge" ) )
  {
    return nullptr;
  }

  const auto runeforge = player->find_runeforge_legendary( expr_str[ 1 ], true );
  if ( runeforge == item_runeforge_t::nil() )
  {
    throw std::invalid_argument(
        fmt::format( "Unknown runeforge legendary name '{}'", expr_str[ 1 ] ) );
  }

  if ( expr_str.size() == 2 || ( expr_str.size() == 3 && util::str_compare_ci( expr_str[ 2 ], "equipped" ) ) )
  {
    return expr_t::create_constant( full_expression_str, runeforge->ok() );
  }

  throw std::invalid_argument(
        fmt::format( "Invalid runeforge legendary expression '{}', allowed values are 'equipped'.",
          expr_str[ 2 ] ) );

  return nullptr; // unreachable
}

report::sc_html_stream& generate_report( const player_t& player, report::sc_html_stream& root )
{
  std::string legendary_str;

  auto add_to_str = [ & ]( const item_t& item, const runeforge_legendary_entry_t& entry ) {
    auto s_data = player.find_spell( entry.spell_id );
    if ( s_data->ok() )
    {
      auto html_str = report_decorators::decorated_spell_data( *player.sim, s_data );
      if ( util::str_compare_ci( entry.name, "Unity" ) )
      {
        auto cov = player.covenant->type();
        if ( cov != covenant_e::DISABLED && cov != covenant_e::INVALID )
        {
          html_str.insert( html_str.find( "\">" ), "?covenant=" + std::to_string( static_cast<unsigned>( cov ) ) );
        }
      }
      legendary_str +=
          fmt::format( "<li class=\"nowrap\">{} ({})</li>\n", report_decorators::decorated_item( item ), html_str );
      return true;
    }
    return false;
  };

  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
  {
    const item_t& item = player.items[ i ];
    if ( !item.active() )
      continue;

    bool found = false;

    for ( auto id : item.parsed.bonus_id )
    {
      auto entry = runeforge_legendary_entry_t::find( id, player.dbc->ptr );
      if ( !entry.empty() )
      {
        if ( add_to_str( item, entry.front() ) )
          found = true;
      }
    }

    // fallback to search via item effect spell id
    if ( !found )
    {
      for ( const auto& eff : item.parsed.data.effects )
      {
        auto entry = runeforge_legendary_entry_t::find_by_spellid( eff.spell_id, player.dbc->ptr );
        if ( !entry.empty() )
        {
          add_to_str( item, entry.front() );
        }
      }
    }
  }

  if ( !legendary_str.empty() )
  {
    root << "<tr class=\"left\"><th>Runeforge</th><td><ul class=\"float\">\n";
    root << legendary_str;
    root << "</ul></td></tr>\n";
  }

  return root;
}
} // Namespace runeforge enas
