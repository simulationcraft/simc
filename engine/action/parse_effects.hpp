// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "player/player.hpp"
#include "player/pet.hpp"
#include "sim/sim.hpp"
#include "util/io.hpp"

#include <functional>

// Mixin to action base class to allow auto parsing and dynamic application of whitelist based buffs & auras.
//
// 1) Add `parse_action_effects_t` as an additional parent with the player class and target data class as template
// parameter:
//
//    struct my_action_base_t : public action_t, parse_action_effects_t<my_player_t, my_target_data_t>
//
// 2) Construct the mixin with a pointer to the player and pointer to the action as arguments:
//
//    parse_action_effects_t( *player, this );
//
// 3) PARSE_BUFF_EFFECTS_SETUP macro can be used to add the necessary overrides to the action class. If custom overrides
// are needed, individual methods can be left out and added manually.
//
//    a) #undef and #define the corresponding PARSE_BUFF_EFFECTS_SETUP_X macro for any custom overrides.
//    b) #define PARSE_BUFF_EFFECTS_SETUP_BASE to the parent class of the action class.
//    c) Call the PARSE_BUFF_EFFECTS_SETUP macro.
//    d) For example, if your action is based on spell_t, and you have custom cost() and composite_ta_multiplier():
//
//       #undef PARSE_BUFF_EFFECTS_SETUP_COST
//       #define PARSE_BUFF_EFFECTS_SETUP_COST
//       double cost() const override
//       {
//         auto c = spell_t::cost() + get_buff_effects_value( flat_cost_effects, true, false );
//         ...CUSTOM FLAT ADD CODE...
//
//         c *= get_buff_effects_value( cost_effects, false, false );
//         ...CUSTOM MULTIPLIER CODE...
//
//         return std::max( 0.0, c );
//       }
//
//       #undef PARSE_BUFF_EFFECTS_SETUP_TA_MULTIPLIER
//       #define PARSE_BUFF_EFFECTS_SETUP_TA_MULTIPLIER
//       double composite_ta_multiplier( const action_state_t* s ) const override
//       {
//         auto ta = spell_t::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_effects );
//         ...CUSTOM MULTIPLIER CODE...
//
//         return ta;
//       }
//
//       #define PARSE_BUFF_EFFECTS_SETUP_BASE spell_t
//       PARSE_BUFF_EFFECTS_SETUP

