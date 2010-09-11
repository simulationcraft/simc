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

  // Eclipse, same as the ingame bar
  int eclipse_bar_value;

  // Buffs
  buff_t* buffs_berserk;
  buff_t* buffs_bear_form;
  buff_t* buffs_cat_form;
  buff_t* buffs_combo_points;
  buff_t* buffs_eclipse_lunar;
  buff_t* buffs_eclipse_solar;
  buff_t* buffs_enrage;
  buff_t* buffs_glyph_of_innervate;
  buff_t* buffs_lacerate;
  buff_t* buffs_lunar_shower;
  buff_t* buffs_moonkin_form;
  buff_t* buffs_natures_swiftness;
  buff_t* buffs_omen_of_clarity;
  buff_t* buffs_pulverize;
  buff_t* buffs_savage_roar;
  buff_t* buffs_stealthed;
  buff_t* buffs_t8_4pc_caster;
  buff_t* buffs_t10_2pc_caster;
  buff_t* buffs_t11_4pc_caster;
  buff_t* buffs_tigers_fury;

  // Cooldowns
  cooldown_t* cooldowns_mangle_bear;
  cooldown_t* cooldowns_fury_swipes;

  // DoTs
  dot_t* dots_rip;
  dot_t* dots_rake;
  dot_t* dots_insect_swarm;
  dot_t* dots_moonfire;

  // Gains
  gain_t* gains_bear_melee;
  gain_t* gains_energy_refund;
  gain_t* gains_enrage;
  gain_t* gains_euphoria;
  gain_t* gains_glyph_of_innervate;
  gain_t* gains_incoming_damage;
  gain_t* gains_moonkin_form;
  gain_t* gains_natural_reaction;
  gain_t* gains_omen_of_clarity;
  gain_t* gains_primal_fury;
  gain_t* gains_tigers_fury;

  // Procs
  proc_t* procs_combo_points_wasted;
  proc_t* procs_fury_swipes;
  proc_t* procs_parry_haste;
  proc_t* procs_primal_fury;
  proc_t* procs_tier8_2pc;

  // Up-Times
  uptime_t* uptimes_energy_cap;
  uptime_t* uptimes_rage_cap;
  uptime_t* uptimes_rip;
  uptime_t* uptimes_rake;

  // Random Number Generation
  rng_t* rng_fury_swipes;
  rng_t* rng_nom_nom_nom;
  rng_t* rng_primal_fury;
  
  attack_t* cat_melee_attack;
  attack_t* bear_melee_attack;

  double equipped_weapon_dps;

  // Talents
  // Checked, build 12875
  /* X: Done!
  Balance
    X Innervate now regenerate mana equal to 20% of the casting Druid's maximum mana pool, down from 33%.
    X Lunar Shower now increases damage done by your Moonfire by 15/30/45% (Up from 2/4/8%)
    X Moonglow now reduces the mana cost of all your damage and healing spells.
    X Nature's Majesty now increases the critical strike chance with all spells.
  Feral
    * Thrash now has a 6 sec cooldown, up from 5 sec.
    * Swipe (Bear) base damage increased by 33%.
    * Swipe (Cat) now deals 215% weapon damage, up from 125%.
    X Furor now increases your maximum mana by 5/10/15% instead of increasing your intellect by 2/4/6%.
  Restoration
    X Heart of the Wild no longer increases maximum mana by 5/10/15%, now increases Intellect by 2/4/6%.
  */
  // Checked, build 12803
  talent_t* talent_balance_of_power;
  talent_t* talent_berserk;
  talent_t* talent_blessing_of_the_grove;
  talent_t* talent_brutal_impact; // NYI
  talent_t* talent_earth_and_moon;
  talent_t* talent_endless_carnage;
  talent_t* talent_euphoria;
  talent_t* talent_feral_aggression;
  talent_t* talent_feral_charge; // NYI
  talent_t* talent_feral_swiftness;
  talent_t* talent_force_of_nature;
  talent_t* talent_fungal_growth;
  talent_t* talent_furor;
  talent_t* talent_fury_of_stormrage; // NYI
  talent_t* talent_fury_swipes;
  talent_t* talent_gale_winds;
  talent_t* talent_genesis;
  talent_t* talent_heart_of_the_wild;
  talent_t* talent_improved_feral_charge; // Valid to run out->charge->ravage?
  talent_t* talent_infected_wounds;
  talent_t* talent_king_of_the_jungle;
  talent_t* talent_leader_of_the_pack;
  talent_t* talent_lunar_guidance;
  talent_t* talent_lunar_shower;
  talent_t* talent_master_shapeshifter;
  talent_t* talent_moonglow;
  talent_t* talent_moonkin_form;
  talent_t* talent_natural_reaction;
  talent_t* talent_natures_grace;
  talent_t* talent_natures_majesty;
  talent_t* talent_natures_swiftness;
  talent_t* talent_nom_nom_nom;
  talent_t* talent_nurturing_instict; // NYI
  talent_t* talent_owlkin_frenzy;
  talent_t* talent_predatory_strikes; // NYI
  talent_t* talent_primal_fury;
  talent_t* talent_primal_madness; // NYI
  talent_t* talent_pulverize; // NYI
  talent_t* talent_rend_and_tear;
  talent_t* talent_solar_beam;
  talent_t* talent_starfall;
  talent_t* talent_starlight_wrath;
  talent_t* talent_survival_instincts; // NYI
  talent_t* talent_thick_hide; // How does the +10% and +33%@bear stack etc
  talent_t* talent_typhoon;

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
    int wrath;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  druid_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, DRUID, name, r )
  {
    active_t10_4pc_caster_dot = 0;
    
    eclipse_bar_value = 0;

    cooldowns_mangle_bear = get_cooldown( "mangle_bear" );
    cooldowns_fury_swipes = get_cooldown( "fury_swipes" );

    distance = 30;

    dots_rip          = get_dot( "rip" );
    dots_rake         = get_dot( "rake" );
    dots_insect_swarm = get_dot( "insect_swarm" );
    dots_moonfire     = get_dot( "moonfire" );

    cat_melee_attack = 0;
    bear_melee_attack = 0;

    equipped_weapon_dps = 0;
    
    talent_balance_of_power      = new talent_t( this, "balance_of_power", "Balance of Power" );
    talent_berserk               = new talent_t( this, "berserk", "Berserk" );
    talent_blessing_of_the_grove = new talent_t( this, "blessing_of_the_grove", "Blessing of the Grove" );
    talent_brutal_impact         = new talent_t( this, "brutal_impact", "Brutal Impact" );
    talent_earth_and_moon        = new talent_t( this, "earth_and_moon", "Earth and Moon" );
    talent_endless_carnage       = new talent_t( this, "endless_carnage", "Endless Carnage" );
    talent_euphoria              = new talent_t( this, "euphoria", "Euphoria" );
    talent_feral_aggression      = new talent_t( this, "feral_aggression", "Feral Aggression" );
    talent_feral_charge          = new talent_t( this, "feral_charge", "Feral Charge" );
    talent_feral_swiftness       = new talent_t( this, "feral_swiftness", "Feral Swiftness" );
    talent_force_of_nature       = new talent_t( this, "force_of_nature", "Force of Nature" );
    talent_fungal_growth         = new talent_t( this, "fungal_growth", "Fungal Growth" );
    talent_furor                 = new talent_t( this, "furor", "Furor" );
    talent_fury_of_stormrage     = new talent_t( this, "fury_of_stormrage", "Fury of Stormrage" );
    talent_fury_swipes           = new talent_t( this, "fury_swipes", "Fury Swipes" );
    talent_gale_winds            = new talent_t( this, "gale_winds", "Gale Winds" );
    talent_genesis               = new talent_t( this, "genesis", "Genesis" );
    talent_heart_of_the_wild     = new talent_t( this, "heart_of_the_wild", "Heart of the Wild" );
    talent_improved_feral_charge = new talent_t( this, "improved_feral_charge", "Improved Feral Charge" );
    talent_infected_wounds       = new talent_t( this, "infected_wounds", "Infected Wounds" );
    talent_king_of_the_jungle    = new talent_t( this, "king_of_the_jungle", "King of the Jungle" );
    talent_leader_of_the_pack    = new talent_t( this, "leader_of_the_pack", "Leader of the Pack" );
    talent_lunar_guidance        = new talent_t( this, "lunar_guidance", "Lunar Guidance" );
    talent_lunar_shower          = new talent_t( this, "lunar_shower", "Lunar Shower" );
    talent_master_shapeshifter   = new talent_t( this, "master_shapeshifter", "Master Shapeshifter" );
    talent_moonglow              = new talent_t( this, "moonglow", "Moonglow" );
    talent_moonkin_form          = new talent_t( this, "moonkin_form", "Moonkin Form" );
    talent_natural_reaction      = new talent_t( this, "natural_reaction", "Natural Reaction" );
    talent_natures_grace         = new talent_t( this, "natures_grace", "Natures Grace" );
    talent_natures_majesty       = new talent_t( this, "natures_majesty", "Natures Majesty" );
    talent_natures_swiftness     = new talent_t( this, "natures_swiftness", "Natures Swiftness" );
    talent_nom_nom_nom           = new talent_t( this, "nom_nom_nom", "Nom Nom Nom" );
    talent_nurturing_instict     = new talent_t( this, "nurturing_instinct", "Nurturing Instinct" );
    talent_owlkin_frenzy         = new talent_t( this, "owlkin_frenzy", "Owlkin Frenzy" );
    talent_predatory_strikes     = new talent_t( this, "predatory_strikes", "Predatory Strikes" );
    talent_primal_fury           = new talent_t( this, "primal_fury", "Primal Fury" );
    talent_primal_madness        = new talent_t( this, "primal_madness", "Primal Madness" );
    talent_pulverize             = new talent_t( this, "pulverize", "Pulverize" );
    talent_rend_and_tear         = new talent_t( this, "rend_and_tear", "Rend and Tear" );
    talent_solar_beam            = new talent_t( this, "solar_beam", "Solar Beam" );
    talent_starfall              = new talent_t( this, "starfall", "Starfall" );
    talent_starlight_wrath       = new talent_t( this, "starlight_wrath", "Starlight Wrath" );
    talent_survival_instincts    = new talent_t( this, "survival_instincts", "Survival Instincts" );
    talent_thick_hide            = new talent_t( this, "thick_hide", "Thick Hide" );
    talent_typhoon               = new talent_t( this, "typhoon", "Typhoon" );
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
  virtual talent_tree_type  primary_tree() SC_CONST;
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
    //druid_t* p = player -> cast_druid();

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
      //druid_t* o = player -> cast_pet() -> owner -> cast_druid();

      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      background = true;
      repeating = true;
      may_crit = true;

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

// trigger_omen_of_clarity ==================================================

static void trigger_omen_of_clarity( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( a -> aoe ) return;
  if ( a -> proc ) return;

  if ( p -> buffs_omen_of_clarity -> trigger() )
    p -> buffs_t10_2pc_caster -> trigger( 1, 0.15 );

}

// trigger_fury_swipes ======================================================

static void trigger_fury_swipes( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( a -> proc ) return;

  if ( a -> result_is_miss() ) return;

  if ( ! a -> weapon ) return;

  if ( ! p -> talent_fury_swipes -> rank() )
    return;

  if ( p -> cooldowns_fury_swipes -> remains() > 0 )
    return;

  if ( p -> rng_fury_swipes -> roll( 0.04 * p -> talent_fury_swipes -> rank() ) )
  {
    if ( a -> sim -> log )
      log_t::output( a -> sim, "%s gains one extra attack through %s",
                     p -> name(), p -> procs_fury_swipes -> name() );

    p -> procs_fury_swipes -> occur();
    p -> cooldowns_fury_swipes -> start( 6.0 );

    p -> main_hand_attack -> proc = true;
    p -> main_hand_attack -> execute();
    p -> main_hand_attack -> proc = false;
  }
}

// trigger_infected_wounds ==================================================

static void trigger_infected_wounds( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( p -> talent_infected_wounds -> rank() )
  {
    p -> sim -> target -> debuffs.infected_wounds -> trigger();
  }
}

// trigger_earth_and_moon ===================================================

static void trigger_earth_and_moon( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if ( p -> talent_earth_and_moon -> rank() == 0 ) return;

  target_t* t = s -> sim -> target;

  t -> debuffs.earth_and_moon -> trigger( 1, 8 );
}

// trigger_eclipse_energy_gain ==============================================

static void trigger_eclipse_energy_gain( spell_t* s, int gain )
{
  // EVERY druid has the eclipse mechanic, regardless of chosen tree
  // but only balance druid get an actual bonus from it (12% base + mastery)
  if ( gain == 0 ) return;
  
  druid_t* p = s -> player -> cast_druid();

  int old_eclipse_bar_value = p -> eclipse_bar_value;
  p -> eclipse_bar_value += gain;

  if ( p -> eclipse_bar_value < 0 ) 
  {
    p -> buffs_eclipse_solar -> expire();

    if ( p -> eclipse_bar_value < -99 ) 
      p -> eclipse_bar_value = -100;
  }

  if ( p -> eclipse_bar_value > 0 ) 
  {
    p -> buffs_eclipse_lunar -> expire();

    if ( p -> eclipse_bar_value > 99 ) 
      p -> eclipse_bar_value = 100;
  }
  
  int actual_gain = p -> eclipse_bar_value - old_eclipse_bar_value;
  if ( s -> sim -> log )
  {
    log_t::output( s -> sim, "%s gains %d (%d) %s from %s (%d)",
                   p -> name(), actual_gain, gain,
                   "Eclipse", s -> name(),
                   p -> eclipse_bar_value );
  }
  
  
  // Eclipse proc:
  // Procs when you reach 100 and only then, you have to have an
  // actual gain of eclipse energy (bar value)
  // buffs themselves have a 30s cd before they proc again
  if ( actual_gain != 0 )
  {
    if ( p -> eclipse_bar_value == 100 ) 
    {
      if ( p -> buffs_eclipse_solar -> trigger() )
      {
        p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.06 * p -> talent_euphoria -> rank() , p -> gains_euphoria );
        p -> buffs_t11_4pc_caster -> trigger( 3 );
      }
    }
    else if ( p -> eclipse_bar_value == -100 ) 
    {
      if ( p -> buffs_eclipse_lunar -> trigger() )
      {
        p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.06 * p -> talent_euphoria -> rank() , p -> gains_euphoria );
        p -> buffs_t11_4pc_caster -> trigger( 3 );
      }
    }
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

  if ( ! p -> talent_primal_fury -> rank() )
    return;

  if ( ! a -> adds_combo_points )
    return;

  if ( p -> rng_primal_fury -> roll( p -> talent_primal_fury -> rank() * 0.5 ) )
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

// trigger_primal_fury =====================================================

static void trigger_primal_fury( druid_bear_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talent_primal_fury -> rank() )
    return;

  if ( p -> rng_primal_fury -> roll( p -> talent_primal_fury -> rank() * 0.5 ) )
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
  }

  if ( harmful ) p -> buffs_stealthed -> expire();
}

