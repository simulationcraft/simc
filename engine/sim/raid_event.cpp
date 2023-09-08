// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "action/heal.hpp"
#include "action/action.hpp"
#include "action/spell.hpp"
#include "buff/buff.hpp"
#include "dbc/dbc.hpp"
#include "player/pet.hpp"
#include "player/player.hpp"
#include "player/player_demise_event.hpp"
#include "player/pet_spawner.hpp"
#include "raid_event.hpp"
#include "sim/event.hpp"
#include "sim/expressions.hpp"
#include "sim/sim.hpp"
#include "util/rng.hpp"

// ==========================================================================
// Raid Events
// ==========================================================================

namespace
{  // UNNAMED NAMESPACE

struct adds_event_t final : public raid_event_t
{
  double count;
  double health;
  std::string master_str;
  player_t* master;
  std::vector<pet_t*> adds;
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
  std::string race_str;
  race_e race;
  std::string enemy_type_str;
  player_e enemy_type;
  bool same_duration;

  adds_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "adds" ),
      count( 1 ),
      health( 100000 ),
      master_str(),
      master(),
      count_range( false ),
      adds_to_remove( 0 ),
      spawn_x_coord( 0 ),
      spawn_y_coord( 0 ),
      spawn_stacked( 0 ),
      spawn_radius_min( 0 ),
      spawn_radius_max( 0 ),
      spawn_radius( 0 ),
      spawn_angle_start( -1 ),
      spawn_angle_end( -1 ),
      race_str(),
      race( RACE_NONE ),
      enemy_type_str(),
      enemy_type( ENEMY_ADD ),
      same_duration( false )
  {
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
    add_option( opt_string( "race", race_str ) );
    add_option( opt_string( "type", enemy_type_str ) );
    add_option( opt_bool( "same_duration", same_duration ) );
    parse_options( options_str );

    if ( !master_str.empty() )
    {
      master = sim->find_player( master_str );
      if ( !master )
      {
        throw std::invalid_argument( fmt::format( "{} cannot find master '{}'.", *this, master_str ) );
      }
    }

    // If the master is not found, default the master to the first created enemy
    if ( !master )
      master = sim->target_list.data().front();

    if ( !master )
    {
      throw std::invalid_argument( fmt::format( "{} no enemy target available in the sim.", *this ) );
    }

    double overlap    = 1;
    timespan_t min_cd = cooldown;

    if ( cooldown_stddev != timespan_t::zero() )
    {
      min_cd -= cooldown_stddev * 6;
      if ( min_cd <= timespan_t::zero() )
      {
        throw std::invalid_argument(
            fmt::format( "{} the cooldown standard deviation ({}) is too large, "
                         "creating a too short minimum cooldown ({})",
                         *this, cooldown_stddev, min_cd ) );
      }
    }

    if ( min_cd > timespan_t::zero() )
      overlap = duration / min_cd;

    if ( overlap > 1 )
    {
      throw std::invalid_argument(
          fmt::format( "{} does not support overlapping add spawning in a single raid event. "
                       "Duration ({}) > reasonable minimum cooldown ({}).",
                       *this, duration, min_cd ) );
    }

    if ( !race_str.empty() )
    {
      race = util::parse_race_type( race_str );
      if ( race == RACE_UNKNOWN )
      {
        throw std::invalid_argument( fmt::format( "{} could not parse race from '{}'.", *this, race_str ) );
      }
    }
    else if ( !sim->target_race.empty() )
    {
      race = util::parse_race_type( sim->target_race );

      if ( race == RACE_UNKNOWN )
      {
        throw std::invalid_argument(
            fmt::format( "{} could not parse race from sim target race '{}'.", *this, sim->target_race ) );
      }
    }

    if ( !enemy_type_str.empty() )
    {
      enemy_type = util::parse_player_type( enemy_type_str );

      if ( !( enemy_type == ENEMY_ADD || enemy_type == ENEMY_ADD_BOSS ) )
      {
        throw std::invalid_argument( fmt::format( "{} could not parse enemy type from '{}'.", *this, enemy_type_str ) );
      }
    }

    sim->add_waves++;

    if ( fabs( spawn_radius ) > 0 || fabs( spawn_radius_max ) > 0 || fabs( spawn_radius_min ) > 0 )
    {
      sim->distance_targeting_enabled = true;
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

      if ( spawn_angle_start == spawn_angle_end && spawn_angle_start >= 0 )  // Full circle
      {
        spawn_angle_start = 0;
        spawn_angle_end   = 360;
      }
      else  // Not a full circle, or, one/both of the values is not specified
      {
        if ( spawn_x_coord != 0 || spawn_y_coord != 0 )  // Not default placement
        {
          if ( spawn_angle_start < 0 )
          {
            spawn_angle_start = 0;
          }
          if ( spawn_angle_end < 0 )
          {
            spawn_angle_end = 360;
          }
        }
        else if ( spawn_x_coord == 0 && spawn_y_coord == 0 && spawn_angle_start < 0 &&
                  spawn_angle_end < 0 )  // Default placement, only spawn adds on near side
        {
          spawn_angle_start = 90;
          spawn_angle_end   = 270;
        }

        if ( spawn_angle_start < 0 )  // Default start value if not specified
        {
          spawn_angle_start = 90;
        }

        if ( spawn_angle_end < 0 )  // Default end value if not specified
        {
          spawn_angle_end = 270;
        }

        if ( spawn_angle_end < spawn_angle_start )
        {
          spawn_angle_end += 360;
        }
      }

      if ( sim->log )
      {
        sim->out_log.printf( "Spawn Angle set between %f and %f degrees.", spawn_angle_start, spawn_angle_end );
      }
    }

    for ( int i = 0; i < util::ceil( overlap ); i++ )
    {
      for ( unsigned add = 0; add < util::ceil( count + count_range ); add++ )
      {
        std::string add_name_str;

        if ( sim->add_waves > 1 &&
             name.empty() )  // Only add wave to secondary wave that aren't given manual names.
        {
          add_name_str += "Wave";
          add_name_str += util::to_string( sim->add_waves );
          add_name_str += "_";
        }

        add_name_str += name;
        add_name_str += util::to_string( add + 1 );

        pet_t* p = master->create_pet( add_name_str );
        assert( p );
        p->resources.base[ RESOURCE_HEALTH ] = health;
        p->race                              = race;
        p->type                              = enemy_type;
        adds.push_back( p );
      }
    }
  }

  void regenerate_cache()
  {
    for ( auto p : affected_players )
    {
      // Invalidate target caches
      for ( size_t i = 0, end = p->action_list.size(); i < end; i++ )
        p->action_list[ i ]->target_cache.is_valid = false;  // Regenerate Cache.
    }
  }

  void _start() override
  {
    adds_to_remove = static_cast<size_t>(
        util::round( std::max( 0.0, sim->rng().range( count - count_range, count + count_range ) ) ) );

    double x_offset      = 0;
    double y_offset      = 0;
    bool offset_computed = false;

    // Keep track of each add duration so after summons (and dismissals) are done, we can adjust the saved_duration of
    // the add event to match the add with the longest lifetime.
    timespan_t add_duration = saved_duration;
    std::vector<timespan_t> add_lifetimes;

    for ( size_t i = 0; i < adds.size(); i++ )
    {
      if ( i >= adds_to_remove )
      {
        adds[ i ]->dismiss();
        continue;
      }

      if ( std::fabs( spawn_radius_max ) > 0 )
      {
        if ( spawn_stacked == 0 || !offset_computed )
        {
          double angle_start = spawn_angle_start * ( m_pi / 180 );
          double angle_end   = spawn_angle_end * ( m_pi / 180 );
          double angle       = sim->rng().range( angle_start, angle_end );
          double radius      = sim->rng().range( std::fabs( spawn_radius_min ), std::fabs( spawn_radius_max ) );
          x_offset           = radius * cos( angle );
          y_offset           = radius * sin( angle );
          offset_computed    = true;
        }
      }

      if ( !same_duration )
      {
        add_duration = duration_time();
        add_lifetimes.push_back( add_duration );
      }

      adds[ i ]->summon( add_duration );
      adds[ i ]->x_position = x_offset + spawn_x_coord;
      adds[ i ]->y_position = y_offset + spawn_y_coord;

      if ( sim->log )
      {
        if ( x_offset != 0 || y_offset != 0 )
        {
          sim->out_log.printf( "New add spawned at %f, %f.", x_offset, y_offset );
        }
      }
    }

    if ( !add_lifetimes.empty() )
    {
      saved_duration = *range::max_element( add_lifetimes );
    }

    regenerate_cache();

    if ( enemy_type == ENEMY_ADD_BOSS )
    {
      for ( auto p : affected_players )
      {
        p->in_boss_encounter++;
      }
    }
  }

