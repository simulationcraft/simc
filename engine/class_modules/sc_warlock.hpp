// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#ifndef SC_WARLOCK_HPP
#define SC_WARLOCK_HPP

struct warlock_targetdata_t : public targetdata_t
{
  dot_t*  dots_corruption;
  dot_t*  dots_unstable_affliction;
  dot_t*  dots_bane_of_agony;
  dot_t*  dots_bane_of_doom;
  dot_t*  dots_immolate;
  dot_t*  dots_drain_life;
  dot_t*  dots_drain_soul;
  dot_t*  dots_shadowflame_dot;
  dot_t*  dots_curse_of_elements;
  dot_t*  dots_burning_embers;

  buff_t* debuffs_haunted;
  buff_t* debuffs_shadow_embrace;

  int affliction_effects();
  int active_dots();

  warlock_targetdata_t( player_t* source, player_t* target );
};

struct warlock_pet_t;
struct warlock_main_pet_t;
struct warlock_guardian_pet_t;


struct warlock_t : public player_t
{
  // Active Pet
  warlock_main_pet_t* active_pet;
  pet_t* pet_ebon_imp;

  // Buffs
  struct buffs_t
  {
    buff_t* backdraft;
    buff_t* decimation;
    buff_t* demon_armor;
    buff_t* demonic_empowerment;
    buff_t* empowered_imp;
    buff_t* eradication;
    buff_t* fel_armor;
    buff_t* metamorphosis;
    buff_t* molten_core;
    buff_t* shadow_trance;
    buff_t* hand_of_guldan;
    buff_t* improved_soul_fire;
    buff_t* dark_intent_feedback;
    buff_t* soulburn;
    buff_t* demon_soul_imp;
    buff_t* demon_soul_felguard;
    buff_t* demon_soul_felhunter;
    buff_t* demon_soul_succubus;
    buff_t* demon_soul_voidwalker;
    buff_t* bane_of_havoc;
    buff_t* searing_pain_soulburn;
    buff_t* tier13_4pc_caster;
  } buffs;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* metamorphosis;
    cooldown_t* infernal;
    cooldown_t* doomguard;
  } cooldowns;

  // Talents

  struct talents_t
  {
    // Put MoP talents in here
  } talents;

  // Affliction
  spell_id_t* talent_unstable_affliction;
  talent_t* talent_doom_and_gloom; // done
  talent_t* talent_improved_life_tap; // done
  talent_t* talent_improved_corruption; // done
  talent_t* talent_jinx;
  talent_t* talent_soul_siphon; // done
  talent_t* talent_siphon_life; // done
  talent_t* talent_eradication; // done
  talent_t* talent_soul_swap;
  talent_t* talent_shadow_embrace; // done
  talent_t* talent_deaths_embrace; // done
  talent_t* talent_nightfall; // done
  talent_t* talent_soulburn_seed_of_corruption;
  talent_t* talent_everlasting_affliction; // done
  talent_t* talent_pandemic; // done
  talent_t* talent_haunt; // done

  // Demonology
  spell_id_t* talent_summon_felguard;
  talent_t* talent_demonic_embrace; // done
  talent_t* talent_dark_arts; // done
  talent_t* talent_mana_feed; // done
  talent_t* talent_demonic_aegis;
  talent_t* talent_master_summoner;
  talent_t* talent_impending_doom;
  talent_t* talent_demonic_empowerment;
  talent_t* talent_molten_core; // done
  talent_t* talent_hand_of_guldan;
  talent_t* talent_aura_of_foreboding;
  talent_t* talent_ancient_grimoire;
  talent_t* talent_inferno;
  talent_t* talent_decimation; // done
  talent_t* talent_cremation;
  talent_t* talent_demonic_pact;
  talent_t* talent_metamorphosis; // done

  // Destruction
  spell_id_t* talent_conflagrate;
  talent_t* talent_bane; // done
  talent_t* talent_shadow_and_flame; // done
  talent_t* talent_improved_immolate;
  talent_t* talent_improved_soul_fire;
  talent_t* talent_emberstorm; // done
  talent_t* talent_improved_searing_pain;
  talent_t* talent_backdraft; // done
  talent_t* talent_shadowburn; // done
  talent_t* talent_burning_embers;
  talent_t* talent_soul_leech; // done
  talent_t* talent_fire_and_brimstone; // done
  talent_t* talent_shadowfury; // done
  talent_t* talent_empowered_imp; // done
  talent_t* talent_bane_of_havoc;
  talent_t* talent_chaos_bolt; // done

  struct passive_spells_t
  {
    spell_id_t* shadow_mastery;
    spell_id_t* demonic_knowledge;
    spell_id_t* cataclysm;
    spell_id_t* doom_and_gloom;
    spell_id_t* pandemic;
    spell_id_t* nethermancy;

  };
  passive_spells_t passive_spells;

  struct mastery_spells_t
  {
    mastery_t* potent_afflictions;
    mastery_t* master_demonologist;
    mastery_t* fiery_apocalypse;
  };
  mastery_spells_t mastery_spells;

  std::string dark_intent_target_str;

  // Gains
  struct gains_t
  {
    gain_t* fel_armor;
    gain_t* felhunter;
    gain_t* life_tap;
    gain_t* soul_leech;
    gain_t* soul_leech_health;
    gain_t* mana_feed;
    gain_t* tier13_4pc;
  } gains;

  // Uptimes
  benefit_t* uptimes_backdraft[ 4 ];

  // Procs
  struct procs_t
  {
    proc_t* empowered_imp;
    proc_t* impending_doom;
    proc_t* shadow_trance;
    proc_t* ebon_imp;
  } procs;

  // Random Number Generators
  struct rngs_t
  {
    rng_t* soul_leech;
    rng_t* everlasting_affliction;
    rng_t* pandemic;
    rng_t* cremation;
    rng_t* impending_doom;
    rng_t* siphon_life;
    rng_t* ebon_imp;
  } rngs;

  // Spells
  spell_t* spells_burning_embers;

  struct glyphs_t
  {
    glyph_t* metamorphosis;
    // Prime
    glyph_t* life_tap;
    glyph_t* shadow_bolt;

    // Major
    glyph_t* chaos_bolt;
    glyph_t* conflagrate;
    glyph_t* corruption;
    glyph_t* bane_of_agony;
    glyph_t* felguard;
    glyph_t* haunt;
    glyph_t* immolate;
    glyph_t* imp;
    glyph_t* incinerate;
    glyph_t* lash_of_pain;
    glyph_t* shadowburn;
    glyph_t* unstable_affliction;
  };
  glyphs_t glyphs;

  timespan_t constants_pandemic_gcd;

  int use_pre_soulburn;

  warlock_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE );


  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new warlock_targetdata_t( source, target );}
  virtual void      init_talents();
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
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& item );
  virtual resource_type_t primary_resource() const { return RESOURCE_MANA; }
  virtual role_type primary_role() const     { return ROLE_SPELL; }
  virtual double    composite_armor() const;
  virtual double    composite_spell_power( const school_type school ) const;
  virtual double    composite_spell_power_multiplier() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_player_td_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;

  // Event Tracking
  virtual action_expr_t* create_expression( action_t*, const std::string& name );


  static void trigger_mana_feed( action_t* s, double impact_result );
  static void trigger_burning_embers ( spell_t* s, double dmg );
};

