// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Rogue
// ==========================================================================

#define MAX_COMBO_POINTS 5

struct rogue_t : public player_t
{
  // Active
  action_t* active_deadly_poison;
  action_t* active_instant_poison;
  action_t* active_wound_poison;

  // Buffs
  int8_t    buffs_adrenaline_rush;
  int8_t    buffs_blade_flurry;
  int8_t    buffs_combo_points;
  int8_t    buffs_envenom;
  int8_t    buffs_poison_doses;
  int8_t    buffs_shiv;
  int8_t    buffs_slice_and_dice;
  int8_t    buffs_stealthed;
  int8_t    buffs_tricks_ready;
  player_t* buffs_tricks_target;

  // Cooldowns
  double cooldowns_;

  // Expirations
  event_t* expirations_envenom;
  event_t* expirations_slice_and_dice;

  // Gains
  gain_t* gains_;

  // Procs
  proc_t* procs_sword_specialization;
  
  // Up-Times
  uptime_t* uptimes_envenom;
  uptime_t* uptimes_slice_and_dice;
  
  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  struct talents_t
  {
    // Talents implemented
    int8_t adrenaline_rush;
    int8_t aggression;
    int8_t blade_flurry;
    int8_t blade_twisting;

    // Talents not yet implemented
    int8_t blood_spatter;
    int8_t close_quarters_combat;
    int8_t cold_blood;
    int8_t combat_potency;
    int8_t cut_to_the_chase;
    int8_t deadliness;
    int8_t deadly_brew;
    int8_t dirty_deeds;
    int8_t dirty_tricks;
    int8_t dual_wield_specialization;
    int8_t filthy_tricks;
    int8_t find_weakness;
    int8_t focused_attacks;
    int8_t ghostly_strike;
    int8_t hemorrhage;
    int8_t honor_among_thieves;
    int8_t hunger_for_blood;
    int8_t improved_ambush;
    int8_t improved_eviscerate;
    int8_t improved_expose_armor;
    int8_t improved_gouge;
    int8_t improved_kidney_shot;
    int8_t improved_poisons;
    int8_t improved_sinister_strike;
    int8_t improved_slice_and_dice;
    int8_t initiative;
    int8_t lethality;
    int8_t mace_specialization;
    int8_t malice;
    int8_t master_of_subtlety;
    int8_t master_poisoner;
    int8_t murder;
    int8_t murder_spree;
    int8_t mutilate;
    int8_t opportunity;
    int8_t overkill;
    int8_t precision;
    int8_t premeditation;
    int8_t preparation;
    int8_t prey_on_the_weak;
    int8_t puncturing_wounds;
    int8_t quick_recovery;
    int8_t relentless_strikes;
    int8_t riposte;
    int8_t ruthlessness;
    int8_t savage_combat;
    int8_t seal_fate;
    int8_t serrated_blades;
    int8_t shadow_dance;
    int8_t shadowstep;
    int8_t sinister_calling;
    int8_t slaughter_from_the_shadows;
    int8_t sleight_of_hand;
    int8_t surprise_attacks;
    int8_t sword_specialization;
    int8_t turn_the_tables;
    int8_t vigor;
    int8_t vile_poisons;
    int8_t vitality;
    int8_t weapon_expertise;
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int8_t dummy;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  rogue_t( sim_t* sim, std::string& name ) : player_t( sim, ROGUE, name )
  {
    // Active
    active_deadly_poison  = 0;
    active_instant_poison = 0;
    active_wound_poison   = 0;

    // Buffs
    buffs_blade_flurry    = 0;
    buffs_adrenaline_rush = 0;
    buffs_combo_points    = 0;
    buffs_envenom         = 0;
    buffs_poison_doses    = 0;
    buffs_shiv            = 0;
    buffs_slice_and_dice  = 0;
    buffs_stealthed       = 0;
    buffs_tricks_ready    = 0;
    buffs_tricks_target   = 0;

    // Cooldowns
    cooldowns_ = 0;

    // Expirations
    expirations_envenom        = 0;
    expirations_slice_and_dice = 0;
  
    // Gains
    gains_ = get_gain( "dummy" );

    // Procs
    procs_sword_specialization = get_proc( "sword_specialization" );

    // Up-Times
    uptimes_envenom        = get_uptime( "envenom" );
    uptimes_slice_and_dice = get_uptime( "slice_and_dice" );
  
    // Auto-Attack
    main_hand_attack = 0;
    off_hand_attack  = 0;

  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual bool      get_talent_trees( std::vector<int8_t*>& assassination, std::vector<int8_t*>& combat, std::vector<int8_t*>& subtlety );
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
  void clear_combo_points()
  {
    if( buffs_combo_points <= 0 ) return;
    buffs_combo_points = 0;
    aura_loss( "Combo Points" );
  }
};

namespace { // ANONYMOUS NAMESPACE =========================================

// ==========================================================================
// Rogue Attack
// ==========================================================================

struct rogue_attack_t : public attack_t
{
  int8_t  requires_weapon;
  int8_t  requires_position;
  bool    requires_stealth;
  bool    requires_combo_points;
  bool    adds_combo_points;
  int16_t min_energy, min_combo_points;
  int16_t max_energy, max_combo_points;

  rogue_attack_t( const char* n, player_t* player, int8_t s=SCHOOL_PHYSICAL, int8_t t=TREE_NONE ) : 
    attack_t( n, player, RESOURCE_ENERGY, s, t ), 
    requires_weapon(WEAPON_NONE),
    requires_position(POSITION_NONE),
    requires_stealth(false),
    requires_combo_points(false),
    adds_combo_points(false), 
    min_energy(0), min_combo_points(0), 
    max_energy(0), max_combo_points(0)
  {
    rogue_t* p = player -> cast_rogue();
    base_crit += p -> talents.malice * 0.01;
    base_hit  += p -> talents.precision * 0.01;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost();
  virtual double haste();
  virtual double execute_time();
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual void   target_debuff( int8_t dmg_type );
  virtual bool   ready();
};

// trigger_poisons =========================================================

static void trigger_poisons( rogue_attack_t* a )
{
  weapon_t* w = a -> weapon;

  if( ! w ) return;

  rogue_t* p = a -> player -> cast_rogue();

  if( w -> buff == DEADLY_POISON )
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

// trigger_sword_specialization ============================================

static void trigger_sword_specialization( rogue_attack_t* a )
{
  if( a -> proc ) return;

  weapon_t* w = a -> weapon;

  if( ! w ) return;

  if( w -> type != WEAPON_SWORD    &&
      w -> type != WEAPON_SWORD_2H )
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

// trigger_combo_points ====================================================

static void trigger_combo_points( rogue_attack_t* a )
{
  rogue_t* p = a -> player -> cast_rogue();

  if( a -> adds_combo_points )
  {
    if( p -> buffs_combo_points < 5 )
    {
      p -> buffs_combo_points++;
      p -> aura_gain( "Combo Point" );
    }

    if( p -> buffs_combo_points < 5 && p -> talents.seal_fate )
    {
      if( a -> sim -> roll( p -> talents.seal_fate * 0.20 ) )
      {
	p -> buffs_combo_points++;
	p -> aura_gain( "Combo Point" );
      }
    }
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

// =========================================================================
// Rogue Attack
// =========================================================================

// rogue_attack_t::parse_options ===========================================

void rogue_attack_t::parse_options( option_t*          options,
                                    const std::string& options_str )
{
  option_t base_options[] =
  {
    { "min_energy",       OPT_INT16, &min_energy        },
    { "max_energy",       OPT_INT16, &max_energy        },
    { "min_combo_points", OPT_INT16, &min_combo_points  },
    { "max_combo_points", OPT_INT16, &max_combo_points  },
    { NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// rogue_attack_t::cost ====================================================

double rogue_attack_t::cost()
{
  //rogue_t* p = player -> cast_rogue();
  double c = attack_t::cost();

  return c;
}

// rogue_attack_t::haste ===================================================

double rogue_attack_t::haste()
{
  //rogue_t* p = player -> cast_rogue();
  double h = attack_t::haste();

  return h;
}

// rogue_attack_t::execute_time ============================================

double rogue_attack_t::execute_time()
{
  //rogue_t* p = player -> cast_rogue();
  double t = attack_t::execute_time();

  return t;
}

// rogue_attack_t::execute =================================================

void rogue_attack_t::execute()
{
  rogue_t* p = player -> cast_rogue();

  attack_t::execute();
  
  if( result_is_hit() )
  {
    trigger_poisons( this );
    trigger_sword_specialization( this );
    trigger_combo_points( this );

    if( result == RESULT_CRIT )
    {
    }
  }

  trigger_tricks_of_the_trade( this );

  p -> buffs_stealthed = -1;  // In-Combat
}

// rogue_attack_t::consume_resource =======================================
  
void rogue_attack_t::consume_resource()
{
  rogue_t* p = player -> cast_rogue();
  attack_t::consume_resource();
  if( requires_combo_points ) p -> clear_combo_points();
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
      if( weapon -> type == WEAPON_MACE    ||
          weapon -> type == WEAPON_MACE_2H )
      {
        player_penetration += p -> talents.mace_specialization * 0.03;
      }
    }
  }
  if( p -> buffs_poison_doses ) player_multiplier *= 1.0 + p -> talents.savage_combat * 0.01;
}

// rogue_attack_t::target_debuff ==========================================

void rogue_attack_t::target_debuff( int8_t dmg_type )
{
  //rogue_t* p = player -> cast_rogue();
  attack_t::target_debuff( dmg_type );

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

  if( min_energy > 0 )
    if( p -> resource_current[ RESOURCE_ENERGY ] < min_energy )
      return false;

  if( max_energy > 0 )
    if( p -> resource_current[ RESOURCE_ENERGY ] > max_energy )
      return false;

  if( min_combo_points > 0 )
    if( p -> buffs_combo_points < min_combo_points )
      return false;

  if( max_combo_points > 0 )
    if( p -> buffs_combo_points > max_combo_points )
      return false;

  if( requires_combo_points && ( p -> buffs_combo_points == 0 ) )
    return false;

  return true;
}

// =========================================================================
// Rogue Attacks
// =========================================================================

// Melee Attack ============================================================

struct melee_t : public rogue_attack_t
{
  melee_t( const char* name, player_t* player ) : 
    rogue_attack_t( name, player )
  {
    rogue_t* p = player -> cast_rogue();

    base_direct_dmg = 1;
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
    if( p -> buffs_slice_and_dice ) t *= 1.0 / 1.30;

    p -> uptimes_slice_and_dice -> update( p -> buffs_slice_and_dice != 0 );

    return t;
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
    return( p -> main_hand_attack -> event == 0 ); // not swinging
  }
};

// Deadly Poison ============================================================

struct deadly_poison_t : public rogue_attack_t
{
  deadly_poison_t( player_t* player ) : 
    rogue_attack_t( "deadly_poison", player, SCHOOL_NATURE, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd    = 0;
    background     = true;
    proc           = true;
    num_ticks      = 4;
    base_tick_time = 3.0;
    tick_power_mod = 0.08 / num_ticks;
    base_td_init   = util_t::ability_rank( p -> level,  244,76,  204,70,  160,62,  96,0  ) / num_ticks;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int8_t success = p -> buffs_shiv;

    if( ! success )
    {
      double chance = 0.30;
      if( p -> buffs_envenom ) chance += 0.15;
      p -> uptimes_envenom -> update( p -> buffs_envenom != 0 );
      success = sim -> rng -> roll( chance );
    }

    if( success )
    {
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
    rogue_attack_t::player_buff();
    player_multiplier *= p -> buffs_poison_doses;
  }

  virtual void last_tick()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::last_tick();
    p -> buffs_poison_doses = 0;
  }
};

// Instant Poison ===========================================================

struct instant_poison_t : public rogue_attack_t
{
  instant_poison_t( player_t* player ) : 
    rogue_attack_t( "instant_poison", player, SCHOOL_NATURE, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd      = 0;
    background       = true;
    proc             = true;
    direct_power_mod = 0.10;
    base_direct_dmg  = util_t::ability_rank( p -> level,  245,73,  161,68,  76,0 );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int8_t success = p -> buffs_shiv;

    if( ! success )
    {
      double chance = 0.20;
      if( p -> buffs_envenom ) chance += 0.15;
      p -> uptimes_envenom -> update( p -> buffs_envenom != 0 );
      success = sim -> rng -> roll( chance );
    }

    if( success )
    {
      rogue_attack_t::execute();
    }
  }
};

// Wound Poison ============================================================

struct wound_poison_t : public rogue_attack_t
{
  wound_poison_t( player_t* player ) : 
    rogue_attack_t( "wound_poison", player, SCHOOL_NATURE, TREE_ASSASSINATION )
  {
    rogue_t* p = player -> cast_rogue();
    trigger_gcd      = 0;
    background       = true;
    proc             = true;
    direct_power_mod = 0.04;
    base_direct_dmg  = util_t::ability_rank( p -> level,  231,78,  188,72,  112,64,  53,0 );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();

    int8_t success = p -> buffs_shiv;

    if( ! success )
    {
      double chance = 0.50;
      if( p -> buffs_envenom ) chance += 0.15;
      p -> uptimes_envenom -> update( p -> buffs_envenom != 0 );
      success = sim -> rng -> roll( chance );
    }

    if( success )
    {
      rogue_attack_t::execute();
    }
  }
};

// Apply Poison ===========================================================

struct apply_poison_t : public spell_t
{
  int8_t main_hand_poison;
  int8_t  off_hand_poison;

  apply_poison_t( player_t* player, const std::string& options_str ) : 
    spell_t( "apply_poison", player ), 
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
      printf( "simcraft: At least one of 'main_hand/off_hand' must be specified in 'apply_poison'.  Accepted values are 'deadly/instant/wound'.\n" );
      exit( 0 );
    }

    if( main_hand_str == "deadly"  ) main_hand_poison =  DEADLY_POISON;
    if( main_hand_str == "instant" ) main_hand_poison = INSTANT_POISON;
    if( main_hand_str == "wound"   ) main_hand_poison =   WOUND_POISON;

    if( off_hand_str == "deadly"  ) off_hand_poison =  DEADLY_POISON;
    if( off_hand_str == "instant" ) off_hand_poison = INSTANT_POISON;
    if( off_hand_str == "wound"   ) off_hand_poison =   WOUND_POISON;

    if( main_hand_poison != WEAPON_BUFF_NONE ) assert( p -> main_hand_weapon.type != WEAPON_NONE );
    if(  off_hand_poison != WEAPON_BUFF_NONE ) assert( p ->  off_hand_weapon.type != WEAPON_NONE );

    if( ! p -> active_deadly_poison  ) p -> active_deadly_poison  = new  deadly_poison_t( p );
    if( ! p -> active_instant_poison ) p -> active_instant_poison = new instant_poison_t( p );
    if( ! p -> active_wound_poison   ) p -> active_wound_poison   = new   wound_poison_t( p );
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
    may_glance             = false;

    weapon_multiplier *= 2.75;
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
    may_glance             = false;
    weapon_multiplier     *= 1.50;
    base_multiplier       *= 1.0 + ( p -> talents.aggression     * 0.03 +
				     p -> talents.blade_twisting * 0.05 );
  }
};

// Envenom ==================================================================

struct envenom_t : public rogue_attack_t
{
  int8_t min_doses;
  int8_t max_doses;
  int8_t no_buff;
  double* combo_point_dmg;

  envenom_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "envenom", player, SCHOOL_NATURE, TREE_ASSASSINATION ), min_doses(0), max_doses(0), no_buff(0)
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> level >= 62 );

    option_t options[] =
    {
      { "min_doses", OPT_INT8, &min_doses },
      { "max_doses", OPT_INT8, &max_doses },
      { "no_buff",   OPT_INT8, &no_buff   },
      { NULL }
    };
    parse_options( options, options_str );
      
    weapon = &( p -> main_hand_weapon );
    requires_combo_points = true;
    may_glance            = false;
    base_cost             = 35;

    static double dmg_80[] = { 216, 432, 648, 864, 1080 };
    static double dmg_74[] = { 176, 352, 528, 704,  880 };
    static double dmg_69[] = { 148, 296, 444, 492,  740 };
    static double dmg_62[] = { 118, 236, 354, 472,  590 };

    combo_point_dmg = ( p -> level >= 80 ? dmg_80 :
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

    double doses_consumed = std::min( p -> buffs_poison_doses, p -> buffs_combo_points );

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
      p -> buffs_poison_doses -= doses_consumed;

      if( p -> buffs_poison_doses == 0 )
      {
	p -> active_deadly_poison -> cancel();
      }
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
      tick_power_mod = doses_consumed * 0.07;
      base_direct_dmg = combo_point_dmg[ doses_consumed-1 ];
      rogue_attack_t::player_buff();
    }
  }

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();

    if( min_doses > 0 )
      if( min_doses < p -> buffs_poison_doses )
	return false;

    if( max_doses > 0 )
      if( max_doses > p -> buffs_poison_doses )
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
      
    weapon = &( p -> main_hand_weapon );
    requires_combo_points = true;
    may_glance            = false;
    base_cost             = 35;
    base_multiplier      *= 1.0 + p -> talents.aggression * 0.03;

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
  }
};

// Expose Armor =============================================================

struct expose_armor_t : public rogue_attack_t
{
  int8_t override_sunder;

  expose_armor_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "expose_armor", player, SCHOOL_PHYSICAL, TREE_ASSASSINATION ), override_sunder(0)
  {
    rogue_t* p = player -> cast_rogue();

    option_t options[] =
    {
      { "override_sunder", OPT_INT8, &override_sunder },
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
    may_glance            = false;
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
	  expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
	  {
	    name = "Expose Armor Expiration";
	    sim -> add_event( this, 30.0 );
	  }
	  virtual void execute()
	  {
	    sim -> target -> debuffs.expose_armor = 0;
	    sim -> target -> expirations.expose_armor = 0;
	  }
	};

	t -> debuffs.expose_armor = debuff;

	event_t*& e = t -> expirations.expose_armor;

	if( e )
	{
	  e -> reschedule( 30.0 );
	}
	else
	{
	  e = new ( sim ) expiration_t( sim, p );
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
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    base_cost = 20;
    cooldown = 10;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    // model threat eventually......
  }
};

// Hemorrhage =============================================================

struct hemorrhage_t : public rogue_attack_t
{
  hemorrhage_t( player_t* player, const std::string& options_str ) : 
    rogue_attack_t( "hemorrhage", player, SCHOOL_PHYSICAL, TREE_SUBTLETY )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.hemorrhage );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    adds_combo_points      = true;
    may_glance             = false;

    weapon_multiplier *= 1.10;
  }
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
      
    may_glance = false;
    base_cost  = 60;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();

    double mh_dd=0, oh_dd=0;
    int8_t mh_result=RESULT_NONE, oh_result=RESULT_NONE;

    weapon = &( player -> main_hand_weapon );
    rogue_attack_t::execute();
    mh_dd = direct_dmg;
    mh_result = result;

    if( result_is_hit() )
    {
      if( player -> off_hand_weapon.type != WEAPON_NONE )
      {
        weapon = &( player -> off_hand_weapon );
        rogue_attack_t::execute();
        oh_dd = direct_dmg;
        oh_result = result;
      }
    }

    direct_dmg = mh_dd + oh_dd;
    result     = mh_result;
    attack_t::update_stats( DMG_DIRECT );

    player -> action_finish( this );
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::player_buff();
    if( p -> buffs_poison_doses > 0 ) player_multiplier *= 1.50;
  }

