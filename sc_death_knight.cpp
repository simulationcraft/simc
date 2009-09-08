// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

struct bloodworm_pet_t;

// ==========================================================================
// Death Knight Runes
// ==========================================================================

enum rune_type
{
  RUNE_TYPE_NONE=0, RUNE_TYPE_BLOOD, RUNE_TYPE_FROST, RUNE_TYPE_UNHOLY, RUNE_TYPE_DEATH, RUNE_TYPE_WASDEATH=8
};

#define RUNE_TYPE_MASK     3
#define RUNE_SLOT_MAX      6

#define RUNE_GRACE_PERIOD   2.0
#define RUNIC_POWER_REFUND  0.9

#define GET_BLOOD_RUNE_COUNT(x)  ((x) & 0x03)
#define GET_FROST_RUNE_COUNT(x)  (((x) >> 3) & 0x03)
#define GET_UNHOLY_RUNE_COUNT(x) (((x) >> 6) & 0x03)

struct dk_rune_t
{
  double cooldown_ready;
  int    type;

  dk_rune_t() : cooldown_ready( 0 ), type( RUNE_TYPE_NONE ) {}

  bool is_death(                     ) SC_CONST { return ( type & RUNE_TYPE_DEATH ) != 0;  }
  bool is_ready( double current_time ) SC_CONST { return cooldown_ready <= current_time; }
  int  get_type(                     ) SC_CONST { return type & RUNE_TYPE_MASK;          }

  void consume( double current_time, double cooldown, bool convert )
  {
    assert ( current_time >= cooldown_ready );

    cooldown_ready = ( current_time <= ( cooldown_ready + RUNE_GRACE_PERIOD ) ? cooldown_ready : current_time ) + cooldown;
    type = ( type & RUNE_TYPE_MASK ) | ( ( type << 1 ) & RUNE_TYPE_WASDEATH ) | ( convert ? RUNE_TYPE_DEATH : 0 ) ;
  }

  void refund()   { cooldown_ready = 0; type  = ( type & RUNE_TYPE_MASK ) | ( ( type >> 1 ) & RUNE_TYPE_DEATH ); }
  void reset()    { cooldown_ready = -RUNE_GRACE_PERIOD - 1; type  = type & RUNE_TYPE_MASK;                               }
};

// ==========================================================================
// Death Knight
// ==========================================================================

enum death_knight_presence { PRESENCE_BLOOD=1, PRESENCE_FROST, PRESENCE_UNHOLY=4 };

struct death_knight_t : public player_t
{
  // Active
  int       active_presence;
  action_t* active_blood_plague;
  action_t* active_frost_fever;
  int       diseases;

  // Pets and Guardians
  bloodworm_pet_t*  active_bloodworms;

  // Buffs
  buff_t* buffs_bloodworms;
  buff_t* buffs_bloody_vengeance;
  buff_t* buffs_scent_of_blood;

  // Gains
  gain_t* gains_rune_abilities;
  gain_t* gains_butchery;
  gain_t* gains_power_refund;
  gain_t* gains_scent_of_blood;

  // Procs
  proc_t* procs_abominations_might;
  proc_t* procs_sudden_doom;

  // Up-Times
  uptime_t* uptimes_blood_plague;
  uptime_t* uptimes_frost_fever;

  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  // Diseases
  spell_t* blood_plague;
  spell_t* frost_fever;

  // Special
  spell_t* sudden_doom;

  // Options
  std::string hysteria_target_str;

  // Talents
  struct talents_t
  {
    int butchery;
    int subversion;
    int blade_barrier;
    int bladed_armor;
    int scent_of_blood;
    int two_handed_weapon_specialization;
    int rune_tap;
    int dark_conviction;
    int death_rune_mastery;
    int improved_rune_tap;
    int spell_reflection;
    int vendetta;
    int bloody_strikes;
    int veteran_of_the_third_war;
    int mark_of_blood;
    int bloody_vengeance;
    int abominations_might;
    int bloodworms;
    int hysteria;
    int improved_blood_presence;
    int improved_death_strike;
    int sudden_doom;
    int vampiric_blood;
    int will_of_the_necropolis;
    int heart_strike;
    int might_of_mograine;
    int blood_gorged;
    int dancing_rune_weapon;

    int improved_icy_touch;
    int runic_power_mastery;
    int toughness;
    int icy_reach;
    int black_ice;
    int nerves_of_cold_steel;
    int icy_talons;
    int lichborne;
    int annihilation;
    int killing_machine;
    int chill_of_the_grave;
    int endless_winter;
    int frigid_deathplate;
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
    int acclimation;
    int frost_strike;
    int guile_of_gorefiend;
    int tundra_stalker;
    int howling_blast;

    int vicious_strikes;
    int virulence;
    int anticipation;
    int epidemic;
    int morbidity;
    int unholy_command;
    int ravenous_dead;
    int outbreak;
    int necrosis;
    int corpse_explosion;
    int on_a_pale_horse;
    int blood_caked_blade;
    int night_of_the_dead;
    int unholy_blight;
    int impurity;
    int dirge;
    int magic_supression;
    int reaping;
    int master_of_ghouls;
    int desecration;
    int anti_magic_zone;
    int improved_unholy_presence;
    int ghoul_frenzy;
    int crypt_fever;
    int bone_shield;
    int wandering_plague;
    int ebon_plaguebringer;
    int scourge_strike;
    int rage_of_rivendare;
    int summon_gargoyle;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  // Glyphs
  struct glyphs_t
  {
    int anti_magic_shell;
    int blood_strike;
    int blood_tap;
    int bone_shield;
    int chains_of_ice;
    int corpse_explosion;
    int dancing_rune_weapon;
    int dark_command;
    int dark_death;
    int death_and_decay;
    int death_grip;
    int death_strike;
    int deaths_embrace;
    int disease;
    int frost_strike;
    int heart_strike;
    int horn_of_winter;
    int howling_blast;
    int hungering_cold;
    int icebound_fortitude;
    int icy_touch;
    int obliterate;
    int pestilence;
    int plague_strike;
    int raise_dead;
    int rune_strike;
    int rune_tap;
    int scourge_strike;
    int strangulate;
    int the_ghoul;
    int unbreakable_armor;
    int unholy_blight;
    int vampiric_blood;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct runes_t
  {
    dk_rune_t slot[RUNE_SLOT_MAX];

    runes_t()    { for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].type = RUNE_TYPE_BLOOD + ( i >> 1 ); }
    void reset() { for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].reset();                           }
  };
  runes_t _runes;

  death_knight_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) :
      player_t( sim, DEATH_KNIGHT, name, race_type )
  {
    // Active
    active_presence     = 0;
    active_blood_plague = NULL;
    active_frost_fever  = NULL;
    diseases            = 0;

    // Pets and Guardians
    active_bloodworms   = NULL;

    // Auto-Attack
    main_hand_attack    = NULL;
    off_hand_attack     = NULL;

    sudden_doom         = NULL;
  }

  // Character Definition
  virtual void      init_actions();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_glyphs();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual double    composite_attack_power() SC_CONST;
  virtual void      regen( double periodicity );
  virtual void      reset();
  virtual void      target_swing();
  virtual void      combat_begin();
  virtual bool      get_talent_trees( std::vector<int*>& blood, std::vector<int*>& frost, std::vector<int*>& unholy );
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_RUNIC; }
  virtual int       primary_tree() SC_CONST;
};

// ==========================================================================
// Guardians
// ==========================================================================

struct bloodworm_pet_t : public pet_t
{
  int spawn_count;

  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
        attack_t( "bloodworm_melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      may_crit    = true;
      background  = true;
      repeating   = true;

      pet_t* p = player -> cast_pet();

      // Orc Command Racial
      if ( p -> owner -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }
    virtual void player_buff()
    {
      bloodworm_pet_t* p = ( bloodworm_pet_t* ) player;
      attack_t::player_buff();
      player_multiplier *= p -> spawn_count;
    }
  };

