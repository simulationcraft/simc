// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Raid Events
// ==========================================================================

struct adds_event_t : public raid_event_t
{
  int count;
  double health;
  adds_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "adds" ), count( 1 ), health( 100000 )
  {
    option_t options[] =
    {
      { "count", OPT_INT, &count },
      { "health", OPT_FLT, &health },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    for ( pet_t* pet = sim -> target -> pet_list; pet; pet = pet -> next_pet )
    {
      pet -> resource_base[ RESOURCE_HEALTH ] = health;
    }
    if ( count > sim -> target_adds )
    {
      sim -> target_adds = count;
    }
  }

  virtual void start()
  {
    raid_event_t::start();
    int i = 0;
    for ( pet_t* pet = sim -> target -> pet_list; pet; pet = pet -> next_pet )
    {
      if ( i >= count ) continue;
      pet -> summon();
      i++;
    }
  }

  virtual void finish()
  {
    int i = 0;
    for ( pet_t* pet = sim -> target -> pet_list; pet; pet = pet -> next_pet )
    {
      if ( i >= count ) continue;
      pet -> dismiss();
      i++;
    }
    raid_event_t::finish();
  }
};

// Casting ==================================================================

struct casting_event_t : public raid_event_t
{
  casting_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "casting" )
  {
    parse_options( NULL, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();
    sim -> target -> debuffs.casting -> increment();
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> sleeping ) continue;
      p -> interrupt();
    }
  }

  virtual void finish()
  {
    sim -> target -> debuffs.casting -> decrement();
    raid_event_t::finish();
  }
};

// Distraction ==============================================================

struct distraction_event_t : public raid_event_t
{
  double skill;

  distraction_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "distraction" ), skill( 0.2 )
  {
    option_t options[] =
    {
      { "skill", OPT_FLT, &skill },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();
    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      p -> skill -= skill;
    }
  }

  virtual void finish()
  {
    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      p -> skill += skill;
    }
    raid_event_t::finish();
  }
};

// Invulnerable =============================================================

struct invulnerable_event_t : public raid_event_t
{
  invulnerable_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "invulnerable" )
  {
    parse_options( NULL, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();
    sim -> target -> debuffs.invulnerable -> increment();
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> sleeping ) continue;
      p -> in_combat = true; // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p -> halt();
      p -> clear_debuffs(); // FIXME! this is really just clearing DoTs at the moment
    }
  }

  virtual void finish()
  {
    sim -> target -> debuffs.invulnerable -> decrement();
    if ( ! sim -> target -> debuffs.invulnerable -> check() )
    {
      // FIXME! restoring optimal_raid target debuffs?
    }
    raid_event_t::finish();
  }
};

// Flying =============================================================

struct flying_event_t : public raid_event_t
{
  flying_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "flying" )
  {
    parse_options( NULL, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();
    sim -> target -> debuffs.flying -> increment();
  }

  virtual void finish()
  {
    sim -> target -> debuffs.flying -> decrement();
    raid_event_t::finish();
  }
};


// Movement =================================================================

struct movement_event_t : public raid_event_t
{
  double move_to;
  double move_distance;
  int players_only;
  bool is_distance;

  movement_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "movement" ), move_to( -2 ), move_distance( 0 ), players_only( 0 ), is_distance( 0 )
  {
    option_t options[] =
    {
      { "to",           OPT_FLT,  &move_to       },
      { "distance",     OPT_FLT,  &move_distance },
      { "players_only", OPT_BOOL, &players_only  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    is_distance = move_distance || ( move_to >= -1 && duration == timespan_t::zero );
    if ( is_distance ) name_str = "movement_distance";
  }

  virtual void start()
  {
    raid_event_t::start();
    int num_affected = ( int )affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      if ( p -> is_pet() && players_only ) continue;
      double my_duration;
      if ( is_distance )
      {
        double my_move_distance = move_distance;
        if ( move_to >= -1 )
        {
          double new_distance = ( move_to < 0 ) ? p -> default_distance : move_to;
          if ( ! my_move_distance )
            my_move_distance = abs( new_distance - p -> distance );
          p -> distance = new_distance;
        }
        my_duration = my_move_distance / p -> composite_movement_speed();
      }
      else
      {
        my_duration = saved_duration.total_seconds();
      }
      if ( my_duration > 0 )
      {
        p -> buffs.raid_movement -> buff_duration = timespan_t::from_seconds(my_duration);
        p -> buffs.raid_movement -> trigger();
      }
      if ( p -> sleeping ) continue;
      if ( p -> buffs.stunned -> check() ) continue;
      p -> in_combat = true; // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p -> moving();
    }
  }
};

