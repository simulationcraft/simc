// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_dragonflight.hpp"

#include "action/absorb.hpp"
#include "action/dot.hpp"
#include "actor_target_data.hpp"
#include "buff/buff.hpp"
#include "darkmoon_deck.hpp"
#include "dbc/item_database.hpp"
#include "ground_aoe.hpp"
#include "item/item.hpp"
#include "item/item_targetdata_initializer.hpp"
#include "set_bonus.hpp"
#include "player/action_priority_list.hpp"
#include "player/action_variable.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "sim/cooldown.hpp"
#include "sim/real_ppm.hpp"
#include "sim/sim.hpp"
#include "stats.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"

namespace unique_gear::dragonflight
{
namespace consumables
{
// TODO: implement reasonable model of losing buff due to damage taken. note that 'rotting from within' randomly procced
// from using toxic phial/potions will eventually cause you to lose the buff, even if you are taking 0 external damage.
void iced_phial_of_corrupting_rage( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "iced_phial_of_corrupting_rage" );
  if ( !buff )
  {
    auto crit = make_buff<stat_buff_t>( effect.player, "corrupting_rage", effect.player->find_spell( 374002 ) )
                    ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 2 ).average( effect.item ) )
                    ->set_quiet( false );

    buff = make_buff( effect.player, "iced_phial_of_corrupting_rage", effect.driver() )
      ->set_cooldown( 0_ms )
      ->set_chance( 1.0 )
      ->set_stack_change_callback( [ crit ]( buff_t*, int, int new_ ) {
        if ( new_ )
          crit->trigger();
        else
          crit->expire();
      } );

    struct overwhelming_rage_self_t : public generic_proc_t
    {
      double hp_pct;
      buff_t* crit;

      overwhelming_rage_self_t( const special_effect_t& e, buff_t* b )
        : generic_proc_t( e, "overwhelming_rage", e.player->find_spell( 374037 ) ),
          hp_pct( spell_data_t::find_spelleffect( data(), E_APPLY_AURA, A_PERIODIC_DAMAGE_PERCENT ).percent() ),
          crit( b )
      {
        not_a_proc = true;
        stats->type = stats_e::STATS_NEUTRAL;
        target = e.player;
      }

      // logs show this can trigger helpful periodic proc flags
      result_amount_type amount_type( const action_state_t*, bool ) const override
      {
        return result_amount_type::HEAL_OVER_TIME;
      }

      double base_ta( const action_state_t* s ) const override
      {
        return s->target->max_health() * hp_pct;
      }

      void last_tick( dot_t* d ) override
      {
        generic_proc_t::last_tick( d );

        crit->trigger();
      }
    };

    auto debuff = create_proc_action<overwhelming_rage_self_t>( "overwhelming_rage", effect, crit );

    crit->set_stack_change_callback( [ buff, debuff ]( buff_t*, int, int new_ ) {
      if ( !new_ && buff->check() )
        debuff->execute();
    } );

    if ( effect.player->sim->dragonflight_opts.corrupting_rage_uptime < 1 )
    {
      double debuff_uptime = 1 - effect.player->sim->dragonflight_opts.corrupting_rage_uptime;
      double disable_chance =
          debuff_uptime / ( debuff->dot_duration - debuff->dot_duration * debuff_uptime ).total_seconds();

      crit->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
      effect.player->register_combat_begin( [ crit, disable_chance ]( player_t* ) {
        make_repeating_event( crit->player->sim, 1_s, [ crit, disable_chance ]() {
          if ( crit->rng().roll( disable_chance ) )
            crit->expire();
        } );
      } );
    }
  }

  effect.custom_buff = buff;
}

// TODO: implement reasonable model of losing buff due to other players coming too close. note that
// driver()->effectN(2)->trigger() is called every 1s to check within a 10yd radius for close players.
void phial_of_charged_isolation( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, effect.name() );
  if ( !buff )
  {
    auto amount = effect.driver()->effectN( 1 ).average( effect.item );
    auto linger_mul = effect.driver()->effectN( 2 ).percent();

    auto stat_buff =
        make_buff<stat_buff_t>( effect.player, "phial_of_charged_isolation_stats", effect.player->find_spell( 371387 ) )
            ->add_stat_from_effect( 1, amount );

    auto linger_buff =
        make_buff<stat_buff_t>( effect.player, "phial_of_charged_isolation_linger", effect.player->find_spell( 384713 ) )
            ->add_stat_from_effect( 1, amount * linger_mul );

    buff = make_buff( effect.player, effect.name(), effect.driver() )
      ->set_stack_change_callback( [ stat_buff ]( buff_t*, int, int new_ ) {
        if ( new_ )
          stat_buff->trigger();
        else
          stat_buff->expire();
      } );

    stat_buff->set_stack_change_callback( [ buff, linger_buff ]( buff_t*, int, int new_ ) {
      if ( !new_ && buff->check() )
        linger_buff->trigger();
    } );
  }

  effect.custom_buff = buff;
}

void phial_of_elemental_chaos( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "elemental_chaos_fire", "elemental_chaos_air", "elemental_chaos_earth",
                                        "elemental_chaos_frost" } ) )
  {
    return;
  }

  auto buff = buff_t::find( effect.player, effect.name() );
  if ( !buff )
  {
    std::vector<buff_t*> buff_list;
    auto amount = effect.driver()->effectN( 1 ).average( effect.item );
    auto duration = effect.driver()->effectN( 1 ).period();

    effect.player->buffs.elemental_chaos_fire = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_fire", effect.player->find_spell( 371348 ) )
            ->add_stat( STAT_CRIT_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_CRIT_DAMAGE_BONUS )
            ->set_duration( duration ) );
    effect.player->buffs.elemental_chaos_air = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_air", effect.player->find_spell( 371350 ) )
            ->add_stat( STAT_HASTE_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS )
            ->add_invalidate( CACHE_RUN_SPEED )
            ->set_duration( duration ) );
    effect.player->buffs.elemental_chaos_earth = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_earth", effect.player->find_spell( 371351 ) )
            ->add_stat( STAT_MASTERY_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
            ->set_duration( duration ) );
    effect.player->buffs.elemental_chaos_frost = buff_list.emplace_back(
        make_buff<stat_buff_t>( effect.player, "elemental_chaos_frost", effect.player->find_spell( 371353 ) )
            ->add_stat( STAT_VERSATILITY_RATING, amount )
            ->set_default_value_from_effect_type( A_MOD_CRITICAL_HEALING_AMOUNT )
            ->set_duration( duration ) );

    buff = make_buff( effect.player, effect.name(), effect.driver() )
      ->set_tick_callback( [ buff_list ]( buff_t* b, int, timespan_t ) {
        buff_list[ b->rng().range( buff_list.size() ) ]->trigger();
      } );
  }

  effect.custom_buff = buff;
}

void phial_of_glacial_fury( special_effect_t& effect )
{
  // Damage proc
  struct glacial_fury_t : public generic_aoe_proc_t
  {
    const buff_t* buff;
    glacial_fury_t( const special_effect_t& e, const buff_t* b )
      : generic_aoe_proc_t( e, "glacial_fury", e.driver()->effectN( 2 ).trigger(), true ), buff( b )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 3 ).average( e.item );
    }

    double composite_aoe_multiplier( const action_state_t* s ) const override
    {
      // spell data is flagged to ignore all caster multpliers, so da_multiplier is not snapshot. however, the 15% per
      // buff does take effect and as this is an aoe action, we can use composite_aoe_multiplier without having  to
      // adjust snapshot flags.
      return generic_aoe_proc_t::composite_aoe_multiplier( s ) * ( 1.0 + buff->check_stack_value() );
    }
  };

  // Buff that triggers when you hit a new target
  auto new_target_buff = create_buff<buff_t>( effect.player, effect.player->find_spell( 373265 ) );
  new_target_buff->set_default_value( effect.driver()->effectN( 4 ).percent() );

  // effect to trigger damage proc
  auto damage_effect            = new special_effect_t( effect.player );
  damage_effect->type           = SPECIAL_EFFECT_EQUIP;
  damage_effect->source         = SPECIAL_EFFECT_SOURCE_ITEM;
  damage_effect->spell_id       = effect.spell_id;
  damage_effect->cooldown_      = 0_ms;
  damage_effect->execute_action = create_proc_action<glacial_fury_t>( "glacial_fury", effect, new_target_buff );
  effect.player->special_effects.push_back( damage_effect );

  // callback to proc damage
  auto damage_cb = new dbc_proc_callback_t( effect.player, *damage_effect );
  damage_cb->initialize();
  damage_cb->deactivate();

  // effect to trigger new target buff
  auto new_target_effect          = new special_effect_t( effect.player );
  new_target_effect->name_str     = "glacial_fury_new_target";
  new_target_effect->type         = SPECIAL_EFFECT_EQUIP;
  new_target_effect->source       = SPECIAL_EFFECT_SOURCE_ITEM;
  new_target_effect->proc_chance_ = 1.0;
  new_target_effect->proc_flags_  = PF_ALL_DAMAGE | PF_PERIODIC;
  new_target_effect->proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new_target_effect->custom_buff  = new_target_buff;
  effect.player->special_effects.push_back( new_target_effect );

  // callback to trigger buff on attacking a new target
  struct glacial_fury_buff_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    glacial_fury_buff_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( !a->harmful )
        return;

      if ( range::contains( target_list, s->target->actor_spawn_index ) )
        return;

      dbc_proc_callback_t::execute( a, s );
      target_list.push_back( s->target->actor_spawn_index );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      target_list.clear();
    }
  };

  auto new_target_cb = new glacial_fury_buff_cb_t( *new_target_effect );
  new_target_cb->initialize();
  new_target_cb->deactivate();

  // Phial buff itself
  auto buff = create_buff<buff_t>( effect.player, effect.driver() );
  buff->set_chance( 1.0 )
      ->set_stack_change_callback( [ damage_cb, new_target_cb ]( buff_t*, int, int new_ ) {
        if ( new_ )
        {
          damage_cb->activate();
          new_target_cb->activate();
        }
        else
        {
          damage_cb->deactivate();
          new_target_cb->deactivate();
        }
      } );

  effect.custom_buff = buff;
}

// TODO: implement reasonable model of losing buff due to movement
void phial_of_static_empowerment( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, effect.name() );
  if ( !buff )
  {
    auto primary = make_buff<stat_buff_t>( effect.player, "static_empowerment", effect.player->find_spell( 370772 ) );
    primary->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect.item ) / primary->max_stack() );
    primary->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );

    buff = make_buff( effect.player, effect.name(), effect.driver() )
      ->set_stack_change_callback( [ primary ]( buff_t*, int, int new_ ) {
        if ( new_ )
          primary->trigger();
        else
          primary->expire();
      } );

    auto speed = make_buff<stat_buff_t>( effect.player, "mobile_empowerment", effect.player->find_spell( 370773 ) )
      ->add_stat( STAT_SPEED_RATING, effect.driver()->effectN( 2 ).average( effect.item ) )
      ->set_stack_change_callback( [ buff, primary ]( buff_t*, int, int new_ ) {
        if ( !new_ && buff->check() )
          primary->trigger();
      } );

    primary->set_stack_change_callback( [ buff, speed ]( buff_t*, int, int new_ ) {
      if ( !new_ && buff->check() )
        speed->trigger();
    } );

    effect.player->buffs.static_empowerment = primary;
  }

  effect.custom_buff = buff;
}

void bottled_putrescence( special_effect_t& effect )
{
  struct bottled_putrescence_t : public proc_spell_t
  {
    struct bottled_putrescence_tick_t : public proc_spell_t
    {
      bottled_putrescence_tick_t( const special_effect_t& e )
        : proc_spell_t( "bottled_putrescence_tick", e.player, e.player->find_spell( 371993 ) )
      {
        dual = ground_aoe = true;
        aoe = -1;
        // every tick damage is rounded down
        base_dd_min = base_dd_max = util::floor( e.driver()->effectN( 1 ).average( e.item ) );
      }

      result_amount_type amount_type( const action_state_t*, bool ) const override
      {
        return result_amount_type::DMG_OVER_TIME;
      }
    };

    action_t* damage;
    timespan_t duration;

    bottled_putrescence_t( const special_effect_t& e )
      : proc_spell_t( "bottled_putrescence", e.player, e.driver() ),
        damage( create_proc_action<bottled_putrescence_tick_t>( "bottled_putresence", e ) ),
        duration( data().effectN( 1 ).trigger()->duration() * inhibitor_mul( e.player ) )
    {
      damage->stats = stats;
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( s->target )
        .pulse_time( damage->base_tick_time )
        .duration( duration - 1_ms )  // has 0tick, but no tick at the end
        .action( damage ), true );
    }
  };

  effect.execute_action = create_proc_action<bottled_putrescence_t>( "bottled_putrescence", effect );
}

void chilled_clarity( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "potion_of_chilled_clarity" } ) )
    return;

  auto buff = create_buff<buff_t>( effect.player, effect.trigger() );
  buff->set_default_value_from_effect_type( A_355 )
      ->set_duration( timespan_t::from_seconds( effect.driver()->effectN( 1 ).base_value() ) )
      ->set_duration_multiplier( inhibitor_mul( effect.player ) );

  effect.custom_buff = effect.player->buffs.chilled_clarity = buff;
}

void elemental_power( special_effect_t& effect )
{
  effect.duration_ = effect.duration() * inhibitor_mul( effect.player );
}

void shocking_disclosure( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "shocking_disclosure" );
  if ( !buff )
  {
    auto damage =
      create_proc_action<generic_aoe_proc_t>( "shocking_disclosure", effect, "shocking_disclosure", effect.trigger() );
    damage->split_aoe_damage = false;
    damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    damage->dual = true;

    if ( auto pot_action = effect.player->find_action( "potion" ) )
      damage->stats = pot_action->stats;

    buff = make_buff( effect.player, "shocking_disclosure", effect.driver() )
      ->set_duration_multiplier( inhibitor_mul( effect.player ) )
      ->set_tick_callback( [ damage ]( buff_t* b, int, timespan_t ) {
        damage->execute_on_target( b->player->target );
      } );
  }

  effect.custom_buff = buff;
}
}  // namespace consumables

namespace enchants
{
custom_cb_t writ_enchant( stat_e stat )
{
  return [ stat ]( special_effect_t& effect ) {
    auto amount = effect.driver()->effectN( 1 ).average( effect.player );

    auto new_driver = effect.trigger();
    auto new_trigger = new_driver->effectN( 1 ).trigger();

    std::string buff_name = new_trigger->name_cstr();
    util::tokenize( buff_name );
    auto buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, buff_name ) );
    auto buff_stat = STAT_NONE;

    if ( stat == STAT_NONE )
    {
      buff_stat = util::translate_rating_mod( new_trigger->effectN( 1 ).misc_value1() );
    }
    else
    {
      buff_stat = effect.player->convert_hybrid_stat( stat );
    }

    if ( buff == nullptr )
    {
      buff = create_buff<stat_buff_t>( effect.player, buff_name, new_trigger );
    }

    buff->add_stat( buff_stat, amount );

    effect.name_str = buff_name;
    effect.custom_buff = buff;
    effect.spell_id = new_driver->id();

    new dbc_proc_callback_t( effect.player, effect );
  };
}

void frozen_devotion( special_effect_t& effect )
{
  auto new_driver = effect.driver()->effectN( 1 ).trigger();
  auto proc = create_proc_action<generic_aoe_proc_t>( "frozen_devotion", effect, "frozen_devotion",
                                                      new_driver->effectN( 1 ).trigger(), true );

  proc->base_dd_min += effect.driver()->effectN( 1 ).average( effect.player );
  proc->base_dd_max += effect.driver()->effectN( 1 ).average( effect.player );

  effect.name_str = new_driver->name_cstr();
  effect.spell_id = new_driver->id();
  effect.execute_action = proc;

  new dbc_proc_callback_t( effect.player, effect );
}

void wafting_devotion( special_effect_t& effect )
{
  auto new_driver = effect.trigger();
  auto new_trigger = new_driver->effectN( 1 ).trigger();

  auto haste = effect.driver()->effectN( 1 ).average( effect.player );
  auto speed = effect.driver()->effectN( 2 ).average( effect.player );

  std::string buff_name = new_trigger->name_cstr();
  util::tokenize( buff_name );

  auto buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, buff_name ) );
  if ( buff == nullptr )
  {
    buff = create_buff<stat_buff_t>( effect.player, buff_name, new_trigger );
  }

  buff->add_stat( STAT_HASTE_RATING, haste )
      ->add_stat( STAT_SPEED_RATING, speed );

  effect.name_str = buff_name;
  effect.custom_buff = buff;
  effect.spell_id = new_driver->id();
  effect.trigger_spell_id = new_trigger->id();

  new dbc_proc_callback_t( effect.player, effect );
}

void completely_safe_rockets( special_effect_t& effect )
{

  auto missile = effect.trigger();
  auto blast = missile -> effectN( 1 ).trigger();

  auto blast_proc = create_proc_action<generic_aoe_proc_t>( "completely_safe_rocket_blast", effect, "completely_safe_rocket_blast", blast, true );
  blast_proc -> base_dd_min = blast_proc -> base_dd_max = effect.driver() -> effectN( 1 ).average( effect.player );
  blast_proc -> travel_speed = missile -> missile_speed();

  effect.execute_action = blast_proc;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;

  new dbc_proc_callback_t( effect.player, effect );
}

void high_intensity_thermal_scanner( special_effect_t& effect )
{
  double value = effect.driver()->effectN( 1 ).average( effect.player );

  auto crit_buff = create_buff<stat_buff_t>( effect.player, "high_intensity_thermal_scanner_crit", effect.trigger() );
  crit_buff->add_stat( STAT_CRIT_RATING, value );

  auto vers_buff = create_buff<stat_buff_t>( effect.player, "high_intensity_thermal_scanner_vers", effect.trigger() );
  vers_buff->add_stat( STAT_VERSATILITY_RATING, value );

  auto haste_buff = create_buff<stat_buff_t>( effect.player, "high_intensity_thermal_scanner_haste", effect.trigger() );
  haste_buff->add_stat( STAT_HASTE_RATING, value );

  auto mastery_buff = create_buff<stat_buff_t>( effect.player, "high_intensity_thermal_scanner_mastery", effect.trigger() );
  mastery_buff->add_stat( STAT_MASTERY_RATING, value );

  struct buff_cb_t : public dbc_proc_callback_t
  {
    buff_t* crit;
    buff_t* vers;
    buff_t* haste;
    buff_t* mastery;

    buff_cb_t( const special_effect_t& e, buff_t* crit, buff_t* vers, buff_t* haste, buff_t* mastery ) : dbc_proc_callback_t( e.player, e ),
      crit( crit ), vers( vers ), haste( haste ), mastery( mastery )
    {}

    void execute( action_t*, action_state_t* s ) override
    {
      switch (s -> target -> race)
      {
      case RACE_MECHANICAL:
      case RACE_DRAGONKIN:
        mastery->trigger();
        break;
      case RACE_ELEMENTAL:
      case RACE_BEAST:
        crit->trigger();
        break;
      case RACE_UNDEAD:
      case RACE_HUMANOID:
        vers->trigger();
        break;
      case RACE_GIANT:
      case RACE_DEMON:
      case RACE_ABERRATION:
      default:
        haste->trigger();
        break;
      }
    }
  };

  new buff_cb_t( effect, crit_buff, vers_buff, haste_buff, mastery_buff );
}

void gyroscopic_kaleidoscope( special_effect_t& effect )
{
  int buff_id;
  switch ( effect.driver() -> id() )
  {
  case 385892:
    buff_id = 385891;
    break;
  case 385886:
    buff_id = 385885;
    break;
  case 385765:
  default:
    buff_id = 385860;
    break;
  }

  auto buff_data = effect.player -> find_spell( buff_id );
  double stat = buff_data -> effectN( 1 ).average( effect.player );

  auto buff = create_buff<stat_buff_t>( effect.player, "gyroscopic_kaleidoscope", buff_data );
  buff -> add_stat( util::parse_stat_type( effect.player -> dragonflight_opts.gyroscopic_kaleidoscope_stat ), stat );

  effect.player->register_combat_begin( [ buff ]( player_t* ) {
    buff -> trigger();
    make_repeating_event( buff -> player -> sim, buff -> buff_duration() , [ buff ]() { buff -> trigger(); } );
  } );
}

void projectile_propulsion_pinion( special_effect_t& effect )
{
  auto buff_data = effect.player -> find_spell( 385942 );
  double value = effect.driver()->effectN( 1 ).average( effect.player ) / 2;

  auto crit_buff = create_buff<stat_buff_t>( effect.player, "projectile_propulsion_pinion_crit", buff_data );
  crit_buff->add_stat( STAT_CRIT_RATING, value );

  auto vers_buff = create_buff<stat_buff_t>( effect.player, "projectile_propulsion_pinion_vers", buff_data );
  vers_buff->add_stat( STAT_VERSATILITY_RATING, value );

  auto haste_buff = create_buff<stat_buff_t>( effect.player, "projectile_propulsion_pinion_haste", buff_data );
  haste_buff->add_stat( STAT_HASTE_RATING, value );

  auto mastery_buff = create_buff<stat_buff_t>( effect.player, "projectile_propulsion_pinion_mastery", buff_data );
  mastery_buff->add_stat( STAT_MASTERY_RATING, value );

  std::map<stat_e, stat_buff_t*> buffs = {
    { STAT_CRIT_RATING, crit_buff },
    { STAT_VERSATILITY_RATING, vers_buff },
    { STAT_HASTE_RATING, haste_buff },
    { STAT_MASTERY_RATING, mastery_buff },
  };

  effect.player->register_combat_begin( [ buffs ]( player_t* p ) {
    static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };
    buffs.find( util::highest_stat( p, ratings ) ) -> second -> trigger();
    buffs.find( util::lowest_stat( p, ratings ) ) -> second -> trigger();
  } );
}

void temporal_spellthread( special_effect_t& effect )
{
  effect.player->resources.base_multiplier[ RESOURCE_MANA ] *= 1.0 + effect.driver()->effectN( 1 ).percent();
}

// 406764 - DoT
// 406770 - Self Damage (TODO)
void shadowflame_wreathe( special_effect_t& effect )
{
  auto new_driver = effect.driver()->effectN( 1 ).trigger();
  auto dot        = create_proc_action<generic_proc_t>( "shadowflame_wreathe", effect, "shadowflame_wreathe",
                                                 new_driver->effectN( 1 ).trigger() );

  dot->dot_behavior = DOT_CLIP; // ticks are lost on refresh
  dot->base_td      += effect.driver()->effectN( 1 ).average( effect.player );

  effect.name_str       = new_driver->name_cstr();
  effect.spell_id       = new_driver->id();
  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );
}

}  // namespace enchants

namespace items
{
// Trinkets

void dragonfire_bomb_dispenser( special_effect_t& effect )
{
  // Skilled Restock
  auto restock_driver = find_special_effect( effect.player, 408667 );
  if ( restock_driver )
  {
    auto restock_buff = create_buff<buff_t>( effect.player, effect.player->find_spell( 408770 ) )
      ->set_quiet( true )
      ->set_stack_change_callback( [ & ]( buff_t* buff, int, int ) {
        if ( buff->at_max_stacks() )
        {
          // At 60 stacks gain a charge
          effect.execute_action->cooldown->reset( true );
          buff->expire();
        }
      } );

    restock_driver->custom_buff = restock_buff;
    restock_driver->proc_flags2_ = PF2_CRIT;

    new dbc_proc_callback_t( effect.player, *restock_driver );
  }

  auto coeff_data = effect.player->find_spell( 408667 );

  // AoE Explosion
  auto explode = create_proc_action<generic_aoe_proc_t>( "dragonfire_bomb_aoe", effect, "dragonfire_bomb_aoe", 408694 );
  explode->name_str_reporting = "AOE";
  explode->base_dd_min = explode->base_dd_max = coeff_data->effectN( 2 ).average( effect.item );
  effect.player->register_on_kill_callback( [ p = effect.player, explode ]( player_t* t ) {
    if ( p->sim->event_mgr.canceled )
      return;

    auto td = p->find_target_data( t );
    if ( td && td->debuff.dragonfire_bomb->check() )
      explode->execute_on_target( t );
  } );

  // ST Damage
  auto st = create_proc_action<generic_proc_t>( "dragonfire_bomb_st", effect, "dragonfire_bomb_st", 408682 );
  st->name_str_reporting = "ST";
  st->base_dd_min = st->base_dd_max = coeff_data->effectN( 1 ).average( effect.item );

  // DoT Driver
  struct dragonfire_bomb_missile_t : public proc_spell_t
  {
    dragonfire_bomb_missile_t( const special_effect_t& e )
      : proc_spell_t( "dragonfire_bomb_dispenser", e.player, e.player->find_spell( e.spell_id ), e.item )
    {}

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      auto td = player->get_target_data( s->target );

      if ( td )
      {
        if ( td->debuff.dragonfire_bomb->check() )
          td->debuff.dragonfire_bomb->expire();

        td->debuff.dragonfire_bomb->trigger();
      }
    }
  };

  // TODO: the driver has two cooldown categories, 1141 for the on-use and 2170 for the charge. currently the generation
  // script prioritizes the charge category so we manually set it here until the script can be adjusted.
  effect.cooldown_category_ = 1141;
  effect.execute_action = create_proc_action<dragonfire_bomb_missile_t>( "dragonfire_bomb_dispenser", effect );
}

struct dragonfire_bomb_dispenser_initializer_t : public item_targetdata_initializer_t
{
  dragonfire_bomb_dispenser_initializer_t() : item_targetdata_initializer_t( 408671 ) {}

  void operator()( actor_target_data_t* td ) const override
  {
    if ( !find_effect( td->source ) )
    {
      td->debuff.dragonfire_bomb = make_buff( *td, "dragonfire_bomb" )->set_quiet( true );
      return;
    }

    struct dragonfire_bomb_debuff_t : buff_t
    {
      action_t* bomb;

      dragonfire_bomb_debuff_t( actor_target_data_t& td )
        : buff_t( td, "dragonfire_bomb", td.source->find_spell( 408675 ) ),
          bomb( td.source->find_action( "dragonfire_bomb_st" ) )
      {}

      void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
      {
        buff_t::expire_override( expiration_stacks, remaining_duration );

        if ( bomb )
          bomb->execute_on_target( player );
      }
    };

    td->debuff.dragonfire_bomb = make_buff<dragonfire_bomb_debuff_t>( *td );  // IT GONNA BLOW!
    td->debuff.dragonfire_bomb->reset();
  }
};

/* Burgeoning Seed
  * id=193634
  * trigger = 382108
  * consume = 384636
  * buff = 384658
*/

void consume_pods( special_effect_t& effect )
{
  auto life_pod = find_special_effect( effect.player, 382108 );

  if ( life_pod != nullptr )
  {
    life_pod->custom_buff = make_buff( effect.player, "brimming_life_pod", life_pod->trigger(), effect.item );
    new dbc_proc_callback_t( effect.player, *life_pod );
  }

  struct burgeoning_seed_t : generic_proc_t
  {
    buff_t* pods;
    buff_t* supernatural;

    burgeoning_seed_t( const special_effect_t& effect )
      : generic_proc_t( effect, "burgeoning_seed", effect.trigger() ),
      pods ( buff_t::find( effect.player, "brimming_life_pod" ) ),
      supernatural( buff_t::find( effect.player, "supernatural" ) )
    {
      if ( !supernatural )
        supernatural = create_buff<stat_buff_t>( effect.player, "supernatural", effect.player->find_spell( 384658 ) )
        ->add_stat( STAT_VERSATILITY_RATING, effect.player->find_spell( 384658 )->effectN( 1 ).average( effect.item ) )
        ->add_stat( STAT_MAX_HEALTH, effect.player->find_spell( 384658 )->effectN( 3 ).average( effect.item ) );
    }

    bool ready() override
    {
      return ( pods && pods->check() );
    }

    void execute() override
    {
      supernatural->trigger( pods->stack() );
      pods->expire();
    }
  };

  effect.execute_action = create_proc_action<burgeoning_seed_t>( "burgeoning_seed", effect );
}

custom_cb_t idol_of_the_aspects( std::string_view type )
{
  return [ type ]( special_effect_t& effect ) {
    int gems = 0;
    for ( const auto& item : effect.player->items )
      for ( auto gem_id : item.parsed.gem_id )
        if ( gem_id && util::str_in_str_ci( effect.player->dbc->item( gem_id ).name, type ) )
          gems++;

    if ( !gems )
      return;

    auto val = effect.driver()->effectN( 3 ).average( effect.item ) / 4;
    auto gift = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 376643 ) );
    gift->set_stat( STAT_CRIT_RATING, val )
        ->add_stat( STAT_HASTE_RATING, val )
        ->add_stat( STAT_MASTERY_RATING, val )
        ->add_stat( STAT_VERSATILITY_RATING, val );

    auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
      ->set_stat_from_effect( 1, std::floor( effect.driver()->effectN( 1 ).average( effect.item ) ) )
      ->set_max_stack( as<int>( effect.driver()->effectN( 2 ).base_value() ) )
      ->set_expire_at_max_stack( true )
      ->set_stack_change_callback( [ gift ]( buff_t*, int, int new_ ) {
        if ( !new_ )
          gift->trigger();
      } );

    effect.custom_buff = buff;

    auto cb = new dbc_proc_callback_t( effect.player, effect );

    gift->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
      if ( new_ )
        cb->deactivate();
      else
        cb->activate();
    } );

    effect.player->callbacks.register_callback_execute_function(
        effect.driver()->id(), [ buff, gems ]( const dbc_proc_callback_t*, action_t*, action_state_t* ) {
          buff->trigger( gems );
        } );
  };
}

struct DF_darkmoon_deck_t : public darkmoon_spell_deck_t
{
  bool bronzescale;  // shuffles every 2.5s
  bool azurescale;   // shuffles from greatest to least
  bool emberscale;   // no longer shuffles even cards
  bool jetscale;     // no longer shuffles on Ace
  bool sagescale;    // only shuffle on jump NYI

  bool reset = false; // whether to reset the deck on shuffle

  DF_darkmoon_deck_t( const special_effect_t& e, std::vector<unsigned> c )
    : darkmoon_spell_deck_t( e, std::move( c ) ),
      bronzescale( unique_gear::find_special_effect( player, 382913 ) != nullptr ),
      azurescale( unique_gear::find_special_effect( player, 383336 ) != nullptr ),
      emberscale( unique_gear::find_special_effect( player, 383333 ) != nullptr ),
      jetscale( unique_gear::find_special_effect( player, 383337 ) != nullptr ),
      sagescale( unique_gear::find_special_effect( player, 383339 ) != nullptr )
  {
    if ( bronzescale )
      shuffle_period = player->find_spell( 382913 )->effectN( 1 ).period();
  }

  // TODO: currently in-game the shuffling does not start until the category cooldown 2042 is up. this is currently
  // inconsistently applied between decks, with some having 20s and others have 90s. adjust to the correct behavior when
  // DF goes live.
  void shuffle() override
  {
    // NOTE: assumes last card, and thus highest index, is Ace
    if ( !reset && jetscale && top_index == cards.size() - 1 )
      return;

    reset = false;
    darkmoon_spell_deck_t::shuffle();
  }

  size_t get_index( bool init ) override
  {
    if ( azurescale )
    {
      if ( init )
      {
        // when you equip it you get the 7 card, but the first shuffle in combat remains at 7. for our purposes since we
        // set the top card on initialization and shuffle again in combat_begin, we initialize to the 8 card, which is
        // second to last.
        top_index = card_ids.size() - 2;
      }
      else
      {
        if ( top_index == 0 )
          top_index = card_ids.size() - 1;
        else
          top_index--;
      }
    }
    else if ( emberscale )
    {
      top_index = player->rng().range( card_ids.size() / 2 ) * 2 + 1;
    }
    else
    {
      darkmoon_spell_deck_t::get_index();
    }

    return top_index;
  }
};

template <typename Base = proc_spell_t>
struct DF_darkmoon_proc_t : public darkmoon_deck_proc_t<Base, DF_darkmoon_deck_t>
{
  using base_t = darkmoon_deck_proc_t<Base, DF_darkmoon_deck_t>;

  DF_darkmoon_proc_t( const special_effect_t& e, std::string_view n, unsigned shuffle_id, std::vector<unsigned> cards )
    : base_t( e, n, shuffle_id, std::move( cards ) )
  {}

  void execute() override
  {
    base_t::execute();

    base_t::deck->reset = true;

    if ( base_t::deck->shuffle_event )
      base_t::deck->shuffle_event->reschedule( base_t::cooldown->base_duration );
  }
};

