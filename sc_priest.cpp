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
  spell_t* devouring_plague_active;
  spell_t* shadow_word_pain_active;
  spell_t* vampiric_touch_active;
  spell_t* vampiric_embrace_active;

  // Buffs
  int8_t buffs_improved_spirit_tap;
  int8_t buffs_inner_focus;
  int8_t buffs_surge_of_light;

  // Expirations
  event_t* expirations_improved_spirit_tap;

  // Up-Times
  uptime_t* uptimes_improved_spirit_tap;

  struct talents_t
  {
    int8_t  darkness;
    int8_t  dispersion;
    int8_t  divine_fury;
    int8_t  enlightenment;
    int8_t  focused_mind;
    int8_t  focused_power;
    int8_t  force_of_will;
    int8_t  holy_specialization;
    int8_t  improved_divine_spirit;
    int8_t  improved_shadow_word_pain;
    int8_t  improved_mind_blast;
    int8_t  improved_spirit_tap;
    int8_t  improved_vampiric_embrace;
    int8_t  inner_focus;
    int8_t  meditation;
    int8_t  mental_agility;
    int8_t  mental_strength;
    int8_t  misery;
    int8_t  pain_and_suffering;
    int8_t  power_infusion;
    int8_t  searing_light;
    int8_t  shadow_focus;
    int8_t  shadow_form;
    int8_t  shadow_power;
    int8_t  shadow_weaving;
    int8_t  spiritual_guidance;
    int8_t  surge_of_light;
    int8_t  twisted_faith;
    int8_t  vampiric_embrace;
    int8_t  vampiric_touch;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  priest_t( sim_t* sim, std::string& name ) : player_t( sim, PRIEST, name ) 
  {
    devouring_plague_active = 0;
    shadow_word_pain_active = 0;
    vampiric_touch_active = 0;
    vampiric_embrace_active = 0;
    uptimes_improved_spirit_tap = sim -> get_uptime( name + "_improved_spirit_tap" );
    expirations_improved_spirit_tap = 0;
    buffs_improved_spirit_tap = 0;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );

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

  virtual void   execute();
  virtual double cost();

  // Passthru Methods
  virtual double execute_time() { return spell_t::execute_time(); }
  virtual void player_buff()    { spell_t::player_buff();         }
  virtual void tick()           { spell_t::tick();                }
  virtual void last_tick()      { spell_t::last_tick();           }
};

// ==========================================================================
// Pet Shadow Fiend
// ==========================================================================

struct shadow_fiend_pet_t : public pet_t
{
  struct auto_attack_t : public attack_t
  {
    auto_attack_t( player_t* player ) : 
      attack_t( "melee", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      weapon = player -> main_hand_weapon;
      base_execute_time = weapon -> swing_time;
      base_dd = 1;
      valid = false;
      background = true;
      repeating = true;
    }
    void player_buff()
    {
      attack_t::player_buff();

      player_t* o = player -> cast_pet() -> owner;

      player_power += 0.57 * ( o -> spell_power[ SCHOOL_MAX    ] + 
			       o -> spell_power[ SCHOOL_SHADOW ] );

      // Arbitrary until I figure out base stats.
      player_crit = 0.05;
    }
    void assess_damage( double amount, int8_t dmg_type )
    {
      attack_t::assess_damage( amount, dmg_type );

      player -> cast_pet() -> owner -> resource_gain( RESOURCE_MANA, amount * 2.5, "shadow_fiend" );
    }
  };

  auto_attack_t* auto_attack;

