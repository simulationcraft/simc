// ==========================================================================
// Priest Definitions Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================
//
// This file contains all definitions for priests. Implementations should
// be done in sc_priest.cpp if they are shared by more than one spec or
// in the respective spec file if they are limited to one spec only.

#pragma once
#include "action/parse_buff_effects.hpp"
#include "player/pet_spawner.hpp"
#include "sc_enums.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
/* Forward declarations
 */
struct priest_t;

namespace actions::spells
{
struct mind_sear_tick_t;
struct shadowy_apparition_spell_t;
struct psychic_link_t;
struct pain_of_death_t;
struct shadow_weaving_t;
struct echoing_void_t;
struct idol_of_cthun_t;
struct shadow_word_pain_t;
struct mental_fortitude_t;
}  // namespace actions::spells

/**
 * Priest target data
 * Contains target specific things
 */
struct priest_td_t final : public actor_target_data_t
{
public:
  struct dots_t
  {
    propagate_const<dot_t*> shadow_word_pain;
    propagate_const<dot_t*> vampiric_touch;
    propagate_const<dot_t*> devouring_plague;
    propagate_const<dot_t*> mind_flay;
    propagate_const<dot_t*> mind_flay_insanity;
    propagate_const<dot_t*> mind_sear;
    propagate_const<dot_t*> void_torrent;
    propagate_const<dot_t*> purge_the_wicked;
  } dots;

  struct buffs_t
  {
    propagate_const<buff_t*> schism;
    propagate_const<buff_t*> death_and_madness_debuff;
    propagate_const<buff_t*> echoing_void;
    propagate_const<buff_t*> echoing_void_collapse;
    propagate_const<buff_t*> apathy;
    propagate_const<buff_t*> sins_of_the_many;
  } buffs;

  priest_t& priest()
  {
    return *debug_cast<priest_t*>( source );
  }
  const priest_t& priest() const
  {
    return *debug_cast<priest_t*>( source );
  }

  priest_td_t( player_t* target, priest_t& p );
  void reset();
  void target_demise();
};

/**
 * Priest class definition
 * Derived from player_t. Contains everything that defines the priest class.
 */
struct priest_t final : public player_t
{
public:
  using base_t = player_t;

  // Buffs
  struct
  {
    // Generic
    propagate_const<buff_t*> desperate_prayer;
    absorb_buff_t* power_word_shield;
    propagate_const<buff_t*> fade;

    // Talents
    propagate_const<buff_t*> twist_of_fate;
    propagate_const<buff_t*> rhapsody;
    propagate_const<buff_t*> rhapsody_timer;
    propagate_const<buff_t*> protective_light;
    propagate_const<buff_t*> from_darkness_comes_light;
    propagate_const<buff_t*> death_and_madness_buff;
    propagate_const<buff_t*> death_and_madness_reset;
    propagate_const<buff_t*> vampiric_embrace;
    propagate_const<buff_t*> words_of_the_pious;

    // Discipline
    propagate_const<buff_t*> inner_focus;
    propagate_const<buff_t*> power_of_the_dark_side;
    propagate_const<buff_t*> sins_of_the_many;
    propagate_const<buff_t*> shadow_covenant;
    propagate_const<buff_t*> borrowed_time;
    propagate_const<buff_t*> revel_in_purity;
    propagate_const<buff_t*> blaze_of_light;
    propagate_const<buff_t*> train_of_thought;

    // Holy
    propagate_const<buff_t*> apotheosis;

    // Shadow
    propagate_const<buff_t*> dispersion;
    propagate_const<buff_t*> shadowform;
    propagate_const<buff_t*> shadowform_state;  // Dummy buff to track whether player entered Shadowform initially
    propagate_const<buff_t*> void_torrent;
    propagate_const<buff_t*> voidform;
    propagate_const<buff_t*> unfurling_darkness;
    propagate_const<buff_t*> unfurling_darkness_cd;  // Blizzard uses a buff to track the ICD
    propagate_const<buff_t*> ancient_madness;
    propagate_const<buff_t*> mind_devourer;
    propagate_const<buff_t*> mind_devourer_ms_active;  // Tracking buff only
    propagate_const<buff_t*> shadowy_insight;
    propagate_const<absorb_buff_t*> mental_fortitude;
    propagate_const<buff_t*> insidious_ire;
    propagate_const<buff_t*> thing_from_beyond;
    propagate_const<buff_t*> idol_of_yoggsaron;
    propagate_const<buff_t*> devoured_pride;
    propagate_const<buff_t*> dark_evangelism;
    propagate_const<buff_t*> surge_of_darkness;
    propagate_const<buff_t*> mind_melt;
    propagate_const<buff_t*> mind_flay_insanity;
    propagate_const<buff_t*> deathspeaker;
    propagate_const<buff_t*> dark_ascension;
    propagate_const<buff_t*> coalescing_shadows;
    propagate_const<buff_t*> coalescing_shadows_dot;

