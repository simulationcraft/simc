// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "soulbinds.hpp"

#include "action/dot.hpp"
#include "action/sc_action_state.hpp"
#include "item/item.hpp"
#include "player/actor_target_data.hpp"
#include "player/covenant.hpp"
#include "player/pet.hpp"
#include "player/unique_gear_helper.hpp"
#include "sim/sc_cooldown.hpp"
#include "sim/sc_sim.hpp"
#include "util/util.hpp"

#include <regex>

#include "simulationcraft.hpp"

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

  covenant_cb_buff_t( buff_t* b, int s ) : covenant_cb_buff_t( b, true, false, s )
  {
  }

  covenant_cb_buff_t( buff_t* b, bool on_class = true, bool on_base = false, int s = -1 )
    : covenant_cb_base_t( on_class, on_base ), buff( b ), stacks( s )
  {
  }

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

double value_from_desc_vars( const special_effect_t& e, util::string_view var, util::string_view prefix = "",
                             util::string_view postfix = "" )
{
  double value = 0;

  if ( const char* vars = e.player->dbc->spell_desc_vars( e.spell_id ).desc_vars() )
  {
    std::cmatch m;
    std::regex r( "\\$" + std::string( var ) + ".*" + std::string( prefix ) + "(\\d*\\.?\\d+)" +
                  std::string( postfix ) );
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
double class_value_from_desc_vars( const special_effect_t& e, util::string_view var,
                                   util::string_view regex_string = "\\?a(\\d+)\\[\\$\\{(\\d*\\.?\\d+)" )
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
      for ( std::sregex_iterator i = begin; i != std::sregex_iterator(); i++ )
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

  e.player->sim->print_debug( "parsed class-specific value for special effect '{}': variable={} value={}", e.name(),
                              var, value );

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
    for ( std::cregex_iterator i = begin; i != std::cregex_iterator(); i++ )
    {
      auto spell = e.player->find_spell( std::stoi( ( *i ).str( 1 ) ) );
      if ( spell->is_class( e.player->type ) )
      {
        e.player->sim->print_debug( "parsed description for special effect '{}': found extra text '{}' for this class",
                                    e.name(), text );
        return true;
      }
    }
  }
  e.player->sim->print_debug( "parsed description for special effect '{}': no extra text '{}' found for this class",
                              e.name(), text );
  return false;
}

struct niyas_tools_proc_t : public unique_gear::proc_spell_t
{
  niyas_tools_proc_t( util::string_view n, player_t* p, const spell_data_t* s, double mod, bool direct = true )
    : proc_spell_t( n, p, s )
  {
    if ( direct )
      spell_power_mod.direct = mod;
    else
      spell_power_mod.tick = mod;
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
    spiked_burrs_t( const special_effect_t& e )
      : niyas_tools_proc_t( "spiked_burrs", e.player, e.player->find_spell( 333526 ),
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
        dot = new niyas_tools_proc_t( "paralytic_poison", e.player, e.player->find_spell( 321519 ),
                                      value_from_desc_vars( e, "pointsA", "\\$SP\\*" ), false );

      direct = e.player->find_action( "paralytic_poison_interrupt" );
      if ( !direct )
        direct = new niyas_tools_proc_t( "paralytic_poison_interrupt", e.player, e.player->find_spell( 321524 ),
                                         value_from_desc_vars( e, "pointsB", "\\$SP\\*" ) );
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
  effect.proc_flags_  = PF_ALL_HEAL | PF_PERIODIC;
  effect.proc_flags2_ = PF2_LANDED | PF2_PERIODIC_HEAL;

  new dbc_proc_callback_t( effect.player, effect );
}

void grove_invigoration( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "redirected_anima" } ) )
    return;

  auto buff = buff_t::find( effect.player, "redirected_anima" );

  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "redirected_anima", effect.player->find_spell( 342814 ) )
               ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
               ->set_default_value_from_effect( 1 )  // default value is used to hold the hp %
               ->set_stack_change_callback( [ effect ]( buff_t*, int /* old */, int /* cur */ ) {
                 effect.player->recalculate_resource_max( RESOURCE_HEALTH );
               } );
  }

  effect.player->buffs.redirected_anima = buff;
  effect.custom_buff                    = buff;

  new dbc_proc_callback_t( effect.player, effect );

  double stack_mod = class_value_from_desc_vars( effect, "mod" );
  effect.player->sim->print_debug( "class-specific properties for grove_invigoration: stack_mod={}", stack_mod );

  int stacks = as<int>( effect.driver()->effectN( 3 ).base_value() * stack_mod );

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff, stacks );
}

