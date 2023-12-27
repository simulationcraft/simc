// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "buff/buff.hpp"
#include "player/player.hpp"
#include "sim/sim.hpp"
#include "util/io.hpp"

#include <functional>

// Mixin to player class to allow auto parsing and dynamic application of whitelist based buffs & auras.
// 1) Add `parse_player_buff_effects_t` as an additional parent with the target data class as template parameter:
//
//    struct my_class_t : public player_t, parse_player_buff_effects_t<death_knight_td_t>
//
// 2) Construct the mixin via `parse_player_buff_effects_t( this );`
//
// 3) `get_player_buff_effects_value( buff effect vector ) returns the modified value.
//    Add the following overrides with any addtional adjustments as needed (BASE is the parent to the action base class):

/* 
double my_class_t::composite_base_armor_multiplier() const
{
  return player_t::composite_base_armor_multiplier() * get_player_buff_effects_value( base_armor_multiplier_buffeffects );
}

double my_class_t::composite_dodge() const
{
  return player_t::composite_dodge() + get_player_buff_effects_value( dodge_additive_buffeffects, true );
}

double my_class_t::composite_damage_versatility() const
{
  return player_t::composite_damage_versatility() + get_player_buff_effects_value( versatility_additive_buffeffects, true );
}

double my_class_t::composite_heal_versatility() const
{
  return player_t::composite_heal_versatility() + get_player_buff_effects_value( versatility_additive_buffeffects, true );
}

double my_class_t::composite_mitigation_versatility() const
{
  return player_t::composite_mitigation_versatility() + ( get_player_buff_effects_value( versatility_additive_buffeffects, true ) / 2 );
}

double my_class_t::composite_mastery() const
{
  return  player_t::composite_mastery() + get_player_buff_effects_value( mastery_additive_buffeffects, true );
}

double my_class_t::composite_attack_power_multiplier() const
{
  return player_t::composite_attack_power_multiplier() *
  get_player_buff_effects_value( attack_power_multiplier_buffeffects );
}

double my_class_t::composite_melee_haste() const
{
  return player_t::composite_melee_haste() * ( 1.0 / get_player_buff_effects_value( all_haste_multiplier_buffeffects ) );
}

double my_class_t::composite_spell_haste() const
{
  return player_t::composite_spell_haste() * ( 1.0 / get_player_buff_effects_value( all_haste_multiplier_buffeffects ) );
}

double my_class_t::composite_melee_speed() const
{
  return player_t::composite_melee_speed() *
  ( 1.0 / get_player_buff_effects_value( all_attack_speed_multiplier_buffeffects ) ) *
  ( 1.0 / get_player_buff_effects_value( melee_attack_speed_multiplier_buffeffects ) );
}

double my_class_t::composite_melee_crit_chance() const
{
  return player_t::composite_melee_crit_chance() + get_player_buff_effects_value( crit_chance_additive_buffeffects, true
);
}

double my_class_t::composite_spell_crit_chance() const
{
  return player_t::composite_spell_crit_chance() + get_player_buff_effects_value( crit_chance_additive_buffeffects, true );
}

double my_class_t::composite_crit_avoidance() const
{
  return player_t::composite_crit_avoidance() + get_player_buff_effects_value( crit_avoidance_additive_buffeffects, true );
}

double my_class_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  return player_t::composite_player_critical_damage_multiplier( s ) * get_player_buff_effects_value( crit_damage_multiplier_buffeffects );
}

double my_class_t::composite_leech() const
{
  return player_t::composite_leech() + get_player_buff_effects_value( leech_additive_buffeffects, true );
}

double my_class_t::composite_melee_expertise( const weapon_t* ) const
{
  return player_t::composite_melee_expertise( nullptr ) + get_player_buff_effects_value( expertise_additive_buffeffects, true );
}

double my_class_t::composite_parry() const
{
  return player_t::composite_parry() + get_player_buff_effects_value( parry_additive_buffeffects, true );
}

double my_class_t::composite_rating_multiplier( rating_e r ) const
{
  double rm = player_t::composite_rating_multiplier( r );

  switch ( r )
  {
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
    case RATING_SPELL_CRIT:
      rm *= get_player_buff_effects_value( crit_rating_multiplier_buffeffects );
      break;
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
    case RATING_SPELL_HASTE:
      rm *= get_player_buff_effects_value( haste_rating_multiplier_buffeffects );
      break;
    case RATING_MASTERY:
      rm *= get_player_buff_effects_value( mastery_rating_multiplier_buffeffects );
      break;
    case RATING_DAMAGE_VERSATILITY:
    case RATING_HEAL_VERSATILITY:
    case RATING_MITIGATION_VERSATILITY:
      rm *= get_player_buff_effects_value( versatility_rating_multiplier_buffeffects );
      break;
    default:
      break;
  }

  return rm;
}

double my_class_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  switch ( attr )
  {
    case ATTR_AGILITY:
      m *= get_player_buff_effects_value( agility_multiplier_buffeffects );
      break;
    case ATTR_INTELLECT:
      m *= get_player_buff_effects_value( intellect_multiplier_buffeffects );
      break;
    case ATTR_STRENGTH:
      m *= get_player_buff_effects_value( strength_multiplier_buffeffects );
      break;
    case ATTR_STAMINA:
      m *= get_player_buff_effects_value( stamina_multiplier_buffeffects );
      break;
    default:
      break;
  }

  return m;
}

double my_class_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= get_player_buff_effects_value( all_damage_multiplier_buffeffects );

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) )
  {
    m *= get_player_buff_effects_value( phys_damage_multiplier_buffeffects );
  }
  if ( dbc::is_school( school, SCHOOL_HOLY ) )
  {
    m *= get_player_buff_effects_value( holy_damage_multiplier_buffeffects );
  }
  if ( dbc::is_school( school, SCHOOL_FIRE ) )
  {
    m *= get_player_buff_effects_value( fire_damage_multiplier_buffeffects );
  }
  if ( dbc::is_school( school, SCHOOL_NATURE ) )
  {
    m *= get_player_buff_effects_value( nature_damage_multiplier_buffeffects );
  }
  if ( dbc::is_school( school, SCHOOL_FROST ) )
  {
    m *= get_player_buff_effects_value( frost_damage_multiplier_buffeffects );
  }
  if ( dbc::is_school( school, SCHOOL_SHADOW ) )
  {
    m *= get_player_buff_effects_value( shadow_damage_multiplier_buffeffects );
  }
  if ( dbc::is_school( school, SCHOOL_ARCANE ) )
  {
    m *= get_player_buff_effects_value( arcane_damage_multiplier_buffeffects );
  }

  return m;
}

double my_class_t::composite_player_pet_damage_multiplier( const action_state_t* state, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( state, guardian );

  if ( guardian )
  {
    m *= get_player_buff_effects_value( guardian_damage_multiplier_buffeffects );
  }
  else
  {
    m *= get_player_buff_effects_value( pet_damage_multiplier_buffeffects );
  }

  return m;
}

double my_class_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  const my_class_td_t* td = get_target_data( target );

  if ( td )
  {
    if( guardian )
    {
      m *= get_debuff_effects_value_from_player( guardian_damage_target_multiplier_dotdebuffs, get_target_data( target ) );
    }
    else
    {
      m *= get_debuff_effects_value_from_player( pet_damage_target_multiplier_dotdebuffs, get_target_data( target ) );
    }
  }

  return m;
}
*/

