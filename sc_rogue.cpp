// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Rogue
// ==========================================================================

struct rogue_t : public player_t
{
  // Active
  action_t* active_anesthetic_poison;
  action_t* active_deadly_poison;
  action_t* active_instant_poison;
  action_t* active_wound_poison;
  action_t* active_rupture;
  action_t* active_slice_and_dice;

  // Buffs
  int buffs_adrenaline_rush;
  int buffs_blade_flurry;
  int buffs_cold_blood;
  int buffs_combo_points;
  int buffs_envenom;
  int buffs_hunger_for_blood;
  int buffs_master_of_subtlety;
  int buffs_overkill;
  int buffs_poison_doses;
  int buffs_shadow_dance;
  int buffs_shadowstep;
  int buffs_shiv;
  int buffs_slice_and_dice;
  int buffs_stealthed;
  int buffs_tricks_ready;
  player_t* buffs_tricks_target;

  // Cooldowns
  double cooldowns_honor_among_thieves;
  double cooldowns_seal_fate;

  // Expirations
  event_t* expirations_envenom;
  event_t* expirations_slice_and_dice;
  event_t* expirations_hunger_for_blood;

  // Gains
  gain_t* gains_adrenaline_rush;
  gain_t* gains_combat_potency;
  gain_t* gains_focused_attacks;
  gain_t* gains_quick_recovery;
  gain_t* gains_relentless_strikes;

  // Procs
  proc_t* procs_combo_points;
  proc_t* procs_deadly_poison;
  proc_t* procs_honor_among_thieves;
  proc_t* procs_ruthlessness;
  proc_t* procs_seal_fate;
  proc_t* procs_sword_specialization;
  
  // Up-Times
  uptime_t* uptimes_blade_flurry;
  uptime_t* uptimes_energy_cap;
  uptime_t* uptimes_envenom;
  uptime_t* uptimes_hunger_for_blood;
  uptime_t* uptimes_poisoned;
  uptime_t* uptimes_slice_and_dice;
  uptime_t* uptimes_tricks_of_the_trade;
  
  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  // Options
  double      honor_among_thieves_interval;
  std::string tricks_of_the_trade_target_str;

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
    int filthy_tricks;
    int find_weakness;
    int focused_attacks;
    int ghostly_strike;
    int hemorrhage;
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
    int honor_among_thieves;

    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int adrenaline_rush;
    int blade_flurry;
    int eviscerate;
    int expose_armor;
    int feint;
    int ghostly_strike;
    int hemorrhage;
    int preparation;
    int rupture;
    int sinister_strike;
    int slice_and_dice;
    int vigor;

    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  rogue_t( sim_t* sim, std::string& name ) : player_t( sim, ROGUE, name )
  {
    // Active
    active_anesthetic_poison = 0;
    active_deadly_poison     = 0;
    active_instant_poison    = 0;
    active_wound_poison      = 0;
    active_rupture           = 0;
    active_slice_and_dice    = 0;

    // Buffs
    buffs_adrenaline_rush    = 0;
    buffs_blade_flurry       = 0;
    buffs_cold_blood         = 0;
    buffs_combo_points       = 0;
    buffs_envenom            = 0;
    buffs_hunger_for_blood   = 0;
    buffs_master_of_subtlety = 0;
    buffs_overkill           = 0;
    buffs_poison_doses       = 0;
    buffs_shadow_dance       = 0;
    buffs_shadowstep         = 0;
    buffs_shiv               = 0;
    buffs_slice_and_dice     = 0;
    buffs_stealthed          = 0;
    buffs_tricks_ready       = 0;
    buffs_tricks_target      = 0;

    // Cooldowns
    cooldowns_honor_among_thieves = 0;
    cooldowns_seal_fate           = 0;

    // Expirations
    expirations_envenom          = 0;
    expirations_slice_and_dice   = 0;
    expirations_hunger_for_blood = 0;
  
    // Gains
    gains_adrenaline_rush    = get_gain( "adrenaline_rush" );
    gains_combat_potency     = get_gain( "combat_potency" );
    gains_focused_attacks    = get_gain( "focused_attacks" );
    gains_quick_recovery     = get_gain( "quick_recovery" );
    gains_relentless_strikes = get_gain( "relentless_strikes" );

    // Procs
    procs_combo_points         = get_proc( "combo_points" );
    procs_deadly_poison        = get_proc( "deadly_poisons" );
    procs_honor_among_thieves  = get_proc( "honor_among_thieves" );
    procs_ruthlessness         = get_proc( "ruthlessness" );
    procs_seal_fate            = get_proc( "seal_fate" );
    procs_sword_specialization = get_proc( "sword_specialization" );

    // Up-Times
    uptimes_blade_flurry        = get_uptime( "blade_flurry" );
    uptimes_energy_cap          = get_uptime( "energy_cap" );
    uptimes_envenom             = get_uptime( "envenom" );
    uptimes_hunger_for_blood    = get_uptime( "hunger_for_blood" );
    uptimes_poisoned            = get_uptime( "poisoned" );
    uptimes_slice_and_dice      = get_uptime( "slice_and_dice" );
    uptimes_tricks_of_the_trade = get_uptime( "tricks_of_the_trade" );
  
    // Auto-Attack
    main_hand_attack = 0;
    off_hand_attack  = 0;

    // Options
    honor_among_thieves_interval = 0;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      raid_event( action_t* );
  virtual void      regen( double periodicity );
  virtual bool      get_talent_trees( std::vector<int*>& assassination, std::vector<int*>& combat, std::vector<int*>& subtlety );
  virtual bool      parse_talents_mmo( const std::string& talent_string );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       primary_resource() { return RESOURCE_ENERGY; }

  // Utilities 
  double combo_point_rank( double* cp_list )
  {
    assert( buffs_combo_points > 0 );
    return cp_list[ buffs_combo_points-1 ];
  }
  double combo_point_rank( double cp1, double cp2, double cp3, double cp4, double cp5 )
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return combo_point_rank( cp_list );
  }
};

namespace { // ANONYMOUS NAMESPACE =========================================

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

  rogue_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE ) : 
    attack_t( n, player, RESOURCE_ENERGY, s, t ), 
    requires_weapon(WEAPON_NONE),
    requires_position(POSITION_NONE),
    requires_stealth(false),
    requires_combo_points(false),
    adds_combo_points(false), 
    min_combo_points(0), max_combo_points(0),
    min_energy(0), max_energy(0), 
    min_hfb_expire(0), max_hfb_expire(0), 
    min_snd_expire(0), max_snd_expire(0), 
    min_rup_expire(0), max_rup_expire(0), 
    min_env_expire(0), max_env_expire(0)
  {
    rogue_t* p = player -> cast_rogue();
    base_crit += p -> talents.malice * 0.01;
    base_hit  += p -> talents.precision * 0.01;
    may_glance = false;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost();
  virtual void   execute();
  virtual void   player_buff();
  virtual double armor();
  virtual bool   ready();
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

    proc = true;

    base_crit += p -> talents.malice * 0.01;
    base_hit  += p -> talents.precision * 0.01;

    base_multiplier *= 1.0 + ( util_t::talent_rank( p -> talents.find_weakness, 3, 0.03 ) +
                               util_t::talent_rank( p -> talents.vile_poisons,  3, 0.07, 0.14, 0.20 ) );

    base_crit_multiplier *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
  }

  virtual void player_buff();
};

// ==========================================================================
// Static Functions
// ==========================================================================

// enter_stealth ===========================================================

static void enter_stealth( rogue_t* p )
{
  p -> buffs_stealthed = 1;

  if( p -> talents.master_of_subtlety )
  {
    p -> buffs_master_of_subtlety = util_t::talent_rank( p -> talents.master_of_subtlety, 3, 4, 7, 10 );
  }
  if( p -> talents.overkill )
  {
    p -> buffs_overkill = 1;
  }
}

// break_stealth ===========================================================

static void break_stealth( rogue_t* p )
{
  if( p -> buffs_stealthed == 1 )
  {
    p -> buffs_stealthed = -1;

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p ) : event_t( sim, p )
      {
        name = "Recent Stealth Expiration";
        sim -> add_event( this, 6.0 );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        if( p -> buffs_master_of_subtlety )
        {
          p -> aura_loss( "Master of Subtlety" );
          p -> buffs_master_of_subtlety = 0;
        }
        if( p -> buffs_overkill )
        {
          p -> aura_loss( "Overkill" );
          p -> buffs_overkill = 0;
        }
      }
    };

    new ( p -> sim ) expiration_t( p -> sim, p );
  }
}

// clear_combo_points ======================================================

static void clear_combo_points( rogue_t* p )
{
  if( p -> buffs_combo_points <= 0 ) return;

  const char* name[] = { "Combo Points (1)",
                         "Combo Points (2)",
                         "Combo Points (3)",
                         "Combo Points (4)",
                         "Combo Points (5)" };

  p -> aura_loss( name[ p -> buffs_combo_points - 1 ] );

  p -> buffs_combo_points = 0;
}

