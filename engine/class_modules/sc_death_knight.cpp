// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================


// TODO:
// DRW blood boil
// DRW necrotic plague
// DRW defile?
// DRW breath of sindragosa?
// Blood T17 4pc bonus
// Blood tanking side in general
// Frost T17 4pc bonus
// Unholy T17 4pc bonus

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

using namespace unique_gear;

struct death_knight_t;

namespace pets {
struct dancing_rune_weapon_pet_t;
}

namespace runeforge
{
  void razorice_attack( special_effect_t&, const item_t& );
  void razorice_debuff( special_effect_t&, const item_t& );
  void fallen_crusader( special_effect_t&, const item_t& );
  void stoneskin_gargoyle( special_effect_t&, const item_t& );
  void spellshattering( special_effect_t&, const item_t& );
  void spellbreaking( special_effect_t&, const item_t& );
}

// ==========================================================================
// Death Knight Runes
// ==========================================================================

enum rune_type
{
  RUNE_TYPE_NONE = 0, RUNE_TYPE_BLOOD, RUNE_TYPE_FROST, RUNE_TYPE_UNHOLY, RUNE_TYPE_DEATH, RUNE_TYPE_WASDEATH = 8
};

enum disease_type { DISEASE_NONE = 0, DISEASE_BLOOD_PLAGUE, DISEASE_FROST_FEVER };

const char * const rune_symbols = "!bfu!!";

const int RUNE_TYPE_MASK = 3;
const int RUNE_SLOT_MAX = 6;
const double RUNIC_POWER_REFUND = 0.9;
static const double RUNIC_POWER_DIVISOR = 30.0;

enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

struct rune_t
{
  death_knight_t* dk;
  int        type;
  rune_state state;
  double     value;   // 0.0 to 1.0, with 1.0 being full
  bool       permanent_death_rune;
  rune_t* paired_rune;
  int        slot_number;

  rune_t() : dk( nullptr ), type( RUNE_TYPE_NONE ), state( STATE_FULL ), value( 0.0 ), permanent_death_rune( false ), paired_rune( NULL ), slot_number( 0 ) {}

  bool is_death() const        { return ( type & RUNE_TYPE_DEATH ) != 0                ; }
  bool is_blood() const        { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_BLOOD  ; }
  bool is_unholy() const       { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_UNHOLY ; }
  bool is_frost() const        { return ( type & RUNE_TYPE_MASK  ) == RUNE_TYPE_FROST  ; }
  bool is_ready() const        { return state == STATE_FULL                            ; }
  bool is_depleted() const     { return state == STATE_DEPLETED                        ; }
  int  get_type() const        { return type & RUNE_TYPE_MASK                          ; }

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

struct runes_t
{
  std::array<rune_t, RUNE_SLOT_MAX> slot;
  runes_t( death_knight_t* p ) :
    slot()
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
    for ( size_t i = 0; i < slot.size(); ++i )
    {
      slot[ i ].slot_number = static_cast<int>(i);
      slot[ i ].dk = p;
    }
  }
  void reset()
  {
    for ( size_t i = 0; i < slot.size(); ++i )
    {
      slot[ i ].reset();
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
  dot_t* dots_necrotic_plague;
  dot_t* dots_defile;

  debuff_t* debuffs_razorice;
  debuff_t* debuffs_mark_of_sindragosa;
  debuff_t* debuffs_necrotic_plague;

  int diseases() const
  {
    int disease_count = 0;
    if ( dots_blood_plague -> is_ticking() ) disease_count++;
    if ( dots_frost_fever  -> is_ticking() ) disease_count++;
    if ( dots_necrotic_plague -> is_ticking() ) disease_count++;
    return disease_count;
  }

  death_knight_td_t( player_t* target, death_knight_t* death_knight );
};

enum death_knight_presence { PRESENCE_BLOOD = 1, PRESENCE_FROST, PRESENCE_UNHOLY = 4 };

struct death_knight_t : public player_t
{
public:
  // Active
  int       active_presence;
  int       t16_tank_2pc_driver;
  double    runic_power_decay_rate;
  double    blood_charge_counter;
  double    shadow_infusion_counter;
  double    fallen_crusader, fallen_crusader_rppm;
  // Buffs
  struct buffs_t
  {
    buff_t* army_of_the_dead;
    buff_t* antimagic_shell;
    buff_t* blood_charge;
    buff_t* blood_presence;
    buff_t* bone_wall;
    buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* dark_transformation;
    buff_t* deaths_advance;
    buff_t* deathbringer;
    buff_t* frost_presence;
    buff_t* icebound_fortitude;
    buff_t* killing_machine;
    buff_t* pillar_of_frost;
    buff_t* rime;
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
    buff_t* rune_tap;
    stat_buff_t* death_shroud;
    stat_buff_t* riposte;
    buff_t* shadow_of_death;
    buff_t* conversion;
    buff_t* frozen_runeblade;
  } buffs;

  struct runeforge_t
  {
    buff_t* rune_of_the_fallen_crusader;
    buff_t* rune_of_the_stoneskin_gargoyle;
    buff_t* rune_of_spellshattering;
    buff_t* rune_of_spellbreaking;
    buff_t* rune_of_spellbreaking_oh;
  } runeforge;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* bone_shield_icd;
    cooldown_t* vampiric_blood;
  } cooldown;

  // Diseases

  struct active_spells_t
  {
    action_t* blood_caked_blade;
    spell_t* blood_plague;
    spell_t* frost_fever;
    melee_attack_t* frozen_power;
    spell_t* frozen_runeblade;
    spell_t* necrotic_plague;
    spell_t* breath_of_sindragosa;
    spell_t* necrosis;
    heal_t* conversion;
  } active_spells;

  // Gains
  struct gains_t
  {
    gain_t* antimagic_shell;
    gain_t* blood_rites;
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
    gain_t* necrotic_plague;
    gain_t* blood_tap;
    gain_t* blood_tap_blood;
    gain_t* blood_tap_frost;
    gain_t* blood_tap_unholy;
    gain_t* plague_leech;
    gain_t* hp_death_siphon;
    gain_t* t15_4pc_tank;
    gain_t* t17_2pc_frost;
    gain_t* t17_4pc_unholy_blood;
    gain_t* t17_4pc_unholy_unholy;
    gain_t* t17_4pc_unholy_frost;
    gain_t* t17_4pc_unholy_waste;
  } gains;

  // Specialization
  struct specialization_t
  {
    // Generic
    const spell_data_t* plate_specialization;
    const spell_data_t* multistrike_attunement;

    // Blood
    const spell_data_t* bladed_armor;
    const spell_data_t* blood_rites;
    const spell_data_t* veteran_of_the_third_war;
    const spell_data_t* scent_of_blood;
    const spell_data_t* improved_blood_presence;
    const spell_data_t* scarlet_fever;
    const spell_data_t* crimson_scourge;
    const spell_data_t* sanguine_fortitude;
    const spell_data_t* will_of_the_necropolis;
    const spell_data_t* resolve;
    const spell_data_t* riposte;

    // Frost
    const spell_data_t* blood_of_the_north;
    const spell_data_t* icy_talons;
    const spell_data_t* improved_frost_presence;
    const spell_data_t* killing_machine;
    const spell_data_t* might_of_the_frozen_wastes;
    const spell_data_t* rime;
    const spell_data_t* threat_of_thassarian;

    // Unholy
    const spell_data_t* ebon_plaguebringer;
    const spell_data_t* improved_unholy_presence;
    const spell_data_t* master_of_ghouls;
    const spell_data_t* necrosis;
    const spell_data_t* reaping;
    const spell_data_t* shadow_infusion;
    const spell_data_t* sudden_doom;
    const spell_data_t* unholy_might;
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
    const spell_data_t* plaguebearer;
    const spell_data_t* plague_leech;
    const spell_data_t* unholy_blight;

    const spell_data_t* deaths_advance;

    const spell_data_t* conversion;

    const spell_data_t* blood_tap;
    const spell_data_t* runic_empowerment;
    const spell_data_t* runic_corruption;

    const spell_data_t* death_siphon;

    const spell_data_t* necrotic_plague;
    const spell_data_t* breath_of_sindragosa;
    const spell_data_t* defile;
  } talent;

  // Spells
  struct spells_t
  {
    const spell_data_t* antimagic_shell;
    const spell_data_t* t15_4pc_tank;
    const spell_data_t* t16_4pc_melee;
    const spell_data_t* blood_rites;
    const spell_data_t* deaths_advance;
    const spell_data_t* necrotic_plague_energize;
  } spell;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* chains_of_ice;
    const spell_data_t* dancing_rune_weapon;
    const spell_data_t* enduring_infection;
    const spell_data_t* festering_blood;
    const spell_data_t* icebound_fortitude;
    const spell_data_t* outbreak;
    const spell_data_t* regenerative_magic;
    const spell_data_t* shifting_presences;
    const spell_data_t* vampiric_blood;
  } glyph;

  // Perks
  struct perks_t
  {
    // Shared
    const spell_data_t* improved_diseases;
    const spell_data_t* enhanced_death_strike;

    // Blood
    const spell_data_t* enhanced_bone_shield;
    const spell_data_t* enhanced_death_coil_blood;
    const spell_data_t* enhanced_rune_tap;
    const spell_data_t* enhanced_will_of_the_necropolis;
    const spell_data_t* improved_death_strike;
    const spell_data_t* improved_pestilence;

    // Frost
    const spell_data_t* empowered_icebound_fortitude;
    const spell_data_t* empowered_pillar_of_frost;
    const spell_data_t* empowered_obliterate;
    const spell_data_t* improved_runeforges;

    // Unholy
    const spell_data_t* empowered_gargoyle;
    const spell_data_t* enhanced_dark_transformation;
    const spell_data_t* enhanced_fallen_crusader;
    const spell_data_t* improved_death_coil;
    const spell_data_t* improved_festering_strike;
    const spell_data_t* improved_scourge_strike;
    const spell_data_t* improved_soul_reaper;
  } perk;

  // Pets and Guardians
  struct pets_t
  {
    std::array< pet_t*, 8 > army_ghoul;
    std::array< pet_t*, 10 > fallen_zandalari;
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon;
    pet_t* ghoul_pet;
    pet_t* gargoyle;
  } pets;

  // Procs
  struct procs_t
  {
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

  real_ppm_t t15_2pc_melee;

  // Runes
  runes_t _runes;

  death_knight_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    active_presence(),
    t16_tank_2pc_driver(),
    runic_power_decay_rate(),
    fallen_crusader( find_spell( 53365 ) -> effectN( 1 ).percent() + find_spell( 157376 ) -> effectN( 2 ).percent() ),
    fallen_crusader_rppm( find_spell( 166441 ) -> real_ppm() ),
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
    t15_2pc_melee( *this ),
    _runes( this )
  {
    range::fill( pets.army_ghoul, nullptr );
    base.distance = 0;

    cooldown.bone_shield_icd = get_cooldown( "bone_shield_icd" );
    cooldown.bone_shield_icd -> duration = timespan_t::from_seconds( 1.0 );

    cooldown.vampiric_blood = get_cooldown( "vampiric_blood" );

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_action_list();
  virtual bool      init_special_effect( special_effect_t&, const item_t&, unsigned );
  virtual void      init_rng();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_resources( bool force );
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_melee_attack_power() const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_melee_speed() const;
  virtual double    composite_melee_haste() const;
  virtual double    composite_spell_haste() const;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const;
  virtual double    composite_rating_multiplier( rating_e ) const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_parry_rating() const;
  virtual double    composite_parry() const;
  virtual double    composite_dodge() const;
  virtual double    composite_multistrike() const;
  virtual double    composite_melee_expertise( weapon_t* ) const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const;
  virtual double    composite_player_absorb_multiplier( const action_state_t* s ) const;
  virtual double    composite_crit_avoidance() const;
  virtual double    passive_movement_modifier() const;
  virtual double    temporary_movement_modifier() const;
  virtual void      regen( timespan_t periodicity );
  virtual void      reset();
  virtual void      arise();
  virtual void      assess_heal( school_e, dmg_e, action_state_t* );
  virtual void      assess_damage( school_e, dmg_e, action_state_t* );
  virtual void      assess_damage_imminent( school_e, dmg_e, action_state_t* );
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual void      combat_begin();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      create_options();
  virtual resource_e primary_resource() const { return RESOURCE_RUNIC_POWER; }
  virtual role_e    primary_role() const;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual void      invalidate_cache( cache_e );

  void      trigger_runic_empowerment( double rpcost );
  void      trigger_runic_corruption( double rpcost );
  void      trigger_plaguebearer( action_state_t* state );
  void      trigger_blood_charge( double rpcost );
  void      trigger_shadow_infusion( double rpcost );
  void      trigger_necrosis( const action_state_t* );
  void      trigger_t17_4pc_frost( const action_state_t* );
  void      trigger_t17_4pc_unholy( const action_state_t* );
  void      apply_diseases( action_state_t* state, unsigned diseases );
  int       runes_count( rune_type rt, bool include_death, int position );
  double    runes_cooldown_any( rune_type rt, bool include_death, int position );
  double    runes_cooldown_all( rune_type rt, bool include_death, int position );
  double    runes_cooldown_time( rune_t* r );
  bool      runes_depleted( rune_type rt, int position );
  void      default_apl_blood();


  target_specific_t<death_knight_td_t*> target_data;

  virtual death_knight_td_t* get_target_data( player_t* target ) const
  {
    death_knight_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new death_knight_td_t( target, const_cast<death_knight_t*>(this) );
    }
    return td;
  }
};


inline death_knight_td_t::death_knight_td_t( player_t* target, death_knight_t* death_knight ) :
  actor_pair_t( target, death_knight )
{
  dots_blood_plague    = target -> get_dot( "blood_plague",    death_knight );
  dots_death_and_decay = target -> get_dot( "death_and_decay", death_knight );
  dots_frost_fever     = target -> get_dot( "frost_fever",     death_knight );
  dots_soul_reaper     = target -> get_dot( "soul_reaper_dot", death_knight );
  dots_necrotic_plague = target -> get_dot( "necrotic_plague", death_knight );
  dots_defile          = target -> get_dot( "defile",          death_knight );

  debuffs_razorice = buff_creator_t( *this, "razorice", death_knight -> find_spell( 51714 ) )
                     .period( timespan_t::zero() );
  debuffs_mark_of_sindragosa = buff_creator_t( *this, "mark_of_sindragosa", death_knight -> find_spell( 155166 ) );
  debuffs_necrotic_plague = buff_creator_t( *this, "necrotic_plague", death_knight -> find_spell( 155159 ) ).period( timespan_t::zero() );
}

inline void rune_t::fill_rune()
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

// Event to spread Necrotic Plague from a source actor to a target actor

template<typename TARGETDATA>
struct np_spread_event_t : public event_t
{
  dot_t* dot;
  np_spread_event_t( dot_t* dot_ ) :
    event_t( *dot_ -> source -> sim, "necrotic_plague_spread" ),
    dot( dot_ )
  {
    sim().add_event( this, timespan_t::zero() );
  }

  void execute()
  {
    if ( dot -> target -> is_sleeping() )
      return;

    // TODO-WOD: Randomize target selection.
    player_t* new_target = 0;
    for ( size_t i = 0, end = sim().target_non_sleeping_list.size(); i < end; i++ )
    {
      player_t* t = sim().target_non_sleeping_list[ i ];
      if ( t == dot -> state -> target )
        continue;

      TARGETDATA* tdata = debug_cast<TARGETDATA*>( dot -> source -> get_target_data( t ) );
      if ( ! tdata -> dots_necrotic_plague -> is_ticking() )
      {
        new_target = t;
        break;
      }
    }

    // Dot is cloned to the new target
    if ( new_target )
    {
      if ( sim().debug )
        sim().out_debug.printf( "Player %s spreading Necrotic Plague from %s to %s",
            dot -> current_action -> player -> name(),
            dot -> target -> name(),
            new_target -> name() );
      dot -> copy( new_target, DOT_COPY_CLONE );

      TARGETDATA* source_tdata = debug_cast<TARGETDATA*>( dot -> source -> get_target_data( dot -> state -> target ) );
      TARGETDATA* tdata = debug_cast<TARGETDATA*>( dot -> source -> get_target_data( new_target ) );
      assert( ! tdata -> debuffs_necrotic_plague -> check() );
      tdata -> debuffs_necrotic_plague -> trigger( source_tdata -> debuffs_necrotic_plague -> check() );
    }
  }
};

// ==========================================================================
// Local Utility Functions
// ==========================================================================

// Log rune status ==========================================================

static void log_rune_status( const death_knight_t* p, bool debug = false )
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

  if ( ! debug )
    p -> sim -> out_log.printf( "%s runes: %s %s", p -> name(), rune_str.c_str(), runeval_str.c_str() );
  else
    p -> sim -> out_debug .printf( "%s runes: %s %s", p -> name(), rune_str.c_str(), runeval_str.c_str() );
}

// Count Death Runes ========================================================

static int count_death_runes( const death_knight_t* p, bool inactive )
{
  // Getting death rune count is a bit more complicated as it depends
  // on talents which runetype can be converted to death runes
  int count = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    const rune_t& r = p -> _runes.slot[ i ];
    if ( ( inactive || r.is_ready() ) && r.is_death() )
      ++count;
  }
  return count;
}

// Consume Runes ============================================================

static void consume_runes( death_knight_t* player, const std::array<bool,RUNE_SLOT_MAX>& use, bool convert_runes = false )
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
        player -> sim -> out_log.printf( "%s consumes rune #%d, type %d", player -> name(), i, consumed_type );
    }
  }

  if ( player -> sim -> log )
  {
    log_rune_status( player );
  }
}

// Group Runes ==============================================================

static int use_rune( const death_knight_t* p, rune_type rt, const std::array<bool,RUNE_SLOT_MAX>& use )
{
  const rune_t* r = 0;
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

static std::pair<int, double> rune_ready_in( const death_knight_t* p, rune_type rt, const std::array<bool,RUNE_SLOT_MAX>& use )
{
  typedef std::pair<int, double> rri_t;

  int fastest_remaining = -1;
  double t = std::numeric_limits<double>::max();
  double rps = 1.0 / 10.0 / p -> cache.attack_haste();
  if ( p -> buffs.runic_corruption -> check() )
    rps *= 2.0;

  const rune_t* r = 0;
  if ( rt == RUNE_TYPE_BLOOD )
    r = &( p -> _runes.slot[ 0 ] );
  else if ( rt == RUNE_TYPE_FROST )
    r = &( p -> _runes.slot[ 2 ] );
  else if ( rt == RUNE_TYPE_UNHOLY )
    r = &( p -> _runes.slot[ 4 ] );

  // 1) Choose first non-death rune of rune_type
  if ( r && ! use[ r -> slot_number ] && ! r -> is_death() )
  {
    if ( r -> is_ready() )
      return rri_t( r -> slot_number, 0 );
    else if ( r -> state == STATE_REGENERATING )
    {
      double ttr = ( 1.0 - r -> value ) / rps;
      if ( ttr < t )
      {
        t = ttr;
        fastest_remaining = r -> slot_number;
      }
    }
  }

  // 2) Choose paired non-death rune of rune_type
  if ( r && ! use[ r -> paired_rune -> slot_number ] && ! r -> paired_rune -> is_death() )
  {
    if ( r -> paired_rune -> is_ready() )
      return rri_t( r -> paired_rune -> slot_number, 0 );
    else if ( r -> paired_rune -> state == STATE_REGENERATING )
    {
      double ttr = ( 1.0 - r -> paired_rune -> value ) / rps;
      if ( ttr < t )
      {
        t = ttr;
        fastest_remaining = r -> paired_rune -> slot_number;
      }
    }
  }

  // 3) Choose first death rune of rune_type
  if ( r && ! use[ r -> slot_number ] && r -> is_death() )
  {
    if ( r -> is_ready() )
      return rri_t( r -> slot_number, 0 );
    else if ( r -> state == STATE_REGENERATING )
    {
      double ttr = ( 1.0 - r -> value ) / rps;
      if ( ttr < t )
      {
        t = ttr;
        fastest_remaining = r -> slot_number;
      }
    }
  }

  // 4) Choose paired death rune of rune_type
  if ( r && ! use[ r -> paired_rune -> slot_number ] && r -> paired_rune -> is_death() )
  {
    if ( r -> paired_rune -> is_ready() )
      return rri_t( r -> paired_rune -> slot_number, 0 );
    else if ( r -> paired_rune -> state == STATE_REGENERATING )
    {
      double ttr = ( 1.0 - r -> paired_rune -> value ) / rps;
      if ( ttr < t )
      {
        t = ttr;
        fastest_remaining = r -> paired_rune -> slot_number;
      }
    }
  }

  // 6) Choose the first death rune of any type, in the order b > u > f
  size_t order[] = { 0, 1, 4, 5, 2, 3 };
  for ( size_t i = 0; i < sizeof_array( order ); i++ )
  {
    const rune_t* r = &( p -> _runes.slot[ i ] );
    if ( ! r -> is_death() )
      continue;

    if ( use[ r -> slot_number ] )
      continue;

    if ( r -> is_ready() )
      return rri_t( r -> slot_number, 0 );
    else if ( r -> state == STATE_REGENERATING )
    {
      double ttr = ( 1.0 - r -> value ) / rps;
      if ( ttr < t )
      {
        t = ttr;
        fastest_remaining = r -> slot_number;
      }
    }
  }

  // 7) We have no ready runes, return the slot that would be ready next
  return rri_t( fastest_remaining, t );
}

static double ready_in( const death_knight_t* player, int blood, int frost, int unholy )
{
  typedef std::pair<int, double> rri_t;
  assert( blood < 2 && frost < 2 && unholy < 2  );

  std::array<bool,RUNE_SLOT_MAX> use;
  range::fill( use, false );

  if ( player -> sim -> debug )
    log_rune_status( player, true );

  double min_ready_blood = 9999, min_ready_frost = 9999, min_ready_unholy = 9999;

  rri_t info_blood, info_frost, info_unholy;

  if ( blood )
  {
    info_blood = rune_ready_in( player, RUNE_TYPE_BLOOD, use );
    if ( info_blood.second < min_ready_blood )
      min_ready_blood = info_blood.second;

    if ( info_blood.first > -1 )
      use[ info_blood.first ] = true;
  }

  if ( frost )
  {
    info_frost = rune_ready_in( player, RUNE_TYPE_FROST, use );
    if ( info_frost.second < min_ready_frost )
      min_ready_frost = info_frost.second;

    if ( info_frost.first > -1 )
      use[ info_frost.first ] = true;
  }

  if ( unholy )
  {
    info_unholy = rune_ready_in( player, RUNE_TYPE_UNHOLY, use );
    if ( info_unholy.second < min_ready_unholy )
      min_ready_unholy = info_unholy.second;

    if ( info_unholy.first > -1 )
      use[ info_unholy.first ] = true;
  }

  double min_ready = std::max( blood * min_ready_blood, frost * min_ready_frost );
  min_ready = std::max( min_ready, unholy * min_ready_unholy );

  if ( player -> sim -> debug )
    player -> sim -> out_debug.printf( "%s ready_in blood=[%d, %.3f] frost=[%d, %.3f] unholy=[%d, %.3f] min_ready=%.3f",
        player -> name(), info_blood.first, min_ready_blood, info_frost.first, min_ready_frost, info_unholy.first, min_ready_unholy, min_ready );

  return min_ready;
}

static bool group_runes ( const death_knight_t* player, int blood, int frost, int unholy, int death, std::array<bool,RUNE_SLOT_MAX>& group )
{
  assert( blood < 2 && frost < 2 && unholy < 2 && death < 2 );

  std::array<bool,RUNE_SLOT_MAX> use;
  range::fill( use, false );

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
    if ( p -> sim -> debug ) log_rune_status( p, true );

    return depleted_runes[ ( int ) p -> rng().range( 0, num_depleted ) ];
  }

  return -1;
}

