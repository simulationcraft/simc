// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_class_modules.hpp"

#ifndef SC_WARLOCK_HPP
#define SC_WARLOCK_HPP

struct warlock_t;

#if SC_WARLOCK == 1

#define NIGHTFALL_LIMIT 5
#define WILD_IMP_LIMIT 10

struct warlock_targetdata_t : public targetdata_t
{
  dot_t*  dots_corruption;
  dot_t*  dots_unstable_affliction;
  dot_t*  dots_agony;
  dot_t*  dots_doom;
  dot_t*  dots_immolate;
  dot_t*  dots_drain_life;
  dot_t*  dots_drain_soul;
  dot_t*  dots_shadowflame;
  dot_t*  dots_malefic_grasp;

  buff_t* debuffs_haunt;

  bool ds_started_below_20;
  int shadowflame_stack;

  int affliction_effects();
  int active_dots();

  warlock_targetdata_t( warlock_t* source, player_t* target );
};

struct wild_imp_pet_t;

struct warlock_t : public player_t
{
  // Active Pet
  struct pets_t
  {
    pet_t* active;
    wild_imp_pet_t* wild_imps[ WILD_IMP_LIMIT ];
  } pets;

  // Buffs
  struct buffs_t
  {
    buff_t* backdraft;
    buff_t* dark_soul;
    buff_t* metamorphosis;
    buff_t* molten_core;
    buff_t* soulburn;
    buff_t* bane_of_havoc;
    buff_t* tier13_4pc_caster;
    buff_t* grimoire_of_sacrifice;
    buff_t* demonic_calling;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* demonic_calling;
    cooldown_t* infernal;
    cooldown_t* doomguard;
    cooldown_t* imp_swarm;
  } cooldowns;

  // Talents

  struct talents_t
  {
    const spell_data_t* dark_regeneration;
    const spell_data_t* soul_leech;
    const spell_data_t* harvest_life;

    const spell_data_t* howl_of_terror;
    const spell_data_t* mortal_coil;
    const spell_data_t* shadowfury;

    const spell_data_t* soul_link;
    const spell_data_t* sacrificial_pact;
    const spell_data_t* dark_bargain;

    const spell_data_t* blood_fear;
    const spell_data_t* burning_rush;
    const spell_data_t* unbound_will;

    const spell_data_t* grimoire_of_supremacy;
    const spell_data_t* grimoire_of_service;
    const spell_data_t* grimoire_of_sacrifice;

    const spell_data_t* archimondes_vengeance;
    const spell_data_t* kiljaedens_cunning;
    const spell_data_t* mannoroths_fury;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    // General
    const spell_data_t* dark_soul;
    const spell_data_t* nethermancy;

    // Affliction
    const spell_data_t* nightfall;
    const spell_data_t* malefic_grasp;

    // Demonology
    const spell_data_t* decimation;
    const spell_data_t* demonic_fury;
    const spell_data_t* metamorphosis;
    const spell_data_t* molten_core;

    // Destruction
    const spell_data_t* backdraft;
    const spell_data_t* burning_embers;
    const spell_data_t* chaotic_energy;

  } spec;

  struct mastery_spells_t
  {
    const spell_data_t* potent_afflictions;
    const spell_data_t* master_demonologist;
    const spell_data_t* emberstorm;
  } mastery_spells;

  std::string dark_intent_target_str;

  // Gains
  struct gains_t
  {
    gain_t* life_tap;
    gain_t* soul_leech;
    gain_t* tier13_4pc;
    gain_t* nightfall;
    gain_t* drain_soul;
    gain_t* incinerate;
    gain_t* rain_of_fire;
    gain_t* fel_flame;
  } gains;

  // Procs
  struct procs_t
  {
    proc_t* wild_imp;
  } procs;

  // Random Number Generators
  struct rngs_t
  {
    rng_t* demonic_calling;
    rng_t* molten_core;
    rng_t* nightfall;
    rng_t* ember_gain;
  } rngs;

  struct glyphs_t
  {
    const spell_data_t* conflagrate;
    const spell_data_t* dark_soul;
    const spell_data_t* demon_training;
    const spell_data_t* doom;
    const spell_data_t* life_tap;
    const spell_data_t* imp_swarm;
  };
  glyphs_t glyphs;

  struct meta_cost_event_t : event_t
  {
    meta_cost_event_t( player_t* p ) :
      event_t( p -> sim, p, "metamorphosis_fury_cost" )
    {
      sim -> add_event( this, timespan_t::from_seconds( 1 ) );
    }

    virtual void execute()
    {
      warlock_t* p = ( warlock_t* ) player;
      p -> meta_cost_event = new ( sim ) meta_cost_event_t( player );
      p -> resource_loss( RESOURCE_DEMONIC_FURY, 6 );
    }
  };

  meta_cost_event_t* meta_cost_event;

  void trigger_metamorphosis() { meta_cost_event = new ( sim ) meta_cost_event_t( this ); buffs.metamorphosis -> trigger(); main_hand_attack -> schedule_execute(); };
  void cancel_metamorphosis()  { main_hand_attack -> cancel(); event_t::cancel( meta_cost_event ); buffs.metamorphosis -> expire(); };

  struct demonic_calling_event_t : event_t
  {
    demonic_calling_event_t( player_t* p, timespan_t delay = timespan_t::from_seconds( 20 ) ) :
      event_t( p -> sim, p, "demonic_calling" )
    {
      sim -> add_event( this, delay );
    }

