// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_shadowlands.hpp"

#include "sim/sc_sim.hpp"

#include "sim/sc_cooldown.hpp"
#include "buff/sc_buff.hpp"
#include "action/dot.hpp"
#include "item/item.hpp"

#include "actor_target_data.hpp"
#include "darkmoon_deck.hpp"
#include "unique_gear.hpp"
#include "unique_gear_helper.hpp"

namespace unique_gear
{
namespace shadowlands
{
// Enchants & Consumable buffs affected by 'Exacting Preparations' soulbind
struct SL_buff_t : public buff_t
{
  SL_buff_t( actor_pair_t q, util::string_view n, const spell_data_t* s ) : buff_t( q, n, s ) {}

  buff_t* set_default_value_from_effect_type( effect_subtype_t a, property_type_t p = P_MAX, double m = 0.0,
                                              effect_type_t e = E_APPLY_AURA ) override
  {
    buff_t::set_default_value_from_effect_type( a, p, m, e );

    auto cov_player = player->is_enemy() ? source : player;
    auto ep  = cov_player->find_soulbind_spell( "Exacting Preparation" );
    if ( ep->ok() )
      apply_affecting_aura( ep );

    return this;
  }
};

// Enchants & Consumable procs affected by 'Exacting Preparations' soulbind
struct SL_proc_spell_t : public proc_spell_t
{
  SL_proc_spell_t( const special_effect_t& e ) : proc_spell_t( e ) {}

  SL_proc_spell_t( util::string_view n, player_t* p, const spell_data_t* s ) : proc_spell_t( n, p, s ) {}

  void init() override
  {
    proc_spell_t::init();

    auto ep = player->find_soulbind_spell( "Exacting Preparation" );
    for ( const auto& eff : ep->effects() )
    {
      if ( data().affected_by_label( eff ) )
      {
        base_multiplier *= 1.0 + eff.percent();
      }
    }
  }
};

// feasts initialization helper
void init_feast( special_effect_t& effect, std::initializer_list<std::pair<stat_e, int>> stat_map )
{
  effect.stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );
  // TODO: Is this actually spec specific?
  if ( effect.player->role == ROLE_TANK && !effect.player->sim->feast_as_dps )
    effect.stat = STAT_STAMINA;

  for ( auto&& stat : stat_map )
  {
    if ( stat.first == effect.stat )
    {
      effect.trigger_spell_id = stat.second;
      break;
    }
  }
  effect.stat_amount = effect.player->find_spell( effect.trigger_spell_id )->effectN( 1 ).average( effect.player );
}

namespace consumables
{
void smothered_shank( special_effect_t& effect )
{
  struct smothered_shank_buff_t : public buff_t
  {
    action_t* belch;

    smothered_shank_buff_t( const special_effect_t& e ) : buff_t( e.player, "smothered_shank", e.driver() )
    {
      belch = create_proc_action<SL_proc_spell_t>( "pungent_belch", e );

      set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        belch->set_target( player->target );
        belch->schedule_execute();
      } );
    }
  };

  effect.name_str = "pungent_belch";
  effect.custom_buff = make_buff<smothered_shank_buff_t>( effect );
}

void surprisingly_palatable_feast( special_effect_t& effect )
{
  init_feast( effect,
              {{STAT_STRENGTH, 327701}, {STAT_STAMINA, 327702}, {STAT_INTELLECT, 327704}, {STAT_AGILITY, 327705}} );
}

void feast_of_gluttonous_hedonism( special_effect_t& effect )
{
  init_feast( effect,
              {{STAT_STRENGTH, 327706}, {STAT_STAMINA, 327707}, {STAT_INTELLECT, 327708}, {STAT_AGILITY, 327709}} );
}

// For things that require check for oils, the effect must be passed as first parameter of the constructor, but this
// parameter is not passed onto the base constructor
template <typename Base>
struct SL_potion_proc_t : public Base
{
  using base_t = SL_potion_proc_t<Base>;

  bool shadowcore;
  bool embalmers;

  template <typename... Args>
  SL_potion_proc_t( const special_effect_t& e, Args&&... args ) : Base( std::forward<Args>( args )... )
  {
    shadowcore = unique_gear::find_special_effect( e.player, 321382 );
    embalmers  = unique_gear::find_special_effect( e.player, 321393 );
  }
};

// proc potion initialization helper
template <typename T>
void init_proc_potion( special_effect_t& effect, util::string_view name )
{
  auto potion            = new special_effect_t( effect.player );
  potion->type           = SPECIAL_EFFECT_EQUIP;
  potion->spell_id       = effect.spell_id;
  potion->cooldown_      = 0_ms;
  potion->execute_action = create_proc_action<T>( name, effect );
  effect.player->special_effects.push_back( potion );

  auto proc = new dbc_proc_callback_t( effect.player, *potion );
  proc->deactivate();
  proc->initialize();

  effect.custom_buff = make_buff( effect.player, effect.name(), effect.driver() )
    ->set_cooldown( 0_ms )
    ->set_chance( 1.0 )
    ->set_stack_change_callback( [proc]( buff_t*, int, int new_ ) {
      if ( new_ )
        proc->activate();
      else
        proc->deactivate();
    } );
}

