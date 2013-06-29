// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

struct death_knight_t;

namespace pets {
struct dancing_rune_weapon_pet_t;
}

// ==========================================================================
// Death Knight Runes
// ==========================================================================

enum rune_type
{
  RUNE_TYPE_NONE = 0, RUNE_TYPE_BLOOD, RUNE_TYPE_FROST, RUNE_TYPE_UNHOLY, RUNE_TYPE_DEATH, RUNE_TYPE_WASDEATH = 8
};

const char * const rune_symbols = "!bfu!!";

#define RUNE_TYPE_MASK     3
#define RUNE_SLOT_MAX      6

#define RUNIC_POWER_REFUND  0.9

// These macros simplify using the result of count_runes(), which
// returns a number of the form 0x000AABBCC where AA is the number of
// Unholy runes, BB is the number of Frost runes, and CC is the number
// of Blood runes.
#define GET_BLOOD_RUNE_COUNT(x)  ((x >>  0) & 0xff)
#define GET_FROST_RUNE_COUNT(x)  ((x >>  8) & 0xff)
#define GET_UNHOLY_RUNE_COUNT(x) ((x >> 16) & 0xff)
#define GET_DEATH_RUNE_COUNT(x)  ((x >> 24) & 0xff)

enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

struct dk_rune_t
{
  death_knight_t* dk;
  int        type;
  rune_state state;
  double     value;   // 0.0 to 1.0, with 1.0 being full
  bool       permanent_death_rune;
  dk_rune_t* paired_rune;
  int        slot_number;

  dk_rune_t() : type( RUNE_TYPE_NONE ), state( STATE_FULL ), value( 0.0 ), permanent_death_rune( false ), paired_rune( NULL ), slot_number( 0 ) {}

  bool is_death()        { return ( type & RUNE_TYPE_DEATH ) != 0                ; }
  bool is_blood()        { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_BLOOD  ; }
  bool is_unholy()       { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_UNHOLY ; }
  bool is_frost()        { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_FROST  ; }
  bool is_ready()        { return state == STATE_FULL                            ; }
  bool is_depleted()     { return state == STATE_DEPLETED                        ; }
  int  get_type()        { return type & RUNE_TYPE_MASK                          ; }

  void regen_rune( death_knight_t* p, timespan_t periodicity, bool rc = false );

  void make_permanent_death_rune()
  {
    permanent_death_rune = true;
    type |= RUNE_TYPE_DEATH;
  }

  void consume( bool convert )
  {
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
    else
    {
      type = ( type & RUNE_TYPE_MASK ) | ( ( type << 1 ) & RUNE_TYPE_WASDEATH ) | ( convert ? RUNE_TYPE_DEATH : 0 );
    }
    value = 0.0;
    state = STATE_DEPLETED;
  }

  void fill_rune();

  void reset()
  {
    value = 1.0;
    state = STATE_FULL;
    type = type & RUNE_TYPE_MASK;
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
  }
};

// ==========================================================================
// Death Knight
// ==========================================================================

struct death_knight_td_t : public actor_pair_t
{
  dot_t* dots_blood_plague;
  dot_t* dots_death_and_decay;
  dot_t* dots_frost_fever;
  dot_t* dots_soul_reaper;

  debuff_t* debuffs_frost_vulnerability;

  int diseases() const
  {
    int disease_count = 0;
    if ( dots_blood_plague -> ticking ) disease_count++;
    if ( dots_frost_fever  -> ticking ) disease_count++;
    return disease_count;
  }

  death_knight_td_t( player_t* target, death_knight_t* death_knight );
};

enum death_knight_presence { PRESENCE_BLOOD = 1, PRESENCE_FROST, PRESENCE_UNHOLY = 4 };

struct death_knight_t : public player_t
{
public:
  // Track healing for Death Strike
  std::vector<std::pair<timespan_t, double> > incoming_damage;

  // Active
  int       active_presence;
  int       t16_tank_2pc_driver;

  // Buffs
  struct buffs_t
  {
    buff_t* antimagic_shell;
    buff_t* blood_charge;
    buff_t* blood_presence;
    buff_t* bone_wall;
    buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* dark_transformation;
    buff_t* deathbringer;
    buff_t* frost_presence;
    buff_t* icebound_fortitude;
    buff_t* killing_machine;
    buff_t* pillar_of_frost;
    buff_t* rime;
    buff_t* rune_strike;
    buff_t* runic_corruption;
    buff_t* scent_of_blood;
    buff_t* shadow_infusion;
    buff_t* sudden_doom;
    buff_t* tier13_4pc_melee;
    buff_t* unholy_presence;
    buff_t* vampiric_blood;
    buff_t* will_of_the_necropolis_dr;
    buff_t* will_of_the_necropolis_rt;

    absorb_buff_t* blood_shield;
    stat_buff_t* death_shroud;
  } buffs;

  struct runeforge_t
  {
    buff_t* rune_of_cinderglacier;
    buff_t* rune_of_the_fallen_crusader;
    buff_t* rune_of_the_stoneskin_gargoyle;
    buff_t* rune_of_the_nerubian_carapace;
    buff_t* rune_of_the_nerubian_carapace_oh;
    buff_t* rune_of_spellshattering;
    buff_t* rune_of_spellbreaking;
    buff_t* rune_of_spellbreaking_oh;
    buff_t* rune_of_swordshattering;
    buff_t* rune_of_swordbreaking;
    buff_t* rune_of_swordbreaking_oh;
  } runeforge;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* bone_shield_icd;
  } cooldown;

  // Diseases

  struct active_spells_t
  {
    action_t* blood_caked_blade;
    spell_t* blood_plague;
    spell_t* frost_fever;
  } active_spells;

  // Gains
  struct gains_t
  {
    gain_t* antimagic_shell;
    gain_t* butchery;
    gain_t* chill_of_the_grave;
    gain_t* frost_presence;
    gain_t* horn_of_winter;
    gain_t* improved_frost_presence;
    gain_t* power_refund;
    gain_t* scent_of_blood;
    gain_t* rune;
    gain_t* rune_unholy;
    gain_t* rune_blood;
    gain_t* rune_frost;
    gain_t* rc_unholy;
    gain_t* rc_blood;
    gain_t* rc_frost;
    gain_t* rc;
    gain_t* rune_unknown;
    gain_t* runic_empowerment;
    gain_t* runic_empowerment_blood;
    gain_t* runic_empowerment_unholy;
    gain_t* runic_empowerment_frost;
    gain_t* empower_rune_weapon;
    gain_t* blood_tap;
    gain_t* blood_tap_blood;
    gain_t* blood_tap_frost;
    gain_t* blood_tap_unholy;
    gain_t* plague_leech;
    gain_t* hp_death_siphon;
    gain_t* t15_4pc_tank;
  } gains;

  // Options
  std::string unholy_frenzy_target_str;

  // Specialization
  struct specialization_t
  {
    // Generic
    const spell_data_t* plate_specialization;

    // Blood
    const spell_data_t* blood_parasite;
    const spell_data_t* blood_rites;
    const spell_data_t* veteran_of_the_third_war;
    const spell_data_t* scent_of_blood;
    const spell_data_t* improved_blood_presence;
    const spell_data_t* scarlet_fever;
    const spell_data_t* crimson_scourge;
    const spell_data_t* sanguine_fortitude;
    const spell_data_t* will_of_the_necropolis;

    // Frost
    const spell_data_t* blood_of_the_north;
    const spell_data_t* icy_talons;
    const spell_data_t* killing_machine;
    const spell_data_t* improved_frost_presence;
    const spell_data_t* brittle_bones;
    const spell_data_t* rime;
    const spell_data_t* might_of_the_frozen_wastes;
    const spell_data_t* threat_of_thassarian;

    // Unholy
    const spell_data_t* master_of_ghouls;
    const spell_data_t* reaping;
    const spell_data_t* unholy_might;
    const spell_data_t* shadow_infusion;
    const spell_data_t* sudden_doom;
    const spell_data_t* ebon_plaguebringer;
    const spell_data_t* improved_unholy_presence;
  } spec;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* blood_shield;
    const spell_data_t* frozen_heart;
    const spell_data_t* dreadblade;
  } mastery;

  // Talents
  struct talents_t
  {
    const spell_data_t* roiling_blood;
    const spell_data_t* plague_leech;
    const spell_data_t* unholy_blight;

    const spell_data_t* death_siphon;

    const spell_data_t* blood_tap;
    const spell_data_t* runic_empowerment;
    const spell_data_t* runic_corruption;
  } talent;

  // Spells
  struct spells_t
  {
    const spell_data_t* blood_parasite;
    const spell_data_t* t15_4pc_tank;
  } spell;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* chains_of_ice;
    const spell_data_t* dancing_rune_weapon;
    const spell_data_t* enduring_infection;
    const spell_data_t* icebound_fortitude;
    const spell_data_t* outbreak;
    const spell_data_t* shifting_presences;
    const spell_data_t* vampiric_blood;
  } glyph;

  // Pets and Guardians
  struct pets_t
  {
    std::array< pet_t*, 8 > army_ghoul;
    std::array< pet_t*, 10 > fallen_zandalari;
    std::array< pet_t*, 10 > bloodworms;
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon;
    pet_t* ghoul_pet;
    pet_t* ghoul_guardian;
    pet_t* gargoyle;
  } pets;

  // Procs
  struct procs_t
  {
    proc_t* blood_parasite;
    proc_t* runic_empowerment;
    proc_t* runic_empowerment_wasted;
    proc_t* oblit_killing_machine;
    proc_t* fs_killing_machine;
    proc_t* sr_killing_machine;
    proc_t* t15_2pc_melee;

    proc_t* ready_blood;
    proc_t* ready_frost;
    proc_t* ready_unholy;
  } procs;

  // RNGs
  struct rngs_t
  {
    rng_t* blood_caked_blade;
    rng_t* blood_parasite;
    rng_t* blood_tap;
    rng_t* plague_leech;
    rng_t* rime;
    rng_t* sudden_doom;
    rng_t* t13_2pc_melee;

    real_ppm_t* t15_2pc_melee;
  } rng;

  // Runes
  struct runes_t
  {
    std::array<dk_rune_t, RUNE_SLOT_MAX> slot;

    runes_t()
    {
      // 6 runes, paired blood, frost and unholy
      slot[0].type = slot[1].type = RUNE_TYPE_BLOOD;
      slot[2].type = slot[3].type = RUNE_TYPE_FROST;
      slot[4].type = slot[5].type = RUNE_TYPE_UNHOLY;
      // each rune pair is paired with each other
      slot[0].paired_rune = &slot[ 1 ]; slot[ 1 ].paired_rune = &slot[ 0 ];
      slot[2].paired_rune = &slot[ 3 ]; slot[ 3 ].paired_rune = &slot[ 2 ];
      slot[4].paired_rune = &slot[ 5 ]; slot[ 5 ].paired_rune = &slot[ 4 ];
      // give each rune a slot number
      for ( size_t i = 0; i < slot.size(); ++i ) { slot[ i ].slot_number = static_cast<int>( i ); }
    }
    void reset() { for ( size_t i = 0; i < slot.size(); ++i ) slot[ i ].reset(); }
  } _runes;

  death_knight_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    active_presence(),
    buffs( buffs_t() ),
    runeforge( runeforge_t() ),
    active_spells( active_spells_t() ),
    gains( gains_t() ),
    spec( specialization_t() ),
    mastery( mastery_t() ),
    talent( talents_t() ),
    spell( spells_t() ),
    glyph( glyphs_t() ),
    pets( pets_t() ),
    procs( procs_t() ),
    rng( rngs_t() ),
    _runes( runes_t() )
  {
    range::fill( pets.army_ghoul, nullptr );
    base.distance = 0;

    cooldown.bone_shield_icd = get_cooldown( "bone_shield_icd" );
    cooldown.bone_shield_icd -> duration = timespan_t::from_seconds( 2.0 );
    for ( size_t i = 0; i < _runes.slot.size(); i++ )
      _runes.slot[ i ].dk = this;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_actions();
  virtual void      init_enchant();
  virtual void      init_rng();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_resources( bool force );
  virtual double    composite_armor_multiplier();
  virtual double    composite_melee_speed();
  virtual double    composite_melee_haste();
  virtual double    composite_attribute_multiplier( attribute_e attr );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual double    composite_parry();
  virtual double    composite_player_multiplier( school_e school );
  virtual double    composite_crit_avoidance();
  virtual void      regen( timespan_t periodicity );
  virtual void      reset();
  virtual void      arise();
  virtual void      assess_damage( school_e, dmg_e, action_state_t* );
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual void      combat_begin();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_RUNIC_POWER; }
  virtual role_e primary_role();
  virtual void      trigger_runic_empowerment();
  virtual int       runes_count( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_any( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_all( rune_type rt, bool include_death, int position );
  virtual double    runes_cooldown_time( dk_rune_t* r );
  virtual bool      runes_depleted( rune_type rt, int position );
  virtual void invalidate_cache( cache_e );

  virtual ~death_knight_t();

  target_specific_t<death_knight_td_t*> target_data;

  virtual death_knight_td_t* get_target_data( player_t* target )
  {
    death_knight_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new death_knight_td_t( target, this );
    }
    return td;
  }
};

death_knight_t::~death_knight_t()
{
  delete rng.t15_2pc_melee;
}

inline death_knight_td_t::death_knight_td_t( player_t* target, death_knight_t* death_knight ) :
  actor_pair_t( target, death_knight )
{
  dots_blood_plague    = target -> get_dot( "blood_plague",    death_knight );
  dots_death_and_decay = target -> get_dot( "death_and_decay", death_knight );
  dots_frost_fever     = target -> get_dot( "frost_fever",     death_knight );
  dots_soul_reaper     = target -> get_dot( "soul_reaper_dot", death_knight );

  debuffs_frost_vulnerability = buff_creator_t( *this, "frost_vulnerability", death_knight -> find_spell( 51714 ) );
}

inline void dk_rune_t::fill_rune()
{
  if ( state != STATE_FULL )
  {
    if ( is_blood() )
      dk -> procs.ready_blood -> occur();
    else if ( is_frost() )
      dk -> procs.ready_frost -> occur();
    else if ( is_unholy() )
      dk -> procs.ready_unholy -> occur();
  }
  value = 1.0;
  state = STATE_FULL;
}

// ==========================================================================
// Local Utility Functions
// ==========================================================================

static void extract_rune_cost( const spell_data_t* spell, int* cost_blood, int* cost_frost, int* cost_unholy, int* cost_death )
{
  // Rune costs appear to be in binary: 0a0b0c0d where 'd' is whether the ability
  // costs a blood rune, 'c' is whether it costs an unholy rune, 'b'
  // is whether it costs a frost rune, and 'a' is whether it costs a  death
  // rune

  if ( ! spell -> ok() ) return;

  uint32_t rune_cost = spell -> rune_cost();
  *cost_blood  =          rune_cost & 0x1;
  *cost_unholy = ( rune_cost >> 2 ) & 0x1;
  *cost_frost  = ( rune_cost >> 4 ) & 0x1;
  *cost_death  = ( rune_cost >> 6 ) & 0x1;
}

// Log rune status ==========================================================

static void log_rune_status( death_knight_t* p )
{
  std::string rune_str;
  std::string runeval_str;
  for ( int j = 0; j < RUNE_SLOT_MAX; ++j )
  {
    char rune_letter = rune_symbols[p -> _runes.slot[j].get_type()];
    std::string runeval = util::to_string( p -> _runes.slot[j].value, 2 );

    if ( p -> _runes.slot[j].is_death() )
      rune_letter = 'd';

    if ( p -> _runes.slot[j].is_ready() )
      rune_letter = toupper( rune_letter );

    rune_str += rune_letter;
    runeval_str += '[' + runeval + ']';
  }
  p -> sim -> output( "%s runes: %s %s", p -> name(), rune_str.c_str(), runeval_str.c_str() );
}

// Count Death Runes ========================================================

static int count_death_runes( death_knight_t* p, bool inactive )
{
  // Getting death rune count is a bit more complicated as it depends
  // on talents which runetype can be converted to death runes
  int count = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[ i ];
    if ( ( inactive || r.is_ready() ) && r.is_death() )
      ++count;
  }
  return count;
}

// Consume Runes ============================================================

static void consume_runes( death_knight_t* player, const bool use[RUNE_SLOT_MAX], bool convert_runes = false )
{
  if ( player -> sim -> log )
  {
    log_rune_status( player );
  }

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    if ( use[i] )
    {
      // Show the consumed type of the rune
      // Not the type it is after consumption
      int consumed_type = player -> _runes.slot[i].type;
      player -> _runes.slot[i].consume( convert_runes );

      if ( player -> sim -> log )
        player -> sim -> output( "%s consumes rune #%d, type %d", player -> name(), i, consumed_type );
    }
  }

  if ( player -> sim -> log )
  {
    log_rune_status( player );
  }
}

// Group Runes ==============================================================

static int use_rune( death_knight_t* p, rune_type rt, bool use[ RUNE_SLOT_MAX ] )
{
  dk_rune_t* r = 0;
  if ( rt == RUNE_TYPE_BLOOD )
    r = &( p -> _runes.slot[ 0 ] );
  else if ( rt == RUNE_TYPE_FROST )
    r = &( p -> _runes.slot[ 2 ] );
  else if ( rt == RUNE_TYPE_UNHOLY )
    r = &( p -> _runes.slot[ 4 ] );

  // 1) Choose first non-death rune of rune_type
  if ( r && ! use[ r -> slot_number ] && r -> is_ready() && ! r -> is_death() )
    return r -> slot_number;
  // 2) Choose paired non-death rune of rune_type
  else if ( r && ! use[ r -> paired_rune -> slot_number ] && r -> paired_rune -> is_ready() && ! r -> paired_rune -> is_death() )
    return r -> paired_rune -> slot_number;
  // 3) Choose first death rune of rune_type
  else if ( r && ! use[ r -> slot_number ] && r -> is_ready() && r -> is_death() )
    return r -> slot_number;
  // 4) Choose paired death rune of rune_type
  else if ( r && ! use[ r -> paired_rune -> slot_number ] && r -> paired_rune -> is_ready() && r -> paired_rune -> is_death() )
    return r -> paired_rune -> slot_number;
  // 5) If the ability uses a death rune, use custom order of f > b > u to pick
  // the death rune
  else if ( rt == RUNE_TYPE_DEATH )
  {
    if ( ! use[ 2 ] && p -> _runes.slot[ 2 ].is_ready() && p -> _runes.slot[ 2 ].is_death() )
      return 2;
    else if ( ! use[ 3 ] && p -> _runes.slot[ 3 ].is_ready() && p -> _runes.slot[ 3 ].is_death() )
      return 3;
    else if ( ! use[ 0 ] && p -> _runes.slot[ 0 ].is_ready() && p -> _runes.slot[ 0 ].is_death() )
      return 0;
    else if ( ! use[ 1 ] && p -> _runes.slot[ 1 ].is_ready() && p -> _runes.slot[ 1 ].is_death() )
      return 1;
    else if ( ! use[ 4 ] && p -> _runes.slot[ 4 ].is_ready() && p -> _runes.slot[ 4 ].is_death() )
      return 4;
    else if ( ! use[ 5 ] && p -> _runes.slot[ 5 ].is_ready() && p -> _runes.slot[ 5 ].is_death() )
      return 5;
  }
  // 6) Choose the first death rune of any type, in the order b > u > f
  else
  {
    if ( ! use[ 0 ] && p -> _runes.slot[ 0 ].is_ready() && p -> _runes.slot[ 0 ].is_death() )
      return 0;
    else if ( ! use[ 1 ] && p -> _runes.slot[ 1 ].is_ready() && p -> _runes.slot[ 1 ].is_death() )
      return 1;
    else if ( ! use[ 4 ] && p -> _runes.slot[ 4 ].is_ready() && p -> _runes.slot[ 4 ].is_death() )
      return 4;
    else if ( ! use[ 5 ] && p -> _runes.slot[ 5 ].is_ready() && p -> _runes.slot[ 5 ].is_death() )
      return 5;
    else if ( ! use[ 2 ] && p -> _runes.slot[ 2 ].is_ready() && p -> _runes.slot[ 2 ].is_death() )
      return 2;
    else if ( ! use[ 3 ] && p -> _runes.slot[ 3 ].is_ready() && p -> _runes.slot[ 3 ].is_death() )
      return 3;
  }

  // 7) No rune found
  return -1;
}

static bool group_runes ( death_knight_t* player, int blood, int frost, int unholy, int death, bool group[ RUNE_SLOT_MAX ] )
{
  assert( blood < 2 && frost < 2 && unholy < 2 && death < 2 );

  bool use[ RUNE_SLOT_MAX ] = { false };
  int use_slot = -1;

  if ( blood )
  {
    if ( ( use_slot = use_rune( player, RUNE_TYPE_BLOOD, use ) ) == -1 )
      return false;
    else
    {
      assert( ! use[ use_slot ] );
      use[ use_slot ] = true;
    }
  }

  if ( frost )
  {
    if ( ( use_slot = use_rune( player, RUNE_TYPE_FROST, use ) ) == -1 )
      return false;
    else
    {
      assert( ! use[ use_slot ] );
      use[ use_slot ] = true;
    }
  }

  if ( unholy )
  {
    if ( ( use_slot = use_rune( player, RUNE_TYPE_UNHOLY, use ) ) == -1 )
      return false;
    else
    {
      assert( ! use[ use_slot ] );
      use[ use_slot ] = true;
    }
  }

  if ( death )
  {
    if ( ( use_slot = use_rune( player, RUNE_TYPE_DEATH, use ) ) == -1 )
      return false;
    else
    {
      assert( ! use[ use_slot ] );
      use[ use_slot ] = true;
    }
  }

  // Storing rune slots selected
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) group[ i ] = use[ i ];

  return true;
}

// Select a "random" fully depleted rune ====================================

static int random_depleted_rune( death_knight_t* p )
{
  int num_depleted = 0;
  int depleted_runes[ RUNE_SLOT_MAX ] = { 0 };

  // Blood prefers Blood runes
  if ( p -> specialization() == DEATH_KNIGHT_BLOOD )
  {
    if ( p -> _runes.slot[ 0 ].is_depleted() )
      depleted_runes[ num_depleted++] = 0;

    if ( p -> _runes.slot[ 1 ].is_depleted() )
      depleted_runes[ num_depleted++ ] = 1;

    // Check Frost and Unholy runes, if Blood runes are not eligible
    if ( num_depleted == 0 )
    {
      for ( int i = 2; i < RUNE_SLOT_MAX; ++i )
      {
        if ( p -> _runes.slot[ i ].is_depleted() )
          depleted_runes[ num_depleted++ ] = i;
      }
    }
  }
  // Frost prefers Unholy runes
  else if ( p -> specialization() == DEATH_KNIGHT_FROST )
  {
    if ( p -> _runes.slot[ 4 ].is_depleted() )
      depleted_runes[ num_depleted++] = 4;

    if ( p -> _runes.slot[ 5 ].is_depleted() )
      depleted_runes[ num_depleted++ ] = 5;

    // Check Blood and Frost runes, if Unholy runes are not eligible
    if ( num_depleted == 0 )
    {
      for ( int i = 0; i < 4; ++i )
      {
        if ( p -> _runes.slot[ i ].is_depleted() )
          depleted_runes[ num_depleted++ ] = i;
      }
    }
  }
  // Finally, Unholy prefers non-Unholy runes
  else if ( p -> specialization() == DEATH_KNIGHT_UNHOLY )
  {
    for ( int i = 0; i < 4; ++i )
    {
      if ( p -> _runes.slot[ i ].is_depleted() )
        depleted_runes[ num_depleted++ ] = i;
    }

    // Check Unholy runes, if Blood and Frost runes are not eligible
    if ( num_depleted == 0 )
    {
      if ( p -> _runes.slot[ 4 ].is_depleted() )
        depleted_runes[ num_depleted++ ] = 4;

      if ( p -> _runes.slot[ 5 ].is_depleted() )
        depleted_runes[ num_depleted++ ] = 5;
    }
  }

  if ( num_depleted > 0 )
  {
    if ( p -> sim -> debug ) log_rune_status( p );

    return depleted_runes[ ( int ) p -> rng.blood_tap -> range( 0, num_depleted ) ];
  }

  return -1;
}

