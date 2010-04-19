// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

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
  action_t* active_holy_shield;
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
  buff_t* buffs_divine_favor;
  buff_t* buffs_divine_illumination;
  buff_t* buffs_divine_plea;
  buff_t* buffs_holy_shield;
  buff_t* buffs_judgements_of_the_pure;
  buff_t* buffs_reckoning;
  buff_t* buffs_redoubt;
  buff_t* buffs_holy_vengeance;
  buff_t* buffs_the_art_of_war;
  buff_t* buffs_vengeance;
  buff_t* buffs_libram_of_avengement;
  buff_t* buffs_libram_of_divine_judgement;
  buff_t* buffs_libram_of_fortitude;
  buff_t* buffs_libram_of_furious_blows;
  buff_t* buffs_libram_of_reciprocation;
  buff_t* buffs_libram_of_valiance;
  buff_t* buffs_tier8_4pc_tank;
  buff_t* buffs_libram_of_three_truths;

  // Gains
  gain_t* gains_divine_plea;
  gain_t* gains_judgements_of_the_wise;
  gain_t* gains_seal_of_command_glyph;
  gain_t* gains_seal_of_wisdom;

  // Procs
  proc_t* procs_parry_haste;
  proc_t* procs_tier10_2pc;

  // Random Number Generation
  rng_t* rng_guarded_by_the_light;
  rng_t* rng_judgements_of_the_wise;
  rng_t* rng_tier10_2pc;

  // Cooldowns
  cooldown_t* cooldowns_divine_storm;

  struct talents_t
  {
    int anticipation;
    int avengers_shield;
    int benediction; 
    int combat_expertise;
    int conviction;
    int crusade;
    int crusader_strike;
    int deflection;
    int divine_favor;
    int divine_illumination;
    int divine_intellect;
    int divine_storm;
    int divine_strength;
    int fanaticism;
    int guarded_by_the_light;
    int enlightened_judgements;
    int hammer_of_the_righteous;
    int healing_light;
    int heart_of_the_crusader;
    int holy_guidance;
    int holy_power;
    int holy_shock;
    int improved_hammer_of_justice;
    int improved_judgements;
    int judgements_of_the_just;
    int judgements_of_the_pure;
    int judgements_of_the_wise;
    int one_handed_weapon_spec;
    int purifying_power;
    int reckoning;
    int redoubt;
    int righteous_vengeance;
    int sacred_duty;
    int sanctified_light;
    int sanctified_wrath;
    int sanctity_of_battle;
    int seal_of_command;
    int seals_of_the_pure;
    int sheath_of_light;
    int the_art_of_war;
    int touched_by_the_light;
    int toughness;
    int two_handed_weapon_spec;
    int vengeance;

    // NYI
    int ardent_defender;
    int aura_mastery;
    int blessed_life;
    int blessing_of_sanctuary;
    int divine_purpose;
    int eye_for_an_eye;
    int holy_shield;
    int improved_blessing_of_might;
    int improved_blessing_of_wisdom;
    int improved_concentration_aura;
    int improved_devotion_aura;
    int improved_righteous_fury;
    int sanctified_retribution;
    int shield_of_the_templar;
    int spiritual_attunement;
    int swift_retribution;
    int vindication;
    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int avengers_shield;
    int avenging_wrath;
    int consecration;
    int crusader_strike;
    int exorcism;
    int hammer_of_wrath;
    int holy_shock;
    int holy_wrath;
    int judgement;
    int seal_of_command;
    int seal_of_righteousness;
    int seal_of_vengeance;
    int sense_undead;
    int shield_of_righteousness;
    int the_wise;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct librams_t
  {
    int deadly_gladiators_fortitude;
    int furious_gladiators_fortitude;
    int relentless_gladiators_fortitude;
    int avengement;
    int discord;
    int divine_judgement;
    int divine_purpose;
    int furious_blows;
    int radiance;
    int reciprocation;
    int resurgence;
    int three_truths;
    int valiance;
    int venture_co_protection;
    int venture_co_retribution;
    int wracking;
    librams_t() { memset( ( void* ) this, 0x0, sizeof( librams_t ) ); }
  };
  librams_t librams;

  bool  tier10_2pc_procs_from_strikes;

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

    tier10_2pc_procs_from_strikes     = false;

    cooldowns_divine_storm = get_cooldown( "divine_storm" );
  }

  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_glyphs();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_items();
  virtual void      init_buffs();
  virtual void      init_actions();
  virtual void      init_rating();
  virtual void      reset();
  virtual void      interrupt();
  virtual double    composite_spell_power( int school ) SC_CONST;
  virtual double    composite_block_value() SC_CONST;
  virtual double    composite_tank_block() SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_HYBRID; }
  virtual int       primary_tree() SC_CONST;
  virtual void      regen( double periodicity );
  virtual int       target_swing();
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_tier10_2pc ======================================================

static void trigger_tier10_2pc( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> set_bonus.tier10_2pc_melee() )
    return;

  if ( ! p -> rng_tier10_2pc -> roll( 0.40 ) )
    return;

  p -> procs_tier10_2pc -> occur();

  p -> cooldowns_divine_storm -> reset();
}

// trigger_guarded_by_the_light ============================================

static void trigger_guarded_by_the_light( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.guarded_by_the_light ) return;

  if ( ! p -> buffs_divine_plea -> check() ) return;

  if( ! p -> rng_guarded_by_the_light -> roll( p -> talents.guarded_by_the_light / 2.0 ) ) return;

  p -> buffs_divine_plea -> trigger();
}

// trigger_judgements_of_the_wise ==========================================

static void trigger_judgements_of_the_wise( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.judgements_of_the_wise ) return;
  if ( ! p -> rng_judgements_of_the_wise -> roll( p -> talents.judgements_of_the_wise / 3.0 ) ) return;

  p -> resource_gain( RESOURCE_MANA, 0.25 * p -> resource_base[ RESOURCE_MANA ], p -> gains_judgements_of_the_wise );
  p -> trigger_replenishment();
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
    may_crit = true;
    base_dd_min = base_dd_max = 1;
    if ( p -> talents.crusade )
    {
      int r = sim -> target -> race;
      bool crusade_race = ( r == RACE_HUMANOID || r == RACE_DEMON || r == RACE_UNDEAD || r == RACE_ELEMENTAL );
      base_multiplier *= 1.0 + 0.01 * p -> talents.crusade * ( crusade_race ? 2 : 1 );
    }
    if ( p -> glyphs.sense_undead )
    {
      if ( sim -> target -> race == RACE_UNDEAD ) base_multiplier *= 1.01;
    }
    if ( p -> main_hand_weapon.group() == WEAPON_1H ) 
    {
      base_multiplier *= 1.0 + util_t::talent_rank( p -> talents.one_handed_weapon_spec, 3, 0.04, 0.07, 0.10 );
    }
  }

  virtual double haste() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double h = attack_t::haste();
    if ( p -> buffs_judgements_of_the_pure -> check() )
    {
      h *= 1.0 / ( 1.0 + p -> talents.judgements_of_the_pure * 0.03 );
    }
    return h;
  }

  virtual double resistance() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double r = attack_t::resistance();
    if ( r > 0 && p -> buffs_avenging_wrath -> check() )
    {
      r *= 1.0 - p -> talents.sanctified_wrath * 0.25;
    }
    return r;
  }

  virtual void execute()
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
        switch ( p -> active_seal )
        {
        case SEAL_OF_COMMAND:       p -> active_seal_of_command_proc       -> execute(); break;
        case SEAL_OF_JUSTICE:       p -> active_seal_of_justice_proc       -> execute(); break;
        case SEAL_OF_LIGHT:         p -> active_seal_of_light_proc         -> execute(); break;
        case SEAL_OF_RIGHTEOUSNESS: p -> active_seal_of_righteousness_proc -> execute(); break;
        case SEAL_OF_WISDOM:        p -> active_seal_of_wisdom_proc        -> execute(); break;
        case SEAL_OF_VENGEANCE:     if ( p -> buffs_holy_vengeance         -> stack() >= 1 ) p -> active_seal_of_vengeance_proc -> execute(); break;
        default:;
        }
      }
    }
  }
  
  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    attack_t::player_buff();
    if ( p -> talents.two_handed_weapon_spec )
    {
      if ( weapon && weapon -> group() == WEAPON_2H )
      {
        player_multiplier *= 1.0 + 0.02 * p -> talents.two_handed_weapon_spec;
      }
    }
    if ( p -> active_seal == SEAL_OF_VENGEANCE && p -> glyphs.seal_of_vengeance )
    {
      player_expertise += 0.10;
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
};