// druid_cat_attack_t::player_buff =========================================

void druid_cat_attack_t::player_buff()
{
  druid_t* p = player -> cast_druid();

  attack_t::player_buff();

  player_dd_adder += p -> buffs_tigers_fury -> value();

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
  virtual void player_buff()
  {
    druid_cat_attack_t::player_buff();
    druid_t* p = player -> cast_druid();
    player_multiplier *= 1 + p -> buffs_savage_roar -> value();
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
      trigger_fury_swipes( this );
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
    check_talent( p -> talent_berserk -> rank() );

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
    // Berserk cancels TF
    p -> buffs_tigers_fury -> expire();
    p -> buffs_berserk -> trigger();
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

    id = 1082;
    parse_data( p -> player_data );

    weapon = &( p -> main_hand_weapon );
    adds_combo_points = true;
    may_crit          = true;
    base_multiplier  *= 1.0 + util_t::talent_rank( p -> talent_blessing_of_the_grove -> rank(), 2, 0.02 );
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

    // By default, do not overwrite Mangle
    max_mangle_expire = 0.001;

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 283, 283, 0, 35 },
      { 75, 4, 239, 239, 0, 35 },
      { 68, 3, 165, 165, 0, 35 },
      { 58, 2, 128, 128, 0, 35 },
      {  0, 0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48566 );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 2.0;

    adds_combo_points = true;
    may_crit          = true;

    base_multiplier  *= 1.0 + p -> glyphs.mangle * 0.1;
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    if ( result_is_hit() )
    {
      target_t* t = sim -> target;
      t -> debuffs.mangle -> trigger();
      trigger_infected_wounds( this );
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
      { 78, 7, 190, 190, 387, 35 },
      { 72, 6, 150, 150, 321, 35 },
      { 64, 5,  90,  90, 138, 35 },
      { 54, 4,  64,  90,  99, 35 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48574 );

    adds_combo_points = true;
    may_crit          = true;
    base_tick_time    = 3.0;
    num_ticks         = 3 + p -> talent_endless_carnage -> rank();
    direct_power_mod  = 0.01;
    tick_power_mod    = 0.06;
    tick_may_crit     = true;
    dot_behavior      = DOT_REFRESH;

    if ( p -> set_bonus.tier9_2pc_melee() ) num_ticks++;

  }
  virtual void tick()
  {
    druid_cat_attack_t::tick();
    trigger_t8_2pc_melee( this );
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
    tick_may_crit     = true;
    dot_behavior      = DOT_REFRESH;

    num_ticks = 8 + ( p -> set_bonus.tier7_2pc_melee() * 2 );
    if ( p -> glyphs.rip )
      base_multiplier *= 1.15;

    static double dmg_80[] = { 36+93*1, 36+93*2, 36+93*3, 36+93*4, 36+93*5 };
    static double dmg_71[] = { 30+67*1, 30+67*2, 30+67*3, 30+67*4, 30+67*5 };
    static double dmg_67[] = { 24+48*1, 24+48*2, 24+48*3, 24+48*4, 24+48*5 };
    static double dmg_60[] = { 17+28*1, 17+28*2, 17+28*3, 17+28*4, 17+28*5 };


    combo_point_dmg = ( p -> level >= 80 ? dmg_80 :
                        p -> level >= 71 ? dmg_71 :
                        p -> level >= 67 ? dmg_67 :
                        dmg_60 );

    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += 0.05;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_cat_attack_t::execute();
    if ( result_is_hit() )
    {
      added_ticks = 0;
      num_ticks = 8 + ( p -> set_bonus.tier7_2pc_melee() * 2 );
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

    buff_value = 0.50;
    if ( p -> glyphs.savage_roar ) buff_value += 0.03;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    double duration = 9.0 + 5.0 * p -> buffs_combo_points -> stack();
    duration += 3.0 * p -> talent_endless_carnage -> rank();
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
      { 80, 9, 296, 296, 0, 40 },
      { 75, 8, 251, 251, 0, 40 },
      { 70, 7, 180, 180, 0, 40 },
      { 61, 6, 105, 105, 0, 40 },
      { 54, 5,  80,  80, 0, 40 },
      {  0, 0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48572 );

    weapon_multiplier *= 2.25;

    weapon = &( p -> main_hand_weapon );
    requires_position  = POSITION_BACK;
    adds_combo_points  = true;
    may_crit           = true;

    base_multiplier  *= 1.0 + util_t::talent_rank( p -> talent_blessing_of_the_grove -> rank(), 2, 0.02 );

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
    if ( result_is_hit() )
    {
      trigger_infected_wounds( this );
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
      player_multiplier *= 1 + 0.20/3.0 * p -> talent_rend_and_tear -> rank();
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

    if ( p -> talent_king_of_the_jungle -> rank() )
    {
      p -> resource_gain( RESOURCE_ENERGY, p -> talent_king_of_the_jungle -> rank() * 20, p -> gains_tigers_fury );
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
  double excess_energy;
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

    base_multiplier *= 1.0 + ( p -> talent_feral_aggression -> rank() * 0.05 );

    static double_pair dmg_78[] = { { 410, 550 }, { 700, 840 }, { 990, 1130 }, { 1280, 1420 }, { 1570, 1710 } };
    static double_pair dmg_72[] = { { 334, 682 }, { 570, 682 }, { 806,  918 }, { 1042, 1154 }, { 1278, 1390 } };
    static double_pair dmg_63[] = { { 226, 292 }, { 395, 461 }, { 564,  630 }, {  733,  799 }, {  902,  968 } };
    static double_pair dmg_60[] = { { 199, 259 }, { 346, 406 }, { 493,  533 }, {  640,  700 }, {  787,  847 } };

    combo_point_dmg   = ( p -> level >= 78 ? dmg_78 :
                          p -> level >= 72 ? dmg_72 :
                          p -> level >= 63 ? dmg_63 :
                          dmg_60 );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    base_dd_min = combo_point_dmg[ p -> buffs_combo_points -> current_stack - 1 ].min;
    base_dd_max = combo_point_dmg[ p -> buffs_combo_points -> current_stack - 1 ].max;

    direct_power_mod = 0.07 * p -> buffs_combo_points -> stack();
    // consumes up to 35 additional energy to increase damage by up to 100%.
    // Assume 100/35 = 2.857% per additional energy consumed
    excess_energy = ( p -> resource_current[ RESOURCE_ENERGY ] - druid_cat_attack_t::cost() );

    if ( excess_energy > 35 )
    {
      excess_energy     = 35;
    }
    else if ( excess_energy < 0 )
    {
      excess_energy     = 0;
    }

    druid_cat_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> rng_nom_nom_nom -> roll( p -> talent_nom_nom_nom -> rank() * 0.5 ) )
      {
        p -> dots_rip -> action -> refresh_duration();
      }
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

    player_multiplier *= 1.0 + excess_energy / 35.0;

    if ( sim -> target -> debuffs.bleeding -> check() )
    {
      player_crit += 0.25/3.0 * p -> talent_rend_and_tear -> rank();
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

  if ( p -> talent_master_shapeshifter -> rank() )
  {
    player_multiplier *= 1.0 + p -> talent_master_shapeshifter -> rank() * 0.04;
  }
  if ( p -> talent_king_of_the_jungle -> rank() && p -> buffs_enrage -> up() )
  {
    player_multiplier *= 1.0 + p -> talent_king_of_the_jungle -> rank() * 0.05;
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
    druid_bear_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_fury_swipes( this );
      trigger_rage_gain( this );
      trigger_omen_of_clarity( this );
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
    cooldown -> duration  = 60 - p -> talent_brutal_impact -> rank() * 15;
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
    check_talent( p -> talent_berserk -> rank() );

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
    tick_power_mod   = 0.01;
    tick_may_crit    = true;
    dot_behavior = DOT_REFRESH;

    if ( p -> set_bonus.tier9_2pc_melee() ) base_td_multiplier *= 1.05;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_bear_attack_t::execute();
    if ( result_is_hit() )
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

    base_multiplier *= 1.0 + p -> glyphs.mangle * 0.10;

    cooldown = p -> get_cooldown( "mangle_bear" );
    cooldown -> duration = 6.0;
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

    trigger_gcd  = 0;
    may_crit     = true;

    // TODO: scaling?
    weapon = &( p -> main_hand_weapon );
    //normalize_weapon_speed = false;

    cooldown -> duration = 3;
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();
    if ( result_is_hit() )
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
      player_multiplier *= 1 + 0.20/3.0 * p -> talent_rend_and_tear -> rank();
    }

  }

};

// Swipe (Bear) ============================================================

struct swipe_bear_t : public druid_bear_attack_t
{
  swipe_bear_t( player_t* player, const std::string& options_str ) :
      druid_bear_attack_t( "swipe_bear", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    // druid_t* p = player -> cast_druid();

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
  double c = spell_t::cost();
  if ( resource == RESOURCE_MANA && p -> talent_moonglow -> rank() )
    c *= 1.0 - 0.03 * p -> talent_moonglow -> rank();

  return spell_t::cost();
}

// druid_spell_t::haste ====================================================

double druid_spell_t::haste() SC_CONST
{
  druid_t* p = player -> cast_druid();
  double h =  spell_t::haste();

  h *= 1.0 / ( 1.0 + 0.01 * p -> talent_natures_grace -> rank() );

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

  if ( result == RESULT_CRIT ) p -> buffs_t11_4pc_caster -> decrement();

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
    player_multiplier *= 1.0 + p -> talent_master_shapeshifter -> rank() * 0.04;
  }
  if ( school == SCHOOL_ARCANE || school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
  {
    player_multiplier *= 1.0 + 0.01 * p -> talent_balance_of_power -> rank();

    player_multiplier *= 1.0 + p -> buffs_t10_2pc_caster -> value();
    // Moonfury: Arcane and Nature spell damage increased by 25%
    // One of the bonuses for choosing balance spec
    if ( p -> primary_tree() == TREE_BALANCE )
      player_multiplier *= 1.25;

    if ( p -> buffs_moonkin_form -> check() )
      player_multiplier *= 1.10;
  }
  // Both eclipse buffs need their own checks
  // Eclipse increases wrath damage by 1.5% per mastery point
  if ( school == SCHOOL_ARCANE || school == SCHOOL_SPELLSTORM )
    if ( p -> buffs_eclipse_lunar -> up() )
      player_multiplier *= 1.0 + p -> composite_mastery() * 0.015;

  if ( school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
    if ( p -> buffs_eclipse_solar -> up() )
      player_multiplier *= 1.0 + p -> composite_mastery() * 0.015;


  player_multiplier *= 1.0 + p -> talent_earth_and_moon -> rank() * 0.02;
  
  player_crit += 0.33 * p -> buffs_t11_4pc_caster -> stack();

}

// druid_spell_t::target_debuff ============================================

void druid_spell_t::target_debuff( int dmg_type )
{
  //druid_t*  p = player -> cast_druid();
  //target_t* t = sim -> target;
  spell_t::target_debuff( dmg_type );
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
    sim -> target -> debuffs.faerie_fire -> trigger( 1 + p -> talent_feral_aggression -> rank() );
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
    // TODO: convert to cata mechanic
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.08;
    id = 770;
  }

  virtual void execute()
  {
    druid_t*  p = player -> cast_druid();
    target_t* t = sim -> target;
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    t -> debuffs.faerie_fire -> trigger();
  }

  virtual bool ready()
  {
    // druid_t*  p = player -> cast_druid();
    // target_t* t = sim -> target;

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
    cooldown -> duration  = 180;

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
    // 
    // 4.0.0 (Cata): 180s cd
    // Targets gets 33% of casting druids MAXMANA over 10 seconds
    innervate_target -> buffs.innervate -> trigger( 1, player -> resource_max[ RESOURCE_MANA ] * 0.20 / 10 );

    // FIXME: Glyph will probably change sometime in the beta
    // druid_t* p = player -> cast_druid();
    // p -> buffs_glyph_of_innervate -> trigger( 1, player -> resource_max[ RESOURCE_MANA ]* 0.9 / 20.0 );
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
    tick_may_crit     = true;
    dot_behavior      = DOT_REFRESH;

    base_multiplier *= 1.0 + ( util_t::talent_rank( p -> talent_genesis -> rank(), 3, 0.02 ) +
                             ( p -> glyphs.insect_swarm          ? 0.30 : 0.00 ) +
                             ( p -> set_bonus.tier7_2pc_caster() ? 0.10 : 0.00 ) );
    if ( p -> set_bonus.tier11_2pc_caster() )
      base_crit += 0.05;
  }

  virtual void tick()
  {
    druid_spell_t::tick();
    druid_t* p = player -> cast_druid();
    p -> buffs_t8_4pc_caster -> trigger();
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
  moonfire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "moonfire", player, SCHOOL_ARCANE, TREE_BALANCE )
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
    tick_may_crit     = true;
    dot_behavior      = DOT_REFRESH;
    
    // Costreduction from moonglow and lunar shower is additive
    // up to 9%+90%=99%
    
    base_crit_bonus_multiplier *= 1.0 + (( p -> primary_tree() == TREE_BALANCE ) ? 1.0 : 0.0 );

    double multiplier_td = ( util_t::talent_rank( p -> talent_genesis -> rank(), 3, 0.02 ) );
    double multiplier_dd = ( util_t::talent_rank( p -> talent_blessing_of_the_grove -> rank(), 2, 0.03 ) );

    if ( p -> glyphs.moonfire )
    {
      multiplier_td += 0.20;
    }
    base_dd_multiplier *= 1.0 + multiplier_dd;
    base_td_multiplier *= 1.0 + multiplier_td;
    if ( p -> set_bonus.tier11_2pc_caster() )
      base_crit += 0.05;
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    // +2/4/8% damage bonus only applies to direct damage
    player_multiplier *= 1.0 + util_t::talent_rank( p -> talent_lunar_shower -> rank(), 3, 0.15 ) * p -> buffs_lunar_shower -> stack();
  }
  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    druid_spell_t::execute();
    // +2/4/8% damage bonus only applies to direct damage
    // Get rid of it for the ticks, hacky :<
    player_multiplier /= 1.0 + util_t::talent_rank( p -> talent_lunar_shower -> rank(), 3, 0.15 ) * p -> buffs_lunar_shower -> stack();


    if ( result_is_hit() )
    {
      num_ticks = 4;
      added_ticks = 0;
      if ( p -> set_bonus.tier6_2pc_caster() ) num_ticks++;
      update_ready();

      // If moving trigger all 3 stacks, because it will stack up immediately
      p -> buffs_lunar_shower -> trigger( p -> buffs.moving -> check() ? 3 : 1 );
    }
  }
  virtual double cost() SC_CONST
  {
    druid_t* p = player -> cast_druid();
    double c = druid_spell_t::cost();
    c *= 1.0 - (0.10 * p -> buffs_lunar_shower -> stack() * p -> talent_lunar_shower -> rank() );

    if ( c < 0 ) c = 0;
    return c;
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

    p -> dodge += 0.02 * p -> talent_feral_swiftness -> rank() + 0.03 * p -> talent_natural_reaction -> rank();
    p -> armor_multiplier += 3.7 * ( 1.0 + 0.11 * p -> talent_thick_hide -> rank() );

    if ( p -> talent_leader_of_the_pack -> rank() )
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

    p -> dodge += 0.02 * p -> talent_feral_swiftness -> rank();

    p -> buffs_cat_form -> start();
    p -> base_gcd = 1.0;
    p -> reset_gcd();

    if ( p -> talent_leader_of_the_pack -> rank() )
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
    check_talent( p -> talent_moonkin_form -> rank() );
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

    if ( p -> talent_furor -> rank() )
    {
      p -> attribute_multiplier[ ATTR_INTELLECT ] *= 1.0 + p -> talent_furor -> rank() * 0.02;
    }

    p -> armor_multiplier += 2.2;
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
    check_talent( p -> talent_natures_swiftness -> rank() );
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
  std::string prev_str;
  int extend_moonfire;
  int instant;

  starfire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "starfire", player, SCHOOL_ARCANE, TREE_BALANCE ),
      extend_moonfire( 0 ), instant( 0 )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { "extendmf", OPT_BOOL,   &extend_moonfire },
      { "prev",     OPT_STRING, &prev_str        },
      { "instant",  OPT_BOOL,   &instant         },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

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

    base_execute_time -= util_t::talent_rank( p -> talent_starlight_wrath -> rank(), 3, 0.15, 0.25, 0.5 );
    base_crit_bonus_multiplier *= 1.0 + (( p -> primary_tree() == TREE_BALANCE ) ? 1.0 : 0.0 );

    if ( p -> set_bonus.tier6_4pc_caster() ) base_crit += 0.05;
    if ( p -> set_bonus.tier7_4pc_caster() ) base_crit += 0.05;
    if ( p -> set_bonus.tier9_4pc_caster() ) base_multiplier *= 1.04;
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
      trigger_eclipse_energy_gain( this, 20 );

      if ( result == RESULT_CRIT )
      {
        trigger_t10_4pc_caster( player, direct_dmg, school );
        trigger_eclipse_energy_gain( this, 4 * p -> talent_euphoria -> rank() );
      }
      if ( p -> glyphs.starfire )
      {
        if ( p -> dots_moonfire -> ticking() )
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
  std::string prev_str;

  wrath_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "wrath", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();

    travel_speed = 21.0;

    option_t options[] =
    {
      { "prev",    OPT_STRING, &prev_str    },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

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

    base_execute_time = 2.5;
    direct_power_mod  = ( base_execute_time / 3.5 );
    may_crit          = true;

    base_execute_time -= util_t::talent_rank( p -> talent_starlight_wrath -> rank(), 3, 0.15, 0.25, 0.5 );
    base_crit_bonus_multiplier *= 1.0 + (( p -> primary_tree() == TREE_BALANCE ) ? 1.0 : 0.0 );

    if ( p -> set_bonus.tier7_4pc_caster() ) base_crit += 0.05;
    if ( p -> set_bonus.tier9_4pc_caster() ) base_multiplier   *= 1.04;

  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    if ( p -> glyphs.wrath && p -> dots_insect_swarm -> ticking() )
      player_multiplier *= 1.10;
  }
  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::travel( travel_result, travel_dmg );
    if ( result_is_hit() )
    {
      trigger_eclipse_energy_gain( this, -13 );
      if ( travel_result == RESULT_CRIT )
      {
        trigger_t10_4pc_caster( player, travel_dmg, SCHOOL_NATURE );

        trigger_eclipse_energy_gain( this, -2 * p -> talent_euphoria -> rank() );
      }
      trigger_earth_and_moon( this );
    }
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

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
    struct starfall_star_t : public druid_spell_t
    {
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
        base_crit_bonus_multiplier *= 1.0 + (( p -> primary_tree() == TREE_BALANCE ) ? 1.0 : 0.0 );

        if ( p -> glyphs.focus )
          base_multiplier *= 1.1;

        id = 53195;
      }

      virtual void execute()
      {
        druid_spell_t::execute();
        tick_dmg = direct_dmg;
        update_stats( DMG_OVER_TIME );
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


// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  starsurge_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "starsurge", player, SCHOOL_SPELLSTORM, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    
    assert ( p -> primary_tree() == TREE_BALANCE );
    
    travel_speed = 21.0;
    
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 85, 11, 1486, 1960, 0, 0.11 },
      { 84, 10, 1486, 1960, 0, 0.11 },
      { 83,  9, 1486, 1960, 0, 0.11 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 78674 );

    base_execute_time    = 2.0;
    direct_power_mod     = ( base_execute_time / 3.5 );
    may_crit             = true;
    cooldown -> duration = 15.0;
  }
  
  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::travel( travel_result, travel_dmg );
    // SF/Wrath even give eclipse on misses,
    // but not Starsurge
    if ( travel_result == RESULT_CRIT || travel_result == RESULT_HIT )
    {
      // It always pushes towards the side the bar is on, positive if at zero
      int gain = 10;
      if (  p -> eclipse_bar_value < 0 ) gain = -gain;
      trigger_eclipse_energy_gain( this, gain );
    }
  }  
};



// Typhoon Spell ============================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "typhoon", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talent_typhoon -> rank() );
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
    base_multiplier *= 1.0 + 0.15 * p -> talent_gale_winds -> rank();
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
  mark_of_the_wild_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "mark_of_the_wild", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    // TODO: Cata => +5% like BoK
    // druid_t* p = player -> cast_druid();

    trigger_gcd = 0;
    id = 1126;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> ooc_buffs() )
      {
        p -> buffs.mark_of_the_wild -> trigger();
      }
    }
  }

  virtual bool ready()
  {
    return ! player -> buffs.mark_of_the_wild -> check();
  }
};