void darkmoon_deck_dance( special_effect_t& effect )
{
  struct darkmoon_deck_dance_t : public DF_darkmoon_proc_t<>
  {
    action_t* damage;
    action_t* heal;

    // card order is [ 2 3 4 5 6 7 8 A ]
    darkmoon_deck_dance_t( const special_effect_t& e )
      : DF_darkmoon_proc_t( e, "refreshing_dance", 382958,
                            { 382861, 382862, 382863, 382864, 382865, 382866, 382867, 382860 } )
    {
      damage =
        create_proc_action<generic_proc_t>( "refreshing_dance_damage", e, "refreshing_dance_damage", 384613 );
      damage->background = damage->dual = true;
      damage->stats = stats;

      // TODO: this value is wrong. unknown which coefficient is used, as the heal itself has a 5% variable so precise
      // comparison to spell query scaled values is difficult.
      heal =
        create_proc_action<base_generic_proc_t<proc_heal_t>>( "refreshing_dance_heal", e, "refreshing_dance_heal", 384624 );
      heal->name_str_reporting = "Heal";
      heal->background = heal->dual = true;
    }

    void impact( action_state_t* s ) override
    {
      DF_darkmoon_proc_t::impact( s );

      damage->execute_on_target( s->target );
      heal->stats->add_execute( sim->current_time(), s->target );

      timespan_t delay = 0_ms;
      for ( size_t i = 0; i < 5 + deck->top_index; i++ )
      {
        // As this chains to the nearest, assume 5yd melee range
        delay += timespan_t::from_seconds( rng().gauss( 5.0 / travel_speed, sim->travel_variance ) );

        if ( i % 2 )
          make_event( *sim, delay, [ t = s->target, this ] { damage->execute_on_target( t ); } );
        else
          make_event( *sim, delay, [ this ] { heal->execute_on_target( player ); } );
      }
    }
  };

  effect.execute_action = create_proc_action<darkmoon_deck_dance_t>( "refreshing_dance", effect );
}

void darkmoon_deck_inferno( special_effect_t& effect )
{
  struct darkmoon_deck_inferno_t : public DF_darkmoon_proc_t<>
  {
    // card order is [ 2 3 4 5 6 7 8 A ]
    darkmoon_deck_inferno_t( const special_effect_t& e )
      : DF_darkmoon_proc_t( e, "darkmoon_deck_inferno", 382958,
                            { 382837, 382838, 382839, 382840, 382841, 382842, 382843, 382835 } )
    {}

    double get_mult( size_t index ) const
    {
      // 7 of fires (index#5) does the base spell_data damage. each card above/below adjusts by 10%
      return 1.0 + ( as<int>( index ) - 5 ) * 0.1;
    }

    double action_multiplier() const override
    {
      return DF_darkmoon_proc_t::action_multiplier() * get_mult( deck->top_index );
    }
  };

  effect.execute_action = create_proc_action<darkmoon_deck_inferno_t>( "darkmoon_deck_inferno", effect );
}

struct awakening_rime_initializer_t : public item_targetdata_initializer_t
{
  awakening_rime_initializer_t() : item_targetdata_initializer_t( 386624 ) {}

  void operator()( actor_target_data_t* td ) const override
  {
    if ( !find_effect( td->source ) )
    {
      td->debuff.awakening_rime = make_buff( *td, "awakening_rime" )->set_quiet( true );
      return;
    }

    assert( !td->debuff.awakening_rime );
    td->debuff.awakening_rime = make_buff( *td, "awakening_rime", td->source->find_spell( 386623 ) );
    td->debuff.awakening_rime->reset();
  }
};

void darkmoon_deck_rime( special_effect_t& effect )
{
  struct darkmoon_deck_rime_t : public DF_darkmoon_proc_t<>
  {
    // card order is [ 2 3 4 5 6 7 8 A ]
    darkmoon_deck_rime_t( const special_effect_t& e )
      : DF_darkmoon_proc_t( e, "awakening_rime", 382958,
                            { 382845, 382846, 382847, 382848, 382849, 382850, 382851, 382844 } )
    {
      // TODO: determine if damage increased per target hit
      auto explode =
          create_proc_action<generic_aoe_proc_t>( "awakened_rime", e, "awakened_rime", e.player->find_spell( 370880 ) );

      e.player->register_on_kill_callback( [ e, explode ]( player_t* t ) {
        if ( e.player->sim->event_mgr.canceled )
          return;

        auto td = e.player->find_target_data( t );
        if ( td && td->debuff.awakening_rime->check() )
          explode->execute_on_target( t );
      } );
    }

    timespan_t get_duration( size_t index ) const
    {
      return 5_s + timespan_t::from_seconds( index );
    }

    void impact( action_state_t* s ) override
    {
      DF_darkmoon_proc_t::impact( s );

      auto td = player->get_target_data( s->target );
      td->debuff.awakening_rime->trigger( get_duration( deck->top_index ) );
    }
  };

  effect.execute_action = create_proc_action<darkmoon_deck_rime_t>( "awakening_rime", effect );
}

void darkmoon_deck_watcher( special_effect_t& effect )
{
  struct darkmoon_deck_watcher_t : public DF_darkmoon_proc_t<proc_heal_t>
  {
    struct watchers_blessing_absorb_buff_t : public absorb_buff_t
    {
      buff_t* stat;

      watchers_blessing_absorb_buff_t( const special_effect_t& e )
        : absorb_buff_t( e.player, "watchers_blessing_absorb", e.driver(), e.item )
      {
        set_name_reporting( "Absorb" );

        stat = create_buff<stat_buff_t>( player, "watchers_blessing_stat", player->find_spell( 384560 ), item );
        stat->set_name_reporting( "Stat" );
      }

      void expire_override( int s, timespan_t d ) override
      {
        absorb_buff_t::expire_override( s, d );

        stat->trigger( d );
      }
    };

    buff_t* shield;

    // TODO: confirm mixed order remains true in-game
    // card order is [ 2 3 6 7 4 5 8 A ]
    darkmoon_deck_watcher_t( const special_effect_t& e )
      : DF_darkmoon_proc_t( e, "watchers_blessing", 382958,
                            { 382853, 382854, 382857, 382858, 382855, 382856, 382859, 382852 } )
    {
      shield = buff_t::find( player, "watchers_blessing_absorb" );
      if ( !shield )
        shield = make_buff<watchers_blessing_absorb_buff_t>( e );
    }

    timespan_t get_duration( size_t index ) const
    {
      return 5_s + timespan_t::from_seconds( index );
    }

    void execute() override
    {
      DF_darkmoon_proc_t::execute();

      auto dur = get_duration( deck->top_index );
      shield->trigger( dur );

      // TODO: placeholder value put at 2s before depletion. change to reasonable value.
      auto deplete = rng().gauss( sim->dragonflight_opts.darkmoon_deck_watcher_deplete, 1_s );
      clamp( deplete, 0_ms, dur );

      make_event( *sim, deplete, [ this ]() { shield->expire(); } );
    }
  };

  effect.execute_action = create_proc_action<darkmoon_deck_watcher_t>( "watchers_blessing", effect );
  effect.buff_disabled = true;
}

void alacritous_alchemist_stone( special_effect_t& effect )
{
  new dbc_proc_callback_t( effect.player, effect );

  auto cd = effect.player->get_cooldown( "potion" );
  auto cd_adj = -timespan_t::from_seconds( effect.driver()->effectN( 1 ).base_value() );

  effect.player->callbacks.register_callback_execute_function(
      effect.spell_id, [ cd, cd_adj ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
        cb->proc_buff->trigger();
        cd->adjust( cd_adj );
      } );
}

void bottle_of_spiraling_winds( special_effect_t& effect )
{
  // TODO: determine what happens on buff refresh
  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger() );
  // TODO: confirm it continues to use the driver's coeff and not the actual buff's coeff
  buff->set_stat( STAT_AGILITY, effect.driver()->effectN( 1 ).average( effect.item ) );

  // TODO: this doesn't function in-game atm.
  /*
  auto decrement = create_buff<buff_t>( effect.player, effect.player->find_spell( 383758 ) )
    ->set_quiet( true )
    ->set_tick_callback( [ buff ]( buff_t*, int, timespan_t ) { buff->decrement(); } );

  buff->set_stack_change_callback( [ decrement ]( buff_t* b, int, int new_ ) {
    if ( b->at_max_stacks() )
    {
      b->freeze_stacks = true;
      decrement->trigger();
    }
    else if ( !new_ )
    {
      b->freeze_stacks = false;
    }
  } );
  */
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void conjured_chillglobe( special_effect_t& effect )
{
  struct conjured_chillglobe_proxy_t : public action_t
  {
    action_t* damage;
    action_t* mana;
    double mana_level;

    conjured_chillglobe_proxy_t( const special_effect_t& e )
      : action_t( action_e::ACTION_USE, "conjured_chillglobe", e.player, e.driver() )
    {
      dual = true;

      auto value_data = e.player->find_spell( 377450 );
      mana_level = value_data->effectN( 1 ).percent();

      damage = new generic_proc_t( e, "conjured_chillglobe_damage", 377451 );
      damage->base_dd_min = damage->base_dd_max = value_data->effectN( 2 ).average( e.item );
      damage->stats = stats;

      mana = new generic_proc_t( e, "conjured_chillglobe_mana", 381824 );
      mana->energize_amount = value_data->effectN( 3 ).average( e.item );
      mana->harmful = false;
      mana->name_str_reporting = "conjured_chillglobe";
    }

    result_e calculate_result( action_state_t* /* state */ ) const override
    { return RESULT_HIT; }

    void execute() override
    {
      action_t::execute();

      if ( player->resources.active_resource[ RESOURCE_MANA ] && player->resources.pct( RESOURCE_MANA ) < mana_level )
        mana->execute();
      else
        damage->execute_on_target( target );
    }
  };

  effect.execute_action = create_proc_action<conjured_chillglobe_proxy_t>( "conjured_chillglobe", effect );
}

struct skewering_cold_initializer_t : public item_targetdata_initializer_t
{
  skewering_cold_initializer_t() : item_targetdata_initializer_t( 383931 ) {}

  void operator()( actor_target_data_t* td ) const override
  {
    if ( !find_effect( td->source ) )
    {
      td->debuff.skewering_cold = make_buff( *td, "skewering_cold" )->set_quiet( true );
      return;
    }

    assert( !td->debuff.skewering_cold );
    td->debuff.skewering_cold =
        make_buff( *td, "skewering_cold", td->source->find_spell( spell_id )->effectN( 1 ).trigger() );
    td->debuff.skewering_cold->reset();
  }
};

void globe_of_jagged_ice( special_effect_t& effect )
{
  auto impale = find_special_effect( effect.player, 383931 );
  new dbc_proc_callback_t( effect.player, *impale );

  effect.player->callbacks.register_callback_execute_function(
      impale->spell_id, []( const dbc_proc_callback_t* cb, action_t*, action_state_t* s ) {
        cb->listener->get_target_data( s->target )->debuff.skewering_cold->trigger();
      } );

  struct breaking_the_ice_t : public generic_aoe_proc_t
  {
    breaking_the_ice_t( const special_effect_t& e ) : generic_aoe_proc_t( e, "breaking_the_ice", 388948, true )
    {
      // Tooltip and damage value is based on max stacks
      base_dd_min = base_dd_max = data().effectN( 1 ).average( e.item ) / data().max_stacks();
    }

    bool target_ready( player_t* t ) override
    {
      return generic_aoe_proc_t::target_ready( t ) && player->get_target_data( t )->debuff.skewering_cold->check();
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      return generic_aoe_proc_t::composite_target_multiplier( t ) *
             player->get_target_data( t )->debuff.skewering_cold->stack();
    }

    void impact( action_state_t* s ) override
    {
      generic_aoe_proc_t::impact( s );
      player->get_target_data( s->target )->debuff.skewering_cold->expire();
    }
  };

  effect.execute_action = create_proc_action<breaking_the_ice_t>( "breaking_the_ice", *impale );
}

void irideus_fragment( special_effect_t& effect )
{
  auto reduce = new special_effect_t( effect.player );
  reduce->name_str = "crumbling_power_reduce";
  reduce->type = SPECIAL_EFFECT_EQUIP;
  reduce->source = SPECIAL_EFFECT_SOURCE_ITEM;
  reduce->spell_id = effect.spell_id;
  reduce->cooldown_ = effect.driver()->internal_cooldown();
  // TODO: determine what exactly the flags are. At bare minimum reduced per cast. Certain other non-cast procs will
  // also reduce. Maybe require individually tailoring per module a la Bron
  reduce->proc_flags_ = PF_ALL_DAMAGE | PF_ALL_HEAL;
  reduce->proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.player->special_effects.push_back( reduce );

  auto cb = new dbc_proc_callback_t( effect.player, *reduce );
  cb->initialize();
  cb->deactivate();


  auto buff = create_buff<stat_buff_t>( effect.player, effect.driver() );
  buff->set_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect.item ) )
      ->set_reverse( true )
      ->set_cooldown( 0_ms )
      ->set_stack_change_callback( [ cb ]( buff_t*, int old_, int new_ ) {
    if ( !old_ )
      cb->activate();
    else if ( !new_ )
      cb->deactivate();
  } );

  effect.custom_buff = buff;

  effect.player->callbacks.register_callback_execute_function(
      effect.spell_id, [ buff ]( const dbc_proc_callback_t*, action_t*, action_state_t* ) { buff->trigger(); } );
}

// TODO: Do properly and add both drivers
// 383798 = Driver for you + value
// 383799 = Stat buff for both
// 383803 = Unused buff
// 386578 = Driver + on-use buff on target
// 389581 = On-use buff on you
// 398396 = On-use
void emerald_coachs_whistle( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 383799 ) )
    ->set_stat_from_effect( 1, effect.driver()->effectN( 2 ).average( effect.item ) );

  effect.custom_buff = buff;

  // self driver procs off druid hostile abilities as well as shadow hostile abilities
  if ( effect.player->type == player_e::DRUID || effect.player->specialization() == PRIEST_SHADOW )
    effect.proc_flags_ |= PF_MAGIC_SPELL | PF_MELEE_ABILITY;

  new dbc_proc_callback_t( effect.player, effect );

  // Pretend we are our bonded partner for the sake of procs from partner
  auto coached = new special_effect_t( effect.player );
  coached->type = SPECIAL_EFFECT_EQUIP;
  coached->source = SPECIAL_EFFECT_SOURCE_ITEM;
  coached->spell_id = 386578;
  coached->custom_buff = buff;
  effect.player->special_effects.push_back( coached );

  new dbc_proc_callback_t( effect.player, *coached );
}

void erupting_spear_fragment( special_effect_t& effect )
{
  struct erupting_spear_fragment_t : public generic_aoe_proc_t
  {
    stat_buff_t* buff;
    double return_speed;

    erupting_spear_fragment_t( const special_effect_t& e )
      : generic_aoe_proc_t( e, "erupting_spear_fragment", e.trigger() ),
        return_speed( e.player->find_spell( 381586 )->missile_speed() )
    {
      auto values = player->find_spell( 381484 );

      travel_speed = e.driver()->missile_speed();
      aoe = -1;
      base_dd_min = base_dd_max = values->effectN( 1 ).average( item );

      buff = create_buff<stat_buff_t>( player, "erupting_flames", player->find_spell( 381476 ) );
      buff->set_stat( STAT_CRIT_RATING, values->effectN( 2 ).average( item ) );
    }

    void impact( action_state_t* s ) override
    {
      generic_aoe_proc_t::impact( s );

      make_event( *sim, timespan_t::from_seconds( player->get_player_distance( *s->target ) / return_speed ),
                  [ this ]() { buff->trigger(); } );
    }
  };

  effect.execute_action = create_proc_action<erupting_spear_fragment_t>( "erupting_spear_fragment", effect );
}

void furious_ragefeather( special_effect_t& effect )
{
  struct soulseeker_arrow_repeat_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;

    soulseeker_arrow_repeat_cb_t( const special_effect_t& e, buff_t* b ) : dbc_proc_callback_t( e.player, e ), buff( b )
    {}

    void execute( action_t* a, action_state_t* s ) override
    {
      buff->expire();

      dbc_proc_callback_t::execute( a, s );
    }
  };

  auto arrow = create_proc_action<generic_proc_t>( "soulseeker_arrow", effect, "soulseeker_arrow", 388755 );
  arrow->base_td = effect.driver()->effectN( 2 ).average( effect.item );

  effect.execute_action = arrow;

  new dbc_proc_callback_t( effect.player, effect );

  unsigned repeat_id = 389407;

  auto buff = create_buff<buff_t>( effect.player, effect.player->find_spell( repeat_id ) );

  auto repeat = new special_effect_t( effect.player );
  repeat->type = SPECIAL_EFFECT_EQUIP;
  repeat->source = SPECIAL_EFFECT_SOURCE_ITEM;
  repeat->spell_id = repeat_id;
  repeat->execute_action = arrow;
  effect.player->special_effects.push_back( repeat );

  auto cb = new soulseeker_arrow_repeat_cb_t( *repeat, buff );
  cb->initialize();
  cb->deactivate();

  buff->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
    if ( new_ )
      cb->activate();
    else
      cb->deactivate();
  } );

  auto p = effect.player;
  p->register_on_kill_callback( [ p, buff ]( player_t* t ) {
    if ( p->sim->event_mgr.canceled )
      return;

    auto d = t->find_dot( "soulseeker_arrow", p );
    if ( d && d->remains() > 0_ms )
      buff->trigger();
  } );
}


// Igneous Flowstone
// 402813 = Primary Driver - Effect1 = Damage, Effect2 = Crit
// 402897 = Stats
// 402903 = High Tide Trigger
// 402896 = Low Tide Trigger
// 402898 = Low Tide Count Down
// 402894 = High Tide Count Up
// 407961 = Lava Wave, damage spell
// 402822 = Weird Area Trigger
// 402813 = Damage Spell in logs?
void igneous_flowstone( special_effect_t& effect )
{
  struct low_tide_cb_t : public dbc_proc_callback_t
  {
    buff_t* trigger_buff;
    buff_t* stat_buff;

    low_tide_cb_t( const special_effect_t& e, buff_t* t, buff_t* s )
      : dbc_proc_callback_t( e.player, e ), trigger_buff( t ), stat_buff( s )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      trigger_buff->expire();
      stat_buff->trigger();
    }
  };

  struct high_tide_cb_t : public dbc_proc_callback_t
  {
    buff_t* trigger_buff;
    action_t* damage;

    high_tide_cb_t( const special_effect_t& e, buff_t* t, action_t* d )
      : dbc_proc_callback_t( e.player, e ), trigger_buff( t ), damage( d )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      trigger_buff->expire();

      size_t n = 1;
      if ( rng().roll( listener->dragonflight_opts.igneous_flowstone_double_lava_wave_chance ) )
        n++;

      while ( n-- )
      {
        damage->set_target( target( s ) );
        auto damage_state    = damage->get_state();
        damage_state->target = damage->target;
        damage->snapshot_state( damage_state, damage->amount_type( damage_state ) );
        damage->schedule_execute( damage_state );
      }
    }
  };

  struct lava_wave_proc_t : public generic_aoe_proc_t
  {
    double min_range;

    lava_wave_proc_t( const special_effect_t& e )
      : generic_aoe_proc_t( e, "lava_wave", 407961, true ), min_range( e.driver()->effectN( 5 ).base_value() )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      auto tm = generic_aoe_proc_t::composite_target_multiplier( t );

      if ( player->get_player_distance( *t ) < min_range )
        tm *= 2.0 / 3.0;  // damage reduced by 1/3 if within minimum range

      return tm;
    }
  };

  auto damage = create_proc_action<lava_wave_proc_t>( "lava_wave", effect );

  auto crit_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 402897 ) )
                       ->set_stat_from_effect( 1, effect.driver()->effectN( 2 ).average( effect.item ) );

  auto low_tide_trigger = create_buff<buff_t>( effect.player, effect.player->find_spell( 402896 ) );

  auto low_tide_driver       = new special_effect_t( effect.player );
  low_tide_driver->type      = SPECIAL_EFFECT_EQUIP;
  low_tide_driver->source    = SPECIAL_EFFECT_SOURCE_ITEM;
  low_tide_driver->spell_id  = low_tide_trigger->data().id();
  // Add a cooldown to prevent a double trigger for classes like windwalker
  low_tide_driver->cooldown_ = 1.5_s;
  effect.player->special_effects.push_back( low_tide_driver );

  auto low_tide_cb = new low_tide_cb_t( *low_tide_driver, low_tide_trigger, crit_buff );
  low_tide_cb->initialize();
  low_tide_cb->deactivate();

  auto high_tide_trigger = create_buff<buff_t>( effect.player, effect.player->find_spell( 402903 ) );

  auto high_tide_driver       = new special_effect_t( effect.player );
  high_tide_driver->type      = SPECIAL_EFFECT_EQUIP;
  high_tide_driver->source    = SPECIAL_EFFECT_SOURCE_ITEM;
  high_tide_driver->spell_id  = high_tide_trigger->data().id();
  // Add a cooldown to prevent a double trigger for classes like windwalker
  high_tide_driver->cooldown_ = 1.5_s;
  effect.player->special_effects.push_back( high_tide_driver );

  auto high_tide_cb = new high_tide_cb_t( *high_tide_driver, high_tide_trigger, damage );
  high_tide_cb->initialize();
  high_tide_cb->deactivate();

  auto low_tide_counter = create_buff<buff_t>( effect.player, effect.player->find_spell( 402898 ) )
                              ->set_reverse( true )
                              ->set_period( 1_s )
                              ->set_stack_change_callback( [ low_tide_trigger ]( buff_t*, int, int n ) {
                                if ( !n )
                                {
                                  low_tide_trigger->trigger();
                                }
                              } );

  auto high_tide_counter = create_buff<buff_t>( effect.player, effect.player->find_spell( 402894 ) )
                               ->set_period( 1_s )
                               ->set_expire_at_max_stack( true )
                               ->set_stack_change_callback( [ high_tide_trigger ]( buff_t*, int, int n ) {
                                 if ( !n )
                                 {
                                   high_tide_trigger->trigger();
                                 }
                               } );

  high_tide_trigger->set_stack_change_callback( [ high_tide_cb, low_tide_counter ]( buff_t*, int, int n ) {
    if ( !n )
    {
      high_tide_cb->deactivate();
      low_tide_counter->trigger();
    }
    else
    {
      high_tide_cb->activate();
    }
  } );

  low_tide_trigger->set_stack_change_callback( [ low_tide_cb, high_tide_counter ]( buff_t*, int, int n ) {
    if ( !n )
    {
      low_tide_cb->deactivate();
      high_tide_counter->trigger();
    }
    else
    {
      low_tide_cb->activate();
    }
  } );

  effect.player->register_combat_begin(
      [ low_tide_trigger, high_tide_trigger, low_tide_counter, high_tide_counter ]( player_t* p ) {
        auto starting_state = p->dragonflight_opts.flowstone_starting_state;
        if ( util::str_compare_ci( starting_state, "low" ) )
          low_tide_trigger->trigger();
        else if ( util::str_compare_ci( starting_state, "ebb" ) )
          low_tide_counter->trigger();
        else if ( util::str_compare_ci( starting_state, "flood" ) )
          high_tide_counter->trigger();
        else if ( util::str_compare_ci( starting_state, "high" ) )
          high_tide_trigger->trigger();
        // As of 04/05/2023 the Active Triggers have no maximum duration and persist through death, so the correct
        // behaviour is randomly picking either active to start with. Other options are still being offered for the curious.
        // TODO: Confirm is raid combat start resets the state of this.
        else if ( p->rng().roll( 0.5 ) )
          low_tide_trigger->trigger();
        else
          high_tide_trigger->trigger();
      } );
}

void idol_of_pure_decay( special_effect_t& effect )
{
  struct idol_of_pure_decay_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    timespan_t dur;
    int count;

    idol_of_pure_decay_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        dur( e.trigger()->duration() ),
        count( as<int>( e.driver()->effectN( 1 ).base_value() ) )
    {
      damage = create_proc_action<generic_aoe_proc_t>( "pure_decay", effect, "pure_decay", 388739, true );
      damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 2 ).average( effect.item );
    }

    void execute( action_t*, action_state_t* ) override
    {
      // damage pulses 3 times over 3s.
      // TODO: confirm if trinket can proc again during the 3s.
      make_repeating_event( listener->sim, dur / count, [ this ]() { damage->execute(); }, count );
    }
  };

  new idol_of_pure_decay_cb_t( effect );
}

void shikaari_huntress_arrowhead( special_effect_t& effect )
{
  // against elite enemies, gives 10% more stats (default)
  // against normal enemies, gives 10% less stats (NYI)
  effect.stat = util::translate_rating_mod( effect.trigger()->effectN( 1 ).misc_value1() );
  effect.stat_amount = effect.trigger()->effectN( 1 ).average( effect.item ) * 1.1;

  new dbc_proc_callback_t( effect.player, effect );
}

struct spiteful_storm_initializer_t : public item_targetdata_initializer_t
{
  spiteful_storm_initializer_t() : item_targetdata_initializer_t( 377466 ) {}

  void operator()( actor_target_data_t* td ) const override
  {
    if ( !find_effect( td->source ) )
    {
      td->debuff.grudge = make_buff( *td, "grudge" )->set_quiet( true );
      return;
    }

    assert( !td->debuff.grudge );
    td->debuff.grudge = make_buff( *td, "grudge", td->source->find_spell( 382428 ) );
    td->debuff.grudge->reset();
  }
};

void spiteful_storm( special_effect_t& effect )
{
  struct spiteful_stormbolt_t : public generic_proc_t
  {
    buff_t* gathering;
    dbc_proc_callback_t* callback;
    player_t* grudge;

    spiteful_stormbolt_t( const special_effect_t& e, buff_t* b, dbc_proc_callback_t* cb )
      : generic_proc_t( e, "spiteful_stormbolt", e.player->find_spell( 382426 ) ),
        gathering( b ),
        callback( cb ),
        grudge( nullptr )
    {
      base_td = e.driver()->effectN( 1 ).average( e.item );
    }

    void reset() override
    {
      generic_proc_t::reset();
      grudge = nullptr;
    }

    void set_target( player_t* t ) override
    {
      if ( grudge )
        target = grudge;
      else
        generic_proc_t::set_target( t );
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      return generic_proc_t::composite_ta_multiplier( s ) * ( 1.0 + gathering->check_stack_value() );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      if ( !grudge )
      {
        grudge = s->target;
        assert( grudge );
        player->get_target_data( grudge )->debuff.grudge->trigger();
      }
    }

    void last_tick( dot_t* d ) override
    {
      generic_proc_t::last_tick( d );

      callback->activate();

      make_event( *sim, travel_time(), [ this ]() { gathering->trigger(); } );
    }
  };

  auto spite = create_buff<buff_t>( effect.player, effect.player->find_spell( 382425 ) );
  spite->set_default_value( effect.driver()->effectN( 2 ).base_value() )
       ->set_max_stack( as<int>( spite->default_value ) )
       ->set_expire_at_max_stack( true );

  effect.custom_buff = spite;
  effect.proc_flags2_ = PF2_ALL_CAST;
  auto cb = new dbc_proc_callback_t( effect.player, effect );

  auto gathering = create_buff<buff_t>( effect.player, "gathering_storm_trinket", effect.player->find_spell( 394864 ) );
  // build 48317: tested to increase damage by 25% per stack, not found in spell data
  gathering->set_default_value( 0.25 );
  gathering->set_name_reporting( "gathering_storm" );

  auto stormbolt = debug_cast<spiteful_stormbolt_t*>(
      create_proc_action<spiteful_stormbolt_t>( "spiteful_stormbolt", effect, gathering, cb ) );

  spite->set_stack_change_callback( [ stormbolt, cb ]( buff_t* b, int, int new_ ) {
    if ( !new_ )
    {
      cb->deactivate();
      stormbolt->execute_on_target( b->player->target );  // always execute_on_target so set_target is properly called
    }
  } );

  int stack_increase = as<int>( effect.driver()->effectN( 3 ).base_value() );
  gathering->set_stack_change_callback( [ spite, stack_increase ]( buff_t*, int, int new_ ) {
    if ( new_ )
      spite->set_max_stack( as<int>( spite->default_value ) + new_ * stack_increase );
    else
      spite->set_max_stack( as<int>( spite->default_value ) );
  } );

  auto p = effect.player;
  p->register_on_kill_callback( [ p, stormbolt, gathering ]( player_t* t ) {
    if ( p->sim->event_mgr.canceled )
      return;

    if ( stormbolt->grudge && t->actor_spawn_index == stormbolt->grudge->actor_spawn_index )
    {
      stormbolt->grudge = nullptr;
      gathering->expire();
    }
  } );
  effect.player->register_combat_begin( [ cb ]( player_t* ) { cb->activate(); } );
}

void spoils_of_neltharus( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "spoils_of_neltharus_crit", "spoils_of_neltharus_haste",
                                        "spoils_of_neltharus_mastery", "spoils_of_neltharus_vers" } ) )
  {
    return;
  }

  struct spoils_of_neltharus_t : public proc_spell_t
  {
    using buffs = std::vector<std::pair<buff_t*, buff_t*>>;

    struct spoils_of_neltharus_cb_t : public dbc_proc_callback_t
    {
      buffs& buff_list;

      spoils_of_neltharus_cb_t( const special_effect_t& e, buffs* list )
        : dbc_proc_callback_t( e.player, e ), buff_list( *list )
      {}

      void execute( action_t*, action_state_t* ) override
      {
        buff_list.front().first->expire();
        std::rotate( buff_list.begin(), buff_list.begin() + rng().range( 1, 4 ), buff_list.end() );
        buff_list.front().first->trigger();
      }
    };

    buffs buff_list;
    dbc_proc_callback_t* cb;
    buff_t* initial = nullptr;

    spoils_of_neltharus_t( const special_effect_t& e ) : proc_spell_t( e )
    {
      auto shuffle = find_special_effect( e.player, 381766 );
      cb = new spoils_of_neltharus_cb_t( *shuffle, &buff_list );
      cb->initialize();
      cb->activate();

      auto init_buff = [ this, shuffle ]( std::string n, unsigned id ) {
        auto spell = player->find_spell( id );

        auto counter = make_buff<buff_t>( player, "spoils_of_neltharus_" + n, spell )
          ->set_quiet( true );

        auto stat = make_buff<stat_buff_t>( player, "spoils_of_neltharus_stat_" + n, spell )
          ->add_stat_from_effect( 1, shuffle->driver()->effectN( 1 ).average( item ) )
          ->set_name_reporting( util::inverse_tokenize( n ) )
          ->set_duration( timespan_t::from_seconds( data().effectN( 2 ).base_value() ) );

        buff_list.emplace_back( counter, stat );
        if ( util::str_compare_ci( player->dragonflight_opts.spoils_of_neltharus_initial_type, n ) )
        {
          initial = buff_list.back().first;
        }
      };

      init_buff( "crit", 381954 );
      init_buff( "haste", 381955 );
      init_buff( "mastery", 381956 );
      init_buff( "vers", 381957 );
    }

    void init_finished() override
    {
      proc_spell_t::init_finished();

      // Make the first counter buff trigger at the start of combat
      player->register_combat_begin( [ this ]( player_t* ) { buff_list.front().first->trigger(); } );
    }

    void reset() override
    {
      proc_spell_t::reset();

      cb->activate();

      if ( initial != nullptr )
      {
        while ( initial != buff_list.front().first )
        {
          std::rotate( buff_list.begin(), buff_list.begin() + 1, buff_list.end() );
        }
      }
      else
      {
        rng().shuffle( buff_list.begin(), buff_list.end() );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      buff_list.front().first->expire();
      buff_list.front().second->trigger();

      cb->deactivate();
      // TODO: ~90+s before the counter buff can be proc'd again. Find spell data source for this if possible.
      make_event( *sim, 90_s, [ this ]() { cb->activate(); } );
    }
  };

  effect.execute_action = create_proc_action<spoils_of_neltharus_t>( "spoils_of_neltharus", effect );
}

void sustaining_alchemist_stone( special_effect_t& effect )
{
  effect.stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );
  effect.stat_amount = effect.driver()->effectN( 2 ).average( effect.item );

  new dbc_proc_callback_t( effect.player, effect );
}

void timebreaching_talon( special_effect_t& effect )
{
  auto debuff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 384050 ) );
  debuff->set_stat( STAT_INTELLECT, effect.driver()->effectN( 1 ).average( effect.item ) );

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 382126 ) );
  buff->set_stat( STAT_INTELLECT, effect.driver()->effectN( 2 ).average( effect.item ) )
      ->set_stack_change_callback( [ debuff ]( buff_t*, int, int new_ ) {
        if ( !new_ )
          debuff->trigger();
      } );

  effect.custom_buff = buff;
}

