// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Paladin
// ==========================================================================

enum seal_type_t { 
  SEAL_NONE=0, 
  SEAL_OF_COMMAND, 
  SEAL_OF_JUSTICE, 
  SEAL_OF_LIGHT, 
  SEAL_OF_RIGHTEOUSNESS, 
  SEAL_OF_VENGEANCE, 
  SEAL_OF_WISDOM, 
  SEAL_MAX
};

struct paladin_t : public player_t
{
  // Active
  int       active_seal;
  action_t* active_seal_of_command_proc;
  action_t* active_seal_of_justice_proc;
  action_t* active_seal_of_light_proc;
  action_t* active_seal_of_righteousness_proc;
  action_t* active_seal_of_vengeance_proc;
  action_t* active_seal_of_vengeance_dot;
  action_t* active_seal_of_wisdom_proc;
  action_t* active_righteous_vengeance;

  // Buffs
  buff_t* buffs_avenging_wrath;
  buff_t* buffs_divine_plea;
  buff_t* buffs_seal_of_vengeance;
  buff_t* buffs_the_art_of_war;
  buff_t* buffs_vengeance;

  // Gains
  gain_t* gains_divine_plea;
  gain_t* gains_judgements_of_the_wise;
  gain_t* gains_seal_of_wisdom;

  // Random Number Generation
  rng_t* rng_judgements_of_the_wise;

  // Auto-Attack
  attack_t* auto_attack;

  struct talents_t
  {
    // Consider all NYI
    int aura_mastery;
    int avengers_shield;
    int benediction; 
    int blessing_of_sanctuary;
    int combat_expertise;
    int conviction;
    int crusade;
    int crusader_strike;
    int divine_favor;
    int divine_illumination;
    int divine_intellect;
    int divine_storm;
    int divine_strength;
    int enlightened_judgements;
    int fanaticism;
    int guarded_by_the_light;
    int hammer_of_the_righteous;
    int healing_light;
    int heart_of_the_crusader;
    int holy_guidance;
    int holy_power;
    int holy_shield;
    int holy_shock;
    int improved_blessing_of_might;
    int improved_blessing_of_wisdom;
    int improved_concentration_aura;
    int improved_hammer_of_justice;
    int improved_judgements;
    int judgements_of_the_just;
    int judgements_of_the_pure;
    int judgements_of_the_wise;
    int one_handed_weapon_spec;
    int purifying_power;
    int reckoning;
    int righteous_vengeance;
    int sacred_duty;
    int sanctified_light;
    int sanctified_retribution;
    int sanctified_wrath;
    int sanctity_of_battle;
    int seal_of_command;
    int seals_of_the_pure;
    int sheath_of_light;
    int spiritual_focus;
    int swift_retribution;
    int the_art_of_war;
    int touched_by_the_light;
    int two_handed_weapon_spec;
    int vengeance;
    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    // Consider all NYI
    int avengers_shield;
    int avenging_wrath;
    int consecration;
    int crusader_strike;
    int exorcism;
    int hammer_of_wrath;
    int holy_shock;
    int holy_wrath;
    int judgement;
    int righteous_defense;
    int seal_of_command;
    int seal_of_righteousness;
    int seal_of_vengeance;
    int sense_undead;
    int shield_of_righteousness;
    int wise;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct librams_t
  {
    // Consider all NYI
    int deadly_gladiators_fortitude;
    int furious_gladiators_fortitude;
    int hateful_gladiators_fortitude;
    int avengement;
    int discord;
    int divine_judgement;
    int divine_purpose;
    int furious_blows;
    int radiance;
    int reciprocation;
    int resurgence;
    int valiance;
    int relentless_gladiators_fortitude;
    int savage_gladiators_fortitude;
    int venture_co_protection;
    int venture_co_retribution;
    librams_t() { memset( ( void* ) this, 0x0, sizeof( librams_t ) ); }
  };
  librams_t librams;

  paladin_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, PALADIN, name, race_type )
  {
    active_seal = SEAL_NONE;

    active_seal_of_command_proc       = 0;
    active_seal_of_justice_proc       = 0;
    active_seal_of_light_proc         = 0;
    active_seal_of_righteousness_proc = 0;
    active_seal_of_vengeance_proc     = 0;
    active_seal_of_vengeance_dot      = 0;
    active_seal_of_wisdom_proc        = 0;

    active_righteous_vengeance = 0;

    auto_attack = 0;
  }

  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_glyphs();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_items();
  virtual void      init_buffs();
  virtual void      init_actions();
  virtual void      reset();
  virtual void      interrupt();
  virtual double    composite_attack_crit() SC_CONST;
  virtual double    composite_spell_crit() SC_CONST;
  virtual double    composite_spell_power( int school ) SC_CONST;
  virtual bool      get_talent_trees( std::vector<int*>& holy, std::vector<int*>& protection, std::vector<int*>& retribution );
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_HYBRID; }
  virtual int       primary_tree() SC_CONST;
  virtual void      regen( double periodicity );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_judgements_of_the_wise ==========================================

static void trigger_judgements_of_the_wise( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.judgements_of_the_wise ) return;
  if ( ! p -> rng_judgements_of_the_wise -> roll( p -> talents.judgements_of_the_wise / 3.0 ) ) return;

  p -> resource_gain( RESOURCE_MANA, 0.25 * p -> resource_base[ RESOURCE_MANA ], p -> gains_judgements_of_the_wise );
  p -> trigger_replenishment();
}

// trigger_righteous_vengeance =============================================

static void trigger_righteous_vengeance( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.righteous_vengeance ) return;

  struct righteous_vengeance_t : public attack_t
  {
    righteous_vengeance_t( paladin_t* p ) : attack_t( "righteous_vengeance", p, RESOURCE_NONE, SCHOOL_HOLY )
    {
      background     = true;
      proc           = true;
      trigger_gcd    = 0;
      base_tick_time = 2;
      num_ticks      = 4;
      reset(); // required since construction occurs after player_t::init()
    }
    void player_buff()   {}
    void target_debuff( int dmg_type ) {}
  };

  if ( ! p -> active_righteous_vengeance ) p -> active_righteous_vengeance = new righteous_vengeance_t( p );

  double dmg = p -> talents.righteous_vengeance * 0.1 * a -> direct_dmg;

  if ( p -> active_righteous_vengeance -> ticking )
  {
    int remaining_ticks = ( p -> active_righteous_vengeance -> num_ticks - 
                            p -> active_righteous_vengeance -> current_tick );

    dmg += p -> active_righteous_vengeance -> base_td * remaining_ticks;

    p -> active_righteous_vengeance -> cancel();
  }
  p -> active_righteous_vengeance -> base_td = dmg / 8;
  p -> active_righteous_vengeance -> schedule_tick();

  // FIXME! Does the DoT tick get "refreshed" or "rescheduled" (ie: delayed similar to Ignite)
}

