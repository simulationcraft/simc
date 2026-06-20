#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

#include "sc_warlock_pets.hpp"
#include "util/util.hpp"
#include "class_modules/apl/warlock.hpp"

#include <queue>

namespace warlock
{

warlock_td_t::warlock_td_t( player_t* target, warlock_t& p )
  : actor_target_data_t( target, &p ), soc_threshold( 0.0 ), warlock( p )
{
  // Shared
  dots.drain_life = target->get_dot( "drain_life", &p );

  // Affliction
  dots.corruption = target->get_dot( "corruption", &p );
  dots.agony = target->get_dot( "agony", &p );
  dots.drain_soul = target->get_dot( "drain_soul", &p );
  dots.seed_of_corruption = target->get_dot( "seed_of_corruption", &p );
  dots.unstable_affliction = target->get_dot( "unstable_affliction", &p );
  dots.malefic_grasp = target->get_dot( "malefic_grasp", &p );

  debuffs.haunt = make_buff( *this, "haunt", p.talents.haunt )
                      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                      ->set_default_value_from_effect( 2 )
                      ->set_cooldown( 0_ms )
                      ->set_stack_change_callback( [ &p ]( buff_t* b, int, int cur ) {
                        p.haunt_target = ( cur == 0 ) ? nullptr : b->player;
                      } );

  if ( p.talents.shadow_of_nathreza_1.ok() )
  {
    debuffs.haunt->set_tick_zero( false )
        ->set_period( p.talents.haunt->effectN( 5 ).period() )
        ->set_tick_callback( [ target, &p ]( buff_t*, int, timespan_t ) {
          p.proc_actions.shadow_of_nathreza->execute_on_target( target );
          if ( p.talents.shadow_of_nathreza_3.ok() )
            helpers::trigger_wrath_of_nathreza( &p, target );
        } );
  }

  // Demonology
  debuffs.doom = make_buff( *this, "doom", p.talents.doom_debuff )
                     ->set_stack_change_callback( [ &p ]( buff_t* b, int, int cur ) {
                       if ( cur == 0 )
                       {
                         p.proc_actions.doom_proc->execute_on_target( b->player );
                       }
                     } );

  // Destruction
  dots.immolate = target->get_dot( "immolate", &p );

  debuffs.lake_of_fire = make_buff( *this, "lake_of_fire", p.talents.lake_of_fire_debuff )
                             ->set_default_value_from_effect( 1 )
                             ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                             ->set_max_stack( 1 )
                             ->set_proc_callbacks( false );

  debuffs.shadowburn = make_buff( *this, "shadowburn", p.talents.shadowburn )
                           ->set_default_value( p.talents.shadowburn_2->effectN( 1 ).base_value() / 10 );

  // Use havoc_debuff where we need the data but don't have the active talent
  // Mayhem proc chance follows a Flat % RNG model, but has ICD
  debuffs.havoc = make_buff( *this, "havoc", p.talents.havoc_debuff )
                      ->set_duration( p.talents.mayhem.ok() ? p.talents.mayhem->effectN( 3 ).time_value() + p.talents.improved_havoc->effectN( 1 ).time_value() : p.talents.havoc->duration() )
                      ->set_cooldown( p.talents.mayhem.ok() ? p.talents.mayhem->internal_cooldown() : 0_ms )
                      ->set_chance( p.talents.mayhem.ok() ? p.talents.mayhem->effectN( 1 ).percent() : p.talents.havoc->proc_chance() )
                      ->set_stack_change_callback( [ &p ]( buff_t* b, int, int cur ) {
                        if ( cur == 0 )
                        {
                          p.havoc_target = nullptr;
                        }
                        else
                        {
                          if ( p.havoc_target && p.havoc_target != b->player )
                            p.get_target_data( p.havoc_target )->debuffs.havoc->expire();
                          p.havoc_target = b->player;
                        }

                        range::for_each( p.havoc_spells, []( action_t* a ) { a->target_cache.is_valid = false; } );
                      } );

  // Diabolist
  debuffs.cloven_soul = make_buff( *this, "cloven_soul", p.hero.cloven_soul_debuff );

  // Hellcaller
  dots.wither = target->get_dot( "wither", &p );

  debuffs.blackened_soul = make_buff( *this, "blackened_soul", p.hero.blackened_soul_trigger )
                               ->set_duration( 0_ms )
                               ->set_tick_zero( false )
                               ->set_period( p.hero.blackened_soul_trigger->effectN( 1 ).period() )
                               ->set_tick_callback( [ &p, target ]( buff_t* blackened_soul_debuff, int, timespan_t ) {
                                  auto tdata = p.get_target_data( target );
                                  if ( tdata->dots.wither->is_ticking() )
                                  {
                                    p.proc_actions.blackened_soul->execute_on_target( target );
                                  }
                                  else
                                  {
                                    // blackened_soul is a 0-duration frozen-stack debuff, so expiring it from its
                                    // tick callback is safe; tick_t will not apply post-callback stack changes.
                                    assert( blackened_soul_debuff->freeze_stacks );
                                    assert( blackened_soul_debuff->buff_duration() == 0_ms );
                                    assert( blackened_soul_debuff->expiration.empty() );
                                    assert( blackened_soul_debuff->tick_event == nullptr );
                                    blackened_soul_debuff->expire();
                                  }
                               } )
                               ->set_tick_behavior( buff_tick_behavior::REFRESH )
                               ->set_freeze_stacks( true )
                               ->set_max_stack( 99 );

  debuffs.wither = make_buff( *this, "wither", p.hero.wither_dot )
                       ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                       ->set_proc_callbacks( false ); // Dummy debuff

  // Soul Harvester
  dots.soul_anathema = target->get_dot( "soul_anathema", &p );

  dots.shared_fate = target->get_dot( "shared_fate", &p );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void warlock_td_t::target_demise()
{
  if ( !( target->is_enemy() ) )
    return;

  // Warlock gains only one shard at most per enemy that dies with UA, regardless of UA stacks
  if ( dots.unstable_affliction->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Unstable Affliction.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, warlock.talents.unstable_affliction_2->effectN( 1 ).base_value(), warlock.gains.unstable_affliction_refund );
  }

  if ( dots.drain_soul->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Drain Soul.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1.0, warlock.gains.drain_soul );
  }