// add_combo_point =========================================================

static void add_combo_point( rogue_t* p )
{
  if( p -> buffs_combo_points >= 5 ) return;

  const char* name[] = { "Combo Points (1)",
                         "Combo Points (2)",
                         "Combo Points (3)",
                         "Combo Points (4)",
                         "Combo Points (5)" };

  p -> buffs_combo_points++;

  p -> aura_gain( name[ p -> buffs_combo_points - 1 ] );

  p -> procs_combo_points -> occur();
}

// trigger_combat_potency ==================================================

static void trigger_combat_potency( rogue_attack_t* a )
{
  weapon_t* w = a -> weapon;

  if( ! w || w -> slot != SLOT_OFF_HAND )
    return;

  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.combat_potency )
    return;

  if( a -> sim -> roll( 0.20 ) )
  {
    p -> resource_gain( RESOURCE_ENERGY, p -> talents.combat_potency * 3.0, p -> gains_combat_potency );
  }
}

// trigger_cut_to_the_chase ================================================

static void trigger_cut_to_the_chase( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.cut_to_the_chase ) 
    return;

  if( ! p -> buffs_slice_and_dice ) 
    return;

  if( a -> sim -> roll( p -> talents.cut_to_the_chase * 0.20 ) )
  {
    p -> buffs_combo_points = 5;
    if( p -> active_slice_and_dice )
    {
      p -> active_slice_and_dice -> refresh_duration();
    }
    p -> buffs_combo_points = 0;
  }
}

// trigger_dirty_deeds =====================================================

static void trigger_dirty_deeds( rogue_attack_t* a )
{
  rogue_t*  p = a -> player -> cast_rogue();

  if( ! p -> talents.dirty_deeds ) 
    return;

  if( a -> sim -> target -> health_percentage() < 35 )
  {
    a -> player_multiplier *= 1.0 + p -> talents.dirty_deeds * 0.10;
  }
}

// trigger_focused_attacks =================================================

static void trigger_focused_attacks( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.focused_attacks )
    return;

  if( a -> sim -> roll( p -> talents.focused_attacks / 3.0 ) )
  {
    p -> resource_gain( RESOURCE_ENERGY, 2, p -> gains_focused_attacks );
  }
}

// trigger_apply_poisons ===================================================

static void trigger_apply_poisons( rogue_attack_t* a )
{
  weapon_t* w = a -> weapon;

  if( ! w ) return;

  rogue_t* p = a -> player -> cast_rogue();

  if( w -> buff == ANESTHETIC_POISON )
  {
    p -> active_anesthetic_poison -> execute();
  }
  else if( w -> buff == DEADLY_POISON )
  {
    p -> active_deadly_poison -> execute();
  }
  else if( w -> buff == INSTANT_POISON )
  {
    p -> active_instant_poison -> execute();
  }
  else if( w -> buff == WOUND_POISON )
  {
    p -> active_wound_poison -> execute();
  }
}

// trigger_quick_recovery ==================================================

static void trigger_quick_recovery( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.quick_recovery )
    return;

  if( ! a -> requires_combo_points )
    return;

  double recovered_energy = p -> talents.quick_recovery * 0.40 * a -> resource_consumed;

  p -> resource_gain( RESOURCE_ENERGY, recovered_energy, p -> gains_quick_recovery );
}

// trigger_relentless_strikes ===============================================

static void trigger_relentless_strikes( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.relentless_strikes )
    return;

  if( ! a -> requires_combo_points )
    return;

  if( a -> sim -> roll( p -> talents.relentless_strikes * p -> buffs_combo_points * 0.04 ) )
  {
    p -> resource_gain( RESOURCE_ENERGY, 25, p -> gains_relentless_strikes );
  }
}

// trigger_ruthlessness ====================================================

static void trigger_ruthlessness( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.ruthlessness )
    return;

  if( ! a -> requires_combo_points )
    return;

  if( a -> sim -> roll( p -> talents.ruthlessness * 0.20 ) )
  {
    p -> procs_ruthlessness -> occur();
    add_combo_point( p );
  }
}

// trigger_seal_fate =======================================================

static void trigger_seal_fate( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.seal_fate )
    return;

  if( ! a -> adds_combo_points ) 
    return;

  // This is to prevent dual-weapon special attacks from triggering a double-proc of Seal Fate

  if( a -> sim -> current_time <= p -> cooldowns_seal_fate )
    return;

  if( p -> talents.seal_fate )
  {
    if( a -> sim -> roll( p -> talents.seal_fate * 0.20 ) )
    {
      p -> cooldowns_seal_fate = a -> sim -> current_time;
      p -> procs_seal_fate -> occur();
      add_combo_point( p );
    }
  }
}

// trigger_sword_specialization ============================================

static void trigger_sword_specialization( rogue_attack_t* a )
{
  if( a -> proc ) return;

  weapon_t* w = a -> weapon;

  if( ! w ) return;

  if( w -> type != WEAPON_SWORD )
    return;
  
  rogue_t* p = a -> player -> cast_rogue();

  if( ! p -> talents.sword_specialization )
    return;

  if( a -> sim -> roll( p -> talents.sword_specialization * 0.01 ) )
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

  if( p -> buffs_tricks_ready )
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        name = "Tricks of the Trade Expiration";
        p -> aura_gain( "Tricks of the Trade" );
        p -> buffs.tricks_of_the_trade = 1;
        sim -> add_event( this, 6.0 );
      }
      virtual void execute()
      {
        player_t* p = player;
        p -> aura_loss( "Tricks of the Trade" );
        p -> buffs.tricks_of_the_trade = 0;
        p -> expirations.tricks_of_the_trade = 0;
      }
    };

    player_t* t = p -> buffs_tricks_target;
    event_t*& e = t -> expirations.tricks_of_the_trade;

    if( e )
    {
      e -> reschedule( 6.0 );
    }
    else
    {
      e = new ( a -> sim ) expiration_t( a -> sim, t );
    }
    
    p -> buffs_tricks_ready = 0;
  }
}

// trigger_deadly_poison ====================================================

static void trigger_deadly_poison( rogue_t* p )
{
  p -> procs_deadly_poison -> occur();

  if( p -> buffs_poison_doses == 0 )
  {
    // Target about to become poisoned.

    target_t* t = p -> sim -> target;

    if( p -> talents.master_poisoner ) t -> debuffs.master_poisoner++;
    if( p -> talents.savage_combat   ) t -> debuffs.savage_combat++;

    t -> debuffs.poisoned++;
  }
}

// consume_deadly_poison ====================================================