// Treants Spell ============================================================

struct treants_spell_t : public druid_spell_t
{
  treants_spell_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "treants", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    check_talent( p -> talent_force_of_nature -> rank() );

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
  if ( name == "starsurge"         ) return new         starsurge_t( this, options_str );
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
    else if ( n == "wrath"        ) glyphs.wrath = 1;
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
  case RACE_WORGEN:
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
  player_t::init_base();

  initial_spell_power_per_spirit = 0.0;

  base_attack_power = -20 + level * 2.0;

  attribute_multiplier_initial[ ATTR_INTELLECT ]   *= 1.0 + talent_heart_of_the_wild -> rank() * 0.02;
  initial_attack_power_per_agility  = 0.0;
  initial_attack_power_per_strength = 2.0;

  // FIXME! Level-specific!  Should be form-specific!
  base_defense = level * 5;
  base_miss    = 0.05;
  base_dodge   = 0.0560970;
  initial_armor_multiplier  = 1.0 + util_t::talent_rank( talent_thick_hide -> rank(), 3, 0.04, 0.07, 0.10 );
  initial_dodge_per_agility = 0.0002090;
  initial_armor_per_agility = 2.0;

  diminished_kfactor    = 0.9560;
  diminished_miss_capi  = 1.0 / 0.16;
  diminished_dodge_capi = 1.0 / 0.88129021;
  diminished_parry_capi = 1.0 / 0.47003525;

