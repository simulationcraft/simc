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
#include "action/parse_effects.hpp"
#include "player/pet_spawner.hpp"
#include "sc_enums.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
/* Forward declarations
 */
struct priest_t;

template <typename Data, typename Base = action_state_t>
struct priest_action_state_t : public Base, public Data
{
  static_assert( std::is_base_of_v<action_state_t, Base> );
  static_assert( std::is_default_constructible_v<Data> );  // required for initialize
  static_assert( std::is_copy_assignable_v<Data> );        // required for copy_state

  using Base::Base;

  void initialize() override
  {
    Base::initialize();
    *static_cast<Data*>( this ) = Data{};
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    Base::debug_str( s );
    if constexpr ( fmt::is_formattable<Data>::value )
      fmt::print( s, " {}", *static_cast<const Data*>( this ) );
    return s;
  }

  void copy_state( const action_state_t* o ) override
  {
    Base::copy_state( o );
    *static_cast<Data*>( this ) = *static_cast<const Data*>( static_cast<const priest_action_state_t*>( o ) );
  }
};

namespace actions::spells
{
struct shadowy_apparition_spell_t;
struct psychic_link_t;
struct shadow_weaving_t;
struct shadow_word_death_t;
struct idol_of_cthun_t;
struct shadow_word_pain_t;
struct mental_fortitude_t;
struct expiation_t;
struct purge_the_wicked_t;
struct holy_fire_t;
struct burning_vehemence_t;
struct entropic_rift_t;
struct entropic_rift_damage_t;
struct collapsing_void_damage_t;
struct halo_t;
struct horrific_vision_t;
struct vision_of_nzoth_t;
struct void_apparition_spell_t;
struct void_bolt_t;
struct shadeburst_t;
}  // namespace actions::spells

