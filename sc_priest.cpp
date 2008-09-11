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
  spell_t* active_devouring_plague;
  spell_t* active_shadow_word_pain;
  spell_t* active_vampiric_touch;
  spell_t* active_vampiric_embrace;
  spell_t* active_mind_blast;
  spell_t* active_shadow_word_death;

  // Buffs
  int8_t buffs_inner_fire;
  int8_t buffs_improved_spirit_tap;
  int8_t buffs_shadow_weaving;
  int8_t buffs_surge_of_light;

  // Expirations
  event_t* expirations_improved_spirit_tap;
  event_t* expirations_shadow_weaving;

  // Gains
  gain_t* gains_shadow_fiend;
  gain_t* gains_dispersion;

  // Up-Times
  uptime_t* uptimes_improved_spirit_tap;

  struct talents_t
  {
    int8_t  aspiration;
    int8_t  creeping_shadows;
    int8_t  darkness;
    int8_t  dispersion;
    int8_t  divine_fury;
    int8_t  divine_spirit;
    int8_t  enlightenment;
    int8_t  focused_mind;
    int8_t  focused_power;
    int8_t  force_of_will;
    int8_t  holy_specialization;
    int8_t  improved_divine_spirit;
    int8_t  improved_inner_fire;
    int8_t  improved_mind_blast;
    int8_t  improved_mind_flay;
    int8_t  improved_shadow_word_pain;
    int8_t  improved_spirit_tap;
    int8_t  improved_vampiric_embrace;
    int8_t  inner_focus;
    int8_t  meditation;
    int8_t  mental_agility;
    int8_t  mental_strength;
    int8_t  mind_flay;
    int8_t  mind_melt;
    int8_t  misery;
    int8_t  pain_and_suffering;
    int8_t  penance;
    int8_t  power_infusion;
    int8_t  searing_light;
    int8_t  shadow_affinity;
    int8_t  shadow_focus;
    int8_t  shadow_form;
    int8_t  shadow_power;
    int8_t  shadow_weaving;
    int8_t  spirit_of_redemption;
    int8_t  spiritual_guidance;
    int8_t  surge_of_light;
    int8_t  twin_disciplines;
    int8_t  twisted_faith;
    int8_t  vampiric_embrace;
    int8_t  vampiric_touch;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int8_t blue_promises;
    int8_t shadow_word_death;
    int8_t shadow_word_pain;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  priest_t( sim_t* sim, std::string& name ) : player_t( sim, PRIEST, name ) 
  {
    // Active
    active_devouring_plague  = 0;
    active_shadow_word_pain  = 0;
    active_vampiric_touch    = 0;
    active_vampiric_embrace  = 0;
    active_mind_blast        = 0;
    active_shadow_word_death = 0;

    // Buffs
    buffs_improved_spirit_tap = 0;
    buffs_inner_fire          = 0;
    buffs_shadow_weaving      = 0;
    buffs_surge_of_light      = 0;

    // Expirations
    expirations_improved_spirit_tap = 0;
    expirations_shadow_weaving      = 0;

    // Gains
    gains_dispersion   = get_gain( "dispersion" );
    gains_shadow_fiend = get_gain( "shadow_fiend" );

    // Up-Times
    uptimes_improved_spirit_tap = get_uptime( "improved_spirit_tap" );
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }

  // Event Tracking
  virtual void regen( double periodicity );
  virtual void spell_hit_event   ( spell_t* );
  virtual void spell_finish_event( spell_t* );
  virtual void spell_damage_event( spell_t*, double amount, int8_t dmg_type );
};

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public spell_t
{
  priest_spell_t( const char* n, player_t* player, int8_t s, int8_t t ) : 
    spell_t( n, player, RESOURCE_MANA, s, t ) 
  {
    priest_t* p = player -> cast_priest();
    base_multiplier *= 1.0 + p -> talents.force_of_will * 0.01;
    base_crit       += p -> talents.force_of_will * 0.01;
  }

  // Overridden Methods
  virtual double haste();
  virtual void   player_buff();

  // Passthru Methods
  virtual double cost()         { return spell_t::cost();         }
  virtual double execute_time() { return spell_t::execute_time(); }
  virtual void   execute()      { spell_t::execute();             }
  virtual void   tick()         { spell_t::tick();                }
  virtual void   last_tick()    { spell_t::last_tick();           }
  virtual bool   ready()        { return spell_t::ready();        }
};

// ==========================================================================
// Pet Shadow Fiend
// ==========================================================================

