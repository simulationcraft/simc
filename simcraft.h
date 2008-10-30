// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <stdarg.h>
#include <float.h>
#include <time.h>
#include <string>
#include <queue>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Platform Initialization ==================================================

#if ! defined( UNIX ) && ! defined( WINDOWS ) && ! defined( MAC )
#  if defined( VISUAL_STUDIO ) || defined( __MSVCRT__ ) || defined( __MINWG32__ )
#    define WINDOWS
#  else
#    define UNIX
#  endif
#endif

// Cross-Platform Support for Multi-Threading ===============================

#if defined( MULTI_THREAD )
#  if defined( UNIX )
#    include <pthread.h>
#    define THREAD_HANDLE_T pthread_t
#  endif
#  if defined( WINDOWS )
#    define WIN32_LEAN_AND_MEAN
#    define VC_EXTRALEAN
#    include <windows.h>
#    include <process.h>
#    define THREAD_HANDLE_T HANDLE
#  endif
#endif

#if ! defined( THREAD_HANDLE_T )
#  define THREAD_HANDLE_T void*
#endif

// Patch Specific Modeling ==================================================

struct patch_t
{
   uint64_t mask;
   uint64_t encode( int arch, int version, int revision ) { return arch*10000 + version*100 + revision; }
   patch_t        ( int arch, int version, int revision ) {         mask = encode( arch, version, revision ); }
   void set       ( int arch, int version, int revision ) {         mask = encode( arch, version, revision ); }
   bool before    ( int arch, int version, int revision ) { return mask <  encode( arch, version, revision ); }
   bool after     ( int arch, int version, int revision ) { return mask >= encode( arch, version, revision ); }
   patch_t() { mask = encode( 0, 0, 0 ); }
};

// Forward Declarations ======================================================

struct action_t;
struct base_stats_t;
struct attack_t;
struct druid_t;
struct event_t;
struct gain_t;
struct mage_t;
struct option_t;
struct pet_t;
struct player_t;
struct priest_t;
struct proc_t;
struct report_t;
struct shaman_t;
struct sim_t;
struct rank_t;
struct rating_t;
struct stats_t;
struct spell_t;
struct talent_translation_t;
struct target_t;
struct uptime_t;
struct warlock_t;
struct warrior_t;
struct weapon_t;

// Event =====================================================================

struct event_t
{
  event_t*  next;
  sim_t*    sim;
  player_t* player;
  uint32_t  id;
  double    time;
  double    reschedule_time;
  int8_t    canceled;
  const char* name;
  event_t( sim_t* s, player_t* p=0, const char* n=0 ) : 
    next(0), sim(s), player(p), reschedule_time(0), canceled(0), name(n) 
  {
    if( ! name ) name = "unknown";
  }
  void reschedule( double new_time );
  double occurs() { return reschedule_time != 0 ? reschedule_time : time; }
  virtual void execute() { printf( "%s\n", name ? name : "(no name)" ); assert(0); }
  virtual ~event_t() {}
  static void cancel( event_t*& e ) { if( e ) { e -> canceled = 1;                 e=0; } }
  static void  early( event_t*& e ) { if( e ) { e -> canceled = 1; e -> execute(); e=0; } }
  // Simple free-list memory manager.
  static void* operator new( size_t, sim_t* ); 
  static void* operator new( size_t ); // DO NOT USE!
  static void  operator delete( void * );
  static void deallocate( event_t* e );
};

struct event_compare_t 
{
  bool operator () (event_t* lhs, event_t* rhs ) const
  {
    return( lhs -> time == rhs -> time ) ? ( lhs -> id > rhs -> id ) : ( lhs -> time > rhs -> time );
  }
};

// Simulation Engine =========================================================

enum player_type { PLAYER_NONE=0, DEATH_KNIGHT, DRUID, HUNTER, MAGE, PALADIN, PRIEST, ROGUE, SHAMAN, WARLOCK, WARRIOR, PLAYER_PET, PLAYER_TYPE_MAX };

enum dmg_type { DMG_DIRECT=0, DMG_OVER_TIME=1 };

enum attribute_type { ATTRIBUTE_NONE=0, ATTR_STRENGTH, ATTR_AGILITY, ATTR_STAMINA, ATTR_INTELLECT, ATTR_SPIRIT, ATTRIBUTE_MAX };

enum resource_type { RESOURCE_NONE=0, RESOURCE_HEALTH, RESOURCE_MANA, RESOURCE_RAGE, RESOURCE_ENERGY, RESOURCE_FOCUS, RESOURCE_RUNIC, RESOURCE_MAX };

enum result_type { RESULT_NONE=0, RESULT_MISS, RESULT_RESIST, RESULT_DODGE, RESULT_PARRY, RESULT_BLOCK, RESULT_GLANCE, RESULT_CRUSH, RESULT_CRIT, RESULT_HIT, RESULT_MAX };

enum action_type { ACTION_USE=0, ACTION_SPELL, ACTION_ATTACK, ACTION_OTHER, ACTION_MAX };

enum school_type {
     SCHOOL_NONE=0, 
     SCHOOL_ARCANE, 
     SCHOOL_CHAOS, 
     SCHOOL_FIRE, 
     SCHOOL_FROST, 
     SCHOOL_FROSTFIRE, 
     SCHOOL_HOLY, 
     SCHOOL_NATURE, 
     SCHOOL_PHYSICAL,
     SCHOOL_SHADOW, 
     SCHOOL_MAX 
};

enum talent_tree_type {
  TREE_NONE=0,
  TREE_BALANCE,    TREE_FERAL,       TREE_RESTORATION, // DRUID
  TREE_ARCANE,     TREE_FIRE,        TREE_FROST,       // MAGE
  TREE_DISCIPLINE, TREE_HOLY,        TREE_SHADOW,      // PRIEST
  TREE_ELEMENTAL,  TREE_ENHANCEMENT,                   // SHAMAN
  TREE_AFFLICTION, TREE_DEMONOLOGY,  TREE_DESTRUCTION, // WARLOCK
  TREE_ARMS,       TREE_PROTECTION,  TREE_FURY,        // WARRIOR
  TALENT_TREE_MAX
};

