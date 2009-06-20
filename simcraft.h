// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef __SIMCRAFT_H
#define __SIMCRAFT_H

// Platform Initialization ==================================================

#if defined( _MSC_VER ) || defined( __MINGW__ ) || defined( _WINDOWS ) || defined( WIN32 )
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  define _CRT_SECURE_NO_WARNINGS
#endif

#if defined( _MSC_VER )
#include "./vs/stdint.h"
#  define snprintf _snprintf
#else
#include <stdint.h>
#endif

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
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Patch Specific Modeling ==================================================

struct patch_t
{
  uint64_t mask;
  uint64_t encode( int arch, int version, int revision ) { return arch*10000 + version*100 + revision; }
  patch_t        ( int arch, int version, int revision ) {         mask = encode( arch, version, revision ); }
  void set       ( int arch, int version, int revision ) {         mask = encode( arch, version, revision ); }
  bool before    ( int arch, int version, int revision ) { return mask <  encode( arch, version, revision ); }
  bool after     ( int arch, int version, int revision ) { return mask >= encode( arch, version, revision ); }
  void decode( int* arch, int* version, int* revision )
  {
    uint64_t m = mask;
    *revision = ( int ) m % 100; m /= 100;
    *version  = ( int ) m % 100; m /= 100;
    *arch     = ( int ) m % 100;
  }
  patch_t() { mask = encode( 3, 1, 3 ); }
};

// Forward Declarations ======================================================

struct action_t;
struct action_callback_t;
struct attack_t;
struct spell_tick_t;
struct base_stats_t;
struct buff_expiration_t;
struct callback_t;
struct death_knight_t;
struct druid_t;
struct enchant_t;
struct event_t;
struct gain_t;
struct hunter_t;
struct mage_t;
struct option_t;
struct paladin_t;
struct pbuff_t;
struct pet_t;
struct player_t;
struct priest_t;
struct proc_t;
struct raid_event_t;
struct rank_t;
struct rating_t;
struct report_t;
struct rng_t;
struct rogue_t;
struct scaling_t;
struct shaman_t;
struct sim_t;
struct spell_t;
struct stats_t;
struct talent_translation_t;
struct target_t;
struct unique_gear_t;
struct uptime_t;
struct warlock_t;
struct warrior_t;
struct weapon_t;
struct urlSplit_t;
struct xml_node_t;

// Enumerations ==============================================================

enum race_type {
  RACE_NONE=0,
  // Target Races
  RACE_BEAST, RACE_DRAGONKIN, RACE_GIANT, RACE_HUMANOID,
  // Player Races
  RACE_NIGHT_ELF, RACE_HUMAN, RACE_GNOME, RACE_DWARF, RACE_DRAENEI, RACE_ORC, RACE_TROLL, RACE_UNDEAD, RACE_BLOOD_ELF, RACE_TAUREN,
  RACE_MAX
};

enum player_type {
  PLAYER_NONE=0,
  DEATH_KNIGHT, DRUID, HUNTER, MAGE, PALADIN, PRIEST, ROGUE, SHAMAN, WARLOCK, WARRIOR,
  PLAYER_PET, PLAYER_GUARDIAN,
  PLAYER_MAX
};

enum dmg_type { DMG_DIRECT=0, DMG_OVER_TIME=1 };

enum attribute_type { ATTRIBUTE_NONE=0, ATTR_STRENGTH, ATTR_AGILITY, ATTR_STAMINA, ATTR_INTELLECT, ATTR_SPIRIT, ATTRIBUTE_MAX };

enum resource_type {
  RESOURCE_NONE=0,
  RESOURCE_HEALTH, RESOURCE_MANA,  RESOURCE_RAGE, RESOURCE_ENERGY, RESOURCE_FOCUS, RESOURCE_RUNIC,
  RESOURCE_MAX };

enum result_type {
  RESULT_NONE=0,
  RESULT_MISS,  RESULT_RESIST, RESULT_DODGE, RESULT_PARRY,
  RESULT_BLOCK, RESULT_GLANCE, RESULT_CRIT,  RESULT_HIT,
  RESULT_MAX
};

#define RESULT_HIT_MASK  ( (1<<RESULT_GLANCE) | (1<<RESULT_BLOCK) | (1<<RESULT_CRIT) | (1<<RESULT_HIT) )
#define RESULT_CRIT_MASK ( (1<<RESULT_CRIT) )
#define RESULT_MISS_MASK ( (1<<RESULT_MISS) )
#define RESULT_ALL_MASK  -1

enum action_type { ACTION_USE=0, ACTION_SPELL, ACTION_ATTACK, ACTION_SEQUENCE, ACTION_OTHER, ACTION_MAX };

enum school_type {
  SCHOOL_NONE=0,
  SCHOOL_ARCANE,    SCHOOL_BLEED,  SCHOOL_CHAOS,  SCHOOL_FIRE,     SCHOOL_FROST,
  SCHOOL_FROSTFIRE, SCHOOL_HOLY,   SCHOOL_NATURE, SCHOOL_PHYSICAL, SCHOOL_SHADOW,
  SCHOOL_MAX
};

enum talent_tree_type {
  TREE_NONE=0,
  TREE_BLOOD,         TREE_UNHOLY,                         // DEATH KNIGHT
  TREE_BALANCE,       TREE_FERAL,        TREE_RESTORATION, // DRUID
  TREE_BEAST_MASTERY, TREE_MARKSMANSHIP, TREE_SURVIVAL,    // HUNTER
  TREE_ARCANE,        TREE_FIRE,         TREE_FROST,       // MAGE
                                         TREE_RETRIBUTION, // PALADIN
  TREE_DISCIPLINE,    TREE_HOLY,         TREE_SHADOW,      // PRIEST
  TREE_ASSASSINATION, TREE_COMBAT,       TREE_SUBTLETY,    // ROGUE
  TREE_ELEMENTAL,     TREE_ENHANCEMENT,                    // SHAMAN
  TREE_AFFLICTION,    TREE_DEMONOLOGY,   TREE_DESTRUCTION, // WARLOCK
  TREE_ARMS,          TREE_PROTECTION,   TREE_FURY,        // WARRIOR
  TALENT_TREE_MAX
};

enum weapon_type {
  WEAPON_NONE=0,
  WEAPON_DAGGER,                                                                                   WEAPON_SMALL,
  WEAPON_BEAST,    WEAPON_SWORD,    WEAPON_MACE,     WEAPON_AXE,   WEAPON_FIST,                    WEAPON_1H,
  WEAPON_BEAST_2H, WEAPON_SWORD_2H, WEAPON_MACE_2H,  WEAPON_AXE_2H, WEAPON_STAFF,  WEAPON_POLEARM, WEAPON_2H,
  WEAPON_BOW,      WEAPON_CROSSBOW, WEAPON_GUN,      WEAPON_WAND,   WEAPON_THROWN,                 WEAPON_RANGED,
  WEAPON_MAX
};

enum weapon_enchant_type { WEAPON_ENCHANT_NONE=0, BERSERKING, MONGOOSE, EXECUTIONER, DEATH_FROST, SCOPE, WEAPON_ENCHANT_MAX };

enum weapon_buff_type {
  WEAPON_BUFF_NONE=0,
  ANESTHETIC_POISON, DEADLY_POISON, FIRE_STONE, FLAMETONGUE, INSTANT_POISON,
  SHARPENING_STONE,  SPELL_STONE,   WINDFURY,   WIZARD_OIL,  WOUND_POISON,
  WEAPON_BUFF_MAX
};

enum slot_type { // these enum values match armory settings
  SLOT_NONE      = -1, 
  SLOT_HEAD      = 0,
  SLOT_NECK      = 1,
  SLOT_SHOULDER  = 2,
  SLOT_SHIRT     = 3,
  SLOT_CHEST     = 4,
  SLOT_WAIST     = 5,
  SLOT_LEGS      = 6,
  SLOT_FEET      = 7,
  SLOT_WRISTS    = 8,
  SLOT_HANDS     = 9,
  SLOT_FINGER_1  = 10,
  SLOT_FINGER_2  = 11,
  SLOT_TRINKET_1 = 12,
  SLOT_TRINKET_2 = 13,
  SLOT_BACK      = 14,
  SLOT_MAIN_HAND = 15, 
  SLOT_OFF_HAND  = 16, 
  SLOT_RANGED    = 17, 
  SLOT_MAX       = 18 
};