void rune_t::regen_rune( death_knight_t* p, timespan_t periodicity, bool rc )
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
  if ( p -> buffs.runic_corruption -> check() )
    runes_per_second *= 2.0;
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

  // Full runes don't regen, however only record the overflow once, instead of two times.
  if ( state == STATE_FULL && paired_rune -> state == STATE_FULL )
  {
    if ( slot_number % 2 == 0 )
    {
      // FIXME: Resource type?
      gains_rune_type -> add( RESOURCE_NONE, 0, regen_amount );
      gains_rune      -> add( RESOURCE_NONE, 0, regen_amount );
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
    p -> sim -> out_debug.printf( "rune %d has %.2f regen time (%.3f per second) with %.2f%% haste",
                        slot_number, 1 / runes_per_second, runes_per_second, 100 * ( 1 / p -> cache.attack_haste() - 1 ) );

  if ( state == STATE_FULL )
  {
    if ( p -> sim -> log )
      log_rune_status( p );

    if ( p -> sim -> debug )
      p -> sim -> out_debug.printf( "rune %d regens to full", slot_number );
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
  dot_t* dots_necrotic_plague;

  buff_t* debuffs_necrotic_plague;

  int diseases() const
  {
    int disease_count = 0;
    if ( dots_blood_plague -> is_ticking() ) disease_count++;
    if ( dots_frost_fever  -> is_ticking() ) disease_count++;
    if ( dots_necrotic_plague -> is_ticking() ) disease_count++;
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
    }

    dancing_rune_weapon_td_t* td( player_t* t = 0 ) const
    { return p() -> get_target_data( t ? t : target ); }

    dancing_rune_weapon_pet_t* p()
    { return static_cast<dancing_rune_weapon_pet_t*>( player ); }
    const dancing_rune_weapon_pet_t* p() const
    { return static_cast<dancing_rune_weapon_pet_t*>( player ); }

    death_knight_t* o()
    { return static_cast< death_knight_t* >( p() -> owner ); }
    const death_knight_t* o() const
    { return static_cast< death_knight_t* >( p() -> owner ); }
  };

  struct drw_melee_attack_t : public melee_attack_t
  {
    drw_melee_attack_t( const std::string& n, dancing_rune_weapon_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      background = true;
      special    = true;
      may_crit   = true;
      school = SCHOOL_PHYSICAL;
    }

    dancing_rune_weapon_td_t* td( player_t* t ) const
    { return p() -> get_target_data( t ); }

    dancing_rune_weapon_pet_t* p()
    { return static_cast<dancing_rune_weapon_pet_t*>( player ); }
    const dancing_rune_weapon_pet_t* p() const
    { return static_cast<dancing_rune_weapon_pet_t*>( player ); }

    death_knight_t* o()
    { return static_cast< death_knight_t* >( p() -> owner ); }
    const death_knight_t* o() const
    { return static_cast< death_knight_t* >( p() -> owner ); }
  };

  struct drw_blood_plague_t : public drw_spell_t
  {
    drw_blood_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_plague", p, p -> owner -> find_spell( 55078 ) )  // Also check spell id 55078
    {
      tick_may_crit    = true;
      dot_behavior     = DOT_REFRESH;
      may_miss         = false;
      may_crit         = false;
      hasted_ticks     = false;
    }

    virtual double composite_crit() const
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
    }

    virtual double composite_crit() const
    { return action_t::composite_crit() + player -> cache.attack_crit(); }
  };

  struct drw_necrotic_plague_t : public drw_spell_t
  {

    drw_necrotic_plague_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "necrotic_plague", p, p -> owner -> find_spell( 155159 ) )
    {
      hasted_ticks = may_miss = may_crit = false;
      background = tick_may_crit = true;
      dot_behavior = DOT_REFRESH;
    }

    double composite_target_multiplier( player_t* target ) const
    {
      double m = drw_spell_t::composite_target_multiplier( target );

      const dancing_rune_weapon_td_t* tdata = td( target );
      if ( tdata -> dots_necrotic_plague -> is_ticking() )
        m *= tdata -> debuffs_necrotic_plague -> stack();

      return m;
    }

    void last_tick( dot_t* dot )
    {
      drw_spell_t::last_tick( dot );
      td( dot -> target ) -> debuffs_necrotic_plague -> expire();
    }

    // Spread Necrotic Plagues use the source NP dot's duration on the target,
    // and never pandemic-extnd the dot on the target.
    timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t triggered_duration ) const
    { return triggered_duration; }

    // Necrotic Plague duration will not be refreshed if it is already ticking on
    // the target, only the stack count will go up. Only Festering Strike is
    // allowed to extend the duration of the Necrotic Plague dot.
    void trigger_dot( action_state_t* s )
    {
      dancing_rune_weapon_td_t* tdata = td( s -> target );
      if ( ! tdata -> dots_necrotic_plague -> is_ticking() )
        drw_spell_t::trigger_dot( s );

      tdata -> debuffs_necrotic_plague -> trigger();

      if ( sim -> debug )
        sim -> out_debug.printf( "%s %s stack increases to %d",
            player -> name(), name(), tdata -> debuffs_necrotic_plague -> check() );
    }

    void tick( dot_t* dot )
    {
      drw_spell_t::tick( dot );

      if ( result_is_hit( dot -> state -> result ) && dot -> ticks_left() > 0 )
      {
        dancing_rune_weapon_td_t* tdata = td( dot -> state -> target );
        tdata -> debuffs_necrotic_plague -> trigger();

        if ( sim -> debug )
          sim -> out_debug.printf( "%s %s stack %d",
              player -> name(), name(), tdata -> debuffs_necrotic_plague -> check() );
        new ( *sim ) np_spread_event_t<dancing_rune_weapon_td_t>( dot );
      }
    }

    double composite_crit() const
    { return action_t::composite_crit() + player -> cache.attack_crit(); }
  };

  struct drw_death_coil_t : public drw_spell_t
  {
    drw_death_coil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "death_coil", p, p -> owner -> find_class_spell( "Death Coil" ) )
    {
      attack_power_mod.direct = 0.85;
    }
  };

  struct drw_death_siphon_t : public drw_spell_t
  {
    drw_death_siphon_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "death_siphon", p, p -> owner -> find_spell( 108196 ) )
    {
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

  struct drw_icy_touch_t : public drw_spell_t
  {
    drw_icy_touch_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "icy_touch", p, p -> owner -> find_class_spell( "Icy Touch" ) )
    {
      attack_power_mod.direct = 0.319;
    }

    virtual void impact( action_state_t* s )
    {
      drw_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        if ( ! p() -> o() -> talent.necrotic_plague -> ok() )
        {
          p() -> drw_frost_fever -> target = s -> target;
          p() -> drw_frost_fever -> execute();
        }
        else
        {
          p() -> drw_necrotic_plague -> target = s -> target;
          p() -> drw_necrotic_plague -> execute();
        }
      }
    }
  };

  struct drw_blood_boil_t: public drw_spell_t
  {
    drw_blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_boil", p, p -> owner -> find_class_spell( "Blood Boil" ) )
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
        if ( td( target ) -> dots_blood_plague -> is_ticking() )
        {
          p() -> drw_blood_plague -> target = s -> target;
          p() -> drw_blood_plague -> execute();
        }

        if ( td( target ) -> dots_frost_fever -> is_ticking() )
        {
          p() -> drw_frost_fever -> target = s -> target;
          p() -> drw_frost_fever -> execute();
        }

        if ( td( target ) -> dots_necrotic_plague -> is_ticking() )
        {
          p() -> drw_necrotic_plague -> target = s -> target;
          p() -> drw_necrotic_plague -> execute();
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
        if ( ! p() -> o() -> talent.necrotic_plague -> ok() )
        {
          p() -> drw_blood_plague -> target = target;
          p() -> drw_blood_plague -> execute();

          p() -> drw_frost_fever -> target = target;
          p() -> drw_frost_fever -> execute();
        }
        else
        {
          p() -> drw_necrotic_plague -> target = target;
          p() -> drw_necrotic_plague -> execute();
        }
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
        if ( ! p() -> o() -> talent.necrotic_plague -> ok() )
        {
          p() -> drw_blood_plague -> target = s->target;
          p() -> drw_blood_plague -> execute();
        }
        else
        {
          p() -> drw_necrotic_plague -> target = s -> target;
          p() -> drw_necrotic_plague -> execute();
        }
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
      if ( o() -> perk.improved_soul_reaper -> ok() )
        pct = o() -> perk.improved_soul_reaper -> effectN( 1 ).base_value();

      if ( o() -> sets.has_set_bonus( SET_MELEE, T15, B4 ) )
        pct = o() -> sets.set( SET_MELEE, T15, B4 ) -> effectN( 1 ).base_value();

      if ( dot -> state -> target -> health_percentage() <= pct )
        drw_melee_attack_t::tick( dot );
    }
  };

  struct drw_melee_t : public drw_melee_attack_t
  {
    drw_melee_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "auto_attack_mh", p )
    {
      auto_attack       = true;
      weapon            = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      special           = false;
    }
  };

  target_specific_t<dancing_rune_weapon_td_t*> target_data;

  spell_t*        drw_blood_plague;
  spell_t*        drw_frost_fever;
  spell_t*        drw_necrotic_plague;

  spell_t*        drw_death_coil;
  spell_t*        drw_death_siphon;
  spell_t*        drw_icy_touch;
  spell_t*        drw_outbreak;
  spell_t*        drw_blood_boil;

  melee_attack_t* drw_death_strike;
  melee_attack_t* drw_plague_strike;
  melee_attack_t* drw_soul_reaper;
  melee_attack_t* drw_melee;

  dancing_rune_weapon_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "dancing_rune_weapon", true ),
    drw_blood_plague( nullptr ), drw_frost_fever( nullptr ), drw_necrotic_plague( 0 ),
    drw_death_coil( nullptr ),
    drw_death_siphon( nullptr ), drw_icy_touch( nullptr ),
    drw_outbreak( nullptr ), drw_blood_boil( nullptr ),
    drw_death_strike( nullptr ),
    drw_plague_strike( nullptr ),
    drw_soul_reaper( nullptr ), drw_melee( nullptr )
  {
    main_hand_weapon.type       = WEAPON_BEAST_2H;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 3.0;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 3.0;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.5 );

    owner_coeff.ap_from_ap = 1.0;
    regen_type = REGEN_DISABLED;
  }

  death_knight_t* o() const
  { return static_cast< death_knight_t* >( owner ); }

  dancing_rune_weapon_td_t* td( player_t* t ) const
  { return get_target_data( t ); }

  virtual dancing_rune_weapon_td_t* get_target_data( player_t* target ) const
  {
    dancing_rune_weapon_td_t*& td = target_data[ target ];
    if ( ! td )
      td = new dancing_rune_weapon_td_t( target, const_cast<dancing_rune_weapon_pet_t*>(this) );
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
    drw_necrotic_plague = new drw_necrotic_plague_t( this );

    drw_death_coil    = new drw_death_coil_t   ( this );
    drw_death_siphon  = new drw_death_siphon_t ( this );
    drw_icy_touch     = new drw_icy_touch_t    ( this );
    drw_outbreak      = new drw_outbreak_t     ( this );
    drw_blood_boil    = new drw_blood_boil_t   ( this );

    drw_death_strike  = new drw_death_strike_t ( this );
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

  double composite_player_multiplier( school_e school ) const
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
  dots_necrotic_plague = target -> get_dot( "necrotic_plague",     drw );

  debuffs_necrotic_plague = buff_creator_t( *this, "necrotic_plague", drw -> owner -> find_spell( 155159 ) ).period( timespan_t::zero() );
}

struct death_knight_pet_t : public pet_t
{
  const spell_data_t* command;

  death_knight_pet_t( sim_t* sim, death_knight_t* owner, const std::string& n, bool guardian, bool dynamic = false ) :
    pet_t( sim, owner, n, guardian, dynamic )
  {
    command = find_spell( 54562 );
  }

  death_knight_t* o()
  { return debug_cast<death_knight_t*>( owner ); }

  double composite_player_multiplier( school_e school ) const
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

      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pets.army_ghoul[ 0 ] )
        stats = p() -> o() -> pets.army_ghoul[ 0 ] -> get_stats( name_str );
    }
  };

  struct army_ghoul_pet_melee_t : public army_ghoul_pet_melee_attack_t
  {
    army_ghoul_pet_melee_t( army_ghoul_pet_t* p ) :
      army_ghoul_pet_melee_attack_t( "auto_attack_mh", p )
    {
      auto_attack       = true;
      school            = SCHOOL_PHYSICAL;
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
    //initial.stats.attack_power = -20;

    // Ghouls don't appear to gain any crit from agi, they may also just have none
    // initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;

    owner_coeff.ap_from_ap = 0.0415;
  }

  virtual resource_e primary_resource() const { return RESOURCE_ENERGY; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new  army_ghoul_pet_auto_melee_attack_t( this );
    if ( name == "claw"           ) return new         army_ghoul_pet_claw_t( this );

    return pet_t::create_action( name, options_str );
  }

  timespan_t available() const
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
// Gargoyle
// ==========================================================================

struct gargoyle_pet_t : public death_knight_pet_t
{
  struct travel_t : public action_t
  {
    bool executed;

    travel_t( player_t* player ) :
      action_t( ACTION_OTHER, "travel", player ),
      executed( false )
    {
      may_miss = false;
      dual = true;
    }

    result_e calculate_result( action_state_t* /* s */ )
    { return RESULT_HIT; }

    block_result_e calculate_block_result( action_state_t* )
    { return BLOCK_RESULT_UNBLOCKED; }

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
    timespan_t execute_time() const
    { return timespan_t::from_seconds( const_cast<travel_t*>( this ) -> rng().gauss( 2.9, 0.2 ) ); }

    bool ready()
    { return ! executed; }
  };

  struct gargoyle_strike_t : public spell_t
  {
    gargoyle_strike_t( gargoyle_pet_t* pet ) :
      spell_t( "gargoyle_strike", pet, pet -> find_pet_spell( "Gargoyle Strike" ) )
    {
      harmful            = true;
      trigger_gcd        = timespan_t::from_seconds( 1.5 );
      may_crit           = true;
      school             = SCHOOL_SHADOWSTORM;
    }

    double composite_da_multiplier( const action_state_t* state ) const
    {
      double m = spell_t::composite_da_multiplier( state );

      death_knight_t* dk = debug_cast< death_knight_t* >( static_cast<gargoyle_pet_t*>(player) -> owner );
      if ( dk -> mastery.dreadblade -> ok() )
        m *= 1.0 + dk -> cache.mastery_value();

      return m;
    }
  };

  gargoyle_pet_t( sim_t* sim, death_knight_t* owner ) :
    death_knight_pet_t( sim, owner, "gargoyle", true )
  { regen_type = REGEN_DISABLED; }

  virtual void init_base_stats()
  {
    action_list_str = "travel/gargoyle_strike";

    // As per Blizzard
    owner_coeff.sp_from_ap = 0.46625;
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

    virtual double action_multiplier() const
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
      ghoul_pet_melee_attack_t( "auto_attack_mh", p )
    {
      auto_attack       = true;
      school            = SCHOOL_PHYSICAL;
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

    //initial.stats.attack_power = -20;

    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
    owner_coeff.ap_from_ap = 0.2664;
  }

  //Ghoul regen doesn't benefit from haste (even bloodlust/heroism)
  virtual resource_e primary_resource() const
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

  timespan_t available() const
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
    death_knight_pet_t( owner -> sim, owner, "fallen_zandalari", true, true )
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
      if ( ! player -> sim -> report_pets_separately )
        stats = p -> o() -> pets.fallen_zandalari[ 0 ] -> find_action( name_str ) -> stats;
    }
  };

  struct zandalari_melee_t : public zandalari_melee_attack_t
  {
    zandalari_melee_t( fallen_zandalari_t* p ) :
      zandalari_melee_attack_t( "auto_attack_mh", p )
    {
      auto_attack       = true;
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

  virtual resource_e primary_resource() const
  { return RESOURCE_ENERGY; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack" ) return new zandalari_auto_attack_t( this );
    if ( name == "strike"        ) return new    zandalari_strike_t( this );

    return death_knight_pet_t::create_action( name, options_str );
  }

  timespan_t available() const
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

struct shadow_of_death_t : public buff_t
{
  int health_gain;

  shadow_of_death_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "shadow_of_death", p -> find_spell( 164047 ) ).chance( p -> perk.enhanced_death_coil_blood -> ok() ).refresh_behavior( BUFF_REFRESH_DISABLED ) ),
    health_gain( 0 )
  { }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    int gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent() );
    player -> stat_gain( STAT_MAX_HEALTH, gain, 0, 0, true );
    health_gain += gain;

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire_override()
  {
    player -> stat_loss( STAT_MAX_HEALTH, health_gain, 0, 0, true );

    health_gain = 0;
    buff_t::expire_override();
  }

  void reset()
  {
    buff_t::reset();

    health_gain = 0;
  }
};

// Template for common death knight action code. See priest_action_t.
template <class Base>
struct death_knight_action_t : public Base
{
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  int    cost_death;
  double convert_runes;
  std::array<bool,RUNE_SLOT_MAX> use;
  gain_t* rp_gains;

  typedef Base action_base_t;
  typedef death_knight_action_t base_t;

  death_knight_action_t( const std::string& n, death_knight_t* p,
                         const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, p, s ),
    cost_blood( 0 ), cost_frost( 0 ), cost_unholy( 0 ), cost_death( 0 ),
    convert_runes( 0 )
  {
    range::fill( use, false );

    action_base_t::may_crit   = true;
    action_base_t::may_glance = false;

    rp_gains = action_base_t::player -> get_gain( "rp_" + action_base_t::name_str );
    extract_rune_cost( s );
  }

  death_knight_t* p() const { return static_cast< death_knight_t* >( action_base_t::player ); }

  death_knight_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }


  void trigger_t16_2pc_tank()
  {
    death_knight_t* p = this -> p();
    if ( ! p -> sets.has_set_bonus( SET_TANK, T16, B2 ) )
      return;

    p -> t16_tank_2pc_driver++;

    if ( p -> t16_tank_2pc_driver == p -> sets.set( SET_TANK, T16, B2 ) -> effectN( 1 ).base_value() )
    {
      p -> buffs.bone_wall -> trigger();
      p -> t16_tank_2pc_driver = 0;
    }
  }

  void trigger_t16_4pc_melee( action_state_t* s )
  {
    death_knight_t* p = this -> p();
    if ( ! p -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      return;

    if ( ! this -> special || ! this -> harmful || this -> proc )
      return;

    if ( ! this -> result_is_hit( s -> result ) )
      return;

    if ( s -> result_amount <= 0 )
      return;

    if ( ! p -> buffs.pillar_of_frost -> check() )
      return;

    p -> active_spells.frozen_power -> target = s -> target;
    p -> active_spells.frozen_power -> schedule_execute();
  }

// Virtual Overrides

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
      action_base_t::consume_resource();
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double m = action_base_t::composite_target_multiplier( t );

    if ( dbc::is_school( action_base_t::school, SCHOOL_FROST ) )
    {
      double debuff = td( t ) -> debuffs_razorice -> data().effectN( 1 ).percent();

      m *= 1.0 + td( t ) -> debuffs_razorice -> check() * debuff;
    }

    return m;
  }

  virtual expr_t* create_expression( const std::string& name_str );

