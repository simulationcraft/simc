// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Raid Events
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct adds_event_t : public raid_event_t
{
  double count;
  double health;
  std::string master_str;
  std::string name_str;
  player_t* master;
  std::vector< pet_t* > adds;
  double count_range;
  size_t adds_to_remove;
  double spawn_x_coord;
  double spawn_y_coord;
  int spawn_stacked;
  double spawn_radius_min;
  double spawn_radius_max;
  double spawn_radius;
  double spawn_angle_start;
  double spawn_angle_end;

  adds_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "adds" ),
    count( 1 ), health( 100000 ), master_str( "Fluffy_Pillow" ), name_str( "Add" ),
    master( 0 ), count_range( false ), adds_to_remove( 0 ), spawn_x_coord( 0 ),
    spawn_y_coord( 0 ), spawn_stacked( 0 ), spawn_radius_min( 0 ), spawn_radius_max( 0 ),
    spawn_radius( 0 ), spawn_angle_start( -1 ), spawn_angle_end( -1 )
  {
    add_option( opt_string( "name", name_str ) );
    add_option( opt_string( "master", master_str ) );
    add_option( opt_float( "count", count ) );
    add_option( opt_float( "health", health ) );
    add_option( opt_float( "count_range", count_range ) );
    add_option( opt_float( "spawn_x", spawn_x_coord ) );
    add_option( opt_float( "spawn_y", spawn_y_coord ) );
    add_option( opt_int( "stacked", spawn_stacked ) );
    add_option( opt_float( "min_distance", spawn_radius_min ) );
    add_option( opt_float( "max_distance", spawn_radius_max ) );
    add_option( opt_float( "distance", spawn_radius ) );
    add_option( opt_float( "angle_start", spawn_angle_start ) );
    add_option( opt_float( "angle_end", spawn_angle_end ) );
    parse_options( options_str );

    master = sim -> find_player( master_str );
    // If the master is not found, default the master to the first created enemy
    if ( ! master ) master = sim -> target_list.data().front();
    assert( master );

    double overlap = 1;
    timespan_t min_cd = cooldown;

    if ( cooldown_stddev != timespan_t::zero() )
    {
      min_cd -= cooldown_stddev * 6;
      if ( min_cd <= timespan_t::zero() )
      {
        sim -> errorf( "The standard deviation of %.3f seconds is too large, creating a too short minimum cooldown (%.3f seconds)", cooldown_stddev.total_seconds(), min_cd.total_seconds() );
        cooldown_stddev = timespan_t::zero();
      }
    }

    if ( min_cd > timespan_t::zero() ) overlap = duration / min_cd;

    if ( overlap > 1 )
    {
      sim -> errorf( "Simc does not support overlapping add spawning in a single raid event (duration of %.3fs > reasonable minimum cooldown of %.3fs).", duration.total_seconds(), min_cd.total_seconds() );
      overlap = 1;
      duration = min_cd - timespan_t::from_seconds( 0.001 );
    }

    sim -> add_waves++;

    if ( fabs( spawn_radius ) > 0 || fabs( spawn_radius_max ) > 0 || fabs( spawn_radius_min ) > 0 )
    {
      sim -> distance_targeting_enabled = true;
      if ( fabs( spawn_radius ) > 0 )
      {
        spawn_radius_min = spawn_radius_max = fabs( spawn_radius );
      }

      double tempangle;
      if ( spawn_angle_start > 360 )
      {
        tempangle = fmod( spawn_angle_start, 360 );
        if ( tempangle != spawn_angle_start )
        {
          spawn_angle_start = tempangle;
        }
      }

      if ( spawn_angle_end > 360 )
      {
        tempangle = fmod( spawn_angle_end, 360 );
        if ( tempangle != spawn_angle_end )
        {
          spawn_angle_end = tempangle;
        }
      }

      if ( spawn_angle_start == spawn_angle_end && spawn_angle_start >= 0 ) //Full circle
      {
        spawn_angle_start = 0;
        spawn_angle_end = 360;
      }
      else //Not a full circle, or, one/both of the values is not specified
      {
        if ( spawn_x_coord != 0 || spawn_y_coord != 0 ) //Not default placement
        {
          if ( spawn_angle_start < 0 )
          {
            spawn_angle_start = 0;
          }
          if (spawn_angle_end < 0 )
          {
            spawn_angle_end = 360;
          }
        }
        else if ( spawn_x_coord == 0 && spawn_y_coord == 0 && spawn_angle_start < 0 && spawn_angle_end < 0 ) //Default placement, only spawn adds on near side
        {
          spawn_angle_start = 90;
          spawn_angle_end = 270;
        }

        if ( spawn_angle_start < 0 ) //Default start value if not specified
        {
          spawn_angle_start = 90;
        }

        if ( spawn_angle_end < 0 ) //Default end value if not specified
        {
          spawn_angle_end = 270;
        }

        if ( spawn_angle_end < spawn_angle_start )
        {
          spawn_angle_end += 360;
        }
      }

      if ( sim -> log )
      {
        sim -> out_log.printf( "Spawn Angle set between %f and %f degrees.", spawn_angle_start, spawn_angle_end );
      }
    }

    for ( int i = 0; i < util::ceil( overlap ); i++ )
    {
      for ( unsigned add = 0; add < util::ceil( count + count_range ); add++ )
      {
        std::string add_name_str = "";

        if ( sim -> add_waves > 1 && name_str == "Add" ) // Only add wave to secondary wave that aren't given manual names.
        {
          add_name_str += "Wave";
          add_name_str += util::to_string( sim -> add_waves );
          add_name_str += "_";
        }

        add_name_str += name_str;
        add_name_str += util::to_string( add + 1 );

        pet_t* p = master -> create_pet( add_name_str );
        assert( p );
        p -> resources.base[ RESOURCE_HEALTH ] = health;
        adds.push_back( p );
      }
    }
  }

  void regenerate_cache()
  {
    for (auto p : affected_players)
    {
      
      // Invalidate target caches
      for ( size_t i = 0, end = p -> action_list.size(); i < end; i++ )
        p -> action_list[i] -> target_cache.is_valid = false; //Regenerate Cache.
    }
  }

  void _start() override
  {
    adds_to_remove = static_cast<size_t>( util::round( std::max( 0.0, sim -> rng().range( count - count_range, count + count_range ) ) ) );

    double x_offset = 0;
    double y_offset = 0;
    bool offset_computed = false;

    for ( size_t i = 0; i < adds.size(); i++ )
    {
      if ( i < adds.size() )
      {
        if ( fabs( spawn_radius_max ) > 0 )
        {
          if ( spawn_stacked == 0 || !offset_computed )
          {
            double angle_start = spawn_angle_start * ( M_PI / 180 );
            double angle_end = spawn_angle_end * ( M_PI / 180 );
            double angle = sim -> rng().range( angle_start, angle_end );
            double radius = sim -> rng().range( fabs( spawn_radius_min ), fabs( spawn_radius_max ) );
            x_offset = radius * cos(angle);
            y_offset = radius * sin(angle);
            offset_computed = true;
          }
        }

        adds[i] -> summon( saved_duration );
        adds[i] -> x_position = x_offset + spawn_x_coord;
        adds[i] -> y_position = y_offset + spawn_y_coord;

        if ( sim -> log )
        {
          if ( x_offset != 0 || y_offset != 0 )
          {
            sim -> out_log.printf( "New add spawned at %f, %f.", x_offset, y_offset );
          }
        }
      }
      if ( i >= adds_to_remove )
      {
        adds[i] -> dismiss();
      }
    }
    regenerate_cache();
  }

  void _finish() override
  {
    for ( size_t i = 0; i < adds_to_remove; i++ )
    {
      adds[i] -> dismiss();
    }
  }
};