  melee_t* melee;

  bloodworm_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "bloodworms", true /*guardian*/ ), spawn_count( 3 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 20;
    main_hand_weapon.swing_time = 2.0;
  }
  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;

    melee = new melee_t( this );
  }
  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    o -> active_bloodworms = this;
    melee -> schedule_execute();
  }
  virtual void dismiss()
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::dismiss();
    o -> active_bloodworms = 0;
  }

  virtual int primary_resource() SC_CONST { return RESOURCE_MANA; }
};

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_attack_t : public attack_t
{
  bool   requires_weapon;
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];
  int    min_blood, max_blood, exact_blood;
  int    min_frost, max_frost, exact_frost;
  int    min_unholy, max_unholy, exact_unholy;
  int    min_death, max_death, exact_death;
  int    min_runic, max_runic, exact_runic;
  int    min_blood_plague, max_blood_plague, exact_blood_plague;
  int    min_frost_fever, max_frost_fever, exact_frost_fever;

  death_knight_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE ) :
      attack_t( n, player, RESOURCE_RUNIC, s, t ),
      requires_weapon( true ),
      cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
      min_blood( -1 ), max_blood( -1 ), exact_blood( -1 ),
      min_frost( -1 ), max_frost( -1 ), exact_frost( -1 ),
      min_unholy( -1 ), max_unholy( -1 ), exact_unholy( -1 ),
      min_death( -1 ), max_death( -1 ), exact_death( -1 ),
      min_runic( -1 ), max_runic( -1 ), exact_runic( -1 ),
      min_blood_plague( -1 ), max_blood_plague( -1 ), exact_blood_plague( -1 ),
      min_frost_fever( -1 ), max_frost_fever( -1 ), exact_frost_fever( -1 )
  {
    death_knight_t* p = player -> cast_death_knight();
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_crit   = true;
    may_glance = false;

    if ( p -> talents.two_handed_weapon_specialization )
      weapon_multiplier *= 1 + ( p -> talents.two_handed_weapon_specialization * 0.02 );

    if ( p -> talents.dark_conviction )
      base_crit += p -> talents.dark_conviction * 0.01;
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
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];
  int    min_blood, max_blood, exact_blood;
  int    min_frost, max_frost, exact_frost;
  int    min_unholy, max_unholy, exact_unholy;
  int    min_death, max_death, exact_death;
  int    min_runic, max_runic, exact_runic;
  int    min_blood_plague, max_blood_plague, exact_blood_plague;
  int    min_frost_fever, max_frost_fever, exact_frost_fever;

  death_knight_spell_t( const char* n, player_t* player, int s, int t ) :
      spell_t( n, player, RESOURCE_RUNIC, s, t ),
      cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( false ),
      min_blood( -1 ), max_blood( -1 ), exact_blood( -1 ),
      min_frost( -1 ), max_frost( -1 ), exact_frost( -1 ),
      min_unholy( -1 ), max_unholy( -1 ), exact_unholy( -1 ),
      min_death( -1 ), max_death( -1 ), exact_death( -1 ),
      min_runic( -1 ), max_runic( -1 ), exact_runic( -1 ),
      min_blood_plague( -1 ), max_blood_plague( -1 ), exact_blood_plague( -1 ),
      min_frost_fever( -1 ), max_frost_fever( -1 ), exact_frost_fever( -1 )
  {
    death_knight_t* p = player -> cast_death_knight();
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_crit = true;
    base_spell_power_multiplier = 0;
    base_attack_power_multiplier = 1;

    if ( p -> talents.dark_conviction )
      base_crit += p -> talents.dark_conviction * 0.01;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual void   reset();

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual bool   ready();
  virtual double total_crit_bonus() SC_CONST;
};

// ==========================================================================
// Local Utility Functions
// ==========================================================================
static int count_runes( player_t* player )
{
  death_knight_t* p = player -> cast_death_knight();
  double t = p -> sim -> current_time;
  int count = 0;

  // Storing ready runes by type in 2 bit blocks with 0 value bit separator (11 bits total)
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( p -> _runes.slot[i].is_ready( t ) )
      count |= 1 << ( i + ( i >> 1 ) );

  // Adding bits in each two-bit block (0xDB = 011 011 011)
  // NOTE: Use GET_XXX_RUNE_COUNT macros to process return value
  return ( ( count & ( count << 1 ) ) | ( count >> 1 ) ) & 0xDB;
}

static void consume_runes( player_t* player, const bool* use, bool convert_runes = false )
{
  death_knight_t* p = player -> cast_death_knight();
  double t = p -> sim -> current_time;

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( use[i] ) p -> _runes.slot[i].consume( t, 10.0, convert_runes );
}

static bool group_runes ( player_t* player, int blood, int frost, int unholy, bool* group )
{
  death_knight_t* p = player -> cast_death_knight();
  double t = p -> sim -> current_time;
  int cost[]  = { blood + frost + unholy, blood, frost, unholy };
  bool use[RUNE_SLOT_MAX] = { false };

  // Selecting available non-death runes to satisfy cost
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready( t ) && ! r.is_death() && cost[r.get_type()] > 0 )
    {
      --cost[r.get_type()];
      --cost[0];
      use[i] = true;
    }
  }

  // Selecting available death runes to satisfy remaining cost
  for ( int i = RUNE_SLOT_MAX; cost[0] > 0 && i--; )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready( t ) && r.is_death() )
    {
      --cost[0];
      use[i] = true;
    }
  }

  if ( cost[0] > 0 ) return false;

  // Storing rune slots selected
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) group[i] = use[i];

  return true;
}

static void refund_power( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( a -> resource_consumed > 0 )
    p -> resource_gain( RESOURCE_RUNIC, a -> resource_consumed * RUNIC_POWER_REFUND, p -> gains_power_refund );
}

static void refund_runes( player_t* player, const bool* use )
{
  death_knight_t* p = player -> cast_death_knight();
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( use[i] ) p -> _runes.slot[i].refund();
}

static void trigger_abominations_might( action_t* a, double base_chance )
{
  death_knight_t* dk = a -> player -> cast_death_knight();

  a -> sim -> auras.abominations_might -> trigger( 1, 1.0, base_chance * dk -> talents.abominations_might );
}

static void trigger_sudden_doom( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> sim -> roll( p -> talents.sudden_doom * 0.05 ) ) return;

  p -> procs_sudden_doom -> occur();
  // TODO: Intialize me
  //p -> sudden_doom -> execute();
}

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

void death_knight_attack_t::parse_options( option_t* options, const std::string& options_str )
{
  option_t base_options[] =
  {
    { "blood",         OPT_INT,  &exact_blood },
    { "blood>",        OPT_INT,  &min_blood },
    { "blood<",        OPT_INT,  &max_blood },
    { "frost",         OPT_INT,  &exact_frost },
    { "frost>",        OPT_INT,  &min_frost },
    { "frost<",        OPT_INT,  &max_frost },
    { "unholy",        OPT_INT,  &exact_unholy },
    { "unholy>",       OPT_INT,  &min_unholy },
    { "unholy<",       OPT_INT,  &max_unholy },
    { "death",         OPT_INT,  &exact_death },
    { "death>",        OPT_INT,  &min_death },
    { "death<",        OPT_INT,  &max_death },
    { "runic",         OPT_INT,  &exact_runic },
    { "runic>",        OPT_INT,  &min_runic },
    { "runic<",        OPT_INT,  &max_runic },
    { "blood_plague",  OPT_INT,  &exact_blood_plague },
    { "blood_plague>", OPT_INT,  &min_blood_plague },
    { "blood_plague<", OPT_INT,  &max_blood_plague },
    { "frost_fever",   OPT_INT,  &exact_frost_fever },
    { "frost_fever>",  OPT_INT,  &min_frost_fever },
    { "frost_fever<",  OPT_INT,  &max_frost_fever },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

void death_knight_attack_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  action_t::reset();
}

void death_knight_attack_t::consume_resource()
{
  if ( cost() > 0 ) attack_t::consume_resource();
  consume_runes( player, use, convert_runes == 0 ? false : sim->roll( convert_runes ) == 1 );
}

void death_knight_attack_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::execute();

  if ( result_is_hit() )
  {
    double gain = -cost();
    if ( gain > 0 ) p -> resource_gain( resource, gain, p -> gains_rune_abilities );

    p -> buffs_bloodworms -> trigger();

    if ( result == RESULT_CRIT )
    {
      p -> buffs_bloody_vengeance -> increment();
    }
  }
  else
  {
    refund_power( this );
    refund_runes( p, use );
  }
}