  resource_base[ RESOURCE_ENERGY ] = 100;
  resource_base[ RESOURCE_RAGE   ] = 100;
  mana_per_intellect      = 15;
  health_per_stamina      = 10;
  energy_regen_per_second = 10;
  
  // Furor: +5/10/15% max mana
  resource_base[ RESOURCE_MANA ] *= 1.0 + talent_furor -> rank() * 0.05;
  mana_per_intellect             *= 1.0 + talent_furor -> rank() * 0.05;
  
  switch ( primary_tree() )
  {
  case TREE_BALANCE:
    // Dreamstate for choosing balance
    // 12857 Dreamstate seems to be gone
    // mp5_per_intellect = 0.10;
    break;
  case TREE_RESTORATION:
    // Intensity for choosing resto
    mana_regen_while_casting = 0.50;
    break;
  case TREE_FERAL:
    break;
  default:
    break;
  }

  base_gcd = 1.5;

  //if ( tank == -1 && talent_is_tank_spec() -> rank ) tank = 1;
}

// druid_t::init_buffs ======================================================

void druid_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_berserk            = new buff_t( this, "berserk"           , 1,  15.0 + ( glyphs.berserk ? 5.0 : 0.0 ) );
  buffs_eclipse_lunar      = new buff_t( this, "lunar_eclipse"     , 1,  45.0, 30.0 );
  buffs_eclipse_solar      = new buff_t( this, "solar_eclipse"     , 1,  45.0, 30.0 );
  buffs_enrage             = new buff_t( this, "enrage"            , 1,  10.0 );
  buffs_lacerate           = new buff_t( this, "lacerate"          , 3,  15.0 );
  buffs_lunar_shower       = new buff_t( this, "lunar_shower"      , 3,   3.0,     0, talent_lunar_shower -> rank() );
  buffs_natures_swiftness  = new buff_t( this, "natures_swiftness" , 1, 180.0, 180.0 );
  buffs_omen_of_clarity    = new buff_t( this, "omen_of_clarity"   , 1,  15.0,     0, 3.5 / 60.0 );
  buffs_pulverize          = new buff_t( this, "pulverize"         , 1,  10.0 + 3.0 * talent_endless_carnage -> rank() );
  buffs_t8_4pc_caster      = new buff_t( this, "t8_4pc_caster"     , 1,  10.0,     0, set_bonus.tier8_4pc_caster() * 0.08 );
  buffs_t10_2pc_caster     = new buff_t( this, "t10_2pc_caster"    , 1,   6.0,     0, set_bonus.tier10_2pc_caster() );
  buffs_t11_4pc_caster     = new buff_t( this, "t11_4pc_caster"    , 3,   8.0,     0, set_bonus.tier11_4pc_caster() );

  buffs_tigers_fury        = new buff_t( this, "tigers_fury"       , 1,   6.0 );
  buffs_glyph_of_innervate = new buff_t( this, "glyph_of_innervate", 1,  10.0,     0, glyphs.innervate);
  
  // stat_buff_t( sim, player, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )

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
  gains_euphoria           = get_gain( "euphoria"           );
  gains_glyph_of_innervate = get_gain( "glyph_of_innervate" );
  gains_incoming_damage    = get_gain( "incoming_damage"    );
  gains_moonkin_form       = get_gain( "moonkin_form"       );
  gains_natural_reaction   = get_gain( "natural_reaction"   );
  gains_omen_of_clarity    = get_gain( "omen_of_clarity"    );
  gains_primal_fury        = get_gain( "primal_fury"        );
  gains_tigers_fury        = get_gain( "tigers_fury"        );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  procs_combo_points_wasted = get_proc( "combo_points_wasted" );
  procs_fury_swipes         = get_proc( "fury_swipes"         );
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

  rng_fury_swipes = get_rng( "fury_swipes" );
  rng_nom_nom_nom = get_rng( "nom_nom_nom" );
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
        action_list_str += "/mangle_bear,mangle<=0.5";
        action_list_str += "/lacerate,lacerate<=6.9";           // This seems to be the sweet spot to prevent Lacerate falling off.
        if ( talent_berserk -> rank() ) action_list_str+="/berserk_bear";
        action_list_str += use_str;
        action_list_str += "/mangle_bear";
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
        if ( talent_berserk -> rank() )action_list_str += "/berserk_cat,if=energy>=80&energy<=90&!buff.tigers_fury.up";
        action_list_str += "/savage_roar,if=buff.combo_points.stack>=1&buff.savage_roar.remains<=1";
        action_list_str += "/rip,if=buff.combo_points.stack>=5&target.time_to_die>=6";
        action_list_str += "/savage_roar,if=buff.combo_points.stack>=3&target.time_to_die>=9&buff.savage_roar.remains<=8&dot.rip.remains-buff.savage_roar.remains>=-3";
        action_list_str += "/ferocious_bite,if=target.time_to_die<=6&buff.combo_points.stack>=5";
        action_list_str += "/ferocious_bite,if=target.time_to_die<=1&buff.combo_points.stack>=4";
        action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.remains>=8&buff.savage_roar.remains>=11";
        if ( glyphs.shred )action_list_str += "/shred,extend_rip=1,if=dot.rip.remains<=4";
        action_list_str += "/mangle_cat,mangle<=1";
        action_list_str += "/rake,if=target.time_to_die>=9";
        action_list_str += "/shred,if=(buff.combo_points.stack<=4|dot.rip.remains>=0.8)&dot.rake.remains>=0.4&(energy>=80|buff.omen_of_clarity.react|dot.rip.remains<=2|buff.berserk.up|cooldown.tigers_fury.remains<=3)";
        action_list_str += "/shred,if=target.time_to_die<=9";
        action_list_str += "/shred,if=buff.combo_points.stack<=0&buff.savage_roar.remains<=2";
      }
    }
    else
    {
      action_list_str += "flask,type=frost_wyrm/food,type=fish_feast/mark_of_the_wild";
      if ( talent_moonkin_form -> rank() ) action_list_str += "/moonkin_form";
      action_list_str += "/snapshot_stats";
      action_list_str += "/speed_potion,if=!in_combat|(buff.bloodlust.react&buff.lunar_eclipse.react)|(target.time_to_die<=60&buff.lunar_eclipse.react)";
      if ( talent_typhoon -> rank() ) action_list_str += "/typhoon,moving=1";
      action_list_str += "/innervate,trigger=-2000";
      if ( talent_force_of_nature -> rank() )
      {
        action_list_str+="/treants,time>=5";
      }
      if ( talent_starfall -> rank() ) action_list_str+="/starfall,if=!eclipse";
      action_list_str += "/starfire,if=buff.t8_4pc_caster.up";
      action_list_str += "/moonfire,if=!ticking&!eclipse";
      action_list_str += "/insect_swarm,if=!ticking&!eclipse";
      action_list_str += "/wrath,if=trigger_lunar";
      action_list_str += use_str;
      action_list_str += "/starfire,if=buff.lunar_eclipse.react&(buff.lunar_eclipse.remains>cast_time)";
      action_list_str += "/wrath,if=buff.solar_eclipse.react&(buff.solar_eclipse.remains>cast_time)";
      action_list_str += "/starfire";
    }
    action_list_default = 1;
  }

  player_t::init_actions();

}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  eclipse_bar_value = 0;
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
  else if ( resource_type == RESOURCE_MANA )
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
  if ( primary_resource() != RESOURCE_ENERGY ) return 0.1;

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

    if ( talent_predatory_strikes -> rank() )
    {
      ap += level * talent_predatory_strikes -> rank() * 0.5;
      weapon_ap *= 1 + util_t::talent_rank( talent_predatory_strikes -> rank(), 3, 0.07, 0.14, 0.20 );
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
    multiplier *= 1.0 + talent_heart_of_the_wild -> rank() * 0.03;
  }
  if ( primary_tree() == TREE_FERAL )
  {
    // Aggression
    // Increases your attack power by 25%.
    multiplier *= 1.25;
    
  }
  return multiplier;
}

