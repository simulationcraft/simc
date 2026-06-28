#include "simulationcraft.hpp"

#include "sc_warlock_pets.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
warlock_pet_t::warlock_pet_t( warlock_t* owner, util::string_view pet_name, pet_e pt, bool guardian )
  : pet_t( owner->sim, owner, pet_name, pt, guardian ),
    affected_by(),
    triggers(),
    special_action( nullptr ),
    melee_attack( nullptr ),
    summon_stats( nullptr ),
    buffs()
{
  owner_coeff.ap_from_sp = 0.5;
  owner_coeff.sp_from_sp = 1.0;
  owner_coeff.health = 0.5;

  affected_by.demonic_brutality = owner->talents.demonic_brutality.ok();
  triggers.hellbent_commander_heartbeat = owner->talents.hellbent_commander.ok();
  triggers.hellbent_commander_arise = owner->talents.hellbent_commander.ok();
  triggers.hellbent_commander_demise = owner->talents.hellbent_commander.ok();

  register_on_arise_callback( this, [ owner ]() { owner->n_active_pets++; } );
  register_on_demise_callback( this, [ owner ]( const player_t* ) { owner->n_active_pets--; } );
}

warlock_t* warlock_pet_t::o()
{ return static_cast<warlock_t*>( owner ); }

const warlock_t* warlock_pet_t::o() const
{ return static_cast<warlock_t*>( owner ); }

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  // Demonology
  buffs.imp_gang_boss = make_buff( actor_pair_t( this, o() ), "imp_gang_boss", o()->talents.imp_gang_boss_buff )
                            ->set_default_value_from_effect( 2 );

  buffs.infernal_command = make_buff( actor_pair_t( this, o() ), "infernal_command", o()->warlock_base.infernal_command_buff )
                               ->set_default_value( o()->warlock_base.infernal_command_buff->effectN( 1 ).percent() );

  buffs.unstable_soul = make_buff( actor_pair_t( this, o() ), "unstable_soul", o()->talents.unstable_soul_buff )
                            ->set_default_value_from_effect( 1 );

  buffs.flametouched = make_buff( this, "ferocity_of_fharg", o()->talents.flametouched_buff );

  buffs.demonic_power = make_buff( this, "demonic_power", o()->talents.demonic_power_buff )
                            ->set_default_value_from_effect( 1 );

  buffs.grimoire_of_service = make_buff( this, "grimoire_of_service", o()->talents.grimoire_of_service_buff )
                                  ->set_default_value_from_effect( 1 );

  // Destruction
  buffs.embers = make_buff( this, "embers", o()->talents.embers )
                     ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                       o()->resource_gain( RESOURCE_SOUL_SHARD, o()->talents.burning_ember->effectN( 1 ).base_value() / 10.0, o()->gains.infernal );
                     } );

  // All Specs


  // To avoid clogging the buff reports, we silence the pet movement statistics since Implosion uses them regularly
  // and there are a LOT of Wild Imps. We can instead lump them into a single tracking buff on the owner.
  player_t::buffs.movement->quiet = true;
  assert( player_t::buffs.movement->stack_change_callback.empty() );
  player_t::buffs.movement->set_stack_change_callback( [ this ]( buff_t*, int prev, int cur )
                            {
                              if ( cur > prev )
                                o()->buffs.pet_movement->trigger();
                              else if ( cur < prev )
                                o()->buffs.pet_movement->decrement();
                            } )
                          ->set_proc_callbacks( false );

  // These buffs are needed for operational purposes but serve little to no reporting purpose
  buffs.imp_gang_boss->quiet = true;
  buffs.infernal_command->quiet = true;
  buffs.unstable_soul->quiet = true;
  buffs.flametouched->quiet = true;
  buffs.grimoire_of_service->quiet = true;
  buffs.embers->quiet = true;
}

