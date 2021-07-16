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
#include "class_modules/monk/sc_monk.hpp"

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
  std::unique_ptr<SL_darkmoon_deck_t> deck;

  SL_darkmoon_deck_proc_t( const special_effect_t& e, util::string_view n, unsigned shuffle_id,
                           std::initializer_list<unsigned> card_list )
    : proc_spell_t( n, e.player, e.trigger(), e.item ), deck()
  {
    auto shuffle = unique_gear::find_special_effect( player, shuffle_id );
    if ( !shuffle )
      return;

    deck = std::make_unique<SL_darkmoon_deck_t>( *shuffle, card_list );
    deck->initialize();
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

      buff = debug_cast<stat_buff_t*>( buff_t::find( player, "voracious_haste" ) );
      if ( !buff )
      {
        buff = make_buff<stat_buff_t>( player, "voracious_haste", e.driver()->effectN( 2 ).trigger(), item )
          ->add_stat( STAT_HASTE_RATING, 0 );
      }
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
    cooldown_t* shared_cd;
    bool is_precombat;

    soul_ignition_buff_t( special_effect_t& e, action_t* d, cooldown_t* cd ) :
      buff_t( e.player, "soul_ignition", e.player->find_spell( 345211 ) ),
      effect( e ),
      damage_action( debug_cast<blazing_surge_t*>( d ) ),
      shared_cd( cd ),
      is_precombat()
    {}

    void expire_override( int stacks, timespan_t remaining_duration ) override
    {
      // If the trinket was used in precombat, assume that it was timed so
      // that it will expire to deal full damage when it first expires.
      if ( is_precombat )
      {
        shared_cd->adjust( -remaining_duration );
        remaining_duration = 0_ms;
      }

      buff_t::expire_override( stacks, remaining_duration );

      damage_action->buff_fraction_elapsed = ( buff_duration() - remaining_duration ) / buff_duration();
      damage_action->set_target( source->target );
      damage_action->execute();
      // the 60 second cooldown associated with the damage effect trigger
      // does not appear in spell data anywhere and is just in the tooltip.
      effect.execute_action->cooldown->start( effect.execute_action, 60_s );
    }
  };

  struct soul_ignition_t : public proc_spell_t
  {
    soul_ignition_buff_t* buff;
    cooldown_t* shared_cd;
    bool has_precombat_action;

    soul_ignition_t( const special_effect_t& e, cooldown_t* cd ) :
      proc_spell_t( "soul_ignition", e.player, e.driver() ),
      shared_cd( cd ),
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
      proc_spell_t::init_finished();
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
        buff->is_precombat = !player->in_combat && has_precombat_action;
        buff->trigger();
        shared_cd->start( 30_s );
      }
    }
  };

  auto category_cd = effect.player->get_cooldown( "item_cd_" + util::to_string( effect.player->find_spell( 345211 )->category() ) );
  auto damage_action = create_proc_action<blazing_surge_t>( "blazing_surge", effect );
  effect.execute_action = create_proc_action<soul_ignition_t>( "soul_ignition", effect, category_cd );
  auto buff = buff_t::find( effect.player, "soul_ignition" );
  if ( !buff )
    make_buff<soul_ignition_buff_t>( effect, damage_action, category_cd );
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

      // Assume we get about half of the value from each application of the extra ally stacks (4 allies = 2 stacks on player's first hit, 22 stacks on player's last hit etc.)
      int base_dmg_stacks = td->debuff.shattered_psyche->check() * (player->sim->shadowlands_opts.shattered_psyche_allies + 1);
      int bonus_dmg_stacks = player->sim->shadowlands_opts.shattered_psyche_allies / 2;
      m *= 1.0 + ( base_dmg_stacks + bonus_dmg_stacks ) * td->debuff.shattered_psyche->check_value();
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

    void execute( action_t*, action_state_t* trigger_state ) override
    {
      if ( buff->check() )
      {
        damage->set_target( trigger_state->target );
        damage->execute();
        buff->decrement();
      }
    }
  };

  auto buff = buff_t::find( effect.player, "shattered_psyche" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "shattered_psyche", effect.driver() );
    buff->set_initial_stack( buff->max_stack() );
    buff->set_cooldown( 0_s );
  }

  action_t* damage = create_proc_action<shattered_psyche_damage_t>( "shattered_psyche", effect );

  effect.custom_buff = buff;
  effect.disable_action();

  auto cb_driver = new special_effect_t( effect.player );
  cb_driver->name_str = "shattered_psyche_driver";
  cb_driver->spell_id = 344662;
  cb_driver->cooldown_ = 0_s;
  cb_driver->proc_flags_ = effect.driver()->proc_flags();
  cb_driver->proc_flags2_ = PF2_CAST_DAMAGE; // Only triggers from damaging casts
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

  timespan_t precast = effect.player->sim->shadowlands_opts.memory_of_past_sins_precast;
  if (precast > 0_s) {
    effect.player->register_combat_begin( [&effect, buff, precast]( player_t* ) {
      buff->trigger( buff->buff_duration() - precast );

      cooldown_t* cd = effect.player->get_cooldown( effect.cooldown_name() );
      cd->start( effect.cooldown() - precast );

      cooldown_t* group_cd = effect.player->get_cooldown( effect.cooldown_group_name());
      group_cd->start(effect.cooldown_group_duration() - precast);
    } );
  }
}


/** Gluttonous spike
 * id=344153 single target proc, proccing aura 344154
 * id=344154 15s aura, with 5 ticks of 344155
 * id=344155 aoe proc, increases damage up to 5 targets
 */
void gluttonous_spike( special_effect_t& effect )
{
  struct gluttonous_spike_pulse_t : public shadowlands_aoe_proc_t
  {
    gluttonous_spike_pulse_t( const special_effect_t& e ) :
      shadowlands_aoe_proc_t( e, "gluttonous_spike_pulse", e.player->find_spell( 344155 ) )
    {
      background = true;
      // The pulse damage comes from the driver
      base_dd_min = base_dd_max = e.driver()->effectN( 1 ).average( e.item );
    }
  };

  struct gluttonous_spike_buff_t : public buff_t
  {
    gluttonous_spike_pulse_t* pulse;
    gluttonous_spike_buff_t( const special_effect_t& e, action_t* action ) : buff_t( e.player, "gluttonous_spike", e.player->find_spell( 344154 ), e.item )
    {
      // Buff should never refresh itself since the trigger is disabled while it's up, but just in case
      set_refresh_behavior( buff_refresh_behavior::DISABLED );

      // Only applies the aura if you get overhealed, according to user configuration
      set_chance( player->sim->shadowlands_opts.gluttonous_spike_overheal_chance );

      // Triggers the given action for each tick
      set_tick_callback( [action]( buff_t* /* buff */, int /* current_tick */, timespan_t /* tick_time */ ) {
        action->execute();
      } );
    }
  };

  struct gluttonous_spike_t : public proc_spell_t
  {
    buff_t* buff;

    gluttonous_spike_t( const special_effect_t& e ) :
      proc_spell_t( "gluttonous_spike", e.player, e.trigger() )
    {
      // Create the pulse action and pass it to the buff
      action_t* pulse_action = create_proc_action<gluttonous_spike_pulse_t>( "gluttonous_spike_pulse", e );

      buff = buff_t::find(player, "gluttonous_spike");
      // Creates the buff if absent
      if (!buff)
      {
        buff = make_buff<gluttonous_spike_buff_t>( player, pulse_action );
      }

      // Main proc damage is in its own spell 344153, and not in the tooltip of the main spell
      base_dd_min = base_dd_max = e.player->find_spell( 344153 )->effectN( 1 ).average( e.item );

      if ( action_t* pulse = e.player->find_action( "gluttonous_spike_pulse" ) )
      {
        add_child( pulse );
      }
    }

    // Being overhealed by the proc applies the 344154 aura, which disables this proc while active
    void execute() override
    {
      // If the aura isn't currently running, execute the spell and triggers the buff
      if (!buff->check())
      {
        proc_spell_t::execute();
        buff->trigger();
      }
    }
  };

  effect.execute_action = create_proc_action<gluttonous_spike_t>( "gluttonous_spike", effect );
  new dbc_proc_callback_t( effect.player, effect );
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
        if ( e->spell_id == 330765 || e->spell_id == 330080 || e->spell_id == 330733 || e->spell_id == 330734 )
            return;
      }
      // Fallback, profile does not specify a stat-giving item bonus, so default to haste.
      effect.spell_id = 330733;
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

      may_crit = true;
      base_dd_min = p->find_spell( 345490 )->effectN( 1 ).min( e.item );
      base_dd_max = p->find_spell( 345490 )->effectN( 1 ).max( e.item );
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
    {
      npc_id = 175519;
    }

    void init_base_stats() override
    {
      pet_t::init_base_stats();

      resources.base[ RESOURCE_ENERGY ] = 100;
    }

    void init_assessors() override
    {
      pet_t::init_assessors();
      auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ) {
        if ( effect.player->specialization() == MONK_BREWMASTER || effect.player->specialization() == MONK_WINDWALKER ||
             effect.player->specialization() == MONK_MISTWEAVER )
        {
          monk::monk_t* monk_player = static_cast<monk::monk_t*>( owner );
          if ( !owner->bugs && monk_player->get_target_data( s->target )->debuff.bonedust_brew->up() )
            monk_player->bonedust_brew_assessor( s );
        }
        return assessor::CONTINUE;
      };

      assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
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
 * - heal spell: Triggers on self or a nearby target with less than 30% health remaining. This always crits.
 * - illusion: ??? (not tested yet, priority unknown)
 * - execute damage: Deal damage to the target if it is an enemy with less than 20% health remaining.
 *                   The 20% is not in spell data and this also always crits.
 * - healer mana: Triggers on a nearby healer with less than 20% mana. Higher priority than secondary
 *                stat buffs, but priority relative to other effects not tested.
 * - secondary stat buffs:
 *   - The secondary stat granted appears to be randomly selected from stat from the player's two highest
 *     secondary stats in terms of rating. When selecting the largest stats, the priority of equal secondary
 *     stats seems to be Vers > Mastery > Haste > Crit. There is a bug where the second stat selected must
 *     have a lower rating than the first stat that was selected. If this is not possible, then the second
 *     stat will be empty and the trinket will have a chance to do nothing when used.
 *   - If a Bloodlust buff is up, the stat buff will last 25 seconds instead of the default 20 seconds.
 *     Additionally, instead of randomly selecting from the player's two highest stats it will always
 *     grant the highest stat.
 *     TODO: Look for other buffs that also cause a bonus duration to occur. The spell data still lists
 *     a 30 second buff duration, so it is possible that there are other conditions that give 30 seconds.
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

    double composite_crit_chance() const override
    {
      double cc = proc_spell_t::composite_crit_chance() + 1;

      return cc;
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

      if ( target->health_percentage() <= 20 && !player->sim->shadowlands_opts.disable_iqd_execute )
      {
        execute_damage->set_target( target );
        execute_damage->execute();
      }
      else
      {
        stat_e s1 = STAT_NONE;
        stat_e s2 = STAT_NONE;
        buff_t* buff;
        timespan_t duration_adjustment;

        s1 = util::highest_stat( player, ratings );

        if ( is_buff_extended() )
        {
          buff = buffs[ s1 ];
          duration_adjustment = 5_s;
        }
        else
        {
          if ( rng().roll( sim->shadowlands_opts.iqd_stat_fail_chance ) )
            return;
          for ( auto s : ratings )
          {
            auto v = util::stat_value( player, s );
            if ( ( s2 == STAT_NONE || v > util::stat_value( player, s2 ) ) &&
                 ( ( player->bugs && v < util::stat_value( player, s1 ) ) || ( !player->bugs && s != s1 ) ) )
              s2 = s;
          }
          buff = rng().roll( 0.5 ) ? buffs[ s1 ] : buffs[ s2 ];
          duration_adjustment = 10_s;
        }

        if ( buff )
          buff->trigger( buff->buff_duration() - duration_adjustment );
      }
    }
  };

  effect.execute_action = create_proc_action<inscrutable_quantum_device_t>( "inscrutable_quantum_device", effect );
  effect.disable_buff();
  effect.stat = STAT_ALL;
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
      max_scaling_targets = as<unsigned>( e.driver()->effectN( 2 ).base_value() + 1 );
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
      max_scaling_targets = as<unsigned>( e.driver()->effectN( 2 ).base_value() + 1 );
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