  if ( debuffs.haunt->check() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} reset Haunt's cooldown.", target->name(), warlock.name() );

    warlock.cooldowns.haunt->reset( true );
  }

  if ( debuffs.shadowburn->check() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains 1 shard from Shadowburn.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, debuffs.shadowburn->check_value(), warlock.gains.shadowburn_refund );
  }

  if ( warlock.hero.demonic_soul.ok() && warlock.hero.shared_fate.ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Shared Fate on all targets in range.", target->name(), warlock.name() );

    warlock.proc_actions.shared_fate->execute();
  }

  if ( warlock.hero.demonic_soul.ok() && warlock.hero.feast_of_souls.ok() )
  {
    if ( warlock.prd_rng.feast_of_souls->trigger() )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} triggers Feast of Souls.", target->name(), warlock.name() );

      warlock.feast_of_souls_gain();
    }
  }
}

int warlock_td_t::count_affliction_dots() const
{
  // NOTE: [Drain Soul, Malefic Grasp, Shared Fate, Soul Anathema] DoTs do not count (they do not affect effects influenced by this count)
  int count = 0;

  if ( dots.agony->is_ticking() )
    count++;

  if ( dots.corruption->is_ticking() )
    count++;

  // NOTE: 2026-02-17: Currently Wither is bugged and does not count
  if ( !warlock.bugs && dots.wither->is_ticking() )
    count++;

  if ( dots.seed_of_corruption->is_ticking() )
    count++;

  // NOTE: UA counts as 1 dot does not matter how many stacks
  if ( dots.unstable_affliction->is_ticking() )
    count++;

  return count;
}

warlock_t::warlock_t( sim_t* sim, util::string_view name, race_e r )
  : parse_player_effects_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    havoc_spells(),
    diabolic_ritual( 0 ),
    demonic_art_buff_replaced( false ),
    wild_imp_ic_shared_offset(),
    n_active_pets( 0 ),
    warlock_pet_list( this ),
    talents(),
    hero(),
    proc_actions(),
    tier(),
    cooldowns(),
    buffs(),
    gains(),
    procs(),
    rng_settings(),
    initial_soul_shards(),
    default_pet(),
    disable_auto_felstorm( false ),
    normalize_destruction_mastery( false ),
    eye_explosion_instanced_bug_cb( false ),
    eye_explosion_instanced_bug_sb( false ),
    eye_explosion_instanced_bug_rof( true ),
    tyrant_antoran_armaments_target_mul( 1.0 )
{
  cooldowns.haunt = get_cooldown( "haunt" );
  cooldowns.dark_harvest = get_cooldown( "dark_harvest" );
  cooldowns.soul_fire = get_cooldown( "soul_fire" );
  cooldowns.summon_doomguard = get_cooldown( "summon_doomguard" );
  cooldowns.felstorm_icd = get_cooldown( "felstorm_icd" );
  cooldowns.echo_of_sargeras = get_cooldown( "echo_of_sargeras_icd" );
  cooldowns.blackened_soul = get_cooldown( "blackened_soul_icd" );

  resource_regeneration = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ] = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;

  sim->register_heartbeat_event_callback( [ this ]( sim_t* ) {
    // NOTE (2026-04-24): Some pets are currently bugged when updating Hellbent Commander stacks on arise/demise.
    // Hellbent Commander's stacks are refreshed on each heartbeat update, but not all pets are correctly accounted for.
    if ( bugs && talents.hellbent_commander.ok() )
    {
      int expected_stacks = 0;

      for ( auto pet : pet_list )
      {
        auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

        if ( lock_pet == nullptr )
          continue;
        if ( lock_pet->is_sleeping() )
          continue;

        if ( lock_pet->triggers.hellbent_commander_heartbeat )
          expected_stacks++;
      }

      const int current_stacks = buffs.hellbent_commander->check();

      const int stack_diff = expected_stacks - current_stacks;
      if ( stack_diff > 0 )
        buffs.hellbent_commander->trigger( stack_diff );
      else if ( stack_diff < 0 )
        buffs.hellbent_commander->decrement( std::abs( stack_diff ) );

      if ( stack_diff != 0 )
      {
        this->sim->print_debug( "{} heartbeat event update {} buff stack number from stacks_before={} to stacks_after={}",
                        this->name(), buffs.hellbent_commander->name(), current_stacks, expected_stacks );
      }
      assert( ( buffs.hellbent_commander->check() == expected_stacks ) && "Incorrent Demon Count for Hellbent Commander" );
    }

    for ( auto pet : active_pets )
    {
      auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

      if ( lock_pet == nullptr )
        continue;
      if ( lock_pet->is_sleeping() )
        continue;

      lock_pet->heartbeat_update_event();
    }
  } );
}

