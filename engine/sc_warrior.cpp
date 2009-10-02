// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Warrior
// ==========================================================================

enum warrior_stance { STANCE_BATTLE=1, STANCE_BERSERKER, STANCE_DEFENSE=4 };

struct warrior_t : public player_t
{
  // Active
  action_t* active_damage_shield;
  action_t* active_deep_wounds;
  int       active_stance;

  std::vector<action_t*> active_heroic_strikes;
  int num_active_heroic_strikes;

  // Buffs
  buff_t* buffs_bloodrage;
  buff_t* buffs_bloodsurge;
  buff_t* buffs_death_wish;
  buff_t* buffs_enrage;
  buff_t* buffs_flurry;
  buff_t* buffs_improved_defensive_stance;
  buff_t* buffs_overpower;
  buff_t* buffs_recklessness;
  buff_t* buffs_revenge;
  buff_t* buffs_shield_block;
  buff_t* buffs_sudden_death;
  buff_t* buffs_sword_and_board;
  buff_t* buffs_taste_for_blood;
  buff_t* buffs_wrecking_crew;
  buff_t* buffs_tier7_4pc_melee;
  buff_t* buffs_tier8_2pc_melee;
  buff_t* buffs_glyph_of_blocking;
  buff_t* buffs_glyph_of_revenge;

  // Cooldowns
  struct _cooldowns_t
  {
    double sword_specialization;
    double shield_slam;
    void reset() { memset( ( void* ) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Gains
  gain_t* gains_anger_management;
  gain_t* gains_avoided_attacks;
  gain_t* gains_bloodrage;
  gain_t* gains_berserker_rage;
  gain_t* gains_glyph_of_heroic_strike;
  gain_t* gains_incoming_damage;
  gain_t* gains_mh_attack;
  gain_t* gains_oh_attack;
  gain_t* gains_shield_specialization;
  gain_t* gains_sudden_death;
  gain_t* gains_unbridled_wrath;

  // Procs
  proc_t* procs_glyph_overpower;
  proc_t* procs_parry_haste;
  proc_t* procs_sword_specialization;

  // Up-Times
  uptime_t* uptimes_rage_cap;

  // Random Number Generation
  rng_t* rng_improved_defensive_stance;
  rng_t* rng_shield_specialization;
  rng_t* rng_sword_specialization;
  rng_t* rng_unbridled_wrath;

  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  struct talents_t
  {
    int anticipation;           
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
    int damage_shield;
    int death_wish;
    int deep_wounds;
    int deflection;
    int devastate;
    int dual_wield_specialization;
    int endless_rage;
    int enrage;
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
    int improved_revenge;
    int improved_slam;
    int improved_spell_reflection;
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
    int rampage;
    int shield_mastery;
    int shield_specialization;
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
    int vigilance;
    int vitality;
    int weapon_mastery;
    int wrecking_crew;
    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int bladestorm;
    int blocking;
    int execution;
    int heroic_strike;
    int mortal_strike;
    int overpower;
    int rending;
    int revenge;
    int whirlwind;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  warrior_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, WARRIOR, name, race_type )
  {
    // Active
    active_damage_shield = 0;
    active_deep_wounds   = 0;
    active_stance        = STANCE_BATTLE;

    // Auto-Attack
    main_hand_attack = 0;
    off_hand_attack  = 0;
  }

  // Character Definition
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      combat_begin();
  virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_block_value() SC_CONST;
  virtual double    composite_tank_miss( int school ) SC_CONST;
  virtual double    composite_tank_block() SC_CONST;
  virtual void      reset();
  virtual void      interrupt();
  virtual void      regen( double periodicity );
  virtual double    resource_loss( int resurce, double amount, action_t* );
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_RAGE; }
  virtual int       primary_role() SC_CONST     { return ROLE_ATTACK; }
  virtual int       primary_tree() SC_CONST;
  virtual int       target_swing();
};

// warrior_t::composite_attribute_multiplier ===============================

double warrior_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );
  if ( attr == ATTR_STRENGTH )
  {
    if ( active_stance == STANCE_BERSERKER )
    {
      m *= 1 + talents.improved_berserker_stance * 0.04;
    }
  }
  return m;
}

namespace   // ANONYMOUS NAMESPACE =========================================
{

// ==========================================================================
// Warrior Attack
// ==========================================================================

struct warrior_attack_t : public attack_t
{
  double min_rage, max_rage;
  int stancemask;
  warrior_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true  ) :
      attack_t( n, player, RESOURCE_RAGE, s, t, special ),
      min_rage( 0 ), max_rage( 0 ),
      stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )

  {
    warrior_t* p = player -> cast_warrior();
    may_glance   = false;
    base_hit    += p -> talents.precision * 0.01;
    base_crit   += p -> talents.cruelty   * 0.01;
    normalize_weapon_speed = true;
    if ( special )
      base_crit_bonus_multiplier *= 1.0 + p -> talents.impale * 0.10;

  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double dodge_chance( int delta_level ) SC_CONST;
  virtual void   consume_resource();
  virtual double cost() SC_CONST;
  virtual void   execute();
  virtual void   player_buff();
  virtual bool   ready();
};

double warrior_attack_t::dodge_chance( int delta_level ) SC_CONST
{
  double chance = attack_t::dodge_chance( delta_level );

  warrior_t* p = player -> cast_warrior();

  chance -= p -> talents.weapon_mastery * 0.01;
  if ( chance < 0 )
    return 0;

  return chance;
}

// ==========================================================================
// Static Functions
// ==========================================================================

// trigger_blood_frenzy =====================================================

static void trigger_blood_frenzy( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();
  if ( p -> talents.blood_frenzy == 0 )
    return;

  target_t* t = a -> sim -> target;

  // Don't alter the duration if it is set to 0 (override/optimal_raid)
  if ( t -> debuffs.blood_frenzy -> duration > 0 )
    t -> debuffs.blood_frenzy -> duration = a -> num_ticks * a -> base_tick_time;

  double value    = p -> talents.blood_frenzy * 2;

  if( value >= t -> debuffs.blood_frenzy -> current_value )
  {
    t -> debuffs.blood_frenzy -> trigger( 1, value );
  }
}

// trigger_deep_wounds ======================================================

static void trigger_deep_wounds( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> talents.deep_wounds )
    return;

  if ( a -> result != RESULT_CRIT )
    return;

  struct deep_wounds_t : public warrior_attack_t
  {
    deep_wounds_t( warrior_t* p ) :
        warrior_attack_t( "deep_wounds", p, SCHOOL_BLEED, TREE_ARMS )
    {
      background = true;
      trigger_gcd = 0;
      base_tick_time = 1.0;
      num_ticks = 6;
      weapon_multiplier = p -> talents.deep_wounds * 0.16;
    }
    virtual double total_multiplier() SC_CONST
    {
      target_t* t = sim -> target;
      return ( t -> debuffs.mangle -> up() || t -> debuffs.trauma -> up() ) ? 1.30 : 1.0;
    }
    virtual void execute()
    {
      player_buff();
      double damage = calculate_weapon_damage();
      if( ticking ) 
      {
	damage += base_td * ( num_ticks - current_tick );
	cancel();
      }
      base_td = damage / 6.0;
      trigger_blood_frenzy( this );
      schedule_tick();
    }
    virtual void tick()
    {

      warrior_attack_t::tick();
      warrior_t* p = player -> cast_warrior();
      p -> buffs_tier7_4pc_melee -> trigger();
    }
  };

  if ( ! p -> active_deep_wounds ) p -> active_deep_wounds = new deep_wounds_t( p );

  // Every action HAS to have an weapon associated.
  assert( a -> weapon != 0 );

  p -> active_deep_wounds -> weapon = a -> weapon;
  p -> active_deep_wounds -> execute();
}

// trigger_rage_gain ========================================================

static void trigger_rage_gain( attack_t* a, double rage_conversion_value )
{
  // Basic Formula:      http://forums.worldofwarcraft.com/thread.html?topicId=17367760070&sid=1&pageNo=1
  // Blue Clarification: http://forums.worldofwarcraft.com/thread.html?topicId=17367760070&sid=1&pageNo=13#250

  warrior_t* p = a -> player -> cast_warrior();
  weapon_t*  w = a -> weapon;

  double hit_factor = 3.5;
  if ( a -> result == RESULT_CRIT ) hit_factor *= 2.0;
  if ( w -> slot == SLOT_OFF_HAND ) hit_factor /= 2.0;

  double rage_from_damage = 7.5 * a -> direct_dmg / rage_conversion_value;
  double rage_from_hit    = w -> swing_time * hit_factor;

  double rage_gain_avg = ( rage_from_damage + rage_from_hit ) / 2.0;
  double rage_gain_max = 15 * a -> direct_dmg / rage_conversion_value;

  double rage_gain = std::min( rage_gain_avg, rage_gain_max );

  if ( p -> talents.endless_rage ) rage_gain *= 1.25;

  p -> resource_gain( RESOURCE_RAGE, rage_gain, w -> slot == SLOT_OFF_HAND ? p -> gains_oh_attack : p -> gains_mh_attack );
}

// trigger_sword_specialization =============================================

