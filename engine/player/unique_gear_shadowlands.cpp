// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear_shadowlands.hpp"

#include "sim/sc_sim.hpp"

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
  if ( !effect.player->buffs.celestial_guidance )
  {
    effect.player->buffs.celestial_guidance =
        make_buff<SL_buff_t>( effect.player, "celestial_guidance", effect.player->find_spell( 324748 ) )
            ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
            ->add_invalidate( CACHE_AGILITY )
            ->add_invalidate( CACHE_INTELLECT )
            ->add_invalidate( CACHE_STRENGTH );
  }

  effect.custom_buff = effect.player->buffs.celestial_guidance;

  new dbc_proc_callback_t( effect.player, effect );
}

void lightless_force( special_effect_t& effect )
{
  effect.trigger_spell_id = 324184;
  effect.execute_action   = create_proc_action<SL_proc_spell_t>( "lightless_force", effect );
  // Spell projectile travels along a narrow beam and does not spread out with distance. Although data has a 40yd range,
  // the 1.5s duration seems to prevent the projectile from surviving beyond ~30yd.
  // TODO: confirm maximum effective range & radius
  effect.execute_action->aoe    = -1;
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

    buff = make_buff<stat_buff_t>( effect.player, "crimson_chorus", effect.trigger(), effect.item )
      ->add_stat( STAT_CRIT_RATING, effect.driver()->effectN( 1 ).average( effect.item ) * mul )
      ->set_period( effect.trigger()->duration() )
      ->set_duration( effect.trigger()->duration() * effect.trigger()->max_stacks() );
  }

  effect.player->register_combat_begin( [ buff ]( player_t* p ) {
    make_repeating_event( *p->sim, 1_min, [ buff ]() {
      buff->trigger();
    } );
  } );
}

void dreadfire_vessel( special_effect_t& effect )
{
  struct dreadfire_vessel_proc_t : public proc_spell_t
  {
    dreadfire_vessel_proc_t( const special_effect_t& e ) : proc_spell_t( e )
    {
      // TODO: determine if the damage in the spell_data is for each flame or all 3 combined
      base_multiplier = 1.0 / 3.0;
      // TODO: determine actual travel speed of the flames (data has 1.5yd/s)
      travel_speed = 0.0;
    }

    void execute() override
    {
      // TODO: determine the how the three flames are fired
      proc_spell_t::execute();
      proc_spell_t::execute();
      proc_spell_t::execute();
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
  auto p    = effect.player;
  auto buff = make_buff<stat_buff_t>( p, "glyph_of_assimilation", p->find_spell( 345500 ) );
  buff->add_stat( STAT_MASTERY_RATING, buff->data().effectN( 1 ).average( effect.item ) );

  range::for_each( p->sim->actor_list, [ p, buff ]( player_t* t ) {
    if ( !t->is_enemy() )
      return;

    t->callbacks_on_demise.emplace_back( [ p, buff ]( player_t* t ) {
      if ( p->sim->event_mgr.canceled )
        return;

      auto d = t->get_dot( "glyph_of_assimilation", p );
      if ( d->remains() > 0_ms )
        buff->trigger( d->remains() * 2.0 );
    } );
  } );
}

void soul_igniter( special_effect_t& effect )
{

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

// Runecarves

void echo_of_eonar( special_effect_t& effect )
{
  struct echo_of_eonar_proc_t : public proc_spell_t
  {
    echo_of_eonar_proc_t( const special_effect_t& e ) : proc_spell_t( e )
    {
      quiet    = true;
      may_miss = may_crit = false;
    }

    // TODO: only debuff on target implemented - buff to allies NYI
    /*void execute() override
    {
      proc_spell_t::execute();
    }*/

    void impact( action_state_t* s ) override
    {
      proc_spell_t::impact( s );

      if ( s->target->is_enemy() )
      {
        auto td = player->get_target_data( s->target );
        td->debuff.echo_of_eonar->trigger();
      }
    }
  };

  // TODO: only debuff on target implemented - buff to allies NYI. buffs.echo_of_eonar will need to be added as
  // appropriate to sc_player.cpp & sc_player.hpp when implemented
  /*if ( !effect.player->buffs.echo_of_eonar )
  {
    effect.player->buffs.echo_of_eonar =
        make_buff( effect.player, "echo_of_eonar", effect.player->find_spell( 338489 ) )
            ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }*/

  // Buff spell/buff ID is 338489. Both buff & debuff spells have the same velocity, so we can use either as the
  // trigger_spell_id for now.
  effect.trigger_spell_id = 338494;
  effect.execute_action   = create_proc_action<echo_of_eonar_proc_t>( "echo_of_eonar", effect );

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

  // Echo of Eonar
  sim.register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( unique_gear::find_special_effect( td->source, 338477 ) )
    {
      assert( !td->debuff.echo_of_eonar );

      td->debuff.echo_of_eonar = make_buff<buff_t>( *td, "echo_of_eonar", td->source->find_spell( 338494 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );
      td->debuff.echo_of_eonar->reset();
    }
    else
      td->debuff.echo_of_eonar = make_buff( *td, "echo_of_eonar" )->set_quiet( true );
  } );
}

}  // namespace shadowlands
}  // namespace unique_gear