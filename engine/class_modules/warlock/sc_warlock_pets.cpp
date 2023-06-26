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
  use_delayed_pet_stat_updates = owner->use_pet_stat_update_delay;
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

void warlock_pet_t::apply_affecting_auras( action_t& action )
{
  pet_t::apply_affecting_auras( action );

  action.apply_affecting_aura( o()->talents.socrethars_guile );
}

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  // Demonology
  buffs.demonic_strength = make_buff( this, "demonic_strength", o()->talents.demonic_strength )
                               ->set_default_value( o()->talents.demonic_strength->effectN( 2 ).percent() )
                               ->set_cooldown( 0_ms );

  buffs.grimoire_of_service = make_buff( this, "grimoire_of_service", find_spell( 216187 ) )
                                  ->set_default_value( find_spell( 216187 )->effectN( 1 ).percent() );

  buffs.annihilan_training = make_buff( this, "annihilan_training", o()->talents.annihilan_training_buff )
                                 ->set_default_value( o()->talents.annihilan_training_buff->effectN( 1 ).percent() );

  buffs.dread_calling = make_buff( this, "dread_calling", find_spell( 387392 ) );

  buffs.imp_gang_boss = make_buff( this, "imp_gang_boss", find_spell( 387458 ) )
                            ->set_default_value_from_effect( 2 );

  buffs.antoran_armaments = make_buff( this, "antoran_armaments", find_spell( 387496 ) )
                                ->set_default_value( o()->talents.antoran_armaments->effectN( 1 ).percent() );

  buffs.the_expendables = make_buff( this, "the_expendables", find_spell( 387601 ) )
                              ->set_default_value_from_effect( 1 );

  buffs.infernal_command = make_buff( this, "infernal_command", find_spell( 387552 ) )
                               ->set_default_value( o()->talents.infernal_command->effectN( 1 ).percent() );

  buffs.soul_glutton = make_buff( this, "soul_glutton", o()->talents.soul_glutton )
                           ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                           ->set_default_value( o()->talents.soul_glutton->effectN( 2 ).percent() );

  buffs.demonic_servitude = make_buff( this, "demonic_servitude" );

  buffs.fiendish_wrath = make_buff( this, "fiendish_wrath", find_spell( 386601 ) )
                             ->set_default_value_from_effect( 1 );

  buffs.festering_hatred = make_buff( this, "festering_hatred" )
                               ->set_max_stack( std::max( 1, as<int>( o()->talents.immutable_hatred->effectN( 2 ).base_value() ) ) )
                               ->set_stack_change_callback( [ this ]( buff_t* b, int, int cur )
                                    {
                                      if ( cur == b->max_stack() )
                                      {
                                        make_event( sim, 0_ms, [ this, b ] { 
                                          auto fg = debug_cast<pets::demonology::felguard_pet_t*>( this );
                                          fg->immutable_hatred.proc->execute_on_target( fg->immutable_hatred.target );
                                          b->expire();
                                        } );
                                      }
                                    } );

  // Destruction
  buffs.embers = make_buff( this, "embers", find_spell( 264364 ) )
                     ->set_period( 500_ms )
                     ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                     ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                       o()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, o()->gains.infernal );
                     } );

  // All Specs
  buffs.demonic_synergy = make_buff( this, "demonic_synergy",  o()->talents.demonic_synergy )
                              ->set_default_value( o()->talents.grimoire_of_synergy->effectN( 2 ).percent() );

  buffs.fury_of_ruvaraad = make_buff( this, "fury_of_ruvaraad", find_spell( 409708 ) )
                               ->set_default_value_from_effect( 1 );

  // To avoid clogging the buff reports, we silence the pet movement statistics since Implosion uses them regularly
  // and there are a LOT of Wild Imps. We can instead lump them into a single tracking buff on the owner.
  player_t::buffs.movement->quiet = true;
  assert( !player_t::buffs.movement->stack_change_callback );
  player_t::buffs.movement->set_stack_change_callback( [ this ]( buff_t*, int prev, int cur )
                            {
                              if ( cur > prev )
                              {
                                o()->buffs.pet_movement->trigger();
                              }
                              else if ( cur < prev )
                              {
                                o()->buffs.pet_movement->decrement();
                              }
                            } );

  // These buffs are needed for operational purposes but serve little to no reporting purpose
  buffs.demonic_strength->quiet = true;
  buffs.grimoire_of_service->quiet = true;
  buffs.annihilan_training->quiet = true;
  buffs.antoran_armaments->quiet = true;
  buffs.festering_hatred->quiet = true;
  buffs.infernal_command->quiet = true;
  buffs.embers->quiet = true;
  buffs.fury_of_ruvaraad->quiet = true;
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

  if ( o()->talents.grimoire_of_synergy->ok() && is_main_pet )
  {
    auto const syn_effect = new special_effect_t( this );
    syn_effect->name_str = "demonic_synergy_pet_effect";
    syn_effect->spell_id = o()->talents.grimoire_of_synergy->id();
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

  if ( pet_type == PET_DREADSTALKER && o()->talents.dread_calling.ok() )
    m *= 1.0 + buffs.dread_calling->check_value();

  if ( buffs.demonic_synergy->check() )
    m *= 1.0 + buffs.demonic_synergy->check_value();

  if ( buffs.the_expendables->check() )
    m *= 1.0 + buffs.the_expendables->check_stack_value();

  if ( buffs.infernal_command->check() )
    m *= 1.0 + buffs.infernal_command->check_value();

  if ( is_main_pet && o()->talents.wrathful_minion->ok() )
    m *= 1.0 + o()->talents.wrathful_minion->effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_spell_haste() const
{
  double m = pet_t::composite_spell_haste();

  if ( o()->talents.demonic_inspiration->ok() )
    m *= 1.0 + o()->talents.demonic_inspiration->effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_spell_speed() const
{
  double m = pet_t::composite_spell_speed();

  if ( o()->talents.demonic_inspiration->ok() )
      m /= 1.0 + o()->talents.demonic_inspiration->effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_melee_speed() const
{
  double m = pet_t::composite_melee_speed();

  if ( o()->talents.demonic_inspiration->ok() )
    m /= 1.0 + o()->talents.demonic_inspiration->effectN( 1 ).percent();

  return m;
}

void warlock_pet_t::arise()
{
  if ( melee_attack )
    melee_attack->reset();

  pet_t::arise();

  if ( o()->talents.reign_of_tyranny.ok() )
  {
    if ( pet_type == PET_WILD_IMP )
    {
      o()->buffs.demonic_servitude->trigger( as<int>( o()->talents.reign_of_tyranny->effectN( 1 ).base_value() ) );
    }
    else if ( pet_type != PET_DEMONIC_TYRANT )
    {
      if ( !( pet_type == PET_PIT_LORD || pet_type == PET_WARLOCK_RANDOM ) )
      {
        o()->buffs.demonic_servitude->trigger( as<int>( o()->talents.reign_of_tyranny->effectN( 2 ).base_value() ) );
      }
    }
  }
}

void warlock_pet_t::demise()
{
  pet_t::demise();

  if ( o()->talents.reign_of_tyranny.ok() )
  {
    if ( pet_type == PET_WILD_IMP )
    {
      o()->buffs.demonic_servitude->decrement( as<int>( o()->talents.reign_of_tyranny->effectN( 1 ).base_value() ) );
    }
    else if ( pet_type != PET_DEMONIC_TYRANT )
    {
      if ( !( pet_type == PET_PIT_LORD || pet_type == PET_WARLOCK_RANDOM ) )
      {
        o()->buffs.demonic_servitude->decrement( as<int>( o()->talents.reign_of_tyranny->effectN( 2 ).base_value() ) );
      }
    }
  }
}

warlock_pet_td_t::warlock_pet_td_t( player_t* target, warlock_pet_t& p ) :
  actor_target_data_t( target, &p ), pet( p )
{
  debuff_infernal_brand = make_buff( *this, "infernal_brand", pet.o()->find_spell( 387476 ) )
                              ->set_default_value( pet.o()->talents.infernal_brand->effectN( 1 ).percent() );

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
    felguard_guillotine( nullptr ),
    demonic_strength_executes( 0 ),
    min_energy_threshold( find_spell( 89751 )->cost( POWER_ENERGY ) ),
    max_energy_threshold( 100 )
{
  action_list_str = "travel";
  action_list_str += "/felstorm_demonic_strength";
  if ( !owner->disable_auto_felstorm )
    action_list_str += "/felstorm";
  action_list_str += "/legion_strike,if=energy>=" + util::to_string( max_energy_threshold );

  felstorm_cd = get_cooldown( "felstorm" );
  dstr_cd = get_cooldown( "felstorm_demonic_strength" );

  owner_coeff.health = 0.75;

  is_main_pet = true;
}

struct felguard_melee_t : public warlock_pet_melee_t
{
  struct fiendish_wrath_t : public warlock_pet_melee_attack_t
  {
    fiendish_wrath_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Fiendish Wrath", p, p->find_spell( 386601 ) )
    {
      background = dual = true;
      aoe = -1;
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      warlock_pet_melee_attack_t::available_targets( tl );

      // Does not hit the main target
      auto it = range::find( tl, target );
      if ( it != tl.end() )
      {
        tl.erase( it );
      }

      return tl.size();
    }
  };

  fiendish_wrath_t* fiendish_wrath;

  felguard_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  {
    fiendish_wrath = new fiendish_wrath_t( p );
    add_child( fiendish_wrath );
  }

  void impact( action_state_t* s ) override
  {
    auto amount = s->result_raw;

    warlock_pet_melee_t::impact( s );

    if ( p()->buffs.fiendish_wrath->check() )
      fiendish_wrath->execute_on_target( s->target, amount );

    if ( p()->o()->talents.immutable_hatred->ok() )
    {
      auto fg = debug_cast<felguard_pet_t*>( p() );
      if ( !( fg->immutable_hatred.target ) )
      {
        fg->immutable_hatred.target = s->target;
      }
      else if ( fg->immutable_hatred.target != s->target )
      {
        fg->immutable_hatred.target = s->target;
        fg->buffs.festering_hatred->expire();
      }

      fg->buffs.festering_hatred->trigger();
    }
  }
};

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
  bool main_pet;

  legion_strike_t( warlock_pet_t* p, util::string_view options_str ) 
    : warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    parse_options( options_str );
    aoe    = -1;
    weapon = &( p->main_hand_weapon );
    main_pet = true;
  }

  legion_strike_t( warlock_pet_t* p, util::string_view options_str, bool is_main_pet )
    : legion_strike_t( p, options_str )
  {
    main_pet = is_main_pet;
  }

  double action_multiplier() const override
  {
    double m = warlock_pet_melee_attack_t::action_multiplier();

    if ( main_pet && p()->o()->talents.fel_and_steel->ok() && p()->o()->min_version_check( VERSION_10_1_5 ) )
      m *= 1.0 + p()->o()->talents.fel_and_steel->effectN( 1 ).percent();

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = warlock_pet_melee_attack_t::composite_da_multiplier( s );

    if ( main_pet && p()->o()->talents.immutable_hatred->ok() && s->n_targets == 1 )
      m *= 1.0 + p()->o()->talents.immutable_hatred->effectN( 1 ).percent();

    return m;
  }
};

struct immutable_hatred_t : public warlock_pet_melee_attack_t
{
  immutable_hatred_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Immutable Hatred", p, p->find_spell( 405681 ) )
  {
    background = dual = true;
    weapon = &( p->main_hand_weapon );
    may_miss = may_block = may_dodge = may_parry = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    debug_cast<felguard_pet_t*>( p() )->immutable_hatred.target = nullptr;
  }
};

struct felstorm_t : public warlock_pet_melee_attack_t
{
  struct felstorm_tick_t : public warlock_pet_melee_attack_t
  {
    bool applies_fel_sunder; // Fel Sunder is applied only by primary pet using Felstorm
    bool fel_and_steel_bonus; // 10.1.5: Fel and Steel now only applies to primary pet

    felstorm_tick_t( warlock_pet_t* p, const spell_data_t *s )
      : warlock_pet_melee_attack_t( "Felstorm (tick)", p, s )
    {
      aoe = -1;
      reduced_aoe_targets = data().effectN( 3 ).base_value();
      background = true;
      weapon = &( p->main_hand_weapon );
      applies_fel_sunder = false;
      fel_and_steel_bonus = false;
    }

    double action_multiplier() const override
    {
      double m = warlock_pet_melee_attack_t::action_multiplier();

      if ( p()->buffs.demonic_strength->check() )
      {
        m *= 1.0 + p()->buffs.demonic_strength->check_value();
      }

      if ( fel_and_steel_bonus )
      {
        m *= 1.0 + p()->o()->talents.fel_and_steel->effectN( p()->o()->min_version_check( VERSION_10_1_5 ) ? 2 : 1 ).percent();
      }

      if ( p()->o()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T29, B2 ) )
        m *= 1.0 + p()->o()->sets->set( WARLOCK_DEMONOLOGY, T29, B2 )->effectN( 1 ).percent();

      return m;
    }

    void impact( action_state_t* s ) override
    {
      warlock_pet_melee_attack_t::impact( s );

      if ( applies_fel_sunder )
        owner_td( s->target )->debuffs_fel_sunder->trigger();
    }
  };

  felstorm_t( warlock_pet_t* p, util::string_view options_str, const std::string n = "Felstorm" )
    : warlock_pet_melee_attack_t( n, p, p->find_spell( 89751 ) )
  {
    parse_options( options_str );
    tick_zero = true;
    hasted_ticks = true;
    may_miss = false;
    may_crit = false;
    channeled = true;

    dynamic_tick_action = true;
    tick_action = new felstorm_tick_t( p, p->find_spell( 89753 ));
  }

  felstorm_t( warlock_pet_t* p, util::string_view options_str, bool main_pet, const std::string n = "Felstorm" )
    : felstorm_t( p, options_str, n )
  {
    if ( main_pet && p->o()->talents.fel_might->ok() )
      cooldown->duration += p->o()->talents.fel_might->effectN( 1 ).time_value();

    if ( main_pet && p->o()->talents.fel_sunder->ok() )
      debug_cast<felstorm_tick_t*>( tick_action )->applies_fel_sunder = true;

    if ( ( main_pet || !p->o()->min_version_check( VERSION_10_1_5 ) ) && p->o()->talents.fel_and_steel->ok() )
      debug_cast<felstorm_tick_t*>( tick_action )->fel_and_steel_bonus = true;

    if ( !main_pet )
      cooldown->duration = 45_s; // 2022-11-11: GFG does not appear to cast a second Felstorm even if the cooldown would come up, so we will pad this value to be longer than the possible duration.

    if ( main_pet )
      internal_cooldown = p->o()->get_cooldown( "felstorm_icd" );

  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s->action->tick_time( s ) * 5.0;
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    // New in 10.0.5 - Hardcoded scripted shared cooldowns while one of Felstorm, Demonic Strength, or Guillotine is active
    if ( internal_cooldown )
    {
      internal_cooldown->start( 5_s * p()->composite_spell_haste() );
    }

    p()->melee_attack->cancel();
  }
};

struct demonic_strength_t : public felstorm_t
{
  demonic_strength_t( warlock_pet_t* p, util::string_view options_str )
    : felstorm_t( p, options_str, std::string( "Felstorm (Demonic Strength)" ) )
  {
    if ( p->o()->talents.fel_sunder->ok() )
      debug_cast<felstorm_tick_t*>( tick_action )->applies_fel_sunder = true;
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

  // 2022-10-03 - Triggering Demonic Strength seems to ignore energy cost for Felstorm
  double cost() const override
  {
    return 0.0;
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
  struct soul_cleave_t : public warlock_pet_melee_attack_t
  {
    soul_cleave_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Soul Cleave", p, p->find_spell( 387502 ) )
    {
      background = dual = true;
      aoe = -1;
      base_multiplier *= p->o()->talents.antoran_armaments->effectN( 2 ).percent(); // Can only proc Soul Cleave with this talent, so this should be fine
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      warlock_pet_melee_attack_t::available_targets( tl );

      // Does not hit the main target
      auto it = range::find( tl, target );
      if ( it != tl.end() )
      {
        tl.erase( it );
      }

      return tl.size();
    }
  };

  soul_cleave_t* soul_cleave;

  soul_strike_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Soul Strike", p, p->find_spell( 267964 ) )
  {
    background = true;

    soul_cleave = new soul_cleave_t( p );
    add_child( soul_cleave );
  }

  void impact( action_state_t* s ) override
  {
    auto amount = s->result_raw;

    warlock_pet_melee_attack_t::impact( s );
    
    if ( p()->o()->talents.antoran_armaments->ok() )
      soul_cleave->execute_on_target( s->target, amount );
  }
};

struct fel_explosion_t : public warlock_pet_spell_t
{
  fel_explosion_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Fel Explosion", p, p->find_spell( 386609 ) )
  {
    background = dual = true;
    callbacks = false;
    aoe = -1;
  }
};

struct felguard_guillotine_t : public warlock_pet_spell_t
{
  fel_explosion_t* fel_explosion;

  felguard_guillotine_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Guillotine", p, p->find_spell( 386542 ) )
  {
    background = true;
    may_miss = may_crit = false;
    base_tick_time = 1_s;

    fel_explosion = new fel_explosion_t( p );
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(),
                                ground_aoe_params_t()
                                    .target( execute_state->target )
                                    .x( execute_state->target->x_position )
                                    .y( execute_state->target->y_position )
                                    .pulse_time( base_tick_time )
                                    .duration( data().duration() )
                                    .start_time( sim->current_time() )
                                    .action( fel_explosion ) );
  }
};

timespan_t felguard_pet_t::available() const
{
  double energy_threshold = max_energy_threshold;
  double time_to_felstorm = ( felstorm_cd->ready - sim->current_time() ).total_seconds();
  double time_to_threshold = 0;
  double time_to_next_event = 0;

  if ( time_to_felstorm <= 0 )
  {
    energy_threshold = min_energy_threshold;
  }

  double deficit = resources.current[ RESOURCE_ENERGY ] - energy_threshold;

  // Not enough energy, figure out how many milliseconds it'll take to get
  if ( deficit < 0 )
  {
    time_to_threshold = util::ceil( std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY ), 3 );
  }

  // Demonic Strength Felstorms do not have an energy requirement, so Felguard must be ready at any time it is up
  // If Demonic Strength will be available before regular Felstorm but before energy threshold, we will check at that time
  if ( o()->talents.demonic_strength->ok() )
  {
    double time_to_dstr = ( dstr_cd->ready - sim->current_time() ).total_seconds();
    if ( time_to_dstr <= 0 )
    {
      return warlock_pet_t::available();
    }
    else if ( time_to_dstr <= time_to_felstorm && time_to_dstr <= time_to_threshold )
    {
      if ( time_to_dstr < 0.001 )
        return warlock_pet_t::available();
      else
        return timespan_t::from_seconds( time_to_dstr );
    }
  }

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
  {
    return warlock_pet_t::available();
  }

  // Next event is either going to be the time to felstorm, or the time to gain enough energy for a
  // threshold value

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
  melee_attack = new felguard_melee_t( this, 1.0, "melee" );

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  // TOCHECK Felguard has a hardcoded 10% multiplier for its auto attack damage. Seems to still be in effect as of 2022-10-02
  melee_attack->base_dd_multiplier *= 1.1;
  // 2023-03-19 Last minute hotfix for 10.0.7
  melee_attack->base_dd_multiplier *= 1.2;

  special_action = new axe_toss_t( this, "" );

  if ( o()->talents.soul_strike->ok() )
  {
    soul_strike = new soul_strike_t( this );
  }

  if ( o()->talents.guillotine->ok() )
  {
    felguard_guillotine = new felguard_guillotine_t( this );
  }

  if ( o()->talents.immutable_hatred->ok() )
  {
    immutable_hatred.proc = new immutable_hatred_t( this );
    immutable_hatred.target = nullptr;
  }
}

action_t* felguard_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "legion_strike" )
    return new legion_strike_t( this, options_str );
  if ( name == "felstorm" )
    return new felstorm_t( this, options_str, true );
  if ( name == "axe_toss" )
    return new axe_toss_t( this, options_str );
  if ( name == "felstorm_demonic_strength" )
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