  void _finish() override
  {
    if ( adds.size() < adds_to_remove )
    {
      adds_to_remove = adds.size();
    }
    for ( size_t i = 0; i < adds_to_remove; i++ )
    {
      adds[ i ]->dismiss();
    }

    if ( enemy_type == ENEMY_ADD_BOSS )
    {
      for ( auto p : affected_players )
      {
        assert( p->in_boss_encounter );
        p->in_boss_encounter--;
      }
    }

    // trigger leave combat state callbacks if no adds are remaining
    if ( sim->fight_style == fight_style_e::FIGHT_STYLE_DUNGEON_SLICE && !sim->target_non_sleeping_list.size() )
    {
      for ( auto p : affected_players )
        p->leave_combat();
    }
  }
};

struct pull_event_t final : raid_event_t
{
  struct mob_t : public pet_t
  {
    pull_event_t* pull_event;

    mob_t( player_t* o, util::string_view n = "Mob", pet_e pt = PET_ENEMY ) : pet_t( o->sim, o, n, pt ),
      pull_event( nullptr )
    {
    }

    timespan_t time_to_percent( double percent ) const override
    {
      double target_hp = resources.initial[ RESOURCE_HEALTH ] * percent;
      if ( target_hp >= resources.current[ RESOURCE_HEALTH ])
        return timespan_t::zero();

      // Per-Add DPS Calculation
      // This extrapolates the DTPS of the current add only. Potentially more precise but also volatile
      //double add_dps = ( resources.initial[ RESOURCE_HEALTH ] - resources.current[ RESOURCE_HEALTH ] ) / ( sim->current_time() - arise_time ).total_seconds();
      //if ( add_dps > 0 )
      //{
      //  return timespan_t::from_seconds( ( resources.current[ RESOURCE_HEALTH ] - target_hp ) / add_dps );
      //}

      // Per-Wave DPS Calculation
      // This extrapolates the DTPS of the wave and divides evenly across all targets, less volatile but potentially incaccurate
      int add_count = 0;
      double pull_damage = 0;
      for ( auto add : pull_event->adds_spawner->active_pets() )
      {
        if ( add->is_active() )
          add_count++;

        pull_damage += add->resources.initial[ RESOURCE_HEALTH ] - add->resources.current[ RESOURCE_HEALTH ];
      }

      if ( pull_damage > 0 )
      {
        double pull_dps = pull_damage / ( sim->current_time() - pull_event->spawn_time ).total_seconds();
        return timespan_t::from_seconds( ( resources.current[ RESOURCE_HEALTH ] - target_hp ) / ( pull_dps / add_count ) );
      }
      
      return pet_t::time_to_percent( percent );
    }

    void demise() override
    {
      pet_t::demise();
      
      if ( pull_event )
        pull_event->on_demise();
    }

    void init_resources( bool force ) override
    {
      base_t::init_resources( force );
    }

    resource_e primary_resource() const override
    {
      return RESOURCE_HEALTH;
    }
  };

  struct redistribute_event_t : public event_t
  {
    pull_event_t* pull;

    redistribute_event_t( pull_event_t* pull_ )
      : event_t( *pull_->sim, 1.0_s ),
        pull( pull_ )
    { }

    const char* name() const override
    {
      return "redistribute_event_t";
    }

    void execute() override
    {
      pull->redistribute_event = nullptr;

      double max_hp     = 0.0;
      double current_hp = 0.0;

      auto active_adds = pull->adds_spawner->active_pets();
      for ( auto add : active_adds )
      {
        max_hp += add->resources.initial[ RESOURCE_HEALTH ];
        current_hp += add->resources.current[ RESOURCE_HEALTH ];
      }

      double pct = current_hp / max_hp;

      for ( auto add : active_adds )
      {
        double new_hp = pct * add->resources.initial[ RESOURCE_HEALTH ];
        double old_hp = add->resources.current[ RESOURCE_HEALTH ];
        if ( new_hp > old_hp )
        {
          add->resource_gain( RESOURCE_HEALTH, new_hp - old_hp );
        }
        else if ( new_hp < old_hp )
        {
          add->resource_loss( RESOURCE_HEALTH, old_hp - new_hp );
        }
      }

      pull->redistribute_event = make_event<redistribute_event_t>( sim(), pull );
    }
  };

  player_t* master;
  std::string enemies_str;
  timespan_t delay;
  timespan_t spawn_time;
  int pull;
  bool bloodlust;
  bool shared_health;
  bool spawned;
  bool demised;
  bool has_boss;
  event_t* spawn_event;
  event_t* redistribute_event;

  struct spawn_parameter
  {
    std::string name;
    bool boss = false;
    double health = 0;
    race_e race = RACE_HUMANOID;
  };
  std::vector<spawn_parameter> spawn_parameters;

  spawner::pet_spawner_t<mob_t, player_t>* adds_spawner;

