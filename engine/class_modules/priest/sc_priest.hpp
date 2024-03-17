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
struct echoing_void_t;
struct echoing_void_demise_t;
struct shadow_word_death_t;
struct idol_of_cthun_t;
struct shadow_word_pain_t;
struct mental_fortitude_t;
struct expiation_t;
struct purge_the_wicked_t;
struct holy_fire_t;
struct burning_vehemence_t;
}  // namespace actions::spells

namespace actions::heals
{
struct essence_devourer_t;
struct atonement_t;
struct divine_aegis_t;
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
    propagate_const<dot_t*> devouring_plague;
    propagate_const<dot_t*> mind_flay;
    propagate_const<dot_t*> mind_flay_insanity;
    propagate_const<dot_t*> void_torrent;
    propagate_const<dot_t*> purge_the_wicked;
    propagate_const<dot_t*> holy_fire;
  } dots;

  struct buffs_t
  {
    propagate_const<buff_t*> schism;
    propagate_const<buff_t*> death_and_madness_debuff;
    propagate_const<buff_t*> echoing_void;
    propagate_const<buff_t*> echoing_void_collapse;
    propagate_const<buff_t*> apathy;
    propagate_const<buff_t*> psychic_horror;
    buff_t* atonement;
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
    propagate_const<buff_t*> levitate;

    // Talents
    propagate_const<buff_t*> twist_of_fate_heal_self_fake;
    propagate_const<buff_t*> twist_of_fate_heal_ally_fake;
    propagate_const<buff_t*> twist_of_fate;
    propagate_const<buff_t*> rhapsody;
    propagate_const<buff_t*> rhapsody_timer;
    propagate_const<buff_t*> protective_light;
    propagate_const<buff_t*> from_darkness_comes_light;
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
    propagate_const<buff_t*> twilight_equilibrium_holy_amp;
    propagate_const<buff_t*> twilight_equilibrium_shadow_amp;
    propagate_const<buff_t*> harsh_discipline;
    propagate_const<buff_t*> train_of_thought;
    propagate_const<buff_t*> wrath_unleashed;
    propagate_const<buff_t*> weal_and_woe;

    // Holy
    propagate_const<buff_t*> apotheosis;
    propagate_const<buff_t*> empyreal_blaze;
    propagate_const<buff_t*> divine_word;
    propagate_const<buff_t*> divine_favor_chastise;
    propagate_const<buff_t*> divine_image;
    propagate_const<buff_t*> answered_prayers;
    propagate_const<buff_t*> answered_prayers_timer;

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
    propagate_const<buff_t*> shadowy_insight;
    propagate_const<absorb_buff_t*> mental_fortitude;
    propagate_const<buff_t*> insidious_ire;
    propagate_const<buff_t*> thing_from_beyond;
    propagate_const<buff_t*> screams_of_the_void;
    propagate_const<buff_t*> idol_of_yoggsaron;
    propagate_const<buff_t*> devoured_pride;
    propagate_const<buff_t*> devoured_despair;
    propagate_const<buff_t*> devoured_anger;
    propagate_const<buff_t*> dark_evangelism;
    propagate_const<buff_t*> mind_melt;
    propagate_const<buff_t*> surge_of_insanity;
    propagate_const<buff_t*> mind_flay_insanity;
    propagate_const<buff_t*> mind_spike_insanity;
    propagate_const<buff_t*> deathspeaker;
    propagate_const<buff_t*> dark_ascension;
    propagate_const<buff_t*> last_shadowy_apparition_crit;

    // Tier Sets
    propagate_const<buff_t*> gathering_shadows;
    propagate_const<buff_t*> dark_reveries;
    propagate_const<buff_t*> light_weaving;
    propagate_const<buff_t*> darkflame_embers;
    propagate_const<buff_t*> darkflame_shroud;
    propagate_const<buff_t*> deaths_torment;
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
    const spell_data_t* holy_nova_heal;
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
    player_talent_t mental_agility;
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
    player_talent_t benevolence;
    player_talent_t power_word_life;
    player_talent_t angelic_bulwark;
    player_talent_t essence_devourer;
    const spell_data_t* essence_devourer_shadowfiend;
    const spell_data_t* essence_devourer_mindbender;
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
      player_talent_t last_word;
      player_talent_t psychic_horror;
      // Row 4
      player_talent_t thought_harvester;
      player_talent_t psychic_link;
      player_talent_t mind_flay_insanity;
      const spell_data_t* mind_flay_insanity_spell;
      player_talent_t surge_of_insanity;
      const spell_data_t* mind_spike_insanity_spell;
      // Row 5
      player_talent_t shadow_crash;
      player_talent_t unfurling_darkness;
      player_talent_t void_eruption;
      const spell_data_t* void_eruption_damage;
      player_talent_t dark_ascension;
      player_talent_t mental_decay;
      player_talent_t mind_spike;
      // Row 6
      player_talent_t whispering_shadows;
      player_talent_t shadowy_insight;
      player_talent_t ancient_madness;
      player_talent_t voidtouched;
      player_talent_t mind_melt;
      // Row 7
      player_talent_t maddening_touch;
      const spell_data_t* maddening_touch_insanity;
      player_talent_t dark_evangelism;
      player_talent_t mind_devourer;
      player_talent_t phantasmal_pathogen;
      player_talent_t minds_eye;
      player_talent_t distorted_reality;
      // Row 8
      // player_talent_t mindbender; - Shared Talent
      player_talent_t deathspeaker;
      player_talent_t auspicious_spirits;
      player_talent_t void_torrent;
      // Row 9
      // player_talent_t inescapable_torment; - Shared Talent
      player_talent_t mastermind;
      player_talent_t screams_of_the_void;
      player_talent_t tormented_spirits;
      player_talent_t insidious_ire;
      player_talent_t malediction;
      // Row 10
      player_talent_t idol_of_yshaarj;
      const spell_data_t* devoured_pride;
      const spell_data_t* devoured_despair;
      const spell_data_t* devoured_anger;
      const spell_data_t* devoured_fear;
      const spell_data_t* devoured_violence;
      player_talent_t idol_of_nzoth;
      player_talent_t idol_of_yoggsaron;
      player_talent_t idol_of_cthun;
    } shadow;

    struct
    {
      player_talent_t mindbender;
      player_talent_t inescapable_torment;
    } shared;

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
      player_talent_t schism;
      const spell_data_t* schism_debuff;
      // Row 4
      player_talent_t bright_pupil;
      player_talent_t enduring_luminescence;
      player_talent_t shield_discipline;
      player_talent_t luminous_barrier;
      player_talent_t power_word_barrier;
      player_talent_t painful_punishment;
      player_talent_t malicious_intent;
      // Row 5
      player_talent_t purge_the_wicked;
      player_talent_t rapture;
      player_talent_t shadow_covenant;
      const spell_data_t* shadow_covenant_buff;
      const spell_data_t* dark_reprimand;
      // Row 6
      player_talent_t revel_in_purity;
      player_talent_t contrition;
      player_talent_t exaltation;
      player_talent_t indemnity;
      player_talent_t pain_and_suffering;
      player_talent_t twilight_corruption;
      // Row
      player_talent_t borrowed_time;
      player_talent_t castigation;
      player_talent_t abyssal_reverie;
      // Row 8
      player_talent_t train_of_thought;
      player_talent_t ultimate_penance;
      player_talent_t lenience;
      player_talent_t evangelism;
      player_talent_t void_summoner;
      // Row 9
      player_talent_t divine_aegis;
      const spell_data_t* divine_aegis_buff;
      player_talent_t blaze_of_light;
      player_talent_t heavens_wrath;
      player_talent_t harsh_discipline;
      const spell_data_t* harsh_discipline_buff;
      player_talent_t expiation;
      // player_talent_t inescapable_torment; - Shared
      // Row 10
      player_talent_t aegis_of_wrath;
      player_talent_t weal_and_woe;
      const spell_data_t* weal_and_woe_buff;
      player_talent_t overloaded_with_light;
      player_talent_t twilight_equilibrium;
      const spell_data_t* twilight_equilibrium_holy_amp;
      const spell_data_t* twilight_equilibrium_shadow_amp;
      // player_talent_t mindbender; - Shared
    } discipline;

    struct
    {
      // Row 1
      player_talent_t holy_word_serenity;
      // Row 2
      player_talent_t holy_word_chastise;
      // Row 3
      player_talent_t empyreal_blaze;
      player_talent_t holy_word_sanctify;
      // Row 4
      player_talent_t searing_light;
      // Row 8
      player_talent_t apotheosis;
      // Row 9
      player_talent_t burning_vehemence;
      const spell_data_t* burning_vehemence_damage;
      player_talent_t harmonious_apparatus;
      player_talent_t light_of_the_naaru;
      player_talent_t answered_prayers;
      // Row 10
      player_talent_t divine_word;
      const spell_data_t* divine_favor_chastise;
      player_talent_t divine_image;
      const spell_data_t* divine_image_buff;
      const spell_data_t* divine_image_summon;
      const spell_data_t* divine_image_searing_light;
      const spell_data_t* divine_image_light_eruption;
      player_talent_t miracle_worker;
    } holy;

    // Shared
    const spell_data_t* shining_force;

    // Discipline
    const spell_data_t* sins_of_the_many;  // assumes 0 atonement targets
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
    const spell_data_t* penance_channel;
    const spell_data_t* penance_tick;
    const spell_data_t* sins_of_the_many;
    double sins_of_the_many_data[ 20 ] = { 0.2,   0.2,  0.2,   0.2,  0.2,   0.175,  0.15, 0.125,  0.1,     0.075,
                                           0.055, 0.04, 0.025, 0.02, 0.015, 0.0125, 0.01, 0.0075, 0.00625, 0.005 };
    const spell_data_t* smite_t31;

    // Holy
    const spell_data_t* holy_priest;  // General holy data
    const spell_data_t* holy_fire;

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
    const spell_data_t* holy_fire;
    player_talent_t devouring_plague;
  } dot_spells;

  // Mastery Spells
  struct
  {
    const spell_data_t* grace;
    const spell_data_t* shadow_weaving;
  } mastery_spells;

  // Cooldowns
  struct
  {
    // Shared
    propagate_const<cooldown_t*> shadow_word_death;
    propagate_const<cooldown_t*> power_word_shield;
    propagate_const<cooldown_t*> mindgames;
    propagate_const<cooldown_t*> mindbender;
    propagate_const<cooldown_t*> shadowfiend;
    propagate_const<cooldown_t*> fiend;

    // Shadow
    propagate_const<cooldown_t*> void_bolt;
    propagate_const<cooldown_t*> mind_blast;
    propagate_const<cooldown_t*> void_eruption;
    propagate_const<cooldown_t*> maddening_touch_icd;

    // Discipline
    propagate_const<cooldown_t*> penance;

    // Holy
    propagate_const<cooldown_t*> holy_fire;
    propagate_const<cooldown_t*> holy_word_chastise;
    propagate_const<cooldown_t*> holy_word_serenity;
    propagate_const<cooldown_t*> holy_word_sanctify;
  } cooldowns;

  struct realppm_t
  {
    propagate_const<real_ppm_t*> idol_of_cthun;
    propagate_const<real_ppm_t*> deathspeaker;
    propagate_const<real_ppm_t*> power_of_the_dark_side;
  } rppm;

  // Gains
  struct
  {
    propagate_const<gain_t*> insanity_auspicious_spirits;
    propagate_const<gain_t*> insanity_death_and_madness;
    propagate_const<gain_t*> mindbender;
    propagate_const<gain_t*> shadowfiend;
    propagate_const<gain_t*> power_of_the_dark_side;
    propagate_const<gain_t*> power_word_solace;
    propagate_const<gain_t*> throes_of_pain;
    propagate_const<gain_t*> insanity_idol_of_cthun_mind_flay;
    propagate_const<gain_t*> insanity_idol_of_cthun_mind_sear;
    propagate_const<gain_t*> hallucinations_power_word_shield;
    propagate_const<gain_t*> insanity_maddening_touch;
    propagate_const<gain_t*> insanity_t30_2pc;
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
    propagate_const<proc_t*> shadowy_apparition_vb;
    propagate_const<proc_t*> shadowy_apparition_swp;
    propagate_const<proc_t*> shadowy_apparition_dp;
    propagate_const<proc_t*> shadowy_apparition_mb;
    propagate_const<proc_t*> mind_devourer;
    propagate_const<proc_t*> void_tendril;
    propagate_const<proc_t*> void_lasher;
    propagate_const<proc_t*> shadowy_insight;
    propagate_const<proc_t*> shadowy_insight_overflow;
    propagate_const<proc_t*> shadowy_insight_missed;
    propagate_const<proc_t*> thing_from_beyond;
    propagate_const<proc_t*> deathspeaker;
    propagate_const<proc_t*> idol_of_nzoth_swp;
    propagate_const<proc_t*> idol_of_nzoth_vt;
    propagate_const<proc_t*> mind_flay_insanity_wasted;
    propagate_const<proc_t*> idol_of_yshaarj_extra_duration;
    propagate_const<proc_t*> void_torrent_ticks_no_mastery;
    propagate_const<proc_t*> mindgames_casts_no_mastery;
    propagate_const<proc_t*> inescapable_torment_missed_mb;
    propagate_const<proc_t*> inescapable_torment_missed_swd;
    // Holy
    propagate_const<proc_t*> divine_favor_chastise;
    propagate_const<proc_t*> divine_image;
  } procs;

  // Special
  struct
  {
    propagate_const<actions::spells::psychic_link_t*> psychic_link;
    propagate_const<actions::spells::shadow_weaving_t*> shadow_weaving;
    propagate_const<actions::spells::shadowy_apparition_spell_t*> shadowy_apparitions;
    propagate_const<actions::spells::echoing_void_t*> echoing_void;
    propagate_const<actions::spells::shadow_word_death_t*> shadow_word_death;
    propagate_const<actions::spells::echoing_void_demise_t*> echoing_void_demise;
    propagate_const<actions::spells::idol_of_cthun_t*> idol_of_cthun;
    propagate_const<actions::spells::shadow_word_pain_t*> shadow_word_pain;
    propagate_const<actions::spells::mental_fortitude_t*> mental_fortitude;
    propagate_const<actions::spells::expiation_t*> expiation;
    propagate_const<actions::spells::purge_the_wicked_t*> purge_the_wicked;
    propagate_const<action_t*> holy_fire;
    propagate_const<action_t*> searing_light;
    propagate_const<action_t*> light_eruption;
    propagate_const<actions::spells::burning_vehemence_t*> burning_vehemence;
    propagate_const<actions::heals::essence_devourer_t*> essence_devourer;
    propagate_const<actions::heals::atonement_t*> atonement;
    propagate_const<actions::heals::divine_aegis_t*> divine_aegis;
  } background_actions;

  // Items
  struct
  {
  } active_items;

  // Pets
  struct priest_pets_t
  {
    spawner::pet_spawner_t<pet_t, priest_t> shadowfiend;
    spawner::pet_spawner_t<pet_t, priest_t> mindbender;
    spawner::pet_spawner_t<pet_t, priest_t> void_tendril;
    spawner::pet_spawner_t<pet_t, priest_t> void_lasher;
    spawner::pet_spawner_t<pet_t, priest_t> thing_from_beyond;

    priest_pets_t( priest_t& p );
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

    int disc_minimum_allies = 5;

    // Forces Idol of Y'Shaarj to give a particular buff for every cast
    // default, pride, anger, despair, fear (NYI), violence
    std::string forced_yshaarj_type = "default";

    double twist_of_fate_heal_rppm = 2;
    timespan_t twist_of_fate_heal_duration_mean = 2_s;
    timespan_t twist_of_fate_heal_duration_stddev = 0.25_s;
  } options;

  vector_with_callback<player_t*> allies_with_atonement;

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
  double composite_leech() const override;
  void pre_analyze_hook() override;
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
  void demise() override;
  void do_dynamic_regen( bool ) override;
  void apply_affecting_auras( action_t& ) override;
  void apply_affecting_auras_late( action_t& );
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
  void init_background_actions_discipline();
  void init_background_actions_holy();
  std::unique_ptr<expr_t> create_expression_discipline( const util::string_view name_str );
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
  double tick_damage_over_time( timespan_t duration, const dot_t* dot ) const;
  void trigger_inescapable_torment( player_t* target, bool echo = false, double mod = 1.0 );
  void trigger_idol_of_yshaarj( player_t* target );
  void trigger_idol_of_cthun( action_state_t* );
  void trigger_atonement( action_state_t* );
  void trigger_divine_aegis( action_state_t* );
  void spawn_idol_of_cthun( action_state_t* );
  void trigger_shadowy_apparitions( proc_t* proc, bool gets_crit_mod );
  int number_of_echoing_voids_active();
  void trigger_psychic_link( action_state_t* );
  void trigger_shadow_weaving( action_state_t* );
  void trigger_void_shield( double result_amount );
  void refresh_insidious_ire_buff( action_state_t* s );
  void spawn_thing_from_beyond();
  void trigger_idol_of_nzoth( player_t* target, proc_t* proc );
  int shadow_weaving_active_dots( const player_t* target, const unsigned int spell_id ) const;
  double shadow_weaving_multiplier( const player_t* target, const unsigned int spell_id ) const;
  void trigger_essence_devourer();

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
struct priest_action_t : public parse_action_effects_t<Base, priest_t, priest_td_t>
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

    if ( ab::data().ok() )
    {
      const auto spell_powers = ab::data().powers();
      if ( spell_powers.size() == 1 && spell_powers.front().aura_id() == 0 )
      {
        ab::resource_current = spell_powers.front().resource();
      }
      else
      {
        // Find the first power entry without a aura id
        auto it = range::find( spell_powers, 0U, &spellpower_data_t::aura_id );
        if ( it != spell_powers.end() )
        {
          ab::resource_current = it->resource();
        }
        else
        {
          auto it = range::find( spell_powers, p.specialization_aura_id(), &spellpower_data_t::aura_id );
          if ( it != spell_powers.end() )
          {
            ab::resource_current = it->resource();
          }
        }
      }

      for ( const spellpower_data_t& pd : spell_powers )
      {
        if ( pd.aura_id() != 0 && pd.aura_id() != p.specialization_aura_id() )
          continue;

        if ( pd._cost != 0 )
          ab::base_costs[ pd.resource() ] = pd.cost();
        else
          ab::base_costs[ pd.resource() ] = floor( pd.cost() * p.resources.base[ pd.resource() ] );

        ab::secondary_costs[ pd.resource() ] = pd.max_cost();

        if ( pd._cost_per_tick != 0 )
          ab::base_costs_per_tick[ pd.resource() ] = pd.cost_per_tick();
        else
          ab::base_costs_per_tick[ pd.resource() ] = floor( pd.cost_per_tick() * p.resources.base[ pd.resource() ] );
      }

      p.apply_affecting_auras_late( *this );
    }
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
    parse_effects( p().buffs.twist_of_fate, p().talents.twist_of_fate );
    parse_effects( p().buffs.words_of_the_pious );  // Spell Direct amount for Smite and Holy Nova
    parse_effects( p().buffs.rhapsody, p().specs.discipline_priest );

    // SHADOW BUFF EFFECTS
    if ( p().specialization() == PRIEST_SHADOW )
    {
      parse_effects( p().buffs.devoured_pride );                   // Spell Direct and Periodic amount
      parse_effects( p().buffs.voidform, 0x4U, IGNORE_STACKS );  // Skip E3 for AM
      parse_effects( p().buffs.shadowform );
      parse_effects( p().buffs.mind_devourer );
      parse_effects( p().buffs.dark_evangelism, p().talents.shadow.dark_evangelism );
      parse_effects( p().buffs.dark_ascension, 0b1000U, IGNORE_STACKS );  // Buffs non-periodic spells - Skip E4
      parse_effects( p().buffs.mind_melt, p().talents.shadow.mind_melt );  // Mind Blast instant cast and Crit increase
      parse_effects( p().buffs.screams_of_the_void, p().talents.shadow.screams_of_the_void );

      if ( p().talents.shadow.ancient_madness.enabled() )
      {
        // We use DA or VF spelldata to construct Ancient Madness to use the correct spell pass-list
        if ( p().talents.shadow.dark_ascension.enabled() )
        {
          parse_effects( p().buffs.ancient_madness, 0b0001U, USE_DEFAULT );  // Skip E1
        }
        else
        {
          parse_effects( p().buffs.ancient_madness, 0b0011U, USE_DEFAULT );  // Skip E1 and E2
        }
      }

      if ( priest().sets->has_set_bonus( PRIEST_SHADOW, T31, B4 ) )
      {
        parse_effects( p().buffs.deaths_torment );
      }
    }

    // DISCIPLINE BUFF EFFECTS
    if ( p().specialization() == PRIEST_DISCIPLINE )
    {
      parse_effects( p().buffs.shadow_covenant, IGNORE_STACKS, USE_DEFAULT,
                     p().talents.discipline.twilight_corruption );
      // 280398 applies the buff to the correct spells, but does not contain the correct buff value
      // (12% instead of 40%) So, override to use our provided default_value (40%) instead
      parse_effects( p().buffs.sins_of_the_many, IGNORE_STACKS, USE_CURRENT );
      parse_effects( p().buffs.twilight_equilibrium_shadow_amp );
      parse_effects( p().buffs.twilight_equilibrium_holy_amp );
      parse_effects( p().buffs.light_weaving );
      parse_effects( p().buffs.weal_and_woe );
    }

    // HOLY BUFF EFFECTS
    if ( p().specialization() == PRIEST_HOLY )
    {
      parse_effects( p().buffs.divine_favor_chastise );
    }
  }

  // Syntax: parse_target_effects( func, debuff[, spells|ignore_mask][,...] )
  //   (int F(TD*))            func: Function taking the target_data as argument and returning an integer mutiplier
  //   (const spell_data_t*) debuff: Spell data of the debuff
  //
  // The following optional arguments can be used in any order:
  //   (const spell_data_t*) spells: List of spells with redirect effects that modify the effects on the debuff
  //   (unsigned)       ignore_mask: Bitmask to skip effect# n corresponding to the n'th bit
  void apply_debuffs_effects()
  {
    // DISCIPLINE DEBUFF EFFECTS
    if ( p().specialization() == PRIEST_DISCIPLINE )
    {
      parse_target_effects( []( priest_td_t* t ) { return t->buffs.schism->check(); },
                            p().talents.discipline.schism_debuff );
    }
  }

  template <typename... Ts>
  void parse_effects( Ts&&... args ) { ab::parse_effects( std::forward<Ts>( args )... ); }
  template <typename... Ts>
  void parse_target_effects( Ts&&... args ) { ab::parse_target_effects( std::forward<Ts>( args )... ); }

  // Reimplement base cost because I need to bypass the removal of precombat costs
  double cost() const override
  {
    resource_e cr = ab::current_resource();

    double c;
    if ( ab::secondary_costs[ cr ] == 0 )
    {
      c = ab::base_costs[ cr ];
    }
    // For now, treat secondary cost as "maximum of player current resource, min + max cost". Entirely
    // possible we need to add some additional functionality (such as an overridable method) to
    // determine the cost, if the default behavior is not universal.
    else
    {
      if ( ab::player->resources.current[ cr ] >= ab::base_costs[ cr ] )
      {
        c = std::min( ab::base_cost(), ab::player->resources.current[ cr ] );
      }
      else
      {
        c = ab::base_costs[ cr ];
      }
    }

    c -= ab::player->current.resource_reduction[ ab::get_school() ];

    c += ab::cost_flat_modifier();

    c *= ab::get_effects_value( ab::cost_effects, false, false );

    if ( c < 0 )
      c = 0;

    if ( ab::sim->debug )
      ab::sim->out_debug.print( "{} action_t::cost: base_cost={} secondary_cost={} cost={} resource={}", *this,
                                ab::base_costs[ cr ], ab::secondary_costs[ cr ], c, cr );

    return floor( c );
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
  using ab = parse_action_effects_t<Base, priest_t, priest_td_t>;
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
      if ( t != target && ( t->is_active() || t->type == HEALING_ENEMY && !t->is_sleeping() ) )
        target_list.push_back( t );
    }

    for ( const auto& t : sim->healing_pet_list )
    {
      if ( t != target && ( t->is_active() || t->type == HEALING_ENEMY && !t->is_sleeping() ) )
        target_list.push_back( t );
    }

    return target_list.size();
  }
};