static void trigger_sword_specialization( attack_t* a )
{
  if ( a -> proc ) return;

  if ( a -> result_is_miss() ) return;

  weapon_t* w = a -> weapon;

  if ( ! w ) return;

  if ( w -> type != WEAPON_SWORD && w -> type != WEAPON_SWORD_2H )
    return;

  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> talents.sword_specialization )
    return;

  if ( ! a -> sim -> cooldown_ready( p -> _cooldowns.sword_specialization ) )
    return;

  if ( p -> rng_sword_specialization -> roll( 0.02 * p -> talents.sword_specialization ) )
  {
    if ( a -> sim -> log )
      log_t::output( a -> sim, "%s gains one extra attack through %s",
                     p -> name(), p -> procs_sword_specialization -> name() );

    p -> procs_sword_specialization -> occur();
    p -> _cooldowns.sword_specialization = a -> sim -> current_time + 6.0;

    p -> main_hand_attack -> proc = true;
    p -> main_hand_attack -> execute();
    p -> main_hand_attack -> proc = false;
  }
}

// trigger_rampage ==========================================================

static void trigger_rampage( attack_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( p -> talents.rampage == 0 )
    return;
  
  p -> sim -> auras.rampage -> trigger();
}

// trigger_sword_and_board ==================================================

static void trigger_sword_and_board( attack_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( p -> buffs_sword_and_board -> trigger() )
  {
    p -> _cooldowns.shield_slam = 0;
  }
}

// trigger_unbridled_wrath ==================================================

static void trigger_unbridled_wrath( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();

  if ( ! p -> talents.unbridled_wrath )
    return;

  double PPM = p -> talents.unbridled_wrath * 3; // 15 ppm @ 5/5
  double chance = a -> weapon -> proc_chance_on_swing( PPM );

  if ( p -> rng_unbridled_wrath -> roll( chance ) )
  {
    p -> resource_gain( RESOURCE_RAGE, 1.0 , p -> gains_unbridled_wrath );
  }
}

// trigger_trauma ===========================================================

static void trigger_trauma( action_t* a )
{
  warrior_t* p = a -> player -> cast_warrior();
  if ( p -> talents.trauma == 0 )
    return;

  if ( a -> result != RESULT_CRIT )
    return;
  
  target_t* t = a -> sim -> target;

  double value = p -> talents.trauma * 15;

  if( value >= t -> debuffs.trauma -> current_value )
  {
    t -> debuffs.trauma -> trigger( 1, value );
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
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// warrior_attack_t::cost ====================================================

double warrior_attack_t::cost() SC_CONST
{
  warrior_t* p = player -> cast_warrior();
  double c = attack_t::cost();
  if ( harmful ) c -= p -> talents.focused_rage;
  if ( p -> buffs_tier7_4pc_melee -> check() ) c -= 5;
  if ( c < 0 ) c = 0;
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
  if ( aoe )
    return;

  if ( result_is_hit() )
    return;

  if ( resource_consumed > 0 )
  {
    warrior_t* p = player -> cast_warrior();
    double rage_restored = resource_consumed * 0.80;
    p -> resource_gain( RESOURCE_RAGE, rage_restored, p -> gains_avoided_attacks );
  }
}
// warrior_attack_t::execute =================================================

void warrior_attack_t::execute()
{
  attack_t::execute();

  warrior_t* p = player -> cast_warrior();
  p -> buffs_tier7_4pc_melee -> expire();


  if ( result_is_hit() )
  {
    p -> buffs_sudden_death -> trigger();

    // Critproccgalore
    if( result == RESULT_CRIT )
    {
      trigger_rampage( this );
      trigger_deep_wounds( this );
      trigger_trauma( this );
      p -> buffs_wrecking_crew -> trigger(1, p -> talents.wrecking_crew * 0.02 );
      p -> buffs_flurry -> trigger( 3 );
    }
  }
  else if ( result == RESULT_DODGE  )
  {
    p -> buffs_overpower -> trigger();
  }
  else if ( result == RESULT_PARRY )
  {
    if ( p -> glyphs.overpower )
    {
      p -> buffs_overpower -> trigger();
      p -> procs_glyph_overpower -> occur();
    }
  }
  if ( special && p -> buffs_recklessness -> check() )
    p -> buffs_recklessness -> decrement();

  trigger_sword_specialization( this );
}

// warrior_attack_t::player_buff ============================================

void warrior_attack_t::player_buff()
{
  attack_t::player_buff();

  warrior_t* p = player -> cast_warrior();

  if ( weapon )
  {
    if ( p -> talents.mace_specialization )
    {
      if ( weapon -> type == WEAPON_MACE ||
           weapon -> type == WEAPON_MACE_2H )
      {
        player_penetration += p -> talents.mace_specialization * 0.03;
      }
    }
    if ( p -> talents.poleaxe_specialization )
    {
      if ( weapon -> type == WEAPON_AXE_2H ||
           weapon -> type == WEAPON_AXE    ||
           weapon -> type == WEAPON_POLEARM )
      {
        player_crit            += p -> talents.poleaxe_specialization * 0.01;
        player_crit_multiplier *= 1 + p -> talents.poleaxe_specialization * 0.01;
      }
    }
    if ( p -> talents.dual_wield_specialization )
    {
      if ( weapon -> slot == SLOT_OFF_HAND )
      {
        player_multiplier *= 1.0 + p -> talents.dual_wield_specialization * 0.05;
      }
    }
    if ( weapon -> group() == WEAPON_2H )
    {
      player_multiplier *= 1.0 + p -> talents.twohanded_weapon_specialization * 0.02;
    }
    if ( weapon -> group() == WEAPON_1H )
    {
      player_multiplier *= 1.0 + p -> talents.onhanded_weapon_specialization * 0.02;
    }
  }
  if ( p -> active_stance == STANCE_BATTLE )
  {
    player_penetration += 0.10;
    if ( player -> set_bonus.tier9_2pc_melee() ) player_penetration += 0.06;
  }
  else if ( p -> active_stance == STANCE_BERSERKER )
  {
    player_crit += 0.03;
    if ( player -> set_bonus.tier9_2pc_melee() ) player_crit += 0.02;

  }
  else if ( p -> active_stance == STANCE_DEFENSE )
  {
    player_multiplier *= 1.0 - 0.05;
  }

  player_multiplier *= 1.0 + p -> buffs_death_wish -> value();
  player_multiplier *= 1.0 + std::max( p -> buffs_enrage        -> value(), 
				       p -> buffs_wrecking_crew -> value() );

  if ( p -> talents.titans_grip && p -> dual_wield() )
  {
    if ( p -> main_hand_attack -> weapon -> group() == WEAPON_2H || 
	 p ->  off_hand_attack -> weapon -> group() == WEAPON_2H ) 
    {
      player_multiplier *= 0.90;
    }
  }

  if ( p -> buffs_improved_defensive_stance -> up() )
  {
    player_multiplier *= 1.0 + 0.05 * p -> talents.improved_defensive_stance;
  }

  if ( special && p -> buffs_recklessness -> up() )
    player_crit += 1.0;

  if ( sim -> debug )
    log_t::output( sim, "warrior_attack_t::player_buff: %s hit=%.2f expertise=%.2f crit=%.2f crit_multiplier=%.2f",
                   name(), player_hit, player_expertise, player_crit, player_crit_multiplier );
}

// warrior_attack_t::ready() ================================================

bool warrior_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  warrior_t* p = player -> cast_warrior();

  if ( min_rage > 0 )
    if ( p -> resource_current[ RESOURCE_RAGE ] < min_rage )
      return false;

  if ( max_rage > 0 )
    if ( p -> resource_current[ RESOURCE_RAGE ] > max_rage )
      return false;

  // Attack vailable in current stance?
  if ( ( stancemask & p -> active_stance ) == 0 )
    return false;

  return true;
}

// Melee Attack ============================================================

struct melee_t : public warrior_attack_t
{
  double rage_conversion_value;
  melee_t( const char* name, player_t* player ) :
      warrior_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, false ), rage_conversion_value( 0 )
  {
    warrior_t* p = player -> cast_warrior();

    base_dd_min = base_dd_max = 1;

    may_glance      = true;
    may_crit        = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;

    normalize_weapon_speed = false;

    if ( p -> dual_wield() ) base_hit -= 0.19;

    // Rage Conversion Value, needed for: damage done => rage gained
    if ( p -> level == 80 )
      rage_conversion_value = 453.3;
    else
      rage_conversion_value = 0.0091107836 * p -> level * p -> level + 3.225598133* p -> level + 4.2652911;
  }
  virtual double haste() SC_CONST
  {
    warrior_t* p = player -> cast_warrior();

    double h = warrior_attack_t::haste();

    h *= 1.0 / ( 1.0 + p -> talents.blood_frenzy * 0.05 );

    return h;
  }

  virtual double execute_time() SC_CONST
  {
    double t = warrior_attack_t::execute_time();
    warrior_t* p = player -> cast_warrior();
    if ( p -> buffs_flurry -> up() )
    {
      t *= 1.0 / ( 1.0 + 0.05 * p -> talents.flurry );
    }
    return t;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    p -> buffs_flurry -> decrement();

    // If any of our Heroic Strike actions are "ready", then execute HS in place of the regular melee swing.
    action_t* active_heroic_strike = 0;

    if ( weapon -> slot == SLOT_MAIN_HAND && ! proc )
    {
      for( int i=0; i < p -> num_active_heroic_strikes; i++ )
      {
	action_t* a = p -> active_heroic_strikes[ i ];
	if ( a -> ready() )
        {
	  active_heroic_strike = a;
	  break;
	}
      }
    }

    if ( active_heroic_strike )
    {
      active_heroic_strike -> execute();
      schedule_execute();
    }
    else
    {
      warrior_attack_t::execute();
      if ( result_is_hit() )
      {
	trigger_rage_gain( this, rage_conversion_value );
	trigger_unbridled_wrath( this );
      }
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

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", player );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    p -> main_hand_attack -> schedule_execute();
    if ( p -> off_hand_attack ) p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Bladestorm ==============================================================

struct bladestorm_tick_t : public warrior_attack_t
{
  bladestorm_tick_t( player_t* player ) :
      warrior_attack_t( "bladestorm", player, SCHOOL_PHYSICAL, TREE_ARMS, false )
  {
    base_dd_min = base_dd_max = 1;
    dual        = true;
    background  = true;
    may_crit    = true;
  }
  virtual void execute()
  {
    warrior_attack_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );
  }
};

struct bladestorm_t : public warrior_attack_t
{
  attack_t* bladestorm_tick;

  bladestorm_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "bladestorm", player, SCHOOL_PHYSICAL, TREE_ARMS )
  {
    warrior_t* p = player -> cast_warrior();
    check_talent( p -> talents.bladestorm );
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful   = false;
    base_cost = 25;
    cooldown  = 90;

    num_ticks      = 6;
    base_tick_time = 1.0;
    channeled      = true;
    tick_zero      = true;

    if ( p -> glyphs.bladestorm )
      cooldown -= 15;

    bladestorm_tick = new bladestorm_tick_t( p );
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    bladestorm_tick -> weapon = &( player -> main_hand_weapon );
    bladestorm_tick -> execute();

    if ( bladestorm_tick -> result_is_hit() )
    {
      if ( player -> off_hand_weapon.type != WEAPON_NONE )
      {
        bladestorm_tick -> weapon = &( player -> off_hand_weapon );
        bladestorm_tick -> execute();
      }
    }

    update_time( DMG_OVER_TIME );
  }

  // Bladestorm not modified by haste effects
  virtual double haste() SC_CONST { return 1.0; }
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
      { NULL, OPT_UNKNOWN, NULL }
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
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    background   = true;
    may_crit     = true;    
    base_cost   -= p -> talents.improved_heroic_strike;
    base_crit   += p -> talents.incite * 0.05;
    trigger_gcd  = 0;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = false;

    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;

    p -> active_heroic_strikes.push_back( this );
  }

  virtual double cost() SC_CONST
  {
    warrior_t* p = player -> cast_warrior();
    if ( p -> buffs_glyph_of_revenge -> up() ) return 0;
    return warrior_attack_t::cost();
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    warrior_attack_t::execute();
    p -> buffs_glyph_of_revenge -> expire();
    if( result_is_hit() )
    {
      trigger_unbridled_wrath( this );
      p -> buffs_bloodsurge -> trigger();
      if ( result == RESULT_CRIT )
      {
        p -> buffs_tier8_2pc_melee -> trigger();
        if ( p -> glyphs.heroic_strike )
        {
          p -> resource_gain( RESOURCE_RAGE, 10.0, p -> gains_glyph_of_heroic_strike );
        }
      }
    }
  }
};