const spell_data_t* warlock_t::conditional_spell_lookup( bool fn, int id )
{
  if ( !fn )
    return spell_data_t::not_found();

  return find_spell( id );
}

bool warlock_t::affliction() const
{ return specialization() == WARLOCK_AFFLICTION; }

bool warlock_t::demonology() const
{ return specialization() == WARLOCK_DEMONOLOGY; }

bool warlock_t::destruction() const
{ return specialization() == WARLOCK_DESTRUCTION; }

bool warlock_t::diabolist() const
{ return has_hero_tree( HERO_DIABOLIST ); }

bool warlock_t::hellcaller() const
{ return has_hero_tree( HERO_HELLCALLER ); }

bool warlock_t::soul_harvester() const
{ return has_hero_tree( HERO_SOUL_HARVESTER ); }

static void accumulate_seed_of_corruption( warlock_td_t* td, double amount )
{
  td->soc_threshold -= amount;

  if ( td->soc_threshold <= 0 )
    td->dots.seed_of_corruption->cancel();
  else if ( td->source->sim->log )
    td->source->sim->print_log( "Remaining damage to explode Seed of Corruption on {} is {}.", td->target->name_str, td->soc_threshold );
}

void warlock_t::init_assessors()
{
  player_t::init_assessors();

  // Assessor responsible for handling the accumulated damage in SoC for the explosion
  auto assessor_soc_fn = [ this ]( result_amount_type, action_state_t* s ) {
    if ( get_target_data( s->target )->dots.seed_of_corruption->is_ticking() )
      accumulate_seed_of_corruption( get_target_data( s->target ), s->result_total );

    return assessor::CONTINUE;
  };

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_soc_fn );

  if ( hero.shared_fate.ok() || hero.feast_of_souls.ok() )
  {
    assert( hero.marked_soul->ok() );
    // Assessor used with Soul Harvester to handle proc triggers (trinkets, enchants, ...) from damage-over-time effects
    auto assessor_sh_fn = [ this ]( result_amount_type amount_type, action_state_t* s ) {
      // Soul Harvester seems to have some hidden trigger tied to damage-over-time effects
      // We assume this trigger is Marked Soul and that it is only active when Shared Fate or Feast of Souls is selected
      if ( amount_type == result_amount_type::DMG_OVER_TIME )
        trigger_aura_applied_callbacks( proc_data_entries.marked_soul, s->target );

      return assessor::CONTINUE;
    };

    assessor_out_damage.add ( assessor::TARGET_DAMAGE + 1, assessor_sh_fn );
  }
}

void warlock_t::init_finished()
{
  parse_player_effects();

  player_t::init_finished();

  // 2026-04-06: The Infernal Command (IC) buff is applied/faded periodically every ~5.25 seconds, with some variance.
  // The timing of IC buff events starts independently for each imp when it spawns, rather than following a single global
  // heartbeat window. However, nearby applications/fades do appear to cluster within small time windows.
  // In-game testing suggests this can be modeled fairly closely using a global periodic window (~0.42s) and some variance.
  // The current value of this buff is 0, so it does not provide any damage increase.
  // It is still relevant, however, because applying the buff can trigger trinkets and other proc effects.
  if ( demonology() )
  {
    register_combat_begin( [ this ]( player_t* ) {
      timespan_t initial_delay = rng().range( 0_ms, 420_ms );
      make_event( sim, initial_delay, [ this ]() {
        make_repeating_event( sim, 420_ms, [ this ]() {
          auto active_pet = warlock_pet_list.active;
          if ( active_pet && active_pet->pet_type == PET_FELGUARD )
          {
            wild_imp_ic_shared_offset = timespan_t::from_millis( rng().range( -267, 267 ) );
            auto imps = warlock_pet_list.wild_imps.active_pets();
            for ( auto imp : imps )
            {
              if ( sim->current_time() >= ( imp->infernal_command_ev_ts + imp->infernal_command_ev_offset ) )
              {
                if ( imp->buffs.infernal_command->check() )
                  imp->buffs.infernal_command->expire();
                else
                  imp->buffs.infernal_command->trigger();

                imp->infernal_command_ev_ts += 5250_ms;
                imp->infernal_command_ev_offset = wild_imp_ic_shared_offset;
              }
            }
          }
        } );
      } );
    } );
  }
}

void warlock_t::invalidate_cache( cache_e c )
{
  parse_player_effects_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( demonology() )
      {
        player_t::invalidate_cache( CACHE_PET_DAMAGE_MULTIPLIER );
        player_t::invalidate_cache( CACHE_GUARDIAN_DAMAGE_MULTIPLIER );
      }
      break;
    default:
      break;
  }
}

double warlock_t::composite_mastery() const
{
  double m = parse_player_effects_t::composite_mastery();

  if ( hero.shared_vessel.ok() )
  {
    if ( buffs.manifested_demonic_soul->check() )
      m += hero.shared_vessel->effectN( 1 ).base_value();
  }

  return m;
}

// Used to determine how many Wild Imps are waiting to be spawned from Hand of Guldan
int warlock_t::get_spawning_imp_count()
{ return as<int>( wild_imp_spawns.size() ); }

