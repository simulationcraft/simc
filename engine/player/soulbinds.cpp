// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "soulbinds.hpp"

#include "player/actor_target_data.hpp"
#include "player/unique_gear_helper.hpp"
#include "player/pet.hpp"

#include "action/dot.hpp"

#include "sim/sc_sim.hpp"
#include "sim/sc_cooldown.hpp"

#include <regex>

namespace covenant
{
namespace soulbinds
{
namespace
{
struct covenant_cb_base_t
{
  bool trigger_on_class; // proc off class ability
  bool trigger_on_base;  // proc off base ability

  covenant_cb_base_t( bool on_class = true, bool on_base = false )
    : trigger_on_class( on_class ), trigger_on_base( on_base )
  {
  }
  virtual void trigger( action_t*, action_state_t* s ) = 0;
  virtual ~covenant_cb_base_t() { }
};

struct covenant_cb_buff_t : public covenant_cb_base_t
{
  buff_t* buff;

  covenant_cb_buff_t( buff_t* b, bool on_class = true, bool on_base = false )
    : covenant_cb_base_t( on_class, on_base ), buff( b )
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

  covenant_cb_action_t( action_t* a, bool self = false, bool on_class = true, bool on_base = false )
    : covenant_cb_base_t( on_class, on_base ), action( a ), self_target( self )
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
  unsigned class_ability;
  unsigned base_ability;
  auto_dispose< std::vector<covenant_cb_base_t*> > cb_list;

  covenant_ability_cast_cb_t( player_t* p, const special_effect_t& e )
    : dbc_proc_callback_t( p, e ),
      class_ability( p->covenant->get_covenant_ability_spell_id() ),
      base_ability( p->covenant->get_covenant_ability_spell_id( true ) ),
      cb_list()
  {
    // Manual overrides for covenant abilities that don't utilize the spells found in __covenant_ability_data dbc table
    if ( p->type == DRUID && p->covenant->type() == covenant_e::KYRIAN )
      class_ability = 326446;
  }

  void initialize() override
  {
    listener->sim->print_debug( "Initializing covenant ability cast handler..." );
    listener->callbacks.register_callback( effect.proc_flags(), effect.proc_flags2(), this );
  }