void warlock_pet_t::init_base_stats()
{
  pet_t::init_base_stats();

  resources.base[ RESOURCE_ENERGY ] = 200;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;

  base.spell_power_per_intellect = 1.0;

  intellect_per_owner = 0;
  stamina_per_owner   = 0;

  main_hand_weapon.type = WEAPON_BEAST;
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

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && !melee_attack->execute_event &&
       ( melee_on_summon || !debug_cast<pets::warlock_pet_melee_t*>( melee_attack )->first ) &&
       !( special_action && ( d = special_action->get_dot() ) && d->is_ticking() ) )
  {
    melee_attack->schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

// Stuff that are updated at the heartbeat update interval
void warlock_pet_t::heartbeat_update_event()
{
};

/*
Felguard had a Haste scaling energy bug that was supposedly fixed once already. Real fix apparently went live
2019-03-12. Preserving code from resource regen override for now in case of future issues. if ( !o()->dbc.ptr && ( pet_type == PET_FELGUARD ||
pet_type == PET_SERVICE_FELGUARD ) ) reg /= cache.spell_haste();
*/

double warlock_pet_t::composite_melee_haste() const
{
  double m = pet_t::composite_melee_haste();

  if ( buffs.flametouched->check() )
    m *= 1.0 + buffs.flametouched->data().effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_melee_auto_attack_speed() const
{
  double m = pet_t::composite_melee_auto_attack_speed();

  if ( buffs.flametouched->check() )
    m /= 1.0 + buffs.flametouched->data().effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_melee_crit_chance() const
{
  double m = pet_t::composite_melee_crit_chance();

  if ( is_main_pet )
    m *= 1.0 + o()->talents.improved_demonic_tactics->effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_spell_crit_chance() const
{
  double m = pet_t::composite_spell_crit_chance();

  if ( is_main_pet )
    m *= 1.0 + o()->talents.improved_demonic_tactics->effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const
{
  double m = pet_t::composite_player_critical_damage_multiplier( s, school );

  if ( affected_by.demonic_brutality )
  {
    // Currently bugged and giving 260% crit damage instead of 230%. Preserving the
    // option to use the intended value for now in case of future issues.
    if ( !o()->bugs )
      m += o()->talents.demonic_brutality->effectN( 2 ).percent() * 0.5;
    else
      m += o()->talents.demonic_brutality->effectN( 2 ).percent();
  }

  return m;
}

void warlock_pet_t::arise()
{
  if ( melee_attack && melee_on_summon )
    melee_attack->reset();

  pet_t::arise();

  if ( triggers.hellbent_commander_arise )
  {
    o()->buffs.hellbent_commander->trigger();
    assert( ( bugs || o()->buffs.hellbent_commander->check() == o()->active_demon_count() ) && "Incorrent Demon Count for Hellbent Commander" );
  }
}

void warlock_pet_t::demise()
{
  if ( !current.sleeping )
  {
    if ( triggers.hellbent_commander_demise )
    {
      o()->buffs.hellbent_commander->decrement();
    }
  }

  pet_t::demise();

  if ( melee_attack )
    melee_attack->reset();

  assert( ( bugs || !o()->talents.hellbent_commander.ok() || o()->buffs.hellbent_commander->check() == o()->active_demon_count() ) && "Incorrent Demon Count for Hellbent Commander" );
}

 // TODO: Add all pet spells to base warlock data

warlock_pet_td_t::warlock_pet_td_t( player_t* target, warlock_pet_t& p ) :
  actor_target_data_t( target, &p ), pet( p )
{
  debuffs.whiplash = make_buff( *this, "whiplash", pet.o()->find_spell( 6360 ) )
                        ->set_default_value( pet.o()->find_spell( 6360 )->effectN( 2 ).percent() )
                        ->set_max_stack( pet.o()->find_spell( 6360 )->max_stacks() - 1 ); // Data erroneously has 11 as the maximum stack
}

namespace pets
{
warlock_simple_pet_t::warlock_simple_pet_t( warlock_t* owner, util::string_view pet_name, pet_e pt )
  : warlock_pet_t( owner, pet_name, pt, true ), special_ability( nullptr )
{ resource_regeneration = regen_type::DISABLED; }

timespan_t warlock_simple_pet_t::available() const
{
  if ( !special_ability || !special_ability->cooldown )
    return warlock_pet_t::available();

  timespan_t cd_remains = special_ability->cooldown->ready - sim->current_time();

  if ( cd_remains <= 1_ms )
    return warlock_pet_t::available();

  return cd_remains;
}

warlock_pet_spell_t::warlock_pet_spell_t( util::string_view token, warlock_pet_t* p, const spell_data_t* s )
  : base_t( token, p, s ),
    affected_by()
{
  affected_by.xalans_cruelty_effect_6 = p->o()->hero.xalans_cruelty.ok() && data().affected_by_label( p->o()->hero.xalans_cruelty->effectN( 6 ) );
  if ( sim->dbc->wowv() < wowv_t{ 12, 0, 7 } ) // TODO: Effect is missing from 12.0.7 PTR; remove this check once added
    affected_by.xalans_cruelty_effect_9 = p->o()->hero.xalans_cruelty.ok() && data().affected_by_label( p->o()->hero.xalans_cruelty->effectN( 9 ) );

  affected_by.xalans_ferocity_effect_6 = p->o()->hero.xalans_ferocity.ok() && data().affected_by_label( p->o()->hero.xalans_ferocity->effectN( 6 ) );
  affected_by.xalans_ferocity_effect_7 = p->o()->hero.xalans_ferocity.ok() && data().affected_by_label( p->o()->hero.xalans_ferocity->effectN( 7 ) );
}

warlock_pet_spell_t::warlock_pet_spell_t( warlock_pet_t* p, util::string_view n )
  : warlock_pet_spell_t( n, p, p->find_pet_spell( n ) )
{ }

double warlock_pet_spell_t::composite_crit_chance_multiplier() const
{
  double m = base_t::composite_crit_chance_multiplier();

  // Xalan's Cruelty effect #6 (id=1190450) is 'Apply Aura Pet (174)' and requires manual handling
  if ( affected_by.xalans_cruelty_effect_6 )
    m *= 1.0 + p()->o()->hero.xalans_cruelty->effectN( 6 ).percent();

  // Xalan's Cruelty effect #9 (id=1322330) is 'Apply Aura Pet (174)' and requires manual handling
  if ( affected_by.xalans_cruelty_effect_9 )
    m *= 1.0 + p()->o()->hero.xalans_cruelty->effectN( 9 ).percent();

  // Xalan's Ferocity effect #6 (id=1166684) is 'Apply Aura Pet (174)' and requires manual handling
  if ( affected_by.xalans_ferocity_effect_6 )
    m *= 1.0 + p()->o()->hero.xalans_ferocity->effectN( 6 ).percent();

  // Xalan's Ferocity effect #7 (id=1190448) is 'Apply Aura Pet (174)' and requires manual handling
  if ( affected_by.xalans_ferocity_effect_7 )
    m *= 1.0 + p()->o()->hero.xalans_ferocity->effectN( 7 ).percent();

  return m;
}

namespace base
{

/// Felhunter Begin

felhunter_pet_t::felhunter_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_FELHUNTER, false )
{
  npc_id = owner->find_spell( 691 )->effectN( 1 ).misc_value1();

  // NOTE: 2026-04-24 Main pets do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  action_list_str = "travel/shadow_bite";

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
  : warlock_pet_t( owner, name, PET_IMP, false ), firebolt_cost( find_spell( 3110 )->cost( POWER_ENERGY ) )
{
  npc_id = owner->find_spell( 688 )->effectN( 1 ).misc_value1();

  // NOTE: 2026-04-24 Main pets do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

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
    return warlock_pet_t::available();

  double time_to_threshold = std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY );

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
    return warlock_pet_t::available();

  return timespan_t::from_seconds( time_to_threshold );
}

/// Imp End

/// Sayaad Begin

sayaad_pet_t::sayaad_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_SAYAAD, false )
{
  npc_id = owner->find_spell( 366222 )->effectN( 1 ).misc_value1();

  // NOTE: 2026-04-24 Main pets do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  action_list_str = "travel/whiplash/lash_of_pain";

  is_main_pet = true;
}

void sayaad_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  main_hand_weapon.swing_time = 3_s;

  melee_attack = new warlock_pet_melee_t( this );
}

struct whiplash_t : public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ) : warlock_pet_spell_t( p, "Whiplash" )
  { }

  void impact( action_state_t* s ) override
  {
    warlock_pet_spell_t::impact( s );

    pet_td( s->target )->debuffs.whiplash->trigger();
  }
};

double sayaad_pet_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = warlock_pet_t::composite_player_target_multiplier( target, school );

  m *= 1.0 + get_target_data( target )->debuffs.whiplash->check_stack_value();

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
  : warlock_pet_t( owner, name, PET_VOIDWALKER, false )
{
  npc_id = owner->find_spell( 697 )->effectN( 1 ).misc_value1();

  // NOTE: 2026-04-24 Main pets do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  action_list_str = "travel/consuming_shadows";

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
  : warlock_pet_t( owner, name, PET_FELGUARD, false ),
    min_energy_threshold( find_spell( 89751 )->cost( POWER_ENERGY ) ),
    max_energy_threshold( 100 )
{
  npc_id = owner->talents.summon_felguard->effectN( 1 ).misc_value1();

  // NOTE: 2026-04-24 Main pets do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  action_list_str = "travel";

  if ( !owner->disable_auto_felstorm )
    action_list_str += "/felstorm";
  action_list_str += "/legion_strike,if=energy>=" + util::to_string( max_energy_threshold );

  felstorm_cd = get_cooldown( "felstorm" );

  is_main_pet = true;
}

struct felguard_melee_t : public warlock_pet_melee_t
{
  felguard_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  { }
};

struct axe_toss_t : public warlock_pet_spell_t
{
  axe_toss_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Axe Toss", p, p->find_spell( 89766 ) )
  {
    parse_options( options_str );

    may_miss = may_block = may_dodge = may_parry = false;
    ignore_false_positive = is_interrupt = true;
    usable_while_casting = true;
  }
};

struct legion_strike_t : public warlock_pet_melee_attack_t
{
  legion_strike_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    parse_options( options_str );
    aoe = -1;
    weapon = &( p->main_hand_weapon );
  }
};

struct felstorm_t : public warlock_pet_melee_attack_t
{
  struct felstorm_tick_t : public warlock_pet_melee_attack_t
  {
    felstorm_tick_t( warlock_pet_t* p, const spell_data_t *s )
      : warlock_pet_melee_attack_t( "Felstorm (tick)", p, s )
    {
      aoe = -1;
      reduced_aoe_targets = data().effectN( 3 ).base_value();
      background = true;
      weapon = &( p->main_hand_weapon );
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

    tick_action = new felstorm_tick_t( p, p->find_spell( 89753 ) );
  }

  // NOTE: Although Felstorm is a "melee" attack, its duration/ticks depend on the pet's 'spell_cast_speed'
  double composite_haste() const override
  { return action_t::composite_haste() * player->cache.spell_cast_speed(); }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  { return s->action->tick_time( s ) * data().duration().total_seconds(); }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    p()->melee_attack->cancel();
  }
};

timespan_t felguard_pet_t::available() const
{
  double energy_threshold = max_energy_threshold;
  double time_to_felstorm = ( felstorm_cd->ready - sim->current_time() ).total_seconds();
  double time_to_threshold = 0;
  double time_to_next_event = 0;

  if ( time_to_felstorm <= 0 )
    energy_threshold = min_energy_threshold;

  double deficit = resources.current[ RESOURCE_ENERGY ] - energy_threshold;

  // Not enough energy, figure out how many milliseconds it'll take to get
  if ( deficit < 0 )
    time_to_threshold = util::ceil( std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY ), 3 );

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
    return warlock_pet_t::available();

  // Next event is either going to be the time to felstorm, or the time to gain enough energy for a
  // threshold value

  if ( time_to_felstorm <= 0 )
    time_to_next_event = time_to_threshold;
  else
    time_to_next_event = std::min( time_to_felstorm, time_to_threshold );

  if ( sim->debug )
  {
    sim->out_debug.print( "{} waiting, deficit={}, threshold={}, t_threshold={}, t_felstorm={} t_wait={}", name(),
                          deficit, energy_threshold, time_to_threshold, time_to_felstorm, time_to_next_event );
  }

  if ( time_to_next_event < 0.001 )
    return warlock_pet_t::available();
  else
    return timespan_t::from_seconds( time_to_next_event );
}

void felguard_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  // Felguard is the only warlock pet type to use an actual weapon.
  main_hand_weapon.type = WEAPON_AXE_2H;
  melee_attack = new felguard_melee_t( this, 1.0, "melee" );

  // 2026-02-17: Validated coefficients
  owner_coeff.ap_from_sp = 1.5201;
  owner_coeff.sp_from_sp = 1.4519; // not validated
  owner_coeff.health = 0.75;

  melee_attack->base_dd_multiplier *= 1.44;

  special_action = new axe_toss_t( this, "" );
}

action_t* felguard_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "legion_strike" )
    return new legion_strike_t( this, options_str );
  if ( name == "felstorm" )
    return new felstorm_t( this, options_str );
  if ( name == "axe_toss" )
    return new axe_toss_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

/// Felguard End

/// Wild Imp Begin

wild_imp_pet_t::wild_imp_pet_t( warlock_t* owner )
  : warlock_pet_t( owner, "wild_imp", PET_WILD_IMP, true ), firebolt( nullptr ), is_hog_imp( true ), power_siphon( false ), imploded( false )
{
  npc_id = owner->warlock_base.wild_imp->effectN( 1 ).misc_value1();

  // Manually handle the Wild Imps contribution to Hellbent Commander on demise to replicate its bugged behavior
  triggers.hellbent_commander_demise &= !bugs;

  resource_regeneration = regen_type::DISABLED;
  owner_coeff.health = 0.15;

  // Each Wild Imp uses its own independent accumulator PRD, reset to 0 on arise
  if ( owner->talents.infernal_rapidity.ok() )
    prd_rng_infernal_rapidity = get_accumulated_rng( "infernal_rapidity_i" + std::to_string( actor_index ), owner->prd_rng.infernal_rapidity_prd_c_value );
}

struct fel_firebolt_t : public warlock_pet_spell_t
{
  fel_firebolt_t* twin = nullptr;
  const bool is_twin;
  mutable bool target_cache_unstable_soul_state = false;

  fel_firebolt_t( warlock_pet_t* p, bool _is_twin = false )
    : warlock_pet_spell_t( "fel_firebolt", p, p->find_spell( 104318 ) ),
    is_twin( _is_twin )
  {
    aoe = 0;
    if ( !is_twin )
    {
      repeating = true;

      if ( p->o()->talents.infernal_rapidity.ok() )
        twin = new fel_firebolt_t( p, true );
    }
    else
    {
      background = dual = proc = true;
      base_costs[ RESOURCE_ENERGY ] = 0;
      base_dd_multiplier *= p->o()->talents.infernal_rapidity->effectN( 2 ).percent();
    }
  }

  void schedule_execute( action_state_t* execute_state ) override
  {
    // We may not be able to execute anything, so reset executing here before we are going to
    // schedule anything else.
    if ( !is_twin )
    {
      player->executing = nullptr;

      if ( player->buffs.movement->check() || player->buffs.stunned->check() )
        return;
    }

    warlock_pet_spell_t::schedule_execute( execute_state );
  }

  int n_targets() const override
  {
    if ( p()->buffs.unstable_soul->check() )
    {
      assert( warlock_pet_spell_t::n_targets() == 0 );
      return as<int>( p()->o()->talents.unstable_soul_buff->effectN( 2 ).base_value() ) + 1;
    }
    else
    {
      return warlock_pet_spell_t::n_targets();
    }
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    warlock_pet_spell_t::available_targets( tl );

    // If there is more than one target, Unstable Soul always fires all extra hits,
    // even if some targets are hit multiple times
    if ( p()->buffs.unstable_soul->check() )
    {
      const int n_tar = n_targets();
      const size_t og_tar_count = tl.size();

      if ( n_tar > 1 && og_tar_count > 1 && og_tar_count < as<size_t>( n_tar ) )
      {
        size_t remaining_tar = as<size_t>( n_tar ) - og_tar_count;

        tl.reserve( as<size_t>( n_tar ) );
        while ( remaining_tar >= og_tar_count )
        {
          for ( size_t i = 0; i < og_tar_count; ++i )
            tl.push_back( tl[ i ] );

          remaining_tar -= og_tar_count;
        }

        if ( remaining_tar > 0 )
        {
          std::vector<player_t*> shuff_tar( tl.begin(), tl.begin() + og_tar_count );
          rng().shuffle( shuff_tar.begin(), shuff_tar.end() );

          for ( size_t i = 0; i < remaining_tar; ++i )
            tl.push_back( shuff_tar[ i ] );
        }
      }
    }

    return tl.size();
  }