    // Tier Sets
    propagate_const<buff_t*> gathering_shadows;
    propagate_const<buff_t*> dark_reveries;
  } buffs;

  // Talents
  struct
  {
    // Priest Tree
    // Row 1
    player_talent_t renew;
    player_talent_t dispel_magic;
    player_talent_t shadowfiend;
    // Row 2
    player_talent_t prayer_of_mending;
    player_talent_t improved_flash_heal;
    player_talent_t purify_disease;
    player_talent_t psychic_voice;
    player_talent_t shadow_word_death;
    // Row 3
    player_talent_t focused_mending;
    player_talent_t holy_nova;
    player_talent_t protective_light;
    const spell_data_t* protective_light_buff;
    player_talent_t from_darkness_comes_light;
    player_talent_t angelic_feather;
    player_talent_t phantasm;
    player_talent_t death_and_madness;
    const spell_data_t* death_and_madness_insanity;
    // Row 4
    player_talent_t spell_warding;
    player_talent_t blessed_recovery;
    player_talent_t rhapsody;
    const spell_data_t* rhapsody_buff;
    player_talent_t leap_of_faith;
    player_talent_t shackle_undead;
    player_talent_t sheer_terror;
    player_talent_t void_tendrils;
    player_talent_t mind_control;
    player_talent_t dominate_mind;
    // Row 5
    player_talent_t words_of_the_pious;
    player_talent_t mass_dispel;
    player_talent_t move_with_grace;
    player_talent_t power_infusion;
    player_talent_t vampiric_embrace;
    player_talent_t sanguine_teachings;
    player_talent_t tithe_evasion;
    // Row 6
    player_talent_t inspiration;
    player_talent_t improved_mass_dispel;
    player_talent_t body_and_soul;
    player_talent_t twins_of_the_sun_priestess;
    player_talent_t void_shield;
    player_talent_t sanlayn;
    player_talent_t apathy;
    // Row 7
    player_talent_t unwavering_will;
    player_talent_t twist_of_fate;
    player_talent_t throes_of_pain;
    // Row 8
    player_talent_t angels_mercy;
    player_talent_t binding_heals;
    player_talent_t halo;
    const spell_data_t* halo_heal_holy;
    const spell_data_t* halo_dmg_holy;
    const spell_data_t* halo_heal_shadow;
    const spell_data_t* halo_dmg_shadow;
    player_talent_t divine_star;
    const spell_data_t* divine_star_heal_holy;
    const spell_data_t* divine_star_dmg_holy;
    const spell_data_t* divine_star_heal_shadow;
    const spell_data_t* divine_star_dmg_shadow;
    player_talent_t translucent_image;
    player_talent_t mindgames;
    const spell_data_t* mindgames_healing_reversal;
    const spell_data_t* mindgames_damage_reversal;
    // Row 9
    player_talent_t surge_of_light;
    player_talent_t lights_inspiration;
    player_talent_t crystalline_reflection;
    player_talent_t improved_fade;
    player_talent_t manipulation;
    // Row 10
    player_talent_t power_word_life;
    player_talent_t angelic_bulwark;
    player_talent_t void_shift;
    player_talent_t shattered_perceptions;