/**Shadowgrasp Totem
 * id=331523 driver with totem summon
 * id=331532 periodic dummy trigger (likely handling the retargeting) controls the tick rate
 * id=329878 scaling spell containing CDR, damage, and healing dummy parameters
 *           effect #1: scaled damage
 *           effect #2: scaled healing
 *           effect #3: CDR on target kill (in seconds)
 * id=331537 damage spell and debuff with death proc trigger
 */
void shadowgrasp_totem( special_effect_t& effect )
{
  struct shadowgrasp_totem_damage_t : public spell_t
  {
    shadowgrasp_totem_damage_t( pet_t* p, const special_effect_t& effect ) :
      spell_t( "shadowgrasp_totem", p, p->owner->find_spell( 331537 ) )
    {
      dot_duration = 0_s;
      base_dd_min = base_dd_max = p->owner->find_spell( 329878 )->effectN( 1 ).average( effect.item );
    }
  };

  struct shadowgrasp_totem_pet_t : public pet_t
  {
    const special_effect_t& effect;

    shadowgrasp_totem_damage_t* damage;

    shadowgrasp_totem_pet_t( const special_effect_t& e ) :
      pet_t( e.player->sim, e.player, "shadowgrasp_totem", true, true),
      effect( e )
    {
      npc_id = 170190;
    }

    void init_spells() override
    {
      pet_t::init_spells();

      damage = new shadowgrasp_totem_damage_t ( this, effect );
    }

    void init_assessors() override
    {
      pet_t::init_assessors();
      auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ) {
        if ( effect.player->specialization() == MONK_BREWMASTER || effect.player->specialization() == MONK_WINDWALKER ||
             effect.player->specialization() == MONK_MISTWEAVER  )
        {
          monk::monk_t* monk_player = static_cast<monk::monk_t*>( owner );
          if ( !owner->bugs && monk_player->get_target_data( s->target )->debuff.bonedust_brew->up() )
            monk_player->bonedust_brew_assessor( s );
        }
        return assessor::CONTINUE;
      };

      assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
    }
  };

  struct shadowgrasp_totem_buff_t : public buff_t
  {
    spawner::pet_spawner_t<shadowgrasp_totem_pet_t> spawner;
    cooldown_t* item_cd;
    timespan_t cd_adjust;
    timespan_t last_retarget;

    shadowgrasp_totem_buff_t( const special_effect_t& effect ) :
      buff_t( effect.player, "shadowgrasp_totem", effect.player->find_spell( 331537 ) ),
      spawner( "shadowgrasp_totem", effect.player, [ &effect ]( player_t* )
        { return new shadowgrasp_totem_pet_t( effect ); } )
    {
      spawner.set_default_duration( effect.player->find_spell( 331537 )->duration() );

      // Periodic trigger in spell 331532 itself is hasted, which appears to control the tick rate
      set_tick_time_behavior( buff_tick_time_behavior::HASTED );
      set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
        auto pet = spawner.pet();
        if ( !pet )
          return;

        pet->damage->set_target( player->target );
        pet->damage->execute();
      } );

      item_cd = effect.player->get_cooldown( effect.cooldown_name() );
      cd_adjust = timespan_t::from_seconds(
        -source->find_spell( 329878 )->effectN( 3 ).base_value() );

      range::for_each( effect.player->sim->actor_list, [ this ]( player_t* t ) {
        t->register_on_demise_callback( source, [ this ]( player_t* actor ) {
          trigger_target_death( actor );
        } );
      } );
    }

    void reset() override
    {
      buff_t::reset();

      last_retarget = 0_s;
    }

    bool trigger( int stacks, double value, double chance, timespan_t duration) override
    {
      spawner.spawn();
      return buff_t::trigger(stacks, value, chance, duration);
    }

    void trigger_target_death( const player_t* actor )
    {
      auto pet = spawner.pet();
      if ( !pet || !actor->is_enemy() )
      {
        return;
      }


      if ( pet->target != actor )
      {
        auto retarget_time = sim->shadowlands_opts.retarget_shadowgrasp_totem;
        if ( retarget_time == 0_s || sim->current_time() - last_retarget < retarget_time )
          return;
        //  Emulate player "retargeting" during Shadowgrasp Totem duration to grab more
        //  15 second cooldown reductions.
        //  Don't actually change the target of the ability, as it will always hit whatever the
        //  player is hitting.
        last_retarget = sim->current_time();
      }

      item_cd->adjust( cd_adjust );
    }
  };

  auto buff = buff_t::find( effect.player, "shadowgrasp_totem" );
  if ( !buff ) {
    buff = make_buff<shadowgrasp_totem_buff_t>( effect );
  }
  effect.custom_buff = buff;
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

void overwhelming_power_crystal( special_effect_t& effect)
{
  // automagic handles almost everything, just need to disable the damage action
  // so the automagic doesn't incorrectly apply it to the target
  effect.disable_action();
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

void flame_of_battle( special_effect_t& effect )
{
  debug_cast<stat_buff_t*>( effect.create_buff() )->stats[ 0 ].amount =
      effect.player->find_spell( 346746 )->effectN( 1 ).average( effect.item );
}

// id=336182 driver, effect#2 has multiplier
// id=336183 damage spell, effect#1 value seems to be overridden by driver effect#1 value
void tablet_of_despair( special_effect_t& effect )
{
  struct burst_of_despair_t : public proc_spell_t
  {
    // each tick has a multiplier of 1.5^(tick #) with tick on application being tick #0
    double tick_factor;
    int tick_number;

    burst_of_despair_t( const special_effect_t& e )
      : proc_spell_t( "burst_of_despair", e.player, e.player->find_spell( 336183 ) ),
        tick_factor( e.driver()->effectN( 2 ).base_value() ),
        tick_number( 0 )
    {
      base_dd_min = base_dd_max = e.trigger()->effectN( 1 ).average( e.item );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double am = proc_spell_t::composite_da_multiplier( s );

      am *= std::pow( tick_factor, tick_number );

      return am;
    }
  };

  struct growing_despair_t : public proc_spell_t
  {
    burst_of_despair_t* burst;

    growing_despair_t( const special_effect_t& e ) : proc_spell_t( e ), burst( nullptr )
    {
      tick_action = create_proc_action<burst_of_despair_t>( "burst_of_despair", e );
      burst = debug_cast<burst_of_despair_t*>( tick_action );
    }

    void tick( dot_t* d ) override
    {
      burst->tick_number = d->current_tick;

      proc_spell_t::tick( d );
    }
  };

  effect.execute_action = create_proc_action<growing_despair_t>( "growing_despair", effect );
}

// id=329536 driver, damage value in effect#2
// id=329540 unknown use, triggered by driver
// id=329548 damage spell
void rotbriar_sprout( special_effect_t& effect )
{
  struct rotbriar_sprout_t : public shadowlands_aoe_proc_t
  {
    rotbriar_sprout_t( const special_effect_t& e ) : shadowlands_aoe_proc_t( e, "rotbriar_sprout", 329548, true )
    {
      base_dd_min = e.driver()->effectN( 2 ).min( e.item );
      base_dd_max = e.driver()->effectN( 2 ).max( e.item );
    }
  };

  effect.execute_action = create_proc_action<rotbriar_sprout_t>( "rotbriar_sprout", effect );

  new dbc_proc_callback_t( effect.player, effect );
}

// id=339343 driver, buff & debuff rating values
// id=339342 buff
// id=339341 debuff
void murmurs_in_the_dark( special_effect_t& effect )
{
  auto debuff = buff_t::find( effect.player, "end_of_night" );
  if ( !debuff )
  {
    debuff = make_buff<stat_buff_t>( effect.player, "end_of_night", effect.player->find_spell( 339341 ) )
      ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );
  }

  auto buff = buff_t::find( effect.player, "fall_of_night" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "fall_of_night", effect.player->find_spell( 339342 ) )
      ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) )
      ->set_stack_change_callback( [ debuff ]( buff_t*, int, int new_ ) {
        if ( !new_ )
          debuff->trigger();
      } );
  }

  effect.custom_buff = buff;
  new dbc_proc_callback_t( effect.player, effect );
}