// =========================================================================
// Paladin Attacks
// =========================================================================

struct paladin_attack_t : public attack_t
{
  bool trigger_seal;

  paladin_attack_t( const char* n, paladin_t* p, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, p, RESOURCE_MANA, s, t, special ),
      trigger_seal( false )
  {
    base_dd_min = base_dd_max = 1;
    if ( p -> talents.crusade )
    {
      int r = sim -> target -> race;
      bool crusade_race = ( r == RACE_HUMANOID || r == RACE_DEMON || r == RACE_UNDEAD || r == RACE_ELEMENTAL );
      base_multiplier *= 1.0 + 0.01 * p -> talents.crusade * ( crusade_race ? 2 : 1 );
    }
  }

  virtual void execute();
  virtual void player_buff();
};

// paladin_attack_t::execute ===============================================

void paladin_attack_t::execute()
{
  paladin_t* p = player -> cast_paladin();

  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      p -> buffs_vengeance -> trigger();
    }
    if ( trigger_seal )
    {
      paladin_t* p = player -> cast_paladin();

      // FIXME! Do any of these procs occur on melee attacks only?

      switch ( p -> active_seal )
      {
      case SEAL_OF_COMMAND:       p -> active_seal_of_command_proc       -> execute(); break;
      case SEAL_OF_JUSTICE:       p -> active_seal_of_justice_proc       -> execute(); break;
      case SEAL_OF_LIGHT:         p -> active_seal_of_light_proc         -> execute(); break;
      case SEAL_OF_RIGHTEOUSNESS: p -> active_seal_of_righteousness_proc -> execute(); break;
      case SEAL_OF_WISDOM:        p -> active_seal_of_wisdom_proc        -> execute(); break;
      case SEAL_OF_VENGEANCE:     if ( p -> buffs_seal_of_vengeance -> stack() == 5 ) p -> active_seal_of_vengeance_proc -> execute(); break;
      default:;
      }
    }
  }
}

// paladin_attack_t::player_buff ===========================================

void paladin_attack_t::player_buff()
{
  paladin_t* p = player -> cast_paladin();

  attack_t::player_buff();

  if ( p -> talents.two_handed_weapon_spec ) // FIXME! Anything this does not affect?
  {
    if ( weapon && weapon -> group() == WEAPON_2H )
    {
      player_multiplier *= 1.0 + 0.02 * p -> talents.two_handed_weapon_spec;
    }
  }
  if ( p -> active_seal == SEAL_OF_VENGEANCE && p -> glyphs.seal_of_vengeance )
  {
    player_expertise += 0.0025 * 10;
  }
  if ( p -> talents.vengeance )
  {
    player_multiplier *= 1.0 + 0.01 * p -> talents.vengeance * p -> buffs_vengeance -> stack();
  }
  if ( p -> buffs_avenging_wrath -> up() ) 
  {
    player_multiplier *= 1.20;
  }
}

// Melee Attack ============================================================

struct melee_t : public paladin_attack_t
{
  melee_t( paladin_t* p ) :
    paladin_attack_t( "melee", p, SCHOOL_PHYSICAL, TREE_NONE, false )
  {
    trigger_seal      = true;
    background        = true;
    repeating         = true;
    trigger_gcd       = 0;
    base_cost         = 0;
    weapon            = &( p -> main_hand_weapon );
    base_execute_time = p -> main_hand_weapon.swing_time;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();

    paladin_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        p -> buffs_the_art_of_war -> trigger();
      }
      if ( p -> active_seal == SEAL_OF_VENGEANCE )
      {
        p -> active_seal_of_vengeance_dot -> execute();
      }
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public paladin_attack_t
{
  auto_attack_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "auto_attack", p )
  {
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> auto_attack = new melee_t( p );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> auto_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> auto_attack -> execute_event == 0 ); // not swinging
  }
};

// Avengers Shield =========================================================

struct avengers_shield_t : public paladin_attack_t
{
  avengers_shield_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "avengers_shield", p )
  {
    check_talent( p -> talents.avengers_shield );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 1100, 1344, 0, 0.26 },
      { 75, 4,  913, 1115, 0, 0.26 },
      { 70, 3,  796,  972, 0, 0.26 },
      { 60, 2,  601,  733, 0, 0.26 },
    };
    init_rank( ranks );

    trigger_seal = true;

    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 30;

    direct_power_mod = 1.0;
    base_spell_power_multiplier  = 0.15;
    base_attack_power_multiplier = 0.15;
  }
};

// Crusader Strike =========================================================

struct crusader_strike_t : public paladin_attack_t
{
  crusader_strike_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "crusader_strike", p )
  {
    check_talent( p -> talents.crusader_strike );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_seal = true;

    weapon                 = &( p -> main_hand_weapon );
    weapon_multiplier     *= 0.75;
    normalize_weapon_speed = true;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 4;

    base_multiplier *= 1.0 + 0.05 * p -> talents.sanctity_of_battle;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    if ( p -> set_bonus.tier8_4pc_melee() ) base_crit += 0.10;
  }

  virtual void execute()
  {
    paladin_attack_t::execute();

    if ( result == RESULT_CRIT )
    {
      trigger_righteous_vengeance( this );
    }
  }
};

// Divine Storm ============================================================

struct divine_storm_t : public paladin_attack_t
{
  divine_storm_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "divine_storm", p )
  {
    check_talent( p -> talents.divine_storm );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_seal = true;

    weapon                 = &( p -> main_hand_weapon );
    weapon_multiplier     *= 1.1;
    normalize_weapon_speed = true;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.12;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 10;

    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    if ( p -> set_bonus.tier7_2pc_melee() ) base_multiplier *= 1.1;
    if ( p -> set_bonus.tier8_4pc_melee() ) base_crit += 0.10;
  }

  virtual void execute()
  {
    paladin_attack_t::execute();
    if ( result == RESULT_CRIT )
    {
      trigger_righteous_vengeance( this );
    }
  }
};

// Hammer of Wrath =========================================================