// trigger_righteous_vengeance =============================================

static void trigger_righteous_vengeance( action_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.righteous_vengeance ) return;

  struct righteous_vengeance_t : public paladin_attack_t
  {
    righteous_vengeance_t( paladin_t* p ) : paladin_attack_t( "righteous_vengeance", p, SCHOOL_HOLY )
    {
      background     = true;
      proc           = true;
      trigger_gcd    = 0;
      base_tick_time = 2;
      num_ticks      = 4;
      tick_may_crit  = p -> set_bonus.tier9_2pc_melee() != 0;
      reset(); // required since construction occurs after player_t::init()
    }
    virtual double total_multiplier() SC_CONST { return 1.0; } // No double-dip!
  };

  if ( ! p -> active_righteous_vengeance ) p -> active_righteous_vengeance = new righteous_vengeance_t( p );

  double dmg = p -> talents.righteous_vengeance * 0.1 * a -> direct_dmg;

  if ( p -> active_righteous_vengeance -> ticking )
  {
    int remaining_ticks = ( p -> active_righteous_vengeance -> num_ticks - 
                            p -> active_righteous_vengeance -> current_tick );

    dmg += p -> active_righteous_vengeance -> base_td * remaining_ticks;

#if 0 // DoT is refreshed like Shadow Word: Pain

    p -> active_righteous_vengeance -> refresh_duration();
  }
  else 
  {
    p -> active_righteous_vengeance -> player_buff();
    p -> active_righteous_vengeance -> schedule_tick();
  } 
#else // DoT is rescheduled like Ignite
    p -> active_righteous_vengeance -> cancel();
  }

  p -> active_righteous_vengeance -> player_buff(); 
  p -> active_righteous_vengeance -> schedule_tick();
#endif
  p -> active_righteous_vengeance -> base_td = dmg / 4;
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

  virtual double execute_time() SC_CONST
  {
    if ( ! player -> in_combat ) return 0.01;
    return paladin_attack_t::execute_time();
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_tier10_2pc( this ); 

      if ( result == RESULT_CRIT )
      {
        p -> buffs_the_art_of_war -> trigger();
      }
      if ( p -> active_seal == SEAL_OF_VENGEANCE )
      {
        p -> active_seal_of_vengeance_dot -> execute();
      }
      trigger_guarded_by_the_light( this );
    }
    if ( p -> buffs_reckoning -> up() )
    {
      p -> buffs_reckoning -> decrement();
      proc = true;
      paladin_attack_t::execute();
      proc = false;
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
    p -> main_hand_attack = new melee_t( p );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
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
    init_rank( ranks, 48827 );

    trigger_seal = true;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown -> duration = 30;

    direct_power_mod = 1.0;
    base_spell_power_multiplier  = 0.15;
    base_attack_power_multiplier = 0.15;

    if ( p -> glyphs.avengers_shield ) base_multiplier *= 2.0;
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

    cooldown -> duration = 4;

    base_multiplier *= 1.0 + 0.05 * p -> talents.sanctity_of_battle;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    if ( p -> set_bonus.tier8_4pc_melee() ) base_crit += 0.10;

    if ( p -> glyphs.crusader_strike ) base_cost *= 0.80;

    if ( p -> librams.radiance ) base_dd_adder += 79;

    id = 35395;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( result_is_hit() )
    {
      if ( p -> tier10_2pc_procs_from_strikes )
      {
        trigger_tier10_2pc( this );
      }
      if ( result == RESULT_CRIT )
      {
        trigger_righteous_vengeance( this );
      }
    }
    if ( p -> librams.    deadly_gladiators_fortitude ) p -> buffs_libram_of_fortitude -> trigger( 1, 120 );
    if ( p -> librams.   furious_gladiators_fortitude ) p -> buffs_libram_of_fortitude -> trigger( 1, 144 );
    if ( p -> librams.relentless_gladiators_fortitude ) p -> buffs_libram_of_fortitude -> trigger( 1, 172 );
    if ( p -> librams.                   three_truths ) p -> buffs_libram_of_three_truths -> trigger();
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

    cooldown -> duration = 10;

    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;

    if ( p -> set_bonus.tier7_2pc_melee() ) base_multiplier *= 1.1;
    if ( p -> set_bonus.tier8_4pc_melee() ) base_crit += 0.10;

    if ( p -> librams.discord                ) base_dd_adder += 235;
    if ( p -> librams.venture_co_retribution ) base_dd_adder += 81;

    id = 53385;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( result_is_hit() )
    {
      if ( p -> tier10_2pc_procs_from_strikes )
      {
        trigger_tier10_2pc( this );
      }
      if ( result == RESULT_CRIT )
      {
        trigger_righteous_vengeance( this );
      }
    }
  }
};

// Hammer of Justice =======================================================

struct hammer_of_justice_t : public paladin_attack_t
{
  hammer_of_justice_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "hammer_of_justice", p, SCHOOL_HOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.03;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;

    cooldown -> duration = 60 - p -> talents.improved_hammer_of_justice * 10;

    id = 10308;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return paladin_attack_t::ready();
  }
};

// Hammer of the Righteous =================================================