void death_knight_attack_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::player_buff();

  if ( p -> active_presence == PRESENCE_BLOOD )
  {
    player_multiplier *= 1.0 + 0.15;
  }

  if ( school == SCHOOL_PHYSICAL )
  {
    player_multiplier *= 1.0 + p -> talents.bloody_vengeance * 0.01 * p -> buffs_bloody_vengeance -> stack();
  }

  if ( p->talents.blood_gorged )
  {
    player_penetration += p -> talents.blood_gorged * 0.02;

    if ( p -> resource_current[ RESOURCE_HEALTH ] >= 
	 p -> resource_max    [ RESOURCE_HEALTH ] * 0.75 )
    {
      player_multiplier *= 1 + p -> talents.blood_gorged * 0.02;
    }
  }
}

bool death_knight_attack_t::ready()
{
  death_knight_t* p = player -> cast_death_knight();

  if ( ! attack_t::ready() )
    return false;

  if ( requires_weapon )
    if ( ! weapon || weapon->group() == WEAPON_RANGED )
      return false;

  if ( exact_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] != exact_runic )
      return false;

  if ( min_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] < min_runic )
      return false;

  if ( max_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] > max_runic )
      return false;

  double ct = sim -> current_time;

  if ( exact_blood_plague > 0 )
    if ( ! p -> active_blood_plague || ( ( p -> active_blood_plague -> duration_ready - ct ) != exact_blood_plague ) )
      return false;

  if ( min_blood_plague > 0 )
    if ( ! p -> active_blood_plague || ( ( p -> active_blood_plague -> duration_ready - ct ) < min_blood_plague ) )
      return false;

  if ( max_blood_plague > 0 )
    if ( p -> active_blood_plague && ( ( p -> active_blood_plague -> duration_ready - ct ) > max_blood_plague ) )
      return false;

  if ( exact_frost_fever > 0 )
    if ( ! p -> active_frost_fever || ( ( p -> active_frost_fever -> duration_ready - ct ) != exact_frost_fever ) )
      return false;

  if ( min_frost_fever > 0 )
    if ( ! p -> active_frost_fever || ( ( p -> active_frost_fever -> duration_ready - ct ) < min_frost_fever ) )
      return false;

  if ( max_frost_fever > 0 )
    if ( p -> active_frost_fever && ( ( p -> active_frost_fever -> duration_ready - ct ) > max_frost_fever ) )
      return false;

  int count = count_runes( p );
  int c = GET_BLOOD_RUNE_COUNT( count );
  if ( ( exact_blood != -1 && c != exact_blood ) ||
       ( min_blood != -1 && c < min_blood ) ||
       ( max_blood != -1 && c > max_blood ) ) return false;

  c = GET_FROST_RUNE_COUNT( count );
  if ( ( exact_frost != -1 && c != exact_frost ) ||
       ( min_frost != -1 && c < min_frost ) ||
       ( max_frost != -1 && c > max_frost ) ) return false;

  c = GET_UNHOLY_RUNE_COUNT( count );
  if ( ( exact_unholy != -1 && c != exact_unholy ) ||
       ( min_unholy != -1 && c < min_unholy ) ||
       ( max_unholy != -1 && c > max_unholy ) ) return false;
  /*
    c = GET_DEATH_RUNE_COUNT( count );
    if ( ( exact_death != -1 && c != exact_death ) ||
         ( min_death != -1 && c < min_death ) ||
         ( max_death != -1 && c > max_death ) ) return false;
  */
  return group_runes( p, cost_blood, cost_frost, cost_unholy, use );
}

// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================

void death_knight_spell_t::parse_options( option_t* options, const std::string& options_str )
{
  option_t base_options[] =
  {
    { "blood",         OPT_INT,  &exact_blood },
    { "blood>",        OPT_INT,  &min_blood },
    { "blood<",        OPT_INT,  &max_blood },
    { "frost",         OPT_INT,  &exact_frost },
    { "frost>",        OPT_INT,  &min_frost },
    { "frost<",        OPT_INT,  &max_frost },
    { "unholy",        OPT_INT,  &exact_unholy },
    { "unholy>",       OPT_INT,  &min_unholy },
    { "unholy<",       OPT_INT,  &max_unholy },
    { "death",         OPT_INT,  &exact_death },
    { "death>",        OPT_INT,  &min_death },
    { "death<",        OPT_INT,  &max_death },
    { "runic",         OPT_INT,  &exact_runic },
    { "runic>",        OPT_INT,  &min_runic },
    { "runic<",        OPT_INT,  &max_runic },
    { "blood_plague",  OPT_INT,  &exact_blood_plague },
    { "blood_plague>", OPT_INT,  &min_blood_plague },
    { "blood_plague<", OPT_INT,  &max_blood_plague },
    { "frost_fever",   OPT_INT,  &exact_frost_fever },
    { "frost_fever>",  OPT_INT,  &min_frost_fever },
    { "frost_fever<",  OPT_INT,  &max_frost_fever },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

void death_knight_spell_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  action_t::reset();
}

void death_knight_spell_t::consume_resource()
{
  if ( cost() > 0 ) spell_t::consume_resource();
  consume_runes( player, use, convert_runes == 0 ? false : sim->roll( convert_runes ) == 1 );
}

void death_knight_spell_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::execute();

  if ( result_is_hit() )
  {
    double gain = -cost();
    if ( gain > 0 ) p -> resource_gain( resource, gain, p -> gains_rune_abilities );

    if ( result == RESULT_CRIT )
    {
      p -> buffs_bloody_vengeance -> trigger();
    }
  }
  else
  {
    refund_power( this );
    refund_runes( p, use );
  }
}