static void consume_deadly_poison( rogue_t* p )
{
  if( p -> buffs_poison_doses == 0 )
  {
    // Target no longer poisoned.

    target_t* t = p -> sim -> target;

    if( p -> talents.master_poisoner ) t -> debuffs.master_poisoner--;
    if( p -> talents.savage_combat   ) t -> debuffs.savage_combat--;

    t -> debuffs.poisoned--;
  }
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
    { NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// rogue_attack_t::cost ====================================================

double rogue_attack_t::cost()
{
  rogue_t* p = player -> cast_rogue();
  double c = attack_t::cost();
  if( c == 0 ) return 0;
  if( p -> buffs_overkill ) c -= 10;
  if( c < 0 ) c = 0;
  // FIXME! In what order do Overkill, Slaughter From The Shadows, and Tier7-4pc get combined?
  if( adds_combo_points && p -> gear.tier7_4pc ) c *= 0.95;  
  return c;
}

// rogue_attack_t::execute =================================================

void rogue_attack_t::execute()
{
  rogue_t* p = player -> cast_rogue();

  attack_t::execute();
  
  if( result_is_hit() )
  {
    trigger_relentless_strikes( this );

    if( requires_combo_points ) clear_combo_points( p );
    if(     adds_combo_points )   add_combo_point ( p );

    trigger_apply_poisons( this );
    trigger_ruthlessness( this );
    trigger_sword_specialization( this );

    if( result == RESULT_CRIT )
    {
      trigger_focused_attacks( this );
      trigger_seal_fate( this );
    }
  }
  else
  {
    trigger_quick_recovery( this );
  }

  trigger_tricks_of_the_trade( this );

  break_stealth( p );

  p -> buffs_cold_blood = 0;
  p -> buffs_shadowstep = 0;
}

// rogue_attack_t::player_buff ============================================

void rogue_attack_t::player_buff()
{
  rogue_t* p = player -> cast_rogue();

  attack_t::player_buff();

  if( weapon )
  {
    if( p -> talents.mace_specialization )
    {
      if( weapon -> type == WEAPON_MACE )
      {
        player_penetration += p -> talents.mace_specialization * 0.03;
      }
    }
    if( p -> talents.close_quarters_combat )
    {
      if( weapon -> type <= WEAPON_SMALL )
      {
        player_crit += p -> talents.close_quarters_combat * 0.01;
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
  if( p -> buffs_cold_blood )
  {
    player_crit += 1.0;
  }
  if( p -> buffs_hunger_for_blood )
  {
    player_multiplier *= 1.0 + p -> buffs_hunger_for_blood * 0.05;
  }
  if( p -> buffs_master_of_subtlety )
  {
    player_multiplier *= 1.0 + p -> buffs_master_of_subtlety * 0.01;
  }
  if( p -> talents.murder )
  {
    int target_race = sim -> target -> race;

    if( target_race == RACE_BEAST     || 
        target_race == RACE_DRAGONKIN ||
        target_race == RACE_GIANT     ||
        target_race == RACE_HUMANOID  )
    {
      player_multiplier      *= 1.0 + p -> talents.murder * 0.02;
      player_crit_multiplier *= 1.0 + p -> talents.murder * 0.02;
    }
  }
  if( p -> buffs_shadowstep )
  {
    player_multiplier *= 1.2;
  }

  if( p -> talents.hunger_for_blood )
  {
    p -> uptimes_hunger_for_blood -> update( p -> buffs_hunger_for_blood == 3 );
  }
  p -> uptimes_tricks_of_the_trade -> update( p -> buffs.tricks_of_the_trade != 0 );
}


// rogue_attack_t::armor() ================================================

double rogue_attack_t::armor()
{
  rogue_t* p = player -> cast_rogue();

  double adjusted_armor = attack_t::armor();

  if( p -> talents.serrated_blades )
  {
    adjusted_armor -= p -> level * 8 * p -> talents.serrated_blades / 3.0;
  }

  return adjusted_armor;
}

// rogue_attack_t::ready() ================================================

bool rogue_attack_t::ready()
{
  if( ! attack_t::ready() )
    return false;

  rogue_t* p = player -> cast_rogue();

  if( requires_weapon != WEAPON_NONE )
    if( ! weapon || weapon -> type != requires_weapon ) 
      return false;

  if( requires_position != POSITION_NONE )
    if( p -> position != requires_position )
      return false;

  if( requires_stealth )
    if( p -> buffs_stealthed <= 0 )
      return false;

  if( requires_combo_points && ( p -> buffs_combo_points == 0 ) )
    return false;

  if( min_combo_points > 0 )
    if( p -> buffs_combo_points < min_combo_points )
      return false;

  if( max_combo_points > 0 )
    if( p -> buffs_combo_points > max_combo_points )
      return false;

  if( min_energy > 0 )
    if( p -> resource_current[ RESOURCE_ENERGY ] < min_energy )
      return false;

  if( max_energy > 0 )
    if( p -> resource_current[ RESOURCE_ENERGY ] > max_energy )
      return false;

  double ct = sim -> current_time;

  if( min_hfb_expire > 0 )
    if( ! p -> expirations_hunger_for_blood || ( ( p -> expirations_hunger_for_blood -> occurs() - ct ) < min_hfb_expire ) )
      return false;

  if( max_hfb_expire > 0 )
    if( p -> expirations_hunger_for_blood && ( ( p -> expirations_hunger_for_blood -> occurs() - ct ) > max_hfb_expire ) )
      return false;

  if( min_snd_expire > 0 )
    if( ! p -> expirations_slice_and_dice || ( ( p -> expirations_slice_and_dice -> occurs() - ct ) < min_snd_expire ) )
      return false;

  if( max_snd_expire > 0 )
    if( p -> expirations_slice_and_dice && ( ( p -> expirations_slice_and_dice -> occurs() - ct ) > max_snd_expire ) )
      return false;

  if( min_env_expire > 0 )
    if( ! p -> expirations_envenom || ( ( p -> expirations_envenom -> occurs() - ct ) < min_env_expire ) )
      return false;

  if( max_env_expire > 0 )
    if( p -> expirations_envenom && ( ( p -> expirations_envenom -> occurs() - ct ) > max_env_expire ) )
      return false;

  if( min_rup_expire > 0 )
    if( ! p -> active_rupture || ( ( p -> active_rupture -> duration_ready - ct ) < min_rup_expire ) )
      return false;

  if( max_rup_expire > 0 )
    if( p -> active_rupture && ( ( p -> active_rupture -> duration_ready - ct ) > max_rup_expire ) )
      return false;

  return true;
}

// Melee Attack ============================================================

struct melee_t : public rogue_attack_t
{
  melee_t( const char* name, player_t* player ) : 
    rogue_attack_t( name, player )
  {
    rogue_t* p = player -> cast_rogue();

    base_direct_dmg = 1;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;

    if( p -> dual_wield() ) base_hit -= 0.18;
  }

  virtual double execute_time()
  {
    rogue_t* p = player -> cast_rogue();

    double t = rogue_attack_t::execute_time();

    if( p -> buffs_blade_flurry   ) t *= 1.0 / 1.20;
    if( p -> buffs_slice_and_dice ) t *= 1.0 / ( 1.40 + ( p -> gear.tier6_2pc ? 0.05 : 0.00 ) );

    p -> uptimes_blade_flurry   -> update( p -> buffs_blade_flurry   != 0 );
    p -> uptimes_slice_and_dice -> update( p -> buffs_slice_and_dice != 0 );

    return t;
  }

  virtual void execute()
  {
    rogue_attack_t::execute();
    if( result_is_hit() )
    {
      trigger_combat_potency( this );
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public rogue_attack_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "auto_attack", player )
  {
    rogue_t* p = player -> cast_rogue();

    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( "melee_main_hand", player );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", player );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    p -> main_hand_attack -> schedule_execute();
    if( p -> off_hand_attack ) p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
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
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 10, 908, 908, 0, 60 },
      { 75,  9, 770, 770, 0, 60 },
      { 70,  8, 509, 509, 0, 60 },
      { 66,  7, 369, 369, 0, 60 },
      { 58,  6, 319, 319, 0, 60 },
      { 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    requires_weapon        = WEAPON_DAGGER;
    requires_position      = POSITION_BACK;
    requires_stealth       = true;
    adds_combo_points      = true;
    weapon_multiplier     *= 2.75;
    base_cost             -= p -> talents.slaughter_from_the_shadows * 3;
    base_multiplier       *= 1.0 + ( p -> talents.find_weakness * 0.02 +
                                     p -> talents.opportunity   * 0.10 );
    base_crit                  += p -> talents.improved_ambush * 0.25;
    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    rogue_attack_t::execute();

    if( result_is_hit() )
    {
      if( sim -> roll( p -> talents.initiative / 3.0 ) )
      {
        add_combo_point( p );
      }
    }
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if( p -> buffs_shadow_dance )
    {
      requires_stealth = false;
      bool success = rogue_attack_t::ready();
      requires_stealth = true;
      return success;
    }

    return rogue_attack_t::ready();
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
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 12, 465, 465, 0, 60 },
      { 74, 11, 383, 383, 0, 60 },
      { 68, 10, 255, 255, 0, 60 },
      { 60,  9, 225, 225, 0, 60 },
      { 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    requires_weapon        = WEAPON_DAGGER;
    requires_position      = POSITION_BACK;
    adds_combo_points      = true;
    base_cost             -= p -> talents.slaughter_from_the_shadows * 3;
    weapon_multiplier     *= 1.50 + p -> talents.sinister_calling * 0.01;

    base_multiplier *= 1.0 + ( p -> talents.aggression       * 0.03 +
			       p -> talents.blade_twisting   * 0.05 +
			       p -> talents.find_weakness    * 0.02 +
			       p -> talents.opportunity      * 0.10 +
			       p -> talents.surprise_attacks * 0.10 +
			       p -> gear.tier6_4pc           * 0.06 );

    base_crit += ( p -> talents.puncturing_wounds * 0.10 +
		   p -> talents.turn_the_tables   * 0.02 );

    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }
};

// Blade Flurry ============================================================

struct blade_flurry_t : public rogue_attack_t
{
  blade_flurry_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "blade_flurry", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.blade_flurry );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    cooldown  = 120;
    base_cost = p -> glyphs.blade_flurry ? 0 : 25;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p ) : event_t( sim, p )
      {
        name = "Blade Flurry Expiration";
        p -> aura_gain( "Blade Flurry" );
        p -> buffs_blade_flurry = 1;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> aura_loss( "Blade Flurry" );
        p -> buffs_blade_flurry = 0;
      }
    };

    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    new ( sim ) expiration_t( sim, p );
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  int min_doses;
  int no_buff;
  double* dose_dmg;

  envenom_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "envenom", player, SCHOOL_NATURE, TREE_ASSASSINATION ), min_doses(1), no_buff(0)
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> level >= 62 );

    option_t options[] =
    {
      { "min_doses", OPT_INT, &min_doses },
      { "no_buff",   OPT_INT, &no_buff   },
      { NULL }
    };
    parse_options( options, options_str );
      
    requires_combo_points = true;
    base_cost = 35;

    base_multiplier *= 1.0 + ( p -> talents.find_weakness * 0.02 + 
                               util_t::talent_rank( p -> talents.vile_poisons, 3, 0.07, 0.14, 0.20 ) );

    base_crit_multiplier *= 1.0 + p -> talents.prey_on_the_weak * 0.04;

    if( p -> talents.surprise_attacks ) may_dodge = false;

    static double dmg_80[] = { 216, 432, 648, 864, 1080 };
    static double dmg_74[] = { 176, 352, 528, 704,  880 };
    static double dmg_69[] = { 148, 296, 444, 492,  740 };
    static double dmg_62[] = { 118, 236, 354, 472,  590 };

    dose_dmg = ( p -> level >= 80 ? dmg_80 :
		 p -> level >= 74 ? dmg_74 :
		 p -> level >= 69 ? dmg_69 : 
		                    dmg_62 );
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p, double duration ) : event_t( sim, p )
      {
        name = "Envenom Expiration";
        p -> aura_gain( "Envenom" );
        p -> buffs_envenom = 1;
        sim -> add_event( this, duration );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> aura_loss( "Envenom" );
        p ->       buffs_envenom = 0;
        p -> expirations_envenom = 0;
      }
    };

    rogue_t* p = player -> cast_rogue();

    int doses_consumed = std::min( p -> buffs_poison_doses, p -> buffs_combo_points );

    double envenom_duration = 1.0 + p -> buffs_combo_points;

    event_t*& e = p -> expirations_envenom;

    if( e )
    {
      e -> reschedule( envenom_duration );
    }
    else
    {
      e = new ( sim ) expiration_t( sim, p, envenom_duration );
    }

    rogue_attack_t::execute();

    if( result_is_hit() )
    {
      if( doses_consumed > 0 )
      {
	p -> buffs_poison_doses -= doses_consumed;

	if( p -> buffs_poison_doses == 0 )
        {
	  p -> active_deadly_poison -> cancel();
	}
	consume_deadly_poison( p );
      }
      trigger_cut_to_the_chase( this );
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();

    int doses_consumed = std::min( p -> buffs_poison_doses, p -> buffs_combo_points );

    if( doses_consumed == 0 )
    {
      rogue_attack_t::player_buff();
      player_multiplier = 0;
    }
    else
    {
      direct_power_mod = p -> buffs_combo_points * 0.07;
      base_direct_dmg = dose_dmg[ doses_consumed-1 ];
      rogue_attack_t::player_buff();
    }

    trigger_dirty_deeds( this );
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if( min_doses > 0 )
      if( min_doses > p -> buffs_poison_doses )
        return false;

    if( no_buff && p -> buffs_envenom )
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
      { NULL }
    };
    parse_options( options, options_str );
      
    requires_combo_points = true;
    base_cost = 35;

    base_multiplier *= 1.0 + ( p -> talents.aggression    * 0.03 +
                               p -> talents.find_weakness * 0.02 +
                               util_t::talent_rank( p -> talents.improved_eviscerate, 3, 0.07, 0.14, 0.20 ) );

    base_crit_multiplier *= 1.0 + p -> talents.prey_on_the_weak * 0.04;

    if( p -> glyphs.eviscerate ) base_crit += 0.10;

    if( p -> talents.surprise_attacks ) may_dodge = false;

    static double_pair dmg_79[] = { { 497, 751 }, { 867, 1121 }, { 1237, 1491 }, { 1607, 1861 }, { 1977, 2231 } };
    static double_pair dmg_73[] = { { 405, 613 }, { 706,  914 }, { 1007, 1215 }, { 1308, 1516 }, { 1609, 1817 } };
    static double_pair dmg_64[] = { { 245, 365 }, { 430,  550 }, {  615,  735 }, {   800, 920 }, {  985, 1105 } };
    static double_pair dmg_60[] = { { 224, 332 }, { 394,  502 }, {  564,  762 }, {   734, 842 }, {  904, 1012 } };

    combo_point_dmg = ( p -> level >= 79 ? dmg_79 :
                        p -> level >= 73 ? dmg_73 :
                        p -> level >= 64 ? dmg_64 : 
                                           dmg_60 );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    base_dd_min = combo_point_dmg[ p -> buffs_combo_points - 1 ].min;
    base_dd_max = combo_point_dmg[ p -> buffs_combo_points - 1 ].max;

    direct_power_mod = 0.05 * p -> buffs_combo_points;

    if( sim -> average_dmg ) 
    {
      base_direct_dmg = ( base_dd_min + base_dd_max ) / 2;
    }
    else
    {
      double range = 0.02 * p -> buffs_combo_points;

      direct_power_mod += range * ( sim -> rng -> real() * 2.0 - 1.0 );
    }

    rogue_attack_t::execute();

    if( result_is_hit() )
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
    rogue_attack_t( "expose_armor", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION ), override_sunder(0)
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { "override_sunder", OPT_INT, &override_sunder },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 77, 7, 0, 0, 450, 25 },
      { 66, 6, 0, 0, 520, 25 },
      { 56, 5, 0, 0, 785, 25 },
      { 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    requires_combo_points = true;
    base_cost -= p -> talents.improved_expose_armor * 5;

    if( p -> talents.surprise_attacks ) may_dodge = false;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::execute();
    if( result_is_hit() )
    {
      target_t* t = sim -> target;

      double debuff = base_td_init * p -> buffs_combo_points;

      if( debuff >= t -> debuffs.expose_armor )
      {
        struct expiration_t : public event_t
        {
          expiration_t( sim_t* sim, player_t* p, double duration ) : event_t( sim, p )
          {
            name = "Expose Armor Expiration";
            sim -> add_event( this, duration );
          }
          virtual void execute()
          {
            sim -> target -> debuffs.expose_armor = 0;
            sim -> target -> expirations.expose_armor = 0;
          }
        };

        double duration = p -> glyphs.expose_armor ? 40 : 30;

        t -> debuffs.expose_armor = debuff;

        event_t*& e = t -> expirations.expose_armor;

        if( e )
        {
          e -> reschedule( duration );
        }
        else
        {
          e = new ( sim ) expiration_t( sim, p, duration );
        }
      }
    }
  };

  virtual bool ready()
  {
    target_t* t = sim -> target;

    if( override_sunder )
    {
      return ( t -> debuffs.expose_armor == 0 );
    }
    else
    {
      return ( t -> debuffs.expose_armor == 0 &&
               t -> debuffs.sunder_armor == 0 );
    }

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
      { NULL }
    };
    parse_options( options, options_str );
      
    base_cost = p -> glyphs.feint ? 10 : 20;
    cooldown = 10;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    // model threat eventually......
  }
};