void bonded_hearts( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "bonded_hearts" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "bonded_hearts", effect.player->find_spell( 352881 ) )
               ->add_invalidate( CACHE_MASTERY )
               ->set_default_value_from_effect( 1 );

    buff->set_stack_change_callback( [ effect ]( buff_t* b, int, int new_ ) {
      auto ra  = debug_cast<stat_buff_t*>( effect.player->buffs.redirected_anima );
      auto mul = 1.0 + b->default_value;

      // In stat_buff_t::bump, the stat value is applied AFTER buff_t::stack_change_callback is triggered, so the
      // sequence will be:
      //  1. redirected_anima is triggered
      //  2. bonded_hearts is triggered via stack_change_callback on redirected_anima
      //  3. the stat value of redirected_anima is applied
      // This means that when bonded_hearts is triggered, the amount will be adjusted before the stat buff is applied,
      // so we don't need to do any manual adjusting of the stat buff amoutn. But when bonded_hearts ends, we will
      // need to manually recalculate and update the stat buff amount.
      if ( new_ )
      {
        for ( auto& s : ra->stats )
        {
          s.amount *= mul;
        }

        ra->default_value *= mul;
      }
      else
      {
        for ( auto& s : ra->stats )
        {
          s.amount /= mul;

          double delta = s.amount * ra->current_stack - s.current_value;

          if ( delta > 0 )
            b->player->stat_gain( s.stat, delta, ra->stat_gain, nullptr, ra->buff_duration() > 0_ms );
          else if ( delta < 0 )
            b->player->stat_loss( s.stat, std::fabs( delta ), ra->stat_gain, nullptr, ra->buff_duration() > 0_ms );

          s.current_value += delta;
        }

        ra->default_value /= mul;
      }

      effect.player->recalculate_resource_max( RESOURCE_HEALTH );
    } );
  }

  effect.player->register_on_arise_callback( effect.player, [ effect, buff ]() {
    auto ra = effect.player->buffs.redirected_anima;
    if ( ra )
    {
      ra->set_stack_change_callback( [ buff ]( buff_t*, int old_, int new_ ) {
        if ( new_ > old_ && buff->rng().roll( buff->sim->shadowlands_opts.bonded_hearts_other_covenant_chance ) )
          buff->trigger();

        buff->player->recalculate_resource_max( RESOURCE_HEALTH );
      } );
    }
  } );
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

    buff =
        make_buff( effect.player, "field_of_blossoms", effect.player->find_spell( 342774 ) )
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

void dream_delver( special_effect_t& effect )
{
  struct dream_delver_cb_t : public dbc_proc_callback_t
  {
    dream_delver_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      dbc_proc_callback_t::execute( a, s );

      auto td = a->player->get_target_data( s->target );
      td->debuff.dream_delver->trigger();
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;

  new dream_delver_cb_t( effect );
}

void first_strike( special_effect_t& effect )
{
  if ( unique_gear::create_fallback_buffs( effect, { "first_strike" } ) )
    return;

  struct first_strike_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    first_strike_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), target_list()
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
    effect.player->buffs.wild_hunt_tactics =
        make_buff( effect.player, "wild_hunt_tactics", effect.driver() )->set_default_value_from_effect( 1 );
}
// Handled in unique_gear_shadowlands.cpp
// void exacting_preparation( special_effect_t& effect ) {}