  pull_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "pull" ),
      enemies_str(),
      delay( 0_s ),
      spawn_time( 0_s ),
      pull( 0 ),
      bloodlust( false ),
      shared_health( false ),
      spawned( false ),
      has_boss( false ),
      spawn_event( nullptr ),
      redistribute_event( nullptr )
  {
    add_option( opt_string( "enemies", enemies_str ) );
    add_option( opt_timespan( "delay", delay ) );
    add_option( opt_int( "pull", pull ) );
    add_option( opt_bool( "bloodlust", bloodlust ) );
    add_option( opt_bool( "shared_health", shared_health ) );

    parse_options( options_str );

    if ( pull == 1 )
      first = 0_s;
    else
      first = timespan_t::max();

    cooldown = sim->max_time * 2;

    master = sim->target_list.data().front();
    if ( !master )
    {
      throw std::invalid_argument( fmt::format( "{} no enemy available in the sim.", *this ) );
    }

    std::string spawner_name = master->name();
    spawner_name += "_pull_spawner";
    adds_spawner = dynamic_cast<spawner::pet_spawner_t<mob_t, player_t>*>( master->find_spawner( spawner_name ) );

    if ( !adds_spawner )
    {
      adds_spawner = new spawner::pet_spawner_t<mob_t, player_t>( spawner_name, master );
    }

    if ( enemies_str.empty() )
    {
      throw std::invalid_argument( fmt::format( "{} no enemies string.", *this ) );
    }
    else
    {
      auto enemy_splits = util::string_split<util::string_view>( enemies_str, "|" );
      if ( enemy_splits.empty() )
      {
        throw std::invalid_argument( fmt::format( "{} at least one enemy is required.", *this ) );
      }
      else
      {
        for ( const util::string_view enemy_str : util::string_split<util::string_view>( enemies_str, "|" ) )
        {
          auto splits = util::string_split<util::string_view>( enemy_str, ":" );
          if ( splits.size() < 2 )
          {
            throw std::invalid_argument( fmt::format( "{} bad enemy string '{}'.", *this, enemy_str ) );
          }
          else
          {
            spawn_parameter spawn;

            if ( util::starts_with( splits[ 0 ], "BOSS_" ) )
            {
              spawn.boss = true;
              has_boss   = true;
            }

            spawn.name   = splits[ 0 ];
            spawn.health = util::to_double( splits[ 1 ] );

            if ( splits.size() > 2 )
              spawn.race = util::parse_race_type( util::tokenize_fn( splits[ 2 ] ) );

            spawn_parameters.emplace_back( spawn );
          }
        }

        // Sort adds by descending HP order to improve retargeting logic if the dungeon_route_smart_targeting option is
        // set to true
        if ( sim->dungeon_route_smart_targeting )
        {
          range::sort( spawn_parameters,
                       []( const spawn_parameter a, const spawn_parameter b ) { return a.health > b.health; } );
        }
      }
    }
  }

  void on_demise()
  {
    // don't schedule another pull until all adds from this one are dead
    for ( auto add : adds_spawner->active_pets() )
    {
      if ( add->is_active() )
        return;
    }

    if ( demised || !spawned )
      return;

    demised = true;
    sim->print_log( "Finished Pull {} in {:.1f} seconds", pull, ( sim->current_time() - spawn_time ).total_seconds() );

    event_t::cancel( redistribute_event );

    if ( has_boss )
    {
      for ( auto p : affected_players )
      {
        assert( p->in_boss_encounter );
        p->in_boss_encounter--;
      }
    }

    // trigger leave combat state callbacks if no adds are remaining
    if ( !sim->target_non_sleeping_list.size() )
    {
      for ( auto p : affected_players )
        p->leave_combat();
    }

    // find the next pull and spawn it
    if ( auto next = next_pull() )
    {
      make_event( *sim, next->delay, [ next ] { next->spawn_pull(); } );
      return;
    }

    // if no suitable pull is found, end the iteration
    make_event<player_demise_event_t>( *sim, *master );
  }

  void regenerate_cache()
  {
    for ( auto p : affected_players )
    {
      for ( size_t i = 0, end = p->action_list.size(); i < end; i++ )
        p->action_list[ i ]->target_cache.is_valid = false;
    }
  }

  void _start() override
  {
    if ( !spawned )
      make_event( *sim, delay, [ this ] { spawn_pull(); } );
  }

  void _finish() override
  {
  }

  bool active()
  {
    if ( spawned )
    {
      for ( auto add : adds_spawner->active_pets() )
      {
        if ( add->is_active() )
          return true;
      }
    }
    else
    {
      if ( spawn_event && spawn_event->remains() >= 0_s )
        return true;
    }

    return false;
  }

  double time_to_die()
  {
    double pull_dtps = 0;
    double pull_hp = 0;

    for ( pet_t* add : adds_spawner->active_pets() )
    {
      pull_dtps += ( add->resources.initial[ RESOURCE_HEALTH ] - add->resources.current[ RESOURCE_HEALTH ] ) / ( sim->current_time() - add->arise_time ).total_seconds();
      pull_hp += add->resources.current[ RESOURCE_HEALTH ];
    }

    return pull_hp / pull_dtps;
  }

  pull_event_t* next_pull()
  {
    for ( auto& raid_event : sim->raid_events )
    {
      if ( raid_event->type == "pull" )
      {
        auto pull_event = dynamic_cast<pull_event_t*>( raid_event.get() );
        if ( pull_event && pull_event->pull == pull + 1 )
        {
          return pull_event;
        }
      }
    }
    return nullptr;
  }

  void spawn_pull()
  {
    if ( spawned )
      return;
    
    spawned = true;
    spawn_time = sim->current_time();

    if ( bloodlust )
    {
      if ( !sim->single_actor_batch )
      {
        for ( auto* p : sim->player_non_sleeping_list )
        {
           if ( p->is_pet() )
            continue;

          p->buffs.bloodlust->trigger();
          p->buffs.exhaustion->trigger();
        }
      }
      else
      {
        auto p = sim->player_no_pet_list[ sim->current_index ];
        if ( p )
        {
          p->buffs.bloodlust->trigger();
          p->buffs.exhaustion->trigger();
        }
      }
    }    

    auto adds = adds_spawner->spawn( as<unsigned>( spawn_parameters.size() ) );
    double total_health = 0;
    for ( size_t i = 0; i < adds.size(); i++ )
    {
      adds[ i ]->resources.base[ RESOURCE_HEALTH ] = spawn_parameters[ i ].health;
      adds[ i ]->resources.infinite_resource[ RESOURCE_HEALTH ] = false;
      adds[ i ]->init_resources( true );
      adds[ i ]->pull_event = this;
      adds[ i ]->type = spawn_parameters[ i ].boss ? ENEMY_ADD_BOSS : ENEMY_ADD;
      adds[ i ]->race = spawn_parameters[ i ].race;

      // Only for use with log output options as it makes the report strange but log much better
      if ( sim->log )
      {
        sim->print_log( "Renaming {} to {}", adds[ i ]->name_str, spawn_parameters[ i ].name );
        adds[ i ]->full_name_str = adds[ i ]->name_str = spawn_parameters[ i ].name;
        total_health += spawn_parameters[ i ].health;
      }
    }

    if ( shared_health )
      redistribute_event = make_event<redistribute_event_t>( *sim, this );

    sim->print_log( "Spawned Pull {}: {} mobs with {} total health, {:.1f}s delay from previous",
                    pull, adds.size(), total_health, delay.total_seconds() );

    regenerate_cache();

    if ( has_boss )
    {
      for ( auto p : affected_players )
      {
        p->in_boss_encounter++;
      }
    }
  }

  void reset() override
  {
    raid_event_t::reset();

    spawned = false;
    demised = false;
    redistribute_event = nullptr;
  }
};

struct move_enemy_t final : public raid_event_t
{
  double x_coord;
  double y_coord;
  std::string enemy_name;
  player_t* enemy;
  double original_x;
  double original_y;

  move_enemy_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "move_enemy" ),
      x_coord( 0.0 ),
      y_coord( 0.0 ),
      enemy_name(),
      enemy( nullptr ),
      original_x( 0.0 ),
      original_y( 0.0 )
  {
    add_option( opt_float( "x", x_coord ) );
    add_option( opt_float( "y", y_coord ) );
    add_option( opt_string( "enemy_name", enemy_name ) );
    parse_options( options_str );

    enemy                           = sim->find_player( enemy_name );
    sim->distance_targeting_enabled = true;

    if ( !enemy )
    {
      throw std::invalid_argument(
          fmt::format( "Move enemy event cannot be created, there is no enemy named '{}'.", enemy_name ) );
    }
  }

  void regenerate_cache()
  {
    for ( auto p : affected_players )
    {
      // Invalidate target caches
      for ( size_t i = 0, end = p->action_list.size(); i < end; i++ )
        p->action_list[ i ]->target_cache.is_valid = false;  // Regenerate Cache.
    }
  }

  void reset() override
  {
    raid_event_t::reset();

    if ( enemy )
    {
      enemy->x_position = enemy->default_x_position;
      enemy->y_position = enemy->default_y_position;
    }
  }

  void _start() override
  {
    if ( enemy )
    {
      original_x        = enemy->x_position;
      original_y        = enemy->y_position;
      enemy->x_position = x_coord;
      enemy->y_position = y_coord;
      regenerate_cache();
    }
  }

  void _finish() override
  {
    if ( enemy )
    {
      enemy->x_position = enemy->default_x_position;
      enemy->y_position = enemy->default_y_position;
      regenerate_cache();
    }
  }
};

// Casting ==================================================================

struct casting_event_t final : public raid_event_t
{
  casting_event_t( sim_t* s, util::string_view options_str ) : raid_event_t( s, "casting" )
  {
    parse_options( options_str );
  }

  void _start() override
  {
    sim->target->debuffs.casting->increment();
  }

  void _finish() override
  {
    sim->target->debuffs.casting->decrement();
  }
};

// Distraction ==============================================================

struct distraction_event_t final : public raid_event_t
{
  double skill;

  distraction_event_t( sim_t* s, util::string_view options_str ) : raid_event_t( s, "distraction" ), skill( 0.2 )
  {
    players_only = true;  // Pets shouldn't have less "skill"

    add_option( opt_float( "skill", skill ) );
    parse_options( options_str );
  }

  void _start() override
  {
    for ( auto p : affected_players )
    {
      p->current.skill_debuff += skill;
    }
  }