struct shadow_fiend_pet_t : public pet_t
{
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) : 
      attack_t( "melee", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd = 1;
      background = true;
      repeating = true;
    }
    virtual void execute()
    {
      attack_t::execute();
      if( result_is_hit() )
      {
	if( ! sim_t::WotLK )
	{
	  enchant_t::trigger_flametongue_totem( this );
	  enchant_t::trigger_windfury_totem( this );
	}
      }
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
      priest_t* p = player -> cast_pet() -> owner -> cast_priest();
      p -> resource_gain( RESOURCE_MANA, amount * 2.5, p -> gains_shadow_fiend );
    }
  };

  melee_t* melee;

  shadow_fiend_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 110;
    main_hand_weapon.swing_time = 1.5;
    main_hand_weapon.school     = SCHOOL_SHADOW;

    melee = new melee_t( this );
  }
  virtual void init_base()
  {
    attribute_base[ ATTR_STRENGTH  ] = 153;
    attribute_base[ ATTR_AGILITY   ] = 108;
    attribute_base[ ATTR_STAMINA   ] = 280;
    attribute_base[ ATTR_INTELLECT ] = 133;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    if( owner -> gear.tier4_2pc ) attribute_base[ ATTR_STAMINA ] += 75;
  }
  virtual void reset()
  {
    player_t::reset();
  }
  virtual void schedule_ready()
  {
    assert(0);
  }
  virtual void summon()
  {
    player_t* o = cast_pet() -> owner;
    if( sim -> log ) report_t::log( sim, "%s summons Shadow Fiend.", o -> name() );
    attribute_initial[ ATTR_STAMINA   ] = attribute[ ATTR_STAMINA   ] = attribute_base[ ATTR_STAMINA   ] + ( 0.30 * o -> attribute[ ATTR_STAMINA   ] );
    attribute_initial[ ATTR_INTELLECT ] = attribute[ ATTR_INTELLECT ] = attribute_base[ ATTR_INTELLECT ] + ( 0.30 * o -> attribute[ ATTR_INTELLECT ] );
    // Kick-off repeating attack
    melee -> execute();
  }
  virtual void dismiss()
  {
    if( sim -> log ) report_t::log( sim, "%s's Shadow Fiend dies.", cast_pet() -> owner -> name() );
    melee -> cancel();
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// stack_shadow_weaving =====================================================

static void stack_shadow_weaving( spell_t* s )
{
  priest_t* p = s -> player -> cast_priest();

  if( p -> talents.shadow_weaving == 0 ) return;

  struct shadow_weaving_expiration_t : public event_t
  {
    shadow_weaving_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Shadow Weaving Expiration";
      sim -> add_event( this, 15.01 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "%s loses Shadow Weaving", sim -> target -> name() );
      if( sim_t::WotLK )
      {
	priest_t* p = player -> cast_priest();
	p -> buffs_shadow_weaving = 0;
	p -> expirations_shadow_weaving = 0;
      }
      else
      {
	sim -> target -> debuffs.shadow_weaving = 0;
	sim -> target -> expirations.shadow_weaving = 0;
      }
    }
  };

  if( rand_t::roll( p -> talents.shadow_weaving * ( sim_t::WotLK ? (1.0/3) : 0.2 ) ) )
  {
    sim_t* sim = s -> sim;
    target_t* t = sim -> target;
    int8_t& stack = sim_t::WotLK ? p -> buffs_shadow_weaving : t -> debuffs.shadow_weaving;

    if( stack < 5 ) 
    {
      stack++;
      if( sim -> log ) report_t::log( sim, "%s gains Shadow Weaving %d", sim_t::WotLK ? p -> name() : t -> name(), stack );
    }

    event_t*& e = sim_t::WotLK ? p -> expirations_shadow_weaving : t -> expirations.shadow_weaving;
    
    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new shadow_weaving_expiration_t( s -> sim, p );
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
    t -> debuffs.misery = std::min( p -> talents.misery, (int8_t) 3 );
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

  if( p -> gear.tier5_2pc && rand_t::roll( 0.06 ) )
  {
    p -> buffs.tier5_2pc = 1;
    p -> buffs.mana_cost_reduction += 150;
    p -> procs.tier5_2pc -> occur();
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
	rand_t::roll( 0.40 ) )
  {
    p -> buffs.tier5_4pc = 1;
    p -> spell_power[ SCHOOL_MAX ] += 100;
    p -> procs.tier5_4pc -> occur();
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
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Ashtongue Talisman Expiration";
      player -> aura_gain( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] += 220;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] -= 220;
      player -> expirations.ashtongue_talisman = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.ashtongue_talisman && rand_t::roll( 0.10 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new expiration_t( s -> sim, p );
    }
  }
}

// trigger_surge_of_light =====================================================