namespace actions::heals
{
struct atonement_t;
struct divine_aegis_t;
struct crystalline_reflection_heal_t;
struct crystalline_reflection_damage_t;
struct echo_of_light_t;
}  // namespace actions::heals

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
    propagate_const<dot_t*> shadow_word_madness;
    propagate_const<dot_t*> mind_flay;
    propagate_const<dot_t*> mind_flay_insanity;
    propagate_const<dot_t*> void_torrent;
    propagate_const<dot_t*> purge_the_wicked;
    propagate_const<dot_t*> holy_fire;
  } dots;

  struct buffs_t
  {
    propagate_const<buff_t*> death_and_madness_debuff;
    buff_t* atonement;
    propagate_const<buff_t*> resonant_energy;
    propagate_const<buff_t*> horrific_visions;
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

// utility to create target_effect_t compatible functions from priest_td_t member references
template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, priest_td_t::buffs_t> )
  {
    if ( stack )
      return
          [ d ]( actor_target_data_t* t ) { return std::invoke( d, static_cast<priest_td_t*>( t )->buffs )->check(); };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<priest_td_t*>( t )->buffs )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, priest_td_t::dots_t> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<priest_td_t*>( t )->dots )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<priest_td_t*>( t )->dots )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of priest_td_t" );
    return nullptr;
  }
}

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
    propagate_const<buff_t*> levitate;
    propagate_const<buff_t*> power_infusion;

    // Talents
    propagate_const<buff_t*> twist_of_fate_heal_self_fake;
    propagate_const<buff_t*> twist_of_fate_heal_ally_fake;
    propagate_const<buff_t*> twist_of_fate;
    propagate_const<buff_t*> protective_light;
    propagate_const<buff_t*> death_and_madness_reset;
    propagate_const<buff_t*> vampiric_embrace;
    propagate_const<buff_t*> surge_of_light;

    // Discipline
    propagate_const<buff_t*> inner_focus;
    propagate_const<buff_t*> power_of_the_dark_side;
    propagate_const<buff_t*> borrowed_time;
    propagate_const<buff_t*> revel_in_purity;
    propagate_const<buff_t*> harsh_discipline;
    propagate_const<buff_t*> train_of_thought;
    propagate_const<buff_t*> wrath_unleashed;
    propagate_const<buff_t*> weal_and_woe;
    propagate_const<buff_t*> evangelism;
    propagate_const<buff_t*> archangel;
    propagate_const<buff_t*> holy_ray;
    propagate_const<buff_t*> greater_smite;

    // Holy
    propagate_const<buff_t*> apotheosis;
    propagate_const<buff_t*> empyreal_blaze;
    propagate_const<buff_t*> divine_favor_chastise;
    propagate_const<buff_t*> divine_image;

    // Shadow
    propagate_const<buff_t*> dispersion;
    propagate_const<buff_t*> shadowform;
    propagate_const<buff_t*> shadowform_state;  // Dummy buff to track whether player entered Shadowform initially
    propagate_const<buff_t*> void_torrent;
    propagate_const<buff_t*> voidform;
    propagate_const<buff_t*> mind_devourer;
    propagate_const<buff_t*> shadowy_insight;
    propagate_const<absorb_buff_t*> mental_fortitude;
    propagate_const<buff_t*> insidious_ire;
    propagate_const<buff_t*> thing_from_beyond;
    propagate_const<buff_t*> screams_of_the_void;
    propagate_const<buff_t*> idol_of_yoggsaron;
    propagate_const<buff_t*> mind_flay_insanity;
    propagate_const<buff_t*> idol_of_yshaarj;
    propagate_const<buff_t*> shattered_psyche;
    propagate_const<buff_t*> void_volley;
    propagate_const<buff_t*> horrific_vision;
    propagate_const<buff_t*> vision_of_nzoth;
    propagate_const<buff_t*> crushing_void;
    propagate_const<buff_t*> ancient_madness_extension;
    propagate_const<buff_t*> ancient_madness;
    propagate_const<buff_t*> vampiric_insight;  // mid_s2_4pc buff

    // Archon
    propagate_const<buff_t*> power_surge;
    propagate_const<buff_t*> sustained_potency;
    propagate_const<buff_t*> resonant_energy_healing;
    propagate_const<buff_t*> resonant_energy_damage;

    // Voidweaver
    propagate_const<buff_t*> voidheart;
    propagate_const<buff_t*> entropic_rift;
    propagate_const<buff_t*> darkening_horizon;
    propagate_const<buff_t*> collapsing_void;
  } buffs;

  // Talents
  struct
  {
    // Priest Tree
    // Row 1
    player_talent_t improved_flash_heal;
    player_talent_t angelic_feather;
    player_talent_t mind_blast;
    player_talent_t holy_fire;
    // Row 2
    player_talent_t holy_nova;
    const spell_data_t* holy_nova_heal;
    player_talent_t dispel_magic;
    player_talent_t spiritual_guidance;
    player_talent_t psychic_scream;
    // Row 3
    player_talent_t lightburst;
    player_talent_t leap_of_faith;
    player_talent_t purify_disease;
    player_talent_t power_infusion;
    player_talent_t painful_invocation;
    player_talent_t sheer_terror;
    player_talent_t petrifying_scream;
    // Row 4
    player_talent_t surge_of_light;
    const spell_data_t* surge_of_light_buff;
    player_talent_t body_and_soul;
    player_talent_t mass_dispel;
    player_talent_t twins_of_the_sun_priestess;
    player_talent_t strength_of_soul;
    player_talent_t mind_control;
    player_talent_t dominate_mind;
    player_talent_t psychic_voice;
    player_talent_t void_tendrils;
    // Row 5
    player_talent_t everlasting_light;
    player_talent_t move_with_grace;
    player_talent_t mental_agility;
    player_talent_t twin_disciplines;
    player_talent_t dark_enlightenment;
    player_talent_t false_autonomy;
    player_talent_t shackle_undead;
    // Row 6
    player_talent_t inspiration;
    player_talent_t binding_heals;
    player_talent_t shadow_word_death;
    const spell_data_t* shadow_word_death_self_damage;
    player_talent_t sanguine_teachings;
    // Row 7
    player_talent_t desperate_prayer;
    player_talent_t twist_of_fate;
    const spell_data_t* twist_of_fate_buff;
    player_talent_t tithe_evasion;
    player_talent_t fade;
    // Row 8
    player_talent_t angels_mercy;
    player_talent_t protective_light;
    const spell_data_t* protective_light_buff;
    player_talent_t mindpierce;
    player_talent_t spectral_illusion;
    player_talent_t improved_fade;
    // Row 9
    player_talent_t lights_inspiration;
    player_talent_t unwavering_will;
    player_talent_t spell_warding;
    player_talent_t phantasm;
    // Row 10
    player_talent_t angelic_bulwark;
    player_talent_t benevolence;
    player_talent_t focused_power;
    player_talent_t phantom_reach;
    player_talent_t translucent_image;

    struct
    {
      player_talent_t mindbender;
      player_talent_t inescapable_torment;
      player_talent_t shadowfiend;
      player_talent_t deaths_torment;
    } shared;

    struct
    {
      // Shadow Tree
      // Row 2
      player_talent_t psychic_link;
      player_talent_t misery;
      player_talent_t invoked_nightmares;
      player_talent_t intangibility;
      player_talent_t mental_fortitude;
      // Row 3
      player_talent_t thought_harvester;
      player_talent_t tentacle_slam;
      const spell_data_t* tentacle_slam_damage;
      const spell_data_t* shadowy_apparition;  // Damage event
      player_talent_t shadowy_apparitions;     // Passive effect
      // Row 4
      player_talent_t tormenting_whispers;
      player_talent_t descending_darkness;
      player_talent_t surge_of_insanity;
      // Row 5
      player_talent_t shadowy_insight;
      player_talent_t voidtouched;
      player_talent_t voidform;
      const spell_data_t* void_volley;
      const spell_data_t* void_volley_buff;
      const spell_data_t* void_volley_missile;
      const spell_data_t* void_volley_damage;
      player_talent_t haunting_shadows;
      player_talent_t mental_decay;
      // Row 6
      player_talent_t dark_thoughts;
      player_talent_t maddening_touch;
      const spell_data_t* maddening_touch_insanity;
      player_talent_t improved_voidform;
      player_talent_t ancient_madness;
      const spell_data_t* ancient_madness_buff;
      player_talent_t phantom_menace;
      player_talent_t shadeburst;
      const spell_data_t* shadeburst_spell;
      player_talent_t dark_evangelism;
      player_talent_t shattered_psyche;
      // Row 7
      player_talent_t subservient_shadows;
      player_talent_t mastermind;
      player_talent_t minds_eye;
      player_talent_t distorted_reality;
      player_talent_t spectral_horrors;
      player_talent_t instilled_doubt;
      // Row 8
      player_talent_t deathspeaker;
      player_talent_t death_and_madness;
      const spell_data_t* death_and_madness_insanity;
      const spell_data_t* death_and_madness_reset_buff;
      player_talent_t mind_devourer;
      player_talent_t auspicious_spirits;
      player_talent_t maddening_tentacles;
      // Row 9
      player_talent_t madness_weaving;
      // Death's Torment (Shared)
      player_talent_t screams_of_the_void;
      player_talent_t tormented_spirits;
      player_talent_t insidious_ire;
      player_talent_t crushing_void;
      const spell_data_t* crushing_void_buff;
      // Row 10
      player_talent_t idol_of_yshaarj;
      const spell_data_t* idol_of_yshaarj_buff;
      const spell_data_t* overburdened_mind;
      player_talent_t idol_of_nzoth;
      const spell_data_t* horrific_visions;        // enemy debuff
      const spell_data_t* horrific_vision_damage;  // 50 stack damage
      const spell_data_t* vision_of_nzoth_damage;  // 100 stack damage
      const spell_data_t* horrific_vision_buff;    // 50 stack buff
      const spell_data_t* vision_of_nzoth_buff;    // 100 stack buff
      player_talent_t idol_of_yoggsaron;
      player_talent_t idol_of_cthun;
      // Apex
      player_talent_t void_apparitions_1;
      player_talent_t void_apparitions_2;
      const spell_data_t* void_apparition;  // Damage event
      const spell_data_t* void_bolt;
      player_talent_t void_apparitions_3;
    } shadow;

    struct
    {
      // Row 1
      player_talent_t atonement;
      const spell_data_t* atonement_buff;
      const spell_data_t* atonement_spell;
      // Row 2
      player_talent_t power_word_radiance;
      player_talent_t pain_suppression;
      player_talent_t power_of_the_dark_side;
      // Row 3
      player_talent_t lights_promise;
      player_talent_t sanctuary;
      player_talent_t pain_transformation;
      player_talent_t protector_of_the_frail;
      player_talent_t dark_indulgence;
      player_talent_t encroaching_shadows;
      // Row 4
      player_talent_t bright_pupil;
      player_talent_t enduring_luminescence;
      player_talent_t shield_discipline;
      player_talent_t ultimate_penitence;
      player_talent_t power_word_barrier;
      player_talent_t painful_punishment;
      player_talent_t revel_in_darkness;
      // Row 5
      player_talent_t holy_ray;
      const spell_data_t* holy_ray_buff;  // 1235193
      player_talent_t lenience;
      player_talent_t shadow_tap;
      // Row 6
      player_talent_t purge_the_wicked;
      player_talent_t castigation;
      player_talent_t indemnity;
      player_talent_t pain_and_suffering;
      player_talent_t occultist;
      // Row 7
      player_talent_t harsh_discipline;
      const spell_data_t* harsh_discipline_buff;
      player_talent_t evangelism;
      player_talent_t abyssal_reverie;
      // Row 8
      player_talent_t divine_procession;
      player_talent_t inner_focus;
      player_talent_t archangel;
      const spell_data_t* archangel_buff;  // 81700
      // Mindbender (Shared)
      player_talent_t shadow_mend;
      // Shadowfiend (Shared)
      // Row 9
      player_talent_t greater_smite;
      const spell_data_t* greater_smite_buff;  // 1253725
      player_talent_t divine_aegis;
      const spell_data_t* divine_aegis_buff;
      player_talent_t borrowed_time;
      player_talent_t blaze_of_light;
      // Deaths Torment (Shared)
      // Inescapable Torment (Shared)
      // Row 10
      player_talent_t eternal_barrier;
      player_talent_t weal_and_woe;
      const spell_data_t* weal_and_woe_buff;
      player_talent_t searing_light;
      const spell_data_t* searing_light_dot;  // 1280134
      player_talent_t expiation;
      // Apex
      player_talent_t master_the_darkness_1;
      player_talent_t master_the_darkness_2;
      player_talent_t master_the_darkness_3;
      const spell_data_t* void_shield;          // 1253593
      const spell_data_t* void_shield_reflect;  // 1253828
    } discipline;

    struct
    {
      // Row 1
      player_talent_t holy_word_serenity;
      // Row 2
      player_talent_t holy_word_sanctify;
      player_talent_t guardian_spirit;
      player_talent_t holy_word_chastise;
      // Row 3
      player_talent_t prayer_of_healing;
      player_talent_t restitution;
      player_talent_t guardian_angel;
      player_talent_t censure;
      player_talent_t empyreal_blaze;
      const spell_data_t* empyreal_blaze_buff;
      // Row 4
      player_talent_t prayerful_litany;
      player_talent_t cosmic_ripple;
      player_talent_t afterlife;
      player_talent_t voice_of_harmony;
      player_talent_t burning_vehemence;
      const spell_data_t* burning_vehemence_damage;
      // Row 5
      player_talent_t uplifting_words;
      player_talent_t cosmic_wave;
      player_talent_t divine_hymn;
      player_talent_t enlightenment;
      player_talent_t benediction;
      // Row 6
      player_talent_t efficient_prayers;
      player_talent_t healing_focus;
      player_talent_t seraphic_crescendo;
      player_talent_t gales_of_song;
      player_talent_t divine_service;
      player_talent_t renewed_faith;
      // Row 7
      player_talent_t angelic_touch;
      player_talent_t apotheosis;
      player_talent_t prayers_of_the_virtuous;
      // Row 8
      player_talent_t dispersing_light;
      player_talent_t trail_of_light;
      player_talent_t miracle_worker;
      player_talent_t eternal_sanctity;
      player_talent_t divinity;
      player_talent_t holy_celerity;
      player_talent_t say_your_prayers;
      // Row 9
      player_talent_t crisis_management;
      player_talent_t light_of_the_naaru;
      player_talent_t light_in_the_darkness;
      player_talent_t prismatic_echoes;
      player_talent_t desperate_times;
      player_talent_t radiant_plea;
      // Row 10
      player_talent_t lightweaver;
      player_talent_t ultimate_serenity;
      player_talent_t divine_image;
      const spell_data_t* divine_image_buff;
      const spell_data_t* divine_image_summon;
      const spell_data_t* divine_image_searing_light;
      const spell_data_t* divine_image_light_eruption;
      player_talent_t lasting_words;
      const spell_data_t* divine_favor_chastise;
      player_talent_t epiphany;
    } holy;

    struct
    {
      player_talent_t halo;
      const spell_data_t* halo_heal_holy;
      const spell_data_t* halo_dmg_holy;
      const spell_data_t* halo_heal_shadow;
      const spell_data_t* halo_dmg_shadow;
      player_talent_t perfected_form;
      player_talent_t power_surge;
      const spell_data_t* power_surge_buff;
      player_talent_t manifested_power;
      player_talent_t energy_conservation;
      const spell_data_t* mind_flay_insanity;
      const spell_data_t* mind_flay_insanity_spell;
      const spell_data_t* mind_flay_insanity_buff;
      player_talent_t shock_pulse;
      player_talent_t incessant_screams;
      player_talent_t word_of_supremacy;
      player_talent_t heightened_alteration;
      player_talent_t empowered_surges;
      player_talent_t spiritwell;
      player_talent_t realized_potential;
      player_talent_t ascendant_prayers;
      player_talent_t energy_compression;
      player_talent_t sustained_potency;
      const spell_data_t* sustained_potency_buff;
      player_talent_t resonant_energy;
      const spell_data_t* resonant_energy_shadow;
      const spell_data_t* resonant_energy_healing;
      const spell_data_t* resonant_energy_damage;
      player_talent_t energy_cycle;
      player_talent_t focused_outburst;
      player_talent_t divine_halo;
    } archon;

    struct
    {
      player_talent_t guiding_light;
      player_talent_t preventive_measures;
      player_talent_t preemptive_care;
      player_talent_t waste_no_time;
      player_talent_t words_of_the_wise;
      player_talent_t assured_safety;
      player_talent_t divine_feathers;
      player_talent_t save_the_day;
      player_talent_t forseen_circumstances;
      player_talent_t prophets_insight;
      player_talent_t prophets_will;
      player_talent_t desperate_measures;
      player_talent_t prompt_prognosis;
      player_talent_t piety;
      player_talent_t unfolding_vision;
      player_talent_t twinsight;
      const spell_data_t* twinsight_healing;  // 1232567
      const spell_data_t* twinsight_damage;   // 1232571
    } oracle;

    struct
    {
      player_talent_t void_torrent;   // shadow only
      player_talent_t entropic_rift;  // discipline only
      const spell_data_t* entropic_rift_aoe;
      const spell_data_t* entropic_rift_damage;
      const spell_data_t* entropic_rift_driver;
      const spell_data_t* entropic_rift_object;
      player_talent_t no_escape;
      player_talent_t dark_energy;
      player_talent_t void_blast;
      const spell_data_t* void_blast_shadow;
      const spell_data_t* void_blast_disc;
      player_talent_t inner_quietus;
      player_talent_t voidheart;
      const spell_data_t* voidheart_buff;
      player_talent_t devour_matter;
      player_talent_t void_empowerment;
      const spell_data_t* void_empowerment_buff;
      player_talent_t darkening_horizon;
      player_talent_t voidwraith;
      const spell_data_t* voidwraith_spell;
      player_talent_t touch_of_the_void;
      player_talent_t quickened_pulse;
      player_talent_t void_infusion;
      player_talent_t void_leech;
      player_talent_t embrace_the_shadow;
      player_talent_t overwhelming_shadows;
      player_talent_t collapsing_void;
      const spell_data_t* collapsing_void_damage;
    } voidweaver;

    struct
    {
      const spell_data_t* mindgames;
      const spell_data_t* mindgames_healing_reversal;
      const spell_data_t* mindgames_damage_reversal;
    } pvp;
  } talents;

  // Specialization Spells
  struct
  {
    const spell_data_t* priest;  // General priest data
    const spell_data_t* levitate_buff;

    // Discipline
    const spell_data_t* discipline_priest;  // General discipline data
    const spell_data_t* penance;
    const spell_data_t* penance_channel;
    const spell_data_t* penance_tick;
    const spell_data_t* contrition_heal;       // 270501
    const spell_data_t* contrition_heal_crit;  // 281469
    const spell_data_t* plea;                  // 200829
    const spell_data_t* renew;
    const spell_data_t* prayer_of_mending;

    // Holy
    const spell_data_t* holy_priest;  // General holy data
    const spell_data_t* echo_of_light;

    // Shadow
    const spell_data_t* mind_flay;
    const spell_data_t* shadow_priest;  // General shadow data
    const spell_data_t* shadowform;
    const spell_data_t* voidform;
    const spell_data_t* hallucinations;
    const spell_data_t* dispersion;
    const spell_data_t* silence;
    const spell_data_t* vampiric_embrace;
    const spell_data_t* vampiric_insight_buff;  // mid_s2_4pc
  } specs;

  // DoT Spells
  struct
  {
    const spell_data_t* shadow_word_pain;
    const spell_data_t* vampiric_touch;
    const spell_data_t* holy_fire;
    player_talent_t shadow_word_madness;
  } dot_spells;

  // Mastery Spells
  struct
  {
    const spell_data_t* grace;
    const spell_data_t* shadow_weaving;
    const spell_data_t* echo_of_light;
  } mastery_spells;

  // Cooldowns
  struct
  {
    // Shared
    propagate_const<cooldown_t*> shadow_word_death;
    propagate_const<cooldown_t*> power_word_shield;

    // Shadow
    propagate_const<cooldown_t*> mind_blast;
    propagate_const<cooldown_t*> maddening_touch_icd;
    propagate_const<cooldown_t*> void_volley;

    // Discipline
    propagate_const<cooldown_t*> penance;
    propagate_const<cooldown_t*> ultimate_penitence;

    // Holy
    propagate_const<cooldown_t*> holy_word_chastise;
    propagate_const<cooldown_t*> holy_word_serenity;
    propagate_const<cooldown_t*> holy_word_sanctify;
  } cooldowns;

  struct realppm_t
  {
    propagate_const<real_ppm_t*> idol_of_cthun;
    propagate_const<real_ppm_t*> power_of_the_dark_side;
  } rppm;

  struct threshold_rngs_t
  {
    threshold_rng_t* shadowy_insight;
    threshold_rng_t* maddening_touch;
    threshold_rng_t* tormented_spirits;
    threshold_rng_t* auspicious_spirits;
  } threshold_rng;

  struct deck_rngs_t
  {
    shuffled_rng_t* random_idol;
    shuffled_rng_t* master_of_darkness;
  } deck_rng;

  // Gains
  struct
  {
    propagate_const<gain_t*> insanity_auspicious_spirits;
    propagate_const<gain_t*> insanity_death_and_madness;
    propagate_const<gain_t*> mindbender;
    propagate_const<gain_t*> shadowfiend;
    propagate_const<gain_t*> voidwraith;
    propagate_const<gain_t*> power_of_the_dark_side;
    propagate_const<gain_t*> insanity_idol_of_cthun_mind_flay;
    propagate_const<gain_t*> insanity_idol_of_cthun_mind_sear;
    propagate_const<gain_t*> hallucinations_power_word_shield;
    propagate_const<gain_t*> insanity_maddening_touch;
    propagate_const<gain_t*> shield_discipline;
    propagate_const<gain_t*> insanity_dark_thoughts;
    propagate_const<gain_t*> insanity_horrific_vision;
    propagate_const<gain_t*> insanity_vision_of_nzoth;
    propagate_const<gain_t*> insanity_vampiric_insight;  // mid_s2_4pc gain
  } gains;

  // Benefits
  struct
  {
  } benefits;

  // Procs
  struct
  {
    // Discipline
    propagate_const<proc_t*> power_of_the_dark_side;
    propagate_const<proc_t*> power_of_the_dark_side_overflow;
    propagate_const<proc_t*> power_of_the_dark_side_dark_indulgence_overflow;
    propagate_const<proc_t*> expiation_lost_no_dot;
    // Shadow
    propagate_const<proc_t*> shadowy_apparition_swp;
    propagate_const<proc_t*> shadowy_apparition_swm;
    propagate_const<proc_t*> shadowy_apparition_mb;
    propagate_const<proc_t*> shadowy_apparition_mfi;
    propagate_const<proc_t*> shadowy_apparition_yshaarj;
    propagate_const<proc_t*> shadowy_apparition_nzoth;
    propagate_const<proc_t*> shadowy_apparition_yogg;
    propagate_const<proc_t*> shadowy_apparition_cthun;
    propagate_const<proc_t*> shadowy_apparition_vampiric_insight;  // mid_s2_4pc proc
    propagate_const<proc_t*> mind_devourer;
    propagate_const<proc_t*> void_tendril;
    propagate_const<proc_t*> void_lasher;
    propagate_const<proc_t*> shadowy_insight;
    propagate_const<proc_t*> shadowy_insight_overflow;
    propagate_const<proc_t*> shadowy_insight_missed;
    propagate_const<proc_t*> thing_from_beyond;
    propagate_const<proc_t*> mind_flay_insanity_wasted;
    propagate_const<proc_t*> void_torrent_ticks_no_mastery;
    propagate_const<proc_t*> mindgames_casts_no_mastery;
    propagate_const<proc_t*> inescapable_torment_missed_mb;
    propagate_const<proc_t*> inescapable_torment_missed_swd;
    propagate_const<proc_t*> shadowfiend;
    propagate_const<proc_t*> void_apparition;
    propagate_const<proc_t*> void_apparition_yshaarj;
    propagate_const<proc_t*> void_apparition_horrific_vision;
    propagate_const<proc_t*> void_apparition_vision_of_nzoth;
    propagate_const<proc_t*> void_apparition_yogg;
    propagate_const<proc_t*> void_apparition_cthun;
    propagate_const<proc_t*> tentacle_slam_idol;
    // Holy
    propagate_const<proc_t*> divine_favor_chastise;
    propagate_const<proc_t*> divine_image;
  } procs;

  struct sample_data_t
  {
    std::unique_ptr<extended_sample_data_t> voidform_duration;
  } sample_data;

  // Special
  struct
  {
    propagate_const<actions::spells::psychic_link_t*> psychic_link;
    propagate_const<actions::spells::shadow_weaving_t*> shadow_weaving;
    propagate_const<actions::spells::shadowy_apparition_spell_t*> shadowy_apparitions;
    propagate_const<actions::spells::void_apparition_spell_t*> void_apparitions;
    propagate_const<actions::spells::shadow_word_death_t*> shadow_word_death;
    propagate_const<actions::spells::idol_of_cthun_t*> idol_of_cthun;
    propagate_const<actions::spells::shadow_word_pain_t*> shadow_word_pain;
    propagate_const<actions::spells::mental_fortitude_t*> mental_fortitude;
    propagate_const<actions::spells::expiation_t*> expiation;
    propagate_const<actions::spells::purge_the_wicked_t*> purge_the_wicked;
    propagate_const<action_t*> searing_light;
    propagate_const<action_t*> light_eruption;
    propagate_const<actions::spells::burning_vehemence_t*> burning_vehemence;
    propagate_const<actions::heals::atonement_t*> atonement;
    propagate_const<actions::heals::divine_aegis_t*> divine_aegis;
    propagate_const<actions::spells::entropic_rift_damage_t*> entropic_rift_damage;
    propagate_const<actions::spells::entropic_rift_t*> entropic_rift;
    propagate_const<actions::spells::collapsing_void_damage_t*> collapsing_void;
    propagate_const<actions::spells::halo_t*> halo;
    propagate_const<actions::heals::crystalline_reflection_heal_t*> crystalline_reflection_heal;
    propagate_const<actions::heals::crystalline_reflection_damage_t*> crystalline_reflection_damage;
    propagate_const<action_t*> echo_of_light;
    propagate_const<actions::spells::horrific_vision_t*> horrific_vision;
    propagate_const<actions::spells::vision_of_nzoth_t*> vision_of_nzoth;
    propagate_const<actions::spells::void_bolt_t*> void_bolt;
    propagate_const<actions::spells::shadeburst_t*> shadeburst;
  } background_actions;

  // Items
  struct
  {
  } active_items;

  // Player Data State
  vector_with_callback<player_t*> allies_with_atonement;
  struct state_t
  {
    player_t* last_entropic_rift_target;
  } state;

  // Pets
  struct priest_pets_t
  {
    spawner::pet_spawner_t<pet_t, priest_t> shadowfiend;
    spawner::pet_spawner_t<pet_t, priest_t> mindbender;
    spawner::pet_spawner_t<pet_t, priest_t> voidwraith;
    spawner::pet_spawner_t<pet_t, priest_t> void_tendril;
    spawner::pet_spawner_t<pet_t, priest_t> void_lasher;
    spawner::pet_spawner_t<pet_t, priest_t> thing_from_beyond;

    priest_pets_t( priest_t& p );
    void set_pet_defaults( priest_t& p );
  } pets;

  // Options
  struct
  {
    // Default param to set if you should cast Power Infusion on yourself
    bool self_power_infusion = true;

    // Add in options to override insanity gained
    // Mindgames gives 20 insanity from the healing and 20 from damage dealt
    // For most content the healing part won't proc, only default damage dealt
    bool mindgames_healing_reversal = false;
    bool mindgames_damage_reversal  = true;

    // Time in seconds between prayer of mending bounces
    double prayer_of_mending_bounce_rate = 2;

    // Option whether or not to start with higher than 0 Insanity based on talents
    // Only takes into account if you have not overriden initial_resource=insanity=X to something greater than 0
    bool init_insanity = true;

    double twist_of_fate_heal_rppm                = 0.0;
    timespan_t twist_of_fate_heal_duration_mean   = 2_s;
    timespan_t twist_of_fate_heal_duration_stddev = 0.25_s;

    // Force enables Devour Matter if the talent is active for all casts of Shadow Word: Death
    bool force_devour_matter = false;

    // Chance for Entropic Rift ticks to miss all targets and deal no damage
    // Can be used to account for boss movement
    double entropic_rift_miss_percent = 0.05;
    // Can be used to account for add movement
    double entropic_rift_miss_percent_secondary = 0.1;
    // Can be used to limit the number of enemies hit
    int entropic_rift_miss_target_cap = 0;

    // Additional Crystalline Reflection Damage Multiplier (Because its bugged and doesnt always do full damage)
    double crystalline_reflection_damage_mult = 0.5;
    bool no_channel_macro_mfi                 = false;

    // Controls whether Discipline is "in a raid" or not.
    bool discipline_in_raid = false;

    // 30% Chance that a Fire Mage steals the proc because you are slow or you just dont hit it or it just bugs out.
    double synergistic_brewterializer_tof_chance = 0.7;
    // ~20% damage penalty to account for GCD. ~10% Miss general chance.
    double synergistic_brewterializer_barrel_hit_chance = 0.75;

    // Chance for outgoing Halo damage pulses to hit (Divine Halo / Archon).
    double archon_halo_outgoing_hit_chance = 0.5;
    // Chance for returning Halo damage pulses to hit (Divine Halo / Archon).
    double archon_halo_return_hit_chance = 0.5;
  } options;

  priest_t( sim_t* sim, util::string_view name, race_e r );

  // player_t overrides
  void init_base_stats() override;
  void init_resources( bool force ) override;
  void init_spells() override;
  void init_special_effects() override;
  void init_special_effects_shadow();
  void create_buffs() override;
  void init_scaling() override;
  void init_finished() override;
  void init_background_actions() override;
  bool validate_actor() override;
  void reset() override;
  void create_options() override;
  std::string create_profile( save_e ) override;
  action_t* create_action( util::string_view name, util::string_view options ) override;
  void create_pets() override;
  void copy_from( player_t* source ) override;
  void merge( player_t& ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void assess_damage( school_e school, result_amount_type dtype, action_state_t* s ) override;
  double composite_mastery_value() const override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_spell_crit_chance() const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_absorb_multiplier( const action_state_t* s ) const override;
  double composite_player_heal_multiplier( const action_state_t* s ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* t, school_e school ) const override;
  double composite_leech() const override;
  double composite_attribute_multiplier( attribute_e ) const override;
  double composite_mitigation_multiplier( const action_state_t*, school_e, bool direct ) const override;
  void pre_analyze_hook() override;
  void analyze( sim_t& sim ) override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void init_action_list() override;
  void init_blizzard_action_list() override;
  void parse_assisted_combat_step( const assisted_combat_step_data_t& step,
                                   action_priority_list_t* assisted_combat ) override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t& rule,
                                                            const assisted_combat_step_data_t& step ) const override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  std::string aura_expr_from_spell_id( unsigned int spell_id, bool on_self ) const override;
  void combat_begin() override;
  void init_uptimes() override;
  void init_rng() override;
  const priest_td_t* find_target_data( const player_t* target ) const override;
  priest_td_t* get_target_data( player_t* target ) const override;
  std::unique_ptr<expr_t> create_expression( util::string_view expression_str ) override;

  void arise() override;
  void demise() override;
  void do_dynamic_regen( bool ) override;
  void invalidate_cache( cache_e ) override;
  void init_items() override;

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
  void init_background_actions_discipline();
  void init_background_actions_holy();
  std::unique_ptr<expr_t> create_expression_discipline( const util::string_view name_str );
  action_t* create_action_discipline( util::string_view name, util::string_view options_str );

  void create_buffs_holy();
  void init_spells_holy();
  void init_rng_holy();
  expr_t* create_expression_holy( action_t* a, util::string_view name_str );
  action_t* create_action_holy( util::string_view name, util::string_view options_str );
  target_specific_t<priest_td_t> _target_data;

