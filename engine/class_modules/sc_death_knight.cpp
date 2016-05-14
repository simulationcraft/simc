// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO:


#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

using namespace unique_gear;

struct death_knight_t;
struct runes_t;

namespace pets {
  struct dancing_rune_weapon_pet_t;
  struct ghoul_pet_t;
}

namespace runeforge {
  // TODO: set up rune forges
  void razorice_attack( special_effect_t& );
  void razorice_debuff( special_effect_t& );
  void fallen_crusader( special_effect_t& );
  void stoneskin_gargoyle( special_effect_t& );
}

// ==========================================================================
// Death Knight Runes
// ==========================================================================


enum disease_type { DISEASE_NONE, DISEASE_BLOOD_PLAGUE, DISEASE_FROST_FEVER, DISEASE_VIRULENT_PLAGUE };
enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

const double RUNIC_POWER_REFUND = 0.9;
const double RUNE_REGEN_BASE = 10;
const double RUNE_REGEN_BASE_SEC = ( 1 / RUNE_REGEN_BASE );

const size_t MAX_RUNES = 6;
const size_t MAX_REGENERATING_RUNES = 3;

struct rune_t
{
  runes_t*   runes;     // Back reference to runes_t array so we can adjust rune state
  rune_state state;     // DEPLETED, REGENERATING, FULL
  double     value;     // 0.0 to 1.0, with 1.0 being full

  rune_t() : runes( nullptr ), state( STATE_FULL ), value( 0.0 )
  { }

  rune_t( runes_t* r ) : runes( r ), state( STATE_FULL ), value( 0.0 )
  { }

  bool is_ready()    const     { return state == STATE_FULL    ; }
  bool is_regenerating() const { return state == STATE_REGENERATING; }
  bool is_depleted() const     { return state == STATE_DEPLETED; }

  // Regenerate this rune for periodicity seconds
  void regen_rune( timespan_t periodicity, bool rc = false );

  // Consume this rune and adjust internal rune state
  rune_t* consume();
  // Fill this rune and adjust internal rune state
  rune_t* fill_rune();

  void reset()
  {
    state = STATE_FULL;
    value = 1.0;
  }
};

struct runes_t
{
  death_knight_t* dk;
  std::array<rune_t, MAX_RUNES> slot;

  runes_t( death_knight_t* p ) : dk( p )
  {
    for ( auto& rune: slot )
    {
      rune = rune_t( this );
    }
  }

  void reset()
  {
    for ( auto& rune : slot)
    {
      rune.reset();
    }
  }

  std::string string_representation() const
  {
    std::string rune_str;
    std::string rune_val_str;

    for ( const auto& rune: slot )
    {
      char rune_letter;
      if ( rune.is_ready() ) {
        rune_letter = 'F';
      } else if ( rune.is_depleted() ) {
        rune_letter = 'd';
      } else {
        rune_letter = 'r';
      }

      std::string rune_val = util::to_string( rune.value, 2 );

      rune_str += rune_letter;
      rune_val_str += '[' + rune_val + ']';
    }
    return rune_str + " " + rune_val_str;
  }

  // Return the number of runes in specific state
  unsigned runes_in_state( rune_state s ) const
  {
    return std::accumulate( slot.begin(), slot.end(), 0U,
        [ s ]( const unsigned& v, const rune_t& r ) { return v + ( r.state == s ); });
  }

  // Return the first rune in a specific state. If no rune in specific state found, return nullptr.
  rune_t* first_rune_in_state( rune_state s )
  {
    auto it = range::find_if( slot, [ s ]( const rune_t& rune ) { return rune.state == s; } );
    if ( it != slot.end() )
    {
      return &(*it);
    }

    return nullptr;
  }

  unsigned runes_regenerating() const
  { return runes_in_state( STATE_REGENERATING ); }

  unsigned runes_depleted() const
  { return runes_in_state( STATE_DEPLETED ); }

  unsigned runes_full() const
  { return runes_in_state( STATE_FULL ); }

  rune_t* first_depleted_rune()
  { return first_rune_in_state( STATE_DEPLETED ); }

  rune_t* first_regenerating_rune()
  { return first_rune_in_state( STATE_REGENERATING ); }

  rune_t* first_full_rune()
  { return first_rune_in_state( STATE_FULL ); }

  void consume( unsigned runes );
};

// ==========================================================================
// Death Knight
// ==========================================================================

struct death_knight_td_t : public actor_target_data_t {
  dot_t* dots_blood_plague;
  dot_t* dots_breath_of_sindragosa;
  dot_t* dots_death_and_decay;
  dot_t* dots_frost_fever;
  dot_t* dots_soul_reaper;
  dot_t* dots_defile;

  debuff_t* debuffs_razorice;

  int diseases() const {
    int disease_count = 0;
    if ( dots_blood_plague -> is_ticking() ) disease_count++;
    if ( dots_frost_fever  -> is_ticking() ) disease_count++;
    return disease_count;
  }

  death_knight_td_t( player_t* target, death_knight_t* death_knight );
};

struct death_knight_t : public player_t {
public:
  // Active
  double runic_power_decay_rate;
  double fallen_crusader, fallen_crusader_rppm;
  double antimagic_shell_absorbed;

  stats_t*  antimagic_shell;

  // Buffs
  struct buffs_t {
    buff_t* army_of_the_dead;
    buff_t* antimagic_shell;
    buff_t* bone_wall;
    buff_t* bone_shield;
    buff_t* crimson_scourge;
    buff_t* dancing_rune_weapon;
    buff_t* dark_transformation;
    buff_t* deaths_advance;
    buff_t* deathbringer;
    buff_t* icebound_fortitude;
    buff_t* killing_machine;
    buff_t* pillar_of_frost;
    buff_t* rime;
    buff_t* runic_corruption;
    buff_t* shadow_infusion;
    buff_t* sudden_doom;
    buff_t* vampiric_blood;
    buff_t* will_of_the_necropolis;

    absorb_buff_t* blood_shield;
    buff_t* rune_tap;
    stat_buff_t* riposte;
    buff_t* shadow_of_death;

  } buffs;

  struct runeforge_t {
    buff_t* rune_of_the_fallen_crusader;
    buff_t* rune_of_the_stoneskin_gargoyle;
  } runeforge;

  // Cooldowns
  struct cooldowns_t {
    cooldown_t* antimagic_shell;
    cooldown_t* bone_shield_icd;
    cooldown_t* vampiric_blood;
  } cooldown;

  // Active Spells
  struct active_spells_t {
    spell_t* blood_plague;
    spell_t* frost_fever;
    spell_t* necrosis;
  } active_spells;

  // Gains
  struct gains_t {
    gain_t* antimagic_shell;
    gain_t* blood_rites;
    gain_t* butchery;
    gain_t* chill_of_the_grave;
    gain_t* power_refund;
    gain_t* rune;
    gain_t* rc;
    gain_t* rune_unknown;
    gain_t* runic_empowerment;
    gain_t* empower_rune_weapon;
    gain_t* blood_tap;
    gain_t* veteran_of_the_third_war;
  } gains;

  // Specialization
  struct specialization_t {
    // Generic
    const spell_data_t* plate_specialization;

    // Blood
    const spell_data_t* bladed_armor;
    const spell_data_t* blood_rites;
    const spell_data_t* veteran_of_the_third_war;
    const spell_data_t* scarlet_fever;
    const spell_data_t* crimson_scourge;
    const spell_data_t* sanguine_fortitude;
    const spell_data_t* will_of_the_necropolis;
    const spell_data_t* riposte;
    const spell_data_t* runic_strikes;

    // Frost
    const spell_data_t* blood_of_the_north;
    const spell_data_t* icy_talons;
    const spell_data_t* killing_machine;
    const spell_data_t* might_of_the_frozen_wastes;
    const spell_data_t* rime;
    const spell_data_t* threat_of_thassarian;

    // Unholy
    const spell_data_t* ebon_plaguebringer;
    const spell_data_t* master_of_ghouls;
    const spell_data_t* necrosis;
    const spell_data_t* reaping;
    const spell_data_t* shadow_infusion;
    const spell_data_t* sudden_doom;
    const spell_data_t* unholy_might;
  } spec;

  // Mastery
  struct mastery_t {
    const spell_data_t* blood_shield;
    const spell_data_t* frozen_heart;
    const spell_data_t* dreadblade;
  } mastery;

  // Talents
  struct talents_t {
    const spell_data_t* blood_tap;
    const spell_data_t* runic_empowerment;
    const spell_data_t* runic_corruption;

    const spell_data_t* breath_of_sindragosa;
    const spell_data_t* defile;
  } talent;

  // Spells
  struct spells_t {
    const spell_data_t* antimagic_shell;
    const spell_data_t* blood_rites;
    const spell_data_t* deaths_advance;
  } spell;

  // Glyphs
  struct glyphs_t {
    const spell_data_t* chains_of_ice;
    const spell_data_t* ice_reaper;
    const spell_data_t* dancing_rune_weapon;
    const spell_data_t* enduring_infection;
    const spell_data_t* festering_blood;
    const spell_data_t* icebound_fortitude;
    const spell_data_t* outbreak;
    const spell_data_t* regenerative_magic;
    const spell_data_t* shifting_presences;
    const spell_data_t* vampiric_blood;
  } glyph;

  // Pets and Guardians
  struct pets_t
  {
    std::array< pet_t*, 8 > army_ghoul;
    pets::dancing_rune_weapon_pet_t* dancing_rune_weapon;
    pets::ghoul_pet_t* ghoul_pet;
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
    proc_t* ready_rune;
    proc_t* km_natural_expiration;
  } procs;

  // Runes
  runes_t _runes;

