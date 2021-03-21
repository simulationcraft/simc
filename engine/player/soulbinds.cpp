// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "soulbinds.hpp"

#include "player/actor_target_data.hpp"
#include "player/unique_gear_helper.hpp"
#include "player/pet.hpp"

#include "action/dot.hpp"

#include "item/item.hpp"

#include "sim/sc_sim.hpp"
#include "sim/sc_cooldown.hpp"

#include <regex>

namespace covenant
{
namespace soulbinds
{
namespace
{
struct covenant_cb_buff_t : public covenant_cb_base_t
{
  buff_t* buff;
  int stacks;

  covenant_cb_buff_t( buff_t* b, int s ) : covenant_cb_buff_t( b, true, false, s ) {}

  covenant_cb_buff_t( buff_t* b, bool on_class = true, bool on_base = false, int s = -1 )
    : covenant_cb_base_t( on_class, on_base ), buff( b ), stacks( s )
  {}

  void trigger( action_t*, action_state_t* ) override
  {
    buff->trigger( stacks );
  }
};

struct covenant_cb_action_t : public covenant_cb_base_t
{
  action_t* action;
  bool self_target;

  covenant_cb_action_t( action_t* a, bool self = false, bool on_class = true, bool on_base = false )
    : covenant_cb_base_t( on_class, on_base ), action( a ), self_target( self )
  {}

