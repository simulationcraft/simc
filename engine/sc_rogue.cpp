// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Rogue
// ==========================================================================

enum poison_type_t { POISON_NONE=0, ANESTHETIC_POISON, DEADLY_POISON, INSTANT_POISON, WOUND_POISON };

struct rogue_t : public player_t
{
  // Active
  action_t* active_anesthetic_poison;
  action_t* active_deadly_poison;
  action_t* active_instant_poison;
  action_t* active_wound_poison;
  action_t* active_rupture;

  // Buffs
  buff_t* buffs_adrenaline_rush;
  buff_t* buffs_blade_flurry;
  buff_t* buffs_cold_blood;
  buff_t* buffs_combo_points;
  buff_t* buffs_envenom;
  buff_t* buffs_hunger_for_blood;
  buff_t* buffs_killing_spree;
  buff_t* buffs_master_of_subtlety;
  buff_t* buffs_overkill;
  buff_t* buffs_poison_doses;
  buff_t* buffs_shadow_dance;
  buff_t* buffs_shadowstep;
  buff_t* buffs_shiv;
  buff_t* buffs_deadly_proc;
  buff_t* buffs_stealthed;
  buff_t* buffs_slice_and_dice;
  buff_t* buffs_tier9_2pc;

  // Cooldowns
  cooldown_t* cooldowns_honor_among_thieves;
  cooldown_t* cooldowns_seal_fate;

  // Expirations
  struct _expirations_t
  {
    event_t* wound_poison;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_adrenaline_rush;
  gain_t* gains_combat_potency;
  gain_t* gains_energy_refund;
  gain_t* gains_focused_attacks;
  gain_t* gains_overkill;
  gain_t* gains_quick_recovery;
  gain_t* gains_relentless_strikes;
  gain_t* gains_tier8_2pc;
  gain_t* gains_tier9_2pc;
  gain_t* gains_tier10_2pc;

  // Procs
  proc_t* procs_combo_points_wasted;
  proc_t* procs_deadly_poison;
  proc_t* procs_honor_among_thieves;
  proc_t* procs_ruthlessness;
  proc_t* procs_seal_fate;
  proc_t* procs_sword_specialization;
  proc_t* procs_tier10_4pc;

  // Up-Times
  uptime_t* uptimes_energy_cap;
  uptime_t* uptimes_poisoned;
  uptime_t* uptimes_rupture;
  uptime_t* uptimes_prey_on_the_weak;

  // Random Number Generation
  rng_t* rng_anesthetic_poison;
  rng_t* rng_combat_potency;
  rng_t* rng_cut_to_the_chase;
  rng_t* rng_deadly_poison;
  rng_t* rng_focused_attacks;
  rng_t* rng_honor_among_thieves;
  rng_t* rng_critical_strike_interval;
  rng_t* rng_initiative;
  rng_t* rng_instant_poison;
  rng_t* rng_master_poisoner;
  rng_t* rng_relentless_strikes;
  rng_t* rng_ruthlessness;
  rng_t* rng_seal_fate;
  rng_t* rng_sinister_strike_glyph;
  rng_t* rng_sword_specialization;
  rng_t* rng_tier10_4pc;
  rng_t* rng_wound_poison;

  // Options
  std::vector<action_callback_t*> critical_strike_callbacks;
  std::vector<double> critical_strike_intervals;
  std::string critical_strike_intervals_str;
  std::string tricks_of_the_trade_target_str;
  player_t*   tricks_of_the_trade_target;

  struct talents_t
  {
    int adrenaline_rush;
    int aggression;
    int blade_flurry;
    int blade_twisting;
    int blood_spatter;
    int close_quarters_combat;
    int cold_blood;
    int combat_potency;
    int cut_to_the_chase;
    int deadliness;
    int dirty_deeds;
    int dual_wield_specialization;
    int elusiveness;
    int filthy_tricks;
    int find_weakness;
    int focused_attacks;
    int ghostly_strike;
    int heightened_senses;
    int hemorrhage;
    int honor_among_thieves;
    int hunger_for_blood;
    int improved_ambush;
    int improved_eviscerate;
    int improved_expose_armor;
    int improved_poisons;
    int improved_sinister_strike;
    int improved_slice_and_dice;
    int initiative;
    int killing_spree;
    int lethality;
    int lightning_reflexes;
    int mace_specialization;
    int malice;
    int master_of_subtlety;
    int master_poisoner;
    int murder;
    int mutilate;
    int opportunity;
    int overkill;
    int precision;
    int premeditation;
    int preparation;
    int prey_on_the_weak;
    int puncturing_wounds;
    int quick_recovery;
    int relentless_strikes;
    int ruthlessness;
    int savage_combat;
    int seal_fate;
    int serrated_blades;
    int shadow_dance;
    int shadowstep;
    int sinister_calling;
    int slaughter_from_the_shadows;
    int sleight_of_hand;
    int surprise_attacks;
    int sword_specialization;
    int turn_the_tables;
    int vigor;
    int vile_poisons;
    int vitality;
    int weapon_expertise;

    // NYI
    int setup;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int adrenaline_rush;
    int backstab;
    int blade_flurry;
    int eviscerate;
    int expose_armor;
    int feint;
    int ghostly_strike;
    int hemorrhage;
    int hunger_for_blood;
    int killing_spree;
    int mutilate;
    int preparation;
    int rupture;
    int shadow_dance;
    int sinister_strike;
    int slice_and_dice;
    int tricks_of_the_trade;
    int vigor;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  rogue_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, ROGUE, name, race_type )
  {
    // Active
    active_anesthetic_poison = 0;
    active_deadly_poison     = 0;
    active_instant_poison    = 0;
    active_wound_poison      = 0;
    active_rupture           = 0;

    // Cooldowns
    cooldowns_honor_among_thieves = get_cooldown( "honor_among_thieves" );
    cooldowns_seal_fate           = get_cooldown( "seal_fate"           );

    // Options
    critical_strike_intervals_str = "1.50/1.75/2.0/2.25";
    tricks_of_the_trade_target_str = "other";
    tricks_of_the_trade_target = 0;
  }

  // Character Definition
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_actions();
  virtual void      register_callbacks();
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      interrupt();
  virtual void      clear_debuffs();
  virtual void      regen( double periodicity );
  virtual double    available() SC_CONST;
  virtual double    composite_attack_crit() SC_CONST;
  virtual double    composite_tank_miss( int school ) SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_ENERGY; }
  virtual int       primary_role() SC_CONST     { return ROLE_ATTACK; }
  virtual int       primary_tree() SC_CONST;
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL );

  // Utilities
  double combo_point_rank( double* cp_list )
  {
    assert( buffs_combo_points -> current_stack > 0 );
    return cp_list[ buffs_combo_points -> current_stack - 1 ];
  }
  double combo_point_rank( double cp1, double cp2, double cp3, double cp4, double cp5 )
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return combo_point_rank( cp_list );
  }
};

namespace   // ANONYMOUS NAMESPACE =========================================
{

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_t : public attack_t
{
  int  requires_weapon;
  int  requires_position;
  bool requires_stealth;
  bool requires_combo_points;
  bool adds_combo_points;
  int  min_combo_points, max_combo_points;
  double min_energy, max_energy;
  double min_hfb_expire, max_hfb_expire;
  double min_snd_expire, max_snd_expire;
  double min_rup_expire, max_rup_expire;
  double min_env_expire, max_env_expire;

  rogue_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_ENERGY, s, t, special ),
      requires_weapon( WEAPON_NONE ),
      requires_position( POSITION_NONE ),
      requires_stealth( false ),
      requires_combo_points( false ),
      adds_combo_points( false ),
      min_combo_points( 0 ), max_combo_points( 0 ),
      min_energy( 0 ), max_energy( 0 ),
      min_hfb_expire( 0 ), max_hfb_expire( 0 ),
      min_snd_expire( 0 ), max_snd_expire( 0 ),
      min_rup_expire( 0 ), max_rup_expire( 0 ),
      min_env_expire( 0 ), max_env_expire( 0 )
  {
    rogue_t* p = player -> cast_rogue();
    base_hit  += p -> talents.precision * 0.01;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost() SC_CONST;
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual bool   ready();
  virtual void   assess_damage( double amount, int dmg_type );
};

// ==========================================================================
// Rogue Poison
// ==========================================================================

struct rogue_poison_t : public spell_t
{
  rogue_poison_t( const char* n, player_t* player ) :
      spell_t( n, player, RESOURCE_NONE, SCHOOL_NATURE, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();

    base_hit  += p -> talents.precision * 0.01;

    base_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vile_poisons,  3, 0.07, 0.14, 0.20 );

    weapon_multiplier = 0;

    // Poisons are spells that use attack power
    base_spell_power_multiplier  = 0.0;
    base_attack_power_multiplier = 1.0;
  }

  virtual void player_buff();
};

// ==========================================================================
// Static Functions
// ==========================================================================

// break_stealth ===========================================================

static void break_stealth( rogue_t* p )
{
  if ( p -> buffs_stealthed -> check() )
  {
    p -> buffs_stealthed -> expire();

    if ( p -> talents.master_of_subtlety )
    {
      double bonus = util_t::talent_rank( p -> talents.master_of_subtlety, 3, 0.04, 0.07, 0.10 );

      p -> buffs_master_of_subtlety -> trigger( 1, bonus );
    }
    if ( p -> talents.overkill )
    {
      p -> buffs_overkill -> trigger();
    }
  }
}

// add_combo_point =========================================================

static void add_combo_point( rogue_t* p )
{
  if ( p -> buffs_combo_points -> current_stack == 5 )
  {
    p -> procs_combo_points_wasted -> occur();
    return;
  }

  p -> buffs_combo_points -> trigger();
}

// trigger_apply_poisons ===================================================

static void trigger_apply_poisons( rogue_attack_t* a )
{
  weapon_t* w = a -> weapon;

  if ( ! w ) return;

  rogue_t* p = a -> player -> cast_rogue();

  if ( w -> buff_type == ANESTHETIC_POISON )
  {
    p -> active_anesthetic_poison -> weapon = w;
    p -> active_anesthetic_poison -> execute();
  }
  else if ( w -> buff_type == DEADLY_POISON )
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

// trigger_other_poisons ===================================================

static void trigger_other_poisons( rogue_t* p, weapon_t* other_w )
{
  if ( ! other_w ) return;

  if ( other_w -> buff_type == ANESTHETIC_POISON )
  {
    p -> active_anesthetic_poison -> weapon = other_w;
    p -> active_anesthetic_poison -> execute();
  }
  else if ( other_w -> buff_type == INSTANT_POISON )
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

// trigger_combat_potency ==================================================

static void trigger_combat_potency( rogue_attack_t* a )
{
  weapon_t* w = a -> weapon;

  if ( ! w || w -> slot != SLOT_OFF_HAND )
    return;

  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.combat_potency )
    return;

  if ( p -> rng_combat_potency -> roll( 0.20 ) )
  {
    p -> resource_gain( RESOURCE_ENERGY, p -> talents.combat_potency * 3.0, p -> gains_combat_potency );
  }
}

// trigger_slice_and_dice ==================================================

static void trigger_slice_and_dice( rogue_t* p,
                                    int      combo_points )
{
  double duration = 6.0 + 3.0 * combo_points;

  if ( p -> glyphs.slice_and_dice ) duration += 3.0;

  duration *= 1.0 + p -> talents.improved_slice_and_dice * 0.25;

  if( p -> buffs_slice_and_dice -> remains_lt( duration ) )
  {
    p -> buffs_slice_and_dice -> duration = duration;
    p -> buffs_slice_and_dice -> trigger();
  }
}

// trigger_cut_to_the_chase ================================================

static void trigger_cut_to_the_chase( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.cut_to_the_chase )
    return;

  if ( ! p -> buffs_slice_and_dice -> check() )
    return;

  if ( p -> rng_cut_to_the_chase -> roll( p -> talents.cut_to_the_chase * 0.20 ) )
  {
    trigger_slice_and_dice( p, 5 );
  }
}

// trigger_dirty_deeds =====================================================

static void trigger_dirty_deeds( rogue_attack_t* a )
{
  rogue_t*  p = a -> player -> cast_rogue();

  if ( ! p -> talents.dirty_deeds )
    return;

  if ( a -> sim -> target -> health_percentage() < 35 )
  {
    a -> player_multiplier *= 1.0 + p -> talents.dirty_deeds * 0.10;
  }
}