void potion_of_deathly_fixation( special_effect_t& effect )
{
  struct deathly_eruption_t : public SL_potion_proc_t<proc_spell_t>
  {
    deathly_eruption_t( const special_effect_t& e )
      : base_t( e, "deathly_eruption", e.player, e.player->find_spell( 322256 ) )
    {
      if ( shadowcore )
        base_multiplier *= 1.0 + e.driver()->effectN( 2 ).percent();
    }
  };

  struct deathly_fixation_t : public proc_spell_t
  {
    action_t* eruption;

    deathly_fixation_t( const special_effect_t& e )
      : proc_spell_t( "deathly_fixation", e.player, e.player->find_spell( 322253 ) ),
        eruption( create_proc_action<deathly_eruption_t>( "deathly_eruption", e ) )
    {
      add_child( eruption );
    }

    void impact( action_state_t* s ) override
    {
      auto d = get_dot( s->target );

      if ( d->at_max_stacks( 1 ) )
      {
        eruption->set_target( s->target );
        eruption->schedule_execute();
        d->cancel();
      }
      else
        proc_spell_t::impact( s );
    }
  };

  init_proc_potion<deathly_fixation_t>( effect, "potion_of_deathly_fixation" );
}

void potion_of_empowered_exorcisms( special_effect_t& effect )
{
  struct empowered_exorcisms_t : public SL_potion_proc_t<proc_spell_t>
  {
    empowered_exorcisms_t( const special_effect_t& e )
      : base_t( e, "empowered_exorcisms", e.player, e.player->find_spell( 322015 ) )
    {
      if ( shadowcore )
      {
        base_multiplier *= 1.0 + e.driver()->effectN( 2 ).percent();
        radius *= 1.0 + e.driver()->effectN( 2 ).percent();
      }
    }

    // manually adjust for aoe reduction here instead of via action_t::reduced_aoe_damage as all targets receive reduced
    // damage, including the primary
    double composite_aoe_multiplier( const action_state_t* s ) const override
    {
      return proc_spell_t::composite_aoe_multiplier( s ) / std::sqrt( s->n_targets );
    }
  };

  init_proc_potion<empowered_exorcisms_t>( effect, "potion_of_empowered_exorcisms" );
}

void potion_of_phantom_fire( special_effect_t& effect )
{
  struct phantom_fire_t : public SL_potion_proc_t<proc_spell_t>
  {
    phantom_fire_t( const special_effect_t& e )
      : base_t( e, "phantom_fire", e.player, e.player->find_spell( 321937 ) )
    {
      if ( shadowcore )
        base_multiplier *= 1.0 + e.driver()->effectN( 2 ).percent();
    }
  };

  init_proc_potion<phantom_fire_t>( effect, "potion_of_phantom_fire" );
}
}  // namespace consumables

namespace enchants
{
void celestial_guidance( special_effect_t& effect )
{
  effect.custom_buff = buff_t::find( effect.player, "celestial_guidance" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff<SL_buff_t>( effect.player, "celestial_guidance", effect.player->find_spell( 324748 ) )
      ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
      ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
      ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
      ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
  }

  new dbc_proc_callback_t( effect.player, effect );
}

void lightless_force( special_effect_t& effect )
{
  effect.trigger_spell_id = 324184;
  effect.execute_action   = create_proc_action<SL_proc_spell_t>( "lightless_force", effect );
  // Spell projectile travels along a narrow beam and does not spread out with distance. Although data has a 40yd range,
  // the 1.5s duration seems to prevent the projectile from surviving beyond ~30yd.
  // TODO: confirm maximum effective range & radius
  effect.execute_action->aoe    = 5;
  effect.execute_action->radius = 5.0;
  effect.execute_action->range  = 30.0;

  new dbc_proc_callback_t( effect.player, effect );
}

void sinful_revelation( special_effect_t& effect )
{
  struct sinful_revelation_cb_t : public dbc_proc_callback_t
  {
    sinful_revelation_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto td = a->player->get_target_data( s->target );
      td->debuff.sinful_revelation->trigger();
    }
  };

  new sinful_revelation_cb_t( effect );
}
}  // namespace enchants

namespace items
{
// Trinkets

void darkmoon_deck_shuffle( special_effect_t& effect )
{
  // Disable the effect, as we handle shuffling within the on-use effect
  effect.type = SPECIAL_EFFECT_NONE;
}

struct SL_darkmoon_deck_t : public darkmoon_deck_t
{
  std::vector<unsigned> card_ids;
  std::vector<const spell_data_t*> cards;
  const spell_data_t* top;

  SL_darkmoon_deck_t( const special_effect_t& e, const std::vector<unsigned>& c )
    : darkmoon_deck_t( e ), card_ids( c ), top( spell_data_t::nil() )
  {}

