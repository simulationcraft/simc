// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_helper.hpp"

namespace {

// Very limited setup to automatically apply some spell data to actions. Currently only considers
// generic (direct damage/healing) and tick damage multipliers. This should be currently enough to
// get automatica application of various spec/class specific, label-based modifiers that have
// started surfacing in 7.1.5 onwards.
void apply_spell_labels( const spell_data_t* spell, action_t* a )
{
  const auto sim = a -> sim;

  for ( size_t i = 1, end = spell -> effect_count(); i <= end; ++i )
  {
    auto& effect = spell -> effectN( i );
    if ( effect.type() != E_APPLY_AURA || effect.subtype() != A_ADD_PCT_LABEL_MODIFIER )
    {
      continue;
    }

    if ( ! a -> data().affected_by_label( effect.misc_value2() ) )
    {
      continue;
    }

    double old_value = 0, new_value = 0;
    std::string modifier_str;
    switch ( static_cast<property_type_t>( effect.misc_value1() ) )
    {
      case P_EFFECT_1:
        old_value = a -> base_multiplier;
        modifier_str = "direct/periodic damage/healing";
        a -> base_multiplier *= 1.0 + effect.percent();
        new_value = a -> base_multiplier;
        break;
      case P_GENERIC:
        old_value = a -> base_dd_multiplier;
        modifier_str = "direct damage/healing";
        a -> base_dd_multiplier *= 1.0 + effect.percent();
        new_value = a -> base_dd_multiplier;
        break;
      case P_TICK_DAMAGE:
        old_value = a -> base_td_multiplier;
        modifier_str = "periodic damage/healing";
        a -> base_td_multiplier *= 1.0 + effect.percent();
        new_value = a -> base_td_multiplier;
        break;
      default:
        break;
    }

    if ( sim -> debug && ! modifier_str.empty() )
    {
      sim -> out_debug.printf( "%s applying %s modifier from %s (id=%u) to %s (id=%u action=%s), value=%.5f -> %.5f",
        a -> player -> name(),
        modifier_str.c_str(),
        spell -> name_cstr(),
        spell -> id(),
        a -> data().name_cstr(),
        a -> data().id(),
        a -> name(),
        old_value,
        new_value );
    }
  }
}
}

namespace unique_gear {
  // Apply all label-based modifiers to an action, if the associated spell data for the application ha
// any labels. Used by proc_action_t-derived spells (currently item special effects) to
// automatically apply various class passives (e.g., Shaman, Enhancement Shaman, ...) to those
// spells. System will be expanded in the future to cover a significantly larger portion of "spell
// application interactions" between different types of objects (e.g., actors and actions).
void apply_label_modifiers( action_t* a )
{
  if ( a -> data().label_count() == 0 )
  {
    return;
  }

  auto spells = dbc::class_passives( a -> player );
  range::for_each( spells, [ a ]( const spell_data_t* spell ) { apply_spell_labels( spell, a ); } );
}
}