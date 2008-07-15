// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Druid
// ==========================================================================

struct shaman_t : public player_t
{
  // Totems
  spell_t* fire_totem;
  spell_t* air_totem;
  spell_t* water_totem;
  spell_t* earth_totem;

  // Active
  spell_t* active_flame_shock;

  // Buffs
  int8_t buffs_elemental_focus;
  int8_t buffs_elemental_mastery;
  int8_t buffs_natures_swiftness;

  struct talents_t
  {
    int8_t  call_of_flame;
    int8_t  call_of_thunder;
    int8_t  concussion;
    int8_t  convection;
    int8_t  elemental_focus;
    int8_t  elemental_fury;
    int8_t  elemental_mastery;
    int8_t  elemental_oath;
    int8_t  elemental_precision;
    int8_t  lava_flow;
    int8_t  lightning_mastery;
    int8_t  lightning_overload;
    int8_t  mana_tide_totem;
    int8_t  mental_quickness;
    int8_t  natures_blessing;
    int8_t  natures_guidance;
    int8_t  natures_swiftness;
    int8_t  restorative_totems;
    int8_t  reverberation;
    int8_t  storm_earth_and_fire;
    int8_t  tidal_mastery;
    int8_t  totemic_focus;
    int8_t  totem_of_wrath;
    int8_t  unrelenting_storm;
    int8_t  wrath_of_air;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  shaman_t( sim_t* sim, std::string& name ) : player_t( sim, SHAMAN, name )
  {
    // Totems
    fire_totem  = 0;
    air_totem   = 0;
    water_totem = 0;
    earth_totem = 0;;

    // Active
    active_flame_shock = 0;

    // Buffs
    buffs_elemental_focus = 0;
    buffs_elemental_mastery = 0;
    buffs_natures_swiftness = 0;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );

  // Event Tracking
  virtual void regen();
  virtual void spell_start_event ( spell_t* );
  virtual void spell_hit_event   ( spell_t* );
  virtual void spell_finish_event( spell_t* );
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

struct shaman_spell_t : public spell_t
{
  shaman_spell_t( const char* n, player_t* p, int8_t s, int8_t t ) : 
    spell_t( n, p, RESOURCE_MANA, s, t ) {}

  virtual double cost();
  virtual void   consume_resource();
  virtual double execute_time();
  virtual void   player_buff();

  // Passthru Methods
  virtual void execute()     { spell_t::execute();      }
  virtual void last_tick()   { spell_t::last_tick();    }
  virtual bool ready()       { return spell_t::ready(); }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_tier5_4pc ========================================================

static void trigger_tier5_4pc( spell_t* s )
{
  if( s -> result != RESULT_CRIT ) return;

  shaman_t* p = s -> player -> cast_shaman();

  if( p -> gear.tier5_4pc && wow_random( 0.25 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 120.0, "tier5_4pc" );
  }
}

// trigger_ashtongue_talisman ===============================================

static void trigger_ashtongue_talisman( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if( p -> gear.ashtongue_talisman && wow_random( 0.15 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 110.0, "ashtongue" );
  }
}

// trigger_lightning_overload ===============================================

static void trigger_lightning_overload( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if( p -> talents.lightning_overload == 0 ) return;

  if( wow_random( p -> talents.lightning_overload * 0.04 ) )
  {
    p -> proc( "lightning_overload" );

    double cost       = s -> base_cost;
    double multiplier = s -> base_multiplier;

    s -> base_cost        = 0;
    s -> base_multiplier /= 2.0;

    s -> time_to_execute = 0;
    s -> execute();

    s -> base_cost       = cost;
    s -> base_multiplier = multiplier;
  }
}

} // ANONYMOUS NAMESPACE ====================================================

// =========================================================================
// Shaman Spell
// =========================================================================

// shaman_spell_t::cost ====================================================

double shaman_spell_t::cost()
{
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_elemental_mastery ) return 0;
  double c = spell_t::cost();
  if( p -> buffs_elemental_focus ) c *= 0.60;
  if( p -> buffs.tier4_4pc )
  {
    c -= 270;
    if( c < 0 ) c = 0;
  }
  return c;
}

// shaman_spell_t::consume_resource ========================================

void shaman_spell_t::consume_resource()
{
  spell_t::consume_resource();
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_elemental_focus > 0 ) 
  {
    p -> buffs_elemental_focus--;
  }
  p -> buffs.tier4_4pc = 0;
  p -> buffs_elemental_mastery = 0;
}