struct move_enemy_t : public raid_event_t
{
  double x_coord;
  double y_coord;
  std::string name;
  player_t* enemy;
  double original_x;
  double original_y;

  move_enemy_t( sim_t* s, const std::string& options_str ):
    raid_event_t( s, "move_enemy" ), x_coord( 0.0 ), y_coord( 0.0 ),
    name( "" ), enemy( 0 ), original_x( 0.0 ), original_y( 0.0 )
  {
    add_option( opt_float( "x", x_coord ) );
    add_option( opt_float( "y", y_coord ) );
    add_option( opt_string( "name", name ) );
    parse_options( options_str );

    enemy = sim -> find_player( name );
    sim -> distance_targeting_enabled = true;

    if ( !enemy )
    {
      sim -> out_log.printf( "Move enemy event cannot be created, there is no enemy named %s.", name.c_str() );
    }
  }

  void regenerate_cache()
  {
    for (auto p : affected_players)
    {
      
      // Invalidate target caches
      for ( size_t i = 0, end = p -> action_list.size(); i < end; i++ )
        p -> action_list[i] -> target_cache.is_valid = false; //Regenerate Cache.
    }
  }

  void reset() override
  {
    raid_event_t::reset();

    if ( enemy )
    {
      enemy -> x_position = enemy -> default_x_position;
      enemy -> y_position = enemy -> default_y_position;
    }
  }

  void _start() override
  {
    if ( enemy )
    {
      original_x = enemy -> x_position;
      original_y = enemy -> y_position;
      enemy -> x_position = x_coord;
      enemy -> y_position = y_coord;
      regenerate_cache();
    }
  }

  void _finish() override
  {
    if ( enemy )
    {
      enemy -> x_position = enemy -> default_x_position;
      enemy -> y_position = enemy -> default_y_position;
      regenerate_cache();
    }
  }
};