enum stat_type {
  STAT_NONE=0,
  STAT_STRENGTH, STAT_AGILITY, STAT_STAMINA, STAT_INTELLECT, STAT_SPIRIT,
  STAT_HEALTH, STAT_MANA, STAT_RAGE, STAT_ENERGY, STAT_FOCUS, STAT_RUNIC,
  STAT_SPELL_POWER, STAT_SPELL_PENETRATION, STAT_MP5,
  STAT_ATTACK_POWER, STAT_EXPERTISE_RATING, STAT_ARMOR_PENETRATION_RATING,
  STAT_HIT_RATING, STAT_CRIT_RATING, STAT_HASTE_RATING,
  STAT_WEAPON_DPS, STAT_WEAPON_SPEED,
  STAT_ARMOR,
  STAT_MAX
};

enum elixir_type {
  ELIXIR_NONE=0,
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
  ELIXIR_MAX
};

enum flask_type {
  FLASK_NONE=0,
  FLASK_BLINDING_LIGHT,
  FLASK_DISTILLED_WISDOM,
  FLASK_ENDLESS_RAGE,
  FLASK_FROST_WYRM,
  FLASK_MIGHTY_RESTORATION,
  FLASK_PURE_DEATH,
  FLASK_RELENTLESS_ASSAULT,
  FLASK_SUPREME_POWER,
  FLASK_MAX
};

enum food_type {
  FOOD_NONE=0,
  FOOD_TENDER_SHOVELTUSK_STEAK,
  FOOD_SMOKED_SALMON,
  FOOD_SNAPPER_EXTREME,
  FOOD_POACHED_BLUEFISH,
  FOOD_BLACKENED_BASILISK,
  FOOD_GOLDEN_FISHSTICKS,
  FOOD_CRUNCHY_SERPENT,
  FOOD_BLACKENED_DRAGONFIN,
  FOOD_DRAGONFIN_FILET,
  FOOD_HEARTY_RHINO,
  FOOD_GREAT_FEAST,
  FOOD_FISH_FEAST,
  FOOD_MAX
};

enum position_type { POSITION_NONE=0, POSITION_FRONT, POSITION_BACK, POSITION_RANGED, POSITION_MAX };

enum encoding_type { ENCODING_NONE=0, ENCODING_BLIZZARD, ENCODING_MMO, ENCODING_WOWHEAD, ENCODING_MAX };

enum profession_type {
  PROF_NONE=0,
  PROF_ALCHEMY,
  PROF_MINING,
  PROF_HERBALISM,
  PROF_LEATHERWORKING,
  PROF_ENGINEERING,
  PROF_BLACKSMITHING,
  PROF_INSCRIPTION,
  PROF_SKINNING,
  PROF_TAILORING,
  PROF_JEWELCRAFTING,
  PROF_ENCHANTING,
  PROF_MAX
};

enum role_type { ROLE_NONE=0, ROLE_ATTACK, ROLE_SPELL, ROLE_TANK, ROLE_HYBRID, ROLE_MAX };

enum rng_type
{
  // Specifies the general class of RNG desired
  RNG_DEFAULT=0,     // Do not care/know where it will be used
  RNG_GLOBAL,        // Returns reference to global RNG on sim_t
  RNG_DETERMINISTIC, // Returns reference to global deterministic RNG on sim_t
  RNG_CYCLIC,        // Normalized even/periodical results are acceptable
  RNG_DISTRIBUTED,   // Normalized variable/distributed values should be returned

  // Specifies a particular RNG desired
  RNG_STANDARD,          // Creates RNG using srand() and rand()
  RNG_MERSENNE_TWISTER,  // Creates RNG using SIMD oriented Fast Mersenne Twister
  RNG_PHASE_SHIFT,       // Simplistic cycle-based RNG, unsuitable for overlapping procs
  RNG_DISTANCE_SIMPLE,   // Simple normalized proc-separation RNG, suitable for fixed proc chance
  RNG_DISTANCE_BANDS,    // Complex normalized proc-separation RNG, suitable for varying proc chance
  RNG_PRE_FILL,          // Deterministic number of procs with random distribution
  RNG_MAX
};

// Thread Wrappers ===========================================================

struct thread_t
{
  static void launch( sim_t* );
  static void wait( sim_t* );
  static void lock();
  static void unlock();
};

// HTTP Download  ============================================================

struct http_t
{
  static bool cache_load();
  static bool cache_save();
  static void cache_clear();
  static void cache_set( const std::string& url, const std::string& result, uint32_t timestamp=0, bool lock=true );
  static bool cache_get( std::string& result, const std::string& url, bool force=false, bool lock=true );
  static bool download( std::string& result, const std::string& url );
  static bool get( std::string& result, const std::string& url, const std::string& confirmation=std::string(), int throttle_seconds=0 );
};

// Options ====================================================================

enum option_type_t
{
  OPT_NONE=0,
  OPT_STRING,     // std::string*
  OPT_APPEND,     // std::string* (append)
  OPT_CHARP,      // char*
  OPT_BOOL,       // int (only valid values are 1 and 0)
  OPT_INT,        // int
  OPT_FLT,        // double
  OPT_LIST,       // std::vector<std::string>*
  OPT_FUNC,       // function pointer
  OPT_STRING_Q,   // same as above, but do not save
  OPT_CHARP_Q,    // same as above, but do not save
  OPT_BOOL_Q,     // same as above, but do not save
  OPT_INT_Q,      // same as above, but do not save
  OPT_FLT_Q,      // same as above, but do not save
  OPT_DEPRECATED,
  OPT_UNKNOWN
};

typedef bool (*option_function_t)( sim_t* sim, const std::string& name, const std::string& value );

struct option_t
{
  const char* name;
  int type;
  void* address;

  void print( FILE* );
  void save ( FILE* );
  bool parse( sim_t*, const std::string& name, const std::string& value );

  static void copy( std::vector<option_t>& opt_vector, option_t* opt_array );
  static bool parse( sim_t*, std::vector<option_t>&, const std::string& name, const std::string& value );
  static bool parse( sim_t*, option_t*,              const std::string& name, const std::string& value );
  static bool parse_file( sim_t*, FILE* file );
  static bool parse_line( sim_t*, char* line );
  static bool parse_token( sim_t*, std::string& token );
};

// Event =====================================================================

struct event_t
{
  event_t*  next;
  sim_t*    sim;
  player_t* player;
  uint32_t  id;
  double    time;
  double    reschedule_time;
  int       canceled;
  const char* name;
  event_t( sim_t* s, player_t* p=0, const char* n=0 ) :
      next( 0 ), sim( s ), player( p ), reschedule_time( 0 ), canceled( 0 ), name( n )
  {
    if ( ! name ) name = "unknown";
  }
double occurs() { return reschedule_time != 0 ? reschedule_time : time; }
  virtual void reschedule( double new_time );
  virtual void execute() { printf( "%s\n", name ? name : "(no name)" ); assert( 0 ); }
  virtual ~event_t() {}
  static void cancel( event_t*& e ) { if ( e ) { e -> canceled = 1;                 e=0; } }
  static void  early( event_t*& e ) { if ( e ) { e -> canceled = 1; e -> execute(); e=0; } }
  // Simple free-list memory manager.
  static void* operator new( size_t, sim_t* );
  static void* operator new( size_t ) throw();  // DO NOT USE!
  static void  operator delete( void* );
  static void  operator delete( void*, sim_t* ) {}
  static void deallocate( event_t* e );
};

struct event_compare_t
{
  bool operator () ( event_t* lhs, event_t* rhs ) const
  {
    return( lhs -> time == rhs -> time ) ? ( lhs -> id > rhs -> id ) : ( lhs -> time > rhs -> time );
  }
};

// Raid Event

struct raid_event_t
{
  sim_t* sim;
  std::string name_str;
  int num_starts;
  double first, last;
  double cooldown;
  double cooldown_stddev;
  double cooldown_min;
  double cooldown_max;
  double duration;
  double duration_stddev;
  double duration_min;
  double duration_max;
  double distance_min;
  double distance_max;
  rng_t* rng;
  std::vector<player_t*> affected_players;

  raid_event_t( sim_t*, const char* name );
  virtual ~raid_event_t() {}