  void initialize() override
  {
    for ( auto c : card_ids )
    {
      auto s = player->find_spell( c );
      if ( s->ok () )
        cards.push_back( s );
    }

    top = cards[ player->rng().range( cards.size() ) ];

    player->register_combat_begin( [this]( player_t* ) {
      make_event<shuffle_event_t>( *player->sim, this, true );
    } );
  }

  void shuffle() override
  {
    top = cards[ player->rng().range( cards.size() ) ];

    player->sim->print_debug( "{} top card is now {} ({})", player->name(), top->name_cstr(), top->id() );
  }
};

struct SL_darkmoon_deck_proc_t : public proc_spell_t
{
  SL_darkmoon_deck_t* deck;

  SL_darkmoon_deck_proc_t( const special_effect_t& e, util::string_view n, unsigned shuffle_id,
                           std::initializer_list<unsigned> card_list )
    : proc_spell_t( n, e.player, e.trigger(), e.item )
  {
    auto shuffle = unique_gear::find_special_effect( player, shuffle_id );
    if ( !shuffle )
      return;

    deck = new SL_darkmoon_deck_t( *shuffle, card_list );
    deck->initialize();
  }

  ~SL_darkmoon_deck_proc_t() { delete deck; }

  // Shadowlands Darkmoon Decks utilize spells which are flagged to scale with item level, but instead return a value as
  // if scaled to item level = max scaling level. As darkmoon decks so far are all item level 200, whether there is any
  // actual item level scaling is unknown atm.
  double get_effect_value( const spell_data_t* spell = nullptr, int index = 1 )
  {
    if ( !spell )
      spell = s_data;

    return spell->effectN( index ).m_coefficient() *
           player->dbc->random_property( spell->max_scaling_level() ).damage_secondary;
  }
};

void darkmoon_deck_putrescence( special_effect_t& effect )
{
  struct putrid_burst_t : public SL_darkmoon_deck_proc_t
  {
    putrid_burst_t( const special_effect_t& e )
      : SL_darkmoon_deck_proc_t( e, "putrid_burst", 333885,
                                 {311464, 311465, 311466, 311467, 311468, 311469, 311470, 311471} )
    {
      split_aoe_damage = true;
      base_dd_max = base_dd_min = get_effect_value();
    }

    void impact( action_state_t* s ) override
    {
      SL_darkmoon_deck_proc_t::impact( s );

      auto td = player->get_target_data( s->target );
      // Crit debuff value uses player level scaling, not item scaling
      td->debuff.putrid_burst->trigger( 1, deck->top->effectN( 1 ).average( player ) * 0.0001 );
    }
  };

  effect.trigger_spell_id = effect.spell_id;
  effect.execute_action   = new putrid_burst_t( effect );
}

void darkmoon_deck_voracity( special_effect_t& effect )
{
  struct voracious_hunger_t : public SL_darkmoon_deck_proc_t
  {
    stat_buff_t* buff;

    voracious_hunger_t( const special_effect_t& e )
      : SL_darkmoon_deck_proc_t( e, "voracious_hunger", 329446,
                                 {311483, 311484, 311485, 311486, 311487, 311488, 311489, 311490} )
    {
      may_crit = false;

      buff = make_buff<stat_buff_t>( player, "voracious_haste", e.driver()->effectN( 2 ).trigger(), item )
        ->add_stat( STAT_HASTE_RATING, 0 );
    }

    void execute() override
    {
      SL_darkmoon_deck_proc_t::execute();

      buff->stats[ 0 ].amount = deck->top->effectN( 1 ).average( player );
      buff->trigger();
    }
  };

  effect.trigger_spell_id = effect.spell_id;
  effect.execute_action   = new voracious_hunger_t( effect );
}

void stone_legion_heraldry( special_effect_t& effect )
{
  double amount   = effect.driver()->effectN( 1 ).average( effect.item );
  unsigned allies = effect.player->sim->shadowlands_opts.stone_legionnaires_in_party;
  double mul      = 1.0 + effect.driver()->effectN( 2 ).percent() * allies;

  effect.player->passive.versatility_rating += amount * mul;

  // Disable further effect handling
  effect.type = SPECIAL_EFFECT_NONE;
}

void cabalists_hymnal( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "crimson_chorus" );
  if ( !buff )
  {
    auto mul =
        1.0 + effect.driver()->effectN( 2 ).percent() * effect.player->sim->shadowlands_opts.crimson_choir_in_party;
    timespan_t duration = effect.trigger()->duration() * effect.trigger()->max_stacks();

    buff = make_buff<stat_buff_t>( effect.player, "crimson_chorus", effect.trigger(), effect.item )
      ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 1 ).average( effect.item ) * mul )
      ->set_period( effect.trigger()->duration() )
      ->set_duration( duration )
      ->set_cooldown( effect.player->find_spell( 344820 )->duration() + duration );
  }

  effect.custom_buff = buff;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