// Casting ==================================================================

struct casting_event_t : public raid_event_t
{
  casting_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "casting" )
  {
    parse_options( options_str );
  }

  virtual void _start() override
  {
    sim -> target -> debuffs.casting -> increment();
    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      p -> interrupt();
    }
  }

  virtual void _finish() override
  {
    sim -> target -> debuffs.casting -> decrement();
  }
};

// Distraction ==============================================================

struct distraction_event_t : public raid_event_t
{
  double skill;

  distraction_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "distraction" ),
    skill( 0.2 )
  {
    players_only = true; // Pets shouldn't have less "skill"

    add_option( opt_float( "skill", skill ) );
    parse_options( options_str );
  }

  virtual void _start() override
  {
    for (auto p : affected_players)
    {
      
      p -> current.skill_debuff += skill;
    }
  }

  virtual void _finish() override
  {
    for (auto p : affected_players)
    {
      
      p -> current.skill_debuff -= skill;
    }
  }
};

// Invulnerable =============================================================

// TODO: Support more than sim -> target
struct invulnerable_event_t : public raid_event_t
{
  bool retarget;
  player_t* target;

  invulnerable_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "invulnerable" ), retarget( false ), target( s -> target )
  {
    add_option( opt_bool( "retarget", retarget ) );
    add_option( opt_func( "target", std::bind( &invulnerable_event_t::parse_target,
      this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) ) );

    parse_options( options_str );
  }

  bool parse_target( sim_t* /* sim */, const std::string& /* name */, const std::string& value )
  {
    auto it = range::find_if( sim -> target_list, [ &value ]( const player_t* target ) {
      return util::str_compare_ci( value, target -> name() );
    } );

    if ( it != sim -> target_list.end() )
    {
      target = *it;
      return true;
    }
    else
    {
      sim -> errorf( "Unknown invulnerability raid event target '%s'", value.c_str() );
      return false;
    }
  }

  void _start() override
  {
    target -> clear_debuffs();
    target -> debuffs.invulnerable -> increment();

    range::for_each( sim -> player_non_sleeping_list, []( player_t* p ) {
      p -> in_combat = true; // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p -> halt();
    } );

    if ( retarget )
    {
      range::for_each( sim -> player_non_sleeping_list, [ this ]( player_t* p ) {
        p -> acquire_target( ACTOR_INVULNERABLE, target );
      } );
    }
  }

  void _finish() override
  {
    target -> debuffs.invulnerable -> decrement();

    if ( retarget )
    {
      range::for_each( sim -> player_non_sleeping_list, [ this ]( player_t* p ) {
        p -> acquire_target( ACTOR_VULNERABLE, target );
      } );
    }

  }
};

// Flying ===================================================================

struct flying_event_t : public raid_event_t
{
  flying_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "flying" )
  {
    parse_options( options_str );
  }

  virtual void _start() override
  {
    sim -> target -> debuffs.flying -> increment();
  }

  virtual void _finish() override
  {
    sim -> target -> debuffs.flying -> decrement();
  }
};


// Movement =================================================================

struct movement_ticker_t : public event_t
{
  const std::vector<player_t*>& players;
  timespan_t duration;

  movement_ticker_t( sim_t& s, const std::vector<player_t*>& p,
                     timespan_t d = timespan_t::zero() )
    : event_t( s, d > timespan_t::zero() ? d : next_execute( p ) ), players( p )
  {
    if ( d > timespan_t::zero() )
    {
      duration = d;
    }
    else
    {
      duration = next_execute( players );
    }

    if ( sim().debug )
      sim().out_debug.printf( "New movement event" );
  }
  virtual const char* name() const override
  {
    return "Player Movement Event";
  }

  static timespan_t next_execute( const std::vector<player_t*>& players )
  {
    timespan_t min_time = timespan_t::max();
    bool any_movement   = false;
    for ( auto player : players )
    {
      timespan_t time_to_finish = player->time_to_move();
      if ( time_to_finish == timespan_t::zero() )
        continue;

      any_movement = true;

      if ( time_to_finish < min_time )
        min_time = time_to_finish;
    }

    if ( min_time > timespan_t::from_seconds( 0.1 ) )
      min_time = timespan_t::from_seconds( 0.1 );

    if ( !any_movement )
      return timespan_t::zero();
    else
      return min_time;
  }

  void execute() override
  {
    for ( auto p : players )
    {
      if ( p->is_sleeping() )
        continue;

      if ( p->time_to_move() == timespan_t::zero() )
        continue;

      p->update_movement( duration );
    }

    timespan_t next = next_execute( players );
    if ( next > timespan_t::zero() )
      make_event<movement_ticker_t>( sim(), sim(), players, next );
  }
};

