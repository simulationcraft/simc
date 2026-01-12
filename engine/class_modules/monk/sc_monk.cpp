
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

MISTWEAVER:
Manatee
                  _.---.._
    _        _.-'         ''-.
  .'  '-,_.-'                 '''.
 (       _                     o  :
  '._ .-'  '-._         \  \-  ---]
                '-.___.-')  )..-'
                         (_/lame

WINDWALKER:
- See about removing tick part of Crackling Tiger Lightning
*/
#include "sc_monk.hpp"

#include "action/action_callback.hpp"
#include "action/parse_effects.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "sc_enums.hpp"

#include <deque>

#include "simulationcraft.hpp"

namespace monk
{
namespace actions
{
template <class Base>
template <typename... Args>
monk_action_t<Base>::monk_action_t( Args &&...args )
  : parse_action_effects_t<Base>( std::forward<Args>( args )... ),
    ww_mastery( false ),
    may_combo_strike( false ),
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
   * Temporary action-specific effects that apply to more than one action.
   * If so, the aura gets parsed here with `parse_effects`.
   */

  // Monk

  // Brewmaster
  parse_effects( p()->buff.blackout_combo );
  parse_effects(
      p()->buff.counterstrike,
      affect_list_t( 1 ).add_spell( p()->baseline.brewmaster.spinning_crane_kick->effectN( 1 ).trigger()->id() ),
      CONSUME_BUFF );

  // Windwalker
  if ( const auto &effect = p()->baseline.windwalker.mastery->effectN( 1 ); effect.ok() )
  {
    auto mastery_parse_entry = [ & ]( std::vector<player_effect_t> &effect_list ) {
      const std::array<unsigned, 3> rsk_ids = { p()->talent.monk.rising_sun_kick->effectN( 1 ).trigger()->id(),
                                                p()->talent.windwalker.rushing_wind_kick_damage->id(),
                                                p()->talent.windwalker.glory_of_the_dawn_damage->id() };

      double value = effect.mastery_value();
      if ( range::contains( rsk_ids, base_t::id ) && p()->talent.windwalker.sunfire_spiral->ok() )
        value *= 1.0 + p()->talent.windwalker.sunfire_spiral->effectN( 1 ).percent();
      add_parse_entry( effect_list )
          .set_buff( p()->buff.combo_strikes )
          .set_func( [ & ] { return ww_mastery; } )
          .set_value( value )
          .set_mastery( true )
          .set_eff( &effect );
    };
    mastery_parse_entry( base_t::da_multiplier_effects );
    mastery_parse_entry( base_t::ta_multiplier_effects );
  }
  parse_effects( p()->buff.hit_combo );
  parse_effects( p()->buff.press_the_advantage );
  parse_effects( p()->buff.combo_breaker, affect_list_t( 1, 2, 3 ).remove_spell(
                                              p()->talent.windwalker.teachings_of_the_monastery_blackout_kick->id() ) );
  parse_effects( p()->buff.zenith );
  parse_effects( p()->buff.invoke_xuen, effect_mask_t( false ).enable( 3 ), "Ferociousness" );

  // Conduit of the Celestials
  parse_effects( p()->buff.heart_of_the_jade_serpent,
                 [ & ] { return !p()->buff.heart_of_the_jade_serpent_yulons_avatar->check(); } );
  parse_effects( p()->buff.heart_of_the_jade_serpent_yulons_avatar );
  parse_effects( p()->buff.heart_of_the_jade_serpent_unity_within );
  parse_effects( p()->buff.jade_sanctuary );
  parse_effects( p()->buff.strength_of_the_black_ox );
  if ( p()->talent.conduit_of_the_celestials.restore_balance->ok() )
    parse_effects( p()->buff.invoke_xuen, effect_mask_t( false ).enable( 4, 5 ), "Restore Balance" );

  // Master of Harmony
  // TODO: parse_effects implementation for A_MOD_HEALING_RECEIVED_FROM_SPELL (283)
  parse_effects( p()->talent.master_of_harmony.aspect_of_harmony_heal,
                 [ & ] { return p()->buff.aspect_of_harmony.heal_ticking(); } );
  parse_effects( p()->buff.balanced_stratagem_physical, CONSUME_BUFF );
  parse_effects( p()->buff.balanced_stratagem_magic, CONSUME_BUFF );

  // Shado-Pan

  // Midnight S1 Set
  // Midnight S2 Set
  // Midnight S3 Set
}

// Action-related parsing of debuffs. Does not work on spells
// These are action multipliers and what-not that only increase the damage
// of abilities. This does not work with tracking buffs or stat-buffs.
// Things like SEF and Serenity or debuffs that increase the crit chance
// of abilities.
template <class Base>
void monk_action_t<Base>::apply_debuff_effects()
{
  parse_target_effects( td_fn( &monk_td_t::dots_t::aspect_of_harmony ),
                        p()->talent.master_of_harmony.aspect_of_harmony_damage );
}

template <class Base>
std::unique_ptr<expr_t> monk_action_t<Base>::create_expression( std::string_view name_str )
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
        assert( _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER, Base::sim->dbc->ptr ) ] == RESOURCE_MAX &&
                "Two power entries per aura id." );
        _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER, Base::sim->dbc->ptr ) ] = pd.resource();
        break;
      case 137025:
        assert( _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER, Base::sim->dbc->ptr ) ] == RESOURCE_MAX &&
                "Two power entries per aura id." );
        _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER, Base::sim->dbc->ptr ) ] = pd.resource();
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

  if ( current_resource() == RESOURCE_CHI )
    p()->buff.dance_of_chiji->trigger();

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
  if ( p()->specialization() == MONK_WINDWALKER )
    if ( may_combo_strike )
      combo_strikes_trigger();

  base_t::execute();
}

