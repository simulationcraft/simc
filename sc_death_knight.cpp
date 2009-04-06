// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Death Knight Runes
// ==========================================================================


enum rune_type {
  RUNE_TYPE_NONE=0, RUNE_TYPE_BLOOD, RUNE_TYPE_FROST, RUNE_TYPE_UNHOLY, RUNE_TYPE_DEATH, RUNE_TYPE_WASDEATH=8
};

#define RUNE_TYPE_MASK  3
#define RUNE_SLOT_MAX   6

struct dk_rune_t
{
	double cooldown_ready;
	int    type;

  dk_rune_t() : cooldown_ready(0), type(RUNE_TYPE_NONE) {}

  bool is_death(                     ) const { return (type & RUNE_TYPE_DEATH) != 0;  }
	bool is_ready( double current_time ) const { return cooldown_ready <= current_time; }
  int  get_type(                     ) const { return type & RUNE_TYPE_MASK;          }

	void consume( double current_time, double cooldown, bool convert )
	{
		assert ( current_time >= cooldown_ready );

		cooldown_ready = current_time + cooldown;
    type = type & RUNE_TYPE_MASK | type << 1 & RUNE_TYPE_WASDEATH | (convert ? RUNE_TYPE_DEATH : 0) ;
	}

	void refund()   { cooldown_ready = 0; type  = type & RUNE_TYPE_MASK | type >> 1 & RUNE_TYPE_DEATH; }
	void reset()    { cooldown_ready = 0; type  = type & RUNE_TYPE_MASK;                               }
};

// ==========================================================================
// Death Knight
// ==========================================================================

struct death_knight_t : public player_t
{

	// Active
  action_t* active_blood_plague;
	action_t* active_frost_fever;
	int diseases;

  // Buffs
  struct _buffs_t
  {
    double  butchery_timer;
    int     scent_of_blood_stack;

