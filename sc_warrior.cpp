// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Warrior
// ==========================================================================

enum warrior_stance { STANCE_NONE=0, STANCE_BATTLE, STANCE_BERSERKER, STANCE_DEFENSE=4 };

struct warrior_t : public player_t
{
  // Active
  action_t* active_deep_wounds;
  action_t* active_heroic_strike;
  int       active_stance;
  
  // action_t* ;

  // Buffs
  struct _buffs_t
  {
    int    bloodsurge;
    double death_wish;
    int    flurry;
    double overpower;
    
    void reset() { memset( (void*) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Cooldowns
  struct _cooldowns_t
  {

    void reset() { memset( (void*) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Expirations
  struct _expirations_t
  {
    event_t* bloodsurge;
    event_t* death_wish;
    void reset() { memset( (void*) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_mh_attack;
  gain_t* gains_oh_attack;

  // Procs
  proc_t* procs_glyph_overpower;
  
  // Up-Times
  uptime_t* uptimes_flurry;
  
  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  struct talents_t
  {
    int amored_to_the_teeth;
    int anger_management;
    int bladestorm;
    int blood_frenzy;
    int bloodsurge;
    int bloodthirst;
    int booming_voice;
    int commanding_presence;
    int concussion_blow;
    int critical_block;
    int cruelty;
    int death_wish;
    int deep_wounds;
    int devastate;
    int dual_wield_specialization;
    int endless_rage;
    int flurry;
    int focused_rage;
    int gag_order;
    int impale;
    int improved_berserker_rage;
    int improved_berserker_stance;
    int improved_bloodrage;
    int improved_defensive_stance;
    int improved_execute;
    int improved_heroic_strike;
    int improved_mortal_strike;
    int improved_overpower;
    int improved_rend;
    int improved_slam;
    int improved_revenge;
    int improved_thunderclap;
    int improved_whirlwind;
    int incite;
    int intensify_rage;
    int mace_specialization;
    int mortal_strike;
    int onhanded_weapon_specialization;
    int poleaxe_specialization;
    int precision;
    int puncture;
    int ranoage;
    int shield_mastery;
    int shockwave;
    int strength_of_arms;
    int sudden_death;
    int sword_and_board;
    int sword_specialization;
    int taste_for_blood;
    int titans_grip;
    int toughness;
    int trauma;
    int twohanded_weapon_specialization;
    int unbridled_wrath;
    int unending_fury;
    int unrelenting_assault;
    int vitality;
    int weapon_mastery;
    int wrecking_crew;
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int execution;
    int heroic_strike;
    int mortal_strike;
    int overpower;
    int rending;
    int whirlwind;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  warrior_t( sim_t* sim, std::string& name ) : player_t( sim, WARRIOR, name )
  {
    // Active
    active_deep_wounds   = 0;
    active_heroic_strike = 0;
    active_stance        = STANCE_BATTLE; 
    
    // Gains
    gains_mh_attack = get_gain( "mh_attack" );
    gains_oh_attack = get_gain( "oh_attack" );

    // Procs
    procs_glyph_overpower = get_proc( "glyph_of_overpower" );
    // Up-Times
    uptimes_flurry                = get_uptime( "flurry" );
  
    // Auto-Attack
    main_hand_attack = 0;
    off_hand_attack  = 0;

  }

  // Character Definition
  virtual void      init_base();
  virtual void      combat_begin();
  virtual double    composite_attack_power_multiplier();
  virtual double    composite_attribute_multiplier(int attr);
  virtual void      reset();
  virtual void      regen( double periodicity );
  virtual bool      get_talent_trees( std::vector<int*>& arms, std::vector<int*>& fury, std::vector<int*>& protection );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       primary_resource() { return RESOURCE_RAGE; }

};

// warrior_t::composite_attack_power_multiplier ============================

double warrior_t::composite_attack_power_multiplier()
{
  double m = attack_power_multiplier;

  if( sim -> P309 && active_stance == STANCE_BERSERKER )
  {
    m *= 1 + talents.improved_berserker_stance * 0.02;
  }
  return m;
}

// warrior_t::composite_attribute_multiplier ===============================

double warrior_t::composite_attribute_multiplier( int attr )
{
  double m = attribute_multiplier[ attr ]; 
  if( attr == ATTR_STRENGTH && ! sim -> P309 && active_stance == STANCE_BERSERKER )
  {
    m *= 1 + talents.improved_berserker_stance * 0.04;
  }
  return m;
}


namespace { // ANONYMOUS NAMESPACE =========================================

// ==========================================================================
// Warrior Attack
// ==========================================================================

struct warrior_attack_t : public attack_t
{
  double min_rage, max_rage;
  int stancemask;
  warrior_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true  ) : 
    attack_t( n, player, RESOURCE_RAGE, s, t, special ), 
    min_rage(0), max_rage(0), 
    stancemask(STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE)
  {
    warrior_t* p = player -> cast_warrior();
    may_glance   = false;
    base_hit    += p -> talents.precision * 0.01;
    base_crit   += p -> talents.cruelty   * 0.01;
    normalize_weapon_speed = true;
    if( special )
      base_crit_bonus_multiplier *= 1.0 + p -> talents.impale * 0.10;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double dodge_chance( int delta_level );
  virtual double cost();
  virtual void   execute();
  virtual void   player_buff();
  virtual bool   ready();
};

double warrior_attack_t::dodge_chance( int delta_level )
{
  double chance = attack_t::dodge_chance( delta_level );
  
  warrior_t* p = player -> cast_warrior();
  
  chance -= p -> talents.weapon_mastery * 0.01;
  
  return chance;  
}

// ==========================================================================
// Static Functions
// ==========================================================================

// trigger_bloodsurge =======================================================

static void trigger_bloodsurge( action_t* a )
{
    
  warrior_t* p = a -> player -> cast_warrior();
  
  if( ! a -> result_is_hit() )
    return;
  
  double chance = util_t::talent_rank( p -> talents.bloodsurge, 3, 0.07, 0.13, 0.20 );
  if( ! a -> sim -> roll( chance ) )
    return;
  
  struct bloodsurge_expiration_t : public event_t
  {
    bloodsurge_expiration_t( sim_t* sim, warrior_t* player ) : event_t( sim, player )
    {
      name = "Bloodsurge Expiration";
      player -> aura_gain( "Bloodsurge" );
      player -> _buffs.bloodsurge = 1;
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      warrior_t* p = player -> cast_warrior();
      p -> aura_loss( "bloodsurge Weapon" );
      p -> _buffs.bloodsurge = 0;
      p -> _expirations.bloodsurge = 0;
    }
  };
  
  event_t*& e = p -> _expirations.bloodsurge;
  
  if( e )
  {
    e -> reschedule( 5.0 );
  }
  else
  {
    e = new ( a -> sim ) bloodsurge_expiration_t( a -> sim, p );
  }
    
}

// trigger_deep_wounds ======================================================

static void trigger_deep_wounds( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if( ! p -> talents.deep_wounds )
    return;

  struct deep_wounds_t : public attack_t
  {
    deep_wounds_t( player_t* p ) : attack_t( "deep_wounds", p, RESOURCE_NONE, SCHOOL_BLEED )
    {
      may_miss    = false;
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;

      base_multiplier = 1.0;
      base_tick_time = 1.0;
      num_ticks      = 6;
      tick_power_mod = 0;
    }
    void player_buff() {}
    void target_debuff( int dmg_type ) {}
  };
  double dmg;
  if( a -> weapon == 0 )
  {
    // If no weapon is a attached to the attack, assume it would calculate with
    // the mainhands damage.
    a -> weapon = &( p -> main_hand_weapon );
    dmg  = p -> talents.deep_wounds * 0.16 * a -> calculate_weapon_damage();
    dmg *= a -> total_dd_multiplier();
    a -> weapon = 0;
  }
  else 
  {
    dmg = p -> talents.deep_wounds * 0.16 * a -> calculate_weapon_damage();
    dmg *= a -> total_dd_multiplier();
  }


  if( ! p -> active_deep_wounds ) p -> active_deep_wounds = new deep_wounds_t( p );

  if( p -> active_deep_wounds -> ticking )
  {
    int num_ticks = p -> active_deep_wounds -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_deep_wounds -> current_tick;

    dmg += p -> active_deep_wounds -> base_tick_dmg * remaining_ticks;

    p -> active_deep_wounds -> cancel();
  }
  p -> active_deep_wounds -> base_tick_dmg = dmg / 6.0;
  p -> active_deep_wounds -> schedule_tick();
}

// trigger_overpower_activation =============================================

static void trigger_overpower_activation( warrior_t* p )
{
  p -> _buffs.overpower = p -> sim -> current_time + 5.0;
  p -> aura_gain( "Overpower Activation" );
}

// =========================================================================
// Warrior Attacks
// =========================================================================

// warrior_attack_t::parse_options =========================================

void warrior_attack_t::parse_options( option_t*          options,
                                      const std::string& options_str )
{
  option_t base_options[] =
  {
    { "min_rage",       OPT_FLT, &min_rage       },
    { "max_rage",       OPT_FLT, &max_rage       },
    { "rage>",          OPT_FLT, &min_rage       },
    { "rage<",          OPT_FLT, &max_rage       },
    { NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// warrior_attack_t::cost ====================================================

double warrior_attack_t::cost()
{
  warrior_t* p = player -> cast_warrior();
  double c = attack_t::cost();
  if( harmful )
    c -= p -> talents.focused_rage;
  return c;
}

// warrior_attack_t::execute =================================================

void warrior_attack_t::execute()
{
  attack_t::execute();

  warrior_t* p = player -> cast_warrior();

  if( result == RESULT_CRIT )
  {
    trigger_deep_wounds( this );
    if( p -> talents.flurry ) 
    {
      p -> aura_gain( "Flurry (3)" );
      p -> _buffs.flurry = 3;
    }
  }
  else if( result == RESULT_DODGE  )
  {
    trigger_overpower_activation( p );
  }
  else if( result == RESULT_PARRY )
  {
    if( p -> glyphs.overpower && sim -> roll( 0.50 ) ) 
    {
      trigger_overpower_activation( p );
      p -> procs_glyph_overpower -> occur();
    }
  }
}

// warrior_attack_t::player_buff ============================================

void warrior_attack_t::player_buff()
{
  attack_t::player_buff();

  warrior_t* p = player -> cast_warrior();
  
  if( weapon )
  {
    if( p -> talents.mace_specialization )
    {
      if( weapon -> type == WEAPON_MACE || 
          weapon -> type == WEAPON_MACE_2H )
      {
        player_penetration += p -> talents.mace_specialization * 0.03;
      }
    }
    if( p -> talents.poleaxe_specialization )
    {
      if( weapon -> type == WEAPON_AXE_2H ||
	      weapon -> type == WEAPON_AXE    ||
	      weapon -> type == WEAPON_POLEARM )
      {
        player_crit            += p -> talents.poleaxe_specialization * 0.01;
        //FIX ME! Is this additive with Impale?
        player_crit_bonus_multiplier *= 1 + p -> talents.poleaxe_specialization * 0.01;
      }
    }
    if( p -> talents.dual_wield_specialization )
    {
      if( weapon -> slot == SLOT_OFF_HAND )
      {
        player_multiplier *= 1.0 + p -> talents.dual_wield_specialization * 0.10;
      }
    }
  }
  if( p -> active_stance == STANCE_BATTLE)
  {
    // A balanced combat stance
  }
  else if( p -> active_stance == STANCE_BERSERKER )
  {
    player_crit += 0.03;
    
  }
  else if( p -> active_stance == STANCE_DEFENSE )
  {
    player_multiplier *= 1.0 - 0.1;
  }
  
  player_multiplier *= 1.0 + p -> _buffs.death_wish;
  
}

// warrior_attack_t::ready() ================================================

bool warrior_attack_t::ready()
{
  if( ! attack_t::ready() )
    return false;

  warrior_t* p = player -> cast_warrior();

  if( min_rage > 0 )
    if( p -> resource_current[ RESOURCE_RAGE ] < min_rage )
      return false;

  if( max_rage > 0 )
    if( p -> resource_current[ RESOURCE_RAGE ] > max_rage )
      return false;
  
  // Attack vailable in current stance?
  if( ( stancemask & p -> active_stance ) == 0)
    return false;
    
  return true;
}

// Melee Attack ============================================================

struct melee_t : public warrior_attack_t
{
  double rage_conversion_value;
  melee_t( const char* name, player_t* player ) : 
    warrior_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, false ), rage_conversion_value(0)
  {
    warrior_t* p = player -> cast_warrior();

    base_direct_dmg = 1;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;
    
    normalize_weapon_speed = false;
    
    if( p -> dual_wield() ) base_hit -= 0.19;
    // Rage Conversion Value, needed for: damage done => rage gained
    rage_conversion_value = 0.0091107836 * p -> level * p -> level + 3.225598133* p -> level + 4.2652911;
  }

  virtual double execute_time()
  {
    double t = warrior_attack_t::execute_time();
    warrior_t* p = player -> cast_warrior();
    if( p -> _buffs.flurry > 0 ) 
    {
      t *= 1.0 / ( 1.0 + 0.05 * p -> talents.flurry  ) ;
    }
    p -> uptimes_flurry -> update( p -> _buffs.flurry > 0 );
    return t;
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
   
    warrior_t* p = player -> cast_warrior();

    if( p -> _buffs.flurry > 0 )
    {
      p -> _buffs.flurry--;
      if( p -> _buffs.flurry == 0) p -> aura_loss( "Flurry" );
    }

    if( result_is_hit() )
    {
      /* http://www.wowwiki.com/Formulas:Rage_generation
      Definitions
      
      For the purposes of the formulae presented here, we define:
      R:	rage generated
      d:	damage amount
      c:	rage conversion value
      s:	weapon speed ( time_to_execute )
      f:	hit factor, 3.5 MH, 1.75 OH, Crit = *2
      Rage Generated By Dealing Damage
      
      Rage is generated by successful autoattack swings ('white' damage) that damage an opponent. Special attacks ('yellow' damage) do not generate rage.
      R	= 15d / 4c + fs / 2 */
      double hitfactor = 3.5;
      if( result == RESULT_CRIT )
        hitfactor *= 2.0;
      if( weapon -> slot == SLOT_OFF_HAND )
        hitfactor /= 2.0;
        
      double rage_gained = 15.0 * direct_dmg / (4.0 * rage_conversion_value ) + time_to_execute * hitfactor / 2.0;

      if( p -> talents.endless_rage )
        rage_gained *= 1.25;
         
      p -> resource_gain( RESOURCE_RAGE, rage_gained, weapon -> slot == SLOT_OFF_HAND ? p -> gains_oh_attack : p -> gains_mh_attack );
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public warrior_attack_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "auto_attack", player )
  {
    warrior_t* p = player -> cast_warrior();

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", player );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", player );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
      
      // Dual-wielding, if one of the two weapons is a 2hander we have to have
      // the Titan's Grip talent!
      if( p -> main_hand_weapon.group() == WEAPON_2H || p -> off_hand_weapon.group() == WEAPON_2H )
        assert(p -> talents.titans_grip != 0 );
    }

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    p -> main_hand_attack -> schedule_execute();
    if( p -> off_hand_attack ) p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Heroic Strike ===========================================================

struct heroic_strike_t : public warrior_attack_t
{
  heroic_strike_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "heroic_strike",  player, SCHOOL_PHYSICAL, TREE_ARMS )
  {
    warrior_t* p = player -> cast_warrior();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 76, 13, 495, 495, 0, 15 },
      { 72, 12, 432, 432, 0, 15 },
      { 70, 11, 317, 317, 0, 15 },
      { 66, 10, 234, 234, 0, 15 },
      { 60,  9, 201, 201, 0, 15 },
      { 56,  8, 178, 178, 0, 15 },
      { 0, 0 }
    };
    init_rank( ranks );
    weapon = &( p -> main_hand_weapon );
    base_cost -= p -> talents.improved_heroic_strike;
    trigger_gcd     = 0;
    
    
    // Heroic Strike needs swinging auto_attack!
    assert( p -> main_hand_attack -> execute_event == 0 );
    observer = &( p -> active_heroic_strike ); 
  }

  // FIX ME!!
  // No real clue how to do a "on next swing" type action.
  // Because the rage is also only consumed when the HS hits
  // Probably needs a fancier way then i could think of
};

// Bloodthirst ===============================================================

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "bloodthirst",  player, SCHOOL_PHYSICAL, TREE_FURY )
  {
    warrior_t* p = player -> cast_warrior();
    assert( p -> talents.bloodthirst );
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    cooldown          = 5.0;
    base_cost         = 30;
    base_direct_dmg   = 1;
    direct_power_mod  = 0.50;

  }
  virtual void execute()
  {
    warrior_attack_t::execute();
    trigger_bloodsurge( this );
  }
};

// Mortal Strike =============================================================

struct mortal_strike_t : public warrior_attack_t
{
  mortal_strike_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "mortal_strike",  player, SCHOOL_PHYSICAL, TREE_FURY )
  {
    warrior_t* p = player -> cast_warrior();
    assert( p -> talents.mortal_strike );
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    static rank_t ranks[] =
    {
      { 80, 8, 380, 380, 0, 30 },
      { 75, 7, 320, 320, 0, 30 },
      { 70, 6, 210, 210, 0, 30 },
      { 66, 5, 185, 185, 0, 30 },
      { 60, 4, 160, 160, 0, 30 },
      { 0, 0 }
    };
    init_rank( ranks );
    
    cooldown               = 6.0 - ( p -> talents.improved_mortal_strike / 3.0 );
    base_multiplier       *= 1 + util_t::talent_rank( p -> talents.improved_mortal_strike, 3, 0.03, 0.06, 0.10)
                               + ( p -> glyphs.mortal_strike ? 0.10 : 0.0 );
    
    weapon = &( p -> main_hand_weapon );
  }
};

// Overpower =================================================================

struct overpower_t : public warrior_attack_t
{
  overpower_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "overpower",  player, SCHOOL_PHYSICAL, TREE_ARMS )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    may_dodge = false; 
    may_parry = false; 
    may_block = false; // The Overpower cannot be blocked, dodged or parried.
    base_cost  = 5.0;
    base_crit      += p -> talents.improved_overpower * 0.25;
    base_direct_dmg = 1;
    cooldown   = 5.0 - p -> talents.unrelenting_assault;
    stancemask = STANCE_BATTLE;
    
    weapon = &( p -> main_hand_weapon );

  }
  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();
    p -> _buffs.overpower = 0;
  }
  virtual bool ready()
  {
    if( ! warrior_attack_t::ready() )
      return false;
      
    warrior_t* p = player -> cast_warrior();

    if( p -> _buffs.overpower > p -> sim -> current_time )
      return true;
    
    return false;
  }
};

// Slam ====================================================================

struct slam_t : public warrior_attack_t
{
  int bloodsurge;
  slam_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "slam",  player, SCHOOL_PHYSICAL, TREE_FURY ),
    bloodsurge(0)
  {
    warrior_t* p = player -> cast_warrior();

    option_t options[] =
    {
      { "bloodsurge", OPT_INT, &bloodsurge },
      { NULL }
    };
    parse_options( options, options_str );
    
    static rank_t ranks[] =
    {
      { 80, 8, 250, 250, 0, 15 },
      { 74, 7, 220, 220, 0, 15 },
      { 69, 6, 140, 140, 0, 15 },
      { 61, 5, 105, 105, 0, 15 },
      { 54, 4,  87,  87, 0, 15 },
      { 0, 0 }
    };
    init_rank( ranks );

    base_execute_time  = 1.5 - p -> talents.improved_slam * 0.5;

   
    weapon = &( p -> main_hand_weapon );
  }
  virtual double haste()
  {
    // No haste for slam cast?
    return 1.0;
  }
  
  virtual double execute_time()
  {
    warrior_t* p = player -> cast_warrior();
    if( p -> _buffs.bloodsurge )
      return 0.0;
    else
      return warrior_attack_t::execute_time();
  }
  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = player -> cast_warrior();
    if( p -> _buffs.bloodsurge )
      event_t::early( p -> _expirations.bloodsurge );
  }  
  virtual void schedule_execute()
  {
    warrior_attack_t::schedule_execute();
    
    warrior_t* p = player -> cast_warrior();
    // While slam is casting, the auto_attack is paused
    // So we simply reschedule the auto_attack by slam's casttime
    double time_to_next_hit;
    // Mainhand
    if( p -> main_hand_attack )
    { 
      time_to_next_hit  = p -> main_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += base_execute_time;
      p -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
    // Offhand
    if( p -> off_hand_attack )
    {
      time_to_next_hit  = p -> off_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += base_execute_time;
      p -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
  }
  
  virtual bool ready()
  {
    if( ! warrior_attack_t::ready() )
      return false;
      
    warrior_t* p = player -> cast_warrior();
    if( p -> _buffs.bloodsurge == 0 && bloodsurge )
      return false;
      
    return true;
  }
};

// Whirlwind ===============================================================

struct whirlwind_t : public warrior_attack_t
{
  whirlwind_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "whirlwind",  player, SCHOOL_PHYSICAL, TREE_FURY )
  {
    warrior_t* p = player -> cast_warrior();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    cooldown               = 10.0 - ( p -> glyphs.whirlwind ? 2 : 0 );

    base_cost              = 25;
    base_multiplier       *= 1 + p -> talents.improved_whirlwind * 0.10 + p -> talents.unending_fury * 0.02;
    base_direct_dmg        = 1;

    
    stancemask = STANCE_BERSERKER;

    weapon = &( p -> main_hand_weapon );
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    attack_t::consume_resource();
    
    // MH hit
    weapon = &( player -> main_hand_weapon );
    warrior_attack_t::execute();
    trigger_bloodsurge( this );

    // OH hit
    weapon = &( player -> off_hand_weapon );
    if( weapon )
    {
      warrior_attack_t::execute();
      trigger_bloodsurge( this );
    }

    player -> action_finish( this );
  }
};


// =========================================================================
// Warrior Spells
// =========================================================================

// Stance ==================================================================

struct stance_t : public spell_t
{
  int switch_to_stance;
  stance_t( player_t* player, const std::string& options_str ) : 
    spell_t( "stance", player ), switch_to_stance(0)
  {
    warrior_t* p = player -> cast_warrior();

    std::string stance_str;
    option_t options[] =
    {
      { "choose",  OPT_STRING, &stance_str     },
      { NULL }
    };
    parse_options( options, options_str );
    
    if( ! stance_str.empty() )
    {
      if( stance_str == "battle" )
        switch_to_stance = STANCE_BATTLE;
      else if( stance_str == "berserker" || stance_str == "zerker" )
        switch_to_stance = STANCE_BERSERKER;
      else if( stance_str == "def" || stance_str == "defensive" )
        switch_to_stance = STANCE_DEFENSE;
    }
    if( switch_to_stance == 0)
      switch_to_stance = p -> active_stance;

    trigger_gcd = 0;
    cooldown    = 1.0;
  }

  virtual void execute()
  {
    const char* stance_name[] = { "Unknown Stance",
                                  "Battle Stance",
                                  "Berserker Stance",
                                  "Unpossible Stance",
                                  "Defensive Stance"};
    warrior_t* p = player -> cast_warrior();
    p -> aura_loss( stance_name[ p -> active_stance  ]);
    p -> active_stance = switch_to_stance;
    p -> aura_gain( stance_name[ p -> active_stance  ]);
    update_ready();
  }
  
  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    return p -> active_stance != switch_to_stance;
  }
};

// Death Wish ==============================================================

struct death_wish_t : public spell_t
{
  death_wish_t( player_t* player, const std::string& options_str ) : 
    spell_t( "death_wish", player )
  {
    warrior_t* p = player -> cast_warrior();

    std::string stance_str;
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    trigger_gcd = 0;
    cooldown    = 180.0 * ( 1.0 - 0.11 * p -> talents.intensify_rage );
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, warrior_t* p ) : event_t( sim, p )
      {
        name = "Death Wish Expiration";
        p -> aura_gain( "Death Wish" );
        p -> _buffs.death_wish = 0.20;
        sim -> add_event( this, 30.0 );
      }
      virtual void execute()
      {
        warrior_t* p = player -> cast_warrior();
        p -> aura_loss( "Death Wish" );
        p -> _buffs.death_wish = 0;
        p -> _expirations.death_wish = 0;
      }
    };

    warrior_t* p = player -> cast_warrior();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> _expirations.death_wish = new ( sim ) expiration_t( sim, p );
  }
  
};



} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Warrior Character Definition
// =========================================================================