void voidmenders_shadowgem( special_effect_t& effect )
{
  auto stacking_buff =
      create_buff<stat_buff_t>( effect.player, "voidmenders_shadowgem_stacks", effect.player->find_spell( 397399 ) );
  stacking_buff->set_cooldown( 0_ms );
  stacking_buff->set_chance( 1.0 );
  stacking_buff->set_max_stack( 20 );
  stacking_buff->set_stat( STAT_CRIT_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );

  auto stacking_driver                = new special_effect_t( effect.player );
  stacking_driver->name_str           = "voidmenders_shadowgem_stacks";
  stacking_driver->type               = SPECIAL_EFFECT_EQUIP;
  stacking_driver->source             = SPECIAL_EFFECT_SOURCE_ITEM;
  stacking_driver->spell_id           = effect.driver()->id();
  stacking_driver->cooldown_          = 0_ms;
  stacking_driver->cooldown_category_ = 0;
  stacking_driver->custom_buff        = stacking_buff;

  effect.player->special_effects.push_back( stacking_driver );

  auto stacking_cb = new dbc_proc_callback_t( effect.player, *stacking_driver );
  stacking_cb->initialize();
  stacking_cb->deactivate();

  auto buff = create_buff<stat_buff_t>( effect.player, "voidmenders_shadowgem", effect.player->find_spell( 397399 ) );
  buff->set_chance( 1.0 );
  buff->set_stat( STAT_CRIT_RATING, effect.driver()->effectN( 1 ).average( effect.item ) )
      ->set_stack_change_callback( [ stacking_cb, stacking_buff ]( buff_t*, int, int new_ ) {
        if ( new_ )
          stacking_cb->activate();
        else
        {
          stacking_cb->deactivate();
          stacking_buff->expire();
        }
      } );

  effect.ppm_ = 0;
  effect.proc_chance_ = 1.0;

  effect.custom_buff = buff;
}

void umbrelskuls_fractured_heart( special_effect_t& effect )
{
  auto dot = create_proc_action<generic_proc_t>( "crystal_sickness", effect, "crystal_sickness", effect.trigger() );
  dot->base_td = effect.driver()->effectN( 2 ).average( effect.item );

  effect.execute_action = dot;

  new dbc_proc_callback_t( effect.player, effect );

  // TODO: explosion doesn't work. placeholder using existing data (which may be wrong)
  // TODO: determine if damage increases per target hit
  auto explode = create_proc_action<generic_aoe_proc_t>( "breaking_the_ice", effect, "breaking_the_ice", 388948 );
  explode->base_dd_min = explode->base_dd_max = effect.driver()->effectN( 3 ).average( effect.item );

  auto p = effect.player;
  p->register_on_kill_callback( [ p, explode ]( player_t* t ) {
    if ( p->sim->event_mgr.canceled )
      return;

    auto d = t->find_dot( "crystal_sickness", p );
    if ( d && d->remains() > 0_ms )
      explode->execute();
  } );
}

void way_of_controlled_currents( special_effect_t& effect )
{
  auto stack =
      create_buff<buff_t>( effect.player, "way_of_controlled_currents_stack", effect.player->find_spell( 381965 ) )
          ->set_name_reporting( "Stack" )
          ->add_invalidate( CACHE_ATTACK_SPEED );

  stack->set_default_value( stack->data().effectN( 1 ).base_value() * 0.01 );

  effect.player->buffs.way_of_controlled_currents = stack;

  auto surge =
      create_buff<buff_t>( effect.player, "way_of_controlled_currents_surge", effect.player->find_spell( 381966 ) )
          ->set_name_reporting( "Surge" );

  auto lockout =
      create_buff<buff_t>( effect.player, "way_of_controlled_currents_lockout", effect.player->find_spell( 397621 ) )
          ->set_quiet( true );

  auto surge_driver = new special_effect_t( effect.player );
  surge_driver->type = SPECIAL_EFFECT_EQUIP;
  surge_driver->source = SPECIAL_EFFECT_SOURCE_ITEM;
  surge_driver->spell_id = surge->data().id();
  surge_driver->cooldown_ = surge->data().internal_cooldown();
  effect.player->special_effects.push_back( surge_driver );

  auto surge_damage = create_proc_action<generic_proc_t>( "way_of_controlled_currents", *surge_driver,
                                                          "way_of_controlled_currents", 381967 );

  surge_damage->base_dd_min = surge_damage->base_dd_max = effect.driver()->effectN( 5 ).average( effect.item );

  surge_driver->execute_action = surge_damage;

  auto surge_cb = new dbc_proc_callback_t( effect.player, *surge_driver );
  surge_cb->initialize();
  surge_cb->deactivate();

  stack->set_stack_change_callback( [ surge ]( buff_t* b, int, int ) {
    if ( b->at_max_stacks() )
      surge->trigger();
  } );

  surge->set_stack_change_callback( [ stack, lockout, surge_cb ]( buff_t*, int, int new_ ) {
    if ( new_ )
    {
      surge_cb->activate();
    }
    else
    {
      surge_cb->deactivate();
      stack->expire();
      lockout->trigger();
    }
  } );

  effect.custom_buff = stack;
  effect.proc_flags2_ = PF2_CRIT;
  auto stack_cb = new dbc_proc_callback_t( effect.player, effect );

  lockout->set_stack_change_callback( [ stack_cb ]( buff_t*, int, int new_ ) {
    if ( new_ )
      stack_cb->deactivate();
    else
      stack_cb->activate();
  } );
}

void whispering_incarnate_icon( special_effect_t& effect )
{
  bool has_heal = false, has_tank = false, has_dps = false;

  auto splits = util::string_split<std::string_view>(
      effect.player->sim->dragonflight_opts.whispering_incarnate_icon_roles, "/" );
  for ( auto s : splits )
  {
    if ( util::str_compare_ci( s, "heal" ) )
      has_heal = true;
    else if ( util::str_compare_ci( s, "tank" ) )
      has_tank = true;
    else if ( util::str_compare_ci( s, "dps" ) )
      has_dps = true;
    else
      throw std::invalid_argument( "Invalid string for dragonflight.whispering_incarnate_icon_roles." );
  }

  unsigned buff_id = 0, proc_buff_id = 0;

  // single inspiration
  constexpr unsigned dps_buff_id = 382082;   // inspired by frost
  constexpr unsigned tank_buff_id = 382081;  // inspired by earth
  constexpr unsigned heal_buff_id = 382083;  // inspired by flame

  switch ( effect.player->specialization() )
  {
    case WARRIOR_PROTECTION:
    case PALADIN_PROTECTION:
    case DEATH_KNIGHT_BLOOD:
    case MONK_BREWMASTER:
    case DRUID_GUARDIAN:
    case DEMON_HUNTER_VENGEANCE:
      buff_id = 382078;  // mark of earth

      if ( has_dps && has_heal )
        proc_buff_id = 394460;  // inspired by frost and fire
      else if ( has_dps )
        proc_buff_id = dps_buff_id;
      else if ( has_heal )
        proc_buff_id = heal_buff_id;
      else
        proc_buff_id = 0;

      break;
    case PALADIN_HOLY:
    case PRIEST_DISCIPLINE:
    case PRIEST_HOLY:
    case SHAMAN_RESTORATION:
    case MONK_MISTWEAVER:
    case DRUID_RESTORATION:
    case EVOKER_PRESERVATION:
      buff_id = 382080;  // mark of fire

      if ( has_dps && has_tank )
        proc_buff_id = 394462;  // inspired by frost and earth
      else if ( has_dps )
        proc_buff_id = dps_buff_id;
      else if ( has_tank )
        proc_buff_id = tank_buff_id;
      else
        proc_buff_id = 0;

      break;
    default:
      buff_id = 382079;  // mark of frost

      if ( has_heal && has_tank )
        proc_buff_id = 394461;  // inspired by fire and earth
      else if ( has_heal )
        proc_buff_id = heal_buff_id;
      else if ( has_tank )
        proc_buff_id = tank_buff_id;

      break;
  }

  auto buff_data = effect.player->find_spell( buff_id );
  auto buff = create_buff<stat_buff_t>( effect.player, buff_data );
  buff->set_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect.item ) )
      ->set_constant_behavior( buff_constant_behavior::ALWAYS_CONSTANT )
      ->set_rppm( rppm_scale_e::RPPM_DISABLE );

  effect.player->register_combat_begin( [ buff ]( player_t* ) {
    buff->trigger();
  } );

  if ( proc_buff_id )
  {
    auto proc_buff_data = effect.player->find_spell( proc_buff_id );
    auto proc_buff      = create_buff<stat_buff_t>( effect.player, proc_buff_data );
    double amount       = effect.driver()->effectN( 2 ).average( effect.item );

    for ( auto& s : proc_buff->stats )
      s.amount = amount;

    effect.spell_id = buff_id;
    effect.custom_buff = proc_buff;

    new dbc_proc_callback_t( effect.player, effect );
  }
}

void the_cartographers_calipers( special_effect_t& effect )
{
  effect.trigger_spell_id = 384114;
  effect.discharge_amount = effect.driver()->effectN( 1 ).average( effect.item );

  new dbc_proc_callback_t( effect.player, effect );
}

// Rumbling Ruby
// 377454 = Driver
// 382094 = Stacking Buff, triggers 382095 at max stacks
// 382095 = Player Buff triggerd from 382094, triggers 382096
// 382096 = Damage Area trigger
// 382097 = Damage spell
void rumbling_ruby( special_effect_t& effect )
{
  special_effect_t* rumbling_ruby_damage_proc = new special_effect_t( effect.player );
  rumbling_ruby_damage_proc->item = effect.item;

  auto ruby_buff = buff_t::find( effect.player, "rumbling_ruby");
  if ( !ruby_buff )
  {
    auto ruby_proc_spell = effect.player -> find_spell( 382095 );
    ruby_buff = make_buff( effect.player, "rumbling_ruby", ruby_proc_spell );
    rumbling_ruby_damage_proc->spell_id = 382095;

    effect.player -> special_effects.push_back( rumbling_ruby_damage_proc );
  }

  auto power_buff = buff_t::find( effect.player, "rumbling_power" );
  if ( !power_buff )
  {
    auto power_proc_spell = effect.player->find_spell( 382094 );
    power_buff = make_buff<stat_buff_t>(effect.player, "concussive_force", power_proc_spell)
                   ->add_stat( STAT_ATTACK_POWER, effect.driver() -> effectN( 1 ).average( effect.item ));
  }

  struct rumbling_ruby_damage_t : public proc_spell_t
  {
    buff_t* ruby_buff;

    rumbling_ruby_damage_t( const special_effect_t& e, buff_t* b ) :
      proc_spell_t( "rumbling_ruby_damage", e.player, e.player->find_spell( 382097 ), e.item ), ruby_buff( b )
    {
      base_dd_min = base_dd_max = e.player -> find_spell( 377454 ) -> effectN( 2 ).average( e.item );
      background = true;
      aoe = -1;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
       double d = proc_spell_t::composite_da_multiplier( s );

       // 10-28-22 In game testing suggests the damage scaling is
       // 50% increase above 90% enemy hp
       // 25% above 75% enemy hp
       // 10% above 50% enemy hp

       if (s->target->health_percentage() >= 50 && s->target->health_percentage() <= 75)
       {
         d *= 1.1;
       }
       if (s->target->health_percentage() >= 75 && s->target->health_percentage() <= 90)
       {
         d *= 1.25;
       }
       if (s->target->health_percentage() >= 90)
       {
         d *= 1.5;
       }
      return d;
    }

    void execute() override
    {
      proc_spell_t::execute();
      ruby_buff -> decrement();
    }
  };

  auto proc_object = new dbc_proc_callback_t( effect.player, *rumbling_ruby_damage_proc );
  rumbling_ruby_damage_proc->execute_action = create_proc_action<rumbling_ruby_damage_t>( "rumbling_ruby_damage", *rumbling_ruby_damage_proc, ruby_buff );
  ruby_buff -> set_stack_change_callback( [ proc_object, power_buff ]( buff_t* b, int, int new_ )
  {
    if ( new_ == 0 )
    {
      b->expire();
      power_buff->expire();
      proc_object->deactivate();
    }
    else if ( b->at_max_stacks() )
    {
      proc_object->activate();
    }
  } );

  power_buff -> set_stack_change_callback( [ ruby_buff ]( buff_t* b, int, int )
  {
    if ( b->at_max_stacks() )
    {
      ruby_buff->trigger();
    }
  } );

  effect.custom_buff = power_buff;
  new dbc_proc_callback_t( effect.player, effect );
  proc_object->deactivate();
}

// Storm-Eater's Boon
// 377453 Driver
// 382090 Damage Effect & Stacking Buff
// 382092 Damage Value
void storm_eaters_boon( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "stormeaters_boon" } ) )
    return;

  buff_t* stack_buff;
  buff_t* main_buff;

  main_buff = make_buff( effect.player, "stormeaters_boon", effect.player->find_spell(377453))
      ->set_cooldown( 0_ms );
  stack_buff = make_buff( effect.player, "stormeaters_boon_stacks", effect.player->find_spell( 382090 ))
      ->set_duration( effect.player -> find_spell( 377453 )->duration() )
      ->set_cooldown( 0_ms );

  effect.custom_buff = effect.player->buffs.stormeaters_boon = main_buff;

  struct storm_eaters_boon_damage_t : public proc_spell_t
  {
    buff_t* stack_buff;
    storm_eaters_boon_damage_t( const special_effect_t& e, buff_t* b ) :
      proc_spell_t( "stormeaters_boon_damage", e.player, e.player->find_spell( 382090 ), e.item ), stack_buff( b )
    {
      background = true;
      base_dd_min = base_dd_max = e.player->find_spell( 382092 )->effectN( 1 ).average(e.item);
      name_str_reporting = "stormeaters_boon";
      reduced_aoe_targets = 5;  // from logs, not found in spell data
    }

     double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = proc_spell_t::composite_da_multiplier( s );

      m *= 1.0 + ( stack_buff -> stack() * 0.1 );

      return m;
    }

     void execute() override
     {
       proc_spell_t::execute();
       stack_buff -> trigger();
     }
  };
  action_t* boon_action = create_proc_action<storm_eaters_boon_damage_t>( "stormeaters_boon_damage", effect, stack_buff );
  main_buff->set_refresh_behavior( buff_refresh_behavior::DISABLED );
  main_buff->set_tick_callback(
      [ boon_action, effect ]( buff_t* /* buff */, int /* current_tick */, timespan_t /* tick_time */ ) {
        boon_action->execute();
        effect.player->get_cooldown( effect.cooldown_group_name() )->start( effect.cooldown_group_duration() );
      } );
  main_buff->set_stack_change_callback( [ stack_buff ](buff_t*, int, int new_)
  {
    if( new_ == 0 )
    {
      stack_buff->expire();
    }
  } );
}

// Decoration of Flame
// 377449 Driver
// 382058 Shield Buff
// 394393 Damage and Shield Value
// 397331 Fireball 1
// 397347 - 397353 Fireballs 2-8
void decoration_of_flame( special_effect_t& effect )
{
  buff_t* buff;

  buff = make_buff( effect.player, "decoration_of_flame", effect.player->find_spell( 377449 ) );

  effect.custom_buff = buff;

  struct decoration_of_flame_damage_t : public proc_spell_t
  {
     buff_t* shield;
     double value;
     unsigned cap;

     decoration_of_flame_damage_t( const special_effect_t& e )
       : proc_spell_t( "decoration_of_flame", e.player, e.player->find_spell( 377449 ), e.item ),
         shield( nullptr ),
         value( e.player->find_spell( 394393 )->effectN( 2 ).average( e.item ) ),
         cap( as<int>( e.driver()->effectN( 3 ).base_value() ) )
     {
       background = true;
       split_aoe_damage = true;
       base_dd_min = base_dd_max = e.player->find_spell( 394393 )->effectN( 1 ).average( e.item );
       aoe = -1;
       radius = 10;
       shield = make_buff<absorb_buff_t>( e.player, "decoration_of_flame_shield", e.player->find_spell( 382058 ) );
     }

    std::vector<player_t*>& target_list() const override
    {
      target_cache.is_valid = false;

      auto& tl = proc_spell_t::target_list();

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* ) {
        return rng().roll( sim->dragonflight_opts.decoration_of_flame_miss_chance );
      } ), tl.end() );

      return tl;
    }

     double composite_da_multiplier( const action_state_t* s ) const override
     {
       double m = proc_spell_t::composite_da_multiplier( s );

       // Damage increases by 10% per target based on in game testing
       m *= 1.0 + ( std::min( cap, s->n_targets ) - 1 ) * 0.1;

       return m;
     }

     void execute() override
     {
       proc_spell_t::execute();

       shield->trigger( -1, value * ( 1.0 + ( num_targets_hit - 1 ) * 0.05 ) );
     }
  };

  action_t* action = create_proc_action<decoration_of_flame_damage_t>( "decoration_of_flame", effect );
  buff->set_stack_change_callback( [ action ]( buff_t*, int, int ) {
    action->execute();
  } );
}

// Manic Grieftorch
// 377463 Driver
// 382135 Damage
// 382136 Damage Driver
// 394954 Damage Value
// 382256 AoE Radius
// 382257 ???
// 395703 ???
// 396434 ???
void manic_grieftorch( special_effect_t& effect )
{
    struct manic_grieftorch_damage_t : public proc_spell_t
  {
    manic_grieftorch_damage_t( const special_effect_t& e ) :
      proc_spell_t( "manic_grieftorch", e.player, e.player->find_spell( 382135 ), e.item )
    {
      background = true;
      base_dd_min = base_dd_max = e.player->find_spell( 394954 )->effectN( 1 ).average( e.item );
    }
  };

  struct manic_grieftorch_missile_t : public proc_spell_t
  {
    manic_grieftorch_missile_t(const special_effect_t& e) :
        proc_spell_t("manic_grieftorch_missile", e.player, e.player->find_spell(382136), e.item)
    {
      background = true;
      aoe = -1;
      radius = e.player -> find_spell( 382256 ) -> effectN( 1 ).radius();
      impact_action = create_proc_action<manic_grieftorch_damage_t>( "manic_grieftorch", e );
    }

    size_t available_targets( std::vector< player_t* >& tl ) const override
    {
    proc_spell_t::available_targets( tl );
    tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* t) {
         if( t == target )
        {
          return false;
        }
        else
        {
          // Has very strange scaling behavior, where it scales with targets very slowly. Using this formula to reduce the cleave chance as target count increases
             return !rng().roll(0.2 * (sqrt(num_targets()) / num_targets()) );
        }
      }), tl.end() );

      return tl.size();
    }
  };

  struct manic_grieftorch_channel_t : public proc_spell_t
  {
    manic_grieftorch_channel_t( const special_effect_t& e ) :
      proc_spell_t( "manic_grieftorch_channel", e.player, e.player->find_spell( 377463 ), e.item)
    {
      background = true;
      channeled = tick_zero = true;
      hasted_ticks = false;
      target_cache.is_valid = false;
      base_tick_time = e.player -> find_spell( 377463 ) -> effectN( 1 ).period();
      tick_action = create_proc_action<manic_grieftorch_missile_t>( "manic_grieftorch_missile", e );
    }

    void execute() override
    {
      proc_spell_t::execute();

      event_t::cancel( player->readying );
      player->delay_auto_attacks( composite_dot_duration( execute_state ) );
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;

      proc_spell_t::last_tick( d );

      if ( was_channeling && !player->readying )
        player->schedule_ready( rng().gauss( sim->channel_lag, sim->channel_lag_stddev ) );
    }
  };

  effect.execute_action = create_proc_action<manic_grieftorch_channel_t>( "manic_grieftorch_channel", effect );
}

// All-Totem of the Master
// TODO - Setup to only proc for tank specs
// 377457 Driver and values
// Effect 1 - Fire/Ice direct damage
// Effect 2 - Ice buff value ?
// Effect 3 - Earth/Air direct damage
// Effect 4 - Fire DoT damage
// Effect 5 - Haste value
// 377458 Earth buff
// 377459 Fire buff
// 377461 Air buff
// 382133 Ice buff
void alltotem_of_the_master( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_NONE;

  struct alltotem_earth_damage_t : public proc_spell_t
  {
    alltotem_earth_damage_t( const special_effect_t& e ) :
      proc_spell_t( "elemental_stance_earth", e.player, e.player->find_spell( 377458 ), e.item )
    {
      background = true;
      aoe = -1;
      base_dd_min = base_dd_max = e.driver()->effectN(3).average(e.item);
    }
  };

  struct alltotem_fire_damage_t : public proc_spell_t
  {
    alltotem_fire_damage_t( const special_effect_t& e ) :
      proc_spell_t( "elemental_stance_fire", e.player, e.player->find_spell( 377459 ), e.item )
    {
      background = true;
      aoe = -1;
      base_dd_min = base_dd_max = e.driver()->effectN(1).average(e.item);
    }
  };

  struct alltotem_fire_dot_damage_t : public proc_spell_t
  {
    alltotem_fire_dot_damage_t( const special_effect_t& e ) :
      proc_spell_t( "elemental_stance_fire_dot", e.player, e.player->find_spell( 377459 ), e.item )
    {
      background = true;
      aoe = -1;
      base_dd_min = base_dd_max = e.driver()->effectN(4).average(e.item);
    }
  };

  struct alltotem_air_damage_t : public proc_spell_t
  {
    alltotem_air_damage_t( const special_effect_t& e ) :
      proc_spell_t( "elemental_stance_air", e.player, e.player->find_spell( 377461 ), e.item )
    {
      background = true;
      aoe = -1;
      base_dd_min = base_dd_max = e.driver()->effectN( 3 ).average( e.item );
    }
  };

  struct alltotem_ice_damage_t : public proc_spell_t
  {
    alltotem_ice_damage_t( const special_effect_t& e ) :
      proc_spell_t( "elemental_stance_ice", e.player, e.player->find_spell( 382133 ), e.item )
    {
      background = true;
      aoe = -1;
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }
  };

  struct alltotem_buffs_t : public proc_spell_t
  {
    buff_t* earth_buff;
    buff_t* fire_buff;
    buff_t* air_buff;
    buff_t* ice_buff;
    std::vector<buff_t*> buffs;
    buff_t* first;

    alltotem_buffs_t( const special_effect_t& e ) :
      proc_spell_t( "alltotem_of_the_master", e.player, e.player -> find_spell( 377457 ), e.item )
    {

      earth_buff = make_buff<stat_buff_t>(e.player, "elemental_stance_earth", e.player->find_spell(377458))
          ->add_stat(STAT_BONUS_ARMOR, e.driver()->effectN(5).average(e.item));
      auto earth_damage = create_proc_action<alltotem_earth_damage_t>( "elemental_stance_earth", e );
      earth_buff->set_stack_change_callback( [ earth_damage ](buff_t*, int, int new_)
      {
        if( new_ == 1 )
        {
          earth_damage->execute();
        }
      } );
      fire_buff = make_buff(e.player, "elemental_stance_fire", e.player->find_spell(377459))
          ->set_period(e.player->find_spell( 377459 )->effectN(3).period());
      auto fire_damage = create_proc_action<alltotem_fire_damage_t>( "elemental_stance_fire", e );
      auto fire_dot = create_proc_action<alltotem_fire_dot_damage_t>( "elemental_stance_fire_dot", e );
      fire_buff->set_stack_change_callback( [ fire_damage ](buff_t*, int, int new_)
      {
        if( new_ == 1 )
        {
          fire_damage->execute();
        }
      } );
      fire_buff->set_tick_callback( [ fire_dot ]( buff_t* /* buff */, int /* current_tick */, timespan_t /* tick_time */ )
      {
        fire_dot->execute();
      } );
      air_buff = make_buff<stat_buff_t>(e.player, "elemental_stance_air", e.player->find_spell(377461))
           ->add_stat(STAT_HASTE_RATING, e.driver()->effectN(5).average(e.item));
      auto air_damage = create_proc_action<alltotem_air_damage_t>( "elemental_stance_air", e );
      air_buff->set_stack_change_callback( [ air_damage ](buff_t*, int, int new_)
      {
        if( new_ == 1 )
        {
          air_damage->execute();
        }
      } );
      ice_buff = make_buff(e.player, "elemental_stance_ice", e.player->find_spell(382133));
      ice_buff->set_default_value(e.driver()->effectN(2).average(e.item));
      auto ice_damage = create_proc_action<alltotem_ice_damage_t>( "elemental_stance_ice", e );
      ice_buff->set_stack_change_callback( [ ice_damage ](buff_t*, int, int new_)
      {
        if( new_ == 1 )
        {
          ice_damage->execute();
        }
      } );

      first = buffs.emplace_back( earth_buff );
      buffs.push_back( fire_buff );
      buffs.push_back( air_buff);
      buffs.push_back( ice_buff );

      add_child(earth_damage);
      add_child(fire_damage);
      add_child(fire_dot);
      add_child(ice_damage);
      add_child(air_damage);
    }

    void rotate()
    {
       buffs.front()->trigger();
       std::rotate(buffs.begin(), buffs.begin() + 1, buffs.end());
    }

    void execute() override
    {
      proc_spell_t::execute();
      rotate();
    }

    void reset() override
    {
      proc_spell_t::reset();
      std::rotate( buffs.begin(), range::find( buffs, first ), buffs.end() );
    }
  };

  action_t* action = create_proc_action<alltotem_buffs_t>( "alltotem_of_the_master", effect );

  if ( effect.player->role == ROLE_TANK )
  {
    effect.player->register_combat_begin( [ &effect, action ]( player_t* ) {
    timespan_t base_period = effect.driver()->internal_cooldown();
    timespan_t period =
        base_period + ( effect.player->sim->dragonflight_opts.alltotem_of_the_master_period +
                        effect.player->rng().range( 0_s, 12_s ) / ( 1 + effect.player->sim->target_non_sleeping_list.size() ) );
    make_repeating_event( effect.player->sim, period, [ action ]() { action->execute(); } );
    } );
  }
}

void tome_of_unstable_power( special_effect_t& effect )
{
  auto buff_spell = effect.player->find_spell( 388583 );
  auto data_spell = effect.player->find_spell( 391290 );

  auto buff = create_buff<stat_buff_t>( effect.player, buff_spell );

  buff->set_duration( effect.driver()->duration() );
  buff->set_stat_from_effect( 1, data_spell->effectN( 1 ).average( effect.item ) )
      ->add_stat_from_effect( 2, data_spell->effectN( 2 ).average( effect.item ) );

  effect.custom_buff = buff;
}

// Algethar Puzzle Box
// 383781 Driver and Buff
void algethar_puzzle_box( special_effect_t& effect )
{
  struct puzzle_box_channel_t : public proc_spell_t
  {
    buff_t* buff;
    action_t* use_action;  // if this exists, then we're prechanneling via the APL

    puzzle_box_channel_t( const special_effect_t& e, buff_t* solved ) :
      proc_spell_t( "algethar_puzzle_box_channel", e.player, e.driver(), e.item)
    {
      channeled = hasted_ticks = true;
      harmful = false;
      dot_duration = base_tick_time = base_execute_time;
      base_execute_time = 0_s;
      buff = solved;
      effect = &e;
      interrupt_auto_attack = false;

      for ( auto a : player->action_list )
      {
        if ( a->action_list && a->action_list->name_str == "precombat" && a->name_str == "use_item_" + item->name_str )
        {
          a->harmful = harmful;  // pass down harmful to allow action_t::init() precombat check bypass
          use_action = a;
          use_action->base_execute_time = 2_s;
          break;
        }
      }
    }

    void execute() override
    {
      if ( !player->in_combat )  // if precombat...
      {
        if ( use_action )  // ...and use_item exists in the precombat apl
        {
          precombat_buff();
        }
      }
      else
      {
        proc_spell_t::execute();
        event_t::cancel( player->readying );
        player->delay_ranged_auto_attacks( composite_dot_duration( execute_state ) );
      }
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;

      cooldown->adjust( d->duration() );

      auto cdgrp = player->get_cooldown( effect->cooldown_group_name() );
      cdgrp->adjust( d->duration() );

      proc_spell_t::last_tick(d);
      buff->trigger();

      if ( was_channeling && !player->readying )
        player->schedule_ready();
    }

    void precombat_buff()
    {
      timespan_t time = 0_ms;

      if ( time == 0_ms )  // No global override, check for an override from an APL variable
      {
        for ( auto v : player->variables )
        {
          if ( v->name_ == "algethar_puzzle_box_precombat_cast" )
          {
            time = timespan_t::from_seconds( v->value() );
            break;
          }
        }
      }

      // shared cd (other trinkets & on-use items)
      auto cdgrp = player->get_cooldown( effect->cooldown_group_name() );

      if ( time == 0_ms )  // No hardcoded override, so dynamically calculate timing via the precombat APL
      {
        time            = 2_s;  // base 2s cast
        const auto& apl = player->precombat_action_list;

        auto it = range::find( apl, use_action );
        if ( it == apl.end() )
        {
          sim->print_debug(
            "WARNING: Precombat /use_item for Algethar Puzzle Box exists but not found in precombat APL!" );
          return;
        }

        cdgrp->start( 1_ms );  // tap the shared group cd so we can get accurate action_ready() checks

                               // add cast time or gcd for any following precombat action
        std::for_each( it + 1, apl.end(), [ &time, this ]( action_t* a ) {
          if ( a->action_ready() )
          {
            timespan_t delta =
              std::max( std::max( a->base_execute_time, a->trigger_gcd ) * a->composite_haste(), a->min_gcd );
            sim->print_debug( "PRECOMBAT: Algethar Puzzle Box precast timing pushed by {} for {}", delta,
              a->name() );
            time += delta;

            return a->harmful;  // stop processing after first valid harmful spell
          }
          return false;
          } );
      }
      else if ( time < 2_s )  // If APL variable can't set to less than cast time
      {
        time = 2_s;
      }

      // how long you cast for
      auto cast = 2_s;
      // total duration of the buff
      auto total = buff->buff_duration();
      // actual duration of the buff you'll get in combat
      auto actual = total + cast - time;
      // cooldown on effect/trinket at start of combat
      auto cd_dur = cooldown->duration + cast - time;
      // shared cooldown at start of combat
      auto cdgrp_dur = std::max( 0_ms, effect->cooldown_group_duration() + cast - time );

      sim->print_debug(
        "PRECOMBAT: Algethar Puzzle Box started {}s before combat via {}, {}s in-combat buff",
        time, use_action ? "APL" : "DRAGONFLIGHT_OPT", actual );

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, actual );

      if ( use_action )  // from the apl, so cooldowns will be started by use_item_t. adjust. we are still in precombat.
      {
        make_event( *sim, [ this, cast, time, cdgrp ] {  // make an event so we adjust after cooldowns are started
          cooldown->adjust( cast - time );

          if ( use_action )
            use_action->cooldown->adjust( cast - time );

          cdgrp->adjust( cast - time );
          } );
      }
      else  // via bfa. option override, start cooldowns. we are in-combat.
      {
        cooldown->start( cd_dur );

        if ( use_action )
          use_action->cooldown->start( cd_dur );

        if ( cdgrp_dur > 0_ms )
          cdgrp->start( cdgrp_dur );
      }
    }
  };
  auto value = effect.driver()->effectN( 1 ).average( effect.item );

  if (effect.player->specialization() == DEATH_KNIGHT_UNHOLY)
  {
    auto mod = 1.0 + effect.player -> find_spell( 137007 ) -> effectN( 6 ).percent();
    value = value * mod;
  }

  auto buff_spell = effect.player->find_spell( 383781 );
  buff_t* buff    = create_buff<stat_buff_t>( effect.player, buff_spell )
    ->set_stat_from_effect( 1, value )
    ->set_cooldown( 0_ms );

  auto action = new puzzle_box_channel_t( effect, buff );
  effect.execute_action = action;
  effect.disable_buff();
}