void dauntless_duelist( special_effect_t& effect )
{
  struct dauntless_duelist_cb_t : public dbc_proc_callback_t
  {
    dauntless_duelist_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e )
    {
    }

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

  struct euphoria_buff_t : public buff_t
  {
    euphoria_buff_t( player_t* p ) : buff_t( p, "euphoria", p->find_spell( 331937 ) )
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }

    void expire_override( int s, timespan_t d ) override
    {
      buff_t::expire_override( s, d );

      auto crit = player->cache.spell_crit_chance();
      auto vers = player->cache.damage_versatility();

      // TODO: implement what happens if vers = crit
      if ( crit > vers )
      {
        if ( player->buffs.fatal_flaw_crit )
        {
          player->buffs.fatal_flaw_crit->trigger();
        }
      }
      else if ( vers > crit )
      {
        if ( player->buffs.fatal_flaw_vers )
        {
          player->buffs.fatal_flaw_vers->trigger();
        }
      }
    }
  };

  auto buff = buff_t::find( effect.player, "euphoria" );
  if ( !buff )
  {
    buff = make_buff<euphoria_buff_t>( effect.player );

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

// Critical Strike - 354053
// Versatility - 354054
void fatal_flaw( special_effect_t& effect )
{
  auto crit_buff = buff_t::find( effect.player, "fatal_flaw_crit" );
  auto vers_buff = buff_t::find( effect.player, "fatal_flaw_vers" );
  if ( !crit_buff )
  {
    crit_buff = make_buff( effect.player, "fatal_flaw_crit", effect.player->find_spell( 354053 ) )
                    ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
                    ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
  }
  if ( !vers_buff )
  {
    vers_buff = make_buff( effect.player, "fatal_flaw_vers", effect.player->find_spell( 354054 ) )
                    ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT )
                    ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );
  }

  effect.player->buffs.fatal_flaw_crit = crit_buff;
  effect.player->buffs.fatal_flaw_vers = vers_buff;
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

    effect.player->sim->print_debug(
        "class-specific properties for wasteland_propriety: duration_mod={}, icd_enabled={}", duration_mod,
        icd_enabled );

    buff = make_buff( effect.player, "wasteland_propriety", effect.player->find_spell( 333218 ) )
               ->set_cooldown( icd_enabled ? effect.player->find_spell( 333221 )->duration() : 0_ms )
               ->set_duration_multiplier( duration_mod )
               ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT )
               ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );
  }

  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff );
}

// 354017 - Critical Strike
// 353266 - Primary Stat
// 354016 - Haste
// 354018 - Versatility
void party_favors( special_effect_t& effect )
{
  auto opt_str = effect.player->sim->shadowlands_opts.party_favor_type;
  if ( util::str_compare_ci( opt_str, "none" ) )
    return;

  buff_t* buff;

  // TODO: Figure out if you can have multiple buffs at a time
  if ( util::str_compare_ci( opt_str, "haste" ) )
  {
    buff = buff_t::find( effect.player, "the_mad_dukes_tea_haste" );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, "the_mad_dukes_tea_haste", effect.player->find_spell( 354016 ) )
                 ->set_default_value_from_effect_type( A_HASTE_ALL )
                 ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    }
  }
  else if ( util::str_compare_ci( opt_str, "crit" ) )
  {
    buff = buff_t::find( effect.player, "the_mad_dukes_tea_crit" );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, "the_mad_dukes_tea_crit", effect.player->find_spell( 354017 ) )
                 ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                 ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE );
    }
  }
  else if ( util::str_compare_ci( opt_str, "primary" ) )
  {
    buff = buff_t::find( effect.player, "the_mad_dukes_tea_primary" );
    if ( !buff )
    {
      buff = make_buff<stat_buff_t>( effect.player, "the_mad_dukes_tea_primary", effect.player->find_spell( 353266 ) )
                 ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                 ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
                 ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
                 ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE );
    }
  }
  else if ( util::str_compare_ci( opt_str, "versatility" ) )
  {
    buff = buff_t::find( effect.player, "the_mad_dukes_tea_versatility" );
    if ( !buff )
    {
      buff =
          make_buff<stat_buff_t>( effect.player, "the_mad_dukes_tea_versatility", effect.player->find_spell( 354018 ) )
              ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY )
              ->set_default_value_from_effect_type( A_MOD_VERSATILITY_PCT );
    }
  }
  else
  {
    buff = nullptr;
  }

  if ( buff )
    effect.player->register_combat_begin( buff );
  else
    effect.player->sim->error( "Warning: Invalid type '{}' for Party Favors, ignoring.", opt_str );
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

void battlefield_presence( special_effect_t& effect )
{
  auto forced_enemies = effect.player->sim->shadowlands_opts.battlefield_presence_enemies;
  auto buff           = buff_t::find( effect.player, "battlefield_presence" );
  auto p              = effect.player;

  if ( !buff )
  {
    buff = make_buff( effect.player, "battlefield_presence", effect.player->find_spell( 352858 ) )
               ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
               ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
               ->set_period( 0_ms );
  }

  // If the option is not set, adjust stacks every time the target_non_sleeping_list changes
  if ( forced_enemies == -1 )
  {
    effect.activation_cb = [ p, buff ]() {
      p->sim->target_non_sleeping_list.register_callback( [ p, buff ]( player_t* ) {
        auto enemies       = as<int>( p->sim->target_non_sleeping_list.size() );
        auto current_stack = buff->current_stack;
        auto max_stack     = buff->max_stack();

        if ( enemies != current_stack )
        {
          // Short circuit if there are more enemies than 3 and we are already at max stacks
          if ( enemies > max_stack && current_stack == max_stack )
            return;

          auto diff = as<int>( enemies ) - current_stack;
          if ( diff > 0 )
            buff->trigger( diff );
          else if ( diff < 0 )
            buff->decrement( -diff );
        }
      } );
    };
  }
  else
  {
    if ( forced_enemies != 0 )
      buff->set_initial_stack( forced_enemies );
  }

  if ( buff && forced_enemies != 0 )
  {
    effect.player->register_combat_begin( buff );
    effect.player->buffs.battlefield_presence = buff;
  }
}