  death_knight_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DEATH_KNIGHT, name, r ),
    runic_power_decay_rate(),
    fallen_crusader( 0 ),
    fallen_crusader_rppm( find_spell( 166441 ) -> real_ppm() ),
    antimagic_shell_absorbed( 0.0 ),
    antimagic_shell( nullptr ),
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
    _runes( this )
  {
    range::fill( pets.army_ghoul, nullptr );
    base.distance = 0;

    cooldown.antimagic_shell = get_cooldown( "antimagic_shell" );
    cooldown.bone_shield_icd = get_cooldown( "bone_shield_icd" );
    cooldown.bone_shield_icd -> duration = timespan_t::from_seconds( 1.0 );

    cooldown.vampiric_blood = get_cooldown( "vampiric_blood" );

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  virtual void      init_spells() override;
  virtual void      init_action_list() override;
  virtual bool      init_actions() override;
  virtual void      init_rng() override;
  virtual void      init_base_stats() override;
  virtual void      init_scaling() override;
  virtual void      create_buffs() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_resources( bool force ) override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    composite_melee_speed() const override;
  virtual double    composite_melee_haste() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    composite_rating_multiplier( rating_e ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual double    composite_parry_rating() const override;
  virtual double    composite_parry() const override;
  virtual double    composite_dodge() const override;
  virtual double    composite_leech() const override;
  virtual double    composite_melee_expertise( const weapon_t* ) const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_critical_damage_multiplier() const override;
  virtual double    composite_crit_avoidance() const override;
  virtual double    passive_movement_modifier() const override;
  virtual double    temporary_movement_modifier() const override;
  virtual void      regen( timespan_t periodicity ) override;
  virtual void      reset() override;
  virtual void      arise() override;
  virtual void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_damage_imminent( school_e, dmg_e, action_state_t* ) override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  virtual void      combat_begin() override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual void      create_pets() override;
  virtual void      create_options() override;
  virtual resource_e primary_resource() const override { return RESOURCE_RUNIC_POWER; }
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual void      invalidate_cache( cache_e ) override;
  double resource_loss( resource_e resource_type, double amount, gain_t* g = nullptr, action_t* a = nullptr ) override;

  double    runes_per_second() const;
  void      trigger_runic_empowerment( double rpcost );
  void      trigger_runic_corruption( double rpcost );
  void      trigger_necrosis( const action_state_t* );
  void      apply_diseases( action_state_t* state, unsigned diseases );
  double    ready_runes_count( bool fractional ) const;
  double    runes_cooldown_min( ) const;
  double    runes_cooldown_max( ) const;
  double    runes_cooldown_time( const rune_t& r ) const;
  void      default_apl_blood();
  void      default_apl_frost();
  void      default_apl_unholy();


  target_specific_t<death_knight_td_t> target_data;

  virtual death_knight_td_t* get_target_data( player_t* target ) const override
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
  actor_target_data_t( target, death_knight )
{
  dots_blood_plague    = target -> get_dot( "blood_plague",    death_knight );
  dots_breath_of_sindragosa = target -> get_dot( "breath_of_sindragosa", death_knight );
  dots_death_and_decay = target -> get_dot( "death_and_decay", death_knight );
  dots_frost_fever     = target -> get_dot( "frost_fever",     death_knight );
  dots_soul_reaper     = target -> get_dot( "soul_reaper_dot", death_knight );
  dots_defile          = target -> get_dot( "defile",          death_knight );

  debuffs_razorice = buff_creator_t( *this, "razorice", death_knight -> find_spell( 51714 ) )
                     .period( timespan_t::zero() );
}

// ==========================================================================
// Local Utility Functions
// ==========================================================================
// RUNE UTILITY
// Log rune status ==========================================================

static void log_rune_status( const death_knight_t* p, bool debug = false ) {
  std::string rune_string = p -> _runes.string_representation();

  if ( ! debug )
    p -> sim -> out_log.printf( "%s runes: %s", p -> name(), rune_string.c_str() );
  else
    p -> sim -> out_debug.printf( "%s runes: %s", p -> name(), rune_string.c_str() );
}

static std::pair<int, double> rune_ready_in( const death_knight_t* p )
{
  // Return the (slot, time) for the next rune to come up.
  int fastest_remaining = -1;
  double t = std::numeric_limits<double>::max();

  double rps = 1.0 / 10.0 / p -> cache.attack_haste();
  if ( p -> buffs.runic_corruption -> check() ) {
    rps *= 2.0;
  }

  for ( size_t j = 0; j < MAX_RUNES; ++j ) {
    double ttr = ( 1.0 - (p -> _runes.slot[j]).value ) / rps;
    if (ttr < t) {
      t = ttr;
      fastest_remaining = j;
    }
  }

  return std::pair<int, double>( fastest_remaining, t);
}


static double ready_in( const death_knight_t* p, int n_runes )
{
  // How long until at least n_runes are ready, assuming no rune-using actions are taken.
  if ( p -> sim -> debug )
    log_rune_status( p, true );

  double rps = 1.0 / 10.0 / p -> cache.attack_haste();
  if ( p -> buffs.runic_corruption -> check() ) {
    rps *= 2.0;
  }

  std::vector< double > ready_times;
  for ( size_t j = 0; j < MAX_RUNES; ++j) {
    ready_times.push_back( (1.0 - (p -> _runes.slot[j]).value) / rps);
  }
  std::sort(ready_times.begin(), ready_times.end());

  return ready_times[n_runes];
}

// Select a "random" fully depleted rune ====================================

static int random_depleted_rune( death_knight_t* p )
{
  // TODO: mrdmnd - implement
  int num_depleted = 0;
  int depleted_runes[ MAX_RUNES ] = { 0 };

  for ( size_t j = 0; j < MAX_RUNES; ++j ) {

  }

  if ( num_depleted > 0 )
  {
    if ( p -> sim -> debug ) log_rune_status( p, true );

    return depleted_runes[ ( int ) p -> rng().range( 0, num_depleted ) ];
  }

  return -1;
}

inline void runes_t::consume( unsigned runes )
{
// We should never get there, ready checks should catch resource constraints
#ifndef NDEBUG
  if ( runes_full() < runes )
  {
    assert( 0 );
  }
#endif
  while ( runes-- ) first_full_rune() -> consume();
  if ( dk -> sim -> debug )
  {
    log_rune_status( dk );
  }
}

inline rune_t* rune_t::consume()
{
  rune_t* new_regenerating_rune = nullptr;

  state = STATE_DEPLETED;
  value = 0.0;

  // Immediately update the state of the next regenerating rune, since rune_t::regen_rune presumes
  // that the internal state of each invidual rune is always consistent with the rune regeneration
  // rules
  if ( runes -> runes_regenerating() < MAX_REGENERATING_RUNES )
  {
    rune_t* new_regenerating_rune = runes -> first_depleted_rune();
    new_regenerating_rune -> state = STATE_REGENERATING;
  }

  // Internal state consistency for current rune regeneration rules
  assert( runes -> runes_regenerating() <= MAX_REGENERATING_RUNES );
  assert( runes -> runes_depleted() == MAX_RUNES - runes -> runes_full() - runes -> runes_regenerating() );

  return new_regenerating_rune;
}

inline rune_t* rune_t::fill_rune()
{
  rune_t* new_regenerating_rune = nullptr;

  if ( state != STATE_FULL )
  {
    runes -> dk -> procs.ready_rune -> occur();
  }

  value = 1.0;
  state = STATE_FULL;

  // Update actor rune resources, so we can re-use a lot of the normal resource mechanisms that the
  // sim core offers
  runes -> dk -> resource_gain( RESOURCE_RUNE, 1, runes -> dk -> gains.rune );

  // Immediately update the state of the next regenerating rune, since rune_t::regen_rune presumes
  // that the internal state of each invidual rune is always consistent with the rune regeneration
  // rules
  if ( runes -> runes_depleted() > 0 && runes -> runes_regenerating() < MAX_REGENERATING_RUNES )
  {
    rune_t* new_regenerating_rune = runes -> first_depleted_rune();
    new_regenerating_rune -> state = STATE_REGENERATING;
  }

  // Internal state consistency for current rune regeneration rules
  assert( runes -> runes_regenerating() <= MAX_REGENERATING_RUNES );
  assert( runes -> runes_depleted() == MAX_RUNES - runes -> runes_full() - runes -> runes_regenerating() );

  return new_regenerating_rune;
}

inline void rune_t::regen_rune( timespan_t periodicity, bool rc )
{
  gain_t* gain = rc ? runes -> dk -> gains.rc : runes -> dk -> gains.rune;

  if ( state == STATE_FULL )
  {
    if ( runes -> runes_regenerating() < MAX_REGENERATING_RUNES )
    {
      double regen_amount = periodicity.total_seconds() * runes -> dk -> runes_per_second();
      gain -> add( RESOURCE_RUNE, 0, regen_amount );
    }
    return;
  }
  // Depleted, rune won't be regenerating
  else if ( state == STATE_DEPLETED )
  {
    return;
  }

  double regen_amount = periodicity.total_seconds() * runes -> dk -> runes_per_second();

  double new_value = value + regen_amount;
  double overflow = 0.0;
  if ( new_value > 1.0 )
  {
    overflow = new_value - 1.0;
  }

  rune_t* overflow_rune = nullptr;
  if ( new_value >= 1.0 )
  {
    overflow_rune = fill_rune();
  }
  else
  {
    value = new_value;
  }

  // If we got an overflow rune (filling up this rune caused a depleted rune to become
  // regenerating), oveflow into that one
  if ( overflow_rune )
  {
    overflow_rune -> value += overflow;
    assert( overflow_rune -> value < 1.0 ); // Sanity check, should never happen
    gain -> add( RESOURCE_RUNE, regen_amount, 0 );
  }
  // No depleted runes found, so overflow into the aether
  else
  {
    gain -> add( RESOURCE_RUNE, 0, overflow );
  }

  if ( state == STATE_FULL && runes -> dk -> sim -> log )
    log_rune_status( runes -> dk );
}

namespace pets {

// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_td_t : public actor_target_data_t
{
  dot_t* dots_blood_plague;

  int diseases() const
  {
    int disease_count = 0;
    if ( dots_blood_plague -> is_ticking() ) disease_count++;
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

    dancing_rune_weapon_td_t* td( player_t* t = nullptr ) const
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
      may_miss         = false;
      may_crit         = false;
      hasted_ticks     = false;
    }

    virtual double composite_crit() const override
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

  struct drw_death_strike_t : public drw_melee_attack_t
  {
    drw_death_strike_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "death_strike", p, p -> owner -> find_class_spell( "Death Strike" ) )
    {
      weapon = &( p -> main_hand_weapon );
    }
  };

  struct drw_blood_boil_t: public drw_spell_t
  {
    drw_blood_boil_t( dancing_rune_weapon_pet_t* p ) :
      drw_spell_t( "blood_boil", p, p -> owner -> find_class_spell( "Blood Boil" ) )
    {
      aoe = -1;
    }

    virtual void impact( action_state_t* s ) override
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

    virtual void execute() override
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

    virtual void impact( action_state_t* s ) override
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
      }

      virtual void init() override
      {
        drw_melee_attack_t::init();
        stats = p() -> get_stats( name(), this );
      }
    };

    soul_reaper_dot_t* soul_reaper_dot;

    drw_soul_reaper_t( dancing_rune_weapon_pet_t* p ) :
      drw_melee_attack_t( "soul_reaper", p, p -> owner -> find_spell( 114866 ) ),
      soul_reaper_dot( nullptr )
    {
      weapon = &( p -> main_hand_weapon );

      dynamic_tick_action = true;
      tick_action = new soul_reaper_dot_t( p );
      add_child( tick_action );
    }

    void init() override
    {
      drw_melee_attack_t::init();

      snapshot_flags |= STATE_MUL_TA;
    }

    void tick( dot_t* dot ) override
    {
      int pct = 35;

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

  target_specific_t<dancing_rune_weapon_td_t> target_data;

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
    drw_blood_plague( nullptr ), drw_frost_fever( nullptr ), drw_necrotic_plague( nullptr ),
    drw_death_coil( nullptr ),
    drw_death_siphon( nullptr ), drw_icy_touch( nullptr ),
    drw_outbreak( nullptr ), drw_blood_boil( nullptr ),
    drw_death_strike( nullptr ),
    drw_plague_strike( nullptr ),
    drw_soul_reaper( nullptr ), drw_melee( nullptr )
  {
    main_hand_weapon.type       = WEAPON_BEAST_2H;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level() ) * 3.0;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level() ) * 3.0;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.5 );

    owner_coeff.ap_from_ap = 1/3.0;
    regen_type = REGEN_DISABLED;
  }

  death_knight_t* o() const
  { return static_cast< death_knight_t* >( owner ); }

  dancing_rune_weapon_td_t* td( player_t* t ) const
  { return get_target_data( t ); }

  virtual dancing_rune_weapon_td_t* get_target_data( player_t* target ) const override
  {
    dancing_rune_weapon_td_t*& td = target_data[ target ];
    if ( ! td )
      td = new dancing_rune_weapon_td_t( target, const_cast<dancing_rune_weapon_pet_t*>(this) );
    return td;
  }

  virtual void init_spells() override
  {
    pet_t::init_spells();

    // Kludge of the century to get pointless initialization warnings to
    // go away.
    type = DEATH_KNIGHT; _spec = DEATH_KNIGHT_BLOOD;

    drw_blood_plague  = new drw_blood_plague_t ( this );

    drw_death_coil    = new drw_death_coil_t   ( this );
    drw_outbreak      = new drw_outbreak_t     ( this );
    drw_blood_boil    = new drw_blood_boil_t   ( this );

    drw_death_strike  = new drw_death_strike_t ( this );
    drw_melee         = new drw_melee_t        ( this );

    type = PLAYER_GUARDIAN; _spec = SPEC_NONE;
  }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    pet_t::summon( duration );
    drw_melee -> schedule_execute();
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( o() -> glyph.dancing_rune_weapon -> ok() )
      m *= 1.0 + o() -> glyph.dancing_rune_weapon -> effectN( 2 ).percent();

    return m;
  }
};