// Frenzying Signoll Flare
// 382119 Driver
// 384290 Smorf's Ambush
// 384294 Siki's Ambush
// 386168 Barf's Ambush
void frenzying_signoll_flare(special_effect_t& effect)
{

  if ( unique_gear::create_fallback_buffs( effect, { "sikis_ambush_crit_rating", "sikis_ambush_mastery_rating", "sikis_ambush_haste_rating",
                  "sikis_ambush_versatility_rating" } ) )
   return;

  struct smorfs_ambush_t : public proc_spell_t
  {
    smorfs_ambush_t( const special_effect_t& e ) :
    proc_spell_t( "smorfs_ambush", e.player, e.player -> find_spell(384290), e.item)
    {
      background = true;
      base_td = e.driver()->effectN( 3 ).average( e.item );
      base_tick_time = e.player -> find_spell(384290) -> effectN( 1 ).period();
    }
  };

  struct barfs_ambush_t : public proc_spell_t
  {
    barfs_ambush_t( const special_effect_t& e ) :
    proc_spell_t( "barfs_ambush", e.player, e.player -> find_spell(386168), e.item)
    {
      background = true;
      base_dd_min = base_dd_max = e.driver() -> effectN( 2 ).average( e.item );
    }
  };

  struct frenzying_signoll_flare_t : public proc_spell_t
  {
    action_t* smorfs;
    action_t* barfs;
    std::shared_ptr<std::map<stat_e, buff_t*>> siki_buffs;
    // When selecting the highest stat, the priority of equal secondary stats is Vers > Mastery > Haste > Crit.
    std::array<stat_e, 4> ratings;

    frenzying_signoll_flare_t(const special_effect_t& e) :
        proc_spell_t("frenzying_signoll_flare", e.player, e.player -> find_spell(382119), e.item)
    {
      background = true;

      smorfs = create_proc_action<smorfs_ambush_t>( "smorfs_ambush", e );
      barfs = create_proc_action<barfs_ambush_t>( "barfs_ambush", e );

      // Use a separate buff for each rating type so that individual uptimes are reported nicely and APLs can easily
      // reference them. Store these in pointers to reduce the size of the events that use them.
      siki_buffs = std::make_shared<std::map<stat_e, buff_t*>>();
      double amount = e.driver()->effectN( 1 ).average( e.item );

      ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING,
                                                     STAT_CRIT_RATING };
      for ( auto stat : ratings )
      {
        auto name = std::string( "sikis_ambush_" ) + util::stat_type_string( stat );
        buff_t* buff = buff_t::find( e.player, name );

        if ( !buff )
        {
           buff = make_buff<stat_buff_t>( e.player, name, e.player->find_spell( 384294 ), e.item )
                 ->add_stat( stat, amount );
         }
        ( *siki_buffs )[ stat ] = buff;
      }

      add_child(smorfs);
      add_child(barfs);
    }

    void execute() override
    {
      proc_spell_t::execute();

      auto selected_effect = player->sim->rng().range( 3 );

      if( selected_effect == 0 )
      {
        smorfs->execute();
      }

      else if( selected_effect == 1 )
      {
        barfs->execute();
      }

      else if (selected_effect == 2 )
      {
        stat_e max_stat = util::highest_stat( player, ratings );
        ( *siki_buffs )[ max_stat ]->trigger();
      }
    }
  };
  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.execute_action = create_proc_action<frenzying_signoll_flare_t>( "frenzying_signoll_flare", effect );
  new dbc_proc_callback_t( effect.player, effect );
}

// Idol of Trampling Hooves
// 386175 Driver
// 392908 Buff
// 385533 Damage
void idol_of_trampling_hooves(special_effect_t& effect)
{
  auto buff_spell = effect.player->find_spell( 392908 );
  auto buff = create_buff<stat_buff_t>( effect.player , buff_spell );
  buff -> set_default_value(effect.driver()->effectN(1).average(effect.item));
  auto damage = create_proc_action<generic_aoe_proc_t>( "trampling_hooves", effect, "trampling_hooves", 385533 );
  damage->split_aoe_damage = true;
  damage->aoe = -1;
  effect.execute_action = damage;
  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// Dragon Games Equipment
// 386692 Driver & Buff
// 386708 Damage Driver
// 386709 Damage
// 383950 Damage Value
void dragon_games_equipment(special_effect_t& effect)
{
  auto damage = create_proc_action<generic_proc_t>( "dragon_games_equipment", effect, "dragon_games_equipment", 386708 );
  damage->base_dd_min = damage -> base_dd_max = effect.player->find_spell(383950)->effectN(1).average(effect.item);
  damage->aoe = 0;

  auto buff_spell = effect.player->find_spell( 386692 );
  auto buff = create_buff<buff_t>( effect.player , buff_spell );
  buff->tick_on_application = false;
  // Override Duration to trigger the correct number of missiles. Testing as of 11-14-2022 shows it only spawning 3, rather than the 4 expected by spell data.
  buff->set_duration( ( buff_spell->duration() / 4 ) * effect.player->sim->dragonflight_opts.dragon_games_kicks * effect.player->rng().range( effect.player->sim->dragonflight_opts.dragon_games_rng, 1.25) );
  buff->set_tick_callback( [ damage ]( buff_t* b, int, timespan_t ) {
        damage->execute_on_target( b->player->target );
      } );

  effect.custom_buff = buff;
}

// Bonemaw's Big Toe
// 397400 Driver & Buff
// 397401 Damage
void bonemaws_big_toe(special_effect_t& effect)
{
  auto buff_spell = effect.player->find_spell( 397400 );
  auto buff = create_buff<stat_buff_t>( effect.player , buff_spell );
  auto value = effect.driver()->effectN(1).average(effect.item);
  buff->add_stat( STAT_CRIT_RATING, value );
  auto damage = create_proc_action<generic_aoe_proc_t>( "fetid_breath", effect, "fetid_breath", 397401 );
  buff->set_tick_callback( [ damage ]( buff_t*, int, timespan_t )
    {
      damage->execute();
    } );
  damage->split_aoe_damage = true;
  damage->aoe = -1;
  effect.custom_buff = buff;
}

// Mutated Magmammoth Scale
// 381705 Driver
// 381727 Buff & Damage Driver
// 381760 Damage
void mutated_magmammoth_scale(special_effect_t& effect)
{
  auto buff_spell = effect.player->find_spell( 381727 );
  auto buff = create_buff<buff_t>( effect.player , buff_spell );
  auto damage = create_proc_action<generic_aoe_proc_t>( "mutated_tentacle_slam", effect, "mutated_tentacle_slam", 381760, true );
  buff->set_tick_callback( [ damage ]( buff_t*, int, timespan_t )
    {
      damage->execute();
    } );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN(1).average(effect.item);
  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// Homeland Raid Horn
// 382139 Driver & Buff
// 384003 Damage Driver
// 384004 Damage
// 387777 Damage value
void homeland_raid_horn(special_effect_t& effect)
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "dwarven_barrage", effect, "dwarven_barrage", 384004, true );
  damage->base_dd_min = damage -> base_dd_max = effect.player->find_spell( 387777 )->effectN( 1 ).average( effect.item );

  auto buff_spell = effect.player->find_spell( 382139 );
  auto buff = create_buff<buff_t>( effect.player , buff_spell );
  buff->set_tick_callback( [ damage ]( buff_t*, int, timespan_t ) {
    damage->execute();
  } );

  effect.custom_buff = buff;
}

// Blazebinder's Hoof
// 383926 Driver & Buff
// 389710 Damage
void blazebinders_hoof(special_effect_t& effect)
{
  struct burnout_wave_t : public proc_spell_t
  {
    double buff_stacks;

    burnout_wave_t( const special_effect_t& e ) :
    proc_spell_t( "burnout_wave", e.player, e.player -> find_spell( 389710 ), e.item), buff_stacks()
    {
      // Value stored in effect 2 appears to be the maximum damage that can be done with 6 stacks. Divide it by max stacks to get damage per stack.
      base_dd_min = base_dd_max = e.driver()->effectN(2).average(e.item) / e.driver()->max_stacks();
      split_aoe_damage = true;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = proc_spell_t::composite_da_multiplier( s );
      // Damage increased linerally by number of stacks
      m *= buff_stacks;

      return m;
    }
  };

  struct bound_by_fire_buff_t : public stat_buff_t
  {
    burnout_wave_t* damage;

    bound_by_fire_buff_t( const special_effect_t& e, action_t* a )
        : stat_buff_t( e.player, "bound_by_fire_and_blaze", e.driver(), e.item ),
        damage( debug_cast<burnout_wave_t*>( a ) )
    {
      set_default_value( e.driver()->effectN( 1 ).average( e.item ) );
      // Disable refresh on the buff to model in game behavior
      set_refresh_behavior( buff_refresh_behavior::DISABLED );
      // Set cooldown on buff to 0 to prevent triggering the cooldown on buff procs
      set_cooldown( 0_ms );
      // Set chance to prevent using the RPPM chance for buff applications
      set_chance( 1.01 );
    }

    void expire_override( int s, timespan_t d ) override
    {
      stat_buff_t::expire_override( s, d );
      // Debug cast to pass stack count through to the damage action
      damage -> buff_stacks = s;
      // Execute the debug cast
      damage -> execute();
    }
  };

  auto damage = create_proc_action<burnout_wave_t>( "burnout_wave", effect );
  auto buff = make_buff<bound_by_fire_buff_t>( effect, damage );

  special_effect_t* bound_by_fire_and_blaze = new special_effect_t( effect.player );
  bound_by_fire_and_blaze->source = effect.source;
  bound_by_fire_and_blaze->spell_id = effect.driver()->id();
  bound_by_fire_and_blaze->custom_buff = buff;
  bound_by_fire_and_blaze->cooldown_ = 0_ms;

  effect.player->special_effects.push_back( bound_by_fire_and_blaze );
  effect.proc_chance_ = 1.01;
  effect.custom_buff = buff;
  // since the on-use effect doesn't use the rppm, set to 0 so trinket expressions correctly determine it has a cooldown
  effect.ppm_ = 0;

  auto cb = new dbc_proc_callback_t( effect.player, *bound_by_fire_and_blaze );
  cb -> deactivate();

  buff->set_stack_change_callback( [ cb ](buff_t*, int, int new_)
  {
    if( !new_ )
    {
      cb->deactivate();
    }
    else
    {
      cb->activate();
    }
  } );
}

void primal_ritual_shell( special_effect_t& effect )
{
  const auto& blessing = effect.player->sim->dragonflight_opts.primal_ritual_shell_blessing;
  {
    if ( util::str_compare_ci( blessing, "wind" ) )
    {
      // Wind Turtle's Blessing - Mastery Proc [390899]
      effect.stat_amount = effect.driver()->effectN( 5 ).average( effect.item );
      effect.spell_id = 390899;
      effect.stat = STAT_MASTERY_RATING;
      effect.name_str = util::tokenize_fn( effect.trigger()->name_cstr() );
    }
    else if ( util::str_compare_ci( blessing, "stone" ) )
    {
      // TODO: Implement Stone Turtle's Blessing - Absorb-Shield Proc [390655]
      effect.player->sim->error( "Stone Turtle's Blessing is not implemented yet" );
    }
    else if ( util::str_compare_ci( blessing, "flame" ) )
    {
      // Flame Turtle's Blessing - Fire Damage Proc [390835]
      effect.discharge_amount = effect.driver()->effectN( 3 ).average( effect.item );
      effect.spell_id = 390835;
      effect.name_str = util::tokenize_fn( effect.trigger()->name_cstr() );
    }
    else if ( util::str_compare_ci( blessing, "sea" ) )
    {
      // TODO: Implement Sea Turtle's Blessing - Heal Proc [390869]
      effect.player->sim->error( "Sea Turtle's Blessing is not implemented yet" );
    }
    else
    {
      throw std::invalid_argument( "Invalid string for dragonflight.primal_ritual_shell_blessing. Valid strings: wind,flame,sea,stone" );
    }
  }
  new dbc_proc_callback_t( effect.player, effect );
}

// Spineripper's Fang
// 383611 Driver
// 383612 Damage spell (damage value is in the driver effect)
// Double damage bonus from behind, not in spell data anywhere
void spinerippers_fang( special_effect_t& effect )
{
  auto action = create_proc_action<generic_proc_t>( "rip_spine", effect, "rip_spine",
                                                    effect.driver()->effectN( 1 ).trigger() );

  double damage = effect.driver()->effectN( 1 ).average( effect.item );
  if ( effect.player->position() == POSITION_BACK ||
       effect.player->position() == POSITION_RANGED_BACK )
  {
    damage *= 2.0;
  }
  action->base_dd_min = action->base_dd_max = damage;

  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

// Integrated Primal Fire
// 392359 Driver and fire damage impact
// 392376 Physical damage impact spell, damage in driver effect 2
void integrated_primal_fire( special_effect_t& effect )
{
  struct cataclysmic_punch_t : public generic_proc_t
  {
    cataclysmic_punch_t( const special_effect_t& e ) : generic_proc_t( e, "cataclysmic_punch", e.driver() )
    {
      // Physical Damage
      impact_action = create_proc_action<generic_proc_t>(
        "cataclysmic_punch_knock", e, "cataclysmic_punch_knock", e.driver()->effectN( 4 ).trigger() );
      impact_action->base_dd_min = impact_action->base_dd_max = e.driver()->effectN( 2 ).average( e.item );
      add_child( impact_action );
    }
  };

  effect.execute_action = create_proc_action<cataclysmic_punch_t>( "cataclysmic_punch", effect );
}

// Bushwhacker's Compass
// 383817 Driver
// 383818 Buffs
// North = Crit, South = Haste, East = Vers, West = Mastery
void bushwhackers_compass(special_effect_t& effect)
{
  struct cb_t : public dbc_proc_callback_t
  {
    std::vector<buff_t*> buffs;
    cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      auto the_path_to_survival_mastery = create_buff<stat_buff_t>(effect.player, "the_path_to_survival_mastery", effect.trigger() )
        -> add_stat( STAT_MASTERY_RATING, effect.trigger()->effectN( 5 ).average( effect.item ));

      auto the_path_to_survival_haste = create_buff<stat_buff_t>(effect.player, "the_path_to_survival_haste", effect.trigger() )
        -> add_stat( STAT_HASTE_RATING, effect.trigger()->effectN( 5 ).average( effect.item ));

      auto the_path_to_survival_crit = create_buff<stat_buff_t>(effect.player, "the_path_to_survival_crit", effect.trigger() )
        -> add_stat( STAT_CRIT_RATING, effect.trigger()->effectN( 5 ).average( effect.item ));

      auto the_path_to_survival_vers = create_buff<stat_buff_t>(effect.player, "the_path_to_survival_vers", effect.trigger() )
        -> add_stat( STAT_VERSATILITY_RATING, effect.trigger()->effectN( 5 ).average( effect.item ));

      buffs =
      {
        the_path_to_survival_mastery,
        the_path_to_survival_haste,
        the_path_to_survival_crit,
        the_path_to_survival_vers
      };
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto buff = effect.player -> sim -> rng().range( buffs.size() );
      buffs[ buff ] -> trigger();
    }
  };
  effect.buff_disabled = true;
  new cb_t( effect );
}

void seasoned_hunters_trophy( special_effect_t& effect )
{
  struct seasoned_hunters_trophy_cb_t : public dbc_proc_callback_t
  {
    buff_t *mastery, *haste, *crit;

    seasoned_hunters_trophy_cb_t( const special_effect_t& e, buff_t* m, buff_t* h, buff_t* c ) :
      dbc_proc_callback_t( e.player, e ), mastery( m ), haste( h ), crit( c )
    { }

    void execute( action_t* /* a */, action_state_t* /* s */ ) override
    {
      mastery->expire();
      haste->expire();
      crit->expire();

      auto pet_count = range::count_if( listener->active_pets, []( const pet_t* pet ) {
        return pet->type == PLAYER_PET;
      } );

      switch ( pet_count )
      {
        case 0:
          mastery->trigger();
          break;
        case 1:
          haste->trigger();
          break;
        default:
          crit->trigger();
          break;
      }
    }
  };

  buff_t* mastery = buff_t::find( effect.player, "hunter_versus_wild" );
  buff_t* haste = buff_t::find( effect.player, "hunters_best_friend" );
  buff_t* crit = buff_t::find( effect.player, "pack_mentality" );

  if ( !mastery )
  {
    mastery = create_buff<stat_buff_t>( effect.player, "hunter_versus_wild",
        effect.player->find_spell( 392271 ) )
      ->add_stat( STAT_MASTERY_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  if ( !haste )
  {
    haste = create_buff<stat_buff_t>( effect.player, "hunters_best_friend",
        effect.player->find_spell( 392275 ) )
      ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  if ( !crit )
  {
    crit = create_buff<stat_buff_t>( effect.player, "pack_mentality",
        effect.player->find_spell( 392248 ) )
      ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  new seasoned_hunters_trophy_cb_t( effect, mastery, haste, crit );
}

// Gral's Discarded Tooth
// 374233 Driver
// 374249 Damage Driver
// 374250 Damage
void grals_discarded_tooth( special_effect_t& effect )
{
  auto missile = effect.trigger();
  auto trigger = missile -> effectN( 1 ).trigger();

  auto damage = create_proc_action<generic_aoe_proc_t>( "chill_of_the_depths", effect, "chill_of_the_depths", trigger -> id(), true);
  damage -> base_dd_min = damage -> base_dd_max = effect.driver()->effectN(1).average(effect.item);
  damage -> travel_speed = missile -> missile_speed();

  effect.execute_action = damage;
  new dbc_proc_callback_t( effect.player, effect );
}

// Static Charged Scale
// 391612 Driver
// 392127 Damage
// 392128 Buff
// Todo: Implement self damage?
void static_charged_scale(special_effect_t& effect)
{
  auto buff_spell = effect.player -> find_spell( 392128 );
  auto buff = create_buff<stat_buff_t>( effect.player, buff_spell );
  buff -> set_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// Ruby Whelp Shell
// 383812 driver
// 389839 st damage
// 390234 aoe damage
// 389820 haste buff
// 383813 crit buff
// Proc selection seems to follow the following distribution from data collected so far.
// - 1/8 for each training level to trigger the trained proc. If a
//   training level is not spent, the proc is random from all six.
// - 1/8 probability to trigger a random proc from a context-aware list
//   of possible procs. The exact conditions for dynamically populating
//   this list are currently unknown. By default, simc will consider all
//   procs to be part of this list.
// - 1/8 probability to trigger a random untrained proc. What happens in
//   the case where all procs have been trained to level 1 has not been
//   tested in game, but simc will randomly select any proc in that case.
void ruby_whelp_shell( special_effect_t& effect )
{
  enum ruby_whelp_type_e
  {
    FIRE_SHOT,
    LOBBING_FIRE_NOVA,
    CURING_WHIFF,
    MENDING_BREATH,
    SLEEPY_RUBY_WARMTH,
    UNDER_RED_WINGS,
    RUBY_WHELP_TYPE_MAX,
    RUBY_WHELP_TYPE_NONE = -1
  };

  auto string_to_whelp_type = [] ( std::string_view whelp_string )
  {
    if ( util::str_compare_ci( whelp_string, "fire_shot" ) )
      return FIRE_SHOT;
    if ( util::str_compare_ci( whelp_string, "lobbing_fire_nova" ) )
      return LOBBING_FIRE_NOVA;
    if ( util::str_compare_ci( whelp_string, "curing_whiff" ) )
      return CURING_WHIFF;
    if ( util::str_compare_ci( whelp_string, "mending_breath" ) )
      return MENDING_BREATH;
    if ( util::str_compare_ci( whelp_string, "sleepy_ruby_warmth" ) )
      return SLEEPY_RUBY_WARMTH;
    if ( util::str_compare_ci( whelp_string, "under_red_wings" ) )
      return UNDER_RED_WINGS;
    return RUBY_WHELP_TYPE_NONE;
  };

  std::vector<int> training_levels( static_cast<size_t>( RUBY_WHELP_TYPE_MAX ), 0 );
  std::vector<ruby_whelp_type_e> context_aware_procs;
  std::vector<std::string_view> splits;

  std::string_view training_str = effect.player->sim->dragonflight_opts.ruby_whelp_shell_training;
  if ( !effect.player->dragonflight_opts.ruby_whelp_shell_training.is_default() )
  {
    training_str = effect.player->dragonflight_opts.ruby_whelp_shell_training;
  }

  std::string_view context_str = effect.player->sim->dragonflight_opts.ruby_whelp_shell_context;
  if ( !effect.player->dragonflight_opts.ruby_whelp_shell_context.is_default() )
  {
    context_str = effect.player->dragonflight_opts.ruby_whelp_shell_context;
  }

  // Setup training levels based on the option.
  splits = util::string_split<std::string_view>( training_str, "/" );
  int total_levels = 0;
  for ( auto s : splits )
  {
    auto pair = util::string_split<std::string_view>( s, ":" );
    if ( pair.size() != 2 )
      throw std::invalid_argument( "Invalid string for dragonflight.ruby_whelp_shell_training." );

    ruby_whelp_type_e whelp_type = string_to_whelp_type( pair[ 0 ] );
    if ( whelp_type == RUBY_WHELP_TYPE_NONE )
      throw std::invalid_argument( "Invalid training type for dragonflight.ruby_whelp_shell_training." );

    int level = util::to_int( pair[ 1 ] );
    if ( level < 0 || level > 6 )
      throw std::invalid_argument( "Invalid training level for dragonflight.ruby_whelp_shell_training." );

    total_levels += level;
    if ( total_levels > 6 )
      throw std::invalid_argument( "More than 6 training levels cannot be applied with dragonflight.ruby_whelp_shell_training." );

    training_levels[ static_cast<size_t>( whelp_type ) ] = level;
  }

  // Setup context aware procs based on the option.
  splits = util::string_split<std::string_view>( context_str, "/" );
  for ( auto s : splits )
  {
    ruby_whelp_type_e whelp_type = string_to_whelp_type( s );
    if ( whelp_type == RUBY_WHELP_TYPE_NONE )
      throw std::invalid_argument( "Invalid proc type for dragonflight.ruby_whelp_shell_context." );

    if ( range::find( context_aware_procs, whelp_type ) != context_aware_procs.end() )
      throw std::invalid_argument( "Duplicate proc type found for dragonflight.ruby_whelp_shell_context." );

    context_aware_procs.push_back( whelp_type );
  }

  struct ruby_whelp_assist_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> training_levels;
    std::vector<ruby_whelp_type_e> context_aware_procs;
    std::vector<ruby_whelp_type_e> untrained_procs;
    int untrained_levels;

    action_t* shot;
    action_t* nova;
    stat_buff_t* haste;
    stat_buff_t* crit;

    ruby_whelp_assist_cb_t( const special_effect_t& e, std::vector<int> t, std::vector<ruby_whelp_type_e> c ) :
      dbc_proc_callback_t( e.player, e ), training_levels( t ), context_aware_procs( c )
    {
      ruby_whelp_type_e whelp_type = RUBY_WHELP_TYPE_NONE;
      untrained_levels = 6;
      for ( auto level : training_levels )
      {
        whelp_type++;
        if ( level == 0 )
          untrained_procs.push_back( whelp_type );
        else
          untrained_levels -= level;
      }

      shot = create_proc_action<generic_proc_t>( "fire_shot", e, "fire_shot", 389839 );
      shot->base_dd_min = shot->base_dd_max = e.driver()->effectN( 1 ).average( e.item );

      nova = create_proc_action<generic_aoe_proc_t>( "lobbing_fire_nova", e, "lobbing_fire_nova", 390234, true );
      nova->base_dd_min = nova->base_dd_max = e.driver()->effectN( 2 ).average( e.item );

      haste = create_buff<stat_buff_t>( e.player, e.player->find_spell( 389820 ) );
      haste->set_stat( STAT_HASTE_RATING, e.driver()->effectN( 6 ).average( e.item ) );

      crit = create_buff<stat_buff_t>( e.player, e.player->find_spell( 383813 ) );
      crit->set_stat( STAT_CRIT_RATING, e.driver()->effectN( 5 ).average( e.item ) );
    }

    void trigger_whelp_proc( ruby_whelp_type_e whelp_type, action_state_t* s )
    {
      switch ( whelp_type )
      {
        case FIRE_SHOT:
          shot->execute_on_target( s->target );
          break;
        case LOBBING_FIRE_NOVA:
          nova->execute_on_target( s->target );
          break;
        case SLEEPY_RUBY_WARMTH:
          crit->trigger();
          break;
        case UNDER_RED_WINGS:
          haste->trigger();
          break;
        default:
          // skip 2 heal possibilities
          break;
      }
    }

    void trigger_random_whelp_proc( action_state_t* s )
    {
      int choice = rng().range( static_cast<int>( RUBY_WHELP_TYPE_MAX ) );
      trigger_whelp_proc( static_cast<ruby_whelp_type_e>( choice ), s );
    }

    void trigger_random_whelp_proc( std::vector<ruby_whelp_type_e>& whelp_types, action_state_t* s )
    {
      if ( whelp_types.empty() )
      {
        // If no types are available, fallback to fully random.
        trigger_random_whelp_proc( s );
      }
      else
      {
        size_t choice = rng().range( whelp_types.size() );
        trigger_whelp_proc( whelp_types[ choice ], s );
      }
    }

    void execute( action_t*, action_state_t* s ) override
    {
      double r = rng().real();

      // Helper function for checking if each chance is
      // successful using the random number generated above.
      auto whelp_roll = [ &r ] ( double chance )
      {
        r -= chance;
        return r < 0.0;
      };

      // Try the trained levels.
      ruby_whelp_type_e whelp_type = RUBY_WHELP_TYPE_NONE;
      for ( auto level : training_levels )
      {
        whelp_type++;
        if ( whelp_roll( level / 8.0 ) )
        {
          trigger_whelp_proc( whelp_type, s );
          return;
        }
      }

      // Try the untrained levels.
      if ( whelp_roll( untrained_levels / 8.0 ) )
      {
        trigger_random_whelp_proc( s );
        return;
      }

      // Try the context-aware procs.
      if ( whelp_roll( 1.0 / 8.0 ) )
      {
        trigger_random_whelp_proc( context_aware_procs, s );
        return;
      }

      // If nothing else succeeded, trigger a random untrained proc.
      trigger_random_whelp_proc( untrained_procs, s );
    }
  };

  new ruby_whelp_assist_cb_t( effect, training_levels, context_aware_procs );
}

// Desperate Invoker's Codex
// 377465 procs the buff 382419 with every cast which reduces the proc action cd by 1s per stack
void desperate_invokers_codex( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "hatred" } ) )
    return;

  auto hatred =
      create_buff<buff_t>( effect.player, effect.player->find_spell( 382419 ) )->set_default_value_from_effect( 1, 1 );

  auto desperate_invocation          = find_special_effect( effect.player, 377465 );
  desperate_invocation->proc_flags_  = PF_ALL_DAMAGE;
  desperate_invocation->proc_flags2_ = PF2_CAST_DAMAGE;
  desperate_invocation->custom_buff  = hatred;
  auto cb                            = new dbc_proc_callback_t( effect.player, *desperate_invocation );
  cb->initialize();

  struct desperate_invocation_t : public generic_proc_t
  {
    buff_t* buff;
    cooldown_t* item_cd;
    cooldown_t* spell_cd;

    desperate_invocation_t( const special_effect_t& e, buff_t* b )
      : generic_proc_t( e, "desperate_invocation", e.driver() ),
        buff( b ),
        item_cd( e.player->get_cooldown( e.cooldown_name() ) ),
        spell_cd( e.player->get_cooldown( e.name() ) )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 377465 )->effectN( 2 ).average( e.item );
    }

    void execute() override
    {
      generic_proc_t::execute();
      make_event( *sim, [ this ]() { item_cd->adjust( -timespan_t::from_seconds( buff->check_stack_value() ) ); } );
      spell_cd->adjust( -timespan_t::from_seconds( buff->check_stack_value() ) );
    }
  };

  effect.execute_action = create_proc_action<desperate_invocation_t>( "Desperate Invocation", effect, hatred );
  hatred->set_stack_change_callback( [ effect ]( buff_t* b, int, int ) {
    if ( !b->at_max_stacks() )
    {
      effect.player->get_cooldown( effect.cooldown_name() )->adjust( -timespan_t::from_seconds( b->check_value() ) );
      effect.player->get_cooldown( effect.name() )->adjust( -timespan_t::from_seconds( b->check_value() ) );
    }
  } );

  effect.player->register_on_combat_state_callback( [ hatred ]( player_t*, bool c ) {
    if ( !c )
      hatred->expire();
  } );
}


// Iceblood Deathsnare
// 377455 is the instant damage on the main target
// 382130 is the debuff applied to the main target
// 394618 is applied to secondary targets
// 382131 is the damage proc that goes on all targets with either 382130 or 394618
// 382132 has the damage

void iceblood_deathsnare( special_effect_t& effect )
{
  struct iceblood_deathsnare_damage_proc_t : public generic_proc_t
  {
    iceblood_deathsnare_damage_proc_t( const special_effect_t& e )
      : generic_proc_t( e, "iceblood_deathsnare_proc", 382131 )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 382132 )->effectN( 2 ).average( e.item );
      aoe                       = -1;
      reduced_aoe_targets       = 5;
    }

    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double am = generic_proc_t::composite_aoe_multiplier( state );

      // Uses a unique damage scaling formula (data here:
      // https://docs.google.com/spreadsheets/d/1FzIE9ZYrGaCQ2vvf_QJon4aa5Z2PAzMTBMs8czMbY6o/edit#gid=0)
      // This is equivalent to (10/9)^(1-n)*n and an additional standard sqrt dr after 5 targets.
      am *= std::pow( 10.0 / 9.0 , 1.0 - state->n_targets ) * state->n_targets;

      return am;
    }

    std::vector<player_t*>& target_list() const override
    {
      target_cache.is_valid = false;

      auto& tl = proc_spell_t::target_list();

      tl.erase( std::remove_if(
                    tl.begin(), tl.end(),
                    [ this ]( player_t* t ) { return !player->get_target_data( t )->debuff.crystalline_web->up(); } ),
                tl.end() );

      return tl;
    }
  };



  struct iceblood_deathsnare_t : public generic_proc_t
  {
    iceblood_deathsnare_t( const special_effect_t& e ) : generic_proc_t( e, "iceblood_deathsnare", e.driver() )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 382132 )->effectN( 1 ).average( e.item );
      aoe                       = -1;
      base_aoe_multiplier       = 0;
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      player->get_target_data( s->target )->debuff.crystalline_web->trigger();
    }
  };

  // Create proc action for the debuff
  create_proc_action<iceblood_deathsnare_damage_proc_t>( "iceblood_deathsnare_proc", effect );

  effect.execute_action = create_proc_action<iceblood_deathsnare_t>( "iceblood_deathsnare", effect );
}

struct iceblood_deathsnare_cb_t : public dbc_proc_callback_t
{
  action_t* damage;

  iceblood_deathsnare_cb_t( const special_effect_t& e, action_t* damage )
    : dbc_proc_callback_t( e.player, e ), damage( damage )
  {
  }

  void execute( action_t*, action_state_t* s ) override
  {
    const auto td = effect.player->find_target_data( s->target );
    if ( td && td->debuff.crystalline_web->up() )
      damage->execute_on_target( s->target );
  }
};

struct iceblood_deathsnare_initializer_t : public item_targetdata_initializer_t
{
  iceblood_deathsnare_initializer_t() : item_targetdata_initializer_t( 377455 )
  {
  }

  void operator()( actor_target_data_t* td ) const override
  {
    if ( !find_effect( td->source ) )
    {
      td->debuff.crystalline_web = make_buff( *td, "crystalline_web" );
      return;
    }

    assert( !td->debuff.crystalline_web );
    auto web          = new special_effect_t( td->source );
    web->name_str     = "iceblood_deathsnare_proc_trigger";
    web->type         = SPECIAL_EFFECT_EQUIP;
    web->source       = SPECIAL_EFFECT_SOURCE_ITEM;
    web->proc_chance_ = 1.0;
    web->proc_flags_  = PF_ALL_DAMAGE;
    web->proc_flags2_ = PF2_ALL_HIT;
    web->cooldown_    = 1.5_s;
    td->source->special_effects.push_back( web );

    auto damage = td->source->find_action( "iceblood_deathsnare_proc" );

    // callback to proc damage
    auto damage_cb = new iceblood_deathsnare_cb_t( *web, damage );
    damage_cb->initialize();
    damage_cb->deactivate();

    td->debuff.crystalline_web =
        make_buff( *td, "crystalline_web", td->source->find_spell( spell_id )->effectN( 3 ).trigger() )
            ->set_cooldown(0_ms)
            ->set_stack_change_callback( [ damage_cb ]( buff_t*, int, int new_ ) {
              if ( new_ == 1 )
                damage_cb->activate();
              else
                damage_cb->deactivate();
            } );

    td->debuff.crystalline_web->reset();
  }
};

// Fractured Crystalspine Quill
// 408625 Driver / Absorb Buff
// 408903 Damage
void fractured_crystalspine_quill( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "fractured_crystalspine_quill", effect, "fractured_crystalspine_quill", effect.trigger(), true );
  auto absorb_buff = create_buff<absorb_buff_t>( effect.player, effect.driver(), effect.item );

  effect.execute_action = damage;
  effect.custom_buff    = absorb_buff;
}

/**Winterpelt Totem
 * id=398292 main cast
 * id=398293 Winterpelt's Blessing buff (proc driver)
 * id=398320 Winterpelt's Fury (damage)
 * id=398322 damage coefficient
 */
