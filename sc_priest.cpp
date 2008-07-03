// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Priest
// ==========================================================================

struct priest_t : public player_t
{
  // shadow_fiend_t* shadow_fiend;

  struct talents_t
  {
    int8_t  darkness;
    int8_t  divine_fury;
    int8_t  focused_mind;
    int8_t  focused_power;
    int8_t  force_of_will;
    int8_t  holy_specialization;
    int8_t  improved_divine_spirit;
    int8_t  improved_shadow_word_pain;
    int8_t  improved_mind_blast;
    int8_t  improved_vampiric_embrace;
    int8_t  inner_focus;
    int8_t  meditation;
    int8_t  mental_agility;
    int8_t  mental_strength;
    int8_t  misery;
    int8_t  power_infusion;
    int8_t  searing_light;
    int8_t  shadow_focus;
    int8_t  shadow_form;
    int8_t  shadow_power;
    int8_t  shadow_weaving;
    int8_t  spiritual_guidance;
    int8_t  surge_of_light;
    int8_t  vampiric_embrace;
    int8_t  vampiric_touch;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  priest_t( sim_t* sim, std::string& name ) : player_t( sim, PRIEST, name ) {}

  // Character Definition
  virtual void      init();
  virtual void      init_base();
  virtual void      init_resources();
  virtual void      reset();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );

  // Event Tracking
  virtual void regen();
  virtual void spell_hit_event   ( spell_t* );
  virtual void spell_finish_event( spell_t* );
  virtual void spell_damage_event( spell_t*, double amount, int8_t dmg_type );
};

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public spell_t
{
  priest_spell_t( const char* n, player_t* p, int8_t s, int8_t t ) : 
    spell_t( n, p, RESOURCE_MANA, s, t ) {}

  virtual void   player_buff();
  virtual void   execute();
  virtual double cost();

  // Passthru Methods
  virtual double execute_time() { return spell_t::execute_time(); }
  virtual void tick()           { spell_t::tick();                }
  virtual void last_tick()      { spell_t::last_tick();           }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// stack_shadow_weaving =====================================================

static void stack_shadow_weaving( spell_t* s )
{
  priest_t* p = s -> player -> priest();

  struct shadow_weaving_expiration_t : public event_t
  {
    shadow_weaving_expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Shadow Weaving Expiration";
      time = 15.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      report_t::log( sim, "%s loses Shadow Weaving", sim -> target -> name() );
      sim -> target -> debuffs.shadow_weaving = 0;
      sim -> target -> expirations.shadow_weaving = 0;
    }
  };

  if( wow_random( p -> talents.shadow_weaving * 0.20 ) )
  {
    target_t* t = s -> sim -> target;

    if( t -> debuffs.shadow_weaving < 5 ) 
    {
      t -> debuffs.shadow_weaving++;
      report_t::log( s -> sim, "%s gains Shadow Weaving %d", t -> name(), t -> debuffs.shadow_weaving );
    }

    event_t*& e = t -> expirations.shadow_weaving;
    
    if( e )
    {
      e -> reschedule( s -> sim -> current_time + 15.0 );
    }
    else
    {
      e = new shadow_weaving_expiration_t( s -> sim );
    }
  }
}

// push_misery =================================================================

static void push_misery( spell_t* s )
{
  target_t* t = s -> sim -> target;
  priest_t* p = s -> player -> priest();

  t -> debuffs.misery_stack++;
  if( p -> talents.misery > t -> debuffs.misery )
  {
    t -> debuffs.misery = p -> talents.misery;
  }
}

// pop_misery ==================================================================

static void pop_misery( spell_t* s )
{
  priest_t* p = s -> player -> priest();
  target_t* t = s -> sim -> target;
    
  if( p -> talents.misery )
  {
    t -> debuffs.misery_stack--;
    if( t -> debuffs.misery_stack == 0 ) t -> debuffs.misery = 0;
  }
}

// push_tier5_2pc =============================================================