  virtual double cooldown_time();
  virtual double duration_time();
  virtual void schedule();
  virtual void reset();
  virtual void start();
  virtual void finish() {}
  virtual void parse_options( option_t*, const std::string& options_str );
  virtual const char* name() { return name_str.c_str(); }
  static raid_event_t* create( sim_t* sim, const std::string& name, const std::string& options_str );
  static void init( sim_t* );
};

// Gear Stats ================================================================

struct gear_stats_t
{
  double attribute[ ATTRIBUTE_MAX ];
  double resource[ RESOURCE_MAX ];
  double spell_power;
  double spell_penetration;
  double mp5;
  double attack_power;
  double expertise_rating;
  double armor_penetration_rating;
  double hit_rating;
  double crit_rating;
  double haste_rating;
  double weapon_dps;
  double weapon_speed;
  double armor;

  gear_stats_t() { memset( ( void* ) this, 0x00, sizeof( gear_stats_t ) ); }

  void   set_stat( int stat, double value );
  void   add_stat( int stat, double value );
  double get_stat( int stat );

  static double stat_mod( int stat );
  gear_stats_t& operator+=(const gear_stats_t& rhs);
  gear_stats_t operator+(const gear_stats_t& rhs) const;

};

// buffs
struct buff_expiration_t : public event_t
{
  pbuff_t* pbuff;
  int aura_id;
  int n_trig;
  buff_expiration_t( pbuff_t* p_buff, double b_duration=0, int aura_idx=0 ) ;
  virtual void execute();
};


struct pbuff_t{
  std::string name_str;
  player_t*  player;
  double buff_value; 
  double value; 
  buff_expiration_t* expiration; 
  int (*callback_expiration)(); //if set, will be called upon expiration
  int *trigger_counter;  // if set, will be incremented/decremented when buff is up/down
  double last_trigger, expected_end;
  uptime_t* uptime_cnt; 
  bool be_silent;
  int aura_id;
  double chance;
  rng_t* rng_chance;
  double buff_duration, buff_cooldown;
  bool ignore; // if he can not obtain this buff (no talents etc)
  pbuff_t* next;
  int n_triggers, n_trg_tries;
  // methods
  pbuff_t(player_t* plr, std::string name, double duration=0, double cooldown=0, int aura_idx=0, double use_value=0, bool t_ignore=false, double t_chance=0 );
  virtual ~pbuff_t() { };
  virtual void reset();
  virtual bool trigger(double val=1, double b_duration=0,int aura_idx=0);
  virtual bool dec_buff();
  virtual bool is_up_silent();
  virtual double mul_value_silent();
  virtual double add_value_silent();
  virtual void update_uptime(bool skip_usage=false);
  virtual bool is_up();
  virtual double mul_value();
  virtual double add_value();
  virtual double expiration_time();
};

struct buff_list_t{
  int n_buffs;
  pbuff_t* first;
  buff_list_t();
  void add_buff(pbuff_t* new_buff);
  void reset_buffs();
  pbuff_t* find_buff(std::string name);
  bool     chk_buff(std::string name);
};

struct act_expression_t{
  int    type;
  double value;
  void   *p_value;
  act_expression_t* operand_1;
  act_expression_t* operand_2;
  act_expression_t();
  virtual double evaluate();
  virtual bool   ok();
  static act_expression_t* create(action_t* action, std::string expression);
  static void warn(int severity, action_t* action, std::string msg);
  static act_expression_t* find_operator(action_t* action, std::string expression, std::string op_str, int op_type, bool binary);

};

// Simulation Engine =========================================================

struct sim_t
{
  int         argc;
  char**      argv;
  sim_t*      parent;
  patch_t     patch;
  bool        P312;
  bool        P313;
  bool        P320;
  rng_t*      rng;
  rng_t*      deterministic_rng;
  event_t*    free_list;
  target_t*   target;
  player_t*   player_list;
  player_t*   active_player;
  int         num_players;
  double      queue_lag, queue_lag_range;
  double      gcd_lag, gcd_lag_range;
  double      channel_lag, channel_lag_range;
  double      travel_variance;
  double      reaction_time, regen_periodicity;
  double      current_time, max_time;
  int         events_remaining, max_events_remaining;
  int         events_processed, total_events_processed;
  int         seed, id, iterations, current_iteration;
  int         infinite_resource[ RESOURCE_MAX ];
  int         armor_update_interval, potion_sickness;
  int         optimal_raid, log, debug, debug_armory;
  int         save_profiles;
  double      jow_chance, jow_ppm;
  urlSplit_t* last_armory_player;

  std::vector<option_t> option_vector;
  std::vector<std::string> party_encoding;

  // Smooth Random Number Generation
  int smooth_rng, deterministic_roll, average_range, average_gauss;

  // Timing Wheel Event Management
  event_t** timing_wheel;
  int    wheel_seconds, wheel_size, wheel_mask, timing_slice;
  double wheel_granularity;

  // Global Gear Stats
  gear_stats_t gear_default;
  gear_stats_t gear_delta;

  // Raid Events
  std::vector<raid_event_t*> raid_events;
  std::string raid_events_str;

  // Buffs and Debuffs Overrides
  struct overrides_t
  {
    // Overrides for NYI classes
    int battle_shout;
    int blessing_of_kings;
    int blessing_of_might;
    int blessing_of_wisdom;
    int crypt_fever;
    int judgement_of_wisdom;
    int sanctified_retribution;
    int snare;
    int sunder_armor;
    int swift_retribution;
    int thunder_clap;
    // Overrides for supported classes
    int abominations_might;
    int arcane_brilliance;
    int bleeding;
    int blood_frenzy;
    int bloodlust;
    int bloodlust_early;
    int curse_of_elements;
    int divine_spirit;
    int earth_and_moon;
    int faerie_fire;
    int ferocious_inspiration;
    int fortitude;
    int heroic_presence;
    int hunters_mark;
    int improved_moonkin_aura;
    int improved_scorch;
    int improved_shadow_bolt;
    int leader_of_the_pack;
    int mana_spring;
    int mangle;
    int mark_of_the_wild;
    int master_poisoner;
    int misery;
    int moonkin_aura;
    int poisoned;
    int rampage;
    int replenishment;
    int savage_combat;
    int strength_of_earth;
    int totem_of_wrath;
    int trauma;
    int trueshot_aura;
    int unleashed_rage;
    int windfury_totem;
    int winters_chill;
    int wrath_of_air;
    overrides_t() { memset( ( void* ) this, 0x0, sizeof( overrides_t ) ); }
  };
  overrides_t overrides;

  // Auras
  struct auras_t
  {
    int improved_moonkin;
    int leader_of_the_pack;
    int moonkin;
    int sanctified_retribution;
    int swift_retribution;
    int trueshot;
    void reset() { memset( ( void* ) this, 0x00, sizeof( auras_t ) ); }
    auras_t() { reset(); }
  };
  auras_t auras;

  // Expirations
  struct expirations_t
  {
    event_t* abominations_might;
    event_t* ferocious_inspiration;
    event_t* rampage;
    event_t* unleashed_rage;
    void reset() { memset( ( void* ) this, 0x00, sizeof( expirations_t ) ); }
    expirations_t() { reset(); }
  };
  expirations_t expirations;

  // Replenishment
  int replenishment_targets;
  std::vector<player_t*> replenishment_candidates;

  // Reporting
  report_t*  report;
  scaling_t* scaling;
  double     raid_dps, total_dmg, total_seconds, elapsed_cpu_seconds;
  int        merge_ignite;
  int        report_progress;
  std::vector<player_t*> players_by_rank;
  std::vector<player_t*> players_by_name;
  std::vector<std::string> id_dictionary;
  std::vector<std::string> dps_charts, gear_charts, dpet_charts;
  std::string downtime_chart, uptimes_chart;
  std::string output_file_str, log_file_str, html_file_str, wiki_file_str, xml_file_str;
  FILE* output_file;
  FILE* log_file;
  int http_throttle;
  int duration_uptimes;

  // Multi-Threading
  int threads;
  std::vector<sim_t*> children;
  void* thread_handle;
  int  thread_index;

  sim_t( sim_t* parent=0, int thrdID=0 );
  virtual ~sim_t();