public:
  void do_holy_word_cdr( cooldown_t* cd, timespan_t amount, bool affected_by_apotheosis = true,
                         bool affected_by_naaru = true );
  double generate_insanity( double num_amount, gain_t* g, action_t* action );
  double tick_damage_over_time( timespan_t duration, const dot_t* dot ) const;
  void trigger_inescapable_torment( player_t* target, bool echo = false, double mod = 1.0 );
  void idol_of_yshaarj_check_and_expire();
  void trigger_idol_of_cthun( action_state_t* );
  void trigger_atonement( action_state_t*, double );
  void trigger_divine_aegis( action_state_t* );
  void spawn_idol_of_cthun( action_state_t* );
  void trigger_shadowy_apparitions( proc_t* proc, player_t* target, double apparition_damage_mod = 1.0 );
  void trigger_psychic_link( action_state_t* );
  void trigger_shadow_weaving( action_state_t* );
  void trigger_ancient_madness( int stacks );
  void trigger_ancient_madness_extension();
  void refresh_insidious_ire_buff( action_state_t* s );
  void spawn_thing_from_beyond();
  void trigger_idol_of_nzoth( player_t* target, int stacks );
  double shadow_weaving_active_dots( const player_t* target, const unsigned int spell_id ) const;
  double shadow_weaving_multiplier( const player_t* target, const unsigned int spell_id ) const;
  // Stores the currently active Entropic Rift event
  void trigger_entropic_rift();
  void extend_entropic_rift();
  void expand_entropic_rift( int stacks = -1 );
  std::string blizzard_apl_action_replace( std::string options );
  void trigger_random_idol( action_state_t* s );
  void trigger_horrific_vision( player_t* target );
  void trigger_vision_of_nzoth( player_t* target );
  void trigger_shadowy_insight( bool guaranteed = false, action_state_t* s = nullptr );
  void trigger_idol_of_yshaarj();

  std::vector<action_t*> secondary_action_list;

  template <typename T, typename... Ts>
  std::pair<T*, bool> get_secondary_action_pair( std::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return { dynamic_cast<T*>( *it ), false };

    auto a        = new T( *this, std::forward<Ts>( args )... );
    a->background = true;
    secondary_action_list.push_back( a );
    return { a, true };
  }
  template <typename T, typename... Ts>
  T* get_secondary_action( std::string_view n, Ts&&... args )
  {
    auto it = range::find( secondary_action_list, n, &action_t::name_str );
    if ( it != secondary_action_list.cend() )
      return dynamic_cast<T*>( *it );

    auto a        = new T( *this, std::forward<Ts>( args )... );
    a->background = true;
    secondary_action_list.push_back( a );
    return a;
  }

  unsigned int specialization_aura_id()
  {
    switch ( specialization() )
    {
      case PRIEST_SHADOW:
        return specs.shadow_priest->id();
        break;
      case PRIEST_DISCIPLINE:
        return specs.discipline_priest->id();
        break;
      case PRIEST_HOLY:
        return specs.holy_priest->id();
        break;
      default:
        return specs.priest->id();
        break;
    }
  }

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
struct priest_action_t : public parse_action_effects_t<Base>
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
    : ab( name, &p, s )
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

    if ( priest().rppm.power_of_the_dark_side->trigger() )
    {
      int stack = priest().buffs.power_of_the_dark_side->check();

      priest().buffs.power_of_the_dark_side->trigger();
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

  // Syntax: parse_effects( data[, spells|condition|ignore_mask|flags|spells][,...] )
  //   (buff_t*) or
  //   (const spell_data_t*)   data: Buff or spell to be checked for to see if effect applies. If buff is used, effect
  //                                 will require the buff to be active. If spell is used, effect will always apply
  //                                 unless an optional condition function is provided.
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the buff
  //   (bool F())         condition: Function that takes no arguments and returns true if the effect should apply
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  //   (parse_flag_e)         flags: Various flags to control how the value is calculated when the action executes
  //                    USE_DEFAULT: Use the buff's default value instead of spell effect data value
  //                    USE_CURRENT: Use the buff's current value instead of spell effect data value
  //                  IGNORE_STACKS: Ignore stacks of the buff and don't multiply the value
  //
  // Example 1: Parse buff1, ignore effects #1 #3 #5, modify by talent1, modify by tier1:
  //   parse_effects( buff1, 0b10101U, talent1, tier1 );
  //
  // Example 2: Parse buff2, don't multiply by stacks, use the default value set on the buff instead of effect value:
  //   parse_effects( buff2, false, USE_DEFAULT );
  //
  // Example 3: Parse spell1, modify by talent1, only apply if my_player_t::check1() returns true:
  //   parse_effects( spell1, talent1, &my_player_t::check1 );
  //
  // Example 4: Parse buff3, only apply if my_player_t::check2() and my_player_t::check3() returns true:
  //   parse_effects( buff3, [ this ] { return p()->check2() && p()->check3(); } );
  void apply_buff_effects()
  {
    // GENERAL PRIEST BUFF EFFECTS
    parse_effects( p().buffs.twist_of_fate );
    parse_effects( p().buffs.surge_of_light, IGNORE_STACKS );

    // ARCHON BUFF EFFECTS
    if ( p().talents.archon.resonant_energy.enabled() && p().is_ptr() )
    {
      if ( p().specialization() == PRIEST_SHADOW )
        parse_effects( p().buffs.resonant_energy_damage );
      else if ( p().specialization() == PRIEST_HOLY )
        parse_effects( p().buffs.resonant_energy_healing );
    }

    // SHADOW BUFF EFFECTS
    if ( p().specialization() == PRIEST_SHADOW )
    {
      parse_effects( p().buffs.voidform, effect_mask_t( true ).disable( 3 ), IGNORE_STACKS );  // Skip E3 for AM
      parse_effects( p().buffs.shadowform );
      parse_effects( p().buffs.mind_devourer );
      parse_effects( p().buffs.shattered_psyche );     // Mind Blast critical strike chance
      parse_effects( p().buffs.screams_of_the_void );  // Buffs non-periodic spells
    }

    // DISCIPLINE BUFF EFFECTS
    if ( p().specialization() == PRIEST_DISCIPLINE )
    {
      parse_effects( p().buffs.weal_and_woe );
      parse_effects( p().buffs.archangel );
      parse_effects( p().buffs.holy_ray );
      parse_effects( p().buffs.evangelism );
      parse_effects( p().buffs.greater_smite );
    }

    // HOLY BUFF EFFECTS
    if ( p().specialization() == PRIEST_HOLY )
    {
      parse_effects( p().buffs.divine_favor_chastise );
    }
  }

  // Syntax: parse_target_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer multiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  void apply_debuffs_effects()
  {
    // Archon (non-PTR)
    if ( p().talents.archon.resonant_energy.enabled() && !p().is_ptr() )
    {
      parse_target_effects( d_fn( &priest_td_t::buffs_t::resonant_energy, true ),
                            p().talents.archon.resonant_energy_shadow );
    }
  }

  template <typename... Ts>
  void parse_effects( Ts&&... args )
  {
    ab::parse_effects( std::forward<Ts>( args )... );
  }
  template <typename... Ts>
  void parse_target_effects( Ts&&... args )
  {
    ab::parse_target_effects( std::forward<Ts>( args )... );
  }

  // Reimplement base cost because I need to bypass the removal of precombat costs
  double cost() const override
  {
    auto cr        = ab::current_resource();
    const auto& bc = ab::base_costs[ cr ];
    auto base      = bc.base;
    auto add       = bc.flat_add + this->cost_flat_modifier();
    auto mul       = bc.pct_mul * this->cost_pct_multiplier();

    double c = ( base + add ) * mul;

    // For now, treat max cost as "maximum of player current resource, min + max cost". Entirely possible we need
    // to add some additional functionality (such as an overridable method) to determine the cost, if the default
    // behavior is not universal.

    // Also for now, cost reductions to base cost are assumed to not apply to max cost, such that the 'min' cost
    // can be modified but the 'max' cost cannot. There are currently no spells with max cost that gets their cost
    // modified so this assumption remains untested. Fix accordingly if it is proven incorrect in the future.

    if ( auto sec = ab::max_base_costs[ cr ] )
    {
      auto cur = ab::player->resources.current[ cr ];
      if ( cur >= c )
      {
        c = std::min( c + sec, cur );
      }
    }

    if ( c < 0 )
      c = 0;

    if ( ab::sim->debug )
    {
      ab::sim->out_debug.print( "{} action_t::cost: base={} add={} mul={} secondary_cost={} cost={} resource={}", *this,
                                base, add, mul, ab::max_base_costs[ cr ], c, cr );
    }

    return c;
  }

  double gain_energize_resource( resource_e resource_type, double amount, gain_t* gain ) override
  {
    if ( resource_type == RESOURCE_INSANITY )
    {
      return priest().generate_insanity( amount, gain, this );
    }
    else
    {
      return ab::gain_energize_resource( resource_type, amount, gain );
    }
  }

private:
  // typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = parse_action_effects_t<Base>;
};  // namespace actions