  void trigger( action_t*, action_state_t* state ) override
  {
    auto t = self_target || !state->target ? action->player : state->target;

    if ( t->is_sleeping() )
      return;

    action->set_target( t );
    action->schedule_execute();
  }
};

// Add an effect to be triggered when covenant ability is cast. Currently has has templates for buff_t & action_t, and
// can be expanded via additional subclasses to covenant_cb_base_t.
template <typename T, typename... S>
void add_covenant_cast_callback( player_t* p, S&&... args )
{
  auto callback = get_covenant_callback( p );
  if ( callback )
  {
    auto cb_entry = new T( std::forward<S>( args )... );
    callback->cb_list.push_back( cb_entry );
  }
}

double value_from_desc_vars( const special_effect_t& e, util::string_view var, util::string_view prefix = "", util::string_view postfix = "" )
{
  double value = 0;

  if ( const char* vars = e.player->dbc->spell_desc_vars( e.spell_id ).desc_vars() )
  {
    std::cmatch m;
    std::regex r( "\\$" + std::string( var ) + ".*" + std::string( prefix ) + "(\\d*\\.?\\d+)" + std::string( postfix ) );
    if ( std::regex_search( vars, m, r ) )
      value = std::stod( m.str( 1 ) );
  }

  e.player->sim->print_debug( "parsed value for special effect '{}': variable={} value={}", e.name(), var, value );

  return value;
}

// Default behavior is where the description variables string contains instances of "?a137005[${1.0}]" defined
// by a spell ID belonging to the target class and the floating point number represents the duration modifier.
// This can be changed to any regex such that the first capture group gives the class spell ID and second
// capture group gives the value to return.
double class_value_from_desc_vars( const special_effect_t& e, util::string_view var, util::string_view regex_string = "\\?a(\\d+)\\[\\$\\{(\\d*\\.?\\d+)" )
{
  double value = 0;

  if ( const char* vars = e.player->dbc->spell_desc_vars( e.spell_id ).desc_vars() )
  {
    std::cmatch m;
    std::regex r_line( "\\$" + std::string( var ) + ".*[\\r\\n]?" );
    if ( std::regex_search( vars, m, r_line ) )
    {
      const std::string line = m.str( 0 );
      std::regex r( regex_string.data(), regex_string.size() );
      std::sregex_iterator begin( line.begin(), line.end(), r );
      for( std::sregex_iterator i = begin; i != std::sregex_iterator(); i++ )
      {
        auto spell = e.player->find_spell( std::stoi( ( *i ).str( 1 ) ) );
        if ( spell->is_class( e.player->type ) )
        {
          value = std::stod( ( *i ).str( 2 ) );
          break;
        }
      }
    }
  }

  e.player->sim->print_debug( "parsed class-specific value for special effect '{}': variable={} value={}", e.name(), var, value );

  return value;
}

// By default, look for a new line being added
bool extra_desc_text_for_class( special_effect_t& e, util::string_view text = "[\r\n]" )
{
  if ( const char* desc = e.player->dbc->spell_text( e.spell_id ).desc() )
  {
    // The relevant part of the description is formatted as a '|' delimited list of the
    // letter 'a' followed by class aura spell IDs until we reach a '[' and the extra text
    std::regex r( "(?=(?:\\??a\\d+\\|?)*\\[" + std::string( text ) + ")a(\\d+)" );
    std::cregex_iterator begin( desc, desc + std::strlen( desc ), r );
    for( std::cregex_iterator i = begin; i != std::cregex_iterator(); i++ )
    {
      auto spell = e.player->find_spell( std::stoi( ( *i ).str( 1 ) ) );
      if ( spell->is_class( e.player->type ) )
      {
        e.player->sim->print_debug( "parsed description for special effect '{}': found extra text '{}' for this class", e.name(), text );
        return true;
      }
    }
  }
  e.player->sim->print_debug( "parsed description for special effect '{}': no extra text '{}' found for this class", e.name(), text );
  return false;
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
  struct spiked_burrs_t : public niyas_tools_proc_t
  {
    spiked_burrs_t( const special_effect_t& e ) :
      niyas_tools_proc_t( "spiked_burrs", e.player, e.player->find_spell( 333526 ),
                          value_from_desc_vars( e, "points", "\\$SP\\*" ), false )
    {
      // In-game, the driver (id=320659) triggers the projectile (id=321659) which triggers the ground effect
      // (id=321660) which finally triggers the dot (id=333526). Since we don't have a way of accounting for the ground
      // effects and mobs moving into it, we execute the dot directly and pull the fixed travel time from the projectile
      // spell.
      travel_delay = e.player->find_spell( 321659 )->missile_speed();
    }

    // UPDATE: Not the case anymore as of 2020-11-20 hotfixes. Keeping commented just in case.
    /*result_e calculate_result( action_state_t* s ) const override
    {
      // If the target is slow-immune (most bosses) everything gets immuned including dot application
      if ( s->target->is_boss() )
        return result_e::RESULT_MISS;

      return niyas_tools_proc_t::calculate_result( s );
    }*/
  };

  effect.execute_action = effect.player->find_action( "spiked_burrs" );
  if ( !effect.execute_action )
    effect.execute_action = new spiked_burrs_t( effect );

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
        dot = new niyas_tools_proc_t( "paralytic_poison", e.player, e.player->find_spell( 321519 ), value_from_desc_vars( e, "pointsA", "\\$SP\\*" ), false );

      direct = e.player->find_action( "paralytic_poison_interrupt" );
      if ( !direct )
        direct = new niyas_tools_proc_t( "paralytic_poison_interrupt", e.player, e.player->find_spell( 321524 ), value_from_desc_vars( e, "pointsB", "\\$SP\\*" ) );
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
  effect.custom_buff = buff_t::find( effect.player, "invigorating_herbs" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff( effect.player, "invigorating_herbs", effect.trigger() )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  }

  // TODO: confirm proc flags
  // 11/17/2020 - For Rogues this procs from all periodic heals (Recuperator/Soothing Darkness/Crimson Vial)
  effect.proc_flags_ = PF_ALL_HEAL | PF_PERIODIC;
  effect.proc_flags2_ = PF2_LANDED | PF2_PERIODIC_HEAL;

  new dbc_proc_callback_t( effect.player, effect );
}

void grove_invigoration( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "redirected_anima" } ) )
    return;

  effect.custom_buff = buff_t::find( effect.player, "redirected_anima" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff =
      make_buff<stat_buff_t>( effect.player, "redirected_anima", effect.player->find_spell( 342814 ) )
        ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );
  }

  new dbc_proc_callback_t( effect.player, effect );

  double stack_mod = class_value_from_desc_vars( effect, "mod" );
  effect.player->sim->print_debug( "class-specific properties for grove_invigoration: stack_mod={}", stack_mod );

  int stacks = as<int>( effect.driver()->effectN( 3 ).base_value() * stack_mod );

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, effect.custom_buff, stacks );
}

