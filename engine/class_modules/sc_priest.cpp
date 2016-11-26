// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
/*
TODO - Updated 2016-09-28 by scamille:

Disc:
  Heal
Holy:
  Everything

Shadow
  - Finish ("required") Artifacts Traits:
      - Thoughts of Insanity
          - Need to implement Shadowmend first.
      - Thrive in the Shadows
  - Shadowmend
  - Setbonuses
    - Several Setbonuses have to be updated by Blizzard: T17, T16 no longer
work.
  - Shadow Priests are being killed by casting surrender to madness way too
early when the simulation doesn't use fixed_time
    and the expression uses 'target.time_to_die' instead of target.health.pct.
    I band-aid fixed it by turning on fixed time when there are only shadow
priests in a simulation, and also changed the expression to use health pct.
    This might be the best fix for it, but it's probably worth looking into
other avenues.
    - Collision 7/25/2016

*/
namespace  // UNNAMED NAMESPACE
{
/* Forward declarations
 */
struct priest_t;
namespace actions
{
namespace spells
{
struct mind_spike_detonation_t;
struct shadowy_apparition_spell_t;
struct sphere_of_insanity_spell_t;
}
}

namespace pets
{
namespace void_tendril
{
struct void_tendril_pet_t;
}
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
    dot_t* shadow_word_pain;
    dot_t* vampiric_touch;
    dot_t* holy_fire;
    dot_t* power_word_solace;
  } dots;

  struct buffs_t
  {
    buff_t* mental_fatigue;
    buff_t* mind_spike;
    buff_t* schism;
  } buffs;

  priest_t& priest;

  priest_td_t( player_t* target, priest_t& p );
  void reset();
  void target_demise();
};

/* Priest class definition
 *
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
    buff_t* power_infusion;
    buff_t* twist_of_fate;

    // Discipline
    buff_t* archangel;
    buff_t* borrowed_time;
    buff_t* holy_evangelism;
    buff_t* inner_focus;

    // Holy

    // Shadow
    buff_t* dispersion;
    buff_t* insanity_drain_stacks;
    buff_t* lingering_insanity;
    buff_t* mind_sear_on_hit_reset;
    buff_t* shadowform;
    buff_t* shadowform_state;  // Dummy buff to track whether player entered
                               // Shadowform initially
    buff_t* shadowy_insight;
    buff_t* sphere_of_insanity;
    buff_t* surrender_to_madness;
    buff_t* surrender_to_madness_death;
    buff_t* vampiric_embrace;
    buff_t* void_ray;
    buff_t* void_torrent;
    buff_t* voidform;

    // Set Bonus
    buff_t* mental_instinct;          // T17 Shadow 4pc
    buff_t* reperation;               // T18 Disc 2p
    buff_t* premonition;              // T18 Shadow 4pc
    stat_buff_t* power_overwhelming;  // T19OH
    buff_t* shadow_t19_4p;            // T19 Shadow 4pc

    // Legion Legendaries
    // Shadow
    buff_t* anunds_last_breath;       // Anund's Seared Shackles stack counter
    buff_t* the_twins_painful_touch;  // To track first casting
  } buffs;

  // Talents
  struct
  {
    const spell_data_t* shining_force;
    const spell_data_t* divine_star;
    const spell_data_t* halo;

    const spell_data_t* twist_of_fate;

    const spell_data_t* angelic_feather;
    const spell_data_t* body_and_soul;
    const spell_data_t* masochism;

    const spell_data_t* psychic_voice;
    const spell_data_t* dominant_mind;

    const spell_data_t* power_infusion;
    const spell_data_t* mindbender;

    // Discipline
    const spell_data_t* the_penitent;
    const spell_data_t* castigation;
    const spell_data_t* schism;

    const spell_data_t* power_word_solace;
    const spell_data_t* shield_discipline;

    const spell_data_t* contrition;

    const spell_data_t* clarity_of_will;

    const spell_data_t* purge_the_wicked;
    const spell_data_t* grace;
    const spell_data_t* shadow_covenant;

    // Holy

    const spell_data_t* trail_of_light;
    const spell_data_t* enduring_renewal;
    const spell_data_t* enlightenment;

    const spell_data_t* path_of_light;
    const spell_data_t* desperate_prayer;

    const spell_data_t* light_of_the_naaru;
    const spell_data_t* guardian_angel;
    const spell_data_t* hymn_of_hope;

    const spell_data_t* surge_of_light;
    const spell_data_t* binding_heal;
    const spell_data_t* piety;

    const spell_data_t* divinity;

    const spell_data_t* apotheosis;
    const spell_data_t* benediction;
    const spell_data_t* circle_of_healing;

    // Shadow

    const spell_data_t* fortress_of_the_mind;
    const spell_data_t* shadow_word_void;

    const spell_data_t* mania;

    const spell_data_t* mind_bomb;

    const spell_data_t* void_lord;
    const spell_data_t* reaper_of_souls;
    const spell_data_t* void_ray;

    const spell_data_t* sanlayn;
    const spell_data_t* auspicious_spirits;
    const spell_data_t* shadowy_insight;

    const spell_data_t* shadow_crash;

    const spell_data_t* legacy_of_the_void;
    const spell_data_t* mind_spike;
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
  } artifact;

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
    cooldown_t* chakra;
    cooldown_t* mindbender;
    cooldown_t* penance;
    cooldown_t* power_word_shield;
    cooldown_t* shadowfiend;
    cooldown_t* silence;

    cooldown_t* mind_blast;
    cooldown_t* shadow_word_death;
    cooldown_t* shadow_word_void;
    cooldown_t* void_bolt;
  } cooldowns;

  // Gains
  struct
  {
    gain_t* mindbender;
    gain_t* power_word_solace;
    gain_t* insanity_auspicious_spirits;
    gain_t* insanity_dispersion;
    gain_t* insanity_void_torrent;
    gain_t* insanity_drain;
    gain_t* insanity_mind_blast;
    gain_t* insanity_mind_flay;
    gain_t* insanity_mind_sear;
    gain_t* insanity_mind_spike;
    gain_t* insanity_mindbender;
    gain_t* insanity_power_infusion;
    gain_t* insanity_shadow_crash;
    gain_t* insanity_shadow_word_death;
    gain_t* insanity_shadow_word_pain_ondamage;
    gain_t* insanity_shadow_word_pain_onhit;
    gain_t* insanity_shadow_word_void;
    gain_t* insanity_surrender_to_madness;
    gain_t* insanity_vampiric_touch_ondamage;
    gain_t* insanity_vampiric_touch_onhit;
    gain_t* insanity_void_bolt;
    gain_t* shadowy_insight;
    gain_t* vampiric_touch_health;
  } gains;

  // Benefits
  struct
  {
  } benefits;

  // Procs
  struct
  {
    proc_t* legendary_anunds_last_breath;
    proc_t* legendary_anunds_last_breath_overflow;
    proc_t* shadowy_insight;
    proc_t* shadowy_insight_overflow;
    proc_t* serendipity;
    proc_t* serendipity_overflow;
    proc_t* shadowy_apparition;
    proc_t* shadowy_apparition_overflow;
    proc_t* surge_of_light;
    proc_t* surge_of_light_overflow;
    proc_t* t17_2pc_caster_mind_blast_reset;
    proc_t* t17_2pc_caster_mind_blast_reset_overflow;
    proc_t* t17_2pc_caster_mind_blast_reset_overflow_seconds;
    proc_t* t17_4pc_holy;
    proc_t* t17_4pc_holy_overflow;
    proc_t* void_eruption_both_dots;
    proc_t* void_eruption_no_dots;
    proc_t* void_eruption_only_shadow_word_pain;
    proc_t* void_eruption_only_vampiric_touch;
    proc_t* void_tendril;
  } procs;

  struct realppm_t
  {
    real_ppm_t* call_to_the_void;
  } rppm;

  // Special
  struct
  {
    actions::spells::mind_spike_detonation_t* mind_spike_detonation;
    actions::spells::shadowy_apparition_spell_t* shadowy_apparitions;
    actions::spells::sphere_of_insanity_spell_t* sphere_of_insanity;
    action_t* mental_fortitude;
  } active_spells;

  struct
  {
    // Archimonde Trinkets
    const special_effect_t* naarus_discipline;
    const special_effect_t* complete_healing;
    const special_effect_t* mental_fatigue;

    // Legion Legendaries
    // Shadow
    const special_effect_t* anunds_seared_shackles;     // wrist
    const special_effect_t* mother_shahrazs_seduction;  // shoulder
    const special_effect_t* mangazas_madness;           // belt
    const special_effect_t* zenkaram_iridis_anadem;     // helm
    const special_effect_t* the_twins_painful_touch;    // ring
  } active_items;

  // Pets
  struct
  {
    pet_t* shadowfiend;
    pet_t* mindbender;
    std::array<pets::void_tendril::void_tendril_pet_t*, 10>
        void_tendril;  // Multiple can be
                       // up at one time.
                       // 10 should be
                       // more than
                       // enough.
  } pets;

  // Options
  struct
  {
    bool autoUnshift       = true;  // Shift automatically out of stance/form
    bool priest_fixed_time = true;
  } options;

  // Glyphs
  struct
  {
    // All Specs
    const spell_data_t* angels;
    const spell_data_t* confession;
    const spell_data_t* holy_resurrection;
    const spell_data_t* shackle_undead;
    const spell_data_t* the_heavens;
    const spell_data_t* the_sha;

    // Discipline
    const spell_data_t* borrowed_time;

    // Holy
    const spell_data_t* inspired_hymns;
    const spell_data_t* the_valkyr;

    // Shadow
    const spell_data_t* dark_archangel;
    const spell_data_t* shadow;
    const spell_data_t* shadow_ravens;
    const spell_data_t* shadowy_friends;
  } glyphs;

  priest_t( sim_t* sim, const std::string& name, race_e r );

  priest_td_t* find_target_data( player_t* target ) const;

  // player_t overrides
  void init_base_stats() override;
  void init_resources( bool force ) override;
  void init_spells() override;
  void create_buffs() override;
  void init_scaling() override;
  void reset() override;
  void create_options() override;
  std::string create_profile( save_e = SAVE_ALL ) override;
  action_t* create_action( const std::string& name,
                           const std::string& options ) override;
  pet_t* create_pet( const std::string& name,
                     const std::string& type = std::string() ) override;
  void create_pets() override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override
  {
    return RESOURCE_MANA;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void assess_damage( school_e school, dmg_e dtype,
                      action_state_t* s ) override;
  double composite_melee_haste() const override;
  double composite_melee_speed() const override;
  double composite_spell_haste() const override;
  double composite_spell_speed() const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_absorb_multiplier(
      const action_state_t* s ) const override;
  double composite_player_heal_multiplier(
      const action_state_t* s ) const override;
  double composite_player_target_multiplier( player_t* t,
                                             school_e school ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void pre_analyze_hook() override;
  void init_action_list() override;
  void combat_begin() override;
  void init_rng() override;
  priest_td_t* get_target_data( player_t* target ) const override;
  expr_t* create_expression( action_t* a,
                             const std::string& name_str ) override;
  bool has_t18_class_trinket() const override;

private:
  void create_cooldowns();
  void create_gains();
  void create_procs();
  void create_benefits();
  void apl_precombat();
  void apl_default();
  void apl_shadow();
  void apl_disc_heal();
  void apl_disc_dmg();
  void apl_holy_heal();
  void apl_holy_dmg();
  void fixup_atonement_stats( const std::string& trigger_spell_name,
                              const std::string& atonement_spell_name );

  target_specific_t<priest_td_t> _target_data;
};

namespace pets
{
// ==========================================================================
// Priest Pets
// ==========================================================================

/* priest pet base
 *
 * defines characteristics common to ALL priest pets
 */
struct priest_pet_t : public pet_t
{
  priest_pet_t( sim_t* sim, priest_t& owner, const std::string& pet_name,
                pet_e pt, bool guardian = false )
    : pet_t( sim, &owner, pet_name, pt, guardian )
  {
    base.position = POSITION_BACK;
    base.distance = 3;
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    owner_coeff.ap_from_sp = 1.0;
    owner_coeff.sp_from_sp = 1.0;
  }

  void schedule_ready( timespan_t delta_time, bool waiting ) override
  {
    if ( main_hand_attack && !main_hand_attack->execute_event )
    {
      main_hand_attack->schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Orc racial
    m *= 1.0 + o().racials.command->effectN( 1 ).percent();

    return m;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
  }

  priest_t& o() const
  {
    return static_cast<priest_t&>( *owner );
  }
};

// ==========================================================================
// Priest Pet Melee
// ==========================================================================

struct priest_pet_melee_t : public melee_attack_t
{
  bool first_swing;

  priest_pet_melee_t( priest_pet_t& p, const char* name )
    : melee_attack_t( name, &p, spell_data_t::nil() ), first_swing( true )
  {
    school            = SCHOOL_SHADOW;
    weapon            = &( p.main_hand_weapon );
    base_execute_time = weapon->swing_time;
    may_crit          = true;
    background        = true;
    repeating         = true;
  }

  void reset() override
  {
    melee_attack_t::reset();
    first_swing = true;
  }

  timespan_t execute_time() const override
  {
    // First swing comes instantly after summoning the pet
    if ( first_swing )
      return timespan_t::zero();

    return melee_attack_t::execute_time();
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    melee_attack_t::schedule_execute( state );

    first_swing = false;
  }
};

// ==========================================================================
// Priest Pet Spell
// ==========================================================================

struct priest_pet_spell_t : public spell_t
{
  priest_pet_spell_t( priest_pet_t& p, const std::string& n )
    : spell_t( n, &p, p.find_pet_spell( n ) )
  {
    may_crit = true;
  }

  priest_pet_spell_t( const std::string& token, priest_pet_t* p,
                      const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s )
  {
    may_crit = true;
  }

  priest_pet_t& p()
  {
    return static_cast<priest_pet_t&>( *player );
  }
  const priest_pet_t& p() const
  {
    return static_cast<priest_pet_t&>( *player );
  }
};

namespace fiend
{
/* Abstract base class for Shadowfiend and Mindbender
 *
 */
struct base_fiend_pet_t : public priest_pet_t
{
  struct buffs_t
  {
    buff_t* shadowcrawl;
  } buffs;

  struct gains_t
  {
    gain_t* fiend;
  } gains;

  action_t* shadowcrawl_action;

  double direct_power_mod;

  base_fiend_pet_t( sim_t* sim, priest_t& owner, pet_e pt,
                    const std::string& name )
    : priest_pet_t( sim, owner, name, pt ),
      buffs(),
      gains(),
      shadowcrawl_action( nullptr ),
      direct_power_mod( 0.0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );

    owner_coeff.health = 0.3;
  }

  virtual double mana_return_percent() const = 0;

  void init_action_list() override;

  void create_buffs() override
  {
    priest_pet_t::create_buffs();

    buffs.shadowcrawl =
        buff_creator_t( this, "shadowcrawl", find_pet_spell( "Shadowcrawl" ) );
  }

  void init_gains() override
  {
    priest_pet_t::init_gains();

    switch ( pet_type )
    {
      case PET_MINDBENDER:
        if ( o().specialization() == PRIEST_SHADOW )
        {
          gains.fiend = o().gains.insanity_mindbender;
        }
        else
        {
          gains.fiend = o().gains.mindbender;
        }
        break;
      default:
        gains.fiend = get_gain( "basefiend" );
        break;
    }
  }

  void init_resources( bool force ) override
  {
    priest_pet_t::init_resources( force );

    resources.initial[ RESOURCE_MANA ] = owner->resources.max[ RESOURCE_MANA ];
    resources.current = resources.max = resources.initial;
  }

  void summon( timespan_t duration ) override
  {
    priest_pet_t::summon( duration );

    if ( shadowcrawl_action )
    {
      /* Ensure that it gets used after the first melee strike.
       * In the combat logs that happen at the same time, but the melee comes
       * first.
       */
      shadowcrawl_action->cooldown->ready =
          sim->current_time() + timespan_t::from_seconds( 0.001 );
    }
  }

  void dismiss( bool expired ) override
  {
    // T18 Shadow 4pc; don't trigger if the we're at the end of the iteration
    // (owner demise is in
    // progress)
    if ( expired )
    {
      o().buffs.premonition->trigger();
    }

    priest_pet_t::dismiss( expired );
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override;
};

// ==========================================================================
// Pet Shadowfiend
// ==========================================================================

struct shadowfiend_pet_t final : public base_fiend_pet_t
{
  shadowfiend_pet_t( sim_t* sim, priest_t& owner,
                     const std::string& name = "shadowfiend" )
    : base_fiend_pet_t( sim, owner, PET_SHADOWFIEND, name )
  {
    direct_power_mod = 1.875;  // Verified 2016/06/02 -- Twintop

    main_hand_weapon.min_dmg =
        owner.dbc.spell_scaling( owner.type, owner.level() ) * 2;
    main_hand_weapon.max_dmg =
        owner.dbc.spell_scaling( owner.type, owner.level() ) * 2;

    main_hand_weapon.damage =
        ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  }

  double mana_return_percent() const override
  {
    return 0.0;
  }
};

// ==========================================================================
// Pet Mindbender
// ==========================================================================

struct mindbender_pet_t final : public base_fiend_pet_t
{
  const spell_data_t* mindbender_spell;

  mindbender_pet_t( sim_t* sim, priest_t& owner,
                    const std::string& name = "mindbender" )
    : base_fiend_pet_t( sim, owner, PET_MINDBENDER, name ),
      mindbender_spell( owner.find_talent_spell( "Mindbender" ) )
  {
    direct_power_mod = 1.5;  // Verified 2016/06/02 -- Twintop

    main_hand_weapon.min_dmg =
        owner.dbc.spell_scaling( owner.type, owner.level() ) * 2;
    main_hand_weapon.max_dmg =
        owner.dbc.spell_scaling( owner.type, owner.level() ) * 2;
    main_hand_weapon.damage =
        ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  }

  double mana_return_percent() const override
  {
    double m = mindbender_spell->effectN( 2 ).percent();
    return m / 100;
  }
};

namespace actions
{  // pets/fiend/actions

struct shadowcrawl_t final : public priest_pet_spell_t
{
  shadowcrawl_t( base_fiend_pet_t& p ) : priest_pet_spell_t( p, "Shadowcrawl" )
  {
    may_miss = false;
    harmful  = false;
  }

  base_fiend_pet_t& p()
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }
  const base_fiend_pet_t& p() const
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }

  void execute() override
  {
    priest_pet_spell_t::execute();

    p().buffs.shadowcrawl->trigger();
  }
};

struct fiend_melee_t : public priest_pet_melee_t
{
  fiend_melee_t( base_fiend_pet_t& p ) : priest_pet_melee_t( p, "melee" )
  {
    weapon                  = &( p.main_hand_weapon );
    weapon_multiplier       = 0.0;
    base_dd_min             = weapon->min_dmg;
    base_dd_max             = weapon->max_dmg;
    attack_power_mod.direct = p.direct_power_mod;
  }

  base_fiend_pet_t& p()
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }
  const base_fiend_pet_t& p() const
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }

  double action_multiplier() const override
  {
    double am = priest_pet_melee_t::action_multiplier();

    am *= 1.0 +
          p().buffs.shadowcrawl->check() *
              p().buffs.shadowcrawl->data().effectN( 2 ).percent();

    return am;
  }

  void execute() override
  {
    priest_pet_melee_t::execute();
    p().buffs.shadowcrawl->up();  // uptime tracking
  }

  void impact( action_state_t* s ) override
  {
    priest_pet_melee_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p().o().specialization() == PRIEST_SHADOW )
      {
        double amount = p().o().talents.mindbender->effectN( 3 ).base_value();
        p().o().resource_gain( RESOURCE_INSANITY, amount, p().gains.fiend );

        if ( p().o().buffs.surrender_to_madness->up() )
        {
          p().o().resource_gain(
              RESOURCE_INSANITY,
              ( amount * ( 1.0 +
                           p().o()
                               .talents.surrender_to_madness->effectN( 1 )
                               .percent() ) ) -
                  amount,
              p().o().gains.insanity_surrender_to_madness );
        }
        else
        {
        }
      }
      else
      {
        double mana_reg_pct = p().mana_return_percent();
        if ( mana_reg_pct > 0.0 )
        {
          p().o().resource_gain( RESOURCE_MANA,
                                 p().o().resources.max[ RESOURCE_MANA ] *
                                     p().mana_return_percent(),
                                 p().gains.fiend );
        }
      }
    }
  }
};

}  // pets/fiend/actions