// Stun =====================================================================

struct stun_event_t : public raid_event_t
{
  stun_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "stun" )
  {
    parse_options( NULL, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();
    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      if ( p -> sleeping ) continue;
      p -> buffs.stunned -> increment();
      p -> in_combat = true; // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p -> stun();
    }
  }

  virtual void finish()
  {
    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      if ( p -> sleeping ) continue;
      p -> buffs.stunned -> decrement();
      if ( ! p -> buffs.stunned -> check() )
      {
        // Don't schedule_ready players who are already working, like pets auto-summoned during the stun event ( ebon imp ).
        if ( ! p -> channeling && ! p -> executing && ! p -> readying && ! p -> sleeping )
          p -> schedule_ready();
      }
    }

    raid_event_t::finish();
  }
};

// Interrupt ================================================================

struct interrupt_event_t : public raid_event_t
{
  interrupt_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "interrupt" )
  {
    parse_options( NULL, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();
    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      if ( p -> sleeping ) continue;
      p -> interrupt();
    }
  }
};

// Damage ===================================================================

struct damage_event_t : public raid_event_t
{
  double amount;
  double amount_range;
  spell_t* raid_damage;

  damage_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "damage" ), amount( 1 ), amount_range( 0 ), raid_damage( 0 )
  {
    std::string type_str = "holy";
    option_t options[] =
    {
      { "amount",        OPT_FLT, &amount        },
      { "amount_range",  OPT_FLT, &amount_range  },
      { "type",          OPT_STRING, &type_str   },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

	assert( duration == timespan_t::zero );

    name_str = "raid_damage_" + type_str;

    struct raid_damage_t : public spell_t
    {
      raid_damage_t( const char* n, player_t* player, const school_type s ) :
        spell_t( n, player, RESOURCE_NONE, s )
      {
        may_crit = false;
        background = true;
        trigger_gcd = timespan_t::zero;
      }
    };

    raid_damage = new raid_damage_t( name_str.c_str(), sim -> target, util_t::parse_school_type( type_str ) );
    raid_damage -> init();
  }

  virtual void start()
  {
    raid_event_t::start();

    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      if ( p -> sleeping ) continue;
      raid_damage -> base_dd_min = raid_damage -> base_dd_max = rng -> range( amount - amount_range, amount + amount_range );
      raid_damage -> target = p;
      raid_damage -> execute();
    }
  }
};

// Heal =====================================================================

struct heal_event_t : public raid_event_t
{
  double amount;
  double amount_range;

  heal_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "heal" ), amount( 1 ), amount_range( 0 )
  {
    option_t options[] =
    {
      { "amount",       OPT_FLT, &amount       },
      { "amount_range", OPT_FLT, &amount_range },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

	assert( duration == timespan_t::zero );
  }

  virtual void start()
  {
    raid_event_t::start();
    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      if ( p -> sleeping ) continue;

      double x = rng -> range( amount - amount_range, amount + amount_range );
      if ( sim -> log ) log_t::output( sim, "%s takes %.0f raid heal.", p -> name(), x );
      p -> resource_gain( RESOURCE_HEALTH, x );
    }
  }
};

// Vulnerable ===============================================================

struct vulnerable_event_t : public raid_event_t
{
  double multiplier;
  vulnerable_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "vulnerable" ), multiplier( 2.0 )
  {
    option_t options[] =
    {
      { "multiplier", OPT_FLT, &multiplier },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();
    sim -> target -> debuffs.vulnerable -> increment( 1, multiplier );
  }

  virtual void finish()
  {
    sim -> target -> debuffs.vulnerable -> decrement();
    raid_event_t::finish();
  }
};

// Position Switch ===============================================================

struct position_event_t : public raid_event_t
{

  position_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "position_switch" )
  {
    parse_options( 0, options_str );
  }

  virtual void start()
  {
    raid_event_t::start();

    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];
      if ( p -> position == POSITION_BACK )
        p -> position = POSITION_FRONT;
      else if ( p -> position == POSITION_RANGED_BACK )
        p -> position = POSITION_RANGED_FRONT;
    }
  }

  virtual void finish()
  {
    int num_affected = ( int ) affected_players.size();
    for ( int i=0; i < num_affected; i++ )
    {
      player_t* p = affected_players[ i ];

      p -> init_position();
    }

    raid_event_t::finish();
  }
};