void field_of_blossoms( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "field_of_blossoms" } ) )
    return;

  if ( !effect.player->find_soulbind_spell( effect.driver()->name_cstr() )->ok() )
    return;

  auto buff = buff_t::find( effect.player, "field_of_blossoms" );
  if ( !buff )
  {
    double duration_mod = class_value_from_desc_vars( effect, "mod" );
    effect.player->sim->print_debug( "class-specific properties for field_of_blossoms: duration_mod={}", duration_mod );

    buff = make_buff( effect.player, "field_of_blossoms", effect.player->find_spell( 342774 ) )
      // the stat buff id=342774 has 15s duration, but the ground effect spell id=342761 only has a 10s duration
      ->set_duration( effect.player->find_spell( 342761 )->duration() )
      ->set_duration_multiplier( duration_mod )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  }

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff );
}

void social_butterfly( special_effect_t& effect )
{
  // ID: 320130 is the buff on player
  // ID: 320212 is the buff on allies (NYI)
  struct social_butterfly_buff_t : public buff_t
  {
    timespan_t ally_buff_dur;

    social_butterfly_buff_t( player_t* p )
      : buff_t( p, "social_butterfly", p->find_spell( 320130 ) ), ally_buff_dur( p->find_spell( 320212 )->duration() )
    {
      set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT );
      set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );
    }

    void expire_override( int s, timespan_t d ) override
    {
      buff_t::expire_override( s, d );

      make_event( *sim, ally_buff_dur, [ this ]() { trigger(); } );
    }
  };

  auto buff = buff_t::find( effect.player, "social_butterfly" );
  if ( !buff )
    buff = make_buff<social_butterfly_buff_t>( effect.player );
  effect.player->register_combat_begin( buff );
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

  effect.custom_buff = buff_t::find( effect.player, "first_strike" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff( effect.player, "first_strike", effect.player->find_spell( 325381 ) )
      ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
      ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
  }

  // The effect does not actually proc on periodic damage at all.
  effect.proc_flags_ = effect.proc_flags() & ~PF_PERIODIC;

  // The effect procs when damage actually happens.
  effect.proc_flags2_ = PF2_ALL_HIT;

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

    t->register_on_demise_callback( p, [ p, cb ]( player_t* t ) {
      if ( p->sim->event_mgr.canceled )
        return;

      auto td = p->find_target_data( t );
      if ( td && td->debuff.adversary->check() )
        cb->activate();
    } );
  } );
}

void thrill_seeker( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "thrill_seeker", "euphoria" } ) )
    return;

  auto counter_buff = buff_t::find( effect.player, "thrill_seeker" );
  if ( !counter_buff )
  {
    counter_buff = make_buff( effect.player, "thrill_seeker", effect.player->find_spell( 331939 ) )
                       ->set_period( 0_ms )
                       ->set_tick_behavior( buff_tick_behavior::NONE );
  }

  auto buff = buff_t::find( effect.player, "euphoria" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "euphoria", effect.player->find_spell( 331937 ) )
               ->set_default_value_from_effect_type( A_HASTE_ALL )
               ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );

    counter_buff->set_stack_change_callback( [ buff ]( buff_t* b, int, int ) {
      if ( b->at_max_stacks() )
      {
        buff->trigger();
        b->expire();
      }
    } );
  }

  auto eff_data = &effect.driver()->effectN( 1 );

  // You still gain stacks while euphoria is active
  effect.player->register_combat_begin( [ eff_data, counter_buff ]( player_t* p ) {
    make_repeating_event( *p->sim, eff_data->period(), [ counter_buff ] { counter_buff->trigger(); } );
  } );

  auto p                     = effect.player;
  int killing_blow_stacks    = as<int>( p->find_spell( 331586 )->effectN( 1 ).base_value() );
  double killing_blow_chance = p->sim->shadowlands_opts.thrill_seeker_killing_blow_chance;
  int number_of_players      = 1;
  // If the user does not override the value for this we will set different defaults based on the sim here
  // Default: 1/20 = 0.05
  // DungeonSlice: 1/4 = 0.25
  if ( killing_blow_chance < 0 )
  {
    if ( effect.player->sim->fight_style == "DungeonSlice" )
    {
      number_of_players = 4;
    }
    else
    {
      number_of_players = 20;
    }
    killing_blow_chance = 1.0 / number_of_players;
  }

  range::for_each( p->sim->actor_list, [ p, counter_buff, killing_blow_stacks, killing_blow_chance ]( player_t* t ) {
    if ( !t->is_enemy() )
      return;

    t->register_on_demise_callback( p, [ p, counter_buff, killing_blow_stacks, killing_blow_chance ]( player_t* ) {
      if ( p->sim->event_mgr.canceled )
        return;

      if ( p->rng().roll( killing_blow_chance ) )
      {
        counter_buff->trigger( killing_blow_stacks );
      }
    } );
  } );
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
  auto buff = buff_t::find( effect.player, "wasteland_propriety" );
  if ( !buff )
  {
    // The duration modifier for each class comes from the description variables of Wasteland Propriety (id=319983)
    double duration_mod = class_value_from_desc_vars( effect, "mod" );

    // The ICD of 60 seconds is enabled for some classes in the description of Wasteland Propriety (id=319983)
    bool icd_enabled = extra_desc_text_for_class( effect );

    effect.player->sim->print_debug( "class-specific properties for wasteland_propriety: duration_mod={}, icd_enabled={}", duration_mod, icd_enabled );

    buff = make_buff( effect.player, "wasteland_propriety", effect.player->find_spell( 333218 ) )
      ->set_cooldown( icd_enabled ? effect.player->find_spell( 333221 )->duration() : 0_ms )
      ->set_duration_multiplier( duration_mod )
      ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT )
      ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );
  }

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff );
}