void felguard_pet_t::arise()
{
  warlock_pet_t::arise();

  if ( o()->talents.annihilan_training->ok() )
    buffs.annihilan_training->trigger();

  if ( o()->talents.antoran_armaments->ok() )
    buffs.antoran_armaments->trigger();
}

double felguard_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  if ( o()->talents.annihilan_training->ok() )
    m *= 1.0 + buffs.annihilan_training->check_value();

  if ( o()->talents.antoran_armaments->ok() )
    m *= 1.0 + buffs.antoran_armaments->check_value();

  return m;
}

double felguard_pet_t::composite_melee_speed() const
{
  double m = warlock_pet_t::composite_melee_speed();

  m /= 1.0 + buffs.fiendish_wrath->check_value();

  return m;
}

double felguard_pet_t::composite_melee_crit_chance() const
{
  double m = warlock_pet_t::composite_melee_crit_chance();

  if ( o()->talents.heavy_handed->ok() )
    m += o()->talents.heavy_handed->effectN( 1 ).percent();

  return m;
}

double felguard_pet_t::composite_spell_crit_chance() const
{
  double m = warlock_pet_t::composite_spell_crit_chance();

  if ( o()->talents.heavy_handed->ok() )
    m += o()->talents.heavy_handed->effectN( 1 ).percent();

  return m;
}