dancing_rune_weapon_td_t::dancing_rune_weapon_td_t( player_t* target, dancing_rune_weapon_pet_t* drw ) :
  actor_target_data_t( target, drw )
{
  dots_blood_plague    = target -> get_dot( "blood_plague",        drw );
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

  double composite_player_multiplier( school_e school ) const override
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
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level() ) * 0.5;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level() ) * 0.5;
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

    void init() override
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

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready() override
    {
      return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
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

  virtual void init_base_stats() override
  {
    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;

    owner_coeff.ap_from_ap = 0.0415;
  }

  virtual resource_e primary_resource() const override { return RESOURCE_ENERGY; }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "auto_attack"    ) return new  army_ghoul_pet_auto_melee_attack_t( this );
    if ( name == "claw"           ) return new         army_ghoul_pet_claw_t( this );

    return pet_t::create_action( name, options_str );
  }

  timespan_t available() const override
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

    result_e calculate_result( action_state_t* /* s */ ) const override
    { return RESULT_HIT; }

    block_result_e calculate_block_result( action_state_t* ) const override
    { return BLOCK_RESULT_UNBLOCKED; }

    void execute() override
    {
      action_t::execute();
      executed = true;
    }

    void cancel() override
    {
      action_t::cancel();
      executed = false;
    }

    // ~3 seconds seems to be the optimal initial delay
    // FIXME: Verify if behavior still exists on 5.3 PTR
    timespan_t execute_time() const override
    { return timespan_t::from_seconds( const_cast<travel_t*>( this ) -> rng().gauss( 2.9, 0.2 ) ); }

    bool ready() override
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

    double composite_da_multiplier( const action_state_t* state ) const override
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

  virtual void init_base_stats() override
  {
    action_list_str = "travel/gargoyle_strike";

    // As per Blizzard
    owner_coeff.sp_from_ap = 0.46625;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
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
  // Unholy T18 4pc buff
  buff_t* crazed_monstrosity;

  ghoul_pet_t( sim_t* sim, death_knight_t* owner, const std::string& name, bool guardian ) :
    death_knight_pet_t( sim, owner, name, guardian ),
    crazed_monstrosity( nullptr )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level() ) * 0.8;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level() ) * 0.8;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    action_list_str = "auto_attack/monstrous_blow/sweeping_claws/claw";
  }

  struct ghoul_pet_melee_attack_t : public melee_attack_t
  {
    ghoul_pet_melee_attack_t( const char* n, ghoul_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      melee_attack_t( n, p, s )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
    }

    virtual double action_multiplier() const override
    {
      double am = melee_attack_t::action_multiplier();

      ghoul_pet_t* p = debug_cast<ghoul_pet_t*>( player );

      am *= 1.0 + p -> o() -> buffs.shadow_infusion -> stack() * p -> o() -> buffs.shadow_infusion -> data().effectN( 1 ).percent();

      if ( p -> o() -> buffs.dark_transformation -> up() )
      {
        double dtb = p -> o() -> buffs.dark_transformation -> data().effectN( 1 ).percent();

        dtb += p -> o() -> sets.set( DEATH_KNIGHT_UNHOLY, T17, B2 ) -> effectN( 2 ).percent();

        am *= 1.0 + dtb;
      }

      am *= 0.8;

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

    virtual void execute() override
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready() override
    {
      return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
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

  struct ghoul_pet_monstrous_blow_t: public ghoul_pet_melee_attack_t
  {
    ghoul_pet_monstrous_blow_t( ghoul_pet_t* p ):
      ghoul_pet_melee_attack_t( "monstrous_blow", p, p -> find_spell( 91797 ) )
    {
      special = true;
    }

    bool ready() override
    {
      ghoul_pet_t* p = debug_cast<ghoul_pet_t*>( player );

      if ( p -> o() -> buffs.dark_transformation -> check() ) // Only usable while dark transformed.
        return ghoul_pet_melee_attack_t::ready();

      return false;
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

    virtual bool ready() override
    {
      death_knight_t* dk = debug_cast<ghoul_pet_t*>( player ) -> o();

      if ( ! dk -> buffs.dark_transformation -> check() )
        return false;

      return ghoul_pet_melee_attack_t::ready();
    }
  };

  virtual void init_base_stats() override
  {
    resources.base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
    owner_coeff.ap_from_ap = 0.5;
  }

  //Ghoul regen doesn't benefit from haste (even bloodlust/heroism)
  virtual resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "auto_attack"    ) return new    ghoul_pet_auto_melee_attack_t( this );
    if ( name == "claw"           ) return new           ghoul_pet_claw_t( this );
    if ( name == "sweeping_claws" ) return new ghoul_pet_sweeping_claws_t( this );
    if ( name == "monstrous_blow" ) return new ghoul_pet_monstrous_blow_t( this );

    return pet_t::create_action( name, options_str );
  }

  void create_buffs() override
  {
    pet_t::create_buffs();

    crazed_monstrosity = buff_creator_t( this, "crazed_monstrosity", find_spell( 187970 ) )
                         .duration( find_spell( 187981 ) -> duration() ) // Grab duration from the player's spell
                         .chance( owner -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T18, B4 ) );
  }

  timespan_t available() const override
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

  double composite_melee_speed() const override
  {
    double s = pet_t::composite_melee_speed();

    if ( crazed_monstrosity -> up() )
    {
      s *= 1.0 / ( 1.0 + crazed_monstrosity -> data().effectN( 3 ).percent() );
    }

    return s;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( crazed_monstrosity -> up() )
    {
      m *= 1.0 + crazed_monstrosity -> data().effectN( 2 ).percent();
    }

    return m;
  }
};

} // namespace pets

namespace { // UNNAMED NAMESPACE

// Template for common death knight action code. See priest_action_t.
template <class Base>
struct death_knight_action_t : public Base
{
  typedef Base action_base_t;
  typedef death_knight_action_t base_t;

  death_knight_action_t( const std::string& n, death_knight_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    action_base_t( n, p, s )
  {
    this -> may_crit   = true;
    this -> may_glance = false;
  }

  death_knight_t* p() const
  { return static_cast< death_knight_t* >( this -> player ); }

  death_knight_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  void consume_resource() override
  {
    action_base_t::consume_resource();

    // Death Knights have unique snowflake mechanism for RP energize. Base actions indicate the
    // amount as a negative value resource cost in spell data, so abuse that.
    if ( this -> base_costs[ RESOURCE_RUNIC_POWER ] < 0 )
    {
      this -> player -> resource_gain( RESOURCE_RUNIC_POWER,
          std::fabs( this -> base_costs[ RESOURCE_RUNIC_POWER ] ), nullptr, this );
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = action_base_t::composite_target_multiplier( t );

    if ( dbc::is_school( this -> school, SCHOOL_FROST ) )
    {
      death_knight_td_t* tdata = td( t );
      double debuff = tdata -> debuffs_razorice -> data().effectN( 1 ).percent();

      m *= 1.0 + tdata -> debuffs_razorice -> check() * debuff;

    }

    return m;
  }

  expr_t* create_expression( const std::string& name_str ) override;
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

  virtual void   consume_resource() override;
  virtual void   execute() override;
  virtual void   impact( action_state_t* state ) override;

  virtual double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_da_multiplier( state );

    if ( player -> main_hand_weapon.group() == WEAPON_2H )
    {
      // Autoattacks use a separate effect for the +damage%
      m *= 1.0 + p() -> spec.might_of_the_frozen_wastes -> effectN( special ? 2 : 3 ).percent();
    }

    return m;
  }

  virtual bool   ready() override;
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

  virtual void   consume_resource() override;
  virtual void   execute() override;
  virtual void   impact( action_state_t* state ) override;

  virtual double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = base_t::composite_da_multiplier( state );

    return m;
  }

  virtual bool   ready() override;
};

struct death_knight_heal_t : public death_knight_action_t<heal_t>
{
  death_knight_heal_t( const std::string& n, death_knight_t* p,
                       const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
  }
};

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
}

// death_knight_melee_attack_t::execute() ===================================

void death_knight_melee_attack_t::execute()
{
  base_t::execute();

  if ( ! result_is_hit( execute_state -> result ) && ! always_consume && resource_consumed > 0 )
    p() -> resource_gain( RESOURCE_RUNIC_POWER, resource_consumed * RUNIC_POWER_REFUND, p() -> gains.power_refund );

  if ( result_is_hit( execute_state -> result ) &&  td( execute_state -> target ) -> dots_blood_plague -> is_ticking())
    p() -> buffs.crimson_scourge -> trigger();
}

// death_knight_melee_attack_t::impact() ====================================

void death_knight_melee_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );
}

// death_knight_melee_attack_t::ready() =====================================

bool death_knight_melee_attack_t::ready()
{
  if ( ! base_t::ready() )
    return false;

  // something like return p() -> ready_runes_count( false ) > rune_cost;
  // TODO: mrdmnd fix this!
  return true;
}


// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================


// death_knight_spell_t::consume_resource() =================================

void death_knight_spell_t::consume_resource()
{
  base_t::consume_resource();
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
}


// death_knight_spell_t::ready() ============================================

bool death_knight_spell_t::ready()
{
  if ( ! base_t::ready() )
    return false;

    // TODO: mrdmnd - fix this
  //return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
  // really more like runes_ready_count > rune_count
  return true;
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

 void reset() override
  {
    death_knight_melee_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t t = death_knight_melee_attack_t::execute_time();

    if ( first )
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    else
      return t;
  }

  virtual void execute() override
  {
    if ( first )
      first = false;

    death_knight_melee_attack_t::execute();
  }


  virtual void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( p() -> spec.blood_rites -> ok() &&
         weapon -> group() == WEAPON_2H )
    {
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
        }

      }

      // Killing Machine is 6 PPM
      if ( p() -> spec.killing_machine -> ok() )
      {
        p() -> buffs.killing_machine -> trigger( 1, buff_t::DEFAULT_VALUE(), weapon -> proc_chance_on_swing( 6 ) );
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
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );
    ignore_false_positive = true;

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

  virtual void execute() override
  {
    player -> main_hand_attack -> schedule_execute();
    if ( player -> off_hand_attack )
    {
      player -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;
    return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
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
    parse_options( options_str );

    harmful     = false;
  }

  virtual void schedule_execute( action_state_t* s ) override
  {
    death_knight_spell_t::schedule_execute( s );

    p() -> buffs.army_of_the_dead -> trigger( 1, p() -> cache.dodge() + p() -> cache.parry() );
  }

  virtual void execute() override
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
      for ( size_t i = 0; i < MAX_RUNES; ++i )
        p() -> _runes.slot[ i ].regen_rune( timespan_t::from_seconds( 5.0 ) );

      //simulate RP decay for that 5 seconds
      p() -> resource_loss( RESOURCE_RUNIC_POWER, p() -> runic_power_decay_rate * 5, nullptr, nullptr );
    }
    else
    {
      // TODO: DBC
      for ( int i = 0; i < 8; i++ )
        p() -> pets.army_ghoul[ i ] -> summon( timespan_t::from_seconds( 40 ) );
    }
  }

  virtual bool ready() override
  {
    if ( p() -> pets.army_ghoul[ 0 ] && ! p() -> pets.army_ghoul[ 0 ] -> is_sleeping() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Diseases =================================================================

struct disease_t : public death_knight_spell_t
{

  disease_t( death_knight_t* p, const std::string& name, unsigned spell_id ) :
    death_knight_spell_t( name, p, p -> find_spell( spell_id ) )
  {
    tick_may_crit    = true;
    background       = true;
    may_miss         = false;
    may_crit         = false;
    hasted_ticks     = false;

    // TODO-WOD: Check if multiplicative
    base_multiplier *= 1.0 + p -> spec.ebon_plaguebringer -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p -> spec.crimson_scourge -> effectN( 1 ).percent();
    if ( p -> glyph.enduring_infection -> ok() )
    {
      base_multiplier += p -> find_spell( 58671 ) -> effectN( 1 ).percent();
    }

  }

  // WODO-TOD: Crit suppression hijinks?
  virtual double composite_crit() const override
  { return action_t::composite_crit() + player -> cache.attack_crit(); }

  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    death_knight_spell_t::assess_damage( type, s );

  }
};

// Blood Plague =============================================================

struct blood_plague_t : public disease_t
{
  blood_plague_t( death_knight_t* p ) :
    disease_t( p, "blood_plague", 55078 )
  { }
};

// Frost Fever ==============================================================

struct frost_fever_t : public disease_t
{
  frost_fever_t( death_knight_t* p ) :
    disease_t( p, "frost_fever", 55095 )
  { }
};

// Virulent Plague ==========================================================

struct virulent_plague_t : public disease_t
{
  virulent_plague_t( death_knight_t* p ) :
    disease_t( p, "virulent_plague", 191587) // TODO: mrdmnd - check that this id is correct
  { }
};

// Wandering Plague =========================================================
// T18 Class trinket
struct wandering_plague_t : public death_knight_spell_t
{
  wandering_plague_t( death_knight_t* p ) :
    death_knight_spell_t( "wandering_plague", p )
  {
    school = SCHOOL_SHADOW;

    background = split_aoe_damage = true;
    callbacks = may_miss = may_crit = false;

    aoe = -1;
  }

  void init() override
  {
    death_knight_spell_t::init();

    snapshot_flags = update_flags = 0;
  }
};

// Necrosis =================================================================

struct necrosis_t : public death_knight_spell_t
{
  necrosis_t( death_knight_t* player ) :
    death_knight_spell_t( "necrosis", player, player -> spec.necrosis -> effectN( 2 ).trigger() )
  {
    background = true;
    callbacks = false;
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

  void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( ! p() -> buffs.dark_transformation -> check() &&
         p() -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T17, B2 )  )
      p() -> buffs.shadow_infusion -> trigger( p() -> sets.set( DEATH_KNIGHT_UNHOLY, T17, B2 ) -> effectN( 1 ).base_value() );
  }
};