private:
  void extract_rune_cost( const spell_data_t* spell )
  {
    // Rune costs appear to be in binary: 0a0b0c0d where 'd' is whether the ability
    // costs a blood rune, 'c' is whether it costs an unholy rune, 'b'
    // is whether it costs a frost rune, and 'a' is whether it costs a  death
    // rune

    if ( ! spell -> ok() ) return;

    uint32_t rune_cost = spell -> rune_cost();
    cost_blood  =          rune_cost & 0x1;
    cost_unholy = ( rune_cost >> 2 ) & 0x1;
    cost_frost  = ( rune_cost >> 4 ) & 0x1;
    cost_death  = ( rune_cost >> 6 ) & 0x1;
  }
};

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_melee_attack_t : public death_knight_action_t<melee_attack_t>
{
  bool   always_consume;

  death_knight_melee_attack_t( const std::string& n, death_knight_t* p,
                               const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ), always_consume( false )
  {
    may_crit   = true;
    may_glance = false;
  }

  void trigger_t15_2pc_melee()
  {
    death_knight_t* p = this -> p();

    if ( ! p -> sets.has_set_bonus( SET_MELEE, T15, B2 ) )
      return;

    if ( ( p -> t15_2pc_melee.trigger() ) )
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

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   impact( action_state_t* state );

  virtual double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = base_t::composite_da_multiplier( state );

    if ( player -> main_hand_weapon.group() == WEAPON_2H )
      m *= 1.0 + p() -> spec.might_of_the_frozen_wastes -> effectN( 2 ).percent();

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
  }

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   impact( action_state_t* state );

  virtual double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = base_t::composite_da_multiplier( state );

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

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_melee_attack_t::consume_resource() ==========================

void death_knight_melee_attack_t::consume_resource()
{
  base_t::consume_resource();

  if ( result_is_hit( execute_state -> result ) || always_consume )
    consume_runes( p(), use, convert_runes == 0 ? false : rng().roll( convert_runes ) == 1 );

  if ( p() -> active_spells.breath_of_sindragosa &&
       p() -> resources.current[ RESOURCE_RUNIC_POWER ] < p() -> active_spells.breath_of_sindragosa -> tick_action -> base_costs[ RESOURCE_RUNIC_POWER ] )
  {
    p() -> active_spells.breath_of_sindragosa -> get_dot() -> cancel();
  }

  if ( p() -> active_spells.conversion &&
       p() -> resources.current[ RESOURCE_RUNIC_POWER ] < p() -> active_spells.conversion -> tick_action -> base_costs[ RESOURCE_RUNIC_POWER ] )
  {
    p() -> active_spells.conversion -> get_dot() -> cancel();
  }
}

// death_knight_melee_attack_t::execute() ===================================

void death_knight_melee_attack_t::execute()
{
  base_t::execute();

  if ( ! result_is_hit( execute_state -> result ) && ! always_consume && resource_consumed > 0 )
    p() -> resource_gain( RESOURCE_RUNIC_POWER, resource_consumed * RUNIC_POWER_REFUND, p() -> gains.power_refund );

  if ( result_is_hit( execute_state -> result ) && td( execute_state -> target ) -> dots_blood_plague -> is_ticking() )
    p() -> buffs.crimson_scourge -> trigger();

  trigger_t15_2pc_melee();
}

// death_knight_melee_attack_t::impact() ====================================

void death_knight_melee_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  trigger_t16_4pc_melee( state );
  p() -> trigger_t17_4pc_frost( state );
}

// death_knight_melee_attack_t::ready() =====================================

bool death_knight_melee_attack_t::ready()
{
  if ( ! base_t::ready() )
    return false;

  //ready_in( p(), cost_blood, cost_frost, cost_unholy );
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
    consume_runes( p(), use, convert_runes == 0 ? false : rng().roll( convert_runes ) == 1 );

  if ( p() -> active_spells.breath_of_sindragosa &&
       p() -> resources.current[ RESOURCE_RUNIC_POWER ] < p() -> active_spells.breath_of_sindragosa -> tick_action -> base_costs[ RESOURCE_RUNIC_POWER ] )
  {
    p() -> active_spells.breath_of_sindragosa -> get_dot() -> cancel();
  }
}

// death_knight_spell_t::execute() ==========================================

void death_knight_spell_t::execute()
{
  base_t::execute();
}

// death_knight_spell_t::impact() ===========================================

void death_knight_spell_t::impact( action_state_t* state )
{
  base_t::impact( state );

  trigger_t16_4pc_melee( state );
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
  bool first;

  melee_t( const char* name, death_knight_t* p, int sw ) :
    death_knight_melee_attack_t( name, p ), sync_weapons( sw ), first ( true )
  {
    auto_attack     = true;
    school          = SCHOOL_PHYSICAL;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = timespan_t::zero();

    if ( p -> dual_wield() )
      base_hit -= 0.19;
  }

 void reset()
  {
    death_knight_melee_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = death_knight_melee_attack_t::execute_time();

    if ( first )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    else
      return t;
  }

  virtual void execute()
  {
    if ( first )
      first = false;

    death_knight_melee_attack_t::execute();
  }


  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_multistrike( s -> result ) && p() -> spec.blood_rites -> ok() )
    {
      double chance = 1.0; // TODO: Blood Rites proc chance?
      if ( rng().roll( chance ) )
        p() -> resource_gain( RESOURCE_RUNIC_POWER,
                              p() -> spell.blood_rites -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                              p() -> gains.blood_rites );
    }

    if ( result_is_hit( s -> result ) )
    {
      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        // T13 2pc gives 2 stacks of SD, otherwise we can only ever have one
        // Ensure that if we have 1 that we only refresh, not add another stack
        int new_stacks = ( p() -> rng().roll( p() -> sets.set( SET_MELEE, T13, B2 ) -> effectN( 1 ).percent() ) ) ? 2 : 1;

        // Mists of Pandaria Sudden Doom is 3 PPM
        if ( p() -> spec.sudden_doom -> ok() &&
             p() -> rng().roll( weapon -> proc_chance_on_swing( 3 ) ) )
        {
          // If we're proccing 2 or we have 0 stacks, trigger like normal
          if ( new_stacks == 2 || p() -> buffs.sudden_doom -> check() == 0 )
            p() -> buffs.sudden_doom -> trigger( new_stacks );
          // refresh stacks. However if we have a double stack and only 1 procced, it refreshes to 1 stack
          else
          {
            p() -> buffs.sudden_doom -> refresh( 0 );
            if ( p() -> buffs.sudden_doom -> check() == 2 && new_stacks == 1 )
              p() -> buffs.sudden_doom -> decrement( 1 );
          }

          p() -> buffs.death_shroud -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, timespan_t::from_seconds( 15 ) );
        }

        if ( p() -> spec.scent_of_blood -> ok() && 
             p() -> buffs.scent_of_blood -> trigger( 1, buff_t::DEFAULT_VALUE(), weapon -> swing_time.total_seconds() / 3.6 ) )
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

    p -> main_hand_attack = new melee_t( "auto_attack_mh", p, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "auto_attack_oh", p, sync_weapons );
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
  }

  virtual void schedule_execute( action_state_t* s )
  {
    death_knight_spell_t::schedule_execute( s );

    p() -> buffs.army_of_the_dead -> trigger( 1, p() -> cache.dodge() + p() -> cache.parry() );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    if ( ! p() -> in_combat )
    {
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

      //simulate RP decay for that 5 seconds
      p() -> resource_loss( RESOURCE_RUNIC_POWER, p() -> runic_power_decay_rate * 5, 0, 0 );
    }
    else
    {
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

// Blood Plague =============================================================

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "blood_plague", p, p -> find_spell( 55078 ) )
  {
    tick_may_crit    = true;
    background       = true;
    dot_behavior     = DOT_REFRESH;
    may_miss         = false;
    may_crit         = false;
    hasted_ticks     = false;

    // TODO-WOD: Check if multiplicative
    base_multiplier *= 1.0 + p -> spec.ebon_plaguebringer -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> spec.crimson_scourge -> effectN( 3 ).percent();
    if ( p -> glyph.enduring_infection -> ok() )
      base_multiplier += p -> find_spell( 58671 ) -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + p -> perk.improved_diseases -> effectN( 1 ).percent();
  }

  // WODO-TOD: Crit suppression hijinks?
  virtual double composite_crit() const
  { return action_t::composite_crit() + player -> cache.attack_crit(); }
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

    // TODO-WOD: Check if multiplicative
    base_multiplier *= 1.0 + p -> spec.ebon_plaguebringer -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> spec.crimson_scourge -> effectN( 3 ).percent();
    if ( p -> glyph.enduring_infection -> ok() )
      base_multiplier *= 1.0 + p -> find_spell( 58671 ) -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + p -> perk.improved_diseases -> effectN( 1 ).percent();
  }

  virtual double composite_crit() const
  { return action_t::composite_crit() + player -> cache.attack_crit(); }

};

// Necrotic Plague ==========================================================

struct necrotic_plague_t : public death_knight_spell_t
{
  necrotic_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "necrotic_plague", p, p -> find_spell( 155159 ) )
  {
    hasted_ticks = may_miss = may_crit = false;
    background = tick_may_crit = true;
    base_multiplier *= 1.0 + p -> spec.ebon_plaguebringer -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> perk.improved_diseases -> effectN( 2 ).percent();
    dot_behavior = DOT_REFRESH;
  }

  double composite_target_multiplier( player_t* target ) const
  {
    double m = death_knight_spell_t::composite_target_multiplier( target );

    death_knight_td_t* tdata = td( target );
    if ( tdata -> dots_necrotic_plague -> is_ticking() )
      m *= tdata -> debuffs_necrotic_plague -> stack();

    return m;
  }

  void last_tick( dot_t* dot )
  {
    death_knight_spell_t::last_tick( dot );
    td( dot -> target ) -> debuffs_necrotic_plague -> expire();
  }

  // Spread Necrotic Plagues use the source NP dot's duration on the target,
  // and never pandemic-extnd the dot on the target.
  timespan_t calculate_dot_refresh_duration( const dot_t*, timespan_t triggered_duration ) const
  { return triggered_duration; }

  // Necrotic Plague duration will not be refreshed if it is already ticking on
  // the target, only the stack count will go up. Only Festering Strike is
  // allowed to extend the duration of the Necrotic Plague dot.
  void trigger_dot( action_state_t* s )
  {
    death_knight_td_t* tdata = td( s -> target );
    if ( ! tdata -> dots_necrotic_plague -> is_ticking() )
      death_knight_spell_t::trigger_dot( s );

    tdata -> debuffs_necrotic_plague -> trigger();

    if ( sim -> debug )
      sim -> out_debug.printf( "%s %s stack increases to %d",
          player -> name(), name(), tdata -> debuffs_necrotic_plague -> check() );
  }

  void tick( dot_t* dot )
  {
    death_knight_spell_t::tick( dot );

    if ( result_is_hit( dot -> state -> result ) && dot -> ticks_left() > 0 )
    {
      death_knight_td_t* tdata = td( dot -> state -> target );
      tdata -> debuffs_necrotic_plague -> trigger();

      if ( sim -> debug )
        sim -> out_debug.printf( "%s %s stack %d",
            player -> name(), name(), tdata -> debuffs_necrotic_plague -> check() );
      new ( *sim ) np_spread_event_t<death_knight_td_t>( dot );
    }
  }

  double composite_crit() const
  { return action_t::composite_crit() + player -> cache.attack_crit(); }
};

// Necrosis =================================================================

struct necrosis_t : public death_knight_spell_t
{
  necrosis_t( death_knight_t* player ) :
    death_knight_spell_t( "necrosis", player, player -> spec.necrosis -> effectN( 2 ).trigger() )
  {
    background = true;
  }
};

// Frozen Runeblade =========================================================

struct frozen_runeblade_attack_t : public death_knight_melee_attack_t
{
  frozen_runeblade_attack_t( death_knight_t* player ) :
    death_knight_melee_attack_t( "frozen_runeblade", player, player -> find_spell( 170205 ) -> effectN( 1 ).trigger() )
  {
    background = true;
    // Implicitly uses main hand weapon
    weapon = &( player -> main_hand_weapon );
  }

  bool usable_moving() const
  { return true; }
};

struct frozen_runeblade_driver_t : public death_knight_spell_t
{
  frozen_runeblade_attack_t* attack;

  frozen_runeblade_driver_t( death_knight_t* player ) :
    death_knight_spell_t( "frozen_runeblade_driver", player, player -> find_spell( 170205 ) ),
    attack( new frozen_runeblade_attack_t( player ) )
  {
    background = proc = dual = quiet = true;
    callbacks = may_miss = may_crit = hasted_ticks = tick_may_crit = false;
  }

  void tick( dot_t* dot )
  {
    death_knight_spell_t::tick( dot );

    attack -> schedule_execute();
  }
};

// Soul Reaper ==============================================================

struct soul_reaper_dot_t : public death_knight_melee_attack_t
{
  soul_reaper_dot_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "soul_reaper_execute", p, p -> find_spell( 114867 ) )
  {
    special = background = may_crit = proc = true;
    may_miss = may_dodge = may_parry = may_block = false;
    weapon_multiplier = 0;
  }

  void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( p() -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T17, B2 ) )
      p() -> buffs.shadow_infusion -> trigger( p() -> sets.set( DEATH_KNIGHT_UNHOLY, T17, B2 ) -> effectN( 1 ).base_value() );
  }
};

struct soul_reaper_t : public death_knight_melee_attack_t
{
  soul_reaper_dot_t* soul_reaper_dot;

  soul_reaper_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "soul_reaper", p, p -> specialization() != DEATH_KNIGHT_UNHOLY ? p -> find_specialization_spell( "Soul Reaper" ) : p -> find_spell( 130736 ) ),
    soul_reaper_dot( 0 )
  {
    parse_options( NULL, options_str );
    special   = true;

    weapon = &( p -> main_hand_weapon );

    dynamic_tick_action = true;
    tick_action = new soul_reaper_dot_t( p );
  }

  virtual double composite_crit() const
  {
    double cc = death_knight_melee_attack_t::composite_crit();
    if ( player -> sets.has_set_bonus( SET_MELEE, T15, B4 ) && p() -> buffs.killing_machine -> check() )
      cc += p() -> buffs.killing_machine -> value();
    return cc;
  }

  double composite_ta_multiplier( const action_state_t* state ) const
  {
    double m = death_knight_melee_attack_t::composite_ta_multiplier( state );

    if ( p() -> mastery.dreadblade -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  void init()
  {
    death_knight_melee_attack_t::init();

    snapshot_flags |= STATE_MUL_TA;
  }

  void impact( action_state_t* state )
  {
    death_knight_melee_attack_t::impact( state );

    p() -> trigger_necrosis( state );
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();
    if ( player -> sets.has_set_bonus( SET_MELEE, T15, B4 ) && p() -> buffs.killing_machine -> check() )
    {
      p() -> procs.sr_killing_machine -> occur();
      p() -> buffs.killing_machine -> expire();
    }

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_soul_reaper -> execute();

    if ( result_is_hit( execute_state -> result ) )
      trigger_t16_2pc_tank();
  }

  void tick( dot_t* dot )
  {
    int pct = p() -> sets.has_set_bonus( SET_MELEE, T15, B4 ) ? p() -> sets.set( SET_MELEE, T15, B4 ) -> effectN( 1 ).base_value() : 35;
    if ( p() -> perk.improved_soul_reaper -> ok() )
      pct = p() -> perk.improved_soul_reaper -> effectN( 1 ).base_value();

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

// Conversion ===============================================================

struct conversion_heal_t : public death_knight_heal_t
{
  conversion_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "conversion_heal", p, p -> find_spell( 119980 ) )
  {
    may_crit = may_multistrike = false;
    background = true;
    resource_current = RESOURCE_RUNIC_POWER;
    base_costs[ RESOURCE_RUNIC_POWER ] = 15;
    target = p;
    pct_heal = data().effectN( 1 ).percent();
  }

  void execute()
  {
    death_knight_heal_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> trigger_runic_empowerment( resource_consumed );
      p() -> trigger_blood_charge( resource_consumed );
      p() -> trigger_runic_corruption( resource_consumed );
      p() -> trigger_shadow_infusion( resource_consumed );
    }
  }
};

struct conversion_t : public death_knight_heal_t
{
  conversion_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_heal_t( "conversion", p, p -> talent.conversion )
  {
    parse_options( NULL, options_str );

    may_miss = may_crit = hasted_ticks = callbacks = false;
    dot_duration = sim -> max_time * 2;

    tick_action = new conversion_heal_t( p );
    for ( size_t idx = 1; idx <= p -> talent.conversion -> power_count(); idx++ )
    {
      const spellpower_data_t& power = p -> talent.conversion -> powerN( idx );
      if ( power.spell_id() && p -> dbc.spec_by_spell( power.aura_id() ) == p -> specialization() )
      {
        base_costs[ power.resource() ] = power.cost_per_second();
        break;
      }
    }

    target = p;
  }

  void execute()
  {
    dot_t* d = get_dot( target );

    if ( d -> is_ticking() )
      d -> cancel();
    else
    {
      p() -> active_spells.conversion = this;
      death_knight_heal_t::execute();

      p() -> buffs.conversion -> trigger();
    }
  }

  void last_tick( dot_t* dot )
  {
    death_knight_heal_t::last_tick( dot );
    p() -> active_spells.conversion = 0;
    p() -> buffs.conversion -> expire();
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

    if ( sim -> debug ) log_rune_status( p(), true );

    rune_t* regen_rune = &( p() -> _runes.slot[ selected_rune ] );

    regen_rune -> fill_rune();
    regen_rune -> type |= RUNE_TYPE_DEATH;

    p() -> gains.blood_tap -> add( RESOURCE_RUNE, 1, 0 );
    if ( regen_rune -> is_blood() )
      p() -> gains.blood_tap_blood -> add( RESOURCE_RUNE, 1, 0 );
    else if ( regen_rune -> is_frost() )
      p() -> gains.blood_tap_frost -> add( RESOURCE_RUNE, 1, 0 );
    else
      p() -> gains.blood_tap_unholy -> add( RESOURCE_RUNE, 1, 0 );

    if ( sim -> log ) sim -> out_log.printf( "%s regened rune %d", name(), selected_rune );

    p() -> buffs.blood_charge -> decrement( consume_charges );

    // Called last so we print the correct runes
    death_knight_spell_t::execute();
  }

  bool ready()
  {
    if ( p() -> buffs.blood_charge -> check() < consume_charges )
      return false;

    bool rd = death_knight_spell_t::ready();

    rune_t& b = p() -> _runes.slot[ 0 ];
    if ( b.is_depleted() || b.paired_rune -> is_depleted() )
      return rd;

    rune_t& f = p() -> _runes.slot[ 2 ];
    if ( f.is_depleted() || f.paired_rune -> is_depleted() )
      return rd;

    rune_t& u = p() -> _runes.slot[ 4 ];
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
      timespan_t pre_cast = timespan_t::from_seconds( rng().range( 8.0, 16.0 ) );

      cooldown -> duration -= pre_cast;
      p() -> buffs.bone_shield -> buff_duration -= pre_cast;

      p() -> buffs.bone_shield -> trigger( static_cast<int>(max_stacks) ); // FIXME
      death_knight_spell_t::execute();

      cooldown -> duration += pre_cast;
      p() -> buffs.bone_shield -> buff_duration += pre_cast;
    }
    else
    {
      p() -> buffs.bone_shield -> trigger( static_cast<int>(max_stacks) ); // FIXME
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
    may_miss = may_crit = may_dodge = may_parry = harmful = false;

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.dancing_rune_weapon -> trigger();
    p() -> pets.dancing_rune_weapon -> summon( data().duration() );

    if ( p() -> sets.has_set_bonus( SET_TANK, T16, B4 ) )
    {
      for ( int i = 2; i < RUNE_SLOT_MAX; ++i )
      {
        p() -> _runes.slot[ i ].type |= RUNE_TYPE_DEATH;
        p() -> _runes.slot[ i ].fill_rune();
      }

      p() -> buffs.deathbringer -> trigger( p() -> buffs.deathbringer -> data().initial_stacks() );
    }
  }
};

// Dark Command =======================================================================

struct dark_command_t: public death_knight_spell_t
{
  dark_command_t( death_knight_t* p, const std::string& options_str ):
    death_knight_spell_t( "dark_command", p, p -> find_class_spell( "Dark Command" ) )
  {
    parse_options( NULL, options_str );
    use_off_gcd = true;
  }

  virtual void impact( action_state_t* s )
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    death_knight_spell_t::impact( s );
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

    if ( p -> perk.enhanced_dark_transformation -> ok() )
      cost_blood = cost_unholy = cost_frost = cost_death = 0;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.dark_transformation -> trigger();
    p() -> buffs.shadow_infusion -> expire();
    p() -> trigger_t17_4pc_unholy( execute_state );
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
    attack_power_mod.tick = p -> find_spell( 52212 ) -> effectN( 1 ).ap_coeff();
    base_tick_time   = timespan_t::from_seconds( 1.0 );
    dot_duration = data().duration(); // 11 with tick_zero
    tick_may_crit = tick_zero = true;
    hasted_ticks     = false;
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const
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
      if ( sim -> debug ) sim -> out_debug.printf( "Ground effect %s can not hit flying target %s", name(), s -> target -> name() );
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

    if ( p() -> talent.defile -> ok() )
      return false;

    if ( p() -> buffs.crimson_scourge -> check() )
      return group_runes( p(), 0, 0, 0, 0, use );
    else
      return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
  }
};