// druid_t::composite_attack_crit ==========================================

double druid_t::composite_attack_crit() SC_CONST
{
  double c = player_t::composite_attack_crit();

  if ( buffs_cat_form -> check() )
  {
    c += 0.04 * talent_master_shapeshifter -> rank();
  }

  return floor( c * 10000.0 ) / 10000.0;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit() SC_CONST
{
  double hit = player_t::composite_spell_hit();
  // BoP does not convert base spirit into hit!
  hit += ( spirit() - attribute_base[ ATTR_SPIRIT ] )* ( talent_balance_of_power -> rank() / 2.0 ) / rating.spell_hit;

  return floor( hit * 10000.0 ) / 10000.0;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit() SC_CONST
{
  double crit = player_t::composite_spell_crit();
  crit += talent_natures_majesty -> rank() * 0.02;
  return floor( crit * 10000.0 ) / 10000.0;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA )
    if ( buffs_bear_form -> check() )
      m *= 1.0 + 0.03 * talent_heart_of_the_wild -> rank();

  return m;
}

// druid_t::composite_tank_crit =============================================

double druid_t::composite_tank_crit( int school ) SC_CONST
{
  double c = player_t::composite_tank_crit( school );

  if ( school == SCHOOL_PHYSICAL )
  {
    c -= 0.02 * talent_thick_hide -> rank();
    if ( c < 0 ) c = 0;
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
      eclipse_expr_t( action_t* a) : action_expr_t( a, "eclipse", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> cast_druid() -> eclipse_bar_value; return TOK_NUM; }
    };
    return new eclipse_expr_t( a );
  }
  return player_t::create_expression( a, name_str );
}