struct hammer_of_the_righteous_t : public paladin_attack_t
{
  hammer_of_the_righteous_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "hammer_of_the_righteous", p, SCHOOL_HOLY )
  {
    check_talent( p -> talents.hammer_of_the_righteous );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );

    base_cost        = p -> resource_base[ RESOURCE_MANA ] * 0.06;
    base_cost       *= 1.0 - p -> talents.benediction * 0.02;
    base_multiplier *= 4.0 / weapon -> swing_time; // This effectively gets us 4x Base Weapon-DPS 

    cooldown -> duration = 6;

    if ( p -> set_bonus.tier7_2pc_tank() ) base_multiplier *= 1.10;
    if ( p -> set_bonus.tier9_2pc_tank() ) base_multiplier *= 1.05;

    id = 53595;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( result_is_hit() )
    {
      if ( p -> active_seal == SEAL_OF_VENGEANCE )
      {
        p -> active_seal_of_vengeance_dot -> execute();
      }
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
    init_rank( ranks, 48806 );

	weapon            = &( p -> main_hand_weapon );
	weapon_multiplier	 *= 0.0;

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    base_crit += 0.25 * p -> talents.sanctified_wrath;

    cooldown -> duration = 6;

    direct_power_mod = 1.0;
    base_spell_power_multiplier  = 0.15;
    base_attack_power_multiplier = 0.15;

    if ( p -> set_bonus.tier8_2pc_melee() ) base_multiplier *= 1.10;

    if ( p -> glyphs.hammer_of_wrath ) base_cost = 0;
  }

  virtual void update_ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> glyphs.avenging_wrath )
    {
      cooldown -> duration = p -> buffs_avenging_wrath -> check() ? 3 : 6;
    }
    paladin_attack_t::update_ready();
  }

  virtual bool ready()
  {
    if ( sim -> target -> health_percentage() > 20 )
      return false;

    return paladin_attack_t::ready();
  }
};

// Shield of Righteousness =================================================