enum weapon_type { WEAPON_NONE=0, 
		   WEAPON_DAGGER,   WEAPON_FIST,                                                                    WEAPON_SMALL,
		   WEAPON_BEAST,    WEAPON_SWORD,    WEAPON_MACE,     WEAPON_AXE,                                   WEAPON_1H,
		   WEAPON_BEAST_2H, WEAPON_SWORD_2H, WEAPON_MACE_2H,  WEAPON_AXE_2H, WEAPON_STAFF,  WEAPON_POLEARM, WEAPON_2H,
		   WEAPON_BOW,      WEAPON_CROSSBOW, WEAPON_GUN,      WEAPON_WAND,   WEAPON_THROWN,                 WEAPON_RANGED,
		   WEAPON_MAX };

enum weapon_enchant_type { WEAPON_ENCHANT_NONE=0, MONGOOSE, EXECUTIONER, DEATH_FROST, WEAPON_ENCHANT_MAX };

enum weapon_buff_type { WEAPON_BUFF_NONE=0, 
			FIRE_STONE,
			FLAMETONGUE, 
			POISON,
			SHARPENING_STONE, 
			SPELL_STONE,
			WINDFURY, 
			WIZARD_OIL,
			WEAPON_BUFF_MAX };

enum elixir_type { ELIXIR_NONE=0,
		   ELIXIR_DRAENIC_WISDOM,                          
		   ELIXIR_MAJOR_MAGEBLOOD, 
		   ELIXIR_GUARDIAN,
		   ELIXIR_ADEPT,
		   ELIXIR_FEL_STRENGTH,
		   ELIXIR_GREATER_ARCANE,
		   ELIXIR_MAJOR_AGILITY, 
		   ELIXIR_MAJOR_FIRE_POWER, 
		   ELIXIR_MAJOR_FROST_POWER, 
		   ELIXIR_MAJOR_SHADOW_POWER, 
		   ELIXIR_MAJOR_STRENGTH, 
		   ELIXIR_MASTERY,
		   ELIXIR_MONGOOSE,
		   ELIXIR_ONSLAUGHT,
		   ELIXIR_SAGES,
		   ELIXIR_BATTLE,
		   ELIXIR_MAX };

enum flask_type { FLASK_NONE=0,
		  FLASK_BLINDING_LIGHT, 
		  FLASK_DISTILLED_WISDOM,
		  FLASK_MIGHTY_RESTORATION, 
		  FLASK_PURE_DEATH, 
		  FLASK_RELENTLESS_ASSAULT, 
		  FLASK_SUPREME_POWER,
		  FLASK_MAX };

enum position_type { POSITION_NONE=0, POSITION_FRONT, POSITION_BACK, POSITION_MAX };

enum encoding_type { ENCODING_NONE=0, ENCODING_BLIZZARD, ENCODING_MMO, ENCODING_WOWHEAD, ENCODING_MAX };

struct sim_t
{
  int         argc;
  char**      argv;
  sim_t*      parent;
  std::string patch_str;
  patch_t     patch;
  event_t*    event_list;
  event_t*    free_list;
  player_t*   player_list;
  player_t*   active_player;
  target_t*   target;
  double      lag, pet_lag, channel_penalty, gcd_penalty, reaction_time, regen_periodicity;
  double      current_time, max_time;
  int32_t     events_remaining, max_events_remaining;
  int32_t     events_processed, total_events_processed;
  int32_t     seed, id, iterations, threads;
  int8_t      infinite_resource[ RESOURCE_MAX ];
  int8_t      potion_sickness, average_dmg, log, debug, timestamp;

  // Gear Default
  struct gear_default_t
  {
    int16_t attribute[ ATTRIBUTE_MAX ];
    int16_t spell_power;
    int16_t attack_power;
    int16_t expertise_rating;
    int16_t armor_penetration_rating;
    int16_t hit_rating;
    int16_t crit_rating;
    int16_t haste_rating;
    
    gear_default_t() { memset( (void*) this, 0x00, sizeof( gear_default_t ) ); }
  };
  gear_default_t gear_default;

  // Gear Delta (for scaling)
  struct gear_delta_t
  {
    int16_t attribute[ ATTRIBUTE_MAX ];
    int16_t spell_power;
    int16_t attack_power;
    int16_t expertise_rating;
    int16_t armor_penetration_rating;
    int16_t hit_rating;
    int16_t crit_rating;
    int16_t haste_rating;
    
    gear_delta_t() { memset( (void*) this, 0x00, sizeof( gear_delta_t ) ); }
  };
  gear_delta_t gear_delta;

  // Reporting
  report_t* report;
  double    raid_dps, total_dmg, total_seconds, elapsed_cpu_seconds;
  int8_t    merge_ignite;
  std::vector<player_t*> players_by_rank;
  std::vector<player_t*> players_by_name;
  std::string html_file_str, wiki_file_str;
  FILE*       output_file;
  FILE*       html_file;
  FILE*       wiki_file;

  sim_t( sim_t* parent=0 );
 ~sim_t();

  void      execute( int argc, char** argv );
  void      add_event( event_t*, double delta_time );
  void      reschedule_event( event_t* );
  void      flush_events();
  void      cancel_events( player_t* );
  event_t*  next_event();
  bool      execute();
  void      reset();
  bool      init();
  void      analyze();
  void      analyze( int iteration );
  void      iterate();
  void      merge( sim_t& other_sim );
  bool      parse_option( const std::string& name, const std::string& value );
  void      print_options();
  bool      time_to_think( double proc_time ) { if( proc_time == 0 ) return false; return current_time - proc_time > reaction_time; }
  bool      cooldown_ready( double cooldown_time ) { return cooldown_time <= current_time; }
  player_t* find_player( const std::string& name );

  // Multi-Threading
  std::vector<sim_t*> children;
  THREAD_HANDLE_T thread_handle;
  void partition();
  void merge();
  void launch_child( sim_t* child );
  void wait_on_child( sim_t* child );
};

// Gear Rating Conversions ===================================================

struct rating_t
{
  double haste;
  double spell_hit, spell_crit;
  double attack_hit, attack_crit;
  double expertise, armor_penetration;
  rating_t() { memset( this, 0x00, sizeof(rating_t) ); }
  void init( int level );
  static double interpolate( int level, double val_60, double val_70, double val_80 );
};

// Weapon ====================================================================