void dreadfire_vessel( special_effect_t& effect )
{
  struct dreadfire_vessel_proc_t : public proc_spell_t
  {
    dreadfire_vessel_proc_t( const special_effect_t& e ) : proc_spell_t( e ) {}

    timespan_t travel_time() const override
    {
      // seems to have a set 1.5s travel time
      return timespan_t::from_seconds( data().missile_speed() );
    }
  };

  effect.execute_action = create_proc_action<dreadfire_vessel_proc_t>( "dreadfire_vessel", effect );
}

void macabre_sheet_music( special_effect_t& effect )
{
  auto data_spell = effect.player->find_spell( 345431 );
  auto stat_buff  = make_buff<stat_buff_t>( effect.player, "blood_waltz", effect.player->find_spell( 345439 ) )
    ->add_stat( STAT_HASTE_RATING, data_spell->effectN( 1 ).average( effect.item ) )
    ->add_stat( STAT_SPEED_RATING, data_spell->effectN( 2 ).average( effect.item ) );

  effect.custom_buff = make_buff( effect.player, "blood_waltz_driver", effect.driver() )
    ->set_quiet( true )
    ->set_cooldown( 0_ms )
    ->set_tick_callback( [ stat_buff ]( buff_t*, int, timespan_t ) {
      // TODO: handle potential delay/movement required to hit the 'dance partners'
      stat_buff->trigger();
    } );
}

void glyph_of_assimilation( special_effect_t& effect )
{
  struct glyph_of_assimilation_t : public proc_spell_t
  {
    buff_t* buff;

    glyph_of_assimilation_t( const special_effect_t& e, buff_t* b ) : proc_spell_t( e ), buff( b ) {}

    void last_tick( dot_t* d ) override
    {
      buff->trigger( 2.0 * composite_dot_duration( d->state ) );
      proc_spell_t::last_tick( d );
    }
  };

  auto p    = effect.player;
  auto buff = make_buff<stat_buff_t>( p, "glyph_of_assimilation", p->find_spell( 345500 ) );
  buff->add_stat( STAT_MASTERY_RATING, buff->data().effectN( 1 ).average( effect.item ) );

  // TODO: This trinket actually always gives the full duration buff to the player when
  // the DoT expires regardless of the remaining duration and whether or not the target
  // dies. Test this again later to make sure it still behaves this way.
  if ( p->bugs )
  {
    effect.execute_action = create_proc_action<glyph_of_assimilation_t>( "glyph_of_assimilation", effect, buff );
  }
  else
  {
    range::for_each( p->sim->actor_list, [ p, buff ]( player_t* t ) {
      if ( !t->is_enemy() )
        return;

      t->register_on_demise_callback( p, [ p, buff ]( player_t* t ) {
        if ( p->sim->event_mgr.canceled )
          return;

        auto d = t->get_dot( "glyph_of_assimilation", p );
        if ( d->remains() > 0_ms )
          buff->trigger( d->remains() * 2.0 );
      } );
    } );
  }
}

/**Soul Igniter
 * id=345251 driver with 500 ms cooldown
 * id=345211 buff and max scaling targets
 * id=345215 damage effect and category cooldown of second action
 * id=345214 damage coefficients and multipliers
 *           effect #1: base damage (effect #2) + buff time bonus (effect #3)
 *                      incorrectly listed as the self damage in the tooltip
 *           effect #2: base damage
 *           effect #3: max bonus damage (40%) from buff time
 *           effect #4: multiplier based on buff elapsed duration (maybe only used for the tooltip)
 *           effect #5: self damage per 1-second buff tick (NYI)
 */