template <class Base>
void monk_action_t<Base>::impact( action_state_t *state )
{
  trigger_mystic_touch( state );

  base_t::impact( state );
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

monk_absorb_t::monk_absorb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_action_t<absorb_t>( name, player, spell_data )
{
}

monk_melee_attack_t::monk_melee_attack_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_action_t<melee_attack_t>( name, player, spell_data )
{
  special    = true;
  may_glance = false;
  // Monk melee attacks do not have hasted GCD by default. Exceptions should be explicitly made.
  // While action_t sets attacks with 1s gcd to be non-hasted, monks use spec auras to lower ability gcds by 500ms which
  // is applied in init_finished, thus currently we cannot rely on action_t spell data parsing.
  gcd_type = gcd_haste_type::NONE;
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

struct monk_snapshot_stats_t : public snapshot_stats_t
{
  monk_snapshot_stats_t( monk_t *player, std::string_view options ) : snapshot_stats_t( player, options )
  {
  }

  void execute() override
  {
    snapshot_stats_t::execute();
  }
};

namespace attacks
{
namespace
{
struct high_impact_t : public monk_spell_t
{
  high_impact_t( monk_t *player )
    : monk_spell_t( player, "high_impact", player->talent.shado_pan.high_impact_debuff->effectN( 1 ).trigger() )
  {
    aoe        = -1;
    background = dual = true;
  }
};

struct shado_over_the_battlefield_t : public monk_spell_t
{
  shado_over_the_battlefield_t( monk_t *player )
    : monk_spell_t( player, "flurry_strike_shado_over_the_battlefield",
                    player->talent.shado_pan.flurry_strike_shado_over_the_battlefield )
  {
    background = dual   = true;
    reduced_aoe_targets = player->talent.shado_pan.shado_over_the_battlefield->effectN( 2 ).base_value();
    aoe                 = -1;
  }
};
};  // namespace

flurry_strikes_t::flurry_strike_t::flurry_strike_t( monk_t *player, std::string_view name )
  : monk_spell_t( player, name, player->talent.shado_pan.flurry_strike )
{
  background = dual = true;
  aoe               = 1;
}

void flurry_strikes_t::flurry_strike_t::impact( action_state_t *state )
{
  monk_spell_t::impact( state );

  if ( auto target_data = p()->get_target_data( state->target ); target_data )
    target_data->debuff.high_impact->trigger();
}

flurry_strikes_t::flurry_strikes_t( bool fallback, monk_t *player )
  : monk_spell_t( player, "flurry_strikes", player->talent.shado_pan.flurry_strikes ),
    fallback( !fallback ),
    high_impact( nullptr ),
    shado_over_the_battlefield( nullptr )
{
  background = dual = true;

  if ( this->fallback )
    return;

  if ( player->talent.shado_pan.high_impact->ok() )
  {
    high_impact = new high_impact_t( player );
    add_child( high_impact );
    player->register_on_kill_callback( [ & ]( player_t *target ) {
      if ( p()->sim->event_mgr.canceled )
        return;

      if ( auto target_data = p()->get_target_data( target ); target_data && target_data->debuff.high_impact->up() )
        high_impact->execute_on_target( target );
    } );
  }

  if ( player->talent.shado_pan.shado_over_the_battlefield->ok() )
  {
    shado_over_the_battlefield = new shado_over_the_battlefield_t( player );
    add_child( shado_over_the_battlefield );
  }

  flurry_strike_variants.insert( { FLURRY_STRIKES, new flurry_strike_t( player, "flurry_strike" ) } );
  if ( player->talent.shado_pan.stand_ready->ok() )
    flurry_strike_variants.insert( { STAND_READY, new flurry_strike_t( player, "flurry_strike_stand_ready" ) } );
  if ( player->talent.shado_pan.wisdom_of_the_wall->ok() )
    flurry_strike_variants.insert(
        { WISDOM_OF_THE_WALL, new flurry_strike_t( player, "flurry_strike_wisdom_of_the_wall" ) } );

  for ( auto &[ key, variant ] : flurry_strike_variants )
  {
    add_child( variant );

    switch ( key )
    {
      case FLURRY_STRIKES:
      case WISDOM_OF_THE_WALL:
        break;
      case STAND_READY:
        if ( const auto &effect = player->talent.shado_pan.stand_ready->effectN( 2 ); effect.ok() )
          add_parse_entry( variant->da_multiplier_effects )
              .set_value( effect.percent() - 1.0 )
              .set_note( "Stand Ready Efficiency Multiplier" )
              .set_eff( &effect );
        break;
      default:
        assert( false );
    }
  }
}

void flurry_strikes_t::execute( source_e source )
{
  if ( fallback )
    return;

  int count = 0;
  switch ( source )
  {
    case FLURRY_STRIKES:
      count = p()->buff.flurry_charge->stack();
      p()->buff.flurry_charge->expire();
      break;
    case STAND_READY:
      if ( p()->buff.stand_ready->check() )
        count = as<int>( p()->talent.shado_pan.stand_ready->effectN( 1 ).base_value() );
      p()->buff.stand_ready->expire();
      break;
    case WISDOM_OF_THE_WALL:
      if ( p()->buff.zenith->check() || p()->buff.invoke_niuzao->check() )
        count = as<int>( p()->talent.shado_pan.wisdom_of_the_wall->effectN( 1 ).base_value() );
      break;
    default:
      assert( false );
  }

  for ( int i = 0; i < count; i++ )
  {
    make_event<events::delayed_execute_event_t>( *sim, p(), flurry_strike_variants.at( source ), p()->target,
                                                 i * 150_ms );
    make_event<events::delayed_execute_event_t>( *sim, p(), shado_over_the_battlefield, p()->target, i * 150_ms );
  }

  monk_spell_t::execute();
}

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
      aoe                      = -1;
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

struct harmonic_surge_t : public monk_spell_t
{
  template <typename TBase>
  struct impact_t : TBase
  {
    impact_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
      : TBase( player, name, spell_data )
    {
      TBase::aoe              = -1;
      TBase::split_aoe_damage = true;

      unsigned offset = 0;

      if ( spell_data->effectN( 1 ).type() == E_SCHOOL_DAMAGE )
        offset += 0;
      if ( spell_data->effectN( 1 ).type() == E_HEAL )
        offset += 1;

      offset += 1;

      assert( offset != 0 );

      // if ( const spelleffect_data_t &effect = player->tier.tww3.moh_2pc->effectN( offset ); effect.ok() )
      //   add_parse_entry( TBase::da_multiplier_effects ).set_value( effect.percent() - 1.0 ).set_eff( &effect );
    }
  };

  action_t *damage;
  action_t *heal;

  harmonic_surge_t( monk_t *player )
    : monk_spell_t( player, "harmonic_surge", spell_data_t::nil() ),
      damage( new impact_t<monk_spell_t>( player, "harmonic_surge_damage", spell_data_t::nil() ) ),
      heal( new impact_t<monk_heal_t>( player, "harmonic_surge_heal", spell_data_t::nil() ) )
  {
  }

  void execute() override
  {
    monk_spell_t::execute();

    damage->execute();
    heal->execute();
  }
};

struct tiger_palm_t : public overwhelming_force_t<monk_melee_attack_t>
{
  bool face_palm;
  action_t *harmonic_surge;

  tiger_palm_t( monk_t *p, std::string_view options_str )
    : base_t( p, "tiger_palm", p->baseline.monk.tiger_palm ), face_palm( false ), harmonic_surge( nullptr )
  {
    parse_options( options_str );

    ww_mastery       = true;
    may_combo_strike = true;
    cast_during_sck  = true;

    spell_power_mod.direct = 0.0;

    std::function<bool()> fp_condition;
    if ( p->wowv_l( { 11, 2, 0 } ) )
      fp_condition = [ & ] { return face_palm; };
    else
      fp_condition = [] { return true; };

    if ( const auto &effect = p->talent.brewmaster.face_palm->effectN( 2 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( fp_condition )
          .set_value( effect.percent() - ( p->wowv_l( { 11, 2, 0 } ) ? 1.0 : 0.0 ) )
          .set_eff( &effect );
    parse_effects( p->buff.combat_wisdom );
  }

  bool ready() override
  {
    if ( p()->talent.brewmaster.press_the_advantage->ok() )
      return false;
    return base_t::ready();
  }

  void execute() override
  {
    if ( p()->buff.blackout_combo->up() )
      p()->proc.blackout_combo_tiger_palm->occur();

    if ( p()->buff.counterstrike->up() )
      p()->proc.counterstrike_tp->occur();

    if ( p()->buff.courage_of_the_white_tiger->up() )
      p()->action.courage_of_the_white_tiger.base->execute();

    base_t::execute();

    if ( harmonic_surge )
      harmonic_surge->execute();

    p()->buff.blackout_combo->expire();

    if ( result_is_miss( execute_state->result ) )
      return;

    p()->buff.combo_breaker->trigger();

    // Reduces the remaining cooldown on your Brews by 1 sec
    p()->baseline.brewmaster.brews.adjust(
        timespan_t::from_seconds( p()->baseline.monk.tiger_palm->effectN( 3 ).base_value() ) );

    if ( face_palm || p()->wowv_ge( { 11, 2, 0 } ) )
      p()->baseline.brewmaster.brews.adjust( p()->talent.brewmaster.face_palm->effectN( 3 ).time_value() );

    if ( p()->buff.combat_wisdom->up() )
    {
      p()->action.combat_wisdom_eh->execute();
      p()->buff.combat_wisdom->expire();
    }
  }

  void impact( action_state_t *s ) override
  {
    base_t::impact( s );

    p()->buff.teachings_of_the_monastery->trigger();
  }
};

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

      if ( ( face_palm = true ) )
      {
        base_action_t::p()->baseline.brewmaster.brews.adjust(
            base_action_t::p()->talent.brewmaster.face_palm->effectN( 3 ).time_value() );
      }

      base_action_t::execute();

      base_action_t::p()->buff.blackout_combo->expire();
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

struct rising_sun_kick_t : monk_melee_attack_t
{
  struct base_damage_t : monk_melee_attack_t
  {
    base_damage_t( monk_t *player, std::string_view name, const spell_data_t *spell )
      : monk_melee_attack_t( player, name, spell )
    {
      ww_mastery = true;
      background = dual = true;

      if ( const auto &effect = player->talent.windwalker.skyfire_heel_buff->effectN( 1 );
           player->talent.windwalker.skyfire_heel->ok() )
      {
        int max_count = as<int>( player->talent.windwalker.skyfire_heel->effectN( 3 ).base_value() );
        add_parse_entry( crit_chance_effects )
            .set_value( effect.percent() )
            .set_value_func( [ &ae = player->sim->active_enemies, max_count ]( double base ) {
              return base * std::min( ae, max_count );
            } )
            .set_note( "Nearby Enemy Scaling" )
            .set_eff( &effect );
      }
    }

    void impact( action_state_t *state ) override
    {
      monk_melee_attack_t::impact( state );

      if ( p()->baseline.windwalker.combat_conditioning->ok() )
        state->target->debuffs.mortal_wounds->trigger();

      if ( !state->chain_target && state->result == RESULT_CRIT && p()->talent.windwalker.xuens_battlegear->ok() )
      {
        timespan_t value = -1 * p()->talent.windwalker.xuens_battlegear->effectN( 2 ).time_value();
        p()->cooldown.fists_of_fury->adjust( value, true );
        p()->proc.xuens_battlegear_reduction->occur();
      }
    }
  };

  template <typename TBase>
  struct glory_of_the_dawn_t : TBase
  {
    struct damage_t : base_damage_t
    {
      damage_t( monk_t *player, std::string_view parent_name )
        : base_damage_t( player, fmt::format( "glory_of_the_dawn_{}", parent_name ),
                         player->talent.windwalker.glory_of_the_dawn_damage )
      {
      }
    };

    action_t *damage;

    template <typename... Args>
    glory_of_the_dawn_t( monk_t *player, Args &&...args )
      : TBase( player, std::forward<Args>( args )... ), damage( nullptr )
    {
      if ( !player->talent.windwalker.glory_of_the_dawn->ok() )
        return;

      damage = new damage_t( player, TBase::name() );
      TBase::add_child( damage );
    }

    void execute() override
    {
      TBase::execute();

      if ( !damage )
        return;

      // TODO: Is this the correct way to get character sheet haste %?
      double chance = TBase::p()->talent.windwalker.glory_of_the_dawn->effectN( 2 ).percent() *
                      ( 1.0 / TBase::p()->composite_spell_haste() - 1.0 );
      if ( TBase::rng().roll( chance ) )
        damage->execute();
    }
  };

  template <typename TBase>
  struct skyfire_heel_t : TBase
  {
    struct damage_t : monk_melee_attack_t
    {
      damage_t( monk_t *player, std::string_view parent_name )
        : monk_melee_attack_t( player, fmt::format( "skyfire_heel_{}", parent_name ),
                               player->talent.windwalker.skyfire_heel_damage )
      {
        aoe = -1;
        // TODO: verify this works with overridden base_dd_x
        reduced_aoe_targets = player->talent.windwalker.skyfire_heel->effectN( 2 ).base_value();

        background = dual = true;
      }
    };

    action_t *damage;

    template <typename... Args>
    skyfire_heel_t( monk_t *player, Args &&...args ) : TBase( player, std::forward<Args>( args )... ), damage( nullptr )
    {
      if ( !player->talent.windwalker.skyfire_heel->ok() )
        return;

      damage = new damage_t( player, TBase::name() );
      TBase::add_child( damage );
    }

    void impact( action_state_t *state ) override
    {
      TBase::impact( state );

      if ( !damage )
        return;

      double value        = state->result_amount * TBase::p()->talent.windwalker.skyfire_heel->effectN( 1 ).percent();
      damage->base_dd_min = damage->base_dd_max = value;
      damage->execute_on_target( state->target );
    }
  };

  using combined_type_t = glory_of_the_dawn_t<skyfire_heel_t<base_damage_t>>;

  struct damage_t : combined_type_t
  {
    damage_t( monk_t *player )
      : combined_type_t( player, "rising_sun_kick_damage", player->talent.monk.rising_sun_kick->effectN( 1 ).trigger() )
    {
    }
  };

  struct rushing_wind_kick_t : combined_type_t
  {
    int target_count_max;
    double target_count_coefficient;

    rushing_wind_kick_t( monk_t *player )
      : combined_type_t( player, "rushing_wind_kick", player->talent.windwalker.rushing_wind_kick_damage ),
        target_count_max( 5 ),
        target_count_coefficient( 0.06 )
    {
      aoe              = -1;
      split_aoe_damage = true;

      add_parse_entry( da_multiplier_effects )
          .set_func( []() { return false; } )
          .set_value( target_count_coefficient )
          .set_note( "Target-count AoE Scaling" );
    }

    double composite_aoe_multiplier( const action_state_t *state ) const override
    {
      double mult = 1.0 + std::min( state->n_targets, 5u ) * target_count_coefficient;
      return combined_type_t::composite_aoe_multiplier( state ) * mult;
    }

    void execute() override
    {
      combined_type_t::execute();

      p()->buff.rushing_wind_kick->expire();
    }
  };

  action_t *rising_sun_kick;
  action_t *rushing_wind_kick;

  rising_sun_kick_t( monk_t *player, std::string_view options_str )
    : monk_melee_attack_t( player, "rising_sun_kick", player->talent.monk.rising_sun_kick ),
      rising_sun_kick( new damage_t( player ) ),
      rushing_wind_kick( nullptr )
  {
    parse_options( options_str );

    may_combo_strike = true;
    cast_during_sck  = true;

    add_child( rising_sun_kick );

    if ( player->talent.windwalker.rushing_wind_kick->ok() )
    {
      rushing_wind_kick = new rushing_wind_kick_t( player );
      add_child( rushing_wind_kick );
    }
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->buff.rushing_wind_kick->up() )
      rushing_wind_kick->execute_on_target( target );
    else
      rising_sun_kick->execute_on_target( target );

    if ( p()->specialization() == MONK_WINDWALKER )
      p()->action.flurry_strikes->execute( attacks::flurry_strikes_t::WISDOM_OF_THE_WALL );
    p()->buff.whirling_dragon_punch->trigger();
    p()->action.chi_wave->execute();
  }
};

struct blackout_kick_totm_proc_t : public monk_melee_attack_t
{
  blackout_kick_totm_proc_t( monk_t *p )
    : monk_melee_attack_t( p, "blackout_kick_totm_proc", p->talent.windwalker.teachings_of_the_monastery_blackout_kick )
  {
    ww_mastery         = false;
    cooldown->duration = timespan_t::zero();
    background = dual = true;
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

  void execute() override
  {
    monk_melee_attack_t::execute();
    p()->buff.memory_of_the_monastery->trigger();
  }

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->talent.windwalker.teachings_of_the_monastery->ok() )
    {
      double totm_reset_chance = p()->talent.windwalker.teachings_of_the_monastery->effectN( 1 ).percent();

      if ( rng().roll( totm_reset_chance ) )
      {
        p()->cooldown.rising_sun_kick->reset( true );
        p()->proc.rsk_reset_totm->occur();
      }
    }
  }
};

template <class base_action_t>
struct charred_passions_t : base_action_t
{
  using base_t = charred_passions_t<base_action_t>;
  struct damage_t : monk_spell_t
  {
    damage_t( monk_t *player, std::string_view name )
      : monk_spell_t( player, fmt::format( "charred_passions_{}", name ),
                      player->talent.brewmaster.charred_passions_damage )
    {
      background = dual = proc = true;
      base_multiplier          = player->talent.brewmaster.charred_passions->effectN( 1 ).percent();
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

  void execute() override
  {
    base_action_t::execute();

    if ( !base_action_t::p()->buff.charred_passions->up() )
      return;

    if ( chp_cooldown->up() )
      chp_cooldown->start( chp_damage->data().effectN( 1 ).trigger()->internal_cooldown() );
  }

  void impact( action_state_t *state ) override
  {
    base_action_t::impact( state );

    if ( !base_action_t::p()->buff.charred_passions->up() )
      return;

    base_action_t::p()->proc.charred_passions->occur();
    chp_damage->execute_on_target( state->target, state->result_amount );

    if ( monk_td_t *target_data = base_action_t::get_td( state->target );
         target_data && target_data->dot.breath_of_fire->is_ticking() && chp_cooldown->up() )
      target_data->dot.breath_of_fire->refresh_duration();
  }
};

struct blackout_kick_t : overwhelming_force_t<charred_passions_t<monk_melee_attack_t>>
{
  blackout_kick_totm_proc_t *bok_totm_proc;

  blackout_kick_t( monk_t *p, std::string_view options_str )
    : base_t( p, "blackout_kick",
              ( p->specialization() == MONK_BREWMASTER ? p->baseline.brewmaster.blackout_kick
                                                       : p->baseline.monk.blackout_kick ) )
  {
    parse_options( options_str );

    ap_type          = attack_power_type::WEAPON_BOTH;
    ww_mastery       = true;
    may_combo_strike = true;
    cast_during_sck  = true;

    if ( p->talent.brewmaster.charred_passions->ok() )
      add_child( base_t::chp_damage );

    if ( p->talent.windwalker.teachings_of_the_monastery->ok() )
    {
      bok_totm_proc = new blackout_kick_totm_proc_t( p );
      add_child( bok_totm_proc );
    }

    if ( p->talent.windwalker.obsidian_spiral->ok() )
      parse_effect_data( p->talent.windwalker.obsidian_spiral_energize->effectN( 1 ) );

    if ( const auto &effect = p->talent.windwalker.shadowboxing_treads->effectN( 3 ); effect.ok() )
      add_parse_entry( target_multiplier_effects )
          .set_func( [ this ]( actor_target_data_t *target_data ) { return target_data->target != target; } )
          .set_value( effect.percent() - 1.0 )
          .set_note( "Secondary Target Reduction" )
          .set_eff( &effect );
  }

  void consume_resource() override
  {
    base_t::consume_resource();

    // Register how much chi is saved without actually refunding the chi
    if ( p()->buff.combo_breaker->up() )
      p()->gain.combo_breaker->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
  }

  void execute() override
  {
    base_t::execute();

    if ( p()->buff.invoke_niuzao->check() )
      p()->pets.niuzao.active_pet()->stomp->execute();

    p()->buff.shuffle->trigger(
        timespan_t::from_seconds( p()->baseline.brewmaster.blackout_kick->effectN( 2 ).base_value() ) );

    p()->buff.blackout_combo->trigger();
    if ( !result_is_hit( execute_state->result ) )
      return;

    if ( p()->buff.combo_breaker->up() )
    {
      if ( p()->rng().roll( p()->talent.windwalker.rushing_wind_kick->effectN( 1 ).percent() ) )
        p()->buff.rushing_wind_kick->trigger();

      if ( p()->rng().roll( p()->talent.windwalker.energy_burst->effectN( 1 ).percent() ) )
        p()->resource_gain( RESOURCE_CHI, p()->talent.windwalker.energy_burst->effectN( 2 ).base_value(),
                            p()->gain.energy_burst );

      p()->buff.combo_breaker->decrement();
    }

    if ( p()->buff.teachings_of_the_monastery->check() )
    {
      int stacks = p()->buff.teachings_of_the_monastery->stack();

      if ( p()->bugs )
        p()->buff.memory_of_the_monastery->expire();
      p()->buff.teachings_of_the_monastery->expire();

      for ( int i = 0; i < stacks; ++i )
      {
        // quick estimate for delay between totm activations, not rigorously tested for
        make_event<events::delayed_execute_event_t>( *sim, p(), bok_totm_proc, p()->target, i * 100_ms );
        if ( p()->rng().roll( p()->talent.conduit_of_the_celestials.xuens_guidance->effectN( 1 ).percent() ) )
          p()->buff.teachings_of_the_monastery->trigger();
      }
    }

    if ( p()->specialization() == MONK_WINDWALKER && p()->buff.strength_of_the_black_ox->check() )
      p()->action.strength_of_the_black_ox.base->execute();
  }

  void impact( action_state_t *s ) override
  {
    base_t::impact( s );

    if ( p()->talent.brewmaster.elusive_footwork->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.elusive_brawler->trigger(
          as<int>( p()->talent.brewmaster.elusive_footwork->effectN( 2 ).base_value() ) );
      p()->proc.elusive_footwork_proc->occur();
    }

    if ( p()->talent.brewmaster.staggering_strikes->ok() )
    {
      double missing_health_percentage = 1.0 - std::max( p()->health_percentage() / 100.0, 0.0 );
      double m = 1.0 + missing_health_percentage * p()->talent.brewmaster.staggering_strikes->effectN( 3 ).percent();

      p()->find_stagger( "Stagger" )
          ->purify_flat(
              s->composite_attack_power() * p()->talent.brewmaster.staggering_strikes->effectN( 2 ).percent() * m,
              "staggering_strikes" );
    }

    if ( p()->talent.windwalker.teachings_of_the_monastery->ok() )
    {
      double totm_reset_chance = p()->talent.windwalker.teachings_of_the_monastery->effectN( 1 ).percent();

      if ( rng().roll( totm_reset_chance ) )
      {
        p()->cooldown.rising_sun_kick->reset( true );
        p()->proc.rsk_reset_totm->occur();
      }
    }
  }
};

struct rushing_jade_wind_t : public monk_melee_attack_t
{
  buff_t *buff;

  rushing_jade_wind_t( monk_t *player, std::string_view options_str )
    : monk_melee_attack_t( player, "rushing_jade_wind", player->talent.brewmaster.rushing_jade_wind ),
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

// Jade Ignition Talent
struct chi_explosion_t : public monk_spell_t
{
  chi_explosion_t( monk_t *player ) : monk_spell_t( player, "chi_explosion", player->talent.windwalker.chi_explosion )
  {
    dual = background = true;
    aoe               = -1;
    school            = SCHOOL_NATURE;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    // TODO: convert to parse_effects manually
    am *= 1 + p()->buff.chi_energy->check_stack_value();

    return am;
  }
};

struct sck_tick_action_t : charred_passions_t<monk_melee_attack_t>
{
  sck_tick_action_t( monk_t *p, std::string_view name, const spell_data_t *data )
    : charred_passions_t<monk_melee_attack_t>( p, name, data )
  {
    ww_mastery = true;

    dual = background   = true;
    aoe                 = -1;
    reduced_aoe_targets = p->baseline.monk.spinning_crane_kick->effectN( 1 ).base_value();
    ap_type             = attack_power_type::WEAPON_BOTH;

    // dance of chiji is scripted
    if ( const auto &effect = p->talent.windwalker.dance_of_chiji_buff->effectN( 2 ); effect.ok() )
      add_parse_entry( da_multiplier_effects )
          .set_func( [ &b = p->buff.dance_of_chiji_hidden ]() { return b->check(); } )
          .set_value( effect.percent() )
          .set_eff( &effect );
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
  chi_explosion_t *chi_x;

  spinning_crane_kick_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "spinning_crane_kick",
                           ( p->specialization() == MONK_BREWMASTER ? p->baseline.brewmaster.spinning_crane_kick
                                                                    : p->baseline.monk.spinning_crane_kick ) ),
      chi_x( nullptr )
  {
    parse_options( options_str );

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
          p()->buff.combo_breaker->increment();  // increment is used to directly trigger without rolling chance
      }
    }

    monk_melee_attack_t::execute();

    if ( p()->specialization() == MONK_WINDWALKER )
      p()->action.flurry_strikes->execute( attacks::flurry_strikes_t::WISDOM_OF_THE_WALL );

    timespan_t buff_duration = composite_dot_duration( execute_state );

    p()->buff.spinning_crane_kick->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, buff_duration );

    if ( chi_x && p()->buff.chi_energy->up() )
      chi_x->execute();
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

struct fists_of_fury_t : monk_melee_attack_t
{
  struct tick_t : monk_melee_attack_t
  {
    tick_t( monk_t *player )
      : monk_melee_attack_t( player, "fists_of_fury_damage",
                             player->talent.windwalker.fists_of_fury->effectN( 3 ).trigger() )
    {
      background = dual   = true;
      aoe                 = -1;
      full_amount_targets = 1;
      reduced_aoe_targets = player->talent.windwalker.fists_of_fury->effectN( 1 ).base_value();
      ww_mastery          = true;

      parse_effects( player->buff.momentum_boost_damage );
      if ( const auto &effect = player->talent.windwalker.momentum_boost->effectN( 1 ); effect.ok() )
        add_parse_entry( da_multiplier_effects )
            .set_value_func(
                [ & ]( double ) { return ( 1.0 / p()->composite_melee_haste() - 1.0 ) * effect.percent(); } )
            .set_eff( &effect );

      add_parse_entry( da_multiplier_effects )
          .set_value( player->talent.windwalker.fists_of_fury->effectN( 6 ).percent() - 1.0 )
          .set_func( [] { return false; } )
          .set_note( "Secondary Target Damage Modifier" )
          .set_eff( &player->talent.windwalker.fists_of_fury->effectN( 6 ) );
    }

    double composite_aoe_multiplier( const action_state_t *state ) const override
    {
      double cam = monk_melee_attack_t::composite_aoe_multiplier( state );

      if ( state->chain_target )
        cam *= p()->talent.windwalker.fists_of_fury->effectN( 6 ).percent();

      return cam;
    }

    void impact( action_state_t *state ) override
    {
      monk_melee_attack_t::impact( state );

      p()->buff.momentum_boost_damage->trigger();
    }
  };

  struct jadefire_stomp_t : monk_melee_attack_t
  {
    jadefire_stomp_t( monk_t *player )
      : monk_melee_attack_t( player, "jadefire_stomp", player->talent.windwalker.jadefire_stomp_damage )
    {
      aoe        = player->talent.windwalker.jadefire_stomp_targeting->effectN( 1 ).base_value();
      background = dual = true;
      ww_mastery        = true;

      if ( const auto &effect = player->talent.windwalker.path_of_jade->effectN( 1 ); effect.ok() )
        add_parse_entry( da_multiplier_effects )
            .set_value( effect.percent() )
            .set_func( [] { return false; } )
            .set_note( "Per-Target Increase" )
            .set_eff( &effect );
    }

    double composite_aoe_multiplier( const action_state_t *state ) const
    {
      double cam = monk_melee_attack_t::composite_aoe_multiplier( state );

      if ( p()->talent.windwalker.path_of_jade->ok() )
      {
        double count =
            std::min( p()->talent.windwalker.path_of_jade->effectN( 2 ).base_value(), as<double>( state->n_targets ) );
        cam *= 1.0 + count * p()->talent.windwalker.path_of_jade->effectN( 1 ).percent();
      }

      return cam;
    }
  };

  action_t *jadefire_stomp;

  fists_of_fury_t( monk_t *player, std::string_view options_str )
    : monk_melee_attack_t( player, "fists_of_fury", player->talent.windwalker.fists_of_fury ), jadefire_stomp( nullptr )
  {
    parse_options( options_str );

    may_combo_strike        = true;
    attack_power_mod.direct = 0;

    // expected base tick time is always 1_s
    // default: 5 ticks over 4s w/ tick 0, 4 / 4 = 1
    // crashing fists: 6 ticks over 5s w/ tick0, 5 / 5 = 1
    base_tick_time = 1_s;
    tick_action    = new tick_t( player );

    if ( player->talent.windwalker.jadefire_stomp->ok() )
    {
      jadefire_stomp = new jadefire_stomp_t( player );
      add_child( jadefire_stomp );
    }
  }

  bool usable_moving() const override
  {
    return true;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->action.flurry_strikes->execute( flurry_strikes_t::FLURRY_STRIKES );
    p()->buff.whirling_dragon_punch->trigger();
  }

  void last_tick( dot_t *dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    if ( jadefire_stomp )
      jadefire_stomp->execute();

    // TODO: is there a better way to do this?
    // Delay the expiration of the buffs until after the tick action happens.
    // Otherwise things trigger before the tick action happens; which is not intended.
    make_event( p()->sim, timespan_t::from_millis( 1 ), [ & ] {
      p()->buff.momentum_boost_damage->expire();
      p()->buff.momentum_boost_speed->trigger();
    } );
  }
};

struct whirling_dragon_punch_aoe_tick_t : public monk_melee_attack_t
{
  timespan_t delay;
  whirling_dragon_punch_aoe_tick_t( std::string_view name, monk_t *p, const spell_data_t *s, timespan_t delay )
    : monk_melee_attack_t( p, name, s ), delay( delay )
  {
    ww_mastery = true;

    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = p->talent.windwalker.whirling_dragon_punch->effectN( 1 ).base_value();

    name_str_reporting = "wdp_aoe";
  }
};

struct whirling_dragon_punch_st_tick_t : public monk_melee_attack_t
{
  whirling_dragon_punch_st_tick_t( std::string_view name, monk_t *p, const spell_data_t *s )
    : monk_melee_attack_t( p, name, s )
  {
    ww_mastery = true;

    background = true;

    name_str_reporting = "wdp_st";
  }
};

struct whirling_dragon_punch_t : public monk_melee_attack_t
{
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

  whirling_dragon_punch_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "whirling_dragon_punch", p->talent.windwalker.whirling_dragon_punch )
  {
    parse_options( options_str );
    interrupt_auto_attack = false;
    channeled             = false;
    may_combo_strike      = true;
    cast_during_sck       = true;

    spell_power_mod.direct = 0.0;

    // 3 server-side hardcoded ticks
    for ( size_t i = 0; i < aoe_ticks.size(); ++i )
    {
      auto delay     = base_tick_time * i;
      aoe_ticks[ i ] = new whirling_dragon_punch_aoe_tick_t(
          "whirling_dragon_punch_aoe_tick", p, p->talent.windwalker.whirling_dragon_punch_aoe_tick, delay );

      add_child( aoe_ticks[ i ] );
    }

    st_tick = new whirling_dragon_punch_st_tick_t( "whirling_dragon_punch_st_tick", p,
                                                   p->talent.windwalker.whirling_dragon_punch_st_tick );
    add_child( st_tick );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

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
  }

  bool ready() override
  {
    // Only usable while Fists of Fury and Rising Sun Kick are on cooldown.
    // TODO: Fix this, this is very wrong
    if ( p()->buff.whirling_dragon_punch->up() )
      return monk_melee_attack_t::ready();

    return false;
  }
};

struct strike_of_the_windlord_main_hand_t : public monk_melee_attack_t
{
  strike_of_the_windlord_main_hand_t( monk_t *p, const char *name, const spell_data_t *s )
    : monk_melee_attack_t( p, name, s )
  {
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
};

struct strike_of_the_windlord_off_hand_t : public monk_melee_attack_t
{
  strike_of_the_windlord_off_hand_t( monk_t *p, const char *name, const spell_data_t *s )
    : monk_melee_attack_t( p, name, s )
  {
    ww_mastery = true;
    ap_type    = attack_power_type::WEAPON_OFFHAND;

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

  void impact( action_state_t *s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->talent.windwalker.thunderfist.ok() )
    {
      int thunderfist_stacks = 1;

      if ( s->chain_target == 0 )
        // The first target will trigger the 4 stacks of the Thunderfist buff, all others will trigger 1 stack
        thunderfist_stacks = as<int>( p()->talent.windwalker.thunderfist->effectN( 1 ).base_value() );

      p()->buff.thunderfist->trigger( thunderfist_stacks );
    }
  }
};

struct strike_of_the_windlord_t : public monk_melee_attack_t
{
  // Off hand hits first followed by main hand
  // The ability does NOT require an off-hand weapon to be executed.
  // The ability uses the main-hand weapon damage for both attacks
  strike_of_the_windlord_main_hand_t *mh_attack;
  strike_of_the_windlord_off_hand_t *oh_attack;

  strike_of_the_windlord_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "strike_of_the_windlord", p->talent.windwalker.strike_of_the_windlord ),
      mh_attack( nullptr ),
      oh_attack( nullptr )
  {
    may_combo_strike = true;
    cast_during_sck  = true;
    cooldown->hasted = false;
    trigger_gcd      = data().gcd();

    parse_options( options_str );

    oh_attack =
        new strike_of_the_windlord_off_hand_t( p, "strike_of_the_windlord_offhand", data().effectN( 4 ).trigger() );
    mh_attack =
        new strike_of_the_windlord_main_hand_t( p, "strike_of_the_windlord_mainhand", data().effectN( 3 ).trigger() );

    add_child( oh_attack );
    add_child( mh_attack );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    // Off-hand attack hits first
    oh_attack->execute();

    if ( result_is_hit( oh_attack->execute_state->result ) )
      mh_attack->execute();

    p()->buff.heart_of_the_jade_serpent->trigger();
    p()->buff.inner_compass_serpent_stance->trigger();
  }
};

struct auto_attack_t : public monk_melee_attack_t
{
  template <typename TBase>
  struct dual_threat_t : TBase
  {
    struct damage_t : public monk_spell_t
    {
      bool allowed;
      bool triggered;
      double chance;

      damage_t( monk_t *player )
        : monk_spell_t( player, "dual_threat", player->talent.windwalker.dual_threat_damage ), allowed( false )
      {
        background                = true;
        allow_class_ability_procs = false;
        may_miss                  = false;

        chance = p()->talent.windwalker.dual_threat->effectN( 1 ).percent();
      }

      void reset() override
      {
        monk_spell_t::reset();
        allowed = false;
      }

      void execute() override
      {
        if ( allowed && p()->rng().roll( chance ) )
        {
          monk_spell_t::execute();
          allowed   = false;
          triggered = true;
        }
        else
          allowed = true;
      }
    };

    damage_t *damage;
    bool allowed;

    template <typename... Args>
    dual_threat_t( monk_t *player, weapon_t *weapon, Args &&...args )
      : TBase( player, weapon, std::forward<Args>( args )... ), damage( nullptr ), allowed( false )
    {
      if ( !player->talent.windwalker.dual_threat->ok() )
        return;

      if ( action_t *dt = player->find_action( "dual_threat" ); dt )
        damage = debug_cast<damage_t *>( dt );
      else
        damage = new damage_t( player );

      if ( action_t *aa = player->find_action( "auto_attack" ); aa )
        aa->add_child( damage );
    }

    void impact( action_state_t *state ) override
    {
      // TODO: check if DT has any impact on melee success rate
      if ( damage && result_is_hit( state->result ) )
      {
        damage->execute_on_target( state->target );
        if ( damage->triggered )
        {
          damage->triggered = false;
          return;
        }
      }

      TBase::impact( state );
    }
  };

  template <typename TBase>
  struct press_the_advantage_t : TBase
  {
    struct damage_t : public monk_spell_t
    {
      damage_t( monk_t *player )
        : monk_spell_t( player, "press_the_advantage", player->talent.brewmaster.press_the_advantage_damage )
      {
        background = dual = true;
      }
    };

    action_t *damage;

    template <typename... Args>
    press_the_advantage_t( monk_t *player, weapon_t *weapon, Args &&...args )
      : TBase( player, weapon, std::forward<Args>( args )... ), damage( nullptr )
    {
      if ( !player->talent.brewmaster.press_the_advantage->ok() || weapon->slot != SLOT_MAIN_HAND )
        return;

      damage = new damage_t( player );
      TBase::add_child( damage );
    }

    void impact( action_state_t *state ) override
    {
      TBase::impact( state );

      if ( !damage || !result_is_hit( state->result ) )
        return;

      TBase::p()->buff.press_the_advantage->trigger();
      TBase::p()->baseline.brewmaster.brews.adjust(
          TBase::p()->talent.brewmaster.press_the_advantage->effectN( 2 ).time_value() );
      damage->execute_on_target( state->target );
    }
  };

  template <typename TBase>
  struct thunderfist_t : TBase
  {
    struct damage_t : public monk_spell_t
    {
      damage_t( monk_t *player )
        : monk_spell_t( player, "thunderfist", player->talent.windwalker.thunderfist_buff->effectN( 1 ).trigger() )
      {
        background = dual = true;
        may_miss          = false;
      }
    };

    action_t *damage;

    template <typename... Args>
    thunderfist_t( monk_t *player, Args &&...args ) : TBase( player, std::forward<Args>( args )... )
    {
      if ( !player->talent.windwalker.thunderfist->ok() )
        return;

      if ( action_t *tf = player->find_action( "thunderfist" ); tf )
        damage = tf;
      else
        damage = new damage_t( player );
    }

    void init() override
    {
      TBase::init();

      if ( action_t *wdp = TBase::p()->find_action( "whirling_dragon_punch" ); wdp )
        wdp->add_child( damage );
      else if ( action_t *sotwl = TBase::p()->find_action( "strike_of_the_windlord" ); sotwl )
        sotwl->add_child( damage );
    }

    void impact( action_state_t *state ) override
    {
      TBase::impact( state );

      if ( !damage || !result_is_hit( state->result ) || !TBase::p()->buff.thunderfist->up() )
        return;

      damage->execute_on_target( state->target );
      TBase::p()->buff.thunderfist->decrement();
    }
  };

  struct melee_t : public monk_melee_attack_t
  {
    bool first;
    bool sync_weapons;

    melee_t( monk_t *player, weapon_t *weapon, action_t *parent )
      : monk_melee_attack_t( player, fmt::format( "melee_{}", util::slot_type_string( weapon->slot ) ) ),
        first( true ),
        sync_weapons( false )
    {
      background = repeating = may_glance = true;
      may_crit = allow_class_ability_procs = not_a_proc = true;
      special                                           = false;
      trigger_gcd                                       = 0_ms;
      school                                            = SCHOOL_PHYSICAL;
      weapon_multiplier                                 = 1.0;

      switch ( weapon->slot )
      {
        case SLOT_MAIN_HAND:
          player->main_hand_attack = this;
          break;
        case SLOT_OFF_HAND:
          player->off_hand_attack = this;
          break;
        default:
          assert( false );
      }
      monk_melee_attack_t::weapon = weapon;
      base_execute_time           = weapon->swing_time;

      if ( player->main_hand_weapon.group() == WEAPON_1H )
        base_hit -= 0.19;

      parent->add_child( this );

      if ( const auto &effect = player->talent.monk.tiger_fang->effectN( 1 ); effect.ok() )
        add_parse_entry( crit_chance_effects ).set_value( effect.percent() ).set_eff( &effect );
    }

    void reset() override
    {
      monk_melee_attack_t::reset();
      first = true;
    }

    void execute() override
    {
      monk_melee_attack_t::execute();
      first = false;
    }

    timespan_t execute_time() const override
    {
      timespan_t time = monk_melee_attack_t::execute_time();

      if ( !first )
        return time;

      if ( weapon->slot == SLOT_MAIN_HAND )
        return 0_ms;
      if ( first && weapon->slot == SLOT_OFF_HAND && sync_weapons )
        return 0_ms;
      if ( first && weapon->slot == SLOT_OFF_HAND && !sync_weapons )
        return time / 2;

      assert( false );
      return time;
    }
  };

  int sync_weapons;

  auto_attack_t( monk_t *player, std::string_view options_str )
    : monk_melee_attack_t( player, "auto_attack" ), sync_weapons( 0 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );

    // these pointers register themselves in places which cause them to get properly destructed
    new dual_threat_t<press_the_advantage_t<thunderfist_t<melee_t>>>( player, &player->main_hand_weapon, this );
    if ( player->off_hand_weapon.type != WEAPON_NONE )
      new dual_threat_t<press_the_advantage_t<thunderfist_t<melee_t>>>( player, &player->off_hand_weapon, this );
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move > 5 )
      return false;

    // no execute event queued implies no swing ongoing
    return p()->main_hand_attack->execute_event == nullptr ||
           ( p()->off_hand_attack && p()->off_hand_attack->execute_event == nullptr );
  }

  void execute() override
  {
    if ( p()->main_hand_attack )
      p()->main_hand_attack->schedule_execute();
    if ( p()->off_hand_attack )
      p()->off_hand_attack->schedule_execute();

    monk_melee_attack_t::execute();
  }
};

struct keg_smash_t : monk_melee_attack_t
{
  cooldown_t *breath_of_fire;

  keg_smash_t( monk_t *player, std::string_view options_str, std::string_view name = "keg_smash" )
    : monk_melee_attack_t( player, name, player->talent.brewmaster.keg_smash ), breath_of_fire( nullptr )
  {
    parse_options( options_str );
    // TODO: can cast_during_sck be automated?
    cast_during_sck = true;

    reduced_aoe_targets = data().effectN( 7 ).base_value();
    aoe                 = -1;
    // scalding brew is scripted?
    if ( const auto &effect = player->talent.brewmaster.scalding_brew->effectN( 1 ); effect.ok() )
      add_parse_entry( target_multiplier_effects )
          .set_func( td_fn( &monk_td_t::dots_t::breath_of_fire ) )
          .set_value( effect.percent() )
          .set_eff( &effect );

    // increased damage to primary target
    if ( const auto &effect = player->talent.brewmaster.stormstouts_last_keg->effectN( 1 ); effect.ok() )
      add_parse_entry( target_multiplier_effects )
          .set_func( [ this ]( actor_target_data_t *target_data ) { return target_data->target == target; } )
          .set_value( effect.percent() )
          .set_eff( &effect )
          .set_note( "Primary Target" );

    if ( player->talent.brewmaster.salsalabims_strength->ok() )
      breath_of_fire = player->get_cooldown( "breath_of_fire" );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( breath_of_fire )
    {
      breath_of_fire->reset( true );
      p()->proc.salsalabims_strength->occur();
    }

    p()->action.flurry_strikes->execute( flurry_strikes_t::FLURRY_STRIKES );
    p()->buff.shuffle->trigger( timespan_t::from_seconds( data().effectN( 6 ).base_value() ) );

    timespan_t reduction = timespan_t::from_seconds( data().effectN( 4 ).base_value() );
    if ( p()->buff.blackout_combo->up() )
    {
      reduction += timespan_t::from_seconds( p()->buff.blackout_combo->data().effectN( 3 ).base_value() );
      p()->proc.blackout_combo_keg_smash->occur();
    }
    p()->buff.blackout_combo->expire();
    p()->baseline.brewmaster.brews.adjust( reduction );
    p()->action.chi_wave->execute();
  }

  void impact( action_state_t *state ) override
  {
    monk_melee_attack_t::impact( state );
    get_td( state->target )->debuff.keg_smash->trigger();
  }
};

struct stomp_t : monk_melee_attack_t
{
  stomp_t( monk_t *player ) : monk_melee_attack_t( player, "stomp", player->talent.brewmaster.walk_with_the_ox_stomp )
  {
    background = true;
    aoe        = -1;
  }
};

struct touch_of_death_t : public monk_melee_attack_t
{
  touch_of_death_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "touch_of_death", p->baseline.monk.touch_of_death )
  {
    ww_mastery = true;
    may_crit = hasted_ticks = false;
    may_combo_strike        = true;
    cast_during_sck         = true;
    parse_options( options_str );

    cooldown->duration = data().cooldown();
  }

  void init() override
  {
    monk_melee_attack_t::init();

    snapshot_flags = update_flags = 0;
  }

  double composite_target_armor( player_t * ) const override
  {
    // instead use the trick to have no multipliers apply?
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
      amount *= p()->talent.monk.improved_touch_of_death->effectN( 2 ).percent();  // 0.35

      // Apply and parsed multipliers
      amount *= base_dd_multiplier;

      // Damage is only affected by Windwalker's Mastery
      // Versatility does not affect the damage of Touch of Death.
      if ( p()->buff.combo_strikes->up() )
        amount *= 1 + p()->cache.mastery_value();
    }

    s->result_total = s->result_raw = amount;

    monk_melee_attack_t::impact( s );

    if ( p()->talent.monk.stagger->ok() )
    {
      p()->find_stagger( "Stagger" )
          ->purify_flat( amount * p()->baseline.brewmaster.touch_of_death_rank_3->effectN( 1 ).percent(),
                         "touch_of_death" );
    }
  }
};