// Defile ==================================================================

struct defile_t : public death_knight_spell_t
{
  defile_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "defile", p, p -> find_talent_spell( "Defile" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
    base_dd_min = base_dd_max = 0;
    school = p -> find_spell( 156000 ) -> get_school_type();
    attack_power_mod.tick = p -> find_spell( 156000 ) -> effectN( 1 ).ap_coeff();
    dot_duration = data().duration();
    tick_may_crit = tick_zero = true; // Zero tick or not?
    hasted_ticks = false;
  }

  double composite_ta_multiplier( const action_state_t* state ) const
  {
    double m = death_knight_spell_t::composite_ta_multiplier( state );

    dot_t* dot = find_dot( state -> target );
    // TODO-WOD: Tooltip is wrong, defile increases by 5% per tick
    if ( dot )
      m *= std::pow( 1.0 + /* data().effectN( 2 ).percent()*/ 0.05, dot -> current_tick );

    return m;
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const
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
      if ( sim -> debug ) sim -> out_debug.printf( "Ground effect %s can not hit flying target %s", name(), s -> target -> name() );
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

    attack_power_mod.direct = 0.85;

    base_multiplier *= 1.0 + p -> perk.improved_death_coil -> effectN( 2 ).percent();
  }

  virtual double cost() const
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

    p() -> trigger_shadow_infusion( base_costs[ RESOURCE_RUNIC_POWER ] );
    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD )
      p() -> buffs.shadow_of_death -> trigger();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> trigger_runic_empowerment( base_costs[ RESOURCE_RUNIC_POWER ] );
      p() -> trigger_blood_charge( base_costs[ RESOURCE_RUNIC_POWER ] );
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ] );
      p() -> trigger_plaguebearer( execute_state );

      if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) && p() -> buffs.dark_transformation -> check() )
        p() -> buffs.dark_transformation -> extend_duration( p(), timespan_t::from_millis( p() -> sets.set( SET_MELEE, T16, B4 ) -> effectN( 1 ).base_value() ) );

      trigger_t16_2pc_tank();
    }
  }
};

// Death Strike =============================================================

struct blood_shield_buff_t : public absorb_buff_t
{
  blood_shield_buff_t( death_knight_t* player ) :
    absorb_buff_t( absorb_buff_creator_t( player, "blood_shield", player -> find_spell( 77535 ) )
                   .school( SCHOOL_PHYSICAL )
                   .source( player -> get_stats( "blood_shield" ) ) )
  { }

  // Clamp shield value so that if T17 4PC is used, we have at least 5% of
  // current max health of absorb left, if Vampiric Blood is up
  void absorb_used( double )
  {
    death_knight_t* p = debug_cast<death_knight_t*>( player );
    if ( p -> sets.has_set_bonus( DEATH_KNIGHT_BLOOD, T17, B4 ) && p -> buffs.vampiric_blood -> up() )
    {
      double min_absorb = p -> resources.max[ RESOURCE_HEALTH ] *
                          p -> sets.set( DEATH_KNIGHT_BLOOD, T17, B4 ) -> effectN( 1 ).percent();

      if ( sim -> debug )
        sim -> out_debug.printf( "%s blood_shield absorb clamped to %f", player -> name(), min_absorb );

      if ( current_value < min_absorb )
        current_value = min_absorb;
    }
  }
};

struct death_strike_offhand_t : public death_knight_melee_attack_t
{
  death_strike_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "death_strike_offhand", p, p -> find_spell( 66188 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    base_multiplier  = 1.0 + p -> spec.veteran_of_the_third_war -> effectN( 7 ).percent();
  }
};

struct blood_shield_t : public absorb_t
{
  blood_shield_t( death_knight_t* p ) :
    absorb_t( "blood_shield", p, p -> find_spell( 77535 ) )
  {
    may_miss = may_crit = may_multistrike = callbacks = false;
    background = proc = true;
  }

  // Self only so we can do this in a simple way
  absorb_buff_t* create_buff( const action_state_t* )
  { return debug_cast<death_knight_t*>( player ) -> buffs.blood_shield; }

  void init()
  {
    absorb_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct death_strike_heal_t : public death_knight_heal_t
{
  blood_shield_t* blood_shield;

  death_strike_heal_t( death_knight_t* p ) :
    death_knight_heal_t( "death_strike_heal", p, p -> find_spell( 45470 ) ),
    blood_shield( p -> specialization() == DEATH_KNIGHT_BLOOD ? new blood_shield_t( p ) : 0 )
  {
    may_crit   = may_multistrike = callbacks = false;
    background = true;
    target     = p;
  }

  double action_multiplier() const
  {
    double m = death_knight_heal_t::action_multiplier();

    m *= 1.0 + p() -> buffs.scent_of_blood -> check() * p() -> buffs.scent_of_blood -> data().effectN( 1 ).percent();
    m *= 1.0 + p() -> perk.improved_death_strike -> effectN( 1 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    death_knight_heal_t::impact( state );

    trigger_blood_shield( state );
  }

  void consume_resource()
  {
    death_knight_heal_t::consume_resource();

    p() -> buffs.scent_of_blood -> expire();
  }

  void trigger_blood_shield( action_state_t* state )
  {
    if ( p() -> specialization() != DEATH_KNIGHT_BLOOD )
      return;

    double current_value = 0;
    if ( blood_shield -> target_specific[ state -> target ] )
      current_value = blood_shield -> target_specific[ state -> target ] -> current_value;

    double amount = current_value;
    if ( p() -> mastery.blood_shield -> ok() )
      amount += state -> result_total * p() -> cache.mastery_value();

    amount = std::min( p() -> resources.max[ RESOURCE_HEALTH ], amount );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s Blood Shield buff trigger, old_value=%f added_value=%f new_value=%f",
                     player -> name(), current_value,
                     state -> result_amount * p() -> cache.mastery_value(),
                     amount );

    blood_shield -> base_dd_min = blood_shield -> base_dd_max = amount;
    blood_shield -> execute();
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
    base_multiplier = 1.0 + p -> spec.veteran_of_the_third_war -> effectN( 7 ).percent();
    base_multiplier = 1.0 + p -> perk.enhanced_death_strike -> effectN( 1 ).percent();

    always_consume = true; // Death Strike always consumes runes, even if doesn't hit

    if ( p -> spec.blood_rites -> ok() )
      convert_runes = 1.0;

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> spec.might_of_the_frozen_wastes -> ok() )
        oh_attack = new death_strike_offhand_t( p );
    }
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.deathbringer -> check() )
      return;

    death_knight_melee_attack_t::consume_resource();
  }

  virtual double cost() const
  {
    if ( p() -> buffs.deathbringer -> check() )
      return 0;

    return death_knight_melee_attack_t::cost();
  }

  virtual void execute()
  {
    death_knight_melee_attack_t::execute();

    if ( oh_attack )
      oh_attack -> execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_death_strike -> execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      trigger_t16_2pc_tank();

      heal -> schedule_execute();
    }

    if ( p() -> sets.has_set_bonus( DEATH_KNIGHT_BLOOD, T17, B2 ) && p() -> buffs.scent_of_blood -> check() )
    {
      timespan_t reduction = p() -> sets.set( DEATH_KNIGHT_BLOOD, T17, B2 ) -> effectN( 1 ).time_value() * p() -> buffs.scent_of_blood -> check();
      p() -> cooldown.vampiric_blood -> ready -= reduction;
    }

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
      rune_t& r = p() -> _runes.slot[ i ];
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

    base_multiplier *= 1.0 + p -> perk.improved_festering_strike -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( ! p() -> talent.necrotic_plague -> ok() )
      {
        td( s -> target ) -> dots_blood_plague -> extend_duration( timespan_t::from_seconds( data().effectN( 3 ).base_value() ), 0 );
        td( s -> target ) -> dots_frost_fever  -> extend_duration( timespan_t::from_seconds( data().effectN( 3 ).base_value() ), 0 );
      }
      else
        td( s -> target ) -> dots_necrotic_plague -> extend_duration( timespan_t::from_seconds( data().effectN( 3 ).base_value() ), 0 );
    }

    p() -> trigger_necrosis( s );
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
    base_multiplier *= 1.0 + p -> spec.threat_of_thassarian -> effectN( 3 ).percent();
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();

    rp_gain = 0; // Incorrectly set to 10 in the DBC
  }

  virtual double composite_crit() const
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
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();

    parse_options( NULL, options_str );

    weapon     = &( p -> main_hand_weapon );

    if ( p -> spec.threat_of_thassarian -> ok() && p -> off_hand_weapon.type != WEAPON_NONE )
    {
      base_multiplier *= 1.0 + p -> spec.threat_of_thassarian -> effectN( 3 ).percent();

      if ( p -> spec.might_of_the_frozen_wastes -> ok() )
        oh_attack = new frost_strike_offhand_t( p );
    }
  }

  virtual double cost() const
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

      p() -> trigger_runic_empowerment( resource_consumed );
      p() -> trigger_blood_charge( resource_consumed );
      p() -> trigger_runic_corruption( resource_consumed );
      p() -> trigger_plaguebearer( execute_state );
    }
  }

  virtual double composite_crit() const
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
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
    death_knight_spell_t::execute();  // 10 RP gain happens in here

    if ( ! sim -> overrides.attack_power_multiplier )
      sim -> auras.attack_power_multiplier -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

// Howling Blast ============================================================

struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> find_class_spell( "Howling Blast" ) )
  {
    parse_options( NULL, options_str );

    aoe                 = -1;
    base_aoe_multiplier = data().effectN( 1 ).percent();
    //attack_power_mod.direct    = 1.207;

    assert( p -> active_spells.frost_fever );
  }

  virtual double action_multiplier() const
  {
    double m = death_knight_spell_t::action_multiplier();

    if ( p() -> buffs.rime -> check() && p() -> perk.empowered_obliterate -> ok() )
      m += p() -> perk.empowered_obliterate -> effectN( 1 ).percent();

    return m;
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.rime -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const
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
      p() -> apply_diseases( s, DISEASE_FROST_FEVER );
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

    attack_power_mod.direct = 0.319;

    if ( p -> spec.reaping -> ok() )
      convert_runes = 1.0;

    assert( p -> active_spells.frost_fever );
  }

  virtual double action_multiplier() const
  {
    double m = death_knight_spell_t::action_multiplier();

    if ( p() -> buffs.rime -> check() && p() -> perk.empowered_obliterate -> ok() )
      m += p() -> perk.empowered_obliterate -> effectN( 1 ).percent();
    
    return m;
  }

  virtual void consume_resource()
  {
    // We only consume resources when rime is not up
    if ( p() -> buffs.rime -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const
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
      p() -> apply_diseases( s, DISEASE_FROST_FEVER );
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

// Obliterate ===============================================================

struct obliterate_offhand_t : public death_knight_melee_attack_t
{
  obliterate_offhand_t( death_knight_t* p ) :
    death_knight_melee_attack_t( "obliterate_offhand", p, p -> find_spell( 66198 ) )
  {
    background       = true;
    weapon           = &( p -> off_hand_weapon );
    special          = true;
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();
  }

  virtual double composite_crit() const
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
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
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> spec.might_of_the_frozen_wastes -> ok() )
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
      if ( rng().roll( p() -> spec.rime -> proc_chance() ) )
      {
        // T13 2pc gives 2 stacks of Rime, otherwise we can only ever have one
        // Ensure that if we have 1 that we only refresh, not add another stack
        int new_stacks = 1;
        if ( rng().roll( p() -> sets.set( SET_MELEE, T13, B2 ) -> effectN( 2 ).percent() ) )
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

  virtual double composite_crit() const
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
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
      p() -> apply_diseases( execute_state, DISEASE_BLOOD_PLAGUE | DISEASE_FROST_FEVER );

      p() -> trigger_runic_empowerment( resource_consumed );
      p() -> trigger_runic_corruption( resource_consumed );
      p() -> trigger_blood_charge( resource_consumed );
    }

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_outbreak -> execute();
  }
};

// Blood Boil ==============================================================


struct blood_boil_spread_t : public death_knight_spell_t
{
  dot_t *bp, *ff, *np;

  blood_boil_spread_t( death_knight_t* p ) :
    death_knight_spell_t( "blood_boil_spread", p, spell_data_t::nil() ),
    bp( 0 ), ff( 0 ), np( 0 )
  {
    harmful = may_miss = callbacks = may_crit = may_dodge = may_parry = may_glance = false;
    dual = quiet = background = proc = true;
    aoe = -1;
  }

  void select_longest_duration_disease( dot_t*& bp, dot_t*& ff, dot_t*& np ) const
  {
    bp = ff = np = 0;

    for ( size_t i = 0, end = target_list().size(); i < end; i++ )
    {
      player_t* actor = target_list()[ i ];
      death_knight_td_t* tdata = td( actor );

      if ( tdata -> dots_blood_plague -> is_ticking() && ( ! bp || tdata -> dots_blood_plague -> remains() > bp -> remains() ) )
        bp = tdata -> dots_blood_plague;

      if ( tdata -> dots_frost_fever -> is_ticking() && ( ! ff || tdata -> dots_frost_fever -> remains() > ff -> remains() ) )
        ff = tdata -> dots_frost_fever;

      if ( tdata -> dots_necrotic_plague -> is_ticking() && ( ! np || tdata -> dots_necrotic_plague -> remains() > np -> remains() ) )
        np = tdata -> dots_necrotic_plague;
    }

    if ( sim -> debug )
    {
      if ( bp )
        sim -> out_debug.printf( "Player %s longest duration Blood Plague on %s (%.3f seconds left)",
            player -> name(), bp -> target -> name(), bp -> target -> name(), bp -> remains().total_seconds() );

      if ( ff )
        sim -> out_debug.printf( "Player %s longest duration Frost Fever on %s (%.3f seconds left)",
            player -> name(), ff -> target -> name(), ff -> remains().total_seconds() );

      if ( np )
        sim -> out_debug.printf( "Player %s longest duration Necrotic Plague on %s (%.3f seconds left)",
            player -> name(), np -> target -> name(), np -> remains().total_seconds() );
    }
  }

  void execute()
  {
    // Select suitable diseases, with and without Scent of Blood. With Scent of
    // Blood, it is used to decide if diseases are going to be refreshed,
    // without, it's used to determine which available disease type is spread
    // to other targets.
    select_longest_duration_disease( bp, ff, np );

    death_knight_spell_t::execute();
  }

  void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    // Scent of Blood adds a NP stack to each target that got hit by the
    // spread, or simply refreshes BP/FF, if there was a disease type to spread
    // in the first place
    if ( p() -> spec.scent_of_blood -> ok() )
    {
      // The scent-of-blood based logic ignores main target(?)
      if ( target == s -> target )
        return;

      if ( bp )
      {
        p() -> active_spells.blood_plague -> target = s -> target;
        p() -> active_spells.blood_plague -> execute();
      }

      if ( ff )
      {
        p() -> active_spells.frost_fever -> target = s -> target;
        p() -> active_spells.frost_fever -> execute();
      }

      if ( np )
      {
        p() -> active_spells.necrotic_plague -> target = s -> target;
        p() -> active_spells.necrotic_plague -> execute();
      }
    }
    // No Scent of Blood -> copy the longest duration BP/FF/NP to all targets.
    // The dots have (individually) been selected earlier (in execute()).
    else
    {
      death_knight_td_t* tdata = td( s -> target );

      // Spread Blood Plague
      if ( bp && bp -> target != s -> target )
      {
        if ( sim -> debug )
          sim -> out_debug.printf( "Player %s spreading Blood Plague from %s to %s",
              player -> name(), bp -> state -> target -> name(), s -> target -> name() );
        if ( tdata -> dots_blood_plague -> is_ticking() )
          tdata -> dots_blood_plague -> cancel();
        bp -> copy( s -> target, DOT_COPY_CLONE );
      }

      // Spread Frost Fever
      if ( ff && ff -> target != s -> target )
      {
        if ( sim -> debug )
          sim -> out_debug.printf( "Player %s spreading Frost Fever from %s to %s",
              player -> name(), ff -> state -> target -> name(), s -> target -> name() );
        if ( tdata -> dots_frost_fever -> is_ticking() )
          tdata -> dots_frost_fever -> cancel();
        ff -> copy( s -> target, DOT_COPY_CLONE );
      }

      // Spread Necrotic Plague
      if ( np && np -> target != s -> target )
      {
        if ( sim -> debug )
          sim -> out_debug.printf( "Player %s spreading Necrotic Plague from %s to %s",
              player -> name(), np -> state -> target -> name(), s -> target -> name() );
        if ( tdata -> dots_necrotic_plague -> is_ticking() )
        {
          tdata -> dots_necrotic_plague -> cancel();
          tdata -> debuffs_necrotic_plague -> expire();
        }

        np -> copy( s -> target, DOT_COPY_CLONE );
        death_knight_td_t* tdata2 = td( np -> target );

        int orig_stacks = tdata2 -> debuffs_necrotic_plague -> check();
        tdata -> debuffs_necrotic_plague -> trigger( orig_stacks );
      }
    }
  }
};

struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_spread_t* spread;

  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> find_class_spell( "Blood Boil" ) ),
    spread( new blood_boil_spread_t( p ) )
  {
    parse_options( NULL, options_str );

    if ( p -> spec.reaping -> ok() )
      convert_runes = 1.0;

    base_multiplier *= 1.0 + p -> spec.crimson_scourge -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + p -> perk.improved_pestilence -> effectN( 1 ).percent();

    aoe = -1;
  }

  virtual void consume_resource()
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const
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

    if ( result_is_hit( execute_state -> result ) )
    {
      // Just use "main" target for the spread selection, spread logic is fully
      // implemented in blood_boil_spread_t
      spread -> target = target;
      spread -> schedule_execute();
    }
  }

  void impact( action_state_t* state )
  {
    death_knight_spell_t::impact( state );

    p() -> trigger_necrosis( state );
  }

  virtual bool ready()
  {
    if ( ! death_knight_spell_t::ready() )
      return false;

    if ( ( ! p() -> in_combat && ! harmful ) || p() -> buffs.crimson_scourge -> check() )
      return group_runes( p(), 0, 0, 0, 0, use );
    else
      return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
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

    if ( p -> perk.empowered_pillar_of_frost -> ok() )
    {
      cost_blood = cost_frost = cost_unholy = cost_death = 0;
      rp_gain = 0;
    }
  }

  void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.pillar_of_frost -> trigger();

    if ( p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T17, B2 ) )
      p() -> resource_gain( RESOURCE_RUNIC_POWER, p() -> sets.set( DEATH_KNIGHT_FROST, T17, B2 ) -> effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER ), p() -> gains.t17_2pc_frost );

    p() -> buffs.frozen_runeblade -> trigger();
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
      if ( p -> spec.might_of_the_frozen_wastes -> ok() )
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
      unsigned diseases = DISEASE_BLOOD_PLAGUE;
      if ( p() -> spec.ebon_plaguebringer -> ok() )
        diseases |= DISEASE_FROST_FEVER;

      p() -> apply_diseases( s, diseases );
    }

    p() -> trigger_necrosis( s );
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

  virtual resource_e current_resource() const
  {
    return RESOURCE_RUNIC_POWER;
  }

  virtual double cost() const
  {
    // Presence changes consume all runic power
    return p() -> resources.current [ RESOURCE_RUNIC_POWER ] * ( 1.0 - p() -> glyph.shifting_presences -> effectN( 1 ).percent() );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    bool to_blood = false;

    switch ( p() -> active_presence )
    {
      case PRESENCE_BLOOD:  p() -> buffs.blood_presence  -> expire(); break;
      case PRESENCE_FROST:  p() -> buffs.frost_presence  -> expire(); break;
      case PRESENCE_UNHOLY: p() -> buffs.unholy_presence -> expire(); break;
    }

    p() -> active_presence = switch_to_presence;

    switch ( p() -> active_presence )
    {
      case PRESENCE_BLOOD:  p() -> buffs.blood_presence  -> trigger(); to_blood = true; break;
      case PRESENCE_FROST:  p() -> buffs.frost_presence  -> trigger(); break;
      case PRESENCE_UNHOLY: p() -> buffs.unholy_presence -> trigger(); break;
    }

    p() -> recalculate_resource_max( RESOURCE_HEALTH );
    if ( ! p() -> in_combat && to_blood )
      p() -> resource_gain( RESOURCE_HEALTH, p() -> resources.max[ RESOURCE_HEALTH ] - p() -> resources.current[ RESOURCE_HEALTH ] );
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

// Death's Advance ===============================================================

struct deaths_advance_t: public death_knight_spell_t
{
  deaths_advance_t( death_knight_t* p, const std::string& options_str ):
    death_knight_spell_t( "raise_dead", p, p -> talent.deaths_advance )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> buffs.deaths_advance -> trigger();
  }
};

