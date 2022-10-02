#include "simulationcraft.hpp"

#include "sc_warlock_pets.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
warlock_pet_t::warlock_pet_t( warlock_t* owner, util::string_view pet_name, pet_e pt, bool guardian )
  : pet_t( owner->sim, owner, pet_name, pt, guardian ),
    special_action( nullptr ),
    melee_attack( nullptr ),
    summon_stats( nullptr ),
    buffs()
{
  owner_coeff.ap_from_sp = 0.5;
  owner_coeff.sp_from_sp = 1.0;

  owner_coeff.health = 0.5;

  register_on_arise_callback( this, [ owner ]() { owner->active_pets++; } );
  register_on_demise_callback( this, [ owner ]( const player_t* ) { owner->active_pets--; } );
}

warlock_t* warlock_pet_t::o()
{
  return static_cast<warlock_t*>( owner );
}

const warlock_t* warlock_pet_t::o() const
{
  return static_cast<warlock_t*>( owner );
}

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  // Demonology
  buffs.demonic_strength = make_buff( this, "demonic_strength", find_spell( 267171 ) )
                               ->set_default_value( find_spell( 267171 )->effectN( 2 ).percent() )
                               ->set_cooldown( 0_ms );

  buffs.grimoire_of_service = make_buff( this, "grimoire_of_service", find_spell( 216187 ) )
                                  ->set_default_value( find_spell( 216187 )->effectN( 1 ).percent() );

  buffs.grim_inquisitors_dread_calling = make_buff( this, "grim_inquisitors_dread_calling", find_spell( 337142 ) );

  buffs.demonic_consumption = make_buff( this, "demonic_consumption", find_spell( 267972 ) )
                                  ->set_default_value( find_spell( 267972 )->effectN( 1 ).percent() )
                                  ->set_max_stack( 1 );

  // Destruction
  buffs.embers = make_buff( this, "embers", find_spell( 264364 ) )
                     ->set_period( 500_ms )
                     ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                     ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                       o()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, o()->gains.infernal );
                     } );

  // All Specs
  buffs.demonic_synergy = make_buff( this, "demonic_synergy", find_spell( 337060 ) )
                              ->set_default_value( o()->legendary.relic_of_demonic_synergy->effectN( 1 ).base_value() );
}

void warlock_pet_t::init_base_stats()
{
  pet_t::init_base_stats();

  resources.base[ RESOURCE_ENERGY ]                  = 200;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;

  base.spell_power_per_intellect = 1.0;

  intellect_per_owner = 0;
  stamina_per_owner   = 0;

  main_hand_weapon.type       = WEAPON_BEAST;
  main_hand_weapon.swing_time = 2_s;
}

void warlock_pet_t::init_action_list()
{
  if ( special_action )
  {
    if ( type == PLAYER_PET )
      special_action->background = true;
    else
      special_action->action_list = get_action_priority_list( "default" );
  }

  pet_t::init_action_list();

  if ( summon_stats )
    for ( size_t i = 0; i < action_list.size(); ++i )
      summon_stats->add_child( action_list[ i ]->stats );
}

void warlock_pet_t::init_special_effects()
{
  pet_t::init_special_effects();

  if ( o()->legendary.relic_of_demonic_synergy->ok() && is_main_pet )
  {
    auto const syn_effect = new special_effect_t( this );
    syn_effect->name_str = "demonic_synergy_pet_effect";
    syn_effect->spell_id = 337057;
    syn_effect->custom_buff = o()->buffs.demonic_synergy;
    special_effects.push_back( syn_effect );

    auto cb = new dbc_proc_callback_t( this, *syn_effect );

    cb->initialize();
  }
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && !melee_attack->execute_event &&
       !( special_action && ( d = special_action->get_dot() ) && d->is_ticking() ) )
  {
    melee_attack->schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

/*
Felguard had a Haste scaling energy bug that was supposedly fixed once already. Real fix apparently went live
2019-03-12. Preserving code from resource regen override for now in case of future issues. if ( !o()->dbc.ptr && ( pet_type == PET_FELGUARD ||
pet_type == PET_SERVICE_FELGUARD ) ) reg /= cache.spell_haste();
*/

double warlock_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = pet_t::composite_player_multiplier( school );

  m *= 1.0 + buffs.grimoire_of_service->check_value();

  if ( pet_type == PET_FELGUARD && o()->conduit.fel_commando->ok() )
    m *= 1.0 + o()->conduit.fel_commando.percent();

  if ( pet_type == PET_DREADSTALKER && o()->legendary.grim_inquisitors_dread_calling->ok() )
    m *= 1.0 + buffs.grim_inquisitors_dread_calling->check_value();

  m *= 1.0 + buffs.demonic_synergy->check_stack_value();

  return m;
}

double warlock_pet_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = pet_t::composite_player_target_multiplier( target, school );

  if ( o()->specialization() == WARLOCK_DEMONOLOGY && school == SCHOOL_SHADOWFLAME &&
       o()->talents.from_the_shadows->ok() )
  {
    auto td = o()->get_target_data( target );

    // TOCHECK: There is no "affected by" information for pets. Presumably matching school should be a sufficient check.
    // If there's a non-warlock guardian in game that benefits from this... well, good luck with that.
    if ( td->debuffs_from_the_shadows->check() )
      m *= 1.0 + td->debuffs_from_the_shadows->check_value();
  }

  return m;
}

warlock_pet_td_t::warlock_pet_td_t( player_t* target, warlock_pet_t& p ) :
  actor_target_data_t( target, &p ), pet( p )
{
  debuff_infernal_brand = make_buff( *this, "infernal_brand", pet.o()->find_spell( 340045 ) )
                              ->set_default_value( pet.o()->find_conduit_spell( "Infernal Brand" ).percent() );

  debuff_whiplash = make_buff( *this, "whiplash", pet.o()->find_spell( 6360 ) )
                        ->set_default_value( pet.o()->find_spell( 6360 )->effectN( 2 ).percent() )
                        ->set_max_stack( pet.o()->find_spell( 6360 )->max_stacks() - 1 ); // Data erroneously has 11 as the maximum stack
}