    void reset() { memset( (void*) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Cooldowns
  struct _cooldowns_t
  {
    double  scent_of_blood;

    void reset() 
    { 
      scent_of_blood = 0;
    }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Expirations
  struct _expirations_t
  {
    // EMPTY

    void reset() { memset( (void*) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_rune_abilities;
	gain_t* gains_runic_overflow;
	gain_t* gains_butchery;
	gain_t* gains_scent_of_blood;

  // Procs
  proc_t* procs_scent_of_blood;
  
  // Up-Times
  
  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  // Diseases
  spell_t* blood_plague;
  spell_t* frost_fever;

  // Options
  int scent_of_blood_interval;

  // Talents
  struct talents_t
  {                                     // Status:
	  int butchery;                       // Done
	  int subversion;											// Done
	  int bladed_armor;                   // In progress...pending armor in player_t
	  int scent_of_blood;                 // Done
	  int two_handed_weapon_spec;         // Done
	  int rune_tap;
	  int dark_conviction;
	  int death_rune_mastery;
	  int improved_rune_tap;
	  int bloody_strikes;
	  int veteran_of_the_third_war;
	  int bloody_vengeance;
	  int abominations_might;
	  int bloodworms;
	  int hysteria;
	  int improved_blood_presence;
	  int improved_death_strike;
	  int sudden_doom;
	  int vampiric_blood;
	  int heart_strike;										// Done
	  int might_of_mograine;
	  int blood_gorged;
	  int dancing_rune_weapon;
	  int improved_icy_touch;
	  int runic_power_mastery;						// Done
	  int toughness;
	  int black_ice;
	  int nerves_of_cold_steel;
	  int icy_talons;
	  int annihilation;
	  int killing_machine;
	  int chill_of_the_grave;
	  int endless_winter;
	  int glacier_rot;
	  int deathchill;
	  int improved_icy_talons;
	  int merciless_combat;
	  int rime;
	  int chilblains;
	  int hungering_cold;
	  int improved_frost_presence;
	  int blood_of_the_north;
	  int unbreakable_armor;
	  int frost_strike;
	  int guile_of_gorefiend;
	  int tundra_stalker;
	  int howling_blast;
	  int vicious_strikes;
	  int virulence;
	  int epidemic;
	  int morbidity;
	  int ravenous_dead;
	  int outbreak;
	  int necrosis;
	  int corpse_explosion;
	  int blood_caked_blade;
	  int master_of_ghouls;
	  int unholy_blight;
	  int impurity;
	  int dirge;
	  int reaping;
	  int ghoul_frenzy;
	  int desecration;
	  int improved_unholy_presence;
	  int night_of_the_dead;
	  int crypt_fever;
	  int bone_shield;
	  int wandering_plague;
	  int ebon_plaguebringer;
	  int scourge_strike;
	  int rage_of_rivendare;
	  int summon_gargoyle;

    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  // Glyphs
  struct glyphs_t
  {
	  int blood_tap;
	  int horn_of_winter;
	  int heart_strike;
	  int blood_strike;
	  int chains_of_ice;
	  int dancing_rune_weapon;
	  int dark_death;
	  int death_strike;
	  int death_and_decay;
	  int disease;
	  int frost_strike;
	  int howling_blast;
	  int hungering_cold;
	  int icy_touch;
	  int obliterate;
	  int plague_strike;
	  int rune_strike;
	  int rune_tap;
	  int scourge_strike;
	  int strangulate;
	  int unholy_blight;
	  int vampiric_blood;
	  int the_ghoul;

    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct runes_t
  {
	  dk_rune_t slot[RUNE_SLOT_MAX];

	  runes_t()    { for( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].type = RUNE_TYPE_BLOOD + (i >> 1); }
	  void reset() { for( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].reset();                           }

  };
  runes_t _runes;

  death_knight_t( sim_t* sim, std::string& name ) :
    player_t( sim, DEATH_KNIGHT, name )
  {
    assert ( sim ->patch.after(3,1,0) );

    // Active
    active_blood_plague     = NULL;
		active_frost_fever      = NULL;
		diseases = 0;

    // Gains
    gains_rune_abilities    = get_gain( "rune_abilities" );
		gains_runic_overflow    = get_gain( "runic_overflow" );
		gains_butchery          = get_gain( "butchery" );
		gains_scent_of_blood    = get_gain( "scent_of_blood" );

    // Procs
    procs_scent_of_blood    = get_proc( "scent_of_blood" );
  
    // Auto-Attack
    main_hand_attack        = NULL;
    off_hand_attack         = NULL;

    blood_plague            = NULL;
    frost_fever             = NULL;

    // Options
    scent_of_blood_interval = 0;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      init_resources( bool force );
  virtual void      combat_begin();
  virtual double    composite_attack_power();
  virtual void      regen( double periodicity );
  virtual void      reset();
	virtual double    resource_gain( int resource, double amount, gain_t* g = 0);
  virtual bool      get_talent_trees( std::vector<int*>& blood, std::vector<int*>& frost, std::vector<int*>& unholy );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual action_t* execute_action();
  virtual int       primary_resource() { return RESOURCE_RUNIC; }

  // Utilities
  bool abort_execute_action;
};

namespace { // ANONYMOUS NAMESPACE =========================================

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_attack_t : public attack_t
{
  bool requires_weapon;
  int  requires_position;
  int  cost_blood;
  int  cost_frost;
  int  cost_unholy;
  bool convert_runes;
  bool use[RUNE_SLOT_MAX];
  bool execute_once;
  bool executed_once;

  death_knight_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE ) : 
    attack_t( n, player, RESOURCE_RUNIC, s, t ), 
    requires_weapon(true),
    requires_position(POSITION_NONE),
		cost_blood(0),
		cost_frost(0),
		cost_unholy(0),
		convert_runes(false),
    execute_once(true),
    executed_once(false)
  {
    death_knight_t* p = player -> cast_death_knight();
 		for( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_glance = false;
    if( p -> talents.two_handed_weapon_spec )
      weapon_multiplier *= 1 + (p -> talents.two_handed_weapon_spec * 0.02);
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual void   reset();

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual bool   ready();
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public spell_t
{
  int  cost_blood;
  int  cost_frost;
  int  cost_unholy;
  bool convert_runes;
  bool use[RUNE_SLOT_MAX];
  bool execute_once;
  bool executed_once;

  death_knight_spell_t( const char* n, player_t* player, int s, int t ) : 
    spell_t( n, player, RESOURCE_RUNIC, s, t ),
		cost_blood(0),
		cost_frost(0),
		cost_unholy(0),
		convert_runes(false),
    execute_once(true),
    executed_once(false)
  {
 		for( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual void   reset();

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
	virtual bool   ready();
	virtual double total_crit_bonus();
};

// ==========================================================================
// Local Utility Functions
// ==========================================================================
static void
consume_runes( player_t* player, const bool* use, bool convert_runes = false )
{
	death_knight_t* p = player -> cast_death_knight();
	double t = p -> sim -> current_time;

	for( int i = 0; i < RUNE_SLOT_MAX; ++i )
		if( use[i] ) p -> _runes.slot[i].consume( t, 10.0, convert_runes );
}

static bool
group_runes ( player_t* player, int blood, int frost, int unholy, bool* group )
{
	death_knight_t* p = player -> cast_death_knight();
	double t = p -> sim -> current_time;
	int cost[]  = { blood + frost + unholy, blood, frost, unholy };
	bool use[RUNE_SLOT_MAX] = { false };

  // Selecting available non-death runes to satisfy cost
	for( int i = 0; i < RUNE_SLOT_MAX; ++i )
	{
		dk_rune_t& r = p -> _runes.slot[i];
		if (r.is_ready(t) && ! r.is_death() && cost[r.get_type()] > 0)
		{
			--cost[r.get_type()];
			--cost[0];
			use[i] = true;
		}
	}

  // Selecting available death runes to satisfy remaining cost
	for( int i = 0; cost[0] > 0 && i < RUNE_SLOT_MAX; ++i )
	{
		dk_rune_t& r = p -> _runes.slot[i];
		if (r.is_ready(t) && r.is_death())
		{
			--cost[0];
			use[i] = true;
		}
	}

  if( cost[0] > 0 ) return false;

  // Storing rune slots selected
	for( int i = 0; i < RUNE_SLOT_MAX; ++i) group[i] = use[i];

	return true;
}

static void
refund_runes( player_t* player, const bool* use )
{
	death_knight_t* p = player -> cast_death_knight();
	for( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if( use[i] ) p -> _runes.slot[i].refund();
}

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

void
death_knight_attack_t::parse_options( option_t* options, const std::string& options_str )
{
  option_t base_options[] =
  {
    { "once", OPT_BOOL, &execute_once },
    { NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

void
death_knight_attack_t::reset()
{
  for( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  executed_once = false;
  action_t::reset();
}

void
death_knight_attack_t::consume_resource()
{
	if (cost() > 0) attack_t::consume_resource();
	consume_runes( player, use );
}

void
death_knight_attack_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::execute();
  
  if( result_is_hit() )
  {
    executed_once = true;

		double gain = -cost();
		if( gain > 0 ) p -> resource_gain( resource, gain, p -> gains_rune_abilities );

    if( p -> _buffs.scent_of_blood_stack && p->_cooldowns.scent_of_blood <= p -> sim -> current_time )
    {
      p -> _buffs.scent_of_blood_stack--;
      p -> resource_gain( resource, 5, p -> gains_scent_of_blood );
    }

    if( result == RESULT_CRIT )
    {
      // EMPTY
    }
  }
  else
  {
    if( cost() > 0 ) execute_once = true;
		refund_runes( p, use );
  }
}

void
death_knight_attack_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::player_buff();

  if( weapon )
  {
    // EMPTY
  }
}

bool
death_knight_attack_t::ready()
{
  death_knight_t* p = player -> cast_death_knight();
  p -> abort_execute_action = false;

  if( execute_once && executed_once )
    return false;

  if( execute_once && ! executed_once )
    p -> abort_execute_action = true;

	if( ! attack_t::ready() )
		return false;

	if( requires_weapon )
		if( ! weapon || weapon->group() == WEAPON_RANGED ) 
			return false;

	return group_runes( p, cost_blood, cost_frost, cost_unholy, use);
}

// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================

void
death_knight_spell_t::parse_options( option_t* options, const std::string& options_str )
{
  option_t base_options[] =
  {
    { "once", OPT_BOOL, &execute_once },
    { NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

void
death_knight_spell_t::reset()
{
  for( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  executed_once = false;
  action_t::reset();
}

void
death_knight_spell_t::consume_resource()
{
	if (cost() > 0) spell_t::consume_resource();
	consume_runes( player, use );
}

void
death_knight_spell_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::execute();
  
  if( result_is_hit() )
  {
    executed_once = true;

  	double gain = -cost();
		if( gain > 0 ) p -> resource_gain( resource, gain, p -> gains_rune_abilities );

    if( result == RESULT_CRIT )
    {
      // EMPTY
    }
  }
  else
  {
    if( cost() > 0 ) execute_once = true;
		refund_runes(p, use);
  }
}

void
death_knight_spell_t::player_buff()
{
	spell_t::player_buff();

  player_power     = player -> composite_attack_power();
  power_multiplier = player -> composite_attack_power_multiplier();

	if( sim -> debug ) report_t::log( sim, "death_knight_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f, p_mult=%.0f", 
				    name(), player_hit, player_crit, player_power, player_penetration, power_multiplier );

}

bool
death_knight_spell_t::ready()
{
  death_knight_t* p = player -> cast_death_knight();
  p -> abort_execute_action = false;

  if( execute_once && executed_once )
    return false;

  if( execute_once && ! executed_once )
    p -> abort_execute_action = true;

	if( ! spell_t::ready() )
		return false;

	return group_runes( player, cost_blood, cost_frost, cost_unholy, use);
}

double
death_knight_spell_t::total_crit_bonus()
{
	return spell_t::total_crit_bonus() + 0.5;
}

// ==========================================================================
// Death Knight Auto Attack Classes and Methods
// ==========================================================================

struct melee_t : public death_knight_attack_t
{
  melee_t( const char* name, player_t* player ) : 
    death_knight_attack_t( name, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    execute_once = false;

    base_direct_dmg = 1;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;

    if( p -> dual_wield() ) base_hit -= 0.19;
  }
};

struct auto_attack_t : public death_knight_attack_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) : 
    death_knight_attack_t( "auto_attack", player )
  {
    death_knight_t* p = player -> cast_death_knight();

    execute_once = false;

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

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    p -> main_hand_attack -> schedule_execute();
    if( p -> off_hand_attack ) p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    return p -> main_hand_attack -> execute_event == 0;
  }
};


// ==========================================================================
// Death Knight Diseases
// ==========================================================================

struct death_knight_disease_t : public death_knight_spell_t
{
    death_knight_disease_t( const char* name, player_t* player, int s, int t) :
			death_knight_spell_t( name, player, s, t)
		{
			death_knight_t* p = player -> cast_death_knight();

      execute_once = false;
	      
			static rank_t ranks[] =
			{
				{ 1, 1, 0, 0, 0, 0 },
				{ 0, 0 }
			};
			init_rank( ranks );
	    
			base_execute_time = 0;
			may_crit          = false;
			may_miss          = false;
			cooldown          = 0.0;
		
			num_ticks         = 5;
			base_tick_time    = 3.0;
			tick_power_mod    = .055;
		}

  virtual bool ready()     { return false; }
  virtual void execute()   { player -> cast_death_knight() -> diseases++; death_knight_spell_t::execute();   }
	virtual void last_tick() { player -> cast_death_knight() -> diseases--;	death_knight_spell_t::last_tick(); }
};

struct blood_plague_t : public death_knight_disease_t
{
		blood_plague_t( player_t* player ) :
			death_knight_disease_t( "blood_plague", player, SCHOOL_SHADOW, TREE_UNHOLY )
		{
			observer = &( player -> cast_death_knight() -> active_blood_plague );
		}
};

struct frost_fever_t : public death_knight_disease_t
{
		frost_fever_t( player_t* player ) :
			death_knight_disease_t( "frost_fever", player, SCHOOL_FROST, TREE_FROST )
		{
			observer = &( player -> cast_death_knight() -> active_frost_fever );
		}
};

// ==========================================================================
// Death Knight Rune Abilities
// ==========================================================================

struct blood_strike_t : public death_knight_attack_t
{
  blood_strike_t( player_t* player, const std::string& options_str  ) : 
    death_knight_attack_t( "blood_strike", player, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80,  6, 305.6, 305.6, 0, -10 },
      { 74,  5, 250,   250,   0, -10 },
      { 69,  4, 164.4, 164.4, 0, -10 },
      { 64,  3, 138.8, 138.8, 0, -10 },
      { 59,  2, 118,   118,   0, -10 },
      { 55,  1, 104,   104,   0, -10 },
      { 0, 0 }
    };
    init_rank( ranks );

		cost_blood = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.4;

		base_crit += p -> talents.subversion * 0.03;
  }

  void
  target_debuff( int dmg_type )
  {
		death_knight_t* p = player -> cast_death_knight();
	  death_knight_attack_t::target_debuff( dmg_type);
		target_multiplier *= 1 + p -> diseases * 0.125;
		assert (p -> diseases <= 2);
  }
};

struct death_strike_t : public death_knight_attack_t
{
  death_strike_t( player_t* player, const std::string& options_str  ) : 
    death_knight_attack_t( "death_strike", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80,  5, 222.75, 222.75, 0, -15 },
      { 75,  4, 150,    150,    0, -15 },
      { 70,  3, 123.75, 123.75, 0, -15 },
      { 63,  2, 97.5,   97.5,   0, -15 },
      { 56,  1, 84,     84,     0, -15 },
      { 0, 0 }
    };
    init_rank( ranks );

		cost_frost = 1;
		cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.75;
  }
};

struct heart_strike_t : public death_knight_attack_t
{
  heart_strike_t( player_t* player, const std::string& options_str  ) : 
    death_knight_attack_t( "heart_strike", player, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();
		assert( p -> talents.heart_strike == 1 );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80,  6, 368,   368,   0, -10 },
      { 74,  5, 300.5, 300.5, 0, -10 },
      { 69,  4, 197.5, 197.5, 0, -10 },
      { 64,  3, 167,   167,   0, -10 },
      { 59,  2, 142,   142,   0, -10 },
      { 55,  1, 125,   125,   0, -10 },
      { 0, 0 }
    };
    init_rank( ranks );

		cost_blood = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.5;

		base_crit += p -> talents.subversion * 0.03;
  }

  void
  target_debuff( int dmg_type )
  {
		death_knight_t* p = player -> cast_death_knight();
	  death_knight_attack_t::target_debuff( dmg_type);
		target_multiplier *= 1 + p -> diseases * 0.1;
		assert (p -> diseases <= 2);
  }
};

struct icy_touch_t : public death_knight_spell_t
{
		icy_touch_t(player_t* player, const std::string& options_str) :
			death_knight_spell_t( "icy_touch", player, SCHOOL_FROST, TREE_FROST)
		{
			option_t options[] =
			{
				{ NULL }
			};
			parse_options( options, options_str );
	      
			static rank_t ranks[] =
			{
				{ 78, 5, 227, 245, 0, -10 },
				{ 73, 4, 187, 203, 0, -10 },
				{ 67, 3, 161, 173, 0, -10 },
				{ 61, 2, 144, 156, 0, -10 },
				{ 55, 1, 127, 137, 0, -10 },
				{ 0, 0 }
			};
			init_rank( ranks );

			cost_frost = 1;
	    
			base_execute_time = 0;
			may_crit          = true;
			direct_power_mod  = 0.1;
			cooldown          = 0.0;

			player -> cast_death_knight() -> frost_fever = new frost_fever_t(player);
		}

	void
	execute()
		{
			death_knight_spell_t::execute();
			if (result_is_hit()) player -> cast_death_knight() -> frost_fever -> execute();
		}
};

struct obliterate_t : public death_knight_attack_t
{
  obliterate_t( player_t* player, const std::string& options_str  ) : 
    death_knight_attack_t( "obliterate", player, SCHOOL_PHYSICAL, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79,  4, 467.2, 467.2, 0, -15 },
      { 73,  3, 381.6, 381.6, 0, -15 },
      { 67,  2, 244,   244,   0, -15 },
      { 61,  1, 198.4, 198.4, 0, -15 },
      { 0, 0 }
    };
    init_rank( ranks );

		cost_frost = 1;
		cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.8;

		base_crit += p -> talents.subversion * 0.03;
  }

  void
  target_debuff( int dmg_type )
  {
		death_knight_t* p = player -> cast_death_knight();
	  death_knight_attack_t::target_debuff( dmg_type);
		target_multiplier *= 1 + p -> diseases * 0.125;
		assert( p -> diseases >= 0 );
		assert( p -> diseases <= 2 );
  }

  void
	execute()
	{
		death_knight_t* p = player -> cast_death_knight();

		death_knight_attack_t::execute();
		if (result_is_hit()) 
		{
      int before = p -> diseases;

			if ( p -> active_blood_plague && p -> active_blood_plague -> ticking )
        p -> active_blood_plague -> cancel();

			if ( p -> active_frost_fever && p -> active_frost_fever -> ticking )
				p -> active_frost_fever -> cancel();

      assert( p -> diseases == 0 );
		}
	}
};

struct plague_strike_t : public death_knight_attack_t
{
  plague_strike_t( player_t* player, const std::string& options_str  ) : 
    death_knight_attack_t( "plague_strike", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80,  6, 189,  189,  0, -10 },
      { 75,  5, 157,  157,  0, -10 },
      { 70,  4, 108,  108,  0, -10 },
      { 65,  3, 89,   89,   0, -10 },
      { 60,  2, 75.5, 75.5, 0, -10 },
      { 55,  1, 62.5, 62.5, 0, -10 },
      { 0, 0 }
    };
    init_rank( ranks );

	  cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.5;

		p -> blood_plague = new blood_plague_t(player);
  }

  void
	execute()
		{
			death_knight_attack_t::execute();
			if (result_is_hit()) player -> cast_death_knight() -> blood_plague -> execute();
		}
};

struct scourge_strike_t : public death_knight_attack_t
{
  scourge_strike_t( player_t* player, const std::string& options_str  ) : 
    death_knight_attack_t( "scourge_strike", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79,  4, 357.188, 357.188, 0, -15 },
      { 73,  3, 291.375, 291.375, 0, -15 },
      { 67,  2, 186.75,  186.75,  0, -15 },
      { 55,  1, 151.875, 151.875, 0, -15 },
      { 0, 0 }
    };
    init_rank( ranks );

		cost_frost = 1;
		cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.45;
  }

  void
  target_debuff( int dmg_type )
  {
		death_knight_t* p = player -> cast_death_knight();
	  death_knight_attack_t::target_debuff( dmg_type);
		target_multiplier *= 1 + p -> diseases * 0.11;
		assert (p -> diseases <= 2);
  }
};

// ==========================================================================
// Death Knight Runic Power Abilities
// ==========================================================================

struct death_coil_t : public death_knight_spell_t
{
		death_coil_t(player_t* player, const std::string& options_str) :
			death_knight_spell_t( "death_coil", player, SCHOOL_SHADOW, TREE_BLOOD)
		{
			option_t options[] =
			{
				{ NULL }
			};
			parse_options( options, options_str );
	      
			static rank_t ranks[] =
			{
				{ 80, 5, 443, 443, 0, 40 },
				{ 76, 4, 381, 381, 0, 40 },
				{ 68, 3, 275, 275, 0, 40 },
				{ 62, 2, 208, 208, 0, 40 },
				{ 55, 1, 184, 184, 0, 40 },
				{ 0, 0 }
			};
			init_rank( ranks );
	    
			base_execute_time = 0;
			may_crit          = true;
			direct_power_mod  = 0.15;
			cooldown          = 0.0;
		}
};

struct frost_strike_t : public death_knight_attack_t
{
  frost_strike_t( player_t* player, const std::string& options_str  ) : 
    death_knight_attack_t( "frost_strike", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80,  6, 150,   150,   0, 40 },
      { 75,  5, 120.6, 120.6, 0, 40 },
      { 70,  4, 85.2,  85.2,  0, 40 },
      { 65,  3, 69,    69,    0, 40 },
      { 60,  2, 61.8,  61.8,  0, 40 },
      { 55,  1, 52.2,  52.2,  0, 40 },
      { 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.6;
  }
};

}	// namespace

// ==========================================================================
// Death Knight Custom "clear" action to reset executed flags
// ==========================================================================

struct clear_t : public action_t
{
  clear_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "clear", player )
  {
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual void execute() 
  {
		death_knight_t* p = player -> cast_death_knight();
    for ( action_t* a = p -> action_list; a != this; a = a -> next )
    {
      death_knight_attack_t* att = dynamic_cast<death_knight_attack_t*>(a);
      if( att )
      {
        att ->executed_once = false;
        continue;
      }

      death_knight_spell_t* spl = dynamic_cast<death_knight_spell_t*>(a);
      if( spl )
      {
        spl ->executed_once = false;
        continue;
      }
    }
  }
};


// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

action_t*
death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  if( name == "auto_attack"         ) return new auto_attack_t        ( this, options_str );
	if( name == "clear"               ) return new clear_t              ( this, options_str );

  if( name == "blood_strike"        ) return new blood_strike_t       ( this, options_str );
	if( name == "death_strike"        ) return new death_strike_t       ( this, options_str );
	if( name == "heart_strike"        ) return new heart_strike_t       ( this, options_str );
	if( name == "icy_touch"           ) return new icy_touch_t          ( this, options_str );
	if( name == "obliterate"          ) return new obliterate_t         ( this, options_str );
	if( name == "plague_strike"       ) return new plague_strike_t      ( this, options_str );
	if( name == "scourge_strike"      ) return new scourge_strike_t     ( this, options_str );

  if( name == "death_coil"          ) return new death_coil_t         ( this, options_str );
	if( name == "frost_strike"        ) return new frost_strike_t       ( this, options_str );

  return player_t::create_action( name, options_str );
}

void
death_knight_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = 185;	// Human Level 80 Stats
  attribute_base[ ATTR_AGILITY   ] = 112;
  attribute_base[ ATTR_STAMINA   ] = 169;
  attribute_base[ ATTR_INTELLECT ] = 35;
  attribute_base[ ATTR_SPIRIT    ] = 60;

  base_attack_power = -20;
  initial_attack_power_per_strength = 2.0;

  base_attack_crit = 0.0318669;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 1 / 2000, 1 / 3200, 1 / 6244.965 );

  base_attack_expertise = 0.0025 * 0;

  resource_base[ RESOURCE_HEALTH ] = rating_t::interpolate( level, 8121, 8121, 8121 );
  resource_base[ RESOURCE_RUNIC ]  = 100 + talents.runic_power_mastery * 15;

  health_per_stamina = 10;

  base_gcd = 1.5;
}

action_t*
death_knight_t::execute_action()
{
  action_t* action=0;

  abort_execute_action = false;

  for( action = action_list; action; action = action -> next )
  {
    if( action -> background )
      continue;

    if( action -> ready() )
      break;

    if( abort_execute_action )
    {
      action = NULL;
      break;
    }
  }

  if( action ) 
  {
    action -> schedule_execute();
  }

  last_foreground_action = action;

  return action;
}

void
death_knight_t::init_resources(bool force)
{
	player_t::init_resources(force);
	resource_current[ RESOURCE_RUNIC ] = 0;
}

void
death_knight_t::combat_begin() 
{
	player_t::combat_begin();

	if( talents.scent_of_blood && scent_of_blood_interval > 0 )
	{
		struct scent_of_blood_proc_t : public event_t
		{
			scent_of_blood_proc_t( sim_t* sim, death_knight_t* p, double interval ) : event_t( sim, p )
			{
				name = "Scent of Blood Proc";
				sim -> add_event( this, interval );
			}
			virtual void execute()
			{
				death_knight_t* p = player -> cast_death_knight();

        if( p -> _cooldowns.scent_of_blood <= sim -> current_time && sim -> roll( 0.15 ) )
        {
          p -> _cooldowns.scent_of_blood = sim -> current_time + 10;
          p -> _buffs.scent_of_blood_stack = p -> talents.scent_of_blood;
  				p -> procs_scent_of_blood -> occur();
        }
				new ( sim ) scent_of_blood_proc_t( sim, p, p -> scent_of_blood_interval );
			}
		};

		new ( sim ) scent_of_blood_proc_t( sim, this, scent_of_blood_interval );
	}
}

double
death_knight_t::composite_attack_power()
{
  return player_t::composite_attack_power() + talents.bladed_armor * 0 /* Replace ZERO with this->armor() when implemented */; 
}

void
death_knight_t::regen( double periodicity )
{
  _buffs.butchery_timer += periodicity;
  if( _buffs.butchery_timer >= 5.0 )
  {
    _buffs.butchery_timer -= 5;
    resource_gain( RESOURCE_RUNIC, talents.butchery, cast_death_knight() -> gains_butchery );
  }
  player_t::regen(periodicity);
}

void
death_knight_t::reset()
{
  player_t::reset();

  // Active
  active_blood_plague = NULL;
	active_frost_fever  = NULL;
	diseases            = 0;

  _buffs.reset();
  _cooldowns.reset();
  _expirations.reset();
  _runes.reset();
}

double
death_knight_t::resource_gain( int resource, double  amount, gain_t* source )
{
  if( sleeping || amount <= 0 ) return 0;

  double actual_amount = player_t::resource_gain( resource, amount, source );

  if( actual_amount < amount && resource == RESOURCE_RUNIC )
    gains_runic_overflow -> add( amount - actual_amount );

  return actual_amount;
}

bool
death_knight_t::get_talent_trees( std::vector<int*>& blood, std::vector<int*>& frost, std::vector<int*>& unholy )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.butchery                 ) }, {  1, &( talents.improved_icy_touch      ) }, {  1, &( talents.vicious_strikes          ) } },
    { {  2, &( talents.subversion               ) }, {  2, &( talents.runic_power_mastery     ) }, {  2, &( talents.virulence                ) } },
    { {  3, NULL                                  }, {  3, &( talents.toughness               ) }, {  3, NULL                                  } },
    { {  4, &( talents.bladed_armor             ) }, {  4, NULL                                 }, {  4, &( talents.epidemic                 ) } },
  	{ {  5, &( talents.scent_of_blood           ) }, {  5, &( talents.black_ice               ) }, {  5, &( talents.morbidity                ) } },
    { {  6, &( talents.two_handed_weapon_spec   ) }, {  6, &( talents.nerves_of_cold_steel    ) }, {  6, NULL                                  } },
    { {  7, &( talents.rune_tap                 ) }, {  7, &( talents.icy_talons              ) }, {  7, &( talents.ravenous_dead            ) } },
    { {  8, &( talents.dark_conviction          ) }, {  8, NULL                                 }, {  8, &( talents.outbreak                 ) } },
    { {  9, &( talents.death_rune_mastery       ) }, {  9, &( talents.annihilation            ) }, {  9, &( talents.necrosis                 ) } },
    { { 10, &( talents.improved_rune_tap        ) }, { 10, &( talents.killing_machine         ) }, { 10, &( talents.corpse_explosion         ) } },
    { { 11, NULL                                  }, { 11, &( talents.chill_of_the_grave      ) }, { 11, NULL                                  } },
    { { 12, NULL                                  }, { 12, &( talents.endless_winter          ) }, { 12, &( talents.blood_caked_blade        ) } },
    { { 13, &( talents.bloody_strikes           ) }, { 13, NULL                                 }, { 13, &( talents.master_of_ghouls         ) } },
    { { 14, &( talents.veteran_of_the_third_war ) }, { 14, &( talents.glacier_rot             ) }, { 14, &( talents.unholy_blight            ) } },
    { { 15, NULL                                  }, { 15, &( talents.deathchill              ) }, { 15, &( talents.impurity                 ) } },
    { { 16, &( talents.bloody_vengeance         ) }, { 16, &( talents.improved_icy_talons     ) }, { 16, &( talents.dirge                    ) } },
    { { 17, &( talents.abominations_might       ) }, { 17, &( talents.merciless_combat        ) }, { 17, NULL                                  } },
    { { 18, &( talents.bloodworms               ) }, { 18, &( talents.rime                    ) }, { 18, &( talents.reaping                  ) } },
    { { 19, &( talents.hysteria                 ) }, { 19, &( talents.chilblains              ) }, { 19, &( talents.ghoul_frenzy             ) } },
    { { 20, &( talents.improved_blood_presence  ) }, { 20, &( talents.hungering_cold          ) }, { 20, &( talents.desecration              ) } },
    { { 21, &( talents.improved_death_strike    ) }, { 21, &( talents.improved_frost_presence ) }, { 21, NULL                                  } },
    { { 22, &( talents.sudden_doom              ) }, { 22, &( talents.blood_of_the_north      ) }, { 22, &( talents.improved_unholy_presence ) } },
    { { 23, &( talents.vampiric_blood           ) }, { 23, &( talents.unbreakable_armor       ) }, { 23, &( talents.night_of_the_dead        ) } },
    { { 24, NULL                                  }, { 24, NULL                                 }, { 24, &( talents.crypt_fever              ) } },
    { { 25, &( talents.heart_strike             ) }, { 25, &( talents.frost_strike            ) }, { 25, &( talents.bone_shield              ) } },
    { { 26, &( talents.might_of_mograine        ) }, { 26, &( talents.guile_of_gorefiend      ) }, { 26, &( talents.wandering_plague         ) } },
    { { 27, &( talents.blood_gorged             ) }, { 27, &( talents.tundra_stalker          ) }, { 27, &( talents.ebon_plaguebringer       ) } },
    { { 28, &( talents.dancing_rune_weapon      ) }, { 28, &( talents.howling_blast           ) }, { 28, &( talents.scourge_strike           ) } },
    { {  0, NULL                                  }, {  0, NULL                                 }, { 29, &( talents.rage_of_rivendare        ) } },
    { {  0, NULL                                  }, {  0, NULL                                 }, { 30, &( talents.summon_gargoyle          ) } },
    { {  0, NULL                                  }, {  0, NULL                                 }, {  0, NULL                                  } }
  };
  
  return player_t::get_talent_trees( blood, frost, unholy, translation );
}