static void trigger_surge_of_light( spell_t* s )
{
  priest_t* p = s -> player -> cast_priest();

  if( p -> talents.surge_of_light )
    if( rand_t::roll( p -> talents.surge_of_light * 0.25 ) )
      p -> buffs_surge_of_light = 1;
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
      spirit_bonus = p -> attribute[ ATTR_SPIRIT ] * p -> talents.improved_spirit_tap * 0.05;
      p -> attribute[ ATTR_SPIRIT ] += spirit_bonus;
      sim -> add_event( this, 8.0 );
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

  if( p -> talents.improved_spirit_tap )
  {
    event_t*& e = p -> expirations_improved_spirit_tap;

    if( e )
    {
      e -> reschedule( 8.0 );
    }
    else
    {
      e = new expiration_t( s -> sim, p );
    }
  }
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Priest Spell
// ==========================================================================

// priest_spell_t::haste ====================================================

double priest_spell_t::haste()
{
  priest_t* p = player -> cast_priest();
  double h = spell_t::haste();
  if( sim_t::WotLK && p -> talents.enlightenment ) 
  {
    h *= 1.0 / ( 1.0 + p -> talents.enlightenment * 0.01 );
  }
  return h;
}

// priest_spell_t::player_buff ==============================================

void priest_spell_t::player_buff()
{
  priest_t* p = player -> cast_priest();
  spell_t::player_buff();
  if( school == SCHOOL_SHADOW )
  {
    if( p -> buffs_shadow_weaving )
    {
      player_multiplier *= 1.0 + p -> buffs_shadow_weaving * 0.02;
    }
  }
  if( p -> talents.focused_power )
  {
    player_multiplier *= 1.0 + p -> talents.focused_power * 0.02;
  }
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
      { 78, 11, 890, 1130, 350, 0.11 },
      { 72, 10, 732,  928, 287, 0.11 },
      { 66,  9, 412,  523, 165, 290  },
      { 60,  8, 355,  449, 145, 255  },
      { 0,   0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    if( rank -> index < 10 )
    {
      base_execute_time = 3.5;
      base_duration     = 10; 
      num_ticks         = 5;
      cooldown          = 0;
      dd_power_mod      = 0.857;
      dot_power_mod     = 0.165;
    }
    else
    {
      base_execute_time = 2.0; 
      base_duration     = 7; 
      num_ticks         = 7;
      cooldown          = 10;
      dd_power_mod      = 0.314;
      dot_power_mod     = 0.210;
    }
    may_crit           = true;    
    base_cost          = rank -> cost;
    base_execute_time -= p -> talents.divine_fury * 0.01;
    base_multiplier   *= 1.0 + p -> talents.searing_light * 0.05;
    base_crit         += p -> talents.holy_specialization * 0.01;
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
      { 80, 12, 707, 793, 0, 0.15 },
      { 75, 11, 604, 676, 0, 0.15 },
      { 69, 10, 545, 611, 0, 385  },
      { 61,  9, 405, 455, 0, 300  },
      { 54,  8, 371, 415, 0, 280  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 2.5; 
    dd_power_mod      = (2.5/3.5); 
    may_crit          = true;
      
    base_cost          = rank -> cost;
    base_execute_time -= p -> talents.divine_fury * 0.1;
    base_multiplier   *= 1.0 + p -> talents.searing_light * 0.05;
    base_crit         += p -> talents.holy_specialization * 0.01;
    
    if( p -> gear.tier4_4pc ) base_multiplier *= 1.05;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    player -> cast_priest() -> buffs_surge_of_light = 0;
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

// Pennnce Spell ===============================================================

struct penance_t : public priest_spell_t
{
  penance_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "penance", player, SCHOOL_HOLY, TREE_HOLY )
  {
    priest_t* p = player -> cast_priest();
    assert( p -> talents.penance );

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4, 437, 437, 0, 0.33 },
      { 76, 3, 355, 355, 0, 0.33 },
      { 68, 2, 325, 325, 0, 0.33 },
      { 60, 1, 263, 263, 0, 0.33 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0.0; 
    base_duration     = 3.0;
    num_ticks         = 3;
    channeled         = true;
    may_crit          = true;
    cooldown          = 10;
    dd_power_mod     = (3.0/3.5)/num_ticks;
      
    base_cost          = rank -> cost;
    cooldown          *= 1.0 - p -> talents.aspiration * 0.10;
    base_multiplier   *= 1.0 + p -> talents.searing_light * 0.05;
    base_crit         += p -> talents.holy_specialization * 0.01;
  }

  // Odd things to handle:
  // (1) Execute is guaranteed.
  // (2) Each "tick" is like an "execute" and can "crit".
  // (3) On each tick hit/miss events are triggered.

  virtual void execute() 
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    dd = 0;
    update_stats( DMG_DIRECT );
    player -> action_finish( this );
  }

  virtual void tick() 
  {
    if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    may_resist = false;
    target_debuff( DMG_DIRECT );
    calculate_result();
    may_resist = true;
    if( result_is_hit() )
    {
      calculate_damage();
      adjust_dd_for_result();
      player -> action_hit( this );
      if( dd > 0 )
      {
	dot_tick = dd;
	assess_damage( dot_tick, DMG_OVER_TIME );
      }
    }
    else
    {
      if( sim -> log ) report_t::log( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
      player -> action_miss( this );
    }
    update_stats( DMG_OVER_TIME );
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
      { 80, 12, 0, 0, 1530, 0.22 },
      { 75, 11, 0, 0, 1302, 0.22 },
      { 70, 10, 0, 0, 1236, 575  },
      { 65,  9, 0, 0, 1002, 510  },
      { 58,  8, 0, 0,  852, 470  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    base_execute_time = 0; 
    base_duration     = 18.0; 
    num_ticks         = 6;
    dot_power_mod     = 1.10; 

    base_dot  = rank -> dot;
    base_cost = rank -> cost;
    base_cost *= 1.0 - p -> talents.mental_agility * 0.02;
    if( sim_t::WotLK ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    if( p -> glyphs.shadow_word_pain ) base_cost *= 0.80;

    int8_t more_ticks = 0;
    if( ! sim_t::WotLK ) more_ticks += p -> talents.improved_shadow_word_pain;
    if( p -> gear.tier6_2pc ) more_ticks++;
    if( more_ticks > 0 )
    {
      double adjust = ( num_ticks + more_ticks ) / (double) num_ticks;

      base_dot      *= adjust;
      dot_power_mod *= adjust;
      base_duration *= adjust;
      num_ticks     += more_ticks;
    }

    if( sim_t::WotLK )
    {
      base_multiplier *= 1.0 + ( p -> talents.darkness                  * 0.02 + 
				 p -> talents.twin_disciplines          * 0.01 +
				 p -> talents.improved_shadow_word_pain * 0.05 );
    }
    else
    {
      base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    }
    base_hit += p -> talents.shadow_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result_is_hit() ) 
    {
      push_misery( this );
      player -> cast_priest() -> active_shadow_word_pain = this;
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
    player -> cast_priest() -> active_shadow_word_pain = 0;
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
      { 80, 5, 0, 0, 850, 0.16 },
      { 75, 4, 0, 0, 735, 0.16 },
      { 70, 3, 0, 0, 690, 475  },
      { 60, 2, 0, 0, 640, 400  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time = 1.5; 
    base_duration     = 15.0; 
    num_ticks         = 5;
    dot_power_mod     = 1.0; 

    base_cost        = rank -> cost;
    if( sim_t::WotLK ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_hit        += p -> talents.shadow_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result_is_hit() ) 
    {
      push_misery( this );
      player -> cast_priest() -> active_vampiric_touch = this;
      if( sim_t::WotLK )
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
        {
	  p -> buffs.replenishment++;
	}
      }
    }
  }

  virtual void last_tick() 
  {
    priest_spell_t::last_tick(); 
    pop_misery( this );
    player -> cast_priest() -> active_vampiric_touch = 0;
    if( sim_t::WotLK )
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	p -> buffs.replenishment--;
      }
    }
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
      { 80, 9, 0, 0, 1720, 0.44 },
      { 76, 8, 0, 0, 1416, 0.44 },
      { 68, 7, 0, 0, 1216, 1145 },
      { 60, 6, 0, 0,  904,  985 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time = 0; 
    base_duration     = 24.0; 
    num_ticks         = 8;  
    cooldown          = 180.0;
    binary            = true;
    dot_power_mod     = ( 24.0 / 15.0 ) / 2.0; 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.mental_agility * 0.02;
    if( sim_t::WotLK ) base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.twin_disciplines * 0.01;
    base_hit        += p -> talents.shadow_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result_is_hit() ) 
    {
      push_misery( this );
      player -> cast_priest() -> active_devouring_plague = this;
    }
  }

  virtual void tick() 
  {
    priest_spell_t::tick(); 
    player -> resource_gain( RESOURCE_HEALTH, dot_tick );
  }

  virtual void last_tick() 
  {
    priest_spell_t::last_tick(); 
    pop_misery( this );
    player -> cast_priest() -> active_devouring_plague = 0;
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
     
    base_execute_time = 0; 
    base_duration     = 60.0; 
    num_ticks         = 1;
    base_cost         = 30;
    base_multiplier   = 0;
    base_hit          = p -> talents.shadow_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    if( result_is_hit() ) 
    {
      player -> cast_priest() -> active_vampiric_embrace = this;
    }
  }

  virtual void tick() 
  {
    priest_spell_t::tick(); 
    player -> cast_priest() -> active_vampiric_embrace = 0;
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
      { 80, 13, 992, 1048, 0, 0.17 },
      { 75, 12, 837,  883, 0, 0.17 },
      { 69, 11, 708,  748, 0, 450  },
      { 63, 10, 557,  588, 0, 380  },
      { 58,  9, 503,  531, 0, 350  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 1.5; 
    cooldown          = 8.0;
    may_crit          = true; 
    dd_power_mod      = (1.5/3.5); 
      
    base_cost       = rank -> cost;
    base_cost       *= 1.0 - p -> talents.focused_mind * 0.05;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_crit       += p -> talents.shadow_power * ( sim_t::WotLK ? 0.02 : 0.03 );
    base_hit        += p -> talents.shadow_focus * ( sim_t::WotLK ? 0.01 : 0.02 );
    cooldown        -= p -> talents.improved_mind_blast * 0.5;

    if( sim_t::WotLK )
    {
      base_cost *= 1.0 - p -> talents.shadow_focus * 0.02;

      if( p -> glyphs.blue_promises )
      {
	base_crit       += p -> talents.mind_melt * 0.02;
	base_crit_bonus *= 1.0 + p -> talents.shadow_power * 0.20;
	dd_power_mod    *= 1.0 + p -> talents.misery * 0.05;
      }
      else
      {
	base_crit_bonus *= 1.0 + p -> talents.shadow_power * 0.10;
      }
    }
    
    if( p -> gear.tier6_4pc ) base_multiplier *= 1.10;

    assert( p -> active_mind_blast == 0 );
    p -> active_mind_blast = this;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    if( result == RESULT_CRIT )
    {
      trigger_improved_spirit_tap( this );
    }
  }
  
  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    if( p -> talents.twisted_faith )
    {
      if( p -> active_devouring_plague ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> active_shadow_word_pain ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> active_vampiric_touch   ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
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
      { 80, 4, 750, 870, 0, 0.22 },
      { 75, 3, 639, 741, 0, 0.22 },
      { 70, 2, 572, 664, 0, 309  },
      { 62, 1, 450, 522, 0, 243  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    may_crit          = true; 
    cooldown          = 12.0;
    dd_power_mod      = (1.5/3.5); 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.mental_agility * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_multiplier *= 1.0 + p -> talents.twin_disciplines * 0.01;
    base_crit       += p -> talents.shadow_power * ( sim_t::WotLK ? 0.02 : 0.03 );
    base_hit        += p -> talents.shadow_focus * ( sim_t::WotLK ? 0.01 : 0.02 );

    if( sim_t::WotLK )
    {
      base_cost       *= 1.0 - p -> talents.shadow_focus * 0.02;
      base_crit_bonus *= 1.0 + p -> talents.shadow_power * ( p -> glyphs.blue_promises ? 0.20 : 0.10 );
    }

    assert( p -> active_shadow_word_death == 0 );
    p -> active_shadow_word_death = this;
  }

  virtual void execute() 
  {
    priest_spell_t::execute(); 
    priest_t* p = player -> cast_priest();
    p -> resource_loss( RESOURCE_HEALTH, dd * ( 1.0 - p -> talents.pain_and_suffering * 0.20 ) );
    if( result == RESULT_CRIT )
    {
      trigger_improved_spirit_tap( this );
    }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();
    priest_t* p = player -> cast_priest();
    if( p -> glyphs.shadow_word_death )
    {
      target_t* t = sim -> target;
      if( t -> initial_health > 0 && ( t -> current_health / t -> initial_health ) < 0.35 )
      {
	player_multiplier *= 1.05;
      }
    }
  }
};

// Mind Flay Spell ============================================================

struct mind_flay_bc_t : public priest_spell_t
{
  mind_flay_bc_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "mind_flay", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    priest_t* p = player -> cast_priest();
    assert( p -> talents.mind_flay );

    option_t options[] =
    {
      { "rank",   OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 0, 0, 690, 0.09 },
      { 76, 8, 0, 0, 576, 0.09 },
      { 68, 7, 0, 0, 528, 230  },
      { 60, 6, 0, 0, 426, 205  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0.0; 
    base_duration     = 3.0; 
    num_ticks         = 3;
    channeled         = true; 
    binary            = true;
    may_crit          = false;
    dot_power_mod     = 0.57;

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.focused_mind * 0.05;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_hit        += p -> talents.shadow_focus * 0.02;
    base_duration   -= p -> talents.improved_mind_flay * 0.1;
    
    if( p -> gear.tier4_4pc ) base_multiplier *= 1.05;
  }

  // Odd thing to handle: WotLK has behaviour similar to Arcane Missiles

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::execute();
    if( result_is_hit() )
    {
      spell_t* swp = p -> active_shadow_word_pain;
      if( swp && rand_t::roll( p -> talents.pain_and_suffering * 0.333333 ) )
      {
	swp -> current_tick = 0;
	swp -> time_remaining = swp -> duration();
	swp -> update_ready();
      }
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::player_buff();
    if( p -> talents.twisted_faith )
    {
      if( p -> active_devouring_plague ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> active_shadow_word_pain ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> active_vampiric_touch   ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
    }
  }

  virtual void tick()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::tick();
    if( result_is_hit() )
    {
      if( p -> talents.creeping_shadows )
      {
	if( p -> active_mind_blast )
        {
	  p -> active_mind_blast -> cooldown_ready -= 0.5 * p -> talents.creeping_shadows;
	}
	if( p -> active_shadow_word_death )
        {
	  p -> active_shadow_word_death -> cooldown_ready -= 0.5 * p -> talents.creeping_shadows;
	}
      }
    }
  }
};

struct mind_flay_wotlk_t : public priest_spell_t
{
  int8_t swp_refresh;

  mind_flay_wotlk_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "mind_flay", player, SCHOOL_SHADOW, TREE_SHADOW ), swp_refresh(0)
  {
    priest_t* p = player -> cast_priest();
    assert( p -> talents.mind_flay );

    option_t options[] =
    {
      { "rank",        OPT_INT8, &rank_index  },
      { "swp_refresh", OPT_INT8, &swp_refresh },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 230, 230, 0, 0.09 },
      { 76, 8, 192, 192, 0, 0.09 },
      { 68, 7, 176, 176, 0, 230  },
      { 60, 6, 142, 142, 0, 205  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0.0; 
    base_duration     = 3.0; 
    num_ticks         = 3;
    channeled         = true; 
    binary            = false;
    may_crit          = true;
    dd_power_mod      = ( (3.0/3.5) * 0.95 ) / 3.0;

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.focused_mind * 0.05;
    base_cost       *= 1.0 - p -> talents.shadow_focus * 0.02;
    base_multiplier *= 1.0 + p -> talents.darkness * 0.02;
    base_hit        += p -> talents.shadow_focus * 0.01;

    if( p -> glyphs.blue_promises )
    {
      base_multiplier *= 1.0 + p -> talents.twin_disciplines * 0.01;
      base_crit       += p -> talents.mind_melt * 0.02;
      base_crit_bonus *= 1.0 + p -> talents.shadow_power * 0.20;
      dd_power_mod    *= 1.0 + p -> talents.misery * 0.05;
    }
    
    if( p -> gear.tier4_4pc ) base_multiplier *= 1.05;
  }

  // Odd thing to handle: WotLK has behaviour similar to Arcane Missiles

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    player_buff();
    schedule_tick();
    update_ready();
    dd = 0;
    update_stats( DMG_DIRECT );
    p -> action_finish( this );
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();
    priest_spell_t::player_buff();
    if( p -> talents.twisted_faith )
    {
      if( p -> active_devouring_plague ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> active_shadow_word_pain ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
      if( p -> active_vampiric_touch   ) player_multiplier *= 1.0 + p -> talents.twisted_faith * 0.01;
    }
  }

  virtual void tick()
  {
    priest_t* p = player -> cast_priest();
    
    if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    may_resist = false;
    target_debuff( DMG_DIRECT );
    calculate_result();
    may_resist = true;

    if( result_is_hit() )
    {
      calculate_damage();
      adjust_dd_for_result();
      p -> action_tick( this ); // FIXME! Should this be hit or tick?
      if( dd > 0 )
      {
	dot_tick = dd;
	assess_damage( dot_tick, DMG_OVER_TIME );
      }
    }
    else
    {
      if( sim -> log ) report_t::log( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
      p -> action_miss( this );
    }

    update_stats( DMG_OVER_TIME );

    if( result_is_hit() )
    {
      spell_t* swp = p -> active_shadow_word_pain;
      if( swp && rand_t::roll( p -> talents.pain_and_suffering * 0.333333 ) )
      {
	swp -> current_tick = 0;
	swp -> time_remaining = swp -> duration();
	swp -> update_ready();
      }
      if( p -> talents.creeping_shadows )
      {
	if( p -> active_mind_blast )
        {
	  p -> active_mind_blast -> cooldown_ready -= 0.5 * p -> talents.creeping_shadows;
	}
	if( p -> active_shadow_word_death )
        {
	  p -> active_shadow_word_death -> cooldown_ready -= 0.5 * p -> talents.creeping_shadows;
	}
      }
    }
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();
    
    if( ! priest_spell_t::ready() )
      return false;

    if( swp_refresh )
    {
      if( ! p -> active_shadow_word_pain )
	return false;

      if( p -> active_shadow_word_pain -> time_remaining > 3.0 )
	return false;
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
    cooldown          = 180;
  }

  virtual void tick()
  {
    priest_t* p = player -> cast_priest();
    p -> resource_gain( RESOURCE_MANA, 0.06 * p -> resource_max[ RESOURCE_MANA ], p -> gains_dispersion );
  }

  virtual bool ready()
  {
    if( ! priest_spell_t::ready() )
      return false;

    return player -> resource_current[ RESOURCE_MANA ] < 0.50 * player -> resource_max[ RESOURCE_MANA ];
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Power Infusion Expiration";
      player -> aura_gain( "Power Infusion" );
      player -> buffs.power_infusion = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Power Infusion" );
      player -> buffs.power_infusion = 0;
    }
  };
   
  power_infusion_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "power_infusion", player, SCHOOL_ARCANE, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();
    assert( p -> talents.power_infusion );
    trigger_gcd = 0;  
    cooldown = sim -> patch.after( 3, 0, 0 ) ? 120.0 : 180.0;
    cooldown *= 1.0 - p -> talents.aspiration * 0.10;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new expiration_t( sim, player );
  }
   
  virtual bool ready()
  {
    if( ! priest_spell_t::ready() )
      return false;

    if( player -> buffs.bloodlust )
      return false;

    return true;
  }
};

// Inner Focus Spell =====================================================

struct inner_focus_t : public priest_spell_t
{
  action_t* free_action;

  inner_focus_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "inner_focus", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    priest_t* p = player -> cast_priest();
    assert( p -> talents.inner_focus );

    cooldown = 180.0;
    cooldown *= 1.0 - p -> talents.aspiration * 0.10;

    std::string spell_name    = options_str;
    std::string spell_options = "";

    std::string::size_type cut_pt = spell_name.find_first_of( "," );       

    if( cut_pt != spell_name.npos )
    {
      spell_options = spell_name.substr( cut_pt + 1 );
      spell_name    = spell_name.substr( 0, cut_pt );
    }

    free_action = p -> create_action( spell_name.c_str(), spell_options.c_str() );
    free_action -> base_cost = 0;
    free_action -> background = true;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    priest_t* p = player -> cast_priest();
    p -> aura_gain( "Inner Focus" );
    free_action -> stats -> adjust_for_lost_time = true;
    free_action -> execute();
    p -> aura_loss( "Inner Focus" );
    update_ready();
  }

  virtual bool ready()
  {
    return( priest_spell_t::ready() && free_action -> ready() );
  }
};

// Divine Spirit Spell =====================================================

struct divine_spirit_t : public priest_spell_t
{
  int8_t improved;

  divine_spirit_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "divine_spirit", player, SCHOOL_HOLY, TREE_DISCIPLINE ), improved(0)
  {
    priest_t* p = player -> cast_priest();
    assert( p -> talents.divine_spirit );
    improved = p -> talents.improved_divine_spirit;
    trigger_gcd = 0;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    double bonus_power = sim -> patch.after( 3, 0, 0 ) ? 0.03 : 0.05;

    double bonus_spirit = ( player -> level <= 69 ) ? 40 :
                          ( player -> level <= 79 ) ? 50 : 80;

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> buffs.divine_spirit == 0 )
      {
	p -> buffs.divine_spirit = 1;
	p -> attribute[ ATTR_SPIRIT ] += bonus_spirit;
      }
      if( p -> buffs.improved_divine_spirit == 0 )
      {
	if( improved > 0 )
	{
	  p -> buffs.improved_divine_spirit = improved;
	  p -> spell_power_per_spirit += improved * bonus_power;
	}
      }
    }
  }

  virtual bool ready()
  {
    if( improved ) return ! player -> buffs.improved_divine_spirit;
    return ! player -> buffs.divine_spirit;
  }
};

// Inner Fire Spell ======================================================

struct inner_fire_t : public priest_spell_t
{
  inner_fire_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "inner_fire", player, SCHOOL_HOLY, TREE_DISCIPLINE )
  {
    trigger_gcd = 0;
  }
   
  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );

    double bonus_power = ( p -> level >= 77 ) ? 120 : 
                         ( p -> level >= 71 ) ?  95 : 0;

    bonus_power *= 1.0 + p -> talents.improved_inner_fire * 0.10;

    p -> spell_power[ SCHOOL_MAX ] += bonus_power;
    p -> buffs_inner_fire = 1;
  }

  virtual bool ready()
  {
    return ! player -> cast_priest() -> buffs_inner_fire;
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
    trigger_gcd = 0;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
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
      double duration = 15.1;
      if( p -> gear.tier4_2pc ) duration += 3.0;
      sim -> add_event( this, duration );
    }
    virtual void execute()
    {
      player -> dismiss_pet( "shadow_fiend" );
    }
  };

  int16_t trigger;

  shadow_fiend_spell_t( player_t* player, const std::string& options_str ) : 
    priest_spell_t( "summon", player, SCHOOL_SHADOW, TREE_SHADOW ), trigger(0)
  {
    priest_t* p = player -> cast_priest();

    option_t options[] =
    {
      { "trigger", OPT_INT16, &trigger },
      { NULL }
    };
    parse_options( options, options_str );

    harmful    = false;
    cooldown   = 300.0;
    base_cost  = 300;
    base_cost *= 1.0 - p -> talents.mental_agility * 0.02;
  }

  virtual void execute() 
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "shadow_fiend" );
    player -> action_finish( this );
    new shadow_fiend_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! priest_spell_t::ready() )
      return false;

    return( player -> resource_max    [ RESOURCE_MANA ] - 
	    player -> resource_current[ RESOURCE_MANA ] ) > trigger;
  }
};