namespace pets
{
warlock_simple_pet_t::warlock_simple_pet_t( warlock_t* owner, util::string_view pet_name, pet_e pt )
  : warlock_pet_t( owner, pet_name, pt, true ), special_ability( nullptr )
{
  resource_regeneration = regen_type::DISABLED;
}

timespan_t warlock_simple_pet_t::available() const
{
  if ( !special_ability || !special_ability->cooldown )
  {
    return warlock_pet_t::available();
  }

  timespan_t cd_remains = special_ability->cooldown->ready - sim->current_time();
  if ( cd_remains <= 1_ms )
  {
    return warlock_pet_t::available();
  }

  return cd_remains;
}

namespace base
{

/// Felhunter Begin

felhunter_pet_t::felhunter_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_FELHUNTER, name != "felhunter" )
{
  action_list_str = "shadow_bite";

  is_main_pet = true;
}

struct spell_lock_t : public warlock_pet_spell_t
{
  spell_lock_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Spell Lock", p, p->find_spell( 19647 ) )
  {
    parse_options( options_str );

    may_miss = may_block = may_dodge = may_parry = false;
    ignore_false_positive = is_interrupt = true;
  }
};

void felhunter_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  melee_attack = new warlock_pet_melee_t( this );
  special_action = new spell_lock_t( this, "" );
}

action_t* felhunter_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shadow_bite" )
    return new warlock_pet_melee_attack_t( this, "Shadow Bite" );
  if ( name == "spell_lock" )
    return new spell_lock_t( this, options_str );
  return warlock_pet_t::create_action( name, options_str );
}

/// Felhunter End

/// Imp Begin

imp_pet_t::imp_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_IMP, name != "imp" ), firebolt_cost( find_spell( 3110 )->cost( POWER_ENERGY ) )
{
  action_list_str = "firebolt";

  owner_coeff.ap_from_sp = 0.625;
  owner_coeff.sp_from_sp = 1.25;
  owner_coeff.health = 0.45;

  is_main_pet = true;
}

action_t* imp_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "firebolt" )
    return new warlock_pet_spell_t( "Firebolt", this, this->find_spell( 3110 ) );
  return warlock_pet_t::create_action( name, options_str );
}

timespan_t imp_pet_t::available() const
{
  double deficit = resources.current[ RESOURCE_ENERGY ] - firebolt_cost;

  if ( deficit >= 0 )
  {
    return warlock_pet_t::available();
  }

  double time_to_threshold = std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY );

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
  {
    return warlock_pet_t::available();
  }

  return timespan_t::from_seconds( time_to_threshold );
}

/// Imp End

/// Sayaad Begin

sayaad_pet_t::sayaad_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_SAYAAD, false )
{
  main_hand_weapon.swing_time = 3_s;
  action_list_str             = "whiplash/lash_of_pain";

  is_main_pet = true;
}

void sayaad_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  melee_attack                = new warlock_pet_melee_t( this );
}

struct whiplash_t : public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ) : warlock_pet_spell_t( p, "Whiplash" )
  {
  }

  void impact( action_state_t* s ) override
  {
    warlock_pet_spell_t::impact( s );

    pet_td( s->target )->debuff_whiplash->trigger();
  }
};

double sayaad_pet_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = warlock_pet_t::composite_player_target_multiplier( target, school );

  m *= 1 + get_target_data( target )->debuff_whiplash->check_stack_value();

  return m;
}

action_t* sayaad_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "lash_of_pain" )
    return new warlock_pet_spell_t( this, "Lash of Pain" );
  if ( name == "whiplash" )
    return new whiplash_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Sayaad End

/// Voidwalker Begin

voidwalker_pet_t::voidwalker_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_VOIDWALKER, name != "voidwalker" )
{
  action_list_str = "consuming_shadows";

  is_main_pet = true;
}

struct consuming_shadows_t : public warlock_pet_spell_t
{
  consuming_shadows_t( warlock_pet_t* p ) 
    : warlock_pet_spell_t( p, "Consuming Shadows" )
  {
    aoe = -1;
    may_crit = false;
  }
};

void voidwalker_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;
  owner_coeff.health = 0.7;

  melee_attack = new warlock_pet_melee_t( this );
}

action_t* voidwalker_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "consuming_shadows" )
    return new consuming_shadows_t( this );
  return warlock_pet_t::create_action( name, options_str );
}

/// Voidwalker End

}  // namespace base

namespace demonology
{

/// Felguard Begin

felguard_pet_t::felguard_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_FELGUARD, name != "felguard" ),
    soul_strike( nullptr ),
    demonic_strength_executes( 0 ),
    min_energy_threshold( find_spell( 89751 )->cost( POWER_ENERGY ) ),
    max_energy_threshold( 100 )
{
  action_list_str = "travel";
  action_list_str += "/demonic_strength_felstorm";
  action_list_str += "/felstorm";
  action_list_str += "/legion_strike,if=energy>=" + util::to_string( max_energy_threshold );

  felstorm_cd = get_cooldown( "felstorm" );

  owner_coeff.health = 0.75;

  is_main_pet = true;
}

struct axe_toss_t : public warlock_pet_spell_t
{
  axe_toss_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Axe Toss", p, p->find_spell( 89766 ) )
  {
    parse_options( options_str );

    may_miss = may_block = may_dodge = may_parry = false;
    ignore_false_positive = is_interrupt = true;
  }
};