  void trigger( action_t* a, action_state_t* s ) override
  {
    if ( a->data().id() != class_ability && a->data().id() != base_ability )
      return;

    for ( const auto& cb : cb_list )
    {
      if ( ( cb->trigger_on_class && a->data().id() == class_ability ) ||
           ( cb->trigger_on_base && a->data().id() == base_ability ) )
        cb->trigger( a, s );
    }
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

double get_coefficient_from_desc_vars( special_effect_t& e, bool sp = true )
{
  double coef = 0;
  if ( const char* vars = e.player->dbc->spell_desc_vars( e.spell_id ).desc_vars() )
  {
    std::cmatch m;
    std::regex r( sp ? "\\$SP\\*([0-9]*\\.?[0-9]*)" : "\\$AP\\*([0-9]*\\.?[0-9]*)" );
    if ( std::regex_search( vars, m, r ) && m.size() >= 2 )
      coef = std::stod( m.str( 1 ) );
  }
  return coef;
}

struct niyas_tools_proc_t : public unique_gear::proc_spell_t
{
  niyas_tools_proc_t( util::string_view n, player_t* p, const spell_data_t* s, double mod, bool direct = true ) : proc_spell_t( n, p, s )
  {
    if ( direct )
      spell_power_mod.direct = mod;
    else
      spell_power_mod.tick   = mod;
  }

  double composite_spell_power() const override
  {
    return std::max( proc_spell_t::composite_spell_power(), proc_spell_t::composite_attack_power() );
  }
};

void niyas_tools_burrs( special_effect_t& effect )
{
  auto action = effect.player->find_action( "spiked_burrs" );
  if ( !action )
    action = new niyas_tools_proc_t( "spiked_burrs", effect.player, effect.player->find_spell( 333526 ), get_coefficient_from_desc_vars( effect ), false );

  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

void niyas_tools_poison( special_effect_t& effect )
{
  struct niyas_tools_poison_cb_t : public dbc_proc_callback_t
  {
    action_t* dot;
    action_t* direct;

    niyas_tools_poison_cb_t( special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
      dot = e.player->find_action( "paralytic_poison" );
      if ( !dot )
        dot = new niyas_tools_proc_t( "paralytic_poison", e.player, e.player->find_spell( 321519 ), get_coefficient_from_desc_vars( e ), false );

      direct = e.player->find_action( "paralytic_poison_interrupt" );
      if ( !direct )
        direct = new niyas_tools_proc_t( "paralytic_poison_interrupt", e.player, e.player->find_spell( 321524 ), get_coefficient_from_desc_vars( e ) );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      if ( !dot->get_dot( s->target )->is_ticking() )
      {
        dot->set_target( s->target );
        dot->schedule_execute();
      }
      else
      {
        direct->set_target( s->target );
        direct->schedule_execute();
      }
    }
  };

  // TODO: confirm if you need to successfully interrupt to apply the poison
  // TODO: confirm what happens if you interrupt again - does it refresh/remove dot, can it be reapeatedly proc'd
  effect.proc_flags2_ |= PF2_CAST_INTERRUPT;
  effect.disable_action();

  new niyas_tools_poison_cb_t( effect );
}

void niyas_tools_herbs( special_effect_t& effect )
{
  if ( !effect.player->buffs.invigorating_herbs )
  {
    effect.player->buffs.invigorating_herbs = make_buff( effect.player, "invigorating_herbs", effect.trigger() )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->add_invalidate( CACHE_HASTE );
  }

  // TODO: confirm proc flags
  effect.proc_flags2_ = PF2_CAST_HEAL;
  effect.custom_buff  = effect.player->buffs.invigorating_herbs;

  new dbc_proc_callback_t( effect.player, effect );
}

void grove_invigoration( special_effect_t& effect )
{
  struct redirected_anima_buff_t : public stat_buff_t
  {
    redirected_anima_buff_t( player_t* p ) : stat_buff_t( p, "redirected_anima", p->find_spell( 342814 ) ) {}

    bool trigger( int, double v, double c, timespan_t d ) override
    {
      // TODO: does this convert ALL stacks or only up to 4 stacks, as per max_stack of the mastery buff
      int anima_stacks = player->buffs.redirected_anima_stacks->check();

      if ( !anima_stacks )
        return false;

      anima_stacks = std::min( anima_stacks, as<int>( data().max_stacks() ) );
      player->buffs.redirected_anima_stacks->decrement( anima_stacks );

      return stat_buff_t::trigger( anima_stacks, v, c, d );
    }
  };

  effect.custom_buff = effect.player->buffs.redirected_anima_stacks;

  new dbc_proc_callback_t( effect.player, effect );

  auto buff = buff_t::find( effect.player, "redirected_anima" );
  if ( !buff )
    buff = make_buff<redirected_anima_buff_t>( effect.player );

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff );
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
  // ID: 320130 is the buff on player
  // ID: 320212 is the buff on allies (NYI)
  struct social_butterfly_buff_t : public buff_t
  {
    social_butterfly_buff_t( player_t* p ) : buff_t( p, "social_butterfly", p->find_spell( 320130 ) )
    {
      set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT );
      add_invalidate( CACHE_VERSATILITY );
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
// Handled in unique_gear_shadowlands.cpp
//void exacting_preparation( special_effect_t& effect ) {}

void dauntless_duelist( special_effect_t& effect )
{
  struct dauntless_duelist_cb_t : public dbc_proc_callback_t
  {
    dauntless_duelist_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ) {}

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto td = a->player->get_target_data( s->target );
      td->debuff.adversary->trigger();

      deactivate();
    }

    void reset() override
    {
      dbc_proc_callback_t::reset();

      activate();
    }
  };

  auto cb = new dauntless_duelist_cb_t( effect );
  auto p  = effect.player;

  range::for_each( p->sim->actor_list, [ p, cb ]( player_t* t ) {
    if ( !t->is_enemy() )
      return;

    t->callbacks_on_demise.emplace_back( [ p, cb ]( player_t* t ) {
      if ( p->sim->event_mgr.canceled )
        return;

      auto td = p->get_target_data( t );
      if ( td->debuff.adversary->check() )
        cb->activate();
    } );
  } );
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
    double duration_mod = 1.0;
    bool icd_enabled = false;

    // The duration modifier for each class comes from the description variables of Wasteland Propriety (id=319983)
    if ( const char* vars = effect.player->dbc->spell_desc_vars( effect.spell_id ).desc_vars() )
    {
      // The description variables string contains instances of "?a137005[${1.0}]", where the class is
      // defined by the class aura spell ID and the floating point number represents the duration modifier.
      std::regex r( "\\?a(\\d+)\\[\\$\\{(\\d*\\.?\\d+)" );
      std::cregex_iterator begin( vars, vars + std::strlen( vars ), r );
      for( std::cregex_iterator i = begin; i != std::cregex_iterator(); i++ )
      {
        auto spell = effect.player->find_spell( std::stoi( ( *i ).str( 1 ) ) );
        if ( spell->is_class( effect.player->type ) )
        {
          duration_mod = std::stod( ( *i ).str( 2 ) );
          break;
        }
      }
    }

    // The ICD of 60 seconds is enabled for some classes in the description of Wasteland Propriety (id=319983)
    if ( const char* desc = effect.player->dbc->spell_text( effect.spell_id ).desc() )
    {
      // The relevant part of the description is formatted as a '|' delimited list of the
      // letter 'a' followed by class aura spell IDs until we reach a '[' and a new line.
      std::regex r( "(?=(?:a\\d+\\|?)*\\[[\r\n])a(\\d+)" );
      std::cregex_iterator begin( desc, desc + std::strlen( desc ), r );
      for( std::cregex_iterator i = begin; i != std::cregex_iterator(); i++ )
      {
        auto spell = effect.player->find_spell( std::stoi( ( *i ).str( 1 ) ) );
        if ( icd_enabled = spell->is_class( effect.player->type ) )
          break;
      }
    }

    effect.player->sim->print_debug( "class-specific properties for wasteland_propriety: duration_mod={}, icd_enabled={}", duration_mod, icd_enabled );

    effect.player->buffs.wasteland_propriety =
        make_buff( effect.player, "wasteland_propriety", effect.player->find_spell( 333218 ) )
            ->set_cooldown( icd_enabled ? effect.player->find_spell( 333221 )->duration() : 0_ms )
            ->set_duration_multiplier( duration_mod )
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
  if ( !effect.player->buffs.superior_tactics )
  {
    effect.player->buffs.superior_tactics = make_buff( effect.player, "superior_tactics", effect.trigger() )
      ->set_cooldown( effect.trigger()->effectN( 2 ).trigger()->duration() )
      ->set_default_value_from_effect( A_MOD_ALL_CRIT_CHANCE )
      ->add_invalidate( CACHE_CRIT_CHANCE );
  }

  // TODO: implement proc'ing from dispels
  effect.proc_flags2_ |= PF2_CAST_INTERRUPT;
  effect.custom_buff = effect.player->buffs.superior_tactics;

  new dbc_proc_callback_t( effect.player, effect );
}

void let_go_of_the_past( special_effect_t& effect )
{
  struct let_go_of_the_past_cb_t : public dbc_proc_callback_t
  {
    unsigned prev_id;

    let_go_of_the_past_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), prev_id( 0 ) {}

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( !a->id || a->background || a->id == prev_id )
        return;

      dbc_proc_callback_t::execute( a, s );
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
      set_refresh_behavior( buff_refresh_behavior::EXTEND );
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

template <typename Base>
struct bron_action_t : public Base
{
  using base_t = bron_action_t<Base>;

  pet_t* bron;

  bron_action_t( util::string_view n, pet_t* p, const spell_data_t* s ) : Base( n, p, s ), bron( p ) {}

  void execute() override
  {
    Base::execute();

    if ( Base::player->main_hand_attack->execute_event )
      Base::player->main_hand_attack->execute_event->reschedule( Base::player->main_hand_weapon.swing_time );
  }
};

void brons_call_to_action( special_effect_t& effect )
{
  struct bron_pet_t : public pet_t
  {
    struct bron_anima_cannon_t : public bron_action_t<spell_t>
    {
      bron_anima_cannon_t( pet_t* p ) : base_t( "anima_cannon", p, p->find_spell( 332525 ) )
      {
        base_execute_time = 0_ms;
        cooldown->duration = 8_s;  // TODO: confirm
      }
    };

    struct bron_smash_t : public bron_action_t<melee_attack_t>
    {
      bron_smash_t( pet_t* p ) : base_t( "smash", p, p->find_spell( 341163 ) )
      {
        may_crit          = true;
        aoe               = -1;
        radius            = data().effectN( 2 ).radius();
        travel_speed      = 0.0;
        base_execute_time = data().cast_time();
        weapon            = &p->main_hand_weapon;
        weapon_multiplier = p->find_spell( 341165 )->effectN( 1 ).percent();
      }
    };

    struct bron_goliath_support_t : public bron_action_t<heal_t>
    {
      bron_goliath_support_t( pet_t* p ) : base_t( "goliath_support", p, p->find_spell( 332526 ) )
      {
        base_execute_time = 0_ms;
        cooldown->duration = 4_s;  // TODO: confirm
      }

      void execute() override
      {
        target = debug_cast<pet_t*>( player )->owner;

        base_t::execute();
      }
    };

    struct bron_melee_t : public melee_attack_t
    {
      bron_melee_t( pet_t* p ) : melee_attack_t( "melee", p, spell_data_t::nil() )
      {
        background = repeating = may_crit = may_glance = true;

        school            = SCHOOL_PHYSICAL;
        weapon            = &p->main_hand_weapon;
        weapon_multiplier = 1.0;
        base_execute_time = weapon->swing_time;
      }
    };

    struct bron_auto_attack_t : public melee_attack_t
    {
      bron_auto_attack_t( pet_t* p ) : melee_attack_t( "auto_attack", p )
      {
        player->main_hand_attack = new bron_melee_t( p );
      }

      bool ready() override
      {
        return !player->main_hand_attack->execute_event;
      }

      void execute() override
      {
        player->main_hand_attack->schedule_execute();
      }
    };

    // TODO: confirm if travel is necessary
    struct bron_travel_t : public action_t
    {
      bron_travel_t( pet_t* p ) : action_t( ACTION_OTHER, "travel", p ) {}

      void execute() override
      {
        player->current.distance = 5.0;
      }

      timespan_t execute_time() const override
      {
        return timespan_t::from_seconds( ( player->current.distance - 5.0 ) / 10.0 );
      }

      bool ready() override
      {
        return ( player->current.distance > 5.0 );
      }

      bool usable_moving() const override { return true; }
    };

    bron_pet_t( player_t* owner ) : pet_t( owner->sim, owner, "bron" )
    {
      main_hand_weapon.type       = WEAPON_BEAST;
      main_hand_weapon.swing_time = 1.5_s;  // TODO: confirm
      main_hand_weapon.min_dmg    = 60.0;   // TODO: confirm
      main_hand_weapon.max_dmg    = 60.0;   // TODO: confirm
      owner_coeff.sp_from_sp      = 1.0;
      owner_coeff.ap_from_sp      = 0.25;
    }

    void init_action_list() override
    {
      action_list_str = "travel";
      action_list_str += "/auto_attack";
      action_list_str += "/goliath_support";
      action_list_str += "/anima_cannon";
      action_list_str += "/smash";

      pet_t::init_action_list();
    }

    action_t* create_action( util::string_view name, const std::string& options_str ) override
    {
      if ( name == "travel" ) return new bron_travel_t( this );
      if ( name == "auto_attack" ) return new bron_auto_attack_t( this );
      if ( name == "goliath_support" ) return new bron_goliath_support_t( this );
      if ( name == "anima_cannon" ) return new bron_anima_cannon_t( this );
      if ( name == "smash" ) return new bron_smash_t( this );

      return pet_t::create_action( name, options_str );
    }
  };

  struct brons_call_to_action_cb_t : public dbc_proc_callback_t
  {
    timespan_t bron_dur;
    pet_t* bron;

    brons_call_to_action_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), bron_dur( e.player->find_spell( 333961 )->duration() )
    {
      bron = e.player->find_pet( "bron" );
      if ( !bron )
        bron = new bron_pet_t( e.player );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( listener->buffs.brons_call_to_action->at_max_stacks() )
      {
        listener->buffs.brons_call_to_action->expire();

        if ( bron->is_sleeping() )
          bron->summon( bron_dur);
      }
      else
        dbc_proc_callback_t::execute( a, s );
    }
  };

  effect.proc_flags_  = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.custom_buff  = effect.player->buffs.brons_call_to_action;

  new brons_call_to_action_cb_t( effect );
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

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( !a->harmful )
        return;

      if ( range::contains( target_list, s->target->actor_spawn_index ) )
        return;

      dbc_proc_callback_t::execute( a, s );
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
  effect.proc_flags_  = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.proc_flags2_ = PF2_CAST_DAMAGE | PF2_CAST_HEAL;
  effect.custom_buff  = effect.player->buffs.embody_the_construct;

  new dbc_proc_callback_t( effect.player, effect );
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
  //register_soulbind_special_effect( 331580, soulbinds::exacting_preparation );  // Nadjia
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
