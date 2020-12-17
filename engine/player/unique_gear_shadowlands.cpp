// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_shadowlands.hpp"

#include "sim/sc_sim.hpp"

#include "sim/sc_cooldown.hpp"
#include "player/action_priority_list.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
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
struct shadowlands_aoe_proc_t : public generic_aoe_proc_t
{
  shadowlands_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, const spell_data_t* s,
                  bool aoe_damage_increase_ = false ) :
    generic_aoe_proc_t( effect, name, s, aoe_damage_increase_ )
  {
    // Default still seems to be 6, however shadowlands seems to indicate the maximum
    // number (slightly confusingly) in the tooltip data, which is referencable directly
    // from spell data
    max_scaling_targets = 6;
  }

  shadowlands_aoe_proc_t( const special_effect_t& effect, ::util::string_view name, unsigned spell_id,
                       bool aoe_damage_increase_ = false ) :
    generic_aoe_proc_t( effect, name, spell_id, aoe_damage_increase_ )
  {
    max_scaling_targets = 6;
  }
};

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
  effect.disable_action();
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
      proc_spell_t::impact( s );

      auto d = get_dot( s->target );

      if ( d->at_max_stacks() )
      {
        eruption->set_target( s->target );
        eruption->schedule_execute();
        d->cancel();
      }
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
  struct lightless_force_proc_t : public SL_proc_spell_t
  {
    lightless_force_proc_t( const special_effect_t& e ) : SL_proc_spell_t( e )
    {
      // Spell projectile travels along a narrow beam and does not spread out with distance. Although data has a 40yd
      // range, the 1.5s duration seems to prevent the projectile from surviving beyond ~30yd.
      // TODO: confirm maximum effective range & radius
      aoe    = 5;
      radius = 5.0;
      range  = 30.0;

      // hardcoded as a dummy effect
      spell_power_mod.direct = data().effectN( 2 ).percent();
    }

    double composite_spell_power() const override
    {
      return std::max( proc_spell_t::composite_spell_power(), proc_spell_t::composite_attack_power() );
    }
  };

  effect.trigger_spell_id = 324184;
  effect.execute_action   = create_proc_action<lightless_force_proc_t>( "lightless_force", effect );

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
void DISABLED_EFFECT( special_effect_t& effect )
{
  // Disable the effect, as we handle shuffling within the on-use effect
  effect.type = SPECIAL_EFFECT_NONE;
}

// Trinkets

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
    }

    void impact( action_state_t* s ) override
    {
      SL_darkmoon_deck_proc_t::impact( s );

      auto td = player->get_target_data( s->target );
      // Crit debuff value is hard coded into each card
      td->debuff.putrid_burst->trigger( 1, deck->top->effectN( 1 ).base_value() * 0.0001 );
    }
  };

  effect.spell_id         = 347047;
  effect.trigger_spell_id = 334058;
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

      buff->stats[ 0 ].amount = deck->top->effectN( 1 ).average( item );
      buff->trigger();
    }
  };

  effect.trigger_spell_id = effect.spell_id;
  effect.execute_action   = new voracious_hunger_t( effect );
}

