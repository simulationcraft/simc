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

// Mixin to player class to allow auto parsing and dynamic application of player wide buffs & auras.
enum player_value_type_e
{
  DATA_VALUE,
  DEFAULT_VALUE,
  CURRENT_VALUE,
};

template <typename Player>
struct parse_aura_effects_t
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
    cache_e cache;

    buff_effect_t( buff_t* b, double v, player_value_type_e t = DATA_VALUE, bool s = true, bool m = false, bfun f = nullptr,
                   const spelleffect_data_t& e = spelleffect_data_t::nil(), cache_e c = CACHE_NONE )
      : buff( b ), value( v ), type( t ), use_stacks( s ), mastery( m ), func( std::move( f ) ), eff( e ), cache( c )
    {
      if( b )
      {
        b->add_invalidate( c );
      }
    }
  };

private:
  player_t* p;
  std::vector<std::pair<size_t, double>> effect_flat_modifiers;
  std::vector<std::pair<size_t, double>> effect_pct_modifiers;

public:
  // auto parsed dynamic effects
  // Damage modifiers 
  std::vector<buff_effect_t> pet_damage_multiplier_auras;
  std::vector<buff_effect_t> guardian_damage_multiplier_auras;
  std::vector<buff_effect_t> phys_damage_multiplier_auras;
  std::vector<buff_effect_t> holy_damage_multiplier_auras;
  std::vector<buff_effect_t> fire_damage_multiplier_auras;
  std::vector<buff_effect_t> nature_damage_multiplier_auras;
  std::vector<buff_effect_t> frost_damage_multiplier_auras;
  std::vector<buff_effect_t> shadow_damage_multiplier_auras;
  std::vector<buff_effect_t> arcane_damage_multiplier_auras;
  std::vector<buff_effect_t> all_damage_multiplier_auras;
  std::vector<buff_effect_t> crit_damage_multiplier_auras;
  std::vector<buff_effect_t> auto_attack_damage_multiplier_auras;
  // Attribute modifiers
  std::vector<buff_effect_t> strength_multiplier_auras;
  std::vector<buff_effect_t> agility_multiplier_auras;
  std::vector<buff_effect_t> intellect_multiplier_auras;
  std::vector<buff_effect_t> stamina_multiplier_auras;
  std::vector<buff_effect_t> leech_additive_auras;
  std::vector<buff_effect_t> expertise_additive_auras;
  std::vector<buff_effect_t> parry_additive_auras;
  std::vector<buff_effect_t> attack_power_multiplier_auras;
  std::vector<buff_effect_t> all_haste_multiplier_auras;
  std::vector<buff_effect_t> all_attack_speed_multiplier_auras;
  std::vector<buff_effect_t> melee_attack_speed_multiplier_auras;
  std::vector<buff_effect_t> spell_speed_multiplier_auras;
  std::vector<buff_effect_t> crit_chance_additive_auras;
  std::vector<buff_effect_t> spell_crit_additive_auras;
  std::vector<buff_effect_t> crit_avoidance_additive_auras;
  std::vector<buff_effect_t> base_armor_multiplier_auras;
  std::vector<buff_effect_t> dodge_additive_auras;
  std::vector<buff_effect_t> versatility_additive_auras;
  std::vector<buff_effect_t> mastery_additive_auras;
  std::vector<buff_effect_t> crit_rating_multiplier_auras;
  std::vector<buff_effect_t> haste_rating_multiplier_auras;
  std::vector<buff_effect_t> mastery_rating_multiplier_auras;
  std::vector<buff_effect_t> versatility_rating_multiplier_auras;

  parse_aura_effects_t( player_t* p ) : p( p ){}
  virtual ~parse_aura_effects_t() = default;

  player_t* player() const
  {
    return debug_cast<player_t*>( p );
  }

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

  // Syntax: apply_buff_aura_effects( buff[, ignore_mask|use_stacks[, value_type]][, spell][,...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit, must be typed as unsigned
  //  use_stacks = optional, default true, whether to multiply value by stacks, mutually exclusive with ignore parameters
  //  value_type = optional, default DATA, where the value comes from.
  //               DATA = spell data, DEFAULT = buff default value, CURRENT_VALUE = buff current value
  //  spell = optional list of spell with redirect effects that modify the effects on the buff
  //
  // Example 1: Parse buff1, ignore effects #1 #3 #5, modify by talent1, modify by tier1:
  //  apply_buff_aura_effects<S,S>( buff1, 0b10101U, talent1, tier1 );
  //
  // Example 2: Parse buff2, don't multiply by stacks, use the default value set on the buff instead of effect value:
  //  apply_buff_aura_effects( buff2, false, DEFAULT );
  template <typename... Ts>
  void apply_buff_aura_effect( buff_t* buff, const bfun& f, const spell_data_t* s_data, size_t i, bool use_stacks,
                           player_value_type_e value_type, bool force, Ts... mods )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery    = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val      = ( buff && value_type == DEFAULT_VALUE ) ? ( buff->default_value * 100 )
                                                            : ( mastery ? eff.mastery_value() : eff.base_value() );
    double val_mul  = eff.subtype() == A_MOD_MASTERY_PCT ? 1 : 0.01;

    // TODO: more robust logic around 'party' buffs with radius
    if ( !( eff.type() == E_APPLY_AURA || eff.type() == E_APPLY_AREA_AURA_PARTY || eff.type() == E_APPLY_AURA_PLAYER_AND_PET || eff.type() == E_APPLY_AURA_PET ) ) return;

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

        player()->sim->print_debug("player-buff-effects: {} {} modified by {} {} buff {} ({}#{})", player()->name(), type, val_str, use_stacks ? "per stack of" : "with", buff->name(),
                                   buff->data().id(), i );
      }
      else if ( mastery && !f )
      {
        player()->sim->print_debug( "player-mastery-effects: {} {} modified by {}*mastery from {} ({}#{})",
                               player()->name(), type, val * val_mul * 100, s_data->name_cstr(),
                                   s_data->id(), i );
      }
      else if ( f )
      {
        player()->sim->print_debug( "player-conditional-effects: {} {} modified by {} with condition from {} ({}#{})",
                               player()->name(), type, val * val_mul, s_data->name_cstr(), s_data->id(),
                                   i );
      }
      else
      {
        player()->sim->print_debug( "player-passive-effects: {} {} modified by {} from {} ({}#{})", player()->name(),
                                   type, val * val_mul, s_data->name_cstr(), s_data->id(), i );
      }
    };

    if ( !val )
      return;

    if ( mastery )
      val_mul = 1.0;

    cache_e cache = CACHE_NONE;

    switch ( eff.subtype() )
    {
      case A_MOD_PET_DAMAGE_DONE:
        pet_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff, cache );
        debug_message( "pet damage" );
        break;
      case A_MOD_GUARDIAN_DAMAGE_DONE:
        guardian_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff, cache );
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
            STAT_MASK_PRIMARY   = 0xb,
          };
          if ( eff.misc_value2() == STAT_MASK_PRIMARY )
          {
            switch ( player()->convert_hybrid_stat( STAT_STR_AGI_INT ) )
            {
              case STAT_STRENGTH:
                cache = CACHE_STRENGTH;
                strength_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff,
                                                        cache );
                break;
              case STAT_AGILITY:
                cache = CACHE_AGILITY;
                agility_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff,
                                                       cache );
                break;
              case STAT_INTELLECT:
                cache = CACHE_INTELLECT;
                intellect_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff,
                                                         cache );
                break;
              default:
                break;
            }
            debug_message( "primary stat multiplier" );
          }
          else
          {
            if ( ( eff.misc_value2() & STAT_MASK_STRENGTH ) == STAT_MASK_STRENGTH )
            {
              cache = CACHE_STRENGTH;
              strength_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                            eff, cache );
              debug_message( "strength multiplier" );
            }
            if ( ( eff.misc_value2() & STAT_MASK_AGILITY ) == STAT_MASK_AGILITY )
            {
              cache = CACHE_AGILITY;
              agility_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                           eff, cache );
              debug_message( "agility multiplier" );
            }
            if ( ( eff.misc_value2() & STAT_MASK_STAMINA ) == STAT_MASK_STAMINA )
            {
              stamina_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                           eff, cache );
              cache = CACHE_STAMINA;
              debug_message( "stamina multiplier" );
            }
            if ( ( eff.misc_value2() & STAT_MASK_INTELLECT ) == STAT_MASK_INTELLECT )
            {
              cache = CACHE_INTELLECT;
              intellect_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                             eff, cache );
              debug_message( "intellect multiplier" );
            }
          }
        }
        break;
      case A_MOD_LEECH_PERCENT:
        cache = CACHE_LEECH;
        leech_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff, cache );
        debug_message( "leech additive modifier" );
        break;
      case A_MOD_EXPERTISE:
        cache = CACHE_ATTACK_EXP;
        expertise_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff, cache );
        debug_message( "expertise additive modifier" );
        break;
      case A_MOD_PARRY_PERCENT:
        cache = CACHE_PARRY;
        parry_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff, cache );
        debug_message( "parry additive modifier" );
        break;
      case A_MOD_DAMAGE_PERCENT_DONE:
        if ( eff.misc_value1() )
        {
          enum school_mask_e
          {
            SCHOOL_MASK_ALL = 0x7f,
          };
          cache = CACHE_PLAYER_DAMAGE_MULTIPLIER;
          if ( eff.misc_value1() == SCHOOL_MASK_ALL )
          {
            all_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                             eff, cache );
            debug_message( "all damage multiplier" );
          }
          else
          {
            if ( ( eff.misc_value1() & SCHOOL_MASK_PHYSICAL ) == SCHOOL_MASK_PHYSICAL )
            {
              phys_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                               eff, cache );
              debug_message( "physical damage multiplier" );
            }
            if ( ( eff.misc_value1() & SCHOOL_MASK_HOLY ) == SCHOOL_MASK_HOLY )
            {
              holy_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                               eff, cache );
              debug_message( "holy damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_FIRE ) == SCHOOL_MASK_FIRE )
            {
              fire_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                               eff, cache );
              debug_message( "fire damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_NATURE ) == SCHOOL_MASK_NATURE )
            {
              nature_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery,
                                                                 f, eff, cache );
              debug_message( "nature damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_FROST ) == SCHOOL_MASK_FROST )
            {
              frost_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                                eff, cache );
              debug_message( "frost damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_SHADOW ) == SCHOOL_MASK_SHADOW )
            {
              shadow_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery,
                                                                 f, eff, cache );
              debug_message( "shadow damage multiplier" );
            }

            if ( ( eff.misc_value1() & SCHOOL_MASK_ARCANE ) == SCHOOL_MASK_ARCANE )
            {
              arcane_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery,
                                                                 f, eff, cache );
              debug_message( "arcane damage multiplier" );
            }
          }
        }
        break;
      case A_MOD_ATTACK_POWER_PCT:
        cache = CACHE_ATTACK_POWER;
        attack_power_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                        eff, cache );
        debug_message( "attack power multiplier" );
        break;
      case A_HASTE_ALL:
        cache = CACHE_HASTE;
        all_haste_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                          eff, cache );
        debug_message( "all haste multiplier" );
        break;
      case A_MOD_RANGED_AND_MELEE_ATTACK_SPEED:
        cache = CACHE_ATTACK_SPEED;
        all_attack_speed_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                       eff, cache );
        debug_message( "all attack speed multiplier" );
        break;
      case A_MOD_MELEE_ATTACK_SPEED_PCT:
        cache = CACHE_ATTACK_SPEED;
        melee_attack_speed_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                              eff, cache );
        debug_message( "melee attack speed multiplier" );
        break;
      case A_MOD_ALL_CRIT_CHANCE:
        cache = CACHE_CRIT_CHANCE;
        crit_chance_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                          eff, cache );
        debug_message( "crit chance additive" );
        break;
      case A_MOD_ATTACKER_MELEE_CRIT_CHANCE:
        cache = CACHE_ATTACK_CRIT_CHANCE;
        crit_avoidance_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                       eff, cache );
        debug_message( "crit avoidance additive" );
        break;
      case A_MOD_BASE_RESISTANCE_PCT:
        cache = CACHE_BONUS_ARMOR;
        base_armor_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                        eff, cache );
        debug_message( "base resistance" );
        break;
      case A_MOD_DODGE_PERCENT:
        cache = CACHE_DODGE;
        dodge_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                        eff, cache );
        debug_message( "dodge additive" );
        break;
      case A_MOD_VERSATILITY_PCT:
        cache = CACHE_VERSATILITY;
        versatility_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                 eff, cache );
        debug_message( "vers additive" );
        break;
      case A_MOD_MASTERY_PCT:
        cache = CACHE_MASTERY;
        mastery_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                       eff, cache );
        debug_message( "mastery additive" );
        break;
      case A_MOD_CRIT_DAMAGE_BONUS:
        cache = CACHE_PLAYER_DAMAGE_MULTIPLIER;
        crit_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                         eff, cache );
        debug_message( "crit damage multiplier" );
        break;
      case A_MOD_STAT_FROM_RATING_PCT:
        switch ( eff.misc_value1() )
        {
          case 1792:
            cache = CACHE_CRIT_CHANCE;
            crit_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                             eff, cache );
            debug_message( "crit rating multiplier" );
            break;
          case 917504:
            cache = CACHE_HASTE;
            haste_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                              eff, cache );
            debug_message( "haste rating multiplier" );
            break;
          case 33554432:
            cache = CACHE_MASTERY;
            mastery_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                                eff, cache );
            debug_message( "mastery rating multiplier" );
            break;
          case 1879048192:
            cache = CACHE_VERSATILITY;
            versatility_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks,
                                                                    mastery, f, eff, cache );
            debug_message( "versatility rating multiplier" );
            break;
          case 1913521920:
            cache = CACHE_MAX;
            crit_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff, cache );
            haste_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f, eff, cache );
            mastery_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                          eff, cache );
            versatility_rating_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                              eff, cache );
            debug_message( "all secondary rating multiplier" );
          default:
            break;
        }
        break;
      case A_MOD_ATTACKSPEED:
        cache = CACHE_ATTACK_SPEED;
        melee_attack_speed_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                          eff, cache );
        debug_message( "melee attack speed multiplier" );
        break;
      case A_MOD_CASTING_SPEED_NOT_STACK:
        cache = CACHE_SPELL_SPEED;
        spell_speed_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                   eff, cache );
        debug_message( "spell speed multiplier" );
        break;
      case A_MOD_SPELL_CRIT_CHANCE:
        cache = CACHE_SPELL_CRIT_CHANCE;
        spell_crit_additive_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                  eff, cache );
        debug_message( "spell crit additive" );
        break;
      case A_MOD_AUTO_ATTACK_PCT:
        auto_attack_damage_multiplier_auras.emplace_back( buff, val * val_mul, value_type, use_stacks, mastery, f,
                                                eff, cache );
        debug_message( "auto attack damage multiplier" );
        break;
      default:
        break;
    }
  }

  template <typename... Ts>
  void apply_buff_aura_effects( buff_t* buff, unsigned ignore_mask, bool use_stacks, player_value_type_e value_type, Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* spell = &buff->data();

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      apply_buff_aura_effect( buff, nullptr, spell, i, use_stacks, value_type, false, mods... );
    }
  }

  template <typename... Ts>
  void apply_buff_aura_effects( buff_t* buff, unsigned ignore_mask, Ts... mods )
  {
    apply_buff_aura_effects( buff, ignore_mask, true, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void apply_buff_aura_effects( buff_t* buff, bool stack, Ts... mods )
  {
    apply_buff_aura_effects( buff, 0U, stack, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void apply_buff_aura_effects( buff_t* buff, player_value_type_e value_type, Ts... mods )
  {
    apply_buff_aura_effects( buff, 0U, true, value_type, mods... );
  }

  template <typename... Ts>
  void apply_buff_aura_effects( buff_t* buff, Ts... mods )
  {
    apply_buff_aura_effects( buff, 0U, true, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void force_buff_aura_effect( buff_t* buff, unsigned idx, bool stack, player_value_type_e value_type, Ts... mods )
  {
    apply_buff_aura_effect( buff, nullptr, &buff->data(), idx, stack, value_type, true, mods... );
  }

  template <typename... Ts>
  void force_buff_aura_effect( buff_t* buff, unsigned idx, Ts... mods )
  {
    force_buff_aura_effect( buff, idx, true, DATA_VALUE, mods... );
  }

  template <typename... Ts>
  void apply_conditional_buff_aura_effects( const spell_data_t* spell, const bfun& func, unsigned ignore_mask = 0U,
                                  bool use_stack = true, player_value_type_e value_type = DATA_VALUE, Ts... mods )
  {
    if ( !spell || !spell->ok() )
      return;

    for ( size_t i = 1 ; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      apply_buff_aura_effect( nullptr, func, spell, i, use_stack, value_type, false, mods... );
    }
  }

  void force_conditional_buff_aura_effect( const spell_data_t* spell, const bfun& func, unsigned idx, bool use_stack = true,
                                 player_value_type_e value_type = DATA_VALUE )
  {
    apply_buff_aura_effect( nullptr, func, spell, idx, use_stack, value_type, true );
  }

  /* Will automatically apply auras: 9, 47, 49, 57, 65, 79, 137, 142, 163, 166, 187,
193, 240, 290, 318, 319, 342, 405, 429, 443, 471, and 531
These are only player aura buffs, things that modify all damage, an attribute, etc.
Will not allow automatic parsing of any other auras, or whitelisted effects! */
  void apply_passive_aura_effects( const spell_data_t* spell, unsigned ignore_mask = 0U )
  {
    apply_conditional_buff_aura_effects( spell, nullptr, ignore_mask );
  }

  void force_passive_aura_effect( const spell_data_t* spell, unsigned idx, bool use_stack = true,
                             player_value_type_e value_type = DATA_VALUE )
  {
    apply_buff_aura_effect( nullptr, nullptr, spell, idx, use_stack, value_type, true );
  }

  double get_aura_effects_value( const std::vector<buff_effect_t>& auras, bool flat = false,
                                 bool benefit = true ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( const auto& i : auras )
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
        eff_val *= player()->cache.mastery();

      if ( flat )
        return_value += eff_val * mod;
      else
        return_value *= 1.0 + eff_val * mod;
    }

    return return_value;
  }
};