void built_for_war( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "built_for_war" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "built_for_war", effect.player->find_spell( 332842 ) )
      ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
      ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
      ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
      ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
  }

  auto eff_data = &effect.driver()->effectN( 1 );

  effect.player->register_combat_begin( [ eff_data, buff ]( player_t* p ) {
    make_repeating_event( *p->sim, eff_data->period(), [ p, eff_data, buff ]() {
      if ( p->health_percentage() > eff_data->base_value() )
        buff->trigger();
    } );
  } );

  // TODO: add option to simulate losing the buff from going below 50% hp
}

void superior_tactics( special_effect_t& effect )
{
  // TODO: implement proc'ing from dispels
  effect.proc_flags2_ |= PF2_CAST_INTERRUPT;

  effect.custom_buff = buff_t::find( effect.player, "superior_tactics" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff( effect.player, "superior_tactics", effect.trigger() )
      ->set_cooldown( effect.trigger()->effectN( 2 ).trigger()->duration() )
      ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
      ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
  }

  new dbc_proc_callback_t( effect.player, effect );
}

void let_go_of_the_past( special_effect_t& effect )
{
  struct let_go_of_the_past_cb_t : public dbc_proc_callback_t
  {
    unsigned prev_id;

    let_go_of_the_past_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), prev_id( 0 ) {}

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( !a->id || a->id == prev_id || a->background || a->trigger_gcd == 0_ms )
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

  effect.proc_flags_ = PF_ALL_DAMAGE;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;

  // TODO: currently this only sets the buffs, but doesn't check for the buff in player_t::target_mitigation(). Possibly
  // we want to consolidate that into vectors on the player like with stat_pct_buff so we don't have to unnecessarily
  // pollute player_t further.
  effect.custom_buff = buff_t::find( effect.player, "let_go_of_the_past" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff( effect.player, "let_go_of_the_past", effect.player->find_spell( 328900 ) )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
      ->set_schools_from_effect( 1 );
  }

  new let_go_of_the_past_cb_t( effect );
}

void combat_meditation( special_effect_t& effect )
{
  // id:328908 buff spell
  // id:328917 sorrowful memories projectile (buff->eff#2->trigger)
  // id:328913 sorrowful memories duration, extension value in eff#2 (projectile->eff#1->trigger)
  // id:345861 lockout buff
  struct combat_meditation_buff_t : public stat_buff_t
  {
    timespan_t ext_dur;
    combat_meditation_buff_t( player_t* p, double duration_mod, double duration_mod_ext, bool icd_enabled ) : stat_buff_t( p, "combat_meditation", p->find_spell( 328908 ) )
    {
      set_cooldown( icd_enabled ? p->find_spell( 345861 )->duration() : 0_ms );
      set_refresh_behavior( buff_refresh_behavior::EXTEND );
      set_duration_multiplier( duration_mod );

      ext_dur = duration_mod_ext * timespan_t::from_seconds( p->find_spell( 328913 )->effectN( 2 ).base_value() );

      // TODO: add more faithful simulation of delay/reaction needed from player to walk into the sorrowful memories
      set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
        if ( current_tick <= 3 && rng().roll( sim->shadowlands_opts.combat_meditation_extend_chance ) )
          extend_duration( player, ext_dur );
      } );
    }
  };

  auto buff = buff_t::find( effect.player, "combat_meditation" );
  if ( !buff )
  {
    double duration_mod = class_value_from_desc_vars( effect, "mod" );
    double duration_mod_ext = class_value_from_desc_vars( effect, "modb" );
    bool icd_enabled = extra_desc_text_for_class( effect, effect.driver()->name_cstr() );

    effect.player->sim->print_debug( "class-specific properties for combat_meditation: duration_mod={}, duration_mod_ext={}, icd_enabled={}", duration_mod, duration_mod_ext, icd_enabled );
    buff = make_buff<combat_meditation_buff_t>( effect.player, duration_mod, duration_mod_ext, icd_enabled );
  }
  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff );
}