// raid_event_t::raid_event_t ===============================================

raid_event_t::raid_event_t( sim_t* s, const char* n ) :
  sim( s ), name_str( n ),
  num_starts( 0 ), first( timespan_t::zero ), last( timespan_t::zero ),
  cooldown( timespan_t::zero ), cooldown_stddev( timespan_t::zero ),
  cooldown_min( timespan_t::zero ), cooldown_max( timespan_t::zero ),
  duration( timespan_t::zero ), duration_stddev( timespan_t::zero ),
  duration_min( timespan_t::zero ), duration_max( timespan_t::zero ),
  distance_min( 0 ), distance_max( 0 ), saved_duration( timespan_t::zero )
{
  rng = ( sim -> deterministic_roll ) ? sim -> deterministic_rng : sim -> rng;
}

// raid_event_t::cooldown_time ==============================================

timespan_t raid_event_t::cooldown_time() const
{
  timespan_t time;

  if ( num_starts == 0 )
  {
    time = timespan_t::zero;

    if ( first > timespan_t::zero )
    {
      time = first;
    }
  }
  else
  {
    time = rng -> gauss( cooldown, cooldown_stddev );

    if ( time < cooldown_min ) time = cooldown_min;
    if ( time > cooldown_max ) time = cooldown_max;
  }

  return time;
}

// raid_event_t::duration_time ==============================================

timespan_t raid_event_t::duration_time() const
{
  timespan_t time = rng -> gauss( duration, duration_stddev );

  if ( time < duration_min ) time = duration_min;
  if ( time > duration_max ) time = duration_max;

  return time;
}

// raid_event_t::start ======================================================

void raid_event_t::start()
{
  if ( sim -> log )
    log_t::output( sim, "Raid event %s starts.", name() );

  num_starts++;

  affected_players.clear();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( distance_min &&
         distance_min > p -> distance )
      continue;

    if ( distance_max &&
         distance_max < p -> distance )
      continue;

    affected_players.push_back( p );
  }
}

// raid_event_t::finish =====================================================

void raid_event_t::finish()
{
  if ( sim -> log )
    log_t::output( sim, "Raid event %s finishes.", name() );
}

// raid_event_t::schedule ===================================================

void raid_event_t::schedule()
{
  if ( sim -> debug ) log_t::output( sim, "Scheduling raid event: %s", name() );

  struct duration_event_t : public event_t
  {
    raid_event_t* raid_event;

    duration_event_t( sim_t* s, raid_event_t* re, timespan_t time ) : event_t( s ), raid_event( re )
    {
      name = re -> name_str.c_str();
      sim -> add_event( this, time );
    }

    virtual void execute()
    {
      raid_event -> finish();
    }
  };

  struct cooldown_event_t : public event_t
  {
    raid_event_t* raid_event;

    cooldown_event_t( sim_t* s, raid_event_t* re, timespan_t time ) : event_t( s ), raid_event( re )
    {
      name = re -> name_str.c_str();
      sim -> add_event( this, time );
    }

    virtual void execute()
    {
      raid_event -> saved_duration = raid_event -> duration_time();
      raid_event -> start();

      timespan_t ct = raid_event -> cooldown_time();

      if ( ct <= raid_event -> saved_duration ) ct = raid_event -> saved_duration + timespan_t::from_seconds(0.01);

      if ( raid_event -> saved_duration > timespan_t::zero )
      {
        new ( sim ) duration_event_t( sim, raid_event, raid_event -> saved_duration );
      }
      else raid_event -> finish();

      if ( raid_event -> last <= timespan_t::zero ||
           raid_event -> last > ( sim -> current_time + ct ) )
      {
        new ( sim ) cooldown_event_t( sim, raid_event, ct );
      }
    }
  };

  new ( sim ) cooldown_event_t( sim, this, cooldown_time() );
}

// raid_event_t::reset ======================================================

void raid_event_t::reset()
{
  num_starts = 0;

  if ( cooldown_min == timespan_t::zero ) cooldown_min = cooldown * 0.5;
  if ( cooldown_max == timespan_t::zero ) cooldown_max = cooldown * 1.5;

  if ( duration_min == timespan_t::zero ) duration_min = duration * 0.5;
  if ( duration_max == timespan_t::zero ) duration_max = duration * 1.5;

  affected_players.clear();
}

// raid_event_t::parse_options ==============================================

