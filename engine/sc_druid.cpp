// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Druid
// ==========================================================================

struct druid_t : public player_t
{
  // Active
  action_t* active_t10_4pc_caster_dot;
  action_t* active_wrath_dot;

  std::vector<action_t*> active_mauls;
  int num_active_mauls;

  // Buffs
  buff_t* buffs_berserk;
  buff_t* buffs_bear_form;
  buff_t* buffs_cat_form;
  buff_t* buffs_combo_points;
  buff_t* buffs_corruptor;
  buff_t* buffs_eclipse_lunar;
  buff_t* buffs_eclipse_solar;
  buff_t* buffs_enrage;
  buff_t* buffs_glyph_of_innervate;
  buff_t* buffs_lacerate;
  buff_t* buffs_lunar_fury;
  buff_t* buffs_moonkin_form;
  buff_t* buffs_mutilation;
  buff_t* buffs_natures_grace;
  buff_t* buffs_natures_swiftness;
  buff_t* buffs_omen_of_clarity;
  buff_t* buffs_savage_roar;
  buff_t* buffs_stealthed;
  buff_t* buffs_t8_4pc_caster;
  buff_t* buffs_t10_2pc_caster;
  buff_t* buffs_terror;
  buff_t* buffs_tigers_fury;
  buff_t* buffs_unseen_moon;
  buff_t* buffs_t10_feral_relic;
  buff_t* buffs_t10_balance_relic;

  // Cooldowns
  cooldown_t* cooldowns_mangle_bear;

  // DoTs
  dot_t* dots_rip;
  dot_t* dots_rake;
  dot_t* dots_insect_swarm;
  dot_t* dots_moonfire;

  // Gains
  gain_t* gains_bear_melee;
  gain_t* gains_energy_refund;
  gain_t* gains_enrage;
  gain_t* gains_glyph_of_innervate;
  gain_t* gains_incoming_damage;
  gain_t* gains_moonkin_form;
  gain_t* gains_natural_reaction;
  gain_t* gains_omen_of_clarity;
  gain_t* gains_primal_fury;
  gain_t* gains_primal_precision;
  gain_t* gains_tigers_fury;

  // Procs
  proc_t* procs_combo_points_wasted;
  proc_t* procs_parry_haste;
  proc_t* procs_primal_fury;
  proc_t* procs_tier8_2pc;

  // Up-Times
  uptime_t* uptimes_energy_cap;
  uptime_t* uptimes_rage_cap;
  uptime_t* uptimes_rip;
  uptime_t* uptimes_rake;

  // Random Number Generation
  rng_t* rng_primal_fury;

  attack_t* cat_melee_attack;
  attack_t* bear_melee_attack;

  double equipped_weapon_dps;

  struct talents_t
  {
    int  balance_of_power;
    int  berserk;
    int  brambles;
    int  brutal_impact;
    int  celestial_focus;
    int  dreamstate;
    int  earth_and_moon;
    int  eclipse;
    int  feral_aggression;
    int  feral_instinct;
    int  feral_swiftness;
    int  ferocity;
    int  force_of_nature;
    int  furor;
    int  genesis;
    int  heart_of_the_wild;
    int  improved_faerie_fire;
    int  improved_insect_swarm;
    int  improved_mangle;
    int  improved_mark_of_the_wild;
    int  improved_moonfire;
    int  improved_moonkin_form;
    int  insect_swarm;
    int  intensity;
    int  king_of_the_jungle;
    int  leader_of_the_pack;
    int  living_spirit;
    int  lunar_guidance;
    int  mangle;
    int  master_shapeshifter;
    int  moonfury;
    int  moonglow;
    int  moonkin_form;
    int  natural_perfection;
    int  natural_reaction;
    int  naturalist;
    int  natures_grace;
    int  natures_majesty;
    int  natures_reach;
    int  natures_splendor;
    int  natures_swiftness;
    int  omen_of_clarity;
    int  predatory_instincts;
    int  predatory_strikes;
    int  primal_fury;
    int  primal_gore;
    int  primal_precision;
    int  protector_of_the_pack;
    int  rend_and_tear;
    int  savage_fury;
    int  sharpened_claws;
    int  shredding_attacks;
    int  starfall;
    int  starlight_wrath;
    int  survival_instincts;
    int  survival_of_the_fittest;
    int  thick_hide;
    int  typhoon;
    int  vengeance;
    int  wrath_of_cenarius;

    // Cataclysm
    int  fungal_growth;
    int  lunar_justice;
    int  solar_beam;
    int  starsurge;
    int  blessing_of_the_grove;


    // NYI
    int gale_winds;
    int infected_wounds;
    int owlkin_frenzy;

    bool is_tank_spec(void)
    {
        bool bear_essentials = thick_hide==3 && survival_instincts==1 && natural_reaction==3 && protector_of_the_pack==3;
        bool kitty_playthings = predatory_instincts > 0;

        return bear_essentials && !kitty_playthings;
    }
    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int berserk;
    int focus;
    int innervate;
    int insect_swarm;
    int mangle;
    int moonfire;
    int monsoon;
    int rip;
    int savage_roar;
    int shred;
    int starfire;
    int starfall;
    int typhoon;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct idols_t
  {
    int corruptor;
    int crying_moon;
    int crying_wind;
    int lunar_eclipse;
    int lunar_fury;
    int mutilation;
    int raven_goddess;
    int ravenous_beast;
    int shooting_star;
    int steadfast_renewal;
    int terror;
    int unseen_moon;
    int worship;
    idols_t() { memset( ( void* ) this, 0x0, sizeof( idols_t ) ); }
  };
  idols_t idols;

  druid_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, DRUID, name, race_type )
  {
    active_t10_4pc_caster_dot = 0;
    active_wrath_dot    = 0;

    cooldowns_mangle_bear = get_cooldown( "mangle_bear" );

    distance = 30;

    dots_rip          = get_dot( "rip" );
    dots_rake         = get_dot( "rake" );
    dots_insect_swarm = get_dot( "insect_swarm" );
    dots_moonfire     = get_dot( "moonfire" );

    cat_melee_attack = 0;
    bear_melee_attack = 0;

    equipped_weapon_dps = 0;
  }

  // Character Definition
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_buffs();
  virtual void      init_items();
  virtual void      init_scaling();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual void      init_rating();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      reset();
  virtual void      interrupt();
  virtual void      clear_debuffs();
  virtual void      regen( double periodicity );
  virtual double    available() SC_CONST;
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attack_power_multiplier() SC_CONST;
  virtual double    composite_attack_crit() SC_CONST;
  virtual double    composite_spell_hit() SC_CONST;
  virtual double    composite_spell_crit() SC_CONST;
  virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
  virtual double    composite_block_value() SC_CONST { return 0; }
  virtual double    composite_tank_parry() SC_CONST { return 0; }
  virtual double    composite_tank_block() SC_CONST { return 0; }
  virtual double    composite_tank_crit( int school ) SC_CONST;
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST;
  virtual int       primary_role() SC_CONST;
  virtual int       primary_tree() SC_CONST;
  virtual int       target_swing();

  // Utilities
  double combo_point_rank( double* cp_list ) SC_CONST
  {
    assert( buffs_combo_points -> check() );
    return cp_list[ buffs_combo_points -> stack() - 1 ];
  }
  double combo_point_rank( double cp1, double cp2, double cp3, double cp4, double cp5 ) SC_CONST
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return combo_point_rank( cp_list );
  }
  void reset_gcd()
  {
    for ( action_t* a=action_list; a; a = a -> next )
    {
      if ( a -> trigger_gcd != 0 ) a -> trigger_gcd = base_gcd;
    }
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct druid_cat_attack_t : public attack_t
{
  int requires_stealth;
  int requires_position;
  bool requires_combo_points;
  bool adds_combo_points;
  int berserk, min_combo_points, max_combo_points;
  double min_energy, max_energy;
  double min_mangle_expire, max_mangle_expire;
  double min_savage_roar_expire, max_savage_roar_expire;
  double min_rip_expire, max_rip_expire;
  double min_rake_expire, max_rake_expire;

  druid_cat_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_ENERGY, s, t, special ),
      requires_stealth( 0 ),
      requires_position( POSITION_NONE ),
      requires_combo_points( false ),
      adds_combo_points( false ),
      berserk( 0 ),
      min_combo_points( 0 ), max_combo_points( 0 ),
      min_energy( 0 ), max_energy( 0 ),
      min_mangle_expire( 0 ), max_mangle_expire( 0 ),
      min_savage_roar_expire( 0 ), max_savage_roar_expire( 0 ),
      min_rip_expire( 0 ), max_rip_expire( 0 ),
      min_rake_expire( 0 ), max_rake_expire( 0 )
  {
    druid_t* p = player -> cast_druid();

    base_crit_multiplier *= 1.0 + util_t::talent_rank( p -> talents.predatory_instincts, 3, 0.03, 0.07, 0.10 );
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost() SC_CONST;
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual bool   ready();
};

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct druid_bear_attack_t : public attack_t
{
  int berserk;
  double min_rage, max_rage;
  double min_mangle_expire, max_mangle_expire;
  double min_lacerate_expire, max_lacerate_expire;
  int    min_lacerate_stack, max_lacerate_stack;

  druid_bear_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_RAGE, s, t, true ),
      berserk( 0 ),
      min_rage( 0 ), max_rage( 0 ),
      min_mangle_expire( 0 ), max_mangle_expire( 0 ),
      min_lacerate_expire( 0 ), max_lacerate_expire( 0 ),
      min_lacerate_stack( 0 ), max_lacerate_stack( 0 )
  {
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost() SC_CONST;
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual bool   ready();
};

// ==========================================================================
// Druid Spell
// ==========================================================================

struct druid_spell_t : public spell_t
{
  int skip_on_eclipse;
  druid_spell_t( const char* n, player_t* p, int s, int t ) :
      spell_t( n, p, RESOURCE_MANA, s, t ), skip_on_eclipse( 0 ) {}
  virtual void   consume_resource();
  virtual double cost() SC_CONST;
  virtual void   execute();
  virtual double execute_time() SC_CONST;
  virtual double haste() SC_CONST;
  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual void   player_buff();
  virtual bool   ready();
  virtual void   schedule_execute();
  virtual void   target_debuff( int dmg_type );
};

// ==========================================================================
// Pet Treants
// ==========================================================================

struct treants_pet_t : public pet_t
{
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
        attack_t( "treant_melee", player )
    {
      druid_t* o = player -> cast_pet() -> owner -> cast_druid();

      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      background = true;
      repeating = true;
      may_crit = true;

      base_multiplier *= 1.0 + o -> talents.brambles * 0.05;

      // Model the three Treants as one actor hitting 3x hard
      base_multiplier *= 3.0;
    }
  };

  melee_t* melee;

  treants_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
      pet_t( sim, owner, pet_name ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 340;
    main_hand_weapon.max_dmg    = 340;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 1.8;
  }
  virtual void init_base()
  {
    pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ] = 331;
    attribute_base[ ATTR_AGILITY   ] = 113;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    melee = new melee_t( this );
  }
  virtual double composite_attack_power() SC_CONST
  {
    double ap = pet_t::composite_attack_power();
    ap += 0.57 * owner -> composite_spell_power( SCHOOL_MAX );
    return ap;
  }
  virtual double composite_attack_hit() SC_CONST
  {
    return owner -> composite_spell_hit();
  }
  virtual double composite_attack_expertise() SC_CONST
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }
  virtual void schedule_ready( double delta_time=0,
                               bool   waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! melee -> execute_event ) melee -> execute();
  }
  virtual void interrupt()
  {
    pet_t::interrupt();
    melee -> cancel();
  }
};

// add_combo_point =========================================================

static void add_combo_point( druid_t* p )
{
  if ( p -> buffs_combo_points -> current_stack == 5 )
  {
    p -> procs_combo_points_wasted -> occur();
    return;
  }

  p -> buffs_combo_points -> trigger();
}

// may_trigger_lunar_eclipse ================================================

static bool may_trigger_lunar_eclipse( druid_t* p )
{
  if ( p -> talents.eclipse == 0 )
    return false;

  // Did the player have enough time by now to realise he procced eclipse?
  // If so, return false as we only want to cast to procc
  if (  p -> buffs_eclipse_lunar -> may_react() )
    return false;

  // Not yet possible to trigger
  if (  ! p -> buffs_eclipse_lunar -> check() )
    if ( p -> sim -> current_time + 1.5 < p -> buffs_eclipse_lunar -> cooldown_ready )
      return false;

  return true;
}

// may_trigger_solar_eclipse ================================================

static bool may_trigger_solar_eclipse( druid_t* p )
{
  if ( p -> talents.eclipse == 0 )
    return false;

  // Did the player have enough time by now to realise he procced eclipse?
  // If so, return false as we only want to cast to procc
  if ( p -> buffs_eclipse_solar -> may_react() )
    return false;

  // Not yet possible to trigger
  if ( ! p -> buffs_eclipse_solar -> check() )
    if ( p -> sim -> current_time + 3.0 < p -> buffs_eclipse_solar -> cooldown_ready )
      return false;

  return true;
}

// trigger_omen_of_clarity ==================================================

static void trigger_omen_of_clarity( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( a -> aoe ) return;
  if ( a -> proc ) return;

  if ( p -> buffs_omen_of_clarity -> trigger() )
    p -> buffs_t10_2pc_caster -> trigger( 1, 0.15 );

}

// trigger_infected_wounds ==================================================

static void trigger_infected_wounds( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( p -> talents.infected_wounds )
  {
    p -> sim -> target -> debuffs.infected_wounds -> trigger();
  }
}

// trigger_earth_and_moon ===================================================

static void trigger_earth_and_moon( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if ( p -> talents.earth_and_moon == 0 ) return;

  target_t* t = s -> sim -> target;

  double value = util_t::talent_rank( p -> talents.earth_and_moon, 3, 4, 9, 13 );

  if( value >= t -> debuffs.earth_and_moon -> current_value )
  {
    t -> debuffs.earth_and_moon -> trigger( 1, value );
  }
}

// trigger_t8_2pc_melee ====================================================

static void trigger_t8_2pc_melee( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> set_bonus.tier8_2pc_melee() )
    return;

  /* 2% chance on tick, http://ptr.wowhead.com/?spell=64752
  /  It is just another chance to procc the same clearcasting
  /  buff that OoC provides. OoC and t8_2pc_melee overwrite
  /  each other. Easier to treat it just (ab)use the OoC handling */

  if ( p -> buffs_omen_of_clarity -> trigger( 1, 1, 0.02 ) )
    p -> procs_tier8_2pc -> occur();
}

// trigger_t10_4pc_caster ==================================================

static void trigger_t10_4pc_caster( player_t* player, double direct_dmg, int school )
{
  druid_t* p = player -> cast_druid();

  if ( ! p -> set_bonus.tier10_4pc_caster() ) return;

  struct t10_4pc_caster_dot_t : public druid_spell_t
  {
    t10_4pc_caster_dot_t( player_t* player ) : druid_spell_t( "tier10_4pc_balance", player, SCHOOL_NATURE, TREE_BALANCE )
    {
      may_miss        = false;
      may_resist      = false;
      background      = true;
      proc            = true;
      trigger_gcd     = 0;
      base_cost       = 0;
      base_multiplier = 1.0;
      tick_power_mod  = 0;
      base_tick_time  = 2.0;
      num_ticks       = 2;
    }
    void player_buff() {}
    void target_debuff( int dmg_type ) {}
  };

  if ( ! p -> active_t10_4pc_caster_dot ) p -> active_t10_4pc_caster_dot = new t10_4pc_caster_dot_t( p );

  double dmg = direct_dmg * 0.07;
  if (  p -> active_t10_4pc_caster_dot -> ticking )
  {
    int num_ticks =  p -> active_t10_4pc_caster_dot -> num_ticks;
    int remaining_ticks = num_ticks -  p -> active_t10_4pc_caster_dot -> current_tick;

    dmg +=  p -> active_t10_4pc_caster_dot -> base_td * remaining_ticks;

    p -> active_t10_4pc_caster_dot -> cancel();
  }
   p -> active_t10_4pc_caster_dot -> base_td = dmg /  p -> active_t10_4pc_caster_dot -> num_ticks;
   p -> active_t10_4pc_caster_dot -> schedule_tick();
}