double felguard_pet_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = warlock_pet_t::composite_player_critical_damage_multiplier( s );

  if ( o()->talents.cavitation->ok() )
    m *= 1.0 + o()->talents.cavitation->effectN( 1 ).percent();

  return m;
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

   if ( o()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T30, B4 ) )
   {
     buffs.fury_of_ruvaraad->trigger();
     o()->buffs.rite_of_ruvaraad->trigger();
   }
 }

 // TODO: Grimoire: Felguard only does a single Felstorm at most, rendering some of this unnecessary
timespan_t grimoire_felguard_pet_t::available() const
{
  double energy_threshold = max_energy_threshold;
  double time_to_felstorm = ( felstorm_cd->ready - sim->current_time() ).total_seconds();
  if ( time_to_felstorm <= 0 )
  {
    energy_threshold = min_energy_threshold;
  }

  double deficit = resources.current[ RESOURCE_ENERGY ] - energy_threshold;
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
  melee_attack = new warlock_pet_melee_t( this );

  owner_coeff.ap_from_sp = 0.65;
  owner_coeff.sp_from_sp = 1.3;

  // TOCHECK Grimoire Felguard also has a hardcoded 10% multiplier for its auto attack damage. Seems to still be in effect as of 2021-12-01
  melee_attack->base_dd_multiplier *= 1.1;
}

