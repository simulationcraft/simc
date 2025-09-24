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
  dots.phantom_singularity = target->get_dot( "phantom_singularity", &p );
  dots.seed_of_corruption = target->get_dot( "seed_of_corruption", &p );
  dots.unstable_affliction = target->get_dot( "unstable_affliction", &p );
  dots.jackpot_ua = target->get_dot( "unstable_affliction_jackpot", &p );
  dots.vile_taint = target->get_dot( "vile_taint_dot", &p );
  dots.soul_rot = target->get_dot( "soul_rot", &p );

  debuffs.haunt = make_buff( *this, "haunt", p.talents.haunt )
                      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                      ->set_default_value_from_effect( 2 )
                      ->set_cooldown( 0_ms );

  debuffs.shadow_embrace = make_buff( *this, "shadow_embrace", p.talents.drain_soul.ok() ? p.talents.shadow_embrace_debuff_ds : p.talents.shadow_embrace_debuff_sb )
                               ->set_default_value_from_effect( 1 );

  debuffs.infirmity = make_buff( *this, "infirmity", p.talents.infirmity_debuff )
                          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Demonology
  debuffs.wicked_maw = make_buff( *this, "wicked_maw", p.talents.wicked_maw_debuff )
                           ->set_default_value_from_effect( 1 );

  debuffs.fel_sunder = make_buff( *this, "fel_sunder", p.talents.fel_sunder_debuff )
                           ->set_default_value( p.talents.fel_sunder->effectN( 1 ).percent() );

  debuffs.doom = make_buff( *this, "doom", p.talents.doom_debuff )
                     ->set_stack_change_callback( [ &p ]( buff_t* b, int, int cur ) {
                       if ( cur == 0 )
                       {
                         p.proc_actions.doom_proc->execute_on_target( b->player );

                         if ( p.talents.pact_of_the_eredruin.ok() && p.rng().roll( p.rng_settings.pact_of_the_eredruin.setting_value ) )
                         {
                           p.warlock_pet_list.doomguards.spawn( 1u );
                           p.procs.pact_of_the_eredruin->occur();
                         }
                       }
                       } );

  // Destruction
  dots.immolate = target->get_dot( "immolate", &p );

  debuffs.eradication = make_buff( *this, "eradication", p.talents.eradication_debuff );

  debuffs.shadowburn = make_buff( *this, "shadowburn", p.talents.shadowburn )
                           ->set_default_value( p.talents.shadowburn_2->effectN( 1 ).base_value() / 10 );

  debuffs.pyrogenics = make_buff( *this, "pyrogenics", p.talents.pyrogenics_debuff )
                           ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  debuffs.conflagrate = make_buff( *this, "conflagrate", p.talents.conflagrate_debuff )
                            ->set_default_value_from_effect( 1 );

  // Use havoc_debuff where we need the data but don't have the active talent
  debuffs.havoc = make_buff( *this, "havoc", p.talents.havoc_debuff )
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
                               ->set_tick_callback( [ this, target ]( buff_t*, int, timespan_t ) {
                                 warlock.proc_actions.blackened_soul->execute_on_target( target );
                               } )
                               ->set_tick_behavior( buff_tick_behavior::REFRESH )
                               ->set_freeze_stacks( true )
                               ->set_tick_time_behavior( buff_tick_time_behavior::CUSTOM )
                               ->set_tick_time_callback( [ & ]( const buff_t* b, unsigned int ) {
                                 timespan_t period = b->buff_period;

                                 if ( p.buffs.maintained_withering->check() )
                                 {
                                   // TOCHECK: 2025-08-16 Currently Hellcaller TWW3 B4 (Maintained Withering) tier bonus
                                   // of doing Blackened Soul damage faster is bugged for destruction and does not work
                                   if ( !p.bugs || !p.destruction() )
                                   {
                                     period *= 1.0 + p.buffs.maintained_withering->data()
                                                         .effectN( p.affliction() ? 2 : 3 )
                                                         .percent();
                                   }
                                 }
                                 return period;
                               } );

  // Soul Harvester
  dots.soul_anathema = target->get_dot( "soul_anathema", &p );

  dots.shared_fate = target->get_dot( "shared_fate", &p );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void warlock_td_t::target_demise()
{
  if ( !( target->is_enemy() ) )
    return;

  if ( dots.unstable_affliction->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Unstable Affliction.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, warlock.talents.unstable_affliction_2->effectN( 1 ).base_value(), warlock.gains.unstable_affliction_refund );
  }

  if ( dots.jackpot_ua->is_ticking() )
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
    warlock.sim->print_log( "Player {} demised. Warlock {} refunds one charge of Shadowburn.", target->name(), warlock.name() );
      
    warlock.cooldowns.shadowburn->reset( true );
   
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
    double chance = 0.0;
    if ( warlock.affliction() )
      chance = warlock.rng_settings.feast_of_souls_aff.setting_value;
    if ( warlock.demonology() )
      chance = warlock.rng_settings.feast_of_souls_demo.setting_value;

    if ( warlock.rng().roll( chance ) )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} triggers Feast of Souls.", target->name(), warlock.name() );

      warlock.feast_of_souls_gain();
    }
  }
}

