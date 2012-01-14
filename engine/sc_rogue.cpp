// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Custom Combo Point Impl.
// ==========================================================================

#define COMBO_POINTS_MAX 5

static inline int clamp( int x, int low, int high )
{
  return x < low ? low : ( x > high ? high : x );
}

struct combo_points_t
{
  sim_t* sim;
  player_t* player;

  proc_t* proc;
  proc_t* wasted;

  int count;

  combo_points_t( player_t* p ) :
    sim( p -> sim ), player( p ), proc( 0 ), wasted( 0 ), count( 0 )
  {
    proc   = player -> get_proc( "combo_points" );
    wasted = player -> get_proc( "combo_points_wasted" );
  }

  void add( int num, const char* action = 0 )
  {
    int actual_num = clamp( num, 0, COMBO_POINTS_MAX - count );
    int overflow   = num - actual_num;

    // we count all combo points gained in the proc
    for ( int i = 0; i < num; i++ )
      proc -> occur();

    // add actual combo points
    if ( actual_num > 0 )
      count += actual_num;

    // count wasted combo points
    if ( overflow > 0 )
    {
      for ( int i = 0; i < overflow; i++ )
        wasted -> occur();
    }

    if ( sim -> log )
    {
      if ( action )
        log_t::output( sim, "%s gains %d (%d) combo_points from %s (%d)",
                       player -> name(), actual_num, num, action, count );
      else
        log_t::output( sim, "%s gains %d (%d) combo_points (%d)",
                       player -> name(), actual_num, num, count );
    }
  }

  void add( int num, const spell_id_t* spell )
  {
    add( num, spell -> token().c_str() );
  }

  void clear( const char* action = 0 )
  {
    if ( sim -> log )
    {
      if ( action )
        log_t::output( sim, "%s spends %d combo_points on %s",
                       player -> name(), count, action );
      else
        log_t::output( sim, "%s loses %d combo_points",
                       player -> name(), count );
    }

    count = 0;
  }

  double rank( double* cp_list ) const
  {
    assert( count > 0 );
    return cp_list[ count - 1 ];
  }

  double rank( double cp1, double cp2, double cp3, double cp4, double cp5 ) const
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return rank( cp_list );
  }
};

// ==========================================================================
// Rogue
// ==========================================================================

// ==========================================================================
// Todo for Cata :
//  New Ability: Redirect, doable once we allow debuffs on multiple targets
//  Review: Ability Scaling
//  Review: Bandit's Guile
//  Review: Energy Regen (how Adrenaline rush stacks with Blade Flurry / haste)
// ==========================================================================

enum poison_type_t { POISON_NONE=0, DEADLY_POISON, INSTANT_POISON, WOUND_POISON };

struct rogue_targetdata_t : public targetdata_t
{
  dot_t* dots_rupture;
  dot_t* dots_hemorrhage;

  buff_t* debuffs_poison_doses;

  combo_points_t* combo_points;

  rogue_targetdata_t( player_t* source, player_t* target )
    : targetdata_t( source, target )
  {
    combo_points = new combo_points_t( this->target );

    debuffs_poison_doses = add_aura( new buff_t( this, "poison_doses",  5  ) );
  }

  virtual void reset()
  {
    targetdata_t::reset();

    combo_points->clear();
  }

  ~rogue_targetdata_t()
  {
    delete combo_points;
  }
};

void register_rogue_targetdata( sim_t* sim )
{
  player_type t = ROGUE;
  typedef rogue_targetdata_t type;

  REGISTER_DOT( rupture );
  REGISTER_DOT( hemorrhage );

  REGISTER_DEBUFF( poison_doses );
}

struct rogue_t : public player_t
{
  // Active
  action_t* active_deadly_poison;
  action_t* active_instant_poison;
  action_t* active_main_gauche;
  action_t* active_tier12_2pc_melee;
  action_t* active_venomous_wound;
  action_t* active_wound_poison;

  // Buffs
  buff_t* buffs_adrenaline_rush;
  buff_t* buffs_bandits_guile;
  buff_t* buffs_blade_flurry;
  buff_t* buffs_cold_blood;
  buff_t* buffs_deadly_proc;
  buff_t* buffs_envenom;
  buff_t* buffs_find_weakness;
  buff_t* buffs_killing_spree;
  buff_t* buffs_master_of_subtlety;
  buff_t* buffs_overkill;
  buff_t* buffs_recuperate;
  buff_t* buffs_revealing_strike;
  buff_t* buffs_shadow_dance;
  buff_t* buffs_shadowstep;
  buff_t* buffs_shiv;
  buff_t* buffs_slice_and_dice;
  buff_t* buffs_stealthed;
  buff_t* buffs_tier11_4pc;
  buff_t* buffs_tier13_2pc;
  buff_t* buffs_tot_trigger;
  buff_t* buffs_vanish;
  buff_t* buffs_vendetta;

  stat_buff_t* buffs_tier12_4pc_crit;
  stat_buff_t* buffs_tier12_4pc_haste;
  stat_buff_t* buffs_tier12_4pc_mastery;

  // Legendary buffs
  buff_t* buffs_fof_fod; // Fangs of the Destroyer
  buff_t* buffs_fof_p1; // Phase 1
  buff_t* buffs_fof_p2;
  buff_t* buffs_fof_p3;

  // Cooldowns
  cooldown_t* cooldowns_adrenaline_rush;
  cooldown_t* cooldowns_honor_among_thieves;
  cooldown_t* cooldowns_killing_spree;
  cooldown_t* cooldowns_seal_fate;

  // Expirations
  struct expirations_t_
  {
    event_t* wound_poison;

    void reset() { memset( ( void* ) this, 0x00, sizeof( expirations_t_ ) ); }
    expirations_t_() { reset(); }
  };
  expirations_t_ expirations_;

  // Gains
  gain_t* gains_adrenaline_rush;
  gain_t* gains_backstab_glyph;
  gain_t* gains_cold_blood;
  gain_t* gains_combat_potency;
  gain_t* gains_energy_refund;
  gain_t* gains_murderous_intent;
  gain_t* gains_overkill;
  gain_t* gains_recuperate;
  gain_t* gains_relentless_strikes;
  gain_t* gains_venomous_vim;

  // Procs
  proc_t* procs_deadly_poison;
  proc_t* procs_honor_among_thieves;
  proc_t* procs_main_gauche;
  proc_t* procs_munched_tier12_2pc_melee;
  proc_t* procs_rolled_tier12_2pc_melee;
  proc_t* procs_ruthlessness;
  proc_t* procs_seal_fate;
  proc_t* procs_serrated_blades;
  proc_t* procs_sinister_strike_glyph;
  proc_t* procs_venomous_wounds;

  // Up-Times
  benefit_t* uptimes_bandits_guile[ 3 ];
  benefit_t* uptimes_energy_cap;
  benefit_t* uptimes_poisoned;

  // Random Number Generation
  rng_t* rng_bandits_guile;
  rng_t* rng_combat_potency;
  rng_t* rng_cut_to_the_chase;
  rng_t* rng_deadly_poison;
  rng_t* rng_hat_interval;
  rng_t* rng_honor_among_thieves;
  rng_t* rng_improved_expose_armor;
  rng_t* rng_initiative;
  rng_t* rng_instant_poison;
  rng_t* rng_main_gauche;
  rng_t* rng_relentless_strikes;
  rng_t* rng_ruthlessness;
  rng_t* rng_seal_fate;
  rng_t* rng_serrated_blades;
  rng_t* rng_sinister_strike_glyph;
  rng_t* rng_venomous_wounds;
  rng_t* rng_vile_poisons;
  rng_t* rng_wound_poison;

  // Spec passives
  passive_spell_t* spec_ambidexterity;
  passive_spell_t* spec_assassins_resolve;
  passive_spell_t* spec_improved_poisons;
  passive_spell_t* spec_master_of_subtlety;
  passive_spell_t* spec_sinister_calling;
  passive_spell_t* spec_vitality;

  // Spec actives
  active_spell_t* spec_blade_flurry;
  active_spell_t* spec_mutilate;
  active_spell_t* spec_shadowstep;

  // Masteries
  mastery_t* mastery_executioner;
  mastery_t* mastery_main_gauche;
  mastery_t* mastery_potent_poisons;

  // Spell Data
  struct spells_t
  {
    spell_data_t* tier13_2pc;
    spell_data_t* tier13_4pc;

    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  };
  spells_t spells;

  // Talents
  struct talents_list_t
  {
    // Assasination
    talent_t* cold_blood;               // done
    talent_t* coup_de_grace;            // done
    talent_t* cut_to_the_chase;         // done
    talent_t* improved_expose_armor;    // XXX: maybe add proc counting for 1/2 setups
    talent_t* lethality;                // done
    talent_t* master_poisoner;          // done
    talent_t* murderous_intent;         // done
    talent_t* overkill;                 // done
    talent_t* puncturing_wounds;        // done
    talent_t* quickening;               // XXX: there is no run speed in simc
    talent_t* ruthlessness;             // done
    talent_t* seal_fate;                // done
    talent_t* vendetta;                 // done
    talent_t* venomous_wounds;          // done: energy return on target death is not implemented (as we only ever simulate 1 target till it dies)
    talent_t* vile_poisons;             // done

    // Combat
    talent_t* adrenaline_rush;          // done
    talent_t* aggression;               // done
    talent_t* bandits_guile;            // done XXX incomplete: it updates damage 'in real time' (so rupture ticks for example get boosted as you build it up)
    talent_t* combat_potency;           // done
    talent_t* improved_sinister_strike; // done
    talent_t* improved_slice_and_dice;  // done
    talent_t* killing_spree;            // done
    talent_t* lightning_reflexes;       // done
    talent_t* precision;                // done
    talent_t* restless_blades;          // done
    talent_t* revealing_strike;         // XXX: does it get eaten by missed finishers?
    talent_t* savage_combat;            // done

    // Subtlety
    talent_t* elusiveness;                // done
    talent_t* energetic_recovery;         // done
    talent_t* find_weakness;              // done
    talent_t* hemorrhage;                 // done
    talent_t* honor_among_thieves;        // XXX more changes needed?
    talent_t* improved_ambush;            // done
    talent_t* initiative;                 // done
    talent_t* opportunity;                // done
    talent_t* premeditation;              // done
    talent_t* preparation;                // done
    talent_t* relentless_strikes;         // done
    talent_t* sanguinary_vein;            // done
    talent_t* serrated_blades;            // XXX: review to check if rupture refreshes the "right" way
    talent_t* shadow_dance;               // done
    talent_t* slaughter_from_the_shadows; // done
    talent_t* waylay;                     // XXX
  };
  talents_list_t talents;

  struct glyphs_t
  {
    glyph_t* adrenaline_rush;     // done
    glyph_t* ambush;              // XXX
    glyph_t* backstab;            // done
    glyph_t* blade_flurry;        // done
    glyph_t* eviscerate;          // done
    glyph_t* expose_armor;        // done
    glyph_t* feint;               // done
    glyph_t* hemorrhage;          // done
    glyph_t* kick;                // done
    glyph_t* killing_spree;       // done
    glyph_t* mutilate;            // done
    glyph_t* preparation;         // done
    glyph_t* revealing_strike;    // done
    glyph_t* rupture;             // done
    glyph_t* shadow_dance;        // done
    glyph_t* sinister_strike;     // done
    glyph_t* slice_and_dice;      // done
    glyph_t* tricks_of_the_trade; // done
    glyph_t* vendetta;            // done
  };
  glyphs_t glyphs;

  action_callback_t* virtual_hat_callback;

  player_t* tot_target;

  // Options
  timespan_t virtual_hat_interval;
  std::string tricks_of_the_trade_target_str;

  uint32_t fof_p1, fof_p2, fof_p3;
  int last_tier12_4pc;

  rogue_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, ROGUE, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_NIGHT_ELF;

    tree_type[ ROGUE_ASSASSINATION ] = TREE_ASSASSINATION;
    tree_type[ ROGUE_COMBAT        ] = TREE_COMBAT;
    tree_type[ ROGUE_SUBTLETY      ] = TREE_SUBTLETY;

    // Active
    active_deadly_poison    = 0;
    active_instant_poison   = 0;
    active_wound_poison     = 0;
    active_venomous_wound   = 0;
    active_main_gauche      = 0;
    active_tier12_2pc_melee = 0;

    fof_p1 = fof_p2 = fof_p3 = 0;
    last_tier12_4pc = -1;

    virtual_hat_callback = 0;

    // Cooldowns
    cooldowns_honor_among_thieves = get_cooldown( "honor_among_thieves" );
    cooldowns_seal_fate           = get_cooldown( "seal_fate"           );
    cooldowns_adrenaline_rush     = get_cooldown( "adrenaline_rush"     );
    cooldowns_killing_spree       = get_cooldown( "killing_spree"       );

    // Options
    virtual_hat_interval = timespan_t::min;
    tricks_of_the_trade_target_str = "";

    distance = 3;
    default_distance = 3;

    create_talents();
    create_glyphs();
    create_options();
  }

  ~rogue_t()
  {
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new rogue_targetdata_t( source, target );}
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_actions();
  virtual void      init_values();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual void      reset();
  virtual double    energy_regen_per_second() const;
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available() const;
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual action_expr_t* create_expression( action_t* a, const std::string& name_str );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_ENERGY; }
  virtual int       primary_role() const     { return ROLE_ATTACK; }
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );
  virtual void      copy_from( player_t* source );

  virtual double    composite_attribute_multiplier( int attr ) const;
  virtual double    composite_attack_speed() const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
};

namespace // ANONYMOUS NAMESPACE ============================================
{

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_t : public attack_t
{
  /**
   * Simple actions do not invoke attack_t ::execute and ::player_buff methods.
   * It's added for simple buff actions ( like AR, CB etc. ) which may consume resources,
   * require CPs etc, but don't do any damage (by themselves at least).
   */
  bool simple;
  buff_t* m_buff;

  int  requires_weapon;
  int  requires_position;
  bool requires_stealth;
  bool requires_combo_points;
  bool affected_by_killing_spree;

  // we now track how much CPs can an action give us
  int adds_combo_points;

  // we now track how much combo points we spent on an action
  int combo_points_spent;

  rogue_attack_t( const char* n, rogue_t* p, bool special = false ) :
    attack_t( n, p, RESOURCE_ENERGY, SCHOOL_PHYSICAL, TREE_NONE, special ),
    simple( false ), m_buff( 0 ), adds_combo_points( 0 )
  {
    init_rogue_attack_t_();
  }

  rogue_attack_t( const char* n, uint32_t id, rogue_t* p, bool simple = false ) :
    attack_t( n, id, p, 0, true ),
    simple( simple ), m_buff( 0 ), adds_combo_points( 0 )
  {
    init_rogue_attack_t_();
  }

  rogue_attack_t( const char* n, const char* sn, rogue_t* p, bool simple = false ) :
    attack_t( n, sn, p, 0, true ),
    simple( simple ), m_buff( 0 ), adds_combo_points( 0 )
  {
    init_rogue_attack_t_();
  }

  void init_rogue_attack_t_();

  virtual double cost() const;
  virtual void   consume_resource();
  virtual void   execute();
  virtual double calculate_weapon_damage();
  virtual void   player_buff();
  virtual bool   ready();
  virtual void   assess_damage( player_t* t, double amount, int dmg_type, int impact_result );
  virtual double total_multiplier() const;
  virtual double armor() const;

  void add_combo_points();
  void add_trigger_buff( buff_t* buff );
  void trigger_buff();
};

// ==========================================================================
// Rogue Poison
// ==========================================================================

struct rogue_poison_t : public spell_t
{
  rogue_poison_t( const char* n, const uint32_t id, rogue_t* p ) :
    spell_t( n, id, p )
  {
    proc             = true;
    background       = true;
    base_cost        = 0.0;
    trigger_gcd      = timespan_t::zero;
    may_crit         = true;
    tick_may_crit    = true;
    hasted_ticks     = false;

    base_hit         += p -> talents.precision -> base_value( E_APPLY_AURA, A_MOD_SPELL_HIT_CHANCE );
    base_multiplier  *= 1.0 + p -> talents.vile_poisons -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    weapon_multiplier = 0;

    // Poisons are spells that use attack power
    base_spell_power_multiplier  = 0.0;
    base_attack_power_multiplier = 1.0;
  }