// Bloodthirst ===============================================================

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "bloodthirst",  player, SCHOOL_PHYSICAL, TREE_FURY )
  {
    warrior_t* p = player -> cast_warrior();
    check_talent( p -> talents.bloodthirst );
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    base_dd_min = base_dd_max = 1;

    may_crit = true;
    cooldown          = 4.0;
    base_cost         = 20;
    base_multiplier *= 1 + p -> talents.unending_fury * 0.02;
    direct_power_mod = 0.50;

    if ( p -> set_bonus.tier8_4pc_melee() ) base_crit += 0.10;
  }
  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();
    if( result_is_hit() ) p -> buffs_bloodsurge -> trigger();
  }
};

// Concussion Blow ===============================================================

struct concussion_blow_t : public warrior_attack_t
{
  concussion_blow_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "concussion_blow",  player, SCHOOL_PHYSICAL, TREE_PROTECTION )
  {
    warrior_t* p = player -> cast_warrior();
    check_talent( p -> talents.concussion_blow );
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    base_dd_min = base_dd_max = 1;

    may_crit          = true;
    cooldown          = 30.0;
    base_cost         = 15;
    direct_power_mod  = 0.75;
  }
};

// Shockwave ===============================================================

struct shockwave_t : public warrior_attack_t
{
  shockwave_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "shockwave",  player, SCHOOL_PHYSICAL, TREE_PROTECTION )
  {
    warrior_t* p = player -> cast_warrior();
    check_talent( p -> talents.shockwave );
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    base_dd_min = base_dd_max = 1;

    may_crit          = true;
    cooldown          = 20.0;
    base_cost         = 15;
    direct_power_mod  = 0.75;
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
  }
};

// Devastate ===============================================================

struct devastate_t : public warrior_attack_t
{
  devastate_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "devastate",  player, SCHOOL_PHYSICAL, TREE_PROTECTION )
  {
    warrior_t* p = player -> cast_warrior();
    check_talent( p -> talents.devastate );
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 1010, 1010, 0, 15 },
      { 75, 4,  850,  850, 0, 15 },
      { 70, 3,  560,  560, 0, 15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 1.00;

    may_crit   = true;
    base_cost -= p -> talents.puncture;
    base_crit += p -> talents.sword_and_board * 0.05;

    if ( p -> set_bonus.tier8_2pc_tank() ) base_crit += 0.10;
    if ( p -> set_bonus.tier9_2pc_tank() ) base_multiplier *= 1.05;
  }
  virtual void execute()
  {
    warrior_attack_t::execute();
    if ( result_is_hit() ) trigger_sword_and_board( this );
  }
};

// Revenge ===============================================================

struct revenge_t : public warrior_attack_t
{
  revenge_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "revenge",  player, SCHOOL_PHYSICAL, TREE_PROTECTION )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 1454, 1776, 0, 5 },
      { 70, 8, 855, 1045, 0, 5 },
      { 63, 7, 699, 853, 0, 5 },
      { 60, 6, 643, 785, 0, 5 },
      { 54, 5, 498, 608, 0, 5 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    may_crit          = true;
    cooldown          = 5.0;
    direct_power_mod  = 0.207;
    base_multiplier  *= 1 + p -> talents.improved_revenge * 0.1;
    if ( p -> talents.unrelenting_assault )
    {
      base_multiplier *= 1 + p -> talents.unrelenting_assault * 0.1;
      cooldown -= ( p -> talents.unrelenting_assault * 2 );
    }
    stancemask = STANCE_DEFENSE;
  }
  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    warrior_attack_t::execute();
    p -> buffs_revenge -> expire();
    if ( result_is_hit() ) trigger_sword_and_board( this );
  }
  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();
    if ( ! p -> buffs_revenge -> check() ) return false;
    return warrior_attack_t::ready();
  }
};

// Shield Bash =============================================================

struct shield_bash_t : public warrior_attack_t
{
  shield_bash_t( player_t* player, const std::string& options_str ) :
    warrior_attack_t( "shield_bash", player, SCHOOL_PHYSICAL, TREE_PROTECTION )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = 10;
    cooldown  = 12;

    stancemask = STANCE_DEFENSE | STANCE_BATTLE;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return warrior_attack_t::ready();
  }
};

// Shield Slam ===============================================================

struct shield_slam_t : public warrior_attack_t
{
  int sword_and_board;
  shield_slam_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "shield_slam",  player, SCHOOL_PHYSICAL, TREE_PROTECTION ),
      sword_and_board( 0 )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { "sword_and_board", OPT_BOOL, &sword_and_board },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8, 990, 1040, 0, 20 },
      { 75, 7, 837,  879, 0, 20 },
      { 70, 6, 549,  577, 0, 20 },
      { 66, 5, 499,  523, 0, 20 },
      { 60, 4, 447,  469, 0, 20 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    may_crit         = true;
    direct_power_mod = 0.0;
    base_multiplier *= 1 + ( 0.05 * p -> talents.gag_order +
			     0.10 * p -> set_bonus.tier7_2pc_tank() );

    base_crit += 0.05 * p -> talents.critical_block;
  }
  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    base_dd_adder = p -> composite_block_value();
    warrior_attack_t::execute();
    p -> _cooldowns.shield_slam = sim -> current_time + 6.0;
    p -> buffs_sword_and_board -> expire();
    p -> buffs_glyph_of_blocking -> trigger();
  }

  virtual double cost() SC_CONST
  {
    warrior_t* p = player -> cast_warrior();
    if ( p -> buffs_sword_and_board -> check() ) return 0;
    double c = warrior_attack_t::cost();
    return c;
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    if ( sword_and_board )
      if ( ! p -> buffs_sword_and_board -> check() )
        return false;

    if ( sim -> current_time < p -> _cooldowns.shield_slam )
      return false;

    return warrior_attack_t::ready();
  }

};

