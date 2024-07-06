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
  dots_drain_life = target->get_dot( "drain_life", &p );
  dots_drain_life_aoe = target->get_dot( "drain_life_aoe", &p );

  // Affliction
  dots_corruption = target->get_dot( "corruption", &p );
  dots_agony = target->get_dot( "agony", &p );
  dots_drain_soul = target->get_dot( "drain_soul", &p );
  dots_phantom_singularity = target->get_dot( "phantom_singularity", &p );
  dots_seed_of_corruption = target->get_dot( "seed_of_corruption", &p );
  dots_unstable_affliction = target->get_dot( "unstable_affliction", &p );
  dots_vile_taint = target->get_dot( "vile_taint_dot", &p );
  dots_soul_rot = target->get_dot( "soul_rot", &p );

  debuffs_haunt = make_buff( *this, "haunt", p.talents.haunt )
                      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                      ->set_default_value_from_effect( 2 )
                      ->set_cooldown( 0_ms );

  debuffs_shadow_embrace = make_buff( *this, "shadow_embrace", p.talents.shadow_embrace_debuff )
                               ->set_default_value( p.talents.shadow_embrace->effectN( 1 ).percent() );

  debuffs_infirmity = make_buff( *this, "infirmity", p.tier.infirmity )
                          ->set_default_value( p.tier.infirmity->effectN( 1 ).percent() )
                          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Demonology
  debuffs_the_houndmasters_stratagem = make_buff( *this, "the_houndmasters_stratagem", p.talents.the_houndmasters_stratagem_debuff )
                                           ->set_default_value_from_effect( 1 );

  debuffs_fel_sunder = make_buff( *this, "fel_sunder", p.talents.fel_sunder_debuff )
                           ->set_default_value( p.talents.fel_sunder->effectN( 1 ).percent() );

  // Destruction
  dots_immolate = target->get_dot( "immolate", &p );

  debuffs_eradication = make_buff( *this, "eradication", p.talents.eradication_debuff )
                            ->set_default_value( p.talents.eradication->effectN( 2 ).percent() );

  debuffs_shadowburn = make_buff( *this, "shadowburn", p.talents.shadowburn )
                           ->set_default_value( p.talents.shadowburn_2->effectN( 1 ).base_value() / 10 );

  debuffs_pyrogenics = make_buff( *this, "pyrogenics", p.talents.pyrogenics_debuff )
                           ->set_default_value( p.talents.pyrogenics->effectN( 1 ).percent() )
                           ->set_schools_from_effect( 1 )
                           ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  debuffs_conflagrate = make_buff( *this, "conflagrate", p.talents.conflagrate_debuff )
                            ->set_default_value_from_effect( 1 );

  // Use havoc_debuff where we need the data but don't have the active talent
  debuffs_havoc = make_buff( *this, "havoc", p.talents.havoc_debuff )
                      ->set_duration( p.talents.mayhem.ok() ? p.talents.mayhem->effectN( 3 ).time_value() : p.talents.havoc->duration() )
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
                            p.get_target_data( p.havoc_target )->debuffs_havoc->expire();
                          p.havoc_target = b->player;
                        }

                        range::for_each( p.havoc_spells, []( action_t* a ) { a->target_cache.is_valid = false; } );
                      } );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void warlock_td_t::target_demise()
{
  if ( !( target->is_enemy() ) )
    return;

  if ( dots_unstable_affliction->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Unstable Affliction.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, warlock.talents.unstable_affliction_2->effectN( 1 ).base_value(), warlock.gains.unstable_affliction_refund );
  }

  if ( dots_drain_soul->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Drain Soul.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1.0, warlock.gains.drain_soul );
  }

  if ( debuffs_haunt->check() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} reset Haunt's cooldown.", target->name(), warlock.name() );

    warlock.cooldowns.haunt->reset( true );
  }

  if ( debuffs_shadowburn->check() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} refunds one charge of Shadowburn.", target->name(), warlock.name() );
      
    warlock.cooldowns.shadowburn->reset( true );
   
    warlock.sim->print_log( "Player {} demised. Warlock {} gains 1 shard from Shadowburn.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, debuffs_shadowburn->check_value(), warlock.gains.shadowburn_refund );
  }
}

