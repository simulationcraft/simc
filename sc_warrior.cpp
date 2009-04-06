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
    int    bloodrage;
    int    bloodsurge;
    double death_wish;
    int    flurry;
    double overpower;
    int    recklessness;
    int    heroic_strike;
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
    event_t* blood_frenzy;
    event_t* bloodsurge;
    event_t* death_wish;
    event_t* recklessness;
    event_t* sudden_death;
    event_t* trauma;
    
    void reset() { memset( (void*) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_anger_management;
  gain_t* gains_avoided_attacks;
  gain_t* gains_bloodrage;
  gain_t* gains_berserker_rage;
  gain_t* gains_glyph_of_heroic_strike;
  gain_t* gains_mh_attack;
  gain_t* gains_oh_attack;

  // Procs
  proc_t* procs_bloodsurge;
  proc_t* procs_glyph_overpower;
  proc_t* procs_taste_for_blood;
  
  // Up-Times
  uptime_t* uptimes_flurry;
  uptime_t* uptimes_heroic_strike;
  uptime_t* uptimes_rage_cap;
  
  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  struct talents_t
  {
    int armored_to_the_teeth;
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
    gains_anger_management       = get_gain( "anger_management" );
    gains_avoided_attacks        = get_gain( "avoided_attacks" );
    gains_bloodrage              = get_gain( "bloodrage" );
    gains_berserker_rage         = get_gain( "berserker_rage" );
    gains_glyph_of_heroic_strike = get_gain( "glyph_of_heroic_strike" );
    gains_mh_attack              = get_gain( "mh_attack" );
    gains_oh_attack              = get_gain( "oh_attack" );
    
    // Procs
    procs_bloodsurge      = get_proc( "bloodusrge" );
    procs_glyph_overpower = get_proc( "glyph_of_overpower" );
    procs_taste_for_blood = get_proc( "taste_for_blood" );
    
    // Up-Times
    uptimes_flurry                = get_uptime( "flurry" );
    uptimes_heroic_strike         = get_uptime( "heroic_strike" );
    uptimes_rage_cap              = get_uptime( "rage_cap" );
  
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
  m *= 1 + talents.blood_frenzy * 0.03;
  return m;
}

// warrior_t::composite_attribute_multiplier ===============================

double warrior_t::composite_attribute_multiplier( int attr )
{
  double m = attribute_multiplier[ attr ]; 
  if( attr == ATTR_STRENGTH )
  {
    if ( ! sim -> P309 && active_stance == STANCE_BERSERKER )
    {
      m *= 1 + talents.improved_berserker_stance * 0.04;
    }
     m *= 1 + talents.strength_of_arms * 0.02;
     m *= 1 + talents.vitality * 0.02;
  }
  else if( attr == ATTR_STAMINA )
  {
     m *= 1 + talents.strength_of_arms * 0.02;
     m *= 1 + talents.vitality * 0.02;
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
  bool aoe_attack;
  int stancemask;
  warrior_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true  ) : 
    attack_t( n, player, RESOURCE_RAGE, s, t, special ), 
    min_rage(0), max_rage(0), 
    aoe_attack(false),
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
  virtual void   consume_resource();
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

// trigger_blood_frenzy =====================================================

static void trigger_blood_frenzy( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();
  if( ! p -> talents.blood_frenzy )
    return;
  
  struct blood_frenzy_expiration_t : public event_t
  {
    blood_frenzy_expiration_t( sim_t* sim, warrior_t* player, double duration ) : event_t( sim, player, 0 )
    {
      name = "Blood Frenzy Expiration";
      sim -> target -> debuffs.blood_frenzy++;
      sim -> add_event( this, duration );
    }
    virtual void execute()
    {
      warrior_t* p = player -> cast_warrior();
      sim -> target -> debuffs.blood_frenzy--;
      p -> _expirations.blood_frenzy = 0;
    }
  };
  
  event_t*& e = p -> _expirations.blood_frenzy;
  
  double duration = a -> num_ticks * a -> base_tick_time;
  
  if( e )
  {
    // The new bleeds duration lasts longer than the current one.
    // Different duration: Deep Wounds vs Rend (+glyph)
    if( duration > ( e -> occurs() - a -> sim -> current_time ) )
      e -> reschedule( duration );
  }
  else
  {
    e = new ( a -> sim ) blood_frenzy_expiration_t( a -> sim, p, duration);
  }
    
}

// trigger_bloodsurge =======================================================

static void trigger_bloodsurge( action_t* a )
{
    
  warrior_t* p = a -> player -> cast_warrior();
  
  if( ! a -> result_is_hit() )
    return;
  
  double chance = util_t::talent_rank( p -> talents.bloodsurge, 3, 0.07, 0.13, 0.20 );
  if( ! a -> sim -> roll( chance ) )
    return;
  
  p -> procs_bloodsurge -> occur();
  
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
      p -> aura_loss( "Bloodsurge" );
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

// trigger_tier7_4pc ========================================================

static void trigger_tier7_4pc( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();
  if( p -> gear.tier7_4pc == 0 )
    return;
    
  if( ! a -> sim -> roll( 0.10 ) ) 
    return;
  
  struct heightened_reflexes_expiration_t : public event_t
  {
    heightened_reflexes_expiration_t( sim_t* sim, warrior_t* p ) : event_t( sim, p )
    {
      name = "Spirits of the Lost (tier7_4pc) Expiration";
      player -> aura_gain( "Spirits of the Lost (tier7_4pc)" );
      player -> buffs.tier7_4pc = 5;
      sim -> add_event( this, 30.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Heightened Reflexes" );
      player -> buffs.tier7_4pc = 0;
      player -> expirations.tier7_4pc = 0;
    }
  };

  p -> procs.tier7_4pc -> occur();

  event_t*& e = p -> expirations.tier7_4pc;

  if( e )
  {
    e -> reschedule( 30.0 );
  }
  else
  {
    e = new ( a -> sim ) heightened_reflexes_expiration_t( a -> sim, p );
  }
}

// trigger_deep_wounds ======================================================

static void trigger_deep_wounds( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if( ! p -> talents.deep_wounds )
    return;

  struct deep_wounds_t : public warrior_attack_t
  {
    deep_wounds_t( warrior_t* p ) : 
      warrior_attack_t( "deep_wounds", p, SCHOOL_BLEED, TREE_ARMS )
    {
      may_miss = may_dodge = may_block = may_glance = may_crit = may_parry = false;
      proc        = true;
      background  = true;
      trigger_gcd = 0;
      base_cost   = 0;

      base_multiplier = 1.0;
      base_tick_time = 1.0;
      num_ticks      = 6;
      tick_power_mod = 0;
      
      weapon_multiplier = 0; // 1/5th of weapon_damage per tick.
      weapon   = &( p -> main_hand_weapon );
    }
    virtual void player_buff() {}
    virtual void target_debuff( int dmg_type ) 
    {
      if( sim -> target -> debuffs.mangle || sim -> target -> debuffs.trauma )
        target_multiplier = 1.30;
    }
    virtual void execute()
    {
      trigger_blood_frenzy( this );
      schedule_tick();
    }
    virtual void tick()
    {
      warrior_attack_t::tick();
      trigger_tier7_4pc( this );
    }
  };

  double dmg                   = 0;
  double tmp_weapon_multiplier = 0;

  // Every action HAS to have an weapon associated.
  assert( a -> weapon != 0 );
  
  // But multiplier is set to 0 for attacks like Bloodthirst, Execute
  // So get the wepaon_damage with a multiplier of 1.0 for DW calculation
  
  tmp_weapon_multiplier = a -> weapon_multiplier;
  a -> weapon_multiplier = 1.0;
  
  dmg  = p -> talents.deep_wounds * 0.16 * a -> calculate_weapon_damage();
  dmg *= a -> total_dd_multiplier();

  a -> weapon_multiplier = tmp_weapon_multiplier;

  if( ! p -> active_deep_wounds ) p -> active_deep_wounds = new deep_wounds_t( p );

  if( p -> active_deep_wounds -> ticking )
  {
    int num_ticks = p -> active_deep_wounds -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_deep_wounds -> current_tick;

    dmg += p -> active_deep_wounds -> base_tick_dmg * remaining_ticks;

    p -> active_deep_wounds -> cancel();
  }
  p -> active_deep_wounds -> base_tick_dmg = dmg / 6.0;
  p -> active_deep_wounds -> execute();
}

// trigger_overpower_activation =============================================

static void trigger_overpower_activation( warrior_t* p )
{
  p -> _buffs.overpower = p -> sim -> current_time + 5.0;
  p -> aura_gain( "Overpower Activation" );
}

// trigger_tier8_2pc ========================================================

static void trigger_tier8_2pc( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();
  if( p -> gear.tier8_2pc == 0 )
    return;

  if( a -> result != RESULT_CRIT )
    return;
    
  if( ! a -> sim -> roll( 0.40 ) ) 
    return;
  
  struct heightened_reflexes_expiration_t : public event_t
  {
    heightened_reflexes_expiration_t( sim_t* sim, warrior_t* p ) : event_t( sim, p )
    {
      name = "Heightened Reflexes (tier8_2pc) Expiration";
      player -> aura_gain( "Heightened Reflexes (tier8_2pc)" );
      player -> haste_rating += 150;
      player -> recalculate_haste();
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Heightened Reflexes" );
      player -> haste_rating -= 150;
      player -> recalculate_haste();
      player -> expirations.tier8_2pc = 0;
    }
  };

  p -> procs.tier8_2pc -> occur();

  event_t*& e = p -> expirations.tier8_2pc;

  if( e )
  {
    e -> reschedule( 5.0 );
  }
  else
  {
    e = new ( a -> sim ) heightened_reflexes_expiration_t( a -> sim, p );
  }
}

// trigger_trauma ===========================================================

static void trigger_trauma( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();
  if( p -> talents.trauma == 0 )
    return;

  if( a -> result != RESULT_CRIT )
    return;
  
  struct trauma_expiration_t : public event_t
  {
    trauma_expiration_t( sim_t* sim, warrior_t* p ) : event_t( sim, p )
    {
      name = "Trauma Expiration";
      sim -> target -> debuffs.trauma++;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      warrior_t* p = player -> cast_warrior();
      sim -> target -> debuffs.trauma--;
      p -> _expirations.trauma = 0;
    }
  };

  event_t*& e = p -> _expirations.trauma;

  if( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new ( a -> sim ) trauma_expiration_t( a -> sim, p );
  }
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

  c -= p -> buffs.tier7_4pc;
  if( c < 0 ) 
    return 0;

  return c;
}
// warrior_attack_t::consume_resource ========================================

void warrior_attack_t::consume_resource()
{
  attack_t::consume_resource();

  // Warrior attacks which are are avoided by the target consume only 20%
  // Only Exception are AoE attacks like Whirlwind/Bladestorm 
  // result != RESULT_NONE is needed so the cost is not reduced when the sim 
  // checks all actions if they are ready base on resource cost.
  if( aoe_attack )
    return;
  
  if( result_is_hit() )
    return;

  warrior_t* p = player -> cast_warrior();  
  double rage_restored = resource_consumed * 0.80;
  p -> resource_gain( RESOURCE_RAGE, rage_restored, p -> gains_avoided_attacks );

}
// warrior_attack_t::execute =================================================

void warrior_attack_t::execute()
{
  attack_t::execute();
  event_t::early( player -> expirations.tier7_4pc );

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
  if( p -> _buffs.recklessness > 0 && special )
    p -> _buffs.recklessness--;
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
        player_crit_multiplier *= 1 + p -> talents.poleaxe_specialization * 0.01;
      }
    }
    if( p -> talents.dual_wield_specialization )
    {
      if( weapon -> slot == SLOT_OFF_HAND )
      {
        player_multiplier *= 1.0 + p -> talents.dual_wield_specialization * 0.05;
      }
    }
    if( weapon -> group() == WEAPON_2H )
    {
      player_multiplier *= 1.0 + p -> talents.twohanded_weapon_specialization * 0.02;
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
  
  if( p -> _buffs.recklessness > 0 && special)
    player_crit += 1.0;
  
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
  virtual double haste()
  {
    warrior_t* p = player -> cast_warrior();

    double h = warrior_attack_t::haste();

    h *= 1.0 / ( 1.0 + p -> talents.blood_frenzy * 0.03);

    return h;
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
   
    warrior_t* p = player -> cast_warrior();

    if( p -> _buffs.flurry > 0 )
    {
      p -> _buffs.flurry--;
      if( p -> _buffs.flurry == 0) p -> aura_loss( "Flurry" );
    }
    if( weapon -> slot == SLOT_MAIN_HAND )
    {
      // We can't rely on the resource check from the time we queued the HS.
      // Easily possible that at the time we reach the MH hit, we have spent
      // all the rage and therefor have to check again if we have enough rage.
      if( p -> active_heroic_strike && p -> resource_current[ RESOURCE_RAGE ] < p -> active_heroic_strike -> cost() )
      {
        p -> active_heroic_strike -> cancel();
      }
      p -> uptimes_heroic_strike -> update( p -> active_heroic_strike != 0 );
      if( p -> active_heroic_strike  )
      {
        p -> active_heroic_strike -> execute();
        //trigger_trauma( p -> active_heroic_strike );
        schedule_execute();
        return;
      }
    }

    warrior_attack_t::execute();
    
    trigger_trauma( this );
    
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
    weapon          = &( p -> main_hand_weapon );
    base_cost      -= p -> talents.improved_heroic_strike;
    trigger_gcd     = 0;
    
    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = false;

    // Heroic Strike needs swinging auto_attack!
    assert( p -> main_hand_attack -> execute_event == 0 );
    observer = &( p -> active_heroic_strike );
  }

  virtual void schedule_execute()
  {
    // We don't actually create a event to execute Heroic Strike instead of the 
    // MH attack.
    if( observer ) *observer = this;
    if( sim -> log ) report_t::log( sim, "%s queues %s", player -> name(), name() );
    player -> schedule_ready( 0 );
  }
  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    warrior_attack_t::execute();

    trigger_tier8_2pc( this );
    if( result == RESULT_CRIT )
      if( p -> glyphs.heroic_strike )
        p -> resource_gain( RESOURCE_RAGE, 10.0, p -> gains_glyph_of_heroic_strike );
  }
  virtual bool ready()
  {
    if( ! warrior_attack_t::ready() )
      return false;

    warrior_t* p = player -> cast_warrior();
    // Allready queued up heroic strike?
    if( p -> active_heroic_strike )
      return false;
      
    return true;
  }  
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
    base_multiplier  *= 1 + p -> talents.unending_fury * 0.02;
    direct_power_mod  = 0.50;
    if( p -> gear.tier8_4pc )
      base_crit += 0.10;

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
  }
  virtual void execute()
  {
    warrior_attack_t::execute();
    trigger_bloodsurge( this );
  }
};

// Execute ===================================================================

struct execute_t : public warrior_attack_t
{
  double excess_rage_mod, excess_rage;
  execute_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "execute",  player, SCHOOL_PHYSICAL, TREE_FURY )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    static rank_t ranks[] =
    {
      { 80, 8, 1456, 1456, 0, 15 },
      { 73, 7, 1142, 1142, 0, 15 },
      { 70, 6,  865,  865, 0, 15 },
      { 65, 5,  687,  687, 0, 15 },
      { 56, 4,  554,  554, 0, 15 },
      { 0, 0 }
    };
    init_rank( ranks );
    
    excess_rage_mod   = ( p -> level >= 80 ? 38 :
                          p -> level >= 73 ? 30 :
                          p -> level >= 70 ? 21 :
                          p -> level >= 65 ? 18 :
                                             15 ); 
    base_cost        -= util_t::talent_rank( p -> talents.improved_execute, 2, 2, 5);
    direct_power_mod  = 0.20;
    
    stancemask = STANCE_BATTLE | STANCE_BERSERKER;
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
  }
  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    excess_rage = ( p -> resource_current[ RESOURCE_RAGE ] - warrior_attack_t::cost() );
    if( p -> glyphs.execution )
      excess_rage += 10;
      
    // FIX ME! Implement Sudden Death somehow over here
    if( excess_rage > 0 )
    {
      base_dd_max      += excess_rage_mod * excess_rage;
      base_dd_min      += excess_rage_mod * excess_rage;
    }
    
    if( p -> glyphs.execution )
      excess_rage -= 10;
    
    warrior_attack_t::execute();
  }
  
  virtual void consume_resource()
  {
    warrior_attack_t::consume_resource();

    // Let the additional rage consumption create it's own debug log entries.
    // FIX ME! Does a missed execute consume the excess_rage?
    if( result_is_hit() )
    {
      if( sim -> debug )
        report_t::log( sim, "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_rage, util_t::resource_type_string( resource ), name() );

      player -> resource_loss( resource, excess_rage );
      stats -> consume_resource( excess_rage );
    }

  }
  
  virtual bool ready()
  {
    if( ! warrior_attack_t::ready() )
      return false;
      
    warrior_t* p = player -> cast_warrior();
    
    if( sim -> target -> health_percentage() > 20 && p -> _expirations.sudden_death == 0 )
      return false;
      
    return true;

  }
};