  virtual double total_multiplier() const;
};

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ============================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buffs_stealthed -> check() )
  {
    p -> buffs_stealthed -> expire();

    if ( p -> spec_master_of_subtlety -> ok() )
      p -> buffs_master_of_subtlety -> trigger();

    if ( p -> talents.overkill -> rank() )
      p -> buffs_overkill -> trigger();
  }

  if ( p -> buffs_vanish -> check() )
  {
    p -> buffs_vanish -> expire();

    if ( p -> spec_master_of_subtlety -> ok() )
      p -> buffs_master_of_subtlety -> trigger();

    if ( p -> talents.overkill -> rank() )
      p -> buffs_overkill -> trigger();
  }
}

// trigger_apply_poisons ====================================================

static void trigger_apply_poisons( rogue_attack_t* a, weapon_t* we = 0 )
{
  weapon_t* w = ( we ) ? we : a -> weapon;

  if ( ! w )
    return;

  rogue_t* p = a -> player -> cast_rogue();

  if ( w -> buff_type == DEADLY_POISON )
  {
    p -> active_deadly_poison -> weapon = w;
    p -> active_deadly_poison -> execute();
  }
  else if ( w -> buff_type == INSTANT_POISON )
  {
    p -> active_instant_poison -> weapon = w;
    p -> active_instant_poison -> execute();
  }
  else if ( w -> buff_type == WOUND_POISON )
  {
    p -> active_wound_poison -> weapon = w;
    p -> active_wound_poison -> execute();
  }
}

// trigger_bandits_guile ====================================================

static void trigger_bandits_guile( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.bandits_guile -> rank() )
    return;

  int current_stack = p -> buffs_bandits_guile -> check();
  if ( current_stack == p -> buffs_bandits_guile -> max_stack )
    return; // we can't refresh the 15% buff

  if ( p -> rng_bandits_guile -> roll( p -> talents.bandits_guile -> proc_chance() ) )
    p -> buffs_bandits_guile -> trigger( 1, ( ( current_stack + 1 ) / 4 ) * 0.10 );
}

// trigger_other_poisons ====================================================

static void trigger_other_poisons( rogue_t* p, weapon_t* other_w )
{
  if ( ! other_w )
    return;

  if ( other_w -> buff_type == INSTANT_POISON )
  {
    p -> active_instant_poison -> weapon = other_w;
    p -> active_instant_poison -> execute();
  }
  else if ( other_w -> buff_type == WOUND_POISON )
  {
    p -> active_wound_poison -> weapon = other_w;
    p -> active_wound_poison -> execute();
  }
}

// trigger_combat_potency ===================================================

static void trigger_combat_potency( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.combat_potency -> rank() )
    return;

  if ( p -> rng_combat_potency -> roll( p -> talents.combat_potency -> proc_chance() ) )
  {
    // energy gain value is in the proc trigger spell
    p -> resource_gain( RESOURCE_ENERGY,
                        p -> dbc.spell( p -> talents.combat_potency -> effect1().trigger_spell_id() ) -> effect1().resource( RESOURCE_ENERGY ),
                        p -> gains_combat_potency );
  }
}

// trigger_cut_to_the_chase =================================================

static void trigger_cut_to_the_chase( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.cut_to_the_chase -> rank() )
    return;

  if ( ! p -> buffs_slice_and_dice -> check() )
    return;

  if ( p -> rng_cut_to_the_chase -> roll( p -> talents.cut_to_the_chase -> proc_chance() ) )
    p -> buffs_slice_and_dice -> trigger( COMBO_POINTS_MAX );
}

// trigger_initiative =======================================================

static void trigger_initiative( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.initiative -> rank() )
    return;

  if ( p -> rng_initiative -> roll( p -> talents.initiative -> effect1().percent() ) )
  {
    rogue_targetdata_t* td = a -> targetdata() -> cast_rogue();
    td -> combo_points -> add( p -> dbc.spell( p -> talents.initiative -> effect1().trigger_spell_id() ) -> effect1().base_value(),
                               p -> talents.initiative );
  }
}

// trigger_energy_refund ====================================================

static void trigger_energy_refund( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains_energy_refund );
}

// trigger_find_weakness ====================================================

static void trigger_find_weakness( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.find_weakness -> rank() )
    return;

  p -> buffs_find_weakness -> trigger();
};

// trigger_relentless_strikes ===============================================

static void trigger_relentless_strikes( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.relentless_strikes -> rank() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  rogue_targetdata_t* td = a -> targetdata() -> cast_rogue();
  double chance = p -> talents.relentless_strikes -> effect_pp_combo_points( 1 ) / 100.0;
  if ( p -> rng_relentless_strikes -> roll( chance * td -> combo_points -> count ) )
  {
    double gain = p -> dbc.spell( p -> talents.relentless_strikes -> effect1().trigger_spell_id() ) -> effect1().resource( RESOURCE_ENERGY );
    p -> resource_gain( RESOURCE_ENERGY, gain, p -> gains_relentless_strikes );
  }
}

// trigger_restless_blades ==================================================

static void trigger_restless_blades( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.restless_blades -> rank() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  timespan_t reduction = p -> talents.restless_blades -> effect1().time_value() * a -> combo_points_spent;

  p -> cooldowns_adrenaline_rush -> ready -= reduction;
  p -> cooldowns_killing_spree -> ready   -= reduction;
}

// trigger_ruthlessness =====================================================

static void trigger_ruthlessness( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.ruthlessness -> rank() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  if ( p -> rng_ruthlessness -> roll( p -> talents.ruthlessness -> proc_chance() ) )
  {
    rogue_targetdata_t* td = a -> targetdata() -> cast_rogue();
    p -> procs_ruthlessness -> occur();
    td -> combo_points -> add( 1, p -> talents.ruthlessness );
  }
}

// trigger_seal_fate ========================================================

static void trigger_seal_fate( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.seal_fate -> rank() )
    return;

  if ( ! a -> adds_combo_points )
    return;

  // This is to prevent dual-weapon special attacks from triggering a double-proc of Seal Fate
  if ( p -> cooldowns_seal_fate -> remains() > timespan_t::zero )
    return;

  if ( p -> rng_seal_fate -> roll( p -> talents.seal_fate -> proc_chance() ) )
  {
    rogue_targetdata_t* td = a -> targetdata() -> cast_rogue();
    p -> cooldowns_seal_fate -> start( timespan_t::from_millis( 1 ) );
    p -> procs_seal_fate -> occur();
    td -> combo_points -> add( 1, p -> talents.seal_fate );
  }
}

// trigger_main_gauche ======================================================

static void trigger_main_gauche( rogue_attack_t* a )
{
  if ( a -> proc )
    return;

  weapon_t* w = a -> weapon;

  if ( ! w || w -> slot != SLOT_MAIN_HAND )
    return;

  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> mastery_main_gauche -> ok() )
    return;

  double chance = p -> composite_mastery() * p -> mastery_main_gauche -> base_value( E_APPLY_AURA, A_DUMMY, 0 );

  if ( p -> rng_main_gauche -> roll( chance ) )
  {
    if ( ! p -> active_main_gauche )
    {
      struct main_gouche_t : public rogue_attack_t
      {
        main_gouche_t( rogue_t* p ) : rogue_attack_t( "main_gauche", p )
        {
          weapon = &( p -> main_hand_weapon );

          base_dd_min = base_dd_max = 1;
          background      = true;
          trigger_gcd     = timespan_t::zero;
          base_cost       = 0;
          may_glance      = false; // XXX: does not glance
          may_crit        = true;
          normalize_weapon_speed = true; // XXX: it's normalized
          proc = true; // it's proc; therefore it cannot trigger main_gauche for chain-procs

          init();
        }

        virtual void execute()
        {
          rogue_attack_t::execute();
          if ( result_is_hit() )
          {
            trigger_combat_potency( this );
          }
        }
      };
      p -> active_main_gauche = new main_gouche_t( p );
    }

    p -> active_main_gauche -> execute();
    p -> procs_main_gauche  -> occur();
  }
}

// trigger_tricks_of_the_trade ==============================================

static void trigger_tricks_of_the_trade( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> buffs_tot_trigger -> check() )
    return;

  player_t* t = p -> tot_target;

  if ( t )
  {
    timespan_t duration = p -> dbc.spell( 57933 ) -> duration();

    if ( t -> buffs.tricks_of_the_trade -> remains_lt( duration ) )
    {
      double value = p -> dbc.spell( 57933 ) -> effect1().base_value();
      value += p -> glyphs.tricks_of_the_trade -> mod_additive( P_EFFECT_1 );

      t -> buffs.tricks_of_the_trade -> buff_duration = duration;
      t -> buffs.tricks_of_the_trade -> trigger( 1, value / 100.0 );
    }

    p -> buffs_tot_trigger -> expire();
  }
}

// trigger_venomous_wounds ==================================================

static void trigger_venomous_wounds( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();
  rogue_targetdata_t* td = a -> targetdata() -> cast_rogue();

  if ( ! p -> talents.venomous_wounds -> rank() )
    return;

  // we currently only check for presence of deadly poison
  if ( ! td -> debuffs_poison_doses -> check() )
    return;

  if ( p -> rng_venomous_wounds -> roll( p -> talents.venomous_wounds -> proc_chance() ) )
  {
    if ( ! p -> active_venomous_wound )
    {
      struct venomous_wound_t : public rogue_poison_t
      {
        venomous_wound_t( rogue_t* p ) :
          rogue_poison_t( "venomous_wound", 79136, p )
        {
          background       = true;
          proc             = true;
          direct_power_mod = extra_coeff();
          init();
        }
      };
      p -> active_venomous_wound = new venomous_wound_t( p );
    }

    p -> procs_venomous_wounds -> occur();
    p -> active_venomous_wound -> execute();

    // Venomous Vim (51637) lists only 2 energy for some reason
    p -> resource_gain( RESOURCE_ENERGY, 10, p -> gains_venomous_vim );
  }
}

// trigger_tier12_2pc_melee =================================================

static void trigger_tier12_2pc_melee( attack_t* s, double dmg )
{
  if ( s -> school != SCHOOL_PHYSICAL ) return;

  rogue_t* p = s -> player -> cast_rogue();
  sim_t* sim = s -> sim;

  if ( ! p -> set_bonus.tier12_2pc_melee() ) return;

  struct burning_wounds_t : public rogue_attack_t
  {
    burning_wounds_t( rogue_t* player ) :
      rogue_attack_t( "burning_wounds", 99173, player )
    {
      background    = true;
      proc          = true;
      may_resist    = true;
      tick_may_crit = false;
      hasted_ticks  = false;
      dot_behavior  = DOT_REFRESH;
      init();
    }
    virtual void impact( player_t* t, int impact_result, double total_dot_dmg )
    {
      rogue_attack_t::impact( t, impact_result, 0 );

      base_td = total_dot_dmg / dot() -> num_ticks;
    }
    virtual timespan_t travel_time()
    {
      return sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay );
    }
    virtual void target_debuff( player_t* /* t */ , int /* dmg_type */ )
    {
      target_multiplier            = 1.0;
      target_hit                   = 0;
      target_crit                  = 0;
      target_attack_power          = 0;
      target_spell_power           = 0;
      target_penetration           = 0;
      target_dd_adder              = 0;
      if ( sim -> debug )
        log_t::output( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                       name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
    }
    virtual double total_td_multiplier() const { return 1.0; }
  };

  double total_dot_dmg = dmg * p -> dbc.spell( 99173 ) -> effect1().percent();

  if ( ! p -> active_tier12_2pc_melee ) p -> active_tier12_2pc_melee = new burning_wounds_t( p );

  dot_t* dot = p -> active_tier12_2pc_melee -> dot();

  if ( dot -> ticking )
  {
    total_dot_dmg += p -> active_tier12_2pc_melee -> base_td * dot -> ticks();
  }

  if ( p -> dbc.spell( 99173 )  -> duration() + sim -> aura_delay < dot -> remains() )
  {
    if ( sim -> log ) log_t::output( sim, "Player %s munches Burning Wounds due to Max Duration.", p -> name() );
    p -> procs_munched_tier12_2pc_melee -> occur();
    return;
  }

  if ( p -> active_tier12_2pc_melee -> travel_event )
  {
    // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
    if ( sim -> log ) log_t::output( sim, "Player %s munches previous Burning Wounds due to Aura Delay.", p -> name() );
    p -> procs_munched_tier12_2pc_melee -> occur();
  }

  p -> active_tier12_2pc_melee -> direct_dmg = total_dot_dmg;
  p -> active_tier12_2pc_melee -> result = RESULT_HIT;
  p -> active_tier12_2pc_melee -> schedule_travel( s -> target );

  if ( p -> active_tier12_2pc_melee -> travel_event && dot -> ticking )
  {
    if ( dot -> tick_event -> occurs() < p -> active_tier12_2pc_melee -> travel_event -> occurs() )
    {
      // Burning Wounds will tick before SPELL_AURA_APPLIED occurs, which means that the current Burning Wounds will
      // both tick -and- get rolled into the next Burning Wounds.
      if ( sim -> log ) log_t::output( sim, "Player %s rolls Burning Wounds.", p -> name() );
      p -> procs_rolled_tier12_2pc_melee -> occur();
    }
  }
}

// trigger_tier12_4pc_melee =================================================