struct movement_event_t : public raid_event_t
{
  double move_distance;
  movement_direction_e direction;
  std::string move_direction;
  double distance_range;
  double move;
  double distance_min;
  double distance_max;
  double avg_player_movement_speed;

  movement_event_t( sim_t* s, const std::string& options_str ):
    raid_event_t( s, "movement" ),
    move_distance( 0 ),
    direction( MOVEMENT_TOWARDS ),
    distance_range( 0 ),
    distance_min( 0 ),
    distance_max( 0 ),
    avg_player_movement_speed( 7.0 )
  {
    add_option( opt_float( "distance", move_distance ) );
    add_option( opt_string( "direction", move_direction ) );
    add_option( opt_float( "distance_range", distance_range ) );
    add_option( opt_float( "distance_min", distance_min ) );
    add_option( opt_float( "distance_max", distance_max ) );
    parse_options( options_str );

    if ( duration > timespan_t::zero() ) move_distance = duration.total_seconds() * avg_player_movement_speed;

    double cooldown_move = cooldown.total_seconds();
    if ( ( move_distance / avg_player_movement_speed ) > cooldown_move ) move_distance = cooldown_move * avg_player_movement_speed;

    double max = ( move_distance + distance_range ) / avg_player_movement_speed;
    if ( max > cooldown_move ) distance_range = ( max - cooldown_move ) * avg_player_movement_speed;

    if ( distance_max < distance_min ) distance_max = distance_min;

    if ( distance_max / avg_player_movement_speed > cooldown_move ) distance_max = cooldown_move * avg_player_movement_speed;
    if ( distance_min / avg_player_movement_speed > cooldown_move ) distance_min = cooldown_move * avg_player_movement_speed;

    if ( move_distance > 0 ) name_str = "movement_distance";
    if ( !move_direction.empty() ) direction = util::parse_movement_direction( move_direction );
  }

  virtual void _start() override
  {
    movement_direction_e m = direction;
    if ( direction == MOVEMENT_RANDOM )
    {
      m = static_cast<movement_direction_e>( int( sim -> rng().range( MOVEMENT_RANDOM_MIN, MOVEMENT_RANDOM_MAX ) ) );
    }

    if ( distance_range > 0 )
    {
      move = sim -> rng().range( move_distance - distance_range, move_distance + distance_range );
      if ( move < distance_min ) move = distance_min;
      else if ( move > distance_max ) move = distance_max;
    }
    else if ( distance_min > 0 || distance_max > 0 ) move = sim -> rng().range( distance_min, distance_max );
    else move = move_distance;

    if ( move <= 0.0 ) return;

    for (auto p : affected_players)
    {
      
      p -> trigger_movement( move, m );

      if ( p -> buffs.stunned -> check() ) continue;
      p -> in_combat = true; // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p -> moving();
    }

    if ( affected_players.size() > 0 )
    {
      make_event<movement_ticker_t>( *sim, *sim, affected_players );
    }
  }

  virtual void _finish() override
  {}
};

// Stun =====================================================================

struct stun_event_t : public raid_event_t
{
  stun_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "stun" )
  {
    parse_options( options_str );
  }

  virtual void _start() override
  {
    for (auto p : affected_players)
    {
      
      p -> buffs.stunned -> increment();
      p -> in_combat = true; // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p -> stun();
    }
  }

  virtual void _finish() override
  {
    for (auto p : affected_players)
    {
      
      p -> buffs.stunned -> decrement();
      if ( ! p -> buffs.stunned -> check() )
      {
        // Don't schedule_ready players who are already working, like pets auto-summoned during the stun event ( ebon imp ).
        if ( ! p -> channeling && ! p -> executing && ! p -> readying )
          p -> schedule_ready();
      }
    }
  }
};

// Interrupt ================================================================

struct interrupt_event_t : public raid_event_t
{
  interrupt_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "interrupt" )
  {
    parse_options( options_str );
  }

  virtual void _start() override
  {
    for (auto p : affected_players)
    {
      
      p -> interrupt();
    }
  }

  virtual void _finish() override
  {}
};

// Damage ===================================================================

struct damage_event_t : public raid_event_t
{
  double amount;
  double amount_range;
  spell_t* raid_damage;
  school_e damage_type;

  damage_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "damage" ),
    amount( 1 ), amount_range( 0 ), raid_damage( 0 )
  {
    std::string type_str = "holy";
    add_option( opt_float( "amount", amount ) );
    add_option( opt_float( "amount_range", amount_range ) );
    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );

    assert( duration == timespan_t::zero() );

    name_str = "raid_damage_" + type_str;
    damage_type = util::parse_school_type( type_str );
  }

  virtual void _start() override
  {
    if ( ! raid_damage )
    {
      struct raid_damage_t : public spell_t
      {
        raid_damage_t( const char* n, player_t* player, school_e s ) :
          spell_t( n, player, spell_data_t::nil() )
        {
          school = s;
          may_crit = false;
          background = true;
          trigger_gcd = timespan_t::zero();
        }
      };

      raid_damage = new raid_damage_t( name_str.c_str(), sim -> target, damage_type );
      raid_damage -> init();
    }

    for (auto p : affected_players)
    {
      
      raid_damage -> base_dd_min = raid_damage -> base_dd_max = sim -> rng().range( amount - amount_range, amount + amount_range );
      raid_damage -> target = p;
      raid_damage -> execute();
    }
  }

  virtual void _finish() override
  {}
};