  std::vector<player_t*>& target_list() const override
  {
    // Invalidate the target cache whenever the current Unstable Soul buff state differs from the cached one,
    // since available_targets() builds a different target list depending on whether the buff is active or not.
    const bool unstable_soul_buff = p()->buffs.unstable_soul->check();
    if ( target_cache_unstable_soul_state != unstable_soul_buff )
    {
      target_cache.is_valid = false;
      target_cache_unstable_soul_state = unstable_soul_buff;
    }

    return warlock_pet_spell_t::target_list();
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    // Extra Fel Firebolts from Infernal Rapidity cannot proc Infernal Rapidity again
    if ( ( twin != nullptr ) && ( p()->bugs ? debug_cast<wild_imp_pet_t*>( p() )->prd_rng_infernal_rapidity->trigger() : rng().roll( p()->o()->talents.infernal_rapidity->effectN( 1 ).percent() ) ) )
    {
      p()->o()->procs.infernal_rapidity->occur();
      twin->execute_on_target( target );
    }
  }

  void consume_resource() override
  {
    warlock_pet_spell_t::consume_resource();

    // Imp dies if it cannot cast
    if ( player->resources.current[ RESOURCE_ENERGY ] < cost() )
      make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
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

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
}

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
{ reschedule_firebolt(); }

void wild_imp_pet_t::finish_moving()
{
  warlock_pet_t::finish_moving();

  reschedule_firebolt();
}

void wild_imp_pet_t::arise()
{
  warlock_pet_t::arise();

  is_hog_imp = ( duration == o()->warlock_base.wild_imp->duration() ); // TODO: Only valid because duration diff, look for a safer way
  power_siphon = false;
  imploded = false;

  // Each Wild Imp uses its own independent accumulator PRD, reset to 0 on arise
  if ( o()->talents.infernal_rapidity.ok() )
    prd_rng_infernal_rapidity->reset( reset_type_e::COMBAT );

  o()->buffs.wild_imps->trigger();

  if ( o()->talents.summon_demonic_tyrant.ok() )
  {
    for ( auto t : o()->warlock_pet_list.demonic_tyrants )
    {
      if ( t->is_active() )
        t->buffs.demonic_power->trigger();
    }
  }

  // Set initial timers for Infernal Command buff sequence of events
  infernal_command_ev_ts = sim->current_time() + 5045_ms;
  infernal_command_ev_offset = o()->wild_imp_ic_shared_offset;

  // Start casting fel firebolts
  firebolt->set_target( o()->target );
  firebolt->schedule_execute();
}

void wild_imp_pet_t::demise()
{
  if ( !current.sleeping )
  {
    o()->buffs.wild_imps->decrement();

    buffs.imp_gang_boss->expire();

    buffs.unstable_soul->expire();

    buffs.infernal_command->expire();

    if ( o()->talents.summon_demonic_tyrant.ok() )
    {
      for ( auto t : o()->warlock_pet_list.demonic_tyrants )
      {
        // NOTE: 2026-04-24: Imploded Wild Imps does not substract stacks from Demonic Power buff (bug?)
        if ( t->is_active() && ( !bugs || !imploded ) )
          t->buffs.demonic_power->decrement();
      }
    }

    if ( o()->talents.demoniac.ok() )
    {
      if ( !power_siphon )
      {
        if ( imploded )
        {
          if ( o()->flat_rng.demoniac_imp_implosion->trigger() )
          {
            o()->buffs.demonic_core->trigger();
            o()->procs.demonic_core_imps_implosion->occur();
          }
        }
        else
        {
          if ( o()->prd_rng.demoniac_imp_fade->trigger() )
          {
            o()->buffs.demonic_core->trigger();
            o()->procs.demonic_core_imps_fade->occur();
          }
        }
      }
    }

    // Manual handling of Hellbent Commander buff for Wild Imps
    // NOTE (2026-04-24): Wild Imps are currently bugged when updating Hellbent Commander stacks on demise:
    // If imploded, imps summoned via HoG decrease one stack each, while those summoned via Inner Demons,
    // Spiteful Reconstitution, or To Hell and Back do not decrease any stacks.
    // If the imps demise normally or are sacrificed with Power Siphon, HoG imps decrease two stacks each,
    // while all other imps decrease one stack each.
    // Hellbent Commander's stacks are updated to their correct value on each heartbeat update.
    if ( bugs && o()->talents.hellbent_commander.ok() )
    {
      if ( imploded )
      {
        if ( is_hog_imp )
          o()->buffs.hellbent_commander->decrement();
      }
      else
      {
        if ( is_hog_imp )
          o()->buffs.hellbent_commander->decrement( 2 );
        else
          o()->buffs.hellbent_commander->decrement();
      }
    }

    if ( expiration )
      event_t::cancel( expiration );
  }

  warlock_pet_t::demise();
}

double wild_imp_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  m *= 1.0 + buffs.imp_gang_boss->check_value();

  m *= 1.0 + buffs.unstable_soul->check_value();

  return m;
}

/// Wild Imp End

/// Dreadstalker Begin

dreadstalker_t::dreadstalker_t( warlock_t* owner ) : warlock_pet_t( owner, "dreadstalker", PET_DREADSTALKER, true )
{
  npc_id = owner->talents.call_dreadstalkers_summon_1->effectN( 1 ).misc_value1();

  action_list_str = "leap/travel/dreadbite";
  resource_regeneration  = regen_type::DISABLED;

  // 2026-02-17: Validated coefficient
  owner_coeff.ap_from_sp = 0.825;

  owner_coeff.health = 0.4;

  melee_on_summon = false;
  server_action_delay = 0_ms; // Will be set when spawning Dreadstalkers to ensure pets are synced on delay
}

dreadstalker_t::dreadstalker_t( warlock_t* owner, util::string_view pet_name, pet_e pet_type )
  : warlock_pet_t( owner, pet_name, pet_type, true )
{
  npc_id = owner->talents.call_dreadstalkers_summon_1->effectN( 1 ).misc_value1();

  action_list_str = "leap/travel/dreadbite";
  resource_regeneration  = regen_type::DISABLED;

  // 2026-02-17: Validated coefficient
  owner_coeff.ap_from_sp = 0.825;

  owner_coeff.health = 0.4;
}

struct dreadbite_t : public warlock_pet_melee_attack_t
{
  dreadbite_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Dreadbite", p, p->find_spell( 271971 ) )
  {
    weapon = &( p->main_hand_weapon );

    if ( p->o()->talents.dreadlash.ok() )
    {
      aoe = -1;
      reduced_aoe_targets = 5; // TOCHECK regularly: 2025-08-27
      radius = 8.0;

      base_dd_multiplier *= 1.0 + p->o()->talents.dreadlash->effectN( 1 ).percent();
    }
  }

  bool ready() override
  {
    if ( debug_cast< dreadstalker_t* >( p() )->dreadbite_executes <= 0 )
      return false;

    return warlock_pet_melee_attack_t::ready();
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    debug_cast< dreadstalker_t* >( p() )->dreadbite_executes--;
  }

  void impact( action_state_t* s ) override
  {
    warlock_pet_melee_attack_t::impact( s );

    if ( p()->o()->talents.blighted_maw.ok() && result_is_hit( s->result ) )
    {
      auto amount = s->result_amount;
      amount *= p()->o()->talents.blighted_maw->effectN( 1 ).percent();

      p()->o()->proc_actions.blighted_maw->execute_on_target( s->target, amount );
    }
  }
};

// Carnivorous Stalkers talent handling requires special version of melee attack
struct dreadstalker_melee_t : warlock_pet_melee_t
{
  dreadstalker_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  { }