// Ghostly Strike ==========================================================

struct ghostly_strike_t : public rogue_attack_t
{
  ghostly_strike_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "ghostly_strike", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.ghostly_strike );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed      = true;
    adds_combo_points           = true;
    cooldown                    = p -> glyphs.ghostly_strike ? 30 : 20;
    base_cost                   = 40;
    weapon_multiplier          *= 1.25 + ( p -> glyphs.ghostly_strike ? 0.4 : 0.0 );
    base_multiplier            *= 1.0 + p -> talents.find_weakness * 0.02;
    base_crit                  += p -> talents.turn_the_tables * 0.02;
    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
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
    rogue_attack_t( "hemorrhage", player, SCHOOL_PHYSICAL, TREE_SUBTLETY ), damage_adder(0)
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.hemorrhage );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    base_direct_dmg = 1;
    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed      = true;
    adds_combo_points           = true;
    base_cost                   = 35 - p -> talents.slaughter_from_the_shadows;
    weapon_multiplier          *= 1.10 + p -> talents.sinister_calling * 0.01;
    base_multiplier            *= 1.0 + ( p -> talents.find_weakness    * 0.02 +
                                          p -> talents.surprise_attacks * 0.10 +
                                          p -> gear.tier6_4pc           * 0.06 );
    base_crit                  += p -> talents.turn_the_tables * 0.02;
    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;

    damage_adder = util_t::ability_rank( p -> level,  75.0,80,  42.0,70,  29.0,0 );

    if( p -> glyphs.hemorrhage ) damage_adder *= 1.40;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::execute();
    if( result_is_hit() )
    {
      target_t* t = sim -> target;

      if( damage_adder >= t -> debuffs.hemorrhage )
      {
        struct expiration_t : public event_t
        {
          expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
          {
            name = "Hemorrhage Expiration";
            sim -> add_event( this, 15.0 );
          }
          virtual void execute()
          {
            sim -> target -> debuffs.hemorrhage = 0;
            sim -> target -> debuffs.hemorrhage_charges = 0;
            sim -> target -> expirations.hemorrhage = 0;
          }
        };

        t -> debuffs.hemorrhage = damage_adder;
        t -> debuffs.hemorrhage_charges = 10;

        event_t*& e = t -> expirations.hemorrhage;

        if( e )
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
    refresh_at(5)
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.hunger_for_blood );

    option_t options[] =
    {
      { "refresh_at", OPT_FLT, &refresh_at },
      { NULL }
    };
    parse_options( options, options_str );

    base_cost = 30;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p ) : event_t( sim, p )
      {
        name = "Hunger For Blood Expiration";
        p -> buffs_hunger_for_blood = 1;
        sim -> add_event( this, 30.0 );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> aura_loss( "Hunger For Blood" );
        p ->       buffs_hunger_for_blood = 0;
        p -> expirations_hunger_for_blood = 0;
      }
    };

    rogue_t* p = player -> cast_rogue();

    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );

    if( p -> buffs_hunger_for_blood < 3 )
    {
      p -> buffs_hunger_for_blood++;
      p -> aura_gain( "Hunger For Blood" );
    }
    
    consume_resource();

    event_t*& e = p -> expirations_hunger_for_blood;

    if( e )
    {
      e -> reschedule( 30.0 );
    }
    else
    {
      e = new ( sim ) expiration_t( sim, p );
    }
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if( ! rogue_attack_t::ready() )
      return false;

    if( p -> buffs_hunger_for_blood == 3 )
      if( ( p -> expirations_hunger_for_blood -> occurs() - sim -> current_time ) > refresh_at )
        return false;

    return true;
  }

  // Rogues can stack the buff prior to entering combat, so if they have yet to use an
  // offensive ability, then this action will not trigger the GCD nor will it cost any energy.

  virtual double gcd()  { return player -> in_combat ? rogue_attack_t::gcd()  : 0; }
  virtual double cost() { return player -> in_combat ? rogue_attack_t::cost() : 0; }
};

