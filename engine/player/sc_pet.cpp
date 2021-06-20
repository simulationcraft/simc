// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "pet.hpp"

#include "action/sc_action.hpp"
#include "action/sc_action_state.hpp"
#include "buff/sc_buff.hpp"
#include "dbc/dbc.hpp"
#include "player/actor_target_data.hpp"
#include "player/spawner_base.hpp"
#include "sim/event.hpp"
#include "sim/sc_sim.hpp"

namespace {
struct expiration_t : public event_t
{
  pet_t& pet;
  
  expiration_t( pet_t& p, timespan_t duration ) :
    event_t( p, duration ),
    pet( p )
  {
  }

  const char* name() const override
  { return "pet_expiration"; }

  void execute() override
  {
    pet.expiration = nullptr;

    if ( ! pet.is_sleeping() )
      pet.dismiss( true );
  }
};
}

pet_t::pet_t( sim_t*             sim,
              player_t*          owner,
              util::string_view name,
              bool               guardian,
              bool               dynamic ) :
  pet_t( sim, owner, name, PET_NONE, guardian, dynamic )
{
}

pet_t::pet_t( sim_t* sim, player_t* owner, util::string_view name, pet_e pet_type, bool guardian, bool dynamic )
  : player_t( sim, pet_type == PET_ENEMY ? ENEMY_ADD : guardian ? PLAYER_GUARDIAN : PLAYER_PET, name, RACE_NONE ),
    full_name_str( owner->name_str + '_' + name_str ),
    owner( owner ),
    stamina_per_owner( 0.75 ),
    intellect_per_owner( 0.30 ),
    summoned( false ),
    dynamic( dynamic ),
    affects_wod_legendary_ring( true ),
    pet_type( pet_type ),
    expiration( nullptr ),
    duration( timespan_t::zero() ),
    npc_id(),
    owner_coeff()
{
  target = owner -> target;
  true_level = owner -> true_level;

  owner -> pet_list.push_back( this );

  party = owner -> party;
  resource_regeneration = owner ->resource_regeneration;

  // Inherit owner's dbc state
  dbc->ptr = owner -> dbc->ptr;
  dbc_override = owner -> dbc_override;

  // Set pet dps data collection to level 2 or higher, so our 32bit GUI users can at least
  // do scale factor simulations with default settings.
  if ( sim -> statistics_level < 2 )
    collected_data.dps.change_mode( true );
}

double pet_t::composite_attribute( attribute_e attr ) const
{
  double a = current.stats.attribute[ attr ];

  switch ( attr )
  {
    case ATTR_INTELLECT:
      a += intellect_per_owner * owner -> cache.intellect();
      break;
    case ATTR_STAMINA:
      a += stamina_per_owner * owner -> cache.stamina();
      break;
    default:
      break;
  }

  return a;
}

double pet_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( owner -> buffs.legendary_aoe_ring && owner -> buffs.legendary_aoe_ring -> check() )
    m *= 1.0 + owner -> buffs.legendary_aoe_ring -> default_value;

  return m;
}

double pet_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  // Same logic as in player_t::composite_player_target_multiplier() above
  // As the Covenant buff isn't created on pets, we need to check the owner
  // Testing shows this appears to work on all pets, even trinkets and such
  if ( owner->buffs.wild_hunt_tactics )
  {
    double health_threshold = 100.0 - ( 100.0 - owner->buffs.wild_hunt_tactics->data().effectN( 5 ).base_value() ) * sim->shadowlands_opts.wild_hunt_tactics_duration_multiplier;
    if ( target->health_percentage() > health_threshold )
      m *= 1.0 + owner->buffs.wild_hunt_tactics->default_value;
  }

  if ( auto td = owner->find_target_data( target ) )
  {
    m *= 1.0 + td->debuff.condensed_lifeforce->check_value();

    // These seem to apply only to "main" pets
    if ( type == PLAYER_PET )
    {
      m *= 1 + td->debuff.adversary->check_value();
      m *= 1 + td->debuff.plagueys_preemptive_strike->check_value();
    }
  }

  return m;
}