action_t* grimoire_felguard_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "legion_strike" )
    return new legion_strike_t( this, options_str, false );
  if ( name == "felstorm" )
    return new felstorm_t( this, options_str, false );

  return warlock_pet_t::create_action( name, options_str );
}

double grimoire_felguard_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  if ( buffs.fury_of_ruvaraad->check() )
    m *= 1.0 + buffs.fury_of_ruvaraad->check_value();

  return m;
}

/// Grimoire: Felguard End

/// Wild Imp Begin

wild_imp_pet_t::wild_imp_pet_t( warlock_t* owner )
  : warlock_pet_t( owner, "wild_imp", PET_WILD_IMP ), firebolt( nullptr ), power_siphon( false ), imploded( false )
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

    if ( p()->o()->warlock_base.fel_firebolt_2->ok() )
      c *= 1.0 + p()->o()->warlock_base.fel_firebolt_2->effectN( 1 ).percent();

    if ( p()->o()->buffs.demonic_power->check() )
    {
      // 2022-02-16 - At some point, Wild Imps stopped despawning if Demonic Tyrant is summoned during their final cast
      c *= 1.0 + p()->o()->talents.demonic_power_buff->effectN( 4 ).percent();
    }

    return c;
  }

  double composite_crit_chance() const override
  {
    double m = warlock_pet_spell_t::composite_crit_chance();

    if ( p()->o()->talents.imperator->ok() )
      m += p()->o()->talents.imperator->effectN( 1 ).percent();

    return m;
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    if ( p()->o()->talents.stolen_power.ok() )
      p()->o()->buffs.stolen_power_building->trigger();
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
  imploded = false;
  o()->buffs.wild_imps->trigger();

  if ( o()->talents.imp_gang_boss.ok() && rng().roll( o()->talents.imp_gang_boss->effectN( 1 ).percent() ) )
  { 
    buffs.imp_gang_boss->trigger();
    o()->procs.imp_gang_boss->occur();
  }

  // TODO: Should we handle cases where the Felguard is summoned while pets are already active?
  if ( o()->talents.infernal_command.ok() && o()->warlock_pet_list.active && o()->warlock_pet_list.active->pet_type == PET_FELGUARD )
  {
    buffs.infernal_command->trigger();
  }

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
      double core_chance = o()->warlock_base.demonic_core->effectN( 1 ).percent();

      if ( o()->talents.bloodbound_imps.ok() )
        core_chance += o()->talents.bloodbound_imps->effectN( imploded ? 2 : 1 ).percent();

      o()->buffs.demonic_core->trigger( 1, buff_t::DEFAULT_VALUE(), core_chance );

      if ( imploded && buffs.imp_gang_boss->check() )
      {
        make_event( sim, 0_ms, [ this ] { this->o()->warlock_pet_list.wild_imps.spawn(); } );
      }
    }

    if ( expiration )
    {
      event_t::cancel( expiration );
    }

    if ( o()->talents.the_expendables.ok() )
      o()->expendables_trigger_helper( this );
  }

  warlock_pet_t::demise();
}