int warlock_td_t::count_affliction_dots() const
{
  int count = 0;

  if ( dots_agony->is_ticking() )
    count++;

  if ( dots_corruption->is_ticking() )
    count++;

  if ( dots_seed_of_corruption->is_ticking() )
    count++;

  if ( dots_unstable_affliction->is_ticking() )
    count++;

  if ( dots_vile_taint->is_ticking() )
    count++;

  if ( dots_phantom_singularity->is_ticking() )
    count++;

  if ( dots_soul_rot->is_ticking() )
    count++;

  return count;
}


warlock_t::warlock_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    ua_target( nullptr ),
    havoc_spells(),
    agony_accumulator( 0.0 ),
    corruption_accumulator( 0.0 ),
    shadow_invocation_proc_chance( 0.0 ),
    active_pets( 0 ),
    warlock_pet_list( this ),
    talents(),
    hero(),
    proc_actions(),
    tier(),
    cooldowns(),
    buffs(),
    gains(),
    procs(),
    initial_soul_shards( 3 ),
    default_pet(),
    disable_auto_felstorm( false )
{
  cooldowns.haunt = get_cooldown( "haunt" );
  cooldowns.shadowburn = get_cooldown( "shadowburn" );
  cooldowns.soul_fire = get_cooldown( "soul_fire" );
  cooldowns.felstorm_icd = get_cooldown( "felstorm_icd" );

  resource_regeneration = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ] = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;
}

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( warlock_base.master_demonologist->ok() )
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;
    default:
      break;
  }
}

double warlock_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  const warlock_td_t* td = get_target_data( target );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( talents.haunt.ok() )
      m *= 1.0 + td->debuffs_haunt->check_value();

    if ( talents.shadow_embrace.ok() )
      m *= 1.0 + td->debuffs_shadow_embrace->check_stack_value();

    if ( sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
      m *= 1.0 + td->debuffs_infirmity->check_stack_value();
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( talents.eradication.ok() )
      m *= 1.0 + td->debuffs_eradication->check_value();

    if ( talents.pyrogenics.ok() && td->debuffs_pyrogenics->has_common_school( school ) )
      m *= 1.0 + td->debuffs_pyrogenics->check_value();
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    if ( talents.fel_sunder.ok() )
      m *= 1.0 + td->debuffs_fel_sunder->check_stack_value();
  }

  return m;
}

double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    m *= 1.0 + buffs.rolling_havoc->check_stack_value();
  }

  return m;
}

double warlock_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    m *= 1.0 + warlock_base.destruction_warlock->effectN( guardian ? 4 : 3 ).percent();

    // 2022-11-27 Rolling Havoc is missing the aura for guardians
    if ( talents.rolling_havoc.ok() && !guardian )
      m *= 1.0 + buffs.rolling_havoc->check_stack_value();
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    m *= 1.0 + warlock_base.demonology_warlock->effectN( guardian ? 5 : 3 ).percent();
    
    // Renormalize to use the guardian effect when appropriate, in case the values are ever different
    if ( !guardian )
      m *= 1.0 + cache.mastery_value();
    else
      m *= 1.0 + ( cache.mastery_value() ) * ( warlock_base.master_demonologist->effectN( 3 ).sp_coeff() / warlock_base.master_demonologist->effectN( 1 ).sp_coeff() );
  }

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    m *= 1.0 + warlock_base.affliction_warlock->effectN( guardian ? 7 : 3 ).percent();
  }

  return m;
}