void dk_rune_t::regen_rune( death_knight_t* p, timespan_t periodicity, bool rc )
{
  // If the other rune is already regening, we don't
  // but if both are full we still continue on to record resource gain overflow
  if ( state == STATE_DEPLETED &&   paired_rune -> state == STATE_REGENERATING ) return;
  if ( state == STATE_FULL     && ! ( paired_rune -> state == STATE_FULL )     ) return;

  // Base rune regen rate is 10 seconds; we want the per-second regen
  // rate, so divide by 10.0.  Haste is a multiplier (so 30% haste
  // means composite_attack_haste is 1/1.3), so we invert it.  Haste
  // linearly scales regen rate -- 100% haste means a rune regens in 5
  // seconds, etc.
  double runes_per_second = 1.0 / 10.0 / p -> cache.attack_haste();

  runes_per_second *= 1.0 + p -> spec.improved_blood_presence -> effectN( 1 ).percent();

  if ( p -> sets -> set( SET_T16_4PC_MELEE ) -> ok() && p -> specialization() == DEATH_KNIGHT_FROST &&
       p -> buffs.pillar_of_frost -> check() )
  {
    runes_per_second *= 1.0 + p -> sets -> set( SET_T16_4PC_MELEE ) -> effectN( 2 ).percent();
  }

  double regen_amount = periodicity.total_seconds() * runes_per_second;

  // record rune gains and overflow
  gain_t* gains_rune      = ( rc ) ? p -> gains.rc : p -> gains.rune;
  gain_t* gains_rune_type;
  if ( ! rc )
  {
    gains_rune_type = is_frost()  ? p -> gains.rune_frost   :
                      is_blood()  ? p -> gains.rune_blood   :
                      is_unholy() ? p -> gains.rune_unholy  :
                                    p -> gains.rune_unknown ; // should never happen, so if you've seen this in a report happy bug hunting
  }
  else
  {
    gains_rune_type = is_frost()  ? p -> gains.rc_frost   :
                      is_blood()  ? p -> gains.rc_blood   :
                      is_unholy() ? p -> gains.rc_unholy  :
                                    p -> gains.rune_unknown ; // should never happen, so if you've seen this in a report happy bug hunting
  }
  // full runes don't regen. if both full, record half of overflow, as the other rune will record the other half
  if ( state == STATE_FULL )
  {
    if ( paired_rune -> state == STATE_FULL )
    {
      // FIXME: Resource type?
      gains_rune_type -> add( RESOURCE_NONE, 0, regen_amount * 0.5 );
      gains_rune      -> add( RESOURCE_NONE, 0, regen_amount * 0.5 );
    }
    return;
  }

  // Chances are, we will overflow by a small amount.  Toss extra
  // overflow into our paired rune if it is regenerating or depleted.
  value += regen_amount;
  double overflow = 0.0;
  if ( value > 1.0 )
  {
    overflow = value - 1.0;
    value = 1.0;
  }

  if ( value >= 1.0 )
  {
    if ( state == STATE_REGENERATING )
    {
      if ( is_blood() )
        dk -> procs.ready_blood -> occur();
      else if ( is_frost() )
        dk -> procs.ready_frost -> occur();
      else if ( is_unholy() )
        dk -> procs.ready_unholy -> occur();
    }
    state = STATE_FULL;
  }
  else
    state = STATE_REGENERATING;

  if ( overflow > 0.0 && ( paired_rune -> state == STATE_REGENERATING || paired_rune -> state == STATE_DEPLETED ) )
  {
    // we shouldn't ever overflow the paired rune, but take care just in case
    paired_rune -> value += overflow;
    if ( paired_rune -> value > 1.0 )
    {
      overflow = paired_rune -> value - 1.0;
      paired_rune -> value = 1.0;
    }
    if ( paired_rune -> value >= 1.0 )
    {
      if ( paired_rune -> state == STATE_REGENERATING )
      {
        if ( paired_rune -> is_blood() )
          dk -> procs.ready_blood -> occur();
        else if ( paired_rune -> is_frost() )
          dk -> procs.ready_frost -> occur();
        else if ( paired_rune -> is_unholy() )
          dk -> procs.ready_unholy -> occur();
      }
      paired_rune -> state = STATE_FULL;
    }
    else
      paired_rune -> state = STATE_REGENERATING;
  }
  gains_rune_type -> add( RESOURCE_NONE, regen_amount - overflow, overflow );
  gains_rune      -> add( RESOURCE_NONE, regen_amount - overflow, overflow );

  if ( p -> sim -> debug )
    p -> sim -> output( "rune %d has %.2f regen time (%.3f per second) with %.2f%% haste",
                        slot_number, 1 / runes_per_second, runes_per_second, 100 * ( 1 / p -> cache.attack_haste() - 1 ) );

  if ( state == STATE_FULL )
  {
    if ( p -> sim -> log )
      log_rune_status( p );

    if ( p -> sim -> debug )
      p -> sim -> output( "rune %d regens to full", slot_number );
  }
}

namespace pets {

// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_td_t : public actor_pair_t
{
  dot_t* dots_blood_plague;
  dot_t* dots_frost_fever;
  dot_t* dots_soul_reaper;

  int diseases() const
  {
    int disease_count = 0;
    if ( dots_blood_plague -> ticking ) disease_count++;
    if ( dots_frost_fever  -> ticking ) disease_count++;
    return disease_count;
  }

  dancing_rune_weapon_td_t( player_t* target, dancing_rune_weapon_pet_t* drw );
};

struct dancing_rune_weapon_pet_t : public pet_t
{
  struct drw_spell_t : public spell_t
  {
    drw_spell_t( const std::string& n, dancing_rune_weapon_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      spell_t( n, p, s )
    {
      background                   = true;
      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;
    }

    dancing_rune_weapon_td_t* td( player_t* t = 0 )
    { return p() -> get_target_data( t ? t : target ); }

    dancing_rune_weapon_pet_t* p() const
    { return static_cast<dancing_rune_weapon_pet_t*>( player ); }

    death_knight_t* o() const
    { return static_cast< death_knight_t* >( player -> cast_pet() -> owner ); }
  };

  struct drw_melee_attack_t : public melee_attack_t
  {
    drw_melee_attack_t( const std::string& n, dancing_rune_weapon_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      background = true;
      special    = true;
      may_crit   = true;
    }

    dancing_rune_weapon_td_t* td( player_t* t = 0 )
    { return p() -> get_target_data( t ? t : target ); }

    dancing_rune_weapon_pet_t* p() const
    { return static_cast<dancing_rune_weapon_pet_t*>( player ); }

    death_knight_t* o() const
    { return static_cast< death_knight_t* >( player -> cast_pet() -> owner ); }
  };

  struct drw_blood_plague_t : public drw_spell_t
  {
    drw_blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_plague", p, p -> owner -> find_spell( 55078 ) )  // Also check spell id 55078
    {
      tick_may_crit    = true;
      tick_power_mod   = data().extra_coeff();
      dot_behavior     = DOT_REFRESH;
      may_miss         = false;
      may_crit         = false;
      hasted_ticks     = false;
    }

    virtual double composite_crit()
    { return action_t::composite_crit() + player -> cache.attack_crit(); }
  };

  struct drw_frost_fever_t : public drw_spell_t
  {
    drw_frost_fever_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "frost_fever", p, p -> owner -> find_spell( 55095 ) )
    {
      hasted_ticks     = false;
      may_miss         = false;
      may_crit         = false;
      tick_may_crit    = true;
      dot_behavior     = DOT_REFRESH;
      tick_power_mod   = data().extra_coeff();
    }
  };

  struct drw_blood_boil_t : public drw_spell_t
  {
    drw_blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_boil", p, p -> owner -> find_class_spell( "Blood Boil" ) )
    {
      aoe              = -1;
      may_crit         = true;
      direct_power_mod = data().extra_coeff();
    }

    double composite_target_multiplier( player_t* t )
    {
      double m = drw_spell_t::composite_target_multiplier( t );

      // Apparently inherits damage bonus from owner's diseases ...
      if ( o() -> get_target_data( t ) -> diseases() > 0 )
        m *= 1.50; // hardcoded into tooltip, 18/12/2012

      return m;
    }
  };

  struct drw_death_coil_t : public drw_spell_t
  {
    drw_death_coil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "death_coil", p, p -> owner -> find_class_spell( "Death Coil" ) )
    {
      direct_power_mod = 0.514;
    }
  };

  struct drw_death_siphon_t : public drw_spell_t
  {
    drw_death_siphon_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "death_siphon", p, p -> owner -> find_spell( 108196 ) )
    {
      direct_power_mod = data().extra_coeff();
    }
  };

  struct drw_death_strike_t : public drw_melee_attack_t
  {
    drw_death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "death_strike", p, p -> owner -> find_class_spell( "Death Strike" ) )
    {
      weapon = &( p -> main_hand_weapon );
    }
  };

  struct drw_heart_strike_t : public drw_melee_attack_t
  {
    drw_heart_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "heart_strike", p, p -> owner -> find_spell( 55050 ) )
    {
      weapon = &( p -> main_hand_weapon );
      aoe = 3;
      base_add_multiplier = 0.75;
    }

    virtual double composite_target_multiplier( player_t* t )
    {
      double ctm = drw_melee_attack_t::composite_target_multiplier( t );

      ctm *= 1.0 + td( t ) -> diseases() * data().effectN( 3 ).percent();

      return ctm;
    }
  };

  struct drw_icy_touch_t : public drw_spell_t
  {
    drw_icy_touch_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "icy_touch", p, p -> owner -> find_class_spell( "Icy Touch" ) )
    {
      direct_power_mod = 0.319;
    }

    virtual void impact( action_state_t* s )
    {
      drw_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        p() -> drw_frost_fever -> target = s -> target;
        p() -> drw_frost_fever -> execute();
      }
    }
  };

  struct drw_pestilence_t : public drw_spell_t
  {
    drw_pestilence_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "pestilence", p, p -> owner -> find_class_spell( "Pestilence" ) )
    {
      aoe = -1;
    }

    virtual void impact( action_state_t* s )
    {
      drw_spell_t::impact( s );

      // Doesn't affect the original target
      if ( s -> target == target )
        return;

      if ( result_is_hit( s -> result ) )
      {
        if ( td( target ) -> dots_blood_plague -> ticking )
        {
          p() -> drw_blood_plague -> target = s -> target;
          p() -> drw_blood_plague -> execute();
        }

        if ( td( target ) -> dots_frost_fever -> ticking )
        {
          p() -> drw_frost_fever -> target = s -> target;
          p() -> drw_frost_fever -> execute();
        }
      }
    }
  };

  struct drw_outbreak_t : public drw_spell_t
  {
    drw_outbreak_t( dancing_rune_weapon_pet_t* p ):
      drw_spell_t( "outbreak", p, p -> owner -> find_class_spell( "Outbreak" ) )
    {
      may_crit = false;
    }

    virtual void execute()
    {
      drw_spell_t::execute();

      if ( result_is_hit( execute_state -> result ) )
      {
        p() -> drw_blood_plague -> target = target;
        p() -> drw_blood_plague -> execute();

        p() -> drw_frost_fever -> target = target;
        p() -> drw_frost_fever -> execute();
      }
    }
  };

  struct drw_plague_strike_t : public drw_melee_attack_t
  {
    drw_plague_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "plague_strike", p, p -> owner -> find_class_spell( "Plague Strike" ) )
    {
      weapon = &( p -> main_hand_weapon );
    }

    virtual void impact( action_state_t* s )
    {
      drw_melee_attack_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        p() -> drw_blood_plague -> target = s->target;
        p() -> drw_blood_plague -> execute();
      }
    }
  };

  struct drw_soul_reaper_t : public drw_melee_attack_t
  {
    struct soul_reaper_dot_t : public drw_melee_attack_t
    {
      soul_reaper_dot_t( dancing_rune_weapon_pet_t* p ) :
        drw_melee_attack_t( "soul_reaper_execute", p, p -> find_spell( 114867 ) )
      {
        may_miss = false;
        weapon_multiplier = 0;
        direct_power_mod = data().extra_coeff();
      }

      virtual void init()
      {
        drw_melee_attack_t::init();
        stats = p() -> get_stats( name(), this );
      }
    };

    soul_reaper_dot_t* soul_reaper_dot;

    drw_soul_reaper_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "soul_reaper", p, p -> owner -> find_spell( 114866 ) ),
      soul_reaper_dot( 0 )
    {
      weapon = &( p -> main_hand_weapon );

      dynamic_tick_action = true;
      tick_action = new soul_reaper_dot_t( p );
      add_child( tick_action );
    }

    void init()
    {
      drw_melee_attack_t::init();

      snapshot_flags |= STATE_MUL_TA;
    }

    void tick( dot_t* dot )
    {
      int pct = 35;
      if ( o() -> set_bonus.tier15_4pc_melee() )
        pct = o() -> sets -> set( SET_T15_4PC_MELEE ) -> effectN( 1 ).base_value();

      if ( dot -> state -> target -> health_percentage() <= pct )
        drw_melee_attack_t::tick( dot );
    }
  };

  struct drw_necrotic_strike_t : public drw_melee_attack_t
  {
    drw_necrotic_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "necrotic_strike", p, p -> owner -> find_class_spell( "Necrotic Strike" ) )
    { weapon = &( p -> main_hand_weapon ); }
  };

  struct drw_melee_t : public drw_melee_attack_t
  {
    drw_melee_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "drw_melee", p )
    {
      weapon            = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      special           = false;
    }
  };

  target_specific_t<dancing_rune_weapon_td_t*> target_data;

  spell_t*        drw_blood_plague;
  spell_t*        drw_frost_fever;

  spell_t*        drw_blood_boil;
  spell_t*        drw_death_coil;
  spell_t*        drw_death_siphon;
  spell_t*        drw_icy_touch;
  spell_t*        drw_outbreak;
  spell_t*        drw_pestilence;

  melee_attack_t* drw_death_strike;
  melee_attack_t* drw_heart_strike;
  melee_attack_t* drw_necrotic_strike;
  melee_attack_t* drw_plague_strike;
  melee_attack_t* drw_soul_reaper;
  melee_attack_t* drw_melee;

  dancing_rune_weapon_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "dancing_rune_weapon", true ),
    drw_blood_plague( nullptr ), drw_frost_fever( nullptr ),
    drw_blood_boil( nullptr ), drw_death_coil( nullptr ),
    drw_death_siphon( nullptr ), drw_icy_touch( nullptr ),
    drw_outbreak( nullptr ), drw_pestilence( nullptr ),
    drw_death_strike( nullptr ), drw_heart_strike( nullptr ),
    drw_necrotic_strike( nullptr ), drw_plague_strike( nullptr ),
    drw_soul_reaper( nullptr ), drw_melee( nullptr )
  {
    main_hand_weapon.type       = WEAPON_BEAST_2H;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 3.0;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 3.0;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.5 );

    owner_coeff.ap_from_ap = 1.0;
  }

  death_knight_t* o() const
  { return static_cast< death_knight_t* >( owner ); }

  dancing_rune_weapon_td_t* td( player_t* t = 0 )
  { return get_target_data( t ? t : target ); }

  virtual dancing_rune_weapon_td_t* get_target_data( player_t* target )
  {
    dancing_rune_weapon_td_t*& td = target_data[ target ];
    if ( ! td )
      td = new dancing_rune_weapon_td_t( target, this );
    return td;
  }

  virtual void init_spells()
  {
    pet_t::init_spells();

    // Kludge of the century to get pointless initialization warnings to
    // go away.
    type = DEATH_KNIGHT; _spec = DEATH_KNIGHT_BLOOD;

    drw_frost_fever   = new drw_frost_fever_t  ( this );
    drw_blood_plague  = new drw_blood_plague_t ( this );

    drw_blood_boil    = new drw_blood_boil_t   ( this );
    drw_death_coil    = new drw_death_coil_t   ( this );
    drw_death_siphon  = new drw_death_siphon_t ( this );
    drw_icy_touch     = new drw_icy_touch_t    ( this );
    drw_outbreak      = new drw_outbreak_t     ( this );
    drw_pestilence    = new drw_pestilence_t   ( this );

    drw_death_strike  = new drw_death_strike_t ( this );
    drw_heart_strike  = new drw_heart_strike_t ( this );
    drw_necrotic_strike = new drw_necrotic_strike_t( this );
    drw_plague_strike = new drw_plague_strike_t( this );
    drw_soul_reaper   = new drw_soul_reaper_t  ( this );
    drw_melee         = new drw_melee_t        ( this );

    type = PLAYER_GUARDIAN; _spec = SPEC_NONE;
  }

  void summon( timespan_t duration = timespan_t::zero() )
  {
    pet_t::summon( duration );
    drw_melee -> schedule_execute();
  }

  double composite_player_multiplier( school_e school )
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> glyph.dancing_rune_weapon -> ok() )
      m *= 1.0 + o() -> glyph.dancing_rune_weapon -> effectN( 2 ).percent();

    return m;
  }
};

dancing_rune_weapon_td_t::dancing_rune_weapon_td_t( player_t* target, dancing_rune_weapon_pet_t* drw ) :
  actor_pair_t( target, drw )
{
  dots_blood_plague    = target -> get_dot( "blood_plague",        drw );
  dots_frost_fever     = target -> get_dot( "frost_fever",         drw );
  dots_soul_reaper     = target -> get_dot( "soul_reaper_execute", drw );
}

struct death_knight_pet_t : public pet_t
{
  const spell_data_t* command;

  death_knight_pet_t( sim_t* sim, death_knight_t* owner, const std::string& n, bool guardian ) :
    pet_t( sim, owner, n, guardian )
  {
    command = find_spell( 54562 );
  }

  death_knight_t* o()
  { return debug_cast<death_knight_t*>( owner ); }

  double composite_player_multiplier( school_e school )
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    return m;
  }
};

// ==========================================================================
// Guardians
// ==========================================================================

// ==========================================================================
// Army of the Dead Ghoul
// ==========================================================================

struct army_ghoul_pet_t : public death_knight_pet_t
{
  army_ghoul_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "army_of_the_dead", true )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    action_list_str = "snapshot_stats/auto_attack/claw";


  }

  struct army_ghoul_pet_melee_attack_t : public melee_attack_t
  {
    army_ghoul_pet_melee_attack_t( const std::string& n, army_ghoul_pet_t* p,
                                   const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
    }

    army_ghoul_pet_t* p() const
    { return static_cast<army_ghoul_pet_t*>( player ); }

    void init()
    {
      melee_attack_t::init();

      if ( player != p() -> o() -> pets.army_ghoul[ 0 ] )
        stats = p() -> o() -> pets.army_ghoul[ 0 ] -> get_stats( name_str );
    }
  };

  struct army_ghoul_pet_melee_t : public army_ghoul_pet_melee_attack_t
  {
    army_ghoul_pet_melee_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_melee_attack_t( "melee", p )
    {
      school          = SCHOOL_PHYSICAL;
      base_execute_time = weapon -> swing_time;
      background        = true;
      repeating         = true;
      special           = false;
    }
  };

  struct army_ghoul_pet_auto_melee_attack_t : public army_ghoul_pet_melee_attack_t
  {
    army_ghoul_pet_auto_melee_attack_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_melee_attack_t( "auto_attack", p )
    {
      weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack = new army_ghoul_pet_melee_t( p );
      trigger_gcd = timespan_t::zero();
      special = true;
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct army_ghoul_pet_claw_t : public army_ghoul_pet_melee_attack_t
  {
    army_ghoul_pet_claw_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_melee_attack_t( "claw", p, p -> find_spell( 91776 ) )
    {
      special = true;
    }
  };

  virtual void init_base_stats()
  {
    initial.stats.attack_power = -20;

    // Ghouls don't appear to gain any crit from agi, they may also just have none
    // initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;

    owner_coeff.ap_from_ap = 0.5;
  }

  virtual resource_e primary_resource() { return RESOURCE_ENERGY; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new  army_ghoul_pet_auto_melee_attack_t( this );
    if ( name == "claw"           ) return new         army_ghoul_pet_claw_t( this );

    return pet_t::create_action( name, options_str );
  }

  timespan_t available()
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    if ( energy > 40 )
      return timespan_t::from_seconds( 0.1 );

    return std::max(
             timespan_t::from_seconds( ( 40 - energy ) / energy_regen_per_second() ),
             timespan_t::from_seconds( 0.1 )
           );
  }
};

// ==========================================================================
// Bloodworms
// ==========================================================================
struct bloodworms_pet_t : public death_knight_pet_t
{
  // FIXME: Verify heal amounts, currently uses 15% owner max hp * stacks * 25%
  struct blood_burst_t : public heal_t
  {
    blood_burst_t( bloodworms_pet_t* p ) :
      heal_t( "blood_burst", p, p -> owner -> find_spell( 81280 ) )
    {
      background = true;
      aoe = -1;
    }

    double base_da_min( const action_state_t* )
    {
      death_knight_t* o = debug_cast< death_knight_t* >( player -> cast_pet() -> owner );
      return o -> resources.max[ RESOURCE_HEALTH ] * 0.15 * p() -> blood_gorged -> check() * 0.25;
    }

    double base_da_max( const action_state_t* )
    {
      death_knight_t* o = debug_cast< death_knight_t* >( player -> cast_pet() -> owner );
      return o -> resources.max[ RESOURCE_HEALTH ] * 0.15 * p() -> blood_gorged -> check() * 0.25;
    }

    bloodworms_pet_t* p() const
    { return debug_cast< bloodworms_pet_t* >( player ); }

    void init()
    {
      heal_t::init();

      death_knight_t* o = debug_cast< death_knight_t* >( player -> cast_pet() -> owner );

      if ( player != o -> pets.bloodworms[ 0 ] )
        stats = o -> pets.bloodworms[ 0 ] -> get_stats( name_str );
    }
  };

  // FIXME: Level 80/85 values
  struct melee_t : public melee_attack_t
  {
    struct blood_burst_event_t : public event_t
    {
      blood_burst_event_t( bloodworms_pet_t* p, timespan_t delta_time ) :
        event_t( p, "blood_burst" )
      { sim.add_event( this, delta_time ); }