void base_fiend_pet_t::init_action_list()
{
  main_hand_attack = new actions::fiend_melee_t( *this );

  if ( action_list_str.empty() )
  {
    action_priority_list_t* precombat = get_action_priority_list( "precombat" );
    // Snapshot stats
    precombat->add_action(
        "snapshot_stats",
        "Snapshot raid buffed stats before combat begins and "
        "pre-potting is done." );

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "shadowcrawl" );
    def->add_action( "wait_for_shadowcrawl" );
  }

  priest_pet_t::init_action_list();
}

action_t* base_fiend_pet_t::create_action( const std::string& name,
                                           const std::string& options_str )
{
  if ( name == "shadowcrawl" )
  {
    shadowcrawl_action = new actions::shadowcrawl_t( *this );
    return shadowcrawl_action;
  }

  if ( name == "wait_for_shadowcrawl" )
    return new wait_for_cooldown_t( this, "shadowcrawl" );

  return priest_pet_t::create_action( name, options_str );
}
}  // pets/fiend

namespace void_tendril
{
// ==========================================================================
// Pet Void Tendril
// ==========================================================================

struct void_tendril_pet_t final : public priest_pet_t
{
public:
  void_tendril_pet_t( sim_t* sim, priest_t& p, void_tendril_pet_t* front_pet )
    : priest_pet_t( sim, p, "void_tendril", PET_VOID_TENDRIL, true ),
      front_pet( front_pet )
  {
    owner_coeff.sp_from_sp = 1.0;
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override;

  void init_action_list() override;

  void summon( timespan_t duration ) override
  {
    priest_pet_t::summon( duration );
  }

  void trigger()
  {
    summon( timespan_t::from_seconds( 10 ) );
  }

  bool init_actions() override
  {
    auto r = priest_pet_t::init_actions();

    // Add all stats as child_stats to front_pet
    if ( front_pet )
    {
      quiet = true;
      for ( auto& stat : stats_list )
      {
        if ( auto front_stat = front_pet->find_stats( stat->name_str ) )
        {
          front_stat->add_child( stat );
        }
      }
    }

    return r;
  }

private:
  void_tendril_pet_t* front_pet;
};

struct void_tendril_mind_flay_t final : public priest_pet_spell_t
{
  void_tendril_mind_flay_t( void_tendril_pet_t& p )
    : priest_pet_spell_t( "mind_flay_void_tendril)", &p,
                          p.o().find_spell( 193473 ) )
  {
    may_crit      = false;
    may_miss      = false;
    channeled     = true;
    hasted_ticks  = false;
    tick_may_crit = true;

    dot_duration = timespan_t::from_seconds( 10.0 );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( 10.0 );
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  void_tendril_pet_t& p()
  {
    return static_cast<void_tendril_pet_t&>( *player );
  }
  const void_tendril_pet_t& p() const
  {
    return static_cast<void_tendril_pet_t&>( *player );
  }
};

void void_tendril_pet_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    action_priority_list_t* precombat = get_action_priority_list( "precombat" );
    // Snapshot stats
    precombat->add_action(
        "snapshot_stats",
        "Snapshot raid buffed stats before combat begins and "
        "pre-potting is done." );

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "mind_flay" );
  }

  priest_pet_t::init_action_list();
}

action_t* void_tendril_pet_t::create_action( const std::string& name,
                                             const std::string& options_str )
{
  if ( name == "mind_flay" )
    return new void_tendril_mind_flay_t( *this );

  return priest_pet_t::create_action( name, options_str );
}

}  // pets/void_tendril

}  // END pets NAMESPACE

namespace actions
{
/* This is a template for common code between priest_spell_t, priest_heal_t and
 * priest_absorb_t.
 * The template is instantiated with either spell_t, heal_t or absorb_t as the
 * 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived
 * class,
 * don't skip it and call spell_t/heal_t or absorb_t directly.
 */
template <typename Base>
struct priest_action_t : public Base
{
public:
  priest_action_t( const std::string& n, priest_t& p,
                   const spell_data_t* s = spell_data_t::nil() )
    : ab( n, &p, s ), priest( p )
  {
    ab::may_crit          = true;
    ab::tick_may_crit     = true;
    ab::weapon_multiplier = 0.0;
  }

  priest_td_t& get_td( player_t* t ) const
  {
    return *( priest.get_target_data( t ) );
  }

  const priest_td_t* find_td( player_t* t ) const
  {
    return priest.find_target_data( t );
  }

  void trigger_void_tendril()
  {
    if ( priest.rppm.call_to_the_void->trigger() )
    {
      for ( auto void_tendril : priest.pets.void_tendril )
      {
        if ( void_tendril->is_sleeping() )
        {
          void_tendril->trigger();
          priest.procs.void_tendril->occur();
          return;
        }
      }
      priest.sim->errorf( "Player %s ran out of void tendrils.\n",
                          priest.name() );
      assert( false );  // Will only get here if there are no available void
                        // tendrils
    }
  }

  bool trigger_shadowy_insight()
  {
    int stack = priest.buffs.shadowy_insight->check();
    if ( priest.buffs.shadowy_insight->trigger() )
    {
      priest.cooldowns.mind_blast->reset( true );

      if ( priest.buffs.shadowy_insight->check() == stack )
      {
        priest.procs.shadowy_insight_overflow->occur();
      }
      else
      {
        priest.procs.shadowy_insight->occur();
      }
      return true;
    }
    return false;
  }

  void trigger_anunds()
  {
    int stack = priest.buffs.anunds_last_breath->check();
    priest.buffs.anunds_last_breath->trigger();

    if ( priest.buffs.anunds_last_breath->check() == stack )
    {
      priest.procs.legendary_anunds_last_breath_overflow->occur();
    }
    else
    {
      priest.procs.legendary_anunds_last_breath->occur();
    }
  }

  double cost() const override
  {
    double c = ab::cost();

    if ( priest.buffs.power_infusion->check() )
    {
      c *= 1.0 + priest.buffs.power_infusion->data().effectN( 2 ).percent();
      c = std::floor( c );
    }

    return c;
  }

  void consume_resource() override
  {
    ab::consume_resource();

    if ( ab::base_execute_time > timespan_t::zero() && !this->channeled )
      priest.buffs.borrowed_time->expire();
  }

protected:
  /* keep reference to the priest. We are sure this will always resolve
   * to the same player as the action_t::player; pointer, and is always valid
   * because it owns the action
   */
  priest_t& priest;

  /// typedef for priest_action_t<action_base_t>
  using base_t = priest_action_t;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = Base;
};

// ==========================================================================
// Priest Absorb
// ==========================================================================

struct priest_absorb_t : public priest_action_t<absorb_t>
{
public:
  priest_absorb_t( const std::string& n, priest_t& player,
                   const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s )
  {
    may_crit      = true;
    tick_may_crit = false;
    may_miss      = false;
  }
};

// ==========================================================================
// Priest Heal
// ==========================================================================

struct priest_heal_t : public priest_action_t<heal_t>
{
  priest_heal_t( const std::string& n, priest_t& player,
                 const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s )
  {
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    am *= 1.0 + priest.buffs.archangel->current_value;

    return am;
  }

  void execute() override
  {
    priest.buffs.archangel->up();  // uptime tracking

    base_t::execute();

    may_crit = true;
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    base_t::impact( s );

    if ( s->result_amount > 0 )
    {
      if ( priest.specialization() != PRIEST_SHADOW &&
           priest.talents.twist_of_fate->ok() &&
           ( save_health_percentage <
             priest.talents.twist_of_fate->effectN( 1 ).base_value() ) )
      {
        priest.buffs.twist_of_fate->trigger();
      }
    }
  }
};

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public priest_action_t<spell_t>
{
  bool is_mind_spell;
  bool is_sphere_of_insanity_spell;

  priest_spell_t( const std::string& n, priest_t& player,
                  const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s ),
      is_mind_spell( false ),
      is_sphere_of_insanity_spell( false )
  {
    weapon_multiplier = 0.0;
  }

  bool usable_moving() const override
  {
    if ( priest.buffs.surrender_to_madness->check() )
    {
      return true;
    }

    return spell_t::usable_moving();
  }

  bool ready() override
  {
    if ( priest.specialization() == PRIEST_SHADOW &&
         priest.buffs.surrender_to_madness_death->check() )
    {
      return false;
    }

    return action_t::ready();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double am = base_t::composite_target_multiplier( t );

    if ( is_mind_spell )
    {
      priest_td_t& td = get_td( t );
      if ( priest.active_items.mental_fatigue )
      {
        am *= 1.0 + td.buffs.mental_fatigue->check_stack_value();
      }
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    double save_health_percentage = s->target->health_percentage();

    if ( priest.buffs.mind_sear_on_hit_reset->check() == 2 )
    {
      priest.buffs.mind_sear_on_hit_reset->decrement();
    }

    base_t::impact( s );

    if ( is_mind_spell )
    {
      priest_td_t& td = get_td( s->target );
      td.buffs.mental_fatigue->up();  // benefit tracking
    }

    if ( result_is_hit( s->result ) )
    {
      if ( priest.specialization() == PRIEST_SHADOW &&
           priest.talents.twist_of_fate->ok() &&
           ( save_health_percentage <
             priest.talents.twist_of_fate->effectN( 1 ).base_value() ) )
      {
        priest.buffs.twist_of_fate->trigger();
      }

      if ( is_sphere_of_insanity_spell &&
           priest.buffs.sphere_of_insanity->up() && s->result_amount > 0 )
      {
        priest.buffs.sphere_of_insanity->current_value += s->result_amount;
      }
    }
  }

  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    base_t::assess_damage( type, s );

    if ( aoe == 0 && result_is_hit( s->result ) &&
         priest.buffs.vampiric_embrace->up() )
      trigger_vampiric_embrace( s );
  }

  /* Based on previous implementation ( pets don't count but get full heal )
   * and http://www.wowhead.com/spell=15286#comments:id=1796701
   * Last checked 2013/05/25
   */
  void trigger_vampiric_embrace( action_state_t* s )
  {
    double amount = s->result_amount;
    amount *= priest.buffs.vampiric_embrace->data().effectN( 1 ).percent() *
              ( 1.0 +
                priest.talents.sanlayn->effectN( 2 )
                    .percent() );  // FIXME additive or multiplicate?

    // Get all non-pet, non-sleeping players
    std::vector<player_t*> ally_list;
    range::remove_copy_if( sim->player_no_pet_list.data(),
                           back_inserter( ally_list ), player_t::_is_sleeping );

    for ( player_t* ally : ally_list )
    {
      ally->resource_gain( RESOURCE_HEALTH, amount,
                           ally->gains.vampiric_embrace );

      for ( pet_t* pet : ally->pet_list )
      {
        pet->resource_gain( RESOURCE_HEALTH, amount,
                            pet->gains.vampiric_embrace );
      }
    }
  }

  void generate_insanity( double num_amount, gain_t* g = nullptr )
  {
    if ( priest.specialization() == PRIEST_SHADOW )
    {
      double amount                           = num_amount;
      double amount_from_power_infusion       = 0.0;
      double amount_from_surrender_to_madness = 0.0;

      if ( priest.buffs.surrender_to_madness->check() &&
           priest.buffs.power_infusion->check() )
      {
        double total_amount =
            amount *
            ( 1.0 +
              priest.buffs.power_infusion->data().effectN( 3 ).percent() ) *
            ( 1.0 +
              priest.talents.surrender_to_madness->effectN( 1 ).percent() );

        amount_from_surrender_to_madness =
            amount *
            priest.talents.surrender_to_madness->effectN( 1 ).percent();

        // Since this effect is multiplicitive, we'll give the extra to Power
        // Infusion since it does not last as long as Surrender to Madness
        amount_from_power_infusion =
            total_amount - amount - amount_from_surrender_to_madness;

        // Make sure the maths line up.
        assert( total_amount ==
                amount + amount_from_power_infusion +
                    amount_from_surrender_to_madness );
      }
      else if ( priest.buffs.surrender_to_madness->check() )
      {
        amount_from_surrender_to_madness =
            ( amount * ( 1.0 +
                         priest.talents.surrender_to_madness->effectN( 1 )
                             .percent() ) ) -
            amount;
      }
      else if ( priest.buffs.power_infusion->check() )
      {
        amount_from_power_infusion =
            ( amount *
              ( 1.0 +
                priest.buffs.power_infusion->data().effectN( 3 ).percent() ) ) -
            amount;
      }

      priest.resource_gain( RESOURCE_INSANITY, amount, g, this );

      if ( amount_from_power_infusion > 0.0 )
      {
        priest.resource_gain( RESOURCE_INSANITY, amount_from_power_infusion,
                              priest.gains.insanity_power_infusion, this );
      }

      if ( amount_from_surrender_to_madness > 0.0 )
      {
        priest.resource_gain(
            RESOURCE_INSANITY, amount_from_surrender_to_madness,
            priest.gains.insanity_surrender_to_madness, this );
      }
    }
  }
};

namespace spells
{
// ==========================================================================
// Priest Spells
// ==========================================================================

struct angelic_feather_t final : public priest_spell_t
{
  angelic_feather_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "angelic_feather", p,
                      p.find_class_spell( "Angelic Feather" ) )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    s->target->buffs.angelic_feather->trigger();
  }
};

struct archangel_t final : public priest_spell_t
{
  archangel_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "archangel", p, p.specs.archangel )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.archangel->trigger();
  }

  bool ready() override
  {
    if ( !priest.buffs.holy_evangelism->check() )
      return false;
    else
      return priest_spell_t::ready();
  }
};

struct dispersion_t final : public priest_spell_t
{
  dispersion_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "dispersion", player,
                      player.find_class_spell( "Dispersion" ) )
  {
    parse_options( options_str );

    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_duration   = timespan_t::from_seconds( 6.0 );

    ignore_false_positive = true;
    channeled             = true;
    harmful               = false;
    tick_may_crit         = false;
    hasted_ticks          = false;
    may_miss              = false;

    if ( priest.artifact.from_the_shadows.rank() )
    {
      cooldown->duration =
          data().cooldown() + priest.artifact.from_the_shadows.time_value();
    }
  }

  void execute() override
  {
    priest.buffs.dispersion->trigger();

    priest_spell_t::execute();
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( 6.0 );
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );
    // reset() instead of expire() because it was not properly creating the buff
    // every 2nd time
    priest.buffs.dispersion->reset();
  }
};

/// Divine Star Base Spell, used for both heal and damage spell.
template <class Base>
struct divine_star_base_t : public Base
{
private:
  typedef Base
      ab;  // the action base ("ab") type (priest_spell_t or priest_heal_t)
public:
  typedef divine_star_base_t base_t;

  divine_star_base_t* return_spell;

  divine_star_base_t( const std::string& n, priest_t& p,
                      const spell_data_t* spell_data,
                      bool is_return_spell = false )
    : ab( n, p, spell_data ),
      return_spell( ( is_return_spell
                          ? nullptr
                          : new divine_star_base_t( n, p, spell_data, true ) ) )
  {
    ab::aoe = -1;

    ab::proc = ab::background = true;
  }

  // Divine Star will damage and heal targets twice, once on the way out and
  // again on the way back. This is determined by distance from the target.
  // If we are too far away, it misses completely. If we are at the very
  // edge distance wise, it will only hit once. If we are within range (and
  // aren't moving such that it would miss the target on the way out and/or
  // back), it will hit twice. Threshold is 24 yards, per tooltip and tests
  // for 2 hits. 28 yards is the threshold for 1 hit.
  void execute() override
  {
    double distance;

    if ( ab::player->sim->distance_targeting_enabled )
      distance = ab::player->get_player_distance( *ab::target );
    else
      distance = ab::player->current.distance;

    if ( distance <= 28 )
    {
      ab::execute();

      if ( return_spell && distance <= 24 )
        return_spell->execute();
    }
  }
};

struct divine_star_t final : public priest_spell_t
{
  divine_star_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "divine_star", p, p.talents.divine_star ),
      _heal_spell( new divine_star_base_t<priest_heal_t>(
          "divine_star_heal", p, p.find_spell( 110745 ) ) ),
      _dmg_spell( new divine_star_base_t<priest_spell_t>(
          "divine_star_damage", p, p.find_spell( 122128 ) ) )
  {
    parse_options( options_str );

    dot_duration = base_tick_time = timespan_t::zero();

    add_child( _heal_spell );
    add_child( _dmg_spell );
  }

  void execute() override
  {
    priest_spell_t::execute();

    _heal_spell->execute();
    _dmg_spell->execute();
  }

private:
  action_t* _heal_spell;
  action_t* _dmg_spell;
};

/// Halo Base Spell, used for both damage and heal spell.
template <class Base>
struct halo_base_t : public Base
{
public:
  halo_base_t( const std::string& n, priest_t& p, const spell_data_t* s )
    : Base( n, p, s )
  {
    Base::aoe        = -1;
    Base::background = true;

    if ( Base::data().ok() )
    {
      // Reparse the correct effect number, because we have two competing ones
      // (
      // were 2 > 1 always wins out )
      Base::parse_effect_data( Base::data().effectN( 1 ) );
    }
    Base::radius = 30;
    Base::range  = 0;
  }

  timespan_t distance_targeting_travel_time( action_state_t* s ) const override
  {
    return timespan_t::from_seconds(
        s->action->player->get_player_distance( *s->target ) /
        Base::travel_speed );
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    double cda = Base::calculate_direct_amount( s );

    // Source: Ghostcrawler 2012-06-20
    // http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97

    double distance;
    if ( s->action->player->sim->distance_targeting_enabled )
    {
      distance = s->action->player->get_player_distance( *s->target );
    }
    else
    {
      distance =
          std::fabs( s->action->player->current.distance -
                     s->target->current
                         .distance );  // Distance from the caster to the target
    }

    // double mult = 0.5 * pow( 1.01, -1 * pow( ( distance - 25 ) / 2, 4 ) ) +
    // 0.1 + 0.015 * distance;
    double mult = 0.5 * exp( -0.00995 * pow( distance / 2 - 12.5, 4 ) ) + 0.1 +
                  0.015 * distance;

    return cda * mult;
  }
};

struct halo_t final : public priest_spell_t
{
  halo_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "halo", p, p.talents.halo ),
      _heal_spell( new halo_base_t<priest_heal_t>( "halo_heal", p,
                                                   p.find_spell( 120692 ) ) ),
      _dmg_spell( new halo_base_t<priest_spell_t>( "halo_damage", p,
                                                   p.find_spell( 120696 ) ) )
  {
    parse_options( options_str );

    add_child( _heal_spell );
    add_child( _dmg_spell );
  }

  void execute() override
  {
    priest_spell_t::execute();

    _heal_spell->execute();
    _dmg_spell->execute();
  }

private:
  action_t* _heal_spell;
  action_t* _dmg_spell;
};

/// Holy Fire Base Spell, used for both Holy Fire and its overriding spell Puge
/// the Wicked
struct holy_fire_base_t : public priest_spell_t
{
  struct glyph_of_the_inquisitor_backlash_t : public priest_spell_t
  {
    double spellpower;
    double multiplier;
    bool critical;

    glyph_of_the_inquisitor_backlash_t( priest_t& p )
      : priest_spell_t( "glyph_of_the_inquisitor_backlash", p,
                        p.talents.power_word_solace->ok()
                            ? p.find_spell( 129250 )
                            : p.find_class_spell( "Holy Fire" ) ),
        spellpower( 0.0 ),
        multiplier( 1.0 ),
        critical( false )
    {
      background = true;
      harmful    = false;
      proc       = true;
      may_crit   = false;
      callbacks  = false;

      target = &priest;

      ability_lag        = sim->default_aura_delay;
      ability_lag_stddev = sim->default_aura_delay_stddev;
    }

    void init() override
    {
      priest_spell_t::init();

      stats->type = STATS_NEUTRAL;
    }

    double composite_spell_power() const override
    {
      return spellpower;
    }

    double composite_da_multiplier(
        const action_state_t* /* state */ ) const override
    {
      double d = multiplier;
      d /= 5;

      if ( critical )
        d *= 2;

      return d;
    }
  };

  glyph_of_the_inquisitor_backlash_t* backlash;