void death_knight_spell_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::player_buff();

  if ( p -> active_presence == PRESENCE_BLOOD )
  {
    player_multiplier *= 1.0 + 0.15;
  }

  if ( school == SCHOOL_PHYSICAL )
  {
    player_multiplier *= 1.0 + p -> talents.bloody_vengeance * 0.01 * p -> buffs_bloody_vengeance -> stack();
  }

  if ( sim -> debug )
    log_t::output( sim, "death_knight_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f, p_mult=%.0f",
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

bool death_knight_spell_t::ready()
{
  death_knight_t* p = player -> cast_death_knight();

  if ( ! spell_t::ready() )
    return false;

  if ( exact_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] != exact_runic )
      return false;

  if ( min_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] < min_runic )
      return false;

  if ( max_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] > max_runic )
      return false;

  double ct = sim -> current_time;

  if ( exact_blood_plague > 0 )
    if ( ! p -> active_blood_plague || ( ( p -> active_blood_plague -> duration_ready - ct ) != exact_blood_plague ) )
      return false;

  if ( min_blood_plague > 0 )
    if ( ! p -> active_blood_plague || ( ( p -> active_blood_plague -> duration_ready - ct ) < min_blood_plague ) )
      return false;

  if ( max_blood_plague > 0 )
    if ( p -> active_blood_plague && ( ( p -> active_blood_plague -> duration_ready - ct ) > max_blood_plague ) )
      return false;

  if ( exact_frost_fever > 0 )
    if ( ! p -> active_frost_fever || ( ( p -> active_frost_fever -> duration_ready - ct ) != exact_frost_fever ) )
      return false;

  if ( min_frost_fever > 0 )
    if ( ! p -> active_frost_fever || ( ( p -> active_frost_fever -> duration_ready - ct ) < min_frost_fever ) )
      return false;

  if ( max_frost_fever > 0 )
    if ( p -> active_frost_fever && ( ( p -> active_frost_fever -> duration_ready - ct ) > max_frost_fever ) )
      return false;

  int count = count_runes( p );
  int c = GET_BLOOD_RUNE_COUNT( count );
  if ( ( exact_blood != -1 && c != exact_blood ) ||
       ( min_blood != -1 && c < min_blood ) ||
       ( max_blood != -1 && c > max_blood ) ) return false;

  c = GET_FROST_RUNE_COUNT( count );
  if ( ( exact_frost != -1 && c != exact_frost ) ||
       ( min_frost != -1 && c < min_frost ) ||
       ( max_frost != -1 && c > max_frost ) ) return false;

  c = GET_UNHOLY_RUNE_COUNT( count );
  if ( ( exact_unholy != -1 && c != exact_unholy ) ||
       ( min_unholy != -1 && c < min_unholy ) ||
       ( max_unholy != -1 && c > max_unholy ) ) return false;
  /*
    c = GET_DEATH_RUNE_COUNT( count );
    if ( ( exact_death != -1 && c != exact_death ) ||
         ( min_death != -1 && c < min_death ) ||
         ( max_death != -1 && c > max_death ) ) return false;
  */
  return group_runes( player, cost_blood, cost_frost, cost_unholy, use );
}

double death_knight_spell_t::total_crit_bonus() SC_CONST
{
  return spell_t::total_crit_bonus() + 0.5;
}

// =========================================================================
// Death Knight Attacks
// =========================================================================

// Melee Attack ============================================================

struct melee_t : public death_knight_attack_t
{
  melee_t( const char* name, player_t* player ) :
      death_knight_attack_t( name, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    base_dd_min = base_dd_max = 1;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> buffs_scent_of_blood -> up() )
      {
        p -> resource_gain( resource, 5, p -> gains_scent_of_blood );
        p -> buffs_scent_of_blood -> decrement();
      }
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public death_knight_attack_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) :
      death_knight_attack_t( "auto_attack", player )
  {
    death_knight_t* p = player -> cast_death_knight();

    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( "melee_main_hand", player );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
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
    if ( p -> off_hand_attack ) p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    return p -> main_hand_attack -> execute_event == 0;
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( player_t* player ) :
      death_knight_spell_t( "blood_plague", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    static rank_t ranks[] =
    {
      { 55, 1, 0, 0, 1, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    trigger_gcd       = 0;
    base_cost         = 0;
    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 5 + p -> talents.epidemic;
    base_attack_power_multiplier = 0.055;
    tick_power_mod    = 1;

    may_crit          = false;
    may_miss          = false;
    cooldown          = 0.0;

    observer = &( p -> active_blood_plague );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();
    p -> diseases++;
  }

  virtual void last_tick()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::last_tick();
    p -> diseases--;
  }

  virtual bool ready() { return false; }
};

struct blood_presence_t : public action_t
{
  blood_presence_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "blood_presence", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = 0;
    trigger_gcd = 0;
    cooldown    = 1.0;
    harmful     = false;
  }

  void execute()
  {
    const char* stance_name[] =
      { "Unknown Presence",
        "Blood Presence",
        "Frost Presence",
        "Unpossible Presence",
        "Unholy Presence"
      };
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> active_presence )
    {
      p -> aura_loss( stance_name[ p -> active_presence  ] );
    }
    p -> active_presence = PRESENCE_BLOOD;
    p -> aura_gain( stance_name[ p -> active_presence  ] );

    update_ready();
  }

  bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    return p -> active_presence != PRESENCE_BLOOD;
  }
};

struct blood_strike_t : public death_knight_attack_t
{
  blood_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "blood_strike", player, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
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
      { 0,   0, 0,     0,     0, 0 }
    };
    init_rank( ranks );

    cost_blood = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier *= 0.4;

    base_crit += p -> talents.subversion * 0.03;
    base_crit_bonus *= p -> talents.might_of_mograine * 0.15;
    base_multiplier *= 1 + p -> talents.bloody_strikes * 0.15;
  }

  void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases * 0.125;
    assert ( p -> diseases <= 2 );
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_abominations_might( this, 0.25 );
      trigger_sudden_doom( this );
    }
  }
};

// TODO: Implement me
struct blood_tap_t : public death_knight_spell_t
{
  blood_tap_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "blood_tap", player, SCHOOL_NONE, TREE_BLOOD )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 5, 227, 245, 0, -10 },
      { 73, 4, 187, 203, 0, -10 },
      { 67, 3, 161, 173, 0, -10 },
      { 61, 2, 144, 156, 0, -10 },
      { 55, 1, 127, 137, 0, -10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;

    base_execute_time = 0;
    direct_power_mod  = 0.1;
    cooldown          = 0.0;
  }

  void execute()
  {
    death_knight_spell_t::execute();
    if ( result_is_hit() ) player -> cast_death_knight() -> frost_fever -> execute();
  }
};

// TODO: Implement me
struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "dancing_rune_weapon", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.dancing_rune_weapon );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown   = 180.0;
    trigger_gcd = 0;
    base_cost  = p -> resource_max[ RESOURCE_MANA ] * 0.12;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "dancing_rune_weapon", 45.0 );
  }
};

struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( player_t* player, const std::string& options_str, bool sudden_doom = false ) :
      death_knight_spell_t( "death_coil", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 443, 443, 0, 40 },
      { 76, 4, 381, 381, 0, 40 },
      { 68, 3, 275, 275, 0, 40 },
      { 62, 2, 208, 208, 0, 40 },
      { 55, 1, 184, 184, 0, 40 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    direct_power_mod  = 0.15;
    cooldown          = 0.0;

    if ( sudden_doom )
    {
      proc = true;
      base_cost = 0;
      trigger_gcd = 0;
    }
  }
  bool ready()
  {
    if ( proc ) return false;

    return death_knight_spell_t::ready();
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80,  5, 222.75, 222.75, 0, -15 },
      { 75,  4, 150,    150,    0, -15 },
      { 70,  3, 123.75, 123.75, 0, -15 },
      { 63,  2, 97.5,   97.5,   0, -15 },
      { 56,  1, 84,     84,     0, -15 },
      { 0,   0, 0,      0,      0,  0 }
    };
    init_rank( ranks );

    cost_frost = 1;
    cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier *= 0.75;

    base_crit += p -> talents.improved_death_strike * 0.03;
    base_crit_bonus *= p -> talents.might_of_mograine * 0.15;
    base_multiplier *= 1 + p -> talents.improved_death_strike * 0.15;
    convert_runes = p->talents.death_rune_mastery / 3;
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() ) trigger_abominations_might( this, 0.5 );
  }
};

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "empower_rune_weapon", player, SCHOOL_NONE, TREE_FROST )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown = 300;
  }

  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    // TODO: What happens to death runes?
    p -> _runes.reset();
    p -> resource_gain( RESOURCE_RUNIC, 25, p -> gains_rune_abilities );
  }
};

struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( player_t* player ) :
      death_knight_spell_t( "frost_fever", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    static rank_t ranks[] =
    {
      { 55, 1, 0, 0, 1, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    trigger_gcd       = 0;
    base_cost         = 0;
    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 5 + p -> talents.epidemic;
    base_attack_power_multiplier = 0.055;
    tick_power_mod    = 1;

    may_crit          = false;
    may_miss          = false;
    cooldown          = 0.0;

    observer = &( p -> active_frost_fever );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();
    p -> diseases++;
  }

  virtual void last_tick()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::last_tick();
    p -> diseases--;
  }

  virtual bool ready()     { return false; }
};