      void execute()
      {
        bloodworms_pet_t* p = debug_cast< bloodworms_pet_t* >( player );
        p -> blood_burst -> execute();
        if ( ! player -> is_sleeping() )
          player -> cast_pet() -> dismiss();
      }
    };

    melee_t( player_t* player ) :
      melee_attack_t( "melee", player )
    {
      school          = SCHOOL_PHYSICAL;
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      may_crit    = true;
      background  = true;
      repeating   = true;
    }

    void init()
    {
      melee_attack_t::init();

      death_knight_t* o = debug_cast< death_knight_t* >( player -> cast_pet() -> owner );

      if ( player != o -> pets.bloodworms[ 0 ] )
        stats = o -> pets.bloodworms[ 0 ] -> get_stats( name_str );
    }

    void impact( action_state_t* s )
    {
      melee_attack_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        if ( p() -> blood_gorged -> check() > 0 )
        {
          death_knight_t* o = debug_cast< death_knight_t* >( player -> cast_pet() -> owner );

          double base_proc_chance = pow( p() -> blood_gorged -> check() + 1, 3.0 );
          double multiplier = 0;
          if ( o -> health_percentage() >= 100 && p() -> blood_gorged -> check() >= 5 )
            multiplier = 0.5;
          else if ( o -> health_percentage() > 60 && o -> health_percentage() < 100 )
            multiplier = 1.0;
          else if ( o -> health_percentage() > 30 && o -> health_percentage() <= 60 )
            multiplier = 1.5;
          else if ( o -> health_percentage() <= 30 )
            multiplier = 2.0;

          if ( sim -> debug )
            sim -> output( "%s-%s burst chance, base=%f multiplier=%f total=%f",
                           o -> name(), player -> name(), base_proc_chance, multiplier, base_proc_chance * multiplier );

          if ( base_proc_chance * multiplier > p() -> rng_blood_burst -> range( 0, 999 ) )
            new ( *sim ) blood_burst_event_t( p(), timespan_t::zero() );
          else
            p() -> blood_gorged -> trigger();
        }
        else
          p() -> blood_gorged -> trigger();
      }
    }

    bloodworms_pet_t* p() const
    { return debug_cast< bloodworms_pet_t* >( player ); }
  };

  melee_t* melee;
  buff_t* blood_gorged;
  blood_burst_t* blood_burst;
  rng_t*  rng_blood_burst;

  bloodworms_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "bloodworms", true /*guardian*/ ),
    melee( nullptr ), blood_gorged( nullptr ), blood_burst( nullptr ), rng_blood_burst( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.55;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.55;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    owner_coeff.ap_from_ap = 0.385;
  }

  void init_spells()
  {
    pet_t::init_spells();

    melee = new melee_t( this );
    blood_burst = new blood_burst_t( this );
  }

  void create_buffs()
  {
    pet_t::create_buffs();

    blood_gorged = buff_creator_t( this, "blood_gorged", find_spell( 81277 ) )
                   .chance( 1 );
  }

  void init_rng()
  {
    pet_t::init_rng();

    rng_blood_burst = get_rng( "blood_burst" );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() )
  {
    pet_t::summon( duration );
    melee -> schedule_execute();
  }

  virtual resource_e primary_resource() { return RESOURCE_MANA; }
};

// ==========================================================================
// Gargoyle
// ==========================================================================

struct gargoyle_pet_t : public death_knight_pet_t
{
  struct travel_t : public action_t
  {
    rng_t* travel_rng;
    bool executed;

    travel_t( player_t* player ) :
      action_t( ACTION_OTHER, "travel", player ),
      travel_rng( 0 ), executed( false )
    {
      may_miss = false;
      dual = true;
      travel_rng = player -> get_rng( "gargoyle_travel" );
    }

    result_e calculate_result( action_state_t* /* s */ )
    { return RESULT_HIT; }

    void execute()
    {
      action_t::execute();
      executed = true;
    }

    void cancel()
    {
      action_t::cancel();
      executed = false;
    }

    // ~3 seconds seems to be the optimal initial delay
    // FIXME: Verify if behavior still exists on 5.3 PTR
    timespan_t execute_time()
    { return timespan_t::from_seconds( travel_rng -> gauss( 2.9, 0.2 ) ); }

    bool ready()
    { return ! executed; }
  };

  struct gargoyle_strike_t : public spell_t
  {
    gargoyle_strike_t( pet_t* pet ) :
      spell_t( "gargoyle_strike", pet, pet -> find_pet_spell( "Gargoyle Strike" ) )
    {
      harmful            = true;
      trigger_gcd        = timespan_t::from_seconds( 1.5 );
      may_crit           = true;
      min_gcd            = timespan_t::from_seconds( 1.5 ); // issue961
      school             = SCHOOL_SHADOWSTORM;
      auto_cast          = true;
    }

    double composite_da_multiplier()
    {
      double m = spell_t::composite_da_multiplier();

      death_knight_t* dk = debug_cast< death_knight_t* >( player -> cast_pet() -> owner );
      if ( dk -> mastery.dreadblade -> ok() )
        m *= 1.0 + dk -> cache.mastery_value();

      return m;
    }
  };

  gargoyle_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "gargoyle", true )
  { }

  virtual void init_base_stats()
  {
    action_list_str = "travel/gargoyle_strike";

    // As per Blizzard
    owner_coeff.sp_from_ap = 0.7;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "gargoyle_strike" ) return new gargoyle_strike_t( this );
    if ( name == "travel"          ) return new travel_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Ghoul
// ==========================================================================

struct ghoul_pet_t : public death_knight_pet_t
{
  ghoul_pet_t( sim_t* sim, death_knight_t* owner, const std::string& name, bool guardian ) :
    death_knight_pet_t( sim, owner, name, guardian )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.8;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.8;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    action_list_str = "auto_attack/sweeping_claws/claw";
  }

  struct ghoul_pet_melee_attack_t : public melee_attack_t
  {
    ghoul_pet_melee_attack_t( const char* n, ghoul_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
    }

    virtual double action_multiplier()
    {
      double am = melee_attack_t::action_multiplier();

      ghoul_pet_t* p = debug_cast<ghoul_pet_t*>( player );

      am *= 1.0 + p -> o() -> buffs.shadow_infusion -> stack() * p -> o() -> buffs.shadow_infusion -> data().effectN( 1 ).percent();

      if ( p -> o() -> buffs.dark_transformation -> up() )
        am *= 1.0 + p -> o() -> buffs.dark_transformation -> data().effectN( 1 ).percent();

      return am;
    }
  };

  struct ghoul_pet_melee_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_melee_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "melee", p )
    {
      school          = SCHOOL_PHYSICAL;
      base_execute_time = weapon -> swing_time;
      background        = true;
      repeating         = true;
      special           = false;
    }
  };

  struct ghoul_pet_auto_melee_attack_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_auto_melee_attack_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "auto_attack", p )
    {
      weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack = new ghoul_pet_melee_t( p );
      trigger_gcd = timespan_t::zero();
      special = true;
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct ghoul_pet_claw_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_claw_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "claw", p, p -> find_spell( 91776 ) )
    {
      special = true;
    }
  };

  struct ghoul_pet_sweeping_claws_t : public ghoul_pet_melee_attack_t
  {
    ghoul_pet_sweeping_claws_t( ghoul_pet_t* p ) :
      ghoul_pet_melee_attack_t( "sweeping_claws", p, p -> find_spell( 91778 ) )
    {
      aoe = 3;
      special = true;
    }

    virtual bool ready()
    {
      death_knight_t* dk = debug_cast<ghoul_pet_t*>( player ) -> o();

      if ( ! dk -> buffs.dark_transformation -> check() )
        return false;

      return ghoul_pet_melee_attack_t::ready();
    }
  };

  virtual void init_base_stats()
  {
    //assert( owner -> specialization() != SPEC_NONE ); // Is there a reason for this?

    initial.stats.attack_power = -20;

    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
    owner_coeff.ap_from_ap = 0.8;
  }

  //Ghoul regen doesn't benefit from haste (even bloodlust/heroism)
  virtual resource_e primary_resource()
  {
    return RESOURCE_ENERGY;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new    ghoul_pet_auto_melee_attack_t( this );
    if ( name == "claw"           ) return new           ghoul_pet_claw_t( this );
    if ( name == "sweeping_claws" ) return new ghoul_pet_sweeping_claws_t( this );

    return pet_t::create_action( name, options_str );
  }

  timespan_t available()
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    // Cheapest Ability need 40 Energy
    if ( energy > 40 )
      return timespan_t::from_seconds( 0.1 );

    return std::max(
             timespan_t::from_seconds( ( 40 - energy ) / energy_regen_per_second() ),
             timespan_t::from_seconds( 0.1 )
           );
  }
};

// ==========================================================================
// Tier15 2 piece minion
// ==========================================================================

struct fallen_zandalari_t : public death_knight_pet_t
{
  fallen_zandalari_t( death_knight_t* owner ) :
    death_knight_pet_t( owner -> sim, owner, "fallen_zandalari", true )
  { }

  struct zandalari_melee_attack_t : public melee_attack_t
  {
    zandalari_melee_attack_t( const std::string& n, fallen_zandalari_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      school   = SCHOOL_PHYSICAL;
      may_crit = special = true;
      weapon   = &( p -> main_hand_weapon );
    }

    void init()
    {
      melee_attack_t::init();
      fallen_zandalari_t* p = debug_cast< fallen_zandalari_t* >( player );
      stats = p -> o() -> pets.fallen_zandalari[ 0 ] -> find_action( name_str ) -> stats;
    }
  };

  struct zandalari_melee_t : public zandalari_melee_attack_t
  {
    zandalari_melee_t( fallen_zandalari_t* p ) :
      zandalari_melee_attack_t( "melee", p )
    {
      base_execute_time = weapon -> swing_time;
      background        = true;
      repeating         = true;
      special           = false;
    }
  };

  struct zandalari_auto_attack_t : public zandalari_melee_attack_t
  {
    zandalari_auto_attack_t( fallen_zandalari_t* p ) :
      zandalari_melee_attack_t( "auto_attack", p )
    {
      p -> main_hand_attack = new zandalari_melee_t( p );
      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    { player -> main_hand_attack -> schedule_execute(); }

    virtual bool ready()
    { return( player -> main_hand_attack -> execute_event == 0 ); }
  };

  struct zandalari_strike_t : public zandalari_melee_attack_t
  {
    zandalari_strike_t( fallen_zandalari_t* p ) :
      zandalari_melee_attack_t( "strike", p, p -> find_spell( 138537 ) )
    {
      special = true;
    }
  };

  virtual void init_base_stats()
  {
    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
    owner_coeff.ap_from_ap = 0.8;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.8;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.8;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    action_list_str = "auto_attack/strike";
  }

  virtual resource_e primary_resource()
  { return RESOURCE_ENERGY; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack" ) return new zandalari_auto_attack_t( this );
    if ( name == "strike"        ) return new    zandalari_strike_t( this );

    return death_knight_pet_t::create_action( name, options_str );
  }

  timespan_t available()
  {
    double energy = resources.current[ RESOURCE_ENERGY ];

    // Cheapest Ability need 40 Energy
    if ( energy > 40 )
      return timespan_t::from_seconds( 0.1 );

    return std::max(
             timespan_t::from_seconds( ( 40 - energy ) / energy_regen_per_second() ),
             timespan_t::from_seconds( 0.1 )
           );
  }
};

} // namespace pets

namespace { // UNNAMED NAMESPACE

// Template for common death knight action code. See priest_action_t.
template <class Base>
struct death_knight_action_t : public Base
{
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  int    cost_death;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];
  gain_t* rp_gains;

  typedef Base action_base_t;
  typedef death_knight_action_t base_t;

  death_knight_action_t( const std::string& n, death_knight_t* p,
                         const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, p, s ),
    cost_blood( 0 ), cost_frost( 0 ), cost_unholy( 0 ), cost_death( 0 ),
    convert_runes( 0 )
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;

    action_base_t::may_crit   = true;
    action_base_t::may_glance = false;

    rp_gains = action_base_t::player -> get_gain( "rp_" + action_base_t::name_str );
    extract_rune_cost( s, &cost_blood, &cost_frost, &cost_unholy, &cost_death );
  }

  death_knight_t* p() const { return static_cast< death_knight_t* >( action_base_t::player ); }

  death_knight_td_t* cast_td( player_t* t = 0 )
  { return p() -> get_target_data( t ? t : action_base_t::target ); }

  virtual void reset()
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
      use[i] = false;

    action_base_t::reset();
  }

  virtual void consume_resource()
  {
    if ( action_base_t::rp_gain > 0 )
    {
      if ( action_base_t::result_is_hit( action_base_t::execute_state -> result ) )
      {
        if ( p() -> buffs.frost_presence -> check() )
        {
          p() -> resource_gain( RESOURCE_RUNIC_POWER,
                                action_base_t::rp_gain * p() -> buffs.frost_presence -> value(),
                                p() -> gains.frost_presence );
        }
        p() -> resource_gain( RESOURCE_RUNIC_POWER, action_base_t::rp_gain, rp_gains );
      }
    }
    else
    {
      action_base_t::consume_resource();
    }
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = action_base_t::composite_target_multiplier( t );

    if ( dbc::is_school( action_base_t::school, SCHOOL_FROST ) )
    {
      m *= 1.0 + cast_td( t ) -> debuffs_frost_vulnerability -> stack() *
           cast_td( t ) -> debuffs_frost_vulnerability -> data().effectN( 1 ).percent();
    }

    return m;
  }
};

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_melee_attack_t : public death_knight_action_t<melee_attack_t>
{
  bool   always_consume;
  bool   requires_weapon;

  death_knight_melee_attack_t( const std::string& n, death_knight_t* p,
                               const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ),
    always_consume( false ), requires_weapon( true )
  {
    may_crit   = true;
    may_glance = false;

    if ( p -> main_hand_weapon.group() == WEAPON_2H )
      base_multiplier += p -> spec.might_of_the_frozen_wastes -> effectN( 2 ).percent();
  }

  virtual void   consume_resource();
  virtual void   execute();

  virtual double composite_da_multiplier()
  {
    double m = base_t::composite_da_multiplier();

    if ( dbc::is_school( school, SCHOOL_SHADOW ) || dbc::is_school( school, SCHOOL_FROST ) )
    {
      if ( ! proc )
        m *= 1.0 + p() -> runeforge.rune_of_cinderglacier -> value();
    }

    return m;
  }

  virtual bool   ready();
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public death_knight_action_t<spell_t>
{
  death_knight_spell_t( const std::string& n, death_knight_t* p,
                        const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    _init_dk_spell();
  }

  void _init_dk_spell()
  {
    may_crit = true;

    base_spell_power_multiplier = 0;
    base_attack_power_multiplier = 1;
  }

  virtual void   consume_resource();
  virtual void   execute();

  virtual double composite_da_multiplier()
  {
    double m = base_t::composite_da_multiplier();

    if ( dbc::is_school( school, SCHOOL_SHADOW ) || dbc::is_school( school, SCHOOL_FROST ) )
    {
      if ( ! proc )
        m *= 1.0 + p() -> runeforge.rune_of_cinderglacier -> value();
    }

    return m;
  }

  virtual bool   ready();
};

struct death_knight_heal_t : public death_knight_action_t<heal_t>
{
  death_knight_heal_t( const std::string& n, death_knight_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    base_attack_power_multiplier = 1;
    base_spell_power_multiplier = 0;
  }

  double composite_da_multiplier()
  {
    double m = base_t::composite_da_multiplier();

    if ( p() -> buffs.vampiric_blood -> check() )
      m *= 1.0 + p() -> buffs.vampiric_blood -> data().effectN( 1 ).percent() +
                 p() -> glyph.vampiric_blood -> effectN( 1 ).percent();

    return m;
  }
};

// Count Runes ==============================================================

// currently not used. but useful. commenting out to get rid of compile warning
//static int count_runes( player_t* player )
//{
//  ;
//  int count_by_type[RUNE_SLOT_MAX / 2] = { 0, 0, 0 }; // blood, frost, unholy
//
//  for ( int i = 0; i < RUNE_SLOT_MAX / 2; ++i )
//    count_by_type[i] = ( ( int )p() -> _runes.slot[2 * i].is_ready() +
//                         ( int )p() -> _runes.slot[2 * i + 1].is_ready() );
//
//  return count_by_type[0] + ( count_by_type[1] << 8 ) + ( count_by_type[2] << 16 );
//}

// ==========================================================================
// Triggers
// ==========================================================================

static void trigger_t16_2pc_tank( action_state_t* s )
{
  if ( ! s -> action -> player -> set_bonus.tier16_2pc_tank() )
    return;

  death_knight_t* p = debug_cast< death_knight_t* >( s -> action -> player );

  p -> t16_tank_2pc_driver++;

  if ( p -> t16_tank_2pc_driver == p -> sets -> set( SET_T16_2PC_TANK ) -> effectN( 1 ).base_value() )
  {
    p -> buffs.bone_wall -> trigger();
    p -> t16_tank_2pc_driver = 0;
  }
}

static void trigger_t15_2pc_melee( death_knight_melee_attack_t* attack )
{
  if ( ! attack -> player -> set_bonus.tier15_2pc_melee() )
    return;

  death_knight_t* p = debug_cast< death_knight_t* >( attack -> player );

  if ( ( p -> rng.t15_2pc_melee -> trigger( *attack ) ) )
  {
    p -> procs.t15_2pc_melee -> occur();
    size_t i;

    for ( i = 0; i < p -> pets.fallen_zandalari.size(); i++ )
    {
      if ( ! p -> pets.fallen_zandalari[ i ] -> is_sleeping() )
        continue;

      p -> pets.fallen_zandalari[ i ] -> summon( timespan_t::from_seconds( 15 ) );
      break;
    }

    assert( i < p -> pets.fallen_zandalari.size() );
  }
}

static void trigger_bloodworms( death_knight_melee_attack_t* attack )
{
  if ( attack -> player -> specialization() != DEATH_KNIGHT_BLOOD )
    return;

  death_knight_t* p = debug_cast< death_knight_t* >( attack -> player );

  if ( p -> rng.blood_parasite -> roll( p -> spec.blood_parasite -> proc_chance() ) )
  {
    p -> procs.blood_parasite -> occur();
    size_t i;

    for ( i = 0; i < p -> pets.bloodworms.size(); i++ )
    {
      if ( ! p -> pets.bloodworms[ i ] -> is_sleeping() )
        continue;

      p -> pets.bloodworms[ i ] -> summon( p -> spell.blood_parasite -> duration() );
      break;
    }

    assert( i < p -> pets.bloodworms.size() );
  }
}

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_melee_attack_t::consume_resource() ==========================

void death_knight_melee_attack_t::consume_resource()
{
  base_t::consume_resource();

  if ( result_is_hit( execute_state -> result ) || always_consume )
    consume_runes( p(), use, convert_runes == 0 ? false : sim -> roll( convert_runes ) == 1 );
}

// death_knight_melee_attack_t::execute() ===================================

void death_knight_melee_attack_t::execute()
{
  base_t::execute();

  if ( ! proc && result_is_hit( execute_state -> result ) )
  {
    trigger_bloodworms( this );
    if ( dbc::is_school( school, SCHOOL_SHADOW ) || dbc::is_school( school, SCHOOL_FROST ) )
    {
      p() -> runeforge.rune_of_cinderglacier -> decrement();
    }
  }

  if ( ! result_is_hit( execute_state -> result ) && ! always_consume && resource_consumed > 0 )
    p() -> resource_gain( RESOURCE_RUNIC_POWER, resource_consumed * RUNIC_POWER_REFUND, p() -> gains.power_refund );

  if ( result_is_hit( execute_state -> result ) && cast_td( execute_state -> target ) -> dots_blood_plague -> ticking )
    p() -> buffs.crimson_scourge -> trigger();

  trigger_t15_2pc_melee( this );
}

// death_knight_melee_attack_t::ready() =====================================

bool death_knight_melee_attack_t::ready()
{
  if ( ! base_t::ready() )
    return false;

  if ( requires_weapon )
    if ( ! weapon || weapon -> group() == WEAPON_RANGED )
      return false;

  return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
}

// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================


// death_knight_spell_t::consume_resource() =================================

void death_knight_spell_t::consume_resource()
{
  base_t::consume_resource();

  if ( result_is_hit( execute_state -> result ) )
    consume_runes( p(), use, convert_runes == 0 ? false : sim -> roll( convert_runes ) == 1 );
}

// death_knight_spell_t::execute() ==========================================

void death_knight_spell_t::execute()
{
  base_t::execute();

  if ( result_is_hit( execute_state -> result ) )
  {
    if ( dbc::is_school( school, SCHOOL_SHADOW ) || dbc::is_school( school, SCHOOL_FROST ) )
    {
      p() -> runeforge.rune_of_cinderglacier -> decrement();
    }
  }
}

// death_knight_spell_t::ready() ============================================

bool death_knight_spell_t::ready()
{
  if ( ! base_t::ready() )
    return false;

  return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
}

// ==========================================================================
// Death Knight Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public death_knight_melee_attack_t
{
  int sync_weapons;
  melee_t( const char* name, death_knight_t* p, int sw ) :
    death_knight_melee_attack_t( name, p ), sync_weapons( sw )
  {
    school          = SCHOOL_PHYSICAL;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = death_knight_melee_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::from_seconds( 0.2 ) ) : t / 2 ) : timespan_t::from_seconds( 0.01 );
    }
    return t;
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        // T13 2pc gives 2 stacks of SD, otherwise we can only ever have one
        // Ensure that if we have 1 that we only refresh, not add another stack
        int new_stacks = ( p() -> set_bonus.tier13_2pc_melee() && p() -> rng.t13_2pc_melee -> roll( p() -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent() ) ) ? 2 : 1;

        // Mists of Pandaria Sudden Doom is 3 PPM
        if ( p() -> spec.sudden_doom -> ok() &&
             p() -> rng.sudden_doom -> roll( weapon -> proc_chance_on_swing( 3 ) ) )
        {
          // If we're proccing 2 or we have 0 stacks, trigger like normal
          if ( new_stacks == 2 || p() -> buffs.sudden_doom -> check() == 0 )
          {
            if ( p() -> buffs.sudden_doom -> trigger( new_stacks ) )
              p() -> buffs.death_shroud -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, timespan_t::from_seconds( 11 ) );
          }
          // refresh stacks. However if we have a double stack and only 1 procced, it refreshes to 1 stack
          else
          {
            p() -> buffs.sudden_doom -> refresh( 0 );
            if ( p() -> buffs.sudden_doom -> check() == 2 && new_stacks == 1 )
              p() -> buffs.sudden_doom -> decrement( 1 );
          }
        }

        if ( p() -> buffs.scent_of_blood -> trigger() )
        {
          p() -> resource_gain( RESOURCE_RUNIC_POWER,
                                p() -> buffs.scent_of_blood -> data().effectN( 2 ).resource( RESOURCE_RUNIC_POWER ),
                                p() -> gains.scent_of_blood );
        }
      }

      // Killing Machine is 6 PPM
      if ( p() -> spec.killing_machine -> ok() )
      {
        if ( p() -> buffs.killing_machine -> trigger( 1, buff_t::DEFAULT_VALUE(), weapon -> proc_chance_on_swing( 6 ) ) )
        {
          timespan_t duration = timespan_t::from_seconds( 4 + ( ( weapon -> group() == WEAPON_2H ) ? 2 : 1 ) * 2 );
          p() -> buffs.death_shroud -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
        }
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public death_knight_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "auto_attack", p ), sync_weapons( 0 )
  {
    option_t options[] =
    {
      opt_bool( "sync_weapons", sync_weapons ),
      opt_null()
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
      p -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    player -> main_hand_attack -> schedule_execute();
    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    if ( player -> is_moving() )
      return false;
    return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Army of the Dead =========================================================

struct army_of_the_dead_t : public death_knight_spell_t
{
  army_of_the_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", p, p -> find_class_spell( "Army of the Dead" ) )
  {
    parse_options( NULL, options_str );

    harmful     = false;
    channeled   = true;
  }

  virtual void execute()
  {
    if ( ! p() -> in_combat )
    {
      // Pre-casting it before the fight
      int saved_ticks = num_ticks;
      num_ticks = 0;
      channeled = false;
      death_knight_spell_t::execute();
      channeled = true;
      num_ticks = saved_ticks;
      // Because of the new rune regen system in 4.0, it only makes
      // sense to cast ghouls 7-10s before a fight begins so you don't
      // waste rune regen and enter the fight depleted.  So, the time
      // you get for ghouls is 4-6 seconds less.
      // TODO: DBC
      for ( int i = 0; i < 8; i++ )
        p() -> pets.army_ghoul[ i ] -> summon( timespan_t::from_seconds( 35 ) );

      // Simulate rune regen for 5 seconds for the consumed runes. Ugly but works
      // Note that this presumes no other rune-using abilities are used
      // precombat
      for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
        p() -> _runes.slot[ i ].regen_rune( p(), timespan_t::from_seconds( 5.0 ) );
    }
    else
    {
      death_knight_spell_t::execute();

      // TODO: DBC
      for ( int i = 0; i < 8; i++ )
        p() -> pets.army_ghoul[ i ] -> summon( timespan_t::from_seconds( 40 ) );
    }
  }

  virtual bool ready()
  {
    if ( p() -> pets.army_ghoul[ 0 ] && ! p() -> pets.army_ghoul[ 0 ] -> is_sleeping() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Blood Boil ===============================================================

struct blood_boil_t : public death_knight_spell_t
{
  death_knight_spell_t* proxy_pestilence;

  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> find_class_spell( "Blood Boil" ) ),
    proxy_pestilence( 0 )
  {
    parse_options( NULL, options_str );

    if ( p -> spec.reaping -> ok() )
      convert_runes = 1.0;

    aoe                = -1;
    direct_power_mod   = data().extra_coeff();

    if ( p -> talent.roiling_blood -> ok() )
    {
      // Make up a dummy pestilence action that consumes no runes, gives no
      // runic power, and converts no runes. It is also prohibited to proc
      // anything by default. It will however show in the reports.
      proxy_pestilence = static_cast< death_knight_spell_t* >( p -> create_action( "pestilence", std::string() ) );
      proxy_pestilence -> cost_blood = 0;
      proxy_pestilence -> cost_unholy = 0;
      proxy_pestilence -> cost_frost = 0;
      proxy_pestilence -> rp_gain = 0;
      proxy_pestilence -> convert_runes = 0;
      proxy_pestilence -> background = true;
      proxy_pestilence -> proc = true;
    }
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost()
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_blood_boil -> execute();

    if ( p() -> buffs.crimson_scourge -> up() )
      p() -> buffs.crimson_scourge -> expire();

    if ( p() -> talent.roiling_blood -> ok() )
    {
      proxy_pestilence -> target = target;
      proxy_pestilence -> execute();
    }
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = death_knight_spell_t::composite_target_multiplier( t );

    if ( cast_td( t ) -> diseases() > 0 )
      m *= 1.50; // hardcoded into tooltip, 18/12/2012

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( p() -> spec.scarlet_fever -> ok() )
    {
      if ( cast_td( s -> target ) -> dots_blood_plague -> ticking )
        cast_td( s -> target ) -> dots_blood_plague -> refresh_duration();

      if ( cast_td( s -> target ) -> dots_frost_fever -> ticking )
        cast_td( s -> target ) -> dots_frost_fever -> refresh_duration();
    }

    if ( result_is_hit( s -> result ) )
      trigger_t16_2pc_tank( s );
  }

  virtual bool ready()
  {
    if ( ! spell_t::ready() )
      return false;

    if ( ( ! p() -> in_combat && ! harmful ) || p() -> buffs.crimson_scourge -> check() )
      return group_runes( p(), 0, 0, 0, 0, use );
    else
      return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
  }
};

// Blood Plague =============================================================

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "blood_plague", p, p -> find_spell( 55078 ) )
  {
    tick_may_crit    = true;
    background       = true;
    tick_power_mod   = data().extra_coeff();
    dot_behavior     = DOT_REFRESH;
    may_miss         = false;
    may_crit         = false;
    hasted_ticks     = false;
    base_multiplier += p -> spec.ebon_plaguebringer -> effectN( 2 ).percent();
    if ( p -> glyph.enduring_infection -> ok() )
      base_multiplier += p -> find_spell( 58671 ) -> effectN( 1 ).percent();
  }

  virtual double composite_crit()
  { return action_t::composite_crit() + player -> cache.attack_crit(); }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( ! sim -> overrides.weakened_blows && p() -> spec.scarlet_fever -> ok() &&
         result_is_hit( s -> result ) )
      s -> target -> debuffs.weakened_blows -> trigger();

    if ( ! sim -> overrides.physical_vulnerability && p() -> spec.ebon_plaguebringer -> ok() &&
         result_is_hit( s -> result ) )
      s -> target -> debuffs.physical_vulnerability -> trigger();
  }
};

// Blood Strike =============================================================

struct blood_strike_offhand_t : public death_knight_melee_attack_t
{
  blood_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "blood_strike_offhand", p, p -> find_spell( 66215 ) )
  {
    special          = true;
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    rp_gain          = 0;
    cost_blood       = 0;
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = death_knight_melee_attack_t::composite_target_multiplier( t );

    ctm *= 1 + cast_td() -> diseases() * 0.1875; // Currently giving a 18.75% increase per disease instead of expected 12.5

    return ctm;
  }
};