// ==========================================================================
// Warlock Pet
// ==========================================================================

namespace pet_stats{
struct _weapon_list_t;
}
struct warlock_pet_t : public pet_t
{
  double damage_modifier;
  int stats_avaiable;
  int stats2_avaiable;

  gain_t* gains_mana_feed;
  proc_t* procs_mana_feed;

  double get_attribute_base( const int level, const int stat_type, const pet_type_t pet_type );
private:
  const pet_stats::_weapon_list_t* get_weapon( pet_type_t pet_type );
public:
  double get_weapon_min( int level, pet_type_t pet_type );
  double get_weapon_max( int level, pet_type_t pet_type );
  timespan_t get_weapon_swing_time( int level, pet_type_t pet_type );
  warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_t pt, bool guardian = false );
  virtual bool ooc_buffs() { return true; }
  virtual void init_base();
  virtual void init_resources( bool force );
  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero,
                               bool   waiting=false );
  virtual void summon( timespan_t duration=timespan_t::zero );
  virtual void dismiss();
  virtual double composite_spell_haste() const;
  virtual double composite_attack_haste() const;
  virtual double composite_spell_power( const school_type school ) const;
  virtual double composite_attack_power() const;
  virtual double composite_attack_crit( weapon_t* ) const;
  virtual double composite_spell_crit() const;
  warlock_t* o() const
  { return static_cast<warlock_t*>( owner ); }
};

// ==========================================================================
// Warlock Main Pet
// ==========================================================================

struct warlock_main_pet_t : public warlock_pet_t
{
  warlock_main_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_t pt );
  virtual void summon( timespan_t duration=timespan_t::zero );
  virtual void dismiss();
  virtual double composite_attack_expertise( weapon_t* ) const;
  virtual resource_type_t primary_resource() const;
  virtual double composite_player_multiplier( const school_type school, action_t* a ) const;
  virtual double composite_mp5() const;
};

// ==========================================================================
// Warlock Guardian Pet
// ==========================================================================

struct warlock_guardian_pet_t : public warlock_pet_t
{
  double snapshot_crit, snapshot_haste, snapshot_sp, snapshot_mastery;

  warlock_guardian_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_type_t pt );
  virtual void summon( timespan_t duration=timespan_t::zero );
  virtual double composite_attack_crit( weapon_t* ) const;
  virtual double composite_attack_expertise( weapon_t* ) const;
  virtual double composite_attack_haste() const;
  virtual double composite_attack_hit() const;
  virtual double composite_attack_power() const;
  virtual double composite_spell_crit() const;
  virtual double composite_spell_haste() const;
  virtual double composite_spell_power( const school_type school ) const;
  virtual double composite_spell_power_multiplier() const;
};

// ==========================================================================
// Pet Imp
// ==========================================================================

struct imp_pet_t : public warlock_main_pet_t
{
  imp_pet_t( sim_t* sim, warlock_t* owner );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Felguard
// ==========================================================================

struct felguard_pet_t : public warlock_main_pet_t
{
  felguard_pet_t( sim_t* sim, warlock_t* owner );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Felhunter
// ==========================================================================

struct felhunter_pet_t : public warlock_main_pet_t
{
  felhunter_pet_t( sim_t* sim, warlock_t* owner );
  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
  virtual void summon( timespan_t duration=timespan_t::zero );
  virtual void dismiss();
};

// ==========================================================================
// Pet Succubus
// ==========================================================================

struct succubus_pet_t : public warlock_main_pet_t
{
  succubus_pet_t( sim_t* sim, warlock_t* owner );
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Voidwalker
// ==========================================================================

struct voidwalker_pet_t : public warlock_main_pet_t
{
  voidwalker_pet_t( sim_t* sim, warlock_t* owner );
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
  virtual double composite_player_multiplier( const school_type school, action_t* a ) const;
};

// ==========================================================================
// Pet Ebon Imp
// ==========================================================================

struct ebon_imp_pet_t : public warlock_guardian_pet_t
{
  ebon_imp_pet_t( sim_t* sim, warlock_t* owner );
  virtual timespan_t available() const;
  virtual double composite_attack_power() const;
  virtual void init_base();
};

#endif