  void execute() override
  {
    warlock_pet_melee_t::execute();

    if ( p()->o()->talents.carnivorous_stalkers.ok() && p()->o()->flat_rng.carnivorous_stalkers->trigger() )
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

void dreadstalker_t::queue_dreadbite()
{
  dreadbite_executes++;

  if ( !readying && !channeling && !executing )
    schedule_ready();

  if ( readying )
  {
    event_t::cancel( readying );
    schedule_ready();
  }
}

struct dreadstalker_leap_t : warlock_pet_t::travel_t
{
  dreadstalker_leap_t( dreadstalker_t* p ) : warlock_pet_t::travel_t( p, "leap" )
  {
    speed = 33.17; // This speed value will be updated in the 'schedule_execute', since leap speed have some variation with distance
  }

  void schedule_execute( action_state_t* s ) override
  {
    debug_cast<warlock_pet_t*>( player )->melee_attack->cancel();

    // The dreadstalkers' travel speed is not constant, since it has a certain acceleration
    // The average speed for various distances is extracted from the ingame behavior and the rest is interpolated, thus obtaining a lookup table
    // 2025-04-06: lookup_table for speeds in [5-40]yd TO 1yd range (there should be no distance values outside this range, but we will handle them just in case)
    const size_t distance_st = static_cast<size_t>( player->current.distance + 0.5 );
    const std::array<double, 36> lookup_table_speed = {
                                  14.81, 15.50, 16.42, 18.89, 20.73, 22.19, // [ 5yd-10yd]
      23.41, 24.45, 25.36, 26.17, 26.89, 27.55, 28.15, 28.70, 29.21, 29.69, // [11yd-20yd]
      30.13, 30.54, 30.94, 31.31, 31.66, 31.99, 32.30, 32.61, 32.89, 33.17, // [21yd-30yd]
      33.43, 33.69, 33.93, 34.17, 34.40, 34.61, 34.83, 35.03, 35.23, 35.42  // [31yd-40yd]
    };
    speed = lookup_table_speed[ std::min( std::max( distance_st, as<size_t>( 5 ) ), as<size_t>( 40 ) ) - as<size_t>( 5 ) ];

    warlock_pet_t::travel_t::schedule_execute( s );
  }

  bool ready() override
  {
    // Dreadstalkers will not do a leap if are summoned too close to the target. In addition, the leap can only occur once.
    return ( ( !debug_cast<dreadstalker_t*>( player )->melee_on_summon ) && ( debug_cast<dreadstalker_t*>( player )->leap_executes > 0 ) && ( warlock_pet_t::travel_t::ready() ) );
  }

  void execute() override
  {
    warlock_pet_t::travel_t::execute();

    // There is an observed delay of up to 1 second before a melee attack begins again for pets after a movement action like the leap (possibly server tick?)
    make_event( sim, debug_cast<dreadstalker_t*>( player )->server_action_delay, [ this ]{
      debug_cast<warlock_pet_t*>( player )->melee_attack->reset();
      debug_cast<warlock_pet_t*>( player )->melee_attack->schedule_execute();
    } );

    debug_cast<dreadstalker_t*>( player )->leap_executes--;
  }
};

void dreadstalker_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();
  resources.base[ RESOURCE_ENERGY ] = 0;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
  melee_attack = new dreadstalker_melee_t( this, 0.96 ); // TOCHECK: This number may require tweaking if the AP coeff changes
}

void dreadstalker_t::arise()
{
  if ( o()->get_player_distance( *target ) <= 5.0 )
    melee_on_summon = true; // Within this range, Dreadstalkers will not do a leap, so they immediately start using auto attacks
  else
    melee_on_summon = false; // Dreadstalkers leap from the player location to target, which has a non-negligible travel time

  warlock_pet_t::arise();

  o()->buffs.dreadstalkers->trigger();

  if ( o()->talents.summon_demonic_tyrant.ok() )
  {
    for ( auto t : o()->warlock_pet_list.demonic_tyrants )
    {
      if ( t->is_active() )
        t->buffs.demonic_power->trigger();
    }
  }

  if ( o()->talents.flametouched.ok() )
    buffs.flametouched->trigger();

  dreadbite_executes = 1;

  leap_executes = 1;
}

void dreadstalker_t::demise()
{
  if ( !current.sleeping )
  {
    o()->buffs.dreadstalkers->decrement();

    if ( o()->talents.summon_demonic_tyrant.ok() )
    {
      for ( auto t : o()->warlock_pet_list.demonic_tyrants )
      {
        if ( t->is_active() )
          t->buffs.demonic_power->decrement();
      }
    }

    if ( o()->talents.demoniac.ok() )
    {
      bool success = o()->buffs.demonic_core->trigger( 1, buff_t::DEFAULT_VALUE(), o()->talents.demonic_core_spell->effectN( 2 ).percent() );
      if ( success )
        o()->procs.demonic_core_dogs->occur();
    }
  }

  warlock_pet_t::demise();
}

double dreadstalker_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  if ( o()->active_4pc<MID1>() )
    m *= 1.0 + o()->tier.wl_demonology_12_0_class_set_4pc->effectN( 1 ).percent();

  return m;
}

double dreadstalker_t::composite_melee_crit_chance() const
{
  double m = warlock_pet_t::composite_melee_crit_chance();

  if ( buffs.flametouched->check() )
    m += buffs.flametouched->data().effectN( 2 ).percent();

  return m;
}

double dreadstalker_t::composite_spell_crit_chance() const
{
  double m = warlock_pet_t::composite_spell_crit_chance();

  if ( buffs.flametouched->check() )
    m += buffs.flametouched->data().effectN( 2 ).percent();

  return m;
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
  if ( name == "leap" )
    return new dreadstalker_leap_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Dreadstalker End

/// Vilefiend Begin

vilefiend_t::vilefiend_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "vilefiend", PET_VILEFIEND )
{
  if ( owner->talents.mark_of_shatug.ok() )
  {
    npc_id = owner->talents.gloomhound->effectN( 1 ).misc_value1();
    npc_suffix = "vilefiend";
  }
  else if ( owner->talents.mark_of_fharg.ok() )
  {
    npc_id = owner->talents.charhound->effectN( 1 ).misc_value1();
    npc_suffix = "vilefiend";
  }
  else
  {
    npc_id = owner->talents.vilefiend->effectN( 1 ).misc_value1();
    // NOTE: 2026-04-24 Regular Vilefiend do not trigger Hellbent Commander on heartbeat (bug?)
    triggers.hellbent_commander_heartbeat &= !bugs;
  }

  // NOTE: 2026-04-24 Vilefiend do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  action_list_str = "bile_spit";
  action_list_str += "/travel";
  action_list_str += "/headbutt";

  // Currently bugged and not being affected by the crit bonus
  affected_by.demonic_brutality = false;

  // 2026-02-17: Validated coefficients
  owner_coeff.ap_from_sp = 0.45;
  owner_coeff.sp_from_sp = 1.95;

  owner_coeff.health = 0.75;

  bile_spit_executes = 1; // Only one Bile Spit per summon
}

struct gloom_slash_t : public warlock_pet_spell_t
{
  gloom_slash_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Gloom Slash", p, p->o()->talents.gloom_slash )
  { background = dual = true; }
};

struct vilefiend_melee_t : public warlock_pet_melee_t
{
  gloom_slash_t* gloom;

  vilefiend_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" )
    : warlock_pet_melee_t( p, wm, name )
  {
    gloom = new gloom_slash_t( p );
  }

  void execute() override
  {
    warlock_pet_melee_t::execute();

    if ( debug_cast<vilefiend_t*>( p() )->mark_of_shatug->check() )
      gloom->execute_on_target( target );
  }
};

struct bile_spit_t : public warlock_pet_spell_t
{
  bile_spit_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Bile Spit", p, p->o()->talents.bile_spit )
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

  // NOTE: 2026-02-17 Bile Spit spell cast time is not affected by any haste effects
  double execute_time_pct_multiplier() const override
  { return 1.0; }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    debug_cast< vilefiend_t* >( p() )->bile_spit_executes--;
  }
};

struct headbutt_t : public warlock_pet_melee_attack_t
{
  gloom_slash_t* gloom;

  headbutt_t( warlock_pet_t* p ) : warlock_pet_melee_attack_t( "Headbutt", p, p->o()->talents.headbutt )
  {
    cooldown->duration = 5_s;

    gloom = new gloom_slash_t( p );
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    if ( debug_cast<vilefiend_t*>( p() )->mark_of_shatug->check() )
      gloom->execute_on_target( target );
  }
};

struct infernal_presence_t : public warlock_pet_spell_t
{
  infernal_presence_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Infernal Presence", p, p->o()->talents.infernal_presence_dmg )
  {
    background = true;
    aoe = -1;
  }
};

void vilefiend_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();

  melee_attack = new vilefiend_melee_t( this, 2.0 );

  special_ability = new headbutt_t( this );
}

void vilefiend_t::create_buffs()
{
  warlock_simple_pet_t::create_buffs();

  mark_of_shatug = make_buff<buff_t>( this, "mark_of_shatug", o()->talents.mark_of_shatug )
                       ->set_proc_callbacks( false );

  mark_of_fharg = make_buff<buff_t>( this, "mark_of_fharg", o()->talents.mark_of_fharg )
                      ->set_proc_callbacks( false );

  auto damage = new infernal_presence_t( this );

  infernal_presence = make_buff<buff_t>( this, "infernal_presence", o()->talents.infernal_presence )
                               ->set_tick_callback( [ damage, this ]( buff_t*, int, timespan_t ) {
                                 if ( target )
                                   damage->execute_on_target( target );
                               } );

  infernal_presence->quiet = true;
}

void vilefiend_t::arise()
{
  warlock_simple_pet_t::arise();

  bile_spit_executes = 1;

  o()->buffs.vilefiend->trigger();

  if ( o()->talents.mark_of_shatug.ok() )
    mark_of_shatug->trigger();

  if ( o()->talents.mark_of_fharg.ok() )
  {
    mark_of_fharg->trigger();
    infernal_presence->trigger();
  }
}

void vilefiend_t::demise()
{
  if ( !current.sleeping )
    o()->buffs.vilefiend->decrement();

  warlock_simple_pet_t::demise();
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
  : warlock_pet_t( owner, name, PET_DEMONIC_TYRANT, true )
{
  npc_id = owner->talents.summon_demonic_tyrant->effectN( owner->talents.antoran_armaments.ok() ? 4 : 1 ).misc_value1();

  // NOTE: 2026-04-24 Demonic Tyrant do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  resource_regeneration = regen_type::DISABLED;
  if ( o()->talents.antoran_armaments.ok() )
    action_list_str = "leap/travel/burning_cleave";
  else
    action_list_str = "demonfire";

  melee_on_summon = false;
}

struct demonfire_t : public warlock_pet_spell_t
{
  demonfire_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Demonfire", p, p->find_spell( 270481 ) )
  { parse_options( options_str ); }
};

struct burning_cleave_t : public warlock_pet_spell_t
{
  burning_cleave_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Burning Cleave", p, p->find_spell( 1264093 ) )
  {
    parse_options( options_str );

    // Actually just an auto attack with a 2s swing time. Simplifying the code doing it this way.
    trigger_gcd = 2_s;
    min_gcd = 0_s;

    weapon = &( p->main_hand_weapon );

    aoe = -1;
    reduced_aoe_targets = as<int>( data().effectN( 2 ).base_value() );
  }

  int n_targets() const override
  {
    // Tyrant with Antoran Armaments (Burning Cleave) has a very narrow arc, so it often misses some targets.
    // This behavior is replicated through a configurable option that controls the target ratio affected by Burning Cleave.
    if ( p()->o()->talents.antoran_armaments.ok() && p()->o()->tyrant_antoran_armaments_target_mul < 1.0 )
    {
      assert( warlock_pet_spell_t::n_targets() == -1 );
      const size_t cur_n_targets = target_list().size();
      return std::max( 1, as<int>( std::lround( cur_n_targets * p()->o()->tyrant_antoran_armaments_target_mul ) ) );
    }
    else
    {
      return warlock_pet_spell_t::n_targets();
    }
  }
};

struct demonic_tyrant_leap_t : warlock_pet_t::travel_t
{
  demonic_tyrant_leap_t( demonic_tyrant_t* p ) : warlock_pet_t::travel_t( p, "leap" )
  {
    speed = 32.0;
  }

  bool ready() override
  {
    // Demonic Tyrants will not do a leap if are summoned too close to the target. In addition, the leap can only occur once.
    return ( ( !debug_cast<demonic_tyrant_t*>( player )->melee_on_summon ) && ( debug_cast<demonic_tyrant_t*>( player )->leap_executes > 0 ) && ( warlock_pet_t::travel_t::ready() ) );
  }

  void execute() override
  {
    warlock_pet_t::travel_t::execute();

    // Demonic Tyrant does not use auto attacks, so there is no need to schedule new ones

    debug_cast<demonic_tyrant_t*>( player )->leap_executes--;
  }
};

action_t* demonic_tyrant_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "demonfire" )
    return new demonfire_t( this, options_str );
  if ( name == "burning_cleave" )
    return new burning_cleave_t( this, options_str );
  if ( name == "leap" )
    return new demonic_tyrant_leap_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