struct blood_strike_t : public death_knight_melee_attack_t
{
  melee_attack_t* oh_attack;

  blood_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "blood_strike", p, p -> find_spell( "Blood Strike" ) ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    special = true;

    weapon = &( p -> main_hand_weapon );

    if ( p -> spec.reaping -> ok() )
      convert_runes = 1.0;

    if ( p -> specialization() == DEATH_KNIGHT_FROST )
      convert_runes = 1.0;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new blood_strike_offhand_t( p );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = death_knight_melee_attack_t::composite_target_multiplier( t );

    ctm *= 1 + cast_td() -> diseases() * data().effectN( 3 ).base_value() / 1000.0;

    return ctm;
  }
};

// Soul Reaper ==============================================================

struct soul_reaper_dot_t : public death_knight_melee_attack_t
{
  soul_reaper_dot_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "soul_reaper_execute", p, p -> find_spell( 114867 ) )
  {
    special = background = may_crit = true;
    may_miss = false;
    weapon_multiplier = 0;
    direct_power_mod = data().extra_coeff();
  }

  virtual void init()
  {
    death_knight_melee_attack_t::init();
    stats = p() -> get_stats( name(), this );
  }
};

struct soul_reaper_t : public death_knight_melee_attack_t
{
  soul_reaper_dot_t* soul_reaper_dot;

  soul_reaper_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "soul_reaper", p, p -> find_specialization_spell( "Soul Reaper" ) ),
    soul_reaper_dot( 0 )
  {
    parse_options( NULL, options_str );
    special   = true;

    weapon = &( p -> main_hand_weapon );

    dynamic_tick_action = true;
    tick_action = new soul_reaper_dot_t( p );
    add_child( tick_action );
  }

  virtual double composite_crit()
  {
    double cc = death_knight_melee_attack_t::composite_crit();
    if ( player -> set_bonus.tier15_4pc_melee() && p() -> buffs.killing_machine -> check() )
      cc += p() -> buffs.killing_machine -> value();
    return cc;
  }

  double composite_ta_multiplier()
  {
    double m = death_knight_melee_attack_t::composite_ta_multiplier();

    if ( p() -> mastery.dreadblade -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  void init()
  {
    death_knight_melee_attack_t::init();

    snapshot_flags |= STATE_MUL_TA;
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();
    if ( player -> set_bonus.tier15_4pc_melee() && p() -> buffs.killing_machine -> check() )
    {
      p() -> procs.sr_killing_machine -> occur();
      p() -> buffs.killing_machine -> expire();
    }

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_soul_reaper -> execute();

    if ( result_is_hit( execute_state -> result ) )
      trigger_t16_2pc_tank( execute_state );
  }

  void tick( dot_t* dot )
  {
    int pct = p() -> set_bonus.tier15_4pc_melee() ? p() -> sets -> set( SET_T15_4PC_MELEE ) -> effectN( 1 ).base_value() : 35;

    if ( dot -> state -> target -> health_percentage() <= pct )
      death_knight_melee_attack_t::tick( dot );
  }
};

// Death Siphon =============================================================

struct death_siphon_heal_t : public death_knight_heal_t
{
  death_siphon_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "death_siphon_heal", p, p -> find_spell( 116783 ) )
  {
    background = true;
    may_crit = true;
    target = p;
  }
};

struct death_siphon_t : public death_knight_spell_t
{
  death_siphon_heal_t* heal;

  death_siphon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_siphon", p, p -> find_talent_spell( "Death Siphon" ) ),
    heal( new death_siphon_heal_t( p ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod = data().extra_coeff();
  }

  void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      heal -> base_dd_min = heal -> base_dd_max = s -> result_amount * data().effectN( 2 ).percent();
      heal -> schedule_execute();
    }

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_death_siphon -> execute();
  }
};

// Blood Tap ================================================================

struct blood_tap_t : public death_knight_spell_t
{
  int consume_charges;
  blood_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_tap", p, p -> find_talent_spell( "Blood Tap" ) ),
    consume_charges( 0 )
  {
    parse_options( NULL, options_str );

    harmful   = false;
    consume_charges = ( int  ) p -> find_spell( 114851 ) -> effectN( 1 ).base_value();
  }

  void execute()
  {
    // Blood tap prefers to refresh runes that are least valuable to you
    int selected_rune = random_depleted_rune( p() );

    // It's possible in the sim to get a regen event between ready() check and
    // execute(), causing a previously eligible rune to be filled. Thus,
    // if we find no rune to pop with blood tap, we just exit early
    if ( selected_rune == -1 )
    {
      death_knight_spell_t::execute();
      return;
    }

    if ( sim -> debug ) log_rune_status( p() );

    dk_rune_t* regen_rune = &( p() -> _runes.slot[ selected_rune ] );

    regen_rune -> fill_rune();
    regen_rune -> type |= RUNE_TYPE_DEATH;

    p() -> gains.blood_tap -> add( RESOURCE_RUNE, 1, 0 );
    if ( regen_rune -> is_blood() )
      p() -> gains.blood_tap_blood -> add( RESOURCE_RUNE, 1, 0 );
    else if ( regen_rune -> is_frost() )
      p() -> gains.blood_tap_frost -> add( RESOURCE_RUNE, 1, 0 );
    else
      p() -> gains.blood_tap_unholy -> add( RESOURCE_RUNE, 1, 0 );

    if ( sim -> log ) sim -> output( "%s regened rune %d", name(), selected_rune );

    p() -> buffs.blood_charge -> decrement( consume_charges );

    // Called last so we print the correct runes
    death_knight_spell_t::execute();
  }

  bool ready()
  {
    if ( p() -> buffs.blood_charge -> check() < consume_charges )
      return false;

    bool rd = death_knight_spell_t::ready();

    dk_rune_t& b = p() -> _runes.slot[ 0 ];
    if ( b.is_depleted() || b.paired_rune -> is_depleted() )
      return rd;

    dk_rune_t& f = p() -> _runes.slot[ 2 ];
    if ( f.is_depleted() || f.paired_rune -> is_depleted() )
      return rd;

    dk_rune_t& u = p() -> _runes.slot[ 4 ];
    if ( u.is_depleted() || u.paired_rune -> is_depleted() )
      return rd;

    return false;
  }
};

// Bone Shield ==============================================================

struct bone_shield_t : public death_knight_spell_t
{
  bone_shield_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "bone_shield", p, p -> find_specialization_spell( "Bone Shield" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    size_t max_stacks = p() -> buffs.bone_shield -> data().initial_stacks() +
                        p() -> buffs.bone_wall -> stack();

    if ( ! p() -> in_combat )
    {
      // Pre-casting it before the fight, perfect timing would be so
      // that the used rune is ready when it is needed in the
      // rotation.  Assume we casted Bone Shield somewhere between
      // 8-16s before we start fighting.  The cost in this case is
      // zero and we don't cause any cooldown.
      timespan_t pre_cast = timespan_t::from_seconds( p() -> sim -> range( 8.0, 16.0 ) );

      cooldown -> duration -= pre_cast;
      p() -> buffs.bone_shield -> buff_duration -= pre_cast;

      p() -> buffs.bone_shield -> trigger( max_stacks ); // FIXME
      death_knight_spell_t::execute();

      cooldown -> duration += pre_cast;
      p() -> buffs.bone_shield -> buff_duration += pre_cast;
    }
    else
    {
      p() -> buffs.bone_shield -> trigger( max_stacks ); // FIXME
      p() -> buffs.bone_wall -> expire();
      death_knight_spell_t::execute();
    }
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", p, p -> find_class_spell( "Dancing Rune Weapon" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.dancing_rune_weapon -> trigger();
    p() -> pets.dancing_rune_weapon -> summon( data().duration() );

    if ( p() -> set_bonus.tier16_4pc_tank() )
    {
      for ( int i = 2; i < RUNE_SLOT_MAX; ++i )
      {
        p() -> _runes.slot[ i ].fill_rune();
        p() -> _runes.slot[ i ].type |= RUNE_TYPE_DEATH;
      }

      p() -> buffs.deathbringer -> trigger( p() -> buffs.deathbringer -> data().initial_stacks() );
    }
  }
};

// Dark Transformation ======================================================

struct dark_transformation_t : public death_knight_spell_t
{
  dark_transformation_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "dark_transformation", p, p -> find_class_spell( "Dark Transformation" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.dark_transformation -> trigger();
    p() -> buffs.shadow_infusion -> expire();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.shadow_infusion -> check() != p() -> buffs.shadow_infusion -> max_stack() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Death and Decay ==========================================================

struct death_and_decay_t : public death_knight_spell_t
{
  death_and_decay_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_and_decay", p, p -> find_class_spell( "Death and Decay" ) )
  {
    parse_options( NULL, options_str );

    aoe              = -1;
    tick_power_mod   = 0.064;
    base_td          = data().effectN( 2 ).average( p );
    base_tick_time   = timespan_t::from_seconds( 1.0 );
    num_ticks        = ( int ) ( data().duration().total_seconds() / base_tick_time.total_seconds() ); // 11 with tick_zero
    tick_may_crit    = true;
    tick_zero        = true;
    hasted_ticks     = false;
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost()
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.crimson_scourge -> up() )
      p() -> buffs.crimson_scourge -> expire();
  }

  virtual void impact( action_state_t* s )
  {
    if ( s -> target -> debuffs.flying -> check() )
    {
      if ( sim -> debug ) sim -> output( "Ground effect %s can not hit flying target %s", name(), s -> target -> name() );
    }
    else
    {
      death_knight_spell_t::impact( s );
    }
  }

  virtual bool ready()
  {
    if ( ! spell_t::ready() )
      return false;

    if ( p() -> buffs.crimson_scourge -> check() )
      return group_runes( p(), 0, 0, 0, 0, use );
    else
      return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
  }
};

// Death Coil ===============================================================

struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", p, p -> find_class_spell( "Death Coil" ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod = 0.514;
    base_dd_min      = data().effectN( 1 ).min( p ); // "average" value of base damage scaling from spell data is inaccurate
    base_dd_max      = data().effectN( 1 ).max( p ); // it is overridden to 0.745 in sc_const_data.cpp apply_hotfixes()

    base_costs[ RESOURCE_RUNIC_POWER ] *= 1.0 + p -> spec.sudden_doom -> effectN( 2 ).percent();
  }

  virtual double cost()
  {
    if ( p() -> buffs.sudden_doom -> check() )
      return 0;

    return death_knight_spell_t::cost();
  }

  void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.sudden_doom -> decrement();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_death_coil -> execute();

    if ( ! p() -> buffs.dark_transformation -> check() )
      p() -> buffs.shadow_infusion -> trigger(); // Doesn't stack while your ghoul is empowered
  }

  void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> trigger_runic_empowerment();
      p() -> buffs.blood_charge -> trigger( 2 );
      if ( p() -> buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, timespan_t::from_seconds( 10.0 * 0.3 * p() -> cache.attack_haste() ) ) )
        p() -> buffs.tier13_4pc_melee -> trigger( 1, buff_t::DEFAULT_VALUE(), p() -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 2 ).percent() );

      if ( p() -> sets -> set( SET_T16_4PC_MELEE ) -> ok() && p() -> buffs.dark_transformation -> check() )
        p() -> buffs.dark_transformation -> extend_duration( p(), timespan_t::from_millis( p() -> sets -> set( SET_T16_4PC_MELEE ) -> effectN( 1 ).base_value() ) );

      trigger_t16_2pc_tank( s );
    }
  }
};

// Death Strike =============================================================

struct death_strike_offhand_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "death_strike_offhand", p, p -> find_spell( 66188 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
  }
};

struct death_strike_heal_t : public death_knight_heal_t
{
  death_strike_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "death_strike_heal", p, p -> find_spell( 45470 ) )
  {
    may_crit   = false;
    background = true;
    target     = p;
  }
};

struct death_strike_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t* oh_attack;
  death_strike_heal_t* heal;

  death_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "death_strike", p, p -> find_class_spell( "Death Strike" ) ),
    oh_attack( 0 ), heal( new death_strike_heal_t( p ) )
  {
    parse_options( NULL, options_str );
    special = true;
    may_parry = false;
    base_multiplier = 1.0 + p -> spec.blood_rites -> effectN( 2 ).percent();

    always_consume = true; // Death Strike always consumes runes, even if doesn't hit

    if ( p -> spec.blood_rites -> ok() )
      convert_runes = 1.0;

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> spec.threat_of_thassarian -> ok() )
        oh_attack = new death_strike_offhand_t( p );
    }
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.deathbringer -> check() )
      return;

    death_knight_melee_attack_t::consume_resource();
  }

  virtual double cost()
  {
    if ( p() -> buffs.deathbringer -> check() )
      return 0;

    return death_knight_melee_attack_t::cost();
  }

  void trigger_death_strike_heal()
  {
    if ( p() -> incoming_damage.size() > 0 )
    {
      std::vector< std::pair< timespan_t, double > >::iterator i = p() -> incoming_damage.begin();
      while ( i != p() -> incoming_damage.end() &&
              sim -> current_time - ( *i ).first > timespan_t::from_seconds( 5.0 ) )
      {
        ++i;
      }

      p() -> incoming_damage.erase( p() -> incoming_damage.begin(), i );
    }

    double heal_amount = 0;
    for ( size_t i = 0; i < p() -> incoming_damage.size(); i++ )
      heal_amount += p() -> incoming_damage[ i ].second;

    if ( sim -> debug )
      sim -> output( "%s Death Strike heal, incoming_damage=%f incoming_heal=%f min_heal=%f", p() -> name(), heal_amount,
                     heal_amount * data().effectN( 1 ).chain_multiplier() / 100.0,
                     p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 3 ).percent() );

    heal_amount = std::max( p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 3 ).percent(),
                            heal_amount * data().effectN( 1 ).chain_multiplier() / 100.0 );

    // Note that the Scent of Blood bonus is calculated here, so we can get
    // it to Blood Shield. Apparently any other healing bonus that the DKs get
    // does not affect the size of the blood shield
    heal_amount *= 1.0 + p() -> buffs.scent_of_blood -> check() * p() -> buffs.scent_of_blood -> data().effectN( 1 ).percent();

    heal -> base_dd_min = heal -> base_dd_max = heal_amount;
    heal -> schedule_execute();
  }

  void trigger_blood_shield()
  {
    if ( p() -> specialization() != DEATH_KNIGHT_BLOOD )
      return;

    double amount = p() -> buffs.blood_shield -> current_value;

    if ( p() -> mastery.blood_shield -> ok() )
      amount += heal -> base_dd_min * p() -> cache.mastery_value();

    if ( sim -> debug )
      sim -> output( "%s Blood Shield buff trigger, old_value=%f added_value=%f new_value=%f",
                     player -> name(), p() -> buffs.blood_shield -> current_value,
                     heal -> base_dd_min * p() -> cache.mastery_value(),
                     amount );

    p() -> buffs.blood_shield -> trigger( 1, amount );
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( oh_attack )
      oh_attack -> execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_death_strike -> execute();

    trigger_death_strike_heal();
    trigger_blood_shield();

    if ( result_is_hit( execute_state -> result ) )
      trigger_t16_2pc_tank( execute_state );

    p() -> buffs.scent_of_blood -> expire();
    p() -> buffs.deathbringer -> decrement();
  }

  virtual bool ready()
  {
    if ( ! melee_attack_t::ready() )
      return false;

    if ( p() -> buffs.deathbringer -> check() )
      return group_runes( p(), 0, 0, 0, 0, use );
    else
      return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
  }
};

// Empower Rune Weapon ======================================================

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "empower_rune_weapon", p, p -> find_spell( 47568 ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    double erw_gain = 0.0;
    double erw_over = 0.0;
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p() -> _runes.slot[ i ];
      erw_gain += 1 - r.value;
      erw_over += r.value;
      r.fill_rune();
    }
    p() -> gains.empower_rune_weapon -> add( RESOURCE_RUNE, erw_gain, erw_over );
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_melee_attack_t
{
  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> find_class_spell( "Festering Strike" ) )
  {
    parse_options( NULL, options_str );

    if ( p -> spec.reaping -> ok() )
      convert_runes = 1.0;
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      cast_td( s -> target ) -> dots_blood_plague -> extend_duration_seconds( timespan_t::from_seconds( data().effectN( 3 ).base_value() ), 0 );
      cast_td( s -> target ) -> dots_frost_fever  -> extend_duration_seconds( timespan_t::from_seconds( data().effectN( 3 ).base_value() ), 0 );
    }
  }
};

// Frost Fever ==============================================================

struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( death_knight_t* p ) :
    death_knight_spell_t( "frost_fever", p, p -> find_spell( 55095 ) )
  {
    base_td          = data().effectN( 1 ).average( p );
    hasted_ticks     = false;
    may_miss         = false;
    may_crit         = false;
    background       = true;
    tick_may_crit    = true;
    dot_behavior     = DOT_REFRESH;
    tick_power_mod   = data().extra_coeff();
    base_multiplier += p -> spec.ebon_plaguebringer -> effectN( 2 ).percent();
    if ( p -> glyph.enduring_infection -> ok() )
      base_multiplier += p -> find_spell( 58671 ) -> effectN( 1 ).percent();
  }

  virtual double composite_crit()
  { return action_t::composite_crit() + player -> cache.attack_crit(); }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( ! sim -> overrides.physical_vulnerability && p() -> spec.brittle_bones -> ok() &&
         result_is_hit( s -> result ) )
      s -> target -> debuffs.physical_vulnerability -> trigger();
  }

};

