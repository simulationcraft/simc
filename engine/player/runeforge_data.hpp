// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef RUNEFORGE_DATA_HPP
#define RUNEFORGE_DATA_HPP

#include "dbc/item_runeforge.hpp"
#include "dbc/item_effect.hpp"

struct spell_data_t;
struct item_t;

class item_runeforge_t
{
  const runeforge_legendary_entry_t* m_entry;
  const item_t*                      m_item;
  // Cache spell data on first access
  mutable const spell_data_t*        m_spell;

public:
  item_runeforge_t();
  item_runeforge_t( const runeforge_legendary_entry_t& entry, const item_t* item );

  bool enabled() const;
  bool ok() const
  { return enabled(); }

  const item_t* item() const;
  const runeforge_legendary_entry_t& runeforge() const;
  const item_effect_t& item_effect() const;

  operator const spell_data_t*() const;
};

#endif /* RUNEFORGE_DATA_HPP */