void winterpelt_totem( special_effect_t& effect )
{
  struct winterpelts_fury_t : public generic_proc_t
  {
    winterpelts_fury_t( const special_effect_t& e ) :
      generic_proc_t( e, "winterpelts_fury", 398320 )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 398322 )->effectN( 1 ).average( e.item );
    }
  };

  auto blessing            = new special_effect_t( effect.player );
  blessing->name_str       = "winterpelts_blessing_cb";
  blessing->type           = SPECIAL_EFFECT_EQUIP;
  blessing->source         = SPECIAL_EFFECT_SOURCE_ITEM;
  blessing->spell_id       = 398293;
  blessing->execute_action = create_proc_action<winterpelts_fury_t>( "winterpelts_fury", effect );
  effect.player->special_effects.push_back( blessing );

  auto blessing_cb = new dbc_proc_callback_t( effect.player, *blessing );
  blessing_cb->initialize();
  blessing_cb->deactivate();

  effect.custom_buff = make_buff( effect.player, "winterpelts_blessing", blessing->driver() )
                         ->set_stack_change_callback( [ blessing_cb ] ( buff_t*, int, int new_ )
                           { if ( new_ ) blessing_cb->activate(); else blessing_cb->deactivate(); } );
}

void screaming_black_dragonscale_equip( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
                           ->add_stat_from_effect( 2, effect.driver()->effectN( 1 ).average( effect.item ) )
                           ->add_stat_from_effect( 3, effect.driver()->effectN( 2 ).average( effect.item ) );

  new dbc_proc_callback_t( effect.player, effect );
}

void screaming_black_dragonscale_use( special_effect_t& effect )
{
  if ( effect.player->sim->dragonflight_opts.screaming_black_dragonscale_damage )
  {
    auto damage           = create_proc_action<generic_aoe_proc_t>( "screaming_descent", effect, "screaming_descent", effect.driver() );
    damage->base_dd_min   = damage->base_dd_max = effect.player->find_spell( 401468 )->effectN( 3 ).average( effect.item );
    effect.execute_action = damage;
  }
  else
  {
    // set the type to SPECIAL_EFFECT_NONE to disable all handling of this effect.
    effect.type = SPECIAL_EFFECT_NONE;
  }
}
// Anshuul the Cosmic Wanderer
// 402583 Driver
// 402574 Damage Value
void anshuul_the_cosmic_wanderer( special_effect_t& effect )
{
  struct anshuul_damage_t : public generic_aoe_proc_t
  {
    anshuul_damage_t( const special_effect_t& e )
      : generic_aoe_proc_t( e, "anshuul_the_cosmic_wanderer", e.driver(), true )
    {
      base_dd_min = base_dd_max = e.trigger()->effectN( 1 ).average( e.item );
      base_execute_time = 0_ms; // Overriding execute time, this is handled by the main action. 
      not_a_proc = true; // Due to cast time on the primary ability
    }
  };

  struct anshuul_channel_t : public proc_spell_t
  {
    action_t* damage;
    anshuul_channel_t( const special_effect_t& e )
      : proc_spell_t( "anshuul_channel", e.player, e.driver(), e.item ),
        damage( create_proc_action<anshuul_damage_t>( "anshuul_the_cosmic_wanderer", e ) )
    {
      channeled = hasted_ticks = true;
      dot_duration = base_tick_time = base_execute_time;
      base_execute_time             = 0_s;
      aoe                           = 0;
      interrupt_auto_attack         = false;
      effect                        = &e;
      // This is actually a cast, you can queue spells out of it - Do not incur channel lag.
      ability_lag        = sim->queue_lag;
      ability_lag_stddev = sim->queue_lag_stddev;
      // Child action handles travel time
      min_travel_time = travel_speed = travel_delay = 0;
    }

    void execute() override
    {
      proc_spell_t::execute();
      event_t::cancel( player->readying );
      player->delay_ranged_auto_attacks( composite_dot_duration( execute_state ) );
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;
      auto cdgrp          = player->get_cooldown( effect->cooldown_group_name() );

      // Cancelled before the last tick completed, reset the cd
      if ( d->end_event )
      {
        cooldown->reset( false );
        cdgrp->reset( false );
      }
      else
      {
        cooldown->adjust( d->duration() );
        cdgrp->adjust( d->duration() );
      }

      proc_spell_t::last_tick( d );
      damage->execute();
      if ( was_channeling && !player->readying )
        player->schedule_ready( 0_ms );
    }
  };

  auto damage           = create_proc_action<anshuul_channel_t>( "anshuul_channel", effect );
  effect.execute_action = damage;
}

// Zaqali Chaos Grapnel
// 400956 Driver
// 400955 Damage Values
// 400959 Missile
void zaqali_chaos_grapnel( special_effect_t& effect )
{
  struct zaqali_chaos_grapnel_missile_t : public generic_aoe_proc_t
  {
    zaqali_chaos_grapnel_missile_t( const special_effect_t& e ) : generic_aoe_proc_t( e, "furious_impact", e.player -> find_spell( 400959 ), true )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 400955 )->effectN( 2 ).average( e.item );
    }
  };

  struct zaqali_chaos_grapnel_t : public generic_proc_t
  {
    action_t* missile;
    zaqali_chaos_grapnel_t( const special_effect_t& e ) : generic_proc_t( e, "impaling_grapnel", e.driver() ),
        missile( create_proc_action<zaqali_chaos_grapnel_missile_t>( "furious_impact", e ) )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 400955 )->effectN( 1 ).average( e.item );
      add_child( missile );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      missile -> execute();
    }
  };

  effect.execute_action = create_proc_action<zaqali_chaos_grapnel_t>( "impaling_grapnel", effect );
}

// TODO: Confirm which driver is Druid and Rogue, spell data at the time of implementation (17/03/2023) was unclear
void neltharions_call_to_suffering( special_effect_t& effect )
{
  int driver_id = effect.spell_id;

  switch ( effect.player->type )
  {
    case DEATH_KNIGHT:
      driver_id = 408089;
      break;
    case DRUID:
      driver_id = 408090;
      break;
    case ROGUE:
      driver_id = 408042;
      break;
    case PRIEST:
      driver_id = 408087;
      break;
    default:
      return;
  }

  struct call_to_suffering_self_t : public generic_proc_t
  {
    call_to_suffering_self_t( const special_effect_t& e )
      : generic_proc_t( e, "call_to_suffering", e.player->find_spell( 408469 ) )
    {
      base_td = e.driver()->effectN( 2 ).average( e.item );
      not_a_proc = true;
      stats->type = stats_e::STATS_NEUTRAL;
      target = e.player;
    }

    // logs show this can trigger helpful periodic proc flags
    result_amount_type amount_type( const action_state_t*, bool ) const override
    {
      return result_amount_type::HEAL_OVER_TIME;
    }
  };

  // Buff scaling is on the main trinket driver.
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 403386 ) )
                           ->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect.item ) );

  effect.execute_action = create_proc_action<call_to_suffering_self_t>( "call_to_suffering", effect );

  // After setting up the buff set the driver to the Class Specific Driver that holds RPPM Data
  effect.spell_id = driver_id;

  new dbc_proc_callback_t( effect.player, effect );
}

void neltharions_call_to_chaos( special_effect_t& effect )
{
  int driver_id = effect.spell_id;

  switch ( effect.player->type )
  {
    case WARRIOR:
      driver_id = 408122;
      break;
    case PALADIN:
      driver_id = 408123;
      break;
    case MAGE:
      driver_id = 408126;
      break;
    case DEMON_HUNTER:
      driver_id = 408128;
      break;
    case EVOKER:
      driver_id = 408127;
      break;
    default:
      return;
  }

  // Buff scaling is on the main trinket driver.
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 403382 ) )
                           ->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect.item ) );

  // After setting up the buff set the driver to the Class Specific Driver that holds RPPM Data
  effect.spell_id = driver_id;
  std::set<unsigned> proc_spell_id;

  // Triggers only on AoE Abilities - Abilities that *can* AoE or abilities that *did* AoE. Evokers need a hack.
  switch ( effect.player->type )
  {
    case EVOKER:
      // Horrifying state hack to trick the Evoker Module into thinking there is a pre-execute state for the is_aoe
      // calls for eternity surge as the action n_targets is based on that state.
      effect.player->callbacks.register_callback_trigger_function(
          driver_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
          []( const dbc_proc_callback_t*, action_t* a, action_state_t* s ) {
            auto _s              = a->pre_execute_state;
            a->pre_execute_state = s;
            auto b               = a->is_aoe();
            a->pre_execute_state = _s;
            return b;
          } );
      break;
    case WARRIOR:
      proc_spell_id = { { 1680, 190411, 396719, 6343, 384318, 376079, 46968, 845, 262161, 227847, 50622, 385059, 228920, 156287, 6572  } };
      // only true AoE, in order - Whirlwind Arms, Whirlwind Fury, Thunder Clap, Thunder Clap Prot, Thunderous Roar
      // Spear of Bastion, Shockwave, Cleave, Warbreaker, Bladestorm, BS Tick, Odyns Fury, Ravager, Rav Tick, Revenge
      effect.player->callbacks.register_callback_trigger_function(
          driver_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
          [ proc_spell_id ]( const dbc_proc_callback_t*, action_t* a, action_state_t* ) {
            return range::contains( proc_spell_id, a->data().id() );
          } );
      break;
    case PALADIN:
    case MAGE:
    case DEMON_HUNTER:
    default:
      // Evoker is unique with it working on cleave abilities. Other classes likely need an ability whitelist but this is a better aproximation for now.
      effect.player->callbacks.register_callback_trigger_function(
          driver_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
          []( const dbc_proc_callback_t*, action_t* a, action_state_t* ) { return a->n_targets() == -1; } );
  }

  new dbc_proc_callback_t( effect.player, effect );
}

void neltharions_call_to_dominance( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "call_to_dominance", "domineering_arrogance" } ) )
    return;

  auto stacking_buff = buff_t::find( effect.player, "domineering_arrogance" );

  if ( !stacking_buff )
  {
    stacking_buff = create_buff<buff_t>( effect.player, "domineering_arrogance", effect.player->find_spell( 411661 ) )
                                                       ->set_default_value( effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  effect.custom_buff = stacking_buff;

  auto stat_effect = new special_effect_t( effect.player );

  auto stat_buff = buff_t::find( effect.player, "call_to_dominance" );

  if ( !stat_buff )
  {
    stat_buff = create_buff<stat_buff_t>( effect.player, "call_to_dominance", effect.player->find_spell( 403380 ) )
                      ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), stacking_buff->default_value )
                      ->set_max_stack( stacking_buff->max_stack() ); // This buff does not have stacks in game but we need to treat it as such in simc for clarity/ease-of-use
  }

  stat_effect->custom_buff = stat_buff;
  stat_effect->type = SPECIAL_EFFECT_EQUIP;
  stat_effect->source = SPECIAL_EFFECT_SOURCE_ITEM;

  std::set<unsigned> proc_spell_id;
  unsigned driver_id = effect.spell_id;

  switch ( effect.player->specialization() )
  {
    case SHAMAN_ELEMENTAL:
      driver_id = 408259;
      proc_spell_id = { { 198067, 192249 } }; // Fire Elemental and Storm Elemental
      break;
    case SHAMAN_ENHANCEMENT:
      driver_id = 408259;
      proc_spell_id = { { 51533 } }; // Feral Spirit
      break;
    case SHAMAN_RESTORATION:
      driver_id = 408259;
      proc_spell_id = { { 98008, 108280, 16191 } }; // Spirit Link, Healing Tide, Mana Tide totems
      break;
    case MONK_BREWMASTER:
      driver_id = 408260;
      proc_spell_id = { { 132578 /*, 395267*/ } };  // Invoke Niuzao, Weapons of Order's Call to Arms
      break;
    case MONK_WINDWALKER:
      driver_id = 408260;
      proc_spell_id = { { 123904 } }; // Invoke Xuen
      break;
    case MONK_MISTWEAVER:
      driver_id = 408260;
      proc_spell_id = { { 322118, 325197 } }; // Invoke Yu'lon or Chi-Ji
      break;
    case WARLOCK_DESTRUCTION:
      driver_id = 408256;
      proc_spell_id = { { 1122 /*, 387160 */ } }; // Summon Infernal. 2023-04-30 PTR: Summon Blasphemy no longer procs trinket. May be intentional.
      break;
    case WARLOCK_DEMONOLOGY:
      driver_id = 408256;
      proc_spell_id = { { 265187 } }; // Summon Demonic Tyrant
      break;
    case WARLOCK_AFFLICTION:
      driver_id = 408256;
      proc_spell_id = { { 205180 } }; // Summon Darkglare
      break;
    case HUNTER_SURVIVAL:
      driver_id = 408262;
      proc_spell_id = { { 201430, 360969 } }; // Stampede and Coordinated Assault
      break;
    case HUNTER_MARKSMANSHIP:
      driver_id = 408262;
      proc_spell_id = { { 288613, 378905 } }; // Trueshot & Windrunner's Guidance procs
      break;
    case HUNTER_BEAST_MASTERY:
      driver_id = 408262;
      proc_spell_id = { { 19574 } }; // Bestial Wrath
      break;
    default:
      return;
  }

  stat_effect->spell_id = driver_id;

  effect.player->special_effects.push_back( stat_effect );

  stat_effect->player->callbacks.register_callback_trigger_function(
      stat_effect->spell_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ proc_spell_id ]( const dbc_proc_callback_t*, action_t* a, action_state_t* ) {
      return range::contains( proc_spell_id, a->data().id() );
    } );

  stat_effect->player->callbacks.register_callback_execute_function(
    stat_effect->spell_id, [ stacking_buff, stat_buff ]( const dbc_proc_callback_t*, action_t*, action_state_t* ) {
      // 2023-04-21 PTR: Subsequent triggers will override existing buff even if lower value (tested with Beast Mastery)
      if ( stacking_buff->check() )
      {
        stat_buff->trigger( stacking_buff->check() );
        stacking_buff->expire();
      }
    } );

  new dbc_proc_callback_t( effect.player, effect );
  new dbc_proc_callback_t( effect.player, *stat_effect );
}

// Elementium Pocket Anvil
// 401303 Main Spell/Value Container
// 401306 On use Cast time/Cooldown
// 401324 Equip Damage
// 408578 Stacking Buff In combat
// 408533 Stacking Buff Out of Combat
// 408513 Warrior Driver
// 408534 Rogue Driver
// 408535 Paladin Driver
// 408536 Monk Driver
// 408537 Demon Hunter Driver
// 408538 Death Knight Driver
// 408539 Druid Driver
// 408540 Hunter Driver
// 408584 Shaman Driver
// TODO - Whitelist Druid, Hunter, Rogue, Shaman, Warrior
// Procs From:
// DK - Heart Strike( 206930 ), Obliterate( 49020, 66198, 222024, 325431 ), Scourge Strike( 55090, 70890, 207311 )
// DH - Chaos Strike, Annihilation, Soul Cleave
// Druid - Mangle, Maul, Shred
// Monk - Tiger Palm
// Hunter - Raptor Strike, Mongoose Bite
// Rogue - Gloomblade, Sinister Strike, Multilate
// Paladin - Crusader Strike, Hammer of the Righteous, Blessed Hammer, Crusading Strikes, Templar Slash, Templar Strikes
// Shaman - Stormstrike
// Warrior - Shield Slam, Mortal Strike, Raging Blow, Annihilator
void elementium_pocket_anvil( special_effect_t& e )
{
  auto anvil_strike_combat =
      create_buff<buff_t>( e.player, "anvil_strike_combat", e.player->find_spell( 408578 ) )
          ->set_cooldown( 0_ms )
          ->set_default_value( e.player->find_spell( 401303 )->effectN( 3 ).percent() );

  auto anvil_strike_no_combat =
      create_buff<buff_t>( e.player, "anvil_strike_no_combat", e.player->find_spell( 408533 ) )
          ->set_default_value( e.player->find_spell( 401303 )->effectN( 3 ).percent() );

  struct elementium_pocket_anvil_use_t : public generic_proc_t
  {
    buff_t* combat_buff;
    buff_t* no_combat_buff;

    elementium_pocket_anvil_use_t( const special_effect_t& e, buff_t* combat, buff_t* no_combat )
      : generic_proc_t( e, "anvil_strike", e.driver() ), combat_buff( combat ), no_combat_buff( no_combat )
    {
      aoe         = -1;
      base_dd_min = base_dd_max = e.player->find_spell( 401303 )->effectN( 2 ).average( e.item );

      interrupt_auto_attack = false;
    }

    void execute() override
    {
      generic_proc_t::execute();

      if ( player->in_combat )
        combat_buff->trigger();
      else
        no_combat_buff->trigger();
    }
  };

  struct elementium_pocket_anvil_equip_t : public generic_proc_t
  {
    buff_t* combat_buff;
    buff_t* no_combat_buff;

    elementium_pocket_anvil_equip_t( const special_effect_t& e, buff_t* combat, buff_t* no_combat )
      : generic_proc_t( e, "echoed_flare", e.player->find_spell( 401324 ) ),
        combat_buff( combat ),
        no_combat_buff( no_combat )
    {
      // 27-4-2023, Probably a bug? Damage value on tooltip multiplied by 0.6x. Tooltip matches in game test result.
      // Retest later to ensure this is still the behavior experienced.
      base_dd_min = base_dd_max = e.player->find_spell( 401303 )->effectN( 1 ).average( e.item ) * 0.6;
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = generic_proc_t::composite_da_multiplier( state );

      m *= 1.0 + combat_buff->check_stack_value();
      m *= 1.0 + no_combat_buff->check_stack_value();

      return m;
    }
  };

  unsigned driver_id = e.spell_id;
  std::set<unsigned> proc_spell_id;
  bool trigger_from_melees = false;

  switch ( e.player->type )
  {
    case DEATH_KNIGHT:
      driver_id     = 408538;
      proc_spell_id = { { // Blood DK
                          206930,
                          // Frost DK
                          49020, 66198, 222024, 325431,
                          // Unholy DK
                          55090, 70890, 207311 } };
      break;
    case DEMON_HUNTER:
      driver_id     = 408537;
      proc_spell_id = { { // Vengeance DH
                          228478,
                          // Havoc
                          199547, 222031, 227518, 201428 } };
      break;
    case DRUID:
      driver_id = 408539;

      for ( const auto& n : { "shred", "mangle", "maul", "raze" } )
        if ( auto a = e.player->find_action( n ) )
          proc_spell_id.insert( a->data().id() );

      break;
    case HUNTER:
      driver_id     = 408540;
      proc_spell_id = { {
          // Raptor Strike (Melee, Ranged)
          186270,  // 265189, 2023-04-06 - Does not work on Raptor Strike during Aspect of the Eagle buff
                   // Mongoose Bite (Melee , Ranged)
          259387,  // 265888, 2023-04-06 - Does not work on Mongoose Bite during Aspect of the Eagle buff
      } };
      break;
    case MONK:
      driver_id     = 408536;
      proc_spell_id = { { // Tiger Palm
                          100780 } };
      break;
    case PALADIN:
      driver_id     = 408535;
      proc_spell_id = { {
          // Shared
          35395,  // Crusader Strike

          // Protection Paladin
          53595, 204019,  // Hammer of the Righteous, Blessed Hammer

          // Retribution Paladin
          404139, 404542, 406647, 407480  // Blessed Hammers, Crusading Strikes, Templar Slash, Templar Strikes
      } };
      // All Paladin Specs (Even Holy) trigger it from Melee attacks, even without Seal of the Crusader
      trigger_from_melees = true;
      break;
    case ROGUE:
      driver_id = 408534;
      // Does not currently work on Shadowstrike for Subtley or Ambush for Assassination/Outlaw
      proc_spell_id = { { // Assassination
                          5374, 27576,
                          // Outlaw -- Sinister Strike
                          193315, 197834,
                          // Subtlety -- Backstab, Gloomblade
                          53, 200758 } };
      break;
    case SHAMAN:
      driver_id     = 408584;
      proc_spell_id = { { 115356, 17364 } };
      break;
    case WARRIOR:
      driver_id     = 408513;
      proc_spell_id = { {
          // Protection
          23922,  // Shield Slam
                  // Arms
          12294,  // Mortal Strike
                  // Fury
          85288, 85384, 96103, 383916, 383915  // Raging Blow & Annihilator
      } };
      break;
    default:
      return;
  }

  auto equip            = new special_effect_t( e.player );
  equip->spell_id       = driver_id;
  equip->execute_action = create_proc_action<elementium_pocket_anvil_equip_t>( "echoed_flare", e, anvil_strike_combat,
                                                                               anvil_strike_no_combat );
  equip->type           = SPECIAL_EFFECT_EQUIP;
  e.player->special_effects.push_back( equip );

  equip->player->callbacks.register_callback_trigger_function(
      driver_id, dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ proc_spell_id, trigger_from_melees ]( const dbc_proc_callback_t*, action_t* a, action_state_t* ) {
        return range::contains( proc_spell_id, a->data().id() ) || ( trigger_from_melees && !a->special );
      } );

  new dbc_proc_callback_t( e.player, *equip );

  e.execute_action = create_proc_action<elementium_pocket_anvil_use_t>( "anvil_strike", e, anvil_strike_combat,
                                                                        anvil_strike_no_combat );
  equip->execute_action->add_child( e.execute_action );

  e.player->register_on_combat_state_callback( [ anvil_strike_combat, anvil_strike_no_combat ]( player_t*, bool c ) {
    if ( c )
    {
      if ( auto stack = anvil_strike_no_combat->check() )
      {
        anvil_strike_combat->trigger( stack );
        anvil_strike_no_combat->expire();
      }
    }
    else
    {
      if ( auto stack = anvil_strike_combat->check() )
      {
        anvil_strike_no_combat->trigger( stack );
        anvil_strike_combat->expire();
      }
    }
  } );
}

// Ominous Chromatic Essence
// 401513 Value Container
// Ruby - 401513, Minor - 405613
// Obsidian - 402221, Minor - 405615
// Emerald - 401521, Minor - 405608
// Bronze - 401518, Minor - 405612
// Azure - 401519, Minor - 405611
void ominous_chromatic_essence( special_effect_t& e )
{
  buff_t* buff;
  buff_t* obsidian_minor;
  buff_t* ruby_minor;
  buff_t* bronze_minor;
  buff_t* azure_minor;
  buff_t* emerald_minor;
  double main_value       = e.driver()->effectN( 1 ).average( e.item );
  double minor_value      = e.driver()->effectN( 2 ).average( e.item );
  const auto& flight      = e.player->dragonflight_opts.ominous_chromatic_essence_dragonflight;
  bool valid              = false;
  bool has_obsidian_major = false;
  bool has_ruby_major     = false;
  bool has_bronze_major   = false;
  bool has_azure_major    = false;
  bool has_emerald_major  = false;
  bool has_obsidian_minor = false;
  bool has_ruby_minor     = false;
  bool has_bronze_minor   = false;
  bool has_azure_minor    = false;
  bool has_emerald_minor  = false;

  if ( util::str_compare_ci( flight, "obsidian" ) )
  {
    main_value = main_value / 4;  // Obsidian Dragonflight splits the total value evenly among all secondaries
    buff       = create_buff<stat_buff_t>( e.player, "obsidian_resonance", e.player->find_spell( 402221 ) )
               ->add_stat_from_effect( 2, main_value )
               ->add_stat_from_effect( 3, main_value )
               ->add_stat_from_effect( 4, main_value )
               ->add_stat_from_effect( 5, main_value );
    valid              = true;
    has_obsidian_major = true;
  }

  if ( util::str_compare_ci( flight, "ruby" ) )
  {
    buff = create_buff<stat_buff_t>( e.player, "ruby_resonance", e.player->find_spell( 401516 ) )
               ->add_stat_from_effect( 2, main_value );
    valid          = true;
    has_ruby_major = true;
  }

  if ( util::str_compare_ci( flight, "bronze" ) )
  {
    buff = create_buff<stat_buff_t>( e.player, "bronze_resonance", e.player->find_spell( 401518 ) )
               ->add_stat_from_effect( 2, main_value );
    valid            = true;
    has_bronze_major = true;
  }

  if ( util::str_compare_ci( flight, "azure" ) )
  {
    buff = create_buff<stat_buff_t>( e.player, "azure_resonance", e.player->find_spell( 401519 ) )
               ->add_stat_from_effect( 2, main_value );
    valid           = true;
    has_azure_major = true;
  }

  if ( util::str_compare_ci( flight, "emerald" ) )
  {
    buff = create_buff<stat_buff_t>( e.player, "emerald_resonance", e.player->find_spell( 401521 ) )
               ->add_stat_from_effect( 2, main_value );
    valid             = true;
    has_emerald_major = true;
  }

  auto splits =
      util::string_split<std::string_view>( e.player->dragonflight_opts.ominous_chromatic_essence_allies, "/" );
  for ( auto s : splits )
  {
    if ( util::str_compare_ci( s, "obsidian" ) )
    {
      if ( !has_obsidian_major )
      {
        has_obsidian_minor = true;
      }
    }
    else if ( util::str_compare_ci( s, "ruby" ) )
    {
      if ( !has_ruby_major )
      {
        has_ruby_minor = true;
      }
    }
    else if ( util::str_compare_ci( s, "bronze" ) )
    {
      if ( !has_bronze_major )
      {
        has_bronze_minor = true;
      }
    }
    else if ( util::str_compare_ci( s, "azure" ) )
    {
      if ( !has_azure_major )
      {
        has_azure_minor = true;
      }
    }
    else if ( util::str_compare_ci( s, "emerald" ) )
    {
      if ( !has_emerald_major )
      {
        has_emerald_minor = true;
      }
    }
    else if ( util::str_compare_ci( s, "" ) )
    {
      return;
    }
    else
    {
      e.player->sim->error(
          "'{}' Is not a valid Dragonflight for Ominous Chromatic Essence Allies. Options are: obsidian, azure, "
          "emerald, ruby, or bronze",
          s );
    }
  }

  // Minor Buffs
  if ( has_obsidian_minor )
  {
    obsidian_minor = create_buff<stat_buff_t>( e.player, "minor_obsidian_resonance", e.player->find_spell( 405615 ) )
                         ->add_stat_from_effect( 2, minor_value / 4 )
                         ->add_stat_from_effect( 3, minor_value / 4 )
                         ->add_stat_from_effect( 4, minor_value / 4 )
                         ->add_stat_from_effect( 5, minor_value / 4 );
  }

  if ( has_ruby_minor )
  {
    ruby_minor = create_buff<stat_buff_t>( e.player, "minor_ruby_resonance", e.player->find_spell( 405613 ) )
                     ->add_stat_from_effect( 2, minor_value );
  }

  if ( has_bronze_minor )
  {
    bronze_minor = create_buff<stat_buff_t>( e.player, "minor_bronze_resonance", e.player->find_spell( 405612 ) )
                       ->add_stat_from_effect( 2, minor_value );
  }

  if ( has_azure_minor )
  {
    azure_minor = create_buff<stat_buff_t>( e.player, "minor_azure_resonance", e.player->find_spell( 405611 ) )
                      ->add_stat_from_effect( 2, minor_value );
  }

  if ( has_emerald_minor )
  {
    emerald_minor = create_buff<stat_buff_t>( e.player, "minor_emerald_resonance", e.player->find_spell( 405608 ) )
                        ->add_stat_from_effect( 2, minor_value );
  }

  if ( valid )
  {
    e.player->register_combat_begin( [ buff, obsidian_minor, ruby_minor, bronze_minor, azure_minor, emerald_minor,
                                       has_obsidian_minor, has_ruby_minor, has_bronze_minor, has_azure_minor,
                                       has_emerald_minor ]( player_t* ) {
      buff->trigger();

      if ( has_obsidian_minor )
      {
        obsidian_minor->trigger();
      }
      if ( has_ruby_minor )
      {
        ruby_minor->trigger();
      }
      if ( has_bronze_minor )
      {
        bronze_minor->trigger();
      }
      if ( has_azure_minor )
      {
        azure_minor->trigger();
      }
      if ( has_emerald_minor )
      {
        emerald_minor->trigger();
      }
    } );
  }
  else
  {
    e.player->sim->error(
        "'{}' Is not a valid Dragonflight for Ominous Chromatic Essence. Options are: obsidian, azure, emerald, ruby, "
        "or bronze",
        flight );
  }
}

// Ward of the Facless Ire
// 401238 Driver & Absorb Shield
// 401257 DoT
// 401239 Value container
void ward_of_the_faceless_ire( special_effect_t& e )
{
  auto num_ticks = e.player->find_spell( 401257 )->duration() / e.player->find_spell( 401257 )->effectN( 2 ).period();
  auto damage_value = e.player->find_spell( 401239 )->effectN( 2 ).average( e.item ) / num_ticks;
  auto damage = create_proc_action<generic_proc_t>( "writhing_ire", e, "writhing_ire", e.player->find_spell( 401257 ) );
  damage->base_td = damage_value;

  auto absorb_buff = create_buff<absorb_buff_t>( e.player, e.driver() )
                         ->set_default_value( e.player->find_spell( 401239 )->effectN( 1 ).average( e.item ) )
                         ->set_stack_change_callback( [ damage ]( buff_t*, int, int new_ ) {
                           if ( !new_ )
                           {
                             damage->execute();
                           }
                         } );
  // If the player is a tank, properly model the absorb buff by casting it on themselves
  // Otherwise, emulate it as if the player is casting it on a player who instantly breaks the shield
  if ( e.player->role == ROLE_TANK )
  {
    e.custom_buff = absorb_buff;
  }
  else
  {
    e.execute_action = damage;
  }
}

// Treemouth's Festering Splinter
void treemouths_festering_splinter( special_effect_t& e )
{
  auto damage = create_proc_action<generic_proc_t>( "treemouths_festering_splinter", e, "treemouths_festering_splinter",
                                                    e.driver() );
  damage->base_dd_min = damage->base_dd_max =
      0;  // Override the damage to 0 as this is dealt to the player, not the target.
          // Letting Simc auto parse this portion makes it think the damage is dealt to the target.
  auto absorb_buff = create_buff<absorb_buff_t>( e.player, e.driver() )
      ->set_default_value_from_effect( 2 )
      ->set_cooldown( 0_ms ); // Overriding Cooldown as its handled by the action

  e.execute_action = damage;
  e.custom_buff    = absorb_buff;
}

// 401395 Driver
// 401394 Stacking Buff
// 401422 Direct Damage
// 401428 DoT
void vessel_of_searing_shadow( special_effect_t& e )
{
  struct vessel_of_searing_shadow_dot_t : public generic_proc_t
  {
    vessel_of_searing_shadow_dot_t( const special_effect_t& e )
      : generic_proc_t( e, "ravenous_shadowflame", e.player->find_spell( 401428 ) )
    {
      base_td = e.driver()->effectN( 3 ).average( e.item );
    }
  };

  struct vessel_of_searing_shadow_direct_t : public generic_proc_t
  {
    action_t* dot;

    vessel_of_searing_shadow_direct_t( const special_effect_t& e )
      : generic_proc_t( e, "shadow_spike", e.player->find_spell( 401422 ) ),
        dot( create_proc_action<vessel_of_searing_shadow_dot_t>( "ravenous_shadowflame", e ) )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 2 ).average( e.item );
      add_child( dot );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      dot->execute();
    }
  };

  struct vessel_of_searing_shadow_self_t : public generic_proc_t
  {
    vessel_of_searing_shadow_self_t( const special_effect_t& e )
      : generic_proc_t( e, "unstable_flames", e.player->find_spell( 401394 ) )
    {
      base_td = e.driver()->effectN( 1 ).average( e.item );
      not_a_proc = true;
      stats->type = stats_e::STATS_NEUTRAL;
      target = e.player;
    }

    // logs show this can trigger helpful periodic proc flags
    result_amount_type amount_type( const action_state_t*, bool ) const override
    {
      return result_amount_type::HEAL_OVER_TIME;
    }
  };

  auto damage = create_proc_action<vessel_of_searing_shadow_direct_t>( "shadow_spike", e );
  auto self = create_proc_action<vessel_of_searing_shadow_self_t>( "unstable_flames", e );

  auto stack_buff = create_buff<stat_buff_t>( e.player, "unstable_flames", e.player->find_spell( 401394 ) )
                        ->add_stat_from_effect( 3, e.driver()->effectN( 4 ).average( e.item ) )
                        ->set_expire_at_max_stack( true )
                        ->set_stack_change_callback( [ damage, self ]( buff_t* b, int, int new_ ) {
                          if ( !new_ )
                          {
                            self->find_dot( b->player )->cancel();
                            damage->execute();
                          }
                          else
                          {
                            self->execute();
                          }
                        } );

  e.custom_buff = stack_buff;

  new dbc_proc_callback_t( e.player, e );
}

// Heart of Thunder
// 413419 Driver
// 413423 Damage
void heart_of_thunder ( special_effect_t& e )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "thunderous_pulse", e, "thunderous_pulse", e.trigger() );

  e.execute_action = damage;
  new dbc_proc_callback_t( e.player, e );
}

