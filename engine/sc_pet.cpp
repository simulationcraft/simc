// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Pet
// ==========================================================================

// pet_t::pet_t =============================================================

pet_t::pet_t( sim_t*             s,
              player_t*          o,
              const std::string& n,
              bool               g ) :
    player_t( s, g ? PLAYER_GUARDIAN : PLAYER_PET, n ), owner( o ), next_pet( 0 )
{
  level = owner -> level;
  full_name_str = owner -> name_str + "_" + name_str;
  next_pet = owner -> pet_list;
  owner -> pet_list = this;

  // Pets have inherent 5% critical strike chance if not overridden.
  base_spell_crit  = 0.05;
  base_attack_crit = 0.05;

  stamina_per_owner = 0.75;
  intellect_per_owner = 0.30;

  party = owner -> party;

  // By default, only report statistics in the context of the owner
  quiet = 1;
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
  double a = floor( composite_attribute_multiplier( ATTR_STAMINA ) * ( stamina_per_owner * owner -> stamina() ) );

  return player_t::stamina() + a;
}

// pet_t::intellect =========================================================

double pet_t::intellect() SC_CONST
{
  double a = floor( composite_attribute_multiplier( ATTR_INTELLECT ) * ( intellect_per_owner * owner -> intellect() ) );

  return player_t::intellect() + a;
}

// player_t::id =============================================================

const char* pet_t::id()
{
  if ( id_str.empty() )
  {
    // create artifical unit ID, format type+subtype+id= TTTSSSSSSSIIIIII
    char buffer[ 1024 ];
    sprintf( buffer, "0xF140601FC5%06X,\"%s\",0x1111", index, name_str.c_str() );
    id_str = buffer;
  }

  return id_str.c_str();
}

// pet_t::init ==============================================================

void pet_t::init()
{
  player_t::init();
  level = owner -> level;
}

// pet_t::init_base =========================================================

void pet_t::init_base()
{
}

// pet_t::reset =============================================================

void pet_t::reset()
{
  player_t::reset();
  sleeping = 1;
  summon_time = 0;
}

// pet_t::summon ============================================================

void pet_t::summon( double duration )
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s summons %s.", owner -> name(), name() );
    log_t::summon_event( this );
  }

  distance = owner -> distance;
  sleeping = 0;
  init_resources( true );
  summon_time = sim -> current_time;

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
	player -> cast_pet() -> dismiss();
      }
    };
    new ( sim ) expiration_t( sim, this, duration );
  }

  schedule_ready();
}

// pet_t::dismiss ===========================================================

void pet_t::dismiss()
{
  if ( sim -> log ) log_t::output( sim, "%s dismisses %s", owner -> name(), name() );

  readying = 0;
  sleeping = 1;

  for ( action_t* a = action_list; a; a = a -> next )
  {
    a -> cancel();
  }

  for( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> expire();
  }

  sim -> cancel_events( this );
}
