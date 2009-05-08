// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Target
// ==========================================================================

// target_t::target_t =======================================================

target_t::target_t( sim_t* s ) :
  sim(s), name_str("Fluffy Pillow"), race(RACE_HUMANOID), level(83), 
  initial_armor(-1), armor(0), block_value(0), shield(0), 
  initial_health( 0 ), current_health(0), total_dmg(0), uptime_list(0)
{
  for( int i=0; i < SCHOOL_MAX; i++ ) spell_resistance[ i ] = 0;
}

// target_t::~target_t ======================================================

target_t::~target_t()
{
  while( uptime_t* u = uptime_list )
  {
    uptime_list = u -> next;
    delete u;
  }
}

// target_t::id =============================================================

const char* target_t::id()
{
  if( id_str.empty() )
  {
    id_str = "0xF1300079AA001884,\"Heroic Training Dummy\",0x10a28"; 
  }

  return id_str.c_str();
}

// target_t::assess_damage ==================================================

void target_t::assess_damage( double amount, 
                              int    school, 
                              int    dmg_type )
{
  total_dmg += amount;

  if( initial_health > 0 )
  {
    current_health -= amount;

    if( current_health <= 0 ) 
    {
      if( sim -> log ) log_t::output( sim, "%s has died.", name() );
    }
    else if( sim -> debug ) log_t::output( sim, "Target %s has %.0f remaining health", name(), current_health );
  }

  // FIXME! Someday create true "callbacks" for the various events to clean up crap like this.
  
  if( ( school == SCHOOL_PHYSICAL ) && ( dmg_type == DMG_DIRECT ) && ( debuffs.hemorrhage_charges > 0 ) )
  {
    debuffs.hemorrhage_charges--;
    if( debuffs.hemorrhage_charges == 0 ) event_t::early( expirations.hemorrhage );
  }
}
   
// target_t::recalculate_health ==============================================

void target_t::recalculate_health()
{
  if( sim -> max_time == 0 ) return;

  if( initial_health == 0 )
  {
    current_health = total_dmg;
    initial_health = current_health * ( sim -> max_time / sim -> current_time );
  }
  else
  {
    double delta_time = sim -> max_time - sim -> current_time;
    double adjust = initial_health * delta_time / sim -> max_time;
    adjust /= sim -> current_iteration + 1; // dampening factor
    initial_health += adjust;
  }

  if( sim -> debug ) log_t::output( sim, "Target initial health calculated to be %.0f", initial_health );     
}

// target_t::time_to_die =====================================================

double target_t::time_to_die()
{
  if( initial_health > 0 )
  {
    return sim -> current_time * current_health / total_dmg;
  }
  else
  {
    return sim -> max_time - sim -> current_time;
  }
}

// target_t::health_percentage ===============================================

double target_t::health_percentage()
{
  if( initial_health <= 0 ) return 100;

  return 100.0 * current_health / initial_health;
}

// target_t::base_armor ======================================================

double target_t::base_armor()
{
  return armor;
}

// target_t::get_uptime =====================================================