void soul_igniter( special_effect_t& effect )
{
  struct blazing_surge_t : public proc_spell_t
  {
    double buff_fraction_elapsed;
    double max_time_multiplier;
    unsigned max_scaling_targets;

    blazing_surge_t( const special_effect_t& e ) : proc_spell_t( "blazing_surge", e.player, e.player->find_spell( 345215 ) )
    {
      split_aoe_damage = true;
      base_dd_min = e.player->find_spell( 345214 )->effectN( 2 ).min( e.item );
      base_dd_max = e.player->find_spell( 345214 )->effectN( 2 ).max( e.item );
      max_time_multiplier = e.player->find_spell( 345214 )->effectN( 4 ).percent();
      max_scaling_targets = as<unsigned>( e.player->find_spell( 345211 )->effectN( 2 ).base_value() );
    }

    double action_multiplier() const override
    {
      double m = proc_spell_t::action_multiplier();

      // This may actually be added as flat damage using the coefficients,
      // but for a trinket like this it is difficult to verify this.
      m *= 1.0 + buff_fraction_elapsed * max_time_multiplier;

      return m;
    }

    double composite_aoe_multiplier( const action_state_t* s ) const override
    {
      double m = proc_spell_t::composite_aoe_multiplier( s );

      // The extra damage for each target appears to be a 15%
      // multiplier that is not listed in spell data anywhere.
      // TODO: this has been tested on 6 targets, verify that
      // it does not continue scaling higher at 7 targets.
      m *= 1.0 + 0.15 * std::min( s->n_targets - 1, max_scaling_targets );

      return m;
    }
  };

  struct soul_ignition_t : public proc_spell_t
  {
    buff_t* buff;
    const spell_data_t* second_action;

    soul_ignition_t( const special_effect_t& e ) :
      proc_spell_t( "soul_ignition", e.player, e.driver() ),
      second_action( e.player->find_spell( 345215 ) )
    {}

    void init_finished() override
    {
      buff = buff_t::find( player, "soul_ignition" );
    }

    bool ready() override
    {
      if ( buff->check() && sim->shadowlands_opts.disable_soul_igniter_second_use )
        return false;

      return proc_spell_t::ready();
    }

    void execute() override
    {
      proc_spell_t::execute();

      // Need to trigger the category cooldown that this trinket does not have on other trinkets.
      auto cd_group = player->get_cooldown( "item_cd_" + util::to_string( second_action->category() ) );
      if ( cd_group )
        cd_group->start( second_action->category_cooldown() );

      if ( buff->check() )
        buff->expire();
      else
        buff->trigger();
    }
  };

  struct soul_ignition_buff_t : public buff_t
  {
    special_effect_t& effect;
    blazing_surge_t* damage_action;

    soul_ignition_buff_t( special_effect_t& e, action_t* d ) :
      buff_t( e.player, "soul_ignition", e.player->find_spell( 345211 ) ),
      effect( e ),
      damage_action( debug_cast<blazing_surge_t*>( d ) )
    {}

    void expire_override( int stacks, timespan_t remaining_duration )
    {
      buff_t::expire_override( stacks, remaining_duration );

      damage_action->buff_fraction_elapsed = ( buff_duration() - remaining_duration ) / buff_duration();
      damage_action->set_target( source->target );
      damage_action->execute();
      // the 60 second cooldown associated with the damage effect trigger
      // does not appear in spell data anywhere and is just in the tooltip.
      effect.execute_action->cooldown->start( effect.execute_action, 60_s );
      auto cd_group = player->get_cooldown( effect.cooldown_group_name() );
      if ( cd_group )
        cd_group->start( effect.cooldown_group_duration() );
    }
  };

  auto damage_action = create_proc_action<blazing_surge_t>( "blazing_surge", effect );
  effect.execute_action = create_proc_action<soul_ignition_t>( "soul_ignition", effect );
  auto buff = buff_t::find( effect.player, "soul_ignition" );
  if ( !buff )
    buff = make_buff<soul_ignition_buff_t>( effect, damage_action );
}

void skulkers_wing( special_effect_t& effect )
{

}

void memory_of_past_sins( special_effect_t& effect )
{

}

void gluttonous_spike( special_effect_t& effect )
{

}

void hateful_chain( special_effect_t& effect )
{

}

void bottled_chimera_toxin( special_effect_t& effect )
{
  // Assume the player keeps the buff up on its own as the item is off-gcd and
  // the buff has a 60 minute duration which is enough for any encounter.
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct chimeric_poison_t : public proc_spell_t
  {
    chimeric_poison_t( const special_effect_t& e )
      : proc_spell_t( "chimeric_poison", e.player, e.trigger(), e.item )
    {
      // Tick damage value lives in a different spell for some reason
      base_td = e.player->find_spell( 345547 )->effectN( 1 ).average( e.item );
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t duration ) const override
    {
      return dot->time_to_next_tick() + duration;
    }
  };

  auto secondary      = new special_effect_t( effect.item );
  secondary->type     = SPECIAL_EFFECT_EQUIP;
  secondary->source   = SPECIAL_EFFECT_SOURCE_ITEM;
  secondary->spell_id = effect.spell_id;
  secondary->execute_action = create_proc_action<chimeric_poison_t>( "chimeric_poison", *secondary );
  effect.player->special_effects.push_back( secondary );

  auto callback = new dbc_proc_callback_t( effect.item, *secondary );
  callback->initialize();
}

/**Empyreal Ordnance
 * id=345539 driver
 * id=345540 projectiles and DoT
 * id=345542 DoT damage
 * id=345541 buff
 * id=345543 buff value
 */
