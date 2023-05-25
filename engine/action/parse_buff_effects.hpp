// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "player/player.hpp"
#include "sim/sim.hpp"

#include <functional>

// Mixin to action base class to allow auto parsing and dynamic application of whitelist based buffs & auras.
// 1) Add `parse_buff_effects_t` as an additional parent with the target data class as template parameter:
//
//    struct my_action_base_t : public action_t, parse_buff_effects_t<my_target_data_t>
//
// 2) Construct the mixin via `parse_buff_effects_t( this );`
//
// 3) `get_buff_effects_value( buff effect vector ) returns the modified value.
//    Add the following overrides with any addtional adjustments as needed (BASE is the parent to the action base class):

/*    double cost() const override
      { return std::max( 0.0, ( BASE::cost() + get_buff_effects_value( flat_cost_buffeffects, true, false ) )
                                             * get_buff_effects_value( cost_buffeffects, false, false ) ); }

      double composite_ta_multiplier( const action_state_t* s ) const override
      { return BASE::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_buffeffects ); }

      double composite_da_multiplier( const action_state_t* s ) const override
      { return BASE::composite_da_multiplier( s ) * get_buff_effects_value( da_multiplier_buffeffects ); }

      double composite_crit_chance() const override
      { return BASE::composite_crit_chance() + get_buff_effects_value( crit_chance_buffeffects, true ); }

      timespan_t execute_time() const override
      { return std::max( 0_ms, BASE::execute_time() * get_buff_effects_value( execute_time_buffeffects ) ); }

      timespan_t composite_dot_duration( const action_state_t* s ) const override
      { return BASE::composite_dot_duration( s ) * get_buff_effects_value( dot_duration_buffeffects ); }

      double recharge_multiplier( const cooldown_t& cd ) const override
      { return BASE::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects ); }

      double composite_target_multiplier( player_t* t ) const override
      { return BASE::composite_target_multiplier( t ) * get_debuff_effect_value( td( t ) ); }
*/

template <typename TD>
struct parse_buff_effects_t
{
  using bfun = std::function<bool()>;
  struct buff_effect_t
  {
    buff_t* buff;
    double value;
    bool use_stacks;
    bool mastery;
    bfun func;

    buff_effect_t( buff_t* b, double v, bool s = true, bool m = false, bfun f = nullptr )
      : buff( b ), value( v ), use_stacks( s ), mastery( m ), func( std::move( f ) )
    {}
  };

  using dfun = std::function<int( TD* )>;
  struct dot_debuff_t
  {
    dfun func;
    double value;
    bool mastery;

    dot_debuff_t( dfun f, double v, bool m = false )
      : func( std::move( f ) ), value( v ), mastery( m )
    {}
  };

private:
  action_t* action_;
  std::vector<std::pair<size_t, double>> effect_flat_modifiers;
  std::vector<std::pair<size_t, double>> effect_pct_modifiers;

public:
  // auto parsed dynamic effects
  std::vector<buff_effect_t> ta_multiplier_buffeffects;
  std::vector<buff_effect_t> da_multiplier_buffeffects;
  std::vector<buff_effect_t> execute_time_buffeffects;
  std::vector<buff_effect_t> dot_duration_buffeffects;
  std::vector<buff_effect_t> recharge_multiplier_buffeffects;
  std::vector<buff_effect_t> cost_buffeffects;
  std::vector<buff_effect_t> flat_cost_buffeffects;
  std::vector<buff_effect_t> crit_chance_buffeffects;
  std::vector<dot_debuff_t> target_multiplier_dotdebuffs;