struct shield_of_righteousness_t : public paladin_attack_t
{
  shield_of_righteousness_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "shield_of_righteousness", p, SCHOOL_HOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 2, 520, 520, 0, 0.06 },
      { 75, 1, 390, 390, 0, 0.06 },
    };
    init_rank( ranks, 61411 );

    may_parry = false;
    may_dodge = false;
    may_block = false;

    base_cost *= 1.0 - p -> talents.benediction * 0.02;

    cooldown -> duration = 6;

    if ( p -> glyphs.shield_of_righteousness ) base_cost *= 0.20;

    if ( p -> librams.venture_co_protection )
    {
      base_dd_min += 96;
      base_dd_max += 96;
    }
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    base_dd_adder = p -> composite_block_value();
    if ( p -> set_bonus.tier8_4pc_tank() ) p -> buffs_tier8_4pc_tank -> trigger();
    paladin_attack_t::execute();
  }

  virtual bool ready()
  {
    if ( player -> main_hand_weapon.group() == WEAPON_2H ) return false;
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

    if ( seal_type == SEAL_OF_WISDOM && p -> glyphs.the_wise )
    {
      base_cost *= 0.50;
    }
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
    base_multiplier  *= 1.0 + ( p -> talents.judgements_of_the_pure * 0.05 +
                                p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    id = 20424;
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

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.05;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + ( p -> talents.the_art_of_war         * 0.05 +
                               p -> talents.judgements_of_the_pure * 0.05 +
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.13;
    base_attack_power_multiplier = 0.08;

    cooldown -> duration = 10 - p -> talents.improved_judgements;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown -> duration--;
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::execute();
    if ( p -> glyphs.seal_of_command )
    {
      p -> resource_gain( RESOURCE_MANA, 0.08 * p -> resource_base[ RESOURCE_MANA ], p -> gains_seal_of_command_glyph );
    }
    if ( p -> librams.divine_judgement )
    {
      p -> buffs_libram_of_divine_judgement -> trigger();
    }
    if ( p -> librams.reciprocation )
    {
      p -> buffs_libram_of_reciprocation -> trigger();
    }
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

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    id = 20164;
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

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + ( p -> talents.the_art_of_war         * 0.05 + 
                               p -> talents.judgements_of_the_pure * 0.05 +
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );   

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.32;
    base_attack_power_multiplier = 0.20;

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    cooldown -> duration = 10 - p -> talents.improved_judgements;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown -> duration--;
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
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

    base_multiplier *= 1.0 + ( p -> talents.judgements_of_the_pure * 0.05 +
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    base_spell_power_multiplier = 0.15;
    base_attack_power_multiplier = 0.15;

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    id = 20165;
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

    base_cost  = 0.05 * p -> resource_base[ RESOURCE_MANA ];
    base_cost *= 1.0 - 0.02 * p -> talents.benediction;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + ( p -> talents.the_art_of_war         * 0.05 +
                               p -> talents.judgements_of_the_pure * 0.05 + 
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.25;
    base_attack_power_multiplier = 0.16;

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    cooldown -> duration = 10 - p -> talents.improved_judgements;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown -> duration--;
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
  }
};

// Seal of Righteousness ====================================================

struct seal_of_righteousness_proc_t : public paladin_attack_t
{
  seal_of_righteousness_proc_t( paladin_t* p ) :
    paladin_attack_t( "seal_of_righteousness_proc", p, SCHOOL_HOLY )
  {
    background  = true;
    proc        = true;
    trigger_gcd = 0;

    base_multiplier *= p -> main_hand_weapon.swing_time;
    base_multiplier *= 1.0 + ( p -> talents.judgements_of_the_pure * 0.05 +
                               p -> talents.seals_of_the_pure      * 0.03 + 
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.044;
    base_attack_power_multiplier = 0.022;

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    if ( p -> set_bonus.tier8_2pc_tank() ) base_multiplier *= 1.10;

    if ( p -> glyphs.seal_of_righteousness ) base_multiplier *= 1.10;

    if ( p -> librams.divine_purpose ) base_spell_power += 94;

    id = 21084;
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

    base_cost  = 0.05 * p -> resource_base[ RESOURCE_MANA ];
    base_cost *= 1.0 - 0.02 * p -> talents.benediction;

    base_multiplier *= 1.0 + ( p -> talents.the_art_of_war         * 0.05 +
                               p -> talents.judgements_of_the_pure * 0.05 +
                               p -> talents.seals_of_the_pure      * 0.03 +
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.32;
    base_attack_power_multiplier = 0.20;

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    cooldown -> duration = 10 - p -> talents.improved_judgements;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown -> duration--;
    if ( p -> set_bonus.tier8_2pc_tank()  ) base_multiplier *= 1.10;
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;

    if ( p -> librams.divine_purpose ) base_spell_power += 94;
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

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier *= 0.0;

    base_td = 1;
    num_ticks = 5;
    base_tick_time = 3;
    
    tick_power_mod = 1.0;
    base_spell_power_multiplier = 0.013;
    base_attack_power_multiplier = 0.025;

    base_multiplier *= 1.0 + ( p -> talents.judgements_of_the_pure * 0.05 +
                               p -> talents.seals_of_the_pure      * 0.03 + 
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    if ( p -> set_bonus.tier8_2pc_tank() ) base_multiplier *= 1.10;

  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= p -> buffs_holy_vengeance -> stack();
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    player_buff();
    target_debuff( DMG_DIRECT );
    calculate_result();
    if ( result_is_hit() )
    {
      p -> buffs_holy_vengeance -> trigger();
      player_buff(); // Re-calculate with the new stacks of the Seal of Vengeance.
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

  virtual void tick()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::tick();
    if ( p -> librams.valiance ) 
    {
      p -> buffs_libram_of_valiance -> trigger();
    }
  }

  virtual void last_tick()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::last_tick();
    p -> buffs_holy_vengeance -> expire();
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

    base_multiplier *= 1.0 + ( p -> talents.judgements_of_the_pure      * 0.05 +
                               p -> talents.seals_of_the_pure           * 0.03 + 
                               p -> set_bonus.tier10_4pc_melee()        * 0.10 );

    if ( p -> set_bonus.tier8_2pc_tank() ) base_multiplier *= 1.10;

    id = 31801;
  }
  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= p -> buffs_holy_vengeance -> stack() / 5;
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

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + ( p -> talents.the_art_of_war         * 0.05 +
                               p -> talents.judgements_of_the_pure * 0.05 +
                               p -> talents.seals_of_the_pure      * 0.03 +
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.22;
    base_attack_power_multiplier = 0.14;

    cooldown -> duration = 10 - p -> talents.improved_judgements;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown -> duration--;
    if ( p -> set_bonus.tier8_2pc_tank()  ) base_multiplier *= 1.10;
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_attack_t::player_buff();
    player_multiplier *= 1.0 + p -> buffs_holy_vengeance -> stack() * 0.10;
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

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    id = 21066;
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

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + ( p -> talents.the_art_of_war         * 0.05 +
                               p -> talents.judgements_of_the_pure * 0.05 +
                               p -> set_bonus.tier10_4pc_melee()   * 0.10 );

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.25;
    base_attack_power_multiplier = 0.16;

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier = 0.0;
	  
    cooldown -> duration = 10 - p -> talents.improved_judgements;

    if ( p -> glyphs.judgement ) base_multiplier *= 1.10;

    if ( p -> set_bonus.tier7_4pc_melee() ) cooldown -> duration--;
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
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
    return 0;
  }

  virtual double cost() SC_CONST 
  { 
    action_t* seal = active_seal(); 
    if ( ! seal ) return 0.0;
    return seal -> cost(); 
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    target_t* t = sim -> target;
    action_t* seal = active_seal();

    if ( ! seal )
      return;

    seal -> execute();

    if ( seal -> result_is_hit() )
    {
      if ( seal -> result == RESULT_CRIT )
      {
        trigger_righteous_vengeance( seal );
      }
      if ( p -> talents.judgements_of_the_pure ) 
      {
        p -> buffs_judgements_of_the_pure -> trigger();
      }
      if ( p -> talents.heart_of_the_crusader ) 
      {
        t -> debuffs.heart_of_the_crusader -> trigger();
      }
      if ( p -> talents.judgements_of_the_just ) 
      {
        t -> debuffs.judgements_of_the_just -> trigger();
      }
      // FIXME! Assume JoW for now unless override set to 0.
      if ( sim -> overrides.judgement_of_wisdom )
        t -> debuffs.judgement_of_wisdom -> trigger();
    }
    trigger_judgements_of_the_wise( seal );
    
    if ( p -> librams.avengement    ) p -> buffs_libram_of_avengement    -> trigger();
    if ( p -> librams.furious_blows ) p -> buffs_libram_of_furious_blows -> trigger();

    p -> last_foreground_action = seal; // Necessary for DPET calculations.
  }

  virtual bool ready()
  {
    action_t* seal = active_seal();
    if( ! seal ) return false;
    if( ! seal -> ready() ) return false;
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
    if ( p -> glyphs.sense_undead )
    {
      if ( sim -> target -> race == RACE_UNDEAD ) base_multiplier *= 1.01;
    }
    if ( p -> main_hand_weapon.group() == WEAPON_1H ) 
    {
      base_multiplier *= 1.0 + util_t::talent_rank( p -> talents.one_handed_weapon_spec, 3, 0.04, 0.07, 0.10 );
    }
    base_crit += p -> talents.holy_power * 0.01;
  }

  virtual double haste() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double h = spell_t::haste();
    if ( p -> buffs_judgements_of_the_pure -> check() )
    {
      h *= 1.0 / ( 1.0 + p -> talents.judgements_of_the_pure * 0.03 );
    }
    return h;
  }

  virtual double cost() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double c = spell_t::cost();
    if ( c > 0 && p -> buffs_divine_illumination -> up() ) c *= 0.5;
    return c;
  }

  virtual double resistance() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double r = spell_t::resistance();
    if ( r > 0 && p -> buffs_avenging_wrath -> check() )
    {
      r *= 1.0 - p -> talents.sanctified_wrath * 0.25;
    }
    return r;
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

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    spell_t::execute();
    if ( result == RESULT_CRIT )
    {
      p -> buffs_vengeance -> trigger();
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

    trigger_gcd = 0;
    harmful = false;
    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.08;
    base_cost *= 1.0 - p -> talents.benediction * 0.02;
    cooldown -> duration = 180 - 30 * p -> talents.sanctified_wrath;

    id = 31884;
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
    init_rank( ranks, 48819 );

    aoe        = true;
    dual       = true;
    background = true;
    may_crit   = false;
    may_miss   = true;
    direct_power_mod = 1.0;
    base_spell_power_multiplier  = 0.04;
    base_attack_power_multiplier = 0.04;

    if ( p -> librams.resurgence ) base_spell_power += 141;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    if ( result_is_hit() )
    {
      tick_dmg = direct_dmg;     
    }
    update_stats( DMG_OVER_TIME );
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
    base_cost     *= 1.0 - ( p -> talents.benediction     * 0.02 +
                             p -> talents.purifying_power * 0.05 );
    num_ticks      = 8;
    base_tick_time = 1;
    cooldown -> duration = 8;

    if ( p -> glyphs.consecration )
    {
      num_ticks += 2;
      cooldown -> duration += 2;
    }

    consecration_tick = new consecration_tick_t( p );

    id = 48819;
  }

  // Consecration ticks are modeled as "direct" damage, requiring a dual-spell setup.

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    consecration_tick -> execute();
    update_time( DMG_OVER_TIME );
  }
};

// Divine Favor ============================================================

struct divine_favor_t : public paladin_spell_t
{
  divine_favor_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "divine_favor", p )
  {
    check_talent( p -> talents.divine_favor );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
    trigger_gcd = 0;
    cooldown -> duration = 120;

    id = 20216;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_divine_favor -> trigger();
  }
};

// Divine Illumination =====================================================

struct divine_illumination_t : public paladin_spell_t
{
  divine_illumination_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "divine_illumination", p )
  {
    check_talent( p -> talents.divine_illumination );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
    trigger_gcd = 0;
    cooldown -> duration = 180;

    id = 31842;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_divine_illumination -> trigger();
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

    harmful = false;
    trigger_gcd = 0;
    cooldown -> duration = 60;

    id = 54428;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_divine_plea -> trigger();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> buffs_divine_plea -> check() ) return false;
    return paladin_spell_t::ready();
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
      { 80, 9, 1033, 1151, 0, 0.08 }, // Dummy level 80 rank.
      { 79, 9, 1028, 1146, 0, 0.08 },
      { 73, 8,  787,  877, 0, 0.08 },
      { 68, 7,  687,  765, 0, 0.08 },
      { 60, 6,  564,  628, 0, 0.08 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48801 );

	 weapon            = &( p -> main_hand_weapon );
	 weapon_multiplier *= 0.0;
	  
    may_crit = true;
    base_execute_time = 1.5;

    cooldown -> duration  = 15;
    cooldown -> duration *= 1.0 - p -> talents.purifying_power * 0.33 / 2;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.15;
    base_attack_power_multiplier = 0.15;

    base_multiplier *= 1.0 + 0.05 * p -> talents.sanctity_of_battle;

    if ( p -> set_bonus.tier8_2pc_melee() ) base_multiplier *= 1.10;

    if ( p -> glyphs.exorcism ) base_multiplier *= 1.20;

    if ( p -> librams.wracking ) base_spell_power += 120;
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

// Holy Shield =============================================================

struct holy_shield_discharge_t : public paladin_spell_t
{
  holy_shield_discharge_t( paladin_t* p ) :
      paladin_spell_t( "holy_shield_discharge", p )
  {
    static rank_t ranks[] =
    {
      { 80, 6, 274, 274, 0, 0 },
      { 75, 5, 235, 235, 0, 0 },
      { 70, 4, 208, 208, 0, 0 },
      { 60, 3, 157, 157, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48952 );

    proc = true;
    background = true;
    may_crit = true;
    trigger_gcd = 0;
    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.09;
    base_attack_power_multiplier = 0.056;
  }
};

struct holy_shield_t : public paladin_spell_t
{
  holy_shield_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "holy_shield", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful  = false;
    cooldown -> duration = 8;

    id = 48952;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_holy_shield -> trigger( 8 );
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> buffs_holy_shield -> current_stack > 1 ) return false;
    return paladin_spell_t::ready();
  }
};

// Holy Shock ==============================================================

struct holy_shock_t : public paladin_spell_t
{
  holy_shock_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "holy_shock", p )
  {
    check_talent( p -> talents.holy_shock );

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
    init_rank( ranks, 48825 );

    may_crit         = true;
    direct_power_mod = 1.5/3.5;
    base_cost       *= 1.0 - p -> talents.benediction * 0.02;
    base_multiplier *= 1.0 + p -> talents.healing_light * 0.04;
    base_crit       += 1.0 + p -> talents.sanctified_light * 0.02;

    cooldown -> duration = 6;

    if ( p -> glyphs.holy_shock ) cooldown -> duration -= 1.0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_spell_t::execute();
    p -> buffs_divine_favor -> expire();
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();
    paladin_spell_t::player_buff();
    if ( p -> buffs_divine_favor -> up() ) 
    {
      player_crit += 1.0;
    }
  }
};