struct hammer_of_wrath_t : public paladin_attack_t
{
  hammer_of_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "hammer_of_wrath", p, SCHOOL_HOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 6, 1139, 1257, 0, 0.12 },
      { 74, 5,  878,  970, 0, 0.12 },
      { 68, 4,  733,  809, 0, 0.12 },
      { 60, 3,  570,  628, 0, 0.14 },
    };
    init_rank( ranks );

    may_parry = false;
    may_dodge = false;

    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    base_crit += 0.25 * p -> talents.sanctified_wrath;
    cooldown   = 6;

    direct_power_mod = 1.0;
    base_spell_power_multiplier  = 0.15;
    base_attack_power_multiplier = 0.15;
  }

  virtual bool ready()
  {
    if ( sim -> target -> health_percentage() > 20 )
      return false;

    return paladin_attack_t::ready();
  }
};

// Paladin Seals ============================================================

struct paladin_seal_t : public paladin_attack_t
{
  int seal_type;

  paladin_seal_t( paladin_t* p, const char* n, int st, const std::string& options_str ) :
    paladin_attack_t( n, p ), seal_type( st )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful    = false;
    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.14;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    paladin_t* p = player -> cast_paladin();
    p -> active_seal = seal_type;
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> active_seal == seal_type ) return false;
    return paladin_attack_t::ready();
  }

  virtual double cost() SC_CONST 
  { 
    return player -> in_combat ? paladin_attack_t::cost() : 0;
  }
};

// Seal of Command ==========================================================

struct seal_of_command_proc_t : public paladin_attack_t
{
  seal_of_command_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_command_proc", p, SCHOOL_HOLY )
  {
    background        = true;
    proc              = true;
    trigger_gcd       = 0;
    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.36;
    weapon_power_mod  = 0;  // FIXME! Assume no weapon-speed adjusted AP contribution.
  }
};

struct seal_of_command_judgement_t : public paladin_attack_t
{
  seal_of_command_judgement_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_command_judgement", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.19;
    weapon_power_mod  = 0;  // FIXME! Assume no weapon-speed adjusted AP contribution.

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 10 - p -> talents.improved_judgements;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.13;
    base_attack_power_multiplier = 0.08;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.1;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown--;
  }
};

// Seal of Justice ==========================================================

struct seal_of_justice_proc_t : public paladin_attack_t
{
  seal_of_justice_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_justice_proc", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;
  }
  virtual void execute() 
  {
    // No need to model stun
  }
};

struct seal_of_justice_judgement_t : public paladin_attack_t
{
  seal_of_justice_judgement_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_justice_judgement", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 10 - p -> talents.improved_judgements;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.32;
    base_attack_power_multiplier = 0.20;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.1;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown--;
  }
};

// Seal of Light ============================================================

struct seal_of_light_proc_t : public paladin_attack_t
{
  seal_of_light_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_light_proc", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;

    base_spell_power_multiplier = 0.15;
    base_attack_power_multiplier = 0.15;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> resource_gain( RESOURCE_HEALTH, total_power() );
  }
};

struct seal_of_light_judgement_t : public paladin_attack_t
{
  seal_of_light_judgement_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_light_judgement", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 10 - p -> talents.improved_judgements;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.25;
    base_attack_power_multiplier = 0.16;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.1;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown--;
  }
};

// Seal of Righteousness ====================================================

struct seal_of_righteousness_proc_t : public paladin_attack_t
{
  seal_of_righteousness_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_righteousness_proc", p, SCHOOL_HOLY )
  {
    background       = true;
    proc             = true;
    trigger_gcd      = 0;
    base_multiplier *= p -> main_hand_weapon.swing_time;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.044;
    base_attack_power_multiplier = 0.022;
  }
};

struct seal_of_righteousness_judgement_t : public paladin_attack_t
{
  seal_of_righteousness_judgement_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_righteousness_judgement", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;
    may_crit  = false;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 10 - p -> talents.improved_judgements;

    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.32;
    base_attack_power_multiplier = 0.20;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.1;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown--;
  }
};

// Seal of Vengeance ========================================================

struct seal_of_vengeance_dot_t : public paladin_attack_t
{
  seal_of_vengeance_dot_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_vengeance_dot", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;
    base_cost   = 0;

    base_td = 1;
    num_ticks = 5;
    base_tick_time = 3;
    
    tick_power_mod = 1.0;
    base_spell_power_multiplier = 0.013;
    base_attack_power_multiplier = 0.025;

    base_multiplier *= ( 1 + p -> talents.seals_of_the_pure * 0.03 );
  }

  virtual void execute()
  {
    // FIXME! Application requires an attack roll?
    paladin_t* p = player -> cast_paladin();
    player_buff();
    target_debuff( DMG_DIRECT );
    calculate_result();
    if ( result_is_hit() )
    {
      p -> buffs_seal_of_vengeance -> trigger();
      if ( ticking )
      {
        refresh_duration();
      }
      else
      {
        schedule_tick();
      }
    }
    update_stats( DMG_DIRECT );
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= p -> buffs_seal_of_vengeance -> stack();
  }

  virtual void last_tick()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::last_tick();
    p -> buffs_seal_of_vengeance -> expire();
  }
};

struct seal_of_vengeance_proc_t : public paladin_attack_t
{
  seal_of_vengeance_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_vengeance_proc", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    may_miss    = false;
    may_dodge   = false;
    may_parry   = false;
    trigger_gcd = 0;

    weapon            = &( p -> main_hand_weapon );
    weapon_multiplier = 0.33;
    weapon_power_mod  = 0;  // FIXME! Assume no weapon-speed adjusted AP contribution.
  }
};

struct seal_of_vengeance_judgement_t : public paladin_attack_t
{
  seal_of_vengeance_judgement_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_vengeance_judgement", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 10 - p -> talents.improved_judgements;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;
    base_multiplier *= 1 + p -> talents.seals_of_the_pure * 0.03;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.22;
    base_attack_power_multiplier = 0.14;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.1;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown--;
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= 1.0 + p -> buffs_seal_of_vengeance -> stack() * 0.10;
  }
};

// Seal of Wisdom ===========================================================

struct seal_of_wisdom_proc_t : public paladin_attack_t
{
  seal_of_wisdom_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_wisdom_proc", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.04, p -> gains_seal_of_wisdom );
  }
};

struct seal_of_wisdom_judgement_t : public paladin_attack_t
{
  seal_of_wisdom_judgement_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_wisdom_judgement", p, SCHOOL_HOLY )
  {
    background = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown   = 10 - p -> talents.improved_judgements;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.25;
    base_attack_power_multiplier = 0.16;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.1;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown--;
  }
};

// Judgement ================================================================