  void _finish() override
  {
    for ( auto p : affected_players )
    {
      p->current.skill_debuff -= skill;
    }
  }
};

// Invulnerable =============================================================

// TODO: Support more than sim -> target
struct invulnerable_event_t final : public raid_event_t
{
  bool retarget;
  player_t* target;

  invulnerable_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "invulnerable" ), retarget( false ), target( s->target )
  {
    add_option( opt_bool( "retarget", retarget ) );
    add_option( opt_func( "target", [this](sim_t* sim, util::string_view name, util::string_view value) { return parse_target(sim, name, value); } ) );

    parse_options( options_str );
  }

  bool parse_target( sim_t* /* sim */, util::string_view /* name */, util::string_view value )
  {
    auto it = range::find_if( sim->target_list, [ &value ]( const player_t* target ) {
      return util::str_compare_ci( value, target->name() );
    } );

    if ( it != sim->target_list.end() )
    {
      target = *it;
      return true;
    }
    else
    {
      sim->error( "Unknown invulnerability raid event target '{}'", value );
      return false;
    }
  }

  void _start() override
  {
    target->clear_debuffs();
    target->debuffs.invulnerable->increment();

    range::for_each( sim->player_non_sleeping_list, []( player_t* p ) {
      p->in_combat =
          true;  // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p->halt();
    } );

    if ( retarget )
    {
      range::for_each( sim->player_non_sleeping_list,
                       [ this ]( player_t* p ) { p->acquire_target( retarget_source::ACTOR_INVULNERABLE, target ); } );
    }
  }

  void _finish() override
  {
    target->debuffs.invulnerable->decrement();

    if ( retarget )
    {
      range::for_each( sim->player_non_sleeping_list,
                       [ this ]( player_t* p ) { p->acquire_target( retarget_source::ACTOR_VULNERABLE, target ); } );
    }
  }
};

// Flying ===================================================================

struct flying_event_t final : public raid_event_t
{
  flying_event_t( sim_t* s, util::string_view options_str ) : raid_event_t( s, "flying" )
  {
    parse_options( options_str );
  }

  void _start() override
  {
    sim->target->debuffs.flying->increment();
  }

  void _finish() override
  {
    sim->target->debuffs.flying->decrement();
  }
};

// Movement =================================================================

struct movement_ticker_t : public event_t
{
  const std::vector<player_t*>& players;
  timespan_t duration;

  movement_ticker_t( sim_t& s, const std::vector<player_t*>& p, timespan_t d = timespan_t::zero() )
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
  }
  const char* name() const override
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

struct movement_event_t final : public raid_event_t
{
  double move_distance;
  movement_direction_type direction;
  std::string move_direction;
  double distance_range;
  double move;
  double distance_min;
  double distance_max;
  double avg_player_movement_speed;

  movement_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "movement" ),
      move_distance( 0 ),
      direction( movement_direction_type::TOWARDS ),
      distance_range( 0 ),
      move(),
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

    if ( duration > timespan_t::zero() )
    {
      move_distance = duration.total_seconds() * avg_player_movement_speed;
    }

    double cooldown_move = cooldown.total_seconds();
    if ( ( move_distance / avg_player_movement_speed ) > cooldown_move )
    {
      sim->error(
          "{} average player movement time ({}) is longer than cooldown movement time ({}). "
          "Capping it the lower value.",
          *this, move_distance / avg_player_movement_speed, cooldown_move );
      move_distance = cooldown_move * avg_player_movement_speed;
    }

    double max = ( move_distance + distance_range ) / avg_player_movement_speed;
    if ( max > cooldown_move )
    {
      distance_range = ( max - cooldown_move ) * avg_player_movement_speed;
    }

    if ( distance_max < distance_min )
    {
      distance_max = distance_min;
    }

    if ( distance_max / avg_player_movement_speed > cooldown_move )
    {
      distance_max = cooldown_move * avg_player_movement_speed;
    }
    if ( distance_min / avg_player_movement_speed > cooldown_move )
    {
      distance_min = cooldown_move * avg_player_movement_speed;
    }

    if ( move_distance > 0 )
    {
      type = "movement_distance";
    }
    if ( !move_direction.empty() )
    {
      direction = util::parse_movement_direction( move_direction );
    }
  }

  void _start() override
  {
    movement_direction_type m = direction;
    if ( direction == movement_direction_type::RANDOM )
    {
      auto min           = static_cast<int>( movement_direction_type::OMNI );
      auto max_exclusive = static_cast<int>( movement_direction_type::RANDOM );
      m                  = static_cast<movement_direction_type>( int( sim->rng().range( min, max_exclusive ) ) );
    }

    if ( distance_range > 0 )
    {
      move = sim->rng().range( move_distance - distance_range, move_distance + distance_range );
      if ( move < distance_min )
        move = distance_min;
      else if ( move > distance_max )
        move = distance_max;
    }
    else if ( distance_min > 0 || distance_max > 0 )
      move = sim->rng().range( distance_min, distance_max );
    else
      move = move_distance;

    if ( move <= 0.0 )
      return;

    for ( auto p : affected_players )
    {
      p->trigger_movement( move, m );

      if ( p->buffs.stunned->check() )
        continue;
      p->in_combat =
          true;  // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p->moving();
    }

    if ( !affected_players.empty() )
    {
      make_event<movement_ticker_t>( *sim, *sim, affected_players );
    }
  }

  void _finish() override
  {
  }
};

// Stun =====================================================================

struct stun_event_t final : public raid_event_t
{
  stun_event_t( sim_t* s, util::string_view options_str ) : raid_event_t( s, "stun" )
  {
    parse_options( options_str );
  }

  void _start() override
  {
    for ( auto p : affected_players )
    {
      p->buffs.stunned->increment();
      p->in_combat =
          true;  // FIXME? this is done to ensure we don't end up in infinite loops of non-harmful actions with gcd=0
      p->stun();
    }
  }

  void _finish() override
  {
    for ( auto p : affected_players )
    {
      p->buffs.stunned->decrement();
    }
  }
};

// Interrupt ================================================================

struct interrupt_event_t final : public raid_event_t
{
  interrupt_event_t( sim_t* s, util::string_view options_str ) : raid_event_t( s, "interrupt" )
  {
    parse_options( options_str );
  }

  void _start() override
  {
    for ( auto p : affected_players )
    {
      p->interrupt();
    }
  }

  void _finish() override
  {
  }
};

// Damage ===================================================================

struct damage_event_t final : public raid_event_t
{
  double amount;
  double amount_range;
  spell_t* raid_damage;
  school_e damage_type;

  damage_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "damage" ), amount( 1 ), amount_range( 0 ), raid_damage( nullptr )
  {
    std::string type_str = "holy";
    add_option( opt_float( "amount", amount ) );
    add_option( opt_float( "amount_range", amount_range ) );
    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );

    if ( duration != timespan_t::zero() )
    {
      throw std::invalid_argument( "Damage raid event does not allow a duration." );
    }

    type        = "raid_damage_" + type_str;
    damage_type = util::parse_school_type( type_str );
  }

  void _start() override
  {
    if ( !raid_damage )
    {
      struct raid_damage_t : public spell_t
      {
        raid_damage_t( util::string_view n, player_t* player, school_e s ) : spell_t( n, player )
        {
          school      = s;
          may_crit    = false;
          background  = true;
          trigger_gcd = timespan_t::zero();
        }
      };

      raid_damage = new raid_damage_t( type, sim->target, damage_type );
      raid_damage->init();
    }

    for ( auto p : affected_players )
    {
      raid_damage->base_dd_min = raid_damage->base_dd_max =
          sim->rng().range( amount - amount_range, amount + amount_range );
      raid_damage->target = p;
      raid_damage->execute();
    }
  }

  void _finish() override
  {
  }
};

// Heal =====================================================================

struct heal_event_t final : public raid_event_t
{
  double amount;
  double amount_range;
  double to_pct, to_pct_range;

  heal_t* raid_heal;