// shaman_spell_t::execute_time ============================================

double shaman_spell_t::execute_time()
{
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_natures_swiftness ) return 0;
  return spell_t::execute_time();
}

// shaman_spell_t::player_buff =============================================

void shaman_spell_t::player_buff()
{
  spell_t::player_buff();
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_elemental_focus   ||
      p -> buffs_elemental_mastery )
  {
    player_hit += 0.5 * p -> talents.elemental_oath;
  }
  if( p -> buffs_elemental_mastery )
  {
    player_crit += 1.0;
  }
}

// =========================================================================
// Shaman Spells
// =========================================================================

// Chain Lightning Spell ===================================================

struct chain_lightning_t : public shaman_spell_t
{
  chain_lightning_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "chain_lightning", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8, 973, 1111, 0, 1695 },
      { 74, 7, 806,  920, 0, 1380 },
      { 70, 6, 734,  838, 0,  760 },
      { 63, 5, 603,  687, 0,  650 },
      { 56, 4, 493,  551, 0,  550 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );

    bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time  = 2.0; 
    dd_power_mod       = 0.7143;
    may_crit           = true;
    cooldown           = 6.0;
    base_cost          = rank -> cost;

    base_execute_time -= p -> talents.lightning_mastery * 0.1;
    base_cost         *= 1.0 - p -> talents.convection * ( AFTER_3_0_0 ? 0.04 : 0.02 );
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_crit         += p -> talents.call_of_thunder * 0.01;
    base_crit         += p -> talents.tidal_mastery * 0.01;
    base_hit          += p -> talents.natures_guidance * 0.01;
    base_hit          += p -> talents.elemental_precision * 0.02;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  lightning_bolt_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "lightning_bolt", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 67, 12, 563, 643, 0, 330 },
      { 62, 11, 495, 565, 0, 300 },
      { 56, 10, 419, 467, 0, 265 },
      { 50,  9, 347, 389, 0, 230 },
      { 44,  8, 282, 316, 0, 195 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );

    bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time  = 2.5; 
    dd_power_mod       = 0.794;
    may_crit           = true;
    base_cost          = rank -> cost;

    base_execute_time -= p -> talents.lightning_mastery * 0.1;
    base_cost         *= 1.0 - p -> talents.convection * ( AFTER_3_0_0 ? 0.04 : 0.02 );
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_crit         += p -> talents.call_of_thunder * 0.01;
    base_crit         += p -> talents.tidal_mastery * 0.01;
    base_hit          += p -> talents.natures_guidance * 0.01;
    base_hit          += p -> talents.elemental_precision * 0.02;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;

    if( p -> gear.tier6_4pc ) base_multiplier *= 1.05;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_ashtongue_talisman( this );
      trigger_lightning_overload( this );
      trigger_tier5_4pc( this );
    }
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  int8_t fswait;

  lava_burst_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "lava_burst", player, SCHOOL_FIRE, TREE_ELEMENTAL ), fswait(0)
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank",   OPT_INT8, &rank_index },
      { "fswait", OPT_INT8, &fswait     },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 74, 1, 888, 1132, 0, 655 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );

    bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time  = 2.0; 
    dd_power_mod       = ( 2.0 / 3.5 );
    may_crit           = true;
    cooldown           = 8.0;

    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.convection * ( AFTER_3_0_0 ? 0.04 : 0.02 );
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_multiplier   *= 1.0 + p -> talents.lava_flow * 0.033333;
    base_hit          += p -> talents.natures_guidance * 0.01;
    base_hit          += p -> talents.elemental_precision * 0.02;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    if( result_is_hit() )
    {
      shaman_t* p = player -> cast_shaman();
      if( p -> active_flame_shock )
      {
	p -> active_flame_shock -> cancel();
	p -> active_flame_shock = 0;
      }
    }
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    if( p -> active_flame_shock ) player_crit += 1.0;
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    if( fswait )
    {
      shaman_t* p = player -> cast_shaman();
      if( ! p -> active_flame_shock ) 
	return false;
      if( p -> active_flame_shock -> time_remaining > 4.0 )
	return false;
    }

    return true;
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "elemental_mastery", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.elemental_mastery );
    cooldown = 180.0;
    trigger_gcd = false;  
    if( ! options_str.empty() )
    {
      // This will prevent Elemental Mastery from being called before the desired "free spell" is ready to be cast.
      cooldown_group = options_str;
      duration_group = options_str;
    }
  }
   
  virtual void execute()
  {
    report_t::log( sim, "%s performs elemental_mastery", player -> name() );
    update_ready();
    shaman_t* p = player -> cast_shaman();
    p -> aura_gain( "Elemental Mastery" );
    p -> buffs_elemental_mastery = 1;
  }
};