// trigger_primal_fury =====================================================

static void trigger_primal_fury( druid_cat_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.primal_fury )
    return;

  if ( ! a -> adds_combo_points )
    return;

  if ( p -> rng_primal_fury -> roll( p -> talents.primal_fury * 0.5 ) )
  {
    add_combo_point( p );
    p -> procs_primal_fury -> occur();
  }
}

// trigger_energy_refund ===================================================

static void trigger_energy_refund( druid_cat_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! a -> adds_combo_points )
    return;

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains_energy_refund );
}

// trigger_primal_precision ================================================

static void trigger_primal_precision( druid_cat_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.primal_precision )
    return;

  if ( ! a -> requires_combo_points )
    return;

  double energy_restored = a -> resource_consumed * p -> talents.primal_precision * 0.40;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains_primal_precision );
}

// trigger_primal_fury =====================================================

static void trigger_primal_fury( druid_bear_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.primal_fury )
    return;

  if ( p -> rng_primal_fury -> roll( p -> talents.primal_fury * 0.5 ) )
  {
    p -> resource_gain( RESOURCE_RAGE, 5.0, p -> gains_primal_fury );
  }
}

// trigger_rage_gain =======================================================

static void trigger_rage_gain( druid_bear_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  double rage_gain = 0;

  p -> resource_gain( RESOURCE_RAGE, rage_gain, p -> gains_bear_melee );
}

// =========================================================================
// Druid Attack
// =========================================================================

// druid_cat_attack_t::parse_options =======================================

void druid_cat_attack_t::parse_options( option_t*          options,
                                        const std::string& options_str )
{
  option_t base_options[] =
  {
    { "berserk",          OPT_INT,  &berserk                },
    { "min_combo_points", OPT_INT,  &min_combo_points       },
    { "max_combo_points", OPT_INT,  &max_combo_points       },
    { "cp>",              OPT_INT,  &min_combo_points       },
    { "cp<",              OPT_INT,  &max_combo_points       },
    { "min_energy",       OPT_FLT,  &min_energy             },
    { "max_energy",       OPT_FLT,  &max_energy             },
    { "energy>",          OPT_FLT,  &min_energy             },
    { "energy<",          OPT_FLT,  &max_energy             },
    { "rip>",             OPT_FLT,  &min_rip_expire         },
    { "rip<",             OPT_FLT,  &max_rip_expire         },
    { "rake>",            OPT_FLT,  &min_rake_expire        },
    { "rake<",            OPT_FLT,  &max_rake_expire        },
    { "mangle>",          OPT_FLT,  &min_mangle_expire      },
    { "mangle<",          OPT_FLT,  &max_mangle_expire      },
    { "savage_roar>",     OPT_FLT,  &min_savage_roar_expire },
    { "savage_roar<",     OPT_FLT,  &max_savage_roar_expire },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// druid_cat_attack_t::cost ================================================

double druid_cat_attack_t::cost() SC_CONST
{
  druid_t* p = player -> cast_druid();
  double c = attack_t::cost();
  if ( c == 0 ) return 0;
  if ( harmful &&  p -> buffs_omen_of_clarity -> check() ) return 0;
  if ( p -> buffs_berserk -> check() ) c *= 0.5;
  return c;
}

// druid_cat_attack_t::consume_resource ====================================

void druid_cat_attack_t::consume_resource()
{
  druid_t* p = player -> cast_druid();
  attack_t::consume_resource();
  if ( harmful && p -> buffs_omen_of_clarity -> up() )
  {
    // Treat the savings like a energy gain.
    double amount = attack_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> buffs_omen_of_clarity -> expire();
    }
  }
}

// druid_cat_attack_t::execute =============================================

void druid_cat_attack_t::execute()
{
  druid_t* p = player -> cast_druid();

  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( requires_combo_points ) p -> buffs_combo_points -> expire();
    if (     adds_combo_points ) add_combo_point ( p );

    if ( result == RESULT_CRIT )
    {
      trigger_primal_fury( this );
    }
  }
  else
  {
    trigger_energy_refund( this );
    trigger_primal_precision( this );
  }

  if( harmful ) p -> buffs_stealthed -> expire();
}

// druid_cat_attack_t::player_buff =========================================

void druid_cat_attack_t::player_buff()
{
  druid_t* p = player -> cast_druid();

  attack_t::player_buff();

  player_dd_adder += p -> buffs_tigers_fury -> value();

  player_multiplier *= 1 + p -> buffs_savage_roar -> value();

  if ( p -> talents.naturalist )
  {
    player_multiplier *= 1 + p -> talents.naturalist * 0.02;
  }

  p -> uptimes_rip  -> update( p -> dots_rip  -> ticking() );
  p -> uptimes_rake -> update( p -> dots_rake -> ticking() );
}

// druid_cat_attack_t::ready ===============================================

bool druid_cat_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();
  target_t* t = sim -> target;

  if ( ! p -> buffs_cat_form -> check() )
    return false;

  if ( requires_position != POSITION_NONE )
    if ( p -> position != requires_position )
      return false;

  if ( requires_stealth )
    if ( ! p -> buffs_stealthed -> check() )
      return false;

  if ( requires_combo_points && ! p -> buffs_combo_points -> check() )
    return false;

  if ( berserk && ! p -> buffs_berserk -> check() )
    return false;

  if ( min_combo_points > 0 )
    if ( p -> buffs_combo_points -> current_stack < min_combo_points )
      return false;

  if ( max_combo_points > 0 )
    if ( p -> buffs_combo_points -> current_stack > max_combo_points )
      return false;

  if ( min_energy > 0 )
    if ( p -> resource_current[ RESOURCE_ENERGY ] < min_energy )
      return false;

  if ( max_energy > 0 )
    if ( p -> resource_current[ RESOURCE_ENERGY ] > max_energy )
      return false;

  double ct = sim -> current_time;

  if ( min_mangle_expire > 0 )
    if ( t -> debuffs.mangle -> remains_lt( min_mangle_expire ) )
      return false;

  if ( max_mangle_expire > 0 )
    if ( t -> debuffs.mangle -> remains_gt( max_mangle_expire ) )
      return false;

  if ( min_savage_roar_expire > 0 )
    if ( p -> buffs_savage_roar -> remains_lt( min_savage_roar_expire ) )
      return false;

  if ( max_savage_roar_expire > 0 )
    if ( p -> buffs_savage_roar -> remains_gt( max_savage_roar_expire ) )
      return false;

  if ( min_rip_expire > 0 )
    if ( ! p -> dots_rip -> ticking() || ( ( p -> dots_rip -> ready - ct ) < min_rip_expire ) )
      return false;

  if ( max_rip_expire > 0 )
    if ( p -> dots_rip -> ticking() && ( ( p -> dots_rip -> ready - ct ) > max_rip_expire ) )
      return false;

  if ( min_rake_expire > 0 )
    if ( ! p -> dots_rake -> ticking() || ( ( p -> dots_rake -> ready - ct ) < min_rake_expire ) )
      return false;

  if ( max_rake_expire > 0 )
    if ( p -> dots_rake -> ticking() && ( ( p -> dots_rake -> ready - ct ) > max_rake_expire ) )
      return false;

  return true;
}

// Cat Melee Attack ========================================================

struct cat_melee_t : public druid_cat_attack_t
{
  cat_melee_t( player_t* player ) :
    druid_cat_attack_t( "cat_melee", player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false )
  {
    base_dd_min = base_dd_max = 1;
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
    may_crit    = true;
  }

  virtual double execute_time() SC_CONST
  {
    if ( ! player -> in_combat ) return 0.01;
    return druid_cat_attack_t::execute_time();
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_omen_of_clarity( this );
    }
  }
};

// Berserk =================================================================

struct berserk_cat_t : public druid_cat_attack_t
{
  int tigers_fury;

  berserk_cat_t( player_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "berserk_cat", player ),
    tigers_fury( 0 )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.berserk );

    option_t options[] =
    {
      { "tigers_fury", OPT_BOOL, &tigers_fury },
      { NULL, OPT_UNKNOWN, NULL }
    };

    parse_options( options, options_str );

    cooldown = p -> get_cooldown( "berserk" );
    cooldown -> duration = 180;
    id = 50334;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_berserk -> trigger();
    // Berserk cancels TF
    p -> buffs_tigers_fury -> expire();
    update_ready();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    if ( tigers_fury && ! p -> buffs_tigers_fury -> check() )
      return false;
    return druid_cat_attack_t::ready();
  }
};

// Claw ====================================================================

struct claw_t : public druid_cat_attack_t
{
  claw_t( player_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "claw", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 8, 370, 370, 0, 45 },
      { 73, 7, 300, 300, 0, 45 },
      { 67, 6, 190, 190, 0, 45 },
      { 58, 7, 115, 115, 0, 45 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48570 );

    weapon = &( p -> main_hand_weapon );
    adds_combo_points = true;
    may_crit          = true;
    base_multiplier  *= 1.0 + p -> talents.savage_fury * 0.1;
  }
};

// Maim ======================================================================

struct maim_t : public druid_cat_attack_t
{
  double* combo_point_dmg;

  maim_t( player_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "maim", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_power_mod = 0;

    may_crit = true;
    requires_combo_points = true;
    base_cost = 35;
    cooldown -> duration = 10;

    static double dmg_74[] = { 224, 382, 540, 698, 856 };
    static double dmg_62[] = { 129, 213, 297, 381, 465 };

    combo_point_dmg = ( p -> level >= 74 ? dmg_74 : dmg_62 );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    base_dd_min = base_dd_max = combo_point_dmg[ p -> buffs_combo_points -> stack() - 1 ];
    druid_cat_attack_t::execute();
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return druid_cat_attack_t::ready();
  }
};

// Mangle (Cat) ============================================================

struct mangle_cat_t : public druid_cat_attack_t
{
  mangle_cat_t( player_t* player, const std::string& options_str ) :
      druid_cat_attack_t( "mangle_cat", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.mangle );

    // By default, do not overwrite Mangle
    max_mangle_expire = 0.001;

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 283, 283, 0, 45 },
      { 75, 4, 239, 239, 0, 45 },
      { 68, 3, 165, 165, 0, 45 },
      { 58, 2, 128, 128, 0, 45 },
      {  0, 0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48566 );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 2.0;

    adds_combo_points = true;
    may_crit          = true;

    base_cost -= p -> talents.ferocity;
    base_cost -= p -> talents.improved_mangle * 2;

    base_multiplier  *= 1.0 + ( p -> talents.savage_fury * 0.1 +
				p -> glyphs.mangle       * 0.1 );
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    if ( result_is_hit() )
    {
      druid_t* p = player -> cast_druid();
      target_t* t = sim -> target;
      t -> debuffs.mangle -> trigger();
      trigger_infected_wounds( this );
      p -> buffs_terror -> trigger();
      p -> buffs_corruptor -> trigger();
      p -> buffs_mutilation -> trigger();
      action_callback_t::trigger( player -> spell_direct_result_callbacks[ RESULT_HIT ], this );
    }
  }
};

// Rake ====================================================================

struct rake_t : public druid_cat_attack_t
{
  rake_t( player_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "rake", player, SCHOOL_BLEED, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 7, 190, 190, 387, 40 },
      { 72, 6, 150, 150, 321, 40 },
      { 64, 5,  90,  90, 138, 40 },
      { 54, 4,  64,  90,  99, 40 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48574 );

    adds_combo_points = true;
    may_crit          = true;
    base_tick_time    = 3.0;
    num_ticks         = 3;
    if ( p -> set_bonus.tier9_2pc_melee() ) num_ticks++;
    tick_may_crit = ( p -> set_bonus.tier10_4pc_melee() == 1 );

    direct_power_mod  = 0.01;
    tick_power_mod    = 0.06;
    base_cost        -= p -> talents.ferocity;
    base_multiplier  *= 1.0 + p -> talents.savage_fury * 0.1;
  }
  virtual void tick()
  {
    druid_t* p = player -> cast_druid();
    druid_cat_attack_t::tick();
    trigger_t8_2pc_melee( this );
    p -> buffs_t10_feral_relic -> trigger();
  }
};

// Rip ======================================================================

struct rip_t : public druid_cat_attack_t
{
  double* combo_point_dmg;

  rip_t( player_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "rip", player, SCHOOL_BLEED, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 49800;

    may_crit              = false;
    requires_combo_points = true;
    base_cost             = 30;
    if ( p -> set_bonus.tier10_2pc_melee() )
      base_cost -= 10;
    base_tick_time        = 2.0;

    num_ticks = 6 + ( p -> glyphs.rip * 2 ) + ( p -> set_bonus.tier7_2pc_melee() * 2 );

    static double dmg_80[] = { 36+93*1, 36+93*2, 36+93*3, 36+93*4, 36+93*5 };
    static double dmg_71[] = { 30+67*1, 30+67*2, 30+67*3, 30+67*4, 30+67*5 };
    static double dmg_67[] = { 24+48*1, 24+48*2, 24+48*3, 24+48*4, 24+48*5 };
    static double dmg_60[] = { 17+28*1, 17+28*2, 17+28*3, 17+28*4, 17+28*5 };


    combo_point_dmg = ( p -> level >= 80 ? dmg_80 :
                        p -> level >= 71 ? dmg_71 :
                        p -> level >= 67 ? dmg_67 :
                        dmg_60 );

    tick_may_crit = ( p -> talents.primal_gore != 0 );
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_cat_attack_t::execute();
    if ( result_is_hit() )
    {
      added_ticks = 0;
      num_ticks = 6 + ( p -> glyphs.rip * 2 ) + ( p -> set_bonus.tier7_2pc_melee() * 2 );
      update_ready();
    }
  }

  virtual void tick()
  {
    druid_cat_attack_t::tick();
    trigger_t8_2pc_melee( this );
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();
    tick_power_mod = p -> buffs_combo_points -> stack() * 0.01;
    base_td_init   = p -> combo_point_rank( combo_point_dmg );
    if ( p -> idols.worship )
      base_td_init += p -> buffs_combo_points -> stack() * 21;
    druid_cat_attack_t::player_buff();
  }
};

// Savage Roar =============================================================

struct savage_roar_t : public druid_cat_attack_t
{
  double buff_value;
  savage_roar_t( player_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "savage_roar", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    requires_combo_points = true;
    base_cost = 25;
    id = 52610;
    harmful = false;

    buff_value = 0.30;
    if( p -> glyphs.savage_roar ) buff_value += 0.03;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    double duration = 9.0 + 5.0 * p -> buffs_combo_points -> stack();
    if ( p -> set_bonus.tier8_4pc_melee() ) duration += 8.0;

    // execute clears CP, so has to be after calculation duration
    druid_cat_attack_t::execute();

    p -> buffs_savage_roar -> duration = duration;
    p -> buffs_savage_roar -> trigger( 1, buff_value );
    //p -> buffs_combo_points -> expire();
  }
};

// Shred ===================================================================

struct shred_t : public druid_cat_attack_t
{
  int omen_of_clarity;
  int extend_rip;