struct weapon_t
{
  int8_t type, school;
  double damage;
  double swing_time;
  int8_t enchant, buff;
  double buff_bonus;
  bool   main;

  int    group();
  double normalized_weapon_speed();
  int8_t proc_per_minute_on_swing( double PPM, double adjusted_swing_time=0 );

  weapon_t( int t=WEAPON_NONE, double d=0, double st=2.0, int s=SCHOOL_PHYSICAL ) : 
    type(t), school(s), damage(d), swing_time(st), enchant(WEAPON_ENCHANT_NONE), buff(WEAPON_BUFF_NONE), buff_bonus(0), main(true) {}
};

// Player ====================================================================

struct player_t
{
  sim_t*      sim;
  std::string name_str;
  player_t*   next;   
  int8_t      type, level, party;
  double      gcd_ready, base_gcd;
  int8_t      sleeping;
  rating_t    rating;
  pet_t*      pet_list;

  // Haste
  int16_t base_haste_rating, initial_haste_rating, haste_rating;
  double  haste;

  // Attributes
  double attribute                   [ ATTRIBUTE_MAX ];
  double attribute_base              [ ATTRIBUTE_MAX ];
  double attribute_initial           [ ATTRIBUTE_MAX ];
  double attribute_multiplier        [ ATTRIBUTE_MAX ];
  double attribute_multiplier_initial[ ATTRIBUTE_MAX ];

  // Spell Mechanics
  double base_spell_power,       initial_spell_power[ SCHOOL_MAX+1 ], spell_power[ SCHOOL_MAX+1 ];
  double base_spell_hit,         initial_spell_hit,                   spell_hit;
  double base_spell_crit,        initial_spell_crit,                  spell_crit;
  double base_spell_penetration, initial_spell_penetration,           spell_penetration;
  double base_mp5,               initial_mp5,                         mp5;
  double spell_power_multiplier,    initial_spell_power_multiplier;
  double spell_power_per_intellect, initial_spell_power_per_intellect;
  double spell_power_per_spirit,    initial_spell_power_per_spirit;
  double spell_crit_per_intellect,  initial_spell_crit_per_intellect;
  double mp5_per_intellect;
  double spirit_regen_while_casting;
  double last_cast;

  // Attack Mechanics
  double    base_attack_power,       initial_attack_power,        attack_power;
  double    base_attack_hit,         initial_attack_hit,          attack_hit;
  double    base_attack_expertise,   initial_attack_expertise,    attack_expertise;
  double    base_attack_crit,        initial_attack_crit,         attack_crit;
  double    base_attack_penetration, initial_attack_penetration,  attack_penetration;
  double    attack_power_multiplier,   initial_attack_power_multiplier;
  double    attack_power_per_strength, initial_attack_power_per_strength;
  double    attack_power_per_agility,  initial_attack_power_per_agility;
  double    attack_crit_per_agility,   initial_attack_crit_per_agility;
  int8_t    position;

  // Weapons
  std::string main_hand_str,    off_hand_str,    ranged_str;
  weapon_t    main_hand_weapon, off_hand_weapon, ranged_weapon;

  // Resources
  double  resource_base   [ RESOURCE_MAX ];
  double  resource_initial[ RESOURCE_MAX ];
  double  resource_max    [ RESOURCE_MAX ];
  double  resource_current[ RESOURCE_MAX ];
  int8_t  resource_constrained;
  int16_t resource_constrained_count;
  double  resource_constrained_total_dmg;
  double  resource_constrained_total_time;
  double  mana_per_intellect;
  double  health_per_stamina;

  // Consumables
  std::string flask_str, elixirs_str, food_str;
  int8_t elixir_guardian;
  int8_t elixir_battle;
  int8_t flask;

  // Events
  event_t* executing;
  event_t* channeling;

  // Action Priority List
  action_t*   action_list;
  std::string action_list_prefix;
  std::string action_list_str;
  std::string action_list_postfix;
  std::string action_list_skip;

  // Reporting
  int8_t    quiet;
  action_t* last_foreground_action;
  double    last_action, total_seconds;
  double    total_waiting;
  double    iteration_dmg, total_dmg;
  double    resource_lost  [ RESOURCE_MAX ];
  double    resource_gained[ RESOURCE_MAX ];
  double    dps, dps_min, dps_max, dps_std_dev, dpr, rps_gain, rps_loss;
  proc_t*   proc_list;
  gain_t*   gain_list;
  stats_t*  stats_list;
  uptime_t* uptime_list;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;
  std::vector<double> iteration_dps;

  struct gear_t
  {
    // Attributes
    int16_t attribute        [ ATTRIBUTE_MAX ];
    int16_t attribute_enchant[ ATTRIBUTE_MAX ];
    // Spell Gear
    int16_t spell_power[ SCHOOL_MAX+1 ], spell_power_enchant[ SCHOOL_MAX+1 ];
    int16_t spell_penetration,           spell_penetration_enchant;
    int16_t mp5, mp5_enchant;
    // Attack Gear
    int16_t attack_power,             attack_power_enchant;
    int16_t armor_penetration_rating, armor_penetration_rating_enchant;
    int16_t expertise_rating,         expertise_rating_enchant;
    // Common Gear
    int16_t hit_rating, hit_rating_enchant;
    int16_t crit_rating, crit_rating_enchant;
    int16_t haste_rating, haste_rating_enchant;
    // Resource Gear
    int16_t resource        [ RESOURCE_MAX ];
    int16_t resource_enchant[ RESOURCE_MAX ];
    // Budgeting
    int16_t spell_power_budget;
    int16_t attack_power_budget;
    int8_t  budget_slots;
    // Unique Gear
    int8_t  ashtongue_talisman;
    int8_t  chaotic_skyfire;
    int8_t  darkmoon_crusade;
    int8_t  darkmoon_wrath;
    int8_t  elder_scribes;
    int8_t  eternal_sage;
    int8_t  eye_of_magtheridon;
    int8_t  lightning_capacitor;
    int8_t  mark_of_defiance;
    int8_t  mystical_skyfire;
    int8_t  quagmirrans_eye;
    int8_t  sextant_of_unstable_currents;
    int8_t  shiffars_nexus_horn;
    int8_t  spellstrike;
    int8_t  spellsurge;
    int8_t  talisman_of_ascendance;
    int8_t  timbals_crystal;
    int8_t  wrath_of_cenarius;
    int8_t  zandalarian_hero_charm;
    int8_t  tier4_2pc, tier4_4pc;
    int8_t  tier5_2pc, tier5_4pc;
    int8_t  tier6_2pc, tier6_4pc;
    