// Frost Strike =============================================================

struct frost_strike_offhand_t : public death_knight_melee_attack_t
{
  frost_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "frost_strike_offhand", p, p -> find_spell( 66196 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;
    base_multiplier += p -> spec.threat_of_thassarian -> effectN( 3 ).percent() +
                       p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    rp_gain = 0; // Incorrectly set to 10 in the DBC
  }

  virtual double composite_crit()
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }
};

struct frost_strike_t : public death_knight_melee_attack_t
{
  frost_strike_offhand_t* oh_attack;

  frost_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "frost_strike", p, p -> find_class_spell( "Frost Strike" ) ),
    oh_attack( 0 )
  {
    special = true;
    base_multiplier += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    parse_options( NULL, options_str );

    weapon     = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      base_multiplier += p -> spec.threat_of_thassarian -> effectN( 3 ).percent();

      if ( p -> spec.threat_of_thassarian -> ok() )
        oh_attack = new frost_strike_offhand_t( p );
    }
  }

  virtual double cost()
  {
    double c = death_knight_melee_attack_t::cost();

    if ( p() -> buffs.frost_presence -> check() )
      c += p() -> spec.improved_frost_presence -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );

    return c;
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( oh_attack )
        oh_attack -> execute();

      if ( p() -> buffs.killing_machine -> check() )
        p() -> procs.fs_killing_machine -> occur();

      p() -> buffs.killing_machine -> expire();
    }
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> trigger_runic_empowerment();
      p() -> buffs.blood_charge -> trigger( 2 );
      if ( p() -> buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, timespan_t::from_seconds( 10.0 * 0.3 * p() -> cache.attack_haste() ) ) )
        p() -> buffs.tier13_4pc_melee -> trigger( 1, buff_t::DEFAULT_VALUE(), p() -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 2 ).percent() );
    }
  }

  virtual double composite_crit()
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }
};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_melee_attack_t
{
  heart_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "heart_strike", p, p -> find_class_spell( "Heart Strike" ) )
  {
    parse_options( NULL, options_str );

    special = true;

    aoe = 3;
    base_add_multiplier = 0.75;
  }

  void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> buffs.dancing_rune_weapon -> check() )
        p() -> pets.dancing_rune_weapon -> drw_heart_strike -> execute();

      trigger_t16_2pc_tank( execute_state );
    }
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = death_knight_melee_attack_t::composite_target_multiplier( t );

    ctm *= 1.0 + cast_td( t ) -> diseases() * data().effectN( 3 ).percent();

    return ctm;
  }
};

// Horn of Winter============================================================

struct horn_of_winter_t : public death_knight_spell_t
{
  horn_of_winter_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "horn_of_winter", p, p -> find_spell( 57330 ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( ! sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );

    //player -> resource_gain( RESOURCE_RUNIC_POWER, 10, p() -> gains.horn_of_winter );
  }
};

// Howling Blast ============================================================

// FIXME: -3% spell crit suppression? Seems to have the same crit chance as FS in the absence of KM
struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> find_class_spell( "Howling Blast" ) )
  {
    parse_options( NULL, options_str );

    aoe                 = -1;
    base_aoe_multiplier = data().effectN( 3 ).percent();
    direct_power_mod    = 0.681;

    assert( p -> active_spells.frost_fever );
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.rime -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost()
  {
    // Rime also prevents getting RP because there are no runes used!
    if ( p() -> buffs.rime -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.rime -> decrement();
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> active_spells.frost_fever -> target = s -> target;
      p() -> active_spells.frost_fever -> execute();
    }
  }

  virtual bool ready()
  {
    if ( p() -> buffs.rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Icy Touch ================================================================

struct icy_touch_t : public death_knight_spell_t
{
  icy_touch_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "icy_touch", p, p -> find_class_spell( "Icy Touch" ) )
  {
    parse_options( NULL, options_str );

    direct_power_mod = 0.319;

    if ( p -> spec.reaping -> ok() )
      convert_runes = 1.0;

    assert( p -> active_spells.frost_fever );
  }

  virtual void consume_resource()
  {
    // We only consume resources when rime is not up
    if ( p() -> buffs.rime -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost()
  {
    // Rime also prevents getting RP because there are no runes used!
    if ( p() -> buffs.rime -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_icy_touch -> execute();

    p() -> buffs.rime -> decrement();
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> active_spells.frost_fever -> target = s->target;
      p() -> active_spells.frost_fever -> execute();
    }
  }

  virtual bool ready()
  {
    if ( p() -> buffs.rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "mind_freeze", p, p -> find_class_spell( "Mind Freeze" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Necrotic Strike ==========================================================

struct necrotic_strike_t : public death_knight_melee_attack_t
{
  necrotic_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "necrotic_strike", p, p -> find_class_spell( "Necrotic Strike" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );

    special = true;
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( ! sim -> overrides.slowed_casting && result_is_hit( s -> result ) )
      s -> target -> debuffs.slowed_casting -> trigger();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_necrotic_strike -> execute();
  }
};

// Obliterate ===============================================================

struct obliterate_offhand_t : public death_knight_melee_attack_t
{
  obliterate_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "obliterate_offhand", p, p -> find_spell( 66198 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;
    base_multiplier += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    // These both stack additive with MOTFW
    // http://elitistjerks.com/f72/t110296-frost_dps_cataclysm_4_0_6_my_life/p14/#post1886388
  }

  virtual double composite_crit()
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = death_knight_melee_attack_t::composite_target_multiplier( t );

    ctm *= 1.0 + cast_td() -> diseases() * data().effectN( 3 ).percent() / 2.0;

    return ctm;
  }
};

struct obliterate_t : public death_knight_melee_attack_t
{
  obliterate_offhand_t* oh_attack;

  obliterate_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "obliterate", p, p -> find_class_spell( "Obliterate" ) ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    special = true;
    base_multiplier += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    weapon = &( p -> main_hand_weapon );

    // These both stack additive with MOTFW
    // http://elitistjerks.com/f72/t110296-frost_dps_cataclysm_4_0_6_my_life/p14/#post1886388

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> spec.threat_of_thassarian -> ok() )
        oh_attack = new obliterate_offhand_t( p );
    }

    if ( p -> main_hand_weapon.group() == WEAPON_2H )
      weapon_multiplier *= 1.0 + p -> spec.might_of_the_frozen_wastes -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( oh_attack )
        oh_attack -> execute();

      if ( p() -> buffs.killing_machine -> check() )
        p() -> procs.oblit_killing_machine -> occur();

      p() -> buffs.killing_machine -> expire();
    }
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> rng.rime -> roll( p() -> spec.rime -> proc_chance() ) )
      {
        // T13 2pc gives 2 stacks of Rime, otherwise we can only ever have one
        // Ensure that if we have 1 that we only refresh, not add another stack
        int new_stacks = 1;
        if ( p() -> rng.t13_2pc_melee -> roll( p() -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 2 ).percent() ) )
          new_stacks++;

        // If we're proccing 2 or we have 0 stacks, trigger like normal
        if ( new_stacks == 2 || p() -> buffs.rime -> check() == 0 )
          p() -> buffs.rime -> trigger( new_stacks );
        // refresh stacks. However if we have a double stack and only 1 procced, it refreshes to 1 stack
        else
        {
          p() -> buffs.rime -> refresh( 0 );
          if ( p() -> buffs.rime -> check() == 2 && new_stacks == 1 )
            p() -> buffs.rime -> decrement( 1 );
        }
      }
    }
  }

  virtual double composite_crit()
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = death_knight_melee_attack_t::composite_target_multiplier( t );

    ctm *= 1.0 + cast_td() -> diseases() * data().effectN( 3 ).percent() / 2.0;

    return ctm;
  }
};

// Outbreak =================================================================

struct outbreak_t : public death_knight_spell_t
{
  outbreak_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "outbreak", p, p -> find_class_spell( "Outbreak" ) )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    cooldown -> duration *= 1.0 + p -> glyph.outbreak -> effectN( 1 ).percent();
    base_costs[ RESOURCE_RUNIC_POWER ] += p -> glyph.outbreak -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );

    assert( p -> active_spells.blood_plague );
    assert( p -> active_spells.frost_fever );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> active_spells.blood_plague -> target = target;
      p() -> active_spells.blood_plague -> execute();

      p() -> active_spells.frost_fever -> target = target;
      p() -> active_spells.frost_fever -> execute();
    }

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_outbreak -> execute();
  }
};

// Pestilence ===============================================================

struct pestilence_t : public death_knight_spell_t
{
  pestilence_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pestilence", p, p -> find_class_spell( "Pestilence" ) )
  {
    parse_options( NULL, options_str );

    if ( p -> spec.reaping -> ok() )
      convert_runes = 1.0;

    aoe = -1;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_pestilence -> execute();
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    // Doesn't affect the original target
    if ( s -> target == target )
      return;

    if ( result_is_hit( s -> result ) )
    {
      if ( cast_td() -> dots_blood_plague -> ticking )
      {
        p() -> active_spells.blood_plague -> target = s -> target;
        p() -> active_spells.blood_plague -> execute();
      }
      if ( cast_td() -> dots_frost_fever -> ticking )
      {
        p() -> active_spells.frost_fever -> target = s -> target;
        p() -> active_spells.frost_fever -> execute();
      }
    }
  }

  virtual bool ready()
  {
    if ( ! death_knight_spell_t::ready() )
      return false;

    // BP or FF must be ticking to use
    return cast_td() -> diseases() > 0;
  }
};

// Pillar of Frost ==========================================================

struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", p, p -> find_class_spell( "Pillar of Frost" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.pillar_of_frost -> trigger();
  }

  bool ready()
  {
    if ( ! spell_t::ready() )
      return false;

    // Always activate runes, even pre-combat.
    return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
  }
};

// Plague Strike ============================================================

struct plague_strike_offhand_t : public death_knight_melee_attack_t
{
  plague_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "plague_strike_offhand", p, p -> find_spell( 66216 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;

    assert( p -> active_spells.blood_plague );
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();


  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> active_spells.blood_plague -> target = s->target;
      p() -> active_spells.blood_plague -> execute();

      if ( p() -> spec.ebon_plaguebringer -> ok() )
      {
        p() -> active_spells.frost_fever -> target = s->target;
        p() -> active_spells.frost_fever -> execute();
      }
    }
  }
};

struct plague_strike_t : public death_knight_melee_attack_t
{
  plague_strike_offhand_t* oh_attack;

  plague_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "plague_strike", p, p -> find_class_spell( "Plague Strike" ) ), oh_attack( 0 )
  {
    parse_options( NULL, options_str );

    special = true;

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> spec.threat_of_thassarian -> ok() )
        oh_attack = new plague_strike_offhand_t( p );
    }

    assert( p -> active_spells.blood_plague );
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_plague_strike -> execute();

    if ( result_is_hit( execute_state -> result ) && oh_attack )
      oh_attack -> execute();
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> active_spells.blood_plague -> target = s->target;
      p() -> active_spells.blood_plague -> execute();

      if ( p() -> spec.ebon_plaguebringer -> ok() )
      {
        p() -> active_spells.frost_fever -> target = s->target;
        p() -> active_spells.frost_fever -> execute();
      }
    }
  }
};

// Presence =================================================================

struct presence_t : public death_knight_spell_t
{
  death_knight_presence switch_to_presence;
  presence_t( death_knight_t* p, const std::string& n, death_knight_presence pres, const std::string& options_str ) :
    death_knight_spell_t( n, p ), switch_to_presence( pres )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero();
    cooldown -> duration = timespan_t::from_seconds( 1.0 );
    harmful     = false;
  }

  virtual resource_e current_resource()
  {
    return RESOURCE_RUNIC_POWER;
  }

  virtual double cost()
  {
    // Presence changes consume all runic power
    return p() -> resources.current [ RESOURCE_RUNIC_POWER ] * ( 1.0 - p() -> glyph.shifting_presences -> effectN( 1 ).percent() );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    switch ( p() -> active_presence )
    {
      case PRESENCE_BLOOD:  p() -> buffs.blood_presence  -> expire(); break;
      case PRESENCE_FROST:  p() -> buffs.frost_presence  -> expire(); break;
      case PRESENCE_UNHOLY: p() -> buffs.unholy_presence -> expire(); break;
    }

    p() -> active_presence = switch_to_presence;

    switch ( p() -> active_presence )
    {
      case PRESENCE_BLOOD:  p() -> buffs.blood_presence  -> trigger(); break;
      case PRESENCE_FROST:  p() -> buffs.frost_presence  -> trigger(); break;
      case PRESENCE_UNHOLY: p() -> buffs.unholy_presence -> trigger(); break;
    }

    p() -> recalculate_resource_max( RESOURCE_HEALTH );
  }

  virtual bool ready()
  {
    if ( p() -> active_presence == switch_to_presence )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Blood Presence ===========================================================

struct blood_presence_t : public presence_t
{
  blood_presence_t( death_knight_t* p, const std::string& options_str ) :
    presence_t( p, "blood_presence", PRESENCE_BLOOD, options_str )
  {
    parse_options( NULL, options_str );
    id = p -> find_class_spell( "Blood Presence" ) -> id();
  }
};

// Frost Presence ===========================================================

struct frost_presence_t : public presence_t
{
  frost_presence_t( death_knight_t* p, const std::string& options_str ) :
    presence_t( p, "frost_presence", PRESENCE_FROST, options_str )
  {
    parse_options( NULL, options_str );
    id = p -> find_class_spell( "Frost Presence" ) -> id();
  }
};

// Unholy Presence ==========================================================

struct unholy_presence_t : public presence_t
{
  unholy_presence_t( death_knight_t* p, const std::string& options_str ) :
    presence_t( p, "unholy_presence", PRESENCE_UNHOLY, options_str )
  {
    parse_options( NULL, options_str );
    id = p -> find_class_spell( "Unholy Presence" ) -> id();
  }
};


// Raise Dead ===============================================================

struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", p, p -> find_class_spell( "Raise Dead" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( p() -> specialization() == DEATH_KNIGHT_UNHOLY )
      p() -> pets.ghoul_pet -> summon( timespan_t::zero() );
    else
      p() -> pets.ghoul_guardian -> summon( p() -> dbc.spell( data().effectN( 1 ).base_value() ) -> duration() );
  }

  virtual bool ready()
  {
    if ( ( p() -> pets.ghoul_pet && ! p() -> pets.ghoul_pet -> is_sleeping() ) ||
         ( p() -> pets.ghoul_guardian && ! p() -> pets.ghoul_guardian -> is_sleeping() ) )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Rune Strike ==============================================================

struct rune_strike_t : public death_knight_melee_attack_t
{
  rune_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "rune_strike", p, p -> find_specialization_spell( "Rune Strike" ) )
  {
    parse_options( NULL, options_str );

    special          = true;
    may_dodge = may_block = may_parry = false;

    weapon = &( p -> main_hand_weapon );
  }

  void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
      trigger_t16_2pc_tank( execute_state );
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> trigger_runic_empowerment();
      p() -> buffs.blood_charge -> trigger( 2 );
      if ( p() -> buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, timespan_t::from_seconds( 10.0 * 0.3 * p() -> cache.attack_haste() ) ) )
        p() -> buffs.tier13_4pc_melee -> trigger( 1, buff_t::DEFAULT_VALUE(), p() -> sets -> set( SET_T13_4PC_MELEE ) -> effectN( 2 ).percent() );
    }
  }
};

// Scourge Strike ===========================================================

struct scourge_strike_t : public death_knight_melee_attack_t
{
  spell_t* scourge_strike_shadow;

  struct scourge_strike_shadow_t : public death_knight_spell_t
  {
    double   disease_coeff;

    scourge_strike_shadow_t( death_knight_t* p ) :
      death_knight_spell_t( "scourge_strike_shadow", p, p -> find_spell( 70890 ) ),
      disease_coeff( 0 )
    {
      may_miss = may_parry = may_dodge = may_crit = false;
      special = proc = background = true;
      weapon = &( player -> main_hand_weapon );
      weapon_multiplier = 0;
      disease_coeff = p -> find_class_spell( "Scourge Strike" ) -> effectN( 3 ).percent();
      dual = true;
    }

    virtual void init()
    {
      death_knight_spell_t::init();
      stats = p() -> get_stats( name(), this );
    }

    double composite_target_multiplier( player_t* target )
    {
      double m = death_knight_spell_t::composite_target_multiplier( target );

      m *= disease_coeff * cast_td( target ) -> diseases();

      return m;
    }
  };

  scourge_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "scourge_strike", p, p -> find_class_spell( "Scourge Strike" ) ),
    scourge_strike_shadow( 0 )
  {
    parse_options( NULL, options_str );

    special = true;
    base_multiplier += p -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    scourge_strike_shadow = new scourge_strike_shadow_t( p );
    add_child( scourge_strike_shadow );
  }

  void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && cast_td( s -> target ) -> diseases() > 0 )
    {
      scourge_strike_shadow -> base_dd_max = scourge_strike_shadow -> base_dd_min = s -> result_amount;
      scourge_strike_shadow -> execute();
    }
  }
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "summon_gargoyle", p, p -> find_class_spell( "Summon Gargoyle" ) )
  {
    rp_gain = 0.0;  // For some reason, the inc file thinks we gain RP for this spell
    parse_options( NULL, options_str );
    num_ticks = 0;
    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> pets.gargoyle -> summon( data().effectN( 3 ).trigger() -> duration() );
  }
};

// Unholy Blight ============================================================

struct unholy_blight_tick_t : public death_knight_spell_t
{
  unholy_blight_tick_t( death_knight_t* p ) :
    death_knight_spell_t( "unholy_blight_tick", p )
  {
    aoe        = -1;
    background = true;
    may_miss   = false;
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    p() -> active_spells.blood_plague -> target = s -> target;
    p() -> active_spells.blood_plague -> execute();

    p() -> active_spells.frost_fever -> target = s -> target;
    p() -> active_spells.frost_fever -> execute();
  }
};

struct unholy_blight_t : public death_knight_spell_t
{
  unholy_blight_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "unholy_blight", p, p -> find_talent_spell( "Unholy Blight" ) )
  {
    parse_options( NULL, options_str );

    may_crit     = false;
    may_miss     = false;
    hasted_ticks = false;

    assert( p -> active_spells.blood_plague );
    assert( p -> active_spells.frost_fever );

    tick_action = new unholy_blight_tick_t( p );
  }
};

// Unholy Frenzy ============================================================

struct unholy_frenzy_t : public death_knight_spell_t
{
  unholy_frenzy_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "unholy_frenzy", p, p -> find_class_spell( "Unholy Frenzy" ) )
  {
    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
      target = p;
    harmful = false;
    num_ticks = 0;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    target -> buffs.unholy_frenzy -> trigger( 1,
        data().effectN( 1 ).percent() +
        p() -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 2 ).percent() );
  }
};

// Plague Leech =============================================================

struct plague_leech_t : public death_knight_spell_t
{
  plague_leech_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "plague_leech", p, p -> find_talent_spell( "Plague Leech" ) )
  {
    may_crit = may_miss = false;

    parse_options( NULL, options_str );
  }

  void impact( action_state_t* state )
  {
    death_knight_spell_t::impact( state );

    int selected_rune = random_depleted_rune( p() );
    if ( selected_rune == -1 )
      return;

    dk_rune_t* regen_rune = &( p() -> _runes.slot[ selected_rune ] );

    regen_rune -> fill_rune();
    regen_rune -> type |= RUNE_TYPE_DEATH;

    p() -> gains.plague_leech -> add( RESOURCE_RUNE, 1, 0 );

    if ( sim -> log ) sim -> output( "%s regened rune %d", name(), selected_rune );

    cast_td( state -> target ) -> dots_frost_fever -> cancel();
    cast_td( state -> target ) -> dots_blood_plague -> cancel();
  }

  bool ready()
  {
    if ( ! cast_td() -> dots_frost_fever -> ticking ||
         ! cast_td() -> dots_blood_plague -> ticking )
      return false;

    bool rd = death_knight_spell_t::ready();

    dk_rune_t& b = p() -> _runes.slot[ 0 ];
    if ( b.is_depleted() || b.paired_rune -> is_depleted() )
      return rd;

    dk_rune_t& f = p() -> _runes.slot[ 2 ];
    if ( f.is_depleted() || f.paired_rune -> is_depleted() )
      return rd;

    dk_rune_t& u = p() -> _runes.slot[ 4 ];
    if ( u.is_depleted() || u.paired_rune -> is_depleted() )
      return rd;

    return false;
  }
};

// Anti-magic Shell =========================================================

struct antimagic_shell_t : public death_knight_spell_t
{
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;
  rng_t* rng;

  antimagic_shell_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "antimagic_shell", p, p -> find_class_spell( "Anti-Magic Shell" ) ),
    interval( 60 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( 0 ),
    rng( p -> get_rng( "antimagic_shell" ) )
  {
    harmful = false;
    base_dd_min = base_dd_max = 0;

    option_t options[] =
    {
      opt_float( "interval", interval ),
      opt_float( "interval_stddev", interval_stddev_opt ),
      opt_float( "damage", damage )
    };

    parse_options( options, options_str );

    if ( interval < data().cooldown().total_seconds() )
    {
      sim -> errorf( "%s minimum interval for Anti-Magic Shell is %.3f seconds.", p -> name(), data().cooldown().total_seconds() );
      interval = data().cooldown().total_seconds();
    }

    // Less than a second standard deviation is translated to a percent of
    // interval
    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;
  }

  void execute()
  {
    timespan_t new_cd = timespan_t::from_seconds( rng -> gauss( interval, interval_stddev ) );
    if ( new_cd < data().cooldown() )
      new_cd = data().cooldown();

    cooldown -> duration = new_cd;

    death_knight_spell_t::execute();

    double absorbed = std::max( damage * data().effectN( 1 ).percent(),
                                p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent() );

    p() -> buffs.antimagic_shell -> trigger( 1, absorbed / 1000.0 / data().duration().total_seconds() );
  }

  bool ready()
  {
    if ( damage == 0 ) return false;
    return death_knight_spell_t::ready();
  }
};

// Vampiric Blood ===========================================================

struct vampiric_blood_t : public death_knight_spell_t
{
  vampiric_blood_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "vampiric_blood", p, p -> find_specialization_spell( "Vampiric Blood" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    base_dd_min = base_dd_max = 0;
  }

  void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.vampiric_blood -> trigger();
  }
};

// Icebound Fortitude =======================================================

struct icebound_fortitude_t : public death_knight_spell_t
{
  icebound_fortitude_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "icebound_fortitude", p, p -> find_class_spell( "Icebound Fortitude" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    cooldown -> duration = data().duration() * ( 1.0 + p -> glyph.icebound_fortitude -> effectN( 1 ).percent() );
    if ( p -> spec.sanguine_fortitude -> ok() )
      base_costs[ RESOURCE_RUNIC_POWER ] = 0;
  }