  shred_t( player_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "shred", player, SCHOOL_PHYSICAL, TREE_FERAL ),
    omen_of_clarity( 0 ), extend_rip( 0 )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { "omen_of_clarity", OPT_BOOL, &omen_of_clarity },
      { "extend_rip",      OPT_BOOL, &extend_rip      },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 296, 296, 0, 60 },
      { 75, 8, 251, 251, 0, 60 },
      { 70, 7, 180, 180, 0, 60 },
      { 61, 6, 105, 105, 0, 60 },
      { 54, 5,  80,  80, 0, 60 },
      {  0, 0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48572 );

    weapon_multiplier *= 2.25;

    weapon = &( p -> main_hand_weapon );
    requires_position  = POSITION_BACK;
    adds_combo_points  = true;
    may_crit           = true;
    base_cost         -= 9 * p -> talents.shredding_attacks;

    if ( p -> idols.ravenous_beast )
      base_dd_adder = 90;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_cat_attack_t::execute();
    if ( p -> glyphs.shred &&
         p -> dots_rip -> ticking()  &&
         p -> dots_rip -> action -> added_ticks < 3 )
    {
      p -> dots_rip -> action -> extend_duration( 1 );
    }
    if( result_is_hit() )
    {
      p -> buffs_mutilation -> trigger();
      trigger_infected_wounds( this );
      action_callback_t::trigger( player -> spell_direct_result_callbacks[ RESULT_HIT ], this );
    }
  }

  virtual void player_buff()
  {
    druid_t*  p = player -> cast_druid();
    target_t* t = sim -> target;

    druid_cat_attack_t::player_buff();

    if ( t -> debuffs.mangle -> up() || t -> debuffs.trauma -> up() ) player_multiplier *= 1.30;

    if ( t -> debuffs.bleeding -> check() )
    {
      player_multiplier *= 1 + 0.04 * p -> talents.rend_and_tear;
    }

  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( omen_of_clarity )
      if ( ! p -> buffs_omen_of_clarity -> may_react() )
        return false;

    if ( extend_rip )
      if ( ! p -> glyphs.shred ||
           ! p -> dots_rip -> ticking() ||
           ( p -> dots_rip -> action -> added_ticks == 3 ) )
        return false;

    return druid_cat_attack_t::ready();
  }
};

// Tigers Fury =============================================================

struct tigers_fury_t : public druid_cat_attack_t
{
  double tiger_value;
  tigers_fury_t( player_t* player, const std::string& options_str ) :
      druid_cat_attack_t( "tigers_fury", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration    = 30.0 - 3.0 * p -> set_bonus.tier7_4pc_melee();
    trigger_gcd = 0;
    tiger_value = util_t::ability_rank( p -> level,  80.0,79, 60.0,71,  40.0,0 );
    id = 50213;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    druid_t* p = player -> cast_druid();

    if ( p -> talents.king_of_the_jungle )
    {
      p -> resource_gain( RESOURCE_ENERGY, p -> talents.king_of_the_jungle * 20, p -> gains_tigers_fury );
    }

    p -> buffs_tigers_fury -> trigger( 1, tiger_value );
    update_ready();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_berserk -> check() )
      return false;

    return druid_cat_attack_t::ready();
  }
};

// Ferocious Bite ============================================================

struct ferocious_bite_t : public druid_cat_attack_t
{
  struct double_pair { double min, max; };
  double excess_engery_mod, excess_energy;
  double_pair* combo_point_dmg;

  ferocious_bite_t( player_t* player, const std::string& options_str ) :
      druid_cat_attack_t( "ferocious_bite", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 48577;

    requires_combo_points = true;
    may_crit  = true;
    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;

    base_cost = 35;

    base_multiplier *= 1.0 + ( p -> talents.feral_aggression * 0.03 );

    static double_pair dmg_78[] = { { 410, 550 }, { 700, 840 }, { 990, 1130 }, { 1280, 1420 }, { 1570, 1710 } };
    static double_pair dmg_72[] = { { 334, 682 }, { 570, 682 }, { 806,  918 }, { 1042, 1154 }, { 1278, 1390 } };
    static double_pair dmg_63[] = { { 226, 292 }, { 395, 461 }, { 564,  630 }, {  733,  799 }, {  902,  968 } };
    static double_pair dmg_60[] = { { 199, 259 }, { 346, 406 }, { 493,  533 }, {  640,  700 }, {  787,  847 } };

    combo_point_dmg   = ( p -> level >= 78 ? dmg_78 :
                          p -> level >= 72 ? dmg_72 :
                          p -> level >= 63 ? dmg_63 :
                          dmg_60 );
    excess_engery_mod = ( p -> level >= 78 ? 9.4 :
                          p -> level >= 72 ? 7.7 :
                          p -> level >= 63 ? 3.4 :
                          2.1 );  // Up to 30 additional energy is converted into damage
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    base_dd_min = combo_point_dmg[ p -> buffs_combo_points -> current_stack - 1 ].min;
    base_dd_max = combo_point_dmg[ p -> buffs_combo_points -> current_stack - 1 ].max;

    direct_power_mod = 0.07 * p -> buffs_combo_points -> stack();

    excess_energy = ( p -> resource_current[ RESOURCE_ENERGY ] - druid_cat_attack_t::cost() );

    if ( excess_energy > 0 )
    {
      // There will be energy left after the Ferocious Bite of which up to 30 will also be converted into damage.
      // Additional damage AND additinal scaling from AP.
      // druid_cat_attack_t::cost() takes care of OoC handling.

      excess_energy     = ( excess_energy > 30 ? 30 : excess_energy );
      direct_power_mod += excess_energy / 410.0;
      base_dd_adder     = excess_engery_mod * excess_energy;
    }
    else
    {
      excess_energy = 0;
      base_dd_adder = 0;
    }

    druid_cat_attack_t::execute();
    if ( excess_energy > 0 )
    {
      direct_power_mod -= excess_energy / 410.0;
    }
  }

  virtual void consume_resource()
  {
    // Ferocious Bite consumes 35+x energy, with 0 <= x <= 30.
    // Consumes the base_cost and handles Omen of Clarity
    druid_cat_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      // Let the additional energy consumption create it's own debug log entries.
      if ( sim -> debug )
        log_t::output( sim, "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_energy, util_t::resource_type_string( resource ), name() );

      player -> resource_loss( resource, excess_energy );
      stats -> consume_resource( excess_energy );
    }

  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    druid_cat_attack_t::player_buff();

    if ( sim -> target -> debuffs.bleeding -> check() )
    {
      player_crit += 0.05 * p -> talents.rend_and_tear;
    }

  }
};

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

// druid_bear_attack_t::parse_options ======================================

void druid_bear_attack_t::parse_options( option_t*          options,
                                         const std::string& options_str )
{
  option_t base_options[] =
  {
    { "berserk",         OPT_INT,  &berserk             },
    { "rage>",           OPT_FLT,  &min_rage            },
    { "rage<",           OPT_FLT,  &max_rage            },
    { "mangle>",         OPT_FLT,  &min_mangle_expire   },
    { "mangle<",         OPT_FLT,  &max_mangle_expire   },
    { "lacerate>",       OPT_FLT,  &min_lacerate_expire },
    { "lacerate<",       OPT_FLT,  &max_lacerate_expire },
    { "lacerate_stack>", OPT_INT,  &min_lacerate_stack  },
    { "lacerate_stack<", OPT_INT,  &max_lacerate_stack  },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// druid_bear_attack_t::cost ===============================================

double druid_bear_attack_t::cost() SC_CONST
{
  druid_t* p = player -> cast_druid();
  double c = attack_t::cost();
  if ( harmful && p -> buffs_omen_of_clarity -> check() ) return 0;
  return c;
}

// druid_bear_attack_t::consume_resource ===================================

void druid_bear_attack_t::consume_resource()
{
  druid_t* p = player -> cast_druid();
  attack_t::consume_resource();
  if ( harmful && p -> buffs_omen_of_clarity -> up() )
  {
    // Treat the savings like a rage gain.
    double amount = attack_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> buffs_omen_of_clarity -> expire();
    }
  }
}

// druid_bear_attack_t::execute ============================================

void druid_bear_attack_t::execute()
{
  attack_t::execute();
  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      trigger_primal_fury( this );
    }
  }
}

// druid_bear_attack_t::player_buff ========================================

void druid_bear_attack_t::player_buff()
{
  druid_t* p = player -> cast_druid();
  attack_t::player_buff();
  if ( p -> talents.naturalist )
  {
    player_multiplier *= 1.0 + p -> talents.naturalist * 0.02;
  }
  if ( p -> talents.master_shapeshifter )
  {
    player_multiplier *= 1.0 + p -> talents.master_shapeshifter * 0.02;
  }
  if ( p -> talents.king_of_the_jungle && p -> buffs_enrage -> up() )
  {
    player_multiplier *= 1.0 + p -> talents.king_of_the_jungle * 0.05;
  }
}

// druid_bear_attack_t::ready ==============================================

bool druid_bear_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();
  target_t* t = sim -> target;

  if ( ! p -> buffs_bear_form -> check() )
    return false;

  if ( berserk && ! p -> buffs_berserk -> check() )
    return false;

  if ( min_rage > 0 )
    if ( p -> resource_current[ RESOURCE_RAGE ] < min_rage )
      return false;

  if ( max_rage > 0 )
    if ( p -> resource_current[ RESOURCE_RAGE ] > max_rage )
      return false;

  if ( min_mangle_expire > 0 )
    if ( t -> debuffs.mangle -> remains_lt( min_mangle_expire ) )
      return false;

  if ( max_mangle_expire > 0 )
    if ( t -> debuffs.mangle -> remains_gt( max_mangle_expire ) )
      return false;

  if ( min_lacerate_expire > 0 )
    if ( p -> buffs_lacerate -> remains_lt( min_lacerate_expire ) )
      return false;

  if ( max_lacerate_expire > 0 )
    if ( p -> buffs_lacerate -> remains_gt( max_lacerate_expire ) )
      return false;

  if ( min_lacerate_stack > 0 )
    if ( p -> buffs_lacerate -> current_stack < min_lacerate_stack )
      return false;

  if ( max_lacerate_stack > 0 )
    if ( p -> buffs_lacerate -> current_stack > max_lacerate_stack )
      return false;

  return true;
}

// Bear Melee Attack =======================================================

struct bear_melee_t : public druid_bear_attack_t
{
  bear_melee_t( player_t* player ) :
    druid_bear_attack_t( "bear_melee", player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false )
  {
    base_dd_min = base_dd_max = 1;
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
    may_crit    = true;
  }

  virtual double execute_time() SC_CONST
  {
    if ( ! player -> in_combat ) return 0.01;
    return druid_bear_attack_t::execute_time();
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    // If any of our Maul actions are "ready", then execute Maul in place of the regular melee swing.
    action_t* active_maul = 0;

    if ( ! proc )
    {
      for( int i=0; i < p -> num_active_mauls; i++ )
      {
        action_t* a = p -> active_mauls[ i ];
        if ( a -> ready() )
        {
          active_maul = a;
          break;
        }
      }
    }

    if ( active_maul )
    {
      active_maul -> execute();
      schedule_execute();
    }
    else
    {
      druid_bear_attack_t::execute();
      if ( result_is_hit() )
      {
        trigger_rage_gain( this );
        trigger_omen_of_clarity( this );
      }
    }
  }
};

// Bash ====================================================================

struct bash_t : public druid_bear_attack_t
{
  bash_t( player_t* player, const std::string& options_str ) :
    druid_bear_attack_t( "bash", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = 10;
    cooldown -> duration  = 60 - p -> talents.brutal_impact * 15;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return druid_bear_attack_t::ready();
  }
};

// Berserk =================================================================

struct berserk_bear_t : public druid_bear_attack_t
{
  berserk_bear_t( player_t* player, const std::string& options_str ) :
    druid_bear_attack_t( "berserk_bear", player )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.berserk );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };

    parse_options( options, options_str );

    cooldown = p -> get_cooldown( "berserk" );
    cooldown -> duration = 180;
    id = 50334;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_berserk -> trigger();
    p -> cooldowns_mangle_bear -> reset();
    update_ready();
  }
};

// Lacerate ================================================================

struct lacerate_t : public druid_bear_attack_t
{
  lacerate_t( player_t* player, const std::string& options_str ) :
      druid_bear_attack_t( "lacerate",  player, SCHOOL_BLEED, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL,     OPT_UNKNOWN, NULL       }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 3, 88, 88, 64, 15 },
      { 73, 2, 70, 70, 51, 15 },
      { 66, 1, 31, 31, 31, 15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    may_crit = true;
    num_ticks = 5;
    base_tick_time = 3.0;
    direct_power_mod = 0.01;
    tick_power_mod = 0.01;
    tick_may_crit = ( p -> talents.primal_gore != 0 );
    base_cost -= p -> talents.shredding_attacks;
    dot_behavior = DOT_REFRESH;

    if ( p -> set_bonus.tier9_2pc_melee() ) base_td_multiplier *= 1.05;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_bear_attack_t::execute();
    if( result_is_hit() )
    {
      p -> buffs_lacerate -> trigger();
      base_td_multiplier = p -> buffs_lacerate -> current_stack;
    }
  }

  virtual void tick()
  {
    druid_bear_attack_t::tick();
    trigger_t8_2pc_melee( this );
  }

  virtual void last_tick()
  {
    druid_t* p = player -> cast_druid();
    druid_bear_attack_t::last_tick();
    p -> buffs_lacerate -> expire();
  }
};

// Mangle (Bear) ===========================================================

struct mangle_bear_t : public druid_bear_attack_t
{
  mangle_bear_t( player_t* player, const std::string& options_str ) :
      druid_bear_attack_t( "mangle_bear", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.mangle );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 260, 260, 0, 20 },
      { 75, 4, 219, 219, 0, 20 },
      { 68, 3, 135, 135, 0, 20 },
      { 58, 2, 105, 105, 0, 20 },
      {  0, 0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48564 );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 1.15;

    may_crit = true;
    base_cost -= p -> talents.ferocity;

    base_multiplier *= 1.0 + ( p -> talents.savage_fury * 0.10 +
			       p -> glyphs.mangle       * 0.10 );

    cooldown = p -> get_cooldown( "mangle_bear" );
    cooldown -> duration = 6.0 - p -> talents.improved_mangle * 0.5;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_bear_attack_t::execute();
    if ( p -> buffs_berserk -> up() ) cooldown -> reset();
    if ( result_is_hit() )
    {
      target_t* t = sim -> target;
      t -> debuffs.mangle -> trigger();
      trigger_infected_wounds( this );
      p -> buffs_terror -> trigger();
      p -> buffs_corruptor -> trigger();
      p -> buffs_mutilation -> trigger();
    }
  }
};

// Maul ====================================================================

struct maul_t : public druid_bear_attack_t
{
  maul_t( player_t* player, const std::string& options_str ) :
      druid_bear_attack_t( "maul",  player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 10, 578, 578, 0, 15 },
      { 73,  9, 472, 472, 0, 15 },
      { 67,  8, 290, 290, 0, 15 },
      { 58,  7, 192, 192, 0, 15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    background   = true;
    may_crit     = true;
    trigger_gcd  = 0;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = false;

    base_cost -= p -> talents.ferocity;
    base_multiplier *= 1.0 + p -> talents.savage_fury * 0.10;

    p -> active_mauls.push_back( this );
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();
    if( result_is_hit() )
    {
      trigger_omen_of_clarity( this );
      trigger_infected_wounds( this );
    }
  }

  virtual void player_buff()
  {
    druid_t*  p = player -> cast_druid();
    target_t* t = sim -> target;

    druid_bear_attack_t::player_buff();

    if ( t -> debuffs.mangle -> up() || t -> debuffs.trauma -> up() ) player_multiplier *= 1.30;

    if ( t -> debuffs.bleeding -> check() )
    {
      player_multiplier *= 1 + 0.04 * p -> talents.rend_and_tear;
    }

  }

};

// Swipe (Bear) ============================================================

