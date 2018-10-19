// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace {
struct expiration_t : public event_t
{
  pet_t& pet;

  expiration_t( pet_t& p, timespan_t duration ) :
    event_t( p, duration ),
    pet( p )
  {
  }

  virtual const char* name() const override
  { return "pet_expiration"; }

  virtual void execute() override
  {
    pet.expiration = nullptr;

    if ( ! pet.is_sleeping() )
      pet.dismiss( true );
  }
};
}

pet_t::pet_t( sim_t*             sim,
              player_t*          owner,
              const std::string& name,
              bool               guardian,
              bool               dynamic ) :
  pet_t( sim, owner, name, PET_NONE, guardian, dynamic )
{
}

pet_t::pet_t( sim_t*             sim,
              player_t*          owner,
              const std::string& name,
              pet_e              type,
              bool               guardian,
              bool               dynamic ) :
  player_t( sim, type == PET_ENEMY ? ENEMY_ADD : guardian ? PLAYER_GUARDIAN : PLAYER_PET, name, RACE_NONE ),
  full_name_str( owner -> name_str + '_' + name_str ),
  owner( owner ),
  stamina_per_owner( 0.75 ),
  intellect_per_owner( 0.30 ),
  summoned( false ),
  dynamic( dynamic ),
  pet_type( type ),
  expiration( nullptr ),
  duration( timespan_t::zero() ),
  affects_wod_legendary_ring( true ),
  owner_coeff()
{
  target = owner -> target;
  true_level = owner -> true_level;

  owner -> pet_list.push_back( this );

  party = owner -> party;
  regen_type = owner -> regen_type;

  // Inherit owner's dbc state
  dbc.ptr = owner -> dbc.ptr;

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

  if ( owner -> buffs.legendary_aoe_ring && owner -> buffs.legendary_aoe_ring -> up() )
    m *= 1.0 + owner -> buffs.legendary_aoe_ring -> default_value;

  return m;
}

void pet_t::init()
{
  player_t::init();

  if ( regen_type == REGEN_DYNAMIC && owner -> regen_type == REGEN_DISABLED )
  {
    sim -> errorf( "Pet %s has dynamic regen, while owner has disabled regen. Disabling pet regeneration also.", name() );
    regen_type = REGEN_DISABLED;
  }
}

void pet_t::init_base_stats()
{
  // Pets have inherent 5% critical strike chance if not overridden.
  base.spell_crit_chance  = 0.05;
  base.attack_crit_chance = 0.05;
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
  sim -> print_log( "{} summons {} for {}.", owner -> name(), name(), summon_duration );

  current.distance = owner -> current.distance;

  // Add to active_pets
  auto it = range::find( owner -> active_pets, this );
  if ( it != owner -> active_pets.end() )
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
  sim -> print_log( "{} dismisses {}", owner -> name(), name() );

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
    sim -> print_debug( "Creating Auras, Buffs, and Debuffs for pet '{}'.", name() );

    buffs.stunned = make_buff( this, "stunned" )
        ->set_max_stack( 1 );
    buffs.movement = new movement_buff_t( this );

    debuffs.casting = make_buff( this, "casting" )
        ->set_max_stack( 1 )
        ->set_quiet( 1 );
  }
}

void pet_t::assess_damage( school_e       school,
                           dmg_e          type,
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

const spell_data_t* pet_t::find_pet_spell( const std::string& name )
{
  unsigned spell_id = dbc.pet_ability_id( type, name.c_str() );

  if ( ! spell_id || ! dbc.spell( spell_id ) )
  {
    if ( ! owner )
      return spell_data_t::not_found();

    return owner -> find_pet_spell( name );
  }

  return dbc::find_spell( this, spell_id );
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
    ap += owner -> composite_melee_attack_power( owner -> default_ap_type() ) *
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