struct legion_strike_t : public warlock_pet_melee_attack_t
{
  legion_strike_t( warlock_pet_t* p, util::string_view options_str ) 
    : warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    parse_options( options_str );
    aoe    = -1;
    weapon = &( p->main_hand_weapon );
  }
};

struct felstorm_t : public warlock_pet_melee_attack_t
{
  struct felstorm_tick_t : public warlock_pet_melee_attack_t
  {
    felstorm_tick_t( warlock_pet_t* p, const spell_data_t *s )
      : warlock_pet_melee_attack_t( "felstorm_tick", p, s )
    {
      aoe = -1;
      reduced_aoe_targets = data().effectN( 3 ).base_value();
      background = true;
      weapon     = &( p->main_hand_weapon );
    }

    double action_multiplier() const override
    {
      double m = warlock_pet_melee_attack_t::action_multiplier();

      if ( p()->buffs.demonic_strength->check() )
      {
        m *= p()->buffs.demonic_strength->default_value;
      }

      return m;
    }
  };

  felstorm_t( warlock_pet_t* p, util::string_view options_str, const std::string n = "felstorm" )
    : warlock_pet_melee_attack_t( n, p, p->find_spell( 89751 ) )
  {
    parse_options( options_str );
    tick_zero    = true;
    hasted_ticks = true;
    may_miss     = false;
    may_crit     = false;
    channeled    = true;

    dynamic_tick_action = true;
    tick_action         = new felstorm_tick_t( p, p->find_spell( 89753 ));
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s->action->tick_time( s ) * 5.0;
  }
};

struct demonic_strength_t : public felstorm_t
{
  demonic_strength_t( warlock_pet_t* p, util::string_view options_str )
    : felstorm_t( p, options_str, "demonic_strength_felstorm" )
  {
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();
    debug_cast< felguard_pet_t* >( p() )->demonic_strength_executes--;
    p()->melee_attack->cancel();
  }

  void last_tick( dot_t* d ) override
  {
    warlock_pet_melee_attack_t::last_tick( d );

    p()->buffs.demonic_strength->expire();
  }

  bool ready() override
  {
    if ( debug_cast< felguard_pet_t* >( p() )->demonic_strength_executes <= 0 )
      return false;
    return warlock_pet_melee_attack_t::ready();
  }
};

struct soul_strike_t : public warlock_pet_melee_attack_t
{
  soul_strike_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Soul Strike", p, p->find_spell( 267964 ) )
  {
    background = true;
  }
};

timespan_t felguard_pet_t::available() const
{
  double energy_threshold = max_energy_threshold;
  double time_to_felstorm = ( felstorm_cd->ready - sim->current_time() ).total_seconds();
  if ( time_to_felstorm <= 0 )
  {
    energy_threshold = min_energy_threshold;
  }

  double deficit           = resources.current[ RESOURCE_ENERGY ] - energy_threshold;
  double time_to_threshold = 0;
  // Not enough energy, figure out how many milliseconds it'll take to get
  if ( deficit < 0 )
  {
    time_to_threshold = util::ceil( std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY ), 3 );
  }

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
  {
    return warlock_pet_t::available();
  }

  // Next event is either going to be the time to felstorm, or the time to gain enough energy for a
  // threshold value
  double time_to_next_event = 0;
  if ( time_to_felstorm <= 0 )
  {
    time_to_next_event = time_to_threshold;
  }
  else
  {
    time_to_next_event = std::min( time_to_felstorm, time_to_threshold );
  }

  if ( sim->debug )
  {
    sim->out_debug.print( "{} waiting, deficit={}, threshold={}, t_threshold={}, t_felstorm={} t_wait={}", name(),
                          deficit, energy_threshold, time_to_threshold, time_to_felstorm, time_to_next_event );
  }

  if ( time_to_next_event < 0.001 )
  {
    return warlock_pet_t::available();
  }
  else
  {
    return timespan_t::from_seconds( time_to_next_event );
  }
}

void felguard_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  // Felguard is the only warlock pet type to use an actual weapon.
  main_hand_weapon.type = WEAPON_AXE_2H;
  melee_attack          = new warlock_pet_melee_t( this );

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  // TOCHECK Felguard has a hardcoded 10% multiplier for its auto attack damage. Seems to still be in effect as of 2021-12-01
  melee_attack->base_dd_multiplier *= 1.1;
  special_action = new axe_toss_t( this, "" );

  if ( o()->talents.soul_strike )
  {
    soul_strike = new soul_strike_t( this );
  }
}

action_t* felguard_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "legion_strike" )
    return new legion_strike_t( this, options_str );
  if ( name == "felstorm" )
    return new felstorm_t( this, options_str );
  if ( name == "axe_toss" )
    return new axe_toss_t( this, options_str );
  if ( name == "demonic_strength_felstorm" )
    return new demonic_strength_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

void felguard_pet_t::queue_ds_felstorm()
{
  demonic_strength_executes++;

  if ( !readying && !channeling && !executing )
  {
    schedule_ready();
  }
}

/// Felguard End

/// Grimoire: Felguard Begin

grimoire_felguard_pet_t::grimoire_felguard_pet_t( warlock_t* owner )
  : warlock_pet_t( owner, "grimoire_felguard", PET_SERVICE_FELGUARD, true ),
    min_energy_threshold( find_spell( 89751 )->cost( POWER_ENERGY ) ),
    max_energy_threshold( 100 )
{
  action_list_str = "travel";
  action_list_str += "/felstorm";
  action_list_str += "/legion_strike,if=energy>=" + util::to_string( max_energy_threshold );

  felstorm_cd = get_cooldown( "felstorm" );

  owner_coeff.health = 0.75;
}

 void grimoire_felguard_pet_t::arise()
 {
   warlock_pet_t::arise();

   buffs.grimoire_of_service->trigger();
 }