static void trigger_tier12_4pc_melee( attack_t* s )
{
  rogue_t* p = s -> player -> cast_rogue();
  sim_t* sim = s -> sim;
  double mult;
  double chance;
  double value = 0.0;

  if ( ! p -> set_bonus.tier12_4pc_melee() ) return;

  if ( p -> buffs_tier12_4pc_haste   -> check() ||
       p -> buffs_tier12_4pc_crit    -> check() ||
       p -> buffs_tier12_4pc_mastery -> check() )
    return;

  chance = sim -> real();
  mult = p -> sets -> set( SET_T12_4PC_MELEE ) -> s_effects[ 0 ] -> percent();

  if ( p -> last_tier12_4pc < 0 )
  {
    if ( chance < 0.3333 )
    {
      p -> last_tier12_4pc = 0;
    }
    else if ( chance < 0.6667 )
    {
      p -> last_tier12_4pc = 1;
    }
    else
    {
      p -> last_tier12_4pc = 2;
    }
  }
  else
  {
    if ( chance < 0.5 )
    {
      p -> last_tier12_4pc--;
      if ( p -> last_tier12_4pc < 0 )
        p -> last_tier12_4pc = 2;
    }
    else
    {
      p -> last_tier12_4pc++;
      if ( p -> last_tier12_4pc > 2 )
        p -> last_tier12_4pc = 0;
    }
  }

  switch ( p -> last_tier12_4pc )
  {
  case 0 :
    value = p -> stats.haste_rating * mult;
    p -> buffs_tier12_4pc_haste   -> trigger( 1, value );
    break;
  case 1:
    value = p -> stats.crit_rating * mult;
    p -> buffs_tier12_4pc_crit    -> trigger( 1, value );
    break;
  case 2:
    value = p -> stats.mastery_rating * mult;
    p -> buffs_tier12_4pc_mastery -> trigger( 1, value );
    break;
  default:
    assert( 0 );
    break;
  }
}

// apply_poison_debuff ======================================================

static void apply_poison_debuff( rogue_t* p, player_t* t )
{
  if ( p -> talents.master_poisoner -> rank() )
    t -> debuffs.master_poisoner -> increment();

  if ( p -> talents.savage_combat -> rank() )
    t -> debuffs.savage_combat -> increment();

  t -> debuffs.poisoned -> increment();
}

// remove_poison_debuff =====================================================

static void remove_poison_debuff( rogue_t* p )
{
  player_t* t = p -> sim -> target;

  if ( p -> talents.master_poisoner -> rank() )
    t -> debuffs.master_poisoner -> decrement();

  if ( p -> talents.savage_combat -> rank() )
    t -> debuffs.savage_combat -> decrement();

  t -> debuffs.poisoned -> decrement();
}

// ==========================================================================
// Attacks
// ==========================================================================

double rogue_attack_t::armor() const
{
  rogue_t* p = player -> cast_rogue();

  double a = attack_t::armor();

  a *= 1.0 - p -> buffs_find_weakness -> value();

  return a;
}

// rogue_attack_t::init_ ====================================================

void rogue_attack_t::init_rogue_attack_t_()
{
  may_crit      = true;
  tick_may_crit = true;
  hasted_ticks  = false;
  affected_by_killing_spree = true;

  // reset some damage related stuff ::parse_data set which we will overwhire anyway in actual spell ctors/executes
  direct_power_mod = 0.0;
  tick_power_mod   = 0.0;

  requires_weapon       = WEAPON_NONE;
  requires_position     = POSITION_NONE;
  requires_stealth      = false;
  requires_combo_points = false;

  if ( player -> dbc.spell( id ) -> effect1().type() == E_ADD_COMBO_POINTS )
    adds_combo_points   = ( int ) player -> dbc.spell( id ) -> effect1().base_value();
  else if ( player -> dbc.spell( id ) -> effect2().type() == E_ADD_COMBO_POINTS )
    adds_combo_points   = ( int ) player -> dbc.spell( id ) -> effect2().base_value();
  else if ( player -> dbc.spell( id ) -> effect3().type() == E_ADD_COMBO_POINTS )
    adds_combo_points   = ( int ) player -> dbc.spell( id ) -> effect3().base_value();

  combo_points_spent    = 0;

  rogue_t* p = player -> cast_rogue();

  base_hit += p -> talents.precision -> effect1().percent();

  base_dd_multiplier *= 1.0 + p -> spec_assassins_resolve -> effect2().percent();
}

// rogue_attack_t::cost =====================================================

double rogue_attack_t::cost() const
{
  double c = attack_t::cost();

  rogue_t* p = player -> cast_rogue();

  if ( c <= 0 )
    return 0;

  if ( p -> set_bonus.tier13_2pc_melee() && p -> buffs_tier13_2pc -> up() )
    c *= 1.0 + p -> spells.tier13_2pc -> effect1().percent();

  return c;
}

// rogue_attack_t::consume_resource =========================================

void rogue_attack_t::consume_resource()
{
  attack_t::consume_resource();

  rogue_t* p = player -> cast_rogue();

  combo_points_spent = 0;

  if ( result_is_hit() )
  {
    trigger_relentless_strikes( this );

    if ( requires_combo_points )
    {
      rogue_targetdata_t* td = targetdata() -> cast_rogue();
      combo_points_spent = td -> combo_points -> count;

      td -> combo_points -> clear( name() );

      if ( p -> buffs_fof_fod -> up() )
        td -> combo_points -> add( 5, "legendary_daggers" );
    }

    trigger_ruthlessness( this );
  }
  else
    trigger_energy_refund( this );
}

// rogue_attack_t::execute ==================================================

void rogue_attack_t::execute()
{
  rogue_t* p = player -> cast_rogue();

  if ( harmful )
    break_stealth( p );

  // simple actions do not invoke any base ::execute()
  // nor any triggers
  if ( simple )
  {
    // if we have a buff attached to the action, trigger it now
    trigger_buff();

    consume_resource();

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    add_combo_points();

    update_ready();
  }
  else
  {
    attack_t::execute();

    if ( result_is_hit() )
    {
      add_combo_points();

      if ( result == RESULT_CRIT )
        trigger_seal_fate( this );

      // attached buff get's triggered only on successfull hit
      trigger_buff();

      if ( may_crit )  // Rupture and Garrote can't proc MG, issue 581
        trigger_main_gauche( this );

      trigger_apply_poisons( this );
      trigger_tricks_of_the_trade( this );

      if ( may_crit && ( result == RESULT_CRIT ) )
      {
        trigger_tier12_2pc_melee( this, direct_dmg );
      }
    }

    // Prevent non-critting abilities (Rupture, Garrote, Shiv; maybe something else) from eating Cold Blood
    if ( may_crit )
      p -> buffs_cold_blood -> expire();
  }
}

// rogue_attack_t::calculate_weapon_damage ==================================

double rogue_attack_t::calculate_weapon_damage()
{
  double dmg = attack_t::calculate_weapon_damage();

  if ( dmg == 0 ) return 0;

  rogue_t* p = player -> cast_rogue();

  if ( weapon -> slot == SLOT_OFF_HAND && p -> spec_ambidexterity -> ok() )
  {
    dmg *= 1.0 + p -> spec_ambidexterity -> effect1().percent();
  }

  return dmg;
}

// rogue_attack_t::player_buff ==============================================

void rogue_attack_t::player_buff()
{
  attack_t::player_buff();

  rogue_t* p = player -> cast_rogue();

  if ( p -> buffs_cold_blood -> check() )
    player_crit += p -> buffs_cold_blood -> effect1().percent();
}

// rogue_attack_t::total_multiplier =========================================

double rogue_attack_t::total_multiplier() const
{
  // we have to overwrite it because Executioner is additive with talents
  rogue_t* p = player -> cast_rogue();

  double add_mult = 0.0;

  if ( requires_combo_points && p -> mastery_executioner -> ok() )
    add_mult = p -> composite_mastery() * p -> mastery_executioner -> effect_coeff( 1 ) / 100.0;

  double m = 1.0;
  // dynamic damage modifiers:

  // Bandit's Guile (combat) - affects all damage done by rogue, stacks are reset when you strike other target with sinister strike/revealing strike
  m *= 1.0 + p -> buffs_bandits_guile -> value();

  // Killing Spree (combat) - affects all direct damage (but you cannot use specials while it is up)
  // affects deadly poison ticks
  // Exception: DOESN'T affect rupture/garrote ticks
  if ( affected_by_killing_spree )
    m *= 1.0 + p -> buffs_killing_spree -> value();

  // Sanguinary Vein (subtlety) - dynamically increases all damage as long as target is bleeding
  if ( target -> debuffs.bleeding -> check() )
    m *= 1.0 + p -> talents.sanguinary_vein -> effect1().percent();

  // Vendetta (Assassination) - dynamically increases all damage as long as target is affected by Vendetta
  m *= 1.0 + p -> buffs_vendetta -> value();

  return ( base_multiplier + add_mult ) * player_multiplier * target_multiplier * m;
}

// rogue_attack_t::ready() ==================================================

bool rogue_attack_t::ready()
{
  rogue_t* p = player -> cast_rogue();

  if ( requires_combo_points )
  {
    rogue_targetdata_t* td = targetdata() -> cast_rogue();
    if ( ! td -> combo_points -> count )
      return false;
  }

  if ( requires_stealth )
  {
    if ( ! p -> buffs_shadow_dance -> check() &&
         ! p -> buffs_stealthed -> check() &&
         ! p -> buffs_vanish -> check() )
    {
      return false;
    }
  }

  //Killing Spree blocks all rogue actions for duration
  if ( p -> buffs_killing_spree -> check() )
    if ( ( special == true ) && ( proc == false ) )
      return false;

  if ( requires_position != POSITION_NONE )
    if ( p -> position != requires_position )
      return false;

  if ( requires_weapon != WEAPON_NONE )
    if ( ! weapon || weapon -> type != requires_weapon )
      return false;

  return attack_t::ready();
}

// rogue_attack_t::assess_damage ============================================

void rogue_attack_t::assess_damage( player_t* t,
                                    double amount,
                                    int    dmg_type,
                                    int impact_result )
{
  attack_t::assess_damage( t, amount, dmg_type, impact_result );

  /*rogue_t* p = player -> cast_rogue();

  // XXX: review, as not all of the damage is 'flurried' to an additional target
  // dots for example don't as far as I remember
  if ( t -> is_enemy() )
  {
    target_t* q = t -> cast_target();

    if ( p -> buffs_blade_flurry -> up() && q -> adds_nearby )
      attack_t::additional_damage( q, amount, dmg_type, impact_result );
  }*/
}

// rogue_attack_t::add_combo_points =========================================

void rogue_attack_t::add_combo_points()
{
  if ( adds_combo_points > 0 )
  {
    rogue_targetdata_t* td = targetdata() -> cast_rogue();

    td -> combo_points -> add( adds_combo_points, name() );
  }
}

// rogue_attack_t::add_trigger_buff =========================================

void rogue_attack_t::add_trigger_buff( buff_t* buff )
{
  if ( buff )
    m_buff = buff;
}

// rogue_attack_t::trigger_buff =============================================

void rogue_attack_t::trigger_buff()
{
  if ( m_buff )
    m_buff -> trigger();
}

// Melee Attack =============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;

  melee_t( const char* name, rogue_t* p, int sw ) :
    rogue_attack_t( name, p ), sync_weapons( sw )
  {
    base_dd_min = base_dd_max = 1;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero;
    base_cost       = 0;
    may_crit        = true;

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = rogue_attack_t::execute_time();

    if ( ! player -> in_combat )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.2 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );

    return t;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      rogue_t* p = player -> cast_rogue();
      if ( weapon -> slot == SLOT_OFF_HAND )
      {
        trigger_combat_potency( this );
      }
      p -> buffs_tier11_4pc -> trigger();

      p -> buffs_fof_p1 -> trigger();
      p -> buffs_fof_p2 -> trigger();
      if ( ! p -> buffs_fof_fod -> check() )
        p -> buffs_fof_p3 -> trigger();

      // Assuming that you get a 5% chance per buff over 30 to proc Fury of the Destroyer, so that upon reaching 50, you'd trigger it automatically
      if ( p -> buffs_fof_p3 -> check() > 30 )
      {
        if ( p -> buffs_fof_fod -> trigger( 1, -1, ( p -> buffs_fof_p3 -> check() - 30 ) * 0.05 ) )
        {
          rogue_targetdata_t* td = targetdata() -> cast_rogue();
          p -> buffs_fof_p3 -> expire();
          td -> combo_points -> add( 5, "legendary_daggers" );
        }
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public action_t
{
  int sync_weapons;

  auto_attack_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "auto_attack", p ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", p, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    p -> main_hand_attack -> schedule_execute();

    if ( p -> off_hand_attack )
      p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if ( p -> is_moving() )
      return false;

    return ( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_attack_t
{
  adrenaline_rush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "adrenaline_rush", p -> talents.adrenaline_rush -> spell_id(), p, true )
  {
    check_talent( p -> talents.adrenaline_rush -> ok() );

    add_trigger_buff( p -> buffs_adrenaline_rush );

    parse_options( NULL, options_str );
  }
};

// Ambush ===================================================================

struct ambush_t : public rogue_attack_t
{
  ambush_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "ambush", "Ambush", p )
  {
    parse_options( NULL, options_str );

    requires_position      = POSITION_BACK;
    requires_stealth       = true;

    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier   *= 1.447; // It'is in the description.

    base_cost             += p -> talents.slaughter_from_the_shadows -> effect1().resource( RESOURCE_ENERGY );
    base_multiplier       *= 1.0 + ( p -> talents.improved_ambush -> effect2().percent() +
                                     p -> talents.opportunity     -> effect1().percent() );
    base_crit             += p -> talents.improved_ambush -> effect1().percent();
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    if ( result_is_hit() )
    {
      trigger_initiative( this );
      trigger_find_weakness( this );
    }

    p -> buffs_shadowstep -> expire();
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();

    rogue_t* p = player -> cast_rogue();

    player_multiplier *= 1.0 + p -> buffs_shadowstep -> value();
  }
};

// Backstab =================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "backstab", 53, p )
  {
    requires_weapon    = WEAPON_DAGGER;
    requires_position  = POSITION_BACK;

    weapon_multiplier *= 1.0 + p -> spec_sinister_calling -> mod_additive( P_EFFECT_2 );

    base_cost         += p -> talents.slaughter_from_the_shadows -> effect1().resource( RESOURCE_ENERGY );
    base_multiplier   *= 1.0 + ( p -> talents.aggression  -> mod_additive( P_GENERIC ) +
                                 p -> talents.opportunity -> mod_additive( P_GENERIC ) );
    base_crit         += p -> talents.puncturing_wounds -> effect1().percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );

    if ( p -> set_bonus.tier11_2pc_melee() )
      base_crit += .05;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    if ( result_is_hit() )
    {
      if ( p -> glyphs.backstab -> ok() && result == RESULT_CRIT )
      {
        double amount = p -> glyphs.backstab -> base_value( E_APPLY_AURA );
        p -> resource_gain( RESOURCE_ENERGY, amount, p -> gains_backstab_glyph );
      }

      if ( p -> talents.murderous_intent -> rank() )
      {
        // the first effect of the talent is the amount energy gained
        // and the second one is target health percent apparently
        double health_pct = p -> talents.murderous_intent -> effect2().base_value();
        if ( target -> health_percentage() < health_pct )
        {
          double amount = p -> talents.murderous_intent -> effect1().resource( RESOURCE_ENERGY );
          p -> resource_gain( RESOURCE_ENERGY, amount, p -> gains_murderous_intent );
        }
      }
    }
  }
};

// Blade Flurry =============================================================

struct blade_flurry_t : public rogue_attack_t
{
  blade_flurry_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "blade_flurry", p -> spec_blade_flurry -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs_blade_flurry );

    parse_options( NULL, options_str );
  }
};