  holy_fire_base_t( const std::string& name, priest_t& p,
                    const spell_data_t* sd )
    : priest_spell_t( name, p, sd ),
      backlash( new glyph_of_the_inquisitor_backlash_t( p ) )
  {
    procs_courageous_primal_diamond = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.holy_evangelism->trigger();
  }

  double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    m *= 1.0 + ( priest.buffs.holy_evangelism->check() *
                 priest.buffs.holy_evangelism->data().effectN( 1 ).percent() );

    return m;
  }
};

struct holy_fire_t final : public holy_fire_base_t
{
  holy_fire_t( priest_t& player, const std::string& options_str )
    : holy_fire_base_t( "holy_fire", player,
                        player.find_class_spell( "Holy Fire" ) )
  {
    parse_options( options_str );
  }
};

struct levitate_t final : public priest_spell_t
{
  levitate_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "levitate", p, p.find_class_spell( "Levitate" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

/// Dummy spell for triggering Lingering Insanity Buff as a pre-combat action
struct lingering_insanity_t final : public priest_spell_t
{
  int stacks_to_trigger;
  timespan_t duration;

  lingering_insanity_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "trigger_lingering_insanity", p, spell_data_t::nil() ),
      stacks_to_trigger( 0 ),
      duration( timespan_t::min() )
  {
    add_option( opt_int( "stacks", stacks_to_trigger, 0, 100 ) );
    add_option( opt_timespan( "duration", duration, timespan_t::zero(),
                              p.buffs.lingering_insanity->buff_duration ) );
    parse_options( options_str );
    ignore_false_positive = true;
    harmful               = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.lingering_insanity->trigger(
        stacks_to_trigger, buff_t::DEFAULT_VALUE(), -1, duration );
  }
};

struct mind_spike_detonation_t final : public priest_spell_t
{
  double detonation_amount;
  player_t* detonation_target;

  mind_spike_detonation_t( priest_t& p )
    : priest_spell_t( "mind_spike_detonation", p,
                      p.find_spell( 217676 ) ),  //.talents.mind_spike)
      detonation_amount( 0.0 ),
      detonation_target( nullptr )
  {
    may_crit                    = false;
    background                  = true;
    proc                        = false;
    callbacks                   = true;
    may_miss                    = false;
    aoe                         = -1;
    is_sphere_of_insanity_spell = true;
    range                       = 8.0;
    trigger_gcd                 = timespan_t::zero();
    school                      = SCHOOL_SHADOWFROST;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    if ( state->target == detonation_target )  // This is the target we
                                               // detonated against. Do full
                                               // damage
    {
      return detonation_amount;
    }
    else  // Other targets, do half damage.
    {
      return detonation_amount / 2.0;
    }
  }

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    priest_td_t& td = get_td( state->target );

    if ( state->target == detonation_target )  // This is the target we
                                               // detonated against. Remove
                                               // debuff
    {
      td.buffs.mind_spike->expire();
    }
  }

  // Trigger mind spike explosion
  void trigger( player_t* target )
  {
    priest_td_t& td = get_td( target );

    detonation_amount = td.buffs.mind_spike->value();
    detonation_target = target;

    if ( priest.sim->debug )
    {
      priest.sim->out_debug << priest.name()
                            << " triggered Mind Spike Detonation.";
    }
    schedule_execute();
  }
};

struct mind_blast_t final : public priest_spell_t
{
private:
  double insanity_gain;

public:
  mind_blast_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "mind_blast", player,
                      player.find_class_spell( "Mind Blast" ) )
  {
    parse_options( options_str );
    is_mind_spell               = true;
    is_sphere_of_insanity_spell = true;

    insanity_gain = data().effectN( 2 ).resource( RESOURCE_INSANITY );
    insanity_gain *=
        ( 1.0 + priest.talents.fortress_of_the_mind->effectN( 2 ).percent() );
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data

    spell_power_mod.direct *=
        1.0 + player.talents.fortress_of_the_mind->effectN( 4 ).percent();

    if ( player.artifact.mind_shattering.rank() )
    {
      base_multiplier *= 1.0 + player.artifact.mind_shattering.percent();
    }

    if ( !priest.active_spells.mind_spike_detonation &&
         priest.talents.mind_spike->ok() )
    {
      priest.active_spells.mind_spike_detonation =
          new mind_spike_detonation_t( player );
      add_child( priest.active_spells.mind_spike_detonation );
    }

    // Disable dynamic hasted cooldown scaling as it is causing a double-dip in
    // haste. -- Twintop 2016/09/17
    // cooldown->hasted = true;
  }

  void init() override
  {
    if ( priest.active_items.mangazas_madness &&
         priest.cooldowns.mind_blast->charges == 1 )
    {
      priest.cooldowns.mind_blast->charges +=
          priest.active_items.mangazas_madness->driver()
              ->effectN( 1 )
              .base_value();
    }
    priest.cooldowns.mind_blast->hasted = true;

    priest_spell_t::init();
  }

  void schedule_execute( action_state_t* s ) override
  {
    priest_spell_t::schedule_execute( s );

    priest.buffs.shadowy_insight->expire();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.power_overwhelming->trigger();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    generate_insanity( insanity_gain, priest.gains.insanity_mind_blast );

    priest_td_t& td = get_td( s->target );

    if ( priest.active_spells.mind_spike_detonation )
    {
      if ( td.buffs.mind_spike->up() )
      {
        priest.active_spells.mind_spike_detonation->trigger( s->target );
      }
    }
  }

  timespan_t execute_time() const override
  {
    if ( priest.buffs.shadowy_insight->check() )
    {
      return timespan_t::zero();
    }

    timespan_t et = priest_spell_t::execute_time();

    return et;
  }

  timespan_t cooldown_base_duration( const cooldown_t& cooldown ) const override
  {
    timespan_t cd = priest_spell_t::cooldown_base_duration( cooldown );
    if ( priest.buffs.voidform->check() )
    {
      cd += -timespan_t::from_seconds( 3.0 );
    }
    return cd;
  }

  void update_ready( timespan_t cd_duration ) override
  {
    // Uptime tracking
    priest.buffs.voidform->up();

    // Shadowy Insight has proc'd during the cast of Mind Blast, the cooldown
    // reset is deferred to the finished cast, instead of "eating" it.
    if ( priest.buffs.shadowy_insight->check() )
    {
      cd_duration            = timespan_t::zero();
      cooldown->last_charged = sim->current_time();

      if ( sim->debug )
      {
        sim->out_debug.printf(
            "%s shadowy insight proc occured during %s cast. Deferring "
            "cooldown reset.",
            priest.name(), name() );
      }
    }

    priest_spell_t::update_ready( cd_duration );
  }
};

struct mind_flay_t final : public priest_spell_t
{
  double insanity_gain;

  mind_flay_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "mind_flay", p,
                      p.find_specialization_spell( "Mind Flay" ) ),
      insanity_gain(
          data().effectN( 3 ).resource( RESOURCE_INSANITY ) *
          ( 1.0 + p.talents.fortress_of_the_mind->effectN( 1 ).percent() ) )
  {
    parse_options( options_str );

    may_crit                    = false;
    channeled                   = true;
    hasted_ticks                = false;
    use_off_gcd                 = true;
    is_mind_spell               = true;
    is_sphere_of_insanity_spell = true;
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data

    if ( p.artifact.void_siphon.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.void_siphon.percent();
    }

    spell_power_mod.tick *=
        1.0 + p.talents.fortress_of_the_mind->effectN( 3 ).percent();
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    if ( priest.talents.void_ray->ok() && priest.buffs.void_ray->check() )
      am *= 1.0 +
            priest.buffs.void_ray->check() *
                priest.buffs.void_ray->data().effectN( 1 ).percent();

    return am;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest.active_items.mental_fatigue )
    {
      if ( d->state && result_is_hit( d->state->result ) )
      {
        // Assumes trigger on hit, not on damage
        priest_td_t& td = get_td( d->state->target );
        td.buffs.mental_fatigue->trigger();
      }
    }

    if ( priest.talents.void_ray->ok() )
    {
      priest.buffs.void_ray->trigger();
    }

    trigger_void_tendril();

    generate_insanity( insanity_gain, priest.gains.insanity_mind_flay );
  }

  bool ready() override
  {
    if ( priest.talents.mind_spike->ok() )
      return false;

    return priest_spell_t::ready();
  }
};

struct mind_sear_tick_t final : public priest_spell_t
{
  double insanity_gain;

  // TODO: Mind Sear is missing damage information in spell data
  mind_sear_tick_t( priest_t& p, const spell_data_t* mind_sear )
    : priest_spell_t( "mind_sear_tick", p, mind_sear->effectN( 1 ).trigger() ),
      insanity_gain( 1.5 )  // TODO: Missing from spell data - Hotfixed to 1.5
                            // from 1 on 2016-09-26.
  {
    background  = true;
    dual        = true;
    aoe         = -1;
    callbacks   = false;
    direct_tick = true;
    use_off_gcd = true;
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data
  }

  void impact( action_state_t* state ) override
  {
    priest_spell_t::impact( state );

    if ( result_is_hit( state->result ) )
    {
      generate_insanity( insanity_gain, priest.gains.insanity_mind_sear );
    }
  }
};

struct mind_sear_t final : public priest_spell_t
{
  mind_sear_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "mind_sear", p,
                      p.find_specialization_spell( ( "Mind Sear" ) ) )
  {
    parse_options( options_str );
    channeled           = true;
    may_crit            = false;
    hasted_ticks        = false;
    dynamic_tick_action = true;
    tick_zero           = false;
    is_mind_spell       = true;

    if ( p.artifact.void_corruption.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.void_corruption.percent();
    }

    tick_action = new mind_sear_tick_t( p, p.find_class_spell( "Mind Sear" ) );
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    if ( priest.talents.void_ray->ok() && priest.buffs.void_ray->check() )
      am *= 1.0 +
            priest.buffs.void_ray->check() *
                priest.buffs.void_ray->data().effectN( 1 ).percent();

    return am;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest.talents.void_ray->ok() )
    {
      priest.buffs.void_ray->trigger();
    }
  }

  void last_tick( dot_t* d ) override
  {
    if ( d->current_tick == d->num_ticks )
    {
      priest.buffs.mind_sear_on_hit_reset->expire();
    }

    priest_spell_t::last_tick( d );
  }

  /// Legendary the_twins_painful_touch
  void spread_twins_painsful_dots( action_state_t* s )
  {
    const priest_td_t* td = find_td( s->target );
    if ( !td )
    {
      // If we do not have targetdata, the target does not have any dots. bail
      // out.
      return;
    }

    std::array<dot_t*, 2> dots = {
        {td->dots.shadow_word_pain, td->dots.vampiric_touch}};

    // First check if there is even a dot active, otherwise we can bail out as
    // well.
    if ( range::find_if( dots, []( const dot_t* d ) {
           return d->remains() > timespan_t::zero();
         } ) == dots.end() )
    {
      if ( sim->debug )
      {
        sim->out_debug.printf(
            "%s %s will not spread the_twins_painful_touch dots because no dot "
            "is on the target.",
            priest.name(), name() );
      }
      return;
    }

    // Now find 2 targets to spread dots to.
    double max_distance  = 10.0;
    unsigned max_targets = 2;
    std::vector<player_t*> valid_targets;
    range::remove_copy_if(
        sim->target_list.data(), std::back_inserter( valid_targets ),
        [s, max_distance]( const player_t* p ) {
          return s->target->get_player_distance( *p ) > max_distance;
        } );
    // Just cut off to max_targets. No special selection.
    if ( valid_targets.size() > max_targets )
    {
      valid_targets.resize( max_targets );
    }
    if ( sim->debug )
    {
      std::string targets;
      sim->out_debug.printf(
          "%s %s selected targets for the_twins_painful_touch dots: %s.",
          priest.name(), name(), targets.c_str() );
    }

    // spread dots to targets
    for ( dot_t* dot : dots )
    {
      for ( player_t* target : valid_targets )
      {
        dot->copy( target, DOT_COPY_CLONE );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    // Mind Sear does on-hit damage only when there is a GCD of another ability
    // between it, so chaining Mind Sears doesn't allow for the on-hit to happen
    // again.
    if ( priest.buffs.mind_sear_on_hit_reset->check() == 0 )
    {
      priest.buffs.mind_sear_on_hit_reset->trigger( 2, 1, 1,
                                                    tick_time( s ) * 6 );
      tick_zero = true;
    }
    else
    {
      priest.buffs.mind_sear_on_hit_reset->trigger( 1 );
      tick_zero = false;
    }

    priest_spell_t::impact( s );

    // Legendary the_twins_painful_touch
    if ( priest.buffs.the_twins_painful_touch->up() )
    {
      spread_twins_painsful_dots( s );
      priest.buffs.the_twins_painful_touch->expire();
    }
  }
};

struct mind_spike_t final : public priest_spell_t
{
  double insanity_gain;

  mind_spike_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "mind_spike", p, p.talents.mind_spike ),
      insanity_gain(
          data().effectN( 3 ).resource( RESOURCE_INSANITY ) *
          ( 1.0 + p.talents.fortress_of_the_mind->effectN( 1 ).percent() ) )
  {
    parse_options( options_str );
    is_mind_spell               = true;
    is_sphere_of_insanity_spell = true;
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data

    if ( p.artifact.void_siphon.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.void_siphon.percent();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    generate_insanity( insanity_gain, priest.gains.insanity_mind_spike );

    if ( result_is_hit( s->result ) )
    {
      priest_td_t& td = get_td( s->target );

      int prev_stacks    = 0;
      double prev_damage = 0.0;

      if ( td.buffs.mind_spike->up() )
      {
        prev_stacks = td.buffs.mind_spike->check();
        prev_damage = td.buffs.mind_spike->value();
        td.buffs.mind_spike->increment();
      }
      else
      {
        td.buffs.mind_spike->trigger();
      }

      if ( td.buffs.mind_spike->check() == td.buffs.mind_spike->max_stack() &&
           td.buffs.mind_spike->check() == prev_stacks )
      {
        td.buffs.mind_spike->current_value =
            round( ( prev_damage / td.buffs.mind_spike->max_stack() ) *
                       ( td.buffs.mind_spike->max_stack() - 1 ) +
                   ( s->result_amount *
                     priest.talents.mind_spike->effectN( 2 ).percent() ) );
      }
      else
      {
        td.buffs.mind_spike->current_value =
            prev_damage +
            s->result_amount *
                priest.talents.mind_spike->effectN( 2 ).percent();
        if ( priest.sim->debug )
        {
          priest.sim->out_debug.printf(
              "%s adds %d to mind_spike_detonation, now %d total at %i stacks",
              priest.name(),
              s->result_amount *
                  priest.talents.mind_spike->effectN( 2 ).percent(),
              td.buffs.mind_spike->current_value,
              td.buffs.mind_spike->stack() );
        }
      }

      if ( priest.active_items.mental_fatigue )
      {
        // Assumes trigger on hit, not on damage
        td.buffs.mental_fatigue->trigger();
      }
    }

    trigger_void_tendril();
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( priest.talents.void_ray->ok() )
    {
      priest.buffs.void_ray->trigger();
    }
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    if ( priest.talents.void_ray->ok() && priest.buffs.void_ray->check() )
    {
      am *= 1.0 +
            priest.buffs.void_ray->check() *
                priest.buffs.void_ray->data().effectN( 1 ).percent();
    }

    return am;
  }

  bool ready() override
  {
    if ( !priest.talents.mind_spike->ok() )
      return false;

    return priest_spell_t::ready();
  }
};

struct pain_suppression_t final : public priest_spell_t
{
  pain_suppression_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "pain_suppression", p,
                      p.find_class_spell( "Pain Suppression" ) )
  {
    parse_options( options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to
    // the player instead
    if ( target->is_enemy() || target->is_add() )
    {
      target = &p;
    }

    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    target->buffs.pain_supression->trigger();
  }
};

/// Penance damage spell
struct penance_t final : public priest_spell_t
{
  struct penance_tick_t final : public priest_spell_t
  {
    penance_tick_t( priest_t& p, stats_t* stats )
      : priest_spell_t( "penance_tick", p, p.dbc.spell( 47666 ) )
    {
      background  = true;
      dual        = true;
      direct_tick = true;
      //      can_trigger_atonement = priest.specs.atonement->ok();

      this->stats = stats;
    }

    bool verify_actor_level() const override
    {
      // Tick spell data is restricted to level 60+, even though main spell is
      // level10+
      return true;
    }

    void impact( action_state_t* state ) override
    {
      priest_spell_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        // TODO: Check if this is the correct place & check for triggering.
        // 2015-04-14: Your Penance [...] each time it deals damage or heals.
        priest.buffs.reperation->trigger();
      }
    }
  };

  penance_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "penance", p, p.find_class_spell( "Penance" ) )
  {
    parse_options( options_str );

    may_crit       = false;
    may_miss       = false;
    channeled      = true;
    tick_zero      = true;
    dot_duration   = timespan_t::from_seconds( 2.0 );
    base_tick_time = timespan_t::from_seconds( 1.0 );

    // HACK: Set can_trigger here even though the tick spell actually
    // does the triggering. We want atonement_penance to be created in
    // priest_heal_t::init() so that it appears in the report.
    //    can_trigger_atonement = priest.specs.atonement->ok();

    if ( priest.talents.castigation->ok() )
    {
      // Add 1 extra millisecond, so we only get 4 ticks instead of an extra
      // tiny 5th tick.
      base_tick_time =
          timespan_t::from_seconds( 2.0 / 3 ) + timespan_t::from_millis( 1 );
    }
    dot_duration += priest.sets.set( PRIEST_DISCIPLINE, T17, B2 )
                        ->effectN( 1 )
                        .time_value();

    dynamic_tick_action = true;
    tick_action         = new penance_tick_t( p, stats );
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    // Do not haste ticks!
    return base_tick_time;
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest.buffs.holy_evangelism->trigger();  // re-checked 2014/07/07:
                                              // offensive penance grants
                                              // evangelism stacks, even though
                                              // not mentioned in the tooltip.

    if ( priest.sets.has_set_bonus( PRIEST_DISCIPLINE, T17, B2 ) )
      priest.buffs.holy_evangelism->trigger();  // Set bonus says "...and
                                                // generates 2 stacks of
                                                // Evangelism." Need to check if
                                                // this happens up front or
                                                // after a particular tick. -
                                                // Twintop 2014/07/11
  }
};

struct power_infusion_t final : public priest_spell_t
{
  power_infusion_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "power_infusion", p, p.talents.power_infusion )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest.buffs.power_infusion->trigger();
  }
};

struct power_word_solace_t final : public holy_fire_base_t
{
  power_word_solace_t( priest_t& player, const std::string& options_str )
    : holy_fire_base_t( "power_word_solace", player,
                        player.find_spell( 129250 ) )
  {
    parse_options( options_str );

    travel_speed = 0.0;  // DBC has default travel taking 54seconds.....
  }

  void impact( action_state_t* s ) override
  {
    holy_fire_base_t::impact( s );

    double amount = data().effectN( 3 ).percent() / 100.0 *
                    priest.resources.max[ RESOURCE_MANA ];
    priest.resource_gain( RESOURCE_MANA, amount,
                          priest.gains.power_word_solace );
  }
};

struct purge_the_wicked_t final : public priest_spell_t
{
  struct purge_the_wicked_dot_t final : public priest_spell_t
  {
    purge_the_wicked_dot_t( priest_t& p )
      : priest_spell_t( "purge_the_wicked", p,
                        p.talents.purge_the_wicked->effectN( 2 ).trigger() )
    {
      may_crit = true;
      // tick_zero = false;
      energize_type =
          ENERGIZE_NONE;  // disable resource generation from spell data
      background = true;
    }
  };

  purge_the_wicked_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "purge_the_wicked", p, p.talents.purge_the_wicked )
  {
    parse_options( options_str );

    may_crit = true;
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data
    execute_action = new purge_the_wicked_dot_t( p );
  }
};

struct schism_t final : public priest_spell_t
{
  schism_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "schism", player, player.talents.schism )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest_td_t& td = get_td( s->target );

    if ( result_is_hit( s->result ) )
    {
      // Assumption scamille 2016-07-28: Schism only applied on hit
      td.buffs.schism->trigger();
    }
  }
};