    struct
    {
      // Shadow Tree
      // Row 2
      player_talent_t dispersion;
      const spell_data_t* shadowy_apparition;  // Damage event
      player_talent_t shadowy_apparitions;     // Passive effect
      player_talent_t silence;
      // Row 3
      player_talent_t intangibility;
      player_talent_t mental_fortitude;
      player_talent_t misery;
      player_talent_t dark_void;
      const spell_data_t* dark_void_insanity;
      player_talent_t last_word;
      player_talent_t psychic_horror;
      // Row 4
      player_talent_t coalescing_shadows;
      const spell_data_t* coalescing_shadows_buff;
      const spell_data_t* coalescing_shadows_dot_buff;
      player_talent_t mind_sear;
      player_talent_t mind_spike;
      // Row 5
      player_talent_t puppet_master;
      player_talent_t mental_decay;
      player_talent_t void_eruption;
      const spell_data_t* void_eruption_damage;
      player_talent_t dark_ascension;
      player_talent_t unfurling_darkness;
      player_talent_t surge_of_darkness;
      const spell_data_t* surge_of_darkness_buff;
      // Row 6
      player_talent_t harnessed_shadows;
      player_talent_t shadowy_insight;
      player_talent_t ancient_madness;
      player_talent_t shadow_crash;
      player_talent_t mind_melt;
      // Row 7
      player_talent_t maddening_touch;
      const spell_data_t* maddening_touch_insanity;
      player_talent_t dark_evangelism;
      player_talent_t auspicious_spirits;
      player_talent_t tormented_spirits;
      player_talent_t psychic_link;
      player_talent_t whispers_of_the_damned;
      // Row 8
      player_talent_t mindbender;
      player_talent_t deathspeaker;
      player_talent_t mind_flay_insanity;
      const spell_data_t* mind_flay_insanity_spell;
      player_talent_t encroaching_shadows;
      player_talent_t damnation;
      player_talent_t void_torrent;
      // Row 9
      player_talent_t inescapable_torment;
      player_talent_t screams_of_the_void;
      player_talent_t pain_of_death;
      player_talent_t mind_devourer;
      player_talent_t insidious_ire;
      player_talent_t malediction;
      // Row 10
      player_talent_t idol_of_yshaarj;
      const spell_data_t* devoured_pride;
      const spell_data_t* devoured_despair;
      const spell_data_t* devoured_anger;
      const spell_data_t* devoured_fear;
      const spell_data_t* devoured_violence;
      player_talent_t idol_of_cthun;
      player_talent_t idol_of_yoggsaron;
      player_talent_t idol_of_nzoth;
    } shadow;

    struct
    {
      // Row 1
      player_talent_t power_of_the_dark_side;
      // Row 3
      player_talent_t dark_indulgence;
      player_talent_t schism;
      // Row 4
      player_talent_t painful_punishment;
      player_talent_t power_word_solace;
      player_talent_t purge_the_wicked;
      player_talent_t malicious_intent;
      // Row 5
      player_talent_t shadow_covenant;
      const spell_data_t* dark_reprimand;
      // Row 6
      player_talent_t embrace_shadow;
      player_talent_t twilight_corruption;
      player_talent_t revel_in_purity;
      player_talent_t pain_and_suffering;
      // Row 7
      player_talent_t borrowed_time;

      // Row 8
      player_talent_t lights_wrath;
      // Row 9
      const spell_data_t* blaze_of_light;
      player_talent_t train_of_thought;
    } discipline;

    // Shared
    const spell_data_t* shining_force;

    // Discipline
    const spell_data_t* castigation;
    const spell_data_t* sins_of_the_many;  // assumes 0 atonement targets

    // Holy
    // // T15
    const spell_data_t* enlightenment;
    // // T50
    const spell_data_t* light_of_the_naaru;
    const spell_data_t* apotheosis;
  } talents;

  // Specialization Spells
  struct
  {
    const spell_data_t* mind_blast;
    const spell_data_t* priest;  // General priest data
    const spell_data_t* shadow_word_death_self_damage;
    const spell_data_t* psychic_scream;
    const spell_data_t* fade;

    // Discipline
    const spell_data_t* discipline_priest;  // General discipline data
    const spell_data_t* penance;
    const spell_data_t* penance_tick;
    const spell_data_t* sins_of_the_many;

    // Holy
    const spell_data_t* holy_priest;  // General holy data
    const spell_data_t* holy_words;
    const spell_data_t* holy_word_serenity;

    // Shadow
    const spell_data_t* mind_flay;
    const spell_data_t* shadow_priest;  // General shadow data
    const spell_data_t* shadowform;
    const spell_data_t* void_bolt;
    const spell_data_t* voidform;
    const spell_data_t* hallucinations;
  } specs;