// Cold Blood ===============================================================

struct cold_blood_t : public rogue_attack_t
{
  cold_blood_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "cold_blood", p -> talents.cold_blood -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs_cold_blood );

    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    p -> resource_gain( RESOURCE_ENERGY, base_value( E_ENERGIZE ), p -> gains_cold_blood );
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  double dose_dmg;

  envenom_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "envenom", 32645, p )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    requires_combo_points = true;

    base_multiplier *= 1.0 + p -> talents.coup_de_grace -> mod_additive( P_GENERIC );

    dose_dmg = effect_average( 1 );

    option_t options[] =
    {
      { "min_doses", OPT_DEPRECATED, NULL },
      { "no_buff",   OPT_DEPRECATED, NULL },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_targetdata_t* td = targetdata() -> cast_rogue();

    p -> buffs_envenom -> trigger( td -> combo_points -> count );

    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      int doses_consumed = std::min( td -> debuffs_poison_doses -> current_stack, combo_points_spent );

      if ( ! p -> talents.master_poisoner -> rank() )
        td -> debuffs_poison_doses -> decrement( doses_consumed );

      if ( ! td -> debuffs_poison_doses -> check() )
        p -> active_deadly_poison -> dot() -> cancel();

      trigger_cut_to_the_chase( this );
      trigger_restless_blades( this );

      p -> buffs_revealing_strike -> expire();
      p -> buffs_tier11_4pc -> expire();
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_targetdata_t* td = targetdata() -> cast_rogue();

    int doses_consumed = std::min( td -> debuffs_poison_doses -> current_stack,
                                   td -> combo_points -> count );

    direct_power_mod = td -> combo_points -> count * 0.09;

    base_dd_min = base_dd_max = doses_consumed * dose_dmg;

    rogue_attack_t::player_buff();

    if ( p -> buffs_revealing_strike -> up() )
      player_multiplier *= 1.0 + p -> buffs_revealing_strike -> value();

    if ( p -> buffs_tier11_4pc -> up() )
      player_crit += 1.0;
  }

  virtual double total_multiplier() const
  {
    // we have to overwrite it because Potent Poisons is additive with talents
    rogue_t* p = player -> cast_rogue();

    double add_mult = 0.0;

    if ( p -> mastery_potent_poisons -> ok() )
      add_mult = p -> composite_mastery() * p -> mastery_potent_poisons -> base_value( E_APPLY_AURA, A_DUMMY );

    double m = 1.0;
    // dynamic damage modifiers:

    // Bandit's Guile (combat) - affects all damage done by rogue, stacks are reset when you strike other target with sinister strike/revealing strike
    if ( p -> buffs_bandits_guile -> check() )
      m *= 1.0 + p -> buffs_bandits_guile -> value();

    // Killing Spree (combat) - affects all direct damage (but you cannot use specials while it is up)
    // affects deadly poison ticks
    // Exception: DOESN'T affect rupture/garrote ticks
    if ( p -> buffs_killing_spree -> check() && affected_by_killing_spree )
      m *= 1.0 + p -> buffs_killing_spree -> value();

    // Sanguinary Vein (subtlety) - dynamically increases all damage as long as target is bleeding
    if ( target -> debuffs.bleeding -> check() )
      m *= 1.0 + p -> talents.sanguinary_vein -> base_value() / 100.0;

    // Vendetta (Assassination) - dynamically increases all damage as long as target is affected by Vendetta
    if ( p -> buffs_vendetta -> check() )
      m *= 1.0 + p -> buffs_vendetta -> value();

    return ( base_multiplier + add_mult ) * player_multiplier * target_multiplier * m;
  }

  virtual bool ready()
  {
    rogue_targetdata_t* td = targetdata() -> cast_rogue();

    // Envenom is not usable when there is no DP on a target
    if ( ! td -> debuffs_poison_doses -> check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Eviscerate ===============================================================

struct eviscerate_t : public rogue_attack_t
{
  double combo_point_dmg_min[ COMBO_POINTS_MAX ];
  double combo_point_dmg_max[ COMBO_POINTS_MAX ];

  eviscerate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "eviscerate", 2098, p )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    requires_combo_points = true;

    base_multiplier *= 1.0 + ( p -> talents.aggression    -> mod_additive( P_GENERIC ) +
                               p -> talents.coup_de_grace -> mod_additive( P_GENERIC ) );
    base_crit       += p -> glyphs.eviscerate -> mod_additive( P_CRIT );

    double min_     = effect_min( 1 );
    double max_     = effect_max( 1 );
    double cp_bonus = effect_bonus( 1 );

    for ( int i = 0; i < COMBO_POINTS_MAX; i++ )
    {
      combo_point_dmg_min[ i ] = min_ + cp_bonus * ( i + 1 );
      combo_point_dmg_max[ i ] = max_ + cp_bonus * ( i + 1 );
    }

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_targetdata_t* td = targetdata() -> cast_rogue();

    base_dd_min = td -> combo_points -> rank( combo_point_dmg_min );
    base_dd_max = td -> combo_points -> rank( combo_point_dmg_max );

    direct_power_mod = 0.091 * td -> combo_points -> count;

    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_cut_to_the_chase( this );
      trigger_restless_blades( this );

      if ( p -> talents.serrated_blades-> rank() &&
           td -> dots_rupture -> ticking )
      {
        double chance = p -> talents.serrated_blades -> base_value() / 100.0 * combo_points_spent;
        if ( p -> rng_serrated_blades -> roll( chance ) )
        {
          td -> dots_rupture -> refresh_duration();
          p -> procs_serrated_blades -> occur();
        }
      }

      p -> buffs_revealing_strike -> expire();
      p -> buffs_tier11_4pc -> expire();
    }
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();

    rogue_t* p = player -> cast_rogue();

    if ( p -> buffs_revealing_strike -> up() )
      player_multiplier *= 1.0 + p -> buffs_revealing_strike -> value();

    if ( p -> buffs_tier11_4pc -> up() )
      player_crit += 1.0;
  }
};

// Expose Armor =============================================================

struct expose_armor_t : public rogue_attack_t
{
  int override_sunder;

  expose_armor_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "expose_armor", 8647, p ), override_sunder( 0 )
  {
    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    requires_combo_points = true;

    option_t options[] =
    {
      { "override_sunder", OPT_BOOL, &override_sunder },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    if ( result_is_hit() )
    {
      timespan_t duration = timespan_t::from_seconds( 10 ) * combo_points_spent;

      duration += p -> glyphs.expose_armor -> mod_additive_duration();

      if ( p -> buffs_revealing_strike -> up() )
        duration *= 1.0 + p -> buffs_revealing_strike -> value();

      if ( target -> debuffs.expose_armor -> remains_lt( duration ) )
      {
        target -> debuffs.expose_armor -> buff_duration = duration;
        target -> debuffs.expose_armor -> trigger( 1, 0.12 );
        target -> debuffs.expose_armor -> source = p;
      }

      if ( p -> talents.improved_expose_armor -> rank() )
      {
        double chance = p -> talents.improved_expose_armor -> rank() * 0.50; // XXX
        rogue_targetdata_t* td = targetdata() -> cast_rogue();
        if ( p -> rng_improved_expose_armor -> roll( chance ) )
          td -> combo_points -> add( combo_points_spent, p -> talents.improved_expose_armor );
      }

      p -> buffs_revealing_strike -> expire();
    }
  };

  virtual bool ready()
  {

    if ( target -> debuffs.expose_armor -> check() )
      return false;

    if ( ! override_sunder )
    {
      if ( target -> debuffs.sunder_armor -> check() ||
           target -> debuffs.faerie_fire -> check() )
      {
        return false;
      }
    }

    return rogue_attack_t::ready();
  }
};

// Fan of Knives ============================================================

struct fan_of_knives_t : public rogue_attack_t
{
  fan_of_knives_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "fan_of_knives", 51723, p )
  {
    parse_options( NULL, options_str );

    aoe    = -1;
    weapon = &( p -> ranged_weapon );

    base_cost += p -> talents.slaughter_from_the_shadows -> effect2().resource( RESOURCE_ENERGY );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    // FIXME: are these independent rolls for each hand?
    if ( p -> rng_vile_poisons -> roll( p -> talents.vile_poisons ->effect3().percent() ) )
    {
      if ( &( p -> main_hand_weapon ) )
        trigger_apply_poisons( this, &( p -> main_hand_weapon ) );
      if ( &( p -> off_hand_weapon ) )
        trigger_apply_poisons( this, &( p -> off_hand_weapon ) );
    }
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();
    if ( p -> ranged_weapon.type != WEAPON_THROWN )
      return false;

    return rogue_attack_t::ready();
  }
};

// Feint ====================================================================

struct feint_t : public rogue_attack_t
{
  feint_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "feint", 1966, p )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    consume_resource();
    update_ready();
    // model threat eventually......
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_attack_t
{
  garrote_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "garrote", 703, p )
  {
    affected_by_killing_spree = false;

    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    // to stop seal fate procs
    may_crit          = false;

    requires_position = POSITION_BACK;
    requires_stealth  = true;

    tick_power_mod    = .07;

    base_multiplier  *= 1.0 + p -> talents.opportunity -> mod_additive( P_TICK_DAMAGE );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    if ( result_is_hit() )
    {
      trigger_initiative( this );
      trigger_find_weakness( this );
    }

    p -> buffs_shadowstep -> expire();
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();

    rogue_t* p = player -> cast_rogue();

    if ( p -> buffs_shadowstep -> check() )
      player_multiplier *= 1.0 + p -> buffs_shadowstep -> value();
  }

  virtual void tick( dot_t* d )
  {
    rogue_attack_t::tick( d );

    trigger_venomous_wounds( this );
  }
};

// Hemorrhage ===============================================================

struct hemorrhage_t : public rogue_attack_t
{
  hemorrhage_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "hemorrhage", p -> talents.hemorrhage -> spell_id(), p )
  {
    // It does not have CP gain effect in spell dbc for soem reason
    adds_combo_points = 1;

    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier *= 1.45; // number taken from spell description

    weapon_multiplier *= 1.0 + p -> spec_sinister_calling -> mod_additive( P_EFFECT_2 );

    base_cost       += p -> talents.slaughter_from_the_shadows -> effect2().resource( RESOURCE_ENERGY );
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );

    parse_options( NULL, options_str );

    if ( p -> glyphs.hemorrhage -> ok() )
    {
      num_ticks = 8;
      base_tick_time = timespan_t::from_seconds( 3.0 );
      dot_behavior = DOT_REFRESH;
    }
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    if ( result_is_hit() )
    {
      target -> debuffs.hemorrhage -> trigger();
      target -> debuffs.hemorrhage -> source = p;

      if ( p -> glyphs.hemorrhage -> ok() )
      {
        // Dot can crit and double dips in player multipliers
        // Damage is based off actual damage, not normalized to hits like FFB
        // http://elitistjerks.com/f78/t105429-cataclysm_mechanics_testing/p6/#post1796877
        double dot_dmg = calculate_direct_damage() * p -> glyphs.hemorrhage -> base_value() / 100.0;
        base_td = dot_dmg / num_ticks;
      }
    }
  }
};

// Kick =====================================================================

struct kick_t : public rogue_attack_t
{
  kick_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "kick", 1766, p )
  {
    base_dd_min = base_dd_max = 1;
    may_miss = may_resist = may_glance = may_block = may_dodge = may_crit = false;
    base_attack_power_multiplier = 0.0;

    parse_options( NULL, options_str );

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    if ( p -> glyphs.kick -> ok() )
    {
      // All kicks are assumed to interrupt a cast
      cooldown -> duration -= timespan_t::from_seconds( 2.0 ); // + 4 Duration - 6 on Success = -2
    }
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_attack_t
{
  killing_spree_tick_t( rogue_t* p, const char* name ) :
    rogue_attack_t( name, p, true )
  {
    background  = true;
    may_crit    = true;
    direct_tick = true;
    normalize_weapon_speed = true;
    base_dd_min = base_dd_max = 1;
  }
};

struct killing_spree_t : public rogue_attack_t
{
  attack_t* attack_mh;
  attack_t* attack_oh;

  killing_spree_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "killing_spree", p -> talents.killing_spree -> spell_id(), p ),
    attack_mh( 0 ), attack_oh( 0 )
  {
    add_trigger_buff( p -> buffs_killing_spree );

    num_ticks = 5;
    base_tick_time = effect_period( 1 );

    may_crit   = false;

    parse_options( NULL, options_str );

    attack_mh = new killing_spree_tick_t( p, "killing_spree_mh" );
    attack_mh -> weapon = &( player -> main_hand_weapon );
    add_child( attack_mh );

    if ( player -> off_hand_weapon.type != WEAPON_NONE )
    {
      attack_oh = new killing_spree_tick_t( p, "killing_spree_oh" );
      attack_oh -> weapon = &( player -> off_hand_weapon );
      add_child( attack_oh );
    }
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );

    attack_mh -> execute();

    if ( attack_oh && attack_mh -> result_is_hit() )
    {
      attack_oh -> execute();
    }

    stats -> add_tick( d -> time_to_tick );
  }

  // Killing Spree not modified by haste effects
  virtual double haste() const { return 1.0; }
  virtual double swing_haste() const { return 1.0; }
};

// Mutilate =================================================================

struct mutilate_strike_t : public rogue_attack_t
{
  mutilate_strike_t( rogue_t* p, const char* name ) :
    rogue_attack_t( name, 5374, p )
  {
    background  = true;
    may_miss = may_dodge = may_parry = false;

    base_multiplier  *= 1.0 + p -> talents.opportunity -> mod_additive( P_GENERIC );
    base_crit        += p -> talents.puncturing_wounds -> effect2().percent();
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );

    if ( p -> set_bonus.tier11_2pc_melee() ) base_crit += .05;
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();

    rogue_t* p = player -> cast_rogue();

    p -> uptimes_poisoned -> update( target -> debuffs.poisoned > 0 );
  }
};

struct mutilate_t : public rogue_attack_t
{
  attack_t* mh_strike;
  attack_t* oh_strike;

  mutilate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "mutilate", p -> spec_mutilate -> spell_id(), p ), mh_strike( 0 ), oh_strike( 0 )
  {
    may_crit = false;

    if ( p -> main_hand_weapon.type != WEAPON_DAGGER ||
         p ->  off_hand_weapon.type != WEAPON_DAGGER )
    {
      sim -> errorf( "Player %s attempting to execute Mutilate without two daggers equipped.", p -> name() );
      sim -> cancel();
    }

    base_cost += p -> glyphs.mutilate -> mod_additive( P_RESOURCE_COST );

    parse_options( NULL, options_str );

    mh_strike = new mutilate_strike_t( p, "mutilate_mh" );
    mh_strike -> weapon = &( p -> main_hand_weapon );
    add_child( mh_strike );

    oh_strike = new mutilate_strike_t( p, "mutilate_oh" );
    oh_strike -> weapon = &( p -> off_hand_weapon );
    add_child( oh_strike );
  }

  virtual void execute()
  {
    attack_t::execute();
    if ( result_is_hit() )
    {
      mh_strike -> execute();
      oh_strike -> execute();

      add_combo_points();

      if ( mh_strike -> result == RESULT_CRIT ||
           oh_strike -> result == RESULT_CRIT )
      {
        trigger_seal_fate( this );
      }
    }
    else
    {
      trigger_energy_refund( this );
    }
  }
};

