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
  spell_t* active_;

  // Buffs
  double buffs_combo_point_events[ MAX_COMBO_POINTS ];
  int8_t buffs_combo_points;
  int8_t buffs_poisons_applied;
  int8_t buffs_stealthed;

  // Cooldowns
  double cooldowns_;

  // Expirations
  event_t* expirations_;

  // Gains
  gain_t* gains_;

  // Procs
  proc_t* procs_sword_specialization;
  
  // Up-Times
  uptime_t* uptimes_;
  
  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  struct talents_t
  {
    // Talents implemented..... (none!)

    // Talents not yet implemented (all!)
    int8_t adrenaline_rush;
    int8_t aggression;
    int8_t blade_flurry;
    int8_t blade_twisting;
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
    active_ = 0;

    // Buffs
    for( int i=0; i < MAX_COMBO_POINTS; i++ ) 
    {
      buffs_combo_point_events[ i ] = 0;
    }
    buffs_combo_points = 0;
    buffs_poisons_applied = 0;
    buffs_stealthed = 0;

    // Cooldowns
    cooldowns_ = 0;

    // Expirations
    expirations_ = 0;
  
    // Gains
    gains_ = get_gain( "dummy" );

    // Procs
    procs_sword_specialization = get_proc( "sword_specialization" );

    // Up-Times
    uptimes_ = get_uptime( "dummy" );
  
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
  bool    adds_combo_points;
  bool    eats_combo_points;
  int16_t min_energy, min_combo_points;
  int16_t max_energy, max_combo_points;

  rogue_attack_t( const char* n, player_t* player, int8_t s=SCHOOL_PHYSICAL, int8_t t=TREE_NONE ) : 
    attack_t( n, player, RESOURCE_ENERGY, s, t ), 
    requires_weapon(WEAPON_NONE),
    requires_position(POSITION_NONE),
    requires_stealth(false),
    adds_combo_points(false), eats_combo_points(false),
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

// trigger_sword_specialization ============================================

static void trigger_sword_specialization( rogue_attack_t* a )
{
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

  if( a -> eats_combo_points )
  {
    p -> buffs_combo_points = 0;
    for( int i=0; i < MAX_COMBO_POINTS; i++ ) 
    {
      p -> buffs_combo_point_events[ i ] = 0;
    }
    p -> aura_loss( "Combo Points" );
  }
  if( a -> adds_combo_points )
  {
    if( p -> buffs_combo_points < 5 )
    {
      p -> buffs_combo_point_events[ p -> buffs_combo_points ] = a -> sim -> current_time;
      p -> buffs_combo_points++;
      p -> aura_gain( "Combo Point" );
    }

    if( p -> buffs_combo_points < 5 && p -> talents.seal_fate )
    {
      if( a -> sim -> roll( p -> talents.seal_fate * 0.20 ) )
      {
	p -> buffs_combo_point_events[ p -> buffs_combo_points ] = a -> sim -> current_time;
	p -> buffs_combo_points++;
	p -> aura_gain( "Combo Point" );
      }
    }
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
  //rogue_t* p = player -> cast_rogue();

  attack_t::execute();
  
  if( result_is_hit() )
  {
    trigger_sword_specialization( this );
    trigger_combo_points( this );

    if( result == RESULT_CRIT )
    {
    }
  }
}

// rogue_attack_t::consume_resource =======================================
  
void rogue_attack_t::consume_resource()
{
  //rogue_t* p = player -> cast_rogue();
  attack_t::consume_resource();

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
  if( p -> buffs_poisons_applied ) player_multiplier *= 1.0 + p -> talents.savage_combat * 0.01;
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
    if( ! sim -> time_to_think( p -> buffs_combo_point_events[ min_combo_points-1 ] ) )
      return false;

  if( max_combo_points > 0 )
    if( sim -> time_to_think( p -> buffs_combo_point_events[ max_combo_points-1 ] ) )
      return false;

  if( eats_combo_points && ( p -> buffs_combo_points == 0 ) )
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

    weapon_multiplier *= 1.50;
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
      
    weapon            = &( p -> main_hand_weapon );
    eats_combo_points = true;
    may_glance        = false;

    base_cost      = 25;
    base_tick_time = 2.0; 

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

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Rogue Character Definition
// =========================================================================

// rogue_t::create_action  =================================================

action_t* rogue_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if( name == "auto_attack"         ) return new auto_attack_t        ( this, options_str );
  if( name == "ambush"              ) return new ambush_t             ( this, options_str );
  if( name == "backstab"            ) return new backstab_t           ( this, options_str );
  if( name == "rupture"             ) return new rupture_t            ( this, options_str );
  if( name == "sinister_strike"     ) return new sinister_strike_t    ( this, options_str );
#if 0
  if( name == "deadly_throw"        ) return new deadly_throw_t       ( this, options_str );
  if( name == "envenom"             ) return new envenom_t            ( this, options_str );
  if( name == "eviscerate"          ) return new eviscerate_t         ( this, options_str );
  if( name == "expose_armor"        ) return new expose_armor_t       ( this, options_str );
  if( name == "feint"               ) return new feint_t              ( this, options_str );
  if( name == "hemorrhage"          ) return new hemorrhage_t         ( this, options_str );
  if( name == "mutilate"            ) return new mutilate_t           ( this, options_str );
  if( name == "shiv"                ) return new shiv_t               ( this, options_str );
  if( name == "slice_and_dice"      ) return new slice_and_dice_t     ( this, options_str );
  if( name == "stealth"             ) return new stealth_t            ( this, options_str );
  if( name == "tricks_of_the_trade" ) return new tricks_of_the_trade_t( this, options_str );
  if( name == "vanish"              ) return new vanish_t             ( this, options_str );
#endif
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

  health_per_stamina = 10;

  base_gcd = 1.0;
}

// rogue_t::reset ===========================================================

void rogue_t::reset()
{
  player_t::reset();

  // Active
  active_ = 0;

  // Buffs
  for( int i=0; i < MAX_COMBO_POINTS; i++ ) 
  {
    buffs_combo_point_events[ i ] = 0;
  }
  buffs_combo_points = 0;
  buffs_poisons_applied = 0;
  buffs_stealthed = 0;

  // Cooldowns
  cooldowns_ = 0;

  // Expirations
  expirations_ = 0;
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