    gear_t() { memset( (void*) this, 0x00, sizeof( gear_t ) ); }

    void allocate_spell_power_budget( sim_t* );
    void allocate_attack_power_budget( sim_t* );
  };
  gear_t gear;
  
  struct buff_t
  {
    // Permanent Buffs
    int8_t    battle_shout;
    int8_t    blessing_of_kings;
    int8_t    blessing_of_might;
    int8_t    blessing_of_salvation;
    int8_t    blessing_of_wisdom;
    int8_t    sanctity_aura;
    int8_t    sanctified_retribution;
    int8_t    swift_retribution;
    // Temporary Buffs
    int8_t    temporary_buffs;
    int8_t    arcane_brilliance;
    double    mark_of_the_wild;
    int8_t    divine_spirit;
    int8_t    bloodlust;
    double    cast_time_reduction;
    int8_t    darkmoon_crusade;
    int8_t    darkmoon_wrath;
    int8_t    demonic_pact;
    int8_t    elemental_oath;
    int8_t    executioner;
    double    flametongue_totem;
    player_t* focus_magic;
    int8_t    focus_magic_feedback;
    double    improved_divine_spirit;
    int8_t    improved_moonkin_aura;
    int8_t    innervate;
    int8_t    lightning_capacitor;
    double    mana_cost_reduction;
    int8_t    moonkin_aura;
    int8_t    mongoose_mh;
    int8_t    mongoose_oh;
    int8_t    power_infusion;
    int8_t    replenishment;
    int8_t    shadow_form;
    double    strength_of_earth;
    int16_t   talisman_of_ascendance;
    double    totem_of_wrath;
    int8_t    unleashed_rage;
    int8_t    violet_eye;
    double    windfury_totem;
    int8_t    water_elemental_regen;
    int8_t    wrath_of_air;
    int16_t   zandalarian_hero_charm;
    int8_t    tier4_2pc, tier4_4pc;
    int8_t    tier5_2pc, tier5_4pc;
    int8_t    tier6_2pc, tier6_4pc;
    buff_t() { memset( (void*) this, 0x0, sizeof( buff_t ) ); }
    void reset()
    { 
      size_t delta = ( (uintptr_t) &temporary_buffs ) - ( (uintptr_t) this );
      memset( (void*) &temporary_buffs, 0x0, sizeof( buff_t ) - delta );
    }
  };
  buff_t buffs;
  