// Raise Dead ===============================================================

struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", p, p -> find_specialization_spell( "Raise Dead" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    p() -> pets.ghoul_pet -> summon( timespan_t::zero() );
  }

  virtual bool ready()
  {
    if ( p() -> pets.ghoul_pet && ! p() -> pets.ghoul_pet -> is_sleeping() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Scourge Strike ===========================================================

struct scourge_strike_t : public death_knight_melee_attack_t
{
  struct scourge_strike_shadow_t : public death_knight_melee_attack_t
  {
    scourge_strike_shadow_t( death_knight_t* p ) :
      death_knight_melee_attack_t( "scourge_strike_shadow", p, p -> find_spell( 70890 ) )
    {
      may_miss = may_parry = may_dodge = false;
      special = proc = background = true;
      weapon = &( player -> main_hand_weapon );
      dual = true;
      school = SCHOOL_SHADOW;

      base_multiplier *= 1.0 + p -> perk.improved_scourge_strike -> effectN( 1 ).percent();
    }

    void impact( action_state_t* state )
    {
      death_knight_melee_attack_t::impact( state );

      p() -> trigger_necrosis( state );
    }
  };

  scourge_strike_shadow_t* scourge_strike_shadow;

  scourge_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "scourge_strike", p, p -> find_class_spell( "Scourge Strike" ) ),
    scourge_strike_shadow( new scourge_strike_shadow_t( p ) )
  {
    parse_options( NULL, options_str );

    special = true;
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + p -> perk.improved_scourge_strike -> effectN( 1 ).percent();

    // TODO-WOD: Do we need to inherit damage or is it a separate roll in WoD?
    add_child( scourge_strike_shadow );
  }

  void impact( action_state_t* state )
  {
    death_knight_melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      scourge_strike_shadow -> target = state -> target;
      scourge_strike_shadow -> schedule_execute();
    }

    p() -> trigger_necrosis( state );
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
    dot_duration = timespan_t::zero();
    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    timespan_t duration = data().effectN( 3 ).trigger() -> duration();
    duration += p() -> perk.empowered_gargoyle -> effectN( 1 ).time_value();

    p() -> pets.gargoyle -> summon( duration );
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

    p() -> apply_diseases( s, DISEASE_BLOOD_PLAGUE | DISEASE_FROST_FEVER );
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

    for ( int i = 0; i < 2; i++ )
    {
      int selected_rune = random_depleted_rune( p() );
      if ( selected_rune == -1 )
        return;

      rune_t* regen_rune = &( p() -> _runes.slot[ selected_rune ] );

      regen_rune -> fill_rune();
      regen_rune -> type |= RUNE_TYPE_DEATH;

      p() -> gains.plague_leech -> add( RESOURCE_RUNE, 1, 0 );

      if ( sim -> log ) sim -> out_log.printf( "%s regened rune %d", name(), selected_rune );
    }

    td( state -> target ) -> dots_frost_fever -> cancel();
    td( state -> target ) -> dots_blood_plague -> cancel();
    td( state -> target ) -> dots_necrotic_plague -> cancel();
  }

  bool ready()
  {
    if ( ( ! p() -> talent.necrotic_plague -> ok() && (
         ( ! td( target ) -> dots_frost_fever -> is_ticking() || ! td( target ) -> dots_blood_plague -> is_ticking() ) ) ) ||
         ( p() -> talent.necrotic_plague -> ok() && ! td( target ) -> dots_necrotic_plague -> is_ticking() ) )
      return false;

    bool rd = death_knight_spell_t::ready();

    rune_t& b = p() -> _runes.slot[ 0 ];
    if ( b.is_depleted() || b.paired_rune -> is_depleted() )
      return rd;

    rune_t& f = p() -> _runes.slot[ 2 ];
    if ( f.is_depleted() || f.paired_rune -> is_depleted() )
      return rd;

    rune_t& u = p() -> _runes.slot[ 4 ];
    if ( u.is_depleted() || u.paired_rune -> is_depleted() )
      return rd;

    return false;
  }
};

// Breath of Sindragosa =====================================================

struct breath_of_sindragosa_tick_t : public death_knight_spell_t
{
  action_t* parent;

  breath_of_sindragosa_tick_t( death_knight_t* p, action_t* parent ) :
    death_knight_spell_t( "breath_of_sindragosa_tick", p, p -> find_spell( 155166 ) ),
    parent( parent )
  {
    aoe        = -1;
    background = true;
    resource_current = RESOURCE_RUNIC_POWER;
  }

  void execute()
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> trigger_runic_empowerment( resource_consumed );
      p() -> trigger_blood_charge( resource_consumed );
      p() -> trigger_runic_corruption( resource_consumed );
      p() -> trigger_shadow_infusion( resource_consumed );
    }
  }

  virtual void impact( action_state_t* s )
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      td( target ) -> debuffs_mark_of_sindragosa -> trigger();
  }
};

struct breath_of_sindragosa_t : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "breath_of_sindragosa", p, p -> find_talent_spell( "Breath of Sindragosa" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_crit = hasted_ticks = callbacks = false;
    tick_zero = true;
    dot_duration = timespan_t::from_seconds( 100 );

    tick_action = new breath_of_sindragosa_tick_t( p, this );
    tick_action -> base_costs[ RESOURCE_RUNIC_POWER ] = base_costs[ RESOURCE_RUNIC_POWER ];
    base_costs[ RESOURCE_RUNIC_POWER ] = 0;

    school = tick_action -> school;
  }

  void execute()
  {
    dot_t* d = get_dot( target );

    if ( d -> is_ticking() )
      d -> cancel();
    else
    {
      p() -> active_spells.breath_of_sindragosa = this;
      death_knight_spell_t::execute();
    }
  }

  void last_tick( dot_t* dot )
  {
    death_knight_spell_t::last_tick( dot );
    p() -> active_spells.breath_of_sindragosa = 0;
  }

  void init()
  {
    death_knight_spell_t::init();

    snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA | STATE_MUL_PERSISTENT;
    update_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA;
  }
};

// Anti-magic Shell =========================================================

struct antimagic_shell_t : public death_knight_spell_t
{
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double damage;

  antimagic_shell_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "antimagic_shell", p, p -> find_class_spell( "Anti-Magic Shell" ) ),
    interval( 60 ), interval_stddev( 0.05 ), interval_stddev_opt( 0 ), damage( 0 )
  {
    harmful = false;
    base_dd_min = base_dd_max = 0;

    option_t options[] =
    {
      opt_float( "interval", interval ),
      opt_float( "interval_stddev", interval_stddev_opt ),
      opt_float( "damage", damage ),
      opt_null()
    };

    parse_options( options, options_str );

    // Allow as low as 15 second intervals, due to new glyph
    if ( interval < 15.0 )
    {
      sim -> errorf( "%s minimum interval for Anti-Magic Shell is 15 seconds.", player -> name() );
      interval = 15.0;
    }

    // Less than a second standard deviation is translated to a percent of
    // interval
    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;

    if ( damage > 0 )
      cooldown -> set_recharge_multiplier( 1.0 );
  }

  void execute()
  {
    if ( damage > 0 )
    {
      timespan_t new_cd = timespan_t::from_seconds( rng().gauss( interval, interval_stddev ) );
      if ( new_cd < timespan_t::from_seconds( 15.0 ) )
        new_cd = timespan_t::from_seconds( 15.0 );

      cooldown -> duration = new_cd;
    }

    death_knight_spell_t::execute();

    // If using the fake soaking, immediately grant the RP in one go
    if ( damage > 0 )
    {
      double absorbed = std::min( damage * data().effectN( 1 ).percent(),
                                  p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 2 ).percent() );

      double generated = absorbed / p() -> resources.max[ RESOURCE_HEALTH ];

      p() -> resource_gain( RESOURCE_RUNIC_POWER, util::round( generated * 100.0 ), p() -> gains.antimagic_shell, this );
    }
    else
      p() -> buffs.antimagic_shell -> trigger();
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

    cooldown -> duration = data().cooldown() * ( 1.0 + p -> glyph.icebound_fortitude -> effectN( 1 ).percent() );
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

struct rune_tap_t : public death_knight_spell_t
{
  cooldown_t* ability_cooldown;
  rune_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "rune_tap", p, p -> find_specialization_spell( "Rune Tap" ) )
  {
    parse_options( NULL, options_str );
    cooldown -> charges = 2;
    cooldown -> duration = timespan_t::from_seconds( 40 );
    cooldown -> duration += p -> perk.enhanced_rune_tap -> effectN( 1 ).time_value();
    ability_cooldown = p -> get_cooldown( "Rune Tap Ability Cooldown" );
    ability_cooldown -> duration = data().cooldown(); // Can only use it once per second.
    use_off_gcd = true;
  }

  void consume_resource()
  {
    if ( p() -> buffs.will_of_the_necropolis_rt -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  void execute()
  {
    death_knight_spell_t::execute();
    ability_cooldown -> start();

    p() -> buffs.rune_tap -> trigger();

    p() -> buffs.will_of_the_necropolis_rt -> expire();
  }

  double cost() const
  {
    if ( p() -> buffs.will_of_the_necropolis_rt -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  bool ready()
  {
    if ( p() -> buffs.will_of_the_necropolis_rt-> check() )
    {
      cost_blood = 0;
      bool r = death_knight_spell_t::ready();
      cost_blood  = 1;
      return r;
    }
    if ( ability_cooldown -> up() )
      return death_knight_spell_t::ready();

    return false;
  }
};

// Death Pact

// TODO-WOD: Healing absorb kludge

struct death_pact_t : public death_knight_heal_t
{
  death_pact_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_heal_t( "death_pact", p, p -> find_talent_spell( "Death Pact" ) )
  {
    may_crit = false;

    parse_options( NULL, options_str );
  }

  double base_da_min( const action_state_t* ) const
  { return p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent(); }

  double base_da_max( const action_state_t* ) const
  { return p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent(); }
};

// Expressions

struct disease_expr_t : public expr_t
{
  enum type_e { TYPE_NONE, TYPE_MIN, TYPE_MAX };

  type_e  type;
  expr_t* bp_expr;
  expr_t* ff_expr;
  expr_t* np_expr;

  double default_value;

  disease_expr_t( const action_t* a, const std::string& expression, type_e t ) :
    expr_t( "disease_expr" ), type( t ), bp_expr( 0 ), ff_expr( 0 ), np_expr( 0 ),
    default_value( 0 )
  {
    death_knight_t* p = debug_cast< death_knight_t* >( a -> player );
    if ( p -> talent.necrotic_plague -> ok() )
      np_expr = a -> target -> get_dot( "necrotic_plague", p ) -> create_expression( p -> active_spells.necrotic_plague, expression, true );
    else
    {
      bp_expr = a -> target -> get_dot( "blood_plague", p ) -> create_expression( p -> active_spells.blood_plague, expression, true );
      ff_expr = a -> target -> get_dot( "frost_fever", p ) -> create_expression( p -> active_spells.frost_fever, expression, true );
    }

    if ( type == TYPE_MIN )
      default_value = std::numeric_limits<double>::max();
    else if ( type == TYPE_MAX )
      default_value = std::numeric_limits<double>::min();
  }

  double evaluate()
  {
    double ret = default_value;

    if ( bp_expr )
    {
      double val = bp_expr -> eval();
      if ( type == TYPE_NONE && val != 0 )
        return val;
      else if ( type == TYPE_MIN && val < ret )
        ret = val;
      else if ( type == TYPE_MAX && val > ret )
        ret = val;
    }

    if ( ff_expr )
    {
      double val = ff_expr -> eval();
      if ( type == TYPE_NONE && val != 0 )
        return val;
      else if ( type == TYPE_MIN && val < ret )
        ret = val;
      else if ( type == TYPE_MAX && val > ret )
        ret = val;
    }

    if ( np_expr )
    {
      double val = np_expr -> eval();
      if ( type == TYPE_NONE && val != 0 )
        return val;
      else if ( type == TYPE_MIN && val < ret )
        ret = val;
      else if ( type == TYPE_MAX && val > ret )
        ret = val;
    }

    if ( ret == default_value )
      ret = 0;

    return ret;
  }

  ~disease_expr_t()
  {
    delete bp_expr;
    delete ff_expr;
    delete np_expr;
  }
};

template <class Base>
expr_t* death_knight_action_t<Base>::create_expression( const std::string& name_str )
{
  std::vector<std::string> split = util::string_split( name_str, "." );
  std::string name, expression;
  disease_expr_t::type_e type = disease_expr_t::TYPE_NONE;

  if ( split.size() == 3 && util::str_compare_ci( split[ 0 ], "dot" ) )
  {
    name = split[ 1 ];
    expression = split[2];
  }
  else if ( split.size() == 2 )
  {
    name = split[ 0 ];
    expression = split[ 1 ];
  }

  if ( util::str_in_str_ci( expression, "min_" ) )
  {
    type = disease_expr_t::TYPE_MIN;
    expression = expression.substr( 4 );
  }
  else if ( util::str_in_str_ci( expression, "max_" ) )
  {
    type = disease_expr_t::TYPE_MAX;
    expression = expression.substr( 4 );
  }

  if ( util::str_compare_ci( name, "disease" ) )
    return new disease_expr_t( this, expression, type );

  return Base::create_expression( name_str );
}

// Buffs ====================================================================

struct runic_corruption_buff_t : public buff_t
{
  core_event_t* regen_event;

  runic_corruption_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "runic_corruption", p -> find_spell( 51460 ) )
            .chance( p -> talent.runic_corruption -> ok() ).affects_regen( true ) ),
    regen_event( 0 )
  {  }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() )
  {
    buff_t::execute( stacks, value, duration );
  }

  void expire_override()
  {
    buff_t::expire_override();
  }
};

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

struct frozen_runeblade_buff_t : public buff_t
{
  int stack_count;

  frozen_runeblade_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "frozen_runeblade", p -> sets.set( DEATH_KNIGHT_FROST, T17, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
                           .chance( p -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T17, B4 ) ).refresh_behavior( BUFF_REFRESH_DISABLED ) ),
    stack_count( 0 )
  { }

  void execute( int stacks, double value, timespan_t duration )
  {
    buff_t::execute( stacks, value, duration );

    stack_count = current_stack;
  }

  void expire_override()
  {
    death_knight_t* p = debug_cast< death_knight_t* >( player );
    p -> active_spells.frozen_runeblade -> dot_duration = stack_count * p -> active_spells.frozen_runeblade -> data().effectN( 1 ).period();
    p -> active_spells.frozen_runeblade -> schedule_execute();

    buff_t::expire_override();

    stack_count = 0;
  }

  void reset()
  {
    buff_t::reset();
    stack_count = 0;
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
  if ( name == "deaths_advance"           ) return new deaths_advance_t           ( this, options_str );
  if ( name == "unholy_presence"          ) return new unholy_presence_t          ( this, options_str );
  if ( name == "frost_presence"           ) return new frost_presence_t           ( this, options_str );
  if ( name == "soul_reaper"              ) return new soul_reaper_t              ( this, options_str );
  if ( name == "plague_leech"             ) return new plague_leech_t             ( this, options_str );
  if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );

  // Blood Actions
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
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

  // Unholy Actions
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "plague_strike"            ) return new plague_strike_t            ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );

  // Talents
  if ( name == "unholy_blight"            ) return new unholy_blight_t            ( this, options_str );
  if ( name == "death_siphon"             ) return new death_siphon_t             ( this, options_str );
  if ( name == "death_pact"               ) return new death_pact_t               ( this, options_str );
  if ( name == "breath_of_sindragosa"     ) return new breath_of_sindragosa_t     ( this, options_str );
  if ( name == "defile"                   ) return new defile_t                   ( this, options_str );
  if ( name == "conversion"               ) return new conversion_t               ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

void parse_rune_type( const std::string& rune_str, bool& include_death, rune_type& type )
{
  if ( util::str_compare_ci( rune_str, "blood" ) )
    type = RUNE_TYPE_BLOOD;
  else if ( util::str_compare_ci( rune_str, "frost" ) )
    type = RUNE_TYPE_FROST;
  else if ( util::str_compare_ci( rune_str, "unholy" ) )
    type = RUNE_TYPE_UNHOLY;
  else if ( util::str_compare_ci( rune_str, "death" ) )
    type = RUNE_TYPE_DEATH;

  if ( rune_str[ 0 ] == 'B' || rune_str[ 0 ] == 'F' || rune_str[ 0 ] == 'U' )
    include_death = true;
}

void death_knight_t::trigger_runic_corruption( double rpcost )
{
  if ( ! rng().roll( talent.runic_corruption -> effectN( 2 ).percent() * rpcost / 100.0 ) )
    return;

  timespan_t duration = timespan_t::from_seconds( 3.0 * cache.attack_haste() );
  if ( buffs.runic_corruption -> check() == 0 )
    buffs.runic_corruption -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  else
    buffs.runic_corruption -> extend_duration( this, duration );

  buffs.tier13_4pc_melee -> trigger( 1, buff_t::DEFAULT_VALUE(), sets.set( SET_MELEE, T13, B4 ) -> effectN( 2 ).percent() );
}