// trigger_initiative ======================================================

static void trigger_initiative( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.initiative )
    return;

  if ( p -> rng_initiative -> roll( p -> talents.initiative / 3.0 ) )
  {
    add_combo_point( p );
  }
}

// trigger_focused_attacks =================================================

static void trigger_focused_attacks( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.focused_attacks )
    return;

  if ( p -> rng_focused_attacks -> roll( p -> talents.focused_attacks / 3.0 ) )
  {
    p -> resource_gain( RESOURCE_ENERGY, 2, p -> gains_focused_attacks );
  }
}

// trigger_energy_refund ===================================================

static void trigger_energy_refund( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! a -> adds_combo_points )
    return;

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains_energy_refund );
}

// trigger_quick_recovery ==================================================

static void trigger_quick_recovery( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.quick_recovery )
    return;

  if ( ! a -> requires_combo_points )
    return;

  double recovered_energy = p -> talents.quick_recovery * 0.40 * a -> resource_consumed;

  p -> resource_gain( RESOURCE_ENERGY, recovered_energy, p -> gains_quick_recovery );
}

// trigger_relentless_strikes ===============================================

static void trigger_relentless_strikes( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.relentless_strikes )
    return;

  if ( ! a -> requires_combo_points )
    return;

  if ( p -> rng_relentless_strikes -> roll( p -> talents.relentless_strikes * p -> buffs_combo_points -> current_stack * 0.04 ) )
  {
    p -> resource_gain( RESOURCE_ENERGY, 25, p -> gains_relentless_strikes );
  }
}

// trigger_ruthlessness ====================================================

static void trigger_ruthlessness( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.ruthlessness )
    return;

  if ( ! a -> requires_combo_points )
    return;

  if ( p -> rng_ruthlessness -> roll( p -> talents.ruthlessness * 0.20 ) )
  {
    p -> procs_ruthlessness -> occur();
    add_combo_point( p );
  }
}

// trigger_seal_fate =======================================================

static void trigger_seal_fate( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.seal_fate )
    return;

  if ( ! a -> adds_combo_points )
    return;

  // This is to prevent dual-weapon special attacks from triggering a double-proc of Seal Fate

  if ( p -> cooldowns_seal_fate -> remains() > 0 )
    return;

  if ( p -> talents.seal_fate )
  {
    if ( p -> rng_seal_fate -> roll( p -> talents.seal_fate * 0.20 ) )
    {
      p -> cooldowns_seal_fate -> start( 0.0001 );
      p -> procs_seal_fate -> occur();
      add_combo_point( p );
    }
  }
}

// trigger_tier10_4pc =======================================================

static void trigger_tier10_4pc( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> set_bonus.tier10_4pc_melee() )
    return;

  if ( ! a -> requires_combo_points )
    return;

  if ( p -> rng_tier10_4pc -> roll( 0.13 ) )
  {
    p -> procs_tier10_4pc -> occur();
    add_combo_point( p );
    add_combo_point( p );
    add_combo_point( p );
  }
}

// trigger_sword_specialization ============================================

static void trigger_sword_specialization( rogue_attack_t* a )
{
  if ( a -> proc ) return;

  weapon_t* w = a -> weapon;

  if ( ! w ) return;

  if ( w -> type != WEAPON_SWORD &&
       w -> type != WEAPON_AXE )
    return;

  rogue_t* p = a -> player -> cast_rogue();

  if ( ! p -> talents.sword_specialization )
    return;

  if ( p -> rng_sword_specialization -> roll( p -> talents.sword_specialization * 0.01 ) )
  {
    p -> main_hand_attack -> proc = true;
    p -> main_hand_attack -> execute();
    p -> main_hand_attack -> proc = false;
    p -> procs_sword_specialization -> occur();
  }
}

// trigger_tricks_of_the_trade ==============================================

static void trigger_tricks_of_the_trade( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  player_t* t = p -> tricks_of_the_trade_target;

  if ( t )
  {
    double duration = ( p -> glyphs.tricks_of_the_trade ? 10.0 : 6.0 );

    if ( t -> buffs.tricks_of_the_trade -> remains_lt( duration ) )
    {
      t -> buffs.tricks_of_the_trade -> duration = duration;
      t -> buffs.tricks_of_the_trade -> trigger();
    }

    p -> tricks_of_the_trade_target = 0;
  }
}

// apply_poison_debuff ======================================================

static void apply_poison_debuff( rogue_t* p )
{
  target_t* t = p -> sim -> target;

  if ( p -> talents.master_poisoner ) t -> debuffs.master_poisoner -> increment();
  if ( p -> talents.savage_combat   ) t -> debuffs.savage_combat   -> increment();

  t -> debuffs.poisoned -> increment();
}

// remove_poison_debuff =====================================================

static void remove_poison_debuff( rogue_t* p )
{
  target_t* t = p -> sim -> target;

  if ( p -> talents.master_poisoner ) t -> debuffs.master_poisoner -> decrement();
  if ( p -> talents.savage_combat   ) t -> debuffs.savage_combat   -> decrement();

  t -> debuffs.poisoned -> decrement();
}

// =========================================================================
// Rogue Attacks
// =========================================================================

// rogue_attack_t::parse_options ===========================================

void rogue_attack_t::parse_options( option_t*          options,
                                    const std::string& options_str )
{
  option_t base_options[] =
  {
    { "min_combo_points", OPT_INT, &min_combo_points },
    { "max_combo_points", OPT_INT, &max_combo_points },
    { "cp>",              OPT_INT, &min_combo_points },
    { "cp<",              OPT_INT, &max_combo_points },
    { "min_energy",       OPT_FLT, &min_energy       },
    { "max_energy",       OPT_FLT, &max_energy       },
    { "energy>",          OPT_FLT, &min_energy       },
    { "energy<",          OPT_FLT, &max_energy       },
    { "hfb>",             OPT_FLT, &min_hfb_expire   },
    { "hfb<",             OPT_FLT, &max_hfb_expire   },
    { "snd>",             OPT_FLT, &min_snd_expire   },
    { "snd<",             OPT_FLT, &max_snd_expire   },
    { "env>",             OPT_FLT, &min_env_expire   },
    { "env<",             OPT_FLT, &max_env_expire   },
    { "rup>",             OPT_FLT, &min_rup_expire   },
    { "rup<",             OPT_FLT, &max_rup_expire   },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// rogue_attack_t::cost ====================================================

double rogue_attack_t::cost() SC_CONST
{
  rogue_t* p = player -> cast_rogue();
  double c = attack_t::cost();
  if ( c <= 0 ) return 0;
  if ( adds_combo_points && p -> set_bonus.tier7_4pc_melee() ) c *= 0.95;
  return c;
}

// rogue_attack_t::consume_resource ========================================

void rogue_attack_t::consume_resource()
{
  rogue_t* p = player -> cast_rogue();
  attack_t::consume_resource();
  if( result_is_hit() )
  {
    trigger_relentless_strikes( this );
    if ( requires_combo_points ) p -> buffs_combo_points -> expire();
  }
}

// rogue_attack_t::execute =================================================

void rogue_attack_t::execute()
{
  rogue_t* p = player -> cast_rogue();

  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( adds_combo_points ) add_combo_point ( p );

    trigger_apply_poisons( this );
    trigger_ruthlessness( this );
    trigger_sword_specialization( this );
    trigger_tricks_of_the_trade( this );

    if ( result == RESULT_CRIT )
    {
      trigger_focused_attacks( this );
      trigger_seal_fate( this );
    }
  }
  else
  {
    trigger_energy_refund( this );
    trigger_quick_recovery( this );
  }

  trigger_tier10_4pc( this ); // FIX-ME: Does it require the finishing move to hit before it can proc?

  break_stealth( p );

  // Prevent non-critting abilities (Rupture, Garrote, Shiv; maybe something else) from eating Cold Blood
  if ( may_crit ) p -> buffs_cold_blood -> expire();

  if ( harmful && special ) p -> buffs_shadowstep -> expire();

  if ( p -> buffs_tier9_2pc -> check() && base_cost > 0 )
  {
    p -> buffs_tier9_2pc -> expire();
    double refund = resource_consumed > 40 ? 40 : resource_consumed;
    p -> resource_gain( RESOURCE_ENERGY, refund, p -> gains_tier9_2pc );
  }
}

// rogue_attack_t::player_buff ============================================

void rogue_attack_t::player_buff()
{
  rogue_t* p = player -> cast_rogue();

  attack_t::player_buff();

  if ( p -> talents.serrated_blades )
  {
    player_penetration += p -> talents.serrated_blades * 0.03;
  }
  if ( weapon )
  {
    if ( p -> talents.mace_specialization )
    {
      if ( weapon -> type == WEAPON_MACE )
      {
        player_penetration += p -> talents.mace_specialization * 0.03;
      }
    }
    if ( p -> talents.close_quarters_combat )
    {
      if ( weapon -> type == WEAPON_DAGGER ||
           weapon -> type == WEAPON_FIST   )
      {
        player_crit += p -> talents.close_quarters_combat * 0.01;
      }
    }
    if ( p -> talents.dual_wield_specialization )
    {
      if ( weapon -> slot == SLOT_OFF_HAND )
      {
        player_multiplier *= 1.0 + p -> talents.dual_wield_specialization * 0.10;
      }
    }
  }
  if ( p -> buffs_cold_blood -> check() )
  {
    player_crit += 1.0;
  }
  if ( p -> buffs_hunger_for_blood -> up() )
  {
    player_multiplier *= 1.05 + p -> glyphs.hunger_for_blood * 0.03;
  }
  if ( p -> buffs_master_of_subtlety -> check() )
  {
    player_multiplier *= 1.0 + p -> buffs_master_of_subtlety -> value();
  }

  player_multiplier *= 1.0 + p -> talents.murder * 0.02;

  if ( special && p -> buffs_shadowstep -> check() )
  {
    player_multiplier *= 1.20;
  }
  if ( p -> buffs_killing_spree -> up() )
  {
    player_multiplier *= 1.20;
  }
  if ( p -> talents.prey_on_the_weak )
  {
    bool active = ( sim -> target -> health_percentage() <= 100.0 );
    if ( active ) player_crit_multiplier *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    p -> uptimes_prey_on_the_weak -> update( active );
  }
}


// rogue_attack_t::ready() ================================================

bool rogue_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  rogue_t* p = player -> cast_rogue();

  if ( requires_weapon != WEAPON_NONE )
    if ( ! weapon || weapon -> type != requires_weapon )
      return false;

  if ( requires_position != POSITION_NONE )
    if ( p -> position != requires_position )
      return false;

  if ( requires_stealth )
    if ( ! p -> buffs_shadowstep   -> check() &&
         ! p -> buffs_shadow_dance -> check() )
      if ( ! p -> buffs_stealthed -> check() )
        return false;

  if ( requires_combo_points && ! p -> buffs_combo_points -> current_stack )
    return false;

  if ( min_combo_points > 0 )
    if ( ! p -> buffs_combo_points -> may_react( min_combo_points ) )
      return false;

  if ( max_combo_points > 0 )
    if ( p -> buffs_combo_points -> may_react( max_combo_points + 1 ) )
      return false;

  if ( min_energy > 0 )
    if ( p -> resource_current[ RESOURCE_ENERGY ] < min_energy )
      return false;

  if ( max_energy > 0 )
    if ( p -> resource_current[ RESOURCE_ENERGY ] > max_energy )
      return false;

  double ct = sim -> current_time;

  if ( min_hfb_expire > 0 )
    if ( p -> buffs_hunger_for_blood -> remains_lt( min_hfb_expire ) )
      return false;

  if ( max_hfb_expire > 0 )
    if ( p -> buffs_hunger_for_blood -> remains_gt( max_hfb_expire ) )
      return false;

  if ( min_snd_expire > 0 )
    if ( p -> buffs_slice_and_dice -> remains_lt( min_snd_expire ) )
      return false;

  if ( max_snd_expire > 0 )
    if ( p -> buffs_slice_and_dice -> remains_gt( max_snd_expire ) )
      return false;

  if ( min_env_expire > 0 )
    if ( p -> buffs_envenom -> remains_lt( min_env_expire ) )
      return false;

  if ( max_env_expire > 0 )
    if ( p -> buffs_envenom -> remains_gt( max_env_expire ) )
      return false;

  if ( min_rup_expire > 0 )
    if ( ! p -> active_rupture || ( ( p -> active_rupture -> dot -> ready - ct ) < min_rup_expire ) )
      return false;

  if ( max_rup_expire > 0 )
    if ( p -> active_rupture && ( ( p -> active_rupture -> dot -> ready - ct ) > max_rup_expire ) )
      return false;

  return true;
}