  int       main( int argc, char** argv );
  void      combat( int iteration );
  void      combat_begin();
  void      combat_end();
  void      add_event( event_t*, double delta_time );
  void      reschedule_event( event_t* );
  void      flush_events();
  void      cancel_events( player_t* );
  event_t*  next_event();
  void      reset();
  bool      init();
  void      analyze();
  void      merge( sim_t& other_sim );
  void      merge();
  void      iterate();
  void      partition();
  void      execute();
  void      print_options();
  std::vector<option_t>& get_options();
  bool      parse_option( const std::string& name, const std::string& value );
  bool      parse_options( int argc, char** argv );
  bool      time_to_think( double proc_time ) { if ( proc_time == 0 ) return false; return current_time - proc_time > reaction_time; }
  bool      cooldown_ready( double cooldown_time ) { return cooldown_time <= current_time; }
  int       roll( double chance );
  double    range( double min, double max );
  double    gauss( double mean, double stddev );
  player_t* find_player( const std::string& name );
};

// Scaling ===================================================================

struct scaling_t
{
  sim_t* sim;
  int    calculate_scale_factors;
  int    center_scale_delta;
  int    scale_lag;
  double scale_factor_noise;
  int    normalize_scale_factors;
  int    smooth_scale_factors;
  int    debug_scale_factors;

  // Gear delta for determining scale factors
  gear_stats_t stats;

  scaling_t( sim_t* s );

  void init_deltas();
  void analyze();
  void analyze_stats();
  void analyze_lag();
  void analyze_gear_weights();
  void normalize();
  void derive();
  int  get_options( std::vector<option_t>& );
};

// Gear Rating Conversions ===================================================

struct rating_t
{
  double  spell_haste,  spell_hit,  spell_crit;
  double attack_haste, attack_hit, attack_crit;
  double expertise, armor_penetration;
  rating_t() { memset( this, 0x00, sizeof( rating_t ) ); }
  void init( int level );
  static double interpolate( int level, double val_60, double val_70, double val_80 );
};

// Weapon ====================================================================

struct weapon_t
{
  int    type, school;
  double damage;
  double swing_time;
  int    enchant, buff;
  double enchant_bonus, buff_bonus;
  int    slot;

  int    group();
  double normalized_weapon_speed();
  double proc_chance_on_swing( double PPM, double adjusted_swing_time=0 );

  weapon_t( int t=WEAPON_NONE, double d=0, double st=2.0, int s=SCHOOL_PHYSICAL ) :
      type( t ), school( s ), damage( d ), swing_time( st ),
      enchant( WEAPON_ENCHANT_NONE ), buff( WEAPON_BUFF_NONE ),
      enchant_bonus( 0 ), buff_bonus( 0 ), slot( SLOT_NONE ) {}};

// Item ======================================================================

struct item_t
{
  int slot;
  sim_t* sim;
  player_t* player;

  // Raw data from Armory
  xml_node_t* slot_xml;
  xml_node_t* item_xml;
  std::string name_str, id_str;

  // Encoded data from profiles
  std::string encoded_name_str;
  std::string encoded_equip_str;
  std::string encoded_use_str;
  std::string encoded_enchant_str;
  std::string encoded_stats_str;
  std::string encoded_gems_str;
  std::string encoded_str;

  // Extracted data
  gear_stats_t stats, gem_stats, enchant_stats;
  struct use_t {
    int stat;
    double amount, duration, cooldown;
  } use;
  struct equip_t {
    int stat, school, max_stacks;
    double amount, proc_chance, duration, cooldown;
  } equip;

  item_t() : slot(SLOT_NONE) { memset( &use, 0x00, sizeof(use_t) );  memset( &equip, 0x00, sizeof(equip_t) ); }
  bool active();
  bool encode();
  bool decode();
  bool download( const std::string& id_str );
  bool replace ( const std::string& id_str );
  const char* name();
};

// Player ====================================================================

struct player_t
{
  sim_t*      sim;
  std::string name_str, talents_str, id_str;
  player_t*   next;
  int         index, type, level, party, member;
  double      distance;
  double      gcd_ready, base_gcd;
  int         stunned, moving, sleeping, initialized;
  rating_t    rating;
  pet_t*      pet_list;

  std::vector<option_t> option_vector;
  std::vector<item_t> items;

  // Profs
  std::string   professions_str;
  double profession [ PROF_MAX ];

  // Race
  std::string race_str;
  int         race;

  // Haste
  double base_haste_rating, initial_haste_rating, haste_rating;
  double spell_haste, attack_haste;

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
  double mana_regen_base;
  double mana_regen_while_casting;
  double energy_regen_per_second;
  double focus_regen_per_second;
  double last_cast;

  // Attack Mechanics
  double base_attack_power,       initial_attack_power,        attack_power;
  double base_attack_hit,         initial_attack_hit,          attack_hit;
  double base_attack_expertise,   initial_attack_expertise,    attack_expertise;
  double base_attack_crit,        initial_attack_crit,         attack_crit;
  double base_attack_penetration, initial_attack_penetration,  attack_penetration;
  double attack_power_multiplier,   initial_attack_power_multiplier;
  double attack_power_per_strength, initial_attack_power_per_strength;
  double attack_power_per_agility,  initial_attack_power_per_agility;
  double attack_crit_per_agility,   initial_attack_crit_per_agility;
  int    position;

  // Defense Mechanics
  double base_armor, initial_armor, armor, armor_snapshot;
  double armor_per_agility;
  bool   use_armor_snapshot;

  // Weapons
  std::string main_hand_str,    off_hand_str,    ranged_str;
  weapon_t    main_hand_weapon, off_hand_weapon, ranged_weapon;

  // Resources
  double  resource_base   [ RESOURCE_MAX ];
  double  resource_initial[ RESOURCE_MAX ];
  double  resource_max    [ RESOURCE_MAX ];
  double  resource_current[ RESOURCE_MAX ];
  double  mana_per_intellect;
  double  health_per_stamina;

  // Replenishment
  std::vector<player_t*> replenishment_targets;

  // Consumables
  std::string flask_str, elixirs_str, food_str;
  int elixir_guardian;
  int elixir_battle;
  int flask;
  int food;

  // Events
  action_t* executing;
  action_t* channeling;
  event_t*  readying;
  bool      in_combat;

  // Callbacks
  std::vector<action_callback_t*> resource_gain_callbacks[ RESOURCE_MAX ];
  std::vector<action_callback_t*> resource_loss_callbacks[ RESOURCE_MAX ];
  std::vector<action_callback_t*> attack_result_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*>  spell_result_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*> tick_callbacks;
  std::vector<action_callback_t*> tick_damage_callbacks;
  std::vector<action_callback_t*> direct_damage_callbacks;

  // Action Priority List
  action_t*   action_list;
  std::string action_list_prefix;
  std::string action_list_str;
  std::string action_list_postfix;
  std::string action_list_skip;
  int         action_list_default;

  // Reporting
  int    quiet;
  action_t* last_foreground_action;
  double    last_action_time, total_seconds;
  double    total_waiting;
  double    iteration_dmg, total_dmg;
  double    resource_lost  [ RESOURCE_MAX ];
  double    resource_gained[ RESOURCE_MAX ];
  double    dps, dps_min, dps_max, dps_std_dev, dps_error;
  double    dpr, rps_gain, rps_loss;
  proc_t*   proc_list;
  gain_t*   gain_list;
  stats_t*  stats_list;
  uptime_t* uptime_list;
  std::vector<double> timeline_resource;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;
  std::vector<double> iteration_dps;
  std::vector<int> distribution_dps;
  std::string action_dpet_chart, action_dmg_chart, gains_chart, uptimes_and_procs_chart;
  std::string timeline_resource_chart, timeline_dps_chart, distribution_dps_chart;
  std::string gear_weights_lootrank_link, gear_weights_wowhead_link, gear_weights_pawn_string;
  std::string save_str;

  // Gear
  gear_stats_t stats;
  gear_stats_t gear_stats;
  gear_stats_t gem_stats;
  enchant_t* enchant;
  unique_gear_t* unique_gear;
  std::string clicky_1, clicky_2, clicky_3;

  // Scale Factors
  gear_stats_t scaling;
  gear_stats_t normalized_scaling;
  int          normalized_to;
  double       scaling_lag;
  int          scales_with[ STAT_MAX ];