void pointed_courage( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "pointed_courage" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "pointed_courage", effect.player->find_spell( 330511 ) )
      ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
      ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
      // TODO: add better handling of allies/enemies nearby mechanic which is checked every tick. tick is disabled
      // for now
      ->set_period( 0_ms );
  }

  effect.player->register_combat_begin( [ buff ]( player_t* p ) {
    buff->trigger( p->sim->shadowlands_opts.pointed_courage_nearby );
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

  effect.custom_buff = buff_t::find( effect.player, "hammer_of_genesis" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff( effect.player, "hammer_of_genesis", effect.player->find_spell( 333943 ) )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  }

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;

  new hammer_of_genesis_cb_t( effect );
}

void brons_call_to_action( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "brons_call_to_action" } ) )
    return;

  struct bron_pet_t : public pet_t
  {
    struct bron_anima_cannon_t : public spell_t
    {
      bron_anima_cannon_t( pet_t* p, const std::string& options_str ) : spell_t( "anima_cannon", p, p->find_spell( 332525 ) )
      {
        parse_options( options_str );

        interrupt_auto_attack = false;
        spell_power_mod.direct = 0.55; // Not in spell data
      }
    };

    struct bron_smash_damage_t : public spell_t
    {
      bron_smash_damage_t( pet_t* p ) : spell_t( "smash", p, p->find_spell( 341165 ) )
      {
        background = true;
        spell_power_mod.direct = 0.25; // Not in spell data
        attack_power_mod.direct = 0.25; // Not in spell data
        aoe = -1;
        radius = data().effectN( 1 ).radius_max();
      }

      double attack_direct_power_coefficient( const action_state_t* s ) const override
      {
        auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
        auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

        if ( ap <= sp )
          return 0;

        return spell_t::attack_direct_power_coefficient( s );
      }

      double spell_direct_power_coefficient( const action_state_t* s ) const override
      {
        auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
        auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

        if ( ap > sp )
          return 0;

        return spell_t::spell_direct_power_coefficient( s );
      }
    };

    struct bron_smash_t : public spell_t
    {
      bron_smash_t( pet_t* p, const std::string& options_str ) : spell_t( "smash_cast", p, p->find_spell( 341163 ) )
      {
        parse_options( options_str );

        impact_action = new bron_smash_damage_t( p );
      }
    };

    struct bron_vitalizing_bolt_t : public heal_t
    {
      bron_vitalizing_bolt_t( pet_t* p, const std::string& options_str ) : heal_t( "vitalizing_bolt", p, p->find_spell( 332526 ) )
      {
        parse_options( options_str );

        interrupt_auto_attack = false;
        spell_power_mod.direct = 0.575; // Not in spell data
      }

      void execute() override
      {
        target = debug_cast<pet_t*>( player )->owner;

        heal_t::execute();
      }
    };

    struct bron_melee_t : public melee_attack_t
    {
      bron_melee_t( pet_t* p ) : melee_attack_t( "melee", p, spell_data_t::nil() )
      {
        background = repeating = may_crit = may_glance = true;

        school            = SCHOOL_PHYSICAL;
        weapon            = &p->main_hand_weapon;
        weapon_multiplier = 0.25;
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
      main_hand_weapon.swing_time = 2.0_s;
      owner_coeff.sp_from_sp      = 2.0;
      owner_coeff.ap_from_ap      = 2.0;
    }

    void init_action_list() override
    {
      // Use line cooldowns to recreate the in-game timings; the spell data does not actually
      // have cooldowns and the apparent cooldowns need to start on cast start to work.
      // TODO: Vitalizing Bolt casts one more time than it should, but it is not a damage
      // spell and the number of hits for all of the damage spells is correct. There might
      // need to be some kind of delay when selecting actions to get the timing to match up.
      action_list_str = "travel";
      action_list_str += "/auto_attack";
      action_list_str += "/vitalizing_bolt,line_cd=4";
      action_list_str += "/anima_cannon,line_cd=8";
      action_list_str += "/smash,line_cd=15";

      pet_t::init_action_list();
    }

    action_t* create_action( util::string_view name, const std::string& options_str ) override
    {
      if ( name == "travel" ) return new bron_travel_t( this );
      if ( name == "auto_attack" ) return new bron_auto_attack_t( this );
      if ( name == "vitalizing_bolt" ) return new bron_vitalizing_bolt_t( this, options_str );
      if ( name == "anima_cannon" ) return new bron_anima_cannon_t( this, options_str );
      if ( name == "smash" ) return new bron_smash_t( this, options_str );

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
      if ( proc_buff->at_max_stacks() )
      {
        proc_buff->expire();

        if ( bron->is_sleeping() )
          bron->summon( bron_dur);
      }
      else
        dbc_proc_callback_t::execute( a, s );
    }
  };

  // TODO: This does not seem to proc on all of the spells implied by these proc flags.
  // For example, Arcane Missiles does not trigger a stack of the buff.
  effect.proc_flags_  = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;

  effect.custom_buff = buff_t::find( effect.player, "brons_call_to_action" );
  if ( !effect.custom_buff )
    effect.custom_buff = make_buff( effect.player, "brons_call_to_action", effect.player->find_spell( 332514 ) );

  new brons_call_to_action_cb_t( effect );
}