  virtual void update_stats( int8_t type ) { }
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
      
    weapon = &( p -> main_hand_weapon );
    requires_combo_points = true;
    may_glance            = false;
    base_cost             = 25;
    base_tick_time        = 2.0; 

    static double dmg_79[] = { 145, 163, 181, 199, 217 };
    static double dmg_74[] = { 122, 137, 152, 167, 182 };
    static double dmg_68[] = {  81,  92, 103, 114, 125 };
    static double dmg_60[] = {  68,  76,  84,  92, 100 };

    combo_point_dmg = ( p -> level >= 79 ? dmg_79 :
			p -> level >= 74 ? dmg_74 :
			p -> level >= 68 ? dmg_68 : 
			                   dmg_60 );
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    num_ticks = 3 + p -> buffs_combo_points;
    rogue_attack_t::execute();
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    tick_power_mod = p -> combo_point_rank( 0.015, 0.024, 0.030, 0.03428571, 0.0375 );
    base_td_init   = p -> combo_point_rank( combo_point_dmg );
    rogue_attack_t::player_buff();
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
    may_glance        = false;
    base_direct_dmg   = 1;
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
    may_glance             = false;

    base_cost -= util_t::talent_rank( p -> talents.improved_sinister_strike, 2, 3.0, 5.0 );