  // DoT Spells
  struct
  {
    const spell_data_t* shadow_word_pain;
    const spell_data_t* vampiric_touch;
    player_talent_t devouring_plague;
  } dot_spells;

  // Mastery Spells
  struct
  {
    const spell_data_t* grace;
    const spell_data_t* echo_of_light;
    const spell_data_t* shadow_weaving;
  } mastery_spells;

  // Cooldowns
  struct
  {
    // Shared
    propagate_const<cooldown_t*> shadow_word_death;
    propagate_const<cooldown_t*> mindgames;

    // Shadow
    propagate_const<cooldown_t*> void_bolt;
    propagate_const<cooldown_t*> mind_blast;
    propagate_const<cooldown_t*> void_eruption;

    // Holy
    propagate_const<cooldown_t*> holy_word_serenity;
    propagate_const<cooldown_t*> holy_fire;

    // Discipline
    propagate_const<cooldown_t*> penance;
  } cooldowns;

  struct realppm_t
  {
    propagate_const<real_ppm_t*> idol_of_cthun;
    propagate_const<real_ppm_t*> deathspeaker;
  } rppm;

  // Gains
  struct
  {
    propagate_const<gain_t*> insanity_auspicious_spirits;
    propagate_const<gain_t*> insanity_death_and_madness;
    propagate_const<gain_t*> insanity_mind_sear;
    propagate_const<gain_t*> mindbender;
    propagate_const<gain_t*> shadowfiend;
    propagate_const<gain_t*> power_of_the_dark_side;
    propagate_const<gain_t*> power_word_solace;
    propagate_const<gain_t*> throes_of_pain;
    propagate_const<gain_t*> insanity_idol_of_cthun_mind_flay;
    propagate_const<gain_t*> insanity_idol_of_cthun_mind_sear;
    propagate_const<gain_t*> hallucinations_power_word_shield;
    propagate_const<gain_t*> hallucinations_vampiric_embrace;
    propagate_const<gain_t*> insanity_dark_void;
    propagate_const<gain_t*> insanity_maddening_touch;
    propagate_const<gain_t*> insanity_whispers_of_the_damned;
  } gains;

  // Benefits
  struct
  {
  } benefits;

  // Procs
  struct
  {
    propagate_const<proc_t*> shadowy_apparition_vb;
    propagate_const<proc_t*> shadowy_apparition_swp;
    propagate_const<proc_t*> shadowy_apparition_dp;
    propagate_const<proc_t*> shadowy_apparition_mb;
    propagate_const<proc_t*> shadowy_apparition_ms;
    propagate_const<proc_t*> holy_fire_cd;
    propagate_const<proc_t*> power_of_the_dark_side;
    propagate_const<proc_t*> power_of_the_dark_side_overflow;
    propagate_const<proc_t*> mind_devourer;
    propagate_const<proc_t*> void_tendril;
    propagate_const<proc_t*> void_lasher;
    propagate_const<proc_t*> shadowy_insight;
    propagate_const<proc_t*> shadowy_insight_overflow;
    propagate_const<proc_t*> shadowy_insight_missed;
    propagate_const<proc_t*> thing_from_beyond;
    propagate_const<proc_t*> coalescing_shadows_mind_sear;
    propagate_const<proc_t*> coalescing_shadows_mind_flay;
    propagate_const<proc_t*> coalescing_shadows_shadow_word_pain;
    propagate_const<proc_t*> coalescing_shadows_shadowy_apparitions;
    propagate_const<proc_t*> deathspeaker;
    propagate_const<proc_t*> surge_of_darkness_vt;
    propagate_const<proc_t*> surge_of_darkness_dp;
    propagate_const<proc_t*> mind_melt_waste;
    propagate_const<proc_t*> idol_of_nzoth_swp;
    propagate_const<proc_t*> idol_of_nzoth_vt;
    propagate_const<proc_t*> mind_flay_insanity_wasted;
    propagate_const<proc_t*> idol_of_yshaarj_extra_duration;
    propagate_const<proc_t*> void_torrent_ticks_no_mastery;
    propagate_const<proc_t*> mindgames_casts_no_mastery;
    propagate_const<proc_t*> inescapable_torment_missed_mb;
    propagate_const<proc_t*> inescapable_torment_missed_swd;
  } procs;