void stone_legion_heraldry( special_effect_t& effect )
{
  double amount   = effect.driver()->effectN( 1 ).average( effect.item );
  amount = item_database::apply_combat_rating_multiplier( *effect.item, amount );
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
  auto p    = effect.player;
  auto buff = make_buff<stat_buff_t>( p, "glyph_of_assimilation", p->find_spell( 345500 ) );
  buff->add_stat( STAT_MASTERY_RATING, buff->data().effectN( 1 ).average( effect.item ) );

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
  struct blazing_surge_t : public shadowlands_aoe_proc_t
  {
    double buff_fraction_elapsed;
    double max_time_multiplier;

    blazing_surge_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "blazing_surge", 345215, true )
    {
      split_aoe_damage = true;
      base_dd_min = e.player->find_spell( 345214 )->effectN( 2 ).min( e.item );
      base_dd_max = e.player->find_spell( 345214 )->effectN( 2 ).max( e.item );
      max_time_multiplier = e.player->find_spell( 345214 )->effectN( 4 ).percent();
      max_scaling_targets = as<unsigned>( e.player->find_spell( 345211 )->effectN( 2 ).base_value() + 1 );
    }

    double action_multiplier() const override
    {
      double m = proc_spell_t::action_multiplier();

      // This may actually be added as flat damage using the coefficients,
      // but for a trinket like this it is difficult to verify this.
      m *= 1.0 + buff_fraction_elapsed * max_time_multiplier;

      return m;
    }
  };

  struct soul_ignition_buff_t : public buff_t
  {
    special_effect_t& effect;
    blazing_surge_t* damage_action;
    bool is_precombat;

    soul_ignition_buff_t( special_effect_t& e, action_t* d ) :
      buff_t( e.player, "soul_ignition", e.player->find_spell( 345211 ) ),
      effect( e ),
      damage_action( debug_cast<blazing_surge_t*>( d ) ),
      is_precombat()
    {}

    void expire_override( int stacks, timespan_t remaining_duration )
    {
      // If the trinket was used in precombat, assume that it was timed so
      // that it will expire to deal full damage when it first expires.
      if ( is_precombat )
        remaining_duration = 0_ms;

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

  struct soul_ignition_t : public proc_spell_t
  {
    soul_ignition_buff_t* buff;
    const spell_data_t* second_action;
    bool has_precombat_action;

    soul_ignition_t( const special_effect_t& e ) :
      proc_spell_t( "soul_ignition", e.player, e.driver() ),
      second_action( e.player->find_spell( 345215 ) ),
      has_precombat_action()
    {
      harmful = false;

      for ( auto a : player->action_list )
      {
        if ( a->action_list && a->action_list->name_str == "precombat" && a->name_str == "use_item_" + e.item->name_str )
        {
          a->harmful = harmful;  // pass down harmful to allow action_t::init() precombat check bypass
          has_precombat_action = true;
          break;
        }
      }
    }

    void init_finished() override
    {
      buff = debug_cast<soul_ignition_buff_t*>( buff_t::find( player, "soul_ignition" ) );
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

      if ( buff->check() )
      {
        buff->expire();
      }
      else
      {
        // The cooldown does not need to be adjusted when this is used before combat begins because
        // the shared cooldown on other trinkets is triggered again when the buff expires and the
        // actual cooldown of this trinket does not start until the buff expires.
        buff->is_precombat = !player->in_combat && has_precombat_action;
        buff->trigger();
      }
    }
  };

  auto damage_action = create_proc_action<blazing_surge_t>( "blazing_surge", effect );
  effect.execute_action = create_proc_action<soul_ignition_t>( "soul_ignition", effect );
  auto buff = buff_t::find( effect.player, "soul_ignition" );
  if ( !buff )
    make_buff<soul_ignition_buff_t>( effect, damage_action );
}

/** Skulker's Wing
 * id=345019 driver speed buff, effect 1: 8y area trigger create
 * id=345113 dummy damage container spell
 * id=345020 actual triggered leap+damage spell
 */
void skulkers_wing( special_effect_t& effect )
{
  struct skulking_predator_t : public proc_spell_t
  {
    skulking_predator_t( const special_effect_t& e ) :
      proc_spell_t( "skulking_predator", e.player, e.player->find_spell( 345020 ) )
    {
      base_dd_min = base_dd_max = e.player->find_spell( 345113 )->effectN( 1 ).average( e.item );
    }
  };

  // Speed buff is only present until the damage trigger happens, this within 100ms if you are already in range
  // For now assume we are nearest to the primary target and just trigger the damage immediately
  // TODO: Speed buff + range-based target selection?

  effect.execute_action = create_proc_action<skulking_predator_t>( "skulking_predator", effect );
}

/** Memory of Past Sins
 * id=344662 driver buff proc stacks, effect 1: damage amp %
 * id=344663 target debuff damage amp stacks
 * id=344664 proc damage, effect 1: shadow damage
 */
void memory_of_past_sins( special_effect_t& effect )
{
  struct shattered_psyche_damage_t : public generic_proc_t
  {
    shattered_psyche_damage_t( const special_effect_t& e)
      : generic_proc_t( e, "shattered_psyche", 344664 )
    {
      callbacks = false;
    }

    double composite_target_da_multiplier( player_t* t ) const override
    {
      double m = proc_spell_t::composite_target_da_multiplier( t );
      auto td = player->get_target_data( t );
      m *= 1.0 + td->debuff.shattered_psyche->stack_value();
      return m;
    }

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      auto td = player->get_target_data( s->target );
      td->debuff.shattered_psyche->trigger();
    }
  };

  struct shattered_psyche_cb_t : public dbc_proc_callback_t
  {
    shattered_psyche_damage_t* damage;
    buff_t* buff;

    shattered_psyche_cb_t( const special_effect_t& effect, action_t* d, buff_t* b )
      : dbc_proc_callback_t( effect.player, effect ), damage( debug_cast<shattered_psyche_damage_t*>( d ) ), buff( b )
    {
    }

    void execute( action_t* a, action_state_t* trigger_state ) override
    {
      damage->target = trigger_state->target;
      damage->execute();
      buff->decrement();
    }
  };

  auto buff = buff_t::find( effect.player, "shattered_psyche" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "shattered_psyche", effect.player->find_spell( 344662 ) );
    buff->set_initial_stack( buff->max_stack() );
  }

  action_t* damage = create_proc_action<shattered_psyche_damage_t>( "shattered_psyche", effect );

  effect.custom_buff = buff;
  effect.disable_action();

  auto cb_driver = new special_effect_t( effect.player );
  cb_driver->name_str = "shattered_psyche_driver";
  cb_driver->spell_id = 344662;
  cb_driver->cooldown_ = 0_s;
  effect.player->special_effects.push_back( cb_driver );

  auto callback = new shattered_psyche_cb_t( *cb_driver, damage, buff );
  callback->initialize();
  callback->deactivate();

  buff->set_stack_change_callback( [ callback ]( buff_t*, int old, int new_ ) {
    if ( old == 0 )
      callback->activate();
    else if ( new_ == 0 )
      callback->deactivate();
  } );
}