void let_go_of_the_past( special_effect_t& effect )
{
  struct let_go_of_the_past_cb_t : public dbc_proc_callback_t
  {
    unsigned prev_id;

    let_go_of_the_past_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), prev_id( 0 )
    {
    }

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

  effect.proc_flags_  = PF_ALL_DAMAGE;
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
    combat_meditation_buff_t( player_t* p, double duration_mod, double duration_mod_ext, bool icd_enabled )
      : stat_buff_t( p, "combat_meditation", p->find_spell( 328908 ) )
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
    double duration_mod     = class_value_from_desc_vars( effect, "mod" );
    double duration_mod_ext = class_value_from_desc_vars( effect, "modb" );
    bool icd_enabled        = extra_desc_text_for_class( effect, effect.driver()->name_cstr() );

    effect.player->sim->print_debug(
        "class-specific properties for combat_meditation: duration_mod={}, duration_mod_ext={}, icd_enabled={}",
        duration_mod, duration_mod_ext, icd_enabled );
    buff = make_buff<combat_meditation_buff_t>( effect.player, duration_mod, duration_mod_ext, icd_enabled );
  }
  add_covenant_cast_callback<covenant_cb_buff_t>( effect.player, buff );
}

void better_together( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "better_together" );
  if ( !buff )
    buff = make_buff<stat_buff_t>( effect.player, "better_together", effect.player->find_spell( 352498 ) )
               ->set_duration( 0_ms );

  effect.player->register_combat_begin( [ buff ]( player_t* p ) {
    if ( p->sim->shadowlands_opts.better_together_ally )
      buff->trigger();
  } );
}

void pointed_courage( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "pointed_courage" );
  if ( !buff )
  {
    buff = make_buff( effect.player, "pointed_courage", effect.player->find_spell( 330511 ) )
               ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
               ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
               // TODO: add better handling of allies/enemies nearby mechanic which is checked every tick. tick is
               // disabled for now
               ->set_period( 0_ms );
  }

  effect.player->register_combat_begin(
      [ buff ]( player_t* p ) { buff->trigger( p->sim->shadowlands_opts.pointed_courage_nearby ); } );
}

void spear_of_the_archon( special_effect_t& effect )
{
  struct spear_of_the_archon_cb_t : public dbc_proc_callback_t
  {
    double hp_pct;

    spear_of_the_archon_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), hp_pct( e.driver()->effectN( 1 ).base_value() )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( s->target->health_percentage() > hp_pct && s->target != a->player )
      {
        dbc_proc_callback_t::trigger( a, s );
      }
    }
  };

  // TODO: Confirm flags
  effect.proc_flags_  = PF_ALL_DAMAGE | PF_PERIODIC;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  effect.proc_chance_ = 1.0;

  effect.custom_buff = buff_t::find( effect.player, "spear_of_the_archon" );
  if ( !effect.custom_buff )
  {
    effect.custom_buff = make_buff( effect.player, "spear_of_the_archon", effect.player->find_spell( 352720 ) )
                             ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
                             ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
  }

  new spear_of_the_archon_cb_t( effect );
}