struct judgement_t : public paladin_attack_t
{
  action_t* seal_of_command;
  action_t* seal_of_justice;
  action_t* seal_of_light;
  action_t* seal_of_righteousness;
  action_t* seal_of_vengeance;
  action_t* seal_of_wisdom;

  judgement_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "judgement", p, SCHOOL_HOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    seal_of_command       = new seal_of_command_judgement_t      ( p );
    seal_of_justice       = new seal_of_justice_judgement_t      ( p );
    seal_of_light         = new seal_of_light_judgement_t        ( p );
    seal_of_righteousness = new seal_of_righteousness_judgement_t( p );
    seal_of_vengeance     = new seal_of_vengeance_judgement_t    ( p );
    seal_of_wisdom        = new seal_of_wisdom_judgement_t       ( p );
  }

  action_t* active_seal() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    switch ( p -> active_seal )
    {
    case SEAL_OF_COMMAND:       return seal_of_command;
    case SEAL_OF_JUSTICE:       return seal_of_justice;
    case SEAL_OF_LIGHT:         return seal_of_light;
    case SEAL_OF_RIGHTEOUSNESS: return seal_of_righteousness;
    case SEAL_OF_VENGEANCE:     return seal_of_vengeance;
    case SEAL_OF_WISDOM:        return seal_of_wisdom;
    }
    assert( 0 ); 
    return 0;
  }

  virtual double cost() SC_CONST { return active_seal() -> cost(); }

  virtual void execute()
  {
    action_t* seal = active_seal();

    seal -> execute();

    if ( seal -> result_is_hit() )
    {
      trigger_judgements_of_the_wise( seal );

      if ( seal -> result == RESULT_CRIT )
      {
        trigger_righteous_vengeance( seal );
      }
    }

    // FIXME! Refresh JoW/JoL debuff here
  }

  virtual bool ready()
  {
    if( ! active_seal() -> ready() ) return false;
    return paladin_attack_t::ready();
  }
};

// =========================================================================
// Paladin Spells
// =========================================================================

struct paladin_spell_t : public spell_t
{
  paladin_spell_t( const char* n, paladin_t* p, int s=SCHOOL_HOLY, int t=TREE_NONE ) :
      spell_t( n, p, RESOURCE_MANA, s, t )
  {
    if ( p -> talents.crusade )
    {
      int r = sim -> target -> race;
      bool crusade_race = ( r == RACE_HUMANOID || r == RACE_DEMON || r == RACE_UNDEAD || r == RACE_ELEMENTAL );
      base_multiplier *= 1.0 + 0.01 * p -> talents.crusade * ( crusade_race ? 2 : 1 );
    }
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    spell_t::player_buff();

    if ( p -> talents.vengeance )
    {
      player_multiplier *= 1.0 + 0.01 * p -> talents.vengeance * p -> buffs_vengeance -> stack();
    }
    if ( p -> buffs_avenging_wrath -> up() ) 
    {
      player_multiplier *= 1.20;
    }
  }
};

// Avenging Wrath ==========================================================

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "avenging_wrath", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful     = false;
    cooldown    = 180 - 30 * p -> talents.sanctified_wrath;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.08;
    base_cost  *= 1.0 - p -> talents.benediction * 0.02;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    p -> buffs_avenging_wrath -> trigger();
  }
};

// Consecration ============================================================

struct consecration_tick_t : public paladin_spell_t
{
  consecration_tick_t( paladin_t* p ) :
      paladin_spell_t( "consecration", p )
  {
    static rank_t ranks[] =
    {
      { 80, 8, 113, 113, 0, 0 },
      { 75, 7,  87,  87, 0, 0 },
      { 70, 6,  72,  72, 0, 0 },
      { 60, 5,  56,  56, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    aoe        = true;
    dual       = true;
    background = true;
    may_crit   = false;
    may_miss   = true;
    direct_power_mod = 1.0;
    base_spell_power_multiplier  = 0.04;
    base_attack_power_multiplier = 0.04;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    if ( result_is_hit() )
    {
      tick_dmg = direct_dmg;
      update_stats( DMG_OVER_TIME );
    }
  }
};

struct consecration_t : public paladin_spell_t
{
  action_t* consecration_tick;

  consecration_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "consecration", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_miss       = false;
    base_cost      = p -> resource_base[ RESOURCE_MANA ] * 0.22;
    base_cost     *= 1.0 - p -> talents.benediction * 0.02;
    num_ticks      = 8;
    base_tick_time = 1;
    cooldown       = 8;

    if( p -> glyphs.consecration )
    {
      num_ticks += 2;
      cooldown  += 2;
    }

    consecration_tick = new consecration_tick_t( p );
  }

  // Consecration ticks are modeled as "direct" damage, requiring a dual-spell setup.

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    consecration_tick -> execute();
    update_time( DMG_OVER_TIME );
  }
};

// Divine Plea =============================================================

struct divine_plea_t : public paladin_spell_t
{
  divine_plea_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "divine_plea", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful     = false;
    cooldown    = 60;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_divine_plea -> trigger();
  }
};

// Exorcism ================================================================

struct exorcism_t : public paladin_spell_t
{
  int art_of_war;
  int undead_demon;

  exorcism_t( paladin_t* p, const std::string& options_str ) :
    paladin_spell_t( "exorcism", p ), art_of_war(0), undead_demon(0)
  {
    option_t options[] =
    {
      { "art_of_war",   OPT_BOOL, &art_of_war   },
      { "undead_demon", OPT_BOOL, &undead_demon },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 9, 1028, 1146, 0, 0.08 },
      { 73, 8,  787,  877, 0, 0.08 },
      { 68, 7,  687,  765, 0, 0.08 },
      { 60, 6,  564,  628, 0, 0.08 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 1.5;
    cooldown = 15;
    may_crit = true;

    direct_power_mod = 1.0;
    base_attack_power_multiplier = 0.15;
    base_attack_power_multiplier = 0.15;

    base_multiplier *= 1.0 + 0.05 * p -> talents.sanctity_of_battle;

    if ( p -> glyphs.exorcism ) base_multiplier *= 1.2;
  }

  virtual double execute_time() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> buffs_the_art_of_war -> up() ) return 0;
    return paladin_spell_t::execute_time();
  }

  virtual void player_buff()
  {
    paladin_spell_t::player_buff();
    int target_race = sim -> target -> race;
    if ( target_race == RACE_UNDEAD ||
         target_race == RACE_DEMON  )
    {
      player_crit += 1.0;
    }
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_spell_t::execute();
    p -> buffs_the_art_of_war -> expire();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();

    if ( art_of_war )
      if ( ! p -> buffs_the_art_of_war -> check() )
        return false;

    if ( undead_demon )
    {
      int target_race = sim -> target -> race;
      if ( target_race != RACE_UNDEAD &&
           target_race != RACE_DEMON  )
        return false;
    }

    return paladin_spell_t::ready();
  }
};