// Mortal Strike =============================================================

struct mortal_strike_t : public warrior_attack_t
{
  mortal_strike_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "mortal_strike",  player, SCHOOL_PHYSICAL, TREE_ARMS )
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
    
    cooldown         = 6.0 - ( p -> talents.improved_mortal_strike / 3.0 );
    base_multiplier *= 1 + util_t::talent_rank( p -> talents.improved_mortal_strike, 3, 0.03, 0.06, 0.10)
                         + ( p -> glyphs.mortal_strike ? 0.10 : 0 );
    if( p -> gear.tier8_4pc )
      base_crit += 0.10;

    weapon_multiplier = 1;
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

    base_cost       = 5.0;
    base_crit      += p -> talents.improved_overpower * 0.25;
    base_direct_dmg = 1;
    cooldown        = 5.0 - p -> talents.unrelenting_assault * 2.0;

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

// Rend ======================================================================

struct rend_t : public warrior_attack_t
{
  rend_t( player_t* player, const std::string& options_str ) : 
    warrior_attack_t( "rend",  player, SCHOOL_BLEED, TREE_ARMS )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    static rank_t ranks[] =
    {
      { 76, 10, 0, 0, 380, 10 },
      { 71,  9, 0, 0, 315, 10 },
      { 68,  8, 0, 0, 215, 10 },
      { 60,  7, 0, 0, 185, 10 },
      { 0, 0 }
    };
    init_rank( ranks );