  shadow_fiend_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name )
  {
    main_hand_weapon = new weapon_t( WEAPON_BEAST_1H, 110, 1.5, SCHOOL_SHADOW );
    auto_attack = new auto_attack_t( this );
  }
  virtual void init_base()
  {
    attribute_base[ ATTR_STRENGTH  ] = 153;
    attribute_base[ ATTR_AGILITY   ] = 108;
    attribute_base[ ATTR_STAMINA   ] = 280;
    attribute_base[ ATTR_INTELLECT ] = 133;

    base_attack_power = -20;
    attack_power_per_strength = 2.0;

    if( owner -> gear.tier4_2pc ) attribute_base[ ATTR_STAMINA ] += 75;
  }
  virtual void reset()
  {
    auto_attack -> reset();
  }
  virtual void schedule_ready()
  {
    assert(0);
  }
  virtual void summon()
  {
    player_t* o = cast_pet() -> owner;
    report_t::log( sim, "%s summons Shadow Fiend.", o -> name() );
    attribute_initial[ ATTR_STAMINA   ] = attribute[ ATTR_STAMINA   ] = attribute_base[ ATTR_STAMINA   ] + (int16_t) ( 0.30 * o -> attribute[ ATTR_STAMINA   ] );
    attribute_initial[ ATTR_INTELLECT ] = attribute[ ATTR_INTELLECT ] = attribute_base[ ATTR_INTELLECT ] + (int16_t) ( 0.30 * o -> attribute[ ATTR_INTELLECT ] );
    auto_attack -> execute();
  }
  virtual void dismiss()
  {
    report_t::log( sim, "%s's Shadow Fiend dies.", cast_pet() -> owner -> name() );
    if( auto_attack -> event )
    {
      auto_attack -> event -> invalid = true;
      auto_attack -> event = 0;
    }
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// stack_shadow_weaving =====================================================

static void stack_shadow_weaving( spell_t* s )
{
  priest_t* p = s -> player -> cast_priest();

  struct shadow_weaving_expiration_t : public event_t
  {
    shadow_weaving_expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Shadow Weaving Expiration";
      time = 15.01;
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
  priest_t* p = s -> player -> cast_priest();

  t -> debuffs.misery_stack++;
  if( p -> talents.misery > t -> debuffs.misery )
  {
    t -> debuffs.misery = p -> talents.misery;
  }
}

// pop_misery ==================================================================

static void pop_misery( spell_t* s )
{
  priest_t* p = s -> player -> cast_priest();
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
  priest_t* p = s -> player -> cast_priest();

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
  priest_t* p = s -> player -> cast_priest();

  if( p -> buffs.tier5_2pc )
  {
    p -> buffs.tier5_2pc = 0;
    p -> buffs.mana_cost_reduction -= 150;
  }
}

// push_tier5_4pc =============================================================

static void push_tier5_4pc( spell_t*s )
{
  priest_t* p = s -> player -> cast_priest();

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
  priest_t* p = s -> player -> cast_priest();

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
  priest_t* p = s -> player -> cast_priest();

  if( p -> talents.surge_of_light )
  {
    p -> buffs_surge_of_light = ( s -> result == RESULT_CRIT );
  }
}

// trigger_improved_spirit_tap =================================================

static void trigger_improved_spirit_tap( spell_t* s )
{
  struct expiration_t : public event_t
  {
    double spirit_bonus;
    expiration_t( sim_t* sim, priest_t* p ) : event_t( sim, p )
    {
      name = "Improved Spirit Tap Expiration";
      p -> buffs_improved_spirit_tap = 1;
      p -> aura_gain( "improved_spirit_tap" );
      spirit_bonus = p -> attribute[ ATTR_SPIRIT ] / 2;
      p -> attribute[ ATTR_SPIRIT ] += spirit_bonus;
      time = 8.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      priest_t* p = player -> cast_priest();
      p -> buffs_improved_spirit_tap = 0;
      p -> aura_loss( "improved_spirit_tap" );
      p -> attribute[ ATTR_SPIRIT ] -= spirit_bonus;
      p -> expirations_improved_spirit_tap = 0;
    }
  };

  priest_t* p = s -> player -> cast_priest();

  if( s -> result == RESULT_CRIT && 
      p -> talents.improved_spirit_tap )
  {
    if( wow_random( p -> talents.improved_spirit_tap * 0.50 ) )
    {
      p -> proc( "improved_spirit_tap" );

      event_t*& e = p -> expirations_improved_spirit_tap;

      if( e )
      {
	e -> reschedule( s -> sim -> current_time + 8.0 );
      }
      else
      {
	e = new expiration_t( s -> sim, p );
      }
    }
  }
}

} // ANONYMOUS NAMESPACE =====================================================

// ==========================================================================
// Priest Spell
// ==========================================================================

// priest_spell_t::execute ===================================================

void priest_spell_t::execute()
{
  spell_t::execute();
  priest_t* p = player -> cast_priest();
  if( p -> buffs_inner_focus && base_cost > 0 )
  {
    p -> aura_loss( "Inner Focus" );
    p -> buffs_inner_focus = 0;
  }
}

// priest_spell_t::cost ======================================================

double priest_spell_t::cost()
{
  priest_t* p = player -> cast_priest();
  if( p -> buffs_inner_focus ) return 0;
  return spell_t::cost();
}

// Holy Fire Spell ===========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "holy_fire", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> cast_priest();
    
    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 11, 890, 1130, 350, 290 },
      { 72, 10, 732,  928, 287, 290 },
      { 66,  9, 412,  523, 165, 290 },
      { 60,  8, 355,  449, 145, 255 },
      { 0,   0 }
    };
    rank = choose_rank( ranks );
      
    base_execute_time = 3.5; 
    base_duration     = 10.0; 
    num_ticks         = ( rank -> index < 10 ) ? 5 : 7;
    cooldown          = ( rank -> index < 10 ) ? 0 : 10;
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
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 12, 707, 793, 0, 615 },
      { 75, 11, 604, 676, 0, 520 },
      { 69, 10, 545, 611, 0, 385 },
      { 61,  9, 405, 455, 0, 300 },
      { 54,  8, 371, 415, 0, 280 },
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
    priest_t* p = player -> cast_priest();
    return p -> buffs_surge_of_light ? 0 : priest_spell_t::execute_time();
  }

  virtual double cost()
  {
    priest_t* p = player -> cast_priest();
    return p -> buffs_surge_of_light ? 0 : priest_spell_t::cost();
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    may_crit = ! ( p -> buffs_surge_of_light );
  }
};