struct touch_of_karma_dot_t : public residual_action::residual_periodic_action_t<spell_t>
{
  using base_t = residual_action::residual_periodic_action_t<spell_t>;
  touch_of_karma_dot_t( monk_t *p ) : base_t( "touch_of_karma", p, p->baseline.windwalker.touch_of_karma_tick )
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
  // When Touch of Karma (ToK) is activated, two spells are placed. A buff on the player (id: 125174), and a
  // debuff on the target (id: 122470). Whenever the player takes damage, a dot (id: 124280) is placed on
  // the target that increases as the player takes damage. Each time the player takes damage, the dot is refreshed
  // and recalculates the dot size based on the current dot size. Just to make it easier to code, I'll wait until
  // the Touch of Karma buff expires before placing a dot on the target. Net result should be the same.

  // 8.1 Good Karma - If the player still has the ToK buff on them, each time the target hits the player, the amount
  // absorbed is immediatly healed by the Good Karma spell (id: 285594)
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double pct_health;
  touch_of_karma_dot_t *touch_of_karma_dot;
  touch_of_karma_t( monk_t *p, std::string_view options_str )
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

struct provoke_t : public monk_melee_attack_t
{
  provoke_t( monk_t *p, std::string_view options_str ) : monk_melee_attack_t( p, "provoke", p->baseline.monk.provoke )
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

struct spear_hand_strike_t : public monk_melee_attack_t
{
  spear_hand_strike_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "spear_hand_strike", p->talent.monk.spear_hand_strike )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    is_interrupt          = true;
    cast_during_sck       = player->specialization() != MONK_WINDWALKER;
    may_miss = may_block = may_dodge = may_parry = false;
  }
};

struct leg_sweep_t : public monk_melee_attack_t
{
  leg_sweep_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "leg_sweep", p->baseline.monk.leg_sweep )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
    cast_during_sck                              = true;
  }
};

struct paralysis_t : public monk_melee_attack_t
{
  paralysis_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "paralysis", p->talent.monk.paralysis )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }
};

struct flying_serpent_kick_t : public monk_melee_attack_t
{
  bool first_charge;
  flying_serpent_kick_t( monk_t *p, std::string_view options_str )
    : monk_melee_attack_t( p, "flying_serpent_kick", p->baseline.windwalker.flying_serpent_kick ), first_charge( true )
  {
    parse_options( options_str );
    may_crit              = true;
    ww_mastery            = true;
    may_combo_strike      = true;
    ignore_false_positive = true;
    aoe                   = -1;
  }

  void reset() override
  {
    monk_melee_attack_t::reset();
    first_charge = true;
  }

  bool ready() override
  {
    if ( p()->talent.windwalker.slicing_winds->ok() )
      return false;
    if ( first_charge )  // Assumes that we fsk into combat, instead of setting initial distance to 20 yards.
      return monk_melee_attack_t::ready();

    return monk_melee_attack_t::ready();
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( first_charge )
      first_charge = !first_charge;
  }
};

struct slicing_winds_t : public monk_melee_attack_t
{
  // TODO: Potentially add the "empowered" channel. "Empowered" channel increases
  // the "lunge" or movement of the attack, but also delays the actual attack by
  // as little as 500 milliseconds. Just pressing the attack; without the
  // "empowered" channel, animation lasts 1 second.
  // Empowered channel has 4 stages; with each lasting about 350 milliseconds.
  struct damage_t : monk_melee_attack_t
  {
    damage_t( monk_t *player )
      : monk_melee_attack_t( player, "slicing_winds_damage", player->talent.windwalker.slicing_winds_damage )
    {
      background = dual   = true;
      ww_mastery          = true;
      may_combo_strike    = true;
      aoe                 = -1;
      reduced_aoe_targets = player->talent.windwalker.slicing_winds->effectN( 3 ).base_value();
    }
  };

  slicing_winds_t( monk_t *player, std::string_view options_str )
    : monk_melee_attack_t( player, "slicing_winds", player->talent.windwalker.slicing_winds )
  {
    parse_options( options_str );

    execute_action = new damage_t( player );
    add_child( execute_action );

    // override gcd as we are not properly handling it as an empowered channel
    trigger_gcd = timespan_t::from_millis( 1400 );

    if ( player->talent.windwalker.airborne_rhythm->ok() )
      parse_effect_data( player->talent.windwalker.airborne_rhythm_energize->effectN( 1 ) );
  }
};
}  // namespace attacks

namespace spells
{
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

    add_child( damage );
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

struct special_delivery_t : public monk_spell_t
{
  special_delivery_t( monk_t *player )
    : monk_spell_t( player, "special_delivery",
                    player->talent.brewmaster.special_delivery_missile->effectN( 1 ).trigger() )
  {
    background   = true;
    travel_delay = player->talent.brewmaster.special_delivery_missile->missile_speed();
    aoe          = -1;
  }

  void execute() override
  {
    if ( !p()->talent.brewmaster.special_delivery->ok() )
      return;
    monk_spell_t::execute();
  }
};

struct black_ox_brew_t : public brew_t<monk_spell_t>
{
  black_ox_brew_t( monk_t *player, std::string_view options_str )
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
      if ( key == p()->talent.brewmaster.celestial_infusion->id() )
        cooldown->reset( true, 1 );
    }

    p()->resource_gain( RESOURCE_ENERGY, p()->talent.brewmaster.black_ox_brew->effectN( 1 ).base_value(),
                        p()->gain.black_ox_brew_energy );

    p()->buff.pretense_of_instability->trigger();
    p()->action.special_delivery->execute();
  }
};

struct roll_t : public monk_spell_t
{
  roll_t( monk_t *player, std::string_view options_str )
    : monk_spell_t( player, "roll",
                    ( player->talent.monk.chi_torpedo->ok() ? spell_data_t::not_found() : player->baseline.monk.roll ) )
  {
    cast_during_sck = true;

    parse_options( options_str );
  }
};

struct chi_torpedo_t : public monk_spell_t
{
  chi_torpedo_t( monk_t *player, std::string_view options_str )
    : monk_spell_t(
          player, "chi_torpedo",
          ( player->talent.monk.chi_torpedo->ok() ? player->talent.monk.chi_torpedo : spell_data_t::not_found() ) )
  {
    parse_options( options_str );

    cast_during_sck = true;
  }
};

struct crackling_jade_lightning_t : public monk_spell_t
{
  struct aoe_dot_t : public monk_spell_t
  {
    aoe_dot_t( monk_t *player )
      : monk_spell_t( player, "crackling_jade_lightning_aoe", player->baseline.monk.crackling_jade_lightning )
    {
      dual = background = true;
      ww_mastery        = true;
    }

    double cost_per_tick( resource_e ) const override
    {
      return 0.0;
    }
  };

  aoe_dot_t *aoe_dot;

  crackling_jade_lightning_t( monk_t *player, std::string_view options_str )
    : monk_spell_t( player, "crackling_jade_lightning", player->baseline.monk.crackling_jade_lightning ),
      aoe_dot( nullptr )
  {
    parse_options( options_str );

    may_combo_strike      = true;
    ww_mastery            = true;
    interrupt_auto_attack = true;
    channeled             = true;

    min_gcd = timespan_t::from_millis( 750 );

    if ( player->talent.brewmaster.jade_flash->ok() )
    {
      aoe_dot = new aoe_dot_t( player );
      add_child( aoe_dot );
    }
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( aoe_dot )
    {
      auto &tl = target_list();
      range::erase_remove( tl, [ this ]( const auto &t ) { return t == target; } );
      size_t count = std::min( tl.size(), as<size_t>( p()->talent.brewmaster.jade_flash->effectN( 2 ).base_value() ) );
      for ( size_t i = 0; i < count; ++i )
        aoe_dot->execute_on_target( tl[ i ] );
    }
  }

  void last_tick( dot_t *dot ) override
  {
    monk_spell_t::last_tick( dot );

    // delay expiration so it occurs after final tick of cjl aoe
    if ( aoe_dot )
      make_event<events::delayed_cb_event_t>( *sim, p(), 1_ms, [ & ]() {
        // buff expire
        const auto &tl = target_list();
        for ( const auto &t : tl )
          get_td( t )->dot.crackling_jade_lightning_aoe->cancel();
      } );

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

struct breath_of_fire_state_t : public action_state_t
{
  bool blackout_combo;

  breath_of_fire_state_t( action_t *a, player_t *t ) : action_state_t( a, t ), blackout_combo( false )
  {
  }

  std::ostringstream &debug_str( std::ostringstream &s ) override
  {
    action_state_t::debug_str( s );
    fmt::print( s, " blackout_combo={}", blackout_combo );
    return s;
  }

  void initialize() override
  {
    action_state_t::initialize();
    blackout_combo = false;
  }

  void copy_state( const action_state_t *o ) override
  {
    action_state_t::copy_state( o );
    auto other_sa_state = debug_cast<const breath_of_fire_state_t *>( o );
    blackout_combo      = other_sa_state->blackout_combo;
  }
};

struct breath_of_fire_dot_t : public monk_spell_t
{
protected:
  using custom_state_t = breath_of_fire_state_t;

public:
  breath_of_fire_dot_t( monk_t *p ) : monk_spell_t( p, "breath_of_fire_dot", p->talent.brewmaster.breath_of_fire_dot )
  {
    background    = true;
    tick_may_crit = may_crit = true;
    hasted_ticks             = false;
  }

  double composite_persistent_multiplier( const action_state_t *state ) const override
  {
    double cpm = monk_spell_t::composite_persistent_multiplier( state );

    if ( auto cs = debug_cast<const custom_state_t *>( state ); cs && cs->blackout_combo )
      cpm *= 1.0 + p()->buff.blackout_combo->data().effectN( 5 ).percent();

    return cpm;
  }

  action_state_t *new_state() override
  {
    return new custom_state_t( this, target );
  }
};

struct breath_of_fire_t : public monk_spell_t
{
protected:
  using custom_state_t = breath_of_fire_state_t;

public:
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
  bool blackout_combo;

  breath_of_fire_t( monk_t *player, std::string_view options_str )
    : monk_spell_t( player, "breath_of_fire", player->talent.brewmaster.breath_of_fire ),
      dragonfire_brew( nullptr ),
      no_bof_hit( false ),
      blackout_combo( false )
  {
    add_option( opt_bool( "no_bof_hit", no_bof_hit ) );
    parse_options( options_str );

    aoe                 = -1;
    reduced_aoe_targets = 1.0;
    full_amount_targets = 1;
    cast_during_sck     = true;

    if ( player->talent.brewmaster.dragonfire_brew->ok() )
      dragonfire_brew = new dragonfire_brew_t( player );

    add_child( player->action.breath_of_fire );
    if ( dragonfire_brew )
      add_child( dragonfire_brew );
  }

  action_state_t *new_state() override
  {
    return new custom_state_t( this, target );
  }

  void execute() override
  {
    p()->buff.charred_passions->trigger();

    if ( no_bof_hit )
      return;

    if ( dragonfire_brew )
      dragonfire_brew->execute();

    monk_spell_t::execute();

    p()->action.flurry_strikes->execute( attacks::flurry_strikes_t::WISDOM_OF_THE_WALL );
  }

  void impact( action_state_t *state ) override
  {
    monk_spell_t::impact( state );

    propagate_const<action_t *> dot = p()->action.breath_of_fire;

    auto dot_state    = debug_cast<custom_state_t *>( dot->get_state() );
    dot_state->target = state->target;

    // blackout combo buffs only one of the breath of fire dot applications from
    // a single cast
    if ( get_td( dot_state->target )->debuff.keg_smash->up() && blackout_combo )
    {
      dot_state->blackout_combo = true;
      blackout_combo            = false;
    }

    dot->snapshot_state( dot_state, dot->amount_type( dot_state ) );
    dot->schedule_execute( dot_state );
  }
};

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

  fortifying_brew_t( monk_t *p, std::string_view options_str )
    : brew_t<monk_spell_t>( p, "fortifying_brew", p->talent.monk.fortifying_brew.find_override_spell() ),
      absorb( p->talent.conduit_of_the_celestials.niuzaos_protection->ok() ? new niuzaos_protection_t( p ) : nullptr )
  {
    cast_during_sck = true;

    parse_options( options_str );

    harmful = may_crit = false;
  }

  void execute() override
  {
    brew_t<monk_spell_t>::execute();

    if ( p()->talent.conduit_of_the_celestials.niuzaos_protection->ok() )
      absorb->execute();
    p()->buff.fortifying_brew->trigger();
    p()->buff.pretense_of_instability->trigger();
    p()->action.special_delivery->execute();
  }
};

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
  exploding_keg_t( monk_t *p, std::string_view options_str )
    : monk_spell_t( p, "exploding_keg", p->talent.brewmaster.exploding_keg )
  {
    parse_options( options_str );
    cast_during_sck = true;
    aoe             = -1;
    add_child( p->action.exploding_keg );
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

  purifying_brew_t( monk_t *player, std::string_view options_str )
    : brew_t<monk_spell_t>( player, "purifying_brew", player->talent.brewmaster.purifying_brew ),
      gai_plins( new gai_plins_imperial_brew_t( player ) )
  {
    parse_options( options_str );

    harmful         = false;
    cast_during_sck = true;
    use_off_gcd     = true;
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
    p()->action.special_delivery->execute();

    p()->buff.ox_stance->trigger();
    p()->buff.aspect_of_harmony.trigger_flat(
        p()->talent.master_of_harmony.clarity_of_purpose->effectN( 1 ).percent() *
        ( 1.0 + p()->composite_damage_versatility() ) *
        p()->composite_total_attack_power_by_type( attack_power_type::WEAPON_MAINHAND ) );

    double pool_size      = p()->find_stagger( "Stagger" )->pool_size();
    double purify_percent = data().effectN( 1 ).percent();
    double purify_amount  = std::max( pool_size * purify_percent, p()->max_health() * data().effectN( 2 ).percent() );
    double cleared        = p()->find_stagger( "Stagger" )->purify_flat( purify_amount, "purifying_brew" );

    double healed = cleared * p()->talent.brewmaster.gai_plins_imperial_brew->effectN( 1 ).percent();
    if ( healed )
    {
      gai_plins->base_dd_min = gai_plins->base_dd_max = healed;
      gai_plins->target                               = p();
      gai_plins->execute();
    }
  }
};

struct courage_of_the_white_tiger_t : conduit_of_the_celestials_container_t
{
  enum cotwt_source_e
  {
    BASE,
    CELESTIAL
  };