  void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.icebound_fortitude -> trigger();
  }
};

// Rune Tap

struct rune_tap_t : public death_knight_heal_t
{
  rune_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_heal_t( "rune_tap", p, p -> find_specialization_spell( "Rune Tap" ) )
  {
    parse_options( NULL, options_str );
  }

  double base_da_min( const action_state_t* )
  { return p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent(); }

  double base_da_max( const action_state_t* )
  { return p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent(); }

  void consume_resource()
  {
    if ( p() -> buffs.will_of_the_necropolis_rt -> check() )
      return;

    death_knight_heal_t::consume_resource();
  }

  double cost()
  {
    if ( p() -> buffs.will_of_the_necropolis_rt -> check() )
      return 0;
    return death_knight_heal_t::cost();
  }

  bool ready()
  {
    if ( p() -> buffs.will_of_the_necropolis_rt-> check() )
    {
      cost_blood = 0;
      bool r = death_knight_heal_t::ready();
      cost_blood  = 1;
      return r;
    }
    return death_knight_heal_t::ready();
  }
};

// Buffs ====================================================================

struct runic_corruption_regen_t : public event_t
{
  buff_t* buff;

  runic_corruption_regen_t( death_knight_t* p, buff_t* b ) :
    event_t( p, "runic_corruption_regen_event" ),
    buff( b )
  {
    sim.add_event( this, timespan_t::from_seconds( 0.1 ) );
  }

  void execute();
};

struct runic_corruption_buff_t : public buff_t
{
  event_t* regen_event;

  runic_corruption_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "runic_corruption", p -> find_spell( 51460 ) )
            .chance( p -> talent.runic_corruption -> proc_chance() ) ),
    regen_event( 0 )
  { }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() )
  {
    buff_t::execute( stacks, value, duration );
    if ( sim -> debug )
      sim_t::output( sim, "%s runic_corruption_regen_event duration=%f", player -> name(), duration.total_seconds() );
    if ( ! regen_event )
    {
      death_knight_t* p = debug_cast< death_knight_t* >( player );
      regen_event = new ( *sim ) runic_corruption_regen_t( p, this );
    }
  }

  void expire_override()
  {
    buff_t::expire_override();

    if ( regen_event )
    {
      timespan_t remaining_duration = timespan_t::from_seconds( 0.1 ) - ( regen_event -> occurs() - sim -> current_time );

      if ( sim -> debug )
        sim_t::output( sim, "%s runic_corruption_regen_event expires, remaining duration %f", player -> name(), remaining_duration.total_seconds() );

      death_knight_t* p = debug_cast< death_knight_t* >( player );
      for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
        p -> _runes.slot[i].regen_rune( p, remaining_duration, true );

      event_t::cancel( regen_event );
    }
  }
};

void runic_corruption_regen_t::execute()
{
  death_knight_t* p = debug_cast< death_knight_t* >( player );
  runic_corruption_buff_t* regen_buff = debug_cast< runic_corruption_buff_t* >( buff );

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    p -> _runes.slot[i].regen_rune( p, timespan_t::from_seconds( 0.1 ), true );

  regen_buff -> regen_event = new ( sim ) runic_corruption_regen_t( p, buff );
}

struct vampiric_blood_buff_t : public buff_t
{
  int health_gain;

  vampiric_blood_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "vampiric_blood", p -> find_specialization_spell( "Vampiric Blood" ) ) ),
    health_gain ( 0 )
  { }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() )
  {
    buff_t::execute( stacks, value, duration );

    death_knight_t* p = debug_cast< death_knight_t* >( player );
    if ( ! p -> glyph.vampiric_blood -> ok() )
    {
      health_gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent() );
      player -> stat_gain( STAT_MAX_HEALTH, health_gain );
      player -> stat_gain( STAT_HEALTH, health_gain );
    }
  }

  void expire_override()
  {
    buff_t::expire_override();

    if ( health_gain > 0 )
    {
      player -> stat_loss( STAT_MAX_HEALTH, health_gain );
      player -> stat_loss( STAT_HEALTH, health_gain );
    }
  }
};

} // UNNAMED NAMESPACE

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "antimagic_shell"          ) return new antimagic_shell_t          ( this, options_str );
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "blood_presence"           ) return new blood_presence_t           ( this, options_str );
  if ( name == "unholy_presence"          ) return new unholy_presence_t          ( this, options_str );
  if ( name == "frost_presence"           ) return new frost_presence_t           ( this, options_str );
  if ( name == "soul_reaper"              ) return new soul_reaper_t              ( this, options_str );
  if ( name == "plague_leech"             ) return new plague_leech_t             ( this, options_str );
  if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blood_strike"             ) return new blood_strike_t             ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "pestilence"               ) return new pestilence_t               ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
  if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "icy_touch"                ) return new icy_touch_t                ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
  if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "necrotic_strike"          ) return new necrotic_strike_t          ( this, options_str );
  if ( name == "plague_strike"            ) return new plague_strike_t            ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
  if ( name == "unholy_frenzy"            ) return new unholy_frenzy_t            ( this, options_str );

  // Talents
  if ( name == "unholy_blight"            ) return new unholy_blight_t            ( this, options_str );
  if ( name == "death_siphon"             ) return new death_siphon_t             ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

expr_t* death_knight_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( util::str_compare_ci( splits[ 0 ], "rune" ) )
  {
    rune_type rt = RUNE_TYPE_NONE;
    bool include_death = true; // whether to include death runes
    switch ( splits[ 1 ][0] )
    {
      case 'B': include_death = false;
      case 'b': rt = RUNE_TYPE_BLOOD; break;
      case 'U': include_death = false;
      case 'u': rt = RUNE_TYPE_UNHOLY; break;
      case 'F': include_death = false;
      case 'f': rt = RUNE_TYPE_FROST; break;
      case 'D': rt = RUNE_TYPE_DEATH; break;
      case 'd': rt = RUNE_TYPE_DEATH; break;
    }
    int position = 0; // any
    switch ( splits[1][splits[1].size() - 1] )
    {
      case '1': position = 1; break;
      case '2': position = 2; break;
    }

    int act = 0;
    if ( splits.size() == 3 && util::str_compare_ci( splits[ 2 ], "cooldown_remains" ) )
      act = 1;
    else if ( splits.size() == 3 && util::str_compare_ci( splits[ 2 ], "cooldown_remains_all" ) )
      act = 2;
    else if ( splits.size() == 3 && util::str_compare_ci( splits[ 2 ], "depleted" ) )
      act = 3;

    struct rune_inspection_expr_t : public expr_t
    {
      death_knight_t* dk;
      rune_type r;
      bool include_death;
      int position;
      int myaction; // -1 count, 0 cooldown remains, 1 cooldown_remains_all

      rune_inspection_expr_t( death_knight_t* p, rune_type r, bool include_death, int position, int myaction )
        : expr_t( "rune_evaluation" ), dk( p ), r( r ),
          include_death( include_death ), position( position ), myaction( myaction )
      { }

      virtual double evaluate()
      {
        switch ( myaction )
        {
          case 0: return dk -> runes_count( r, include_death, position );
          case 1: return dk -> runes_cooldown_any( r, include_death, position );
          case 2: return dk -> runes_cooldown_all( r, include_death, position );
          case 3: return dk -> runes_depleted( r, position );
        }
        return 0.0;
      }
    };
    return new rune_inspection_expr_t( this, rt, include_death, position, act );
  }
  else if ( splits.size() == 2 )
  {
    rune_type rt = RUNE_TYPE_NONE;
    bool include_death = true;
    switch ( splits[ 0 ][ 0 ] )
    {
      case 'B': include_death = false;
      case 'b': rt = RUNE_TYPE_BLOOD; break;
      case 'U': include_death = false;
      case 'u': rt = RUNE_TYPE_UNHOLY; break;
      case 'F': include_death = false;
      case 'f': rt = RUNE_TYPE_FROST; break;
      case 'D': rt = RUNE_TYPE_DEATH; break;
      case 'd': rt = RUNE_TYPE_DEATH; break;
    }

    if ( rt != RUNE_TYPE_NONE && util::str_compare_ci( splits[ 1 ], "cooldown_remains" ) )
    {
      struct rune_cooldown_expr_t : public expr_t
      {
        death_knight_t* dk;
        rune_type r;
        bool death;

        rune_cooldown_expr_t( death_knight_t* p, rune_type r, bool include_death ) :
          expr_t( "rune_cooldown_remains" ),
          dk( p ), r( r ), death( include_death ) {}
        virtual double evaluate()
        {
          return dk -> runes_cooldown_any( r, death, 0 );
        }
      };

      return new rune_cooldown_expr_t( this, rt, include_death );
    }
  }
  else if ( name_str == "inactive_death" )
  {
    struct death_expr_t : public expr_t
    {
      death_knight_t* dk;
      death_expr_t( death_knight_t* p ) :
        expr_t( "inactive_death" ), dk( p ) { }
      virtual double evaluate()
      {
        return count_death_runes( dk, true );
      }
    };
    return new death_expr_t( this );
  }
  else
  {
    rune_type rt = RUNE_TYPE_NONE;
    bool include_death = true;
    switch ( splits[ 0 ][ 0 ] )
    {
      case 'B': include_death = false;
      case 'b': rt = RUNE_TYPE_BLOOD; break;
      case 'U': include_death = false;
      case 'u': rt = RUNE_TYPE_UNHOLY; break;
      case 'F': include_death = false;
      case 'f': rt = RUNE_TYPE_FROST; break;
      case 'D': rt = RUNE_TYPE_DEATH; break;
      case 'd': rt = RUNE_TYPE_DEATH; break;
    }

    struct rune_expr_t : public expr_t
    {
      death_knight_t* dk;
      rune_type r;
      bool death;
      rune_expr_t( death_knight_t* p, rune_type r, bool include_death ) :
        expr_t( "rune" ), dk( p ), r( r ), death( include_death ) { }
      virtual double evaluate()
      { return dk -> runes_count( r, death, 0 ); }
    };
    if ( rt ) return new rune_expr_t( this, rt, include_death );

  }

  return player_t::create_expression( a, name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    pets.gargoyle             = create_pet( "gargoyle" );
    pets.ghoul_pet            = create_pet( "ghoul_pet" );
  }
  else
    pets.ghoul_guardian       = create_pet( "ghoul_guardian" );

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    pets.dancing_rune_weapon  = new pets::dancing_rune_weapon_pet_t ( sim, this );
    for ( int i = 0; i < 10; i++ )
      pets.bloodworms[ i ] = new pets::bloodworms_pet_t( sim, this );
  }

  for ( int i = 0; i < 8; i++ )
    pets.army_ghoul[ i ] = new pets::army_ghoul_pet_t( sim, this );

  for ( int i = 0; i < 10; i++ )
    pets.fallen_zandalari[ i ] = new pets::fallen_zandalari_t( this );
}

// death_knight_t::create_pet ===============================================

pet_t* death_knight_t::create_pet( const std::string& pet_name,
                                   const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "gargoyle"       ) return new pets::gargoyle_pet_t( sim, this );
  if ( pet_name == "ghoul_pet"      ) return new pets::ghoul_pet_t   ( sim, this, "ghoul", false );
  if ( pet_name == "ghoul_guardian" ) return new pets::ghoul_pet_t   ( sim, this, "ghoul", true );

  return 0;
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_melee_haste()
{
  double haste = player_t::composite_melee_haste();

  haste *= 1.0 / ( 1.0 + buffs.unholy_presence -> value() );

  return haste;
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  rng.blood_caked_blade = get_rng( "blood_caked_blade" );
  rng.blood_parasite   = get_rng( "blood_parasite"   );
  rng.blood_tap         = get_rng( "blood_tap"         );
  rng.plague_leech      = get_rng( "plague_leech"      );
  rng.rime              = get_rng( "rime"              );
  rng.sudden_doom       = get_rng( "sudden_doom"       );
  rng.t13_2pc_melee     = get_rng( "t13_2pc_melee"     );

  rng.t15_2pc_melee     = new real_ppm_t( "t15_2pc_melee", *this, 1.0 );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attribute_multiplier[ ATTR_STRENGTH ] *= 1.0 + spec.unholy_might -> effectN( 1 ).percent();
  base.attribute_multiplier[ ATTR_STAMINA ]  *= 1.0 + spec.veteran_of_the_third_war -> effectN( 1 ).percent();

  base.stats.attack_power = level * ( level > 80 ? 3.0 : 2.0 );

  // Level 90 values, horribly off for anything else
  base.dodge   = 0.0300 + spec.veteran_of_the_third_war -> effectN( 2 ).percent();
  base.miss    = 0.0600;
  base.parry   = 0.0300;
  base.block   = 0.0000;
  base.parry_rating_per_strength = dbc.combat_rating( RATING_PARRY, level ) / 952.0 / 100.0;
  base.dodge_per_agility = 1 / 10000.0 / 100.0;

  base.attack_power_per_strength = 2.0;

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;

  base_gcd = timespan_t::from_seconds( 1.0 );

  // Horribly off, what are the new values?
  diminished_kfactor   = 0.956;
  diminished_dodge_cap = 0.906425;
  diminished_parry_cap = 2.37186;
}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Specialization

  // Generic
  spec.plate_specialization = find_specialization_spell( "Plate Specialization" );

  // Blood
  spec.blood_parasite             = find_specialization_spell( "Blood Parasite" );
  spec.blood_rites                = find_specialization_spell( "Blood Rites" );
  spec.veteran_of_the_third_war   = find_specialization_spell( "Veteran of the Third War" );
  spec.scent_of_blood             = find_specialization_spell( "Scent of Blood" );
  spec.improved_blood_presence    = find_specialization_spell( "Improved Blood Presence" );
  spec.blood_parasite             = find_specialization_spell( "Blood Parasite" );
  spec.scarlet_fever              = find_specialization_spell( "Scarlet Fever" );
  spec.crimson_scourge            = find_specialization_spell( "Crimson Scourge" );
  spec.sanguine_fortitude         = find_specialization_spell( "Sanguine Fortitude" );
  spec.will_of_the_necropolis     = find_specialization_spell( "Will of the Necropolis" );

  // Frost
  spec.blood_of_the_north         = find_specialization_spell( "Blood of the North" );
  spec.icy_talons                 = find_specialization_spell( "Icy Talons" );
  spec.improved_frost_presence    = find_specialization_spell( "Improved Frost Presence" );
  spec.brittle_bones              = find_specialization_spell( "Brittle Bones" );
  spec.rime                       = find_specialization_spell( "Rime" );
  spec.might_of_the_frozen_wastes = find_specialization_spell( "Might of the Frozen Wastes" );
  spec.threat_of_thassarian       = find_specialization_spell( "Threat of Thassarian" );
  spec.killing_machine            = find_specialization_spell( "Killing Machine" );

  // Unholy
  spec.master_of_ghouls           = find_specialization_spell( "Master of Ghouls" );
  spec.reaping                    = find_specialization_spell( "Reaping" );
  spec.unholy_might               = find_specialization_spell( "Unholy Might" );
  spec.shadow_infusion            = find_specialization_spell( "Shadow Infusion" );
  spec.sudden_doom                = find_specialization_spell( "Sudden Doom" );
  spec.ebon_plaguebringer         = find_specialization_spell( "Ebon Plaguebringer" );
  spec.improved_unholy_presence   = find_specialization_spell( "Improved Unholy Presence" );

  mastery.blood_shield            = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart            = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade              = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Talents
  talent.roiling_blood            = find_talent_spell( "Roiling Blood" );
  talent.plague_leech             = find_talent_spell( "Plague Leech" );
  talent.unholy_blight            = find_talent_spell( "Unholy Blight" );

  talent.death_siphon             = find_talent_spell( "Death Siphon" );

  talent.blood_tap                = find_talent_spell( "Blood Tap" );
  talent.runic_empowerment        = find_talent_spell( "Runic Empowerment" );
  talent.runic_corruption         = find_talent_spell( "Runic Corruption" );

  // Glyphs
  glyph.chains_of_ice             = find_glyph_spell( "Glyph of Chains of Ice" );
  glyph.dancing_rune_weapon       = find_glyph_spell( "Glyph of Dancing Rune Weapon" );
  glyph.enduring_infection        = find_glyph_spell( "Glyph of Enduring Infection" );
  glyph.icebound_fortitude        = find_glyph_spell( "Glyph of Icebound Fortitude" );
  glyph.outbreak                  = find_glyph_spell( "Glyph of Outbreak" );
  glyph.shifting_presences        = find_glyph_spell( "Glyph of Shifting Presences" );
  glyph.vampiric_blood            = find_glyph_spell( "Glyph of Vampiric Blood" );

  // Generic spells
  spell.blood_parasite           = find_spell( 50452 );
  spell.t15_4pc_tank             = find_spell( 138214 );

  // Active Spells
  active_spells.blood_plague = new blood_plague_t( this );
  active_spells.frost_fever = new frost_fever_t( this );


  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P     M2P     M4P     T2P     T4P    H2P    H4P
    {     0,     0, 105609, 105646, 105552, 105587,     0,     0 }, // Tier13
    {     0,     0, 123077, 123078, 123079, 123080,     0,     0 }, // Tier14
    {     0,     0, 138343, 138347, 138195, 138197,     0,     0 }, // Tier15
    {     0,     0, 144899, 144907, 144934, 144950,     0,     0 }, // Tier16
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// death_knight_t::init_actions =============================================