// warrior_t::create_action  =================================================

action_t* warrior_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if( name == "auto_attack"         ) return new auto_attack_t        ( this, options_str );
  if( name == "bloodthirst"         ) return new bloodthirst_t        ( this, options_str );
  if( name == "heroic_strike"       ) return new heroic_strike_t      ( this, options_str );
  if( name == "mortal_strike"       ) return new mortal_strike_t      ( this, options_str );
  if( name == "overpower"           ) return new overpower_t          ( this, options_str );
  if( name == "slam"                ) return new slam_t               ( this, options_str );
  if( name == "stance"              ) return new stance_t             ( this, options_str );
  if( name == "whirlwind"           ) return new whirlwind_t          ( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_base ========================================================

void warrior_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = 179; // Tauren lvl 80
  attribute_base[ ATTR_AGILITY   ] = 108;
  attribute_base[ ATTR_STAMINA   ] = 161;
  attribute_base[ ATTR_INTELLECT ] =  31;
  attribute_base[ ATTR_SPIRIT    ] =  61;
  resource_base[  RESOURCE_RAGE  ] = 100;
  
  initial_attack_power_per_strength = 2.0;
  
  // FIX ME!
  base_attack_power = -20;
  base_attack_crit = 0.031905;
  
  initial_attack_crit_per_agility = rating_t::interpolate( level, 1 / 2000, 1 / 3200, 1 / 6256.61 ); 
  
  health_per_stamina = 10;
  
  base_gcd = 1.5;
}