  struct expirations_t
  {
    float spellsurge;
    event_t* ashtongue_talisman;
    event_t* darkmoon_crusade;
    event_t* darkmoon_wrath;
    event_t* executioner;
    event_t* eye_of_magtheridon;
    event_t* focus_magic_feedback;
    event_t* mongoose_mh;
    event_t* mongoose_oh;
    event_t* spellstrike;
    event_t* wrath_of_cenarius;
    event_t *tier4_2pc, *tier4_4pc;
    event_t *tier5_2pc, *tier5_4pc;
    event_t *tier6_2pc, *tier6_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( expirations_t ) ); }
    expirations_t() { reset(); }
  };
  expirations_t expirations;
  
  struct cooldowns_t
  {
    double bloodlust;
    double elder_scribes;
    double eternal_sage;
    double judgement_of_wisdom;
    double mark_of_defiance;
    double mystical_skyfire;
    double quagmirrans_eye;
    double sextant_of_unstable_currents;
    double shiffars_nexus_horn;
    double spellsurge;
    double tier4_2pc, tier4_4pc;
    double tier5_2pc, tier5_4pc;
    double tier6_2pc, tier6_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( cooldowns_t ) ); }
    cooldowns_t() { reset(); }
  };
  cooldowns_t cooldowns;
  
  struct uptimes_t
  {
    uptime_t* executioner;
    uptime_t* mongoose_mh;
    uptime_t* mongoose_oh;
    uptime_t *tier4_2pc, *tier4_4pc;
    uptime_t *tier5_2pc, *tier5_4pc;
    uptime_t *tier6_2pc, *tier6_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( uptimes_t ) ); }
    uptimes_t() { reset(); }
  };
  uptimes_t uptimes;

  struct gains_t
  {
    gain_t* ashtongue_talisman;
    gain_t* dark_rune;
    gain_t* innervate;
    gain_t* judgement_of_wisdom;
    gain_t* mana_gem;
    gain_t* mana_potion;
    gain_t* mana_spring;
    gain_t* mana_tide;
    gain_t* mark_of_defiance;
    gain_t* mp5_regen;
    gain_t* replenishment;
    gain_t* restore_mana;
    gain_t* spellsurge;
    gain_t* spirit_regen;
    gain_t* vampiric_touch;
    gain_t* water_elemental_regen;
    gain_t *tier4_2pc, *tier4_4pc;
    gain_t *tier5_2pc, *tier5_4pc;
    gain_t *tier6_2pc, *tier6_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( gains_t ) ); }
    gains_t() { reset(); }
  };
  gains_t gains;
  
  struct procs_t
  {
    proc_t* ashtongue_talisman;
    proc_t* elder_scribes;
    proc_t* eternal_sage;
    proc_t* eye_of_magtheridon;
    proc_t* judgement_of_wisdom;
    proc_t* lightning_capacitor;
    proc_t* mark_of_defiance;
    proc_t* mystical_skyfire;
    proc_t* quagmirrans_eye;
    proc_t* sextant_of_unstable_currents;
    proc_t* shiffars_nexus_horn;
    proc_t* spellstrike;
    proc_t* timbals_crystal;
    proc_t* windfury;
    proc_t* wrath_of_cenarius;
    proc_t *tier4_2pc, *tier4_4pc;
    proc_t *tier5_2pc, *tier5_4pc;
    proc_t *tier6_2pc, *tier6_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( procs_t ) ); }
    procs_t() { reset(); }
  };
  procs_t procs;

  struct actions_t
  {
    action_t* lightning_discharge;
    action_t* timbals_discharge;
    void reset() { memset( (void*) this, 0x00, sizeof( actions_t ) ); }
    actions_t() { reset(); }
  };
  actions_t actions;

  struct scaling_t
  {
    double attribute[ ATTRIBUTE_MAX ];
    double spell_power;
    double attack_power;
    double expertise_rating;
    double armor_penetration_rating;
    double hit_rating;
    double crit_rating;
    double haste_rating;
    
    scaling_t() { memset( (void*) this, 0x00, sizeof( scaling_t ) ); }
  };
  scaling_t scaling;
  
  player_t( sim_t* sim, int8_t type, const std::string& name );
  
  virtual ~player_t();

  virtual const char* name() { return name_str.c_str(); }

  virtual void init();
  virtual void init_base() = 0;
  virtual void init_core();
  virtual void init_spell();
  virtual void init_attack();
  virtual void init_weapon( weapon_t*, std::string& );
  virtual void init_resources( bool force = false );
  virtual void init_consumables();
  virtual void init_actions();
  virtual void init_rating();
  virtual void init_stats();
  virtual void reset();

  virtual double composite_attack_power();
  virtual double composite_attack_crit();
  virtual double composite_attack_expertise()   { return attack_expertise;   }
  virtual double composite_attack_hit()         { return attack_hit;         }
  virtual double composite_attack_penetration() { return attack_penetration; }

  virtual double composite_spell_power( int8_t school );
  virtual double composite_spell_crit();
  virtual double composite_spell_hit()         { return spell_hit;         }
  virtual double composite_spell_penetration() { return spell_penetration; }

  virtual double composite_attack_power_multiplier();
  virtual double composite_spell_power_multiplier() { return 1.0; }
  virtual double composite_attribute_multiplier( int8_t attr );

  virtual double strength();
  virtual double agility();
  virtual double stamina();
  virtual double intellect();
  virtual double spirit();
  
  virtual void      schedule_ready( double delta_time=0, bool waiting=false );
  virtual action_t* execute_action();

  virtual void regen( double periodicity=2.0 );
  virtual void resource_gain( int8_t resource, double amount, gain_t* g=0 );
  virtual void resource_loss( int8_t resource, double amount );
  virtual bool resource_available( int8_t resource, double cost );
  virtual void check_resources();
  virtual int  primary_resource() { return RESOURCE_NONE; }

  virtual void  summon_pet( const std::string& name );
  virtual void dismiss_pet( const std::string& name );

  // Managing action_xxx events:
  // (1) To "throw" an event, ALWAYS invoke the action_xxx function.
  // (2) To "catch" an event, ALWAYS implement a spell_xxx or attack_xxx virtual function in player sub-class.
  // Disregarding these instructions may result in serious injury and/or death.

  virtual void action_start ( action_t* );
  virtual void action_miss  ( action_t* );
  virtual void action_hit   ( action_t* );
  virtual void action_tick  ( action_t* );
  virtual void action_damage( action_t*, double amount, int8_t dmg_type );
  virtual void action_finish( action_t* );

  virtual void spell_start_event ( spell_t* );
  virtual void spell_miss_event  ( spell_t* );
  virtual void spell_hit_event   ( spell_t* );
  virtual void spell_tick_event  ( spell_t* );
  virtual void spell_damage_event( spell_t*, double amount, int8_t dmg_type );
  virtual void spell_finish_event( spell_t* );

  virtual void attack_start_event ( attack_t* );
  virtual void attack_miss_event  ( attack_t* );
  virtual void attack_hit_event   ( attack_t* );
  virtual void attack_tick_event  ( attack_t* );
  virtual void attack_damage_event( attack_t*, double amount, int8_t dmg_type );
  virtual void attack_finish_event( attack_t* );

  void parse_talents( talent_translation_t*, const std::string& talent_string );
  void parse_wowhead( talent_translation_t translation[][3], const std::string& talent_string );

  virtual bool      parse_talents( const std::string& talent_string, int encoding ) { return false; }
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name ) { return 0; }

  // Class-Specific Methods

  static player_t * create_druid  ( sim_t* sim, std::string& name );
  static player_t * create_mage   ( sim_t* sim, std::string& name );
  static player_t * create_priest ( sim_t* sim, std::string& name );
  static player_t * create_shaman ( sim_t* sim, std::string& name );
  static player_t * create_warlock( sim_t* sim, std::string& name );

  druid_t  * cast_druid  () { assert( type == DRUID      ); return (druid_t  *) this; }
  mage_t   * cast_mage   () { assert( type == MAGE       ); return (mage_t   *) this; }
  priest_t * cast_priest () { assert( type == PRIEST     ); return (priest_t *) this; }
  shaman_t * cast_shaman () { assert( type == SHAMAN     ); return (shaman_t *) this; }
  warlock_t* cast_warlock() { assert( type == WARLOCK    ); return (warlock_t*) this; }
  pet_t*     cast_pet    () { assert( type == PLAYER_PET ); return (pet_t    *) this; }

  bool      in_gcd() { return gcd_ready > sim -> current_time; }
  bool      recent_cast();
  action_t* find_action( const std::string& );
  void      share_cooldown( const std::string& name, double ready );
  void      share_duration( const std::string& name, double ready );
  void      recalculate_haste()  {  haste = 1.0 / ( 1.0 + haste_rating / rating.haste ); }
  double    spirit_regen_per_second();
  void      init_mana_costs( rank_t* );
  bool      dual_wield() { return main_hand_weapon.type != WEAPON_NONE && off_hand_weapon.type != WEAPON_NONE; }
  void      aura_gain( const char* name );
  void      aura_loss( const char* name );
  gain_t*   get_gain  ( const std::string& name );
  proc_t*   get_proc  ( const std::string& name );
  stats_t*  get_stats ( const std::string& name );
  uptime_t* get_uptime( const std::string& name );
};

// Pet =======================================================================

struct pet_t : public player_t
{
  std::string full_name_str;
  player_t* owner;
  pet_t* next_pet;

  double   stamina_per_owner;
  double intellect_per_owner;

  pet_t( sim_t* sim, player_t* owner, const std::string& name );

  virtual double composite_attack_hit() { return owner -> composite_attack_hit(); }
  virtual double composite_spell_hit()  { return owner -> composite_spell_hit();  }

  virtual double stamina();
  virtual double intellect();

  virtual void reset();
  virtual void summon();
  virtual void dismiss();