// Thunderclap ===============================================================

struct thunderclap_t : public warrior_attack_t
{
  thunderclap_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "thunderclap",  player, SCHOOL_PHYSICAL, TREE_PROTECTION )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 9, 300, 300, 0, 20 },
      { 73, 8, 247, 247, 0, 20 },
      { 67, 7, 123, 123, 0, 20 },
      { 58, 6, 103, 103, 0, 20 },
      { 48, 5, 82, 82, 0, 20 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    aoe               = true;
    may_crit          = true;
    cooldown          = 6.0;
    direct_power_mod  = 0.12;
    may_dodge         = false;
    may_parry         = false;
    may_block         = false;
    base_multiplier  *= 1 + p -> talents.improved_thunderclap * 0.1;
    base_crit        += p -> talents.incite * 0.05;
  }
};

// Execute ===================================================================

struct execute_t : public warrior_attack_t
{
  double excess_rage_mod, excess_rage;
  int sudden_death;
  execute_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "execute",  player, SCHOOL_PHYSICAL, TREE_FURY ),
      excess_rage_mod( 0 ),
      excess_rage( 0 ),
      sudden_death( 0 )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { "sudden_death", OPT_BOOL, &sudden_death },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8, 1456, 1456, 0, 15 },
      { 73, 7, 1142, 1142, 0, 15 },
      { 70, 6,  865,  865, 0, 15 },
      { 65, 5,  687,  687, 0, 15 },
      { 56, 4,  554,  554, 0, 15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    excess_rage_mod   = ( p -> level >= 80 ? 38 :
                          p -> level >= 73 ? 30 :
                          p -> level >= 70 ? 21 :
                          p -> level >= 65 ? 18 :
                          15 );

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    may_crit          = true;
    base_cost        -= util_t::talent_rank( p -> talents.improved_execute, 2, 2, 5 );
    direct_power_mod  = 0.20;

    // Execute consumes rage no matter if it missed or not
    aoe = true;

    stancemask = STANCE_BATTLE | STANCE_BERSERKER;
  }

  virtual void consume_resource()
  {
    warrior_t* p = player -> cast_warrior();

    resource_consumed = cost();

    double max_consumed = 0;
    max_consumed = std::min( p -> resource_current[ RESOURCE_RAGE ], 30.0 );

    double excess_rage = max_consumed - resource_consumed;

    resource_consumed = max_consumed;

    if ( sim -> debug )
      log_t::output( sim, "%s consumes %.1f %s for %s", player -> name(),
                     resource_consumed, util_t::resource_type_string( resource ), name() );

    player -> resource_loss( resource, resource_consumed );

    stats -> consume_resource( resource_consumed );

    if ( p -> glyphs.execution ) excess_rage += 10;

    base_dd_adder = ( excess_rage > 0 ) ? excess_rage_mod * excess_rage : 0.0;

    if ( p -> buffs_sudden_death -> up() )
    {
      double current_rage = p -> resource_current[ RESOURCE_RAGE ];

      if ( current_rage < 10 )
      {
        p -> resource_gain( RESOURCE_RAGE, 10.0 - current_rage, p -> gains_sudden_death );
      }
    }
  }

  virtual void execute()
  {
    warrior_attack_t::execute();
    warrior_t* p = player -> cast_warrior();
    p -> buffs_sudden_death -> expire();
  }
  
  virtual bool ready()
  {
    if ( ! warrior_attack_t::ready() )
      return false;

    warrior_t* p = player -> cast_warrior();

    if ( ! p -> buffs_sudden_death -> check() )
    {
      if ( sim -> target -> health_percentage() > 20 )
        return false;

      if ( sudden_death )
        return false;
    }

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
    check_talent( p -> talents.mortal_strike );
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8, 380, 380, 0, 30 },
      { 75, 7, 320, 320, 0, 30 },
      { 70, 6, 210, 210, 0, 30 },
      { 66, 5, 185, 185, 0, 30 },
      { 60, 4, 160, 160, 0, 30 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    may_crit         = true;
    cooldown         = 6.0 - ( p -> talents.improved_mortal_strike / 3.0 );
    base_multiplier *= 1.0 + ( ( util_t::talent_rank( p -> talents.improved_mortal_strike, 3, 0.03, 0.06, 0.10 ) ) +
			       ( p -> glyphs.mortal_strike ? 0.10 : 0 ) );

    if ( p -> set_bonus.tier8_4pc_melee() ) base_crit += 0.10;

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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );

    may_crit  = true;
    may_dodge = false;
    may_parry = false;
    may_block = false; // The Overpower cannot be blocked, dodged or parried.

    base_dd_min = base_dd_max = 1;

    base_cost        = 5.0;
    base_crit       += p -> talents.improved_overpower * 0.25;
    base_multiplier *= 1.0 + p -> talents.unrelenting_assault * 0.1;
    cooldown         = 5.0 - p -> talents.unrelenting_assault * 2.0;

    if ( p -> talents.unrelenting_assault == 2 )
      trigger_gcd = 1.0;

    stancemask = STANCE_BATTLE;

  }
  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    // Track some information on what got us the overpower
    // Talents or lack of expertise
    p -> buffs_overpower -> up();
    p -> buffs_taste_for_blood -> up();
    warrior_attack_t::execute();
    p -> buffs_overpower -> expire();
    p -> buffs_taste_for_blood -> expire();
  }

  virtual bool ready()
  {
    if ( ! warrior_attack_t::ready() )
      return false;

    warrior_t* p = player -> cast_warrior();

    if ( p -> buffs_overpower -> check() || p -> buffs_taste_for_blood -> check() )
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 76, 10, 0, 0, 76, 10 },
      { 71,  9, 0, 0, 63, 10 },
      { 68,  8, 0, 0, 43, 10 },
      { 60,  7, 0, 0, 37, 10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );

    may_crit          = false;
    base_cost         = 10.0;
    base_tick_time    = 3.0;
    num_ticks         = 5 + ( p -> glyphs.rending ? 2 : 0 );
    base_multiplier  *= 1 + p -> talents.improved_rend * 0.10;

    normalize_weapon_speed = false;

    stancemask = STANCE_BATTLE | STANCE_DEFENSE;


  }
  virtual void player_buff()
  {
    warrior_attack_t::player_buff();
    // If used while the victim is above 75% health, Rend does 35% more damage.
    if ( sim -> target -> health_percentage() > 75 )
      player_multiplier *= 1.35;
  }

  virtual void tick()
  {
    warrior_attack_t::tick();
    warrior_t* p = player -> cast_warrior();
    p -> buffs_tier7_4pc_melee -> trigger();
    p -> buffs_taste_for_blood -> trigger();
  }
  virtual void execute()
  {
    base_td = base_td_init + calculate_weapon_damage() / 5.0;
    warrior_attack_t::execute();
    if ( result_is_hit() ) trigger_blood_frenzy( this );
  }
};

// Slam ====================================================================

struct slam_t : public warrior_attack_t
{
  int bloodsurge;
  slam_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "slam",  player, SCHOOL_PHYSICAL, TREE_FURY ),
      bloodsurge( 0 )
  {
    warrior_t* p = player -> cast_warrior();

    option_t options[] =
    {
      { "bloodsurge", OPT_BOOL, &bloodsurge },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8, 250, 250, 0, 15 },
      { 74, 7, 220, 220, 0, 15 },
      { 69, 6, 140, 140, 0, 15 },
      { 61, 5, 105, 105, 0, 15 },
      { 54, 4,  87,  87, 0, 15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = false;

    may_crit = true;
    base_execute_time  = 1.5 - p -> talents.improved_slam * 0.5;
    base_multiplier   *= 1 + p -> talents.unending_fury * 0.02 + ( p -> set_bonus.tier7_2pc_melee() ? 0.10 : 0.0 );

    if ( player -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
  }
  virtual double haste() SC_CONST
  {
    // No haste for slam cast?
    return 1.0;
  }

  virtual double execute_time() SC_CONST
  {
    warrior_t* p = player -> cast_warrior();
    if ( p -> buffs_bloodsurge -> check() )
      return 0.0;
    else
      return warrior_attack_t::execute_time();
  }
  virtual void execute()
  {
    warrior_attack_t::execute();

    warrior_t* p = player -> cast_warrior();
    p -> buffs_bloodsurge -> expire();
    if ( result == RESULT_CRIT )
    {
      p -> buffs_tier8_2pc_melee -> trigger();
    }
  }

  virtual void schedule_execute()
  {
    warrior_attack_t::schedule_execute();

    warrior_t* p = player -> cast_warrior();

    // While slam is casting, the auto_attack is paused
    // So we simply reschedule the auto_attack by slam's casttime
    double time_to_next_hit;
    // Mainhand
    if ( p -> main_hand_attack )
    {
      time_to_next_hit  = p -> main_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += execute_time();
      p -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
    // Offhand
    if ( p -> off_hand_attack )
    {
      time_to_next_hit  = p -> off_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += execute_time();
      p -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
  }

  virtual bool ready()
  {
    if ( ! warrior_attack_t::ready() )
      return false;

    warrior_t* p = player -> cast_warrior();

    if ( bloodsurge )
    {
      // Player does not instantaneous become aware of the proc
      if ( ! p -> buffs_bloodsurge -> may_react() )
        return false;
    }

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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );

    base_dd_min = base_dd_max = 1;

    may_crit         = true;
    cooldown         = 10.0 - ( p -> glyphs.whirlwind ? 2 : 0 );
    base_cost        = 25;
    base_multiplier *= 1 + p -> talents.improved_whirlwind * 0.10 + p -> talents.unending_fury * 0.02;
    
    aoe  = true;
    stancemask = STANCE_BERSERKER;
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();

    /* Special case for Recklessness + Whirlwind + Dualwield
    *  Every hand uses one charge, _BUT_ if only one charge
    *  is left on recklessness, both hands will crit.
    *  So we deal with offhand first and increase charges by
    *  if only one is left, so offhand also gets one.
    */
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> buffs_recklessness -> stack() == 1 )
        p -> buffs_recklessness -> current_stack++;
    }
    // MH hit
    weapon = &( player -> main_hand_weapon );
    warrior_attack_t::execute();

    if( result_is_hit() ) p -> buffs_bloodsurge -> trigger();

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      weapon = &( player -> off_hand_weapon );
      warrior_attack_t::execute();
      if( result_is_hit() ) p -> buffs_bloodsurge -> trigger();
    }

    warrior_attack_t::consume_resource();
  }
};