// rogue_attack_t::assess_damage ===========================================

void rogue_attack_t::assess_damage( double amount,
                                    int    dmg_type )
{
  attack_t::assess_damage( amount, dmg_type );

  rogue_t* p = player -> cast_rogue();

  if ( p -> buffs_blade_flurry -> up() && sim -> target -> adds_nearby )
  {
    attack_t::additional_damage( amount, dmg_type );
  }
}

// Melee Attack ============================================================

struct melee_t : public rogue_attack_t
{
  int sync_weapons;

  melee_t( const char* name, player_t* player, int sw ) :
    rogue_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, false ), sync_weapons( sw )
  {
    rogue_t* p = player -> cast_rogue();

    base_dd_min = base_dd_max = 1;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;
    may_crit        = true;

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  virtual double haste() SC_CONST
  {
    rogue_t* p = player -> cast_rogue();

    double h = rogue_attack_t::haste();

    if ( p -> talents.lightning_reflexes )
    {
      h *= 1.0 / ( 1.0 + util_t::talent_rank( p -> talents.lightning_reflexes, 3, 0.04, 0.07, 0.10 ) );
    }

    if ( p -> buffs_blade_flurry   -> up() ) h *= 1.0 / ( 1.0 + 0.20 );
    if ( p -> buffs_slice_and_dice -> up() ) h *= 1.0 / ( 1.0 + 0.40 + ( p -> set_bonus.tier6_2pc_melee() ? 0.05 : 0.00 ) );

    return h;
  }

  virtual double execute_time() SC_CONST
  {
    double t = rogue_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, 0.2 ) : t/2 ) : 0.01;
    }
    return t;
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

// Auto Attack =============================================================

struct auto_attack_t : public rogue_attack_t
{
  int sync_weapons;

  auto_attack_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "auto_attack", player ),
      sync_weapons( 0 )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( "melee_main_hand", player, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    p -> main_hand_attack -> schedule_execute();
    if ( p -> off_hand_attack )
    {
      p -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public rogue_attack_t
{
  adrenaline_rush_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "adrenaline_rush", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.adrenaline_rush );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown -> duration = 180;

    id = 13750;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_adrenaline_rush -> trigger();
    update_ready();
  }
};

// Ambush ==================================================================

struct ambush_t : public rogue_attack_t
{
  ambush_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "ambush", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 10, 330, 330, 0, 60 },
      { 75,  9, 280, 280, 0, 60 },
      { 70,  8, 185, 185, 0, 60 },
      { 66,  7, 134, 134, 0, 60 },
      { 58,  6, 116, 116, 0, 60 },
      {  0,  0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48691 );

    weapon = &( p -> main_hand_weapon );
    may_crit               = true;
    normalize_weapon_speed = true;
    requires_weapon        = WEAPON_DAGGER;
    requires_position      = POSITION_BACK;
    requires_stealth       = true;
    adds_combo_points      = true;
    weapon_multiplier     *= 2.75;
    base_cost             -= p -> talents.slaughter_from_the_shadows * 4;
    base_multiplier       *= 1.0 + ( p -> talents.find_weakness * 0.02 +
                                     p -> talents.opportunity   * 0.10 +
                                     p -> talents.slaughter_from_the_shadows * 0.01 );
    base_crit             += p -> talents.improved_ambush * 0.25;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_initiative( this );
    }
  }
};

// Backstab ==================================================================

struct backstab_t : public rogue_attack_t
{
  backstab_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "backstab", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 12, 310, 310, 0, 60 },
      { 74, 11, 225, 225, 0, 60 },
      { 68, 10, 170, 170, 0, 60 },
      { 60,  9, 150, 150, 0, 60 },
      {  0,  0,   0,   0, 0,  0 }
    };
    init_rank( ranks, 48657 );

    weapon = &( p -> main_hand_weapon );
    may_crit               = true;
    normalize_weapon_speed = true;
    requires_weapon        = WEAPON_DAGGER;
    requires_position      = POSITION_BACK;
    adds_combo_points      = true;
    base_cost             -= p -> talents.slaughter_from_the_shadows * 4;
    weapon_multiplier     *= 1.50;
    weapon_multiplier     *= 1.0 + p -> talents.sinister_calling * 0.02;

    base_multiplier *= 1.0 + ( p -> talents.aggression          * 0.03 +
                               p -> talents.blade_twisting      * 0.05 +
                               p -> talents.find_weakness       * 0.02 +
                               p -> talents.opportunity         * 0.10 +
                               p -> talents.surprise_attacks    * 0.10 +
                               p -> set_bonus.tier6_4pc_melee() * 0.06 +
                               p -> talents.slaughter_from_the_shadows * 0.01 );

    base_crit += ( p -> talents.puncturing_wounds * 0.10 +
                   p -> talents.turn_the_tables   * 0.02 );

    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += .05;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::execute();
    if ( p -> glyphs.backstab &&
         p -> active_rupture  &&
         p -> active_rupture -> added_ticks < 3 )
    {
      p -> active_rupture -> extend_duration( 1 );
    }
  }
};

// Blade Flurry ============================================================

struct blade_flurry_t : public rogue_attack_t
{
  blade_flurry_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "blade_flurry", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.blade_flurry );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = p -> glyphs.blade_flurry ? 0 : 25;
    cooldown -> duration  = 120;

    id = 13877;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_blade_flurry -> trigger();
    update_ready();
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  int min_doses;
  int no_buff;
  double dose_dmg;

  envenom_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "envenom", player, SCHOOL_NATURE, TREE_ASSASSINATION ), min_doses( 1 ), no_buff( 0 )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> level >= 62 );

    option_t options[] =
    {
      { "min_doses", OPT_INT,  &min_doses },
      { "no_buff",   OPT_BOOL, &no_buff   },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
    may_crit = true;
    requires_combo_points = true;
    base_cost = 35;

    base_multiplier *= 1.0 + ( p -> talents.find_weakness * 0.02 +
                               p -> talents.slaughter_from_the_shadows * 0.01 +
                               util_t::talent_rank( p -> talents.vile_poisons, 3, 0.07, 0.14, 0.20 ) );

    if ( p -> talents.surprise_attacks ) may_dodge = false;

    dose_dmg = ( p -> level >= 80 ? 215 :
                 p -> level >= 74 ? 175 :
                 p -> level >= 69 ? 147 : 117 );

    id = 57993;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int doses_consumed = std::min( p -> buffs_poison_doses -> current_stack,
                                   p -> buffs_combo_points -> current_stack );

    double envenom_duration = 1.0 + p -> buffs_combo_points -> current_stack;

    if ( p -> buffs_envenom -> remains() < envenom_duration )
    {
      p -> buffs_envenom -> duration = envenom_duration;
      p -> buffs_envenom -> trigger();
    }

    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( ! p -> rng_master_poisoner -> roll( p -> talents.master_poisoner / 3.0 ) )
      {
        p -> buffs_poison_doses -> decrement( doses_consumed );
      }
      if ( ! p -> buffs_poison_doses -> check() )
      {
        p -> active_deadly_poison -> cancel();
      }

      trigger_cut_to_the_chase( this );
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();

    int doses_consumed = std::min( p -> buffs_poison_doses -> current_stack,
                                   p -> buffs_combo_points -> current_stack );

    direct_power_mod = p -> buffs_combo_points -> current_stack * 0.09;

    base_dd_min = base_dd_max = doses_consumed * dose_dmg;

    rogue_attack_t::player_buff();

    trigger_dirty_deeds( this );
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    // Envenom is not usable when there is no DP on a target
    if ( ! p -> buffs_poison_doses -> check() )
      return false;

    if ( min_doses > 0 )
      if ( min_doses > p -> buffs_poison_doses -> current_stack )
        return false;

    if ( no_buff )
      if ( p -> buffs_envenom -> check() )
        return false;

    return rogue_attack_t::ready();
  }
};

// Eviscerate ================================================================

struct eviscerate_t : public rogue_attack_t
{
  struct double_pair { double min, max; };

  double_pair* combo_point_dmg;

  eviscerate_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "eviscerate", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    may_crit = true;
    requires_combo_points = true;
    base_cost = 35;

    base_multiplier *= 1.0 + ( p -> talents.aggression    * 0.03 +
                               p -> talents.find_weakness * 0.02 +
                               p -> talents.slaughter_from_the_shadows * 0.01 +
                               util_t::talent_rank( p -> talents.improved_eviscerate, 3, 0.07, 0.14, 0.20 ) );

    if ( p -> glyphs.eviscerate ) base_crit += 0.10;

    if ( p -> talents.surprise_attacks ) may_dodge = false;

    static double_pair dmg_79[] = { { 497, 751 }, { 867, 1121 }, { 1237, 1491 }, { 1607, 1861 }, { 1977, 2231 } };
    static double_pair dmg_73[] = { { 405, 613 }, { 706,  914 }, { 1007, 1215 }, { 1308, 1516 }, { 1609, 1817 } };
    static double_pair dmg_64[] = { { 245, 365 }, { 430,  550 }, {  615,  735 }, {   800, 920 }, {  985, 1105 } };
    static double_pair dmg_60[] = { { 224, 332 }, { 394,  502 }, {  564,  762 }, {   734, 842 }, {  904, 1012 } };

    combo_point_dmg = ( p -> level >= 79 ? dmg_79 :
                        p -> level >= 73 ? dmg_73 :
                        p -> level >= 64 ? dmg_64 :
                        dmg_60 );

    id = 48668;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    base_dd_min = combo_point_dmg[ p -> buffs_combo_points -> current_stack - 1 ].min;
    base_dd_max = combo_point_dmg[ p -> buffs_combo_points -> current_stack - 1 ].max;

    direct_power_mod = 0.07 * p -> buffs_combo_points -> current_stack;

    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_cut_to_the_chase( this );
    }
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }
};

// Expose Armor =============================================================

struct expose_armor_t : public rogue_attack_t
{
  int override_sunder;

  expose_armor_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "expose_armor", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION ), override_sunder( 0 )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { "override_sunder", OPT_BOOL, &override_sunder },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    requires_combo_points = true;
    base_cost = 25 - p -> talents.improved_expose_armor * 5;

    if ( p -> talents.surprise_attacks ) may_dodge = false;

    id = 8647;
  }

  virtual void execute()
  {
    double duration;
    rogue_t* p = player -> cast_rogue();

    duration = 6 * p -> buffs_combo_points -> current_stack;
    rogue_attack_t::execute();
    if ( result_is_hit() )
    {
      target_t* t = sim -> target;

      if ( p -> glyphs.expose_armor ) duration += 10;

      if( t -> debuffs.expose_armor -> remains_lt( duration ) )
      {
        t -> debuffs.expose_armor -> duration = duration;
        t -> debuffs.expose_armor -> trigger( 1, 0.2 );
      }
    }
  };

  virtual bool ready()
  {
    target_t* t = sim -> target;

    if ( t -> debuffs.expose_armor -> check() )
      return false;

    if ( ! override_sunder )
      if ( t -> debuffs.sunder_armor -> check() )
        return false;

    return rogue_attack_t::ready();
  }
};

// Feint ====================================================================

struct feint_t : public rogue_attack_t
{
  feint_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "feint", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = p -> glyphs.feint ? 10 : 20;
    cooldown -> duration = 10;

    id = 48659;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    // model threat eventually......
  }
};

// Garrote ==================================================================

struct garrote_t : public rogue_attack_t
{
  garrote_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "garrote", player, SCHOOL_BLEED, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 10, 0, 0, 119, 50 },
      { 75,  9, 0, 0, 110, 50 },
      { 70,  8, 0, 0, 102, 50 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48676 );

    // it uses a weapon (for poison app)
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;
    // to stop seal fate procs
    may_crit          = false;

    requires_position = POSITION_BACK;
    requires_stealth  = true;
    adds_combo_points = true;

    num_ticks         = 6;
    base_tick_time    = 3.0;
    tick_power_mod    = .07;

    base_cost        -= p -> talents.dirty_deeds * 10;
    base_multiplier  *= 1.0 + ( p -> talents.find_weakness * 0.02 +
                                p -> talents.opportunity   * 0.10 +
                                p -> talents.blood_spatter * 0.15 +
                                p -> talents.slaughter_from_the_shadows * 0.01 );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_initiative( this );
    }
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }
};