void demonic_tyrant_t::arise()
{
  if ( o()->get_player_distance( *target ) <= 5.0 )
    melee_on_summon = true; // Within this range, Demonic Tyrant will not do a leap, so it immediately starts to attack
  else
    melee_on_summon = false; // Demonic Tyrant leaps from the player location to target, which has a non-negligible travel time

  warlock_pet_t::arise();

  leap_executes = 1;
}

double demonic_tyrant_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  m *= 1.0 + buffs.demonic_power->check_stack_value();

  return m;
}

/// Demonic Tyrant End

/// Doomguard Begin

struct doom_bolt_volley_t : public warlock_pet_spell_t
{
  doom_bolt_volley_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Doom Bolt Volley", p, p->o()->talents.doom_bolt_volley )
  {
    radius = p->o()->talents.doom_bolt_volley->effectN( 2 ).base_value();
  }
};

doomguard_t::doomguard_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "doomguard", PET_DOOMGUARD )
{
  npc_id = owner->talents.summon_doomguard->effectN( 1 ).misc_value1();

  action_list_str = "doom_bolt_volley";

  // 2026-02-17: Validated coefficients
  owner_coeff.ap_from_sp = 1.0;
  owner_coeff.sp_from_sp = 1.0;
}

void doomguard_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();

  special_ability = new doom_bolt_volley_t( this );
}

action_t* doomguard_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "doom_bolt_volley" )
    return new doom_bolt_volley_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

void doomguard_t::arise()
{
  warlock_simple_pet_t::arise();

  o()->buffs.doomguard->trigger();
}

void doomguard_t::demise()
{
  if ( !current.sleeping )
    o()->buffs.doomguard->decrement();

  warlock_simple_pet_t::demise();
}

/// Doomguard End

/// Grimoire: Imp Lord Begin

struct greater_felbolt_t : public warlock_pet_spell_t
{
  greater_felbolt_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Greater Felbolt", p, p->find_spell( 1277116 ) )
  { parse_options( options_str ); }
};

grimoire_imp_lord_t::grimoire_imp_lord_t( warlock_t* owner )
  : warlock_pet_t( owner, "grimoire_imp_lord", PET_SERVICE_IMP ),
    max_energy_threshold( 125 )
{
  npc_id = owner->talents.grimoire_imp_lord->effectN( 2 ).misc_value1();
  npc_suffix = "grimoire";

  // NOTE: 2026-04-24 Grimoire: Imp Lord do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  action_list_str = "greater_felbolt,if=energy>=" + util::to_string( max_energy_threshold );
}

void grimoire_imp_lord_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  resources.base[ RESOURCE_ENERGY ] = 200;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;

  // 2026-02-17: Validated coefficients
  owner_coeff.ap_from_sp = 1.375;
  owner_coeff.sp_from_sp = 2.75;
  owner_coeff.health = 0.45;
}

action_t* grimoire_imp_lord_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "greater_felbolt" )
    return new greater_felbolt_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

void grimoire_imp_lord_t::arise()
{
  warlock_pet_t::arise();

  buffs.grimoire_of_service->trigger();

  o()->buffs.grimoire_imp_lord->trigger();
}

void grimoire_imp_lord_t::demise()
{
  if ( !current.sleeping )
  {
    buffs.grimoire_of_service->decrement();

    o()->buffs.grimoire_imp_lord->decrement();
  }

  warlock_pet_t::demise();
}

double grimoire_imp_lord_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  m *= 1.0 + buffs.grimoire_of_service->check_value();

  return m;
}

/// Grimoire: Imp Lord End

/// Grimoire: Fel Ravager Begin

struct abyssal_bite_t : public warlock_pet_spell_t
{
  abyssal_bite_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Abyssal Bite", p, p->find_spell( 1277117 ) )
  { parse_options( options_str ); }

};

grimoire_fel_ravager_t::grimoire_fel_ravager_t( warlock_t* owner )
  : warlock_pet_t( owner, "grimoire_fel_ravager", PET_SERVICE_FELHUNTER ),
    max_energy_threshold( 160 )
{
  npc_id = owner->talents.grimoire_fel_ravager->effectN( 2 ).misc_value1();
  npc_suffix = "grimoire";

  // NOTE: 2026-04-24 Grimoire: Fel Ravager do not trigger Hellbent Commander on demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_demise &= !bugs;

  action_list_str = "travel/abyssal_bite,if=energy>=" + util::to_string( max_energy_threshold );
}

void grimoire_fel_ravager_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  resources.base[ RESOURCE_ENERGY ] = 200;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;

  // 2026-02-17: Validated coefficients
  owner_coeff.ap_from_sp = 1.26;
  owner_coeff.sp_from_sp = 2.51;
  owner_coeff.health = 0.5;

  melee_attack = new warlock_pet_melee_t( this );
  special_action = new base::spell_lock_t( this, "" );
}

action_t* grimoire_fel_ravager_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "abyssal_bite" )
    return new abyssal_bite_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

void grimoire_fel_ravager_t::arise()
{
  warlock_pet_t::arise();

  buffs.grimoire_of_service->trigger();

  o()->buffs.grimoire_fel_ravager->trigger();
}

void grimoire_fel_ravager_t::demise()
{
  if ( !current.sleeping )
  {
    buffs.grimoire_of_service->decrement();

    o()->buffs.grimoire_fel_ravager->decrement();
  }

  warlock_pet_t::demise();
}

double grimoire_fel_ravager_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  m *= 1.0 + buffs.grimoire_of_service->check_value();

  return m;
}

/// Grimoire: Fel Ravager End

/// Dominion of Argus Pet Base begin
struct dominion_of_argus_spell_base_t : public warlock_pet_spell_t
{
  dominion_of_argus_spell_base_t( std::string_view name, dominion_of_argus_pet_t* p, const spell_data_t* data )
    : warlock_pet_spell_t( name, p, data )
  {
    background = repeating = true;
  }
};

dominion_of_argus_pet_t::dominion_of_argus_pet_t( warlock_t* owner, std::string_view n, pet_e type )
  : warlock_pet_t( owner, n, type, true ), main_action( nullptr )
{
  resource_regeneration = regen_type::DISABLED;
  affected_by.demonic_brutality = false;
  // NOTE: 2026-04-24 DoA guardians do not trigger Hellbent Commander on arise/demise (must wait to player heatbeat) (bug?)
  triggers.hellbent_commander_arise &= !bugs;
  triggers.hellbent_commander_demise &= !bugs;
}

void dominion_of_argus_pet_t::set_main_action( action_t* a )
{
  this->main_action = a;
}

// Use APL-less logic to schedule the action executes since they only cast one spell.
// This significantly speeds up sim time when the pet is active, since we don't have to evaluate the APL every GCD.
void dominion_of_argus_pet_t::reschedule_main_action()
{
  if ( executing || is_sleeping() || player_t::buffs.movement->check() || player_t::buffs.stunned->check() )
    return;

  timespan_t gcd_adjust = gcd_ready - sim->current_time();
  if ( gcd_adjust > 0_ms )
  {
    make_event( sim, gcd_adjust, [ this ]() {
      main_action->set_target( o()->target );
      main_action->schedule_execute();
    } );
  }
  else
  {
    main_action->set_target( o()->target );
    main_action->schedule_execute();
  }
}

resource_e dominion_of_argus_pet_t::primary_resource() const
{
  return RESOURCE_NONE;
}

void dominion_of_argus_pet_t::schedule_ready( timespan_t /* delta_time */, bool /* waiting */ )
{
  reschedule_main_action();
}

void dominion_of_argus_pet_t::finish_moving()
{
  warlock_pet_t::finish_moving();

  reschedule_main_action();
}

void dominion_of_argus_pet_t::arise()
{
  warlock_pet_t::arise();
  // Start casting their main action
  main_action->set_target( o()->target );
  main_action->schedule_execute();
}
/// Dominion of Argus Pet Base end

/// Lady Sacrolash Begin
struct shadow_nova_t : public dominion_of_argus_spell_base_t
{
  shadow_nova_t( dominion_of_argus_pet_t* p )
    : dominion_of_argus_spell_base_t( "Shadow Nova", p, p->find_spell( 1282507 ) )
  {
    aoe                 = -1;
    reduced_aoe_targets = as<int>( data().effectN( 2 ).base_value() );
  }
};

lady_sacrolash_t::lady_sacrolash_t( warlock_t* owner )
  : dominion_of_argus_pet_t( owner, "lady_sacrolash", PET_LADY_SACROLASH )
{
  npc_id = owner->talents.doa_lady_sacrolash_summon->effectN( 1 ).misc_value1();

  owner_coeff.sp_from_sp = 1.0;
}

void lady_sacrolash_t::create_actions()
{
  dominion_of_argus_pet_t::create_actions();
  set_main_action( new shadow_nova_t( this ) );
}
/// Lady Sacrolash End

/// Grand Warlock Alythess Begin
struct blaze_t : public warlock_pet_spell_t
{
  blaze_t( dominion_of_argus_pet_t* p )
    : warlock_pet_spell_t( "Blaze", p, p->find_spell( 1282534 ) )
  {
    aoe                 = -1;
    background          = true;
    reduced_aoe_targets = as<int>( data().effectN( 2 ).base_value() );
  }
};

struct blaze_missile_t : public dominion_of_argus_spell_base_t
{
  blaze_missile_t( dominion_of_argus_pet_t* p )
    : dominion_of_argus_spell_base_t( "Blaze Missile", p, p->find_spell( 1282533 ) )
  {
    impact_action = new blaze_t( p );
    // Merge the two actions in the HTML report for cleaner reporting
    impact_action->stats = stats;
    stats->action_list.push_back( impact_action );
    name_str_reporting = "blaze";
  }
};

