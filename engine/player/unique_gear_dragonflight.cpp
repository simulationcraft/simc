// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_dragonflight.hpp"

#include "action/dot.hpp"
#include "actor_target_data.hpp"
#include "buff/buff.hpp"
#include "darkmoon_deck.hpp"
#include "dbc/item_database.hpp"
#include "ground_aoe.hpp"
#include "item/item.hpp"
#include "item/item_targetdata_initializer.hpp"
#include "sim/sim.hpp"
#include "set_bonus.hpp"
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
      ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );

    buff = make_buff( effect.player, "iced_phial_of_corrupting_rage", effect.driver() )
      ->set_cooldown( 0_ms )
      ->set_chance( 1.0 )
      ->set_stack_change_callback( [ crit ]( buff_t*, int, int new_ ) {
        if ( new_ )
          crit->trigger();
        else
          crit->expire();
      } );

    auto debuff = make_buff( effect.player, "overwhelming_rage", effect.player->find_spell( 374037 ) )
      ->set_stack_change_callback( [ buff, crit ]( buff_t*, int, int new_ ) {
        if ( !new_ && buff->check() )
          crit->trigger();
      } );

    crit->set_stack_change_callback( [ buff, debuff ]( buff_t*, int, int new_ ) {
      if ( !new_ && buff->check() )
        debuff->trigger();
    } );
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
            ->add_stat( STAT_STR_AGI_INT, amount );

    auto linger_buff =
        make_buff<stat_buff_t>( effect.player, "phial_of_charged_isolation_linger", effect.player->find_spell( 384713 ) )
            ->add_stat( STAT_STR_AGI_INT, amount * linger_mul );

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
  struct glacial_fury_t : public proc_spell_t
  {
    const buff_t* buff;

    glacial_fury_t( const special_effect_t& e, const buff_t* b )
      : proc_spell_t( "glacial_fury", e.player, e.driver()->effectN( 2 ).trigger() ), buff( b )
    {
      base_dd_min = base_dd_max = e.driver()->effectN( 3 ).average( e.item );
    }

    double composite_aoe_multiplier( const action_state_t* s ) const override
    {
      // spell data is flagged to ignore all caster multpliers, so da_multiplier is not snapshot. however, the 15% per
      // buff does take effect and as this is an aoe action, we can use composite_aoe_multiplier without having  to
      // adjust snapshot flags.
      return proc_spell_t::composite_aoe_multiplier( s ) * ( 1.0 + buff->check_stack_value() );
    }
  };

  // Buff that triggers when you hit a new target
  auto new_target_buff = buff_t::find( effect.player, "glacial_fury" );
  if ( !new_target_buff )
  {
    new_target_buff = make_buff( effect.player, "glacial_fury", effect.player->find_spell( 373265 ) )
      ->set_default_value( effect.driver()->effectN( 4 ).percent() );
  }

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
  auto buff = buff_t::find( effect.player, effect.name() );
  if ( !buff )
  {
    buff = make_buff( effect.player, effect.name(), effect.driver() )
      ->set_chance( 1.0 )
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
  }

  effect.custom_buff = buff;
}

// TODO: implement reasonable model of losing buff due to movement
void phial_of_static_empowerment( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, effect.name() );
  if ( !buff )
  {
    auto primary = make_buff<stat_buff_t>( effect.player, "static_empowerment", effect.player->find_spell( 370772 ) );
    primary->add_stat( STAT_STR_AGI_INT, effect.driver()->effectN( 1 ).average( effect.item ) / primary->max_stack() );

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

    if ( effect.player->buffs.movement )
    {
      effect.player->buffs.movement->set_stack_change_callback( [ primary, buff ]( buff_t*, int, int new_ ) {
        if ( new_ && buff->check() )
          primary->expire();
      } );
    }
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

    bottled_putrescence_t( const special_effect_t& e )
      : proc_spell_t( "bottled_putrescence", e.player, e.driver() ),
        damage( create_proc_action<bottled_putrescence_tick_t>( "bottled_putresence", e ) )
    {
      damage->stats = stats;
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      make_event<ground_aoe_event_t>( *sim, player, ground_aoe_params_t()
        .target( s->target )
        .pulse_time( damage->base_tick_time )
        .duration( data().effectN( 1 ).trigger()->duration() - 1_ms )  // has 0tick, but no tick at the end
        .action( damage ), true );
    }
  };

  effect.execute_action = create_proc_action<bottled_putrescence_t>( "bottled_putrescence", effect );
}