  heal_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "heal" ), amount( 1 ), amount_range( 0 ), to_pct( 0 ), to_pct_range( 0 ), raid_heal( nullptr )
  {
    add_option( opt_float( "amount", amount ) );
    add_option( opt_float( "amount_range", amount_range ) );
    add_option( opt_float( "to_pct", to_pct ) );
    add_option( opt_float( "to_pct_range", to_pct_range ) );
    parse_options( options_str );

    if ( duration != timespan_t::zero() )
    {
      throw std::invalid_argument( "Heal raid event does not allow a duration." );
    }
  }

  void _start() override
  {
    if ( !raid_heal )
    {
      struct raid_heal_t : public heal_t
      {
        raid_heal_t( util::string_view n, player_t* player ) : heal_t( n, player )
        {
          school      = SCHOOL_HOLY;
          may_crit    = false;
          background  = true;
          trigger_gcd = timespan_t::zero();
        }
      };

      raid_heal = new raid_heal_t( name.empty() ? type : name, sim->target );
      raid_heal->init();
    }

    for ( auto p : affected_players )
    {
      double amount_to_heal = 0.0;

      // to_pct tells this event to heal each player up to a percent of their max health
      if ( to_pct > 0 )
      {
        double pct_actual = to_pct;
        if ( to_pct_range > 0 )
          pct_actual = sim->rng().range( to_pct - to_pct_range, to_pct + to_pct_range );

        sim->print_debug( "{} heals {} {}% ({}) of max health, current health {}", *this, p->name(), pct_actual,
                          p->resources.max[ RESOURCE_HEALTH ] * pct_actual / 100,
                          p->resources.current[ RESOURCE_HEALTH ] );

        amount_to_heal = p->resources.max[ RESOURCE_HEALTH ] * pct_actual / 100;
        amount_to_heal -= p->resources.current[ RESOURCE_HEALTH ];
      }
      else
      {
        amount_to_heal = sim->rng().range( amount - amount_range, amount + amount_range );
      }

      // heal if there's any healing to be done
      if ( amount_to_heal > 0.0 )
      {
        raid_heal->base_dd_min = raid_heal->base_dd_max = amount_to_heal;
        raid_heal->target                               = p;
        raid_heal->execute();

        sim->print_log( "Event {} healed {} for '{}' (before player modifiers).", name, p->name(),
                        amount_to_heal );
      }
    }
  }

  void _finish() override
  {
  }
};

// Damage Taken Debuff=========================================================

struct damage_taken_debuff_event_t final : public raid_event_t
{
  int amount;

  damage_taken_debuff_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "damage_taken" ), amount( 1 )
  {
    add_option( opt_int( "amount", amount ) );
    parse_options( options_str );

    if ( duration != timespan_t::zero() )
    {
      throw std::invalid_argument( "Damage taken raid event does not allow a duration." );
    }
  }

  void _start() override
  {
    for ( auto p : affected_players )
    {
      sim->print_log( "{} gains {} stacks of damage_taken debuff from {}.", p->name(), amount, *this );

      if ( p->debuffs.damage_taken )
        p->debuffs.damage_taken->trigger( amount );
    }
  }

  void _finish() override
  {
  }
};

// Damage Done Buff =======================================================

struct damage_done_buff_event_t final : public raid_event_t
{
  double multiplier;

  damage_done_buff_event_t( sim_t* s, util::string_view options_str )
    : raid_event_t( s, "damage_done" ), multiplier( 1.0 )
  {
    add_option( opt_float( "multiplier", multiplier ) );
    parse_options( options_str );
  }

  void _start() override
  {
    for ( auto p : affected_players )
    {
      if ( p->buffs.damage_done )
        p->buffs.damage_done->increment( 1, multiplier );
    }
  }

  void _finish() override
  {
    for ( auto p : affected_players )
    {
      if ( p->buffs.damage_done )
        p->buffs.damage_done->decrement();
    }
  }
};

// Buff Raid Event ==========================================================

struct buff_raid_event_t final : public raid_event_t
{
  std::unordered_map<size_t, buff_t*> buff_list;
  std::string buff_str;
  unsigned stacks;

  buff_raid_event_t( sim_t* s, std::string_view options_str ) : raid_event_t( s, "buff" ), stacks( 1 )
  {
    add_option( opt_string( "buff_name", buff_str ) );
    add_option( opt_uint( "stacks", stacks ) );
    parse_options( options_str );

    players_only = true;

    if ( buff_str.empty() )
      throw std::invalid_argument( fmt::format( "{} you must specify a buff_name.", *this ) );
  }

  void _start() override
  {
    for ( auto p : affected_players )
    {
      auto& b = buff_list[ p->actor_index ];
      if ( !b )
        b = buff_t::find( p, buff_str );

      if ( b )
      {
        b->trigger( stacks, duration > 0_ms ? duration : timespan_t::min() );
      }
      else
      {
        sim->error( "Error: Invalid buff raid event, buff_name '{}' not found on player '{}'.", buff_str, p->name() );
        sim->cancel();
      }
    }
  }

  void _finish() override {}
};

// Vulnerable ===============================================================

struct vulnerable_event_t final : public raid_event_t
{
  double multiplier;
  player_t* target = nullptr;

  vulnerable_event_t( sim_t* s, util::string_view options_str ) : raid_event_t( s, "vulnerable" ), multiplier( 2.0 )
  {
    add_option( opt_float( "multiplier", multiplier ) );
    add_option( opt_func( "target", [this](sim_t* sim, util::string_view name, util::string_view value) { return parse_target(sim, name, value); } ) );
    parse_options( options_str );
  }

  bool parse_target( sim_t* /* sim */, util::string_view /* name */, util::string_view value )
  {
    auto it = range::find_if( sim->target_list, [ &value ]( const player_t* target ) {
      return util::str_compare_ci( value, target->name() );
    } );

    if ( it != sim->target_list.end() )
    {
      target = *it;
      return true;
    }
    else
    {
      sim->error( "Unknown vulnerability raid event target '{}'", value );
      return true;
    }
  }

  void _start() override
  {
    if ( target )
      target->debuffs.vulnerable->increment( 1, multiplier );
    else
      sim->target->debuffs.vulnerable->increment( 1, multiplier );
  }

  void _finish() override
  {
    if ( target )
      target->debuffs.vulnerable->decrement();
    else
      sim->target->debuffs.vulnerable->decrement();
  }
};

// Position Switch ==========================================================

struct position_event_t : public raid_event_t
{
  position_event_t( sim_t* s, util::string_view options_str ) : raid_event_t( s, "position_switch" )
  {
    parse_options( options_str );
  }

  void _start() override
  {
    for ( auto p : affected_players )
    {
      if ( p->position() == POSITION_BACK )
        p->change_position( POSITION_FRONT );
      else if ( p->position() == POSITION_RANGED_BACK )
        p->change_position( POSITION_RANGED_FRONT );
    }
  }

  void _finish() override
  {
    for ( auto p : affected_players )
    {
      p->change_position( p->initial.position );
    }
  }
};

raid_event_t* get_next_raid_event( const std::vector<raid_event_t*>& matching_events )
{
  raid_event_t* result     = nullptr;
  timespan_t time_to_event = timespan_t::from_seconds( -1 );

  for ( const auto& event : matching_events )
  {
    if ( time_to_event < timespan_t::zero() || event->until_next() < time_to_event )
    {
      result        = event;
      time_to_event = result->until_next();
    }
  }

  return result;
}

/**
 * Create a list of raid events which are up, sorted descending by remaining uptime.
 */
std::vector<raid_event_t*> get_longest_active_raid_events( const std::vector<raid_event_t*>& matching_events )
{
  std::vector<raid_event_t*> result;

  for ( const auto& event : matching_events )
  {
    if ( event->up() )
    {
      result.push_back( event );
    }
  }

  range::sort( result, []( const raid_event_t* l, const raid_event_t* r ) {
    timespan_t lv = l->remains();
    timespan_t rv = r->remains();
    if ( lv == rv )
    {
      // Integer comparison to break ties
      return l < r;
    }

    return lv > rv;
  } );

  return result;
}

}  // UNNAMED NAMESPACE