  // Special
  struct
  {
    propagate_const<actions::spells::psychic_link_t*> psychic_link;
    propagate_const<actions::spells::shadow_weaving_t*> shadow_weaving;
    propagate_const<actions::spells::shadowy_apparition_spell_t*> shadowy_apparitions;
    propagate_const<actions::spells::echoing_void_t*> echoing_void;
    propagate_const<actions::spells::idol_of_cthun_t*> idol_of_cthun;
    propagate_const<actions::spells::shadow_word_pain_t*> shadow_word_pain;
    propagate_const<actions::spells::mental_fortitude_t*> mental_fortitude;
    propagate_const<actions::spells::pain_of_death_t*> pain_of_death;
  } background_actions;

  // Items
  struct
  {
  } active_items;

  // Pets
  struct priest_pets_t
  {
    propagate_const<pet_t*> shadowfiend;
    propagate_const<pet_t*> mindbender;
    spawner::pet_spawner_t<pet_t, priest_t> void_tendril;
    spawner::pet_spawner_t<pet_t, priest_t> void_lasher;
    spawner::pet_spawner_t<pet_t, priest_t> thing_from_beyond;

    priest_pets_t( priest_t& p );
  } pets;

  // Options
  struct
  {
    bool autoUnshift = true;  // Shift automatically out of stance/form
    bool fixed_time  = true;

    // Default param to set if you should cast Power Infusion on yourself
    bool self_power_infusion = true;

    // Add in options to override insanity gained
    // Mindgames gives 20 insanity from the healing and 20 from damage dealt
    // For most content the healing part won't proc, only default damage dealt
    bool mindgames_healing_reversal = false;
    bool mindgames_damage_reversal  = true;

    bool power_infusion_fiend = false;

    // Actives the screams bug with Mental Decay and Shadow Word: Pain
    bool priest_screams_bug = true;

    // Last tick of Mind Sear does not get buffed by Gathering Shadows
    bool gathering_shadows_bug = true;

    // Auspicious Spirits generates insanity on execute not hit
    bool as_insanity_bug = true;
  } options;

  priest_t( sim_t* sim, util::string_view name, race_e r );

  // player_t overrides
  void init_base_stats() override;
  void init_resources( bool force ) override;
  void init_spells() override;
  void create_buffs() override;
  void init_scaling() override;
  void init_finished() override;
  void init_background_actions() override;
  void reset() override;
  void create_options() override;
  std::string create_profile( save_e ) override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
  void create_pets() override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void assess_damage( school_e school, result_amount_type dtype, action_state_t* s ) override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_spell_crit_chance() const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_absorb_multiplier( const action_state_t* s ) const override;
  double composite_player_heal_multiplier( const action_state_t* s ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* t, school_e school ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void init_action_list() override;
  void combat_begin() override;
  void init_rng() override;
  const priest_td_t* find_target_data( const player_t* target ) const override;
  priest_td_t* get_target_data( player_t* target ) const override;
  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override;
  std::unique_ptr<expr_t> create_pet_expression( util::string_view expression_str,
                                                 util::span<util::string_view> splits );
  void arise() override;
  void do_dynamic_regen( bool ) override;
  void apply_affecting_auras( action_t& ) override;
  void invalidate_cache( cache_e ) override;

private:
  void create_cooldowns();
  void create_gains();
  void create_procs();
  void create_benefits();
  void create_buffs_shadow();
  void init_rng_shadow();
  void init_spells_shadow();
  std::unique_ptr<expr_t> create_expression_shadow( util::string_view name_str );
  action_t* create_action_shadow( util::string_view name, util::string_view options_str );

  void create_buffs_discipline();
  void init_spells_discipline();
  void init_rng_discipline();

  void init_background_actions_shadow();
  std::unique_ptr<expr_t> create_expression_discipline( action_t* a, const util::string_view name_str );
  action_t* create_action_discipline( util::string_view name, util::string_view options_str );

  void create_buffs_holy();
  void init_spells_holy();
  void init_rng_holy();
  void generate_apl_holy();
  expr_t* create_expression_holy( action_t* a, util::string_view name_str );
  action_t* create_action_holy( util::string_view name, util::string_view options_str );
  target_specific_t<priest_td_t> _target_data;

public:
  void generate_insanity( double num_amount, gain_t* g, action_t* action );
  void adjust_holy_word_serenity_cooldown();
  double tick_damage_over_time( timespan_t duration, const dot_t* dot ) const;
  void trigger_inescapable_torment( player_t* target );
  void trigger_idol_of_cthun( action_state_t* );
  void trigger_shadowy_apparitions( proc_t* proc, bool gets_crit_mod );
  int number_of_echoing_voids_active();
  void trigger_psychic_link( action_state_t* );
  void trigger_pain_of_death( action_state_t* );
  void trigger_shadow_weaving( action_state_t* );
  void trigger_void_shield( double result_amount );
  void refresh_insidious_ire_buff( action_state_t* s );
  bool is_screams_of_the_void_up( player_t* target, const unsigned int spell_id ) const;
  void spawn_thing_from_beyond();
  void trigger_idol_of_nzoth( player_t* target, proc_t* proc );
  int shadow_weaving_active_dots( const player_t* target, const unsigned int spell_id ) const;
  double shadow_weaving_multiplier( const player_t* target, const unsigned int spell_id ) const;

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;
};

namespace actions
{
/**
 * Priest action base class
 *
 * This is a template for common code between priest_spell_t, priest_heal_t and priest_absorb_t.
 * The template is instantiated with either spell_t, heal_t or absorb_t as the 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived class, don't skip it and call
 * spell_t/heal_t or absorb_t directly.
 */
template <typename Base>
struct priest_action_t : public Base, public parse_buff_effects_t<priest_td_t>
{
protected:
  priest_t& priest()
  {
    return *debug_cast<priest_t*>( ab::player );
  }
  const priest_t& priest() const
  {
    return *debug_cast<priest_t*>( ab::player );
  }