// 9.1 Trinkets

// id=356029 buff
// id=353492 driver
void forbidden_necromantic_tome( special_effect_t& effect )  // NYI: Battle Rezz Ghoul
{
  auto buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "forbidden_knowledge" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "forbidden_knowledge", effect.player -> find_spell( 356029 ))
           ->add_stat( STAT_CRIT_RATING, effect.driver() ->effectN( 1 ).average( effect.item ) );
    
    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

void soul_cage_fragment( special_effect_t& effect )
{
  auto buff = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "torturous_might" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "torturous_might", effect.player->find_spell( 357672 ) )
           ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

void decanter_of_endless_howling( special_effect_t& effect )
{
  effect.proc_flags2_ = PF2_CRIT;
  auto buff           = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "decanted_warsong" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "decanted_warsong", effect.player->find_spell( 356687 ) )
               ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI ),
                           effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;
    new dbc_proc_callback_t( effect.player, effect );
  }
}

void tome_of_monstrous_constructions( special_effect_t& effect ) // TODO: Create buff that tracks the name of the mob, and only trigger on mobs with the same name.
{
  // Assume the player keeps the buff up on its own as the item is off-gcd and
  // the buff has a 60 minute duration which is enough for any encounter.
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct tome_of_monstrous_constructions_t : public proc_spell_t
  {
    tome_of_monstrous_constructions_t( const special_effect_t& e )
      : proc_spell_t( "studious_comprehension", e.player, e.player -> find_spell( 357168 ), e.item )
    {
      base_dd_min = e.player -> find_spell( 357169 ) -> effectN( 1 ).min( e.item );
      base_dd_max = e.player-> find_spell( 357169 ) -> effectN( 1 ).max( e.item );
    }
  };

  auto secondary      = new special_effect_t( effect.item );
  secondary->type     = SPECIAL_EFFECT_EQUIP;
  secondary->source   = SPECIAL_EFFECT_SOURCE_ITEM;
  secondary->spell_id = 357163;
  secondary->execute_action =
      create_proc_action<tome_of_monstrous_constructions_t>( "studious_comprehension", *secondary );
  effect.player->special_effects.push_back( secondary );

  auto callback = new dbc_proc_callback_t( effect.item, *secondary );
  callback->initialize();
}

void tormentors_rack_fragment( special_effect_t& effect )
{
  struct excruciating_twinge_t : public proc_spell_t
  {
    excruciating_twinge_t( const special_effect_t& e )
      : proc_spell_t( "excruciating_twinge", e.player, e.player->find_spell( 356181 ), e.item )
    {
      base_td = e.driver()->effectN( 1 ).average( e.item );
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t duration ) const override
    {
      return dot->time_to_next_tick() + duration;
    }
  };

  struct tormentors_rack_fragment_callback_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      proc_action->target = target( state );
      proc_action->schedule_execute();
    }
  };

  auto p    = effect.player;
  auto buff = make_buff<stat_buff_t>( p, "shredded_soul", p->find_spell( 356281 ) );
  buff->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );

  range::for_each( p->sim->actor_list, [ p, buff ]( player_t* t ) {
    if ( !t->is_enemy() )
      return;

    t->register_on_demise_callback( p, [ p, buff ]( player_t* t ) {
      if ( p->sim->event_mgr.canceled )
        return;

      auto d = t->get_dot( "excruciating_twinge", p );
      if ( d->remains() > 0_ms )
        buff->trigger(); // Going to assume that the player picks this up automatically, might need to add delay.
    } );
  } );

  effect.execute_action = create_proc_action<excruciating_twinge_t>( "excruciating_twinge", effect );

  new tormentors_rack_fragment_callback_t( effect.player, effect );
}

void old_warriors_soul( special_effect_t& effect )
{
  auto buff   = debug_cast<stat_buff_t*>( buff_t::find( effect.player, "undying_rage" ) );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "undying_rage", effect.player->find_spell( 356492 ) )
               ->add_stat( STAT_HASTE_RATING, effect.driver()->effectN( 1 ).average( effect.item ) );

    effect.custom_buff = buff;
    effect.player->register_combat_begin( [ &effect, buff ]( player_t* ) {
      buff->trigger();
      make_repeating_event( buff->source->sim, effect.player->find_spell( 356490 )->effectN( 1 ).period(),
                            [ buff ]() { buff->trigger(); } );
    } );
  }
}

void salvaged_fusion_amplifier( special_effect_t& effect)
{
  struct salvaged_fusion_amplifier_damage_t : public generic_proc_t
  {
    salvaged_fusion_amplifier_damage_t( const special_effect_t& e ) :
      generic_proc_t( e, "fusion_amplification", 355605 )
    {
      base_dd_min = e.driver() -> effectN( 1 ).average( e.item );
      base_dd_max = e.driver() -> effectN( 1 ).average( e.item );
    }
  };

  struct salvaged_fusion_amplifier_cb_t : public dbc_proc_callback_t
  {
    salvaged_fusion_amplifier_damage_t* damage;
    buff_t* buff;

    salvaged_fusion_amplifier_cb_t( const special_effect_t& effect, action_t* d, buff_t* b )
      : dbc_proc_callback_t( effect.player, effect ),
        damage( debug_cast<salvaged_fusion_amplifier_damage_t*>( d ) ),
        buff( b )
    {
    }

    void execute( action_t*, action_state_t* trigger_state ) override
    {
      if ( buff->check() )
      {
        damage->set_target( trigger_state->target );
        damage->execute();
      }
    }
  };

  auto buff = buff_t::find( effect.player, "salvaged_fusion_amplifier" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "salvaged_fusion_amplifier", effect.driver() );
    // The actual buff needs to always trigger
    buff->set_chance( 1.0 );
  }

  action_t* damage = create_proc_action<salvaged_fusion_amplifier_damage_t>( "fusion_amplification", effect );

  effect.custom_buff = buff;
  effect.disable_action();

  // since the on-use effect doesn't use the rppm, set to 0 so trinket expressions correctly determine it has a cooldown
  effect.ppm_ = 0;

  auto cb_driver          = new special_effect_t( effect.player );
  cb_driver->name_str     = "salvaged_fusion_amplifier_driver";
  cb_driver->spell_id     = 355333;
  cb_driver->cooldown_    = 0_s;
  cb_driver->proc_flags_  = effect.driver()->proc_flags();
  cb_driver->proc_flags2_ = PF2_ALL_HIT; // As this triggers from white+yellow hits not just casts, use ALL_HIT
  effect.player->special_effects.push_back( cb_driver );

  [[maybe_unused]] auto callback = new salvaged_fusion_amplifier_cb_t( *cb_driver, damage, buff );

  timespan_t precast = effect.player->sim->shadowlands_opts.salvaged_fusion_amplifier_precast;
  if ( precast > 0_s )
  {
    effect.player->register_combat_begin( [ &effect, buff, precast ]( player_t* ) {
      buff->trigger( buff->buff_duration() - precast );

      cooldown_t* cd = effect.player->get_cooldown( effect.cooldown_name() );
      cd->start( effect.cooldown() - precast );

      cooldown_t* group_cd = effect.player->get_cooldown( effect.cooldown_group_name() );
      group_cd->start( effect.cooldown_group_duration() - precast );
    } );
  }
}

void miniscule_mailemental_in_an_envelope( special_effect_t& effect )
{
  struct unstable_goods_t : public proc_spell_t
  {
    unstable_goods_t( const special_effect_t& e )
      : proc_spell_t( "unstable_goods", e.player, e.player->find_spell( 352542 ) )
    {
      base_dd_min = e.driver()->effectN( 1 ).min( e.item );
      base_dd_max = e.driver()->effectN( 1 ).max( e.item );
    }
  };

  struct unstable_goods_callback_t : public dbc_proc_callback_t
  {
    using dbc_proc_callback_t::dbc_proc_callback_t;

    void execute( action_t*, action_state_t* state ) override
    {
      if ( state->target->is_sleeping() )
        return;

      proc_action->target = target( state );
      proc_action->schedule_execute();
    }
  };

  effect.execute_action = create_proc_action<unstable_goods_t>( "unstable_goods", effect );

  new unstable_goods_callback_t( effect.player, effect );
}

/**Titanic Ocular Gland
 * id=355313 coefficients for stat amounts
 * id=355794 "Worthy" buff
 * id=355951 "Unworthy" debuff
 */