expr_t* death_knight_t::create_expression( action_t* a, const std::string& name_str )
{
  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.size() == 2 && util::str_compare_ci( splits[ 1 ], "ready_in" ) )
  {
    struct ability_ready_expr_t : public expr_t
    {
      death_knight_melee_attack_t* action;

      ability_ready_expr_t( action_t* a ) :
        expr_t( "ability_ready_expr" ),
        action( debug_cast<death_knight_melee_attack_t*>( a ) )
      { }

      double evaluate()
      {
        return ready_in( debug_cast<death_knight_t*>( action -> player ),
                         action -> cost_blood,
                         action -> cost_frost,
                         action -> cost_unholy );
      }
    };

    action_t* a = find_action( splits[ 0 ] );
    if ( a )
      return new ability_ready_expr_t( a );
  }

  if ( util::str_compare_ci( splits[ 0 ], "rune" ) )
  {
    rune_type rt = RUNE_TYPE_NONE;
    bool include_death = false; // whether to include death runes
    parse_rune_type( splits[ 1 ], include_death, rt );

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
    bool include_death = false;
    parse_rune_type( splits[ 0 ], include_death, rt );

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
    bool include_death = false;
    parse_rune_type( splits[ 0 ], include_death, rt );

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

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    pets.dancing_rune_weapon = new pets::dancing_rune_weapon_pet_t( sim, this );

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

  return 0;
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_melee_haste() const
{
  double haste = player_t::composite_melee_haste();

  haste *= 1.0 / ( 1.0 + buffs.unholy_presence -> value() );

  haste *= 1.0 / ( 1.0 + spec.veteran_of_the_third_war -> effectN( 6 ).percent() );

  haste *= 1.0 / ( 1.0 + spec.icy_talons -> effectN( 2 ).percent() );

  return haste;
}

// death_knight_t::composite_spell_haste() ==================================

double death_knight_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  haste *= 1.0 / ( 1.0 + buffs.unholy_presence -> value() );

  haste *= 1.0 / ( 1.0 + spec.veteran_of_the_third_war -> effectN( 6 ).percent() );

  haste *= 1.0 / ( 1.0 + spec.icy_talons -> effectN( 2 ).percent() );

  return haste;
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng()
{
  player_t::init_rng();

  const spell_data_t* spell = find_spell( 138343 );
  t15_2pc_melee.set_frequency( spell -> real_ppm() );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attribute_multiplier[ ATTR_STRENGTH ] *= 1.0 + spec.unholy_might -> effectN( 1 ).percent();

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility = 0.0;

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;

  base_gcd = timespan_t::from_seconds( 1.0 );

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base.dodge += 0.030 + spec.veteran_of_the_third_war -> effectN( 2 ).percent();

}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Specialization

  // Generic
  spec.plate_specialization       = find_specialization_spell( "Plate Specialization" );
  spec.multistrike_attunement     = find_specialization_spell( "Multistrike Attunement" );

  // Blood
  spec.bladed_armor               = find_specialization_spell( "Bladed Armor" );
  spec.blood_rites                = find_specialization_spell( "Blood Rites" );
  spec.veteran_of_the_third_war   = find_specialization_spell( "Veteran of the Third War" );
  spec.scent_of_blood             = find_specialization_spell( "Scent of Blood" );
  spec.improved_blood_presence    = find_specialization_spell( "Improved Blood Presence" );
  spec.crimson_scourge            = find_specialization_spell( "Crimson Scourge" );
  spec.sanguine_fortitude         = find_specialization_spell( "Sanguine Fortitude" );
  spec.will_of_the_necropolis     = find_specialization_spell( "Will of the Necropolis" );
  spec.resolve                    = find_specialization_spell( "Resolve" );
  spec.riposte                    = find_specialization_spell( "Riposte" );

  // Frost
  spec.blood_of_the_north         = find_specialization_spell( "Blood of the North" );
  spec.icy_talons                 = find_specialization_spell( "Icy Talons" );
  spec.improved_frost_presence    = find_specialization_spell( "Improved Frost Presence" );
  spec.rime                       = find_specialization_spell( "Rime" );
  spec.might_of_the_frozen_wastes = find_specialization_spell( "Might of the Frozen Wastes" );
  spec.killing_machine            = find_specialization_spell( "Killing Machine" );

  // Unholy
  spec.master_of_ghouls           = find_specialization_spell( "Master of Ghouls" );
  spec.necrosis                   = find_specialization_spell( "Necrosis" );
  spec.reaping                    = find_specialization_spell( "Reaping" );
  spec.unholy_might               = find_specialization_spell( "Unholy Might" );
  spec.shadow_infusion            = find_specialization_spell( "Shadow Infusion" );
  spec.sudden_doom                = find_specialization_spell( "Sudden Doom" );
  spec.ebon_plaguebringer         = find_specialization_spell( "Ebon Plaguebringer" );
  spec.improved_unholy_presence   = find_specialization_spell( "Improved Unholy Presence" );
  spec.threat_of_thassarian       = find_specialization_spell( "Threat of Thassarian" );

  mastery.blood_shield            = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart            = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade              = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Talents
  talent.plaguebearer             = find_talent_spell( "Plaguebearer" );
  talent.plague_leech             = find_talent_spell( "Plague Leech" );
  talent.unholy_blight            = find_talent_spell( "Unholy Blight" );

  talent.deaths_advance           = find_talent_spell( "Death's Advance" );
  spell.deaths_advance            = find_spell( 124285 ); // Passive movement speed is in a completely unlinked spell id.

  talent.conversion               = find_talent_spell( "Conversion" );

  talent.blood_tap                = find_talent_spell( "Blood Tap" );
  talent.runic_empowerment        = find_talent_spell( "Runic Empowerment" );
  talent.runic_corruption         = find_talent_spell( "Runic Corruption" );

  talent.death_siphon             = find_talent_spell( "Death Siphon" );

  talent.necrotic_plague          = find_talent_spell( "Necrotic Plague" );
  talent.breath_of_sindragosa     = find_talent_spell( "Breath of Sindragosa" );
  talent.defile                   = find_talent_spell( "Defile" );

  // Glyphs
  glyph.chains_of_ice             = find_glyph_spell( "Glyph of Chains of Ice" );
  glyph.dancing_rune_weapon       = find_glyph_spell( "Glyph of Dancing Rune Weapon" );
  glyph.enduring_infection        = find_glyph_spell( "Glyph of Enduring Infection" );
  glyph.festering_blood           = find_glyph_spell( "Glyph of Festering Blood" );
  glyph.icebound_fortitude        = find_glyph_spell( "Glyph of Icebound Fortitude" );
  glyph.outbreak                  = find_glyph_spell( "Glyph of Outbreak" );
  glyph.regenerative_magic        = find_glyph_spell( "Glyph of Regenerative Magic" );
  glyph.shifting_presences        = find_glyph_spell( "Glyph of Shifting Presences" );
  glyph.vampiric_blood            = find_glyph_spell( "Glyph of Vampiric Blood" );

  // Generic spells
  spell.antimagic_shell           = find_class_spell( "Anti-Magic Shell" );
  spell.t15_4pc_tank              = find_spell( 138214 );
  spell.t16_4pc_melee             = find_spell( 144909 );
  spell.blood_rites               = find_spell( 163948 );
  spell.necrotic_plague_energize  = find_spell( 155165 );

  // Perks

  // Shared
  perk.improved_diseases               = find_perk_spell( "Improved Diseases" );
  perk.enhanced_death_strike           = find_perk_spell( "Enhanced Death Strike" );

  // Blood
  perk.enhanced_bone_shield            = find_perk_spell( "Enhanced Bone Shield" );
  perk.enhanced_death_coil_blood       = find_perk_spell( "Enhanced Death Coil", DEATH_KNIGHT_BLOOD );
  perk.enhanced_rune_tap               = find_perk_spell( "Enhanced Rune Tap" );
  perk.enhanced_will_of_the_necropolis = find_perk_spell( "Enhanced Will of the Necropolis" );
  perk.improved_death_strike           = find_perk_spell( "Improved Death Strike" );
  perk.improved_pestilence             = find_perk_spell( "Improved Pestilence" );

  // Frost
  perk.empowered_icebound_fortitude    = find_perk_spell( "Empowered Icebound Fortitude" );
  perk.empowered_pillar_of_frost       = find_perk_spell( "Empowered Pillar of Frost" );
  perk.empowered_obliterate            = find_perk_spell( "Empowered Obliterate" );
  perk.improved_runeforges             = find_perk_spell( "Improved Runeforges" );

  // Unholy
  perk.empowered_gargoyle             = find_perk_spell( "Empowered Gargoyle" );
  perk.enhanced_dark_transformation   = find_perk_spell( "Enhanced Dark Transformation" );
  perk.enhanced_fallen_crusader       = find_perk_spell( "Enhanced Fallen Crusader" );
  perk.improved_festering_strike      = find_perk_spell( "Improved Festering Strike" );
  perk.improved_scourge_strike        = find_perk_spell( "Improved Scourge Strike" );
  perk.improved_death_coil            = find_perk_spell( "Improved Death Coil" );
  perk.improved_soul_reaper            = find_perk_spell( "Improved Soul Reaper" );

  // Active Spells
  active_spells.blood_plague = new blood_plague_t( this );
  active_spells.frost_fever = new frost_fever_t( this );
  active_spells.necrotic_plague = new necrotic_plague_t( this );

  if ( spec.necrosis -> ok() )
    active_spells.necrosis = new necrosis_t( this );

  if ( sets.has_set_bonus( SET_MELEE, T16, B4 ) && specialization() == DEATH_KNIGHT_FROST )
  {
    struct frozen_power_t : public melee_attack_t
    {
      frozen_power_t( death_knight_t* p ) :
        melee_attack_t( "frozen_power", p, p -> spell.t16_4pc_melee -> effectN( 1 ).trigger() )
      {
        special = background = callbacks = proc = may_crit = true;
        if ( p -> main_hand_weapon.group() != WEAPON_2H )
          base_multiplier = 0.5;
      }
    };

    active_spells.frozen_power = new frozen_power_t( this );
  }

  if ( sets.has_set_bonus( DEATH_KNIGHT_FROST, T17, B4 ) )
    active_spells.frozen_runeblade = new frozen_runeblade_driver_t( this );

}

// death_knight_t::default_apl_blood ========================================

// FIXME: Add talent support