  priest_t& p()
  {
    return *debug_cast<priest_t*>( ab::player );
  }

  const priest_t& p() const
  {
    return *debug_cast<priest_t*>( ab::player );
  }

  // typedef for priest_action_t<action_base_t>
  using base_t = priest_action_t;

public:
  priest_action_t( util::string_view name, priest_t& p, const spell_data_t* s = spell_data_t::nil() )
    : ab( name, &p, s ), parse_buff_effects_t( this )
  {
    if ( ab::data().ok() )
    {
      apply_buff_effects();
      apply_debuffs_effects();
    }

    ab::may_crit          = true;
    ab::tick_may_crit     = true;
    ab::weapon_multiplier = 0.0;
  }

  priest_td_t* td( player_t* t ) const
  {
    return p().get_target_data( t );
  }
  priest_td_t& get_td( player_t* t )
  {
    return *( priest().get_target_data( t ) );
  }

  const priest_td_t* find_td( const player_t* t ) const
  {
    return priest().find_target_data( t );
  }

  void trigger_power_of_the_dark_side()
  {
    if ( !priest().talents.discipline.power_of_the_dark_side.enabled() )
      return;

    int stack = priest().buffs.power_of_the_dark_side->check();
    if ( priest().buffs.power_of_the_dark_side->trigger() )
    {
      if ( priest().buffs.power_of_the_dark_side->check() == stack )
      {
        priest().procs.power_of_the_dark_side_overflow->occur();
      }
      else
      {
        priest().procs.power_of_the_dark_side->occur();
      }
    }
  }