struct priest_absorb_t : public priest_action_t<absorb_t>
{
  bool disc_mastery;

public:
  priest_absorb_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s ), disc_mastery( false )
  {
    may_crit      = true;
    tick_may_crit = false;
    may_miss      = false;
    target        = &player;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto cdm = base_t::composite_da_multiplier( s );

    if ( disc_mastery && p().mastery_spells.grace->ok() && p().find_target_data( s->target ) &&
         p().find_target_data( s->target )->buffs.atonement->check() )
    {
      cdm *= 1 + p().cache.mastery_value();
    }

    return cdm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    auto ctm = base_t::composite_ta_multiplier( s );

    if ( disc_mastery && p().mastery_spells.grace->ok() && p().find_target_data( s->target ) &&
         p().find_target_data( s->target )->buffs.atonement->check() )
    {
      ctm *= 1 + p().cache.mastery_value();
    }

    return ctm;
  }

  size_t available_targets( std::vector<player_t*>& target_list ) const override
  {
    target_list.clear();
    target_list.push_back( target );

    for ( const auto& t : sim->healing_no_pet_list )
    {
      if ( t != target && ( t->is_active() || ( t->type == HEALING_ENEMY && !t->is_sleeping() ) ) )
        target_list.push_back( t );
    }

    // Remove non Healing Enemy pets from valid target list
    for ( const auto& t : sim->healing_pet_list )
    {
      if ( t != target && ( ( t->type == HEALING_ENEMY && !t->is_sleeping() ) ) )
        target_list.push_back( t );
    }

    return target_list.size();
  }

  void execute() override
  {
    base_t::execute();

    if ( priest().talents.surge_of_light.enabled() )
      priest().buffs.surge_of_light->trigger();
  }
};

