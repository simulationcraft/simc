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
  level = owner -> level;
  full_name_str = owner -> name_str + '_' + name_str;
  expiration = 0;
  duration = timespan_t::zero();

  owner -> pet_list.push_back( this );

  // Pets have inherent 5% critical strike chance if not overridden.
  base.spell_crit  = 0.05;
  base.attack_crit = 0.05;

  stamina_per_owner = 0.75;
  intellect_per_owner = 0.30;

  party = owner -> party;

  // Inherit owner's dbc state
  dbc.ptr = owner -> dbc.ptr;
}
pet_t::pet_t( sim_t*             s,
              player_t*          o,
              const std::string& n,
              bool               g ) :
  player_t( s, g ? PLAYER_GUARDIAN : PLAYER_PET, n ),
  owner( o ), summoned( false ), pet_type( PET_NONE ),
  owner_coeff( owner_coefficients_t() )
{
  init_pet_t_();
}

pet_t::pet_t( sim_t*             s,
              player_t*          o,
              const std::string& n,
              pet_e         pt,
              bool               g ) :
  player_t( s, pt == PET_ENEMY ? ENEMY_ADD : g ? PLAYER_GUARDIAN : PLAYER_PET, n ),
  owner( o ), summoned( false ), pet_type( pt ),
  owner_coeff( owner_coefficients_t() )
{
  init_pet_t_();
}

// base_t::pet_attribute =================================

double pet_t::composite_attribute( attribute_e attr )
{
  double a = current.attribute[ attr ];

  switch ( attr )
  {
  case ATTR_INTELLECT:
    a += intellect_per_owner * owner -> intellect();
    break;
  case ATTR_STAMINA:
    a += stamina_per_owner * owner -> stamina();
    break;
  default:
    break;
  }

  return a;
}

// pet_t::init_base =========================================================

void pet_t::init_base()
{
  //mp5_per_spirit = dbc.mp5_per_spirit( pet_type, level );
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

  expiration = 0;
}

// pet_t::summon ============================================================

void pet_t::summon( timespan_t summon_duration )
{
  if ( sim -> log )
  {
    sim -> output( "%s summons %s. for %.2fs", owner -> name(), name(), summon_duration.total_seconds() );
  }

  current.distance = owner -> current.distance;

  owner -> active_pets++;

  summoned = true;

  // Take care of remaining expiration
  if ( expiration )
  {
    event_t::cancel( expiration );
    expiration = 0;
  }

  if ( summon_duration > timespan_t::zero() )
  {
    duration = summon_duration;
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, pet_t* p, timespan_t duration ) :
        event_t( sim, p )
      {
        sim -> add_event( this, duration );
      }

      virtual void execute()
      {
        player -> cast_pet() -> expiration = 0;
        if ( ! player -> current.sleeping ) player -> cast_pet() -> dismiss();
      }
    };
    expiration = new ( sim ) expiration_t( sim, this, summon_duration );
  }

  arise();

  owner -> trigger_ready();
}

// pet_t::dismiss ===========================================================

void pet_t::dismiss()
{
  if ( sim -> log ) sim -> output( "%s dismisses %s", owner -> name(), name() );

  owner -> active_pets--;

  if ( expiration )
  {
    event_t::cancel( expiration );
    expiration = 0;
  }

  duration = timespan_t::zero();

  demise();
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
  if ( !quiet )
    quiet = ! sim -> report_pets_separately;

  base_t::combat_begin();
}

// pet_t::find_pet_spell =============================================

const spell_data_t* pet_t::find_pet_spell( const std::string& name, const std::string& token )
{
  unsigned spell_id = dbc.pet_ability_id( type, name.c_str() );

  if ( ! spell_id || ! dbc.spell( spell_id ) )
  {
    if ( ! owner )
      return spell_data_t::not_found();

    return owner -> find_pet_spell( name, token );
  }

  dbc_t::add_token( spell_id, token, dbc.ptr );

  return ( dbc.spell( spell_id ) );
}

void pet_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.initial[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * owner_coeff.health;

  resources.current = resources.max = resources.initial;
}

double pet_t::hit_exp()
{
  double e = owner -> composite_attack_expertise( &( owner -> main_hand_weapon ) );

  if ( owner -> off_hand_weapon.damage > 0 )
    e = std::max( e, owner -> composite_attack_expertise( &( owner -> off_hand_weapon ) ) );

  return ( e + owner -> composite_attack_hit() ) * 0.50; // attack and spell hit are equal in MoP
}

double pet_t::pet_crit()
{
  double c = owner -> composite_attack_crit( &( owner -> main_hand_weapon ) );

  if ( owner -> off_hand_weapon.damage > 0 )
    c = std::max( c, owner -> composite_attack_crit( &( owner -> off_hand_weapon ) ) );

  return std::max( c, owner -> composite_spell_crit() );
}

double pet_t::composite_attack_power()
{
  double ap = 0;
  if ( owner_coeff.ap_from_ap > 0.0 )
    ap += owner -> composite_attack_power() * owner -> composite_attack_power_multiplier() * owner_coeff.ap_from_ap;
  if ( owner_coeff.ap_from_sp > 0.0 )
    ap += owner -> composite_spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier() * owner_coeff.ap_from_sp;
  return ap;
}

double pet_t::composite_spell_power( school_e school )
{
  double sp = 0;
  if ( owner_coeff.sp_from_ap > 0.0 )
    sp += owner -> composite_attack_power() * owner -> composite_attack_power_multiplier() * owner_coeff.sp_from_ap;
  if ( owner_coeff.sp_from_sp > 0.0 )
    sp += owner -> composite_spell_power( school ) * owner -> composite_spell_power_multiplier() * owner_coeff.sp_from_sp;
  return sp;
}