// 407895 driver
// 407896 damage
void drogbar_rocks( special_effect_t& effect )
{
  effect.proc_flags2_ = PF2_CRIT;

  auto proc = create_proc_action<generic_proc_t>( "drogbar_rocks", effect, "drogbar_rocks", effect.trigger() );
  effect.execute_action = proc;
  new dbc_proc_callback_t( effect.player, effect );
}

// 407903 Proc driver
// 407904 Buff
// 407907 Damage action
void drogbar_stones( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "drogbar_stones" );
  if ( !buff )
  {
    auto buff_spell = effect.trigger();
    buff = create_buff<buff_t>( effect.player, buff_spell );

    auto drogbar_stones_damage = new special_effect_t( effect.player );
    drogbar_stones_damage->name_str = "drogbar_stones_damage";
    drogbar_stones_damage->item = effect.item;
    drogbar_stones_damage->spell_id = buff->data().id();
    drogbar_stones_damage->type = SPECIAL_EFFECT_EQUIP;
    drogbar_stones_damage->source = SPECIAL_EFFECT_SOURCE_ITEM;
    drogbar_stones_damage->execute_action = create_proc_action<generic_proc_t>(
      "drogbar_stones_damage", *drogbar_stones_damage, "drogbar_stones_damage", effect.player->find_spell( 407907 ) );

    drogbar_stones_damage->player->callbacks.register_callback_execute_function(
        drogbar_stones_damage->spell_id,
        [ buff, drogbar_stones_damage, effect ]( const dbc_proc_callback_t*, action_t*, action_state_t* ) {
          if ( buff->check() )
          {
            drogbar_stones_damage->execute_action->execute();

            buff->expire();
          }
        } );

    auto damage = new dbc_proc_callback_t( effect.player, *drogbar_stones_damage );
    damage->initialize();
    damage->deactivate();

    buff->set_stack_change_callback( [damage] ( buff_t*, int, int new_ ) {
    if ( new_ )
    {
      damage->activate();
    }
    else
    {
      damage->deactivate();
    }
    } );

  }
  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// 408607 driver
// 408984 spawned moth
// 408983 primary stat buff
void underlight_globe( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.player -> find_spell( 408983 ) )
    ->add_stat_from_effect( 1, effect.driver() -> effectN(1).average( effect.item ) );

  new dbc_proc_callback_t( effect.player, effect );
}

// 408641 driver
// 409067 trigger buff
// TODO: implement self-damage DoT
void stirring_twilight_ember( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>( effect.player, effect.trigger() )
                           ->add_stat_from_effect( 1, effect.trigger()->effectN( 1 ).average( effect.item ) );

  // Disable self-damage DoT for now
  effect.action_disabled = true;

  new dbc_proc_callback_t( effect.player, effect );
}

// Heatbound Medallion
// 407512 Driver/Damage
void heatbound_medallion( special_effect_t& e )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "heatbound_release", e, "heatbound_release", e.driver(), true );
  e.execute_action = damage;
}

// Firecaller's Focus
// 407523 driver & damage value
// 407524 travel time
// 407536 delay before damage
// 407537 damage
void firecallers_focus( special_effect_t& e )
{
  struct firecallers_focus_missile_t : public generic_proc_t
  {
    action_t* damage;
    timespan_t duration;
    firecallers_focus_missile_t( const special_effect_t& e )
      : generic_proc_t( e, "firecallers_focus", e.trigger() ),
        damage( create_proc_action<generic_aoe_proc_t>( "firecallers_explosion", e, "firecallers_explosion",
                                                        e.trigger()->effectN( 1 ).trigger()->effectN( 1 ).trigger(),
                                                        true ) ),
        duration( timespan_t::from_millis( e.trigger()->effectN( 1 ).trigger()->effectN( 1 ).misc_value1() ) )
    {
      damage->base_dd_min = base_dd_max =
          e.driver()->effectN( 1 ).average( e.item ) *
          2;  // Damage from this trinket appears to be doubled for some reason, likely a bug.

      base_dd_min = base_dd_max = 0;  // Disable damage on missile, auto parsing passes through damage from the driver.
      damage->dual = true;
      stats = damage->stats;
    }

    void impact( action_state_t* a ) override
    {
      generic_proc_t::impact( a );
      make_event<ground_aoe_event_t>(
          *sim, player,
          ground_aoe_params_t().target( a->target ).pulse_time( duration ).duration( duration ).action( damage ) );
    }
  };

  auto missile     = create_proc_action<firecallers_focus_missile_t>( "firecallers_focus", e );
  e.execute_action = missile;
  new dbc_proc_callback_t( e.player, e );
}

// Mirror of Fractured Tomorrows
// 418527 Driver, Buff and Value container
// Effect 1-5, Buff Values
// Effect 6, Sand Bolt value
// Effect 7, Sand Cleave value
// Effect 8, Sand Shield value
// Effect 9, Restorative Sands Value
// Effect 10, Auto Attack value
// Melee DPS:
// 208957 NPC ID
// 419591 Auto Attack 
// 418588 Sand Cleave
// 418774 Summon Driver
// Ranged DPS:
// 208887 NPC ID
// 418605 Sand Bolt Driver
// 418607 Sand Bolt Damage
// 418773 Summon Driver
// Tank:
// 208958 NPC ID
// 418999 Sand Shield
// 418775 Summon Driver
// Heal:
// 208959 NPC ID
// 418605 Sand Bolt Driver
// 418607 Sand Bolt Damage
// 419052 Restorative Sands
// 418776 Summon Driver
// TODO: 
// Check Tank Pet Auto Attack, as of 29-6-2023 was using default pet autos, scaling with player attack power.
// Re-evaluate healer pet APL logic, may need to be adjusted up/down based on log data for raid.
// Implement the cast start delay for Sand Bolt.
void mirror_of_fractured_tomorrows( special_effect_t& e )
{
  if ( unique_gear::create_fallback_buffs(
           e, { "mirror_of_fractured_tomorrows_crit_rating", "mirror_of_fractured_tomorrows_mastery_rating",
                "mirror_of_fractured_tomorrows_haste_rating", "mirror_of_fractured_tomorrows_versatility_rating" } ) )
    return;

  struct future_self_auto_attack_t : public spell_t
  {
    action_t* action;
    future_self_auto_attack_t( pet_t* p, const special_effect_t& e, action_t* a, util::string_view options_str )
      : spell_t( "auto_attack", p, p->find_spell( 419591 ) ), action( a )
    {
      parse_options( options_str );
      base_dd_min = base_dd_max = e.driver()->effectN( 10 ).average( e.item );
      auto proxy                = action;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }
  };

  struct sand_cleave_t : public spell_t
  {
    action_t* action;
    sand_cleave_t( pet_t* p, const special_effect_t& e, action_t* a, util::string_view options_str ) : spell_t( "sand_cleave", p, p->find_spell( 418588 ) ), action( a )
    {
      parse_options( options_str );
      aoe = -1;
      base_dd_min = base_dd_max = e.driver()->effectN( 7 ).average( e.item );
      auto proxy                = action;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
    }
  };

  struct sand_shield_t : public spell_t
  {
    buff_t* shield;
    action_t* action;
    sand_shield_t( pet_t* p, const special_effect_t& e, action_t* a, util::string_view options_str )
      : spell_t( "sand_shield", p, p->find_spell( 418999 ) ), shield( nullptr ), action( a )
    {
      parse_options( options_str );
      auto proxy                = action;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
      auto shield_id = p->find_spell( 418999 );
      shield         = create_buff<absorb_buff_t>( e.player, shield_id )
                   ->set_default_value( e.driver()->effectN( 8 ).average( e.item ) );
    }

    void execute() override
    {
      spell_t::execute();
      shield->trigger();
    }
  };

  struct restorative_sands_t : public heal_t
  {
    action_t* action;
    restorative_sands_t( pet_t* p, const special_effect_t& e, action_t* a, util::string_view options_str )
      : heal_t( "restorative_sands", p, p->find_spell( 419052 ) ), action( a )
    {
      parse_options( options_str );
      auto proxy                = action;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );
      base_dd_min = base_dd_max = e.driver() -> effectN( 9 ).average( e.item );
    }
  };

  struct sand_bolt_missile_t : public spell_t
  {
    action_t* action;
    sand_bolt_missile_t( pet_t* p, const special_effect_t& e, action_t* a, util::string_view options_str )
      : spell_t( "sand_bolt", p, p->find_spell( 418605 ) ), action( a )
    {
      parse_options( options_str );
      auto proxy                = action;
      auto it                   = range::find( proxy->child_action, data().id(), &action_t::id );
      if ( it != proxy->child_action.end() )
        stats = ( *it )->stats;
      else
        proxy->add_child( this );

      auto damage = create_proc_action<generic_proc_t>( "sand_bolt_damage", p, "sand_bolt_damage", p->find_spell( 418607 ) );
      damage -> base_dd_min = damage -> base_dd_max = e.driver()->effectN( 6 ).average( e.item );
      damage -> stats = stats;
      impact_action = damage;
    }
  };

  struct future_self_pet_t : public pet_t
  {
    const special_effect_t& effect;
    action_t* action;

    future_self_pet_t( const special_effect_t& e, action_t* a )
      : pet_t( e.player->sim, e.player, "future_self", true, true ), effect( e ), action( a )
    {
      unsigned pet_id;
      switch ( e.player->role )
      {
        case ROLE_ATTACK:
          pet_id = 208957;
          break;
        case ROLE_SPELL:
          pet_id = 208887;
          break;
        case ROLE_TANK:
          pet_id = 208958;
          break;
        case ROLE_HEAL:
          pet_id = 208959;
          break;
        default:
          return;
      }

      npc_id = pet_id;
    }

    void init_base_stats() override
    {
      pet_t::init_base_stats();
    }

    resource_e primary_resource() const override
    {
      return RESOURCE_NONE;
    }

    action_t* create_action( util::string_view name, util::string_view options ) override
    {
      if ( name == "auto_attack" )
      {
        return new future_self_auto_attack_t( this, effect, action, options );
      }

      if ( name == "sand_cleave" )
      {
        return new sand_cleave_t( this, effect, action, options );
      }

      if ( name == "sand_bolt" )
      {
        return new sand_bolt_missile_t( this, effect, action, options );
      }

      if ( name == "sand_shield" )
      {
        return new sand_shield_t( this, effect, action, options );
      }

      if ( name == "restorative_sands" )
      {
        return new restorative_sands_t( this, effect, action, options );
      }

      return pet_t::create_action( name, options );
    }

    void init_action_list() override
    {
      pet_t::init_action_list();

      auto def = get_action_priority_list( "default" );
      switch ( effect.player->role )
      {
        case ROLE_ATTACK:
          def->add_action( "sand_cleave" );
          def->add_action( "auto_attack" );
          break;
        case ROLE_SPELL:
          def->add_action( "sand_bolt" );
          break;
        case ROLE_TANK:
          def->add_action( "sand_shield" );
          def->add_action( "auto_attack" );
          break;
        case ROLE_HEAL:
          // Use APL logic to alternate between each spell, more properly emulating in game behavior
          def->add_action( "sand_bolt,if=prev.restorative_sands" );
          def->add_action( "restorative_sands" );
          break;
        default:
          return;
      }
    }
  };

  struct mirror_of_fractured_tomorrows_t : public spell_t
  {
    spawner::pet_spawner_t<future_self_pet_t> spawner;
    const special_effect_t& effect;
    std::array<stat_e, 4> ratings;
    std::shared_ptr<std::map<stat_e, buff_t*>> buffs;

    mirror_of_fractured_tomorrows_t( const special_effect_t& e )
      : spell_t( "mirror_of_fractured_tomorrows", e.player, e.driver() ),
        spawner( "future_self", e.player, [ &e, this ]( player_t* ) { return new future_self_pet_t( e, this ); } ),
        effect( e )
    {
      unsigned summon_driver;
      switch ( e.player->role )
      {
        case ROLE_ATTACK:
          summon_driver = 418774;
          break;
        case ROLE_SPELL:
          summon_driver = 418773;
          break;
        case ROLE_TANK:
          summon_driver = 418775;
          break;
        case ROLE_HEAL:
          summon_driver = 418776;
          break;
        default:
          return;
      }
      dual = false;
      auto amount = e.driver()->effectN( 1 ).average( e.item );
      buffs = std::make_shared<std::map<stat_e, buff_t*>>();
      ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };

      for ( auto stat : ratings )
      {
        auto name = std::string( "mirror_of_fractured_tomorrows_" ) + util::stat_type_string( stat );
        auto buff = create_buff<stat_buff_t>( e.player, name, e.player->find_spell( 418527 ) )
                    ->set_stat( stat, amount )
                    ->set_name_reporting( util::stat_type_abbrev( stat ) );

        ( *buffs )[ stat ] = buff;
      }

      spawner.set_default_duration( e.player->find_spell( summon_driver )->duration() );
    }

    void execute() override
    {
      spell_t::execute();
      spawner.spawn();

      stat_e max_stat = util::highest_stat( effect.player, ratings );
      ( *buffs )[ max_stat ]->trigger();
    }
  };

  e.disable_buff();
  e.execute_action = create_proc_action<mirror_of_fractured_tomorrows_t>( "mirror_of_fractured_tomorrows", e );
}

// Weapons
void bronzed_grip_wrappings( special_effect_t& effect )
{
  // TODO: currently whether the proc is damage or heal seems arbitrarily based on the spell that proc'd it, with no
  // discernable pattern as of yet.
  // * white melee hits will always proc damage
  // * white ranged hit will not proc anything
  // * heals will always proc heal
  // * yellow attacks and spells varies depending on the spell that procs it. some abilities always proc heal, others
  // always proc damage, yet others almost always proc only one type but have exceptions where it procs the other for
  // unknown reason.
  //
  // For now we err on the side of undersimming and disable all procs except for white hits. If this is not fixed by the
  // time it is available with raid launch, each spec will need to explicitly determine whether an ability will execute
  // the damage or heal on proc via effect_callback_t::register_callback_execute_function() to driver id 396442.
  struct bronzed_grip_wrappings_cb_t : public dbc_proc_callback_t
  {
    bronzed_grip_wrappings_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( s->target->is_sleeping() )
        return;

      if ( execute_fn )
      {
        ( *execute_fn )( this, a, s );
      }
      else
      {
        if ( !a->special )
          proc_action->execute_on_target( s->target );
      }
    }
  };

  auto amount = effect.driver()->effectN( 2 ).average( effect.item );

  effect.trigger_spell_id = effect.driver()->effectN( 2 ).trigger_spell_id();
  effect.spell_id = effect.driver()->effectN( 1 ).trigger_spell_id();
  effect.discharge_amount = amount;

  new bronzed_grip_wrappings_cb_t( effect );
}

void fang_adornments( special_effect_t& effect )
{
  effect.school = effect.driver()->get_school_type();
  effect.discharge_amount = effect.driver()->effectN( 1 ).average( effect.item );
  effect.override_result_es_mask |= RESULT_CRIT_MASK;
  effect.result_es_mask &= ~RESULT_CRIT_MASK;

  new dbc_proc_callback_t( effect.player, effect );
}

// Spore Keepers Baton
// 405226 Buff Driver
// 406793 Dot
// 405232 Vers Buff
// 406795 Absorb Shield
void spore_keepers_baton( special_effect_t& effect )
{
  auto dot     = create_proc_action<generic_proc_t>( "sporeadic_adaptability", effect, "sporeadic_adaptability", effect.player->find_spell( 406793 ) );
  dot->base_td = effect.driver()->effectN( 2 ).average( effect.item );

  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 405232 ) )
                  ->add_stat_from_effect( 1, effect.driver()->effectN( 1 ).average( effect.item ) );

  effect.player->callbacks.register_callback_execute_function(
      effect.driver()->id(), [ dot, buff ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* s ) {
        if ( s->result_type == result_amount_type::HEAL_DIRECT || s->result_type == result_amount_type::HEAL_OVER_TIME )
        {
          buff->trigger();
        }
        else
        {
          dot->set_target( cb->target( s ) );
          auto proc_state    = dot->get_state();
          proc_state->target = dot->target;
          dot->snapshot_state( proc_state, dot->amount_type( proc_state ) );
          dot->schedule_execute( proc_state );
        }
      } );

  new dbc_proc_callback_t( effect.player, effect );
}

// Forgestorm
// 381698 Buff Driver
// 381699 Buff and Damage Driver
// 381700 Damage
void forgestorm( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "forgestorm_ignited" );
  if ( !buff )
  {
    auto buff_spell = effect.trigger();
    buff = create_buff<buff_t>( effect.player, buff_spell );

    auto forgestorm_damage = new special_effect_t( effect.player );
    forgestorm_damage->name_str = "forgestorm_ignited_damage";
    forgestorm_damage->item = effect.item;
    forgestorm_damage->spell_id = buff->data().id();
    forgestorm_damage->type = SPECIAL_EFFECT_EQUIP;
    forgestorm_damage->source = SPECIAL_EFFECT_SOURCE_ITEM;
    forgestorm_damage->execute_action = create_proc_action<generic_aoe_proc_t>(
      "forgestorm_ignited_damage", *forgestorm_damage, "forgestorm_ignited_damage", effect.player->find_spell( 381700 ), true );
    forgestorm_damage->execute_action->base_dd_min = forgestorm_damage->execute_action->base_dd_max
      = effect.player->find_spell( 381698 )->effectN( 1 ).average( effect.item );
    effect.player -> special_effects.push_back( forgestorm_damage );

    auto damage = new dbc_proc_callback_t( effect.player, *forgestorm_damage );
    damage->initialize();
    damage->deactivate();

    buff->set_stack_change_callback( [damage]( buff_t*, int, int new_ )
    {
      if ( new_ )
      {
        damage->activate();
      }
      else
      {
        damage->deactivate();
      }
    } );
  }
  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}

// 394928 driver
// 397118 player buff
// 397478 unique target debuff
void neltharax( special_effect_t& effect )
{
  auto buff =
    create_buff<buff_t>( effect.player, "heavens_nemesis", effect.player->find_spell( 397118 ) )
      ->set_default_value_from_effect( 1 );

  if ( buff->data().effectN( 1 ).subtype() == A_MOD_RANGED_AND_MELEE_ATTACK_SPEED )
    buff->add_invalidate( CACHE_ATTACK_SPEED );

  effect.player -> buffs.heavens_nemesis = buff;

  struct auto_cb_t : public dbc_proc_callback_t
  {
    buff_t* attack_speed;

    auto_cb_t( const special_effect_t& e, buff_t* buff ) : dbc_proc_callback_t( e.player, e ), attack_speed( buff )
    {}

    void execute( action_t*, action_state_t* s ) override
    {
      auto debuff = effect.player -> find_target_data( s -> target ) -> debuff.heavens_nemesis;
      if ( debuff -> check() )
      {
        // Increment the attack speed if target is our current mark.
        attack_speed -> trigger();
      }
      else
      {
        attack_speed -> expire();
        // Find a current mark if there is one and remove the mark on it.
        auto i = range::find_if( effect.player -> sim -> target_non_sleeping_list, [ this ]( const player_t* t ) {
          auto td = effect.player -> find_target_data( t );
          if ( td ) {
            auto debuff = td -> debuff.heavens_nemesis;
            if ( debuff -> check() )
            {
              debuff -> expire();
              return true;
            }
          }
          return false;
        });
        // Only set a new mark and start stacking the buff if there was no mark cleared on this impact.
        if ( i == effect.player -> sim -> target_non_sleeping_list.end() )
        {
          debuff -> trigger();
          attack_speed -> trigger();
        }
      }
    }
  };

  new auto_cb_t( effect, buff );
}

struct heavens_nemesis_initializer_t : public item_targetdata_initializer_t
{
  heavens_nemesis_initializer_t() : item_targetdata_initializer_t( 394928 ) {}

  void operator()( actor_target_data_t* td ) const override
  {
    if ( !find_effect( td->source ) )
    {
      td->debuff.heavens_nemesis = make_buff( *td, "heavens_nemesis_mark" )->set_quiet( true );
      return;
    }

    assert( !td->debuff.heavens_nemesis );
    td->debuff.heavens_nemesis = make_buff( *td, "heavens_nemesis_mark", td->source->find_spell( 397478 ) );
    td->debuff.heavens_nemesis->reset();
  }
};

// Ashkandur
// 408790 Driver/Damage Value
// 408791 Damage effect
void ashkandur( special_effect_t& e )
{
  struct ashkandur_t : public generic_proc_t
  {
    ashkandur_t( const special_effect_t& e )
      : generic_proc_t( e, "ashkandur_fall_of_the_brotherhood", e.player->find_spell( 408791 ) )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = generic_proc_t::composite_target_multiplier( t );

      if ( ( player->sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE && player->target->race == RACE_HUMANOID ) ||
           player->dragonflight_opts.ashkandur_humanoid )
      {
        m *= 2; // Doubles damage against humanoid targets.
      }
      return m;
    }
  };

  e.execute_action = create_proc_action<ashkandur_t>( "ashkandur_fall_of_the_brotherhood", e );
  new dbc_proc_callback_t( e.player, e );
}

// Shadowed Razing Annihilator
// 408711 Driver & Main damage
// 411024 Residual AoE Damage
// TODO: Test the damage increase per target, early data suggested maybe its 30%, rather than the default 15%
void shadowed_razing_annihilator( special_effect_t& e )
{
  struct shadowed_razing_annihilator_residual_t : public generic_aoe_proc_t
  {
    shadowed_razing_annihilator_residual_t( const special_effect_t& e )
      : generic_aoe_proc_t( e, "shadowed_razing_annihilator_residual", e.player->find_spell( 411024 ), true )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 4 ).average( e.item );
    }
  };

  auto aoe_damage =
      create_proc_action<shadowed_razing_annihilator_residual_t>( "shadowed_razing_annihilator_residual", e );
  auto buff = create_buff<buff_t>( e.player, e.driver() )
                  ->set_quiet( true )
                  ->set_duration( 2_s )
                  ->set_stack_change_callback( [ aoe_damage ]( buff_t*, int, int new_ ) {
                    if ( !new_ )
                    {
                      aoe_damage->execute();
                    }
                  } );
  ;

  struct shadowed_razing_annihilator_t : public generic_proc_t
  {
    action_t* aoe;
    buff_t* buff;

    shadowed_razing_annihilator_t( const special_effect_t& e, buff_t* b )
      : generic_proc_t( e, "shadowed_razing_annihilator", e.driver() ),
        aoe( create_proc_action<shadowed_razing_annihilator_residual_t>( "shadowed_razing_annihilator_residual", e ) ),
        buff( b )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
      add_child( aoe );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      buff->trigger();
    }
  };

  e.execute_action = create_proc_action<shadowed_razing_annihilator_t>( "shadowed_razing_annihilator", e, buff );
}

// Djaruun, Pillar of the Elder Flame
// 408821 Driver
// 403545 Damage values
// 408835 Buff
// 408815 Siphon Damage effect
// 408836 AoE Damage per hit effect
void djaruun_pillar_of_the_elder_flame ( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "seething_rage" } ) )
    return;

  struct djaruun_pillar_of_the_elder_flame_siphon_t : public generic_aoe_proc_t
  {
    djaruun_pillar_of_the_elder_flame_siphon_t( const special_effect_t& effect )
      : generic_aoe_proc_t( effect, "djaruun_pillar_of_the_elder_flame_siphon", effect.player->find_spell( 408815 ) )
    {
      base_dd_min = base_dd_max = effect.player->find_spell( 403545 )->effectN( 1 ).average( effect.item );
      split_aoe_damage = false;
      name_str_reporting = "Siphon";
    }
  };

  //25/04/2023 - 10.1.0.49255
  //The tooltip doesn't state it increases damage per target, but it does
  struct djaruun_pillar_of_the_elder_flame_on_hit_t : public generic_aoe_proc_t
  {
    djaruun_pillar_of_the_elder_flame_on_hit_t( const special_effect_t& effect )
      : generic_aoe_proc_t( effect, "djaruun_pillar_of_the_elder_flame_on_hit", effect.player->find_spell( 408836 ), true )
    {
      base_dd_min = base_dd_max = effect.player->find_spell( 403545 ) -> effectN( 2 ).average( effect.item );
      name_str_reporting = "Wave";
    }
  };

  auto siphon_damage = create_proc_action<djaruun_pillar_of_the_elder_flame_siphon_t>( "djaruun_pillar_of_the_elder_flame_siphon", effect );
  auto on_hit = new special_effect_t( effect.player );
  on_hit->name_str = "djaruun_pillar_of_the_elder_flame_cb";
  on_hit->type = SPECIAL_EFFECT_EQUIP;
  on_hit->source = SPECIAL_EFFECT_SOURCE_ITEM;
  on_hit->spell_id = 408835;
  on_hit->execute_action = create_proc_action<djaruun_pillar_of_the_elder_flame_on_hit_t>( "djaruun_pillar_of_the_elder_flame_on_hit", effect );
  effect.player->special_effects.push_back( on_hit );

  auto on_hit_cb = new dbc_proc_callback_t( effect.player, *on_hit );
  on_hit_cb->initialize();
  on_hit_cb->deactivate();

  auto buff = create_buff<buff_t>( effect.player, effect.player -> find_spell( 408835 ) );
  buff->set_stack_change_callback( [ on_hit_cb ]( buff_t*, int old_, int new_ ) {
    if ( !old_ )
      on_hit_cb->activate();
    else if ( !new_ )
      on_hit_cb->deactivate();
  } );

  struct djaruun_of_the_elder_flame_t : public generic_proc_t
  {
    action_t* siphon;
    action_t* aoe;

    djaruun_of_the_elder_flame_t( const special_effect_t& effect, action_t *siphon )
      : generic_proc_t( effect, "elder_flame", effect.player->find_spell( 408821 ) ),
        siphon( create_proc_action<djaruun_pillar_of_the_elder_flame_siphon_t>( "djaruun_pillar_of_the_elder_flame_siphon", effect ) ),
        aoe( create_proc_action<djaruun_pillar_of_the_elder_flame_on_hit_t>( "djaruun_pillar_of_the_elder_flame_on_hit", effect ) )
    {
      add_child( aoe );
      add_child( siphon );
    }

    void execute( ) override
    {
      siphon->execute();
    }
  };

  effect.custom_buff = buff;

  effect.player->callbacks.register_callback_execute_function(
    effect.spell_id, [ buff ]( const dbc_proc_callback_t*, action_t*, action_state_t* ) { buff->trigger(); } );

  effect.execute_action = create_proc_action<djaruun_of_the_elder_flame_t>( "elder_flame", effect, siphon_damage );
}

// Armor
void assembly_guardians_ring( special_effect_t& effect )
{
  auto cb = new dbc_proc_callback_t( effect.player, effect );
  cb->initialize();

  cb->proc_buff->set_default_value( effect.driver()->effectN( 1 ).average( effect.item ) );
}

void assembly_scholars_loop( special_effect_t& effect )
{
  effect.discharge_amount = effect.driver()->effectN( 1 ).average( effect.item );

  new dbc_proc_callback_t( effect.player, effect );
}

void blue_silken_lining( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 387336 ) );
  buff->set_constant_behavior( buff_constant_behavior::NEVER_CONSTANT );
  bool first = !buff->manual_stats_added;
  // In some cases, the buff values from separate items don't stack. This seems to fix itself
  // when the player loses and regains the buff, so we just assume they stack properly.
  buff->add_stat( STAT_MASTERY_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

  // In case the player has two copies of this embellishment, set up the buff events only once.
  if ( first && buff->sim->dragonflight_opts.blue_silken_lining_uptime > 0.0 )
  {
    buff->player->register_combat_begin( [ buff ]( player_t* p ) {
      buff->trigger();
      make_repeating_event( *p->sim, p->sim->dragonflight_opts.blue_silken_lining_update_interval, [ buff, p ] {
        if ( p->rng().roll( p->sim->dragonflight_opts.blue_silken_lining_uptime ) )
          buff->trigger();
        else
          buff->expire();
      } );
    } );
  }
}

void breath_of_neltharion( special_effect_t& effect )
{
  struct breath_of_neltharion_t : public generic_proc_t
  {
    breath_of_neltharion_t( const special_effect_t& e ) : generic_proc_t( e, "breath_of_neltharion", e.driver() )
    {
      channeled = true;
      harmful = false;

      tick_action = create_proc_action<generic_aoe_proc_t>(
          "breath_of_neltharion_tick", e, "breath_of_neltharion_tick", e.trigger() );
      tick_action->split_aoe_damage = false;
      tick_action->stats = stats;
      tick_action->dual = true;
    }

    bool usable_moving() const override
    {
      return true;
    }

    void execute() override
    {
      generic_proc_t::execute();

      event_t::cancel( player->readying );
      player->reset_auto_attacks( composite_dot_duration( execute_state ) );
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;

      generic_proc_t::last_tick( d );

      if ( was_channeling && !player->readying )
        player->schedule_ready( rng().gauss( sim->channel_lag, sim->channel_lag_stddev ) );
    }
  };

  effect.execute_action = create_proc_action<breath_of_neltharion_t>( "breath_of_neltharion", effect );
}

void coated_in_slime( special_effect_t& effect )
{
  auto mul = toxified_mul( effect.player );

  effect.player->passive.add_stat( util::translate_rating_mod( effect.driver()->effectN( 2 ).misc_value1() ),
                                   effect.driver()->effectN( 2 ).average( effect.item ) * mul );

  effect.trigger_spell_id = effect.driver()->effectN( 3 ).trigger_spell_id();
  effect.discharge_amount = effect.driver()->effectN( 3 ).average( effect.item ) * mul;

  new dbc_proc_callback_t( effect.player, effect );
}

void elemental_lariat( special_effect_t& effect )
{
  enum gem_type_e
  {
    AIR_GEM   = 0x1,
    EARTH_GEM = 0x2,
    FIRE_GEM  = 0x4,
    FROST_GEM = 0x8
  };

  // while part of this info is available in ItemSparse.db2, seems unnecessary to export another field to item_data.inc
  // just for this single effect
  using gem_name_type = std::pair<const char*, gem_type_e>;
  static constexpr gem_name_type gem_types[] =
  {
    { "Crafty Alexstraszite",   AIR_GEM   },
    { "Energized Malygite",     AIR_GEM   },
    { "Quick Ysemerald",        AIR_GEM   },
    { "Keen Neltharite",        AIR_GEM   },
    { "Forceful Nozdorite",     AIR_GEM   },
    { "Sensei's Alexstraszite", EARTH_GEM },
    { "Zen Malygite",           EARTH_GEM },
    { "Keen Ysemerald",         EARTH_GEM },
    { "Fractured Neltharite",   EARTH_GEM },
    { "Puissant Nozdorite",     EARTH_GEM },
    { "Deadly Alexstraszite",   FIRE_GEM  },
    { "Radiant Malygite",       FIRE_GEM  },
    { "Crafty Ysemerald",       FIRE_GEM  },
    { "Sensei's Neltharite",    FIRE_GEM  },
    { "Jagged Nozdorite",       FIRE_GEM  },
    { "Radiant Alexstraszite",  FROST_GEM },
    { "Stormy Malygite",        FROST_GEM },
    { "Energized Ysemerald",    FROST_GEM },
    { "Zen Neltharite",         FROST_GEM },
    { "Steady Nozdorite",       FROST_GEM },
  };

  unsigned gem_mask = 0;
  unsigned gem_count = 0;
  for ( const auto& item : effect.player->items )
  {
    for ( auto gem_id : item.parsed.gem_id )
    {
      if ( gem_id )
      {
        auto n = effect.player->dbc->item( gem_id ).name;
        auto it = range::find( gem_types, n, &gem_name_type::first );
        if ( it != std::end( gem_types ) )
        {
          gem_mask |= ( *it ).second;
          gem_count++;
        }
      }
    }
  }

  if ( !gem_mask )
    return;

  auto val = effect.driver()->effectN( 1 ).average( effect.item );
  auto dur = timespan_t::from_seconds( effect.driver()->effectN( 3 ).base_value() +
                                       effect.driver()->effectN( 2 ).base_value() * gem_count );

  auto cb = new dbc_proc_callback_t( effect.player, effect );
  std::vector<buff_t*> buffs;

  auto add_buff = [ &effect, cb, gem_mask, val, dur, &buffs ]( gem_type_e type, std::string suf, unsigned id, stat_e stat ) {
    if ( gem_mask & type )
    {
      auto name = "elemental_lariat__empowered_" + suf;
      auto buff = buff_t::find( effect.player, name );
      if ( !buff )
      {
        buff = make_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( id ) )
          ->add_stat( stat, val )
          ->set_duration( dur )
          // TODO: confirm the driver is disabled while the buff is up. alternatively it could be:
          // 1) proc triggers but is ignore while buff is up
          // 2) proc does not trigger but builds blp while buff is up
          ->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
            if ( new_ )
              cb->deactivate();
            else
              cb->activate();
          } );
      }
      buffs.push_back( buff );
    }
  };

  add_buff( AIR_GEM, "air", 375342, STAT_HASTE_RATING );
  add_buff( EARTH_GEM, "earth", 375345, STAT_MASTERY_RATING );
  add_buff( FIRE_GEM, "flame", 375335, STAT_CRIT_RATING );
  add_buff( FROST_GEM, "frost", 375343, STAT_VERSATILITY_RATING );

  effect.player->callbacks.register_callback_execute_function(
      effect.driver()->id(), [ buffs ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
        buffs[ cb->rng().range( buffs.size() ) ]->trigger();
      } );
}