void death_knight_t::default_apl_blood()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  std::string srpct     = sets.has_set_bonus( SET_MELEE, T15, B4 ) ? "45" : "35";
  std::string flask_str = "flask,type=";
  std::string food_str  = "food,type=";
  std::string potion_str = "potion,name=";

  if ( primary_role() == ROLE_TANK )
  {
    potion_str += ( level >= 90 ) ? "draenic_armor_potion" : ( level >= 85 ) ? "mountains_potion" : "earthen_potion";
    flask_str += ( level >= 90 ) ? "greater_draenic_stamina_flask" : ( level >= 85 ) ? "earth" : "steelskin";
    food_str += ( level >= 90 ) ? "talador_surf_and_turf" : ( level >= 85 ) ? "chun_tian_spring_rolls" : "beer_basted_crocolisk";
  }
  else
  {
    potion_str += ( level >= 90 ) ? "draenic_strength_potion" : ( level >= 85 ) ? "mogu_power" : "golemblood";
    flask_str += ( level >= 90 ) ? "greater_draenic_strength_flask" : ( level >= 85 ) ? "winters_bite" : "titanic_strength";
    food_str += ( level >= 90 ) ? "calamari_crepes" : ( level >= 85 ) ? "black_pepper_ribs_and_shrimp" : "beer_basted_crocolisk";
  }

  // Precombat actions

  if ( sim -> allow_flasks && level >= 80 )
    precombat -> add_action( flask_str );

  if ( sim -> allow_food && level >= 80 )
    precombat -> add_action( food_str );

  precombat -> add_action( this, "Blood Presence" );
  precombat -> add_action( this, "Horn of Winter" );
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && level >= 80 )
    precombat -> add_action( potion_str );

  precombat -> add_action( this, "Bone Shield" );

  // Action list proper

  def -> add_action( "auto_attack" );

  for ( size_t i = 0; i < get_profession_actions().size(); i++ )
    def -> add_action( get_profession_actions()[ i ], "if=time>10" );

  for ( size_t i = 0; i < get_racial_actions().size(); i++ )
    def -> add_action( get_racial_actions()[ i ], "if=time>10" );

  for ( size_t i = 0; i < get_item_actions().size(); i++ )
    def -> add_action( get_item_actions()[ i ], "if=time>10" );

  if ( primary_role() == ROLE_TANK )
  {
    if ( sim -> allow_potions && level >= 80 )
      def -> add_action( potion_str + ",if=buff.potion.down&buff.blood_shield.down&!unholy&!frost" );

    def -> add_action( this, "Anti-Magic Shell" );
    def -> add_talent( this, "Conversion", "if=!buff.conversion.up&runic_power>50&health.pct<90" );
    def -> add_action( this, "Death Strike", "if=incoming_damage_5s>=health.max*0.65" );
    def -> add_action( this, "Army of the Dead", "if=buff.bone_shield.down&buff.dancing_rune_weapon.down&buff.icebound_fortitude.down&buff.vampiric_blood.down" );
    def -> add_action( this, "Bone Shield", "if=buff.army_of_the_dead.down&buff.bone_shield.down&buff.dancing_rune_weapon.down&buff.icebound_fortitude.down&buff.vampiric_blood.down" );
    def -> add_action( this, "Vampiric Blood", "if=health.pct<50" );
    def -> add_action( this, "Icebound Fortitude", "if=health.pct<30&buff.army_of_the_dead.down&buff.dancing_rune_weapon.down&buff.bone_shield.down&buff.vampiric_blood.down" );
    def -> add_action( this, "Rune Tap", "if=health.pct<50&buff.army_of_the_dead.down&buff.dancing_rune_weapon.down&buff.bone_shield.down&buff.vampiric_blood.down&buff.icebound_fortitude.down" );
    def -> add_action( this, "Dancing Rune Weapon", "if=health.pct<80&buff.army_of_the_dead.down&buff.icebound_fortitude.down&buff.bone_shield.down&buff.vampiric_blood.down" );
    def -> add_talent( this, "Death Pact", "if=health.pct<50" );
    def -> add_action( this, "Outbreak", "if=(!talent.necrotic_plague.enabled&!disease.min_remains<8)|!disease.ticking" );
    def -> add_action( this, "Death Coil", "if=runic_power>90" );
    def -> add_action( this, "Plague Strike", "if=(!talent.necrotic_plague.enabled&!dot.blood_plague.ticking)|(talent.necrotic_plague.enabled&!dot.necrotic_plague.ticking)" );
    def -> add_action( this, "Icy Touch", "if=(!talent.necrotic_plague.enabled&!dot.frost_fever.ticking)|(talent.necrotic_plague.enabled&!dot.necrotic_plague.ticking)" );
    def -> add_talent( this, "Defile" );
    def -> add_action( this, "Death Strike", "if=(unholy=2|frost=2)" );
    def -> add_action( this, "Death Coil", "if=runic_power>70" );
    def -> add_action( this, "Soul Reaper", "if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + srpct + "&blood>=1" );
    def -> add_action( this, "Blood Boil", "if=blood=2" );
    def -> add_talent( this, "Blood Tap" );
    def -> add_action( this, "Blood Boil" );
    def -> add_action( this, "Death Coil" );
    def -> add_action( this, "Empower Rune Weapon", "if=!blood&!unholy&!frost" );
  }
  else
  {
    precombat -> add_action( this, "Army of the Dead" );

    if ( sim -> allow_potions && level >= 80 )
      def -> add_action( potion_str + ",if=buff.bloodlust.react|target.time_to_die<=60" );

    def -> add_action( this, "Anti-Magic Shell" );
    def -> add_action( this, "Bone Shield", "if=buff.bone_shield.down&buff.dancing_rune_weapon.down&buff.icebound_fortitude.down&buff.vampiric_blood.down" );
    def -> add_action( this, "Vampiric Blood", "if=health.pct<50" );
    def -> add_action( this, "Icebound Fortitude", "if=health.pct<30&buff.dancing_rune_weapon.down&buff.bone_shield.down&buff.vampiric_blood.down" );
    def -> add_action( this, "Rune Tap", "if=health.pct<90" );
    def -> add_action( this, "Dancing Rune Weapon" );
    def -> add_action( this, "Death Coil", "if=runic_power>80" );
    def -> add_talent( this, "Death Pact", "if=health.pct<50" );
    def -> add_action( this, "Outbreak", "if=buff.dancing_rune_weapon.up" );
    def -> add_action( this, "Death Strike", "if=unholy=2|frost=2" );
    def -> add_action( this, "Plague Strike", "if=!disease.ticking" );
    def -> add_action( this, "Icy Touch", "if=!disease.ticking" );
    def -> add_action( this, "Soul Reaper", "if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + srpct );
    def -> add_action( this, "Death Strike" );
    def -> add_talent( this, "Blood Tap" );
    def -> add_action( this, "Empower Rune Weapon", "if=!blood&!unholy&!frost" );
  }

  // FIMXME Needs support for T5 other than RC. Severely reduces TMI right now.
  // if ( talent.blood_tap -> ok() )
  // action_list_str += "/blood_tap,if=(unholy=0&frost>=1)|(unholy>=1&frost=0)|(death=1)";
}

// death_knight_t::init_actions =============================================

void death_knight_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    default_apl_blood();
    use_default_action_list = true;
    player_t::init_action_list();
    return;
  }


  int tree = specialization();

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );
  action_priority_list_t* st = get_action_priority_list( "single_target" );
  std::string soul_reaper_pct = ( perk.improved_soul_reaper -> ok() || sets.has_set_bonus( SET_MELEE, T15, B4 ) ) ? "45" : "35";
  std::string flask_str = "flask,type=";
  std::string food_str = "food,type=";
  std::string potion_str = "potion,name=";
  potion_str += ( level >= 90 ) ? "draenic_strength" : ( ( level >= 85 ) ? "mogu_power" : "golemblood" );
  food_str += ( level >= 85 ) ? "black_pepper_ribs_and_shrimp" : "beer_basted_crocolisk";

  if ( tree == DEATH_KNIGHT_UNHOLY )
    flask_str += ( level >= 90 ) ? "greater_draenic_strength_flask" : ( ( level >= 85 ) ? "winters_bite" : "titanic_strength" );
  else
    flask_str += ( level >= 90 ) ? "greater_draenic_strength_flask" : ( ( level >= 85 ) ? "winters_bite" : "titanic_strength" );

  if ( tree == DEATH_KNIGHT_FROST || tree == DEATH_KNIGHT_UNHOLY )

    // Precombat actions

    if ( sim -> allow_flasks && level >= 80 )
      precombat -> add_action( flask_str );

  if ( sim -> allow_food && level >= 80 )
    precombat -> add_action( food_str );


  precombat -> add_action( this, "Horn of Winter" );


  if ( specialization() == DEATH_KNIGHT_FROST )
    precombat -> add_action( this, "Frost Presence" );
  else
    precombat -> add_action( this, "Unholy Presence" );




  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  precombat -> add_action( this, "Army of the Dead" );

  if ( sim -> allow_potions && level >= 80 )
    precombat -> add_action( potion_str );

  for ( size_t i = 0; i < get_profession_actions().size(); i++ )
    precombat -> add_action( get_profession_actions()[i] );

  def -> add_action( "auto_attack" );
  def -> add_talent( this, "Death's Advance", "if=movement.remains>2" );
  def -> add_action( this, "Anti-Magic Shell", "damage=100000" );


  switch ( specialization() )
  {
  case DEATH_KNIGHT_FROST:
  {
    // Frost specific precombat stuff
    precombat -> add_action( this, "Pillar of Frost" );

    def -> add_action( this, "Pillar of Frost" );

    if ( sim -> allow_potions && level >= 80 )
      def -> add_action( potion_str + ",if=target.time_to_die<=30|(target.time_to_die<=60&buff.pillar_of_frost.up)" );

    def -> add_action( this, "Empower Rune Weapon", "if=target.time_to_die<=60&buff.potion.up" );

    for ( size_t i = 0; i < get_profession_actions().size(); i++ )
      def -> add_action( get_profession_actions()[i] );

    for ( size_t i = 0; i < get_racial_actions().size(); i++ )
      def -> add_action( get_racial_actions()[i] );

    for ( size_t i = 0; i < get_item_actions().size(); i++ )
      def -> add_action( get_item_actions()[i] );

      //decide between single_target and aoe rotation
      def -> add_action( "call_action_list,name=aoe,if=active_enemies>=3" );
      def -> add_action( "call_action_list,name=single_target,if=active_enemies<3" );

    //decide between single_target and aoe rotation
    def -> add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
    def -> add_action( "run_action_list,name=single_target,if=active_enemies<3" );

    if ( main_hand_weapon.group() == WEAPON_2H )
    {
      // Diseases for free
      st -> add_talent( this, "Plague Leech", "if=disease.min_remains<1" );
      st -> add_action( this, "Outbreak", "if=!disease.min_ticking" );
      st -> add_talent( this, "Unholy Blight", "if=!disease.min_ticking" );

      // Soul Reaper
      st -> add_action( this, "Soul Reaper", "if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct );
      st -> add_talent( this, "Blood Tap", "if=(target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct + "&cooldown.soul_reaper.remains=0)" );

      // Diseases for runes
      st -> add_action( this, "Howling Blast", "if=!talent.necrotic_plague.enabled&!dot.frost_fever.ticking" );
      st -> add_action( this, "Howling Blast", "if=talent.necrotic_plague.enabled&!dot.necrotic_plague.ticking" );
      st -> add_action( this, "Plague Strike", "if=!talent.necrotic_plague.enabled&!dot.blood_plague.ticking" );

      // Rime
      st -> add_action( this, "Howling Blast", "if=buff.rime.react" );

      // Killing Machine
      st -> add_action( this, "Obliterate", "if=buff.killing_machine.react" );
      st -> add_talent( this, "Blood Tap", "if=buff.killing_machine.react" );

      // Don't waste Runic Power
      st -> add_talent( this, "Blood Tap", "if=buff.blood_charge.stack>10&runic_power>76" );
      st -> add_action( this, "Frost Strike", "if=runic_power>76" );

      // Keep runes on cooldown
      st -> add_action( this, "Obliterate", "if=blood=2|frost=2|unholy=2" );

      // Refresh diseases
      st -> add_talent( this, "Plague Leech", "if=disease.min_remains<3" );
      st -> add_action( this, "Outbreak", "if=disease.min_remains<3" );
      st -> add_talent( this, "Unholy Blight", "if=disease.min_remains<3" );

      // Regenerate resources
      st -> add_action( this, "Frost Strike", "if=talent.runic_empowerment.enabled&(frost=0|unholy=0|blood=0)" );
      st -> add_action( this, "Frost Strike", "if=talent.blood_tap.enabled&buff.blood_charge.stack<=10" );

      // Normal stuff
      st -> add_action( this, "Obliterate" );
      st -> add_talent( this, "Blood Tap", "if=buff.blood_charge.stack>10&runic_power>=20" );
      st -> add_action( this, "Frost Strike" );

      // Better than waiting
      st -> add_talent( this, "Plague Leech" );
      st -> add_action( this, "Empower Rune Weapon" );
    }
    else
    {
      st -> add_talent( this, "Blood Tap", "if=buff.blood_charge.stack>10&(runic_power>76|(runic_power>=20&buff.killing_machine.react))" );


      // Soul Reaper
      st -> add_action( this, "Soul Reaper", "if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct );
      st -> add_talent( this, "Blood Tap", "if=(target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct + "&cooldown.soul_reaper.remains=0)" );

      // Defile
      st -> add_action( this, "Defile" );
      st -> add_talent( this, "Blood Tap", "if=talent.defile.enabled&cooldown.defile.remains=0" );

      // Killing Machine / Very High RP
      st -> add_action( this, "Frost Strike", "if=buff.killing_machine.react|runic_power>88" );
      st -> add_action( this, "Frost Strike", "if=cooldown.antimagic_shell.remains<1&runic_power>=50&!buff.antimagic_shell.up" );

      // Capped Runes
      st -> add_action( this, "Howling Blast", "if=death>1|frost>1" );

      // Diseases for free
      st -> add_talent( this, "Unholy Blight", "if=!disease.ticking" );

      // Diseases for runes
      st -> add_action( this, "Howling Blast", "if=!talent.necrotic_plague.enabled&!dot.frost_fever.ticking" );
      st -> add_action( this, "Howling Blast", "if=talent.necrotic_plague.enabled&!dot.necrotic_plague.ticking" );
      st -> add_action( this, "Plague Strike", "if=!talent.necrotic_plague.enabled&!dot.blood_plague.remains<10&unholy>0" );

      // Rime
      st -> add_action( this, "Howling Blast", "if=buff.rime.react" );

      // Don't waste Runic Power
      st -> add_action( this, "Frost Strike", "if=runic_power>76" );

      // Keep Runes on Cooldown
      st -> add_action( this, "Obliterate", "if=unholy>0&!buff.killing_machine.react" );
      st -> add_action( this, "Howling Blast", "if=!(target.health.pct-3*(target.health.pct%target.time_to_die)<=35&cooldown.soul_reaper.remains<2)|death+frost>=2" );

      // Generate Runic Power or Runes
      st -> add_talent( this, "Blood Tap", "if=target.health.pct-3*(target.health.pct%target.time_to_die)>35|buff.blood_charge.stack>=8" );

      // Better than waiting
      st -> add_talent( this, "Blood Tap" );
      st -> add_talent( this, "Plague Leech" );
      st -> add_action( this, "Empower Rune Weapon" );
    }

    //AoE
    aoe -> add_talent( this, "Unholy Blight" );
    aoe -> add_action( this, "Blood Boil", "if=!talent.necrotic_plague.enabled&dot.blood_plague.ticking&talent.plague_leech.enabled,line_cd=28" );
    aoe -> add_action( this, "Blood Boil", "if=!talent.necrotic_plague.enabled&dot.blood_plague.ticking&talent.unholy_blight.enabled&cooldown.unholy_blight.remains<49,line_cd=28" );
    aoe -> add_action( this, "Howling Blast" );
    aoe -> add_talent( this, "Blood Tap", "if=buff.blood_charge.stack>10" );
    aoe -> add_action( this, "Frost Strike", "if=runic_power>76" );
    aoe -> add_action( this, "Death and Decay", "if=unholy=1" );
    aoe -> add_action( this, "Plague Strike", "if=unholy=2" );
    aoe -> add_talent( this, "Blood Tap" );
    aoe -> add_action( this, "Frost Strike" );
    aoe -> add_talent( this, "Plague Leech", "if=unholy=1" );
    aoe -> add_action( this, "Plague Strike", "if=unholy=1" );
    aoe -> add_action( this, "Empower Rune Weapon" );


    break;
  }
  case DEATH_KNIGHT_UNHOLY:
  {
    precombat -> add_action( this, "Raise Dead" );

    for ( size_t i = 0; i < get_profession_actions().size(); i++ )
      def -> add_action( get_profession_actions()[i] );

    for ( size_t i = 0; i < get_racial_actions().size(); i++ )
      def -> add_action( get_racial_actions()[i] );

    for ( size_t i = 0; i < get_item_actions().size(); i++ )
      def -> add_action( get_item_actions()[i] );

    if ( sim -> allow_potions && level >= 80 )
      def -> add_action( potion_str + ",if=buff.dark_transformation.up&target.time_to_die<=60" );

    //decide between single_target and aoe rotation
    def -> add_action( "run_action_list,name=aoe,if=active_enemies>=3" );
    def -> add_action( "run_action_list,name=single_target,if=active_enemies<3" );


    // Stop BT charges from capping
    st -> add_talent( this, "Blood Tap", "if=buff.blood_charge.stack>10&runic_power>=32" );

    // Diseases for free
    st -> add_talent( this, "Unholy Blight", "if=!talent.necrotic_plague.enabled&(dot.frost_fever.remains<3|dot.blood_plague.remains<3)" );
    st -> add_talent( this, "Unholy Blight", "if=talent.necrotic_plague.enabled&(dot.necrotic_plague.remains<3)" );
    st -> add_action( this, "Outbreak", "if=!talent.necrotic_plague.enabled&(dot.frost_fever.remains<3|dot.blood_plague.remains<3)" );
    st -> add_action( this, "Outbreak", "if=talent.necrotic_plague.enabled&(dot.necrotic_plague.remains<3)" );

    // Soul Reaper
    st -> add_action( this, "Soul Reaper", "if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct );
    st -> add_talent( this, "Blood Tap", "if=(target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct + "&cooldown.soul_reaper.remains=0)" );


    // Diseases for Runes
    st -> add_action( this, "Plague Strike", "if=!talent.necrotic_plague.enabled&(!dot.blood_plague.ticking|!dot.frost_fever.ticking)" );
    st -> add_action( this, "Plague Strike", "if=talent.necrotic_plague.enabled&(!dot.necrotic_plague.ticking)" );

    // GCD Cooldowns
    st -> add_action( this, "Summon Gargoyle" );
    st -> add_action( this, "Dark Transformation" );
    st -> add_talent( this, "Blood Tap,if=buff.shadow_infusion.stack=5" );

    // Don't waste runic power
    st -> add_action( this, "Death Coil", "if=runic_power>90" );

    // Get runes on cooldown
    st -> add_action( this, "Death and Decay", "if=unholy=2" );
    st -> add_talent( this, "Blood Tap", "if=unholy=2&cooldown.death_and_decay.remains=0" );
    st -> add_action( this, "Scourge Strike", "if=unholy=2" );
    st -> add_action( this, "Death Coil", "if=runic_power>80" );
    st -> add_action( this, "Festering Strike", "if=blood=2&frost=2" );

    // Normal stuff
    st -> add_action( this, "Death and Decay" );
    st -> add_talent( this, "Blood Tap", "if=cooldown.death_and_decay.remains=0" );
    st -> add_action( this, "Death Coil", "if=buff.sudden_doom.react|(buff.dark_transformation.down&rune.unholy<=1)" );
    st -> add_action( this, "Scourge Strike" );
    st -> add_talent( this, "Plague Leech", "if=cooldown.outbreak.remains<1" );
    st -> add_action( this, "Festering Strike" );
    st -> add_action( this, "Death Coil" );

    // Less waiting
    st -> add_talent( this, "Blood Tap", "if=buff.blood_charge.stack>=8" );
    st -> add_action( this, "Empower Rune Weapon" );

    //AoE
    aoe -> add_talent( this, "Unholy Blight" );
    aoe -> add_action( this, "Plague Strike", "if=!talent.necrotic_plague.enabled&(!dot.blood_plague.ticking|!dot.frost_fever.ticking)" );
    aoe -> add_action( this, "Plague Strike", "if=talent.necrotic_plague.enabled&(!dot.necrotic_plague.ticking)" );
    aoe -> add_action( this, "Blood Boil", "if=blood=2|(frost=2&death=2)" );
    aoe -> add_action( this, "Summon Gargoyle" );
    aoe -> add_action( this, "Dark Transformation" );
    aoe -> add_talent( this, "Blood Tap", "if=buff.shadow_infusion.stack=5" );
    aoe -> add_action( this, "Death and Decay", "if=unholy=1" );
    aoe -> add_action( this, "Soul Reaper", "if=target.health.pct-3*(target.health.pct%target.time_to_die)<=" + soul_reaper_pct );
    aoe -> add_action( this, "Scourge Strike", "if=unholy=2" );
    aoe -> add_talent( this, "Blood Tap", "if=buff.blood_charge.stack>10" );
    aoe -> add_action( this, "Death Coil", "if=runic_power>90|buff.sudden_doom.react|(buff.dark_transformation.down&rune.unholy<=1)" );
    aoe -> add_action( this, "Blood Boil" );
    aoe -> add_action( this, "Icy Touch" );
    aoe -> add_action( this, "Scourge Strike", "if=unholy=1" );
    aoe -> add_action( this, "Death Coil" );
    aoe -> add_talent( this, "Blood Tap" );
    aoe -> add_talent( this, "Plague Leech", "if=unholy=1" );
    aoe -> add_action( this, "Empower Rune Weapon" );

    break;
  }
  default: break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// Runeforges

void runeforge::razorice_attack( special_effect_t& effect, 
                                 const item_t& item )
{
  struct razorice_attack_t : public death_knight_melee_attack_t
  {
    razorice_attack_t( death_knight_t* player, const std::string& name, const item_t& /*item*/ ) :
      death_knight_melee_attack_t( name, player, player -> find_spell( 50401 ) )
    {
      school      = SCHOOL_FROST;
      may_miss    = callbacks = false;
      background  = proc = true;

      weapon_multiplier += player -> perk.improved_runeforges -> effectN( 1 ).percent();
      weapon = &( player -> main_hand_weapon );
      /*
      if ( item.slot == SLOT_OFF_HAND )
        weapon = &( player -> off_hand_weapon );
      else if ( item.slot == SLOT_MAIN_HAND )
        weapon = &( player -> main_hand_weapon );
      */
    }

    // No double dipping to Frost Vulnerability
    double composite_target_multiplier( player_t* t ) const
    {
      double m = death_knight_melee_attack_t::composite_target_multiplier( t );

      m /= 1.0 + td( t ) -> debuffs_razorice -> check() *
            td( t ) -> debuffs_razorice -> data().effectN( 1 ).percent();

      return m;
    }
  };

  effect.execute_action = new razorice_attack_t( debug_cast<death_knight_t*>( item.player ), effect.name(), item );

  new dbc_proc_callback_t( item, effect );
}

void runeforge::razorice_debuff( special_effect_t& effect, 
                                 const item_t& item )
{
  struct razorice_callback_t : public dbc_proc_callback_t
  {
    razorice_callback_t( const item_t& item, const special_effect_t& effect ) :
     dbc_proc_callback_t( item, effect )
    { }

    void execute( action_t* a, action_state_t* state )
    {
      debug_cast< death_knight_t* >( a -> player ) -> get_target_data( state -> target ) -> debuffs_razorice -> trigger();
      if ( a -> sim -> current_time < timespan_t::from_seconds( 0.01 ) )
        debug_cast< death_knight_t* >( a -> player ) -> get_target_data( state -> target ) -> debuffs_razorice -> constant = false;
    }
  };

  new razorice_callback_t( item, effect );
}

void runeforge::fallen_crusader( special_effect_t& effect, 
                                 const item_t& item )
{
  // Fallen Crusader buff is shared between mh/oh
  buff_t* b = buff_t::find( item.player, "unholy_strength" );
  if ( ! b )
    return;

  const death_knight_t* dk = debug_cast<const death_knight_t*>( item.player );

  effect.ppm_ = -1.0 * dk -> fallen_crusader_rppm;
  effect.custom_buff = b;

  new dbc_proc_callback_t( item, effect );
}

void runeforge::stoneskin_gargoyle( special_effect_t&, const item_t& item )
{
  death_knight_t* p = debug_cast<death_knight_t*>( item.player );
  p -> runeforge.rune_of_the_stoneskin_gargoyle -> default_chance = 1.0;
}

void runeforge::spellshattering( special_effect_t&, const item_t& item )
{
  death_knight_t* p = debug_cast<death_knight_t*>( item.player );
  p -> runeforge.rune_of_spellshattering -> default_chance = 1.0;
}

void runeforge::spellbreaking( special_effect_t&, const item_t& item )
{
  death_knight_t* p = debug_cast<death_knight_t*>( item.player );
  if ( item.slot == SLOT_MAIN_HAND )
    p -> runeforge.rune_of_spellbreaking -> default_chance = 1.0;
  else if ( item.slot == SLOT_OFF_HAND )
    p -> runeforge.rune_of_spellbreaking_oh -> default_chance = 1.0;
}

bool death_knight_t::init_special_effect( special_effect_t& effect,
                                          const item_t&,
                                          unsigned spell_id )
{
  static unique_gear::special_effect_db_item_t __runeforge_db[] =
  {
    {  50401, 0,    runeforge::razorice_attack },
    {  51714, 0,    runeforge::razorice_debuff },
    { 166441, 0,    runeforge::fallen_crusader },
    {  62157, 0, runeforge::stoneskin_gargoyle },
    {  53362, 0,    runeforge::spellshattering }, // Damage taken, we don't use the silence reduction
    {  54449, 0,      runeforge::spellbreaking }, // Damage taken, we don't use the silence reduction

    // Last entry must be all zeroes
    {     0, 0,                             0 },
  };

  bool ret = false;
  const special_effect_db_item_t& dbitem = find_special_effect_db_item( __runeforge_db,
                                                                        (int)sizeof_array( __runeforge_db ),
                                                                        spell_id );

  // All runeforges defined here will be custom callbacks
  ret = dbitem.spell_id == spell_id;
  if ( ret )
  {
    effect.custom_init = dbitem.custom_cb;
    effect.type = SPECIAL_EFFECT_CUSTOM;
  }

  return ret;
}

// death_knight_t::init_scaling =============================================

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( off_hand_weapon.type != WEAPON_NONE )
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;

  scales_with[ STAT_AGILITY ] = false;
}

// death_knight_t::init_buffs ===============================================
struct death_shroud_mastery
{
  death_shroud_mastery( player_t* p ) : player(p) {}
  bool operator()(const stat_buff_t&) const
  {

    double haste = player -> composite_melee_haste_rating();
    if ( player -> sim -> scaling -> scale_stat == STAT_HASTE_RATING )
      haste -= player -> sim -> scaling -> scale_value * player -> composite_rating_multiplier( RATING_MELEE_HASTE );

    double mastery = player -> composite_mastery_rating();
    if ( player -> sim -> scaling -> scale_stat == STAT_MASTERY_RATING )
      mastery -= player -> sim -> scaling -> scale_value * player -> composite_rating_multiplier( RATING_MASTERY );

    if ( mastery >= haste )
      return true;
    return false;
  }
  player_t* player;
};

struct death_shroud_haste
{
  death_shroud_haste( player_t* p ) : player(p) {}
  bool operator()(const stat_buff_t&) const
  {

    double haste = player -> composite_melee_haste_rating();
    if ( player -> sim -> scaling -> scale_stat == STAT_HASTE_RATING )
      haste -= player -> sim -> scaling -> scale_value * player -> composite_rating_multiplier( RATING_MELEE_HASTE );

    double mastery = player -> composite_mastery_rating();
    if ( player -> sim -> scaling -> scale_stat == STAT_MASTERY_RATING )
      mastery -= player -> sim -> scaling -> scale_value * player -> composite_rating_multiplier( RATING_MASTERY );

    if ( haste > mastery )
      return true;
    return false;
  }
   player_t* player;
};

void death_knight_t::create_buffs()
{
  player_t::create_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buffs.army_of_the_dead    = buff_creator_t( this, "army_of_the_dead", find_class_spell( "Army of the Dead" ) )
                              .cd( timespan_t::zero() );
  buffs.blood_shield        = new blood_shield_buff_t( this );
  buffs.rune_tap            = buff_creator_t( this, "rune_tap", find_specialization_spell( "Rune Tap" ) -> effectN( 1 ).trigger() );

  buffs.antimagic_shell     = buff_creator_t( this, "antimagic_shell", find_class_spell( "Anti-Magic Shell" ) )
                              .cd( timespan_t::zero() );
  buffs.blood_charge        = buff_creator_t( this, "blood_charge", find_spell( 114851 ) )
                              .chance( find_talent_spell( "Blood Tap" ) -> ok() );
  buffs.blood_presence      = buff_creator_t( this, "blood_presence", find_class_spell( "Blood Presence" ) )
                              .add_invalidate( CACHE_STAMINA );
  buffs.bone_shield         = buff_creator_t( this, "bone_shield", find_specialization_spell( "Bone Shield" ) )
                              .cd( timespan_t::zero() )
                              .max_stack( specialization() == DEATH_KNIGHT_BLOOD ? ( find_specialization_spell( "Bone Shield" ) -> initial_stacks() +
                                          find_spell( 144948 ) -> max_stacks() + perk.enhanced_bone_shield -> effectN( 1 ).base_value() ) : -1 );
  buffs.bone_wall           = buff_creator_t( this, "bone_wall", find_spell( 144948 ) )
                              .chance( sets.has_set_bonus( SET_TANK, T16, B2 ) );
  buffs.crimson_scourge     = buff_creator_t( this, "crimson_scourge" ).spell( find_spell( 81141 ) )
                              .chance( spec.crimson_scourge -> proc_chance() );
  buffs.dancing_rune_weapon = buff_creator_t( this, "dancing_rune_weapon", find_spell( 81256 ) )
                              .cd( timespan_t::zero() )
                              .add_invalidate( CACHE_PARRY );
  buffs.dark_transformation = buff_creator_t( this, "dark_transformation", find_class_spell( "Dark Transformation" ) );
  buffs.deaths_advance      = buff_creator_t( this, "deaths_advance", talent.deaths_advance )
                              .default_value( talent.deaths_advance -> effectN( 1 ).percent() );
  buffs.deathbringer        = buff_creator_t( this, "deathbringer", find_spell( 144953 ) )
                              .chance( sets.has_set_bonus( SET_TANK, T16, B4 ) );
  buffs.death_shroud        = stat_buff_creator_t( this, "death_shroud", sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).trigger() )
                              .chance( sets.set( SET_MELEE, T16, B2 ) -> proc_chance() )
                              .add_stat( STAT_MASTERY_RATING, sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).trigger() -> effectN( 2 ).base_value(), death_shroud_mastery( this ) )
                              .add_stat( STAT_HASTE_RATING, sets.set( SET_MELEE, T16, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value(), death_shroud_haste( this ) );
  buffs.frost_presence      = buff_creator_t( this, "frost_presence", find_class_spell( "Frost Presence" ) )
                              .default_value( find_class_spell( "Frost Presence" ) -> effectN( 1 ).percent() );
  buffs.icebound_fortitude  = buff_creator_t( this, "icebound_fortitude", find_class_spell( "Icebound Fortitude" ) )
                              .duration( find_class_spell( "Icebound Fortitude" ) -> duration() *
                                         ( 1.0 + glyph.icebound_fortitude -> effectN( 2 ).percent() ) )
                              .cd( timespan_t::zero() );
  buffs.killing_machine     = buff_creator_t( this, "killing_machine", find_spell( 51124 ) )
                              .default_value( find_spell( 51124 ) -> effectN( 1 ).percent() )
                              .chance( find_specialization_spell( "Killing Machine" ) -> proc_chance() ); // PPM based!
  buffs.pillar_of_frost     = buff_creator_t( this, "pillar_of_frost", find_class_spell( "Pillar of Frost" ) )
                              .cd( timespan_t::zero() )
                              .default_value( find_class_spell( "Pillar of Frost" ) -> effectN( 1 ).percent() +
                                              sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).percent() +
                                              perk.empowered_pillar_of_frost -> effectN( 1 ).percent() )
                              .add_invalidate( CACHE_STRENGTH );
  buffs.rime                = buff_creator_t( this, "rime", find_spell( 59052 ) )
                              .max_stack( ( sets.has_set_bonus( SET_MELEE, T13, B2 ) ) ? 2 : 1 )
                              .cd( timespan_t::zero() )
                              .chance( spec.rime -> ok() );
  buffs.riposte             = stat_buff_creator_t( this, "riposte", spec.riposte -> effectN( 1 ).trigger() )
                              .cd( spec.riposte -> internal_cooldown() )
                              .chance( spec.riposte -> proc_chance() )
                              .add_stat( STAT_CRIT_RATING, 0 );
  //buffs.runic_corruption    = buff_creator_t( this, "runic_corruption", find_spell( 51460 ) )
  //                            .chance( talent.runic_corruption -> proc_chance() );
  buffs.runic_corruption    = new runic_corruption_buff_t( this );
  buffs.scent_of_blood      = buff_creator_t( this, "scent_of_blood", find_spell( 50421 ) )
                              .chance( spec.scent_of_blood -> ok() );
  // Trick simulator into recalculating health when this buff goes up/down.
  buffs.shadow_of_death     = new shadow_of_death_t( this );
  buffs.sudden_doom         = buff_creator_t( this, "sudden_doom", find_spell( 81340 ) )
                              .max_stack( ( sets.has_set_bonus( SET_MELEE, T13, B2 ) ) ? 2 : 1 )
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
                                    .cd( timespan_t::from_seconds( 45 ) + perk.enhanced_will_of_the_necropolis -> effectN( 1 ).time_value() );
  buffs.will_of_the_necropolis_rt = buff_creator_t( this, "will_of_the_necropolis_rt", find_spell( 96171 ) )
                                    .cd( timespan_t::from_seconds( 45 ) + perk.enhanced_will_of_the_necropolis -> effectN( 1 ).time_value() );

  runeforge.rune_of_the_fallen_crusader = buff_creator_t( this, "unholy_strength", find_spell( 53365 ) )
                                          .add_invalidate( CACHE_STRENGTH );

  runeforge.rune_of_the_stoneskin_gargoyle = buff_creator_t( this, "stoneskin_gargoyle", find_spell( 62157 ) )
                                             .chance( 0 );
  runeforge.rune_of_spellshattering = buff_creator_t( this, "rune_of_spellshattering", find_spell( 53362 ) )
                                      .chance( 0 );
  runeforge.rune_of_spellbreaking = buff_creator_t( this, "rune_of_spellbreaking", find_spell( 54449 ) )
                                    .chance( 0 );
  runeforge.rune_of_spellbreaking_oh = buff_creator_t( this, "rune_of_spellbreaking_oh", find_spell( 54449 ) )
                                       .chance( 0 );

  buffs.conversion = buff_creator_t( this, "conversion", talent.conversion ).duration( timespan_t::zero() );

  buffs.frozen_runeblade = new frozen_runeblade_buff_t( this );
}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains.antimagic_shell                  = get_gain( "antimagic_shell"            );
  gains.blood_rites                      = get_gain( "blood_rites"                );
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
  gains.necrotic_plague                  = get_gain( "necrotic_plague"            );
  gains.plague_leech                     = get_gain( "plague_leech"               );
  gains.rc                               = get_gain( "runic_corruption_all"       );
  gains.rc_unholy                        = get_gain( "runic_corruption_unholy"    );
  gains.rc_blood                         = get_gain( "runic_corruption_blood"     );
  gains.rc_frost                         = get_gain( "runic_corruption_frost"     );
  // gains.blood_tap_blood                  = get_gain( "blood_tap_blood"            );
  //gains.blood_tap_blood          -> type = ( resource_e ) RESOURCE_RUNE_BLOOD   ;
  gains.hp_death_siphon                  = get_gain( "hp_death_siphon"            );
  gains.t15_4pc_tank                     = get_gain( "t15_4pc_tank"               );
  gains.t17_2pc_frost                    = get_gain( "t17_2pc_frost"              );
  gains.t17_4pc_unholy_blood             = get_gain( "Unholy T17 4PC: Blood runes" );
  gains.t17_4pc_unholy_frost             = get_gain( "Unholy T17 4PC: Frost runes" );
  gains.t17_4pc_unholy_unholy            = get_gain( "Unholy T17 4PC: Unholy runes" );
  gains.t17_4pc_unholy_waste             = get_gain( "Unholy T17 4PC: Wasted runes" );
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

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

  runic_power_decay_rate = 1; // 1 RP per second decay
  blood_charge_counter = 0;
  shadow_infusion_counter = 0;

  t15_2pc_melee.reset();

  _runes.reset();
}