// Premeditation ============================================================

struct premeditation_t : public rogue_attack_t
{
  premeditation_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "premeditation", p -> talents.premeditation -> spell_id(), p, true )
  {
    harmful = false;
    requires_stealth = true;

    parse_options( NULL, options_str );
  }
};

// Recuperate ===============================================================

struct recuperate_t : public rogue_attack_t
{
  recuperate_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "recuperate", 73651, p )
  {
    requires_combo_points = true;
    base_tick_time = effect_period( 1 );
    parse_options( NULL, options_str );
    dot_behavior = DOT_REFRESH;
    harmful = false;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_targetdata_t* td = targetdata() -> cast_rogue();
    num_ticks = 2 * td -> combo_points -> count;
    rogue_attack_t::execute();
    p -> buffs_recuperate -> trigger();
  }

  virtual void tick( dot_t* /* d */ )
  {
    rogue_t* p = player -> cast_rogue();
    if ( p -> talents.energetic_recovery -> rank() )
      p -> resource_gain( RESOURCE_ENERGY, p -> talents.energetic_recovery -> base_value(), p -> gains_recuperate );
  }

  virtual void last_tick( dot_t* d )
  {
    rogue_t* p = player -> cast_rogue();
    p -> buffs_recuperate -> expire();
    rogue_attack_t::last_tick( d );
  }
};

// Revealing Strike =========================================================

struct revealing_strike_t : public rogue_attack_t
{
  revealing_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "revealing_strike", p -> talents.revealing_strike -> spell_id(), p )
  {
    add_trigger_buff( p -> buffs_revealing_strike );

    parse_options( NULL, options_str );

    // Legendary buff increases RS damage
    if ( p -> fof_p1 || p -> fof_p2 || p -> fof_p3 )
      base_multiplier *= 1.45; // FIX ME: once dbc data exists 1.0 + p -> dbc.spell( 110211 ) -> effect1().percent();
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_bandits_guile( this );
    }
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  double combo_point_dmg[ COMBO_POINTS_MAX ];

  rupture_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "rupture", 1943, p )
  {
    affected_by_killing_spree = false;

    // to trigger poisons
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    // to stop seal fate procs
    may_crit              = false;
    requires_combo_points = true;
    dot_behavior          = DOT_REFRESH;

    double base     = effect_average( 1 );
    double cp_bonus = effect_bonus( 1 );

    for ( int i = 0; i < COMBO_POINTS_MAX; i++ )
      combo_point_dmg[ i ] = base + cp_bonus * ( i + 1 );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    // We have to save these values for refreshes by Serrated Baldes, so
    // we simply reset them to zeroes on each execute and check them in ::player_buff.
    tick_power_mod = 0.0;
    base_td        = 0.0;

    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      p -> buffs_revealing_strike -> expire();
      trigger_restless_blades( this );
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    rogue_t* p = player -> cast_rogue();
    if ( result_is_hit( impact_result ) )
      num_ticks = 3 + combo_points_spent + ( int )( p -> glyphs.rupture -> mod_additive_duration() / base_tick_time );
    rogue_attack_t::impact( t, impact_result, travel_dmg );
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();

    // If the values are zeroes, we are called from ::execute and we must update.
    // If the values are initialized, we are called from ::refresh_duration and we don't
    // need to update.
    if ( tick_power_mod == 0.0 && base_td == 0.0 )
    {
      rogue_targetdata_t* td = targetdata() -> cast_rogue();
      tick_power_mod = td -> combo_points -> rank( 0.015, 0.024, 0.030, 0.03428571, 0.0375 );
      base_td = td -> combo_points -> rank( combo_point_dmg );
    }

    rogue_attack_t::player_buff();

    if ( p -> buffs_revealing_strike -> up() )
      player_multiplier *= 1.0 + p -> buffs_revealing_strike -> value();
  }

  virtual void tick( dot_t* d )
  {
    rogue_attack_t::tick( d );

    trigger_venomous_wounds( this );
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_attack_t
{
  shadowstep_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadowstep", p -> spec_shadowstep -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs_shadowstep );

    harmful = false;

    parse_options( NULL, options_str );
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_attack_t
{
  shiv_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shiv", 5938, p )
  {
    weapon = &( p -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE )
      background = true; // Do not allow execution.

    base_cost        += weapon -> swing_time.total_seconds() * 10.0;
    adds_combo_points = 1;
    may_crit          = false;
    base_dd_min = base_dd_max = 1;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    p -> buffs_shiv -> trigger();
    rogue_attack_t::execute();
    p -> buffs_shiv -> expire();
  }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  sinister_strike_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "sinister_strike", 1752, p )
  {
    adds_combo_points = 1; // it has an effect but with no base value :rollseyes:

    base_cost       += p -> talents.improved_sinister_strike -> mod_additive( P_RESOURCE_COST );
    base_multiplier *= 1.0 + ( p -> talents.aggression               -> mod_additive( P_GENERIC ) +
                               p -> talents.improved_sinister_strike -> mod_additive( P_GENERIC ) );
    crit_bonus_multiplier *= 1.0 + p -> talents.lethality -> mod_additive( P_CRIT_DAMAGE );

    if ( p -> set_bonus.tier11_2pc_melee() )
      base_crit += .05;

    // Legendary buff increases SS damage
    if ( p -> fof_p1 || p -> fof_p2 || p -> fof_p3 )
      base_multiplier *= 1.45; // FIX ME: once dbc data exists 1.0 + p -> dbc.spell( 110211 ) -> effect1().percent();

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    rogue_t* p = player -> cast_rogue();

    if ( result_is_hit() )
    {
      trigger_bandits_guile( this );

      if ( p -> rng_sinister_strike_glyph -> roll( p -> glyphs.sinister_strike -> proc_chance() ) )
      {
        rogue_targetdata_t* td = targetdata() -> cast_rogue();
        p -> procs_sinister_strike_glyph -> occur();
        td -> combo_points -> add( 1, p -> glyphs.sinister_strike );
      }
    }
  }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_attack_t
{
  slice_and_dice_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "slice_and_dice", 5171, p )
  {
    requires_combo_points = true;

    harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_targetdata_t* td = targetdata() -> cast_rogue();
    int action_cp = td -> combo_points -> count;

    rogue_attack_t::execute();

    p -> buffs_slice_and_dice -> trigger ( action_cp );

  }
};

// Pool Energy ==============================================================

struct pool_energy_t : public action_t
{
  timespan_t wait;
  int for_next;

  pool_energy_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "pool_energy", p ),
    wait( timespan_t::from_seconds( 0.5 ) ), for_next( 0 )
  {
    option_t options[] =
    {
      { "wait",     OPT_TIMESPAN,  &wait     },
      { "for_next", OPT_BOOL, &for_next },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

  }

  virtual void execute()
  {
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );
  }

  virtual timespan_t gcd() const
  {
    return wait;
  }

  virtual bool ready()
  {
    if ( for_next )
    {
      if ( next -> ready() )
        return false;

      // If the next action in the list would be "ready" if it was not constrained by energy,
      // then this command will pool energy until we have enough.

      player -> resource_current[ RESOURCE_ENERGY ] += 100;

      bool energy_limited = next -> ready();

      player -> resource_current[ RESOURCE_ENERGY ] -= 100;

      if ( ! energy_limited )
        return false;
    }

    return action_t::ready();
  }
};

// Preparation ==============================================================

struct preparation_t : public rogue_attack_t
{
  std::vector<cooldown_t*> cooldown_list;

  preparation_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "preparation", p -> talents.preparation -> spell_id(), p, true )
  {
    harmful = false;

    cooldown_list.push_back( p -> get_cooldown( "shadowstep" ) );
    cooldown_list.push_back( p -> get_cooldown( "vanish"     ) );

    if ( p -> glyphs.preparation -> ok() )
      cooldown_list.push_back( p -> get_cooldown( "kick" ) );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();

    int num_cooldowns = ( int ) cooldown_list.size();
    for ( int i = 0; i < num_cooldowns; i++ )
      cooldown_list[ i ] -> reset();
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_attack_t
{
  shadow_dance_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "shadow_dance", p -> talents.shadow_dance -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs_shadow_dance );

    p -> buffs_shadow_dance -> buff_duration += p -> glyphs.shadow_dance -> mod_additive_duration();
    if ( p -> set_bonus.tier13_4pc_melee() )
      p -> buffs_shadow_dance -> buff_duration += p -> spells.tier13_4pc -> effect1().time_value();

    parse_options( NULL, options_str );
  }
};

// Tricks of the Trade ======================================================

struct tricks_of_the_trade_t : public rogue_attack_t
{
  tricks_of_the_trade_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "tricks_of_the_trade", 57934, p )
  {
    parse_options( NULL, options_str );

    if ( p -> glyphs.tricks_of_the_trade -> ok() )
      base_cost = 0;

    if ( ! p -> tricks_of_the_trade_target_str.empty() )
    {
      target_str = p -> tricks_of_the_trade_target_str;
    }

    if ( target_str.empty() )
    {
      target = p;
    }
    else
    {
      if ( target_str == "self" ) // This is only for backwards compatibility
      {
        target = p;
      }
      else
      {
        player_t* q = sim -> find_player( target_str );

        if ( q )
          target = q;
        else
        {
          sim -> errorf( "%s %s: Unable to locate target '%s'.\n", player -> name(), name(), options_str.c_str() );
          target = p;
        }
      }
    }

    p -> tot_target = target;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    rogue_attack_t::execute();

    trigger_tier12_4pc_melee( this );
    p -> buffs_tier13_2pc -> trigger();
    p -> buffs_tot_trigger -> trigger();
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_attack_t
{
  vanish_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vanish", 1856, p, true )
  {
    add_trigger_buff( p -> buffs_vanish );

    harmful = false;

    cooldown -> duration += p -> talents.elusiveness -> effect1().time_value();

    parse_options( NULL, options_str );
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if ( p -> buffs_stealthed -> check() )
      return false;

    return rogue_attack_t::ready();
  }
};

// Vendetta =================================================================

struct vendetta_t : public rogue_attack_t
{
  vendetta_t( rogue_t* p, const std::string& options_str ) :
    rogue_attack_t( "vendetta", p -> talents.vendetta -> spell_id(), p, true )
  {
    add_trigger_buff( p -> buffs_vendetta );

    harmful = false;

    parse_options( NULL, options_str );
  }
};

// ==========================================================================
// Poisons
// ==========================================================================

// rogue_poison_t::total_multiplier =========================================

double rogue_poison_t::total_multiplier() const
{
  // we have to overwrite it because Potent Poisons is additive with talents
  rogue_t* p = player -> cast_rogue();

  double add_mult = 0.0;

  if ( p -> mastery_potent_poisons -> ok() )
    add_mult = p -> composite_mastery() * p -> mastery_potent_poisons -> base_value( E_APPLY_AURA, A_DUMMY );

  double m = 1.0;
  // dynamic damage modifiers:

  // Bandit's Guile (combat) - affects all damage done by rogue, stacks are reset when you strike other target with sinister strike/revealing strike
  if ( p -> buffs_bandits_guile -> check() )
    m *= 1.0 + p -> buffs_bandits_guile -> value();

  // Killing Spree (combat) - affects all direct damage (but you cannot use specials while it is up)
  // affects deadly poison ticks
  if ( p -> buffs_killing_spree -> check() )
    m *= 1.0 + p -> buffs_killing_spree -> value();

  // Sanguinary Vein (subtlety) - dynamically increases all damage as long as target is bleeding
  if ( target -> debuffs.bleeding -> check() )
    m *= 1.0 + p -> talents.sanguinary_vein -> base_value() / 100.0;

  // Vendetta (Assassination) - dynamically increases all damage as long as target is affected by Vendetta
  if ( p -> buffs_vendetta -> check() )
    m *= 1.0 + p -> buffs_vendetta -> value();

  return ( base_multiplier + add_mult ) * player_multiplier * target_multiplier * m;
}

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  deadly_poison_t( rogue_t* player ) :  rogue_poison_t( "deadly_poison", 2818, player )
  {
    tick_power_mod = extra_coeff();
    may_crit   = false;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int success = p -> buffs_shiv -> check() || p -> buffs_deadly_proc -> check();

    if ( ! success )
    {
      double chance = 0.30;

      chance += p -> spec_improved_poisons -> mod_additive( P_PROC_CHANCE );

      if ( p -> buffs_envenom -> up() )
        chance += p -> buffs_envenom -> base_value( E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 100.0;

      success = p -> rng_deadly_poison -> roll( chance );
    }

    if ( success )
    {
      p -> procs_deadly_poison -> occur();

      player_buff();
      target_debuff( target, DMG_DIRECT );
      calculate_result();

      if ( result_is_hit() )
      {
        rogue_targetdata_t* td = targetdata() -> cast_rogue();
        if ( td -> debuffs_poison_doses -> check() == ( int ) max_stacks() )
        {
          p -> buffs_deadly_proc -> trigger();
          weapon_t* other_w = ( weapon -> slot == SLOT_MAIN_HAND ) ? &( p -> off_hand_weapon ) : &( p -> main_hand_weapon );
          trigger_other_poisons( p, other_w );
          p -> buffs_deadly_proc -> expire();
        }

        td -> debuffs_poison_doses -> trigger();

        if ( sim -> log )
          log_t::output( sim, "%s performs %s (%d)", player -> name(), name(), td -> debuffs_poison_doses -> current_stack );

        if ( dot() -> ticking )
        {
          dot() -> refresh_duration();
        }
        else
        {
          schedule_travel( target );
          apply_poison_debuff( p, target );
        }
      }

      update_ready();
      stats -> add_execute( time_to_execute );
    }
  }

  virtual double calculate_tick_damage()
  {
    rogue_targetdata_t* td = targetdata() -> cast_rogue();
    return rogue_poison_t::calculate_tick_damage() * td -> debuffs_poison_doses -> stack();
  }

  virtual void last_tick( dot_t* d )
  {
    rogue_poison_t::last_tick( d );

    rogue_t* p = player -> cast_rogue();
    rogue_targetdata_t* td = targetdata() -> cast_rogue();

    td -> debuffs_poison_doses -> expire();
    remove_poison_debuff( p );
  }
};

// Instant Poison ===========================================================

struct instant_poison_t : public rogue_poison_t
{
  instant_poison_t( rogue_t* player ) : rogue_poison_t( "instant_poison", 8680, player )
  {
    direct_power_mod = extra_coeff();
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    double chance;

    if ( p -> buffs_deadly_proc -> check() )
    {
      chance = 1.0;
      may_crit = true;
    }
    else if ( p -> buffs_shiv -> check() )
    {
      chance = 1.0;
      may_crit = false;
    }
    else
    {
      double m = 1.0;

      m += p -> spec_improved_poisons -> mod_additive( P_PROC_FREQUENCY );

      if ( p -> buffs_envenom -> up() )
        m += p -> buffs_envenom -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );

      chance = weapon -> proc_chance_on_swing( 8.57 * m );
      may_crit = true;
    }

    if ( p -> rng_instant_poison -> roll( chance ) )
      rogue_poison_t::execute();
  }
};