void titanic_ocular_gland( special_effect_t& effect )
{
  // When selecting the highest stat, the priority of equal secondary stats is Vers > Mastery > Haste > Crit.
  static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING, STAT_HASTE_RATING, STAT_CRIT_RATING };

  // Use a separate buff for each rating type so that individual uptimes are reported nicely and APLs can easily reference them.
  // Store these in pointers to reduce the size of the events that use them.
  auto worthy_buffs = std::make_shared<std::map<stat_e, buff_t*>>();
  auto unworthy_buffs = std::make_shared<std::map<stat_e, buff_t*>>();
  double amount = effect.driver()->effectN( 1 ).average( effect.item );

  for ( auto stat : ratings )
  {
    auto name = std::string( "worthy_" ) + util::stat_type_string( stat );
    buff_t* buff = buff_t::find( effect.player, name );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( 355794 ), effect.item )
               ->add_stat( stat, amount )
               ->set_can_cancel( false );
    }
    ( *worthy_buffs )[ stat ] = buff;

    name = std::string( "unworthy_" ) + util::stat_type_string( stat );
    buff = buff_t::find( effect.player, name );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, name, effect.player->find_spell( 355951 ), effect.item )
               ->add_stat( stat, -amount )
               ->set_can_cancel( false );
    }
    ( *unworthy_buffs )[ stat ] = buff;
  }

  auto update_buffs = [p = effect.player, worthy_buffs, unworthy_buffs]() mutable
  {
    bool worthy = p->rng().roll( p->sim->shadowlands_opts.titanic_ocular_gland_worthy_chance );
    bool buff_active = false;
    stat_e max_stat = util::highest_stat( p, ratings );

    // Iterate over all of the buffs and expire any that should not be active. Only one buff is
    // active at a time, so this process only needs to continue until a single active buff is found.
    for ( auto stat : ratings )
    {
      if ( ( *worthy_buffs )[ stat ]->check() )
      {
        // The Worthy buff will update to a different stat if that stat becomes higher. 
        // TODO: The buff only changes if the other stat is actually higher, otherwise the previous highest stat is used.
        if ( max_stat != stat || !worthy )
        {
          ( *worthy_buffs )[ stat ]->expire();
          max_stat = util::highest_stat( p, ratings );
        }
        else
        {
          buff_active = true;
        }
        break;
      }
      if ( ( *unworthy_buffs )[ stat ]->check() )
      {
        // The Unworthy debuff will never update its stat.
        if ( worthy )
        {
          ( *unworthy_buffs )[ stat ]->expire();
          max_stat = util::highest_stat( p, ratings );
        }
        else
        {
          buff_active = true;
        }
        break;
      }
    }

    if ( !buff_active )
    {
      if ( worthy )
        ( *worthy_buffs )[ max_stat ]->execute();
      else
        ( *unworthy_buffs )[ max_stat ]->execute();
    }
  };

  // While above the health threshold, this trinket appears to check your secondary stats every 5.25 seconds.
  // Also use this as an interval for checking whether the player is above the health threshold to toggle between
  // Worthy and Unworthy. In game, the buffs will actually switch instantly as soon as the player's health changes.
  effect.player->register_combat_begin( [update_buffs]( player_t* p ) mutable
  {
    timespan_t period = 5.25_s;
    timespan_t first_update = p->rng().real() * period;
    update_buffs();
    make_event( p->sim, first_update, [period, update_buffs, p]() mutable
    {
      update_buffs();
      make_repeating_event( p->sim, period, update_buffs );
    } );
  } );
}

/**Ebonsoul Vise
 * id=355327 damage debuff
 * id=357785 crit pickup buff
 */
void ebonsoul_vise( special_effect_t& effect )
{
  struct ebonsoul_vise_t : public proc_spell_t
  {
    double max_duration_bonus;

    ebonsoul_vise_t( const special_effect_t& e )
      : proc_spell_t( "ebonsoul_vise", e.player, e.driver(), e.item ),
        max_duration_bonus( e.driver()->effectN( 3 ).percent() )
    {
      base_td = e.driver()->effectN( 1 ).average( e.item );
    }

    timespan_t composite_dot_duration( const action_state_t* state ) const override
    {
      auto target      = state->target;
      double bonus_mul = 1.0 - ( target->is_active() ? target->health_percentage() * 0.01 : 0.0 );

      timespan_t new_duration = dot_duration * ( 1 + max_duration_bonus * bonus_mul );

      return new_duration;
    }
  };

  player_t* player = effect.player;
  buff_t* buff     = make_buff<stat_buff_t>( player, "shredded_soul_ebonsoul_vise", player->find_spell( 357785 ) )
                     ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 2 ).average( effect.item ) );

  range::for_each( player->sim->actor_list, [ player, buff ]( player_t* target ) {
    if ( !target->is_enemy() )
      return;

    target->register_on_demise_callback( player, [ player, buff ]( player_t* target ) {
      if ( player->sim->event_mgr.canceled )
        return;

      dot_t* dot     = target->get_dot( "ebonsoul_vise", player );
      bool picked_up = player->rng().roll( player->sim->shadowlands_opts.shredded_soul_pickup_chance );

      // TODO: handle potential movement required to pick up the soul
      if ( dot->remains() > 0_ms && picked_up )
        buff->trigger();
    } );
  } );

  effect.execute_action = create_proc_action<ebonsoul_vise_t>( "ebonsoul_vise", effect );
}

/**Shadowed Orb of Torment
 * id=355321 driver
 * id=356326 mastery buff
 * id=356334 mastery buff value
 */