void gluttonous_spike( special_effect_t& /* effect */ )
{

}

void hateful_chain( special_effect_t& effect )
{
  struct hateful_rage_t : public proc_spell_t
  {
    hateful_rage_t( const special_effect_t& e ) :
      proc_spell_t( "hateful_rage", e.player, e.player->find_spell( 345361 ) )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
    }
  };

  struct hateful_chain_callback_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      // XXX: Assume the actor always has more health than the target
      // TODO: Handle actor health < target health case?
      proc_action->target = target( state );
      proc_action->schedule_execute();
    }
  };

  effect.execute_action = create_proc_action<hateful_rage_t>( "hateful_rage", effect );

  new hateful_chain_callback_t( effect.player, effect );
}

void bottled_flayedwing_toxin( special_effect_t& effect )
{
  // Assume the player keeps the buff up on its own as the item is off-gcd and
  // the buff has a 60 minute duration which is enough for any encounter.
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct flayedwing_toxin_t : public proc_spell_t
  {
    flayedwing_toxin_t( const special_effect_t& e )
      : proc_spell_t( "flayedwing_toxin", e.player, e.trigger(), e.item )
    {
      // Tick behavior is odd, it always ticks on refresh but is not rescheduled
      tick_zero = true;
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
  secondary->execute_action = create_proc_action<flayedwing_toxin_t>( "flayedwing_toxin", *secondary );
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
      size_t n = num_bolts / s->n_targets + ( as<unsigned>( s->chain_target ) < num_bolts % s->n_targets ? 1 : 0 );
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
// id=345807: crit value coefficients
// id=345806: heal amoutn coefficient
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

      make_event( *sim, travel_time(), [ this, t = execute_state->target ] {
        double bonus_mul = 1.0 - ( t->is_active() ? t->health_percentage() * 0.01 : 0.0 );

        buff->stats.back().amount = base_crit_value + max_crit_bonus * bonus_mul;
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
  struct abomiblast_t : public shadowlands_aoe_proc_t
  {
    abomiblast_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "abomiblast", 345638, true )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
      max_scaling_targets = as<unsigned>( e.driver()->effectN( 2 ).base_value() + 1 );
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

/**
 * Mistcaller Ocarina
 * id=330067 driver #1 (versatility)
 * id=332299 driver #2 (crit)
 * id=332300 driver #3 (haste)
 * id=332301 driver #4 (mastery)
 * id=330132 buff #1 (versatility)
 * id=332077 buff #2 (crit)
 * id=332078 buff #3 (haste)
 * id=332079 buff #4 (mastery)
 */
void mistcaller_ocarina( special_effect_t& effect )
{
  using id_pair_t = std::pair<unsigned, unsigned>;
  static constexpr id_pair_t spells[] = {
    { 330067, 330132 }, // versatility
    { 332299, 332077 }, // crit
    { 332300, 332078 }, // haste
    { 332301, 332079 }, // mastery
  };
  auto it = range::find( spells, effect.spell_id, &id_pair_t::first );
  if ( it == range::end( spells ) )
    return; // Unknown driver spell, "disable" the trinket

  // Assume the player keeps the buff up on its own and disable some stuff
  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.cooldown_ = 0_ms;
  effect.duration_ = 0_ms;

  const spell_data_t* buff_spell = effect.player->find_spell( it->second );
  const std::string buff_name = util::tokenize_fn( buff_spell->name_cstr() );

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, buff_name ) );
  if ( !buff )
  {
    double amount = effect.driver()->effectN( 1 ).average( effect.item );

    buff = make_buff<stat_buff_t>( effect.player, buff_name, buff_spell );
    for ( auto& s : buff->stats )
      s.amount = amount;

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

/**Unbound Changeling
 * id=330747 coefficients for stat amounts, and also the special effect on the base item
 * id=330767 given by bonus_id=6915
 * id=330739 given by bonus_id=6916
 * id=330740 given by bonus_id=6917
 * id=330741 given by bonus_id=6918
 * id=330765 driver #1 (crit, haste, and mastery)
 * id=330080 driver #2 (crit)
 * id=330733 driver #3 (haste)
 * id=330734 driver #4 (mastery)
 * id=330764 buff #1 (crit, haste, and mastery)
 * id=330730 buff #2 (crit)
 * id=330131 buff #3 (haste)
 * id=330729 buff #4 (mastery)
 */
void unbound_changeling( special_effect_t& effect )
{
  auto stat_type = effect.player->sim->shadowlands_opts.unbound_changeling_stat_type;
  if ( stat_type == "all" )
    effect.spell_id = 330765;
  else if ( stat_type == "crit" )
    effect.spell_id = 330080;
  else if ( stat_type == "haste" )
    effect.spell_id = 330733;
  else if ( stat_type == "mastery" )
    effect.spell_id = 330734;
  else
  {
    if ( effect.spell_id == 330747 )
    {
      // If one of the bonus ID effects is present, bail out and let that bonus ID handle things instead.
      for ( auto& e : effect.item->parsed.special_effects )
      {
        if ( e->spell_id == 330767 || e->spell_id == 330739 || e->spell_id == 330740 || e->spell_id == 330741 )
            return;
      }
      // Fallback, profile does not specify a stat-giving item bonus, so default to haste.
      effect.spell_id = 330733;
    }
    else
    {
      effect.spell_id = effect.driver()->effectN( 1 ).trigger_spell_id();
    }
  }

  stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "unbound_changeling" ) );
  if ( effect.spell_id > 0 && !buff )
  {
    int buff_spell_id = effect.driver()->effectN( 1 ).trigger_spell_id();
    buff = make_buff<stat_buff_t>( effect.player, "unbound_changeling", effect.player->find_spell( buff_spell_id ) );
    double amount = effect.player->find_spell( 330747 )->effectN( buff_spell_id != 330764 ? 1 : 2 ).average( effect.item );

    for ( auto& s : buff->stats )
      s.amount = amount;

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

/**Infinitely Divisible Ooze
 * id=345490 driver and coefficient
 * id=345489 summon spell
 * id=345495 pet cast (noxious bolt)
 */
void infinitely_divisible_ooze( special_effect_t& effect )
{
  struct noxious_bolt_t : public spell_t
  {
    noxious_bolt_t( pet_t* p, const special_effect_t& e ) : spell_t( "noxious_bolt", p, p->find_spell( 345495 ) )
    {
      // Merge the stats object with other instances of the pet
      auto ta = p->owner->find_pet( "frothing_pustule" );
      if ( ta && ta->find_action( "noxious_bolt" ) )
        stats = ta->find_action( "noxious_bolt" )->stats;

      may_crit = false;
      base_dd_min = p->find_spell( 345490 )->effectN( 1 ).min( e.item );
      base_dd_max = p->find_spell( 345490 )->effectN( 1 ).max( e.item );
    }

    double composite_haste() const override
    {
      return 1.0;
    }

    double composite_versatility( const action_state_t* ) const override
    {
      return 1.0;
    }

    void execute() override
    {
      spell_t::execute();

      if ( player->resources.current[ RESOURCE_ENERGY ] <= 0 )
      {
        make_event( *sim, 0_ms, [ this ] { player->cast_pet()->dismiss(); } );
      }
    }
  };

  struct frothing_pustule_pet_t : public pet_t
  {
    const special_effect_t& effect;

    frothing_pustule_pet_t( const special_effect_t& e ) :
      pet_t( e.player->sim, e.player, "frothing_pustule", true, true ),
      effect( e )
    {}

    void init_base_stats() override
    {
      pet_t::init_base_stats();

      resources.base[ RESOURCE_ENERGY ] = 100;
    }

    resource_e primary_resource() const override
    {
      return RESOURCE_ENERGY;
    }

    action_t* create_action( util::string_view name, const std::string& options ) override
    {
      if ( name == "noxious_bolt" )
      {
        return new noxious_bolt_t( this, effect );
      }

      return pet_t::create_action( name, options );
    }

    void init_action_list() override
    {
      pet_t::init_action_list();

      if ( action_list_str.empty() )
        get_action_priority_list( "default" )->add_action( "noxious_bolt" );
    }
  };

  struct infinitely_divisible_ooze_cb_t : public dbc_proc_callback_t
  {
    spawner::pet_spawner_t<frothing_pustule_pet_t> spawner;

    infinitely_divisible_ooze_cb_t( const special_effect_t& e ) :
      dbc_proc_callback_t( e.player, e ),
      spawner( "infinitely_divisible_ooze", e.player, [ &e ]( player_t* )
        { return new frothing_pustule_pet_t( e ); } )
    {
      spawner.set_default_duration( e.player->find_spell( 345489 )->duration() );
    }

    void execute( action_t*, action_state_t* ) override
    {
      spawner.spawn();
    }
  };

  new infinitely_divisible_ooze_cb_t( effect );
}

/**Inscrutable Quantum Device
 * id=330323 driver
 * id=330363 remove CC from self (NYI)
 * id=330364 heal spell (NYI)
 * id=330372 illusion and threat drop (NYI)
 * id=347940 illusion helper spell, taunt (NYI)
 * id=347941 illusion helper aura (NYI)
 * id=330373 execute damage on target
 * id=330376 restore mana to healer (NYI)
 * id=330366 crit buff
 * id=330368 haste buff
 * id=330380 mastery buff
 * id=330367 vers buff
 * id=348098 unknown spell
 * When this trinket is used, it triggers one of the effects listed above, following the priority list below.
 * - remove CC from self: Always triggers if you are under a hard CC mechanic, does not trigger if the CC mechanic
 *                        does not prevent the player from acting (e.g., it won't trigger while rooted).
 * - heal spell: Triggers on self or a nearby target with less than 30% health remaining.
 * - illusion: ??? (not tested yet, priority unknown)
 * - execute damage: Deal damage to the target if it is an enemy with less than 20% health remaining (the 20% is not in spell data).
 * - healer mana: triggers on a nearby healer with less than 20% mana??? (not tested yet, priority unknown)
 * - secondary stat buffs:
 *   - If a Bloodlust buff is up, the stat buff will last 25 seconds instead of the default 20 seconds.
 *     TODO: Look for other buffs that also cause this bonus duration to occur. The spell data still lists
 *     a 30 second buff duration, so it is possible that there are other conditions that give 30 seconds.
 *   - The secondary stat granted appears to be randomly selected from stat from the player's two highest
 *     secondary stats in terms of rating. When selecting the largest stats, the priority of equal secondary
 *     stats seems to be Vers > Mastery > Haste > Crit. There is a bug where the second stat selected must
 *     have a lower rating than the first stat that was selected. If this is not possible, then the second
 *     stat will be empty and the trinket will have a chance to do nothing when used.
 */
void inscrutable_quantum_device ( special_effect_t& effect )
{
  static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };
  static constexpr std::array<int, 4> buff_ids = { 330367, 330380, 330368, 330366 };

  struct inscrutable_quantum_device_execute_t : public proc_spell_t
  {
    inscrutable_quantum_device_execute_t( const special_effect_t& e ) :
      proc_spell_t( "inscrutable_quantum_device_execute", e.player, e.player->find_spell( 330373 ), e.item )
    {
      cooldown->duration = 0_ms;
    }
  };

  struct inscrutable_quantum_device_t : public proc_spell_t
  {
    std::unordered_map<stat_e, buff_t*> buffs;
    std::vector<buff_t*> bonus_buffs;
    action_t* execute_damage;

    inscrutable_quantum_device_t( const special_effect_t& e ) :
      proc_spell_t( "inscrutable_quantum_device", e.player, e.player->find_spell( 330323 ) )
    {
      buffs[ STAT_NONE ] = nullptr;
      for ( unsigned i = 0; i < ratings.size(); i++ )
      {
        auto name = std::string( "inscrutable_quantum_device_" ) + util::stat_type_string( ratings[ i ] );
        stat_buff_t* buff = debug_cast<stat_buff_t*>( buff_t::find( e.player, name ) );
        if ( !buff )
        {
          buff = make_buff<stat_buff_t>( e.player, name, e.player->find_spell( buff_ids[ i ] ), e.item );
          buff->set_cooldown( 0_ms );
        }
        buffs[ ratings[ i ] ] = buff;
      }
      execute_damage = create_proc_action<inscrutable_quantum_device_execute_t>( "inscrutable_quantum_device_execute", e );
    }

    void init_finished() override
    {
      proc_spell_t::init_finished();

      bonus_buffs.clear();

      bonus_buffs.push_back( player->buffs.bloodlust );

      if ( player->type == MAGE )
      {
        if ( buff_t* b = buff_t::find( player, "temporal_warp" ) )
          bonus_buffs.push_back( b );
        if ( buff_t* b = buff_t::find( player, "time_warp" ) )
          bonus_buffs.push_back( b );
      }
    }

    bool is_buff_extended()
    {
      return range::any_of( bonus_buffs, [] ( buff_t* b ) { return b->check(); } );
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( target->health_percentage() <= 20 )
      {
        execute_damage->set_target( target );
        execute_damage->execute();
      }
      else
      {
        stat_e s1 = STAT_NONE;
        stat_e s2 = STAT_NONE;
        s1 = util::highest_stat( player, ratings );
        for ( auto s : ratings )
        {
          auto v = player->get_stat_value( s );
          if ( ( s2 == STAT_NONE || v > player->get_stat_value( s2 ) ) &&
               ( ( player->bugs && v < player->get_stat_value( s1 ) ) || ( !player->bugs && s != s1 ) ) )
            s2 = s;
        }

        buff_t* buff = rng().roll( 0.5 ) ? buffs[ s1 ] : buffs[ s2 ];
        if ( buff )
          buff->trigger( buff->buff_duration() - ( is_buff_extended() ? 5_s : 10_s ) );
      }
    }
  };

  effect.execute_action = create_proc_action<inscrutable_quantum_device_t>( "inscrutable_quantum_device", effect );
}

void phial_of_putrefaction( special_effect_t& effect )
{
  struct liquefying_ooze_t : public proc_spell_t
  {
    liquefying_ooze_t( const special_effect_t& e ) :
      proc_spell_t( "liquefying_ooze", e.player, e.player->find_spell( 345466 ), e.item )
    {
      base_td = e.driver()->effectN( 1 ).average( e.item );
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t ) const override
    { return dot->remains(); }
  };

  struct phial_of_putrefaction_proc_t : public dbc_proc_callback_t
  {
    phial_of_putrefaction_proc_t( const special_effect_t* e ) :
      dbc_proc_callback_t( e->player, *e ) { }

    void execute( action_t*, action_state_t* s ) override
    {
      auto d = proc_action->get_dot( s->target );

      // Phial only procs when at max stacks, otherwise the buff just lingers on the
      // character, waiting for the dot to fall off
      if ( !d->is_ticking() || !d->at_max_stacks() )
      {
        proc_buff->expire();
        proc_action->set_target( s->target );
        proc_action->execute();
      }
    }
  };

  auto putrefaction_buff = buff_t::find( effect.player, "phial_of_putrefaction" );
  if ( !putrefaction_buff )
  {
    putrefaction_buff = make_buff( effect.player, "phial_of_putrefaction",
        effect.player->find_spell( 345464 ) )
      ->set_duration( timespan_t::zero() );

    special_effect_t* putrefaction_proc = new special_effect_t( effect.player );
    putrefaction_proc->spell_id = 345464;
    putrefaction_proc->cooldown_ = 1_ms; // Proc only once per time unit
    putrefaction_proc->custom_buff = putrefaction_buff;
    putrefaction_proc->execute_action = create_proc_action<liquefying_ooze_t>(
        "liquefying_ooze", effect );

    effect.player->special_effects.push_back( putrefaction_proc );

    auto proc_object = new phial_of_putrefaction_proc_t( putrefaction_proc );
    proc_object->deactivate();

    putrefaction_buff->set_stack_change_callback( [proc_object]( buff_t*, int, int new_ ) {
      if ( new_ == 1 ) { proc_object->activate(); }
      else { proc_object->deactivate(); }
    } );

    effect.player->register_combat_begin( [&effect, putrefaction_buff]( player_t* ) {
      putrefaction_buff->trigger();
      make_repeating_event( putrefaction_buff->source->sim, effect.driver()->effectN( 1 ).period(),
          [putrefaction_buff]() { putrefaction_buff->trigger(); } );
    } );
  }
}

void grim_codex( special_effect_t& effect )
{
  struct grim_codex_t : public proc_spell_t
  {
    double dmg_primary = 0.0;
    double dmg_secondary = 0.0;

    grim_codex_t( const special_effect_t& e ) :
      proc_spell_t( "spectral_scythe", e.player, e.driver(), e.item )
    {
      aoe = -1;

      dmg_primary = player->find_spell( 345877 )->effectN( 1 ).average( e.item );
      dmg_secondary = player->find_spell( 345877 )->effectN( 2 ).average( e.item );
    }

    double base_da_min( const action_state_t* s ) const override
    { return s->chain_target == 0 ? dmg_primary : dmg_secondary; }

    double base_da_max( const action_state_t* s ) const override
    { return s->chain_target == 0 ? dmg_primary : dmg_secondary; }
  };

  effect.execute_action = create_proc_action<grim_codex_t>( "grim_codex", effect );
}

void anima_field_emitter( special_effect_t& effect )
{
  struct anima_field_emitter_proc_t : public dbc_proc_callback_t
  {
    timespan_t max_duration;

    anima_field_emitter_proc_t( const special_effect_t& e ) :
      dbc_proc_callback_t( e.player, e ),
      max_duration( effect.player->find_spell( 345534 )->duration() )
    { }

    void execute( action_t*, action_state_t* ) override
    {
      timespan_t buff_duration = max_duration;
      if ( listener->sim->shadowlands_opts.anima_field_emitter_mean != std::numeric_limits<double>::max() )
      {
        double new_duration = rng().gauss( listener->sim->shadowlands_opts.anima_field_emitter_mean,
            listener->sim->shadowlands_opts.anima_field_emitter_stddev );
        buff_duration = timespan_t::from_seconds( clamp( new_duration, 0.0, max_duration.total_seconds() ) );
      }

      if ( buff_duration > timespan_t::zero() )
      {
        proc_buff->trigger( buff_duration );
      }
    }
  };

  buff_t* buff = buff_t::find( effect.player, "anima_field" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "anima_field", effect.player->find_spell( 345535 ) )
      ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;

    new anima_field_emitter_proc_t( effect );
  }
}

void decanter_of_animacharged_winds( special_effect_t& effect )
{
  // TODO: "Damage is increased for each enemy struck, up to 5 enemies."
  struct splash_of_animacharged_wind_t : public shadowlands_aoe_proc_t
  {
    splash_of_animacharged_wind_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "splash_of_animacharged_wind", e.trigger(), true )
    {
      max_scaling_targets = e.driver()->effectN( 2 ).base_value() + 1;
    }
  };

  effect.execute_action = create_proc_action<splash_of_animacharged_wind_t>(
      "splash_of_animacharged_wind", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void bloodspattered_scale( special_effect_t& effect )
{
  struct blood_barrier_t : public shadowlands_aoe_proc_t
  {
    buff_t* absorb;

    blood_barrier_t( const special_effect_t& e, buff_t* absorb_ ) :
      shadowlands_aoe_proc_t( e, "blood_barrier", e.trigger(), true ), absorb( absorb_ )
    {
      max_scaling_targets = e.driver()->effectN( 2 ).base_value() + 1;
    }

    void execute() override
    {
      proc_spell_t::execute();

      absorb->trigger( 1, execute_state->result_amount * execute_state->n_targets );
    }
  };

  auto buff = buff_t::find( effect.player, "blood_barrier" );
  if ( !buff )
  {
    buff = make_buff<absorb_buff_t>( effect.player, "blood_barrier",
        effect.player->find_spell( 329849 ), effect.item )
      ->set_default_value( effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.execute_action = create_proc_action<blood_barrier_t>( "blood_barrier", effect, buff );
  }
}

void shadowgrasp_totem( special_effect_t& effect )
{
  struct shadowgrasp_totem_damage_t : public generic_proc_t
  {
    action_t* parent;

    shadowgrasp_totem_damage_t( const special_effect_t& effect ) :
      generic_proc_t( effect, "shadowgrasp_totem", 331537 ), parent( nullptr )
    {
      dot_duration = 0_s;
      base_dd_min = base_dd_max = player->find_spell( 329878 )->effectN( 1 ).average( effect.item );
    }

    void init_finished() override
    {
      generic_proc_t::init_finished();

      parent = player->find_action( "use_item_shadowgrasp_totem" );
    }
  };

  struct shadowgrasp_totem_buff_t : public buff_t
  {
    event_t* retarget_event;
    shadowgrasp_totem_damage_t* action;
    cooldown_t* item_cd;
    timespan_t cd_adjust;

    shadowgrasp_totem_buff_t( const special_effect_t& effect ) :
      buff_t( effect.player, "shadowgrasp_totem", effect.player->find_spell( 331537 ) ),
      retarget_event( nullptr ), action( new shadowgrasp_totem_damage_t( effect ) )
    {
      set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        action->set_target( action->parent->target );
        action->execute();
      } );

      item_cd = effect.player->get_cooldown( effect.cooldown_name() );
      cd_adjust = timespan_t::from_seconds(
          -source->find_spell( 329878 )->effectN( 3 ).base_value() );

      range::for_each( effect.player->sim->actor_list, [this]( player_t* t ) {
        t->register_on_demise_callback( source, [this]( player_t* actor ) {
          trigger_target_death( actor );
        } );
      } );
    }

    void reset() override
    {
      buff_t::reset();

      retarget_event = nullptr;
    }

    void trigger_target_death( const player_t* actor )
    {
      if ( !check() || !actor->is_enemy() || action->parent->target != actor )
      {
        return;
      }

      item_cd->adjust( cd_adjust );

      if ( !retarget_event && sim->shadowlands_opts.retarget_shadowgrasp_totem > 0_s )
      {
        retarget_event = make_event( sim, sim->shadowlands_opts.retarget_shadowgrasp_totem, [this]() {
          retarget_event = nullptr;

          // Retarget parent action to emulate player "retargeting" during Shadowgrasp
          // Totem duration to grab more 15 second cooldown reductions
          {
            auto new_target = action->parent->select_target_if_target();
            if ( new_target )
            {
              sim->print_debug( "{} action {} retargeting to a new target: {}",
                source->name(), action->parent->name(), new_target->name() );
              action->parent->set_target( new_target );
            }
          }
        } );
      }
    }
  };

  auto buff = buff_t::find( effect.player, "shadowgrasp_totem" );
  if ( !buff )
  {
    buff = make_buff<shadowgrasp_totem_buff_t>( effect );
    effect.custom_buff = buff;
  }
}

// TODO: Implement healing?
void hymnal_of_the_path( special_effect_t& effect )
{
  struct hymnal_of_the_path_t : public proc_spell_t
  {
    hymnal_of_the_path_t( const special_effect_t& e ) :
      proc_spell_t( "hymnal_of_the_path", e.player, e.player->find_spell( 348141 ) )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
    }
  };

  struct hymnal_of_the_path_cb_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      // XXX: Assume the actor always has more health than the target
      // TODO: Handle actor health < target health case?
      proc_action->set_target( target( state ) );
      proc_action->schedule_execute();
    }
  };

  effect.execute_action = create_proc_action<hymnal_of_the_path_t>(
      "hymnal_of_the_path", effect );

  new hymnal_of_the_path_cb_t( effect.player, effect );
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

void maw_rattle( special_effect_t& /* effect */ )
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

void third_eye_of_the_jailer( special_effect_t& /* effect */ )
{

}

void vitality_sacrifice( special_effect_t& /* effect */ )
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
    // Scopes
    unique_gear::register_special_effect( 321532, "329666trigger" ); // Infra-green Reflex Sight
    unique_gear::register_special_effect( 321533, "330038trigger" ); // Optical Target Embiggener

    // Trinkets
    unique_gear::register_special_effect( 347047, items::darkmoon_deck_putrescence );
    unique_gear::register_special_effect( 331624, items::darkmoon_deck_voracity );
    unique_gear::register_special_effect( 344686, items::stone_legion_heraldry );
    unique_gear::register_special_effect( 344806, items::cabalists_hymnal );
    unique_gear::register_special_effect( 345432, items::macabre_sheet_music );
    unique_gear::register_special_effect( 345319, items::glyph_of_assimilation );
    unique_gear::register_special_effect( 345251, items::soul_igniter );
    unique_gear::register_special_effect( 345019, items::skulkers_wing );
    unique_gear::register_special_effect( 344662, items::memory_of_past_sins );
    unique_gear::register_special_effect( 344063, items::gluttonous_spike );
    unique_gear::register_special_effect( 345357, items::hateful_chain );
    unique_gear::register_special_effect( 343385, items::overflowing_anima_cage );
    unique_gear::register_special_effect( 343393, items::sunblood_amethyst );
    unique_gear::register_special_effect( 345545, items::bottled_flayedwing_toxin );
    unique_gear::register_special_effect( 345539, items::empyreal_ordnance );
    unique_gear::register_special_effect( 345801, items::soulletting_ruby );
    unique_gear::register_special_effect( 345567, items::satchel_of_misbegotten_minions );
    unique_gear::register_special_effect( 330747, items::unbound_changeling );
    unique_gear::register_special_effect( 330767, items::unbound_changeling );
    unique_gear::register_special_effect( 330739, items::unbound_changeling );
    unique_gear::register_special_effect( 330740, items::unbound_changeling );
    unique_gear::register_special_effect( 330741, items::unbound_changeling );
    unique_gear::register_special_effect( 345490, items::infinitely_divisible_ooze );
    unique_gear::register_special_effect( 330323, items::inscrutable_quantum_device );
    unique_gear::register_special_effect( 345465, items::phial_of_putrefaction );
    unique_gear::register_special_effect( 345739, items::grim_codex );
    unique_gear::register_special_effect( 345533, items::anima_field_emitter );
    unique_gear::register_special_effect( 342427, items::decanter_of_animacharged_winds );
    unique_gear::register_special_effect( 329840, items::bloodspattered_scale );
    unique_gear::register_special_effect( 331523, items::shadowgrasp_totem );
    unique_gear::register_special_effect( 348135, items::hymnal_of_the_path );
    unique_gear::register_special_effect( 330067, items::mistcaller_ocarina );
    unique_gear::register_special_effect( 332299, items::mistcaller_ocarina );
    unique_gear::register_special_effect( 332300, items::mistcaller_ocarina );
    unique_gear::register_special_effect( 332301, items::mistcaller_ocarina );

    // Runecarves
    unique_gear::register_special_effect( 338477, items::echo_of_eonar );
    unique_gear::register_special_effect( 339344, items::judgment_of_the_arbiter );
    unique_gear::register_special_effect( 340197, items::maw_rattle );
    unique_gear::register_special_effect( 339340, items::norgannons_sagacity );
    unique_gear::register_special_effect( 339348, items::sephuzs_proclamation );
    unique_gear::register_special_effect( 339058, items::third_eye_of_the_jailer );
    unique_gear::register_special_effect( 338743, items::vitality_sacrifice );

    // Disabled effects
    unique_gear::register_special_effect( 329028, items::DISABLED_EFFECT ); // Light-Infused Armor shield
    unique_gear::register_special_effect( 333885, items::DISABLED_EFFECT ); // Darkmoon Deck: Putrescence shuffler
    unique_gear::register_special_effect( 329446, items::DISABLED_EFFECT ); // Darkmoon Deck: Voracity shuffler
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
    if ( unique_gear::find_special_effect( td->source, 347047 ) )
    {
      assert( !td->debuff.putrid_burst );

      td->debuff.putrid_burst = make_buff<buff_t>( *td, "putrid_burst", td->source->find_spell( 334058 ) )
        ->set_cooldown( 0_ms );
      td->debuff.putrid_burst->reset();
    }
    else
      td->debuff.putrid_burst = make_buff( *td, "putrid_burst" )->set_quiet( true );
  } );

  // Memory of Past Sins
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 344662 ) )
    {
      assert( !td->debuff.shattered_psyche );

      td->debuff.shattered_psyche =
          make_buff<buff_t>( *td, "shattered_psyche_debuff", td->source->find_spell( 344663 ) )
              ->set_default_value( td->source->find_spell( 344662 )->effectN( 1 ).percent() );
      td->debuff.shattered_psyche->reset();
    }
    else
      td->debuff.shattered_psyche = make_buff( *td, "shattered_psyche_debuff" )->set_quiet( true );
  } );
}

}  // namespace shadowlands
}  // namespace unique_gear
