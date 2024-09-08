
// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
]NOTES:

- To evaluate Combo Strikes in the APL, use:
    if=combo_break    true if action is a repeat and can combo strike
    if=combo_strike   true if action is not a repeat or can't combo strike

- To show CJL can be interupted in the APL, use:
     &!prev_gcd.crackling_jade_lightning,interrupt=1

TODO:

GENERAL:
- See other options of modeling Spinning Crane Kick

WINDWALKER:
- Add Cyclone Strike Counter as an expression
- See about removing tick part of Crackling Tiger Lightning

MISTWEAVER:
- Essence Font - See if the implementation can be corrected to the intended design.
- Life Cocoon - Double check if the Enveloping Mists and Renewing Mists from Mists of Life proc the mastery or not.
- Not Modeled:
zen pulse + talent behind (echoing reverberation) needs to be added
sheilun + talent behind (shaohao's lessons) needs to be added
secret infusion (talent) needs to be added
touch of death for mistweaver (basically copy from ww but without the mastery interaction) needs to be added
Ancient Concordance + Awakened Jadefire (two jadefire stomp talents, like jadefire harmony esque) needs to be added
and I guess the ap% of the spells were not updated throughout the buffs, but idk about that one

BREWMASTER:
*/
#include "sc_monk.hpp"

#include "action/action_callback.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/apl_monk.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "sc_enums.hpp"

#include <deque>

#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace monk
{
namespace actions
{
// ==========================================================================
// Monk Actions
// ==========================================================================
// Template for common monk action code. See priest_action_t.

template <class Base>
template <typename... Args>
monk_action_t<Base>::monk_action_t( Args &&...args )
  : parse_action_effects_t<Base>( std::forward<Args>( args )... ),
    sef_ability( actions::sef_ability_e::SEF_NONE ),
    ww_mastery( false ),
    may_combo_strike( false ),
    trigger_chiji( false ),
    cast_during_sck( false ),
    track_cd_waste( false )
{
  range::fill( _resource_by_stance, RESOURCE_MAX );

  apply_buff_effects();
  apply_debuff_effects();

  track_cd_waste = base_t::data().cooldown() > 0_ms || base_t::data().charge_cooldown() > 0_ms;
}

template <class Base>
std::string monk_action_t<Base>::full_name() const
{
  std::string n = base_t::data().name_cstr();
  return n.empty() ? base_t::name_str : n;
}

template <class Base>
monk_t *monk_action_t<Base>::p()
{
  return debug_cast<monk_t *>( base_t::player );
}

template <class Base>
const monk_t *monk_action_t<Base>::p() const
{
  return debug_cast<monk_t *>( base_t::player );
}

template <class Base>
monk_td_t *monk_action_t<Base>::get_td( player_t *target ) const
{
  return p()->get_target_data( target );
}

template <class Base>
const monk_td_t *monk_action_t<Base>::find_td( player_t *target ) const
{
  return p()->find_target_data( target );
}

template <class Base>
void monk_action_t<Base>::apply_buff_effects()
{
  /*
   * Permanent action-specific effects that apply to more than one action.
   */

  // Monk
  apply_affecting_aura( p()->baseline.monk.aura );
  apply_affecting_aura( p()->talent.monk.chi_proficiency );
  apply_affecting_aura( p()->talent.monk.fast_feet );

  // Brewmaster
  apply_affecting_aura( p()->baseline.brewmaster.aura );
  apply_affecting_aura( p()->baseline.brewmaster.aura_2 );

  // Mistweaver
  apply_affecting_aura( p()->baseline.mistweaver.aura );
  apply_affecting_aura( p()->baseline.mistweaver.aura_2 );
  apply_affecting_aura( p()->baseline.mistweaver.aura_3 );

  // Windwalker
  apply_affecting_aura( p()->baseline.windwalker.aura );
  apply_affecting_aura( p()->baseline.windwalker.aura_2 );
  apply_affecting_aura( p()->talent.windwalker.brawlers_intensity );
  apply_affecting_aura( p()->talent.windwalker.hardened_soles );
  apply_affecting_aura( p()->talent.windwalker.shadowboxing_treads );
  apply_affecting_aura( p()->talent.windwalker.rising_star );

  // Conduit of the Celestials
  apply_affecting_aura( p()->talent.conduit_of_the_celestials.temple_training );
  apply_affecting_aura( p()->talent.conduit_of_the_celestials.xuens_guidance );

  // Master of Harmony
  apply_affecting_aura( p()->talent.master_of_harmony.manifestation );

  // Shado-Pan
  parse_effects( p()->talent.shado_pan.efficient_training, p()->specialization() == MONK_WINDWALKER
                                                               ? effect_mask_t( true ).disable( 5 )
                                                               : effect_mask_t( true ) );
  apply_affecting_aura( p()->talent.shado_pan.one_versus_many );
  apply_affecting_aura( p()->talent.shado_pan.vigilant_watch );

  // TWW S1 Set Effects
  apply_affecting_aura( p()->sets->set( MONK_BREWMASTER, TWW1, B2 ) );

  // TWW S2 Set Effects

  // TWW S3 Set Effects

  // TWW S4 Set Effects

  /*
   * Temporary action-specific effects that apply to more than one action.
   * If so, the aura gets parsed here with `parse_effects`.
   */

  // Monk
  parse_effects( p()->buff.fatal_touch );

  // Brewmaster
  parse_effects( p()->buff.blackout_combo );
  parse_effects(
      p()->buff.counterstrike,
      affect_list_t( 1 ).add_spell( p()->baseline.brewmaster.spinning_crane_kick->effectN( 1 ).trigger()->id() ),
      CONSUME_BUFF );

  // Mistweaver
  parse_effects( p()->buff.jadefire_brand, p()->talent.windwalker.jadefire_brand_heal );

  // Windwalker
  if ( const auto &effect = p()->baseline.windwalker.mastery->effectN( 1 ); effect.ok() && true )
  {
    add_parse_entry( base_t::da_multiplier_effects )
        .set_buff( p()->buff.combo_strikes )
        .set_func( [ & ] { return ww_mastery; } )
        .set_value( effect.mastery_value() )
        .set_mastery( true )
        .set_eff( &effect );
    add_parse_entry( base_t::ta_multiplier_effects )
        .set_buff( p()->buff.combo_strikes )
        .set_func( [ & ] { return ww_mastery; } )
        .set_value( effect.mastery_value() )
        .set_mastery( true )
        .set_eff( &effect );
  }
  parse_effects( p()->buff.ordered_elements );
  parse_effects( p()->buff.hit_combo );
  parse_effects( p()->buff.press_the_advantage );
  parse_effects( p()->buff.pressure_point );
  parse_effects( p()->buff.storm_earth_and_fire, IGNORE_STACKS, effect_mask_t( false ).enable( 1, 2, 3, 7, 8 ),
                 affect_list_t( 1, 2 ).add_spell( p()->passives.chi_explosion->id() ) );
  parse_effects( p()->buff.bok_proc, p()->talent.windwalker.courageous_impulse );

  // Conduit of the Celestials
  parse_effects( p()->buff.august_dynasty );
  parse_effects( p()->buff.heart_of_the_jade_serpent_cdr,
                 [ & ] { return !p()->buff.heart_of_the_jade_serpent_cdr_celestial->check(); } );
  parse_effects( p()->buff.heart_of_the_jade_serpent_cdr_celestial );
  parse_effects( p()->buff.jade_sanctuary );
  parse_effects( p()->buff.strength_of_the_black_ox );

  // Master of Harmony
  // TODO: parse_effects implementation for A_MOD_HEALING_RECEIVED (283)
  parse_effects( p()->talent.master_of_harmony.aspect_of_harmony_heal, p()->talent.master_of_harmony.coalescence,
                 [ & ] { return p()->buff.aspect_of_harmony.heal_ticking(); } );
  parse_effects( p()->buff.balanced_stratagem_physical, CONSUME_BUFF );
  parse_effects( p()->buff.balanced_stratagem_magic, CONSUME_BUFF );

  // Shado-Pan
  parse_effects( p()->buff.wisdom_of_the_wall_crit );

  // TWW S1 Set Effects
  parse_effects(
      p()->buff.tiger_strikes,
      affect_list_t( 1 ).add_spell(
          p()->baseline.monk.spinning_crane_kick->effectN( 1 ).trigger()->id(), p()->passives.fists_of_fury_tick->id(),
          p()->passives.whirling_dragon_punch_aoe_tick->id(), p()->passives.whirling_dragon_punch_st_tick->id(),
          p()->talent.windwalker.strike_of_the_windlord->effectN( 3 ).trigger()->id(),  // mainhand
          p()->talent.windwalker.strike_of_the_windlord->effectN( 4 ).trigger()->id()   // offhand
          ) );
  parse_effects( p()->buff.tigers_ferocity );
  parse_effects( p()->buff.flow_of_battle_damage );

  // TWW S2 Set Effects

  // TWW S3 Set Effects

  // TWW S4 Set Effects
}

// Action-related parsing of debuffs. Does not work on spells
// These are action multipliers and what-not that only increase the damage
// of abilities. This does not work with tracking buffs or stat-buffs.
// Things like SEF and Serenity or debuffs that increase the crit chance
// of abilities.
template <class Base>
void monk_action_t<Base>::apply_debuff_effects()
{
  if ( p()->talent.brewmaster.weapons_of_order->ok() )
    parse_target_effects( td_fn( &monk_td_t::debuff_t::weapons_of_order ),
                          p()->talent.brewmaster.weapons_of_order_debuff );

  parse_target_effects( td_fn( &monk_td_t::dots_t::aspect_of_harmony ),
                        p()->talent.master_of_harmony.aspect_of_harmony_damage,
                        p()->talent.master_of_harmony.coalescence );

  if ( p()->talent.windwalker.jadefire_harmony->ok() )
    parse_target_effects( td_fn( &monk_td_t::debuff_t::jadefire_brand ), p()->talent.windwalker.jadefire_brand_dmg );
}

template <class Base>
std::unique_ptr<expr_t> monk_action_t<Base>::create_expression( util::string_view name_str )
{
  if ( name_str == "combo_strike" )
    return make_mem_fn_expr( name_str, *this, &monk_action_t::is_combo_strike );
  else if ( name_str == "combo_break" )
    return make_mem_fn_expr( name_str, *this, &monk_action_t::is_combo_break );
  return base_t::create_expression( name_str );
}

template <class Base>
bool monk_action_t<Base>::usable_moving() const
{
  if ( base_t::usable_moving() )
    return true;

  if ( this->execute_time() > timespan_t::zero() )
    return false;

  if ( this->channeled )
    return false;

  if ( this->range > 0 && this->range < p()->current.distance_to_move )
    return false;

  return true;
}

template <class Base>
bool monk_action_t<Base>::ready()
{
  // Spell data nil or not_found
  if ( base_t::data().id() == 0 )
    return false;

  // These abilities are able to be used during Spinning Crane Kick
  if ( cast_during_sck )
    base_t::usable_while_casting = p()->channeling && p()->baseline.monk.spinning_crane_kick &&
                                   ( p()->channeling->id == p()->baseline.monk.spinning_crane_kick->id() );

  return base_t::ready();
}

template <class Base>
void monk_action_t<Base>::init()
{
  base_t::init();

  // Iterate through power entries, and find if there are resources linked to one of our stances
  for ( const spellpower_data_t &pd : base_t::data().powers() )
  {
    switch ( pd.aura_id() )
    {
      case 137023:
        assert( _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER ) ] == RESOURCE_MAX &&
                "Two power entries per aura id." );
        _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER ) ] = pd.resource();
        break;
      case 137024:
        assert( _resource_by_stance[ dbc::spec_idx( MONK_MISTWEAVER ) ] == RESOURCE_MAX &&
                "Two power entries per aura id." );
        _resource_by_stance[ dbc::spec_idx( MONK_MISTWEAVER ) ] = pd.resource();
        break;
      case 137025:
        assert( _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER ) ] == RESOURCE_MAX &&
                "Two power entries per aura id." );
        _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER ) ] = pd.resource();
        break;
      default:
        break;
    }
  }

  // Allow this ability to be cast during SCK
  if ( cast_during_sck && !base_t::background && !base_t::dual )
  {
    if ( base_t::usable_while_casting )
    {
      cast_during_sck = false;
      p()->sim->print_debug( "{}: cast_during_sck ignored because usable_while_casting = true", full_name() );
    }
    else
    {
      base_t::usable_while_casting = true;
      base_t::use_while_casting    = true;
    }
  }
}

template <class Base>
void monk_action_t<Base>::init_finished()
{
  // 2H Weapon Scaling
  if ( base_t::attack_power_mod.direct > 0 )
  {
    if ( base_t::ap_type == attack_power_type::WEAPON_BOTH && p()->main_hand_weapon.group() == WEAPON_2H )
    {
      base_t::ap_type = attack_power_type::WEAPON_MAINHAND;
      base_t::base_multiplier *= 0.98;  // This value is not included in spelldata but is included in the tooltip label
    }
  }
  base_t::init_finished();
}

template <class Base>
void monk_action_t<Base>::reset_swing()
{
  if ( p()->main_hand_attack && p()->main_hand_attack->execute_event )
  {
    p()->main_hand_attack->cancel();
    p()->main_hand_attack->schedule_execute();
  }
  if ( p()->off_hand_attack && p()->off_hand_attack->execute_event )
  {
    p()->off_hand_attack->cancel();
    p()->off_hand_attack->schedule_execute();
  }
}

template <class Base>
resource_e monk_action_t<Base>::current_resource() const
{
  if ( p()->specialization() == SPEC_NONE )
  {
    return base_t::current_resource();
  }

  resource_e resource_by_stance = _resource_by_stance[ dbc::spec_idx( p()->specialization() ) ];

  if ( resource_by_stance == RESOURCE_MAX )
    return base_t::current_resource();

  return resource_by_stance;
}

// Check if the combo ability under consideration is different from the last
template <class Base>
bool monk_action_t<Base>::is_combo_strike()
{
  if ( !may_combo_strike )
    return false;

  // We don't know if the first attack is a combo or not, so assume it
  // is. If you change this, also change is_combo_break so that it
  // doesn't combo break on the first attack.
  if ( p()->combo_strike_actions.empty() )
    return true;

  if ( p()->combo_strike_actions.back()->id != base_t::id )
    return true;

  return false;
}

// This differs from combo_strike when the ability can't combo strike. In
// that case both is_combo_strike and is_combo_break are false.
template <class Base>
bool monk_action_t<Base>::is_combo_break()
{
  if ( !may_combo_strike )
    return false;

  return !is_combo_strike();
}

// Trigger Windwalker's Combo Strike Mastery, the Hit Combo talent,
// and other effects that trigger from combo strikes.
// Triggers from execute() on abilities with may_combo_strike = true
// Side effect: modifies combo_strike_actions
template <class Base>
void monk_action_t<Base>::combo_strikes_trigger()
{
  if ( !p()->baseline.windwalker.mastery->ok() )
    return;

  if ( is_combo_strike() )
  {
    p()->buff.combo_strikes->trigger();

    p()->buff.hit_combo->trigger();

    if ( p()->talent.windwalker.xuens_bond->ok() )
      p()->cooldown.invoke_xuen->adjust( p()->talent.windwalker.xuens_bond->effectN( 2 ).time_value(),
                                         true );  // Saved as -100

    if ( p()->talent.windwalker.meridian_strikes->ok() )
      p()->cooldown.touch_of_death->adjust(
          -1 * timespan_t::from_seconds( p()->talent.windwalker.meridian_strikes->effectN( 2 ).base_value() / 100 ),
          true );  // Saved as 35

    p()->buff.fury_of_xuen_stacks->trigger();
  }
  else
  {
    p()->combo_strike_actions.clear();
    p()->buff.combo_strikes->expire();
    p()->buff.hit_combo->expire();
  }

  // Record the current action in the history.
  p()->combo_strike_actions.push_back( this );
}

template <class Base>
void monk_action_t<Base>::consume_resource()
{
  base_t::consume_resource();

  if ( !base_t::execute_state )  // Fixes rare crashes at combat_end.
    return;

  auto final_cost = base_t::cost();

  if ( current_resource() == RESOURCE_ENERGY )
  {
    if ( final_cost > 0 )
    {
      if ( p()->talent.shado_pan.flurry_strikes.ok() )
      {
        // Tiger Palm, Keg Smash, and Crackling Jade Lightning contribute their current cost to the accumulator
        if ( base_t::id == p()->baseline.monk.tiger_palm->id() ||
             base_t::id == p()->talent.brewmaster.keg_smash->id() ||
             base_t::id == p()->baseline.monk.crackling_jade_lightning->id() )
          // this needs to be rounded to the nearest whole number
          p()->flurry_strikes_energy += std::lround( final_cost );

        // Detox, Paralysis and Vivify, and Spinning Crane Kick do not count towards Flurry Strikes
        if ( p()->flurry_strikes_energy >= p()->talent.shado_pan.flurry_strikes->effectN( 2 ).base_value() )
        {
          p()->flurry_strikes_energy -= as<int>( p()->talent.shado_pan.flurry_strikes->effectN( 2 ).base_value() );
          p()->active_actions.flurry_strikes->execute();
        }
      }

      if ( p()->talent.shado_pan.efficient_training.ok() )
      {
        p()->efficient_training_energy += as<int>( final_cost );
        if ( p()->efficient_training_energy >= p()->talent.shado_pan.efficient_training->effectN( 3 ).base_value() )
        {
          timespan_t cdr =
              timespan_t::from_millis( -1 * p()->talent.shado_pan.efficient_training->effectN( 4 ).base_value() );
          p()->efficient_training_energy -=
              as<int>( p()->talent.shado_pan.efficient_training->effectN( 3 ).base_value() );
          p()->cooldown.storm_earth_and_fire->adjust( cdr );
          p()->cooldown.weapons_of_order->adjust( cdr );
        }
      }
    }
  }

  if ( current_resource() == RESOURCE_CHI )
  {
    // Dance of Chi-Ji talent triggers from spending chi
    p()->buff.dance_of_chiji->trigger();

    auto base_cost = base_t::base_cost();

    if ( base_cost )
    {
      // This triggers prior to cost reduction
      p()->buff.heart_of_the_jade_serpent_stack_ww->trigger( as<int>( base_cost ) );

      if ( p()->talent.windwalker.spiritual_focus->ok() )
      {
        p()->spiritual_focus_count += base_cost;

        if ( p()->spiritual_focus_count >= p()->talent.windwalker.spiritual_focus->effectN( 1 ).base_value() )
        {
          if ( p()->talent.windwalker.storm_earth_and_fire->ok() )
            p()->cooldown.storm_earth_and_fire->adjust(
                -1 * p()->talent.windwalker.spiritual_focus->effectN( 2 ).time_value() );

          p()->spiritual_focus_count -= p()->talent.windwalker.spiritual_focus->effectN( 1 ).base_value();
        }
      }

      if ( p()->talent.windwalker.drinking_horn_cover->ok() && p()->cooldown.drinking_horn_cover->up() )
      {
        if ( p()->buff.storm_earth_and_fire->up() )
        {
          auto time_extend = p()->talent.windwalker.drinking_horn_cover->effectN( 1 ).percent();
          time_extend *= base_cost;

          p()->buff.storm_earth_and_fire->extend_duration( p(), timespan_t::from_seconds( time_extend ) );

          p()->find_pet( "earth_spirit" )->adjust_duration( timespan_t::from_seconds( time_extend ) );
          p()->find_pet( "fire_spirit" )->adjust_duration( timespan_t::from_seconds( time_extend ) );
        }
        p()->cooldown.drinking_horn_cover->start( p()->talent.windwalker.drinking_horn_cover->internal_cooldown() );
      }
    }

    p()->buff.the_emperors_capacitor->trigger();
  }

  // Chi Savings on Dodge & Parry & Miss
  if ( base_t::last_resource_cost > 0 )
  {
    double chi_restored = base_t::last_resource_cost;
    if ( !base_t::aoe && base_t::result_is_miss( base_t::execute_state->result ) )
      p()->resource_gain( RESOURCE_CHI, chi_restored, p()->gain.chi_refund );
  }

  // Energy refund, estimated at 80%
  if ( current_resource() == RESOURCE_ENERGY && base_t::last_resource_cost > 0 && !base_t::hit_any_target )
  {
    double energy_restored = base_t::last_resource_cost * 0.8;

    p()->resource_gain( RESOURCE_ENERGY, energy_restored, p()->gain.energy_refund );
  }
}

template <class Base>
void monk_action_t<Base>::execute()
{
  if ( p()->specialization() == MONK_MISTWEAVER )
  {
    if ( trigger_chiji && p()->buff.invoke_chiji->up() )
      p()->buff.invoke_chiji_evm->trigger();
  }
  else if ( p()->specialization() == MONK_WINDWALKER )
  {
    if ( may_combo_strike )
      combo_strikes_trigger();
  }

  base_t::execute();

  trigger_storm_earth_and_fire( this );
}

template <class Base>
void monk_action_t<Base>::impact( action_state_t *s )
{
  trigger_mystic_touch( s );

  base_t::impact( s );

  if ( s->result_type == result_amount_type::DMG_DIRECT || s->result_type == result_amount_type::DMG_OVER_TIME )
  {
    p()->trigger_empowered_tiger_lightning( s );

    if ( !base_t::result_is_miss( s->result ) && s->result_amount > 0 )
    {
      if ( p()->talent.shado_pan.flurry_strikes->ok() )
      {
        double damage_contribution = s->result_amount;

        if ( p()->talent.shado_pan.one_versus_many->ok() &&
             ( base_t::data().id() == 117418 || base_t::data().id() == 121253 ) )
          damage_contribution *= ( 1.0f + p()->talent.shado_pan.one_versus_many->effectN( 1 ).percent() );

        p()->flurry_strikes_damage += damage_contribution;

        double ap_threshold = p()->talent.shado_pan.flurry_strikes->effectN( 5 ).percent() *
                              p()->composite_melee_attack_power() * p()->composite_damage_versatility();

        if ( p()->flurry_strikes_damage >= ap_threshold )
        {
          p()->flurry_strikes_damage -= ap_threshold;
          p()->buff.flurry_charge->trigger();
        }
      }

      if ( s->action->id != 451585 && get_td( s->target )->debuff.gale_force->check() &&
           p()->rng().roll( get_td( s->target )->debuff.gale_force->default_chance ) )
      {
        double amount = s->result_amount * get_td( s->target )->debuff.gale_force->data().effectN( 1 ).percent();
        p()->active_actions.gale_force->base_dd_min = p()->active_actions.gale_force->base_dd_max = amount;
        p()->active_actions.gale_force->execute_on_target( s->target );
      }
    }

    // TWW1 Windwalker 2PC
    if ( p()->buff.tiger_strikes->up() && base_t::data().affected_by( p()->buff.tiger_strikes->data().effectN( 1 ) ) )
      p()->buff.tiger_strikes->decrement();
  }
}

template <class Base>
void monk_action_t<Base>::tick( dot_t *dot )
{
  base_t::tick( dot );

  if ( !base_t::result_is_miss( dot->state->result ) && dot->state->result_type == result_amount_type::DMG_OVER_TIME )
    p()->trigger_empowered_tiger_lightning( dot->state );
}

template <class Base>
void monk_action_t<Base>::trigger_storm_earth_and_fire( const action_t *a )
{
  p()->trigger_storm_earth_and_fire( a, sef_ability, ( may_combo_strike && p()->buff.combo_strikes->check() ) );
}

template <class Base>
void monk_action_t<Base>::trigger_mystic_touch( action_state_t *s )
{
  if ( base_t::sim->overrides.mystic_touch )
    return;

  if ( base_t::result_is_miss( s->result ) || s->result_amount == 0.0 )
    return;

  if ( s->target->debuffs.mystic_touch && p()->baseline.monk.mystic_touch->ok() )
    s->target->debuffs.mystic_touch->trigger();
}

monk_spell_t::monk_spell_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_action_t<spell_t>( name, player, spell_data )
{
  ap_type = attack_power_type::WEAPON_MAINHAND;
}

monk_heal_t::monk_heal_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_action_t<heal_t>( name, player, spell_data )
{
  harmful = false;
  ap_type = attack_power_type::WEAPON_MAINHAND;
}

double monk_heal_t::action_multiplier() const
{
  double am = base_t::action_multiplier();

  player_t *t = ( execute_state ) ? execute_state->target : target;

  switch ( p()->specialization() )
  {
    case MONK_MISTWEAVER:

      if ( auto td = this->get_td( t ) )  // Use get_td since we can have a ticking dot without target-data
      {
        if ( td->dot.enveloping_mist->is_ticking() )
        {
          if ( p()->talent.mistweaver.mist_wrap->ok() )
            am *= 1.0 + p()->talent.mistweaver.enveloping_mist->effectN( 2 ).percent() +
                  p()->talent.mistweaver.mist_wrap->effectN( 2 ).percent();
          else
            am *= 1.0 + p()->talent.mistweaver.enveloping_mist->effectN( 2 ).percent();
        }
      }

      if ( p()->buff.life_cocoon->check() )
        am *= 1.0 + p()->talent.mistweaver.life_cocoon->effectN( 2 ).percent();

      break;

    case MONK_WINDWALKER:

      break;

    case MONK_BREWMASTER:

      break;

    default:
      assert( 0 );
      break;
  }

  return am;
}

monk_absorb_t::monk_absorb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_action_t<absorb_t>( name, player, spell_data )
{
}

monk_melee_attack_t::monk_melee_attack_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_action_t<melee_attack_t>( name, player, spell_data )
{
  special    = true;
  may_glance = false;
}

// Physical tick_action abilities need amount_type() override, so the
// tick_action are properly physically mitigated.
result_amount_type monk_melee_attack_t::amount_type( const action_state_t *state, bool periodic ) const
{
  if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
  {
    return result_amount_type::DMG_DIRECT;
  }
  else
  {
    return base_t::amount_type( state, periodic );
  }
}

monk_buff_t::monk_buff_t( monk_t *player, std::string_view name, const spell_data_t *spell_data, const item_t *item )
  : buff_t( player, name, spell_data, item )
{
}

monk_buff_t::monk_buff_t( monk_td_t *target_data, std::string_view name, const spell_data_t *spell_data,
                          const item_t *item )
  : buff_t( *target_data, name, spell_data, item )
{
}

monk_td_t &monk_buff_t::get_td( player_t *t )
{
  return *( p().get_target_data( t ) );
}

const monk_td_t *monk_buff_t::find_td( player_t *t ) const
{
  return p().find_target_data( t );
}

monk_t &monk_buff_t::p()
{
  return *debug_cast<monk_t *>( buff_t::source );
}

const monk_t &monk_buff_t::p() const
{
  return *debug_cast<monk_t *>( buff_t::source );
}

summon_pet_t::summon_pet_t( monk_t *player, std::string_view name, std::string_view pet_name,
                            const spell_data_t *spell_data )
  : monk_spell_t( player, name, spell_data ),
    summoning_duration( timespan_t::zero() ),
    pet_name( pet_name ),
    pet( nullptr )
{
  harmful = false;
}

void summon_pet_t::init_finished()
{
  pet = player->find_pet( pet_name );
  if ( !pet )
    background = true;

  monk_spell_t::init_finished();
}

void summon_pet_t::execute()
{
  pet->summon( summoning_duration );

  monk_spell_t::execute();
}

bool summon_pet_t::ready()
{
  if ( !pet )
    return false;
  return monk_spell_t::ready();
}

struct monk_snapshot_stats_t : public snapshot_stats_t
{
  monk_snapshot_stats_t( monk_t *player, util::string_view options ) : snapshot_stats_t( player, options )
  {
  }

  void execute() override
  {
    snapshot_stats_t::execute();

    // auto *monk                                              = debug_cast<monk_t *>( player );
    // monk->stagger->sample_data->buffed_base_value           = monk->stagger->base_value();
    // monk->stagger->sample_data->buffed_percent_player_level = monk->stagger->percent( monk->level() );
    // monk->stagger->sample_data->buffed_percent_target_level = monk->stagger->percent( target->level() );
  }
};

namespace pet_summon
{
// ==========================================================================
// Storm, Earth, and Fire
// ==========================================================================

struct storm_earth_and_fire_t : public monk_spell_t
{
  storm_earth_and_fire_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "storm_earth_and_fire", p->talent.windwalker.storm_earth_and_fire )
  {
    parse_options( options_str );

    cast_during_sck  = false;
    trigger_gcd      = timespan_t::zero();
    may_combo_strike = true;
    callbacks = harmful = may_miss = may_crit = may_dodge = may_parry = may_block = false;
  }

  bool ready() override
  {
    if ( p()->buff.storm_earth_and_fire->check() )
      return false;

    return monk_spell_t::ready();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->summon_storm_earth_and_fire( data().duration() );

    if ( p()->talent.windwalker.ordered_elements.ok() )
    {
      p()->cooldown.rising_sun_kick->reset( true );
      p()->resource_gain( RESOURCE_CHI, p()->talent.windwalker.ordered_elements->effectN( 2 ).base_value(),
                          p()->gain.ordered_elements );
    }
  }
};

struct storm_earth_and_fire_fixate_t : public monk_spell_t
{
  storm_earth_and_fire_fixate_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "storm_earth_and_fire_fixate",
                    p->find_spell( (int)p->talent.windwalker.storm_earth_and_fire->effectN( 5 ).base_value() ) )
  {
    parse_options( options_str );

    cast_during_sck    = true;
    callbacks          = false;
    trigger_gcd        = timespan_t::zero();
    cooldown->duration = timespan_t::zero();
  }

  bool target_ready( player_t *target ) override
  {
    if ( !p()->storm_earth_and_fire_fixate_ready( target ) )
      return false;

    return monk_spell_t::target_ready( target );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->storm_earth_and_fire_fixate( target );
  }
};

}  // namespace pet_summon

namespace attacks
{

// ==========================================================================
// Windwalking Aura Toggle
// ==========================================================================

struct windwalking_aura_t : public monk_spell_t
{
  windwalking_aura_t( monk_t *player ) : monk_spell_t( player, "windwalking_aura_toggle" )
  {
    harmful     = false;
    background  = true;
    trigger_gcd = timespan_t::zero();
  }

  size_t available_targets( std::vector<player_t *> &tl ) const override
  {
    tl.clear();

    for ( auto t : sim->player_non_sleeping_list )
    {
      tl.push_back( t );
    }

    return tl.size();
  }

  std::vector<player_t *> &check_distance_targeting( std::vector<player_t *> &tl ) const override
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t *target_to_buff = tl[ i ];

      if ( p()->get_player_distance( *target_to_buff ) > 10.0 )
        tl.erase( tl.begin() + i );
    }

    return tl;
  }
};

// ==========================================================================
// Flurry Strikes
// ==========================================================================
struct flurry_strikes_t : public monk_melee_attack_t
{
  struct high_impact_t : public monk_spell_t
  {
    high_impact_t( monk_t *p )
      : monk_spell_t( p, "high_impact", p->talent.shado_pan.high_impact_debuff->effectN( 1 ).trigger() )  // 451039
    {
      aoe        = -1;
      background = dual = true;
    }
  };

  struct flurry_strike_wisdom_t : public monk_spell_t
  {
    flurry_strike_wisdom_t( monk_t *p )
      : monk_spell_t( p, "flurry_strike_wisdom", p->talent.shado_pan.wisdom_of_the_wall_flurry )
    {
      aoe        = -1;
      background = dual = true;

      name_str_reporting = "flurry_strike_wisdom_of_the_wall";
    }
  };

  struct flurry_strike_t : public monk_melee_attack_t
  {
    enum wisdom_buff_e
    {
      WISDOM_OF_THE_WALL_CRIT,
      WISDOM_OF_THE_WALL_DODGE,
      WISDOM_OF_THE_WALL_FLURRY,
      WISDOM_OF_THE_WALL_MASTERY
    };

    int flurry_strikes_counter;
    int flurry_strikes_threshold;
    shuffled_rng_t *deck;
    flurry_strike_wisdom_t *wisdom_flurry;

    flurry_strike_t( monk_t *p )
      : monk_melee_attack_t( p, "flurry_strike", p->talent.shado_pan.flurry_strikes_hit ),
        flurry_strikes_counter( 0 ),
        flurry_strikes_threshold( as<int>( p->talent.shado_pan.wisdom_of_the_wall->effectN( 1 ).base_value() ) ),
        deck( p->get_shuffled_rng( "wisdom_of_the_wall", { { WISDOM_OF_THE_WALL_CRIT, 1 },
                                                           { WISDOM_OF_THE_WALL_DODGE, 1 },
                                                           { WISDOM_OF_THE_WALL_FLURRY, 1 },
                                                           { WISDOM_OF_THE_WALL_MASTERY, 1 } } ) )
    {
      background = dual = true;

      apply_affecting_aura( p->talent.shado_pan.pride_of_pandaria );

      wisdom_flurry = new flurry_strike_wisdom_t( p );
      add_child( wisdom_flurry );
    }

    void impact( action_state_t *s ) override
    {
      monk_melee_attack_t::impact( s );

      if ( p()->talent.shado_pan.wisdom_of_the_wall->ok() )
      {
        flurry_strikes_counter++;

        if ( flurry_strikes_counter >= flurry_strikes_threshold )
        {
          flurry_strikes_counter -= flurry_strikes_threshold;

          // Draw new card
          const auto card = wisdom_buff_e( deck->trigger() );
          switch ( card )
          {
            case WISDOM_OF_THE_WALL_CRIT:
              p()->buff.wisdom_of_the_wall_crit->trigger();
              break;
            case WISDOM_OF_THE_WALL_DODGE:
              p()->buff.wisdom_of_the_wall_dodge->trigger();
              break;
            case WISDOM_OF_THE_WALL_FLURRY:
              p()->buff.wisdom_of_the_wall_flurry->trigger();
              break;
            case WISDOM_OF_THE_WALL_MASTERY:
              p()->buff.wisdom_of_the_wall_mastery->trigger();
              break;
            default:
              break;
          }
        }
      }

      p()->buff.against_all_odds->trigger();

      if ( auto target_data = p()->get_target_data( s->target ); target_data )
        target_data->debuff.high_impact->trigger();

      if ( p()->buff.wisdom_of_the_wall_flurry->up() )
      {
        wisdom_flurry->set_target( s->target );
        wisdom_flurry->execute();
      }
    }
  };

  flurry_strike_t *strike;
  high_impact_t *high_impact;

  flurry_strikes_t( monk_t *p ) : monk_melee_attack_t( p, "flurry_strikes", p->talent.shado_pan.flurry_strikes )
  {
    strike = new flurry_strike_t( p );
    add_child( strike );

    if ( !p->talent.shado_pan.high_impact->ok() )
      return;

    high_impact = new high_impact_t( p );
    add_child( high_impact );
    p->register_on_kill_callback( [ this, p ]( player_t *target ) {
      if ( p->sim->event_mgr.canceled )
        return;

      if ( auto target_data = p->get_target_data( target ); target_data && target_data->debuff.high_impact->up() )
        high_impact->execute_on_target( target );
    } );
  }

  void execute() override
  {
    // 150ms of delay between executes has been observed, with some small amount of jitter
    if ( p()->buff.flurry_charge->up() )
      for ( int charge = 1; charge <= p()->buff.flurry_charge->stack(); charge++ )
        make_event<events::delayed_execute_event_t>( *sim, p(), strike, p()->target, charge * 150_ms );

    p()->buff.flurry_charge->expire();
    p()->buff.vigilant_watch->expire();
  }
};

// ==========================================================================
// Overwhelming Force
// ==========================================================================

template <class base_action_t>
struct overwhelming_force_t : base_action_t
{
  using base_t = overwhelming_force_t<base_action_t>;
  struct damage_t : monk_spell_t
  {
    damage_t( monk_t *player, std::string_view name )
      : monk_spell_t( player, fmt::format( "overwhelming_force_{}", name ),
                      player->talent.master_of_harmony.overwhelming_force_damage )
    {
      background = dual = proc = true;
      base_multiplier          = player->talent.master_of_harmony.overwhelming_force->effectN( 1 ).percent();
      reduced_aoe_targets      = player->talent.master_of_harmony.overwhelming_force->effectN( 2 ).base_value();
    }

    void init() override
    {
      monk_spell_t::init();
      update_flags = snapshot_flags &= STATE_NO_MULTIPLIER | STATE_MUL_SPELL_DA;
    }
  };

  damage_t *overwhelming_force_damage;

  template <typename... Args>
  overwhelming_force_t( monk_t *player, Args &&...args )
    : base_action_t( player, std::forward<Args>( args )... ), overwhelming_force_damage( nullptr )
  {
    if ( !player->talent.master_of_harmony.overwhelming_force->ok() )
      return;

    overwhelming_force_damage = new damage_t( player, base_action_t::name_str );
    base_action_t::add_child( overwhelming_force_damage );
  }

  void impact( action_state_t *state ) override
  {
    base_action_t::impact( state );

    if ( !base_action_t::p()->talent.master_of_harmony.overwhelming_force->ok() || state->chain_target > 0 )
      return;

    /*
     * If the triggering hit is a crit, the damage is divided by the crit bonus
     * multiplier, and then multiplied by 2.0 (or the context base crit bonus?)
     *
     * E.g.
     * Base Damage (Crit) 64286, Crit Bonus Multiplier 2.02
     * Base Damage (Pre-Crit) 64286 / 2.02 ~ 31825
     * Overwhelming Force Damage 31825 * 0.15 * 2 = ~9547
     */
    double amount = state->result_amount;
    if ( state->result == RESULT_CRIT && base_action_t::p()->bugs )
    {
      amount /= 1.0 + state->result_crit_bonus;
      amount *= 2.0;
    }
    overwhelming_force_damage->base_dd_min = overwhelming_force_damage->base_dd_max = amount;
    overwhelming_force_damage->execute();
  }
};

// ==========================================================================
// Tiger Palm
// ==========================================================================

// Tiger's Ferocity ( Windwalker TWW1 4PC )
struct tigers_ferocity_t : public monk_melee_attack_t
{
  std::vector<player_t *> &t_list;

  tigers_ferocity_t( monk_t *p )
    : monk_melee_attack_t( p, "tigers_ferocity", p->tier.tww1.ww_4pc_dmg ), t_list( target_cache.list )
  {
    background = dual   = true;
    aoe                 = -1;
    reduced_aoe_targets = p->tier.tww1.ww_4pc->effectN( 2 ).base_value();
  }

  std::vector<player_t *> &target_list() const override
  {
    // The player's target is not hit by this ability so we need to modify the target list.
    t_list = base_t::target_list();
    t_list.erase( std::remove( t_list.begin(), t_list.end(), player->target ), t_list.end() );
    return t_list;
  }
};

// Tiger Palm base ability ===================================================
struct tiger_palm_t : public overwhelming_force_t<monk_melee_attack_t>
{
  bool face_palm;
  action_t *tigers_ferocity;

  tiger_palm_t( monk_t *p, util::string_view options_str )
    : base_t( p, "tiger_palm", p->baseline.monk.tiger_palm ), face_palm( false )
  {
    parse_options( options_str );

    // allow Darting Hurricane to reduce GCD all the way down to 500ms
    min_gcd          = 500_ms;
    ww_mastery       = true;
    may_combo_strike = true;
    trigger_chiji    = true;
    sef_ability      = actions::sef_ability_e::SEF_TIGER_PALM;
    cast_during_sck  = player->specialization() != MONK_WINDWALKER;

    if ( p->specialization() == MONK_WINDWALKER )
      energize_amount = p->baseline.windwalker.aura->effectN( 4 ).base_value();
    else
      energize_type = action_energize::NONE;

    spell_power_mod.direct = 0.0;

    apply_affecting_aura( p->talent.windwalker.touch_of_the_tiger );
    apply_affecting_aura( p->talent.windwalker.inner_peace );

    if ( const auto &effect = p->talent.brewmaster.face_palm->effectN( 2 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( [ & ] { return face_palm; } )
          .set_value( effect.percent() - 1.0 )
          .set_eff( &effect );
    parse_effects( p->buff.combat_wisdom );
    parse_effects( p->buff.martial_mixture );
    parse_effects( p->buff.darting_hurricane );

    if ( p->sets->has_set_bonus( MONK_WINDWALKER, TWW1, B4 ) )
    {
      tigers_ferocity = new tigers_ferocity_t( p );
      add_child( tigers_ferocity );
    }
  }

  bool ready() override
  {
    if ( p()->talent.brewmaster.press_the_advantage->ok() )
      return false;
    return monk_melee_attack_t::ready();
  }

  void execute() override
  {
    //============
    // Pre-Execute
    //============

    if ( ( face_palm = rng().roll( p()->talent.brewmaster.face_palm->effectN( 1 ).percent() ) ) )
      p()->proc.face_palm->occur();

    if ( p()->buff.blackout_combo->up() )
      p()->proc.blackout_combo_tiger_palm->occur();

    if ( p()->buff.counterstrike->up() )
      p()->proc.counterstrike_tp->occur();

    //------------

    monk_melee_attack_t::execute();

    p()->buff.blackout_combo->expire();

    if ( result_is_miss( execute_state->result ) )
      return;

    //-----------

    //============
    // Post-hit
    //============

    p()->buff.teachings_of_the_monastery->trigger();

    // Combo Breaker calculation
    if ( p()->baseline.windwalker.combo_breaker->ok() && p()->buff.bok_proc->trigger() &&
         p()->buff.storm_earth_and_fire->up() )
    {
      p()->trigger_storm_earth_and_fire_bok_proc( pets::sef_pet_e::SEF_FIRE );
      p()->trigger_storm_earth_and_fire_bok_proc( pets::sef_pet_e::SEF_EARTH );
    }

    // Reduces the remaining cooldown on your Brews by 1 sec
    p()->baseline.brewmaster.brews.adjust(
        timespan_t::from_seconds( p()->baseline.monk.tiger_palm->effectN( 3 ).base_value() ) );

    if ( face_palm )
      p()->baseline.brewmaster.brews.adjust( p()->talent.brewmaster.face_palm->effectN( 3 ).time_value() );

    if ( p()->buff.combat_wisdom->up() )
    {
      p()->passive_actions.combat_wisdom_eh->execute();
      p()->buff.combat_wisdom->expire();
    }

    p()->buff.darting_hurricane->decrement();

    // T33 Windwalker Set Bonus
    p()->buff.tiger_strikes->trigger();
    p()->buff.tigers_ferocity->expire();

    p()->buff.martial_mixture->expire();
  }

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    // Apply Mark of the Crane
    p()->trigger_mark_of_the_crane( s );

    if ( p()->sets->has_set_bonus( MONK_WINDWALKER, TWW1, B4 ) )
    {
      double damage = s->result_amount;

      if ( p()->buff.storm_earth_and_fire->up() )
      {
        // Damage during SEF is based on the actor's damage before the SEF modifier.
        damage /= ( 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent() );

        // Tested 09/08/2024. Tiger's Ferocity deals additional damage during SEF.
        damage *= ( 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent() ) * 3;
      }

      damage *= p()->tier.tww1.ww_4pc->effectN( 1 ).percent();

      tigers_ferocity->base_dd_min = tigers_ferocity->base_dd_max = damage;
      tigers_ferocity->execute_on_target( s->target );
    }
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

// Glory of the Dawn =================================================
struct glory_of_the_dawn_t : public monk_melee_attack_t
{
  glory_of_the_dawn_t( monk_t *p, const std::string &name )
    : monk_melee_attack_t( p, name, p->passives.glory_of_the_dawn_damage )
  {
    background  = true;
    ww_mastery  = true;
    sef_ability = actions::sef_ability_e::SEF_GLORY_OF_THE_DAWN;
  }

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->talent.windwalker.acclamation.ok() )
      get_td( s->target )->debuff.acclamation->trigger();

    if ( p()->talent.windwalker.xuens_battlegear->ok() && ( s->result == RESULT_CRIT ) )
    {
      p()->cooldown.fists_of_fury->adjust( -1 * p()->talent.windwalker.xuens_battlegear->effectN( 2 ).time_value(),
                                           true );
      p()->proc.xuens_battlegear_reduction->occur();
    }
  }
};

// Rising Sun Kick Damage Trigger ===========================================

template <class base_action_t>
struct press_the_advantage_t : base_action_t
{
  using base_t = press_the_advantage_t<base_action_t>;
  struct damage_t : base_action_t
  {
    const double mod;
    bool face_palm;

    damage_t( monk_t *player, std::string_view name )
      : base_action_t( player, {}, name ),
        mod( 1.0 - player->talent.brewmaster.press_the_advantage->effectN( 3 ).percent() ),
        face_palm( false )
    {
      base_action_t::proc        = true;
      base_action_t::trigger_gcd = 0_s;
      base_action_t::background  = true;
      base_action_t::dual        = true;

      base_action_t::parse_effects( player->buff.counterstrike,
                                    affect_list_t( 1 ).add_spell( base_action_t::data().id() ),
                                    player->buff.counterstrike->data().effectN( 1 ).percent() * mod );
      base_action_t::parse_effects( player->buff.blackout_combo,
                                    affect_list_t( 1 ).add_spell( base_action_t::data().id() ),
                                    player->buff.blackout_combo->data().effectN( 1 ).percent() * mod );

      // effect must still be rolled in execute so it triggers brew cdr
      if ( const auto &effect = player->talent.brewmaster.face_palm->effectN( 2 ); effect.ok() )
        add_parse_entry( base_action_t::da_multiplier_effects )
            .set_func( [ & ] { return face_palm; } )
            .set_value( ( effect.percent() - 1.0 ) * mod )
            .set_eff( &effect );
    }

    void init_finished() override
    {
      base_action_t::init_finished();

      if ( action_t *pta = base_action_t::p()->find_action( "press_the_advantage" );
           pta && base_action_t::p()->talent.brewmaster.press_the_advantage->ok() )
        pta->add_child( this );
    }

    void execute() override
    {
      base_action_t::p()->buff.press_the_advantage->expire();

      if ( ( face_palm = base_action_t::rng().roll(
                 base_action_t::p()->talent.brewmaster.face_palm->effectN( 1 ).percent() ) ) )
      {
        base_action_t::p()->proc.face_palm->occur();
        base_action_t::p()->baseline.brewmaster.brews.adjust(
            base_action_t::p()->talent.brewmaster.face_palm->effectN( 3 ).time_value() );
      }

      base_action_t::execute();

      base_action_t::p()->buff.blackout_combo->expire();

      if ( base_action_t::p()->talent.brewmaster.chi_surge->ok() )
        base_action_t::p()->active_actions.chi_surge->execute();

      if ( base_action_t::p()->talent.brewmaster.call_to_arms->ok() && base_action_t::rng().roll( 0.3 ) )
        base_action_t::p()->active_actions.niuzao_call_to_arms_summon->execute();
    }
  };

  propagate_const<damage_t *> press_the_advantage_action;
  propagate_const<proc_t *> press_the_advantage_proc;

  template <typename... Args>
  press_the_advantage_t( monk_t *player, Args &&...args )
    : base_action_t( player, std::forward<Args>( args )... ), press_the_advantage_action( nullptr )
  {
    if ( !player->talent.brewmaster.press_the_advantage->ok() )
      return;

    press_the_advantage_action =
        new damage_t( player, fmt::format( "{}_press_the_advantage", base_action_t::name_str ) );
    press_the_advantage_proc = player->get_proc( fmt::format( "{} - Press The Advantage", base_action_t::name_str ) );
  }

  void impact( action_state_t *state ) override
  {
    base_action_t::impact( state );

    if ( base_action_t::p()->buff.press_the_advantage->stack() != 10 )
      return;

    // TODO: Schedule execute with the appropriate delay.
    base_action_t::p()->buff.press_the_advantage->expire();
    press_the_advantage_proc->occur();
    press_the_advantage_action->execute();
  }
};

struct rising_sun_kick_dmg_t : public overwhelming_force_t<monk_melee_attack_t>
{
  rising_sun_kick_dmg_t( monk_t *p, std::string_view /* options_str */,
                         std::string_view name = "rising_sun_kick_damage" )
    : base_t( p, name, p->talent.monk.rising_sun_kick->effectN( 1 ).trigger() )
  {
    ww_mastery = true;

    if ( p->specialization() == MONK_WINDWALKER )
      ap_type = attack_power_type::WEAPON_BOTH;

    background = dual = true;
    may_crit          = true;
    trigger_chiji     = true;
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->buff.thunder_focus_tea->up() )
    {
      p()->cooldown.rising_sun_kick->adjust( p()->talent.mistweaver.thunder_focus_tea->effectN( 1 ).time_value(),
                                             true );

      p()->buff.thunder_focus_tea->decrement();
    }

    if ( p()->talent.brewmaster.strike_at_dawn->ok() )
      p()->buff.elusive_brawler->trigger();

    if ( p()->talent.brewmaster.black_ox_adept->ok() )
      p()->buff.ox_stance->trigger();

    // Brewmaster RSK also applies the WoO debuff.
    if ( p()->buff.weapons_of_order->up() )
      for ( const auto &target : target_list() )
        get_td( target )->debuff.weapons_of_order->trigger();
  }

  void impact( action_state_t *s ) override
  {
    base_t::impact( s );

    p()->buff.transfer_the_power->trigger();

    if ( p()->talent.windwalker.xuens_battlegear->ok() && ( s->result == RESULT_CRIT ) )
    {
      p()->cooldown.fists_of_fury->adjust( -1 * p()->talent.windwalker.xuens_battlegear->effectN( 2 ).time_value(),
                                           true );
      p()->proc.xuens_battlegear_reduction->occur();
    }

    if ( p()->baseline.windwalker.combat_conditioning->ok() )
      s->target->debuffs.mortal_wounds->trigger();

    // Apply Mark of the Crane
    p()->trigger_mark_of_the_crane( s );

    if ( p()->talent.windwalker.acclamation.ok() )
      get_td( s->target )->debuff.acclamation->trigger();
  }
};

struct rising_sun_kick_t : public monk_melee_attack_t
{
  glory_of_the_dawn_t *gotd;

  rising_sun_kick_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "rising_sun_kick", p->talent.monk.rising_sun_kick )
  {
    parse_options( options_str );

    may_combo_strike = true;
    sef_ability      = actions::sef_ability_e::SEF_RISING_SUN_KICK;
    ap_type          = attack_power_type::NONE;
    cast_during_sck  = player->specialization() != MONK_WINDWALKER;

    attack_power_mod.direct = 0;

    execute_action        = new press_the_advantage_t<rising_sun_kick_dmg_t>( p, options_str );
    execute_action->stats = stats;

    if ( p->talent.windwalker.glory_of_the_dawn->ok() )
    {
      gotd = new glory_of_the_dawn_t( p, "glory_of_the_dawn" );
      add_child( gotd );
    }
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    // TODO: Is this the correct way to get character sheet haste %?
    auto gotd_chance = p()->talent.windwalker.glory_of_the_dawn->effectN( 2 ).percent() *
                       ( ( 1.0 / p()->composite_spell_haste() ) - 1.0 );

    if ( rng().roll( gotd_chance ) )
      gotd->execute_on_target( this->target );

    p()->buff.whirling_dragon_punch->trigger();

    p()->active_actions.chi_wave->execute();

    if ( p()->buff.storm_earth_and_fire->up() && p()->talent.windwalker.ordered_elements->ok() )
      p()->buff.ordered_elements->trigger();

    p()->buff.tigers_ferocity->trigger();

    p()->buff.august_dynasty->expire();
  }
};

// ==========================================================================
// Blackout Kick
// ==========================================================================

// Blackout Kick Proc from Teachings of the Monastery =======================
struct blackout_kick_totm_proc_t : public monk_melee_attack_t
{
  blackout_kick_totm_proc_t( monk_t *p )
    : monk_melee_attack_t( p, "blackout_kick_totm_proc", p->talent.windwalker.teachings_of_the_monastery_blackout_kick )
  {
    sef_ability        = actions::sef_ability_e::SEF_BLACKOUT_KICK_TOTM;
    ww_mastery         = false;
    cooldown->duration = timespan_t::zero();
    background = dual = true;
    trigger_chiji     = true;
    trigger_gcd       = timespan_t::zero();
  }

  void init_finished() override
  {
    monk_melee_attack_t::init_finished();
    action_t *bok = player->find_action( "blackout_kick" );
    if ( bok )
    {
      attack_power_mod = bok->attack_power_mod;
      bok->add_child( this );
    }
  }

  double composite_target_multiplier( player_t *target ) const override
  {
    double m = base_t::composite_target_multiplier( target );

    if ( target != p()->target && p()->talent.windwalker.shadowboxing_treads->ok() )
      m *= p()->talent.windwalker.shadowboxing_treads->effectN( 3 ).percent();

    return m;
  }

  // Force 100 milliseconds for the animation, but not delay the overall GCD
  timespan_t execute_time() const override
  {
    return timespan_t::from_millis( 100 );
  }

  double cost() const override
  {
    return 0;
  }

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    // The initial hit along with each individual TotM hits has a chance to reset the cooldown
    auto totmResetChance = p()->shared.teachings_of_the_monastery->effectN( 1 ).percent();

    if ( p()->specialization() == MONK_MISTWEAVER )
      totmResetChance += p()->baseline.mistweaver.aura->effectN( 21 ).percent();

    if ( rng().roll( totmResetChance ) )
    {
      p()->cooldown.rising_sun_kick->reset( true );
      p()->proc.rsk_reset_totm->occur();
    }

    // Mark of the Crane is only triggered on the initial target
    if ( s->chain_target == 0 )
      p()->trigger_mark_of_the_crane( s );

    // Martial Mixture triggers from each ToTM impact
    p()->buff.martial_mixture->trigger();
  }
};

// Charred Passions ============================================================
template <class base_action_t>
struct charred_passions_t : base_action_t
{
  using base_t = charred_passions_t<base_action_t>;
  struct damage_t : monk_spell_t
  {
    damage_t( monk_t *player, std::string_view name )
      : monk_spell_t( player, fmt::format( "charred_passions_{}", name ), player->talent.brewmaster.charred_passions )
    {
      background = dual = proc = true;
      may_crit                 = false;
      base_multiplier          = data().effectN( 1 ).percent();
    }

    void init() override
    {
      monk_spell_t::init();
      update_flags = snapshot_flags = STATE_NO_MULTIPLIER | STATE_MUL_SPELL_DA;
    }
  };

  damage_t *chp_damage;
  cooldown_t *chp_cooldown;

  template <typename... Args>
  charred_passions_t( monk_t *player, Args &&...args ) : base_action_t( player, std::forward<Args>( args )... )
  {
    if ( !player->talent.brewmaster.charred_passions->ok() )
      return;

    chp_cooldown = player->get_cooldown( "charred_passions" );
    chp_damage   = new damage_t( player, base_action_t::name_str );
    // TODO: Have a more resilient way to re-map stats objects.
    // Issue: When SCK tick stats replace the action stats of SCK channel, adding
    // a child of SCK tick breaks reporting.
    // base_action_t::add_child( damage );
  }

  void impact( action_state_t *state ) override
  {
    base_action_t::impact( state );

    if ( !base_action_t::p()->buff.charred_passions->up() )
      return;

    base_action_t::p()->proc.charred_passions->occur();
    chp_damage->base_dd_min = chp_damage->base_dd_max = state->result_amount;
    chp_damage->execute();

    if ( monk_td_t *target_data = base_action_t::get_td( state->target );
         target_data && target_data->dot.breath_of_fire->is_ticking() && chp_cooldown->up() )
    {
      target_data->dot.breath_of_fire->refresh_duration();
      chp_cooldown->start( chp_damage->data().effectN( 1 ).trigger()->internal_cooldown() );
    }
  }
};

// Blackout Kick Baseline ability =======================================
struct blackout_kick_t : overwhelming_force_t<charred_passions_t<monk_melee_attack_t>>
{
  blackout_kick_totm_proc_t *bok_totm_proc;
  cooldown_t *keg_smash_cooldown;

  blackout_kick_t( monk_t *p, util::string_view options_str )
    : base_t( p, "blackout_kick",
              ( p->specialization() == MONK_BREWMASTER ? p->baseline.brewmaster.blackout_kick
                                                       : p->baseline.monk.blackout_kick ) ),
      keg_smash_cooldown( nullptr )
  {
    parse_options( options_str );
    if ( p->specialization() == MONK_WINDWALKER )
      ap_type = attack_power_type::WEAPON_BOTH;

    sef_ability      = actions::sef_ability_e::SEF_BLACKOUT_KICK;
    ww_mastery       = true;
    may_combo_strike = true;
    trigger_chiji    = true;
    cast_during_sck  = p->specialization() != MONK_WINDWALKER;

    apply_affecting_aura( p->talent.brewmaster.fluidity_of_motion );
    apply_affecting_aura( p->talent.brewmaster.shadowboxing_treads );
    apply_affecting_aura( p->talent.brewmaster.elusive_footwork );

    if ( player->sets->set( MONK_BREWMASTER, TWW1, B4 )->ok() )
      keg_smash_cooldown = player->get_cooldown( "keg_smash" );

    if ( p->talent.brewmaster.charred_passions->ok() )
      add_child( base_t::chp_damage );

    if ( p->shared.teachings_of_the_monastery->ok() )
    {
      bok_totm_proc = new blackout_kick_totm_proc_t( p );
      add_child( bok_totm_proc );
    }

    if ( p->baseline.windwalker.blackout_kick_rank_2->ok() )
      base_costs[ RESOURCE_CHI ].base +=
          p->baseline.windwalker.blackout_kick_rank_2->effectN( 1 ).base_value();  // Reduce base from 3 chi to 1
  }

  double composite_target_multiplier( player_t *target ) const override
  {
    double m = base_t::composite_target_multiplier( target );

    if ( target != p()->target && p()->talent.windwalker.shadowboxing_treads->ok() )
      m *= p()->talent.windwalker.shadowboxing_treads->effectN( 3 ).percent();

    return m;
  }

  void consume_resource() override
  {
    base_t::consume_resource();

    // Register how much chi is saved without actually refunding the chi
    if ( p()->buff.bok_proc->up() )
      p()->gain.bok_proc->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
  }

  void execute() override
  {
    base_t::execute();

    p()->buff.shuffle->trigger( timespan_t::from_seconds( p()->talent.brewmaster.shuffle->effectN( 1 ).base_value() ) );

    p()->buff.flow_of_battle_damage->trigger();
    // 08-18-2024: Sampling of a large number of logs strongly suggests a proc rate of 0.33.
    // Reproducible via running https://github.com/renanthera/crunch/tree/ec850f8b37b922f177d88b0c1626271a382ce771
    if ( keg_smash_cooldown && p()->sets->set( MONK_BREWMASTER, TWW1, B4 )->ok() && p()->rng().roll( 0.33 ) )
    {
      keg_smash_cooldown->reset( false );
      p()->buff.flow_of_battle_free_keg_smash->trigger();
    }

    if ( result_is_hit( execute_state->result ) )
    {
      if ( p()->buff.bok_proc->up() )
      {
        if ( p()->rng().roll( p()->talent.windwalker.energy_burst->effectN( 1 ).percent() ) )
          p()->resource_gain( RESOURCE_CHI, p()->talent.windwalker.energy_burst->effectN( 2 ).base_value(),
                              p()->gain.energy_burst );

        p()->buff.bok_proc->decrement();
      }

      p()->buff.blackout_combo->trigger();

      if ( p()->baseline.windwalker.blackout_kick_rank_3->ok() )
      {
        // Reduce the cooldown of Rising Sun Kick and Fists of Fury
        timespan_t cd_reduction = -1 * p()->baseline.monk.blackout_kick->effectN( 3 ).time_value();

        if ( p()->buff.storm_earth_and_fire->up() && p()->talent.windwalker.ordered_elements->ok() )
        {
          cd_reduction += ( -1 * p()->talent.windwalker.ordered_elements->effectN( 1 ).time_value() );
          p()->proc.blackout_kick_cdr_oe->occur();
        }
        else
          p()->proc.blackout_kick_cdr->occur();

        p()->cooldown.rising_sun_kick->adjust( cd_reduction, true );
        p()->cooldown.fists_of_fury->adjust( cd_reduction, true );
      }

      p()->buff.transfer_the_power->trigger();

      if ( p()->buff.teachings_of_the_monastery->up() )
      {
        p()->buff.teachings_of_the_monastery->expire();

        if ( p()->rng().roll( p()->talent.conduit_of_the_celestials.xuens_guidance->effectN( 1 ).percent() ) )
          p()->buff.teachings_of_the_monastery->trigger();
      }

      if ( p()->specialization() == MONK_WINDWALKER )
      {
        p()->buff.strength_of_the_black_ox->expire();
        p()->buff.inner_compass_ox_stance->trigger();
      }

      p()->buff.vigilant_watch->trigger();

      p()->buff.tigers_ferocity->trigger();
    }
  }

  void impact( action_state_t *s ) override
  {
    base_t::impact( s );

    p()->buff.hit_scheme->trigger();

    // Teachings of the Monastery
    // Used by both Windwalker and Mistweaver
    if ( p()->buff.teachings_of_the_monastery->up() )
    {
      int stacks = p()->buff.teachings_of_the_monastery->current_stack;

      if ( p()->talent.windwalker.memory_of_the_monastery.enabled() && p()->bugs )
      {
        // TODO: Confirm proper mechanics for this. Tested 17/06/2024 and behaviour has it expire previous stacks before
        // triggering new which feels like a bug.
        p()->buff.memory_of_the_monastery->expire();
      }

      for ( int i = 0; i < stacks; i++ )
      {
        // Transfer the power and Memory of the Monastery triggers from ToTM hits but only on the primary target
        if ( s->chain_target == 0 )
        {
          p()->buff.transfer_the_power->trigger();

          if ( p()->talent.windwalker.memory_of_the_monastery.enabled() )
            p()->buff.memory_of_the_monastery->trigger();
        }

        bok_totm_proc->execute_on_target( s->target );
      }

      // The initial hit along with each individual TotM hits has a chance to reset the cooldown
      auto totmResetChance = p()->shared.teachings_of_the_monastery->effectN( 1 ).percent();

      if ( p()->specialization() == MONK_MISTWEAVER )
        totmResetChance += p()->baseline.mistweaver.aura->effectN( 21 ).percent();

      if ( rng().roll( totmResetChance ) )
      {
        p()->cooldown.rising_sun_kick->reset( true );
        p()->proc.rsk_reset_totm->occur();
      }
    }

    p()->trigger_mark_of_the_crane( s );

    if ( p()->talent.brewmaster.elusive_footwork->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.elusive_brawler->trigger(
          as<int>( p()->talent.brewmaster.elusive_footwork->effectN( 2 ).base_value() ) );
      p()->proc.elusive_footwork_proc->occur();
    }

    if ( p()->talent.brewmaster.staggering_strikes->ok() )
      p()->find_stagger( "Stagger" )
          ->purify_flat(
              s->composite_attack_power() * p()->talent.brewmaster.staggering_strikes->effectN( 2 ).percent(),
              "staggering_strikes" );

    // Martial Mixture triggers from each BoK impact
    p()->buff.martial_mixture->trigger();
  }
};

// ==========================================================================
// Rushing Jade Wind
// ==========================================================================

struct flight_of_the_red_crane_dmg_t : public monk_spell_t
{
  flight_of_the_red_crane_dmg_t( monk_t *p )
    : monk_spell_t( p, "flight_of_the_red_crane_dmg", p->talent.conduit_of_the_celestials.flight_of_the_red_crane_dmg )
  {
    background = true;
    aoe        = as<int>( p->talent.conduit_of_the_celestials.flight_of_the_red_crane->effectN( 1 ).base_value() );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.flight_of_the_red_crane->trigger();
  }
};

struct flight_of_the_red_crane_heal_t : public monk_heal_t
{
  flight_of_the_red_crane_heal_t( monk_t *p )
    : monk_heal_t( p, "flight_of_the_red_crane_heal", p->talent.conduit_of_the_celestials.flight_of_the_red_crane_heal )
  {
    background = true;
    aoe        = as<int>( p->talent.conduit_of_the_celestials.flight_of_the_red_crane->effectN( 1 ).base_value() );
    target     = p;
  }
};

struct rushing_jade_wind_t : public monk_melee_attack_t
{
  buff_t *buff;

  rushing_jade_wind_t( monk_t *player, util::string_view options_str )
    : monk_melee_attack_t( player, "rushing_jade_wind", player->shared.rushing_jade_wind ),
      buff( player->buff.rushing_jade_wind )
  {
    parse_options( options_str );
    may_combo_strike = true;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    buff->trigger();
  }
};

// ==========================================================================
// Spinning Crane Kick
// ==========================================================================

// Jade Ignition Legendary
struct chi_explosion_t : public monk_spell_t
{
  chi_explosion_t( monk_t *player ) : monk_spell_t( player, "chi_explosion", player->passives.chi_explosion )
  {
    dual = background = true;
    aoe               = -1;
    school            = SCHOOL_NATURE;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    am *= 1 + p()->buff.chi_energy->check_stack_value();

    return am;
  }
};

struct sck_tick_action_t : charred_passions_t<monk_melee_attack_t>
{
  sck_tick_action_t( monk_t *p, std::string_view name, const spell_data_t *data )
    : charred_passions_t<monk_melee_attack_t>( p, name, data )
  {
    ww_mastery    = true;
    trigger_chiji = true;

    dual = background   = true;
    aoe                 = -1;
    reduced_aoe_targets = p->baseline.monk.spinning_crane_kick->effectN( 1 ).base_value();

    ap_type = attack_power_type::WEAPON_BOTH;

    parse_effects( p->talent.windwalker.crane_vortex );

    // dance of chiji is scripted
    if ( const auto &effect = p->talent.windwalker.dance_of_chiji->effectN( 1 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( [ &b = p->buff.dance_of_chiji_hidden ]() { return b->check(); } )
          .set_value( effect.percent() )
          .set_eff( &effect );

    parse_effects( p->buff.cyclone_strikes, USE_CURRENT );
  }

  result_amount_type report_amount_type( const action_state_t * ) const override
  {
    return result_amount_type::DMG_DIRECT;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->buff.shuffle->trigger(
        timespan_t::from_seconds( p()->baseline.brewmaster.spinning_crane_kick_rank_2->effectN( 1 ).base_value() ) );
  }
};

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  struct spinning_crane_kick_state_t : public action_state_t
  {
    spinning_crane_kick_state_t( action_t *a, player_t *target ) : action_state_t( a, target )
    {
    }

    proc_types2 cast_proc_type2() const override
    {
      // Spinning Crane Kick seems to trigger Bron's Call to Action (and possibly other
      // effects that care about casts).
      return PROC2_CAST_GENERIC;
    }
  };

  chi_explosion_t *chi_x;

  spinning_crane_kick_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "spinning_crane_kick",
                           ( p->specialization() == MONK_BREWMASTER ? p->baseline.brewmaster.spinning_crane_kick
                                                                    : p->baseline.monk.spinning_crane_kick ) ),
      chi_x( nullptr )
  {
    parse_options( options_str );

    sef_ability      = actions::sef_ability_e::SEF_SPINNING_CRANE_KICK;
    may_combo_strike = true;
    tick_zero        = true;
    tick_action      = new sck_tick_action_t( p, "spinning_crane_kick_tick", data().effectN( 1 ).trigger() );

    interrupt_auto_attack = p->specialization() != MONK_WINDWALKER;
    if ( p->specialization() == MONK_BREWMASTER )
    {
      dot_behavior    = DOT_EXTEND;
      cast_during_sck = true;

      if ( p->talent.brewmaster.charred_passions->ok() )
        add_child( debug_cast<sck_tick_action_t *>( tick_action )->chp_damage );
    }

    if ( p->specialization() == MONK_WINDWALKER )
    {
      channeled    = true;
      dot_behavior = DOT_CLIP;
    }

    if ( p->talent.windwalker.jade_ignition->ok() )
    {
      chi_x = new chi_explosion_t( p );
      add_child( chi_x );
    }

    if ( p->baseline.windwalker.mark_of_the_crane->ok() && p->user_options.motc_override == 0 )
    {
      p->register_on_kill_callback( [ p ]( player_t *target ) {
        if ( p->sim->event_mgr.canceled )
          return;

        if ( auto target_data = p->get_target_data( target );
             target_data && target_data->debuff.mark_of_the_crane->up() )
        {
          make_event( p->sim, target_data->debuff.mark_of_the_crane->remains(), [ p, target_data ]() {
            p->sim->print_debug( "mark of the crane fell off dead target: {} ", target_data->target->name_str );
            p->buff.cyclone_strikes->decrement();
          } );
          target_data->debuff.mark_of_the_crane->expire();
        }
      } );
    }
  }

  bool ready() override
  {
    if ( p()->channeling && p()->channeling->id == id )
      return false;

    return monk_melee_attack_t::ready();
  }

  bool usable_moving() const override
  {
    return true;
  }

  action_state_t *new_state() override
  {
    return new spinning_crane_kick_state_t( this, p()->target );
  }

  double cost_flat_modifier() const override
  {
    double c = monk_melee_attack_t::cost_flat_modifier();

    c += p()->buff.dance_of_chiji_hidden->check_value();  // saved as -2

    return c;
  }

  void execute() override
  {
    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.dance_of_chiji->up() )
      {
        p()->buff.dance_of_chiji->decrement();
        p()->buff.dance_of_chiji_hidden->trigger();

        if ( p()->rng().roll( p()->talent.windwalker.sequenced_strikes->effectN( 1 ).percent() ) )
          p()->buff.bok_proc->increment();  // increment is used to not incur the rppm cooldown
      }
    }

    monk_melee_attack_t::execute();

    timespan_t buff_duration = composite_dot_duration( execute_state );

    p()->buff.spinning_crane_kick->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, buff_duration );

    if ( chi_x && p()->buff.chi_energy->up() )
      chi_x->execute();

    if ( p()->buff.celestial_flames->up() )
      p()->active_actions.breath_of_fire->execute_on_target( execute_state->target );

    if ( p()->talent.windwalker.transfer_the_power->ok() )
      p()->buff.transfer_the_power->trigger();

    p()->buff.tigers_ferocity->trigger();
  }

  void last_tick( dot_t *dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    p()->buff.dance_of_chiji_hidden->expire();

    p()->buff.chi_energy->expire();

    if ( p()->buff.counterstrike->up() )
      p()->proc.counterstrike_sck->occur();
  }
};

// ==========================================================================
// Fists of Fury
// ==========================================================================

struct fists_of_fury_tick_t : public monk_melee_attack_t
{
  fists_of_fury_tick_t( monk_t *p, util::string_view name )
    : monk_melee_attack_t( p, name, p->passives.fists_of_fury_tick )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = p->talent.windwalker.fists_of_fury->effectN( 1 ).base_value();
    full_amount_targets = 1;
    ww_mastery          = true;

    base_costs[ RESOURCE_CHI ] = 0;
    dot_duration               = timespan_t::zero();
    trigger_gcd                = timespan_t::zero();

    parse_effects( p->buff.momentum_boost_damage );
  }

  double composite_target_multiplier( player_t *target ) const override
  {
    double m = monk_melee_attack_t::composite_target_multiplier( target );

    if ( target != p()->target )
      m *= p()->talent.windwalker.fists_of_fury->effectN( 6 ).percent();

    return m;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->buff.transfer_the_power->check_stack_value();

    if ( p()->talent.windwalker.momentum_boost.ok() )
      am *= 1 + ( ( ( 1.0 / p()->composite_melee_haste() ) - 1.0 ) *
                  p()->talent.windwalker.momentum_boost->effectN( 1 ).percent() );

    return am;
  }

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    p()->buff.chi_energy->trigger();
    p()->buff.momentum_boost_damage->trigger();
  }
};

struct fists_of_fury_t : public monk_melee_attack_t
{
  fists_of_fury_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "fists_of_fury", p->talent.windwalker.fists_of_fury )
  {
    parse_options( options_str );

    cooldown         = p->cooldown.fists_of_fury;
    sef_ability      = actions::sef_ability_e::SEF_FISTS_OF_FURY;
    may_combo_strike = true;

    channeled = tick_zero = true;
    interrupt_auto_attack = true;

    attack_power_mod.direct = 0;
    weapon_power_mod        = 0;

    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

    // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
    base_tick_time = dot_duration / 4;

    ability_lag = p->world_lag;

    tick_action        = new fists_of_fury_tick_t( p, "fists_of_fury_tick" );
    tick_action->stats = stats;
  }

  bool usable_moving() const override
  {
    return true;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->buff.fury_of_xuen_stacks->up() && rng().roll( p()->buff.fury_of_xuen_stacks->stack_value() ) )
    {
      p()->buff.fury_of_xuen_stacks->expire();
      p()->buff.fury_of_xuen->trigger();
      p()->active_actions.fury_of_xuen_summon->execute();
    }

    p()->buff.whirling_dragon_punch->trigger();

    p()->buff.tigers_ferocity->trigger();
  }

  void last_tick( dot_t *dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    // Delay the expiration of the buffs until after the tick action happens.
    // Otherwise things trigger before the tick action happens; which is not intended.
    make_event( p()->sim, timespan_t::from_millis( 1 ), [ & ] {
      p()->buff.transfer_the_power->expire();
      p()->buff.pressure_point->trigger();
      p()->buff.momentum_boost_damage->expire();
      p()->buff.momentum_boost_speed->trigger();
    } );
  }
};

// ==========================================================================
// Whirling Dragon Punch
// ==========================================================================

struct whirling_dragon_punch_aoe_tick_t : public monk_melee_attack_t
{
  timespan_t delay;
  whirling_dragon_punch_aoe_tick_t( util::string_view name, monk_t *p, const spell_data_t *s, timespan_t delay )
    : monk_melee_attack_t( p, name, s ), delay( delay )
  {
    ww_mastery = true;

    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = p->talent.windwalker.whirling_dragon_punch->effectN( 1 ).base_value();

    name_str_reporting = "wdp_aoe";
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->talent.windwalker.knowledge_of_the_broken_temple->effectN( 2 ).percent();

    return am;
  }
};

struct whirling_dragon_punch_st_tick_t : public monk_melee_attack_t
{
  whirling_dragon_punch_st_tick_t( util::string_view name, monk_t *p, const spell_data_t *s )
    : monk_melee_attack_t( p, name, s )
  {
    ww_mastery = true;

    background = true;

    name_str_reporting = "wdp_st";
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->talent.windwalker.knowledge_of_the_broken_temple->effectN( 2 ).percent();

    return am;
  }
};

struct whirling_dragon_punch_t : public monk_melee_attack_t
{
  struct whirling_dragon_punch_state_t : public action_state_t
  {
    whirling_dragon_punch_state_t( action_t *a, player_t *target ) : action_state_t( a, target )
    {
    }

    proc_types2 cast_proc_type2() const override
    {
      // Whirling Dragon Punch seems to trigger Bron's Call to Action (and possibly other
      // effects that care about casts).
      return PROC2_CAST_GENERIC;
    }
  };

  std::array<whirling_dragon_punch_aoe_tick_t *, 3> aoe_ticks;
  whirling_dragon_punch_st_tick_t *st_tick;

  struct whirling_dragon_punch_tick_event_t : public event_t
  {
    whirling_dragon_punch_aoe_tick_t *tick;

    whirling_dragon_punch_tick_event_t( whirling_dragon_punch_aoe_tick_t *tick, timespan_t delay )
      : event_t( *tick->player, delay ), tick( tick )
    {
    }

    void execute() override
    {
      tick->execute();
    }
  };

  whirling_dragon_punch_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "whirling_dragon_punch", p->talent.windwalker.whirling_dragon_punch )
  {
    sef_ability = actions::sef_ability_e::SEF_WHIRLING_DRAGON_PUNCH;

    parse_options( options_str );
    interrupt_auto_attack = false;
    channeled             = false;
    may_combo_strike      = true;
    cast_during_sck       = false;

    spell_power_mod.direct = 0.0;

    // 3 server-side hardcoded ticks
    for ( size_t i = 0; i < aoe_ticks.size(); ++i )
    {
      auto delay     = base_tick_time * i;
      aoe_ticks[ i ] = new whirling_dragon_punch_aoe_tick_t( "whirling_dragon_punch_aoe_tick", p,
                                                             p->passives.whirling_dragon_punch_aoe_tick, delay );

      add_child( aoe_ticks[ i ] );
    }

    st_tick = new whirling_dragon_punch_st_tick_t( "whirling_dragon_punch_st_tick", p,
                                                   p->passives.whirling_dragon_punch_st_tick );
    add_child( st_tick );

    apply_affecting_aura( p->talent.windwalker.revolving_whirl );
  }

  action_state_t *new_state() override
  {
    return new whirling_dragon_punch_state_t( this, p()->target );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->movement.whirling_dragon_punch->trigger();

    for ( auto &tick : aoe_ticks )
      make_event<whirling_dragon_punch_tick_event_t>( *sim, tick, tick->delay );

    st_tick->execute();

    if ( p()->talent.windwalker.knowledge_of_the_broken_temple->ok() &&
         p()->talent.windwalker.teachings_of_the_monastery->ok() )
    {
      int stacks = as<int>( p()->talent.windwalker.knowledge_of_the_broken_temple->effectN( 1 ).base_value() );
      p()->buff.teachings_of_the_monastery->trigger( stacks );
    }

    // TODO: Check if this can proc without being talented into DoCJ
    if ( p()->talent.windwalker.dance_of_chiji->ok() &&
         p()->rng().roll( p()->talent.windwalker.revolving_whirl->effectN( 1 ).percent() ) )
      p()->buff.dance_of_chiji->increment();  // increment is used to not incur the rppm cooldown

    p()->buff.tigers_ferocity->trigger();
  }

  bool ready() override
  {
    // Only usable while Fists of Fury and Rising Sun Kick are on cooldown.
    if ( p()->buff.whirling_dragon_punch->up() )
      return monk_melee_attack_t::ready();

    return false;
  }
};

// ==========================================================================
// Strike of the Windlord
// ==========================================================================
// Off hand hits first followed by main hand
// The ability does NOT require an off-hand weapon to be executed.
// The ability uses the main-hand weapon damage for both attacks

struct strike_of_the_windlord_main_hand_t : public monk_melee_attack_t
{
  strike_of_the_windlord_main_hand_t( monk_t *p, const char *name, const spell_data_t *s )
    : monk_melee_attack_t( p, name, s )
  {
    sef_ability = actions::sef_ability_e::SEF_STRIKE_OF_THE_WINDLORD;

    ww_mastery = true;
    ap_type    = attack_power_type::WEAPON_MAINHAND;

    aoe       = -1;
    may_dodge = may_parry = may_block = may_miss = true;
    dual = background = true;
  }

  // Damage must be divided on non-main target by the number of targets
  double composite_aoe_multiplier( const action_state_t *state ) const override
  {
    if ( state->target != target )
    {
      return 1.0 / state->n_targets;
    }

    return 1.0;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->talent.windwalker.communion_with_wind->effectN( 2 ).percent();

    return am;
  }
};

struct strike_of_the_windlord_off_hand_t : public monk_melee_attack_t
{
  strike_of_the_windlord_off_hand_t( monk_t *p, const char *name, const spell_data_t *s )
    : monk_melee_attack_t( p, name, s )
  {
    sef_ability = actions::sef_ability_e::SEF_STRIKE_OF_THE_WINDLORD_OH;
    ww_mastery  = true;
    ap_type     = attack_power_type::WEAPON_OFFHAND;

    aoe       = -1;
    may_dodge = may_parry = may_block = may_miss = true;
    dual = background = true;
  }

  // Damage must be divided on non-main target by the number of targets
  double composite_aoe_multiplier( const action_state_t *state ) const override
  {
    if ( state->target != target )
    {
      return 1.0 / state->n_targets;
    }

    return 1.0;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->talent.windwalker.communion_with_wind->effectN( 2 ).percent();

    return am;
  }

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->talent.windwalker.thunderfist.ok() )
    {
      int thunderfist_stacks = 1;

      if ( s->chain_target == 0 )
        thunderfist_stacks += as<int>( p()->talent.windwalker.thunderfist->effectN( 1 ).base_value() );

      p()->buff.thunderfist->trigger( thunderfist_stacks );
    }

    if ( p()->talent.windwalker.rushing_jade_wind.ok() )
      p()->trigger_mark_of_the_crane( s );

    if ( p()->talent.windwalker.gale_force.ok() )
      get_td( s->target )->debuff.gale_force->trigger();
  }
};

struct strike_of_the_windlord_t : public monk_melee_attack_t
{
  strike_of_the_windlord_main_hand_t *mh_attack;
  strike_of_the_windlord_off_hand_t *oh_attack;

  strike_of_the_windlord_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "strike_of_the_windlord", p->talent.windwalker.strike_of_the_windlord ),
      mh_attack( nullptr ),
      oh_attack( nullptr )
  {
    apply_affecting_effect( p->talent.windwalker.communion_with_wind->effectN( 1 ) );

    may_combo_strike = true;
    cast_during_sck  = false;
    cooldown->hasted = false;
    trigger_gcd      = data().gcd();

    parse_options( options_str );

    oh_attack =
        new strike_of_the_windlord_off_hand_t( p, "strike_of_the_windlord_offhand", data().effectN( 4 ).trigger() );
    mh_attack =
        new strike_of_the_windlord_main_hand_t( p, "strike_of_the_windlord_mainhand", data().effectN( 3 ).trigger() );

    add_child( oh_attack );
    add_child( mh_attack );

    if ( p->talent.windwalker.thunderfist.ok() )
      add_child( p->passive_actions.thunderfist );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    // Off-hand attack hits first
    oh_attack->execute();

    if ( result_is_hit( oh_attack->execute_state->result ) )
      mh_attack->execute();

    if ( p()->talent.windwalker.rushing_jade_wind.ok() )
    {
      p()->buff.rushing_jade_wind->trigger();
      if ( p()->bugs )
        combo_strikes_trigger();
    }

    p()->buff.tigers_ferocity->trigger();

    if ( p()->talent.windwalker.darting_hurricane.ok() )
      p()->buff.darting_hurricane->increment(
          as<int>( p()->talent.windwalker.darting_hurricane->effectN( 2 )
                       .base_value() ) );  // increment is used to not incur the rppm cooldown

    if ( p()->buff.heart_of_the_jade_serpent->up() )
    {
      p()->buff.heart_of_the_jade_serpent_cdr->trigger();
      p()->buff.inner_compass_serpent_stance->trigger();
      p()->buff.heart_of_the_jade_serpent->decrement();
    }
  }
};

// ==========================================================================
// Thunderfist
// ==========================================================================

struct thunderfist_t : public monk_spell_t
{
  thunderfist_t( monk_t *player )
    : monk_spell_t( player, "thunderfist", player->passives.thunderfist->effectN( 1 ).trigger() )
  {
    background = true;
    may_crit   = true;
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    p()->buff.thunderfist->decrement( 1 );
  }
};

// ==========================================================================
// Melee
// ==========================================================================

struct press_the_advantage_melee_t : public monk_spell_t
{
  press_the_advantage_melee_t( monk_t *player )
    : monk_spell_t( player, "press_the_advantage", player->find_spell( 418360 ) )
  {
    background = true;

    if ( p()->talent.brewmaster.press_the_advantage->ok() && p()->talent.brewmaster.chi_surge->ok() )
      add_child( p()->active_actions.chi_surge );
  }
};

struct melee_t : public monk_melee_attack_t
{
  int sync_weapons;
  bool dual_threat_enabled = true;  // Dual Threat requires one succesful melee inbetween casts
  bool first;
  bool oh;

  melee_t( util::string_view name, monk_t *player, int sw, bool is_oh = false )
    : monk_melee_attack_t( player, name ), sync_weapons( sw ), first( true ), oh( is_oh )
  {
    background = repeating = may_glance = true;
    may_crit                            = true;
    trigger_gcd                         = timespan_t::zero();
    special                             = false;
    school                              = SCHOOL_PHYSICAL;
    weapon_multiplier                   = 1.0;
    allow_class_ability_procs           = true;
    not_a_proc                          = true;

    monk_melee_attack_t::apply_buff_effects();
    monk_melee_attack_t::apply_debuff_effects();

    if ( player->main_hand_weapon.group() == WEAPON_1H )
    {
      if ( player->specialization() != MONK_MISTWEAVER )
        base_hit -= 0.19;
    }
  }

  void reset() override
  {
    monk_melee_attack_t::reset();
    first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = monk_melee_attack_t::execute_time();

    if ( first )
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 )
                                               : timespan_t::zero();
    else
      return t;
  }

  void execute() override
  {
    first = false;
    monk_melee_attack_t::execute();
  }

  void impact( action_state_t *s ) override
  {
    if ( dual_threat_enabled && p()->rng().roll( p()->talent.windwalker.dual_threat->effectN( 1 ).percent() ) )
    {
      s->result_total = 0;
      p()->dual_threat_kick->execute();
      dual_threat_enabled = false;
    }
    else
    {
      monk_melee_attack_t::impact( s );

      if ( p()->talent.brewmaster.press_the_advantage->ok() && weapon->slot == SLOT_MAIN_HAND )
        p()->buff.press_the_advantage->trigger();

      if ( result_is_hit( s->result ) )
      {
        if ( p()->talent.brewmaster.press_the_advantage->ok() && weapon->slot == SLOT_MAIN_HAND )
        {
          // Reduce Brew cooldown by 0.5 seconds
          p()->baseline.brewmaster.brews.adjust(
              p()->talent.brewmaster.press_the_advantage->effectN( 1 ).time_value() );

          // Trigger the Press the Advantage damage proc
          p()->passive_actions.press_the_advantage->target = s->target;
          p()->passive_actions.press_the_advantage->schedule_execute();
        }

        if ( p()->buff.thunderfist->up() )
          p()->passive_actions.thunderfist->execute_on_target( s->target );

        dual_threat_enabled = true;
      }
    }
  }
};

// ==========================================================================
// Auto Attack
// ==========================================================================

// Dual Threat WW Talent

struct dual_threat_t : public monk_melee_attack_t
{
  dual_threat_t( monk_t *p ) : monk_melee_attack_t( p, "dual_threat_kick", p->passives.dual_threat_kick )
  {
    background = true;
    may_glance = true;
    may_crit   = true;  // I assume so? This ability doesn't appear in the combat log yet on alpha

    allow_class_ability_procs = false;  // Is not proccing Thunderfist or other class ability procs

    school            = SCHOOL_PHYSICAL;
    weapon_multiplier = 1.0;
    weapon            = &( player->main_hand_weapon );

    cooldown->duration = base_execute_time = trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->buff.dual_threat->trigger();
  }
};

struct auto_attack_t : public monk_melee_attack_t
{
  int sync_weapons;

  dual_threat_t *dual_threat_kick;

  auto_attack_t( monk_t *player, util::string_view options_str )
    : monk_melee_attack_t( player, "auto_attack" ), sync_weapons( 0 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );

    ignore_false_positive = true;
    trigger_gcd           = timespan_t::zero();
    //    background            = true;

    p()->main_hand_attack                    = new melee_t( "melee_main_hand", player, sync_weapons );
    p()->main_hand_attack->weapon            = &( player->main_hand_weapon );
    p()->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;

    add_child( p()->main_hand_attack );

    if ( player->off_hand_weapon.type != WEAPON_NONE )
    {
      if ( !player->dual_wield() )
        return;

      p()->off_hand_attack                    = new melee_t( "melee_off_hand", player, sync_weapons, true );
      p()->off_hand_attack->weapon            = &( player->off_hand_weapon );
      p()->off_hand_attack->base_execute_time = player->off_hand_weapon.swing_time;
      p()->off_hand_attack->id                = 1;

      add_child( p()->off_hand_attack );
    }

    if ( p()->talent.windwalker.dual_threat.ok() )
    {
      p()->dual_threat_kick = new dual_threat_t( player );

      add_child( p()->dual_threat_kick );
    }
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move > 5 )
      return false;

    return ( p()->main_hand_attack->execute_event == nullptr ||
             ( p()->off_hand_attack && p()->off_hand_attack->execute_event == nullptr ) );  // not swinging
  }

  void execute() override
  {
    if ( player->main_hand_attack )
      p()->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack )
      p()->off_hand_attack->schedule_execute();
  }
};

// ==========================================================================
// Keg Smash
// ==========================================================================
struct keg_smash_t : monk_melee_attack_t
{
  keg_smash_t( monk_t *player, std::string_view options_str, std::string_view name = "keg_smash" )
    : monk_melee_attack_t( player, name, player->talent.brewmaster.keg_smash )
  {
    parse_options( options_str );
    // TODO: can cast_during_sck be automated?
    cast_during_sck = true;

    // No auto-parsing is presently possible.
    reduced_aoe_targets = data().effectN( 7 ).base_value();
    aoe                 = -1;

    apply_affecting_aura( player->talent.brewmaster.stormstouts_last_keg );
    parse_effects( player->buff.hit_scheme );
    // we have to set this up by hand, as scalding brew is scripted
    if ( const auto &effect = player->talent.brewmaster.scalding_brew->effectN( 1 ); effect.ok() )
      add_parse_entry( target_multiplier_effects )
          .set_func( td_fn( &monk_td_t::dots_t::breath_of_fire ) )
          .set_value( effect.percent() )
          .set_eff( &effect );
    parse_effects( player->buff.flow_of_battle_free_keg_smash );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->buff.hit_scheme->expire();
    p()->buff.flow_of_battle_free_keg_smash->expire();

    if ( p()->talent.brewmaster.salsalabims_strength->ok() )
    {
      p()->cooldown.breath_of_fire->reset( true );
      p()->proc.salsalabims_strength->occur();
    }

    p()->buff.shuffle->trigger( timespan_t::from_seconds( data().effectN( 6 ).base_value() ) );

    timespan_t reduction = timespan_t::from_seconds( data().effectN( 4 ).base_value() );
    if ( p()->buff.blackout_combo->up() )
    {
      reduction += timespan_t::from_seconds( p()->buff.blackout_combo->data().effectN( 3 ).base_value() );
      p()->proc.blackout_combo_keg_smash->occur();
    }
    p()->buff.blackout_combo->expire();

    p()->baseline.brewmaster.brews.adjust( reduction );
  }

  void impact( action_state_t *state ) override
  {
    monk_melee_attack_t::impact( state );
    get_td( state->target )->debuff.keg_smash->trigger();
    if ( p()->buff.weapons_of_order->up() )
      get_td( state->target )->debuff.weapons_of_order->trigger();
  }
};

// ==========================================================================
// Touch of Death
// ==========================================================================
struct touch_of_death_t : public monk_melee_attack_t
{
  touch_of_death_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "touch_of_death", p->baseline.monk.touch_of_death )
  {
    ww_mastery = true;
    may_crit = hasted_ticks = false;
    may_combo_strike        = true;
    cast_during_sck         = true;
    parse_options( options_str );

    cooldown->duration = data().cooldown();

    apply_affecting_aura( p->talent.monk.fatal_touch );
  }

  void init() override
  {
    monk_melee_attack_t::init();

    snapshot_flags = update_flags = 0;
  }

  double composite_target_armor( player_t * ) const override
  {
    return 0;
  }

  bool target_ready( player_t *target ) override
  {
    // Deals damage equal to 35% of your maximum health against players and stronger creatures under 15% health
    // 2023-10-19 Tooltip lies and the 15% health works on all non-player targets.
    if ( p()->talent.monk.improved_touch_of_death->ok() &&
         ( target->health_percentage() < p()->talent.monk.improved_touch_of_death->effectN( 1 ).base_value() ) )
      return monk_melee_attack_t::target_ready( target );

    // You exploit the enemy target's weakest point, instantly killing creatures if they have less health than you
    // Only applicable in health based sims
    if ( target->current_health() > 0 && target->current_health() <= p()->max_health() )
      return monk_melee_attack_t::target_ready( target );

    return false;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->buff.touch_of_death_ww->trigger();
    p()->buff.fatal_touch->trigger();
  }

  void impact( action_state_t *s ) override
  {
    double max_hp, amount;

    // In execute range ToD deals player's max HP
    amount = max_hp = p()->max_health();

    // Not in execute range
    // or not a health-based fight style
    // or a secondary target... these always get hit for the 35% from Improved Touch of Death regardless if you're
    // talented into it or not
    if ( s->chain_target > 0 || target->current_health() == 0 || target->current_health() > max_hp )
    {
      amount *= p()->passives.improved_touch_of_death->effectN( 2 ).percent();  // 0.35

      amount *= 1 + p()->talent.windwalker.meridian_strikes->effectN( 1 ).percent();

      // Damage is only affected by Windwalker's Mastery
      // Versatility does not affect the damage of Touch of Death.
      if ( p()->buff.combo_strikes->up() )
        amount *= 1 + p()->cache.mastery_value();
    }

    s->result_total = s->result_raw = amount;

    monk_melee_attack_t::impact( s );

    if ( p()->baseline.brewmaster.stagger->ok() )
    {
      p()->find_stagger( "Stagger" )
          ->purify_flat( amount * p()->baseline.brewmaster.touch_of_death_rank_3->effectN( 1 ).percent(),
                         "touch_of_death" );
    }
  }
};

// ==========================================================================
// Touch of Karma
// ==========================================================================
// When Touch of Karma (ToK) is activated, two spells are placed. A buff on the player (id: 125174), and a
// debuff on the target (id: 122470). Whenever the player takes damage, a dot (id: 124280) is placed on
// the target that increases as the player takes damage. Each time the player takes damage, the dot is refreshed
// and recalculates the dot size based on the current dot size. Just to make it easier to code, I'll wait until
// the Touch of Karma buff expires before placing a dot on the target. Net result should be the same.

// 8.1 Good Karma - If the player still has the ToK buff on them, each time the target hits the player, the amount
// absorbed is immediatly healed by the Good Karma spell (id: 285594)

struct touch_of_karma_dot_t : public residual_action::residual_periodic_action_t<spell_t>
{
  using base_t = residual_action::residual_periodic_action_t<spell_t>;
  touch_of_karma_dot_t( monk_t *p ) : base_t( "touch_of_karma", p, p->passives.touch_of_karma_tick )
  {
    may_miss = may_crit = false;
    dual                = true;
    proc                = true;
    ap_type             = attack_power_type::NO_WEAPON;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    base_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags = update_flags = 0;
    snapshot_flags |= STATE_VERSATILITY;
  }
};

struct touch_of_karma_t : public monk_melee_attack_t
{
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double pct_health;
  touch_of_karma_dot_t *touch_of_karma_dot;
  touch_of_karma_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "touch_of_karma", p->baseline.windwalker.touch_of_karma ),
      interval( 100 ),
      interval_stddev( 0.05 ),
      interval_stddev_opt( 0 ),
      pct_health( 0.5 ),
      touch_of_karma_dot( new touch_of_karma_dot_t( p ) )
  {
    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "pct_health", pct_health ) );
    parse_options( options_str );

    cooldown->duration = data().cooldown();
    base_dd_min = base_dd_max = 0;
    ap_type                   = attack_power_type::NO_WEAPON;
    cast_during_sck           = true;

    double max_pct = data().effectN( 3 ).percent();

    if ( pct_health > max_pct )  // Does a maximum of 50% of the monk's HP.
      pct_health = max_pct;

    if ( interval < cooldown->duration.total_seconds() )
    {
      sim->error( "{} minimum interval for Touch of Karma is 90 seconds.", *player );
      interval = cooldown->duration.total_seconds();
    }

    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;

    trigger_gcd = timespan_t::zero();
    may_crit = may_miss = may_dodge = may_parry = false;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_melee_attack_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags = update_flags = 0;
  }

  bool target_ready( player_t *target ) override
  {
    if ( target->name_str == "Target Dummy" )
      return false;

    return monk_melee_attack_t::target_ready( target );
  }

  void execute() override
  {
    timespan_t new_cd        = timespan_t::from_seconds( rng().gauss( interval, interval_stddev ) );
    timespan_t data_cooldown = data().cooldown();
    if ( new_cd < data_cooldown )
      new_cd = data_cooldown;

    cooldown->duration = new_cd;

    monk_melee_attack_t::execute();

    if ( pct_health > 0 )
    {
      double damage_amount = pct_health * player->max_health();

      // Redirects 70% of the damage absorbed
      damage_amount *= data().effectN( 4 ).percent();

      residual_action::trigger( touch_of_karma_dot, execute_state->target, damage_amount );
    }
  }
};

// ==========================================================================
// Provoke
// ==========================================================================

struct provoke_t : public monk_melee_attack_t
{
  provoke_t( monk_t *p, util::string_view options_str ) : monk_melee_attack_t( p, "provoke", p->baseline.monk.provoke )
  {
    parse_options( options_str );
    use_off_gcd           = true;
    ignore_false_positive = true;
  }

  void impact( action_state_t *s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    monk_melee_attack_t::impact( s );
  }
};

// ==========================================================================
// Spear Hand Strike
// ==========================================================================

struct spear_hand_strike_t : public monk_melee_attack_t
{
  spear_hand_strike_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "spear_hand_strike", p->talent.monk.spear_hand_strike )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    is_interrupt          = true;
    cast_during_sck       = player->specialization() != MONK_WINDWALKER;
    may_miss = may_block = may_dodge = may_parry = false;
  }
};

// ==========================================================================
// Leg Sweep
// ==========================================================================

struct leg_sweep_t : public monk_melee_attack_t
{
  leg_sweep_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "leg_sweep", p->baseline.monk.leg_sweep )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
    cast_during_sck                              = player->specialization() != MONK_WINDWALKER;

    radius += p->talent.monk.tiger_tail_sweep->effectN( 1 ).base_value();
  }
};

// ==========================================================================
// Paralysis
// ==========================================================================

struct paralysis_t : public monk_melee_attack_t
{
  paralysis_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "paralysis", p->talent.monk.paralysis )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }
};

// ==========================================================================
// Flying Serpent Kick
// ==========================================================================

struct flying_serpent_kick_t : public monk_melee_attack_t
{
  bool first_charge;
  double movement_speed_increase;
  flying_serpent_kick_t( monk_t *p, util::string_view options_str )
    : monk_melee_attack_t( p, "flying_serpent_kick", p->baseline.windwalker.flying_serpent_kick ),
      first_charge( true ),
      movement_speed_increase( p->baseline.windwalker.flying_serpent_kick->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    may_crit                        = true;
    ww_mastery                      = true;
    may_combo_strike                = true;
    ignore_false_positive           = true;
    movement_directionality         = movement_direction_type::OMNI;
    aoe                             = -1;
    p->cooldown.flying_serpent_kick = cooldown;
  }

  void reset() override
  {
    monk_melee_attack_t::reset();
    first_charge = true;
  }

  bool ready() override
  {
    if ( first_charge )  // Assumes that we fsk into combat, instead of setting initial distance to 20 yards.
      return monk_melee_attack_t::ready();

    return monk_melee_attack_t::ready();
  }

  void execute() override
  {
    if ( p()->current.distance_to_move >= 0 )
    {
      p()->buff.flying_serpent_kick_movement->trigger(
          1, movement_speed_increase, 1,
          timespan_t::from_seconds(
              std::min( 1.5, p()->current.distance_to_move /
                                 ( p()->base_movement_speed *
                                   ( 1 + p()->stacking_movement_modifier() + movement_speed_increase ) ) ) ) );
      p()->current.moving_away = 0;
    }

    monk_melee_attack_t::execute();

    if ( first_charge )
    {
      p()->movement.flying_serpent_kick->trigger();

      first_charge = !first_charge;
    }
  }
};
}  // namespace attacks

namespace spells
{

// ==========================================================================
// Special Delivery
// ==========================================================================
struct special_delivery_t : public monk_spell_t
{
  special_delivery_t( monk_t *player )
    : monk_spell_t( player, "special_delivery",
                    player->talent.brewmaster.special_delivery_missile->effectN( 1 ).trigger() )
  {
    background   = true;
    travel_delay = player->talent.brewmaster.special_delivery_missile->missile_speed();
  }

  void execute() override
  {
    if ( !p()->talent.brewmaster.special_delivery->ok() )
      return;
    monk_spell_t::execute();
  }
};

// ==========================================================================
// Black Ox Brew
// ==========================================================================

struct black_ox_brew_t : public brew_t<monk_spell_t>
{
  // special_delivery_t *delivery;
  black_ox_brew_t( monk_t *player, util::string_view options_str )
    : brew_t<monk_spell_t>( player, "black_ox_brew", player->talent.brewmaster.black_ox_brew )
  {
    parse_options( options_str );

    harmful       = false;
    use_off_gcd   = true;
    energize_type = action_energize::NONE;  // disable resource gain from spell data
  }

  void execute() override
  {
    brew_t<monk_spell_t>::execute();

    for ( auto &[ key, cooldown ] : p()->baseline.brewmaster.brews.cooldowns )
    {
      if ( key == p()->talent.brewmaster.purifying_brew->id() )
        cooldown->reset( true, 2 );
      if ( key == p()->talent.brewmaster.celestial_brew->id() )
        cooldown->reset( true, 1 );
    }

    p()->resource_gain( RESOURCE_ENERGY, p()->talent.brewmaster.black_ox_brew->effectN( 1 ).base_value(),
                        p()->gain.black_ox_brew_energy );

    p()->active_actions.special_delivery->execute();
  }
};

// ==========================================================================
// Roll
// ==========================================================================

struct roll_t : public monk_spell_t
{
  roll_t( monk_t *player, util::string_view options_str )
    : monk_spell_t( player, "roll",
                    ( player->talent.monk.chi_torpedo->ok() ? spell_data_t::not_found() : player->baseline.monk.roll ) )
  {
    cast_during_sck = player->specialization() != MONK_WINDWALKER;

    parse_options( options_str );

    cooldown->duration += player->talent.monk.celerity->effectN( 1 ).time_value();
    cooldown->charges += (int)player->talent.monk.celerity->effectN( 2 ).base_value();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->movement.roll->trigger();
  }
};

// ==========================================================================
// Chi Torpedo
// ==========================================================================

struct chi_torpedo_t : public monk_spell_t
{
  chi_torpedo_t( monk_t *player, util::string_view options_str )
    : monk_spell_t(
          player, "chi_torpedo",
          ( player->talent.monk.chi_torpedo->ok() ? player->talent.monk.chi_torpedo : spell_data_t::not_found() ) )
  {
    parse_options( options_str );

    cast_during_sck = player->specialization() != MONK_WINDWALKER;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.chi_torpedo->trigger();
    p()->movement.chi_torpedo->trigger();
  }
};

// ==========================================================================
// Crackling Jade Lightning
// ==========================================================================

struct crackling_jade_lightning_t : public monk_spell_t
{
  struct crackling_jade_lightning_aoe_t : public monk_spell_t
  {
    crackling_jade_lightning_aoe_t( monk_t *p )
      : monk_spell_t( p, "crackling_jade_lightning_aoe", p->baseline.monk.crackling_jade_lightning )
    {
      dual = background = true;
      ww_mastery        = true;

      parse_effects( p->talent.windwalker.power_of_the_thunder_king, effect_mask_t( true ).disable( 1 ) );
      parse_effects( p->buff.the_emperors_capacitor );
    }

    double cost_per_tick( resource_e ) const override
    {
      return 0.0;
    }
  };

  crackling_jade_lightning_aoe_t *aoe_dot;

  crackling_jade_lightning_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "crackling_jade_lightning", p->baseline.monk.crackling_jade_lightning ),
      aoe_dot( new crackling_jade_lightning_aoe_t( p ) )
  {
    sef_ability           = actions::sef_ability_e::SEF_CRACKLING_JADE_LIGHTNING;
    may_combo_strike      = true;
    ww_mastery            = true;
    interrupt_auto_attack = true;
    channeled             = true;

    parse_options( options_str );

    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd = timespan_t::from_millis( 750 );

    parse_effects( p->talent.windwalker.power_of_the_thunder_king, effect_mask_t( true ).disable( 1 ) );
    parse_effects( p->buff.the_emperors_capacitor );

    if ( p->talent.windwalker.power_of_the_thunder_king->ok() )
      add_child( aoe_dot );
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->talent.windwalker.power_of_the_thunder_king->ok() )
    {
      const auto &tl = target_list();
      int count      = 0;

      for ( auto &t : tl )
      {
        // Don't apply AoE version to primary target
        if ( t == target )
          continue;

        if ( count < p()->talent.windwalker.power_of_the_thunder_king->effectN( 1 ).base_value() )
        {
          aoe_dot->execute_on_target( t );
          count++;
        }
      }
    }
  }

  void last_tick( dot_t *dot ) override
  {
    monk_spell_t::last_tick( dot );

    p()->buff.the_emperors_capacitor->expire();

    if ( p()->talent.windwalker.power_of_the_thunder_king->ok() )
    {
      const auto &tl = target_list();
      for ( const auto &t : tl )
      {
        get_td( t )->dot.crackling_jade_lightning_aoe->cancel();
        get_td( t )->dot.crackling_jade_lightning_sef->cancel();
        get_td( t )->dot.crackling_jade_lightning_sef_aoe->cancel();
      }
    }
    // Reset swing timer
    if ( player->main_hand_attack )
    {
      player->main_hand_attack->cancel();
      if ( !player->main_hand_attack->target->is_sleeping() )
      {
        player->main_hand_attack->schedule_execute();
      }
    }

    if ( player->off_hand_attack )
    {
      player->off_hand_attack->cancel();
      if ( !player->off_hand_attack->target->is_sleeping() )
      {
        player->off_hand_attack->schedule_execute();
      }
    }
  }
};

// ==========================================================================
// Breath of Fire
// ==========================================================================
struct breath_of_fire_dot_t : public monk_spell_t
{
  bool blackout_combo;

  breath_of_fire_dot_t( monk_t *p )
    : monk_spell_t( p, "breath_of_fire_dot", p->talent.brewmaster.breath_of_fire_dot ), blackout_combo( false )
  {
    background    = true;
    tick_may_crit = may_crit = true;
    hasted_ticks             = false;
  }

  double composite_persistent_multiplier( const action_state_t *state ) const override
  {
    double cpm = monk_spell_t::composite_persistent_multiplier( state );
    if ( blackout_combo )
      cpm *= 1.0 + p()->buff.blackout_combo->data().effectN( 5 ).percent();
    return cpm;
  }

  void execute() override
  {
    // blackout combo buffs only one of the breath of fire dot applications from
    // a single cast
    monk_spell_t::execute();
    blackout_combo = p()->buff.blackout_combo->up();
    p()->buff.blackout_combo->expire();
  }
};

struct breath_of_fire_t : public monk_spell_t
{
  struct dragonfire_brew_t : monk_spell_t
  {
    dragonfire_brew_t( monk_t *player )
      : monk_spell_t( player, "dragonfire_brew", player->talent.brewmaster.dragonfire_brew_hit )
    {
      background = true;
      aoe        = -1;
    }

    void execute() override
    {
      for ( size_t i = 0; i < p()->talent.brewmaster.dragonfire_brew->effectN( 1 ).base_value(); i++ )
        monk_spell_t::execute();
    }
  };

  dragonfire_brew_t *dragonfire_brew;
  bool no_bof_hit;

  breath_of_fire_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "breath_of_fire", p->talent.brewmaster.breath_of_fire ),
      dragonfire_brew( new dragonfire_brew_t( p ) ),
      no_bof_hit( false )
  {
    add_option( opt_bool( "no_bof_hit", no_bof_hit ) );
    parse_options( options_str );
    gcd_type = gcd_haste_type::NONE;

    aoe                 = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    cast_during_sck     = true;

    add_child( p->active_actions.breath_of_fire );
    add_child( dragonfire_brew );
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();
    // Currently the value is saved as 100% and each of the values is stagger_index / 3 * base_value
    double bof_stagger_bonus = p()->talent.brewmaster.dragonfire_brew->effectN( 2 ).percent();
    am *= 1.0 + ( p()->find_stagger( "Stagger" )->level_index() / 3.0 ) * bof_stagger_bonus;

    return am;
  }

  void execute() override
  {
    p()->buff.charred_passions->trigger();
    if ( no_bof_hit )
      return;

    monk_spell_t::execute();
    dragonfire_brew->execute();

    if ( p()->buff.blackout_combo->up() )
      p()->proc.blackout_combo_breath_of_fire->occur();
    // defer boc consumption to be handled by bof dot
  }

  void impact( action_state_t *state ) override
  {
    monk_spell_t::impact( state );

    if ( get_td( state->target )->debuff.keg_smash->up() )
      p()->active_actions.breath_of_fire->execute_on_target( state->target );
  }
};

// ==========================================================================
// Fortifying Brew
// ==========================================================================

// Niuzao's Protection ==============================================
struct fortifying_brew_t : brew_t<monk_spell_t>
{
  struct niuzaos_protection_t : public monk_absorb_t
  {
    niuzaos_protection_t( monk_t *p )
      : monk_absorb_t( p, "niuzaos_protection", p->talent.conduit_of_the_celestials.niuzaos_protection )
    {
      background  = true;
      target      = p;
      base_dd_min = p->max_health() * data().effectN( 2 ).percent();
      base_dd_max = base_dd_min;
    }
  };

  niuzaos_protection_t *absorb;

  fortifying_brew_t( monk_t *p, util::string_view options_str )
    : brew_t<monk_spell_t>( p, "fortifying_brew", p->talent.monk.fortifying_brew.find_override_spell() ),
      absorb( p->talent.conduit_of_the_celestials.niuzaos_protection->ok() ? new niuzaos_protection_t( p ) : nullptr )
  {
    cast_during_sck = player->specialization() != MONK_WINDWALKER;

    parse_options( options_str );

    harmful = may_crit = false;

    apply_affecting_aura( p->talent.monk.expeditious_fortification );
    apply_affecting_aura( p->talent.monk.ironshell_brew );
    apply_affecting_aura( p->talent.brewmaster.fortifying_brew_determination );
  }

  void execute() override
  {
    brew_t<monk_spell_t>::execute();

    if ( p()->talent.conduit_of_the_celestials.niuzaos_protection->ok() )
      absorb->execute();
    p()->buff.fortifying_brew->trigger();
    p()->active_actions.special_delivery->execute();
  }
};

// ==========================================================================
// Exploding Keg
// ==========================================================================
// Exploding Keg Proc ==============================================
struct exploding_keg_proc_t : public monk_spell_t
{
  exploding_keg_proc_t( monk_t *p )
    : monk_spell_t( p, "exploding_keg_proc", p->talent.brewmaster.exploding_keg->effectN( 4 ).trigger() )
  {
    background = dual = true;
    proc              = true;
  }
};

struct exploding_keg_t : public monk_spell_t
{
  exploding_keg_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "exploding_keg", p->talent.brewmaster.exploding_keg )
  {
    parse_options( options_str );
    cast_during_sck = true;
    gcd_type        = gcd_haste_type::NONE;
    aoe             = -1;
    add_child( p->active_actions.exploding_keg );
  }

  timespan_t travel_time() const override
  {
    // Always has the same time to land regardless of distance, probably represented there.
    return timespan_t::from_seconds( data().missile_speed() );
  }

  void execute() override
  {
    p()->buff.exploding_keg->trigger();
    monk_spell_t::execute();
  }

  void impact( action_state_t *s ) override
  {
    monk_spell_t::impact( s );
    get_td( s->target )->debuff.exploding_keg->trigger();
  }
};

// ==========================================================================
// Purifying Brew
// ==========================================================================
struct purifying_brew_t : public brew_t<monk_spell_t>
{
  struct gai_plins_imperial_brew_t : monk_heal_t
  {
    gai_plins_imperial_brew_t( monk_t *player )
      : monk_heal_t( player, "gai_plins_imperial_brew", player->talent.brewmaster.gai_plins_imperial_brew_heal )
    {
      background = true;
    }

    void init() override
    {
      monk_heal_t::init();
      snapshot_flags = update_flags = STATE_NO_MULTIPLIER;
    }
  };

  gai_plins_imperial_brew_t *gai_plins;

  purifying_brew_t( monk_t *player, util::string_view options_str )
    : brew_t<monk_spell_t>( player, "purifying_brew", player->talent.brewmaster.purifying_brew ),
      gai_plins( new gai_plins_imperial_brew_t( player ) )
  {
    parse_options( options_str );

    harmful         = false;
    cast_during_sck = true;
    use_off_gcd     = true;

    apply_affecting_aura( player->talent.brewmaster.light_brewing );
  }

  bool ready() override
  {
    if ( p()->find_stagger( "Stagger" )->is_ticking() )
      return monk_spell_t::ready();
    return false;
  }

  void execute() override
  {
    brew_t<monk_spell_t>::execute();

    p()->buff.pretense_of_instability->trigger();
    p()->active_actions.special_delivery->execute();

    auto stacks = as<unsigned>( p()->find_stagger( "Stagger" )->level_index() );
    if ( stacks > 0 )
    {
      p()->buff.purified_chi->trigger( stacks );
      p()->buff.ox_stance->trigger( stacks );
      p()->buff.aspect_of_harmony.trigger_flat(
          stacks * p()->talent.master_of_harmony.clarity_of_purpose->effectN( 1 ).percent() *
          ( 1.0 + p()->composite_damage_versatility() ) *
          p()->composite_total_attack_power_by_type( attack_power_type::WEAPON_MAINHAND ) );
      p()->buff.recent_purifies->trigger( stacks );
    }

    double purify_percent = data().effectN( 1 ).percent();
    purify_percent += 2.0 * p()->talent.master_of_harmony.mantra_of_purity->effectN( 1 ).percent();
    double cleared = p()->find_stagger( "Stagger" )->purify_percent( purify_percent, "purifying_brew" );

    double healed = cleared * p()->talent.brewmaster.gai_plins_imperial_brew->effectN( 1 ).percent();
    if ( healed )
    {
      gai_plins->base_dd_min = gai_plins->base_dd_max = healed;
      gai_plins->target                               = p();
      gai_plins->execute();
    }

    if ( p()->buff.blackout_combo->up() )
    {
      timespan_t delay = timespan_t::from_seconds( p()->buff.blackout_combo->data().effectN( 4 ).base_value() );
      p()->find_stagger( "Stagger" )->delay_tick( delay );
      p()->proc.blackout_combo_purifying_brew->occur();
    }
    p()->buff.blackout_combo->expire();
  }
};

// ==========================================================================
// Mana Tea
// ==========================================================================
// Manatee
//                   _.---.._
//     _        _.-'         ''-.
//   .'  '-,_.-'                 '''.
//  (       _                     o  :
//   '._ .-'  '-._         \  \-  ---]
//                 '-.___.-')  )..-'
//                          (_/lame

struct mana_tea_t : public monk_spell_t
{
  mana_tea_t( monk_t *p, util::string_view options_str ) : monk_spell_t( p, "mana_tea", p->talent.mistweaver.mana_tea )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.mana_tea->trigger();
  }
};

// ==========================================================================
// Thunder Focus Tea
// ==========================================================================

struct thunder_focus_tea_t : public monk_spell_t
{
  thunder_focus_tea_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "Thunder_focus_tea", p->talent.mistweaver.thunder_focus_tea )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.thunder_focus_tea->trigger( p()->buff.thunder_focus_tea->max_stack() );
  }
};

// ==========================================================================
// Dampen Harm
// ==========================================================================

struct dampen_harm_t : public monk_spell_t
{
  dampen_harm_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "dampen_harm", p->talent.monk.dampen_harm )
  {
    parse_options( options_str );
    cast_during_sck = true;
    harmful         = false;
    base_dd_min     = 0;
    base_dd_max     = 0;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.dampen_harm->trigger();
  }
};

// ==========================================================================
// Diffuse Magic
// ==========================================================================

struct diffuse_magic_t : public monk_spell_t
{
  diffuse_magic_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "diffuse_magic", p->talent.monk.diffuse_magic )
  {
    parse_options( options_str );
    cast_during_sck = player->specialization() != MONK_WINDWALKER;
    harmful         = false;
    base_dd_min     = 0;
    base_dd_max     = 0;
  }

  void execute() override
  {
    p()->buff.diffuse_magic->trigger();
    monk_spell_t::execute();
  }
};

// ==========================================================================
// Invoke Xuen, the White Tiger
// ==========================================================================

// Courage of the White Tiger

struct courage_of_the_white_tiger_t : public monk_melee_attack_t
{
  struct courage_of_the_white_tiger_heal_t : public monk_heal_t
  {
    courage_of_the_white_tiger_heal_t( monk_t *p )
      : monk_heal_t( p, "courage_of_the_white_tiger_heal",
                     p->talent.conduit_of_the_celestials.courage_of_the_white_tiger_heal )
    {
      background = dual = true;
      target            = p;
      base_dd_min = base_dd_max = 1.0;
    }
  };

  courage_of_the_white_tiger_heal_t *heal;

  courage_of_the_white_tiger_t( monk_t *p )
    : monk_melee_attack_t( p, "courage_of_the_white_tiger_dmg",
                           p->talent.conduit_of_the_celestials.courage_of_the_white_tiger_damage ),
      heal( new courage_of_the_white_tiger_heal_t( p ) )
  {
    background = true;
    // allow for 2 seconds for the summoning animation to happen.
    time_to_travel = 2000_ms;

    // we have to set this up by hand, as Unity Within multiplier is scripted
    if ( const auto &effect = p->talent.conduit_of_the_celestials.unity_within_dmg_mult->effectN( 1 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( [ p ]() { return p->buff.unity_within->check(); } )
          .set_value( effect.percent() )
          .set_eff( &effect );
  }

  void execute() override
  {
    p()->buff.strength_of_the_black_ox->trigger();

    p()->buff.inner_compass_tiger_stance->trigger();

    monk_melee_attack_t::execute();
  }

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    double result = s->result_total;

    result *= p()->talent.conduit_of_the_celestials.courage_of_the_white_tiger->effectN( 2 ).percent();

    heal->base_dd_min = heal->base_dd_max = result;
    heal->execute();
  }
};

struct xuen_spell_t : public monk_spell_t
{
  xuen_spell_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "invoke_xuen_the_white_tiger", p->talent.windwalker.invoke_xuen_the_white_tiger )
  {
    parse_options( options_str );

    cast_during_sck = false;
    // Specifically set for 10.1 class trinket
    harmful  = true;
    gcd_type = gcd_haste_type::NONE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->bugs )
    {
      // BUG: Invoke Xuen and Fury of Xuen reset both damage cache to 0 when either spawn
      for ( auto target : p()->sim->target_non_sleeping_list )
      {
        auto td = p()->get_target_data( target );
        if ( td )
        {
          td->debuff.empowered_tiger_lightning->current_value              = 0;
          td->debuff.fury_of_xuen_empowered_tiger_lightning->current_value = 0;
        }
      }
    }

    p()->pets.xuen.spawn( p()->talent.windwalker.invoke_xuen_the_white_tiger->duration(), 1 );

    p()->buff.invoke_xuen->trigger();

    p()->buff.invokers_delight->trigger();

    if ( p()->talent.windwalker.flurry_of_xuen->ok() )
      p()->buff.flurry_of_xuen->trigger();

    p()->buff.courage_of_the_white_tiger->trigger();

    if ( p()->talent.monk.summon_white_tiger_statue->ok() )
      p()->pets.white_tiger_statue.spawn( p()->passives.summon_white_tiger_statue->duration(), 1 );
  }
};

// Fury of Xuen Invoke Xuen =============================================================
struct fury_of_xuen_summon_t final : monk_spell_t
{
  fury_of_xuen_summon_t( monk_t *p ) : monk_spell_t( p, "fury_of_xuen_summon", p->find_spell( 396168 ) )
  {
    cooldown->duration = 0_ms;
    track_cd_waste     = false;
    background         = true;
    // Specifically set for 10.1 class trinket
    harmful = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( !p()->talent.windwalker.fury_of_xuen->ok() )
      return;

    if ( p()->bugs )
    {
      // BUG: Invoke Xuen and Fury of Xuen reset both damage cache to 0 when either spawn
      for ( auto target : p()->sim->target_non_sleeping_list )
      {
        auto td = p()->get_target_data( target );
        if ( td )
        {
          td->debuff.empowered_tiger_lightning->current_value              = 0;
          td->debuff.fury_of_xuen_empowered_tiger_lightning->current_value = 0;
        }
      }
    }

    p()->pets.fury_of_xuen_tiger.spawn( p()->passives.fury_of_xuen->duration(), 1 );

    if ( p()->talent.windwalker.flurry_of_xuen->ok() )
      p()->buff.flurry_of_xuen->trigger();
  }
};

struct empowered_tiger_lightning_t : public monk_spell_t
{
  empowered_tiger_lightning_t( monk_t *p )
    : monk_spell_t( p, "empowered_tiger_lightning", p->passives.empowered_tiger_lightning )
  {
    background = true;
    may_crit   = false;
  }

  bool ready() override
  {
    return p()->baseline.windwalker.empowered_tiger_lightning->ok();
  }
};

struct fury_of_xuen_empowered_tiger_lightning_t : public monk_spell_t
{
  fury_of_xuen_empowered_tiger_lightning_t( monk_t *p )
    : monk_spell_t( p, "empowered_tiger_lightning_fury_of_xuen", p->passives.empowered_tiger_lightning )
  {
    background = true;
    may_crit   = false;
  }

  bool ready() override
  {
    return p()->baseline.windwalker.empowered_tiger_lightning->ok();
  }
};

struct flurry_of_xuen_t : public monk_spell_t
{
  flurry_of_xuen_t( monk_t *p )
    : monk_spell_t( p, "flurry_of_xuen", p->passives.flurry_of_xuen_driver->effectN( 1 )._trigger_spell )
  {
    background = true;
    may_crit   = true;

    aoe                 = -1;
    reduced_aoe_targets = p->talent.windwalker.flurry_of_xuen->effectN( 2 ).base_value();
  }

  double composite_da_multiplier( const action_state_t *s ) const override
  {
    double da = monk_spell_t::composite_da_multiplier( s );

    // Tested 18/06/2024. FLoX does 150% increased damage during Storm Earth, and Fire.
    if ( p()->bugs && p()->buff.storm_earth_and_fire->check() )
      da *= 2.5;

    return da;
  }
};

// ==========================================================================
// Invoke Niuzao, the Black Ox
// ==========================================================================

struct strength_of_the_black_ox_t : public monk_spell_t
{
  strength_of_the_black_ox_t( monk_t *p )
    : monk_spell_t( p, "strength_of_the_black_ox", p->talent.conduit_of_the_celestials.strength_of_the_black_ox_damage )
  {
    background = true;

    aoe                 = -1;
    reduced_aoe_targets = p->talent.conduit_of_the_celestials.strength_of_the_black_ox->effectN( 2 ).base_value();

    // we have to set this up by hand, as Unity Within multiplier is scripted
    if ( const auto &effect = p->talent.conduit_of_the_celestials.unity_within_dmg_mult->effectN( 1 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( [ p ]() { return p->buff.unity_within->check(); } )
          .set_value( effect.percent() )
          .set_eff( &effect );
  }
};

struct strength_of_the_black_ox_absorb_t : public monk_absorb_t
{
  strength_of_the_black_ox_absorb_t( monk_t *p )
    : monk_absorb_t( p, "strength_of_the_black_ox_absorb",
                     p->talent.conduit_of_the_celestials.strength_of_the_black_ox_absorb )
  {
    background  = true;
    aoe         = as<int>( data().effectN( 3 ).base_value() );
    base_dd_min = p->max_health() * data().effectN( 2 ).percent();
    base_dd_max = base_dd_min;
  }
};

struct niuzao_spell_t : public monk_spell_t
{
  niuzao_spell_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "invoke_niuzao_the_black_ox", p->talent.brewmaster.invoke_niuzao_the_black_ox )
  {
    parse_options( options_str );

    cast_during_sck = true;
    // Specifically set for 10.1 class trinket
    harmful = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.niuzao.spawn( p()->talent.brewmaster.invoke_niuzao_the_black_ox->duration(), 1 );

    p()->buff.invoke_niuzao->trigger();

    p()->buff.invokers_delight->trigger();
  }
};

// Call to Arms Invoke Niuzao =============================================================
struct niuzao_call_to_arms_summon_t final : monk_spell_t
{
  niuzao_call_to_arms_summon_t( monk_t *p ) : monk_spell_t( p, "niuzao_call_to_arms_summon", p->find_spell( 395267 ) )
  {
    cooldown->duration = 0_ms;
    track_cd_waste     = false;
    background         = true;
    // Specifically set for 10.1 class trinket
    harmful = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.call_to_arms_invoke_niuzao->trigger();

    p()->pets.call_to_arms_niuzao.spawn( p()->talent.brewmaster.call_to_arms_buff->duration(), 1 );
  }
};

// ==========================================================================
// Invoke Chi-Ji, the Red Crane
// ==========================================================================

struct chiji_spell_t : public monk_spell_t
{
  chiji_spell_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "invoke_chiji_the_red_crane", p->talent.mistweaver.invoke_chi_ji_the_red_crane )
  {
    parse_options( options_str );

    // Specifically set for 10.1 class trinket
    harmful = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.chiji.spawn( p()->talent.mistweaver.invoke_chi_ji_the_red_crane->duration(), 1 );

    p()->buff.invoke_chiji->trigger();

    p()->buff.invokers_delight->trigger();

    p()->buff.courage_of_the_white_tiger->trigger();
  }
};

// ==========================================================================
// Invoke Yu'lon, the Jade Serpent
// ==========================================================================

struct yulon_spell_t : public monk_spell_t
{
  yulon_spell_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "invoke_yulon_the_jade_serpent", p->talent.mistweaver.invoke_yulon_the_jade_serpent )
  {
    parse_options( options_str );

    // Specifically set for 10.1 class trinket
    harmful = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.yulon.spawn( p()->talent.mistweaver.invoke_yulon_the_jade_serpent->duration(), 1 );

    p()->buff.invokers_delight->trigger();

    p()->buff.courage_of_the_white_tiger->trigger();
  }
};

// ==========================================================================
// Unity Within
// ==========================================================================

struct unity_within_t : public monk_spell_t
{
  unity_within_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "unity_within", p->talent.conduit_of_the_celestials.unity_within )
  {
    parse_options( options_str );

    harmful              = false;
    usable_while_casting = true;
  }

  bool ready() override
  {
    return p()->buff.unity_within->up();
  }

  void execute() override
  {
    // Expiring this before the execute so that there is an easy way to apply the damage bonus.
    p()->buff.unity_within->expire();

    monk_spell_t::execute();
  }
};

// ==========================================================================
// Celestial Conduit
// ==========================================================================

// This is a separate spell from the normal Flight of the Red Crane damage spell
struct flight_of_the_red_crane_celestial_dmg_t : public monk_spell_t
{
  flight_of_the_red_crane_celestial_dmg_t( monk_t *p )
    : monk_spell_t( p, "flight_of_the_red_crane_celestial_dmg",
                    p->talent.conduit_of_the_celestials.flight_of_the_red_crane_celestial_dmg )
  {
    background = true;
    aoe        = as<int>( p->talent.conduit_of_the_celestials.flight_of_the_red_crane->effectN( 1 ).base_value() );

    // we have to set this up by hand, as Unity Within multiplier is scripted
    if ( const auto &effect = p->talent.conduit_of_the_celestials.unity_within_dmg_mult->effectN( 1 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( [ p ]() { return p->buff.unity_within->check(); } )
          .set_value( effect.percent() )
          .set_eff( &effect );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.flight_of_the_red_crane->trigger();
  }
};

struct flight_of_the_red_crane_celestial_heal_t : public monk_heal_t
{
  flight_of_the_red_crane_celestial_heal_t( monk_t *p )
    : monk_heal_t( p, "flight_of_the_red_crane_celestial",
                   p->talent.conduit_of_the_celestials.flight_of_the_red_crane_celestial_dmg )
  {
    background = true;
    aoe        = as<int>( p->talent.conduit_of_the_celestials.flight_of_the_red_crane->effectN( 1 ).base_value() );
    target     = p;

    // we have to set this up by hand, as Unity Within multiplier is scripted
    if ( const auto &effect = p->talent.conduit_of_the_celestials.unity_within_dmg_mult->effectN( 1 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( [ p ]() { return p->buff.unity_within->check(); } )
          .set_value( effect.percent() )
          .set_eff( &effect );
  }
};

struct celestial_conduit_t : public monk_spell_t
{
  struct celestial_conduit_dmg_t : public monk_spell_t
  {
    celestial_conduit_dmg_t( monk_t *p )
      : monk_spell_t( p, "celestial_conduit_dmg", p->talent.conduit_of_the_celestials.celestial_conduit_dmg )
    {
      background       = true;
      aoe              = -1;
      split_aoe_damage = true;
    }

    double composite_aoe_multiplier( const action_state_t *state ) const override
    {
      double cam = monk_spell_t::composite_aoe_multiplier( state );

      if ( state->n_targets > 0 )
        cam *= 1 + ( p()->talent.conduit_of_the_celestials.celestial_conduit->effectN( 1 ).percent() *
                     std::min( as<double>( state->n_targets ),
                               p()->talent.conduit_of_the_celestials.celestial_conduit->effectN( 3 ).base_value() ) );

      return cam;
    }
  };

  struct celestial_conduit_heal_t : public monk_heal_t
  {
    celestial_conduit_heal_t( monk_t *p )
      : monk_heal_t( p, "celestial_conduit_heal", p->talent.conduit_of_the_celestials.celestial_conduit_heal )
    {
      background = true;
      target     = p;
    }

    double composite_aoe_multiplier( const action_state_t *state ) const override
    {
      double cam = monk_heal_t::composite_aoe_multiplier( state );

      if ( state->n_targets > 0 )
        cam *= 1 + ( p()->talent.conduit_of_the_celestials.celestial_conduit->effectN( 1 ).percent() *
                     std::min( (double)state->n_targets,
                               p()->talent.conduit_of_the_celestials.celestial_conduit->effectN( 3 ).base_value() ) );

      return cam;
    }
  };

  celestial_conduit_dmg_t *damage;
  celestial_conduit_heal_t *heal;

  celestial_conduit_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "celestial_conduit", p->talent.conduit_of_the_celestials.celestial_conduit ),
      damage( new celestial_conduit_dmg_t( p ) ),
      heal( new celestial_conduit_heal_t( p ) )
  {
    parse_options( options_str );

    may_combo_strike      = true;
    sef_ability           = actions::sef_ability_e::SEF_CELESTIAL_CONDUIT;
    channeled             = true;
    interrupt_auto_attack = false;

    tick_action = damage;

    apply_affecting_aura( p->talent.conduit_of_the_celestials.flight_of_the_red_crane );
    apply_affecting_aura( p->talent.conduit_of_the_celestials.jade_sanctuary );
  }

  bool usable_moving() const override
  {
    return true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.unity_within->trigger();
  }

  void tick( dot_t *d ) override
  {
    monk_spell_t::tick( d );

    heal->execute();
  }

  void last_tick( dot_t *dot ) override
  {
    monk_spell_t::last_tick( dot );

    p()->buff.unity_within->expire();
  }
};

// ==========================================================================
// Chi Surge
// ==========================================================================

struct chi_surge_t : monk_spell_t
{
  chi_surge_t( monk_t *player )
    : monk_spell_t( player, "chi_surge", player->talent.brewmaster.chi_surge->effectN( 1 ).trigger() )
  {
    harmful = true;
    dual = background = true;
    aoe               = -1;

    apply_affecting_aura( player->talent.brewmaster.press_the_advantage );
  }

  double composite_persistent_multiplier( const action_state_t *state ) const override
  {
    return monk_spell_t::composite_persistent_multiplier( state ) / as<double>( state->n_targets );
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( execute_state->n_targets <= 0 )
      return;
    unsigned targets_hit = std::min( 5U, execute_state->n_targets );
    double cdr           = p()->talent.brewmaster.chi_surge->effectN( 1 ).base_value();  // Saved as 4
    p()->cooldown.weapons_of_order->adjust( timespan_t::from_seconds( -1 * cdr * targets_hit ) );
    p()->proc.chi_surge->occur();
  }
};

// ==========================================================================
// Weapons of Order
// ==========================================================================

struct weapons_of_order_t : public monk_spell_t
{
  weapons_of_order_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "weapons_of_order", p->talent.brewmaster.weapons_of_order )
  {
    parse_options( options_str );
    may_combo_strike = true;
    // Specifically set for 10.1 class trinket
    harmful         = p->talent.brewmaster.call_to_arms->ok() ? true : false;
    cast_during_sck = true;

    if ( p->talent.brewmaster.weapons_of_order->ok() && !p->sim->enable_all_talents )
      add_child( p->active_actions.chi_surge );
  }

  void execute() override
  {
    p()->buff.weapons_of_order->trigger();

    monk_spell_t::execute();

    p()->cooldown.keg_smash->reset( true, 1 );

    if ( p()->talent.brewmaster.chi_surge->ok() )
      p()->active_actions.chi_surge->execute();

    if ( p()->talent.brewmaster.call_to_arms->ok() )
      p()->active_actions.niuzao_call_to_arms_summon->execute();

    p()->buff.invoke_niuzao->trigger( p()->talent.brewmaster.call_to_arms_buff->duration() );
  }
};

// ==========================================================================
// Jadefire Stomp
// ==========================================================================

struct jadefire_stomp_ww_damage_t : public monk_spell_t
{
  jadefire_stomp_ww_damage_t( monk_t *p )
    : monk_spell_t( p, "jadefire_stomp_ww_dmg", p->talent.windwalker.jadefire_stomp_ww_damage )
  {
    background = true;
    ww_mastery = true;
  }
};

struct jadefire_stomp_damage_t : public monk_spell_t
{
  jadefire_stomp_damage_t( monk_t *p )
    : monk_spell_t( p, "jadefire_stomp_dmg", p->talent.windwalker.jadefire_stomp_damage )
  {
    background = true;
    ww_mastery = true;

    attack_power_mod.direct = p->talent.windwalker.jadefire_stomp_damage->effectN( 1 ).ap_coeff();
    spell_power_mod.direct  = 0;

    apply_affecting_aura( p->talent.windwalker.singularly_focused_jade );
  }

  double composite_aoe_multiplier( const action_state_t *state ) const override
  {
    double cam = monk_spell_t::composite_aoe_multiplier( state );

    if ( p()->talent.windwalker.path_of_jade->ok() && state->n_targets > 0 )
      cam *=
          1 + ( p()->talent.windwalker.path_of_jade->effectN( 1 ).percent() *
                std::min( (double)state->n_targets, p()->talent.windwalker.path_of_jade->effectN( 2 ).base_value() ) );

    return cam;
  }
};

struct jadefire_stomp_heal_t : public monk_heal_t
{
  jadefire_stomp_heal_t( monk_t *p )
    : monk_heal_t( p, "jadefire_stomp_heal", p->talent.windwalker.jadefire_stomp_damage )
  {
    background = true;

    attack_power_mod.direct = 0;
    spell_power_mod.direct  = p->talent.windwalker.jadefire_stomp_damage->effectN( 2 ).sp_coeff();

    apply_affecting_aura( p->talent.windwalker.singularly_focused_jade );
  }

  void impact( action_state_t *s ) override
  {
    monk_heal_t::impact( s );

    p()->buff.jadefire_brand->trigger();
  }
};

struct jadefire_stomp_t : public monk_spell_t
{
  jadefire_stomp_damage_t *damage;
  jadefire_stomp_heal_t *heal;
  jadefire_stomp_ww_damage_t *ww_damage;
  jadefire_stomp_t( monk_t *p, util::string_view options_str )
    : monk_spell_t( p, "jadefire_stomp", p->shared.jadefire_stomp )
  {
    parse_options( options_str );
    may_combo_strike = true;
    cast_during_sck  = player->specialization() != MONK_WINDWALKER;
    gcd_type         = gcd_haste_type::NONE;  // Need to define this manually for some reason

    damage = new jadefire_stomp_damage_t( p );
    heal   = new jadefire_stomp_heal_t( p );
    aoe    = as<int>( data().effectN( 1 ).base_value() );

    apply_affecting_aura( p->talent.windwalker.singularly_focused_jade );
    aoe += as<int>( p->talent.windwalker.singularly_focused_jade->effectN( 1 ).base_value() );

    if ( p->specialization() == MONK_WINDWALKER )
    {
      ww_damage = new jadefire_stomp_ww_damage_t( p );

      add_child( ww_damage );
    }

    add_child( damage );
    add_child( heal );
  }

  std::vector<player_t *> &target_list() const override
  {
    // Check if target cache is still valid. If not, recalculate it
    if ( !target_cache.is_valid )
    {
      available_targets( target_cache.list );  // This grabs the full list of targets, which will also pickup various
                                               // awfulness that some classes have.. such as prismatic crystal.
      if ( sim->distance_targeting_enabled )
        check_distance_targeting( target_cache.list );
      target_cache.is_valid = true;
    }

    if ( !target_cache.list.empty() )
    {
      // Prioritize enemies / players that do not have jadefire brand
      // the ability does not do this inherently but it is assumed that an observant player would
      range::sort( target_cache.list, [ this ]( player_t *left, player_t *right ) {
        return get_td( left )->debuff.jadefire_brand->remains().total_millis() <
               get_td( right )->debuff.jadefire_brand->remains().total_millis();
      } );
    }

    return target_cache.list;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.jadefire_stomp->trigger();
    p()->buff.jadefire_brand->trigger();
    p()->buff.august_dynasty->trigger();
  }

  void impact( action_state_t *s ) override
  {
    monk_spell_t::impact( s );

    heal->execute();

    damage->execute_on_target( s->target );

    if ( p()->specialization() == MONK_WINDWALKER )
      ww_damage->execute_on_target( s->target );

    get_td( s->target )->debuff.jadefire_brand->trigger();
  }
};

// ==========================================================================
// Gale Force
// ==========================================================================

struct gale_force_t : public monk_spell_t
{
  gale_force_t( monk_t *p ) : monk_spell_t( p, "gale_force", p->talent.windwalker.gale_force_damage )
  {
    background  = true;
    base_dd_min = base_dd_max = 1;
  }
};
}  // namespace spells

namespace heals
{
// ==========================================================================
// Soothing Mist
// ==========================================================================
/*
struct soothing_mist_t : public monk_heal_t
{
  soothing_mist_t( monk_t *p ) : monk_heal_t( p, "soothing_mist", p->passives.soothing_mist_heal )
  {
    background = dual = true;

    tick_zero = true;
  }

  bool ready() override
  {
    if ( p()->buff.channeling_soothing_mist->check() )
      return false;

    return monk_heal_t::ready();
  }

  void impact( action_state_t *s ) override
  {
    monk_heal_t::impact( s );

    p()->buff.channeling_soothing_mist->trigger();
  }

  void last_tick( dot_t *d ) override
  {
    monk_heal_t::last_tick( d );

    p()->buff.channeling_soothing_mist->expire();
  }
}; */

// ==========================================================================
// Gust of Mists
// ==========================================================================
// The mastery actually affects the Spell Power Coefficient but I am not sure if that
// would work normally. Using Action Multiplier since it APPEARS to calculate the same.
//
// TODO: Double Check if this works.

struct gust_of_mists_t : public monk_heal_t
{
  gust_of_mists_t( monk_t *p )
    : monk_heal_t( p, "gust_of_mists", p->baseline.mistweaver.mastery->effectN( 2 ).trigger() )
  {
    background = dual      = true;
    spell_power_mod.direct = 1;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    am *= p()->cache.mastery_value();

    // Mastery's Effect 3 gives a flat add modifier of 0.1 Spell Power co-efficient
    // TODO: Double check calculation

    return am;
  }
};

// ==========================================================================
// Enveloping Mist
// ==========================================================================

struct enveloping_mist_t : public monk_heal_t
{
  gust_of_mists_t *mastery;

  enveloping_mist_t( monk_t *p, util::string_view options_str )
    : monk_heal_t( p, "enveloping_mist", p->talent.mistweaver.enveloping_mist )
  {
    parse_options( options_str );

    may_miss = false;
    target   = p;

    dot_duration = p->talent.mistweaver.enveloping_mist->duration();
    if ( p->talent.mistweaver.mist_wrap->ok() )
      dot_duration += p->talent.mistweaver.mist_wrap->effectN( 1 ).time_value();

    mastery = new gust_of_mists_t( p );
  }

  double execute_time_pct_multiplier() const override
  {
    auto mul = monk_heal_t::execute_time_pct_multiplier();

    mul *= 1 + p()->talent.mistweaver.thunder_focus_tea->effectN( 3 ).percent();  // saved as -100

    return mul;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->talent.mistweaver.lifecycles->ok() )
    {
      p()->buff.lifecycles_enveloping_mist->expire();

      p()->buff.lifecycles_vivify->trigger();
    }

    p()->buff.thunder_focus_tea->decrement();

    mastery->execute();
  }
};

// ==========================================================================
// Renewing Mist
// ==========================================================================
/*
Bouncing only happens when overhealing, so not going to bother with bouncing

struct renewing_mist_t : public monk_heal_t
{
  gust_of_mists_t *mastery;

  renewing_mist_t( monk_t *p, util::string_view options_str )
    : monk_heal_t( p, "renewing_mist", p->talent.mistweaver.renewing_mist )
  {
    parse_options( options_str );
    may_crit = may_miss = false;
    dot_duration        = p->passives.renewing_mist_heal->duration();

    mastery = new gust_of_mists_t( p );
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown->duration;

    if ( p()->buff.thunder_focus_tea->check() )
      cd *= 1 + p()->talent.mistweaver.thunder_focus_tea->effectN( 1 ).percent();

    monk_heal_t::update_ready( cd );
  }

  void execute() override
  {
    monk_heal_t::execute();

    mastery->execute();

    p()->buff.thunder_focus_tea->decrement();
  }
}; */

// ==========================================================================
// Vivify
// ==========================================================================

struct vivify_t : public monk_heal_t
{
  gust_of_mists_t *mastery;

  vivify_t( monk_t *p, util::string_view options_str ) : monk_heal_t( p, "vivify", p->baseline.monk.vivify )
  {
    parse_options( options_str );

    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();

    base_execute_time += p->talent.monk.vivacious_vivification->effectN( 1 ).time_value();

    mastery = new gust_of_mists_t( p );
  }

  double cost_pct_multiplier() const override
  {
    double c = monk_heal_t::cost_pct_multiplier();

    if ( p()->buff.thunder_focus_tea->check() )
      c *= 1 + p()->talent.mistweaver.thunder_focus_tea->effectN( 2 ).percent();  // saved as -100

    return c;
  }

  void execute() override
  {
    monk_heal_t::execute();

    p()->buff.thunder_focus_tea->decrement();

    if ( p()->talent.mistweaver.lifecycles )
    {
      p()->buff.lifecycles_vivify->expire();

      p()->buff.lifecycles_enveloping_mist->trigger();
    }

    if ( p()->baseline.mistweaver.mastery->ok() )
      mastery->execute();

    p()->active_actions.chi_wave->execute();
  }
};

// ==========================================================================
// Revival
// ==========================================================================

struct revival_t : public monk_heal_t
{
  revival_t( monk_t *p, util::string_view options_str ) : monk_heal_t( p, "revival", p->talent.mistweaver.revival )
  {
    parse_options( options_str );

    may_miss = false;
    aoe      = -1;
  }
};

// ==========================================================================
// Expel Harm
// ==========================================================================
struct expel_harm_t : monk_heal_t
{
  struct damage_t : monk_spell_t
  {
    damage_t( monk_t *player ) : monk_spell_t( player, "expel_harm_damage", player->baseline.monk.expel_harm_damage )
    {
      background = dual = true;
      base_dd_min = base_dd_max = 1.0;
    }
  };

  damage_t *damage;

  expel_harm_t( monk_t *player, std::string_view options_str )
    : monk_heal_t( player, "expel_harm",
                   player->talent.windwalker.combat_wisdom->ok() ? player->talent.windwalker.combat_wisdom_expel_harm
                                                                 : player->baseline.monk.expel_harm ),
      damage( new damage_t( player ) )
  {
    parse_options( options_str );
    may_combo_strike = false;
    cast_during_sck  = player->specialization() != MONK_WINDWALKER;
    if ( player->talent.windwalker.combat_wisdom->ok() )
      background = true;

    apply_affecting_aura( player->baseline.brewmaster.expel_harm_rank_2 );
    apply_affecting_aura( player->talent.monk.vigorous_expulsion );
    apply_affecting_aura( player->talent.monk.profound_rebuttal );

    add_child( damage );
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();
    if ( !p()->talent.monk.strength_of_spirit->ok() )
      return am;
    am *=
        1.0 + ( 1.0 - p()->health_percentage() / 100.0 ) * p()->talent.monk.strength_of_spirit->effectN( 1 ).percent();
    return am;
  }

  void execute() override
  {
    if ( p()->talent.brewmaster.gift_of_the_ox->ok() )
      p()->buff.expel_harm_accumulator->trigger();

    monk_heal_t::execute();

    if ( !p()->talent.brewmaster.tranquil_spirit->ok() )
      return;
    double percent = p()->talent.brewmaster.tranquil_spirit->effectN( 1 ).percent();
    p()->find_stagger( "Stagger" )->purify_percent( percent, "tranquil_spirit_eh" );
    p()->proc.tranquil_spirit_expel_harm->occur();
  }

  void impact( action_state_t *s ) override
  {
    monk_heal_t::impact( s );

    double result = s->result_total;
    if ( p()->buff.gift_of_the_ox && p()->buff.gift_of_the_ox->up() &&
         p()->baseline.brewmaster.expel_harm_rank_2->ok() )
    {
      p()->buff.gift_of_the_ox->consume( 5 );
      result += p()->buff.expel_harm_accumulator->check_value();
      p()->buff.expel_harm_accumulator->expire();
    }
    result *= data().effectN( 2 ).percent();

    damage->base_dd_min = damage->base_dd_max = result;
    damage->execute();
  }
};

// ==========================================================================
// Zen Pulse
// ==========================================================================
/*
struct zen_pulse_echo_heal_t : public monk_heal_t
{
  zen_pulse_echo_heal_t( monk_t *p ) : monk_heal_t( p, "zen_pulse_echo_heal", p->passives.zen_pulse_echo_heal )
  {
    background  = true;
    target      = p;
    trigger_gcd = timespan_t::zero();
  }

  bool ready() override
  {
    return false;
  }
};

struct zen_pulse_echo_dmg_t : public monk_spell_t
{
  zen_pulse_echo_heal_t *heal;
  zen_pulse_echo_dmg_t( monk_t *player )
    : monk_spell_t( player, "zen_pulse_echo_damage", player->passives.zen_pulse_echo_damage )
  {
    background  = true;
    target      = player->target;
    aoe         = -1;
    trigger_gcd = timespan_t::zero();
    //          spell_power_mod.direct = player->passives.zen_pulse_echo_damage->effectN( 2 ).sp_coeff();
    spell_power_mod.tick = 0;

    heal = new zen_pulse_echo_heal_t( player );
  }

  double cost() const override
  {
    return 0;
  }

  bool ready() override
  {
    return false;
  }

  void impact( action_state_t *s ) override
  {
    monk_spell_t::impact( s );

    heal->execute();
  }
};

struct zen_pulse_heal_t : public monk_heal_t
{
  zen_pulse_heal_t( monk_t *p ) : monk_heal_t( p, "zen_pulse_heal", p->passives.zen_pulse_heal )
  {
    background  = true;
    trigger_gcd = timespan_t::zero();
    target      = p;
  }
};

struct zen_pulse_dmg_t : public monk_spell_t
{
  zen_pulse_heal_t *heal;
  zen_pulse_dmg_t( monk_t *player )
    : monk_spell_t( player, "zen_pulse_damage", player->find_spell( 124081 ) ), heal( new zen_pulse_heal_t( player ) )
  {
    background           = true;
    target               = player->target;
    trigger_gcd          = timespan_t::zero();
    aoe                  = -1;
    spell_power_mod.tick = 0;
  }

  double cost() const override
  {
    return 0;
  }

  void impact( action_state_t *s ) override
  {
    monk_spell_t::impact( s );

    heal->execute();
  }
};

struct zen_pulse_t : public monk_spell_t
{
  spell_t *damage;
  zen_pulse_echo_dmg_t *echo;
  zen_pulse_t( monk_t *player, util::string_view options_str )
    : monk_spell_t( player, "zen_pulse", player->talent.mistweaver.zen_pulse ),
      damage( new zen_pulse_dmg_t( player ) ),
      echo( new zen_pulse_echo_dmg_t( player ) )
  {
    parse_options( options_str );
    may_miss = may_dodge = may_parry = false;

    add_child( damage );
    add_child( echo );
  }

  void execute() override
  {
    monk_spell_t::execute();
    damage->execute();

    if ( p()->get_dot( "enveloping_mist", p() )->is_ticking() )
      echo->execute();
  }
}; */

// ==========================================================================
// Chi Wave
// ==========================================================================
struct chi_wave_t : public monk_spell_t
{
  template <class TBase>
  struct bounce_t : TBase
  {
    using TBase::execute;
    std::function<void( unsigned )> other_cb;
    std::function<void( unsigned )> this_cb;
    unsigned count;

    bounce_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
      : TBase( player, fmt::format( "chi_wave_{}", name ), spell_data ), count( 0 )
    {
      TBase::dual         = true;
      TBase::travel_speed = player->talent.monk.chi_wave_driver->missile_speed();
      this_cb             = [ this ]( unsigned new_count ) { this->execute( new_count ); };
    }

    void execute( unsigned new_count )
    {
      count = new_count;
      if ( count > TBase::p()->talent.monk.chi_wave_driver->effectN( 1 ).base_value() )
        return;

      TBase::execute();
    }

    void impact( action_state_t *state ) override
    {
      TBase::impact( state );
      other_cb( ++count );
    }
  };

  bounce_t<monk_heal_t> *heal;
  bounce_t<monk_spell_t> *damage;

  chi_wave_t( monk_t *player )
    : monk_spell_t( player, "chi_wave", player->talent.monk.chi_wave_driver ),
      heal( new bounce_t<monk_heal_t>( player, "heal", player->talent.monk.chi_wave_heal ) ),
      damage( new bounce_t<monk_spell_t>( player, "damage", player->talent.monk.chi_wave_damage ) )
  {
    background       = true;
    may_combo_strike = false;

    heal->other_cb   = damage->this_cb;
    damage->other_cb = heal->this_cb;

    damage->sef_ability = sef_ability_e::SEF_CHI_WAVE;

    stats = damage->stats;

    add_child( heal );
  }

  void execute() override
  {
    if ( !p()->buff.chi_wave->up() )
      return;
    p()->buff.aspect_of_harmony.trigger_path_of_resurgence();
    p()->buff.chi_wave->expire();
    monk_spell_t::execute();

    if ( player->target->is_enemy() )
      damage->execute( 0 );
    else
      heal->execute( 0 );
  }
};

// ==========================================================================
// Chi Burst
// ==========================================================================
struct chi_burst_t : monk_spell_t
{
  template <class TBase>
  struct hit_t : TBase
  {
    hit_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
      : TBase( player, fmt::format( "chi_burst_{}", name ), spell_data )
    {
      TBase::background = TBase::dual = true;
      TBase::reduced_aoe_targets      = player->talent.monk.chi_burst_projectile->effectN( 1 ).base_value();
      TBase::aoe                      = -1;

      // TODO: Helper to check if a damaging effect exists on the passed spell
      for ( const auto &effect : spell_data->effects() )
        if ( effect.type() == E_SCHOOL_DAMAGE )
          TBase::ww_mastery = true;
    }
  };

  hit_t<monk_spell_t> *damage;
  hit_t<monk_heal_t> *heal;
  propagate_const<buff_t *> buff;

  // TODO: Figure out what you have to do to simulate this as a projectile.
  chi_burst_t( monk_t *player, std::string_view options_str )
    : monk_spell_t( player, "chi_burst",
                    player->specialization() == MONK_WINDWALKER ? player->talent.monk.chi_burst_projectile
                                                                : player->talent.monk.chi_burst ),
      damage( new hit_t<monk_spell_t>( player, "damage", player->talent.monk.chi_burst_damage ) ),
      heal( new hit_t<monk_heal_t>( player, "heal", player->talent.monk.chi_burst_heal ) ),
      buff( buff_t::find( player, "chi_burst" ) )
  {
    parse_options( options_str );
    may_combo_strike = true;
    gcd_type         = gcd_haste_type::NONE;

    stats = damage->stats;
    add_child( heal );
  }

  bool ready() override
  {
    if ( p()->specialization() != MONK_WINDWALKER )
      return monk_spell_t::ready();
    if ( buff && buff->up() )
      return monk_spell_t::ready();
    return false;
  }

  void execute() override
  {
    p()->buff.aspect_of_harmony.trigger_path_of_resurgence();
    monk_spell_t::execute();

    if ( buff )
    {
      if ( p()->bugs )
        buff->expire();
      else
        buff->decrement();
    }

    damage->execute();
    heal->execute();
  }
};

// ==========================================================================
// Healing Elixirs
// ==========================================================================

struct healing_elixir_t : public monk_heal_t
{
  healing_elixir_t( monk_t *p ) : monk_heal_t( p, "healing_elixir", p->shared.healing_elixir )
  {
    harmful = may_crit = false;
    target             = p;
    base_pct_heal      = data().effectN( 1 ).percent();
  }
};

// ==========================================================================
// Refreshing Jade Wind
// ==========================================================================

struct refreshing_jade_wind_heal_t : public monk_heal_t
{
  refreshing_jade_wind_heal_t( monk_t *player )
    : monk_heal_t( player, "refreshing_jade_wind_heal",
                   player->talent.mistweaver.refreshing_jade_wind->effectN( 1 ).trigger() )
  {
    background = true;
    aoe        = 6;
  }
};

struct refreshing_jade_wind_t : public monk_spell_t
{
  refreshing_jade_wind_heal_t *heal;
  refreshing_jade_wind_t( monk_t *player, util::string_view options_str )
    : monk_spell_t( player, "refreshing_jade_wind", player->talent.mistweaver.refreshing_jade_wind )
  {
    parse_options( options_str );
    heal = new refreshing_jade_wind_heal_t( player );
  }

  void tick( dot_t *d ) override
  {
    monk_spell_t::tick( d );

    heal->execute();
  }
};

// ==========================================================================
// Celestial Fortune
// ==========================================================================
// This is a Brewmaster-specific critical strike effect

struct celestial_fortune_t : public monk_heal_t
{
  celestial_fortune_t( monk_t *p )
    : monk_heal_t( p, "celestial_fortune", p->baseline.brewmaster.celestial_fortune_heal )
  {
    background = true;
    proc       = true;
    target     = player;
    may_crit   = false;

    base_multiplier = p->baseline.brewmaster.celestial_fortune->effectN( 1 ).percent();
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_heal_t::init();
    // disable the snapshot_flags for all multipliers, but specifically allow
    // action_multiplier() to be called so we can override.
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_MUL_DA;
  }
};
}  // namespace heals

namespace absorbs
{
// ==========================================================================
// Celestial Brew
// ==========================================================================
struct celestial_brew_t : public brew_t<monk_absorb_t>
{
  struct celestial_brew_t_state_t : public action_state_t
  {
    celestial_brew_t_state_t( action_t *a, player_t *target ) : action_state_t( a, target )
    {
    }

    proc_types2 cast_proc_type2() const override
    {
      // Celestial Brew seems to trigger Bron's Call to Action (and possibly other
      // effects that care about casts).
      return PROC2_CAST_HEAL;
    }
  };

  celestial_brew_t( monk_t *p, util::string_view options_str )
    : brew_t<monk_absorb_t>( p, "celestial_brew", p->talent.brewmaster.celestial_brew )
  {
    parse_options( options_str );
    harmful = may_crit = false;
    callbacks          = true;
    cast_during_sck    = true;

    apply_affecting_aura( p->talent.brewmaster.light_brewing );
    apply_affecting_aura( p->talent.master_of_harmony.endless_draught );
  }

  action_state_t *new_state() override
  {
    return new celestial_brew_t_state_t( this, player );
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    am *= 1 + p()->buff.purified_chi->check_stack_value();

    return am;
  }

  void execute() override
  {
    if ( p()->buff.blackout_combo->up() )
    {
      p()->buff.purified_chi->trigger( (int)p()->talent.brewmaster.blackout_combo->effectN( 6 ).base_value() );
      p()->proc.blackout_combo_celestial_brew->occur();
    }

    p()->buff.aspect_of_harmony.trigger_spend();
    brew_t<monk_absorb_t>::execute();

    p()->buff.blackout_combo->expire();
    p()->buff.purified_chi->expire();
    p()->buff.pretense_of_instability->trigger();
    p()->active_actions.special_delivery->execute();
  }
};

// ==========================================================================
// Life Cocoon
// ==========================================================================
struct life_cocoon_t : public monk_absorb_t
{
  life_cocoon_t( monk_t *p, util::string_view options_str )
    : monk_absorb_t( p, "life_cocoon", p->talent.mistweaver.life_cocoon )
  {
    parse_options( options_str );
    harmful = may_crit = false;

    base_dd_min = p->max_health() * p->talent.mistweaver.life_cocoon->effectN( 3 ).percent();
    base_dd_max = base_dd_min;
  }

  void impact( action_state_t *s ) override
  {
    p()->buff.life_cocoon->trigger( 1, s->result_amount );
    stats->add_result( 0.0, s->result_amount, result_amount_type::ABSORB, s->result, s->block_result, s->target );
  }
};
}  // namespace absorbs

template <class base_action_t>
template <typename... Args>
brew_t<base_action_t>::brew_t( monk_t *player, Args &&...args ) : base_action_t( player, std::forward<Args>( args )... )
{
  player->baseline.brewmaster.brews.insert_cooldown( this );
}

template <class base_action_t>
void brew_t<base_action_t>::execute()
{
  base_action_t::execute();
  base_action_t::p()->buff.celestial_flames->trigger();
}

void brews_t::insert_cooldown( action_t *action )
{
  cooldowns.insert( { action->id, action->cooldown } );
}

void brews_t::adjust( timespan_t reduction )
{
  for ( auto &[ key, cooldown ] : cooldowns )
  {
    cooldown->adjust( -reduction );
    cooldown->sim.print_debug( "reducing cooldown of brew ({}) by {}", cooldown->name(), reduction );
  }
}

using namespace pets;
using namespace pet_summon;
using namespace attacks;
using namespace spells;
using namespace heals;
using namespace absorbs;
}  // namespace actions

namespace buffs
{
using namespace actions;
// ==========================================================================
// Monk Buffs
// ==========================================================================

// ==========================================================================
// Gift of the Ox
// ==========================================================================
gift_of_the_ox_t::gift_of_the_ox_t( monk_t *player )
  : monk_buff_t( player, "gift_of_the_ox", player->talent.brewmaster.gift_of_the_ox_buff ),
    player( player ),
    heal_trigger(
        new orb_t( player, "gift_of_the_ox_trigger", player->talent.brewmaster.gift_of_the_ox_heal_trigger ) ),
    heal_expire( new orb_t( player, "gift_of_the_ox_expire", player->talent.brewmaster.gift_of_the_ox_heal_expire ) ),
    accumulator( 0.0 )
{
  // we're just using buff tracking to provide stats.
  // stack changes are all controlled by the events we create, so duration is set
  // to be extra high
  set_max_stack( 5 );
  set_duration( 2 * buff_duration() );
  set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  set_refresh_behavior( buff_refresh_behavior::NONE );
}

void gift_of_the_ox_t::spawn_orb( int count )
{
  if ( is_fallback )
    return;

  int overflow = std::max( count + as<int>( queue.size() ) - max_stack(), 0 );
  player->sim->print_debug( "{} adding {} Gift of the Ox Orbs. start={} apply={} overflow={} end={}", player->name(),
                            count, queue.size(), count, overflow,
                            std::min( count + as<int>( queue.size() ), max_stack() ) );

  for ( ; count > 0; --count )
  {
    monk_buff_t::trigger();
    if ( as<int>( queue.size() ) == max_stack() )
      heal_trigger->execute();
    else
      queue.emplace( make_event<orb_event_t>( *sim, player, data().duration(), &queue, [ this ]() {
        player->sim->print_debug( "{} expiring 1 out of {} Gift of the Ox Orbs. current={} expire={}", player->name(),
                                  queue.size(), queue.size(), 1 );
        decrement();
        heal_expire->execute();
      } ) );
  }
}

void gift_of_the_ox_t::trigger_from_damage( double amount )
{
  if ( is_fallback )
    return;

  accumulator += amount;
  if ( accumulator < player->max_health() )
    return;

  int added = std::lround( accumulator / player->max_health() );
  accumulator -= added * player->max_health();
  spawn_orb( added );
}

int gift_of_the_ox_t::consume( int count )
{
  if ( is_fallback )
    return 0;

  int available = std::min( count, as<int>( queue.size() ) );
  player->sim->print_debug( "{} consuming {} out of {} Gift of the Ox Orbs. start={} quantity={} available={} end={}",
                            player->name(), available, queue.size(), queue.size(), count, available,
                            queue.size() - available );
  for ( int i = available; i > 0; --i )
  {
    heal_trigger->execute();
    remove();
  }
  return available;
}

void gift_of_the_ox_t::remove()
{
  event_t::cancel( queue.front() );
  queue.pop();
}

void gift_of_the_ox_t::reset()
{
  if ( is_fallback )
    return;

  monk_buff_t::reset();
  accumulator = 0.0;
  while ( !queue.empty() )
  {
    remove();
  }
}

gift_of_the_ox_t::orb_t::orb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_heal_t( player, name, spell_data )
{
  background = true;
}

double gift_of_the_ox_t::orb_t::action_multiplier() const
{
  double am = monk_heal_t::action_multiplier();
  if ( p()->talent.monk.strength_of_spirit->ok() && p()->buff.expel_harm_accumulator->check() )
    am *= 1.0 + ( 1.0 - std::max( p()->health_percentage() / 100.0, 0.0 ) ) *
                    p()->talent.monk.strength_of_spirit->effectN( 1 ).percent();
  return am;
}

void gift_of_the_ox_t::orb_t::impact( action_state_t *state )
{
  monk_heal_t::impact( state );

  if ( p()->talent.brewmaster.tranquil_spirit->ok() )
  {
    double percent = p()->talent.brewmaster.tranquil_spirit->effectN( 1 ).percent();
    p()->find_stagger( "Stagger" )->purify_percent( percent, "tranquil_spirit_goto" );
    p()->proc.tranquil_spirit_goto->occur();
  }

  if ( !p()->buff.expel_harm_accumulator->check() )
    return;

  double current = p()->buff.expel_harm_accumulator->check_value();
  double total   = current + state->result_amount;
  sim->print_debug( "{} adding {} to Expel Harm accumulator. current={} add={} total={}", p()->name(),
                    state->result_total, current, state->result_total, total );
  p()->buff.expel_harm_accumulator->trigger( 1, total );
}

gift_of_the_ox_t::orb_event_t::orb_event_t( monk_t *player, timespan_t duration, std::queue<orb_event_t *> *queue,
                                            std::function<void()> expire_cb )
  : event_t( *player, duration ), queue( queue ), expire_cb( std::move( expire_cb ) )
{
}

void gift_of_the_ox_t::orb_event_t::execute()
{
  assert( id == queue->front()->id );
  expire_cb();
  queue->pop();
}

const char *gift_of_the_ox_t::orb_event_t::name() const
{
  return "orb_event_t";
}

// ==========================================================================
// Shuffle
// ==========================================================================
shuffle_t::shuffle_t( monk_t *player )
  : monk_buff_t( player, "shuffle", player->talent.brewmaster.shuffle_buff ),
    accumulator( 0_s ),
    max_duration( 3.0 * base_buff_duration )
{
  set_trigger_spell( player->talent.brewmaster.shuffle );
}

void shuffle_t::trigger( timespan_t duration )
{
  accumulator += duration;

  duration = std::min( duration + remains(), max_duration );
  monk_buff_t::extend_duration_or_trigger( duration );

  p().cooldown.invoke_niuzao->adjust( p().talent.brewmaster.walk_with_the_ox->effectN( 2 ).time_value() );

  if ( !p().talent.brewmaster.quick_sip->ok() )
    return;

  // when you apply a shuffle refresh/application, quick sip's value is multiplied
  // by threshold // accumulator, where // refers to integer division
  timespan_t threshold = timespan_t::from_seconds( p().talent.brewmaster.quick_sip->effectN( 2 ).base_value() );
  int count            = as<int>( timespan_t::to_native( accumulator ) / timespan_t::to_native( threshold ) );
  if ( count > 0 )
    p().find_stagger( "Stagger" )
        ->purify_percent( as<double>( count ) * p().talent.brewmaster.quick_sip->effectN( 1 ).percent(), "quick_sip" );
  accumulator -= threshold * count;
}

// ===============================================================================
// Fortifying Brew Buff
// ===============================================================================
struct fortifying_brew_t : public monk_buff_t
{
  int health_gain;
  fortifying_brew_t( monk_t *player )
    : monk_buff_t( player, "fortifying_brew", player->talent.monk.fortifying_brew ), health_gain( 0 )
  {
    cooldown->duration = timespan_t::zero();
    set_trigger_spell( player->talent.monk.fortifying_brew );
    add_invalidate( CACHE_DODGE );
    add_invalidate( CACHE_ARMOR );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    double health_multiplier = p().talent.monk.fortifying_brew->effectN( 1 ).percent();

    // TODO: Fix spell data
    // if ( p().talent.brewmaster.fortifying_brew_determination->ok() )
    //   health_multiplier = p().talent.monk.fortifying_brew->effectN( 6 ).percent();

    // Extra Health is set by current max_health, doesn't change when max_health changes.
    health_gain = static_cast<int>( p().max_health() * health_multiplier );

    p().stat_gain( STAT_MAX_HEALTH, health_gain, (gain_t *)nullptr, (action_t *)nullptr, true );
    p().stat_gain( STAT_HEALTH, health_gain, (gain_t *)nullptr, (action_t *)nullptr, true );
    return monk_buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    monk_buff_t::expire_override( expiration_stacks, remaining_duration );
    p().stat_loss( STAT_MAX_HEALTH, health_gain, (gain_t *)nullptr, (action_t *)nullptr, true );
    p().stat_loss( STAT_HEALTH, health_gain, (gain_t *)nullptr, (action_t *)nullptr, true );
  }
};

// ===============================================================================
// Touch of Karma Buff
// ===============================================================================
struct touch_of_karma_buff_t : public monk_buff_t
{
  touch_of_karma_buff_t( monk_t *p, util::string_view n, const spell_data_t *s ) : monk_buff_t( p, n, s )
  {
    default_value = 0;
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( p->baseline.windwalker.touch_of_karma );

    set_duration( s->duration() );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // Make sure the value is reset upon each trigger
    current_value = 0;

    return monk_buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    monk_buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Whirling Dragon Punch Buff
// ===============================================================================
struct whirling_dragon_punch_buff_t : monk_buff_t
{
  whirling_dragon_punch_buff_t( monk_t *player )
    : monk_buff_t( player, "whirling_dragon_punch", player->talent.windwalker.whirling_dragon_punch_buff )
  {
    // current measured value for grace period
    // partial testing as of 01/08/2024 dd/mm/yy
    base_buff_duration = 1500_ms;
    set_refresh_behavior( buff_refresh_behavior::NONE );
  }

  bool trigger( int, double, double, timespan_t ) override
  {
    timespan_t buff_duration =
        std::min( p().cooldown.rising_sun_kick->remains(), p().cooldown.fists_of_fury->remains() );

    if ( buff_duration > 0_ms )
      return monk_buff_t::trigger( -1, DEFAULT_VALUE(), -1.0, base_buff_duration + buff_duration );

    return false;
  }
};

// ===============================================================================
// Rushing Jade Wind Buff
// ===============================================================================
struct rushing_jade_wind_buff_t : public monk_buff_t
{
  struct tick_action_t : monk_melee_attack_t
  {
    tick_action_t( monk_t *p ) : monk_melee_attack_t( p, "rushing_jade_wind_tick", p->passives.rushing_jade_wind_tick )
    {
      ww_mastery = true;

      dual = background   = true;
      aoe                 = -1;
      reduced_aoe_targets = p->passives.rushing_jade_wind->effectN( 1 ).base_value();

      // Merge action statistics if RJW exists as an active ability
      if ( const action_t *action = p->find_action( "rushing_jade_wind" ); action )
        stats = action->stats;
    }
  };

  timespan_t _period;
  action_t *rushing_jade_wind_tick;

  rushing_jade_wind_buff_t( monk_t *player )
    : monk_buff_t( player, "rushing_jade_wind", player->passives.rushing_jade_wind ),
      rushing_jade_wind_tick( new tick_action_t( player ) )
  {
    set_tick_time_behavior( buff_tick_time_behavior::CUSTOM );
    set_tick_time_callback( [ this ]( const buff_t *, unsigned int ) { return _period; } );

    set_tick_callback( [ this ]( buff_t *, int, timespan_t ) { rushing_jade_wind_tick->execute(); } );
    set_tick_behavior( buff_tick_behavior::REFRESH );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

    apply_affecting_aura( player->talent.conduit_of_the_celestials.yulons_knowledge );
    apply_affecting_aura( player->baseline.brewmaster.aura );
    apply_affecting_aura( player->baseline.windwalker.aura );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // RJW snapshots the tick period on cast.
    if ( duration == timespan_t::min() )
    {
      duration = monk_buff_t::buff_duration();
      duration *= p().cache.spell_cast_speed();
    }

    _period = monk_buff_t::buff_period * p().cache.spell_cast_speed();

    return monk_buff_t::trigger( stacks, value, chance, duration );
  }
};

// ===============================================================================
// Invoke Xuen the White Tiger
// ===============================================================================

struct invoke_xuen_the_white_tiger_buff_t : public monk_buff_t
{
  static void invoke_xuen_callback( buff_t *b, int, timespan_t )
  {
    auto *p = debug_cast<monk_t *>( b->player );
    if ( p->baseline.windwalker.empowered_tiger_lightning->ok() )
    {
      double empowered_tiger_lightning_multiplier =
          p->baseline.windwalker.empowered_tiger_lightning->effectN( 2 ).percent();

      for ( auto target : p->sim->target_non_sleeping_list )
      {
        if ( p->find_target_data( target ) )
        {
          auto td = p->get_target_data( target );
          if ( td->debuff.empowered_tiger_lightning->up() )
          {
            double value                                        = td->debuff.empowered_tiger_lightning->check_value();
            td->debuff.empowered_tiger_lightning->current_value = 0;
            if ( value > 0 )
            {
              p->active_actions.empowered_tiger_lightning->base_dd_min = value * empowered_tiger_lightning_multiplier;
              p->active_actions.empowered_tiger_lightning->base_dd_max = value * empowered_tiger_lightning_multiplier;
              p->active_actions.empowered_tiger_lightning->execute_on_target( target );
            }
          }
        }
      }
    }
  }

  invoke_xuen_the_white_tiger_buff_t( monk_t *p, util::string_view n, const spell_data_t *s ) : monk_buff_t( p, n, s )
  {
    set_cooldown( timespan_t::zero() );
    set_duration( s->duration() );

    set_period( s->effectN( 2 ).period() );

    set_tick_callback( invoke_xuen_callback );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( buff_t::trigger( stacks, value, chance, duration ) )
    {
      if ( p().talent.conduit_of_the_celestials.restore_balance->ok() )
        p().buff.rushing_jade_wind->trigger( remains() );

      return true;
    }

    return false;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    monk_buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Fury of Xuen Stacking Buff
// ===============================================================================
struct fury_of_xuen_stacking_buff_t : public monk_buff_t
{
  fury_of_xuen_stacking_buff_t( monk_t *p, util::string_view n, const spell_data_t *s ) : monk_buff_t( p, n, s )
  {
    // Currently this is saved as 100, but we need to utilize it as a percent so (100 / 100) = 1 * 0.01 = 1%
    set_default_value( ( s->effectN( 3 ).percent() / 100 ) );
    set_cooldown( p->talent.windwalker.fury_of_xuen->internal_cooldown() );
    set_trigger_spell( p->talent.windwalker.fury_of_xuen );
    set_cooldown( timespan_t::zero() );
  }
};

// ===============================================================================
// Fury of Xuen Haste Buff
// ===============================================================================
struct fury_of_xuen_t : public monk_buff_t
{
  static void fury_of_xuen_callback( buff_t *b, int, timespan_t )
  {
    auto *p = debug_cast<monk_t *>( b->player );
    if ( p->baseline.windwalker.empowered_tiger_lightning->ok() )
    {
      double empowered_tiger_lightning_multiplier =
          p->baseline.windwalker.empowered_tiger_lightning->effectN( 2 ).percent();

      for ( auto target : p->sim->target_non_sleeping_list )
      {
        if ( p->find_target_data( target ) )
        {
          auto td = p->get_target_data( target );
          if ( td->debuff.fury_of_xuen_empowered_tiger_lightning->up() )
          {
            double value = td->debuff.fury_of_xuen_empowered_tiger_lightning->check_value();
            td->debuff.fury_of_xuen_empowered_tiger_lightning->current_value = 0;
            if ( value > 0 )
            {
              p->active_actions.fury_of_xuen_empowered_tiger_lightning->base_dd_min =
                  value * empowered_tiger_lightning_multiplier;
              p->active_actions.fury_of_xuen_empowered_tiger_lightning->base_dd_max =
                  value * empowered_tiger_lightning_multiplier;
              p->active_actions.fury_of_xuen_empowered_tiger_lightning->execute_on_target( target );
            }
          }
        }
      }
    }
  }

  fury_of_xuen_t( monk_t *p, util::string_view n, const spell_data_t *s ) : monk_buff_t( p, n, s )
  {
    set_trigger_spell( p->talent.windwalker.fury_of_xuen );
    set_default_value_from_effect( 1 );
    set_cooldown( timespan_t::zero() );
    set_period( s->effectN( 3 ).period() );

    set_tick_callback( fury_of_xuen_callback );

    set_pct_buff_type( STAT_PCT_BUFF_CRIT );
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    set_pct_buff_type( STAT_PCT_BUFF_MASTERY );

    add_invalidate( CACHE_CRIT_CHANCE );
    add_invalidate( CACHE_ATTACK_CRIT_CHANCE );
    add_invalidate( CACHE_SPELL_CRIT_CHANCE );

    add_invalidate( CACHE_ATTACK_HASTE );
    add_invalidate( CACHE_HASTE );
    add_invalidate( CACHE_SPELL_HASTE );

    add_invalidate( CACHE_MASTERY );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( buff_t::trigger( stacks, value, chance, duration ) )
    {
      if ( p().talent.conduit_of_the_celestials.restore_balance->ok() )
        p().buff.rushing_jade_wind->trigger( remains() );

      return true;
    }

    return false;
  }
};

// ===============================================================================
// Niuzao Rank 2 Purifying Buff
// ===============================================================================
struct purifying_buff_t : public monk_buff_t
{
  purifying_buff_t( monk_t *player ) : monk_buff_t( player, "purifying_brew", spell_data_t::nil() )
  {
    set_trigger_spell( player->talent.brewmaster.purifying_brew );
    set_can_cancel( true );
    set_quiet( true );
    set_cooldown( timespan_t::zero() );
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
    set_default_value( 0.2 );

    set_refresh_behavior( buff_refresh_behavior::NONE );

    set_duration( 6_s );
    set_max_stack( 99 );
  }
};

// ===============================================================================
// Touch of Death Windwalker Buff
// ===============================================================================
// This buff is set up so that it provides the chi from
// Windwalker's Touch of Death Rank 2. In-game, applying Touch of Death will spawn
// three chi orbs that the player can pick up whenever they do not have max
// Chi. Given we want to provide the chi but apply it slowly if the player is at
// max chi, then we need to set up so that it triggers on it's own.

struct touch_of_death_ww_buff_t : public monk_buff_t
{
  touch_of_death_ww_buff_t( monk_t *p, util::string_view n, const spell_data_t *s ) : monk_buff_t( p, n, s )
  {
    set_can_cancel( false );
    set_quiet( true );
    set_reverse( true );
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( p->baseline.windwalker.touch_of_death_rank_3 );

    set_duration( timespan_t::from_minutes( 3 ) );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_zero( true );

    set_max_stack( (int)p->find_spell( 325215 )->effectN( 1 ).base_value() );
    set_reverse_stack_count( 1 );
  }

  void decrement( int stacks, double value ) override
  {
    if ( stacks > 0 && player->resources.current[ RESOURCE_CHI ] < player->resources.max[ RESOURCE_CHI ] )
    {
      buff_t::decrement( stacks, value );

      player->resource_gain( RESOURCE_CHI, 1, p().gain.touch_of_death_ww );
    }
  }
};

// ===============================================================================
// Windwalking Buff
// ===============================================================================
struct windwalking_driver_t : public monk_buff_t
{
  double movement_increase;
  windwalking_driver_t( monk_t *p, util::string_view n, const spell_data_t *s )
    : monk_buff_t( p, n, s ), movement_increase( 0 )
  {
    set_tick_callback( [ p, this ]( buff_t *, int /* total_ticks */, timespan_t /* tick_time */ ) {
      range::for_each( p->windwalking_aura->target_list(), [ this ]( player_t *target ) {
        target->buffs.windwalking_movement_aura->trigger( 1, ( movement_increase ), 1, timespan_t::from_seconds( 10 ) );
      } );
    } );
    set_trigger_spell( p->talent.monk.windwalking );
    set_cooldown( timespan_t::zero() );
    set_duration( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_behavior( buff_tick_behavior::CLIP );
    movement_increase = p->talent.monk.windwalking->effectN( 1 ).percent();
  }
};

// ===============================================================================
// Aspect of Harmony (Master of Harmony)
// ===============================================================================
aspect_of_harmony_t::aspect_of_harmony_t()
  : accumulator( nullptr ),
    spender( nullptr ),
    damage( nullptr ),
    heal( nullptr ),
    purified_spirit( nullptr ),
    path_of_resurgence( nullptr ),
    fallback( false )
{
}

void aspect_of_harmony_t::construct_buffs( monk_t *player )
{
  path_of_resurgence =
      make_buff_fallback( player->talent.master_of_harmony.path_of_resurgence->ok(), &*player, "path_of_resurgence",
                          player->talent.master_of_harmony.path_of_resurgence->effectN( 1 ).trigger() )
          ->apply_affecting_aura( player->talent.monk.chi_wave );

  if ( fallback || !player->talent.master_of_harmony.aspect_of_harmony->ok() )
  {
    fallback = true;
    buff_t::make_fallback( player, "aspect_of_harmony_accumulator", &*player );
    buff_t::make_fallback( player, "aspect_of_harmony_spender", &*player );
    return;
  }

  accumulator = new accumulator_t( player, this );
  spender     = new spender_t( player, this );
}

void aspect_of_harmony_t::construct_actions( monk_t *player )
{
  if ( fallback || !player->talent.master_of_harmony.aspect_of_harmony->ok() )
  {
    fallback = true;
    return;
  }

  damage = new spender_t::tick_t<monk_spell_t>( player, "aspect_of_harmony_damage",
                                                player->talent.master_of_harmony.aspect_of_harmony_damage );
  heal   = new spender_t::tick_t<monk_heal_t>( player, "aspect_of_harmony_heal",
                                               player->talent.master_of_harmony.aspect_of_harmony_heal );

  if ( player->specialization() == MONK_BREWMASTER )
    purified_spirit = new spender_t::purified_spirit_t<monk_spell_t>(
        player, player->talent.master_of_harmony.purified_spirit_damage, this );
  if ( player->specialization() == MONK_MISTWEAVER )
    purified_spirit = new spender_t::purified_spirit_t<monk_heal_t>(
        player, player->talent.master_of_harmony.purified_spirit_heal, this );
  damage->add_child( purified_spirit );
}

void aspect_of_harmony_t::trigger( action_state_t *state )
{
  if ( fallback || state->result_amount <= 0.0 )
    return;

  if ( !spender->check() )
    accumulator->trigger_with_state( state );
  if ( spender->check() )
    spender->trigger_with_state( state );
}

void aspect_of_harmony_t::trigger_flat( double amount )
{
  if ( fallback || spender->check() )
    return;

  accumulator->sim->print_debug( "Aspect of Harmony +A: {}, P: {}, T: {}", amount, accumulator->current_value,
                                 accumulator->current_value + amount );
  accumulator->current_value += amount;
}

void aspect_of_harmony_t::trigger_spend()
{
  if ( fallback )
    return;

  spender->trigger();
}

void aspect_of_harmony_t::trigger_path_of_resurgence()
{
  if ( fallback )
    return;

  path_of_resurgence->trigger();
}

bool aspect_of_harmony_t::heal_ticking()
{
  if ( heal )
    return heal->get_dot()->is_ticking();
  return false;
}

aspect_of_harmony_t::accumulator_t::accumulator_t( monk_t *player, aspect_of_harmony_t *aspect_of_harmony )
  : monk_buff_t( player, "aspect_of_harmony_accumulator",
                 player->talent.master_of_harmony.aspect_of_harmony_accumulator ),
    aspect_of_harmony( aspect_of_harmony )
{
  set_default_value( 0.0 );
}

void aspect_of_harmony_t::accumulator_t::trigger_with_state( action_state_t *state )
{
  size_t result_type_offset = 0;
  switch ( state->result_type )
  {
    case result_amount_type::DMG_DIRECT:
    case result_amount_type::DMG_OVER_TIME:
      result_type_offset = 1;
      break;
    case result_amount_type::HEAL_DIRECT:
    case result_amount_type::HEAL_OVER_TIME:
      result_type_offset = 3;
      break;
    default:
      assert( false && "result_type_offset is zero" );
      return;
  }

  size_t index_offset = p().specialization() == MONK_BREWMASTER ? 0 : 1;
  double multiplier =
      p().talent.master_of_harmony.aspect_of_harmony->effectN( result_type_offset + index_offset ).percent();

  if ( aspect_of_harmony->path_of_resurgence && aspect_of_harmony->path_of_resurgence->up() )
    multiplier *=
        1.0 + aspect_of_harmony->path_of_resurgence->data().effectN( result_type_offset + index_offset ).percent();

  const auto whitelist = { p().baseline.brewmaster.blackout_kick->id(),
                           p().talent.monk.rising_sun_kick->effectN( 1 ).trigger()->id(),
                           p().baseline.monk.tiger_palm->id() };

  if ( const auto &effect = p().talent.master_of_harmony.way_of_a_thousand_strikes->effectN( 1 );
       effect.ok() && std::find( whitelist.begin(), whitelist.end(), state->action->id ) != whitelist.end() )
    multiplier *= 1.0 + effect.percent();

  double amount = std::min( check_value() + state->result_amount * multiplier, p().max_health() );
  sim->print_debug( "Aspect of Harmony +A: {}, P: {}, T: {}", state->result_amount * multiplier, check_value(),
                    check_value() + state->result_amount * multiplier );
  monk_buff_t::trigger( -1, amount );
}

aspect_of_harmony_t::spender_t::spender_t( monk_t *player, aspect_of_harmony_t *aspect_of_harmony )
  : actions::monk_buff_t( player, "aspect_of_harmony_spender",
                          player->talent.master_of_harmony.aspect_of_harmony_spender ),
    aspect_of_harmony( aspect_of_harmony ),
    pool( 0.0 )
{
  set_default_value( 0.0 );

  if ( player->talent.master_of_harmony.purified_spirit->ok() )
    set_stack_change_callback( [ = ]( buff_t *, int, int new_ ) {
      if ( !new_ )
        aspect_of_harmony->purified_spirit->execute();
    } );
}

void aspect_of_harmony_t::spender_t::reset()
{
  monk_buff_t::reset();
  pool = 0.0;
  sim->print_debug( "Aspect of Harmony =P: 0" );
}

bool aspect_of_harmony_t::spender_t::trigger( int stacks, double, double chance, timespan_t duration )
{
  pool = aspect_of_harmony->accumulator->check_value();
  aspect_of_harmony->accumulator->expire();
  sim->print_debug( "Aspect of Harmony +P: {}", pool );
  return monk_buff_t::trigger( stacks, pool, chance, duration );
}

void aspect_of_harmony_t::spender_t::trigger_with_state( action_state_t *state )
{
  double multiplier = p().talent.master_of_harmony.aspect_of_harmony->effectN( 6 ).percent();
  double amount     = std::min( state->result_amount * multiplier, current_value );
  if ( amount >= current_value )
  {
    sim->print_debug( "Aspect of Harmony -P: {}, P: {}, T: {}", amount, current_value, current_value - amount );
    pool = current_value;
    expire();
    return;
  }
  current_value -= amount;

  const auto whitelist = { p().baseline.monk.expel_harm->id(), p().baseline.monk.vivify->id(),
                           p().baseline.monk.blackout_kick->id(), p().baseline.monk.tiger_palm->id() };

  auto in_hg_whitelist = [ whitelist, id = state->action->id, this ]() {
    return p().talent.master_of_harmony.harmonic_gambit->ok() &&
           std::find( whitelist.begin(), whitelist.end(), id ) != whitelist.end();
  };

  switch ( state->result_type )
  {
    case result_amount_type::DMG_DIRECT:
    case result_amount_type::DMG_OVER_TIME:
      if ( p().specialization() == MONK_BREWMASTER || in_hg_whitelist() )
      {
        sim->print_debug( "Aspect of Harmony -P: {}, P: {}, T: {}", amount, current_value + amount, current_value );
        residual_action::trigger( aspect_of_harmony->damage, state->target, amount );
      }
      break;
    case result_amount_type::HEAL_DIRECT:
    case result_amount_type::HEAL_OVER_TIME:
      if ( p().specialization() == MONK_MISTWEAVER || in_hg_whitelist() )
      {
        sim->print_debug( "Aspect of Harmony -P: {}, P: {}, T: {}", amount, current_value + amount, current_value );
        residual_action::trigger( aspect_of_harmony->heal, state->target, amount );
      }
      break;
    default:
      break;
  }
}

template <class base_action_t>
aspect_of_harmony_t::spender_t::purified_spirit_t<base_action_t>::purified_spirit_t(
    monk_t *player, const spell_data_t *spell_data, aspect_of_harmony_t *aspect_of_harmony )
  : base_action_t( player, "purified_spirit", spell_data ), aspect_of_harmony( aspect_of_harmony )
{
  base_action_t::aoe              = -1;
  base_action_t::split_aoe_damage = true;
  base_action_t::dot_behavior     = DOT_CLIP;
}

template <class base_action_t>
void aspect_of_harmony_t::spender_t::purified_spirit_t<base_action_t>::init()
{
  base_action_t::init();
  base_action_t::update_flags = base_action_t::snapshot_flags &= STATE_NO_MULTIPLIER;
}

template <class base_action_t>
void aspect_of_harmony_t::spender_t::purified_spirit_t<base_action_t>::execute()
{
  base_action_t::base_td = aspect_of_harmony->spender->pool / 4.0;
  base_action_t::sim->print_debug( "Purified Spirit consuming rest of pool. Pool: {} TA: {}",
                                   aspect_of_harmony->spender->pool, aspect_of_harmony->spender->pool / 4.0 );
  aspect_of_harmony->spender->pool = 0.0;
  if ( base_action_t::base_td > 0.0 )
    base_action_t::execute();
}

template <class base_action_t>
aspect_of_harmony_t::spender_t::tick_t<base_action_t>::tick_t( monk_t *player, std::string_view name,
                                                               const spell_data_t *spell_data )
  : residual_action::residual_periodic_action_t<base_action_t>( player, name, spell_data )
{
}

}  // namespace buffs

namespace items
{
// MONK MODULE INTERFACE ====================================================

void do_trinket_init( monk_t *player, specialization_e spec, const special_effect_t *&ptr,
                      const special_effect_t &effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player->find_spell( effect.spell_id )->ok() || player->specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

void init()
{
}
}  // namespace items

}  // end namespace monk

namespace monk
{
// ==========================================================================
// Monk Character Definition
// ==========================================================================

// Debuffs ==================================================================
monk_td_t::monk_td_t( player_t *target, monk_t *p ) : actor_target_data_t( target, p ), dot(), debuff(), monk( *p )
{
  // Windwalker
  debuff.acclamation = make_buff( *this, "acclamation", p->talent.windwalker.acclamation->effectN( 1 ).trigger() )
                           ->set_trigger_spell( p->talent.windwalker.acclamation )
                           ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                           ->set_default_value_from_effect( 1 );

  debuff.empowered_tiger_lightning = make_buff( *this, "empowered_tiger_lightning", spell_data_t::nil() )
                                         ->set_trigger_spell( p->baseline.windwalker.empowered_tiger_lightning )
                                         ->set_quiet( true )
                                         ->set_cooldown( timespan_t::zero() )
                                         ->set_refresh_behavior( buff_refresh_behavior::NONE )
                                         ->set_max_stack( 1 )
                                         ->set_default_value( 0 );
  debuff.fury_of_xuen_empowered_tiger_lightning =
      make_buff( *this, "empowered_tiger_lightning_fury_of_xuen", spell_data_t::nil() )
          ->set_trigger_spell( p->baseline.windwalker.empowered_tiger_lightning )
          ->set_quiet( true )
          ->set_cooldown( timespan_t::zero() )
          ->set_refresh_behavior( buff_refresh_behavior::NONE )
          ->set_max_stack( 1 )
          ->set_default_value( 0 );

  debuff.gale_force =
      make_buff( *this, "gale_force", p->find_spell( 451582 ) )->set_trigger_spell( p->talent.windwalker.gale_force );

  debuff.mark_of_the_crane = make_buff( *this, "mark_of_the_crane", p->passives.mark_of_the_crane )
                                 ->set_stack_change_callback( [ p ]( buff_t *, int, int new_ ) {
                                   if ( p->user_options.motc_override == 0 )
                                   {
                                     if ( new_ )
                                       p->buff.cyclone_strikes->trigger();
                                   }
                                 } )
                                 ->set_expire_callback( [ p ]( buff_t *, int, timespan_t remaining_duration ) {
                                   if ( remaining_duration == 0_s && p->user_options.motc_override == 0 )
                                     p->buff.cyclone_strikes->decrement();
                                 } )
                                 ->set_max_stack( 1 )
                                 ->set_trigger_spell( p->baseline.windwalker.mark_of_the_crane )
                                 ->set_default_value( p->passives.cyclone_strikes->effectN( 1 ).percent() )
                                 ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  debuff.touch_of_karma = make_buff( *this, "touch_of_karma_debuff", p->baseline.windwalker.touch_of_karma )
                              // set the percent of the max hp as the default value.
                              ->set_default_value_from_effect( 3 );

  // Brewmaster
  debuff.keg_smash = make_buff( *this, "keg_smash", p->talent.brewmaster.keg_smash )
                         ->set_cooldown( timespan_t::zero() )
                         ->set_default_value_from_effect( 3 );

  debuff.exploding_keg = make_buff( *this, "exploding_keg_debuff", p->talent.brewmaster.exploding_keg )
                             ->set_trigger_spell( p->talent.brewmaster.exploding_keg )
                             ->set_cooldown( timespan_t::zero() );

  // Shado-Pan

  debuff.high_impact = make_buff_fallback( p->talent.shado_pan.high_impact->ok(), *this, "high_impact",
                                           p->talent.shado_pan.high_impact_debuff )
                           ->set_trigger_spell( p->talent.shado_pan.high_impact )
                           ->set_quiet( true );

  debuff.veterans_eye = make_buff( *this, "veterans_eye_debuff", p->find_spell( 451071 ) )
                            ->set_trigger_spell( p->talent.shado_pan.veterans_eye )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                            ->set_quiet( true );

  // Covenant Abilities
  debuff.jadefire_stomp = make_buff( *this, "jadefire_stomp_debuff", p->find_spell( 388199 ) )
                              ->set_trigger_spell( p->shared.jadefire_stomp );

  debuff.weapons_of_order = make_buff( *this, "weapons_of_order_debuff", p->talent.brewmaster.weapons_of_order_debuff )
                                ->set_trigger_spell( p->talent.brewmaster.weapons_of_order )
                                ->set_default_value_from_effect( 1 );

  debuff.jadefire_brand = make_buff( *this, "jadefire_brand_damage", p->talent.windwalker.jadefire_brand_dmg )
                              ->set_trigger_spell( p->talent.windwalker.jadefire_harmony )
                              ->set_default_value_from_effect( 1 );

  debuff.storm_earth_and_fire = make_buff( *this, "storm_earth_and_fire_target", spell_data_t::nil() )
                                    ->set_trigger_spell( p->talent.windwalker.storm_earth_and_fire )
                                    ->set_cooldown( timespan_t::zero() );

  dot.breath_of_fire                   = target->get_dot( "breath_of_fire_dot", p );
  dot.crackling_jade_lightning_aoe     = target->get_dot( "crackling_jade_lightning_aoe", p );
  dot.crackling_jade_lightning_sef     = target->get_dot( "crackling_jade_lightning_sef", p );
  dot.crackling_jade_lightning_sef_aoe = target->get_dot( "crackling_jade_lightning_sef_aoe", p );
  dot.enveloping_mist                  = target->get_dot( "enveloping_mist", p );
  dot.renewing_mist                    = target->get_dot( "renewing_mist", p );
  dot.soothing_mist                    = target->get_dot( "soothing_mist", p );
  dot.touch_of_karma                   = target->get_dot( "touch_of_karma", p );
  dot.aspect_of_harmony                = target->get_dot( "aspect_of_harmony_damage", p );
}

monk_t::monk_t( sim_t *sim, util::string_view name, race_e r )
  : base_t( sim, MONK, name, r ),
    active_actions(),
    passive_actions(),
    squirm_timer( 0 ),
    spiritual_focus_count( 0 ),
    efficient_training_energy( 0 ),
    flurry_strikes_energy( 0 ),
    flurry_strikes_damage( 0 ),
    buff(),
    gain(),
    proc(),
    cooldown(),
    baseline(),
    talent(),
    tier(),
    pets( this ),
    user_options( options_t() ),
    shared(),
    passives()
{
  // actives
  windwalking_aura = nullptr;

  cooldown.anvil__stave           = get_cooldown( "anvil__stave" );
  cooldown.blackout_kick          = get_cooldown( "blackout_kick" );
  cooldown.breath_of_fire         = get_cooldown( "breath_of_fire" );
  cooldown.chi_torpedo            = get_cooldown( "chi_torpedo" );
  cooldown.drinking_horn_cover    = get_cooldown( "drinking_horn_cover" );
  cooldown.expel_harm             = get_cooldown( "expel_harm" );
  cooldown.jadefire_stomp         = get_cooldown( "jadefire_stomp" );
  cooldown.fists_of_fury          = get_cooldown( "fists_of_fury" );
  cooldown.flying_serpent_kick    = get_cooldown( "flying_serpent_kick" );
  cooldown.healing_elixir         = get_cooldown( "healing_elixir" );
  cooldown.invoke_niuzao          = get_cooldown( "invoke_niuzao_the_black_ox" );
  cooldown.invoke_xuen            = get_cooldown( "invoke_xuen_the_white_tiger" );
  cooldown.invoke_yulon           = get_cooldown( "invoke_yulon_the_jade_serpent" );
  cooldown.keg_smash              = get_cooldown( "keg_smash" );
  cooldown.rising_sun_kick        = get_cooldown( "rising_sun_kick" );
  cooldown.refreshing_jade_wind   = get_cooldown( "refreshing_jade_wind" );
  cooldown.roll                   = get_cooldown( "roll" );
  cooldown.storm_earth_and_fire   = get_cooldown( "storm_earth_and_fire" );
  cooldown.strike_of_the_windlord = get_cooldown( "strike_of_the_windlord" );
  cooldown.thunder_focus_tea      = get_cooldown( "thunder_focus_tea" );
  cooldown.touch_of_death         = get_cooldown( "touch_of_death" );
  cooldown.weapons_of_order       = get_cooldown( "weapons_of_order" );
  cooldown.whirling_dragon_punch  = get_cooldown( "whirling_dragon_punch" );

  resource_regeneration = regen_type::DYNAMIC;
  if ( specialization() != MONK_MISTWEAVER )
  {
    regen_caches[ CACHE_HASTE ]        = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }
  user_options.initial_chi =
      talent.windwalker.combat_wisdom.ok() ? (int)talent.windwalker.combat_wisdom->effectN( 1 ).base_value() : 0;
  user_options.chi_burst_healing_targets = 8;
  user_options.motc_override             = 0;
  user_options.squirm_frequency          = 15;
}

void monk_t::parse_player_effects()
{
  /*
   * Permanent actor-specific effects go here.
   * Make sure to use a specific `find_spell` method (i.e. `find_specialization_spell`)
   * for all of these spells or they will be applied to actors of the incorrect spec.
   */

  // class and spec shared auras
  parse_effects( baseline.monk.aura );
  parse_effects( baseline.monk.critical_strikes );
  parse_effects( baseline.monk.two_hand_adjustment );
  /*
   * 2024-5-14: 2-Hand adjustment was demonstrated to not work for BrM.
   * Requires confirmation from WW to verify this is correct for both specs.
   */
  if ( matching_gear )
    parse_effects( baseline.monk.leather_specialization );

  // brewmaster player auras
  parse_effects( baseline.brewmaster.aura );
  parse_effects( baseline.brewmaster.aura_2 );
  parse_effects( baseline.brewmaster.brewmasters_balance );
  parse_effects( baseline.brewmaster.celestial_fortune );

  // mistweaver player auras
  parse_effects( baseline.mistweaver.aura );
  parse_effects( baseline.mistweaver.aura_2 );
  parse_effects( baseline.mistweaver.aura_3 );

  // windwalker player auras
  parse_effects( baseline.windwalker.aura );
  parse_effects( baseline.windwalker.aura_2 );
  parse_effects( buff.hit_combo, effect_mask_t( true ).disable( 4 ) );

  // class talent auras
  parse_effects( talent.monk.grace_of_the_crane );
  parse_effects( talent.monk.calming_presence );
  parse_effects( talent.monk.ferocity_of_xuen );
  parse_effects( talent.monk.chi_proficiency );
  parse_effects( talent.monk.martial_instincts );

  // brewmaster talent auras
  parse_effects( buff.pretense_of_instability );
  parse_effects( buff.weapons_of_order, effect_mask_t( false ).enable( 1 ) );

  // mistweaver talent auras
  // windwalker talent auras
  parse_target_effects( td_fn( &monk_td_t::debuff_t::acclamation ),
                        talent.windwalker.acclamation->effectN( 1 ).trigger() );
  parse_effects( buff.invokers_delight, baseline.windwalker.aura );
  parse_effects( buff.memory_of_the_monastery );
  parse_effects( buff.momentum_boost_speed );
  parse_effects( buff.dual_threat );
  parse_effects( buff.ferociousness, USE_CURRENT );

  if ( talent.windwalker.jadefire_harmony->ok() )
    parse_target_effects( td_fn( &monk_td_t::debuff_t::jadefire_brand ), talent.windwalker.jadefire_brand_dmg, 0b001U );

  // Shadopan
  if ( auto &b = buff.wisdom_of_the_wall_dodge; !b->is_fallback )
  {
    auto add_and_invalidate = [ & ]( std::vector<player_effect_t> &effect_vector, int effect_index,
                                     cache_e invalidate ) {
      add_parse_entry( effect_vector )
          .set_type( USE_CURRENT )
          .set_buff( b )
          .set_eff( &b->data().effectN( effect_index ) );
      b->add_invalidate( invalidate );
    };
    add_and_invalidate( dodge_effects, 1, CACHE_DODGE );
    add_and_invalidate( crit_chance_effects, 2, CACHE_CRIT_CHANCE );
  }
  parse_effects( buff.wisdom_of_the_wall_mastery );
  parse_effects( buff.against_all_odds );
  parse_effects( buff.veterans_eye );

  // Conduit of the Celestials
  parse_effects( buff.flight_of_the_red_crane );
  parse_effects( buff.inner_compass_crane_stance );
  parse_effects( buff.inner_compass_ox_stance );
  parse_effects( buff.inner_compass_serpent_stance );
  parse_effects( buff.inner_compass_tiger_stance );

  // TWW S1 Set Effects
  parse_effects( buff.shuffle, sets->set( MONK_BREWMASTER, TWW1, B2 ) );

  // TWW S2 Set Effects

  // TWW S3 Set Effects

  // TWW S4 Set Effects
}

// monk_t::create_action ====================================================

action_t *monk_t::create_action( util::string_view name, util::string_view options_str )
{
  using namespace actions;
  // General
  if ( name == "snapshot_stats" )
    return new monk_snapshot_stats_t( this, options_str );
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "crackling_jade_lightning" )
    return new crackling_jade_lightning_t( this, options_str );
  if ( name == "tiger_palm" )
    return new tiger_palm_t( this, options_str );
  if ( name == "blackout_kick" )
    return new blackout_kick_t( this, options_str );
  if ( name == "expel_harm" )
    return new expel_harm_t( this, options_str );
  if ( name == "leg_sweep" )
    return new leg_sweep_t( this, options_str );
  if ( name == "paralysis" )
    return new paralysis_t( this, options_str );
  if ( name == "rising_sun_kick" )
    return new rising_sun_kick_t( this, options_str );
  if ( name == "roll" )
    return new roll_t( this, options_str );
  if ( name == "spear_hand_strike" )
    return new spear_hand_strike_t( this, options_str );
  if ( name == "spinning_crane_kick" )
    return new spinning_crane_kick_t( this, options_str );
  if ( name == "vivify" )
    return new vivify_t( this, options_str );

  // Brewmaster
  if ( name == "breath_of_fire" )
    return new breath_of_fire_t( this, options_str );
  if ( name == "celestial_brew" )
    return new celestial_brew_t( this, options_str );
  if ( name == "exploding_keg" )
    return new exploding_keg_t( this, options_str );
  if ( name == "fortifying_brew" )
    return new fortifying_brew_t( this, options_str );
  if ( name == "invoke_niuzao" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "invoke_niuzao_the_black_ox" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "keg_smash" )
    return new press_the_advantage_t<keg_smash_t>( this, options_str );
  if ( name == "purifying_brew" )
    return new purifying_brew_t( this, options_str );
  if ( name == "provoke" )
    return new provoke_t( this, options_str );

  // Mistweaver
  if ( name == "enveloping_mist" )
    return new enveloping_mist_t( this, options_str );
  if ( name == "invoke_chiji" )
    return new chiji_spell_t( this, options_str );
  if ( name == "invoke_chiji_the_red_crane" )
    return new chiji_spell_t( this, options_str );
  if ( name == "invoke_yulon" )
    return new yulon_spell_t( this, options_str );
  if ( name == "invoke_yulon_the_jade_serpent" )
    return new yulon_spell_t( this, options_str );
  if ( name == "life_cocoon" )
    return new life_cocoon_t( this, options_str );
  if ( name == "mana_tea" )
    return new mana_tea_t( this, options_str );
  // if ( name == "renewing_mist" )
  //   return new renewing_mist_t( this, options_str );
  if ( name == "revival" )
    return new revival_t( this, options_str );
  if ( name == "thunder_focus_tea" )
    return new thunder_focus_tea_t( this, options_str );
  // if ( name == "zen_pulse" )
  //   return new zen_pulse_t( this, options_str );

  // Windwalker
  if ( name == "fists_of_fury" )
    return new fists_of_fury_t( this, options_str );
  if ( name == "flying_serpent_kick" )
    return new flying_serpent_kick_t( this, options_str );
  if ( name == "touch_of_karma" )
    return new touch_of_karma_t( this, options_str );
  if ( name == "touch_of_death" )
    return new touch_of_death_t( this, options_str );
  if ( name == "storm_earth_and_fire" )
    return new storm_earth_and_fire_t( this, options_str );
  if ( name == "storm_earth_and_fire_fixate" )
    return new storm_earth_and_fire_fixate_t( this, options_str );

  // Talents
  if ( name == "chi_burst" )
    return new chi_burst_t( this, options_str );
  if ( name == "chi_torpedo" )
    return new chi_torpedo_t( this, options_str );
  if ( name == "black_ox_brew" )
    return new black_ox_brew_t( this, options_str );
  if ( name == "dampen_harm" )
    return new dampen_harm_t( this, options_str );
  if ( name == "diffuse_magic" )
    return new diffuse_magic_t( this, options_str );
  if ( name == "strike_of_the_windlord" )
    return new strike_of_the_windlord_t( this, options_str );
  if ( name == "invoke_xuen" )
    return new xuen_spell_t( this, options_str );
  if ( name == "invoke_xuen_the_white_tiger" )
    return new xuen_spell_t( this, options_str );
  if ( name == "refreshing_jade_wind" )
    return new refreshing_jade_wind_t( this, options_str );
  if ( name == "rushing_jade_wind" )
    return new rushing_jade_wind_t( this, options_str );
  if ( name == "whirling_dragon_punch" )
    return new whirling_dragon_punch_t( this, options_str );

  // Covenant Abilities
  if ( name == "jadefire_stomp" )
    return new jadefire_stomp_t( this, options_str );
  if ( name == "weapons_of_order" )
    return new weapons_of_order_t( this, options_str );

  // Hero Talents
  if ( name == "celestial_conduit" )
    return new celestial_conduit_t( this, options_str );
  if ( name == "unity_within" )
    return new unity_within_t( this, options_str );

  return base_t::create_action( name, options_str );
}

void monk_t::trigger_celestial_fortune( action_state_t *s )
{
  if ( !baseline.brewmaster.celestial_fortune->ok() || s->action == active_actions.celestial_fortune ||
       s->result_raw == 0.0 )
  {
    return;
  }
  switch ( s->action->id )
  {
    case 143924:  // Leech
    case 259760:  // Entropic Embrace (Void Elf)
      return;
  }

  // flush out percent heals
  if ( s->action->type == ACTION_HEAL )
  {
    auto *heal_cast = debug_cast<heal_t *>( s->action );
    if ( ( s->result_type == result_amount_type::HEAL_DIRECT && heal_cast->base_pct_heal > 0 ) ||
         ( s->result_type == result_amount_type::HEAL_OVER_TIME && heal_cast->tick_pct_heal > 0 ) )
      return;
  }

  // Attempt to proc the heal
  if ( active_actions.celestial_fortune && rng().roll( composite_melee_crit_chance() ) )
  {
    sim->print_debug( "triggering celestial fortune from (id: {}, name: {}) with (amount: {}) damage base",
                      s->action->id, s->action->name_str, s->result_amount );
    active_actions.celestial_fortune->base_dd_max = active_actions.celestial_fortune->base_dd_min = s->result_amount;
    active_actions.celestial_fortune->schedule_execute();
  }
}

void monk_t::trigger_mark_of_the_crane( action_state_t *s )
{
  if ( !baseline.windwalker.mark_of_the_crane->ok() )
    return;

  if ( !action_t::result_is_hit( s->result ) )
    return;

  if ( get_target_data( s->target )->debuff.mark_of_the_crane->up() || !buff.cyclone_strikes->at_max_stacks() )
    get_target_data( s->target )->debuff.mark_of_the_crane->trigger();
}

player_t *monk_t::next_mark_of_the_crane_target( action_state_t *state )
{
  std::vector<player_t *> targets = state->action->target_list();
  if ( targets.empty() )
  {
    return nullptr;
  }
  if ( targets.size() > 1 )
  {
    // Have the SEF converge onto the the cleave target if there are only 2 targets
    if ( targets.size() == 2 )
      return targets[ 1 ];
    // SEF do not change targets if they are unfixated and there's only 3 targets.
    if ( targets.size() == 3 )
      return state->target;

    // First of all find targets that do not have the cyclone strike debuff applied and send the SEF to those targets
    for ( player_t *target : targets )
    {
      if ( !get_target_data( target )->debuff.mark_of_the_crane->check() &&
           !get_target_data( target )->debuff.storm_earth_and_fire->check() )
      {
        // remove the current target as having an SEF on it
        get_target_data( state->target )->debuff.storm_earth_and_fire->expire();
        // make the new target show that a SEF is on the target
        get_target_data( target )->debuff.storm_earth_and_fire->trigger();
        return target;
      }
    }

    // If all targets have the debuff, find the lowest duration of cyclone strike debuff as well as not have a SEF
    // debuff (indicating that an SEF is not already on the target and send the SEF to that new target.
    player_t *lowest_duration = targets[ 0 ];

    // They should never attack the player target
    for ( player_t *target : targets )
    {
      if ( !get_target_data( target )->debuff.storm_earth_and_fire->check() )
      {
        if ( get_target_data( target )->debuff.mark_of_the_crane->remains() <
             get_target_data( lowest_duration )->debuff.mark_of_the_crane->remains() )
          lowest_duration = target;
      }
    }
    // remove the current target as having an SEF on it
    get_target_data( state->target )->debuff.storm_earth_and_fire->expire();
    // make the new target show that a SEF is on the target
    get_target_data( lowest_duration )->debuff.storm_earth_and_fire->trigger();

    return lowest_duration;
  }
  // otherwise, target the same as the player
  return targets.front();
}

// Currently at maximum stacks for target count
bool monk_t::mark_of_the_crane_max()
{
  if ( !baseline.windwalker.mark_of_the_crane->ok() )
    return true;

  if ( user_options.motc_override > 0 )
    return true;

  int count   = buff.cyclone_strikes->current_stack;
  int targets = as<int>( sim->target_non_sleeping_list.data().size() );

  if ( count == 0 || ( ( targets - count ) > 0 && !buff.cyclone_strikes->at_max_stacks() ) )
    return false;

  return true;
}

// monk_t::activate =========================================================

void monk_t::activate()
{
  base_t::activate();

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    sim->target_non_sleeping_list.register_callback( sef_despawn_cb_t( this ) );
  }
}

void monk_t::collect_resource_timeline_information()
{
  base_t::collect_resource_timeline_information();

  // sim->print_debug(
  //     "{} current pool statistics pool_size={} pool_size_percent={} tick_size={} tick_size_percent={} cap={}",
  //     name(), stagger->pool_size(), stagger->pool_size_percent(), stagger->tick_size(), stagger->tick_size_percent(),
  //     resources.max[ RESOURCE_HEALTH ] );
  // stagger->sample_data->pool_size.add( sim->current_time(), stagger->pool_size() );
  // stagger->sample_data->pool_size_percent.add( sim->current_time(), stagger->pool_size_percent() );
  // stagger->sample_data->effectiveness.add( sim->current_time(), stagger->percent( target->level() ) );
}

bool monk_t::validate_fight_style( fight_style_e style ) const
{
  if ( specialization() == MONK_BREWMASTER )
  {
    switch ( style )
    {
      case FIGHT_STYLE_DUNGEON_ROUTE:
      case FIGHT_STYLE_DUNGEON_SLICE:
        return false;
      default:
        return true;
    }
  }
  if ( specialization() == MONK_WINDWALKER )
  {
    switch ( style )
    {
      case FIGHT_STYLE_PATCHWERK:
      case FIGHT_STYLE_CASTING_PATCHWERK:
        return true;
      case FIGHT_STYLE_DUNGEON_ROUTE:
      case FIGHT_STYLE_DUNGEON_SLICE:
      {
        sim->error( "Action Priority Lists (APL) for Dungeon Route and Dungeon Slice are not final." );
        return true;
      }
      default:
        return false;
    }
  }
  return true;
}

// monk_t::init_spells ======================================================
void monk_t::init_spells()
{
  base_t::init_spells();

  specialization_e current_spec = specialization();

  // Talents spells =====================================
  auto _CT = [ this, &current_spec ]( std::string_view name ) {
    return find_talent_spell( talent_tree::CLASS, name, current_spec );
  };
  auto _ST = [ this, &current_spec ]( std::string_view name ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, name, current_spec );
  };
  auto _HT = [ this ]( std::string_view name ) { return find_talent_spell( talent_tree::HERO, name ); };

  // auto _CTID = [ this, &current_spec ]( int id ) { return find_talent_spell( talent_tree::CLASS, id, current_spec );
  // };
  auto _STID = [ this, &current_spec ]( int id ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, id, current_spec );
  };
  // auto _HTID = [ this ]( int id ) { return find_talent_spell( talent_tree::HERO, id ); };

  // =================================================================================================

  /*
   * Always use the most specific possible `find_spell` method that you can use.
   * If you don't, you will have auras getting applied to incorrect specs, actions
   * usable for incorrect specs, etc.
   *
   * Place all relevant spells near the parent spell or talent
   * i.e. spells and talents relevant to Fortifying Brew go near the Fortifying Brew
   * talent.
   *
   * Default spells for the class and specs get placed in `baseline`.
   * If they are not owned by a specific spec, they get placed in `monk.`
   * If they are owned by a specific spec, they get placed in `spec_name`.
   *
   * Talent spells for the class and specs get placed in `talents`.
   * If they are class talents, they get placed in `monk`.
   * If they are spec or hero talents, they get placed in `spec/ht_name`.
   */

  // monk_t::baseline::monk
  {
    baseline.monk.aura                     = find_spell( 137022 );  // TODO: Blacklist 130610, use name instead
    baseline.monk.critical_strikes         = find_specialization_spell( "Critical Strikes" );
    baseline.monk.two_hand_adjustment      = find_specialization_spell( "Windwalker Monk Two-Hand Adjustment" );
    baseline.monk.leather_specialization   = find_specialization_spell( "Leather Specialization" );
    baseline.monk.expel_harm               = find_spell( 322101 );
    baseline.monk.expel_harm_damage        = find_spell( 115129 );
    baseline.monk.blackout_kick            = find_class_spell( "Blackout Kick" );
    baseline.monk.crackling_jade_lightning = find_class_spell( "Crackling Jade Lightning" );
    baseline.monk.leg_sweep                = find_class_spell( "Leg Sweep" );
    baseline.monk.mystic_touch             = find_spell( 113746 );
    baseline.monk.provoke                  = find_class_spell( "Provoke" );
    baseline.monk.roll                     = find_class_spell( "Roll" );
    baseline.monk.spinning_crane_kick      = find_spell( 101546 );
    baseline.monk.tiger_palm               = find_class_spell( "Tiger Palm" );
    baseline.monk.touch_of_death           = find_spell( 322109 );
    baseline.monk.vivify                   = find_class_spell( "Vivify" );
  }

  // monk_t::baseline::brewmaster
  {
    current_spec                                   = MONK_BREWMASTER;
    baseline.brewmaster.mastery                    = find_mastery_spell( MONK_BREWMASTER );
    baseline.brewmaster.aura                       = find_specialization_spell( "Brewmaster Monk" );
    baseline.brewmaster.aura_2                     = find_specialization_spell( 462087 );
    baseline.brewmaster.brewmasters_balance        = find_specialization_spell( "Brewmaster's Balance" );
    baseline.brewmaster.celestial_fortune          = find_specialization_spell( "Celestial Fortune" );
    baseline.brewmaster.celestial_fortune_heal     = find_spell( 216521 );  // TODO: Can you be more specific?
    baseline.brewmaster.expel_harm_rank_2          = find_rank_spell( "Expel Harm", "Rank 2", current_spec );
    baseline.brewmaster.blackout_kick              = find_spell( 205523 );
    baseline.brewmaster.stagger                    = find_specialization_spell( "Stagger" );
    baseline.brewmaster.stagger_self_damage        = find_spell( 124255 );
    baseline.brewmaster.light_stagger              = find_spell( 124275 );
    baseline.brewmaster.moderate_stagger           = find_spell( 124273 );
    baseline.brewmaster.heavy_stagger              = find_spell( 124272 );
    baseline.brewmaster.spinning_crane_kick        = find_specialization_spell( "Spinning Crane Kick" );
    baseline.brewmaster.spinning_crane_kick_rank_2 = find_rank_spell( "Spinning Crane Kick", "Rank 2", current_spec );
    baseline.brewmaster.touch_of_death_rank_3      = find_rank_spell( "Touch of Death", "Rank 3", current_spec );
  }

  // monk_t::baseline::mistweaver
  {
    current_spec                          = MONK_MISTWEAVER;
    baseline.mistweaver.mastery           = find_mastery_spell( MONK_MISTWEAVER );
    baseline.mistweaver.aura              = find_specialization_spell( "Mistweaver Monk" );
    baseline.mistweaver.aura_2            = find_specialization_spell( 428200 );
    baseline.mistweaver.aura_3            = find_specialization_spell( 462090 );
    baseline.mistweaver.expel_harm_rank_2 = find_rank_spell( "Expel Harm", "Rank 2", MONK_MISTWEAVER );
  }

  // monk_t::baseline::windwalker
  {
    current_spec                                  = MONK_WINDWALKER;
    baseline.windwalker.mastery                   = find_mastery_spell( MONK_WINDWALKER );
    baseline.windwalker.aura                      = find_specialization_spell( "Windwalker Monk" );
    baseline.windwalker.aura_2                    = find_specialization_spell( 462091 );
    baseline.windwalker.blackout_kick_rank_2      = find_rank_spell( "Blackout Kick", "Rank 2", MONK_WINDWALKER );
    baseline.windwalker.blackout_kick_rank_3      = find_rank_spell( "Blackout Kick", "Rank 3", MONK_WINDWALKER );
    baseline.windwalker.combo_breaker             = find_specialization_spell( "Combo Breaker" );
    baseline.windwalker.combat_conditioning       = find_specialization_spell( "Combat Conditioning" );
    baseline.windwalker.empowered_tiger_lightning = find_specialization_spell( "Empowered Tiger Lightning" );
    baseline.windwalker.flying_serpent_kick       = find_specialization_spell( "Flying Serpent Kick" );
    baseline.windwalker.mark_of_the_crane         = find_specialization_spell( "Mark of the Crane" );
    baseline.windwalker.touch_of_death_rank_3     = find_spell( 344361 );
    baseline.windwalker.touch_of_karma            = find_specialization_spell( "Touch of Karma" );
  }

  // monk_t::talent::monk
  {
    current_spec = specialization();
    // Row 1
    talent.monk.rising_sun_kick = _CT( "Rising Sun Kick" );
    talent.monk.soothing_mist   = _CT( "Soothing Mist" );
    talent.monk.paralysis       = _CT( "Paralysis" );
    // Row 2
    talent.monk.elusive_mists     = _CT( "Elusive Mists" );
    talent.monk.tigers_lust       = _CT( "Tiger's Lust" );
    talent.monk.crashing_momentum = _CT( "Crashing Momentum" );
    talent.monk.disable           = _CT( "Disable" );
    talent.monk.fast_feet         = _CT( "Fast Feet" );
    // Row 3
    talent.monk.grace_of_the_crane = _CT( "Grace of the Crane" );
    talent.monk.bounding_agility   = _CT( "Bounding Agility" );
    talent.monk.calming_presence   = _CT( "Calming Presence" );
    talent.monk.winds_reach        = _CT( "Wind's Reach" );
    talent.monk.detox              = _CT( "Detox" );           // Brewmaster and Windwalker
    talent.monk.improved_detox     = _CT( "Improved Detox" );  // Mistweaver only
    // Row 4
    talent.monk.vivacious_vivification = _CT( "Vivacious Vivification" );
    talent.monk.jade_walk              = _CT( "Jade Walk" );
    talent.monk.pressure_points        = _CT( "Pressure Points" );
    talent.monk.spear_hand_strike      = _CT( "Spear Hand Strike" );
    talent.monk.ancient_arts           = _CT( "Ancient Arts" );
    // Row 5
    talent.monk.chi_wave             = _CT( "Chi Wave" );
    talent.monk.chi_wave_buff        = find_spell( 450380 );
    talent.monk.chi_wave_driver      = find_spell( 115098 );
    talent.monk.chi_wave_damage      = find_spell( 132467 );
    talent.monk.chi_wave_heal        = find_spell( 132463 );
    talent.monk.chi_burst            = _CT( "Chi Burst" );
    talent.monk.chi_burst_buff       = find_spell( 460490 );
    talent.monk.chi_burst_projectile = find_spell( 461404 );
    talent.monk.chi_burst_damage     = find_spell( 148135 );
    talent.monk.chi_burst_heal       = find_spell( 130654 );
    talent.monk.transcendence        = _CT( "Transcendence" );
    talent.monk.energy_transfer      = _CT( "Energy Transfer" );
    talent.monk.celerity             = _CT( "Celerity" );
    talent.monk.chi_torpedo          = _CT( "Chi Torpedo" );
    // Row 6
    talent.monk.quick_footed            = _CT( "Quick Footed" );
    talent.monk.hasty_provocation       = _CT( "Hasty Provocation" );
    talent.monk.ferocity_of_xuen        = _CT( "Ferocity of Xuen" );
    talent.monk.ring_of_peace           = _CT( "Ring of Peace" );
    talent.monk.song_of_chi_ji          = _CT( "Song of Chi-Ji" );
    talent.monk.spirits_essence         = _CT( "Spirit's Essence" );
    talent.monk.tiger_tail_sweep        = _CT( "Tiger Tail Sweep" );
    talent.monk.improved_touch_of_death = _CT( "Improved Touch of Death" );
    // Row 7
    talent.monk.vigorous_expulsion   = _CT( "Vigorous Expulsion" );
    talent.monk.yulons_grace         = _CT( "Yu'lon's Grace" );
    talent.monk.diffuse_magic        = _CT( "Diffuse Magic" );
    talent.monk.peace_and_prosperity = _CT( "Peace and Prosperity" );
    talent.monk.fortifying_brew      = _CT( "Fortifying Brew" );
    talent.monk.fortifying_brew_buff = find_spell( 120954 );
    talent.monk.dance_of_the_wind    = _CT( "Dance of the Wind" );
    talent.monk.dampen_harm          = _CT( "Dampen Harm" );  // Brewmaster only
    // Row 8
    talent.monk.save_them_all              = _CT( "Save Them All" );
    talent.monk.swift_art                  = _CT( "Swift Art" );
    talent.monk.strength_of_spirit         = _CT( "Strength of Spirit" );
    talent.monk.profound_rebuttal          = _CT( "Profound Rebuttal" );
    talent.monk.summon_black_ox_statue     = _CT( "Summon Black Ox Statue" );      // Brewmaster only
    talent.monk.summon_jade_serpent_statue = _CT( "Summon Jade Serpent Statue" );  // Mistweaver only
    talent.monk.summon_white_tiger_statue  = _CT( "Summon White Tiger Statue" );   // Windwalker only
    talent.monk.claw_of_the_white_tiger    = find_spell( 389541 );
    talent.monk.ironshell_brew             = _CT( "Ironshell Brew" );
    talent.monk.celestial_determination    = _CT( "Celestial Determination" );
    // Row 9
    talent.monk.chi_proficiency   = _CT( "Chi Proficiency" );
    talent.monk.healing_winds     = _CT( "Healing Winds" );
    talent.monk.windwalking       = _CT( "Windwalking" );
    talent.monk.bounce_back       = _CT( "Bounce Back" );
    talent.monk.martial_instincts = _CT( "Martial Instincts" );
    // Row 10
    talent.monk.lighter_than_air    = _CT( "Lighter Than Air" );
    talent.monk.flow_of_chi         = _CT( "Flow of Chi" );
    talent.monk.escape_from_reality = _CT( "Escape from Reality" );
    talent.monk.transcendence       = _CT( "Transcendence: Linked Spirits" );
    talent.monk.fatal_touch         = _CT( "Fatal Touch" );
    talent.monk.rushing_reflexes    = _CT( "Rushing Reflexes" );
    talent.monk.clash               = _CT( "Clash" );
  }

  // monk_t::talent::brewmaster
  {
    current_spec                                          = MONK_BREWMASTER;
    talent.brewmaster.keg_smash                           = _ST( "Keg Smash" );
    talent.brewmaster.purifying_brew                      = _ST( "Purifying Brew" );
    talent.brewmaster.shuffle                             = _ST( "Shuffle" );
    talent.brewmaster.shuffle_buff                        = find_spell( 215479 );
    talent.brewmaster.staggering_strikes                  = _ST( "Staggering Strikes" );
    talent.brewmaster.gift_of_the_ox                      = _ST( "Gift of the Ox" );
    talent.brewmaster.spirit_of_the_ox                    = _ST( "Spirit of the Ox" );
    talent.brewmaster.gift_of_the_ox_buff                 = find_spell( 124506 );
    talent.brewmaster.gift_of_the_ox_heal_trigger         = find_spell( 124507 );
    talent.brewmaster.gift_of_the_ox_heal_expire          = find_spell( 178173 );
    talent.brewmaster.quick_sip                           = _ST( "Quick Sip" );
    talent.brewmaster.hit_scheme                          = _ST( "Hit Scheme" );
    talent.brewmaster.elixir_of_determination             = _ST( "Elixir of Determination" );
    talent.brewmaster.special_delivery                    = _ST( "Special Delivery" );
    talent.brewmaster.special_delivery_missile            = find_spell( 196732 );
    talent.brewmaster.rushing_jade_wind                   = _ST( "Rushing Jade Wind" );
    talent.brewmaster.celestial_flames                    = _ST( "Celestial Flames" );
    talent.brewmaster.celestial_brew                      = _ST( "Celestial Brew" );
    talent.brewmaster.purified_chi                        = find_spell( 325092 );
    talent.brewmaster.autumn_blessing                     = _ST( "Autumn Blessing" );
    talent.brewmaster.one_with_the_wind                   = _ST( "One With the Wind" );
    talent.brewmaster.zen_meditation                      = _ST( "Zen Meditation" );
    talent.brewmaster.strike_at_dawn                      = _ST( "Strike at Dawn" );
    talent.brewmaster.breath_of_fire                      = _ST( "Breath of Fire" );
    talent.brewmaster.breath_of_fire_dot                  = find_spell( 123725 );
    talent.brewmaster.gai_plins_imperial_brew             = _ST( "Gai Plin's Imperial Brew" );
    talent.brewmaster.gai_plins_imperial_brew_heal        = find_spell( 383701 );
    talent.brewmaster.invoke_niuzao_the_black_ox          = _ST( "Invoke Niuzao, the Black Ox" );
    talent.brewmaster.invoke_niuzao_the_black_ox_stomp    = find_spell( 227291 );
    talent.brewmaster.tranquil_spirit                     = _ST( "Tranquil Spirit" );
    talent.brewmaster.shadowboxing_treads                 = _ST( "Shadowboxing Treads" );
    talent.brewmaster.fluidity_of_motion                  = _ST( "Fluidity of Motion" );
    talent.brewmaster.scalding_brew                       = _ST( "Scalding Brew" );
    talent.brewmaster.salsalabims_strength                = _ST( "Sal'salabim's Strength" );
    talent.brewmaster.fortifying_brew_determination       = _ST( "Fortifying Brew: Determination" );
    talent.brewmaster.bob_and_weave                       = _ST( "Bob and Weave" );
    talent.brewmaster.black_ox_brew                       = _ST( "Black Ox Brew" );
    talent.brewmaster.walk_with_the_ox                    = _ST( "Walk With the Ox" );
    talent.brewmaster.light_brewing                       = _ST( "Light Brewing" );
    talent.brewmaster.training_of_niuzao                  = _ST( "Training of Niuzao" );
    talent.brewmaster.pretense_of_instability             = _ST( "Pretense of Instability" );
    talent.brewmaster.counterstrike                       = _ST( "Counterstrike" );
    talent.brewmaster.dragonfire_brew                     = _ST( "Dragonfire Brew" );
    talent.brewmaster.dragonfire_brew_hit                 = find_spell( 387621 );
    talent.brewmaster.charred_passions                    = _ST( "Charred Passions" );
    talent.brewmaster.high_tolerance                      = _ST( "High Tolerance" );
    talent.brewmaster.exploding_keg                       = _ST( "Exploding Keg" );
    talent.brewmaster.improved_invoke_niuzao_the_black_ox = _ST( "Improved Invoke Niuzao, the Black Ox" );
    talent.brewmaster.elusive_footwork                    = _ST( "Elusive Footwork" );
    talent.brewmaster.anvil__stave                        = _ST( "Anvil & Stave" );
    talent.brewmaster.face_palm                           = _ST( "Face Palm" );
    talent.brewmaster.ox_stance                           = _ST( "Ox Stance" );
    talent.brewmaster.ox_stance_buff                      = find_spell( 455071 );
    talent.brewmaster.stormstouts_last_keg                = _ST( "Stormstout's Last Keg" );
    talent.brewmaster.blackout_combo                      = _ST( "Blackout Combo" );
    talent.brewmaster.press_the_advantage                 = _ST( "Press the Advantage" );
    talent.brewmaster.weapons_of_order                    = _ST( "Weapons of Order" );
    talent.brewmaster.weapons_of_order_debuff             = find_spell( 387179 );
    talent.brewmaster.black_ox_adept                      = _ST( "Black Ox Adept" );
    talent.brewmaster.heightened_guard                    = _ST( "Heightened Guard" );
    talent.brewmaster.call_to_arms                        = _ST( "Call to Arms" );
    talent.brewmaster.call_to_arms_buff                   = find_spell( 395267 );
    talent.brewmaster.chi_surge                           = _ST( "Chi Surge" );
  }

  // monk_t::talent::mistweaver
  {
    current_spec = MONK_MISTWEAVER;
    // Row 1
    talent.mistweaver.enveloping_mist = _ST( "Enveloping Mist" );
    // Row 2
    talent.mistweaver.thunder_focus_tea = _ST( "Thunder Focus Tea" );
    talent.mistweaver.renewing_mist     = _ST( "Renewing Mist" );
    // Row 3
    talent.mistweaver.life_cocoon    = _ST( "Life Cocoon" );
    talent.mistweaver.mana_tea       = _ST( "Mana Tea" );
    talent.mistweaver.healing_elixir = _ST( "Healing Elixir" );
    // Row 4
    talent.mistweaver.teachings_of_the_monastery = _ST( "Teachings of the Monastery" );
    talent.mistweaver.crane_style                = _ST( "Crane Style" );
    talent.mistweaver.revival                    = _ST( "Revival" );
    talent.mistweaver.restoral                   = _ST( "Restoral" );
    talent.mistweaver.invigorating_mists         = _ST( "Invigorating Mists" );
    // Row 5
    talent.mistweaver.nourishing_chi      = _ST( "Nourishing Chi" );
    talent.mistweaver.calming_coalescence = _ST( "Calming Coalescence" );
    talent.mistweaver.uplifting_spirits   = _ST( "Uplifting Spirits" );
    talent.mistweaver.energizing_brew     = _ST( "Energizing Brew" );
    talent.mistweaver.lifecycles          = _ST( "Lifecycles" );
    talent.mistweaver.zen_pulse           = _ST( "Zen Pulse" );
    // Row 6
    talent.mistweaver.mists_of_life                 = _ST( "Mists of Life" );
    talent.mistweaver.overflowing_mists             = _ST( "Overflowing Mists" );
    talent.mistweaver.invoke_yulon_the_jade_serpent = _ST( "Invoke Yu'lon, the Jade Serpent" );
    talent.mistweaver.invoke_chi_ji_the_red_crane   = _ST( "Invoke Chi-Ji, the Red Crane" );
    talent.mistweaver.deep_clarity                  = _ST( "Deep Clarity" );
    talent.mistweaver.rapid_diffusion               = _ST( "Rapid Diffusion" );
    // Row 7
    talent.mistweaver.chrysalis                 = _ST( "Chrysalis" );
    talent.mistweaver.burst_of_life             = _ST( "Burst of Life" );
    talent.mistweaver.yulons_whisper            = _ST( "Yu'lon's Whisper" );
    talent.mistweaver.mist_wrap                 = _ST( "Mist Wrap" );
    talent.mistweaver.refreshing_jade_wind      = _ST( "Refreshing Jade Wind" );
    talent.mistweaver.refreshing_jade_wind_tick = find_spell( 162530 );
    talent.mistweaver.celestial_harmony         = _ST( "Celestial Harmony" );
    talent.mistweaver.dancing_mists             = _ST( "Dancing Mists" );
    talent.mistweaver.chi_harmony               = _ST( "Chi Harmony" );
    // Row 8
    talent.mistweaver.jadefire_stomp         = _ST( "Jadefire Stomp" );
    talent.mistweaver.peer_into_peace        = _ST( "Peer Into Peace" );
    talent.mistweaver.jade_bond              = _ST( "Jade Bond" );
    talent.mistweaver.gift_of_the_celestials = _ST( "Gift of the Celestials" );
    talent.mistweaver.focused_thunder        = _ST( "Focused Thunder" );
    talent.mistweaver.sheiluns_gift          = _ST( "Sheilun's Gift" );
    // Row 9
    talent.mistweaver.ancient_concordance = _ST( "Ancient Concordance" );
    talent.mistweaver.ancient_teachings   = _ST( "Ancient Teachings" );
    talent.mistweaver.resplendent_mist    = _ST( "Resplendent Mist" );
    talent.mistweaver.secret_infusion     = _ST( "Secret Infusion" );
    talent.mistweaver.misty_peaks         = _ST( "Misty Peaks" );
    talent.mistweaver.peaceful_mending    = _ST( "Peaceful Mending" );
    talent.mistweaver.veil_of_pride       = _ST( "Veil of Pride" );
    talent.mistweaver.shaohaos_lessons    = _ST( "Shaohao's Lessons" );
    // Row 10
    talent.mistweaver.awakened_jadefire     = _ST( "Awakened Jadefire" );
    talent.mistweaver.dance_of_chiji        = _ST( "Dance of Chi-Ji" );
    talent.mistweaver.tea_of_serenity       = _ST( "Tea of Serenity" );
    talent.mistweaver.tea_of_plenty         = _ST( "Tea of Plenty" );
    talent.mistweaver.unison                = _ST( "Unison" );
    talent.mistweaver.mending_proliferation = _ST( "Mending Proliferation" );
    talent.mistweaver.invokers_delight      = _ST( "Invoker's Delight" );
    talent.mistweaver.tear_of_morning       = _ST( "Tear of Morning" );
    talent.mistweaver.rising_mist           = _ST( "Rising Mist" );
    talent.mistweaver.legacy_of_wisdom      = _ST( "Legacy of Wisdom" );
  }

  // monk_t::talent::windwalker
  {
    current_spec = MONK_WINDWALKER;
    // Row 1
    talent.windwalker.fists_of_fury = _ST( "Fists of Fury" );
    // Row 2
    talent.windwalker.momentum_boost           = _ST( "Momentum Boost" );
    talent.windwalker.combat_wisdom            = _ST( "Combat Wisdom" );
    talent.windwalker.combat_wisdom_expel_harm = find_spell( 451968 );
    talent.windwalker.acclamation              = _ST( "Acclamation" );
    // Row 3
    talent.windwalker.touch_of_the_tiger = _ST( "Touch of the Tiger" );
    talent.windwalker.hardened_soles     = _ST( "Hardened Soles" );
    talent.windwalker.ascension          = _ST( "Ascension" );  // TODO: NYI: EFFECT 2 ENERGY REGEN
    talent.windwalker.ferociousness      = _ST( "Ferociousness" );
    // Row 4
    talent.windwalker.crane_vortex                             = _ST( "Crane Vortex" );
    talent.windwalker.teachings_of_the_monastery               = _ST( "Teachings of the Monastery" );
    talent.windwalker.teachings_of_the_monastery_blackout_kick = find_spell( 228649 );
    talent.windwalker.glory_of_the_dawn                        = _ST( "Glory of the Dawn" );
    // 8 Required
    // Row 5
    talent.windwalker.jade_ignition        = _STID( 392979 );  // _ST( "Jade Ignition" );
    talent.windwalker.courageous_impulse   = _ST( "Courageous Impulse" );
    talent.windwalker.storm_earth_and_fire = _ST( "Storm, Earth, and Fire" );
    talent.windwalker.flurry_of_xuen       = _ST( "Flurry of Xuen" );
    talent.windwalker.hit_combo            = _ST( "Hit Combo" );
    talent.windwalker.brawlers_intensity   = _ST( "Brawler's Intensity" );
    talent.windwalker.meridian_strikes     = _ST( "Meridian Strikes" );
    // Row 6
    talent.windwalker.dance_of_chiji         = _ST( "Dance of Chi-Ji" );
    talent.windwalker.drinking_horn_cover    = _ST( "Drinking Horn Cover" );
    talent.windwalker.spiritual_focus        = _ST( "Spiritual Focus" );
    talent.windwalker.ordered_elements       = _ST( "Ordered Elements" );
    talent.windwalker.strike_of_the_windlord = _ST( "Strike of the Windlord" );
    // Row 7
    talent.windwalker.martial_mixture             = _ST( "Martial Mixture" );
    talent.windwalker.energy_burst                = _ST( "Energy Burst" );
    talent.windwalker.shadowboxing_treads         = _STID( 392982 );
    talent.windwalker.invoke_xuen_the_white_tiger = _ST( "Invoke Xuen, the White Tiger" );
    talent.windwalker.inner_peace                 = _ST( "Inner Peace" );
    talent.windwalker.rushing_jade_wind           = _ST( "Rushing Jade Wind" );
    talent.windwalker.thunderfist                 = _ST( "Thunderfist" );
    // 20 Required
    // Row 8
    talent.windwalker.sequenced_strikes = _ST( "Sequenced Strikes" );
    talent.windwalker.rising_star       = _ST( "Rising Star" );
    talent.windwalker.invokers_delight  = _ST( "Invoker's Delight" );
    talent.windwalker.dual_threat       = _ST( "Dual Threat" );
    talent.windwalker.gale_force        = _STID( 451580 );
    talent.windwalker.gale_force_damage = find_spell( 451585 );
    // Row 9
    talent.windwalker.last_emperors_capacitor    = _ST( "Last Emperor's Capacitor" );
    talent.windwalker.whirling_dragon_punch      = _ST( "Whirling Dragon Punch" );
    talent.windwalker.whirling_dragon_punch_buff = find_spell( 196742 );
    talent.windwalker.xuens_bond                 = _ST( "Xuen's Bond" );
    talent.windwalker.xuens_battlegear           = _ST( "Xuen's Battlegear" );
    talent.windwalker.transfer_the_power         = _ST( "Transfer the Power" );
    talent.windwalker.jadefire_fists             = _ST( "Jadefire Fists" );
    talent.windwalker.jadefire_stomp             = _ST( "Jadefire Stomp" );
    talent.windwalker.jadefire_stomp_damage      = find_spell( 388207 );
    talent.windwalker.jadefire_stomp_ww_damage   = find_spell( 388201 );
    talent.windwalker.communion_with_wind        = _ST( "Communion With Wind" );
    // Row 10
    talent.windwalker.power_of_the_thunder_king      = _ST( "Power of the Thunder King" );
    talent.windwalker.revolving_whirl                = _ST( "Revolving Whirl" );
    talent.windwalker.knowledge_of_the_broken_temple = _ST( "Knowledge of the Broken Temple" );
    talent.windwalker.memory_of_the_monastery        = _ST( "Memory of the Monastery" );
    talent.windwalker.fury_of_xuen                   = _ST( "Fury of Xuen" );
    talent.windwalker.path_of_jade                   = _ST( "Path of Jade" );
    talent.windwalker.singularly_focused_jade        = _ST( "Singularly Focused Jade" );
    talent.windwalker.jadefire_harmony               = _ST( "Jadefire Harmony" );
    talent.windwalker.jadefire_brand_dmg             = find_spell( 395414 );
    talent.windwalker.jadefire_brand_heal            = find_spell( 395413 );
    talent.windwalker.darting_hurricane              = _ST( "Darting Hurricane" );
  }

  current_spec = specialization();

  // monk_t::talent::conduit_of_the_celestials
  {
    // Row 1
    talent.conduit_of_the_celestials.celestial_conduit      = _HT( "Celestial Conduit" );
    talent.conduit_of_the_celestials.celestial_conduit_dmg  = find_spell( 443038 );
    talent.conduit_of_the_celestials.celestial_conduit_heal = find_spell( 443039 );
    // Row 2
    talent.conduit_of_the_celestials.temple_training                   = _HT( "Temple Training" );
    talent.conduit_of_the_celestials.xuens_guidance                    = _HT( "Xuen's Guidance" );
    talent.conduit_of_the_celestials.courage_of_the_white_tiger        = _HT( "Courage of the White Tiger" );
    talent.conduit_of_the_celestials.courage_of_the_white_tiger_damage = find_spell( 457917 );
    talent.conduit_of_the_celestials.courage_of_the_white_tiger_heal   = find_spell( 443106 );
    talent.conduit_of_the_celestials.restore_balance                   = _HT( "Restore Balance" );
    // Row 3
    talent.conduit_of_the_celestials.heart_of_the_jade_serpent              = _HT( "Heart of the Jade Serpent" );
    talent.conduit_of_the_celestials.strength_of_the_black_ox               = _HT( "Strength of the Black Ox" );
    talent.conduit_of_the_celestials.strength_of_the_black_ox_absorb        = find_spell( 443113 );
    talent.conduit_of_the_celestials.strength_of_the_black_ox_damage        = find_spell( 443127 );
    talent.conduit_of_the_celestials.flight_of_the_red_crane                = _HT( "Flight of the Red Crane" );
    talent.conduit_of_the_celestials.flight_of_the_red_crane_dmg            = find_spell( 443263 );
    talent.conduit_of_the_celestials.flight_of_the_red_crane_heal           = find_spell( 443272 );
    talent.conduit_of_the_celestials.flight_of_the_red_crane_celestial_dmg  = find_spell( 443611 );
    talent.conduit_of_the_celestials.flight_of_the_red_crane_celestial_heal = find_spell( 443614 );
    // Row 4
    talent.conduit_of_the_celestials.niuzaos_protection = _HT( "Niuzao's Protection" );
    talent.conduit_of_the_celestials.jade_sanctuary     = _HT( "Jade Sanctuary" );
    talent.conduit_of_the_celestials.chi_jis_swiftness  = _HT( "Chi-Ji's Swiftness" );
    talent.conduit_of_the_celestials.inner_compass      = _HT( "Inner Compass" );
    talent.conduit_of_the_celestials.august_dynasty     = _HT( "August Dynasty" );
    // Row 5
    talent.conduit_of_the_celestials.unity_within          = _HT( "Unity Within" );
    talent.conduit_of_the_celestials.unity_within_dmg_mult = find_spell( 443591 );
  }

  // monk_t::talent::master_of_harmony
  {
    // Row 1
    talent.master_of_harmony.aspect_of_harmony             = _HT( "Aspect of Harmony" );
    talent.master_of_harmony.aspect_of_harmony_driver      = find_spell( 450567 );
    talent.master_of_harmony.aspect_of_harmony_accumulator = find_spell( 450521 );
    talent.master_of_harmony.aspect_of_harmony_spender     = find_spell( 450711 );
    talent.master_of_harmony.aspect_of_harmony_damage      = find_spell( 450763 );
    talent.master_of_harmony.aspect_of_harmony_heal        = find_spell( 450769 );
    // Row 2
    talent.master_of_harmony.manifestation               = _HT( "Manifestation" );
    talent.master_of_harmony.purified_spirit             = _HT( "Purified Spirit" );
    talent.master_of_harmony.purified_spirit_damage      = find_spell( 450820 );
    talent.master_of_harmony.purified_spirit_heal        = find_spell( 450805 );
    talent.master_of_harmony.harmonic_gambit             = _HT( "Harmonic Gambit" );
    talent.master_of_harmony.balanced_stratagem          = _HT( "Balanced Stratagem" );
    talent.master_of_harmony.balanced_stratagem_magic    = find_spell( 451508 );
    talent.master_of_harmony.balanced_stratagem_physical = find_spell( 451514 );
    // Row 3
    talent.master_of_harmony.tigers_vigor          = _HT( "Tiger's Vigor" );
    talent.master_of_harmony.roar_from_the_heavens = _HT( "Roar from the Heavens" );
    talent.master_of_harmony.endless_draught       = _HT( "Endless Draught" );
    talent.master_of_harmony.mantra_of_purity      = _HT( "Mantra of Purity" );
    talent.master_of_harmony.mantra_of_tenacity    = _HT( "Mantra of Tenacity" );
    // Row 4
    talent.master_of_harmony.overwhelming_force        = _HT( "Overwhelming Force" );
    talent.master_of_harmony.overwhelming_force_damage = find_spell( 452333 );
    talent.master_of_harmony.path_of_resurgence        = _HT( "Path of Resurgence" );
    talent.master_of_harmony.way_of_a_thousand_strikes = _HT( "Way of a Thousand Strikes" );
    talent.master_of_harmony.clarity_of_purpose        = _HT( "Clarity of Purpose" );
    // Row 5
    talent.master_of_harmony.coalescence = _HT( "Coalescence" );
  }

  // monk_t::talent::shado-pan
  {
    // Row 1
    talent.shado_pan.flurry_strikes     = _HT( "Flurry Strikes" );
    talent.shado_pan.flurry_strikes_hit = find_spell( 450617 );
    // Row 2
    talent.shado_pan.pride_of_pandaria  = _HT( "Pride of Pandaria" );
    talent.shado_pan.high_impact        = _HT( "High Impact" );
    talent.shado_pan.high_impact_debuff = find_spell( 451037 );
    talent.shado_pan.veterans_eye       = _HT( "Veteran's Eye" );
    talent.shado_pan.martial_precision  = _HT( "Martial Precision" );
    // Row 3
    talent.shado_pan.protect_and_serve   = _HT( "Protect and Serve" );
    talent.shado_pan.lead_from_the_front = _HT( "Lead from the Front" );
    talent.shado_pan.one_versus_many     = _HT( "One Versus Many" );
    talent.shado_pan.whirling_steel      = _HT( "Whirling Steel" );
    talent.shado_pan.predictive_training = _HT( "Predictive Training" );
    // Row 4
    talent.shado_pan.against_all_odds   = _HT( "Against All Odds" );
    talent.shado_pan.efficient_training = _HT( "Efficient Training" );
    talent.shado_pan.vigilant_watch     = _HT( "Vigilant Watch" );
    // Row 5
    talent.shado_pan.wisdom_of_the_wall        = _HT( "Wisdom of the Wall" );
    talent.shado_pan.wisdom_of_the_wall_flurry = find_spell( 451250 );
  }

  // monk_t::talent::tier
  {
    tier.tww1.ww_4pc                      = find_spell( 454505 );
    tier.tww1.ww_4pc_dmg                  = find_spell( 454508 );
    tier.tww1.brm_4pc_damage_buff         = find_spell( 457257 );
    tier.tww1.brm_4pc_free_keg_smash_buff = find_spell( 457271 );
  }

  // Passives =========================================
  // General
  passives.rushing_jade_wind      = find_spell( 116847 );
  passives.rushing_jade_wind_tick = find_spell( 148187 );

  // Windwalker
  passives.bok_proc                         = find_spell( 116768 );
  passives.chi_explosion                    = find_spell( 393056 );
  passives.crackling_tiger_lightning        = find_spell( 123996 );
  passives.crackling_tiger_lightning_driver = find_spell( 123999 );
  passives.cyclone_strikes                  = find_spell( 220358 );
  passives.dance_of_chiji                   = find_spell( 325202 );
  passives.dance_of_chiji_bug               = find_spell( 286585 );
  passives.dual_threat_kick                 = find_spell( 451839 );
  passives.dizzying_kicks                   = find_spell( 196723 );
  passives.empowered_tiger_lightning        = find_spell( 335913 );
  passives.fists_of_fury_tick               = find_spell( 117418 );
  passives.flurry_of_xuen_driver            = find_spell( 452117 );
  passives.focus_of_xuen                    = find_spell( 252768 );
  passives.fury_of_xuen_stacking_buff       = find_spell( 396167 );
  passives.fury_of_xuen                     = find_spell( 396168 );
  passives.glory_of_the_dawn_damage         = find_spell( 392959 );
  passives.hidden_masters_forbidden_touch   = find_spell( 213114 );
  passives.hit_combo                        = find_spell( 196741 );
  passives.improved_touch_of_death          = find_spell( 322113 );
  passives.mark_of_the_crane                = find_spell( 228287 );
  passives.summon_white_tiger_statue        = find_spell( 388686 );
  passives.thunderfist                      = find_spell( 393565 );
  passives.touch_of_karma_tick              = find_spell( 124280 );
  passives.whirling_dragon_punch_aoe_tick   = find_spell( 158221 );
  passives.whirling_dragon_punch_st_tick    = find_spell( 451767 );

  //================================================================================
  // Shared Spells
  // These spells share common effects but are unique in that you may only have one
  //================================================================================

  // Returns first valid spell in argument list, pass highest priority to first argument
  // Returns spell_data_t::not_found() if none are valid
  auto _priority = []( auto... spell_list ) {
    for ( const auto &spell : { spell_list... } )
      if ( spell && spell->ok() )
        return spell.spell();
    return spell_data_t::not_found();
  };

  shared.jadefire_stomp = _priority( talent.windwalker.jadefire_stomp, talent.mistweaver.jadefire_stomp );

  shared.invokers_delight = _priority( talent.windwalker.invokers_delight, talent.mistweaver.invokers_delight );

  shared.rushing_jade_wind = _priority( talent.windwalker.rushing_jade_wind, talent.brewmaster.rushing_jade_wind );

  shared.teachings_of_the_monastery =
      _priority( talent.windwalker.teachings_of_the_monastery, talent.mistweaver.teachings_of_the_monastery );

  // Active Action Spells

  // General
  active_actions.chi_wave = new actions::chi_wave_t( this );
  windwalking_aura        = new actions::windwalking_aura_t( this );

  // Conduit of the Celestials
  bool uw  = talent.conduit_of_the_celestials.unity_within->ok();
  bool cwt = talent.conduit_of_the_celestials.courage_of_the_white_tiger->ok() || uw;
  bool frc = talent.conduit_of_the_celestials.flight_of_the_red_crane->ok() || uw;
  bool sbt = talent.conduit_of_the_celestials.strength_of_the_black_ox->ok() || uw;

  if ( cwt )
  {
    active_actions.courage_of_the_white_tiger = new actions::courage_of_the_white_tiger_t( this );
  }

  if ( frc )
  {
    active_actions.flight_of_the_red_crane_damage = new actions::flight_of_the_red_crane_dmg_t( this );
    active_actions.flight_of_the_red_crane_heal   = new actions::flight_of_the_red_crane_heal_t( this );
    active_actions.flight_of_the_red_crane_celestial_damage =
        new actions::flight_of_the_red_crane_celestial_dmg_t( this );
    active_actions.flight_of_the_red_crane_celestial_heal =
        new actions::flight_of_the_red_crane_celestial_heal_t( this );
  }

  if ( sbt )
  {
    active_actions.strength_of_the_black_ox_dmg    = new actions::strength_of_the_black_ox_t( this );
    active_actions.strength_of_the_black_ox_absorb = new actions::strength_of_the_black_ox_absorb_t( this );
  }

  // Shado-Pan
  if ( talent.shado_pan.flurry_strikes->ok() )
    active_actions.flurry_strikes = new actions::flurry_strikes_t( this );

  // Brewmaster
  if ( specialization() == MONK_BREWMASTER )
  {
    active_actions.special_delivery           = new actions::special_delivery_t( this );
    active_actions.breath_of_fire             = new actions::spells::breath_of_fire_dot_t( this );
    active_actions.celestial_fortune          = new actions::heals::celestial_fortune_t( this );
    active_actions.exploding_keg              = new actions::spells::exploding_keg_proc_t( this );
    active_actions.niuzao_call_to_arms_summon = new actions::niuzao_call_to_arms_summon_t( this );

    active_actions.chi_surge = new actions::spells::chi_surge_t( this );
  }

  // Windwalker
  if ( specialization() == MONK_WINDWALKER )
  {
    active_actions.empowered_tiger_lightning = new actions::empowered_tiger_lightning_t( this );
    active_actions.flurry_of_xuen            = new actions::flurry_of_xuen_t( this );
    active_actions.fury_of_xuen_summon       = new actions::fury_of_xuen_summon_t( this );
    active_actions.fury_of_xuen_empowered_tiger_lightning =
        new actions::fury_of_xuen_empowered_tiger_lightning_t( this );
    active_actions.gale_force = new actions::gale_force_t( this );
  }

  // Passive Action Spells
  passive_actions.combat_wisdom_eh    = new actions::heals::expel_harm_t( this, "" );
  passive_actions.thunderfist         = new actions::thunderfist_t( this );
  passive_actions.press_the_advantage = new actions::press_the_advantage_melee_t( this );
}

// monk_t::init_base ========================================================

void monk_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    if ( specialization() == MONK_MISTWEAVER )
      base.distance = 40;
    else
      base.distance = 5;
  }
  base_t::init_base_stats();

  base_gcd = timespan_t::from_seconds( 1.5 );

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
    {
      // base_gcd += spec.brewmaster_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds
      base.attack_power_per_agility                      = 1.0;
      resources.base[ RESOURCE_ENERGY ]                  = 100;
      resources.base[ RESOURCE_MANA ]                    = 0;
      resources.base[ RESOURCE_CHI ]                     = 0;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10.0;
      resources.base_regen_per_second[ RESOURCE_MANA ]   = 0;
      break;
    }
    case MONK_MISTWEAVER:
    {
      base.spell_power_per_intellect                     = 1.0;
      resources.base[ RESOURCE_ENERGY ]                  = 0;
      resources.base[ RESOURCE_CHI ]                     = 0;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
      break;
    }
    case MONK_WINDWALKER:
    {
      if ( base.distance < 1 )
        base.distance = 5;
      // base_gcd += baseline.windwalker.aura->effectN( 14 ).time_value();  // Saved as -500 milliseconds
      base.attack_power_per_agility     = 1.0;
      resources.base[ RESOURCE_ENERGY ] = 100;
      resources.base[ RESOURCE_ENERGY ] += talent.windwalker.ascension->effectN( 3 ).base_value();
      resources.base[ RESOURCE_ENERGY ] += talent.windwalker.inner_peace->effectN( 1 ).base_value();
      resources.base[ RESOURCE_MANA ] = 0;
      resources.base[ RESOURCE_CHI ]  = 4;
      resources.base[ RESOURCE_CHI ] += baseline.windwalker.aura->effectN( 10 ).base_value();
      resources.base[ RESOURCE_CHI ] += talent.windwalker.ascension->effectN( 1 ).base_value();
      resources.base_regen_per_second[ RESOURCE_ENERGY ] =
          10.0 * ( 1 + talent.windwalker.ascension->effectN( 2 ).percent() );
      resources.base_regen_per_second[ RESOURCE_MANA ] = 0;
      break;
    }
    default:
      break;
  }

  resources.base_regen_per_second[ RESOURCE_CHI ] = 0;
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  base_t::init_scaling();

  if ( specialization() != MONK_MISTWEAVER )
  {
    scaling->disable( STAT_INTELLECT );
    scaling->disable( STAT_SPELL_POWER );
    scaling->enable( STAT_AGILITY );
    scaling->enable( STAT_WEAPON_DPS );
  }
  else
  {
    scaling->disable( STAT_AGILITY );
    scaling->disable( STAT_MASTERY_RATING );
    scaling->disable( STAT_ATTACK_POWER );
    scaling->disable( STAT_WEAPON_DPS );
    scaling->enable( STAT_INTELLECT );
    scaling->enable( STAT_SPIRIT );
  }
  scaling->disable( STAT_STRENGTH );

  if ( specialization() == MONK_WINDWALKER )
  {
    // Touch of Death
    scaling->enable( STAT_STAMINA );
  }
  if ( specialization() == MONK_BREWMASTER )
  {
    scaling->enable( STAT_BONUS_ARMOR );
  }

  if ( off_hand_weapon.type != WEAPON_NONE )
    scaling->enable( STAT_WEAPON_OFFHAND_DPS );
}

// monk_t::create_buffs =====================================================

struct self_damage_override : stagger_impl::self_damage_t<monk_t>
{
  self_damage_override( monk_t *player, stagger_impl::stagger_effect_t<monk_t> *stagger_effect )
    : stagger_impl::self_damage_t<monk_t>( player, stagger_effect )
  {
    dot_duration = player->find_spell( 124273 )->duration();
    dot_duration +=
        timespan_t::from_seconds( player->talent.brewmaster.bob_and_weave->effectN( 1 ).base_value() / 10.0 );
  }
};

struct debuff_override : stagger_impl::debuff_t<monk_t>
{
  using base_t = stagger_impl::debuff_t<monk_t>;
  debuff_override( monk_t *player, const stagger_data_t *parent_data, const level_data_t *data )
    : base_t( player, parent_data, data )
  {
    set_default_value_from_effect_type( A_HASTE_ALL );
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    apply_affecting_aura( player->talent.brewmaster.high_tolerance );
    set_stack_change_callback( [ player ]( buff_t *, int old_, int new_ ) {
      if ( old_ )
        player->buff.training_of_niuzao->expire();
      if ( new_ )
        player->buff.training_of_niuzao->trigger();
    } );
  }
};

struct training_of_niuzao_buff : actions::monk_buff_t
{
  training_of_niuzao_buff( monk_t *player )
    : actions::monk_buff_t( player, "training_of_niuzao", player->talent.brewmaster.training_of_niuzao )
  {
    set_default_value( 0.0 );
    set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
  }

  bool trigger( int /* stacks */ = -1, double /* value */ = DEFAULT_VALUE(), double chance = -1.0,
                timespan_t duration = timespan_t::min() ) override
  {
    double v = p().find_stagger( "Stagger" )->level_index() * data().effectN( 1 ).base_value();
    return actions::monk_buff_t::trigger( 1, v, chance, duration );
  }
};

void monk_t::create_buffs()
{
  create_stagger<debuff_override, self_damage_override>(
      { baseline.brewmaster.stagger_self_damage,
        { { baseline.brewmaster.light_stagger, 0.0 },
          { baseline.brewmaster.moderate_stagger, 0.2 },
          { baseline.brewmaster.heavy_stagger, 0.6 },
          { spell_data_t::nil(), 10.0 } },
        { "quick_sip", "staggering_strikes", "touch_of_death", "purifying_brew", "tranquil_spirit_eh",
          "tranquil_spirit_goto" },
        [ this ]() { return specialization() == MONK_BREWMASTER; },
        [ this ]( school_e, result_amount_type, const action_state_t *state ) {
          if ( state->action->id == baseline.brewmaster.stagger_self_damage->id() )
            return false;
          return true;
        },
        [ this ]( school_e school, result_amount_type, action_state_t *state ) {
          double stagger_rating = agility() * baseline.brewmaster.stagger->effectN( 1 ).percent();
          if ( talent.brewmaster.high_tolerance->ok() )
            stagger_rating *= 1.0 + talent.brewmaster.high_tolerance->effectN( 5 ).percent();

          if ( talent.brewmaster.fortifying_brew_determination->ok() && buff.fortifying_brew->up() )
            stagger_rating *= 1.0 + talent.monk.fortifying_brew_buff->effectN( 6 ).percent();

          if ( buff.shuffle->up() )
            stagger_rating *= 1.0 + talent.brewmaster.shuffle_buff->effectN( 1 ).percent();

          // multiplier is not available in spell data :(
          if ( buff.ox_stance->up() && state->result_amount / current_health() >
                                           talent.brewmaster.ox_stance->effectN( 1 ).percent() +
                                               talent.brewmaster.heightened_guard->effectN( 1 ).percent() )
          {
            stagger_rating *= 1.0 + 0.4;
            buff.ox_stance->decrement();
          }

          double k = dbc->armor_mitigation_constant( state->target->level() );
          k *= dbc->get_armor_constant_mod( difficulty_e::MYTHIC );

          double stagger_percent = stagger_rating / ( stagger_rating + k );

          // order of operations here is untestable with current in game values
          if ( school != SCHOOL_PHYSICAL )
            stagger_percent *= baseline.brewmaster.stagger->effectN( 5 ).percent();

          return std::min( stagger_percent, 0.99 );
        } } );

  base_t::create_buffs();

  // General
  buff.chi_burst = make_buff_fallback( talent.monk.chi_burst->ok() && specialization() == MONK_WINDWALKER, this,
                                       "chi_burst", talent.monk.chi_burst_buff );

  // buff.channeling_soothing_mist = make_buff( this, "channeling_soothing_mist", passives.soothing_mist_heal )
  //                                     ->set_trigger_spell( talent.monk.soothing_mist );

  buff.chi_torpedo = make_buff_fallback( talent.monk.chi_torpedo->ok(), this, "chi_torpedo", find_spell( 119085 ) )
                         ->set_trigger_spell( talent.monk.chi_torpedo )
                         ->set_default_value_from_effect( 1 );

  buff.chi_wave = make_buff_fallback( talent.monk.chi_wave->ok(), this, "chi_wave", talent.monk.chi_wave_buff );

  buff.dampen_harm = make_buff_fallback( talent.monk.dampen_harm->ok(), this, "dampen_harm", talent.monk.dampen_harm );

  buff.diffuse_magic = make_buff_fallback( talent.monk.diffuse_magic->ok() && specialization() == MONK_BREWMASTER, this,
                                           "diffuse_magic", talent.monk.diffuse_magic )
                           ->set_default_value_from_effect( 1 );

  buff.invokers_delight = make_buff_fallback( shared.invokers_delight->ok(), this, "invokers_delight",
                                              shared.invokers_delight->effectN( 1 ).trigger() )
                              ->set_trigger_spell( shared.invokers_delight );

  buff.combat_wisdom =
      make_buff_fallback( talent.windwalker.combat_wisdom->ok(), this, "combat_wisdom", find_spell( 129914 ) )
          ->set_trigger_spell( talent.windwalker.combat_wisdom )
          ->set_default_value_from_effect( 1 );

  buff.fatal_touch = make_buff_fallback( talent.monk.fatal_touch->ok(), this, "fatal_touch",
                                         talent.monk.fatal_touch->effectN( 2 ).trigger() )
                         ->set_trigger_spell( talent.monk.fatal_touch );

  buff.fortifying_brew = make_buff_fallback<buffs::fortifying_brew_t>(
      talent.monk.fortifying_brew->ok() && specialization() == MONK_BREWMASTER, this, "fortifying_brew" );

  buff.jadefire_stomp = make_buff_fallback( shared.jadefire_stomp->ok(), this, "jadefire_stomp", find_spell( 388193 ) )
                            ->set_trigger_spell( shared.jadefire_stomp )
                            ->set_default_value_from_effect( 2 );

  buff.rushing_jade_wind = make_buff_fallback<buffs::rushing_jade_wind_buff_t>(
      talent.brewmaster.rushing_jade_wind->ok() || talent.windwalker.rushing_jade_wind->ok() ||
          talent.conduit_of_the_celestials.restore_balance->ok(),
      this, "rushing_jade_wind" );

  buff.spinning_crane_kick = make_buff( this, "spinning_crane_kick", baseline.monk.spinning_crane_kick )
                                 ->set_default_value_from_effect( 2 )
                                 ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.teachings_of_the_monastery =
      make_buff_fallback( shared.teachings_of_the_monastery->ok(), this, "teachings_of_the_monastery",
                          find_spell( 202090 ) )
          ->set_trigger_spell( shared.teachings_of_the_monastery )
          ->set_default_value_from_effect( 1 )
          ->modify_max_stack( as<int>( talent.windwalker.knowledge_of_the_broken_temple->effectN( 3 ).base_value() ) );

  buff.windwalking_driver = new buffs::windwalking_driver_t( this, "windwalking_aura_driver", find_spell( 365080 ) );

  buff.yulons_grace = make_buff<absorb_buff_t>( this, "yulons_grace", find_spell( 414143 ) );

  // Brewmaster
  buff.training_of_niuzao = make_buff_fallback<training_of_niuzao_buff>( talent.brewmaster.training_of_niuzao->ok(),
                                                                         this, "training_of_niuzao" );
  buff.ox_stance =
      make_buff_fallback( talent.brewmaster.ox_stance->ok(), this, "ox_stance", talent.brewmaster.ox_stance_buff );

  buff.blackout_combo = make_buff_fallback( talent.brewmaster.blackout_combo->ok(), this, "blackout_combo",
                                            talent.brewmaster.blackout_combo->effectN( 5 ).trigger() );

  buff.call_to_arms_invoke_niuzao = make_buff_fallback(
      talent.brewmaster.call_to_arms->ok(), this, "call_to_arms_invoke_niuzao", talent.brewmaster.call_to_arms_buff );

  buff.celestial_brew = make_buff<absorb_buff_t>( this, "celestial_brew", talent.brewmaster.celestial_brew );
  buff.celestial_brew->set_absorb_source( get_stats( "celestial_brew" ) )->set_cooldown( timespan_t::zero() );

  buff.purified_chi =
      make_buff_fallback( talent.brewmaster.celestial_brew->ok(), this, "purified_chi", talent.brewmaster.purified_chi )
          ->set_trigger_spell( talent.brewmaster.celestial_brew )
          ->set_default_value_from_effect( 1 );

  buff.charred_passions = make_buff_fallback( talent.brewmaster.charred_passions->ok(), this, "charred_passions",
                                              talent.brewmaster.charred_passions->effectN( 1 ).trigger() )
                              ->set_trigger_spell( talent.brewmaster.charred_passions );

  buff.counterstrike = make_buff_fallback( talent.brewmaster.counterstrike->ok(), this, "counterstrike",
                                           talent.brewmaster.counterstrike->effectN( 1 ).trigger() )
                           ->set_cooldown( talent.brewmaster.counterstrike->internal_cooldown() );

  buff.celestial_flames =
      make_buff( this, "celestial_flames", talent.brewmaster.celestial_flames->effectN( 1 ).trigger() )
          ->set_chance( talent.brewmaster.celestial_flames->proc_chance() )
          ->set_default_value_from_effect( 1 );

  buff.elusive_brawler = make_buff( this, "elusive_brawler", baseline.brewmaster.mastery->effectN( 3 ).trigger() )
                             ->add_invalidate( CACHE_DODGE );

  buff.exploding_keg =
      make_buff( this, "exploding_keg", talent.brewmaster.exploding_keg )->set_default_value_from_effect( 2 );

  if ( talent.brewmaster.gift_of_the_ox->ok() || talent.brewmaster.spirit_of_the_ox->ok() )
    buff.gift_of_the_ox = new buffs::gift_of_the_ox_t( this );
  else if ( specialization() == MONK_BREWMASTER )
    buff_t::make_fallback( this, "gift_of_the_ox", this );

  buff.expel_harm_accumulator =
      make_buff_fallback( talent.brewmaster.gift_of_the_ox->ok() || talent.brewmaster.spirit_of_the_ox->ok(), this,
                          "expel_harm_accumulator" )
          ->set_can_cancel( true )
          ->set_quiet( true )
          ->set_cooldown( 0_ms )
          ->set_duration( 1_ms )
          ->set_max_stack( 1 )
          ->set_default_value( 0.0 );

  buff.invoke_niuzao = make_buff( this, "invoke_niuzao_the_black_ox", talent.brewmaster.invoke_niuzao_the_black_ox )
                           ->set_default_value_from_effect( 2 )
                           ->set_cooldown( timespan_t::zero() );

  buff.hit_scheme = make_buff( this, "hit_scheme", talent.brewmaster.hit_scheme->effectN( 1 ).trigger() )
                        ->set_default_value_from_effect( 1 );

  buff.press_the_advantage =
      make_buff( this, "press_the_advantage", talent.brewmaster.press_the_advantage->effectN( 2 ).trigger() )
          ->set_default_value_from_effect( 1 );

  buff.pretense_of_instability =
      make_buff_fallback( talent.brewmaster.pretense_of_instability->ok(), this, "pretense_of_instability",
                          talent.brewmaster.pretense_of_instability->effectN( 1 ).trigger() )
          ->set_trigger_spell( talent.brewmaster.pretense_of_instability )
          ->add_invalidate( CACHE_DODGE );

  buff.shuffle = make_buff<buffs::shuffle_t>( this );

  buff.tiger_strikes =
      make_buff_fallback( sets->set( MONK_WINDWALKER, TWW1, B2 )->ok(), this, "tiger_strikes", find_spell( 454485 ) )
          ->set_trigger_spell( sets->set( MONK_WINDWALKER, TWW1, B2 ) );

  buff.tigers_ferocity =
      make_buff_fallback( sets->set( MONK_WINDWALKER, TWW1, B4 )->ok(), this, "tigers_ferocity", find_spell( 454502 ) )
          ->set_trigger_spell( sets->set( MONK_WINDWALKER, TWW1, B4 ) );

  buff.flow_of_battle_damage = make_buff_fallback( sets->set( MONK_BREWMASTER, TWW1, B4 )->ok(), this,
                                                   "flow_of_battle_damage", tier.tww1.brm_4pc_damage_buff )
                                   ->set_trigger_spell( sets->set( MONK_BREWMASTER, TWW1, B4 ) );

  buff.flow_of_battle_free_keg_smash =
      make_buff_fallback( sets->set( MONK_BREWMASTER, TWW1, B4 )->ok(), this, "flow_of_battle_free_keg_smash",
                          tier.tww1.brm_4pc_free_keg_smash_buff )
          ->set_trigger_spell( sets->set( MONK_BREWMASTER, TWW1, B4 ) );

  buff.weapons_of_order = make_buff_fallback( talent.brewmaster.weapons_of_order->ok(), this, "weapons_of_order",
                                              talent.brewmaster.weapons_of_order )
                              ->set_trigger_spell( talent.brewmaster.weapons_of_order )
                              ->set_cooldown( timespan_t::zero() );

  buff.recent_purifies = make_buff_fallback<buffs::purifying_buff_t>(
      talent.brewmaster.improved_invoke_niuzao_the_black_ox->ok(), this, "recent_purifies" );

  // Mistweaver
  buff.invoke_chiji = make_buff( this, "invoke_chiji", find_spell( 343818 ) )
                          ->set_trigger_spell( talent.mistweaver.invoke_chi_ji_the_red_crane );

  buff.invoke_chiji_evm = make_buff( this, "invoke_chiji_evm", find_spell( 343820 ) )
                              ->set_trigger_spell( talent.mistweaver.invoke_chi_ji_the_red_crane )
                              ->set_default_value_from_effect( 1 );

  buff.life_cocoon = make_buff<absorb_buff_t>( this, "life_cocoon", talent.mistweaver.life_cocoon );
  buff.life_cocoon->set_absorb_source( get_stats( "life_cocoon" ) )
      ->set_trigger_spell( talent.mistweaver.life_cocoon )
      ->set_cooldown( timespan_t::zero() );

  buff.mana_tea = make_buff( this, "mana_tea", talent.mistweaver.mana_tea )->set_default_value_from_effect( 1 );

  buff.lifecycles_enveloping_mist = make_buff( this, "lifecycles_enveloping_mist", find_spell( 197919 ) )
                                        ->set_trigger_spell( talent.mistweaver.lifecycles )
                                        ->set_default_value_from_effect( 1 );

  buff.lifecycles_vivify = make_buff( this, "lifecycles_vivify", find_spell( 197916 ) )
                               ->set_trigger_spell( talent.mistweaver.lifecycles )
                               ->set_default_value_from_effect( 1 );

  buff.refreshing_jade_wind = make_buff( this, "refreshing_jade_wind", talent.mistweaver.refreshing_jade_wind )
                                  ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.thunder_focus_tea =
      make_buff( this, "thunder_focus_tea", talent.mistweaver.thunder_focus_tea )
          ->modify_max_stack( (int)( talent.mistweaver.focused_thunder->effectN( 1 ).base_value() ) );

  // Windwalker
  buff.bok_proc = make_buff_fallback( baseline.windwalker.combo_breaker->ok(), this, "bok_proc", passives.bok_proc )
                      ->set_trigger_spell( baseline.windwalker.combo_breaker )
                      ->set_chance( baseline.windwalker.combo_breaker->effectN( 1 ).percent() *
                                    ( 1.0f + talent.windwalker.memory_of_the_monastery->effectN( 1 ).percent() ) );

  buff.chi_energy =
      make_buff_fallback( talent.windwalker.jade_ignition->ok(), this, "chi_energy", find_spell( 393057 ) )
          ->set_default_value_from_effect( 1 );

  buff.combo_strikes =
      make_buff_fallback( baseline.windwalker.mastery->ok(), this, "combo_strikes" )
          ->set_trigger_spell( baseline.windwalker.mastery )
          ->set_duration( timespan_t::from_minutes( 60 ) )
          ->set_quiet( true );  // In-game does not show this buff but I would like to use it for background stuff

  // Do not use a fallback buff - it is possible to get a dance of chiji proc without the talent from other sources.
  buff.dance_of_chiji = make_buff( this, "dance_of_chiji", passives.dance_of_chiji )
                            ->set_trigger_spell( talent.windwalker.dance_of_chiji );

  buff.dance_of_chiji_hidden = make_buff( this, "dance_of_chiji_hidden" )
                                   ->set_default_value( passives.dance_of_chiji->effectN( 1 ).base_value() )
                                   ->set_duration( timespan_t::from_seconds( 1.5 ) )
                                   ->set_quiet( true );

  buff.darting_hurricane = make_buff_fallback( talent.windwalker.darting_hurricane->ok(), this, "darting_hurricane",
                                               talent.windwalker.darting_hurricane->effectN( 1 ).trigger() )
                               ->set_default_value_from_effect( 1 );

  buff.dual_threat =
      make_buff_fallback( talent.windwalker.dual_threat->ok(), this, "dual_threat", find_spell( 451833 ) )
          ->set_trigger_spell( talent.windwalker.dual_threat )
          ->set_default_value_from_effect( 1 );

  buff.jadefire_brand = make_buff_fallback( talent.windwalker.jadefire_harmony->ok(), this, "jadefire_brand_heal",
                                            talent.windwalker.jadefire_brand_heal )
                            ->set_default_value_from_effect( 1 );

  buff.ferociousness = make_buff_fallback( talent.windwalker.ferociousness->ok(), this, "ferociousness",
                                           talent.windwalker.ferociousness )
                           ->set_quiet( true )
                           ->set_default_value_from_effect( 1 )
                           ->set_tick_callback( [ this ]( buff_t *self, int, timespan_t ) {
                             double previous     = self->current_value;
                             self->current_value = self->default_value;
                             if ( ( pets.xuen.n_active_pets() + pets.fury_of_xuen_tiger.n_active_pets() ) > 0 )
                               self->current_value *= 1 + self->data().effectN( 2 ).percent();
                             if ( previous != self->current_value )
                               invalidate_cache( CACHE_CRIT_CHANCE );
                           } )
                           ->set_period( 1_s )
                           ->set_freeze_stacks( true )
                           ->set_tick_behavior( buff_tick_behavior::CLIP );

  buff.flying_serpent_kick_movement = make_buff_fallback( baseline.windwalker.flying_serpent_kick->ok(), this,
                                                          "flying_serpent_kick_movement_buff" )  // find_spell( 115057 )
                                          ->set_trigger_spell( baseline.windwalker.flying_serpent_kick );

  buff.fury_of_xuen_stacks = make_buff_fallback<buffs::fury_of_xuen_stacking_buff_t>(
      talent.windwalker.fury_of_xuen->ok(), this, "fury_of_xuen_stacks", passives.fury_of_xuen_stacking_buff );

  buff.fury_of_xuen = make_buff_fallback<buffs::fury_of_xuen_t>( talent.windwalker.fury_of_xuen->ok(), this,
                                                                 "fury_of_xuen", passives.fury_of_xuen );

  buff.hit_combo = make_buff_fallback( talent.windwalker.hit_combo->ok(), this, "hit_combo", passives.hit_combo )
                       ->set_default_value_from_effect( 1 );

  buff.flurry_of_xuen = make_buff_fallback( talent.windwalker.flurry_of_xuen->ok(), this, "flurry_of_xuen",
                                            passives.flurry_of_xuen_driver )
                            ->set_tick_callback( [ this ]( buff_t * /* b */, int, timespan_t ) {
                              active_actions.flurry_of_xuen->execute();
                            } )
                            ->set_tick_behavior( buff_tick_behavior::CLIP )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                            ->set_freeze_stacks( true );

  buff.invoke_xuen = make_buff<buffs::invoke_xuen_the_white_tiger_buff_t>(
      this, "invoke_xuen_the_white_tiger", talent.windwalker.invoke_xuen_the_white_tiger );

  buff.cyclone_strikes = make_buff_fallback( baseline.windwalker.mark_of_the_crane->ok(), this, "cyclone_strikes",
                                             passives.cyclone_strikes )
                             ->set_period( 0_ms )
                             ->set_duration( timespan_t::zero() )
                             ->set_default_value_from_effect( 1 )
                             ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  if ( user_options.motc_override > 0 )
  {
    buff.cyclone_strikes->set_initial_stack( user_options.motc_override )->set_freeze_stacks( true );
  }

  buff.martial_mixture = make_buff_fallback( talent.windwalker.martial_mixture->ok(), this, "martial_mixure",
                                             talent.windwalker.martial_mixture->effectN( 1 ).trigger() )
                             ->set_trigger_spell( talent.windwalker.martial_mixture );

  buff.memory_of_the_monastery = make_buff_fallback( talent.windwalker.memory_of_the_monastery->ok(), this,
                                                     "memory_of_the_monastery", find_spell( 454970 ) )
                                     ->set_trigger_spell( talent.windwalker.memory_of_the_monastery );

  buff.momentum_boost_damage =
      make_buff_fallback( talent.windwalker.momentum_boost->ok(), this, "momentum_boost_damage",
                          talent.windwalker.momentum_boost->effectN( 1 ).trigger() )
          ->set_default_value_from_effect( 1 );

  buff.momentum_boost_speed =
      make_buff_fallback( talent.windwalker.momentum_boost->ok(), this, "momentum_boost_speed", find_spell( 451298 ) )
          ->set_trigger_spell( talent.windwalker.momentum_boost );

  buff.ordered_elements =
      make_buff_fallback( talent.windwalker.ordered_elements->ok(), this, "ordered_elements", find_spell( 451462 ) )
          ->set_trigger_spell( talent.windwalker.ordered_elements );

  buff.pressure_point =
      make_buff_fallback( talent.windwalker.xuens_battlegear->ok(), this, "pressure_point", find_spell( 393053 ) )
          ->set_default_value_from_effect( 1 )
          ->set_refresh_behavior( buff_refresh_behavior::NONE );

  buff.storm_earth_and_fire =
      make_buff_fallback( talent.windwalker.storm_earth_and_fire->ok(), this, "storm_earth_and_fire",
                          talent.windwalker.storm_earth_and_fire )
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER )
          ->set_can_cancel( false )  // Undocumented hotfix 2018-09-28 - SEF can no longer be canceled.
          ->set_cooldown( timespan_t::zero() );

  buff.the_emperors_capacitor = make_buff_fallback( talent.windwalker.last_emperors_capacitor->ok(), this,
                                                    "the_emperors_capacitor", find_spell( 393039 ) )
                                    ->set_default_value_from_effect( 1 );

  buff.thunderfist =
      make_buff_fallback( talent.windwalker.thunderfist->ok(), this, "thunderfist", passives.thunderfist );

  buff.touch_of_death_ww = new buffs::touch_of_death_ww_buff_t( this, "touch_of_death_ww", spell_data_t::nil() );

  buff.touch_of_karma = new buffs::touch_of_karma_buff_t( this, "touch_of_karma", find_spell( 125174 ) );

  buff.transfer_the_power =
      make_buff_fallback( talent.windwalker.transfer_the_power->ok(), this, "transfer_the_power", find_spell( 195321 ) )
          ->set_default_value_from_effect( 1 );

  buff.whirling_dragon_punch = make_buff_fallback<buffs::whirling_dragon_punch_buff_t>(
      talent.windwalker.whirling_dragon_punch->ok(), this, "whirling_dragon_punch" );

  // Conduit of the Celestials
  buff.august_dynasty = make_buff_fallback( talent.conduit_of_the_celestials.august_dynasty->ok(), this,
                                            "august_dynasty", find_spell( 442850 ) );

  buff.celestial_conduit = make_buff_fallback( talent.conduit_of_the_celestials.celestial_conduit->ok(), this,
                                               "celestial_conduit", find_spell( 443028 ) );

  buff.chi_jis_swiftness = make_buff_fallback( talent.conduit_of_the_celestials.chi_jis_swiftness->ok(), this,
                                               "chi_jis_swiftness", find_spell( 443569 ) );

  buff.courage_of_the_white_tiger =
      make_buff_fallback( talent.conduit_of_the_celestials.courage_of_the_white_tiger->ok(), this,
                          "courage_of_the_white_tiger", find_spell( 460127 ) )
          ->set_expire_callback(
              [ this ]( buff_t *, double, timespan_t ) { active_actions.courage_of_the_white_tiger->execute(); } );

  buff.flight_of_the_red_crane = make_buff_fallback( talent.conduit_of_the_celestials.flight_of_the_red_crane->ok(),
                                                     this, "flight_of_the_red_crane", find_spell( 457459 ) );

  buff.heart_of_the_jade_serpent_cdr =
      make_buff_fallback( talent.conduit_of_the_celestials.heart_of_the_jade_serpent->ok(), this,
                          "heart_of_the_jade_serpent_cdr", find_spell( 443421 ) );

  buff.heart_of_the_jade_serpent_cdr_celestial =
      make_buff_fallback( talent.conduit_of_the_celestials.heart_of_the_jade_serpent->ok(), this,
                          "heart_of_the_jade_serpent_cdr_celestial", find_spell( 443616 ) );

  buff.heart_of_the_jade_serpent_stack_mw =
      make_buff_fallback( talent.conduit_of_the_celestials.heart_of_the_jade_serpent->ok(), this,
                          "heart_of_the_jade_serpent_stack_mw", find_spell( 443506 ) )
          ->set_stack_change_callback( [ this ]( buff_t *buff_, int, int new_ ) {
            if ( new_ == buff_->max_stack() )
              buff.heart_of_the_jade_serpent_cdr->trigger();
          } )
          ->set_expire_at_max_stack( true );

  buff.heart_of_the_jade_serpent = make_buff_fallback( talent.conduit_of_the_celestials.heart_of_the_jade_serpent->ok(),
                                                       this, "heart_of_the_jade_serpent", find_spell( 456368 ) );

  buff.heart_of_the_jade_serpent_stack_ww =
      make_buff_fallback( talent.conduit_of_the_celestials.heart_of_the_jade_serpent->ok(), this,
                          "heart_of_the_jade_serpent_stack_ww", find_spell( 443424 ) )
          ->set_stack_change_callback( [ this ]( buff_t *buff_, int, int new_ ) {
            if ( new_ >= buff_->max_stack() )
              buff.heart_of_the_jade_serpent->trigger();
          } )
          ->set_expire_at_max_stack( true );

  buff.inner_compass_crane_stance = make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this,
                                                        "crane_stance", find_spell( 443572 ) )
                                        ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
                                          if ( old_ == 0 )
                                          {
                                            buff.inner_compass_ox_stance->expire();
                                            buff.inner_compass_serpent_stance->expire();
                                            buff.inner_compass_tiger_stance->expire();
                                          }
                                        } );

  buff.inner_compass_ox_stance = make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this,
                                                     "ox_stance", find_spell( 443574 ) )
                                     ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
                                       if ( old_ == 0 )
                                       {
                                         buff.inner_compass_crane_stance->expire();
                                         buff.inner_compass_serpent_stance->expire();
                                         buff.inner_compass_tiger_stance->expire();
                                       }
                                     } );

  buff.inner_compass_serpent_stance = make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this,
                                                          "serpent_stance", find_spell( 443576 ) )
                                          ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
                                            if ( old_ == 0 )
                                            {
                                              buff.inner_compass_crane_stance->expire();
                                              buff.inner_compass_ox_stance->expire();
                                              buff.inner_compass_tiger_stance->expire();
                                            }
                                          } );

  buff.inner_compass_tiger_stance = make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this,
                                                        "tiger_stance", find_spell( 443575 ) )
                                        ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
                                          if ( old_ == 0 )
                                          {
                                            buff.inner_compass_crane_stance->expire();
                                            buff.inner_compass_ox_stance->expire();
                                            buff.inner_compass_serpent_stance->expire();
                                          }
                                        } );

  buff.jade_sanctuary = make_buff_fallback( talent.conduit_of_the_celestials.jade_sanctuary->ok(), this,
                                            "jade_sanctuary", find_spell( 448508 ) );

  buff.strength_of_the_black_ox =
      make_buff_fallback( talent.conduit_of_the_celestials.strength_of_the_black_ox->ok(), this,
                          "strength_of_the_black_ox", find_spell( 443112 ) )
          ->set_expire_callback( [ this ]( buff_t *, double, timespan_t ) {
            if ( specialization() == MONK_MISTWEAVER )
            {
              active_actions.strength_of_the_black_ox_absorb->execute();
            }
            if ( specialization() == MONK_WINDWALKER )
            {
              active_actions.strength_of_the_black_ox_dmg->execute();
              buff.teachings_of_the_monastery->trigger(
                  as<int>( talent.conduit_of_the_celestials.strength_of_the_black_ox->effectN( 3 ).base_value() ) );
            }
          } );

  buff.unity_within =
      make_buff_fallback( talent.conduit_of_the_celestials.unity_within->ok(), this, "unity_within",
                          find_spell( 443592 ) )
          ->set_expire_callback( [ this ]( buff_t *, double, timespan_t ) {
            buff.jade_sanctuary->trigger();
            if ( specialization() == MONK_MISTWEAVER )
            {
              active_actions.flight_of_the_red_crane_celestial_heal->execute();
              active_actions.strength_of_the_black_ox_absorb->execute();
            }
            if ( specialization() == MONK_WINDWALKER )
            {
              active_actions.strength_of_the_black_ox_dmg->execute();
              if ( const int count =
                       as<int>( talent.conduit_of_the_celestials.strength_of_the_black_ox->effectN( 3 ).base_value() );
                   count > 0 )
                buff.teachings_of_the_monastery->trigger( count );

              active_actions.flight_of_the_red_crane_celestial_damage->execute();
            }
            buff.heart_of_the_jade_serpent_cdr_celestial->trigger();
            active_actions.courage_of_the_white_tiger->execute();
          } );

  buff.aspect_of_harmony.construct_buffs( this );

  buff.balanced_stratagem_magic =
      make_buff_fallback( talent.master_of_harmony.balanced_stratagem->ok(), this, "balanced_stratagem_magic",
                          talent.master_of_harmony.balanced_stratagem_magic );
  buff.balanced_stratagem_physical =
      make_buff_fallback( talent.master_of_harmony.balanced_stratagem->ok(), this, "balanced_stratagem_physical",
                          talent.master_of_harmony.balanced_stratagem_physical );

  // Shado-Pan

  buff.against_all_odds =
      make_buff_fallback( talent.shado_pan.against_all_odds->ok(), this, "against_all_odds", find_spell( 451061 ) )
          ->set_default_value_from_effect( 1 )
          ->set_trigger_spell( talent.shado_pan.against_all_odds );

  buff.flurry_charge =
      make_buff_fallback( talent.shado_pan.flurry_strikes->ok(), this, "flurry_charge", find_spell( 451021 ) )
          ->set_default_value_from_effect( 1 );

  buff.veterans_eye =
      make_buff_fallback( talent.shado_pan.veterans_eye->ok(), this, "veterans_eye", find_spell( 451085 ) )
          ->set_default_value_from_effect( 1 );

  buff.vigilant_watch =
      make_buff_fallback( talent.shado_pan.vigilant_watch->ok(), this, "vigilant_watch", find_spell( 451233 ) )
          ->set_default_value_from_effect( 1 );

  buff.wisdom_of_the_wall_crit = make_buff_fallback( talent.shado_pan.wisdom_of_the_wall->ok(), this,
                                                     "wisdom_of_the_wall_crit", find_spell( 452684 ) )
                                     ->set_default_value_from_effect( 1 );

  buff.wisdom_of_the_wall_dodge = make_buff_fallback( talent.shado_pan.wisdom_of_the_wall->ok(), this,
                                                      "wisdom_of_the_wall_dodge", find_spell( 451242 ) )
                                      ->set_trigger_spell( talent.shado_pan.wisdom_of_the_wall )
                                      ->set_stack_change_callback( [ & ]( buff_t *self, int, int ) {
                                        self->current_value =
                                            self->data().effectN( 3 ).percent() * composite_damage_versatility();
                                      } )
                                      ->set_tick_behavior( buff_tick_behavior::CLIP );

  buff.wisdom_of_the_wall_flurry = make_buff_fallback( talent.shado_pan.wisdom_of_the_wall->ok(), this,
                                                       "wisdom_of_the_wall_flurry", find_spell( 452688 ) )
                                       ->set_trigger_spell( talent.shado_pan.wisdom_of_the_wall )
                                       ->set_default_value_from_effect( 1 );

  buff.wisdom_of_the_wall_mastery = make_buff_fallback( talent.shado_pan.wisdom_of_the_wall->ok(), this,
                                                        "wisdom_of_the_wall_mastery", find_spell( 452685 ) )
                                        ->set_trigger_spell( talent.shado_pan.wisdom_of_the_wall )
                                        ->set_default_value_from_effect( 1 );
  // ------------------------------
  // Movement
  // ------------------------------

  movement.chi_torpedo = new monk_movement_t( this, "chi_torpedo_movement", talent.monk.chi_torpedo );
  movement.chi_torpedo->set_distance( 10 );

  movement.flying_serpent_kick = new monk_movement_t( this, "fsk_movement", baseline.windwalker.flying_serpent_kick );
  movement.flying_serpent_kick->set_distance( 1 );

  movement.melee_squirm = new monk_movement_t( this, "melee_squirm" );
  movement.melee_squirm->set_distance( 1 );

  movement.roll = new monk_movement_t( this, "roll_movement", baseline.monk.roll );
  movement.roll->set_distance( 8 );

  movement.whirling_dragon_punch = new monk_movement_t( this, "wdp_movement", talent.windwalker.whirling_dragon_punch );
  movement.whirling_dragon_punch->set_distance( 1 );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.black_ox_brew_energy     = get_gain( "black_ox_brew_energy" );
  gain.bok_proc                 = get_gain( "blackout_kick_proc" );
  gain.chi_refund               = get_gain( "chi_refund" );
  gain.chi_burst                = get_gain( "chi_burst" );
  gain.crackling_jade_lightning = get_gain( "crackling_jade_lightning" );
  gain.energy_burst             = get_gain( "energy_burst" );
  gain.energizing_elixir_energy = get_gain( "energizing_elixir_energy" );
  gain.energizing_elixir_chi    = get_gain( "energizing_elixir_chi" );
  gain.energy_refund            = get_gain( "energy_refund" );
  gain.focus_of_xuen            = get_gain( "focus_of_xuen" );
  gain.gift_of_the_ox           = get_gain( "gift_of_the_ox" );
  gain.open_palm_strikes        = get_gain( "open_palm_strikes" );
  gain.ordered_elements         = get_gain( "ordered_elements" );
  gain.power_strikes            = get_gain( "power_strikes" );
  gain.tiger_palm               = get_gain( "tiger_palm" );
  gain.touch_of_death_ww        = get_gain( "touch_of_death_ww" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.anvil__stave                   = get_proc( "Anvil & Stave" );
  proc.blackout_combo_tiger_palm      = get_proc( "Blackout Combo - Tiger Palm" );
  proc.blackout_combo_breath_of_fire  = get_proc( "Blackout Combo - Breath of Fire" );
  proc.blackout_combo_keg_smash       = get_proc( "Blackout Combo - Keg Smash" );
  proc.blackout_combo_celestial_brew  = get_proc( "Blackout Combo - Celestial Brew" );
  proc.blackout_combo_purifying_brew  = get_proc( "Blackout Combo - Purifying Brew" );
  proc.blackout_combo_rising_sun_kick = get_proc( "Blackout Combo - Rising Sun Kick (Press the Advantage)" );
  proc.blackout_kick_cdr_oe           = get_proc( "Blackout Kick CDR During SEF" );
  proc.blackout_kick_cdr              = get_proc( "Blackout Kick CDR" );
  proc.bountiful_brew_proc            = get_proc( "Bountiful Brew Trigger" );
  proc.charred_passions               = get_proc( "Charred Passions" );
  proc.chi_surge                      = get_proc( "Chi Surge CDR" );
  proc.counterstrike_tp               = get_proc( "Counterstrike - Tiger Palm" );
  proc.counterstrike_sck              = get_proc( "Counterstrike - Spinning Crane Kick" );
  proc.elusive_footwork_proc          = get_proc( "Elusive Footwork" );
  proc.face_palm                      = get_proc( "Face Palm" );
  proc.glory_of_the_dawn              = get_proc( "Glory of the Dawn" );
  proc.keg_smash_scalding_brew        = get_proc( "Keg Smash - Scalding Brew" );
  proc.quick_sip                      = get_proc( "Quick Sip" );
  proc.rsk_reset_totm                 = get_proc( "Rising Sun Kick TotM Reset" );
  proc.salsalabims_strength           = get_proc( "Sal'salabim Breath of Fire Reset" );
  proc.tranquil_spirit_expel_harm     = get_proc( "Tranquil Spirit - Expel Harm" );
  proc.tranquil_spirit_goto           = get_proc( "Tranquil Spirit - Gift of the Ox" );
  proc.xuens_battlegear_reduction     = get_proc( "Xuen's Battlegear CD Reduction" );
  proc.elusive_brawler_preserved      = get_proc( "Elusive Brawler Stacks Preserved" );
}

// monk_t::init_assessors ===================================================

void monk_t::init_assessors()
{
  base_t::init_assessors();
}

void monk_t::create_proc_callback( const spell_data_t *effect_driver,
                                   bool ( *trigger )( monk_t *player, action_state_t *state ), proc_flag PF_OVERRIDE,
                                   proc_flag2 PF2_OVERRIDE, action_t *proc_action_override )
{
  auto effect = new special_effect_t( this );

  effect->spell_id     = effect_driver->id();
  effect->cooldown_    = effect_driver->internal_cooldown();
  effect->proc_chance_ = effect_driver->proc_chance();
  effect->ppm_         = effect_driver->_rppm;

  sim->print_debug( "initializing driver {} {} {}", effect->name_str, effect->spell_id, effect->cooldown_ );

  if ( proc_action_override == nullptr )
  {
    // If we didn't define a custom action in initialization then
    // search action list for the first trigger we have a valid action for
    for ( const auto &e : effect_driver->effects() )
    {
      for ( auto t : action_list )
        if ( e.trigger()->ok() && t->id == e.trigger()->id() )
          proc_action_override = t;

      if ( proc_action_override != nullptr )
        break;
    }
  }

  if ( proc_action_override != nullptr )
  {
    effect->name_str         = proc_action_override->name_str;
    effect->trigger_str      = proc_action_override->name_str;
    effect->trigger_spell_id = proc_action_override->id;
    effect->action_disabled  = false;
    effect->execute_action   = effect->create_action();

    if ( effect->execute_action == nullptr )
      effect->execute_action = proc_action_override;

    if ( proc_action_override->harmful )
    {
      // Translate harmful proc_flags
      // e.g., the driver for a debuff uses MELEE_ABILITY_TAKEN instead of MELEE_ABILITY

      const std::unordered_map<uint64_t, uint64_t> translation_map = {
          { PF_MELEE_TAKEN, PF_MELEE },           { PF_MELEE_ABILITY_TAKEN, PF_MELEE_ABILITY },
          { PF_RANGED_TAKEN, PF_RANGED },         { PF_RANGED_ABILITY_TAKEN, PF_RANGED_ABILITY },
          { PF_NONE_HEAL_TAKEN, PF_NONE_HEAL },   { PF_NONE_SPELL_TAKEN, PF_NONE_SPELL },
          { PF_MAGIC_HEAL_TAKEN, PF_MAGIC_HEAL }, { PF_MAGIC_SPELL_TAKEN, PF_MAGIC_SPELL },
          { PF_PERIODIC_TAKEN, PF_PERIODIC },     { PF_DAMAGE_TAKEN, PF_ALL_DAMAGE },
      };

      for ( auto t : translation_map )
      {
        if ( effect->proc_flags_ & t.first )
        {
          effect->proc_flags_ = ( effect->proc_flags_ & ~t.first ) | t.second;
        }
      }
    }
  }

  // defer configuration of proc flags in case proc_action_override is used
  effect->proc_flags_  = PF_OVERRIDE ? PF_OVERRIDE : effect_driver->proc_flags();
  effect->proc_flags2_ = PF2_OVERRIDE;

  // We still haven't assigned a name, it is most likely a buff
  // dynamically find buff
  if ( effect->name_str == "" )
  {
    for ( const auto &e : effect_driver->effects() )
    {
      for ( auto t : buff_list )
      {
        if ( e.trigger()->ok() && e.trigger()->id() == t->data().id() )
        {
          effect->name_str    = t->name_str;
          effect->custom_buff = t;
        }
      }

      if ( effect->create_buff() != nullptr )
        break;
    }
  }

  struct monk_effect_callback : dbc_proc_callback_t
  {
    monk_t *p;
    bool ( *callback_trigger )( monk_t *p, action_state_t *state );

    monk_effect_callback( const special_effect_t &effect, monk_t *p,
                          bool ( *trigger )( monk_t *p, action_state_t *state ) )
      : dbc_proc_callback_t( effect.player, effect ), p( p ), callback_trigger( trigger )
    {
    }

    void trigger( action_t *a, action_state_t *state ) override
    {
      if ( callback_trigger == NULL )
      {
        assert( 0 );
        return;
      }

      if ( a->internal_id == 0 && state->action->id == effect.trigger_spell_id )
        return;

      if ( callback_trigger( p, state ) )
      {
        dbc_proc_callback_t::trigger( a, state );

        if ( p->sim->debug )
        {
          // Debug reporting
          auto action =
              range::find_if( p->proc_tracking[ effect.name() ], [ a ]( action_t *it ) { return it->id == a->id; } );

          if ( action == p->proc_tracking[ effect.name() ].end() )
            p->proc_tracking[ effect.name() ].push_back( a );
        }
      }
    }

    void execute( action_t *a, action_state_t *state ) override
    {
      if ( !state->target->is_sleeping() )
      {
        // Dynamically find and execute proc tracking
        auto effect_proc = p->find_proc( effect.trigger()->_name );
        if ( effect_proc )
          effect_proc->occur();
      }

      dbc_proc_callback_t::execute( a, state );
    }
  };

  special_effects.push_back( effect );  // Garbage collection

  new monk_effect_callback( *effect, this, trigger );
}

void monk_t::create_proc_callback( const spell_data_t *effect_driver,
                                   bool ( *trigger )( monk_t *player, action_state_t *state ),
                                   action_t *proc_action_override )
{
  create_proc_callback( effect_driver, trigger, static_cast<proc_flag>( 0 ), static_cast<proc_flag2>( 0 ),
                        proc_action_override );
}

void monk_t::create_proc_callback( const spell_data_t *effect_driver,
                                   bool ( *trigger )( monk_t *player, action_state_t *state ), proc_flag PF_OVERRIDE,
                                   action_t *proc_action_override )
{
  create_proc_callback( effect_driver, trigger, PF_OVERRIDE, static_cast<proc_flag2>( 0 ), proc_action_override );
}

void monk_t::create_proc_callback( const spell_data_t *effect_driver,
                                   bool ( *trigger )( monk_t *player, action_state_t *state ), proc_flag2 PF2_OVERRIDE,
                                   action_t *proc_action_override )
{
  create_proc_callback( effect_driver, trigger, static_cast<proc_flag>( 0 ), PF2_OVERRIDE, proc_action_override );
}

// monk_t::init_special_effects ===========================================

void monk_t::init_special_effects()
{
  // ======================================
  // Exploding Keg Talent
  // ======================================

  if ( talent.brewmaster.exploding_keg.ok() )
    create_proc_callback( talent.brewmaster.exploding_keg.spell(), []( monk_t *p, action_state_t *state ) {
      // Exploding keg damage is only triggered when the player buff is up, regardless if the enemy has the debuff
      if ( !p->buff.exploding_keg->up() )
        return false;
      p->active_actions.exploding_keg->set_target( state->target );
      return true;
    } );

  // ======================================
  // Flurry of Xuen ( Windwalker Talent )
  // ======================================

  if ( talent.windwalker.flurry_of_xuen.ok() )
  {
    create_proc_callback( talent.windwalker.flurry_of_xuen.spell(), []( monk_t *p, action_state_t *state ) {
      if ( state->action->id == p->active_actions.flurry_of_xuen->id ||
           state->action->id == p->active_actions.empowered_tiger_lightning->id )
        return false;
      return true;
    } );

    callbacks.register_callback_execute_function(
        talent.windwalker.flurry_of_xuen.spell()->id(),
        [ this ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          active_actions.flurry_of_xuen->set_target( state->target );
          buff.flurry_of_xuen->trigger();
        } );
  }

  // ======================================
  // Darting Hurricane ( Windwalker Talent )
  // ======================================

  if ( talent.windwalker.darting_hurricane.ok() )
    create_proc_callback( talent.windwalker.darting_hurricane.spell(), []( monk_t *p, action_state_t *state ) {
      if ( state->action->id == p->talent.windwalker.strike_of_the_windlord->id() ||
           state->action->id == p->talent.windwalker.strike_of_the_windlord->effectN( 3 ).trigger_spell_id() ||
           state->action->id == p->talent.windwalker.strike_of_the_windlord->effectN( 4 ).trigger_spell_id() ||
           state->action->id == p->passives.dual_threat_kick->id() )
        return false;
      return true;
    } );

  // ======================================
  // Veteran's Eye ( Shado-pan Talent )
  // ======================================

  if ( talent.shado_pan.veterans_eye->ok() )
    create_proc_callback(
        talent.shado_pan.veterans_eye.spell(),
        []( monk_t *p, action_state_t *state ) {
          auto td = p->get_target_data( state->target );
          if ( td )
          {
            td->debuff.veterans_eye->trigger();
            if ( td->debuff.veterans_eye->at_max_stacks() )
            {
              p->buff.veterans_eye->trigger();
              td->debuff.veterans_eye->reset();
            }
          }
          return true;
        },
        PF2_ALL_HIT );

  // ======================================
  // Chi Burst ( Monk Talent )
  // ======================================
  if ( talent.monk.chi_burst->ok() && specialization() == MONK_WINDWALKER )
    create_proc_callback( talent.monk.chi_burst.spell(), []( monk_t *, action_state_t * ) { return true; } );

  // ======================================
  // Spirit of the Ox ( Brewmaster Talent )
  // ======================================
  if ( talent.brewmaster.spirit_of_the_ox->ok() )
  {
    create_proc_callback( talent.brewmaster.spirit_of_the_ox.spell(), []( monk_t *player, action_state_t *state ) {
      if ( state->action->id != player->talent.monk.rising_sun_kick->effectN( 1 ).trigger()->id() ||
           state->action->id != player->baseline.brewmaster.blackout_kick->id() )
        return false;
      return true;
    } );
    callbacks.register_callback_execute_function(
        talent.brewmaster.spirit_of_the_ox.spell()->id(),
        [ this ]( const dbc_proc_callback_t *, action_t *, action_state_t * ) {
          buff.gift_of_the_ox->spawn_orb( 1 );
        } );
  }

  // ======================================
  // Master of Harmony
  // ======================================
  if ( talent.master_of_harmony.aspect_of_harmony->ok() )
  {
    create_proc_callback( talent.master_of_harmony.aspect_of_harmony_driver,
                          []( monk_t *, action_state_t * ) { return true; } );
    callbacks.register_callback_execute_function(
        talent.master_of_harmony.aspect_of_harmony_driver->id(),
        [ this ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          buff.aspect_of_harmony.trigger( state );
        } );
  }

  if ( talent.master_of_harmony.balanced_stratagem->ok() )
  {
    create_proc_callback( talent.master_of_harmony.balanced_stratagem, []( monk_t *, action_state_t *state ) {
      if ( state->action->school == SCHOOL_NONE )
        return false;
      return true;
    } );
    callbacks.register_callback_execute_function(
        talent.master_of_harmony.balanced_stratagem->id(),
        [ this ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          if ( state->action->school == SCHOOL_PHYSICAL )
            buff.balanced_stratagem_magic->trigger();
          if ( state->action->school != SCHOOL_PHYSICAL )
            buff.balanced_stratagem_physical->trigger();
        } );
  }

  // ======================================
  // Courage of the White Tiger
  // ======================================
  if ( talent.conduit_of_the_celestials.courage_of_the_white_tiger->ok() )
  {
    create_proc_callback(
        talent.conduit_of_the_celestials.courage_of_the_white_tiger.spell(), []( monk_t *p, action_state_t *state ) {
          if ( ( p->specialization() == MONK_MISTWEAVER && state->action->id == p->baseline.monk.vivify->id() ) ||
               state->action->id == p->baseline.monk.tiger_palm->id() )
            return true;

          return false;
        } );

    callbacks.register_callback_execute_function(
        talent.conduit_of_the_celestials.courage_of_the_white_tiger.spell()->id(),
        [ this ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          active_actions.courage_of_the_white_tiger->execute_on_target( state->target );
        } );
  }

  // ======================================
  // Flight of the Red Crane
  // ======================================
  if ( talent.conduit_of_the_celestials.flight_of_the_red_crane->ok() )
  {
    create_proc_callback(
        talent.conduit_of_the_celestials.flight_of_the_red_crane.spell(), []( monk_t *p, action_state_t *state ) {
          if ( ( p->specialization() == MONK_MISTWEAVER &&
                 state->action->id == p->talent.mistweaver.refreshing_jade_wind_tick->id() ) ||
               ( p->specialization() == MONK_WINDWALKER &&
                 state->action->id == p->passives.rushing_jade_wind_tick->id() ) ||
               state->action->id == p->baseline.monk.spinning_crane_kick->effectN( 1 ).trigger()->id() )
            return true;

          return false;
        } );

    callbacks.register_callback_execute_function(
        talent.conduit_of_the_celestials.flight_of_the_red_crane.spell()->id(),
        [ this ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          if ( specialization() == MONK_MISTWEAVER )
          {
            active_actions.flight_of_the_red_crane_heal->execute_on_target( state->target );
          }
          else
          {
            active_actions.flight_of_the_red_crane_damage->execute_on_target( state->target );
          }
          buff.inner_compass_crane_stance->trigger();
        } );
  }

  // ======================================

  base_t::init_special_effects();
}

// monk_t::init_special_effect ============================================

void monk_t::init_special_effect( special_effect_t & /*effect*/ )
{
  // Monk module has custom triggering logic (defined above) so override the initial
  // proc flags so we get wider trigger attempts than the core implementation. The
  // overridden proc condition above will take care of filtering out actions that are
  // not allowed to proc it.

  /*switch ( effect.driver()->id() )
  {
    default:
      break;
  }*/
}

void monk_t::init_finished()
{
  base_t::init_finished();
  parse_player_effects();
}

// monk_t::reset ============================================================

void monk_t::reset()
{
  base_t::reset();

  squirm_timer          = 0;
  spiritual_focus_count = 0;
  combo_strike_actions.clear();

  // ===================================
  // Proc Tracking
  // ===================================

  if ( sim->debug )
  {
    auto stream = sim->out_debug.raw().get_stream();
    bool first  = true;

    for ( auto &[ name, list ] : proc_tracking )
    {
      if ( list.size() > 0 )
      {
        if ( first )
        {
          *stream << "\n Monk Proc Tracking ... ( spellid: name )" << '\n' << '\n';
          first = false;
        }

        *stream << name << " procced from: " << '\n';
        for ( auto a : list )
          *stream << " - " << a->id << " : " << a->name_str << '\n';

        list.clear();
      }
    }
  }
}

// monk_t::create_storm_earth_and_fire_target_list ====================================

std::vector<player_t *> monk_t::create_storm_earth_and_fire_target_list() const
{
  // Make a copy of the non sleeping target list
  std::vector<player_t *> l = sim->target_non_sleeping_list.data();

  // Sort the list by selecting non-cyclone striked targets first, followed by ascending order of
  // the debuff remaining duration
  range::sort( l, [ this ]( const player_t *l, const player_t *r ) {
    auto td_left  = find_target_data( l );
    auto td_right = find_target_data( r );
    bool lcs      = td_left ? td_left->debuff.mark_of_the_crane->check() : false;
    bool rcs      = td_right ? td_right->debuff.mark_of_the_crane->check() : false;
    // Neither has cyclone strike
    if ( !lcs && !rcs )
    {
      return false;
    }
    // Left side does not have cyclone strike, right side does
    else if ( !lcs && rcs )
    {
      return true;
    }
    // Left side has cyclone strike, right side does not
    else if ( lcs && !rcs )
    {
      return false;
    }

    // Both have cyclone strike, order by remaining duration, use actor index as a tiebreaker
    timespan_t lv = td_left ? td_left->debuff.mark_of_the_crane->remains() : 0_ms;
    timespan_t rv = td_right ? td_right->debuff.mark_of_the_crane->remains() : 0_ms;
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv < rv;
  } );

  if ( sim->debug )
  {
    sim->out_debug.print( "{} storm_earth_and_fire target list, n_targets={}", *this, l.size() );
    range::for_each( l, [ this ]( player_t *t ) {
      auto td = find_target_data( t );
      sim->out_debug.print( "{} cs={}", *t, td ? td->debuff.mark_of_the_crane->remains().total_seconds() : 0.0 );
    } );
  }

  return l;
}

// monk_t::affected_by_sef ==================================================

bool monk_t::affected_by_sef( spell_data_t data ) const
{
  auto filter = [ & ]( std::vector<size_t> indices ) {
    return std::any_of( indices.begin(), indices.end(), [ & ]( size_t index ) {
      return data.affected_by( talent.windwalker.storm_earth_and_fire->effectN( index ) );
    } );
  };
  // Chi Explosion must be added manually.
  return filter( { 1, 2, 7, 8 } ) || data.id() == 337342;
}

// monk_t::retarget_storm_earth_and_fire ====================================

void monk_t::retarget_storm_earth_and_fire( pet_t *pet, std::vector<player_t *> &targets ) const
{
  // Clones attack your target if there are no other targets available
  if ( targets.size() == 1 )
    pet->target = pet->owner->target;
  else
  {
    // Clones will now only re-target when you use an ability that applies Mark of the Crane, and their current target
    // already has Mark of the Crane. https://us.battle.net/forums/en/wow/topic/20752377961?page=29#post-573
    auto td = find_target_data( pet->target );

    if ( !td || !td->debuff.mark_of_the_crane->check() )
      return;

    for ( auto it = targets.begin(); it != targets.end(); ++it )
    {
      player_t *candidate_target = *it;

      // Candidate target is a valid target
      if ( *it != pet->owner->target && *it != pet->target && !candidate_target->debuffs.invulnerable->check() )
      {
        pet->target = *it;
        // This target has been chosen, so remove from the list (so that the second pet can choose something else)
        targets.erase( it );
        break;
      }
    }
  }

  range::for_each( pet->action_list,
                   [ pet ]( action_t *a ) { a->acquire_target( retarget_source::SELF_ARISE, nullptr, pet->target ); } );
}

// monk_t::composite_attack_power_multiplier() ==========================

double monk_t::composite_attack_power_multiplier() const
{
  double ap = base_t::composite_attack_power_multiplier();

  ap *= 1.0 + cache.mastery() * baseline.brewmaster.mastery->effectN( 2 ).mastery_value();

  return ap;
}

// monk_t::composite_dodge ==============================================

double monk_t::composite_dodge() const
{
  double d = base_t::composite_dodge();

  d += talent.monk.dance_of_the_wind->effectN( 1 ).percent();

  if ( specialization() == MONK_BREWMASTER )
  {
    d += buff.elusive_brawler->current_stack * cache.mastery_value();
  }

  if ( buff.fortifying_brew->check() )
    d += talent.monk.ironshell_brew->effectN( 1 ).percent();

  if ( buff.wisdom_of_the_wall_dodge->check() )
    d += buff.wisdom_of_the_wall_dodge->check_value();

  return d;
}

// monk_t::temporary_movement_modifier =====================================

double monk_t::non_stacking_movement_modifier() const
{
  double ms = base_t::non_stacking_movement_modifier();

  ms = std::max( buff.chi_torpedo->check_stack_value(), ms );

  ms = std::max( buff.flying_serpent_kick_movement->check_value(), ms );

  return ms;
}

// monk_t::composite_player_target_armor ==============================

double monk_t::composite_player_target_armor( player_t *target ) const
{
  double armor = player_t::composite_player_target_armor( target );

  armor *= ( 1.0f - talent.shado_pan.martial_precision->effectN( 1 ).percent() );

  return armor;
}

// monk_t::invalidate_cache ==============================================

void monk_t::invalidate_cache( cache_e c )
{
  base_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_ATTACK_POWER:
    case CACHE_AGILITY:
      if ( specialization() == MONK_BREWMASTER || specialization() == MONK_WINDWALKER )
        base_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_SPELL_POWER:
    case CACHE_INTELLECT:
      if ( specialization() == MONK_MISTWEAVER )
        base_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_BONUS_ARMOR:
      break;
    case CACHE_MASTERY:
      if ( specialization() == MONK_WINDWALKER )
        base_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      else if ( specialization() == MONK_BREWMASTER )
      {
        base_t::invalidate_cache( CACHE_ATTACK_POWER );
        base_t::invalidate_cache( CACHE_SPELL_POWER );
        base_t::invalidate_cache( CACHE_DODGE );
      }
      break;
    default:
      break;
  }
}

// monk_t::create_options ===================================================

void monk_t::create_options()
{
  base_t::create_options();

  add_option( opt_int( "monk.initial_chi", user_options.initial_chi, 0, 6 ) );
  add_option( opt_int( "monk.chi_burst_healing_targets", user_options.chi_burst_healing_targets, 0, 30 ) );
  add_option( opt_int( "monk.motc_override", user_options.motc_override, 0, 5 ) );
  add_option( opt_float( "monk.squirm_frequency", user_options.squirm_frequency, 0, 30 ) );
}

// monk_t::copy_from =========================================================

void monk_t::copy_from( player_t *source )
{
  base_t::copy_from( source );

  auto *source_p = debug_cast<monk_t *>( source );

  user_options = source_p->user_options;
}

// monk_t::primary_resource =================================================

resource_e monk_t::primary_resource() const
{
  if ( specialization() == MONK_MISTWEAVER )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}

// monk_t::primary_role =====================================================

role_e monk_t::primary_role() const
{
  // First, check for the user-specified role
  switch ( base_t::primary_role() )
  {
    case ROLE_TANK:
    case ROLE_ATTACK:
    case ROLE_HEAL:
      return base_t::primary_role();
      break;
    default:
      break;
  }

  // Else, fall back to spec
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      return ROLE_TANK;
      break;
    default:
      return ROLE_ATTACK;
      break;
  }
}

// monk_t::convert_hybrid_stat ==============================================

stat_e monk_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    case STAT_STR_AGI_INT:
      switch ( specialization() )
      {
        case MONK_MISTWEAVER:
          return STAT_INTELLECT;
        case MONK_BREWMASTER:
        case MONK_WINDWALKER:
          return STAT_AGILITY;
        default:
          return STAT_NONE;
      }
    case STAT_AGI_INT:
      if ( specialization() == MONK_MISTWEAVER )
        return STAT_INTELLECT;
      else
        return STAT_AGILITY;
    case STAT_STR_AGI:
      return STAT_AGILITY;
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_SPIRIT:
      if ( specialization() == MONK_MISTWEAVER )
        return s;
      else
        return STAT_NONE;
    case STAT_BONUS_ARMOR:
      if ( specialization() == MONK_BREWMASTER )
        return s;
      else
        return STAT_NONE;
    default:
      return s;
  }
}

// monk_t::combat_begin ====================================================

void monk_t::combat_begin()
{
  // Trigger Ferociousness precombat
  if ( talent.windwalker.ferociousness->ok() )
    buff.ferociousness->trigger();

  // Mark of the Crane Override
  if ( user_options.motc_override > 0 )
    buff.cyclone_strikes->trigger();

  base_t::combat_begin();

  if ( talent.monk.windwalking->ok() )
  {
    if ( sim->distance_targeting_enabled )
    {
      buff.windwalking_driver->trigger();
    }
    else
    {
      buffs.windwalking_movement_aura->trigger( 1, buffs.windwalking_movement_aura->data().effectN( 1 ).percent(), 1,
                                                timespan_t::zero() );
    }
  }

  if ( talent.monk.chi_wave->ok() )
  {
    // Player starts combat with buff
    buff.chi_wave->trigger();
    // ... and then regains the buff in time intervals while in combat
    make_repeating_event( sim, talent.monk.chi_wave->effectN( 1 ).period(), [ this ]() { buff.chi_wave->trigger(); } );
  }

  if ( specialization() == MONK_WINDWALKER )
  {
    if ( user_options.initial_chi > 0 )
    {
      resources.current[ RESOURCE_CHI ] =
          clamp( as<double>( user_options.initial_chi + resources.current[ RESOURCE_CHI ] ), 0.0,
                 resources.max[ RESOURCE_CHI ] );
      sim->print_debug( "Combat starting chi has been set to {}", resources.current[ RESOURCE_CHI ] );
    }
    else
    {
      resources.current[ RESOURCE_CHI ] = talent.windwalker.combat_wisdom->effectN( 1 ).base_value();
    }

    if ( talent.windwalker.combat_wisdom->ok() )
    {
      // Player starts combat with buff
      buff.combat_wisdom->trigger();
      // ... and then regains the buff in time intervals while in combat
      bool trigger( double index );
      make_repeating_event( sim, talent.windwalker.combat_wisdom->effectN( 2 ).period(),
                            [ this ]() { buff.combat_wisdom->trigger(); } );
    }
  }

  // if ( specialization() == MONK_BREWMASTER )
  //   stagger->current->debuff->trigger();

  // Melee Squirm
  // Periodic 1 YD movement to simulate combat movement
  make_repeating_event( sim, timespan_t::from_seconds( 1 ), [ this ]() {
    squirm_timer += 1;

    // Do not interrupt a cast
    if ( !( executing && !executing->usable_moving() ) && !( queueing && !queueing->usable_moving() ) &&
         !( channeling && !channeling->usable_moving() ) )
    {
      if ( user_options.squirm_frequency > 0 && squirm_timer >= user_options.squirm_frequency )
      {
        movement.melee_squirm->trigger();
        squirm_timer = 0;
      }
    }
  } );
}

// monk_t::assess_damage ====================================================

void monk_t::assess_damage( school_e school, result_amount_type dtype, action_state_t *s )
{
  if ( specialization() == MONK_BREWMASTER )
  {
    if ( s->result == RESULT_DODGE )
    {
      // In order to trigger the expire before the hit but not actually remove the buff until AFTER the hit
      // We are setting a 1 millisecond delay on the expire.

      if ( rng().roll( 1.0 - talent.brewmaster.one_with_the_wind->effectN( 1 ).percent() ) )
        buff.elusive_brawler->expire();
      else
        proc.elusive_brawler_preserved->occur();

      // Saved as 5/10 base values but need it as 0.5 and 1 base values
      if ( talent.brewmaster.anvil__stave->ok() && cooldown.anvil__stave->up() )
      {
        cooldown.anvil__stave->start( talent.brewmaster.anvil__stave->internal_cooldown() );
        proc.anvil__stave->occur();
        baseline.brewmaster.brews.adjust(
            timespan_t::from_seconds( talent.brewmaster.anvil__stave->effectN( 1 ).base_value() / 10 ) );
      }

      buff.counterstrike->trigger();
    }
    if ( s->result == RESULT_MISS )
      buff.counterstrike->trigger();
  }

  // trigger the mastery if the player gets hit by a physical attack; but not from stagger
  if ( action_t::result_is_hit( s->result ) && school == SCHOOL_PHYSICAL &&
       s->action->id != baseline.brewmaster.stagger_self_damage->id() )
    buff.elusive_brawler->trigger();

  base_t::assess_damage( school, dtype, s );
}

// monk_t::target_mitigation ====================================================

void monk_t::target_mitigation( school_e school, result_amount_type dt, action_state_t *s )
{
  monk_td_t *target_td = get_target_data( s->action->player );

  // Dampen Harm // Reduces hits by 20 - 50% based on the size of the hit
  if ( buff.dampen_harm->up() )
  {
    double dampen_max_percent = buff.dampen_harm->data().effectN( 3 ).percent();
    if ( s->result_amount >= max_health() )
      s->result_amount *= 1 - dampen_max_percent;
    else
    {
      double dampen_min_percent = buff.dampen_harm->data().effectN( 2 ).percent();
      s->result_amount *= 1 - ( dampen_min_percent +
                                ( ( dampen_max_percent - dampen_min_percent ) * ( s->result_amount / max_health() ) ) );
    }
  }

  // Diffuse Magic
  if ( school != SCHOOL_PHYSICAL )
    s->result_amount *= 1.0 + buff.diffuse_magic->value();  // Stored as -60%

  // If Breath of Fire is ticking on the source target, the player receives 5% less damage
  if ( target_td->dot.breath_of_fire->is_ticking() )
    s->result_amount *=
        1.0 + active_actions.breath_of_fire->data().effectN( 2 ).percent() + buff.celestial_flames->value() / 100.0;

  // Damage Reduction Cooldowns
  if ( buff.fortifying_brew->up() )
    s->result_amount *= ( 1.0 + talent.monk.fortifying_brew->effectN( 2 ).percent() );  // Saved as -20%

  // Touch of Karma Absorbtion
  if ( buff.touch_of_karma->up() )
  {
    double percent_HP = baseline.windwalker.touch_of_karma->effectN( 3 ).percent() * max_health();
    if ( ( buff.touch_of_karma->value() + s->result_amount ) >= percent_HP )
    {
      double difference = percent_HP - buff.touch_of_karma->value();
      buff.touch_of_karma->current_value += difference;
      s->result_amount -= difference;
      buff.touch_of_karma->expire();
    }
    else
    {
      buff.touch_of_karma->current_value += s->result_amount;
      s->result_amount = 0;
    }
  }

  // Gift of the Ox Trigger Calculations ===========================================================

  // Gift of the Ox is no longer a random chance, under the hood. When you are hit, it increments a counter by
  // (DamageTakenBeforeAbsorbsOrStagger / MaxHealth). It now drops an orb whenever that reaches 1.0, and decrements it
  // by 1.0. The tooltip still says ‘chance’, to keep it understandable.
  if ( buff.gift_of_the_ox && s->action->id != baseline.brewmaster.stagger_self_damage->id() )
    buff.gift_of_the_ox->trigger_from_damage( s->result_amount );

  base_t::target_mitigation( school, dt, s );
}

// monk_t::assess_heal ===================================================

void monk_t::assess_heal( school_e school, result_amount_type dmg_type, action_state_t *s )
{
  base_t::assess_heal( school, dmg_type, s );

  if ( specialization() == MONK_BREWMASTER )
    trigger_celestial_fortune( s );
}

// =========================================================================
// Monk APL
// =========================================================================

// monk_t::default_flask ===================================================
std::string monk_t::default_flask() const
{
  return monk_apl::flask( this );
}

// monk_t::default_potion ==================================================
std::string monk_t::default_potion() const
{
  return monk_apl::potion( this );
}

// monk_t::default_food ====================================================
std::string monk_t::default_food() const
{
  return monk_apl::food( this );
}

// monk_t::default_rune ====================================================
std::string monk_t::default_rune() const
{
  return monk_apl::rune( this );
}

// monk_t::temporary_enchant ===============================================
std::string monk_t::default_temporary_enchant() const
{
  return monk_apl::temporary_enchant( this );
}

// monk_t::init_action_list =====================================================
void monk_t::init_action_list()
{
  // Mistweaver isn't supported atm
  if ( !sim->allow_experimental_specializations && specialization() == MONK_MISTWEAVER && role != ROLE_ATTACK )
  {
    if ( !quiet )
      sim->error( "Monk mistweaver healing for {} is not currently supported.", *this );

    quiet = true;
    return;
  }
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->error( "{} has no weapon equipped at the Main-Hand slot.", *this );
    quiet = true;
    return;
  }
  if ( main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim->error( "{} has a 1-Hand weapon equipped in the Off-Hand while a 2-Hand weapon is equipped in the Main-Hand.",
                  *this );
    quiet = true;
    return;
  }
  if ( !action_list_str.empty() )
  {
    base_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  // Combat
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      monk_apl::brewmaster( this );
      break;
    case MONK_WINDWALKER:
      monk_apl::windwalker( this );
      break;
    case MONK_MISTWEAVER:
      monk_apl::mistweaver( this );
      break;
    default:
      monk_apl::no_spec( this );
      break;
  }
  use_default_action_list = true;

  base_t::init_action_list();
}

// monk_t::create_actions =======================================================
void monk_t::create_actions()
{
  base_t::create_actions();

  buff.aspect_of_harmony.construct_actions( this );
}

void monk_t::trigger_empowered_tiger_lightning( action_state_t *s )
{
  /*
   * From discovery by the Peak of Serenity Discord server, ETL has two remaining bugs
   * 1.) If both tigers are up the damage cache is a shared pool for both tigers and resets to 0 when either spawn
   * 2.) SEF Actions contribute 0 to the FoX pool
   */

  if ( specialization() != MONK_WINDWALKER || !baseline.windwalker.empowered_tiger_lightning->ok() )
    return;

  if ( s->result_amount <= 0 )
    return;

  // Proc cannot proc from itself
  if ( s->action->id == passives.empowered_tiger_lightning->id() )
    return;

  auto td = get_target_data( s->target );

  if ( !td )
    return;

  // These abilities are always blacklisted by both tigers
  auto etl_blacklist = {
      122470,  // Touch of Karma
      451585,  // Gale Force
      450615,  // Flurry Strikes
      115129,  // Expel Harm
      389541,  // White Tiger State
  };

  for ( unsigned int id : etl_blacklist )
    if ( s->action->id == id )
      return;

  // 1 = Xuen, 2 = FoX, 3 = Both
  auto mode = ( 2 * buff.fury_of_xuen->check() ) + buff.invoke_xuen->check();

  if ( mode == 0 )
    return;

  double xuen_contribution = mode != 2 ? s->result_amount : 0;
  double fox_contribution  = mode > 1 ? s->result_amount : 0;

  // No damage done by SEF spirits contributes to FoX ETL
  if ( s->action->player->name_str.find( "_spirit" ) != std::string::npos )
    fox_contribution = 0;

  // Return value
  double amount = xuen_contribution + fox_contribution;

  if ( amount > 0 )
  {
    auto cache = mode == 2 ? td->debuff.fury_of_xuen_empowered_tiger_lightning : td->debuff.empowered_tiger_lightning;
    auto duration = std::max( buff.invoke_xuen->remains(), buff.fury_of_xuen->remains() );

    if ( cache->check() )
      cache->current_value += amount;
    else
      cache->trigger( -1, amount, -1, duration );
  }
}

// monk_t::create_expression ==================================================

std::unique_ptr<expr_t> monk_t::create_expression( util::string_view name_str )
{
  auto splits = util::string_split<util::string_view>( name_str, "." );
  if ( splits.size() == 2 && splits[ 0 ] == "spinning_crane_kick" )
  {
    if ( splits[ 1 ] == "count" )
      return make_fn_expr( name_str, [ this ] { return buff.cyclone_strikes->current_stack; } );
    else if ( splits[ 1 ] == "max" )
      return make_fn_expr( name_str, [ this ] { return mark_of_the_crane_max(); } );
  }

  return base_t::create_expression( name_str );
}

// monk_t::monk_report =================================================

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class monk_report_t : public player_report_extension_t
{
public:
  monk_report_t( monk_t &player ) : p( player )
  {
  }

  struct monk_bug
  {
    std::string desc;
    std::string date;
    bool match;
  };

  auto_dispose<std::vector<monk_bug *>> issues;

  void monk_bugreport( report::sc_html_stream &os )
  {
    // Description: Self-explanatory
    // Date: Self-explanatory
    // Match: True if sim matches in-game behavior
    auto ReportIssue = [ this ]( std::string_view desc, std::string_view date, bool match = false ) {
      monk_bug *new_issue = new monk_bug;
      new_issue->desc     = desc;
      new_issue->date     = date;
      new_issue->match    = match;
      issues.push_back( new_issue );
    };

    // Add bugs / issues with sims here:
    ReportIssue( "The ETL cache for both tigers resets to 0 when either spawn", "2023-08-03", true );
    ReportIssue( "SEF does not contribute to Fury of Xuen's ETL cache", "2024-08-01", true );
    ReportIssue( "Blackout Combo buffs both the initial and periodic effect of Breath of Fire", "2023-03-08", true );
    ReportIssue( "Rushing Jade Wind is expiring mastery and Hit Combo on each of the SotWL hit events", "2024-07-20",
                 true );
    ReportIssue( "Flurry of Xuen does additional damage during Storm, Earth, and Fire", "2024-08-01", true );
    ReportIssue( "Memory of the Monastery stacks are overwritten each time the buff is applied", "2024-08-01", true );
    ReportIssue( "Chi Burst consumes both stacks of the buff on use", "2024-08-09", true );

    // =================================================

    os << "<div class=\"player-section\">\n";
    os << "<h2 class=\"toggle\">Known Bugs and Issues</h2>\n";
    os << "<div class=\"toggle-content hide\">\n";

    for ( auto issue : issues )
    {
      if ( issue->desc.empty() )
        continue;

      os << "<h3>" << issue->desc << "</h3>\n";

      os << "<table class=\"sc even\">\n"
         << "<thead>\n"
         << "<tr>\n"
         << "<th class=\"left\">Effective Date</th>\n"
         << "<th class=\"left\">Sim Matches Game Behavior</th>\n"
         << "</tr>\n"
         << "</thead>\n";

      os << "<tr>\n"
         << "<td class=\"left\"><strong>" << issue->date << "</strong></td>\n"
         << "<td class=\"left\" colspan=\"5\"><strong>" << ( issue->match ? "YES" : "NO" ) << "</strong></td>\n"
         << "</tr>\n";

      os << "</table>\n";
    }

    os << "</table>\n";
    os << "</div>\n";
    os << "</div>\n";
  }

  void html_customsection( report::sc_html_stream &os ) override
  {
    monk_bugreport( os );
    os << "<div class=\"player-section\">\n";
    p.parsed_effects_html( os );
    os << "</div>\n";
  }

private:
  monk_t &p;
};

struct monk_module_t : public module_t
{
  monk_module_t() : module_t( MONK )
  {
  }

  player_t *create_player( sim_t *sim, util::string_view name, race_e r ) const override
  {
    auto p              = new monk_t( sim, name, r );
    p->report_extension = std::make_unique<monk_report_t>( *p );
    return p;
  }
  bool valid() const override
  {
    return true;
  }

  void static_init() const override
  {
    items::init();
  }

  void register_hotfixes() const override
  {
    /*
          hotfix::register_effect( "Monk", "2023-11-14", "Manually apply BrM-T31-2p Buff", 1098484)
            .field( "base_value" )
            .operation( hotfix::HOTFIX_SET )
            .modifier( 40 )
            .verification_value( 20 );
          hotfix::register_effect( "Monk", "2023-11-14", "Manually apply BrM-T31-4p Buff", 1098485)
            .field( "base_value" )
            .operation( hotfix::HOTFIX_SET )
            .modifier( 15 )
            .verification_value( 10 );
    */
  }

  void init( player_t *p ) const override
  {
    p->buffs.windwalking_movement_aura = make_buff( p, "windwalking_movement_aura", p->find_spell( 365080 ) )
                                             ->add_invalidate( CACHE_RUN_SPEED )
                                             ->set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS );
  }
  void combat_begin( sim_t * ) const override
  {
  }
  void combat_end( sim_t * ) const override
  {
  }
};
}  // end namespace monk

const module_t *module_t::monk()
{
  static monk::monk_module_t m;
  return &m;
}