  struct impact_t : monk_melee_attack_t
  {
    struct heal_t : monk_heal_t
    {
      template <typename... Args>
      heal_t( monk_t *player, std::string_view name )
        : monk_heal_t( player, fmt::format( "{}_heal", name ),
                       player->talent.conduit_of_the_celestials.courage_of_the_white_tiger_heal )
      {
        background = dual = true;
        target            = player;
        base_dd_min = base_dd_max = 1.0;
      }
    };

    cotwt_source_e source;
    action_t *heal;

    impact_t( monk_t *player, std::string_view name, cotwt_source_e source )
      : monk_melee_attack_t( player, name, player->talent.conduit_of_the_celestials.courage_of_the_white_tiger_damage ),
        source( source ),
        heal( new heal_t( player, name ) )
    {
      background     = true;
      time_to_travel = 2_s;

      if ( source == CELESTIAL )
        if ( const auto &effect = player->talent.conduit_of_the_celestials.unity_within_dmg_mult->effectN( 1 );
             effect.ok() )
          add_parse_entry( da_multiplier_effects ).set_value( effect.percent() - 1.0 ).set_eff( &effect );
    }

    void execute() override
    {
      monk_melee_attack_t::execute();

      if ( source == BASE )
      {
        p()->buff.strength_of_the_black_ox->trigger();
        p()->buff.inner_compass_tiger_stance->trigger();
        p()->buff.courage_of_the_white_tiger->expire();
      }
    }

    void impact( action_state_t *state ) override
    {
      monk_melee_attack_t::impact( state );

      double result = state->result_total;
      result *= p()->talent.conduit_of_the_celestials.courage_of_the_white_tiger->effectN( 2 ).percent();
      heal->base_dd_min = heal->base_dd_max = result;
      heal->execute();
    }
  };

  courage_of_the_white_tiger_t( monk_t *player ) : conduit_of_the_celestials_container_t( player )
  {
    base      = new impact_t( player, "courage_of_the_white_tiger", BASE );
    celestial = new impact_t( player, "courage_of_the_white_tiger_celestial", CELESTIAL );
  }
};

struct xuen_summon_t : public monk_spell_t
{
  xuen_summon_t( monk_t *player, std::string_view options_str )
    : monk_spell_t( player, "invoke_xuen_the_white_tiger",
                    player->talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger )
  {
    parse_options( options_str );

    cast_during_sck = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->bugs )
      for ( auto target : p()->sim->target_non_sleeping_list )
        if ( auto *td = p()->get_target_data( target ); td )
          td->debuff.empowered_tiger_lightning->current_value = 0;

    p()->pets.xuen.spawn( p()->talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger->duration(), 1 );
    p()->buff.invoke_xuen->trigger();
    p()->buff.flurry_of_xuen->trigger();
    p()->buff.courage_of_the_white_tiger->trigger();
    p()->buff.celestial_conduit->trigger();
  }
};

struct empowered_tiger_lightning_t : public monk_spell_t
{
  empowered_tiger_lightning_t( monk_t *player )
    : monk_spell_t( player, "empowered_tiger_lightning", player->baseline.windwalker.empowered_tiger_lightning_damage )
  {
    background = true;
  }
};

struct flurry_of_xuen_t : public monk_spell_t
{
  flurry_of_xuen_t( monk_t *player )
    : monk_spell_t( player, "flurry_of_xuen", player->talent.windwalker.flurry_of_xuen_driver->effectN( 1 ).trigger() )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = player->talent.windwalker.flurry_of_xuen->effectN( 2 ).base_value();
  }
};

struct strength_of_the_black_ox_t : conduit_of_the_celestials_container_t
{
  enum sotbo_source_e
  {
    BASE,
    CELESTIAL
  };

  template <class base_action_t, sotbo_source_e source_effect>
  struct impact_t : base_action_t
  {
    sotbo_source_e source;

    template <typename... Args>
    impact_t( monk_t *player, Args &&...args ) : base_action_t( player, std::forward<Args>( args )... )
    {
      source                    = source_effect;
      base_action_t::background = true;

      if constexpr ( std::is_same_v<monk_absorb_t, base_action_t> )
      {
        base_action_t::aoe         = as<int>( base_action_t::data().effectN( 3 ).base_value() );
        base_action_t::base_dd_min = base_action_t::base_dd_max =
            player->max_health() * base_action_t::data().effectN( 2 ).percent();
      }

      if constexpr ( std::is_same_v<monk_spell_t, base_action_t> )
      {
        base_action_t::aoe = -1;
        base_action_t::reduced_aoe_targets =
            player->talent.conduit_of_the_celestials.strength_of_the_black_ox->effectN( 2 ).base_value();
      }

      if ( source == CELESTIAL )
        if ( const auto &effect = player->talent.conduit_of_the_celestials.unity_within_dmg_mult->effectN( 1 );
             effect.ok() )
          add_parse_entry( base_action_t::da_multiplier_effects )
              .set_value( effect.percent() - 1.0 )
              .set_eff( &effect );
    }

    void execute() override
    {
      base_action_t::execute();

      if ( source == BASE )
      {
        base_action_t::p()->buff.strength_of_the_black_ox->expire();
        base_action_t::p()->buff.inner_compass_ox_stance->trigger();
      }

      if ( base_action_t::p()->specialization() == MONK_WINDWALKER )
        base_action_t::p()->buff.teachings_of_the_monastery->trigger(
            as<int>( base_action_t::p()
                         ->talent.conduit_of_the_celestials.strength_of_the_black_ox->effectN( 3 )
                         .base_value() ) );
    }
  };

  strength_of_the_black_ox_t( monk_t *player ) : conduit_of_the_celestials_container_t( player )
  {
    base      = new impact_t<monk_spell_t, BASE>( player, "strength_of_the_black_ox_dmg",
                                                  player->talent.conduit_of_the_celestials.strength_of_the_black_ox_damage );
    celestial = new impact_t<monk_spell_t, CELESTIAL>(
        player, "strength_of_the_black_ox_celestial_dmg",
        player->talent.conduit_of_the_celestials.strength_of_the_black_ox_damage );
  }
};

struct niuzao_spell_t : public monk_spell_t
{
  niuzao_spell_t( monk_t *p, std::string_view options_str )
    : monk_spell_t( p, "invoke_niuzao_the_black_ox", p->talent.brewmaster.invoke_niuzao_the_black_ox )
  {
    parse_options( options_str );

    cast_during_sck = true;
    // Specifically set for 10.1 class trinket
    harmful = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd = timespan_t::from_millis( 750 );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.niuzao.spawn( p()->talent.brewmaster.invoke_niuzao_the_black_ox->duration(), 1 );
    p()->buff.invoke_niuzao->trigger();
    p()->buff.stand_ready->trigger();
  }
};

struct unity_within_t : public monk_spell_t
{
  unity_within_t( monk_t *p, std::string_view options_str )
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

struct celestial_conduit_t : public monk_spell_t
{
  template <typename TBase>
  struct tick_action_t : TBase
  {
    tick_action_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
      : TBase( player, name, spell_data )
    {
      TBase::background = true;

      if constexpr ( std::is_same_v<TBase, monk_spell_t> )
      {
        TBase::aoe              = -1;
        TBase::split_aoe_damage = true;
        TBase::ww_mastery       = true;
      }

      if constexpr ( std::is_same_v<TBase, monk_heal_t> )
        TBase::target = player;
    }

    double composite_aoe_multiplier( const action_state_t *state ) const override
    {
      double cam = TBase::composite_aoe_multiplier( state );

      if ( state->n_targets )
        cam *=
            1 +
            ( TBase::p()->talent.conduit_of_the_celestials.celestial_conduit_action->effectN( 1 ).percent() *
              std::min(
                  as<double>( state->n_targets ),
                  TBase::p()->talent.conduit_of_the_celestials.celestial_conduit_action->effectN( 3 ).base_value() ) );

      return cam;
    }
  };

  action_t *damage;
  action_t *heal;

  celestial_conduit_t( monk_t *player, std::string_view options_str )
    : monk_spell_t( player, "celestial_conduit", player->talent.conduit_of_the_celestials.celestial_conduit_action ),
      damage( new tick_action_t<monk_spell_t>( player, "celestial_conduit_damage",
                                               player->talent.conduit_of_the_celestials.celestial_conduit_damage ) ),
      heal( new tick_action_t<monk_heal_t>( player, "celestial_conduit_heal",
                                            player->talent.conduit_of_the_celestials.celestial_conduit_heal ) )
  {
    parse_options( options_str );

    may_combo_strike      = true;
    channeled             = true;
    interrupt_auto_attack = false;

    tick_action = damage;
  }

  bool ready() override
  {
    return p()->talent.conduit_of_the_celestials.celestial_conduit->ok() && p()->buff.celestial_conduit->check() &&
           monk_spell_t::ready();
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

struct zenith_t : public monk_spell_t
{
  struct zenith_stomp_t : public monk_spell_t
  {
    zenith_stomp_t( monk_t *player ) : monk_spell_t( player, "zenith_stomp", player->talent.monk.zenith_stomp_damage )
    {
      aoe                 = -1;
      reduced_aoe_targets = player->talent.monk.zenith_stomp->effectN( 1 ).base_value();
    }
  };

  action_t *zenith_stomp;

  zenith_t( monk_t *player, std::string_view options_str )
    : monk_spell_t( player, "zenith", player->talent.windwalker.zenith ), zenith_stomp( nullptr )
  {
    parse_options( options_str );

    if ( player->talent.monk.zenith_stomp->ok() )
    {
      zenith_stomp = new zenith_stomp_t( player );
      add_child( zenith_stomp );
    }
  }

  void execute() override
  {
    p()->buff.heart_of_the_jade_serpent_yulons_avatar->trigger();

    monk_spell_t::execute();

    if ( zenith_stomp )
      zenith_stomp->execute_on_target( target );

    p()->buff.zenith->trigger();
    p()->cooldown.rising_sun_kick->reset( true );
    p()->buff.stand_ready->trigger();
  }
};
}  // namespace spells

namespace heals
{
struct vivify_t : public monk_heal_t
{
  vivify_t( monk_t *p, std::string_view options_str ) : monk_heal_t( p, "vivify", p->baseline.monk.vivify )
  {
    parse_options( options_str );

    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    base_execute_time += p->talent.monk.vivacious_vivification->effectN( 1 ).time_value();

    cast_during_sck = false;
  }

  void execute() override
  {
    monk_heal_t::execute();

    p()->action.chi_wave->execute();
  }
};

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
    cast_during_sck  = true;
    if ( player->talent.windwalker.combat_wisdom->ok() )
      background = true;

    add_child( damage );
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();
    if ( !p()->talent.monk.strength_of_spirit->ok() )
      return am;
    // TODO: convert to parse_effects
    am *=
        1.0 + ( 1.0 - p()->health_percentage() / 100.0 ) * p()->talent.monk.strength_of_spirit->effectN( 1 ).percent();
    return am;
  }