uptime_t* target_t::get_uptime( const std::string& name )
{
  uptime_t* u=0;

  for( u = uptime_list; u; u = u -> next )
  {
    if( u -> name_str == name )
      return u;
  }

  u = new uptime_t( name );

  uptime_t** tail = &uptime_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// target_t::init ============================================================

void target_t::init()
{
  if( initial_armor < 0 ) initial_armor = sim -> P309 ? 13000 : 10643;

  if( ! race_str.empty() )
  {
    for( race = RACE_NONE; race < RACE_MAX; race++ )
      if( race_str == util_t::race_type_string( race ) )
        break;

    if( race == RACE_MAX )
    {
      printf( "simcraft: '%s' is not a valid value for 'target_race'\n", race_str.c_str() );
      exit(0);
    }
  }

  uptimes.blood_frenzy         = get_uptime( "blood_frenzy"         );
  uptimes.improved_scorch      = get_uptime( "improved_scorch"      );
  uptimes.improved_shadow_bolt = get_uptime( "improved_shadow_bolt" );
  uptimes.mangle               = get_uptime( "mangle"               );
  uptimes.master_poisoner      = get_uptime( "master_poisoner"      );
  uptimes.savage_combat        = get_uptime( "savage_combat"        );
  uptimes.trauma               = get_uptime( "trauma"               );
  uptimes.totem_of_wrath       = get_uptime( "totem_of_wrath"       );
  uptimes.winters_chill        = get_uptime( "winters_chill"        );
  uptimes.winters_grasp        = get_uptime( "winters_grasp"        );
}

// target_t::reset ===========================================================

void target_t::reset()
{
  if( sim -> debug ) log_t::output( sim, "Reseting target %s", name() );
  total_dmg = 0;
  armor = initial_armor;
  current_health = initial_health;
  debuffs.reset();
  expirations.reset();
  cooldowns.reset();
}

// target_t::combat_begin ====================================================

void target_t::combat_begin()
{
  if( sim -> overrides.bleeding              ) debuffs.bleeding = 1;
  if( sim -> overrides.blood_frenzy          ) debuffs.blood_frenzy = 1;
  if( sim -> overrides.crypt_fever           ) debuffs.crypt_fever = 1;
  if( sim -> overrides.curse_of_elements     ) debuffs.curse_of_elements = 13;
  if( sim -> overrides.earth_and_moon        ) debuffs.earth_and_moon = 13;
  if( sim -> overrides.faerie_fire           ) debuffs.faerie_fire = sim -> P309 ? 1260 : 0.05;
  if( sim -> overrides.hunters_mark          ) debuffs.hunters_mark = 450;
  if( sim -> overrides.improved_scorch       ) debuffs.improved_scorch = 5;
  if( sim -> overrides.improved_shadow_bolt  ) debuffs.improved_shadow_bolt = 5;
  if( sim -> overrides.judgement_of_wisdom   ) debuffs.judgement_of_wisdom = 1;
  if( sim -> overrides.mangle                ) debuffs.mangle = 1;
  if( sim -> overrides.master_poisoner       ) debuffs.master_poisoner = 1;
  if( sim -> overrides.misery                ) { debuffs.misery = 3; debuffs.misery_stack = 1; }
  if( sim -> overrides.poisoned              ) debuffs.poisoned = 1;
  if( sim -> overrides.razorice              ) debuffs.razorice = 1;
  if( sim -> overrides.savage_combat         ) debuffs.savage_combat = 1;
  if( sim -> overrides.snare                 ) debuffs.snare = 1;
  if( sim -> overrides.sunder_armor          ) debuffs.sunder_armor = sim -> P309 ? 3925 : 0.20;
  if( sim -> overrides.thunder_clap          ) debuffs.thunder_clap = 1;
  if( sim -> overrides.totem_of_wrath        ) debuffs.totem_of_wrath = 1;
  if( sim -> overrides.trauma                ) debuffs.trauma = 1;
  if( sim -> overrides.winters_chill         ) debuffs.winters_chill = 5;

  if( sim -> overrides.bloodlust )
  {
    // Setup a periodic check for Bloodlust

    struct bloodlust_proc_t : public event_t
    {
      bloodlust_proc_t( sim_t* sim ) : event_t( sim, 0 )
      {
        name = "Bloodlust Proc";
        for( player_t* p = sim -> player_list; p; p = p -> next )
        {
          if( p -> sleeping ) continue;
          if( sim -> cooldown_ready( p -> cooldowns.bloodlust ) )
          {
            p -> aura_gain( "Bloodlust" );
            p -> buffs.bloodlust = 1;
            p -> cooldowns.bloodlust = sim -> current_time + (sim -> P309 ? 300 : 600);
          }
        }
        sim -> add_event( this, 40.0 );
      }
      virtual void execute()
      {
        for( player_t* p = sim -> player_list; p; p = p -> next )
        {
          if( p -> buffs.bloodlust > 0 )
          {
            p -> aura_loss( "Bloodlust" );
            p -> buffs.bloodlust = 0;
          }
        }
      }
    };
    struct bloodlust_check_t : public event_t
    {
      bloodlust_check_t( sim_t* sim ) : event_t( sim, 0 )
      {
        name = "Bloodlust Check";
        sim -> add_event( this, 1.0 );
      }
      virtual void execute()
      {
        target_t* t = sim -> target;
        if( ( sim -> overrides.bloodlust_early && ( sim -> current_time > sim -> overrides.bloodlust_early ) ) ||
            ( t -> health_percentage() < 25 ) || 
            ( t -> time_to_die()       < 60 ) )
        {
          new ( sim ) bloodlust_proc_t( sim );
        }
        else
        {
          new ( sim ) bloodlust_check_t( sim );
        }
      }
    };

    new ( sim ) bloodlust_check_t( sim );
  }
}

// target_t::combat_end ======================================================

void target_t::combat_end()
{
}

// target_t::parse_option ======================================================

bool target_t::parse_option( const std::string& name,
                             const std::string& value )
{
  option_t options[] =
  {
    // @option_doc loc=global/target/general title="Target General"
    { "target_name",           OPT_STRING, &( name_str                          ) },
    { "target_race",           OPT_STRING, &( race_str                          ) },
    { "target_level",          OPT_INT,    &( level                             ) },
    { "target_health",         OPT_FLT,    &( initial_health                    ) },
    { "target_id",             OPT_STRING, &( id_str                            ) },
    // @option_doc loc=global/target/defense title="Target Defense"
    { "target_resist_holy",    OPT_INT,    &( spell_resistance[ SCHOOL_HOLY   ] ) },
    { "target_resist_shadow",  OPT_INT,    &( spell_resistance[ SCHOOL_SHADOW ] ) },
    { "target_resist_arcane",  OPT_INT,    &( spell_resistance[ SCHOOL_ARCANE ] ) },
    { "target_resist_frost",   OPT_INT,    &( spell_resistance[ SCHOOL_FROST  ] ) },
    { "target_resist_fire",    OPT_INT,    &( spell_resistance[ SCHOOL_FIRE   ] ) },
    { "target_resist_nature",  OPT_INT,    &( spell_resistance[ SCHOOL_NATURE ] ) },
    { "target_armor",          OPT_INT,    &( initial_armor                     ) },
    { "target_shield",         OPT_INT,    &( shield                            ) },
    { "target_block",          OPT_INT,    &( block_value                       ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( sim -> output_file, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}