// Natures Swiftness Spell ==================================================

struct shamans_swiftness_t : public shaman_spell_t
{
  shamans_swiftness_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "natures_swiftness", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.natures_swiftness );
    trigger_gcd = false;  
    cooldown = 180.0;
    if( ! options_str.empty() )
    {
      // This will prevent Natures Swiftness from being called before the desired "free spell" is ready to be cast.
      cooldown_group = options_str;
      duration_group = options_str;
    }
  }
   
  virtual void execute()
  {
    report_t::log( sim, "%s performs natures_swiftness", player -> name() );
    update_ready();
    shaman_t* p = player -> cast_shaman();
    p -> aura_gain( "Natures Swiftness" );
    p -> buffs_natures_swiftness = 1;
  }
};

// Earth Shock Spell =======================================================

struct earth_shock_t : public shaman_spell_t
{
  earth_shock_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "earth_shock", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 69, 8, 658, 692, 0, 535 },
      { 60, 7, 517, 545, 0, 450 },
      { 48, 6, 359, 381, 0, 345 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0; 
    dd_power_mod      = 0.41;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost       = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * ( AFTER_3_0_0 ? 0.04 : 0.02 );
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.natures_guidance * 0.01;
    base_hit        += p -> talents.elemental_precision * 0.02;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }
};

// Frost Shock Spell =======================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "frost_shock", player, SCHOOL_FROST, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 68, 5, 640, 676, 0, 525 },
      { 58, 4, 486, 514, 0, 430 },
      { 46, 3, 333, 353, 0, 325 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0; 
    dd_power_mod      = 0.41;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * ( AFTER_3_0_0 ? 0.04 : 0.02 );
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.natures_guidance * 0.01;
    base_hit        += p -> talents.elemental_precision * 0.02;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }
};

// Flame Shock Spell =======================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "flame_shock", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 70, 7, 377, 377, 420, 500 },
      { 60, 6, 309, 309, 344, 450 },
      { 52, 5, 230, 230, 256, 345 },
      { 40, 4, 152, 152, 168, 250 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0; 
    dd_power_mod      = 0.15;
    base_duration     = 12.0;
    num_ticks         = 6;
    dot_power_mod     = 0.52;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * ( AFTER_3_0_0 ? 0.04 : 0.02 );
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_dot         = rank -> dot * ( 1.0 + p -> talents.storm_earth_and_fire * 0.20 );
    dot_power_mod   *= ( 1.0 + p -> talents.storm_earth_and_fire * 0.20 );
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_multiplier *= 1.0 + p -> talents.lava_flow * 0.033333;
    base_hit        += p -> talents.natures_guidance * 0.01;
    base_hit        += p -> talents.elemental_precision * 0.02;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    if( result_is_hit() )
    {
      player -> cast_shaman() -> active_flame_shock = this;
    }
  }

  virtual void last_tick() 
  {
    shaman_spell_t::last_tick(); 
    player -> cast_shaman() -> active_flame_shock = 0;
  }
};

// Searing Totem Spell =======================================================