struct soul_reaper_t : public death_knight_melee_attack_t
{
  soul_reaper_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "soul_reaper", p, p -> specialization() != DEATH_KNIGHT_UNHOLY ? p -> find_specialization_spell( "Soul Reaper" ) : p -> find_spell( 130736 ) )
  {
    parse_options( options_str );
    special   = true;

    weapon = &( p -> main_hand_weapon );

    dynamic_tick_action = true;
    tick_action = new soul_reaper_dot_t( p );
  }

  double false_positive_pct() const override
  {
    if ( target -> health_percentage() > 40 )
      return 0;
    else
      return death_knight_melee_attack_t::false_positive_pct();
  }

  virtual double composite_crit() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit();
    if ( player -> sets.has_set_bonus( SET_MELEE, T15, B4 ) && p() -> buffs.killing_machine -> check() )
      cc += p() -> buffs.killing_machine -> value();
    return cc;
  }

  void init() override
  {
    death_knight_melee_attack_t::init();

    snapshot_flags |= STATE_MUL_TA;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_melee_attack_t::impact( state );

    p() -> trigger_necrosis( state );
  }

  virtual void execute() override
  {
    death_knight_melee_attack_t::execute();
    if ( player -> sets.has_set_bonus( SET_MELEE, T15, B4 ) && p() -> buffs.killing_machine -> check() )
    {
      p() -> procs.sr_killing_machine -> occur();
      p() -> buffs.killing_machine -> expire();
    }

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_soul_reaper -> execute();
  }

  void tick( dot_t* dot ) override
  {
    int pct = p() -> sets.has_set_bonus( SET_MELEE, T15, B4 ) ? p() -> sets.set( SET_MELEE, T15, B4 ) -> effectN( 1 ).base_value() : 35;

    if ( dot -> state -> target -> health_percentage() <= pct )
      death_knight_melee_attack_t::tick( dot );
  }
};

// Blood Tap ================================================================

struct blood_tap_t : public death_knight_spell_t
{

  blood_tap_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_tap", p, p -> find_talent_spell( "Blood Tap" ) )

  {
    parse_options( options_str );
    harmful   = false;
  }
/*
  void execute() override
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

  bool ready() override
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
  */
};

// Bone Shield ==============================================================

struct bone_shield_t : public death_knight_spell_t
{
  bone_shield_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "bone_shield", p, p -> find_specialization_spell( "Bone Shield" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
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

    parse_options( options_str );
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.dancing_rune_weapon -> trigger();
    p() -> pets.dancing_rune_weapon -> summon( data().duration() );
  }
};

// Dark Command =======================================================================

struct dark_command_t: public death_knight_spell_t
{
  dark_command_t( death_knight_t* p, const std::string& options_str ):
    death_knight_spell_t( "dark_command", p, p -> find_class_spell( "Dark Command" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    use_off_gcd = true;
  }

  virtual void impact( action_state_t* s ) override
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
    death_knight_spell_t( "dark_transformation", p, p -> find_specialization_spell( "Dark Transformation" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.dark_transformation -> trigger();
    p() -> buffs.shadow_infusion -> expire();
    p() -> pets.ghoul_pet -> crazed_monstrosity -> trigger();
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.shadow_infusion -> check() != p() -> buffs.shadow_infusion -> max_stack() )
      return false;

    if ( p() -> pets.ghoul_pet -> is_sleeping() )
    {
      return false;
    }

    return death_knight_spell_t::ready();
  }
};

// Death and Decay ==========================================================

struct death_and_decay_t : public death_knight_spell_t
{
  death_and_decay_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_and_decay", p, p -> find_class_spell( "Death and Decay" ) )
  {
    parse_options( options_str );

    aoe              = -1;
    attack_power_mod.tick = p -> find_spell( 52212 ) -> effectN( 1 ).ap_coeff();
    base_tick_time   = timespan_t::from_seconds( 1.0 );
    dot_duration = data().duration(); // 11 with tick_zero
    tick_may_crit = tick_zero = true;
    hasted_ticks     = false;
    ignore_false_positive = true;
    ground_aoe = true;
  }

  virtual void consume_resource() override
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const override
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.crimson_scourge -> up() )
      p() -> buffs.crimson_scourge -> expire();
  }

  virtual void impact( action_state_t* s ) override
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

  virtual bool ready() override
  {
    if ( ! spell_t::ready() )
      return false;

    if ( p() -> talent.defile -> ok() )
      return false;

    if ( p() -> buffs.crimson_scourge -> check() )
        // TODO: mrdmnd
      //return group_runes( p(), 0, 0, 0, 0, use );
      return true;
    else
      // TODO: mrdmnd
      // return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
      return true;
  }
};

// Defile ==================================================================

struct defile_t : public death_knight_spell_t
{
  cooldown_t* dnd_cd;

  defile_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "defile", p, p -> find_talent_spell( "Defile" ) ),
    dnd_cd( p -> get_cooldown( "death_and_decay" ) )
  {
    parse_options( options_str );

    aoe = -1;
    base_dd_min = base_dd_max = 0;
    school = p -> find_spell( 156000 ) -> get_school_type();
    attack_power_mod.tick = p -> find_spell( 156000 ) -> effectN( 1 ).ap_coeff();
    radius =  data().effectN( 1 ).radius();
    dot_duration = data().duration();
    tick_may_crit = true;
    hasted_ticks = tick_zero = false;
    ignore_false_positive = true;
    ground_aoe = true;
  }

  // Defile very likely counts as direct damage, as it procs certain trinkets that are flagged for
  // "aoe harmful spell", but not "periodic".
  dmg_e amount_type( const action_state_t*, bool ) const override
  { return DMG_DIRECT; }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = death_knight_spell_t::composite_ta_multiplier( state );

    dot_t* dot = find_dot( state -> target );

    if ( dot )
    {
      m *= std::pow( 1.0 + data().effectN( 2 ).percent() / 100, std::max( dot -> current_tick - 1, 0 ) );
    }

    return m;
  }

  virtual void consume_resource() override
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const override
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.crimson_scourge -> up() )
      p() -> buffs.crimson_scourge -> expire();
  }

  virtual void update_ready( timespan_t cd_duration ) override
  {
    death_knight_spell_t::update_ready( cd_duration );

    dnd_cd -> start( data().cooldown() );
  }

  virtual void impact( action_state_t* s ) override
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

  virtual bool ready() override
  {
    if ( ! spell_t::ready() )
      return false;

    if ( p() -> buffs.crimson_scourge -> check() )
      //return group_runes( p(), 0, 0, 0, 0, use );
      // TODO
      return true;
    else
      //return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
      // TODO
      return true;
  }
};

// Death Coil ===============================================================


struct death_coil_t : public death_knight_spell_t
{

  death_coil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", p, p -> find_class_spell( "Death Coil" ) )
  {
    parse_options( options_str );

    attack_power_mod.direct = 0.80;

  }

  virtual double cost() const override
  {
    if ( p() -> buffs.sudden_doom -> check() )
      return 0;

    return death_knight_spell_t::cost();
  }

  void impact( action_state_t* state ) override {
    death_knight_spell_t::impact( state );
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.sudden_doom -> decrement();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_death_coil -> execute();

    double dc_rpcost = base_costs[ RESOURCE_RUNIC_POWER ];
    if ( player -> sets.has_set_bonus( DEATH_KNIGHT_UNHOLY, T18, B2 ) )
    {
      dc_rpcost += base_costs[ RESOURCE_RUNIC_POWER ];
    }

    if ( p() -> specialization() == DEATH_KNIGHT_BLOOD )
      p() -> buffs.shadow_of_death -> trigger();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> trigger_runic_empowerment( base_costs[ RESOURCE_RUNIC_POWER ] );
      p() -> trigger_runic_corruption( base_costs[ RESOURCE_RUNIC_POWER ] );
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
  void absorb_used( double ) override
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
    may_miss = may_crit = callbacks = false;
    background = proc = true;
  }

  // Self only so we can do this in a simple way
  absorb_buff_t* create_buff( const action_state_t* ) override
  { return debug_cast<death_knight_t*>( player ) -> buffs.blood_shield; }

  void init() override
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
    blood_shield( p -> specialization() == DEATH_KNIGHT_BLOOD ? new blood_shield_t( p ) : nullptr )
  {
    may_crit   = callbacks = false;
    background = true;
    target     = p;
  }

  void impact( action_state_t* state ) override
  {
    death_knight_heal_t::impact( state );

    trigger_blood_shield( state );
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
    oh_attack( nullptr ), heal( new death_strike_heal_t( p ) )
  {
    parse_options( options_str );
    special = true;
    may_parry = false;
    base_multiplier = 1.0 + p -> spec.veteran_of_the_third_war -> effectN( 7 ).percent();

    always_consume = true; // Death Strike always consumes runes, even if doesn't hit

    weapon = &( p -> main_hand_weapon );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> spec.might_of_the_frozen_wastes -> ok() )
        oh_attack = new death_strike_offhand_t( p );
    }
  }

  virtual void consume_resource() override
  {
    if ( p() -> buffs.deathbringer -> check() )
      return;

    death_knight_melee_attack_t::consume_resource();
  }

  virtual double cost() const override
  {
    if ( p() -> buffs.deathbringer -> check() )
      return 0;

    return death_knight_melee_attack_t::cost();
  }

  virtual void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( oh_attack )
      oh_attack -> execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_death_strike -> execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      heal -> schedule_execute();
    }

    p() -> buffs.deathbringer -> decrement();
  }

  virtual bool ready() override
  {
    if ( ! melee_attack_t::ready() )
      return false;

    if ( p() -> buffs.deathbringer -> check() )
      //return group_runes( p(), 0, 0, 0, 0, use );
      //todo: mrdmnd
      return true;
    else
      //return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
      //TODO:mrdmnd
      return true;
  }
};