struct frost_strike_t : public death_knight_attack_t
{
  frost_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "frost_strike", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
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
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.6;
  }
  bool ready()
  {
    return death_knight_attack_t::ready();
  }
};

struct heart_strike_t : public death_knight_attack_t
{
  heart_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "heart_strike", player, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.heart_strike );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
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
      { 0,   0, 0,     0,     0,  0 }
    };
    init_rank( ranks );

    cost_blood = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.5;

    base_crit += p -> talents.subversion * 0.03;
    base_crit_bonus *= p -> talents.might_of_mograine * 0.15;
    base_multiplier *= 1 + p -> talents.bloody_strikes * 0.15;
  }

  void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases * 0.1;
    assert ( p -> diseases <= 2 );
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_abominations_might( this, 0.25 );
      trigger_sudden_doom( this );
    }
  }
};

// TODO: Implement me
struct horn_of_winter_t : public death_knight_spell_t
{
  horn_of_winter_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "horn_of_winter", player, SCHOOL_NONE, TREE_FROST )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 5, 227, 245, 0, -10 },
      { 73, 4, 187, 203, 0, -10 },
      { 67, 3, 161, 173, 0, -10 },
      { 61, 2, 144, 156, 0, -10 },
      { 55, 1, 127, 137, 0, -10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;

    base_execute_time = 0;
    direct_power_mod  = 0.1;
    cooldown          = 0.0;
  }

  void execute()
  {
    death_knight_spell_t::execute();
    if ( result_is_hit() ) player -> cast_death_knight() -> frost_fever -> execute();
  }
};

struct hysteria_t : public action_t
{
  player_t* hysteria_target;

  hysteria_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "hysteria", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD ), hysteria_target( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.hysteria );

    std::string target_str = p -> hysteria_target_str;
    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( target_str.empty() )
    {
      hysteria_target = p;
    }
    else
    {
      hysteria_target = sim -> find_player( target_str );
      assert ( hysteria_target != 0 );
    }

    trigger_gcd = 0;
    cooldown = 30;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s grants %s Hysteria", player -> name(), hysteria_target -> name() );
    hysteria_target -> buffs.hysteria -> trigger();
    update_ready();
  }

  virtual bool ready()
  {
    if ( hysteria_target -> buffs.hysteria -> check() )
      return false;

    return action_t::ready();
  }
};

struct icy_touch_t : public death_knight_spell_t
{
  icy_touch_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "icy_touch", player, SCHOOL_FROST, TREE_FROST )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 5, 227, 245, 0, -10 },
      { 73, 4, 187, 203, 0, -10 },
      { 67, 3, 161, 173, 0, -10 },
      { 61, 2, 144, 156, 0, -10 },
      { 55, 1, 127, 137, 0, -10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;

    base_execute_time = 0;
    direct_power_mod  = 0.1;
    cooldown          = 0.0;
  }

  void execute()
  {
    death_knight_spell_t::execute();
    if ( result_is_hit() )
    {
      death_knight_t* p = player -> cast_death_knight();
      if ( ! p -> frost_fever )
      {
        p -> frost_fever = new frost_fever_t( p );
      }
      p -> frost_fever -> execute();
    }
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79,  4, 467.2, 467.2, 0, -15 },
      { 73,  3, 381.6, 381.6, 0, -15 },
      { 67,  2, 244,   244,   0, -15 },
      { 61,  1, 198.4, 198.4, 0, -15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;
    cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.8;

    base_crit += p -> talents.subversion * 0.03;
    convert_runes = p->talents.death_rune_mastery / 3;
  }

  void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases * 0.125;
    assert( p -> diseases >= 0 );
    assert( p -> diseases <= 2 );
  }

  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_abominations_might( this, 0.5 );

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
      { NULL, OPT_UNKNOWN, NULL }
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
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.5;
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      death_knight_t* p = player -> cast_death_knight();
      if ( ! p -> blood_plague )
      {
        p -> blood_plague = new blood_plague_t( p );
      }
      p -> blood_plague -> execute();
    }
  }
};

// TODO: Implement me
struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "raise_dead", player, SCHOOL_NONE, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.dancing_rune_weapon );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown   = 180.0;
    trigger_gcd = 0;
    base_cost  = p -> resource_max[ RESOURCE_MANA ] * 0.12;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "raise_dead", 45.0 );
  }
};

// TODO: Implement me
struct rune_tap_t : public death_knight_spell_t
{
  rune_tap_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "rune_tap", player, SCHOOL_NONE, TREE_BLOOD )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 5, 227, 245, 0, -10 },
      { 73, 4, 187, 203, 0, -10 },
      { 67, 3, 161, 173, 0, -10 },
      { 61, 2, 144, 156, 0, -10 },
      { 55, 1, 127, 137, 0, -10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;

    base_execute_time = 0;
    direct_power_mod  = 0.1;
    cooldown          = 0.0;
  }

  void execute()
  {
    death_knight_spell_t::execute();
    if ( result_is_hit() ) player -> cast_death_knight() -> frost_fever -> execute();
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79,  4, 357.188, 357.188, 0, -15 },
      { 73,  3, 291.375, 291.375, 0, -15 },
      { 67,  2, 186.75,  186.75,  0, -15 },
      { 55,  1, 151.875, 151.875, 0, -15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;
    cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.45;
  }

  void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases * 0.11;
    assert ( p -> diseases <= 2 );
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  =================================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "auto_attack"             ) return new auto_attack_t             ( this, options_str );

  // Blood Actions
//if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blood_presence"           ) return new blood_presence_t           ( this, options_str );
  if ( name == "blood_strike"             ) return new blood_strike_t             ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
//if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
//if ( name == "death_pact"               ) return new death_pact_t               ( this, options_str );
//if ( name == "forceful_deflection"      ) return new forceful_deflection_t      ( this, options_str );  Passive
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "hysteria"                 ) return new hysteria_t                 ( this, options_str );
//if ( name == "improved_blood_presence"  ) return new improved_blood_presence_t  ( this, options_str );  Passive
//if ( name == "mark_of_blood"            ) return new mark_of_blood_t            ( this, options_str );
//if ( name == "pestilence"               ) return new pestilence_t               ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
//if ( name == "strangulate"              ) return new strangulate_t              ( this, options_str );
//if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );

  // Frost Actions
//if ( name == "chains_of_ice"            ) return new chains_of_ice_t            ( this, options_str );
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
//if ( name == "frost_fever"              ) return new frost_fever_t              ( this, options_str );  Passive
//if ( name == "frost_presence"           ) return new frost_presence_t           ( this, options_str );
//if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
//if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );
  if ( name == "icy_touch"                ) return new icy_touch_t                ( this, options_str );
//if ( name == "improved_frost_presence"  ) return new improved_frost_presence_t  ( this, options_str );  Passive
//if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
//if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
//if ( name == "path_of_frost"            ) return new path_of_frost_t            ( this, options_str );  Non-Combat
//if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );
//if ( name == "runic_focus"              ) return new runic_focus_t              ( this, options_str );  Passive

  // Unholy Actions
//if ( name == "anti_magic_shell"         ) return new anti_magic_shell_t         ( this, options_str );
//if ( name == "anti_magic_zone"          ) return new anti_magic_zone_t          ( this, options_str );
//if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
//if ( name == "blood_plague"             ) return new blood_plague_t             ( this, options_str );  Passive
//if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
//if ( name == "corpse_explosion"         ) return new corpse_explosion_t         ( this, options_str );
//if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
//if ( name == "death_gate"               ) return new death_gate_t               ( this, options_str );  Non-Combat
//if ( name == "death_grip"               ) return new death_grip_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
//if ( name == "ghoul_frenzy"             ) return new ghoul_frenzy_t             ( this, options_str );
//if ( name == "improved_unholy_presence" ) return new improved_unholy_presence_t ( this, options_str );  Passive
  if ( name == "plague_strike"            ) return new plague_strike_t            ( this, options_str );