struct searing_totem_t : public shaman_spell_t
{
  searing_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "searing_totem", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 69, 7, 0, 0, 58 * num_ticks, 205 },
      { 60, 6, 0, 0, 47 * num_ticks, 170 },
      { 50, 5, 0, 0, 39 * num_ticks, 145 },
      { 40, 4, 0, 0, 30 * num_ticks, 110 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );

    base_execute_time = 0; 
    base_duration     = 60.0;
    dd_power_mod      = 0.08;
    num_ticks         = 30;
    may_crit          = true;
    duration_group    = "fire_totem";
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_multiplier *= 1.0 + p -> talents.call_of_flame * 0.05;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }

  // Odd things to handle:
  // (1) Execute is guaranteed.
  // (2) Each "tick" is like an "execute".
  // (3) No hit/miss events are triggered.

  virtual void execute() 
  {
    report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    player -> action_finish( this );
  }

  virtual void tick() 
  {
    report_t::debug( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    may_resist = false;
    calculate_result();
    may_resist = true;
    if( result_is_hit() )
    {
      calculate_damage();
      adjust_damage_for_result();
      if( dd > 0 )
      {
	dot_tick = dd;
	assess_damage( dot_tick, DMG_OVER_TIME );
      }
    }
    else
    {
      report_t::log( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), wow_result_type_string( result ) );
    }
    update_stats( DMG_OVER_TIME );
  }
};

// Totem of Wrath Spell =====================================================

struct totem_of_wrath_t : public shaman_spell_t
{
  totem_of_wrath_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "totem_of_wrath", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.totem_of_wrath );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    base_cost      = 400;
    base_cost     *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost     *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group = "fire_totem";
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
	name = "Totem of Wrath Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> aura_gain( "Totem of Wrath" );
	    p -> buffs.totem_of_wrath = 1;
	  }
	}
	time = 120;
	sim -> add_event( this );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> aura_loss( "Totem of Wrath" );
	    p -> buffs.totem_of_wrath = 0;
	  }
	}
      }
    };

    report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> buffs.totem_of_wrath == 0 );
  }
};

// Wrath of Air Totem Spell =================================================

struct wrath_of_air_totem_t : public shaman_spell_t
{
  wrath_of_air_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "wrath_of_air_totem", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    base_cost      = 320;
    base_cost     *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost     *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group = "air_totem";
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
	name = "Totem of Wrath Expiration";
	int8_t bonus = 101;  // barely fits in the 7 bits!
	if( player -> gear.tier4_2pc ) bonus += 20;
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> aura_gain( "Wrath of Air Totem" );
	    p -> buffs.wrath_of_air = bonus;
	  }
	}
	time = 120;
	sim -> add_event( this );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> aura_loss( "Wrath of Air Totem" );
	    p -> buffs.wrath_of_air = 0;
	  }
	}
      }
    };

    report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> buffs.wrath_of_air == 0 );
  }
};

// Mana Tide Totem Spell ==================================================

struct mana_tide_totem_t : public shaman_spell_t
{
  mana_tide_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "mana_tide_totem", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.mana_tide_totem );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    harmful        = false;
    base_duration  = 12.0; 
    num_ticks      = 4;
    cooldown       = 300.0;
    duration_group = "water_totem";
    base_cost      = 320;
    base_cost      *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost      *= 1.0 - p -> talents.mental_quickness * 0.02;
  }

  virtual void execute() 
  {
    report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    player -> action_finish( this );
  }

  virtual void tick() 
  {
    report_t::debug( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> party == player -> party )
      {
	p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.06, "mana_tide" );
      }
    }
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.75 * player -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Mana Spring Totem Spell ================================================

struct mana_spring_totem_t : public shaman_spell_t
{
  mana_spring_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "mana_tide_totem", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    harmful         = false;
    base_duration   = 120.0; 
    num_ticks       = 60;
    duration_group  = "water_totem";
    base_cost       = 120;
    base_cost      *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost      *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_multiplier = 1.0 + p -> talents.restorative_totems * 0.05;
  }

  virtual void execute() 
  {
    report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    player -> action_finish( this );
  }

  virtual void tick() 
  {
    report_t::debug( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> party == player -> party )
      {
	p -> resource_gain( RESOURCE_MANA, base_multiplier * 20.0, "mana_spring" );
      }
    }
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.95 * player -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Bloodlust Spell ===========================================================

struct bloodlust_t : public shaman_spell_t
{
  double target_pct;

  bloodlust_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "bloodlust", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), target_pct(0)
  {
    option_t options[] =
    {
      { "target", OPT_INT8, &target_pct },
      { NULL }
    };
    parse_options( options, options_str );
      
    harmful   = false;
    base_cost = 750;
    cooldown  = 600;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
	name = "Bloodlust Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> aura_gain( "Bloodlust" );
	    p -> buffs.bloodlust = 1;
	  }
	}
	time = 40;
	sim -> add_event( this );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> aura_loss( "Bloodlust" );
	    p -> buffs.bloodlust = 0;
	  }
	}
      }
    };

    report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    target_t* t = sim -> target;

    if( target_pct == 0 || t -> initial_health <= 0 )
      return true;

    return( ( t -> current_health / t -> initial_health ) < ( target_pct / 100.0 ) );
  }
};