// Empower Rune Weapon ======================================================

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "empower_rune_weapon", p, p -> find_spell( 47568 ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    double erw_gain = 0.0;
    double erw_over = 0.0;
    for ( size_t i = 0; i < MAX_RUNES; ++i )
    {
      rune_t& r = p() -> _runes.slot[ i ];
      erw_gain += 1 - r.value;
      erw_over += r.value;
      r.reset();
    }
    p() -> gains.empower_rune_weapon -> add( RESOURCE_RUNE, erw_gain, erw_over );
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_melee_attack_t
{
  festering_strike_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "festering_strike", p, p -> find_specialization_spell( "Festering Strike" ) )
  {
    parse_options( options_str );
  }

  virtual void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> dots_blood_plague -> extend_duration( timespan_t::from_seconds( data().effectN( 3 ).base_value() ), 0 );
      td( s -> target ) -> dots_frost_fever  -> extend_duration( timespan_t::from_seconds( data().effectN( 3 ).base_value() ), 0 );
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
  }

  virtual double composite_crit() const override
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
    oh_attack( nullptr )
  {
    special = true;
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();

    parse_options( options_str );

    weapon     = &( p -> main_hand_weapon );

    if ( p -> spec.threat_of_thassarian -> ok() && p -> off_hand_weapon.type != WEAPON_NONE )
    {
      base_multiplier *= 1.0 + p -> spec.threat_of_thassarian -> effectN( 3 ).percent();

      if ( p -> spec.might_of_the_frozen_wastes -> ok() )
        oh_attack = new frost_strike_offhand_t( p );
    }
  }

  virtual double cost() const override
  {
    double c = death_knight_melee_attack_t::cost();

    return c;
  }

  virtual void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( oh_attack )
        oh_attack -> execute();

      if ( p() -> buffs.killing_machine -> check() )
        p() -> procs.fs_killing_machine -> occur();

      if ( ! p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T18, B4 ) || 
           ( p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T18, B4 ) &&
             ! p() -> rng().roll( player -> sets.set( DEATH_KNIGHT_FROST, T18, B4 ) -> effectN( 1 ).percent() ) ) )
      {
        p() -> buffs.killing_machine -> expire();
      }

      p() -> trigger_runic_empowerment( resource_consumed );
      p() -> trigger_runic_corruption( resource_consumed );
    }
  }

  virtual double composite_crit() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }
};

// Howling Blast ============================================================

struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", p, p -> find_specialization_spell( "Howling Blast" ) )
  {
    parse_options( options_str );

    aoe                 = -1;
    base_aoe_multiplier = data().effectN( 1 ).percent();

    assert( p -> active_spells.frost_fever );
  }

  virtual void consume_resource() override
  {
    if ( p() -> buffs.rime -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const override
  {
    // Rime also prevents getting RP because there are no runes used!
    if ( p() -> buffs.rime -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.rime -> decrement();
  }

  virtual void impact( action_state_t* s ) override
  {
    death_knight_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> apply_diseases( s, DISEASE_FROST_FEVER );

    // Howling blast has no travel time, so rime is still up at this point, as
    // impact() will be called in execute().
  }

  virtual bool ready() override
  {
    if ( p() -> buffs.rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      //cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      //cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Chains of Ice ============================================================

struct chains_of_ice_t : public death_knight_spell_t
{
  const spell_data_t* pvp_bonus;

  chains_of_ice_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "chains_of_ice", p, p -> find_class_spell( "Chains of Ice" ) ),
    pvp_bonus( p -> find_spell( 62459 ) )
  {
    parse_options( options_str );

    int exclusivity_check = 0;

    for ( size_t i = 0, end = sizeof_array( p -> items[ SLOT_HANDS ].parsed.data.id_spell ); i < end; i++ )
    {
      if ( p -> items[ SLOT_HANDS ].parsed.data.id_spell[ i ] == static_cast<int>( pvp_bonus -> id() ) )
      {
        energize_type     = ENERGIZE_ON_HIT;
        energize_resource = RESOURCE_RUNIC_POWER;
        energize_amount   = pvp_bonus -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RUNIC_POWER );
        break;
      }
    }

    if ( exclusivity_check > 1 )
    {
      sim -> errorf( "Disabling Chains of Ice because multiple exclusive glyphs are affecting it." );
      background = true;
    }
  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
      p() -> apply_diseases( state, DISEASE_FROST_FEVER );
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "mind_freeze", p, p -> find_class_spell( "Mind Freeze" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready() override
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

  virtual double composite_crit() const override
  {
    double cc = death_knight_melee_attack_t::composite_crit();

    cc += p() -> buffs.killing_machine -> value();

    return cc;
  }

  void impact( action_state_t* s ) override
  {
    death_knight_melee_attack_t::impact( s );
  }
};

struct obliterate_t : public death_knight_melee_attack_t
{
  obliterate_offhand_t* oh_attack;

  obliterate_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_melee_attack_t( "obliterate", p, p -> find_class_spell( "Obliterate" ) ),
    oh_attack( nullptr )
  {
    parse_options( options_str );
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

    std::cerr << base_costs[ RESOURCE_RUNE ] << std::endl;
  }

  virtual void execute() override
  {
    death_knight_melee_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( oh_attack )
        oh_attack -> execute();

      if ( p() -> buffs.killing_machine -> check() )
        p() -> procs.oblit_killing_machine -> occur();

      if ( ! p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T18, B4 ) || 
           ( p() -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T18, B4 ) &&
             ! p() -> rng().roll( player -> sets.set( DEATH_KNIGHT_FROST, T18, B4 ) -> effectN( 1 ).percent() ) ) )
      {
        p() -> buffs.killing_machine -> expire();
      }
    }
  }

  virtual void impact( action_state_t* s ) override
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

  virtual double composite_crit() const override
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
    parse_options( options_str );

    may_crit = false;

    cooldown -> duration *= 1.0 + p -> glyph.outbreak -> effectN( 1 ).percent();
    base_costs[ RESOURCE_RUNIC_POWER ] += p -> glyph.outbreak -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> apply_diseases( execute_state, DISEASE_BLOOD_PLAGUE | DISEASE_FROST_FEVER );

      if ( resource_consumed > 0 )
      {
        p() -> trigger_runic_empowerment( resource_consumed );
        p() -> trigger_runic_corruption( resource_consumed );
      }
    }

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_outbreak -> execute();
  }
};

// Blood Boil ==============================================================

struct blood_boil_t : public death_knight_spell_t
{

  blood_boil_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", p, p -> find_class_spell( "Blood Boil" ) )
  {
    parse_options( options_str );


    base_multiplier *= 1.0 + p -> spec.crimson_scourge -> effectN( 1 ).percent();

    aoe = -1;
  }

  virtual void consume_resource() override
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return;

    death_knight_spell_t::consume_resource();
  }

  virtual double cost() const override
  {
    if ( p() -> buffs.crimson_scourge -> check() )
      return 0;
    return death_knight_spell_t::cost();
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    if ( p() -> buffs.dancing_rune_weapon -> check() )
      p() -> pets.dancing_rune_weapon -> drw_blood_boil -> execute();

    if ( p() -> buffs.crimson_scourge -> up() )
      p() -> buffs.crimson_scourge -> expire();

  }

  void impact( action_state_t* state ) override
  {
    death_knight_spell_t::impact( state );

    p() -> trigger_necrosis( state );
  }

  virtual bool ready() override
  {
    if ( ! death_knight_spell_t::ready() )
      return false;

    if ( ( ! p() -> in_combat && ! harmful ) || p() -> buffs.crimson_scourge -> check() )
      //return group_runes( p(), 0, 0, 0, 0, use );
      return true;
    else
      //return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
      return true;
  }
};

// Pillar of Frost ==========================================================

struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", p, p -> find_class_spell( "Pillar of Frost" ) )
  {
    parse_options( options_str );

    harmful = false;

    if ( p -> sets.has_set_bonus( DEATH_KNIGHT_FROST, T17, B2 ) )
    {
      energize_type = ENERGIZE_ON_CAST;
      energize_amount = p -> sets.set( DEATH_KNIGHT_FROST, T17, B2 ) -> effectN( 1 ).trigger() -> effectN( 2 ).resource( RESOURCE_RUNIC_POWER );
      energize_resource = RESOURCE_RUNIC_POWER;
    }
  }

  void execute() override
  {
    death_knight_spell_t::execute();

    p() -> buffs.pillar_of_frost -> trigger();
  }

  bool ready() override
  {
    if ( ! spell_t::ready() )
      return false;

    // Always activate runes, even pre-combat.
    //return group_runes( p(), cost_blood, cost_frost, cost_unholy, cost_death, use );
    return true;
  }
};

// Raise Dead ===============================================================

struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", p, p -> find_specialization_spell( "Raise Dead" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    p() -> pets.ghoul_pet -> summon( timespan_t::zero() );
  }

  virtual bool ready() override
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
    }

    void impact( action_state_t* state ) override
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
    parse_options( options_str );

    special = true;
    base_multiplier *= 1.0 + p -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();

    // TODO-WOD: Do we need to inherit damage or is it a separate roll in WoD?
    add_child( scourge_strike_shadow );
  }

  void impact( action_state_t* state ) override
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
    parse_options( options_str );
    dot_duration = timespan_t::zero();
    harmful = false;
  }

  virtual void execute() override
  {
    death_knight_spell_t::execute();

    timespan_t duration = data().effectN( 3 ).trigger() -> duration();

    p() -> pets.gargoyle -> summon( duration );
  }
};

// Breath of Sindragosa =====================================================

struct breath_of_sindragosa_tick_t: public death_knight_spell_t
{
  action_t* parent;

  breath_of_sindragosa_tick_t( death_knight_t* p, action_t* parent ):
    death_knight_spell_t( "breath_of_sindragosa_tick", p, p -> find_spell( 155166 ) ),
    parent( parent )
  {
    aoe = -1;
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target == target )
      death_knight_spell_t::impact( s );
    else
    {
      double damage = s -> result_amount;
      damage /= execute_state -> n_targets;
      s -> result_amount = damage;
      death_knight_spell_t::impact( s );
    }
  }
};

struct breath_of_sindragosa_t : public death_knight_spell_t
{
  breath_of_sindragosa_t( death_knight_t* p, const std::string& options_str ) :
    death_knight_spell_t( "breath_of_sindragosa", p, p -> find_talent_spell( "Breath of Sindragosa" ) )
  {
    parse_options( options_str );

    may_miss = may_crit = hasted_ticks = callbacks = false;
    tick_zero = dynamic_tick_action = true;

    tick_action = new breath_of_sindragosa_tick_t( p, this );
    school = tick_action -> school;

    for ( size_t idx = 1; idx <= data().power_count(); idx++ )
    {
      const spellpower_data_t& power = data().powerN( idx );
      if ( power.aura_id() == 0 || p -> dbc.spec_by_spell( power.aura_id() ) == p -> specialization() )
      {
        base_costs_per_tick[ power.resource() ] = power.cost_per_tick();
      }
    }
  }

  // Breath of Sindragosa very likely counts as direct damage, as it procs certain trinkets that are
  // flagged for "aoe harmful spell", but not "periodic".
  dmg_e amount_type( const action_state_t*, bool ) const override
  { return DMG_DIRECT; }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return player -> sim -> expected_iteration_time * 2;
  }

  void cancel() override
  {
    death_knight_spell_t::cancel();
    if ( dot_t* d = get_dot( target ) )
      d -> cancel();
  }

  bool consume_cost_per_tick( const dot_t& dot ) override
  {
    bool ret = death_knight_spell_t::consume_cost_per_tick( dot );

    p() -> trigger_runic_empowerment( resource_consumed );
    p() -> trigger_runic_corruption( resource_consumed );

    return ret;
  }

  void execute() override
  {
    dot_t* d = get_dot( target );

    if ( d -> is_ticking() )
      d -> cancel();
    else
    {
      death_knight_spell_t::execute();
    }
  }

  void init() override
  {
    death_knight_spell_t::init();

    snapshot_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA | STATE_MUL_PERSISTENT;
    update_flags |= STATE_MUL_TA | STATE_TGT_MUL_TA;
  }
};