raid_event_t::raid_event_t( sim_t* s, util::string_view type )
  : sim( s ),
    name(),
    type( type ),
    num_starts( 0 ),
    first( timespan_t::min() ),
    last( timespan_t::min() ),
    first_pct( -1.0 ),
    last_pct( -1.0 ),
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
    force_stop( false ),
    player_chance( 1.0 ),
    affected_role( ROLE_NONE ),
    player_if_expr_str(),
    saved_duration( timespan_t::zero() ),
    player_expressions(),
    is_up( false ),
    activation_status( activation_status_e::not_yet_activated ),
    cooldown_event(),
    duration_event(),
    start_event(),
    end_event()
{
  add_option( opt_string( "name", name ) );
  add_option( opt_timespan( "first", first, timespan_t::zero(), timespan_t::max() ) );
  add_option( opt_timespan( "last", last, timespan_t::zero(), timespan_t::max() ) );
  add_option( opt_float( "first_pct", first_pct, 0.0, 100 ) );
  add_option( opt_float( "last_pct", last_pct, 0.0, 100 ) );
  add_option( opt_timespan( "period", cooldown ) );
  add_option( opt_timespan( "cooldown", cooldown ) );
  add_option( opt_timespan( "cooldown_stddev", cooldown_stddev ) );
  add_option( opt_timespan( "cooldown_min", cooldown_min ) );
  add_option( opt_timespan( "cooldown_max", cooldown_max ) );
  add_option( opt_timespan( "duration", duration ) );
  add_option( opt_timespan( "duration_stddev", duration_stddev ) );
  add_option( opt_timespan( "duration_min", duration_min ) );
  add_option( opt_timespan( "duration_max", duration_max ) );
  add_option( opt_bool( "players_only", players_only ) );
  add_option( opt_float( "player_chance", player_chance, 0.0, 1.0 ) );
  add_option( opt_float( "distance_min", distance_min ) );
  add_option( opt_float( "distance_max", distance_max ) );
  add_option( opt_string( "affected_role", affected_role_str ) );
  add_option( opt_string( "player_if", player_if_expr_str ) );
  add_option( opt_bool( "force_stop", force_stop ) );
}

timespan_t raid_event_t::cooldown_time()
{
  timespan_t time;

  if ( num_starts == 0 )
  {
    time = timespan_t::zero();
  }
  else
  {
    time = sim->rng().gauss( cooldown, cooldown_stddev );

    time = clamp( time, cooldown_min, cooldown_max );
  }

  return time;
}

timespan_t raid_event_t::duration_time()
{
  timespan_t time = sim->rng().gauss( duration, duration_stddev );

  time = clamp( time, duration_min, duration_max );

  return time;
}

timespan_t raid_event_t::next_time() const
{
  return sim->current_time() + until_next();
}

timespan_t raid_event_t::until_next() const
{
  if ( start_event )
  {
    return start_event->remains();
  }
  if ( first_pct != -1 && num_starts == 0 && !sim->fixed_time && sim->current_iteration != 0 )
  {
    return sim->target->time_to_percent( first_pct );
  }
  if ( cooldown_event )
  {
    return cooldown_event->remains();
  }
  if ( duration_event )
  {
    return duration_event->remains() + cooldown;  // avg. estimate
  }

  return timespan_t::max();
}

/**
 * Remaining duration of currently active raid event.
 *
 * Returns timespan_t::max() if raid event is not active.
 */
timespan_t raid_event_t::remains() const
{
  assert( is_up );
  return duration_event ? duration_event->remains() : timespan_t::max();
}

/**
 * Check if raid event is currently up.
 */
bool raid_event_t::up() const
{
  assert( is_up == static_cast<bool>( duration_event ) );
  return is_up;
}

void raid_event_t::start()
{
  sim->print_log( "{} starts.", *this );

  num_starts++;
  is_up = true;

  affected_players.clear();

  for ( auto& p : sim->player_non_sleeping_list )
  {
    // Filter players
    if ( filter_player( p ) )
      continue;

    auto& expr_uptr = player_expressions[ p->actor_index ];
    if ( !expr_uptr )
    {
      try
      {
        expr_uptr = expr_t::parse( p, player_if_expr_str, false );
      }
      catch ( const std::exception& e )
      {
        sim->error( "{} player_if expression error '{}': {}", *this, player_if_expr_str, e.what() );
        sim->cancel();
      }
    }

    if ( expr_uptr && !expr_uptr->success() )
    {
      continue;
    }

    affected_players.push_back( p );
  }

  _start();
}

void raid_event_t::finish()
{
  // Make sure we dont have any players which were active on start, but are now sleeping
  auto filter_sleeping = []( const player_t* p ) { return p->is_sleeping(); };
  affected_players.erase( std::remove_if( affected_players.begin(), affected_players.end(), filter_sleeping ),
                          affected_players.end() );

  is_up = false;

  _finish();

  sim->print_log( "{} finishes.", *this );
}

/**
 * Raid event activation. When triggerd, raid event scheduling starts.
 */
void raid_event_t::activate()
{
  if ( activation_status == activation_status_e::deactivated )
  {
    sim->print_debug( "{} already deactivated. (last/last_pct happened before first/last_pct).", *this );
    return;
  }
  if ( activation_status == activation_status_e::activated )
  {
    // Already activated, do nothing.
    return;
  }
  sim->print_debug( "{} activated (first/first_pct reached).", *this );
  activation_status = activation_status_e::activated;
  schedule();
}

/**
 * Raid event deactivatin. When called, raid event scheduling stops.
 * Without force_stop, a currently up raid event will still finish.
 * With force_stop, the currently up raid event will be stopped directly.
 */
void raid_event_t::deactivate()
{
  sim->print_debug( "{} deactivated (last/last_pct reached).", *this );
  activation_status = activation_status_e::deactivated;
  event_t::cancel( cooldown_event );
  if ( force_stop )
  {
    sim->print_debug( "{} is force stopped.", *this );
    event_t::cancel( duration_event );
    finish();
  }
}

void raid_event_t::combat_begin()
{
  struct end_event_t : public event_t
  {
    raid_event_t* raid_event;

    end_event_t( sim_t& s, raid_event_t* re, timespan_t time ) : event_t( s, time ), raid_event( re )
    {
    }

    const char* name() const override
    {
      return raid_event->type.c_str();
    }

    void execute() override
    {
      raid_event->deactivate();
      raid_event->end_event = nullptr;
    }
  };

  struct start_event_t : public event_t
  {
    raid_event_t* raid_event;

    start_event_t( sim_t& s, raid_event_t* re, timespan_t time ) : event_t( s, time ), raid_event( re )
    {
    }

    const char* name() const override
    {
      return raid_event->type.c_str();
    }

    void execute() override
    {
      raid_event->activate();
      raid_event->start_event = nullptr;
    }
  };

  if ( last_pct == -1 && last > timespan_t::zero() )
  {
    end_event = make_event<end_event_t>( *sim, *sim, this, last );
  }
  if ( last_pct != -1 && ( sim->current_iteration == 0 || sim->fixed_time ) )
  {
    // There is no resource callback from fluffy pillow in these circumstances, thus use time based events as well.
    timespan_t end_time = ( 1.0 - last_pct / 100.0 ) * sim->expected_iteration_time;
    end_event           = make_event<end_event_t>( *sim, *sim, this, end_time );
  }

  if ( first_pct == -1 )
  {
    timespan_t start_time = std::max( first, timespan_t::zero() );
    start_event           = make_event<start_event_t>( *sim, *sim, this, start_time );
  }
  if ( first_pct != -1 && ( sim->current_iteration == 0 || sim->fixed_time ) )
  {
    // There is no resource callback from fluffy pillow in these circumstances, thus use time based events as well.
    timespan_t start_time = ( 1.0 - first_pct / 100.0 ) * sim->expected_iteration_time;
    start_event           = make_event<start_event_t>( *sim, *sim, this, start_time );
  }
}