// Holy Shock ==============================================================

struct holy_shock_t : public paladin_spell_t
{
  holy_shock_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "holy_shock", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 7, 1296, 1402, 0, 0.18 },
      { 75, 6, 1043, 1129, 0, 0.18 },
      { 70, 5,  904,  978, 0, 0.18 },
      { 64, 4,  693,  749, 0, 0.21 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    may_crit         = true;
    direct_power_mod = 1.5/3.5;
    base_cost       *= 1.0 - p -> talents.benediction * 0.02;
    cooldown         = 6;
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Paladin Character Definition
// ==========================================================================

// paladin_t::create_action ==================================================

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new auto_attack_t            ( this, options_str );
  if ( name == "avengers_shield"         ) return new avengers_shield_t        ( this, options_str );
  if ( name == "avenging_wrath"          ) return new avenging_wrath_t         ( this, options_str );
  if ( name == "consecration"            ) return new consecration_t           ( this, options_str );
  if ( name == "crusader_strike"         ) return new crusader_strike_t        ( this, options_str );
  if ( name == "divine_plea"             ) return new divine_plea_t            ( this, options_str );
  if ( name == "divine_storm"            ) return new divine_storm_t           ( this, options_str );
  if ( name == "exorcism"                ) return new exorcism_t               ( this, options_str );
  if ( name == "hammer_of_wrath"         ) return new hammer_of_wrath_t        ( this, options_str );
  if ( name == "holy_shock"              ) return new holy_shock_t             ( this, options_str );
  if ( name == "judgement"               ) return new judgement_t              ( this, options_str );

  if ( name == "seal_of_command"         ) return new paladin_seal_t( this, "seal_of_command",       SEAL_OF_COMMAND,       options_str );
  if ( name == "seal_of_justice"         ) return new paladin_seal_t( this, "seal_of_justice",       SEAL_OF_JUSTICE,       options_str );
  if ( name == "seal_of_light"           ) return new paladin_seal_t( this, "seal_of_light",         SEAL_OF_LIGHT,         options_str );
  if ( name == "seal_of_righteousness"   ) return new paladin_seal_t( this, "seal_of_righteousness", SEAL_OF_RIGHTEOUSNESS, options_str );
  if ( name == "seal_of_corruption"      ) return new paladin_seal_t( this, "seal_of_vengeance",     SEAL_OF_VENGEANCE,     options_str );
  if ( name == "seal_of_vengeance"       ) return new paladin_seal_t( this, "seal_of_vengeance",     SEAL_OF_VENGEANCE,     options_str );
  if ( name == "seal_of_wisdom"          ) return new paladin_seal_t( this, "seal_of_wisdom",        SEAL_OF_WISDOM,        options_str );

//if ( name == "aura_mastery"            ) return new aura_mastery_t           ( this, options_str );
//if ( name == "blessings"               ) return new blessings_t              ( this, options_str );
//if ( name == "concentration_aura"      ) return new concentration_aura_t     ( this, options_str );
//if ( name == "devotion_aura"           ) return new devotion_aura_t          ( this, options_str );
//if ( name == "retribution_aura"        ) return new retribution_aura_t       ( this, options_str );
//if ( name == "shield_of_righteousness" ) return new shield_of_righteousness_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// paladin_t::init_race ======================================================

void paladin_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_BLOOD_ELF:
    break;
  default:
    race = RACE_DWARF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// paladin_t::init_base =====================================================

void paladin_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_STRENGTH ] *= 1.0 + talents.divine_strength * 0.03;

  base_attack_power = ( level * 3 ) - 20;
  initial_attack_power_per_strength = 2.0;
  initial_attack_power_per_agility  = 0.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// paladin_t::reset =========================================================

void paladin_t::reset()
{
  player_t::reset();

  active_seal = SEAL_NONE;
}

// paladin_t::interrupt =====================================================

void paladin_t::interrupt()
{
  player_t::interrupt();

  if ( auto_attack ) auto_attack -> cancel();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains_divine_plea            = get_gain( "divine_plea"            );
  gains_judgements_of_the_wise = get_gain( "judgements_of_the_wise" );
  gains_seal_of_wisdom         = get_gain( "seal_of_wisdom"         );
}

// paladin_t::init_rng ======================================================

void paladin_t::init_rng()
{
  player_t::init_rng();

  rng_judgements_of_the_wise = get_rng( "judgements_of_the_wise" );
}

// paladin_t::init_glyphs ===================================================

void paladin_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "avengers_shield"         ) glyphs.avengers_shield = 1;
    else if ( n == "avenging_wrath"          ) glyphs.avenging_wrath = 1;
    else if ( n == "consecration"            ) glyphs.consecration = 1;
    else if ( n == "crusader_strike"         ) glyphs.crusader_strike = 1;
    else if ( n == "exorcism"                ) glyphs.exorcism = 1;
    else if ( n == "hammer_of_wrath"         ) glyphs.hammer_of_wrath = 1;
    else if ( n == "holy_shock"              ) glyphs.holy_shock = 1;
    else if ( n == "holy_wrath"              ) glyphs.holy_wrath = 1;
    else if ( n == "judgement"               ) glyphs.judgement = 1;
    else if ( n == "righteous_defense"       ) glyphs.righteous_defense = 1;
    else if ( n == "seal_of_command"         ) glyphs.seal_of_command = 1;
    else if ( n == "seal_of_righteousness"   ) glyphs.seal_of_righteousness = 1;
    else if ( n == "seal_of_vengeance"       ) glyphs.seal_of_vengeance = 1;
    else if ( n == "sense_undead"            ) glyphs.sense_undead = 1;
    else if ( n == "shield_of_righteousness" ) glyphs.shield_of_righteousness = 1;
    else if ( n == "wise"                    ) glyphs.wise = 1;
    else if ( n == "beacon_of_light"         ) ;
    else if ( n == "blessing_of_kings"       ) ;
    else if ( n == "blessing_of_might"       ) ;
    else if ( n == "blessing_of_wisdom"      ) ;
    else if ( n == "cleansing"               ) ;
    else if ( n == "divine_plea"             ) ;
    else if ( n == "divine_storm"            ) ;
    else if ( n == "divinity"                ) ;
    else if ( n == "flash_of_light"          ) ;
    else if ( n == "hammer_of_justice"       ) ;
    else if ( n == "hammer_of_the_righteous" ) ;
    else if ( n == "holy_light"              ) ;
    else if ( n == "lay_on_hands"            ) ;
    else if ( n == "salvation"               ) ;
    else if ( n == "seal_of_light"           ) ;
    else if ( n == "seal_of_wisdom"          ) ;
    else if ( n == "spiritual_attunement"    ) ;
    else if ( n == "turn_evil"               ) ;
    else if ( ! sim -> parent ) util_t::printf( "simcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

// paladin_t::init_scaling ==================================================

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_SPIRIT ] = 0;
}