bool
death_knight_t::parse_option( const std::string& name, const std::string& value )
{
  option_t options[] =
  {
    // Talents
	  { "butchery",                 OPT_INT, &( talents.butchery                 ) },
	  { "subversion",               OPT_INT, &( talents.subversion               ) },
	  { "bladed_armor",             OPT_INT, &( talents.bladed_armor             ) },
	  { "scent_of_blood",           OPT_INT, &( talents.scent_of_blood           ) },
	  { "two_handed_weapon_spec",   OPT_INT, &( talents.two_handed_weapon_spec   ) },
	  { "rune_tap",                 OPT_INT, &( talents.rune_tap                 ) },
	  { "dark_conviction",          OPT_INT, &( talents.dark_conviction          ) },
	  { "death_rune_mastery",       OPT_INT, &( talents.death_rune_mastery       ) },
	  { "improved_rune_tap",        OPT_INT, &( talents.improved_rune_tap        ) },
	  { "bloody_strikes",           OPT_INT, &( talents.bloody_strikes           ) },
	  { "veteran_of_the_third_war", OPT_INT, &( talents.veteran_of_the_third_war ) },
	  { "bloody_vengeance",         OPT_INT, &( talents.bloody_vengeance         ) },
	  { "abominations_might",       OPT_INT, &( talents.abominations_might       ) },
	  { "bloodworms",               OPT_INT, &( talents.bloodworms               ) },
	  { "hysteria",                 OPT_INT, &( talents.hysteria                 ) },
	  { "improved_blood_presence",  OPT_INT, &( talents.improved_blood_presence  ) },
	  { "improved_death_strike",    OPT_INT, &( talents.improved_death_strike    ) },
	  { "sudden_doom",              OPT_INT, &( talents.sudden_doom              ) },
	  { "vampiric_blood",           OPT_INT, &( talents.vampiric_blood           ) },
	  { "heart_strike",             OPT_INT, &( talents.heart_strike             ) },
	  { "might_of_mograine",        OPT_INT, &( talents.might_of_mograine        ) },
	  { "blood_gorged",             OPT_INT, &( talents.blood_gorged             ) },
	  { "dancing_rune_weapon",      OPT_INT, &( talents.dancing_rune_weapon      ) },
	  { "improved_icy_touch",       OPT_INT, &( talents.improved_icy_touch       ) },
	  { "runic_power_mastery",      OPT_INT, &( talents.runic_power_mastery      ) },
	  { "toughness",                OPT_INT, &( talents.toughness                ) },
	  { "black_ice",                OPT_INT, &( talents.black_ice                ) },
	  { "nerves_of_cold_steel",     OPT_INT, &( talents.nerves_of_cold_steel     ) },
	  { "icy_talons",               OPT_INT, &( talents.icy_talons               ) },
	  { "annihilation",             OPT_INT, &( talents.annihilation             ) },
	  { "killing_machine",          OPT_INT, &( talents.killing_machine          ) },
	  { "chill_of_the_grave",       OPT_INT, &( talents.chill_of_the_grave       ) },
	  { "endless_winter",           OPT_INT, &( talents.endless_winter           ) },
	  { "glacier_rot",              OPT_INT, &( talents.glacier_rot              ) },
	  { "deathchill",               OPT_INT, &( talents.deathchill               ) },
	  { "improved_icy_talons",      OPT_INT, &( talents.improved_icy_talons      ) },
	  { "merciless_combat",         OPT_INT, &( talents.merciless_combat         ) },
	  { "rime",                     OPT_INT, &( talents.rime                     ) },
	  { "chilblains",               OPT_INT, &( talents.chilblains               ) },
	  { "hungering_cold",           OPT_INT, &( talents.hungering_cold           ) },
	  { "improved_frost_presence",  OPT_INT, &( talents.improved_frost_presence  ) },
	  { "blood_of_the_north",       OPT_INT, &( talents.blood_of_the_north       ) },
	  { "unbreakable_armor",        OPT_INT, &( talents.unbreakable_armor        ) },
	  { "frost_strike",             OPT_INT, &( talents.frost_strike             ) },
	  { "guile_of_gorefiend",       OPT_INT, &( talents.guile_of_gorefiend       ) },
	  { "tundra_stalker",           OPT_INT, &( talents.tundra_stalker           ) },
	  { "howling_blast",            OPT_INT, &( talents.howling_blast            ) },
	  { "vicious_strikes",          OPT_INT, &( talents.vicious_strikes          ) },
	  { "virulence",                OPT_INT, &( talents.virulence                ) },
	  { "epidemic",                 OPT_INT, &( talents.epidemic                 ) },
	  { "morbidity",                OPT_INT, &( talents.morbidity                ) },
	  { "ravenous_dead",            OPT_INT, &( talents.ravenous_dead            ) },
	  { "outbreak",                 OPT_INT, &( talents.outbreak                 ) },
	  { "necrosis",                 OPT_INT, &( talents.necrosis                 ) },
	  { "corpse_explosion",         OPT_INT, &( talents.corpse_explosion         ) },
	  { "blood_caked_blade",        OPT_INT, &( talents.blood_caked_blade        ) },
	  { "master_of_ghouls",         OPT_INT, &( talents.master_of_ghouls         ) },
	  { "unholy_blight",            OPT_INT, &( talents.unholy_blight            ) },
	  { "impurity",                 OPT_INT, &( talents.impurity                 ) },
	  { "dirge",                    OPT_INT, &( talents.dirge                    ) },
	  { "reaping",                  OPT_INT, &( talents.reaping                  ) },
	  { "ghoul_frenzy",             OPT_INT, &( talents.ghoul_frenzy             ) },
	  { "desecration",              OPT_INT, &( talents.desecration              ) },
	  { "improved_unholy_presence", OPT_INT, &( talents.improved_unholy_presence ) },
	  { "night_of_the_dead",        OPT_INT, &( talents.night_of_the_dead        ) },
	  { "crypt_fever",              OPT_INT, &( talents.crypt_fever              ) },
	  { "bone_shield",              OPT_INT, &( talents.bone_shield              ) },
	  { "wandering_plague",         OPT_INT, &( talents.wandering_plague         ) },
	  { "ebon_plaguebringer",       OPT_INT, &( talents.ebon_plaguebringer       ) },
	  { "ebon_plaguebringer",       OPT_INT, &( talents.ebon_plaguebringer       ) },
	  { "rage_of_rivendare",        OPT_INT, &( talents.rage_of_rivendare        ) },
	  { "summon_gargoyle",          OPT_INT, &( talents.summon_gargoyle          ) },

    // Glyphs
	  { "glyph_blood_tap",           OPT_INT, &( glyphs.blood_tap           ) },
	  { "glyph_horn_of_winter",      OPT_INT, &( glyphs.horn_of_winter      ) },
	  { "glyph_heart_strike",        OPT_INT, &( glyphs.heart_strike        ) },
	  { "glyph_blood_strike",        OPT_INT, &( glyphs.blood_strike        ) },
	  { "glyph_chains_of_ice",       OPT_INT, &( glyphs.chains_of_ice       ) },
	  { "glyph_dancing_rune_weapon", OPT_INT, &( glyphs.dancing_rune_weapon ) },
	  { "glyph_dark_death",          OPT_INT, &( glyphs.dark_death          ) },
	  { "glyph_death_strike",        OPT_INT, &( glyphs.death_strike        ) },
	  { "glyph_death_and_decay",     OPT_INT, &( glyphs.death_and_decay     ) },
	  { "glyph_disease",             OPT_INT, &( glyphs.disease             ) },
	  { "glyph_frost_strike",        OPT_INT, &( glyphs.frost_strike        ) },
	  { "glyph_howling_blast",       OPT_INT, &( glyphs.howling_blast       ) },
	  { "glyph_hungering_cold",      OPT_INT, &( glyphs.hungering_cold      ) },
	  { "glyph_icy_touch",           OPT_INT, &( glyphs.icy_touch           ) },
	  { "glyph_obliterate",          OPT_INT, &( glyphs.obliterate          ) },
	  { "glyph_plague_strike",       OPT_INT, &( glyphs.plague_strike       ) },
	  { "glyph_rune_strike",         OPT_INT, &( glyphs.rune_strike         ) },
	  { "glyph_rune_tap",            OPT_INT, &( glyphs.rune_tap            ) },
	  { "glyph_scourge_strike",      OPT_INT, &( glyphs.scourge_strike      ) },
	  { "glyph_strangulate",         OPT_INT, &( glyphs.strangulate         ) },
	  { "glyph_unholy_blight",       OPT_INT, &( glyphs.unholy_blight       ) },
	  { "glyph_vampiric_blood",      OPT_INT, &( glyphs.vampiric_blood      ) },
	  { "glyph_the_ghoul",           OPT_INT, &( glyphs.the_ghoul           ) },

    // Options
    { "scent_of_blood_interval",   OPT_INT, &( scent_of_blood_interval    ) },
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

// player_t implementations ============================================

player_t*
player_t::create_death_knight( sim_t* sim, std::string& name ) 
{
  return new death_knight_t( sim, name );
}