// ==========================================================================
// Priest Event Tracking
// ==========================================================================

// priest_t::spell_hit_event ================================================

void priest_t::spell_hit_event( spell_t* s )
{
  player_t::spell_hit_event( s );

  if( s -> school == SCHOOL_SHADOW )
  {
    stack_shadow_weaving( s );
    if( s -> num_ticks ) push_misery( s );
  }

  if( s -> result == RESULT_CRIT )
  {
    trigger_surge_of_light( s );
  }
}

// priest_t::spell_finish_event ==============================================

void priest_t::spell_finish_event( spell_t* s )
{
  player_t::spell_finish_event( s );

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
  player_t::spell_damage_event( s, amount, dmg_type );

  if( s -> school == SCHOOL_SHADOW )
  {
    if( ! sim_t::WotLK && active_vampiric_touch )
    {
      double mana = amount * ( sim_t::WotLK ? 0.02 : 0.05 );

      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> party == party )
        {
	  p -> resource_gain( RESOURCE_MANA, mana, p -> gains.vampiric_touch );
	}
      }
    }
    if( active_vampiric_embrace )
    {
      double adjust = 0.15 + talents.improved_vampiric_embrace * 0.05;
      double health = amount * adjust;

      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> party == party )
        {
	  p -> resource_gain( RESOURCE_HEALTH, health );
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
  if( name == "divine_spirit"    ) return new divine_spirit_t     ( this, options_str );
  if( name == "holy_fire"        ) return new holy_fire_t         ( this, options_str );
  if( name == "inner_fire"       ) return new inner_fire_t        ( this, options_str );
  if( name == "inner_focus"      ) return new inner_focus_t       ( this, options_str );
  if( name == "mind_blast"       ) return new mind_blast_t        ( this, options_str );
  if( name == "penance"          ) return new penance_t           ( this, options_str );
  if( name == "power_infusion"   ) return new power_infusion_t    ( this, options_str );
  if( name == "shadow_word_death") return new shadow_word_death_t ( this, options_str );
  if( name == "shadow_word_pain" ) return new shadow_word_pain_t  ( this, options_str );
  if( name == "shadow_form"      ) return new shadow_form_t       ( this, options_str );
  if( name == "smite"            ) return new smite_t             ( this, options_str );
  if( name == "shadow_fiend"     ) return new shadow_fiend_spell_t( this, options_str );
  if( name == "vampiric_embrace" ) return new vampiric_embrace_t  ( this, options_str );
  if( name == "vampiric_touch"   ) return new vampiric_touch_t    ( this, options_str );

  if( name == "mind_flay" ) 
  {
    if( sim_t::WotLK )
      return new mind_flay_wotlk_t( this, options_str );
    else
      return new mind_flay_bc_t( this, options_str );
  }

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
  attribute_base[ ATTR_STRENGTH  ] =  40;
  attribute_base[ ATTR_AGILITY   ] =  45;
  attribute_base[ ATTR_STAMINA   ] =  60;
  attribute_base[ ATTR_INTELLECT ] = 145;
  attribute_base[ ATTR_SPIRIT    ] = 155;

  attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + talents.enlightenment * 0.01;
  attribute_multiplier_initial[ ATTR_SPIRIT    ] *= 1.0 + talents.enlightenment * 0.01;
  attribute_multiplier_initial[ ATTR_SPIRIT    ] *= 1.0 + talents.spirit_of_redemption * 0.05;

  base_spell_crit = 0.0125;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );
  initial_spell_power_per_spirit = ( talents.spiritual_guidance * 0.05 +
				     talents.twisted_faith      * 0.02 );

  base_attack_power = -10;
  base_attack_crit  = 0.03;
  initial_attack_power_per_strength = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/16.0, 0.01/24.9, 0.01/52.1 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1383, 2620, 3863 );

  health_per_stamina = 10;
  mana_per_intellect = 15;

  if( sim_t::WotLK )
  {
    attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.mental_strength * 0.03;
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

  // Active
  active_devouring_plague = 0;
  active_shadow_word_pain = 0;
  active_vampiric_touch   = 0;
  active_vampiric_embrace = 0;

  // Buffs
  buffs_improved_spirit_tap = 0;
  buffs_inner_fire          = 0;
  buffs_shadow_weaving      = 0;
  buffs_surge_of_light      = 0;

  // Expirations
  expirations_improved_spirit_tap = 0;
  expirations_shadow_weaving      = 0;
}