void shadowed_orb_of_torment( special_effect_t& effect )
{
  struct tormented_insight_channel_t : public proc_spell_t
  {
    buff_t* buff;
    action_t* use_action;  // if this exists, then we're prechanneling via the APL

    tormented_insight_channel_t( const special_effect_t& e, buff_t* b )
      : proc_spell_t( "tormented_insight", e.player, e.driver(), e.item ), buff( b ), use_action( nullptr )
    {
      // Override this so it doesn't trigger self-harm portion of the trinket
      base_td      = 0;
      effect       = &e;
      channeled    = true;
      hasted_ticks = harmful = false;

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

    void precombat_buff()
    {
      timespan_t time = sim->shadowlands_opts.shadowed_orb_of_torment_precombat_channel;

      if ( time == 0_ms )  // No global override, check for an override from an APL variable
      {
        for ( auto v : player->variables )
        {
          if ( v->name_ == "shadowed_orb_of_torment_precombat_channel" )
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
        time     = 2_s;  // base 2s channel for full effect
        auto apl = player->precombat_action_list;

        auto it = range::find( apl, use_action );
        if ( it == apl.end() )
        {
          sim->print_debug(
              "WARNING: Precombat /use_item for Shadowed Orb of Torment exists but not found in precombat APL!" );
          return;
        }

        cdgrp->start( 1_ms );  // tap the shared group cd so we can get accurate action_ready() checks

        // add cast time or gcd for any following precombat action
        std::for_each( it + 1, apl.end(), [ &time, this ]( action_t* a ) {
          if ( a->action_ready() )
          {
            timespan_t delta =
                std::max( std::max( a->base_execute_time, a->trigger_gcd ) * a->composite_haste(), a->min_gcd );
            sim->print_debug( "PRECOMBAT: Shadowed Orb of Torment prechannel timing pushed by {} for {}", delta,
                              a->name() );
            time += delta;

            return a->harmful;  // stop processing after first valid harmful spell
          }
          return false;
        } );
      }

      // how long you channel for (rounded down to seconds)
      auto channel = std::min( 2_s, timespan_t::from_seconds( static_cast<int>( time.total_seconds() ) ) );
      // total duration of the buff from channeling. Base is 10s for each tick of the channel (2 ticks per second)
      auto total = buff->buff_duration() * ( channel.total_seconds() * 2 );
      // actual duration of the buff you'll get in combat
      auto actual = total + channel - time;
      // cooldown on effect/trinket at start of combat
      auto cd_dur = cooldown->duration - time;
      // shared cooldown at start of combat
      auto cdgrp_dur = std::max( 0_ms, effect->cooldown_group_duration() - time );

      sim->print_debug(
          "PRECOMBAT: Shadowed Orb of Torment started {}s before combat via {}, channeled for {}s, {}s in-combat buff",
          time, use_action ? "APL" : "SHADOWLANDS_OPT", channel, actual );

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, actual );

      if ( use_action )  // from the apl, so cooldowns will be started by use_item_t. adjust. we are still in precombat.
      {
        make_event( *sim, [ this, time, cdgrp ] {  // make an event so we adjust after cooldowns are started
          cooldown->adjust( -time );

          if ( use_action )
            use_action->cooldown->adjust( -time );

          cdgrp->adjust( -time );
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

    timespan_t tick_time( const action_state_t* ) const override
    {
      return base_tick_time;
    }

    void trigger_dot( action_state_t* s ) override
    {
      if ( player->in_combat )  // only trigger channel 'dot' in combat
      {
        proc_spell_t::trigger_dot( s );
      }
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( player->in_combat )  // only channel in-combat
      {
        event_t::cancel( player->readying );
        player->reset_auto_attacks( data().duration() );
      }
      else  // if precombat...
      {
        if ( use_action )  // ...and use_item exists in the precombat apl
        {
          precombat_buff();
        }
      }
    }

    void last_tick( dot_t* d ) override
    {
      bool was_channeling = player->channeling == this;
      timespan_t duration = d->current_tick * buff->buff_duration();

      buff->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
      proc_spell_t::last_tick( d );

      if ( was_channeling && !player->readying )
      {
        player->schedule_ready( rng().gauss( sim->channel_lag, sim->channel_lag_stddev ) );
      }
    }
  };

  buff_t* buff = buff_t::find( effect.player, "tormented_insight" );
  if ( !buff )
  {
    double val = effect.player->find_spell( 356334 )->effectN( 1 ).average( effect.item );
    buff       = make_buff<stat_buff_t>( effect.player, "tormented_insight", effect.player->find_spell( 356326 ) )
               ->add_stat( STAT_MASTERY_RATING, val );
  }

  auto action           = new tormented_insight_channel_t( effect, buff );
  effect.execute_action = action;

  // pre-combat channeling hack via shadowlands options
  if ( effect.player->sim->shadowlands_opts.shadowed_orb_of_torment_precombat_channel > 0_ms )  // option is set
  {
    effect.player->register_combat_begin( [ action ]( player_t* ) {
      if ( !action->use_action )  // no use_item in precombat apl, so we apply the buff on combat start
        action->precombat_buff();
    } );
  }
}

/**Relic of the Frozen Wastes
  (355301) Relic of the Frozen Wastes (equip):
    chance to deal damage (effect 1) and apply debuff Frozen Heart (355759)
  (355303) Frostlord's Call (use):
    (355912, damage from equip effect 2) deals physical damage to target
    (357409, damage from equip effect 3) deals frost damage to all targets between the player and target inclusive with Frozen Heart debuff
 */
void relic_of_the_frozen_wastes_use( special_effect_t& effect )
{
  struct frost_tinged_carapace_spikes_t : public generic_proc_t
  {
    frost_tinged_carapace_spikes_t( const special_effect_t& effect, const spell_data_t* equip )
      : generic_proc_t( effect, "frosttinged_carapace_spikes", effect.player->find_spell( 357409 ) )
    {
      background = dual = true;
      callbacks = false;
      aoe = -1;
      base_dd_max = base_dd_min = equip->effectN( 3 ).average( effect.item );
    }

    void execute() override
    {
      target_cache.is_valid = false;

      generic_proc_t::execute();
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      tl.clear();

      for ( size_t i = 0, actors = sim->target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* t = sim->target_non_sleeping_list[ i ];
        if ( t->is_enemy() && player->get_target_data( t )->debuff.frozen_heart->up() )
          tl.push_back( t );
      }

      return tl.size();
    }
  };

  struct nerubian_ambush_t : public generic_proc_t
  {
    action_t* splash;

    nerubian_ambush_t( const special_effect_t& effect, const spell_data_t* equip ) :
      generic_proc_t( effect, "nerubian_ambush", effect.player->find_spell(355912) )
    {
      background = dual = true;
      callbacks = false;
      base_dd_max = base_dd_min = equip->effectN( 2 ).average( effect.item );
    }
  };

  struct frostlords_call_t : public generic_proc_t
  {
    action_t* ambush;
    action_t* spikes;

    frostlords_call_t( const special_effect_t& effect, const spell_data_t* equip )
      : generic_proc_t( effect, "frostlords_call", effect.driver() ),
        ambush( create_proc_action<nerubian_ambush_t>( "nerubian_ambush", effect, equip ) ),
        spikes( create_proc_action<frost_tinged_carapace_spikes_t>( "frosttinged_carapace_spikes", effect, equip ) )
    {
      callbacks = false;
      travel_speed = 20;

      add_child( ambush );
      add_child( spikes );
    }

    void impact( action_state_t* s ) override
    {
      generic_proc_t::impact( s );

      ambush->set_target( s->target );
      ambush->execute();

      spikes->set_target( s->target );
      spikes->execute();
    }
  };

  const spell_data_t* equip = effect.player->find_spell( 355301 );
  effect.execute_action = create_proc_action<frostlords_call_t>( "frostlords_call", effect, equip );
}

void relic_of_the_frozen_wastes_equip( special_effect_t& effect )
{
  struct frozen_heart_t : generic_proc_t
  {
    frozen_heart_t( const special_effect_t& effect ) : generic_proc_t( effect, "frozen_heart", effect.trigger() )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    }

    void execute() override
    {
      generic_proc_t::execute();

      actor_target_data_t* td = player->get_target_data( target );
      td->debuff.frozen_heart->trigger();
    }
  };

  effect.execute_action = create_proc_action<frozen_heart_t>( "frozen_heart", effect );
  new dbc_proc_callback_t( effect.player, effect );
}

/**Ticking Sack of Terror
  (351679) driver, damage on effect 1
  (351682) debuff
  (351694) fire damage at 3 stacks
 */
void ticking_sack_of_terror( special_effect_t& effect )
{
  struct volatile_detonation_t : generic_proc_t
  {
    volatile_detonation_t( const special_effect_t& effect )
      : generic_proc_t( effect, "volatile_detonation", effect.player->find_spell( 351694 ) )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
    }
  };

  struct volatile_satchel_cb_t : dbc_proc_callback_t
  {
    action_t* damage;

    volatile_satchel_cb_t( const special_effect_t& effect )
      : dbc_proc_callback_t( effect.player, effect ),
        damage( create_proc_action<volatile_detonation_t>( "volatile_detonation", effect ) )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      actor_target_data_t* td = a->player->get_target_data( s->target );

      // Damage is dealt when the debuff falls off *or* reaches max stacks, whichever comes first
      // This needs to be handled via a stack_change_callback, but can't be set in register_target_data_initializer
      if ( !td->debuff.volatile_satchel->stack_change_callback )
      {
        td->debuff.volatile_satchel->set_stack_change_callback( [ this ]( buff_t* b, int /* old_ */, int new_ ) {
          if ( new_ == 0 )
          {
            damage->set_target( b->player );
            damage->execute();
          }
        } );
      }

      if ( td->debuff.volatile_satchel->at_max_stacks() )
        td->debuff.volatile_satchel->expire();
      else
        td->debuff.volatile_satchel->trigger();
    }
  };

  new volatile_satchel_cb_t( effect );
}

// id=351927 hold stat amount
// id=351952 buff
// TODO: implement external buff to simulate being an ally
void soleahs_secret_technique( special_effect_t& effect )
{
  // Assuming you don't actually use this trinket during combat but rather beforehand
  effect.type = SPECIAL_EFFECT_EQUIP;

  auto opt_str = effect.player->sim->shadowlands_opts.soleahs_secret_technique_type;
  if ( util::str_compare_ci( opt_str, "none" ) )
    return;

  auto val = effect.player->find_spell( 351927 )->effectN( 1 ).average( effect.item );

  buff_t* buff;

  if ( util::str_compare_ci( opt_str, "haste" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_haste" );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_haste", effect.player->find_spell( 351952 ) )
              ->add_stat( STAT_HASTE_RATING, val );
    }
  }
  else if ( util::str_compare_ci( opt_str, "crit" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_crit" );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_crit", effect.player->find_spell( 351952 ) )
              ->add_stat( STAT_CRIT_RATING, val );
    }
  }
  else if ( util::str_compare_ci( opt_str, "versatility" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_versatility" );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_versatility", effect.player->find_spell( 351952 ) )
              ->add_stat( STAT_VERSATILITY_RATING, val );
    }
  }
  else if ( util::str_compare_ci( opt_str, "mastery" ) )
  {
    buff = buff_t::find( effect.player, "soleahs_secret_technique_mastery" );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "soleahs_secret_technique_mastery", effect.player->find_spell( 351952 ) )
              ->add_stat( STAT_MASTERY_RATING, val );
    }
  }
  else
  {
    buff = nullptr;
  }

  if ( buff )
    effect.player->register_combat_begin( buff );
  else
    effect.player->sim->error( "Warning: Invalid type '{}' for So'leah's Secret Technique, ignoring.", opt_str );
}

// Weapons

// id=331011 driver
// id=331016 DoT proc debuff
void poxstorm( special_effect_t& effect )
{
  // When dual-wielded, Poxstorm has one shared DoT which is duration refreshed
  // The damage is overwritten by the last proc'd version in the case of unequal item levels
  struct bubbling_pox_t : public proc_spell_t
  {
    bubbling_pox_t( const special_effect_t& effect )
      : proc_spell_t( "bubbling_pox", effect.player, effect.trigger(), effect.item )
    {
    }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t duration ) const override
    {
      return dot->time_to_next_tick() + duration;
    }
  };

  // Note, no create_proc_action here, since there is the possibility of dual-wielding them and
  // special_effect_t does not have enough support for defining "shared spells" on initialization
  effect.execute_action = new bubbling_pox_t( effect );

  new dbc_proc_callback_t( effect.player, effect );
}

void init_jaithys_the_prison_blade( special_effect_t& effect, int proc_id, int spell_id, int rank )
{
  // When same item levels are dual-wielded, Jaithys currently combines damage and str buff
  // Dev confirmed this is a bug and all procs should be independent
  action_t* harsh_tutelage_damage = create_proc_action<generic_proc_t>(  
      "harsh_tutelage", effect, "harsh_tutelage" + std::to_string( rank ), proc_id );

  harsh_tutelage_damage->base_dd_min = effect.player->find_spell( spell_id )->effectN( 1 ).average( effect.item );
  harsh_tutelage_damage->base_dd_max = effect.player->find_spell( spell_id )->effectN( 1 ).average( effect.item );

  effect.execute_action = harsh_tutelage_damage;

  // rank 2+ has a str buff
  if ( rank > 2 )
  {
    buff_t* buff = buff_t::find( effect.player, "harsh_tutelage" + std::to_string( rank ) );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "harsh_tutelage" + std::to_string( rank ),
                                  effect.player->find_spell( proc_id ) )
              ->add_stat( STAT_STRENGTH, effect.player->find_spell( spell_id )->effectN( 2 ).average( effect.item ) );
    }
    effect.custom_buff = buff;
  }

  new dbc_proc_callback_t( effect.player, effect );
}

void jaithys_the_prison_blade_1( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358564, 358562, 1 );
}

void jaithys_the_prison_blade_2( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358566, 358565, 2 );
}