static void push_tier5_2pc( spell_t*s )
{
  priest_t* p = s -> player -> priest();

  assert( p -> buffs.tier5_2pc == 0 );

  if( p -> gear.tier5_2pc && wow_random( 0.06 ) )
  {
    p -> buffs.tier5_2pc = 1;
    p -> buffs.mana_cost_reduction += 150;
    p -> proc( "tier5_2pc" );
  }

}

// pop_tier5_2pc =============================================================

static void pop_tier5_2pc( spell_t*s )
{
  priest_t* p = s -> player -> priest();

  if( p -> buffs.tier5_2pc )
  {
    p -> buffs.tier5_2pc = 0;
    p -> buffs.mana_cost_reduction -= 150;
  }
}

// push_tier5_4pc =============================================================

static void push_tier5_4pc( spell_t*s )
{
  priest_t* p = s -> player -> priest();

  if(   p ->  gear.tier5_4pc && 
      ! p -> buffs.tier5_4pc &&
	wow_random( 0.40 ) )
  {
    p -> buffs.tier5_4pc = 1;
    p -> spell_power[ SCHOOL_MAX ] += 100;
    p -> proc( "tier5_4pc" );
  }
}

// pop_tier5_4pc =============================================================

static void pop_tier5_4pc( spell_t*s )
{
  priest_t* p = s -> player -> priest();

  if( p -> buffs.tier5_4pc)
  {
    p -> buffs.tier5_4pc = 0;
    p -> spell_power[ SCHOOL_MAX ] -= 100;
  }
}

// trigger_ashtongue_talisman ======================================================

static void trigger_ashtongue_talisman( spell_t* s )
{
  struct ashtongue_talisman_expiration_t : public event_t
  {
    ashtongue_talisman_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Ashtongue Talisman Expiration";
      player -> aura_gain( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] += 220;
      time = 10.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      player -> aura_loss( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] -= 220;
      player -> expirations.ashtongue_talisman = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.ashtongue_talisman && wow_random( 0.10 ) )
  {
    p -> proc( "ashtongue_talisman" );

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( s -> sim -> current_time + 10.0 );
    }
    else
    {
      e = new ashtongue_talisman_expiration_t( s -> sim, p );
    }
  }
}

// trigger_surge_of_light =====================================================

static void trigger_surge_of_light( spell_t* s )
{
  priest_t* priest = s -> player -> priest();

  if( priest -> talents.surge_of_light )
  {
    priest -> buffs.surge_of_light = ( s -> result == RESULT_CRIT );
  }
}

} // ANONYMOUS NAMESPACE =====================================================

// ==========================================================================
// Priest Spell
// ==========================================================================

// priest_spell_t::player_buff ================================================

void priest_spell_t::player_buff()
{
  spell_t::player_buff();

  priest_t* p = player -> priest();

  player_power += p -> spirit * p -> talents.spiritual_guidance * 0.05;
}

// priest_spell_t::execute ===================================================

void priest_spell_t::execute()
{
  spell_t::execute();

  if( player -> buffs.inner_focus )
  {
    player -> aura_loss( "Inner Focus" );
    player -> buffs.inner_focus = 0;
  }
}

// priest_spell_t::cost ======================================================

double priest_spell_t::cost()
{
  if( player -> buffs.inner_focus ) return 0;
  return spell_t::cost();
}

// Holy Fire Spell ===========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "holy_fire", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> priest();
    
    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 66, 9, 412, 523, 165, 290 },
      { 60, 8, 355, 449, 145, 255 },
      { 54, 7, 304, 386, 125, 230 },
      { 48, 6, 254, 322, 100, 200 },
      { 42, 5, 204, 258,  85, 170 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
      
    base_execute_time = 3.5; 
    base_duration     = 10.0; 
    num_ticks         = 5;
    dd_power_mod      = 0.857; 
    dot_power_mod     = 0.165; 
    may_crit          = true;    

    base_execute_time -= p -> talents.divine_fury * 0.01;
    base_multiplier   *= 1.0 + p -> talents.searing_light * 0.05;
    base_multiplier   *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit         += p -> talents.holy_specialization * 0.01;
    base_crit         += p -> talents.force_of_will * 0.01;
  }
};

