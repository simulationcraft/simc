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

  proc_flags_ = 0;
  proc_flags2_ = 0;
  
  stat = STAT_NONE;
  stat_amount = 0;

  school = SCHOOL_NONE;
  discharge_amount = 0;
  discharge_scaling = 0;
  aoe = 0;
  
  // Must match buff creator defaults for now
  max_stacks = -1;
  proc_chance_ = -1;

  // ppm < 0 = real ppm, ppm > 0 = normal "ppm"
  ppm_ = 0;
  rppm_scale = RPPM_NONE;

  // Must match buff creator defaults for now
  duration = timespan_t::min();

  // Currently covers both cooldown and internal cooldown, as typically
  // cooldown is only used for on-use effects. Must match buff creator default
  // for now.
  cooldown_ = timespan_t::min();

  tick = timespan_t::zero();
  
  cost_reduction = false;
  no_refresh = false;
  chance_to_discharge = false;
  reverse = false;
  proc_delay = false;
  unique = false;
  weapon_proc = false;
  
  override_result_es_mask = 0;
  result_es_mask = 0;
  
  spell_id = 0;
  trigger_spell_id = 0;

  execute_action = 0;
  custom_buff = 0;
}

// special_effect_t::driver =================================================

const spell_data_t* special_effect_t::driver() const
{
  if ( ! item )
    return spell_data_t::nil();

  return item -> player -> find_spell( spell_id );
}

// special_effect_t::trigger ================================================

const spell_data_t* special_effect_t::trigger() const
{
  if ( ! item )
    return spell_data_t::nil();

  if ( trigger_spell_id > 0 )
    return item -> player -> find_spell( trigger_spell_id );

  for ( size_t i = 1, end = driver() -> effect_count(); i <= end; i++ )
  {
    if ( driver() -> effectN( i ).trigger() -> ok() )
      return driver() -> effectN( i ).trigger();
  }

  return spell_data_t::nil();
}

// special_effect_t::create_buff ============================================

buff_t* special_effect_t::create_buff() const
{
  if ( custom_buff != 0 )
    return custom_buff;

  return 0;
}

// special_effect_t::proc_flags =============================================

unsigned special_effect_t::proc_flags() const
{
  if ( proc_flags_ != 0 )
    return proc_flags_;

  if ( driver() -> proc_flags() != 0 )
    return driver() -> proc_flags();

  return 0;
}

// special_effect_t::proc_flags2 ============================================

unsigned special_effect_t::proc_flags2() const
{
  // These do not have a spell-data equivalent, so they are going to always 
  // be set manually, or (most commonly) default to 0. The new proc callback
  // system will automatically use "default values" appropriate for the proc,
  // if it is passed a zero proc_flags2.
  return proc_flags2_;
}

// special_effect_t::ppm ====================================================

double special_effect_t::ppm() const
{
  return ppm_;
}

// special_effect_t::rppm ===================================================

double special_effect_t::rppm() const
{
  if ( ppm_ < 0 )
    return std::fabs( ppm_ );

  return driver() -> real_ppm();
}

// special_effect_t::cooldown ===============================================

timespan_t special_effect_t::cooldown() const
{
  if ( cooldown_ > timespan_t::zero() )
    return cooldown_;

  if ( driver() -> cooldown() > timespan_t::zero() )
    return driver() -> cooldown();
  else if ( driver() -> internal_cooldown() > timespan_t::zero() )
    return driver() -> internal_cooldown();

  return timespan_t::zero();
}

// special_effect_t::proc_chance ============================================

double special_effect_t::proc_chance() const
{
  if ( proc_chance_ > 0 )
    return proc_chance_;

  // Push out the "101%" proc chances automatically, seemingly used as a 
  // special value
  if ( driver() -> proc_chance() > 0 && driver() -> proc_chance() <= 1.0 )
    return driver() -> proc_chance();

  return 0;
}

// special_effect_t::name ===================================================