// priest_t::regen  ==========================================================

void priest_t::regen( double periodicity )
{
  double spirit_regen = periodicity * spirit_regen_per_second();

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;
  }
  else if( recent_cast() )
  {
    double while_casting = talents.meditation * 0.10;
    if( buffs_improved_spirit_tap ) while_casting += talents.improved_spirit_tap * 0.10;
    spirit_regen *= while_casting;
  }

  double mp5_regen = periodicity * mp5 / 5.0;

  resource_gain( RESOURCE_MANA, spirit_regen, gains.spirit_regen );
  resource_gain( RESOURCE_MANA,    mp5_regen, gains.mp5_regen    );

  if( buffs.replenishment )
  {
    double replenishment_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.005 / 1.0;

    resource_gain( RESOURCE_MANA, replenishment_regen, gains.replenishment );
  }

  if( buffs.water_elemental_regen )
  {
    double water_elemental_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.006 / 5.0;

    resource_gain( RESOURCE_MANA, water_elemental_regen, gains.water_elemental_regen );
  }
}

// priest_t::parse_talents =================================================

void priest_t::parse_talents( const std::string& talent_string )
{
  if( talent_string.size() == 64 )
  {
    talent_translation_t translation[] =
    {
      {  8,  &( talents.inner_focus               ) },
      {  9,  &( talents.meditation                ) },
      { 10,  &( talents.improved_inner_fire       ) },
      { 11,  &( talents.mental_agility            ) },
      { 13,  &( talents.mental_strength           ) },
      { 14,  &( talents.divine_spirit             ) },
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
      { 46,  &( talents.shadow_affinity           ) },
      { 47,  &( talents.improved_shadow_word_pain ) },
      { 48,  &( talents.shadow_focus              ) },
      { 50,  &( talents.improved_mind_blast       ) },
      { 50,  &( talents.mind_flay                 ) },
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
  else if( talent_string.size() == 81 )
  {
    talent_translation_t translation[] =
    {
      {  2,  &( talents.twin_disciplines          ) },
      {  8,  &( talents.inner_focus               ) },
      {  9,  &( talents.meditation                ) },
      { 10,  &( talents.improved_inner_fire       ) },
      { 11,  &( talents.mental_agility            ) },
      { 13,  &( talents.mental_strength           ) },
      { 14,  &( talents.divine_spirit             ) },
      { 15,  &( talents.improved_divine_spirit    ) },
      { 16,  &( talents.focused_power             ) },
      { 17,  &( talents.enlightenment             ) },
      { 19,  &( talents.power_infusion            ) },
      { 23,  &( talents.aspiration                ) },
      { 28,  &( talents.penance                   ) },
      { 31,  &( talents.holy_specialization       ) },
      { 33,  &( talents.divine_fury               ) },
      { 39,  &( talents.searing_light             ) },
      { 41,  &( talents.spirit_of_redemption      ) },
      { 42,  &( talents.spiritual_guidance        ) },
      { 43,  &( talents.surge_of_light            ) },
      { 56,  &( talents.improved_spirit_tap       ) },
      { 58,  &( talents.shadow_affinity           ) },
      { 59,  &( talents.improved_shadow_word_pain ) },
      { 60,  &( talents.shadow_focus              ) },
      { 62,  &( talents.improved_mind_blast       ) },
      { 63,  &( talents.mind_flay                 ) },
      { 66,  &( talents.shadow_weaving            ) },
      { 68,  &( talents.vampiric_embrace          ) },
      { 69,  &( talents.improved_vampiric_embrace ) },
      { 70,  &( talents.focused_mind              ) },
      { 71,  &( talents.mind_melt                 ) },
      { 72,  &( talents.darkness                  ) },
      { 73,  &( talents.shadow_form               ) },
      { 74,  &( talents.shadow_power              ) },
      { 76,  &( talents.misery                    ) },
      { 78,  &( talents.vampiric_touch            ) },
      { 79,  &( talents.pain_and_suffering        ) },
      { 80,  &( talents.twisted_faith             ) },
      { 81,  &( talents.dispersion                ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  }
  else
  {
    fprintf( sim -> output_file, "Malformed Priest talent string.  Number encoding should have length 64 for Burning Crusade or 80 for Wrath of the Lich King.\n" );
    assert( 0 );
  }
}

// priest_t::parse_option  =================================================

bool priest_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    { "aspiration",                OPT_INT8,  &( talents.aspiration                ) },
    { "creeping_shadows",          OPT_INT8,  &( talents.creeping_shadows          ) },
    { "darkness",                  OPT_INT8,  &( talents.darkness                  ) },
    { "dispersion",                OPT_INT8,  &( talents.dispersion                ) },
    { "divine_fury",               OPT_INT8,  &( talents.divine_fury               ) },
    { "enlightenment",             OPT_INT8,  &( talents.enlightenment             ) },
    { "focused_mind",              OPT_INT8,  &( talents.focused_mind              ) },
    { "focused_power",             OPT_INT8,  &( talents.focused_power             ) },
    { "force_of_will",             OPT_INT8,  &( talents.force_of_will             ) },
    { "holy_specialization",       OPT_INT8,  &( talents.holy_specialization       ) },
    { "divine_spirit",             OPT_INT8,  &( talents.divine_spirit             ) },
    { "improved_divine_spirit",    OPT_INT8,  &( talents.improved_divine_spirit    ) },
    { "improved_inner_fire",       OPT_INT8,  &( talents.improved_inner_fire       ) },
    { "improved_mind_blast",       OPT_INT8,  &( talents.improved_mind_blast       ) },
    { "improved_mind_flay",        OPT_INT8,  &( talents.improved_mind_flay        ) },
    { "improved_shadow_word_pain", OPT_INT8,  &( talents.improved_shadow_word_pain ) },
    { "improved_spirit_tap",       OPT_INT8,  &( talents.improved_spirit_tap       ) },
    { "improved_vampiric_embrace", OPT_INT8,  &( talents.improved_vampiric_embrace ) },
    { "inner_focus",               OPT_INT8,  &( talents.inner_focus               ) },
    { "meditation",                OPT_INT8,  &( talents.meditation                ) },
    { "mental_agility",            OPT_INT8,  &( talents.mental_agility            ) },
    { "mental_strength",           OPT_INT8,  &( talents.mental_strength           ) },
    { "mind_flay",                 OPT_INT8,  &( talents.mind_flay                 ) },
    { "mind_melt",                 OPT_INT8,  &( talents.mind_melt                 ) },
    { "misery",                    OPT_INT8,  &( talents.misery                    ) },
    { "pain_and_suffering",        OPT_INT8,  &( talents.pain_and_suffering        ) },
    { "penance",                   OPT_INT8,  &( talents.penance                   ) },
    { "power_infusion",            OPT_INT8,  &( talents.power_infusion            ) },
    { "searing_light",             OPT_INT8,  &( talents.searing_light             ) },
    { "shadow_affinity",           OPT_INT8,  &( talents.shadow_affinity           ) },
    { "shadow_focus",              OPT_INT8,  &( talents.shadow_focus              ) },
    { "shadow_form",               OPT_INT8,  &( talents.shadow_form               ) },
    { "shadow_power",              OPT_INT8,  &( talents.shadow_power              ) },
    { "shadow_weaving",            OPT_INT8,  &( talents.shadow_weaving            ) },
    { "spirit_of_redemption",      OPT_INT8,  &( talents.spirit_of_redemption      ) },
    { "spiritual_guidance",        OPT_INT8,  &( talents.spiritual_guidance        ) },
    { "surge_of_light",            OPT_INT8,  &( talents.surge_of_light            ) },
    { "twin_disciplines",          OPT_INT8,  &( talents.twin_disciplines          ) },
    { "twisted_faith",             OPT_INT8,  &( talents.twisted_faith             ) },
    { "vampiric_embrace",          OPT_INT8,  &( talents.vampiric_embrace          ) },
    { "vampiric_touch",            OPT_INT8,  &( talents.vampiric_touch            ) },
    // Glyphs
    { "glyph_blue_promises",       OPT_INT8,  &( glyphs.blue_promises              ) },
    { "glyph_shadow_word_death",   OPT_INT8,  &( glyphs.shadow_word_death          ) },
    { "glyph_shadow_word_pain",    OPT_INT8,  &( glyphs.shadow_word_pain           ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( sim, options );
    return false;
  }

  if( player_t::parse_option( name, value ) ) return true;

  return option_t::parse( sim, options, name, value );
}

// player_t::create_priest  =================================================

player_t* player_t::create_priest( sim_t*       sim, 
				   std::string& name ) 
{
  return new priest_t( sim, name );
}