timespan_t grimoire_felguard_pet_t::available() const
{
  double energy_threshold = max_energy_threshold;
  double time_to_felstorm = ( felstorm_cd->ready - sim->current_time() ).total_seconds();
  if ( time_to_felstorm <= 0 )
  {
    energy_threshold = min_energy_threshold;
  }

  double deficit           = resources.current[ RESOURCE_ENERGY ] - energy_threshold;
  double time_to_threshold = 0;
  // Not enough energy, figure out how many milliseconds it'll take to get
  if ( deficit < 0 )
  {
    time_to_threshold = util::ceil( std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY ), 3 );
  }

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
  {
    return warlock_pet_t::available();
  }

  // Next event is either going to be the time to felstorm, or the time to gain enough energy for a
  // threshold value
  double time_to_next_event = 0;
  if ( time_to_felstorm <= 0 )
  {
    time_to_next_event = time_to_threshold;
  }
  else
  {
    time_to_next_event = std::min( time_to_felstorm, time_to_threshold );
  }

  if ( sim->debug )
  {
    sim->out_debug.print( "{} waiting, deficit={}, threshold={}, t_threshold={}, t_felstorm={} t_wait={}", name(),
                          deficit, energy_threshold, time_to_threshold, time_to_felstorm, time_to_next_event );
  }

  if ( time_to_next_event < 0.001 )
  {
    return warlock_pet_t::available();
  }
  else
  {
    return timespan_t::from_seconds( time_to_next_event );
  }
}

void grimoire_felguard_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  // Felguard is the only warlock pet type to use an actual weapon.
  main_hand_weapon.type = WEAPON_AXE_2H;
  melee_attack          = new warlock_pet_melee_t( this );

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  // TOCHECK Grimoire Felguard also has a hardcoded 10% multiplier for its auto attack damage. Seems to still be in effect as of 2021-12-01
  melee_attack->base_dd_multiplier *= 1.1;
}

action_t* grimoire_felguard_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "legion_strike" )
    return new legion_strike_t( this, options_str );
  if ( name == "felstorm" )
    return new felstorm_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

/// Grimoire: Felguard End

/// Wild Imp Begin

wild_imp_pet_t::wild_imp_pet_t( warlock_t* owner )
  : warlock_pet_t( owner, "wild_imp", PET_WILD_IMP ), firebolt( nullptr ), power_siphon( false )
{
  resource_regeneration = regen_type::DISABLED;
  owner_coeff.health    = 0.15;
}

struct fel_firebolt_t : public warlock_pet_spell_t
{
  fel_firebolt_t( warlock_pet_t* p ) : warlock_pet_spell_t( "fel_firebolt", p, p->find_spell( 104318 ) )
  {
    repeating = true;
  }

  void schedule_execute( action_state_t* execute_state ) override
  {
    // We may not be able to execute anything, so reset executing here before we are going to
    // schedule anything else.
    player->executing = nullptr;

    if ( player->buffs.movement->check() || player->buffs.stunned->check() )
      return;

    warlock_pet_spell_t::schedule_execute( execute_state );
  }

  void consume_resource() override
  {
    warlock_pet_spell_t::consume_resource();

    // Imp dies if it cannot cast
    if ( player->resources.current[ RESOURCE_ENERGY ] < cost() )
    {
      make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
    }
  }

  double cost() const override
  {
    double c = warlock_pet_spell_t::cost();

    if ( p()->o()->spec.fel_firebolt_2->ok() )
      c *= 1.0 + p()->o()->spec.fel_firebolt_2->effectN( 1 ).percent();

    if ( p()->o()->buffs.demonic_power->check() )
    {
      // 2022-02-16 - At some point, Wild Imps stopped despawning if Demonic Tyrant is summoned during their final cast
      c *= 1.0 + p()->o()->buffs.demonic_power->data().effectN( 4 ).percent();
    }

    return c;
  }
};

void wild_imp_pet_t::create_actions()
{
  warlock_pet_t::create_actions();

  firebolt = new fel_firebolt_t( this );
}

void wild_imp_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  resources.base[ RESOURCE_ENERGY ]                  = 100;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
}

//TODO: Utilize new execute_on_target
void wild_imp_pet_t::reschedule_firebolt()
{
  if ( executing || is_sleeping() || player_t::buffs.movement->check() || player_t::buffs.stunned->check() )
    return;

  timespan_t gcd_adjust = gcd_ready - sim->current_time();
  if ( gcd_adjust > 0_ms )
  {
    make_event( sim, gcd_adjust, [ this ]() {
      firebolt->set_target( o()->target );
      firebolt->schedule_execute();
    } );
  }
  else
  {
    firebolt->set_target( o()->target );
    firebolt->schedule_execute();
  }
}

void wild_imp_pet_t::schedule_ready( timespan_t /* delta_time */, bool /* waiting */ )
{
  reschedule_firebolt();
}

void wild_imp_pet_t::finish_moving()
{
  warlock_pet_t::finish_moving();

  reschedule_firebolt();
}

void wild_imp_pet_t::arise()
{
  warlock_pet_t::arise();

  power_siphon = false;
  o()->buffs.wild_imps->increment();

  // Start casting fel firebolts
  firebolt->set_target( o()->target );
  firebolt->schedule_execute();
}

void wild_imp_pet_t::demise()
{
  if ( !current.sleeping )
  {
    o()->buffs.wild_imps->decrement();

    if ( !power_siphon )
    {
      o()->buffs.demonic_core->trigger( 1, buff_t::DEFAULT_VALUE(), o()->warlock_base.demonic_core->effectN( 1 ).percent() );
    }

    if ( expiration )
    {
      event_t::cancel( expiration );
    }
  }

  warlock_pet_t::demise();
}

/// Wild Imp End

/// Dreadstalker Begin