// Pummel ==================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( player_t* player, const std::string& options_str ) :
      warrior_attack_t( "pummel",  player, SCHOOL_PHYSICAL, TREE_FURY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    base_cost = 10.0;
    base_dd_min = base_dd_max = 1;
    may_miss = may_resist = may_glance = may_block = may_dodge = may_crit = false;
    base_attack_power_multiplier = 0;
    cooldown = 10;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return warrior_attack_t::ready();
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
      min_rage( 0 ), max_rage( 0 ),
      stancemask( STANCE_BATTLE|STANCE_BERSERKER|STANCE_DEFENSE )
  {}

  virtual double cost() SC_CONST;
  virtual void   execute();
  virtual double gcd() SC_CONST;
  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual bool   ready();
};

// warrior_spell_t::execute ==================================================

void warrior_spell_t::execute()
{
  warrior_spell_t::consume_resource();

  // As it seems tier7 4pc is consumed by everything, no matter if it costs
  // rage. "Reduces the rage cost of your next ability is reduced by 5."
  warrior_t* p = player -> cast_warrior();
  p -> buffs_tier7_4pc_melee -> expire();
  
  update_ready();
}

// warrior_spell_t::cost =====================================================

double warrior_spell_t::cost() SC_CONST
{
  warrior_t* p = player -> cast_warrior();
  double c = spell_t::cost();
  if ( harmful ) c -= p -> talents.focused_rage;
  if ( p -> buffs_tier7_4pc_melee -> check() ) c -= 5;
  if ( c < 0 ) c = 0;
  return c;
}

// warrior_spell_t::gcd ======================================================

double warrior_spell_t::gcd() SC_CONST
{
  // Unaffected by haste
  return trigger_gcd;
}

// warrior_spell_t::ready() ==================================================

bool warrior_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  warrior_t* p = player -> cast_warrior();

  if ( min_rage > 0 )
    if ( p -> resource_current[ RESOURCE_RAGE ] < min_rage )
      return false;

  if ( max_rage > 0 )
    if ( p -> resource_current[ RESOURCE_RAGE ] > max_rage )
      return false;

  // Attack vailable in current stance?
  if ( ( stancemask & p -> active_stance ) == 0 )
    return false;

  return true;
}

// Battle Shout ============================================================

struct battle_shout_t : public warrior_spell_t
{
  float refresh_early;
  int   shout_base_bonus;
  battle_shout_t( player_t* player, const std::string& options_str ) :
      warrior_spell_t( "battle_shout", player ),
      refresh_early( 0.0 ),
      shout_base_bonus( 0 )
  {
  }
  virtual bool ready() { return false; }
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
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// Berserker Rage ==========================================================

struct berserker_rage_t : public warrior_spell_t
{
  berserker_rage_t( player_t* player, const std::string& options_str ) :
      warrior_spell_t( "berserker_rage", player )
  {
    warrior_t* p = player -> cast_warrior();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = 0;
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

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = 0;
    trigger_gcd = 0;
    cooldown    = 60 * ( 1.0 - 0.11 * p -> talents.intensify_rage );;
    harmful     = false;
  }

  virtual void execute()
  {
    warrior_t* p = player -> cast_warrior();
    warrior_spell_t::execute();
    p -> resource_gain( RESOURCE_RAGE, 20 * ( 1 + p -> talents.improved_bloodrage * 0.25 ), p -> gains_bloodrage );
    p -> buffs_bloodrage -> trigger();
  }
};

// Death Wish ==============================================================

struct death_wish_t : public warrior_spell_t
{
  death_wish_t( player_t* player, const std::string& options_str ) :
      warrior_spell_t( "death_wish", player )
  {
    warrior_t* p = player -> cast_warrior();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = 10;
    trigger_gcd = 0;
    cooldown    = 180.0 * ( 1.0 - 0.11 * p -> talents.intensify_rage );
    harmful     = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();
    p -> buffs_death_wish -> trigger( 1, 0.20 );
  }
};

// Recklessness ============================================================

struct recklessness_t : public warrior_spell_t
{
  recklessness_t( player_t* player, const std::string& options_str ) :
      warrior_spell_t( "recklessness", player )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown    = 300.0 * ( 1.0 - 0.11 * p -> talents.intensify_rage );
    harmful     = false;

    stancemask  = STANCE_BERSERKER;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();
    p -> buffs_recklessness -> trigger( 3 );
  }
};

// Shield Block ============================================================

struct shield_block_t : public warrior_spell_t
{
  shield_block_t( player_t* player, const std::string& options_str ) :
      warrior_spell_t( "shield_block", player )
  {
    warrior_t* p = player -> cast_warrior();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown = 60.0 - 10.0 * p -> talents.shield_mastery;
    harmful  = false;
  }

  virtual void execute()
  {
    warrior_spell_t::execute();
    warrior_t* p = player -> cast_warrior();
    p -> buffs_shield_block -> trigger();
  }
};

// Stance ==================================================================