  void execute() override
  {
    if ( p()->talent.brewmaster.gift_of_the_ox->ok() )
      p()->buff.expel_harm_accumulator->trigger();

    monk_heal_t::execute();

    if ( p()->talent.brewmaster.tranquil_spirit->ok() )
    {
      double percent = p()->talent.brewmaster.tranquil_spirit->effectN( 1 ).percent();
      p()->find_stagger( "Stagger" )->purify_percent( percent, "tranquil_spirit_eh" );
      p()->proc.tranquil_spirit_expel_harm->occur();
    }
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
struct absorb_brew_t : public brew_t<monk_absorb_t>
{
  absorb_brew_t( monk_t *player, std::string_view options_str, std::string_view name, const spell_data_t *spell_data )
    : brew_t<monk_absorb_t>( player, name, spell_data )
  {
    parse_options( options_str );
    cast_during_sck = true;
  }

  void execute() override
  {
    p()->buff.aspect_of_harmony.trigger_spend();

    brew_t<monk_absorb_t>::execute();

    p()->buff.pretense_of_instability->trigger();
    p()->action.special_delivery->execute();
  }
};

struct celestial_brew_t : public absorb_brew_t
{
  celestial_brew_t( monk_t *player, std::string_view options_str )
    : absorb_brew_t( player, options_str, "celestial_brew", player->talent.brewmaster.celestial_brew )
  {
  }
};

struct celestial_infusion_t : public absorb_brew_t
{
  celestial_infusion_t( monk_t *player, std::string_view options_str )
    : absorb_brew_t( player, options_str, "celestial_infusion", player->talent.brewmaster.celestial_infusion )
  {
  }

  absorb_buff_t *create_buff( const action_state_t *state ) override
  {
    buff_t *b = buff_t::find( state->target, name_str, player );
    if ( b )
      return debug_cast<buffs::fractional_absorb_t *>( b );

    auto buff = make_buff<buffs::fractional_absorb_t>( p(), name_str, &data() );
    buff->set_absorb_fraction( data().effectN( 2 ).percent() );
    buff->set_absorb_source( stats );

    return buff;
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
}  // namespace actions

namespace buffs
{
template <typename Base>
monk_buff_t<Base>::monk_buff_t( monk_t *player, std::string_view name, const spell_data_t *spell_data,
                                const item_t *item )
  : base_t( player, name, spell_data, item )
{
}

template <typename Base>
monk_buff_t<Base>::monk_buff_t( monk_td_t *target_data, std::string_view name, const spell_data_t *spell_data,
                                const item_t *item )
  : base_t( *target_data, name, spell_data, item )
{
}

template <typename Base>
monk_td_t &monk_buff_t<Base>::get_td( player_t *t )
{
  return *( base_t::p().get_target_data( t ) );
}

template <typename Base>
const monk_td_t *monk_buff_t<Base>::find_td( player_t *t ) const
{
  return base_t::p().find_target_data( t );
}

template <typename Base>
monk_t &monk_buff_t<Base>::p()
{
  return *debug_cast<monk_t *>( base_t::source );
}

template <typename Base>
const monk_t &monk_buff_t<Base>::p() const
{
  return *debug_cast<monk_t *>( base_t::source );
}

gift_of_the_ox_t::gift_of_the_ox_t( monk_t *player )
  : monk_buff_t<>( player, "gift_of_the_ox", player->talent.brewmaster.gift_of_the_ox_buff ),
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

shuffle_t::shuffle_t( monk_t *player )
  : monk_buff_t<>( player, "shuffle", player->talent.brewmaster.shuffle_buff ),
    accumulator( 0_s ),
    max_duration( 3.0 * base_buff_duration )
{
  set_trigger_spell( player->talent.brewmaster.shuffle );
}

void shuffle_t::trigger( timespan_t duration )
{
  if ( !p().talent.brewmaster.shuffle->ok() )
    return;

  accumulator += duration;

  duration = std::min( duration + remains(), max_duration );
  monk_buff_t::extend_duration_or_trigger( duration );

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

struct fortifying_brew_t : public monk_buff_t<>
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

struct touch_of_karma_buff_t : public monk_buff_t<>
{
  touch_of_karma_buff_t( monk_t *p, std::string_view n, const spell_data_t *s ) : monk_buff_t( p, n, s )
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

struct whirling_dragon_punch_buff_t : monk_buff_t<>
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

struct rushing_jade_wind_buff_t : public monk_buff_t<>
{
  struct tick_action_t : actions::monk_melee_attack_t
  {
    tick_action_t( monk_t *p )
      : monk_melee_attack_t( p, "rushing_jade_wind_tick", p->talent.shared_spell.rushing_jade_wind_tick )
    {
      ww_mastery = true;

      dual = background   = true;
      aoe                 = -1;
      reduced_aoe_targets = p->talent.shared_spell.rushing_jade_wind_buff->effectN( 1 ).base_value();

      // Merge action statistics if RJW exists as an active ability
      if ( const action_t *action = p->find_action( "rushing_jade_wind" ); action )
        stats = action->stats;
    }
  };

  timespan_t _period;
  action_t *rushing_jade_wind_tick;

  rushing_jade_wind_buff_t( monk_t *player )
    : monk_buff_t( player, "rushing_jade_wind", player->talent.shared_spell.rushing_jade_wind_buff ),
      rushing_jade_wind_tick( nullptr )
  {
    set_tick_time_behavior( buff_tick_time_behavior::CUSTOM );
    set_tick_time_callback( [ this ]( const buff_t *, unsigned int ) { return _period; } );

    set_tick_callback( [ this ]( buff_t *, int, timespan_t ) {
      if ( rushing_jade_wind_tick )
      {
        rushing_jade_wind_tick->execute();
        return;
      }

      if ( action_t *rjw = p().find_action( "rushing_jade_wind_tick" ); rjw )
      {
        rushing_jade_wind_tick = rjw;
        rjw->execute();
      }
    } );
    set_tick_behavior( buff_tick_behavior::REFRESH );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
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

struct invoke_xuen_the_white_tiger_buff_t : public monk_buff_t<>
{
  double multiplier;
  action_t *etl;

  invoke_xuen_the_white_tiger_buff_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
    : monk_buff_t( player, name, spell_data ), multiplier( 0.0 ), etl( nullptr )
  {
    set_cooldown( timespan_t::zero() );
    set_period( spell_data->effectN( 2 ).period() );

    if ( !player->baseline.windwalker.empowered_tiger_lightning->ok() )
      return;

    multiplier = player->baseline.windwalker.empowered_tiger_lightning->effectN( 2 ).percent();
    // defer etl action lookup until callback invocation, as (background) actions
    // are not yet constructed (see: sim_t::init_actor())

    set_tick_callback( [ & ]( buff_t *, int, timespan_t ) {
      if ( !etl )
        etl = p().find_action( "empowered_tiger_lightning" );

      for ( player_t *target : p().sim->target_non_sleeping_list )
        if ( monk_td_t *target_data = p().get_target_data( target ); target_data )
          if ( buff_t *debuff = target_data->debuff.empowered_tiger_lightning; debuff && debuff->up() )
            if ( double value = debuff->check_value(); value )
            {
              etl->base_dd_min = etl->base_dd_max = value * multiplier;
              etl->execute_on_target( target );
              debuff->expire();
            }
    } );
  }
};

empowered_tiger_lightning_t::empowered_tiger_lightning_t( monk_td_t &target_data )
  : monk_buff_t( &target_data, "empowered_tiger_lightning", spell_data_t::nil() )
{
  set_quiet( true );
  set_trigger_spell( target_data.monk.baseline.windwalker.empowered_tiger_lightning );
  set_duration( timespan_t::from_seconds(
                    target_data.monk.baseline.windwalker.empowered_tiger_lightning->effectN( 1 ).base_value() ) +
                1_ms );  // add small buffer to avoid expiring before tick action
  set_default_value( 0.0 );
}

bool empowered_tiger_lightning_t::trigger( const action_state_t *state )
{
  const std::array<unsigned, 4> blacklist = {
      p().baseline.monk.touch_of_death->id(),
      p().baseline.windwalker.empowered_tiger_lightning_damage->id(),
      p().baseline.windwalker.touch_of_karma_tick->id(),
      p().talent.windwalker.skyfire_heel_damage->id(),
  };

  if ( action_t::result_is_miss( state->result ) )
    return false;

  if ( !state->result_amount )
    return false;

  if ( !p().buff.invoke_xuen->check() )
    return false;

  if ( range::contains( blacklist, state->action->id ) )
    return false;

  switch ( state->result_type )
  {
    case result_amount_type::DMG_DIRECT:
    case result_amount_type::DMG_OVER_TIME:
      if ( check() )
        current_value += state->result_amount;
      else
        trigger( 1, state->result_amount );
      return true;
    default:
      return false;
  }
}

struct touch_of_death_ww_buff_t : public monk_buff_t<>
{
  // This buff is set up so that it provides the chi from
  // Windwalker's Touch of Death Rank 2. In-game, applying Touch of Death will spawn
  // three chi orbs that the player can pick up whenever they do not have max
  // Chi. Given we want to provide the chi but apply it slowly if the player is at
  // max chi, then we need to set up so that it triggers on it's own.
  touch_of_death_ww_buff_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
    : monk_buff_t( player, name, spell_data )
  {
    set_can_cancel( false );
    set_quiet( true );
    set_reverse( true );
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( player->baseline.windwalker.touch_of_death_rank_3 );

    // TODO: find actual orb duration
    set_duration( timespan_t::from_minutes( 3 ) );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_zero( true );

    set_max_stack( as<int>( player->baseline.windwalker.touch_of_death_rank_3->effectN( 1 ).base_value() ) );
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
                          player->talent.master_of_harmony.path_of_resurgence->effectN( 1 ).trigger() );

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

  damage = new spender_t::tick_t<actions::monk_spell_t>( player, "aspect_of_harmony_damage",
                                                         player->talent.master_of_harmony.aspect_of_harmony_damage );
  heal   = new spender_t::tick_t<actions::monk_heal_t>( player, "aspect_of_harmony_heal",
                                                        player->talent.master_of_harmony.aspect_of_harmony_heal );

  purified_spirit = new spender_t::purified_spirit_t<actions::monk_spell_t>(
      player, player->talent.master_of_harmony.purified_spirit_damage, this );
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
  sim->print_debug( "AoH does_gen: {} {}", state->action->name(), state->action->id );
  monk_buff_t::trigger( -1, amount );
}

aspect_of_harmony_t::spender_t::spender_t( monk_t *player, aspect_of_harmony_t *aspect_of_harmony )
  : monk_buff_t( player, "aspect_of_harmony_spender", player->talent.master_of_harmony.aspect_of_harmony_spender ),
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
  pool          = 0.0;
  current_value = 0.0;
  sim->print_debug( "Aspect of Harmony =P: 0" );
}

bool aspect_of_harmony_t::spender_t::trigger( int stacks, double, double chance, timespan_t duration )
{
  pool = aspect_of_harmony->accumulator->check_value();
  aspect_of_harmony->accumulator->expire();

  sim->print_debug( "Aspect of Harmony +P: {}", current_value );
  return monk_buff_t::trigger( stacks, pool, chance, duration );
}

void aspect_of_harmony_t::spender_t::trigger_with_state( action_state_t *state )
{
  const auto whitelist = { p().baseline.monk.expel_harm->id(), p().baseline.monk.vivify->id(),
                           p().baseline.monk.blackout_kick->id(), p().baseline.monk.tiger_palm->id() };

  auto in_hg_whitelist = [ whitelist, id = state->action->id, this ]() {
    return p().talent.master_of_harmony.harmonic_gambit->ok() &&
           std::find( whitelist.begin(), whitelist.end(), id ) != whitelist.end();
  };

  double multiplier = p().talent.master_of_harmony.aspect_of_harmony->effectN( 6 ).percent();
  double amount     = std::min( state->result_amount * multiplier, current_value );

  action_t *spend_target = nullptr;
  switch ( state->result_type )
  {
    case result_amount_type::DMG_DIRECT:
    case result_amount_type::DMG_OVER_TIME:
      if ( p().specialization() == MONK_BREWMASTER || in_hg_whitelist() )
        spend_target = aspect_of_harmony->damage;
      break;
    case result_amount_type::HEAL_DIRECT:
    case result_amount_type::HEAL_OVER_TIME:
      if ( in_hg_whitelist() )
        spend_target = aspect_of_harmony->heal;
      break;
    default:
      break;
  }

  double bonus = 0.0;
  if ( spend_target )
  {
    current_value -= amount;

    // approximation of coalescence intensification mechanic
    dot_t *dot = spend_target->get_dot( state->target );
    if ( dot && dot->state && dot->is_ticking() )
      bonus = std::min( spend_target->base_ta( dot->state ) * dot->ticks_left() * 0.5, current_value );
    current_value -= bonus;

    sim->print_debug( "AoH does_spend: {} {}", state->action->name(), state->action->id );
    sim->print_debug( "Aspect of Harmony -P: {}, P: {}, T: {}, A: {}, B: {}", amount + bonus,
                      current_value + amount + bonus, current_value, amount, bonus );

    residual_action::trigger( spend_target, state->target, amount + bonus );
  }

  if ( current_value == 0.0 )
  {
    expire();
  }
}

template <class base_action_t>
aspect_of_harmony_t::spender_t::purified_spirit_t<base_action_t>::purified_spirit_t(
    monk_t *player, const spell_data_t *spell_data, aspect_of_harmony_t *aspect_of_harmony )
  : base_action_t( player, "purified_spirit", spell_data ), aspect_of_harmony( aspect_of_harmony )
{
  base_action_t::aoe = -1;
  // base_action_t::split_aoe_damage = true; this does not work for dots
  base_action_t::dot_behavior = DOT_CLIP;
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
  base_action_t::base_td = aspect_of_harmony->spender->current_value / 4.0 / as<double>( base_action_t::num_targets() );
  base_action_t::sim->print_debug( "Purified Spirit consuming rest of pool. Pool: {} TA: {}",
                                   aspect_of_harmony->spender->current_value,
                                   aspect_of_harmony->spender->current_value / 4.0 );
  aspect_of_harmony->spender->current_value = 0.0;
  if ( base_action_t::base_td > 0.0 )
    base_action_t::execute();
}

template <class base_action_t>
aspect_of_harmony_t::spender_t::tick_t<base_action_t>::tick_t( monk_t *player, std::string_view name,
                                                               const spell_data_t *spell_data )
  : residual_action::residual_periodic_action_t<base_action_t>( player, name, spell_data )
{
}

fractional_absorb_t::fractional_absorb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data )
  : monk_buff_t<absorb_buff_t>( player, name, spell_data ), absorb_fraction( 1.0 )
{
}

double fractional_absorb_t::consume( double amount, action_state_t *state )
{
  return base_t::consume( amount * absorb_fraction, state );
}

absorb_buff_t *fractional_absorb_t::set_absorb_fraction( double fraction )
{
  absorb_fraction = fraction;
  return this;
}
}  // namespace buffs
}  // end namespace monk

namespace monk
{
monk_td_t::monk_td_t( player_t *target, monk_t *player )
  : actor_target_data_t( target, player ), dot(), debuff(), monk( *player )
{
  // Windwalker
  debuff.empowered_tiger_lightning = make_buff_fallback<buffs::empowered_tiger_lightning_t>(
      player->baseline.windwalker.empowered_tiger_lightning->ok(), *this, "empowered_tiger_lightning" );

  // Brewmaster
  debuff.keg_smash = make_buff_fallback( player->talent.brewmaster.keg_smash->ok(), *this, "keg_smash",
                                         player->talent.brewmaster.keg_smash )
                         ->set_cooldown( timespan_t::zero() )
                         ->set_default_value_from_effect( 3 );

  debuff.exploding_keg = make_buff_fallback( player->talent.brewmaster.exploding_keg->ok(), *this,
                                             "exploding_keg_debuff", player->talent.brewmaster.exploding_keg )
                             ->set_trigger_spell( player->talent.brewmaster.exploding_keg )
                             ->set_cooldown( timespan_t::zero() );

  // Shado-Pan

  debuff.high_impact = make_buff_fallback( player->talent.shado_pan.high_impact->ok(), *this, "high_impact",
                                           player->talent.shado_pan.high_impact_debuff )
                           ->set_trigger_spell( player->talent.shado_pan.high_impact )
                           ->set_quiet( true );

  dot.breath_of_fire               = target->get_dot( "breath_of_fire_dot", player );
  dot.crackling_jade_lightning_aoe = target->get_dot( "crackling_jade_lightning_aoe", player );
  dot.aspect_of_harmony            = target->get_dot( "aspect_of_harmony_damage", player );
}

monk_t::monk_t( sim_t *sim, std::string_view name, race_e r )
  : base_t( sim, MONK, name, r ),
    action(),
    buff(),
    gain(),
    proc(),
    cooldown(),
    baseline(),
    talent(),
    tier(),
    pets( this ),
    user_options( options_t() )
{
  cooldown.anvil_and_stave = get_cooldown( "anvil_and_stave" );
  cooldown.blackout_kick   = get_cooldown( "blackout_kick" );
  cooldown.fists_of_fury   = get_cooldown( "fists_of_fury" );
  cooldown.rising_sun_kick = get_cooldown( "rising_sun_kick" );

  resource_regeneration              = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ]        = true;
  regen_caches[ CACHE_ATTACK_HASTE ] = true;
  user_options.initial_chi =
      talent.windwalker.combat_wisdom.ok() ? (int)talent.windwalker.combat_wisdom->effectN( 1 ).base_value() : 0;
  user_options.chi_burst_healing_targets = 8;
}

bool monk_t::wowv_l( wowv_t value ) const
{
  return sim->dbc->wowv() < value;
}

bool monk_t::wowv_ge( wowv_t value ) const
{
  return sim->dbc->wowv() >= value;
}

void monk_t::parse_player_effects()
{
  /*
   * Permanent actor-specific effects go here.
   * Make sure to use a specific `find_spell` method (i.e. `find_specialization_spell`)
   * for all of these spells or they will be applied to actors of the incorrect spec.
   */

  // class and spec shared auras

  // brewmaster player auras

  // mistweaver player auras

  // windwalker player auras
  parse_effects( buff.hit_combo, effect_mask_t( true ).disable( 4 ) );

  // class talent auras

  // brewmaster talent auras
  parse_effects( buff.pretense_of_instability );
  parse_effects( talent.brewmaster.heart_of_the_ox, [ & ]( double value ) {
    if ( buff.invoke_niuzao->check() )
      // value not available in spell data :(
      value *= 1.0 + talent.brewmaster.invoke_niuzao_the_black_ox->effectN( 6 ).percent();
    return value;
  } );

  // windwalker talent auras
  parse_effects( buff.memory_of_the_monastery );
  parse_effects( buff.momentum_boost_speed );
  parse_effects( talent.windwalker.ferociousness, [ & ]( double value ) {
    if ( buff.invoke_xuen->check() )
      value *= 1.0 + talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger->effectN( 3 ).percent();
    return value;
  } );
  parse_effects( talent.windwalker.martial_agility, [ & ]( double value ) {
    if ( buff.zenith->check() )
      return talent.windwalker.martial_agility->effectN( 3 ).percent();
    return value;
  } );

  // Shadopan

  // Conduit of the Celestials
  parse_effects( buff.inner_compass_crane_stance );
  parse_effects( buff.inner_compass_ox_stance );
  parse_effects( buff.inner_compass_serpent_stance );
  parse_effects( buff.inner_compass_tiger_stance );

  effect_mask_t em = talent.conduit_of_the_celestials.flowing_wisdom->ok() ? effect_mask_t( true )
                                                                           : effect_mask_t( true ).disable( 8 );
  parse_effects( buff.heart_of_the_jade_serpent, em,
                 [ & ] { return !buff.heart_of_the_jade_serpent_yulons_avatar->check(); } );
  parse_effects( buff.heart_of_the_jade_serpent_yulons_avatar, em );
  parse_effects( buff.heart_of_the_jade_serpent_unity_within, em );

  // Midnight S1 Set Effects
  // Midnight S2 Set Effects
  // Midnight S3 Set Effects
}

action_t *monk_t::create_action( std::string_view name, std::string_view options_str )
{
  using namespace actions;
  using namespace actions::attacks;
  using namespace actions::spells;
  using namespace actions::heals;
  using namespace actions::absorbs;
  // Monk
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
  if ( name == "fortifying_brew" )
    return new fortifying_brew_t( this, options_str );
  if ( name == "provoke" )
    return new provoke_t( this, options_str );
  if ( name == "chi_torpedo" )
    return new chi_torpedo_t( this, options_str );
  if ( name == "touch_of_death" )
    return new touch_of_death_t( this, options_str );

  // Brewmaster
  if ( name == "breath_of_fire" )
    return new breath_of_fire_t( this, options_str );
  if ( name == "celestial_brew" && talent.brewmaster.celestial_infusion->ok() )
    return new celestial_infusion_t( this, options_str );
  if ( name == "celestial_brew" )
    return new celestial_brew_t( this, options_str );
  if ( name == "celestial_infusion" )
    return new celestial_infusion_t( this, options_str );
  if ( name == "exploding_keg" )
    return new exploding_keg_t( this, options_str );
  if ( name == "invoke_niuzao" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "invoke_niuzao_the_black_ox" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "keg_smash" )
    return new press_the_advantage_t<keg_smash_t>( this, options_str );
  if ( name == "purifying_brew" )
    return new purifying_brew_t( this, options_str );
  if ( name == "chi_burst" )
    return new chi_burst_t( this, options_str );
  if ( name == "black_ox_brew" )
    return new black_ox_brew_t( this, options_str );

  // Windwalker
  if ( name == "fists_of_fury" )
    return new fists_of_fury_t( this, options_str );
  if ( name == "flying_serpent_kick" )
    return new flying_serpent_kick_t( this, options_str );
  if ( name == "slicing_winds" )
    return new slicing_winds_t( this, options_str );
  if ( name == "touch_of_karma" )
    return new touch_of_karma_t( this, options_str );
  if ( name == "strike_of_the_windlord" )
    return new strike_of_the_windlord_t( this, options_str );
  if ( name == "invoke_xuen" )
    return new xuen_summon_t( this, options_str );
  if ( name == "invoke_xuen_the_white_tiger" )
    return new xuen_summon_t( this, options_str );
  if ( name == "rushing_jade_wind" )
    return new rushing_jade_wind_t( this, options_str );
  if ( name == "whirling_dragon_punch" )
    return new whirling_dragon_punch_t( this, options_str );
  if ( name == "zenith" )
    return new zenith_t( this, options_str );

  // Conduit of the Celestials
  if ( name == "celestial_conduit" )
    return new celestial_conduit_t( this, options_str );
  if ( name == "unity_within" )
    return new unity_within_t( this, options_str );

  return base_t::create_action( name, options_str );
}

void monk_t::trigger_celestial_fortune( action_state_t *s )
{
  if ( !baseline.brewmaster.celestial_fortune->ok() || s->action == action.celestial_fortune || s->result_raw == 0.0 )
  {
    return;
  }
  switch ( s->action->id )
  {
    case 143924:  // Leech
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
  if ( action.celestial_fortune && rng().roll( composite_melee_crit_chance() ) )
  {
    sim->print_debug( "triggering celestial fortune from (id: {}, name: {}) with (amount: {}) damage base",
                      s->action->id, s->action->name_str, s->result_amount );
    action.celestial_fortune->base_dd_max = action.celestial_fortune->base_dd_min = s->result_amount;
    action.celestial_fortune->schedule_execute();
  }
}

bool monk_t::validate_actor()
{
  if ( specialization() == MONK_MISTWEAVER )
  {
    if ( !quiet )
      sim->error( "Mistweaver Monk for {} is not currently supported.", *this );
    return false;
  }

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->error( "{} has no weapon equipped at the Main-Hand slot.", *this );
    return false;
  }

  if ( main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim->error( "{} both a 1-hand and 2-hand weapon equipped at once.", *this );
    return false;
  }

  if ( specialization() == MONK_WINDWALKER && has_hero_tree( HERO_CONDUIT_OF_THE_CELESTIALS ) )
  {
    auto count =
        range::count_if( player_traits, [ is_ptr = is_ptr() ]( std::tuple<talent_tree, unsigned, unsigned> entry ) {
          if ( std::get<talent_tree>( entry ) != talent_tree::HERO )
            return false;
          const trait_data_t *trait = trait_data_t::find( std::get<1>( entry ), is_ptr );
          if ( !trait )
            return false;
          return static_cast<hero_tree_e>( trait->id_sub_tree ) == HERO_CONDUIT_OF_THE_CELESTIALS;
        } );

    // Report without counting the hidden talent that activates the subtree
    count -= 1;
    if ( count < 10 )
    {
      sim->error(
          "Invalid Conduit of the Celestials Hero Talent tree, possibly low level. Found {} talents, expected 10.",
          count );
      return false;
    }
  }

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
    case MONK_WINDWALKER:
      return true;
    default:
      sim->error( "No specialization was selected for {}.", *this );
      return false;
  }

  return false;
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
      case FIGHT_STYLE_DUNGEON_ROUTE:
      case FIGHT_STYLE_DUNGEON_SLICE:
        return true;
      default:
        return false;
    }
  }
  return true;
}

void monk_t::init_spells()
{
  base_t::init_spells();

  auto _CT = [ & ]( std::string_view name ) { return find_talent_spell( talent_tree::CLASS, name, specialization() ); };
  auto _ST = [ & ]( std::string_view name ) {
    return find_talent_spell( talent_tree::SPECIALIZATION, name, specialization() );
  };
  auto _HT = [ & ]( std::string_view name ) { return find_talent_spell( talent_tree::HERO, name ); };

  auto _STID = [ & ]( int id ) { return find_talent_spell( talent_tree::SPECIALIZATION, id, specialization() ); };

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
    baseline.monk.tiger_palm               = find_spell( 100780 );
    baseline.monk.touch_of_death           = find_spell( 322109 );
    baseline.monk.vivify                   = find_class_spell( "Vivify" );
  }

  // monk_t::baseline::brewmaster
  {
    baseline.brewmaster.mastery                = find_mastery_spell( MONK_BREWMASTER );
    baseline.brewmaster.aura                   = find_specialization_spell( "Brewmaster Monk" );
    baseline.brewmaster.aura_2                 = find_specialization_spell( 462087 );
    baseline.brewmaster.aura_3                 = find_specialization_spell( 1246978 );
    baseline.brewmaster.aura_4                 = find_specialization_spell( 1258153 );
    baseline.brewmaster.brewmasters_balance    = find_specialization_spell( "Brewmaster's Balance" );
    baseline.brewmaster.celestial_fortune      = find_specialization_spell( "Celestial Fortune" );
    baseline.brewmaster.celestial_fortune_heal = find_spell( 216521 );  // TODO: Can you be more specific?
    baseline.brewmaster.expel_harm_rank_2      = find_rank_spell( "Expel Harm", "Rank 2", specialization() );
    baseline.brewmaster.blackout_kick          = find_spell( 205523 );
    baseline.brewmaster.stagger_self_damage    = find_spell( 124255 );
    baseline.brewmaster.light_stagger          = find_spell( 124275 );
    baseline.brewmaster.moderate_stagger       = find_spell( 124274 );
    baseline.brewmaster.heavy_stagger          = find_spell( 124273 );
    baseline.brewmaster.spinning_crane_kick    = find_specialization_spell( "Spinning Crane Kick" );
    baseline.brewmaster.spinning_crane_kick_rank_2 =
        find_rank_spell( "Spinning Crane Kick", "Rank 2", specialization() );
    baseline.brewmaster.touch_of_death_rank_3 = find_rank_spell( "Touch of Death", "Rank 3", specialization() );
  }

  // monk_t::baseline::windwalker
  {
    baseline.windwalker.mastery                   = find_mastery_spell( MONK_WINDWALKER );
    baseline.windwalker.aura                      = find_specialization_spell( "Windwalker Monk" );
    baseline.windwalker.aura_2                    = find_specialization_spell( 462091 );
    baseline.windwalker.aura_3                    = find_specialization_spell( 1222923 );
    baseline.windwalker.aura_4                    = find_specialization_spell( 1258122 );
    baseline.windwalker.blackout_kick_rank_2      = find_rank_spell( "Blackout Kick", "Rank 2", MONK_WINDWALKER );
    baseline.windwalker.blackout_kick_rank_3      = find_rank_spell( "Blackout Kick", "Rank 3", MONK_WINDWALKER );
    baseline.windwalker.combat_conditioning       = find_specialization_spell( "Combat Conditioning" );
    baseline.windwalker.flying_serpent_kick       = find_specialization_spell( "Flying Serpent Kick" );
    baseline.windwalker.touch_of_death_rank_3     = find_rank_spell( "Touch of Death", "Rank 3", specialization() );
    baseline.windwalker.touch_of_karma            = find_specialization_spell( "Touch of Karma" );
    baseline.windwalker.touch_of_karma_buff       = find_spell( 125174 );
    baseline.windwalker.touch_of_karma_tick       = find_spell( 124280 );
    baseline.windwalker.empowered_tiger_lightning = find_specialization_spell( "Empowered Tiger Lightning" );
    baseline.windwalker.empowered_tiger_lightning_damage = find_spell( 335913 );
  }

  // monk_t::talent::monk
  {
    talent.monk.soothing_mist                = _CT( "Soothing Mist" );
    talent.monk.paralysis                    = _CT( "Paralysis" );
    talent.monk.rising_sun_kick              = _CT( "Rising Sun Kick" );
    talent.monk.stagger                      = _CT( "Stagger" );
    talent.monk.elusive_mists                = _CT( "Elusive Mists" );
    talent.monk.tigers_lust                  = _CT( "Tiger's Lust" );
    talent.monk.crashing_momentum            = _CT( "Crashing Momentum" );
    talent.monk.disable                      = _CT( "Disable" );
    talent.monk.fast_feet                    = _CT( "Fast Feet" );
    talent.monk.grace_of_the_crane           = _CT( "Grace of the Crane" );
    talent.monk.bounding_agility             = _CT( "Bounding Agility" );
    talent.monk.calming_presence             = _CT( "Calming Presence" );
    talent.monk.winds_reach                  = _CT( "Wind's Reach" );
    talent.monk.detox                        = _CT( "Detox" );
    talent.monk.vivacious_vivification       = _CT( "Vivacious Vivification" );
    talent.monk.jade_walk                    = _CT( "Jade Walk" );
    talent.monk.pressure_points              = _CT( "Pressure Points" );
    talent.monk.spear_hand_strike            = _CT( "Spear Hand Strike" );
    talent.monk.ancient_arts                 = _CT( "Ancient Arts" );
    talent.monk.chi_wave                     = _CT( "Chi Wave" );
    talent.monk.chi_wave_buff                = find_spell( 450380 );
    talent.monk.chi_wave_driver              = find_spell( 115098 );
    talent.monk.chi_wave_damage              = find_spell( 132467 );
    talent.monk.chi_wave_heal                = find_spell( 132463 );
    talent.monk.chi_burst                    = _CT( "Chi Burst" );
    talent.monk.chi_burst_buff               = find_spell( 460490 );
    talent.monk.chi_burst_projectile         = find_spell( 461404 );
    talent.monk.chi_burst_damage             = find_spell( 148135 );
    talent.monk.chi_burst_heal               = find_spell( 130654 );
    talent.monk.tiger_fang                   = _CT( "Tiger Fang" );
    talent.monk.transcendence                = _CT( "Transcendence" );
    talent.monk.energy_transfer              = _CT( "Energy Transfer" );
    talent.monk.celerity                     = _CT( "Celerity" );
    talent.monk.chi_torpedo                  = _CT( "Chi Torpedo" );
    talent.monk.stillstep_coil               = _CT( "Stillstep Coil" );
    talent.monk.quick_footed                 = _CT( "Quick Footed" );
    talent.monk.hasty_provocation            = _CT( "Hasty Provocation" );
    talent.monk.ferocity_of_xuen             = _CT( "Ferocity of Xuen" );
    talent.monk.ring_of_peace                = _CT( "Ring of Peace" );
    talent.monk.song_of_chi_ji               = _CT( "Song of Chi-Ji" );
    talent.monk.spirits_essence              = _CT( "Spirit's Essence" );
    talent.monk.tiger_tail_sweep             = _CT( "Tiger Tail Sweep" );
    talent.monk.improved_touch_of_death      = _CT( "Improved Touch of Death" );
    talent.monk.vigorous_expulsion           = _CT( "Vigorous Expulsion" );
    talent.monk.yulons_grace                 = _CT( "Yu'lon's Grace" );
    talent.monk.yulons_grace_buff            = find_spell( 414143 );
    talent.monk.peace_and_prosperity         = _CT( "Peace and Prosperity" );
    talent.monk.fortifying_brew              = _CT( "Fortifying Brew" );
    talent.monk.fortifying_brew_buff         = find_spell( 120954 );
    talent.monk.dance_of_the_wind            = _CT( "Dance of the Wind" );
    talent.monk.save_them_all                = _CT( "Save Them All" );
    talent.monk.swift_art                    = _CT( "Swift Art" );
    talent.monk.strength_of_spirit           = _CT( "Strength of Spirit" );
    talent.monk.profound_rebuttal            = _CT( "Profound Rebuttal" );
    talent.monk.summon_black_ox_statue       = _CT( "Summon Black Ox Statue" );
    talent.monk.zenith_stomp                 = _CT( "Zenith Stomp" );
    talent.monk.zenith_stomp_damage          = find_spell( 1272696 );
    talent.monk.ironshell_brew               = _CT( "Ironshell Brew" );
    talent.monk.expeditious_fortification    = _CT( "Expeditious Fortification" );
    talent.monk.diffuse_magic                = _CT( "Diffuse Magic" );
    talent.monk.celestial_determination      = _CT( "Celestial Determination" );
    talent.monk.chi_proficiency              = _CT( "Chi Proficiency" );
    talent.monk.healing_winds                = _CT( "Healing Winds" );
    talent.monk.windwalking                  = _CT( "Windwalking" );
    talent.monk.chi_transfer                 = _CT( "Chi Transfer" );
    talent.monk.martial_instincts            = _CT( "Martial Instincts" );
    talent.monk.lighter_than_air             = _CT( "Lighter Than Air" );
    talent.monk.flow_of_chi                  = _CT( "Flow of Chi" );
    talent.monk.escape_from_reality          = _CT( "Escape from Reality" );
    talent.monk.transcendence_linked_spirits = _CT( "Transcendence: Linked Spirits" );
    talent.monk.fatal_touch                  = _CT( "Fatal Touch" );
    talent.monk.rushing_reflexes             = _CT( "Rushing Reflexes" );
  }