dreadstalker_t::dreadstalker_t( warlock_t* owner ) : warlock_pet_t( owner, "dreadstalker", PET_DREADSTALKER )
{
  action_list_str        = "travel/dreadbite";
  resource_regeneration  = regen_type::DISABLED;

  // TOCHECK: This has been adjusted through various hotfixes over several years to the current value
  // Last checked 2021-12-02
  owner_coeff.ap_from_sp = 0.552;

  owner_coeff.health = 0.4;
}

struct dreadbite_t : public warlock_pet_melee_attack_t
{
  dreadbite_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Dreadbite", p, p->find_spell( 205196 ) )
  {
    weapon = &( p->main_hand_weapon );

    if ( p->o()->talents.dreadlash->ok() )
    {
      aoe    = -1;
      radius = 8.0;
    }
  }

  bool ready() override
  {
    if ( debug_cast< dreadstalker_t* >( p() )->dreadbite_executes <= 0 )
      return false;

    return warlock_pet_melee_attack_t::ready();
  }

  double action_multiplier() const override
  {
    double m = warlock_pet_melee_attack_t::action_multiplier();

    if ( p()->o()->talents.dreadlash->ok() )
    {
      m *= 1.0 + p()->o()->talents.dreadlash->effectN( 1 ).percent();
    }

    return m;
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    debug_cast< dreadstalker_t* >( p() )->dreadbite_executes--;
  }

  void impact( action_state_t* s ) override
  {
    warlock_pet_melee_attack_t::impact( s );

    //if ( p()->o()->talents.from_the_shadows->ok() )
    //  this->owner_td( s->target )->debuffs_from_the_shadows->trigger();
  }
};

// SL - Soulbind conduit (Carnivorous Stalkers) handling requires special version of melee attack
struct dreadstalker_melee_t : warlock_pet_melee_t
{
  dreadstalker_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  {  }

  void execute() override
  {
    warlock_pet_melee_t::execute();

    //if ( p()->o()->conduit.carnivorous_stalkers.ok() && rng().roll( p()->o()->conduit.carnivorous_stalkers.percent() ) )
    //{
    //  debug_cast< dreadstalker_t* >( p() )->dreadbite_executes++;
    //  p()->o()->procs.carnivorous_stalkers->occur();
    //  if ( p()->readying )
    //  {
    //    event_t::cancel( p()->readying );
    //    p()->schedule_ready();
    //  }
    //}
  }
};

void dreadstalker_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();
  resources.base[ RESOURCE_ENERGY ]                  = 0;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
  melee_attack                                       = new dreadstalker_melee_t( this, 0.83 ); // TOCHECK: This number may require tweaking if the AP coeff changes
}

void dreadstalker_t::arise()
{
  warlock_pet_t::arise();

  o()->buffs.dreadstalkers->trigger();

  dreadbite_executes = 1;
}

void dreadstalker_t::demise()
{
  if ( !current.sleeping )
  {
    o()->buffs.dreadstalkers->decrement();
    o()->buffs.demonic_core->trigger( 1, buff_t::DEFAULT_VALUE(), o()->warlock_base.demonic_core->effectN( 2 ).percent() );
  }

  warlock_pet_t::demise();
}

timespan_t dreadstalker_t::available() const
{
  // Dreadstalker does not need to wake up to check for something to do after it has travelled and
  // done its dreadbite
  return sim->expected_iteration_time * 2;
}

action_t* dreadstalker_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "dreadbite" )
    return new dreadbite_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Dreadstalker End

/// Vilefiend Begin

vilefiend_t::vilefiend_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "vilefiend", PET_VILEFIEND )
{
  action_list_str = "bile_spit";
  action_list_str += "/travel";
  action_list_str += "/headbutt";

  owner_coeff.ap_from_sp = 0.23;
  owner_coeff.health     = 0.75;

  bile_spit_executes = 1; // Only one Bile Spit per summon
}

struct bile_spit_t : public warlock_pet_spell_t
{
  bile_spit_t( warlock_pet_t* p ) : warlock_pet_spell_t( "bile_spit", p, p->find_spell( 267997 ) )
  {
    tick_may_crit = false;
    hasted_ticks  = false;
  }

  bool ready() override
  {
    if ( debug_cast< vilefiend_t* >( p() )->bile_spit_executes <= 0 )
      return false;

    return warlock_pet_spell_t::ready();
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    debug_cast< vilefiend_t* >( p() )->bile_spit_executes--;
  }
};

struct headbutt_t : public warlock_pet_melee_attack_t
{
  headbutt_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "headbutt", p, p->find_spell( 267999 ) )
  {
    cooldown->duration = timespan_t::from_seconds( 5 );
  }
};

void vilefiend_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();

  melee_attack = new warlock_pet_melee_t( this, 2.0 );
  special_ability = new headbutt_t( this );
}

void vilefiend_t::arise()
{
  warlock_simple_pet_t::arise();

  bile_spit_executes = 1;
}

action_t* vilefiend_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "bile_spit" )
    return new bile_spit_t( this );
  if ( name == "headbutt" )
    return new headbutt_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Vilefiend End

/// Demonic Tyrant Begin

demonic_tyrant_t::demonic_tyrant_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_DEMONIC_TYRANT, name != "demonic_tyrant" )
{
  resource_regeneration = regen_type::DISABLED;
  action_list_str += "/demonfire";
}

struct demonfire_t : public warlock_pet_spell_t
{
  demonfire_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "demonfire", p, p->find_spell( 270481 ) )
  {
    parse_options( options_str );
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = warlock_pet_spell_t::bonus_da( s );

    if ( p()->buffs.demonic_consumption->check() )
    {
      da += p()->buffs.demonic_consumption->check_value();
    }

    return da;
  }
};

action_t* demonic_tyrant_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "demonfire" )
    return new demonfire_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

/// Demonic Tyrant End