struct shadow_word_death_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_word_death_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_death", p,
                      p.find_class_spell( "Shadow Word: Death" ) ),
      insanity_gain(
          p.find_spell( 190714 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    const spell_data_t* shadow_word_death_2 =
        p.find_specialization_spell( 231689 );
    if ( shadow_word_death_2 )
    {
      cooldown->charges += shadow_word_death_2->effectN( 1 ).base_value();
    }
    if ( p.artifact.deaths_embrace.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.deaths_embrace.percent();
    }
  }

  void impact( action_state_t* s ) override
  {
    double total_insanity_gain    = 0.0;
    double save_health_percentage = s->target->health_percentage();

    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // http://us.battle.net/wow/en/forum/topic/20743676204#3 - 2016/05/12
      // SWD always grants at least 10 Insanity.
      // TODO: Add in a custom buff that checks after 1 second to see if the
      // target SWD was cast on is now dead.
      // TODO: Check in beta if the target is dead vs. SWD is the killing blow.
      total_insanity_gain = 10.0;

      if ( priest.talents.reaper_of_souls->ok() ||
           ( ( save_health_percentage > 0.0 ) &&
             ( s->target->health_percentage() <= 0.0 ) ) )
      {
        total_insanity_gain = insanity_gain;
      }

      generate_insanity( total_insanity_gain,
                         priest.gains.insanity_shadow_word_death );
    }
  }

  bool ready() override
  {
    if ( !priest_spell_t::ready() )
      return false;

    if ( ( priest.talents.reaper_of_souls->ok() &&
           target->health_percentage() < 35.0 ) ||
         target->health_percentage() < 20.0 )  // FIXME Spelldata effectN(2)?
    {
      return true;
    }

    return false;
  }
};

struct shadow_crash_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_crash_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_crash", p, p.talents.shadow_crash ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    const spell_data_t* missile = priest.find_spell( 205386 );
    school                      = missile->get_school_type();
    spell_power_mod.direct      = missile->effectN( 1 ).sp_coeff();
    aoe                         = -1;
    radius                      = data().effectN( 1 ).radius();

    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data
  }

  void execute() override
  {
    priest_spell_t::execute();

    generate_insanity( insanity_gain, priest.gains.insanity_shadow_crash );
  }
};

struct shadowform_t final : public priest_spell_t
{
  shadowform_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadowform", p, p.find_class_spell( "Shadowform" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.shadowform_state->trigger();
    priest.buffs.shadowform->trigger();
  }
};

struct shadowy_apparition_spell_t final : public priest_spell_t
{
  double insanity_gain;

  shadowy_apparition_spell_t( priest_t& p )
    : priest_spell_t( "shadowy_apparitions", p, p.find_spell( 78203 ) ),
      insanity_gain( 4 )  // Spell Data missing?
                          // data().effectN(2).resource(RESOURCE_INSANITY) *
  // priest.talents.auspicious_spirits->effectN(2).percent() ) )
  {
    background   = true;
    proc         = false;
    callbacks    = true;
    may_miss     = false;
    trigger_gcd  = timespan_t::zero();
    travel_speed = 6.0;
    const spell_data_t* dmg_data =
        p.find_spell( 148859 );  // Hardcoded into tooltip 2014/06/01

    parse_effect_data( dmg_data->effectN( 1 ) );
    school = SCHOOL_SHADOW;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest.talents.auspicious_spirits->ok() )
    {
      generate_insanity( insanity_gain,
                         priest.gains.insanity_auspicious_spirits );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    d *= 1.0 + priest.talents.auspicious_spirits->effectN( 1 ).percent();

    return d;
  }

  /* Trigger a shadowy apparition
   */
  void trigger()
  {
    if ( priest.sim->debug )
      priest.sim->out_debug << priest.name()
                            << " triggered shadowy apparition.";

    priest.procs.shadowy_apparition->occur();
    schedule_execute();
  }
};

struct sphere_of_insanity_spell_t final : public priest_spell_t
{
  double damage_amount;

  sphere_of_insanity_spell_t( priest_t& p )
    : priest_spell_t( "sphere_of_insanity", p, p.find_spell( 194182 ) ),
      damage_amount( 0.0 )
  {
    may_crit    = false;
    background  = true;
    proc        = false;
    callbacks   = true;
    may_miss    = false;
    aoe         = -1;
    range       = 0.0;
    radius      = 100.0;
    trigger_gcd = timespan_t::zero();
    school      = SCHOOL_SHADOW;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    dot_t* d = state->target->get_dot( "shadow_word_pain", &priest );

    if ( d )  // Shadow Word: Pain is ticking on the target; deal damage
    {
      return damage_amount *
             0.05;  // TODO: replace with data from spell 194182, effect #3
    }
    else
    {
      return 0.0;
    }
  }

  /// Trigger a sphere of insanity damage
  void trigger( double amount )
  {
    if ( priest.sim->debug )
    {
      priest.sim->out_debug << priest.name()
                            << " triggered Sphere of Insanity damage.";
    }
    damage_amount = amount;
    schedule_execute();
  }
};

struct shadow_word_pain_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_word_pain_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_pain", p,
                      p.find_class_spell( "Shadow Word: Pain" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    may_crit  = true;
    tick_zero = false;
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data

    if ( p.artifact.to_the_pain.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.to_the_pain.percent();
    }

    if ( priest.specs.shadowy_apparitions->ok() &&
         !priest.active_spells.shadowy_apparitions )
    {
      priest.active_spells.shadowy_apparitions =
          new shadowy_apparition_spell_t( p );
      if ( !priest.artifact.unleash_the_shadows.rank() )
      {
        // If SW:P is the only action having SA, then we can add it as a child
        // stat
        add_child( priest.active_spells.shadowy_apparitions );
      }
    }

    if ( priest.artifact.sphere_of_insanity.rank() &&
         !priest.active_spells.sphere_of_insanity )
    {
      priest.active_spells.sphere_of_insanity =
          new sphere_of_insanity_spell_t( p );
      add_child( priest.active_spells.sphere_of_insanity );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest.buffs.sphere_of_insanity->up() )
    {
      priest.active_spells.sphere_of_insanity->trigger(
          priest.buffs.sphere_of_insanity->current_value );
      priest.buffs.sphere_of_insanity->current_value = 0;
    }

    generate_insanity( insanity_gain,
                       priest.gains.insanity_shadow_word_pain_onhit );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( priest.active_spells.shadowy_apparitions &&
         ( d->state->result_amount > 0 ) )
    {
      if ( d->state->result == RESULT_CRIT )
      {
        priest.active_spells.shadowy_apparitions->trigger();
      }
    }

    if ( d->state->result_amount > 0 )
    {
      trigger_shadowy_insight();
    }

    if ( priest.buffs.sphere_of_insanity->up() )
    {
      priest.active_spells.sphere_of_insanity->trigger(
          priest.buffs.sphere_of_insanity->current_value );
      priest.buffs.sphere_of_insanity->current_value = 0;
    }

    if ( priest.active_items.anunds_seared_shackles )
    {
      trigger_anunds();
    }
  }

  double cost() const override
  {
    double c = priest_spell_t::cost();

    if ( priest.specialization() == PRIEST_SHADOW )
      return 0.0;

    return c;
  }

  double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.madness->ok() )
    {
      m *= 1.0 + priest.cache.mastery_value();
    }

    if ( priest.artifact.mass_hysteria.rank() )
    {
      m *= 1.0 + ( priest.artifact.mass_hysteria.percent() *
                   priest.buffs.voidform->stack() );
    }

    return m;
  }
};

struct shadow_word_void_t final : public priest_spell_t
{
  double insanity_gain;

  shadow_word_void_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "shadow_word_void", player,
                      player.talents.shadow_word_void ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    generate_insanity( insanity_gain, priest.gains.insanity_shadow_word_void );
  }

  void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    return d;
  }
};

struct silence_t final : public priest_spell_t
{
  silence_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "silence", player, player.find_class_spell( "Silence" ) )
  {
    parse_options( options_str );
    may_miss = may_crit   = false;
    ignore_false_positive = true;

    cooldown           = priest.cooldowns.silence;
    cooldown->duration = data().cooldown();
  }

  void execute() override
  {
    priest_spell_t::execute();

    // Only interrupts, does not keep target silenced. This works in most cases
    // since bosses are rarely able to be completely silenced.
    target->debuffs.casting->expire();
  }

  bool ready() override
  {
    if ( !target->debuffs.casting->check() ||
         cooldown->remains() > timespan_t::zero() )
      return false;

    return priest_spell_t::ready();
  }
};

struct smite_t final : public priest_spell_t
{
  smite_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "smite", p, p.find_class_spell( "Smite" ) )
  {
    parse_options( options_str );

    procs_courageous_primal_diamond = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.holy_evangelism->trigger();

    priest.buffs.power_overwhelming->trigger();
  }
};

/// Priest Pet Summon Base Spell
struct summon_pet_t : public priest_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, priest_t& p,
                const spell_data_t* sd = spell_data_t::nil() )
    : priest_spell_t( n, p, sd ),
      summoning_duration( timespan_t::zero() ),
      pet_name( n ),
      pet( nullptr )
  {
    harmful = false;
  }

  bool init_finished() override
  {
    pet = player->find_pet( pet_name );

    return priest_spell_t::init_finished();
  }

  void execute() override
  {
    pet->summon( summoning_duration );

    priest_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

struct summon_shadowfiend_t final : public summon_pet_t
{
  summon_shadowfiend_t( priest_t& p, const std::string& options_str )
    : summon_pet_t( "shadowfiend", p, p.find_class_spell( "Shadowfiend" ) )
  {
    parse_options( options_str );
    harmful            = false;
    summoning_duration = data().duration();
    cooldown           = p.cooldowns.shadowfiend;
    cooldown->duration = data().cooldown();
    cooldown->duration +=
        priest.sets.set( PRIEST_SHADOW, T18, B2 )->effectN( 1 ).time_value();
  }
};

struct summon_mindbender_t final : public summon_pet_t
{
  summon_mindbender_t( priest_t& p, const std::string& options_str )
    : summon_pet_t( "mindbender", p, p.find_talent_spell( "Mindbender" ) )
  {
    parse_options( options_str );
    harmful            = false;
    summoning_duration = data().duration();
    cooldown           = p.cooldowns.mindbender;
    cooldown->duration = data().cooldown();
    cooldown->duration +=
        priest.sets.set( PRIEST_SHADOW, T18, B2 )->effectN( 2 ).time_value();
  }
};

struct surrender_to_madness_t final : public priest_spell_t
{
  surrender_to_madness_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "surrender_to_madness", p,
                      p.talents.surrender_to_madness )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.surrender_to_madness->trigger();
  }
};

struct vampiric_embrace_t final : public priest_spell_t
{
  vampiric_embrace_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "vampiric_embrace", p,
                      p.find_class_spell( "Vampiric Embrace" ) )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();
    priest.buffs.vampiric_embrace->trigger();
  }

  bool ready() override
  {
    if ( priest.buffs.vampiric_embrace->check() )
      return false;

    return priest_spell_t::ready();
  }
};

struct vampiric_touch_t final : public priest_spell_t
{
  double insanity_gain;

  vampiric_touch_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "vampiric_touch", p,
                      p.find_class_spell( "Vampiric Touch" ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );
    init_mental_fortitude();
    may_crit = false;
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data

    spell_power_mod.tick *= 1.0 + p.talents.sanlayn->effectN( 1 ).percent();

    if ( p.artifact.touch_of_darkness.rank() )
    {
      base_multiplier *= 1.0 + p.artifact.touch_of_darkness.percent();
    }

    if ( !priest.active_spells.shadowy_apparitions &&
         priest.specs.shadowy_apparitions->ok() &&
         p.artifact.unleash_the_shadows.rank() )
    {
      priest.active_spells.shadowy_apparitions =
          new shadowy_apparition_spell_t( p );
    }
  }
  void init_mental_fortitude();

  void trigger_heal( action_state_t* s )
  {
    double amount_to_heal =
        s->result_amount *
        0.5;  // TODO: 50% factor not yer available through spelldata

    if ( priest.active_items.zenkaram_iridis_anadem )
    {
      amount_to_heal *= 1.0 +
                        priest.active_items.zenkaram_iridis_anadem->driver()
                            ->effectN( 1 )
                            .percent();
    }
    double actual_amount = priest.resource_gain(
        RESOURCE_HEALTH, amount_to_heal, priest.gains.vampiric_touch_health );
    double overheal = amount_to_heal - actual_amount;
    if ( priest.active_spells.mental_fortitude && overheal > 0.0 )
    {
      priest.active_spells.mental_fortitude->pre_execute_state =
          priest.active_spells.mental_fortitude->get_state( s );
      priest.active_spells.mental_fortitude->base_dd_min =
          priest.active_spells.mental_fortitude->base_dd_max = overheal;
      priest.active_spells.mental_fortitude->target          = &priest;
      priest.active_spells.mental_fortitude->execute();
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    generate_insanity( insanity_gain,
                       priest.gains.insanity_vampiric_touch_onhit );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    trigger_heal( d->state );

    if ( priest.artifact.unleash_the_shadows.rank() )
    {
      if ( priest.active_spells.shadowy_apparitions &&
           ( d->state->result_amount > 0 ) )
      {
        if ( d->state->result == RESULT_CRIT &&
             rng().roll( priest.artifact.unleash_the_shadows.percent() ) )
        {
          priest.active_spells.shadowy_apparitions->trigger();
        }
      }
    }

    if ( priest.sets.has_set_bonus( PRIEST_SHADOW, T19, B2 ) )
    {
      generate_insanity(
          priest.sets.set( PRIEST_SHADOW, T19, B2 )->effectN( 1 ).base_value(),
          priest.gains.insanity_vampiric_touch_ondamage );
    }

    if ( priest.active_items.anunds_seared_shackles )
    {
      trigger_anunds();
    }
  }

  double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.madness->ok() )
    {
      m *= 1.0 + priest.cache.mastery_value();
    }

    if ( priest.artifact.mass_hysteria.rank() )
    {
      m *= 1.0 + ( priest.artifact.mass_hysteria.percent() *
                   priest.buffs.voidform->stack() );
    }

    return m;
  }
};

struct void_bolt_t final : public priest_spell_t
{
  double insanity_gain;
  const spell_data_t* rank2;

  void_bolt_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "void_bolt", player, player.find_spell( 205448 ) ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) ),
      rank2( player.find_specialization_spell( 231688 ) )
  {
    parse_options( options_str );
    use_off_gcd                 = true;
    is_sphere_of_insanity_spell = true;
    energize_type =
        ENERGIZE_NONE;  // disable resource generation from spell data.

    if ( player.artifact.sinister_thoughts.rank() )
    {
      base_multiplier *= 1.0 + player.artifact.sinister_thoughts.percent();
    }

    cooldown->hasted = true;
  }

  void execute() override
  {
    priest_spell_t::execute();

    generate_insanity( insanity_gain, priest.gains.insanity_void_bolt );

    if ( priest.buffs.shadow_t19_4p->up() )
    {
      cooldown->reset( false );
    }
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( rank2->ok() )
    {
      if ( const priest_td_t* td = find_td( s->target ) )
      {
        if ( td->dots.shadow_word_pain->is_ticking() )
        {
          td->dots.shadow_word_pain->refresh_duration();
        }

        if ( td->dots.vampiric_touch->is_ticking() )
        {
          td->dots.vampiric_touch->refresh_duration();
        }
      }
    }
  }

  bool ready() override
  {
    if ( !( priest.buffs.voidform->check() ) )
      return false;

    return priest_spell_t::ready();
  }

  double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.madness->ok() )
    {
      m *= 1.0 + priest.cache.mastery_value();
    }

    if ( priest.buffs.anunds_last_breath->check() )
    {
      m *= 1.0 +
           ( priest.buffs.anunds_last_breath->data().effectN( 1 ).percent() *
             priest.buffs.anunds_last_breath->stack() );
      priest.buffs.anunds_last_breath->expire();
    }
    return m;
  }
};

struct void_eruption_t final : public priest_spell_t
{
  action_t* void_bolt;

  void_eruption_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "void_eruption", p, p.find_spell( 228360 ) ),
      void_bolt( nullptr )
  {
    parse_options( options_str );
    base_costs[ RESOURCE_INSANITY ] = 0.0;

    may_miss = false;
    aoe      = -1;
    range    = 0.0;
    radius   = 100.0;
    school   = SCHOOL_SHADOW;
    cooldown = priest.cooldowns.void_bolt;
  }

  void init() override
  {
    priest_spell_t::init();
    void_bolt = player->find_action( "void_bolt" );
  }

  std::vector<player_t*>& target_list() const override
  {
    std::vector<player_t*>& tl = priest_spell_t::target_list();

    priest_spell_t::available_targets( tl );

    auto it = tl.begin();

    while ( it != tl.end() )
    {
      priest_td_t& td = priest_spell_t::get_td( *it );

      if ( !td.dots.shadow_word_pain->is_ticking() &&
           !td.dots.vampiric_touch->is_ticking() )  // Neither SWP nor VT
      {
        priest.procs.void_eruption_no_dots->occur();
        it = tl.erase( it );
      }
      else if ( td.dots.shadow_word_pain->is_ticking() &&
                td.dots.vampiric_touch->is_ticking() )  // Both SWP and VT
      {
        priest.procs.void_eruption_both_dots->occur();
        it = tl.insert( it, *it );
        it += 2;
      }
      else  // SWP or VT
      {
        if ( td.dots.shadow_word_pain->is_ticking() )
        {
          priest.procs.void_eruption_only_shadow_word_pain->occur();
        }
        else
        {
          priest.procs.void_eruption_only_vampiric_touch->occur();
        }
        it++;
      }
    }

    return tl;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.voidform->trigger();
    priest.cooldowns.void_bolt->reset( true );

    if ( priest.sets.has_set_bonus( PRIEST_SHADOW, T19, B4 ) )
    {
      priest.buffs.shadow_t19_4p->trigger();
    }
    else
    {
      priest.cooldowns.void_bolt->start( void_bolt );
      priest.cooldowns.void_bolt->adjust(
          -timespan_t::from_millis( 1000 *
                                    ( 1.5 * priest.composite_spell_speed() ) ),
          true );
    }

    if ( priest.artifact.sphere_of_insanity.rank() )
    {
      priest.buffs.sphere_of_insanity->trigger();
      priest.buffs.sphere_of_insanity->current_value = 0.0;
    }

    if ( priest.active_items.mother_shahrazs_seduction &&
         priest.buffs.lingering_insanity->up() )
    {
      int mss_vf_stacks =
          floor( priest.buffs.lingering_insanity->remains().total_seconds() /
                 priest.active_items.mother_shahrazs_seduction->driver()
                     ->effectN( 1 )
                     .base_value() );

      priest.buffs.voidform->bump( mss_vf_stacks );
    }

    if ( priest.talents.void_lord->ok() &&
         priest.buffs.lingering_insanity->up() )
    {
      timespan_t time =
          priest.buffs.lingering_insanity->remains() -
          ( priest.talents.void_lord->effectN( 1 ).time_value() * 1000 );
      priest.buffs.lingering_insanity->extend_duration( player, -time );
    }
    else
    {
      priest.buffs.lingering_insanity->expire();
    }
  }

  bool ready() override
  {
    if ( !priest.buffs.voidform->check() &&
         ( priest.resources.current[ RESOURCE_INSANITY ] >=
               priest.resources.max[ RESOURCE_INSANITY ] ||
           ( priest.talents.legacy_of_the_void->ok() &&
             ( priest.resources.current[ RESOURCE_INSANITY ] >=
               priest.resources.max[ RESOURCE_INSANITY ] +
                   priest.talents.legacy_of_the_void->effectN( 2 )
                       .base_value() ) ) ) )
    {
      return priest_spell_t::ready();
    }
    else
    {
      return false;
    }
  }
};