    virtual void execute()
    {
      warlock_t* p = ( warlock_t* ) player;
      p -> demonic_calling_event = new ( sim ) demonic_calling_event_t( player );
      if ( p -> cooldowns.imp_swarm -> remains() == timespan_t::zero() ) p -> buffs.demonic_calling -> trigger();
    }
  };

  demonic_calling_event_t* demonic_calling_event;

  int nightfall_index;
  timespan_t nightfall_times[ NIGHTFALL_LIMIT ];
  bool verify_nightfall();

  int use_pre_soulburn;
  int initial_burning_embers, initial_demonic_fury;
  timespan_t ember_react;

  warlock_t( sim_t* sim, const std::string& name, race_type_e r = RACE_UNDEAD );

  // Character Definition
  virtual warlock_targetdata_t* new_targetdata( player_t* target )
  { return new warlock_targetdata_t( this, target ); }
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_benefits();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      init_resources( bool force );
  virtual void      reset();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  buff_t*   create_buff( const spell_data_t* sd, const std::string& token );
  buff_t*   create_buff( int id, const std::string& token );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual bool      create_profile( std::string& profile_str, save_type_e=SAVE_ALL, bool save_html=false ) const;
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( const item_t& ) const;
  virtual resource_type_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_type_e primary_role() const     { return ROLE_SPELL; }
  virtual double    composite_spell_power_multiplier() const;
  virtual double    matching_gear_multiplier( attribute_type_e attr ) const;
  virtual double composite_player_multiplier( school_type_e school, const action_t* a ) const;
  virtual double composite_spell_crit() const;
  virtual double composite_spell_haste() const;
  virtual double composite_mastery() const;
  virtual void combat_begin();
  virtual expr_t* create_expression( action_t* a, const std::string& name_str );
  virtual double resource_loss( resource_type_e resource_type, double amount, gain_t* gain = 0, action_t* action = 0 );
};

// ==========================================================================
// Warlock Pet
// ==========================================================================

namespace pet_stats{
struct _weapon_list_t;
}
struct warlock_pet_t : public pet_t
{
  double ap_per_owner_sp;
  int stats_avaiable;
  int stats2_avaiable;
  gain_t* owner_fury_gain;

  double get_attribute_base( int level, int stat_type_e, pet_type_e pet_type );
private:
  const pet_stats::_weapon_list_t* get_weapon( pet_type_e pet_type );
public:
  double get_weapon_min( int level, pet_type_e pet_type );
  double get_weapon_max( int level, pet_type_e pet_type );
  timespan_t get_weapon_swing_time( int level, pet_type_e pet_type );
  warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt, bool guardian = false );
  virtual bool ooc_buffs() { return true; }
  virtual void init_base();
  virtual timespan_t available() const;
  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero(),
                               bool   waiting=false );
  virtual double composite_spell_haste() const;
  virtual double composite_attack_haste() const;
  virtual double composite_spell_power( school_type_e school ) const;
  virtual double composite_attack_power() const;
  virtual double composite_attack_crit( const weapon_t* ) const;
  virtual double composite_spell_crit() const;
  virtual double composite_player_multiplier( school_type_e school, const action_t* a ) const;
  virtual double composite_attack_hit() const { return owner -> composite_spell_hit(); }
  virtual resource_type_e primary_resource() const { return RESOURCE_ENERGY; }
  virtual double energy_regen_per_second() const;
  warlock_t* o() const
  { return static_cast<warlock_t*>( owner ); }
};

// ==========================================================================
// Warlock Main Pet
// ==========================================================================

struct warlock_main_pet_t : public warlock_pet_t
{
  warlock_main_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt );
  virtual double composite_attack_expertise( const weapon_t* ) const;
};

// ==========================================================================
// Warlock Guardian Pet
// ==========================================================================

struct warlock_guardian_pet_t : public warlock_pet_t
{
  warlock_guardian_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_e pt );
  virtual void summon( timespan_t duration=timespan_t::zero() );
};

// ==========================================================================
// Pet Imp
// ==========================================================================

struct imp_pet_t : public warlock_main_pet_t
{
  imp_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "imp" );
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Felguard
// ==========================================================================

struct felguard_pet_t : public warlock_main_pet_t
{
  felguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felguard" );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Felhunter
// ==========================================================================

struct felhunter_pet_t : public warlock_main_pet_t
{
  felhunter_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felhunter" );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Succubus
// ==========================================================================

struct succubus_pet_t : public warlock_main_pet_t
{
  succubus_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "succubus" );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Voidwalker
// ==========================================================================

struct voidwalker_pet_t : public warlock_main_pet_t
{
  voidwalker_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "voidwalker" );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Infernal
// ==========================================================================

struct infernal_pet_t : public warlock_guardian_pet_t
{
  infernal_pet_t( sim_t* sim, warlock_t* owner );
  virtual double composite_spell_power( school_type_e school ) const;
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Doomguard
// ==========================================================================

struct doomguard_pet_t : public warlock_guardian_pet_t
{
  doomguard_pet_t( sim_t* sim, warlock_t* owner );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Wild Imp
// ==========================================================================

struct wild_imp_pet_t : public warlock_guardian_pet_t
{
  wild_imp_pet_t* main_imp;

  wild_imp_pet_t( sim_t* sim, warlock_t* owner, wild_imp_pet_t* m = 0 );
  virtual void init_base();
  virtual timespan_t available() const;
  virtual void demise();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

#endif

#endif