struct stance_t : public warrior_spell_t
{
  int switch_to_stance;
  stance_t( player_t* player, const std::string& options_str ) :
      warrior_spell_t( "stance", player ), switch_to_stance( 0 )
  {
    //warrior_t* p = player -> cast_warrior();

    std::string stance_str;
    option_t options[] =
    {
      { "choose",  OPT_STRING, &stance_str     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( ! stance_str.empty() )
    {
      if ( stance_str == "battle" )
        switch_to_stance = STANCE_BATTLE;
      else if ( stance_str == "berserker" || stance_str == "zerker" )
        switch_to_stance = STANCE_BERSERKER;
      else if ( stance_str == "def" || stance_str == "defensive" )
        switch_to_stance = STANCE_DEFENSE;
    }
    else
    {
      // Default to Battle Stance
      switch_to_stance = STANCE_BATTLE;
    }

    base_cost   = 0;
    trigger_gcd = 0;
    cooldown    = 1.0;
    harmful     = false;
  }

  virtual void execute()
  {
    const char* stance_name[] =
      { "Unknown Stance",
        "Battle Stance",
        "Berserker Stance",
        "Unpossible Stance",
        "Defensive Stance"
      };
    warrior_t* p = player -> cast_warrior();
    p -> aura_loss( stance_name[ p -> active_stance  ] );
    p -> active_stance = switch_to_stance;
    p -> aura_gain( stance_name[ p -> active_stance  ] );

    update_ready();
  }

  virtual bool ready()
  {
    warrior_t* p = player -> cast_warrior();

    return p -> active_stance != switch_to_stance;
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Warrior Character Definition
// =========================================================================

// warrior_t::composite_tank_miss ==========================================

double warrior_t::composite_tank_miss( int school ) SC_CONST
{
  double m = player_t::composite_tank_miss( school );

  if ( school != SCHOOL_PHYSICAL )
  {
    m += talents.improved_spell_reflection * 0.02;
  }

  if      ( m > 1.0 ) m = 1.0;
  else if ( m < 0.0 ) m = 0.0;

  return m;
}

// warrior_t::create_action  =================================================

action_t* warrior_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "auto_attack"     ) return new auto_attack_t    ( this, options_str );
  if ( name == "berserker_rage"  ) return new berserker_rage_t ( this, options_str );
  if ( name == "bladestorm"      ) return new bladestorm_t     ( this, options_str );
  if ( name == "bloodrage"       ) return new bloodrage_t      ( this, options_str );
  if ( name == "bloodthirst"     ) return new bloodthirst_t    ( this, options_str );
  if ( name == "concussion_blow" ) return new concussion_blow_t( this, options_str );
  if ( name == "death_wish"      ) return new death_wish_t     ( this, options_str );
  if ( name == "devastate"       ) return new devastate_t      ( this, options_str );
  if ( name == "execute"         ) return new execute_t        ( this, options_str );
  if ( name == "heroic_strike"   ) return new heroic_strike_t  ( this, options_str );
  if ( name == "mortal_strike"   ) return new mortal_strike_t  ( this, options_str );
  if ( name == "overpower"       ) return new overpower_t      ( this, options_str );
  if ( name == "pummel"          ) return new pummel_t         ( this, options_str );
  if ( name == "recklessness"    ) return new recklessness_t   ( this, options_str );
  if ( name == "rend"            ) return new rend_t           ( this, options_str );
  if ( name == "revenge"         ) return new revenge_t        ( this, options_str );
  if ( name == "shield_bash"     ) return new shield_bash_t    ( this, options_str );
  if ( name == "shield_block"    ) return new shield_block_t   ( this, options_str );
  if ( name == "shield_slam"     ) return new shield_slam_t    ( this, options_str );
  if ( name == "shockwave"       ) return new shockwave_t      ( this, options_str );
  if ( name == "slam"            ) return new slam_t           ( this, options_str );
  if ( name == "stance"          ) return new stance_t         ( this, options_str );
  if ( name == "thunderclap"     ) return new thunderclap_t    ( this, options_str );
  if ( name == "whirlwind"       ) return new whirlwind_t      ( this, options_str );

//if ( name == "retaliation"     ) return new retaliation_t    ( this, options_str );

  return player_t::create_action( name, options_str );
}

// warrior_t::init_glyphs =====================================================

void warrior_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "bladestorm"    ) glyphs.bladestorm = 1;
    else if ( n == "execution"     ) glyphs.execution = 1;
    else if ( n == "heroic_strike" ) glyphs.heroic_strike = 1;
    else if ( n == "mortal_strike" ) glyphs.mortal_strike = 1;
    else if ( n == "overpower"     ) glyphs.overpower = 1;
    else if ( n == "rending"       ) glyphs.rending = 1;
    else if ( n == "revenge"       ) glyphs.revenge = 1;
    else if ( n == "whirlwind"     ) glyphs.whirlwind = 1;
    else if ( n == "blocking"      ) glyphs.blocking = 1;
    // To prevent warnings....
    else if ( n == "battle"           ) ;
    else if ( n == "bloodrage"        ) ;
    else if ( n == "bloodthirst"      ) ;
    else if ( n == "charge"           ) ;
    else if ( n == "cleaving"         ) ;
    else if ( n == "command"          ) ;
    else if ( n == "devastate"        ) ;
    else if ( n == "enduring_victory" ) ;
    else if ( n == "hamstring"        ) ;
    else if ( n == "last_stand"       ) ;
    else if ( n == "shield_wall"      ) ;
    else if ( n == "sunder_armor"     ) ;
    else if ( n == "thunder_clap"     ) ;
    else if ( n == "vigilance"        ) ;
    else if ( ! sim -> parent ) util_t::printf( "simcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

// warrior_t::init_race ======================================================

void warrior_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_NIGHT_ELF:
  case RACE_GNOME:
  case RACE_UNDEAD:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_TAUREN:
    break;
  default:
    race = RACE_NIGHT_ELF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// warrior_t::init_base ========================================================

void warrior_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( level, WARRIOR, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  resource_base[  RESOURCE_RAGE  ] = 100;

  initial_attack_power_per_strength = 2.0;
  initial_attack_power_per_agility  = 0.0;

  base_attack_power = level * 2 + 60;
  base_attack_expertise += talents.vitality * 0.02;
  base_attack_expertise += talents.strength_of_arms * 0.02;

  // FIXME! Level-specific!
  base_defense = level * 5;
  base_miss    = 0.05;
  base_dodge   = 0.03664 + 0.01 * talents.anticipation;
  base_parry   = 0.05 + 0.01 * talents.deflection;
  base_block   = 0.05 + 0.01 * talents.shield_specialization;
  initial_armor_multiplier *= 1.0 + 0.02 * talents.toughness;
  initial_dodge_per_agility = 0.0001180;
  initial_armor_per_agility = 2.0;

  diminished_kfactor    = 0.9560;
  diminished_miss_capi  = 1.0 / 0.16;
  diminished_dodge_capi = 1.0 / 0.88129021;
  diminished_parry_capi = 1.0 / 0.47003525;

  attribute_multiplier_initial[ ATTR_STRENGTH ]   *= 1 + talents.strength_of_arms * 0.02 + talents.vitality * 0.02;
  attribute_multiplier_initial[ ATTR_STAMINA  ]   *= 1 + talents.strength_of_arms * 0.02 + talents.vitality * 0.02;

  health_per_stamina = 10;

  base_gcd = 1.5;

  if ( tank == -1 && primary_tree() == TREE_PROTECTION ) tank = 1;
}

// warrior_t::init_buffs =======================================================

void warrior_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_bloodrage                 = new buff_t( this, "bloodrage",                 1, 10.0 );
  buffs_bloodsurge                = new buff_t( this, "bloodsurge",                1,  5.0,   0, util_t::talent_rank( talents.bloodsurge, 3, 0.07, 0.13, 0.20 ) );
  buffs_death_wish                = new buff_t( this, "death_wish",                1, 30.0,   0, talents.death_wish );
  buffs_enrage                    = new buff_t( this, "enrage",                    1, 12.0,   0, talents.enrage ? 0.30 : 0.00 );
  buffs_flurry                    = new buff_t( this, "flurry",                    3, 15.0,   0, talents.flurry );
  buffs_improved_defensive_stance = new buff_t( this, "improved_defensive_stance", 1, 12.0,   0, talents.improved_defensive_stance / 2.0 );
  buffs_overpower                 = new buff_t( this, "overpower",                 1,  6.0, 1.0 );
  buffs_recklessness              = new buff_t( this, "recklessness",              3, 12.0 );
  buffs_revenge                   = new buff_t( this, "revenge",                   1 );
  buffs_shield_block              = new buff_t( this, "shield_block",              1, 10.0 );
  buffs_sudden_death              = new buff_t( this, "sudden_death",              1, 10.0,   0, talents.sudden_death * 0.03 );
  buffs_sword_and_board           = new buff_t( this, "sword_and_board",           1,  5.0,   0, talents.sword_and_board * 0.10 );
  buffs_taste_for_blood           = new buff_t( this, "taste_for_blood",           1,  9.0, 6.0, talents.taste_for_blood / 3.0 );
  buffs_wrecking_crew             = new buff_t( this, "wrecking_crew",             1, 12.0,   0, talents.wrecking_crew );
  buffs_tier7_4pc_melee           = new buff_t( this, "tier7_4pc_melee",           1, 30.0,   0, set_bonus.tier7_4pc_melee() * 0.10 );
  buffs_glyph_of_blocking         = new buff_t( this, "glyph_of_blocking",         1, 10.0,   0, glyphs.blocking );
  buffs_glyph_of_revenge          = new buff_t( this, "glyph_of_revenge",          1,    0,   0, glyphs.revenge );
  
  buffs_tier8_2pc_melee = new stat_buff_t( this, "tier8_2pc_melee", STAT_HASTE_RATING, 150, 1, 5.0, 0, set_bonus.tier8_2pc_melee() * 0.40 );
}

// warrior_t::init_gains =======================================================

void warrior_t::init_gains()
{
  player_t::init_gains();

  gains_anger_management       = get_gain( "anger_management" );
  gains_avoided_attacks        = get_gain( "avoided_attacks" );
  gains_bloodrage              = get_gain( "bloodrage" );
  gains_berserker_rage         = get_gain( "berserker_rage" );
  gains_glyph_of_heroic_strike = get_gain( "glyph_of_heroic_strike" );
  gains_incoming_damage        = get_gain( "incoming_damage" );
  gains_mh_attack              = get_gain( "mh_attack" );
  gains_oh_attack              = get_gain( "oh_attack" );
  gains_shield_specialization  = get_gain( "shield_specialization" );
  gains_sudden_death           = get_gain( "sudden_death" );
  gains_unbridled_wrath        = get_gain( "unbridled_wrath" );
}

// warrior_t::init_procs =======================================================

void warrior_t::init_procs()
{
  player_t::init_procs();

  procs_glyph_overpower      = get_proc( "glyph_of_overpower",   sim );
  procs_parry_haste          = get_proc( "parry_haste",          sim );
  procs_sword_specialization = get_proc( "sword_specialization", sim );
}

// warrior_t::init_uptimes =====================================================

void warrior_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_rage_cap = get_uptime( "rage_cap" );
}

// warrior_t::init_rng =========================================================

void warrior_t::init_rng()
{
  player_t::init_rng();

  rng_improved_defensive_stance = get_rng( "improved_defensive_stance" );
  rng_shield_specialization     = get_rng( "shield_specialization" );
  rng_sword_specialization      = get_rng( "sword_specialization"  );
  rng_unbridled_wrath           = get_rng( "unbridled_wrath"       );
}

// warrior_t::init_actions =====================================================