// Wound Poison =============================================================

struct wound_poison_t : public rogue_poison_t
{
  wound_poison_t( rogue_t* player ) : rogue_poison_t( "wound_poison", 13218, player )
  {
    direct_power_mod = extra_coeff();
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      player_t* target;
      expiration_t( sim_t* sim, rogue_t* p, player_t* t ) : event_t( sim, p ), target( t )
      {
        name = "Wound Poison Expiration";
        apply_poison_debuff( p, target );
        sim -> add_event( this, timespan_t::from_seconds( 15.0 ) );
      }

      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> expirations_.wound_poison = 0;
        remove_poison_debuff( p );
      }
    };

    rogue_t* p = player -> cast_rogue();
    double chance;

    if ( p -> buffs_deadly_proc -> check() )
    {
      chance = 1.0;
      may_crit = true;
    }
    else if ( p -> buffs_shiv -> check() )
    {
      chance = 1.0;
      may_crit = false;
    }
    else
    {
      double PPM = 21.43;
      chance = weapon -> proc_chance_on_swing( PPM );
      may_crit = true;
    }

    if ( p -> rng_wound_poison -> roll( chance ) )
    {
      rogue_poison_t::execute();

      if ( result_is_hit() )
      {
        event_t*& e = p -> expirations_.wound_poison;

        if ( e )
          e -> reschedule( timespan_t::from_seconds( 15.0 ) );
        else
          e = new ( sim ) expiration_t( sim, p, target );
      }
    }
  }
};

// Apply Poison =============================================================

struct apply_poison_t : public action_t
{
  int main_hand_poison;
  int  off_hand_poison;
  int    thrown_poison;

  apply_poison_t( rogue_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "apply_poison", p ),
    main_hand_poison( 0 ), off_hand_poison( 0 ), thrown_poison( 0 )
  {
    std::string main_hand_str;
    std::string  off_hand_str;
    std::string    thrown_str;

    option_t options[] =
    {
      { "main_hand", OPT_STRING, &main_hand_str },
      {  "off_hand", OPT_STRING,  &off_hand_str },
      {    "thrown", OPT_STRING,    &thrown_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( main_hand_str.empty() &&
         off_hand_str.empty() &&
         thrown_str.empty() )
    {
      sim -> errorf( "Player %s: At least one of 'main_hand/off_hand/thrown' must be specified in 'apply_poison'.  Accepted values are 'deadly/instant/wound'.\n", p -> name() );
      sim -> cancel();
    }

    trigger_gcd = timespan_t::zero;

    if ( p -> main_hand_weapon.type != WEAPON_NONE )
    {
      if ( main_hand_str == "deadly"    ) main_hand_poison =     DEADLY_POISON;
      if ( main_hand_str == "instant"   ) main_hand_poison =    INSTANT_POISON;
      if ( main_hand_str == "wound"     ) main_hand_poison =      WOUND_POISON;
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( off_hand_str == "deadly"     ) off_hand_poison =  DEADLY_POISON;
      if ( off_hand_str == "instant"    ) off_hand_poison = INSTANT_POISON;
      if ( off_hand_str == "wound"      ) off_hand_poison =   WOUND_POISON;
    }
    if ( p -> ranged_weapon.type == WEAPON_THROWN )
    {
      if ( thrown_str == "deadly"       ) thrown_poison =  DEADLY_POISON;
      if ( thrown_str == "instant"      ) thrown_poison = INSTANT_POISON;
      if ( thrown_str == "wound"        ) thrown_poison =   WOUND_POISON;
    }

    if ( ! p -> active_deadly_poison    ) p -> active_deadly_poison     = new  deadly_poison_t( p );
    if ( ! p -> active_instant_poison   ) p -> active_instant_poison    = new instant_poison_t( p );
    if ( ! p -> active_wound_poison     ) p -> active_wound_poison      = new   wound_poison_t( p );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    if ( main_hand_poison ) p -> main_hand_weapon.buff_type = main_hand_poison;
    if (  off_hand_poison ) p ->  off_hand_weapon.buff_type =  off_hand_poison;
    if (    thrown_poison ) p ->    ranged_weapon.buff_type =    thrown_poison;
  };

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if ( p -> main_hand_weapon.type != WEAPON_NONE )
      if ( main_hand_poison )
        return( main_hand_poison != p -> main_hand_weapon.buff_type );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      if ( off_hand_poison )
        return( off_hand_poison != p -> off_hand_weapon.buff_type );

    if ( p -> ranged_weapon.type == WEAPON_THROWN )
      if ( thrown_poison )
        return ( thrown_poison != p -> ranged_weapon.buff_type );

    return false;
  }
};

// ==========================================================================
// Stealth
// ==========================================================================

struct stealth_t : public spell_t
{
  bool used;

  stealth_t( rogue_t* p, const std::string& options_str ) :
    spell_t( "stealth", p ), used( false )
  {
    trigger_gcd = timespan_t::zero;
    id = 1784;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffs_stealthed -> trigger();
    used = true;
  }

  virtual bool ready()
  {
    return ! used;
  }

  virtual void reset()
  {
    spell_t::reset();
    used = false;
  }
};

// ==========================================================================
// Buffs
// ==========================================================================

struct adrenaline_rush_buff_t : public buff_t
{
  adrenaline_rush_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "adrenaline_rush" )
  {
    // we track the cooldown in the actual action
    // and because of restless blades have to remove it here
    cooldown -> duration = timespan_t::zero;
    buff_duration += p -> glyphs.adrenaline_rush -> mod_additive_duration();
    if ( p -> set_bonus.tier13_4pc_melee() )
      buff_duration += p -> spells.tier13_4pc -> effect2().time_value();
  }

  virtual bool trigger( int, double, double )
  {
    // we keep haste % as current_value
    return buff_t::trigger( 1, base_value( E_APPLY_AURA, A_319 ) );
  }
};

struct envenom_buff_t : public buff_t
{
  envenom_buff_t( rogue_t* p ) :
    buff_t( p, 32645, "envenom" ) { }

  virtual bool trigger( int cp, double, double )
  {
    timespan_t new_duration = timespan_t::from_seconds( 1.0 ) + timespan_t::from_seconds( cp );

    if ( remains_lt( new_duration ) )
    {
      buff_duration = new_duration;
      return buff_t::trigger();
    }
    else
      return false;
  }
};

struct find_weakness_buff_t : public buff_t
{
  find_weakness_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "find_weakness" )
  {
    if ( ! p -> dbc.spell( id ) )
      return;

    // Duration is specified in the actual debuff (or is it a buff?) placed on the target
    buff_duration = p -> dbc.spell( 91021 ) -> duration();

    init_buff_shared();
  }

  virtual bool trigger( int, double, double )
  {
    return buff_t::trigger( 1, effect1().percent() );
  }
};

struct killing_spree_buff_t : public buff_t
{
  killing_spree_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "killing_spree" )
  {
    // we track the cooldown in the actual action
    // and because of restless blades have to remove it here
    cooldown -> duration = timespan_t::zero;
  }

  virtual bool trigger( int, double, double )
  {
    rogue_t* p = player -> cast_rogue();

    double value = 0.20; // XXX
    value += p -> glyphs.killing_spree -> mod_additive( P_EFFECT_3 ) / 100.0;

    return buff_t::trigger( 1, value );
  }
};

struct master_of_subtlety_buff_t : public buff_t
{
  master_of_subtlety_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "master_of_subtlety" )
  {
    buff_duration = timespan_t::from_seconds( 6.0 );
  }

  virtual bool trigger( int, double, double )
  {
    return buff_t::trigger( 1, effect1().percent() );
  }
};

struct revealing_strike_buff_t : public buff_t
{
  revealing_strike_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "revealing_strike" ) { }

  virtual bool trigger( int, double, double )
  {
    rogue_t* p = player -> cast_rogue();

    double value = base_value( E_APPLY_AURA, A_DUMMY ) / 100.0;
    value += p -> glyphs.revealing_strike -> mod_additive( P_EFFECT_3 ) / 100.0;

    return buff_t::trigger( 1, value );
  }
};

struct shadowstep_buff_t : public buff_t
{
  shadowstep_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "shadowstep" ) { }

  virtual bool trigger( int, double, double )
  {
    return buff_t::trigger( 1, base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) );
  }
};

struct slice_and_dice_buff_t : public buff_t
{
  uint32_t id;

  slice_and_dice_buff_t( rogue_t* p ) :
    buff_t( p, 5171, "slice_and_dice" ), id( 5171 ) { }

  virtual bool trigger( int cp, double, double )
  {
    rogue_t* p = player -> cast_rogue();

    timespan_t new_duration = p -> dbc.spell( id ) -> duration();
    new_duration += timespan_t::from_seconds( 3.0 ) * cp;
    new_duration += p -> glyphs.slice_and_dice -> mod_additive_duration();
    new_duration *= 1.0 + p -> talents.improved_slice_and_dice -> mod_additive( P_DURATION );

    if ( remains_lt( new_duration ) )
    {
      buff_duration = new_duration;
      return buff_t::trigger( 1,
        base_value( E_APPLY_AURA, A_319 ) * ( 1.0 + p -> composite_mastery() * p -> mastery_executioner -> effect_coeff( 1 ) / 100.0 ) );
    }
    else
      return false;
  }
};

struct vendetta_buff_t : public buff_t
{
  vendetta_buff_t( rogue_t* p, uint32_t id ) :
    buff_t( p, id, "vendetta" )
  {
    buff_duration *= 1.0 + p -> glyphs.vendetta -> mod_additive( P_DURATION );
    if ( p -> set_bonus.tier13_4pc_melee() )
      buff_duration += p -> spells.tier13_4pc -> effect3().time_value();
  }

  virtual bool trigger( int, double, double )
  {
    return buff_t::trigger( 1, base_value( E_APPLY_AURA, A_MOD_DAMAGE_FROM_CASTER ) );
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

// rogue_t::composite_attribute_multiplier ==================================

double rogue_t::composite_attribute_multiplier( int attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_AGILITY )
    m *= 1.0 + spec_sinister_calling -> base_value( E_APPLY_AURA, A_MOD_TOTAL_STAT_PERCENTAGE );

  return m;
}

// rogue_t::composite_attack_speed ==========================================

double rogue_t::composite_attack_speed() const
{
  double h = player_t::composite_attack_speed();

  if ( talents.lightning_reflexes -> rank() )
    h *= 1.0 / ( 1.0 + talents.lightning_reflexes -> effect2().percent() );

  if ( buffs_slice_and_dice -> check() )
    h *= 1.0 / ( 1.0 + buffs_slice_and_dice -> value() );

  if ( buffs_adrenaline_rush -> check() )
    h *= 1.0 / ( 1.0 + buffs_adrenaline_rush -> value() );

  return h;
}

// rogue_t::matching_gear_multiplier ========================================

double rogue_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( attr == ATTR_AGILITY )
    return 0.05;

  return 0.0;
}

// rogue_t::composite_attack_power_multiplier ===============================

double rogue_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  m *= 1.0 + talents.savage_combat -> base_value( E_APPLY_AURA, A_MOD_ATTACK_POWER_PCT );

  m *= 1.0 + spec_vitality -> base_value( E_APPLY_AURA, A_MOD_ATTACK_POWER_PCT );

  return m;
}

// rogue_t::composite_player_multiplier =====================================

double rogue_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( buffs_master_of_subtlety -> check() ||
       ( spec_master_of_subtlety -> ok() && ( buffs_stealthed -> check() || buffs_vanish -> check() ) ) )
    m *= 1.0 + buffs_master_of_subtlety -> value();

  return m;
}

// rogue_t::init_actions ====================================================

