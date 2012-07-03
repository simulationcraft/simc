// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Pet
// ==========================================================================

// pet_t::pet_t =============================================================

void pet_t::init_pet_t_()
{
  target = owner -> target;
  level = owner -> level;
  full_name_str = owner -> name_str + '_' + name_str;
  expiration = 0;

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
  coeff( owner_coefficients_t() )
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
  coeff( owner_coefficients_t() )
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

void pet_t::summon( timespan_t duration )
{
  if ( sim -> log )
  {
    sim -> output( "%s summons %s. for %.2fs", owner -> name(), name(), duration.total_seconds() );
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

  if ( duration > timespan_t::zero() )
  {
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
    expiration = new ( sim ) expiration_t( sim, this, duration );
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

  demise();
}

// pet_t::assess_damage =====================================================

double pet_t::assess_damage( double              amount,
                             school_e       school,
                             dmg_e          type,
                             result_e       result,
                             action_t*           action )
{
  if ( ! action || action -> aoe )
    amount *= 0.10;

  return base_t::assess_damage( amount, school, type, result, action );
}

// pet_t::combat_begin ======================================================

void pet_t::combat_begin()
{
  // By default, only report statistics in the context of the owner
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

  resources.initial[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * coeff.health;

  resources.current = resources.max = resources.initial;
}