// Function for returning the time until a certain number of imps will have spawned
// In the case where count is equal to or greater than number of incoming imps, time to last imp is returned
// Otherwise, time to the Nth imp spawn will be returned
// All other cases will return 0. A negative (or zero) count value will behave as "all"
timespan_t warlock_t::time_to_imps( int count )
{
  timespan_t max = 0_ms;
  if ( count >= as<int>( wild_imp_spawns.size() ) || count <= 0 )
  {
    for ( auto ev : wild_imp_spawns )
    {
      timespan_t ex = debug_cast<helpers::imp_delay_event_t*>( ev )->expected_time();
      if ( ex > max )
        max = ex;
    }
    return max;
  }
  else
  {
    std::priority_queue<timespan_t> shortest;
    for ( auto ev : wild_imp_spawns )
    {
      timespan_t ex = debug_cast<helpers::imp_delay_event_t*>( ev )->expected_time();
      if ( as<int>( shortest.size() ) >= count && ex < shortest.top() )
      {
        shortest.pop();
        shortest.push( ex );
      }
      else if ( as<int>( shortest.size() ) < count )
      {
        shortest.push( ex );
      }
    }

    if ( !shortest.empty() )
    {
      return shortest.top();
    }
    else
    {
      return 0_ms;
    }
  }
}

int warlock_t::active_demon_count( bool include_diabolist ) const
{
  int count = 0;

  for ( auto pet : this->pet_list )
  {
    auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

    if ( lock_pet == nullptr )
      continue;
    if ( lock_pet->is_sleeping() )
      continue;

    // NOTE: 2026-02-17 Dibolist guardians seems to not count for some effects/talents (Sacrificed Souls)
    if ( !include_diabolist && lock_pet->is_diabolist_guardian )
      continue;

    count++;
  }

  return count;
}

std::pair<timespan_t, timespan_t> warlock_t::dreadstalkers_delay_duration_adjustment_helper( const player_t& target )
{
  std::pair<timespan_t, timespan_t> ret;
  timespan_t& delay = ret.first;
  timespan_t& dur_adjust = ret.second;
  const double dist = get_player_distance( target );
  // The summon is considered at melee range if the distance is less than or equal to 5 yards
  if ( dist > 5.0 )
  {
    // Set a randomized offset on first melee attacks after travel time. Make sure it's the same value for each dog so they're synced
    delay = rng().range( 0_s, 1_s );
    // Adjust the extra duration, that appear to be offset from the melee attack check in a correlated manner
    // Despawn events appear to be offset from the melee attack check in a correlated manner
    // Starting with this which mimics despawns on the "off-beats" compared to the 1s heartbeat for the melee attack, and taking into account the distance
    // Last tested 2025-04-06
    // The maximum despawn extra duration is 820ms. The initial offset point is 260ms, increasing 24ms with each yard
    // There is some variance in the delay-duration_adj relation that can be modeled fairly closely using a normal distribution
    dur_adjust = ( delay + timespan_t::from_millis( rng().gauss( 260.0, 25.0 ) + 24.0 * dist ) ) % 820_ms;
  }
  else
  {
    // There is no delay on the first melee attack when summoned from melee
    delay = 0_ms;
    // In this case the extra duration of the dreadstalkers can be assumed random between the minumum (0ms) and the maximum (820ms) (last tested 2025-04-06)
    dur_adjust = timespan_t::from_millis( rng().range( 0.0, 820.0 ) );
  }
  return ret;
}

static std::string append_rng_option( warlock_t::rng_settings_t::rng_setting_t setting )
{
  std::string str = "";

  if ( setting.setting_value != setting.default_value )
    str += "rng_" + setting.option_name + util::to_string( setting.setting_value ) + "\n";

  return str;
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype & SAVE_PLAYER )
  {
    if ( initial_soul_shards != 3 )
      profile_str += "warlock.soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )
      profile_str += "warlock.default_pet=" + default_pet + "\n";
    if ( disable_auto_felstorm )
      profile_str += "warlock.disable_felstorm=" + util::to_string( as<int>( disable_auto_felstorm ) ) + "\n";
    if ( normalize_destruction_mastery )
      profile_str +=
          "warlock.normalize_destruction_mastery=" + util::to_string( as<int>( normalize_destruction_mastery ) ) + "\n";
    if ( eye_explosion_instanced_bug_cb )
      profile_str +=
          "warlock.eye_explosion_instanced_bug_cb=" + util::to_string( as<int>( eye_explosion_instanced_bug_cb ) ) + "\n";
    if ( eye_explosion_instanced_bug_sb )
      profile_str +=
          "warlock.eye_explosion_instanced_bug_sb=" + util::to_string( as<int>( eye_explosion_instanced_bug_sb ) ) + "\n";
    if ( !eye_explosion_instanced_bug_rof )
      profile_str +=
          "warlock.eye_explosion_instanced_bug_rof=" + util::to_string( as<int>( eye_explosion_instanced_bug_rof ) ) + "\n";
    if ( tyrant_antoran_armaments_target_mul < 1.0 )
      profile_str +=
          "warlock.tyrant_antoran_armaments_target_mul=" + util::to_string( tyrant_antoran_armaments_target_mul ) + "\n";
    rng_settings.for_each( [ &profile_str ]( auto& setting ) { profile_str += append_rng_option( setting ); } );
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  auto* p = debug_cast<warlock_t*>( source );

  initial_soul_shards = p->initial_soul_shards;
  default_pet = p->default_pet;
  disable_auto_felstorm = p->disable_auto_felstorm;
  normalize_destruction_mastery = p->normalize_destruction_mastery;
  eye_explosion_instanced_bug_cb = p->eye_explosion_instanced_bug_cb;
  eye_explosion_instanced_bug_sb = p->eye_explosion_instanced_bug_sb;
  eye_explosion_instanced_bug_rof = p->eye_explosion_instanced_bug_rof;
  tyrant_antoran_armaments_target_mul = p->tyrant_antoran_armaments_target_mul;

  rng_settings = p->rng_settings;
}

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
      return STAT_NONE;
    case STAT_SPIRIT:
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