void death_knight_t::init_actions()
{
  if ( false )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's class isn't supported yet.", name() );
    quiet = true;
    return;
  }

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    int tree = specialization();

    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;
    std::string& aoe_list_str = get_action_priority_list( "aoe" ) -> action_list_str;
    std::string& st_list_str = get_action_priority_list( "single_target" ) -> action_list_str;
    std::string soul_reaper_pct = set_bonus.tier15_4pc_melee() ? "45" : "35";

    if ( tree == DEATH_KNIGHT_FROST || tree == DEATH_KNIGHT_UNHOLY || ( tree == DEATH_KNIGHT_BLOOD && primary_role() != ROLE_TANK ) )
    {
      if ( level >= 80 )
      {
        if ( sim -> allow_flasks )
        {
          // Flask
          if ( level > 85 )
            precombat_list += "/flask,type=winters_bite";
          else
            precombat_list += "/flask,type=titanic_strength";
        }

        if ( sim -> allow_food )
        {
          // Food
          if ( level > 85 )
          {
            precombat_list += "/food,type=black_pepper_ribs_and_shrimp";
          }
          else
          {
            precombat_list += "/food,type=beer_basted_crocolisk";
          }
        }
      }
    }
    else if ( tree == DEATH_KNIGHT_BLOOD && primary_role() == ROLE_TANK )
    {
      if ( level >= 80 )
      {
        if ( sim -> allow_flasks )
        {
          // Flask
          if ( level >  85 )
            precombat_list += "/flask,type=earth";
          else
            precombat_list += "/flask,type=steelskin";
        }

        if ( sim -> allow_food )
        {
          // Food
          if ( level > 85 )
            precombat_list += "/food,type=great_pandaren_banquet";
          else
            precombat_list += "/food,type=beer_basted_crocolisk";
        }
      }
    }

    if ( specialization() == DEATH_KNIGHT_BLOOD )
      precombat_list += "/blood_presence";
    else if ( specialization() == DEATH_KNIGHT_FROST )
      precombat_list += "/frost_presence";
    else
      precombat_list += "/unholy_presence";

    precombat_list += "/horn_of_winter";

    precombat_list += "/snapshot_stats";

    precombat_list += "/army_of_the_dead";

    if ( sim -> allow_potions && ( specialization() == DEATH_KNIGHT_FROST || specialization() == DEATH_KNIGHT_UNHOLY || primary_role() == ROLE_ATTACK ) )
    {
      if ( level > 85 )       precombat_list += "/mogu_power_potion";
      else if ( level >= 80 ) precombat_list += "/golemblood_potion";
    }

    precombat_list += init_use_racial_actions();

    action_list_str += "/auto_attack";

    switch ( specialization() )
    {
      case DEATH_KNIGHT_BLOOD:
      {
        if ( sim -> allow_potions )
        {
          if ( level > 85 )
            precombat_list += "/mogu_power_potion";
          else if ( level >= 80 )
            precombat_list += "/golemblood_potion";
        }

        if ( level >= 87 )
          precombat_list += "/bone_shield";

        action_list_str += init_use_item_actions( ",if=time>=10" );
        action_list_str += init_use_profession_actions();
        action_list_str += init_use_racial_actions( ",if=time>=10" );
        if ( sim -> allow_potions )
        {
          if ( level > 85 )
            action_list_str += "/mogu_power_potion,if=buff.bloodlust.react|target.time_to_die<=60";
          else if ( level >= 80 )
            action_list_str += "/golemblood_potion,if=buff.bloodlust.react|target.time_to_die<=60";
        }
        action_list_str += "/auto_attack";
        if ( level >= 76 )
          action_list_str += "/vampiric_blood,if=health<30000";
        if ( level >= 87 )
          action_list_str += "/bone_shield,if=buff.bone_shield.down";
        if ( level >= 62 )
          action_list_str += "/icebound_fortitude,if=health.pct<50";
        if ( level >= 64 )
          action_list_str += "/rune_tap,if=health.pct<90";
        action_list_str += "/dancing_rune_weapon";
        action_list_str += "/raise_dead,if=time>=10";
        action_list_str += "/outbreak,if=(dot.frost_fever.remains<=2|dot.blood_plague.remains<=2)|(!dot.blood_plague.ticking&!dot.frost_fever.ticking)";
        action_list_str += "/plague_strike,if=!dot.blood_plague.ticking";
        action_list_str += "/icy_touch,if=!dot.frost_fever.ticking";
        action_list_str += "/soul_reaper,if=target.health.pct-3*(target.health.pct%target.time_to_die)<=35";
        if ( talent.blood_tap -> ok() )
          action_list_str += "/blood_tap,if=(unholy=0&frost>=1)|(unholy>=1&frost=0)|(death=1)";
        action_list_str += "/death_strike";
        //action_list_str += "/death_siphon,if=health.pct<90";
        action_list_str += "/death_siphon,if=enabled&hit_damage>action.necrotic_strike.hit_damage";
        action_list_str += "/necrotic_strike,if=!talent.death_siphon.enabled|hit_damage>action.death_siphon.hit_damage";
        action_list_str += "/blood_boil,if=buff.crimson_scourge.up";
        action_list_str += "/heart_strike,if=(blood=1&blood.cooldown_remains<1)|blood=2";
        action_list_str += "/rune_strike,if=runic_power>=40";
        action_list_str += "/horn_of_winter";
        action_list_str += "/empower_rune_weapon,if=blood=0&unholy=0&frost=0";
        break;
      }
      case DEATH_KNIGHT_FROST:
      {
        // Frost specific precombat stuff
        if ( level >= 68 ) precombat_list += "/pillar_of_frost";
        if ( level >= 56 ) precombat_list += "/raise_dead";

        if ( level >= 68 ) action_list_str += "/pillar_of_frost";

        if ( sim -> allow_potions )
        {
          if ( level > 85 ) action_list_str += "/mogu_power_potion,if=target.time_to_die<=30|(target.time_to_die<=60&buff.pillar_of_frost.up)";
          else if ( level >= 80 ) action_list_str += "/golemblood_potion,if=target.time_to_die<=30|(target.time_to_die<=60&buff.pillar_of_frost.up)";
        }

        if ( level > 75 && main_hand_weapon.group() != WEAPON_2H )
        {
          action_list_str += "/empower_rune_weapon,if=target.time_to_die<=60";

          if ( sim -> allow_potions )
            action_list_str += "&(buff.mogu_power_potion.up|buff.golemblood_potion.up)";
        }

        action_list_str += init_use_item_actions( ",if=buff.pillar_of_frost.up" );
        action_list_str += init_use_profession_actions();
        action_list_str += init_use_racial_actions();

        if ( level >= 56 ) action_list_str += "/raise_dead";

        //decide between single_target and aoe rotation
        action_list_str += "/run_action_list,name=aoe,if=active_enemies>=5";
        action_list_str += "/run_action_list,name=single_target,if=active_enemies<5";

        if ( main_hand_weapon.group() == WEAPON_2H )
        {
          // Diseases for free
          st_list_str += "/plague_leech,if=talent.plague_leech.enabled&(dot.blood_plague.remains<1|dot.frost_fever.remains<1)";
          if ( level >= 82 ) st_list_str += "/outbreak,if=!dot.frost_fever.ticking|!dot.blood_plague.ticking";
          st_list_str += "/unholy_blight,if=talent.unholy_blight.enabled&(!dot.frost_fever.ticking|!dot.blood_plague.ticking)";

          // Soul Reaper
          if ( level >= 87 )
          {
            st_list_str += "/soul_reaper,if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct;
            st_list_str += "/blood_tap,if=talent.blood_tap.enabled&(target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct + "&cooldown.soul_reaper.remains=0)";
          }

          // Diseases for runes
          st_list_str += "/howling_blast,if=!dot.frost_fever.ticking";
          st_list_str += "/plague_strike,if=!dot.blood_plague.ticking";

          // Rime
          st_list_str += "/howling_blast,if=buff.rime.react";

          // Killing Machine
          if ( level >= 61 ) st_list_str += "/obliterate,if=buff.killing_machine.react";
          st_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.killing_machine.react";

          // Don't waste Runic Power
          st_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.blood_charge.stack>10&runic_power>76";
          st_list_str += "/frost_strike,if=runic_power>76";

          // Keep runes on cooldown
          if ( level >= 61 ) st_list_str += "/obliterate,if=blood=2|frost=2|unholy=2";

          // Refresh diseases
          st_list_str += "/plague_leech,if=talent.plague_leech.enabled&(dot.blood_plague.remains<3|dot.frost_fever.remains<3)";
          if ( level >= 82 ) st_list_str += "/outbreak,if=dot.frost_fever.remains<3|dot.blood_plague.remains<3";
          st_list_str += "/unholy_blight,if=talent.unholy_blight.enabled&(dot.frost_fever.remains<3|dot.blood_plague.remains<3)";

          // Regenerate resources
          st_list_str += "/frost_strike,if=talent.runic_empowerment.enabled&frost=0";
          st_list_str += "/frost_strike,if=talent.blood_tap.enabled&buff.blood_charge.stack<=10";
          st_list_str += "/horn_of_winter";
          st_list_str += "/frost_strike,if=talent.runic_corruption.enabled&buff.runic_corruption.down";

          // Normal stuff
          if ( level >= 61 ) st_list_str += "/obliterate";
          if ( level >= 75 )
          {
            st_list_str += "/empower_rune_weapon,if=target.time_to_die<=60";
            if ( sim -> allow_potions )
              st_list_str += "&(buff.mogu_power_potion.up|buff.golemblood_potion.up)";
          }
          st_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.blood_charge.stack>10&runic_power>=20";
          st_list_str += "/frost_strike";

          // Better than waiting
          st_list_str += "/plague_leech,if=talent.plague_leech.enabled";
          if ( level >= 75 ) st_list_str += "/empower_rune_weapon";
        }
        else
        {
          st_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.blood_charge.stack>10&(runic_power>76|(runic_power>=20&buff.killing_machine.react))";

          // Killing Machine / Very High RP
          st_list_str += "/frost_strike,if=buff.killing_machine.react|runic_power>88";

          // Diseases for free
          st_list_str += "/plague_leech,if=talent.plague_leech.enabled&(dot.blood_plague.remains<3|dot.frost_fever.remains<3|cooldown.outbreak.remains<1)";
          if ( level >= 82 ) st_list_str += "/outbreak,if=dot.frost_fever.remains<3|dot.blood_plague.remains<3";
          st_list_str += "/unholy_blight,if=talent.unholy_blight.enabled&(dot.frost_fever.remains<3|dot.blood_plague.remains<3)";

          // Soul Reaper
          if ( level >= 87 )
          {
            st_list_str += "/soul_reaper,if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct;
            st_list_str += "/blood_tap,if=talent.blood_tap.enabled&(target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct + "&cooldown.soul_reaper.remains=0)";
          }

          // Diseases for runes
          st_list_str += "/howling_blast,if=!dot.frost_fever.ticking";
          st_list_str += "/plague_strike,if=!dot.blood_plague.ticking";

          // Rime
          st_list_str += "/howling_blast,if=buff.rime.react";

          // Don't waste Runic Power
          st_list_str += "/frost_strike,if=runic_power>76";

          // Keep Runes on Cooldown
          if ( level >= 61 ) st_list_str += "/obliterate,if=unholy>1";
          st_list_str += "/howling_blast,if=death>1|frost>1";

          // Generate Runic Power or Runes
          st_list_str += "/horn_of_winter";
          if ( level >= 61 ) st_list_str += "/obliterate,if=unholy>0";
          st_list_str += "/howling_blast";
          st_list_str += "/frost_strike,if=talent.runic_empowerment.enabled&(frost=0|blood=0)";
          st_list_str += "/blood_tap,if=talent.blood_tap.enabled&(target.health.pct-3*(target.health.pct%target.time_to_die)>" + soul_reaper_pct + "|buff.blood_charge.stack>=8)";
          st_list_str += "/frost_strike,if=talent.runic_corruption.enabled&buff.runic_corruption.down";
          if ( level >= 60 ) st_list_str += "/death_and_decay";

          // Better than waiting
          st_list_str += "/frost_strike,if=runic_power>=40";
          if ( level >= 75 ) st_list_str += "/empower_rune_weapon";
        }

        //AoE
        aoe_list_str = "/unholy_blight,if=talent.unholy_blight.enabled";
        aoe_list_str += "/pestilence,if=dot.blood_plague.ticking&talent.plague_leech.enabled,line_cd=28";
        aoe_list_str += "/pestilence,if=dot.blood_plague.ticking&talent.unholy_blight.enabled&cooldown.unholy_blight.remains<49,line_cd=28";
        aoe_list_str += "/howling_blast";
        aoe_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.blood_charge.stack>10";
        aoe_list_str += "/frost_strike,if=runic_power>76";
        aoe_list_str += "/death_and_decay,if=unholy=1";
        aoe_list_str += "/plague_strike,if=unholy=2";
        aoe_list_str += "/blood_tap,if=talent.blood_tap.enabled";
        aoe_list_str += "/frost_strike";
        aoe_list_str += "/horn_of_winter";
        aoe_list_str += "/plague_leech,if=talent.plague_leech.enabled&unholy=1";
        aoe_list_str += "/plague_strike,if=unholy=1";
        aoe_list_str += "/empower_rune_weapon";

        if ( race == RACE_GOBLIN ) action_list_str += "/rocket_barrage";
        break;
      }
      case DEATH_KNIGHT_UNHOLY:
      {
        precombat_list += "/raise_dead";

        action_list_str += init_use_profession_actions();
        action_list_str += init_use_racial_actions( ",if=time>=2" );

        if ( sim -> allow_potions )
        {
          if ( level > 85 )
            action_list_str += "/mogu_power_potion,if=buff.dark_transformation.up&target.time_to_die<=35";
          else if ( level >= 80 )
            action_list_str += "/golemblood_potion,if=buff.dark_transformation.up&target.time_to_die<=35";
        }

        if ( level >= 66 ) action_list_str += "/unholy_frenzy,if=time>=4";
        action_list_str += init_use_item_actions( ",if=time>=4" );

        //decide between single_target and aoe rotation
        action_list_str += "/run_action_list,name=aoe,if=active_enemies>=5";
        action_list_str += "/run_action_list,name=single_target,if=active_enemies<5";

        // Disease Gaming

        if ( level >= 82 ) st_list_str += "/outbreak,if=stat.attack_power>(dot.blood_plague.attack_power*1.1)&time>15&!(cooldown.unholy_blight.remains>79)";
        st_list_str += "/plague_strike,if=stat.attack_power>(dot.blood_plague.attack_power*1.1)&time>15&!(cooldown.unholy_blight.remains>79)";

        st_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.blood_charge.stack>10&runic_power>=32";

        // Diseases for free
        st_list_str += "/unholy_blight,if=talent.unholy_blight.enabled&(dot.frost_fever.remains<3|dot.blood_plague.remains<3)";
        if ( level >= 82 ) st_list_str += "/outbreak,if=dot.frost_fever.remains<3|dot.blood_plague.remains<3";

        // Soul Reaper
        if ( level >= 87 )
        {
          st_list_str += "/soul_reaper,if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct;
          st_list_str += "/blood_tap,if=talent.blood_tap.enabled&(target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct + "&cooldown.soul_reaper.remains=0)";
        }

        // Diseases for Runes
        st_list_str += "/plague_strike,if=!dot.blood_plague.ticking|!dot.frost_fever.ticking";

        // GCD Cooldowns
        if ( level >= 74 ) st_list_str += "/summon_gargoyle";
        if ( level >= 70 ) st_list_str += "/dark_transformation";
        st_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.shadow_infusion.stack=5";

        // Don't waste runic power
        st_list_str += "/death_coil,if=runic_power>90";

        // Get runes on cooldown
        if ( level >= 60 ) st_list_str += "/death_and_decay,if=unholy=2";
        st_list_str += "/blood_tap,if=talent.blood_tap.enabled&unholy=2&cooldown.death_and_decay.remains=0";
        if ( level >= 58 ) st_list_str += "/scourge_strike,if=unholy=2";
        if ( level >= 64 ) st_list_str += "/festering_strike,if=blood=2&frost=2";

        // Normal stuff
        if ( level >= 60 ) st_list_str += "/death_and_decay";
        st_list_str += "/blood_tap,if=talent.blood_tap.enabled&cooldown.death_and_decay.remains=0";
        st_list_str += "/death_coil,if=buff.sudden_doom.react|(buff.dark_transformation.down&rune.unholy<=1)";
        if ( level >= 58 ) st_list_str += "/scourge_strike";
        st_list_str += "/plague_leech,if=talent.plague_leech.enabled&cooldown.outbreak.remains<1";
        if ( level >= 64 ) st_list_str += "/festering_strike";
        st_list_str += "/horn_of_winter";
        st_list_str += "/death_coil,if=buff.dark_transformation.down|(cooldown.summon_gargoyle.remains>8&buff.dark_transformation.remains>8)";

        // Less waiting
        st_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.blood_charge.stack>=8";
        if ( level >= 75 ) st_list_str += "/empower_rune_weapon";

        //AoE
        aoe_list_str = "/unholy_blight,if=talent.unholy_blight.enabled";
        aoe_list_str += "/plague_strike,if=!dot.blood_plague.ticking|!dot.frost_fever.ticking";
        aoe_list_str += "/pestilence,if=dot.blood_plague.ticking&talent.plague_leech.enabled,line_cd=28";
        aoe_list_str += "/pestilence,if=dot.blood_plague.ticking&talent.unholy_blight.enabled&cooldown.unholy_blight.remains<49,line_cd=28";
        if ( level >= 74 ) aoe_list_str += "/summon_gargoyle";
        if ( level >= 70 ) aoe_list_str += "/dark_transformation";
        aoe_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.shadow_infusion.stack=5";
        aoe_list_str += "/blood_boil,if=blood=2|death=2";
        aoe_list_str += "/death_and_decay,if=unholy=1";
        aoe_list_str += "/soul_reaper,if=unholy=2&target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct;
        aoe_list_str += "/scourge_strike,if=unholy=2";
        aoe_list_str += "/blood_tap,if=talent.blood_tap.enabled&buff.blood_charge.stack>10";
        aoe_list_str += "/death_coil,if=runic_power>90|buff.sudden_doom.react|(buff.dark_transformation.down&rune.unholy<=1)";
        aoe_list_str += "/blood_boil";
        aoe_list_str += "/icy_touch";
        aoe_list_str += "/soul_reaper,if=unholy=1&target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct;
        aoe_list_str += "/scourge_strike,if=unholy=1";
        aoe_list_str += "/death_coil";
        aoe_list_str += "/blood_tap,if=talent.blood_tap.enabled";
        aoe_list_str += "/plague_leech,if=talent.plague_leech.enabled&unholy=1";
        aoe_list_str += "/horn_of_winter";
        aoe_list_str += "/empower_rune_weapon";

        break;
      }
      default: break;
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// death_knight_t::init_enchant =============================================

void death_knight_t::init_enchant()
{
  player_t::init_enchant();

  std::string& mh_enchant = items[ SLOT_MAIN_HAND ].parsed.enchant.name_str;
  std::string& oh_enchant = items[ SLOT_OFF_HAND  ].parsed.enchant.name_str;

  // Rune of Cinderglacier ==================================================
  struct cinderglacier_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    cinderglacier_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w || w -> slot != slot ) return;

      // FIX ME: What is the proc rate? For now assuming the same as FC
      buff -> trigger( 2, buff_t::DEFAULT_VALUE(), w -> proc_chance_on_swing( 2.0 ) );

      // FIX ME: This should roll the benefit when casting DND, it does not
    }
  };

  // Rune of the Fallen Crusader ============================================
  struct fallen_crusader_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    fallen_crusader_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // RotFC is 2 PPM.
      buff -> trigger( 1, 0.15, w -> proc_chance_on_swing( 2.0 ) );
    }
  };

  // Rune of the Razorice ===================================================

  // Damage Proc
  struct razorice_attack_t : public death_knight_melee_attack_t
  {
    razorice_attack_t( death_knight_t* player ) :
      death_knight_melee_attack_t( "razorice", player, player -> find_spell( 50401 ) )
    {
      school      = SCHOOL_FROST;
      may_miss    = false;
      may_crit    = false;
      background  = true;
      proc        = true;
      callbacks   = false;
    }

    virtual double composite_target_multiplier( player_t* t )
    {
      double m = death_knight_melee_attack_t::composite_target_multiplier( t );

      m /= 1.0 + cast_td( t ) -> debuffs_frost_vulnerability -> check() *
                 cast_td( t ) -> debuffs_frost_vulnerability -> data().effectN( 1 ).percent();

      return m;
    }
  };

  struct razorice_callback_t : public action_callback_t
  {
    int slot;
    razorice_attack_t* razorice_damage_proc;

    razorice_callback_t( death_knight_t* p, int s ) :
      action_callback_t( p ), slot( s ), razorice_damage_proc( 0 )
    {
      razorice_damage_proc = new razorice_attack_t( p );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      // FIXME: Currently, in game seems to have a bug, where main-hand
      // Razorice runeforge will trigger from both main- and off hand hits (or
      // some similar bug), essentially doubling the proc rate of the
      // runeforge.
      if ( listener -> bugs )
      {
        if ( slot == SLOT_OFF_HAND && w -> slot != slot )
          return;
      }
      else
      {
        if ( w -> slot != slot )
          return;
      }

      // http://elitistjerks.com/f72/t64830-dw_builds_3_2_revenge_offhand/p28/#post1332820
      // double PPM        = 2.0;
      // double swing_time = a -> time_to_execute;
      // double chance     = w -> proc_chance_on_swing( PPM, swing_time );

      debug_cast< death_knight_t* >( a -> player ) -> get_target_data( a -> execute_state -> target ) -> debuffs_frost_vulnerability -> trigger();
      razorice_damage_proc -> weapon = w;
      razorice_damage_proc -> execute();
    }
  };

  runeforge.rune_of_cinderglacier       = buff_creator_t( this, "rune_of_cinderglacier", find_spell( 53386 ) )
                                          .default_value( find_spell( 53386 ) -> effectN( 1 ).percent() );
  runeforge.rune_of_the_fallen_crusader = buff_creator_t( this, "rune_of_the_fallen_crusader" ).max_stack( 1 )
                                          .duration( timespan_t::from_seconds( 15.0 ) )
                                          .add_invalidate( CACHE_STRENGTH );

  runeforge.rune_of_the_stoneskin_gargoyle = buff_creator_t( this, "rune_of_the_stoneskin_gargoyle", find_spell( 62157 ) )
                                             .quiet( true )
                                             .chance( 0 );
  runeforge.rune_of_the_nerubian_carapace = buff_creator_t( this, "rune_of_the_nerubian_carapace", find_spell( 70163 ) )
                                           .quiet( true )
                                           .chance( 0 );
  runeforge.rune_of_the_nerubian_carapace_oh = buff_creator_t( this, "rune_of_the_nerubian_carapace_oh", find_spell( 70163 ) )
                                               .quiet( true )
                                               .chance( 0 );

  runeforge.rune_of_spellshattering = buff_creator_t( this, "rune_of_spellshattering", find_spell( 53362 ) )
                                      .quiet( true )
                                      .chance( 0 );
  runeforge.rune_of_spellbreaking = buff_creator_t( this, "rune_of_spellbreaking", find_spell( 54449 ) )
                                    .quiet( true )
                                    .chance( 0 );
  runeforge.rune_of_spellbreaking_oh = buff_creator_t( this, "rune_of_spellbreaking_oh", find_spell( 54449 ) )
                                       .quiet( true )
                                       .chance( 0 );

  runeforge.rune_of_swordshattering = buff_creator_t( this, "rune_of_swordshattering", find_spell( 53387 ) )
                                      .quiet( true )
                                      .chance( 0 );
  runeforge.rune_of_swordbreaking = buff_creator_t( this, "rune_of_swordbreaking", find_spell( 54448 ) )
                                    .quiet( true )
                                    .chance( 0 );
  runeforge.rune_of_swordbreaking_oh = buff_creator_t( this, "rune_of_swordbreaking_oh", find_spell( 54448 ) )
                                       .quiet( true )
                                       .chance( 0 );

  if ( mh_enchant == "rune_of_the_fallen_crusader" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_MAIN_HAND, runeforge.rune_of_the_fallen_crusader ) );
  }
  else if ( mh_enchant == "rune_of_razorice" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_MAIN_HAND ) );
  }
  else if ( mh_enchant == "rune_of_cinderglacier" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_MAIN_HAND, runeforge.rune_of_cinderglacier ) );
  }

  if ( oh_enchant == "rune_of_the_fallen_crusader" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_OFF_HAND, runeforge.rune_of_the_fallen_crusader ) );
  }
  else if ( oh_enchant == "rune_of_razorice" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_OFF_HAND ) );
  }
  else if ( oh_enchant == "rune_of_cinderglacier" )
  {
    callbacks.register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_OFF_HAND, runeforge.rune_of_cinderglacier ) );
  }

  if ( mh_enchant == "rune_of_the_stoneskin_gargoyle" )
    runeforge.rune_of_the_stoneskin_gargoyle -> default_chance = 1.0;

  if ( mh_enchant == "rune_of_the_nerubian_carapace" )
    runeforge.rune_of_the_nerubian_carapace -> default_chance = 1.0;

  if ( oh_enchant == "rune_of_the_nerubian_carapace" )
    runeforge.rune_of_the_nerubian_carapace_oh -> default_chance = 1.0;

  if ( mh_enchant == "rune_of_spellshattering" )
    runeforge.rune_of_spellshattering -> default_chance = 1.0;

  if ( mh_enchant == "rune_of_swordshattering" )
    runeforge.rune_of_swordshattering -> default_chance = 1.0;

  if ( mh_enchant == "rune_of_spellbreaking" )
    runeforge.rune_of_spellbreaking -> default_chance = 1.0;

  if ( oh_enchant == "rune_of_spellbreaking" )
    runeforge.rune_of_spellbreaking_oh -> default_chance = 1.0;

  if ( mh_enchant == "rune_of_swordbreaking" )
    runeforge.rune_of_swordbreaking -> default_chance = 1.0;

  if ( oh_enchant == "rune_of_swordbreaking" )
    runeforge.rune_of_swordbreaking_oh -> default_chance = 1.0;
}

// death_knight_t::init_scaling =============================================

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( off_hand_weapon.type != WEAPON_NONE )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2          ] = true;
  }

  if ( primary_role() == ROLE_TANK )
  {
    scales_with[ STAT_PARRY_RATING ] = true;
    scales_with[ STAT_DODGE_RATING ] = true;
    scales_with[ STAT_AGILITY      ] = false; // Yes yes on paper this isnt really true.
    scales_with[ STAT_BLOCK_RATING ] = false;
  }
}

// death_knight_t::init_buffs ===============================================

static bool death_shroud_mastery( void* data )
{
  player_t* player = static_cast< player_t* >( data );
  if ( player -> current.stats.get_stat( STAT_MASTERY_RATING ) >=
       player -> current.stats.get_stat( STAT_HASTE_RATING ) )
    return true;
  return false;
}

static bool death_shroud_haste( void* data )
{
  player_t* player = static_cast< player_t* >( data );
  if ( player -> current.stats.get_stat( STAT_HASTE_RATING ) >
       player -> current.stats.get_stat( STAT_MASTERY_RATING ) )
    return true;
  return false;
}