double wild_imp_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  if ( buffs.imp_gang_boss->check() )
    m *= 1.0 + buffs.imp_gang_boss->check_value();

  return m;
}

/// Wild Imp End

/// Dreadstalker Begin

dreadstalker_t::dreadstalker_t( warlock_t* owner ) : warlock_pet_t( owner, "dreadstalker", PET_DREADSTALKER )
{
  action_list_str = "travel/dreadbite";
  resource_regeneration  = regen_type::DISABLED;

  // TOCHECK: This has been adjusted through various hotfixes over several years to the current value
  // Last checked 2021-12-02
  owner_coeff.ap_from_sp = 0.55;

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

    if ( p()->o()->talents.fel_and_steel->ok() && !p()->o()->min_version_check( VERSION_10_1_5 ) )
    {
      m *= 1.0 + p()->o()->talents.fel_and_steel->effectN( 1 ).percent();
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

    if ( p()->o()->talents.the_houndmasters_stratagem->ok() )
      owner_td( s->target )->debuffs_the_houndmasters_stratagem->trigger();
  }
};

// Carnivorous Stalkers talent handling requires special version of melee attack
struct dreadstalker_melee_t : warlock_pet_melee_t
{
  dreadstalker_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  {  }

  void execute() override
  {
    warlock_pet_melee_t::execute();

    if ( p()->o()->talents.carnivorous_stalkers->ok() && rng().roll( p()->o()->talents.carnivorous_stalkers->effectN( 1 ).percent() ) )
    {
      debug_cast<dreadstalker_t*>( p() )->dreadbite_executes++;
      p()->o()->procs.carnivorous_stalkers->occur();
      if ( p()->readying )
      {
        event_t::cancel( p()->readying );
        p()->schedule_ready();
      }
    }
  }
};

