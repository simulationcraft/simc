// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
// This file contains all definitions for priests. Implementations should 
// be done on sc_priest.cpp if they are shared by more than one spec or
// in the respective spec file if they are limited to one spec only.

#pragma once
#include "simulationcraft.hpp"

namespace priest
{
  /* Forward declarations
  */
  struct priest_t;
  namespace actions
  {
    namespace spells
    {
      struct mind_sear_tick_t;
      struct shadowy_apparition_spell_t;
      struct sphere_of_insanity_spell_t;
      struct blessed_dawnlight_medallion_t;
    }  // namespace spells
  }  // namespace actions

  namespace pets
  {
  }

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
      propagate_const<dot_t*> holy_fire;
      propagate_const<dot_t*> power_word_solace;
    } dots;

    struct buffs_t
    {
      propagate_const<buff_t*> schism;
    } buffs;

    priest_t& priest;

    priest_td_t(player_t* target, priest_t& p);
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
      // Talents
      propagate_const<buff_t*> power_infusion;
      propagate_const<buff_t*> twist_of_fate;

      // Discipline
      propagate_const<buff_t*> archangel;
      propagate_const<buff_t*> borrowed_time;
      propagate_const<buff_t*> holy_evangelism;
      propagate_const<buff_t*> inner_focus;

      // Holy

      // Shadow
      propagate_const<buff_t*> dispersion;
      propagate_const<buff_t*> insanity_drain_stacks;
      propagate_const<buff_t*> lingering_insanity;
      propagate_const<buff_t*> shadowform;
      propagate_const<buff_t*> shadowform_state;  // Dummy buff to track whether player entered Shadowform initially
      propagate_const<buff_t*> shadowy_insight;
      propagate_const<buff_t*> sphere_of_insanity;
      propagate_const<buff_t*> surrender_to_madness;
      propagate_const<buff_t*> surrender_to_madness_death;
      propagate_const<buff_t*> vampiric_embrace;
      propagate_const<buff_t*> void_ray;
      propagate_const<buff_t*> void_torrent;
      propagate_const<buff_t*> voidform;

      // Set Bonus
      propagate_const<stat_buff_t*> power_overwhelming;  // T19OH
      propagate_const<buff_t*> void_vb;                  // T19 Shadow 4pc
      propagate_const<buff_t*> empty_mind;               // T20 Shadow 2pc
      propagate_const<buff_t*> overwhelming_darkness;    // T21 Shadow 4pc

                                                         // Legion Legendaries
      haste_buff_t* sephuzs_secret;
      // Shadow
      propagate_const<buff_t*> anunds_last_breath;       // Anund's Seared Shackles stack counter
      propagate_const<buff_t*> the_twins_painful_touch;  // To track first casting
      propagate_const<buff_t*> zeks_exterminatus;        // Aura for Zeks proc
      propagate_const<buff_t*> iridis_empowerment;       // Fake aura for Helm

                                                         // Artifact
      propagate_const<buff_t*> mind_quickening;
    } buffs;

    // Talents
    struct
    {
      const spell_data_t* shining_force;
      const spell_data_t* divine_star;
      const spell_data_t* halo;

      const spell_data_t* twist_of_fate;

      const spell_data_t* angelic_feather;

      const spell_data_t* psychic_voice;

      const spell_data_t* power_infusion;
      const spell_data_t* mindbender;

      // Discipline
      const spell_data_t* the_penitent;
      const spell_data_t* castigation;
      const spell_data_t* schism;

      const spell_data_t* body_and_soul;
      const spell_data_t* masochism;

      const spell_data_t* power_word_solace;
      const spell_data_t* shield_discipline;
      const spell_data_t* dominant_mind;

      const spell_data_t* sanctuary;
      const spell_data_t* clarity_of_will;
      const spell_data_t* shadow_covenant;

      const spell_data_t* purge_the_wicked;

      const spell_data_t* grace;
      const spell_data_t* evangelism;

      // Holy

      const spell_data_t* trail_of_light;
      const spell_data_t* enduring_renewal;
      const spell_data_t* enlightenment;

      const spell_data_t* body_and_mind; // NYI    
      const spell_data_t* perseverance;  // NYI

      const spell_data_t* light_of_the_naaru;
      const spell_data_t* guardian_angel;
      const spell_data_t* after_life;   // NYI

      const spell_data_t* censure;      // NYI

      const spell_data_t* surge_of_light;
      const spell_data_t* binding_heal;
      const spell_data_t* circle_of_healing;

      const spell_data_t* benediction;

      const spell_data_t* cosmic_ripple;
      const spell_data_t* apotheosis;
      const spell_data_t* holy_word_salvation;

      // Shadow
      // T15
      const spell_data_t* fortress_of_the_mind;
      const spell_data_t* shadow_word_void;
      const spell_data_t* shadowy_insight;
      // T30
      const spell_data_t* mania;
      const spell_data_t* sanlayn;     // NYI
      // T45
      const spell_data_t* dark_void;
      const spell_data_t* misery;
      // T60
      const spell_data_t* last_word;   // NYI
      const spell_data_t* mind_bomb;
      const spell_data_t* psychic_horror; // NYI
      // T75
      const spell_data_t* auspicious_spirits;
      const spell_data_t* shadow_word_death;
      const spell_data_t* shadow_crash;
      // T90
      const spell_data_t* lingering_insanity;
      // T100
      const spell_data_t* legacy_of_the_void;
      const spell_data_t* void_torrent;
      const spell_data_t* surrender_to_madness;
    } talents;

    // Artifacts
    struct artifact_spell_data_t
    {
      // Shadow - Xal'atath, Blade of the Black Empire
      artifact_power_t call_to_the_void;
      artifact_power_t creeping_shadows;
      artifact_power_t darkening_whispers;
      artifact_power_t deaths_embrace;
      artifact_power_t from_the_shadows;
      artifact_power_t mass_hysteria;
      artifact_power_t mental_fortitude;
      artifact_power_t mind_shattering;
      artifact_power_t sinister_thoughts;
      artifact_power_t sphere_of_insanity;
      artifact_power_t thoughts_of_insanity;   // NYI
      artifact_power_t thrive_in_the_shadows;  // NYI
      artifact_power_t to_the_pain;
      artifact_power_t touch_of_darkness;
      artifact_power_t unleash_the_shadows;
      artifact_power_t void_corruption;
      artifact_power_t void_siphon;
      artifact_power_t void_torrent;
      artifact_power_t darkness_of_the_conclave;
      artifact_power_t fiending_dark;
      artifact_power_t mind_quickening;
      artifact_power_t lash_of_insanity;
    } artifact;

    struct legendary_t
    {
      const spell_data_t* sephuzs_secret;

      legendary_t() : sephuzs_secret(spell_data_t::not_found())
      {
      }
    } legendary;

    // Specialization Spells
    struct
    {
      // Discipline
      const spell_data_t* archangel;
      const spell_data_t* atonement;
      const spell_data_t* borrowed_time;
      const spell_data_t* divine_aegis;
      const spell_data_t* evangelism;
      const spell_data_t* grace;
      const spell_data_t* meditation_disc;
      const spell_data_t* mysticism;
      const spell_data_t* spirit_shell;
      const spell_data_t* enlightenment;

      // Holy
      const spell_data_t* meditation_holy;
      const spell_data_t* rapid_renewal;
      const spell_data_t* serendipity;
      const spell_data_t* divine_providence;

      const spell_data_t* focused_will;

      // Shadow
      const spell_data_t* shadowy_apparitions;
      const spell_data_t* voidform;
      const spell_data_t* void_eruption;
      const spell_data_t* shadow_priest;
    } specs;

    // Mastery Spells
    struct
    {
      const spell_data_t* absolution;  // NYI
      const spell_data_t* echo_of_light;
      const spell_data_t* madness;
    } mastery_spells;

    // Cooldowns
    struct
    {
      propagate_const<cooldown_t*> chakra;
      propagate_const<cooldown_t*> mindbender;
      propagate_const<cooldown_t*> penance;
      propagate_const<cooldown_t*> power_word_shield;
      propagate_const<cooldown_t*> shadowfiend;
      propagate_const<cooldown_t*> silence;

      propagate_const<cooldown_t*> mind_blast;
      //propagate_const<cooldown_t*> shadow_word_death;
      propagate_const<cooldown_t*> shadow_word_void;
      propagate_const<cooldown_t*> void_bolt;
      propagate_const<cooldown_t*> mind_bomb;
      propagate_const<cooldown_t*> sephuzs_secret;
    } cooldowns;

    // Gains
    struct
    {
      propagate_const<gain_t*> mindbender;
      propagate_const<gain_t*> power_word_solace;
      propagate_const<gain_t*> insanity_auspicious_spirits;
      propagate_const<gain_t*> insanity_dispersion;
      propagate_const<gain_t*> insanity_void_torrent;
      propagate_const<gain_t*> insanity_drain;
      propagate_const<gain_t*> insanity_mind_blast;
      propagate_const<gain_t*> insanity_mind_flay;
      propagate_const<gain_t*> insanity_mind_sear;
      propagate_const<gain_t*> insanity_pet;
      propagate_const<gain_t*> insanity_power_infusion;
      propagate_const<gain_t*> insanity_shadow_crash;
      propagate_const<gain_t*> insanity_shadow_word_death;
      propagate_const<gain_t*> insanity_shadow_word_pain_ondamage;
      propagate_const<gain_t*> insanity_shadow_word_pain_onhit;
      propagate_const<gain_t*> insanity_shadow_word_void;
      propagate_const<gain_t*> insanity_surrender_to_madness;
      propagate_const<gain_t*> insanity_vampiric_touch_ondamage;
      propagate_const<gain_t*> insanity_vampiric_touch_onhit;
      propagate_const<gain_t*> insanity_void_bolt;
      propagate_const<gain_t*> insanity_blessing;
      propagate_const<gain_t*> shadowy_insight;
      propagate_const<gain_t*> vampiric_touch_health;
      propagate_const<gain_t*> insanity_call_to_the_void;

    } gains;

    // Benefits
    struct
    {
    } benefits;

    // Procs
    struct
    {
      propagate_const<proc_t*> legendary_zeks_exterminatus;
      propagate_const<proc_t*> legendary_zeks_exterminatus_overflow;
      propagate_const<proc_t*> legendary_anunds_last_breath;
      propagate_const<proc_t*> legendary_anunds_last_breath_overflow;
      propagate_const<proc_t*> shadowy_insight;
      propagate_const<proc_t*> shadowy_insight_overflow;
      propagate_const<proc_t*> serendipity;
      propagate_const<proc_t*> serendipity_overflow;
      propagate_const<proc_t*> shadowy_apparition;
      propagate_const<proc_t*> shadowy_apparition_overflow;
      propagate_const<proc_t*> surge_of_light;
      propagate_const<proc_t*> surge_of_light_overflow;
      propagate_const<proc_t*> void_eruption_has_dots;
      propagate_const<proc_t*> void_eruption_no_dots;
      propagate_const<proc_t*> void_tendril;
    } procs;

    struct realppm_t
    {
      propagate_const<real_ppm_t*> call_to_the_void;
      propagate_const<real_ppm_t*> shadowy_insight;
    } rppm;

    // Special
    struct
    {
      propagate_const<actions::spells::mind_sear_tick_t*> mind_sear_tick;
      propagate_const<actions::spells::shadowy_apparition_spell_t*> shadowy_apparitions;
      propagate_const<action_t*> sphere_of_insanity;
      propagate_const<action_t*> mental_fortitude;
      propagate_const<action_t*> void_tendril;
    } active_spells;

    struct
    {
      // Legion Legendaries Shadow
      const special_effect_t* anunds_seared_shackles;     // wrist
      const special_effect_t* mother_shahrazs_seduction;  // shoulder
      const special_effect_t* mangazas_madness;           // belt
      const special_effect_t* zenkaram_iridis_anadem;     // helm
      const special_effect_t* the_twins_painful_touch;    // ring
      const special_effect_t* zeks_exterminatus;          // cloak
      const special_effect_t* sephuzs_secret;             // ring
      const special_effect_t* heart_of_the_void;          // chest
    } active_items;

    // Pets
    struct
    {
      propagate_const<pet_t*> shadowfiend;
      propagate_const<pet_t*> mindbender;
    } pets;

    // Options
    struct
    {
      bool autoUnshift = true;  // Shift automatically out of stance/form
      bool priest_fixed_time = true;
      bool priest_ignore_healing = false;  // Remove Healing calculation codes
      bool priest_suppress_sephuz = false;  // Sephuz's Secret won't proc if set true
      int priest_set_voidform_duration = 0; // Voidform will always have this duration
    } options;

    priest_t(sim_t* sim, const std::string& name, race_e r);

    priest_td_t* find_target_data(player_t* target) const;

    // player_t overrides
    void init_base_stats() override;
    void init_resources(bool force) override;
    void init_spells() override;
    void create_buffs() override;
    void init_scaling() override;
    void reset() override;
    void create_options() override;
    std::string create_profile(save_e = SAVE_ALL) override;
    action_t* create_action(const std::string& name, const std::string& options) override;
    virtual action_t* create_proc_action(const std::string& name, const special_effect_t& effect) override;
    pet_t* create_pet(const std::string& name, const std::string& type = std::string()) override;
    void create_pets() override;
    void copy_from(player_t* source) override;
    resource_e primary_resource() const override
    {
      return RESOURCE_MANA;
    }
    role_e primary_role() const override;
    stat_e convert_hybrid_stat(stat_e s) const override;
    stat_e primary_stat() const override { return STAT_INTELLECT; }
    void assess_damage(school_e school, dmg_e dtype, action_state_t* s) override;
    double composite_melee_haste() const override;
    double composite_melee_speed() const override;
    double composite_spell_haste() const override;
    double composite_spell_speed() const override;
    double composite_player_multiplier(school_e school) const override;
    double composite_player_pet_damage_multiplier(const action_state_t*) const override;
    double composite_player_absorb_multiplier(const action_state_t* s) const override;
    double composite_player_heal_multiplier(const action_state_t* s) const override;
    double composite_player_target_multiplier(player_t* t, school_e school) const override;
    double matching_gear_multiplier(attribute_e attr) const override;
    void target_mitigation(school_e, dmg_e, action_state_t*) override;
    void pre_analyze_hook() override;
    void init_action_list() override;
    void combat_begin() override;
    void init_rng() override;
    priest_td_t* get_target_data(player_t* target) const override;
    expr_t* create_expression(action_t* a, const std::string& name_str) override;
    void trigger_sephuzs_secret(const action_state_t* state, spell_mechanic mechanic, double proc_chance = -1.0);
    void trigger_call_to_the_void(const dot_t* d);

    void do_dynamic_regen() override
    {
      player_t::do_dynamic_regen();

      // Drain insanity, and adjust the time when all insanity is depleted from the actor
      insanity.drain();
      insanity.adjust_end_event();
    }

  private:
    void create_cooldowns();
    void create_gains();
    void create_procs();
    void create_benefits();
    void create_apl_precombat();
    void create_apl_default();
    void fixup_atonement_stats(const std::string& trigger_spell_name, const std::string& atonement_spell_name);

    void create_buffs_shadow();
    void init_rng_shadow();
    void init_spells_shadow();
    void generate_apl_shadow();
    action_t* create_action_shadow( const std::string& name, const std::string& options_str );

    void create_buffs_discipline();
    void init_spells_discipline();
    void init_rng_discipline();
    void generate_apl_discipline_d();
    void generate_apl_discipline_h();
    action_t* create_action_discipline( const std::string& name, const std::string& options_str );

    void create_buffs_holy();
    void init_spells_holy();
    void init_rng_holy();
    void generate_apl_holy_d();
    void generate_apl_holy_h();
    action_t* create_action_holy( const std::string& name, const std::string& options_str );

    target_specific_t<priest_td_t> _target_data;

  public:
    void generate_insanity(double num_amount, gain_t* g, action_t* action)
    {
      if (specialization() == PRIEST_SHADOW)
      {
        double amount = num_amount;
        double amount_from_power_infusion = 0.0;
        double amount_from_surrender_to_madness = 0.0;

        if (buffs.surrender_to_madness_death->check())
        {
          double total_amount = 0.0;
        }
        else
        {

          if (buffs.surrender_to_madness->check() && buffs.power_infusion->check())
          {
            double total_amount = amount * (1.0 + buffs.power_infusion->data().effectN(2).percent()) *
              (1.0 + talents.surrender_to_madness->effectN(1).percent());

            amount_from_surrender_to_madness = amount * talents.surrender_to_madness->effectN(1).percent();

            // Since this effect is multiplicitive, we'll give the extra to Power Infusion since it does not last as long as
            // Surrender to Madness
            amount_from_power_infusion = total_amount - amount - amount_from_surrender_to_madness;

            // Make sure the maths line up.
            assert(total_amount == amount + amount_from_power_infusion + amount_from_surrender_to_madness);
          }
          else if (buffs.surrender_to_madness->check())
          {
            amount_from_surrender_to_madness =
              (amount * (1.0 + talents.surrender_to_madness->effectN(1).percent())) - amount;
          }
          else if (buffs.power_infusion->check())
          {
            amount_from_power_infusion =
              (amount * (1.0 + buffs.power_infusion->data().effectN(2).percent())) - amount;
          }

          insanity.gain(amount, g, action);

          if (amount_from_power_infusion > 0.0)
          {
            insanity.gain(amount_from_power_infusion, gains.insanity_power_infusion, action);
          }

          if (amount_from_surrender_to_madness > 0.0)
          {
            insanity.gain(amount_from_surrender_to_madness, gains.insanity_surrender_to_madness, action);
          }
        }
      }
    }

    /// Simple insanity expiration event that kicks the actor out of Voidform
    struct insanity_end_event_t : public event_t
    {
      priest_t& actor;

      insanity_end_event_t(priest_t& actor_, const timespan_t& duration_)
        : event_t(*actor_.sim, duration_), actor(actor_)
      {
      }

      void execute() override
      {
        if (actor.sim->debug)
        {
          actor.sim->out_debug.printf("%s insanity-track insanity-loss", actor.name());
        }

        actor.buffs.voidform->expire();
        actor.insanity.end = nullptr;
      }
    };

    /**
    * Insanity tracking
    *
    * Handles the resource gaining from abilities, and insanity draining and manages an event that forcibly punts the
    * actor out of Voidform the exact moment insanity hitszero (millisecond resolution).
    */
    struct insanity_state_t final
    {
      event_t* end;             // End event for dropping out of voidform (insanity reaches 0)
      timespan_t last_drained;  // Timestamp when insanity was last drained
      priest_t& actor;

      const double base_drain_per_sec;
      const double stack_drain_multiplier;
      double base_drain_multiplier;

      insanity_state_t(priest_t& a)
        : end(nullptr),
        last_drained(timespan_t::zero()),
        actor(a),
        base_drain_per_sec(a.find_spell(194249)->effectN(3).base_value() / -500.0),
        stack_drain_multiplier(2 / 3.0),  // Hardcoded Patch 7.1.5 (2016-12-02)
        base_drain_multiplier(1.0)
      {
      }

      /// Deferred init for actor dependent stuff not ready in the ctor
      void init()
      {
        if (actor.sets->has_set_bonus(PRIEST_SHADOW, T20, B4))
        {
          if (actor.talents.surrender_to_madness->ok())
          {
            base_drain_multiplier -= actor.sets->set(PRIEST_SHADOW, T20, B4)->effectN(2).percent();
          }
          else
          {
            base_drain_multiplier -= actor.sets->set(PRIEST_SHADOW, T20, B4)->effectN(1).percent();
          }
        }
      }

      /// Start the insanity drain tracking
      void set_last_drained()
      {
        last_drained = actor.sim->current_time();
      }

      /// Start (or re-start) tracking of the insanity drain plus end event
      void begin_tracking()
      {
        set_last_drained();
        adjust_end_event();
      }

      timespan_t time_to_end() const
      {
        return end ? end->remains() : timespan_t::zero();
      }

      void reset()
      {
        end = nullptr;
        last_drained = timespan_t::zero();
      }

      /// Compute insanity drain per second with current state of the actor
      double insanity_drain_per_second() const
      {
        if (actor.buffs.voidform->check() == 0)
        {
          return 0;
        }

        // Insanity does not drain during Dispersion
        if (actor.buffs.dispersion->check())
        {
          return 0;
        }

        // Insanity does not drain during Void Torrent
        if (actor.buffs.void_torrent->check())
        {
          return 0;
        }

        return base_drain_multiplier *
          (base_drain_per_sec + (actor.buffs.insanity_drain_stacks->current_value - 1) * stack_drain_multiplier);
      }

      /// Gain some insanity
      void gain(double value, gain_t* gain_obj, action_t* source_action = nullptr)
      {
        // Drain before gaining, but don't adjust end-event yet
        drain();

        if (actor.sim->debug)
        {
          auto current = actor.resources.current[RESOURCE_INSANITY];
          auto max = actor.resources.max[RESOURCE_INSANITY];

          actor.sim->out_debug.printf(
            "%s insanity-track gain, value=%f, current=%.1f/%.1f, "
            "new=%.1f/%.1f",
            actor.name(), value, current, max, clamp(current + value, 0.0, max), max);
        }

        actor.resource_gain(RESOURCE_INSANITY, value, gain_obj, source_action);

        // Explicitly adjust end-event after gaining some insanity
        adjust_end_event();
      }

      /**
      * Triggers the insanity drain, and is called in places that changes the insanity state of the actor in a relevant
      * way.
      * These are:
      * - Right before the actor decides to do something (scans APL for an ability to use)
      * - Right before insanity drain stack increases (every second)
      */
      void drain()
      {
        double drain_per_second = insanity_drain_per_second();
        double drain_interval = (actor.sim->current_time() - last_drained).total_seconds();

        // Don't drain if draining is disabled, or if we have already drained on this timestamp
        if (drain_per_second == 0 || drain_interval == 0)
        {
          return;
        }

        double drained = drain_per_second * drain_interval;
        // Ensure we always have enough to drain. This should always be true, since the drain is
        // always kept track of in relation to time.
#ifndef NDEBUG
        if (actor.resources.current[RESOURCE_INSANITY] < drained)
        {
          actor.sim->errorf("%s warning, insanity-track overdrain, current=%f drained=%f total=%f", actor.name(),
            actor.resources.current[RESOURCE_INSANITY], drained,
            actor.resources.current[RESOURCE_INSANITY] - drained);
          drained = actor.resources.current[RESOURCE_INSANITY];
        }
#else
        assert(actor.resources.current[RESOURCE_INSANITY] >= drained);
#endif

        if (actor.sim->debug)
        {
          auto current = actor.resources.current[RESOURCE_INSANITY];
          auto max = actor.resources.max[RESOURCE_INSANITY];

          actor.sim->out_debug.printf(
            "%s insanity-track drain, "
            "drain_per_second=%f, last_drained=%.3f, drain_interval=%.3f, "
            "current=%.1f/%.1f, new=%.1f/%.1f",
            actor.name(), drain_per_second, last_drained.total_seconds(), drain_interval, current, max,
            (current - drained), max);
        }

        // Update last drained, we're about to reduce the amount of insanity the actor has
        last_drained = actor.sim->current_time();

        actor.resource_loss(RESOURCE_INSANITY, drained, actor.gains.insanity_drain);
      }

      /**
      * Predict (with current state) when insanity is going to be fully depleted, and adjust (or create) an event for it.
      * Called in conjunction with insanity_state_t::drain(), after the insanity drain occurs (and potentially after a
      * relevant state change such as insanity drain stack buff increase occurs). */
      void adjust_end_event()
      {
        double drain_per_second = insanity_drain_per_second();

        // Ensure that the current insanity level is correct
        if (last_drained != actor.sim->current_time())
        {
          drain();
        }

        // All drained, cancel voidform. TODO: Can this really even happen?
        if (actor.resources.current[RESOURCE_INSANITY] == 0 && actor.options.priest_set_voidform_duration == 0)
        {
          event_t::cancel(end);
          actor.buffs.voidform->expire();
          return;
        }
        else if (actor.options.priest_set_voidform_duration > 0 && actor.options.priest_set_voidform_duration < actor.buffs.voidform->stack())
        {
          event_t::cancel(end);
          actor.buffs.voidform->expire();
          actor.resources.current[RESOURCE_INSANITY] = 0;
          return;
        }

        timespan_t seconds_left =
          drain_per_second ? timespan_t::from_seconds(actor.resources.current[RESOURCE_INSANITY] / drain_per_second)
          : timespan_t::zero();

        if (actor.sim->debug && drain_per_second > 0 && (!end || (end->remains() != seconds_left)))
        {
          auto current = actor.resources.current[RESOURCE_INSANITY];
          auto max = actor.resources.max[RESOURCE_INSANITY];

          actor.sim->out_debug.printf(
            "%s insanity-track adjust-end-event, "
            "drain_per_second=%f, insanity=%.1f/%.1f, seconds_left=%.3f, "
            "old_left=%.3f",
            actor.name(), drain_per_second, current, max, seconds_left.total_seconds(),
            end ? end->remains().total_seconds() : -1.0);
        }

        // If we have no draining occurring, cancel the event.
        if (drain_per_second == 0)
        {
          event_t::cancel(end);
        }
        // We have no drain event yet, so make a new event that triggers the cancellation of Voidform.
        else if (end == nullptr)
        {
          end = make_event<insanity_end_event_t>(*actor.sim, actor, seconds_left);
        }
        // Adjust existing event
        else
        {
          // New expiry time is sooner than the current insanity depletion event, create a new event with the new expiry
          // time.
          if (seconds_left < end->remains())
          {
            event_t::cancel(end);
            end = make_event<insanity_end_event_t>(*actor.sim, actor, seconds_left);
          }
          // End event is in the future, so just reschedule the current end event without creating a new one needlessly.
          else if (seconds_left > end->remains())
          {
            end->reschedule(seconds_left);
          }
        }
      }
    } insanity;

    std::string default_potion() const override;
    std::string default_flask() const override;
    std::string default_food() const override;
    std::string default_rune() const override;
  };

  
  namespace pets
  {
    /**
    * Pet base class
    *
    * Defines characteristics common to ALL priest pets.
    */
    struct priest_pet_t : public pet_t
    {
      priest_pet_t(sim_t* sim, priest_t& owner, const std::string& pet_name, pet_e pt, bool guardian = false)
        : pet_t(sim, &owner, pet_name, pt, guardian)
      {
      }

      void init_base_stats() override
      {
        pet_t::init_base_stats();

        base.position = POSITION_BACK;
        base.distance = 3;

        owner_coeff.ap_from_sp = 1.0;
        owner_coeff.sp_from_sp = 1.0;
      }

      void schedule_ready(timespan_t delta_time, bool waiting) override
      {
        if (main_hand_attack && !main_hand_attack->execute_event)
        {
          main_hand_attack->schedule_execute();
        }

        pet_t::schedule_ready(delta_time, waiting);
      }

      double composite_player_multiplier(school_e school) const override
      {
        double m = pet_t::composite_player_multiplier(school);

        // Orc racial
        m *= 1.0 + o().racials.command->effectN(1).percent();

        return m;
      }

      resource_e primary_resource() const override
      {
        return RESOURCE_ENERGY;
      }

      priest_t& o()
      {
        return static_cast<priest_t&>(*owner);
      }
      const priest_t& o() const
      {
        return static_cast<priest_t&>(*owner);
      }
    };

    struct priest_pet_melee_t : public melee_attack_t
    {
      bool first_swing;

      priest_pet_melee_t(priest_pet_t& p, const char* name)
        : melee_attack_t(name, &p, spell_data_t::nil()), first_swing(true)
      {
        school = SCHOOL_SHADOW;
        weapon = &(p.main_hand_weapon);
        base_execute_time = weapon->swing_time;
        may_crit = true;
        background = true;
        repeating = true;
      }

      void reset() override
      {
        melee_attack_t::reset();
        first_swing = true;
      }

      timespan_t execute_time() const override
      {
        // First swing comes instantly after summoning the pet
        if (first_swing)
          return timespan_t::zero();

        return melee_attack_t::execute_time();
      }

      void schedule_execute(action_state_t* state = nullptr) override
      {
        melee_attack_t::schedule_execute(state);

        first_swing = false;
      }
    };

    struct priest_pet_spell_t : public spell_t
    {
      priest_pet_spell_t(priest_pet_t& p, const std::string& n) : spell_t(n, &p, p.find_pet_spell(n))
      {
        may_crit = true;
      }

      priest_pet_spell_t(const std::string& token, priest_pet_t* p, const spell_data_t* s = spell_data_t::nil())
        : spell_t(token, p, s)
      {
        may_crit = true;
      }

      priest_pet_t& p()
      {
        return static_cast<priest_pet_t&>(*player);
      }
      const priest_pet_t& p() const
      {
        return static_cast<priest_pet_t&>(*player);
      }
    };

    namespace fiend
    {
      /**
      * Abstract base class for Shadowfiend and Mindbender
      */
      struct base_fiend_pet_t : public priest_pet_t
      {
        struct buffs_t
        {
          propagate_const<buff_t*> shadowcrawl;
        } buffs;

        struct gains_t
        {
          propagate_const<gain_t*> fiend;
        } gains;

        propagate_const<action_t*> shadowcrawl_action;

        double direct_power_mod;

        base_fiend_pet_t(sim_t* sim, priest_t& owner, pet_e pt, const std::string& name)
          : priest_pet_t(sim, owner, name, pt), buffs(), gains(), shadowcrawl_action(nullptr), direct_power_mod(0.0)
        {
          main_hand_weapon.type = WEAPON_BEAST;
          main_hand_weapon.swing_time = timespan_t::from_seconds(1.5);

          owner_coeff.health = 0.3;
        }

        virtual double mana_return_percent() const = 0;
        virtual double insanity_gain() const = 0;

        void init_action_list() override;

        void create_buffs() override
        {
          priest_pet_t::create_buffs();

          buffs.shadowcrawl = buff_creator_t(this, "shadowcrawl", find_pet_spell("Shadowcrawl"));
        }

        void init_gains() override
        {
          priest_pet_t::init_gains();

          if (o().specialization() == PRIEST_SHADOW)
          {
            gains.fiend = o().gains.insanity_pet;
          }
          else
          {
            switch (pet_type)
            {
            case PET_MINDBENDER:
            {
              gains.fiend = o().gains.mindbender;
            }
            break;
            default:
              gains.fiend = get_gain("basefiend");
              break;
            }
          }
        }

        void init_resources(bool force) override
        {
          priest_pet_t::init_resources(force);

          resources.initial[RESOURCE_MANA] = owner->resources.max[RESOURCE_MANA];
          resources.current = resources.max = resources.initial;
        }

        void summon(timespan_t duration) override
        {
          priest_pet_t::summon(duration);

          if (shadowcrawl_action)
          {
            // Ensure that it gets used after the first melee strike. In the combat logs that happen at the same time, but the
            // melee comes first.
            shadowcrawl_action->cooldown->ready = sim->current_time() + timespan_t::from_seconds(0.001);
          }
        }

        action_t* create_action(const std::string& name, const std::string& options_str) override;
      };

      struct shadowfiend_pet_t final : public base_fiend_pet_t
      {
        shadowfiend_pet_t(sim_t* sim, priest_t& owner, const std::string& name = "shadowfiend")
          : base_fiend_pet_t(sim, owner, PET_SHADOWFIEND, name)
        {
          direct_power_mod = 0.6;  // According to Sephuz 2018-02-07 -- N1gh7h4wk hardcoded

          main_hand_weapon.min_dmg = owner.dbc.spell_scaling(owner.type, owner.level()) * 2;
          main_hand_weapon.max_dmg = owner.dbc.spell_scaling(owner.type, owner.level()) * 2;

          main_hand_weapon.damage = (main_hand_weapon.min_dmg + main_hand_weapon.max_dmg) / 2;
        }

        double mana_return_percent() const override
        {
          return 0.0;
        }
        double insanity_gain() const override
        {
          return o().find_spell(262485)->effectN(1).resource(RESOURCE_INSANITY);
        }
      };

      struct mindbender_pet_t final : public base_fiend_pet_t
      {
        const spell_data_t* mindbender_spell;

        mindbender_pet_t(sim_t* sim, priest_t& owner, const std::string& name = "mindbender")
          : base_fiend_pet_t(sim, owner, PET_MINDBENDER, name), mindbender_spell(owner.find_spell(123051))
        {
          direct_power_mod = 0.65;  // According to Sephuz 2018-02-07 -- N1gh7h4wk hardcoded

          main_hand_weapon.min_dmg = owner.dbc.spell_scaling(owner.type, owner.level()) * 2;
          main_hand_weapon.max_dmg = owner.dbc.spell_scaling(owner.type, owner.level()) * 2;
          main_hand_weapon.damage = (main_hand_weapon.min_dmg + main_hand_weapon.max_dmg) / 2;
        }

        double mana_return_percent() const override
        {
          double m = mindbender_spell->effectN(1).percent();
          return m / 100;
        }
        double insanity_gain() const override
        {
          return o().find_spell(200010)->effectN(1).resource(RESOURCE_INSANITY);
        }
      };

      namespace actions
      {
        struct shadowcrawl_t final : public priest_pet_spell_t
        {
          shadowcrawl_t(base_fiend_pet_t& p) : priest_pet_spell_t(p, "Shadowcrawl")
          {
            may_miss = false;
            harmful = false;
          }

          base_fiend_pet_t& p()
          {
            return static_cast<base_fiend_pet_t&>(*player);
          }
          const base_fiend_pet_t& p() const
          {
            return static_cast<base_fiend_pet_t&>(*player);
          }

          void execute() override
          {
            priest_pet_spell_t::execute();

            p().buffs.shadowcrawl->trigger();
          }
        };

        struct fiend_melee_t : public priest_pet_melee_t
        {
          fiend_melee_t(base_fiend_pet_t& p) : priest_pet_melee_t(p, "melee")
          {
            weapon = &(p.main_hand_weapon);
            weapon_multiplier = 0.0;
            base_dd_min = weapon->min_dmg;
            base_dd_max = weapon->max_dmg;
            attack_power_mod.direct = p.direct_power_mod;
          }

          base_fiend_pet_t& p()
          {
            return static_cast<base_fiend_pet_t&>(*player);
          }
          const base_fiend_pet_t& p() const
          {
            return static_cast<base_fiend_pet_t&>(*player);
          }

          double action_multiplier() const override
          {
            double am = priest_pet_melee_t::action_multiplier();

            am *= 1.0 + p().buffs.shadowcrawl->check() * p().buffs.shadowcrawl->data().effectN(2).percent();

            return am;
          }

          timespan_t execute_time() const override
          {
            if (base_execute_time == timespan_t::zero())
              return timespan_t::zero();

            if (!harmful && !player->in_combat)
              return timespan_t::zero();

            return base_execute_time * player->cache.spell_speed();
          }

          void execute() override
          {
            priest_pet_melee_t::execute();

            p().buffs.shadowcrawl->up();  // uptime tracking
          }

          void impact(action_state_t* s) override
          {
            priest_pet_melee_t::impact(s);

            if (result_is_hit(s->result))
            {
              if (p().o().specialization() == PRIEST_SHADOW)
              {
                double amount = p().insanity_gain();
                if (p().o().buffs.surrender_to_madness_death->up())
                {
                  amount = 0.0; // generation with debuff is zero N1gh7h4wk 2018/01/26
                }
                if (p().o().buffs.surrender_to_madness->up())
                {
                  p().o().resource_gain(
                    RESOURCE_INSANITY,
                    (amount * (1.0 + p().o().talents.surrender_to_madness->effectN(1).percent())) - amount,
                    p().o().gains.insanity_surrender_to_madness);
                }
                p().o().insanity.gain(amount, p().gains.fiend);
              }
              else
              {
                double mana_reg_pct = p().mana_return_percent();
                if (mana_reg_pct > 0.0)
                {
                  p().o().resource_gain(RESOURCE_MANA, p().o().resources.max[RESOURCE_MANA] * p().mana_return_percent(),
                    p().gains.fiend);
                }
              }
            }
          }
        };
      }  // namespace actions

      void base_fiend_pet_t::init_action_list()
      {
        main_hand_attack = new actions::fiend_melee_t(*this);

        if (action_list_str.empty())
        {
          action_priority_list_t* precombat = get_action_priority_list("precombat");
          precombat->add_action("snapshot_stats",
            "Snapshot raid buffed stats before combat begins and "
            "pre-potting is done.");

          action_priority_list_t* def = get_action_priority_list("default");
          def->add_action("shadowcrawl");
          def->add_action("wait_for_shadowcrawl");
        }

        priest_pet_t::init_action_list();
      }

      action_t* base_fiend_pet_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "shadowcrawl")
        {
          shadowcrawl_action = new actions::shadowcrawl_t(*this);
          return shadowcrawl_action;
        }

        if (name == "wait_for_shadowcrawl")
          return new wait_for_cooldown_t(this, "shadowcrawl");

        return priest_pet_t::create_action(name, options_str);
      }
    }  // namespace fiend
  }  // namespace pets

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
    struct priest_action_t : public Base
    {
      bool shadow_damage_increase;
      bool shadow_dot_increase;

    public:
      priest_action_t(const std::string& n, priest_t& p, const spell_data_t* s);

      priest_td_t& get_td(player_t* t);
      priest_td_t* find_td(player_t* t);
      const priest_td_t* find_td(player_t* t) const;
      bool trigger_shadowy_insight();
      bool trigger_zeks();
      void trigger_anunds();
      double cost() const override;
      void consume_resource() override;

    protected:
      priest_t& priest()
      { return *debug_cast<priest_t*>( ab::player ); }
      const priest_t& priest() const
      { return *debug_cast<priest_t*>( ab::player ); }

      /// typedef for priest_action_t<action_base_t>
      using base_t = priest_action_t;

    private:
      /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
      using ab = Base;
    };

    struct priest_absorb_t : public priest_action_t<absorb_t>
    {
    public:
      priest_absorb_t(const std::string& n, priest_t& player, const spell_data_t* s);
    };

    struct priest_heal_t : public priest_action_t<heal_t>
    {
      priest_heal_t(const std::string& n, priest_t& player, const spell_data_t* s = spell_data_t::nil())
        : base_t(n, player, s);

      double action_multiplier() const override;
      void execute() override;
      void impact(action_state_t* s) override
    };

    struct priest_spell_t : public priest_action_t<spell_t>
    {
      bool is_sphere_of_insanity_spell;
      bool is_mastery_spell;

      priest_spell_t(const std::string& n, priest_t& player, const spell_data_t* s = spell_data_t::nil())
        : base_t(n, player, s);

      bool usable_moving() const override;
      void impact(action_state_t* s) override;
      double action_multiplier() const override;
      void assess_damage(dmg_e type, action_state_t* s) override;
      void trigger_vampiric_embrace(action_state_t* s);
    };

    namespace spells
    {
      struct angelic_feather_t final : public priest_spell_t
      {
        angelic_feather_t(priest_t& p, const std::string& options_str);
        void impact(action_state_t* s) override;
        bool ready() override;
      };

      /// Divine Star Base Spell, used for both heal and damage spell.
      template <class Base>
      struct divine_star_base_t : public Base
      {
      private:
        typedef Base ab;  // the action base ("ab") type (priest_spell_t or priest_heal_t)
      public:
        typedef divine_star_base_t base_t;

        propagate_const<divine_star_base_t*> return_spell;

        divine_star_base_t(const std::string& n, priest_t& p, const spell_data_t* spell_data, bool is_return_spell = false)
          : ab(n, p, spell_data),
          return_spell((is_return_spell ? nullptr : new divine_star_base_t(n, p, spell_data, true)))
        {
          ab::aoe = -1;

          ab::proc = ab::background = true;
        }

        // Divine Star will damage and heal targets twice, once on the way out and again on the way back. This is determined
        // by distance from the target. If we are too far away, it misses completely. If we are at the very edge distance
        // wise, it will only hit once. If we are within range (and aren't moving such that it would miss the target on the
        // way out and/or back), it will hit twice. Threshold is 24 yards, per tooltip and tests for 2 hits. 28 yards is the
        // threshold for 1 hit.
        void execute() override
        {
          double distance;

          distance = ab::player->get_player_distance(*ab::target);

          if (distance <= 28)
          {
            ab::execute();

            if (return_spell && distance <= 24)
              return_spell->execute();
          }
        }
      };

      struct divine_star_t final : public priest_spell_t
      {
        divine_star_t(priest_t& p, const std::string& options_str);
        void execute() override;

      private:
        action_t* _heal_spell;
        action_t* _dmg_spell;
      };

      /// Halo Base Spell, used for both damage and heal spell.
      template <class Base>
      struct halo_base_t : public Base
      {
      public:
        halo_base_t(const std::string& n, priest_t& p, const spell_data_t* s) : Base(n, p, s)
        {
          Base::aoe = -1;
          Base::background = true;

          if (Base::data().ok())
          {
            // Reparse the correct effect number, because we have two competing ones ( were 2 > 1 always wins out )
            Base::parse_effect_data(Base::data().effectN(1));
          }
          Base::radius = 30;
          Base::range = 0;
        }

        timespan_t distance_targeting_travel_time(action_state_t* s) const override
        {
          return timespan_t::from_seconds(s->action->player->get_player_distance(*s->target) / Base::travel_speed);
        }

        double calculate_direct_amount(action_state_t* s) const override
        {
          double cda = Base::calculate_direct_amount(s);

          // Source: Ghostcrawler 2012-06-20 http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97

          double distance;
          distance = s->action->player->get_player_distance(*s->target);

          // double mult = 0.5 * pow( 1.01, -1 * pow( ( distance - 25 ) / 2, 4 ) ) + 0.1 + 0.015 * distance;
          double mult = 0.5 * exp(-0.00995 * pow(distance / 2 - 12.5, 4)) + 0.1 + 0.015 * distance;

          return cda * mult;
        }
      };

      struct halo_t final : public priest_spell_t
      {
        halo_t(priest_t& p, const std::string& options_str);
        void execute() override;

      private:
        propagate_const<action_t*> _heal_spell;
        propagate_const<action_t*> _dmg_spell;
      };

      struct levitate_t final : public priest_spell_t
      {
        levitate_t(priest_t& p, const std::string& options_str);
      };

      struct power_infusion_t final : public priest_spell_t
      {
        power_infusion_t(priest_t& p, const std::string& options_str);
        void execute() override;
      };

      struct shadow_word_pain_t final : public priest_spell_t
      {
        double insanity_gain;
        bool casted;

        shadow_word_pain_t(priest_t& p, const std::string& options_str, bool _casted = true);

        double spell_direct_power_coefficient(const action_state_t* s) const override;
        void impact(action_state_t* s) override;
        void last_tick(dot_t* d) override;
        void tick(dot_t* d) override;
        double cost() const override;
        double action_multiplier() const override;
      };

      struct smite_t final : public priest_spell_t
      {
        smite_t(priest_t& p, const std::string& options_str);

        void execute() override;
      };

      /// Priest Pet Summon Base Spell
      struct summon_pet_t : public priest_spell_t
      {
        timespan_t summoning_duration;
        std::string pet_name;
        propagate_const<pet_t*> pet;

      public:
        summon_pet_t(const std::string& n, priest_t& p, const spell_data_t* sd = spell_data_t::nil());

        bool init_finished() override;
        void execute() override;
        bool ready() override;
      };

      struct summon_shadowfiend_t final : public summon_pet_t
      {
        summon_shadowfiend_t(priest_t& p, const std::string& options_str);
      };

      struct summon_mindbender_t final : public summon_pet_t
      {
        summon_mindbender_t(priest_t& p, const std::string& options_str);
      };

    } // Spells Namespace

    namespace heals
    {
      struct power_word_shield_t final : public priest_absorb_t
      {
        bool ignore_debuff;

        power_word_shield_t(priest_t& p, const std::string& options_str);

        void impact(action_state_t* s) override;
      };
    } // Heals Namespace

  } // Actions Namespace

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
    template <typename Base>
    struct priest_buff_t : public Base
    {
    public:
      using base_t = priest_buff_t;  // typedef for priest_buff_t<buff_base_t>

      priest_buff_t(actor_pair_t q, const std::string& name, const spell_data_t* s = spell_data_t::nil() ) :
        Base(q, name, s)
      {
      }

    protected:
      priest_t& priest()
      { return *debug_cast<priest_t*>( Base::source ); }
      const priest_t& priest() const
      { return *debug_cast<priest_t*>( Base::source ); }
    };

    /** Custom insanity_drain_stacks buff */
    struct insanity_drain_stacks_t final : public priest_buff_t<buff_t>
    {
      struct stack_increase_event_t final : public player_event_t
      {
        propagate_const<insanity_drain_stacks_t*> ids;

        stack_increase_event_t(insanity_drain_stacks_t* s);

        const char* name() const override;
        void execute() override;
      };

      propagate_const<stack_increase_event_t*> stack_increase;

      insanity_drain_stacks_t(priest_t& p);
      bool trigger(int stacks, double value, double chance, timespan_t duration) override;
      void expire_override(int expiration_stacks, timespan_t remaining_duration) override;
      void bump(int stacks, double /* value */) override;
      void reset() override;
    };

  } // Buffs Namespace

  namespace items
  {

    void do_trinket_init(const priest_t* priest, specialization_e spec, const special_effect_t*& ptr, const special_effect_t& effect);

        // Legion Legendaries

    // Shadow
    void anunds_seared_shackles(special_effect_t& effect);
    void mangazas_madness(special_effect_t& effect);
    void mother_shahrazs_seduction(special_effect_t& effect);
    void the_twins_painful_touch(special_effect_t& effect);
    void zenkaram_iridis_anadem(special_effect_t& effect);
    void zeks_exterminatus(special_effect_t& effect);
    void heart_of_the_void(special_effect_t& effect);

    using namespace unique_gear;

    struct sephuzs_secret_enabler_t : public scoped_actor_callback_t<priest_t>
    {
      sephuzs_secret_enabler_t();

      void manipulate(priest_t* priest, const special_effect_t& e) override;
    };

    struct sephuzs_secret_t : public class_buff_cb_t<priest_t, haste_buff_t, haste_buff_creator_t>
    {
      sephuzs_secret_t();

      haste_buff_t*& buff_ptr(const special_effect_t& e) override;
      haste_buff_creator_t creator(const special_effect_t& e) const override;
    };

    void init();

  } // Items Namespace

  /**
  * Report Extension Class
  * Here you can define class specific report extensions/overrides
  */
  struct priest_report_t final : public player_report_extension_t
  {
  public:
    priest_report_t(priest_t& player);
    void html_customsection(report::sc_html_stream& /* os*/);

  private:
    priest_t& p;
  };

  struct priest_module_t final : public module_t
  {
    priest_module_t();

    player_t* create_player(sim_t* sim, const std::string& name, race_e r) const override;
    bool valid() const override;
    void init(player_t* p) const override;
    void static_init() const override;
    void register_hotfixes() const override;
    void combat_begin(sim_t*) const override;
    void combat_end(sim_t*) const override;
  };

}  // PRIEST NAMESPACE

const module_t* module_t::priest()
{
  static priest_module_t m;
  return &m;
}