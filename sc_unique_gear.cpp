// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// stat_proc_callback =======================================================

struct stat_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stat, stacks, max_stacks;
  double amount, proc_chance, duration;
  double cooldown, cooldown_ready;
  event_t* expiration;
  proc_t* proc;
  rng_t* rng;

  stat_proc_callback_t( const std::string& n, player_t* p, int s, int ms, double a, double pc, double d, double cd ) :
    action_callback_t( p -> sim, p ),
    name_str(n), stat(s), stacks(0), max_stacks(ms), amount(a), proc_chance(pc), duration(d), cooldown(cd), 
    cooldown_ready(0), expiration(0), proc(0), rng(0)
  {
    if( proc_chance ) 
    {
      proc = p -> get_proc( name_str.c_str() );
      rng  = p -> get_rng ( name_str.c_str() );
    }
  }

  virtual void reset() { stacks=0; cooldown_ready=0; expiration=0; }

  virtual void trigger( action_t* a )
  {
    if( cooldown )
      if( sim -> current_time < cooldown_ready )
        return;

    if( proc_chance )
    {
      if( ! rng -> roll( proc_chance ) )
        return;

      proc -> occur();
    }

    if( cooldown ) 
      cooldown_ready = sim -> current_time + cooldown;

    if( stacks < max_stacks )
    {
      stacks++;
      listener -> aura_gain( name_str.c_str() );
      listener -> stat_gain( stat, amount );
    }

    if( duration )
    {
      if( expiration )
      {
        expiration -> reschedule( duration );
      }
      else
      {
        struct expiration_t : public event_t
        {
          stat_proc_callback_t* callback;
          
          expiration_t( sim_t* sim, player_t* player, stat_proc_callback_t* cb ) : event_t( sim, player ), callback( cb )
          {
            name = callback -> name_str.c_str();
            sim -> add_event( this, callback -> duration );
          }
          virtual void execute()
          {
            player -> aura_loss( callback -> name_str.c_str() );
            player -> stat_loss( callback -> stat, callback -> amount * callback -> stacks );
            callback -> expiration = 0;
            callback -> stacks = 0;
          }
        };

        expiration = new ( sim ) expiration_t( sim, listener, this );
      }
    }
  }
};

// discharge_proc_callback ==================================================

struct discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stacks, max_stacks;
  double proc_chance;
  double cooldown, cooldown_ready;
  spell_t* spell;
  proc_t* proc;
  rng_t* rng;

  discharge_proc_callback_t( const std::string& n, player_t* p, int ms, int school, double min, double max, double pc, double cd ) :
    action_callback_t( p -> sim, p ),
    name_str(n), stacks(0), max_stacks(ms), proc_chance(pc), cooldown(cd), cooldown_ready(0)
  {
    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double min, double max, int s ) : 
        spell_t( n, p, RESOURCE_NONE, s )
      {
        trigger_gcd = 0;
        base_dd_min = min;
        base_dd_max = max;
        may_crit    = true;
        background  = true;
        base_spell_power_multiplier = 0;
        reset();
      }
    };

    spell = new discharge_spell_t( name_str.c_str(), p, min, max, school );

    proc = p -> get_proc( name_str.c_str() );
    rng  = p -> get_rng ( name_str.c_str() );
  }

  virtual void reset() { stacks=0; cooldown_ready=0; }

  virtual void trigger( action_t* a )
  {
    if( cooldown )
      if( sim -> current_time < cooldown_ready )
        return;

    if( proc_chance )
      if( ! rng -> roll( proc_chance ) )
        return;

    if( ++stacks < max_stacks )
    {
      listener -> aura_gain( name_str.c_str() );
    }
    else
    {
      stacks = 0;
      spell -> execute();
      proc -> occur();
      
      if( cooldown ) 
        cooldown_ready = sim -> current_time + cooldown;
    }
  }
};

// ==========================================================================
// Attack Power Trinket Action
// ==========================================================================

struct attack_power_trinket_t : public action_t
{
  double attack_power, length;
  