// warrior_t::combat_begin =====================================================

void warrior_t::combat_begin()
{
  player_t::combat_begin();
  // We start with zero rage into combat.
  resource_current[ RESOURCE_RAGE ] = 0;

}

// warrior_t::reset ===========================================================

void warrior_t::reset()
{
  player_t::reset();

  _buffs.reset();
  _cooldowns.reset();
  _expirations.reset();

}

// warrior_t::regen ==========================================================

void warrior_t::regen( double periodicity )
{
  player_t::regen( periodicity );
  // Anger Management?
   
}

// warrior_t::get_talent_trees ==============================================

bool warrior_t::get_talent_trees( std::vector<int*>& arms,
                                std::vector<int*>& fury,
                                std::vector<int*>& protection )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.improved_heroic_strike          ) }, {  1, &( talents.amored_to_the_teeth       ) }, {  1, &( talents.improved_bloodrage         ) } },
    { {  2, NULL                                         }, {  2, &( talents.booming_voice             ) }, {  2, NULL                                    } },
    { {  3, &( talents.improved_rend                   ) }, {  3, &( talents.cruelty                   ) }, {  3, &( talents.improved_thunderclap       ) } },
    { {  4, NULL                                         }, {  4, NULL                                   }, {  4, &( talents.incite                     ) } },
    { {  5, NULL                                         }, {  5, &( talents.unbridled_wrath           ) }, {  5, NULL                                    } },
    { {  6, NULL                                         }, {  6, NULL                                   }, {  6, NULL                                    } },
    { {  7, &( talents.improved_overpower              ) }, {  7, NULL                                   }, {  7, &( talents.improved_revenge           ) } },
    { {  8, &( talents.anger_management                ) }, {  8, NULL                                   }, {  8, &( talents.shield_mastery             ) } },
    { {  9, &( talents.impale                          ) }, {  9, &( talents.commanding_presence       ) }, {  9, &( talents.toughness                  ) } },
    { { 10, &( talents.deep_wounds                     ) }, { 10, &( talents.dual_wield_specialization ) }, { 10, NULL                                    } },
    { { 11, &( talents.twohanded_weapon_specialization ) }, { 11, &( talents.improved_execute          ) }, { 11, NULL                                    } },
    { { 12, &( talents.taste_for_blood                 ) }, { 12, NULL                                   }, { 12, &( talents.puncture                   ) } },
    { { 13, &( talents.poleaxe_specialization          ) }, { 13, &( talents.precision                 ) }, { 13, NULL                                    } },
    { { 14, NULL                                         }, { 14, &( talents.death_wish                ) }, { 14, &( talents.concussion_blow            ) } },
    { { 15, &( talents.mace_specialization             ) }, { 15, &( talents.weapon_mastery            ) }, { 15, &( talents.gag_order                  ) } },
    { { 16, &( talents.sword_specialization            ) }, { 16, &( talents.improved_berserker_rage   ) }, { 16, &( talents.onhanded_weapon_specialization ) } },
    { { 17, NULL                                         }, { 17, &( talents.flurry                    ) }, { 17, &( talents.improved_defensive_stance  ) } },
    { { 18, NULL                                         }, { 18, &( talents.intensify_rage            ) }, { 18, NULL                                    } },
    { { 19, &( talents.trauma                          ) }, { 19, &( talents.bloodthirst               ) }, { 19, &( talents.focused_rage               ) } },
    { { 20, NULL                                         }, { 20, &( talents.improved_whirlwind        ) }, { 20, &( talents.vitality                   ) } },
    { { 21, &( talents.mortal_strike                   ) }, { 21, NULL                                   }, { 21, NULL                                    } },
    { { 22, &( talents.strength_of_arms                ) }, { 22, &( talents.improved_berserker_stance ) }, { 22, NULL                                    } },
    { { 23, &( talents.improved_slam                   ) }, { 23, NULL                                   }, { 23, &( talents.devastate                  ) } },
    { { 24, &( talents.improved_mortal_strike          ) }, { 24, &( talents.ranoage                   ) }, { 24, &( talents.critical_block             ) } },
    { { 25, &( talents.unrelenting_assault             ) }, { 25, &( talents.bloodsurge                ) }, { 25, &( talents.sword_and_board            ) } },
    { { 26, &( talents.sudden_death                    ) }, { 26, &( talents.unending_fury             ) }, { 26, NULL                                    } },
    { { 27, &( talents.endless_rage                    ) }, { 27, &( talents.titans_grip               ) }, { 27, &( talents.shockwave                  ) } },
    { { 28, &( talents.blood_frenzy                    ) }, {  0, NULL                                   }, {  0, NULL                                    } },
    { { 29, &( talents.wrecking_crew                   ) }, {  0, NULL                                   }, {  0, NULL                                    } },
    { { 30, &( talents.bladestorm                      ) }, {  0, NULL                                   }, {  0, NULL                                    } }
  };
  
  return player_t::get_talent_trees( arms, fury, protection, translation );
}