  parse_buff_effects_t( action_t* a ) : action_( a ) {}
  virtual ~parse_buff_effects_t() = default;

  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  template <typename T>
  void parse_spell_effects_mods( double& val, bool& mastery, const spell_data_t* base, size_t idx, T mod )
  {
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
        double pct = mod_spell_effects_value( mod, eff );

        if ( eff.subtype() == A_ADD_FLAT_MODIFIER || eff.subtype() == A_ADD_FLAT_LABEL_MODIFIER )
          val += pct;
        else if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
          val *= 1.0 + pct / 100;
        else if ( eff.subtype() == A_PROC_TRIGGER_SPELL_WITH_VALUE )
          val = pct;
        else
          continue;

        if ( action_->player->find_mastery_spell( action_->player->specialization() ) == mod )
          mastery = true;
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

  // Syntax: parse_buff_effects( buff[, ignore_mask|use_stacks[, use_default]][, spell][,...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit, must be typed as unsigned
  //  use_stacks = optional, default true, whether to multiply value by stacks, mutually exclusive with ignore parameters
  //  use_default = optional, default false, whether to use buff's default value over effect's value
  //  spell = optional list of spell with redirect effects that modify the effects on the buff
  //
  // Example 1: Parse buff1, ignore effects #1 #3 #5, modify by talent1, modify by tier1:
  //  parse_buff_effects<S,S>( buff1, 0b10101U, talent1, tier1 );
  //
  // Example 2: Parse buff2, don't multiply by stacks, use the default value set on the buff instead of effect value:
  //  parse_buff_effects( buff2, false, true );
  template <typename... Ts>
  void parse_buff_effect( buff_t* buff, const bfun& f, const spell_data_t* s_data, size_t i, bool use_stacks,
                          bool use_default, bool force, Ts... mods )
  {
    const auto& eff = s_data->effectN( i );
    double val      = eff.base_value();
    double val_mul  = 0.01;
    bool mastery    = action_->player->find_mastery_spell( action_->player->specialization() ) == s_data;

    // TODO: more robust logic around 'party' buffs with radius
    if ( !( eff.type() == E_APPLY_AURA || eff.type() == E_APPLY_AREA_AURA_PARTY ) || eff.radius() ) return;

    if ( i <= 5 )
      parse_spell_effects_mods( val, mastery, s_data, i, mods... );

    auto debug_message = [ & ]( std::string_view type ) {
      if ( buff )
      {
        action_->sim->print_debug( "buff-effects: {} ({}) {} modified by {}{} with buff {} ({}#{})", action_->name(),
                                   action_->id, type, val * val_mul, mastery ? "+mastery" : "", buff->name(),
                                   buff->data().id(), i );
      }
      else if ( mastery && !f )
      {
        action_->sim->print_debug( "mastery-effects: {} ({}) {} modified by {}+mastery from {} ({}#{})",
                                   action_->name(), action_->id, type, val * val_mul, s_data->name_cstr(), s_data->id(),
                                   i );
      }
      else if ( f )
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

    if ( !action_->special && eff.subtype() == A_MOD_AUTO_ATTACK_PCT )
    {
      da_multiplier_buffeffects.emplace_back( buff, val * val_mul );
      debug_message( "auto attack" );
      return;
    }

    if ( !action_->data().affected_by_all( eff ) && !force )
      return;

    if ( use_default && buff)
    {
      val = buff->default_value;
      val_mul = 1.0;
    }

    if ( !mastery && !val )
      return;

    if ( eff.subtype() == A_ADD_PCT_MODIFIER || eff.subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      switch ( eff.misc_value1() )
      {
        case P_GENERIC:
          da_multiplier_buffeffects.emplace_back( buff, val * val_mul, use_stacks, mastery, f );
          debug_message( "direct damage" );
          break;
        case P_DURATION:
          dot_duration_buffeffects.emplace_back( buff, val * val_mul, use_stacks, mastery, f );
          debug_message( "duration" );
          break;
        case P_TICK_DAMAGE:
          ta_multiplier_buffeffects.emplace_back( buff, val * val_mul, use_stacks, mastery, f );
          debug_message( "tick damage" );
          break;
        case P_CAST_TIME:
          execute_time_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "cast time" );
          break;
        case P_COOLDOWN:
          recharge_multiplier_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "cooldown" );
          break;
        case P_RESOURCE_COST:
          cost_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "cost percent" );
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
          crit_chance_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "crit chance" );
          break;
        case P_RESOURCE_COST:
          val_mul = eff.resource_multiplier( action_->current_resource() );
          flat_cost_buffeffects.emplace_back( buff, val * val_mul, use_stacks, false, f );
          debug_message( "flat cost" );
          break;
        default:
          return;
      }
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore_mask, bool use_stacks, bool use_default, Ts... mods )
  {
    if ( !buff )
      return;

    const spell_data_t* spell = &buff->data();

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( buff, nullptr, spell, i, use_stacks, use_default, false, mods... );
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore_mask, Ts... mods )
  {
    parse_buff_effects( buff, ignore_mask, true, false, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, bool use_default, Ts... mods )
  {
    parse_buff_effects( buff, 0U, stack, use_default, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, Ts... mods )
  {
    parse_buff_effects( buff, 0U, stack, false, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, Ts... mods )
  {
    parse_buff_effects( buff, 0U, true, false, mods... );
  }

  template <typename... Ts>
  void force_buff_effect( buff_t* buff, unsigned idx, bool stack, bool use_default, Ts... mods )
  {
    if ( action_->data().affected_by_all( buff->data().effectN( idx ) ) )
      return;

    parse_buff_effect( buff, nullptr, &buff->data(), idx, stack, use_default, true, mods... );
  }

  template <typename... Ts>
  void force_buff_effect( buff_t* buff, unsigned idx, Ts... mods )
  {
    force_buff_effect( buff, idx, true, false, mods... );
  }

  void parse_conditional_effects( const spell_data_t* spell, const bfun& func, unsigned ignore_mask = 0U,
                                  bool use_stack = true, bool use_default = false )
  {
    if ( !spell || !spell->ok() )
      return;

    for ( size_t i = 1 ; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( nullptr, func, spell, i, use_stack, use_default, false );
    }
  }

  void force_conditional_effect( const spell_data_t* spell, const bfun& func, unsigned idx, bool use_stack = true,
                                 bool use_default = false )
  {
    if ( action_->data().affected_by_all( spell->effectN( idx ) ) )
      return;

    parse_buff_effect( nullptr, func, spell, idx, use_stack, use_default, true );
  }

  void parse_passive_effects( const spell_data_t* spell, unsigned ignore_mask = 0U )
  {
    parse_conditional_effects( spell, nullptr, ignore_mask );
  }

  double get_buff_effects_value( const std::vector<buff_effect_t>& buffeffects, bool flat = false,
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
      }

      if ( i.mastery )
        eff_val += action_->player->cache.mastery_value();

      if ( flat )
        return_value += eff_val * mod;
      else
        return_value *= 1.0 + eff_val * mod;
    }

    return return_value;
  }

  // Syntax: parse_debuff_effects( func, debuff[, spell][,...] )
  //  func = function taking the class's target_data as argument and returning an integer
  //  debuff = spell data of the debuff
  //  spell = optional list of spells with redirect effects that modify the effects on the debuff
  template <typename... Ts>
  void parse_debuff_effect( const dfun& func, const spell_data_t* s_data, size_t i, bool force, Ts... mods )
  {
    const auto& eff = s_data->effectN( i );
    double val = eff.base_value();
    double val_mul = 0.01;
    bool mastery = false;

    if ( eff.type() != E_APPLY_AURA )
      return;

    if ( i <= 5 )
      parse_spell_effects_mods( val, mastery, s_data, i, mods... );

    if ( !mastery && !val )
      return;

    if ( !( eff.subtype() == A_MOD_DAMAGE_FROM_CASTER_SPELLS && action_->data().affected_by_all( eff ) ) &&
         !( eff.subtype() == A_MOD_AUTO_ATTACK_FROM_CASTER && !action_->special ) && !force )
      return;

    target_multiplier_dotdebuffs.emplace_back( func, val * val_mul, mastery );
    action_->sim->print_debug( "dot-debuffs: {} ({}) damage modified by {}{} on targets with dot {} ({}#{})",
                               action_->name(), action_->id, val * val_mul, mastery ? "+mastery" : "",
                               s_data->name_cstr(), s_data->id(), i );

  }

  template <typename... Ts>
  void parse_debuff_effects( const dfun& func, const spell_data_t* spell, Ts... mods )
  {
    if ( !spell->ok() )
      return;

    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      parse_debuff_effect( func, spell, i, false, mods... );
    }
  }

  template <typename... Ts>
  void force_debuff_effect( const dfun& func, const spell_data_t* spell, unsigned idx, Ts... mods )
  {
    if ( action_->data().affected_by_all( spell->effectN( idx ) ) )
      return;

    parse_debuff_effect( func, spell, idx, true, mods... );
  }

  virtual double get_debuff_effects_value( TD* t ) const
  {
    double return_value = 1.0;

    for ( const auto& i : target_multiplier_dotdebuffs )
    {
      if ( auto check = i.func( t ) )
      {
        auto eff_val = i.value;

        if ( i.mastery )
          eff_val += action_->player->cache.mastery_value();

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
    spelleffect_data_t temp = action_->data().effectN( idx );

    for ( auto [ i, v ] : effect_flat_modifiers )
      if ( i == idx  )
        temp._base_value += v;

    for ( auto [ i, v ] : effect_pct_modifiers )
      if ( i == idx  )
        temp._base_value *= 1.0 + v;

    return temp;
  }
};