  virtual const char* name() { return full_name_str.c_str(); }
};

// Target ====================================================================

struct target_t
{
  sim_t*      sim;
  std::string name_str;
  int8_t      level;
  int16_t     spell_resistance[ SCHOOL_MAX ];
  int16_t     initial_armor, armor;
  int16_t     block_value;
  int8_t      shield;
  double      initial_health, current_health;
  double      total_dmg;
  uptime_t*   uptime_list;

  struct cooldowns_t
  {
    double judgement_of_wisdom;
    void reset() { memset( (void*) this, 0x00, sizeof( cooldowns_t ) ); }
    cooldowns_t() { reset(); }
  };
  cooldowns_t cooldowns;

  struct debuff_t
  {
    // Permanent De-Buffs (until appropriate player class implemented)
    double   judgement_of_wisdom;
    int8_t   snare;
    double   sunder_armor;
    // Temporary De-Buffs
    int8_t   temporary_debuffs;
    int8_t   affliction_effects;
    int8_t   curse_of_elements;
    double   faerie_fire;
    double   frozen;
    int8_t   improved_faerie_fire;
    int8_t   improved_scorch;
    int8_t   mangle;
    int8_t   misery;
    int8_t   misery_stack;
    int8_t   earth_and_moon;
    int8_t   slow;
    int8_t   winters_chill;
    int8_t   winters_grasp;
    debuff_t() { memset( (void*) this, 0x0, sizeof( debuff_t ) ); }
    void reset()
    { 
      size_t delta = ( (uintptr_t) &temporary_debuffs ) - ( (uintptr_t) this );
      memset( (void*) &temporary_debuffs, 0x0, sizeof( debuff_t ) - delta );
    }
    bool snared() { return snare || slow || frozen; }
  };
  debuff_t debuffs;
  
  struct expirations_t
  {
    event_t* curse_of_elements;
    event_t* faerie_fire;
    event_t* frozen;
    event_t* earth_and_moon;
    event_t* improved_scorch;
    event_t* nature_vulnerability;
    event_t* shadow_vulnerability;
    event_t* shadow_weaving;
    event_t* winters_chill;
    event_t* winters_grasp;
    void reset() { memset( (void*) this, 0x00, sizeof( expirations_t ) ); }
    expirations_t() { reset(); }
  };
  expirations_t expirations;
  
  struct uptimes_t
  {
    uptime_t* winters_grasp;
    uptime_t* winters_chill;
    uptime_t* improved_scorch;
    void reset() { memset( (void*) this, 0x00, sizeof( uptimes_t ) ); }
    uptimes_t() { reset(); }
  };
  uptimes_t uptimes;

  target_t( sim_t* s );
 ~target_t();

  void init();
  void reset();
  void assess_damage( double amount, int8_t school, int8_t type );
  void recalculate_health();
  double composite_armor( player_t* );
  uptime_t* get_uptime( const std::string& name );
  bool parse_option( const std::string& name, const std::string& value );
  const char* name() { return name_str.c_str(); }
};

// Stats =====================================================================

struct stats_t
{
  std::string name_str;
  sim_t* sim;
  player_t* player;
  stats_t* next;
  int8_t school;
  bool channeled;
  bool analyzed;
  bool initialized;

  double resource_consumed;
  double frequency, num_executes, num_ticks;
  double total_execute_time, total_tick_time;
  double total_dmg;
  double dps, dpe, dpet, dpr;

  struct stats_results_t
  {
    double count, min_dmg, max_dmg, avg_dmg, total_dmg;
  };
  stats_results_t execute_results[ RESULT_MAX ];
  stats_results_t    tick_results[ RESULT_MAX ];

  int num_buckets;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;

  void consume_resource( double r ) { resource_consumed += r; }
  void add( double amount, int8_t dmg_type, int8_t result, double time );
  void init();
  void analyze();
  void merge( stats_t* other );
  stats_t( const std::string& name, player_t* );
};

// Rank ======================================================================

struct rank_t
{
  int8_t level, index;
  double dd_min, dd_max, tick, cost;
};

// Base Stats ================================================================

struct base_stats_t
{
  int8_t level;
  double health, mana;
  double strength, agility, stamina, intellect, spirit;
  double spell_crit, melee_crit;
};

// Action ====================================================================

struct action_t
{
  sim_t* sim;
  int8_t type;
  std::string name_str;
  player_t* player;
  int8_t school, resource, tree, result;
  bool bleed, binary, channeled, background, repeating, aoe, harmful, proc;
  bool may_miss, may_resist, may_dodge, may_parry, may_glance, may_block, may_crush, may_crit;
  double min_gcd, trigger_gcd;
  double base_execute_time, base_tick_time, base_cost;
  double   base_multiplier,   base_hit,   base_crit,   base_crit_bonus,   base_power,   base_penetration;
  double player_multiplier, player_hit, player_crit, player_crit_bonus, player_power, player_penetration;
  double target_multiplier, target_hit, target_crit, target_crit_bonus, target_power, target_penetration;
  double resource_consumed;
  double direct_dmg, base_direct_dmg, direct_power_mod;
  double   tick_dmg, base_tick_dmg,     tick_power_mod;
  int16_t num_ticks, current_tick, added_ticks;
  int8_t ticking;
  std::string cooldown_group, duration_group;
  double cooldown, cooldown_ready, duration_ready;
  weapon_t* weapon;
  bool normalize_weapon_speed;
  stats_t* stats;
  rank_t* rank;
  int8_t rank_index;
  event_t* event;
  double time_to_execute, time_to_tick;
  action_t** observer;
  action_t* next;
  std::vector<action_t*> same_list;

  action_t( int8_t type, const char* name, player_t* p=0, int8_t r=RESOURCE_NONE, int8_t s=SCHOOL_NONE, int8_t t=TREE_NONE );
  virtual ~action_t() {}