struct swipe_bear_t : public druid_bear_attack_t
{
  swipe_bear_t( player_t* player, const std::string& options_str ) :
      druid_bear_attack_t( "swipe_bear", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 77, 8, 108, 108, 0, 20 },
      { 72, 7,  95,  95, 0, 20 },
      { 64, 6,  76,  76, 0, 20 },
      { 54, 5,  54,  54, 0, 20 },
      { 44, 4,  37,  37, 0, 20 },
      { 34, 3,  19,  19, 0, 20 },
      { 24, 2,  13,  13, 0, 20 },
      { 16, 1,   9,   9, 0, 20 },
      {  0, 0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48562 );

    weapon_multiplier = 0;
    direct_power_mod = 0.07;
    may_crit = true;
    base_cost -= p -> talents.ferocity;

    base_multiplier *= 1.0 + p -> talents.feral_instinct * 0.10;
  }

  virtual void assess_damage( double amount,
                               int    dmg_type )
  {
    druid_bear_attack_t::assess_damage( amount, dmg_type );

    for ( int i=0; i < sim -> target -> adds_nearby && i < 10; i ++ )
    {
      druid_bear_attack_t::additional_damage( amount, dmg_type );
    }
  }
};

// ==========================================================================
// Druid Spell
// ==========================================================================

// druid_spell_t::parse_options ============================================

void druid_spell_t::parse_options( option_t*          options,
                                   const std::string& options_str )
{
  option_t base_options[] =
  {
    { "skip_on_eclipse",  OPT_INT, &skip_on_eclipse        },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// druid_spell_t::ready ====================================================

bool druid_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();

  if ( skip_on_eclipse > 0 )
    if ( p -> buffs_eclipse_lunar -> may_react() || p -> buffs_eclipse_solar -> may_react() )
      return false;

  return true;
}

// druid_spell_t::cost =====================================================

double druid_spell_t::cost() SC_CONST
{
  druid_t* p = player -> cast_druid();
  if ( harmful && p -> buffs_omen_of_clarity -> check() ) return 0;
  return spell_t::cost();
}

// druid_spell_t::haste ====================================================

double druid_spell_t::haste() SC_CONST
{
  druid_t* p = player -> cast_druid();
  double h = spell_t::haste();
  if ( p -> talents.celestial_focus ) h *= 1.0 / ( 1.0 + p -> talents.celestial_focus * 0.01 );
  if ( p -> buffs_natures_grace -> up() )
  {
    h *= 1.0 / ( 1.0 + 0.20 );
  }
  return h;
}

// druid_spell_t::execute_time =============================================

double druid_spell_t::execute_time() SC_CONST
{
  druid_t* p = player -> cast_druid();
  if ( p -> buffs_natures_swiftness -> check() ) return 0;
  return spell_t::execute_time();
}

// druid_spell_t::schedule_execute =========================================

void druid_spell_t::schedule_execute()
{
  druid_t* p = player -> cast_druid();

  spell_t::schedule_execute();

  if ( base_execute_time > 0 )
  {
    p -> buffs_natures_swiftness -> expire();
  }
}

// druid_spell_t::execute ==================================================

void druid_spell_t::execute()
{
  druid_t* p = player -> cast_druid();

  spell_t::execute();

  if ( result == RESULT_CRIT )
  {
    if ( p -> buffs_moonkin_form -> check() && ! aoe && ! proc )
    {
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.02, p -> gains_moonkin_form );
    }

    p -> buffs_natures_grace -> trigger();
  }

  if ( ! aoe )
  {
    trigger_omen_of_clarity( this );
  }
}

// druid_spell_t::consume_resource =========================================

void druid_spell_t::consume_resource()
{
  druid_t* p = player -> cast_druid();
  spell_t::consume_resource();
  if ( harmful && p -> buffs_omen_of_clarity -> up() )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> buffs_omen_of_clarity -> expire();
    }
  }
}

// druid_spell_t::player_buff ==============================================

void druid_spell_t::player_buff()
{
  druid_t* p = player -> cast_druid();

  spell_t::player_buff();

  if ( p -> buffs_moonkin_form -> check() )
  {
    player_multiplier *= 1.0 + p -> talents.master_shapeshifter * 0.02;
  }
  if ( school == SCHOOL_ARCANE || school == SCHOOL_NATURE )
    player_multiplier *= 1.0 + p -> buffs_t10_2pc_caster -> value();

  player_multiplier *= 1.0 + p -> talents.earth_and_moon * 0.02;
}

// druid_spell_t::target_debuff ============================================

void druid_spell_t::target_debuff( int dmg_type )
{
  druid_t*  p = player -> cast_druid();
  target_t* t = sim -> target;
  spell_t::target_debuff( dmg_type );
  if ( t -> debuffs.faerie_fire -> up() )
  {
    target_crit += p -> talents.improved_faerie_fire * 0.01;
  }
}

// Auto Attack =============================================================

struct auto_attack_t : public action_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", player )
  {
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;
    p -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Enrage ==================================================================

struct enrage_t : public druid_spell_t
{
  double max_rage;

  enrage_t( player_t* player, const std::string& options_str ) :
    druid_spell_t( "enrage", player, RESOURCE_NONE, TREE_FERAL )
  {
    option_t options[] =
    {
      { "rage<", OPT_FLT,     &max_rage },
      { NULL,    OPT_UNKNOWN, NULL      }
    };
    max_rage = 0;
    parse_options( options, options_str );
    trigger_gcd = 0;
    cooldown -> duration = 60;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_enrage -> trigger();
    p -> resource_gain( RESOURCE_RAGE, 20, p -> gains_enrage );
    update_ready();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    if ( ! p -> buffs_bear_form -> check() )
      return false;
    if ( max_rage > 0 )
      if ( p -> resource_current[ RESOURCE_RAGE ] > max_rage )
        return false;
    return druid_spell_t::ready();
  }
};

// Faerie Fire (Feral) ======================================================

struct faerie_fire_feral_t : public druid_spell_t
{
  int debuff_only;

  faerie_fire_feral_t( player_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire_feral", player, SCHOOL_PHYSICAL, TREE_FERAL ), debuff_only( 0 )
  {
    option_t options[] =
    {
      { "debuff_only", OPT_BOOL, &debuff_only },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit = true;
    cooldown -> duration = 6.0;
    base_dd_min = base_dd_max = 1;
    direct_power_mod = 1.0;
    base_spell_power_multiplier = 0;
    base_attack_power_multiplier = 0.15;
    id = 16857;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    if ( p -> buffs_bear_form -> check() )
    {
      // The damage component is only active in (Dire) Bear Form
      spell_t::execute();
    }
    else
    {
      update_ready();
    }
    sim -> target -> debuffs.faerie_fire -> trigger();
  }

  virtual bool ready()
  {
    if ( debuff_only )
      if ( sim -> target -> debuffs.faerie_fire -> up() )
        return false;

    return druid_spell_t::ready();
  }
};

// Faerie Fire Spell =======================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "faerie_fire", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.07;
    id = 770;
  }

  virtual void execute()
  {
    druid_t*  p = player -> cast_druid();
    target_t* t = sim -> target;
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    t -> debuffs.faerie_fire -> trigger();
    if ( p -> talents.improved_faerie_fire >= t -> debuffs.improved_faerie_fire -> current_value )
    {
      t -> debuffs.improved_faerie_fire -> trigger( 1, p -> talents.improved_faerie_fire );
    }
  }

  virtual bool ready()
  {
    druid_t*  p = player -> cast_druid();
    target_t* t = sim -> target;

    if ( t -> debuffs.faerie_fire -> up() )
    {
      if ( t -> debuffs.improved_faerie_fire -> current_value >= p -> talents.improved_faerie_fire )
              return false;

      if ( t -> debuffs.misery -> current_value > p -> talents.improved_faerie_fire )
              return false;
    }

    return druid_spell_t::ready();
  }
};

// Innervate Spell =========================================================

struct innervate_t : public druid_spell_t
{
  int trigger;
  player_t* innervate_target;

  innervate_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "innervate", player, SCHOOL_NATURE, TREE_BALANCE ), trigger( 0 )
  {
    druid_t* p = player -> cast_druid();

    std::string target_str;
    option_t options[] =
    {
      { "trigger", OPT_INT,    &trigger    },
      { "target",  OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 29166;

    base_cost = 0.0;
    base_execute_time = 0;
    cooldown -> duration  = 240;

    harmful   = false;

    // If no target is set, assume we have innervate for ourself
    innervate_target = target_str.empty() ? p : sim -> find_player( target_str );
    assert ( innervate_target != 0 );
  }
  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(),name() );

    consume_resource();
    update_ready();

    // In 3.1.2 Innervate is changed to 450% of base mana of the caster
    // Per second: player -> resource_base[ RESOURCE_MANA ]* 4.5 / 20.0
    // ~
    // Glyph of Innervate: 90%
    // Per second: player -> resource_base[ RESOURCE_MANA ]* 0.9 / 20.0
    // In ::regen() we then just give the player:
    // buffs.innervate * periodicity mana
    //
    // 3.2.0: The ManaPerSecond from Innervate is the same, but duration
    // and cooldown got halfed.

    innervate_target -> buffs.innervate -> trigger( 1, player -> resource_base[ RESOURCE_MANA ]* 4.5 / 20.0);

    druid_t* p = player -> cast_druid();
    p -> buffs_glyph_of_innervate -> trigger( 1, player -> resource_base[ RESOURCE_MANA ]* 0.9 / 20.0 );
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    if ( trigger < 0 )
      return ( player -> resource_current[ RESOURCE_MANA ] + trigger ) < 0;

    return ( player -> resource_max    [ RESOURCE_MANA ] -
             player -> resource_current[ RESOURCE_MANA ] ) > trigger;
  }
};

// Insect Swarm Spell ======================================================

struct insect_swarm_t : public druid_spell_t
{
  insect_swarm_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "insect_swarm", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.insect_swarm );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 7, 0, 0, 215, 0.08 },
      { 70, 6, 0, 0, 172, 175  },
      { 60, 5, 0, 0, 124, 155  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48468 );

    base_execute_time = 0;
    base_tick_time    = 2.0;
    num_ticks         = 6;
    tick_power_mod    = 0.2;

    base_multiplier *= 1.0 + ( util_t::talent_rank( p -> talents.genesis, 5, 0.01 ) +
                               ( p -> glyphs.insect_swarm          ? 0.30 : 0.00 )  +
                               ( p -> set_bonus.tier7_2pc_caster() ? 0.10 : 0.00 ) );

    if ( p -> idols.crying_wind )
    {
      // Druid T8 Balance Relic -- Increases the spell power of your Insect Swarm by 374.
      base_spell_power += 374;
    }

    if ( p -> talents.natures_splendor ) num_ticks++;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();
    if ( ! p -> glyphs.insect_swarm )
      p -> sim -> target -> debuffs.insect_swarm -> trigger();
  }

  virtual void tick()
  {
    druid_spell_t::tick();
    druid_t* p = player -> cast_druid();
    p -> buffs_t8_4pc_caster -> trigger();
    p -> buffs_t10_balance_relic -> trigger();
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( skip_on_eclipse < 0 )
      if ( p -> buffs_eclipse_lunar -> check() )
        return false;

    return true;
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  double base_crit_tick, base_crit_direct;

  moonfire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "moonfire", player, SCHOOL_ARCANE, TREE_BALANCE ),
      base_crit_tick( 0 ), base_crit_direct( 0 )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 14, 406, 476, 200, 0.21 },
      { 75, 13, 347, 407, 171, 0.21 },
      { 70, 12, 305, 357, 150, 495  },
      { 64, 11, 220, 220, 111, 430  },
      { 58, 10, 189, 221,  96, 375  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48463 );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 4;
    direct_power_mod  = 0.15;
    tick_power_mod    = 0.13;
    may_crit          = true;
    tick_may_crit     = ( p -> set_bonus.tier9_2pc_caster() != 0 );

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.moonglow,    3, 0.03 );
    base_crit = util_t::talent_rank( p -> talents.improved_moonfire, 2, 0.05 );

    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

    double multiplier_td = ( util_t::talent_rank( p -> talents.moonfury,          3, 0.03, 0.06, 0.10 ) +
                             util_t::talent_rank( p -> talents.improved_moonfire, 2, 0.05 ) +
                             util_t::talent_rank( p -> talents.genesis,           5, 0.01 ) );

    double multiplier_dd = ( util_t::talent_rank( p -> talents.moonfury,          3, 0.03, 0.06, 0.10 ) +
                             util_t::talent_rank( p -> talents.improved_moonfire, 2, 0.05 ) );

    if ( p -> glyphs.moonfire )
    {
      multiplier_dd -= 0.90;
      multiplier_td += 0.75;
    }
    base_dd_multiplier *= 1.0 + multiplier_dd;
    base_td_multiplier *= 1.0 + multiplier_td;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    base_crit = util_t::talent_rank( p -> talents.improved_moonfire, 2, 0.05 );
    druid_spell_t::execute();
    base_crit = 0;

    if ( result_is_hit() )
    {
      num_ticks = 4;
      added_ticks = 0;
      if ( p -> talents.natures_splendor     ) num_ticks++;
      if ( p -> set_bonus.tier6_2pc_caster() ) num_ticks++;
      update_ready();
      p -> buffs_unseen_moon -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( skip_on_eclipse < 0 )
      if ( p -> buffs_eclipse_solar -> check() )
        return false;

    return true;
  }
  virtual void tick()
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::tick();
    p -> buffs_lunar_fury -> trigger();
    p -> buffs_t10_balance_relic -> trigger();
  }
};

// Bear Form Spell ========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "bear_form", player, SCHOOL_NATURE, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();
    trigger_gcd = 0;
    base_execute_time = 0;
    base_cost = 0;
    id = 768;
    if ( ! p -> bear_melee_attack ) p -> bear_melee_attack = new bear_melee_t( p );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    weapon_t* w = &( p -> main_hand_weapon );

    if ( w -> type != WEAPON_BEAST )
    {
      w -> type = WEAPON_BEAST;
      w -> school = SCHOOL_PHYSICAL;
      w -> damage = 54.8 * 2.5;
      w -> swing_time = 2.5;
    }

    // Force melee swing to restart if necessary
    if ( p -> main_hand_attack ) p -> main_hand_attack -> cancel();

    p -> main_hand_attack = p -> bear_melee_attack;
    p -> main_hand_attack -> weapon = w;

    if ( p -> buffs_cat_form     -> check() ) p -> buffs_cat_form     -> expire();
    if ( p -> buffs_moonkin_form -> check() ) p -> buffs_moonkin_form -> expire();

    p -> buffs_bear_form -> start();
    p -> base_gcd = 1.0;
    p -> reset_gcd();

    p -> dodge += 0.02 * p -> talents.feral_swiftness + 0.02 * p -> talents.natural_reaction;
    p -> armor_multiplier += 3.7 * ( 1.0 + 0.11 * p -> talents.survival_of_the_fittest );

    if ( p -> talents.leader_of_the_pack )
    {
      sim -> auras.leader_of_the_pack -> trigger();
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return ! d -> buffs_bear_form -> check();
  }
};