double warlock_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  const warlock_td_t* td = get_target_data( target );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( talents.haunt.ok() && td->debuffs_haunt->check() )
      m *= 1.0 + td->debuffs_haunt->data().effectN( guardian ? 4 : 3 ).percent();

    if ( talents.shadow_embrace.ok() )
      m *= 1.0 + td->debuffs_shadow_embrace->check_stack_value();

    if ( sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) && !guardian )
    {
      // TOCHECK: Guardian effect is missing from spell data as of 2023-04-04
      m *= 1.0 + td->debuffs_infirmity->check_stack_value();
    }
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( talents.eradication.ok() )
      m *= 1.0 + td->debuffs_eradication->check_value();
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    // Fel Sunder lacks guardian effect, so only main pet is benefitting. Last checked 2022-11-27
    if ( talents.fel_sunder.ok() && !guardian )
      m *= 1.0 + td->debuffs_fel_sunder->check_stack_value();
  }

  return m;
}

double warlock_t::composite_spell_crit_chance() const
{
  double m = player_t::composite_spell_crit_chance();

  m += talents.demonic_tactics->effectN( 1 ).percent();

  if ( specialization() == WARLOCK_DESTRUCTION && talents.backlash.ok() )
    m += talents.backlash->effectN( 1 ).percent();

  return m;
}

double warlock_t::composite_melee_crit_chance() const
{
  double m = player_t::composite_melee_crit_chance();

  if ( specialization() == WARLOCK_DESTRUCTION && talents.backlash.ok() )
    m += talents.backlash->effectN( 1 ).percent();

  return m;
}

double warlock_t::composite_rating_multiplier( rating_e r ) const
{
  double m = player_t::composite_rating_multiplier( r );

  switch ( r )
  {
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
    case RATING_SPELL_CRIT:
      m *= 1.0 + talents.demonic_tactics->effectN( 2 ).percent();
      break;
    default:
      break;
  }

  return m;
}

// Note: Level is checked to be >=27 by the function calling this. This is technically wrong for warlocks due to
// a missing level requirement in data, but correct generally.
double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return warlock_base.nethermancy->effectN( 1 ).percent();

  return 0.0;
}

static void accumulate_seed_of_corruption( warlock_td_t* td, double amount )
{
  td->soc_threshold -= amount;

  if ( td->soc_threshold <= 0 )
    td->dots_seed_of_corruption->cancel();
  else if ( td->source->sim->log )
    td->source->sim->print_log( "Remaining damage to explode Seed of Corruption on {} is {}.", td->target->name_str, td->soc_threshold );
}

void warlock_t::init_assessors()
{
  player_t::init_assessors();

  auto assessor_fn = [ this ]( result_amount_type rt, action_state_t* s ){
    if ( get_target_data( s->target )->dots_seed_of_corruption->is_ticking() )
      accumulate_seed_of_corruption( get_target_data( s->target ), s->result_total );

    return assessor::CONTINUE;
  };

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );

  for ( auto pet : pet_list )
  {
    pet->assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
  }
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

int warlock_t::active_demon_count() const
{
  int count = 0;

  for ( auto& pet : this->pet_list )
  {
    auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );
    
    if ( lock_pet == nullptr )
      continue;
    if ( lock_pet->is_sleeping() )
      continue;
    
    count++;
  }

  return count;
}

void warlock_t::expendables_trigger_helper( warlock_pet_t* source )
{
  for ( auto& pet : this->pet_list )
  {
    auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

    if ( lock_pet == nullptr )
      continue;
    if ( lock_pet->is_sleeping() )
      continue;

    if ( lock_pet == source )
      continue;

    // Pit Lord is not affected by The Expendables
    if ( lock_pet->pet_type == PET_PIT_LORD )
      continue;

    lock_pet->buffs.the_expendables->trigger();
  }
}