void flaring_cowl( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "flaring_cowl", effect, "flaring_cowl", 377079, true );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );

  auto period = effect.trigger()->effectN( 1 ).period();
  effect.player->register_combat_begin( [ period, damage ]( player_t* p ) {
    make_repeating_event( p->sim, period, [ damage ]() { damage->execute(); } );
  } );
}

void thriving_thorns( special_effect_t& effect )
{
  struct launched_thorns_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    action_t* heal;

    launched_thorns_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      auto mul = toxified_mul( e.player );

      e.player->passive.add_stat( STAT_STAMINA, e.driver()->effectN( 2 ).average( e.item ) * mul );

      auto damage_trg = e.player->find_spell( 379395 );  // velocity & missile reference for damage
      damage = create_proc_action<generic_proc_t>( "launched_thorns", e, "launched_thorns",
                                                   damage_trg->effectN( 1 ).trigger() );
      damage->travel_speed = damage_trg->missile_speed();
      damage->base_dd_min = damage->base_dd_max = e.driver()->effectN( 4 ).average( e.item ) * mul;

      auto heal_trg = e.player->find_spell( 379405 );  // velocity & missile refenence for heal
      heal = create_proc_action<base_generic_proc_t<proc_heal_t>>( "launched_thorns_heal", e, "launched_thorns_heal",
                                                                   heal_trg->effectN( 1 ).trigger() );
      heal->travel_speed = heal_trg->missile_speed();
      heal->base_dd_min = heal->base_dd_max = e.driver()->effectN( 3 ).average( e.item ) * mul;
      heal->name_str_reporting = "Heal";
    }

    void execute( action_t*, action_state_t* s ) override
    {
      if ( s->result_type == result_amount_type::HEAL_DIRECT || s->result_type == result_amount_type::HEAL_OVER_TIME )
        heal->execute_on_target( listener );
      else
        damage->execute_on_target( s->target );
    }
  };

  new launched_thorns_cb_t( effect );
}

// Broodkeepers Blaze
// 394452 Driver
// 394453 Damage & Buff
void broodkeepers_blaze(special_effect_t& effect)
{
  // callback to trigger debuff on specific schools
  struct broodkeepers_blaze_cb_t : public dbc_proc_callback_t
  {
    broodkeepers_blaze_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( dbc::is_school( a->get_school(), SCHOOL_FIRE ) )
      {
        dbc_proc_callback_t::trigger( a , s );
      }
    }
  };

  auto dot = create_proc_action<generic_proc_t>("broodkeepers_blaze", effect, "broodkeepers_blaze", 394453 );
  dot->base_td = effect.driver()->effectN(1).average(effect.item);
  effect.execute_action = dot;
  new broodkeepers_blaze_cb_t( effect );
}

void amice_of_the_blue( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_aoe_proc_t>( "amice_of_the_blue", effect, "amice_of_the_blue", effect.trigger() );
  damage->split_aoe_damage = true;
  damage->aoe = -1;
  effect.execute_action = damage;
  new dbc_proc_callback_t( effect.player, effect );
}

void rallied_to_victory( special_effect_t& effect )
{
  effect.custom_buff = create_buff<stat_buff_t>(effect.player, effect.trigger())
    ->add_stat(STAT_VERSATILITY_RATING, effect.driver()->effectN(1).average(effect.item));

  new dbc_proc_callback_t(effect.player, effect);
}

void deep_chill( special_effect_t& effect )
{
  effect.trigger_spell_id = 381006;
  effect.discharge_amount = effect.driver()->effectN(3).average(effect.item) * toxified_mul(effect.player);

  new dbc_proc_callback_t( effect.player, effect );
}

void potent_venom( special_effect_t& effect )
{
  double mult = toxified_mul(effect.player);
  double gain = effect.driver()->effectN(3).average(effect.item) * mult;
  double loss = std::abs(effect.driver()->effectN(2).average(effect.item)) * mult;

  struct potent_venom_t : public buff_t
  {
    stat_e gain = STAT_NONE;
    stat_e loss = STAT_NONE;

    potent_venom_t(player_t* p, util::string_view name, const spell_data_t* spell, const special_effect_t& effect) :
      buff_t(p, name, spell, effect.item)
    {
    }

    void reset() override
    {
      buff_t::reset();

      gain = STAT_NONE;
      loss = STAT_NONE;
    }
  };

  auto buff = create_buff<potent_venom_t>(effect.player, effect.driver()->effectN(3).trigger(), effect);
  buff->set_stack_change_callback( [effect, buff, gain, loss] ( buff_t*, int, int new_ )
    {
      static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };
      if ( new_ )
      {
        buff->gain = util::highest_stat(effect.player, ratings);
        buff->loss = util::lowest_stat(effect.player, ratings);

        buff->player->stat_gain(buff->gain, gain);
        buff->player->stat_loss(buff->loss, loss);
      }
      else
      {
        buff->player->stat_loss(buff->gain, gain);
        buff->player->stat_gain(buff->loss, loss);
      }
    } );
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// Allied Wristguards of Companionship
// 395959 Driver
// 395965 Buff
void allied_wristguards_of_companionship( special_effect_t& effect )
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger() );
  buff->add_stat( STAT_VERSATILITY_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

  timespan_t period = effect.driver()->effectN( 1 ).period();

  effect.player->register_combat_begin( [ buff, period ]( player_t* p ) {
    int allies = p->sim->dragonflight_opts.allied_wristguards_allies;

    buff->trigger( p->sim->rng().range( 1, allies ) );

    make_repeating_event( p->sim, period, [ buff, p, allies ]() {
      if ( p->rng().roll( p->sim->dragonflight_opts.allied_wristguards_ally_leave_chance ) )
      {
        buff->expire();
        buff->trigger( p->sim->rng().range( 1, allies ) );
      }
      else if ( buff->check() < allies )
      {
        buff->expire();
        buff->trigger( allies );
      }
      else if ( buff->check() == allies )
      {
        return;
      }
    } );
  } );
}

// Allied Chestplate of Generosity
// 378134 Driver
// 378139 Buff
// TODO: Potentially model allies recieving the buff as well?
void allied_chestplate_of_generosity(special_effect_t& effect)
{
  auto buff = create_buff<stat_buff_t>( effect.player, effect.trigger() );
  buff -> set_default_value( effect.driver() -> effectN( 1 ).average( effect.item ) );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

// 406219 Damage Taken Driver
// 406928 Crit Damage Driver
// 406927 Crit Stats
// 406921 Damage Taken Stats
void adaptive_dracothyst_armguards( special_effect_t& effect )
{
  // Handle both drivers, this is the Crit Driver.
  if ( effect.driver()->id() == 406928 )
  {
    auto buff = create_buff<stat_buff_t>( effect.player, "adaptive_stonescales_crit", effect.driver()->effectN( 1 ).trigger() );
    buff->add_stat_from_effect( 1, effect.driver()->effectN( 2 ).average( effect.item ) );
    effect.custom_buff  = buff;
    effect.name_str     = "adaptive_stonescales_crit";
    effect.proc_flags2_ = PF2_CRIT;
    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    auto buff = create_buff<stat_buff_t>( effect.player, "adaptive_stonescales_dmgtaken",
                                          effect.driver()->effectN( 1 ).trigger() );
    buff->add_stat_from_effect( 1, effect.driver()->effectN( 2 ).average( effect.item ) );
    effect.custom_buff = buff;
    effect.name_str    = "adaptive_stonescales_dmgtaken";
    auto dbc = new dbc_proc_callback_t( effect.player, effect );

    // Fake damage taken events to trigger this for DPS Players
    effect.player->register_combat_begin( [ dbc, buff ]( player_t* p ) {
      if ( dbc->rppm && p->sim->dragonflight_opts.adaptive_stonescales_period > 0_s )
      {
        make_repeating_event( p->sim, p->sim->dragonflight_opts.adaptive_stonescales_period, [ dbc, buff ]() {
          // Roll RPPM from the DBC Object so it will still work on sims with damage taken events.
          if ( dbc->rppm && dbc->rppm->trigger() )
          {
            buff->trigger();
          }
        } );
      }
    } );
  }
}

// 395601 Driver
// 395603 Buff
void hood_of_surging_time( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "prepared_time" } ) )
    return;

  struct hood_of_surging_time_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    hood_of_surging_time_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), target_list()
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( range::contains( target_list, s->target->actor_spawn_index ) )
        return;

      dbc_proc_callback_t::execute( a, s );
      target_list.push_back( s->target->actor_spawn_index );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      target_list.clear();
    }
  };

  auto buff = buff_t::find( effect.player, "prepared_time" );
  if ( !buff )
  {
    buff =
        create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 395603 ) )
            ->add_stat( STAT_HASTE_RATING, effect.player->find_spell( 395603 )->effectN( 1 ).average( effect.item ) );
  }

  effect.custom_buff = buff;

  // TODO: Check if this procs off of periodic, First Strike did not
  effect.proc_flags_ = effect.proc_flags() & ~PF_PERIODIC;

  // The effect procs when damage actually happens.
  effect.proc_flags2_ = PF2_ALL_HIT;

  new hood_of_surging_time_cb_t( effect );

  // Create repeating combat even to easier simulate increased uptime on encounters where you can get first strike procs
  // while contuining your single target rotation
  effect.player->register_combat_begin( [ buff ]( player_t* ) {
    if ( buff->sim->dragonflight_opts.hood_of_surging_time_chance > 0.0 &&
         buff->sim->dragonflight_opts.hood_of_surging_time_period > 0_s &&
         buff->sim->dragonflight_opts.hood_of_surging_time_stacks > 0 )
    {
      make_repeating_event( buff->sim, buff->sim->dragonflight_opts.hood_of_surging_time_period, [ buff ] {
        if ( buff->rng().roll( buff->sim->dragonflight_opts.hood_of_surging_time_chance ) )
          buff->trigger( buff->sim->dragonflight_opts.hood_of_surging_time_stacks );
      } );
    }
  } );
}
// Voice of the Silent Star
// 409434 Driver
// 409447 Stat Buff
// 409442 Stacking Buff
// TODO - Potentially better proc emulation?
void voice_of_the_silent_star( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs(
           effect, { "power_beyond_imagination_crit_rating", "power_beyond_imagination_mastery_rating",
                     "power_beyond_imagination_haste_rating", "power_beyond_imagination_versatility_rating" } ) )
    return;

  static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING,
                                                     STAT_CRIT_RATING };

  auto buffs = std::make_shared<std::map<stat_e, buff_t*>>();
  double amount = effect.driver()->effectN( 1 ).average( effect.item ) +
                  ( effect.driver()->effectN( 3 ).average( effect.item ) * effect.driver()->effectN( 4 ).base_value() );

  for ( auto stat : ratings )
  {
    auto name = std::string( "power_beyond_imagination_" ) + util::stat_type_string( stat );
    auto buff = create_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( 409447 ), effect.item )
      ->set_stat( stat, amount )
      ->set_name_reporting( util::stat_type_abbrev( stat ) );

    ( *buffs )[ stat ] = buff;
  }

  auto stack_buff = create_buff<buff_t>( effect.player, "the_voice_beckons", effect.player->find_spell( 409442 ) )
    ->set_expire_at_max_stack( true )
    ->set_stack_change_callback( [ buffs, effect ]( buff_t*, int, int new_ ) {
      if ( !new_ )
      {
        stat_e max_stat = util::highest_stat( effect.player, ratings );
        ( *buffs )[ max_stat ]->trigger();
      }
    } );

  effect.custom_buff = stack_buff;
  // Overriding Proc Flags as sims dont usually take damage to proc this.
  effect.proc_flags_ = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_ALL_HIT;

  if( effect.player -> dragonflight_opts.voice_of_the_silent_star_enable )
  {
    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    effect.type = SPECIAL_EFFECT_NONE;
  }
}

// Shadowflame-Tempered Armor Patch
// 406254 Driver / Value Container
// 412547 RPPM Data
// 406251 Damage
// 406887 Buff
void roiling_shadowflame( special_effect_t& e )
{
  auto stack_buff = create_buff<buff_t>( e.player, "roused_shadowflame", e.player->find_spell( 406887 ) )
                        ->set_default_value( e.player->find_spell( 406254 )->effectN( 4 ).percent() );

  struct roiling_shadowflame_t : public generic_proc_t
  {
    buff_t* buff;

    roiling_shadowflame_t( const special_effect_t& e, buff_t* b )
      : generic_proc_t( e, "roiling_shadowflame", e.player->find_spell( 406251 ) ), buff( b )
    {
    }

    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = generic_proc_t::composite_da_multiplier( state );

      m *= 1.0 + buff->check_stack_value();

      return m;
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );
      if ( buff->at_max_stacks() )
      {
        buff->expire();
      }
      else
      {
        buff->trigger();
      }
    }

  };

  auto new_driver_id = 412547;  // Rppm data moved out of the main driver into this spell
  e.spell_id         = new_driver_id;

  auto damage = create_proc_action<roiling_shadowflame_t>( "roiling_shadowflame", e, stack_buff );

  damage->base_dd_min += e.player->find_spell( 406254 )->effectN( 2 ).average( e.item );
  damage->base_dd_max += e.player->find_spell( 406254 )->effectN( 2 ).average( e.item );

  e.execute_action = damage;
  // Forcing name to share proc and rppm, as otherwise putting it on a shield will give a _oh suffix
  e.name_str = "roiling_shadowflame";

  new dbc_proc_callback_t( e.player, e );

  e.player->register_on_combat_state_callback( [ stack_buff ]( player_t*, bool c ) {
    if ( !c )
      stack_buff->expire();
  } );
}

// Stacking debuff: 407087
// Damage debuff: 407090
void ever_decaying_spores( special_effect_t& effect )
{
  struct ever_decaying_spores_damage_t : generic_proc_t
  {
    ever_decaying_spores_damage_t( const special_effect_t& effect )
      : generic_proc_t( effect, "everdecaying_spores", effect.player->find_spell( 407090 ) )
    {
      base_td = effect.driver()->effectN( 2 ).average( effect.item ) * toxified_mul( effect.player );
    }
  };

  struct ever_decaying_spores_cb_t : dbc_proc_callback_t
  {
    action_t* damage;

    ever_decaying_spores_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.player, effect ),
        damage( create_proc_action<ever_decaying_spores_damage_t>( "ever_decaying_spores", effect ) )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      actor_target_data_t* td = a->player->get_target_data( s->target );

      if ( !td->debuff.ever_decaying_spores->stack_change_callback )
      {
        td->debuff.ever_decaying_spores->set_stack_change_callback( [ this ]( buff_t* b, int /* old_ */, int new_ ) {
          if ( new_ == 0 )
          {
            damage->set_target( b->player );
            damage->execute();
          }
        } );
      }

      // Every time this procs it has a chance to damage the player instead of applying a debuff stack and it will also eat the proc
      // if the debuff is ticking on the target.
      // Testing shows this chance is currently 20% but since it's not found in spell data will have to rechecked in case this changes.
      if ( rng().roll( 0.8 ) )
      {
        td->debuff.ever_decaying_spores->trigger();
        if ( td->debuff.ever_decaying_spores->at_max_stacks() )
          td->debuff.ever_decaying_spores->expire();
      }
    }


    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( !damage->get_dot( s->target )->is_ticking() )
        dbc_proc_callback_t::trigger( a, s );
    }
  };

  new ever_decaying_spores_cb_t( effect );
}

struct ever_decaying_spores_initializer_t : public item_targetdata_initializer_t
{
  ever_decaying_spores_initializer_t() : item_targetdata_initializer_t( 406244 )
  {
  }

  void operator()( actor_target_data_t* td ) const override
  {
    if ( !find_effect( td->source ) )
    {
      td->debuff.ever_decaying_spores = make_buff( *td, "ever_decaying_spores" )->set_quiet( true );
      return;
    }

    assert( !td->debuff.ever_decaying_spores );
    td->debuff.ever_decaying_spores = make_buff( *td, "ever_decaying_spores", td->source->find_spell( 407087 ) );
    td->debuff.ever_decaying_spores->reset();
  }
};

}  // namespace items

namespace sets
{
void playful_spirits_fur( special_effect_t& effect )
{
  if ( !effect.player->sets->has_set_bonus( effect.player->specialization(), T29_PLAYFUL_SPIRITS_FUR, B2 ) )
    return;

  auto set_driver_id = effect.player->sets->set( effect.player->specialization(), T29_PLAYFUL_SPIRITS_FUR, B2 )->id();

  if ( effect.driver()->id() == set_driver_id )
  {
    effect.trigger_spell_id = 376932;

    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    unique_gear::find_special_effect( effect.player, set_driver_id )->discharge_amount +=
        effect.driver()->effectN( 1 ).average( effect.item );
  }
}

void horizon_striders_garments( special_effect_t& effect )
{
  if ( !effect.player->sets->has_set_bonus( effect.player->specialization(), T29_HORIZON_STRIDERS_GARMENTS, B2 ) )
    return;

  auto set_driver_id =
      effect.player->sets->set( effect.player->specialization(), T29_HORIZON_STRIDERS_GARMENTS, B2 )->id();

  if ( effect.driver()->id() == set_driver_id )
  {
    effect.proc_flags2_ = PF2_CRIT;

    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 377143 ) )
                    ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );
    auto driver = unique_gear::find_special_effect( effect.player, set_driver_id );
    driver->custom_buff = buff;
  }
}

// Damage buff - 388061
// TODO: Healing buff - 388064
void azureweave_vestments( special_effect_t& effect )
{
  if ( !effect.player->sets->has_set_bonus( effect.player->specialization(), T29_AZUREWEAVE_VESTMENTS, B2 ) )
    return;

  auto set_driver_id = effect.player->sets->set( effect.player->specialization(), T29_AZUREWEAVE_VESTMENTS, B2 )->id();

  if ( effect.driver()->id() == set_driver_id )
  {
    // effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;

    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 388061 ) )
                ->add_stat( STAT_INTELLECT, effect.driver()->effectN( 1 ).average( effect.item ) );
    auto driver         = unique_gear::find_special_effect( effect.player, set_driver_id );
    driver->custom_buff = buff;
  }
}

// building stacks - 387141 (moment_of_time)
// haste - 387142 (unleashed_time)
void woven_chronocloth( special_effect_t& effect )
{
  if ( !effect.player->sets->has_set_bonus( effect.player->specialization(), T29_WOVEN_CHRONOCLOTH, B2 ) )
    return;

  auto set_driver_id = effect.player->sets->set( effect.player->specialization(), T29_WOVEN_CHRONOCLOTH, B2 )->id();

  if ( effect.driver()->id() == set_driver_id )
  {
    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    auto haste_buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 387142 ) )
                       ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

    auto buff = buff_t::find( effect.player, effect.name() );
    if ( !buff )
    {
      buff = create_buff<buff_t>( effect.player, effect.player->find_spell( 387141 ) )
                 ->set_expire_at_max_stack( true )
                 ->set_stack_change_callback( [ haste_buff ]( buff_t* b, int, int /*new_*/ ) {
                   if ( b->at_max_stacks() )
                   {
                     haste_buff->trigger();
                   }
                 } );
    }
    auto driver         = unique_gear::find_special_effect( effect.player, set_driver_id );
    driver->custom_buff = buff;
  }
}

void raging_tempests( special_effect_t& effect )
{
  auto check_set = [ effect ]( set_bonus_e b ) {
    return effect.player->sets->has_set_bonus( effect.player->specialization(), T29_RAGING_TEMPESTS, b ) &&
           effect.player->sets->set( effect.player->specialization(), T29_RAGING_TEMPESTS, b )->id() ==
               effect.driver()->id();
  };

  if ( check_set( B2 ) )
  {
    static constexpr std::array<stat_e, 4> ratings =
        { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };

    auto buff = create_buff<stat_buff_t>( effect.player, effect.driver() );
    buff->set_constant_behavior( buff_constant_behavior::ALWAYS_CONSTANT );

    auto val = effect.driver()->effectN( 1 ).average( effect.player );

    // the amount is set when you eqiup the item and does not dynamically update. To account for any shenanigans with
    // temporary buffs during equip, instead of implementing as a passive stat bonus we create a buff to trigger on
    // combat start, accounting for anything in the precombat apl.
    effect.player->register_combat_begin( [ buff, effect, val ]( player_t* p ) {
      buff->set_stat( util::highest_stat( p, ratings ), val );
      buff->trigger();
    } );
  }
  else if ( check_set( B4 ) )
  {
    auto counter = create_buff<buff_t>( effect.player, effect.player->find_spell( 390529 ) )
      ->set_expire_at_max_stack( true );

    auto driver = new special_effect_t( effect.player );
    driver->type = SPECIAL_EFFECT_EQUIP;
    driver->source = SPECIAL_EFFECT_SOURCE_ITEM;
    driver->spell_id = 390579;
    driver->duration_ = 0_ms;
    driver->execute_action =
        create_proc_action<generic_proc_t>( "spark_of_the_primals", *driver, "spark_of_the_primals", 392038 );
    effect.player->special_effects.push_back( driver );

    auto cb = new dbc_proc_callback_t( effect.player, *driver );
    cb->initialize();
    cb->deactivate();

    counter->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ) {
      if ( !new_ )
        cb->activate();
    } );

    effect.player->callbacks.register_callback_execute_function(
        driver->spell_id, [ cb ]( const dbc_proc_callback_t*, action_t* a, action_state_t* ) {
          cb->proc_action->execute_on_target( a->target );
          cb->deactivate();
        } );

    effect.player->register_on_kill_callback( [ counter ]( player_t* ) {
      counter->trigger();
    } );
  }
}

//407914 set driver
//407913 primary stat buff
//407939 primary stat value
void might_of_the_drogbar( special_effect_t& effect )
{
  if ( !effect.player->sets->has_set_bonus( effect.player->specialization(), T30_MIGHT_OF_THE_DROGBAR, B2 ) )
    return;

  auto set_driver_id = effect.player->sets->set( effect.player->specialization(), T30_MIGHT_OF_THE_DROGBAR, B2 )->id();

  if ( effect.driver()->id() == set_driver_id )
  {
    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    auto buff = create_buff<stat_buff_t>( effect.player, effect.player->find_spell( 407913 ) )
      ->add_stat ( STAT_STR_AGI_INT, effect.driver()->effectN( 1 ).average( effect.item ) );
    auto driver = unique_gear::find_special_effect( effect.player, set_driver_id );
    driver->custom_buff = buff;
  }
}
}  // namespace sets

namespace primordial_stones
{
enum primordial_stone_drivers_e
{
  WIND_SCULPTED_STONE      = 401678, // NYI (buffs speed)
  STORM_INFUSED_STONE      = 402928,
  ECHOING_THUNDER_STONE    = 402929,
  FLAME_LICKED_STONE       = 402930,
  RAGING_MAGMA_STONE       = 402931, // NYI (requires getting hit, damage)
  SEARING_SMOKEY_STONE     = 402932,
  ENTROPIC_FEL_STONE       = 402934,
  INDOMITABLE_EARTH_STONE  = 402935,
  SHINING_OBSIDIAN_STONE   = 402936,
  PRODIGIOUS_SAND_STONE    = 402937, // NYI (driver does not exist)
  GLEAMING_IRON_STONE      = 402938,
  DELUGING_WATER_STONE     = 402939, // NYI (heal)
  FREEZING_ICE_STONE       = 402940,
  COLD_FROST_STONE         = 402941,
  EXUDING_STEAM_STONE      = 402942, // NYI (procs on receiving heals)
  SPARKLING_MANA_STONE     = 402943, // NYI (restores mana)
  SWIRLING_MOJO_STONE      = 402944, // NYI (requires creature deaths, gives the player an item to activate its buff)
  HUMMING_ARCANE_STONE     = 402947,
  HARMONIC_MUSIC_STONE     = 402948, // NYI (buffs tertiary stats)
  WILD_SPIRIT_STONE        = 402949, // NYI (heal)
  NECROMANTIC_DEATH_STONE  = 402951, // NYI
  PESTILENT_PLAGUE_STONE   = 402952,
  OBSCURE_PASTEL_STONE     = 402955,
  DESIROUS_BLOOD_STONE     = 402957,
  PROPHETIC_TWILIGHT_STONE = 402959,
};

enum primordial_stone_family_e
{
  PRIMORDIAL_NONE,
  PRIMORDIAL_ARCANE,
  PRIMORDIAL_EARTH,
  PRIMORDIAL_FIRE,
  PRIMORDIAL_FROST,
  PRIMORDIAL_NATURE,
  PRIMORDIAL_NECROMANTIC,
  PRIMORDIAL_SHADOW,
};

primordial_stone_family_e get_stone_family( const special_effect_t& e )
{
  switch ( e.driver()->id() )
  {
    case SPARKLING_MANA_STONE:
    case HUMMING_ARCANE_STONE:
    case HARMONIC_MUSIC_STONE:
    case OBSCURE_PASTEL_STONE:
      return PRIMORDIAL_ARCANE;
    case INDOMITABLE_EARTH_STONE:
    case SHINING_OBSIDIAN_STONE:
    case GLEAMING_IRON_STONE:
      return PRIMORDIAL_EARTH;
    case FLAME_LICKED_STONE:
    case RAGING_MAGMA_STONE:
    case SEARING_SMOKEY_STONE:
    case ENTROPIC_FEL_STONE:
      return PRIMORDIAL_FIRE;
    case DELUGING_WATER_STONE:
    case FREEZING_ICE_STONE:
    case COLD_FROST_STONE:
    case EXUDING_STEAM_STONE:
      return PRIMORDIAL_FROST;
    case WIND_SCULPTED_STONE:
    case STORM_INFUSED_STONE:
    case ECHOING_THUNDER_STONE:
    case 403170: // The Echoing Thunder Stone effect will have this driver after it is initialized.
    case WILD_SPIRIT_STONE:
    case PESTILENT_PLAGUE_STONE:
      return PRIMORDIAL_NATURE;
    case NECROMANTIC_DEATH_STONE:
    case DESIROUS_BLOOD_STONE:
      return PRIMORDIAL_NECROMANTIC;
    case SWIRLING_MOJO_STONE:
    case PROPHETIC_TWILIGHT_STONE:
      return PRIMORDIAL_SHADOW;
    default:
      break;
  }

  return PRIMORDIAL_NONE;
}

enum primordial_stone_type_e
{
  PRIMORDIAL_TYPE_NONE = 0,
  PRIMORDIAL_TYPE_DAMAGE,
  PRIMORDIAL_TYPE_HEAL,
  PRIMORDIAL_TYPE_ABSORB
};

primordial_stone_type_e get_stone_type( const special_effect_t& e )
{
  switch ( e.driver()->id() )
  {
    case STORM_INFUSED_STONE:
    case ECHOING_THUNDER_STONE:
    case 403170: // The Echoing Thunder Stone effect will have this driver after it is initialized.
    case FLAME_LICKED_STONE:
    case SHINING_OBSIDIAN_STONE:
    case FREEZING_ICE_STONE:
    case HUMMING_ARCANE_STONE:
    case PESTILENT_PLAGUE_STONE:
    case SEARING_SMOKEY_STONE:
      return PRIMORDIAL_TYPE_DAMAGE;
    case DESIROUS_BLOOD_STONE:
      return PRIMORDIAL_TYPE_HEAL;
    default:
      break;
  }

  return PRIMORDIAL_TYPE_NONE;
}

action_t* find_primordial_stone_action( player_t* player, unsigned driver )
{
  switch ( driver )
  {
    // damage stones
    case STORM_INFUSED_STONE:
      return player->find_action( "storm_infused_stone" );
    case ECHOING_THUNDER_STONE:
    case 403170: // The Echoing Thunder Stone effect will have this driver after it is initialized.
      return player->find_action( "uncontainable_charge" );
    case FLAME_LICKED_STONE:
      return player->find_action( "flame_licked_stone" );
    case SHINING_OBSIDIAN_STONE:
      return player->find_action( "shining_obsidian_stone" );
    case FREEZING_ICE_STONE:
      return player->find_action( "freezing_ice_stone" );
    case HUMMING_ARCANE_STONE:
      return player->find_action( "humming_arcane_stone" );
    case PESTILENT_PLAGUE_STONE:
      return player->find_action( "pestilent_plague_stone" );
    case DESIROUS_BLOOD_STONE:
      return player->find_action( "desirous_blood_stone" );
    case SEARING_SMOKEY_STONE:
      return player->find_action( "searing_smokey_stone" );

    // healing stones
    case DELUGING_WATER_STONE:
      return player->find_action( "deluging_water_stone" );
    case WILD_SPIRIT_STONE:
      return player->find_action( "wild_spirit_stone" );
    case EXUDING_STEAM_STONE:
      return player->find_action( "exuding_steam_stone" );

    // absorb stones
    case COLD_FROST_STONE:
      return player->find_action( "cold_frost_stone" );
    case INDOMITABLE_EARTH_STONE:
      return player->find_action( "indomitable_earth_stone" );

    // other
    case HARMONIC_MUSIC_STONE:
      return nullptr;

    default:
      break;
  }

  return nullptr;
}

template <typename BASE>
struct damage_stone_base_t : public BASE
{
  double entropic_fel_stone_multiplier;

  template<typename... ARGS>
  damage_stone_base_t( const special_effect_t& effect, ARGS&&... args  ) :
    BASE( effect, std::forward<ARGS>( args )... ),
    entropic_fel_stone_multiplier()
  {
    // If Entropic Fel Stone is equipped, Fire damage stones are changed to Chaos.
    if ( ( dbc::get_school_mask( BASE::get_school() ) & dbc::get_school_mask( SCHOOL_FIRE ) ) != 0)
    {
      if ( auto e = find_special_effect( effect.player, ENTROPIC_FEL_STONE ) )
      {
        entropic_fel_stone_multiplier = e->driver()->effectN( 1 ).percent();
        BASE::set_school( SCHOOL_CHAOS );
      }
    }
  }

  double action_multiplier() const override
  {
    double m = BASE::action_multiplier();

    m *= 1.0 + entropic_fel_stone_multiplier;

    return m;
  }
};

using damage_stone_t = damage_stone_base_t<generic_proc_t>;
using aoe_damage_stone_t = damage_stone_base_t<generic_aoe_proc_t>;

struct heal_stone_t : public proc_heal_t
{
  heal_stone_t( const special_effect_t& effect, std::string_view name, const spell_data_t* spell ) :
    proc_heal_t( name, effect.player, spell )
  {}

  heal_stone_t( const special_effect_t& effect, std::string_view name, unsigned spell_id ) :
    heal_stone_t( effect, name, effect.player->find_spell( spell_id ) )
  {}
};

struct absorb_stone_t : public absorb_t
{
  buff_t* buff;
  action_t* shining_obsidian_stone;

  absorb_stone_t( std::string_view n, const special_effect_t& e, const spell_data_t* s, buff_t* b ) :
    absorb_t( n, e.player, s ), buff( b ), shining_obsidian_stone()
  {
    background = true;
    base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
  }

  void init_finished() override
  {
    absorb_t::init_finished();

    if ( find_special_effect( player, SHINING_OBSIDIAN_STONE ) )
      shining_obsidian_stone = find_primordial_stone_action( player, SHINING_OBSIDIAN_STONE );
  }

  absorb_buff_t* create_buff( const action_state_t* ) override
  {
    return debug_cast<absorb_buff_t*>( buff );
  }

  void execute() override
  {
    absorb_t::execute();

    if ( shining_obsidian_stone )
    {
      shining_obsidian_stone->execute_on_target( player->target );
    }
  }
};

struct primordial_stone_cb_t : public dbc_proc_callback_t
{
  action_t* twilight_action = nullptr;

  primordial_stone_cb_t( player_t* p, const special_effect_t& e ) : dbc_proc_callback_t( p, e ) {}