// Smite Spell ================================================================

struct smite_t : public priest_spell_t
{
  smite_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "smite", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 69, 10, 545, 611, 0, 385 },
      { 61,  9, 405, 455, 0, 300 },
      { 54,  8, 371, 415, 0, 280 },
      { 46,  7, 287, 323, 0, 230 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    base_execute_time = 2.5; 
    dd_power_mod      = (2.5/3.5); 
    may_crit          = true;
      
    base_execute_time -= p -> talents.divine_fury * 0.1;
    base_multiplier   *= 1.0 + p -> talents.searing_light * 0.05;
    base_multiplier   *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit         += p -> talents.holy_specialization * 0.01;
    base_crit         += p -> talents.force_of_will * 0.01;
    base_hit          += p -> talents.focused_power * 0.02;
    
    if( p -> gear.tier4_4pc ) base_multiplier *= 1.05;
  }

  virtual double execute_time()
  {
    return player -> buffs.surge_of_light ? 0 : priest_spell_t::execute_time();
  }

  virtual double cost()
  {
    return player -> buffs.surge_of_light ? 0 : priest_spell_t::cost();
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    may_crit = ! ( player -> buffs.surge_of_light );
  }
};

// Shadow Word Pain Spell ======================================================

struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "shadow_word_pain", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 70, 10, 0, 0, 1236, 575 },
      { 65,  9, 0, 0, 1002, 510 },
      { 58,  8, 0, 0,  852, 470 },
      { 50,  7, 0, 0,  672, 385 },
      { 42,  6, 0, 0,  510, 305 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
      
    base_execute_time = 0; 
    base_duration     = 18.0; 
    num_ticks         = 6;
    dot_power_mod     = 1.10; 

    base_dot  = rank -> dot;
    base_cost = rank -> cost;
    base_cost *= 1.0 - p -> talents.mental_agility * 0.02;

    int8_t more_ticks = p -> talents.improved_shadow_word_pain;
    if( p -> gear.tier6_2pc ) more_ticks++;
    if( more_ticks > 0 )
    {
      double adjust = ( num_ticks + more_ticks ) / (double) num_ticks;

      base_dot      *= adjust;
      dot_power_mod *= adjust;
      base_duration *= adjust;
      num_ticks     += more_ticks;
    }

    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * 0.02;
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    push_misery( this );
  }

  virtual void tick() 
  {
    priest_spell_t::tick(); 
    push_tier5_4pc( this );
    trigger_ashtongue_talisman( this );
  }

  virtual void last_tick() 
  {
    priest_spell_t::last_tick(); 
    pop_misery( this );
  }
};

// Vampiric Touch Spell ======================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "vampiric_touch", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> priest();

    assert( p -> talents.vampiric_touch );
     
    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 70, 3, 0, 0, 690, 475 },
      { 60, 2, 0, 0, 640, 400 },
      { 50, 1, 0, 0, 450, 325 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
      
    base_execute_time = 1.5; 
    base_duration     = 15.0; 
    num_ticks         = 5;
    dot_power_mod     = 1.0; 
      
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * 0.02;
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    player -> buffs.vampiric_touch = 1;
    push_misery( this );
  }

  virtual void last_tick() 
  {
    priest_spell_t::last_tick(); 
    pop_misery( this );
    player -> buffs.vampiric_touch = 0;
  }
};

// Devouring Plague Spell ======================================================

struct devouring_plague_t : public priest_spell_t
{
  devouring_plague_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "devouring_plague", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 68, 7, 0, 0, 1216, 1145 },
      { 60, 6, 0, 0,  904,  985 },
      { 52, 5, 0, 0,  712,  810 },
      { 44, 4, 0, 0,  544,  645 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
      
    base_execute_time = 0; 
    base_duration     = 24.0; 
    num_ticks         = 8;  
    cooldown          = 180.0;
    binary            = true;
    dot_power_mod     = ( 24.0 / 15.0 ) / 2.0; 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.mental_agility * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * 0.02;
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    push_misery( this );
  }

  virtual void tick() 
  {
    priest_spell_t::tick(); 
    player -> resource_gain( RESOURCE_HEALTH, dot_tick, "devouring_plague" );
  }

  virtual void last_tick() 
  {
    priest_spell_t::last_tick(); 
    pop_misery( this );
  }
};