// Ghostly Strike ==========================================================

struct ghostly_strike_t : public rogue_attack_t
{
  ghostly_strike_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "ghostly_strike", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.ghostly_strike );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    base_dd_min = base_dd_max   = 1;
    may_crit                    = true;
    normalize_weapon_speed      = true;
    adds_combo_points           = true;
    cooldown -> duration        = p -> glyphs.ghostly_strike ? 30 : 20;
    base_cost                   = 40;
    if ( weapon -> type == WEAPON_DAGGER )
      weapon_multiplier        *= 1.80 + ( p -> glyphs.ghostly_strike ? 0.4 : 0.0 );
    else
      weapon_multiplier        *= 1.25 + ( p -> glyphs.ghostly_strike ? 0.4 : 0.0 );
    base_multiplier            *= 1.0 + ( p -> talents.find_weakness * 0.02 +
                                          p -> talents.slaughter_from_the_shadows * 0.01 );
    base_crit                  += p -> talents.turn_the_tables * 0.02;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;

    id = 14278;
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }
};

// Hemorrhage =============================================================

struct hemorrhage_t : public rogue_attack_t
{
  double damage_adder;

  hemorrhage_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "hemorrhage", player, SCHOOL_PHYSICAL, TREE_SUBTLETY ), damage_adder( 0 )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.hemorrhage );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_min = base_dd_max = 1;
    weapon = &( p -> main_hand_weapon );
    may_crit               = true;
    normalize_weapon_speed = true;
    adds_combo_points      = true;
    base_cost              = 35 - p -> talents.slaughter_from_the_shadows;

    weapon_multiplier *= ( weapon -> type == WEAPON_DAGGER ) ? 1.60 : 1.10;
    weapon_multiplier *= 1.0 + p -> talents.sinister_calling * 0.02;

    base_multiplier *= 1.0 + ( p -> talents.find_weakness       * 0.02 +
                               p -> talents.surprise_attacks    * 0.10 +
                               p -> set_bonus.tier6_4pc_melee() * 0.06 +
                               p -> talents.slaughter_from_the_shadows * 0.01 );
    base_crit += p -> talents.turn_the_tables * 0.02;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;

    damage_adder = util_t::ability_rank( p -> level,  75.0,80,  42.0,70,  29.0,0 );

    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += .05;

    if ( p -> glyphs.hemorrhage ) damage_adder *= 1.40;

    id = 48660;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();
    if ( result_is_hit() )
    {
      target_t* t = sim -> target;

      if ( damage_adder >= t -> debuffs.hemorrhage -> current_value )
      {
        t -> debuffs.hemorrhage -> trigger( 10, damage_adder );
      }
    }
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }
};

// Hunger For Blood =========================================================

struct hunger_for_blood_t : public rogue_attack_t
{
  double refresh_at;

  hunger_for_blood_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "hunger_for_blood", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION ),
      refresh_at( 5 )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.hunger_for_blood );

    option_t options[] =
    {
      { "refresh_at", OPT_FLT, &refresh_at },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = 15.0;

    id = 51662;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_hunger_for_blood -> trigger();
    consume_resource();
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if ( ! sim -> target -> debuffs.bleeding -> check() )
      return false;

    if ( refresh_at > 0 )
      if ( p -> buffs_hunger_for_blood -> remains_gt( refresh_at ) )
        return false;

    return rogue_attack_t::ready();
  }
};

// Kick =====================================================================

struct kick_t : public rogue_attack_t
{
  kick_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "kick", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    base_cost = 25.0;
    base_dd_min = base_dd_max = 1;
    may_miss = may_resist = may_glance = may_block = may_dodge = may_crit = false;
    base_attack_power_multiplier = 0;
    cooldown -> duration = 10;

    id = 1766;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return rogue_attack_t::ready();
  }
};

// Killing Spree ============================================================

struct killing_spree_tick_t : public rogue_attack_t
{
  killing_spree_tick_t( player_t* player ) :
      rogue_attack_t( "killing_spree", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();

    dual        = true;
    background  = true;
    may_crit    = true;
    direct_tick = true;
    base_dd_min = base_dd_max = 1;
    base_multiplier *= 1.0 + ( p -> talents.find_weakness * 0.02 +
                               p -> talents.slaughter_from_the_shadows * 0.01 );
  }

  virtual void execute()
  {
    rogue_attack_t::execute();
    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );
  }
};

struct killing_spree_t : public rogue_attack_t
{
  attack_t* killing_spree_tick;

  killing_spree_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "killing_spree", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.killing_spree );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_min = base_dd_max = 1;
    channeled = true;
    num_ticks = 5;
    base_tick_time = 0.5;
    cooldown -> duration = 120;

    if ( p -> glyphs.killing_spree ) cooldown -> duration -= 45;

    killing_spree_tick = new killing_spree_tick_t( p );

    id = 51690;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::execute();
    p -> buffs_killing_spree -> trigger();
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }

  virtual void tick()
  {
    if ( sim -> debug )
      log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    killing_spree_tick -> weapon = &( player -> main_hand_weapon );
    killing_spree_tick -> execute();

    if ( killing_spree_tick -> result_is_hit() )
    {
      if ( player -> off_hand_weapon.type != WEAPON_NONE )
      {
        killing_spree_tick -> weapon = &( player -> off_hand_weapon );
        killing_spree_tick -> execute();
      }
    }

    update_time( DMG_OVER_TIME );
  }

  virtual void last_tick()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::last_tick();
    p -> buffs_killing_spree -> expire();
  }

  // Killing Spree not modified by haste effects
  virtual double haste() SC_CONST { return 1.0; }
};

// Mutilate =================================================================

struct mutilate_t : public rogue_attack_t
{
  mutilate_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "mutilate", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.mutilate );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 6, 181, 181, 0, 60 },
      { 75, 5, 153, 153, 0, 60 },
      { 70, 4, 101, 101, 0, 60 },
      { 60, 3,  88,  88, 0, 60 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48666 );

    may_crit               = true;
    requires_weapon        = WEAPON_DAGGER;
    weapon                 = &( p -> main_hand_weapon );
    adds_combo_points      = true;
    normalize_weapon_speed = true;

    base_multiplier  *= 1.0 + ( p -> talents.find_weakness       * 0.02 +
                                p -> talents.opportunity         * 0.10 +
                                p -> set_bonus.tier6_4pc_melee() * 0.06 +
                                p -> talents.slaughter_from_the_shadows * 0.01 );

    base_crit += ( p -> talents.puncturing_wounds * 0.05 +
                   p -> talents.turn_the_tables   * 0.02 );

    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += .05;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;

    if ( p -> glyphs.mutilate ) base_cost -= 5;
  }

  virtual void execute()
  {
    attack_t::consume_resource();

    weapon = &( player -> main_hand_weapon );
    rogue_attack_t::execute();

    if ( result_is_hit() )
    {
      weapon = &( player -> off_hand_weapon );
      rogue_attack_t::execute();
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::player_buff();
    if ( sim -> target -> debuffs.poisoned ) player_multiplier *= 1.20;
    p -> uptimes_poisoned -> update( sim -> target -> debuffs.poisoned > 0 );
    trigger_dirty_deeds( this );
  }

  virtual void consume_resource() { }

  virtual bool ready()
  {
    // Mutilate NEEDS a dagger in the off-hand!
    if ( player -> off_hand_weapon.type != WEAPON_DAGGER )
      return false;

    return rogue_attack_t::ready();
  }
};

// Premeditation =============================================================

struct premeditation_t : public rogue_attack_t
{
  premeditation_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "premeditation", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    requires_stealth = true;
    cooldown -> duration = 20;

    id = 14183;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    add_combo_point( p );
    add_combo_point( p );
    update_ready();
  }
};

// Rupture ==================================================================

struct rupture_t : public rogue_attack_t
{
  double* combo_point_dmg;

  rupture_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "rupture", player, SCHOOL_BLEED, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0;

    may_crit              = false;
    requires_combo_points = true;
    base_cost             = 25;
    base_tick_time        = 2.0;
    num_ticks             = 3;
    base_multiplier      *= 1.0 + ( p -> talents.blood_spatter       * 0.15 +
                                    p -> talents.find_weakness       * 0.02 +
                                    p -> talents.serrated_blades     * 0.10 +
                                    p -> set_bonus.tier7_2pc_melee() * 0.10 +
                                    p -> talents.slaughter_from_the_shadows * 0.01 );

    if ( p -> talents.surprise_attacks ) may_dodge = false;

    if ( p -> set_bonus.tier8_4pc_melee() ) tick_may_crit = true;

    tick_may_crit = true;

    static double dmg_79[] = { 145, 163, 181, 199, 217 };
    static double dmg_74[] = { 122, 137, 152, 167, 182 };
    static double dmg_68[] = {  81,  92, 103, 114, 125 };
    static double dmg_60[] = {  68,  76,  84,  92, 100 };

    combo_point_dmg = ( p -> level >= 79 ? dmg_79 :
                        p -> level >= 74 ? dmg_74 :
                        p -> level >= 68 ? dmg_68 :
                        dmg_60 );

    observer = &( p -> active_rupture );

    id = 48672;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    // Save CP, because execute() will delete them
    int cp = p -> buffs_combo_points -> current_stack;
    rogue_attack_t::execute();
    if( result_is_hit() )
    {
      added_ticks = 0;
      num_ticks = 3 + cp;
      if ( p -> glyphs.rupture ) num_ticks += 2;
      update_ready();
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    tick_power_mod = p -> combo_point_rank( 0.015, 0.024, 0.030, 0.03428571, 0.0375 );
    base_td_init   = p -> combo_point_rank( combo_point_dmg );
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }

  virtual void tick()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::tick();
    p -> buffs_tier9_2pc -> trigger();
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_attack_t
{
  shadowstep_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "shadowstep", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    base_cost = 10;
    cooldown -> duration = 30;

    cooldown -> duration -= p -> talents.filthy_tricks * 5;
    base_cost            -= p -> talents.filthy_tricks * 5;

    id = 36554;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_shadowstep -> trigger();
    update_ready();
  }
};

// Shiv =====================================================================

struct shiv_t : public rogue_attack_t
{
  shiv_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "shiv", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( p -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE ) background = true; // Do not allow execution.

    base_cost = 20 + weapon -> swing_time * 10.0;
    adds_combo_points = true;
    base_dd_min = base_dd_max = 1;
    base_multiplier *= 1.0 + ( p -> talents.find_weakness    * 0.02 +
                               p -> talents.surprise_attacks * 0.10 +
                               p -> talents.slaughter_from_the_shadows * 0.01 );
    base_crit += p -> talents.turn_the_tables * 0.02;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;

    may_crit = false;

    id = 5938;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    p -> buffs_shiv -> trigger();
    rogue_attack_t::execute();
    p -> buffs_shiv -> expire();
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }
};

// Sinister Strike ==========================================================

struct sinister_strike_t : public rogue_attack_t
{
  sinister_strike_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "sinister_strike", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 12, 180, 180, 0, 45 },
      { 76, 11, 150, 150, 0, 45 },
      { 70, 10,  98,  98, 0, 45 },
      { 62,  9,  80,  80, 0, 45 },
      { 54,  8,  68,  68, 0, 45 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 48638 );

    weapon = &( p -> main_hand_weapon );
    may_crit               = true;
    normalize_weapon_speed = true;
    adds_combo_points      = true;

    base_cost -= util_t::talent_rank( p -> talents.improved_sinister_strike, 2, 3.0, 5.0 );

    base_multiplier *= 1.0 + ( p -> talents.aggression          * 0.03 +
                               p -> talents.blade_twisting      * 0.05 +
                               p -> talents.find_weakness       * 0.02 +
                               p -> talents.surprise_attacks    * 0.10 +
                               p -> set_bonus.tier6_4pc_melee() * 0.06 +
                               p -> talents.slaughter_from_the_shadows * 0.01 );

    base_crit += p -> talents.turn_the_tables * 0.02;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;

    if ( p -> set_bonus.tier9_4pc_melee() ) base_crit += .05;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::execute();
    if ( result == RESULT_CRIT )
    {
      if ( p -> glyphs.sinister_strike &&
           p -> rng_sinister_strike_glyph -> roll( 0.50 ) )
      {
        add_combo_point( p );
      }
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::player_buff();
    if ( weapon -> type <= WEAPON_SMALL ) player_crit += p -> talents.close_quarters_combat * 0.01;
    trigger_dirty_deeds( this );
  }
};