pet_t* warlock_t::create_main_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );
  if ( p )
    return p;
  using namespace pets;

  if ( pet_name == "felhunter" )
    return new pets::base::felhunter_pet_t( this, pet_name );
  if ( pet_name == "imp" )
    return new pets::base::imp_pet_t( this, pet_name );
  if ( pet_name == "sayaad" || pet_name == "incubus" || pet_name == "succubus" )
    return new pets::base::sayaad_pet_t( this, pet_name );
  if ( pet_name == "voidwalker" )
    return new pets::base::voidwalker_pet_t( this, pet_name );
  if ( pet_name == "felguard" && demonology() && talents.summon_felguard.ok() )
    return new pets::demonology::felguard_pet_t( this, pet_name );

  return nullptr;
}

std::unique_ptr<expr_t> warlock_t::create_pet_expression( util::string_view name_str )
{
  if ( name_str == "last_cast_imps" )
  {
    return make_fn_expr( "last_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] < 32;
      } );
    } );
  }
  else if ( name_str == "two_cast_imps" )
  {
    return make_fn_expr( "two_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] < 48;
      } );
    } );
  }
  else if ( name_str == "last_cast_igb_imps" )
  {
    return make_fn_expr( "last_cast_igb_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] < 32 && pet->buffs.imp_gang_boss->check();
      } );
    } );
  }
  else if ( name_str == "two_cast_igb_imps" )
  {
    return make_fn_expr( "two_cast_igb_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] < 48 && pet->buffs.imp_gang_boss->check();
      } );
    } );
  }
  else if ( name_str == "igb_ratio" )
  {
    return make_fn_expr( "igb_ratio", [ this ]() {
      auto igb_count = warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->buffs.imp_gang_boss->check();
        } );

      return igb_count / as<double>( buffs.wild_imps->stack() );
      } );
  }

  return player_t::create_expression( name_str );
}

