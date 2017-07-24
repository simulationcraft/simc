// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Pet
// ==========================================================================
pet_t::owner_coefficients_t::owner_coefficients_t() :
  armor ( 1.0 ),
  health( 1.0 ),
  ap_from_ap( 0.0 ),
  ap_from_sp( 0.0 ),
  sp_from_ap( 0.0 ),
  sp_from_sp( 0.0 )
{}

// pet_t::pet_t =============================================================

void pet_t::init_pet_t_()
{
  target = owner -> target;
  true_level = owner -> true_level;
  full_name_str = owner -> name_str + '_' + name_str;
  expiration = nullptr;
  duration = timespan_t::zero();
  affects_wod_legendary_ring = true;

  owner -> pet_list.push_back( this );

  stamina_per_owner = 0.75;
  intellect_per_owner = 0.30;

  party = owner -> party;
  regen_type = owner -> regen_type;

  // Inherit owner's dbc state
  dbc.ptr = owner -> dbc.ptr;

  // Set pet dps data collection to level 2 or higher, so our 32bit GUI users can at least
  // do scale factor simulations with default settings.
  if ( sim -> statistics_level < 2 )
    collected_data.dps.change_mode( true );
}

pet_t::pet_t( sim_t*             s,
              player_t*          o,
              const std::string& n,
              bool               g,
              bool               d ) :
  player_t( s, g ? PLAYER_GUARDIAN : PLAYER_PET, n, RACE_NONE ),
  owner( o ), summoned( false ), dynamic( d ), pet_type( PET_NONE ),
  owner_coeff( owner_coefficients_t() )
{
  init_pet_t_();
}

pet_t::pet_t( sim_t*             s,
              player_t*          o,
              const std::string& n,
              pet_e              pt,
              bool               g,
              bool               d ) :
  player_t( s, pt == PET_ENEMY ? ENEMY_ADD : g ? PLAYER_GUARDIAN : PLAYER_PET, n, RACE_NONE ),
  owner( o ), summoned( false ), dynamic( d ), pet_type( pt ),
  owner_coeff( owner_coefficients_t() )
{
  init_pet_t_();
}

// base_t::pet_attribute ====================================================

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

// pet_t::composite_player_multiplier =======================================

double pet_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( owner -> buffs.legendary_aoe_ring && owner -> buffs.legendary_aoe_ring -> up() )
    m *= 1.0 + owner -> buffs.legendary_aoe_ring -> default_value;

  return m;
}

// pet_t::init ==============================================================

void pet_t::init()
{
  player_t::init();

  if ( regen_type == REGEN_DYNAMIC && owner -> regen_type == REGEN_DISABLED )
  {
    sim -> errorf( "Pet %s has dynamic regen, while owner has disabled regen. Disabling pet regeneration also.", name() );
    regen_type = REGEN_DISABLED;
  }
}

// pet_t::init_base =========================================================

void pet_t::init_base_stats()
{
  //mp5_per_spirit = dbc.mp5_per_spirit( pet_type, level );

  // Pets have inherent 5% critical strike chance if not overridden.
  base.spell_crit_chance  = 0.05;
  base.attack_crit_chance = 0.05;

}

// pet_t::init_target =======================================================

void pet_t::init_target()
{
  if ( ! target_str.empty() )
    base_t::init_target();
  else
    target = owner -> target;
}

// pet_t::reset =============================================================

void pet_t::reset()
{
  base_t::reset();

  expiration = nullptr;
}

// pet_t::summon ============================================================

void pet_t::summon( timespan_t summon_duration )
{
  if ( sim -> log )
  {
    sim -> out_log.printf( "%s summons %s. for %.2fs", owner -> name(), name(), summon_duration.total_seconds() );
  }

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
    struct expiration_t : public event_t
    {
      pet_t& pet;
      expiration_t( pet_t& p, timespan_t duration ) :
        event_t( p, duration ),
        pet( p )
      {
      }
      virtual const char* name() const override
      { return "pet_summon_duration"; }
      virtual void execute() override
      {
        pet.expiration = nullptr;
        if ( ! pet.is_sleeping() )
          pet.dismiss( true );
      }
    };
    expiration = make_event<expiration_t>( *sim, *this, summon_duration );
  }

  arise();

  owner -> trigger_ready();
}

// pet_t::dismiss ===========================================================

void pet_t::dismiss( bool expired )
{
  if ( sim -> log ) sim -> out_log.printf( "%s dismisses %s", owner -> name(), name() );

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

// pet_t::create_options ====================================================

// Specialize pet "player-scope" options. Realistically speaking, anything else than adjusting the
// pet APL seems somewhat pointless. Class modules are still free to override the pet's
// create_options() to add to the list.
void pet_t::create_options()
{
  add_option( opt_string( "actions", action_list_str ) );
  add_option( opt_append( "actions+", action_list_str ) );
  add_option( opt_map( "actions.", alist_map ) );
  add_option( opt_string( "action_list", choose_action_list ) );
}

// pet_t::create_buffs ======================================================

// Create the bare necessity of buffs for pets, skipping anything that's done in
// player_t::create_buffs.

void pet_t::create_buffs()
{
  if ( sim -> debug )
    sim -> out_debug.printf( "Creating Auras, Buffs, and Debuffs for player (%s)", name() );

  buffs.stunned = buff_creator_t( this, "stunned" ).max_stack( 1 );
  buffs.movement = buff_creator_t( this, "movement" );

  debuffs.casting = buff_creator_t( this, "casting" ).max_stack( 1 ).quiet( 1 );
}

// pet_t::assess_damage =====================================================

void pet_t::assess_damage( school_e       school,
                           dmg_e          type,
                           action_state_t* s )
{
  if ( ! is_add() && ( ! s -> action || s -> action -> aoe ) )
    s -> result_amount *= 0.10;

  return base_t::assess_damage( school, type, s );
}

// pet_t::combat_begin ======================================================

void pet_t::combat_begin()
{
  // By default, only report statistics in the context of the owner
  if ( ! quiet )
    quiet = ! sim -> report_pets_separately;

  base_t::combat_begin();
}

// pet_t::find_pet_spell ====================================================

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
  return std::max( owner -> cache.attack_crit_chance(),
                   owner -> cache.spell_crit_chance() );
}

double pet_t::composite_melee_attack_power() const
{
  double ap = 0;
  if ( owner_coeff.ap_from_ap > 0.0 )
    ap += owner -> cache.attack_power() * owner -> composite_attack_power_multiplier() * owner_coeff.ap_from_ap;
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