grand_warlock_alythess_t::grand_warlock_alythess_t( warlock_t* owner )
  : dominion_of_argus_pet_t( owner, "grand_warlock_alythess", PET_GRAND_WARLOCK_ALYTHESS )
{
  npc_id = owner->talents.doa_grand_warlock_alythess_summon->effectN( 1 ).misc_value1();

  owner_coeff.sp_from_sp = 1.0;
}

void grand_warlock_alythess_t::create_actions()
{
  dominion_of_argus_pet_t::create_actions();
  set_main_action( new blaze_missile_t( this ) );
}
/// Grand Warlock Alythess End

/// Antoran Inquisitor Begin
struct mind_sear_t : public warlock_pet_spell_t
{
  mind_sear_t( dominion_of_argus_pet_t* p )
    : warlock_pet_spell_t( "Mind Sear", p, p->find_spell( 1280460 ) )
  {
    background = true;
    aoe = -1;
  }
};

struct mind_sear_channel_t : public dominion_of_argus_spell_base_t
{
  mind_sear_channel_t( dominion_of_argus_pet_t* p )
    : dominion_of_argus_spell_base_t( "Mind Sear Channel", p, p->find_spell( 1280457 ) )
  {
    tick_action  = new mind_sear_t( p );
    hasted_ticks = false;
    tick_zero = channeled = true;
    base_tick_time        = data().effectN( 1 ).period();
    base_execute_time     = data().duration();
    name_str_reporting    = "mind_sear";
  }
};

antoran_inquisitor_t::antoran_inquisitor_t( warlock_t* owner )
  : dominion_of_argus_pet_t( owner, "antoran_inquisitor", PET_ANTORAN_INQUISITOR )
{
  npc_id = owner->talents.doa_antoran_inquisitor_summon->effectN( 1 ).misc_value1();

  owner_coeff.sp_from_sp = 1.0;
}

void antoran_inquisitor_t::create_actions()
{
  dominion_of_argus_pet_t::create_actions();
  set_main_action( new mind_sear_channel_t( this ) );
}
/// Antoran Inquisitor End

/// Antoran Jailer Begin
struct soul_barrage_t : public warlock_pet_spell_t
{
  soul_barrage_t( dominion_of_argus_pet_t* p ) : warlock_pet_spell_t( "Soul Barrage", p, p->find_spell( 1292391 ) )
  {
    background = true;
  }
};

struct soul_barrage_cast_t : public dominion_of_argus_spell_base_t
{
  soul_barrage_cast_t( dominion_of_argus_pet_t* p )
    : dominion_of_argus_spell_base_t( "Soul Barrage (Cast)", p, p->find_spell( 1292384 ) )
  {
    aoe           = as<int>( data().effectN( 1 ).base_value() );
    impact_action = new soul_barrage_t( p );

    // Merge the two actions in the HTML report for cleaner reporting
    impact_action->stats = stats;
    stats->action_list.push_back( impact_action );
    name_str_reporting = "soul_barrage";
  }
};

antoran_jailer_t::antoran_jailer_t( warlock_t* owner )
  : dominion_of_argus_pet_t( owner, "antoran_jailer", PET_ANTORAN_JAILER )
{
  npc_id = owner->talents.doa_antoran_jailer_summon->effectN( 1 ).misc_value1();

  owner_coeff.sp_from_sp = 1.0;
}

void antoran_jailer_t::create_actions()
{
  dominion_of_argus_pet_t::create_actions();
  set_main_action( new soul_barrage_cast_t( this ) );
}

/// Antoran Jailer End

}  // namespace demonology

namespace destruction
{

/// Infernal Begin

infernal_t::infernal_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_INFERNAL, true ), immolation( nullptr )
{
  npc_id = owner->talents.summon_infernal_main->effectN( 1 ).misc_value1();

  resource_regeneration = regen_type::DISABLED;

  type = MAIN;

  owner_coeff.ap_from_sp = 2.2275;
  owner_coeff.sp_from_sp = 2.2275;
}

struct immolation_tick_t : public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Immolation", p, p->o()->talents.immolation_dmg )
  {
    aoe = -1;
    background = may_crit = true;
  }
};

struct infernal_melee_t : warlock_pet_melee_t
{
  infernal_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  { }
};

void infernal_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  melee_attack = new infernal_melee_t( this, 1.0 );
}

void infernal_t::create_buffs()
{
  warlock_pet_t::create_buffs();

  auto damage = new immolation_tick_t( this );

  immolation = make_buff<buff_t>( this, "immolation", o()->talents.immolation_buff )
                   ->set_tick_callback( [ damage, this ]( buff_t*, int, timespan_t ) {
                     damage->execute_on_target( target );
                   } );
}

void infernal_t::arise()
{
  warlock_pet_t::arise();

  // 2024-07-18 Testing indicates there is a delay after spawn before first melee
  // Embers looks to trigger at around the same time as first melee swing, but Immolation takes longer to apply (and has no zero-tick)
  // Additionally, there is some unknown amount of movement adjustment the pet can take, so we model this with a distribution
  timespan_t delay = rng().gauss<1000,100>();

  make_event( *sim, delay, [ this ] {
    buffs.embers->trigger();

    melee_attack->set_target( target );
    melee_attack->schedule_execute();
  } );

  timespan_t immolation_delay = rng().range( 0_ms, 750_ms );
  make_event( *sim, delay + immolation_delay, [ this ] {
    immolation->trigger();
  } );
}

void infernal_t::demise()
{
  warlock_pet_t::demise();

  if ( o()->hero.abyssal_dominion.ok() && type == MAIN )
    make_event( sim, [ this ] {
      // Summon two infernal fragments
      o()->summons.fragment->execute();
      o()->summons.fragment->execute();
    } );
}

double infernal_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  if ( o()->hero.abyssal_dominion.ok() && ( type == MAIN || type == RAIN ) )
    m *= 1.0 + o()->hero.abyssal_dominion->effectN( 3 ).percent();

  return m;
}

/// Infernal End

/// Infernal Rain of Chaos Begin

infernal_roc_t::infernal_roc_t( warlock_t* owner, util::string_view name ) : destruction::infernal_t( owner, name )
{
  npc_id = owner->talents.summon_infernal_roc->effectN( 1 ).misc_value1();
  npc_suffix = "roc";

  type                   = RAIN;
  owner_coeff.ap_from_sp = 1.5;
  owner_coeff.sp_from_sp = 1.5;
}

/// Infernal Rain of Chaos End
///
/// Dimensional Rifts Begin

shadowy_tear_t::shadowy_tear_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
{
  npc_id = owner->talents.shadowy_tear_summon->effectN( 1 ).misc_value1();

  resource_regeneration = regen_type::DISABLED;

  action_list_str = "shadow_barrage";
}

struct rift_shadow_bolt_t : public warlock_pet_spell_t
{
  rift_shadow_bolt_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Shadow Bolt", p, p->o()->talents.rift_shadow_bolt )
  {
      background = dual = true;
  }
};

struct shadow_barrage_t : public warlock_pet_spell_t
{
  shadow_barrage_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Shadow Barrage", p, p->o()->talents.shadow_barrage )
  {
    tick_action = new rift_shadow_bolt_t( p );
  }

  bool ready() override
  {
    if ( debug_cast<shadowy_tear_t*>( p() )->barrages <=0 )
      return false;

    return warlock_pet_spell_t::ready();
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    debug_cast<shadowy_tear_t*>( p() )->barrages--;
  }

  double last_tick_factor( const dot_t*, timespan_t, timespan_t ) const override
  { return 1.0; }
};

void shadowy_tear_t::arise()
{
  warlock_pet_t::arise();

  barrages = 1;
}

action_t* shadowy_tear_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shadow_barrage" )
    return new shadow_barrage_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

unstable_tear_t::unstable_tear_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
{
  npc_id = owner->talents.unstable_tear_summon->effectN( 1 ).misc_value1();

  resource_regeneration = regen_type::DISABLED;

  action_list_str = "chaos_barrage";
}

struct chaos_barrage_tick_t : public warlock_pet_spell_t
{
  chaos_barrage_tick_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Chaos Barrage (tick)", p, p->o()->talents.chaos_barrage_tick )
  {
      background = dual = true;
  }
};

struct chaos_barrage_t : public warlock_pet_spell_t
{
  chaos_barrage_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Chaos Barrage", p, p->o()->talents.chaos_barrage )
  {
    tick_action = new chaos_barrage_tick_t( p );
  }

  bool ready() override
  {
    if ( debug_cast<unstable_tear_t*>( p() )->barrages <= 0 )
      return false;

    return warlock_pet_spell_t::ready();
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    debug_cast<unstable_tear_t*>( p() )->barrages--;
  }
};

void unstable_tear_t::arise()
{
  warlock_pet_t::arise();

  barrages = 1;
}

action_t* unstable_tear_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "chaos_barrage" )
    return new chaos_barrage_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

chaos_tear_t::chaos_tear_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
{
  npc_id = owner->talents.chaos_tear_summon->effectN( 1 ).misc_value1();

  resource_regeneration = regen_type::DISABLED;

  action_list_str = "chaos_bolt";
}

struct rift_chaos_bolt_t : public warlock_pet_spell_t
{
  rift_chaos_bolt_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Chaos Bolt", p, p->o()->talents.rift_chaos_bolt )
  { }

  double composite_crit_chance() const override
  { return 1.0; }

  bool ready() override
  {
    if ( debug_cast<chaos_tear_t*>( p() )->bolts <= 0 )
      return false;

    return warlock_pet_spell_t::ready();
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    debug_cast<chaos_tear_t*>( p() )->bolts--;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = warlock_pet_spell_t::composite_da_multiplier( s );

    m *= 1.0 + p()->current_pet_stats.composite_spell_crit;

    return m;
  }

  double action_multiplier() const override
  {
    double m = warlock_pet_spell_t::action_multiplier();

    double min_percentage = p()->o()->talents.chaos_incarnate.ok() ? p()->o()->talents.chaos_incarnate->effectN( 1 ).percent() : 0.5;
    double chaotic_energies_rng = rng().range( min_percentage , 1.0 );

    if ( p()->o()->normalize_destruction_mastery )
      chaotic_energies_rng = ( min_percentage + 1.0 ) * 0.5;

    m *= 1.0 + chaotic_energies_rng * p()->o()->cache.mastery_value();

    return m;
  }
};