#define PARSE_BUFF_EFFECTS_SETUP_COST \
  double cost() const override \
  { \
    return std::max( 0.0, ( PARSE_BUFF_EFFECTS_SETUP_BASE::cost() + \
                            get_buff_effects_value( flat_cost_effects, true, false ) ) * \
                          get_buff_effects_value( cost_effects, false, false ) ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_TA_MULTIPLIER \
  double composite_ta_multiplier( const action_state_t* s ) const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::composite_ta_multiplier( s ) * \
           get_buff_effects_value( ta_multiplier_effects ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_DA_MULTIPLIER \
  double composite_da_multiplier( const action_state_t* s ) const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::composite_da_multiplier( s ) * \
           get_buff_effects_value( da_multiplier_effects ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_CRIT_CHANCE \
  double composite_crit_chance() const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::composite_crit_chance() + \
           get_buff_effects_value( crit_chance_effects, true ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_EXECUTE_TIME \
  timespan_t execute_time() const override \
  { \
    return std::max( 0_ms, PARSE_BUFF_EFFECTS_SETUP_BASE::execute_time() * \
                           get_buff_effects_value( execute_time_effects ) ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_DOT_DURATION \
  timespan_t composite_dot_duration( const action_state_t* s ) const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::composite_dot_duration( s ) * \
           get_buff_effects_value( dot_duration_effects ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_TICK_TIME \
  timespan_t tick_time( const action_state_t* s ) const override \
  { \
    return std::max( 1_ms, PARSE_BUFF_EFFECTS_SETUP_BASE::tick_time( s ) * \
                           get_buff_effects_value( tick_time_effects ) ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_COOLDOWN_DURATION \
  timespan_t cooldown_duration() const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::cooldown_duration() * \
           get_buff_effects_value( recharge_multiplier_effects ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_RECHARGE_MULTIPLIER \
  double recharge_multiplier( const cooldown_t& cd ) const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::recharge_multiplier( cd ) * \
           get_buff_effects_value( recharge_multiplier_effects ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_TARGET_MULTIPLIER \
  double composite_target_multiplier( player_t* t ) const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::composite_target_multiplier( t ) * \
           get_debuff_effects_value( target_multiplier_effects, parse_effects_target_data( t ) ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_TARGET_CRIT_CHANCE \
  double composite_target_crit_chance( player_t* t ) const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::composite_target_crit_chance( t ) + \
           get_debuff_effects_value( target_crit_chance_effects, parse_effects_target_data( t ), true ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_TARGET_CRIT_DAMAGE_BONUS_MULTIPLIER \
  double composite_target_crit_damage_bonus_multiplier( player_t* t ) const override \
  { \
    return PARSE_BUFF_EFFECTS_SETUP_BASE::composite_target_crit_damage_bonus_multiplier( t ) * \
           get_debuff_effects_value( target_crit_damage_effects, parse_effects_target_data( t ) ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP_HTML_CUSTOMSECTION \
  void html_customsection( report::sc_html_stream& os ) override \
  { \
    parsed_html_report( os ); \
  }

#define PARSE_BUFF_EFFECTS_SETUP \
  PARSE_BUFF_EFFECTS_SETUP_COST \
  PARSE_BUFF_EFFECTS_SETUP_TA_MULTIPLIER \
  PARSE_BUFF_EFFECTS_SETUP_DA_MULTIPLIER \
  PARSE_BUFF_EFFECTS_SETUP_CRIT_CHANCE \
  PARSE_BUFF_EFFECTS_SETUP_EXECUTE_TIME \
  PARSE_BUFF_EFFECTS_SETUP_DOT_DURATION \
  PARSE_BUFF_EFFECTS_SETUP_TICK_TIME \
  PARSE_BUFF_EFFECTS_SETUP_COOLDOWN_DURATION \
  PARSE_BUFF_EFFECTS_SETUP_RECHARGE_MULTIPLIER \
  PARSE_BUFF_EFFECTS_SETUP_TARGET_MULTIPLIER \
  PARSE_BUFF_EFFECTS_SETUP_TARGET_CRIT_CHANCE \
  PARSE_BUFF_EFFECTS_SETUP_TARGET_CRIT_DAMAGE_BONUS_MULTIPLIER \
  PARSE_BUFF_EFFECTS_SETUP_HTML_CUSTOMSECTION

enum value_type_e
{
  USE_DATA,
  USE_DEFAULT,
  USE_CURRENT
};

struct action_effect_t
{
  buff_t* buff;                   // nullptr
  double value;                   // 0.0
  value_type_e type;              // USE_DATA
  bool use_stacks;                // true
  bool mastery;                   // false
  std::function<bool()> func;     // nullptr
  const spelleffect_data_t* eff;  // &spelleffect_data_t::nil()

  action_effect_t( buff_t* b = nullptr, double v = 0.0, value_type_e t = USE_DATA, bool s = true, bool m = false,
                  std::function<bool()> f = nullptr, const spelleffect_data_t* e = &spelleffect_data_t::nil() )
    : buff( b ), value( v ), type( t ), use_stacks( s ), mastery( m ), func( std::move( f ) ), eff( e )
  {}
};

// TODO: add value type to debuffs if it becomes necessary in the future
template <typename TD>
struct target_effect_t
{
  std::function<int( TD* )> func;  // nullptr
  double value;                    // 0.0
  bool mastery;                    // false
  const spelleffect_data_t* eff;   // &spelleffect_data_t::nil()

  target_effect_t( std::function<int( TD* )> f = nullptr, double v = 0.0, bool m = false,
                const spelleffect_data_t* e = &spelleffect_data_t::nil() )
    : func( std::move( f ) ), value( v ), mastery( m ), eff( e )
  {}
};

// used to store values from parameter pack recursion of parse_effect/parse_debuff_effects
template <typename T, typename = std::enable_if_t<std::is_default_constructible_v<T>>>
struct pack_t
{
  T data;
  std::vector<const spell_data_t*> list;
  unsigned mask = 0U;
};

struct parse_effects_t
{
  parse_effects_t() = default;
  virtual ~parse_effects_t() = default;

  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  template <typename T>
  void apply_affecting_mod( double& val, bool& mastery, const spell_data_t* base, size_t idx, T mod )
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

  template <typename U>
  void apply_affecting_mods( const pack_t<U>& tmp, double& val, bool& mastery, const spell_data_t* base, size_t idx )
  {
    // Apply effect modifying effects from mod list. Blizz only currently supports modifying effects 1-5
    if ( idx > 5 )
      return;

    for ( size_t j = 0; j < tmp.list.size(); j++ )
      apply_affecting_mod( val, mastery, base, idx, tmp.list[ j ] );
  }

  template <typename U>
  void parse_spell_effect_mods( pack_t<U>& ) {}

  template <typename U, typename T>
  void parse_spell_effect_mods( pack_t<U>& tmp, T mod )
  {
    if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), T> )
      tmp.list.push_back( mod );
    else if constexpr ( std::is_invocable_v<T> )
      tmp.data.func = std::move( mod );
    else if constexpr ( std::is_same_v<T, value_type_e> )
      tmp.data.type = mod;
    else if constexpr ( std::is_same_v<T, bool> )  // check bool before is_integral as bool is also integral
      tmp.data.use_stacks = mod;
    else if constexpr ( std::is_integral_v<T> )
      tmp.mask = mod;
    else
      assert( false && "Invalid type for parse_spell_effect_mods" );
  }

  template <typename U, typename T, typename... Ts>
  void parse_spell_effect_mods( pack_t<U>& tmp, T mod, Ts... mods )
  {
    parse_spell_effect_mods( tmp, mod );
    parse_spell_effect_mods( tmp, mods... );
  }
};

template <typename PLAYER, typename TD, typename OWNER = PLAYER>
struct parse_action_effects_t : public parse_effects_t
{
private:
  PLAYER* player_;
  action_t* action_;
  std::vector<std::pair<size_t, double>> effect_flat_modifiers;
  std::vector<std::pair<size_t, double>> effect_pct_modifiers;

public:
  // auto parsed dynamic effects
  std::vector<action_effect_t> ta_multiplier_effects;
  std::vector<action_effect_t> da_multiplier_effects;
  std::vector<action_effect_t> execute_time_effects;
  std::vector<action_effect_t> dot_duration_effects;
  std::vector<action_effect_t> tick_time_effects;
  std::vector<action_effect_t> recharge_multiplier_effects;
  std::vector<action_effect_t> cost_effects;
  std::vector<action_effect_t> flat_cost_effects;
  std::vector<action_effect_t> crit_chance_effects;
  std::vector<target_effect_t<TD>> target_multiplier_effects;
  std::vector<target_effect_t<TD>> target_crit_damage_effects;
  std::vector<target_effect_t<TD>> target_crit_chance_effects;

  parse_action_effects_t( PLAYER* p, action_t* a ) : parse_effects_t(), player_( p ), action_( a ) {}
  virtual ~parse_action_effects_t() = default;

  // Syntax: parse_effects( data[, spells|condition|ignore_mask|use_stacks|value_type|spells][,...] )
  //   (buff_t*) or
  //   (const spell_data_t*)   data: Buff or spell to be checked for to see if effect applies. If buff is used, effect
  //                                 will require the buff to be active. If spell is used, effect will always apply
  //                                 unless an optional condition function is provided.
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the buff
  //   (bool F())         condition: Function that takes no arguments and returns true if the effect should apply
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  //   (bool)            use_stacks: Default true, whether to multiply value by stacks
  //   (value_type_e)          type: Source of the value to be used for the effect
  //                                 USE_DATA = spell data (default)
  //                                 USE_DEFAULT = buff default value
  //                                 USE_CURRENT = buff current value
  //
  // Example 1: Parse buff1, ignore effects #1 #3 #5, modify by talent1, modify by tier1:
  //   parse_effects( buff1, 0b10101U, talent1, tier1 );
  //
  // Example 2: Parse buff2, don't multiply by stacks, use the default value set on the buff instead of effect value:
  //   parse_effects( buff2, false, USE_DEFAULT );
  //
  // Example 3: Parse spell1, modify by talent1, only apply if my_player_t::check1() returns true:
  //   parse_effects( spell1, talent1, &my_player_t::check1 );
  //
  // Example 4: Parse buff3, only apply if my_player_t::check2() and my_player_t::check3() returns true:
  //   parse_effects( buff3, [ this ] { return p()->check2() && p()->check3(); } );
  void parse_spell_effect( pack_t<action_effect_t>& tmp, const spell_data_t* s_data, size_t i, bool force )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val = 0.0;
    double val_mul = 0.01;

    if ( tmp.data.buff && tmp.data.type == USE_DEFAULT )
      val = tmp.data.buff->default_value * 100;
    else if ( mastery )
      val = eff.mastery_value();
    else
      val = eff.base_value();

    // TODO: more robust logic around 'party' buffs with radius
    if ( eff.radius() )
      return;

    // Only parse apply aura effects
    switch ( eff.type() )
    {
      case E_APPLY_AURA:
      case E_APPLY_AREA_AURA_PARTY:
      case E_APPLY_AURA_PET:
        break;
      default:
        return;
    }

    apply_affecting_mods( tmp, val, mastery, s_data, i );

    if ( !val )
      return;

    if ( mastery )
      val_mul = 1.0;

    auto debug_message = [ & ]( std::string_view type ) {
      if ( tmp.data.buff )
      {
        std::string val_str;

        if ( tmp.data.type == value_type_e::USE_CURRENT )
          val_str = "current value";
        else if ( tmp.data.type == value_type_e::USE_DEFAULT )
          val_str = fmt::format( "default value ({})", val * val_mul );
        else if ( mastery )
          val_str = fmt::format( "{}*mastery", val * val_mul * 100 );
        else
          val_str = fmt::format( "{}", val * val_mul );

        action_->sim->print_debug( "buff-effects: {} ({}) {} modified by {} {} buff {} ({}#{})", action_->name(),
                                   action_->id, type, val_str, tmp.data.use_stacks ? "per stack of" : "with",
                                   tmp.data.buff->name(), tmp.data.buff->data().id(), i );
      }
      else if ( mastery && !tmp.data.func )
      {
        action_->sim->print_debug( "mastery-effects: {} ({}) {} modified by {}*mastery from {} ({}#{})",
                                   action_->name(), action_->id, type, val * val_mul * 100, s_data->name_cstr(),
                                   s_data->id(), i );
      }
      else if ( tmp.data.func )
      {
        action_->sim->print_debug( "conditional-effects: {} ({}) {} modified by {} with condition from {} ({}#{})",
                                   action_->name(), action_->id, type, val * val_mul, s_data->name_cstr(), s_data->id(),
                                   i );
      }
      else
      {
        action_->sim->print_debug( "passive-effects: {} ({}) {} modified by {} from {} ({}#{})", action_->name(),
                                   action_->id, type, val * val_mul, s_data->name_cstr(), s_data->id(), i );
      }
    };

    std::vector<action_effect_t>* vec = nullptr;
    std::string str;

    if ( !action_->special && eff.subtype() == A_MOD_AUTO_ATTACK_PCT )
    {
      vec = &da_multiplier_effects;
      str = "auto attack";
    }
    else if ( !action_->data().affected_by_all( eff ) && !force )
    {
      return;
    }
    else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_GENERIC:
          vec = &da_multiplier_effects;
          str = "direct damage";
          break;
        case P_DURATION:
          vec = &dot_duration_effects;
          str = "duration";
          break;
        case P_TICK_DAMAGE:
          vec = &ta_multiplier_effects;
          str = "tick damage";
          break;
        case P_CAST_TIME:
          vec = &execute_time_effects;
          str = "cast time";
          break;
        case P_TICK_TIME:
          vec = &tick_time_effects;
          str = "tick time";
          break;
        case P_COOLDOWN:
          vec = &recharge_multiplier_effects;
          str = "cooldown";
          break;
        case P_RESOURCE_COST:
          vec = &cost_effects;
          str = "cost percent";
          break;
        default:
          return;
      }
    }
    else if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_CRIT:
          vec = &crit_chance_effects;
          str = "crit chance";
          break;
        case P_RESOURCE_COST:
          val_mul = eff.resource_multiplier( action_->current_resource() );
          vec = &flat_cost_effects;
          str = "flat cost";
          break;
        default:
          return;
      }
    }

    if ( vec )
    {
      tmp.data.value = val * val_mul;
      tmp.data.mastery = mastery;
      tmp.data.eff = &eff;

      vec->push_back( tmp.data );
      debug_message( str );
    }
  }

  template <typename T>
  const spell_data_t* resolve_parse_data( T data, pack_t<action_effect_t>& tmp )
  {
    if constexpr ( std::is_invocable_v<decltype( &buff_t::data ), T> )
    {
      if ( !data )
        return nullptr;

      tmp.data.buff = data;
      return &data->data();
    }
    else if constexpr ( std::is_invocable_v<decltype( &spell_data_t::ok ), T> )
    {
      return data;
    }
    else
    {
      return nullptr;
    }
  }

  template <typename T, typename... Ts>
  void parse_effects( T data, Ts... mods )
  {
    pack_t<action_effect_t> pack;
    const spell_data_t* spell = resolve_parse_data( data, pack );

    if ( !spell || !spell->ok() )
      return;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      // local copy of pack per effect
      pack_t<action_effect_t> tmp = pack;

      parse_spell_effect( tmp, spell, i, false );
    }
  }

  template <typename T, typename... Ts>
  void force_effect( T data, unsigned idx, Ts... mods )
  {
    pack_t<action_effect_t> pack;
    const spell_data_t* spell = resolve_parse_data( data, pack );

    if ( !spell || !spell->ok() )
      return;

    if ( action_->data().affected_by_all( spell->effectN( idx ) ) )
    {
      assert( false && "Effect already affects action, no need to force" );
      return;
    }

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    parse_spell_effect( pack, spell, idx, true );
  }

  double get_buff_effects_value( const std::vector<action_effect_t>& buffeffects, bool flat = false,
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

        if ( i.type == USE_CURRENT )
          eff_val = i.buff->check_value();
      }

      if ( i.mastery )
        eff_val *= action_->player->cache.mastery();

      if ( flat )
        return_value += eff_val * mod;
      else
        return_value *= 1.0 + eff_val * mod;
    }

    return return_value;
  }

  // method for getting target data as each module may have different action scoped method
  TD* parse_effects_target_data( player_t* t ) const
  {
    if constexpr ( std::is_invocable_v<decltype( &pet_t::owner ), PLAYER> )
      return static_cast<OWNER*>( player_->owner )->get_target_data( t );
    else
      return player_->get_target_data( t );
  }

  // Syntax: parse_debuff_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer mutiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  template <typename... Ts>
  void parse_debuff_effect( pack_t<target_effect_t<TD>>& tmp, const spell_data_t* s_data, size_t i, bool force )
  {
    const auto& eff = s_data->effectN( i );
    bool mastery = s_data->flags( SX_MASTERY_AFFECTS_POINTS );
    double val = 0.0;
    double val_mul = 0.01;

    if ( mastery )
      val = eff.mastery_value();
    else
      val = eff.base_value();

    if ( eff.type() != E_APPLY_AURA )
      return;

    apply_affecting_mods( tmp, val, mastery, s_data, i );

    if ( !val )
      return;

    if ( mastery )
      val_mul = 1.0;

    auto debug_message = [ & ]( std::string_view type ) {
      action_->sim->print_debug( "dot-debuffs: {} ({}) {} modified by {}{} on targets with dot {} ({}#{})",
                                action_->name(), action_->id, type, val * val_mul, mastery ? "*mastery" : "",
                                s_data->name_cstr(), s_data->id(), i );
    };

    std::vector<target_effect_t<TD>>* vec = nullptr;
    std::string str;

    if ( !action_->special && eff.subtype() == A_MOD_AUTO_ATTACK_FROM_CASTER )
    {
      vec = &target_multiplier_effects;
      str = "auto attack";
    }
    else if ( !action_->data().affected_by_all( eff ) && !force )
    {
      return;
    }
    else
    {
      switch ( eff.subtype() )
      {
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS:
        case A_MOD_DAMAGE_FROM_CASTER_SPELLS_LABEL:
          vec = &target_multiplier_effects;
          str = "damage";
          break;
        case A_MOD_CRIT_CHANCE_FROM_CASTER_SPELLS:
          vec = &target_crit_chance_effects;
          str = "crit chance";
          break;
        case A_MOD_CRIT_DAMAGE_PCT_FROM_CASTER_SPELLS:
          vec = &target_crit_damage_effects;
          str = "crit damage";
          break;
        default:
          return;
      }
    }

    if ( vec )
    {
      tmp.data.value = val * val_mul;
      tmp.data.mastery = mastery;
      tmp.data.eff = &eff;

      vec->push_back( tmp.data );
      debug_message( str );
    }
  }

  template <typename... Ts>
  void parse_debuff_effects( const std::function<int( TD* )>& fn, const spell_data_t* spell, Ts... mods )
  {
    pack_t<target_effect_t<TD>> pack;

    if ( !spell || !spell->ok() )
      return;

    pack.data.func = std::move( fn );

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( pack.mask & 1 << ( i - 1 ) )
        continue;

      // local copy of pack per effect
      pack_t<target_effect_t<TD>> tmp = pack;

      parse_debuff_effect( tmp, spell, i, false );
    }
  }

  template <typename... Ts>
  void force_debuff_effect( const std::function<int( TD* )>& fn, const spell_data_t* spell, unsigned idx, Ts... mods )
  {
    pack_t<target_effect_t<TD>> pack;

    if ( !spell || !spell->ok() )
      return;

    if ( action_->data().affected_by_all( spell->effectN( idx ) ) )
    {
      assert( false && "Effect already affects action, no need to force" );
      return;
    }

    pack.data.func = std::move( fn );

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

    parse_debuff_effect( pack, spell, idx, true );
  }

  double get_debuff_effects_value( const std::vector<target_effect_t<TD>>& dotdebuffs, TD* td, bool flat = false ) const
  {
    double return_value = flat ? 0.0 : 1.0;

    for ( const auto& i : dotdebuffs )
    {
      if ( auto check = i.func( td ) )
      {
        auto eff_val = i.value;

        if ( i.mastery )
          eff_val *= action_->player->cache.mastery();

        if ( flat )
          return_value += eff_val * check;
        else
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
    pack_t<void*> pack;

    if ( !s_data || !s_data->ok() )
      return;

    // parse mods and populate pack
    parse_spell_effect_mods( pack, mods... );

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
        case P_EFFECT_1: target_idx = 1; break;
        case P_EFFECT_2: target_idx = 2; break;
        case P_EFFECT_3: target_idx = 3; break;
        case P_EFFECT_4: target_idx = 4; break;
        case P_EFFECT_5: target_idx = 5; break;
        default:
          continue;
      }

      if ( !action_->data().affected_by_all( eff ) )
        continue;

      double val = eff.base_value();
      bool m;  // dummy throwaway

      apply_affecting_mods( pack, val, m, s_data, i );

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
    spelleffect_data_t temp = action_->data().effectN( idx );

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
    if ( !total_effects_count() )
      return;

    os << "<div>\n"
       << "<h4>Affected By (Dynamic)</h4>\n"
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

    print_parsed_type( os, da_multiplier_effects, "Direct Damage" );
    print_parsed_type( os, ta_multiplier_effects, "Periodic Damage" );
    print_parsed_type( os, crit_chance_effects, "Critical Strike Chance" );
    print_parsed_type( os, execute_time_effects, "Execute Time" );
    print_parsed_type( os, dot_duration_effects, "Dot Duration" );
    print_parsed_type( os, tick_time_effects, "Tick Time" );
    print_parsed_type( os, recharge_multiplier_effects, "Recharge Multiplier" );
    print_parsed_type( os, flat_cost_effects, "Flat Cost" );
    print_parsed_type( os, cost_effects, "Percent Cost" );
    print_parsed_type( os, target_multiplier_effects, "Damage on Debuff" );
    print_parsed_type( os, target_crit_chance_effects, "Crit Chance on Debuff" );
    print_parsed_type( os, target_crit_damage_effects, "Crit Damage on Debuff" );
    print_parsed_custom_type( os );

    os << "</table>\n"
       << "</div>\n";
  }

  virtual size_t total_effects_count()
  {
    return ta_multiplier_effects.size() +
           da_multiplier_effects.size() +
           execute_time_effects.size() +
           dot_duration_effects.size() +
           tick_time_effects.size() +
           recharge_multiplier_effects.size() +
           cost_effects.size() +
           flat_cost_effects.size() +
           crit_chance_effects.size() +
           target_multiplier_effects.size();
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

  std::string value_type_name( value_type_e t )
  {
    switch ( t )
    {
      case USE_DEFAULT: return "Default Value";
      case USE_CURRENT: return "Current Value";
      default:          return "Spell Data";
    }
  }

  void print_parsed_line( report::sc_html_stream& os, const action_effect_t& entry )
  {
    std::vector<std::string> notes;

    if ( !entry.use_stacks )
      notes.push_back( "No-stacks" );

    if ( entry.mastery )
      notes.push_back( "Mastery" );

    if ( entry.func )
      notes.push_back( "Conditional" );

    os.format( "<td>{}</td><td>{}</td><td>{}</td><td>{:.3f}</td><td>{}</td><td>{}</td></tr>\n",
               entry.eff->spell()->name_cstr(),
               entry.eff->spell()->id(),
               entry.eff->index() + 1,
               entry.value * ( entry.mastery ? 100 : 1 ),
               value_type_name( entry.type ),
               util::string_join( notes ) );
  }

  void print_parsed_line( report::sc_html_stream& os, const target_effect_t<TD>& entry )
  {
    os.format( "<td>{}</td><td>{}</td><td>{}</td><td>{:.3f}</td><td>{}</td><td>{}</td></tr>\n",
               entry.eff->spell()->name_cstr(),
               entry.eff->spell()->id(),
               entry.eff->index() + 1,
               entry.value * ( entry.mastery ? 100 : 1 ),
               "",
               entry.mastery ? "Mastery" : "" );
  }
};