    base_multiplier *= 1.0 + ( p -> talents.aggression       * 0.03 +
                               p -> talents.blade_twisting   * 0.05 +
			       p -> talents.find_weakness    * 0.02 +
                               p -> talents.surprise_attacks * 0.10 );

    base_crit_bonus *= 1.0 + ( p -> talents.lethality        * 0.06 +
                               p -> talents.prey_on_the_weak * 0.04 );
  }

  virtual void player_buff()
  {
    rogue_t* p = player -> cast_rogue();
    rogue_attack_t::player_buff();
    if( weapon -> type <= WEAPON_SMALL ) player_crit += p -> talents.close_quarters_combat * 0.01;
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
      }
    };

    rogue_t* p = player -> cast_rogue();

    double duration = 6.0 + 3.0 * p -> buffs_combo_points;

    consume_resource();
    update_ready();

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

  virtual bool ready()
  {
    rogue_t* p = player -> cast_rogue();
    return p -> buffs_slice_and_dice == 0;
  }
};

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

    cooldown = 300;
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
	p -> energy_regen_per_tick *= 2.0;
	sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
	rogue_t* p = player -> cast_rogue();
	p -> aura_loss( "Adrenaline Rush" );
	p -> buffs_adrenaline_rush = 0;
	p -> energy_regen_per_tick /= 2.0;
      }
    };

    rogue_t* p = player -> cast_rogue();

    update_ready();

    new ( sim ) expiration_t( sim, p );
  }
};