//if ( name == "raise_ally"               ) return new raise_ally_t               ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
//if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
//if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
//if ( name == "unholy_blight"            ) return new unholy_blight_t            ( this, options_str );
//if ( name == "unholy_presence"          ) return new unholy_presence_t          ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::init_race ======================================================

void death_knight_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_NIGHT_ELF:
  case RACE_GNOME:
  case RACE_UNDEAD:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_TAUREN:
  case RACE_BLOOD_ELF:
    break;
  default:
    race = RACE_NIGHT_ELF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

void death_knight_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_HEALTH );
  base_attack_crit                 = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_MELEE_CRIT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( level, DEATH_KNIGHT, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_STRENGTH ] *= 1.0 + talents.veteran_of_the_third_war * 0.02 + talents.abominations_might * 0.01;
  attribute_multiplier_initial[ ATTR_STAMINA ]  *= 1.0 + talents.veteran_of_the_third_war * 0.02;

  base_attack_power = -20;
  base_attack_expertise = 0.01 * talents.veteran_of_the_third_war * 2;

  initial_attack_power_per_strength = 2.0;

  resource_base[ RESOURCE_RUNIC ]  = 100 + talents.runic_power_mastery * 15;

  health_per_stamina = 10;
  base_gcd = 1.5;
}

void death_knight_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str  = "flask,type=endless_rage";
    action_list_str += "/food,type=dragonfin_filet";
    action_list_str += "/blood_presence";
    int num_items = items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }

    if ( race == RACE_DWARF )
    {
      if ( talents.bladed_armor > 0 )
        action_list_str += "/stoneform";
    }
    else if ( race == RACE_ORC )
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

    action_list_str += "/auto_attack";

    action_list_default = 1;
  }

  player_t::init_actions();
}

void death_knight_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    // Major Glyphs
    if     ( n == "anti_magic_shell"    ) glyphs.anti_magic_shell = 1;
    else if ( n == "blood_strike"        ) glyphs.blood_strike = 1;
    else if ( n == "bone_shield"         ) glyphs.bone_shield = 1;
    else if ( n == "chains_of_ice"       ) glyphs.chains_of_ice = 1;
    else if ( n == "dancing_rune_weapon" ) glyphs.dancing_rune_weapon = 1;
    else if ( n == "dark_command"        ) glyphs.dark_command = 1;
    else if ( n == "dark_death"          ) glyphs.dark_death = 1;
    else if ( n == "death_and_decay"     ) glyphs.death_and_decay = 1;
    else if ( n == "death_grip"          ) glyphs.death_grip = 1;
    else if ( n == "death_strike"        ) glyphs.death_strike = 1;
    else if ( n == "disease"             ) glyphs.disease = 1;
    else if ( n == "frost_strike"        ) glyphs.frost_strike = 1;
    else if ( n == "heart_strike"        ) glyphs.heart_strike = 1;
    else if ( n == "howling_blast"       ) glyphs.howling_blast = 1;
    else if ( n == "hungering_cold"      ) glyphs.hungering_cold = 1;
    else if ( n == "icebound_fortitude"  ) glyphs.icebound_fortitude = 1;
    else if ( n == "icy_touch"           ) glyphs.icy_touch = 1;
    else if ( n == "obliterate"          ) glyphs.obliterate = 1;
    else if ( n == "plague_strike"       ) glyphs.plague_strike = 1;
    else if ( n == "rune_strike"         ) glyphs.rune_strike = 1;
    else if ( n == "rune_tap"            ) glyphs.rune_tap = 1;
    else if ( n == "scourge_strike"      ) glyphs.scourge_strike = 1;
    else if ( n == "strangulate"         ) glyphs.strangulate = 1;
    else if ( n == "the_ghoul"           ) glyphs.the_ghoul = 1;
    else if ( n == "unbreakable_armor"   ) glyphs.unbreakable_armor = 1;
    else if ( n == "unholy_blight"       ) glyphs.unholy_blight = 1;
    else if ( n == "vampiric_blood"      ) glyphs.vampiric_blood = 1;

    // Minor Glyphs
    else if ( n == "blood_tap"           ) glyphs.blood_tap = 1;
    else if ( n == "corpse_explosion"    ) glyphs.corpse_explosion = 1;
    else if ( n == "deaths_embrace"      ) glyphs.deaths_embrace = 1;
    else if ( n == "horn_of_winter"      ) glyphs.horn_of_winter = 1;
    else if ( n == "pestilence"          ) glyphs.pestilence = 1;
    else if ( n == "raise_dead"          ) glyphs.raise_dead = 1;

    else if ( ! sim -> parent ) util_t::printf( "simcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

void death_knight_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_bloody_vengeance   = new buff_t( this, "bloody_vengeance",   3,                      0.0,  0.0, talents.bloody_vengeance );
  buffs_scent_of_blood     = new buff_t( this, "scent_of_blood",     talents.scent_of_blood, 0.0, 10.0, talents.scent_of_blood ? 0.15 : 0.00 );

  struct bloodworms_buff_t : public buff_t
  {
    bloodworms_buff_t( death_knight_t* p ) :
        buff_t( p, "bloodworms", 1, 19.99, 20.01, p -> talents.bloodworms * 0.03 ) {}
    virtual void start( int stacks, double value )
    {
      buff_t::start( stacks, value );
      player -> summon_pet( "bloodworms" );
    }
    virtual void expire()
    {
      buff_t::expire();
      death_knight_t* p = player -> cast_death_knight();
      if ( p -> active_bloodworms ) p -> active_bloodworms -> dismiss();
    }
  };
  buffs_bloodworms = new bloodworms_buff_t( this );
}

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains_rune_abilities     = get_gain( "rune_abilities" );
  gains_butchery           = get_gain( "butchery" );
  gains_power_refund       = get_gain( "power_refund" );
  gains_scent_of_blood     = get_gain( "scent_of_blood" );
}

void death_knight_t::init_procs()
{
  player_t::init_procs();

  procs_abominations_might = get_proc( "abominations_might", sim );
  procs_sudden_doom        = get_proc( "sudden_doom",        sim );
}

void death_knight_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_blood_plague       = get_uptime( "blood_plague" );
  uptimes_frost_fever        = get_uptime( "frost_fever" );
}

void death_knight_t::reset()
{
  player_t::reset();

  // Active
  active_presence     = 0;
  active_blood_plague = NULL;
  active_frost_fever  = NULL;
  diseases            = 0;

  // Pets and Guardians
  active_bloodworms = NULL;

  _runes.reset();
}

void death_knight_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.butchery )
  {
    struct butchery_regen_t : public event_t
    {
      butchery_regen_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
        name = "Butchery Regen";
        sim -> add_event( this, 5.0 );
      }
      virtual void execute()
      {
        death_knight_t* p = player -> cast_death_knight();
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.butchery, p -> gains_butchery );
        new ( sim ) butchery_regen_t( sim, p );
      }
    };
    new ( sim ) butchery_regen_t( sim, this );
  }
}

void death_knight_t::target_swing()
{
  player_t::target_swing();

  buffs_scent_of_blood -> trigger( 3 );
}

double death_knight_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();
  if ( talents.bladed_armor )
  { 
    ap += talents.bladed_armor * composite_armor() / 180;
  }
  return ap;
}

void death_knight_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  uptimes_blood_plague -> update( active_blood_plague != 0 );
  uptimes_frost_fever -> update( active_frost_fever != 0 );
}

// death_knight_t::primary_tree =================================================