  // monk_t::talent::brewmaster
  {
    talent.brewmaster.keg_smash                        = _ST( "Keg Smash" );
    talent.brewmaster.purifying_brew                   = _ST( "Purifying Brew" );
    talent.brewmaster.shuffle                          = _ST( "Shuffle" );
    talent.brewmaster.shuffle_buff                     = find_spell( 215479 );
    talent.brewmaster.august_blessing                  = _ST( "August Blessing" );
    talent.brewmaster.staggering_strikes               = _ST( "Staggering Strikes" );
    talent.brewmaster.quick_sip                        = _ST( "Quick Sip" );
    talent.brewmaster.elixir_of_determination          = _ST( "Elixir of Determination" );
    talent.brewmaster.improved_blackout_kick           = _ST( "Improved Blackout Kick" );
    talent.brewmaster.swift_as_a_coursing_river        = _ST( "Swift as a Coursing River" );
    talent.brewmaster.gift_of_the_ox                   = _ST( "Gift of the Ox" );
    talent.brewmaster.gift_of_the_ox_buff              = find_spell( 124506 );
    talent.brewmaster.gift_of_the_ox_heal_trigger      = find_spell( 124507 );
    talent.brewmaster.gift_of_the_ox_heal_expire       = find_spell( 178173 );
    talent.brewmaster.special_delivery                 = _ST( "Special Delivery" );
    talent.brewmaster.special_delivery_missile         = find_spell( 196732 );
    talent.brewmaster.rushing_jade_wind                = _ST( "Rushing Jade Wind" );
    talent.brewmaster.spirit_of_the_ox                 = _ST( "Spirit of the Ox" );
    talent.brewmaster.jade_flash                       = _ST( "Jade Flash" );
    talent.brewmaster.celestial_brew                   = _ST( "Celestial Brew" );
    talent.brewmaster.celestial_infusion               = _ST( "Celestial Infusion" );
    talent.brewmaster.niuzaos_resolve                  = _ST( "Niuzao's Resolve" );
    talent.brewmaster.celestial_flames                 = _ST( "Celestial Flames" );
    talent.brewmaster.shadowboxing_treads              = _ST( "Shadowboxing Treads" );
    talent.brewmaster.fluidity_of_motion               = _ST( "Fluidity of Motion" );
    talent.brewmaster.elusive_footwork                 = _ST( "Elusive Footwork" );
    talent.brewmaster.one_with_the_wind                = _ST( "One With the Wind" );
    talent.brewmaster.breath_of_fire                   = _ST( "Breath of Fire" );
    talent.brewmaster.breath_of_fire_dot               = find_spell( 123725 );
    talent.brewmaster.gai_plins_imperial_brew          = _ST( "Gai Plin's Imperial Brew" );
    talent.brewmaster.gai_plins_imperial_brew_heal     = find_spell( 383701 );
    talent.brewmaster.light_brewing                    = _ST( "Light Brewing" );
    talent.brewmaster.training_of_niuzao               = _ST( "Training of Niuzao" );
    talent.brewmaster.pretense_of_instability          = _ST( "Pretense of Instability" );
    talent.brewmaster.scalding_brew                    = _ST( "Scalding Brew" );
    talent.brewmaster.salsalabims_strength             = _ST( "Sal'salabim's Strength" );
    talent.brewmaster.fortifying_brew_determination    = _ST( "Fortifying Brew: Determination" );
    talent.brewmaster.bob_and_weave                    = _ST( "Bob and Weave" );
    talent.brewmaster.black_ox_brew                    = _ST( "Black Ox Brew" );
    talent.brewmaster.walk_with_the_ox                 = _ST( "Walk With the Ox" );
    talent.brewmaster.walk_with_the_ox_stomp           = find_spell( 1242373 );
    talent.brewmaster.zen_state                        = _ST( "Zen State" );
    talent.brewmaster.tranquil_spirit                  = _ST( "Tranquil Spirit" );
    talent.brewmaster.face_palm                        = _ST( "Face Palm" );
    talent.brewmaster.dragonfire_brew                  = _ST( "Dragonfire Brew" );
    talent.brewmaster.dragonfire_brew_hit              = find_spell( 387621 );
    talent.brewmaster.charred_passions                 = _ST( "Charred Passions" );
    talent.brewmaster.charred_passions_damage          = find_spell( 386959 );
    talent.brewmaster.high_tolerance                   = _ST( "High Tolerance" );
    talent.brewmaster.press_the_advantage              = _ST( "Press the Advantage" );
    talent.brewmaster.press_the_advantage_damage       = find_spell( 418360 );
    talent.brewmaster.blackout_combo                   = _ST( "Blackout Combo" );
    talent.brewmaster.anvil_and_stave                  = _ST( "Anvil and Stave" );
    talent.brewmaster.counterstrike                    = _ST( "Counterstrike" );
    talent.brewmaster.exploding_keg                    = _ST( "Exploding Keg" );
    talent.brewmaster.ox_stance                        = _ST( "Ox Stance" );
    talent.brewmaster.ox_stance_buff                   = find_spell( 455071 );
    talent.brewmaster.awakening_spirit                 = _ST( "Awakening Spirit" );
    talent.brewmaster.vital_flame                      = _ST( "Vital Flame" );
    talent.brewmaster.invoke_niuzao_the_black_ox       = _ST( "Invoke Niuzao, the Black Ox" );
    talent.brewmaster.invoke_niuzao_the_black_ox_npc   = find_spell( 123904 );
    talent.brewmaster.invoke_niuzao_the_black_ox_stomp = find_spell( 227291 );
    talent.brewmaster.fuel_on_the_fire                 = _ST( "Fuel on the Fire" );
    talent.brewmaster.empty_the_cellar                 = _ST( "Empty the Cellar" );
    talent.brewmaster.keg_volley                       = _ST( "Keg Volley" );
    talent.brewmaster.stormstouts_last_keg             = _ST( "Stormstout's Last Keg" );
    talent.brewmaster.heart_of_the_ox                  = _ST( "Heart of the Ox" );
    talent.brewmaster.mighty_stomp                     = _ST( "Mighty Stomp" );
    talent.brewmaster.bring_me_another_1               = _ST( "Bring Me Another" );
    talent.brewmaster.bring_me_another_2               = _STID( 1265138 );
    talent.brewmaster.bring_me_another_3               = _STID( 1265141 );
  }

  // monk_t::talent::windwalker
  {
    talent.windwalker.fists_of_fury                   = _ST( "Fists of Fury" );
    talent.windwalker.momentum_boost                  = _ST( "Momentum Boost" );
    talent.windwalker.momentum_boost_speed            = find_spell( 451298 );
    talent.windwalker.combat_wisdom                   = _ST( "Combat Wisdom" );
    talent.windwalker.combat_wisdom_buff              = find_spell( 129914 );
    talent.windwalker.combat_wisdom_expel_harm        = find_spell( 451968 );
    talent.windwalker.sharp_reflexes                  = _ST( "Sharp Reflexes" );
    talent.windwalker.touch_of_the_tiger              = _ST( "Touch of the Tiger" );
    talent.windwalker.ferociousness                   = _ST( "Ferociousness" );
    talent.windwalker.hardened_soles                  = _ST( "Hardened Soles" );
    talent.windwalker.ascension                       = _ST( "Ascension" );  // TODO: NYI: EFFECT 2 ENERGY REGEN
    talent.windwalker.dual_threat                     = _ST( "Dual Threat" );
    talent.windwalker.dual_threat_damage              = find_spell( 451839 );
    talent.windwalker.teachings_of_the_monastery      = _ST( "Teachings of the Monastery" );
    talent.windwalker.teachings_of_the_monastery_buff = find_spell( 202090 );
    talent.windwalker.teachings_of_the_monastery_blackout_kick = find_spell( 228649 );
    talent.windwalker.glory_of_the_dawn                        = _ST( "Glory of the Dawn" );
    talent.windwalker.glory_of_the_dawn_damage                 = find_spell( 392959 );
    talent.windwalker.crane_vortex                             = _ST( "Crane Vortex" );
    talent.windwalker.meridian_strikes                         = _ST( "Meridian Strikes" );
    talent.windwalker.rising_star                              = _ST( "Rising Star" );
    talent.windwalker.zenith                                   = _ST( "Zenith" );
    talent.windwalker.hit_combo                                = _ST( "Hit Combo" );
    talent.windwalker.hit_combo_buff                           = find_spell( 196741 );
    talent.windwalker.brawlers_intensity                       = _ST( "Brawler's Intensity" );
    talent.windwalker.jade_ignition                            = _ST( "Jade Ignition" );
    talent.windwalker.chi_energy_buff                          = find_spell( 393057 );
    talent.windwalker.chi_explosion                            = find_spell( 393056 );
    talent.windwalker.cyclones_drift                           = _ST( "Cyclone's Drift" );
    talent.windwalker.crashing_fists                           = _ST( "Crashing Fists" );
    talent.windwalker.drinking_horn_cover                      = _ST( "Drinking Horn Cover" );
    talent.windwalker.spiritual_focus                          = _ST( "Spiritual Focus" );
    talent.windwalker.obsidian_spiral                          = _ST( "Obsidian Spiral" );
    talent.windwalker.obsidian_spiral_energize                 = find_spell( 1249833 );
    talent.windwalker.combo_breaker                            = _ST( "Combo Breaker" );
    talent.windwalker.combo_breaker_buff                       = find_spell( 116768 );
    talent.windwalker.dance_of_chiji                           = _ST( "Dance of Chi-Ji" );
    // do not use talent.windwalker.dance_of_chiji->effectN( 1 ).trigger() to avoid talent known dependency
    talent.windwalker.dance_of_chiji_buff            = find_spell( 325202 );
    talent.windwalker.shadowboxing_treads            = _STID( 392982 );
    talent.windwalker.strike_of_the_windlord         = _ST( "Strike of the Windlord" );
    talent.windwalker.whirling_dragon_punch          = _ST( "Whirling Dragon Punch" );
    talent.windwalker.whirling_dragon_punch_aoe_tick = find_spell( 158221 );
    talent.windwalker.whirling_dragon_punch_st_tick  = find_spell( 451767 );
    talent.windwalker.whirling_dragon_punch_buff     = find_spell( 196742 );
    talent.windwalker.energy_burst                   = _ST( "Energy Burst" );
    talent.windwalker.inner_peace                    = _ST( "Inner Peace" );
    talent.windwalker.sequenced_strikes              = _ST( "Sequenced Strikes" );
    talent.windwalker.sunfire_spiral                 = _ST( "Sunfire Spiral" );
    talent.windwalker.communion_with_wind            = _ST( "Communion With Wind" );
    talent.windwalker.revolving_whirl                = _ST( "Revolving Whirl" );
    talent.windwalker.echo_technique                 = _ST( "Echo Technique" );
    talent.windwalker.universal_energy               = _ST( "Universal Energy" );
    talent.windwalker.memory_of_the_monastery        = _ST( "Memory of the Monastery" );
    talent.windwalker.memory_of_the_monastery_buff   = find_spell( 454970 );
    talent.windwalker.rushing_wind_kick              = _ST( "Rushing Wind Kick" );
    talent.windwalker.rushing_wind_kick_buff         = find_spell( 1250554 );
    talent.windwalker.rushing_wind_kick_damage       = find_spell( 468179 );
    talent.windwalker.xuens_battlegear               = _ST( "Xuen's Battlegear" );
    talent.windwalker.thunderfist                    = _ST( "Thunderfist" );
    talent.windwalker.thunderfist_buff               = find_spell( 393565 );
    talent.windwalker.knowledge_of_the_broken_temple = _ST( "Knowledge of the Broken Temple" );
    talent.windwalker.slicing_winds                  = _ST( "Slicing Winds" );
    talent.windwalker.slicing_winds_damage           = find_spell( 1217411 );
    talent.windwalker.jadefire_stomp                 = _ST( "Jadefire Stomp" );
    talent.windwalker.jadefire_stomp_damage          = find_spell( 1248815 );
    talent.windwalker.jadefire_stomp_targeting       = find_spell( 1248812 );
    talent.windwalker.skyfire_heel                   = _ST( "Skyfire Heel" );
    talent.windwalker.skyfire_heel_damage            = find_spell( 1248712 );
    talent.windwalker.skyfire_heel_buff              = find_spell( 1248705 );
    talent.windwalker.harmonic_combo                 = _ST( "Harmonic Combo" );
    talent.windwalker.flurry_of_xuen                 = _ST( "Flurry of Xuen" );
    talent.windwalker.flurry_of_xuen_driver          = find_spell( 452117 );
    talent.windwalker.martial_agility                = _ST( "Martial Agility" );
    talent.windwalker.airborne_rhythm                = _ST( "Airborne Rhythm" );
    talent.windwalker.airborne_rhythm_energize       = find_spell( 1248835 );
    talent.windwalker.hurricanes_vault               = _ST( "Hurricane's Vault" );
    talent.windwalker.path_of_jade                   = _ST( "Path of Jade" );
    talent.windwalker.singularly_focused_jade        = _ST( "Singularly Focused Jade" );
  }

  // monk_t::talent::conduit_of_the_celestials
  {
    talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger       = _HT( "Invoke Xuen, the White Tiger" );
    talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger_npc   = find_spell( 132578 );
    talent.conduit_of_the_celestials.crackling_tiger_lightning_driver  = find_spell( 123999 );
    talent.conduit_of_the_celestials.temple_training                   = _HT( "Temple Training" );
    talent.conduit_of_the_celestials.xuens_guidance                    = _HT( "Xuen's Guidance" );
    talent.conduit_of_the_celestials.courage_of_the_white_tiger        = _HT( "Courage of the White Tiger" );
    talent.conduit_of_the_celestials.courage_of_the_white_tiger_buff   = find_spell( 460127 );
    talent.conduit_of_the_celestials.courage_of_the_white_tiger_damage = find_spell( 457917 );
    talent.conduit_of_the_celestials.courage_of_the_white_tiger_heal   = find_spell( 443106 );
    talent.conduit_of_the_celestials.restore_balance                   = _HT( "Restore Balance" );
    talent.conduit_of_the_celestials.xuens_bond                        = _HT( "Xuen's Bond" );
    talent.conduit_of_the_celestials.heart_of_the_jade_serpent         = _HT( "Heart of the Jade Serpent" );
    talent.conduit_of_the_celestials.heart_of_the_jade_serpent_buff    = find_spell( 443421 );
    talent.conduit_of_the_celestials.chijis_swiftness                  = _HT( "Chi-Ji's Swiftness" );
    talent.conduit_of_the_celestials.chijis_swiftness_buff             = find_spell( 443028 );
    talent.conduit_of_the_celestials.strength_of_the_black_ox          = _HT( "Strength of the Black Ox" );
    talent.conduit_of_the_celestials.strength_of_the_black_ox_buff     = find_spell( 443112 );
    talent.conduit_of_the_celestials.strength_of_the_black_ox_absorb   = find_spell( 443113 );
    talent.conduit_of_the_celestials.strength_of_the_black_ox_damage   = find_spell( 443127 );
    talent.conduit_of_the_celestials.path_of_the_falling_star          = _HT( "Path of the Falling Star" );
    talent.conduit_of_the_celestials.yulons_avatar                     = _HT( "Yu'lon's Avatar" );
    talent.conduit_of_the_celestials.yulons_avatar_buff                = find_spell( 1238904 );
    talent.conduit_of_the_celestials.niuzaos_protection                = _HT( "Niuzao's Protection" );
    talent.conduit_of_the_celestials.jade_sanctuary                    = _HT( "Jade Sanctuary" );
    talent.conduit_of_the_celestials.jade_sanctuary_buff               = find_spell( 448508 );
    talent.conduit_of_the_celestials.celestial_conduit                 = _HT( "Celestial Conduit" );
    talent.conduit_of_the_celestials.celestial_conduit_action          = find_spell( 443028 );
    talent.conduit_of_the_celestials.celestial_conduit_damage          = find_spell( 443038 );
    talent.conduit_of_the_celestials.celestial_conduit_heal            = find_spell( 443039 );
    talent.conduit_of_the_celestials.inner_compass                     = _HT( "Inner Compass" );
    talent.conduit_of_the_celestials.inner_compass_crane_stance_buff   = find_spell( 443572 );
    talent.conduit_of_the_celestials.inner_compass_ox_stance_buff      = find_spell( 443574 );
    talent.conduit_of_the_celestials.inner_compass_tiger_stance_buff   = find_spell( 443575 );
    talent.conduit_of_the_celestials.inner_compass_serpent_stance_buff = find_spell( 443576 );
    talent.conduit_of_the_celestials.flowing_wisdom                    = _HT( "Flowing Wisdom" );
    talent.conduit_of_the_celestials.unity_within                      = _HT( "Unity Within" );
    talent.conduit_of_the_celestials.unity_within_buff                 = find_spell( 443592 );
    talent.conduit_of_the_celestials.unity_within_heart_of_the_jade_serpent_buff = find_spell( 443616 );
    talent.conduit_of_the_celestials.unity_within_dmg_mult                       = find_spell( 443591 );
  }

  // monk_t::talent::master_of_harmony
  {
    talent.master_of_harmony.aspect_of_harmony             = _HT( "Aspect of Harmony" );
    talent.master_of_harmony.aspect_of_harmony_driver      = find_spell( 450567 );
    talent.master_of_harmony.aspect_of_harmony_accumulator = find_spell( 450521 );
    talent.master_of_harmony.aspect_of_harmony_spender     = find_spell( 450711 );
    talent.master_of_harmony.aspect_of_harmony_damage      = find_spell( 450763 );
    talent.master_of_harmony.aspect_of_harmony_heal        = find_spell( 450769 );
    talent.master_of_harmony.manifestation                 = _HT( "Manifestation" );
    talent.master_of_harmony.purified_spirit               = _HT( "Purified Spirit" );
    talent.master_of_harmony.purified_spirit_damage        = find_spell( 450820 );
    talent.master_of_harmony.purified_spirit_heal          = find_spell( 450805 );
    talent.master_of_harmony.harmonic_gambit               = _HT( "Harmonic Gambit" );
    talent.master_of_harmony.balanced_stratagem            = _HT( "Balanced Stratagem" );
    talent.master_of_harmony.balanced_stratagem_magic      = find_spell( 451508 );
    talent.master_of_harmony.balanced_stratagem_physical   = find_spell( 451514 );
    talent.master_of_harmony.harmonic_surge                = _HT( "Harmonic Surge" );
    talent.master_of_harmony.tigers_vigor                  = _HT( "Tiger's Vigor" );
    talent.master_of_harmony.roar_from_the_heavens         = _HT( "Roar from the Heavens" );
    talent.master_of_harmony.endless_draught               = _HT( "Endless Draught" );
    talent.master_of_harmony.mantra_of_purity              = _HT( "Mantra of Purity" );
    talent.master_of_harmony.mantra_of_tenacity            = _HT( "Mantra of Tenacity" );
    talent.master_of_harmony.potential_energy              = _HT( "Potential Energy" );
    talent.master_of_harmony.overwhelming_force            = _HT( "Overwhelming Force" );
    talent.master_of_harmony.overwhelming_force_damage     = find_spell( 452333 );
    talent.master_of_harmony.path_of_resurgence            = _HT( "Path of Resurgence" );
    talent.master_of_harmony.way_of_a_thousand_strikes     = _HT( "Way of a Thousand Strikes" );
    talent.master_of_harmony.clarity_of_purpose            = _HT( "Clarity of Purpose" );
    talent.master_of_harmony.meditative_focus              = _HT( "Meditative Focus" );
    talent.master_of_harmony.coalescence                   = _HT( "Coalescence" );
  }