// Holy Wrath ==============================================================

struct holy_wrath_t : public paladin_spell_t
{
  holy_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "holy_wrath", p )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 1058, 1242, 0, 0.20 }, // Dummy rank.
      { 78, 5, 1050, 1234, 0, 0.20 },
      { 72, 4,  857, 1007, 0, 0.20 },
      { 69, 3,  777,  913, 0, 0.20 },
      { 60, 2,  551,  649, 0, 0.24 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48817 );

    aoe = true;
    may_crit = true;

    base_cost *= 1.0 - p -> talents.benediction * 0.02;

    cooldown -> duration  = 30 - p -> glyphs.holy_wrath * 15;
    cooldown -> duration *= 1.0 - p -> talents.purifying_power * 0.33 / 2;

    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0.07;
    base_attack_power_multiplier = 0.07;

    if ( p -> librams.wracking ) base_spell_power += 120;
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
  if ( name == "divine_favor"            ) return new divine_favor_t           ( this, options_str );
  if ( name == "divine_illumination"     ) return new divine_illumination_t    ( this, options_str );
  if ( name == "divine_plea"             ) return new divine_plea_t            ( this, options_str );
  if ( name == "divine_storm"            ) return new divine_storm_t           ( this, options_str );
  if ( name == "exorcism"                ) return new exorcism_t               ( this, options_str );
  if ( name == "hammer_of_justice"       ) return new hammer_of_justice_t      ( this, options_str );
  if ( name == "hammer_of_wrath"         ) return new hammer_of_wrath_t        ( this, options_str );
  if ( name == "hammer_of_the_righteous" ) return new hammer_of_the_righteous_t( this, options_str );
  if ( name == "holy_shield"             ) return new holy_shield_t            ( this, options_str );
  if ( name == "holy_shock"              ) return new holy_shock_t             ( this, options_str );
  if ( name == "holy_wrath"              ) return new holy_wrath_t             ( this, options_str );
  if ( name == "judgement"               ) return new judgement_t              ( this, options_str );
  if ( name == "shield_of_righteousness" ) return new shield_of_righteousness_t( this, options_str );

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
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, level, PALADIN, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  initial_attack_power_per_strength = 2.0;
  initial_attack_power_per_agility  = 0.0;

  initial_spell_power_per_intellect = talents.holy_guidance * 0.04;

  attribute_multiplier_initial[ ATTR_STRENGTH  ] *= 1.0 + talents.divine_strength  * 0.03;
  attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + talents.combat_expertise * 0.02 + talents.sacred_duty * 0.04;
  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.divine_intellect * 0.02;

  base_attack_power = ( level * 3 ) - 20;
  base_attack_expertise += talents.combat_expertise * 0.02;
  base_attack_hit       += talents.enlightened_judgements * 0.02;
  base_attack_crit      += talents.conviction * 0.01;
  base_attack_crit      += talents.sanctity_of_battle * 0.01;
  base_attack_crit      += talents.combat_expertise * 0.02;

  base_spell_hit  += talents.enlightened_judgements * 0.02;
  base_spell_crit += talents.conviction * 0.01;
  base_spell_crit += talents.sanctity_of_battle * 0.01;
  base_spell_crit += talents.combat_expertise * 0.02;

  // FIXME! Level-specific!
  base_defense = level * 5;
  base_miss    = 0.05;
  base_dodge   = 0.0349430 + 0.01 * talents.anticipation;
  base_parry   = 0.05 + 0.01 * talents.deflection;
  base_block   = 0.05;
  initial_armor_multiplier *= 1.0 + 0.02 * talents.toughness;
  initial_dodge_per_agility = 0.0001670;
  initial_armor_per_agility = 2.0;

  diminished_kfactor    = 0.9560;
  diminished_miss_capi  = 1.0 / 0.16;
  diminished_dodge_capi = 1.0 / 0.88129021;
  diminished_parry_capi = 1.0 / 0.47003525;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  if ( tank == -1 && talents.holy_shield ) tank = 1;
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

  if ( main_hand_attack ) main_hand_attack -> cancel();
}

// paladin_t::init_gains ====================================================

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains_divine_plea            = get_gain( "divine_plea"            );
  gains_judgements_of_the_wise = get_gain( "judgements_of_the_wise" );
  gains_seal_of_command_glyph  = get_gain( "seal_of_command_glyph"  );
  gains_seal_of_wisdom         = get_gain( "seal_of_wisdom"         );
}

// paladin_t::init_procs ====================================================