void jaithys_the_prison_blade_3( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358568, 358567, 3 );
}
    
void jaithys_the_prison_blade_4( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358570, 358569, 4 );
}

void jaithys_the_prison_blade_5( special_effect_t& effect )
{
  init_jaithys_the_prison_blade( effect, 358572, 358571, 5 );
}

/**Yasahm the Riftbreaker
    351527 driver, proc on crit
    351531 trigger buff, damage on effect 1
    351561 damage proc on crit after 5th stack
  */
void yasahm_the_riftbreaker( special_effect_t& effect )
{
  struct preternatural_charge_t : public proc_spell_t
  {
    preternatural_charge_t( const special_effect_t& effect )
      : proc_spell_t( "preternatural_charge", effect.player, effect.player->find_spell( 351561 ), effect.item )
    {
      base_dd_min = base_dd_max = effect.trigger()->effectN( 1 ).average( effect.item );
    }
  };

  auto proc = create_proc_action<preternatural_charge_t>( "preternatural_charge", effect );
  auto buff   = buff_t::find( effect.player, "preternatural_charge" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "preternatural_charge", effect.trigger() )->set_max_stack( effect.trigger()->max_stacks() + 1 );
    buff->set_stack_change_callback( [ proc ]( buff_t* buff, [[maybe_unused]] int old, int cur ) {
      if ( cur == buff->max_stack() )
      {
        proc->set_target( buff->player->target );
        proc->execute();
        make_event( buff->sim, [ buff ] { buff->expire(); } );
      }
    } );
  }

  effect.custom_buff  = buff;
  effect.proc_flags2_ = PF2_CRIT;
  new dbc_proc_callback_t( effect.player, effect );
}

// TODO: Add proc restrictions to match the weapons or expansion options.
void cruciform_veinripper(special_effect_t& effect)
{

  struct sadistic_glee_t : public proc_spell_t
  {
    double scaled_dmg;

    sadistic_glee_t(const special_effect_t& e)
      : proc_spell_t("sadistic_glee", e.player, e.player->find_spell(353466), e.item),
        scaled_dmg(e.driver()->effectN(1).average(e.item))
    {
      base_td = scaled_dmg;
    }

    double base_ta(const action_state_t* /* s */) const override
    {
      return scaled_dmg;
    }

    // TODO: Confirm Dot Refresh Behaviour
    timespan_t calculate_dot_refresh_duration(const dot_t* dot, timespan_t duration) const override
    {
      return dot->time_to_next_tick() + duration;
    }
  };

  auto sadistic_glee = static_cast<sadistic_glee_t*>(effect.player->find_action("sadistic_glee"));

  if (!sadistic_glee)
    effect.execute_action = create_proc_action<sadistic_glee_t>("sadistic_glee", effect);
  else
    sadistic_glee->scaled_dmg += effect.driver()->effectN(1).average(effect.item);

  effect.spell_id = 357588;
  effect.rppm_modifier_ = 0.5;

  new dbc_proc_callback_t(effect.player, effect);
}


// Armor

/**Passably-Forged Credentials
 * id=352081 driver
 * id=352091 buff
 */
void passablyforged_credentials( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "passable_credentials" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "passable_credentials", effect.trigger() )
             ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ), effect.driver()->effectN( 1 ).average( effect.item ) );
  }

  effect.custom_buff = buff;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE | PF2_PERIODIC_HEAL;
  new dbc_proc_callback_t( effect.player, effect );
}

/**Dark Ranger's Quiver
    353513 driver, damage on effect 1
    353514 trigger buff
    353515 cleave damage at max stacks
 */
void dark_rangers_quiver( special_effect_t& effect )
{
  if ( effect.player->type != HUNTER )
    return;

  struct withering_fire_t : public proc_spell_t
  {
    withering_fire_t( const special_effect_t& effect ) : proc_spell_t( "withering_fire", effect.player, effect.player->find_spell( 353515 ), effect.item )
    {
      base_dd_min = base_dd_max = effect.driver()->effectN( 1 ).average( effect.item );
      aoe = data().max_targets();
    }
  };

  auto cleave = create_proc_action<withering_fire_t>( "withering_fire", effect );
  auto buff = buff_t::find( effect.player, "withering_fire" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "withering_fire", effect.trigger() );
    buff->set_stack_change_callback( [ cleave ]( buff_t* buff, int, int cur ) {
      if ( cur == buff->max_stack() )
      {
        cleave->set_target( buff->player->target );
        cleave->execute();
        make_event( buff->sim, [ buff ] { buff->expire(); } );
      }
    } );
  }

  effect.custom_buff = buff;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE;
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

// Helper function to determine whether a Rune Word is active. Returns rank of the lowest shard if found.
int rune_word_active( special_effect_t& effect, spell_label label )
{
  using id_rank_pair_t = std::pair<unsigned, unsigned>;
  static constexpr id_rank_pair_t ranks[] = {
    // Blood
    { 187057, 1},  // Shard of Bek
    { 187284, 2},
    { 187293, 3},
    { 187302, 4},
    { 187312, 5},
    { 187059, 1},  // Shard of Jas
    { 187285, 2},
    { 187294, 3},
    { 187303, 4},
    { 187313, 5},
    { 187061, 1},  // Shard of Rev
    { 187286, 2},
    { 187295, 3},
    { 187304, 4},
    { 187314, 5},
    // Frost
    { 187063, 1},  // Shard of Cor
    { 187287, 2},
    { 187296, 3},
    { 187305, 4},
    { 187315, 5},
    { 187065, 1},  // Shard of Kyr
    { 187288, 2},
    { 187297, 3},
    { 187306, 4},
    { 187316, 5},
    { 187071, 1},  // Shard of Tel
    { 187289, 2},
    { 187298, 3},
    { 187307, 4},
    { 187317, 5},
    // Unholy
    { 187073, 1},  // Shard of Dyz
    { 187290, 2},
    { 187299, 3},
    { 187308, 4},
    { 187318, 5},
    { 187076, 1},  // Shard of Oth
    { 187291, 2},
    { 187300, 3},
    { 187309, 4},
    { 187319, 5},
    { 187079, 1},  // Shard of Zed
    { 187292, 2},
    { 187301, 3},
    { 187310, 4},
    { 187320, 5},
  };

  if ( !effect.player->sim->shadowlands_opts.enable_rune_words )
  {
    effect.player->sim->print_debug( "{}: rune word {} from item {} is inactive by global override",
      effect.player->name(), effect.driver()->name_cstr(), effect.item->name() );
    return 0;
  }

  unsigned equipped_shards = 0;
  unsigned rank = 0;

  for ( const auto& item : effect.player->items )
  {
    // We cannot just check special effects because some of the Shard of Domination effects
    // are simple stat bonuses and a special effect with a spell ID will not be created.
    for ( auto gem_id : item.parsed.gem_id )
    {
      if ( gem_id == 0 )
        continue;

      const auto& gem = item.player->dbc->item( gem_id );
      if ( gem.id == 0 )
        throw std::invalid_argument( fmt::format( "No gem data for id {}.", gem_id ) );

      const gem_property_data_t& gem_prop = item.player->dbc->gem_property( gem.gem_properties );
      if ( !gem_prop.id )
        continue;

      if ( gem_prop.color != SOCKET_COLOR_SHARD_OF_DOMINATION )
        continue;

      const item_enchantment_data_t& enchant_data = item.player->dbc->item_enchantment( gem_prop.enchant_id );
      for ( size_t i = 0; i < range::size( enchant_data.ench_prop ); i++ )
      {
        switch ( enchant_data.ench_type[ i ] )
        {
          case ITEM_ENCHANTMENT_COMBAT_SPELL:
          case ITEM_ENCHANTMENT_EQUIP_SPELL:
          case ITEM_ENCHANTMENT_USE_SPELL:
          {
            auto spell = item.player->find_spell( enchant_data.ench_prop[ i ] );
            if ( spell->affected_by_label( label ) )
            {
              equipped_shards++;

              auto it = range::find( ranks, gem_id, &id_rank_pair_t::first );
              if ( it != range::end( ranks ) )
              {
                if ( rank == 0 )
                  rank = it->second;
                else
                  rank = std::min( rank, it->second );
              }
            }
            break;
          }
          default:
            break;
        }
      }
    }
  }

  int active = equipped_shards >= 3 ? as<int>( rank ) : 0;
  effect.player->sim->print_debug(
      "{}: rune word {} from item {} is {} (rank {}) with {}/3 shards of domination equipped", effect.player->name(),
      util::tokenize_fn( effect.driver()->name_cstr() ), effect.item->name(), active ? "active" : "inactive", rank,
      equipped_shards );

  return active;
}

/**Blood Link
 * id=357347 equip special effect
 * id=355761 driver and coefficient
 * id=355767 damage
 * id=355768 heal self
 * id=355769 heal other
 * id=355804 debuff
 */