// Cat Form Spell =========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "cat_form", player, SCHOOL_NATURE, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();
    trigger_gcd = 0;
    base_execute_time = 0;
    base_cost = 0;
    id = 768;
    if ( ! p -> cat_melee_attack ) p -> cat_melee_attack = new cat_melee_t( p );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    weapon_t* w = &( p -> main_hand_weapon );

    if ( w -> type != WEAPON_BEAST )
    {
      w -> type = WEAPON_BEAST;
      w -> school = SCHOOL_PHYSICAL;
      w -> min_dmg = 54.8; // FIX-ME: find the correct min and max values.
      w -> max_dmg = 54.8;
      w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
      w -> swing_time = 1.0;
    }

    // Force melee swing to restart if necessary
    if ( p -> main_hand_attack ) p -> main_hand_attack -> cancel();

    p -> main_hand_attack = p -> cat_melee_attack;
    p -> main_hand_attack -> weapon = w;

    if ( p -> buffs_bear_form    -> check() ) p -> buffs_bear_form    -> expire();
    if ( p -> buffs_moonkin_form -> check() ) p -> buffs_moonkin_form -> expire();

    p -> dodge += 0.02 * p -> talents.feral_swiftness;

    p -> buffs_cat_form -> start();
    p -> base_gcd = 1.0;
    p -> reset_gcd();

    if ( p -> talents.leader_of_the_pack )
    {
      sim -> auras.leader_of_the_pack -> trigger();
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return ! d -> buffs_cat_form -> check();
  }
};

// Moonkin Form Spell =====================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "moonkin_form", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.moonkin_form );
    trigger_gcd = 0;
    base_execute_time = 0;
    base_cost = 0;
    id = 24858;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    if ( p -> buffs_bear_form -> check() ) p -> buffs_bear_form -> expire();
    if ( p -> buffs_cat_form  -> check() ) p -> buffs_cat_form  -> expire();

    p -> buffs_moonkin_form -> start();

    sim -> auras.moonkin -> trigger();

    if ( p -> talents.improved_moonkin_form )
    {
      sim -> auras.improved_moonkin -> trigger();
    }

    if ( p -> talents.furor )
    {
      p -> attribute_multiplier[ ATTR_INTELLECT ] *= 1.0 + p -> talents.furor * 0.02;
    }

    p -> spell_power_per_spirit += ( p -> talents.improved_moonkin_form * 0.10 );

    p -> armor_multiplier += 3.7;
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return ! d -> buffs_moonkin_form -> check();
  }
};

// Natures Swiftness Spell ==================================================

struct druids_swiftness_t : public druid_spell_t
{
  cooldown_t* sub_cooldown;
  dot_t*      sub_dot;

  druids_swiftness_t( player_t* player, const std::string& options_str ) :
    druid_spell_t( "natures_swiftness", player, SCHOOL_NATURE, TREE_RESTORATION ),
    sub_cooldown(0), sub_dot(0)
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.natures_swiftness );
    trigger_gcd = 0;
    cooldown -> duration = 180.0;
    if ( ! options_str.empty() )
    {
      // This will prevent Natures Swiftness from being called before the desired "fast spell" is ready to be cast.
      sub_cooldown = p -> get_cooldown( options_str );
      sub_dot      = p -> get_dot     ( options_str );
    }
    id = 17116;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    druid_t* p = player -> cast_druid();
    p -> buffs_natures_swiftness -> trigger();
    update_ready();
  }

  virtual bool ready()
  {
    if ( sub_cooldown )
      if ( sub_cooldown -> remains() > 0 )
        return false;

    if ( sub_dot )
      if ( sub_dot -> remains() > 0 )
        return false;

    return druid_spell_t::ready();
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  int eclipse_benefit;
  int eclipse_trigger;
  std::string prev_str;
  int extend_moonfire;
  int instant;

  starfire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "starfire", player, SCHOOL_ARCANE, TREE_BALANCE ),
      eclipse_benefit( 0 ), eclipse_trigger( 0 ), extend_moonfire( 0 ), instant( 0 )
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
    {
      { "extendmf", OPT_BOOL,   &extend_moonfire },
      { "eclipse",  OPT_STRING, &eclipse_str     },
      { "prev",     OPT_STRING, &prev_str        },
      { "instant",  OPT_BOOL,   &instant         },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( ! eclipse_str.empty() )
    {
      eclipse_benefit = ( eclipse_str == "benefit" );
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    // Starfire is leanred at level 78, but gains 5 damage per level
    // so the actual damage range at 80 is: 1038 to 1222
    static rank_t ranks[] =
    {
      { 80, 11, 1038, 1222, 0, 0.16 },
      { 78, 10, 1028, 1212, 0, 0.16 },
      { 72,  9,  854, 1006, 0, 0.16 },
      { 67,  8,  818,  964, 0, 370  },
      { 60,  7,  693,  817, 0, 340  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48465 );

    base_execute_time = 3.5;
    direct_power_mod  = ( base_execute_time / 3.5 );
    may_crit          = true;

    base_cost         *= 1.0 - util_t::talent_rank( p -> talents.moonglow, 3, 0.03 );
    base_execute_time -= util_t::talent_rank( p -> talents.starlight_wrath, 5, 0.1 );
    base_multiplier   *= 1.0 + util_t::talent_rank( p -> talents.moonfury, 3, 0.03, 0.06, 0.10 );
    base_crit         += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
    direct_power_mod  += util_t::talent_rank( p -> talents.wrath_of_cenarius, 5, 0.04 );
    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

    if ( p -> idols.shooting_star )
    {
      // Equip: Increases the damage dealt by Starfire by 165.
      base_dd_min += 165;
      base_dd_max += 165;
    }
    if ( p -> set_bonus.tier6_4pc_caster() ) base_crit += 0.05;
    if ( p -> set_bonus.tier7_4pc_caster() ) base_crit += 0.05;
    if ( p -> set_bonus.tier9_4pc_caster() ) base_multiplier *= 1.04;
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();

    if( p -> buffs_eclipse_lunar -> up() )
    {
      player_crit += 0.40 + p -> set_bonus.tier8_2pc_caster() * 0.07;
    }

    if ( p -> dots_moonfire -> ticking() )
    {
      player_crit += 0.01 * p -> talents.improved_insect_swarm;
    }
  }

  virtual void schedule_execute()
  {
    druid_spell_t::schedule_execute();
    druid_t* p = player -> cast_druid();
    p -> buffs_t8_4pc_caster -> expire();
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::execute();

    if ( result_is_hit() )
    {
      trigger_earth_and_moon( this );

      if ( result == RESULT_CRIT )
      {
        trigger_t10_4pc_caster( player, direct_dmg, school );

        if ( ! p -> buffs_eclipse_lunar -> check() )
        {
          if ( p -> buffs_eclipse_solar -> trigger() )
            p -> buffs_eclipse_lunar -> cooldown_ready = sim -> current_time + 15;
        }
      }
      if ( p -> glyphs.starfire )
      {
        if( p -> dots_moonfire -> ticking() )
          if ( p -> dots_moonfire -> action -> added_ticks < 3 )
            p -> dots_moonfire -> action -> extend_duration( 1 );
      }
    }
  }
  virtual double execute_time() SC_CONST
  {
    druid_t* p = player -> cast_druid();
    if ( p -> buffs_t8_4pc_caster -> check() )
      return 0;

    return druid_spell_t::execute_time();
  }
  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( instant )
      if ( ! p -> buffs_t8_4pc_caster -> may_react() )
        return false;

    if ( extend_moonfire )
    {
      if ( ! p -> glyphs.starfire ) return false;
      if ( ! p -> dots_moonfire -> ticking() ) return false;
      if ( p -> dots_moonfire -> action -> added_ticks > 2 ) return false;
    }

    if ( eclipse_benefit )
    {
      if ( ! p -> buffs_eclipse_lunar -> may_react() )
        return false;

      // Don't cast starfire if eclipse will fade before the cast finished
      if ( p -> buffs_eclipse_lunar -> remains_lt( execute_time() ) )
        return false;
    }

    if ( eclipse_trigger )
      if ( ! may_trigger_solar_eclipse( p ) )
        return false;

    if ( ! prev_str.empty() )
    {
      if ( ! p -> last_foreground_action )
        return false;

      if ( p -> last_foreground_action -> name_str != prev_str )
        return false;
    }

    return true;
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  int eclipse_benefit;
  int eclipse_trigger;
  std::string prev_str;
  double moonfury_bonus;

  wrath_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "wrath", player, SCHOOL_NATURE, TREE_BALANCE ), eclipse_benefit( 0 ), eclipse_trigger( 0 ), moonfury_bonus( 0.0 )
  {
    druid_t* p = player -> cast_druid();

    travel_speed = 21.0;

    std::string eclipse_str;
    option_t options[] =
    {
      { "eclipse", OPT_STRING, &eclipse_str },
      { "prev",    OPT_STRING, &prev_str    },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( ! eclipse_str.empty() )
    {
      eclipse_benefit = ( eclipse_str == "benefit" );
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    // Wrath is leanred at level 79, but gains 4 damage per level
    // so the actual damage range at 80 is: 557 to 627
    static rank_t ranks[] =
    {
      { 80, 13, 557, 627, 0, 0.11 },
      { 79, 12, 553, 623, 0, 0.11 },
      { 74, 11, 504, 568, 0, 0.11 },
      { 69, 10, 431, 485, 0, 255  },
      { 61,  9, 397, 447, 0, 210  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48461 );

    base_execute_time = 2.0;
    direct_power_mod  = ( base_execute_time / 3.5 );
    may_crit          = true;

    base_cost         *= 1.0 - util_t::talent_rank( p -> talents.moonglow, 3, 0.03 );
    base_execute_time -= util_t::talent_rank( p -> talents.starlight_wrath, 5, 0.1 );
    base_crit         += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
    direct_power_mod  += util_t::talent_rank( p -> talents.wrath_of_cenarius, 5, 0.02 );
    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

    // The % bonus from eclipse and moonfury are additive, so have to sum them up in player_buff()
    moonfury_bonus = util_t::talent_rank( p -> talents.moonfury, 3, 0.03, 0.06, 0.10 );

    if ( p -> set_bonus.tier7_4pc_caster() ) base_crit += 0.05;
    if ( p -> set_bonus.tier9_4pc_caster() ) base_multiplier   *= 1.04;

    if ( p -> idols.steadfast_renewal )
    {
      // Equip: Increases the damage dealt by Wrath by 70.
      base_dd_min += 70;
      base_dd_max += 70;
    }
  }

  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::travel( travel_result, travel_dmg );
    if (travel_result == RESULT_CRIT || travel_result == RESULT_HIT )
    {
      if ( travel_result == RESULT_CRIT )
      {
        trigger_t10_4pc_caster( player, travel_dmg, SCHOOL_NATURE );
        if( ! p -> buffs_eclipse_solar -> check() )
        {
          if ( p -> buffs_eclipse_lunar -> trigger() )
            p -> buffs_eclipse_solar -> cooldown_ready = sim -> current_time + 15;
        }
      }
      trigger_earth_and_moon( this );
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::player_buff();

    // Eclipse and Moonfury being additive has to be handled here
    double bonus = moonfury_bonus;

    if( p -> buffs_eclipse_solar -> up() )
    {
      bonus += 0.40 + p -> set_bonus.tier8_2pc_caster() * 0.07;
    }

    player_multiplier *= 1.0 + bonus;

    if ( p -> dots_insect_swarm -> ticking() )
    {
      player_multiplier *= 1.0 + p -> talents.improved_insect_swarm * 0.01;
    }
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( eclipse_benefit )
    {
      if ( ! p -> buffs_eclipse_solar -> may_react() )
        return false;

      // Don't cast wrath if eclipse will fade before the cast finished.
      if ( p -> buffs_eclipse_solar -> remains_lt( execute_time() ) )
        return false;
    }

    if ( eclipse_trigger )
      if ( ! may_trigger_lunar_eclipse( p ) )
        return false;

    if ( ! prev_str.empty() )
    {
      if ( ! p -> last_foreground_action )
        return false;

      if ( p -> last_foreground_action -> name_str != prev_str )
        return false;
    }

    return true;
  }
};

// Starfall Spell ===========================================================

struct starfall_t : public druid_spell_t
{
  spell_t* starfall_star;

  starfall_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "starfall", player, SCHOOL_ARCANE, TREE_BALANCE )
  {
    struct starfall_star_splash_t : public druid_spell_t
    {
      starfall_star_splash_t( player_t* player ) : druid_spell_t( "starfall", player, SCHOOL_ARCANE, TREE_BALANCE )
      {
        druid_t* p = player -> cast_druid();

        static rank_t ranks[] =
        {
          { 80, 4, 101, 101, 0, 0 },
          { 75, 3,  84,  85, 0, 0 },
          { 70, 2,  57,  58, 0, 0 },
          { 60, 1,  25,  26, 0, 0 },
          {  0, 0,   0,   0, 0, 0 }
        };

        init_rank( ranks );
        direct_power_mod  = 0.13;

        may_crit          = true;
        may_miss          = true;
        may_resist        = true;
        background        = true;
        aoe               = true; // Prevents Moonkin Form mana gains.
        dual              = true;

        base_crit                  += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
        base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );
        if ( p -> glyphs.focus ) base_multiplier *= 1.1;
        id = 53190;
      }
      virtual void execute()
      {
        druid_spell_t::execute();
        tick_dmg = direct_dmg;
        update_stats( DMG_OVER_TIME );
      }
    };

    struct starfall_star_t : public druid_spell_t
    {
      action_t* starfall_star_splash;
      starfall_star_t( player_t* player ) : druid_spell_t( "starfall", player, SCHOOL_ARCANE, TREE_BALANCE )
      {
        druid_t* p = player -> cast_druid();

        direct_power_mod = 0.30;
        may_crit         = true;
        may_miss         = true;
        may_resist       = true;
        background       = true;
        proc             = true;
        dual             = true;

        base_dd_min = base_dd_max  = 0;
        base_crit                  += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
        base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

        if ( p -> glyphs.focus )
          base_multiplier *= 1.2;

        starfall_star_splash = new starfall_star_splash_t( p );

        id = 53195;
      }

      virtual void execute()
      {
        druid_spell_t::execute();
        tick_dmg = direct_dmg;
        update_stats( DMG_OVER_TIME );

        if ( result_is_hit() )
        {
          // FIXME! Just an assumption that the splash damage only occurs if the star did not miss. (
          starfall_star_splash -> execute();
        }
      }
    };
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4, 563, 653, 0, 0.35 },
      { 75, 3, 474, 551, 0, 0.35 },
      { 70, 2, 324, 377, 0, 0.35 },
      { 60, 1, 114, 167, 0, 0.35 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    num_ticks      = 10;
    base_tick_time = 1.0;

    cooldown -> duration          = 90;
    if ( p -> glyphs.starfall )
      cooldown -> duration  -= 30;

    base_execute_time = 0;

    may_miss = may_crit = false; // The spell only triggers the buff



    starfall_star = new starfall_star_t( p );
    starfall_star -> base_dd_max = base_dd_max;
    starfall_star -> base_dd_min = base_dd_min;
    base_dd_min = base_dd_max = 0;

  }
  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    starfall_star -> execute();
    update_time( DMG_OVER_TIME );
  }
};

// Typhoon Spell ============================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "typhoon", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.typhoon );
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 1190, 1190, 0, 0.25 },
      { 75, 4, 1010, 1010, 0, 0.25 },
      { 70, 3,  735,  735, 0, 0.25 },
      { 60, 2,  550,  550, 0, 0.25 },
      { 50, 1,  400,  400, 0, 0.25 },
      {  0, 0,    0,    0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    direct_power_mod  = 0.193;
    base_multiplier *= 1.0 + 0.15 * p -> talents.gale_winds;
    aoe = true;

    cooldown -> duration = 20;
    if ( p -> glyphs.monsoon )
      cooldown -> duration -= 3;
    if ( p -> glyphs.typhoon )
      base_cost *= 0.92;
  }
};

// Mark of the Wild Spell =====================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  double bonus;

  mark_of_the_wild_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "mark_of_the_wild", player, SCHOOL_NATURE, TREE_RESTORATION ), bonus( 0 )
  {
    druid_t* p = player -> cast_druid();

    trigger_gcd = 0;
    bonus  = util_t::ability_rank( player -> level,  37.0,80, 14.0,70,  12.0,0 );
    bonus *= 1.0 + p -> talents.improved_mark_of_the_wild * 0.20;
    id = 48469;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> ooc_buffs() )
      {
        p -> buffs.mark_of_the_wild -> trigger( 1, bonus );
        p -> init_resources( true );
      }
    }
  }

  virtual bool ready()
  {
    return( player -> buffs.mark_of_the_wild -> current_value < bonus );
  }
};