// Shadow Word Pain Spell ======================================================

struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "shadow_word_pain", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 12, 0, 0, 1530, 915 },
      { 75, 11, 0, 0, 1302, 775 },
      { 70, 10, 0, 0, 1236, 575 },
      { 65,  9, 0, 0, 1002, 510 },
      { 58,  8, 0, 0,  852, 470 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
      
    static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0; 
    base_duration     = 18.0; 
    num_ticks         = 6;
    dot_power_mod     = 1.10; 

    base_dot  = rank -> dot;
    base_cost = rank -> cost;
    base_cost *= 1.0 - p -> talents.mental_agility * 0.02;
    if( AFTER_3_0 ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;

    int8_t more_ticks = 0;
    if( ! AFTER_3_0 ) more_ticks += p -> talents.improved_shadow_word_pain;
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
    if( AFTER_3_0 ) base_multiplier *= 1.0 + p -> talents.improved_shadow_word_pain * 0.05;
    base_hit        += p -> talents.shadow_focus * ( AFTER_3_0 ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result == RESULT_HIT ) 
    {
      push_misery( this );
      player -> cast_priest() -> shadow_word_pain_active = this;
    }
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
    player -> cast_priest() -> shadow_word_pain_active = 0;
  }
};

// Vampiric Touch Spell ======================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "vampiric_touch", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    assert( p -> talents.vampiric_touch );
     
    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 0, 0, 850, 700 },
      { 75, 4, 0, 0, 735, 595 },
      { 70, 3, 0, 0, 690, 475 },
      { 60, 2, 0, 0, 640, 400 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
      
    static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 1.5; 
    base_duration     = 15.0; 
    num_ticks         = 5;
    dot_power_mod     = 1.0; 

    base_cost        = rank -> cost;
    if( AFTER_3_0 ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * ( AFTER_3_0 ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result == RESULT_HIT ) 
    {
      push_misery( this );
      player -> cast_priest() -> vampiric_touch_active = this;
    }
  }

  virtual void last_tick() 
  {
    priest_spell_t::last_tick(); 
    pop_misery( this );
    player -> cast_priest() -> vampiric_touch_active = 0;
  }
};