    base_cost         = 10.0;
    base_tick_time    = 3.0;
    num_ticks         = 5 + ( p -> glyphs.rending ? 2 : 0 );
    base_multiplier  *= 1 + p -> talents.improved_rend * 0.10;
    
    stancemask = STANCE_BATTLE | STANCE_DEFENSE;
    
    weapon_multiplier = 1.0 / 5.0; // 1/5th of weapon_damage per tick.
    weapon = &( p -> main_hand_weapon );

  }
  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    // If used while the victim is above 75% health, Rend does 35% more damage.
    if( sim -> target -> health_percentage() > 75 )
      player_multiplier *= 1.35;
  }
  
  virtual void tick()
  {
    warrior_attack_t::tick();
    warrior_t* p = player -> cast_warrior();
    trigger_tier7_4pc( this );
    if( sim -> roll ( p -> talents.taste_for_blood * 0.10 ) )
    {
      trigger_overpower_activation( p );
      p -> procs_taste_for_blood -> occur();
    }
  }
  virtual void execute()
  {
    weapon_multiplier = 0; // Else there would be a direct damage component
    warrior_attack_t::execute();
    weapon_multiplier = 1.0 / 5.0; // 1/5th of weapon_damage per tick.
    trigger_blood_frenzy( this );
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
    base_multiplier   *= 1 + p -> talents.unending_fury * 0.02 + ( p -> gear.tier7_2pc ? 0.10 : 0.0 );

    normalize_weapon_speed = false;
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

    trigger_tier8_2pc( this );
        
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

    aoe_attack = true;
    stancemask = STANCE_BERSERKER;

    weapon = &( p -> main_hand_weapon );
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    // OH hit
    weapon = &( player -> off_hand_weapon );
    if( weapon )
    {
    /* Special case for Recklessness + Whirlwind + Dualwield
    *  Every hand uses one charge, _BUT_ if only one charge 
    *  is left on recklessness, both hands will crit.
    *  So we deal with offhand first and increase charges by
    *  if only one is left, so mainhand also gets one.
    */
      if( p -> _buffs.recklessness == 1 && special )
        p -> _buffs.recklessness++;
      warrior_attack_t::execute();
      trigger_bloodsurge( this );
    }
    // MH hit
    weapon = &( player -> main_hand_weapon );
    warrior_attack_t::execute();
    trigger_bloodsurge( this );

    warrior_attack_t::consume_resource();

    player -> action_finish( this );
  }
};