void chaos_tear_t::arise()
{
  warlock_pet_t::arise();

  bolts = 1;
}

action_t* chaos_tear_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "chaos_bolt" )
    return new rift_chaos_bolt_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Dimensional Rifts End

/// Overfiend Begin

overfiend_t::overfiend_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
{
  npc_id = owner->talents.summon_overfiend->effectN( 1 ).misc_value1();

  resource_regeneration = regen_type::DISABLED;

  action_list_str = "chaos_bolt";
}

struct overfiend_chaos_bolt_t : public warlock_pet_spell_t
{
  overfiend_chaos_bolt_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Chaos Bolt", p, p->o()->talents.overfiend_cb )
  {
    spell_power_mod.direct = p->o()->talents.chaos_bolt->effectN( 1 ).sp_coeff();

    base_dd_multiplier *= p->o()->talents.avatar_of_destruction->effectN( 1 ).percent();
  }

  double composite_crit_chance() const override
  { return 1.0; }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = warlock_pet_spell_t::composite_da_multiplier( s );

    m *= 1.0 + p()->current_pet_stats.composite_spell_crit;

    return m;
  }

  double action_multiplier() const override
  {
    double m = warlock_pet_spell_t::action_multiplier();

    double min_percentage = p()->o()->talents.chaos_incarnate.ok() ? p()->o()->talents.chaos_incarnate->effectN( 1 ).percent() : 0.5;
    double chaotic_energies_rng = rng().range( min_percentage , 1.0 );

    if ( p()->o()->normalize_destruction_mastery )
      chaotic_energies_rng = ( min_percentage + 1.0 ) * 0.5;

    m *= 1.0 + chaotic_energies_rng * p()->o()->cache.mastery_value();

    return m;
  }
};

action_t* overfiend_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "chaos_bolt" )
    return new overfiend_chaos_bolt_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Overfiend End

}  // namespace destruction

namespace affliction
{

/// Darkglare Begin

darkglare_t::darkglare_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_DARKGLARE, true )
{
  npc_id = owner->talents.summon_darkglare->effectN( 1 ).misc_value1();

  action_list_str += "eye_beam";
}

struct eye_beam_t : public warlock_pet_spell_t
{
  player_t* last_chain_target;

  eye_beam_t( warlock_pet_t* p ) : warlock_pet_spell_t( "Eye Beam", p, p->o()->talents.eye_beam )
  {
    if ( p->o()->talents.nether_plating.ok() )
      radius = p->o()->talents.nether_plating->effectN( 2 ).base_value();
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    double dot_multiplier = p()->o()->talents.summon_darkglare->effectN( 3 ).percent();
    // NOTE: 2026-02-17 Darkglare Eye Beam target multipliers when using Nether Plating
    // Expected behavior: Each target hit in AoE takes damage based on their DoT count
    // Ingame real behavior (bug?): Only the last target hit in the AoE chain receives increased damage,
    // and it does so based on the DoT count of the first target hit (the target of the Darkglare).
    if ( !p()->o()->bugs || !p()->o()->talents.nether_plating.ok() )
    {
      double dots = p()->o()->get_target_data( target )->count_affliction_dots();

      m *= 1.0 + ( dots * dot_multiplier );
    }
    else
    {
      assert( last_chain_target && "Darkglare has no valid AoE last chain target" );
      if ( target == last_chain_target )
      {
        double dots = p()->o()->get_target_data( p()->target )->count_affliction_dots();

        m *= 1.0 + ( dots * dot_multiplier );
      }
    }

    return m;
  }

  void execute() override
  {
    int num_targets = n_targets();
    if ( p()->o()->bugs && p()->o()->talents.nether_plating.ok() && ( num_targets == -1 || num_targets > 0 ) )
    {
      std::vector<player_t*>& tl = target_list();
      const int max_targets = as<int>( tl.size() );
      num_targets = ( num_targets < 0 ) ? max_targets : std::min( max_targets, num_targets );
      last_chain_target = ( num_targets > 0 ) ? tl[ num_targets - 1 ] : nullptr;
    }

    warlock_pet_spell_t::execute();
  }
};

void darkglare_t::arise()
{
  warlock_pet_t::arise();

  o()->buffs.darkglare_presence->trigger();
};

void darkglare_t::demise()
{
  if ( !current.sleeping )
  {
    o()->buffs.darkglare_presence->expire();
  }

  warlock_pet_t::demise();
};

action_t* darkglare_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "eye_beam" )
    return new eye_beam_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Darkglare End

/// Desperate Soul Begin

desperate_soul_t::desperate_soul_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
{
  npc_id = owner->talents.summon_desperate_soul->effectN( 1 ).misc_value1();

  action_list_str = "wrath_of_nathreza";
}

struct wrath_of_nathreza_t : public warlock_pet_spell_t
{
  wrath_of_nathreza_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Wrath of Nathreza", p, p->o()->talents.wrath_of_nathreza_impact )
  {
    aoe = -1;
    reduced_aoe_targets = as<int>( p->o()->talents.wrath_of_nathreza->effectN( 2 ).base_value() );
  }

  bool ready() override
  {
    if ( debug_cast<desperate_soul_t*>( p() )->wraths <= 0 )
      return false;

    return warlock_pet_spell_t::ready();
  }

  void execute() override
  {
    if ( p()->o()->haunt_target && !p()->o()->haunt_target->is_sleeping() )
      target = p()->o()->haunt_target;

    warlock_pet_spell_t::execute();

    debug_cast<desperate_soul_t*>( p() )->wraths--;

    if ( debug_cast<desperate_soul_t*>( p() )->wraths <= 0 )
      make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
  }
};

void desperate_soul_t::arise()
{
  warlock_pet_t::arise();

  wraths = 1;
};

action_t* desperate_soul_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "wrath_of_nathreza" )
    return new wrath_of_nathreza_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Desperate_Soul End

}  // namespace affliction

namespace diabolist
{
  /// Diabolic Ritual Demons Begin

  overlord_t::overlord_t( warlock_t* owner, util::string_view name )
    : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
  {
    npc_id = owner->hero.summon_overlord->effectN( 1 ).misc_value1();

    is_diabolist_guardian = true;
    affected_by.demonic_brutality = false;
    // NOTE: 2026-04-24 Diabolist guardians do not trigger Hellbent Commander (bug?)
    triggers.hellbent_commander_heartbeat &= !bugs;
    triggers.hellbent_commander_arise &= !bugs;
    triggers.hellbent_commander_demise &= !bugs;
    resource_regeneration = regen_type::DISABLED;

    owner_coeff.ap_from_sp = 1.0;

    action_list_str = "charge/wicked_cleave";
  }

  struct overlord_charge_t : warlock_pet_t::travel_t
  {
    overlord_charge_t( overlord_t* p ) : warlock_pet_t::travel_t( p, "charge" )
    {
      speed = 25.0;
    }

    bool ready() override
    {
      // Overlord will do the charge even if is summoned at melee range. In addition, the charge can only occur once.
      return debug_cast<overlord_t*>( player )->charge_executes > 0;
    }

    timespan_t execute_time() const override
    {
      const timespan_t cast_time = 1_s; // TODO: Set cast time from spell data (432113)
      return cast_time * player->cache.spell_cast_speed() + warlock_pet_t::travel_t::execute_time();
    }

    void execute() override
    {
      warlock_pet_t::travel_t::execute();

      debug_cast<overlord_t*>( player )->charge_executes--;
    }
  };

  struct wicked_cleave_t : public warlock_pet_spell_t
  {
    wicked_cleave_t( warlock_pet_t* p )
      : warlock_pet_spell_t( "Wicked Cleave", p, p->o()->hero.wicked_cleave )
    { aoe = -1; }

    bool ready() override
    {
      if ( debug_cast<overlord_t*>( p() )->cleaves <= 0 )
        return false;

      return warlock_pet_spell_t::ready();
    }

    void execute() override
    {
      warlock_pet_spell_t::execute();

      debug_cast<overlord_t*>( p() )->cleaves--;

      if ( debug_cast<overlord_t*>( p() )->cleaves <= 0 )
        make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
    }

    // NOTE: 2026-02-17 Overlord Wicked Cleave crits does not benefit from other crit dmg bonus multipliers (bug?)
    double composite_crit_damage_bonus_multiplier() const override
    { return p()->bugs ? 1.0 : warlock_pet_spell_t::composite_crit_damage_bonus_multiplier(); }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_pet_spell_t::composite_da_multiplier( s ); // base value

      if ( p()->o()->demonology() )
      {
        // Added in build: 11.2.0.62253: reduces Diab Demons Damage by 20% for Demonology
        m *= 1.0 + p()->o()->hero.diabolic_ritual->effectN( 3 ).percent();
      }

      if ( p()->o()->destruction() )
      {
        // Added in build 11.2.0.62253: Increases Diab Demons damage by 15% for Destruction, missing from Patch Notes.
        m *= 1.0 + p()->o()->hero.diabolic_ritual->effectN( 4 ).percent();
      }

      return m;
    }