std::unique_ptr<expr_t> warlock_t::create_expression( util::string_view name_str )
{
  // TODO: Remove time to shard expression?
  if ( name_str == "time_to_shard" )
  {
    return make_fn_expr( name_str, [ this]() {
      auto td               = get_target_data( target );
      dot_t* agony          = td->dots.agony;
      double active_agonies = get_active_dots( agony );
      if ( sim->debug )
        sim->out_debug.printf( "active agonies: %f", active_agonies );

      if ( active_agonies == 0 || !agony->current_action )
      {
        return std::numeric_limits<double>::infinity();
      }
      action_state_t* agony_state = agony->current_action->get_state( agony->state );
      timespan_t dot_tick_time    = agony->current_action->tick_time( agony_state );
      double creeping_death_mul   = 1.0;
      if ( talents.creeping_death.ok() )
      {
        if ( !bugs || talents.creeping_death.rank() < 2 )
          creeping_death_mul *= 1.0 + talents.creeping_death->effectN( 1 ).percent();
        else
          creeping_death_mul *= 1.0 + ( talents.creeping_death->effectN( 1 ).percent() * 0.5 );
      }

      // Seeks to return the average expected time for the player to generate a single soul shard.
      // TOCHECK regularly.
      double average = 1.0 / ( ( rng_settings.agony_energize.setting_value * 0.5 ) * std::pow( active_agonies, -2.0 / 3.0 ) * creeping_death_mul )
                       * dot_tick_time.total_seconds() / active_agonies;

      if ( sim->debug )
        sim->out_debug.printf( "time to shard return: %f", average );

      action_state_t::release( agony_state );
      return average;
    } );
  }
  else if ( name_str == "pet_count" )
  {
    return make_ref_expr( name_str, n_active_pets );
  }
  else if ( name_str == "last_cast_imps" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "two_cast_imps" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "last_cast_igb_imps" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "two_cast_igb_imps" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "igb_ratio" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "havoc_active" )
  {
    return make_fn_expr( name_str, [ this ] { return havoc_target != nullptr; } );
  }
  else if ( name_str == "havoc_remains" )
  {
    return make_fn_expr( name_str, [ this ] {
      return havoc_target ? get_target_data( havoc_target )->debuffs.havoc->remains().total_seconds() : 0.0;
    } );
  }
  else if ( name_str == "incoming_imps" )
  {
    return make_fn_expr( name_str, [ this ] { return this->get_spawning_imp_count(); } );
  }
  else if ( name_str == "can_seed" )
  {
    std::vector<action_t*> soc_list;
    for ( auto a : action_list )
    {
      if ( a->name_str == "seed_of_corruption" )
        soc_list.push_back( a );
    }

    return make_fn_expr( name_str, [this, soc_list] {
      std::vector<player_t*> no_dots;

      if ( soc_list.empty() )
        return false;

      //All the actions should have the same target list, so do this once only
      const auto& tl = soc_list[ 0 ]->target_list();

      for ( auto t : tl )
      {
        if ( !get_target_data( t )->dots.seed_of_corruption->is_ticking() )
          no_dots.push_back( t );
      }

      //If there are no targets without a seed already, this expression should be false
      if ( no_dots.empty() )
        return false;

      //If all of the remaining unseeded targets have a seed in flight, we should also return false
      for ( auto t : no_dots )
      {
        bool can_seed = true;

        for ( auto s : soc_list )
        {
          if ( s->has_travel_events_for( t ) )
          {
            can_seed = false;
            break;
          }
        }

        if ( can_seed )
          return true;
      }

      return false;
    } );
  }
  else if ( name_str == "diabolic_ritual" )
  {
    return make_fn_expr( name_str, [ this ]()
      {
        return buffs.ritual_overlord->check() || buffs.ritual_mother->check() || buffs.ritual_pit_lord->check();
      } );
  }
  else if ( name_str == "demonic_art" )
  {
    return make_fn_expr( name_str, [ this ]()
      {
        return buffs.art_overlord->check() || buffs.art_mother->check() || buffs.art_pit_lord->check();
      } );
  }

  auto splits = util::string_split<util::string_view>( name_str, "." );

  if ( splits.size() == 3 && splits[ 0 ] == "time_to_imps" && splits[ 2 ] == "remains" )
  {
    auto amt = splits[ 1 ] == "all" ? -1 : util::to_int( splits[ 1 ] );

    return make_fn_expr( name_str, [ this, amt ]() {
      return this->time_to_imps( amt );
    } );
  }
  else if ( splits.size() == 2 && splits[ 0 ] == "dot_refreshable_count" )
  {
    enum class refreshable_dot_e
    {
      INVALID = -1,
      WITHER,
      IMMOLATE,
      CORRUPTION,
      AGONY,
      UA,
      SOC
    };

    unsigned action_id = 0;
    refreshable_dot_e dot_sel = refreshable_dot_e::INVALID;
    const auto& dot_name = splits[ 1 ];
    if ( dot_name == "wither" )
    {
      if ( hero.wither.ok() )
      {
        dot_sel = refreshable_dot_e::WITHER;
        action_id = hero.wither_direct->id();
      }
    }
    else if ( dot_name == "immolate" )
    {
      if ( destruction() )
      {
        dot_sel = refreshable_dot_e::IMMOLATE;
        action_id = warlock_base.immolate_old->id();
      }
    }
    else if ( dot_name == "corruption" )
    {
      if ( affliction() )
      {
        dot_sel = refreshable_dot_e::CORRUPTION;
        action_id = warlock_base.corruption->id();
      }
    }
    else if ( dot_name == "agony" )
    {
      if ( affliction() )
      {
        dot_sel = refreshable_dot_e::AGONY;
        action_id = talents.agony->id();
      }
    }
    else if ( dot_name == "unstable_affliction" )
    {
      if ( affliction() )
      {
        dot_sel = refreshable_dot_e::UA;
        action_id = talents.unstable_affliction->id();
      }
    }
    else if ( dot_name == "seed_of_corruption" )
    {
      if ( affliction() )
      {
        dot_sel = refreshable_dot_e::SOC;
        action_id = talents.seed_of_corruption->id();
      }
    }
    else if ( dot_name == "immolate_or_wither" || dot_name == "wither_or_immolate" )
    {
      if ( hero.wither.ok() )
      {
        dot_sel = refreshable_dot_e::WITHER;
        action_id = hero.wither_direct->id();
      }
      else if ( destruction() )
      {
        dot_sel = refreshable_dot_e::IMMOLATE;
        action_id = warlock_base.immolate_old->id();
      }
    }
    else if ( dot_name == "corruption_or_wither" || dot_name == "wither_or_corruption" )
    {
      if ( hero.wither.ok() )
      {
        dot_sel = refreshable_dot_e::WITHER;
        action_id = hero.wither_direct->id();
      }
      else if ( affliction() )
      {
        dot_sel = refreshable_dot_e::CORRUPTION;
        action_id = warlock_base.corruption->id();
      }
    }

    action_t* action = nullptr;
    if ( action_id != 0 )
    {
      for ( auto a : action_list )
      {
        if ( a->id == action_id )
        {
          action = a;
          break;
        }
      }
    }

    return make_fn_expr( name_str, [ this, action, dot_sel ]()
    {
      size_t dot_refresh_targets = 0;

      if ( !action || dot_sel == refreshable_dot_e::INVALID )
        return dot_refresh_targets;

      action_state_t* state = nullptr;

      const auto& tl = action->target_list();
      for ( auto t : tl )
      {
        warlock_td_t* tdata = get_target_data( t );

        auto& dot = [ dot_sel, tdata ]() -> auto&
        {
          switch ( dot_sel )
          {
            case refreshable_dot_e::WITHER:
              return tdata->dots.wither;
            case refreshable_dot_e::IMMOLATE:
              return tdata->dots.immolate;
            case refreshable_dot_e::CORRUPTION:
              return tdata->dots.corruption;
            case refreshable_dot_e::AGONY:
              return tdata->dots.agony;
            case refreshable_dot_e::UA:
              return tdata->dots.unstable_affliction;
            case refreshable_dot_e::SOC:
              return tdata->dots.seed_of_corruption;
            default:
              assert( false && "Unhandled refreshable_dot_e value" );
              return tdata->dots.corruption;
          }
        }();

        if ( dot == nullptr || !dot->is_ticking() )
        {
          dot_refresh_targets++;
          continue;
        }

        if ( !state || state->action != dot->current_action )
        {
          if ( state )
            action_state_t::release( state );

          state = dot->current_action->get_state();
        }

        dot->current_action->snapshot_state( state, result_amount_type::DMG_OVER_TIME );
        timespan_t new_duration = dot->current_action->composite_dot_duration( state );

        if ( dot->current_action->dot_refreshable( dot, new_duration ) )
          dot_refresh_targets++;
      }

      if ( state )
        action_state_t::release( state );

      return dot_refresh_targets;
    } );
  }

  return player_t::create_expression( name_str );
}