void dreadstalker_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();
  resources.base[ RESOURCE_ENERGY ] = 0;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
  melee_attack = new dreadstalker_melee_t( this, o()->min_version_check( VERSION_10_1_5 ) ? 0.96 : 0.83 ); // TOCHECK: This number may require tweaking if the AP coeff changes
}

void dreadstalker_t::arise()
{
  warlock_pet_t::arise();

  o()->buffs.dreadstalkers->trigger();

  if ( o()->talents.infernal_command->ok() && o()->warlock_pet_list.active && o()->warlock_pet_list.active->pet_type == PET_FELGUARD )
  {
    buffs.infernal_command->trigger();
  }

  dreadbite_executes = 1;
}

void dreadstalker_t::demise()
{
  if ( !current.sleeping )
  {
    o()->buffs.dreadstalkers->decrement();
    o()->buffs.demonic_core->trigger( 1, buff_t::DEFAULT_VALUE(), o()->warlock_base.demonic_core->effectN( 2 ).percent() );
    
    if ( o()->talents.shadows_bite->ok() )
      o()->buffs.shadows_bite->trigger();
  }

  warlock_pet_t::demise();
}

timespan_t dreadstalker_t::available() const
{
  // Dreadstalker does not need to wake up after it has travelled and done its Dreadbite
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
  bile_spit_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Bile Spit", p, p->find_spell( 267997 ) )
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
  headbutt_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Headbutt", p, p->find_spell( 267999 ) )
  {
    cooldown->duration = 5_s;
  }
};