// 323491: humanoid (mastery rating)
// 323498: beast (primary stat)
// 323502: dragonkin (crit rating)
// 323504: elemental (magic damage)
// 323506: giant (physical damage)
void volatile_solvent( special_effect_t& effect )
{
  auto opt_str = effect.player->sim->shadowlands_opts.volatile_solvent_type;
  if ( util::str_compare_ci( opt_str, "none" ) )
    return;

  auto splits = util::string_split<util::string_view>( opt_str, "/:" );

  for ( auto type_str : splits )
  {
    auto race_type = util::parse_race_type( type_str );
    if ( race_type == RACE_UNKNOWN )
    {
      if      ( util::str_compare_ci( type_str, "mastery"  ) ) race_type = RACE_HUMANOID;
      else if ( util::str_compare_ci( type_str, "primary"  ) ) race_type = RACE_BEAST;
      else if ( util::str_compare_ci( type_str, "crit"     ) ) race_type = RACE_DRAGONKIN;
      else if ( util::str_compare_ci( type_str, "magic"    ) ) race_type = RACE_ELEMENTAL;
      else if ( util::str_compare_ci( type_str, "physical" ) ) race_type = RACE_GIANT;
    }

    buff_t* buff;

    switch ( race_type )
    {
      case RACE_HUMANOID:
        buff = buff_t::find( effect.player, "volatile_solvent_humanoid" );
        if ( !buff )
        {
          buff =
            make_buff<stat_buff_t>( effect.player, "volatile_solvent_humanoid", effect.player->find_spell( 323491 ) );
        }
        break;

      case RACE_BEAST:
        buff = buff_t::find( effect.player, "volatile_solvent_beast" );
        if ( !buff )
        {
          buff = make_buff( effect.player, "volatile_solvent_beast", effect.player->find_spell( 323498 ) )
                  ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                  ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
                  ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
                  ->set_default_value_from_effect_type( A_MOD_PERCENT_STAT );
        }
        break;

      case RACE_DRAGONKIN:
        buff = buff_t::find( effect.player, "volatile_solvent_dragonkin" );
        if ( !buff )
        {
          buff = make_buff( effect.player, "volatile_solvent_dragonkin", effect.player->find_spell( 323502 ) )
                  ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                  ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE );
        }
        break;

      case RACE_ELEMENTAL:
        buff = buff_t::find( effect.player, "volatile_solvent_elemental" );
        if ( !buff )
        {
          buff = make_buff( effect.player, "volatile_solvent_elemental", effect.player->find_spell( 323504 ) )
                  ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
                  ->set_schools_from_effect( 1 );
        }
        effect.player->buffs.volatile_solvent_damage = buff;
        break;

      case RACE_GIANT:
        buff = buff_t::find( effect.player, "volatile_solvent_giant" );
        if ( !buff )
        {
          buff = make_buff( effect.player, "volatile_solvent_giant", effect.player->find_spell( 323506 ) )
                  ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
                  ->set_schools_from_effect( 2 );
        }
        effect.player->buffs.volatile_solvent_damage = buff;
        break;

      default: buff = nullptr; break;
    }

    if ( buff )
      effect.player->register_combat_begin( buff );
    else
      effect.player->sim->error( "Warning: Invalid type '{}' for Volatile Solvent, ignoring.", type_str );
  }
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
  auto buff = buff_t::find( effect.player, "gnashing_chompers" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "gnashing_chompers", effect.player->find_spell( 324242 ) )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
      ->set_period( 0_ms )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  }

  range::for_each( effect.player->sim->actor_list, [ buff ]( player_t* p ) {
    if ( !p->is_enemy() )
      return;

    p->register_on_demise_callback( buff->player, [ buff ]( player_t* ) {
      if ( buff->sim->event_mgr.canceled )
        return;

      buff->trigger();
    } );
  } );
}