// paladin_t::init_items ====================================================

void paladin_t::init_items()
{
  player_t::init_items();

  std::string& libram = items[ SLOT_RANGED ].encoded_name_str;

  if      ( libram == "deadly_gladiators_libram_of_fortitude"     ) librams.deadly_gladiators_fortitude = 1;
  else if ( libram == "furious_gladiators_libram_of_fortitude"    ) librams.furious_gladiators_fortitude = 1;
  else if ( libram == "hateful_gladiators_libram_of_fortitude"    ) librams.hateful_gladiators_fortitude = 1;
  else if ( libram == "libram_of_avengement"                      ) librams.avengement = 1;
  else if ( libram == "libram_of_discord"                         ) librams.discord = 1;
  else if ( libram == "libram_of_divine_judgement"                ) librams.divine_judgement = 1;
  else if ( libram == "libram_of_divine_purpose"                  ) librams.divine_purpose = 1;
  else if ( libram == "libram_of_furious_blows"                   ) librams.furious_blows = 1;
  else if ( libram == "libram_of_radiance"                        ) librams.radiance = 1;
  else if ( libram == "libram_of_reciprocation"                   ) librams.reciprocation = 1;
  else if ( libram == "libram_of_resurgence"                      ) librams.resurgence = 1;
  else if ( libram == "libram_of_valiance"                        ) librams.valiance = 1;
  else if ( libram == "relentless_gladiators_libram_of_fortitude" ) librams.relentless_gladiators_fortitude = 1;
  else if ( libram == "savage_gladiators_libram_of_fortitude"     ) librams.savage_gladiators_fortitude = 1;
  else if ( libram == "venture_co_libram_of_protection"           ) librams.venture_co_protection = 1;
  else if ( libram == "venture_co_libram_of_retribution"          ) librams.venture_co_retribution = 1;
  // To prevent warnings...
  else if ( libram == "brutal_gladiators_libram_of_fortitude" ) ;
  else if ( libram == "brutal_gladiators_libram_of_justice" ) ;
  else if ( libram == "brutal_gladiators_libram_of_vengeance" ) ;
  else if ( libram == "deadly_gladiators_libram_of_justice" ) ;
  else if ( libram == "furious_gladiators_libram_of_justice" ) ;
  else if ( libram == "gladiators_libram_of_fortitude" ) ;
  else if ( libram == "gladiators_libram_of_justice" ) ;
  else if ( libram == "gladiators_libram_of_vengeance" ) ;
  else if ( libram == "hateful_gladiators_libram_of_justice" ) ;
  else if ( libram == "libram_of_absolute_truth" ) ;
  else if ( libram == "libram_of_defiance" ) ;
  else if ( libram == "libram_of_mending" ) ;
  else if ( libram == "libram_of_obstruction" ) ;
  else if ( libram == "libram_of_renewal" ) ;
  else if ( libram == "libram_of_repentance" ) ;
  else if ( libram == "libram_of_souls_redeemed" ) ;
  else if ( libram == "libram_of_the_lightbringer" ) ;
  else if ( libram == "libram_of_the_resolute" ) ;
  else if ( libram == "libram_of_the_sacred_shield" ) ;
  else if ( libram == "libram_of_tolerance" ) ;
  else if ( libram == "libram_of_veracity" ) ;
  else if ( libram == "mericiless_gladiators_libram_of_fortitude" ) ;
  else if ( libram == "mericiless_gladiators_libram_of_justice" ) ;
  else if ( libram == "mericiless_gladiators_libram_of_vengeance" ) ;
  else if ( libram == "savage_gladiators_libram_of_justice" ) ;
  else if ( libram == "savage_gladiators_libram_of_justice" ) ;
  else if ( libram == "vengeful_gladiators_libram_of_fortitude" ) ;
  else if ( libram == "vengeful_gladiators_libram_of_justice" ) ;
  else if ( libram == "vengeful_gladiators_libram_of_vengeance" ) ;
  else if ( libram == "venture_co_libram_of_mostly_holy_deeds" ) ;
  else
  {
    util_t::printf( "simcraft: %s has unknown libram %s\n", name(), libram.c_str() );
  }
}

// paladin_t::decode_set ====================================================

int paladin_t::decode_set( item_t& item )
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

  bool is_melee = ( strstr( s, "helm"           ) || 
		    strstr( s, "shoulderplates" ) ||
		    strstr( s, "battleplate"    ) ||
		    strstr( s, "chestpiece"     ) ||
		    strstr( s, "legplates"      ) ||
		    strstr( s, "gauntlets"      ) );

  bool is_tank = ( strstr( s, "faceguard"      ) || 
		   strstr( s, "shoulderguards" ) ||
		   strstr( s, "breastplate"    ) ||
		   strstr( s, "legguards"      ) ||
		   strstr( s, "handguards"     ) );
  
  if ( strstr( s, "redemption" ) ) 
  {
    if ( is_melee ) return SET_T7_MELEE;
    if ( is_tank  ) return SET_T7_TANK;
  }
  if ( strstr( s, "aegis" ) )
  {
    if ( is_melee ) return SET_T8_MELEE;
    if ( is_tank  ) return SET_T8_TANK;
  }
  if ( strstr( s, "turalyons" ) ||
       strstr( s, "liadrins"  ) ) 
  {
    if ( is_melee ) return SET_T9_MELEE;
    if ( is_tank  ) return SET_T9_TANK;
  }

  return SET_NONE;
}

// paladin_t::composite_attack_crit ==========================================

double paladin_t::composite_attack_crit() SC_CONST
{
  double ac = player_t::composite_attack_crit();
  ac += talents.conviction         * 0.01;
  ac += talents.sanctity_of_battle * 0.01;
  return ac;
}

// paladin_t::composite_spell_crit ===========================================

double paladin_t::composite_spell_crit() SC_CONST
{
  double sc = player_t::composite_spell_crit();
  sc += talents.conviction         * 0.01;
  sc += talents.sanctity_of_battle * 0.01;
  return sc;
}