    void impact( action_state_t* s ) override
    {
      warlock_pet_spell_t::impact( s );

      if ( p()->o()->hero.cloven_souls.ok() )
        owner_td( s->target )->debuffs.cloven_soul->trigger();
    }
  };

  void overlord_t::arise()
  {
    warlock_pet_t::arise();

    charge_executes = 1;
    cleaves = 1;
  }

  action_t* overlord_t::create_action( util::string_view name, util::string_view options_str )
  {
    if ( name == "wicked_cleave" )
      return new wicked_cleave_t( this );
    if ( name == "charge" )
      return new overlord_charge_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }

  // NOTE: 2026-02-17 Overlord does not benefit from critical dmg multiplier effects (bug?)
  double overlord_t::composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const
  { return bugs ? 1.0 : warlock_pet_t::composite_player_critical_damage_multiplier( s, school ); }

  mother_of_chaos_t::mother_of_chaos_t( warlock_t* owner, util::string_view name )
    : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
  {
    npc_id = owner->hero.summon_mother->effectN( 1 ).misc_value1();

    is_diabolist_guardian = true;
    affected_by.demonic_brutality = false;
    // NOTE: 2026-04-24 Diabolist guardians do not trigger Hellbent Commander (bug?)
    triggers.hellbent_commander_heartbeat &= !bugs;
    triggers.hellbent_commander_arise &= !bugs;
    triggers.hellbent_commander_demise &= !bugs;

    action_list_str = "chaos_salvo";
  }

  struct chaos_salvo_tick_t : public warlock_pet_spell_t
  {
    chaos_salvo_tick_t( warlock_pet_t* p )
      : warlock_pet_spell_t( "Chaos Salvo (tick)", p, p->o()->hero.chaos_salvo_dmg )
    {
      background = dual = true;
      aoe = -1;

      travel_speed = p->o()->hero.chaos_salvo_missile->missile_speed();
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_pet_spell_t::composite_da_multiplier( s ); // base value

      if ( p()->o()->demonology() )
      {
        // Added in build: 11.2.0.62253: reduces Diab Demons Damage by 20% for Demonology
        m *= 1.0 + p()->o()->hero.diabolic_ritual->effectN( 3 ).percent();
      }

      if ( p()->o()->destruction() )
      {
        // Added in build 11.2.0.62253: Increases Diab Demons damage by 15% for Destruction, missing from Patch Notes.
        m *= 1.0 + p()->o()->hero.diabolic_ritual->effectN( 4 ).percent();
      }

      return m;
    }
  };

  struct chaos_salvo_t : public warlock_pet_spell_t
  {
    chaos_salvo_t( warlock_pet_t* p )
      : warlock_pet_spell_t( "Chaos Salvo", p, p->o()->hero.chaos_salvo )
    {
      channeled = true;
      // TOCHECK: Does Mother of Chaos have any cap with haste scaling?
      tick_action = new chaos_salvo_tick_t( p ); }

    bool ready() override
    {
      if ( debug_cast<mother_of_chaos_t*>( p() )->salvos <= 0 )
        return false;

      return warlock_pet_spell_t::ready();
    }

    void execute() override
    {
      warlock_pet_spell_t::execute();

      debug_cast<mother_of_chaos_t*>( p() )->salvos--;
    }

    void last_tick( dot_t* d ) override
    {
      warlock_pet_spell_t::last_tick( d );

      if ( debug_cast<mother_of_chaos_t*>( p() )->salvos <= 0 )
        make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
    }
  };

  void mother_of_chaos_t::arise()
  {
    warlock_pet_t::arise();

    salvos = 1;
  }

  action_t* mother_of_chaos_t::create_action( util::string_view name, util::string_view options_str )
  {
    if ( name == "chaos_salvo" )
      return new chaos_salvo_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }

  pit_lord_t::pit_lord_t( warlock_t* owner, util::string_view name )
    : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
  {
    npc_id = owner->hero.summon_pit_lord->effectN( 1 ).misc_value1();

    is_diabolist_guardian = true;
    affected_by.demonic_brutality = false;
    // NOTE: 2026-04-24 Diabolist guardians do not trigger Hellbent Commander (bug?)
    triggers.hellbent_commander_heartbeat &= !bugs;
    triggers.hellbent_commander_arise &= !bugs;
    triggers.hellbent_commander_demise &= !bugs;
    resource_regeneration = regen_type::DISABLED;

    action_list_str = "felseeker";
  }

  struct felseeker_tick_t : public warlock_pet_spell_t
  {
    felseeker_tick_t( warlock_pet_t* p )
      : warlock_pet_spell_t( "Felseeker (tick)", p, p->o()->hero.felseeker_dmg )
    {
      background = dual = true;
      aoe = -1;

      base_costs[ RESOURCE_ENERGY ] = 0.0;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_pet_spell_t::composite_da_multiplier( s ); // base value

      if ( p()->o()->demonology() )
      {
        // Added in build: 11.2.0.62253: reduces Diab Demons Damage by 20% for Demonology
        m *= 1.0 + p()->o()->hero.diabolic_ritual->effectN( 3 ).percent();
      }

      if ( p()->o()->destruction() )
      {
        // Added in build 11.2.0.62253: Increases Diab Demons damage by 15% for Destruction, missing from Patch Notes.
        m *= 1.0 + p()->o()->hero.diabolic_ritual->effectN( 4 ).percent();
      }

      return m;
    }
  };

  struct felseeker_t : public warlock_pet_spell_t
  {
    felseeker_t( warlock_pet_t* p )
      : warlock_pet_spell_t( "Felseeker", p, p->o()->hero.felseeker )
    {
      channeled = true;
      tick_zero = tick_on_application = false;
      tick_action = new felseeker_tick_t( p );
      base_costs_per_tick[ RESOURCE_ENERGY ] = p->o()->hero.felseeker_dmg->cost( POWER_ENERGY );
    }

    bool ready() override
    {
      if ( debug_cast<pit_lord_t*>( p() )->felseekers <= 0 )
        return false;

      return warlock_pet_spell_t::ready();
    }

    void execute() override
    {
      warlock_pet_spell_t::execute();

      debug_cast<pit_lord_t*>( p() )->felseekers--;
    }

    void last_tick( dot_t* d ) override
    {
      warlock_pet_spell_t::last_tick( d );

      if ( debug_cast<pit_lord_t*>( p() )->felseekers <= 0 )
        make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
    }
  };

  void pit_lord_t::arise()
  {
    warlock_pet_t::arise();

    felseekers = 1;
  }

  void pit_lord_t::init_base_stats()
  {
    warlock_pet_t::init_base_stats();

    resources.base[ RESOURCE_ENERGY ] = 99; // Fudge this so that Felseeker only does 4 ticks instead of an extra one at zero resources
  }

  action_t* pit_lord_t::create_action( util::string_view name, util::string_view options_str )
  {
    if ( name == "felseeker" )
      return new felseeker_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }

  /// Diabolic Ritual Demons End

  /// Infernal Fragment Begin

  infernal_fragment_t::infernal_fragment_t( warlock_t* owner, util::string_view name )
    : destruction::infernal_t( owner, name )
  {
    npc_id = owner->hero.infernal_fragmentation->effectN( 1 ).misc_value1();

    type = FRAG;
    owner_coeff.ap_from_sp = 1.5 * owner->hero.abyssal_dominion->effectN( 4 ).percent();
    owner_coeff.sp_from_sp = 1.5 * owner->hero.abyssal_dominion->effectN( 4 ).percent();
  }

  /// Infernal Fragment End

  /// Diabolic Imp Begin

  diabolic_imp_t::diabolic_imp_t( warlock_t* owner, util::string_view name )
    : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
  {
    npc_id = owner->hero.diabolic_imp->effectN( 1 ).misc_value1();

    resource_regeneration = regen_type::DISABLED;

    action_list_str = "diabolic_bolt";
  }

  struct diabolic_bolt_t : public warlock_pet_spell_t
  {
    diabolic_bolt_t( warlock_pet_t* p )
      : warlock_pet_spell_t( "Diabolic Bolt", p, p->o()->hero.diabolic_bolt )
    { base_costs[ RESOURCE_ENERGY ] = 0.0; }

    bool ready() override
    {
      if ( debug_cast<diabolic_imp_t*>( p() )->bolts <= 0 )
        return false;

      return warlock_pet_spell_t::ready();
    }

    void execute() override
    {
      warlock_pet_spell_t::execute();

      debug_cast<diabolic_imp_t*>( p() )->bolts--;

      if ( debug_cast<diabolic_imp_t*>( p() )->bolts <= 0 )
        make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
    }
  };

  void diabolic_imp_t::arise()
  {
    warlock_pet_t::arise();

    bolts = 5;
  }

  action_t* diabolic_imp_t::create_action( util::string_view name, util::string_view options_str )
  {
    if ( name == "diabolic_bolt" )
      return new diabolic_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }

  /// Diabolic Imp End
}  // namespace diabolist

namespace soul_harvester
{
/// Rampaging Demonic Soul Begin
struct soul_swipe_base_t : public warlock_pet_spell_t
{
  soul_swipe_base_t( std::string_view n, warlock_pet_t* p, const spell_data_t* s ) : warlock_pet_spell_t( n, p, s )
  {
    base_dd_multiplier *= 1.0 + p->o()->hero.eternal_hunger->effectN( 2 ).percent();
  }
};

struct soul_swipe_aoe_t : public soul_swipe_base_t
{
  soul_swipe_aoe_t( warlock_pet_t* p, std::string_view n = "soul_swipe_aoe" )
    : soul_swipe_base_t( n, p, p->find_spell( 1269049 ) )
  {
    spell_power_mod.direct = data().effectN( 2 ).sp_coeff();
    aoe                    = -1;
    background             = true;
    // NOTE: 2026-02-17: The AoE also seems to affect the main target (bug?)
    if ( !p->bugs )
      target_filter_callback = secondary_targets_only();
  }
};

struct soul_swipe_t : public soul_swipe_base_t
{
  soul_swipe_t( warlock_pet_t* p, std::string_view n ) : soul_swipe_base_t( n, p, p->find_spell( 1269049 ) )
  {
    // Actually just an auto attack with a 1s swing time. Simplifying the code doing it this way.
    trigger_gcd = 1_s;
    min_gcd = 0_s;

    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    aoe                    = 0; // Single target spell
    impact_action          = new soul_swipe_aoe_t( p );
    add_child( impact_action );
  }
};

rampaging_demonic_soul_t::rampaging_demonic_soul_t( warlock_t* owner, std::string_view name )
  : warlock_pet_t( owner, name, PET_WARLOCK_RANDOM, true )
{
  npc_id = owner->hero.manifested_avarice_spell->effectN( 1 ).misc_value1();

  resource_regeneration         = regen_type::DISABLED;
  affected_by.demonic_brutality = false;
  action_list_str               = "soul_swipe";
  owner_coeff.sp_from_sp        = 1.0;
  // NOTE: 2026-04-24 Demonic Soul do not trigger Hellbent Commander (bug?)
  triggers.hellbent_commander_heartbeat &= !bugs;
  triggers.hellbent_commander_arise  &= !bugs;
  triggers.hellbent_commander_demise &= !bugs;
}

void rampaging_demonic_soul_t::arise()
{
  warlock_pet_t::arise();
}

action_t* rampaging_demonic_soul_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "soul_swipe" )
    return new soul_swipe_t( this, name );

  return warlock_pet_t::create_action( name, options_str );
}
/// Rampaging Demonic Soul End

}  // namespace soul_harvester
}  // namespace pets
}  // namespace warlock