// Killing Spree ============================================================

struct killing_spree_t : public rogue_attack_t
{
  killing_spree_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "killing_spree", player, SCHOOL_PHYSICAL, TREE_COMBAT )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.killing_spree );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    channeled       = true;
    cooldown        = 120;
    num_ticks       = 5;
    base_direct_dmg = 1;
    base_tick_time  = 0.5;
    base_cost       = 0;
    base_multiplier *= 1.0 + p -> talents.find_weakness * 0.02;
    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    schedule_tick();
    update_ready();
    // For cleaner statistics gathering we register the damage as a DoT
    direct_dmg = 0;
    attack_t::update_stats( DMG_DIRECT );
    p -> action_finish( this );
  }

  virtual double tick_time() 
  {
    // Killing Spree not modified by haste effects
    return base_tick_time;
  }

  virtual void tick()
  {
    rogue_t* p = player -> cast_rogue();

    double mh_dd=0, oh_dd=0;
    int mh_result=RESULT_NONE, oh_result=RESULT_NONE;

    weapon = &( p -> main_hand_weapon );
    rogue_attack_t::execute();
    mh_dd = direct_dmg;
    mh_result = result;

    if( result_is_hit() )
    {
      if( p -> off_hand_weapon.type != WEAPON_NONE )
      {
        weapon = &( p -> off_hand_weapon );
        rogue_attack_t::execute();
        oh_dd = direct_dmg;
        oh_result = result;
      }
    }

    tick_dmg = mh_dd + oh_dd;
    result   = mh_result;
    attack_t::update_stats( DMG_OVER_TIME );
  }

  virtual void player_buff()
  {
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }

  virtual void update_stats( int type ) { }
};

// Mutilate =================================================================

struct mutilate_t : public rogue_attack_t
{
  mutilate_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "mutilate", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.mutilate );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 6, 181, 181, 0, 60 },
      { 75, 5, 153, 153, 0, 60 },
      { 70, 4, 101, 101, 0, 60 },
      { 60, 3,  88,  88, 0, 60 },
      { 0, 0 }
    };
    init_rank( ranks );

    adds_combo_points      = true;
    normalize_weapon_speed = true;

    base_multiplier  *= 1.0 + ( p -> talents.find_weakness * 0.02 +
                                p -> talents.opportunity   * 0.10 +
                                p -> gear.tier6_4pc        * 0.06 );

    base_crit += ( p -> talents.puncturing_wounds * 0.05 +
		   p -> talents.turn_the_tables   * 0.02 );

    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
  }

  virtual void execute()
  {
    attack_t::consume_resource();

    weapon = &( player -> main_hand_weapon );
    rogue_attack_t::execute();

    if( result_is_hit() )
    {
      if( player -> off_hand_weapon.type != WEAPON_NONE )
      {
        weapon = &( player -> off_hand_weapon );
        rogue_attack_t::execute();
      }
    }

    player -> action_finish( this );
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::player_buff();
    if( sim -> target -> debuffs.poisoned ) player_multiplier *= 1.20;
    p -> uptimes_poisoned -> update( sim -> target -> debuffs.poisoned > 0 );
    trigger_dirty_deeds( this );
  }

  virtual void consume_resource() { }
};

// Premeditation =============================================================

struct premeditation_t : public rogue_attack_t
{
  premeditation_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "premeditation", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    requires_stealth = true;
    cooldown = 20;
    base_cost = 0;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    add_combo_point( p );
    add_combo_point( p );
    update_ready();
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if( p -> buffs_shadow_dance )
    {
      requires_stealth = false;
      bool success = rogue_attack_t::ready();
      requires_stealth = true;
      return success;
    }

    return rogue_attack_t::ready();
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
      { NULL }
    };
    parse_options( options, options_str );
      
    requires_combo_points = true;
    base_cost             = 25;
    base_tick_time        = 2.0; 
    base_multiplier      *= 1.0 + ( p -> talents.blood_spatter   * 0.15 +
                                    p -> talents.find_weakness   * 0.03 +
                                    p -> talents.serrated_blades * 0.10 +
                                    p -> gear.tier7_2pc          * 0.10 );

    base_crit_multiplier *= 1.0 + p -> talents.prey_on_the_weak * 0.04;

    if( p -> talents.surprise_attacks ) may_dodge = false;

    static double dmg_79[] = { 145, 163, 181, 199, 217 };
    static double dmg_74[] = { 122, 137, 152, 167, 182 };
    static double dmg_68[] = {  81,  92, 103, 114, 125 };
    static double dmg_60[] = {  68,  76,  84,  92, 100 };

    combo_point_dmg = ( p -> level >= 79 ? dmg_79 :
                        p -> level >= 74 ? dmg_74 :
                        p -> level >= 68 ? dmg_68 : 
                                           dmg_60 );

    observer = &( p -> active_rupture );    
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    num_ticks = 3 + p -> buffs_combo_points;
    if( p -> glyphs.rupture ) num_ticks += 2;
    rogue_attack_t::execute();
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    tick_power_mod = p -> combo_point_rank( 0.015, 0.024, 0.030, 0.03428571, 0.0375 );
    base_td_init   = p -> combo_point_rank( combo_point_dmg );
    rogue_attack_t::player_buff();
    trigger_dirty_deeds( this );
  }
};

// Shadowstep ===============================================================

struct shadowstep_t : public rogue_attack_t
{
  shadowstep_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "shadowstep", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown = 30;
    base_cost = 10;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_shadowstep = 1;
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
      { NULL }
    };
    parse_options( options, options_str );
      
    weapon = &( p -> off_hand_weapon );
    adds_combo_points = true;
    base_direct_dmg   = 1;
    base_multiplier  *= 1.0 + ( p -> talents.find_weakness    * 0.02 +
                                p -> talents.surprise_attacks * 0.10 );
    base_crit                  += p -> talents.turn_the_tables * 0.02;
    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    p -> buffs_shiv = 1;
    rogue_attack_t::execute();
    p -> buffs_shiv = 0;
  }

  virtual double cost()
  {
    base_cost = 20 + weapon -> swing_time * 10.0;
    return rogue_attack_t::cost();
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
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 12, 180, 180, 0, 45 },
      { 76, 11, 150, 150, 0, 45 },
      { 70, 10,  98,  98, 0, 45 },
      { 62,  9,  80,  80, 0, 45 },
      { 54,  8,  68,  68, 0, 45 },
      { 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    adds_combo_points      = true;

    base_cost -= util_t::talent_rank( p -> talents.improved_sinister_strike, 2, 3.0, 5.0 );

    base_multiplier *= 1.0 + ( p -> talents.aggression       * 0.03 +
                               p -> talents.blade_twisting   * 0.05 +
                               p -> talents.find_weakness    * 0.02 +
                               p -> talents.surprise_attacks * 0.10 +
                               p -> gear.tier6_4pc           * 0.06 );

    base_crit += p -> talents.turn_the_tables * 0.02;

    base_crit_multiplier       *= 1.0 + p -> talents.prey_on_the_weak * 0.04;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.lethality * 0.06;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::execute();
    if( result_is_hit() )
    {
      if( p -> glyphs.sinister_strike && sim -> roll( 0.50 ) )
      {
        add_combo_point( p );
      }
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::player_buff();
    if( weapon -> type <= WEAPON_SMALL ) player_crit += p -> talents.close_quarters_combat * 0.01;
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
      { NULL }
    };
    parse_options( options, options_str );

    requires_combo_points = true;
    base_cost = 25;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    refresh_duration();
    consume_resource();
    trigger_relentless_strikes( this );
    clear_combo_points( p );
    p -> active_slice_and_dice = this;
  }

  virtual void refresh_duration()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p, double duration ) : event_t( sim, p )
      {
        name = "Slice and Dice Expiration";
        p -> aura_gain( "Slice and Dice" );
        p -> buffs_slice_and_dice = 1;
        sim -> add_event( this, duration );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> aura_loss( "Slice and Dice" );
        p ->       buffs_slice_and_dice = 0;
        p -> expirations_slice_and_dice = 0;
	p ->      active_slice_and_dice = 0;
      }
    };

    rogue_t* p = player -> cast_rogue();

    double duration = 6.0 + 3.0 * p -> buffs_combo_points;

    if( p -> glyphs.slice_and_dice ) duration += 3.0;

    duration *= 1.0 + p -> talents.improved_slice_and_dice * 0.25;

    event_t*& e = p -> expirations_slice_and_dice;

    if( e )
    {
      e -> reschedule( duration );
    }
    else
    {
      e = new ( sim ) expiration_t( sim, p, duration );
    }
  }
};