void pet_t::init()
{
  player_t::init();

  if (resource_regeneration == regen_type::DYNAMIC && owner ->resource_regeneration == regen_type::DISABLED )
  {
    sim -> error( "{} has dynamic regen, while owner has disabled regen. Disabling pet regeneration also.", *this );
    resource_regeneration = regen_type::DISABLED;
  }
}

void pet_t::init_base_stats()
{
  // Pets have inherent 5% critical strike chance if not overridden.
  base.spell_crit_chance  = 0.05;
  base.attack_crit_chance = 0.05;

  base.armor_coeff = dbc->armor_mitigation_constant( level() );
  sim -> print_debug( "{} base armor coefficient set to {}.", *this, base.armor_coeff );
}

void pet_t::init_target()
{
  if ( ! target_str.empty() )
    base_t::init_target();
  else
    target = owner -> target;
}

void pet_t::reset()
{
  base_t::reset();

  expiration = nullptr;
}

void pet_t::summon( timespan_t summon_duration )
{
  sim -> print_log( "{} summons {} for {}.", *owner, name_str, summon_duration );

  current.distance = owner -> current.distance;

  // Add to active_pets
  auto it = range::find( owner -> active_pets, this );
  if ( it == owner -> active_pets.end() )
    owner -> active_pets.push_back( this );

  summoned = true;

  // Take care of remaining expiration
  if ( expiration )
  {
    event_t::cancel( expiration );
    expiration = nullptr;
  }

  if ( summon_duration > timespan_t::zero() )
  {
    duration = summon_duration;
    expiration = make_event<expiration_t>( *sim, *this, summon_duration );
  }

  arise();

  owner -> trigger_ready();
}

void pet_t::dismiss( bool expired )
{
  sim -> print_log( "{} dismisses {}", *owner, name_str );

  // Remove from active_pets list
  auto it = range::find( owner -> active_pets, this );
  if ( it != owner -> active_pets.end() )
    erase_unordered( owner -> active_pets, it );

  if ( expiration && ! expired )
  {
    event_t::cancel( expiration );
    expiration = nullptr;
  }

  duration = timespan_t::zero();

  demise();
}

/**
 * Specialize pet "player-scope" options.
 *
 * Realistically speaking, anything else than adjusting the pet APL seems somewhat pointless.
 * Class modules are still free to override the pet's create_options() to add to the list.
 */
void pet_t::create_options()
{
  add_option( opt_string( "actions", action_list_str ) );
  add_option( opt_append( "actions+", action_list_str ) );
  add_option( opt_map( "actions.", alist_map ) );
  add_option( opt_string( "action_list", choose_action_list ) );
}

/**
 * Create pet buffs.
 *
 * Create the bare necessity of buffs for pets, skipping anything that's done in player_t::create_buffs.
 */
void pet_t::create_buffs()
{
  if ( is_enemy() )
  {
    player_t::create_buffs();
  }
  else
  {
    sim->print_debug( "Creating Auras, Buffs, and Debuffs for {}.", *this );

    buffs.stunned  = make_buff( this, "stunned" )
      ->set_max_stack( 1 );
    buffs.movement = new movement_buff_t( this );

    // Blood of the Enemy Essence Major R3 increase crit damage buff
    buffs.seething_rage = make_buff( this, "seething_rage", find_spell( 297126 ) )
      ->set_default_value( find_spell( 297126 )->effectN( 1 ).percent() );

    debuffs.casting = make_buff( this, "casting" )
      ->set_max_stack( 1 )
      ->set_quiet( true );
  }
}

void pet_t::adjust_duration( timespan_t adjustment )
{
  if ( !expiration || adjustment == 0_ms )
  {
    return;
  }

  auto new_duration = expiration->remains() + adjustment;
  if ( new_duration <= 0_ms )
  {
    sim->print_debug( "{} pet {} duration adjusted to {}, dismissing ...",
      *owner, name_str, new_duration );
    dismiss();
  }
  else
  {
    duration += adjustment;

    sim->print_debug( "{} pet {} duration adjusted to {}",
      *owner, name_str, new_duration );

    if ( new_duration > expiration->remains() )
    {
      expiration->reschedule( new_duration );
    }
    else
    {
      event_t::cancel( expiration );
      expiration = make_event<expiration_t>( *sim, *this, new_duration );
    }
  }
}