void raid_event_t::schedule()
{
  sim->print_debug( "Scheduling {}", *this );

  struct duration_event_t : public event_t
  {
    raid_event_t* raid_event;

    duration_event_t( sim_t& s, raid_event_t* re, timespan_t time ) : event_t( s, time ), raid_event( re )
    {
    }

    const char* name() const override
    {
      return raid_event->type.c_str();
    }

    void execute() override
    {
      if ( raid_event->is_up )
      {
        raid_event->finish();
      }
      raid_event->duration_event = nullptr;
    }
  };

  struct cooldown_event_t : public event_t
  {
    raid_event_t* raid_event;

    cooldown_event_t( sim_t& s, raid_event_t* re, timespan_t time ) : event_t( s, time ), raid_event( re )
    {
    }

    const char* name() const override
    {
      return raid_event->type.c_str();
    }

    void execute() override
    {
      raid_event->saved_duration = raid_event->duration_time();
      raid_event->start();

      timespan_t ct = raid_event->cooldown_time();

      if ( ct <= raid_event->saved_duration )
        ct = raid_event->saved_duration + timespan_t::from_seconds( 0.01 );

      if ( raid_event->saved_duration > timespan_t::zero() )
      {
        raid_event->duration_event =
            make_event<duration_event_t>( sim(), sim(), raid_event, raid_event->saved_duration );
      }
      else
        raid_event->finish();

      if ( raid_event->activation_status == activation_status_e::activated )
      {
        raid_event->cooldown_event = make_event<cooldown_event_t>( sim(), sim(), raid_event, ct );
      }
      else
      {
        raid_event->cooldown_event = nullptr;
      }
    }
  };

  cooldown_event = make_event<cooldown_event_t>( *sim, *sim, this, cooldown_time() );
}

// raid_event_t::reset ======================================================

void raid_event_t::reset()
{
  num_starts        = 0;
  is_up             = false;
  activation_status = activation_status_e::not_yet_activated;
  event_t::cancel( cooldown_event );
  event_t::cancel( duration_event );
  event_t::cancel( start_event );
  event_t::cancel( end_event );

  if ( cooldown_min == timespan_t::zero() )
    cooldown_min = cooldown * 0.5;
  if ( cooldown_max == timespan_t::zero() )
    cooldown_max = cooldown * 1.5;

  if ( duration_min == timespan_t::zero() )
    duration_min = duration * 0.5;
  if ( duration_max == timespan_t::zero() )
    duration_max = duration * 1.5;

  affected_players.clear();
}

// raid_event_t::parse_options ==============================================

void raid_event_t::parse_options( util::string_view options_str )
{
  if ( options_str.empty() )
    return;

  opts::parse( sim, type, options, options_str,
               [ this ]( opts::parse_status status, util::string_view name, util::string_view value ) {
                 // Fail parsing if strict parsing is used and the option is not found
                 if ( sim->strict_parsing && status == opts::parse_status::NOT_FOUND )
                 {
                   return opts::parse_status::FAILURE;
                 }

                 // .. otherwise, just warn that there's an unknown option
                 if ( status == opts::parse_status::NOT_FOUND )
                 {
                   sim->error( "Warning: Unknown raid event '{}' option '{}' with value '{}', ignoring", name, name,
                               value );
                 }

                 return status;
               } );

  if ( !affected_role_str.empty() )
  {
    affected_role = util::parse_role_type( affected_role_str );
    if ( affected_role == ROLE_NONE )
    {
      throw std::invalid_argument( fmt::format( "Unknown affected role '{}'.", affected_role_str ) );
    }
  }

  if ( first_pct != -1 && first >= timespan_t::zero() )
  {
    throw std::invalid_argument( "first= and first_pct= cannot be used together." );
  }
  if ( last_pct != -1 && last >= timespan_t::zero() )
  {
    throw std::invalid_argument( "last= and last_pct= cannot be used together." );
  }

  if ( last_pct != -1 && !sim->fixed_time )
  {
    assert( sim->target );
    sim->target->register_resource_callback(
        RESOURCE_HEALTH, last_pct, [ this ]() { deactivate(); }, true );
  }

  if ( first_pct != -1 && !sim->fixed_time )
  {
    assert( sim->target );
    sim->target->register_resource_callback(
        RESOURCE_HEALTH, first_pct, [ this ]() { activate(); }, true );
  }
}

// raid_event_t::create =====================================================

std::unique_ptr<raid_event_t> raid_event_t::create( sim_t* sim, util::string_view name, util::string_view options_str )
{
  if ( name == "adds" )
    return std::unique_ptr<raid_event_t>( new adds_event_t( sim, options_str ) );
  if ( name == "pull" )
    return std::unique_ptr<raid_event_t>( new pull_event_t( sim, options_str ) );
  if ( name == "move_enemy" )
    return std::unique_ptr<raid_event_t>( new move_enemy_t( sim, options_str ) );
  if ( name == "casting" )
    return std::unique_ptr<raid_event_t>( new casting_event_t( sim, options_str ) );
  if ( name == "distraction" )
    return std::unique_ptr<raid_event_t>( new distraction_event_t( sim, options_str ) );
  if ( name == "invul" )
    return std::unique_ptr<raid_event_t>( new invulnerable_event_t( sim, options_str ) );
  if ( name == "invulnerable" )
    return std::unique_ptr<raid_event_t>( new invulnerable_event_t( sim, options_str ) );
  if ( name == "interrupt" )
    return std::unique_ptr<raid_event_t>( new interrupt_event_t( sim, options_str ) );
  if ( name == "movement" )
    return std::unique_ptr<raid_event_t>( new movement_event_t( sim, options_str ) );
  if ( name == "moving" )
    return std::unique_ptr<raid_event_t>( new movement_event_t( sim, options_str ) );
  if ( name == "damage" )
    return std::unique_ptr<raid_event_t>( new damage_event_t( sim, options_str ) );
  if ( name == "heal" )
    return std::unique_ptr<raid_event_t>( new heal_event_t( sim, options_str ) );
  if ( name == "stun" )
    return std::unique_ptr<raid_event_t>( new stun_event_t( sim, options_str ) );
  if ( name == "vulnerable" )
    return std::unique_ptr<raid_event_t>( new vulnerable_event_t( sim, options_str ) );
  if ( name == "position_switch" )
    return std::unique_ptr<raid_event_t>( new position_event_t( sim, options_str ) );
  if ( name == "flying" )
    return std::unique_ptr<raid_event_t>( new flying_event_t( sim, options_str ) );
  if ( name == "damage_taken_debuff" )
    return std::unique_ptr<raid_event_t>( new damage_taken_debuff_event_t( sim, options_str ) );
  if ( name == "damage_done_buff" )
    return std::unique_ptr<raid_event_t>( new damage_done_buff_event_t( sim, options_str ) );
  if ( name == "buff" )
    return std::unique_ptr<raid_event_t>( new buff_raid_event_t( sim, options_str ) );

  throw std::invalid_argument( fmt::format( "Invalid raid event type '{}'.", name ) );
}

// raid_event_t::init =======================================================

void raid_event_t::init( sim_t* sim )
{
  auto splits = util::string_split<util::string_view>( sim->raid_events_str, "/\\" );

  for ( const auto& split : splits )
  {
    auto name    = split;
    util::string_view options{};

    sim->print_debug( "Creating raid event '{}'.", name );

    util::string_view::size_type cut_pt = name.find_first_of( "," );

    if ( cut_pt != name.npos )
    {
      options = name.substr( cut_pt + 1 );
      name    = name.substr( 0, cut_pt );
    }
    try
    {
      auto raid_event = create( sim, name, options );

      if ( raid_event->cooldown <= timespan_t::zero() )
      {
        throw std::invalid_argument( "Cooldown not set or negative." );
      }
      if ( raid_event->cooldown <= raid_event->cooldown_stddev )
      {
        throw std::invalid_argument( "Cooldown lower than cooldown standard deviation." );
      }
      if ( sim->fight_style != FIGHT_STYLE_DUNGEON_ROUTE && raid_event->type == "pull" )
      {
        throw std::invalid_argument( "DungeonRoute fight style is required for pull events." );
      }
      if ( sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE && raid_event->type == "adds" )
      {
        throw std::invalid_argument( "DungeonRoute fight style is only compatible with pull events." );
      }

      sim->print_debug( "Successfully created '{}'.", *( raid_event.get() ) );
      sim->raid_events.push_back( std::move( raid_event ) );
    }
    catch ( const std::exception& )
    {
      std::throw_with_nested( std::invalid_argument( fmt::format( "Error creating raid event from '{}'", split ) ) );
    }
  }

  if ( sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE )
  {
    for ( const auto& raid_event : sim->raid_events )
    {
      if ( raid_event->type == "pull" )
        return;
    }
    throw std::invalid_argument( "DungeonRoute fight style requires at least one pull event." );
  }
}