void vilefiend_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();

  melee_attack = new warlock_pet_melee_t( this, 2.0 );
  melee_attack->base_dd_multiplier *= 1.3; // 2023-03-19 Last minute hotfix for 10.0.7

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
    : warlock_pet_spell_t( "Demonfire", p, p->find_spell( 270481 ) )
  {
    parse_options( options_str );
  }
};

action_t* demonic_tyrant_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "demonfire" )
    return new demonfire_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

void demonic_tyrant_t::arise()
{
  warlock_pet_t::arise();

  if ( o()->talents.reign_of_tyranny->ok() )
  {
    buffs.demonic_servitude->trigger( 1, ( o()->buffs.demonic_servitude->check() + 1 ) * o()->buffs.demonic_servitude->check_value() ); // Demonic Servitude has a permanent extra 1 stack on tracking (last checked 2023-03-17)
  }
}

double demonic_tyrant_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  if ( o()->talents.reign_of_tyranny->ok() )
  {
    m *= 1.0 + buffs.demonic_servitude->check_value();
    m *= 1.0 + o()->talents.reign_of_tyranny->effectN( 4 ).percent();
  }

  return m;
}

/// Demonic Tyrant End

/// Pit Lord Begin

pit_lord_t::pit_lord_t( warlock_t* owner, util::string_view name ) : warlock_pet_t( owner, name, PET_PIT_LORD, name != "pit_lord" )
{
  owner_coeff.ap_from_sp = 1.21;
  owner_coeff.sp_from_sp = 1.21;

  soul_glutton_damage_bonus = owner->talents.soul_glutton->effectN( 1 ).percent();
}