// druid_t::get_talent_trees ===============================================

std::vector<talent_translation_t>& druid_t::get_talent_list()
{
  talent_list.clear();
  return talent_list;
}

// druid_t::get_options ================================================

std::vector<option_t>& druid_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t druid_options[] =
    {
      // @option_doc loc=player/druid/talents title="Talents"
      // @option_doc loc=player/druid/misc title="Misc"
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
  return talent_moonkin_form -> rank() ? ROLE_SPELL : ROLE_ATTACK;
}

// druid_t::primary_resource ================================================

int druid_t::primary_resource() SC_CONST
{
  if ( talent_moonkin_form -> rank() ) return RESOURCE_MANA;
  if ( tank > 0 ) return RESOURCE_RAGE;
  return RESOURCE_ENERGY;
}

// druid_t::primary_tree ====================================================

talent_tree_type druid_t::primary_tree() SC_CONST
{
  if ( level > 9 && level <= 69 )
  {
    if ( talent_tab_points[ TREE_BALANCE     ] > 0 ) return TREE_BALANCE;
    if ( talent_tab_points[ TREE_FERAL       ] > 0 ) return TREE_FERAL;
    if ( talent_tab_points[ TREE_RESTORATION ] > 0 ) return TREE_RESTORATION;
    return TREE_NONE;
  }
  else
  {
    if ( talent_tab_points[ TREE_BALANCE     ] > 30 ) return TREE_BALANCE;
    if ( talent_tab_points[ TREE_FERAL       ] > 30 ) return TREE_FERAL;
    if ( talent_tab_points[ TREE_RESTORATION ] > 30 ) return TREE_RESTORATION;
    return TREE_NONE;
  }
  return TREE_NONE;
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
    if ( talent_natural_reaction -> rank() )
    {
      resource_gain( RESOURCE_RAGE, talent_natural_reaction -> rank(), gains_natural_reaction );
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
                                  race_type r )
{
  return new druid_t( sim, name, r );
}

// player_t::druid_init =====================================================

void player_t::druid_init( sim_t* sim )
{
  sim -> auras.leader_of_the_pack = new aura_t( sim, "leader_of_the_pack" );
  sim -> auras.moonkin            = new aura_t( sim, "moonkin" );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.innervate        = new buff_t( p, "innervate",        1, 10.0 );
    p -> buffs.mark_of_the_wild = new buff_t( p, "mark_of_the_wild", 1 );
  }

  target_t* t = sim -> target;
  t -> debuffs.earth_and_moon       = new debuff_t( sim, "earth_and_moon",       1,  12.0 );
  t -> debuffs.faerie_fire          = new debuff_t( sim, "faerie_fire",          3, 300.0 );
  t -> debuffs.infected_wounds      = new debuff_t( sim, "infected_wounds",      1,  12.0 );
  t -> debuffs.mangle               = new debuff_t( sim, "mangle",               1,  60.0 );
}

// player_t::druid_combat_begin =============================================

void player_t::druid_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.leader_of_the_pack     ) sim -> auras.leader_of_the_pack -> override();
  if ( sim -> overrides.moonkin_aura           ) sim -> auras.moonkin            -> override();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.mark_of_the_wild ) p -> buffs.mark_of_the_wild -> override();
    }
  }

  target_t* t = sim -> target;
  if ( sim -> overrides.earth_and_moon       ) t -> debuffs.earth_and_moon       -> override( 1, 13 );
  if ( sim -> overrides.faerie_fire          ) t -> debuffs.faerie_fire          -> override( 3 );
  if ( sim -> overrides.infected_wounds      ) t -> debuffs.infected_wounds      -> override();
  if ( sim -> overrides.mangle               ) t -> debuffs.mangle               -> override();
}