  // Syntax: parse_buff_effects[<S[,S...]>]( buff[, ignore_mask|use_stacks[, use_default]][, spell1[,spell2...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit
  //  use_stacks = optional, default true, whether to multiply value by stacks
  //  use_default = optional, default false, whether to use buff's default value over effect's value
  //  S = optional list of template parameter(s) to indicate spell(s) with redirect effects
  //  spell = optional list of spell(s) with redirect effects that modify the effects on the buff
  void apply_buff_effects()
  {
    // using S = const spell_data_t*;

    parse_buff_effects( p().buffs.voidform );
    parse_buff_effects( p().buffs.shadowform );
    parse_buff_effects( p().buffs.twist_of_fate, p().talents.twist_of_fate );
    parse_buff_effects( p().buffs.mind_devourer );
    parse_buff_effects( p().buffs.dark_evangelism, p().talents.shadow.dark_evangelism );
    parse_buff_effects( p().buffs.surge_of_darkness, false );  // Mind Spike instant cast
    parse_buff_effects( p().buffs.mind_melt );                 // Mind Blast instant cast and Crit increase
    // TODO: check why we cant use_default=true to get the value correct
    parse_buff_effects( p().buffs.dark_ascension );  // Buffs corresponding non-periodic spells
    parse_buff_effects( p().buffs.coalescing_shadows );
    parse_buff_effects( p().buffs.coalescing_shadows_dot );
    parse_buff_effects( p().buffs.words_of_the_pious );       // Spell Direct amount for Smite and Holy Nova
    parse_buff_effects( p().buffs.gathering_shadows, true );  // Spell Direct amount for Mind Sear (NOT DP)
    parse_buff_effects( p().buffs.devoured_pride );           // Spell Direct and Periodic amount

    // Discipline
    parse_buff_effects( p().buffs.shadow_covenant, false, true );
    // 280398 applies the buff to the correct spells, but does not contain the correct buff value (12% instead of 40%)
    // So, override to use our provided default_value (40%) instead
    parse_buff_effects( p().buffs.sins_of_the_many, false, true );
  }

  // Syntax: parse_dot_debuffs[<S[,S...]>]( func, spell_data_t* dot[, spell_data_t* spell1[,spell2...] )
  //  func = function returning the dot_t* of the dot
  //  dot = spell data of the dot
  //  S = optional list of template parameter(s) to indicate spell(s)with redirect effects
  //  spell = optional list of spell(s) with redirect effects that modify the effects on the dot
  void apply_debuffs_effects()
  {
    // using S = const spell_data_t*;

    parse_debuff_effects( []( priest_td_t* t ) { return t->buffs.schism->check(); }, p().talents.discipline.schism );
  }

  double cost() const override
  {
    double c = ab::cost() * std::max( 0.0, get_buff_effects_value( cost_buffeffects, false, false ) );
    return c;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t ) * get_debuff_effects_value( td( t ) );
    return tm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double ta = ab::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_buffeffects );
    return ta;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = ab::composite_da_multiplier( s ) * get_buff_effects_value( da_multiplier_buffeffects );
    return da;
  }

  double composite_crit_chance() const override
  {
    double cc = ab::composite_crit_chance() + get_buff_effects_value( crit_chance_buffeffects, true );
    return cc;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = std::max( 0_ms, ab::execute_time() * get_buff_effects_value( execute_time_buffeffects ) );
    return et;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t dd = ab::composite_dot_duration( s ) * get_buff_effects_value( dot_duration_buffeffects );
    return dd;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = ab::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects, false, false );
    return rm;
  }

  void gain_energize_resource( resource_e resource_type, double amount, gain_t* gain ) override
  {
    if ( resource_type == RESOURCE_INSANITY )
    {
      priest().generate_insanity( amount, gain, this );
    }
    else
    {
      ab::gain_energize_resource( resource_type, amount, gain );
    }
  }

private:
  // typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = Base;
};  // namespace actions

struct priest_absorb_t : public priest_action_t<absorb_t>
{
public:
  priest_absorb_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s )
  {
    may_crit      = true;
    tick_may_crit = false;
    may_miss      = false;
  }
};

struct priest_heal_t : public priest_action_t<heal_t>
{
  priest_heal_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s )
  {
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    base_t::impact( s );

    if ( s->result_amount > 0 )
    {
      // TODO: Use proper base_value() from talent struct when fixed
      if ( priest().specialization() != PRIEST_SHADOW && priest().talents.twist_of_fate.enabled() &&
           ( save_health_percentage < priest().talents.twist_of_fate->effectN( 1 ).base_value() ) )
      {
        priest().buffs.twist_of_fate->trigger();
      }
    }
  }
};

struct priest_spell_t : public priest_action_t<spell_t>
{
  bool affected_by_shadow_weaving;
  bool ignores_automatic_mastery;
  timespan_t vf_extension = 0_s;