void lead_by_example( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "lead_by_example" } ) )
    return;

  auto buff = buff_t::find( effect.player, "lead_by_example" );
  if ( !buff )
  {
    auto s_data = effect.player->find_spell( 342181 );
    double duration = effect.driver()->effectN( 3 ).base_value();

    // The duration modifier for each class comes from the description variables of Lead by Example (id=342156)
    duration *= class_value_from_desc_vars( effect, "mod" );

    int allies_nearby = effect.player->sim->shadowlands_opts.lead_by_example_nearby;
    // If the user doesn't specify a number of allies affected by LbA, use default values based on position and fight style
    if ( allies_nearby < 0 )
    {
      switch( effect.player -> position() )
      {
        // Assume that players right in front or at the back of the boss have enough allies nearby to get full effect
        case POSITION_BACK:
        case POSITION_FRONT:
          // For DungeonSlice, always assume two allies
          if ( util::str_compare_ci( effect.player -> sim -> fight_style, "DungeonSlice" ) )
            allies_nearby = 2;
          else
            allies_nearby = 4;
          break;
        // Assume two nearby allies for other positions
        default:
          allies_nearby = 2;
          break;
      }
    }

    buff = make_buff( effect.player, "lead_by_example", s_data )
      ->set_default_value_from_effect( 1 )
      ->modify_default_value( s_data->effectN( 2 ).percent() * allies_nearby )
      ->set_duration( timespan_t::from_seconds( duration ) )
      ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
      ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
      ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
  }

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff );
}

void forgeborne_reveries( special_effect_t& effect )
{
  int count = 0;

  for ( const auto& item : effect.player->items )
  {
    if ( item.slot < SLOT_HEAD || item.slot > SLOT_BACK )
      continue;

    if ( item.parsed.enchant_id )
    {
      // All armor enchants currently have a stat associated with them, unlike temporaries & weapon enchants which
      // can be purely procs. So for now we should be safe filtering for enchants with ITEM_ENCHANTMENT_STAT.
      auto ench = effect.player->dbc->item_enchantment( item.parsed.enchant_id );
      if ( range::contains( ench.ench_type, ITEM_ENCHANTMENT_STAT ) )
        count++;

      if ( count >= 3 )  // hardcoded into tooltip desc
        break;
    }
  }

  if ( count == 0 ) {
    // no enchants are applied, do not apply the buff
    //
    // this is done because the buff code replaces a multiplier of 0 with the
    // default value from the spelldata---which in this case is 1, or a 100%
    // increase in stats
    return;
  }

  auto buff = buff_t::find( effect.player, "forgeborne_reveries" );
  if ( !buff )
  {
    // armor increase NYI
    buff = make_buff( effect.player, "forgeborne_reveries", effect.player->find_spell( 348272 ) )
      ->set_default_value_from_effect( 1, count * 0.01 )
      ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
      ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
      ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
  }

  effect.player->register_combat_begin( buff );
}

void serrated_spaulders( special_effect_t& effect )
{

}