// paladin_t::composite_spell_power ==========================================

double paladin_t::composite_spell_power( int school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );
  if ( talents.sheath_of_light )
  {
    sp += composite_attack_power_multiplier() * composite_attack_power() * talents.sheath_of_light * 0.1;
  }
  return sp;
}

// paladin_t::primary_tree ==================================================

int paladin_t::primary_tree() SC_CONST
{
  return TREE_RETRIBUTION;
}

// paladin_t::regen  ========================================================

void paladin_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( buffs_divine_plea -> up() )
  {
    double amount = periodicity * resource_max[ RESOURCE_MANA ] * 0.25 / 15.0;
    resource_gain( RESOURCE_MANA, amount, gains_divine_plea );
  }
}

// paladin_t::init_buffs ====================================================

void paladin_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_avenging_wrath    = new buff_t( this, "avenging_wrath",    1, 20.0 );
  buffs_divine_plea       = new buff_t( this, "divine_plea",       1, 15.0 );
  buffs_seal_of_vengeance = new buff_t( this, "seal_of_vengeance", 5 );
  buffs_the_art_of_war    = new buff_t( this, "the_art_of_war",    1, 15.0 );
  buffs_vengeance         = new buff_t( this, "vengeance",         3, 30.0 );

  // stat_buff_t( sim, player, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )
}

// paladin_t::init_actions ==================================================

void paladin_t::init_actions()
{
  active_seal_of_command_proc       = new seal_of_command_proc_t      ( this );
  active_seal_of_justice_proc       = new seal_of_justice_proc_t      ( this );
  active_seal_of_light_proc         = new seal_of_light_proc_t        ( this );
  active_seal_of_righteousness_proc = new seal_of_righteousness_proc_t( this );
  active_seal_of_vengeance_proc     = new seal_of_vengeance_proc_t    ( this );
  active_seal_of_vengeance_dot      = new seal_of_vengeance_dot_t     ( this );
  active_seal_of_wisdom_proc        = new seal_of_wisdom_proc_t       ( this );

  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=endless_rage/food,type=fish_feast/seal_of_vengeance/auto_attack";
    int num_items = items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    if ( race == RACE_BLOOD_ELF ) action_list_str += "/arcane_torrent";
    action_list_str += "/avenging_wrath";
    action_list_str += "/hammer_of_wrath/judgement";
    if ( talents.divine_storm    ) action_list_str += "/divine_storm";
    if ( talents.crusader_strike ) action_list_str += "/crusader_strike";
    action_list_str += "/exorcism";
    action_list_str += "/consecration";
    action_list_str += "/mana_potion";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// paladin_t::get_talents_trees ==============================================

bool paladin_t::get_talent_trees( std::vector<int*>& holy, std::vector<int*>& protection, std::vector<int*>& retribution )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.spiritual_focus             ) }, {  1, NULL                                      }, {  1, NULL                                    } },
    { {  2, &( talents.seals_of_the_pure           ) }, {  2, &( talents.divine_strength              ) }, {  2, &( talents.benediction                ) } },
    { {  3, &( talents.healing_light               ) }, {  3, NULL                                      }, {  3, &( talents.improved_judgements        ) } },
    { {  4, &( talents.divine_intellect            ) }, {  4, NULL                                      }, {  4, &( talents.heart_of_the_crusader      ) } },
    { {  5, NULL                                     }, {  5, NULL                                      }, {  5, &( talents.improved_blessing_of_might ) } },
    { {  6, &( talents.aura_mastery                ) }, {  6, NULL                                      }, {  6, NULL                                    } },
    { {  7, NULL                                     }, {  7, NULL                                      }, {  7, &( talents.conviction                 ) } },
    { {  8, NULL                                     }, {  8, NULL                                      }, {  8, &( talents.seal_of_command            ) } },
    { {  9, &( talents.improved_concentration_aura ) }, {  9, NULL                                      }, {  9, NULL                                    } },
    { { 10, &( talents.improved_blessing_of_wisdom ) }, { 10, &( talents.improved_hammer_of_justice   ) }, { 10, NULL                                    } },
    { { 11, NULL                                     }, { 11, NULL                                      }, { 11, &( talents.sanctity_of_battle         ) } },
    { { 12, NULL                                     }, { 12, &( talents.blessing_of_sanctuary        ) }, { 12, &( talents.crusade                    ) } },
    { { 13, &( talents.divine_favor                ) }, { 13, &( talents.reckoning                    ) }, { 13, &( talents.two_handed_weapon_spec     ) } },
    { { 14, &( talents.sanctified_light            ) }, { 14, &( talents.sacred_duty                  ) }, { 14, &( talents.sanctified_retribution     ) } },
    { { 15, &( talents.purifying_power             ) }, { 15, &( talents.one_handed_weapon_spec       ) }, { 15, &( talents.vengeance                  ) } },
    { { 16, &( talents.holy_power                  ) }, { 16, NULL                                      }, { 16, NULL                                    } },
    { { 17, NULL                                     }, { 17, &( talents.holy_shield                  ) }, { 17, &( talents.the_art_of_war             ) } },
    { { 18, &( talents.holy_shock                  ) }, { 18, NULL                                      }, { 18, NULL                                    } },
    { { 19, NULL                                     }, { 19, NULL                                      }, { 19, &( talents.judgements_of_the_wise     ) } },
    { { 20, NULL                                     }, { 20, &( talents.combat_expertise             ) }, { 20, &( talents.fanaticism                 ) } },
    { { 21, &( talents.holy_guidance               ) }, { 21, &( talents.touched_by_the_light         ) }, { 21, &( talents.sanctified_wrath           ) } },
    { { 22, &( talents.divine_illumination         ) }, { 22, &( talents.avengers_shield              ) }, { 22, &( talents.swift_retribution          ) } },
    { { 23, &( talents.judgements_of_the_pure      ) }, { 23, &( talents.guarded_by_the_light         ) }, { 23, &( talents.crusader_strike            ) } },
    { { 24, NULL                                     }, { 24, NULL                                      }, { 24, &( talents.sheath_of_light            ) } },
    { { 25, &( talents.enlightened_judgements      ) }, { 25, &( talents.judgements_of_the_just       ) }, { 25, &( talents.righteous_vengeance        ) } },
    { { 26, NULL                                     }, { 26, &( talents.hammer_of_the_righteous      ) }, { 26, &( talents.divine_storm               ) } },
    { {  0, NULL                                     }, {  0, NULL                                      }, {  0, NULL                                    } }
  };

  return get_talent_translation( holy, protection, retribution, translation );
}