struct priest_heal_t : public priest_action_t<heal_t>
{
  bool disc_mastery;

  priest_heal_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s ), disc_mastery( false )
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
      if ( t != target && ( t->is_active() || t->type == HEALING_ENEMY && !t->is_sleeping() ) )
        target_list.push_back( t );
    }

    for ( const auto& t : sim->healing_pet_list )
    {
      if ( t != target && ( t->is_active() || t->type == HEALING_ENEMY && !t->is_sleeping() ) )
        target_list.push_back( t );
    }

    return target_list.size();
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    base_t::impact( s );

    if ( s->result_amount > 0 )
    {
      // TODO: Use proper base_value() from talent struct when fixed
      if ( priest().talents.twist_of_fate.enabled() &&
           ( save_health_percentage < priest().talents.twist_of_fate->effectN( 1 ).base_value() ) )
      {
        priest().buffs.twist_of_fate->trigger();
      }

      if ( s->result == RESULT_CRIT && p().talents.discipline.divine_aegis.enabled() )
      {
        p().trigger_divine_aegis( s );
      }
    }
  }
};

struct priest_spell_t : public priest_action_t<spell_t>
{
  bool affected_by_shadow_weaving;
  bool triggers_atonement;
  bool ignores_automatic_mastery;

  priest_spell_t( util::string_view name, priest_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( name, player, s ),
      affected_by_shadow_weaving( false ),
      triggers_atonement( false ),
      ignores_automatic_mastery( false )
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

  void execute() override
  {
    base_t::execute();
    if ( priest().talents.discipline.twilight_equilibrium.enabled() )
    {
      // Mindbender (123040) and Shadowfiend (34433) don't apply this buff
      // Non-harmful actions don't apply this buff
      if ( school == SCHOOL_SHADOW && id != 34433 && id != 123040 && harmful && !background )
      {
        priest().buffs.twilight_equilibrium_holy_amp->trigger();
        priest().buffs.twilight_equilibrium_shadow_amp->expire();
      }
      // Holy and Radiant (SCHOOL_HOLYFIRE) applies this buff
      // Non-harmful actions don't apply this buff
      if ( ( school == SCHOOL_HOLY || school == SCHOOL_HOLYFIRE ) && harmful && !background )
      {
        priest().buffs.twilight_equilibrium_shadow_amp->trigger();
        priest().buffs.twilight_equilibrium_holy_amp->expire();
      }
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

      if ( triggers_atonement && s->chain_target == 0 )
        p().trigger_atonement( s );
    }
  }

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( triggers_atonement && result_is_hit( d->state->result ) )
    {
      p().trigger_atonement( d->state );
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
struct symbol_of_hope_t : public buff_t
{
  // This is player_t* instead of priest_t* because it can be any type of player.
  symbol_of_hope_t( player_t* p );
  void update_cooldowns( int new_ );

private:
  bool affected_actions_initialized;
  std::vector<std::pair<action_t*, double>> affected_actions;
};

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