  struct buff_t
  {
    int       abominations_might;
    double    arcane_brilliance;
    int       battle_shout;
    int       blessing_of_kings;
    int       blessing_of_might;
    double    blessing_of_wisdom;
    double    divine_spirit;
    int       bloodlust;
    double    cast_time_reduction;
    double    demonic_pact;
    pet_t*    demonic_pact_pet;
    int       elemental_oath;
    int       ferocious_inspiration;
    double    flametongue_totem;
    player_t* focus_magic;
    int       focus_magic_feedback;
    double    fortitude;
    double    innervate;
    double    glyph_of_innervate;
    int       hysteria;
    double    mana_cost_reduction;
    double    mana_spring;
    double    mark_of_the_wild;
    int       mongoose_mh;
    int       mongoose_oh;
    int       power_infusion;
    int       rampage;
    int       replenishment;
    int       shadow_form;
    double    strength_of_earth;
    double    totem_of_wrath;
    int       tricks_of_the_trade;
    int       unleashed_rage;
    double    windfury_totem;
    int       water_elemental;
    int       wrath_of_air;
    int       tier4_2pc, tier4_4pc;
    int       tier5_2pc, tier5_4pc;
    int       tier6_2pc, tier6_4pc;
    int       tier7_2pc, tier7_4pc;
    int       tier8_2pc, tier8_4pc;
    void reset() { memset( ( void* ) this, 0x0, sizeof( buff_t ) ); }
    buff_t() { reset(); }
  };
  buff_t buffs;

  struct expirations_t
  {
    event_t* ashtongue_talisman;
    event_t* focus_magic_feedback;
    event_t* hysteria;
    event_t* replenishment;
    event_t* tricks_of_the_trade;
    event_t *tier4_2pc, *tier4_4pc;
    event_t *tier5_2pc, *tier5_4pc;
    event_t *tier6_2pc, *tier6_4pc;
    event_t *tier7_2pc, *tier7_4pc;
    event_t *tier8_2pc, *tier8_4pc;
    void reset() { memset( ( void* ) this, 0x00, sizeof( expirations_t ) ); }
    expirations_t() { reset(); }
  };
  expirations_t expirations;

  struct cooldowns_t
  {
    double bloodlust;
    double tier4_2pc, tier4_4pc;
    double tier5_2pc, tier5_4pc;
    double tier6_2pc, tier6_4pc;
    double tier7_2pc, tier7_4pc;
    double tier8_2pc, tier8_4pc;
    void reset() { memset( ( void* ) this, 0x00, sizeof( cooldowns_t ) ); }
    cooldowns_t() { reset(); }
  };
  cooldowns_t cooldowns;

  struct uptimes_t
  {
    uptime_t* moving;
    uptime_t* replenishment;
    uptime_t* stunned;
    uptime_t *tier4_2pc, *tier4_4pc;
    uptime_t *tier5_2pc, *tier5_4pc;
    uptime_t *tier6_2pc, *tier6_4pc;
    uptime_t *tier7_2pc, *tier7_4pc;
    uptime_t *tier8_2pc, *tier8_4pc;
    void reset() { memset( ( void* ) this, 0x00, sizeof( uptimes_t ) ); }
    uptimes_t() { reset(); }
  };
  uptimes_t uptimes;

  struct gains_t
  {
    gain_t* ashtongue_talisman;
    gain_t* blessing_of_wisdom;
    gain_t* dark_rune;
    gain_t* energy_regen;
    gain_t* focus_regen;
    gain_t* innervate;
    gain_t* glyph_of_innervate;
    gain_t* judgement_of_wisdom;
    gain_t* mana_potion;
    gain_t* mana_spring;
    gain_t* mana_tide;
    gain_t* mp5_regen;
    gain_t* replenishment;
    gain_t* restore_mana;
    gain_t* spellsurge;
    gain_t* spirit_intellect_regen;
    gain_t* vampiric_touch;
    gain_t* water_elemental;
    gain_t *tier4_2pc, *tier4_4pc;
    gain_t *tier5_2pc, *tier5_4pc;
    gain_t *tier6_2pc, *tier6_4pc;
    gain_t *tier7_2pc, *tier7_4pc;
    gain_t *tier8_2pc, *tier8_4pc;
    void reset() { memset( ( void* ) this, 0x00, sizeof( gains_t ) ); }
    gains_t() { reset(); }
  };
  gains_t gains;

  struct procs_t
  {
    proc_t* ashtongue_talisman;
    proc_t* honor_among_thieves_donor;
    proc_t *tier4_2pc, *tier4_4pc;
    proc_t *tier5_2pc, *tier5_4pc;
    proc_t *tier6_2pc, *tier6_4pc;
    proc_t *tier7_2pc, *tier7_4pc;
    proc_t *tier8_2pc, *tier8_4pc;
    void reset() { memset( ( void* ) this, 0x00, sizeof( procs_t ) ); }
    procs_t() { reset(); }
  };
  procs_t procs;

  buff_list_t buff_list;

  rng_t* rng_list;

  struct rngs_t
  {
    rng_t* lag_channel;
    rng_t* lag_gcd;
    rng_t* lag_queue;
    rng_t* ashtongue_talisman;
    rng_t *tier4_2pc, *tier4_4pc;
    rng_t *tier5_2pc, *tier5_4pc;
    rng_t *tier6_2pc, *tier6_4pc;
    rng_t *tier7_2pc, *tier7_4pc;
    rng_t *tier8_2pc, *tier8_4pc;
    void reset() { memset( ( void* ) this, 0x00, sizeof( rngs_t ) ); }
    rngs_t() { reset(); }
  };
  rngs_t rngs;


  player_t( sim_t* sim, int type, const std::string& name );

  virtual ~player_t();

  virtual const char* name() { return name_str.c_str(); }
  virtual const char* id();

  virtual void init();
  virtual void init_base() = 0;
  virtual void init_core();
  virtual void init_race();
  virtual void init_spell();
  virtual void init_attack();
  virtual void init_defense();
  virtual void init_weapon( weapon_t*, std::string& );
  virtual void init_unique_gear();
  virtual void init_enchant();
  virtual void init_resources( bool force = false );
  virtual void init_consumables();
  virtual void init_professions();
  virtual void init_actions();
  virtual void init_rating();
  virtual void init_scaling();
  virtual void init_gains();
  virtual void init_procs();
  virtual void init_uptimes();
  virtual void init_rng();
  virtual void init_stats();

  virtual void reset();
  virtual void combat_begin();
  virtual void combat_end();

  virtual double composite_attack_power();
  virtual double composite_attack_crit();
  virtual double composite_attack_expertise()   { return attack_expertise;   }
  virtual double composite_attack_hit()         { return attack_hit;         }
  virtual double composite_attack_penetration() { return attack_penetration; }

  virtual double composite_armor();
  virtual double composite_armor_snapshot()     { return armor_snapshot; }

  virtual double composite_spell_power( int school );
  virtual double composite_spell_crit();
  virtual double composite_spell_hit()         { return spell_hit;         }
  virtual double composite_spell_penetration() { return spell_penetration; }

  virtual double composite_attack_power_multiplier();
  virtual double composite_spell_power_multiplier() { return spell_power_multiplier; }
  virtual double composite_attribute_multiplier( int attr );

  virtual double strength();
  virtual double agility();
  virtual double stamina();
  virtual double intellect();
  virtual double spirit();

  virtual void      interrupt();
  virtual void      clear_debuffs();
  virtual void      schedule_ready( double delta_time=0, bool waiting=false );
  virtual action_t* execute_action();

  virtual void   regen( double periodicity=2.0 );
  virtual double resource_gain( int resource, double amount, gain_t* g=0, action_t* a=0 );
  virtual double resource_loss( int resource, double amount, action_t* a=0 );
  virtual bool   resource_available( int resource, double cost );
  virtual int    primary_resource() { return RESOURCE_NONE; }
  virtual int    primary_role()     { return ROLE_HYBRID; }
  virtual int    primary_tree()     { return TALENT_TREE_MAX; }

  virtual void stat_gain( int stat, double amount );
  virtual void stat_loss( int stat, double amount );

  virtual void  summon_pet( const char* name );
  virtual void dismiss_pet( const char* name );

  virtual void register_callbacks();
  virtual void register_resource_gain_callback( int resource, action_callback_t* );
  virtual void register_resource_loss_callback( int resource, action_callback_t* );
  virtual void register_attack_result_callback( int result_mask, action_callback_t* );
  virtual void register_spell_result_callback ( int result_mask, action_callback_t* );
  virtual void register_tick_callback         ( action_callback_t* );
  virtual void register_tick_damage_callback  ( action_callback_t* );
  virtual void register_direct_damage_callback( action_callback_t* );