// Slice and Dice ===========================================================

struct slice_and_dice_t : public rogue_attack_t
{
  slice_and_dice_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "slice_and_dice", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    requires_combo_points = true;
    base_cost = 25;

    id = 6774;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    trigger_slice_and_dice( p, p -> buffs_combo_points -> current_stack );
    consume_resource();
    trigger_ruthlessness( this );
  }
};

// Pool Energy ==============================================================

struct pool_energy_t : public rogue_attack_t
{
  double wait;
  int for_next;

  pool_energy_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "pool_energy", player ), wait( 0.5 ), for_next( 0 )
  {
    option_t options[] =
    {
      { "wait",     OPT_FLT,  &wait     },
      { "for_next", OPT_BOOL, &for_next },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
  }

  virtual double gcd() SC_CONST
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

      if ( ! energy_limited ) return false;
    }

    return rogue_attack_t::ready();
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public rogue_attack_t
{
  shadow_dance_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "shadow_dance", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.shadow_dance );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown -> duration = 60;

    id = 51713;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_shadow_dance -> duration = p -> glyphs.shadow_dance ? 8.0 : 6.0;
    p -> buffs_shadow_dance -> trigger();
  }
};

// Tricks of the Trade =======================================================

struct tricks_of_the_trade_t : public rogue_attack_t
{
  player_t* tricks_target;
  bool unspecified_target;

  tricks_of_the_trade_t( player_t* player, const std::string& options_str ) :
    rogue_attack_t( "tricks_target", player, SCHOOL_PHYSICAL, TREE_SUBTLETY ),
    unspecified_target( false )
  {
    rogue_t* p = player -> cast_rogue();

    std::string target_str = p -> tricks_of_the_trade_target_str;

    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;

    base_cost = 15;
    base_cost -= p -> talents.filthy_tricks * 5.0;
    if ( p -> set_bonus.tier10_2pc_melee() ) base_cost = 0;

    cooldown -> duration  = 30.0 - p -> talents.filthy_tricks * 5.0;

    if ( target_str.empty() || target_str == "none" )
    {
      tricks_target = 0;
    }
    else if ( target_str == "other" )
    {
      tricks_target = 0;
      unspecified_target = true;
    }
    else if ( target_str == "self" )
    {
      tricks_target = p;
    }
    else
    {
      tricks_target = sim -> find_player( target_str );

      assert ( tricks_target != 0 );
      assert ( tricks_target != player );
    }

    id = 57934;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    consume_resource();

    if ( p -> set_bonus.tier10_2pc_melee() )
      p -> resource_gain( RESOURCE_ENERGY, 15, p -> gains_tier10_2pc );

    update_ready();

    if ( ! unspecified_target )
    {
      if ( sim -> log ) log_t::output( sim, "%s grants %s Tricks of the Trade", p -> name(), tricks_target -> name() );

      p -> tricks_of_the_trade_target = tricks_target;
    }
  }

  virtual bool ready()
  {
    if ( ! unspecified_target )
    {
      if ( ! tricks_target ) return false;

      if ( tricks_target -> buffs.tricks_of_the_trade -> check() )
        return false;
    }

    return rogue_attack_t::ready();
  }
};

// Vanish ===================================================================

struct vanish_t : public rogue_attack_t
{
  vanish_t( player_t* player, const std::string& options_str ) :
      rogue_attack_t( "vanish", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    rogue_t* p = player -> cast_rogue();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown -> duration = 180 - 30 * p -> talents.elusiveness;

    id = 26889;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_stealthed -> trigger();
    update_ready();
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();
    if ( p -> buffs_stealthed -> check() )
      return false;
    return rogue_attack_t::ready();
  }
};

// =========================================================================
// Rogue Poisons
// =========================================================================

// rogue_poison_t::player_buff =============================================

void rogue_poison_t::player_buff()
{
  rogue_t* p = player -> cast_rogue();

  spell_t::player_buff();

  if ( p -> buffs_hunger_for_blood -> up() )
  {
    player_multiplier *= 1.05 + p -> glyphs.hunger_for_blood * 0.03;
  }
  if ( p -> buffs_master_of_subtlety -> check() )
  {
    player_multiplier *= 1.0 + p -> buffs_master_of_subtlety -> value();
  }
  if ( p -> talents.murder )
  {
    int target_race = sim -> target -> race;

    if ( target_race == RACE_BEAST || target_race == RACE_DRAGONKIN ||
         target_race == RACE_GIANT || target_race == RACE_HUMANOID  )
    {
      player_multiplier *= 1.0 + p -> talents.murder * 0.02;
    }
  }
  if ( p -> buffs_killing_spree -> up() )
  {
    player_multiplier *= 1.20;
  }
  if ( p -> talents.prey_on_the_weak )
  {
    bool active = sim -> target -> health_percentage() <= 100.0;
    if ( active ) player_crit_multiplier *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    p -> uptimes_prey_on_the_weak -> update( active );
  }
}

// Anesthetic Poison ========================================================

struct anesthetic_poison_t : public rogue_poison_t
{
  anesthetic_poison_t( player_t* player ) : rogue_poison_t( "anesthetic_poison", player )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd      = 0;
    background       = true;
    may_crit         = true;
    direct_power_mod = 0;
    base_dd_min      = util_t::ability_rank( p -> level,  218,77,  134,68,  0,0 );
    base_dd_max      = util_t::ability_rank( p -> level,  280,77,  172,68,  0,0 );

    id = 57982;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    double chance = ( p -> buffs_shiv -> check() || p -> buffs_deadly_proc -> check() ) ? 1.0 : .5;
    may_crit = ( p -> buffs_shiv -> check() ) ? false : true;
    if ( p -> rng_anesthetic_poison -> roll( chance ) )
    {
      rogue_poison_t::execute();
    }
  }
};

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_poison_t
{
  deadly_poison_t( player_t* player ) :
      rogue_poison_t( "deadly_poison", player )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd    = 0;
    background     = true;
    num_ticks      = 4;
    base_tick_time = 3.0;
    tick_power_mod = 0.12 / num_ticks * 0.9; // * 0.9 for the 10% Hot-Fix nerf on Jan 7th. 2010.
    base_td_init   = util_t::ability_rank( p -> level,  296,80,  244,76,  204,70,  160,62,  96,0  ) / num_ticks;

    id = 57972;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int success = p -> buffs_shiv -> check() || p -> buffs_deadly_proc -> check();

    if ( ! success )
    {
      double chance = 0.30;
      chance += p -> talents.improved_poisons * 0.04;
      if ( p -> buffs_envenom -> up() )
      {
        chance += 0.15;
      }
      success = p -> rng_deadly_poison -> roll( chance );
    }

    if ( success )
    {
      p -> procs_deadly_poison -> occur();

      player_buff();
      target_debuff( DMG_DIRECT );
      calculate_result();

      if ( result_is_hit() )
      {
        if ( p -> buffs_poison_doses -> check() == 5 )
        {
          p -> buffs_deadly_proc -> trigger();
          weapon_t* other_w = ( weapon -> slot == SLOT_MAIN_HAND ) ? &( p -> off_hand_weapon ) : &( p -> main_hand_weapon );
          trigger_other_poisons( p, other_w );
          p -> buffs_deadly_proc -> expire();
        }

        p -> buffs_poison_doses -> trigger();

        if ( sim -> log ) log_t::output( sim, "%s performs %s (%d)", player -> name(), name(), p -> buffs_poison_doses -> current_stack );

        if ( ticking )
        {
          refresh_duration();
        }
        else
        {
          schedule_tick();
          apply_poison_debuff( p );
        }
      }
    }
  }

  virtual double calculate_tick_damage()
  {
    rogue_t* p = player -> cast_rogue();
    tick_dmg = rogue_poison_t::calculate_tick_damage();
    tick_dmg *= p -> buffs_poison_doses -> stack();
    return tick_dmg;
  }

  virtual void tick()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_poison_t::tick();
    if ( p -> set_bonus.tier8_2pc_melee() )
    {
      p -> resource_gain( RESOURCE_ENERGY, 1, p -> gains_tier8_2pc );
    }
  }

  virtual void last_tick()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_poison_t::last_tick();
    p -> buffs_poison_doses -> expire();
    remove_poison_debuff( p );
  }
};

// Instant Poison ===========================================================

struct instant_poison_t : public rogue_poison_t
{
  instant_poison_t( player_t* player ) :
      rogue_poison_t( "instant_poison", player )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd      = 0;
    background       = true;
    may_crit         = true;
    direct_power_mod = 0.10 * 0.9; // * 0.9 for the Hot-Fix nerf on Jan. 7th 2010.
    base_dd_min      = util_t::ability_rank( p -> level,  300,79,  245,73,  161,68,  76,0 );
    base_dd_max      = util_t::ability_rank( p -> level,  400,79,  327,73,  215,68,  100,0 );

    id = 57967;
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
      double PPM = 8.57;
      PPM *= 1.0 + ( p -> talents.improved_poisons * 0.10 ) + ( p -> buffs_envenom -> up() ? 0.75 : 0.00 );
      chance = weapon -> proc_chance_on_swing( PPM );
      may_crit = true;
    }
    if ( p -> rng_instant_poison -> roll( chance ) )
    {
      rogue_poison_t::execute();
    }
  }
};

// Wound Poison ============================================================

struct wound_poison_t : public rogue_poison_t
{
  wound_poison_t( player_t* player ) : rogue_poison_t( "wound_poison", player )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd      = 0;
    background       = true;
    may_crit         = true;
    direct_power_mod = .04 * 0.9; // * 0.9 for the Hot-Fix nerf on Jan. 7th, 2010.
    base_dd_min = base_dd_max = util_t::ability_rank( p -> level,  231,78,  188,72,  112,64,  53,0 );

    id = 57978;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p ) : event_t( sim, p )
      {
        name = "Wound Poison Expiration";
        apply_poison_debuff( p );
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> _expirations.wound_poison = 0;
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
        event_t*& e = p -> _expirations.wound_poison;

        if ( e )
        {
          e -> reschedule( 15.0 );
        }
        else
        {
          e = new ( sim ) expiration_t( sim, p );
        }
      }
    }
  }
};

// Apply Poison ===========================================================

struct apply_poison_t : public rogue_poison_t
{
  int main_hand_poison;
  int  off_hand_poison;

  apply_poison_t( player_t* player, const std::string& options_str ) :
      rogue_poison_t( "apply_poison", player ),
      main_hand_poison(0), off_hand_poison(0)
  {
    rogue_t* p = player -> cast_rogue();

    std::string main_hand_str;
    std::string  off_hand_str;

    option_t options[] =
    {
      { "main_hand", OPT_STRING, &main_hand_str },
      {  "off_hand", OPT_STRING,  &off_hand_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( main_hand_str.empty() &&
         off_hand_str.empty() )
    {
      sim -> errorf( "Player %s: At least one of 'main_hand/off_hand' must be specified in 'apply_poison'.  Accepted values are 'anesthetic/deadly/instant/wound'.\n", p -> name() );
      sim -> cancel();
    }

    trigger_gcd = 0;

    if ( p -> main_hand_weapon.type != WEAPON_NONE )
    {
      if ( main_hand_str == "anesthetic" ) main_hand_poison = ANESTHETIC_POISON;
      if ( main_hand_str == "deadly"     ) main_hand_poison =     DEADLY_POISON;
      if ( main_hand_str == "instant"    ) main_hand_poison =    INSTANT_POISON;
      if ( main_hand_str == "wound"      ) main_hand_poison =      WOUND_POISON;
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( off_hand_str == "anesthetic" ) off_hand_poison = ANESTHETIC_POISON;
      if ( off_hand_str == "deadly"     ) off_hand_poison =     DEADLY_POISON;
      if ( off_hand_str == "instant"    ) off_hand_poison =    INSTANT_POISON;
      if ( off_hand_str == "wound"      ) off_hand_poison =      WOUND_POISON;
    }

    if ( ! p -> active_anesthetic_poison ) p -> active_anesthetic_poison = new anesthetic_poison_t( p );
    if ( ! p -> active_deadly_poison     ) p -> active_deadly_poison     = new     deadly_poison_t( p );
    if ( ! p -> active_instant_poison    ) p -> active_instant_poison    = new    instant_poison_t( p );
    if ( ! p -> active_wound_poison      ) p -> active_wound_poison      = new      wound_poison_t( p );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    if ( main_hand_poison ) p -> main_hand_weapon.buff_type = main_hand_poison;
    if (  off_hand_poison ) p ->  off_hand_weapon.buff_type =  off_hand_poison;
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

    return false;
  }
};

// =========================================================================
// Rogue Spells
// =========================================================================

// Cold Blood ==============================================================

struct cold_blood_t : public spell_t
{
  cold_blood_t( player_t* player, const std::string& options_str ) :
      spell_t( "cold_blood", player )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.cold_blood );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown -> duration = 180;

    id = 14177;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_cold_blood -> trigger();
    update_ready();
  }
};