// Pool Energy ==============================================================

struct pool_energy_t : public rogue_attack_t
{
  double wait;
  int for_next;

  pool_energy_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "pool_energy", player ), wait(0.5), for_next(0)
  {
    option_t options[] =
    {
      { "wait",     OPT_FLT, &wait     },
      { "for_next", OPT_INT, &for_next },
      { NULL }
    };
    parse_options( options, options_str );
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
  }

  virtual double gcd()
  {
    return wait;
  }

  virtual bool ready()
  {
    if( for_next )
    {
      if( next -> ready() )
	return false;

      // If the next action in the list would be "ready" if it was not constrained by energy,
      // then this command will pool energy until we have enough.

      player -> resource_current[ RESOURCE_ENERGY ] += 100;

      bool energy_limited = next -> ready();
      
      player -> resource_current[ RESOURCE_ENERGY ] -= 100;

      if( ! energy_limited ) return false;
    }

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

  player_power     = p -> composite_attack_power();
  power_multiplier = p -> composite_attack_power_multiplier();

  if( p -> buffs_cold_blood )
  {
    player_crit += 1.0;
  }
  if( p -> buffs_hunger_for_blood )
  {
    player_multiplier *= 1.0 + p -> buffs_hunger_for_blood * 0.05;
  }
  if( p -> buffs_master_of_subtlety )
  {
    player_multiplier *= 1.0 + p -> buffs_master_of_subtlety * 0.01;
  }
  if( p -> talents.murder )
  {
    int target_race = sim -> target -> race;

    if( target_race == RACE_BEAST     || 
        target_race == RACE_DRAGONKIN ||
        target_race == RACE_GIANT     ||
        target_race == RACE_HUMANOID  )
    {
      player_multiplier      *= 1.0 + p -> talents.murder * 0.02;
      player_crit_multiplier *= 1.0 + p -> talents.murder * 0.02;
    }
  }
  if( p -> buffs_shadowstep )
  {
    player_multiplier *= 1.2;
  }

  if( p -> talents.hunger_for_blood )
  {
    p -> uptimes_hunger_for_blood -> update( p -> buffs_hunger_for_blood == 3 );
  }
  p -> uptimes_tricks_of_the_trade -> update( p -> buffs.tricks_of_the_trade != 0 );
}

// Anesthetic Poison ========================================================

struct anesthetic_poison_t : public rogue_poison_t
{
  anesthetic_poison_t( player_t* player ) : 
    rogue_poison_t( "anesthetic_poison", player )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd      = 0;
    background       = true;
    proc             = true;
    may_crit         = true;
    direct_power_mod = 0;
    base_direct_dmg  = util_t::ability_rank( p -> level,  249,77,  153,68,  0,0 );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if( p -> buffs_shiv || sim -> roll( 0.50 ) )
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
    proc           = true;
    num_ticks      = 4;
    base_tick_time = 3.0;
    tick_power_mod = 0.08 / num_ticks;
    base_td_init   = util_t::ability_rank( p -> level,  296,80,  244,76,  204,70,  160,62,  96,0  ) / num_ticks;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int success = p -> buffs_shiv;

    if( ! success )
    {
      double chance = 0.30;
      chance += p -> talents.improved_poisons * 0.02;
      if( p -> buffs_envenom ) chance += 0.15;
      p -> uptimes_envenom -> update( p -> buffs_envenom != 0 );
      success = sim -> roll( chance );
    }

    if( success )
    {
      trigger_deadly_poison( p );

      if( p -> buffs_poison_doses < 5 ) p -> buffs_poison_doses++;

      if( sim -> log ) report_t::log( sim, "%s performs %s (%d)", player -> name(), name(), p -> buffs_poison_doses );

      if( ticking )
      {
        refresh_duration();
      }
      else
      {
        player_buff();
        schedule_tick();
      }
    }
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_poison_t::player_buff();
    player_multiplier *= p -> buffs_poison_doses;
  }

  virtual void last_tick()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_poison_t::last_tick();
    p -> buffs_poison_doses = 0;
    consume_deadly_poison( p );
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
    proc             = true;
    may_crit         = true;
    direct_power_mod = 0.10;
    base_direct_dmg  = util_t::ability_rank( p -> level,  350,79,  286,73,  188,68,  88,0 );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int success = p -> buffs_shiv;

    if( ! success )
    {
      double chance = 0.20;
      chance += p -> talents.improved_poisons * 0.02;
      if( p -> buffs_envenom ) chance += 0.15;
      p -> uptimes_envenom -> update( p -> buffs_envenom != 0 );
      success = sim -> roll( chance );
    }

    if( success )
    {
      rogue_poison_t::execute();
    }
  }
};

// Wound Poison ============================================================

struct wound_poison_t : public rogue_poison_t
{
  wound_poison_t( player_t* player ) : 
    rogue_poison_t( "wound_poison", player )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd      = 0;
    background       = true;
    proc             = true;
    may_crit         = true;
    direct_power_mod = 0.04;
    base_direct_dmg  = util_t::ability_rank( p -> level,  231,78,  188,72,  112,64,  53,0 );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if( p -> buffs_shiv || sim -> roll( 0.50 ) )
    {
      rogue_poison_t::execute();
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
    main_hand_poison( WEAPON_BUFF_NONE ), 
     off_hand_poison( WEAPON_BUFF_NONE )
  {
    rogue_t* p = player -> cast_rogue();
    
    std::string main_hand_str;
    std::string  off_hand_str;

    option_t options[] =
    {
      { "main_hand", OPT_STRING, &main_hand_str },
      {  "off_hand", OPT_STRING,  &off_hand_str },
      { NULL }
    };
    parse_options( options, options_str );

    if( main_hand_str.empty() &&
         off_hand_str.empty() ) 
    {
      printf( "simcraft: At least one of 'main_hand/off_hand' must be specified in 'apply_poison'.  Accepted values are 'anesthetic/deadly/instant/wound'.\n" );
      exit( 0 );
    }

    trigger_gcd = 0;

    if( main_hand_str == "anesthetic" ) main_hand_poison = ANESTHETIC_POISON;
    if( main_hand_str == "deadly"     ) main_hand_poison =     DEADLY_POISON;
    if( main_hand_str == "instant"    ) main_hand_poison =    INSTANT_POISON;
    if( main_hand_str == "wound"      ) main_hand_poison =      WOUND_POISON;

    if( off_hand_str == "anesthetic" ) off_hand_poison = ANESTHETIC_POISON;
    if( off_hand_str == "deadly"     ) off_hand_poison =     DEADLY_POISON;
    if( off_hand_str == "instant"    ) off_hand_poison =    INSTANT_POISON;
    if( off_hand_str == "wound"      ) off_hand_poison =      WOUND_POISON;

    if( main_hand_poison != WEAPON_BUFF_NONE ) assert( p -> main_hand_weapon.type != WEAPON_NONE );
    if(  off_hand_poison != WEAPON_BUFF_NONE ) assert( p ->  off_hand_weapon.type != WEAPON_NONE );

    if( ! p -> active_anesthetic_poison ) p -> active_anesthetic_poison = new anesthetic_poison_t( p );
    if( ! p -> active_deadly_poison     ) p -> active_deadly_poison     = new     deadly_poison_t( p );
    if( ! p -> active_instant_poison    ) p -> active_instant_poison    = new    instant_poison_t( p );
    if( ! p -> active_wound_poison      ) p -> active_wound_poison      = new      wound_poison_t( p );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    if( main_hand_poison != WEAPON_BUFF_NONE ) p -> main_hand_weapon.buff = main_hand_poison;
    if(  off_hand_poison != WEAPON_BUFF_NONE ) p ->  off_hand_weapon.buff =  off_hand_poison;
  };

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if( main_hand_poison != WEAPON_BUFF_NONE ) 
      return( main_hand_poison != p -> main_hand_weapon.buff );

    if( off_hand_poison != WEAPON_BUFF_NONE ) 
      return( off_hand_poison != p -> off_hand_weapon.buff );

    return false;
  }
};