void warrior_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=endless_rage/food,type=hearty_rhino";
    if      ( primary_tree() == TREE_ARMS       ) action_list_str += "/stance,choose=battle";
    else if ( primary_tree() == TREE_FURY       ) action_list_str += "/stance,choose=berserker";
    else if ( primary_tree() == TREE_PROTECTION ) action_list_str += "/stance,choose=defensive";
    action_list_str += "/auto_attack";
    action_list_str += "/snapshot_stats";
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    if ( race == RACE_DWARF )
    {
      if ( talents.armored_to_the_teeth > 0 )
        action_list_str += "/stoneform";
    }
    else if ( race == RACE_ORC )
    {
      action_list_str += "/blood_fury";
    }
    else if ( race == RACE_TROLL )
    {
      action_list_str += "/berserking";
    }
    if ( primary_tree() == TREE_ARMS )
    {
      action_list_str += "/bloodrage,rage<=85";
      action_list_str += "/heroic_strike,rage>=95";
      action_list_str += "/mortal_strike";
      action_list_str += "/rend";
      action_list_str += "/overpower";
      action_list_str += "/bladestorm";
      action_list_str += "/execute";
      //action_list_str += "/slam,rage>=40";
      action_list_str += "/slam";
    }
    else if ( primary_tree() == TREE_FURY )
    {
      action_list_str += "/bloodrage,rage<=85";
      action_list_str += "/heroic_strike,rage>=75,health_percentage<=20";
      action_list_str += "/whirlwind,rage<=95,health_percentage<=20";
      action_list_str += "/bloodthirst,rage<=75,health_percentage<=20";
      action_list_str += "/slam,bloodsurge=1,rage<=45,health_percentage<=20";
      action_list_str += "/death_wish,health_percentage<=20";
      action_list_str += "/execute";
      action_list_str += "/recklessness,health_percentage<=20";
      action_list_str += "/heroic_strike,rage>=42,health_percentage>=20";
      action_list_str += "/whirlwind,health_percentage>=20";
      action_list_str += "/bloodthirst,health_percentage>=20";
      action_list_str += "/slam,bloodsurge=1,health_percentage>=20";
      action_list_str += "/death_wish,time>=10,health_percentage>=20,time_to_die>=135";
      action_list_str += "/recklessness,health_percentage>=20,time_to_die>=230";
      action_list_str += "/berserker_rage";
    }
    else if ( primary_tree() == TREE_PROTECTION )
    {
      action_list_str += "/bloodrage,rage<=85";
      action_list_str += "/heroic_strike,rage>=50";
      action_list_str += "/shield_block,sync=shield_slam";
      action_list_str += "/shield_slam";
      action_list_str += "/revenge";
      if ( talents.concussion_blow ) action_list_str += "/concussion_blow";
      if ( talents.shockwave       ) action_list_str += "/shockwave";
      if ( talents.devastate       ) action_list_str += "/devastate";
    }
    else
    {
      action_list_str += "/stance,choose=berserker/auto_attack";
    }

    action_list_default = 1;
  }

  player_t::init_actions();

  num_active_heroic_strikes = ( int ) active_heroic_strikes.size();
}

// warrior_t::primary_tree ====================================================

int warrior_t::primary_tree() SC_CONST
{
  if ( talents.mortal_strike ) return TREE_ARMS;
  if ( talents.bloodthirst   ) return TREE_FURY;
  if ( talents.vigilance     ) return TREE_PROTECTION;
  if ( talents.devastate     ) return TREE_PROTECTION;

  return TALENT_TREE_MAX;
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
  active_stance = STANCE_BATTLE;
  _cooldowns.reset();
}

// warrior_t::interrupt ======================================================

void warrior_t::interrupt()
{
  player_t::interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
  if (  off_hand_attack )  off_hand_attack -> cancel();
}

// warrior_t::composite_attack_power =========================================

double warrior_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();
  if ( talents.armored_to_the_teeth ) 
  {
    ap += talents.armored_to_the_teeth * composite_armor() / 108.0;
  }
  return ap;
}

// warrior_t::composite_tank_block ===========================================

double warrior_t::composite_tank_block() SC_CONST
{
  double b = player_t::composite_tank_block();
  if ( buffs_shield_block -> up() ) b *= 2.0;
  if ( buffs_glyph_of_blocking -> up() ) b *= 1.10;
  return b;
}

// warrior_t::composite_block_value ==========================================

double warrior_t::composite_block_value() SC_CONST
{
  double bv = player_t::composite_block_value();
  if ( talents.shield_mastery )
  {
    bv *= 1.0 + 0.15 * talents.shield_mastery;
  }
  if ( buffs_shield_block -> up() ) bv *= 2.0;
  return bv;
}

// warrior_t::regen ==========================================================

void warrior_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( talents.anger_management )
  {
    resource_gain( RESOURCE_RAGE, ( periodicity / 3.0 ), gains_anger_management );
  }
  if ( buffs_bloodrage -> up() )
  {
    resource_gain( RESOURCE_RAGE, ( periodicity / 1.0 ) * talents.improved_bloodrage * 0.25, gains_bloodrage );
  }

  uptimes_rage_cap -> update( resource_current[ RESOURCE_RAGE ] ==
                              resource_max    [ RESOURCE_RAGE] );

}

// warrior_t::resource_loss ================================================

double warrior_t::resource_loss( int       resource,
                                 double    amount,
                                 action_t* action )
{
  double actual_amount = player_t::resource_loss( resource, amount, action );

  if ( resource == RESOURCE_HEALTH )
  {
    // FIXME!!! Only works for level 80 currently!
    double coeff_60 = 453.3;
    double coeff_70 = 453.3;
    double coeff_80 = 453.3;

    double coeff = rating_t::interpolate( level, coeff_60, coeff_70, coeff_80 );

    double rage_gain = 5 * amount / ( 2 * coeff );

    resource_gain( RESOURCE_RAGE, rage_gain, gains_incoming_damage );
  }

  return actual_amount;
}

// warrior_t::target_swing ==================================================

int warrior_t::target_swing()
{
  int result = player_t::target_swing();

  if ( sim -> log ) log_t::output( sim, "%s swing result: %s", sim -> target -> name(), util_t::result_type_string( result ) );

  if ( result == RESULT_HIT    ||
       result == RESULT_CRIT   ||
       result == RESULT_GLANCE ||
       result == RESULT_BLOCK  )
  {
    resource_gain( RESOURCE_RAGE, 100.0, gains_incoming_damage );  // FIXME! Assume it caps rage every time.
    buffs_enrage -> trigger( 1, 0.02 * talents.enrage );
  }
  if ( result == RESULT_BLOCK ||
       result == RESULT_DODGE ||
       result == RESULT_PARRY )
  {
    if ( talents.shield_specialization )
    {
      if ( rng_shield_specialization -> roll( 0.20 * talents.shield_specialization ) )
      {
	resource_gain( RESOURCE_RAGE, 5.0, gains_shield_specialization );
      }
    }
    if ( talents.improved_defensive_stance && active_stance == STANCE_DEFENSE )
    {
      buffs_improved_defensive_stance -> trigger();
    }
    buffs_revenge -> trigger();
  }
  if ( result == RESULT_BLOCK )
  {
    if ( talents.damage_shield )
    {
      struct damage_shield_t : public warrior_attack_t
      {
	damage_shield_t( player_t* player ) :
	  warrior_attack_t( "damage_shield",  player, SCHOOL_PHYSICAL, TREE_PROTECTION )
	{
	  proc = background  = true;
	  may_miss = may_crit = may_dodge = may_parry = false;
	  base_dd_min = base_dd_max = 1;
	  trigger_gcd = 0;
	  reset();
	}
      };

      if ( ! active_damage_shield ) active_damage_shield = new damage_shield_t( this );
      active_damage_shield -> base_dd_adder = composite_block_value() * 0.10 * talents.damage_shield;
      active_damage_shield -> execute();
    }
  }
  if ( result == RESULT_PARRY )
  {
    if ( main_hand_attack && main_hand_attack -> execute_event )
    {
      double swing_time = main_hand_attack -> time_to_execute;
      double max_reschedule = ( main_hand_attack -> execute_event -> occurs() - 0.20 * swing_time ) - sim -> current_time;

      if ( max_reschedule > 0 )
      {
	main_hand_attack -> reschedule_execute( std::min( ( 0.40 * swing_time ), max_reschedule ) );
	procs_parry_haste -> occur();
      }
    }
  }
  return result;
}

// warrior_t::get_talent_trees ==============================================