  virtual bool get_talent_trees( std::vector<int*>& tree1, std::vector<int*>& tree2, std::vector<int*>& tree3, talent_translation_t translation[][3] );
  virtual bool get_talent_trees( std::vector<int*>& tree1, std::vector<int*>& tree2, std::vector<int*>& tree3 );
  virtual bool parse_talents( std::vector<int*>& talent_tree, const std::string& talent_string );
  virtual bool parse_talents( const std::string& talent_string );
  virtual bool parse_talents_mmo( const std::string& talent_string );
  virtual bool parse_talents_wowhead( const std::string& talent_string );
  virtual bool parse_talents( const std::string& talent_string, int encoding );

  virtual std::vector<option_t>& get_options();
  virtual bool parse_option( const std::string& name, const std::string& value );

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name ) { return 0; }
  virtual pet_t*    find_pet     ( const std::string& name );

  virtual void trigger_replenishment();

  // Class-Specific Methods

  static player_t* create( sim_t* sim, const std::string& type, const std::string& name );

  static player_t * create_death_knight( sim_t* sim, const std::string& name );
  static player_t * create_druid       ( sim_t* sim, const std::string& name );
  static player_t * create_hunter      ( sim_t* sim, const std::string& name );
  static player_t * create_mage        ( sim_t* sim, const std::string& name );
  static player_t * create_paladin     ( sim_t* sim, const std::string& name );
  static player_t * create_priest      ( sim_t* sim, const std::string& name );
  static player_t * create_rogue       ( sim_t* sim, const std::string& name );
  static player_t * create_shaman      ( sim_t* sim, const std::string& name );
  static player_t * create_warlock     ( sim_t* sim, const std::string& name );
  static player_t * create_warrior     ( sim_t* sim, const std::string& name );

  bool is_pet() { return type == PLAYER_PET || type == PLAYER_GUARDIAN; }

  death_knight_t* cast_death_knight() { assert( type == DEATH_KNIGHT ); return ( death_knight_t* ) this; }
  druid_t       * cast_druid       () { assert( type == DRUID        ); return ( druid_t       * ) this; }
  hunter_t      * cast_hunter      () { assert( type == HUNTER       ); return ( hunter_t      * ) this; }
  mage_t        * cast_mage        () { assert( type == MAGE         ); return ( mage_t        * ) this; }
  paladin_t     * cast_paladin     () { assert( type == PALADIN      ); return ( paladin_t     * ) this; }
  priest_t      * cast_priest      () { assert( type == PRIEST       ); return ( priest_t      * ) this; }
  rogue_t       * cast_rogue       () { assert( type == ROGUE        ); return ( rogue_t       * ) this; }
  shaman_t      * cast_shaman      () { assert( type == SHAMAN       ); return ( shaman_t      * ) this; }
  warlock_t     * cast_warlock     () { assert( type == WARLOCK      ); return ( warlock_t     * ) this; }
  warrior_t     * cast_warrior     () { assert( type == WARRIOR      ); return ( warrior_t     * ) this; }
  pet_t         * cast_pet         () { assert( is_pet()             ); return ( pet_t         * ) this; }

  bool      in_gcd() { return gcd_ready > sim -> current_time; }
  bool      recent_cast();
  action_t* find_action( const std::string& );
  void      share_cooldown( const std::string& name, double ready );
  void      share_duration( const std::string& name, double ready );
  void      recalculate_haste();
  double    mana_regen_per_second();
  bool      dual_wield() { return main_hand_weapon.type != WEAPON_NONE && off_hand_weapon.type != WEAPON_NONE; }
  void      aura_gain( const char* name, int aura_id=0 );
  void      aura_loss( const char* name, int aura_id=0 );
  gain_t*   get_gain  ( const std::string& name );
  proc_t*   get_proc  ( const std::string& name );
  stats_t*  get_stats ( const std::string& name );
  uptime_t* get_uptime( const std::string& name );
  rng_t*    get_rng   ( const std::string& name, int type=RNG_DEFAULT );
};

// Pet =======================================================================

struct pet_t : public player_t
{
  std::string full_name_str;
  player_t* owner;
  pet_t* next_pet;

  double   stamina_per_owner;
  double intellect_per_owner;

  double summon_time;

  pet_t( sim_t* sim, player_t* owner, const std::string& name, bool guardian=false );

  virtual double composite_attack_hit() { return owner -> composite_attack_hit(); }
  virtual double composite_spell_hit()  { return owner -> composite_spell_hit();  }

  virtual double stamina();
  virtual double intellect();

  virtual void init();
  virtual void reset();
  virtual void summon();
  virtual void dismiss();

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual const char* name() { return full_name_str.c_str(); }
  virtual const char* id();
};

// Target ====================================================================

struct target_t
{
  sim_t* sim;
  std::string name_str, race_str, id_str;
  int race;
  int level;
  int spell_resistance[ SCHOOL_MAX ];
  int initial_armor, armor;
  int block_value;
  int shield;
  int vulnerable;
  int invulnerable;
  int casting;
  double initial_health, current_health;
  double total_dmg;
  uptime_t* uptime_list;

  struct cooldowns_t
  {
    double place_holder;
    void reset() { memset( ( void* ) this, 0x00, sizeof( cooldowns_t ) ); }
    cooldowns_t() { reset(); }
  };
  cooldowns_t cooldowns;

  struct debuff_t
  {
    int    bleeding;
    int    blood_frenzy;
    int    crypt_fever;
    int    curse_of_elements;
    double expose_armor;
    double faerie_fire;
    double frozen;
    double hemorrhage;
    int    hemorrhage_charges;
    double hunters_mark;
    int    improved_faerie_fire;
    int    improved_scorch;
    int    improved_shadow_bolt;
    int    judgement_of_wisdom;
    int    mangle;
    int    master_poisoner;
    int    misery;
    int    misery_stack;
    int    earth_and_moon;
    int    poisoned;
    int    savage_combat;
    int    slow;
    int    snare;
    double sunder_armor;
    int    trauma;
    int    thunder_clap;
    int    totem_of_wrath;
    int    winters_chill;
    int    winters_grasp;
    void reset() { memset( ( void* ) this, 0x0, sizeof( debuff_t ) ); }
    debuff_t() { reset(); }
    bool snared() { return frozen || slow || snare || thunder_clap; }
  };
  debuff_t debuffs;

  struct expirations_t
  {
    event_t* curse_of_elements;
    event_t* expose_armor;
    event_t* faerie_fire;
    event_t* frozen;
    event_t* earth_and_moon;
    event_t* hemorrhage;
    event_t* hunters_mark;
    event_t* improved_faerie_fire;
    event_t* improved_scorch;
    event_t* improved_shadow_bolt;
    event_t* nature_vulnerability;
    event_t* shadow_vulnerability;
    event_t* shadow_weaving;
    event_t* winters_chill;
    event_t* winters_grasp;
    void reset() { memset( ( void* ) this, 0x00, sizeof( expirations_t ) ); }
    expirations_t() { reset(); }
  };
  expirations_t expirations;

  struct uptimes_t
  {
    uptime_t* blood_frenzy;
    uptime_t* improved_scorch;
    uptime_t* improved_shadow_bolt;
    uptime_t* invulnerable;
    uptime_t* mangle;
    uptime_t* master_poisoner;
    uptime_t* savage_combat;
    uptime_t* trauma;
    uptime_t* totem_of_wrath;
    uptime_t* vulnerable;
    uptime_t* winters_chill;
    uptime_t* winters_grasp;
    void reset() { memset( ( void* ) this, 0x00, sizeof( uptimes_t ) ); }
    uptimes_t() { reset(); }
  };
  uptimes_t uptimes;

  target_t( sim_t* s );
  ~target_t();

  void init();
  void reset();
  void combat_begin();
  void combat_end();
  void assess_damage( double amount, int school, int type );
  void recalculate_health();
  double time_to_die();
  double health_percentage();
  double base_armor();
  uptime_t* get_uptime( const std::string& name );
  int get_options( std::vector<option_t>& );
  const char* name() { return name_str.c_str(); }
  const char* id();
};

// Stats =====================================================================

struct stats_t
{
  std::string name_str;
  sim_t* sim;
  player_t* player;
  stats_t* next;
  int school;
  bool channeled;
  bool analyzed;
  bool initialized;