  void initialize() override
  {
    dbc_proc_callback_t::initialize();

    // if the current callback has an action and we find a prophetic twilight stone
    if ( !proc_action || !find_special_effect( listener, PROPHETIC_TWILIGHT_STONE ) )
      return;

    auto type = get_stone_type( effect );

    // depending on the type of the current callback's effect, find the opposite stone type
    if ( type == PRIMORDIAL_TYPE_HEAL )
    {
      for ( auto e : effect.item->parsed.special_effects )
      {
        if ( get_stone_type( *e ) == PRIMORDIAL_TYPE_DAMAGE )
        {
          twilight_action = find_primordial_stone_action( e->player, e->driver()->id() );
          break;
        }
      }
    }
    else if ( type == PRIMORDIAL_TYPE_DAMAGE )
    {
      for ( auto e : effect.item->parsed.special_effects )
      {
        if ( get_stone_type( *e ) == PRIMORDIAL_TYPE_HEAL )
        {
          twilight_action = find_primordial_stone_action( e->player, e->driver()->id() );
          break;
        }
      }
    }
  }

  void execute( action_t* a, action_state_t* s ) override
  {
    dbc_proc_callback_t::execute( a, s );

    if ( twilight_action )
      twilight_action->execute_on_target( s->target );
  }
};

// Echoing Thunder Stone
struct uncontainable_charge_t : public damage_stone_t
{
  uncontainable_charge_t( const special_effect_t& e ) :
    damage_stone_t( e, "uncontainable_charge", 403171 )
  {
    auto driver = e.player->find_spell( ECHOING_THUNDER_STONE );
    base_dd_min = base_dd_max = driver->effectN( 1 ).average( e.item );
  }
};

struct flame_licked_stone_t : public damage_stone_t
{
  flame_licked_stone_t( const special_effect_t& e ) :
    damage_stone_t( e, "flame_licked_stone", 403225 )
  {
    auto driver = e.player->find_spell( FLAME_LICKED_STONE );
    base_td = driver->effectN( 1 ).average( e.item );
  }
};

struct freezing_ice_stone_t : public damage_stone_t
{
  freezing_ice_stone_t( const special_effect_t& e ) :
    damage_stone_t( e, "freezing_ice_stone", 403391 )
  {
    auto driver = e.player->find_spell( FREEZING_ICE_STONE );
    base_dd_min = base_dd_max = driver->effectN( 1 ).average( e.item );
  }
};

struct storm_infused_stone_t : public damage_stone_t
{
  storm_infused_stone_t( const special_effect_t& e ) :
    damage_stone_t( e, "storm_infused_stone", 403087 )
  {
    auto driver = e.player->find_spell( STORM_INFUSED_STONE );
    base_dd_min = base_dd_max = driver->effectN( 1 ).average( e.item );
  }
};

struct searing_smokey_stone_t : public damage_stone_t
{
  searing_smokey_stone_t( const special_effect_t &e ) :
    damage_stone_t( e, "searing_smokey_stone", 403257 )
  {
    auto driver = e.player->find_spell( SEARING_SMOKEY_STONE );
    base_dd_min = base_dd_max = driver->effectN( 1 ).average( e.item );
  }
};

// TODO: Healing?
struct desirous_blood_stone_t : public damage_stone_t
{
  desirous_blood_stone_t( const special_effect_t& e ) :
    damage_stone_t( e, "desirous_blood_stone", 404911 )
  {
    auto driver = e.player->find_spell( DESIROUS_BLOOD_STONE );
    base_dd_min = base_dd_max = driver->effectN( 1 ).average( e.item );
  }
};

// TODO: Is this damage fully uncapped?
struct pestilent_plague_stone_aoe_t : public generic_aoe_proc_t
{
  pestilent_plague_stone_aoe_t( const special_effect_t& e ) :
    generic_aoe_proc_t( e, "pestilent_plague_stone_aoe", 405221 )
  {
    auto driver = e.player->find_spell( PESTILENT_PLAGUE_STONE );
    base_td = driver->effectN( 1 ).average( e.item );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    generic_aoe_proc_t::available_targets( tl );

    // Remove the main target, this only hits everything else in range.
    tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* t ) { return t == this->target; } ), tl.end() );

    return tl.size();
  }
};

struct pestilent_plague_stone_t : public damage_stone_t
{
  timespan_t aoe_delay;
  action_t* aoe_damage;

  pestilent_plague_stone_t( const special_effect_t& e ) :
    damage_stone_t( e, "pestilent_plague_stone", 405220 )
  {
    auto driver = e.player->find_spell( PESTILENT_PLAGUE_STONE );
    base_td = driver->effectN( 1 ).average( e.item );
    aoe_delay = timespan_t::from_millis( data().effectN( 3 ).misc_value1() );
    aoe_damage = create_proc_action<pestilent_plague_stone_aoe_t>( "pestilent_plague_stone_aoe", e );
    add_child( aoe_damage );
  }

  void impact( action_state_t* s ) override
  {
    damage_stone_t::impact( s );

    if ( result_is_hit( s->result ) && aoe_damage )
    {
      make_event( *sim, aoe_delay, [ t = s->target, this ] { aoe_damage->execute_on_target( t ); } );
    }
  }
};

struct humming_arcane_stone_damage_t : public generic_proc_t
{
  humming_arcane_stone_damage_t( const special_effect_t& e ) :
    generic_proc_t( e, "humming_arcane_stone_damage", 405209 )
  {
    auto driver = e.player->find_spell( HUMMING_ARCANE_STONE );
    base_dd_min = base_dd_max = driver->effectN( 1 ).average( e.item );
  }
};

struct humming_arcane_stone_t : public damage_stone_t
{
  size_t num_families;
  action_t* damage;

  humming_arcane_stone_t( const special_effect_t& e ) :
    damage_stone_t( e, "humming_arcane_stone", 405206 )
  {
    damage = create_proc_action<humming_arcane_stone_damage_t>( "humming_arcane_stone_damage", e );
    damage->name_str_reporting = "Missile";
    add_child( damage );

    std::set<primordial_stone_family_e> stone_families;
    for ( const auto se : e.item->parsed.special_effects )
    {
      if ( se->source == SPECIAL_EFFECT_SOURCE_GEM && se->driver()->affected_by_label( LABEL_PRIMORDIAL_STONE ) )
      {
        auto family = get_stone_family( *se );
        if ( family != PRIMORDIAL_NONE )
        {
          stone_families.insert( family );
        }
      }
    }

    num_families = stone_families.size();
  }

  void execute() override
  {
    damage_stone_t::execute();

    for ( size_t i = 0; i < num_families; i++ )
      damage->execute_on_target( target );
  }
};

struct shining_obsidian_stone_t : public aoe_damage_stone_t
{
  shining_obsidian_stone_t( const special_effect_t& e ) :
    aoe_damage_stone_t( e, "shining_obsidian_stone", 404941, true )
  {
    auto driver = e.player->find_spell( SHINING_OBSIDIAN_STONE );
    base_dd_min = base_dd_max = driver->effectN( 1 ).average( e.item );
  }
};

struct cold_frost_stone_t : public absorb_stone_t
{
  cold_frost_stone_t( const special_effect_t& e )
    : absorb_stone_t( "cold_frost_stone", e, e.trigger(),
                      unique_gear::create_buff<absorb_buff_t>( e.player, e.trigger(), e.item ) )
  {}
};

struct indomitable_earth_stone_t : public absorb_stone_t
{
  indomitable_earth_stone_t( const special_effect_t& e )
    : absorb_stone_t( "indomitable_earth_stone", e, e.trigger(),
                      unique_gear::create_buff<absorb_buff_t>( e.player, e.trigger(), e.item ) )
  {}
};

struct gleaming_iron_stone_t : public absorb_stone_t
{
  buff_t* aa_buff;

  gleaming_iron_stone_t( const special_effect_t& e )
    : absorb_stone_t( "gleaming_iron_stone", e, e.trigger(),
                      unique_gear::create_buff<absorb_buff_t>( e.player, e.player->find_spell( 403376 ), e.item ) )
  {
    aa_buff = unique_gear::create_buff<buff_t>( player, "gleaming_iron_stone_autoattack", player->find_spell( 405001 ) )
      ->set_default_value( e.driver()->effectN( 2 ).average( e.item ) )
      ->set_stack_change_callback( [ this ]( buff_t* b, int, int new_ ) {
        if ( new_ )
        {
          player->auto_attack_modifier += b->default_value;
        }
        else
        {
          player->auto_attack_modifier -= b->default_value;
          make_event( b->sim, 3_s, [ this ] { execute(); });
        }
      } );
  }

  void execute() override
  {
    absorb_stone_t::execute();

    aa_buff->trigger();
  }
};

action_t* create_primordial_stone_action( const special_effect_t& effect, primordial_stone_drivers_e driver )
{
  action_t* action = find_primordial_stone_action( effect.player, driver );
  if ( action )
  {
    return action;
  }

  switch ( driver )
  {
    // damage stones
    case STORM_INFUSED_STONE:
      return new storm_infused_stone_t( effect );
    case ECHOING_THUNDER_STONE:
      return new uncontainable_charge_t( effect );
    case FLAME_LICKED_STONE:
      return new flame_licked_stone_t( effect );
    case SHINING_OBSIDIAN_STONE:
      return new shining_obsidian_stone_t( effect );
    case FREEZING_ICE_STONE:
      return new freezing_ice_stone_t( effect );
    case HUMMING_ARCANE_STONE:
      return new humming_arcane_stone_t( effect );
    case PESTILENT_PLAGUE_STONE:
      return new pestilent_plague_stone_t( effect );
    case DESIROUS_BLOOD_STONE:
      return new desirous_blood_stone_t( effect );
    case SEARING_SMOKEY_STONE:
      return new searing_smokey_stone_t( effect );

    // healing stones
    case DELUGING_WATER_STONE:
      return nullptr;
    case WILD_SPIRIT_STONE:
      return nullptr;
    case EXUDING_STEAM_STONE:
      return nullptr;

    // absorb stones
    case COLD_FROST_STONE:
      return new cold_frost_stone_t( effect );
    case INDOMITABLE_EARTH_STONE:
      return new indomitable_earth_stone_t( effect );
    case GLEAMING_IRON_STONE:
      return new gleaming_iron_stone_t( effect );

    // misc
    case HARMONIC_MUSIC_STONE:
      return nullptr;

    default:
      break;
  }

  return nullptr;
}

/**Echoing Thunder Stone
 * id=404866 Primordial Stones aura
 * id=402929 driver
 * id=403094 stacking buff
 * id=403170 ready buff
 * id=403171 damage
 */
void echoing_thunder_stone( special_effect_t& effect )
{
  struct uncontainable_charge_cb_t : public primordial_stone_cb_t
  {
    buff_t* ready_buff;

    uncontainable_charge_cb_t( const special_effect_t& e, buff_t* b ) :
      primordial_stone_cb_t( e.player, e ), ready_buff ( b )
    {}

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( s->target->is_sleeping() )
        return;

      primordial_stone_cb_t::execute( a, s );

      ready_buff->expire();
    }
  };

  auto ready_buff = create_buff<buff_t>( effect.player, effect.player->find_spell( 403170 ) );
  auto counter    = create_buff<buff_t>( effect.player, effect.player->find_spell( 403094 ) )
                      ->set_expire_at_max_stack( true )
                      ->set_stack_change_callback( [ ready_buff ] ( buff_t*, int, int cur )
                        { if ( cur == 0 ) ready_buff->trigger(); } );

  auto trigger_counter = [ p = effect.player, counter ]
  {
    // If these effects become common or more precise control is desired, this check should be moved to a virtual method in player_t.
    bool can_move = !p->buffs.stunned->check();
    if ( p->channeling && !p->channeling->usable_moving() )
      can_move = false;
    if ( p->executing && !p->executing->usable_moving() )
      can_move = false;

    if ( can_move )
    {
      // Each time the driver ticks, stacks of Rolling Thunder are applied if the player is moving.
      // The base number of stacks is 4 and is modified by speed (seems to be rounded up from 3.5).
      // TODO: Check more data points to make sure this model is correct.
      int num_stacks = as<int>( std::round( 3.5 * p->composite_movement_speed() / p->base_movement_speed ) );
      counter->trigger( num_stacks );
    }
  };

  effect.player->register_combat_begin( [ effect, trigger_counter ] ( player_t* p )
  {
    timespan_t period = effect.driver()->effectN( 1 ).period();
    timespan_t first_update = p->rng().real() * period;
    make_event( p->sim, first_update, [ p, period, trigger_counter ]
    {
      trigger_counter();
      make_repeating_event( p->sim, period, [ trigger_counter ] { trigger_counter(); } );
    } );
  } );

  effect.spell_id = 403170;
  effect.execute_action = create_primordial_stone_action( effect, ECHOING_THUNDER_STONE );
  auto cb = new uncontainable_charge_cb_t( effect, ready_buff );
  cb->initialize();
  cb->deactivate();

  ready_buff->set_stack_change_callback( [ cb, counter ] ( buff_t*, int old, int )
  {
    if ( old == 0 )
    {
      cb->activate();
      counter->set_chance( 0.0 );
    }
    else
    {
      cb->deactivate();
      counter->set_chance( 1.0 );
    }
  } );
}

/**Flame Licked Stone
 * id=404865 Primordial Stones aura
 * id=402930 driver
 * id=403225 DoT
 */
void flame_licked_stone( special_effect_t& effect )
{
  effect.execute_action = create_primordial_stone_action( effect, FLAME_LICKED_STONE );

  new primordial_stone_cb_t( effect.player, effect );
}

/**Freezing Ice Stone
 * id=404874 Primordial Stones aura
 * id=402940 driver
 * id=403391 damage
 */
void freezing_ice_stone( special_effect_t& effect )
{
  effect.execute_action = create_primordial_stone_action( effect, FREEZING_ICE_STONE );

  new primordial_stone_cb_t( effect.player, effect );
}

void searing_smokey_stone( special_effect_t &effect )
{
  effect.execute_action = create_primordial_stone_action( effect, SEARING_SMOKEY_STONE );
  effect.proc_flags2_ |= PF2_CAST_INTERRUPT;

  new primordial_stone_cb_t( effect.player, effect );
}

void storm_infused_stone( special_effect_t& effect )
{
  effect.execute_action = create_primordial_stone_action( effect, STORM_INFUSED_STONE );
  effect.proc_flags2_ = PF2_CRIT;

  new primordial_stone_cb_t( effect.player, effect );
}

void desirous_blood_stone( special_effect_t& effect )
{
  effect.execute_action = create_primordial_stone_action( effect, DESIROUS_BLOOD_STONE );

  new primordial_stone_cb_t( effect.player, effect );
}

void pestilent_plague_stone( special_effect_t& effect )
{
  effect.execute_action = create_primordial_stone_action( effect, PESTILENT_PLAGUE_STONE );

  new primordial_stone_cb_t( effect.player, effect );
}

void humming_arcane_stone( special_effect_t& effect )
{
  effect.execute_action = create_primordial_stone_action( effect, HUMMING_ARCANE_STONE );

  new primordial_stone_cb_t( effect.player, effect );

  effect.player->callbacks.register_callback_trigger_function(
      effect.driver()->id(), dbc_proc_callback_t::trigger_fn_type::CONDITION,
      []( const dbc_proc_callback_t*, action_t* a, action_state_t* ) {
        return a->get_school() != SCHOOL_PHYSICAL;
      } );
}

void shining_obsidian_stone( special_effect_t& effect )
{
  create_primordial_stone_action( effect, SHINING_OBSIDIAN_STONE );
}

void cold_frost_stone( special_effect_t& effect )
{
  // TODO slow NYI
  auto shield = create_primordial_stone_action( effect, COLD_FROST_STONE );
  timespan_t period = effect.driver()->effectN( 1 ).period();

  effect.player->register_combat_begin( [ shield, period ]( player_t* p ) {
    make_event( p->sim, p->rng().real() * period, [ p, period, shield ] {
      shield->execute();
      make_repeating_event( p->sim, period, [ shield ] { shield->execute(); } );
    } );
  } );
}

void indomitable_earth_stone( special_effect_t& effect )
{
  effect.execute_action = create_primordial_stone_action( effect, INDOMITABLE_EARTH_STONE );

  new dbc_proc_callback_t( effect.player, effect );
}

void gleaming_iron_stone( special_effect_t& effect )
{
  auto shield = create_primordial_stone_action( effect, GLEAMING_IRON_STONE );

  effect.player->register_combat_begin( [ shield ]( player_t* p ) {
    make_event( p->sim, 3_s, [ shield ] { shield->execute(); } );
  } );
}

void obscure_pastel_stone( special_effect_t& effect )
{
  static constexpr std::array<primordial_stone_drivers_e, 14> possible_stones = {
    // damage stones
    STORM_INFUSED_STONE,
    ECHOING_THUNDER_STONE,
    FLAME_LICKED_STONE,
    SHINING_OBSIDIAN_STONE,
    FREEZING_ICE_STONE,
    HUMMING_ARCANE_STONE,
    PESTILENT_PLAGUE_STONE,
    DESIROUS_BLOOD_STONE,
    // healing stones
    DELUGING_WATER_STONE,
    WILD_SPIRIT_STONE,
    EXUDING_STEAM_STONE,
    // absorb stones
    COLD_FROST_STONE,
    INDOMITABLE_EARTH_STONE,
    // other
    HARMONIC_MUSIC_STONE,
  };

  struct obscure_pastel_stone_t : public generic_proc_t
  {
    std::vector<action_t*> stone_actions;

    obscure_pastel_stone_t( const special_effect_t& effect ) :
      generic_proc_t( effect, "obscure_pastel_stone", 405257 ), stone_actions()
    {
      stone_actions.reserve( stone_actions.size() );
      for ( auto driver : possible_stones )
        stone_actions.push_back( create_primordial_stone_action( effect, driver ) );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        // TODO: If heal and absorb procs are implemented, they should target the player.
        auto action = stone_actions[ rng().range( stone_actions.size() ) ];
        if ( action )
          action->execute_on_target( s->target );
      }
    }
  };

  effect.execute_action = create_proc_action<obscure_pastel_stone_t>( "obscure_pastel_stone", effect );

  new primordial_stone_cb_t( effect.player, effect);
}
}  // namespace primordial_stones

void register_special_effects()
{
  // Food
  // Phials
  register_special_effect( 374000, consumables::iced_phial_of_corrupting_rage );
  register_special_effect( 371386, consumables::phial_of_charged_isolation );
  register_special_effect( 371339, consumables::phial_of_elemental_chaos );
  register_special_effect( 373257, consumables::phial_of_glacial_fury );
  register_special_effect( 370652, consumables::phial_of_static_empowerment );

  // Potion
  register_special_effect( 372046, consumables::bottled_putrescence );
  register_special_effect( { 371149, 371151, 371152 }, consumables::chilled_clarity );
  register_special_effect( { 371024, 371028 }, consumables::elemental_power );  // normal & ultimate
  register_special_effect( 370816, consumables::shocking_disclosure );

  // Enchants
  register_special_effect( { 390164, 390167, 390168 }, enchants::writ_enchant() );  // burning writ
  register_special_effect( { 390172, 390183, 390190 }, enchants::writ_enchant() );  // earthen writ
  register_special_effect( { 390243, 390244, 390246 }, enchants::writ_enchant() );  // frozen writ
  register_special_effect( { 390248, 390249, 390251 }, enchants::writ_enchant() );  // wafting writ
  register_special_effect( { 390215, 390217, 390219 },
                           enchants::writ_enchant( STAT_STR_AGI_INT ) );     // sophic writ
  register_special_effect( { 390346, 390347, 390348 },
                           enchants::writ_enchant( STAT_BONUS_ARMOR ) );            // earthen devotion
  register_special_effect( { 390222, 390227, 390229 },
                           enchants::writ_enchant( STAT_STR_AGI_INT ) );     // sophic devotion
  register_special_effect( { 390351, 390352, 390353 }, enchants::frozen_devotion );
  register_special_effect( { 390358, 390359, 390360 }, enchants::wafting_devotion );

  register_special_effect( { 386260, 386299, 386305 }, enchants::completely_safe_rockets );
  register_special_effect( { 386156, 386157, 386158 }, enchants::high_intensity_thermal_scanner );
  register_special_effect( { 385765, 385886, 385892 }, enchants::gyroscopic_kaleidoscope );
  register_special_effect( { 385939, 386127, 386136 }, enchants::projectile_propulsion_pinion );
  register_special_effect( { 387302, 387303, 387306 }, enchants::temporal_spellthread );
  register_special_effect( { 405764, 405765, 405766 }, enchants::shadowflame_wreathe );

  // Trinkets
  register_special_effect( 408671, items::dragonfire_bomb_dispenser );
  register_special_effect( 384636, items::consume_pods );                            // burgeoning seed
  register_special_effect( 376636, items::idol_of_the_aspects( "neltharite" ) );     // idol of the earth warder
  register_special_effect( 376638, items::idol_of_the_aspects( "ysemerald" ) );      // idol of the dreamer
  register_special_effect( 376640, items::idol_of_the_aspects( "malygite" ) );       // idol of the spellweaver
  register_special_effect( 376642, items::idol_of_the_aspects( "alexstraszite" ) );  // idol of the lifebinder
  register_special_effect( 384615, items::darkmoon_deck_dance );
  register_special_effect( 382957, items::darkmoon_deck_inferno );
  register_special_effect( 386624, items::darkmoon_deck_rime );
  register_special_effect( 384532, items::darkmoon_deck_watcher );
  register_special_effect( 375626, items::alacritous_alchemist_stone );
  register_special_effect( 383751, items::bottle_of_spiraling_winds );
  register_special_effect( 396391, items::conjured_chillglobe );
  register_special_effect( 388931, items::globe_of_jagged_ice );
  register_special_effect( 383941, items::irideus_fragment );
  register_special_effect( 383798, items::emerald_coachs_whistle );
  register_special_effect( 381471, items::erupting_spear_fragment );
  register_special_effect( 383920, items::furious_ragefeather );
  register_special_effect( 402813, items::igneous_flowstone );
  register_special_effect( 388603, items::idol_of_pure_decay );
  register_special_effect( 384191, items::shikaari_huntress_arrowhead );
  register_special_effect( 377466, items::spiteful_storm );
  register_special_effect( 381768, items::spoils_of_neltharus, true );
  register_special_effect( 397399, items::voidmenders_shadowgem );
  register_special_effect( 375844, items::sustaining_alchemist_stone );
  register_special_effect( 385884, items::timebreaching_talon );
  register_special_effect( 385902, items::umbrelskuls_fractured_heart );
  register_special_effect( 377456, items::way_of_controlled_currents );
  register_special_effect( 377452, items::whispering_incarnate_icon );
  register_special_effect( 384112, items::the_cartographers_calipers );
  register_special_effect( 377454, items::rumbling_ruby );
  register_special_effect( 377453, items::storm_eaters_boon, true );
  register_special_effect( 377449, items::decoration_of_flame );
  register_special_effect( 377463, items::manic_grieftorch );
  register_special_effect( 377457, items::alltotem_of_the_master );
  register_special_effect( 388559, items::tome_of_unstable_power );
  register_special_effect( 383781, items::algethar_puzzle_box );
  register_special_effect( 382119, items::frenzying_signoll_flare );
  register_special_effect( 386175, items::idol_of_trampling_hooves );
  register_special_effect( 386692, items::dragon_games_equipment );
  register_special_effect( 397400, items::bonemaws_big_toe );
  register_special_effect( 381705, items::mutated_magmammoth_scale );
  register_special_effect( 382139, items::homeland_raid_horn );
  register_special_effect( 383926, items::blazebinders_hoof );
  register_special_effect( 390764, items::primal_ritual_shell );
  register_special_effect( 383611, items::spinerippers_fang );
  register_special_effect( 392359, items::integrated_primal_fire );
  register_special_effect( 383817, items::bushwhackers_compass );
  register_special_effect( 392237, items::seasoned_hunters_trophy );
  register_special_effect( 374233, items::grals_discarded_tooth );
  register_special_effect( 391612, items::static_charged_scale );
  register_special_effect( 383812, items::ruby_whelp_shell );
  register_special_effect( 377464, items::desperate_invokers_codex, true );
  register_special_effect( 377455, items::iceblood_deathsnare );
  register_special_effect( 398292, items::winterpelt_totem );
  register_special_effect( 401468, items::screaming_black_dragonscale_equip);
  register_special_effect( 405940, items::screaming_black_dragonscale_use);
  register_special_effect( 403385, items::neltharions_call_to_suffering );
  register_special_effect( 403366, items::neltharions_call_to_chaos );
  register_special_effect( 403368, items::neltharions_call_to_dominance, true );
  register_special_effect( 402583, items::anshuul_the_cosmic_wanderer );
  register_special_effect( 400956, items::zaqali_chaos_grapnel );
  register_special_effect( 401306, items::elementium_pocket_anvil );
  register_special_effect( 401513, items::ominous_chromatic_essence );
  register_special_effect( 401238, items::ward_of_the_faceless_ire );
  register_special_effect( 395175, items::treemouths_festering_splinter );
  register_special_effect( 401395, items::vessel_of_searing_shadow );
  register_special_effect( 413419, items::heart_of_thunder );
  register_special_effect( 407895, items::drogbar_rocks );
  register_special_effect( 407903, items::drogbar_stones );
  register_special_effect( 408607, items::underlight_globe );
  register_special_effect( 408641, items::stirring_twilight_ember );
  register_special_effect( 407512, items::heatbound_medallion );
  register_special_effect( 408625, items::fractured_crystalspine_quill );
  register_special_effect( 407523, items::firecallers_focus );
  register_special_effect( 418527, items::mirror_of_fractured_tomorrows, true );

  // Weapons
  register_special_effect( 396442, items::bronzed_grip_wrappings );             // bronzed grip wrappings embellishment
  register_special_effect( 405226, items::spore_keepers_baton );                // Spore Keepers Baton Weapon
  register_special_effect( 377708, items::fang_adornments );                    // fang adornments embellishment
  register_special_effect( 381698, items::forgestorm );                         // Forgestorm Weapon
  register_special_effect( 394928, items::neltharax );                          // Neltharax, Enemy of the Sky
  register_special_effect( 408790, items::ashkandur );                          // Ashkandur, Fall of the Brotherhood
  register_special_effect( 408711, items::shadowed_razing_annihilator );        // Shadowed Razing Annihilator
  register_special_effect( 408821, items::djaruun_pillar_of_the_elder_flame, true );  // Djaruun, Pillar of the Elder Flame

  // Armor
  register_special_effect( 397038, items::assembly_guardians_ring );
  register_special_effect( 397040, items::assembly_scholars_loop );
  register_special_effect( 387335, items::blue_silken_lining );    // blue silken lining embellishment
  register_special_effect( 385520, items::breath_of_neltharion );  // breath of neltharion tinker
  register_special_effect( 378423, items::coated_in_slime );
  register_special_effect( 375323, items::elemental_lariat );
  register_special_effect( 381424, items::flaring_cowl );
  register_special_effect( 379396, items::thriving_thorns );
  register_special_effect( 394452, items::broodkeepers_blaze );
  register_special_effect( 387144, items::amice_of_the_blue );
  register_special_effect( 378134, items::rallied_to_victory );
  register_special_effect( 380717, items::deep_chill );
  register_special_effect( 379985, items::potent_venom );
  register_special_effect( 395959, items::allied_wristguards_of_companionship );
  register_special_effect( 378134, items::allied_chestplate_of_generosity );
  register_special_effect( 395601, items::hood_of_surging_time, true );
  register_special_effect( 409434, items::voice_of_the_silent_star, true );
  register_special_effect( 406254, items::roiling_shadowflame );
  register_special_effect( { 406219, 406928 }, items::adaptive_dracothyst_armguards );
  register_special_effect( 406244, items::ever_decaying_spores );
  // Sets
  register_special_effect( { 393620, 393982 }, sets::playful_spirits_fur );
  register_special_effect( { 393983, 393762 }, sets::horizon_striders_garments );
  register_special_effect( { 393987, 393768 }, sets::azureweave_vestments );
  register_special_effect( { 393993, 393818 }, sets::woven_chronocloth );
  register_special_effect( { 389987, 389498, 391117 }, sets::raging_tempests );
  register_special_effect( { 407914, 407939 }, sets::might_of_the_drogbar );

  // Primordial Stones
  register_special_effect( primordial_stones::ECHOING_THUNDER_STONE,    primordial_stones::echoing_thunder_stone );
  register_special_effect( primordial_stones::FLAME_LICKED_STONE,       primordial_stones::flame_licked_stone );
  register_special_effect( primordial_stones::FREEZING_ICE_STONE,       primordial_stones::freezing_ice_stone );
  register_special_effect( primordial_stones::STORM_INFUSED_STONE,      primordial_stones::storm_infused_stone );
  register_special_effect( primordial_stones::DESIROUS_BLOOD_STONE,     primordial_stones::desirous_blood_stone );
  register_special_effect( primordial_stones::PESTILENT_PLAGUE_STONE,   primordial_stones::pestilent_plague_stone );
  register_special_effect( primordial_stones::HUMMING_ARCANE_STONE,     primordial_stones::humming_arcane_stone );
  register_special_effect( primordial_stones::SHINING_OBSIDIAN_STONE,   primordial_stones::shining_obsidian_stone );
  register_special_effect( primordial_stones::OBSCURE_PASTEL_STONE,     primordial_stones::obscure_pastel_stone );
  register_special_effect( primordial_stones::SEARING_SMOKEY_STONE,     primordial_stones::searing_smokey_stone );
  register_special_effect( primordial_stones::COLD_FROST_STONE,         primordial_stones::cold_frost_stone );
  register_special_effect( primordial_stones::INDOMITABLE_EARTH_STONE,  primordial_stones::indomitable_earth_stone );
  register_special_effect( primordial_stones::GLEAMING_IRON_STONE,      primordial_stones::gleaming_iron_stone );
  register_special_effect( primordial_stones::ENTROPIC_FEL_STONE,       DISABLED_EFFECT ); // Necessary for other gems to find the driver.
  register_special_effect( primordial_stones::PROPHETIC_TWILIGHT_STONE, DISABLED_EFFECT );

  // Disabled
  register_special_effect( 408667, DISABLED_EFFECT );  // dragonfire bomb dispenser (skilled restock)
  register_special_effect( 382108, DISABLED_EFFECT );  // burgeoning seed
  register_special_effect( 382958, DISABLED_EFFECT );  // df darkmoon deck shuffler
  register_special_effect( 382913, DISABLED_EFFECT );  // bronzescale sigil (faster shuffle)
  register_special_effect( 383336, DISABLED_EFFECT );  // azurescale sigil (shuffle greatest to least)
  register_special_effect( 383333, DISABLED_EFFECT );  // emberscale sigil (no longer shuffles even cards)
  register_special_effect( 383337, DISABLED_EFFECT );  // jetscale sigil (no longer shuffles on Ace)
  register_special_effect( 383339, DISABLED_EFFECT );  // sagescale sigil (shuffle on jump) NYI
  register_special_effect( 378758, DISABLED_EFFECT );  // toxified embellishment
  register_special_effect( 371700, DISABLED_EFFECT );  // potion absorption inhibitor embellishment
  register_special_effect( 381766, DISABLED_EFFECT );  // spoils of neltharius shuffler
  register_special_effect( 383931, DISABLED_EFFECT );  // globe of jagged ice counter
  register_special_effect( 389843, DISABLED_EFFECT );  // ruby whelp shell (on-use)
  register_special_effect( 377465, DISABLED_EFFECT );  // Desperate Invocation (cdr proc)
  register_special_effect( 398396, DISABLED_EFFECT );  // emerald coach's whistle on-use
  register_special_effect( 382132, DISABLED_EFFECT );  // Iceblood Deathsnare damage data
  register_special_effect( 410530, DISABLED_EFFECT );  // Infurious Boots of Reprieve - Mettle (NYI)
}

void register_target_data_initializers( sim_t& sim )
{
  sim.register_target_data_initializer( items::dragonfire_bomb_dispenser_initializer_t() );
  sim.register_target_data_initializer( items::awakening_rime_initializer_t() );
  sim.register_target_data_initializer( items::skewering_cold_initializer_t() );
  sim.register_target_data_initializer( items::spiteful_storm_initializer_t() );
  sim.register_target_data_initializer( items::heavens_nemesis_initializer_t() );
  sim.register_target_data_initializer( items::iceblood_deathsnare_initializer_t() );
  sim.register_target_data_initializer( items::ever_decaying_spores_initializer_t() );
}

void register_hotfixes()
{
}

// check and return multiplier for toxified armor patch
// TODO: spell data seems to indicate you can have up to 4 stacks. currently implemented as a simple check
double toxified_mul( player_t* player )
{
  if ( auto toxic = unique_gear::find_special_effect( player, 378758 ) )
    return 1.0 + toxic->driver()->effectN( 2 ).percent();

  return 1.0;
}

// check and return multiplier for potion absorption inhibitor
double inhibitor_mul( player_t* player )
{
  int count = 0;

  for ( const auto& item : player->items )
    if ( range::contains( item.parsed.special_effects, 371700U, &special_effect_t::spell_id ) )
      count++;

  return 1.0 + count * player->find_spell( 371700 )->effectN( 1 ).percent();
}
}  // namespace unique_gear::dragonflight