std::vector<talent_translation_t>& warrior_t::get_talent_list()
{
  if(talent_list.empty())
  {
	  talent_translation_t translation_table[][MAX_TALENT_TREES] =
	  {
    { {  1, 3, &( talents.improved_heroic_strike          ) }, {  1, 3, &( talents.armored_to_the_teeth      ) }, {  1, 2, &( talents.improved_bloodrage             ) } },
    { {  2, 5, &( talents.deflection                      ) }, {  2, 2, &( talents.booming_voice             ) }, {  2, 5, &( talents.shield_specialization          ) } },
    { {  3, 2, &( talents.improved_rend                   ) }, {  3, 5, &( talents.cruelty                   ) }, {  3, 3, &( talents.improved_thunderclap           ) } },
    { {  4, 0, NULL                                         }, {  4, 0, NULL                                   }, {  4, 3, &( talents.incite                         ) } },
    { {  5, 0, NULL                                         }, {  5, 5, &( talents.unbridled_wrath           ) }, {  5, 5, &( talents.anticipation                   ) } },
    { {  6, 0, NULL                                         }, {  6, 0, NULL                                   }, {  6, 0, NULL                                        } },
    { {  7, 2, &( talents.improved_overpower              ) }, {  7, 0, NULL                                   }, {  7, 2, &( talents.improved_revenge               ) } },
    { {  8, 1, &( talents.anger_management                ) }, {  8, 0, NULL                                   }, {  8, 2, &( talents.shield_mastery                 ) } },
    { {  9, 2, &( talents.impale                          ) }, {  9, 5, &( talents.commanding_presence       ) }, {  9, 5, &( talents.toughness                      ) } },
    { { 10, 3, &( talents.deep_wounds                     ) }, { 10, 5, &( talents.dual_wield_specialization ) }, { 10, 2, &( talents.improved_spell_reflection      ) } },
    { { 11, 3, &( talents.twohanded_weapon_specialization ) }, { 11, 2, &( talents.improved_execute          ) }, { 11, 0, NULL                                        } },
    { { 12, 3, &( talents.taste_for_blood                 ) }, { 12, 5, &( talents.enrage                    ) }, { 12, 3, &( talents.puncture                       ) } },
    { { 13, 5, &( talents.poleaxe_specialization          ) }, { 13, 3, &( talents.precision                 ) }, { 13, 0, NULL                                        } },
    { { 14, 0, NULL                                         }, { 14, 1, &( talents.death_wish                ) }, { 14, 1, &( talents.concussion_blow                ) } },
    { { 15, 5, &( talents.mace_specialization             ) }, { 15, 0, NULL                                   }, { 15, 2, &( talents.gag_order                      ) } },
    { { 16, 5, &( talents.sword_specialization            ) }, { 16, 2, &( talents.improved_berserker_rage   ) }, { 16, 5, &( talents.onhanded_weapon_specialization ) } },
    { { 17, 2, &( talents.weapon_mastery                  ) }, { 17, 5, &( talents.flurry                    ) }, { 17, 2, &( talents.improved_defensive_stance      ) } },
    { { 18, 0, NULL                                         }, { 18, 3, &( talents.intensify_rage            ) }, { 18, 1, &( talents.vigilance                      ) } },
    { { 19, 2, &( talents.trauma                          ) }, { 19, 1, &( talents.bloodthirst               ) }, { 19, 3, &( talents.focused_rage                   ) } },
    { { 20, 0, NULL                                         }, { 20, 2, &( talents.improved_whirlwind        ) }, { 20, 3, &( talents.vitality                       ) } },
    { { 21, 1, &( talents.mortal_strike                   ) }, { 21, 0, NULL                                   }, { 21, 0, NULL                                        } },
    { { 22, 2, &( talents.strength_of_arms                ) }, { 22, 5, &( talents.improved_berserker_stance ) }, { 22, 0, NULL                                        } },
    { { 23, 2, &( talents.improved_slam                   ) }, { 23, 0, NULL                                   }, { 23, 1, &( talents.devastate                      ) } },
    { { 24, 0, NULL                                         }, { 24, 1, &( talents.rampage                   ) }, { 24, 3, &( talents.critical_block                 ) } },
    { { 25, 3, &( talents.improved_mortal_strike          ) }, { 25, 3, &( talents.bloodsurge                ) }, { 25, 3, &( talents.sword_and_board                ) } },
    { { 26, 2, &( talents.unrelenting_assault             ) }, { 26, 5, &( talents.unending_fury             ) }, { 26, 2, &( talents.damage_shield                  ) } },
    { { 27, 3, &( talents.sudden_death                    ) }, { 27, 1, &( talents.titans_grip               ) }, { 27, 1, &( talents.shockwave                      ) } },
    { { 28, 1, &( talents.endless_rage                    ) }, {  0, 0, NULL                                   }, {  0, 0, NULL                                        } },
    { { 29, 2, &( talents.blood_frenzy                    ) }, {  0, 0, NULL                                   }, {  0, 0, NULL                                        } },
    { { 30, 5, &( talents.wrecking_crew                   ) }, {  0, 0, NULL                                   }, {  0, 0, NULL                                        } },
    { { 31, 1, &( talents.bladestorm                      ) }, {  0, 0, NULL                                   }, {  0, 0, NULL                                        } },
    { {  0, 0, NULL                                         }, {  0, 0, NULL                                   }, {  0, 0, NULL                                        } }
  };

    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
  }
  return talent_list;
}

// warrior_t::get_options ================================================

std::vector<option_t>& warrior_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t warrior_options[] =
    {
      // @option_doc loc=player/warrior/talents title="Talents"
      { "anticipation",                    OPT_INT, &( talents.anticipation                    ) },
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
      { "damage_shield",                   OPT_INT, &( talents.damage_shield                   ) },
      { "death_wish",                      OPT_INT, &( talents.death_wish                      ) },
      { "deep_wounds",                     OPT_INT, &( talents.deep_wounds                     ) },
      { "deflection",                      OPT_INT, &( talents.deflection                      ) },
      { "devastate",                       OPT_INT, &( talents.devastate                       ) },
      { "dual_wield_specialization",       OPT_INT, &( talents.dual_wield_specialization       ) },
      { "endless_rage",                    OPT_INT, &( talents.endless_rage                    ) },
      { "enrage",                          OPT_INT, &( talents.enrage                          ) },
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
      { "improved_spell_reflection",       OPT_INT, &( talents.improved_spell_reflection       ) },
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
      { "rampage",                         OPT_INT, &( talents.rampage                         ) },
      { "shield_mastery",                  OPT_INT, &( talents.shield_mastery                  ) },
      { "shield_specialization",           OPT_INT, &( talents.shield_specialization           ) },
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
      { "vigilance",                       OPT_INT, &( talents.vigilance                       ) },
      { "vitality",                        OPT_INT, &( talents.vitality                        ) },
      { "weapon_mastery",                  OPT_INT, &( talents.weapon_mastery                  ) },
      { "wrecking_crew",                   OPT_INT, &( talents.wrecking_crew                   ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, warrior_options );
  }

  return options;
}

// warrior_t::decode_set ===================================================

int warrior_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  bool is_melee = ( strstr( s, "helmet"         ) || 
		    strstr( s, "shoulderplates" ) ||
		    strstr( s, "battleplate"    ) ||
		    strstr( s, "legplates"      ) ||
		    strstr( s, "gauntlets"      ) );

  bool is_tank = ( strstr( s, "greathelm"   ) || 
		   strstr( s, "pauldrons"   ) ||
		   strstr( s, "breastplate" ) ||
		   strstr( s, "legguards"   ) ||
		   strstr( s, "handguards"  ) );
  
  if ( strstr( s, "dreadnaught" ) ) 
  {
    if ( is_melee ) return SET_T7_MELEE;
    if ( is_tank  ) return SET_T7_TANK;
  }
  if ( strstr( s, "siegebreaker" ) )
  {
    if ( is_melee ) return SET_T8_MELEE;
    if ( is_tank  ) return SET_T8_TANK;
  }
  if ( strstr( s, "wrynns" ) ||
       strstr( s, "hellscreams"  ) ) 
  {
    if ( is_melee ) return SET_T9_MELEE;
    if ( is_tank  ) return SET_T9_TANK;
  }

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_warrior ===============================================

player_t* player_t::create_warrior( sim_t* sim, const std::string& name, int race_type )
{
  return new warrior_t( sim, name, race_type );
}

// warrior_init ===================================================

void player_t::warrior_init( sim_t* sim )
{
  sim -> auras.battle_shout = new aura_t( sim, "battle_shout", 1, 120.0 );
  sim -> auras.rampage      = new aura_t( sim, "rampage",      1,  10.0 );

  target_t* t = sim -> target;
  t -> debuffs.blood_frenzy = new debuff_t( sim, "blood_frenzy", 1, 15.0 );
  t -> debuffs.sunder_armor = new debuff_t( sim, "sunder_armor", 1, 30.0 );
  t -> debuffs.thunder_clap = new debuff_t( sim, "thunder_clap", 1, 30.0 );
  t -> debuffs.trauma       = new debuff_t( sim, "trauma",       1, 15.0 );
}

// player_t::warrior_combat_begin ===========================================

void player_t::warrior_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.battle_shout ) sim -> auras.battle_shout -> override( 1, 548 );
  if ( sim -> overrides.rampage      ) sim -> auras.rampage      -> override();

  target_t* t = sim -> target;
  if ( sim -> overrides.blood_frenzy ) t -> debuffs.blood_frenzy -> override();
  if ( sim -> overrides.sunder_armor ) t -> debuffs.sunder_armor -> override( 1, 0.20 );
  if ( sim -> overrides.thunder_clap ) t -> debuffs.thunder_clap -> override();
  if ( sim -> overrides.trauma       ) t -> debuffs.trauma       -> override();
}