// Anti-magic Shell =========================================================

struct antimagic_shell_buff_t : public buff_t
{
  antimagic_shell_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "antimagic_shell", p -> find_class_spell( "Anti-Magic Shell" ) )
                              .cd( timespan_t::zero() ) )
  { }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    death_knight_t* p = debug_cast< death_knight_t* >( player );
    p -> antimagic_shell_absorbed = 0.0;
    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    death_knight_t* p = debug_cast< death_knight_t* >( player );
    if ( p -> antimagic_shell_absorbed >= 0.0 && p -> glyph.regenerative_magic -> ok() )
    {
      double cd_reduction = -20 + ( 20.0 * p -> antimagic_shell_absorbed / ( p -> resources.max[RESOURCE_HEALTH] * 0.4 ) );
      p -> cooldown.antimagic_shell -> adjust( timespan_t::from_seconds( cd_reduction ) );
    }
  }
};

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
    cooldown = p -> cooldown.antimagic_shell;
    harmful = may_crit = may_miss = false;
    base_dd_min = base_dd_max = 0;
    target = player;

    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "damage", damage ) );
    parse_options( options_str );

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

    /*
    if ( damage > 0 )
      cooldown -> set_recharge_multiplier( 1.0 );
    */

    // Setup an Absorb stats tracker for AMS if it's used "for reals"
    if ( damage == 0 )
    {
      stats -> type = STATS_ABSORB;
      if ( ! p -> antimagic_shell )
        p -> antimagic_shell = stats;
    }
  }

  void execute() override
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

      // AMS generates 2 runic power per percentage max health absorbed.
      p() -> resource_gain( RESOURCE_RUNIC_POWER, util::round( generated * 100.0 * 2.0 ), p() -> gains.antimagic_shell, this );
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
    parse_options( options_str );

    harmful = false;
    base_dd_min = base_dd_max = 0;
  }

  void execute() override
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
    parse_options( options_str );

    harmful = false;

    cooldown -> duration = data().cooldown() * ( 1.0 + p -> glyph.icebound_fortitude -> effectN( 1 ).percent() );
    if ( p -> spec.sanguine_fortitude -> ok() )
      base_costs[ RESOURCE_RUNIC_POWER ] = 0;
  }

  void execute() override
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
    parse_options( options_str );
    cooldown -> charges = data().charges();
    cooldown -> duration = data().charge_cooldown();
    ability_cooldown = p -> get_cooldown( "Rune Tap Ability Cooldown" );
    ability_cooldown -> duration = data().cooldown(); // Can only use it once per second.
    use_off_gcd = true;
  }

  void execute() override
  {
    death_knight_spell_t::execute();
    ability_cooldown -> start();

    p() -> buffs.rune_tap -> trigger();
  }

  bool ready() override
  {
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
    base_pct_heal = data().effectN( 1 ).percent();

    parse_options( options_str );
  }
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
    expr_t( "disease_expr" ), type( t ), bp_expr( nullptr ), ff_expr( nullptr ), np_expr( nullptr ),
    default_value( 0 )
  {
    death_knight_t* p = debug_cast< death_knight_t* >( a -> player );
    bp_expr = a -> target -> get_dot( "blood_plague", p ) -> create_expression( p -> active_spells.blood_plague, expression, true );
    ff_expr = a -> target -> get_dot( "frost_fever", p ) -> create_expression( p -> active_spells.frost_fever, expression, true );

    if ( type == TYPE_MIN )
      default_value = std::numeric_limits<double>::max();
    else if ( type == TYPE_MAX )
      default_value = std::numeric_limits<double>::min();
  }

  double evaluate() override
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
  runic_corruption_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "runic_corruption", p -> find_spell( 51460 ) )
            .chance( p -> talent.runic_corruption -> ok() ).affects_regen( true ) )
  {  }
};

struct vampiric_blood_buff_t : public buff_t
{
  int health_gain;

  vampiric_blood_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "vampiric_blood", p -> find_specialization_spell( "Vampiric Blood" ) ).cd( timespan_t::zero() ) ),
    health_gain ( 0 )
  { }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() ) override
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

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( health_gain > 0 )
    {
      player -> stat_loss( STAT_MAX_HEALTH, health_gain );
      player -> stat_loss( STAT_HEALTH, health_gain );
    }
  }
};

struct killing_machine_buff_t : public buff_t
{
  killing_machine_buff_t( death_knight_t* p ) :
    buff_t( buff_creator_t( p, "killing_machine", p -> find_spell( 51124 ) )
                           .default_value( p -> find_spell( 51124 ) -> effectN( 1 ).percent() )
                           .chance( p -> find_specialization_spell( "Killing Machine" ) -> proc_chance() ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    if ( remaining_duration == timespan_t::zero() )
    {
      death_knight_t* dk = debug_cast<death_knight_t*>( player );
      dk -> procs.km_natural_expiration -> occur();
    }

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

} // UNNAMED NAMESPACE

// Runeforges ==============================================================

void runeforge::razorice_attack( special_effect_t& effect )
{
  struct razorice_attack_t : public death_knight_melee_attack_t
  {
    razorice_attack_t( death_knight_t* player, const std::string& name ) :
      death_knight_melee_attack_t( name, player, player -> find_spell( 50401 ) )
    {
      school      = SCHOOL_FROST;
      may_miss    = callbacks = false;
      background  = proc = true;

      weapon = &( player -> main_hand_weapon );
    }

    // No double dipping to Frost Vulnerability
    double composite_target_multiplier( player_t* t ) const override
    {
      double m = death_knight_melee_attack_t::composite_target_multiplier( t );

      m /= 1.0 + td( t ) -> debuffs_razorice -> check() *
            td( t ) -> debuffs_razorice -> data().effectN( 1 ).percent();

      return m;
    }
  };

  effect.execute_action = new razorice_attack_t( debug_cast<death_knight_t*>( effect.item -> player ), effect.name() );
  effect.proc_chance_ = 1.0;
  new dbc_proc_callback_t( effect.item, effect );
}

void runeforge::razorice_debuff( special_effect_t& effect )
{
  struct razorice_callback_t : public dbc_proc_callback_t
  {
    razorice_callback_t( const special_effect_t& effect ) :
     dbc_proc_callback_t( effect.item, effect )
    { 
    }

    void execute( action_t* a, action_state_t* state ) override
    {
      debug_cast< death_knight_t* >( a -> player ) -> get_target_data( state -> target ) -> debuffs_razorice -> trigger();
      if ( a -> sim -> current_time() < timespan_t::from_seconds( 0.01 ) )
        debug_cast< death_knight_t* >( a -> player ) -> get_target_data( state -> target ) -> debuffs_razorice -> constant = false;
    }
  };

  new razorice_callback_t( effect );
}

void runeforge::fallen_crusader( special_effect_t& effect )
{
  // Fallen Crusader buff is shared between mh/oh
  buff_t* b = buff_t::find( effect.item -> player, "unholy_strength" );
  if ( ! b )
    return;

  action_t* heal = effect.item -> player -> find_action( "unholy_strength" );
  if ( ! heal )
  {
    struct fallen_crusader_heal_t : public death_knight_heal_t
    {
      fallen_crusader_heal_t( death_knight_t* dk, const spell_data_t* data ) :
        death_knight_heal_t( "unholy_strength", dk, data )
      {
        background = true;
        target = player;
        callbacks = may_crit = false;
        base_pct_heal = data -> effectN( 2 ).percent();
      }

      // Procs by default target the target of the action that procced them.
      void execute() override
      {
        target = player;

        death_knight_heal_t::execute();
      }
    };

    heal = new fallen_crusader_heal_t( debug_cast< death_knight_t* >( effect.item -> player ), &b -> data() );
  }

  const death_knight_t* dk = debug_cast<const death_knight_t*>( effect.item -> player );

  effect.ppm_ = -1.0 * dk -> fallen_crusader_rppm;
  effect.custom_buff = b;
  effect.execute_action = heal;

  new dbc_proc_callback_t( effect.item, effect );
}

void runeforge::stoneskin_gargoyle( special_effect_t& effect )
{
  death_knight_t* p = debug_cast<death_knight_t*>( effect.item -> player );
  p -> runeforge.rune_of_the_stoneskin_gargoyle -> default_chance = 1.0;
}

double death_knight_t::resource_loss( resource_e resource_type, double amount, gain_t* g, action_t* a )
{
  double actual_amount = player_t::resource_loss( resource_type, amount, g, a );
  if ( resource_type == RESOURCE_RUNE )
  {
    _runes.consume( amount );
    // Ensure rune state is consistent with the actor resource state for runes
    assert( _runes.runes_full() == resources.current[ RESOURCE_RUNE ] );
  }

  return actual_amount;
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
}

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "antimagic_shell"          ) return new antimagic_shell_t          ( this, options_str );
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "chains_of_ice"            ) return new chains_of_ice_t            ( this, options_str );
  if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );
  if ( name == "soul_reaper"              ) return new soul_reaper_t              ( this, options_str );

  // Blood Actions
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
  if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
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
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );

  // Talents
  if ( name == "breath_of_sindragosa"     ) return new breath_of_sindragosa_t     ( this, options_str );
  if ( name == "death_pact"               ) return new death_pact_t               ( this, options_str );
  if ( name == "defile"                   ) return new defile_t                   ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

static expr_t* create_ready_in_expression( death_knight_t* player, const std::string& action_name )
{
  struct ability_ready_expr_t : public expr_t
  {
    death_knight_melee_attack_t* action;

    ability_ready_expr_t( action_t* a ) :
      expr_t( "ability_ready_expr" ),
      action( debug_cast<death_knight_melee_attack_t*>( a ) )
    { }

    double evaluate() override
    {
      return ready_in( action -> p(), action -> base_costs[ RESOURCE_RUNE ] );
    }
  };

  action_t* action = player -> find_action( action_name );
  if ( ! action )
  {
    return nullptr;
  }

  return new ability_ready_expr_t( action );
}

static expr_t* create_rune_expression( death_knight_t* player, const std::string& rune_type_operation = std::string() )
{
  struct rune_inspection_expr_t : public expr_t
  {
    death_knight_t* dk;
    int myaction;
    // 0 == ready runes (nonfractional), 1 == min cooldown, 2 = max_cooldown, 4 == ready runes (fractional)

    rune_inspection_expr_t( death_knight_t* p, int myaction_ ) :
      expr_t( "rune_evaluation" ), dk( p ), myaction( myaction_ )
    { }

    virtual double evaluate() override
    {
      switch ( myaction )
      {
        case 0: return dk -> ready_runes_count( false );
        case 1: return dk -> runes_cooldown_min( );
        case 2: return dk -> runes_cooldown_max( );
        case 4: return dk -> ready_runes_count( true );
      }
      return 0.0;
    }
  };


  int op = 0;
  if ( util::str_compare_ci( rune_type_operation, "cooldown_min" ) )
  {
    op = 1;
  }
  else if ( util::str_compare_ci( rune_type_operation, "cooldown_max" ) )
  {
    op = 2;
  }
  else if ( util::str_compare_ci( rune_type_operation, "frac" ) || util::str_compare_ci( rune_type_operation, "fractional" ) )
  {
    op = 4;
  }

  return new rune_inspection_expr_t( player, op );
}