  virtual void parse_options( option_t*, const std::string& options_str );
  virtual rank_t* choose_rank( rank_t* rank_list );
  virtual double cost();
  virtual double haste()        { return 1.0;               }
  virtual double gcd()          { return trigger_gcd;       }
  virtual double execute_time() { return base_execute_time; }
  virtual double tick_time()    { return base_tick_time;    }
  virtual void   player_buff();
  virtual void   target_debuff( int8_t dmg_type );
  virtual void   calculate_result() { assert(0); }
  virtual bool   result_is_hit();
  virtual bool   result_is_miss();
  virtual double calculate_direct_damage(); 
  virtual double calculate_tick_damage(); 
  virtual double resistance();
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   tick();
  virtual void   last_tick();
  virtual void   assess_damage( double amount, int8_t dmg_type );
  virtual void   schedule_execute();
  virtual void   schedule_tick();
  virtual void   refresh_duration();
  virtual void   extend_duration( int8_t extra_ticks );
  virtual void   update_ready();
  virtual void   update_stats( int8_t type );
  virtual bool   ready();
  virtual void   reset();
  virtual void   cancel() { event_t::cancel( event ); reset(); }
  virtual const char* name() { return name_str.c_str(); }

  virtual double total_multiplier() { return base_multiplier * player_multiplier * target_multiplier; }
  virtual double total_power()      { return base_power      + player_power      + target_power;      }
  virtual double total_hit()        { return base_hit        + player_hit        + target_hit;        }
  virtual double total_crit()       { return base_crit       + player_crit       + target_crit;       }
  virtual double total_crit_bonus() { return base_crit_bonus * player_crit_bonus * target_crit_bonus; }

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
};

// Attack ====================================================================

struct attack_t : public action_t
{
  double base_expertise, player_expertise, target_expertise;

  attack_t( const char* n=0, player_t* p=0, int8_t r=RESOURCE_NONE, int8_t s=SCHOOL_PHYSICAL, int8_t t=TREE_NONE );
  virtual ~attack_t() {}

  // Attack Overrides
  virtual double haste();
  virtual void   player_buff();
  virtual void   target_debuff( int8_t dmg_type );
  virtual int    build_table( double* chances, int* results );
  virtual void   calculate_result();

  // Passthru Methods
  virtual double cost()                              { return action_t::cost();                    }
  virtual double gcd()                               { return action_t::gcd();                     }
  virtual double execute_time()                      { return action_t::execute_time();            }
  virtual double tick_time()                         { return action_t::tick_time();               }
  virtual double calculate_direct_damage()           { return action_t::calculate_direct_damage(); }
  virtual double calculate_tick_damage()             { return action_t::calculate_tick_damage();   }
  virtual double resistance()                        { return action_t::resistance();              }
  virtual void   consume_resource()                  { action_t::consume_resource();               }
  virtual void   execute()                           { action_t::execute();                        }
  virtual void   tick()                              { action_t::tick();                           }
  virtual void   last_tick()                         { action_t::last_tick();                      }
  virtual void   assess_damage( double a, int8_t t ) { action_t::assess_damage( a, t );            }
  virtual void   schedule_execute()                  { action_t::schedule_execute();               }
  virtual void   schedule_tick()                     { action_t::schedule_tick();                  }
  virtual bool   ready()                             { return action_t::ready();                   }
  virtual void   reset()                             { action_t::reset();                          }

  virtual double total_expertise() { return base_expertise + player_expertise + target_expertise; }
};

// Spell =====================================================================

struct spell_t : public action_t
{
  spell_t( const char* n=0, player_t* p=0, int8_t r=RESOURCE_NONE, int8_t s=SCHOOL_PHYSICAL, int8_t t=TREE_NONE );
  virtual ~spell_t() {}

  // Spell Overrides
  virtual double haste();
  virtual double gcd();
  virtual double execute_time();
  virtual double tick_time();
  virtual void   player_buff();
  virtual void   target_debuff( int8_t dmg_type );
  virtual double level_based_miss_chance( int8_t player, int8_t target );
  virtual void   calculate_result();
   
  // Passthru Methods
  virtual double cost()                              { return action_t::cost();                    }
  virtual double calculate_direct_damage()           { return action_t::calculate_direct_damage(); }
  virtual double calculate_tick_damage()             { return action_t::calculate_tick_damage();   }
  virtual double resistance()                        { return action_t::resistance();              }
  virtual void   consume_resource()                  { action_t::consume_resource();               }
  virtual void   execute()                           { action_t::execute();                        }
  virtual void   tick()                              { action_t::tick();                           }
  virtual void   last_tick()                         { action_t::last_tick();                      }
  virtual void   assess_damage( double a, int8_t t ) { action_t::assess_damage( a, t );            }
  virtual void   schedule_execute()                  { action_t::schedule_execute();               }
  virtual void   schedule_tick()                     { action_t::schedule_tick();                  }
  virtual bool   ready()                             { return action_t::ready();                   }
  virtual void   reset()                             { action_t::reset();                          }
};

// Player Ready Event ========================================================

struct player_ready_event_t : public event_t
{
  player_ready_event_t( sim_t* sim, player_t* p, double delta_time );
  virtual void execute();
};

// Action Execute Event ======================================================

struct action_execute_event_t : public event_t
{
  action_t* action;
  action_execute_event_t( sim_t* sim, action_t* a, double time_to_execute );
  virtual void execute();
};

// Action Tick Event =========================================================

struct action_tick_event_t : public event_t
{
  action_t* action;
  action_tick_event_t( sim_t* sim, action_t* a, double time_to_tick );
  virtual void execute();
};

// Regen Event ===============================================================

struct regen_event_t : public event_t
{
   regen_event_t( sim_t* sim );
   virtual void execute();
};

// Unique Gear ===============================================================

struct unique_gear_t
{
  static void spell_start_event ( spell_t* ) {}
  static void spell_miss_event  ( spell_t* );
  static void spell_hit_event   ( spell_t* );
  static void spell_tick_event  ( spell_t* );
  static void spell_damage_event( spell_t*, double amount, int8_t dmg_type ) {}
  static void spell_finish_event( spell_t* );

  static void attack_start_event ( attack_t* ) {}
  static void attack_miss_event  ( attack_t* ) {}
  static void attack_hit_event   ( attack_t* ) {}
  static void attack_tick_event  ( attack_t* ) {}
  static void attack_damage_event( attack_t*, double amount, int8_t dmg_type ) {}
  static void attack_finish_event( attack_t* ) {}

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
};

// Enchants ===================================================================