// Preparation ==============================================================

struct preparation_t : public spell_t
{
  std::vector<cooldown_t*> cooldown_list;

  preparation_t( player_t* player, const std::string& options_str ) :
      spell_t( "preparation", player )
  {
    rogue_t* p = player -> cast_rogue();
    check_talent( p -> talents.preparation );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;

    cooldown -> duration = 600;
    cooldown -> duration -= p -> talents.filthy_tricks * 150;

    cooldown_list.push_back( p -> get_cooldown( "cold_blood" ) );
    cooldown_list.push_back( p -> get_cooldown( "vanish"     ) );

    if ( p -> glyphs.preparation )
    {
      cooldown_list.push_back( p -> get_cooldown( "blade_flurry" ) );
    }

    id = 14185;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    int num_cooldowns = (int) cooldown_list.size();
    for ( int i=0; i < num_cooldowns; i++ )
    {
      cooldown_list[ i ] -> reset();
    }
    update_ready();
  }
};

// Stealth ==================================================================

struct stealth_t : public spell_t
{
  bool used;

  stealth_t( player_t* player, const std::string& options_str ) :
    spell_t( "stealth", player ), used(false)
  {
    trigger_gcd = 0;

    id = 1784;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
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

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Rogue Character Definition
// =========================================================================

// rogue_t::composite_attack_crit ==========================================

double rogue_t::composite_attack_crit() SC_CONST
{
  double ac = player_t::composite_attack_crit();
  if( talents.malice ) ac += talents.malice * 0.01;
  return ac;
}

// rogue_t::composite_tank_miss ============================================

double rogue_t::composite_tank_miss( int school ) SC_CONST
{
  double m = player_t::composite_tank_miss( school );

  if ( school != SCHOOL_PHYSICAL )
  {
    m += talents.heightened_senses * 0.02;
  }

  if      ( m > 1.0 ) m = 1.0;
  else if ( m < 0.0 ) m = 0.0;

  return m;
}

// rogue_t::primary_tree ===================================================

int rogue_t::primary_tree() SC_CONST
{
  if ( talents.mutilate ) return TREE_ASSASSINATION;
  if ( talents.adrenaline_rush || talents.killing_spree ) return TREE_COMBAT;
  if ( talents.honor_among_thieves ) return TREE_SUBTLETY;

  return TALENT_TREE_MAX;
}

// rogue_t::init_actions ===================================================

void rogue_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    action_list_str += "flask,type=endless_rage";
    action_list_str += "/food,type=";
    action_list_str += ( primary_tree() == TREE_COMBAT ) ? "imperial_manta_steak" : "mega_mammoth_meal";

    action_list_str += "/apply_poison";
    std::string slow_hand = "main_hand";
    std::string fast_hand = "off_hand";
    if( main_hand_weapon.swing_time <= off_hand_weapon.swing_time ) std::swap( slow_hand, fast_hand );
    action_list_str += "," + slow_hand + "=";
    action_list_str += ( talents.improved_poisons > 2 ) ? "instant" : "wound";
    action_list_str += "," + fast_hand + "=deadly";
    action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<20";
    action_list_str += "/auto_attack";
    action_list_str += "/snapshot_stats";
    if ( talents.overkill || talents.master_of_subtlety ) action_list_str += "/stealth";
    action_list_str += "/kick";
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    if ( race == RACE_ORC )
    {
      action_list_str += "/blood_fury";
    }
    else if ( race == RACE_TROLL )
    {
      action_list_str += "/berserking";
    }
    else if ( race == RACE_BLOOD_ELF )
    {
      action_list_str += "/arcane_torrent";
    }
    if ( primary_tree() == TREE_ASSASSINATION )
    {
      bool rupture_less = ( ! talents.blood_spatter   &&
                            ! talents.serrated_blades &&
                            ! glyphs.rupture );
      if ( talents.hunger_for_blood )
      {
        action_list_str += "/pool_energy,for_next=1";
        action_list_str += "/hunger_for_blood,if=buff.hunger_for_blood.remains<2";
      }
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.remains<1";
      if ( ! talents.cut_to_the_chase )
      {
        action_list_str += "/pool_energy,if=energy<60&buff.slice_and_dice.remains<5";
        action_list_str += "/slice_and_dice,if=buff.combo_points.stack>=3&buff.slice_and_dice.remains<2";
      }
      if ( ! rupture_less )
      {
        action_list_str += "/rupture,if=buff.combo_points.stack>=4&target.time_to_die>15&time>10&buff.slice_and_dice.remains>11";
      }
      if ( talents.cold_blood ) action_list_str += "/cold_blood,sync=envenom";
      action_list_str += "/envenom,if=buff.combo_points.stack>=4&buff.envenom.down";
      action_list_str += "/envenom,if=buff.combo_points.stack>=4&energy>90";
      action_list_str += "/envenom,if=buff.combo_points.stack>=2&buff.slice_and_dice.remains<2";
      action_list_str += "/tricks_of_the_trade";
      action_list_str += "/mutilate,if=buff.combo_points.stack<4";
      if ( talents.overkill ) action_list_str += "/vanish,if=time>30&energy>50";
    }
    else if ( primary_tree() == TREE_COMBAT )
    {
      bool rupture_less = (   false                       &&
                            ! talents.blood_spatter       &&
                            ! talents.serrated_blades     &&
                              talents.improved_eviscerate &&
                              talents.aggression          &&
                            ! glyphs.rupture );
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.down&time<4";
      action_list_str += "/slice_and_dice,if=buff.slice_and_dice.remains<2&buff.combo_points.stack>=3";
      action_list_str += "/tricks_of_the_trade";
      if ( talents.killing_spree )
      {
        action_list_str += "/killing_spree,if=energy<20&buff.slice_and_dice.remains>5";
      }
      if ( talents.blade_flurry  )
      {
        action_list_str += "/blade_flurry,if=target.adds_never&buff.slice_and_dice.remains>=5";
        action_list_str += "/blade_flurry,if=target.adds>0";
      }
      if ( ! rupture_less )
      {
        action_list_str += "/rupture,if=buff.combo_points.stack=5&target.time_to_die>10";
      }
      if (   talents.puncturing_wounds        &&
             talents.opportunity              &&
             talents.close_quarters_combat    &&
           ! talents.improved_sinister_strike &&
           ! talents.improved_eviscerate      &&
           ( main_hand_weapon.type == WEAPON_DAGGER ) )
      {
        action_list_str += "/backstab";
      }
      else
      {
        if ( rupture_less )
        {
          action_list_str += "/eviscerate,if=buff.combo_points.stack=5&buff.slice_and_dice.remains>7";
          action_list_str += "/eviscerate,if=buff.combo_points.stack>=4&buff.slice_and_dice.remains>4&energy>40";
        }
        else
        {
          action_list_str += "/eviscerate,if=buff.combo_points.stack=5&buff.slice_and_dice.remains>7&dot.rupture.remains>6";
          action_list_str += "/eviscerate,if=buff.combo_points.stack>=4&buff.slice_and_dice.remains>4&energy>40&dot.rupture.remains>5";
        }
        action_list_str += "/eviscerate,if=buff.combo_points.stack=5&target.time_to_die<10";
        action_list_str += "/sinister_strike,if=buff.combo_points.stack<5";
      }
      if ( talents.adrenaline_rush ) action_list_str += "/adrenaline_rush,if=energy<20";
    }
    else if ( primary_tree() == TREE_SUBTLETY )
    {
      action_list_str += "/pool_energy,for_next=1/slice_and_dice,if=buff.combo_points.stack>=4&buff.slice_and_dice.remains<3";
      action_list_str += "/tricks_of_the_trade";
      // CP conditionals track reaction time, so responding when you see CP=4 will often result in CP=5 finishers
      action_list_str += "/rupture,if=buff.combo_points.stack>=4";
      if ( talents.improved_poisons )
      {
        action_list_str += "/envenom,if=buff.combo_points.stack>=4&buff.envenom.remains<1";
      }
      action_list_str += "/eviscerate,if=buff.combo_points.stack>=4";
      action_list_str += talents.hemorrhage ? "/hemorrhage" : "/sinister_strike";
      action_list_str += ",if=buff.combo_points.stack<=2&energy>=80";
    }
    else
    {
      action_list_str += "/pool_energy,if=energy<60&buff.slice_and_dice.remains<5";
      action_list_str += "/slice_and_dice,if=buff.combo_points.stack>=3&buff.slice_and_dice.remains<2";
      action_list_str += "/sinister_strike,if=buff.combo_points.stack<5";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// rogue_t::create_action  =================================================

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
  if ( name == "garrote"             ) return new garrote_t            ( this, options_str );
  if ( name == "ghostly_strike"      ) return new ghostly_strike_t     ( this, options_str );
  if ( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if ( name == "hunger_for_blood"    ) return new hunger_for_blood_t   ( this, options_str );
  if ( name == "kick"                ) return new kick_t               ( this, options_str );
  if ( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if ( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if ( name == "pool_energy"         ) return new pool_energy_t        ( this, options_str );
  if ( name == "premeditation"       ) return new premeditation_t      ( this, options_str );
  if ( name == "preparation"         ) return new preparation_t        ( this, options_str );
  if ( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if ( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if ( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if ( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if ( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
  if ( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if ( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if ( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if ( name == "tricks_of_the_trade" ) return new tricks_of_the_trade_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::init_glyphs ======================================================

void rogue_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "adrenaline_rush"     ) glyphs.adrenaline_rush = 1;
    else if ( n == "backstab"            ) glyphs.backstab = 1;
    else if ( n == "blade_flurry"        ) glyphs.blade_flurry = 1;
    else if ( n == "eviscerate"          ) glyphs.eviscerate = 1;
    else if ( n == "expose_armor"        ) glyphs.expose_armor = 1;
    else if ( n == "feint"               ) glyphs.feint = 1;
    else if ( n == "ghostly_strike"      ) glyphs.ghostly_strike = 1;
    else if ( n == "hemorrhage"          ) glyphs.hemorrhage = 1;
    else if ( n == "hunger_for_blood"    ) glyphs.hunger_for_blood = 1;
    else if ( n == "killing_spree"       ) glyphs.killing_spree = 1;
    else if ( n == "mutilate"            ) glyphs.mutilate = 1;
    else if ( n == "preparation"         ) glyphs.preparation = 1;
    else if ( n == "rupture"             ) glyphs.rupture = 1;
    else if ( n == "shadow_dance"        ) glyphs.shadow_dance = 1;
    else if ( n == "sinister_strike"     ) glyphs.sinister_strike = 1;
    else if ( n == "slice_and_dice"      ) glyphs.slice_and_dice = 1;
    else if ( n == "tricks_of_the_trade" ) glyphs.tricks_of_the_trade = 1;
    else if ( n == "vigor"               ) glyphs.vigor = 1;
    // To prevent warning messages....
    else if ( n == "blurred_speed"       ) ;
    else if ( n == "cloak_of_shadows"    ) ;
    else if ( n == "distract"            ) ;
    else if ( n == "fan_of_knives"       ) ;
    else if ( n == "garrote"             ) ;
    else if ( n == "pick_lock"           ) ;
    else if ( n == "pick_pocket"         ) ;
    else if ( n == "safe_fall"           ) ;
    else if ( n == "sprint"              ) ;
    else if ( n == "vanish"              ) ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// rogue_t::init_race ======================================================

void rogue_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_NIGHT_ELF:
  case RACE_GNOME:
  case RACE_UNDEAD:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_BLOOD_ELF:
    break;
  default:
    race = RACE_NIGHT_ELF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// rogue_t::init_base ========================================================

void rogue_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, level, ROGUE, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_AGILITY ] *= 1.0 + talents.sinister_calling * 0.03;

  base_attack_power = ( level * 2 ) - 20;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 1.0;
  initial_attack_power_multiplier  *= 1.0 + ( talents.savage_combat * 0.02 +
                                              talents.deadliness    * 0.02 );

  base_attack_expertise = talents.weapon_expertise * 0.05;

  resource_base[ RESOURCE_ENERGY ] = 100 + ( talents.vigor ? ( glyphs.vigor ? 20 : 10 ) : 0 );

  health_per_stamina       = 10;
  energy_regen_per_second  = 10;
  energy_regen_per_second *= 1.0 + util_t::talent_rank( talents.vitality, 3, 0.08, 0.16, 0.25 );

  base_gcd = 1.0;
}

// rogue_t::init_gains =======================================================

void rogue_t::init_gains()
{
  player_t::init_gains();

  gains_adrenaline_rush    = get_gain( "adrenaline_rush" );
  gains_combat_potency     = get_gain( "combat_potency" );
  gains_energy_refund      = get_gain( "energy_refund" );
  gains_focused_attacks    = get_gain( "focused_attacks" );
  gains_overkill           = get_gain( "overkill" );
  gains_quick_recovery     = get_gain( "quick_recovery" );
  gains_relentless_strikes = get_gain( "relentless_strikes" );
  gains_tier8_2pc          = get_gain( "tier8_2pc" );
  gains_tier9_2pc          = get_gain( "tier9_2pc" );
  gains_tier10_2pc         = get_gain( "tier10_2pc" );
}

// rogue_t::init_procs =======================================================

void rogue_t::init_procs()
{
  player_t::init_procs();

  procs_combo_points_wasted  = get_proc( "combo_points_wasted"  );
  procs_deadly_poison        = get_proc( "deadly_poisons"       );
  procs_honor_among_thieves  = get_proc( "honor_among_thieves"  );
  procs_ruthlessness         = get_proc( "ruthlessness"         );
  procs_seal_fate            = get_proc( "seal_fate"            );
  procs_sword_specialization = get_proc( "sword_specialization" );
  procs_tier10_4pc           = get_proc( "tier10_4pc"           );
}

// rogue_t::init_uptimes =====================================================

void rogue_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_energy_cap          = get_uptime( "energy_cap" );
  uptimes_poisoned            = get_uptime( "poisoned" );
  uptimes_rupture             = get_uptime( "rupture" );
  uptimes_prey_on_the_weak    = get_uptime( "prey_on_the_weak" );
}

// rogue_t::init_rng =========================================================

void rogue_t::init_rng()
{
  player_t::init_rng();

  rng_anesthetic_poison     = get_rng( "anesthetic_poison"     );
  rng_combat_potency        = get_rng( "combat_potency"        );
  rng_cut_to_the_chase      = get_rng( "cut_to_the_chase"      );
  rng_deadly_poison         = get_rng( "deadly_poison"         );
  rng_focused_attacks       = get_rng( "focused_attacks"       );
  rng_honor_among_thieves   = get_rng( "honor_among_thieves"   );
  rng_initiative            = get_rng( "initiative"            );
  rng_instant_poison        = get_rng( "instant_poison"        );
  rng_master_poisoner       = get_rng( "master_poisoner"       );
  rng_relentless_strikes    = get_rng( "relentless_strikes"    );
  rng_ruthlessness          = get_rng( "ruthlessness"          );
  rng_seal_fate             = get_rng( "seal_fate"             );
  rng_sinister_strike_glyph = get_rng( "sinister_strike_glyph" );
  rng_sword_specialization  = get_rng( "sword_specialization"  );
  rng_tier10_4pc            = get_rng( "tier10_4pc"            );
  rng_wound_poison          = get_rng( "wound_poison"          );

  // Overlapping procs require the use of a "distributed" RNG-stream when normalized_rng=1
  // also useful for frequent checks with low probability of proc and timed effect

  rng_critical_strike_interval = get_rng( "critical_strike_interval", RNG_DISTRIBUTED );
  rng_critical_strike_interval -> average_range = false;
}

// rogue_t::init_scaling ====================================================

void rogue_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
  scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = 1;
}

// rogue_t::init_buffs ======================================================

void rogue_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_adrenaline_rush    = new buff_t( this, "adrenaline_rush",    1, ( glyphs.adrenaline_rush ? 20.0 : 15.0 ) );
  buffs_blade_flurry       = new buff_t( this, "blade_flurry",       1, 15.0  );
  buffs_cold_blood         = new buff_t( this, "cold_blood",         1        );
  buffs_combo_points       = new buff_t( this, "combo_points",       5        );
  buffs_deadly_proc        = new buff_t( this, "deadly_proc",        1        );
  buffs_envenom            = new buff_t( this, "envenom",            1,  6.0  );
  buffs_hunger_for_blood   = new buff_t( this, "hunger_for_blood",   1, 60.0  );
  buffs_killing_spree      = new buff_t( this, "killing_spree",      1        );
  buffs_master_of_subtlety = new buff_t( this, "master_of_subtlety", 1,  6.0  );
  buffs_overkill           = new buff_t( this, "overkill",           1, 20.0  );
  buffs_poison_doses       = new buff_t( this, "poison_doses",       5        );
  buffs_shadow_dance       = new buff_t( this, "shadow_dance",       1,  6.0  );
  buffs_shadowstep         = new buff_t( this, "shadowstep",         1, 10.0  );
  buffs_shiv               = new buff_t( this, "shiv",               1        );
  buffs_stealthed          = new buff_t( this, "stealthed",          1        );
  buffs_slice_and_dice     = new buff_t( this, "slice_and_dice",     1        );
  buffs_tier9_2pc          = new buff_t( this, "tier9_2pc",          1, 10.0, 0.0, set_bonus.tier9_2pc_melee() * 0.02  );
}

// trigger_honor_among_thieves =============================================

struct honor_among_thieves_callback_t : public action_callback_t
{
  honor_among_thieves_callback_t( rogue_t* r ) : action_callback_t( r -> sim, r ) {}

  virtual void trigger( action_t* a )
  {
    rogue_t* rogue = listener -> cast_rogue();

    if ( a )
    {
      if ( ! a -> special || a -> aoe || a -> pseudo_pet )
        return;

      a -> player -> procs.hat_donor -> occur();
    }

    if ( rogue -> cooldowns_honor_among_thieves -> remains() > 0 )
      return;

    if ( ! rogue -> rng_honor_among_thieves -> roll( rogue -> talents.honor_among_thieves / 3.0 ) ) return;

    add_combo_point( rogue );

    rogue -> procs_honor_among_thieves -> occur();

    rogue -> cooldowns_honor_among_thieves -> start( 1.0 );
  }
};

// rogue_t::register_callbacks ===============================================

void rogue_t::register_callbacks()
{
  player_t::register_callbacks();

  if ( talents.honor_among_thieves )
  {
    action_callback_t* cb = new honor_among_thieves_callback_t( this );

    register_attack_result_callback( RESULT_CRIT_MASK, cb );
    register_spell_result_callback ( RESULT_CRIT_MASK, cb );

    if( party )
    {
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if ( p == this           ) continue;
        if ( p -> party != party ) continue;
        if ( p -> is_pet()       ) continue;

        cb = new honor_among_thieves_callback_t( this );

        p -> register_attack_result_callback( RESULT_CRIT_MASK, cb );
        p -> register_spell_result_callback ( RESULT_CRIT_MASK, cb );
      }
    }
    else // Virtual Party
    {
      std::vector<std::string> intervals;
      int num_intervals = util_t::string_split( intervals, critical_strike_intervals_str, ",;|/" );
      if( num_intervals == 0 )
      {
        intervals.push_back( "1.0" );
        num_intervals = 1;
      }
      while( num_intervals < 4 )
      {
        intervals.push_back( intervals[ num_intervals-1 ] );
        num_intervals++;
      }
      for( int i=0; i < num_intervals; i++ )
      {
        critical_strike_intervals.push_back( atof( intervals[ i ].c_str() ) );
        critical_strike_callbacks.push_back( new honor_among_thieves_callback_t( this ) );
      }
    }
  }
}

// rogue_t::combat_begin ===================================================

void rogue_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.honor_among_thieves )
  {
    if ( party == 0 ) // Virtual Party
    {
      int num_intervals = ( int ) critical_strike_intervals.size();

      for( int i=0; i < num_intervals; i++ )
      {
        struct critical_strike_t : public event_t
        {
          action_callback_t* callback;
          double interval;

          critical_strike_t( sim_t* sim, rogue_t* p, action_callback_t* cb, double i ) :
            event_t( sim, p ), callback(cb), interval(i)
          {
            name = "Critical Strike";
            double time = p -> rng_critical_strike_interval -> range( interval*0.5, interval*1.5 );
            sim -> add_event( this, time );
          }
          virtual void execute()
          {
            rogue_t* p = player -> cast_rogue();
            callback -> trigger( NULL );
            new ( sim ) critical_strike_t( sim, p, callback, interval );
          }
        };
        new ( sim ) critical_strike_t( sim, this, critical_strike_callbacks[ i ], critical_strike_intervals[ i ] );
      }
    }
  }
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  // Active
  active_rupture = 0;

  _expirations.reset();

  // Reset the callbacks for the Virtual Party
  int num_intervals = ( int ) critical_strike_callbacks.size();
  for( int i=0; i < num_intervals; i++ )
  {
    critical_strike_callbacks[ i ] -> reset();
  }

  tricks_of_the_trade_target = 0;
}

// rogue_t::interrupt ======================================================

void rogue_t::interrupt()
{
  player_t::interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
  if (  off_hand_attack )  off_hand_attack -> cancel();
}

// rogue_t::clear_debuffs ==================================================

void rogue_t::clear_debuffs()
{
  player_t::clear_debuffs();

  buffs_poison_doses -> expire();
  buffs_combo_points -> expire();
}

// rogue_t::regen ==========================================================

void rogue_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( buffs_adrenaline_rush -> up() )
  {
    if ( sim -> infinite_resource[ RESOURCE_ENERGY ] == 0 )
    {
      double energy_regen = periodicity * energy_regen_per_second;

      resource_gain( RESOURCE_ENERGY, energy_regen, gains_adrenaline_rush );
    }
  }
  if ( buffs_overkill -> up() )
  {
    if ( sim -> infinite_resource[ RESOURCE_ENERGY ] == 0 )
    {
      double energy_regen = periodicity * energy_regen_per_second * 0.30;

      resource_gain( RESOURCE_ENERGY, energy_regen, gains_overkill );
    }
  }

  uptimes_energy_cap -> update( resource_current[ RESOURCE_ENERGY ] ==
                                resource_max    [ RESOURCE_ENERGY ] );

  uptimes_rupture -> update( active_rupture != 0 );
}

// rogue_t::available ======================================================

double rogue_t::available() SC_CONST
{
  double energy = resource_current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return 0.1;

  return std::max( ( 25 - energy ) / energy_regen_per_second, 0.1 );
}

// rogue_t::get_talent_trees ==============================================

std::vector<talent_translation_t>& rogue_t::get_talent_list()
{
  if(talent_list.empty())
  {
          talent_translation_t translation_table[][MAX_TALENT_TREES] =
          {
    { {  1, 3, &( talents.improved_eviscerate   ) }, {  1, 0, NULL                                   }, {  1, 5, &( talents.relentless_strikes         ) } },
    { {  2, 0, NULL                               }, {  2, 2, &( talents.improved_sinister_strike  ) }, {  2, 0, NULL                                    } },
    { {  3, 5, &( talents.malice                ) }, {  3, 5, &( talents.dual_wield_specialization ) }, {  3, 2, &( talents.opportunity                ) } },
    { {  4, 3, &( talents.ruthlessness          ) }, {  4, 2, &( talents.improved_slice_and_dice   ) }, {  4, 2, &( talents.sleight_of_hand            ) } },
    { {  5, 2, &( talents.blood_spatter         ) }, {  5, 0, NULL                                   }, {  5, 0, NULL                                    } },
    { {  6, 3, &( talents.puncturing_wounds     ) }, {  6, 5, &( talents.precision                 ) }, {  6, 0, NULL                                    } },
    { {  7, 1, &( talents.vigor                 ) }, {  7, 0, NULL                                   }, {  7, 2, &( talents.elusiveness                ) } },
    { {  8, 2, &( talents.improved_expose_armor ) }, {  8, 0, NULL                                   }, {  8, 1, &( talents.ghostly_strike             ) } },
    { {  9, 5, &( talents.lethality             ) }, {  9, 5, &( talents.close_quarters_combat     ) }, {  9, 3, &( talents.serrated_blades            ) } },
    { { 10, 3, &( talents.vile_poisons          ) }, { 10, 0, NULL                                   }, { 10, 3, &( talents.setup                      ) } },
    { { 11, 5, &( talents.improved_poisons      ) }, { 11, 0, NULL                                   }, { 11, 3, &( talents.initiative                 ) } },
    { { 12, 0, NULL                               }, { 12, 3, &( talents.lightning_reflexes        ) }, { 12, 2, &( talents.improved_ambush            ) } },
    { { 13, 1, &( talents.cold_blood            ) }, { 13, 5, &( talents.aggression                ) }, { 13, 2, &( talents.heightened_senses          ) } },
    { { 14, 0, NULL                               }, { 14, 5, &( talents.mace_specialization       ) }, { 14, 1, &( talents.preparation                ) } },
    { { 15, 2, &( talents.quick_recovery        ) }, { 15, 1, &( talents.blade_flurry              ) }, { 15, 2, &( talents.dirty_deeds                ) } },
    { { 16, 5, &( talents.seal_fate             ) }, { 16, 5, &( talents.sword_specialization      ) }, { 16, 1, &( talents.hemorrhage                 ) } },
    { { 17, 2, &( talents.murder                ) }, { 17, 2, &( talents.weapon_expertise          ) }, { 17, 3, &( talents.master_of_subtlety         ) } },
    { { 18, 0, NULL                               }, { 18, 2, &( talents.blade_twisting            ) }, { 18, 5, &( talents.deadliness                 ) } },
    { { 19, 1, &( talents.overkill              ) }, { 19, 3, &( talents.vitality                  ) }, { 19, 0, NULL                                    } },
    { { 20, 0, NULL                               }, { 20, 1, &( talents.adrenaline_rush           ) }, { 20, 1, &( talents.premeditation              ) } },
    { { 21, 3, &( talents.focused_attacks       ) }, { 21, 0, NULL                                   }, { 21, 0, NULL                                    } },
    { { 22, 3, &( talents.find_weakness         ) }, { 22, 0, NULL                                   }, { 22, 5, &( talents.sinister_calling           ) } },
    { { 23, 3, &( talents.master_poisoner       ) }, { 23, 5, &( talents.combat_potency            ) }, { 23, 0, NULL                                    } },
    { { 24, 1, &( talents.mutilate              ) }, { 24, 0, NULL                                   }, { 24, 3, &( talents.honor_among_thieves        ) } },
    { { 25, 3, &( talents.turn_the_tables       ) }, { 25, 1, &( talents.surprise_attacks          ) }, { 25, 1, &( talents.shadowstep                 ) } },
    { { 26, 5, &( talents.cut_to_the_chase      ) }, { 26, 2, &( talents.savage_combat             ) }, { 26, 2, &( talents.filthy_tricks              ) } },
    { { 27, 1, &( talents.hunger_for_blood      ) }, { 27, 5, &( talents.prey_on_the_weak          ) }, { 27, 5, &( talents.slaughter_from_the_shadows ) } },
    { {  0, 0, NULL                               }, { 28, 1, &( talents.killing_spree             ) }, { 28, 1, &( talents.shadow_dance               ) } },
    { {  0, 0, NULL                               }, {  0, 0, NULL                                   }, {  0, 0, NULL                                    } }
  };

    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
  }
  return talent_list;
}

// rogue_t::get_options ================================================

std::vector<option_t>& rogue_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t rogue_options[] =
    {
      // @option_doc loc=skip
      { "adrenaline_rush",            OPT_INT, &( talents.adrenaline_rush            ) },
      { "aggression",                 OPT_INT, &( talents.aggression                 ) },
      { "blade_flurry",               OPT_INT, &( talents.blade_flurry               ) },
      { "blade_twisting",             OPT_INT, &( talents.blade_twisting             ) },
      { "blood_spatter",              OPT_INT, &( talents.blood_spatter              ) },
      { "close_quarters_combat",      OPT_INT, &( talents.close_quarters_combat      ) },
      { "cold_blood",                 OPT_INT, &( talents.cold_blood                 ) },
      { "combat_potency",             OPT_INT, &( talents.combat_potency             ) },
      { "cut_to_the_chase",           OPT_INT, &( talents.cut_to_the_chase           ) },
      { "deadliness",                 OPT_INT, &( talents.deadliness                 ) },
      { "dirty_deeds",                OPT_INT, &( talents.dirty_deeds                ) },
      { "dual_wield_specialization",  OPT_INT, &( talents.dual_wield_specialization  ) },
      { "elusiveness",                OPT_INT, &( talents.elusiveness                ) },
      { "filthy_tricks",              OPT_INT, &( talents.filthy_tricks              ) },
      { "find_weakness",              OPT_INT, &( talents.find_weakness              ) },
      { "focused_attacks",            OPT_INT, &( talents.focused_attacks            ) },
      { "ghostly_strike",             OPT_INT, &( talents.ghostly_strike             ) },
      { "hemorrhage",                 OPT_INT, &( talents.hemorrhage                 ) },
      { "honor_among_thieves",        OPT_INT, &( talents.honor_among_thieves        ) },
      { "hunger_for_blood",           OPT_INT, &( talents.hunger_for_blood           ) },
      { "improved_ambush",            OPT_INT, &( talents.improved_ambush            ) },
      { "improved_eviscerate",        OPT_INT, &( talents.improved_eviscerate        ) },
      { "improved_expose_armor",      OPT_INT, &( talents.improved_expose_armor      ) },
      { "improved_poisons",           OPT_INT, &( talents.improved_poisons           ) },
      { "improved_sinister_strike",   OPT_INT, &( talents.improved_sinister_strike   ) },
      { "improved_slice_and_dice",    OPT_INT, &( talents.improved_slice_and_dice    ) },
      { "initiative",                 OPT_INT, &( talents.initiative                 ) },
      { "killing_spree",              OPT_INT, &( talents.killing_spree              ) },
      { "heightened_senses",          OPT_INT, &( talents.heightened_senses          ) },
      { "lethality",                  OPT_INT, &( talents.lethality                  ) },
      { "lightning_reflexes",         OPT_INT, &( talents.lightning_reflexes         ) },
      { "mace_specialization",        OPT_INT, &( talents.mace_specialization        ) },
      { "malice",                     OPT_INT, &( talents.malice                     ) },
      { "master_of_subtlety",         OPT_INT, &( talents.master_of_subtlety         ) },
      { "master_poisoner",            OPT_INT, &( talents.master_poisoner            ) },
      { "murder",                     OPT_INT, &( talents.murder                     ) },
      { "mutilate",                   OPT_INT, &( talents.mutilate                   ) },
      { "opportunity",                OPT_INT, &( talents.opportunity                ) },
      { "overkill",                   OPT_INT, &( talents.overkill                   ) },
      { "precision",                  OPT_INT, &( talents.precision                  ) },
      { "premeditation",              OPT_INT, &( talents.premeditation              ) },
      { "preparation",                OPT_INT, &( talents.preparation                ) },
      { "prey_on_the_weak",           OPT_INT, &( talents.prey_on_the_weak           ) },
      { "puncturing_wounds",          OPT_INT, &( talents.puncturing_wounds          ) },
      { "quick_recovery",             OPT_INT, &( talents.quick_recovery             ) },
      { "relentless_strikes",         OPT_INT, &( talents.relentless_strikes         ) },
      { "ruthlessness",               OPT_INT, &( talents.ruthlessness               ) },
      { "savage_combat",              OPT_INT, &( talents.savage_combat              ) },
      { "seal_fate",                  OPT_INT, &( talents.seal_fate                  ) },
      { "serrated_blades",            OPT_INT, &( talents.serrated_blades            ) },
      { "setup",                      OPT_INT, &( talents.setup                      ) },
      { "shadow_dance",               OPT_INT, &( talents.shadow_dance               ) },
      { "shadowstep",                 OPT_INT, &( talents.shadowstep                 ) },
      { "sinister_calling",           OPT_INT, &( talents.sinister_calling           ) },
      { "slaughter_from_the_shadows", OPT_INT, &( talents.slaughter_from_the_shadows ) },
      { "sleight_of_hand",            OPT_INT, &( talents.sleight_of_hand            ) },
      { "surprise_attacks",           OPT_INT, &( talents.surprise_attacks           ) },
      { "sword_specialization",       OPT_INT, &( talents.sword_specialization       ) },
      { "turn_the_tables",            OPT_INT, &( talents.turn_the_tables            ) },
      { "vigor",                      OPT_INT, &( talents.vigor                      ) },
      { "vile_poisons",               OPT_INT, &( talents.vile_poisons               ) },
      { "vitality",                   OPT_INT, &( talents.vitality                   ) },
      { "weapon_expertise",           OPT_INT, &( talents.weapon_expertise           ) },
      // @option_doc loc=player/rogue/misc title="Misc"
      { "critical_strike_intervals",  OPT_STRING, &( critical_strike_intervals_str   ) },
      { "tricks_of_the_trade_target", OPT_STRING, &( tricks_of_the_trade_target_str ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, rogue_options );
  }

  return options;
}

// rogue_t::create_profile =================================================

bool rogue_t::create_profile( std::string& profile_str, int save_type )
{
  player_t::create_profile( profile_str, save_type );

  if ( save_type == SAVE_ALL || save_type == SAVE_ACTIONS )
  {
    if ( talents.honor_among_thieves )
    {
      profile_str += "# When using this profile with a real raid AND party setups, this parameter will be ignored.\n";
      profile_str += "# These values represent the avg donor intervals (unlimited by cooldown) of the Rogues's party members.\n";
      profile_str += "# This does not affect HAT procs generated by the Rogue himself.\n";
      profile_str += "critical_strike_intervals=" + critical_strike_intervals_str + "\n";
    }
  }

  return true;
}

// rogue_t::decode_set =====================================================

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

  if ( strstr( s, "bonescythe"  ) ) return SET_T7_MELEE;
  if ( strstr( s, "terrorblade" ) ) return SET_T8_MELEE;
  if ( strstr( s, "vancleefs"   ) ) return SET_T9_MELEE;
  if ( strstr( s, "garonas"     ) ) return SET_T9_MELEE;
  if ( strstr( s, "shadowblade" ) ) return SET_T10_MELEE;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_rogue  ==================================================

player_t* player_t::create_rogue( sim_t* sim, const std::string& name, int race_type )
{
  return new rogue_t( sim, name, race_type );
}

// player_t::rogue_init =====================================================

void player_t::rogue_init( sim_t* sim )
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.tricks_of_the_trade = new buff_t( p, "tricks_of_the_trade", 1, 15.0 );
  }

  target_t* t = sim -> target;
  t -> debuffs.expose_armor    = new debuff_t( sim, "expose_armor",     1 );
  t -> debuffs.hemorrhage      = new debuff_t( sim, "hemorrhage",      10, 15.0 );
  t -> debuffs.master_poisoner = new debuff_t( sim, "master_poisoner", -1 );
  t -> debuffs.poisoned        = new debuff_t( sim, "poisoned",        -1 );
  t -> debuffs.savage_combat   = new debuff_t( sim, "savage_combat",   -1 );
}

// player_t::rogue_combat_begin =============================================

void player_t::rogue_combat_begin( sim_t* sim )
{
  target_t* t = sim -> target;
  if( sim -> overrides.expose_armor    ) t -> debuffs.expose_armor    -> override( 1, 0.2 );
  if( sim -> overrides.master_poisoner ) t -> debuffs.master_poisoner -> override();
  if( sim -> overrides.poisoned        ) t -> debuffs.poisoned        -> override();
  if( sim -> overrides.savage_combat   ) t -> debuffs.savage_combat   -> override();
}