  // monk_t::talent::shado_pan
  {
    talent.shado_pan.flurry_strikes                           = _HT( "Flurry Strikes" );
    talent.shado_pan.flurry_charge                            = find_spell( 451021 );
    talent.shado_pan.flurry_strike                            = find_spell( 450617 );
    talent.shado_pan.pride_of_pandaria                        = _HT( "Pride of Pandaria" );
    talent.shado_pan.high_impact                              = _HT( "High Impact" );
    talent.shado_pan.high_impact_debuff                       = find_spell( 451037 );
    talent.shado_pan.veterans_eye                             = _HT( "Veteran's Eye" );
    talent.shado_pan.martial_precision                        = _HT( "Martial Precision" );
    talent.shado_pan.shado_over_the_battlefield               = _HT( "Shado Over the Battlefield" );
    talent.shado_pan.flurry_strike_shado_over_the_battlefield = find_spell( 451250 );
    talent.shado_pan.combat_stance                            = _HT( "Combat Stance" );
    talent.shado_pan.initiators_edge                          = _HT( "Initiator's Edge" );
    talent.shado_pan.one_versus_many                          = _HT( "One Versus Many" );
    talent.shado_pan.whirling_steel                           = _HT( "Whirling Steel" );
    talent.shado_pan.predictive_training                      = _HT( "Predictive Training" );
    talent.shado_pan.stand_ready                              = _HT( "Stand Ready" );
    talent.shado_pan.stand_ready_buff                         = find_spell( 1237196 );
    talent.shado_pan.stand_ready_driver                       = find_spell( 1237196 );
    talent.shado_pan.against_all_odds                         = _HT( "Against All Odds" );
    talent.shado_pan.efficient_training                       = _HT( "Efficient Training" );
    talent.shado_pan.vigilant_watch                           = _HT( "Vigilant Watch" );
    talent.shado_pan.weapons_of_the_wall                      = _HT( "Weapons of the Wall" );
    talent.shado_pan.wisdom_of_the_wall                       = _HT( "Wisdom of the Wall" );
  }

  // monk_t::tier
  {
  }

  // Shared Talent Spells
  talent.shared_spell.rushing_jade_wind_buff = find_spell( 116847 );
  talent.shared_spell.rushing_jade_wind_tick = find_spell( 148187 );

  // Register passives
  // Instant Spells with a reduced GCD
  register_passive_affect_list( baseline.brewmaster.aura_2, affect_list_t( 3 ).remove_label( 640 ) );
  // Instant Spells with a reduced GCD
  register_passive_affect_list( baseline.brewmaster.aura_4,
                                affect_list_t( 7 ).remove_spell( 115098,  // Chi Wave Action
                                                                 130654,  // Chi Burst Heal
                                                                 148135,  // Chi Burst Damage
                                                                 185099,  // Rising Sun Kick Damage
                                                                 228649,  // Teachings of the Monastery Blackout Kick
                                                                 261682,  // Chi Burst Energize
                                                                 280184,  // Unknown Leg Sweep?
                                                                 322111,  // Touch of Death Damage
                                                                 331433,  // Unknown Tiger Palm?
                                                                 392959,  // Glory of the Dawn
                                                                 450342,  // Crashing Momentum
                                                                 451968,  // Combat Wisdom Expel Harm Heal
                                                                 468179,  // Rushing Wind Kick Damage
                                                                 1249625  // Zenith
                                                                 ) );
  // Chargeless Spells with a reduced Charge Cooldown
  register_passive_affect_list( talent.brewmaster.fluidity_of_motion,
                                affect_list_t( 2 ).remove_spell( 100784 ) );  // Blackout Kick
  // Instant Spells with a reduced GCD
  register_passive_affect_list( baseline.windwalker.aura_2, affect_list_t( 2 ).remove_label( 640 ) );
  // Instant Spells with a reduced GCD
  register_passive_affect_list( baseline.windwalker.aura_4,
                                affect_list_t( 8 ).remove_spell( 115098,  // Chi Wave Action
                                                                 130654,  // Chi Burst Heal
                                                                 148135,  // Chi Burst Damage
                                                                 185099,  // Rising Sun Kick Damage
                                                                 228649,  // Teachings of the Monastery Blackout Kick
                                                                 261682,  // Chi Burst Energize
                                                                 280184,  // Old Leg Sweep Modifier
                                                                 322111,  // Touch of Death Damage
                                                                 331433,  // Some Weird Tiger Palm
                                                                 392959,  // Glory of the Dawn
                                                                 450342,  // Crashing Momentum
                                                                 451968,  // Combat Wisdom Expel Harm Heal
                                                                 468179,  // Rushing Wind Kick Damage
                                                                 1249625  // Zenith
                                                                 ) );
  // Scripted enablement based on specialization
  register_passive_effect_mask( talent.shado_pan.efficient_training, specialization() == MONK_WINDWALKER
                                                                         ? effect_mask_t( true ).disable( 5 )
                                                                         : effect_mask_t( true ) );

  // brewmaster
  deregister_passive_spell( talent.brewmaster.heart_of_the_ox );
  // windwalker
  deregister_passive_spell( talent.windwalker.ferociousness );
  deregister_passive_spell( talent.windwalker.martial_agility );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_all_passive_sets();
}

void monk_t::init_background_actions()
{
  base_t::init_background_actions();

  // we just look it up via `find_action` anyway, so it doesn't need to explicitly
  // be set anywhere (for now)
  new buffs::rushing_jade_wind_buff_t::tick_action_t( this );

  // General
  action.chi_wave = new actions::spells::chi_wave_t( this );

  // Conduit of the Celestials
  bool uw  = talent.conduit_of_the_celestials.unity_within->ok();
  bool cwt = talent.conduit_of_the_celestials.courage_of_the_white_tiger->ok() || uw;
  bool sbt = talent.conduit_of_the_celestials.strength_of_the_black_ox->ok() || uw;

  if ( cwt )
    action.courage_of_the_white_tiger = actions::spells::courage_of_the_white_tiger_t( this );

  if ( sbt )
    action.strength_of_the_black_ox = actions::spells::strength_of_the_black_ox_t( this );

  // Shado-Pan
  action.flurry_strikes = new actions::attacks::flurry_strikes_t( talent.shado_pan.flurry_strikes->ok(), this );

  // Brewmaster
  if ( specialization() == MONK_BREWMASTER )
  {
    action.special_delivery  = new actions::spells::special_delivery_t( this );
    action.breath_of_fire    = new actions::spells::breath_of_fire_dot_t( this );
    action.celestial_fortune = new actions::heals::celestial_fortune_t( this );
    action.exploding_keg     = new actions::spells::exploding_keg_proc_t( this );
    action.walk_with_the_ox  = new actions::attacks::stomp_t( this );
  }

  // Windwalker
  if ( specialization() == MONK_WINDWALKER )
  {
    action.empowered_tiger_lightning = new actions::spells::empowered_tiger_lightning_t( this );
    action.flurry_of_xuen            = new actions::spells::flurry_of_xuen_t( this );
    action.combat_wisdom_eh          = new actions::heals::expel_harm_t( this, "" );
  }
}

void monk_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    if ( specialization() == MONK_MISTWEAVER )
      base.distance = 40;
    else
      base.distance = 5;
  }

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      base.attack_power_per_agility              = 1.0;
      resources.active_resource[ RESOURCE_MANA ] = false;
      break;
    case MONK_WINDWALKER:
      if ( base.distance < 1 )
        base.distance = 5;
      base.attack_power_per_agility              = 1.0;
      resources.active_resource[ RESOURCE_MANA ] = false;
      break;
    default:
      break;
  }

  base_t::init_base_stats();
}

void monk_t::init_scaling()
{
  base_t::init_scaling();

  scaling->disable( STAT_STRENGTH );
  scaling->disable( STAT_INTELLECT );
  scaling->disable( STAT_SPELL_POWER );

  scaling->enable( STAT_AGILITY );
  scaling->enable( STAT_WEAPON_DPS );
  scaling->enable( STAT_STAMINA );

  if ( off_hand_weapon.type != WEAPON_NONE )
    scaling->enable( STAT_WEAPON_OFFHAND_DPS );
}

// TODO: move these to somewhere sensible
struct self_damage_override : stagger_impl::self_damage_t<monk_t>
{
  self_damage_override( monk_t *player, stagger_impl::stagger_effect_t<monk_t> *stagger_effect )
    : stagger_impl::self_damage_t<monk_t>( player, stagger_effect )
  {
    // not automatic
    dot_duration = player->baseline.brewmaster.heavy_stagger->duration();
    dot_duration +=
        timespan_t::from_seconds( player->talent.brewmaster.bob_and_weave->effectN( 1 ).base_value() / 10.0 );
  }
};

struct training_of_niuzao_buff : buffs::monk_buff_t<>
{
  training_of_niuzao_buff( monk_t *player )
    : buffs::monk_buff_t<>( player, "training_of_niuzao", player->talent.brewmaster.training_of_niuzao )
  {
    set_default_value( 0.0 );
    set_pct_buff_type( STAT_PCT_BUFF_MASTERY );
  }

  bool trigger( int = -1, double = DEFAULT_VALUE(), double chance = -1.0,
                timespan_t duration = timespan_t::min() ) override
  {
    double v = p().find_stagger( "Stagger" )->level_index() * data().effectN( 1 ).base_value();
    return base_t::trigger( 1, v, chance, duration );
  }
};

void monk_t::create_buffs()
{
  create_stagger<stagger_impl::debuff_t<monk_t>, self_damage_override>(
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
          double multiplier     = talent.monk.stagger->effectN( 1 ).percent();
          double stagger_rating = agility() * multiplier;

          if ( talent.brewmaster.fortifying_brew_determination->ok() && buff.fortifying_brew->up() )
            stagger_rating *= 1.0 + talent.monk.fortifying_brew_buff->effectN( 6 ).percent();

          if ( buff.shuffle->up() )
            stagger_rating *= 1.0 + talent.brewmaster.shuffle_buff->effectN( 1 ).percent();

          // multiplier is not available in spell data :(
          if ( buff.ox_stance->up() &&
               state->result_amount / current_health() > talent.brewmaster.ox_stance->effectN( 1 ).percent() )
          {
            stagger_rating *= 1.0 + 0.4;
            buff.ox_stance->decrement();
          }

          // multiplier is not available in spell data :(
          if ( talent.brewmaster.zen_state->ok() )
            stagger_rating *= 1.0 + 1.3 * ( 1.0 - current_health() );

          double k = dbc->armor_mitigation_constant( state->target->level() );
          k *= dbc->get_armor_constant_mod( difficulty_e::MYTHIC );

          double stagger_percent = stagger_rating / ( stagger_rating + k );

          // order of operations here is untestable with current in game values
          if ( school != SCHOOL_PHYSICAL )
            stagger_percent *= talent.monk.stagger->effectN( 5 ).percent();

          return std::min( stagger_percent, 0.99 );
        } } );

  base_t::create_buffs();

  // Monk
  buff.combat_wisdom = make_buff_fallback( talent.windwalker.combat_wisdom->ok(), this, "combat_wisdom",
                                           talent.windwalker.combat_wisdom_buff )
                           ->set_trigger_spell( talent.windwalker.combat_wisdom )
                           ->set_default_value_from_effect( 1 );

  buff.chi_wave = make_buff_fallback( talent.monk.chi_wave->ok(), this, "chi_wave", talent.monk.chi_wave_buff );

  buff.fortifying_brew = make_buff_fallback<buffs::fortifying_brew_t>(
      talent.monk.fortifying_brew->ok() && specialization() == MONK_BREWMASTER, this, "fortifying_brew" );

  buff.rushing_jade_wind = make_buff_fallback<buffs::rushing_jade_wind_buff_t>(
      talent.brewmaster.rushing_jade_wind->ok(), this, "rushing_jade_wind" );

  buff.spinning_crane_kick = make_buff( this, "spinning_crane_kick", baseline.monk.spinning_crane_kick )
                                 ->set_default_value_from_effect( 2 )
                                 ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );

  buff.yulons_grace = make_buff_fallback<absorb_buff_t>( talent.monk.yulons_grace->ok(), this, "yulons_grace",
                                                         talent.monk.yulons_grace_buff );

  // Brewmaster
  buff.training_of_niuzao = make_buff_fallback<training_of_niuzao_buff>( talent.brewmaster.training_of_niuzao->ok(),
                                                                         this, "training_of_niuzao" );
  buff.ox_stance =
      make_buff_fallback( talent.brewmaster.ox_stance->ok(), this, "ox_stance", talent.brewmaster.ox_stance_buff );

  buff.blackout_combo = make_buff_fallback( talent.brewmaster.blackout_combo->ok(), this, "blackout_combo",
                                            talent.brewmaster.blackout_combo->effectN( 5 ).trigger() );

  buff.charred_passions = make_buff_fallback( talent.brewmaster.charred_passions->ok(), this, "charred_passions",
                                              talent.brewmaster.charred_passions->effectN( 1 ).trigger() )
                              ->set_trigger_spell( talent.brewmaster.charred_passions );

  buff.counterstrike = make_buff_fallback( talent.brewmaster.counterstrike->ok(), this, "counterstrike",
                                           talent.brewmaster.counterstrike->effectN( 1 ).trigger() )
                           ->set_cooldown( talent.brewmaster.counterstrike->internal_cooldown() );

  buff.elusive_brawler = make_buff_fallback( specialization() == MONK_BREWMASTER, this, "elusive_brawler",
                                             baseline.brewmaster.mastery->effectN( 3 ).trigger() )
                             ->add_invalidate( CACHE_DODGE );

  buff.exploding_keg = make_buff_fallback( talent.brewmaster.exploding_keg->ok(), this, "exploding_keg",
                                           talent.brewmaster.exploding_keg )
                           ->set_default_value_from_effect( 2 );

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
          ->set_max_stack( 1 );

  buff.invoke_niuzao = make_buff_fallback( talent.brewmaster.invoke_niuzao_the_black_ox->ok(), this,
                                           "invoke_niuzao_the_black_ox", talent.brewmaster.invoke_niuzao_the_black_ox )
                           ->set_default_value_from_effect( 2 )
                           ->set_cooldown( timespan_t::zero() );

  buff.press_the_advantage =
      make_buff_fallback( talent.brewmaster.press_the_advantage->ok(), this, "press_the_advantage",
                          talent.brewmaster.press_the_advantage->effectN( 2 ).trigger() )
          ->set_default_value_from_effect( 1 );

  buff.pretense_of_instability =
      make_buff_fallback( talent.brewmaster.pretense_of_instability->ok(), this, "pretense_of_instability",
                          talent.brewmaster.pretense_of_instability->effectN( 1 ).trigger() )
          ->set_trigger_spell( talent.brewmaster.pretense_of_instability )
          ->add_invalidate( CACHE_DODGE );

  // the override is a little weird, we'll just let this always init
  buff.shuffle = make_buff<buffs::shuffle_t>( this );

  // Windwalker
  buff.teachings_of_the_monastery =
      make_buff_fallback( talent.windwalker.teachings_of_the_monastery->ok(), this, "teachings_of_the_monastery",
                          talent.windwalker.teachings_of_the_monastery_buff )
          ->set_trigger_spell( talent.windwalker.teachings_of_the_monastery )
          ->set_default_value_from_effect( 1 );

  buff.combo_breaker = make_buff_fallback( talent.windwalker.combo_breaker->ok(), this, "combo_breaker",
                                           talent.windwalker.combo_breaker_buff )
                           ->set_trigger_spell( talent.windwalker.combo_breaker )
                           ->set_chance( talent.windwalker.combo_breaker->effectN( 1 ).percent() );

  buff.chi_energy =
      make_buff_fallback( talent.windwalker.jade_ignition->ok(), this, "chi_energy", talent.windwalker.chi_energy_buff )
          ->set_default_value_from_effect( 1 );

  buff.combo_strikes =
      make_buff_fallback( baseline.windwalker.mastery->ok(), this, "combo_strikes" )
          ->set_trigger_spell( baseline.windwalker.mastery )
          ->set_duration( timespan_t::from_minutes( 60 ) )
          ->set_quiet( true );  // In-game does not show this buff but I would like to use it for background stuff

  // Create the buff even if untalented - it is possible to get a dance of chiji proc without the talent from other
  // sources.
  buff.dance_of_chiji = make_buff_fallback( specialization() == MONK_WINDWALKER, this, "dance_of_chiji",
                                            talent.windwalker.dance_of_chiji_buff )
                            ->set_trigger_spell( talent.windwalker.dance_of_chiji );

  buff.dance_of_chiji_hidden =
      make_buff_fallback( specialization() != MONK_BREWMASTER, this, "dance_of_chiji_hidden" )
          ->set_default_value( talent.windwalker.dance_of_chiji_buff->effectN( 1 ).base_value() )
          ->set_duration( timespan_t::from_seconds( 1.5 ) )
          ->set_quiet( true );

  buff.hit_combo =
      make_buff_fallback( talent.windwalker.hit_combo->ok(), this, "hit_combo", talent.windwalker.hit_combo_buff )
          ->set_default_value_from_effect( 1 );

  buff.flurry_of_xuen =
      make_buff_fallback( talent.windwalker.flurry_of_xuen->ok(), this, "flurry_of_xuen",
                          talent.windwalker.flurry_of_xuen_driver )
          ->set_tick_callback( [ this ]( buff_t * /* b */, int, timespan_t ) { action.flurry_of_xuen->execute(); } )
          ->set_tick_behavior( buff_tick_behavior::CLIP )
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_freeze_stacks( true );

  buff.invoke_xuen = make_buff_fallback<buffs::invoke_xuen_the_white_tiger_buff_t>(
                         talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger->ok(), this,
                         "invoke_xuen_the_white_tiger", talent.conduit_of_the_celestials.invoke_xuen_the_white_tiger )
                         ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.memory_of_the_monastery =
      make_buff_fallback( talent.windwalker.memory_of_the_monastery->ok(), this, "memory_of_the_monastery",
                          talent.windwalker.memory_of_the_monastery_buff )
          ->set_trigger_spell( talent.windwalker.memory_of_the_monastery );

  buff.momentum_boost_damage =
      make_buff_fallback( talent.windwalker.momentum_boost->ok(), this, "momentum_boost_damage",
                          talent.windwalker.momentum_boost->effectN( 1 ).trigger() )
          ->set_default_value_from_effect( 1 );

  buff.momentum_boost_speed = make_buff_fallback( talent.windwalker.momentum_boost->ok(), this, "momentum_boost_speed",
                                                  talent.windwalker.momentum_boost_speed )
                                  ->set_trigger_spell( talent.windwalker.momentum_boost );

  buff.thunderfist = make_buff_fallback( talent.windwalker.thunderfist->ok(), this, "thunderfist",
                                         talent.windwalker.thunderfist_buff );

  buff.touch_of_death_ww = new buffs::touch_of_death_ww_buff_t( this, "touch_of_death_ww", spell_data_t::nil() );

  buff.touch_of_karma =
      new buffs::touch_of_karma_buff_t( this, "touch_of_karma", baseline.windwalker.touch_of_karma_buff );

  buff.whirling_dragon_punch = make_buff_fallback<buffs::whirling_dragon_punch_buff_t>(
      talent.windwalker.whirling_dragon_punch->ok(), this, "whirling_dragon_punch" );

  buff.zenith = make_buff_fallback( talent.windwalker.zenith->ok(), this, "zenith", talent.windwalker.zenith )
                    ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buff.rushing_wind_kick = make_buff_fallback( talent.windwalker.rushing_wind_kick->ok(), this, "rushing_wind_kick",
                                               talent.windwalker.rushing_wind_kick_buff );

  // Conduit of the Celestials
  buff.celestial_conduit =
      make_buff_fallback( talent.conduit_of_the_celestials.celestial_conduit->ok(), this, "celestial_conduit",
                          talent.conduit_of_the_celestials.celestial_conduit->effectN( 1 ).trigger() );

  buff.chijis_swiftness =
      make_buff_fallback( talent.conduit_of_the_celestials.chijis_swiftness->ok(), this, "chijis_swiftness",
                          talent.conduit_of_the_celestials.chijis_swiftness_buff );

  buff.courage_of_the_white_tiger = make_buff_fallback(
      talent.conduit_of_the_celestials.courage_of_the_white_tiger->ok(), this, "courage_of_the_white_tiger",
      talent.conduit_of_the_celestials.courage_of_the_white_tiger_buff );

  buff.heart_of_the_jade_serpent = make_buff_fallback(
      talent.conduit_of_the_celestials.heart_of_the_jade_serpent->ok(), this, "heart_of_the_jade_serpent",
      talent.conduit_of_the_celestials.heart_of_the_jade_serpent_buff );

  buff.heart_of_the_jade_serpent_yulons_avatar = make_buff_fallback(
      talent.conduit_of_the_celestials.yulons_avatar->ok(), this, "heart_of_the_jade_serpent_yulons_avatar",
      talent.conduit_of_the_celestials.yulons_avatar_buff );

  // TODO: does this cancel zenith (yulon's avatar) hotjs?
  buff.heart_of_the_jade_serpent_unity_within =
      make_buff_fallback( talent.conduit_of_the_celestials.unity_within->ok(), this,
                          "heart_of_the_jade_serpent_unity_within",
                          talent.conduit_of_the_celestials.unity_within_heart_of_the_jade_serpent_buff )
          ->set_stack_change_callback( [ & ]( buff_t *, int old_, int new_ ) {
            if ( new_ && !old_ )
              buff.heart_of_the_jade_serpent->expire();
          } );

  buff.inner_compass_crane_stance =
      make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this, "crane_stance",
                          talent.conduit_of_the_celestials.inner_compass_crane_stance_buff )
          ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
            if ( old_ == 0 )
            {
              buff.inner_compass_ox_stance->expire();
              buff.inner_compass_serpent_stance->expire();
              buff.inner_compass_tiger_stance->expire();
            }
          } );

  buff.inner_compass_ox_stance =
      make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this, "ox_stance",
                          talent.conduit_of_the_celestials.inner_compass_ox_stance_buff )
          ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
            if ( old_ == 0 )
            {
              buff.inner_compass_crane_stance->expire();
              buff.inner_compass_serpent_stance->expire();
              buff.inner_compass_tiger_stance->expire();
            }
          } );

  buff.inner_compass_serpent_stance =
      make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this, "serpent_stance",
                          talent.conduit_of_the_celestials.inner_compass_serpent_stance_buff )
          ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
            if ( old_ == 0 )
            {
              buff.inner_compass_crane_stance->expire();
              buff.inner_compass_ox_stance->expire();
              buff.inner_compass_tiger_stance->expire();
            }
          } );

  buff.inner_compass_tiger_stance =
      make_buff_fallback( talent.conduit_of_the_celestials.inner_compass->ok(), this, "tiger_stance",
                          talent.conduit_of_the_celestials.inner_compass_tiger_stance_buff )
          ->set_stack_change_callback( [ this ]( buff_t *, int old_, int ) {
            if ( old_ == 0 )
            {
              buff.inner_compass_crane_stance->expire();
              buff.inner_compass_ox_stance->expire();
              buff.inner_compass_serpent_stance->expire();
            }
          } );

  buff.jade_sanctuary = make_buff_fallback( talent.conduit_of_the_celestials.jade_sanctuary->ok(), this,
                                            "jade_sanctuary", talent.conduit_of_the_celestials.jade_sanctuary_buff );

  buff.strength_of_the_black_ox =
      make_buff_fallback( talent.conduit_of_the_celestials.strength_of_the_black_ox->ok(), this,
                          "strength_of_the_black_ox", talent.conduit_of_the_celestials.unity_within_buff );

  buff.unity_within = make_buff_fallback( talent.conduit_of_the_celestials.unity_within->ok(), this, "unity_within",
                                          talent.conduit_of_the_celestials.unity_within_buff )
                          ->set_expire_callback( [ this ]( buff_t *, double, timespan_t ) {
                            buff.jade_sanctuary->trigger();
                            action.strength_of_the_black_ox.celestial->execute();
                            action.courage_of_the_white_tiger.celestial->execute();

                            buff.heart_of_the_jade_serpent_unity_within->trigger();
                          } );

  buff.aspect_of_harmony.construct_buffs( this );

  buff.balanced_stratagem_magic =
      make_buff_fallback( talent.master_of_harmony.balanced_stratagem->ok(), this, "balanced_stratagem_magic",
                          talent.master_of_harmony.balanced_stratagem_magic );
  buff.balanced_stratagem_physical =
      make_buff_fallback( talent.master_of_harmony.balanced_stratagem->ok(), this, "balanced_stratagem_physical",
                          talent.master_of_harmony.balanced_stratagem_physical );

  // Shado-Pan
  buff.flurry_charge =
      make_buff_fallback( talent.shado_pan.flurry_strikes->ok(), this, "flurry_charge", talent.shado_pan.flurry_charge )
          ->set_default_value_from_effect( 1 );

  buff.stand_ready =
      make_buff_fallback( talent.shado_pan.stand_ready->ok(), this, "stand_ready", talent.shado_pan.stand_ready_buff )
          ->set_default_value_from_effect( 1 );
}

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.black_ox_brew_energy = get_gain( "black_ox_brew_energy" );
  gain.combo_breaker        = get_gain( "combo_breaker" );
  gain.chi_refund           = get_gain( "chi_refund" );
  gain.energy_burst         = get_gain( "energy_burst" );
  gain.energy_refund        = get_gain( "energy_refund" );
  gain.touch_of_death_ww    = get_gain( "touch_of_death_ww" );
  gain.zenith               = get_gain( "zenith" );
}

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.anvil_and_stave            = get_proc( "Anvil & Stave" );
  proc.blackout_combo_tiger_palm  = get_proc( "Blackout Combo - Tiger Palm" );
  proc.blackout_combo_keg_smash   = get_proc( "Blackout Combo - Keg Smash" );
  proc.charred_passions           = get_proc( "Charred Passions" );
  proc.counterstrike_tp           = get_proc( "Counterstrike - Tiger Palm" );
  proc.counterstrike_sck          = get_proc( "Counterstrike - Spinning Crane Kick" );
  proc.elusive_footwork_proc      = get_proc( "Elusive Footwork" );
  proc.rsk_reset_totm             = get_proc( "Rising Sun Kick TotM Reset" );
  proc.salsalabims_strength       = get_proc( "Sal'salabim Breath of Fire Reset" );
  proc.tranquil_spirit_expel_harm = get_proc( "Tranquil Spirit - Expel Harm" );
  proc.tranquil_spirit_goto       = get_proc( "Tranquil Spirit - Gift of the Ox" );
  proc.xuens_battlegear_reduction = get_proc( "Xuen's Battlegear CD Reduction" );
  proc.elusive_brawler_preserved  = get_proc( "Elusive Brawler Stacks Preserved" );
}