expr_t* death_knight_t::create_expression( action_t* a, const std::string& name_str ) {
  std::vector<std::string> splits = util::string_split( name_str, "." );

  // Single word expressions are always rune expressions
  if ( splits.size() == 1 )
  {
    expr_t* e = create_rune_expression( this, splits[ 0 ] );
    if ( e )
    {
      return e;
    }
  }
  else if ( splits.size() == 2 )
  {
    // For example, obliterate.ready_in
    if ( util::str_compare_ci( splits[ 1 ], "ready_in" ) )
    {
      expr_t* e = create_ready_in_expression( this, splits[ 0 ] );
      if ( e )
      {
        return e;
      }
    }
    // Otherwise, presume a rune expression (e.g., blood.death, blood.frac,
    // ...) Note that the second word is checked if it contains "death", if so,
    // the second word is presumed to be a specifier. Otherwise, it is presumed
    // to be an operation (cooldown_min, cooldown_max).
    else
    {
      bool has_specifier = false;
      if ( util::str_compare_ci( splits[ 1 ], "death" ) )
      {
        has_specifier = true;
      }

      // TODO: MRDMND - string logic here
      expr_t* e = create_rune_expression( this, std::string() );

      if ( e )
      {
        return e;
      }
    }
  }
  // 3 part expressions are presumed to be in the form
  // <runetype>.<specifier>.<operation>, e.g., blood.death.cooldown_min
  else if ( splits.size() == 3 )
  {
    // TODO: MRDMND - string logic here
    expr_t* e = create_rune_expression( this, std::string() );
    if ( e )
    {
      return e;
    }
  }

  return player_t::create_expression( a, name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  if ( specialization() == DEATH_KNIGHT_UNHOLY )
  {
    if ( find_action( "summon_gargoyle" ) )
    {
      pets.gargoyle = new pets::gargoyle_pet_t( sim, this );
    }

    if ( find_action( "raise_dead" ) )
    {
      pets.ghoul_pet = new pets::ghoul_pet_t( sim, this, "ghoul", false );
    }
  }

  if ( find_action( "dancing_rune_weapon" ) && specialization() == DEATH_KNIGHT_BLOOD )
  {
    pets.dancing_rune_weapon = new pets::dancing_rune_weapon_pet_t( sim, this );
  }

  if ( find_action( "army_of_the_dead" ) )
  {
    for ( int i = 0; i < 8; i++ )
    {
      pets.army_ghoul[ i ] = new pets::army_ghoul_pet_t( sim, this );
    }
  }
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_melee_haste() const
{
  double haste = player_t::composite_melee_haste();

  //haste *= 1.0 / ( 1.0 + buffs.unholy_presence -> value() );

  haste *= 1.0 / ( 1.0 + spec.veteran_of_the_third_war -> effectN( 6 ).percent() );

  haste *= 1.0 / ( 1.0 + spec.icy_talons -> effectN( 2 ).percent() );

  //if ( buffs.obliteration -> up() )
  //{
  //  haste *= 1.0 / ( 1.0 + buffs.obliteration -> data().effectN( 1 ).percent() );
  //}

  return haste;
}

// death_knight_t::composite_spell_haste() ==================================

double death_knight_t::composite_spell_haste() const
{
  double haste = player_t::composite_spell_haste();

  haste *= 1.0 / ( 1.0 + spec.veteran_of_the_third_war -> effectN( 6 ).percent() );

  haste *= 1.0 / ( 1.0 + spec.icy_talons -> effectN( 2 ).percent() );

  /*if ( buffs.obliteration -> up() )
  {
    haste *= 1.0 / ( 1.0 + buffs.obliteration -> data().effectN( 1 ).percent() );
  }*/

  return haste;
}

// death_knight_t::init_rng =================================================

void death_knight_t::init_rng() {
  player_t::init_rng();
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attribute_multiplier[ATTR_STRENGTH] *= 1.0 + spec.unholy_might -> effectN( 1 ).percent();

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility = 0.0;

  resources.base[ RESOURCE_RUNIC_POWER ] = 100;
  resources.base[ RESOURCE_RUNE        ] = MAX_RUNES;

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

  // Blood
  spec.bladed_armor               = find_specialization_spell( "Bladed Armor" );
  spec.blood_rites                = find_specialization_spell( "Blood Rites" );
  spec.veteran_of_the_third_war   = find_specialization_spell( "Veteran of the Third War" );
  spec.crimson_scourge            = find_specialization_spell( "Crimson Scourge" );
  spec.sanguine_fortitude         = find_specialization_spell( "Sanguine Fortitude" );
  spec.will_of_the_necropolis     = find_specialization_spell( "Will of the Necropolis" );
  spec.riposte                    = find_specialization_spell( "Riposte" );
  spec.runic_strikes              = find_specialization_spell( "Runic Strikes" );

  // Frost
  spec.blood_of_the_north         = find_specialization_spell( "Blood of the North" );
  spec.icy_talons                 = find_specialization_spell( "Icy Talons" );
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
  spec.threat_of_thassarian       = find_specialization_spell( "Threat of Thassarian" );

  mastery.blood_shield            = find_mastery_spell( DEATH_KNIGHT_BLOOD );
  mastery.frozen_heart            = find_mastery_spell( DEATH_KNIGHT_FROST );
  mastery.dreadblade              = find_mastery_spell( DEATH_KNIGHT_UNHOLY );

  // Talents
  talent.blood_tap                = find_talent_spell( "Blood Tap" );
  talent.runic_empowerment        = find_talent_spell( "Runic Empowerment" );
  talent.runic_corruption         = find_talent_spell( "Runic Corruption" );

  talent.breath_of_sindragosa     = find_talent_spell( "Breath of Sindragosa" );
  talent.defile                   = find_talent_spell( "Defile" );

  // Glyphs
  glyph.chains_of_ice             = find_glyph_spell( "Glyph of Chains of Ice" );
  glyph.ice_reaper                = find_glyph_spell( "Glyph of the Ice Reaper" );
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
  spell.blood_rites               = find_spell( 163948 );

  // Active Spells
  if ( spec.necrosis -> ok() )
    active_spells.necrosis = new necrosis_t( this );

  fallen_crusader += find_spell( 53365 ) -> effectN( 1 ).percent();
}

// death_knight_t::default_apl_blood ========================================

void death_knight_t::default_apl_blood()
{
    // TODO: mrdmnd - implement
}

// death_knight_t::default_apl_frost ========================================

void death_knight_t::default_apl_frost()
{
    // TODO: mrdmnd - implement
}

void death_knight_t::default_apl_unholy()
{
    // TODO: mrdmnd - implement
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

  player_t::init_action_list();
}

bool death_knight_t::init_actions()
{
  active_spells.blood_plague = new blood_plague_t( this );
  active_spells.frost_fever = new frost_fever_t( this );

  return player_t::init_actions();
}

// death_knight_t::init_scaling =============================================

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( off_hand_weapon.type != WEAPON_NONE )
    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = true;

  if ( specialization() == DEATH_KNIGHT_BLOOD )
    scales_with[ STAT_BONUS_ARMOR ] = true;

  scales_with[ STAT_AGILITY ] = false;
}

// death_knight_t::init_buffs ===============================================
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

  buffs.antimagic_shell     = new antimagic_shell_buff_t( this );

  buffs.bone_shield         = buff_creator_t( this, "bone_shield", find_specialization_spell( "Bone Shield" ) )
                              .cd( timespan_t::zero() )
                              .max_stack( find_specialization_spell( "Bone Shield" ) -> initial_stacks() + find_spell( 144948 ) -> max_stacks() );
  buffs.bone_wall           = buff_creator_t( this, "bone_wall", find_spell( 144948 ) )
                              .chance( sets.has_set_bonus( SET_TANK, T16, B2 ) );
  buffs.crimson_scourge     = buff_creator_t( this, "crimson_scourge" ).spell( find_spell( 81141 ) )
                              .chance( spec.crimson_scourge -> proc_chance() );
  buffs.dancing_rune_weapon = buff_creator_t( this, "dancing_rune_weapon", find_spell( 81256 ) )
                              .cd( timespan_t::zero() )
                              .add_invalidate( CACHE_PARRY );
  buffs.dark_transformation = buff_creator_t( this, "dark_transformation", find_class_spell( "Dark Transformation" ) );
  buffs.deathbringer        = buff_creator_t( this, "deathbringer", find_spell( 144953 ) )
                              .chance( sets.has_set_bonus( SET_TANK, T16, B4 ) );
  buffs.icebound_fortitude  = buff_creator_t( this, "icebound_fortitude", find_class_spell( "Icebound Fortitude" ) )
                              .duration( find_class_spell( "Icebound Fortitude" ) -> duration() *
                                         ( 1.0 + glyph.icebound_fortitude -> effectN( 2 ).percent() ) )
                              .cd( timespan_t::zero() );
  buffs.killing_machine     = new killing_machine_buff_t( this );/*buff_creator_t( this, "killing_machine", find_spell( 51124 ) )
                              .default_value( find_spell( 51124 ) -> effectN( 1 ).percent() )
                              .chance( find_specialization_spell( "Killing Machine" ) -> proc_chance() ); // PPM based! */
  buffs.pillar_of_frost     = buff_creator_t( this, "pillar_of_frost", find_class_spell( "Pillar of Frost" ) )
                              .cd( timespan_t::zero() )
                              .default_value( find_class_spell( "Pillar of Frost" ) -> effectN( 1 ).percent() +
                                              sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).percent() )
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
  buffs.sudden_doom         = buff_creator_t( this, "sudden_doom", find_spell( 81340 ) )
                              .max_stack( ( sets.has_set_bonus( SET_MELEE, T13, B2 ) ) ? 2 : 1 )
                              .cd( timespan_t::zero() )
                              .chance( spec.sudden_doom -> ok() );
  buffs.shadow_infusion     = buff_creator_t( this, "shadow_infusion", spec.shadow_infusion -> effectN( 1 ).trigger() );

  buffs.vampiric_blood      = new vampiric_blood_buff_t( this );
  buffs.will_of_the_necropolis = buff_creator_t( this, "will_of_the_necropolis", find_spell( 157335 ) )
                                 .cd( find_spell( 157335 ) -> duration() );

  runeforge.rune_of_the_fallen_crusader = buff_creator_t( this, "unholy_strength", find_spell( 53365 ) )
                                          .add_invalidate( CACHE_STRENGTH );

  runeforge.rune_of_the_stoneskin_gargoyle = buff_creator_t( this, "stoneskin_gargoyle", find_spell( 62157 ) )
                                             .add_invalidate( CACHE_ARMOR )
                                             .add_invalidate( CACHE_STAMINA )
                                             .chance( 0 );

}

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains.antimagic_shell                  = get_gain( "antimagic_shell"            );
  gains.blood_rites                      = get_gain( "blood_rites"                );
  gains.butchery                         = get_gain( "butchery"                   );
  gains.chill_of_the_grave               = get_gain( "chill_of_the_grave"         );
  gains.power_refund                     = get_gain( "power_refund"               );
  gains.rune                             = get_gain( "rune_regen_all"             );
  gains.runic_empowerment                = get_gain( "runic_empowerment"          );
  gains.empower_rune_weapon              = get_gain( "empower_rune_weapon"        );
  gains.blood_tap                        = get_gain( "blood_tap"                  );
  gains.rc                               = get_gain( "runic_corruption_all"       );
  // gains.blood_tap_blood                  = get_gain( "blood_tap_blood"            );
  //gains.blood_tap_blood          -> type = ( resource_e ) RESOURCE_RUNE_BLOOD   ;
  gains.veteran_of_the_third_war         = get_gain( "Veteran of the Third War" );
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

  procs.ready_rune              = get_proc( "Rune ready" );

  procs.km_natural_expiration    = get_proc( "Killing Machine expired naturally" );
}

// death_knight_t::init_resources ===========================================

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RUNIC_POWER ] = 0;
  resources.current[ RESOURCE_RUNE        ] = resources.max[ RESOURCE_RUNE ];
}