// Devouring Plague Spell ======================================================

struct devouring_plague_t : public priest_spell_t
{
  devouring_plague_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "devouring_plague", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 0, 0, 1720, 1850 },
      { 76, 8, 0, 0, 1416, 1520 },
      { 68, 7, 0, 0, 1216, 1145 },
      { 60, 6, 0, 0,  904,  985 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
      
    static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0; 
    base_duration     = 24.0; 
    num_ticks         = 8;  
    cooldown          = 180.0;
    binary            = true;
    dot_power_mod     = ( 24.0 / 15.0 ) / 2.0; 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.mental_agility * 0.02;
    if( AFTER_3_0 ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * ( AFTER_3_0 ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result == RESULT_HIT ) 
    {
      push_misery( this );
      player -> cast_priest() -> devouring_plague_active = this;
    }
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
    player -> cast_priest() -> devouring_plague_active = 0;
  }
};

// Vampiric Embrace Spell ======================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "vampiric_embrace", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    assert( p -> talents.vampiric_embrace );
     
    static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0; 
    base_duration     = 60.0; 
    num_ticks         = 1;
    base_cost         = 30;
    base_multiplier   = 0;
    base_hit          = p -> talents.shadow_focus * ( AFTER_3_0 ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result == RESULT_HIT ) 
    {
      player -> cast_priest() -> vampiric_embrace_active = this;
    }
  }

  virtual void tick() 
  {
    priest_spell_t::tick(); 
    player -> cast_priest() -> vampiric_embrace_active = 0;
  }
};

// Mind Blast Spell ============================================================

