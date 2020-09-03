// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "soulbinds.hpp"

#include "player/actor_target_data.hpp"
#include "player/unique_gear_helper.hpp"
#include "sim/sc_sim.hpp"

namespace covenant
{
namespace soulbinds
{
namespace
{
struct covenant_cb_base_t
{
  covenant_cb_base_t()
  {
  }
  virtual void trigger( action_t*, action_state_t* s ) = 0;
  virtual ~covenant_cb_base_t() { }
};

struct covenant_cb_buff_t : public covenant_cb_base_t
{
  buff_t* buff;

  covenant_cb_buff_t( buff_t* b ) : covenant_cb_base_t(), buff( b )
  {
  }
  void trigger( action_t*, action_state_t* ) override
  {
    buff->trigger();
  }
};

struct covenant_cb_action_t : public covenant_cb_base_t
{
  action_t* action;
  bool self_target;

  covenant_cb_action_t( action_t* a, bool self = false ) : covenant_cb_base_t(), action( a ), self_target( self )
  {
  }
  void trigger( action_t*, action_state_t* state ) override
  {
    auto t = self_target || !state->target ? action->player : state->target;

    if ( t->is_sleeping() )
      return;

    action->set_target( t );
    action->schedule_execute();
  }
};

struct covenant_ability_cast_cb_t : public dbc_proc_callback_t
{
  unsigned covenant_ability;
  auto_dispose< std::vector<covenant_cb_base_t*> > cb_list;

  covenant_ability_cast_cb_t( player_t* p, const special_effect_t& e )
    : dbc_proc_callback_t( p, e ), covenant_ability( p->covenant->get_covenant_ability_spell_id() ), cb_list()
  {
    // Manual overrides for covenant abilities that don't utilize the spells found in __covenant_ability_data dbc table
    if ( p->type == DRUID && p->covenant->type() == covenant_e::KYRIAN )
      covenant_ability = 326446;
  }

  void initialize() override
  {
    listener->sim->print_debug( "Initializing covenant ability cast handler..." );
    listener->callbacks.register_callback( effect.proc_flags(), effect.proc_flags2(), this );
  }

  void trigger( action_t* a, action_state_t* s ) override
  {
    if ( a->data().id() != covenant_ability )
      return;

    for ( const auto& t : cb_list )
      t->trigger( a, s );
  }
};

// Add an effect to be triggered when covenant ability is cast. Currently has has templates for buff_t & action_t, and
// can be expanded via additional subclasses to covenant_cb_base_t.
template <typename T, typename... S>
void add_covenant_cast_callback( player_t* p, S&&... args )
{
  if ( !p->covenant->enabled() )
    return;

  if ( !p->covenant->cast_callback )
  {
    auto eff                   = new special_effect_t( p );
    eff->name_str              = "covenant_cast_callback";
    eff->proc_flags_           = PF_ALL_DAMAGE;
    eff->proc_flags2_          = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
    p->special_effects.push_back( eff );
    p->covenant->cast_callback = new covenant_ability_cast_cb_t( p, *eff );
  }

  auto cb_entry = new T( std::forward<S>( args )... );
  auto cb       = debug_cast<covenant_ability_cast_cb_t*>( p->covenant->cast_callback );
  cb->cb_list.push_back( cb_entry );
}

void niyas_tools_burrs( special_effect_t& effect )
{

}

void niyas_tools_poison( special_effect_t& effect )
{

}

void niyas_tools_herbs( special_effect_t& effect )
{

}

void grove_invigoration( special_effect_t& effect )
{
  struct redirected_anima_buff_t : public buff_t
  {
    redirected_anima_buff_t( player_t* p ) : buff_t( p, "redirected_anima", p->find_spell( 342814 ) )
    {
      set_default_value_from_effect_type( A_MOD_MASTERY_PCT );
      add_invalidate( CACHE_MASTERY );
    }

    bool trigger( int s, double v, double c, timespan_t d ) override
    {
      int anima_stacks = player->buffs.redirected_anima_stacks->check();

      if ( !anima_stacks )
        return false;

      player->buffs.redirected_anima_stacks->expire();

      return buff_t::trigger( anima_stacks, v, c, d );
    }
  };

  effect.custom_buff = effect.player->buffs.redirected_anima_stacks;

  new dbc_proc_callback_t( effect.player, effect );

  if ( !effect.player->buffs.redirected_anima )
    effect.player->buffs.redirected_anima = make_buff<redirected_anima_buff_t>( effect.player );

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, effect.player->buffs.redirected_anima );
}

void field_of_blossoms( special_effect_t& effect )
{
  if ( !effect.player->find_soulbind_spell( effect.driver()->name_cstr() )->ok() )
    return;

  if ( !effect.player->buffs.field_of_blossoms )
  {
    effect.player->buffs.field_of_blossoms =
        make_buff( effect.player, "field_of_blossoms", effect.player->find_spell( 342774 ) )
            ->set_cooldown( effect.player->find_spell( 342781 )->duration() )
            ->set_default_value_from_effect_type( A_HASTE_ALL )
            ->add_invalidate( CACHE_HASTE );
  }

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, effect.player->buffs.field_of_blossoms );
}

void social_butterfly( special_effect_t& effect )
{
  struct social_butterfly_buff_t : public buff_t
  {
    social_butterfly_buff_t( player_t* p ) : buff_t( p, "social_butterfly", p->find_spell( 320212 ) )
    {
      add_invalidate( CACHE_VERSATILITY );
      set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT );
    }

    void expire_override( int s, timespan_t d ) override
    {
      buff_t::expire_override( s, d );

      make_event( *sim, data().duration(), [this]() { trigger(); } );
    }
  };