void chilled_clarity( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "potion_of_chilled_clarity" } ) )
    return;

  auto buff = buff_t::find( effect.player, "potion_of_chilled_clarity" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "potion_of_chilled_clarity", effect.trigger() )
      ->set_default_value_from_effect_type( A_355 )
      ->set_duration( timespan_t::from_seconds( effect.driver()->effectN( 1 ).base_value() ) );
  }

  effect.custom_buff = effect.player->buffs.chilled_clarity = buff;
}

void shocking_disclosure( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "shocking_disclosure" );
  if ( !buff )
  {
    auto damage = create_proc_action<generic_aoe_proc_t>( "shocking_disclosure", effect, "shocking_disclosure",
                                                          effect.trigger() );
    damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    damage->dual = true;

    if ( auto pot_action = effect.player->find_action( "potion" ) )
      damage->stats = pot_action->stats;

    buff = make_buff( effect.player, "shocking_disclosure", effect.driver() )
      ->set_tick_callback( [ damage ]( buff_t* b, int, timespan_t ) {
        damage->execute_on_target( b->player->target );
      } );
  }

  effect.custom_buff = buff;
}
}  // namespace consumables

namespace enchants
{
custom_cb_t writ_enchant( stat_e stat, bool cr )
{
  return [ stat, cr ]( special_effect_t& effect ) {
    auto amount = effect.driver()->effectN( 1 ).average( effect.player );

    if ( cr )
    {
      amount = item_database::apply_combat_rating_multiplier( effect.player, CR_MULTIPLIER_WEAPON,
                                                              effect.player->level(), amount );
    }

    effect.stat_amount = amount;
    effect.stat = stat;

    new dbc_proc_callback_t( effect.player, effect );
  };
}

void earthen_devotion( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "earthen_devotion" );
  if ( !buff )
  {
    auto amount = effect.driver()->effectN( 1 ).average( effect.player );
    amount = item_database::apply_combat_rating_multiplier( effect.player, CR_MULTIPLIER_WEAPON,
                                                            effect.player->level(), amount );

    buff = make_buff<stat_buff_t>( effect.player, "earthen_devotion", effect.trigger() )
      ->add_stat( STAT_BONUS_ARMOR, amount );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void frozen_devotion( special_effect_t& effect )
{
  auto damage =
      create_proc_action<generic_aoe_proc_t>( "frozen_devotion", effect, "frozen_devotion", effect.trigger() );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.player );

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}

void titanic_devotion( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "titanic_devotion" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "titanic_devotion", effect.trigger() )
      ->add_stat( STAT_STR_AGI_INT, effect.driver()->effectN( 1 ).average( effect.player ) );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}


void wafting_devotion( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "wafting_devotion" );
  if ( !buff )
  {
    auto haste = effect.driver()->effectN( 1 ).average( effect.player );
    haste = item_database::apply_combat_rating_multiplier( effect.player, CR_MULTIPLIER_WEAPON,
                                                            effect.player->level(), haste );

    auto speed = effect.driver()->effectN( 2 ).average( effect.player );
    speed = item_database::apply_combat_rating_multiplier( effect.player, CR_MULTIPLIER_WEAPON,
                                                            effect.player->level(), speed );

    buff = make_buff<stat_buff_t>( effect.player, "wafting_devotion", effect.trigger() )
      ->add_stat( STAT_HASTE_RATING, haste )
      ->add_stat( STAT_SPEED_RATING, speed );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}
}  // namespace enchants