/* ----------------------------------------------------------
* NOTE NOTE NOTE
* Applies DYNAMIC (Buffs, Debuffs, DoTs, or anything else that could change state during combat)
* effects that effect the player as a whole, IE: a % Crit Chance buff, all pet/guardian damage modifiers, and the likes
* NOTE NOTE NOTE
*
* This system can also handle passive effects, but increases sim initialization time!
*
* General Useage is parse_effects( buff, modifying_spell_1, modifying_spell_2, modifying_spell_3 );
*
* USEAGE EXAMPLES *
* -----------------
* Baseline effect with no affecting talents, or spells
* --
* parse_effects( warlock_base.affliction_warlock );
* --
****** This system CAN NOT handle buffs that modify other buffs and/or debuffs. ******
* Debuff
* --
* parse_target_effects( d_fn( &warlock_td_t::debuffs_t::fel_sunder ), talents.fell_sunder );
* --
* DoT
* --
* parse_target_effects( d_fn( &warlock_td_t::dots_t::unstable_affliction ), talents.unstable_affliction );
* --
* More advanced examples can be found in other modules that use this system.
* A few are sc_druid.cpp, sc_death_knight.cpp, and sc_demon_hunter.cpp
------------------------------------------------------------- */
void warlock_t::parse_player_effects()
{
  // Shared
  if ( !demonology() )
  {
    parse_effects( buffs.grimoire_of_sacrifice ); // 196099
  }

  // Affliction
  if ( affliction() )
  {
    // Affliction Mastery
    parse_effects( warlock_base.potent_afflictions ); // 77215
    // Affliction Debuffs/DoTs
    // NOTE: Shadow of Nathreza II (rank 2) only increases by 2% (as if it were rank 1) the
    // effect #2 and #3 (pet and guardian damage) of the Haunt debuff damage bonus (bug)
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::haunt ), talents.haunt );
  }

  // Demonology
  if ( demonology() )
  {
    // Demonology Mastery
    parse_effects( warlock_base.master_demonologist ); // 77219
    // Demonology Buffs
    parse_effects( buffs.hellbent_commander ); // 1281559
  }

  // Destruction
  if ( destruction() )
  {
  }

  // Diabolist
  if ( diabolist() )
  {
    // Diabolist Buffs
    parse_effects( buffs.abyssal_dominion ); // 456323

    // Diabolist Debuffs/DoTs
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::cloven_soul ), hero.cloven_soul_debuff ); // 434424
  }

  // Hellcaller
  if ( hellcaller() )
  {
  }

  // Soul Harvester
  if ( soul_harvester() )
  {
    //parse_effects( buffs.manifested_demonic_soul ); // 1269042 // Implemented manually in 'composite_mastery'
  }
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  double actual_amount = player_t::resource_gain( resource_type, amount, source, action );

  // Succulent Soul proc from Demonic Soul talent can only occur from a effective soul shard gain (not overflow)
  if ( resource_type == RESOURCE_SOUL_SHARD && actual_amount > 0.0 && hero.demonic_soul.ok() )
  {
    for ( int i = 0; i < as<int>( actual_amount ); i++ )
    {
      if ( prd_rng.succulent_soul->trigger() )
      {
        buffs.succulent_soul->trigger();
        procs.succulent_soul->occur();
      }
    }
  }

  return actual_amount;
}

void warlock_t::feast_of_souls_gain( bool from_quietus_seed )
{
  // NOTE: 2026-03-17 The shard gained from Feast of Souls can also proc another Succulent Soul (bug?)
  if ( bugs )
    resource_gain( RESOURCE_SOUL_SHARD, 1.0, gains.feast_of_souls );
  else
    player_t::resource_gain( RESOURCE_SOUL_SHARD, 1.0, gains.feast_of_souls );

  buffs.succulent_soul->trigger();
  procs.succulent_soul->occur();
  procs.feast_of_souls->occur();

  // NOTE: 2026-03-06 If Feast of Souls is gained by consuming Nightfall with SoC (Quietus hero talent) and after
  // the gain you only have one stack of Succulent Soul, it will be spent without producing its effects (bug)
  // This behavior is modeled here simply by decrementing a stack of Succulent Soul without triggering its effects
  if ( bugs && from_quietus_seed && buffs.succulent_soul->check() == 1 )
    buffs.succulent_soul->decrement();
}