struct priest_heal_t : public priest_action_t<heal_t>
{
  bool disc_mastery;
  bool holy_mastery;
  bool divine_aegis;

  priest_heal_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s ), disc_mastery( false ), holy_mastery( false ), divine_aegis( true )
  {
    target = &player;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    auto cdm = base_t::composite_da_multiplier( s );

    if ( disc_mastery && p().mastery_spells.grace->ok() && p().find_target_data( s->target ) &&
         p().find_target_data( s->target )->buffs.atonement->check() )
    {
      cdm *= 1 + p().cache.mastery_value();
    }

    return cdm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    auto ctm = base_t::composite_ta_multiplier( s );

    if ( disc_mastery && p().mastery_spells.grace->ok() && p().find_target_data( s->target ) &&
         p().find_target_data( s->target )->buffs.atonement->check() )
    {
      ctm *= 1 + p().cache.mastery_value();
    }

    return ctm;
  }

  size_t available_targets( std::vector<player_t*>& target_list ) const override
  {
    target_list.clear();
    target_list.push_back( target );

    for ( const auto& t : sim->healing_no_pet_list )
    {
      if ( t != target && ( t->is_active() || ( t->type == HEALING_ENEMY && !t->is_sleeping() ) ) )
        target_list.push_back( t );
    }

    // Remove non Healing Enemy pets from valid target list
    for ( const auto& t : sim->healing_pet_list )
    {
      if ( t != target && ( ( t->type == HEALING_ENEMY && !t->is_sleeping() ) ) )
        target_list.push_back( t );
    }

    return target_list.size();
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    base_t::impact( s );

    if ( s->result_total > 0 )
    {
      if ( priest().specialization() == PRIEST_HOLY && holy_mastery )
      {
        residual_action::trigger( p().background_actions.echo_of_light, s->target,
                                  s->result_total * p().composite_mastery_value() );
      }
    }

    if ( s->result_amount > 0 )
    {
      // TODO: Use proper base_value() from talent struct when fixed
      if ( priest().talents.twist_of_fate.enabled() &&
           ( save_health_percentage < priest().talents.twist_of_fate->effectN( 1 ).base_value() ) )
      {
        priest().buffs.twist_of_fate->trigger();
      }

      if ( s->result == RESULT_CRIT && divine_aegis && p().talents.discipline.divine_aegis.enabled() )
      {
        p().trigger_divine_aegis( s );
      }
    }
  }

  void execute() override
  {
    base_t::execute();

    if ( priest().talents.surge_of_light.enabled() )
      priest().buffs.surge_of_light->trigger();
  }
};