enum player_value_type_e
{
  DATA_VALUE,
  DEFAULT_VALUE,
  CURRENT_VALUE,
};

template <typename TD>
struct parse_player_buff_effects_t
{
  using bfun = std::function<bool()>;
  struct buff_effect_t
  {
    buff_t* buff;
    double value;
    player_value_type_e type;
    bool use_stacks;
    bool mastery;
    bfun func;
    const spelleffect_data_t& eff;

    buff_effect_t( buff_t* b, double v, player_value_type_e t = DATA_VALUE, bool s = true, bool m = false, bfun f = nullptr,
                   const spelleffect_data_t& e = spelleffect_data_t::nil() )
      : buff( b ), value( v ), type( t ), use_stacks( s ), mastery( m ), func( std::move( f ) ), eff( e )
    {}
  };

  // TODO: add value type to debuffs if it becomes necessary in the future
  using dfun = std::function<int( TD* )>;
  struct dot_debuff_t
  {
    dfun func;
    double value;
    bool mastery;
    const spelleffect_data_t& eff;

    dot_debuff_t( dfun f, double v, bool m = false, const spelleffect_data_t& e = spelleffect_data_t::nil() )
      : func( std::move( f ) ), value( v ), mastery( m ), eff( e )
    {}
  };

private:
  player_t* player_;
  std::vector<std::pair<size_t, double>> effect_flat_modifiers;
  std::vector<std::pair<size_t, double>> effect_pct_modifiers;

public:
  // auto parsed dynamic effects
  // Damage modifiers 
  std::vector<buff_effect_t> pet_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> guardian_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> phys_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> holy_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> fire_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> nature_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> frost_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> shadow_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> arcane_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> all_damage_multiplier_buffeffects;
  std::vector<buff_effect_t> crit_damage_multiplier_buffeffects;
  // Debuff damage modifiers
  std::vector<dot_debuff_t> pet_damage_target_multiplier_dotdebuffs;
  std::vector<dot_debuff_t> guardian_damage_target_multiplier_dotdebuffs;
  // Attribute modifiers
  std::vector<buff_effect_t> strength_multiplier_buffeffects;
  std::vector<buff_effect_t> agility_multiplier_buffeffects;
  std::vector<buff_effect_t> intellect_multiplier_buffeffects;
  std::vector<buff_effect_t> stamina_multiplier_buffeffects;
  std::vector<buff_effect_t> leech_additive_buffeffects;
  std::vector<buff_effect_t> expertise_additive_buffeffects;
  std::vector<buff_effect_t> parry_additive_buffeffects;
  std::vector<buff_effect_t> attack_power_multiplier_buffeffects;
  std::vector<buff_effect_t> all_haste_multiplier_buffeffects;
  std::vector<buff_effect_t> all_attack_speed_multiplier_buffeffects;
  std::vector<buff_effect_t> melee_attack_speed_multiplier_buffeffects;
  std::vector<buff_effect_t> crit_chance_additive_buffeffects;
  std::vector<buff_effect_t> crit_avoidance_additive_buffeffects;
  std::vector<buff_effect_t> base_armor_multiplier_buffeffects;
  std::vector<buff_effect_t> dodge_additive_buffeffects;
  std::vector<buff_effect_t> versatility_additive_buffeffects;
  std::vector<buff_effect_t> mastery_additive_buffeffects;
  std::vector<buff_effect_t> crit_rating_multiplier_buffeffects;
  std::vector<buff_effect_t> haste_rating_multiplier_buffeffects;
  std::vector<buff_effect_t> mastery_rating_multiplier_buffeffects;
  std::vector<buff_effect_t> versatility_rating_multiplier_buffeffects;