void pit_lord_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  melee_attack = new warlock_pet_melee_t( this, 4.35 );
}

void pit_lord_t::arise()
{
  warlock_pet_t::arise();

  if ( o()->buffs.nether_portal_total->check() )
  {
    buffs.soul_glutton->trigger( o()->buffs.nether_portal_total->current_stack );
    o()->buffs.nether_portal_total->expire();
  }

  melee_attack->set_target( target );
  melee_attack->schedule_execute();
}

double pit_lord_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  if ( buffs.soul_glutton->check() )
  {
    m *= 1.0 + soul_glutton_damage_bonus * buffs.soul_glutton->current_stack;
  }

  return m;
}

double pit_lord_t::composite_melee_speed() const
{
  double m = warlock_pet_t::composite_melee_speed();

  m /= 1.0 + buffs.soul_glutton->check_stack_value();

  return m;
}

/// Pit Lord End

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
  immolation_tick_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Immolation", p, p->find_spell( 20153 ) )
  {
    aoe = -1;
    background = may_crit = true;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = warlock_pet_spell_t::composite_target_da_multiplier( t );

    if ( p()->o()->talents.infernal_brand->ok() )
      m *= 1.0 + pet_td( t )->debuff_infernal_brand->check_stack_value();

    return m;
  }
};

struct infernal_melee_t : warlock_pet_melee_t
{
  infernal_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  {  }

  void impact( action_state_t* s ) override
  {
    warlock_pet_melee_t::impact( s );

    if ( p()->o()->talents.infernal_brand->ok() )
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

  auto damage = new immolation_tick_t( this );

  immolation = make_buff<buff_t>( this, "immolation", find_spell( 19483 ) )
                   ->set_tick_time_behavior( buff_tick_time_behavior::HASTED )
                   ->set_tick_callback( [ damage, this ]( buff_t*, int, timespan_t ) {
                        damage->execute_on_target( target );
                     } );
}

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

/// Infernal End

/// Blasphemy Begin
blasphemy_t::blasphemy_t( warlock_t* owner, util::string_view name )
  : infernal_t( owner, name )
{  }

struct blasphemous_existence_t : public warlock_pet_spell_t
{
  blasphemous_existence_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Blasphemous Existence", p, p->find_spell( 367819 ) )
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
    grim_reach_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Grim Reach", p, p->find_spell( 390097 ) )
    {
      background = dual = true;

      base_dd_min = base_dd_max = 0.0;
    }
  };
  
  grim_reach_t* grim_reach;
  eye_beam_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Eye Beam", p, p->find_spell( 205231 ) )
  {
    grim_reach = new grim_reach_t( p );
  }

  double composite_target_multiplier( player_t* target ) const
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    double dots = p()->o()->get_target_data( target )->count_affliction_dots();

    double dot_multiplier = p()->o()->talents.summon_darkglare->effectN( 3 ).percent();

    if ( p()->o()->talents.malevolent_visionary->ok() )
      dot_multiplier += p()->o()->talents.malevolent_visionary->effectN( 1 ).percent();

    m *= 1.0 + ( dots * dot_multiplier );

    return m;
  }

  void impact( action_state_t* s ) override
  {
    warlock_pet_spell_t::impact( s );

    auto raw_damage = s->result_total;

    if ( p()->o()->talents.grim_reach->ok() )
    {
      grim_reach->base_dd_min = grim_reach->base_dd_max = raw_damage * p()->o()->talents.grim_reach->effectN( 1 ).percent();
      for ( player_t* target : sim->target_non_sleeping_list )
      {
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