  priest_spell_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s ), affected_by_shadow_weaving( false ), ignores_automatic_mastery( false )
  {
    weapon_multiplier = 0.0;

    if ( priest().talents.shadow.void_eruption.enabled() )
    {
      vf_extension = timespan_t::from_millis( priest().talents.shadow.void_eruption->effectN( 3 ).base_value() ) /
                     priest().talents.shadow.void_eruption->effectN( 4 ).base_value();
    }
  }

  bool usable_moving() const override
  {
    return base_t::usable_moving();
  }

  bool ready() override
  {
    return base_t::ready();
  }

  void consume_resource() override
  {
    if ( current_resource() == RESOURCE_INSANITY )
      extend_vf( base_cost() );
    base_t::consume_resource();
  }

  void extend_vf( double insanity )
  {
    if ( priest().specialization() == PRIEST_SHADOW && priest().talents.shadow.void_eruption.enabled() &&
         priest().buffs.voidform->up() )
    {
      priest().buffs.voidform->extend_duration( &priest(), vf_extension * insanity );
    }
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // TODO: Use proper base_value() from talent struct when fixed
      if ( priest().specialization() == PRIEST_SHADOW && priest().talents.twist_of_fate.enabled() &&
           ( save_health_percentage < priest().talents.twist_of_fate->effectN( 3 ).base_value() ) )
      {
        priest().buffs.twist_of_fate->trigger();
      }
    }
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = base_t::composite_target_da_multiplier( t );

    if ( affected_by_shadow_weaving )
    {
      // Guarding against Unfurling Darkness, it does not get the mastery benefit
      unsigned int spell_id = id;
      if ( ignores_automatic_mastery )
      {
        sim->print_debug( "{} {} cast does not benefit from Mastery automatically.", *player, name_str );
        spell_id = 1;
      }

      tdm *= priest().shadow_weaving_multiplier( t, spell_id );
    }

    return tdm;
  }

  double composite_target_ta_multiplier( player_t* t ) const override
  {
    double ttm = base_t::composite_target_ta_multiplier( t );

    if ( affected_by_shadow_weaving )
    {
      ttm *= priest().shadow_weaving_multiplier( t, id );
    }

    return ttm;
  }

  void assess_damage( result_amount_type type, action_state_t* s ) override
  {
    base_t::assess_damage( type, s );

    if ( result_is_hit( s->result ) && priest().talents.void_shield.enabled() &&
         priest().buffs.power_word_shield->check() )
      priest().trigger_void_shield( s->result_amount * priest().talents.void_shield->effectN( 1 ).percent() );

    if ( aoe == 0 && result_is_hit( s->result ) && priest().buffs.vampiric_embrace->up() )
      trigger_vampiric_embrace( s );
  }

  /* Based on previous implementation ( pets don't count but get full heal )
   * and https://www.wowhead.com/spell=15286#comments:id=1796701
   * Last checked 2013-05-25
   */
  void trigger_vampiric_embrace( action_state_t* s )
  {
    // TODO: is this additive or multiplicative?
    double amount = s->result_amount;
    amount *= priest().buffs.vampiric_embrace->data().effectN( 1 ).percent();

    if ( priest().talents.sanlayn.enabled() )
    {
      amount *= priest().talents.sanlayn->effectN( 2 ).percent();
    }

    for ( player_t* ally : sim->player_no_pet_list )
    {
      if ( ally->current.sleeping )
      {
        continue;
      }
      ally->resource_gain( RESOURCE_HEALTH, amount, ally->gains.vampiric_embrace );

      for ( pet_t* pet : ally->pet_list )
      {
        pet->resource_gain( RESOURCE_HEALTH, amount, pet->gains.vampiric_embrace );
      }
    }
  }
};

}  // namespace actions

namespace buffs
{
/**
 * This is a template for common code between priest buffs.
 * The template is instantiated with any type of buff ( buff_t, debuff_t,
 * absorb_buff_t, etc. ) as the 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived
 * class,
 * don't skip it and call buff_t/absorb_buff_t/etc. directly.
 */
template <typename Base = buff_t>
struct priest_buff_t : public Base
{
public:
  using base_t = priest_buff_t;  // typedef for priest_buff_t<buff_base_t>

  priest_buff_t( priest_td_t& td, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( td, name, s, item )
  {
  }

  priest_buff_t( priest_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {
  }

protected:
  priest_t& priest()
  {
    return *debug_cast<priest_t*>( Base::source );
  }
  const priest_t& priest() const
  {
    return *debug_cast<priest_t*>( Base::source );
  }
};
}  // namespace buffs

}  // namespace priestspace
