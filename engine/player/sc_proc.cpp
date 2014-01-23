// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// special_effect_t::reset ======================================

void special_effect_t::reset()
{
  type = SPECIAL_EFFECT_NONE;
  source = SPECIAL_EFFECT_SOURCE_NONE;

  name_str.clear();
  trigger_str.clear();
  encoding_str.clear();
  
  trigger_type = PROC_NONE;
  trigger_mask = 0;
  
  stat = STAT_NONE;
  stat_amount = 0;

  school = SCHOOL_NONE;
  discharge_amount = 0;
  discharge_scaling = 0;
  aoe = 0;
  
  // Must match buff creator defaults for now
  max_stacks = -1;
  
  proc_chance = 0;
  ppm = 0;
  rppm_scale = RPPM_NONE;

  // Must match buff creator defaults for now
  duration = timespan_t::min();

  // Currntly covers both cooldown and internal cooldown, as typically cooldown
  // is only used for on-use effects.
  cooldown = timespan_t::zero();

  tick = timespan_t::zero();
  
  cost_reduction = false;
  no_refresh = false;
  chance_to_discharge = false;
  reverse = false;
  proc_delay = false;
  unique = false;
  
  override_result_es_mask = 0;
  result_es_mask = 0;
  
  spell_id = 0;
  trigger_spell_id = 0;

  execute_action = 0;
  custom_buff = 0;
}

// special_effect_t::to_string ==============================================

std::string special_effect_t::to_string()
{
  std::ostringstream s;

  s << "type=" << util::special_effect_string( type );
  s << " source=" << util::special_effect_source_string( source );

  if ( ! trigger_str.empty() )
    s << " proc=" << trigger_str;
  else
    s << name_str;

  if ( spell_id > 0 )
    s << " driver=" << spell_id;

  if ( trigger_spell_id > 0 )
    s << " trigger=" << trigger_spell_id;

  if ( stat != STAT_NONE )
  {
    s << " stat=" << util::stat_type_abbrev( stat );
    s << " amount=" << stat_amount;
    s << " duration=" << duration.total_seconds();
    if ( tick != timespan_t::zero() )
      s << " tick=" << tick.total_seconds();
    if ( reverse )
      s << " Reverse";
    if ( no_refresh )
      s << " NoRefresh";
  }

  if ( school != SCHOOL_NONE )
  {
    s << " school=" << util::school_type_string( school );
    s << " amount=" << discharge_amount;
    if ( discharge_scaling > 0 )
      s << " coeff=" << discharge_scaling;
  }

  if ( max_stacks > 0 )
    s << " max_stack=" << max_stacks;

  if ( proc_chance > 0 )
    s << " proc_chance=" << proc_chance * 100.0 << "%";

  if ( ppm > 0 )
    s << " ppm=" << ppm;
  else if ( ppm < 0 )
    s << " rppm=" << std::fabs( ppm );

  if ( cooldown > timespan_t::zero() )
  {
    if ( type == SPECIAL_EFFECT_EQUIP )
      s << " icd=";
    else if ( type == SPECIAL_EFFECT_USE )
      s << " cd=";
    s << cooldown.total_seconds();
  }

  return s.str();
}

// special_effect_t::parse_spell_data =======================================

// TODO: Discharge/resource procs, option verification, plumbing to item system
// NOTE: This isn't enabled in the sim currently