  attack_power_trinket_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "attack_power_trinket", p ), attack_power(0), length(0)
  {
    option_t options[] =
    {
      { "name",     OPT_STRING, &name_str     },
      { "power",    OPT_FLT,    &attack_power },
      { "length",   OPT_FLT,    &length       },
      { "cooldown", OPT_FLT,    &cooldown     },
      { "sync",     OPT_STRING, &sync_str     },
      { NULL }
    };
    parse_options( options, options_str );

    if( attack_power <= 0 ||
        length       <= 0 ||
        cooldown     <= 0 )
    {
      fprintf( sim -> output_file, "Expected format: attack_power_trinket,power=X,length=Y,cooldown=Z\n" );
      assert(0);
    }
    trigger_gcd = 0;
    harmful = false;
  }
   
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      attack_power_trinket_t* trinket;

      expiration_t( sim_t* sim, attack_power_trinket_t* t ) : event_t( sim, t -> player ), trinket( t )
      {
        name = "Attack Power Trinket Expiration";
        player -> aura_gain( "Attack Power Trinket" );
        player -> attack_power += trinket -> attack_power;
        sim -> add_event( this, trinket -> length );
      }
      virtual void execute()
      {
        player -> aura_loss( "Attack Power Trinket" );
        player -> attack_power -= trinket -> attack_power;
      }
    };
  
    if( sim -> log ) log_t::output( sim, "Player %s uses %s Attack Power Trinket", player -> name(), name() );
    cooldown_ready = player -> sim -> current_time + cooldown;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, length );
    new ( sim ) expiration_t( sim, this );
  }
};

// ==========================================================================
// Spell Power Trinket Action
// ==========================================================================

struct spell_power_trinket_t : public action_t
{
  double spell_power, length;
  
  spell_power_trinket_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "spell_power_trinket", p ), spell_power(0), length(0)
  {
    option_t options[] =
    {
      { "name",     OPT_STRING, &name_str    },
      { "power",    OPT_FLT,    &spell_power },
      { "length",   OPT_FLT,    &length      },
      { "cooldown", OPT_FLT,    &cooldown    },
      { "sync",     OPT_STRING, &sync_str    },
      { NULL }
    };
    parse_options( options, options_str );

    if( spell_power <= 0 ||
        length      <= 0 ||
        cooldown    <= 0 )
    {
      fprintf( sim -> output_file, "Expected format: spell_power_trinket,power=X,length=Y,cooldown=Z\n" );
      exit(0);
    }
    trigger_gcd = 0;
    harmful = false;
  }
   
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      spell_power_trinket_t* trinket;

      expiration_t( sim_t* sim, spell_power_trinket_t* t ) : event_t( sim, t -> player ), trinket( t )
      {
        name = "Spell Power Trinket Expiration";
        player -> aura_gain( "Spell Power Trinket" );
        player -> spell_power[ SCHOOL_MAX ] += trinket -> spell_power;
        sim -> add_event( this, trinket -> length );
      }
      virtual void execute()
      {
        player -> aura_loss( "Spell Power Trinket" );
        player -> spell_power[ SCHOOL_MAX ] -= trinket -> spell_power;
      }
    };
  
    if( sim -> log ) log_t::output( sim, "Player %s uses %s Spell Power Trinket", player -> name(), name() );
    cooldown_ready = player -> sim -> current_time + cooldown;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, length );
    new ( sim ) expiration_t( sim, this );
  }
};

// ==========================================================================
// Haste Trinket Action
// ==========================================================================

struct haste_trinket_t : public action_t
{
  int    haste_rating;
  double length;
  
  haste_trinket_t( player_t* p, const std::string& options_str ) : 
    action_t( ACTION_USE, "haste_trinket", p ), haste_rating(0), length(0)
  {
    option_t options[] =
    {
      { "name",     OPT_STRING, &name_str     },
      { "rating",   OPT_INT,    &haste_rating },
      { "length",   OPT_FLT,    &length       },
      { "cooldown", OPT_FLT,    &cooldown     },
      { "sync",     OPT_STRING, &sync_str     },
      { NULL }
    };
    parse_options( options, options_str );

    if( haste_rating <= 0 ||
        length       <= 0 ||
        cooldown     <= 0 )
    {
      fprintf( sim -> output_file, "Expected format: haste_trinket,rating=X,length=Y,cooldown=Z\n" );
      assert(0);
    }
    trigger_gcd = 0;
    harmful = false;
  }
   
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      haste_trinket_t* trinket;