int warlock_td_t::count_affliction_dots() const
{
  // NOTE: Shared Fate and Soul Anathema DoTs do not count (they do not affect effects influenced by this count)
  int count = 0;

  if ( dots.agony->is_ticking() )
    count++;

  if ( dots.corruption->is_ticking() )
    count++;

  if ( dots.seed_of_corruption->is_ticking() )
    count++;

  if ( dots.unstable_affliction->is_ticking() )
    count++;

  if ( dots.vile_taint->is_ticking() )
    count++;

  if ( dots.phantom_singularity->is_ticking() )
    count++;

  if ( dots.soul_rot->is_ticking() )
    count++;

  if ( dots.wither->is_ticking() )
    count++;

  return count;
}

int warlock_td_t::count_affliction_dots( bool include_tier_ua ) const
{
  int count = count_affliction_dots();

  if ( !include_tier_ua )
    return count;

  if ( dots.jackpot_ua->is_ticking() )
    count++;

  return count;
}


warlock_t::warlock_t( sim_t* sim, util::string_view name, race_e r )
  : parse_player_effects_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    ua_target( nullptr ),
    havoc_spells(),
    agony_accumulator( 0.0 ),
    corruption_accumulator( 0.0 ),
    diabolic_ritual( 0 ),
    demonic_art_buff_replaced( false ),
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
    rng_settings(),
    initial_soul_shards( 3 ),
    default_pet(),
    disable_auto_felstorm( false ),
    normalize_destruction_mastery( false )
{
  cooldowns.haunt = get_cooldown( "haunt" );
  cooldowns.shadowburn = get_cooldown( "shadowburn" );
  cooldowns.soul_fire = get_cooldown( "soul_fire" );
  cooldowns.dimensional_rift = get_cooldown( "dimensional_rift" );
  cooldowns.felstorm_icd = get_cooldown( "felstorm_icd" );
  cooldowns.blackened_soul = get_cooldown( "blackened_soul_icd" );
  cooldowns.seeds_of_their_demise = get_cooldown( "seeds_of_their_demise_icd" );

  resource_regeneration = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ] = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;
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

double warlock_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = parse_player_effects_t::composite_player_target_pet_damage_multiplier( target, guardian );

  if ( demonology() )
  {
    // TOCHECK: Fel Sunder lacks guardian effect, so only player and main pet is benefitting (bug?). Last checked 2025-09-23
    if ( !bugs && guardian && talents.fel_sunder.ok() )
      m *= 1.0 + get_target_data( target )->debuffs.fel_sunder->check_stack_value();
  }

  return m;
}

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

  auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ){
    if ( get_target_data( s->target )->dots.seed_of_corruption->is_ticking() )
      accumulate_seed_of_corruption( get_target_data( s->target ), s->result_total );

    return assessor::CONTINUE;
  };

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );

  for ( auto pet : pet_list )
  {
    pet->assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
  }
}