  double resource_consumed;
  double frequency, num_executes, num_ticks;
  double total_execute_time, total_tick_time;
  double total_dmg, portion_dmg;
  double dps, portion_dps, dpe, dpet, dpr;
  double total_intervals, num_intervals;
  double last_execute;

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
  void add( double amount, int dmg_type, int result, double time );
  void init();
  void reset( action_t* );
  void analyze();
  void merge( stats_t* other );
  stats_t( const std::string& name, player_t* );
};

// Rank ======================================================================

struct rank_t
{
  int level, index;
  double dd_min, dd_max, tick, cost;
  int id;
};

// Base Stats ================================================================

struct base_stats_t
{
  int level;
  double health, mana;
  double strength, agility, stamina, intellect, spirit;
  double spell_crit, melee_crit;
};



// Action ====================================================================

struct action_t
{
  sim_t* sim;
  int type;
  std::string name_str;
  player_t* player;
  int id, school, resource, tree, result;
  bool dual, special, binary, channeled, background, repeating, aoe, harmful, proc;
  bool may_miss, may_resist, may_dodge, may_parry, may_glance, may_block, may_crush, may_crit;
  bool tick_may_crit, tick_zero, clip_dot;
  double min_gcd, trigger_gcd, range;
  double weapon_power_mod, direct_power_mod, tick_power_mod;
  double base_execute_time, base_tick_time, base_cost;
  double base_dd_min, base_dd_max, base_td, base_td_init;
  double base_dd_multiplier, base_td_multiplier;
  double   base_multiplier,   base_hit,   base_crit,   base_penetration;
  double player_multiplier, player_hit, player_crit, player_penetration;
  double target_multiplier, target_hit, target_crit, target_penetration;
  double   base_spell_power,   base_attack_power;
  double player_spell_power, player_attack_power;
  double target_spell_power, target_attack_power;
  double   base_spell_power_multiplier,   base_attack_power_multiplier;
  double player_spell_power_multiplier, player_attack_power_multiplier;
  double   base_crit_multiplier,   base_crit_bonus_multiplier, base_crit_bonus;
  double player_crit_multiplier, player_crit_bonus_multiplier;
  double target_crit_multiplier, target_crit_bonus_multiplier;
  double base_dd_adder, player_dd_adder, target_dd_adder;
  double resource_consumed;
  double direct_dmg, tick_dmg;
  double resisted_dmg, blocked_dmg;
  int num_ticks, current_tick, added_ticks;
  int ticking;
  std::string cooldown_group, duration_group;
  double cooldown, cooldown_ready, duration_ready;
  weapon_t* weapon;
  double weapon_multiplier;
  bool normalize_weapon_damage;
  bool normalize_weapon_speed;
  rng_t* rng[ RESULT_MAX ];
  rng_t* rng_travel;
  stats_t* stats;
  event_t* execute_event;
  event_t* tick_event;
  double time_to_execute, time_to_tick, time_to_travel, travel_speed;
  int rank_index, bloodlust_active;
  double min_current_time, max_current_time;
  double min_time_to_die, max_time_to_die;
  double min_health_percentage, max_health_percentage;
  int vulnerable, invulnerable, wait_on_ready,has_if_exp, is_ifall;
  act_expression_t* if_exp;
  std::string if_expression;
  std::string sync_str;
  action_t*   sync_action;
  action_t** observer;
  action_t* next;

  action_t( int type, const char* name, player_t* p=0, int r=RESOURCE_NONE, int s=SCHOOL_NONE, int t=TREE_NONE, bool special=false );
  virtual ~action_t() {}

  virtual void      base_parse_options( option_t*, const std::string& options_str );
  virtual void      parse_options( option_t*, const std::string& options_str );
  virtual option_t* merge_options( std::vector<option_t>&, option_t*, option_t* );
  virtual rank_t*   init_rank( rank_t* rank_list, int id=0 );

  virtual double cost();
  virtual double haste()        { return 1.0;               }
  virtual double gcd()          { return trigger_gcd;       }
  virtual double execute_time() { return base_execute_time; }
  virtual double tick_time()    { return base_tick_time;    }
  virtual double travel_time();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual void   calculate_result() { assert( 0 ); }
  virtual bool   result_is_hit();
  virtual bool   result_is_miss();
  virtual double calculate_direct_damage();
  virtual double calculate_tick_damage();
  virtual double calculate_weapon_damage();
  virtual double armor();
  virtual double resistance();
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   tick();
  virtual void   last_tick();
  virtual void   travel( int result, double dmg );
  virtual void   assess_damage( double amount, int dmg_type );
  virtual void   schedule_execute();
  virtual void   schedule_tick();
  virtual void   schedule_travel();
  virtual void   refresh_duration();
  virtual void   extend_duration( int extra_ticks );
  virtual void   update_ready();
  virtual void   update_stats( int type );
  virtual void   update_time( int type );
  virtual bool   ready();
  virtual void   reset();
  virtual void   cancel();
  virtual const char* name() { return name_str.c_str(); }

  virtual double total_multiplier() { return   base_multiplier * player_multiplier * target_multiplier; }
  virtual double total_hit()        { return   base_hit        + player_hit        + target_hit;        }
  virtual double total_crit()       { return   base_crit       + player_crit       + target_crit;       }
  virtual double total_crit_bonus();

  virtual double total_spell_power()  { return ( base_spell_power  + player_spell_power  + target_spell_power  ) * base_spell_power_multiplier  * player_spell_power_multiplier;  }
  virtual double total_attack_power() { return ( base_attack_power + player_attack_power + target_attack_power ) * base_attack_power_multiplier * player_attack_power_multiplier; }
  virtual double total_power();

  // Some actions require different multipliers for the "direct" and "tick" portions.

  virtual double total_dd_multiplier() { return total_multiplier() * base_dd_multiplier; }
  virtual double total_td_multiplier() { return total_multiplier() * base_td_multiplier; }
};

// Attack ====================================================================

struct attack_t : public action_t
{
  double base_expertise, player_expertise, target_expertise;

  attack_t( const char* n=0, player_t* p=0, int r=RESOURCE_NONE, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=false );
  virtual ~attack_t() {}

  // Attack Overrides
  virtual double haste();
  virtual double execute_time();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual int    build_table( double* chances, int* results );
  virtual void   calculate_result();
  virtual void   execute();

  // Attack Specific
  virtual double   miss_chance( int delta_level );
  virtual double  dodge_chance( int delta_level );
  virtual double  parry_chance( int delta_level );
  virtual double glance_chance( int delta_level );
  virtual double  block_chance( int delta_level );
  virtual double   crit_chance( int delta_level );
  virtual double total_expertise() { return base_expertise + player_expertise + target_expertise; }
};

// Spell =====================================================================

struct spell_t : public action_t
{
  spell_t( const char* n=0, player_t* p=0, int r=RESOURCE_NONE, int s=SCHOOL_PHYSICAL, int t=TREE_NONE );
  virtual ~spell_t() {}

  // Spell Overrides
  virtual double haste();
  virtual double gcd();
  virtual double execute_time();
  virtual double tick_time();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual double level_based_miss_chance( int player, int target );
  virtual void   calculate_result();
  virtual void   execute();
};

// Sequence ==================================================================

struct sequence_t : public action_t
{
  std::vector<action_t*> sub_actions;
  int current_action;

  sequence_t( const char* name, player_t*, const std::string& sub_action_str );
  virtual ~sequence_t();
  virtual void schedule_execute();
  virtual void reset();
  virtual bool ready();
  virtual void restart() { current_action=0; }
};

// Action Callback ===========================================================

struct action_callback_t
{
  sim_t* sim;
  player_t* listener;
  action_callback_t( sim_t* s, player_t* l ) : sim( s ), listener( l ) {}
  virtual ~action_callback_t() {}
  virtual void trigger( action_t*, void* call_data=0 ) = 0;
  virtual void reset() {}
  static void trigger( std::vector<action_callback_t*>& v, action_t* a, void* call_data=0 )
  {
    std::vector<action_callback_t*>::size_type i = v.size();
    while ( i ) v[--i]->trigger( a, call_data );
  }
  static void   reset( std::vector<action_callback_t*>& v )
  {
    std::vector<action_callback_t*>::size_type i = v.size();
    while ( i ) v[--i]->reset();
  }
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

// Action Travel Event =======================================================

struct action_travel_event_t : public event_t
{
  action_t* action;
  int result;
  double damage;
  action_travel_event_t( sim_t* sim, action_t* a, double time_to_travel );
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
  // Tier Gear
  int  ashtongue_talisman;
  int  tier4_2pc, tier4_4pc;
  int  tier5_2pc, tier5_4pc;
  int  tier6_2pc, tier6_4pc;
  int  tier7_2pc, tier7_4pc;
  int  tier8_2pc, tier8_4pc;