      expiration_t( sim_t* sim, haste_trinket_t* t ) : event_t( sim, t -> player ), trinket( t )
      {
        name = "Haste Trinket Expiration";
        player -> aura_gain( "Haste Trinket" );
        player -> haste_rating += trinket -> haste_rating;
        player -> recalculate_haste();
        sim -> add_event( this, trinket -> length );
      }
      virtual void execute()
      {
        player -> aura_loss( "Haste Trinket" );
        player -> haste_rating -= trinket -> haste_rating;
        player -> recalculate_haste();
      }
    };
  
    if( sim -> log ) log_t::output( sim, "Player %s uses %s Haste Trinket", player -> name(), name() );
    cooldown_ready = player -> sim -> current_time + cooldown;
    // Trinket use may not overlap.....
    player -> share_cooldown( cooldown_group, length );
    new ( sim ) expiration_t( sim, this );
  }
};

// ==========================================================================
// Hand-Mounted Pyro Rocket
// ==========================================================================


struct hand_mounted_pyro_rocket_t : public spell_t
{
  hand_mounted_pyro_rocket_t( player_t* p, const std::string& options_str ) : 
    spell_t( "hand_mounted_pyro_rocket", p, RESOURCE_NONE, SCHOOL_FIRE )
    // FIX ME! Does this use attack or spell mechanics?
  {
    cooldown    = 45;
    trigger_gcd = 0;
    base_dd_min = 1440;
    base_dd_max = 1760;
    may_crit    = true;
    // FIX ME!
    // Hand-Mounted Pyro Rocket: No longer on the global cooldown. Damage 
    // increased, Cooldown reduced. Now invokes a 10-second DPS burst item 
    // category cooldown.
    // Which items share the cooldown the 10s cd?
  }
};

// ==========================================================================
// unique_gear_t::parse_option
// ==========================================================================