namespace items
{
// Trinkets
custom_cb_t idol_of_the_aspects( std::string_view type )
{
  return [ type ]( special_effect_t& effect )
  {
    int gems = 0;
    for ( const auto& item : effect.player->items )
      for ( auto gem_id : item.parsed.gem_id )
        if ( gem_id && util::str_in_str_ci( effect.player->dbc->item( gem_id ).name, type ) )
          gems++;

    if ( !gems )
      return;

    auto gift = buff_t::find( effect.player, "gift_of_the_aspects" );
    if ( !gift )
    {
      auto val = effect.driver()->effectN( 3 ).average( effect.item ) / 4;
      gift = make_buff<stat_buff_t>( effect.player, "gift_of_the_aspects", effect.player->find_spell( 376643 ) )
        ->add_stat( STAT_CRIT_RATING, val )
        ->add_stat( STAT_HASTE_RATING, val )
        ->add_stat( STAT_MASTERY_RATING, val )
        ->add_stat( STAT_VERSATILITY_RATING, val );
    }
    auto buff = buff_t::find( effect.player, effect.name() );
    if ( !buff )
    {
      auto stat = util::translate_rating_mod( effect.trigger()->effectN( 1 ).misc_value1() );
      buff = make_buff<stat_buff_t>( effect.player, effect.name(), effect.trigger() )
        ->add_stat( stat, effect.driver()->effectN( 1 ).average( effect.item ) )
        ->set_max_stack( as<int>( effect.driver()->effectN( 2 ).base_value() ) )
        ->set_expire_at_max_stack( true )
        ->set_stack_change_callback( [ gift ]( buff_t*, int, int new_ ) {
          if ( !new_ )
            gift->trigger();
        } );
    }

    effect.custom_buff = buff;

    new dbc_proc_callback_t( effect.player, effect );

    effect.player->callbacks.register_callback_execute_function(
        effect.driver()->id(), [ buff, gems ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
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
    if ( jetscale && top_index == cards.size() - 1 )
      return;

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

    if ( base_t::deck->shuffle_event )
      base_t::deck->shuffle_event->reschedule( base_t::data().category_cooldown() );
  }
};

void darkmoon_deck_dance( special_effect_t& effect )
{
  struct darkmoon_deck_dance_t : public DF_darkmoon_proc_t<>
  {
    action_t* damage;
    action_t* heal;

    // TODO: confirm mixed order remains true in-game
    // card order is [ 2 3 6 7 4 5 8 A ]
    darkmoon_deck_dance_t( const special_effect_t& e )
      : DF_darkmoon_proc_t( e, "refreshing_dance", 382958,
                            { 382861, 382862, 382865, 382866, 382863, 382864, 382867, 382860 } )
    {
      damage =
        create_proc_action<generic_proc_t>( "refreshing_dance_damage", e, "refreshing_dance_damage", 384613 );
      damage->background = damage->dual = true;
      damage->stats = stats;

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
          make_event( *sim, delay, [ s, this ]() { damage->execute_on_target( s->target ); } );
        else
          make_event( *sim, delay, [ this ]() { heal->execute_on_target( player ); } );
      }
    }
  };
  // TODO: currently this trinket is doing ~1% of the damage it should based on spell data.
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

        stat = buff_t::find( player, "watchers_blessing_stat" );
        if ( !stat )
        {
          stat = make_buff<stat_buff_t>( player, "watchers_blessing_stat", player->find_spell( 384560 ), item )
            ->set_name_reporting( "Stat" );
        }
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

// TODO: Do properly and add both drivers
// 383798 = Driver for you
// 386578 = Driver for target
// 383799 = Buff for Target (?)
// 383803 = Buff for You (?)
void emerald_coachs_whistle( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "star_coach" );
  if ( !buff )
  {
    auto buff_spell = effect.player->find_spell( 383803 );

    double amount = buff_spell->effectN( 1 ).average( effect.item );

    buff = make_buff<stat_buff_t>( effect.player, "star_coach", buff_spell )
               ->add_stat( STAT_MASTERY_RATING, amount );
  }

  effect.custom_buff  = buff;

  effect.proc_flags_  = effect.player->find_spell( 386578 )->proc_flags(); // Pretend we are our bonded partner for the sake of procs, and trigger it from that.
  effect.proc_flags2_ = PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

void the_cartographers_calipers( special_effect_t& effect )
{
  auto damage = create_proc_action<generic_proc_t>( "precision_blast", effect, "precision_blast", 384114 );
  damage->base_dd_min = damage->base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );

  effect.execute_action = damage;

  new dbc_proc_callback_t( effect.player, effect );
}
// Weapons
// Armor
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

  // TODO: does having more of a type increase the chances of getting that buff?
  unsigned gems = 0;
  for ( const auto& item : effect.player->items )
  {
    for ( auto gem_id : item.parsed.gem_id )
    {
      if ( gem_id )
      {
        auto n = effect.player->dbc->item( gem_id ).name;
        auto it = range::find( gem_types, n, &gem_name_type::first );
        if ( it != std::end( gem_types ) )
          gems |= ( *it ).second;
      }
    }
  }

  if ( !gems )
    return;

  auto val = effect.driver()->effectN( 1 ).average( effect.item );
  auto dur = timespan_t::from_seconds( effect.driver()->effectN( 2 ).base_value() );
  std::vector<buff_t*> buffs;

  auto add_buff = [ &effect, gems, val, dur, &buffs ]( gem_type_e type, std::string suf, unsigned id, stat_e stat ) {
    if ( gems & type )
    {
      auto name = "elemental_lariat__empowered_" + suf;
      auto buff = buff_t::find( effect.player, name );
      if ( !buff )
      {
        buff = make_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( id ) )
          ->add_stat( stat, val )
          ->set_duration( dur );
      }
      buffs.push_back( buff );
    }
  };