std::string special_effect_t::name() const
{
  if ( ! name_str.empty() )
    return name_str;

  // Guess proc name based on spells.
  std::string n;
  // Use driver name, if it's not hidden, or there's no trigger spell to use
  if ( ! driver() -> flags( SPELL_ATTR_HIDDEN ) || ! trigger() -> ok() )
    n = driver() -> name_cstr();
  // Driver is hidden, try to use the spell that the driver procs
  else if ( trigger() -> ok() )
    n = trigger() -> name_cstr();

  util::tokenize( n );

  // Assert on empty names; we do need a reasonable name for the proc.
  assert( ! n.empty() );

  // Append weapon suffix automatically to the name.
  // TODO: We need a "shared" mechanism here
  if ( item -> slot == SLOT_OFF_HAND )
    n += "_oh";

  return n;
}

// special_effect_t::to_string ==============================================

std::string special_effect_t::to_string() const
{
  std::ostringstream s;

  s << name();
  s << " type=" << util::special_effect_string( type );
  s << " source=" << util::special_effect_source_string( source );

  if ( ! trigger_str.empty() )
    s << " proc=" << trigger_str;

  if ( spell_id > 0 )
    s << " driver=" << spell_id;

  if ( trigger() -> ok() )
    s << " trigger=" << trigger() -> id();

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

  if ( proc_chance() > 0 )
    s << " proc_chance=" << proc_chance() * 100.0 << "%";

  if ( ppm() > 0 )
    s << " ppm=" << ppm();

  if ( rppm() > 0 )
  {
    s << " rppm=" << rppm();
    switch ( rppm_scale )
    {
      case RPPM_HASTE:
        s << " (Haste)";
        break;
      case RPPM_ATTACK_CRIT:
        s << " (AttackCrit)";
        break;
      case RPPM_SPELL_CRIT:
        s << " (SpellCrit)";
        break;
      default:
        break;
    }
  }

  if ( cooldown() > timespan_t::zero() )
  {
    if ( type == SPECIAL_EFFECT_EQUIP )
      s << " icd=";
    else if ( type == SPECIAL_EFFECT_USE )
      s << " cd=";
    s << cooldown().total_seconds();
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
  if ( proc_chance_ == -1 && ppm_ == 0 )
  {
    // RPPM Trumps proc chance, however in theory we could use both, as most
    // RPPM effects give a (special?) proc chance value of 101%.
    if ( driver_spell -> real_ppm() == 0 )
      proc_chance_ = driver_spell -> proc_chance();
    else
      ppm_ = -1.0 * driver_spell -> real_ppm();
  }

  // Cooldown / Internal cooldown is defined in the driver as well
  if ( cooldown_ == timespan_t::min() )
    cooldown_ = driver_spell -> cooldown();

  if ( cooldown_ == timespan_t::min() )
    cooldown_ = driver_spell -> internal_cooldown();

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

  if ( max_stacks == -1 && proc_spell -> max_stacks() > 0 )
    max_stacks = proc_spell -> max_stacks();

  // Finally, compute the stats/damage of the proc based on the spell data we
  // have, since the user has not defined the stats of the item in the equip=
  // or use= string

  bool has_ap = false;

  for ( size_t i = 1; i <= proc_spell -> effect_count(); i++ )
  {
    const spelleffect_data_t& effect = proc_spell -> effectN( i );
    if ( effect.type() == E_APPLY_AURA )
    {
      stat_e s = STAT_NONE;

      if ( effect.subtype() == A_MOD_STAT )
        s = static_cast< stat_e >( effect.misc_value1() + 1 );
      else if ( effect.subtype() == A_MOD_RATING )
        s = util::translate_rating_mod( effect.misc_value1() );
      else if ( effect.subtype() == A_MOD_DAMAGE_DONE && effect.misc_value1() == 126 )
        s = STAT_SPELL_POWER;
      else if ( effect.subtype() == A_MOD_RESISTANCE )
        s = STAT_ARMOR;
      else if ( ! has_ap && ( effect.subtype() == A_MOD_ATTACK_POWER || effect.subtype() == A_MOD_RANGED_ATTACK_POWER ) )
      {
        s = STAT_ATTACK_POWER;
        has_ap = true;
      }
      else if ( effect.subtype() == A_MOD_INCREASE_HEALTH_2 )
        s = STAT_MAX_HEALTH;

      double value = 0;
      if ( source == SPECIAL_EFFECT_SOURCE_ITEM )
        value = util::round( effect.average( item ) );
      else
        value = util::round( effect.average( item.player ) );

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
    else if ( effect.type() == E_SCHOOL_DAMAGE )
    {
      school = proc_spell -> get_school_type();
      if ( source == SPECIAL_EFFECT_SOURCE_ITEM )
        discharge_amount = util::round( effect.average( item ) );
      else
        discharge_amount = util::round( effect.average( item.player ) );

      discharge_scaling = effect.coeff();
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
      effect.proc_chance_ = t.value / 100.0;
    }
    else if ( t.name == "ppm" )
    {
      effect.ppm_ = t.value;
    }
    else if ( util::str_prefix_ci( t.name, "rppm" ) )
    {
      if ( t.value != 0 )
        effect.ppm_ = -t.value;

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
      effect.cooldown_ = timespan_t::from_seconds( t.value );
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
 * driver spell to contain a non-zero RPPM or proc chance. Additionally, we
 * need to have some sort of flags indicating what events trigger the proc.
 */
bool proc::usable_proc( const special_effect_t& effect )
{
  // Valid proc flags (either old- or new style), we can proc this effect.
  if ( ( effect.trigger_type != PROC_NONE && effect.trigger_mask != 0 ) ||
       effect.proc_flags() > 0 )
    return true;
    
  // A non-zero chance to proc it through one of the proc chance triggers
  if ( effect.ppm() > 0 || effect.rppm() > 0 || effect.proc_chance() > 0 )
    return true;

  return false;
}

/**
 * Initialize the proc callback. This method is called by each actor through
 * player_t::register_callbacks(), which is invoked as the last thing in the
 * actor initialization process.
 */
void dbc_proc_callback_t::initialize()
{
  if ( listener -> sim -> debug )
    listener -> sim -> out_debug.printf( "Initializing %s: %s", effect.name().c_str(), effect.to_string().c_str() );
  
  // Initialize proc chance triggers. Note that this code only chooses one, and
  // prioritizes RPPM > PPM > proc chance.
  if ( effect.rppm() > 0 )
    rppm = real_ppm_t( *listener, effect.rppm(), effect.rppm_scale, effect.driver() -> id() );
  else if ( effect.ppm() > 0 )
    ppm = effect.ppm();
  else if ( effect.proc_chance() != 0 )
    proc_chance = effect.proc_chance();

  // Initialize cooldown, if applicable
  if ( effect.cooldown() > timespan_t::zero() )
  {
    cooldown = listener -> get_cooldown( cooldown_name() );
    cooldown -> duration = effect.cooldown();
  }

  // Initialize the potential proc buff through special_effect_t. Can return 0,
  // in which case the proc does not trigger a buff.
  proc_buff = effect.create_buff();
  
  // TODO: Create action (through special_effect_t)
  
  // Register callback to new proc system
  listener -> callbacks.register_callback( effect.proc_flags(), effect.proc_flags2(), this );
}

/**
 * Cooldown name needs some special handling. Cooldowns in WoW are global,
 * regardless of the number of procs (i.e., weapon enchants). Thus, we straight
 * up use the driver's name, or override it with the special_effect_t name_str
 * if it's defined.
 */
std::string dbc_proc_callback_t::cooldown_name() const
{
  if ( ! effect.name_str.empty() )
    return effect.name_str;

  std::string n = effect.driver() -> name_cstr();
  assert( ! n.empty() );
  util::tokenize( n );

  return n;
}