void death_knight_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.blood_shield        = absorb_buff_creator_t( this, "blood_shield", find_spell( 77535 ) )
                              .school( SCHOOL_PHYSICAL );

  buffs.antimagic_shell     = buff_creator_t( this, "antimagic_shell", find_class_spell( "Anti-Magic Shell" ) )
                              .cd( timespan_t::zero() );
  buffs.blood_charge        = buff_creator_t( this, "blood_charge", find_spell( 114851 ) )
                              .chance( find_talent_spell( "Blood Tap" ) -> ok() );
  buffs.blood_presence      = buff_creator_t( this, "blood_presence", find_class_spell( "Blood Presence" ) )
                              .add_invalidate( CACHE_STAMINA );
  buffs.bone_shield         = buff_creator_t( this, "bone_shield", find_specialization_spell( "Bone Shield" ) )
                              .cd( timespan_t::zero() )
                              .max_stack( specialization() == DEATH_KNIGHT_BLOOD ? ( find_specialization_spell( "Bone Shield" ) -> initial_stacks() +
                                          find_spell( 144948 ) -> max_stacks() ) : -1 );
  buffs.bone_wall           = buff_creator_t( this, "bone_wall", find_spell( 144948 ) )
                              .chance( maybe_ptr( dbc.ptr ) && set_bonus.tier16_2pc_tank() );
  buffs.crimson_scourge     = buff_creator_t( this, "crimson_scourge" ).spell( find_spell( 81141 ) )
                              .chance( spec.crimson_scourge -> proc_chance() );
  buffs.dancing_rune_weapon = buff_creator_t( this, "dancing_rune_weapon", find_spell( 81256 ) )
                              .add_invalidate( CACHE_PARRY );
  buffs.dark_transformation = buff_creator_t( this, "dark_transformation", find_class_spell( "Dark Transformation" ) );
  buffs.deathbringer        = buff_creator_t( this, "deathbringer", find_spell( 144953 ) )
                              .chance( maybe_ptr( dbc.ptr ) && set_bonus.tier16_4pc_tank() );
  buffs.death_shroud        = stat_buff_creator_t( this, "death_shroud", sets -> set( SET_T16_2PC_MELEE ) -> effectN( 1 ).trigger() )
                              .chance( sets -> set( SET_T16_2PC_MELEE ) -> proc_chance() )
                              .add_stat( STAT_MASTERY_RATING, sets -> set( SET_T16_2PC_MELEE ) -> effectN( 1 ).trigger() -> effectN( 2 ).base_value(), death_shroud_mastery )
                              .add_stat( STAT_HASTE_RATING, sets -> set( SET_T16_2PC_MELEE ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(), death_shroud_haste );
  buffs.frost_presence      = buff_creator_t( this, "frost_presence", find_class_spell( "Frost Presence" ) )
                              .default_value( find_class_spell( "Frost Presence" ) -> effectN( 1 ).percent() );
  buffs.icebound_fortitude  = buff_creator_t( this, "icebound_fortitude", find_class_spell( "Icebound Fortitude" ) )
                              .duration( find_class_spell( "Icebound Fortitude" ) -> duration() *
                                         ( 1.0 + glyph.icebound_fortitude -> effectN( 2 ).percent() ) );
  buffs.killing_machine     = buff_creator_t( this, "killing_machine", find_spell( 51124 ) )
                              .default_value( find_spell( 51124 ) -> effectN( 1 ).percent() )
                              .chance( find_specialization_spell( "Killing Machine" ) -> proc_chance() ); // PPM based!
  buffs.pillar_of_frost     = buff_creator_t( this, "pillar_of_frost", find_class_spell( "Pillar of Frost" ) )
                              .default_value( find_class_spell( "Pillar of Frost" ) -> effectN( 1 ).percent() +
                                              sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent() )
                              .add_invalidate( CACHE_STRENGTH );
  buffs.rime                = buff_creator_t( this, "rime", find_spell( 59052 ) )
                              .max_stack( ( set_bonus.tier13_2pc_melee() ) ? 2 : 1 )
                              .cd( timespan_t::zero() )
                              .chance( spec.rime -> ok() );
  //buffs.runic_corruption    = buff_creator_t( this, "runic_corruption", find_spell( 51460 ) )
  //                            .chance( talent.runic_corruption -> proc_chance() );
  buffs.runic_corruption    = new runic_corruption_buff_t( this );
  buffs.scent_of_blood      = buff_creator_t( this, "scent_of_blood", spec.scent_of_blood -> effectN( 1 ).trigger() )
                              .chance( spec.scent_of_blood -> proc_chance() );
  buffs.sudden_doom         = buff_creator_t( this, "sudden_doom", find_spell( 81340 ) )
                              .max_stack( ( set_bonus.tier13_2pc_melee() ) ? 2 : 1 )
                              .cd( timespan_t::zero() )
                              .chance( spec.sudden_doom -> ok() );
  buffs.shadow_infusion     = buff_creator_t( this, "shadow_infusion", spec.shadow_infusion -> effectN( 1 ).trigger() );
  buffs.tier13_4pc_melee    = stat_buff_creator_t( this, "tier13_4pc_melee" )
                              .spell( find_spell( 105647 ) );

  buffs.unholy_presence     = buff_creator_t( this, "unholy_presence", find_class_spell( "Unholy Presence" ) )
                              .default_value( find_class_spell( "Unholy Presence" ) -> effectN( 1 ).percent() +
                                              spec.improved_unholy_presence -> effectN( 1 ).percent() )
                              .add_invalidate( CACHE_HASTE );
  buffs.vampiric_blood      = new vampiric_blood_buff_t( this );
  buffs.will_of_the_necropolis_dr = buff_creator_t( this, "will_of_the_necropolis_dr", find_spell( 81162 ) )
                                    .cd( timespan_t::from_seconds( 45 ) );
  buffs.will_of_the_necropolis_rt = buff_creator_t( this, "will_of_the_necropolish_bt", find_spell( 96171 ) )
                                    .cd( timespan_t::from_seconds( 45 ) );
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains.antimagic_shell                  = get_gain( "antimagic_shell"            );
  gains.butchery                         = get_gain( "butchery"                   );
  gains.chill_of_the_grave               = get_gain( "chill_of_the_grave"         );
  gains.frost_presence                   = get_gain( "frost_presence"             );
  gains.horn_of_winter                   = get_gain( "horn_of_winter"             );
  gains.improved_frost_presence          = get_gain( "improved_frost_presence"    );
  gains.power_refund                     = get_gain( "power_refund"               );
  gains.scent_of_blood                   = get_gain( "scent_of_blood"             );
  gains.rune                             = get_gain( "rune_regen_all"             );
  gains.rune_unholy                      = get_gain( "rune_regen_unholy"          );
  gains.rune_blood                       = get_gain( "rune_regen_blood"           );
  gains.rune_frost                       = get_gain( "rune_regen_frost"           );
  gains.rune_unknown                     = get_gain( "rune_regen_unknown"         );
  gains.runic_empowerment                = get_gain( "runic_empowerment"          );
  gains.runic_empowerment_blood          = get_gain( "runic_empowerment_blood"    );
  gains.runic_empowerment_frost          = get_gain( "runic_empowerment_frost"    );
  gains.runic_empowerment_unholy         = get_gain( "runic_empowerment_unholy"   );
  gains.empower_rune_weapon              = get_gain( "empower_rune_weapon"        );
  gains.blood_tap                        = get_gain( "blood_tap"                  );
  gains.blood_tap_blood                  = get_gain( "blood_tap_blood"            );
  gains.blood_tap_frost                  = get_gain( "blood_tap_frost"            );
  gains.blood_tap_unholy                 = get_gain( "blood_tap_unholy"           );
  gains.plague_leech                     = get_gain( "plague_leech"               );
  gains.rc                               = get_gain( "runic_corruption_all"       );
  gains.rc_unholy                        = get_gain( "runic_corruption_unholy"    );
  gains.rc_blood                         = get_gain( "runic_corruption_blood"     );
  gains.rc_frost                         = get_gain( "runic_corruption_frost"     );
  // gains.blood_tap_blood                  = get_gain( "blood_tap_blood"            );
  //gains.blood_tap_blood          -> type = ( resource_e ) RESOURCE_RUNE_BLOOD   ;
  gains.hp_death_siphon                  = get_gain( "hp_death_siphon"            );
  gains.t15_4pc_tank                     = get_gain( "t15_4pc_tank"               );
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

  procs.blood_parasite           = get_proc( "blood_parasite"              );
  procs.runic_empowerment        = get_proc( "runic_empowerment"            );
  procs.runic_empowerment_wasted = get_proc( "runic_empowerment_wasted"     );
  procs.oblit_killing_machine    = get_proc( "oblit_killing_machine"        );
  procs.sr_killing_machine       = get_proc( "sr_killing_machine"           );
  procs.fs_killing_machine       = get_proc( "frost_strike_killing_machine" );
  procs.t15_2pc_melee            = get_proc( "t15_2pc_melee"                );

  procs.ready_blood              = get_proc( "Blood runes ready" );
  procs.ready_frost              = get_proc( "Frost runes ready" );
  procs.ready_unholy             = get_proc( "Unholy runes ready" );
}

// death_knight_t::init_resources ===========================================

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RUNIC_POWER ] = 0;
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  // Active
  active_presence = 0;

  t16_tank_2pc_driver = 0;

  rng.t15_2pc_melee -> reset();

  _runes.reset();

  incoming_damage.clear();
}

// death_knight_t::combat_begin =============================================

void death_knight_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    vengeance_start();
}

// death_knight_t::assess_damage ============================================

void death_knight_t::assess_damage( school_e     school,
                                    dmg_e        dtype,
                                    action_state_t* s )
{
  double health_pct = health_percentage();

  player_t::assess_damage( school, dtype, s );

  // Bone shield will only decrement, if someone did damage to the dk
  if ( s -> result_amount > 0 )
  {
    if ( cooldown.bone_shield_icd -> up() )
    {
      buffs.bone_shield -> decrement();
      cooldown.bone_shield_icd -> start();

      if ( set_bonus.tier15_4pc_tank() && buffs.bone_shield -> check() )
      {
        resource_gain( RESOURCE_RUNIC_POWER,
                       spell.t15_4pc_tank -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                       gains.t15_4pc_tank );
      }
    }

    incoming_damage.push_back( std::pair<timespan_t, double>( sim -> current_time, s -> result_amount ) );

    while ( incoming_damage.size() > 0 &&
            sim -> current_time - incoming_damage.front().first > timespan_t::from_seconds( 5.0 ) )
      incoming_damage.erase( incoming_damage.begin() );
  }

  if ( health_pct >= 35 && health_percentage() < 35 )
  {
    buffs.will_of_the_necropolis_dr -> trigger();
    buffs.will_of_the_necropolis_rt -> trigger();
  }
}

// death_knight_t::target_mitigation ========================================

void death_knight_t::target_mitigation( school_e school, dmg_e type, action_state_t* state )
{
  if ( buffs.blood_presence -> check() )
    state -> result_amount *= 1.0 + buffs.blood_presence -> data().effectN( 7 ).percent();

  if ( school != SCHOOL_PHYSICAL && runeforge.rune_of_spellshattering -> check() )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellshattering -> data().effectN( 1 ).percent();

  if ( school != SCHOOL_PHYSICAL && runeforge.rune_of_spellbreaking -> check() )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellbreaking -> data().effectN( 1 ).percent();

  if ( school != SCHOOL_PHYSICAL && runeforge.rune_of_spellbreaking_oh -> check() )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellbreaking_oh -> data().effectN( 1 ).percent();

  if ( buffs.bone_shield -> up() )
    state -> result_amount *= 1.0 + buffs.bone_shield -> data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude -> up() )
    state -> result_amount *= 1.0 + buffs.icebound_fortitude -> data().effectN( 3 ).percent() + spec.sanguine_fortitude -> effectN( 1 ).percent();

  if ( buffs.will_of_the_necropolis_dr -> up() )
    state -> result_amount *= 1.0 + spec.will_of_the_necropolis -> effectN( 1 ).percent();

  player_t::target_mitigation( school, type, state );
}

// death_knight_t::composite_armor_multiplier ===============================

double death_knight_t::composite_armor_multiplier()
{
  double a = player_t::composite_armor_multiplier();

  if ( buffs.blood_presence -> check() )
    a *= 1.0 + buffs.blood_presence -> data().effectN( 3 ).percent();

  if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
    a *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 1 ).percent();

  if ( runeforge.rune_of_the_nerubian_carapace -> check() )
    a *= 1.0 + runeforge.rune_of_the_nerubian_carapace -> data().effectN( 1 ).percent();

  if ( runeforge.rune_of_the_nerubian_carapace_oh -> check() )
    a *= 1.0 + runeforge.rune_of_the_nerubian_carapace_oh -> data().effectN( 1 ).percent();

  return a;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( attribute_e attr )
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + runeforge.rune_of_the_fallen_crusader -> value();
    m *= 1.0 + buffs.pillar_of_frost -> value();
  }
  else if ( attr == ATTR_STAMINA )
  {
    if ( buffs.blood_presence -> check() )
      m *= 1.0 + buffs.blood_presence -> data().effectN( 6 ).percent();

    if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
      m *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 2 ).percent();

    if ( runeforge.rune_of_the_nerubian_carapace -> check() )
      m *= 1.0 + runeforge.rune_of_the_nerubian_carapace -> data().effectN( 2 ).percent();

    if ( runeforge.rune_of_the_nerubian_carapace_oh -> check() )
      m *= 1.0 + runeforge.rune_of_the_nerubian_carapace_oh -> data().effectN( 2 ).percent();
  }

  return m;
}

// death_knight_t::matching_gear_multiplier =================================

double death_knight_t::matching_gear_multiplier( attribute_e attr )
{
  int tree = specialization();

  if ( tree == DEATH_KNIGHT_UNHOLY || tree == DEATH_KNIGHT_FROST )
    if ( attr == ATTR_STRENGTH )
      return spec.plate_specialization -> effectN( 1 ).percent();

  if ( tree == DEATH_KNIGHT_BLOOD )
    if ( attr == ATTR_STAMINA )
      return spec.plate_specialization -> effectN( 1 ).percent();

  return 0.0;
}

// death_knight_t::composite_tank_parry =====================================

double death_knight_t::composite_parry()
{
  double parry = player_t::composite_parry();

  if ( buffs.dancing_rune_weapon -> up() )
    parry += buffs.dancing_rune_weapon -> data().effectN( 1 ).percent();

  if ( runeforge.rune_of_swordshattering -> check() )
    parry += runeforge.rune_of_swordshattering -> data().effectN( 1 ).percent();

  if ( runeforge.rune_of_swordbreaking -> check() )
    parry += runeforge.rune_of_swordbreaking -> data().effectN( 1 ).percent();

  if ( runeforge.rune_of_swordbreaking_oh -> check() )
    parry += runeforge.rune_of_swordbreaking_oh -> data().effectN( 1 ).percent();

  return parry;
}

// death_knight_t::composite_player_multiplier ==============================

double death_knight_t::composite_player_multiplier( school_e school )
{
  double m = player_t::composite_player_multiplier( school );

  if ( mastery.dreadblade -> ok() && dbc::is_school( school, SCHOOL_SHADOW )  )
    m *= 1.0 + cache.mastery_value();

  if ( mastery.frozen_heart -> ok() && dbc::is_school( school, SCHOOL_FROST )  )
    m *= 1.0 + cache.mastery_value();

  return m;
}

// death_knight_t::composite_attack_speed() =================================

double death_knight_t::composite_melee_speed()
{
  double haste = player_t::composite_melee_speed();

  haste *= 1.0 / ( 1.0 + spec.icy_talons -> effectN( 1 ).percent() );

  return haste;
}

// death_knight_t::composite_tank_crit ======================================

double death_knight_t::composite_crit_avoidance()
{
  double c = player_t::composite_crit_avoidance();

  c += spec.improved_blood_presence -> effectN( 3 ).percent();

  return c;
}

void death_knight_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;
    default: break;
  }
}

// death_knight_t::primary_role =============================================

role_e death_knight_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// death_knight_t::regen ====================================================

void death_knight_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buffs.antimagic_shell -> check() )
    resource_gain( RESOURCE_RUNIC_POWER, buffs.antimagic_shell -> value() * periodicity.total_seconds(), gains.antimagic_shell );

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    _runes.slot[i].regen_rune( this, periodicity );
}

// death_knight_t::create_options ===========================================

void death_knight_t::create_options()
{
  player_t::create_options();

  option_t death_knight_options[] =
  {
    opt_string( "unholy_frenzy_target", unholy_frenzy_target_str ),
    opt_null()
  };

  option_t::copy( options, death_knight_options );
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

  if ( strstr( s, "necrotic_boneplate" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "breastplate"   ) ||
                      strstr( s, "greaves"       ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T13_MELEE;
    if ( is_tank  ) return SET_T13_TANK;
  }

  if ( strstr( s, "lost_catacomb" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "breastplate"   ) ||
                      strstr( s, "greaves"       ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T14_MELEE;
    if ( is_tank  ) return SET_T14_TANK;
  }

  if ( strstr( s, "_of_the_allconsuming_maw" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "breastplate"   ) ||
                      strstr( s, "greaves"       ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T15_MELEE;
    if ( is_tank  ) return SET_T15_TANK;
  }

  if ( strstr( s, "_gladiators_dreadplate_" ) ) return SET_PVP_MELEE;

  return SET_NONE;
}

// death_knight_t::trigger_runic_empowerment ================================

void death_knight_t::trigger_runic_empowerment()
{
  if ( ! sim -> roll( talent.runic_empowerment -> proc_chance() ) )
    return;

  int depleted_runes[RUNE_SLOT_MAX];
  int num_depleted = 0;

  // Find fully depleted runes, i.e., both runes are on CD
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( _runes.slot[i].is_depleted() )
      depleted_runes[ num_depleted++ ] = i;

  if ( num_depleted > 0 )
  {
    int rune_to_regen = depleted_runes[ ( int ) ( sim -> default_rng() -> real() * num_depleted * 0.9999 ) ];
    dk_rune_t* regen_rune = &_runes.slot[rune_to_regen];
    regen_rune -> fill_rune();
    if      ( regen_rune -> is_blood()  ) gains.runic_empowerment_blood  -> add ( RESOURCE_RUNE_BLOOD, 1, 0 );
    else if ( regen_rune -> is_unholy() ) gains.runic_empowerment_unholy -> add ( RESOURCE_RUNE_UNHOLY, 1, 0 );
    else if ( regen_rune -> is_frost()  ) gains.runic_empowerment_frost  -> add ( RESOURCE_RUNE_FROST, 1, 0 );

    gains.runic_empowerment -> add ( RESOURCE_RUNE, 1, 0 );
    if ( sim -> log ) sim -> output( "runic empowerment regen'd rune %d", rune_to_regen );
    procs.runic_empowerment -> occur();

    buffs.tier13_4pc_melee -> trigger( 1, buff_t::DEFAULT_VALUE(), sets -> set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent() );
  }
  else
  {
    // If there were no available runes to refresh
    procs.runic_empowerment_wasted -> occur();
    gains.runic_empowerment -> add ( RESOURCE_RUNE, 0, 1 );
  }
}

// death_knight_t rune inspections ==========================================

// death_knight_t::runes_count ==============================================
// how many runes of type rt are available
int death_knight_t::runes_count( rune_type rt, bool include_death, int position )
{
  int result = 0;
  // positional checks first
  if ( position > 0 && ( rt == RUNE_TYPE_BLOOD || rt == RUNE_TYPE_FROST || rt == RUNE_TYPE_UNHOLY ) )
  {
    dk_rune_t* r = &_runes.slot[( ( rt - 1 ) * 2 ) + ( position - 1 ) ];
    if ( r -> is_ready() )
      result = 1;
  }
  else
  {
    int rpc = 0;
    for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
    {
      dk_rune_t* r = &_runes.slot[ i ];
      // query a specific position death rune.
      if ( position != 0 && rt == RUNE_TYPE_DEATH && r -> is_death() )
      {
        if ( ++rpc == position )
        {
          if ( r -> is_ready() )
            result = 1;
          break;
        }
      }
      // just count the runes
      else if ( ( ( ( include_death || rt == RUNE_TYPE_DEATH ) && r -> is_death() ) || r -> get_type() == rt )
                && r -> is_ready() )
      {
        result++;
      }
    }
  }
  return result;
}

// death_knight_t::runes_cooldown_any =======================================

double death_knight_t::runes_cooldown_any( rune_type rt, bool include_death, int position )
{
  dk_rune_t* rune = 0;
  int rpc = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    dk_rune_t* r = &_runes.slot[i];
    if ( position == 0 && include_death && r -> is_death() && r -> is_ready() )
      return 0;
    if ( position == 0 && r -> get_type() == rt && r -> is_ready() )
      return 0;
    if ( ( ( include_death && r -> is_death() ) || ( r -> get_type() == rt ) ) )
    {
      if ( position != 0 && ++rpc == position )
      {
        rune = r;
        break;
      }
      if ( !rune || ( r -> value > rune -> value ) )
        rune = r;
    }
  }

  assert( rune );

  double time = this -> runes_cooldown_time( rune );
  // if it was a  specified rune and is depleted, we have to add its paired rune to the time
  if ( rune -> is_depleted() )
    time += this -> runes_cooldown_time( rune -> paired_rune );

  return time;
}

// death_knight_t::runes_cooldown_all =======================================

double death_knight_t::runes_cooldown_all( rune_type rt, bool include_death, int position )
{
  // if they specified position then they only get 1 answer. duh. handled in the other function
  if ( position > 0 )
    return this -> runes_cooldown_any( rt, include_death, position );

  // find all matching runes. total the pairs. Return the highest number.

  double max = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; i += 2 )
  {
    double total = 0;
    dk_rune_t* r = &_runes.slot[i];
    if ( ( ( rt == RUNE_TYPE_DEATH && r -> is_death() ) || r -> get_type() == rt ) && !r -> is_ready() )
    {
      total += this->runes_cooldown_time( r );
    }
    r = r -> paired_rune;
    if ( ( ( rt == RUNE_TYPE_DEATH && r -> is_death() ) || r -> get_type() == rt ) && !r -> is_ready() )
    {
      total += this->runes_cooldown_time( r );
    }
    if ( max < total )
      max = total;
  }
  return max;
}

// death_knight_t::runes_cooldown_time ======================================

double death_knight_t::runes_cooldown_time( dk_rune_t* rune )
{
  double result_num;

  double runes_per_second = 1.0 / 10.0 / composite_melee_haste();
  // Unholy Presence's 10% (or, talented, 15%) increase is factored in elsewhere as melee haste.
  result_num = ( 1.0 - rune -> value ) / runes_per_second;

  return result_num;
}

// death_knight_t::runes_depleted ===========================================

bool death_knight_t::runes_depleted( rune_type rt, int position )
{
  dk_rune_t* rune = 0;
  int rpc = 0;
  // iterate, to allow finding death rune slots as well
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    dk_rune_t* r = &_runes.slot[i];
    if ( r -> get_type() == rt && ++rpc == position )
    {
      rune = r;
      break;
    }
  }
  if ( ! rune ) return false;
  return rune -> is_depleted();
}

void death_knight_t::arise()
{
  player_t::arise();

  if ( spec.blood_of_the_north -> ok() )
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      if ( _runes.slot[i].type == RUNE_TYPE_BLOOD )
        _runes.slot[i].make_permanent_death_rune();
    }
  }

  if ( specialization() == DEATH_KNIGHT_FROST  && ! sim -> overrides.attack_speed ) sim -> auras.attack_speed -> trigger();
  if ( specialization() == DEATH_KNIGHT_UNHOLY && ! sim -> overrides.attack_speed ) sim -> auras.attack_speed -> trigger();

  runeforge.rune_of_the_stoneskin_gargoyle -> trigger();
  runeforge.rune_of_the_nerubian_carapace -> trigger();
  runeforge.rune_of_the_nerubian_carapace_oh -> trigger();
  runeforge.rune_of_swordshattering -> trigger();
  runeforge.rune_of_swordbreaking -> trigger();
  runeforge.rune_of_swordbreaking_oh -> trigger();
  runeforge.rune_of_spellshattering -> trigger();
  runeforge.rune_of_spellbreaking -> trigger();
  runeforge.rune_of_spellbreaking_oh -> trigger();
}

// DEATH_KNIGHT MODULE INTERFACE ============================================

struct death_knight_module_t : public module_t
{
  death_knight_module_t() : module_t( DEATH_KNIGHT ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new death_knight_t( sim, name, r );
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.unholy_frenzy = haste_buff_creator_t( p, "unholy_frenzy", p -> find_spell( 49016 ) ).add_invalidate( CACHE_HASTE );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::death_knight()
{
  static death_knight_module_t m;
  return &m;
}