// Vampiric Embrace Spell ======================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "vampiric_embrace", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> priest();

    assert( p -> talents.vampiric_embrace );
     
    base_execute_time = 0; 
    base_duration     = 60.0; 
    num_ticks         = 1;
    base_cost         = 30;
    base_multiplier   = 0;
    base_hit          = p -> talents.shadow_focus * 0.02;
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result == RESULT_HIT ) player -> buffs.vampiric_embrace = 1;
  }

  virtual void tick() 
  {
    priest_spell_t::tick(); 
    player -> buffs.vampiric_embrace = 0;
  }
};

// Mind Blast Spell ============================================================

struct mind_blast_t : public priest_spell_t
{
  mind_blast_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "mind_blast", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 69, 11, 708, 748, 0, 450 },
      { 63, 10, 557, 588, 0, 380 },
      { 58,  9, 503, 531, 0, 350 },
      { 52,  8, 425, 449, 0, 310 },
      { 46,  7, 346, 366, 0, 265 },
      { 40,  6, 279, 297, 0, 225 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    base_execute_time = 1.5; 
    cooldown          = 8.0;
    may_crit          = true; 
    dd_power_mod      = (1.5/3.5); 
      
    base_cost       = rank -> cost;
    base_cost       *= 1.0 - p -> talents.focused_mind * 0.05;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit       += p -> talents.shadow_power * 0.03;
    base_crit       += p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * 0.02;
    base_hit        += p -> talents.focused_power * 0.02;
    cooldown        -= p -> talents.improved_mind_blast * 0.5;
    
    if( p -> gear.tier6_4pc ) base_multiplier *= 1.10;
  }
};

// Shadow Word Death Spell ======================================================

struct shadow_word_death_t : public priest_spell_t
{
  shadow_word_death_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "shadow_word_death", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> priest();

    assert( sim -> patch.after( 2, 0, 0 ) );

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 70, 2, 572, 664, 0, 309 },
      { 62, 1, 450, 522, 0, 243 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    may_crit          = true; 
    cooldown          = 12.0;
    dd_power_mod      = (1.5/3.5); 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.mental_agility * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit       += p -> talents.shadow_power * 0.03;
    base_crit       += p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * 0.02;
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    player -> resource_loss( RESOURCE_HEALTH, dd, "shadow_word_death" );
  }
};

// Mind Flay Spell ============================================================