void hammer_of_genesis( special_effect_t& effect )
{
  struct hammer_of_genesis_cb_t : public dbc_proc_callback_t
  {
    std::vector<int> target_list;

    hammer_of_genesis_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), target_list()
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
      bron_anima_cannon_t( pet_t* p, const std::string& options_str )
        : spell_t( "anima_cannon", p, p->find_spell( 332525 ) )
      {
        parse_options( options_str );

        interrupt_auto_attack  = false;
        spell_power_mod.direct = 0.55;  // Not in spell data
      }
    };

    struct bron_smash_damage_t : public spell_t
    {
      bron_smash_damage_t( pet_t* p ) : spell_t( "smash", p, p->find_spell( 341165 ) )
      {
        background              = true;
        spell_power_mod.direct  = 0.25;  // Not in spell data
        attack_power_mod.direct = 0.25;  // Not in spell data
        aoe                     = -1;
        radius                  = data().effectN( 1 ).radius_max();
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
      bron_vitalizing_bolt_t( pet_t* p, const std::string& options_str )
        : heal_t( "vitalizing_bolt", p, p->find_spell( 332526 ) )
      {
        parse_options( options_str );

        interrupt_auto_attack  = false;
        spell_power_mod.direct = 0.575;  // Not in spell data
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
      bron_travel_t( pet_t* p ) : action_t( ACTION_OTHER, "travel", p )
      {
      }

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

      bool usable_moving() const override
      {
        return true;
      }
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
      if ( name == "travel" )
        return new bron_travel_t( this );
      if ( name == "auto_attack" )
        return new bron_auto_attack_t( this );
      if ( name == "vitalizing_bolt" )
        return new bron_vitalizing_bolt_t( this, options_str );
      if ( name == "anima_cannon" )
        return new bron_anima_cannon_t( this, options_str );
      if ( name == "smash" )
        return new bron_smash_t( this, options_str );

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

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( a->background || a->trigger_gcd <= 0_ms || a->data().flags( spell_attribute::SX_NO_THREAT ) )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( proc_buff->at_max_stacks() )
      {
        proc_buff->expire();

        if ( bron->is_sleeping() )
          bron->summon( bron_dur );
      }
      else
        dbc_proc_callback_t::execute( a, s );
    }
  };

  // TODO: This technically uses proc flag 34 (Cast Successful), which currently isn't supported by simc.
  effect.proc_flags_  = PF_ALL_DAMAGE | PF_ALL_HEAL;
  effect.proc_flags2_ = PF2_CAST | PF2_CAST_DAMAGE | PF2_CAST_HEAL;

  effect.custom_buff = buff_t::find( effect.player, "brons_call_to_action" );
  if ( !effect.custom_buff )
    effect.custom_buff = make_buff( effect.player, "brons_call_to_action", effect.player->find_spell( 332514 ) );

  new brons_call_to_action_cb_t( effect );
}