void empyreal_ordnance( special_effect_t& effect )
{
  struct empyreal_ordnance_bolt_t : public proc_spell_t
  {
    buff_t* buff;
    double buff_travel_speed;

    empyreal_ordnance_bolt_t( const special_effect_t& e, buff_t* b ) :
      proc_spell_t( "empyreal_ordnance_bolt", e.player, e.player->find_spell( 345540 ) ),
      buff( b )
    {
      base_td = e.player->find_spell( 345542 )->effectN( 1 ).average( e.item );
      buff_travel_speed = e.player->find_spell( 345544 )->missile_speed();
    }

    timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t duration ) const override
    {
      return duration;
    }

    void last_tick( dot_t* d ) override
    {
      double distance = player->get_player_distance( *d->target );
      // TODO: This travel time seems slightly wrong at long range (about 500 ms too fast).
      timespan_t buff_travel_time = timespan_t::from_seconds( buff_travel_speed ? distance / buff_travel_speed : 0 );
      int stack = d->current_stack();
      make_event( *sim, buff_travel_time, [ this, stack ] { buff->trigger( stack ); } );
      proc_spell_t::last_tick( d );
    }
  };

  struct empyreal_ordnance_t : public proc_spell_t
  {
    action_t* empyreal_ordnance_bolt;
    size_t num_bolts;

    empyreal_ordnance_t( const special_effect_t& e, buff_t* b ) :
      proc_spell_t( "empyreal_ordnance", e.player, e.driver() )
    {
      num_bolts = as<size_t>( e.driver()->effectN( 1 ).base_value() );
      aoe = as<int>( e.driver()->effectN( 2 ).base_value() );
      // Spell data for this trinket has a cast time of 500 ms even though it is instant and can be used while moving.
      base_execute_time = 0_ms;
      empyreal_ordnance_bolt = create_proc_action<empyreal_ordnance_bolt_t>( "empyreal_ordnance_bolt", e, b );
      add_child( empyreal_ordnance_bolt );
    }

    timespan_t travel_time() const override
    {
      return data().cast_time();
    }

    void impact( action_state_t* s ) override
    {
      size_t n = num_bolts / s->n_targets + ( s->chain_target < num_bolts % s->n_targets ? 1 : 0 );
      player_t* t = s->target;
      for ( size_t i = 0; i < n; i++ )
      {
        // The bolts don't seem to all hit the target
        // at exactly the same time, which creates a
        // small partial tick at the end of the DoT.
        make_event( *sim, i * 50_ms, [ this, t ]
          {
            empyreal_ordnance_bolt->set_target( t );
            empyreal_ordnance_bolt->execute();
          } );
      }
    }
  };

  auto buff = buff_t::find( effect.player, "empyreal_surge" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "empyreal_surge", effect.player->find_spell( 345541 ) )
             ->add_stat( STAT_INTELLECT, effect.player->find_spell( 345543 )->effectN( 1 ).average( effect.item ) );
  }

  effect.execute_action = create_proc_action<empyreal_ordnance_t>( "empyreal_ordnance", effect, buff );
}

// The slow debuff on the target and the heal when gaining the crit buff are not implemented.
void soulletting_ruby( special_effect_t& effect )
{
  struct soulletting_ruby_t : public proc_spell_t
  {
    stat_buff_t* buff;
    double base_crit_value;
    double max_crit_bonus;

    soulletting_ruby_t( const special_effect_t& e, stat_buff_t* b ) :
      proc_spell_t( "soulletting_ruby", e.player, e.player->find_spell( 345802 ) ),
      buff( b ),
      base_crit_value( e.player->find_spell( 345807 )->effectN( 1 ).average( e.item ) ),
      max_crit_bonus( e.player->find_spell( 345807 )->effectN( 2 ).average( e.item ) )
    {}

    void execute() override
    {
      proc_spell_t::execute();
      double value = base_crit_value + max_crit_bonus * ( 1.0 - target->health_percentage() * 0.01 );
      make_event( *sim, travel_time(), [ this, value ]
        {
          buff->stats.back().amount = value;
          buff->trigger();
        } );
    }
  };

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "soul_infusion" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "soul_infusion", effect.player->find_spell( 345805 ) )
             ->add_stat( STAT_CRIT_RATING, 0 );
  }

  effect.execute_action = create_proc_action<soulletting_ruby_t>( "soulletting_ruby", effect, buff );
}