monk_effect_callback_t::monk_effect_callback_t( const special_effect_t &effect, monk_t *player )
  : dbc_proc_callback_t( effect.player, effect ), player( player )
{
}

void monk_effect_callback_t::trigger( action_t *action, action_state_t *state )
{
  dbc_proc_callback_t::trigger( action, state );

  if ( player->sim->debug )
  {
    // Debug reporting
    auto find_a = range::find_if( player->proc_tracking[ effect.name() ],
                                  [ & ]( action_t *it ) { return it->id == action->id; } );

    if ( find_a == player->proc_tracking[ effect.name() ].end() )
      player->proc_tracking[ effect.name() ].push_back( action );
  }
}

void monk_effect_callback_t::execute( action_t *action, action_state_t *state )
{
  if ( !state->target->is_sleeping() )
  {
    // Dynamically find and execute proc tracking
    auto effect_proc = player->find_proc( effect.trigger()->_name );
    if ( effect_proc )
      effect_proc->occur();
  }

  dbc_proc_callback_t::execute( action, state );
}

void monk_effect_callback_t::initialize()
{
  dbc_proc_callback_t::initialize();

  for ( const monk_effect_callback_t::post_init_callback_fn_t &fn : post_init_callbacks )
    fn( this );
}

monk_effect_callback_t *monk_effect_callback_t::register_post_init_callback(
    const monk_effect_callback_t::post_init_callback_fn_t &fn )
{
  post_init_callbacks.emplace_back( std::move( fn ) );
  return this;
}

monk_effect_callback_t *monk_effect_callback_t::register_callback_trigger_function(
    dbc_proc_callback_t::trigger_fn_type t, const dbc_proc_callback_t::trigger_fn_t &fn )
{
  player->callbacks.register_callback_trigger_function( effect.spell_id, t, fn );
  return this;
}

monk_effect_callback_t *monk_effect_callback_t::register_callback_execute_function(
    const dbc_proc_callback_t::execute_fn_t &fn )
{
  player->callbacks.register_callback_execute_function( effect.spell_id, fn );
  return this;
}

monk_effect_callback_t *monk_t::create_proc_callback( monk_callback_init_t params )
{
  special_effect_t *effect = new special_effect_t( this );

  effect->spell_id     = params.effect_driver->id();
  effect->cooldown_    = params.effect_driver->internal_cooldown();
  effect->proc_chance_ = params.effect_driver->proc_chance();
  effect->ppm_         = params.ppm ? params.ppm : params.effect_driver->_rppm;

  sim->print_debug( "initializing driver {} {} {}", effect->name_str, effect->spell_id, effect->cooldown_ );

  if ( !params.action_override )
  {
    // If we didn't define a custom action in initialization then
    // search action list for the first trigger we have a valid action for
    for ( const auto &e : params.effect_driver->effects() )
    {
      for ( auto t : action_list )
        if ( e.trigger()->ok() && t->id == e.trigger()->id() )
          params.action_override = t;

      if ( params.action_override )
        break;
    }
  }

  if ( params.action_override )
  {
    effect->name_str         = params.action_override->name_str;
    effect->trigger_str      = params.action_override->name_str;
    effect->trigger_spell_id = params.action_override->id;
    effect->action_disabled  = false;
    effect->execute_action   = effect->create_action();

    if ( !effect->execute_action )
      effect->execute_action = params.action_override;

    if ( params.action_override->harmful )
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
  effect->proc_flags_ = params.pf_override != 0ull ? params.pf_override : params.effect_driver->proc_flags();
  if ( params.pf2_override != 0ull )
    effect->proc_flags2_ = params.pf2_override;

  // We still haven't assigned a name, it is most likely a buff
  // dynamically find buff
  if ( effect->name_str == "" )
  {
    for ( const auto &e : params.effect_driver->effects() )
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

  special_effects.push_back( effect );  // Garbage collection

  return new monk_effect_callback_t( *effect, this );
}

void monk_t::init_special_effects()
{
  // TODO: CXX20: use designated initializers to make this suck less

  if ( talent.brewmaster.exploding_keg.ok() )
    create_proc_callback( { talent.brewmaster.exploding_keg.spell() } )
        ->register_callback_trigger_function( dbc_proc_callback_t::trigger_fn_type::CONDITION,
                                              [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t * ) {
                                                // Exploding keg damage is only triggered when the player buff is up,
                                                // regardless if the enemy has the debuff
                                                return buff.exploding_keg->check();
                                              } );

  if ( talent.windwalker.flurry_of_xuen.ok() )
    create_proc_callback( { talent.windwalker.flurry_of_xuen.spell() } )
        ->register_callback_trigger_function( dbc_proc_callback_t::trigger_fn_type::CONDITION,
                                              [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
                                                return state->action->id != action.flurry_of_xuen->id &&
                                                       state->action->id != action.empowered_tiger_lightning->id;
                                              } )
        ->register_callback_execute_function( [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          action.flurry_of_xuen->set_target( state->target );
          buff.flurry_of_xuen->trigger();
        } );

  if ( talent.monk.chi_burst->ok() && specialization() == MONK_WINDWALKER )
    create_proc_callback( { talent.monk.chi_burst.spell() } );

  if ( talent.brewmaster.spirit_of_the_ox->ok() )
    create_proc_callback( { talent.brewmaster.spirit_of_the_ox.spell() } )
        ->register_callback_trigger_function( dbc_proc_callback_t::trigger_fn_type::CONDITION,
                                              [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
                                                return state->action->id ==
                                                           talent.monk.rising_sun_kick->effectN( 1 ).trigger()->id() ||
                                                       state->action->id == baseline.brewmaster.blackout_kick->id();
                                              } )
        ->register_callback_execute_function( [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t * ) {
          buff.gift_of_the_ox->spawn_orb( 1 );
        } );

  if ( talent.master_of_harmony.aspect_of_harmony->ok() )
    create_proc_callback( { talent.master_of_harmony.aspect_of_harmony_driver,
                            static_cast<proc_flag>( PF_ALL_DAMAGE | PF_ALL_HEAL | PF_PERIODIC ), PF2_ALL_HIT } )
        ->register_callback_trigger_function( dbc_proc_callback_t::trigger_fn_type::TRIGGER,
                                              [ & ]( const dbc_proc_callback_t *, action_t *action, action_state_t * ) {
                                                // TODO: don't hardcode these ids
                                                constexpr std::array<unsigned, 6> blacklist = {
                                                    216521,  // celestial fortune
                                                    178173,  // goto expire
                                                    124507,  // goto trigger
                                                    387621,  // dragonfire brew
                                                    115129,  // expel harm damage
                                                    124255,  // stagger
                                                };
                                                if ( range::contains( blacklist, action->id ) )
                                                  return false;
                                                if ( action->allow_class_ability_procs )
                                                  return true;
                                                return false;
                                              } )
        ->register_callback_execute_function( [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          buff.aspect_of_harmony.trigger( state );
        } );

  if ( talent.master_of_harmony.balanced_stratagem->ok() )
    create_proc_callback( { talent.master_of_harmony.balanced_stratagem.spell() } )
        ->register_callback_trigger_function( dbc_proc_callback_t::trigger_fn_type::CONDITION,
                                              [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
                                                return state->action->school != SCHOOL_NONE;
                                              } )
        ->register_callback_execute_function( [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          if ( state->action->school == SCHOOL_PHYSICAL )
            buff.balanced_stratagem_magic->trigger();
          if ( state->action->school != SCHOOL_PHYSICAL )
            buff.balanced_stratagem_physical->trigger();
        } );

  if ( talent.conduit_of_the_celestials.courage_of_the_white_tiger->ok() )
    create_proc_callback( { talent.conduit_of_the_celestials.courage_of_the_white_tiger, static_cast<proc_flag>( 0ull ),
                            static_cast<proc_flag2>( 0ull ), action.courage_of_the_white_tiger.base } )
        ->register_callback_trigger_function( dbc_proc_callback_t::trigger_fn_type::CONDITION,
                                              [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
                                                if ( state->action->id == baseline.monk.tiger_palm->id() )
                                                  return true;
                                                return false;
                                              } );

  if ( talent.brewmaster.walk_with_the_ox.ok() )
  {
    action.walk_with_the_ox_rng = get_accumulated_rng(
        "walk_with_the_ox", 0.0075 * talent.brewmaster.walk_with_the_ox->effectN( 1 ).base_value() );
    create_proc_callback( { talent.brewmaster.walk_with_the_ox, static_cast<proc_flag>( 0ull ),
                            static_cast<proc_flag2>( 0ull ), action.walk_with_the_ox } )
        ->register_callback_trigger_function(
            dbc_proc_callback_t::trigger_fn_type::CONDITION,
            [ & ]( const dbc_proc_callback_t *dbc_proc_cb, action_t *, action_state_t * ) {
              if ( dbc_proc_cb->cooldown->down() )
                return false;
              dbc_proc_cb->cooldown->start();
              return static_cast<bool>( action.walk_with_the_ox_rng->trigger() );
            } );
  }

  if ( talent.shado_pan.stand_ready->ok() )
    create_proc_callback( { talent.shado_pan.stand_ready_driver } )
        ->register_callback_trigger_function(
            dbc_proc_callback_t::trigger_fn_type::CONDITION,
            [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t * ) { return buff.stand_ready->check(); } )
        ->register_callback_execute_function( [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t * ) {
          action.flurry_strikes->execute( actions::attacks::flurry_strikes_t::STAND_READY );
        } );

  if ( baseline.windwalker.empowered_tiger_lightning->ok() )
    create_proc_callback( { baseline.windwalker.empowered_tiger_lightning, PF_ALL_DAMAGE,
                            static_cast<proc_flag2>( PF2_ALL_HIT | PF2_PERIODIC_DAMAGE ) } )
        ->register_callback_execute_function( [ & ]( const dbc_proc_callback_t *, action_t *, action_state_t *state ) {
          monk_td_t *target_data = get_target_data( state->target );
          if ( !target_data )
            return;

          propagate_const<buff_t *> debuff = target_data->debuff.empowered_tiger_lightning;
          if ( !debuff )
            return;

          debug_cast<buffs::empowered_tiger_lightning_t *>( debuff.get() )->trigger( state );
        } )
        ->register_post_init_callback( []( monk_effect_callback_t *cb ) {
          cb->proc_chance                       = 1.0;
          cb->can_proc_from_procs               = true;
          cb->can_only_proc_from_class_abilites = true;
        } );

  base_t::init_special_effects();
}

void monk_t::init_finished()
{
  base_t::init_finished();
  parse_player_effects();
}

void monk_t::reset()
{
  base_t::reset();

  combo_strike_actions.clear();

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

double monk_t::composite_attack_power_multiplier() const
{
  double ap = base_t::composite_attack_power_multiplier();

  // TODO: implement using parse_effects
  ap *= 1.0 + cache.mastery() * baseline.brewmaster.mastery->effectN( 2 ).mastery_value();

  return ap;
}

double monk_t::composite_dodge() const
{
  double d = base_t::composite_dodge();

  if ( specialization() == MONK_BREWMASTER )
    d += buff.elusive_brawler->current_stack * cache.mastery_value();

  return d;
}

double monk_t::composite_player_target_armor( player_t *target ) const
{
  double armor = player_t::composite_player_target_armor( target );

  return armor;
}

void monk_t::invalidate_cache( cache_e c )
{
  base_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_ATTACK_POWER:
    case CACHE_AGILITY:
      base_t::invalidate_cache( CACHE_SPELL_POWER );
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

void monk_t::create_options()
{
  base_t::create_options();

  add_option( opt_int( "monk.initial_chi", user_options.initial_chi, 0, 6 ) );
  add_option( opt_int( "monk.chi_burst_healing_targets", user_options.chi_burst_healing_targets, 0, 30 ) );

  // shado-pan options
  add_option(
      opt_int( "monk.shado_pan.initial_charge_accumulator", user_options.shado_pan_initial_charge_accumulator, 0, 9 ) );
}

void monk_t::copy_from( player_t *source )
{
  base_t::copy_from( source );
  user_options = debug_cast<monk_t *>( source )->user_options;
}

resource_e monk_t::primary_resource() const
{
  return RESOURCE_ENERGY;
}

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

stat_e monk_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    case STAT_STR_AGI_INT:
      switch ( specialization() )
      {
        case MONK_BREWMASTER:
        case MONK_WINDWALKER:
          return STAT_AGILITY;
        default:
          return STAT_NONE;
      }
    case STAT_AGI_INT:
      return STAT_AGILITY;
    case STAT_STR_AGI:
      return STAT_AGILITY;
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_SPIRIT:
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

void monk_t::combat_begin()
{
  base_t::combat_begin();

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
}

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
      if ( talent.brewmaster.anvil_and_stave->ok() && cooldown.anvil_and_stave->up() )
      {
        cooldown.anvil_and_stave->start( talent.brewmaster.anvil_and_stave->internal_cooldown() );
        proc.anvil_and_stave->occur();
        baseline.brewmaster.brews.adjust(
            timespan_t::from_seconds( talent.brewmaster.anvil_and_stave->effectN( 1 ).base_value() / 10 ) );
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

void monk_t::target_mitigation( school_e school, result_amount_type dt, action_state_t *s )
{
  monk_td_t *target_td = get_target_data( s->action->player );

  // If Breath of Fire is ticking on the source target, the player receives 5% less damage
  if ( target_td->dot.breath_of_fire->is_ticking() )
  {
    double mult = 1.0;
    mult += action.breath_of_fire->data().effectN( 2 ).percent();
    s->result_amount *= mult;
  }

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

  // Gift of the Ox is no longer a random chance, under the hood. When you are hit, it increments a counter by
  // (DamageTakenBeforeAbsorbsOrStagger / MaxHealth). It now drops an orb whenever that reaches 1.0, and decrements it
  // by 1.0. The tooltip still says ‘chance’, to keep it understandable.
  if ( buff.gift_of_the_ox && s->action->id != baseline.brewmaster.stagger_self_damage->id() )
    buff.gift_of_the_ox->trigger_from_damage( s->result_amount );

  base_t::target_mitigation( school, dt, s );
}

void monk_t::assess_heal( school_e school, result_amount_type dmg_type, action_state_t *s )
{
  base_t::assess_heal( school, dmg_type, s );

  if ( specialization() == MONK_BREWMASTER )
    trigger_celestial_fortune( s );
}

void monk_t::create_actions()
{
  base_t::create_actions();
  buff.aspect_of_harmony.construct_actions( this );
}

std::unique_ptr<expr_t> monk_t::create_expression( std::string_view name_str )
{
  auto splits = util::string_split<std::string_view>( name_str, "." );

  if ( splits.size() >= 3 && splits[ 1 ] == "celestial_brew" && talent.brewmaster.celestial_infusion->ok() )
  {
    if ( splits[ 0 ] == "cooldown" )
      return get_cooldown( "celestial_infusion" )->create_expression( splits[ 2 ] );
    if ( splits[ 0 ] == "action" )
      return find_action( "celestial_infusion" )->create_expression( splits[ 2 ] );
    if ( splits[ 0 ] == "buff" )
    {
      buff_t *buff = buff_t::find( this, "celestial_infusion" );
      assert( buff );
      return buff_t::create_expression( splits[ 1 ], splits[ 2 ], *buff );
    }
  }

  return base_t::create_expression( name_str );
}

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
    auto ReportIssue = [ this ]( std::string_view desc, std::string_view date, bool match = false ) {
      monk_bug *new_issue = new monk_bug;
      new_issue->desc     = desc;
      new_issue->date     = date;
      new_issue->match    = match;
      issues.push_back( new_issue );
    };

    ReportIssue( "The ETL cache for both tigers resets to 0 when either spawn", "2023-08-03", true );
    ReportIssue( "Blackout Combo buffs both the initial and periodic effect of Breath of Fire", "2023-03-08", true );
    ReportIssue( "Memory of the Monastery stacks are overwritten each time the buff is applied", "2024-08-01", true );
    ReportIssue( "Chi Burst consumes both stacks of the buff on use", "2024-08-09", true );

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
  }

private:
  monk_t &p;
};

struct monk_module_t : public module_t
{
  monk_module_t() : module_t( MONK )
  {
  }

  player_t *create_player( sim_t *sim, std::string_view name, race_e race ) const override
  {
    monk_t *player           = new monk_t( sim, name, race );
    player->report_extension = std::make_unique<monk_report_t>( *player );
    return player;
  }

  bool valid() const override
  {
    return true;
  }

  void static_init() const override
  {
  }

  void register_hotfixes() const override
  {
  }

  void combat_begin( sim_t * ) const override
  {
  }

  void combat_end( sim_t * ) const override
  {
  }

  void init( player_t * ) const override
  {
  }
};
}  // end namespace monk

const module_t *module_t::monk()
{
  static monk::monk_module_t m;
  return &m;
}