// Heal =====================================================================

struct heal_event_t : public raid_event_t
{
  double amount;
  double amount_range;
  double to_pct, to_pct_range;

  heal_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "heal" ), amount( 1 ), amount_range( 0 ), to_pct( 0 ), to_pct_range( 0 )
  {
    add_option( opt_float( "amount", amount ) );
    add_option( opt_float( "amount_range", amount_range ) );
    add_option( opt_float( "to_pct", to_pct ) );
    add_option( opt_float( "to_pct_range", to_pct_range ) );
    parse_options( options_str );

    assert( duration == timespan_t::zero() );
  }

  virtual void _start() override
  {
    for (auto p : affected_players)
    {
      
      double x; // amount to heal

      // to_pct tells this event to heal each player up to a percent of their max health
      if ( to_pct > 0 )
      {
        double pct_actual = to_pct;
        if ( to_pct_range > 0 )
          pct_actual = sim -> rng().range( to_pct - to_pct_range, to_pct + to_pct_range );
        if ( sim -> debug )
          sim -> out_debug.printf( "%s healing to %.3f%% (%.0f) of max health, current health %.0f",
              p -> name(), pct_actual, p -> resources.max[ RESOURCE_HEALTH ] * pct_actual / 100,
              p -> resources.current[ RESOURCE_HEALTH ] );
        x = p -> resources.max[ RESOURCE_HEALTH ] * pct_actual / 100;
        x -= p -> resources.current[ RESOURCE_HEALTH ];
      }
      else
      {
        x = sim -> rng().range( amount - amount_range, amount + amount_range );
        p -> resource_gain( RESOURCE_HEALTH, x );
      }

      // heal if there's any healing to be done
      if ( x > 0 )
      {
        if ( sim -> log )
          sim -> out_log.printf( "%s takes %.0f raid heal.", p -> name(), x );

        p -> resource_gain( RESOURCE_HEALTH, x );
      }
    }
  }

  virtual void _finish() override
  {}
};

// Damage Taken Debuff=========================================================

struct damage_taken_debuff_event_t : public raid_event_t
{
  int amount;

  damage_taken_debuff_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "damage_taken" ), amount( 1 )
  {
    add_option( opt_int( "amount", amount ) );
    parse_options( options_str );

    assert( duration == timespan_t::zero() );
  }

  virtual void _start() override
  {
    for (auto p : affected_players)
    {
      

      if ( sim -> log ) sim -> out_log.printf( "%s gains %d stacks of damage_taken debuff.", p -> name(), amount );

      p -> debuffs.damage_taken -> trigger( amount );

    }
  }

  virtual void _finish() override
  {
  }
};

// Vulnerable ===============================================================

struct vulnerable_event_t : public raid_event_t
{
  double multiplier;

  vulnerable_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "vulnerable" ), multiplier( 2.0 )
  {
    add_option( opt_float( "multiplier", multiplier ) );
    parse_options( options_str );
  }

  virtual void _start() override
  {
    sim -> target -> debuffs.vulnerable -> increment( 1, multiplier );
  }

  virtual void _finish() override
  {
    sim -> target -> debuffs.vulnerable -> decrement();
  }
};

// Position Switch ==========================================================

struct position_event_t : public raid_event_t
{

  position_event_t( sim_t* s, const std::string& options_str ) :
    raid_event_t( s, "position_switch" )
  {
    parse_options( options_str );
  }

  virtual void _start() override
  {
    for (auto p : affected_players)
    {
      
      if ( p -> position() == POSITION_BACK )
        p -> change_position( POSITION_FRONT );
      else if ( p -> position() == POSITION_RANGED_BACK )
        p -> change_position( POSITION_RANGED_FRONT );
    }
  }

  virtual void _finish() override
  {
    for (auto p : affected_players)
    {
      

      p -> change_position( p -> initial.position );
    }
  }
};

} // UNNAMED NAMESPACE

// raid_event_t::raid_event_t ===============================================