void warlock_t::init_finished()
{
  for ( auto& b : buff_list )
    if ( b->data().ok() )
      apply_affecting_auras( *b );

  parse_player_effects();

  parse_player_effects_t ::init_finished();
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

    // 2025-03-28: The Expendables talent does not apply to Greater Dreadstalkers (maybe a bug?)
    if ( ( lock_pet->pet_type != PET_FELHUNTER || !lock_pet->bugs ) && lock_pet->pet_type != PET_WARLOCK_RANDOM )
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
    case VERSION_11_1_0:
      return !( version_11_1_0_data == spell_data_t::not_found() );
    case VERSION_ANY:
      return true;
  }

  return false;
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
      profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )
      profile_str += "default_pet=" + default_pet + "\n";
    if ( disable_auto_felstorm )
      profile_str += "disable_felstorm=" + util::to_string( disable_auto_felstorm ) + "\n";
    if ( normalize_destruction_mastery )
      profile_str += "normalize_destruction_mastery=" + util::to_string( normalize_destruction_mastery ) + "\n";

    profile_str += append_rng_option( rng_settings.cunning_cruelty_sb );
    profile_str += append_rng_option( rng_settings.cunning_cruelty_ds );
    profile_str += append_rng_option( rng_settings.agony );
    profile_str += append_rng_option( rng_settings.nightfall );
    profile_str += append_rng_option( rng_settings.pact_of_the_eredruin );
    profile_str += append_rng_option( rng_settings.shadow_invocation );
    profile_str += append_rng_option( rng_settings.spiteful_reconstitution );
    profile_str += append_rng_option( rng_settings.decimation );
    profile_str += append_rng_option( rng_settings.dimension_ripper );
    profile_str += append_rng_option( rng_settings.blackened_soul );
    profile_str += append_rng_option( rng_settings.bleakheart_tactics );
    profile_str += append_rng_option( rng_settings.seeds_of_their_demise );
    profile_str += append_rng_option( rng_settings.mark_of_perotharn );
    profile_str += append_rng_option( rng_settings.succulent_soul_aff );
    profile_str += append_rng_option( rng_settings.succulent_soul_demo );
    profile_str += append_rng_option( rng_settings.feast_of_souls_aff );
    profile_str += append_rng_option( rng_settings.feast_of_souls_demo );
    profile_str += append_rng_option( rng_settings.umbral_lattice );
    profile_str += append_rng_option( rng_settings.empowered_legion_strike );
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

  rng_settings.cunning_cruelty_sb = p->rng_settings.cunning_cruelty_sb;
  rng_settings.cunning_cruelty_ds = p->rng_settings.cunning_cruelty_ds;
  rng_settings.agony = p->rng_settings.agony;
  rng_settings.nightfall = p->rng_settings.nightfall;
  rng_settings.pact_of_the_eredruin = p->rng_settings.pact_of_the_eredruin;
  rng_settings.shadow_invocation = p->rng_settings.shadow_invocation;
  rng_settings.spiteful_reconstitution = p->rng_settings.spiteful_reconstitution;
  rng_settings.decimation = p->rng_settings.decimation;
  rng_settings.dimension_ripper = p->rng_settings.dimension_ripper;
  rng_settings.blackened_soul = p->rng_settings.blackened_soul;
  rng_settings.bleakheart_tactics = p->rng_settings.bleakheart_tactics;
  rng_settings.seeds_of_their_demise = p->rng_settings.seeds_of_their_demise;
  rng_settings.mark_of_perotharn = p->rng_settings.mark_of_perotharn;
  rng_settings.succulent_soul_aff = p->rng_settings.succulent_soul_aff;
  rng_settings.succulent_soul_demo = p->rng_settings.succulent_soul_demo;
  rng_settings.feast_of_souls_aff = p->rng_settings.feast_of_souls_aff;
  rng_settings.feast_of_souls_demo = p->rng_settings.feast_of_souls_demo;
  rng_settings.umbral_lattice = p->rng_settings.umbral_lattice;
  rng_settings.empowered_legion_strike = p->rng_settings.empowered_legion_strike;
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
  if ( pet_name == "felguard" && demonology() )
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
    });
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
* Buff that is modified by a talent (Buff with a talent that modifies an effect to have a value)
* --
* parse_effects( buff.rolling_havoc, talents.rolling_havoc );
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
  parse_effects( warlock_base.nethermancy ); // 86091
  parse_effects( talents.demonic_tactics ); // 452894
  parse_effects( talents.demonic_embrace ); // 288843
  parse_effects( talents.wrathful_minion ); // 386864
  parse_effects( talents.demonic_fortitude ); // 386617
  if ( !demonology() )
  {
    parse_effects( talents.summoners_embrace ); // 453105
    parse_effects( buffs.grimoire_of_sacrifice ); // 196099
  }

  // Affliction
  if ( affliction() )
  {
    parse_effects( warlock_base.affliction_warlock ); // 137043
    parse_effects( warlock_base.potent_afflictions ); // 77215

    // Affliction Debuffs/DoTs
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::haunt ), talents.haunt ); // 48181
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::shadow_embrace ), talents.drain_soul.ok() ? talents.shadow_embrace_debuff_ds : talents.shadow_embrace_debuff_sb ); // 32390 / 453206
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::infirmity ), talents.infirmity_debuff ); // 458219
  }

  // Demonology
  if ( demonology() )
  {
    parse_effects( warlock_base.demonology_warlock ); // 137044
    parse_effects( warlock_base.master_demonologist ); // 77219
    parse_effects( talents.rune_of_shadows ); // 453744
    parse_effects( talents.master_summoner ); // 1240189
    parse_effects( talents.demonic_brutality ); // 453908
    parse_effects( tier.hexflame_demo_2pc ); // 453644 // TWW1

    // Demonology Debuffs/DoTs
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::fel_sunder ), talents.fel_sunder_debuff, talents.fel_sunder ); // 387402 (m: 387399)
  }

  // Destruction
  if ( destruction() )
  {
    parse_effects( warlock_base.destruction_warlock ); // 137046
    parse_effects( talents.backlash ); // 387384

    // Destruction Buffs
    parse_effects( buffs.rolling_havoc, talents.rolling_havoc ); // 387570 (m: 387569)

    // Destruction Debuffs/DoTs
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::eradication ), talents.eradication_debuff, talents.eradication ); // 196414 (m: 196412)
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::pyrogenics ), talents.pyrogenics_debuff, talents.pyrogenics ); // 387096 (m: 387095)
  }

  // Diabolist
  if ( diabolist() )
  {
    parse_effects( hero.flames_of_xoroth ); // 429657

    // Diabolist Buffs
    parse_effects( buffs.abyssal_dominion ); // 456323

    // Diabolist Debuffs/DoTs
    parse_target_effects( d_fn( &warlock_td_t::debuffs_t::cloven_soul ), hero.cloven_soul_debuff ); // 434424
  }

  // Hellcaller
  if ( hellcaller() )
  {
    if ( destruction() )
    {
      parse_effects( hero.xalans_ferocity, warlock_base.destruction_warlock ); // 440044 (m: 137046)
      parse_effects( hero.xalans_cruelty, warlock_base.destruction_warlock ); // 440040 (m: 137046)
    }
    else
    {
      parse_effects( hero.xalans_ferocity ); // 440044
      parse_effects( hero.xalans_cruelty ); // 440040
    }
    parse_effects( hero.illhoofs_design ); // 440070
  }

  // Soul Harvester

}