  // Specific Gear
  int  bandits_insignia;
  int  blood_of_the_old_god;
  int  chaotic_skyflare;
  int  comets_trail;
  int  darkmoon_card_crusade;
  int  darkmoon_card_greatness;
  int  dark_matter;
  int  dying_curse;
  int  egg_of_mortal_essence;
  int  elder_scribes;
  int  elemental_focus_stone;
  int  ember_skyflare;
  int  embrace_of_the_spider;
  int  eternal_sage;
  int  extract_of_necromantic_power;
  int  eye_of_magtheridon;
  int  eye_of_the_broodmother;
  int  flare_of_the_heavens;
  int  forge_ember;
  int  fury_of_the_five_flights;
  int  grim_toll;
  int  illustration_of_the_dragon_soul;
  int  lightning_capacitor;
  int  mark_of_defiance;
  int  mirror_of_truth;
  int  mjolnir_runestone;
  int  mystical_skyfire;
  int  pandoras_plea;
  int  pyrite_infuser;
  int  quagmirrans_eye;
  int  relentless_earthstorm;
  int  sextant_of_unstable_currents;
  int  shiffars_nexus_horn;
  int  spellstrike;
  int  sundial_of_the_exiled;
  int  thunder_capacitor;
  int  timbals_crystal;
  int  wrath_of_cenarius;
  int  austere_earthsiege;
  int  lightweave_embroidery;

  unique_gear_t() { memset( ( void* ) this, 0x00, sizeof( unique_gear_t ) ); }

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
  static int get_options( std::vector<option_t>&, player_t* );
  static void init( player_t* );
  static void register_callbacks( player_t* );
};

// Enchants ===================================================================

struct enchant_t
{
  gear_stats_t stats;
  int spellsurge;

  enchant_t() { memset( ( void* ) this, 0x00, sizeof( enchant_t ) ); }

  static int get_options( std::vector<option_t>&, player_t* );
  static void init( player_t* );
  static void register_callbacks( player_t* );
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
  double actual, overflow;
  int id;
  gain_t* next;
  gain_t( const std::string& n, int _id=0 ) : name_str( n ), actual( 0 ), overflow( 0 ), id( _id ) {}
  void add( double a, double o=0 ) { actual += a; overflow += o; }
  void merge( gain_t* other ) { actual += other -> actual; overflow += other -> overflow; }
  const char* name() { return name_str.c_str(); }
};

// Proc ======================================================================

struct proc_t
{
  std::string name_str;
  double count;
  double frequency;
  proc_t* next;
  proc_t( const std::string& n ) : name_str( n ), count( 0 ), frequency( 0 ) {}
  void occur() { count++; }
  void merge( proc_t* other ) { count += other -> count; }
  const char* name() { return name_str.c_str(); }
};

// Uptime ======================================================================

struct uptime_t
{
  std::string name_str;
  uint64_t up, down;
  uptime_t* next;
  int type;
  sim_t* sim;
  double last_check;
  double total_time;
  bool last_status;
  double up_time;
  int n_rewind, n_up, n_down;
  double n_triggers;
  double avg_up, avg_dur;
  uptime_t( const std::string& n, sim_t* the_sim=0 );
  void   update( bool is_up, bool skip_usage=false );
  double percentage(int p_type=0);
  void   merge( uptime_t* other );
  const char* name();
  void rewind();
};


// Report =====================================================================

struct report_t
{
  static void print_profiles( sim_t* );
  static void print_text( FILE*, sim_t*, bool detail=true );
  static void print_html( sim_t* );
  static void print_wiki( sim_t* );
  static void print_xml( sim_t* );
  static void print_suite( sim_t* );
};

// Chart ======================================================================

struct chart_t
{
  static int raid_dps ( std::vector<std::string>& images, sim_t* );
  static int raid_dpet( std::vector<std::string>& images, sim_t* );
  static int raid_gear( std::vector<std::string>& images, sim_t* );

  static const char* raid_downtime    ( std::string& s, sim_t* );
  static const char* raid_uptimes     ( std::string& s, sim_t* );
  static const char* action_dpet      ( std::string& s, player_t* );
  static const char* action_dmg       ( std::string& s, player_t* );
  static const char* gains            ( std::string& s, player_t* );
  static const char* uptimes_and_procs( std::string& s, player_t* );
  static const char* timeline_resource( std::string& s, player_t* );
  static const char* timeline_dps     ( std::string& s, player_t* );
  static const char* distribution_dps ( std::string& s, player_t* );

  static const char* gear_weights_lootrank( std::string& s, player_t* );
  static const char* gear_weights_wowhead ( std::string& s, player_t* );
  static const char* gear_weights_pawn ( std::string& s, player_t* );
};

// Talent Translation =========================================================

struct talent_translation_t
{
  int  index;
  int* address;
};

// Log =======================================================================

struct log_t
{
  // Generic Output
  static void output( sim_t*, const char* format, ... );
  // Combat Log
  static void start_event( action_t* );
  static void damage_event( action_t*, double dmg, int dmg_type );
  static void miss_event( action_t* );
  static void resource_gain_event( player_t*, int resource, double amount, double actual_amount, gain_t* source );
  static void aura_gain_event( player_t*, const char* name, int id );
  static void aura_loss_event( player_t*, const char* name, int id );
  static void summon_event( pet_t* );
};

// Pseudo Random Number Generation ===========================================

struct rng_t
{
  std::string name_str;
  bool   gauss_pair_use;
  double gauss_pair_value;
  double expected_roll,  actual_roll,  num_roll;
  double expected_range, actual_range, num_range;
  double expected_gauss, actual_gauss, num_gauss;
  bool   average_range, average_gauss;
  rng_t* next;

  rng_t( const std::string& n, bool avg_range=false, bool avg_gauss=false );
  virtual ~rng_t() {}

  virtual int    type() { return RNG_STANDARD; }
  virtual double real();
  virtual int    roll( double chance );
  virtual double range( double min, double max );
  virtual double gauss( double mean, double stddev );
  virtual void   seed( uint32_t start );
  virtual void   report( FILE* );

  static rng_t* create( sim_t*, const std::string& name, int type=RNG_STANDARD );
};

void testRng( sim_t* sim );

// Utilities =================================================================

struct util_t
{
  static double talent_rank( int num, int max, double increment );
  static double talent_rank( int num, int max, double value1, double value2, ... );

  static int talent_rank( int num, int max, int increment );
  static int talent_rank( int num, int max, int value1, int value2, ... );

  static double ability_rank( int player_level, double ability_value, int ability_level, ... );
  static int    ability_rank( int player_level, int    ability_value, int ability_level, ... );

  static char* dup( const char* );

  static const char* attribute_type_string     ( int type );
  static const char* dmg_type_string           ( int type );
  static const char* elixir_type_string        ( int type );
  static const char* flask_type_string         ( int type );
  static const char* food_type_string          ( int type );
  static const char* player_type_string        ( int type );
  static const char* profession_type_string    ( int type );
  static const char* race_type_string          ( int type );
  static const char* resource_type_string      ( int type );
  static const char* result_type_string        ( int type );
  static const char* school_type_string        ( int type );
  static const char* stat_type_string          ( int type );
  static const char* stat_type_abbrev          ( int type );
  static const char* talent_tree_string        ( int tree );
  static const char* weapon_buff_type_string   ( int type );
  static const char* weapon_enchant_type_string( int type );
  static const char* weapon_type_string        ( int type );

  static int string_split( std::vector<std::string>& results, const std::string& str, const char* delim );
  static int string_split( const std::string& str, const char* delim, const char* format, ... );

  static int milliseconds();
};

std::string tolower( std::string src );
std::string trim( std::string src );
void replace_char( std::string& src, char old_c, char new_c  );
void replace_str( std::string& src, std::string old_str, std::string new_str  );
std::string proper_option_name( const std::string& full_name );

bool armory_option_parse( sim_t* sim, const std::string& name, const std::string& value);


#endif // __SIMCRAFT_H