// Treants Spell ============================================================

struct treants_spell_t : public druid_spell_t
{
  treants_spell_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "treants", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talents.force_of_nature );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration = 180.0;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.12;
    id = 33831;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "treants", 30.0 );
  }
};

// Stealth ==================================================================

struct stealth_t : public spell_t
{
  stealth_t( player_t* player, const std::string& options_str ) :
      spell_t( "stealth", player )
  {
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_stealthed -> trigger();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    return ! p -> buffs_stealthed -> check();
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack"       ) return new       auto_attack_t( this, options_str );
  if ( name == "bash"              ) return new              bash_t( this, options_str );
  if ( name == "berserk_bear"      ) return new      berserk_bear_t( this, options_str );
  if ( name == "berserk_cat"       ) return new       berserk_cat_t( this, options_str );
  if ( name == "bear_form"         ) return new         bear_form_t( this, options_str );
  if ( name == "cat_form"          ) return new          cat_form_t( this, options_str );
  if ( name == "claw"              ) return new              claw_t( this, options_str );
  if ( name == "enrage"            ) return new            enrage_t( this, options_str );
  if ( name == "faerie_fire"       ) return new       faerie_fire_t( this, options_str );
  if ( name == "faerie_fire_feral" ) return new faerie_fire_feral_t( this, options_str );
  if ( name == "ferocious_bite"    ) return new    ferocious_bite_t( this, options_str );
  if ( name == "insect_swarm"      ) return new      insect_swarm_t( this, options_str );
  if ( name == "innervate"         ) return new         innervate_t( this, options_str );
  if ( name == "lacerate"          ) return new          lacerate_t( this, options_str );
  if ( name == "maim"              ) return new              maim_t( this, options_str );
  if ( name == "mangle_bear"       ) return new       mangle_bear_t( this, options_str );
  if ( name == "mangle_cat"        ) return new        mangle_cat_t( this, options_str );
  if ( name == "mark_of_the_wild"  ) return new  mark_of_the_wild_t( this, options_str );
  if ( name == "maul"              ) return new              maul_t( this, options_str );
  if ( name == "moonfire"          ) return new          moonfire_t( this, options_str );
  if ( name == "moonkin_form"      ) return new      moonkin_form_t( this, options_str );
  if ( name == "natures_swiftness" ) return new  druids_swiftness_t( this, options_str );
  if ( name == "rake"              ) return new              rake_t( this, options_str );
  if ( name == "rip"               ) return new               rip_t( this, options_str );
  if ( name == "savage_roar"       ) return new       savage_roar_t( this, options_str );
  if ( name == "shred"             ) return new             shred_t( this, options_str );
  if ( name == "starfire"          ) return new          starfire_t( this, options_str );
  if ( name == "starfall"          ) return new          starfall_t( this, options_str );
  if ( name == "stealth"           ) return new           stealth_t( this, options_str );
  if ( name == "swipe_bear"        ) return new        swipe_bear_t( this, options_str );
  if ( name == "tigers_fury"       ) return new       tigers_fury_t( this, options_str );
  if ( name == "treants"           ) return new     treants_spell_t( this, options_str );
  if ( name == "typhoon"           ) return new           typhoon_t( this, options_str );
  if ( name == "wrath"             ) return new             wrath_t( this, options_str );
#if 0
  if ( name == "cower"             ) return new             cower_t( this, options_str );
  if ( name == "maim"              ) return new              maim_t( this, options_str );
  if ( name == "prowl"             ) return new             prowl_t( this, options_str );
  if ( name == "ravage"            ) return new            ravage_t( this, options_str );
  if ( name == "swipe_cat"         ) return new         swipe_cat_t( this, options_str );
#endif

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "treants" ) return new treants_pet_t( sim, this, pet_name );

  return 0;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  create_pet( "treants" );
}

// druid_t::init_rating =====================================================

void druid_t::init_rating()
{
  player_t::init_rating();

  rating.attack_haste *= 1.0 / 1.30;
}

// druid_t::init_glyphs =====================================================

void druid_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "berserk"      ) glyphs.berserk = 1;
    else if ( n == "focus"        ) glyphs.focus = 1;
    else if ( n == "innervate"    ) glyphs.innervate = 1;
    else if ( n == "insect_swarm" ) glyphs.insect_swarm = 1;
    else if ( n == "mangle"       ) glyphs.mangle = 1;
    else if ( n == "moonfire"     ) glyphs.moonfire = 1;
    else if ( n == "monsoon"      ) glyphs.monsoon = 1;
    else if ( n == "rip"          ) glyphs.rip = 1;
    else if ( n == "savage_roar"  ) glyphs.savage_roar = 1;
    else if ( n == "shred"        ) glyphs.shred = 1;
    else if ( n == "starfire"     ) glyphs.starfire = 1;
    else if ( n == "starfall"     ) glyphs.starfall = 1;
    else if ( n == "typhoon"      ) glyphs.typhoon = 1;
    // minor glyphs, to prevent 'not-found' warning
    else if ( n == "aquatic_form"          ) ;
    else if ( n == "barkskin"              ) ;
    else if ( n == "challenging_roar"      ) ;
    else if ( n == "dash"                  ) ;
    else if ( n == "entangling_roots"      ) ;
    else if ( n == "frenzied_regeneration" ) ;
    else if ( n == "growling"              ) ;
    else if ( n == "healing_touch"         ) ;
    else if ( n == "hurricane"             ) ;
    else if ( n == "lifebloom"             ) ;
    else if ( n == "maul"                  ) ;
    else if ( n == "nourish"               ) ;
    else if ( n == "rapid_rejuvenation"    ) ;
    else if ( n == "rebirth"               ) ;
    else if ( n == "rejuvenation"          ) ;
    else if ( n == "regrowth"              ) ;
    else if ( n == "survival_instincts"    ) ;
    else if ( n == "swiftmend"             ) ;
    else if ( n == "the_wild"              ) ;
    else if ( n == "thorns"                ) ;
    else if ( n == "unburdened_rebirth"    ) ;
    else if ( n == "wild_growth"           ) ;
    else if ( n == "wrath"                 ) ;
    else if ( ! sim -> parent )
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// druid_t::init_race ======================================================

void druid_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_NIGHT_ELF:
  case RACE_TAUREN:
    break;
  default:
    race = RACE_NIGHT_ELF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// druid_t::init_base =======================================================

void druid_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, level, DRUID, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  base_spell_crit += talents.natural_perfection * 0.01;

  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute_multiplier_initial[ i ] *= 1.0 + 0.02 * talents.survival_of_the_fittest;
    attribute_multiplier_initial[ i ] *= 1.0 + 0.01 * talents.improved_mark_of_the_wild;
  }

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + 0.04 * talents.heart_of_the_wild;

  initial_spell_power_per_intellect = talents.lunar_guidance * 0.04;
  initial_spell_power_per_spirit = 0.0;

  base_attack_power = -20 + level * 2.0;
  base_attack_expertise = talents.primal_precision * 0.05;

  initial_attack_power_per_agility  = 0.0;
  initial_attack_power_per_strength = 2.0;

  // FIXME! Level-specific!  Should be form-specific!
  base_defense = level * 5;
  base_miss    = 0.05;
  base_dodge   = 0.0560970;
  initial_armor_multiplier  = 1.0 + util_t::talent_rank( talents.thick_hide, 3, 0.04, 0.07, 0.10 );
  initial_dodge_per_agility = 0.0002090;
  initial_armor_per_agility = 2.0;

  diminished_kfactor    = 0.9560;
  diminished_miss_capi  = 1.0 / 0.16;
  diminished_dodge_capi = 1.0 / 0.88129021;
  diminished_parry_capi = 1.0 / 0.47003525;

  resource_base[ RESOURCE_ENERGY ] = 100;
  resource_base[ RESOURCE_RAGE   ] = 100;

  health_per_stamina      = 10;
  mana_per_intellect      = 15;
  energy_regen_per_second = 10;

  mana_regen_while_casting = util_t::talent_rank( talents.intensity,  3, 0.17, 0.33, 0.50 );

  mp5_per_intellect = util_t::talent_rank( talents.dreamstate, 3, 0.04, 0.07, 0.10 );

  base_gcd = 1.5;

  if ( tank == -1 && talents.is_tank_spec() ) tank = 1;
}

// druid_t::init_buffs ======================================================

void druid_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_berserk            = new buff_t( this, "berserk"           , 1,  15.0 + ( glyphs.berserk ? 5.0 : 0.0 ) );
  buffs_eclipse_lunar      = new buff_t( this, "lunar_eclipse"     , 1,  15.0,  30.0, talents.eclipse / 5.0 );
  buffs_eclipse_solar      = new buff_t( this, "solar_eclipse"     , 1,  15.0,  30.0, talents.eclipse / 3.0 );
  buffs_enrage             = new buff_t( this, "enrage"            , 1,  10.0 );
  buffs_lacerate           = new buff_t( this, "lacerate"          , 5,  15.0 );
  buffs_natures_grace      = new buff_t( this, "natures_grace"     , 1,   3.0,     0, talents.natures_grace / 3.0 );
  buffs_natures_swiftness  = new buff_t( this, "natures_swiftness" , 1, 180.0, 180.0 );
  buffs_omen_of_clarity    = new buff_t( this, "omen_of_clarity"   , 1,  15.0,     0, talents.omen_of_clarity * 3.5 / 60.0 );
  buffs_t8_4pc_caster      = new buff_t( this, "t8_4pc_caster"     , 1,  10.0,     0, set_bonus.tier8_4pc_caster() * 0.08 );
  buffs_t10_2pc_caster     = new buff_t( this, "t10_2pc_caster"    , 1,   6.0,     0, set_bonus.tier10_2pc_caster() );

  buffs_tigers_fury        = new buff_t( this, "tigers_fury"       , 1,   6.0 );
  buffs_glyph_of_innervate = new buff_t( this, "glyph_of_innervate", 1,  10.0,     0, glyphs.innervate);

  // stat_buff_t( sim, player, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_lunar_fury  = new stat_buff_t( this, "idol_lunary_fury",  STAT_CRIT_RATING, 200, 1, 12.0,     0, idols.lunar_fury * 0.70  );
  buffs_mutilation  = new stat_buff_t( this, "idol_mutilation",   STAT_AGILITY,     200, 1, 16.0,     0, idols.mutilation * 0.70  );
  buffs_corruptor   = new stat_buff_t( this, "idol_primal_wrath", STAT_AGILITY,     153, 1, 12.0,     0, idols.corruptor          ); // 100% chance!
  buffs_terror      = new stat_buff_t( this, "idol_terror",       STAT_AGILITY,      65, 1, 10.0, 10.01, idols.terror * 0.85      );
  buffs_unseen_moon = new stat_buff_t( this, "idol_unseen_moon",  STAT_SPELL_POWER, 140, 1, 10.0,     0, idols.unseen_moon * 0.50 );
  // PTR Idols
  buffs_t10_feral_relic   = new stat_buff_t( this, "idol_crying_moon",   STAT_AGILITY,     44, 5, 15.0, 0, idols.crying_moon   );
  buffs_t10_balance_relic = new stat_buff_t( this, "idol_lunar_eclipse", STAT_CRIT_RATING, 44, 5, 15.0, 0, idols.lunar_eclipse );

  // simple
  buffs_bear_form    = new buff_t( this, "bear_form" );
  buffs_cat_form     = new buff_t( this, "cat_form" );
  buffs_combo_points = new buff_t( this, "combo_points", 5 );
  buffs_moonkin_form = new buff_t( this, "moonkin_form" );
  buffs_savage_roar  = new buff_t( this, "savage_roar" );
  buffs_stealthed    = new buff_t( this, "stealthed" );
}

// druid_t::init_items ======================================================

void druid_t::init_items()
{
  player_t::init_items();

  std::string& idol = items[ SLOT_RANGED ].encoded_name_str;

  if ( idol.empty() ) return;

  if      ( idol == "idol_of_lunar_fury"         ) idols.lunar_fury = 1;
  else if ( idol == "idol_of_steadfast_renewal"  ) idols.steadfast_renewal = 1;
  else if ( idol == "idol_of_terror"             ) idols.terror = 1;
  else if ( idol == "idol_of_the_corruptor"      ) idols.corruptor = 1;
  else if ( idol == "idol_of_the_crying_wind"    ) idols.crying_wind = 1;
  else if ( idol == "idol_of_mutilation"         ) idols.mutilation = 1;
  else if ( idol == "idol_of_the_raven_goddess"  ) idols.raven_goddess = 1;
  else if ( idol == "idol_of_the_ravenous_beast" ) idols.ravenous_beast = 1;
  else if ( idol == "idol_of_the_shooting_star"  ) idols.shooting_star = 1;
  else if ( idol == "idol_of_the_unseen_moon"    ) idols.unseen_moon = 1;
  else if ( idol == "idol_of_worship"            ) idols.worship = 1;
  else if ( idol == "idol_of_the_crying_moon"    ) idols.crying_moon = 1;
  else if ( idol == "idol_of_the_lunar_eclipse"  ) idols.lunar_eclipse = 1;
  // To prevent warnings....
  else if ( idol == "idol_of_awakening"            ) ;
  else if ( idol == "idol_of_flaring_growth"       ) ;
  else if ( idol == "idol_of_lush_moss"            ) ;
  else if ( idol == "idol_of_the_black_willow"     ) ;
  else if ( idol == "idol_of_the_flourishing_life" ) ;
  else if ( idol == "harolds_rejuvenating_broach"  ) ;
  else
  {
    sim -> errorf( "Player %s has unknown idol %s", name(), idol.c_str() );
  }

  if ( idols.raven_goddess ) gear.add_stat( STAT_CRIT_RATING, 40 );
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time;

  scales_with[ STAT_WEAPON_SPEED  ] = 0;
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  gains_bear_melee         = get_gain( "bear_melee"         );
  gains_energy_refund      = get_gain( "energy_refund"      );
  gains_enrage             = get_gain( "enrage"             );
  gains_glyph_of_innervate = get_gain( "glyph_of_innervate" );
  gains_incoming_damage    = get_gain( "incoming_damage"    );
  gains_moonkin_form       = get_gain( "moonkin_form"       );
  gains_natural_reaction   = get_gain( "natural_reaction"   );
  gains_omen_of_clarity    = get_gain( "omen_of_clarity"    );
  gains_primal_fury        = get_gain( "primal_fury"        );
  gains_primal_precision   = get_gain( "primal_precision"   );
  gains_tigers_fury        = get_gain( "tigers_fury"        );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  procs_combo_points_wasted = get_proc( "combo_points_wasted" );
  procs_parry_haste         = get_proc( "parry_haste"         );
  procs_primal_fury         = get_proc( "primal_fury"         );
  procs_tier8_2pc           = get_proc( "tier8_2pc"           );
}

// druid_t::init_uptimes ====================================================

void druid_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_energy_cap = get_uptime( "energy_cap" );
  uptimes_rage_cap   = get_uptime( "rage_cap"   );
  uptimes_rip        = get_uptime( "rip"        );
  uptimes_rake       = get_uptime( "rake"       );
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  player_t::init_rng();

  rng_primal_fury = get_rng( "primal_fury" );
}

// druid_t::init_actions ====================================================