  parse_player_buff_effects_t( player_t* p ) : player_( p ) {}
  virtual ~parse_player_buff_effects_t() = default;

  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  template <typename T>
  void parse_spell_effects_mods( double& val, bool& mastery, const spell_data_t* base, size_t idx, T mod )
  {
    bool mod_is_mastery = false;

    if ( mod->effect_count() && mod->flags( SX_MASTERY_AFFECTS_POINTS ) )
    {
      mastery = true;
      mod_is_mastery = true;
    }

    for ( size_t i = 1; i <= mod->effect_count(); i++ )
    {
      const auto& eff = mod->effectN( i );

      if ( eff.type() != E_APPLY_AURA )
        continue;

      if ( ( base->affected_by_all( eff ) &&
             ( ( eff.misc_value1() == P_EFFECT_1 && idx == 1 ) || ( eff.misc_value1() == P_EFFECT_2 && idx == 2 ) ||
               ( eff.misc_value1() == P_EFFECT_3 && idx == 3 ) || ( eff.misc_value1() == P_EFFECT_4 && idx == 4 ) ||
               ( eff.misc_value1() == P_EFFECT_5 && idx == 5 ) ) ) ||
           ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE && eff.trigger_spell_id() == base->id() && idx == 1 ) )
      {
        double pct = mod_is_mastery ? eff.mastery_value() : mod_spell_effects_value( mod, eff );

        if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
          val += pct;
        else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
          val *= 1.0 + pct / 100;
        else if ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE )
          val = pct;
      }
    }
  }

  void parse_spell_effects_mods( double&, bool&, const spell_data_t*, size_t ) {}

  template <typename T, typename... Ts>
  void parse_spell_effects_mods( double& val, bool& m, const spell_data_t* base, size_t idx, T mod, Ts... mods )
  {
    parse_spell_effects_mods( val, m, base, idx, mod );
    parse_spell_effects_mods( val, m, base, idx, mods... );
  }

  // Syntax: parse_player_buff_effects( buff[, ignore_mask|use_stacks[, value_type]][, spell][,...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit, must be typed as unsigned
  //  use_stacks = optional, default true, whether to multiply value by stacks, mutually exclusive with ignore parameters
  //  value_type = optional, default DATA, where the value comes from.
  //               DATA = spell data, DEFAULT = buff default value, CURRENT_VALUE = buff current value
  //  spell = optional list of spell with redirect effects that modify the effects on the buff
  //
  // Example 1: Parse buff1, ignore effects #1 #3 #5, modify by talent1, modify by tier1:
  //  parse_player_buff_effects<S,S>( buff1, 0b10101U, talent1, tier1 );
  //
  // Example 2: Parse buff2, don't multiply by stacks, use the default value set on the buff instead of effect value:
  //  parse_player_buff_effects( buff2, false, DEFAULT );
  template <typename... Ts>
  void parse_player_buff_effect( buff_t* buff, const bfun& f, const spell_data_t* s_data, size_t i, bool use_stacks,
                           player_value_type_e value_type, bool force, Ts... mods )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery    = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val      = ( buff && value_type == DEFAULT_VALUE ) ? ( buff->default_value * 100 )
                                                            : ( mastery ? eff.mastery_value() : eff.base_value() );
    double val_mul  = 0.01;

    // TODO: more robust logic around 'party' buffs with radius
    if ( !( eff.type() == E_APPLY_AURA || eff.type() == E_APPLY_AREA_AURA_PARTY ) || eff.radius() ) return;

    if ( i <= 5 )
      parse_spell_effects_mods( val, mastery, s_data, i, mods... );

    auto debug_message = [ & ]( std::string_view type ) {
      if ( buff )
      {
        std::string val_str;

        if ( value_type == player_value_type_e::CURRENT_VALUE )
          val_str = "current value";
        else if ( value_type == player_value_type_e::DEFAULT_VALUE )
          val_str = fmt::format( "default value ({})", val * val_mul );
        else if ( mastery )
          val_str = fmt::format( "{}*mastery", val * val_mul * 100 );
        else
          val_str = fmt::format( "{}", val * val_mul );

        player_->sim->print_debug( "player-buff-effects: {} {} modified by {} {} buff {} ({}#{})", player_->name(), type, val_str, use_stacks ? "per stack of" : "with", buff->name(),
                                   buff->data().id(), i );
      }
      else if ( mastery && !f )
      {
        player_->sim->print_debug( "player-mastery-effects: {} {} modified by {}*mastery from {} ({}#{})",
                                   player_->name(), type, val * val_mul * 100, s_data->name_cstr(),
                                   s_data->id(), i );
      }
      else if ( f )
      {
        player_->sim->print_debug( "player-conditional-effects: {} {} modified by {} with condition from {} ({}#{})",
                                   player_->name(), type, val * val_mul, s_data->name_cstr(), s_data->id(),
                                   i );
      }
      else
      {
        player_->sim->print_debug( "player-passive-effects: {} {} modified by {} from {} ({}#{})", player_->name(),
                                   type, val * val_mul, s_data->name_cstr(), s_data->id(), i );
      }
    };

    if ( !val )
      return;

    if ( mastery )
      val_mul = 1.0;

    switch ( eff.subtype() )
    {
      case A_MOD_PET_DAMAGE_DONE:
        pet_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff );
        debug_message( "pet damage" );
        break;
      case A_MOD_GUARDIAN_DAMAGE_DONE:
        guardian_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff );
        debug_message( "guardian damage" );
        break;
      case A_MOD_TOTAL_STAT_PERCENTAGE:
        if ( eff.misc_value2() )
        {
          enum stat_mask_e
          {
            STAT_MASK_STRENGTH  = 0x1,
            STAT_MASK_AGILITY   = 0x2,
            STAT_MASK_STAMINA   = 0x4,
            STAT_MASK_INTELLECT = 0x8,
          };
          if ( eff.misc_value2() == 0xb )
          {
            switch ( player_->convert_hybrid_stat( STAT_STR_AGI_INT ) )
            {
              case STAT_STRENGTH:
                strength_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                              eff );
                break;
              case STAT_AGILITY:
                agility_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                             eff );
                break;
              case STAT_INTELLECT:
                intellect_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                               eff );
                break;
              default:
                break;
            }
          }
          else
          {
            if ( ( eff.misc_value2() & STAT_MASK_STRENGTH ) == STAT_MASK_STRENGTH )
            {
              strength_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                            eff );
              debug_message( "strength multiplier" );
            }
            if ( ( eff.misc_value2() & STAT_MASK_AGILITY ) == STAT_MASK_AGILITY )
            {
              agility_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                           eff );
              debug_message( "agility multiplier" );
            }
            if ( ( eff.misc_value2() & STAT_MASK_STAMINA ) == STAT_MASK_STAMINA )
            {
              stamina_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                           eff );
              debug_message( "stamina multiplier" );
            }
            if ( ( eff.misc_value2() & STAT_MASK_INTELLECT ) == STAT_MASK_INTELLECT )
            {
              intellect_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                             eff );
              debug_message( "intellect multiplier" );
            }
          }
        }
        break;
      case A_MOD_LEECH_PERCENT:
        leech_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff );
        debug_message( "leech additive modifier" );
        break;
      case A_MOD_EXPERTISE:
        expertise_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff );
        debug_message( "expertise additive modifier" );
        break;
      case A_MOD_PARRY_PERCENT:
        parry_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff );
        debug_message( "parry additive modifier" );
        break;
      case A_MOD_DAMAGE_PERCENT_DONE:
        if ( eff.misc_value1() )
        {
          if ( eff.misc_value1() == 0x7f )
          {
            all_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                             eff );
            debug_message( "all damage multiplier" );
          }
          else
          {
            if ( ( eff.misc_value1() & SCHOOL_MASK_PHYSICAL ) == SCHOOL_MASK_PHYSICAL )
            {
              phys_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                               eff );
              debug_message( "physical damage multiplier" );
            }
            if ( ( eff.misc_value1() & SCHOOL_MASK_HOLY ) == SCHOOL_MASK_HOLY )
            {
              holy_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                               eff );
              debug_message( "holy damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_FIRE ) == SCHOOL_MASK_FIRE )
            {
              fire_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                               eff );
              debug_message( "fire damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_NATURE ) == SCHOOL_MASK_NATURE )
            {
              nature_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery,
                                                                 f, eff );
              debug_message( "nature damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_FROST ) == SCHOOL_MASK_FROST )
            {
              frost_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                                eff );
              debug_message( "frost damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_SHADOW ) == SCHOOL_MASK_SHADOW )
            {
              shadow_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery,
                                                                 f, eff );
              debug_message( "shadow damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_ARCANE ) == SCHOOL_MASK_ARCANE )
            {
              arcane_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery,
                                                                 f, eff );
              debug_message( "arcane damage multiplier" );
            }
          }
        }
        break;
      case A_MOD_ATTACK_POWER_PCT:
        attack_power_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                        eff );
        debug_message( "attack power multiplier" );
        break;
      case A_HASTE_ALL:
        all_haste_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                          eff );
        debug_message( "all haste multiplier" );
        break;
      case A_MOD_RANGED_AND_MELEE_ATTACK_SPEED:
        all_attack_speed_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                       eff );
        debug_message( "all attack speed multiplier" );
        break;
      case A_MOD_MELEE_ATTACK_SPEED_PCT:
        melee_attack_speed_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                              eff );
        debug_message( "melee attack speed multiplier" );
        break;
      case A_MOD_ALL_CRIT_CHANCE:
        crit_chance_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                          eff );
        debug_message( "crit chance additive" );
        break;
      case A_MOD_ATTACKER_MELEE_CRIT_CHANCE:
        crit_avoidance_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                       eff );
        debug_message( "crit avoidance additive" );
        break;
      case A_MOD_BASE_RESISTANCE_PCT:
        base_armor_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                        eff );
        debug_message( "base resistance" );
        break;
      case A_MOD_DODGE_PERCENT:
        dodge_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                        eff );
        debug_message( "dodge additive" );
        break;
      case A_MOD_VERSATILITY_PCT:
        versatility_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                 eff );
        debug_message( "vers additive" );
        break;
      case A_MOD_MASTERY_PCT:
        mastery_additive_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                       eff );
        debug_message( "mastery additive" );
        break;
      case A_MOD_CRIT_DAMAGE_BONUS:
        crit_damage_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                         eff );
        debug_message( "crit damage multiplier" );
        break;
      case A_MOD_STAT_FROM_RATING_PCT:
        switch ( eff.misc_value1() )
        {
          case 1792:
            crit_rating_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                             eff );
            debug_message( "crit rating multiplier" );
            break;
          case 917504:
            haste_rating_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                              eff );
            debug_message( "haste rating multiplier" );
            break;
          case 33554432:
            mastery_rating_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                                eff );
            debug_message( "mastery rating multiplier" );
            break;
          case 1879048192:
            versatility_rating_multiplier_buffeffects.emplace_back( buff, val * val_mul, value_type, use_stacks,
                                                                    mastery, f, eff );
            debug_message( "versatility rating multiplier" );
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

  template <typename... Ts>
  void parse_player_buff_effects( buff_t* buff, unsigned ignore_mask, bool use_stacks, player_value_type_e value_type, Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* spell = &buff->data();

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_player_buff_effect( buff, nullptr, spell, i, use_stacks, value_type, false, mods... );
    }
  }

  template <typename... Ts>
  void parse_player_buff_effects( buff_t* buff, unsigned ignore_mask, Ts... mods )
  {
    parse_player_buff_effects( buff, ignore_mask, true, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void parse_player_buff_effects( buff_t* buff, bool stack, Ts... mods )
  {
    parse_player_buff_effects( buff, 0U, stack, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void parse_player_buff_effects( buff_t* buff, player_value_type_e value_type, Ts... mods )
  {
    parse_player_buff_effects( buff, 0U, true, value_type, mods... );
  }

  template <typename... Ts>
  void parse_player_buff_effects( buff_t* buff, Ts... mods )
  {
    parse_player_buff_effects( buff, 0U, true, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void force_player_buff_effect( buff_t* buff, unsigned idx, bool stack, player_value_type_e value_type, Ts... mods )
  {
    parse_player_buff_effect( buff, nullptr, &buff->data(), idx, stack, value_type, true, mods... );
  }

  template <typename... Ts>
  void force_player_buff_effect( buff_t* buff, unsigned idx, Ts... mods )
  {
    force_player_buff_effect( buff, idx, true, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void parse_player_conditional_effects( const spell_data_t* spell, const bfun& func, unsigned ignore_mask = 0U,
                                  bool use_stack = true, player_value_type_e value_type = DATA_VALUE, Ts... mods )
  {
    if ( !spell || !spell->ok() )
      return;

    for ( size_t i = 1 ; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_player_buff_effect( nullptr, func, spell, i, use_stack, value_type, false, mods... );
    }
  }

  void force_player_conditional_effect( const spell_data_t* spell, const bfun& func, unsigned idx, bool use_stack = true,
                                 player_value_type_e value_type = DATA_VALUE )
  {
    parse_player_buff_effect( nullptr, func, spell, idx, use_stack, value_type, true );
  }

  void parse_player_passive_effects( const spell_data_t* spell, unsigned ignore_mask = 0U )
  {
    parse_player_conditional_effects( spell, nullptr, ignore_mask );
  }

  void force_player_passive_effect( const spell_data_t* spell, unsigned idx, bool use_stack = true,
                             player_value_type_e value_type = DATA_VALUE )
  {
    parse_player_buff_effect( nullptr, nullptr, spell, idx, use_stack, value_type, true );
  }

  double get_player_buff_effects_value( const std::vector<buff_effect_t>& buffeffects, bool flat = false,
                                 bool benefit = true ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( const auto& i : buffeffects )
    {
      double eff_val = i.value;
      int mod = 1;

      if ( i.func && !i.func() )
          continue;  // continue to next effect if conditional effect function is false

      if ( i.buff )
      {
        auto stack = benefit ? i.buff->stack() : i.buff->check();

        if ( !stack )
          continue;  // continue to next effect if stacks == 0 (buff is down)

        mod = i.use_stacks ? stack : 1;

        if ( i.type == CURRENT_VALUE )
          eff_val = i.buff->check_value();
      }

      if ( i.mastery )
        eff_val *= player_->cache.mastery();

      if ( flat )
        return_value += eff_val * mod;
      else
        return_value *= 1.0 + eff_val * mod;
    }

    return return_value;
  }

  // Syntax: parse_debuff_effects_from_player( func, debuff[, spell][,...] )
  //  func = function taking the class's target_data as argument and returning an integer
  //  debuff = spell data of the debuff
  //  spell = optional list of spells with redirect effects that modify the effects on the debuff
  template <typename... Ts>
  void parse_debuff_effect_from_player( const dfun& func, const spell_data_t* s_data, size_t i, bool use_stacks, bool force, Ts... mods )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery    = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val      = mastery ? eff.mastery_value() : eff.base_value();
    double val_mul  = 0.01;

    if ( eff.type() != E_APPLY_AURA )
      return;

    if ( i <= 5 )
      parse_spell_effects_mods( val, mastery, s_data, i, mods... );

    if ( !val )
      return;

    if ( mastery )
      val_mul = 1.0;

    auto debug_message = [ & ]( std::string_view type ) {
      std::string val_str;
      if ( !mastery )
      {
        val_str = "current value";
        player_->sim->print_debug( "player-debuff-effects: {} {} modified by {} {} debuff {} ({}#{})", player_->name(),
                                   type, val_str, "with", s_data->name_cstr(), s_data->id(), i);
      }
      else if ( mastery )
      {
        val_str = fmt::format( "{}*mastery", val * val_mul * 100 );
        player_->sim->print_debug( "player-mastery-debuff-effects: {} {} modified by {}*mastery from {} ({}#{})",
                                   player_->name(), type, val * val_mul * 100, s_data->name_cstr(), s_data->id(), i );
      }
    };

    switch ( eff.subtype() )
    {
      case A_MOD_DAMAGE_FROM_CASTER_PET:
        pet_damage_target_multiplier_dotdebuffs.emplace_back( func, val * val_mul, mastery, eff );
        debug_message( "pet damage dot/debuff" );
        break;
      case A_MOD_DAMAGE_FROM_CASTER_GUARDIAN:
        guardian_damage_target_multiplier_dotdebuffs.emplace_back( func, val * val_mul, mastery, eff );
        debug_message( "guardian damage dot/debuff" );
        break;
      default:
        break;
    }
  }

  template <typename... Ts>
  void parse_debuff_effects_from_player( const dfun& func, const spell_data_t* spell, Ts... mods )
  {
    if ( !spell->ok() )
      return;

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      parse_debuff_effect_from_player( func, spell, i, true, false, mods... );
    }
  }

  template <typename... Ts>
  void force_debuff_effect_from_player( const dfun& func, const spell_data_t* spell, unsigned idx, Ts... mods )
  {
    parse_debuff_effect_from_player( func, spell, idx, true, true, mods... );
  }

  virtual double get_debuff_effects_value_from_player( const std::vector<dot_debuff_t>& debuffeffects, TD* t ) const
  {
    double return_value = 1.0;

    for ( const auto& i : debuffeffects )
    {
      if ( auto check = i.func( t ) )
      {
        auto eff_val = i.value;

        if ( i.mastery )
          eff_val *= player_->cache.mastery();

        return_value *= 1.0 + eff_val * check;
      }
    }

    return return_value;
  }

  // Syntax: parse_effect_modifiers( modifier[, spell][,...] )
  //  modifier = spell data containing effects that modify effects on the action
  //  spell = optional list of spells with redirect effects that modify the effects of the modifier
  // Syntax: modified_effect_<value|percent>( N )
  //  returns base_value() or percent() of the action data's N-th effect, modified by any previously parsed effects.
  //  Note that this is not a fast accessor and will iterate through all parsed modifiers.
  template <typename... Ts>
  void parse_effect_modifiers( const spell_data_t* s_data, Ts... mods )
  {
    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      const auto& eff = s_data->effectN( i );
      auto subtype = eff.subtype();
      size_t target_idx = 0;

      if ( eff.type() != E_APPLY_AURA )
        continue;
      
      switch ( subtype )
      {
        case A_ADD_FLAT_MODIFIER:
        case A_ADD_FLAT_LABEL_MODIFIER:
        case A_ADD_PCT_MODIFIER:
        case A_ADD_PCT_LABEL_MODIFIER:
          break;
        default:
          continue;
      }

      switch ( eff.property_type() )
      {
        case P_EFFECT_1:
          target_idx = 1;
          break;
        case P_EFFECT_2:
          target_idx = 2;
          break;
        case P_EFFECT_3:
          target_idx = 3;
          break;
        case P_EFFECT_4:
          target_idx = 4;
          break;
        case P_EFFECT_5:
          target_idx = 5;
          break;
        default:
          continue;
      }

      double val = eff.base_value();
      bool m;  // dummy throwaway

      if ( i <= 5 )
        parse_spell_effects_mods( val, m, s_data, i, mods... );

      switch ( subtype )
      {
        case A_ADD_FLAT_MODIFIER:
        case A_ADD_FLAT_LABEL_MODIFIER:
          effect_flat_modifiers.emplace_back( target_idx, val );
          break;
        case A_ADD_PCT_MODIFIER:
        case A_ADD_PCT_LABEL_MODIFIER:
          effect_pct_modifiers.emplace_back( target_idx, val * 0.01 );
          break;
        default:
          break;
      }
    }
  }

  
  // return a copy of the effect with modified value
  spelleffect_data_t modified_effect( size_t idx )
  {
    spelleffect_data_t temp = this->data().effectN( idx );

    for ( auto [ i, v ] : effect_flat_modifiers )
      if ( i == idx  )
        temp._base_value += v;

    for ( auto [ i, v ] : effect_pct_modifiers )
      if ( i == idx  )
        temp._base_value *= 1.0 + v;

    return temp;
  }

  
  void parsed_html_report( report::sc_html_stream& os )
  {
    if ( !total_buffeffects_count() )
      return;
    os << "<h3 class=\"toggle open\">Player Effects</h3>\n"
       << "<div class=\"toggle-content\">\n";

    os << "<h4>Affected By (Dynamic)</h4>\n"
       << "<table class=\"details nowrap\" style=\"width:min-content\">\n";

    os << "<tr>\n"
       << "<th class=\"small\">Type</th>\n"
       << "<th class=\"small\">Spell</th>\n"
       << "<th class=\"small\">ID</th>\n"
       << "<th class=\"small\">#</th>\n"
       << "<th class=\"small\">Value</th>\n"
       << "<th class=\"small\">Source</th>\n"
       << "<th class=\"small\">Notes</th>\n"
       << "</tr>\n";

    print_parsed_type( os, pet_damage_multiplier_buffeffects, "Pet Damage Modifier" );
    print_parsed_type( os, guardian_damage_multiplier_buffeffects, "Guardian Damage Modifier" );
    print_parsed_type( os, strength_multiplier_buffeffects, "Strength Modifier" );
    print_parsed_type( os, agility_multiplier_buffeffects, "Agility Modifier" );
    print_parsed_type( os, stamina_multiplier_buffeffects, "Stamina Modifier" );
    print_parsed_type( os, intellect_multiplier_buffeffects, "Intellect Modifier" );
    print_parsed_type( os, leech_additive_buffeffects, "Leech Modifier" );
    print_parsed_type( os, expertise_additive_buffeffects, "Expertise Modifier" );
    print_parsed_type( os, parry_additive_buffeffects, "Parry Modifier" );
    print_parsed_type( os, crit_chance_additive_buffeffects, "Crit Chance Modifier" );
    print_parsed_type( os, crit_avoidance_additive_buffeffects, "Crit Avoidance Modifier" );
    print_parsed_type( os, all_haste_multiplier_buffeffects, "Haste Modifier" );
    print_parsed_type( os, all_attack_speed_multiplier_buffeffects, "All Attack Speed Modifier" );
    print_parsed_type( os, melee_attack_speed_multiplier_buffeffects, "Melee Attack Speed Modifier" );
    print_parsed_type( os, attack_power_multiplier_buffeffects, "Attack Power Modifier" );
    print_parsed_type( os, phys_damage_multiplier_buffeffects, "Physical Damage Increase" );
    print_parsed_type( os, holy_damage_multiplier_buffeffects, "Holy Damage Increase" );
    print_parsed_type( os, fire_damage_multiplier_buffeffects, "Fire Damage Increase" );
    print_parsed_type( os, nature_damage_multiplier_buffeffects, "Nature Damage Increase" );
    print_parsed_type( os, frost_damage_multiplier_buffeffects, "Frost Damage Increase" );
    print_parsed_type( os, shadow_damage_multiplier_buffeffects, "Shadow Damage Increase" );
    print_parsed_type( os, arcane_damage_multiplier_buffeffects, "Arcane Damage Increase" );
    print_parsed_type( os, all_damage_multiplier_buffeffects, "All Damage Increase" );
    print_parsed_type( os, pet_damage_target_multiplier_dotdebuffs, "Pet Damage Debuff" );
    print_parsed_type( os, guardian_damage_target_multiplier_dotdebuffs, "Guardian Damage Debuff" );
    print_parsed_type( os, base_armor_multiplier_buffeffects, "Base Resistance Modifier" );
    print_parsed_type( os, dodge_additive_buffeffects, "Dodge Modifier" );
    print_parsed_type( os, versatility_additive_buffeffects, "Versatility Modifier" );
    print_parsed_type( os, mastery_additive_buffeffects, "Mastery Modifier" );
    print_parsed_type( os, crit_damage_multiplier_buffeffects, "Crit Damage Modifier" );
    print_parsed_type( os, crit_rating_multiplier_buffeffects, "Crit Rating Modifier" );
    print_parsed_type( os, haste_rating_multiplier_buffeffects, "Haste Rating Modifier" );
    print_parsed_type( os, mastery_rating_multiplier_buffeffects, "Mastery Rating Modifier" );
    print_parsed_type( os, versatility_rating_multiplier_buffeffects, "Versatility Rating Modifier" );
    print_parsed_custom_type( os );

    os << "</table>\n"
       << "</div>\n";
  }

  virtual size_t total_buffeffects_count()
  {
    return  pet_damage_multiplier_buffeffects.size() +
      guardian_damage_multiplier_buffeffects.size() +
      strength_multiplier_buffeffects.size() +
      agility_multiplier_buffeffects.size() +
      intellect_multiplier_buffeffects.size() +
      stamina_multiplier_buffeffects.size() +
      leech_additive_buffeffects.size() +
      expertise_additive_buffeffects.size() +
      parry_additive_buffeffects.size() +
      phys_damage_multiplier_buffeffects.size() +
      holy_damage_multiplier_buffeffects.size() +
      fire_damage_multiplier_buffeffects.size() +
      nature_damage_multiplier_buffeffects.size() +
      frost_damage_multiplier_buffeffects.size() +
      shadow_damage_multiplier_buffeffects.size() +
      arcane_damage_multiplier_buffeffects.size() +
      all_damage_multiplier_buffeffects.size() +
      attack_power_multiplier_buffeffects.size() +
      all_haste_multiplier_buffeffects.size() +
      all_attack_speed_multiplier_buffeffects.size() +
      melee_attack_speed_multiplier_buffeffects.size() +
      crit_chance_additive_buffeffects.size() +
      crit_avoidance_additive_buffeffects.size() +
      pet_damage_target_multiplier_dotdebuffs.size() +
      guardian_damage_target_multiplier_dotdebuffs.size() +
      dodge_additive_buffeffects.size() +
      versatility_additive_buffeffects.size() +
      mastery_additive_buffeffects.size() +
      crit_damage_multiplier_buffeffects.size() +
      crit_rating_multiplier_buffeffects.size() +
      haste_rating_multiplier_buffeffects.size() +
      mastery_rating_multiplier_buffeffects.size() +
      versatility_rating_multiplier_buffeffects.size() +
      base_armor_multiplier_buffeffects.size();
  }

  virtual void print_parsed_custom_type( report::sc_html_stream& ) {}

  template <typename V>
  void print_parsed_type( report::sc_html_stream& os, const V& entries, std::string_view n )
  {
    auto c = entries.size();
    if ( !c )
      return;

    os.format( "<tr><td class=\"label\" rowspan=\"{}\">{}</td>\n", c, n );

    for ( size_t i = 0; i < c; i++ )
    {
      if ( i > 0 )
        os << "<tr>";

      print_parsed_line( os, entries[ i ] );
    }
  }

  std::string value_type_name( player_value_type_e t )
  {
    switch ( t )
    {
      case DEFAULT_VALUE: return "Default Value";
      case CURRENT_VALUE: return "Current Value";
      default:          return "Spell Data";
    }
  }

  void print_parsed_line( report::sc_html_stream& os, const buff_effect_t& entry )
  {
    std::vector<std::string> notes;

    if ( !entry.use_stacks )
      notes.push_back( "No-stacks" );

    if ( entry.mastery )
      notes.push_back( "Mastery" );

    if ( entry.func )
      notes.push_back( "Conditional" );

    os.format( "<td>{}</td><td>{}</td><td>{}</td><td>{:.3f}</td><td>{}</td><td>{}</td></tr>\n",
               entry.eff.spell()->name_cstr(),
               entry.eff.spell()->id(),
               entry.eff.index() + 1,
               entry.value * ( entry.mastery ? 100 : 1 ),
               value_type_name( entry.type ),
               util::string_join( notes ) );
  }

  void print_parsed_line( report::sc_html_stream& os, const dot_debuff_t& entry )
  {
    os.format( "<td>{}</td><td>{}</td><td>{}</td><td>{:.3f}</td><td>{}</td><td>{}</td></tr>\n",
               entry.eff.spell()->name_cstr(),
               entry.eff.spell()->id(),
               entry.eff.index() + 1,
               entry.value * ( entry.mastery ? 100 : 1 ),
               "",
               entry.mastery ? "Mastery" : "" );
  }
};