// paladin_t::get_options ====================================================

std::vector<option_t>& paladin_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t paladin_options[] =
    {
      // @option_doc loc=player/paladin/talents title="Talents"
      { "aura_mastery",                OPT_INT, &( talents.aura_mastery                ) },
      { "avengers_shield",             OPT_INT, &( talents.avengers_shield             ) },
      { "benediction;",                OPT_INT, &( talents.benediction                 ) },
      { "blessing_of_sanctuary",       OPT_INT, &( talents.blessing_of_sanctuary       ) },
      { "combat_expertise",            OPT_INT, &( talents.combat_expertise            ) },
      { "conviction",                  OPT_INT, &( talents.conviction                  ) },
      { "crusade",                     OPT_INT, &( talents.crusade                     ) },
      { "crusader_strike",             OPT_INT, &( talents.crusader_strike             ) },
      { "divine_favor",                OPT_INT, &( talents.divine_favor                ) },
      { "divine_illumination",         OPT_INT, &( talents.divine_illumination         ) },
      { "divine_intellect",            OPT_INT, &( talents.divine_intellect            ) },
      { "divine_storm",                OPT_INT, &( talents.divine_storm                ) },
      { "divine_strength",             OPT_INT, &( talents.divine_strength             ) },
      { "enlightened_judgements",      OPT_INT, &( talents.enlightened_judgements      ) },
      { "fanaticism",                  OPT_INT, &( talents.fanaticism                  ) },
      { "guarded_by_the_light",        OPT_INT, &( talents.guarded_by_the_light        ) },
      { "hammer_of_the_righteous",     OPT_INT, &( talents.hammer_of_the_righteous     ) },
      { "healing_light",               OPT_INT, &( talents.healing_light               ) },
      { "heart_of_the_crusader",       OPT_INT, &( talents.heart_of_the_crusader       ) },
      { "holy_guidance",               OPT_INT, &( talents.holy_guidance               ) },
      { "holy_power",                  OPT_INT, &( talents.holy_power                  ) },
      { "holy_shield",                 OPT_INT, &( talents.holy_shield                 ) },
      { "holy_shock",                  OPT_INT, &( talents.holy_shock                  ) },
      { "improved_blessing_of_might",  OPT_INT, &( talents.improved_blessing_of_might  ) },
      { "improved_blessing_of_wisdom", OPT_INT, &( talents.improved_blessing_of_wisdom ) },
      { "improved_concentration_aura", OPT_INT, &( talents.improved_concentration_aura ) },
      { "improved_hammer_of_justice",  OPT_INT, &( talents.improved_hammer_of_justice  ) },
      { "improved_judgements",         OPT_INT, &( talents.improved_judgements         ) },
      { "judgements_of_the_just",      OPT_INT, &( talents.judgements_of_the_just      ) },
      { "judgements_of_the_pure",      OPT_INT, &( talents.judgements_of_the_pure      ) },
      { "judgements_of_the_wise",      OPT_INT, &( talents.judgements_of_the_wise      ) },
      { "one_handed_weapon_spec",      OPT_INT, &( talents.one_handed_weapon_spec      ) },
      { "purifying_power",             OPT_INT, &( talents.purifying_power             ) },
      { "reckoning",                   OPT_INT, &( talents.reckoning                   ) },
      { "righteous_vengeance",         OPT_INT, &( talents.righteous_vengeance         ) },
      { "sacred_duty",                 OPT_INT, &( talents.sacred_duty                 ) },
      { "sanctified_light",            OPT_INT, &( talents.sanctified_light            ) },
      { "sanctified_retribution",      OPT_INT, &( talents.sanctified_retribution      ) },
      { "sanctified_wrath",            OPT_INT, &( talents.sanctified_wrath            ) },
      { "sanctity_of_battle",          OPT_INT, &( talents.sanctity_of_battle          ) },
      { "seal_of_command",             OPT_INT, &( talents.seal_of_command             ) },
      { "seals_of_the_pure",           OPT_INT, &( talents.seals_of_the_pure           ) },
      { "sheath_of_light",             OPT_INT, &( talents.sheath_of_light             ) },
      { "spiritual_focus",             OPT_INT, &( talents.spiritual_focus             ) },
      { "swift_retribution",           OPT_INT, &( talents.swift_retribution           ) },
      { "the_art_of_war",              OPT_INT, &( talents.the_art_of_war              ) },
      { "touched_by_the_light",        OPT_INT, &( talents.touched_by_the_light        ) },
      { "two_handed_weapon_spec",      OPT_INT, &( talents.two_handed_weapon_spec      ) },
      { "vengeance",                   OPT_INT, &( talents.vengeance                   ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, paladin_options );
  }
  return options;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

player_t* player_t::create_paladin( sim_t* sim, const std::string& name, int race_type )
{
  return new paladin_t( sim, name, race_type );
}

// player_t::paladin_init ====================================================

void player_t::paladin_init( sim_t* sim )
{
  sim -> auras.sanctified_retribution = new buff_t( sim, "sanctified_retribution", 1 );
  sim -> auras.swift_retribution      = new buff_t( sim, "swift_retribution",      1 );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.blessing_of_kings  = new buff_t( p, "blessing_of_kings",  1 );
    p -> buffs.blessing_of_might  = new buff_t( p, "blessing_of_might",  1 );
    p -> buffs.blessing_of_wisdom = new buff_t( p, "blessing_of_wisdom", 1 );
  }

  target_t* t = sim -> target;
  t -> debuffs.judgement_of_wisdom = new debuff_t( sim, "judgement_of_wisdom", 1 );
}

// player_t::paladin_combat_begin ============================================

void player_t::paladin_combat_begin( sim_t* sim )
{
  if( sim -> overrides.sanctified_retribution ) sim -> auras.sanctified_retribution -> override();
  if( sim -> overrides.swift_retribution      ) sim -> auras.swift_retribution      -> override();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.blessing_of_kings  ) p -> buffs.blessing_of_kings  -> override();
      if ( sim -> overrides.blessing_of_might  ) p -> buffs.blessing_of_might  -> override( 1, 688 );
      if ( sim -> overrides.blessing_of_wisdom ) p -> buffs.blessing_of_wisdom -> override( 1, 91 * 1.2 );
    }
  }

  target_t* t = sim -> target;
  if ( sim -> overrides.judgement_of_wisdom ) t -> debuffs.judgement_of_wisdom -> override();
}