void raid_event_t::parse_options( option_t*          options,
                                  const std::string& options_str )
{
  if ( options_str.empty() ) return;

  option_t base_options[] =
  {
    { "first",           OPT_TIMESPAN, &first           },
    { "last",            OPT_TIMESPAN, &last            },
    { "period",          OPT_TIMESPAN, &cooldown        },
    { "cooldown",        OPT_TIMESPAN, &cooldown        },
    { "cooldown_stddev", OPT_TIMESPAN, &cooldown_stddev },
    { "cooldown>",       OPT_TIMESPAN, &cooldown_min    },
    { "cooldown<",       OPT_TIMESPAN, &cooldown_max    },
    { "duration",        OPT_TIMESPAN, &duration        },
    { "duration_stddev", OPT_TIMESPAN, &duration_stddev },
    { "duration>",       OPT_TIMESPAN, &duration_min    },
    { "duration<",       OPT_TIMESPAN, &duration_max    },
    { "distance>",       OPT_FLT, &distance_min    },
    { "distance<",       OPT_FLT, &distance_max    },
    { NULL, OPT_UNKNOWN, NULL }
  };

  std::vector<option_t> merged_options;

  if ( options )
    for ( int i=0; options[ i ].name; i++ )
      merged_options.push_back( options[ i ] );

  for ( int i=0; base_options[ i ].name; i++ )
    merged_options.push_back( base_options[ i ] );

  if ( ! option_t::parse( sim, name(), merged_options, options_str ) )
  {
    sim -> errorf( "Raid Event %s: Unable to parse options str '%s'.\n", name(), options_str.c_str() );
    sim -> cancel();
  }

}

// raid_event_t::create =====================================================

raid_event_t* raid_event_t::create( sim_t* sim,
                                    const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "adds"         ) return new         adds_event_t( sim, options_str );
  if ( name == "casting"      ) return new      casting_event_t( sim, options_str );
  if ( name == "distraction"  ) return new  distraction_event_t( sim, options_str );
  if ( name == "invul"        ) return new invulnerable_event_t( sim, options_str );
  if ( name == "invulnerable" ) return new invulnerable_event_t( sim, options_str );
  if ( name == "interrupt"    ) return new    interrupt_event_t( sim, options_str );
  if ( name == "movement"     ) return new     movement_event_t( sim, options_str );
  if ( name == "moving"       ) return new     movement_event_t( sim, options_str );
  if ( name == "damage"       ) return new       damage_event_t( sim, options_str );
  if ( name == "heal"         ) return new         heal_event_t( sim, options_str );
  if ( name == "stun"         ) return new         stun_event_t( sim, options_str );
  if ( name == "vulnerable"   ) return new   vulnerable_event_t( sim, options_str );
  if ( name == "position_switch" ) return new  position_event_t( sim, options_str );
  if ( name == "flying" )       return new  flying_event_t( sim, options_str );

  return 0;
}

// raid_event_t::init =======================================================

void raid_event_t::init( sim_t* sim )
{
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, sim -> raid_events_str, "/\\" );

  for ( int i=0; i < num_splits; i++ )
  {
    std::string name = splits[ i ];
    std::string options = "";

    if ( sim -> debug ) log_t::output( sim, "Creating raid event: %s", name.c_str() );

    std::string::size_type cut_pt = name.find_first_of( "," );

    if ( cut_pt != name.npos )
    {
      options = name.substr( cut_pt + 1 );
      name    = name.substr( 0, cut_pt );
    }

    raid_event_t* e = create( sim, name, options );

    if ( ! e )
    {
      sim -> errorf( "Unknown raid event: %s\n", splits[ i ].c_str() );
      sim -> cancel();
      continue;
    }

	assert( e -> cooldown > timespan_t::zero );
    assert( e -> cooldown > e -> cooldown_stddev );

    sim -> raid_events.push_back( e );
  }
}

// raid_event_t::reset ======================================================

void raid_event_t::reset( sim_t* sim )
{
  int num_events = ( int ) sim -> raid_events.size();
  for ( int i=0; i < num_events; i++ )
  {
    sim -> raid_events[ i ] -> reset();
  }
}

// raid_event_t::combat_begin ===============================================

void raid_event_t::combat_begin( sim_t* sim )
{
  int num_events = ( int ) sim -> raid_events.size();
  for ( int i=0; i < num_events; i++ )
  {
    sim -> raid_events[ i ] -> schedule();
  }
}