  if ( !effect.player->buffs.social_butterfly )
    effect.player->buffs.social_butterfly = make_buff<social_butterfly_buff_t>( effect.player );

  effect.player->register_combat_begin( []( player_t* p ) { p->buffs.social_butterfly->trigger(); } );
}

void first_strike( special_effect_t& effect )
{
  struct first_strike_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    first_strike_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), target_list() {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( range::contains( target_list, s->target->actor_spawn_index ) )
        return;

      dbc_proc_callback_t::trigger( a, s );
      target_list.push_back( s->target->actor_spawn_index );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      target_list.clear();
    }
  };

  if ( !effect.player->buffs.first_strike )
  {
    effect.player->buffs.first_strike = make_buff( effect.player, "first_strike", effect.player->find_spell( 325381 ) )
      ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
      ->add_invalidate( CACHE_CRIT_CHANCE );
  }

  effect.custom_buff = effect.player->buffs.first_strike;

  new first_strike_cb_t( effect );
}

void wild_hunt_tactics( special_effect_t& effect )
{
  // dummy buffs to hold info, this is never triggered
  // TODO: determine if there's a better place to hold this data
  if ( !effect.player->buffs.wild_hunt_tactics )
    effect.player->buffs.wild_hunt_tactics = make_buff( effect.player, "wild_hunt_tactics", effect.driver() )
      ->set_default_value_from_effect( 1 );
}

void exacting_preparation( special_effect_t& effect )
{

}

void dauntless_duelist( special_effect_t& effect )
{
  struct dauntless_duelist_cb_t : public dbc_proc_callback_t
  {
    dauntless_duelist_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void trigger( action_t* a, action_state_t* st ) override
    {
      auto td = a->player->get_target_data( st->target );
      td->debuff.adversary->trigger();

      deactivate();
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();

      activate();
    }
  };

  new dauntless_duelist_cb_t( effect );
}

void thrill_seeker( special_effect_t& effect )
{
  if ( !effect.player->buffs.euphoria )
  {
    effect.player->buffs.euphoria = make_buff( effect.player, "euphoria", effect.player->find_spell( 331937 ) )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->add_invalidate( CACHE_HASTE );
  }

  auto eff_data = &effect.driver()->effectN( 1 );

  // TODO: do you still gain stacks while euphoria is active?
  effect.player->register_combat_begin( [eff_data]( player_t* p ) {
    make_repeating_event( *p->sim, eff_data->period(), [p]() { p->buffs.thrill_seeker->trigger(); } );
  } );

  // TODO: implement gains from killing blows
}

void refined_palate( special_effect_t& effect )
{

}