// =========================================================================
// Rogue Spells
// =========================================================================

// Adrenaline Rush ==========================================================

struct adrenaline_rush_t : public spell_t
{
  adrenaline_rush_t( player_t* player, const std::string& options_str ) : 
    spell_t( "adrenaline_rush", player )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.adrenaline_rush );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown = 300;
    if( p -> glyphs.adrenaline_rush ) cooldown -= 60;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p ) : event_t( sim, p )
      {
        name = "Adrenaline Rush Expiration";
        p -> aura_gain( "Adrenaline Rush" );
        p -> buffs_adrenaline_rush = 1;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> aura_loss( "Adrenaline Rush" );
        p -> buffs_adrenaline_rush = 0;
      }
    };

    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    new ( sim ) expiration_t( sim, p );
  }
};

// Cold Blood ==============================================================

struct cold_blood_t : public spell_t
{
  cold_blood_t( player_t* player, const std::string& options_str ) : 
    spell_t( "cold_blood", player )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.cold_blood );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    trigger_gcd = 0;
    cooldown = 180;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_cold_blood = 1;
    update_ready();
  }
};

// Preparation ==============================================================

struct preparation_t : public spell_t
{
  preparation_t( player_t* player, const std::string& options_str ) : 
    spell_t( "preparation", player )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.preparation );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    trigger_gcd = 0;

    cooldown = 600;
    cooldown -= p -> talents.filthy_tricks * 150;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    for( action_t* a = player -> action_list; a; a = a -> next )
    {
      if( a -> name_str == "vanish"     ||
          a -> name_str == "cold_blood" ||
          ( a -> name_str == "blade_flurry" && p -> glyphs.preparation ) )
      {
        a -> cooldown_ready = 0;
      }
    }
    update_ready();
  }
};

// Shadow Dance =============================================================

struct shadow_dance_t : public spell_t
{
  shadow_dance_t( player_t* player, const std::string& options_str ) : 
    spell_t( "shadow_dance", player )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.shadow_dance );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    trigger_gcd = 0;
    cooldown = 120;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, rogue_t* p ) : event_t( sim, p )
      {
        name = "Shadow Dance Expiration";
        p -> aura_gain( "Shadow Dance" );
        p -> buffs_shadow_dance = 1;
        sim -> add_event( this, 10.0 );
      }
      virtual void execute()
      {
        rogue_t* p = player -> cast_rogue();
        p -> aura_loss( "Shadow Dance" );
        p -> buffs_shadow_dance = 0;
      }
    };

    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    new ( sim ) expiration_t( sim, p );
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
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    enter_stealth( p );
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();
    return p -> buffs_stealthed == 0;
  }
};

// Tricks of the Trade =======================================================

struct tricks_of_the_trade_t : public spell_t
{
  player_t* tricks_target;

  tricks_of_the_trade_t( player_t* player, const std::string& options_str ) : 
    spell_t( "tricks_target", player, RESOURCE_ENERGY )
  {
    rogue_t* p = player -> cast_rogue();

    std::string target_str = p -> tricks_of_the_trade_target_str;

    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    
    base_cost = 15;
    cooldown  = 30.0;
    cooldown -= p -> talents.filthy_tricks * 5.0;

    if( target_str.empty() )
    {
      tricks_target = p;
    }
    else
    {
      tricks_target = sim -> find_player( target_str );

      assert ( tricks_target != 0 );
      assert ( tricks_target != player );
    }
  }
   
  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    if( sim -> log ) report_t::log( sim, "%s grants %s Tricks of the Trade", p -> name(), tricks_target -> name() );

    consume_resource();
    update_ready();
    
    p -> buffs_tricks_target = tricks_target;
    p -> buffs_tricks_ready  = 1;
  }

  virtual bool ready()
  {
    if( tricks_target -> buffs.tricks_of_the_trade )
      return false;

    return spell_t::ready();
  }
};

// Vanish ===================================================================