// death_knight_t::reset ====================================================

void death_knight_t::reset() {
  player_t::reset();
  runic_power_decay_rate = 1; // 1 RP per second decay
  antimagic_shell_absorbed = 0.0;
  _runes.reset();
}

// death_knight_t::combat_begin =============================================

struct vottw_regen_event_t : public event_t
{
  death_knight_t* dk;

  vottw_regen_event_t( death_knight_t* player ) :
    event_t( *player ),
    dk( player )
  {
    add_event( timespan_t::from_seconds( 1 ) );
  }
  virtual const char* name() const override
  { return "veteran_of_the_third_war"; }
  void execute() override
  {
    dk -> resource_gain( RESOURCE_RUNIC_POWER,
                         dk -> spec.veteran_of_the_third_war -> effectN( 8 ).base_value(),
                         dk -> gains.veteran_of_the_third_war );

    new ( sim() ) vottw_regen_event_t( dk );
  }
};


void death_knight_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == DEATH_KNIGHT_BLOOD )
  {
    new ( *sim ) vottw_regen_event_t( this );
  }
}

// death_knight_t::assess_heal ==============================================

void death_knight_t::assess_heal( school_e school, dmg_e t, action_state_t* s )
{
  if ( buffs.vampiric_blood -> up() )
    s -> result_total *= 1.0 + buffs.vampiric_blood -> data().effectN( 1 ).percent() +
                                glyph.vampiric_blood -> effectN( 1 ).percent();

  player_t::assess_heal( school, t, s );
}

// death_knight_t::assess_damage_imminent ===================================

void death_knight_t::assess_damage_imminent( school_e school, dmg_e, action_state_t* s )
{
  if ( school != SCHOOL_PHYSICAL )
  {
    if ( buffs.antimagic_shell -> up() )
    {
      double absorbed = s -> result_amount * spell.antimagic_shell -> effectN( 1 ).percent();
      antimagic_shell_absorbed += absorbed;

      double max_hp_absorb = resources.max[RESOURCE_HEALTH] * 0.4;

      if ( antimagic_shell_absorbed > max_hp_absorb )
      {
        absorbed = antimagic_shell_absorbed - max_hp_absorb;
        antimagic_shell_absorbed = -1.0; // Set to -1.0 so expire_override knows that we don't need to reduce cooldown from regenerative magic. 
        buffs.antimagic_shell -> expire();
      }

      double generated = absorbed / resources.max[RESOURCE_HEALTH];

      s -> result_amount -= absorbed;
      s -> result_absorbed -= absorbed;
      s -> self_absorb_amount += absorbed;
      iteration_absorb_taken += absorbed;

      //gains.antimagic_shell -> add( RESOURCE_HEALTH, absorbed );

      if ( antimagic_shell )
        antimagic_shell -> add_result( absorbed, absorbed, ABSORB, RESULT_HIT, BLOCK_RESULT_UNBLOCKED, this );

      resource_gain( RESOURCE_RUNIC_POWER, util::round( generated * 100.0 ), gains.antimagic_shell, s -> action );
    }
  }
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
    }
  }

  if ( health_pct >= spec.will_of_the_necropolis -> effectN( 1 ).base_value() &&
       health_percentage() < spec.will_of_the_necropolis -> effectN( 1 ).base_value() )
  {
    buffs.will_of_the_necropolis -> trigger();
    buffs.rune_tap -> trigger();
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
  //if ( buffs.blood_presence -> check() )
  //  state -> result_amount *= 1.0 + buffs.blood_presence -> data().effectN( 6 ).percent();

  if ( buffs.rune_tap -> up() )
    state -> result_amount *= 1.0 + buffs.rune_tap -> data().effectN( 1 ).percent();

  if ( buffs.bone_shield -> up() )
    state -> result_amount *= 1.0 + buffs.bone_shield -> data().effectN( 1 ).percent();

  if ( buffs.icebound_fortitude -> up() )
    state -> result_amount *= 1.0 + buffs.icebound_fortitude -> data().effectN( 3 ).percent() + spec.sanguine_fortitude -> effectN( 1 ).percent();

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

//  if ( buffs.blood_presence -> check() )
//    a *= 1.0 + buffs.blood_presence -> data().effectN( 3 ).percent();

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
//    if ( buffs.blood_presence -> check() )
//      m *= 1.0 + buffs.blood_presence -> data().effectN( 5 ).percent();

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

// death_knight_t::composite_leech ========================================

double death_knight_t::composite_leech() const
{
  double leech = player_t::composite_leech();

  // TODO: Additive or multiplicative?
/*  if ( buffs.lichborne -> up() )
  {
    leech += buffs.lichborne -> data().effectN( 1 ).percent();
  }*/

  return leech;
}

// death_knight_t::composite_melee_expertise ===============================

double death_knight_t::composite_melee_expertise( const weapon_t* ) const
{
  double expertise = player_t::composite_melee_expertise( nullptr );

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

  if ( dbc::is_school( school, SCHOOL_SHADOW )  )
  {
    if ( mastery.dreadblade -> ok() )
    {
      m *= 1.0 + cache.mastery_value();
    }
  }

  if ( mastery.frozen_heart -> ok() && dbc::is_school( school, SCHOOL_FROST )  )
    m *= 1.0 + cache.mastery_value();

  /*m *= 1.0 + spec.improved_blood_presence -> effectN( 2 ).percent();*/

  /*if ( buffs.crazed_monstrosity -> up() )
  {
    m *= 1.0 + buffs.crazed_monstrosity -> data().effectN( 1 ).percent();
  }*/

  return m;
}

// death_knight_t::composite_player_critical_damage_multiplier ===================

double death_knight_t::composite_player_critical_damage_multiplier() const
{
  double m = player_t::composite_player_critical_damage_multiplier();

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

  //c += spec.improved_blood_presence -> effectN( 1 ).percent();

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

  /*
  if ( buffs.unholy_presence -> up() )
    ms += buffs.unholy_presence -> data().effectN( 2 ).percent();

  if ( talent.deaths_advance -> ok() )
    ms += spell.deaths_advance -> effectN( 2 ).percent();
  */
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

  add_option( opt_float( "fallen_crusader_str", fallen_crusader ) );
  add_option( opt_float( "fallen_crusader_rppm", fallen_crusader_rppm ) );

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
void death_knight_t::regen( timespan_t periodicity ) {
  player_t::regen( periodicity );

  if ( sim -> debug )
    log_rune_status( this );

  for ( rune_t& rune : _runes.slot) {
    rune.regen_rune(periodicity);
  }

  if ( sim -> debug )
    log_rune_status( this );
}

// death_knight_t::runes_per_second =========================================
// Base rune regen rate is 10 seconds; we want the per-second regen
// rate, so divide by 10.0.  Haste is a multiplier (so 30% haste
// means composite_attack_haste is 1/1.3), so we invert it.  Haste
// linearly scales regen rate -- 100% haste means a rune regens in 5
// seconds, etc.
inline double death_knight_t::runes_per_second() const {
  double rps = RUNE_REGEN_BASE_SEC / cache.attack_haste();
  // Runic corruption doubles rune regeneration speed
  if ( buffs.runic_corruption -> check() ) {
    rps *= 2.0;
  }

  return rps;
}

// death_knight_t::trigger_runic_empowerment ================================

void death_knight_t::trigger_runic_empowerment( double rpcost )
{
  if ( ! rng().roll( talent.runic_empowerment -> effectN( 1 ).percent() * rpcost / 100.0 ) )
    return;

  int depleted_runes[MAX_RUNES];
  int num_depleted = 0;

  // Find fully depleted runes, i.e., both runes are on CD
  for ( size_t i = 0; i < MAX_RUNES; ++i )
  {
    if ( _runes.slot[i].is_depleted() )
    {
      depleted_runes[ num_depleted++ ] = i;
    }
  }

  if ( num_depleted > 0 )
  {
    int rune_to_regen = depleted_runes[ ( int ) ( player_t::rng().real() * num_depleted * 0.9999 ) ];
    rune_t* regen_rune = &_runes.slot[rune_to_regen];
    regen_rune -> reset();
    gains.runic_empowerment -> add ( RESOURCE_RUNE, 1, 0 );
    if ( sim -> log ) sim -> out_log.printf( "runic empowerment regen'd rune %d", rune_to_regen );
    procs.runic_empowerment -> occur();

  }
  else
  {
    // If there were no available runes to refresh
    procs.runic_empowerment_wasted -> occur();
    gains.runic_empowerment -> add ( RESOURCE_RUNE, 0, 1 );
  }
}

void death_knight_t::trigger_necrosis( const action_state_t* state )
{
  if ( ! spec.necrosis -> ok() )
    return;

  active_spells.necrosis -> target = state -> target;
  active_spells.necrosis -> schedule_execute();
}

void death_knight_t::apply_diseases( action_state_t* state, unsigned diseases )
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

// death_knight_t rune inspections ==========================================

// death_knight_t::ready_runes_count ========================================

// how many runes of type rt are ready?
double death_knight_t::ready_runes_count( bool fractional ) const
{
  // If fractional, then a rune array [0.1, 0.1, 0.3, 0.4, 0.1, 0.2] would return
  // 0.1+0.1+0.3+0.4+0.1+0.2 = 1.2 (total runes)
  // This could be used to estimate when you're in a "high resource" state versus a low resource state.

  // If fractional is false, then calling this method on that rune array would return zero, because
  // there are no runes of value 1.0
  double result = 0;
  for ( size_t rune_idx = 0; rune_idx < MAX_RUNES; ++rune_idx )
  {
    const rune_t& r = _runes.slot[ rune_idx ];
    if ( fractional || r.is_ready() )
    {
      result += r.value;
    }
  }

  return result;
}

// death_knight_t::runes_cooldown_min =======================================

double death_knight_t::runes_cooldown_min( ) const
{
  double min_time = std::numeric_limits<double>::max();

  for ( size_t rune_idx = 0; rune_idx < MAX_RUNES; ++rune_idx )
  {
    const rune_t& r = _runes.slot[ rune_idx ];

    if ( r.is_ready() ) {
      return 0.0;
    }

    double time = runes_cooldown_time( r );
    if ( time < min_time )
    {
      min_time = time;
    }
  }

  return min_time;
}

// death_knight_t::runes_cooldown_max =======================================

double death_knight_t::runes_cooldown_max( ) const
{
  double max_time = 0;

  for ( size_t rune_idx = 0; rune_idx < MAX_RUNES; ++rune_idx )
  {
    const rune_t& r = _runes.slot[ rune_idx ];

    double time = runes_cooldown_time( r );
    if ( time > max_time )
    {
      max_time = time;
    }
  }

  return max_time;
}

// death_knight_t::runes_cooldown_time ======================================

double death_knight_t::runes_cooldown_time( const rune_t& rune ) const
{
  return rune.is_ready() ? 0.0 : ( 1.0 - rune.value ) / runes_per_second();
}

void death_knight_t::arise()
{
  player_t::arise();
  runeforge.rune_of_the_stoneskin_gargoyle -> trigger();
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

static void do_trinket_init( death_knight_t*          player,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       player -> specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

struct death_knight_module_t : public module_t {
  death_knight_module_t() : module_t( DEATH_KNIGHT ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override {
    auto  p = new death_knight_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new death_knight_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override {
    unique_gear::register_special_effect(  50401,    runeforge::razorice_attack );
    unique_gear::register_special_effect(  51714,    runeforge::razorice_debuff );
    unique_gear::register_special_effect( 166441,    runeforge::fallen_crusader );
    unique_gear::register_special_effect(  62157, runeforge::stoneskin_gargoyle );
  }

  virtual void register_hotfixes() const override {}
  virtual void init( player_t* ) const override {}
  virtual bool valid() const override { return true; }
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::death_knight() {
  static death_knight_module_t m;
  return &m;
}