// =========================================================================
// Warrior Spells
// =========================================================================

struct warrior_spell_t : public spell_t
{
  double min_rage, max_rage;
  int stancemask;
  warrior_spell_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE ) : 
    spell_t( n, player, RESOURCE_RAGE, s, t ), 
    min_rage(0), max_rage(0), 
    stancemask(STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE)
  {
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost();
  virtual void   execute();
  virtual bool   ready();
};

// warrior_spell_t::parse_options ===========================================

void warrior_spell_t::parse_options( option_t*          options,
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
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// warrior_spell_t::execute ==================================================

void warrior_spell_t::execute()
{
  spell_t::execute();
  // As it seems tier7 4pc is consumed by everything, no matter if it costs
  // rage. "Reduces the rage cost of your next ability is reduced by 5."
  event_t::early( player -> expirations.tier7_4pc );
}

// warrior_spell_t::cost =====================================================

double warrior_spell_t::cost()
{
  warrior_t* p = player -> cast_warrior();
  double c = spell_t::cost();
  if( harmful )
    c -= p -> talents.focused_rage;

  c -= p -> buffs.tier7_4pc;

  if( c < 0 ) 
    return 0;

  return c;
}

// warrior_spell_t::ready() ==================================================

bool warrior_spell_t::ready()
{
  if( ! spell_t::ready() )
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

// Stance ==================================================================

struct stance_t : public warrior_spell_t
{
  int switch_to_stance;
  stance_t( player_t* player, const std::string& options_str ) : 
    warrior_spell_t( "stance", player ), switch_to_stance(0)
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
    harmful     = false;
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

// Berserker Rage ==========================================================

struct berserker_rage_t : public warrior_spell_t
{
  berserker_rage_t( player_t* player, const std::string& options_str ) : 
    warrior_spell_t( "berserker_rage", player )
  {
    warrior_t* p = player -> cast_warrior();

    std::string stance_str;
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    base_cost   = 0;
    trigger_gcd = 0;
    cooldown    = 30 * ( 1.0 - 0.11 * p -> talents.intensify_rage );;
    harmful     = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    
    warrior_t* p = player -> cast_warrior();      
    p -> resource_gain( RESOURCE_RAGE, 10 * p -> talents.improved_berserker_rage , p -> gains_berserker_rage );
  }
};

// Bloodrage ===============================================================

struct bloodrage_t : public warrior_spell_t
{
  bloodrage_t( player_t* player, const std::string& options_str ) : 
    warrior_spell_t( "bloodrage", player )
  {
    warrior_t* p = player -> cast_warrior();

    std::string stance_str;
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    base_cost   = 0;
    trigger_gcd = 0;
    cooldown    = 60 * ( 1.0 - 0.11 * p -> talents.intensify_rage );;
    harmful     = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    
    warrior_t* p = player -> cast_warrior();      
    p -> resource_gain( RESOURCE_RAGE, 10 * (1 + p -> talents.improved_bloodrage * 0.25), p -> gains_bloodrage );
    p -> _buffs.bloodrage = 10;
    struct bloodrage_buff_t : public event_t
    {
      bloodrage_buff_t( sim_t* sim, player_t* player) : event_t( sim, player)
      {
        name = "Bloodrage Buff";
        sim -> add_event( this, 1.0 );
      }
      virtual void execute()
      {
        warrior_t* p = player -> cast_warrior();
        p -> resource_gain( RESOURCE_RAGE, 1 + p -> talents.improved_bloodrage * 0.25, p -> gains_bloodrage );
        p -> _buffs.bloodrage--;
        if( p -> _buffs.bloodrage > 0)
        {
          new ( sim ) bloodrage_buff_t( sim, p );
        }
      }
    };
    new ( sim ) bloodrage_buff_t( sim, p );
  }
};

// Death Wish ==============================================================

struct death_wish_t : public warrior_spell_t
{
  death_wish_t( player_t* player, const std::string& options_str ) : 
    warrior_spell_t( "death_wish", player )
  {
    warrior_t* p = player -> cast_warrior();

    std::string stance_str;
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    base_cost   = 10;
    trigger_gcd = 0;
    cooldown    = 180.0 * ( 1.0 - 0.11 * p -> talents.intensify_rage );
    harmful     = false;
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

// Recklessness ============================================================

struct recklessness_t : public warrior_spell_t
{
  recklessness_t( player_t* player, const std::string& options_str ) : 
    warrior_spell_t( "recklessness", player )
  {
    warrior_t* p = player -> cast_warrior();

    std::string stance_str;
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    cooldown    = 300.0 * ( 1.0 - 0.11 * p -> talents.intensify_rage );
    harmful     = false;
    
    stancemask  = STANCE_BERSERKER;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, warrior_t* p ) : event_t( sim, p )
      {
        name = "Recklessness Expiration";
        p -> aura_gain( "Recklessness" );
        p -> _buffs.recklessness = 3;
        sim -> add_event( this, 12.0 );
      }
      virtual void execute()
      {
        warrior_t* p = player -> cast_warrior();
        p -> aura_loss( "Recklessness" );
        p -> _buffs.recklessness = 0;
        p -> _expirations.recklessness = 0;
      }
    };

    warrior_t* p = player -> cast_warrior();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> _expirations.recklessness = new ( sim ) expiration_t( sim, p );
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
  if( name == "berserker_rage"      ) return new berserker_rage_t     ( this, options_str );
  if( name == "bloodrage"           ) return new bloodrage_t          ( this, options_str );
  if( name == "bloodthirst"         ) return new bloodthirst_t        ( this, options_str );
  if( name == "death_wish"          ) return new death_wish_t         ( this, options_str );
  if( name == "execute"             ) return new execute_t            ( this, options_str );
  if( name == "heroic_strike"       ) return new heroic_strike_t      ( this, options_str );
  if( name == "mortal_strike"       ) return new mortal_strike_t      ( this, options_str );
  if( name == "overpower"           ) return new overpower_t          ( this, options_str );
  if( name == "recklessness"        ) return new recklessness_t       ( this, options_str );
  if( name == "rend"                ) return new rend_t               ( this, options_str );
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
  base_attack_expertise = 0.25 * talents.vitality * 0.02;  
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
  // FIX ME! Is this approach better, or a repeating 3sec timer which gives
  // 1.0 rage.
  if( talents.anger_management )
  {
    if( sim -> infinite_resource[ RESOURCE_RAGE ] == 0 )
    {
      double rage_regen = 1.0 / 3.0; // 1 rage per 3 sec.
      rage_regen *= periodicity;
      resource_gain( RESOURCE_RAGE, rage_regen, gains_anger_management );
    }
  }
  uptimes_rage_cap -> update( resource_current[ RESOURCE_RAGE ] ==
                              resource_max    [ RESOURCE_RAGE] );
   
}

// warrior_t::get_talent_trees ==============================================

bool warrior_t::get_talent_trees( std::vector<int*>& arms,
                                std::vector<int*>& fury,
                                std::vector<int*>& protection )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.improved_heroic_strike          ) }, {  1, &( talents.armored_to_the_teeth      ) }, {  1, &( talents.improved_bloodrage         ) } },
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
    { { 30, &( talents.bladestorm                      ) }, {  0, NULL                                   }, {  0, NULL                                    } },
    { {  0, NULL                                         }, {  0, NULL                                   }, {  0, NULL                                    } }
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
    { "armored_to_the_teeth",            OPT_INT, &( talents.armored_to_the_teeth            ) },
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