void druid_t::init_actions()
{
  if ( primary_role() == ROLE_ATTACK && main_hand_weapon.type == WEAPON_NONE )
  {
    sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }
  if ( primary_tree() == TREE_RESTORATION )
  {
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    std::string use_str = "";
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        use_str += "/use_item,name=";
        use_str += items[ i ].name();
      }
    }

    if ( primary_tree() == TREE_FERAL )
    {
      if ( tank > 0 )
      {
        action_list_str += "flask,type=endless_rage/food,type=rhinolicious_wormsteak";
        action_list_str += "/bear_form";
        action_list_str += "/auto_attack";
        action_list_str += "/snapshot_stats";
        action_list_str += "/faerie_fire_feral,debuff_only=1";  // Use on pull.
        if ( talents.mangle )  action_list_str += "/mangle_bear,mangle<=0.5";
        action_list_str += "/lacerate,lacerate<=6.9";           // This seems to be the sweet spot to prevent Lacerate falling off.
        if ( talents.berserk ) action_list_str+="/berserk_bear";
        action_list_str += use_str;
        if ( talents.mangle )  action_list_str += "/mangle_bear";
        action_list_str += "/faerie_fire_feral";
        action_list_str += "/swipe_bear,berserk=0,lacerate_stack>=5";
      }
      else
      {
        action_list_str += "flask,type=endless_rage";
        action_list_str += "/food,type=hearty_rhino";
        action_list_str += "/cat_form";
        action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
        action_list_str += "/auto_attack";
        action_list_str += "/snapshot_stats";
        action_list_str += "/maim";
        action_list_str += "/faerie_fire_feral,debuff_only=1";
        action_list_str += "/tigers_fury,if=energy<=30&!buff.berserk.up";
        if ( talents.berserk )action_list_str += "/berserk_cat,if=energy>=80&energy<=90&!buff.tigers_fury.up";
        action_list_str += "/savage_roar,if=buff.combo_points.stack>=1&buff.savage_roar.remains<=1";
        action_list_str += "/rip,if=buff.combo_points.stack>=5&target.time_to_die>=6";
        action_list_str += "/savage_roar,if=buff.combo_points.stack>=3&target.time_to_die>=9&buff.savage_roar.remains<=8&dot.rip.remains-buff.savage_roar.remains>=-3";
        action_list_str += "/ferocious_bite,if=target.time_to_die<=6&buff.combo_points.stack>=5";
        action_list_str += "/ferocious_bite,if=target.time_to_die<=1&buff.combo_points.stack>=4";
        action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.remains>=8&buff.savage_roar.remains>=11";
        if ( glyphs.shred )action_list_str += "/shred,extend_rip=1,if=dot.rip.remains<=4";
        if ( talents.mangle ) action_list_str += "/mangle_cat,mangle<=1";
        action_list_str += "/rake,if=target.time_to_die>=9";
        action_list_str += "/shred,if=(buff.combo_points.stack<=4|dot.rip.remains>=0.8)&dot.rake.remains>=0.4&(energy>=80|buff.omen_of_clarity.react|dot.rip.remains<=2|buff.berserk.up|cooldown.tigers_fury.remains<=3)";
        action_list_str += "/shred,if=target.time_to_die<=9";
        action_list_str += "/shred,if=buff.combo_points.stack<=0&buff.savage_roar.remains<=2";
      }
    }
    else if ( primary_tree() == TREE_BALANCE )
    {
      action_list_str += "flask,type=frost_wyrm/food,type=fish_feast/mark_of_the_wild";
      if ( talents.moonkin_form ) action_list_str += "/moonkin_form";
      action_list_str += "/snapshot_stats";
      action_list_str += "/speed_potion,if=!in_combat|(buff.bloodlust.react&buff.lunar_eclipse.react)|(target.time_to_die<=60&buff.lunar_eclipse.react)";
      if ( talents.improved_faerie_fire ) action_list_str += "/faerie_fire";
      action_list_str += "/moonfire,moving=1,if=!ticking";
      action_list_str += "/insect_swarm,moving=1,if=!ticking";
      if ( talents.typhoon ) action_list_str += "/typhoon,moving=1";
      action_list_str += "/innervate,trigger=-2000";
      if ( talents.force_of_nature )
      {
        action_list_str+="/treants,time>=5";
      }
      if ( talents.starfall ) action_list_str+="/starfall,if=!eclipse";
      action_list_str += "/starfire,if=buff.t8_4pc_caster.up";
      action_list_str += "/moonfire,if=!ticking&!eclipse";
      if ( talents.insect_swarm && ( glyphs.insect_swarm )) action_list_str += "/insect_swarm,if=!ticking&!eclipse";
      action_list_str += "/wrath,if=trigger_lunar";
      action_list_str += use_str;
      action_list_str += "/starfire,if=buff.lunar_eclipse.react&(buff.lunar_eclipse.remains>cast_time)";
      action_list_str += "/wrath,if=buff.solar_eclipse.react&(buff.solar_eclipse.remains>cast_time)";
      action_list_str += "/starfire";
    }
    action_list_default = 1;
  }

  player_t::init_actions();

  num_active_mauls = ( int ) active_mauls.size();
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  base_gcd = 1.5;
}

// druid_t::interrupt =======================================================

void druid_t::interrupt()
{
  player_t::interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
}

// druid_t::clear_debuffs ===================================================

void druid_t::clear_debuffs()
{
  player_t::clear_debuffs();

  buffs_combo_points -> expire();
}

// druid_t::regen ===========================================================

void druid_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  int resource_type = primary_resource();

  if ( resource_type == RESOURCE_ENERGY )
  {
    uptimes_energy_cap -> update( resource_current[ RESOURCE_ENERGY ] ==
                                  resource_max    [ RESOURCE_ENERGY ] );
  }
  else if( resource_type == RESOURCE_MANA )
  {
    resource_gain( RESOURCE_MANA, buffs_glyph_of_innervate -> value() * periodicity, gains_glyph_of_innervate );
  }
  else if ( resource_type == RESOURCE_RAGE )
  {
    if ( buffs_enrage -> up() ) resource_gain( RESOURCE_RAGE, 1.0 * periodicity, gains_enrage );

    uptimes_rage_cap -> update( resource_current[ RESOURCE_RAGE ] ==
                                resource_max    [ RESOURCE_RAGE ] );
  }
}

// druid_t::available =======================================================

double druid_t::available() SC_CONST
{
  if( primary_resource() != RESOURCE_ENERGY ) return 0.1;

  double energy = resource_current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return 0.1;

  return std::max( ( 25 - energy ) / energy_regen_per_second, 0.1 );
}

// druid_t::composite_attack_power ==========================================

double druid_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();

  if ( buffs_cat_form  -> check() ||
       buffs_bear_form -> check() )
  {
    double weapon_ap = ( equipped_weapon_dps - 54.8 ) * 14;

    if ( talents.predatory_strikes )
    {
      ap += level * talents.predatory_strikes * 0.5;
      weapon_ap *= 1 + util_t::talent_rank( talents.predatory_strikes, 3, 0.07, 0.14, 0.20 );
    }

    ap += weapon_ap;
  }

  if ( buffs_cat_form  -> check() ) ap += floor( 1.0 * agility() );

  return floor( ap );
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() SC_CONST
{
  double multiplier = player_t::composite_attack_power_multiplier();

  if ( buffs_cat_form -> check() )
  {
    multiplier *= 1.0 + talents.heart_of_the_wild * 0.02;
  }
  else if ( buffs_bear_form -> check() )
  {
    multiplier *= 1.0 + 0.02 * talents.protector_of_the_pack;
  }

  return multiplier;
}

// druid_t::composite_attack_crit ==========================================

