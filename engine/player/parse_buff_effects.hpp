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
// 1) Add `parse_buff_effects_t` as an additional parent
// 2) Within the constructor, set `action_ = this;`
// 3) `get_buff_effects_value( buff effect vector ) returns the modified value.
//    Add the following overrides with any addtional adjustments as needed (BASE is the parent to the action base class):

/*    double cost() const override
      {
        double c = BASE::cost();
        c += get_buff_effects_value( flat_cost_buffeffects, true, false );
        c *= std::max( 0.0, get_buff_effects_value( cost_buffeffects, false, false ) );
        return c;
      }

      double composite_ta_multiplier( const action_state_t* s ) const override
      {
        return BASE::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_buffeffects );
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        return BASE::composite_da_multiplier( s ) * get_buff_effects_value( da_multiplier_buffeffects );
      }

      double composite_crit_chance() const override
      {
        return BASE::composite_crit_chance() + get_buff_effects_value( crit_chance_buffeffects, true );
      }

      timespan_t execute_time() const override
      {
        return std::max( 0_ms, BASE::execute_time() * get_buff_effects_value( execute_time_buffeffects ) );
      }

      double recharge_multiplier( const cooldown_t& cd ) const override
      {
        return BASE::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects );
      }
*/

// Syntax: parse_buff_effects[<S|C[,...]>]( buff[, ignore_mask|use_stacks[, use_default]][, spell|conduit][,...] )
//  buff = buff to be checked for to see if effect applies
//  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit
//  use_stacks = optional, default true, whether to multiply value by stacks, mutually exclusive with ignore parameters
//  use_default = optional, default false, whether to use buff's default value over effect's value
//  S/C = optional list of template parameter to indicate spell or conduit with redirect effects
//  spell/conduit = optional list of spell or conduit with redirect effects that modify the effects on the buff
//
// Example 1: Parse buff1, ignore effects #1 #3 #5, modify by talent1, modiry by tier1:
//  parse_buff_effects<S,S>( buff1, 0b10101, talent1, tier1 );
//
// Example 2: Parse buff2, don't multiply by stacks, use the default value set on the buff instead of effect value:
//  parse_buff_effects( buff2, false, true );

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

  // auto parsed dynamic effects
  std::vector<buff_effect_t> ta_multiplier_buffeffects;
  std::vector<buff_effect_t> da_multiplier_buffeffects;
  std::vector<buff_effect_t> execute_time_buffeffects;
  std::vector<buff_effect_t> dot_duration_buffeffects;
  std::vector<buff_effect_t> recharge_multiplier_buffeffects;
  std::vector<buff_effect_t> cost_buffeffects;
  std::vector<buff_effect_t> flat_cost_buffeffects;
  std::vector<buff_effect_t> crit_chance_buffeffects;
  action_t* action_;

  virtual void apply_buff_effects() = 0;

  double mod_spell_effects_value( const spell_data_t*, const spelleffect_data_t& e ) { return e.base_value(); }

  double mod_spell_effects_value( const conduit_data_t& c, const spelleffect_data_t& ) { return c.value(); }

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

  template <typename... Ts>
  void parse_buff_effect( buff_t* buff, bfun f, const spell_data_t* s_data, size_t i, bool use_stacks, bool use_default,
                          Ts... mods )
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

    if ( !action_->data().affected_by_all( eff ) )
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

    const spell_data_t* s_data = &buff->data();

    for ( size_t i = 1; i <= s_data->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( buff, nullptr, s_data, i, use_stacks, use_default, mods... );
    }
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, unsigned ignore_mask, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, ignore_mask, true, false, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, bool use_default, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, 0U, stack, use_default, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, bool stack, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, 0U, stack, false, mods... );
  }

  template <typename... Ts>
  void parse_buff_effects( buff_t* buff, Ts... mods )
  {
    parse_buff_effects<Ts...>( buff, 0U, true, false, mods... );
  }

  void parse_conditional_effects( const spell_data_t* spell, bfun f, unsigned ignore_mask = 0U, bool use_stack = true,
                                  bool use_default = false )
  {
    if ( !spell || !spell->ok() )
      return;

    for ( size_t i = 1 ; i <= spell->effect_count(); i++ )
    {
      if ( ignore_mask & 1 << ( i - 1 ) )
        continue;

      parse_buff_effect( nullptr, f, spell, i, use_stack, use_default );
    }
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
};