void raid_event_t::reset( sim_t* sim )
{
  for ( auto& raid_event : sim->raid_events )
  {
    raid_event->reset();
  }
}

void raid_event_t::combat_begin( sim_t* sim )
{
  for ( auto& raid_event : sim->raid_events )
  {
    raid_event->combat_begin();
  }
}

/**
 * Filter players which shouldn't be added to the affected_players list.
 */
bool raid_event_t::filter_player( const player_t* p )
{
  if ( distance_min && distance_min > p->current.distance )
    return true;

  if ( distance_max && distance_max < p->current.distance )
    return true;

  if ( p->is_pet() && players_only )
    return true;

  if ( !sim->rng().roll( player_chance ) )
    return true;

  if ( affected_role != ROLE_NONE && p->role != affected_role )
    return true;

  return false;
}

double raid_event_t::evaluate_raid_event_expression( sim_t* s, util::string_view type_or_name, util::string_view filter,
                                                     bool test_filter, bool* is_constant )
{
  // correct for "damage" type event
  if ( util::str_compare_ci( type_or_name, "damage" ) )
    type_or_name = "raid_damage_";

  // filter the list for raid events that match the type requested
  std::vector<raid_event_t*> matching_events;

  // special handling for "pull" events since they're not time based events
  // "adds" and "pull" event types are mutually exclusive within a sim
  // handled via the same apl expression syntax as adds
  if ( util::str_compare_ci( type_or_name, "adds" ) && s->fight_style == FIGHT_STYLE_DUNGEON_ROUTE )
  {
    pull_event_t* current_pull = nullptr;
    for ( const auto& raid_event : s->raid_events )
    {
      if ( util::str_prefix_ci( raid_event->type, "pull" ) )
      {
        pull_event_t* pull_event = dynamic_cast<pull_event_t*>( raid_event.get() );
        matching_events.push_back( pull_event );

        if ( !current_pull && pull_event->first == 0_s )
          current_pull = pull_event;

        if ( pull_event->active() )
          current_pull = pull_event;
      }
    }

    if ( matching_events.empty() )
    {
      assert( is_constant );
      *is_constant = true;
      if ( filter == "in" || filter == "cooldown" )
        return timespan_t::max().total_seconds();
      else if ( !test_filter )  // When evaluating filter expr validity, let this one continue through
      {
        return 0.0;
      }
    }
    else if ( filter == "exists" || filter == "up" )
    {
      assert( is_constant );
      *is_constant = true;
      return 1.0;
    }

    if ( filter == "remains" )
    {
      if ( current_pull && current_pull->spawned && current_pull->active() )
        return current_pull->time_to_die();
      else
        return 0.0;
    }

    // the following expressions should refer to the next pull
    if ( !current_pull )
      return 0.0;

    pull_event_t* next_pull = current_pull->next_pull();

    if ( filter == "in" )
    {
      if ( !next_pull )
        return timespan_t::max().total_seconds();
      else
      {
        if ( current_pull->spawned )
          return current_pull->time_to_die() + next_pull->delay.total_seconds();
        else
          return s->max_time.total_seconds() / matching_events.size();
      }
    }

    if ( filter == "count" )
    {
      if ( next_pull )
        return as<double>( next_pull->spawn_parameters.size() );
      else
        return 0.0;
    }

    if ( filter == "duration" )
      return s->max_time.total_seconds() / matching_events.size();

    if ( filter == "distance" || filter == "max_distance" || filter == "min_distance" )
    {
      return 0.0;
    }

    if ( filter == "cooldown" )
      return timespan_t::max().total_seconds();

    throw std::invalid_argument( fmt::format( "Invalid filter expression '{}' for pull raid event.", filter ) );
  }

  // Add all raid event which match type or name
  for ( const auto& raid_event : s->raid_events )
  {
    if ( util::str_prefix_ci( raid_event->type, type_or_name ) )
      matching_events.push_back( raid_event.get() );

    if ( util::str_prefix_ci( raid_event->name, type_or_name ) )
      matching_events.push_back( raid_event.get() );
  }

  if ( matching_events.empty() )
  {
    assert(is_constant);
    *is_constant = true;
    if ( filter == "in" || filter == "cooldown" )
      return timespan_t::max().total_seconds();  // ridiculously large number
    else if ( !test_filter )                     // When evaluating filter expr validity, let this one continue through
    {
      return 0.0;
    }
    // return constant based on filter
  }
  else if ( filter == "exists" )
  {
    assert(is_constant);
    *is_constant = true;
    return 1.0;
  }

  if ( filter == "remains" )
  {
    auto events = get_longest_active_raid_events( matching_events );
    if ( events.empty() )
    {
      return 0.0;
    }
    return events.front()->remains().total_seconds();
  }

  if ( filter == "up" )
  {
    for ( const auto& event : matching_events )
    {
      if ( event->up() )
      {
        return 1.0;
      }
    }
    return 0.0;
  }

  // For all remaining expression, go through the list of matching raid events and look for the one happening first
  raid_event_t* e = get_next_raid_event( matching_events );
  // timespan_t time_to_event = timespan_t::from_seconds( -1 );
  if ( e == nullptr )
    return 0.0;

  // now that we have the event in question, use the filter to figure out return
  if ( filter == "in" )
  {
    if ( e->until_next() > timespan_t::zero() )
    {
      return e->until_next().total_seconds();
    }
    else
    {
      return timespan_t::max().total_seconds();
    }
  }

  if ( filter == "duration" )
    return e->duration_time().total_seconds();

  if ( filter == "cooldown" )
    return e->cooldown_time().total_seconds();

  if ( filter == "distance" )
    return e->distance();

  if ( filter == "max_distance" )
    return e->max_distance();

  if ( filter == "min_distance" )
    return e->min_distance();

  if ( filter == "amount" )
  {
    if ( auto result_amount_typevent = dynamic_cast<damage_event_t*>( e ) )
    {
      return result_amount_typevent->amount;
    }
    else
    {
      throw std::invalid_argument(
          fmt::format( "Invalid filter expression '{}' for non-damage raid event '{}'.", filter, type_or_name ) );
    }
  }

  if ( filter == "to_pct" )
  {
    if ( auto heal_event = dynamic_cast<heal_event_t*>( e ) )
    {
      return heal_event->to_pct;
    }
    else
    {
      throw std::invalid_argument(
          fmt::format( "Invalid filter expression '{}' for non-heal raid event '{}'.", filter, type_or_name ) );
    }
  }

  if ( filter == "count" )
  {
    if ( auto adds_event = dynamic_cast<adds_event_t*>( e ) )
    {
      return adds_event->count;
    }
    else
    {
      throw std::invalid_argument(
          fmt::format( "Invalid filter expression '{}' for non-adds raid event '{}'.", filter, type_or_name ) );
    }
  }

  // if we have no idea what filter they've specified, return 0
  // todo: should this generate an error or something instead?

  throw std::invalid_argument( fmt::format( "Unknown filter expression '{}'.", filter ) );
  ;
}

void sc_format_to( const raid_event_t& raid_event, fmt::format_context::iterator out )
{
  fmt::format_to( out, "Raid event (type={} name={})", raid_event.type, raid_event.name );
}