// ============================================================================
// Shaman Event Tracking
// ============================================================================

// shaman_t::spell_start_event ================================================

void shaman_t::spell_start_event( spell_t* s )
{
  player_t::spell_start_event( s );

}

// shaman_t::spell_hit_event ==================================================

void shaman_t::spell_hit_event( spell_t* s )
{
  player_t::spell_hit_event( s );

  if( s -> result == RESULT_CRIT )
  {
    buffs_elemental_focus = 2;

    if( gear.tier4_4pc )
    {
      buffs.tier4_4pc = wow_random( 0.11 ) ;
    }
  }
}

// shaman_t::spell_finish_event ===============================================

void shaman_t::spell_finish_event( spell_t* s )
{
  player_t::spell_finish_event( s );

}

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
				   const std::string& options )
{
  if( name == "bloodlust"          ) return new           bloodlust_t( this, options );
  if( name == "chain_lightning"    ) return new     chain_lightning_t( this, options );
  if( name == "earth_shock"        ) return new         earth_shock_t( this, options );
  if( name == "elemental_mastery"  ) return new   elemental_mastery_t( this, options );
  if( name == "flame_shock"        ) return new         flame_shock_t( this, options );
  if( name == "frost_shock"        ) return new         frost_shock_t( this, options );
  if( name == "lava_burst"         ) return new          lava_burst_t( this, options );
  if( name == "lightning_bolt"     ) return new      lightning_bolt_t( this, options );
  if( name == "mana_spring_totem"  ) return new   mana_spring_totem_t( this, options );
  if( name == "mana_tide_totem"    ) return new     mana_tide_totem_t( this, options );
  if( name == "natures_swiftness"  ) return new   shamans_swiftness_t( this, options );
  if( name == "searing_totem"      ) return new       searing_totem_t( this, options );
  if( name == "totem_of_wrath"     ) return new      totem_of_wrath_t( this, options );
  if( name == "wrath_of_air_totem" ) return new  wrath_of_air_totem_t( this, options );

  return 0;
}

// shaman_t::init_base ========================================================

void shaman_t::init_base()
{

  attribute_base[ ATTR_STRENGTH  ] = 105;
  attribute_base[ ATTR_AGILITY   ] =  60;
  attribute_base[ ATTR_STAMINA   ] = 115;
  attribute_base[ ATTR_INTELLECT ] = 105;
  attribute_base[ ATTR_SPIRIT    ] = 120;

  base_spell_crit = 0.0225;
  spell_crit_per_intellect = 0.01 / ( level + 10 );
  spell_power_per_intellect = talents.natures_blessing * 0.10;

  base_attack_power = 120;
  base_attack_crit  = 0.0167;
  attack_power_per_strength = 2.0;
  attack_crit_per_agility = 1.0 / ( 25 + ( level - 70 ) * 0.5 );

  resource_base[ RESOURCE_HEALTH ] = 3185;
  resource_base[ RESOURCE_MANA   ] = 2680;

  if( gear.tier6_2pc )
  {
    // Simply assume the totems are out all the time.

    gear.spell_power_enchant[ SCHOOL_MAX ] += 45;
    gear.spell_crit_rating_enchant         += 35;
    gear.mp5_enchant                       += 15;
  }
}

// shaman_t::reset ===========================================================

void shaman_t::reset()
{
  player_t::reset();

  // Totems
  fire_totem  = 0;
  air_totem   = 0;
  water_totem = 0;
  earth_totem = 0;;

  // Active
  active_flame_shock = 0;

  // Buffs
  buffs_elemental_focus = 0;
  buffs_elemental_mastery = 0;
  buffs_natures_swiftness = 0;
}

// shaman_t::regen  ==========================================================