raid_event_t::raid_event_t( sim_t* s, const std::string& n ) :
  sim( s ),
  name_str( n ),
  num_starts( 0 ),
  first( timespan_t::zero() ),
  last( timespan_t::zero() ),
  next( timespan_t::zero() ),
  cooldown( timespan_t::zero() ),
  cooldown_stddev( timespan_t::zero() ),
  cooldown_min( timespan_t::zero() ),
  cooldown_max( timespan_t::zero() ),
  duration( timespan_t::zero() ),
  duration_stddev( timespan_t::zero() ),
  duration_min( timespan_t::zero() ),
  duration_max( timespan_t::zero() ),
  distance_min( 0 ),
  distance_max( 0 ),
  players_only( false ),
  player_chance( 1.0 ),
  affected_role( ROLE_NONE ),
  saved_duration( timespan_t::zero() )
{
  add_option( opt_string( "first", first_str ) );
  add_option( opt_string( "last", last_str ) );
  add_option( opt_timespan( "period", cooldown ) );
  add_option( opt_timespan( "cooldown", cooldown ) );
  add_option( opt_timespan( "cooldown_stddev", cooldown_stddev ) );
  add_option( opt_timespan( "cooldown_min", cooldown_min ) );
  add_option( opt_timespan( "cooldown_max", cooldown_max ) );
  add_option( opt_timespan( "duration", duration ) );
  add_option( opt_timespan( "duration_stddev", duration_stddev ) );
  add_option( opt_timespan( "duration_min", duration_min ) );
  add_option( opt_timespan( "duration_max", duration_max ) );
  add_option( opt_bool( "players_only",  players_only ) );
  add_option( opt_float( "player_chance", player_chance ) );
  add_option( opt_float( "distance_min", distance_min ) );
  add_option( opt_float( "distance_max", distance_max ) );
  add_option( opt_string( "affected_role", affected_role_str ) );

}

// raid_event_t::cooldown_time ==============================================

timespan_t raid_event_t::cooldown_time()
{
  timespan_t time;

  if ( num_starts == 0 )
  {
    if ( first > timespan_t::zero() )
    {
      time = first;
    }
    else
    {
      time = timespan_t::from_millis( 10 );
    }
  }
  else
  {
    time = sim -> rng().gauss( cooldown, cooldown_stddev );

    time = clamp( time, cooldown_min, cooldown_max );
  }

  return time;
}

// raid_event_t::duration_time ==============================================

timespan_t raid_event_t::duration_time()
{
  timespan_t time = sim -> rng().gauss( duration, duration_stddev );

  time = clamp( time, duration_min, duration_max );

  return time;
}

// raid_event_t::start ======================================================

void raid_event_t::start()
{
  if ( sim -> log )
    sim -> out_log.printf( "Raid event %s starts.", name_str.c_str() );

  num_starts++;

  affected_players.clear();

  for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
  {
    player_t* p = sim -> player_non_sleeping_list[ i ];

    // Filter players
    if ( filter_player( p ) )
      continue;

    affected_players.push_back( p );
  }

  _start();
}

bool filter_sleeping( const player_t* p )
{
  return p -> is_sleeping();
}
// raid_event_t::finish =====================================================

void raid_event_t::finish()
{
  // Make sure we dont have any players which were active on start, but are now sleeping
  affected_players.erase( std::remove_if( affected_players.begin(), affected_players.end(), filter_sleeping ), affected_players.end() );

  _finish();

  if ( sim -> log )
    sim -> out_log.printf( "Raid event %s finishes.", name_str.c_str() );
}

// raid_event_t::schedule ===================================================

void raid_event_t::schedule()
{
  if ( sim -> debug ) sim -> out_debug.printf( "Scheduling raid event: %s", name_str.c_str() );

  struct duration_event_t : public event_t
  {
    raid_event_t* raid_event;

    duration_event_t( sim_t& s, raid_event_t* re, timespan_t time ) :
      event_t( s, time ),
      raid_event( re )
    {
      re -> set_next( s.current_time() + time );
    }
    virtual const char* name() const override
    { return raid_event -> name(); }
    virtual void execute() override
    {
      raid_event -> finish();
    }
  };

  struct cooldown_event_t : public event_t
  {
    raid_event_t* raid_event;

    cooldown_event_t( sim_t& s, raid_event_t* re, timespan_t time ) :
      event_t( s, time ),
      raid_event( re )
    {
      re -> set_next( s.current_time() + time );
    }
    virtual const char* name() const override
    { return raid_event -> name(); }
    virtual void execute() override
    {
      raid_event -> saved_duration = raid_event -> duration_time();
      raid_event -> start();

      timespan_t ct = raid_event -> cooldown_time();

      if ( ct <= raid_event -> saved_duration ) ct = raid_event -> saved_duration + timespan_t::from_seconds( 0.01 );

      if ( raid_event -> saved_duration > timespan_t::zero() )
      {
        make_event<duration_event_t>( sim(), sim(), raid_event, raid_event -> saved_duration );
      }
      else raid_event -> finish();

      if ( raid_event -> last <= timespan_t::zero() ||
           raid_event -> last > ( sim().current_time() + ct ) )
      {
        make_event<cooldown_event_t>( sim(), sim(), raid_event, ct );
      }
    }
  };

  make_event<cooldown_event_t>( *sim, *sim, this, cooldown_time() );
}