// Setup to allow taking specific pets from Dominion of Argus, or a random one if the random enum is passed in.
// This is just to future proof this in case of a tier set, talent or something else that summons a specific pet from
// Dominion of Argus being added.
void warlock_t::summon_dominion_of_argus_pet( dominion_of_argus_pet_e pet )
{
  dominion_of_argus_pet_e actual_pet = pet;
  // Odds for each pet currently seem to be
  // Jailer: 30%, Sacrolash: 20%, Grand Warlock Alythess: 20%, Inquisitor: 30% (last tested 2026-02-19)
  // More testing required for more accurate rates.
  // Probably better to do a better weighted random selection than this.
  if ( pet == DOA_PET_RANDOM )
  {
    const std::array<dominion_of_argus_pet_e, 10> pets = {
        DOA_PET_JAILER,        DOA_PET_JAILER,        DOA_PET_JAILER,     DOA_PET_SACROLASH,  DOA_PET_SACROLASH,
        DOA_PET_GRAND_WARLOCK, DOA_PET_GRAND_WARLOCK, DOA_PET_INQUISITOR, DOA_PET_INQUISITOR, DOA_PET_INQUISITOR };
    actual_pet = rng().range( pets );
  }

  switch ( actual_pet )
  {
    case DOA_PET_JAILER:
      summons.antoran_jailer->execute();
      break;
    case DOA_PET_SACROLASH:
      summons.lady_sacrolash->execute();
      break;
    case DOA_PET_GRAND_WARLOCK:
      summons.grand_warlock_alythess->execute();
      break;
    case DOA_PET_INQUISITOR:
      summons.antoran_inquisitor->execute();
      break;
    default:
      break;
  }
}

std::vector<player_t*> warlock_t::get_smart_targets( const std::vector<player_t*>& _tl, propagate_const<dot_t*> warlock_td_t::dots_t::* dot, int n_targets, player_t* exclude, double dis, bool really_smart )
{
  if ( n_targets < 1 || !_tl.size() )
    return {};

  auto tl = _tl; // make a copy

  if ( exclude )
  {
    if ( dis && sim->distance_targeting_enabled )
    {
      // remove out of range
      range::erase_remove( tl, [ exclude, dis ]( player_t* t ) {
        return t == exclude || t->get_player_distance( *exclude ) > dis;
      } );
    }
    else
    {
      range::erase_remove( tl, exclude );
    }
  }

  if ( tl.size() > 1 )
  {
    // randomize remaining targets
    rng().shuffle( tl.begin(), tl.end() );

    if ( really_smart )
    {
      // sort by time remaining
      range::sort( tl, [ this, &dot ]( player_t* a, player_t* b ) {
        return std::invoke( dot, get_target_data( a )->dots )->remains() <
               std::invoke( dot, get_target_data( b )->dots )->remains();
      } );
    }
    else
    {
      // prioritize undotted over dotted
      std::partition( tl.begin(), tl.end(), [ this, &dot ]( player_t* t ) {
        return !std::invoke( dot, get_target_data( t )->dots )->is_ticking();
      } );
    }
  }

  // slice to n_targets
  if ( as<int>( tl.size() ) > n_targets )
    tl.resize( n_targets );

  return tl;
}

player_t* warlock_t::get_smart_target( const std::vector<player_t*>& _tl, propagate_const<dot_t*> warlock_td_t::dots_t::* dot, player_t* exclude, double dis, bool really_smart )
{
  std::vector<player_t*> players = get_smart_targets( _tl, dot, 1, exclude, dis, really_smart );

  if ( players.size() )
    return players[ 0 ];

  return nullptr;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t : public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ) : p( player )
  {}

  void html_customsection( report::sc_html_stream& ) override
  {}

private:
  [[maybe_unused]] warlock_t& p;
};

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK )
  { }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new warlock_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new warlock_report_t( *p ) );
    return p;
  }

  void register_hotfixes() const override
  { }

  void register_actor_initializers( sim_t* ) const override
  { }

  bool valid() const override
  { return true; }
};

warlock::warlock_t::pets_t::pets_t( warlock_t* w )
  : active( nullptr ),
    infernals( "infernal", w ),
    darkglares( "darkglare", w ),
    desperate_souls( "desperate_soul", w ),
    dreadstalkers( "dreadstalker", w ),
    vilefiends( "vilefiend", w ),
    demonic_tyrants( "demonic_tyrant", w ),
    grimoire_imp_lords( "demonic_imp_lord", w ),
    grimoire_fel_ravagers( "demonic_fel_ravager", w ),
    wild_imps( "wild_imp", w ),
    doomguards( "doomguard", w ),
    lady_sacrolash( "lady_sacrolash", w ),
    grand_warlock_alythess( "grand_warlock_alythess", w ),
    antoran_inquisitor( "antoran_inquisitor", w ),
    antoran_jailer( "antoran_jailer", w ),
    shadowy_rifts( "shadowy_tear", w ),
    unstable_rifts( "unstable_tear", w ),
    chaos_rifts( "chaos_tear", w ),
    rocs( "infernal_roc", w ),
    overfiends( "overfiend", w ),
    overlords( "overlord", w ),
    mothers( "mother_of_chaos", w ),
    pit_lords( "pit_lord", w ),
    fragments( "infernal_fragment", w ),
    diabolic_imps( "diabolic_imp", w ),
    demonic_souls( "demonic_soul", w )
{ }
}  // namespace warlock

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