namespace random_demons
{
/// Shivarra Begin

shivarra_t::shivarra_t( warlock_t* owner ) : warlock_simple_pet_t( owner, "shivarra", PET_WARLOCK_RANDOM )
{
  action_list_str        = "travel/multi_slash";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct multi_slash_t : public warlock_pet_melee_attack_t
{
  struct multi_slash_damage_t : public warlock_pet_melee_attack_t
  {
    multi_slash_damage_t( warlock_pet_t* p, unsigned slash_num )
      : warlock_pet_melee_attack_t( "multi-slash-" + std::to_string( slash_num ), p, p->find_spell( 272172 ) )
    {
      background              = true;
      attack_power_mod.direct = data().effectN( slash_num ).ap_coeff();
    }
  };

  std::array<multi_slash_damage_t*, 4> slashes;

  multi_slash_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "multi-slash", p, p->find_spell( 272172 ) )
  {
    for ( unsigned i = 0; i < slashes.size(); ++i )
    {
      // Slash number is the spelldata effects number, so increase by 1.
      slashes[ i ] = new multi_slash_damage_t( p, i + 1 );
      add_child( slashes[ i ] );
    }
  }

  void execute() override
  {
    for ( auto& slash : slashes )
    {
      slash->execute();
    }
    cooldown->start( timespan_t::from_millis( rng().range( 7000, 9000 ) ) );
  }
};

void shivarra_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();
  off_hand_weapon = main_hand_weapon;
  melee_attack    = new warlock_pet_melee_t( this, 2.0 );
  special_ability = new multi_slash_t( this );
}

void shivarra_t::arise()
{
  warlock_simple_pet_t::arise();
  special_ability->cooldown->start( timespan_t::from_millis( rng().range( 3500, 5100 ) ) );
}

action_t* shivarra_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "multi_slash" )
    return new multi_slash_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Shivarra End

/// Darkhound Begin

darkhound_t::darkhound_t( warlock_t* owner ) : warlock_simple_pet_t( owner, "darkhound", PET_WARLOCK_RANDOM )
{
  action_list_str        = "travel/fel_bite";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct fel_bite_t : public warlock_pet_melee_attack_t
{
  fel_bite_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "fel_bite", p, p->find_spell( 272435 ) )
  {
  }

  void update_ready( timespan_t /* cd = timespan_t::min() */ ) override
  {
    warlock_pet_melee_attack_t::update_ready( timespan_t::from_millis( rng().range( 4500, 6500 ) ) );
  }
};

void darkhound_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();
  melee_attack = new warlock_pet_melee_t( this, 2.0 );
  special_ability = new fel_bite_t( this );
}

void darkhound_t::arise()
{
  warlock_simple_pet_t::arise();
  special_ability->cooldown->start( timespan_t::from_millis( rng().range( 3000, 5000 ) ) );
}

action_t* darkhound_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "fel_bite" )
    return new fel_bite_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Darkhound End

/// Bilescourge Begin

bilescourge_t::bilescourge_t( warlock_t* owner ) : warlock_simple_pet_t( owner, "bilescourge", PET_WARLOCK_RANDOM )
{
  action_list_str        = "toxic_bile";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

action_t* bilescourge_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "toxic_bile" )
    return new warlock_pet_spell_t( "toxic_bile", this, this->find_spell( 272167 ) );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Bilescourge End

/// Urzul Begin

urzul_t::urzul_t( warlock_t* owner ) : warlock_simple_pet_t( owner, "urzul", PET_WARLOCK_RANDOM )
{
  action_list_str        = "travel/many_faced_bite";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct many_faced_bite_t : public warlock_pet_melee_attack_t
{
  many_faced_bite_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "many_faced_bite", p, p->find_spell( 272439 ) )
  {
  }

  void update_ready( timespan_t /* cd = timespan_t::min() */ ) override
  {
    warlock_pet_melee_attack_t::update_ready( timespan_t::from_millis( rng().range( 4500, 6000 ) ) );
  }
};

void urzul_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();
  melee_attack = new warlock_pet_melee_t( this, 2.0 );
  special_ability = new many_faced_bite_t( this );
}

void urzul_t::arise()
{
  warlock_simple_pet_t::arise();
  special_ability->cooldown->start( timespan_t::from_millis( rng().range( 3500, 4500 ) ) );
}

action_t* urzul_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "many_faced_bite" )
    return new many_faced_bite_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Urzul End

/// Void Terror Begin

void_terror_t::void_terror_t( warlock_t* owner ) : warlock_simple_pet_t( owner, "void_terror", PET_WARLOCK_RANDOM )
{
  action_list_str        = "travel/double_breath";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct double_breath_t : public warlock_pet_spell_t
{
  struct double_breath_damage_t : public warlock_pet_spell_t
  {
    double_breath_damage_t( warlock_pet_t* p, unsigned breath_num )
      : warlock_pet_spell_t( "double_breath-" + std::to_string( breath_num ), p, p->find_spell( 272156 ) )
    {
      attack_power_mod.direct = data().effectN( breath_num ).ap_coeff();
    }
  };

  double_breath_damage_t* breath_1;
  double_breath_damage_t* breath_2;

  double_breath_t( warlock_pet_t* p ) : warlock_pet_spell_t( "double_breath", p, p->find_spell( 272156 ) )
  {
    breath_1 = new double_breath_damage_t( p, 1U );
    breath_2 = new double_breath_damage_t( p, 2U );
    add_child( breath_1 );
    add_child( breath_2 );
  }

  void execute() override
  {
    breath_1->execute();
    breath_2->execute();
    cooldown->start( timespan_t::from_millis( rng().range( 6000, 9000 ) ) );
  }
};

void void_terror_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();
  melee_attack = new warlock_pet_melee_t( this, 2.0 );
  special_ability = new double_breath_t( this );
}