bool unique_gear_t::parse_option( player_t*          p,
                                  const std::string& name, 
                                  const std::string& value )
{
  option_t options[] =
  {
    // @option_doc loc=player/tier title="Tier Bonuses"
    { "ashtongue_talisman",                   OPT_BOOL, &( p -> unique_gear -> ashtongue_talisman               ) },
    { "tier4_2pc",                            OPT_BOOL, &( p -> unique_gear -> tier4_2pc                        ) },
    { "tier4_4pc",                            OPT_BOOL, &( p -> unique_gear -> tier4_4pc                        ) },
    { "tier5_2pc",                            OPT_BOOL, &( p -> unique_gear -> tier5_2pc                        ) },
    { "tier5_4pc",                            OPT_BOOL, &( p -> unique_gear -> tier5_4pc                        ) },
    { "tier6_2pc",                            OPT_BOOL, &( p -> unique_gear -> tier6_2pc                        ) },
    { "tier6_4pc",                            OPT_BOOL, &( p -> unique_gear -> tier6_4pc                        ) },
    { "tier7_2pc",                            OPT_BOOL, &( p -> unique_gear -> tier7_2pc                        ) },
    { "tier7_4pc",                            OPT_BOOL, &( p -> unique_gear -> tier7_4pc                        ) },
    { "tier8_2pc",                            OPT_BOOL, &( p -> unique_gear -> tier8_2pc                        ) },
    { "tier8_4pc",                            OPT_BOOL, &( p -> unique_gear -> tier8_4pc                        ) },
    // @option_doc loc=player/procs title="Unique Gear/Gem Effects"
    { "bandits_insignia",                     OPT_BOOL,  &( p -> unique_gear -> bandits_insignia                ) },
    { "chaotic_skyfire",                      OPT_BOOL,  &( p -> unique_gear -> chaotic_skyflare                ) },
    { "chaotic_skyflare",                     OPT_BOOL,  &( p -> unique_gear -> chaotic_skyflare                ) },
    { "darkmoon_crusade",                     OPT_BOOL,  &( p -> unique_gear -> darkmoon_crusade                ) },
    { "darkmoon_greatness",                   OPT_BOOL,  &( p -> unique_gear -> darkmoon_greatness              ) },
    { "dying_curse",                          OPT_BOOL,  &( p -> unique_gear -> dying_curse                     ) },
    { "egg_of_mortal_essence",                OPT_BOOL,  &( p -> unique_gear -> egg_of_mortal_essence           ) },
    { "elder_scribes",                        OPT_BOOL,  &( p -> unique_gear -> elder_scribes                   ) },
    { "elemental_focus_stone",                OPT_BOOL,  &( p -> unique_gear -> elemental_focus_stone           ) },
    { "ember_skyflare",                       OPT_BOOL,  &( p -> unique_gear -> ember_skyflare                  ) },
    { "embrace_of_the_spider",                OPT_BOOL,  &( p -> unique_gear -> embrace_of_the_spider           ) },
    { "eternal_sage",                         OPT_BOOL,  &( p -> unique_gear -> eternal_sage                    ) },
    { "extract_of_necromatic_power",          OPT_BOOL,  &( p -> unique_gear -> extract_of_necromatic_power     ) },
    { "eye_of_magtheridon",                   OPT_BOOL,  &( p -> unique_gear -> eye_of_magtheridon              ) },
    { "eye_of_the_broodmother",               OPT_BOOL,  &( p -> unique_gear -> eye_of_the_broodmother          ) },
    { "flare_of_the_heavens",                 OPT_BOOL,  &( p -> unique_gear -> flare_of_the_heavens            ) },
    { "forge_ember",                          OPT_BOOL,  &( p -> unique_gear -> forge_ember                     ) },
    { "fury_of_the_five_flights",             OPT_BOOL,  &( p -> unique_gear -> fury_of_the_five_flights        ) },
    { "grim_toll",                            OPT_BOOL,  &( p -> unique_gear -> grim_toll                       ) },
    { "illustration_of_the_dragon_soul",      OPT_BOOL,  &( p -> unique_gear -> illustration_of_the_dragon_soul ) },
    { "lightning_capacitor",                  OPT_BOOL,  &( p -> unique_gear -> lightning_capacitor             ) },
    { "mirror_of_truth",                      OPT_BOOL,  &( p -> unique_gear -> mirror_of_truth                 ) },
    { "mark_of_defiance",                     OPT_BOOL,  &( p -> unique_gear -> mark_of_defiance                ) },
    { "mystical_skyfire",                     OPT_BOOL,  &( p -> unique_gear -> mystical_skyfire                ) },
    { "pyrite_infuser",                       OPT_BOOL,  &( p -> unique_gear -> pyrite_infuser                  ) },
    { "quagmirrans_eye",                      OPT_BOOL,  &( p -> unique_gear -> quagmirrans_eye                 ) },
    { "relentless_earthstorm",                OPT_BOOL,  &( p -> unique_gear -> relentless_earthstorm           ) },
    { "sextant_of_unstable_currents",         OPT_BOOL,  &( p -> unique_gear -> sextant_of_unstable_currents    ) },
    { "shiffars_nexus_horn",                  OPT_BOOL,  &( p -> unique_gear -> shiffars_nexus_horn             ) },
    { "spellstrike",                          OPT_BOOL,  &( p -> unique_gear -> spellstrike                     ) },
    { "sundial_of_the_exiled",                OPT_BOOL,  &( p -> unique_gear -> sundial_of_the_exiled           ) },
    { "thunder_capacitor",                    OPT_BOOL,  &( p -> unique_gear -> thunder_capacitor               ) },
    { "timbals_crystal",                      OPT_BOOL,  &( p -> unique_gear -> timbals_crystal                 ) },
    { "wrath_of_cenarius",                    OPT_BOOL,  &( p -> unique_gear -> wrath_of_cenarius               ) },
    { NULL, OPT_UNKNOWN }
  };
  
  if( name.empty() )
  {
    option_t::print( p -> sim -> output_file, options );
    return false;
  }

  return option_t::parse( p -> sim, options, name, value );
}

// ==========================================================================
// unique_gear_t::create_action
// ==========================================================================