// Use this as a helper function when two versions are needed simultaneously (ie a PTR cycle)
// It must be adjusted manually over time, and any use of it should be removed once a patch goes live
// Returns TRUE if actor's dbc version >= version specified
// When checking VERSION_PTR, will only return true if PTR dbc is being used, regardless of version number
bool warlock_t::min_version_check( version_check_e version ) const
{
  //If we ever get a full DBC version string checker, replace these returns with that function
  switch ( version )
  {
    case VERSION_PTR:
      return is_ptr();
    case VERSION_ANY:
      return true;
  }

  return false;
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype & SAVE_PLAYER )
  {
    if ( initial_soul_shards != 3 )
      profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )
      profile_str += "default_pet=" + default_pet + "\n";
    if ( disable_auto_felstorm )
      profile_str += "disable_felstorm=" + util::to_string( disable_auto_felstorm );
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

pet_t* warlock_t::create_main_pet( util::string_view pet_name, util::string_view pet_type )
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
  if ( pet_name == "felguard" && specialization() == WARLOCK_DEMONOLOGY )
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
      dot_t* agony          = td->dots_agony;
      double active_agonies = get_active_dots( agony );
      if ( sim->debug )
        sim->out_debug.printf( "active agonies: %f", active_agonies );
      
      if ( active_agonies == 0 || !agony->current_action )
      {
        return std::numeric_limits<double>::infinity();
      }
      action_state_t* agony_state = agony->current_action->get_state( agony->state );
      timespan_t dot_tick_time    = agony->current_action->tick_time( agony_state );

      // Seeks to return the average expected time for the player to generate a single soul shard.
      // TOCHECK regularly.

      double average =
          1.0 / ( 0.184 * std::pow( active_agonies, -2.0 / 3.0 ) ) * dot_tick_time.total_seconds() / active_agonies;

      if ( sim->debug )
        sim->out_debug.printf( "time to shard return: %f", average );

      action_state_t::release( agony_state );
      return average;
    } );
  }
  else if ( name_str == "pet_count" )
  {
    return make_ref_expr( name_str, active_pets );
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
      return havoc_target ? get_target_data( havoc_target )->debuffs_havoc->remains().total_seconds() : 0.0;
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
        if ( !get_target_data( t )->dots_seed_of_corruption->is_ticking() )
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
    });
  }

  auto splits = util::string_split<util::string_view>( name_str, "." );

  if ( splits.size() == 3 && splits[ 0 ] == "time_to_imps" && splits[ 2 ] == "remains" )
  {
    auto amt = splits[ 1 ] == "all" ? -1 : util::to_int( splits[ 1 ] );

    return make_fn_expr( name_str, [ this, amt ]() {
      return this->time_to_imps( amt );
    } );
  }

  return player_t::create_expression( name_str );
}

void warlock_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  if ( warlock_base.demonology_warlock )
  {
    action.apply_affecting_aura( warlock_base.demonology_warlock );
  }
  if ( warlock_base.destruction_warlock )
  {
    action.apply_affecting_aura( warlock_base.destruction_warlock );
  }
  if ( warlock_base.affliction_warlock )
  {
    action.apply_affecting_aura( warlock_base.affliction_warlock );
  }

  // TODO: Remove apply_affectings and move to affected_by in actions
  action.apply_affecting_aura( talents.dark_virtuosity );
  action.apply_affecting_aura( talents.kindled_malice );
  action.apply_affecting_aura( talents.xavius_gambit );
}

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK )
  { }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  { return new warlock_t( sim, name, r ); }

  void register_hotfixes() const override
  { }

  bool valid() const override
  { return true; }

  void init( player_t* ) const override
  { }

  void combat_begin( sim_t* ) const override
  { }

  void combat_end( sim_t* ) const override
  { }
};

warlock::warlock_t::pets_t::pets_t( warlock_t* w )
  : active( nullptr ),
    infernals( "infernal", w ),
    blasphemy( "blasphemy", w ),
    darkglares( "darkglare", w ),
    dreadstalkers( "dreadstalker", w ),
    vilefiends( "vilefiend", w ),
    demonic_tyrants( "demonic_tyrant", w ),
    grimoire_felguards( "grimoire_felguard", w ),
    wild_imps( "wild_imp", w )
{ }
}  // namespace warlock

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