void void_terror_t::arise()
{
  warlock_simple_pet_t::arise();
  special_ability->cooldown->start( timespan_t::from_millis( rng().range( 1800, 5000 ) ) );
}

action_t* void_terror_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "double_breath" )
    return new double_breath_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Void Terror End

/// Wrathguard Begin

wrathguard_t::wrathguard_t( warlock_t* owner ) : warlock_simple_pet_t( owner, "wrathguard", PET_WARLOCK_RANDOM )
{
  action_list_str        = "travel/overhead_assault";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct overhead_assault_t : public warlock_pet_melee_attack_t
{
  overhead_assault_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "overhead_assault", p, p->find_spell( 272432 ) )
  {
  }

  void update_ready( timespan_t /* cd = timespan_t::min() */ ) override
  {
    warlock_pet_melee_attack_t::update_ready( timespan_t::from_millis( rng().range( 4500, 6500 ) ) );
  }
};

void wrathguard_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();
  off_hand_weapon = main_hand_weapon;
  melee_attack    = new warlock_pet_melee_t( this, 2.0 );
  special_ability = new overhead_assault_t( this );
}

void wrathguard_t::arise()
{
  warlock_simple_pet_t::arise();
  special_ability->cooldown->start( timespan_t::from_millis( rng().range( 3000, 5000 ) ) );
}

action_t* wrathguard_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "overhead_assault" )
    return new overhead_assault_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Wrathguard End

/// Vicious Hellhound Begin

vicious_hellhound_t::vicious_hellhound_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "vicious_hellhound", PET_WARLOCK_RANDOM )
{
  action_list_str        = "travel/demon_fangs";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct demon_fangs_t : public warlock_pet_melee_attack_t
{
  demon_fangs_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "demon_fangs", p, p->find_spell( 272013 ) )
  {
  }

  void update_ready( timespan_t /* cd = timespan_t::min() */ ) override
  {
    warlock_pet_melee_attack_t::update_ready( timespan_t::from_millis( rng().range( 4500, 6000 ) ) );
  }
};

void vicious_hellhound_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();

  main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
  melee_attack                = new warlock_pet_melee_t( this, 1.0 );
  special_ability = new demon_fangs_t( this );
}

void vicious_hellhound_t::arise()
{
  warlock_simple_pet_t::arise();
  special_ability->cooldown->start( timespan_t::from_millis( rng().range( 3200, 5100 ) ) );
}

action_t* vicious_hellhound_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "demon_fangs" )
    return new demon_fangs_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Vicious Hellhound End

/// Illidari Satyr Begin

illidari_satyr_t::illidari_satyr_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "illidari_satyr", PET_WARLOCK_RANDOM )
{
  action_list_str        = "travel/shadow_slash";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct shadow_slash_t : public warlock_pet_melee_attack_t
{
  shadow_slash_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "shadow_slash", p, p->find_spell( 272012 ) )
  {
  }

  void update_ready( timespan_t /* cd = timespan_t::min() */ ) override
  {
    warlock_pet_melee_attack_t::update_ready( timespan_t::from_millis( rng().range( 4500, 6100 ) ) );
  }
};

void illidari_satyr_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();
  off_hand_weapon = main_hand_weapon;
  melee_attack    = new warlock_pet_melee_t( this, 1.0 );
  special_ability = new shadow_slash_t( this );
}

void illidari_satyr_t::arise()
{
  warlock_simple_pet_t::arise();
  special_ability->cooldown->start( timespan_t::from_millis( rng().range( 3500, 5000 ) ) );
}

action_t* illidari_satyr_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shadow_slash" )
    return new shadow_slash_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Illidari Satyr End

/// Eyes of Guldan Begin

eyes_of_guldan_t::eyes_of_guldan_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "eye_of_guldan", PET_WARLOCK_RANDOM )
{
  action_list_str        = "eye_of_guldan";
  owner_coeff.ap_from_sp = 0.12;
  owner_coeff.health     = 0.75;
}

struct eye_of_guldan_t : public warlock_pet_spell_t
{
  eye_of_guldan_t( warlock_pet_t* p ) : warlock_pet_spell_t( "eye_of_guldan", p, p->find_spell( 272131 ) )
  {
    hasted_ticks = false;
  }
};

void eyes_of_guldan_t::arise()
{
  warlock_simple_pet_t::arise();
  o()->buffs.eyes_of_guldan->trigger();
}

void eyes_of_guldan_t::demise()
{
  if ( !current.sleeping )
    o()->buffs.eyes_of_guldan->decrement();

  warlock_simple_pet_t::demise();
}

void eyes_of_guldan_t::init_base_stats()
{
  special_ability = new eye_of_guldan_t( this );
}

action_t* eyes_of_guldan_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "eye_of_guldan" )
    return new eye_of_guldan_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

/// Eyes of Guldan End

/// Prince Malchezaar Begin

prince_malchezaar_t::prince_malchezaar_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "prince_malchezaar", PET_WARLOCK_RANDOM )
{
  owner_coeff.ap_from_sp = 1.15;
  owner_coeff.health     = 0.75;
  action_list_str        = "travel";
}

void prince_malchezaar_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();
  off_hand_weapon = main_hand_weapon;
  melee_attack    = new warlock_pet_melee_t( this );
}

void prince_malchezaar_t::arise()
{
  warlock_simple_pet_t::arise();
  o()->buffs.prince_malchezaar->trigger();
}

void prince_malchezaar_t::demise()
{
  if ( !current.sleeping )
    o()->buffs.prince_malchezaar->decrement();

  warlock_simple_pet_t::demise();
}

timespan_t prince_malchezaar_t::available() const
{
  if ( !expiration )
  {
    return warlock_simple_pet_t::available();
  }

  return expiration->remains() + 1_ms;
}

///Prince Malchezaar End

}  // namespace random_demons
}  // namespace demonology