bool special_effect_t::parse_spell_data( const item_t& item, unsigned driver_id )
{
  player_t* player = item.player;
  sim_t* sim = item.sim;

  if ( stat != STAT_NONE || school != SCHOOL_NONE )
    return true;

  // No driver spell, exit early, and be content
  if ( driver_id == 0 )
    return true;

  const spell_data_t* driver_spell = player -> find_spell( driver_id );

  if ( driver_spell == spell_data_t::nil() )
  {
    sim -> errorf( "Player %s unable to find the proc driver spell %u for item '%s' in slot %s",
                   player -> name(), spell_id, item.name(), item.slot_name() );
    return false;
  }

  // Driver has the proc chance defined, typically. Use the id-based proc chance
  // only if there's no proc chance or rppm defined for the special effect
  // already
  if ( proc_chance == 0 && ppm == 0 )
  {
    // RPPM Trumps proc chance, however in theory we could use both, as most
    // RPPM effects give a (special?) proc chance value of 101%.
    if ( driver_spell -> real_ppm() == 0 )
      proc_chance = driver_spell -> proc_chance();
    else
      ppm = -1.0 * driver_spell -> real_ppm();
  }

  // Cooldown / Internal cooldown is defined in the driver as well
  if ( cooldown == timespan_t::zero() )
    cooldown = driver_spell -> cooldown();

  if ( cooldown == timespan_t::zero() )
    cooldown = driver_spell -> internal_cooldown();

  const spell_data_t* proc_spell = spell_data_t::nil();

  if ( trigger_spell_id == 0 )
  {
    for ( size_t i = 1; i <= driver_spell -> effect_count(); i++ )
    {
      if ( ( proc_spell = driver_spell -> effectN( i ).trigger() ) != spell_data_t::nil() )
        break;
    }
  }
  else
    proc_spell = player -> find_spell( trigger_spell_id );

  if ( proc_spell == spell_data_t::nil() )
    proc_spell = driver_spell;

  if ( duration == timespan_t::min() && proc_spell -> duration() != timespan_t::zero() )
    duration = proc_spell -> duration();

  // Figure out the amplitude of the ticking effect from the proc spell, if
  // there's no user specified tick time
  if ( tick == timespan_t::zero() )
  {
    for ( size_t i = 1; i <= proc_spell -> effect_count(); i++ )
    {
      if ( proc_spell -> effectN( i ).type() != E_APPLY_AURA )
        continue;

      if ( proc_spell -> effectN( i ).period() != timespan_t::zero() )
      {
        tick = proc_spell -> effectN( i ).period();
        break;
      }
    }
  }

  if ( item.item_level() == 0 )
  {
    sim -> errorf( "Player %s unable to compute proc attributes, no ilevel defined for item '%s' in slot %s",
                   player -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( max_stacks == -1 )
    max_stacks = proc_spell -> max_stacks();

  // Finally, compute the stats/damage of the proc based on the spell data we
  // have, since the user has not defined the stats of the item in the equip=
  // or use= string

  const random_prop_data_t& ilevel_points = player -> dbc.random_property( item.item_level() );
  double budget;
  if ( item.parsed.data.quality >= 4 )
    budget = static_cast< double >( ilevel_points.p_epic[ 0 ] );
  else if ( item.parsed.data.quality == 3 )
    budget = static_cast< double >( ilevel_points.p_rare[ 0 ] );
  else
    budget = static_cast< double >( ilevel_points.p_uncommon[ 0 ] );

  for ( size_t i = 1; i <= proc_spell -> effect_count(); i++ )
  {
    stat_e s = STAT_NONE;

    if ( proc_spell -> effectN( i ).type() != E_APPLY_AURA )
      continue;

    if ( proc_spell -> effectN( i ).subtype() == A_MOD_STAT )
      s = static_cast< stat_e >( proc_spell -> effectN( i ).misc_value1() + 1 );
    else if ( proc_spell -> effectN( i ).subtype() == A_MOD_RATING )
      s = util::translate_rating_mod( proc_spell -> effectN( i ).misc_value1() );

    double value = util::round( budget * proc_spell -> effectN( i ).m_average() );

    // Bail out on first valid non-zero stat value
    if ( s != STAT_NONE && value != 0 )
    {
      stat = s;
      stat_amount = value;
      // On-Use stat buffs need an increase in stack count, since it may not 
      // be explicitly defined in the spell data. Set it to 1 to suppress a 
      // warning from the buff initialization code.
      if ( max_stacks == -1 )
        max_stacks = 1;
      break;
    }
  }

  spell_id = driver_id;

  return true;
}

// proc::parse_special_effect ===============================================

bool proc::parse_special_effect_encoding( special_effect_t& effect,
                                          const item_t& item,
                                          const std::string& encoding )
{
  if ( encoding.empty() || encoding == "custom" || encoding == "none" ) return true;

  std::vector<item_database::token_t> tokens;

  size_t num_tokens = item_database::parse_tokens( tokens, encoding );

  effect.encoding_str = encoding;

  for ( size_t i = 0; i < num_tokens; i++ )
  {
    item_database::token_t& t = tokens[ i ];
    stat_e s;
    school_e sc;

    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      effect.stat = s;
      effect.stat_amount = t.value;
    }
    else if ( ( sc = util::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      effect.school = sc;
      effect.discharge_amount = t.value;

      std::vector<std::string> splits = util::string_split( t.value_str, "+" );
      if ( splits.size() == 2 )
      {
        effect.discharge_amount  = atof( splits[ 0 ].c_str() );
        effect.discharge_scaling = atof( splits[ 1 ].c_str() ) / 100.0;
      }
    }
    else if ( t.name == "stacks" || t.name == "stack" )
    {
      effect.max_stacks = ( int ) t.value;
    }
    else if ( t.name == "%" )
    {
      effect.proc_chance = t.value / 100.0;
    }
    else if ( t.name == "ppm" )
    {
      effect.ppm = t.value;
    }
    else if ( util::str_prefix_ci( t.name, "rppm" ) )
    {
      effect.ppm = -t.value;

      if ( util::str_in_str_ci( t.name, "spellcrit" ) )
        effect.rppm_scale = RPPM_SPELL_CRIT;
      else if ( util::str_in_str_ci( t.name, "attackcrit" ) )
        effect.rppm_scale = RPPM_ATTACK_CRIT;
      else if ( util::str_in_str_ci( t.name, "haste" ) )
        effect.rppm_scale = RPPM_HASTE;
      else
        effect.rppm_scale = RPPM_NONE;
    }
    else if ( t.name == "duration" || t.name == "dur" )
    {
      effect.duration = timespan_t::from_seconds( t.value );
    }
    else if ( t.name == "cooldown" || t.name == "cd" )
    {
      effect.cooldown = timespan_t::from_seconds( t.value );
    }
    else if ( t.name == "tick" )
    {
      effect.tick = timespan_t::from_seconds( t.value );
    }
    else if ( t.full == "reverse" )
    {
      effect.reverse = true;
    }
    else if ( t.full == "chance" )
    {
      effect.chance_to_discharge = true;
    }
    else if ( t.name == "costrd" )
    {
      effect.cost_reduction = true;
      effect.no_refresh = true;
    }
    else if ( t.name == "nocrit" )
    {
      effect.override_result_es_mask |= RESULT_CRIT_MASK;
      effect.result_es_mask &= ~RESULT_CRIT_MASK;
    }
    else if ( t.name == "maycrit" )
    {
      effect.override_result_es_mask |= RESULT_CRIT_MASK;
      effect.result_es_mask |= RESULT_CRIT_MASK;
    }
    else if ( t.name == "nomiss" )
    {
      effect.override_result_es_mask |= RESULT_MISS_MASK;
      effect.result_es_mask &= ~RESULT_MISS_MASK;
    }
    else if ( t.name == "maymiss" )
    {
      effect.override_result_es_mask |= RESULT_MISS_MASK;
      effect.result_es_mask |= RESULT_MISS_MASK;
    }
    else if ( t.name == "nododge" )
    {
      effect.override_result_es_mask |= RESULT_DODGE_MASK;
      effect.result_es_mask &= ~RESULT_DODGE_MASK;
    }
    else if ( t.name == "maydodge" )
    {
      effect.override_result_es_mask |= RESULT_DODGE_MASK;
      effect.result_es_mask |= RESULT_DODGE_MASK;
    }
    else if ( t.name == "noparry" )
    {
      effect.override_result_es_mask |= RESULT_PARRY_MASK;
      effect.result_es_mask &= ~RESULT_PARRY_MASK;
    }
    else if ( t.name == "mayparry" )
    {
      effect.override_result_es_mask |= RESULT_PARRY_MASK;
      effect.result_es_mask |= RESULT_PARRY_MASK;
    }
    else if ( t.name == "norefresh" )
    {
      effect.no_refresh = true;
    }
    else if ( t.full == "aoe" )
    {
      effect.aoe = -1;
    }
    else if ( t.name == "aoe" )
    {
      effect.aoe = ( int ) t.value;
      if ( effect.aoe < -1 )
        effect.aoe = -1;
    }
    else if ( t.name == "driver" )
    {
      effect.spell_id = static_cast<int>( t.value );
    }
    else if ( t.name == "trigger" )
    {
      effect.trigger_spell_id = static_cast<int>( t.value );
    }
    else if ( t.full == "ondamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "onheal" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_HEAL;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ontickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "onspelltickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ondirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ondirectcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_CRIT;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "onspelldirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ondirectharmfulspellhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onspelldamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_SPELL_MASK;
    }
    else if ( t.full == "onattackdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onattacktickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onattackdirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onarcanedamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_ARCANE );
    }
    else if ( t.full == "onbleeddamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_PHYSICAL );
    }
    else if ( t.full == "onchaosdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_CHAOS );
    }
    else if ( t.full == "onfiredamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_FIRE );
    }
    else if ( t.full == "onfrostdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_FROST );
    }
    else if ( t.full == "onfrostfiredamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_FROSTFIRE );
    }
    else if ( t.full == "onholydamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_HOLY );
    }
    else if ( t.full == "onnaturedamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_NATURE );
    }
    else if ( t.full == "onphysicaldamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_PHYSICAL );
    }
    else if ( t.full == "onshadowdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_SHADOW );
    }
    else if ( t.full == "ondraindamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_DRAIN );
    }
    else if ( t.full == "ontick" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_ALL_MASK;
    }
    else if ( t.full == "ontickhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "ontickcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspellcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else if ( t.full == "onspellhit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onspellcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL_AND_TICK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspelltickcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspelldirectcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspellmiss" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onharmfulspellcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else if ( t.full == "onharmfulspellhit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL_LANDING;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onharmfulspellcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onharmfulspellmiss" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL_LANDING;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onhealcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else if ( t.full == "onhealhit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onhealdirectcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onhealmiss" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onattack" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_ALL_MASK;
    }
    else if ( t.full == "onattackhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onattackcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onattackmiss" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onspelldamageheal" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE_HEAL;
      effect.trigger_mask = SCHOOL_SPELL_MASK;
    }
    else if ( t.full == "ondamagehealspellcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_DAMAGE_HEAL_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else
    {
      item.sim -> errorf( "Player %s has unknown 'use/equip=' token '%s' at slot %s\n", item.player -> name(), t.full.c_str(), item.slot_name() );
      return false;
    }
  }

  return true;
}

/*
 * Figure out the number of usable effects, given a spell id. It'll scan
 * through the effects in the spell, and any possible (chains) of triggred
 * spells, and see if there are any directly convertible effects to a proc (or
 * an on use effect) for simc. This function needs to be kept updated with the
 * generic support we have for procs.
 */
int proc::usable_effects( player_t* player, unsigned spell_id )
{
  int n_usable = 0;
  const spell_data_t* spell = player -> find_spell( spell_id );
  if ( spell == spell_data_t::nil() )
    return 0;

  for ( size_t i = 1; i <= spell -> effect_count(); i++ )
  {
    if ( spell -> effectN( i ).trigger() != spell_data_t::nil() )
      n_usable += usable_effects( player, spell -> effectN( i ).trigger() -> id() );

    effect_type_t type = spell -> effectN( i ).type();
    effect_subtype_t subtype = spell -> effectN( i ).subtype();

    if ( type == E_SCHOOL_DAMAGE )
      n_usable++;
    else if ( type == E_APPLY_AURA )
    {
      if ( subtype == A_MOD_STAT )
        n_usable++;
      else if ( subtype == A_MOD_RATING )
        n_usable++;
      else if ( subtype == A_MOD_DAMAGE_DONE )
        n_usable++;
      else if ( subtype == A_MOD_RESISTANCE )
        n_usable++;
      else if ( subtype == A_MOD_ATTACK_POWER )
        n_usable++;
      else if ( subtype == A_MOD_RANGED_ATTACK_POWER )
        n_usable++;
      else if ( subtype == A_MOD_INCREASE_HEALTH_2 )
        n_usable++;
    }
  }
  
  return n_usable;
}

/**
 * Ensure that the proc has enough information to actually proc in simc. This
 * essentially requires a non-zero proc_chance or PPM field in effect, or the
 * driver spell to contain a non-zero RPPM or proc chance.
 */
bool proc::usable_proc( player_t* player,
                        const special_effect_t& effect,
                        unsigned driver_id )
{
  if ( effect.ppm != 0 || effect.proc_chance != 0 )
    return true;

  const spell_data_t* driver = player -> find_spell( driver_id );
  if ( driver -> real_ppm() != 0 || driver -> proc_chance() != 0 )
    return true;

  return false;
}