/* ----------------------------------------------------------
* NOTE NOTE NOTE
* Applies PASSIVE (Talents, Spec Auras, Baseline) 
* effects to any action created by the player 
* NOTE NOTE NOTE
------------------------------------------------------------- */
void warlock_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  // Shared
  action.apply_affecting_aura( talents.wrathful_minion ); // 386864
  action.apply_affecting_aura( talents.demonic_inspiration ); // 386858  // The attack speed increase to the main pet is a Dummy effect
  action.apply_affecting_aura( talents.sargerei_technique ); // 405955  // Sargerei Technique appears to double dip for Infernal Bolt due to Destro/Demo modifier
  // Only Socrethars Guile effects #1 and #4 are enabled for Affliction, and they are handled directly in agony_t
  if ( !affliction() )
    action.apply_affecting_aura( talents.socrethars_guile ); // 405936
  if ( !demonology() )
    action.apply_affecting_aura( talents.summoners_embrace ); // 453105

  // Affliction
  if ( affliction() )
  {
    action.apply_affecting_aura( warlock_base.affliction_warlock ); // 137043
    action.apply_affecting_aura( warlock_base.agony_2 ); // 231792
    action.apply_affecting_aura( talents.writhe_in_agony ); // 196102
    action.apply_affecting_aura( talents.dark_virtuosity ); // 405327
    action.apply_affecting_aura( talents.absolute_corruption ); // 196103
    action.apply_affecting_aura( talents.siphon_life ); // 452999
    action.apply_affecting_aura( talents.kindled_malice ); // 405330  // 2025-09-21 This is still affecting SoC
    action.apply_affecting_aura( talents.improved_shadow_bolt ); // 453080
    action.apply_affecting_aura( talents.sacrolashs_dark_strike ); // 386986
    action.apply_affecting_aura( talents.improved_haunt ); // 458034
    action.apply_affecting_aura( talents.malediction ); // 453087
    action.apply_affecting_aura( talents.malevolent_visionary ); // 387273
    action.apply_affecting_aura( talents.contagion ); // 453096
    action.apply_affecting_aura( talents.creeping_death ); // 264000
    action.apply_affecting_aura( talents.xavius_gambit ); // 416615
    action.apply_affecting_aura( talents.perpetual_unstability ); // 459376
    action.apply_affecting_aura( talents.improved_malefic_rapture ); // 454378
    action.apply_affecting_aura( tier.hexflame_aff_2pc ); // 453643 // TWW1
  }

  // Demonology
  if ( demonology() )
  {
    action.apply_affecting_aura( warlock_base.demonology_warlock ); // 137044
    action.apply_affecting_aura( talents.spiteful_reconstitution ); // 428394
    action.apply_affecting_aura( talents.rune_of_shadows ); // 453744
    action.apply_affecting_aura( talents.imperator ); // 416230
    action.apply_affecting_aura( talents.shadow_invocation ); // 422054
    action.apply_affecting_aura( talents.master_summoner ); // 1240189
    action.apply_affecting_aura( talents.impending_doom ); // 455587
    action.apply_affecting_aura( tier.hexflame_demo_2pc ); // 453644 // TWW1
  }

  // Destruction
  if ( destruction() )
  {
    action.apply_affecting_aura( warlock_base.destruction_warlock ); // 137046
    action.apply_affecting_aura( talents.improved_conflagrate ); // 231793
    action.apply_affecting_aura( talents.scalding_flames ); // 388832
    action.apply_affecting_aura( talents.explosive_potential ); // 388827
    action.apply_affecting_aura( talents.blistering_atrophy ); // 456939
    action.apply_affecting_aura( talents.emberstorm ); // 454744
    action.apply_affecting_aura( talents.raging_demonfire ); // 387166
    action.apply_affecting_aura( talents.demonfire_mastery ); // 456946
    action.apply_affecting_aura( talents.devastation ); // 454735
    action.apply_affecting_aura( talents.ruin ); // 387103
    action.apply_affecting_aura( talents.improved_chaos_bolt ); // 456951
    action.apply_affecting_aura( tier.hexflame_destro_2pc ); // 453647
  }

  // Diabolist
  if ( diabolist() )
  {
    action.apply_affecting_aura( hero.flames_of_xoroth ); // 429657
    action.apply_affecting_aura( hero.abyssal_dominion ); // 429581
  }

  // Hellcaller
  if ( hellcaller() )
  {
    if ( destruction() )
    {
      action.apply_affecting_aura( hero.xalans_ferocity, warlock_base.destruction_warlock ); // 440044 (m: 137046)
      action.apply_affecting_aura( hero.xalans_cruelty, warlock_base.destruction_warlock ); // 440040 (m: 137046)
    }
    else
    {
      action.apply_affecting_aura( hero.xalans_ferocity ); // 440044
      action.apply_affecting_aura( hero.xalans_cruelty ); // 440040
    }
    action.apply_affecting_aura( hero.hatefury_rituals ); // 440048
    action.apply_affecting_aura( hero.bleakheart_tactics ); // 440051

    // Effects #2 and #4 of Mark of Xavius are disabled for Affliction ( and effect #1 is handled in agony_t )
    if ( destruction() )
      action.apply_affecting_aura( hero.mark_of_xavius ); // 440046

    // NOTE: There are two effects in 'warlock_base.affliction_warlock' that modify Mark of Perotharn, but they cancel each other out
    // In any case, currently `action.apply_affecting_aura` can't handle two modifier effects (from the same modifier spell) redirecting onto a single
    action.apply_affecting_aura( hero.mark_of_perotharn ); // 440045
    if ( bugs ) // Mark of Perotharn is being applied twice in what appears to be a bug
      action.apply_affecting_aura( hero.mark_of_perotharn ); // 440045

  }

  // Soul Harvester
  if ( soul_harvester() )
  {
    action.apply_affecting_aura( hero.necrolyte_teachings ); // 449620
    action.apply_affecting_aura( hero.wicked_reaping ); // 449631
    action.apply_affecting_aura( hero.quietus ); // 449634
    action.apply_affecting_aura( hero.sataiels_volition ); // 449637
    action.apply_affecting_aura( tier.inquisitor_sh_4pc ); // 1236416
  }
}