// death_knight_t::combat_begin =============================================

void death_knight_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    resolve_manager.start();
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, dmg_e t, action_state_t* s )
{
  if ( buffs.vampiric_blood -> up() )
    s -> result_amount *= 1.0 + buffs.vampiric_blood -> data().effectN( 1 ).percent() +
                                glyph.vampiric_blood -> effectN( 1 ).percent();

  player_t::assess_heal( school, t, s );
}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::assess_damage_imminent( school_e school, dmg_e, action_state_t* s )
{
  if ( buffs.antimagic_shell -> up() && school != SCHOOL_PHYSICAL )
  {
    double absorbed = s -> result_amount * spell.antimagic_shell -> effectN( 1 ).percent();

    double generated = absorbed / resources.max[ RESOURCE_HEALTH ];

    s -> result_amount -= absorbed;
    s -> result_absorbed -= absorbed;

    gains.antimagic_shell -> add( RESOURCE_HEALTH, absorbed );

    resource_gain( RESOURCE_RUNIC_POWER, util::round( generated * 100.0 ), gains.antimagic_shell, s -> action );
  }

  if ( talent.necrotic_plague -> ok() && s -> result_amount > 0 && ! s -> action -> result_is_multistrike( s -> result ) )
    resource_gain( RESOURCE_RUNIC_POWER, spell.necrotic_plague_energize -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ), gains.necrotic_plague );
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

      if ( sets.has_set_bonus( SET_TANK, T15, B4 ) && buffs.bone_shield -> check() )
      {
        resource_gain( RESOURCE_RUNIC_POWER,
                       spell.t15_4pc_tank -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER ),
                       gains.t15_4pc_tank );
      }
    }
  }

  if ( health_pct >= 35 && health_percentage() < 35 )
  {
    buffs.will_of_the_necropolis_dr -> trigger();
    buffs.will_of_the_necropolis_rt -> trigger();
  }

  if ( s -> result == RESULT_DODGE || s -> result == RESULT_PARRY )
  {
    buffs.riposte -> stats[ 0 ].amount = ( current.stats.dodge_rating + current.stats.parry_rating ) * spec.riposte -> effectN( 1 ).percent();
    buffs.riposte -> trigger();
  }
}

// death_knight_t::target_mitigation ========================================

void death_knight_t::target_mitigation( school_e school, dmg_e type, action_state_t* state )
{
  if ( buffs.blood_presence -> check() )
    state -> result_amount *= 1.0 + buffs.blood_presence -> data().effectN( 6 ).percent();

  if ( school != SCHOOL_PHYSICAL && runeforge.rune_of_spellshattering -> check() )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellshattering -> data().effectN( 1 ).percent();

  if ( school != SCHOOL_PHYSICAL && runeforge.rune_of_spellbreaking -> check() )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellbreaking -> data().effectN( 1 ).percent();

  if ( school != SCHOOL_PHYSICAL && runeforge.rune_of_spellbreaking_oh -> check() )
    state -> result_amount *= 1.0 + runeforge.rune_of_spellbreaking_oh -> data().effectN( 1 ).percent();

  if ( buffs.rune_tap -> up() )
    state -> result_amount *= 1.0 + buffs.rune_tap -> data().effectN( 1 ).percent();

  if ( buffs.bone_shield -> up() )
    state -> result_amount *= 1.0 + buffs.bone_shield -> data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude -> up() )
    state -> result_amount *= 1.0 + buffs.icebound_fortitude -> data().effectN( 3 ).percent() + spec.sanguine_fortitude -> effectN( 1 ).percent() + perk.empowered_icebound_fortitude -> effectN( 1 ).percent();

  if ( buffs.will_of_the_necropolis_dr -> up() )
    state -> result_amount *= 1.0 + spec.will_of_the_necropolis -> effectN( 1 ).percent();

  if ( buffs.army_of_the_dead -> check() )
    state -> result_amount *= 1.0 - buffs.army_of_the_dead -> value();

  if ( talent.defile -> ok() )
  {
    death_knight_td_t* tdata = get_target_data( state -> action -> player );
    if ( tdata -> dots_defile -> is_ticking() )
      state -> result_amount *= 1.0 - talent.defile -> effectN( 4 ).percent();
  }

  player_t::target_mitigation( school, type, state );
}

// death_knight_t::composite_armor_multiplier ===============================

double death_knight_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buffs.blood_presence -> check() )
    a *= 1.0 + buffs.blood_presence -> data().effectN( 3 ).percent();

  if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
    a *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 1 ).percent();

  return a;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    if ( runeforge.rune_of_the_fallen_crusader -> up() )
      m *= 1.0 + fallen_crusader;
    m *= 1.0 + buffs.pillar_of_frost -> value();
  }
  else if ( attr == ATTR_STAMINA )
  {
    if ( buffs.blood_presence -> check() )
      m *= 1.0 + buffs.blood_presence -> data().effectN( 5 ).percent();

    if ( runeforge.rune_of_the_stoneskin_gargoyle -> check() )
      m *= 1.0 + runeforge.rune_of_the_stoneskin_gargoyle -> data().effectN( 2 ).percent();
  }

  return m;
}

double death_knight_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
    case RATING_MULTISTRIKE:
      m *= 1.0 + spec.multistrike_attunement -> effectN( 1 ).percent();
      m *= 1.0 + spec.necrosis -> effectN( 1 ).percent();
      break;
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      m *= 1.0 + spec.icy_talons -> effectN( 3 ).percent();
      break;
    default:
      break;
  }
  return m;
}

// death_knight_t::matching_gear_multiplier =================================

double death_knight_t::matching_gear_multiplier( attribute_e attr ) const
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

// death_knight_t::composite_multistrike ===================================

double death_knight_t::composite_multistrike() const
{
  double multistrike = player_t::composite_multistrike();

  multistrike += spec.veteran_of_the_third_war -> effectN( 5 ).percent();

  return multistrike;
}

// death_knight_t::composite_melee_expertise ===============================

double death_knight_t::composite_melee_expertise( weapon_t* ) const
{
  double expertise = player_t::composite_melee_expertise( 0 );

  expertise += spec.veteran_of_the_third_war -> effectN( 3 ).percent();

  return expertise;
}
// warrior_t::composite_parry_rating() ========================================

double death_knight_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // add Riposte
  if ( spec.riposte -> ok() )
    p += composite_melee_crit_rating();

  return p;
}

// death_knight_t::composite_parry ============================================

double death_knight_t::composite_parry() const
{
  double parry = player_t::composite_parry();

  if ( buffs.dancing_rune_weapon -> up() )
    parry += buffs.dancing_rune_weapon -> data().effectN( 1 ).percent();

  return parry;
}

// death_knight_t::composite_dodge ============================================

double death_knight_t::composite_dodge() const
{
  double dodge = player_t::composite_dodge();

  dodge += spec.veteran_of_the_third_war -> effectN( 2 ).percent();

  return dodge;
}

// death_knight_t::composite_player_multiplier ==============================

double death_knight_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( mastery.dreadblade -> ok() && dbc::is_school( school, SCHOOL_SHADOW )  )
    m *= 1.0 + cache.mastery_value();

  if ( mastery.frozen_heart -> ok() && dbc::is_school( school, SCHOOL_FROST )  )
    m *= 1.0 + cache.mastery_value();

  m *= 1.0 + spec.improved_blood_presence -> effectN( 2 ).percent();

  return m;
}

// death_knight_t::composite_player_heal_multiplier ==============================

double death_knight_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  // Resolve applies a blanket -60% healing & absorb for tanks
  if ( spec.resolve -> ok() )
    m *= 1.0 + spec.resolve -> effectN( 2 ).percent();

  return m;

}

// death_knight_t::composite_player_absorb_multiplier ==============================

double death_knight_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );

  // Resolve applies a blanket -60% healing & absorb for tanks
  if ( spec.resolve -> ok() )
    m *= 1.0 + spec.resolve -> effectN( 3 ).percent();

  return m;

}

// death_knight_t::composite_melee_attack_power ==================================

double death_knight_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += spec.bladed_armor -> effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

double death_knight_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  m *= 1.0 + mastery.blood_shield -> effectN( 3 ).mastery_value() * composite_mastery();

  return m;
}

// death_knight_t::composite_attack_speed() =================================

double death_knight_t::composite_melee_speed() const
{
  double haste = player_t::composite_melee_speed();

  haste *= 1.0 / ( 1.0 + spec.icy_talons -> effectN( 1 ).percent() );

  return haste;
}

// death_knight_t::composite_tank_crit ======================================

double death_knight_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.improved_blood_presence -> effectN( 1 ).percent();

  return c;
}

// death_knight_t::temporary_movement_modifier ==================================

double death_knight_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  if ( buffs.deaths_advance -> up() )
    temporary = std::max( buffs.deaths_advance -> default_value, temporary );

  return temporary;
}

// death_knight_t::passive_movement_modifier====================================

double death_knight_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buffs.unholy_presence -> up() )
    ms += buffs.unholy_presence -> data().effectN( 2 ).percent();

  if ( talent.deaths_advance -> ok() )
    ms += spell.deaths_advance -> effectN( 2 ).percent();

  return ms;
}

// death_knight_t::invalidate_cache =========================================

void death_knight_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_CRIT:
      if ( spec.riposte -> ok() )
        player_t::invalidate_cache( CACHE_PARRY );
      break;
    case CACHE_BONUS_ARMOR:
      if ( spec.bladed_armor -> ok() )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_MASTERY:
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      if ( specialization() == DEATH_KNIGHT_BLOOD )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    default: break;
  }
}

// death_knight_t::primary_role =============================================

role_e death_knight_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// death_knight_t::create_options ================================================

void death_knight_t::create_options()
{
  player_t::create_options();

  option_t death_knight_options[] =
  {
    opt_float( "fallen_crusader_str", fallen_crusader ),
    opt_float( "fallen_crusader_rppm", fallen_crusader_rppm ),
    opt_null()
  };

  option_t::copy( options, death_knight_options );
}

// death_knight_t::convert_hybrid_stat ==============================================

stat_e death_knight_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  // This is a guess at how AGI/INT will work for DKs, TODO: confirm
  case STAT_AGI_INT: 
    return STAT_NONE; 
  case STAT_STR_AGI_INT:
  case STAT_STR_AGI:
  case STAT_STR_INT:
    return STAT_STRENGTH;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == DEATH_KNIGHT_BLOOD )
      return s;
    else
      return STAT_NONE;     
  default: return s; 
  }
}

// death_knight_t::regen ====================================================

void death_knight_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    _runes.slot[ i ].regen_rune( this, periodicity );
}

// death_knight_t::trigger_runic_empowerment ================================

void death_knight_t::trigger_runic_empowerment( double rpcost )
{
  if ( ! rng().roll( talent.runic_empowerment -> effectN( 1 ).percent() * rpcost / 100.0 ) )
    return;

  int depleted_runes[RUNE_SLOT_MAX];
  int num_depleted = 0;

  // Find fully depleted runes, i.e., both runes are on CD
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( _runes.slot[i].is_depleted() )
      depleted_runes[ num_depleted++ ] = i;

  if ( num_depleted > 0 )
  {
    int rune_to_regen = depleted_runes[ ( int ) ( player_t::rng().real() * num_depleted * 0.9999 ) ];
    rune_t* regen_rune = &_runes.slot[rune_to_regen];
    regen_rune -> fill_rune();
    if      ( regen_rune -> is_blood()  ) gains.runic_empowerment_blood  -> add ( RESOURCE_RUNE_BLOOD, 1, 0 );
    else if ( regen_rune -> is_unholy() ) gains.runic_empowerment_unholy -> add ( RESOURCE_RUNE_UNHOLY, 1, 0 );
    else if ( regen_rune -> is_frost()  ) gains.runic_empowerment_frost  -> add ( RESOURCE_RUNE_FROST, 1, 0 );

    gains.runic_empowerment -> add ( RESOURCE_RUNE, 1, 0 );
    if ( sim -> log ) sim -> out_log.printf( "runic empowerment regen'd rune %d", rune_to_regen );
    procs.runic_empowerment -> occur();

    buffs.tier13_4pc_melee -> trigger( 1, buff_t::DEFAULT_VALUE(), sets.set( SET_MELEE, T13, B4 ) -> effectN( 1 ).percent() );
  }
  else
  {
    // If there were no available runes to refresh
    procs.runic_empowerment_wasted -> occur();
    gains.runic_empowerment -> add ( RESOURCE_RUNE, 0, 1 );
  }
}

void death_knight_t::trigger_blood_charge( double rpcost )
{
  double multiplier = rpcost / RUNIC_POWER_DIVISOR;

  if ( ! talent.blood_tap -> ok() )
    return;

  blood_charge_counter += 2 * multiplier;
  int stacks = 0;
  while ( blood_charge_counter >= 1 )
  {
    stacks++;
    blood_charge_counter--;
  }

  if ( sim -> debug )
    sim -> out_debug.printf( "%s generates %f blood charges, %u stacks, %f charges remaining",
        name(), 2 * multiplier, stacks, blood_charge_counter );

  if ( stacks > 0 )
    buffs.blood_charge -> trigger( stacks );
}

void death_knight_t::trigger_shadow_infusion( double rpcost )
{
  if ( buffs.dark_transformation -> check() )
    return;

  double multiplier = rpcost / RUNIC_POWER_DIVISOR;

  if ( ! spec.shadow_infusion -> ok() )
    return;

  shadow_infusion_counter += multiplier;
  int stacks = 0;
  while ( shadow_infusion_counter >= 1 )
  {
    stacks++;
    shadow_infusion_counter--;
  }

  if ( stacks > 0 )
  {
    buffs.shadow_infusion -> trigger( stacks );
    shadow_infusion_counter -= stacks;
  }
}

void death_knight_t::trigger_necrosis( const action_state_t* state )
{
  if ( ! spec.necrosis -> ok() )
    return;

  if ( ! state -> action -> result_is_multistrike( state -> result ) )
    return;

  active_spells.necrosis -> target = state -> target;
  active_spells.necrosis -> schedule_execute();
}

void death_knight_t::trigger_t17_4pc_frost( const action_state_t* state )
{
  if ( ! sets.has_set_bonus( DEATH_KNIGHT_FROST, T17, B4 ) )
    return;

  if ( ! state -> action -> special || state -> action -> proc ||
       state -> action -> background )
    return;

  if ( state -> result_amount == 0 )
    return;

  if ( state -> action -> result_is_multistrike( state -> result ) ||
       ! state -> action -> result_is_hit( state -> result ) )
    return;

  if ( ! buffs.frozen_runeblade -> check() )
    return;

  buffs.frozen_runeblade -> trigger();
}

void death_knight_t::trigger_t17_4pc_unholy( const action_state_t* )
{
  if ( ! sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T17, B4 ) )
    return;

  size_t max_runes = sets.set( DEATH_KNIGHT_UNHOLY, T17, B4 ) -> effectN( 2 ).base_value();
  size_t n_regened = 0;
  for ( size_t i = 0; i < _runes.slot.size() && n_regened < max_runes; i++ )
  {
    if ( _runes.slot[ i ].state != STATE_REGENERATING )
      continue;

    rune_t* regen_rune = &( _runes.slot[ i ] );

    regen_rune -> fill_rune();
    regen_rune -> type |= RUNE_TYPE_DEATH;

    if ( regen_rune -> is_blood() )
      gains.t17_4pc_unholy_blood -> add( RESOURCE_RUNE, 1, 0 );
    else if ( regen_rune -> is_frost() )
      gains.t17_4pc_unholy_frost -> add( RESOURCE_RUNE, 1, 0 );
    else if ( regen_rune -> is_unholy() )
      gains.t17_4pc_unholy_unholy -> add( RESOURCE_RUNE, 1, 0 );

    if ( sim -> log ) sim -> out_log.printf( "%s regened rune %d", name(), i );
    n_regened++;
  }

  for ( size_t i = 0; i < max_runes - n_regened; i++ )
    gains.t17_4pc_unholy_waste -> add( RESOURCE_RUNE, 1, 0 );

  if ( sim -> debug )
    log_rune_status( this, true );
}

// death_knight_t::trigger_plaguebearer =====================================

void death_knight_t::trigger_plaguebearer( action_state_t* s )
{
  if ( ! talent.plaguebearer -> ok() )
    return;

  timespan_t pb_extend = timespan_t::from_seconds( talent.plaguebearer -> effectN( 1 ).base_value() );

  death_knight_td_t* tdata = get_target_data( s -> target );
  if ( tdata -> dots_blood_plague -> is_ticking() )
    tdata -> dots_blood_plague -> extend_duration( pb_extend );

  if ( tdata -> dots_frost_fever -> is_ticking() )
    tdata -> dots_frost_fever -> extend_duration( pb_extend );

  if ( tdata -> dots_necrotic_plague -> is_ticking() )
  {
    active_spells.necrotic_plague -> target = s -> target;
    active_spells.necrotic_plague -> schedule_execute();
  }
}

void death_knight_t::apply_diseases( action_state_t* state, unsigned diseases )
{
  if ( ! talent.necrotic_plague -> ok() )
  {
    if ( diseases & DISEASE_BLOOD_PLAGUE )
    {
      active_spells.blood_plague -> target = state -> target;
      active_spells.blood_plague -> execute();
    }

    if ( diseases & DISEASE_FROST_FEVER )
    {
      active_spells.frost_fever -> target = state -> target;
      active_spells.frost_fever -> execute();
    }
  }
  else
  {
    active_spells.necrotic_plague -> target = state -> target;
    active_spells.necrotic_plague -> execute();
  }
}

// death_knight_t rune inspections ==========================================

// death_knight_t::runes_count ==============================================
// how many runes of type rt are available
int death_knight_t::runes_count( rune_type rt, bool include_death, int position )
{
  double result = 0;
  // positional checks first
  if ( position > 0 && ( rt == RUNE_TYPE_BLOOD || rt == RUNE_TYPE_FROST || rt == RUNE_TYPE_UNHOLY ) )
  {
    rune_t* r = &_runes.slot[( ( rt - 1 ) * 2 ) + ( position - 1 ) ];
    result += r -> value;
  }
  else
  {
    int rpc = 0;
    for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
    {
      rune_t* r = &_runes.slot[ i ];
      // query a specific position death rune.
      if ( position != 0 && rt == RUNE_TYPE_DEATH && r -> is_death() )
      {
        if ( ++rpc == position )
        {
          result += r -> value;
          break;
        }
      }
      // just count the runes
      else if ( ( ( ( include_death || rt == RUNE_TYPE_DEATH ) && r -> is_death() ) || r -> get_type() == rt ) )
      {
        result += r -> value;
      }
    }
  }
  return static_cast<int>( result );
}

// death_knight_t::runes_cooldown_any =======================================

double death_knight_t::runes_cooldown_any( rune_type rt, bool include_death, int position )
{
  rune_t* rune = 0;
  int rpc = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    rune_t* r = &_runes.slot[i];
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
    rune_t* r = &_runes.slot[i];
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

double death_knight_t::runes_cooldown_time( rune_t* rune )
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
  rune_t* rune = 0;
  int rpc = 0;
  // iterate, to allow finding death rune slots as well
  for ( int i = 0; i < RUNE_SLOT_MAX; i++ )
  {
    rune_t* r = &_runes.slot[i];
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

  if ( specialization() == DEATH_KNIGHT_FROST  && ! sim -> overrides.haste ) sim -> auras.haste -> trigger();
  if ( specialization() == DEATH_KNIGHT_UNHOLY && ! sim -> overrides.haste ) sim -> auras.haste -> trigger();
  if ( specialization() == DEATH_KNIGHT_FROST  && ! sim -> overrides.versatility ) sim -> auras.versatility -> trigger();
  if ( specialization() == DEATH_KNIGHT_UNHOLY && ! sim -> overrides.versatility ) sim -> auras.versatility -> trigger();

  runeforge.rune_of_the_stoneskin_gargoyle -> trigger();
  runeforge.rune_of_spellshattering -> trigger();
  runeforge.rune_of_spellbreaking -> trigger();
  runeforge.rune_of_spellbreaking_oh -> trigger();
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class death_knight_report_t : public player_report_extension_t
{
public:
  death_knight_report_t( death_knight_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void) p; // Stop annoying compiler nag
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  death_knight_t& p;
};

// DEATH_KNIGHT MODULE INTERFACE ============================================

struct death_knight_module_t : public module_t
{
  death_knight_module_t() : module_t( DEATH_KNIGHT ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    death_knight_t* p = new death_knight_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new death_knight_report_t( *p ) );
    return p;
  }
  virtual void init( sim_t* ) const {}
  virtual bool valid() const { return true; }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::death_knight()
{
  static death_knight_module_t m;
  return &m;
}