void paladin_t::init_procs()
{
  player_t::init_procs();

  procs_parry_haste = get_proc( "parry_haste" );
  procs_tier10_2pc  = get_proc( "tier10_2pc"  ); 
}

// paladin_t::init_rng ======================================================

void paladin_t::init_rng()
{
  player_t::init_rng();

  rng_guarded_by_the_light   = get_rng( "guarded_by_the_light"   );
  rng_judgements_of_the_wise = get_rng( "judgements_of_the_wise" );
  rng_tier10_2pc             = get_rng( "tier10_2pc"             );
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
    else if ( n == "seal_of_command"         ) glyphs.seal_of_command = 1;
    else if ( n == "seal_of_righteousness"   ) glyphs.seal_of_righteousness = 1;
    else if ( n == "seal_of_vengeance"       ) glyphs.seal_of_vengeance = 1;
    else if ( n == "sense_undead"            ) glyphs.sense_undead = 1;
    else if ( n == "shield_of_righteousness" ) glyphs.shield_of_righteousness = 1;
    else if ( n == "the_wise"                ) glyphs.the_wise = 1;
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
    else if ( n == "righteous_defense"       ) ;
    else if ( n == "salvation"               ) ;
    else if ( n == "seal_of_light"           ) ;
    else if ( n == "seal_of_wisdom"          ) ;
    else if ( n == "spiritual_attunement"    ) ;
    else if ( n == "turn_evil"               ) ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
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
  else if ( libram == "relentless_gladiators_libram_of_fortitude" ) librams.relentless_gladiators_fortitude = 1;
  else if ( libram == "libram_of_avengement"                      ) librams.avengement = 1;
  else if ( libram == "libram_of_discord"                         ) librams.discord = 1;
  else if ( libram == "libram_of_divine_judgement"                ) librams.divine_judgement = 1;
  else if ( libram == "libram_of_divine_purpose"                  ) librams.divine_purpose = 1;
  else if ( libram == "libram_of_furious_blows"                   ) librams.furious_blows = 1;
  else if ( libram == "libram_of_radiance"                        ) librams.radiance = 1;
  else if ( libram == "libram_of_reciprocation"                   ) librams.reciprocation = 1;
  else if ( libram == "libram_of_resurgence"                      ) librams.resurgence = 1;
  else if ( libram == "libram_of_three_truths"                    ) librams.three_truths = 1;
  else if ( libram == "libram_of_valiance"                        ) librams.valiance = 1;
  else if ( libram == "libram_of_wracking"                        ) librams.wracking = 1;
  else if ( libram == "venture_co._libram_of_protection"          ) librams.venture_co_protection = 1;
  else if ( libram == "venture_co._libram_of_retribution"         ) librams.venture_co_retribution = 1;
  // To prevent warnings...
  else if ( libram == "blessed_book_of_nagrand" ) ;
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
  else if ( libram == "libram_of_the_eternal_tower" ) ;
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
  else if ( ! libram.empty() )
  {
    sim -> errorf( "Player %s has unknown libram %s", name(), libram.c_str() );
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
  if ( strstr( s, "lightsworn" ) )
  {
    if ( is_melee  ) return SET_T10_MELEE;
    if ( is_tank   ) return SET_T10_TANK;
  }

  return SET_NONE;
}

// paladin_t::init_buffs ====================================================

void paladin_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_avenging_wrath         = new buff_t( this, "avenging_wrath",         1, 20.0 );
  buffs_divine_favor           = new buff_t( this, "divine_favor",           1, 15.0 );
  buffs_divine_illumination    = new buff_t( this, "divine_illumination",    1, 15.0 );
  buffs_divine_plea            = new buff_t( this, "divine_plea",            1, 15.0 );
  buffs_holy_shield            = new buff_t( this, "holy_shield",            8, 10.0 );
  buffs_judgements_of_the_pure = new buff_t( this, "judgements_of_the_pure", 1, 60.0 );
  buffs_reckoning              = new buff_t( this, "reckoning",              4,  8.0, 0, talents.reckoning * 0.02 );
  buffs_redoubt                = new buff_t( this, "redoubt",                5, 10.0, 0, talents.redoubt ? 0.10 : 0.0 );
  buffs_holy_vengeance         = new buff_t( this, "holy_vengeance",         5       );
  buffs_the_art_of_war         = new buff_t( this, "the_art_of_war",         1, 15.0, 0, talents.the_art_of_war );
  buffs_vengeance              = new buff_t( this, "vengeance",              3, 30.0 );

  // stat_buff_t( sim, player, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_libram_of_avengement       = new stat_buff_t( this, "libram_of_avengement",        STAT_CRIT_RATING,   53, 1,  5.0            );
  buffs_libram_of_divine_judgement = new stat_buff_t( this, "libram_of_divine_judgement",  STAT_ATTACK_POWER, 200, 1, 10.0, 0.0, 0.40 );
  buffs_libram_of_fortitude        = new stat_buff_t( this, "libram_of_fortitude",         STAT_ATTACK_POWER, 172, 1, 10.0            );
  buffs_libram_of_furious_blows    = new stat_buff_t( this, "libram_of_furious_blows",     STAT_CRIT_RATING,   61, 1,  5.0            );
  buffs_libram_of_reciprocation    = new stat_buff_t( this, "libram_of_reciprocation",     STAT_CRIT_RATING,  173, 1, 10.0, 0.0, 0.15 );
  buffs_libram_of_three_truths     = new stat_buff_t( this, "libram_of_three_truths",      STAT_STRENGTH,      44, 5, 15.0            );
  buffs_libram_of_valiance         = new stat_buff_t( this, "libram_of_valiance",          STAT_STRENGTH,     200, 1, 16.0, 6.0, 0.70 );
  buffs_tier8_4pc_tank             = new stat_buff_t( this, "tier8_4pc_tank",              STAT_BLOCK_VALUE,  225, 1,  6.0            );
}

// paladin_t::init_actions ==================================================

void paladin_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  active_holy_shield = new holy_shield_discharge_t( this );

  active_seal_of_command_proc       = new seal_of_command_proc_t      ( this );
  active_seal_of_justice_proc       = new seal_of_justice_proc_t      ( this );
  active_seal_of_light_proc         = new seal_of_light_proc_t        ( this );
  active_seal_of_righteousness_proc = new seal_of_righteousness_proc_t( this );
  active_seal_of_vengeance_proc     = new seal_of_vengeance_proc_t    ( this );
  active_seal_of_vengeance_dot      = new seal_of_vengeance_dot_t     ( this );
  active_seal_of_wisdom_proc        = new seal_of_wisdom_proc_t       ( this );

  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=endless_rage/food,type=dragonfin_filet/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60/auto_attack";
    if      ( glyphs.seal_of_command       ) action_list_str += "/seal_of_command";
    else if ( glyphs.seal_of_righteousness ) action_list_str += "/seal_of_righteousness";
    else                                     action_list_str += "/seal_of_vengeance";
    action_list_str += "/snapshot_stats";
    action_list_str += "/hammer_of_justice";
    if ( talents.holy_shield && tank > 0 ) action_list_str += "/holy_shield";
    int num_items = ( int ) items.size();
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
    if ( talents.avengers_shield && 
          glyphs.avengers_shield ) action_list_str += "/avengers_shield";

    if ( talents.hammer_of_the_righteous ) action_list_str += "/hammer_of_the_righteous";
    action_list_str += "/judgement";
    if ( talents.crusader_strike ) action_list_str += "/crusader_strike";
    if ( talents.divine_storm    ) action_list_str += "/divine_storm";
    action_list_str += "/consecration,if=target.time_to_die>=6";
    action_list_str += "/hammer_of_wrath";
    action_list_str += "/exorcism,if=buff.the_art_of_war.react";
    if ( talents.holy_shock ) action_list_str += "/holy_shock";
    if ( main_hand_weapon.group() == WEAPON_1H ) action_list_str += "/shield_of_righteousness";
    action_list_str += "/divine_plea";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// paladin_t::init_rating ==================================================