struct mind_flay_t : public priest_spell_t
{
  double  wait;
  int8_t cancel;
  mind_flay_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "mind_flay", player, SCHOOL_SHADOW, TREE_SHADOW ), wait(0), cancel(0)
  {
    priest_t* p = player -> priest();

    option_t options[] =
    {
      { "rank",   OPT_INT8, &rank_index },
      { "wait",   OPT_FLT,  &wait       },
      { "cancel", OPT_INT8, &cancel     },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 68, 7, 0, 0, 528, 230 },
      { 60, 6, 0, 0, 426, 205 },
      { 52, 5, 0, 0, 330, 165 },
      { 44, 4, 0, 0, 261, 135 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    base_execute_time = 0.0; 
    base_duration     = 3.0; 
    num_ticks         = 3;
    channeled         = true; 
    binary            = true; 
    dot_power_mod     = 0.57;

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.focused_mind * 0.05;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * 0.02;
    
    if( p -> gear.tier4_4pc ) base_multiplier *= 1.05;
  }

  virtual void tick()
  {
    priest_spell_t::tick();

    if( cancel       != 0 && 
	current_tick == 2 )
    {
      // Cancel spell after two ticks if another spell with higher priority is ready.

      for( action_t* a = player -> action_list; a; a = a -> next )
      {
	if( a == this ) break;
	if( a -> valid == false ) continue;
	if( a -> name_str == "mind_flay" ) continue;
	if( ! a -> harmful ) continue;

	if( a -> ready() )
	{
	  time_remaining = 0;
	  current_tick = 3;
	  break;
	}
      }
    }
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    if( wait > 0 )
    {
      // Do not start this spell if another spell with higher priority will be ready in "wait" seconds.

      for( action_t* a = player -> action_list; a; a = a -> next )
      {
	if( a == this ) break;
	if( a -> valid == false ) continue;
	if( a -> name_str == "mind_flay" ) continue;
	if( ! a -> harmful ) continue;

	if( a -> debuff_ready > 0 &&
	    a -> debuff_ready > ( wait + sim -> current_time + a -> execute_time() ) )
	  continue;

	if( a -> cooldown_ready <= ( wait + sim -> current_time ) )
	  continue;

	if( player -> resource_available( resource, cost() ) )
	  continue;

	return false;
      }
    }
    return true;
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
{
  struct power_infusion_expiration_t : public event_t
  {
    power_infusion_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Power Infusion Expiration";
      time = 15.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      player -> aura_loss( "Power Infusion" );
      player -> buffs.power_infusion = 0;
      player -> haste_rating -= 320;
      player -> recalculate_haste();
    }
  };
   
  power_infusion_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "power_infusion", player, SCHOOL_ARCANE, TREE_DISCIPLINE )
  {
    priest_t* p = player -> priest();
    assert( p -> talents.power_infusion );
    trigger_gcd = false;  
    cooldown = 180.0;
  }
   
  virtual void execute()
  {
    report_t::log( sim, "Player %s casts Power Infusion", player -> name() );
    player -> aura_gain( "Power Infusion" );
    player -> buffs.power_infusion = 1;
    player -> haste_rating += 320;
    player -> recalculate_haste();
    new power_infusion_expiration_t( sim, player );
  }
};

// Inner Focus Spell =====================================================

struct inner_focus_t : public priest_spell_t
{
  inner_focus_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "inner_focus", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    priest_t* p = player -> priest();
    assert( p -> talents.inner_focus );
    trigger_gcd = false;  
    cooldown = 180.0;
    assert( options_str.size() > 0 );
    // This will prevent InnerFocus from being called before the desired "free spell" is ready to be cast.
    cooldown_group = options_str;
    debuff_group   = options_str;
  }
   
  virtual void execute()
  {
    report_t::log( sim, "Player %s casts Inner Focus", player -> name() );
    player -> aura_gain( "Inner Focus" );
    player -> buffs.inner_focus = 1;
    cooldown_ready = sim -> current_time + cooldown;
  }
};

// Shadow Form Spell =======================================================

struct shadow_form_t : public priest_spell_t
{
  shadow_form_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "shadow_form", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> priest();
    assert( p -> talents.shadow_form );
    trigger_gcd = false;
  }
   
  virtual void execute()
  {
    report_t::log( sim, "%s performs shadow_form", player -> name() );
    player -> buffs.shadow_form = 1;
  }

  virtual bool ready()
  {
    return( player -> buffs.shadow_form == 0 );
  }
};

// Shadow Fiend Spell ========================================================

struct shadow_fiend_t : public priest_spell_t
{
  double mana;

  shadow_fiend_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "shadow_fiend", player, SCHOOL_SHADOW, TREE_SHADOW ), mana(0)
  {
    priest_t* p = player -> priest();

    // assert( p -> shadow_fiend );

    mana        = atof( options_str.c_str() );
    harmful    = false;
    cooldown   = 300.0;
    base_cost  = 300;
    base_cost *= 1.0 - p -> talents.mental_agility * 0.02;
  }

  virtual void execute() 
  {
    update_cooldowns();
    consume_resource();

    // shadow_fiend -> schedule_ready();

    player -> action_finish( this );
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    return( player -> resource_initial[ RESOURCE_MANA ] - 
	    player -> resource_current[ RESOURCE_MANA ] ) > mana;
  }
};

// ==========================================================================
// Priest Event Tracking
// ==========================================================================