// warrior_t::parse_option  ==============================================

bool warrior_t::parse_option( const std::string& name,
                            const std::string& value )
{
  option_t options[] =
  {
    // Talents
    { "amored_to_the_teeth",             OPT_INT, &( talents.amored_to_the_teeth             ) },
    { "anger_management",                OPT_INT, &( talents.anger_management                ) },
    { "bladestorm",                      OPT_INT, &( talents.bladestorm                      ) },
    { "blood_frenzy",                    OPT_INT, &( talents.blood_frenzy                    ) },
    { "bloodsurge",                      OPT_INT, &( talents.bloodsurge                      ) },
    { "bloodthirst",                     OPT_INT, &( talents.bloodthirst                     ) },
    { "booming_voice",                   OPT_INT, &( talents.booming_voice                   ) },
    { "commanding_presence",             OPT_INT, &( talents.commanding_presence             ) },
    { "concussion_blow",                 OPT_INT, &( talents.concussion_blow                 ) },
    { "critical_block",                  OPT_INT, &( talents.critical_block                  ) },
    { "cruelty",                         OPT_INT, &( talents.cruelty                         ) },
    { "death_wish",                      OPT_INT, &( talents.death_wish                      ) },
    { "deep_wounds",                     OPT_INT, &( talents.deep_wounds                     ) },
    { "devastate",                       OPT_INT, &( talents.devastate                       ) },
    { "dual_wield_specialization",       OPT_INT, &( talents.dual_wield_specialization       ) },
    { "endless_rage",                    OPT_INT, &( talents.endless_rage                    ) },
    { "flurry",                          OPT_INT, &( talents.flurry                          ) },
    { "focused_rage",                    OPT_INT, &( talents.focused_rage                    ) },
    { "gag_order",                       OPT_INT, &( talents.gag_order                       ) },
    { "impale",                          OPT_INT, &( talents.impale                          ) },
    { "improved_berserker_rage",         OPT_INT, &( talents.improved_berserker_rage         ) },
    { "improved_berserker_stance",       OPT_INT, &( talents.improved_berserker_stance       ) },
    { "improved_bloodrage",              OPT_INT, &( talents.improved_bloodrage              ) },
    { "improved_defensive_stance",       OPT_INT, &( talents.improved_defensive_stance       ) },
    { "improved_execute",                OPT_INT, &( talents.improved_execute                ) },
    { "improved_heroic_strike",          OPT_INT, &( talents.improved_heroic_strike          ) },
    { "improved_mortal_strike",          OPT_INT, &( talents.improved_mortal_strike          ) },
    { "improved_overpower",              OPT_INT, &( talents.improved_overpower              ) },
    { "improved_rend",                   OPT_INT, &( talents.improved_rend                   ) },
    { "improved_revenge",                OPT_INT, &( talents.improved_revenge                ) },
    { "improved_slam",                   OPT_INT, &( talents.improved_slam                   ) },
    { "improved_thunderclap",            OPT_INT, &( talents.improved_thunderclap            ) },
    { "improved_whirlwind",              OPT_INT, &( talents.improved_whirlwind              ) },
    { "incite",                          OPT_INT, &( talents.incite                          ) },
    { "intensify_rage",                  OPT_INT, &( talents.intensify_rage                  ) },
    { "mace_specialization",             OPT_INT, &( talents.mace_specialization             ) },
    { "mortal_strike",                   OPT_INT, &( talents.mortal_strike                   ) },
    { "onhanded_weapon_specialization",  OPT_INT, &( talents.onhanded_weapon_specialization  ) },
    { "poleaxe_specialization",          OPT_INT, &( talents.poleaxe_specialization          ) },
    { "precision",                       OPT_INT, &( talents.precision                       ) },
    { "puncture",                        OPT_INT, &( talents.puncture                        ) },
    { "ranoage",                         OPT_INT, &( talents.ranoage                         ) },
    { "shield_mastery",                  OPT_INT, &( talents.shield_mastery                  ) },
    { "shockwave",                       OPT_INT, &( talents.shockwave                       ) },
    { "strength_of_arms",                OPT_INT, &( talents.strength_of_arms                ) },
    { "sudden_death",                    OPT_INT, &( talents.sudden_death                    ) },
    { "sword_and_board",                 OPT_INT, &( talents.sword_and_board                 ) },
    { "sword_specialization",            OPT_INT, &( talents.sword_specialization            ) },
    { "taste_for_blood",                 OPT_INT, &( talents.taste_for_blood                 ) },
    { "titans_grip",                     OPT_INT, &( talents.titans_grip                     ) },
    { "toughness",                       OPT_INT, &( talents.toughness                       ) },
    { "trauma",                          OPT_INT, &( talents.trauma                          ) },
    { "twohanded_weapon_specialization", OPT_INT, &( talents.twohanded_weapon_specialization ) },
    { "unbridled_wrath",                 OPT_INT, &( talents.unbridled_wrath                 ) },
    { "unending_fury",                   OPT_INT, &( talents.unending_fury                   ) },
    { "unrelenting_assault",             OPT_INT, &( talents.unrelenting_assault             ) },
    { "vitality",                        OPT_INT, &( talents.vitality                        ) },
    { "weapon_mastery",                  OPT_INT, &( talents.weapon_mastery                  ) },
    { "wrecking_crew",                   OPT_INT, &( talents.wrecking_crew                   ) },
    // Glyphs
    { "glyph_of_execution",              OPT_INT, &( glyphs.execution                        ) },
    { "glyph_of_heroic_strike",          OPT_INT, &( glyphs.heroic_strike                    ) },
    { "glyph_of_mortal_strike",          OPT_INT, &( glyphs.mortal_strike                    ) },
    { "glyph_of_overpower",              OPT_INT, &( glyphs.overpower                        ) },
    { "glyph_of_rending",                OPT_INT, &( glyphs.rending                          ) },
    { "glyph_of_whirlwind",              OPT_INT, &( glyphs.whirlwind                        ) },
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

// player_t::create_warrior ===============================================

player_t* player_t::create_warrior( sim_t*       sim, 
                                    std::string& name ) 
{
  return new warrior_t( sim, name );
}