  add_buff( AIR_GEM, "air", 375342, STAT_HASTE_RATING );
  add_buff( EARTH_GEM, "earth", 375345, STAT_MASTERY_RATING );
  add_buff( FIRE_GEM, "flame", 375335, STAT_CRIT_RATING );
  add_buff( FROST_GEM, "frost", 375343, STAT_VERSATILITY_RATING );

  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_execute_function(
      effect.driver()->id(), [ buffs ]( const dbc_proc_callback_t* cb, action_t*, action_state_t* ) {
        buffs[ cb->rng().range( buffs.size() ) ]->trigger();
      } );
}
}  // namespace items

namespace sets
{
void playful_spirits_fur( special_effect_t& effect )
{
  if ( !effect.player->sets->has_set_bonus( effect.player->specialization(), T29_PLAYFUL_SPIRITS_FUR, B2 ) )
    return;

  auto snowball = create_proc_action<generic_proc_t>( "magic_snowball", effect, "magic_snowball", 376932 );

  if ( effect.driver() == effect.player->sets->set( effect.player->specialization(), T29_PLAYFUL_SPIRITS_FUR, B2 ) )
  {
    effect.execute_action = snowball;

    new dbc_proc_callback_t( effect.player, effect );
  }
  else
  {
    auto val = effect.driver()->effectN( 1 ).average( effect.item );

    snowball->base_dd_min += val;
    snowball->base_dd_max += val;
  }
}
}  // namespace sets

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
  register_special_effect( 370816, consumables::shocking_disclosure );

  // Enchants
  register_special_effect( { 390164, 390167, 390168 }, enchants::writ_enchant( STAT_CRIT_RATING ) );
  register_special_effect( { 390172, 390183, 390190 }, enchants::writ_enchant( STAT_MASTERY_RATING ) );
  register_special_effect( { 390243, 390244, 390246 }, enchants::writ_enchant( STAT_VERSATILITY_RATING ) );
  register_special_effect( { 390248, 390249, 390251 }, enchants::writ_enchant( STAT_HASTE_RATING ) );
  register_special_effect( { 390215, 390217, 390219 }, enchants::writ_enchant( STAT_STR_AGI_INT, false ) );
  register_special_effect( { 390346, 390347, 390348 }, enchants::earthen_devotion );
  register_special_effect( { 390351, 390352, 390353 }, enchants::frozen_devotion );
  register_special_effect( { 390222, 390227, 390229 }, enchants::titanic_devotion );
  register_special_effect( { 390358, 390359, 390360 }, enchants::wafting_devotion );

  // Trinkets
  register_special_effect( 376636, items::idol_of_the_aspects( "neltharite" ) );     // idol of the earth warder
  register_special_effect( 376638, items::idol_of_the_aspects( "ysemerald" ) );      // idol of the dreamer
  register_special_effect( 376640, items::idol_of_the_aspects( "malygite" ) );       // idol of the spellweaver
  register_special_effect( 376642, items::idol_of_the_aspects( "alexstraszite" ) );  // idol of the lifebinder
  register_special_effect( 384615, items::darkmoon_deck_dance );
  register_special_effect( 382957, items::darkmoon_deck_inferno );
  register_special_effect( 386624, items::darkmoon_deck_rime );
  register_special_effect( 384532, items::darkmoon_deck_watcher );
  register_special_effect( 383798, items::emerald_coachs_whistle );
  register_special_effect( 384112, items::the_cartographers_calipers );
  // Weapons
  // Armor
  register_special_effect( 375323, items::elemental_lariat );

  // Sets
  register_special_effect( { 393620, 393982 }, sets::playful_spirits_fur );

  // Disabled
  register_special_effect( 382958, DISABLED_EFFECT );  // df darkmoon deck shuffler
  register_special_effect( 382913, DISABLED_EFFECT );  // bronzescale sigil (faster shuffle)
  register_special_effect( 383336, DISABLED_EFFECT );  // azurescale sigil (shuffle greatest to least)
  register_special_effect( 383333, DISABLED_EFFECT );  // emberscale sigil (no longer shuffles even cards)
  register_special_effect( 383337, DISABLED_EFFECT );  // jetscale sigil (no longer shuffles on Ace)
  register_special_effect( 383339, DISABLED_EFFECT );  // sagescale sigil (shuffle on jump) NYI
}

void register_target_data_initializers( sim_t& sim )
{
  sim.register_target_data_initializer( items::awakening_rime_initializer_t() );
}
}  // namespace unique_gear::dragonflight