namespace destruction
{

/// Infernal Begin

infernal_t::infernal_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_INFERNAL, name != "infernal" ), immolation( nullptr )
{
  resource_regeneration = regen_type::DISABLED;
}

struct immolation_tick_t : public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p, const spell_data_t* s )
    : warlock_pet_spell_t( "immolation", p, s )
  {
    aoe        = -1;
    background = may_crit = true;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = warlock_pet_spell_t::composite_target_da_multiplier( t );

    if ( pet_td( t )->debuff_infernal_brand->check() )
      m *= 1.0 + pet_td( t )->debuff_infernal_brand->check_stack_value();

    return m;
  }
};

struct infernal_melee_t : warlock_pet_melee_t
{
  infernal_melee_t(warlock_pet_t* p, double wm, const char* name = "melee") :
    warlock_pet_melee_t (p, wm, name)
  {  }

  void impact( action_state_t* s ) override
  {
    warlock_pet_melee_t::impact( s );

    if ( p()->o()->conduit.infernal_brand.ok() )
    {
      pet_td( s->target )->debuff_infernal_brand->trigger();
    }
  }
};

void infernal_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  melee_attack = new infernal_melee_t( this, 2.0 );
}

void infernal_t::create_buffs()
{
  warlock_pet_t::create_buffs();

  auto damage = new immolation_tick_t( this, find_spell( 19483 )->effectN( 1 ).trigger() );

  immolation =
      make_buff<buff_t>( this, "immolation", find_spell( 19483 ) )
          ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
          ->set_tick_callback( [ damage, this ]( buff_t* /* b  */, int /* stacks */, timespan_t /* tick_time */ ) {
            damage->set_target( target );
            damage->execute();
          } );
}

// TODO: utilize new execute_on_target
void infernal_t::arise()
{
  warlock_pet_t::arise();

  // 2022-06-28 Testing indicates there is a ~1.6 second delay after spawn before first melee
  // Embers looks to trigger at around the same time as first melee swing, but Immolation takes another minimum GCD to apply (and has no zero-tick)
  // Additionally, there is some unknown amount of movement adjustment the pet can take, so we model this with a distribution
  timespan_t delay = timespan_t::from_seconds( rng().gauss( 1.6, 0.2, true ) );

  make_event( *sim, delay, [ this ] {
    buffs.embers->trigger();

    melee_attack->set_target( target );
    melee_attack->schedule_execute();
  } );

  make_event( *sim, delay + 750_ms, [ this ] {
    immolation->trigger();
  } );
}

void infernal_t::demise()
{
  if ( !current.sleeping )
  {
    buffs.embers->expire();
    immolation->expire();
  }

  warlock_pet_t::demise();
}

/// Infernal End

/// Blasphemy Begin
blasphemy_t::blasphemy_t( warlock_t* owner, util::string_view name )
  : infernal_t( owner, name )
{
}

struct blasphemous_existence_t : public warlock_pet_spell_t
{
  blasphemous_existence_t( warlock_pet_t* p ) : warlock_pet_spell_t( "blasphemous_existence", p, p->find_spell( 367819 ) )
  {
    aoe = -1;
    background = true;
  }
};

void blasphemy_t::init_base_stats()
{
  infernal_t::init_base_stats();

  blasphemous_existence = new blasphemous_existence_t( this );
}

/// Blasphemy End

}  // namespace destruction

namespace affliction
{

/// Darkglare Begin

darkglare_t::darkglare_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_DARKGLARE, name != "darkglare" )
{
  action_list_str += "eye_beam";
}

struct eye_beam_t : public warlock_pet_spell_t
{
  struct grim_reach_t : public warlock_pet_spell_t
  {
    grim_reach_t( warlock_pet_t* p ) : warlock_pet_spell_t( "grim_reach", p, p->find_spell( 390097 ) )
    {
      background = dual = true;

      base_dd_min = base_dd_max = 0.0;
    }
  };
  
  grim_reach_t* grim_reach;
  eye_beam_t( warlock_pet_t* p ) : warlock_pet_spell_t( "eye_beam", p, p->find_spell( 205231 ) )
  {
    grim_reach = new grim_reach_t( p );
  }

  double action_multiplier() const override
  {
    double m = warlock_pet_spell_t::action_multiplier();

    double dots = 0.0;

    for ( player_t* target : sim->target_non_sleeping_list )
    {
      dots += p()->o()->get_target_data( target )->count_affliction_dots();
    }

    double dot_multiplier = p()->o()->talents.summon_darkglare->effectN( 3 ).percent();

    if ( p()->o()->talents.malevolent_visionary.ok() )
      dot_multiplier += p()->o()->talents.malevolent_visionary->effectN( 1 ).percent();

    m *= 1.0 + ( dots * dot_multiplier );

    return m;
  }
  
  bool ready() override
  {
    for ( player_t* target : sim->target_non_sleeping_list )
    {
      if ( p()->o()->get_target_data( target )->count_affliction_dots() > 0 )
        return warlock_pet_spell_t::ready();
    }

    return false;
  }

  void impact( action_state_t* s ) override
  {
    auto raw_damage = s->result_raw;

    warlock_pet_spell_t::impact( s );

    if ( p()->o()->talents.grim_reach.ok() )
    {
      grim_reach->base_dd_min = grim_reach->base_dd_max = raw_damage * p()->o()->talents.grim_reach->effectN( 1 ).percent();
      for ( player_t* target : sim->target_non_sleeping_list )
      {
        if ( target == s->target )
          continue;

        if ( p()->o()->get_target_data( target )->count_affliction_dots() > 0 )
        {
          grim_reach->execute_on_target( target );
        }
      }
    }
  }
};

action_t* darkglare_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "eye_beam" )
    return new eye_beam_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Darkglare End

}  // namespace affliction
}  // namespace pets
}  // namespace warlock