void rogue_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    if ( level > 80 )
    {
      action_list_str += "flask,type=winds";
      action_list_str += "/food,type=seafood_magnifique_feast";
    }
    else
    {
      action_list_str += "flask,type=endless_rage";
      action_list_str += "/food,type=blackened_dragonfin";
    }
    action_list_str += "/apply_poison,main_hand=instant,off_hand=deadly";
    action_list_str += "/snapshot_stats";
    if ( level > 80 )
    {
      action_list_str += "/tolvir_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<30";
    }
    else
    {
      action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<20";
    }
    action_list_str += "/auto_attack";

    if ( talents.overkill -> rank() || spec_master_of_subtlety -> ok() )
      action_list_str += "/stealth";

    action_list_str += "/kick";

    if ( primary_tree() == TREE_ASSASSINATION )
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_profession_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier12_4pc_melee|set_bonus.tier13_2pc_melee";

      action_list_str += "/garrote";
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.down";
      if ( ! talents.cut_to_the_chase -> rank() )
      {
        action_list_str += "/pool_energy,if=energy<60&buff.slice_and_dice.remains<5";
        action_list_str += "/slice_and_dice,if=combo_points>=3&buff.slice_and_dice.remains<2";
      }
      if ( talents.vendetta -> rank() )
      {
        action_list_str += "/rupture,if=(!ticking|ticks_remain<2)&time<6";
        action_list_str += "/vendetta";
      }
      action_list_str += "/rupture,if=(!ticking|ticks_remain<2)&buff.slice_and_dice.remains>6";
      if ( talents.cold_blood -> rank() )
        action_list_str += "/cold_blood,sync=envenom";
      action_list_str += "/envenom,if=combo_points>=4&buff.envenom.down";
      action_list_str += "/envenom,if=combo_points>=4&energy>90";
      action_list_str += "/envenom,if=combo_points>=2&buff.slice_and_dice.remains<3";
      action_list_str += "/backstab,if=combo_points<5&target.health_pct<35";
      action_list_str += "/mutilate,if=combo_points<4&target.health_pct>=35";
      if ( talents.overkill -> rank() )
        action_list_str += "/vanish,if=time>30&energy>50";
    }
    else if ( primary_tree() == TREE_COMBAT )
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_profession_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier12_4pc_melee|set_bonus.tier13_2pc_melee";

      // TODO: Add Blade Flurry
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.down";
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.remains<2";
      if ( talents.killing_spree -> rank() )
        action_list_str += "/killing_spree,if=energy<35&buff.slice_and_dice.remains>4&buff.adrenaline_rush.down";
      if ( talents.adrenaline_rush -> rank() )
        action_list_str += "/adrenaline_rush,if=energy<35";
      if ( talents.bandits_guile -> rank() )
        action_list_str += "/eviscerate,if=combo_points=5&buff.bandits_guile.stack>=12";
      action_list_str += "/rupture,if=!ticking&combo_points=5&target.time_to_die>10";
      action_list_str += "/eviscerate,if=combo_points=5";
      if ( talents.revealing_strike -> rank() )
        action_list_str += "/revealing_strike,if=combo_points=4&buff.revealing_strike.down";
      action_list_str += "/sinister_strike,if=combo_points<5";
    }
    else if ( primary_tree() == TREE_SUBTLETY )
    {
      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier12_4pc_melee|set_bonus.tier13_2pc_melee";

      if ( talents.shadow_dance -> rank() )
      {
        action_list_str += "/pool_energy,for_next=1";
        action_list_str += "/shadow_dance,if=energy>85&combo_points<5&buff.stealthed.down";
      }

      int num_items = ( int ) items.size();
      int hand_enchant_found = -1;
      int found_item = -1;

      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].use.active() )
        {
          if ( items[ i ].slot == SLOT_HANDS )
          {
            hand_enchant_found = i;
            continue;
          }
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
          if ( found_item < 0 )
          {
            action_list_str += ",if=buff.shadow_dance.up";
            found_item = i;
          }
          else
          {
            action_list_str += ",if=buff.shadow_dance.cooldown_remains>20";
          }
        }
      }
      if ( hand_enchant_found >= 0 )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ hand_enchant_found ].name();
        if ( found_item < 0 )
        {
          action_list_str += ",if=buff.shadow_dance.up";
        }
        else
        {
          action_list_str += ",if=buff.shadow_dance.cooldown_remains>20";
        }
      }

      action_list_str += init_use_profession_actions( ( found_item >= 0 ) ? "" : ",if=buff.shadow_dance.up" );

      action_list_str += init_use_racial_actions( ",if=buff.shadow_dance.up" );

      action_list_str += "/pool_energy,for_next=1";
      action_list_str += "/vanish,if=time>10&energy>60&combo_points<=1&cooldown.shadowstep.remains<=0&!buff.shadow_dance.up&!buff.master_of_subtlety.up&!buff.find_weakness.up";

      action_list_str += "/shadowstep,if=buff.stealthed.up|buff.shadow_dance.up";
      if ( talents.premeditation -> rank() )
        action_list_str += "/premeditation,if=(combo_points<=3&cooldown.honor_among_thieves.remains>1.75)|combo_points<=2";

      action_list_str += "/ambush,if=combo_points<=4";

      if ( talents.preparation -> rank() )
        action_list_str += "/preparation,if=cooldown.vanish.remains>60";

      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.remains<3&combo_points=5";

      action_list_str += "/rupture,if=combo_points=5&!ticking";

      if ( talents.energetic_recovery -> rank() )
        action_list_str += "/recuperate,if=combo_points=5&remains<3";

      action_list_str += "/eviscerate,if=combo_points=5&dot.rupture.remains>1";

      if ( talents.hemorrhage -> rank() )
      {
        action_list_str += "/hemorrhage,if=combo_points<4&dot.hemorrhage.remains<4";
        action_list_str += "/hemorrhage,if=combo_points<5&energy>80&dot.hemorrhage.remains<4";
      }

      action_list_str += "/backstab,if=combo_points<4";
      action_list_str += "/backstab,if=combo_points<5&energy>80";
    }
    else
    {
      action_list_str += init_use_item_actions();

      action_list_str += init_use_racial_actions();

      /* Putting this here for now but there is likely a better place to put it */
      action_list_str += "/tricks_of_the_trade,if=set_bonus.tier12_4pc_melee";

      action_list_str += "/pool_energy,if=energy<60&buff.slice_and_dice.remains<5";
      action_list_str += "/slice_and_dice,if=combo_points>=3&buff.slice_and_dice.remains<2";
      action_list_str += "/sinister_strike,if=combo_points<5";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// rogue_t::create_action  ==================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack"         ) return new auto_attack_t        ( this, options_str );
  if ( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if ( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if ( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if ( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if ( name == "blade_flurry"        ) return new blade_flurry_t       ( this, options_str );
  if ( name == "cold_blood"          ) return new cold_blood_t         ( this, options_str );
  if ( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if ( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if ( name == "expose_armor"        ) return new expose_armor_t       ( this, options_str );
  if ( name == "feint"               ) return new feint_t              ( this, options_str );
  if ( name == "fan_of_knives"       ) return new fan_of_knives_t      ( this, options_str );
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "pool_energy"         ) return new pool_energy_t        ( this, options_str );
  if ( name == "premeditation"       ) return new premeditation_t      ( this, options_str );
  if ( name == "preparation"         ) return new preparation_t        ( this, options_str );
  if ( name == "recuperate"          ) return new recuperate_t         ( this, options_str );
  if ( name == "revealing_strike"    ) return new revealing_strike_t   ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if ( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "vendetta"            ) return new vendetta_t           ( this, options_str );
  if ( name == "tricks_of_the_trade" ) return new tricks_of_the_trade_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::create_expression ===============================================

action_expr_t* rogue_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "combo_points" )
  {
    struct combo_points_expr_t : public action_expr_t
    {
      combo_points_expr_t( action_t* a ) : action_expr_t( a, "combo_points", TOK_NUM ) {}
      virtual int evaluate()
      {
        rogue_targetdata_t* td = action -> targetdata() -> cast_rogue();
        result_num = td -> combo_points -> count;
        return TOK_NUM;
      }
    };
    return new combo_points_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// rogue_t::init_base =======================================================

void rogue_t::init_base()
{
  player_t::init_base();

  base_attack_power = ( level * 2 ) - 20;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 2.0;

  resource_base[ RESOURCE_ENERGY ] = 100 + spec_assassins_resolve -> base_value( E_APPLY_AURA, A_MOD_INCREASE_ENERGY );

  base_energy_regen_per_second = 10 + spec_vitality -> base_value( E_APPLY_AURA, A_MOD_POWER_REGEN_PERCENT ) / 10.0;

  base_gcd = timespan_t::from_seconds( 1.0 );

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;
}

// rogue_t::init_talents ====================================================

void rogue_t::init_talents()
{
  // Talents

  // Assasination
  talents.cold_blood                 = find_talent( "Cold Blood" );
  talents.coup_de_grace              = find_talent( "Coup de Grace" );
  talents.cut_to_the_chase           = find_talent( "Cut to the Chase" );
  talents.improved_expose_armor      = find_talent( "Improved Expose Armor" );
  talents.lethality                  = find_talent( "Lethality" );
  talents.master_poisoner            = find_talent( "Master Poisoner" );
  talents.murderous_intent           = find_talent( "Murderous Intent" );
  talents.overkill                   = find_talent( "Overkill" );
  talents.puncturing_wounds          = find_talent( "Puncturing Wounds" );
  talents.quickening                 = find_talent( "Quickening" );
  talents.ruthlessness               = find_talent( "Ruthlessness" );
  talents.seal_fate                  = find_talent( "Seal Fate" );
  talents.vendetta                   = find_talent( "Vendetta" );
  talents.venomous_wounds            = find_talent( "Venomous Wounds" );
  talents.vile_poisons               = find_talent( "Vile Poisons" );

  // Combat
  talents.adrenaline_rush            = find_talent( "Adrenaline Rush" );
  talents.aggression                 = find_talent( "Aggression" );
  talents.bandits_guile              = find_talent( "Bandit's Guile" );
  talents.combat_potency             = find_talent( "Combat Potency" );
  talents.improved_sinister_strike   = find_talent( "Improved Sinister Strike" );
  talents.improved_slice_and_dice    = find_talent( "Improved Slice and Dice" );
  talents.killing_spree              = find_talent( "Killing Spree" );
  talents.lightning_reflexes         = find_talent( "Lightning Reflexes" );
  talents.precision                  = find_talent( "Precision" );
  talents.restless_blades            = find_talent( "Restless Blades" );
  talents.revealing_strike           = find_talent( "Revealing Strike" );
  talents.savage_combat              = find_talent( "Savage Combat" );

  // Subtlety
  talents.elusiveness                = find_talent( "Elusiveness" );
  talents.energetic_recovery         = find_talent( "Energetic Recovery" );
  talents.find_weakness              = find_talent( "Find Weakness" );
  talents.hemorrhage                 = find_talent( "Hemorrhage" );
  talents.honor_among_thieves        = find_talent( "Honor Among Thieves" );
  talents.improved_ambush            = find_talent( "Improved Ambush" );
  talents.initiative                 = find_talent( "Initiative" );
  talents.opportunity                = find_talent( "Opportunity" );
  talents.premeditation              = find_talent( "Premeditation" );
  talents.preparation                = find_talent( "Preparation" );
  talents.relentless_strikes         = find_talent( "Relentless Strikes" );
  talents.sanguinary_vein            = find_talent( "Sanguinary Vein" );
  talents.serrated_blades            = find_talent( "Serrated Blades" );
  talents.shadow_dance               = find_talent( "Shadow Dance" );
  talents.slaughter_from_the_shadows = find_talent( "Slaughter from the Shadows" );
  talents.waylay                     = find_talent( "Waylay" );

  player_t::init_talents();
}

// rogue_t::init_spells =====================================================

void rogue_t::init_spells()
{
  player_t::init_spells();

  // Spec passives
  spec_improved_poisons   = new passive_spell_t( this, "improved_poisons",   14117 );
  spec_assassins_resolve  = new passive_spell_t( this, "assassins_resolve",  84601 );
  spec_vitality           = new passive_spell_t( this, "vitality",           61329 );
  spec_ambidexterity      = new passive_spell_t( this, "ambidexterity",      13852 );
  spec_master_of_subtlety = new passive_spell_t( this, "master_of_subtlety", 31223 );
  spec_sinister_calling   = new passive_spell_t( this, "sinister_calling",   31220 );

  // Spec Actives
  spec_mutilate           = new active_spell_t( this, "mutilate",            1329 );
  spec_blade_flurry       = new active_spell_t( this, "blade_flurry",        13877 );
  spec_shadowstep         = new active_spell_t( this, "shadowstep",          36554 );

  // Masteries
  mastery_potent_poisons  = new mastery_t( this, "potent_poisons",     76803, TREE_ASSASSINATION );
  mastery_main_gauche     = new mastery_t( this, "main_gauche",        76806, TREE_COMBAT );
  mastery_executioner     = new mastery_t( this, "executioner",        76808, TREE_SUBTLETY );

  spells.tier13_2pc       = spell_data_t::find( 105864, "Tricks of Time", dbc.ptr );
  spells.tier13_4pc       = spell_data_t::find( 105865, "Item - Rogue T13 4P Bonus (Shadow Dance, Adrenaline Rush, and Vendetta)", dbc.ptr );

  glyphs.adrenaline_rush     = find_glyph( "Glyph of Adrenaline Rush"     );
  glyphs.ambush              = find_glyph( "Glyph of Ambush"              );
  glyphs.backstab            = find_glyph( "Glyph of Backstab"            );
  glyphs.blade_flurry        = find_glyph( "Glyph of Blade Flurry"        );
  glyphs.eviscerate          = find_glyph( "Glyph of Eviscerate"          );
  glyphs.expose_armor        = find_glyph( "Glyph of Expose Armor"        );
  glyphs.feint               = find_glyph( "Glyph of Feint"               );
  glyphs.hemorrhage          = find_glyph( "Glyph of Hemorrhage"          );
  glyphs.kick                = find_glyph( "Glyph of Kick"                );
  glyphs.killing_spree       = find_glyph( "Glyph of Killing Spree"       );
  glyphs.mutilate            = find_glyph( "Glyph of Mutilate"            );
  glyphs.preparation         = find_glyph( "Glyph of Preparation"         );
  glyphs.revealing_strike    = find_glyph( "Glyph of Revealing Strike"    );
  glyphs.rupture             = find_glyph( "Glyph of Rupture"             );
  glyphs.shadow_dance        = find_glyph( "Glyph of Shadow Dance"        );
  glyphs.sinister_strike     = find_glyph( "Glyph of Sinister Strike"     );
  glyphs.slice_and_dice      = find_glyph( "Glyph of Slice and Dice"      );
  glyphs.tricks_of_the_trade = find_glyph( "Glyph of Tricks of the Trade" );
  glyphs.vendetta            = find_glyph( "Glyph of Vendetta"            );

  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {     0,     0,  90460,  90473,     0,     0,     0,     0 }, // Tier11
    {     0,     0,  99174,  99175,     0,     0,     0,     0 }, // Tier12
    {     0,     0, 105849, 105865,     0,     0,     0,     0 }, // Tier13
    {     0,     0,      0,      0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// rogue_t::init_gains ======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains_adrenaline_rush    = get_gain( "adrenaline_rush"    );
  gains_backstab_glyph     = get_gain( "backstab_glyph"     );
  gains_cold_blood         = get_gain( "cold_blood"         );
  gains_combat_potency     = get_gain( "combat_potency"     );
  gains_energy_refund      = get_gain( "energy_refund"      );
  gains_murderous_intent   = get_gain( "murderous_intent"   );
  gains_overkill           = get_gain( "overkill"           );
  gains_recuperate         = get_gain( "recuperate"         );
  gains_relentless_strikes = get_gain( "relentless_strikes" );
  gains_venomous_vim       = get_gain( "venomous_vim"       );
}

// rogue_t::init_procs ======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs_deadly_poison            = get_proc( "deadly_poisons"        );
  procs_honor_among_thieves      = get_proc( "honor_among_thieves"   );
  procs_main_gauche              = get_proc( "main_gauche"           );
  procs_ruthlessness             = get_proc( "ruthlessness"          );
  procs_seal_fate                = get_proc( "seal_fate"             );
  procs_serrated_blades          = get_proc( "serrated_blades"       );
  procs_sinister_strike_glyph    = get_proc( "sinister_strike_glyph" );
  procs_venomous_wounds          = get_proc( "venomous_wounds"       );
  procs_munched_tier12_2pc_melee = get_proc( "munched_burning_wounds" );
  procs_rolled_tier12_2pc_melee  = get_proc( "rolled_burning_wounds" );
}

// rogue_t::init_uptimes ====================================================

void rogue_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_bandits_guile[ 0 ] = get_benefit( "shallow_insight" );
  uptimes_bandits_guile[ 1 ] = get_benefit( "moderate_insight" );
  uptimes_bandits_guile[ 2 ] = get_benefit( "deep_insight" );

  uptimes_energy_cap = get_benefit( "energy_cap" );
  uptimes_poisoned   = get_benefit( "poisoned"   );
}

// rogue_t::init_rng ========================================================

void rogue_t::init_rng()
{
  player_t::init_rng();

  rng_bandits_guile         = get_rng( "bandits_guile"         );
  rng_combat_potency        = get_rng( "combat_potency"        );
  rng_cut_to_the_chase      = get_rng( "cut_to_the_chase"      );
  rng_deadly_poison         = get_rng( "deadly_poison"         );
  rng_honor_among_thieves   = get_rng( "honor_among_thieves"   );
  rng_improved_expose_armor = get_rng( "improved_expose_armor" );
  rng_initiative            = get_rng( "initiative"            );
  rng_instant_poison        = get_rng( "instant_poison"        );
  rng_main_gauche           = get_rng( "main_gauche"           );
  rng_relentless_strikes    = get_rng( "relentless_strikes"    );
  rng_ruthlessness          = get_rng( "ruthlessness"          );
  rng_seal_fate             = get_rng( "seal_fate"             );
  rng_serrated_blades       = get_rng( "serrated_blades"       );
  rng_sinister_strike_glyph = get_rng( "sinister_strike_glyph" );
  rng_venomous_wounds       = get_rng( "venomous_wounds"       );
  rng_vile_poisons          = get_rng( "vile_poisons"          );
  rng_wound_poison          = get_rng( "wound_poison"          );

  // Overlapping procs require the use of a "distributed" RNG-stream when normalized_rng=1
  // also useful for frequent checks with low probability of proc and timed effect

  rng_hat_interval = get_rng( "hat_interval", RNG_DISTRIBUTED );
  rng_hat_interval -> average_range = false;
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
  scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors;
  scales_with[ STAT_HIT_RATING2           ] = 1;
}

// rogue_t::init_buffs ======================================================

void rogue_t::init_buffs()
{
  // Handle the Legendary here, as it's called after init_items()
  if ( find_item( "vengeance" ) && find_item( "fear" ) )
  {
    fof_p1 = 1;
  }
  else if ( find_item( "the_sleeper" ) && find_item( "the_dreamer" ) )
  {
    fof_p2 = 1;
  }
  else if ( find_item( "golad_twilight_of_aspects" ) && find_item( "tiriosh_nightmare_of_ages" ) )
  {
    fof_p3 = 1;
  }

  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  buffs_bandits_guile      = new buff_t( this, "bandits_guile", 12, timespan_t::from_seconds( 15.0 ), timespan_t::zero, 1.0, true );
  buffs_deadly_proc        = new buff_t( this, "deadly_proc",   1  );
  buffs_overkill           = new buff_t( this, "overkill",      1, timespan_t::from_seconds( 20.0 ) );
  buffs_recuperate         = new buff_t( this, "recuperate",    1  );
  buffs_shiv               = new buff_t( this, "shiv",          1  );
  buffs_stealthed          = new buff_t( this, "stealthed",     1  );
  buffs_tier11_4pc         = new buff_t( this, "tier11_4pc",    1, timespan_t::from_seconds( 15.0 ), timespan_t::zero, set_bonus.tier11_4pc_melee() * 0.01 );
  buffs_tier13_2pc         = new buff_t( this, "tier13_2pc",    1, spells.tier13_2pc -> duration(), timespan_t::zero, ( set_bonus.tier13_2pc_melee() ) ? 1.0 : 0 );
  buffs_tot_trigger        = new buff_t( this, 57934, "tricks_of_the_trade_trigger", -1, timespan_t::min, true );
  buffs_vanish             = new buff_t( this, "vanish",        1, timespan_t::from_seconds( 3.0 ) );

  buffs_tier12_4pc_haste   = new stat_buff_t( this, "future_on_fire",    STAT_HASTE_RATING,   0.0, 1, dbc.spell( 99186 ) -> duration() );
  buffs_tier12_4pc_crit    = new stat_buff_t( this, "fiery_devastation", STAT_CRIT_RATING,    0.0, 1, dbc.spell( 99187 ) -> duration() );
  buffs_tier12_4pc_mastery = new stat_buff_t( this, "master_of_flames",  STAT_MASTERY_RATING, 0.0, 1, dbc.spell( 99188 ) -> duration() );

  buffs_blade_flurry       = new buff_t( this, spec_blade_flurry -> spell_id(), "blade_flurry" );
  buffs_cold_blood         = new buff_t( this, talents.cold_blood -> spell_id(), "cold_blood" );
  buffs_shadow_dance       = new buff_t( this, talents.shadow_dance -> spell_id(), "shadow_dance" );

  buffs_adrenaline_rush    = new adrenaline_rush_buff_t    ( this, talents.adrenaline_rush -> spell_id() );
  buffs_envenom            = new envenom_buff_t            ( this );
  buffs_find_weakness      = new find_weakness_buff_t      ( this, talents.find_weakness -> spell_id() );
  buffs_killing_spree      = new killing_spree_buff_t      ( this, talents.killing_spree -> spell_id() );
  buffs_master_of_subtlety = new master_of_subtlety_buff_t ( this, spec_master_of_subtlety -> spell_id()  );
  buffs_revealing_strike   = new revealing_strike_buff_t   ( this, talents.revealing_strike -> spell_id() );
  buffs_shadowstep         = new shadowstep_buff_t         ( this, spec_shadowstep -> effect_trigger_spell( 1 ) );
  buffs_slice_and_dice     = new slice_and_dice_buff_t     ( this );
  buffs_vendetta           = new vendetta_buff_t           ( this, talents.vendetta -> spell_id() );

  // Legendary buffs
  // buffs_fof_p1            = new stat_buff_t( this, 109959, "legendary_daggers_p1", STAT_AGILITY, dbc.spell( 109959 ) -> effect1().base_value(), fof_p1 );
  // buffs_fof_p2            = new stat_buff_t( this, 109955, "legendary_daggers_p2", STAT_AGILITY, dbc.spell( 109955 ) -> effect1().base_value(), fof_p2 );
  // buffs_fof_p3            = new stat_buff_t( this, 109939, "legendary_daggers_p3", STAT_AGILITY, dbc.spell( 109939 ) -> effect1().base_value(), fof_p3 );
  // buffs_fof_fod           = new buff_t( this, 109949, "legendary_daggers", fof_p3 );
  // None of the buffs are currently in the DBC, so define them manually for now
  buffs_fof_p1            = new stat_buff_t( this, "legendary_daggers_p1", STAT_AGILITY,  2.0, 50, timespan_t::from_seconds( 30.0 ), timespan_t::from_seconds( 2.5 ), fof_p1 ); // Chance appears as 100% in DBC
  buffs_fof_p2            = new stat_buff_t( this, "legendary_daggers_p2", STAT_AGILITY,  5.0, 50, timespan_t::from_seconds( 30.0 ), timespan_t::from_seconds( 2.5 ), fof_p2 ); // http://ptr.wowhead.com/spell=109959#comments
  buffs_fof_p3            = new stat_buff_t( this, "legendary_daggers_p3", STAT_AGILITY, 17.0, 50, timespan_t::from_seconds( 30.0 ), timespan_t::from_seconds( 2.5 ), fof_p3 );
  buffs_fof_fod           = new buff_t( this, "legendary_daggers", 1, timespan_t::from_seconds( 6.0 ), timespan_t::zero, fof_p3 );
}

// rogue_t::init_values =====================================================

void rogue_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_melee() )
    attribute_initial[ ATTR_AGILITY ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    attribute_initial[ ATTR_AGILITY ]   += 90;
}

// trigger_honor_among_thieves ==============================================

struct honor_among_thieves_callback_t : public action_callback_t
{
  honor_among_thieves_callback_t( rogue_t* r ) : action_callback_t( r -> sim, r ) {}

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    rogue_t* rogue = listener -> cast_rogue();

    if ( a )
    {
      // only procs from specials; repeating is here for hunter autoshot, which doesn't proc it either
      if ( ! a -> special || a -> repeating )
        return;

      // doesn't proc from pets (only tested for hunter pets though)
      if ( a -> player -> is_pet() )
        return;

      a -> player -> procs.hat_donor -> occur();
    }

//    if ( sim -> debug )
//      log_t::output( sim, "Eligible For Honor Among Thieves" );

    if ( rogue -> cooldowns_honor_among_thieves -> remains() > timespan_t::zero )
      return;

    if ( ! rogue -> rng_honor_among_thieves -> roll( rogue -> talents.honor_among_thieves -> proc_chance() ) )
      return;

    rogue_targetdata_t* td = targetdata_t::get( rogue, rogue->target ) -> cast_rogue();

    td -> combo_points -> add( 1, rogue -> talents.honor_among_thieves );

    rogue -> procs_honor_among_thieves -> occur();

    rogue -> cooldowns_honor_among_thieves -> start( timespan_t::from_seconds( 5.0 ) - timespan_t::from_seconds( rogue -> talents.honor_among_thieves -> rank() ) );
  }
};

// rogue_t::register_callbacks ==============================================

void rogue_t::register_callbacks()
{
  player_t::register_callbacks();

  if ( talents.honor_among_thieves -> rank() )
  {
    action_callback_t* cb = new honor_among_thieves_callback_t( this );

    register_attack_callback( RESULT_CRIT_MASK, cb );
    register_spell_callback ( RESULT_CRIT_MASK, cb );
    register_tick_callback( RESULT_CRIT_MASK, cb );

    if ( virtual_hat_interval < timespan_t::zero )
    {
      virtual_hat_interval = timespan_t::from_seconds( 5.20 ) - timespan_t::from_seconds( talents.honor_among_thieves -> rank() );
    }
    if ( virtual_hat_interval > timespan_t::zero )
    {
      virtual_hat_callback = cb;
    }
    else
    {
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if ( p == this     ) continue;
        if ( p -> is_pet() ) continue;

        p -> register_attack_callback( RESULT_CRIT_MASK, cb );
        p -> register_spell_callback ( RESULT_CRIT_MASK, cb );
        p -> register_tick_callback( RESULT_CRIT_MASK, cb );
      }
    }
  }
}

// rogue_t::combat_begin ====================================================

void rogue_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.honor_among_thieves -> rank() )
  {
    if ( virtual_hat_interval > timespan_t::zero )
    {
      struct virtual_hat_event_t : public event_t
      {
        action_callback_t* callback;
        timespan_t interval;

        virtual_hat_event_t( sim_t* sim, rogue_t* p, action_callback_t* cb, timespan_t i ) :
          event_t( sim, p ), callback( cb ), interval( i )
        {
          name = "Virtual HAT Event";
          timespan_t cooldown = timespan_t::from_seconds( 5.0 ) - timespan_t::from_seconds( p -> talents.honor_among_thieves -> rank() );
          timespan_t remainder = interval - cooldown;
          if ( remainder < timespan_t::zero ) remainder = timespan_t::zero;
          timespan_t time = cooldown + p -> rng_hat_interval -> range( remainder*0.5, remainder*1.5 ) + timespan_t::from_seconds( 0.01 );
          sim -> add_event( this, time );
        }
        virtual void execute()
        {
          rogue_t* p = player -> cast_rogue();
          callback -> trigger( NULL );
          new ( sim ) virtual_hat_event_t( sim, p, callback, interval );
        }
      };
      new ( sim ) virtual_hat_event_t( sim, this, virtual_hat_callback, virtual_hat_interval );
    }
  }
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  expirations_.reset();
  last_tier12_4pc = -1;
}