void soothing_shade( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "soothing_shade" );
  if ( !buff )
    buff = make_buff<stat_buff_t>( effect.player, "soothing_shade", effect.player->find_spell( 336885 ) );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void wasteland_propriety( special_effect_t& effect )
{
  if ( !effect.player->buffs.wasteland_propriety )
  {
    effect.player->buffs.wasteland_propriety =
        make_buff( effect.player, "wasteland_propriety", effect.player->find_spell( 333218 ) )
            ->set_cooldown( effect.player->find_spell( 333221 )->duration() )
            ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT )
            ->add_invalidate( CACHE_VERSATILITY );
  }

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, effect.player->buffs.wasteland_propriety );
}

void built_for_war( special_effect_t& effect )
{
  if ( !effect.player->buffs.built_for_war )
  {
    effect.player->buffs.built_for_war =
        make_buff( effect.player, "built_for_war", effect.player->find_spell( 332842 ) )
            ->set_default_value_from_effect_type( A_MOD_PERCENT_STAT )
            ->add_invalidate( CACHE_STRENGTH )
            ->add_invalidate( CACHE_AGILITY )
            ->add_invalidate( CACHE_INTELLECT );
  }

  auto eff_data = &effect.driver()->effectN( 1 );

  effect.player->register_combat_begin( [eff_data]( player_t* p ) {
    make_repeating_event( *p->sim, eff_data->period(), [p, eff_data]() {
      if ( p->health_percentage() > eff_data->base_value() )
        p->buffs.built_for_war->trigger();
    } );
  } );

  // TODO: add option to simulate losing the buff from going below 50% hp
}

void superior_tactics( special_effect_t& effect )
{

}

void let_go_of_the_past( special_effect_t& effect )
{
  struct let_go_of_the_past_cb_t : public dbc_proc_callback_t
  {
    unsigned prev_id;

    let_go_of_the_past_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), prev_id( 0 ) {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( !a->id || a->background || a->id == prev_id )
        return;

      dbc_proc_callback_t::trigger( a, s );
      prev_id = a->id;
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      prev_id = 0;
    }
  };

  if ( !effect.player->buffs.let_go_of_the_past )
  {
    effect.player->buffs.let_go_of_the_past =
        make_buff( effect.player, "let_go_of_the_past", effect.player->find_spell( 328900 ) )
            ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT )
            ->add_invalidate( CACHE_VERSATILITY );
  }

  effect.proc_flags_ = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.custom_buff = effect.player->buffs.let_go_of_the_past;

  new let_go_of_the_past_cb_t( effect );
}

void combat_meditation( special_effect_t& effect )
{
  struct combat_meditation_buff_t : public buff_t
  {
    timespan_t ext_dur;

    combat_meditation_buff_t( player_t* p )
      : buff_t( p, "combat_meditation", p->find_spell( 328908 ) ),
        ext_dur( timespan_t::from_seconds( p->find_spell( 328913 )->effectN( 2 ).base_value() ) )
    {
      set_default_value_from_effect_type( A_MOD_MASTERY_PCT );
      add_invalidate( CACHE_MASTERY );
      // TODO: add more faithful simulation of delay/reaction needed from player to walk into the sorrowful memories
      set_tick_callback( [this]( buff_t*, int, timespan_t ) {
        if ( rng().roll( sim->shadowlands_opts.combat_meditation_extend_chance ) )
          extend_duration( player, ext_dur );
      } );
    }
  };

  if ( !effect.player->buffs.combat_meditation )
    effect.player->buffs.combat_meditation = make_buff<combat_meditation_buff_t>( effect.player );

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, effect.player->buffs.combat_meditation );
}

void pointed_courage( special_effect_t& effect )
{
  if ( !effect.player->buffs.pointed_courage )
  {
    effect.player->buffs.pointed_courage =
        make_buff( effect.player, "pointed_courage", effect.player->find_spell( 330511 ) )
            ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
            ->add_invalidate( CACHE_CRIT_CHANCE )
            // TODO: add better handling of allies/enemies nearby mechanic which is checked every tick. tick is disabled
            // for now
            ->set_period( 0_ms );
  }

  effect.player->register_combat_begin( []( player_t* p ) {
    p->buffs.pointed_courage->trigger( p->sim->shadowlands_opts.pointed_courage_nearby );
  } );
}