double druid_t::composite_attack_crit() SC_CONST
{
  double c = player_t::composite_attack_crit();

  if ( buffs_cat_form -> check() )
  {
    c += 0.02 * talents.sharpened_claws;
    c += 0.02 * talents.master_shapeshifter;
  }
  else if ( buffs_bear_form -> check() )
  {
    c += 0.02 * talents.sharpened_claws;
  }

  return floor( c * 10000.0 ) / 10000.0;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit() SC_CONST
{
  double hit = player_t::composite_spell_hit();

  hit += talents.balance_of_power * 0.02;

  return floor( hit * 10000.0 ) / 10000.0;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit() SC_CONST
{
  double crit = player_t::composite_spell_crit();

  return floor( crit * 10000.0 ) / 10000.0;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA )
    if ( buffs_bear_form -> check() )
      if ( talents.heart_of_the_wild )
        m *= 1.0 + 0.02 * talents.heart_of_the_wild;

  return m;
}

// druid_t::composite_tank_crit =============================================

double druid_t::composite_tank_crit( int school ) SC_CONST
{
  double c = player_t::composite_tank_crit( school );

  if ( school == SCHOOL_PHYSICAL )
  {
    if ( buffs_bear_form -> check() )
    {
      c -= 0.02 * talents.survival_of_the_fittest;
      if ( c < 0 ) c = 0;
    }
  }

  return c;
}

// druid_t::create_expression ===============================================

action_expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "eclipse" )
  {
    struct eclipse_expr_t : public action_expr_t
    {
      buff_t* solar_eclipse;
      buff_t* lunar_eclipse;
      eclipse_expr_t( action_t* a, buff_t* s, buff_t* l ) : action_expr_t( a, "eclipse" ), solar_eclipse(s), lunar_eclipse(l) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = ( solar_eclipse -> may_react() || lunar_eclipse -> may_react() ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new eclipse_expr_t( a, buff_t::find( this, "solar_eclipse" ), buff_t::find( this, "lunar_eclipse" ) );
  }
  else if ( name_str == "trigger_lunar" )
  {
    struct trigger_lunar_expr_t : public action_expr_t
    {
      trigger_lunar_expr_t( action_t* a ) : action_expr_t( a, "trigger_lunar" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = may_trigger_lunar_eclipse( action -> player -> cast_druid() ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new trigger_lunar_expr_t( a );
  }
  else if ( name_str == "trigger_solar" )
  {
    struct trigger_solar_expr_t : public action_expr_t
    {
      trigger_solar_expr_t( action_t* a ) : action_expr_t( a, "trigger_solar" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = may_trigger_solar_eclipse( action -> player -> cast_druid() ) ? 1.0 : 0.0; return TOK_NUM; }
    };
    return new trigger_solar_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// druid_t::get_talent_trees ===============================================

std::vector<talent_translation_t>& druid_t::get_talent_list()
{
  if(talent_list.empty())
  {
    if(sim -> P400)
    {
      talent_translation_t translation_table[][MAX_TALENT_TREES] =
      {
        { {  1, 5, &( talents.starlight_wrath       ) }, {  1, 5, &( talents.ferocity                ) }, {  1, 2, &( talents.blessing_of_the_grove     ) } },
        { {  2, 5, &( talents.genesis               ) }, {  2, 5, &( talents.feral_aggression        ) }, {  2, 0, NULL                                   } },
        { {  3, 3, &( talents.moonglow              ) }, {  3, 3, &( talents.feral_instinct          ) }, {  3, 5, &( talents.furor                     ) } },
        { {  4, 2, &( talents.natures_majesty       ) }, {  4, 2, &( talents.savage_fury             ) }, {  4, 5, &( talents.naturalist                ) } },
        { {  5, 2, &( talents.improved_moonfire     ) }, {  5, 3, &( talents.thick_hide              ) }, {  5, 0, NULL                                   } },
        { {  6, 3, &( talents.celestial_focus       ) }, {  6, 2, &( talents.feral_swiftness         ) }, {  6, 0, NULL                                   } },
        { {  7, 3, &( talents.solar_beam            ) }, {  7, 1, &( talents.survival_instincts      ) }, {  7, 3, NULL                                   } },
        { {  8, 1, &( talents.natures_grace         ) }, {  8, 3, &( talents.sharpened_claws         ) }, {  8, 1, &( talents.omen_of_clarity           ) } },
        { {  9, 2, &( talents.natures_reach         ) }, {  9, 2, &( talents.shredding_attacks       ) }, {  9, 2, &( talents.master_shapeshifter       ) } },
        { { 10, 5, &( talents.natures_splendor      ) }, { 10, 3, &( talents.predatory_strikes       ) }, { 10, 0, NULL                                   } },
        { { 11, 3, &( talents.lunar_justice         ) }, { 11, 2, &( talents.primal_fury             ) }, { 11, 0, NULL                                   } },
        { { 12, 3, &( talents.brambles              ) }, { 12, 2, &( talents.primal_precision        ) }, { 12, 1, &( talents.natures_swiftness         ) } },
        { { 13, 1, &( talents.starsurge             ) }, { 13, 2, &( talents.brutal_impact           ) }, { 13, 0, NULL                                   } },
        { { 14, 3, &( talents.vengeance             ) }, { 14, 0, NULL                                 }, { 14, 0, NULL                                   } },
        { { 15, 3, &( talents.dreamstate            ) }, { 15, 0, NULL                                 }, { 15, 0, NULL                                   } },
        { { 16, 3, &( talents.lunar_guidance        ) }, { 16, 3, &( talents.natural_reaction        ) }, { 16, 0, NULL                                   } },
        { { 17, 2, &( talents.balance_of_power      ) }, { 17, 5, &( talents.heart_of_the_wild       ) }, { 17, 3, &( talents.living_spirit             ) } },
        { { 18, 1, &( talents.moonkin_form          ) }, { 18, 3, &( talents.survival_of_the_fittest ) }, { 18, 0, NULL                                   } },
        { { 19, 3, &( talents.improved_moonkin_form ) }, { 19, 1, &( talents.leader_of_the_pack      ) }, { 19, 3, &( talents.natural_perfection        ) } },
        { { 20, 3, &( talents.owlkin_frenzy         ) }, { 20, 0, NULL                                 }, { 20, 0, NULL                                   } },
        { { 21, 3, &( talents.wrath_of_cenarius     ) }, { 21, 0, NULL                                 }, { 21, 0, NULL                                   } },
        { { 22, 5, &( talents.eclipse               ) }, { 22, 3, &( talents.protector_of_the_pack   ) }, { 22, 0, NULL                                   } },
        { { 23, 3, &( talents.typhoon               ) }, { 23, 3, &( talents.predatory_instincts     ) }, { 23, 0, NULL                                   } },
        { { 24, 1, &( talents.force_of_nature       ) }, { 24, 3, &( talents.infected_wounds         ) }, { 24, 0, NULL                                   } },
        { { 25, 1, &( talents.gale_winds            ) }, { 25, 3, &( talents.king_of_the_jungle      ) }, { 25, 0, NULL                                   } },
        { { 26, 2, &( talents.earth_and_moon        ) }, { 26, 1, &( talents.mangle                  ) }, { 26, 0, NULL                                   } },
        { { 27, 3, &( talents.fungal_growth         ) }, { 27, 3, &( talents.improved_mangle         ) }, {  0, 0, NULL                                   } },
        { { 28, 1, &( talents.starfall              ) }, { 28, 5, &( talents.rend_and_tear           ) }, {  0, 0, NULL                                   } },
        { {  0, 0, NULL                               }, { 29, 1, &( talents.primal_gore             ) }, {  0, 0, NULL                                   } },
        { {  0, 0, NULL                               }, { 30, 1, &( talents.berserk                 ) }, {  0, 0, NULL                                   } },
        { {  0, 0, NULL                               }, {  0, 0, NULL                                 }, {  0, 0, NULL                                   } },
      };
      util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
    }
    else
    {
      talent_translation_t translation_table[][MAX_TALENT_TREES] =
      {
        { {  1, 5, &( talents.starlight_wrath       ) }, {  1, 5, &( talents.ferocity                ) }, {  1, 2, &( talents.improved_mark_of_the_wild ) } },
        { {  2, 5, &( talents.genesis               ) }, {  2, 5, &( talents.feral_aggression        ) }, {  2, 0, NULL                                   } },
        { {  3, 3, &( talents.moonglow              ) }, {  3, 3, &( talents.feral_instinct          ) }, {  3, 5, &( talents.furor                     ) } },
        { {  4, 2, &( talents.natures_majesty       ) }, {  4, 2, &( talents.savage_fury             ) }, {  4, 5, &( talents.naturalist                ) } },
        { {  5, 2, &( talents.improved_moonfire     ) }, {  5, 3, &( talents.thick_hide              ) }, {  5, 0, NULL                                   } },
        { {  6, 3, &( talents.brambles              ) }, {  6, 2, &( talents.feral_swiftness         ) }, {  6, 0, NULL                                   } },
        { {  7, 3, &( talents.natures_grace         ) }, {  7, 1, &( talents.survival_instincts      ) }, {  7, 3, &( talents.intensity                 ) } },
        { {  8, 1, &( talents.natures_splendor      ) }, {  8, 3, &( talents.sharpened_claws         ) }, {  8, 1, &( talents.omen_of_clarity           ) } },
        { {  9, 2, &( talents.natures_reach         ) }, {  9, 2, &( talents.shredding_attacks       ) }, {  9, 2, &( talents.master_shapeshifter       ) } },
        { { 10, 5, &( talents.vengeance             ) }, { 10, 3, &( talents.predatory_strikes       ) }, { 10, 0, NULL                                   } },
        { { 11, 3, &( talents.celestial_focus       ) }, { 11, 2, &( talents.primal_fury             ) }, { 11, 0, NULL                                   } },
        { { 12, 3, &( talents.lunar_guidance        ) }, { 12, 2, &( talents.primal_precision        ) }, { 12, 1, &( talents.natures_swiftness         ) } },
        { { 13, 1, &( talents.insect_swarm          ) }, { 13, 2, &( talents.brutal_impact           ) }, { 13, 0, NULL                                   } },
        { { 14, 3, &( talents.improved_insect_swarm ) }, { 14, 0, NULL                                 }, { 14, 0, NULL                                   } },
        { { 15, 3, &( talents.dreamstate            ) }, { 15, 0, NULL                                 }, { 15, 0, NULL                                   } },
        { { 16, 3, &( talents.moonfury              ) }, { 16, 3, &( talents.natural_reaction        ) }, { 16, 0, NULL                                   } },
        { { 17, 2, &( talents.balance_of_power      ) }, { 17, 5, &( talents.heart_of_the_wild       ) }, { 17, 3, &( talents.living_spirit             ) } },
        { { 18, 1, &( talents.moonkin_form          ) }, { 18, 3, &( talents.survival_of_the_fittest ) }, { 18, 0, NULL                                   } },
        { { 19, 3, &( talents.improved_moonkin_form ) }, { 19, 1, &( talents.leader_of_the_pack      ) }, { 19, 3, &( talents.natural_perfection        ) } },
        { { 20, 3, &( talents.improved_faerie_fire  ) }, { 20, 0, NULL                                 }, { 20, 0, NULL                                   } },
        { { 21, 3, &( talents.owlkin_frenzy         ) }, { 21, 0, NULL                                 }, { 21, 0, NULL                                   } },
        { { 22, 5, &( talents.wrath_of_cenarius     ) }, { 22, 3, &( talents.protector_of_the_pack   ) }, { 22, 0, NULL                                   } },
        { { 23, 3, &( talents.eclipse               ) }, { 23, 3, &( talents.predatory_instincts     ) }, { 23, 0, NULL                                   } },
        { { 24, 1, &( talents.typhoon               ) }, { 24, 3, &( talents.infected_wounds         ) }, { 24, 0, NULL                                   } },
        { { 25, 1, &( talents.force_of_nature       ) }, { 25, 3, &( talents.king_of_the_jungle      ) }, { 25, 0, NULL                                   } },
        { { 26, 2, &( talents.gale_winds            ) }, { 26, 1, &( talents.mangle                  ) }, { 26, 0, NULL                                   } },
        { { 27, 3, &( talents.earth_and_moon        ) }, { 27, 3, &( talents.improved_mangle         ) }, {  0, 0, NULL                                   } },
        { { 28, 1, &( talents.starfall              ) }, { 28, 5, &( talents.rend_and_tear           ) }, {  0, 0, NULL                                   } },
        { {  0, 0, NULL                               }, { 29, 1, &( talents.primal_gore             ) }, {  0, 0, NULL                                   } },
        { {  0, 0, NULL                               }, { 30, 1, &( talents.berserk                 ) }, {  0, 0, NULL                                   } },
        { {  0, 0, NULL                               }, {  0, 0, NULL                                 }, {  0, 0, NULL                                   } },
      };
    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
    }
  }
  return talent_list;}

// druid_t::get_options ================================================

std::vector<option_t>& druid_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t druid_options[] =
    {
      // @option_doc loc=player/druid/talents title="Talents"
      { "balance_of_power",          OPT_INT,  &( talents.balance_of_power          ) },
      { "berserk",                   OPT_INT,  &( talents.berserk                   ) },
      { "brambles",                  OPT_INT,  &( talents.brambles                  ) },
      { "brutal_impact",             OPT_INT,  &( talents.brutal_impact             ) },
      { "celestial_focus",           OPT_INT,  &( talents.celestial_focus           ) },
      { "dreamstate",                OPT_INT,  &( talents.dreamstate                ) },
      { "earth_and_moon",            OPT_INT,  &( talents.earth_and_moon            ) },
      { "eclipse",                   OPT_INT,  &( talents.eclipse                   ) },
      { "feral_aggression",          OPT_INT,  &( talents.feral_aggression          ) },
      { "feral_instinct",            OPT_INT,  &( talents.feral_instinct            ) },
      { "feral_swiftness",           OPT_INT,  &( talents.feral_swiftness           ) },
      { "ferocity",                  OPT_INT,  &( talents.ferocity                  ) },
      { "force_of_nature",           OPT_INT,  &( talents.force_of_nature           ) },
      { "furor",                     OPT_INT,  &( talents.furor                     ) },
      { "gale_winds",                OPT_INT,  &( talents.gale_winds                ) },
      { "genesis",                   OPT_INT,  &( talents.genesis                   ) },
      { "heart_of_the_wild",         OPT_INT,  &( talents.heart_of_the_wild         ) },
      { "improved_faerie_fire",      OPT_INT,  &( talents.improved_faerie_fire      ) },
      { "improved_insect_swarm",     OPT_INT,  &( talents.improved_insect_swarm     ) },
      { "improved_mangle",           OPT_INT,  &( talents.improved_mangle           ) },
      { "improved_mark_of_the_wild", OPT_INT,  &( talents.improved_mark_of_the_wild ) },
      { "improved_moonfire",         OPT_INT,  &( talents.improved_moonfire         ) },
      { "improved_moonkin_form",     OPT_INT,  &( talents.improved_moonkin_form     ) },
      { "infected_wounds",           OPT_INT,  &( talents.infected_wounds           ) },
      { "insect_swarm",              OPT_INT,  &( talents.insect_swarm              ) },
      { "intensity",                 OPT_INT,  &( talents.intensity                 ) },
      { "king_of_the_jungle",        OPT_INT,  &( talents.king_of_the_jungle        ) },
      { "leader_of_the_pack",        OPT_INT,  &( talents.leader_of_the_pack        ) },
      { "living_spirit",             OPT_INT,  &( talents.living_spirit             ) },
      { "lunar_guidance",            OPT_INT,  &( talents.lunar_guidance            ) },
      { "mangle",                    OPT_INT,  &( talents.mangle                    ) },
      { "master_shapeshifter",       OPT_INT,  &( talents.master_shapeshifter       ) },
      { "moonfury",                  OPT_INT,  &( talents.moonfury                  ) },
      { "moonglow",                  OPT_INT,  &( talents.moonglow                  ) },
      { "moonkin_form",              OPT_INT,  &( talents.moonkin_form              ) },
      { "natural_perfection",        OPT_INT,  &( talents.natural_perfection        ) },
      { "natural_reaction",          OPT_INT,  &( talents.natural_reaction          ) },
      { "naturalist",                OPT_INT,  &( talents.naturalist                ) },
      { "natures_grace",             OPT_INT,  &( talents.natures_grace             ) },
      { "natures_majesty",           OPT_INT,  &( talents.natures_majesty           ) },
      { "natures_reach",             OPT_INT,  &( talents.natures_reach             ) },
      { "natures_splendor",          OPT_INT,  &( talents.natures_splendor          ) },
      { "natures_swiftness",         OPT_INT,  &( talents.natures_swiftness         ) },
      { "omen_of_clarity",           OPT_INT,  &( talents.omen_of_clarity           ) },
      { "owlkin_frenzy",             OPT_INT,  &( talents.owlkin_frenzy             ) },
      { "predatory_instincts",       OPT_INT,  &( talents.predatory_instincts       ) },
      { "predatory_strikes",         OPT_INT,  &( talents.predatory_strikes         ) },
      { "primal_fury",               OPT_INT,  &( talents.primal_fury               ) },
      { "primal_gore",               OPT_INT,  &( talents.primal_gore               ) },
      { "primal_precision",          OPT_INT,  &( talents.primal_precision          ) },
      { "protector_of_the_pack",     OPT_INT,  &( talents.protector_of_the_pack     ) },
      { "rend_and_tear",             OPT_INT,  &( talents.rend_and_tear             ) },
      { "savage_fury",               OPT_INT,  &( talents.savage_fury               ) },
      { "sharpened_claws",           OPT_INT,  &( talents.sharpened_claws           ) },
      { "shredding_attacks",         OPT_INT,  &( talents.shredding_attacks         ) },
      { "survival_instincts",        OPT_INT,  &( talents.survival_of_the_fittest   ) },
      { "survival_of_the_fittest",   OPT_INT,  &( talents.survival_instincts        ) },
      { "starlight_wrath",           OPT_INT,  &( talents.starlight_wrath           ) },
      { "thick_hide",                OPT_INT,  &( talents.thick_hide                ) },
      { "typhoon",                   OPT_INT,  &( talents.typhoon                   ) },
      { "vengeance",                 OPT_INT,  &( talents.vengeance                 ) },
      { "wrath_of_cenarius",         OPT_INT,  &( talents.wrath_of_cenarius         ) },
      // @option_doc loc=player/druid/misc title="Misc"
      { "idol",                      OPT_STRING, &( items[ SLOT_RANGED ].options_str ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, druid_options );
  }

  return options;
}

// druid_t::decode_set ======================================================

int druid_t::decode_set( item_t& item )
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

  bool is_caster = ( strstr( s, "cover"     ) ||
                     strstr( s, "mantle"    ) ||
                     strstr( s, "vestment" ) ||
                     strstr( s, "trousers"  ) ||
                     strstr( s, "gloves"    ) );

  bool is_melee = ( strstr( s, "headguard"    ) ||
                    strstr( s, "shoulderpads" ) ||
                    strstr( s, "raiment"      ) ||
                    strstr( s, "legguards"    ) ||
                    strstr( s, "handgrips"    ) );

  if ( strstr( s, "dreamwalker" ) )
  {
    if ( is_caster ) return SET_T7_CASTER;
    if ( is_melee  ) return SET_T7_MELEE;
  }
  if ( strstr( s, "nightsong" ) )
  {
    if ( is_caster ) return SET_T8_CASTER;
    if ( is_melee  ) return SET_T8_MELEE;
  }
  if ( strstr( s, "malfurions" ) ||
       strstr( s, "runetotems" ) )
  {
    if ( is_caster ) return SET_T9_CASTER;
    if ( is_melee  ) return SET_T9_MELEE;
  }
  if ( strstr( s, "lasherweave" ) )
  {
    if ( is_caster ) return SET_T10_CASTER;
    if ( is_melee  ) return SET_T10_MELEE;
  }

  return SET_NONE;
}

// druid_t::primary_role ====================================================

int druid_t::primary_role() SC_CONST
{
  return talents.moonkin_form ? ROLE_SPELL : ROLE_ATTACK;
}

// druid_t::primary_resource ================================================

int druid_t::primary_resource() SC_CONST
{
  if ( talents.moonkin_form ) return RESOURCE_MANA;
  if ( tank > 0 ) return RESOURCE_RAGE;
  return RESOURCE_ENERGY;
}

// druid_t::primary_tree ====================================================

int druid_t::primary_tree() SC_CONST
{
  if ( talents.moonkin_form       ) return TREE_BALANCE;
  if ( talents.leader_of_the_pack ) return TREE_FERAL;
  return TREE_RESTORATION;
}

// druid_t::target_swing ====================================================

int druid_t::target_swing()
{
  int result = player_t::target_swing();

  if ( sim -> log ) log_t::output( sim, "%s swing result: %s", sim -> target -> name(), util_t::result_type_string( result ) );

  if ( result == RESULT_HIT    ||
       result == RESULT_CRIT   ||
       result == RESULT_GLANCE ||
       result == RESULT_BLOCK  )
  {
    resource_gain( RESOURCE_RAGE, 100.0, gains_incoming_damage );  // FIXME! Assume it caps rage every time.
  }
  if ( result == RESULT_DODGE )
  {
    if ( talents.natural_reaction )
    {
      resource_gain( RESOURCE_RAGE, talents.natural_reaction, gains_natural_reaction );
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

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_druid  ==================================================

player_t* player_t::create_druid( sim_t*             sim,
                                  const std::string& name,
                                  int race_type )
{
  return new druid_t( sim, name, race_type );
}

// player_t::druid_init =====================================================

void player_t::druid_init( sim_t* sim )
{
  sim -> auras.leader_of_the_pack = new aura_t( sim, "leader_of_the_pack" );
  sim -> auras.moonkin            = new aura_t( sim, "moonkin" );
  sim -> auras.improved_moonkin   = new aura_t( sim, "improved_moonkin" );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.innervate        = new      buff_t( p, "innervate",                      1, 10.0 );
    p -> buffs.mark_of_the_wild = new stat_buff_t( p, "mark_of_the_wild", STAT_MAX, 37, 1 );
  }

  target_t* t = sim -> target;
  t -> debuffs.earth_and_moon       = new debuff_t( sim, "earth_and_moon",       1,  12.0 );
  t -> debuffs.faerie_fire          = new debuff_t( sim, "faerie_fire",          1, 300.0 );
  t -> debuffs.improved_faerie_fire = new debuff_t( sim, "improved_faerie_fire", 1, 300.0 );
  t -> debuffs.infected_wounds      = new debuff_t( sim, "infected_wounds",      1,  12.0 );
  t -> debuffs.insect_swarm         = new debuff_t( sim, "insect_swarm",         1,  12.0 );
  t -> debuffs.mangle               = new debuff_t( sim, "mangle",               1,  60.0 );
}

// player_t::druid_combat_begin =============================================

void player_t::druid_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.leader_of_the_pack     ) sim -> auras.leader_of_the_pack -> override();
  if ( sim -> overrides.moonkin_aura           ) sim -> auras.moonkin            -> override();
  if ( sim -> overrides.improved_moonkin_aura  ) sim -> auras.improved_moonkin   -> override();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.mark_of_the_wild ) p -> buffs.mark_of_the_wild -> override( 1, 37.0 * 1.20 );
    }
  }

  target_t* t = sim -> target;
  if ( sim -> overrides.earth_and_moon       ) t -> debuffs.earth_and_moon       -> override( 1, 13 );
  if ( sim -> overrides.faerie_fire          ) t -> debuffs.faerie_fire          -> override();
  if ( sim -> overrides.improved_faerie_fire ) t -> debuffs.improved_faerie_fire -> override( 1, 3 );
  if ( sim -> overrides.infected_wounds      ) t -> debuffs.infected_wounds      -> override();
  if ( sim -> overrides.insect_swarm         ) t -> debuffs.insect_swarm         -> override();
  if ( sim -> overrides.mangle               ) t -> debuffs.mangle               -> override();
}