/* ----------------------------------------------------------
* NOTE NOTE NOTE
* Applies PASSIVE (Talents, Spec Auras, Baseline)
* effects to any buff that could be applied to the player
* NOTE NOTE NOTE
------------------------------------------------------------- */
void warlock_t::apply_affecting_auras( buff_t& buff )
{
  // Shared

  // Affliction
  if ( affliction() )
    buff.apply_affecting_aura( warlock_base.affliction_warlock ); // 137043

  // Demonology
  if ( demonology() )
    buff.apply_affecting_aura( warlock_base.demonology_warlock ); // 137044

  // Destruction
  if ( destruction() )
    buff.apply_affecting_aura( warlock_base.destruction_warlock ); // 137046

  // Diabolist

  // Hellcaller

  // Soul Harvester
  if ( soul_harvester() )
    buff.apply_affecting_aura( hero.necrolyte_teachings ); // 449620
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  double actual_amount = player_t::resource_gain( resource_type, amount, source, action );

  // Succulent Soul proc from Demonic Soul talent can only occur from a effective soul shard gain (not overflow)
  if ( resource_type == RESOURCE_SOUL_SHARD && actual_amount > 0.0 && hero.demonic_soul.ok() )
  {
    for ( int i = 0; i < as<int>( actual_amount ); i++ )
    {
      double chance = 0.0;

      if ( affliction() )
        chance = rng_settings.succulent_soul_aff.setting_value;

      if ( demonology() )
        chance = rng_settings.succulent_soul_demo.setting_value;

      if ( rng().roll( chance ) )
      {
        buffs.succulent_soul->trigger();
        procs.succulent_soul->occur();
      }
    }
  }

  return actual_amount;
}

void warlock_t::feast_of_souls_gain()
{
  // TOCHECK: 2025-08-27 The shard gained from Feast of Souls can also proc another Succulent Soul (bug?)
  if ( bugs )
    resource_gain( RESOURCE_SOUL_SHARD, 1.0, gains.feast_of_souls );
  else
    player_t::resource_gain( RESOURCE_SOUL_SHARD, 1.0, gains.feast_of_souls );

  buffs.succulent_soul->trigger();
  procs.succulent_soul->occur();
  procs.feast_of_souls->occur();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t : public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ) : p( player )
  {}

  void html_customsection( report::sc_html_stream& os ) override
  {
    os << R"(<div class="player-section custom_section">)";
    p.parsed_effects_html( os );
    os << "</div>\n";
  }

private:
  warlock_t& p;
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
    darkglares( "darkglare", w ),
    dreadstalkers( "dreadstalker", w ),
    vilefiends( "vilefiend", w ),
    demonic_tyrants( "demonic_tyrant", w ),
    grimoire_felguards( "grimoire_felguard", w ),
    wild_imps( "wild_imp", w ),
    doomguards( "Doomguard", w ),
    greater_dreadstalkers( "greater_dreadstalker", w ),
    shadow_rifts( "shadowy_tear", w ),
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