void satchel_of_misbegotten_minions( special_effect_t& effect )
{
  struct abomiblast_t : public proc_spell_t
  {
    unsigned max_scaling_targets;

    abomiblast_t( const special_effect_t& e ) :
      proc_spell_t( "abomiblast", e.player, e.player->find_spell( 345638 ) )
    {
      split_aoe_damage = true;
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
      max_scaling_targets = as<unsigned>( e.driver()->effectN( 2 ).base_value() );
    }

    double composite_aoe_multiplier( const action_state_t* s ) const override
    {
      double m = proc_spell_t::composite_aoe_multiplier( s );

      // The extra damage for each target appears to be a 15%
      // multiplier that is not listed in spell data anywhere.
      // TODO: this has been tested above 2 target, verify the target cap.
      m *= 1.0 + 0.15 * std::min( s->n_targets - 1, max_scaling_targets );

      return m;
    }
  };

  struct misbegotten_minion_t : public proc_spell_t
  {
    misbegotten_minion_t( const special_effect_t& e ) :
      proc_spell_t( "misbegotten_minion", e.player, e.player->find_spell( 346032 ) )
    {
      quiet = true;
      impact_action = create_proc_action<abomiblast_t>( "abomiblast", e );
    }
  };

  effect.execute_action = create_proc_action<misbegotten_minion_t>( "misbegotten_minion", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// Runecarves

void echo_of_eonar( special_effect_t& effect )
{
  if ( !effect.player->buffs.echo_of_eonar )
  {
    effect.player->buffs.echo_of_eonar =
      make_buff( effect.player, "echo_of_eonar", effect.player->find_spell( 347458 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
        ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  // TODO: buff to allies? (id=338489)

  effect.custom_buff = effect.player->buffs.echo_of_eonar;

  new dbc_proc_callback_t( effect.player, effect );
}

void judgment_of_the_arbiter( special_effect_t& effect )
{
  struct judgment_of_the_arbiter_arc_t : public proc_spell_t
  {
    judgment_of_the_arbiter_arc_t( const special_effect_t& e )
      : proc_spell_t( "judgment_of_the_arbiter_arc", e.player, e.player->find_spell( 339675 ) )
    {
      aoe    = -1;
      radius = 20.0;  // hardcoded here as decription says arcing happens to ally 'within 5-20 yards of you'
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      proc_spell_t::available_targets( tl );

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* ) {
        return !rng().roll( sim->shadowlands_opts.judgment_of_the_arbiter_arc_chance );
      }), tl.end() );

      return tl.size();
    }

    void execute() override
    {
      // Calling available_targets here to forces regeneration of target_cache every execute, since we're using a random
      // roll to model targets getting hit by the arc. Also prevents executes when rolls result in no targets, so we
      // don't get inaccurate execute count reporting.
      if ( available_targets( target_cache.list ) )
        proc_spell_t::execute();
      else
      {
        // sanity check, but this shouldn't be required for normal execution as no state is passed to schedule_execute()
        if ( pre_execute_state )
          action_state_t::release( pre_execute_state );
      }
    }
  };

  struct judgment_of_the_arbiter_proc_t : public proc_spell_t
  {
    action_t* arc;

    judgment_of_the_arbiter_proc_t( const special_effect_t& e ) : proc_spell_t( e ), arc( nullptr )
    {
      if ( sim->shadowlands_opts.judgment_of_the_arbiter_arc_chance > 0.0 )
      {
        arc = create_proc_action<judgment_of_the_arbiter_arc_t>( "judgment_of_the_arbiter_arc", e );
        add_child( arc );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( arc )
      {
        arc->set_target( execute_state->target );
        arc->schedule_execute();
      }
    }
  };

  effect.execute_action = create_proc_action<judgment_of_the_arbiter_proc_t>( "judgment_of_the_arbiter", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void maw_rattle( special_effect_t& effect )
{

}

void norgannons_sagacity( special_effect_t& effect )
{
  effect.proc_flags2_ |= PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.custom_buff = effect.player->buffs.norgannons_sagacity_stacks;

  new dbc_proc_callback_t( effect.player, effect );
}

void sephuzs_proclamation( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "sephuzs_proclamation" );
  if ( !buff )
  {
    auto val = effect.trigger()->effectN( 1 ).average( effect.player );
    buff     = make_buff<stat_buff_t>( effect.player, "sephuzs_proclamation", effect.trigger() )
               ->add_stat( STAT_HASTE_RATING, val )
               ->add_stat( STAT_CRIT_RATING, val )
               ->add_stat( STAT_MASTERY_RATING, val )
               ->add_stat( STAT_VERSATILITY_RATING, val );
  }

  effect.proc_flags2_ |= PF2_CAST_INTERRUPT;
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void third_eye_of_the_jailer( special_effect_t& effect )
{

}

void vitality_sacrifice( special_effect_t& effect )
{

}

void overflowing_anima_cage( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "anima_infusion" );
  if ( !buff )
  {
    auto val = effect.player->find_spell( 343387 )->effectN( 1 ).average( effect.item );
    // The actual buff is Anima Infusion (id=343386), but the spell data there is not needed.
    buff = make_buff<stat_buff_t>( effect.player, "anima_infusion", effect.player->find_spell( 343385 ) )
             ->add_stat( STAT_CRIT_RATING, val )
             ->set_cooldown( 0_ms );
  }

  effect.custom_buff = buff;
}

void sunblood_amethyst( special_effect_t& effect )
{
  struct anima_font_proc_t : public proc_spell_t
  {
    buff_t* buff;

    anima_font_proc_t( const special_effect_t& e )
      : proc_spell_t( "anima_font", e.player, e.driver()->effectN( 2 ).trigger() )
    {
      // id:344414 Projectile with travel speed data (effect->driver->eff#2->trigger)
      // id:343394 'Font of power' spell with duration data (projectile->eff#1->trigger)
      // id:343396 Actual buff is id:343396
      // id:343397 Coefficient data
      buff = make_buff<stat_buff_t>( e.player, "anima_font", e.player->find_spell( 343396 ) )
        ->add_stat( STAT_INTELLECT, e.player->find_spell( 343397 )->effectN( 1 ).average( e.item ) )
        ->set_duration( data().effectN( 1 ).trigger()->duration() );
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      // TODO: add way to handle staying within range of the anima font to gain the int buff
      buff->trigger();
    }
  };

  struct tear_anima_proc_t : public proc_spell_t
  {
    action_t* font;

    tear_anima_proc_t( const special_effect_t& e )
      : proc_spell_t( e ), font( create_proc_action<anima_font_proc_t>( "anima_font", e ) )
    {}

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      // Assumption is that the 'tear' travels back TO player FROM target, which for sim purposes is treated as a normal
      // projectile FROM player TO target
      font->set_target( s->target );
      font->schedule_execute();
    }
  };

  effect.trigger_spell_id = effect.spell_id;
  effect.execute_action   = create_proc_action<tear_anima_proc_t>( "tear_anima", effect );
}

}  // namespace items

void register_hotfixes()
{
}

void register_special_effects()
{
    // Food
    unique_gear::register_special_effect( 308637, consumables::smothered_shank );
    unique_gear::register_special_effect( 308458, consumables::surprisingly_palatable_feast );
    unique_gear::register_special_effect( 308462, consumables::feast_of_gluttonous_hedonism );

    // Potion
    unique_gear::register_special_effect( 307497, consumables::potion_of_deathly_fixation );
    unique_gear::register_special_effect( 307494, consumables::potion_of_empowered_exorcisms );
    unique_gear::register_special_effect( 307495, consumables::potion_of_phantom_fire );

    // Enchant
    unique_gear::register_special_effect( 324747, enchants::celestial_guidance );
    unique_gear::register_special_effect( 323932, enchants::lightless_force );
    unique_gear::register_special_effect( 324250, enchants::sinful_revelation );

    // Trinkets
    unique_gear::register_special_effect( 333885, items::darkmoon_deck_shuffle );
    unique_gear::register_special_effect( 334058, items::darkmoon_deck_putrescence );
    unique_gear::register_special_effect( 329446, items::darkmoon_deck_shuffle );
    unique_gear::register_special_effect( 331624, items::darkmoon_deck_voracity );
    unique_gear::register_special_effect( 344686, items::stone_legion_heraldry );
    unique_gear::register_special_effect( 344806, items::cabalists_hymnal );
    unique_gear::register_special_effect( 344732, items::dreadfire_vessel );
    unique_gear::register_special_effect( 345432, items::macabre_sheet_music );
    unique_gear::register_special_effect( 345319, items::glyph_of_assimilation );
    unique_gear::register_special_effect( 345251, items::soul_igniter );
    unique_gear::register_special_effect( 345019, items::skulkers_wing );
    unique_gear::register_special_effect( 344662, items::memory_of_past_sins );
    unique_gear::register_special_effect( 344063, items::gluttonous_spike );
    unique_gear::register_special_effect( 345357, items::hateful_chain );
    unique_gear::register_special_effect( 343385, items::overflowing_anima_cage );
    unique_gear::register_special_effect( 343393, items::sunblood_amethyst );
    unique_gear::register_special_effect( 345545, items::bottled_chimera_toxin );
    unique_gear::register_special_effect( 345539, items::empyreal_ordnance );
    unique_gear::register_special_effect( 345801, items::soulletting_ruby );
    unique_gear::register_special_effect( 345567, items::satchel_of_misbegotten_minions );

    // Runecarves
    unique_gear::register_special_effect( 338477, items::echo_of_eonar );
    unique_gear::register_special_effect( 339344, items::judgment_of_the_arbiter );
    unique_gear::register_special_effect( 340197, items::maw_rattle );
    unique_gear::register_special_effect( 339340, items::norgannons_sagacity );
    unique_gear::register_special_effect( 339348, items::sephuzs_proclamation );
    unique_gear::register_special_effect( 339058, items::third_eye_of_the_jailer );
    unique_gear::register_special_effect( 338743, items::vitality_sacrifice );
}

void register_target_data_initializers( sim_t& sim )
{
  // Sinful Revelation
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 324250 ) )
    {
      assert( !td->debuff.sinful_revelation );

      td->debuff.sinful_revelation = make_buff<SL_buff_t>( *td, "sinful_revelation", td->source->find_spell( 324260 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );
      td->debuff.sinful_revelation->reset();
    }
    else
      td->debuff.sinful_revelation = make_buff( *td, "sinful_revelation" )->set_quiet( true );
  } );

  // Darkmoon Deck: Putrescence
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 334058 ) )
    {
      assert( !td->debuff.putrid_burst );

      td->debuff.putrid_burst = make_buff<buff_t>( *td, "putrid_burst", td->source->find_spell( 334058 ) )
        ->set_cooldown( 0_ms );
      td->debuff.putrid_burst->reset();
    }
    else
      td->debuff.putrid_burst = make_buff( *td, "putrid_burst" )->set_quiet( true );
  } );
}

}  // namespace shadowlands
}  // namespace unique_gear