// rogue_t::energy_regen_per_second =========================================

double rogue_t::energy_regen_per_second() const
{
  double r = player_t::energy_regen_per_second();

  if ( buffs_blade_flurry -> up() )
    r *= glyphs.blade_flurry -> ok() ? 0.90 : 0.80;

  return r;
}

// rogue_t::regen ===========================================================

void rogue_t::regen( timespan_t periodicity )
{
  // XXX review how this all stacks (additive/multiplicative)

  player_t::regen( periodicity );

  if ( buffs_adrenaline_rush -> up() )
  {
    if ( infinite_resource[ RESOURCE_ENERGY ] == 0 )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second();

      resource_gain( RESOURCE_ENERGY, energy_regen, gains_adrenaline_rush );
    }
  }

  if ( talents.overkill -> rank() && buffs_overkill -> up() )
  {
    if ( infinite_resource[ RESOURCE_ENERGY ] == 0 )
    {
      double energy_regen = periodicity.total_seconds() * energy_regen_per_second() * 0.30;

      resource_gain( RESOURCE_ENERGY, energy_regen, gains_overkill );
    }
  }

  uptimes_energy_cap -> update( resource_current[ RESOURCE_ENERGY ] ==
                                resource_max    [ RESOURCE_ENERGY ] );

  for ( int i = 0; i < 3; i++ )
    uptimes_bandits_guile[ i ] -> update( ( buffs_bandits_guile -> current_stack / 4 - 1 ) == i );
}

// rogue_t::available =======================================================

timespan_t rogue_t::available() const
{
  double energy = resource_current[ RESOURCE_ENERGY ];

  if ( energy > 25 )
    return timespan_t::from_seconds( 0.1 );

  return std::max(
    timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
    timespan_t::from_seconds( 0.1 )
  );
}

// rogue_t::copy_options ====================================================

void rogue_t::create_options()
{
  player_t::create_options();

  option_t rogue_options[] =
  {
    { "virtual_hat_interval",       OPT_TIMESPAN, &( virtual_hat_interval         ) },
    { "tricks_of_the_trade_target", OPT_STRING, &( tricks_of_the_trade_target_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, rogue_options );
}

// rogue_t::create_profile ==================================================

bool rogue_t::create_profile( std::string& profile_str, int save_type, bool save_html )
{
  player_t::create_profile( profile_str, save_type, save_html );

  if ( save_type == SAVE_ALL || save_type == SAVE_ACTIONS )
  {
    if ( talents.honor_among_thieves -> rank() )
    {
      profile_str += "# These values represent the avg HAT donor interval of the raid.\n";
      profile_str += "# A negative value will make the Rogue use a programmed default interval.\n";
      profile_str += "# A zero value will disable virtual HAT procs and assume a real raid is being simulated.\n";
      profile_str += "virtual_hat_interval=-1\n";  // Force it to generate profiles using programmed default.
    }
  }

  return true;
}

// rogue_t::copy_from =======================================================

void rogue_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  rogue_t* p = source -> cast_rogue();
  virtual_hat_interval = p -> virtual_hat_interval;
}

// rogue_t::decode_set ======================================================

int rogue_t::decode_set( item_t& item )
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

  if ( strstr( s, "wind_dancers" ) ) return SET_T11_MELEE;
  if ( strstr( s, "dark_phoenix" ) ) return SET_T12_MELEE;
  if ( strstr( s, "blackfang_battleweave" ) ) return SET_T13_MELEE;

  if ( strstr( s, "_gladiators_leather_" ) )  return SET_PVP_CASTER;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_rogue  ==================================================

player_t* player_t::create_rogue( sim_t* sim, const std::string& name, race_type r )
{
  return new rogue_t( sim, name, r );
}

// player_t::rogue_init =====================================================

void player_t::rogue_init( sim_t* sim )
{
  sim -> auras.honor_among_thieves = new aura_t( sim, "honor_among_thieves" );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.tricks_of_the_trade  = new   buff_t( p, "tricks_of_the_trade", 1, timespan_t::from_seconds( 6.0 ) );
    p -> debuffs.expose_armor       = new debuff_t( p, "expose_armor",     1 );
    p -> debuffs.hemorrhage         = new debuff_t( p, "hemorrhage",       1, timespan_t::from_seconds( 60.0 ) );
    p -> debuffs.master_poisoner    = new debuff_t( p, "master_poisoner", -1 );
    p -> debuffs.poisoned           = new debuff_t( p, "poisoned",        -1 );
    p -> debuffs.savage_combat      = new debuff_t( p, "savage_combat",   -1 );
  }
}

// player_t::rogue_combat_begin =============================================

void player_t::rogue_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.honor_among_thieves )
    sim -> auras.honor_among_thieves -> override();

  for ( player_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.expose_armor    ) t -> debuffs.expose_armor    -> override( 1, 0.12 );
    if ( sim -> overrides.hemorrhage      ) t -> debuffs.hemorrhage      -> override();
    if ( sim -> overrides.master_poisoner ) t -> debuffs.master_poisoner -> override();
    if ( sim -> overrides.poisoned        ) t -> debuffs.poisoned        -> override();
    if ( sim -> overrides.savage_combat   ) t -> debuffs.savage_combat   -> override();
  }
}