void hammer_of_genesis( special_effect_t& effect )
{
  struct hammer_of_genesis_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    hammer_of_genesis_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), target_list() {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( range::contains( target_list, s->target->actor_spawn_index ) )
        return;

      dbc_proc_callback_t::trigger( a, s );
      target_list.push_back( s->target->actor_spawn_index );
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      target_list.clear();
    }
  };

  if ( !effect.player->buffs.hammer_of_genesis )
  {
    effect.player->buffs.hammer_of_genesis =
        make_buff( effect.player, "hammer_of_genesis", effect.player->find_spell( 333943 ) )
            ->set_default_value_from_effect_type( A_HASTE_ALL )
            ->add_invalidate( CACHE_HASTE );
  }

  // TODO: confirm that spell_data proc flags are correct (doesn't proc from white hits)
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.custom_buff = effect.player->buffs.hammer_of_genesis;

  new hammer_of_genesis_cb_t( effect );
}

void brons_call_to_action( special_effect_t& effect )
{

}

void volatile_solvent( special_effect_t& effect )
{

}

void plagueys_preemptive_strike( special_effect_t& effect )
{
  struct plagueys_preemptive_strike_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    plagueys_preemptive_strike_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), target_list() {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( !a->harmful )
        return;

      if ( range::contains( target_list, s->target->actor_spawn_index ) )
        return;

      dbc_proc_callback_t::trigger( a, s );
      target_list.push_back( s->target->actor_spawn_index );

      auto td = a->player->get_target_data( s->target );
      td->debuff.plagueys_preemptive_strike->trigger();
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();
      target_list.clear();
    }
  };

  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE;

  new plagueys_preemptive_strike_cb_t( effect );
}

void gnashing_chompers( special_effect_t& effect )
{
  auto buff = effect.player->buffs.gnashing_chompers;
  if ( !buff )
  {
    buff = make_buff( effect.player, "gnashing_chompers", effect.player->find_spell( 324242 ) )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->add_invalidate( CACHE_HASTE )
      ->set_period( 0_ms )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  range::for_each( effect.player->sim->actor_list, [buff]( player_t* p ) {
    if ( !p->is_enemy() )
      return;

    p->callbacks_on_demise.emplace_back( [buff]( player_t* ) { buff->trigger(); } );
  } );
}

void embody_the_construct( special_effect_t& effect )
{

}

void serrated_spaulders( special_effect_t& effect )
{

}

void heirmirs_arsenal_marrowed_gemstone( special_effect_t& effect )
{
  if ( !effect.player->buffs.marrowed_gemstone_enhancement )
  {
    effect.player->buffs.marrowed_gemstone_enhancement =
        make_buff( effect.player, "marrowed_gemstone_enhancement", effect.player->find_spell( 327069 ) )
            ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
            ->add_invalidate( CACHE_CRIT_CHANCE );
    // TODO: confirm if cooldown applies only to the crit buff, or to the counter as well
    effect.player->buffs.marrowed_gemstone_enhancement->set_cooldown(
        effect.player->buffs.marrowed_gemstone_enhancement->buff_duration() +
        effect.player->find_spell( 327073 )->duration() );
  }

  effect.proc_flags2_ = PF2_CRIT;
  effect.custom_buff = effect.player->buffs.marrowed_gemstone_charging;

  new dbc_proc_callback_t( effect.player, effect );
}

// Helper function for registering an effect, with autoamtic skipping initialization if soulbind spell is not available
void register_soulbind_special_effect( unsigned spell_id, custom_cb_t init_callback )
{
  unique_gear::register_special_effect( spell_id, [ &, init_callback ]( special_effect_t& effect ) {
    if ( !effect.player->find_soulbind_spell( effect.driver()->name_cstr() )->ok() )
      return;
    init_callback( effect );
  } );
}

}  // namespace