struct priest_spell_t : public priest_action_t<spell_t>
{
  bool affected_by_shadow_weaving;
  bool triggers_atonement;
  bool ignores_automatic_mastery;
  int idol_of_nzoth_execute_stacks;
  int idol_of_nzoth_impact_stacks;
  int idol_of_nzoth_tick_stacks;

  priest_spell_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s ),
      affected_by_shadow_weaving( false ),
      triggers_atonement( false ),
      ignores_automatic_mastery( false ),
      idol_of_nzoth_execute_stacks( 0 ),
      idol_of_nzoth_impact_stacks( 0 ),
      idol_of_nzoth_tick_stacks( 0 )
  {
    weapon_multiplier = 0.0;

    track_cd_waste = data().cooldown() > 0_ms || data().charge_cooldown() > 0_ms;
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
    base_t::consume_resource();
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );
  }

  virtual double composite_atonement_multiplier( action_state_t* s )
  {
    double mul = p().talents.discipline.atonement->effectN( 1 ).percent();

    if ( !p().options.discipline_in_raid )
      mul *= 1 + p().talents.discipline.atonement->effectN( 3 ).percent();

    if ( p().talents.discipline.abyssal_reverie.enabled() &&
         ( dbc::get_school_mask( s->action->school ) & SCHOOL_SHADOW ) != SCHOOL_SHADOW )
      mul *= 1 + p().talents.discipline.abyssal_reverie->effectN( 1 ).percent();

    if ( p().talents.voidweaver.voidheart.enabled() && p().buffs.voidheart->check() )
      mul *= 1.0 + p().talents.voidweaver.voidheart->effectN( 2 ).percent();

    return mul;
  }

  void execute() override
  {
    base_t::execute();

    if ( priest().talents.shadow.idol_of_nzoth.enabled() && idol_of_nzoth_execute_stacks > 0 )
    {
      priest().trigger_idol_of_nzoth( target, idol_of_nzoth_execute_stacks );
    }
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    base_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // TODO: Use proper base_value() from talent struct when fixed
      if ( priest().talents.twist_of_fate.enabled() &&
           ( save_health_percentage < priest().talents.twist_of_fate->effectN( 3 ).base_value() ) )
      {
        priest().buffs.twist_of_fate->trigger();
      }

      if ( triggers_atonement && ( s->chain_target == 0 || split_aoe_damage ) )
      {
        p().trigger_atonement( s, composite_atonement_multiplier( s ) );
      }

      if ( priest().talents.shadow.idol_of_nzoth.enabled() && idol_of_nzoth_impact_stacks > 0 )
      {
        priest().trigger_idol_of_nzoth( target, idol_of_nzoth_impact_stacks );
      }
    }
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( triggers_atonement && result_is_hit( d->state->result ) )
    {
      p().trigger_atonement( d->state, composite_atonement_multiplier( d->state ) );
    }

    if ( priest().talents.shadow.idol_of_nzoth.enabled() && idol_of_nzoth_tick_stacks > 0 )
    {
      priest().trigger_idol_of_nzoth( d->target, idol_of_nzoth_tick_stacks );
    }
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = base_t::composite_target_da_multiplier( t );

    if ( affected_by_shadow_weaving )
    {
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

    for ( player_t* ally : sim->player_no_pet_list )
    {
      if ( ally->current.sleeping )
      {
        continue;
      }
      // TODO: re-write as a proper heal (15290 or 199397)
      ally->resource_gain( RESOURCE_HEALTH, amount );

      for ( pet_t* pet : ally->pet_list )
      {
        pet->resource_gain( RESOURCE_HEALTH, amount );
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
  priest_buff_t( actor_pair_t q, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( q, name, s, item )
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