void shaman_t::regen()
{
  double spirit_regen = spirit_regen_per_second() * 2.0;

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;
  }
  else if( recent_cast() )
  {
    spirit_regen = 0;
  }

  double mp5_regen = mp5 + intellect() * talents.unrelenting_storm * 0.02;

  mp5_regen /= 2.5;

  resource_gain( RESOURCE_MANA, spirit_regen, "spirit_regen" );
  resource_gain( RESOURCE_MANA,    mp5_regen, "mp5_regen"    );
}

// shaman_t::parse_talents ===============================================

void shaman_t::parse_talents( const std::string& talent_string )
{
  assert( talent_string.size() == 61 );
  
  talent_translation_t translation[] =
  {
    {  1,  &( talents.convection          ) },
    {  2,  &( talents.concussion          ) },
    {  5,  &( talents.call_of_flame       ) },
    {  6,  &( talents.elemental_focus     ) },
    {  7,  &( talents.reverberation       ) },
    {  8,  &( talents.call_of_thunder     ) },
    { 13,  &( talents.elemental_fury      ) },
    { 14,  &( talents.unrelenting_storm   ) },
    { 15,  &( talents.elemental_precision ) },
    { 16,  &( talents.lightning_mastery   ) },
    { 17,  &( talents.elemental_mastery   ) },
    { 19,  &( talents.lightning_overload  ) },
    { 20,  &( talents.totem_of_wrath      ) },
    { 35,  &( talents.mental_quickness    ) },
    { 46,  &( talents.totemic_focus       ) },
    { 47,  &( talents.natures_guidance    ) },
    { 51,  &( talents.restorative_totems  ) },
    { 52,  &( talents.tidal_mastery       ) },
    { 54,  &( talents.natures_swiftness   ) },
    { 57,  &( talents.mana_tide_totem     ) },
    { 59,  &( talents.natures_blessing    ) },
    { 0, NULL }
  };

  player_t::parse_talents( translation, talent_string );
}

// shaman_t::parse_option  ==============================================

bool shaman_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    { "call_of_flame",        OPT_INT8,  &( talents.call_of_flame        ) },
    { "call_of_thunder",      OPT_INT8,  &( talents.call_of_thunder      ) },
    { "concussion",           OPT_INT8,  &( talents.concussion           ) },
    { "convection",           OPT_INT8,  &( talents.convection           ) },
    { "elemental_focus",      OPT_INT8,  &( talents.elemental_focus      ) },
    { "elemental_fury",       OPT_INT8,  &( talents.elemental_fury       ) },
    { "elemental_mastery",    OPT_INT8,  &( talents.elemental_mastery    ) },
    { "elemental_oath",       OPT_INT8,  &( talents.elemental_oath       ) },
    { "elemental_precision",  OPT_INT8,  &( talents.elemental_precision  ) },
    { "lava_flow"          ,  OPT_INT8,  &( talents.lava_flow            ) },
    { "lightning_mastery",    OPT_INT8,  &( talents.lightning_mastery    ) },
    { "lightning_overload",   OPT_INT8,  &( talents.lightning_overload   ) },
    { "mana_tide_totem",      OPT_INT8,  &( talents.mana_tide_totem      ) },
    { "mental_quickness",     OPT_INT8,  &( talents.mental_quickness     ) },
    { "natures_blessing",     OPT_INT8,  &( talents.natures_blessing     ) },
    { "natures_guidance",     OPT_INT8,  &( talents.natures_guidance     ) },
    { "natures_swiftness",    OPT_INT8,  &( talents.natures_swiftness    ) },
    { "restorative_totems",   OPT_INT8,  &( talents.restorative_totems   ) },
    { "reverberation",        OPT_INT8,  &( talents.reverberation        ) },
    { "storm_earth_and_fire", OPT_INT8,  &( talents.storm_earth_and_fire ) },
    { "tidal_mastery",        OPT_INT8,  &( talents.tidal_mastery        ) },
    { "totem_of_wrath",       OPT_INT8,  &( talents.totem_of_wrath       ) },
    { "totemic_focus",        OPT_INT8,  &( talents.totemic_focus        ) },
    { "unrelenting_storm",    OPT_INT8,  &( talents.unrelenting_storm    ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( options );
    return false;
  }

  if( player_t::parse_option( name, value ) ) return true;

  return option_t::parse( options, name, value );
}

// player_t::create_shaman  =================================================

player_t* player_t::create_shaman( sim_t*       sim, 
				   std::string& name ) 
{
  return new shaman_t( sim, name );
}