void heirmirs_arsenal_marrowed_gemstone( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "marrowed_gemstone_charging", "marrowed_gemstone_enhancement" } ) )
    return;

  struct marrowed_gemstone_cb_t : public dbc_proc_callback_t
  {
    buff_t* buff;

    marrowed_gemstone_cb_t( const special_effect_t& e, buff_t* b ) : dbc_proc_callback_t( e.player, e ), buff( b )
    {}

    // cooldown applies to both the buff AND the counter, so don't trigger if buff is on cd
    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( buff->cooldown->down() )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  auto counter_buff = buff_t::find( effect.player, "marrowed_gemstone_charging" );
  if ( !counter_buff )
    counter_buff = make_buff( effect.player, "marrowed_gemstone_charging", effect.player->find_spell( 327066 ) )
      ->modify_max_stack( 1 );

  auto buff = buff_t::find( effect.player, "marrowed_gemstone_enhancement" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "marrowed_gemstone_enhancement", effect.player->find_spell( 327069 ) )
      ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
      ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
    buff->set_cooldown( buff->buff_duration() + effect.player->find_spell( 327073 )->duration() );

    counter_buff->set_stack_change_callback( [ buff ] ( buff_t* b, int, int )
    {
      if ( b->at_max_stacks() )
      {
        buff->trigger();
        b->expire();
      }
    } );
  }

  effect.proc_flags2_ = PF2_CRIT;
  effect.custom_buff = counter_buff;

  new marrowed_gemstone_cb_t( effect, buff );
}

// Helper function for registering an effect, with autoamtic skipping initialization if soulbind spell is not available
void register_soulbind_special_effect( unsigned spell_id, const custom_cb_t& init_callback, bool fallback = false )
{
  unique_gear::register_special_effect( spell_id, [ &, init_callback ] ( special_effect_t& effect ) {
    if ( effect.source != SPECIAL_EFFECT_SOURCE_FALLBACK && !effect.player->find_soulbind_spell( effect.driver()->name_cstr() )->ok() )
      return;
    init_callback( effect );
  }, fallback );
}

}  // namespace

void register_special_effects()
{
  // Night Fae
  register_soulbind_special_effect( 320659, soulbinds::niyas_tools_burrs );  // Niya
  register_soulbind_special_effect( 320660, soulbinds::niyas_tools_poison );
  register_soulbind_special_effect( 320662, soulbinds::niyas_tools_herbs );
  register_soulbind_special_effect( 322721, soulbinds::grove_invigoration, true );
  register_soulbind_special_effect( 319191, soulbinds::field_of_blossoms, true );  // Dreamweaver
  register_soulbind_special_effect( 319210, soulbinds::social_butterfly );
  register_soulbind_special_effect( 325069, soulbinds::first_strike );  // Korayn
  register_soulbind_special_effect( 325066, soulbinds::wild_hunt_tactics );
  // Venthyr
  //register_soulbind_special_effect( 331580, soulbinds::exacting_preparation );  // Nadjia
  register_soulbind_special_effect( 331584, soulbinds::dauntless_duelist );
  register_soulbind_special_effect( 331586, soulbinds::thrill_seeker, true );
  register_soulbind_special_effect( 336239, soulbinds::soothing_shade );  // Theotar
  register_soulbind_special_effect( 319983, soulbinds::wasteland_propriety );
  register_soulbind_special_effect( 319973, soulbinds::built_for_war );  // Draven
  register_soulbind_special_effect( 332753, soulbinds::superior_tactics );
  // Kyrian
  register_soulbind_special_effect( 328257, soulbinds::let_go_of_the_past );  // Pelagos
  register_soulbind_special_effect( 328266, soulbinds::combat_meditation );
  register_soulbind_special_effect( 329778, soulbinds::pointed_courage );    // Kleia
  register_soulbind_special_effect( 333935, soulbinds::hammer_of_genesis );  // Mikanikos
  register_soulbind_special_effect( 333950, soulbinds::brons_call_to_action, true );
  // Necrolord
  register_soulbind_special_effect( 323074, soulbinds::volatile_solvent );  // Marileth
  register_soulbind_special_effect( 323090, soulbinds::plagueys_preemptive_strike );
  register_soulbind_special_effect( 323919, soulbinds::gnashing_chompers );  // Emeni
  register_soulbind_special_effect( 342156, soulbinds::lead_by_example, true );
  register_soulbind_special_effect( 326514, soulbinds::forgeborne_reveries );  // Heirmir
  register_soulbind_special_effect( 326504, soulbinds::serrated_spaulders );
  register_soulbind_special_effect( 326572, soulbinds::heirmirs_arsenal_marrowed_gemstone, true );
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