// priest_t::spell_hit_event ================================================

void priest_t::spell_hit_event( spell_t* s )
{
  if( s -> school == SCHOOL_SHADOW )
  {
    stack_shadow_weaving( s );
    if( s -> num_ticks ) push_misery( s );
  }

  trigger_surge_of_light( s );
}

// priest_t::spell_finish_event ==============================================

void priest_t::spell_finish_event( spell_t* s )
{
  if( s -> harmful )
  {
    pop_tier5_2pc ( s );
    push_tier5_2pc( s );
  }
  pop_tier5_4pc( s );
}

// priest_t::spell_damage_event ==============================================

void priest_t::spell_damage_event( spell_t* s,
				   double   amount,
				   int8_t   dmg_type )
{
  if( s -> school == SCHOOL_SHADOW )
  {
    if( buffs.vampiric_touch )
    {
      double mana = amount * 0.05;

      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> party == party )
        {
	  p -> resource_gain( RESOURCE_MANA, mana, "vampiric_touch" );
	}
      }
    }
    if( buffs.vampiric_embrace )
    {
      double adjust = 0.15 + talents.improved_vampiric_embrace * 0.05;
      double health = amount * adjust;

      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> party == party )
        {
	  p -> resource_gain( RESOURCE_HEALTH, health, "vampiric_embrace" );
	}
      }
    }
  }
}

// ==========================================================================
// Priest Character Definition
// ==========================================================================

// priest_t::create_action ===================================================

action_t* priest_t::create_action( const std::string& name,
				   const std::string& options_str )
{
  if( name == "devouring_plague"  ) return new devouring_plague_t ( this, options_str );
  if( name == "holy_fire"         ) return new holy_fire_t        ( this, options_str );
  if( name == "inner_focus"       ) return new inner_focus_t      ( this, options_str );
  if( name == "mind_blast"        ) return new mind_blast_t       ( this, options_str );
  if( name == "mind_flay"         ) return new mind_flay_t        ( this, options_str );
  if( name == "power_infusion"    ) return new power_infusion_t   ( this, options_str );
  if( name == "shadow_fiend"      ) return new shadow_fiend_t     ( this, options_str );
  if( name == "shadow_word_death" ) return new shadow_word_death_t( this, options_str );
  if( name == "shadow_word_pain"  ) return new shadow_word_pain_t ( this, options_str );
  if( name == "shadow_form"       ) return new shadow_form_t      ( this, options_str );
  if( name == "smite"             ) return new smite_t            ( this, options_str );
  if( name == "vampiric_embrace"  ) return new vampiric_embrace_t ( this, options_str );
  if( name == "vampiric_touch"    ) return new vampiric_touch_t   ( this, options_str );

  return 0;
}

// priest_t::init ============================================================

void priest_t::init()
{
  player_t::init();

  // Create shadowfiend pet here.
}

// priest_t::init_base =======================================================

void priest_t::init_base()
{
  base_strength  = 40;
  base_agility   = 45;
  base_stamina   =  60;
  base_intellect = 145;
  base_spirit    = 155;

  base_spell_crit = 0.0125;

  base_attack_power = -10;
  base_attack_crit  = 0.03;

  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = 2340;
}

// priest_t::init_resources ==================================================

void priest_t::init_resources()
{
  mana_per_intellect *= 1.0 + 0.02 * talents.mental_strength;

  player_t::init_resources();
}

// priest_t::reset ===========================================================

void priest_t::reset()
{
  player_t::reset();
}

// priest_t::regen  ==========================================================

void priest_t::regen()
{
  double spirit_regen = spirit_regen_per_second() * 2.0;

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;
  }
  else if( recent_cast() )
  {
    spirit_regen *= talents.meditation * 0.10;
  }

  double mp5_regen = mp5 / 2.5;

  resource_gain( RESOURCE_MANA, spirit_regen, "spirit_regen" );
  resource_gain( RESOURCE_MANA,    mp5_regen, "mp5_regen"    );
}

// priest_t::parse_talents =================================================