struct vanish_t : public spell_t
{
  vanish_t( player_t* player, const std::string& options_str ) : 
    spell_t( "vanish", player )
  {
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    trigger_gcd = 0;
    cooldown = 180;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    enter_stealth( p );
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();
    return p -> buffs_stealthed < 1;
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Rogue Character Definition
// =========================================================================

// rogue_t::create_action  =================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if( name == "auto_attack"         ) return new auto_attack_t        ( this, options_str );
  if( name == "adrenaline_rush"     ) return new adrenaline_rush_t    ( this, options_str );
  if( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if( name == "apply_poison"        ) return new apply_poison_t       ( this, options_str );
  if( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if( name == "blade_flurry"        ) return new blade_flurry_t       ( this, options_str );
  if( name == "cold_blood"          ) return new cold_blood_t         ( this, options_str );
  if( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if( name == "expose_armor"        ) return new expose_armor_t       ( this, options_str );
  if( name == "feint"               ) return new feint_t              ( this, options_str );
  if( name == "ghostly_strike"      ) return new ghostly_strike_t     ( this, options_str );
  if( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if( name == "hunger_for_blood"    ) return new hunger_for_blood_t   ( this, options_str );
  if( name == "killing_spree"       ) return new killing_spree_t      ( this, options_str );
  if( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if( name == "pool_energy"         ) return new pool_energy_t        ( this, options_str );
  if( name == "premeditation"       ) return new premeditation_t      ( this, options_str );
  if( name == "preparation"         ) return new preparation_t        ( this, options_str );
  if( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if( name == "shadow_dance"        ) return new shadow_dance_t       ( this, options_str );
  if( name == "shadowstep"          ) return new shadowstep_t         ( this, options_str );
  if( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
  if( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if( name == "vanish"              ) return new vanish_t             ( this, options_str );
  if( name == "tricks_of_the_trade" ) return new tricks_of_the_trade_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// rogue_t::init_base ========================================================

void rogue_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = 113;
  attribute_base[ ATTR_AGILITY   ] = 189;
  attribute_base[ ATTR_STAMINA   ] = 105;
  attribute_base[ ATTR_INTELLECT ] =  37;
  attribute_base[ ATTR_SPIRIT    ] =  65;

  attribute_multiplier_initial[ ATTR_AGILITY ] *= 1.0 + talents.sinister_calling * 0.03;

  base_attack_power = ( level * 2 ) - 20;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 1.0;
  initial_attack_power_multiplier  *= 1.0 + ( talents.savage_combat * 0.02 +
                                              talents.deadliness    * 0.02 );

  base_attack_crit = -0.003;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.000355, 0.000250, 0.000120 );

  base_attack_expertise = 0.25 * talents.weapon_expertise * 0.05;

  resource_base[ RESOURCE_HEALTH ] = 3524;
  resource_base[ RESOURCE_ENERGY ] = 100 + ( talents.vigor ? 10 : 0 ) + ( glyphs.vigor ? 10 : 0 );

  health_per_stamina       = 10;
  energy_regen_per_second  = 10;
  energy_regen_per_second *= 1.0 + util_t::talent_rank( talents.vitality, 3, 0.08, 0.16, 0.25 );

  base_gcd = 1.0;
}

// rogue_t::combat_begin ===================================================

void rogue_t::combat_begin() 
{
  player_t::combat_begin();

  if( talents.honor_among_thieves )
  {
    if( honor_among_thieves_interval > 0 )
    {
      if( party != 0 )
      {
	printf( "simcraft: %s cannot have both a party specification and a non-zero 'honor_among_thieves_interval' value.\n", name() );
	exit(0);
      }

      struct honor_among_thieves_proc_t : public event_t
      {
	honor_among_thieves_proc_t( sim_t* sim, rogue_t* p, double interval ) : event_t( sim, p )
	{
	  name = "Honor Among Thieves Proc";
	  sim -> add_event( this, interval );
	}
	virtual void execute()
	{
	  rogue_t* p = player -> cast_rogue();
	  add_combo_point( p );
	  p -> procs_honor_among_thieves -> occur();
	  // Next proc comes in +/- 50% random range centered on 'honor_among_thieves_interval'
	  double interval = p -> honor_among_thieves_interval;
	  interval += ( sim -> rng -> real() - 0.5 ) * interval;
	  new ( sim ) honor_among_thieves_proc_t( sim, p, interval );
	}
      };
      
      // First proc comes 1.0 seconds into combat.
      new ( sim ) honor_among_thieves_proc_t( sim, this, 1.0 );
    }
    else if( party == 0 )
    {
      printf( "simcraft: %s must have a party specification or 'honor_among_thieves_interval' must be set.\n", name() );
      exit(0);
    }
  }
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  // Active
  active_rupture        = 0;
  active_slice_and_dice = 0;

  // Buffs
  buffs_adrenaline_rush    = 0;
  buffs_blade_flurry       = 0;
  buffs_cold_blood         = 0;
  buffs_combo_points       = 0;
  buffs_poison_doses       = 0;
  buffs_envenom            = 0;
  buffs_hunger_for_blood   = 0;
  buffs_master_of_subtlety = 0;
  buffs_overkill           = 0;
  buffs_shadow_dance       = 0;
  buffs_shadowstep         = 0;
  buffs_shiv               = 0;
  buffs_slice_and_dice     = 0;
  buffs_stealthed          = 0;
  buffs_tricks_ready       = 0;
  buffs_tricks_target      = 0;

  // Cooldowns
  cooldowns_honor_among_thieves = 0;
  cooldowns_seal_fate           = 0;

  // Expirations
  expirations_envenom          = 0;
  expirations_slice_and_dice   = 0;
  expirations_hunger_for_blood = 0;
}

// rogue_t::raid_event ====================================================

void rogue_t::raid_event( action_t* a )
{
  if( talents.honor_among_thieves && ( honor_among_thieves_interval == 0 ) )
  {
    player_t* p = a -> player;

    if( p -> party != 0 &&
        p -> party == party  &&
	a -> result == RESULT_CRIT && 
	! a -> proc &&
	! a -> may_glance &&
	sim -> roll( talents.honor_among_thieves / 3.0 ) )
    {
      if( sim -> current_time > p -> cooldowns.honor_among_thieves )
      {
	add_combo_point( this );
	procs_honor_among_thieves -> occur();
	p -> cooldowns.honor_among_thieves = sim -> current_time + 1.0;
      }
    }
  }
}

// rogue_t::regen ==========================================================

void rogue_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if( buffs_adrenaline_rush )
  {
    if( sim -> infinite_resource[ RESOURCE_ENERGY ] == 0 && resource_max[ RESOURCE_ENERGY ] > 0 )
    {
      double energy_regen = periodicity * energy_regen_per_second;

      resource_gain( RESOURCE_ENERGY, energy_regen, gains_adrenaline_rush );
    }
  }
  
  uptimes_energy_cap -> update( resource_current[ RESOURCE_ENERGY ] == 
				resource_max    [ RESOURCE_ENERGY ] );
}

// rogue_t::get_talent_trees ==============================================

bool rogue_t::get_talent_trees( std::vector<int*>& assassination,
                                std::vector<int*>& combat,
                                std::vector<int*>& subtlety )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.improved_eviscerate   ) }, {  1, NULL                                   }, {  1, &( talents.relentless_strikes         ) } },
    { {  2, NULL                               }, {  2, &( talents.improved_sinister_strike  ) }, {  2, NULL                                    } },
    { {  3, &( talents.malice                ) }, {  3, &( talents.dual_wield_specialization ) }, {  3, &( talents.opportunity                ) } },
    { {  4, &( talents.ruthlessness          ) }, {  4, &( talents.improved_slice_and_dice   ) }, {  4, &( talents.sleight_of_hand            ) } },
    { {  5, &( talents.blood_spatter         ) }, {  5, NULL                                   }, {  5, NULL                                    } },
    { {  6, &( talents.puncturing_wounds     ) }, {  6, &( talents.precision                 ) }, {  6, NULL                                    } },
    { {  7, &( talents.vigor                 ) }, {  7, NULL                                   }, {  7, NULL                                    } },
    { {  8, &( talents.improved_expose_armor ) }, {  8, NULL                                   }, {  8, &( talents.ghostly_strike             ) } },
    { {  9, &( talents.lethality             ) }, {  9, &( talents.close_quarters_combat     ) }, {  9, &( talents.serrated_blades            ) } },
    { { 10, &( talents.vile_poisons          ) }, { 10, NULL                                   }, { 10, NULL                                    } },
    { { 11, &( talents.improved_poisons      ) }, { 11, NULL                                   }, { 11, &( talents.initiative                 ) } },
    { { 12, NULL                               }, { 12, NULL                                   }, { 12, &( talents.improved_ambush            ) } },
    { { 13, &( talents.cold_blood            ) }, { 13, &( talents.aggression                ) }, { 13, NULL                                    } },
    { { 14, NULL                               }, { 14, &( talents.mace_specialization       ) }, { 14, &( talents.preparation                ) } },
    { { 15, &( talents.quick_recovery        ) }, { 15, &( talents.blade_flurry              ) }, { 15, &( talents.dirty_deeds                ) } },
    { { 16, &( talents.seal_fate             ) }, { 16, &( talents.sword_specialization      ) }, { 16, &( talents.hemorrhage                 ) } },
    { { 17, &( talents.murder                ) }, { 17, &( talents.weapon_expertise          ) }, { 17, &( talents.master_of_subtlety         ) } },
    { { 18, NULL                               }, { 18, &( talents.blade_twisting            ) }, { 18, &( talents.deadliness                 ) } },
    { { 19, &( talents.overkill              ) }, { 19, &( talents.vitality                  ) }, { 19, NULL                                    } },
    { { 20, NULL                               }, { 20, &( talents.adrenaline_rush           ) }, { 20, &( talents.premeditation              ) } },
    { { 21, &( talents.focused_attacks       ) }, { 21, NULL                                   }, { 21, NULL                                    } },
    { { 22, &( talents.find_weakness         ) }, { 22, NULL                                   }, { 22, &( talents.sinister_calling           ) } },
    { { 23, &( talents.master_poisoner       ) }, { 23, &( talents.combat_potency            ) }, { 23, NULL                                    } },
    { { 24, &( talents.mutilate              ) }, { 24, NULL                                   }, { 24, &( talents.honor_among_thieves        ) } },
    { { 25, &( talents.turn_the_tables       ) }, { 25, &( talents.surprise_attacks          ) }, { 25, &( talents.shadowstep                 ) } },
    { { 26, &( talents.cut_to_the_chase      ) }, { 26, &( talents.savage_combat             ) }, { 26, &( talents.filthy_tricks              ) } },
    { { 27, &( talents.hunger_for_blood      ) }, { 27, &( talents.prey_on_the_weak          ) }, { 27, &( talents.slaughter_from_the_shadows ) } },
    { {  0, NULL                               }, { 28, &( talents.killing_spree             ) }, { 28, &( talents.shadow_dance               ) } },
    { {  0, NULL                               }, {  0, NULL                                   }, {  0, NULL                                    } }
  };
  
  return player_t::get_talent_trees( assassination, combat, subtlety, translation );
}

// rogue_t::parse_talents_mmo =============================================

bool rogue_t::parse_talents_mmo( const std::string& talent_string )
{
  // rogue mmo encoding: Combat-Assassination-Subtlety

  int size1 = 28;
  int size2 = 27;

  std::string        combat_string( talent_string,     0,  size1 );
  std::string assassination_string( talent_string, size1,  size2 );
  std::string      subtlety_string( talent_string, size1 + size2  );

  return parse_talents( assassination_string + combat_string + subtlety_string );
}

// rogue_t::parse_option  ==============================================

bool rogue_t::parse_option( const std::string& name,
                            const std::string& value )
{
  option_t options[] =
  {
    // Talents
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
    { "lethality",                  OPT_INT, &( talents.lethality                  ) },
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
    // Glyphs
    { "glyph_adrenaline_rush",      OPT_INT, &( glyphs.adrenaline_rush             ) },
    { "glyph_blade_flurry",         OPT_INT, &( glyphs.blade_flurry                ) },
    { "glyph_eviscerate",           OPT_INT, &( glyphs.eviscerate                  ) },
    { "glyph_expose_armor",         OPT_INT, &( glyphs.expose_armor                ) },
    { "glyph_feint",                OPT_INT, &( glyphs.feint                       ) },
    { "glyph_ghostly_strike",       OPT_INT, &( glyphs.ghostly_strike              ) },
    { "glyph_hemorrhage",           OPT_INT, &( glyphs.hemorrhage                  ) },
    { "glyph_preparation",          OPT_INT, &( glyphs.preparation                 ) },
    { "glyph_rupture",              OPT_INT, &( glyphs.rupture                     ) },
    { "glyph_sinister_strike",      OPT_INT, &( glyphs.sinister_strike             ) },
    { "glyph_slice_and_dice",       OPT_INT, &( glyphs.slice_and_dice              ) },
    { "glyph_vigor",                OPT_INT, &( glyphs.vigor                       ) },
    // Options
    { "honor_among_thieves_interval", OPT_FLT,    &( honor_among_thieves_interval   ) },
    { "tricks_of_the_trade_target",   OPT_STRING, &( tricks_of_the_trade_target_str ) },
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

// player_t::create_rogue  =================================================

player_t* player_t::create_rogue( sim_t*       sim, 
                                  std::string& name ) 
{
  return new rogue_t( sim, name );
}