// TODO: Confirm behaviour for all Classes. This potentially may be triggered from any 976 labeled action dealing
// damage. Spell ID 353349 containing the ICD for this.
void effusive_anima_accelerator( special_effect_t& effect )
{
  struct effusive_anima_accelerator_t : public unique_gear::proc_spell_t
  {
  private:
    std::vector<cooldown_t*> affected_cooldowns;
    double cdr;

  public:
    effusive_anima_accelerator_t( const special_effect_t& e )
      : unique_gear::proc_spell_t( "effusive_anima_accelerator", e.player, e.player->find_spell( 353248 ) ),
        cdr( class_value_from_desc_vars( e, "cd" ) )
    {
      spell_power_mod.tick =
          e.player->find_spell( 353248 )->effectN( 2 ).base_value() * class_value_from_desc_vars( e, "mod" ) / 100;
    }

    void init_finished() override
    {
      super::init_finished();

      affected_cooldowns.clear();
      for ( auto a : player->action_list )
      {
        auto rank_str = player->dbc->spell_text( a->data().id() ).rank();
        if ( rank_str && a->data().affected_by_label( 976 ) && util::str_compare_ci( rank_str, "Kyrian" ) )
        {
          if ( !range::contains( affected_cooldowns, a->cooldown ) )
          {
            affected_cooldowns.push_back( a->cooldown );
          }
        }
      }
    }

    double composite_spell_power() const override
    {
      return std::max( super::composite_spell_power(), super::composite_attack_power() );
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      return super::composite_persistent_multiplier( s ) / s->n_targets;
    }

    void execute() override
    {
      super::execute();
      int targets_hit = std::min( 5U, execute_state->n_targets );
      if ( targets_hit > 0 )
      {
        for ( auto c : affected_cooldowns )
        {
          c->adjust( timespan_t::from_seconds( -cdr * targets_hit ) );
          sim->print_debug( "{} cooldown reduced by {} and set to {}", c->name_str, cdr * targets_hit, c->remains() );
        }
      }
    }
  };

  add_covenant_cast_callback<covenant_cb_action_t>( effect.player, new effusive_anima_accelerator_t( effect ) );
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
      if ( util::str_compare_ci( type_str, "mastery" ) )
        race_type = RACE_HUMANOID;
      else if ( util::str_compare_ci( type_str, "primary" ) )
        race_type = RACE_BEAST;
      else if ( util::str_compare_ci( type_str, "crit" ) )
        race_type = RACE_DRAGONKIN;
      else if ( util::str_compare_ci( type_str, "magic" ) )
        race_type = RACE_ELEMENTAL;
      else if ( util::str_compare_ci( type_str, "physical" ) )
        race_type = RACE_GIANT;
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
                     ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE );
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
                     ->set_schools_from_effect( 1 )
                     ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
        }
        effect.player->buffs.volatile_solvent_damage = buff;
        break;

      case RACE_GIANT:
        buff = buff_t::find( effect.player, "volatile_solvent_giant" );
        if ( !buff )
        {
          buff = make_buff( effect.player, "volatile_solvent_giant", effect.player->find_spell( 323506 ) )
                     ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_DONE )
                     ->set_schools_from_effect( 2 )
                     ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
        }
        effect.player->buffs.volatile_solvent_damage = buff;
        break;

      default:
        buff = nullptr;
        break;
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

    plagueys_preemptive_strike_cb_t( const special_effect_t& e ) : dbc_proc_callback_t( e.player, e ), target_list()
    {
    }

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
    auto s_data     = effect.player->find_spell( 342181 );
    double duration = effect.driver()->effectN( 3 ).base_value();

    // The duration modifier for each class comes from the description variables of Lead by Example (id=342156)
    duration *= class_value_from_desc_vars( effect, "mod" );

    int allies_nearby = effect.player->sim->shadowlands_opts.lead_by_example_nearby;
    // If the user doesn't specify a number of allies affected by LbA, use default values based on position and fight
    // style
    if ( allies_nearby < 0 )
    {
      switch ( effect.player->position() )
      {
        // Assume that players right in front or at the back of the boss have enough allies nearby to get full effect
        case POSITION_BACK:
        case POSITION_FRONT:
          // For DungeonSlice, always assume two allies
          if ( util::str_compare_ci( effect.player->sim->fight_style, "DungeonSlice" ) )
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

  if ( count == 0 )
  {
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

void serrated_spaulders( special_effect_t& )
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
    {
    }

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

    counter_buff->set_stack_change_callback( [ buff ]( buff_t* b, int, int ) {
      if ( b->at_max_stacks() )
      {
        buff->trigger();
        b->expire();
      }
    } );
  }

  effect.proc_flags2_ = PF2_CRIT;
  effect.custom_buff  = counter_buff;

  new marrowed_gemstone_cb_t( effect, buff );
}

void carvers_eye( special_effect_t& effect )
{
  struct carvers_eye_cb_t : public dbc_proc_callback_t
  {
    double hp_pct;

    // Effect 1 and 3 both have the same value, but the tooltip uses effect 3
    carvers_eye_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), hp_pct( e.driver()->effectN( 3 ).base_value() )
    {
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      // Can only proc on a target you haven't procced on for 10s
      auto td = a->player->get_target_data( s->target );
      if ( s->target->health_percentage() > hp_pct && s->target != a->player &&
           !td->debuff.carvers_eye_debuff->check() )
      {
        dbc_proc_callback_t::trigger( a, s );
        td->debuff.carvers_eye_debuff->trigger();
      }
    }
  };

  // TODO: Confirm flags (including pet damage)
  effect.proc_flags_  = PF_ALL_DAMAGE | PF_PERIODIC;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  effect.proc_chance_ = 1.0;

  effect.custom_buff = buff_t::find( effect.player, "carvers_eye" );
  if ( !effect.custom_buff )
  {
    auto val = effect.player->find_spell( 351414 )->effectN( 1 ).average( effect.player );

    effect.custom_buff = make_buff<stat_buff_t>( effect.player, "carvers_eye", effect.player->find_spell( 351414 ) )
                             ->add_stat( STAT_MASTERY_RATING, val );
  }

  new carvers_eye_cb_t( effect );
}

struct mnemonic_residual_action_t : public residual_action::residual_periodic_action_t<spell_t>
{
  mnemonic_residual_action_t( const special_effect_t& effect )
    : residual_action::residual_periodic_action_t<spell_t>( "mnemonic_equipment", effect.player,
                                                            effect.player->find_spell( 351687 ) )
  {
  }
};