int death_knight_t::primary_tree() SC_CONST
{
  if ( talents.heart_strike   ) return TREE_BLOOD;
  if ( talents.frost_strike   ) return TREE_FROST;
  if ( talents.scourge_strike ) return TREE_UNHOLY;

  return TALENT_TREE_MAX;
}

// death_knight_t::get_talent_trees ==============================================

bool death_knight_t::get_talent_trees( std::vector<int*>& blood, std::vector<int*>& frost, std::vector<int*>& unholy )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.butchery                         ) }, {  1, &( talents.improved_icy_touch      ) }, {  1, &( talents.vicious_strikes          ) } },
    { {  2, &( talents.subversion                       ) }, {  2, &( talents.runic_power_mastery     ) }, {  2, &( talents.virulence                ) } },
    { {  3, &( talents.blade_barrier                    ) }, {  3, &( talents.toughness               ) }, {  3, &( talents.anticipation             ) } },
    { {  4, &( talents.bladed_armor                     ) }, {  4, &( talents.icy_reach               ) }, {  4, &( talents.epidemic                 ) } },
    { {  5, &( talents.scent_of_blood                   ) }, {  5, &( talents.black_ice               ) }, {  5, &( talents.morbidity                ) } },
    { {  6, &( talents.two_handed_weapon_specialization ) }, {  6, &( talents.nerves_of_cold_steel    ) }, {  6, &( talents.unholy_command           ) } },
    { {  6, &( talents.rune_tap                         ) }, {  7, &( talents.icy_talons              ) }, {  7, &( talents.ravenous_dead            ) } },
    { {  8, &( talents.dark_conviction                  ) }, {  8, &( talents.lichborne               ) }, {  8, &( talents.outbreak                 ) } },
    { {  9, &( talents.death_rune_mastery               ) }, {  9, &( talents.annihilation            ) }, {  9, &( talents.necrosis                 ) } },
    { { 10, &( talents.improved_rune_tap                ) }, { 10, &( talents.killing_machine         ) }, { 10, &( talents.corpse_explosion         ) } },
    { { 11, &( talents.spell_reflection                 ) }, { 11, &( talents.chill_of_the_grave      ) }, { 11, &( talents.on_a_pale_horse          ) } },
    { { 12, &( talents.vendetta                         ) }, { 12, &( talents.endless_winter          ) }, { 12, &( talents.blood_caked_blade        ) } },
    { { 13, &( talents.bloody_strikes                   ) }, { 13, &( talents.frigid_deathplate       ) }, { 13, &( talents.night_of_the_dead        ) } },
    { { 14, &( talents.veteran_of_the_third_war         ) }, { 14, &( talents.glacier_rot             ) }, { 14, &( talents.unholy_blight            ) } },
    { { 15, &( talents.mark_of_blood                    ) }, { 15, &( talents.deathchill              ) }, { 15, &( talents.impurity                 ) } },
    { { 16, &( talents.bloody_vengeance                 ) }, { 16, &( talents.improved_icy_talons     ) }, { 16, &( talents.dirge                    ) } },
    { { 17, &( talents.abominations_might               ) }, { 17, &( talents.merciless_combat        ) }, { 17, &( talents.magic_supression         ) } },
    { { 18, &( talents.bloodworms                       ) }, { 18, &( talents.rime                    ) }, { 18, &( talents.reaping                  ) } },
    { { 19, &( talents.hysteria                         ) }, { 19, &( talents.chilblains              ) }, { 19, &( talents.master_of_ghouls         ) } },
    { { 20, &( talents.improved_blood_presence          ) }, { 20, &( talents.hungering_cold          ) }, { 20, &( talents.desecration              ) } },
    { { 21, &( talents.improved_death_strike            ) }, { 21, &( talents.improved_frost_presence ) }, { 21, &( talents.anti_magic_zone          ) } },
    { { 22, &( talents.sudden_doom                      ) }, { 22, &( talents.blood_of_the_north      ) }, { 22, &( talents.improved_unholy_presence ) } },
    { { 23, &( talents.vampiric_blood                   ) }, { 23, &( talents.unbreakable_armor       ) }, { 23, &( talents.ghoul_frenzy             ) } },
    { { 24, &( talents.will_of_the_necropolis           ) }, { 24, &( talents.acclimation             ) }, { 24, &( talents.crypt_fever              ) } },
    { { 25, &( talents.heart_strike                     ) }, { 25, &( talents.frost_strike            ) }, { 25, &( talents.bone_shield              ) } },
    { { 26, &( talents.might_of_mograine                ) }, { 26, &( talents.guile_of_gorefiend      ) }, { 26, &( talents.wandering_plague         ) } },
    { { 27, &( talents.blood_gorged                     ) }, { 27, &( talents.tundra_stalker          ) }, { 27, &( talents.ebon_plaguebringer       ) } },
    { { 28, &( talents.dancing_rune_weapon              ) }, { 28, &( talents.howling_blast           ) }, { 28, &( talents.scourge_strike           ) } },
    { {  0, NULL                                          }, {  0, NULL                                 }, { 29, &( talents.rage_of_rivendare        ) } },
    { {  0, NULL                                          }, {  0, NULL                                 }, { 30, &( talents.summon_gargoyle          ) } },
    { {  0, NULL                                          }, {  0, NULL                                 }, {  0, NULL                                  } }
  };

  return get_talent_translation( blood, frost, unholy, translation );
}

// death_knight_t::get_options ================================================