void register_special_effects()
{
  // Night Fae
  register_soulbind_special_effect( 320659, soulbinds::niyas_tools_burrs );  // Niya
  register_soulbind_special_effect( 320660, soulbinds::niyas_tools_poison );
  register_soulbind_special_effect( 320662, soulbinds::niyas_tools_herbs );
  register_soulbind_special_effect( 322721, soulbinds::grove_invigoration );
  register_soulbind_special_effect( 319191, soulbinds::field_of_blossoms );  // Dreamweaver
  register_soulbind_special_effect( 319210, soulbinds::social_butterfly );
  register_soulbind_special_effect( 325069, soulbinds::first_strike );  // Korayn
  register_soulbind_special_effect( 325066, soulbinds::wild_hunt_tactics );
  // Venthyr
  register_soulbind_special_effect( 331580, soulbinds::exacting_preparation );  // Nadjia
  register_soulbind_special_effect( 331584, soulbinds::dauntless_duelist );
  register_soulbind_special_effect( 331586, soulbinds::thrill_seeker );
  register_soulbind_special_effect( 336243, soulbinds::refined_palate );  // Theotar
  register_soulbind_special_effect( 336239, soulbinds::soothing_shade );
  register_soulbind_special_effect( 319983, soulbinds::wasteland_propriety );
  register_soulbind_special_effect( 319973, soulbinds::built_for_war );  // Draven
  register_soulbind_special_effect( 332753, soulbinds::superior_tactics );
  // Kyrian
  register_soulbind_special_effect( 328257, soulbinds::let_go_of_the_past );  // Pelagos
  register_soulbind_special_effect( 328266, soulbinds::combat_meditation );
  register_soulbind_special_effect( 329778, soulbinds::pointed_courage );    // Kleia
  register_soulbind_special_effect( 333935, soulbinds::hammer_of_genesis );  // Mikanikos
  register_soulbind_special_effect( 333950, soulbinds::brons_call_to_action );
  // Necrolord
  register_soulbind_special_effect( 323074, soulbinds::volatile_solvent );  // Marileth
  register_soulbind_special_effect( 323090, soulbinds::plagueys_preemptive_strike );
  register_soulbind_special_effect( 323919, soulbinds::gnashing_chompers );  // Emeni
  register_soulbind_special_effect( 342156, soulbinds::embody_the_construct );
  register_soulbind_special_effect( 326504, soulbinds::serrated_spaulders );  // Heirmir
  register_soulbind_special_effect( 326572, soulbinds::heirmirs_arsenal_marrowed_gemstone );
}

void initialize_soulbinds( player_t* player )
{
  if ( !player->covenant )
    return;

  for ( auto soulbind_spell : player->covenant->soulbind_spells() )
  {
    auto spell = player->find_spell( soulbind_spell );
    if ( !spell->ok() )
      continue;

    special_effect_t effect{ player };
    effect.type   = SPECIAL_EFFECT_EQUIP;
    effect.source = SPECIAL_EFFECT_SOURCE_SOULBIND;

    unique_gear::initialize_special_effect( effect, soulbind_spell );

    // Ensure the soulbind has a custom special effect to protect against errant auto-inference
    if ( !effect.is_custom() )
      continue;

    player->special_effects.push_back( new special_effect_t( effect ) );
  }
}

void register_target_data_initializers( sim_t* sim )
{
  // Dauntless Duelist
  sim->register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( td->source->find_soulbind_spell( "Dauntless Duelist" )->ok() )
    {
      assert( !td->debuff.adversary );

      td->debuff.adversary = make_buff( *td, "adversary", td->source->find_spell( 331934 ) )
        ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );
      td->debuff.adversary->reset();
    }
    else
      td->debuff.adversary = make_buff( *td, "adversary" )->set_quiet( true );
  } );

  // Plaguey's Preemptive Strike
  sim->register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( td->source->find_soulbind_spell( "Plaguey's Preemptive Strike" )->ok() )
    {
      assert( !td->debuff.plagueys_preemptive_strike );

      td->debuff.plagueys_preemptive_strike =
          make_buff( *td, "plagueys_preemptive_strike", td->source->find_spell( 323416 ) )
              ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );
      td->debuff.plagueys_preemptive_strike->reset();
    }
    else
      td->debuff.plagueys_preemptive_strike = make_buff( *td, "plagueys_preemptive_strike" )->set_quiet( true );
  } );
}

}  // namespace soulbinds
}  // namespace covenant