// Blade Flurry ============================================================

struct blade_flurry_t : public spell_t
{
  blade_flurry_t( player_t* player, const std::string& options_str ) : 
    spell_t( "blade_flurry", player, RESOURCE_ENERGY )
  {
    rogue_t* p = player -> cast_rogue();
    assert( p -> talents.blade_flurry );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
    
    cooldown  = 120;
    base_cost = 25;
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
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    p -> buffs_stealthed = 1;
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
    spell_t( "tricks_target", player )
  {
    std::string target_str;
    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL }
    };
    parse_options( options, options_str );

    cooldown = 30.0;
    
    tricks_target = sim -> find_player( target_str );

    assert ( tricks_target != 0 );
    assert ( tricks_target != player );
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
      
    cooldown = 180;
  }

  virtual void execute()
  {
    rogue_t* p = player -> cast_rogue();
    p -> buffs_stealthed = 1;
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
  if( name == "apply_poson"         ) return new apply_poison_t       ( this, options_str );
  if( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if( name == "expose_armor"        ) return new expose_armor_t       ( this, options_str );
  if( name == "feint"               ) return new feint_t              ( this, options_str );
  if( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if( name == "rupture"             ) return new rupture_t            ( this, options_str );
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
  // FIXME! These are level 70 values.
  attribute_base[ ATTR_STRENGTH  ] =  94;
  attribute_base[ ATTR_AGILITY   ] = 156;
  attribute_base[ ATTR_STAMINA   ] =  90;
  attribute_base[ ATTR_INTELLECT ] =  37;
  attribute_base[ ATTR_SPIRIT    ] =  65;

  base_attack_power = ( level * 2 ) - 20;
  initial_attack_power_multiplier  *= 1.0 + talents.savage_combat * 0.02;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 1.0;

  base_attack_crit = -0.003;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.0355, 0.0250, 0.0120 );

  resource_base[ RESOURCE_HEALTH ] = 3524;
  resource_base[ RESOURCE_ENERGY ] = 100;

  health_per_stamina    = 10;
  energy_regen_per_tick = 20;

  base_gcd = 1.0;
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  // Buffs
  buffs_adrenaline_rush = 0;
  buffs_blade_flurry    = 0;
  buffs_combo_points    = 0;
  buffs_poison_doses    = 0;
  buffs_envenom         = 0;
  buffs_shiv            = 0;
  buffs_slice_and_dice  = 0;
  buffs_stealthed       = 0;
  buffs_tricks_ready    = 0;
  buffs_tricks_target   = 0;

  // Cooldowns
  cooldowns_ = 0;

  // Expirations
  expirations_envenom        = 0;
  expirations_slice_and_dice = 0;
}

// rogue_t::get_talent_trees ==============================================

bool rogue_t::get_talent_trees( std::vector<int8_t*>& assassination,
                                std::vector<int8_t*>& combat,
                                std::vector<int8_t*>& subtlety )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.improved_eviscerate   ) }, {  1, &( talents.improved_gouge            ) }, {  1, &( talents.relentless_strikes         ) } },
    { {  2, NULL                               }, {  2, &( talents.improved_sinister_strike  ) }, {  2, NULL                                    } },
    { {  3, &( talents.malice                ) }, {  3, &( talents.dual_wield_specialization ) }, {  3, &( talents.opportunity                ) } },
    { {  4, &( talents.ruthlessness          ) }, {  4, &( talents.improved_slice_and_dice   ) }, {  4, &( talents.sleight_of_hand            ) } },
    { {  5, &( talents.blood_spatter         ) }, {  5, NULL                                   }, {  5, &( talents.dirty_tricks               ) } },
    { {  6, &( talents.puncturing_wounds     ) }, {  6, &( talents.precision                 ) }, {  6, NULL                                    } },
    { {  7, &( talents.vigor                 ) }, {  7, NULL                                   }, {  7, NULL                                    } },
    { {  8, &( talents.improved_expose_armor ) }, {  8, &( talents.riposte                   ) }, {  8, &( talents.ghostly_strike             ) } },
    { {  9, &( talents.lethality             ) }, {  9, &( talents.close_quarters_combat     ) }, {  9, &( talents.serrated_blades            ) } },
    { { 10, &( talents.vile_poisons          ) }, { 10, NULL                                   }, { 10, NULL                                    } },
    { { 11, &( talents.improved_poisons      ) }, { 11, NULL                                   }, { 11, &( talents.initiative                 ) } },
    { { 12, NULL                               }, { 12, NULL                                   }, { 12, &( talents.improved_ambush            ) } },
    { { 13, &( talents.cold_blood            ) }, { 13, &( talents.mace_specialization       ) }, { 13, NULL                                    } },
    { { 14, &( talents.improved_kidney_shot  ) }, { 14, &( talents.blade_flurry              ) }, { 14, &( talents.preparation                ) } },
    { { 15, &( talents.quick_recovery        ) }, { 15, &( talents.sword_specialization      ) }, { 15, &( talents.dirty_deeds                ) } },
    { { 16, &( talents.seal_fate             ) }, { 16, &( talents.aggression                ) }, { 16, &( talents.hemorrhage                 ) } },
    { { 17, &( talents.murder                ) }, { 17, &( talents.weapon_expertise          ) }, { 17, &( talents.master_of_subtlety         ) } },
    { { 18, &( talents.deadly_brew           ) }, { 18, NULL                                   }, { 18, &( talents.deadliness                 ) } },
    { { 19, &( talents.overkill              ) }, { 19, &( talents.vitality                  ) }, { 19, NULL                                    } },
    { { 20, NULL                               }, { 20, &( talents.adrenaline_rush           ) }, { 20, &( talents.premeditation              ) } },
    { { 21, &( talents.focused_attacks       ) }, { 21, NULL                                   }, { 21, NULL                                    } },
    { { 22, &( talents.find_weakness         ) }, { 22, NULL                                   }, { 22, &( talents.sinister_calling           ) } },
    { { 23, &( talents.master_poisoner       ) }, { 23, &( talents.combat_potency            ) }, { 23, NULL                                    } },
    { { 24, &( talents.mutilate              ) }, { 24, NULL                                   }, { 24, &( talents.honor_among_thieves        ) } },
    { { 25, &( talents.turn_the_tables       ) }, { 25, &( talents.surprise_attacks          ) }, { 25, &( talents.shadowstep                 ) } },
    { { 26, &( talents.cut_to_the_chase      ) }, { 26, &( talents.savage_combat             ) }, { 26, &( talents.filthy_tricks              ) } },
    { { 27, &( talents.hunger_for_blood      ) }, { 27, &( talents.prey_on_the_weak          ) }, { 27, &( talents.slaughter_from_the_shadows ) } },
    { {  0, NULL                               }, { 28, &( talents.murder_spree              ) }, { 28, &( talents.shadow_dance               ) } },
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
    { "adrenaline_rush",            OPT_INT8, &( talents.adrenaline_rush            ) },
    { "aggression",                 OPT_INT8, &( talents.aggression                 ) },
    { "blade_flurry",               OPT_INT8, &( talents.blade_flurry               ) },
    { "blade_twisting",             OPT_INT8, &( talents.blade_twisting             ) },
    { "blood_spatter",              OPT_INT8, &( talents.blood_spatter              ) },
    { "close_quarters_combat",      OPT_INT8, &( talents.close_quarters_combat      ) },
    { "cold_blood",                 OPT_INT8, &( talents.cold_blood                 ) },
    { "combat_potency",             OPT_INT8, &( talents.combat_potency             ) },
    { "cut_to_the_chase",           OPT_INT8, &( talents.cut_to_the_chase           ) },
    { "deadliness",                 OPT_INT8, &( talents.deadliness                 ) },
    { "deadly_brew",                OPT_INT8, &( talents.deadly_brew                ) },
    { "dirty_deeds",                OPT_INT8, &( talents.dirty_deeds                ) },
    { "dirty_tricks",               OPT_INT8, &( talents.dirty_tricks               ) },
    { "dual_wield_specialization",  OPT_INT8, &( talents.dual_wield_specialization  ) },
    { "filthy_tricks",              OPT_INT8, &( talents.filthy_tricks              ) },
    { "find_weakness",              OPT_INT8, &( talents.find_weakness              ) },
    { "focused_attacks",            OPT_INT8, &( talents.focused_attacks            ) },
    { "ghostly_strike",             OPT_INT8, &( talents.ghostly_strike             ) },
    { "hemorrhage",                 OPT_INT8, &( talents.hemorrhage                 ) },
    { "honor_among_thieves",        OPT_INT8, &( talents.honor_among_thieves        ) },
    { "hunger_for_blood",           OPT_INT8, &( talents.hunger_for_blood           ) },
    { "improved_ambush",            OPT_INT8, &( talents.improved_ambush            ) },
    { "improved_eviscerate",        OPT_INT8, &( talents.improved_eviscerate        ) },
    { "improved_expose_armor",      OPT_INT8, &( talents.improved_expose_armor      ) },
    { "improved_gouge",             OPT_INT8, &( talents.improved_gouge             ) },
    { "improved_kidney_shot",       OPT_INT8, &( talents.improved_kidney_shot       ) },
    { "improved_poisons",           OPT_INT8, &( talents.improved_poisons           ) },
    { "improved_sinister_strike",   OPT_INT8, &( talents.improved_sinister_strike   ) },
    { "improved_slice_and_dice",    OPT_INT8, &( talents.improved_slice_and_dice    ) },
    { "initiative",                 OPT_INT8, &( talents.initiative                 ) },
    { "lethality",                  OPT_INT8, &( talents.lethality                  ) },
    { "mace_specialization",        OPT_INT8, &( talents.mace_specialization        ) },
    { "malice",                     OPT_INT8, &( talents.malice                     ) },
    { "master_of_subtlety",         OPT_INT8, &( talents.master_of_subtlety         ) },
    { "master_poisoner",            OPT_INT8, &( talents.master_poisoner            ) },
    { "murder",                     OPT_INT8, &( talents.murder                     ) },
    { "murder_spree",               OPT_INT8, &( talents.murder_spree               ) },
    { "mutilate",                   OPT_INT8, &( talents.mutilate                   ) },
    { "opportunity",                OPT_INT8, &( talents.opportunity                ) },
    { "overkill",                   OPT_INT8, &( talents.overkill                   ) },
    { "precision",                  OPT_INT8, &( talents.precision                  ) },
    { "premeditation",              OPT_INT8, &( talents.premeditation              ) },
    { "preparation",                OPT_INT8, &( talents.preparation                ) },
    { "prey_on_the_weak",           OPT_INT8, &( talents.prey_on_the_weak           ) },
    { "puncturing_wounds",          OPT_INT8, &( talents.puncturing_wounds          ) },
    { "quick_recovery",             OPT_INT8, &( talents.quick_recovery             ) },
    { "relentless_strikes",         OPT_INT8, &( talents.relentless_strikes         ) },
    { "riposte",                    OPT_INT8, &( talents.riposte                    ) },
    { "ruthlessness",               OPT_INT8, &( talents.ruthlessness               ) },
    { "savage_combat",              OPT_INT8, &( talents.savage_combat              ) },
    { "seal_fate",                  OPT_INT8, &( talents.seal_fate                  ) },
    { "serrated_blades",            OPT_INT8, &( talents.serrated_blades            ) },
    { "shadow_dance",               OPT_INT8, &( talents.shadow_dance               ) },
    { "shadowstep",                 OPT_INT8, &( talents.shadowstep                 ) },
    { "sinister_calling",           OPT_INT8, &( talents.sinister_calling           ) },
    { "slaughter_from_the_shadows", OPT_INT8, &( talents.slaughter_from_the_shadows ) },
    { "sleight_of_hand",            OPT_INT8, &( talents.sleight_of_hand            ) },
    { "surprise_attacks",           OPT_INT8, &( talents.surprise_attacks           ) },
    { "sword_specialization",       OPT_INT8, &( talents.sword_specialization       ) },
    { "turn_the_tables",            OPT_INT8, &( talents.turn_the_tables            ) },
    { "vigor",                      OPT_INT8, &( talents.vigor                      ) },
    { "vile_poisons",               OPT_INT8, &( talents.vile_poisons               ) },
    { "vitality",                   OPT_INT8, &( talents.vitality                   ) },
    { "weapon_expertise",           OPT_INT8, &( talents.weapon_expertise           ) },
    // Glyphs
    { "glyph_dummy",                OPT_INT8, &( glyphs.dummy                       ) },
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