void blood_link( special_effect_t& effect )
{
  auto rank = rune_word_active( effect, LABEL_SHARD_OF_DOMINATION_BLOOD );
  if ( rank == 0 )
    return;

  struct blood_link_damage_t : proc_spell_t
  {
    blood_link_damage_t( const special_effect_t& e, int rank ) :
      proc_spell_t( "blood_link", e.player, e.player->find_spell( 355767 ) )
    {
      base_dd_min = e.driver()->effectN( 2 ).min( e.player );
      base_dd_max = e.driver()->effectN( 2 ).max( e.player );
      base_dd_multiplier *= 1.0 + ( rank - 1 ) * 0.5;
    }
  };

  // Note, this DoT does not actually do any damage. This is effectively just a debuff,
  // except that it refreshses based on remaining time to tick like a DoT does.
  struct blood_link_dot_t : proc_spell_t
  {
    player_t* last_target;

    blood_link_dot_t( const special_effect_t& e )
      : proc_spell_t( "blood_link_dot", e.player, e.player->find_spell( 355804 ) ), last_target()
    {}

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t duration ) const override
    {
      return dot->time_to_next_tick() + duration;
    }

    void execute() override
    {
      proc_spell_t::execute();

      if ( hit_any_target )
      {
        if ( last_target && last_target != target )
          get_dot( last_target )->cancel();

        last_target = target;
      }
    }
  };

  blood_link_dot_t* dot_action = debug_cast<blood_link_dot_t*>( create_proc_action<blood_link_dot_t>( "blood_link", effect ) );
  effect.spell_id = 355761;
  auto buff = buff_t::find( effect.player, "blood_link" );
  if ( !buff )
  {
    auto tick_action = create_proc_action<blood_link_damage_t>( "blood_link_tick", effect, rank );
    buff = make_buff( effect.player, "blood_link", effect.driver() )
             ->set_quiet( true )
             ->set_can_cancel( false )
             ->set_tick_callback( [ tick_action, dot_action ] ( buff_t*, int, timespan_t )
               {
                  if ( dot_action->get_dot( dot_action->last_target )->is_ticking() )
                  {
                    tick_action->set_target( dot_action->last_target );
                    tick_action->execute();
                  }
               } );
  }

  effect.execute_action = dot_action;
  new dbc_proc_callback_t( effect.player, effect );
  effect.player->register_combat_begin( [ buff ]( player_t* p )
  {
    timespan_t first_tick = p->rng().real() * buff->tick_time();
    buff->set_period( first_tick );
    buff->trigger();
    buff->set_period( timespan_t::min() );
  } );
}

/**Winds of Winter
 * id=357348 equip special effect
 * id=355724 driver and damage cap coefficient
 * id=355733 damage
 * id=355735 absorb (NYI)
 */
void winds_of_winter( special_effect_t& effect )
{
  auto rank = rune_word_active( effect, LABEL_SHARD_OF_DOMINATION_FROST );
  if ( rank == 0 )
    return;

  struct winds_of_winter_damage_t : proc_spell_t
  {
    winds_of_winter_damage_t( const special_effect_t& e )
      : proc_spell_t( "winds_of_winter", e.player, e.player->find_spell( 355733 ) )
    {
      base_dd_min = base_dd_max = 1.0; // Ensure that the correct snapshot flags are set.
    }
  };

  struct winds_of_winter_cb_t : public dbc_proc_callback_t
  {
    double accumulated_damage;
    double damage_cap;
    double damage_fraction;
    action_t* damage;
    buff_t* buff;
    player_t* target;

    winds_of_winter_cb_t( const special_effect_t& e, action_t* a, buff_t* b, int rank ) :
      dbc_proc_callback_t( e.player, e ),
      accumulated_damage(),
      damage_cap( e.driver()->effectN( 1 ).average( e.player ) * ( 1.0 + ( rank - 1 ) * 0.5 ) ),
      damage_fraction( e.driver()->effectN( 2 ).percent() * ( 1.0 + ( rank - 1 ) * 0.5 ) ),
      damage( a ),
      buff( b ),
      target()
    {
      buff->set_tick_callback( [ this ] ( buff_t*, int, timespan_t )
      {
        if ( !target || target->is_sleeping() || accumulated_damage == 0 )
          return;

        damage->base_dd_min = damage->base_dd_max = accumulated_damage;
        accumulated_damage = 0;
        damage->set_target( target );
        damage->execute();
      } );
    }

    void execute( action_t*, action_state_t* s ) override
    {
      if ( s->target->is_sleeping() )
        return;

      accumulated_damage += std::min( damage_cap, s->result_amount * damage_fraction );
      target = s->target; // Winds of Winter hits the last target you accumulated damage from.
    }
  };

  effect.spell_id = 355724;
  effect.proc_flags2_ = PF2_CRIT;
  auto damage = create_proc_action<winds_of_winter_damage_t>( "winds_of_winter", effect );
  auto buff = buff_t::find( effect.player, "winds_of_winter" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "winds_of_winter", effect.driver() )
             ->set_quiet( true )
             ->set_can_cancel( false );
  }

  new winds_of_winter_cb_t( effect, damage, buff, rank );
  effect.player->register_combat_begin( [ buff ]( player_t* p )
  {
    timespan_t first_tick = p->rng().real() * buff->tick_time();
    buff->set_period( first_tick );
    buff->trigger();
    buff->set_period( timespan_t::min() );
  } );
}

/**Chaos Bane
 * id=357349 equip special effect
 * id=355829 driver with RPPM and buff trigger & coefficients
 *    effect#3 is soul fragment stat coefficient
 *    effect#4 is chaos bane stat coefficient
 *    effect#5 is chaso bane damage coefficient
 * id=356042 Soul Fragment buff
 * id=356043 Chaos Bane buff
 * id=356046 damage
 */
void chaos_bane( special_effect_t& effect )
{
  auto rank = rune_word_active( effect, LABEL_SHARD_OF_DOMINATION_UNHOLY );
  if ( rank == 0 )
    return;

  struct chaos_bane_t : proc_spell_t
  {
    buff_t* buff;
    chaos_bane_t( const special_effect_t& e, buff_t* b, int rank ) :
      proc_spell_t( "chaos_bane", e.player, e.player->find_spell( 356046 ) ),
      buff( b )
    {
      // coeff in driver eff#5
      base_dd_max = base_dd_min = e.driver()->effectN( 5 ).average( e.player );
      split_aoe_damage = true;
      base_dd_multiplier *= 1.0 + ( rank - 1 ) * 0.5;
    }

    void execute() override
    {
      buff->trigger();

      proc_spell_t::execute();
    }
  };

  struct chaos_bane_cb_t : public dbc_proc_callback_t
  {
    buff_t* chaos_bane;

    chaos_bane_cb_t( const special_effect_t& e, buff_t* b ) :
      dbc_proc_callback_t( e.player, e ),
      chaos_bane( b )
    {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( chaos_bane->check() )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  // driver with rppm is 355829. this spell also contains the various coefficients.
  effect.spell_id = 355829;

  auto buff = buff_t::find( effect.player, "chaos_bane" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "chaos_bane", effect.player->find_spell( 356043 ) )
      // coeff in driver eff#4
      ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ),
                  effect.driver()->effectN( 4 ).average( effect.player ) * ( 1.0 + ( rank - 1 ) * 0.5 ) )
      ->set_can_cancel( false );
  }

  auto proc = create_proc_action<chaos_bane_t>( "chaos_bane", effect, buff, rank );
  effect.custom_buff = buff_t::find( effect.player, "soul_fragment" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff =
        make_buff<stat_buff_t>( effect.player, "soul_fragment", effect.player->find_spell( 356042 ) )
          // coeff in driver eff#3
          ->add_stat( effect.player->convert_hybrid_stat( STAT_STR_AGI_INT ),
                      effect.driver()->effectN( 3 ).average( effect.player )  * ( 1.0 + ( rank - 1 ) * 0.5 ) )
          ->set_can_cancel( false )
          ->set_stack_change_callback( [ proc ]( buff_t* b, int, int cur ) {
            if ( cur == b->max_stack() )
            {
              proc->set_target( b->player->target );
              proc->execute();
              make_event( b->sim, [ b ] { b->expire(); } );
            }
          } );
  }

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE | PF2_PERIODIC_HEAL;
  new chaos_bane_cb_t( effect, buff );
}

/**Shard of Dyz
 * id=355755 driver Rank 1
 * id=357037 driver Rank 2 (Ominous)
 * id=357055 driver Rank 3 (Desolate)
 * id=357065 driver Rank 4 (Foreboding)
 * id=357076 driver Rank 5 (Portentous)
 * id=356329 Scouring Touch debuff
 */
