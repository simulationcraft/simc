// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Pet
// ==========================================================================

// pet_t::pet_t =============================================================

void pet_t::init_pet_t_()
{
  target = owner -> target;
  level = owner -> level;
  full_name_str = owner -> name_str + "_" + name_str;

  pet_t** last = &( owner -> pet_list );
  while ( *last ) last = &( ( *last ) -> next_pet );
  *last = this;
  next_pet = 0;

  // Pets have inherent 5% critical strike chance if not overridden.
  base_spell_crit  = 0.05;
  base_attack_crit = 0.05;

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
              player_t( s, g ? PLAYER_GUARDIAN : PLAYER_PET, n ), owner( o ), next_pet( 0 ), summoned( false ), pet_type( PET_NONE )
{
  init_pet_t_();
}

pet_t::pet_t( sim_t*             s,
              player_t*          o,
              const std::string& n,
              pet_type_t         pt,
              bool               g ) :
              player_t( s, g ? PLAYER_GUARDIAN : PLAYER_PET, n ), owner( o ), next_pet( 0 ), summoned( false ), pet_type( pt )
{
  init_pet_t_();
}

// pet_t::create_action =====================================================

action_t* pet_t::create_action( const std::string& name,
                                const std::string& options_str )
{
  return player_t::create_action( name, options_str );
}

// pet_t::stamina ===========================================================

double pet_t::stamina() SC_CONST
{
  double a = composite_attribute_multiplier( ATTR_STAMINA ) * ( stamina_per_owner * owner -> stamina() );

  return player_t::stamina() + a;
}

// pet_t::intellect =========================================================

double pet_t::intellect() SC_CONST
{
  double a = composite_attribute_multiplier( ATTR_INTELLECT ) * ( intellect_per_owner * owner -> intellect() );

  return player_t::intellect() + a;
}

// player_t::id =============================================================

const char* pet_t::id()
{
  if ( id_str.empty() )
  {
    // create artifical unit ID, format type+subtype+id= TTTSSSSSSSIIIIII
    char buffer[ 1024 ];
    snprintf( buffer, sizeof( buffer ), "0xF140601FC5%06X,\"%s\",0x1111", index, name_str.c_str() );
    id_str = buffer;
  }

  return id_str.c_str();
}

// pet_t::init ==============================================================

void pet_t::init()
{
  player_t::init();
}

// pet_t::init_base =========================================================

void pet_t::init_base()
{
}

// pet_t::init_target =======================================================

void pet_t::init_target()
{
  if ( ! target_str.empty() )
    player_t::init_target();
  else
    target = owner -> target;
}

void pet_t::init_talents()
{
  specialization = primary_tab();
}
// pet_t::reset =============================================================

void pet_t::reset()
{
  player_t::reset();
  summon_time = 0;
}

// pet_t::summon ============================================================

void pet_t::summon( double duration )
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s summons %s. for %.2fs", owner -> name(), name(), duration );
  }

  distance = owner -> distance;

  owner -> active_pets++;

  summon_time = sim -> current_time;
  summoned = true;

  if( duration > 0 )
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p, double duration ) : event_t( sim, p )
      {
        sim -> add_event( this, duration );
      }

      virtual void execute()
      {
        if ( ! player -> sleeping ) player -> cast_pet() -> dismiss();
      }
    };
    new ( sim ) expiration_t( sim, this, duration );
  }

  arise();
}

// pet_t::dismiss ===========================================================

void pet_t::dismiss()
{
  if ( sim -> log ) log_t::output( sim, "%s dismisses %s", owner -> name(), name() );

  owner -> active_pets--;

  demise();
}

// pet_t::assess_damage ==================================================

double pet_t::assess_damage( double            amount,
                             const school_type school,
                             int               dmg_type,
                             int               result,
                             action_t*         action )
{
  if ( ! action )
    amount *= 0.10;
  else if ( action -> aoe )
    amount *= 0.10;

  return player_t::assess_damage( amount, school, dmg_type, result, action );
}

// pet_t::combat_begin ==================================================

void pet_t::combat_begin()
{
  // By default, only report statistics in the context of the owner
  quiet = ! sim -> report_pets_separately;

  player_t::combat_begin();
}