// raid_event_t::reset ======================================================

void raid_event_t::reset()
{
  num_starts = 0;

  if ( cooldown_min == timespan_t::zero() ) cooldown_min = cooldown * 0.5;
  if ( cooldown_max == timespan_t::zero() ) cooldown_max = cooldown * 1.5;

  if ( duration_min == timespan_t::zero() ) duration_min = duration * 0.5;
  if ( duration_max == timespan_t::zero() ) duration_max = duration * 1.5;

  affected_players.clear();
}

// raid_event_t::parse_options ==============================================

void raid_event_t::parse_options( const std::string& options_str )
{
  if ( options_str.empty() ) return;

  try
  {
    opts::parse( sim, name_str, options, options_str );
  }
  catch( const std::exception& e )
  {
    sim -> errorf( "Raid Event %s: Unable to parse options str '%s': %s", name_str.c_str(), options_str.c_str(), e.what() );
    sim -> cancel();
  }

  if ( player_chance > 1.0 || player_chance < 0.0 )
  {
    sim -> errorf( "Player Chance needs to be withing [ 0.0, 1.0 ]. Overriding to 1.0\n" );
    player_chance = 1.0;
  }

  if ( ! affected_role_str.empty() )
  {
    affected_role = util::parse_role_type( affected_role_str );
    if ( affected_role == ROLE_NONE )
      sim -> errorf( "Unknown role '%s' specified for raid event.\n", affected_role_str.c_str() );
  }

  // Parse first/last
  if ( first_str.size() > 1 )
  {
    if ( *( first_str.end() - 1 ) == '%' )
    {
      double pct = atof( first_str.substr( 0, first_str.size() - 1 ).c_str() ) / 100.0;
      first = sim -> max_time * pct;
    }
    else
      first = timespan_t::from_seconds( atof( first_str.c_str() ) );

    if ( first.total_seconds() < 0 )
      first = timespan_t::zero();
  }

  if ( last_str.size() > 1 )
  {
    if ( *( last_str.end() - 1 ) == '%' )
    {
      double pct = atof( last_str.substr( 0, last_str.size() - 1 ).c_str() ) / 100.0;
      last = sim -> max_time * pct;
    }
    else
      last = timespan_t::from_seconds( atof( last_str.c_str() ) );

    if ( last.total_seconds() < 0 )
      last = timespan_t::zero();
  }
}

// raid_event_t::create =====================================================

std::unique_ptr<raid_event_t> raid_event_t::create( sim_t* sim,
                                    const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "adds"         ) return std::unique_ptr<raid_event_t>(new         adds_event_t( sim, options_str ));
  if ( name == "move_enemy"   ) return std::unique_ptr<raid_event_t>(new         move_enemy_t( sim, options_str ));
  if ( name == "casting"      ) return std::unique_ptr<raid_event_t>(new      casting_event_t( sim, options_str ));
  if ( name == "distraction"  ) return std::unique_ptr<raid_event_t>(new  distraction_event_t( sim, options_str ));
  if ( name == "invul"        ) return std::unique_ptr<raid_event_t>(new invulnerable_event_t( sim, options_str ));
  if ( name == "invulnerable" ) return std::unique_ptr<raid_event_t>(new invulnerable_event_t( sim, options_str ));
  if ( name == "interrupt"    ) return std::unique_ptr<raid_event_t>(new    interrupt_event_t( sim, options_str ));
  if ( name == "movement"     ) return std::unique_ptr<raid_event_t>(new     movement_event_t( sim, options_str ));
  if ( name == "moving"       ) return std::unique_ptr<raid_event_t>(new     movement_event_t( sim, options_str ));
  if ( name == "damage"       ) return std::unique_ptr<raid_event_t>(new       damage_event_t( sim, options_str ));
  if ( name == "heal"         ) return std::unique_ptr<raid_event_t>(new         heal_event_t( sim, options_str ));
  if ( name == "stun"         ) return std::unique_ptr<raid_event_t>(new         stun_event_t( sim, options_str ));
  if ( name == "vulnerable"   ) return std::unique_ptr<raid_event_t>(new   vulnerable_event_t( sim, options_str ));
  if ( name == "position_switch" ) return std::unique_ptr<raid_event_t>(new  position_event_t( sim, options_str ));
  if ( name == "flying" )       return std::unique_ptr<raid_event_t>(new       flying_event_t( sim, options_str ));
  if ( name == "damage_taken_debuff" ) return std::unique_ptr<raid_event_t>(new   damage_taken_debuff_event_t( sim, options_str ));

  return std::unique_ptr<raid_event_t>();
}