struct mind_blast_t : public priest_spell_t
{
  mind_blast_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "mind_blast", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 13, 992, 1048, 0, 715 },
      { 75, 12, 837,  883, 0, 605 },
      { 69, 11, 708,  748, 0, 450 },
      { 63, 10, 557,  588, 0, 380 },
      { 58,  9, 503,  531, 0, 350 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 1.5; 
    cooldown          = 8.0;
    may_crit          = true; 
    dd_power_mod      = (1.5/3.5); 
      
    base_cost       = rank -> cost;
    base_cost       *= 1.0 - p -> talents.focused_mind * 0.05;
    if( AFTER_3_0 ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit       += p -> talents.shadow_power * ( AFTER_3_0 ? 0.02 : 0.03 );
    base_crit       += p -> talents.force_of_will * 0.01;
    if( AFTER_3_0 ) base_crit_bonus *= 1.0 + p -> talents.shadow_power * 0.10;
    base_hit        += p -> talents.shadow_focus * ( AFTER_3_0 ? 0.01 : 0.02 );
    base_hit        += p -> talents.focused_power * 0.02;
    cooldown        -= p -> talents.improved_mind_blast * 0.5;
    
    if( p -> gear.tier6_4pc ) base_multiplier *= 1.10;
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    if( p -> talents.twisted_faith )
    {
      if( p -> devouring_plague_active ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> shadow_word_pain_active ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> vampiric_touch_active   ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
    }
  }
};

// Shadow Word Death Spell ======================================================

struct shadow_word_death_t : public priest_spell_t
{
  shadow_word_death_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "shadow_word_death", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4, 750, 870, 0, 510 },
      { 75, 3, 639, 741, 0, 430 },
      { 70, 2, 572, 664, 0, 309 },
      { 62, 1, 450, 522, 0, 243 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0; 
    may_crit          = true; 
    cooldown          = 12.0;
    dd_power_mod      = (1.5/3.5); 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.mental_agility * 0.02;
    if( AFTER_3_0 ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit       += p -> talents.shadow_power * ( AFTER_3_0 ? 0.02 : 0.03 );
    base_crit       += p -> talents.force_of_will * 0.01;
    if( AFTER_3_0 ) base_crit_bonus *= 1.0 + p -> talents.shadow_power * 0.10;
    base_hit        += p -> talents.shadow_focus * ( AFTER_3_0 ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    priest_t* p = player -> cast_priest();
    p -> resource_loss( RESOURCE_HEALTH, dd * ( 1.0 - p -> talents.pain_and_suffering * 0.20 ), "shadow_word_death" );
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
    priest_t* p = player -> cast_priest();

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
      { 80, 9, 0, 0, 690, 390 },
      { 76, 8, 0, 0, 576, 320 },
      { 68, 7, 0, 0, 528, 230 },
      { 60, 6, 0, 0, 426, 205 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

    base_execute_time = 0.0; 
    base_duration     = 3.0; 
    num_ticks         = 3;
    channeled         = true; 
    binary            = true; 
    dot_power_mod     = 0.57;

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.focused_mind * 0.05;
    if( AFTER_3_0 ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_hit        += p -> talents.shadow_focus * ( AFTER_3_0 ? 0.01 : 0.02 );
    
    if( p -> gear.tier4_4pc ) base_multiplier *= 1.05;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    if( result == RESULT_HIT )
    {
      priest_t* p = player -> cast_priest();
      spell_t*  swp = p -> shadow_word_pain_active;
      if( swp && wow_random( p -> talents.pain_and_suffering * 0.333333 ) )
      {
	swp -> current_tick = 0;
	swp -> time_remaining = swp -> duration();
      }
    }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    if( p -> talents.twisted_faith )
    {
      if( p -> devouring_plague_active ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> shadow_word_pain_active ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> vampiric_touch_active   ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
    }
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

// Dispersion Spell ============================================================

struct dispersion_t : public priest_spell_t
{
  dispersion_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "dispersion", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();

    assert( p -> talents.dispersion );

    base_execute_time = 0.0; 
    base_duration     = 6.0; 
    num_ticks         = 6;
    channeled         = true; 
    harmful           = false;
    base_cost         = 0;
    cooldown          = 300;
  }

  virtual void tick()
  {
    player -> resource_gain( RESOURCE_MANA, 0.06 * player -> resource_initial[ RESOURCE_MANA ], "dispersion" );
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    return player -> resource_current[ RESOURCE_MANA ] < 0.50 * player -> resource_initial[ RESOURCE_MANA ];
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
      priest_t* p = player -> cast_priest();
      p -> aura_loss( "Power Infusion" );
      p -> buffs.power_infusion = 0;
      p -> haste_rating -= 320;
      p -> recalculate_haste();
    }
  };
   
  power_infusion_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "power_infusion", player, SCHOOL_ARCANE, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();
    assert( p -> talents.power_infusion );
    trigger_gcd = false;  
    cooldown = sim -> patch.after( 3, 0, 0 ) ? 120.0 : 180.0;
  }
   
  virtual void execute()
  {
    report_t::log( sim, "Player %s casts Power Infusion", player -> name() );
    priest_t* p = player -> cast_priest();
    p -> aura_gain( "Power Infusion" );
    p -> buffs.power_infusion = 1;
    p -> haste_rating += 320;
    p -> recalculate_haste();
    new power_infusion_expiration_t( sim, player );
  }
};

// Inner Focus Spell =====================================================

struct inner_focus_t : public priest_spell_t
{
  inner_focus_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "inner_focus", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();
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
    priest_t* p = player -> cast_priest();
    p -> aura_gain( "Inner Focus" );
    p -> buffs_inner_focus = 1;
    cooldown_ready = sim -> current_time + cooldown;
  }
};

// Shadow Form Spell =======================================================

struct shadow_form_t : public priest_spell_t
{
  shadow_form_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "shadow_form", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();
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

struct shadow_fiend_spell_t : public priest_spell_t
{
  struct shadow_fiend_expiration_t : public event_t
  {
    shadow_fiend_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      time = 15.1;
      if( p -> gear.tier4_2pc ) time += 3.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      player -> dismiss_pet( "shadow_fiend" );
    }
  };

  double mana;

  shadow_fiend_spell_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "summon", player, SCHOOL_SHADOW, TREE_SHADOW ), mana(0)
  {
    priest_t* p = player -> cast_priest();

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
    player -> summon_pet( "shadow_fiend" );
    player -> action_finish( this );
    new shadow_fiend_expiration_t( sim, player );
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

  trigger_improved_spirit_tap( s );
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

  uptimes_improved_spirit_tap -> update( buffs_improved_spirit_tap );
}

// priest_t::spell_damage_event ==============================================

void priest_t::spell_damage_event( spell_t* s,
				   double   amount,
				   int8_t   dmg_type )
{
  if( s -> school == SCHOOL_SHADOW )
  {
    if( vampiric_touch_active )
    {
      static bool AFTER_3_0 = sim -> patch.after( 3, 0, 0 );

      double mana = amount * ( AFTER_3_0 ? 0.02 : 0.05 );

      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> party == party )
        {
	  p -> resource_gain( RESOURCE_MANA, mana, "vampiric_touch" );
	}
      }
    }
    if( vampiric_embrace_active )
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
  if( name == "devouring_plague" ) return new devouring_plague_t  ( this, options_str );
  if( name == "dispersion"       ) return new dispersion_t        ( this, options_str );
  if( name == "holy_fire"        ) return new holy_fire_t         ( this, options_str );
  if( name == "inner_focus"      ) return new inner_focus_t       ( this, options_str );
  if( name == "mind_blast"       ) return new mind_blast_t        ( this, options_str );
  if( name == "mind_flay"        ) return new mind_flay_t         ( this, options_str );
  if( name == "power_infusion"   ) return new power_infusion_t    ( this, options_str );
  if( name == "shadow_word_death") return new shadow_word_death_t ( this, options_str );
  if( name == "shadow_word_pain" ) return new shadow_word_pain_t  ( this, options_str );
  if( name == "shadow_form"      ) return new shadow_form_t       ( this, options_str );
  if( name == "smite"            ) return new smite_t             ( this, options_str );
  if( name == "shadow_fiend"     ) return new shadow_fiend_spell_t( this, options_str );
  if( name == "vampiric_embrace" ) return new vampiric_embrace_t  ( this, options_str );
  if( name == "vampiric_touch"   ) return new vampiric_touch_t    ( this, options_str );

  return 0;
}

// priest_t::create_pet ======================================================

pet_t* priest_t::create_pet( const std::string& pet_name )
{
  if( pet_name == "shadow_fiend" ) return new shadow_fiend_pet_t( sim, this, pet_name );

  return 0;
}

// priest_t::init_base =======================================================

void priest_t::init_base()
{
  static bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

  attribute_base[ ATTR_STRENGTH  ] =  40;
  attribute_base[ ATTR_AGILITY   ] =  45;
  attribute_base[ ATTR_STAMINA   ] =  60;
  attribute_base[ ATTR_INTELLECT ] = 145;
  attribute_base[ ATTR_SPIRIT    ] = 155;

  attribute_multiplier[ ATTR_STAMINA   ] *= 1.0 + talents.enlightenment * 0.01;
  attribute_multiplier[ ATTR_INTELLECT ] *= 1.0 + talents.enlightenment * 0.01;
  attribute_multiplier[ ATTR_SPIRIT    ] *= 1.0 + talents.enlightenment * 0.01;

  base_spell_crit = 0.0125;
  spell_crit_per_intellect = 0.01 / ( level + 10 );
  spell_power_per_spirit = ( talents.spiritual_guidance * 0.05 +
			     talents.twisted_faith      * 0.06 );

  if( AFTER_3_0_0 ) spell_power_multiplier = 1.0 + talents.enlightenment * 0.01;

  base_attack_power = -10;
  base_attack_crit  = 0.03;
  attack_power_per_strength = 1.0;
  attack_crit_per_agility = 1.0 / ( 25 + ( level - 70 ) * 0.5 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = 2340;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  if( AFTER_3_0_0 )
  {
    attribute_multiplier[ ATTR_INTELLECT ] *= 1.0 + talents.mental_strength * 0.01;
  }
  else
  {
    mana_per_intellect *= 1.0 + talents.mental_strength * 0.02;
  }
}

// priest_t::reset ===========================================================

void priest_t::reset()
{
  player_t::reset();

  devouring_plague_active = 0;
  shadow_word_pain_active = 0;
  vampiric_touch_active   = 0;
  vampiric_embrace_active = 0;

  expirations_improved_spirit_tap = 0;
  buffs_improved_spirit_tap = 0;
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
    { 21,  &( talents.enlightenment             ) },
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
    { "dispersion",                OPT_INT8,  &( talents.dispersion                ) },
    { "divine_fury",               OPT_INT8,  &( talents.divine_fury               ) },
    { "enlightenment",             OPT_INT8,  &( talents.enlightenment             ) },
    { "focused_mind",              OPT_INT8,  &( talents.focused_mind              ) },
    { "focused_power",             OPT_INT8,  &( talents.focused_power             ) },
    { "force_of_will",             OPT_INT8,  &( talents.force_of_will             ) },
    { "holy_specialization",       OPT_INT8,  &( talents.holy_specialization       ) },
    { "improved_divine_spirit",    OPT_INT8,  &( talents.improved_divine_spirit    ) },
    { "improved_shadow_word_pain", OPT_INT8,  &( talents.improved_shadow_word_pain ) },
    { "improved_mind_blast",       OPT_INT8,  &( talents.improved_mind_blast       ) },
    { "improved_spirit_tap",       OPT_INT8,  &( talents.improved_spirit_tap       ) },
    { "improved_vampiric_embrace", OPT_INT8,  &( talents.improved_vampiric_embrace ) },
    { "inner_focus",               OPT_INT8,  &( talents.inner_focus               ) },
    { "meditation",                OPT_INT8,  &( talents.meditation                ) },
    { "mental_agility",            OPT_INT8,  &( talents.mental_agility            ) },
    { "mental_strength",           OPT_INT8,  &( talents.mental_strength           ) },
    { "misery",                    OPT_INT8,  &( talents.misery                    ) },
    { "pain_and_suffering",        OPT_INT8,  &( talents.pain_and_suffering        ) },
    { "power_infusion",            OPT_INT8,  &( talents.power_infusion            ) },
    { "searing_light",             OPT_INT8,  &( talents.searing_light             ) },
    { "shadow_focus",              OPT_INT8,  &( talents.shadow_focus              ) },
    { "shadow_form",               OPT_INT8,  &( talents.shadow_form               ) },
    { "shadow_power",              OPT_INT8,  &( talents.shadow_power              ) },
    { "shadow_weaving",            OPT_INT8,  &( talents.shadow_weaving            ) },
    { "spiritual_guidance",        OPT_INT8,  &( talents.spiritual_guidance        ) },
    { "surge_of_light",            OPT_INT8,  &( talents.surge_of_light            ) },
    { "twisted_faith",             OPT_INT8,  &( talents.twisted_faith             ) },
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

player_t* player_t::create_priest( sim_t*       sim, 
				   std::string& name ) 
{
  return new priest_t( sim, name );
}