void priest_t::parse_talents( const std::string& talent_string )
{
  assert( talent_string.size() == 64 );
  
  talent_translation_t translation[] =
  {
    {  8,  &( talents.inner_focus               ) },
    {  9,  &( talents.meditation                ) },
    { 11,  &( talents.mental_agility            ) },
    { 13,  &( talents.mental_strength           ) },
    { 15,  &( talents.improved_divine_spirit    ) },
    { 16,  &( talents.focused_power             ) },
    { 17,  &( talents.force_of_will             ) },
    { 19,  &( talents.power_infusion            ) },
    { 25,  &( talents.holy_specialization       ) },
    { 27,  &( talents.divine_fury               ) },
    { 33,  &( talents.searing_light             ) },
    { 36,  &( talents.spiritual_guidance        ) },
    { 37,  &( talents.surge_of_light            ) },
    { 47,  &( talents.improved_shadow_word_pain ) },
    { 48,  &( talents.shadow_focus              ) },
    { 50,  &( talents.improved_mind_blast       ) },
    { 54,  &( talents.shadow_weaving            ) },
    { 56,  &( talents.vampiric_embrace          ) },
    { 57,  &( talents.improved_vampiric_embrace ) },
    { 58,  &( talents.focused_mind              ) },
    { 60,  &( talents.darkness                  ) },
    { 61,  &( talents.shadow_form               ) },
    { 62,  &( talents.shadow_power              ) },
    { 63,  &( talents.misery                    ) },
    { 64,  &( talents.vampiric_touch            ) },
    { 0, NULL }
  };

  player_t::parse_talents( translation, talent_string );
}

// priest_t::parse_option  =================================================

bool priest_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    { "darkness",                  OPT_INT8,  &( talents.darkness                  ) },
    { "divine_fury",               OPT_INT8,  &( talents.divine_fury               ) },
    { "focused_mind",              OPT_INT8,  &( talents.focused_mind              ) },
    { "focused_power",             OPT_INT8,  &( talents.focused_power             ) },
    { "force_of_will",             OPT_INT8,  &( talents.force_of_will             ) },
    { "holy_specialization",       OPT_INT8,  &( talents.holy_specialization       ) },
    { "improved_divine_spirit",    OPT_INT8,  &( talents.improved_divine_spirit    ) },
    { "improved_shadow_word_pain", OPT_INT8,  &( talents.improved_shadow_word_pain ) },
    { "improved_mind_blast",       OPT_INT8,  &( talents.improved_mind_blast       ) },
    { "improved_vampiric_embrace", OPT_INT8,  &( talents.improved_vampiric_embrace ) },
    { "inner_focus",               OPT_INT8,  &( talents.inner_focus               ) },
    { "meditation",                OPT_INT8,  &( talents.meditation                ) },
    { "mental_agility",            OPT_INT8,  &( talents.mental_agility            ) },
    { "mental_strength",           OPT_INT8,  &( talents.mental_strength           ) },
    { "misery",                    OPT_INT8,  &( talents.misery                    ) },
    { "power_infusion",            OPT_INT8,  &( talents.power_infusion            ) },
    { "searing_light",             OPT_INT8,  &( talents.searing_light             ) },
    { "shadow_focus",              OPT_INT8,  &( talents.shadow_focus              ) },
    { "shadow_form",               OPT_INT8,  &( talents.shadow_form               ) },
    { "shadow_power",              OPT_INT8,  &( talents.shadow_power              ) },
    { "shadow_weaving",            OPT_INT8,  &( talents.shadow_weaving            ) },
    { "spiritual_guidance",        OPT_INT8,  &( talents.spiritual_guidance        ) },
    { "surge_of_light",            OPT_INT8,  &( talents.surge_of_light            ) },
    { "vampiric_embrace",          OPT_INT8,  &( talents.vampiric_embrace          ) },
    { "vampiric_touch",            OPT_INT8,  &( talents.vampiric_touch            ) },
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

// player_t::create_priest  =================================================

priest_t* player_t::create_priest( sim_t*       sim, 
				   std::string& name ) 
{
  return new priest_t( sim, name );
}