void pet_t::assess_damage( school_e       school,
                           result_amount_type          type,
                           action_state_t* s )
{
  if ( ! is_add() && ( ! s -> action || s -> action -> aoe ) )
    s -> result_amount *= 0.10;

  return base_t::assess_damage( school, type, s );
}

void pet_t::init_finished()
{
  player_t::init_finished();

  // By default, only report statistics in the context of the owner
  if ( ! quiet )
    quiet = ! sim -> report_pets_separately;
}

const spell_data_t* pet_t::find_pet_spell( util::string_view name )
{
  unsigned spell_id = dbc->pet_ability_id( type, name );

  if ( ! spell_id || ! dbc->spell( spell_id ) )
  {
    if ( ! owner )
      return spell_data_t::not_found();

    return owner -> find_pet_spell( name );
  }

  return dbc::find_spell( this, spell_id );
}

bool pet_t::requires_data_collection() const
{
  return active_during_iteration || ( dynamic && sim -> report_pets_separately == 1 );
}

void pet_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.initial[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * owner_coeff.health;

  resources.current = resources.max = resources.initial;
}

double pet_t::hit_exp() const
{
  double h_e = owner -> cache.attack_hit();

  h_e += owner -> cache.attack_expertise();

  return h_e * 0.50; // attack and spell hit are equal in MoP
}

double pet_t::pet_crit() const
{
  return std::max( owner -> cache.attack_crit_chance(), owner -> cache.spell_crit_chance() );
}

double pet_t::composite_melee_attack_power() const
{
  double ap = 0;

  if ( owner_coeff.ap_from_ap > 0.0 )
  {
    // Use owner's default attack power type for the inheritance
    ap += owner -> composite_melee_attack_power_by_type( owner -> default_ap_type() ) *
          owner -> composite_attack_power_multiplier() *
          owner_coeff.ap_from_ap;
  }

  if ( owner_coeff.ap_from_sp > 0.0 )
    ap += owner -> cache.spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier() * owner_coeff.ap_from_sp;

  return ap;
}

double pet_t::composite_spell_power( school_e school ) const
{
  double sp = 0;

  if ( owner_coeff.sp_from_ap > 0.0 )
    sp += owner -> cache.attack_power() * owner -> composite_attack_power_multiplier() * owner_coeff.sp_from_ap;

  if ( owner_coeff.sp_from_sp > 0.0 )
    sp += owner -> cache.spell_power( school ) * owner -> composite_spell_power_multiplier() * owner_coeff.sp_from_sp;

  return sp;
}

double pet_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s );

  m *= 1.0 + owner -> racials.brawn -> effectN( 1 ).percent();
  m *= 1.0 + owner -> racials.might_of_the_mountain -> effectN( 1 ).percent();

  return m;
}

timespan_t pet_t::composite_active_time() const
{
  // Only dynamic spawns need to consider a non-standard active time, since persistent pets can be
  // presumed to be active for the duration of the iteration (or at least the remaining duration of
  // the iteration, starting from their summoning time).
  if ( ! spawner || sim -> report_pets_separately )
  {
    return player_t::composite_active_time();
  }

  // TODO: Binds pet base name and the spawner base name together, should this be changed?
  auto ptr = owner -> find_spawner( name_str );
  if ( ! ptr )
  {
    return player_t::composite_active_time();
  }

  return ptr -> iteration_uptime();
}

void pet_t::acquire_target( retarget_source event, player_t* context )
{
  player_t::acquire_target( event, context );

  // Dynamic pets have to retarget all actions at this point, if they arise during iteration
  if ( dynamic && event == retarget_source::SELF_ARISE )
  {
    range::for_each( action_list, [this, event, context]( action_t* action ) {
      action->acquire_target( event, context, target );
    } );
  }
}

void format_to( const pet_t& pet, fmt::format_context::iterator out )
{
  fmt::format_to( out, "Pet '{}'", pet.full_name_str );
}