struct void_torrent_t final : public priest_spell_t
{
  void_torrent_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "void_torrent", p, p.artifact.void_torrent )
  {
    parse_options( options_str );

    may_crit      = false;
    channeled     = true;
    use_off_gcd   = true;
    is_mind_spell = false;
    tick_zero     = true;

    dot_duration = timespan_t::from_seconds( 4.0 );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( 4.0 );
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    timespan_t t = base_tick_time;

    double h = priest.composite_spell_haste();

    t *= h;

    return t;
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );

    priest.buffs.void_torrent->expire();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.void_torrent->trigger();
  }

  bool ready() override
  {
    if ( !priest.buffs.voidform->check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

}  // NAMESPACE spells

// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================

namespace heals
{
// Power Word: Shield Spell =================================================

struct power_word_shield_t final : public priest_absorb_t
{
  bool ignore_debuff;

  power_word_shield_t( priest_t& p, const std::string& options_str )
    : priest_absorb_t( "power_word_shield", p,
                       p.find_class_spell( "Power Word: Shield" ) ),
      ignore_debuff( false )
  {
    add_option( opt_bool( "ignore_debuff", ignore_debuff ) );
    parse_options( options_str );

    spell_power_mod.direct = 4.59;  // last checked 2015/02/21
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );

    priest.buffs.borrowed_time->trigger();

    // Talent
    if ( priest.talents.body_and_soul->ok() )
      s->target->buffs.body_and_soul->trigger();
  }
};

// Power Word: Shield Spell =================================================

struct mental_fortitude_t final : public priest_absorb_t
{
  mental_fortitude_t( priest_t& p )
    : priest_absorb_t( "mental_fortitude", p, p.find_spell( 194018 ) )
  {
    background = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );
  }

  void assess_damage( dmg_e, action_state_t* s ) override
  {
    // Add stacking of buff value, and limit by players health factor.
    auto& buff            = target_specific[ s->target ];
    double stacked_amount = s->result_amount;
    if ( buff && buff->check() )
    {
      stacked_amount += buff->current_value;
    }
    double limit   = priest.resources.max[ RESOURCE_HEALTH ] * 0.08;
    stacked_amount = std::min( stacked_amount, limit );
    if ( sim->log )
    {
      sim->out_log.printf( "%s %s stacked amount: %.2f", player->name(), name(),
                           stacked_amount );
    }

    // Trigger Absorb Buff
    if ( buff == nullptr )
      buff = create_buff( s );

    if ( result_is_hit( s->result ) )
    {
      buff->trigger( 1, stacked_amount );
      if ( sim->log )
        sim->out_log.printf( "%s %s applies absorb on %s for %.0f (%.0f) (%s)",
                             player->name(), name(), s->target->name(),
                             s->result_amount, stacked_amount,
                             util::result_type_string( s->result ) );
    }

    stats->add_result( 0.0, s->result_total, ABSORB, s->result, s->block_result,
                       s->target );
  }
};

}  // NAMESPACE heals

void spells::vampiric_touch_t::init_mental_fortitude()
{
  if ( !priest.active_spells.mental_fortitude &&
       priest.artifact.mental_fortitude.rank() )
  {
    priest.active_spells.mental_fortitude =
        new heals::mental_fortitude_t( priest );
  }
}
}  // NAMESPACE actions

namespace buffs
{  // namespace buffs

/* This is a template for common code between priest buffs.
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

  template <typename Buff_Creator>
  priest_buff_t( priest_td_t& p, const Buff_Creator& params )
    : Base( params ), priest( p.priest )
  {
  }

  template <typename Buff_Creator>
  priest_buff_t( priest_t& p, const Buff_Creator& params )
    : Base( params ), priest( p )
  {
  }

  priest_td_t& get_td( player_t* t ) const
  {
    return *( priest.get_target_data( t ) );
  }

protected:
  priest_t& priest;
};

/* Custom insanity_drain_stacks buff
*/
struct insanity_drain_stacks_t final : public priest_buff_t<buff_t>
{
  // There is probably a better way to accomplish this. Overriding tick in
  // buff_t maybe? - Twintop 2016/06/05
  struct stack_increase_event_t final : public player_event_t
  {
    insanity_drain_stacks_t* ids;

    stack_increase_event_t( insanity_drain_stacks_t* s )
      : player_event_t( *s->player, timespan_t::from_seconds( 1.0 ) ), ids( s )
    {
    }

    const char* name() const override
    {
      return "insanity_drain_stack_increase";
    }

    void execute() override
    {
      auto priest = debug_cast<priest_t*>( player() );

      // If we are currently channeling Void Torrent or Dispersion, we don't
      // gain stacks.
      if ( !( priest->buffs.void_torrent->check() ||
              priest->buffs.dispersion->check() ) )
      {
        priest->buffs.insanity_drain_stacks->increment();
      }
      ids->stack_increase = make_event<stack_increase_event_t>( sim(), ids );
    }
  };

  stack_increase_event_t* stack_increase;

  insanity_drain_stacks_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "insanity_drain_stacks" )
                     .max_stack( 999 )
                     .chance( 1.0 )
                     .duration( timespan_t::zero() ) ),
      stack_increase( nullptr )
  {
  }

  bool trigger( int stacks, double value, double chance,
                timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    assert( stack_increase == nullptr );
    stack_increase = make_event<stack_increase_event_t>( *sim, this );

    return r;
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    event_t::cancel( stack_increase );

    base_t::expire_override( expiration_stacks, remaining_duration );
  }

  void reset() override
  {
    base_t::reset();

    event_t::cancel( stack_increase );
  }
};

/* Custom voidform buff
 */
struct voidform_t final : public priest_buff_t<haste_buff_t>
{
  struct insanity_loss_event_t final : public player_event_t
  {
    voidform_t* vf;

    insanity_loss_event_t( voidform_t* s )
      : player_event_t( *s->player,
                        timespan_t::from_seconds( drain_interval() ) ),
        vf( s )
    {
    }

    double drain_interval() const
    { return 0.05; }

    const char* name() const override
    {
      return "voidform_insanity_loss";
    }

    void execute() override
    {
      // Updated 2016-06-08 by Twintop:
      // ---
      // http://us.battle.net/wow/en/forum/topic/20743504316?page=2#31
      // http://us.battle.net/wow/en/forum/topic/20743504316?page=4#71
      // ---
      // Drain starts at 8 over 1 second and increases by 1.1 over 2 seconds per
      // stack of Voidform. I.E.: 8 over t=0->1, 8.55 over t=1->2, etc.
      // Drain happens continuously, like energy in reverse.
      // We make ticks happen every 0.05sec (drain_interval) to get as close to
      // contiunuous as possible without killing simulation lengths.
      // CHECK ME: Triggering Voidform in-game and not using any abilities
      // results in 10 stacks, rarely 11 stacks if you have high latency.
      // Sim seems to mostly get 9 stacks, rarely 10 stacks.
      // ---
      // 2016/06/05 update by Twintop:
      // The drain amount for Insanity is tracked separately from the Voidform
      // stacks. "Insanity Drain Stacks" always begins at 1 (Legendary Shoulders
      // can let you start with more stacks of Voidform). Using Dispersion or
      // Void Torrent cause these "Insanity Drain Stacks" to pause while being
      // channeled.
      // ---
      // 2016/11/15 update by Anshlun:
      // Updated the drain formula to match the most recent hotfix:
      // Drain = 8 + 0.55 * drain_stacks
      auto priest = debug_cast<priest_t*>( player() );

      // Base Insanity loss per second
      double base_insanity_loss =
          priest->buffs.voidform->data().effectN( 2 ).base_value() / -500.0;

      // Insanity loss per additional Insanity Drain stacks (>1) per second
      double loss_per_additional_stack =
          0.55;  // Hardcoded Patch 7.1 2016-11-16

      // Combined Insanity loss per second
      double insanity_loss_per_second =
          base_insanity_loss +
          ( priest->buffs.insanity_drain_stacks->check() - 1 ) *
              loss_per_additional_stack;

      // Adjust from Insanity Loss per second to Insanity Loss per drain
      // interval
      double insanity_loss = insanity_loss_per_second * drain_interval();

      if ( insanity_loss > priest->resources.current[ RESOURCE_INSANITY ] )
      {
        insanity_loss = priest->resources.current[ RESOURCE_INSANITY ];
      }

      priest->resource_loss( RESOURCE_INSANITY, insanity_loss,
                             priest->gains.insanity_drain );

      if ( priest->buffs.dispersion->check() )
      {
        priest->resource_gain( RESOURCE_INSANITY, insanity_loss,
                               priest->gains.insanity_dispersion );
      }

      if ( priest->buffs.void_torrent->check() )
      {
        priest->resource_gain( RESOURCE_INSANITY, insanity_loss,
                               priest->gains.insanity_void_torrent );
      }

      // If you don't have enough insanity for the drain, you drop out
      // with however much insanity you had left. HOWEVER, there is a
      // bug presently where this extra insanity doesn't count towards
      // your next Voidform. We're going to just remove this extra
      // insanity and drop down to 0 until this bug is fixed, one way
      // or another. -- 2014/04/17 Twintop
      if ( priest->resources.current[ RESOURCE_INSANITY ] == 0 )
      {
        if ( sim().debug )
        {
          sim().out_debug << "Insanity-loss event cancels voidform because "
                             "priest is out of insanity.";
        }
        vf->insanity_loss = nullptr;  // avoid double-canceling
        priest->buffs.voidform->expire();
        priest->buffs.sphere_of_insanity->expire();
        return;
      }

      vf->insanity_loss = make_event<insanity_loss_event_t>( sim(), vf );
    }
  };

  insanity_loss_event_t* insanity_loss;

  voidform_t( priest_t& p )
    : base_t( p, haste_buff_creator_t( &p, "voidform" )
                     .spell( p.find_spell( 194249 ) )
                     .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                     .add_invalidate( CACHE_HASTE )
                     .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER ) ),
      insanity_loss( nullptr )
  {
  }

  bool freeze_stacks() override
  {
    // Hotfixed 2016-09-24: Voidform stacks no longer increase while Dispersion
    // is active.
    // Note: These abilities prevent Insanity drain from increasing while
    // active, which effectively reduces the Insanity drain for the entire
    // remainder the current Voidform. This was proving to be too powerful, so
    // we're now making sure that the damage bonus and Insanity drain from
    // Voidform remain in sync.
    if ( priest.buffs.dispersion->check() )
      return true;

    return base_t::freeze_stacks();
  }

  bool trigger( int stacks, double value, double chance,
                timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    assert( insanity_loss == nullptr );
    insanity_loss = make_event<insanity_loss_event_t>( *sim, this );

    priest.buffs.insanity_drain_stacks->trigger();
    priest.buffs.the_twins_painful_touch->trigger();
    priest.buffs.shadowform->expire();

    return r;
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    if ( priest.buffs.lingering_insanity->check() )
      priest.buffs.lingering_insanity->expire();

    priest.buffs.insanity_drain_stacks->expire();

    priest.buffs.lingering_insanity->trigger( expiration_stacks );

    priest.buffs.the_twins_painful_touch->expire();

    if ( priest.buffs.shadowform_state->check() )
    {
      priest.buffs.shadowform->trigger();
    }

    if ( priest.buffs.surrender_to_madness->check() )
    {
      if ( sim->log )
      {
        sim->out_log.printf(
            "%s %s: Surrender to Madness kills you. You die. Horribly.",
            priest.name(), name() );
      }
      priest.demise();
      priest.arise();
      priest.buffs.surrender_to_madness_death->trigger();
    }

    event_t::cancel( insanity_loss );

    base_t::expire_override( expiration_stacks, remaining_duration );
  }

  void reset() override
  {
    base_t::reset();

    event_t::cancel( insanity_loss );
  }
};

/* Custom surrender_to_madness buff
*/
struct surrender_to_madness_t final : public priest_buff_t<buff_t>
{
  surrender_to_madness_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "surrender_to_madness" )
                     .spell( p.talents.surrender_to_madness ) )
  {
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    priest.buffs.voidform->expire();
  }
};

/* Custom archangel buff
 * snapshots evangelism stacks and expires it
 */
struct archangel_t final : public priest_buff_t<buff_t>
{
  archangel_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "archangel" )
                     .spell( p.specs.archangel )
                     .max_stack( 5 ) )
  {
    default_value = data().effectN( 1 ).percent();
  }

  bool trigger( int stacks, double /* value */, double chance,
                timespan_t duration ) override
  {
    stacks                 = priest.buffs.holy_evangelism->stack();
    double archangel_value = default_value * stacks;
    bool success = base_t::trigger( stacks, archangel_value, chance, duration );

    priest.buffs.holy_evangelism->expire();

    return success;
  }
};

}  // end namespace buffs

namespace items
{
void do_trinket_init( const priest_t* priest, specialization_e spec,
                      const special_effect_t*& ptr,
                      const special_effect_t& effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from
  // working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !priest->find_spell( effect.spell_id )->ok() ||
       priest->specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is
  // "enabled"
  ptr = &( effect );
}

// Archimonde Trinkets

void discipline_trinket( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_DISCIPLINE,
                   priest->active_items.naarus_discipline, effect );
}

void holy_trinket( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_HOLY, priest->active_items.complete_healing,
                   effect );
}

void shadow_trinket( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_SHADOW, priest->active_items.mental_fatigue,
                   effect );
}

// Legion Legendaries

// Shadow
void anunds_seared_shackles( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_SHADOW,
                   priest->active_items.anunds_seared_shackles, effect );
}

void mangazas_madness( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_SHADOW, priest->active_items.mangazas_madness,
                   effect );
}

void mother_shahrazs_seduction( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_SHADOW,
                   priest->active_items.mother_shahrazs_seduction, effect );
}

void the_twins_painful_touch( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_SHADOW,
                   priest->active_items.the_twins_painful_touch, effect );
}

void zenkaram_iridis_anadem( special_effect_t& effect )
{
  priest_t* priest = debug_cast<priest_t*>( effect.player );
  assert( priest );
  do_trinket_init( priest, PRIEST_SHADOW,
                   priest->active_items.zenkaram_iridis_anadem, effect );
}

void init()
{
  // Archimonde Trinkets
  unique_gear::register_special_effect( 184912, discipline_trinket );
  unique_gear::register_special_effect( 184914, holy_trinket );
  unique_gear::register_special_effect( 184915, shadow_trinket );

  // Legion Legendaries
  unique_gear::register_special_effect( 215209, anunds_seared_shackles );
  unique_gear::register_special_effect( 207701, mangazas_madness );
  unique_gear::register_special_effect( 215250, mother_shahrazs_seduction );
  unique_gear::register_special_effect( 207721, the_twins_painful_touch );
  unique_gear::register_special_effect( 224999, zenkaram_iridis_anadem );
}

}  // items

// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p )
  : actor_target_data_t( target, &p ), dots(), buffs(), priest( p )
{
  dots.holy_fire         = target->get_dot( "holy_fire", &p );
  dots.power_word_solace = target->get_dot( "power_word_solace", &p );
  dots.shadow_word_pain  = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch    = target->get_dot( "vampiric_touch", &p );

  if ( priest.active_items.mental_fatigue )
  {
    buffs.mental_fatigue =
        buff_creator_t( *this, "mental_fatigue",
                        priest.active_items.mental_fatigue->driver()
                            ->effectN( 1 )
                            .trigger() )
            .default_value(
                priest.active_items.mental_fatigue->driver()
                    ->effectN( 1 )
                    .average( priest.active_items.mental_fatigue->item ) /
                100.0 / 100.0 );
  }
  else
  {
    buffs.mental_fatigue =
        buff_creator_t( *this, "mental_fatigue" ).chance( 0 );
  }

  if ( priest.talents.mind_spike->ok() )
    buffs.mind_spike = buff_creator_t( *this, "mind_spike" )
                           .spell( p.talents.mind_spike )
                           .default_value( 0.0 )
                           .duration( timespan_t::from_seconds( 10 ) )
                           .max_stack( 10 );  // FIXME no value in Data?

  buffs.schism = buff_creator_t( *this, "schism" ).spell( p.talents.schism );

  target->callbacks_on_demise.push_back(
      [this]( player_t* ) { target_demise(); } );
}

void priest_td_t::reset()
{
}

void priest_td_t::target_demise()
{
  if ( priest.sim->debug )
  {
    priest.sim->out_debug.printf(
        "Player %s demised. Priest %s resets targetdata for him.",
        target->name(), priest.name() );
  }

  reset();
}

// ==========================================================================
// Priest Definitions
// ==========================================================================

priest_t::priest_t( sim_t* sim, const std::string& name, race_e r )
  : player_t( sim, PRIEST, name, r ),
    buffs(),
    talents(),
    specs(),
    mastery_spells(),
    cooldowns(),
    gains(),
    benefits(),
    procs(),
    rppm(),
    active_spells(),
    active_items(),
    pets(),
    options(),
    glyphs()
{
  base.distance = 27.0;  // Halo

  create_cooldowns();
  create_gains();
  create_procs();
  create_benefits();

  regen_type = REGEN_DYNAMIC;
}

/* Construct priest cooldowns
 *
 */
void priest_t::create_cooldowns()
{
  cooldowns.chakra            = get_cooldown( "chakra" );
  cooldowns.mindbender        = get_cooldown( "mindbender" );
  cooldowns.penance           = get_cooldown( "penance" );
  cooldowns.power_word_shield = get_cooldown( "power_word_shield" );
  cooldowns.shadowfiend       = get_cooldown( "shadowfiend" );
  cooldowns.silence           = get_cooldown( "silence" );
  cooldowns.mind_blast        = get_cooldown( "mind_blast" );
  cooldowns.shadow_word_death = get_cooldown( "shadow_word_death" );
  cooldowns.shadow_word_void  = get_cooldown( "shadow_word_void" );
  cooldowns.void_bolt         = get_cooldown( "void_bolt" );

  if ( specialization() == PRIEST_DISCIPLINE )
  {
    cooldowns.power_word_shield->duration = timespan_t::zero();
  }
  else
  {
    cooldowns.power_word_shield->duration = timespan_t::from_seconds( 4.0 );
  }
}

/* Construct priest gains
 *
 */
void priest_t::create_gains()
{
  gains.mindbender        = get_gain( "Mana Gained from Mindbender" );
  gains.power_word_solace = get_gain( "Mana Gained from Power Word: Solace" );
  gains.insanity_auspicious_spirits =
      get_gain( "Insanity Gained from Auspicious Spirits" );
  gains.insanity_dispersion = get_gain( "Insanity Saved by Dispersion" );
  gains.insanity_drain      = get_gain( "Insanity Drained by Voidform" );
  gains.insanity_mind_blast = get_gain( "Insanity Gained from Mind Blast" );
  gains.insanity_mind_flay  = get_gain( "Insanity Gained from Mind Flay" );
  gains.insanity_mind_sear  = get_gain( "Insanity Gained from Mind Sear" );
  gains.insanity_mind_spike = get_gain( "Insanity Gained from Mind Spike" );
  gains.insanity_mindbender = get_gain( "Insanity Gained from Mindbender" );
  gains.insanity_power_infusion =
      get_gain( "Insanity Gained from Power Infusion" );
  gains.insanity_shadow_crash = get_gain( "Insanity Gained from Shadow Crash" );
  gains.insanity_shadow_word_death =
      get_gain( "Insanity Gained from Shadow Word: Death" );
  gains.insanity_shadow_word_pain_onhit =
      get_gain( "Insanity Gained from Shadow Word: Pain Casts" );
  gains.insanity_shadow_word_void =
      get_gain( "Insanity Gained from Shadow Word: Void" );
  gains.insanity_surrender_to_madness =
      get_gain( "Insanity Gained from Surrender to Madness" );
  gains.insanity_vampiric_touch_ondamage =
      get_gain( "Insanity Gained from Vampiric Touch Damage (T19 2P)" );
  gains.insanity_vampiric_touch_onhit =
      get_gain( "Insanity Gained from Vampiric Touch Casts" );
  gains.insanity_void_bolt    = get_gain( "Insanity Gained from Void Bolt" );
  gains.insanity_void_torrent = get_gain( "Insanity Saved by Void Torrent" );
  gains.vampiric_touch_health = get_gain( "Health from Vampiric Touch Ticks" );
}

/* Construct priest procs
 *
 */