void shard_of_dyz( special_effect_t& effect )
{
  struct shard_of_dyz_cb_t : public dbc_proc_callback_t
  {
    double debuff_value;

    shard_of_dyz_cb_t( const special_effect_t& e ) :
      dbc_proc_callback_t( e.player, e ),
      debuff_value( 0.0001 * e.driver()->effectN( 1 ).average( e.player) )
    {}

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto td = a->player->get_target_data( s->target );
      td->debuff.scouring_touch->set_default_value( debuff_value );
      td->debuff.scouring_touch->trigger();
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new shard_of_dyz_cb_t( effect );
}

/**Shard of Cor
 * id=355741 driver Rank 1
 * id=357034 driver Rank 2 (Ominous)
 * id=357052 driver Rank 3 (Desolate)
 * id=357062 driver Rank 4 (Foreboding)
 * id=357073 driver Rank 5 (Portentous)
 * id=356364 Coldhearted buff
 */
void shard_of_cor( special_effect_t& effect )
{
  struct shard_of_cor_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    shard_of_cor_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        target_list()
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

  buff_t* buff = buff_t::find( effect.player, "coldhearted" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "coldhearted", effect.player->find_spell( 356364 ) )
               ->set_default_value( 0.0001 * effect.driver()->effectN( 1 ).average( effect.player ), 1 )
               ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  effect.custom_buff = effect.player->buffs.coldhearted = buff;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new shard_of_cor_cb_t( effect );
}

/**Shard of Bek
 * id=355721 driver Rank 1
 * id=357031 driver Rank 2 (Ominous)
 * id=357049 driver Rank 3 (Desolate)
 * id=357058 driver Rank 4 (Foreboding)
 * id=357069 driver Rank 5 (Portentous)
 * id=356372 Exsanguinated debuff
 */
void shard_of_bek( special_effect_t& effect )
{
  struct shard_of_bek_cb_t : public dbc_proc_callback_t
  {
    double debuff_value;
    double health_pct_threshold;

    shard_of_bek_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        debuff_value( 0.0001 * e.driver()->effectN( 1 ).average( e.player ) ),
        health_pct_threshold( e.driver()->effectN( 2 ).base_value() )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto health_diff = a->player->health_percentage() - s->target->health_percentage();
      if ( health_diff > health_pct_threshold )
      {
        auto td = a->player->get_target_data( s->target );
        td->debuff.exsanguinated->set_default_value( debuff_value );
        td->debuff.exsanguinated->trigger();
      }
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  new shard_of_bek_cb_t( effect );
}

/**Shard of Zed
 * id=355766 driver Rank 1
 * id=357040 driver Rank 2 (Ominous)
 * id=357057 driver Rank 3 (Desolate)
 * id=357067 driver Rank 4 (Foreboding)
 * id=357077 driver Rank 5 (Portentous)
 * id=356321 Unholy Aura buff with periodic dummy
 * id=356320 Siphon Essence AoE leech spell
 */
void shard_of_zed( special_effect_t& effect )
{
  struct siphon_essence_t : proc_spell_t
  {
    siphon_essence_t( const special_effect_t& e ) :
      proc_spell_t( "siphon_essence", e.player, e.player->find_spell( 356320 ) )
    {
      base_dd_max = base_dd_min = e.driver()->effectN( 1 ).average( e.player );
    }
  };

  effect.custom_buff = buff_t::find( effect.player, "unholy_aura" );
  if ( !effect.custom_buff )
  {
    auto tick_damage = create_proc_action<siphon_essence_t>( "siphon_essence", effect );
    effect.custom_buff = make_buff( effect.player, "unholy_aura", effect.player->find_spell( 356321 ) )
      ->set_tick_callback( [ tick_damage ]( buff_t* buff, int, timespan_t )
        {
          tick_damage->set_target( buff->player->target );
          tick_damage->execute();
        } );
  }

  // TODO: confirm proc flags
  // 07/15/2021 - Logs seem somewhat inconclusive on this, perhaps it is delayed in triggering
  effect.proc_flags_ = PF_ALL_HEAL | PF_PERIODIC;
  effect.proc_flags2_ = PF2_LANDED | PF2_PERIODIC_HEAL;

  new dbc_proc_callback_t( effect.player, effect );
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
    unique_gear::register_special_effect( 330765, items::unbound_changeling );
    unique_gear::register_special_effect( 330080, items::unbound_changeling );
    unique_gear::register_special_effect( 330733, items::unbound_changeling );
    unique_gear::register_special_effect( 330734, items::unbound_changeling );
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
    unique_gear::register_special_effect( 336841, items::flame_of_battle );
    unique_gear::register_special_effect( 329831, items::overwhelming_power_crystal );
    unique_gear::register_special_effect( 336182, items::tablet_of_despair );
    unique_gear::register_special_effect( 329536, items::rotbriar_sprout );
    unique_gear::register_special_effect( 339343, items::murmurs_in_the_dark );

    // 9.1 Trinkets
    unique_gear::register_special_effect( 353492, items::forbidden_necromantic_tome );
    unique_gear::register_special_effect( 357672, items::soul_cage_fragment );
    unique_gear::register_special_effect( 353692, items::tome_of_monstrous_constructions );
    unique_gear::register_special_effect( 352429, items::miniscule_mailemental_in_an_envelope );
    unique_gear::register_special_effect( 355323, items::decanter_of_endless_howling );
    unique_gear::register_special_effect( 355324, items::tormentors_rack_fragment );
    unique_gear::register_special_effect( 355297, items::old_warriors_soul );
    unique_gear::register_special_effect( 355333, items::salvaged_fusion_amplifier );
    unique_gear::register_special_effect( 355313, items::titanic_ocular_gland );
    unique_gear::register_special_effect( 355327, items::ebonsoul_vise );
    unique_gear::register_special_effect( 355301, items::relic_of_the_frozen_wastes_equip );
    unique_gear::register_special_effect( 355303, items::relic_of_the_frozen_wastes_use );
    unique_gear::register_special_effect( 355321, items::shadowed_orb_of_torment );
    unique_gear::register_special_effect( 351679, items::ticking_sack_of_terror );
    unique_gear::register_special_effect( 351926, items::soleahs_secret_technique );

    // Weapons
    unique_gear::register_special_effect( 331011, items::poxstorm );
    unique_gear::register_special_effect( 358562, items::jaithys_the_prison_blade_1 );
    unique_gear::register_special_effect( 358565, items::jaithys_the_prison_blade_2 );
    unique_gear::register_special_effect( 358567, items::jaithys_the_prison_blade_3 );
    unique_gear::register_special_effect( 358569, items::jaithys_the_prison_blade_4 );
    unique_gear::register_special_effect( 358571, items::jaithys_the_prison_blade_5 );
    unique_gear::register_special_effect( 351527, items::yasahm_the_riftbreaker );
    unique_gear::register_special_effect( 359168, items::cruciform_veinripper);

    // Armor
    unique_gear::register_special_effect( 352081, items::passablyforged_credentials );
    unique_gear::register_special_effect( 353513, items::dark_rangers_quiver );

    // Runecarves
    unique_gear::register_special_effect( 338477, items::echo_of_eonar );
    unique_gear::register_special_effect( 339344, items::judgment_of_the_arbiter );
    unique_gear::register_special_effect( 340197, items::maw_rattle );
    unique_gear::register_special_effect( 339340, items::norgannons_sagacity );
    unique_gear::register_special_effect( 339348, items::sephuzs_proclamation );
    unique_gear::register_special_effect( 339058, items::third_eye_of_the_jailer );
    unique_gear::register_special_effect( 338743, items::vitality_sacrifice );

    // 9.1 Shards of Domination
    unique_gear::register_special_effect( 357347, items::blood_link ); // Rune Word: Blood
    unique_gear::register_special_effect( 357348, items::winds_of_winter ); // Rune Word: Frost
    unique_gear::register_special_effect( 357349, items::chaos_bane ); // Rune Word: Unholy

    unique_gear::register_special_effect( 355755, items::shard_of_dyz );
    unique_gear::register_special_effect( 357037, items::shard_of_dyz );
    unique_gear::register_special_effect( 357055, items::shard_of_dyz );
    unique_gear::register_special_effect( 357065, items::shard_of_dyz );
    unique_gear::register_special_effect( 357076, items::shard_of_dyz );

    unique_gear::register_special_effect( 355741, items::shard_of_cor );
    unique_gear::register_special_effect( 357034, items::shard_of_cor );
    unique_gear::register_special_effect( 357052, items::shard_of_cor );
    unique_gear::register_special_effect( 357062, items::shard_of_cor );
    unique_gear::register_special_effect( 357073, items::shard_of_cor );

    unique_gear::register_special_effect( 355721, items::shard_of_bek );
    unique_gear::register_special_effect( 357031, items::shard_of_bek );
    unique_gear::register_special_effect( 357049, items::shard_of_bek );
    unique_gear::register_special_effect( 357058, items::shard_of_bek );
    unique_gear::register_special_effect( 357069, items::shard_of_bek );

    unique_gear::register_special_effect( 355766, items::shard_of_zed );
    unique_gear::register_special_effect( 357040, items::shard_of_zed );
    unique_gear::register_special_effect( 357057, items::shard_of_zed );
    unique_gear::register_special_effect( 357067, items::shard_of_zed );
    unique_gear::register_special_effect( 357077, items::shard_of_zed );

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

  // Relic of the Frozen Wastes
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 355301 ) )
    {
      assert( !td->debuff.frozen_heart );

      td->debuff.frozen_heart = make_buff<buff_t>( *td, "frozen_heart", td->source->find_spell( 355759 ) );
      td->debuff.frozen_heart->reset();
    }
    else
      td->debuff.frozen_heart = make_buff( *td, "frozen_heart" )->set_quiet( true );
  } );

  // Ticking Sack of Terror
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 351679 ) )
    {
      assert( !td->debuff.volatile_satchel );

      td->debuff.volatile_satchel = make_buff<buff_t>( *td, "volatile_satchel", td->source->find_spell( 351682 ) );
      td->debuff.volatile_satchel->reset();
    }
    else
      td->debuff.volatile_satchel = make_buff( *td, "volatile_satchel" )->set_quiet( true );
  } );

  // Shard of Dyz (Scouring Touch debuff)
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 355755 )
      || unique_gear::find_special_effect( td->source, 357037 )
      || unique_gear::find_special_effect( td->source, 357055 )
      || unique_gear::find_special_effect( td->source, 357065 )
      || unique_gear::find_special_effect( td->source, 357076 ) )
    {
      assert( !td->debuff.scouring_touch );

      td->debuff.scouring_touch = make_buff( *td, "scouring_touch", td->source->find_spell( 356329 ) );
      td->debuff.scouring_touch->reset();
    }
    else
      td->debuff.scouring_touch = make_buff( *td, "scouring_touch" )->set_quiet( true );
  } );

  // Shard of Bek (Exsanguinated debuff)
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 355721 )
      || unique_gear::find_special_effect( td->source, 357031 )
      || unique_gear::find_special_effect( td->source, 357049 )
      || unique_gear::find_special_effect( td->source, 357058 )
      || unique_gear::find_special_effect( td->source, 357069 ) )
    {
      assert( !td->debuff.exsanguinated );

      td->debuff.exsanguinated = make_buff( *td, "exsanguinated", td->source->find_spell( 356372 ) );
      td->debuff.exsanguinated->reset();
    }
    else
      td->debuff.exsanguinated = make_buff( *td, "exsanguinated" )->set_quiet( true );
  } );
}

}  // namespace shadowlands
}  // namespace unique_gear