void mnemonic_equipment( special_effect_t& effect )
{
  struct mnemonic_equipment_cb_t : public dbc_proc_callback_t
  {
    double hp_pct;
    double dmg_repeat_pct;
    mnemonic_residual_action_t* mnemonic_residual_action;

    mnemonic_equipment_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ),
        hp_pct( e.driver()->effectN( 1 ).base_value() ),
        dmg_repeat_pct( e.driver()->effectN( 2 ).percent() )
    {
      mnemonic_residual_action = new mnemonic_residual_action_t( e );
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( s->target->health_percentage() < hp_pct && s->target != a->player )
      {
        dbc_proc_callback_t::trigger( a, s );
      }
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( !a->harmful )
        return;

      if ( s->target->health_percentage() < hp_pct )
      {
        dbc_proc_callback_t::execute( a, s );
        auto amount = s->result_amount * dmg_repeat_pct;

        // In game this has been bugged, instead of rolling the damage over it overrides it
        // Simulating this bug by simply expiring the dot before we trigger a new one
        if ( a->player->bugs )
        {
          mnemonic_residual_action->get_dot( s->target )->cancel();
        }
        residual_action::trigger( mnemonic_residual_action, s->target, amount );
      }
    }
  };

  // TODO: Confirm flags/check if pet damage procs this
  effect.proc_flags_  = PF_ALL_DAMAGE | PF_PERIODIC;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  effect.proc_chance_ = 1.0;

  new mnemonic_equipment_cb_t( effect );
}

// Passive which increases Stamina based on Renown level
void deepening_bond( special_effect_t& effect )
{
  const auto spell = effect.driver();

  if ( effect.player->sim->debug )
  {
    effect.player->sim->out_debug.print( "{} increasing stamina by {}% ({})", effect.player->name(),
                                         spell->effectN( 1 ).base_value(), spell->name_cstr() );
  }

  effect.player->base.attribute_multiplier[ ATTR_STAMINA ] *= 1.0 + spell->effectN( 1 ).percent();
}

// Helper function for registering an effect, with autoamtic skipping initialization if soulbind spell is not available
void register_soulbind_special_effect( unsigned spell_id, const custom_cb_t& init_callback, bool fallback = false )
{
  unique_gear::register_special_effect(
      spell_id,
      [ &, init_callback ]( special_effect_t& effect ) {
        if ( effect.source != SPECIAL_EFFECT_SOURCE_FALLBACK &&
             !effect.player->find_soulbind_spell( effect.driver()->name_cstr() )->ok() )
          return;
        init_callback( effect );
      },
      fallback );
}

}  // namespace