std::vector<option_t>& death_knight_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t death_knight_options[] =
    {
      // @option_doc loc=player/deathknight/misc title="Misc"
      { "butchery",                         OPT_INT, &( talents.butchery                         ) },
      { "subversion",                       OPT_INT, &( talents.subversion                       ) },
      { "blade_barrier",                    OPT_INT, &( talents.blade_barrier                    ) },
      { "bladed_armor",                     OPT_INT, &( talents.bladed_armor                     ) },
      { "scent_of_blood",                   OPT_INT, &( talents.scent_of_blood                   ) },
      { "two_handed_weapon_specialization", OPT_INT, &( talents.two_handed_weapon_specialization ) },
      { "rune_tap",                         OPT_INT, &( talents.rune_tap                         ) },
      { "dark_conviction",                  OPT_INT, &( talents.dark_conviction                  ) },
      { "death_rune_mastery",               OPT_INT, &( talents.death_rune_mastery               ) },
      { "improved_rune_tap",                OPT_INT, &( talents.improved_rune_tap                ) },
      { "spell_reflection",                 OPT_INT, &( talents.spell_reflection                 ) },
      { "vendetta",                         OPT_INT, &( talents.vendetta                         ) },
      { "bloody_strikes",                   OPT_INT, &( talents.bloody_strikes                   ) },
      { "veteran_of_the_third_war",         OPT_INT, &( talents.veteran_of_the_third_war         ) },
      { "mark_of_blood",                    OPT_INT, &( talents.mark_of_blood                    ) },
      { "bloody_vengeance",                 OPT_INT, &( talents.bloody_vengeance                 ) },
      { "abominations_might",               OPT_INT, &( talents.abominations_might               ) },
      { "bloodworms",                       OPT_INT, &( talents.bloodworms                       ) },
      { "hysteria",                         OPT_INT, &( talents.hysteria                         ) },
      { "improved_blood_presence",          OPT_INT, &( talents.improved_blood_presence          ) },
      { "improved_death_strike",            OPT_INT, &( talents.improved_death_strike            ) },
      { "sudden_doom",                      OPT_INT, &( talents.sudden_doom                      ) },
      { "vampiric_blood",                   OPT_INT, &( talents.vampiric_blood                   ) },
      { "will_of_the_necropolis",           OPT_INT, &( talents.will_of_the_necropolis           ) },
      { "heart_strike",                     OPT_INT, &( talents.heart_strike                     ) },
      { "might_of_mograine",                OPT_INT, &( talents.might_of_mograine                ) },
      { "blood_gorged",                     OPT_INT, &( talents.blood_gorged                     ) },
      { "dancing_rune_weapon",              OPT_INT, &( talents.dancing_rune_weapon              ) },

      { "improved_icy_touch",               OPT_INT, &( talents.improved_icy_touch               ) },
      { "runic_power_mastery",              OPT_INT, &( talents.runic_power_mastery              ) },
      { "toughness",                        OPT_INT, &( talents.toughness                        ) },
      { "icy_reach",                        OPT_INT, &( talents.icy_reach                        ) },
      { "black_ice",                        OPT_INT, &( talents.black_ice                        ) },
      { "nerves_of_cold_steel",             OPT_INT, &( talents.nerves_of_cold_steel             ) },
      { "icy_talons",                       OPT_INT, &( talents.icy_talons                       ) },
      { "lichborne",                        OPT_INT, &( talents.lichborne                        ) },
      { "annihilation",                     OPT_INT, &( talents.annihilation                     ) },
      { "killing_machine",                  OPT_INT, &( talents.killing_machine                  ) },
      { "chill_of_the_grave",               OPT_INT, &( talents.chill_of_the_grave               ) },
      { "endless_winter",                   OPT_INT, &( talents.endless_winter                   ) },
      { "frigid_deathplate",                OPT_INT, &( talents.frigid_deathplate                ) },
      { "glacier_rot",                      OPT_INT, &( talents.glacier_rot                      ) },
      { "deathchill",                       OPT_INT, &( talents.deathchill                       ) },
      { "improved_icy_talons",              OPT_INT, &( talents.improved_icy_talons              ) },
      { "merciless_combat",                 OPT_INT, &( talents.merciless_combat                 ) },
      { "rime",                             OPT_INT, &( talents.rime                             ) },
      { "chilblains",                       OPT_INT, &( talents.chilblains                       ) },
      { "hungering_cold",                   OPT_INT, &( talents.hungering_cold                   ) },
      { "improved_frost_presence",          OPT_INT, &( talents.improved_frost_presence          ) },
      { "blood_of_the_north",               OPT_INT, &( talents.blood_of_the_north               ) },
      { "unbreakable_armor",                OPT_INT, &( talents.unbreakable_armor                ) },
      { "acclimation",                      OPT_INT, &( talents.acclimation                      ) },
      { "frost_strike",                     OPT_INT, &( talents.frost_strike                     ) },
      { "guile_of_gorefiend",               OPT_INT, &( talents.guile_of_gorefiend               ) },
      { "tundra_stalker",                   OPT_INT, &( talents.tundra_stalker                   ) },
      { "howling_blast",                    OPT_INT, &( talents.howling_blast                    ) },

      { "vicious_strikes",                  OPT_INT, &( talents.vicious_strikes                  ) },
      { "virulence",                        OPT_INT, &( talents.virulence                        ) },
      { "anticipation",                     OPT_INT, &( talents.anticipation                     ) },
      { "epidemic",                         OPT_INT, &( talents.epidemic                         ) },
      { "morbidity",                        OPT_INT, &( talents.morbidity                        ) },
      { "unholy_command",                   OPT_INT, &( talents.unholy_command                   ) },
      { "ravenous_dead",                    OPT_INT, &( talents.ravenous_dead                    ) },
      { "outbreak",                         OPT_INT, &( talents.outbreak                         ) },
      { "necrosis",                         OPT_INT, &( talents.necrosis                         ) },
      { "corpse_explosion",                 OPT_INT, &( talents.corpse_explosion                 ) },
      { "on_a_pale_horse",                  OPT_INT, &( talents.on_a_pale_horse                  ) },
      { "blood_caked_blade",                OPT_INT, &( talents.blood_caked_blade                ) },
      { "night_of_the_dead",                OPT_INT, &( talents.night_of_the_dead                ) },
      { "unholy_blight",                    OPT_INT, &( talents.unholy_blight                    ) },
      { "impurity",                         OPT_INT, &( talents.impurity                         ) },
      { "dirge",                            OPT_INT, &( talents.dirge                            ) },
      { "magic_supression",                 OPT_INT, &( talents.magic_supression                 ) },
      { "reaping",                          OPT_INT, &( talents.reaping                          ) },
      { "master_of_ghouls",                 OPT_INT, &( talents.master_of_ghouls                 ) },
      { "desecration",                      OPT_INT, &( talents.desecration                      ) },
      { "anti_magic_zone",                  OPT_INT, &( talents.anti_magic_zone                  ) },
      { "improved_unholy_presence",         OPT_INT, &( talents.improved_unholy_presence         ) },
      { "ghoul_frenzy",                     OPT_INT, &( talents.ghoul_frenzy                     ) },
      { "crypt_fever",                      OPT_INT, &( talents.crypt_fever                      ) },
      { "bone_shield",                      OPT_INT, &( talents.bone_shield                      ) },
      { "wandering_plague",                 OPT_INT, &( talents.wandering_plague                 ) },
      { "ebon_plaguebringer",               OPT_INT, &( talents.ebon_plaguebringer               ) },
      { "scourge_strike",                   OPT_INT, &( talents.scourge_strike                   ) },
      { "rage_of_rivendare",                OPT_INT, &( talents.rage_of_rivendare                ) },
      { "summon_gargoyle",                  OPT_INT, &( talents.summon_gargoyle                  ) },
      // @option_doc loc=player/deathknight/misc title="Misc"
      { "hysteria_target",                  OPT_STRING, &( hysteria_target_str                   ) },
      { "sigil",                            OPT_STRING, &( items[ SLOT_RANGED ].options_str      ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, death_knight_options );
  }

  return options;
}

// death_knight_t::decode_set ===============================================

int death_knight_t::decode_set( item_t& item )
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

  bool is_melee = ( strstr( s, "helmet"         ) || 
		    strstr( s, "shoulderplates" ) ||
		    strstr( s, "battleplate"    ) ||
		    strstr( s, "legplates"      ) ||
		    strstr( s, "gauntlets"      ) );

  bool is_tank = ( strstr( s, "faceguard"  ) || 
		   strstr( s, "pauldrons"  ) ||
		   strstr( s, "chestguard" ) ||
		   strstr( s, "legguards"  ) ||
		   strstr( s, "handguards" ) );
    
  if ( strstr( s, "scourgeborne" ) ) 
  {
    if ( is_melee ) return SET_T7_MELEE;
    if ( is_tank  ) return SET_T7_TANK;
  }
  if ( strstr( s, "darkruned" ) )
  {
    if ( is_melee ) return SET_T8_MELEE;
    if ( is_tank  ) return SET_T8_TANK;
  }
  if ( strstr( s, "koltiras"    ) ||
       strstr( s, "thassarians" ) ) 
  {
    if ( is_melee ) return SET_T9_MELEE;
    if ( is_tank  ) return SET_T9_TANK;
  }
  return SET_NONE;
}

// player_t implementations ============================================

player_t* player_t::create_death_knight( sim_t* sim, const std::string& name, int race_type )
{
  death_knight_t* p = new death_knight_t( sim, name, race_type );

  new bloodworm_pet_t( sim, p );

  return p;
}

// player_t::death_knight_init ==============================================

void player_t::death_knight_init( sim_t* sim )
{
  sim -> auras.abominations_might = new aura_t( sim, "abominations_might" );

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.hysteria = new buff_t( p, "hysteria", 1, 30.0 );
  }

  target_t* t = sim -> target;
  t -> debuffs.crypt_fever  = new debuff_t( sim, "crypt_fever", -1 );
}

// player_t::death_knight_combat_begin ======================================

void player_t::death_knight_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.abominations_might ) sim -> auras.abominations_might -> override();

  target_t* t = sim -> target;
  if ( sim -> overrides.crypt_fever ) t -> debuffs.crypt_fever -> override();
}

