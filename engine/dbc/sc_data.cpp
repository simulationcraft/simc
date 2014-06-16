// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Spell Data
// ==========================================================================

spell_data_nil_t spell_data_nil_t::singleton;

spell_data_not_found_t spell_data_not_found_t::singleton;

// spell_data_t::override ===================================================

bool spell_data_t::override_field( const std::string& field, double value )
{
  if ( util::str_compare_ci( field, "prj_speed" ) )
    _prj_speed = value;
  else if ( util::str_compare_ci( field, "school" ) )
    _school = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "scaling_class" ) )
    _scaling_type = ( int ) value;
  else if ( util::str_compare_ci( field, "spell_level" ) )
    _spell_level = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "max_level" ) )
    _max_level = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "min_range" ) )
    _min_range = value;
  else if ( util::str_compare_ci( field, "max_range" ) )
    _max_range = value;
  else if ( util::str_compare_ci( field, "cooldown" ) )
    _cooldown = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "internal_cooldown" ) )
    _internal_cooldown = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "gcd" ) )
    _gcd = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "duration" ) )
    _duration = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "rune_cost" ) )
    _rune_cost = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "runic_power_gain" ) )
    _runic_power_gain = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "max_stack" ) )
    _max_stack = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "proc_chance" ) )
    _proc_chance = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "proc_charges" ) )
    _proc_charges = ( unsigned ) value;
  else if ( util::str_compare_ci( field, "cast_min" ) )
    _cast_min = ( int ) value;
  else if ( util::str_compare_ci( field, "cast_max" ) )
    _cast_max = ( int ) value;
  else if ( util::str_compare_ci( field, "rppm" ) )
    _rppm = value;
  else
    return false;
  return true;
}

// ==========================================================================
// Spell Effect Data
// ==========================================================================

spelleffect_data_nil_t spelleffect_data_nil_t::singleton;

bool spelleffect_data_t::override_field( const std::string& field, double value )
{
  if ( util::str_compare_ci( field, "average" ) )
    _m_avg = value;
  else if ( util::str_compare_ci( field, "delta" ) )
    _m_delta = value;
  else if ( util::str_compare_ci( field, "bonus" ) )
    _m_unk = value;
  else if ( util::str_compare_ci( field, "coefficient" ) )
    _sp_coeff = value;
  else if ( util::str_compare_ci( field, "period" ) )
    _amplitude = value;
  else if ( util::str_compare_ci( field, "base_value" ) )
    _base_value = ( int ) value;
  else if ( util::str_compare_ci( field, "misc_value1" ) )
    _misc_value = ( int ) value;
  else if ( util::str_compare_ci( field, "misc_value2" ) )
    _misc_value_2 = ( int ) value;
  else if ( util::str_compare_ci( field, "chain_multiplier" ) )
    _m_chain = value;
  else if ( util::str_compare_ci( field, "points_per_combo_points" ) )
    _pp_combo_points = value;
  else if ( util::str_compare_ci( field, "points_per_level" ) )
    _real_ppl = value;
  else if ( util::str_compare_ci( field, "die_sides" ) )
    _die_sides = ( int ) value;
  else
    return false;
  return true;
}

// ==========================================================================
// Spell Power Data
// ==========================================================================

spellpower_data_nil_t spellpower_data_nil_t::singleton;

// ==========================================================================
// Talent Data
// ==========================================================================

talent_data_nil_t talent_data_nil_t::singleton;