void register_special_effects()
{
  // Night Fae
  register_soulbind_special_effect( 320659, soulbinds::niyas_tools_burrs );  // Niya
  register_soulbind_special_effect( 320660, soulbinds::niyas_tools_poison );
  register_soulbind_special_effect( 320662, soulbinds::niyas_tools_herbs );
  register_soulbind_special_effect( 322721, soulbinds::grove_invigoration, true );
  register_soulbind_special_effect( 352503, soulbinds::bonded_hearts );
  register_soulbind_special_effect( 319191, soulbinds::field_of_blossoms, true );  // Dreamweaver
  register_soulbind_special_effect( 319210, soulbinds::social_butterfly );
  register_soulbind_special_effect( 352786, soulbinds::dream_delver );
  register_soulbind_special_effect( 325069, soulbinds::first_strike, true );  // Korayn
  register_soulbind_special_effect( 325066, soulbinds::wild_hunt_tactics );
  // Venthyr
  // register_soulbind_special_effect( 331580, soulbinds::exacting_preparation );  // Nadjia
  register_soulbind_special_effect( 331584, soulbinds::dauntless_duelist );
  register_soulbind_special_effect( 331586, soulbinds::thrill_seeker, true );
  register_soulbind_special_effect( 352373, soulbinds::fatal_flaw );
  register_soulbind_special_effect( 336239, soulbinds::soothing_shade );  // Theotar
  register_soulbind_special_effect( 319983, soulbinds::wasteland_propriety );
  register_soulbind_special_effect( 351750, soulbinds::party_favors );
  register_soulbind_special_effect( 319973, soulbinds::built_for_war );  // Draven
  register_soulbind_special_effect( 332753, soulbinds::superior_tactics );
  register_soulbind_special_effect( 352417, soulbinds::battlefield_presence );
  // Kyrian
  register_soulbind_special_effect( 328257, soulbinds::let_go_of_the_past );  // Pelagos
  register_soulbind_special_effect( 328266, soulbinds::combat_meditation );
  register_soulbind_special_effect( 351146, soulbinds::better_together );
  register_soulbind_special_effect( 329778, soulbinds::pointed_courage );  // Kleia
  register_soulbind_special_effect( 351488, soulbinds::spear_of_the_archon );
  register_soulbind_special_effect( 333935, soulbinds::hammer_of_genesis );  // Mikanikos
  register_soulbind_special_effect( 333950, soulbinds::brons_call_to_action, true );
  register_soulbind_special_effect( 352188, soulbinds::effusive_anima_accelerator );
  // Necrolord
  register_soulbind_special_effect( 323074, soulbinds::volatile_solvent );  // Marileth
  register_soulbind_special_effect( 323090, soulbinds::plagueys_preemptive_strike );
  register_soulbind_special_effect( 323919, soulbinds::gnashing_chompers );  // Emeni
  register_soulbind_special_effect( 342156, soulbinds::lead_by_example, true );
  register_soulbind_special_effect( 326514, soulbinds::forgeborne_reveries );  // Heirmir
  register_soulbind_special_effect( 326504, soulbinds::serrated_spaulders );
  register_soulbind_special_effect( 326572, soulbinds::heirmirs_arsenal_marrowed_gemstone, true );
  register_soulbind_special_effect( 350899, soulbinds::carvers_eye );
  register_soulbind_special_effect( 350936, soulbinds::mnemonic_equipment );
  // Covenant Renown Stamina Passives
  unique_gear::register_special_effect( 344052, soulbinds::deepening_bond );  // Night Fae Rank 1
  unique_gear::register_special_effect( 344053, soulbinds::deepening_bond );  // Night Fae Rank 2
  unique_gear::register_special_effect( 344057, soulbinds::deepening_bond );  // Night Fae Rank 3
  unique_gear::register_special_effect( 344068, soulbinds::deepening_bond );  // Venthyr Rank 1
  unique_gear::register_special_effect( 344069, soulbinds::deepening_bond );  // Venthyr Rank 2
  unique_gear::register_special_effect( 344070, soulbinds::deepening_bond );  // Venthyr Rank 3
  unique_gear::register_special_effect( 344076, soulbinds::deepening_bond );  // Necrolord Rank 1
  unique_gear::register_special_effect( 344077, soulbinds::deepening_bond );  // Necrolord Rank 2
  unique_gear::register_special_effect( 344078, soulbinds::deepening_bond );  // Necrolord Rank 3
  unique_gear::register_special_effect( 344087, soulbinds::deepening_bond );  // Kyrian Rank 1
  unique_gear::register_special_effect( 344089, soulbinds::deepening_bond );  // Kyrian Rank 2
  unique_gear::register_special_effect( 344091, soulbinds::deepening_bond );  // Kyrian Rank 3
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

  for ( auto renown_spell : player->covenant->renown_spells() )
  {
    auto spell = player->find_spell( renown_spell );

    if ( !spell->ok() )
      continue;

    special_effect_t effect{ player };
    effect.type   = SPECIAL_EFFECT_EQUIP;
    effect.source = SPECIAL_EFFECT_SOURCE_SOULBIND;

    unique_gear::initialize_special_effect( effect, renown_spell );

    // Ensure the renown spell has a custom special effect to protect against errant auto-inference
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

  // Dream Delver
  sim->register_target_data_initializer( []( actor_target_data_t* td ) {
    if ( td->source->find_soulbind_spell( "Dream Delver" )->ok() )
    {
      assert( !td->debuff.dream_delver );

      td->debuff.dream_delver = make_buff( *td, "dream_delver", td->source->find_spell( 353354 ) )
                                    ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );
      td->debuff.dream_delver->reset();
    }
    else
      td->debuff.dream_delver = make_buff( *td, "dream_delver" )->set_quiet( true );
  } );

  // Carver's Eye dummy buff to track cooldown
  sim->register_target_data_initializer( []( actor_target_data_t* td ) {
    auto carvers_eye = td->source->find_soulbind_spell( "Carver's Eye" );
    if ( carvers_eye->ok() )
    {
      assert( !td->debuff.carvers_eye_debuff );

      td->debuff.carvers_eye_debuff =
          make_buff( *td, "carvers_eye_debuff", td->source->find_spell( 350899 ) )
              ->set_quiet( true )
              ->set_duration( timespan_t::from_seconds( carvers_eye->effectN( 2 ).base_value() ) );
      td->debuff.carvers_eye_debuff->reset();
    }
    else
      td->debuff.carvers_eye_debuff = make_buff( *td, "carvers_eye_debuff" )->set_quiet( true );
  } );
}

}  // namespace soulbinds
}  // namespace covenant
