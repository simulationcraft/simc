#include "sim/pvp_scaling.hpp"

#include "sim/sim.hpp"
#include "dbc/spell_data.hpp"
#include "util/util.hpp"

namespace pvp
{

// Parse user-provided PvP coefficient override string.
// Format: "spell_id:multiplier/espell_effect_id:multiplier" ('e' prefix = effect-level)
void parse_coefficient_overrides( pvp_config_t& pvp )
{
  if ( pvp.coefficient_override_str.empty() )
    return;

  auto entries = util::string_split<std::string_view>( pvp.coefficient_override_str, "/" );
  for ( auto entry : entries )
  {
    if ( entry.empty() )
      continue;

    bool is_effect = entry[ 0 ] == 'e' || entry[ 0 ] == 'E';
    if ( is_effect )
      entry.remove_prefix( 1 );

    auto parts = util::string_split<std::string_view>( entry, ":" );
    if ( parts.size() != 2 )
      continue;

    unsigned id = util::to_unsigned( parts[ 0 ] );
    double coeff = std::stod( std::string( parts[ 1 ] ) );

    if ( is_effect )
      pvp.effect_overrides[ id ] = coeff;
    else
      pvp.coefficient_overrides[ id ] = coeff;
  }
}

// Set dampening defaults based on PvP format. Arena starts dampening immediately,
// battlegrounds have no dampening, wargames start at 5 minutes. (2026-03-20)
void init_format_defaults( pvp_config_t& pvp, bool user_set_dampening_start )
{
  if ( pvp.mode == "arena" )
  {
    if ( !user_set_dampening_start )
      pvp.dampening_start_sec = 0.0;
  }
  else if ( pvp.mode == "bg" )
  {
    pvp.dampening_enabled = false;
  }
  else if ( pvp.mode == "wargame" )
  {
    if ( !user_set_dampening_start )
      pvp.dampening_start_sec = 300.0;
  }
}

// Iterate spell 134735 effects by aura subtype and populate pvp_config_t fields.
// As of 12.0.1 the spell has 11 effects; only effect 3 (A_448, crit -50%) is non-zero.
// Subtypes 42/119/240 are skipped (proc trigger, PvP state check, expertise). (2026-03-20)
void init_modifiers_from_spell( pvp_config_t& pvp, const spell_data_t* pvp_rules, sim_t* sim )
{
  if ( !pvp_rules || !pvp_rules->ok() )
    return;

  if ( pvp_rules->effect_count() != 11 && sim )
    sim->print_debug( "PvP: spell 134735 has {} effects (expected 11)", pvp_rules->effect_count() );

  for ( size_t i = 1; i <= pvp_rules->effect_count(); i++ )
  {
    const auto& e = pvp_rules->effectN( i );
    switch ( e.subtype() )
    {
      case A_MOD_HEALING_RECEIVED_PCT:
        pvp.healing_received_mod = 1.0 + e.percent();
        break;
      case A_MOD_ABSORB_RECEIVED_PERCENT:
        pvp.absorb_received_mod = 1.0 + e.percent();
        break;
      case A_448: // "Mod Crit Healing %" — reduces crit bonus damage/healing by 50%
        pvp.crit_damage_mod = 1.0 + e.percent();
        break;
      case A_MOD_RESILIENCE:
        if ( e.base_value() != 0 )
          pvp.crit_damage_mod = 1.0 + e.percent();
        break;
      case A_MOD_MANA_REGEN_PCT:
        pvp.mana_regen_mod = 1.0 + e.percent();
        break;
      case A_MOD_PET_DAMAGE_DONE:
        pvp.pet_damage_mod = 1.0 + e.percent();
        break;
      case A_MOD_RESISTANCE_PCT:
        pvp.resistance_mod = 1.0 + e.percent();
        break;
      default:
        if ( sim )
          sim->print_debug( "PvP: Unhandled spell 134735 effect {} subtype {}", i, static_cast<unsigned>( e.subtype() ) );
        break;
    }
  }
}

} // namespace pvp