action_t* unique_gear_t::create_action( player_t*          p,
                                        const std::string& name, 
                                        const std::string& options_str )
{
  if( name == "attack_power_trinket"     ) return new attack_power_trinket_t    ( p, options_str );
  if( name == "haste_trinket"            ) return new haste_trinket_t           ( p, options_str );
  if( name == "spell_power_trinket"      ) return new spell_power_trinket_t     ( p, options_str );
  if( name == "hand_mounted_pyro_rocket" ) return new hand_mounted_pyro_rocket_t( p, options_str );

  return 0;
}

// ==========================================================================
// unique_gear_t::init
// ==========================================================================

void unique_gear_t::init( player_t* p )
{
  if( p -> unique_gear -> ember_skyflare )
  {
    p -> attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.02;    
  }
}

// ==========================================================================
// unique_gear_t::register_callbacks
// ==========================================================================

void unique_gear_t::register_callbacks( player_t* p )
{
  action_callback_t* cb;

  // Stat Procs

  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> darkmoon_crusade ) 
  {
    cb = new stat_proc_callback_t( "darkmoon_crusade", p, STAT_SPELL_POWER, 10, 8, 0.0, 10.0, 0.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> darkmoon_greatness )
  {
    int attr[] = { ATTR_STRENGTH, ATTR_AGILITY, ATTR_INTELLECT, ATTR_SPIRIT };
    int stat[] = { STAT_STRENGTH, STAT_AGILITY, STAT_INTELLECT, STAT_SPIRIT };

    int max_stat=-1;
    double max_value=0;

    for( int i=0; i < 4; i++ ) 
    {
      if( p -> attribute[ attr[ i ] ] > max_value )
      {
        max_value = p -> attribute[ attr[ i ] ];
        max_stat = stat[ i ];
      }
    }
    cb = new stat_proc_callback_t( "darkmoon_greatness", p, max_stat, 1, 300, 0.35, 15.0, 45.0 );

    p -> register_tick_damage_callback( cb );
    p -> register_direct_damage_callback( cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> dying_curse )
  {
    cb = new stat_proc_callback_t( "dying_curse", p, STAT_SPELL_POWER, 1, 765, 0.15, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> egg_of_mortal_essence )
  {
    cb = new stat_proc_callback_t( "egg_of_mortal_essence", p, STAT_HASTE_RATING, 1, 505, 0.10, 10.0, 45.0 );
    p -> register_resource_gain_callback( RESOURCE_HEALTH, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> elder_scribes )
  {
    cb = new stat_proc_callback_t( "elder_scribes", p, STAT_SPELL_POWER, 1, 130, 0.05, 10.0, 60.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> elemental_focus_stone )
  {
    cb = new stat_proc_callback_t( "elemental_focus_stone", p, STAT_HASTE_RATING, 1, 522, 0.10, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> embrace_of_the_spider )
  {
    cb = new stat_proc_callback_t( "embrace_of_the_spider", p, STAT_HASTE_RATING, 1, 505, 0.10, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> eternal_sage )
  {
    cb = new stat_proc_callback_t( "eternal_sage", p, STAT_SPELL_POWER, 1, 95, 0.10, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> eye_of_magtheridon )
  {
    cb = new stat_proc_callback_t( "eye_of_magtheridon", p, STAT_SPELL_POWER, 1, 170, 1.00, 10.0, 0.0 );
    p -> register_spell_result_callback( RESULT_MISS_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> eye_of_the_broodmother ) 
  {
    cb = new stat_proc_callback_t( "eye_of_the_broodmother", p, STAT_SPELL_POWER, 5, 25, 0.0, 10.0, 0.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> flare_of_the_heavens )
  {
    cb = new stat_proc_callback_t( "flare_of_the_heavens", p, STAT_SPELL_POWER, 1, 850, 0.10, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> forge_ember )
  {
    cb = new stat_proc_callback_t( "forge_ember", p, STAT_SPELL_POWER, 1, 512, 0.10, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> fury_of_the_five_flights ) 
  {
    cb = new stat_proc_callback_t( "fury_of_the_five_flights", p, STAT_ATTACK_POWER, 20, 16, 0.0, 10.0, 0.0 );
    p -> register_attack_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> grim_toll )
  {
    cb = new stat_proc_callback_t( "grim_toll", p, STAT_ARMOR_PENETRATION_RATING, 1, 612, 0.15, 10.0, 45.0 );
    p -> register_attack_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> illustration_of_the_dragon_soul ) 
  {
    cb = new stat_proc_callback_t( "illustration_of_the_dragon_soul", p, STAT_SPELL_POWER, 10, 20, 0.0, 10.0, 0.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> mark_of_defiance )
  {
    cb = new stat_proc_callback_t( "mark_of_defiance", p, STAT_MANA, 1, 150, 0.15, 0.0, 15.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> mirror_of_truth )
  {
    cb = new stat_proc_callback_t( "mirror_of_truth", p, STAT_ATTACK_POWER, 1, 1000, 0.10, 10.0, 50.0 );
    p -> register_attack_result_callback( RESULT_CRIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> mystical_skyfire )
  {
    cb = new stat_proc_callback_t( "mystical_skyfire", p, STAT_HASTE_RATING, 1, 320, 0.15, 4.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> pyrite_infuser )
  {
    cb = new stat_proc_callback_t( "pyrite_infuser", p, STAT_ATTACK_POWER, 1, 1234, 0.10, 10.0, 45.0 );
    p -> register_attack_result_callback( RESULT_CRIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> quagmirrans_eye )
  {
    cb = new stat_proc_callback_t( "quagmirrans_eye", p, STAT_HASTE_RATING, 1, 320, 0.10, 6.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> shiffars_nexus_horn )
  {
    cb = new stat_proc_callback_t( "shiffars_nexus_horn", p, STAT_SPELL_POWER, 1, 225, 0.20, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_CRIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> sextant_of_unstable_currents )
  {
    cb = new stat_proc_callback_t( "sextant_of_unstable_currents", p, STAT_SPELL_POWER, 1, 190, 0.20, 15.0, 45.0 );
    p -> register_spell_result_callback( RESULT_CRIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> spellstrike )
  {
    cb = new stat_proc_callback_t( "spellstrike", p, STAT_SPELL_POWER, 1, 92, 0.05, 10.0, 0.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> sundial_of_the_exiled )
  {
    cb = new stat_proc_callback_t( "sundial_of_the_exiled", p, STAT_SPELL_POWER, 1, 590, 0.10, 10.0, 45.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> wrath_of_cenarius )
  {
    cb = new stat_proc_callback_t( "wrath_of_cenarius", p, STAT_SPELL_POWER, 1, 132, 0.05, 10.0, 0.0 );
    p -> register_spell_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------

  // Discharge Procs

  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> bandits_insignia )
  {
    cb = new discharge_proc_callback_t( "bandits_insignia", p, 1, SCHOOL_ARCANE, 1504, 2256, 0.15, 45 );
    p -> register_attack_result_callback( RESULT_HIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> extract_of_necromatic_power )
  {
    cb = new discharge_proc_callback_t( "extract_of_necromatic_power", p, 1, SCHOOL_SHADOW, 1050, 1050, 0.10, 15 );
    p -> register_tick_callback( cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> lightning_capacitor )
  {
    cb = new discharge_proc_callback_t( "lightning_capacitor", p, 3, SCHOOL_NATURE, 750, 750, 0, 2.5 );
    p -> register_spell_result_callback( RESULT_CRIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> timbals_crystal )
  {
    cb = new discharge_proc_callback_t( "tmbals_focusing_crystal", p, 1, SCHOOL_SHADOW, 380, 380, 0.10, 15 );
    p -> register_tick_callback( cb );
  }
  //---------------------------------------------------------------------------------------------------------
  if( p -> unique_gear -> thunder_capacitor )
  {
    cb = new discharge_proc_callback_t( "thunder_capacitor", p, 3, SCHOOL_NATURE, 1276, 1276, 0, 2.5 );
    p -> register_spell_result_callback( RESULT_CRIT_MASK, cb );
  }
  //---------------------------------------------------------------------------------------------------------
}
