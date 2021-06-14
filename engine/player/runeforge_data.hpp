// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef RUNEFORGE_DATA_HPP
#define RUNEFORGE_DATA_HPP

#include "dbc/item_runeforge.hpp"
#include "dbc/item_effect.hpp"

#include "report/reports.hpp"

#include <memory>

struct spell_data_t;
struct item_t;
struct expr_t;
struct player_t;

class item_runeforge_t
{
  const runeforge_legendary_entry_t* m_entry;
  const item_t*                      m_item;
  const spell_data_t*                m_spell;

  item_runeforge_t( const spell_data_t* spell );
public:
  static const item_runeforge_t& nil();
  static const item_runeforge_t& not_found();

  item_runeforge_t();
  item_runeforge_t( const runeforge_legendary_entry_t& entry, const item_t* item );

  bool enabled() const;
  bool ok() const
  { return enabled(); }

  const runeforge_legendary_entry_t& runeforge() const;
  const item_effect_t& item_effect() const;

  const item_t* item() const
  { return m_item; }

  const spell_data_t* operator->() const
  { return m_spell; }

  operator const spell_data_t*() const
  { return m_spell; }
};

namespace runeforge
{
std::unique_ptr<expr_t> create_expression( const player_t* player,
                                           util::span<const util::string_view> expr_str, util::string_view full_expression_str );
report::sc_html_stream& generate_report( const player_t& player, report::sc_html_stream& root );
} // Namespace runeforge ends

#endif /* RUNEFORGE_DATA_HPP */