// raid_event_t::init =======================================================

void raid_event_t::init( sim_t* sim )
{
  std::vector<std::string> splits = util::string_split( sim -> raid_events_str, "/\\" );

  for ( size_t i = 0; i < splits.size(); i++ )
  {
    std::string name = splits[ i ];
    std::string options = "";

    if ( sim -> debug ) sim -> out_debug.printf( "Creating raid event: %s", name.c_str() );

    std::string::size_type cut_pt = name.find_first_of( "," );

    if ( cut_pt != name.npos )
    {
      options = name.substr( cut_pt + 1 );
      name    = name.substr( 0, cut_pt );
    }

    auto raid_event = create( sim, name, options );

    if ( ! raid_event )
    {
      sim -> errorf( "Unknown raid event: %s\n", splits[ i ].c_str() );
      sim -> cancel();
      continue;
    }

    if ( raid_event -> cooldown == timespan_t::zero() )
    {
      continue;
    }

    assert( raid_event -> cooldown > timespan_t::zero() );
    assert( raid_event -> cooldown > raid_event -> cooldown_stddev );

    sim -> raid_events.push_back( std::move(raid_event) );
  }
}

// raid_event_t::reset ======================================================

void raid_event_t::reset( sim_t* sim )
{
  for ( auto& raid_event : sim -> raid_events )
  {
    raid_event -> reset();
  }
}

// raid_event_t::combat_begin ===============================================

void raid_event_t::combat_begin( sim_t* sim )
{
  for ( auto& raid_event : sim -> raid_events )
  {
    raid_event -> schedule();
  }
}

/* This (virtual) function filters players which shouldn't be added to the
 * affected_players list.
 */

bool raid_event_t::filter_player( const player_t* p )
{
  if ( distance_min &&
       distance_min > p -> current.distance )
    return true;

  if ( distance_max &&
       distance_max < p -> current.distance )
    return true;

  if ( p -> is_pet() && players_only )
    return true;

  if ( ! sim -> rng().roll( player_chance ) )
    return true;

  if ( affected_role != ROLE_NONE && p -> role != affected_role )
    return true;

  return false;
}

double raid_event_t::evaluate_raid_event_expression( sim_t* s, std::string& type, std::string& filter )
{
  // correct for "damage" type event
  if ( util::str_compare_ci( type, "damage" ) )
    type = "raid_damage_";

  // filter the list for raid events that match the type requested
  std::vector<raid_event_t*> matching_type;
  for ( const auto& raid_event : s -> raid_events )
    if ( util::str_prefix_ci( raid_event -> name(), type ) )
      matching_type.push_back( raid_event.get() );

  if ( matching_type.empty() )
  {
    if ( util::str_compare_ci( filter, "in" ) || util::str_compare_ci( filter, "cooldown" ) )
      return 1.0e10; // ridiculously large number
    else
      return 0.0;
    // return constant based on filter
  }
  else if ( util::str_compare_ci( filter, "exists" ) )
    return 1.0;

  // now go through the list of matching raid events and look for the one happening first
  raid_event_t* e = 0;
  timespan_t time_to_event = timespan_t::from_seconds( -1 );

  for ( size_t i = 0; i < matching_type.size(); i++ )
    if ( time_to_event < timespan_t::zero() || matching_type[ i ] -> next_time() - s -> current_time() < time_to_event )
    {
    e = matching_type[ i ];
    time_to_event = e -> next_time() - s -> current_time();
    }
  if ( e == 0 )
    return 0.0;

  // now that we have the event in question, use the filter to figure out return
  if ( util::str_compare_ci( filter, "in" ) )
    return time_to_event > timespan_t::zero() ? time_to_event.total_seconds() : 1.0e10;
  else if ( util::str_compare_ci( filter, "duration" ) )
    return e -> duration_time().total_seconds();
  else if ( util::str_compare_ci( filter, "cooldown" ) )
    return e -> cooldown_time().total_seconds();
  else if ( util::str_compare_ci( filter, "distance" ) )
    return e -> distance();
  else if ( util::str_compare_ci( filter, "max_distance" ) )
    return e -> max_distance();
  else if ( util::str_compare_ci( filter, "min_distance" ) )
    return e -> min_distance();
  else if ( util::str_compare_ci( filter, "amount" ) )
    return dynamic_cast<damage_event_t*>( e ) -> amount;
  else if ( util::str_compare_ci( filter, "to_pct" ) )
    return dynamic_cast<heal_event_t*>( e ) -> to_pct;
  else if ( util::str_compare_ci( filter, "count" ) )
    return dynamic_cast<adds_event_t*>( e ) -> count;

  // if we have no idea what filter they've specified, return 0
  // todo: should this generate an error or something instead?
  else
    return 0.0;
}