void priest_t::create_procs()
{
  procs.shadowy_apparition = get_proc( "Shadowy Apparition Procced" );
  procs.shadowy_apparition =
      get_proc( "Shadowy Apparition Insanity lost to overflow" );
  procs.shadowy_insight =
      get_proc( "Shadowy Insight Mind Blast CD Reset from Shadow Word: Pain" );
  procs.shadowy_insight_overflow =
      get_proc( "Shadowy Insight Mind Blast CD Reset lost to overflow" );
  procs.surge_of_light          = get_proc( "Surge of Light" );
  procs.surge_of_light_overflow = get_proc( "Surge of Light lost to overflow" );
  procs.t17_2pc_caster_mind_blast_reset =
      get_proc( "Tier17 2pc Mind Blast CD Reduction occurances" );
  procs.t17_2pc_caster_mind_blast_reset_overflow = get_proc(
      "Tier17 2pc Mind Blast CD Reduction occurances lost to overflow" );
  procs.t17_2pc_caster_mind_blast_reset_overflow_seconds =
      get_proc( "Tier17 2pc Mind Blast CD Reduction in seconds (total)" );
  procs.serendipity = get_proc( "Serendipity (Non-Tier 17 4pc)" );
  procs.serendipity_overflow =
      get_proc( "Serendipity lost to overflow (Non-Tier 17 4pc)" );
  procs.t17_4pc_holy = get_proc( "Tier17 4pc Serendipity" );
  procs.t17_4pc_holy_overflow =
      get_proc( "Tier17 4pc Serendipity lost to overflow" );
  procs.void_eruption_both_dots =
      get_proc( "Void Eruption casted when a target with both DoTs was up" );
  procs.void_eruption_no_dots =
      get_proc( "Void Eruption casted when a target with no DoTs was up" );
  procs.void_eruption_only_shadow_word_pain = get_proc(
      "Void Eruption casted when a target with only Shadow Word: Pain was up" );
  procs.void_eruption_only_vampiric_touch = get_proc(
      "Void Eruption casted when a target with only Vampiric Touch was up" );
  procs.void_tendril = get_proc( "Void Tendril spawned from Call to the Void" );

  procs.legendary_anunds_last_breath = get_proc(
      "Legendary - Anund's Seared Shackles - Void Bolt damage increases (2% "
      "per)" );
  procs.legendary_anunds_last_breath_overflow = get_proc(
      "Legendary - Anund's Seared Shackles - Void Bolt damage increases (2% "
      "per) lost to overflow" );
}

/* Construct priest benefits
 *
 */
void priest_t::create_benefits()
{
}

/* Define the acting role of the priest
 * If base_t::primary_role() has a valid role defined, use it,
 * otherwise select spec-based default.
 */
role_e priest_t::primary_role() const
{
  switch ( base_t::primary_role() )
  {
    case ROLE_HEAL:
      return ROLE_HEAL;
    case ROLE_DPS:
    case ROLE_SPELL:
      return ROLE_SPELL;
    default:
      if ( specialization() == PRIEST_HOLY )
        return ROLE_HEAL;
      break;
  }

  return ROLE_SPELL;
}

/**
 * @brief Convert hybrid stats
 *
 *  Converts hybrid stats that either morph based on spec or only work
 *  for certain specs into the appropriate "basic" stats
 */
stat_e priest_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_STR_INT:
    case STAT_AGI_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
      return STAT_NONE;
    case STAT_SPIRIT:
      if ( specialization() != PRIEST_SHADOW )
      {
        return s;
      }
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

expr_t* priest_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "primary_target" )
  {
    return make_fn_expr( name_str,
                         [this, a]() { return target == a->target; } );
  }

  else if ( name_str == "shadowy_apparitions_in_flight" )
  {
    return make_fn_expr( name_str, [this]() {
      if ( !active_spells.shadowy_apparitions )
        return 0.0;

      return static_cast<double>(
          active_spells.shadowy_apparitions->get_num_travel_events() );
    } );
  }

  else if ( name_str == "current_insanity_drain" )
  {
    // Current Insanity Drain for the next 1.0 sec.
    // Does not account for a new stack occurring in the middle and can be
    // anywhere from 0.0 - 0.5 off the real value.
    // Does not account for Dispersion or Void Torrent
    return make_fn_expr( name_str, [this]() {
      if ( !buffs.voidform->check() )
        return 0.0;

      return ( ( buffs.voidform->data().effectN( 2 ).base_value() / -500.0 ) +
               ( ( buffs.insanity_drain_stacks->check() - 1 ) / 2.0 ) );
    } );
  }

  return player_t::create_expression( a, name_str );
}

void priest_t::assess_damage( school_e school, dmg_e dtype, action_state_t* s )
{
  if ( buffs.shadowform->check() )
  {
    s->result_amount *= 1.0 +
                        buffs.shadowform->check() *
                            buffs.shadowform->data().effectN( 2 ).percent();
  }

  player_t::assess_damage( school, dtype, s );
}

// priest_t::composite_spell_haste ==========================================

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.power_infusion->check() )
    h /= 1.0 + buffs.power_infusion->data().effectN( 1 ).percent();

  if ( buffs.mental_instinct->check() )
    h /= 1.0 +
         buffs.mental_instinct->data().effectN( 1 ).percent() *
             buffs.mental_instinct->check();

  if ( buffs.lingering_insanity->check() )
  {
    h /= 1.0 +
         ( buffs.lingering_insanity->check() ) *
             buffs.lingering_insanity->data().effectN( 1 ).percent();
  }

  if ( buffs.voidform->check() )
  {
    h /= 1.0 +
         ( buffs.voidform->check() ) *
             buffs.voidform->data().effectN( 3 ).percent();
  }

  return h;
}

// priest_t::composite_melee_haste ==========================================

double priest_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.power_infusion->check() )
    h /= 1.0 + buffs.power_infusion->data().effectN( 1 ).percent();

  if ( buffs.mental_instinct->check() )
    h /= 1.0 +
         buffs.mental_instinct->data().effectN( 1 ).percent() *
             buffs.mental_instinct->check();

  if ( buffs.lingering_insanity->check() )
  {
    h /= 1.0 +
         ( buffs.lingering_insanity->check() - 1 ) *
             buffs.lingering_insanity->data().effectN( 1 ).percent();
  }

  if ( buffs.voidform->check() )
  {
    h /= 1.0 +
         ( buffs.voidform->check() - 1 ) *
             buffs.voidform->data().effectN( 3 ).percent();
  }

  return h;
}

// priest_t::composite_spell_speed ==========================================

double priest_t::composite_spell_speed() const
{
  double h = player_t::composite_spell_speed();

  if ( buffs.borrowed_time->check() )
    h /= 1.0 + buffs.borrowed_time->data().effectN( 1 ).percent();

  return h;
}

// priest_t::composite_melee_speed ==========================================

double priest_t::composite_melee_speed() const
{
  double h = player_t::composite_melee_speed();

  if ( buffs.borrowed_time->check() )
    h /= 1.0 + buffs.borrowed_time->data().effectN( 1 ).percent();

  return h;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  if ( buffs.shadowform->check() )
  {
    m *= 1.0 +
         buffs.shadowform->data().effectN( 1 ).percent() *
             buffs.shadowform->check();
  }
  if ( specs.voidform->ok() && buffs.voidform->check() &&
       ( dbc::is_school( SCHOOL_SHADOW, school ) ||
         dbc::is_school( SCHOOL_SHADOWFROST, school ) ) )
  {
    m *= 1.0 + buffs.voidform->data().effectN( 1 ).percent();
  }

  if ( specialization() == PRIEST_SHADOW && artifact.creeping_shadows.rank() &&
       ( dbc::is_school( SCHOOL_SHADOW, school ) ||
         dbc::is_school( SCHOOL_SHADOWFROST, school ) ) )
  {
    m *= 1.0 + artifact.creeping_shadows.percent();
  }

  if ( specialization() == PRIEST_SHADOW &&
       artifact.darkening_whispers.rank() &&
       ( dbc::is_school( SCHOOL_SHADOW, school ) ||
         dbc::is_school( SCHOOL_SHADOWFROST, school ) ) )
  {
    m *= 1.0 + artifact.darkening_whispers.percent();
  }

  if ( buffs.twist_of_fate->check() )
  {
    m *= 1.0 + buffs.twist_of_fate->current_value;
  }

  if ( buffs.reperation->check() )
  {
    m *= 1.0 +
         buffs.reperation->data().effectN( 1 ).percent() *
             buffs.reperation->check();
  }

  return m;
}

// priest_t::composite_player_heal_multiplier ===============================

double priest_t::composite_player_heal_multiplier(
    const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buffs.twist_of_fate->check() )
  {
    m *= 1.0 + buffs.twist_of_fate->current_value;
  }

  if ( specs.voidform->ok() && talents.void_lord->ok() &&
       buffs.voidform->check() )
    m *= 1.0 + talents.void_lord->effectN( 1 ).percent();

  if ( specs.grace->ok() )
    m *= 1.0 + specs.grace->effectN( 1 ).percent();

  if ( buffs.reperation->check() )
  {
    m *= 1.0 +
         buffs.reperation->data().effectN( 1 ).percent() *
             buffs.reperation->check();
  }

  return m;
}

double priest_t::composite_player_absorb_multiplier(
    const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );

  if ( specs.voidform->ok() && talents.void_lord->ok() &&
       buffs.voidform->check() )
    m *= 1.0 + talents.void_lord->effectN( 1 ).percent();

  if ( specs.grace->ok() )
    m *= 1.0 + specs.grace->effectN( 2 ).percent();

  if ( buffs.reperation->check() )
  {
    // TODO: check assumption that it increases absorb as well.
    m *= 1.0 +
         buffs.reperation->data().effectN( 1 ).percent() *
             buffs.reperation->check();
  }

  return m;
}

double priest_t::composite_player_target_multiplier( player_t* t,
                                                     school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( t, school );

  if ( const auto td = find_target_data( t ) )
  {
    if ( td->buffs.schism->check() )
    {
      m *= 1.0 + td->buffs.schism->data().effectN( 2 ).percent();
    }
  }

  return m;
}

// priest_t::matching_gear_multiplier =======================================

double priest_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// priest_t::create_action ==================================================

action_t* priest_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  // Misc
  if ( name == "angelic_feather" )
    return new angelic_feather_t( *this, options_str );
  if ( name == "archangel" )
    return new archangel_t( *this, options_str );
  if ( name == "dispersion" )
    return new dispersion_t( *this, options_str );
  if ( name == "levitate" )
    return new levitate_t( *this, options_str );
  if ( name == "pain_suppression" )
    return new pain_suppression_t( *this, options_str );
  if ( name == "power_infusion" )
    return new power_infusion_t( *this, options_str );
  if ( name == "surrender_to_madness" )
    return new surrender_to_madness_t( *this, options_str );
  if ( name == "silence" )
    return new silence_t( *this, options_str );
  if ( name == "vampiric_embrace" )
    return new vampiric_embrace_t( *this, options_str );

  // Shadow
  if ( name == "lingering_insanity" )
    return new lingering_insanity_t( *this, options_str );
  if ( name == "mind_blast" )
    return new mind_blast_t( *this, options_str );
  if ( name == "mind_flay" )
    return new mind_flay_t( *this, options_str );
  if ( name == "mind_spike" )
    return new mind_spike_t( *this, options_str );
  if ( name == "mind_sear" )
    return new mind_sear_t( *this, options_str );
  if ( name == "shadowform" )
    return new shadowform_t( *this, options_str );
  if ( name == "shadow_crash" )
    return new shadow_crash_t( *this, options_str );
  if ( name == "shadow_word_death" )
    return new shadow_word_death_t( *this, options_str );
  if ( ( name == "shadow_word_pain" ) || ( name == "purge_the_wicked" ) )
  {
    if ( talents.purge_the_wicked->ok() )
    {
      return new purge_the_wicked_t( *this, options_str );
    }
    else
    {
      return new shadow_word_pain_t( *this, options_str );
    }
  }
  if ( name == "shadow_word_void" )
    return new shadow_word_void_t( *this, options_str );
  if ( name == "vampiric_touch" )
    return new vampiric_touch_t( *this, options_str );
  if ( name == "void_bolt" )
    return new void_bolt_t( *this, options_str );
  if ( name == "void_torrent" )
    return new void_torrent_t( *this, options_str );
  if ( name == "void_eruption" || name == "voidform" )  // Backward
                                                        // compatability for
                                                        // now. -- Twintop
                                                        // 2016/06/23
    return new void_eruption_t( *this, options_str );

  if ( ( name == "shadowfiend" ) || ( name == "mindbender" ) )
  {
    if ( talents.mindbender->ok() )
      return new summon_mindbender_t( *this, options_str );
    else
      return new summon_shadowfiend_t( *this, options_str );
  }

  // Disc+Holy
  if ( name == "penance" )
    return new penance_t( *this, options_str );
  if ( name == "smite" )
    return new smite_t( *this, options_str );
  if ( name == "holy_fire" || name == "power_word_solace" )
  {
    if ( talents.power_word_solace->ok() )
      return new power_word_solace_t( *this, options_str );
    else
      return new holy_fire_t( *this, options_str );
  }
  if ( name == "halo" )
    return new halo_t( *this, options_str );
  if ( name == "divine_star" )
    return new divine_star_t( *this, options_str );
  if ( name == "schism" )
    return new schism_t( *this, options_str );

  // Heals
  if ( name == "power_word_shield" )
    return new power_word_shield_t( *this, options_str );

  return base_t::create_action( name, options_str );
}

// priest_t::create_pet =====================================================

pet_t* priest_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p && pet_name != "void_tendril" )
    return p;

  if ( pet_name == "shadowfiend" )
    return new pets::fiend::shadowfiend_pet_t( sim, *this );
  if ( pet_name == "mindbender" )
    return new pets::fiend::mindbender_pet_t( sim, *this );

  sim->errorf( "Tried to create priest pet %s.", pet_name.c_str() );

  return nullptr;
}

// priest_t::create_pets ====================================================

void priest_t::create_pets()
{
  base_t::create_pets();

  if ( find_action( "shadowfiend" ) && !talents.mindbender->ok() )
  {
    pets.shadowfiend = create_pet( "shadowfiend" );
  }

  if ( ( find_action( "mindbender" ) || find_action( "shadowfiend" ) ) &&
       talents.mindbender->ok() )
  {
    pets.mindbender = create_pet( "mindbender" );
  }

  if ( artifact.call_to_the_void.rank() )
  {
    for ( size_t i = 0; i < pets.void_tendril.size(); i++ )
    {
      pets.void_tendril[ i ] = new pets::void_tendril::void_tendril_pet_t(
          sim, *this, pets.void_tendril.front() );

      if ( i > 0 )
      {
        pets.void_tendril[ i ]->quiet = 1;
      }
    }
  }
}

// priest_t::init_base ======================================================

void priest_t::init_base_stats()
{
  base_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  if ( specialization() == PRIEST_SHADOW )
    resources.base[ RESOURCE_INSANITY ] = 100.0;

  // Discipline/Holy
  base.mana_regen_from_spirit_multiplier =
      specs.meditation_disc->ok()
          ? specs.meditation_disc->effectN( 1 ).percent()
          : specs.meditation_holy->effectN( 1 ).percent();
}

// priest_t::init_resources ===================================================

void priest_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.current[ RESOURCE_INSANITY ] = 0.0;
}

// priest_t::init_scaling ===================================================

void priest_t::init_scaling()
{
  base_t::init_scaling();

  // Atonement heals are capped at a percentage of the Priest's health,
  // so there may be scaling with stamina.
  if ( specs.atonement->ok() && primary_role() == ROLE_HEAL )
    scales_with[ STAT_STAMINA ] = true;
}

// priest_t::init_spells ====================================================

void priest_t::init_spells()
{
  base_t::init_spells();
  // Talents
  // Shared

  talents.body_and_soul   = find_talent_spell( "Body and Soul" );
  talents.angelic_feather = find_talent_spell( "Angelic Feather" );
  talents.masochism       = find_talent_spell( "Masochism" );

  talents.shining_force = find_talent_spell( "Shining Force" );
  talents.psychic_voice = find_talent_spell( "Psychic Voice" );
  talents.dominant_mind = find_talent_spell( "Dominant Mind" );

  talents.mindbender = find_talent_spell( "Mindbender" );

  talents.twist_of_fate  = find_talent_spell( "Twist of Fate" );
  talents.power_infusion = find_talent_spell( "Power Infusion" );

  talents.divine_star = find_talent_spell( "Divine Star" );
  talents.halo        = find_talent_spell( "Halo" );

  // Discipline

  talents.the_penitent = find_talent_spell( "The Penitent" );
  talents.castigation  = find_talent_spell( "Castigation" );
  talents.schism       = find_talent_spell( "Schism" );

  talents.power_word_solace = find_talent_spell( "Power Word: Solace" );
  talents.shield_discipline = find_talent_spell( "Shield Discipline" );

  talents.contrition = find_talent_spell( "Contrition" );

  talents.clarity_of_will = find_talent_spell( "Clarity of Will" );

  talents.purge_the_wicked = find_talent_spell( "Purge the Wicked" );
  talents.grace            = find_talent_spell( "Grace" );
  talents.shadow_covenant  = find_talent_spell( "Shadow Covenant" );

  // Holy
  talents.trail_of_light   = find_talent_spell( "Trail of Light" );
  talents.enduring_renewal = find_talent_spell( "Enduring Renewal" );
  talents.enlightenment    = find_talent_spell( "Enlightenment" );

  talents.path_of_light    = find_talent_spell( "Path of Light" );
  talents.desperate_prayer = find_talent_spell( "Desperate Prayer" );

  talents.light_of_the_naaru = find_talent_spell( "Light of the Naaru" );
  talents.guardian_angel     = find_talent_spell( "Guardian Angel" );
  talents.hymn_of_hope       = find_talent_spell( "Hymn of Hope" );

  talents.surge_of_light = find_talent_spell( "Surge of Light" );
  talents.binding_heal   = find_talent_spell( "Binding Heal" );
  talents.piety          = find_talent_spell( "Piety" );

  talents.divinity = find_talent_spell( "Divinity" );

  talents.apotheosis        = find_talent_spell( "Apotheosis" );
  talents.benediction       = find_talent_spell( "Benediction" );
  talents.circle_of_healing = find_talent_spell( "Circle of Healing" );

  // Shadow
  talents.fortress_of_the_mind = find_talent_spell( "Fortress of the Mind" );
  talents.shadow_word_void     = find_talent_spell( "Shadow Word: Void" );

  talents.mania = find_talent_spell( "Mania" );

  talents.mind_bomb = find_talent_spell( "Mind Bomb" );

  talents.void_lord       = find_talent_spell( "Void Lord" );
  talents.reaper_of_souls = find_talent_spell( "Reaper of Souls" );
  talents.void_ray        = find_talent_spell( "Void Ray" );

  talents.sanlayn            = find_talent_spell( "San'layn" );
  talents.auspicious_spirits = find_talent_spell( "Auspicious Spirits" );
  talents.shadowy_insight    = find_talent_spell( "Shadowy Insight" );

  talents.shadow_crash = find_talent_spell( "Shadow Crash" );

  talents.legacy_of_the_void   = find_talent_spell( "Legacy of the Void" );
  talents.mind_spike           = find_talent_spell( "Mind Spike" );
  talents.surrender_to_madness = find_talent_spell( "Surrender to Madness" );

  // Artifacts
  // Shadow - Xal'atath, Blade of the Black Empire

  artifact.call_to_the_void     = find_artifact_spell( "Call to the Void" );
  artifact.creeping_shadows     = find_artifact_spell( "Creeping Shadows" );
  artifact.darkening_whispers   = find_artifact_spell( "Darkening Whispers" );
  artifact.deaths_embrace       = find_artifact_spell( "Death's Embrace" );
  artifact.from_the_shadows     = find_artifact_spell( "From the Shadows" );
  artifact.mass_hysteria        = find_artifact_spell( "Mass Hysteria" );
  artifact.mental_fortitude     = find_artifact_spell( "Mental Fortitude" );
  artifact.mind_shattering      = find_artifact_spell( "Mind Shattering" );
  artifact.sinister_thoughts    = find_artifact_spell( "Sinister Thoughts" );
  artifact.sphere_of_insanity   = find_artifact_spell( "Sphere of Insanity" );
  artifact.thoughts_of_insanity = find_artifact_spell( "Thoughts of Insanity" );
  artifact.thrive_in_the_shadows =
      find_artifact_spell( "Thrive in the Shadows" );
  artifact.to_the_pain         = find_artifact_spell( "To the Pain" );
  artifact.touch_of_darkness   = find_artifact_spell( "Touch of Darkness" );
  artifact.unleash_the_shadows = find_artifact_spell( "Unleash the Shadows" );
  artifact.void_corruption     = find_artifact_spell( "Void Corruption" );
  artifact.void_siphon         = find_artifact_spell( "Void Siphon" );
  artifact.void_torrent        = find_artifact_spell( "Void Torrent" );

  // General Spells

  // Discipline
  specs.atonement       = find_specialization_spell( "Atonement" );
  specs.archangel       = find_specialization_spell( "Archangel" );
  specs.borrowed_time   = find_specialization_spell( "Borrowed Time" );
  specs.divine_aegis    = find_specialization_spell( "Divine Aegis" );
  specs.evangelism      = find_specialization_spell( "Evangelism" );
  specs.grace           = find_specialization_spell( "Grace" );
  specs.meditation_disc = find_specialization_spell(
      "Meditation", "meditation_disc", PRIEST_DISCIPLINE );
  specs.mysticism     = find_specialization_spell( "Mysticism" );
  specs.spirit_shell  = find_specialization_spell( "Spirit Shell" );
  specs.enlightenment = find_specialization_spell( "Enlightenment" );

  // Holy
  specs.meditation_holy =
      find_specialization_spell( "Meditation", "meditation_holy", PRIEST_HOLY );
  specs.serendipity       = find_specialization_spell( "Serendipity" );
  specs.rapid_renewal     = find_specialization_spell( "Rapid Renewal" );
  specs.divine_providence = find_specialization_spell( "Divine Providence" );
  specs.focused_will      = find_specialization_spell( "Focused Will" );

  // Shadow
  specs.voidform      = find_specialization_spell( "Voidform" );
  specs.void_eruption = find_specialization_spell( "Void Eruption" );
  specs.shadowy_apparitions =
      find_specialization_spell( "Shadowy Apparitions" );

  // Mastery Spells
  mastery_spells.absolution    = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.madness       = find_mastery_spell( PRIEST_SHADOW );

  // Range Based on Talents
  if ( talents.divine_star->ok() )
    base.distance = 24.0;
  else if ( talents.halo->ok() )
    base.distance = 27.0;
  else
    base.distance = 27.0;
}