void paladin_t::init_rating()
{
  player_t::init_rating();

  rating.attack_haste *= 1.0 / 1.30;
}

// paladin_t::composite_spell_power ==========================================

double paladin_t::composite_spell_power( int school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );
  if ( talents.sheath_of_light )
  {
    sp += composite_attack_power_multiplier() * composite_attack_power() * talents.sheath_of_light * 0.1;
  }
  if ( talents.touched_by_the_light )
  {
    sp += stamina() * 0.10 * talents.touched_by_the_light;
  }
  return sp;
}

// paladin_t::composite_block_value ==========================================

double paladin_t::composite_block_value() SC_CONST
{
  double bv = player_t::composite_block_value();
  if ( talents.redoubt )
  {
    bv *= 1.0 + 0.01 * talents.redoubt;
  }
  return bv;
}

// paladin_t::composite_tank_block ===========================================

double paladin_t::composite_tank_block() SC_CONST
{
  double b = player_t::composite_tank_block();
  if ( buffs_holy_shield -> up() ) b += 0.30;
  if ( buffs_redoubt     -> up() ) b += 0.10 * talents.redoubt;
  return b;
}

// paladin_t::primary_tree ==================================================

int paladin_t::primary_tree() SC_CONST
{
  if ( talents.holy_shock  ) return TREE_HOLY;
  if ( talents.holy_shield ) return TREE_PROTECTION;
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

// paladin_t::target_swing ==================================================

int paladin_t::target_swing()
{
  int result = player_t::target_swing();

  if ( sim -> log ) log_t::output( sim, "%s swing result: %s", sim -> target -> name(), util_t::result_type_string( result ) );

  if ( result == RESULT_BLOCK )
  {
    if ( buffs_holy_shield -> check() )
    {
      active_holy_shield -> execute();
      buffs_holy_shield -> decrement();
    }
    buffs_redoubt -> decrement();
  }
  if ( result == RESULT_HIT    ||
       result == RESULT_CRIT   ||
       result == RESULT_GLANCE ||
       result == RESULT_BLOCK  )
  {
    buffs_reckoning -> trigger();
    buffs_redoubt   -> trigger( 5 );
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

// paladin_t::get_talents_trees ==============================================

std::vector<talent_translation_t>& paladin_t::get_talent_list()
{
  if(talent_list.empty())
  {
	  talent_translation_t translation_table[][MAX_TALENT_TREES] =
	  {
		  { {  1, 0, NULL                                     }, {  1, 0, NULL                                      }, {  1, 5, &( talents.deflection                 ) } },
    { {  2, 5, &( talents.seals_of_the_pure           ) }, {  2, 5, &( talents.divine_strength              ) }, {  2, 5, &( talents.benediction                ) } },
    { {  3, 3, &( talents.healing_light               ) }, {  3, 0, NULL                                      }, {  3, 2, &( talents.improved_judgements        ) } },
    { {  4, 5, &( talents.divine_intellect            ) }, {  4, 0, NULL                                      }, {  4, 3, &( talents.heart_of_the_crusader      ) } },
    { {  5, 0, NULL                                     }, {  5, 5, &( talents.anticipation                 ) }, {  5, 2, &( talents.improved_blessing_of_might ) } },
    { {  6, 1, &( talents.aura_mastery                ) }, {  6, 0, NULL                                      }, {  6, 2, &( talents.vindication                ) } },
    { {  7, 0, NULL                                     }, {  7, 3, &( talents.improved_righteous_fury      ) }, {  7, 5, &( talents.conviction                 ) } },
    { {  8, 0, NULL                                     }, {  8, 5, &( talents.toughness                    ) }, {  8, 1, &( talents.seal_of_command            ) } },
    { {  9, 3, &( talents.improved_concentration_aura ) }, {  9, 0, NULL                                      }, {  9, 0, NULL                                    } },
    { { 10, 2, &( talents.improved_blessing_of_wisdom ) }, { 10, 2, &( talents.improved_hammer_of_justice   ) }, { 10, 2, &( talents.eye_for_an_eye             ) } },
    { { 11, 0, NULL                                     }, { 11, 3, &( talents.improved_devotion_aura       ) }, { 11, 3, &( talents.sanctity_of_battle         ) } },
    { { 12, 0, NULL                                     }, { 12, 1, &( talents.blessing_of_sanctuary        ) }, { 12, 3, &( talents.crusade                    ) } },
    { { 13, 1, &( talents.divine_favor                ) }, { 13, 5, &( talents.reckoning                    ) }, { 13, 3, &( talents.two_handed_weapon_spec     ) } },
    { { 14, 3, &( talents.sanctified_light            ) }, { 14, 2, &( talents.sacred_duty                  ) }, { 14, 1, &( talents.sanctified_retribution     ) } },
    { { 15, 2, &( talents.purifying_power             ) }, { 15, 3, &( talents.one_handed_weapon_spec       ) }, { 15, 3, &( talents.vengeance                  ) } },
    { { 16, 5, &( talents.holy_power                  ) }, { 16, 2, &( talents.spiritual_attunement         ) }, { 16, 2, &( talents.divine_purpose             ) } },
    { { 17, 0, NULL                                     }, { 17, 1, &( talents.holy_shield                  ) }, { 17, 2, &( talents.the_art_of_war             ) } },
    { { 18, 1, &( talents.holy_shock                  ) }, { 18, 3, &( talents.ardent_defender              ) }, { 18, 0, NULL                                    } },
    { { 19, 3, &( talents.blessed_life                ) }, { 19, 3, &( talents.redoubt                      ) }, { 19, 3, &( talents.judgements_of_the_wise     ) } },
    { { 20, 0, NULL                                     }, { 20, 3, &( talents.combat_expertise             ) }, { 20, 3, &( talents.fanaticism                 ) } },
    { { 21, 5, &( talents.holy_guidance               ) }, { 21, 3, &( talents.touched_by_the_light         ) }, { 21, 2, &( talents.sanctified_wrath           ) } },
    { { 22, 1, &( talents.divine_illumination         ) }, { 22, 1, &( talents.avengers_shield              ) }, { 22, 3, &( talents.swift_retribution          ) } },
    { { 23, 5, &( talents.judgements_of_the_pure      ) }, { 23, 2, &( talents.guarded_by_the_light         ) }, { 23, 1, &( talents.crusader_strike            ) } },
    { { 24, 0, NULL                                     }, { 24, 3, &( talents.shield_of_the_templar        ) }, { 24, 3, &( talents.sheath_of_light            ) } },
    { { 25, 2, &( talents.enlightened_judgements      ) }, { 25, 2, &( talents.judgements_of_the_just       ) }, { 25, 3, &( talents.righteous_vengeance        ) } },
    { { 26, 0, NULL                                     }, { 26, 1, &( talents.hammer_of_the_righteous      ) }, { 26, 1, &( talents.divine_storm               ) } },
    { {  0, 0, NULL                                     }, {  0, 0, NULL                                      }, {  0, 0, NULL                                    } }
  };

    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
  }
  return talent_list;}

// paladin_t::get_options ====================================================

std::vector<option_t>& paladin_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t paladin_options[] =
    {
      // @option_doc loc=player/paladin/talents title="Talents"
      { "anticipation",                OPT_INT, &( talents.anticipation                ) },
      { "ardent_defender",             OPT_INT, &( talents.ardent_defender             ) },
      { "aura_mastery",                OPT_INT, &( talents.aura_mastery                ) },
      { "avengers_shield",             OPT_INT, &( talents.avengers_shield             ) },
      { "benediction;",                OPT_INT, &( talents.benediction                 ) },
      { "blessed_life",                OPT_INT, &( talents.blessed_life                ) },
      { "blessing_of_sanctuary",       OPT_INT, &( talents.blessing_of_sanctuary       ) },
      { "combat_expertise",            OPT_INT, &( talents.combat_expertise            ) },
      { "conviction",                  OPT_INT, &( talents.conviction                  ) },
      { "crusade",                     OPT_INT, &( talents.crusade                     ) },
      { "crusader_strike",             OPT_INT, &( talents.crusader_strike             ) },
      { "deflection",                  OPT_INT, &( talents.deflection                  ) },
      { "divine_favor",                OPT_INT, &( talents.divine_favor                ) },
      { "divine_illumination",         OPT_INT, &( talents.divine_illumination         ) },
      { "divine_intellect",            OPT_INT, &( talents.divine_intellect            ) },
      { "divine_purpose",              OPT_INT, &( talents.divine_purpose              ) },
      { "divine_storm",                OPT_INT, &( talents.divine_storm                ) },
      { "divine_strength",             OPT_INT, &( talents.divine_strength             ) },
      { "enlightened_judgements",      OPT_INT, &( talents.enlightened_judgements      ) },
      { "eye_for_an_eye",              OPT_INT, &( talents.eye_for_an_eye              ) },
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
      { "improved_devotion_aura",      OPT_INT, &( talents.improved_devotion_aura      ) },
      { "improved_hammer_of_justice",  OPT_INT, &( talents.improved_hammer_of_justice  ) },
      { "improved_judgements",         OPT_INT, &( talents.improved_judgements         ) },
      { "improved_righteous_fury",     OPT_INT, &( talents.improved_righteous_fury     ) },
      { "judgements_of_the_just",      OPT_INT, &( talents.judgements_of_the_just      ) },
      { "judgements_of_the_pure",      OPT_INT, &( talents.judgements_of_the_pure      ) },
      { "judgements_of_the_wise",      OPT_INT, &( talents.judgements_of_the_wise      ) },
      { "one_handed_weapon_spec",      OPT_INT, &( talents.one_handed_weapon_spec      ) },
      { "purifying_power",             OPT_INT, &( talents.purifying_power             ) },
      { "reckoning",                   OPT_INT, &( talents.reckoning                   ) },
      { "redoubt",                     OPT_INT, &( talents.redoubt                     ) },
      { "righteous_vengeance",         OPT_INT, &( talents.righteous_vengeance         ) },
      { "sacred_duty",                 OPT_INT, &( talents.sacred_duty                 ) },
      { "sanctified_light",            OPT_INT, &( talents.sanctified_light            ) },
      { "sanctified_retribution",      OPT_INT, &( talents.sanctified_retribution      ) },
      { "sanctified_wrath",            OPT_INT, &( talents.sanctified_wrath            ) },
      { "sanctity_of_battle",          OPT_INT, &( talents.sanctity_of_battle          ) },
      { "seal_of_command",             OPT_INT, &( talents.seal_of_command             ) },
      { "seals_of_the_pure",           OPT_INT, &( talents.seals_of_the_pure           ) },
      { "sheath_of_light",             OPT_INT, &( talents.sheath_of_light             ) },
      { "shield_of_the_templar",       OPT_INT, &( talents.shield_of_the_templar       ) },
      { "spiritual_attunement",        OPT_INT, &( talents.spiritual_attunement        ) },
      { "swift_retribution",           OPT_INT, &( talents.swift_retribution           ) },
      { "the_art_of_war",              OPT_INT, &( talents.the_art_of_war              ) },
      { "toughness",                   OPT_INT, &( talents.toughness                   ) },
      { "touched_by_the_light",        OPT_INT, &( talents.touched_by_the_light        ) },
      { "two_handed_weapon_spec",      OPT_INT, &( talents.two_handed_weapon_spec      ) },
      { "vengeance",                   OPT_INT, &( talents.vengeance                   ) },
      { "vindication",                 OPT_INT, &( talents.vindication                 ) },
      // @option_doc loc=player/paladin/misc title="Misc"
      { "tier10_2pc_procs_from_strikes", OPT_BOOL, &( tier10_2pc_procs_from_strikes    ) },
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
  sim -> auras.devotion_aura          = new aura_t( sim, "devotion_aura",          1 );
  sim -> auras.sanctified_retribution = new buff_t( sim, "sanctified_retribution", 1 );
  sim -> auras.swift_retribution      = new buff_t( sim, "swift_retribution",      1 );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.blessing_of_kings  = new buff_t( p, "blessing_of_kings",  1 );
    p -> buffs.blessing_of_might  = new buff_t( p, "blessing_of_might",  1 );
    p -> buffs.blessing_of_wisdom = new buff_t( p, "blessing_of_wisdom", 1 );
  }

  target_t* t = sim -> target;

  t -> debuffs.heart_of_the_crusader  = new debuff_t( sim, "heart_of_the_crusader",  1, 20.0 );
  t -> debuffs.judgement_of_wisdom    = new debuff_t( sim, "judgement_of_wisdom",    1, 20.0 );
  t -> debuffs.judgements_of_the_just = new debuff_t( sim, "judgements_of_the_just", 1, 20.0 );
}

// player_t::paladin_combat_begin ============================================

void player_t::paladin_combat_begin( sim_t* sim )
{
  if( sim -> overrides.devotion_aura          ) sim -> auras.devotion_aura          -> override( 1, 1205 * 1.5 );
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
  if ( sim -> overrides.heart_of_the_crusader  ) t -> debuffs.heart_of_the_crusader  -> override();
  if ( sim -> overrides.judgement_of_wisdom    ) t -> debuffs.judgement_of_wisdom    -> override();
  if ( sim -> overrides.judgements_of_the_just ) t -> debuffs.judgements_of_the_just -> override();
}