struct enchant_t
{
  static void spell_start_event ( spell_t* ) {}
  static void spell_miss_event  ( spell_t* ) {}
  static void spell_hit_event   ( spell_t* ) {}
  static void spell_tick_event  ( spell_t* ) {}
  static void spell_damage_event( spell_t*, double amount, int8_t dmg_type ) {}
  static void spell_finish_event( spell_t* );

  static void attack_start_event ( attack_t* ) {}
  static void attack_miss_event  ( attack_t* ) {}
  static void attack_hit_event   ( attack_t* );
  static void attack_tick_event  ( attack_t* ) {}
  static void attack_damage_event( attack_t*, double amount, int8_t dmg_type ) {}
  static void attack_finish_event( attack_t* ) {}

  static void trigger_flametongue_totem( attack_t* );
  static void trigger_windfury_totem   ( attack_t* );
};

// Consumable ================================================================

struct consumable_t
{
  static void init_flask  ( player_t* );
  static void init_elixirs( player_t* );
  static void init_food   ( player_t* );

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
};

// Gain ======================================================================

struct gain_t
{
  std::string name_str;
  double amount;
  gain_t* next;
  gain_t( const std::string& n ) : name_str(n), amount(0) {}
  void add( double a ) { amount += a; }
  void merge( gain_t* other ) { amount += other -> amount; }
  const char* name() { return name_str.c_str(); }
};

// Proc ======================================================================

struct proc_t
{
  std::string name_str;
  double count;
  double frequency;
  proc_t* next;
  proc_t( const std::string& n ) : name_str(n), count(0), frequency(0) {}
  void occur() { count++; }
  void merge( proc_t* other ) { count += other -> count; }
  const char* name() { return name_str.c_str(); }
};

// Up-Time ===================================================================

struct uptime_t
{
  std::string name_str;
  int32_t up, down;
  uptime_t* next;
  uptime_t( const std::string& n ) : name_str(n), up(0), down(0) {}
  void   update( bool is_up ) { if( is_up ) up++; else down++; }
  double percentage() { return (up==0) ? 0 : (100.0*up/(up+down)); }
  void   merge( uptime_t* other ) { up += other -> up; down += other -> down; }
  const char* name() { return name_str.c_str(); }
};

// Report =====================================================================

struct report_t
{
  sim_t* sim;

  int8_t report_actions;
  int8_t report_attack_stats;
  int8_t report_chart;
  int8_t report_core_stats;
  int8_t report_dpr;
  int8_t report_dps;
  int8_t report_gains;
  int8_t report_miss;
  int8_t report_rps;
  int8_t report_name;
  int8_t report_performance;
  int8_t report_procs;
  int8_t report_raid_dps;
  int8_t report_spell_stats;
  int8_t report_statistics;
  int8_t report_tag;
  int8_t report_uptime;
  int8_t report_waiting;

  int16_t report_scaling;
  int16_t report_delta_attribute[ ATTRIBUTE_MAX ];
  int16_t report_delta_spell_power;
  int16_t report_delta_attack_power;
  int16_t report_delta_expertise_rating;
  int16_t report_delta_armor_penetration_rating;
  int16_t report_delta_hit_rating;
  int16_t report_delta_crit_rating;
  int16_t report_delta_haste_rating;

  report_t( sim_t* s );
  bool parse_option( const std::string& name, const std::string& value );
  void print_action      ( stats_t* );
  void print_actions     ( player_t* );
  void print_core_stats  ( player_t* );
  void print_spell_stats ( player_t* );
  void print_attack_stats( player_t* );
  void print_gains();
  void print_procs();
  void print_uptime();
  void print_waiting();
  void print_performance();
  void print_scale_factors();
  void print();
  const char* chart_raid_dps( std::string& s );
  int         chart_raid_dpet( std::string& s , std::vector<std::string>& images );
  const char* chart_raid_downtime( std::string& s );
  const char* chart_raid_gear( std::string& s );
  const char* chart_raid_uptimes( std::string& s );
  const char* chart_action_dpet      ( std::string& s, player_t* );
  const char* chart_action_dmg       ( std::string& s, player_t* );
  const char* chart_gains            ( std::string& s, player_t* );
  const char* chart_uptimes_and_procs( std::string& s, player_t* );
  const char* chart_timeline         ( std::string& s, player_t* );
  void chart_html();
  void chart_wiki();
  void chart();
  void scale();
  static void timestamp( sim_t* sim );
  static void va_printf( sim_t*, const char* format, va_list );
  inline static void log( sim_t* sim, const char* format, ... )
  {
    va_list vap;
    va_start( vap, format );
    va_printf( sim, format, vap );
  }
};

// Talent Translation =========================================================

struct talent_translation_t
{
  int8_t  index;
  int8_t* address;

  static bool verify( player_t*, const std::string talent_string );
};

// Options ====================================================================

enum option_type_t { OPT_STRING=0, OPT_CHAR_P, OPT_INT8, OPT_INT16, OPT_INT32, OPT_FLT, OPT_DEPRECATED, OPT_UNKNOWN };

struct option_t
{
  char*  name; 
  int8_t type; 
  void*  address;

  static void print( sim_t*, option_t* );
  static bool parse( sim_t*, option_t*, const std::string& name, const std::string& value );
  static bool parse( sim_t*, char* line );
  static bool parse( sim_t*, int argc, char** argv );
};

// Random Number Generation ==================================================

struct rand_t
{
  static void     init( uint32_t seed );
  static uint32_t gen_uint32();
  static double   gen_float();
  static int8_t   roll( double chance );
};

// Utilities =================================================================

struct util_t
{
  static char* dup( const char* );

  static const char* player_type_string        ( int8_t type );
  static const char* attribute_type_string     ( int8_t type );
  static const char* dmg_type_string           ( int8_t type );
  static const char* result_type_string        ( int8_t type );
  static const char* resource_type_string      ( int8_t type );
  static const char* school_type_string        ( int8_t type );
  static const char* talent_tree_string        ( int8_t tree );
  static const char* weapon_type_string        ( int8_t type );
  static const char* weapon_enchant_type_string( int8_t type );
  static const char* weapon_buff_type_string   ( int8_t type );
  static const char* elixir_type_string        ( int8_t type );
  static const char* flask_type_string         ( int8_t type );

  static int string_split( std::vector<std::string>& results, const std::string& str, const char* delim );
  static int string_split( const std::string& str, const char* delim, const char* format, ... );
};