// priest_t::init_buffs =====================================================

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Talents
  buffs.power_infusion = haste_buff_creator_t( this, "power_infusion" )
                             .spell( talents.power_infusion )
                             .add_invalidate( CACHE_SPELL_HASTE )
                             .add_invalidate( CACHE_HASTE );

  buffs.twist_of_fate =
      buff_creator_t( this, "twist_of_fate" )
          .spell( talents.twist_of_fate )
          .duration( talents.twist_of_fate->effectN( 1 ).trigger()->duration() )
          .default_value( talents.twist_of_fate->effectN( 1 )
                              .trigger()
                              ->effectN( 2 )
                              .percent() )
          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buffs.shadowform = buff_creator_t( this, "shadowform" )
                         .spell( find_class_spell( "Shadowform" ) )
                         .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.shadowform_state =
      buff_creator_t( this, "shadowform_state" ).chance( 1.0 ).quiet( true );

  buffs.shadowy_insight =
      buff_creator_t( this, "shadowy_insight" )
          .spell( talents.shadowy_insight )
          .max_stack(
              1 );  // Spell Data says 2, really is 1 -- 2016/04/17 Twintop

  buffs.void_ray = buff_creator_t( this, "void_ray" )
                       .spell( talents.void_ray->effectN( 1 ).trigger() );

  buffs.void_torrent =
      buff_creator_t( this, "void_torrent" ).spell( artifact.void_torrent );

  buffs.surrender_to_madness = new buffs::surrender_to_madness_t( *this );

  buffs.surrender_to_madness_death =
      buff_creator_t( this, "surrender_to_madness_death" )
          .spell( talents.surrender_to_madness )
          .chance( 1.0 )
          .duration( timespan_t::zero() )
          .default_value( 0.0 );

  buffs.sphere_of_insanity = buff_creator_t( this, "sphere_of_insanity" )
                                 .spell( find_spell( 194182 ) )
                                 .chance( 1.0 )
                                 .duration( timespan_t::zero() )
                                 .default_value( 0.0 );

  // Discipline
  buffs.archangel = new buffs::archangel_t( *this );

  buffs.borrowed_time =
      buff_creator_t( this, "borrowed_time" )
          .spell( find_spell( 59889 ) )
          .chance( specs.borrowed_time->ok() )
          .default_value( find_spell( 59889 )->effectN( 1 ).percent() )
          .add_invalidate( CACHE_HASTE );

  buffs.holy_evangelism = buff_creator_t( this, "holy_evangelism" )
                              .spell( find_spell( 81661 ) )
                              .chance( specs.evangelism->ok() )
                              .activated( false );

  // Shadow

  buffs.voidform = new buffs::voidform_t( *this );

  buffs.lingering_insanity = haste_buff_creator_t( this, "lingering_insanity" )
                                 .spell( find_spell( 197937 ) )
                                 .add_invalidate( CACHE_HASTE );

  buffs.insanity_drain_stacks = new buffs::insanity_drain_stacks_t( *this );

  buffs.vampiric_embrace =
      buff_creator_t( this, "vampiric_embrace",
                      find_class_spell( "Vampiric Embrace" ) )
          .duration( find_class_spell( "Vampiric Embrace" )->duration() );

  buffs.mind_sear_on_hit_reset =
      buff_creator_t( this, "mind_sear_on_hit_reset" )
          .max_stack( 2 )
          .duration( timespan_t::from_seconds( 5.0 ) );

  buffs.dispersion =
      buff_creator_t( this, "dispersion", find_class_spell( "Dispersion" ) )
          .duration( find_class_spell( "Dispersion" )->duration() );

  // Set Bonuses
  buffs.mental_instinct =
      buff_creator_t( this, "mental_instinct" )
          .spell( sets.set( PRIEST_SHADOW, T17, B4 )->effectN( 1 ).trigger() )
          .chance( sets.has_set_bonus( PRIEST_SHADOW, T17, B4 ) )
          .add_invalidate( CACHE_SPELL_HASTE )
          .add_invalidate( CACHE_HASTE );

  buffs.reperation =
      buff_creator_t( this, "reperation" )
          .spell( find_spell( 186478 ) )
          .chance( sets.has_set_bonus( PRIEST_DISCIPLINE, T18, B2 ) )
          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buffs.premonition =
      buff_creator_t( this, "premonition" )
          .spell( find_spell( 188779 ) )
          .chance( sets.has_set_bonus( PRIEST_SHADOW, T18, B4 ) );

  buffs.power_overwhelming =
      stat_buff_creator_t( this, "power_overwhelming" )
          .spell(
              sets.set( specialization(), T19OH, B8 )->effectN( 1 ).trigger() )
          .trigger_spell( sets.set( specialization(), T19OH, B8 ) );

  buffs.shadow_t19_4p =
      buff_creator_t( this, "shadow_t19_4p" )
          .spell( find_spell( 211654 ) )
          .chance( 1.0 )
          .duration( timespan_t::from_seconds(
              6.0 ) );  // TODO Update with spelldata once available

  buffs.anunds_last_breath = buff_creator_t( this, "anunds_last_breath" )
                                 .spell( find_spell( 215210 ) );
  //.chance( 1.0 )
  //.duration(timespan_t::from_seconds(60.0)) // Probably 1 minute like the rest
  // of our temp buffs.
  //.max_stack(100); // Data isn't pulling this in.

  buffs.the_twins_painful_touch =
      buff_creator_t( this, "the_twins_painful_touch" )
          .spell( find_spell( 207721 ) )
          .chance( active_items.the_twins_painful_touch ? 1.0 : 0.0 );
  //.duration(timespan_t::from_seconds(10.0));
}

// priest_t::init_rng ==================================================

void priest_t::init_rng()
{
  rppm.call_to_the_void =
      get_rppm( "call_to_the_void", artifact.call_to_the_void );

  player_t::init_rng();
}

// ALL Spec Pre-Combat Action Priority List

void priest_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  if ( sim->allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";

    if ( true_level > 100 )
    {
      flask_action += "flask_of_the_whispered_pact";
    }
    else if ( true_level > 90 )
    {
      switch ( specialization() )
      {
        case PRIEST_DISCIPLINE:
          flask_action += "greater_draenic_intellect_flask";
          break;
        case PRIEST_HOLY:
          flask_action += "greater_draenic_intellect_flask";
          break;
        case PRIEST_SHADOW:
        default:
          if ( talents.auspicious_spirits->ok() )
            flask_action += "greater_draenic_intellect_flask";
          else
            flask_action += "greater_draenic_intellect_flask";
          break;
      }
    }
    else if ( true_level > 85 )
      flask_action += "warm_sun";
    else
      flask_action += "draconic_mind";

    precombat->add_action( flask_action );
  }

  // Food
  if ( sim->allow_food && level() >= 80 )
  {
    std::string food_action = "food,type=";

    if ( level() >
         100 )  // Break this in to specs at some point -- Twintop 2016/08/06
    {
      food_action += "azshari_salad";  // Haste
    }
    else if ( level() > 90 )
    {
      switch ( specialization() )
      {
        case PRIEST_DISCIPLINE:
          if ( primary_role() != ROLE_HEAL )
            food_action += "salty_squid_roll";
          else
            food_action += "pickled_eel";
          break;
        case PRIEST_HOLY:
          food_action += "salty_squid_roll";
          break;
        case PRIEST_SHADOW:
        default:
          food_action += "buttered_sturgeon";
          break;
      }
    }
    else if ( level() > 85 )
      food_action += "mogu_fish_stew";
    else
      food_action += "seafood_magnifique_feast";

    precombat->add_action( food_action );
  }

  if ( true_level > 100 )
    precombat->add_action( "augmentation,type=defiled" );

  // Snapshot stats
  precombat->add_action( "snapshot_stats",
                         "Snapshot raid buffed stats before combat begins and "
                         "pre-potting is done." );

  if ( sim->allow_potions && true_level >= 80 )
  {
    if ( true_level > 100 )
      precombat->add_action( "potion,name=deadly_grace" );
    else if ( true_level > 90 )
      precombat->add_action( "potion,name=draenic_intellect" );
    else if ( true_level > 85 )
      precombat->add_action( "potion,name=jade_serpent" );
    else
      precombat->add_action( "potion,name=volcanic" );
  }

  // Precast
  switch ( specialization() )
  {
    case PRIEST_DISCIPLINE:
      break;
    case PRIEST_HOLY:
      break;
    case PRIEST_SHADOW:
    default:
      precombat->add_action( this, "Shadowform", "if=!buff.shadowform.up" );
      precombat->add_action( 
        "variable,op=set,name=s2mbeltcheck,value=1,if=cooldown.mind_blast."
        "charges>=2" );
      precombat->add_action(
        "variable,op=set,name=s2mbeltcheck,value=0,if=cooldown.mind_blast."
        "charges<=1" );
      precombat->add_action( "mind_blast" );
      break;
  }
}

// priest_t::has_t18_class_trinket
// ==============================================

bool priest_t::has_t18_class_trinket() const
{
  if ( specialization() == PRIEST_DISCIPLINE )
  {
    return active_items.naarus_discipline != nullptr;
  }
  else if ( specialization() == PRIEST_HOLY )
  {
    return active_items.complete_healing != nullptr;
  }
  else if ( specialization() == PRIEST_SHADOW )
  {
    return active_items.mental_fatigue != nullptr;
  }

  return false;
}

// NO Spec Combat Action Priority List

void priest_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // DEFAULT
  if ( sim->allow_potions )
    def->add_action( "mana_potion,if=mana.pct<=75" );

  if ( find_class_spell( "Shadowfiend" )->ok() )
  {
    def->add_action( this, "Shadowfiend" );
  }
  if ( race == RACE_TROLL )
    def->add_action( "berserking" );
  if ( race == RACE_BLOOD_ELF )
    def->add_action( "arcane_torrent,if=mana.pct<=90" );
  def->add_action( this, "Shadow Word: Pain",
                   ",if=remains<tick_time|!ticking" );
  def->add_action( this, "Smite" );
}

// Shadow Combat Action Priority List

void priest_t::apl_shadow()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* main         = get_action_priority_list( "main" );
  action_priority_list_t* vf           = get_action_priority_list( "vf" );
  action_priority_list_t* s2m          = get_action_priority_list( "s2m" );

  // On-Use Items
  for ( const std::string& item_action : get_item_actions() )
  {
    default_list->add_action( item_action );
  }

  // Professions
  for ( const std::string& profession_action : get_profession_actions() )
  {
    default_list->add_action( profession_action );
  }

  // Potions
  if ( sim->allow_potions && true_level >= 80 )
  {
    if ( true_level > 100 )
      default_list->add_action(
          "potion,name=deadly_grace,if=buff.bloodlust.react|target.time_to_die<"
          "=40|(buff.voidform.stack>80&buff.power_infusion.up)" );
    else if ( true_level > 90 )
      default_list->add_action(
          "potion,name=draenic_intellect,if=buff.bloodlust.react|target.time_"
          "to_die<=40" );
    else if ( true_level > 85 )
      default_list->add_action(
          "potion,name=jade_serpent,if=buff.bloodlust.react|target.time_to_"
          "die<"
          "=40" );
    else
      default_list->add_action(
          "potion,name=volcanic,if=buff.bloodlust.react|target.time_to_die<="
          "40" );
  }

  // Choose which APL to use based on talents and fight conditions.

  default_list->add_action(
      "variable,op=set,name=actors_fight_time_mod,value=0" );
  default_list->add_action(
      "variable,op=set,name=actors_fight_time_mod,value=-((-(450)+(time+target."
      "time_to_die))%10),if=time+target.time_to_die>450&time+target.time_to_"
      "die<600" );
  default_list->add_action(
      "variable,op=set,name=actors_fight_time_mod,value=((450-(time+target."
      "time_to_die))%5),if=time+target.time_to_die<=450" );
  default_list->add_action(
      "variable,op=set,name=s2mcheck,value=0.8*(130+7*variable.s2mbeltcheck+"
      "((raw_haste_pct*25)*(2+(1*talent.reaper_of_souls.enabled)+(2*artifact."
      "mass_hysteria.rank)-(1*talent.sanlayn.enabled))))-(variable.actors_fight"
      "_time_mod*nonexecute_actors_pct)" );
  default_list->add_action( "variable,op=min,name=s2mcheck,value=180" );
  default_list->add_action(
      "call_action_list,name=s2m,if=buff.voidform.up&buff.surrender_to_madness."
      "up" );
  default_list->add_action( "call_action_list,name=vf,if=buff.voidform.up" );
  default_list->add_action( "call_action_list,name=main" );

  // Main APL
  main->add_action(
      "surrender_to_madness,if=talent.surrender_to_madness.enabled&target.time_"
      "to_die<=variable.s2mcheck" );
  main->add_action(
      "mindbender,if=talent.mindbender.enabled&!talent.surrender_to_madness."
      "enabled" );
  main->add_action(
      "mindbender,if=talent.mindbender.enabled&talent.surrender_to_madness."
      "enabled&target.time_to_die>variable.s2mcheck+60" );
  main->add_action(
      "shadow_word_pain,if=dot.shadow_word_pain.remains<(3+(4%3))*gcd" );
  main->add_action(
      "vampiric_touch,if=dot.vampiric_touch.remains<(4+(4%3))*gcd" );
  main->add_action(
      "void_eruption,if=insanity>=85|(talent.auspicious_spirits.enabled&"
      "insanity>=(80-shadowy_apparitions_in_flight*4))" );
  main->add_action( "shadow_crash,if=talent.shadow_crash.enabled" );
  main->add_action(
      "mindbender,if=talent.mindbender.enabled&set_bonus.tier18_2pc" );
  main->add_action(
      "shadow_word_pain,if=!ticking&talent.legacy_of_the_void.enabled&insanity>"
      "=70,cycle_targets=1" );
  main->add_action(
      "vampiric_touch,if=!ticking&talent.legacy_of_the_void.enabled&insanity>="
      "70,cycle_targets=1" );
  main->add_action(
      "shadow_word_death,if=!talent.reaper_of_souls.enabled&cooldown.shadow_"
      "word_death.charges=2&insanity<=90" );
  main->add_action(
      "shadow_word_death,if=talent.reaper_of_souls.enabled&cooldown.shadow_"
      "word_death.charges=2&insanity<=70" );
  main->add_action(
      "mind_blast,if=talent.legacy_of_the_void.enabled&(insanity<=81|(insanity<"
      "=75.2&talent.fortress_of_the_mind.enabled))" );
  main->add_action(
      "mind_blast,if=!talent.legacy_of_the_void.enabled|(insanity<=96|("
      "insanity<=95.2&talent.fortress_of_the_mind.enabled))" );
  main->add_action(
      "shadow_word_pain,if=!ticking&target.time_to_die>10&(active_enemies<5&("
      "talent.auspicious_spirits.enabled|talent.shadowy_insight.enabled)),"
      "cycle_targets=1" );
  main->add_action(
      "vampiric_touch,if=!ticking&target.time_to_die>10&(active_enemies<4|"
      "talent.sanlayn.enabled|(talent.auspicious_spirits.enabled&artifact."
      "unleash_the_shadows.rank)),cycle_targets=1" );
  main->add_action(
      "shadow_word_pain,if=!ticking&target.time_to_die>10&(active_enemies<5&"
      "artifact.sphere_of_insanity.rank),cycle_targets=1" );
  main->add_action(
      "shadow_word_void,if=(insanity<=70&talent.legacy_of_the_void.enabled)|("
      "insanity<=85&!talent.legacy_of_the_void.enabled)" );
  main->add_action(
      "mind_flay,line_cd=10,if=!talent.mind_spike.enabled&"
      "active_enemies>=2&active_enemies<4,interrupt=1,chain=1" );
  main->add_action( "mind_sear,if=active_enemies>=2,interrupt=1,chain=1" );
  main->add_action(
      "mind_flay,if=!talent.mind_spike.enabled,interrupt=1,chain=1" );
  main->add_action( "mind_spike,if=talent.mind_spike.enabled" );
  main->add_action( "shadow_word_pain" );

  // Voidform APL
  vf->add_action(
      "surrender_to_madness,if=talent.surrender_to_madness.enabled&insanity>="
      "25&(cooldown.void_bolt.up|cooldown.void_torrent.up|cooldown.shadow_word_"
      "death.up|buff.shadowy_insight.up)&target.time_to_die<=variable.s2mcheck-"
      "(buff.insanity_drain_stacks.stack)" );
  vf->add_action( "shadow_crash,if=talent.shadow_crash.enabled" );
  vf->add_action(
      "void_torrent,if=dot.shadow_word_pain.remains>5.5&dot.vampiric_"
      "touch.remains>5.5&talent.surrender_to_madness.enabled&target.time_to_"
      "die>"
      "variable.s2mcheck-(buff.insanity_drain_stacks.stack)+60" );
  vf->add_action( "void_torrent,if=!talent.surrender_to_madness.enabled" );
  vf->add_action(
      "mindbender,if=talent.mindbender.enabled&!talent.surrender_to_madness."
      "enabled" );
  vf->add_action(
      "mindbender,if=talent.mindbender.enabled&talent.surrender_to_madness."
      "enabled&target.time_to_die>variable.s2mcheck-(buff.insanity_drain_"
      "stacks.stack)+30" );
  vf->add_action(
      "power_infusion,if=buff.voidform.stack>=10&buff.insanity_drain_stacks."
      "stack<=30&!talent.surrender_to_madness.enabled" );
  vf->add_action(
      "power_infusion,if=buff.voidform.stack>=10&talent.surrender_to_madness."
      "enabled&target.time_to_die>variable.s2mcheck-(buff.insanity_drain_"
      "stacks.stack)+41" );
  vf->add_action(
      "berserking,if=buff.voidform.stack>=10&buff.insanity_drain_stacks.stack<="
      "20&!talent.surrender_to_madness.enabled" );
  vf->add_action(
      "berserking,if=buff.voidform.stack>=10&talent.surrender_to_madness."
      "enabled&target.time_to_die>variable.s2mcheck-(buff.insanity_drain_"
      "stacks.stack)+60" );
  vf->add_action(
      "void_bolt,if=dot.shadow_word_pain.remains<3.5*gcd&dot.vampiric_touch."
      "remains<3.5*gcd&target.time_to_die>10,cycle_targets=1" );
  vf->add_action(
      "void_bolt,if=dot.shadow_word_pain.remains<3.5*gcd&(talent.auspicious_"
      "spirits.enabled|talent.shadowy_insight.enabled)&target.time_to_die>10,"
      "cycle_targets=1" );
  vf->add_action(
      "void_bolt,if=dot.vampiric_touch.remains<3.5*gcd&(talent.sanlayn.enabled|"
      "(talent.auspicious_spirits.enabled&artifact.unleash_the_shadows.rank))&"
      "target.time_to_die>10,cycle_targets=1" );
  vf->add_action(
      "void_bolt,if=dot.shadow_word_pain.remains<3.5*gcd&artifact.sphere_of_"
      "insanity.rank&target.time_to_die>10,cycle_targets=1" );
  vf->add_action( "void_bolt" );

  vf->add_action(
      "shadow_word_death,if=!talent.reaper_of_souls.enabled&current_insanity_"
      "drain*gcd.max>insanity&(insanity-(current_insanity_drain*gcd.max)+10)<"
      "100" );
  vf->add_action(
      "shadow_word_death,if=talent.reaper_of_souls.enabled&current_insanity_"
      "drain*gcd.max>insanity&(insanity-(current_insanity_drain*gcd.max)+30)<"
      "100" );
  vf->add_action(
      "wait,sec=action.void_bolt.usable_in,if=action.void_bolt.usable_in<gcd."
      "max*0.28" );
  vf->add_action( "mind_blast" );
  vf->add_action(
      "wait,sec=action.mind_blast.usable_in,if=action.mind_blast.usable_in<gcd."
      "max*0.28" );
  vf->add_action( "shadow_word_death,if=cooldown.shadow_word_death.charges=2" );
  vf->add_action(
      "shadowfiend,if=!talent.mindbender.enabled,if=buff.voidform.stack>15" );
  vf->add_action(
      "shadow_word_void,if=(insanity-(current_insanity_drain*gcd.max)+25)<"
      "100" );
  vf->add_action(
      "shadow_word_pain,if=!ticking&(active_enemies<5|talent.auspicious_"
      "spirits.enabled|talent.shadowy_insight.enabled|artifact.sphere_of_"
      "insanity.rank)" );
  vf->add_action(
      "vampiric_touch,if=!ticking&(active_enemies<4|talent.sanlayn.enabled|("
      "talent.auspicious_spirits.enabled&artifact.unleash_the_shadows.rank))" );
  vf->add_action(
      "shadow_word_pain,if=!ticking&target.time_to_die>10&(active_enemies<5&("
      "talent.auspicious_spirits.enabled|talent.shadowy_insight.enabled)),"
      "cycle_targets=1" );
  vf->add_action(
      "vampiric_touch,if=!ticking&target.time_to_die>10&(active_enemies<4|"
      "talent.sanlayn.enabled|(talent.auspicious_spirits.enabled&artifact."
      "unleash_the_shadows.rank)),cycle_targets=1" );
  vf->add_action(
      "shadow_word_pain,if=!ticking&target.time_to_die>10&(active_enemies<5&"
      "artifact.sphere_of_insanity.rank),cycle_targets=1" );
  vf->add_action(
      "wait,sec=action.void_bolt.usable_in,if=action.void_bolt.usable|action."
      "void_bolt.usable_in<gcd.max*0.8" );
  vf->add_action(
      "mind_flay,line_cd=10,if=!talent.mind_spike.enabled&"
      "active_enemies>=2&active_enemies<4,chain=1,interrupt_immediate=1,"
      "interrupt_if=action.void_bolt.usable" );
  vf->add_action(
      "mind_sear,if=active_enemies>=2,interrupt_immediate=1,"
      "interrupt_if=action.void_bolt.usable" );
  vf->add_action(
      "mind_flay,if=!talent.mind_spike.enabled,chain=1,interrupt_immediate=1,"
      "interrupt_if=action.void_bolt.usable" );
  vf->add_action( "mind_spike,if=talent.mind_spike.enabled" );
  vf->add_action( "shadow_word_pain" );

  // Surrender to Madness APL
  s2m->add_action( "shadow_crash,if=talent.shadow_crash.enabled" );
  s2m->add_action( "mindbender,if=talent.mindbender.enabled" );
  s2m->add_action(
      "void_torrent,if=dot.shadow_word_pain.remains>5.5&dot.vampiric_"
      "touch.remains>5.5" );
  s2m->add_action( "berserking,if=buff.voidform.stack>=80" );
  s2m->add_action( "dispersion,if=dot.shadow_word_pain.remains>7.5&dot.vampiric_"
                   "touch.remains>7.5&buff.voidform.stack<10");
 s2m->add_action(
      "shadow_word_death,if=!talent.reaper_of_souls.enabled&current_insanity_"
      "drain*gcd.max>insanity&(insanity-(current_insanity_drain*gcd.max)+15)<"
      "100&!buff.power_infusion.up&buff.insanity_drain_stacks.stack<=60&"
      "cooldown.shadow_word_death.charges=2" );
  s2m->add_action(
      "shadow_word_death,if=talent.reaper_of_souls.enabled&current_insanity_"
      "drain*gcd.max>insanity&(insanity-(current_insanity_drain*gcd.max)+75)<"
      "100&!buff.power_infusion.up&buff.insanity_drain_stacks.stack<=60&"
      "cooldown.shadow_word_death.charges=2" );

  s2m->add_action(
      "void_bolt,if=dot.shadow_word_pain.remains<3.5*gcd&dot.vampiric_touch."
      "remains<3.5*gcd&target.time_to_die>10,cycle_targets=1" );
  s2m->add_action(
      "void_bolt,if=dot.shadow_word_pain.remains<3.5*gcd&(talent.auspicious_"
      "spirits.enabled|talent.shadowy_insight.enabled)&target.time_to_die>10,"
      "cycle_targets=1" );
  s2m->add_action(
      "void_bolt,if=dot.vampiric_touch.remains<3.5*gcd&(talent.sanlayn.enabled|"
      "(talent.auspicious_spirits.enabled&artifact.unleash_the_shadows.rank))&"
      "target.time_to_die>10,cycle_targets=1" );
  s2m->add_action(
      "void_bolt,if=dot.shadow_word_pain.remains<3.5*gcd&artifact.sphere_of_"
      "insanity.rank&target.time_to_die>10,cycle_targets=1" );
  s2m->add_action( "void_bolt" );

  s2m->add_action(
      "shadow_word_death,if=!talent.reaper_of_souls.enabled&current_insanity_"
      "drain*gcd.max>insanity&(insanity-(current_insanity_drain*gcd.max)+15)<"
      "100" );
  s2m->add_action(
      "shadow_word_death,if=talent.reaper_of_souls.enabled&current_insanity_"
      "drain*gcd.max>insanity&(insanity-(current_insanity_drain*gcd.max)+75)<"
      "100" );
  s2m->add_action( 
      "power_infusion,if=cooldown.shadow_word_death.charges=0&cooldown.shadow_"
      "word_death.remains>3*gcd.max" );
  s2m->add_action(
      "wait,sec=action.void_bolt.usable_in,if=action.void_bolt.usable_in<gcd."
      "max*0.28" );
  s2m->add_action(
      "dispersion,if=current_insanity_drain*gcd.max>insanity&!buff.power_"
      "infusion.up" );
  s2m->add_action( "mind_blast" );
  s2m->add_action(
      "wait,sec=action.mind_blast.usable_in,if=action.mind_blast.usable_in<gcd."
      "max*0.28" );
  s2m->add_action(
      "shadow_word_death,if=cooldown.shadow_word_death.charges=2" );
  s2m->add_action(
      "shadowfiend,if=!talent.mindbender.enabled,if=buff.voidform.stack>15" );
  s2m->add_action(
      "shadow_word_void,if=(insanity-(current_insanity_drain*gcd.max)+75)<"
      "100" );
  s2m->add_action(
      "shadow_word_pain,if=!ticking&(active_enemies<5|talent.auspicious_"
      "spirits.enabled|talent.shadowy_insight.enabled|artifact.sphere_of_"
      "insanity.rank)" );
  s2m->add_action(
      "vampiric_touch,if=!ticking&(active_enemies<4|talent.sanlayn.enabled|("
      "talent.auspicious_spirits.enabled&artifact.unleash_the_shadows.rank))" );
  s2m->add_action(
      "shadow_word_pain,if=!ticking&target.time_to_die>10&(active_enemies<5&("
      "talent.auspicious_spirits.enabled|talent.shadowy_insight.enabled)),"
      "cycle_targets=1" );
  s2m->add_action(
      "vampiric_touch,if=!ticking&target.time_to_die>10&(active_enemies<4|"
      "talent.sanlayn.enabled|(talent.auspicious_spirits.enabled&artifact."
      "unleash_the_shadows.rank)),cycle_targets=1" );
  s2m->add_action(
      "shadow_word_pain,if=!ticking&target.time_to_die>10&(active_enemies<5&"
      "artifact.sphere_of_insanity.rank),cycle_targets=1" );
  s2m->add_action(
      "mind_flay,line_cd=10,if=!talent.mind_spike.enabled&active_enemies>=2&"
      "active_enemies<4,chain=1,interrupt_immediate=1,interrupt_if=((current_"
      "insanity_drain*gcd.max>insanity&(insanity-(current_insanity_drain*gcd."
      "max)+75)<100&cooldown.shadow_word_death.charges>=1)|action.void_bolt."
      "usable)&ticks>=2" );
  s2m->add_action( 
      "mind_sear,if=active_enemies>=2,interrupt=1,interrupt_immediate=1,"
      "interrupt_if=((current_insanity_drain*gcd.max>insanity&(insanity-(current_"
      "insanity_drain*gcd.max)+75)<100&cooldown.shadow_word_death.charges>=1)|"
      "action.void_bolt.usable)&ticks>=2" );
  s2m->add_action(
      "mind_flay,if=!talent.mind_spike.enabled,chain=1,interrupt_immediate=1,"
      "interrupt_if=((current_insanity_drain*gcd.max>insanity&(insanity-(current_"
      "insanity_drain*gcd.max)+75)<100&cooldown.shadow_word_death.charges>=1)|"
      "action.void_bolt.usable)&ticks>=2" );
  s2m->add_action( "mind_spike,if=talent.mind_spike.enabled" );
}

// Discipline Heal Combat Action Priority List

void priest_t::apl_disc_heal()
{
  apl_default();
}

// Discipline Damage Combat Action Priority List

void priest_t::apl_disc_dmg()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // On-Use Items
  for ( const std::string& item_action : get_item_actions() )
  {
    def->add_action( item_action );
  }

  // Professions
  for ( const std::string& profession_action : get_profession_actions() )
  {
    def->add_action( profession_action );
  }

  // Potions
  if ( sim->allow_potions && true_level >= 80 )
  {
    if ( true_level > 100 )
      def->add_action(
          "potion,name=,if=buff.bloodlust.react|target.time_to_die<=40" );
    else if ( true_level > 90 )
      def->add_action(
          "potion,name=draenic_intellect,if=buff.bloodlust.react|target.time_"
          "to_die<=40" );
    else if ( true_level > 85 )
      def->add_action(
          "potion,name=jade_serpent,if=buff.bloodlust.react|target.time_to_"
          "die<"
          "=40" );
    else
      def->add_action(
          "potion,name=volcanic,if=buff.bloodlust.react|target.time_to_die<="
          "40" );
  }

  if ( race == RACE_BLOOD_ELF )
    def->add_action( "arcane_torrent,if=mana.pct<=95" );

  if ( find_class_spell( "Shadowfiend" )->ok() )
  {
    def->add_action( "mindbender,if=talent.mindbender.enabled" );
    def->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  }

  if ( race != RACE_BLOOD_ELF )
  {
    for ( const std::string& racial_action : get_racial_actions() )
    {
      def->add_action( racial_action );
    }
  }

  def->add_talent( this, "Power Infusion", "if=talent.power_infusion.enabled" );

  def->add_talent( this, "Purge the Wicked", "if=!ticking" );
  def->add_action( this, "Shadow Word: Pain",
                   "if=!ticking&!talent.purge_the_wicked.enabled" );

  def->add_talent( this, "Schism" );
  def->add_action( this, "Penance" );

  //  def->add_talent( this, "Power Word: Solace",
  //                   "if=talent.power_word_solace.enabled" );

  def->add_action( this, "Smite",
                   "if=glyph.smite.enabled&(dot.power_word_solace.remains+dot."
                   "holy_fire.remains)>cast_time" );
  def->add_talent( this, "Purge the Wicked", "if=remains<(duration*0.3)" );
  def->add_action(
      this, "Shadow Word: Pain",
      "if=remains<(duration*0.3)&!talent.purge_the_wicked.enabled" );
  def->add_talent( this, "Divine Star", "if=mana.pct>80" );
  def->add_action( this, "Smite" );
  def->add_action( this, "Shadow Word: Pain" );
}

// Holy Heal Combat Action Priority List

void priest_t::apl_holy_heal()
{
  apl_default();
}

// Holy Damage Combat Action Priority List

void priest_t::apl_holy_dmg()
{
  apl_default();
}

/* Always returns non-null targetdata pointer
 */
priest_td_t* priest_t::get_target_data( player_t* target ) const
{
  priest_td_t*& td = _target_data[ target ];
  if ( !td )
  {
    td = new priest_td_t( target, const_cast<priest_t&>( *this ) );
  }
  return td;
}

/* Returns targetdata if found, nullptr otherwise
 */
priest_td_t* priest_t::find_target_data( player_t* target ) const
{
  return _target_data[ target ];
}

// priest_t::init_actions ===================================================

void priest_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat();  // PRE-COMBAT

  switch ( specialization() )
  {
    case PRIEST_SHADOW:
      apl_shadow();  // SHADOW
      break;
    case PRIEST_DISCIPLINE:
      apl_disc_dmg();  // DISCIPLINE
      break;
    //    case PRIEST_HOLY:
    //      if ( primary_role() != ROLE_HEAL )
    //        apl_holy_dmg();  // HOLY DAMAGE
    //      else
    //        apl_holy_heal();  // HOLY HEAL
    //      break;
    default:
      apl_default();  // DEFAULT
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

// priest_t::combat_begin ===================================================

void priest_t::combat_begin()
{
  if ( !sim->fixed_time )
  {
    if ( options.priest_fixed_time && talents.surrender_to_madness->ok() )
    {
      // Check if there are any players in the sim other than shadow priests
      // with Surrender to Madness
      bool found_non_stm_players =
          ( sim->player_list.end() !=
            range::find_if( sim->player_list, [this]( const player_t* p ) {
              if ( p->role == ROLE_NONE )
              {
                return false;
              }
              else if ( p->specialization() != PRIEST_SHADOW )
              {
                return true;
              }
              else
              {
                const priest_t* priest = debug_cast<const priest_t*>( p );
                if ( !priest->talents.surrender_to_madness->ok() )
                {
                  return true;
                }
              }
              return false;
            } ) );

      if ( !found_non_stm_players )
      {
        // We have a simulation only with STM shadows priests, so change it to
        // fixed time simulation.
        sim->fixed_time = true;
        sim->errorf(
            "Due to Shadow Priest deaths during Surrender to Madness, fixed "
            "time has been enabled in this simulation." );
        sim->errorf(
            "This allows sims with only Shadow Priests to give useful "
            "feedback, as otherwise the sim would continue" );
        sim->errorf(
            "on for 300+ seconds "
            "and reduce the target's health in the next iteration by "
            "significantly more than it should." );
        sim->errorf(
            "To disable this option, add priest_fixed_time=0 to your sim." );
      }
    }
  }

  player_t::combat_begin();
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  base_t::reset();

  // Reset Target Data
  for ( player_t* target : sim->target_list )
  {
    if ( auto td = find_target_data( target ) )
    {
      td->reset();
    }
  }
}

/* Copy stats from the trigger spell to the atonement spell
 * to get proper HPR and HPET reports.
 */

void priest_t::fixup_atonement_stats( const std::string& trigger_spell_name,
                                      const std::string& atonement_spell_name )
{
  if ( stats_t* trigger = find_stats( trigger_spell_name ) )
  {
    if ( stats_t* atonement = get_stats( atonement_spell_name ) )
    {
      atonement->resource_gain.merge( trigger->resource_gain );
      atonement->total_execute_time = trigger->total_execute_time;
      atonement->total_tick_time    = trigger->total_tick_time;
      atonement->num_ticks          = trigger->num_ticks;
    }
  }
}

/* Fixup Atonement Stats HPE, HPET and HPR
 */

void priest_t::pre_analyze_hook()
{
  base_t::pre_analyze_hook();

  if ( specs.atonement->ok() )
  {
    fixup_atonement_stats( "smite", "atonement_smite" );
    fixup_atonement_stats( "holy_fire", "atonement_holy_fire" );
    fixup_atonement_stats( "penance", "atonement_penance" );
  }

  if ( specialization() == PRIEST_DISCIPLINE ||
       specialization() == PRIEST_HOLY )
    fixup_atonement_stats( "power_word_solace", "atonement_power_word_solace" );
}

// priest_t::target_mitigation ==============================================

void priest_t::target_mitigation( school_e school, dmg_e dt, action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( buffs.dispersion->check() )
  {
    s->result_amount *=
        1.0 + ( buffs.dispersion->data().effectN( 1 ).percent() );
  }
}

// priest_t::create_options =================================================

void priest_t::create_options()
{
  base_t::create_options();

  add_option( opt_deprecated( "double_dot", "action_list=double_dot" ) );
  add_option( opt_bool( "autounshift", options.autoUnshift ) );
  add_option( opt_bool( "priest_fixed_time", options.priest_fixed_time ) );
}

// priest_t::create_profile =================================================

std::string priest_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  if ( type == SAVE_ALL )
  {
    if ( !options.autoUnshift )
      profile_str +=
          "autounshift=" + util::to_string( options.autoUnshift ) + "\n";

    if ( !options.priest_fixed_time )
      profile_str += "priest_fixed_time=" +
                     util::to_string( options.priest_fixed_time ) + "\n";
  }

  return profile_str;
}

// priest_t::copy_from ======================================================

void priest_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  priest_t* source_p = debug_cast<priest_t*>( source );

  options = source_p->options;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
struct priest_report_t final : public player_report_extension_t
{
public:
  priest_report_t( priest_t& player ) : p( player )
  {
  }

  void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }

private:
  priest_t& p;
};

// PRIEST MODULE INTERFACE ==================================================

struct priest_module_t final : public module_t
{
  priest_module_t() : module_t( PRIEST )
  {
  }

  player_t* create_player( sim_t* sim, const std::string& name,
                           race_e r = RACE_NONE ) const override
  {
    auto p = new priest_t( sim, name, r );
    p->report_extension =
        std::unique_ptr<player_report_extension_t>( new priest_report_t( *p ) );
    return p;
  }
  bool valid() const override
  {
    return true;
  }
  void init( player_t* p ) const override
  {
    p->buffs.guardian_spirit = buff_creator_t(
        p, "guardian_spirit",
        p->find_spell( 47788 ) );  // Let the ability handle the CD
    p->buffs.pain_supression = buff_creator_t(
        p, "pain_supression",
        p->find_spell( 33206 ) );  // Let the ability handle the CD
    p->buffs.naarus_discipline =
        buff_creator_t( p, "naarus_discipline", p->find_spell( 185103 ) );
  }
  void static_init() const override
  {
    items::init();
  }
  void register_hotfixes() const override
  {
    /*
    hotfix::register_effect( "Priest", "2016-09-26", "Mind Sear damage increased
    by 80% and Insanity generation increased by 50%.", 326288 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.80 )
      .verification_value( 0.30 );

    hotfix::register_effect( "Priest", "2016-09-26", "Mind Flay damage increased
    by 20%.", 7301 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.20 )
      .verification_value( 0.50 );

    hotfix::register_effect( "Priest", "2016-09-26", "Mind Spike damage
    increased by 28%.", 66820 )
      .field( "sp_coefficient" )
      .operation( hotfix::HOTFIX_MUL )
      .modifier( 1.28 )
      .verification_value( 0.35 );

    hotfix::register_spell( "Priest", "2016-09-26", "Void Ray maximum stacks
    reduced to 4.", 205372 )
      .field( "max_stack" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 4.0 )
      .verification_value( 5.0 );
      */
  }

  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

}  // UNNAMED NAMESPACE

const module_t* module_t::priest()
{
  static priest_module_t m;
  return &m;
}
