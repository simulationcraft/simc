// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace  // UNNAMED NAMESPACE
{
/* Forward declarations
 */
struct priest_t;
namespace actions
{
namespace spells
{
struct shadowy_apparition_spell_t;
}
}

/**
 * Priest target data
 * Contains target specific things
 */
struct priest_td_t : public actor_target_data_t
{
public:
  struct dots_t
  {
    dot_t* devouring_plague_tick;
    dot_t* shadow_word_pain;
    dot_t* vampiric_touch;
    dot_t* void_entropy;
    dot_t* holy_fire;
    dot_t* power_word_solace;
    dot_t* renew;
  } dots;

  struct buffs_t
  {
    absorb_buff_t* divine_aegis;
    absorb_buff_t* spirit_shell;
    buff_t* holy_word_serenity;
    buff_t* mental_fatigue;
  } buffs;

  bool glyph_of_mind_harvest_consumed;

  priest_t& priest;

  priest_td_t( player_t* target, priest_t& p );
  void reset();
  void target_demise();
};

/* Priest class definition
 *
 * Derived from player_t. Contains everything that defines the priest class.
 */
struct priest_t : public player_t
{
public:
  typedef player_t base_t;

  // Buffs
  struct
  {
    // Talents
    buff_t* glyph_of_levitate;
    buff_t* power_infusion;
    buff_t* twist_of_fate;
    buff_t* surge_of_light;

    // Discipline
    buff_t* archangel;
    buff_t* borrowed_time;
    buff_t* holy_evangelism;
    buff_t* inner_focus;
    buff_t* spirit_shell;
    buff_t* saving_grace_penalty;

    // Holy
    buff_t* chakra_chastise;
    buff_t* chakra_sanctuary;
    buff_t* chakra_serenity;
    buff_t* divine_insight;
    buff_t* serendipity;

    buff_t* focused_will;

    // Shadow
    buff_t* insanity;
    buff_t* shadowy_insight;
    buff_t* shadow_word_death_reset_cooldown;
    buff_t* mind_sear_on_hit_reset;
    buff_t* glyph_of_mind_flay;
    buff_t* glyph_of_mind_spike;
    buff_t* shadowform;
    buff_t* vampiric_embrace;
    buff_t* surge_of_darkness;
    buff_t* dispersion;

    // Set Bonus
    buff_t* empowered_shadows;  // t16 4pc caster
    buff_t* mental_instinct;    // t17 4pc caster
    buff_t* absolution;         // t16 4pc heal holy word
    buff_t* resolute_spirit;    // t16 4pc heal spirit shell
    buff_t* clear_thoughts;     // t17 4pc disc
    buff_t* shadow_power;       // WoD Shadow 2pc PvP
    buff_t* reperation;         // T18 Disc 2p
    buff_t* premonition;        // T18 Shadow 4pc
  } buffs;

  // Talents
  struct
  {
    const spell_data_t* desperate_prayer;
    const spell_data_t* spectral_guise;
    const spell_data_t* angelic_bulwark;

    const spell_data_t* body_and_soul;
    const spell_data_t* angelic_feather;
    const spell_data_t* phantasm;

    const spell_data_t* surge_of_light;
    const spell_data_t* surge_of_darkness;
    const spell_data_t* mindbender;
    const spell_data_t* power_word_solace;
    const spell_data_t* insanity;

    const spell_data_t* void_tendrils;
    const spell_data_t* psychic_scream;
    const spell_data_t* dominate_mind;

    const spell_data_t* twist_of_fate;
    const spell_data_t* power_infusion;
    const spell_data_t* divine_insight;
    const spell_data_t* spirit_shell;
    const spell_data_t* shadowy_insight;

    const spell_data_t* cascade;
    const spell_data_t* divine_star;
    const spell_data_t* halo;

    const spell_data_t* void_entropy;
    const spell_data_t* clarity_of_power;
    const spell_data_t* clarity_of_will;
    const spell_data_t* clarity_of_purpose;
    const spell_data_t* auspicious_spirits;
    const spell_data_t* saving_grace;
    const spell_data_t* words_of_mending;
  } talents;

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
    const spell_data_t* strength_of_soul;
    const spell_data_t* enlightenment;

    // Holy
    const spell_data_t* meditation_holy;
    const spell_data_t* rapid_renewal;
    const spell_data_t* serendipity;
    const spell_data_t* divine_providence;

    const spell_data_t* focused_will;

    // Shadow
    const spell_data_t* shadowform;
    const spell_data_t* shadowy_apparitions;
    const spell_data_t* shadow_orbs;
    const spell_data_t* mastermind;
  } specs;

  // Mastery Spells
  struct
  {
    const spell_data_t* shield_discipline;
    const spell_data_t* echo_of_light;
    const spell_data_t* mental_anguish;
  } mastery_spells;

  // Perk Spells
  struct
  {
    // General
    const spell_data_t* enhanced_power_word_shield;

    // Healing
    const spell_data_t* enhanced_focused_will;
    const spell_data_t* enhanced_leap_of_faith;

    // Discipline

    // Holy
    const spell_data_t* enhanced_chakras;
    const spell_data_t* enhanced_renew;

    // Shadow
    const spell_data_t* enhanced_mind_flay;
    const spell_data_t* enhanced_shadow_orbs;
    const spell_data_t* enhanced_shadow_word_death;
  } perks;

  // Cooldowns
  struct
  {
    cooldown_t* angelic_feather;
    cooldown_t* chakra;
    cooldown_t* mindbender;
    cooldown_t* mind_blast;
    cooldown_t* penance;
    cooldown_t* power_word_shield;
    cooldown_t* shadowfiend;
    cooldown_t* silence;
  } cooldowns;

  // Gains
  struct
  {
    gain_t* auspicious_spirits;
    gain_t* devouring_plague_health;
    gain_t* shadowy_insight_shadow_word_pain;
    gain_t* shadowy_insight_mind_spike;
    gain_t* mindbender;
    gain_t* power_word_solace;
    gain_t* shadow_orb_mind_blast;
    gain_t* shadow_orb_shadow_word_death;
    gain_t* shadow_orb_auspicious_spirits;
    gain_t* shadow_orb_mind_harvest;
    gain_t* surge_of_darkness_devouring_plague;
    gain_t* surge_of_darkness_vampiric_touch;
  } gains;

  // Benefits
  struct
  {
    benefit_t* smites_with_glyph_increase;
  } benefits;

  // Procs
  struct
  {
    proc_t* divine_insight;
    proc_t* divine_insight_overflow;
    proc_t* shadowy_insight;
    proc_t* shadowy_insight_from_mind_spike;
    proc_t* shadowy_insight_from_shadow_word_pain;
    proc_t* shadowy_insight_overflow;
    proc_t* mind_spike_dot_removal;
    proc_t* mind_spike_dot_removal_devouring_plague;
    proc_t* mind_spike_dot_removal_devouring_plague_ticks;
    proc_t* mind_spike_dot_removal_shadow_word_pain;
    proc_t* mind_spike_dot_removal_shadow_word_pain_ticks;
    proc_t* mind_spike_dot_removal_vampiric_touch;
    proc_t* mind_spike_dot_removal_vampiric_touch_ticks;
    proc_t* mind_spike_dot_removal_void_entropy;
    proc_t* mind_spike_dot_removal_void_entropy_ticks;
    proc_t* serendipity;
    proc_t* serendipity_overflow;
    proc_t* shadowy_apparition;
    proc_t* surge_of_darkness;
    proc_t* surge_of_darkness_from_devouring_plague;
    proc_t* surge_of_darkness_from_vampiric_touch;
    proc_t* surge_of_darkness_overflow;
    proc_t* surge_of_light;
    proc_t* surge_of_light_overflow;
    proc_t* t15_2pc_caster;
    proc_t* t15_4pc_caster;
    proc_t* t15_2pc_caster_shadow_word_pain;
    proc_t* t15_2pc_caster_vampiric_touch;
    proc_t* t17_2pc_caster_mind_blast_reset;
    proc_t* t17_2pc_caster_mind_blast_reset_overflow;
    proc_t* t17_2pc_caster_mind_blast_reset_overflow_seconds;
    proc_t* t17_4pc_holy;
    proc_t* t17_4pc_holy_overflow;
    proc_t* void_entropy_extensions;

    luxurious_sample_data_t* sd_mind_spike_dot_removal_devouring_plague_ticks;
  } procs;

  // Special
  struct
  {
    const spell_data_t* surge_of_darkness;
    action_t* echo_of_light;
    actions::spells::shadowy_apparition_spell_t* shadowy_apparitions;
  } active_spells;

  struct
  {
    const special_effect_t* naarus_discipline;
    const special_effect_t* complete_healing;
    const special_effect_t* mental_fatigue;
  } active_items;

  // Pets
  struct
  {
    pet_t* shadowfiend;
    pet_t* mindbender;
    pet_t* lightwell;
  } pets;

  // Options
  struct priest_options_t
  {
    int initial_shadow_orbs;
    std::string atonement_target_str;
    bool autoUnshift;  // Shift automatically out of stance/form
    priest_options_t() : initial_shadow_orbs( 0 ), autoUnshift( true )
    {
    }
  } options;

  // Glyphs
  struct
  {
    // All Specs
    const spell_data_t* dispel_magic;
    const spell_data_t* fade;
    const spell_data_t* fear_ward;
    const spell_data_t* leap_of_faith;
    const spell_data_t* levitate;
    const spell_data_t* mass_dispel;
    const spell_data_t* power_word_shield;
    const spell_data_t* prayer_of_mending;
    const spell_data_t* psychic_scream;
    const spell_data_t* reflective_shield;
    const spell_data_t* restored_faith;
    const spell_data_t* scourge_imprisonment;
    const spell_data_t* weakened_soul;

    // Healing Specs
    const spell_data_t* holy_fire;
    const spell_data_t* inquisitor;
    const spell_data_t* purify;
    const spell_data_t* shadow_magic;
    const spell_data_t* smite;

    // Discipline
    const spell_data_t* borrowed_time;
    const spell_data_t* penance;

    // Holy
    const spell_data_t* binding_heal;
    const spell_data_t* circle_of_healing;
    const spell_data_t* deep_wells;
    const spell_data_t* guardian_spirit;
    const spell_data_t* lightwell;
    const spell_data_t* redeemer;
    const spell_data_t* renew;
    const spell_data_t* spirit_of_redemption;

    // Shadow
    const spell_data_t* delayed_coalescence;
    const spell_data_t* dispersion;
    const spell_data_t* focused_mending;
    const spell_data_t* free_action;
    const spell_data_t* mind_blast;
    const spell_data_t* mind_flay;
    const spell_data_t* mind_harvest;
    const spell_data_t* mind_spike;
    const spell_data_t* miraculous_dispelling;
    const spell_data_t* psychic_horror;
    const spell_data_t* shadow_word_death;
    const spell_data_t* silence;
    const spell_data_t* vampiric_embrace;
  } glyphs;

  priest_t( sim_t* sim, const std::string& name, race_e r );

  // player_t overrides
  void init_base_stats() override;
  void init_spells() override;
  void create_buffs() override;
  void init_scaling() override;
  void init_resources( bool force ) override;
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
  double composite_armor() const override;
  double composite_melee_haste() const override;
  double composite_melee_speed() const override;
  double composite_spell_haste() const override;
  double composite_spell_speed() const override;
  double composite_spell_power( school_e school ) const override;
  double composite_spell_power_multiplier() const override;
  double composite_spell_crit() const override;
  double composite_melee_crit() const override;
  double composite_multistrike() const override;
  double composite_player_multistrike_damage_multiplier() const override;
  double composite_player_multistrike_healing_multiplier() const override;
  double spirit() const;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_absorb_multiplier(
      const action_state_t* s ) const override;
  double composite_player_heal_multiplier(
      const action_state_t* s ) const override;
  double temporary_movement_modifier() const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  void invalidate_cache( cache_e ) override;
  void target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void pre_analyze_hook() override;
  void init_action_list() override;
  priest_td_t* get_target_data( player_t* target ) const override;
  expr_t* create_expression( action_t* a,
                             const std::string& name_str ) override;
  void assess_damage_imminent( school_e, dmg_e, action_state_t* ) override;
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
  priest_td_t* find_target_data( player_t* target ) const;

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

  struct _stat_list_t
  {
    int level;
    std::array<double, ATTRIBUTE_MAX> stats;
  };

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    owner_coeff.ap_from_sp = 1.0;
    owner_coeff.sp_from_sp = 1.0;

    // Base Stats, same for all pets. Depends on level
    static const _stat_list_t pet_base_stats[] = {
        //   none, str, agi, sta, int, spi
        {85, {{0, 453, 883, 353, 159, 225}}},
    };

    // Loop from end to beginning to get the data for the highest available
    // level equal or lower than the player level
    int i = as<int>( sizeof_array( pet_base_stats ) );
    while ( --i > 0 )
    {
      if ( pet_base_stats[ i ].level <= level() )
        break;
    }
    if ( i >= 0 )
      base.stats.attribute = pet_base_stats[ i ].stats;
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
        gains.fiend = o().gains.mindbender;
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

struct shadowfiend_pet_t : public base_fiend_pet_t
{
  shadowfiend_pet_t( sim_t* sim, priest_t& owner,
                     const std::string& name = "shadowfiend" )
    : base_fiend_pet_t( sim, owner, PET_SHADOWFIEND, name )
  {
    direct_power_mod = 0.75;

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

struct mindbender_pet_t : public base_fiend_pet_t
{
  const spell_data_t* mindbender_spell;

  mindbender_pet_t( sim_t* sim, priest_t& owner,
                    const std::string& name = "mindbender" )
    : base_fiend_pet_t( sim, owner, PET_MINDBENDER, name ),
      mindbender_spell( owner.find_talent_spell( "Mindbender" ) )
  {
    direct_power_mod = 0.75;

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

// ==========================================================================
// Pet Lightwell
// ==========================================================================

struct lightwell_pet_t : public priest_pet_t
{
public:
  int charges;

  lightwell_pet_t( sim_t* sim, priest_t& p )
    : priest_pet_t( sim, p, "lightwell", PET_NONE, true ), charges( 0 )
  {
    role = ROLE_HEAL;

    action_priority_list_t* precombat = get_action_priority_list( "precombat" );
    // Snapshot stats
    precombat->add_action(
        "snapshot_stats",
        "Snapshot raid buffed stats before combat begins and "
        "pre-potting is done." );

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "lightwell_renew" );
    // def -> add_action( "wait,sec=cooldown.lightwell_renew.remains" );

    owner_coeff.sp_from_sp = 1.0;
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override;

  void summon( timespan_t duration ) override
  {
    charges = 15 + o().glyphs.deep_wells->effectN( 1 ).base_value();

    priest_pet_t::summon( duration );
  }
};

namespace actions
{  // namespace for pet actions

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

struct shadowcrawl_t : public priest_pet_spell_t
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
          p().buffs.shadowcrawl->up() *
              p().buffs.shadowcrawl->data().effectN( 2 ).percent();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    priest_pet_melee_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      double mana_reg_pct = p().mana_return_percent();
      if ( mana_reg_pct > 0.0 )
      {
        p().o().resource_gain(
            RESOURCE_MANA,
            p().o().resources.max[ RESOURCE_MANA ] * p().mana_return_percent(),
            p().gains.fiend );
      }
    }
  }
};

struct lightwell_renew_t : public heal_t
{
  lightwell_renew_t( lightwell_pet_t& p )
    : heal_t( "lightwell_renew", &p, p.find_spell( 126154 ) )
  {
    may_crit      = false;
    tick_may_crit = true;
  }

  lightwell_pet_t& p()
  {
    return static_cast<lightwell_pet_t&>( *player );
  }
  const lightwell_pet_t& p() const
  {
    return static_cast<lightwell_pet_t&>( *player );
  }

  void execute() override
  {
    p().charges--;

    target = find_lowest_player();

    heal_t::execute();
  }

  void last_tick( dot_t* d ) override
  {
    heal_t::last_tick( d );

    // Need to kill the actor with an event instead of performing the demise
    // directly inside the
    // last_tick().
    if ( p().charges <= 0 )
      new ( *sim ) player_demise_event_t( p() );
  }

  bool ready() override
  {
    if ( p().charges <= 0 )
      return false;

    return heal_t::ready();
  }
};

}  // end namespace actions ( for pets )

// ==========================================================================
// Pet Shadowfiend/Mindbender Base
// ==========================================================================

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

action_t* lightwell_pet_t::create_action( const std::string& name,
                                          const std::string& options_str )
{
  if ( name == "lightwell_renew" )
    return new actions::lightwell_renew_t( *this );

  return priest_pet_t::create_action( name, options_str );
}

}  // END pets NAMESPACE

namespace buffs
{
struct weakened_soul_t : public buff_t
{
  weakened_soul_t( player_t* p )
    : buff_t(
          buff_creator_t( p, "weakened_soul" ).spell( p->find_spell( 6788 ) ) )
  {
  }

  /* Use this function to trigger weakened souls, and NOT buff_t::trigger
   * It automatically deduces the duration with which weakened souls should be
   * applied,
   * depending on the triggering priest
   */
  bool trigger_weakened_souls( priest_t& p )
  {
    timespan_t duration;

    duration = buff_duration +
               p.perks.enhanced_power_word_shield->effectN( 1 ).time_value();

    return buff_t::trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );
  }
};
}  // namespace buffs

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

    can_cancel_shadowform  = p.options.autoUnshift;
    castable_in_shadowform = true;
  }

  double shadow_orbs_to_consume() const
  {
    return std::min( 3.0, priest.resources.current[ RESOURCE_SHADOW_ORB ] );
  }

  priest_td_t& get_td( player_t* t ) const
  {
    return *( priest.get_target_data( t ) );
  }

  bool trigger_shadowy_insight()
  {
    int stack = priest.buffs.shadowy_insight->check();
    if ( priest.buffs.shadowy_insight->trigger() )
    {
      priest.cooldowns.mind_blast->reset( true );

      if ( priest.buffs.shadowy_insight->check() == stack )
        priest.procs.shadowy_insight_overflow->occur();
      else
        priest.procs.shadowy_insight->occur();
      return true;
    }
    return false;
  }

  bool trigger_divine_insight()
  {
    int stack = priest.buffs.divine_insight->check();
    if ( priest.buffs.divine_insight->trigger() )
    {
      if ( priest.buffs.divine_insight->check() == stack )
        priest.procs.divine_insight_overflow->occur();
      else
        priest.procs.divine_insight->occur();
      return true;
    }
    return false;
  }

  bool trigger_surge_of_light()
  {
    int stack = priest.buffs.surge_of_light->check();
    if ( priest.buffs.surge_of_light->trigger() )
    {
      if ( priest.buffs.surge_of_light->check() == stack )
        priest.procs.surge_of_light_overflow->occur();
      else
        priest.procs.surge_of_light->occur();
      return true;
    }
    return false;
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    cancel_shadowform();

    ab::schedule_execute( state );
  }

  bool ready() override
  {
    if ( !ab::ready() )
      return false;

    if ( !check_shadowform() )
      return false;

    return true;
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
  bool castable_in_shadowform;
  bool can_cancel_shadowform;

  /* keep reference to the priest. We are sure this will always resolve
   * to the same player as the action_t::player; pointer, and is always valid
   * because it owns the action
   */
  priest_t& priest;

  /// typedef for priest_action_t<action_base_t>
  typedef priest_action_t base_t;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  typedef Base ab;

  bool check_shadowform() const
  {
    return ( castable_in_shadowform || can_cancel_shadowform ||
             ( !priest.buffs.shadowform->check() ) );
  }

  void cancel_shadowform()
  {
    if ( !castable_in_shadowform )
    {
      priest.buffs.shadowform->expire();
    }
  }
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
    may_crit        = true;
    tick_may_crit   = false;
    may_multistrike = 1;
    may_miss        = false;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( priest.mastery_spells.shield_discipline->ok() )
      am *= 1.0 + priest.cache.mastery_value();

    return am;
  }
};

// ==========================================================================
// Priest Heal
// ==========================================================================

struct priest_heal_t : public priest_action_t<heal_t>
{
  struct divine_aegis_t : public priest_absorb_t
  {
    // Multistrike test results 14/12/05 by philoptik:
    // triggers from hit -> critting multistrike
    // does NOT trigger from crit -> normal multistrike
    // contrary to general tooltip interpretation.

    divine_aegis_t( const std::string& n, priest_t& p )
      : priest_absorb_t( n + "_divine_aegis", p, p.find_spell( 47753 ) )
    {
      check_spell( p.specs.divine_aegis );
      proc                   = true;
      background             = true;
      may_multistrike        = 0;
      may_crit               = false;
      spell_power_mod.direct = 0.0;
    }

    void init() override
    {
      action_t::init();
      snapshot_flags |= STATE_MUL_DA;
    }

    void impact( action_state_t* s ) override
    {
      absorb_buff_t& buff = *get_td( s->target ).buffs.divine_aegis;
      // Divine Aegis caps absorbs at 40% of target's health
      double old_amount = buff.value();

      // when healing a tank that's below 0 life in the sim, Divine Aegis causes
      // an exception because it tries to
      // clamp s -. result_amount between 0 and a negative number. This is a
      // workaround that treats a tank with
      // negative life as being at maximum health for the purposes of Divine
      // Aegis.
      double limiting_factor = 0.6;  // WoD 14/12/05
      double upper_limit =
          s->target->resources.current[ RESOURCE_HEALTH ] * limiting_factor -
          old_amount;
      if ( upper_limit <= 0 )
        upper_limit =
            s->target->resources.max[ RESOURCE_HEALTH ] * limiting_factor -
            old_amount;

      double new_amount = clamp( s->result_amount, 0.0, upper_limit );
      buff.trigger( 1, old_amount + new_amount );
      stats->add_result( 0.0, new_amount, ABSORB, s->result, s->block_result,
                         s->target );
      buff.absorb_source->add_result( 0.0, new_amount, ABSORB, s->result,
                                      s->block_result, s->target );
    }

    void trigger( const action_state_t* s, double crit_amount )
    {
      base_dd_min = base_dd_max =
          crit_amount * priest.specs.divine_aegis->effectN( 1 ).percent();
      target = s->target;
      if ( sim->debug )
      {
        sim->out_debug.printf(
            "%s %s triggers Divine Aegis with base_amount=%.2f.",
            player->name(), name(), base_dd_min );
      }
      execute();
    }
  };

  // FIXME:
  // * Supposedly does not scale with Archangel.
  // * There should be no min/max range on shell sizes.
  // * Verify that PoH scales the same as single-target.
  // * Verify the 30% "DA factor" did not change with the 50% DA change.
  struct spirit_shell_absorb_t : public priest_absorb_t
  {
    double trigger_crit_multiplier;

    spirit_shell_absorb_t( priest_heal_t& trigger )
      : priest_absorb_t( trigger.name_str + "_shell", trigger.priest,
                         trigger.priest.specs.spirit_shell ),
        trigger_crit_multiplier( 0.0 )
    {
      background = true;
      proc       = true;
      may_crit   = false;
      snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;
    }

    double action_multiplier() const override  // override
    {
      double am;

      am = absorb_t::action_multiplier();

      return am *                               // ( am ) *
             ( 1 + trigger_crit_multiplier ) *  // ( 1 + crit ) *
             ( 1 +
               trigger_crit_multiplier *
                   0.3 );  // ( 1 + crit * 30% "DA factor" )
    }

    void impact( action_state_t* s ) override
    {
      // Spirit Shell caps absorbs at 60% of target's health
      buff_t& spirit_shell = *get_td( s->target ).buffs.spirit_shell;
      double old_amount    = spirit_shell.value();
      double new_amount    = clamp(
          s->result_amount, 0.0,
          s->target->resources.current[ RESOURCE_HEALTH ] * 0.6 - old_amount );

      spirit_shell.trigger( 1, old_amount + new_amount );
      stats->add_result( 0.0, new_amount, ABSORB, s->result, s->block_result,
                         s->target );
    }

    void trigger( action_state_t* s )
    {
      assert( s->result != RESULT_CRIT );
      base_dd_min = base_dd_max = s->result_amount;
      target                  = s->target;
      trigger_crit_multiplier = s->composite_crit();
      execute();
    }
  };

  divine_aegis_t* da;
  spirit_shell_absorb_t* ss;
  unsigned divine_aegis_trigger_mask;
  bool can_trigger_EoL, can_trigger_spirit_shell;

  void init() override
  {
    base_t::init();

    if ( divine_aegis_trigger_mask && priest.specs.divine_aegis->ok() )
    {
      da = new divine_aegis_t( name_str, priest );
      // add_child( da );
    }

    if ( can_trigger_spirit_shell )
      ss = new spirit_shell_absorb_t( *this );
  }

  priest_heal_t( const std::string& n, priest_t& player,
                 const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s ),
      da( nullptr ),
      ss( nullptr ),
      divine_aegis_trigger_mask( RESULT_CRIT_MASK |
                                 ( 1 << RESULT_MULTISTRIKE_CRIT ) ),
      can_trigger_EoL( true ),
      can_trigger_spirit_shell( false )
  {
  }

  double composite_crit() const override
  {
    double cc = base_t::composite_crit();

    if ( priest.buffs.chakra_serenity->up() )
      cc += priest.buffs.chakra_serenity->data().effectN( 1 ).percent();

    return cc;
  }

  double composite_target_crit( player_t* t ) const override
  {
    double ctc = base_t::composite_target_crit( t );

    if ( get_td( t ).buffs.holy_word_serenity->check() )
      ctc +=
          get_td( t ).buffs.holy_word_serenity->data().effectN( 2 ).percent();

    return ctc;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    am *= 1.0 + priest.buffs.archangel->value();

    return am;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = base_t::composite_target_multiplier( t );

    return ctm;
  }

  void execute() override
  {
    if ( can_trigger_spirit_shell )
      may_crit = priest.buffs.spirit_shell->check() == 0;

    base_t::execute();

    may_crit = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( ss && priest.buffs.spirit_shell->up() )
    {
      ss->trigger( s );
    }
    else
    {
      double save_health_percentage = s->target->health_percentage();

      base_t::impact( s );

      if ( s->result_amount > 0 )
      {
        trigger_echo_of_light( this, s );

        if ( priest.buffs.chakra_serenity->up() &&
             get_td( s->target ).dots.renew->is_ticking() )
        {
          get_td( s->target ).dots.renew->refresh_duration();
        }

        if ( priest.specialization() != PRIEST_SHADOW &&
             priest.talents.twist_of_fate->ok() &&
             ( save_health_percentage <
               priest.talents.twist_of_fate->effectN( 1 ).base_value() ) )
        {
          priest.buffs.twist_of_fate->trigger();
        }
      }
    }
  }

  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    remove_crit_amount_divine_aegis( s );
    base_t::assess_damage( type, s );
    trigger_divine_aegis( s );
  }

  /* Helper function to check if divine aegis trigger can be applied.
   */
  bool can_trigger_divine_aegis( const action_state_t* s ) const
  {
    if ( s->result_total > 0 )
    {
      if ( da && ( ( 1 << s->result ) & divine_aegis_trigger_mask ) != 0 )
      {
        return true;
      }
    }
    return false;
  }

  void remove_crit_amount_divine_aegis( action_state_t* s )
  {
    if ( can_trigger_divine_aegis( s ) )
    {
      double crit_bonus       = total_crit_bonus();
      double non_crit_portion = s->result_total / ( 1.0 + crit_bonus );
      s->result_total =
          non_crit_portion;  // remove crit portion from source spell
    }
  }

  void trigger_divine_aegis( action_state_t* s )
  {
    if ( can_trigger_divine_aegis( s ) )
    {
      double crit_bonus  = total_crit_bonus();
      double crit_amount = s->result_total * crit_bonus;
      da->trigger( s, crit_amount );
    }
  }

  // Priest Echo of Light, Ignite-Mechanic specialization
  void trigger_echo_of_light( priest_heal_t* h, action_state_t* s )
  {
    priest_t& p = h->priest;
    if ( !p.mastery_spells.echo_of_light->ok() || !can_trigger_EoL )
      return;

    residual_action::trigger(
        p.active_spells.echo_of_light,                 // ignite spell
        s->target,                                     // target
        s->result_amount * p.cache.mastery_value() );  // ignite damage
  }

  void trigger_serendipity( bool tier17_4pc = false )
  {
    int stack      = priest.buffs.serendipity->check();
    bool triggered = false;

    if ( tier17_4pc && priest.sets.has_set_bonus( PRIEST_HOLY, T17, B4 ) &&
         rng().roll(
             priest.sets.set( PRIEST_HOLY, T17, B4 )->effectN( 1 ).percent() ) )
    {
      triggered = priest.buffs.serendipity->trigger();
    }
    else if ( !tier17_4pc )
    {
      triggered = priest.buffs.serendipity->trigger();
    }

    if ( triggered )
    {
      if ( tier17_4pc )
      {
        if ( priest.buffs.serendipity->check() == stack )
          priest.procs.t17_4pc_holy_overflow->occur();
        else
          priest.procs.t17_4pc_holy->occur();
      }
      else
      {
        if ( priest.buffs.serendipity->check() == stack )
          priest.procs.serendipity_overflow->occur();
        else
          priest.procs.serendipity->occur();
      }
    }
  }

  void trigger_strength_of_soul( player_t* t )
  {
    if ( priest.specs.strength_of_soul->ok() && t->buffs.weakened_soul->up() )
      t->buffs.weakened_soul->extend_duration(
          player,
          timespan_t::from_seconds(
              -1 * priest.specs.strength_of_soul->effectN( 1 ).base_value() ) );
  }

  void consume_serendipity()
  {
    priest.buffs.serendipity->up();
    priest.buffs.serendipity->expire();
  }

  void consume_surge_of_light()
  {
    priest.buffs.surge_of_light->up();
    priest.buffs.surge_of_light->expire();
  }

  /**
   * 124519-boss-13-priest-trinket Discipline
   */
  void trigger_naarus_discipline( const action_state_t* s )
  {
    if ( priest.active_items.naarus_discipline )
    {
      const item_t* item = priest.active_items.naarus_discipline->item;
      double stacks      = 1;
      double value       = priest.active_items.naarus_discipline->trigger()
                         ->effectN( 1 )
                         .average( item ) /
                     100.0;
      s->target->buffs.naarus_discipline->trigger( static_cast<int>( stacks ),
                                                   value );
    }
  }
};

// Shadow Orb State ===================================================

struct shadow_orb_state_t : public action_state_t
{
  int orbs_used;

  shadow_orb_state_t( action_t* a, player_t* t )
    : action_state_t( a, t ), orbs_used( 0 )
  {
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " orbs_used=" << orbs_used;
    return s;
  }

  void initialize() override
  {
    action_state_t::initialize();
    orbs_used = 0;
  }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    const shadow_orb_state_t* dps_t =
        static_cast<const shadow_orb_state_t*>( o );
    orbs_used = dps_t->orbs_used;
  }
};

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public priest_action_t<spell_t>
{
  // Atonement heal =========================================================
  struct atonement_heal_t : public priest_heal_t
  {
    atonement_heal_t( const std::string& n, priest_t& p )
      : priest_heal_t(
            n, p,
            p.find_spell(
                81749 ) /* accessed through id so both disc and holy can use it */ )
    {
      background     = true;
      round_base_dmg = false;
      hasted_ticks   = false;

      // We set the base crit chance to 100%, so that simply setting
      // may_crit = true (in trigger()) will force a crit. When the trigger
      // spell crits, the atonement crits as well and procs Divine Aegis.
      base_crit = 1.0;

      snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;

      if ( !p.options.atonement_target_str.empty() )
        target = sim->find_player( p.options.atonement_target_str );
      else
        target = nullptr;
    }

    void trigger( double damage, dmg_e dmg_type, result_e result )
    {
      // Atonement caps at 30% of the Priest's health
      double cap = priest.resources.current[ RESOURCE_HEALTH ] * 0.3;

      if ( result == RESULT_CRIT )
      {
        // Assume crits cap at 200% of the non-crit cap; needs testing.
        cap *= 2.0;
      }

      base_dd_min = base_dd_max =
          std::min( cap, damage * data().effectN( 1 ).percent() );

      direct_tick = dual = ( dmg_type == DMG_OVER_TIME );
      may_crit = ( result == RESULT_CRIT );

      execute();
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = priest_heal_t::composite_target_multiplier( target );
      if ( target == player )
        m *= 0.5;  // Hardcoded in the tooltip
      return m;
    }

    double total_crit_bonus() const override
    {
      return 0;
    }

    void execute() override
    {
      player_t* saved_target = target;
      if ( !target )
        target = find_lowest_player();

      priest_heal_t::execute();

      target = saved_target;
    }

    void tick( dot_t* d ) override
    {
      player_t* saved_target = target;
      if ( !target )
        target = find_lowest_player();

      priest_heal_t::tick( d );

      target = saved_target;
    }
  };

  atonement_heal_t* atonement;
  bool can_trigger_atonement;
  bool is_mind_spell;

  priest_spell_t( const std::string& n, priest_t& player,
                  const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s ),
      atonement( nullptr ),
      can_trigger_atonement( false ),
      is_mind_spell( false )
  {
    weapon_multiplier = 0.0;
  }

  void init() override
  {
    base_t::init();

    if ( can_trigger_atonement )
      atonement = new atonement_heal_t( "atonement_" + name_str, priest );
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
    }
  }

  void assess_damage( dmg_e type, action_state_t* s ) override
  {
    base_t::assess_damage( type, s );

    if ( aoe == 0 && result_is_hit( s->result ) &&
         priest.buffs.vampiric_embrace->up() )
      trigger_vampiric_embrace( s );

    if ( atonement )
      trigger_atonement( type, s );
  }

  /* Based on previous implementation ( pets don't count but get full heal )
   * and http://www.wowhead.com/spell=15286#comments:id=1796701
   * Last checked 2013/05/25
   */
  void trigger_vampiric_embrace( action_state_t* s )
  {
    double amount = s->result_amount;
    amount *= ( priest.buffs.vampiric_embrace->data().effectN( 1 ).percent() +
                priest.glyphs.vampiric_embrace->effectN( 2 ).percent() );

    // Get all non-pet, non-sleeping players
    std::vector<player_t*> ally_list;
    ;
    range::remove_copy_if( sim->player_no_pet_list.data(),
                           back_inserter( ally_list ), player_t::_is_sleeping );

    for ( size_t i = 0; i < ally_list.size(); ++i )
    {
      player_t& q = *ally_list[ i ];

      q.resource_gain( RESOURCE_HEALTH, amount, q.gains.vampiric_embrace );

      for ( size_t j = 0; j < q.pet_list.size(); ++j )
      {
        pet_t& r = *q.pet_list[ j ];
        r.resource_gain( RESOURCE_HEALTH, amount, r.gains.vampiric_embrace );
      }
    }
  }

  void trigger_atonement( dmg_e type, action_state_t* s )
  {
    atonement->trigger( s->result_amount, direct_tick ? DMG_OVER_TIME : type,
                        s->result );
  }

  void generate_shadow_orb( unsigned num_orbs, gain_t* g = nullptr )
  {
    if ( priest.specs.shadow_orbs->ok() )
      priest.resource_gain( RESOURCE_SHADOW_ORB, num_orbs, g, this );
  }

  void trigger_insanity( action_state_t* s, int orbs )
  {
    if ( priest.talents.insanity->ok() )
    {
      if ( priest.buffs.insanity->up() )
      {
        priest.buffs.insanity->trigger(
            1, 0.0, 1.0,
            timespan_t::from_seconds(
                priest.buffs.insanity->default_value * orbs * s->haste +
                priest.buffs.insanity->remains().total_seconds() ) );
      }
      else
      {
        priest.buffs.insanity->trigger(
            1, 0.0, 1.0,
            timespan_t::from_seconds( priest.buffs.insanity->default_value *
                                      orbs * s->haste ) );
      }
    }
  }

  bool trigger_surge_of_darkness()
  {
    int stack = priest.buffs.surge_of_darkness->check();
    if ( priest.buffs.surge_of_darkness->trigger() )
    {
      if ( priest.buffs.surge_of_darkness->check() == stack )
        priest.procs.surge_of_darkness_overflow->occur();
      else
        priest.procs.surge_of_darkness->occur();
      return true;
    }
    return false;
  }
};

namespace spells
{
int cancel_dot( dot_t& dot )
{
  if ( dot.is_ticking() )
  {
    int lostTicks = dot.ticks_left();
    dot.cancel();
    return lostTicks;
  }
  return 0;
}

// ==========================================================================
// Priest Abilities
// ==========================================================================

// ==========================================================================
// Priest Non-Harmful Spells
// ==========================================================================

// Angelic Feather===========================================================

struct angelic_feather_t : public priest_spell_t
{
  angelic_feather_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "angelic_feather", p,
                      p.find_class_spell( "Angelic Feather" ) )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
    castable_in_shadowform = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    s->target->buffs.angelic_feather->trigger();
  }
};

// Archangel Spell ==========================================================

struct archangel_t : public priest_spell_t
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

    if ( priest.sets.has_set_bonus( PRIEST_DISCIPLINE, T17, B4 ) )
      priest.buffs.clear_thoughts->trigger();
  }

  bool ready() override
  {
    if ( !priest.buffs.holy_evangelism->check() )
      return false;
    else
      return priest_spell_t::ready();
  }
};

// Chakra_Pre Spell =========================================================

struct chakra_base_t : public priest_spell_t
{
  chakra_base_t( priest_t& p, const spell_data_t* s,
                 const std::string& options_str )
    : priest_spell_t( "chakra", p, s )
  {
    parse_options( options_str );

    harmful               = false;
    ignore_false_positive = true;

    p.cooldowns.chakra->duration = cooldown->duration;

    if ( priest.perks.enhanced_chakras->ok() )
    {
      p.cooldowns.chakra->duration -=
          p.perks.enhanced_chakras->effectN( 1 ).time_value();
    }

    cooldown = p.cooldowns.chakra;
  }

  void switch_to( buff_t* b )
  {
    if ( priest.buffs.chakra_sanctuary != b )
      priest.buffs.chakra_sanctuary->expire();
    if ( priest.buffs.chakra_serenity != b )
      priest.buffs.chakra_serenity->expire();
    if ( priest.buffs.chakra_chastise != b )
      priest.buffs.chakra_chastise->expire();

    b->trigger();
  }
};

struct chakra_chastise_t : public chakra_base_t
{
  chakra_chastise_t( priest_t& p, const std::string& options_str )
    : chakra_base_t( p, p.find_class_spell( "Chakra: Chastise" ), options_str )
  {
  }

  void execute() override
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_chastise );
  }
};

struct chakra_sanctuary_t : public chakra_base_t
{
  chakra_sanctuary_t( priest_t& p, const std::string& options_str )
    : chakra_base_t( p, p.find_class_spell( "Chakra: Sanctuary" ), options_str )
  {
  }

  void execute() override
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_sanctuary );
  }
};

struct chakra_serenity_t : public chakra_base_t
{
  chakra_serenity_t( priest_t& p, const std::string& options_str )
    : chakra_base_t( p, p.find_class_spell( "Chakra: Serenity" ), options_str )
  {
  }

  void execute() override
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_serenity );
  }
};

// Dispersion Spell =========================================================

struct dispersion_t : public priest_spell_t
{
  dispersion_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "dispersion", player,
                      player.find_class_spell( "Dispersion" ) )
  {
    parse_options( options_str );

    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_duration   = timespan_t::from_seconds( 6.0 ) +
                   priest.glyphs.delayed_coalescence->effectN( 1 ).time_value();

    ignore_false_positive = true;
    channeled             = true;
    harmful               = false;
    tick_may_crit         = false;
    // hasted_ticks      = false;

    cooldown->duration =
        data().cooldown() + priest.glyphs.dispersion->effectN( 1 ).time_value();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.dispersion->trigger(
        1, 0, 1, dot_duration );  // timespan_t::from_seconds( 6.0 ) +
                                  // priest.glyphs.delayed_coalescence ->
                                  // effectN( 1 ).time_value() );
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return timespan_t::from_seconds( 6.0 ) +
           priest.glyphs.delayed_coalescence->effectN( 1 ).time_value();
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );
    priest.buffs.dispersion->expire();
  }
};

// Fortitude Spell ==========================================================

struct fortitude_t : public priest_spell_t
{
  fortitude_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "fortitude", player,
                      player.find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    harmful = false;

    background = ( sim->overrides.stamina != 0 );
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( !sim->overrides.stamina )
      sim->auras.stamina->trigger();
  }
};

// Levitate =================================================================

struct levitate_t : public priest_spell_t
{
  levitate_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "levitate", p, p.find_class_spell( "Levitate" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.glyph_of_levitate->trigger();
  }
};

// Pain Supression ==========================================================

struct pain_suppression_t : public priest_spell_t
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

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
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

// Shadowform Spell ========================================================

struct shadowform_t : public priest_spell_t
{
  shadowform_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadowform", p, p.find_class_spell( "Shadowform" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;

    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.shadowform->trigger();
  }
};

// Spirit Shell Spell =======================================================

struct spirit_shell_t : public priest_spell_t
{
  spirit_shell_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "spirit_shell", p, p.specs.spirit_shell )
  {
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.spirit_shell->trigger();
  }
};

// Pet Summon Base Class

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

struct summon_shadowfiend_t : public summon_pet_t
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

struct summon_mindbender_t : public summon_pet_t
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

// ==========================================================================
// Priest Damage Spells
// ==========================================================================

// Shadowy Apparition Spell =================================================

struct shadowy_apparition_spell_t : public priest_spell_t
{
  shadowy_apparition_spell_t( priest_t& p )
    : priest_spell_t( "shadowy_apparitions", p, p.find_spell( 78203 ) )
  {
    background          = true;
    proc                = false;
    callbacks           = true;
    may_miss            = false;
    instant_multistrike = 0;

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

    if ( priest.talents.auspicious_spirits->ok() && result_is_hit( s->result ) )
    {
      generate_shadow_orb( 1, priest.gains.shadow_orb_auspicious_spirits );
    }

    if ( rng().roll(
             priest.sets.set( SET_CASTER, T15, B2 )->effectN( 1 ).percent() ) )
    {
      priest_td_t& td = get_td( s->target );
      priest.procs.t15_2pc_caster->occur();

      timespan_t extend_duration = timespan_t::from_seconds(
          priest.sets.set( SET_CASTER, T15, B2 )->effectN( 2 ).base_value() );

      if ( td.dots.shadow_word_pain->is_ticking() )
      {
        td.dots.shadow_word_pain->extend_duration( extend_duration );
        priest.procs.t15_2pc_caster_shadow_word_pain->occur();
      }

      if ( td.dots.vampiric_touch->is_ticking() )
      {
        td.dots.vampiric_touch->extend_duration( extend_duration );
        priest.procs.t15_2pc_caster_vampiric_touch->occur();
      }
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

// Mind Blast Spell =========================================================

struct mind_blast_t : public priest_spell_t
{
  bool casted_with_shadowy_insight;

  mind_blast_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "mind_blast", player,
                      player.find_class_spell( "Mind Blast" ) ),
      casted_with_shadowy_insight( false )
  {
    parse_options( options_str );
    instant_multistrike = 0;
    is_mind_spell       = true;

    // Glyph of Mind Harvest
    if ( priest.glyphs.mind_harvest->ok() )
      priest.cooldowns.mind_blast->duration +=
          priest.glyphs.mind_harvest->effectN( 2 )
              .time_value();  // Effect #2 --
    // http://www.wowhead.com/spell=162532
    if ( priest.talents.clarity_of_power->ok() )
      priest.cooldowns.mind_blast->duration +=
          priest.talents.clarity_of_power->effectN( 3 )
              .time_value();  // Now Effect #3... wod.wowhead.com/spell=155246
  }

  void execute() override
  {
    priest_spell_t::execute();

    casted_with_shadowy_insight = false;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      generate_shadow_orb( 1, priest.gains.shadow_orb_mind_blast );

      // Glyph of Mind Harvest
      if ( priest.glyphs.mind_harvest->ok() )
      {
        priest_td_t& td = get_td( s->target );
        if ( !td.glyph_of_mind_harvest_consumed )
        {
          td.glyph_of_mind_harvest_consumed = true;
          generate_shadow_orb(
              priest.glyphs.mind_harvest->effectN( 1 ).base_value(),
              priest.gains.shadow_orb_mind_harvest );  // Effect #1 --
          // http://www.wowhead.com/spell=162532

          if ( sim->debug )
            sim->out_debug.printf(
                "%s consumed Glyph of Mind Harvest on target %s.",
                priest.name(), s->target->name() );
        }
      }
      priest.buffs.glyph_of_mind_spike->expire();
    }
  }

  void consume_resource() override
  {
    if ( casted_with_shadowy_insight )
      resource_consumed = 0.0;
    else
      resource_consumed = cost();

    player->resource_loss( current_resource(), resource_consumed, nullptr,
                           this );

    if ( sim->log )
      sim->out_log.printf(
          "%s consumes %.1f %s for %s (%.0f)", player->name(),
          resource_consumed, util::resource_type_string( current_resource() ),
          name(), player->resources.current[ current_resource() ] );

    stats->consume_resource( current_resource(), resource_consumed );
  }

  double cost() const override
  {
    if ( priest.buffs.shadowy_insight->check() )
      return 0.0;

    return priest_spell_t::cost();
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    if ( priest.buffs.shadowy_insight->up() )
    {
      casted_with_shadowy_insight = true;
    }

    priest_spell_t::schedule_execute( state );

    priest.buffs.shadowy_insight->expire();
  }

  void update_ready( timespan_t cd_duration ) override
  {
    if ( cd_duration < timespan_t::zero() )
      cd_duration = cooldown->duration;

    // CD is now always reduced by haste. Documented in the WoD Alpha Patch
    // Notes, unfortunately not in any tooltip!
    // 2014-06-17
    cd_duration = cooldown->duration * composite_haste();

    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows->up();  // benefit tracking
    priest.buffs.empowered_shadows->expire();
    priest.buffs.glyph_of_mind_spike->up();  // benefit tracking
  }

  timespan_t execute_time() const override
  {
    if ( priest.buffs.shadowy_insight->check() ||
         priest.talents.clarity_of_power->ok() )
    {
      return timespan_t::zero();
    }

    timespan_t et = priest_spell_t::execute_time();

    et *= 1.0 +
          priest.buffs.glyph_of_mind_spike->check() *
              priest.buffs.glyph_of_mind_spike->data().effectN( 1 ).percent();

    return et;
  }

  void reset() override
  {
    priest_spell_t::reset();

    casted_with_shadowy_insight = false;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( priest.buffs.empowered_shadows->check() )
      d *= 1.0 +
           priest.buffs.empowered_shadows->current_value *
               priest.buffs.empowered_shadows->check();

    if ( priest.mastery_spells.mental_anguish->ok() )
    {
      d *= 1.0 + priest.cache.mastery_value();
    }

    d *= 1.0 + priest.sets.set( SET_CASTER, T16, B2 )->effectN( 1 ).percent();

    return d;
  }
};

// Mind Spike Spell =========================================================

struct mind_spike_t : public priest_spell_t
{
  struct mind_spike_state_t : public action_state_t
  {
    bool surge_of_darkness;

    mind_spike_state_t( action_t* a, player_t* t )
      : action_state_t( a, t ), surge_of_darkness( false )
    {
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s ) << " surge_of_darkness="
                                     << surge_of_darkness;
      return s;
    }

    void copy_state( const action_state_t* o ) override
    {
      action_state_t::copy_state( o );
      const mind_spike_state_t* dps_t =
          static_cast<const mind_spike_state_t*>( o );
      surge_of_darkness = dps_t->surge_of_darkness;
    }

    void initialize() override
    {
      action_state_t::initialize();
      surge_of_darkness = false;
    }
  };

  bool casted_with_surge_of_darkness;

  mind_spike_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "mind_spike", player,
                      player.find_class_spell( "Mind Spike" ) ),
      casted_with_surge_of_darkness( false )
  {
    parse_options( options_str );
    instant_multistrike = 0;
    is_mind_spell       = true;
  }

  action_state_t* new_state() override
  {
    return new mind_spike_state_t( this, target );
  }

  action_state_t* get_state( const action_state_t* s ) override
  {
    action_state_t* s_ = priest_spell_t::get_state( s );

    if ( !s )
    {
      mind_spike_state_t* ds_ = static_cast<mind_spike_state_t*>( s_ );
      ds_->surge_of_darkness  = false;
    }

    return s_;
  }

  void snapshot_state( action_state_t* state, dmg_e type ) override
  {
    mind_spike_state_t* ms_s = static_cast<mind_spike_state_t*>( state );

    ms_s->surge_of_darkness = casted_with_surge_of_darkness;

    priest_spell_t::snapshot_state( state, type );
  }

  void execute() override
  {
    priest_spell_t::execute();

    casted_with_surge_of_darkness = false;
  }

  void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows->up();  // benefit tracking
    priest.buffs.empowered_shadows->expire();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      const mind_spike_state_t* ms_s =
          static_cast<const mind_spike_state_t*>( s );
      if ( !ms_s->surge_of_darkness &&
           !priest.buffs.surge_of_darkness->up() )  // In Beta (and I suspect on
                                                    // Live), if you are in the
                                                    // middle of hardcasting a
                                                    // Mind Spike and a Surge of
      // Darkness proc occurs before
      // the cast time ends, DoTs
      // are not removed. Probably a
      // bug. -Twintop 2014/07/05
      {
        int ticksLost   = 0;
        priest_td_t& td = get_td( s->target );

        bool removed_dot = false;

        ticksLost = cancel_dot( *td.dots.devouring_plague_tick );
        if ( ticksLost > 0 )
        {
          priest.procs.mind_spike_dot_removal_devouring_plague->occur();

          for ( int x = 0; x < ticksLost; x++ )
            priest.procs.mind_spike_dot_removal_devouring_plague_ticks->occur();

          priest.procs.sd_mind_spike_dot_removal_devouring_plague_ticks->add(
              ticksLost );

          removed_dot = true;
        }

        ticksLost = cancel_dot( *td.dots.shadow_word_pain );
        if ( ticksLost > 0 )
        {
          priest.procs.mind_spike_dot_removal_shadow_word_pain->occur();

          for ( int x = 0; x < ticksLost; x++ )
            priest.procs.mind_spike_dot_removal_shadow_word_pain_ticks->occur();

          removed_dot = true;
        }

        ticksLost = cancel_dot( *td.dots.vampiric_touch );
        if ( ticksLost > 0 )
        {
          priest.procs.mind_spike_dot_removal_vampiric_touch->occur();

          for ( int x = 0; x < ticksLost; x++ )
            priest.procs.mind_spike_dot_removal_vampiric_touch_ticks->occur();

          removed_dot = true;
        }

        ticksLost = cancel_dot( *td.dots.void_entropy );
        if ( ticksLost > 0 )
        {
          priest.procs.mind_spike_dot_removal_void_entropy->occur();

          for ( int x = 0; x < ticksLost; x++ )
            priest.procs.mind_spike_dot_removal_void_entropy_ticks->occur();

          removed_dot = true;
        }

        if ( removed_dot )
          priest.procs.mind_spike_dot_removal->occur();

        priest.buffs.glyph_of_mind_spike->trigger();
      }

      if ( trigger_shadowy_insight() )
        priest.procs.shadowy_insight_from_mind_spike->occur();
    }
  }

  void schedule_execute( action_state_t* state ) override
  {
    if ( priest.buffs.surge_of_darkness->up() )
    {
      casted_with_surge_of_darkness = true;
    }

    priest_spell_t::schedule_execute( state );

    priest.buffs.surge_of_darkness->decrement();
  }

  void consume_resource() override
  {
    if ( casted_with_surge_of_darkness )
      resource_consumed = 0.0;
    else
      resource_consumed = cost();

    player->resource_loss( current_resource(), resource_consumed, nullptr,
                           this );

    if ( sim->log )
      sim->out_log.printf(
          "%s consumes %.1f %s for %s (%.0f)", player->name(),
          resource_consumed, util::resource_type_string( current_resource() ),
          name(), player->resources.current[ current_resource() ] );

    stats->consume_resource( current_resource(), resource_consumed );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( casted_with_surge_of_darkness )
    {
      d *= 1.0 + priest.active_spells.surge_of_darkness->effectN( 4 ).percent();
    }

    if ( priest.mastery_spells.mental_anguish->ok() )
    {
      d *= 1.0 + priest.cache.mastery_value();
    }

    if ( priest.buffs.empowered_shadows->check() )
      d *= 1.0 +
           priest.buffs.empowered_shadows->current_value *
               priest.buffs.empowered_shadows->check();

    priest_td_t& td = get_td( target );
    if ( priest.talents.clarity_of_power->ok() &&
         !td.dots.shadow_word_pain->is_ticking() &&
         !td.dots.vampiric_touch->is_ticking() )
      d *= 1.0 + priest.talents.clarity_of_power->effectN( 1 ).percent();

    return d;
  }

  double cost() const override
  {
    if ( priest.buffs.surge_of_darkness->check() )
      return 0.0;

    return priest_spell_t::cost();
  }

  timespan_t execute_time() const override
  {
    if ( priest.buffs.surge_of_darkness->check() )
    {
      return timespan_t::zero();
    }

    return priest_spell_t::execute_time();
  }

  void reset() override
  {
    priest_spell_t::reset();
    casted_with_surge_of_darkness = false;
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t : public priest_spell_t
{
  mind_sear_tick_t( priest_t& p, const spell_data_t* mind_sear )
    : priest_spell_t( "mind_sear_tick", p, mind_sear->effectN( 1 ).trigger() )
  {
    radius              = data().effectN( 1 ).radius();
    background          = true;
    dual                = true;
    aoe                 = -1;
    callbacks           = false;
    direct_tick         = true;
    instant_multistrike = 0;
    use_off_gcd         = true;
  }
};

template <bool insanity = false>
struct mind_sear_base_t : public priest_spell_t
{
  mind_sear_base_t( priest_t& p, const std::string& options_str,
                    const std::string& name = "mind_sear" )
    : priest_spell_t( name, p, p.find_class_spell( insanity ? "Searing Insanity"
                                                            : "Mind Sear" ) )
  {
    parse_options( options_str );
    channeled           = true;
    may_crit            = false;
    hasted_ticks        = false;
    dynamic_tick_action = true;
    tick_zero           = false;
    instant_multistrike = 0;
    is_mind_spell       = true;

    tick_action = new mind_sear_tick_t(
        p, p.find_class_spell( insanity ? "Searing Insanity" : "Mind Sear" ) );
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    priest_td_t& td = get_td( target );
    if ( priest.talents.clarity_of_power->ok() &&
         !td.dots.shadow_word_pain->is_ticking() &&
         !td.dots.vampiric_touch->is_ticking() )
      am *= 1.0 + priest.talents.clarity_of_power->effectN( 1 ).percent();

    return am;
  }

  void last_tick( dot_t* d ) override
  {
    if ( d->current_tick == d->num_ticks )
    {
      priest.buffs.mind_sear_on_hit_reset->expire();
    }

    priest_spell_t::last_tick( d );
  }

  void impact( action_state_t* s ) override
  {
    // Mind Sear does on-hit damage only when there is a GCD of another ability
    // between it, so chaining Mind Sears doesn't allow for the on-hit to happen
    // again.
    if ( priest.buffs.mind_sear_on_hit_reset->check() == 0 )
    {
      priest.buffs.mind_sear_on_hit_reset->trigger( 2, 1, 1,
                                                    tick_time( s->haste ) * 6 );
      tick_zero = true;
    }
    else
    {
      priest.buffs.mind_sear_on_hit_reset->trigger( 1 );
      tick_zero = false;
    }

    priest_spell_t::impact( s );
  }
};

struct searing_insanity_t : public mind_sear_base_t<true>
{
  typedef mind_sear_base_t<true> base_t;

  searing_insanity_t( priest_t& p, const std::string& options_str )
    : base_t( p, options_str, "searing_insanity" )
  {
  }

  void execute() override
  {
    // benefit tracking
    priest.buffs.insanity->up();

    base_t::execute();
  }

  bool ready() override
  {
    if ( !priest.buffs.insanity->check() )
      return false;

    return base_t::ready();
  }
};

// Shadow Word Death Spell ==================================================

struct shadow_word_death_t : public priest_spell_t
{
  struct shadow_word_death_backlash_t : public priest_spell_t
  {
    double spellpower;
    double multiplier;

    shadow_word_death_backlash_t( priest_t& p )
      : priest_spell_t( "shadow_word_death_backlash", p,
                        p.find_class_spell( "Shadow Word: Death" ) ),
        spellpower( 0.0 ),
        multiplier( 1.0 )
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

      return d / 4.0;
    }
  };

  shadow_word_death_backlash_t* backlash;

  shadow_word_death_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_death", p,
                      p.find_specialization_spell( "Shadow Word: Death" ) ),
      backlash( new shadow_word_death_backlash_t( p ) )
  {
    parse_options( options_str );
    instant_multistrike = 0;

    spell_power_mod.direct = data().effectN( 2 ).sp_coeff();
    base_multiplier *=
        1.0 + p.sets.set( SET_CASTER, T13, B2 )->effectN( 1 ).percent();
  }

  void execute() override
  {
    bool below_20 = ( target->health_percentage() < 20.0 );

    priest_spell_t::execute();

    if ( below_20 && !priest.buffs.shadow_word_death_reset_cooldown->up() )
    {
      cooldown->reset( false );
      priest.buffs.shadow_word_death_reset_cooldown->trigger();
    }
  }

  void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows->up();  // benefit tracking
    priest.buffs.empowered_shadows->expire();
  }

  void impact( action_state_t* s ) override
  {
    s->result_amount = floor( s->result_amount );

    bool over_20 = ( s->target->health_percentage() >= 20.0 );
    if ( backlash && over_20 && s->result == RESULT_HIT )
    {
      backlash->spellpower = s->composite_spell_power();
      backlash->multiplier = s->da_multiplier;
      backlash->schedule_execute();
    }

    if ( result_is_hit( s->result ) )
    {
      if ( over_20 )
      {
        s->result_amount /= 4.0;
      }
      // Assume from the wording of perk 'Enhanced Shadow Word: Death' that
      // it's
      // a 100% chance.
      else if ( !priest.buffs.shadow_word_death_reset_cooldown->check() ||
                priest.perks.enhanced_shadow_word_death->ok() )
      {
        generate_shadow_orb( 1, priest.gains.shadow_orb_shadow_word_death );
      }
    }

    priest_spell_t::impact( s );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( priest.buffs.empowered_shadows->check() )
      d *= 1.0 +
           priest.buffs.empowered_shadows->current_value *
               priest.buffs.empowered_shadows->check();

    priest_td_t& td = get_td( target );
    if ( priest.talents.clarity_of_power->ok() &&
         !td.dots.shadow_word_pain->is_ticking() &&
         !td.dots.vampiric_touch->is_ticking() )
      d *= 1.0 + priest.talents.clarity_of_power->effectN( 1 ).percent();

    return d;
  }

  bool ready() override
  {
    if ( !priest_spell_t::ready() )
      return false;

    if ( priest.glyphs.shadow_word_death->ok() ||
         target->health_percentage() < 20.0 )
      return true;

    return false;
  }
};

// Shadow Orb State ===================================================

struct dp_state_t : public shadow_orb_state_t
{
  double tick_dmg;
  typedef shadow_orb_state_t base_t;

  dp_state_t( action_t* a, player_t* t ) : base_t( a, t ), tick_dmg( 0.0 )
  {
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    base_t::debug_str( s ) << " direct_dmg=" << tick_dmg
                           << " tick_heal=" << tick_dmg;
    return s;
  }

  void initialize() override
  {
    base_t::initialize();
    tick_dmg = 0.0;
  }

  void copy_state( const action_state_t* o ) override
  {
    base_t::copy_state( o );
    const dp_state_t* dps_t = static_cast<const dp_state_t*>( o );
    tick_dmg                = dps_t->tick_dmg;
  }
};

// Devouring Plague DoT ===================================================

struct devouring_plague_dot_t : public priest_spell_t
{
  double heal_to_dmg_coefficient;  // proportion of tick heal to tick dmg

  devouring_plague_dot_t( priest_t& p )
    : priest_spell_t( "devouring_plague_tick", p, p.find_spell( 158831 ) ),
      heal_to_dmg_coefficient( 1.0 )
  {
    const spell_data_t* parent_spelldata =
        p.find_class_spell( "Devouring Plague" );
    heal_to_dmg_coefficient = parent_spelldata->effectN( 3 ).percent() /
                              parent_spelldata->effectN( 2 ).percent();

    hasted_ticks    = true;
    tick_may_crit   = false;
    may_multistrike = false;
    tick_zero       = false;

    base_dd_min = base_dd_max = spell_power_mod.tick = spell_power_mod.direct =
        base_td = 0.0;

    background = true;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s->haste ) / base_tick_time );
  }

  void init() override
  {
    priest_spell_t::init();

    snapshot_flags = update_flags = 0;
  }

  void reset() override
  {
    priest_spell_t::reset();
    base_ta_adder = 0;
  }

  action_state_t* new_state() override
  {
    return new dp_state_t( this, target );
  }

  action_state_t* get_state( const action_state_t* s = nullptr ) override
  {
    action_state_t* s_ = priest_spell_t::get_state( s );

    if ( !s )
    {
      dp_state_t* ds_ = static_cast<dp_state_t*>( s_ );
      ds_->orbs_used  = 0;
      ds_->tick_dmg   = 0.0;
    }

    return s_;
  }

  double calculate_tick_amount( action_state_t* state,
                                double dot_multiplier ) const override
  {
    // We already got the dmg stored, just return it.
    const dp_state_t* ds = static_cast<const dp_state_t*>( state );
    return ds->tick_dmg * dot_multiplier;
  }

  /* Precondition: dot is ticking!
   */
  void append_damage( player_t* target, double dmg )
  {
    dot_t* dot = get_dot( target );
    if ( !dot->is_ticking() )
    {
      if ( sim->debug )
        sim->out_debug.printf(
            "%s could not appended damage because the dot is no longer "
            "ticking."
            "( This should only be the case if the dot drops between main "
            "impact and multistrike impact. )",
            player->name() );
      return;
    }

    dp_state_t* ds = static_cast<dp_state_t*>( dot->state );
    ds->tick_dmg += dmg / dot->ticks_left();
    if ( sim->debug )
      sim->out_debug.printf(
          "%s appended %.2f damage / %.2f per tick. New dmg per tick: %.2f",
          player->name(), dmg, dmg / dot->ticks_left(), ds->tick_dmg );
  }

  void impact( action_state_t* state ) override
  {
    dp_state_t* s = static_cast<dp_state_t*>( state );
    double saved_impact_dmg =
        s->result_amount;  // catch previous remaining dp damage
    int saved_orbs = s->orbs_used;

    s->result_amount = 0.0;
    priest_spell_t::impact( s );

    dot_t* dot     = get_dot( state->target );
    dp_state_t* ds = static_cast<dp_state_t*>( dot->state );
    assert( ds );
    ds->tick_dmg  = saved_impact_dmg / dot->ticks_left();
    ds->orbs_used = saved_orbs;
    if ( sim->debug )
      sim->out_debug.printf(
          "%s DP dot started with total of %.2f damage / %.2f per tick and "
          "%.2f heal / %.2f per tick.",
          player->name(), saved_impact_dmg, ds->tick_dmg, saved_impact_dmg,
          ds->tick_dmg );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    const dp_state_t* ds = static_cast<const dp_state_t*>( d->state );

    double a = ds->tick_dmg;
    priest.resource_gain( RESOURCE_HEALTH, a,
                          priest.gains.devouring_plague_health );

    priest.buffs.mental_instinct->trigger();

    if ( trigger_surge_of_darkness() )
      priest.procs.surge_of_darkness_from_devouring_plague->occur();
  }

  timespan_t calculate_dot_refresh_duration(
      const dot_t* dot, timespan_t triggered_duration ) const override
  {
    // Old Mop Dot Behaviour
    return dot->time_to_next_tick() + triggered_duration;
  }
};

// Devouring Plague Spell ===================================================

struct devouring_plague_t : public priest_spell_t
{
  devouring_plague_dot_t* dot_spell;

  devouring_plague_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "devouring_plague", p,
                      p.find_class_spell( "Devouring Plague" ) ),
      dot_spell( new devouring_plague_dot_t( p ) )
  {
    parse_options( options_str );

    parse_effect_data( data().effectN( 1 ) );

    spell_power_mod.direct = spell_power_mod.direct / 3.0;
    base_td                = 0;
    base_tick_time         = timespan_t::zero();
    dot_duration           = timespan_t::zero();
    instant_multistrike    = 0;

    add_child( dot_spell );
  }

  action_state_t* new_state() override
  {
    return new shadow_orb_state_t( this, target );
  }

  action_state_t* get_state( const action_state_t* s = nullptr ) override
  {
    action_state_t* s_ = priest_spell_t::get_state( s );

    if ( !s )
    {
      shadow_orb_state_t* ds_ = static_cast<shadow_orb_state_t*>( s_ );
      ds_->orbs_used          = 0;
    }

    return s_;
  }

  void snapshot_state( action_state_t* s, dmg_e type ) override
  {
    shadow_orb_state_t* ds = static_cast<shadow_orb_state_t*>( s );

    ds->orbs_used = (int)shadow_orbs_to_consume();

    priest_spell_t::snapshot_state( s, type );
  }

  void init() override
  {
    priest_spell_t::init();

    snapshot_flags |= STATE_HASTE;
  }

  void consume_resource() override
  {
    resource_consumed = cost();

    if ( execute_state->result != RESULT_MISS )
    {
      resource_consumed = shadow_orbs_to_consume();

      trigger_insanity( execute_state, static_cast<int>( resource_consumed ) );

      if ( priest.sets.has_set_bonus( PRIEST_SHADOW, T17, B2 ) )
      {
        if ( priest.cooldowns.mind_blast->remains() == timespan_t::zero() )
        {
          priest.procs.t17_2pc_caster_mind_blast_reset_overflow->occur();
        }
        else
        {
          priest.procs.t17_2pc_caster_mind_blast_reset->occur();
        }

        priest.cooldowns.mind_blast->adjust(
            -timespan_t::from_seconds( resource_consumed *
                                       priest.sets.set( PRIEST_SHADOW, T17, B2 )
                                           ->effectN( 1 )
                                           .time_value()
                                           .total_seconds() ),
            true );
      }
    }

    player->resource_loss( current_resource(), resource_consumed, nullptr,
                           this );

    if ( sim->log )
      sim->out_log.printf(
          "%s consumes %.1f %s for %s (%.0f)", player->name(),
          resource_consumed, util::resource_type_string( current_resource() ),
          name(), player->resources.current[ current_resource() ] );

    stats->consume_resource( current_resource(), resource_consumed );

    // Grants 20% per stack, 60% in total. Overriding DBC data as it is a flat
    // 50% no matter stack count. Updated 2013/08/11
    priest.buffs.empowered_shadows->trigger( 1, resource_consumed * 0.2 );
  }

  double action_da_multiplier() const override
  {
    double m = priest_spell_t::action_da_multiplier();

    m *= shadow_orbs_to_consume();

    return m;
  }

  // Shortened and modified version of the ignite mechanic
  // Damage from the old dot is added to the new one.
  // Important here that the combined damage then will get multiplicated by the
  // new orb amount.
  void trigger_dp_dot( action_state_t* state )
  {
    dot_t* dot = dot_spell->get_dot( state->target );

    double dmg_to_pass_to_dp = 0.0;

    if ( dot->is_ticking() )
    {
      const dp_state_t* ds = debug_cast<const dp_state_t*>( dot->state );
      dmg_to_pass_to_dp +=
          ds->tick_dmg * dot->ticks_left() * data().effectN( 2 ).percent();

      if ( sim->debug )
        sim->out_debug.printf(
            "%s DP was still ticking. Added %.2f damage to new dot, and "
            "%.4f%% "
            "heal%%/tick.",
            player->name(), dmg_to_pass_to_dp, dmg_to_pass_to_dp * 100.0 );
    }

    // Pass total amount of damage to the ignite dot_spell, and let it divide it
    // by the correct number of ticks!

    dp_state_t* s = debug_cast<dp_state_t*>( dot_spell->get_state() );
    s->target        = state->target;
    s->result        = RESULT_HIT;
    s->result_amount = dmg_to_pass_to_dp;  // pass the old remaining dp damage
                                           // to the dot_spell state, which will
                                           // be catched in its impact method.
    s->haste = state->haste;
    const shadow_orb_state_t* os =
        static_cast<const shadow_orb_state_t*>( state );
    s->orbs_used = os->orbs_used;
    dot_spell->schedule_travel( s );
    dot_spell->stats->add_execute( timespan_t::zero(), s->target );
  }

  void transfer_dmg_to_dot( action_state_t* state )
  {
    if ( sim->debug )
      sim->out_debug.printf(
          "%s DP result %s appends %.2f damage to dot", player->name(),
          util::result_type_string( state->result ), state->result_amount );

    dot_spell->append_damage( state->target, state->result_amount );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit_or_multistrike( s->result ) )
    {
      trigger_dp_dot( s );
      transfer_dmg_to_dot( s );
      priest.resource_gain( RESOURCE_HEALTH, s->result_amount,
                            priest.gains.devouring_plague_health );

      priest_td_t& td = get_td( s->target );

      // TODO: Verify if Multistrikes also refresh. -- Twintop 2014/08/05
      // DP refreshes the
      if ( td.dots.void_entropy->is_ticking() && result_is_hit( s->result ) )
      {
        timespan_t nextTick = td.dots.void_entropy->time_to_next_tick();

        if ( td.dots.void_entropy->remains().total_seconds() >
             ( 60 + nextTick.total_seconds() ) )
        {
          td.dots.void_entropy->reduce_duration( timespan_t::from_seconds(
              td.dots.void_entropy->remains().total_seconds() - 60 -
              nextTick.total_seconds() ) );
        }
        else
        {
          td.dots.void_entropy->extend_duration( timespan_t::from_seconds(
              60 - td.dots.void_entropy->remains().total_seconds() +
              nextTick.total_seconds() ) );
        }

        td.dots.void_entropy->time_to_tick = nextTick;

        priest.procs.void_entropy_extensions->occur();
      }
    }
  }

  bool ready() override
  {
    // WoD Alpha 2014/06/19 added by twintop
    if ( priest.resources.current[ RESOURCE_SHADOW_ORB ] < 3.0 )
      return false;

    return priest_spell_t::ready();
  }
};

// Mind Flay Spell ==========================================================
template <bool insanity = false>
struct mind_flay_base_t : public priest_spell_t
{
  mind_flay_base_t( priest_t& p, const std::string& options_str,
                    const std::string& name = "mind_flay" )
    : priest_spell_t(
          name, p, p.find_class_spell( insanity ? "Insanity" : "Mind Flay" ) )
  {
    parse_options( options_str );

    may_crit      = false;
    channeled     = true;
    hasted_ticks  = false;
    use_off_gcd   = true;
    is_mind_spell = true;

    if ( priest.perks.enhanced_mind_flay->ok() )
    {
      base_tick_time *=
          1.0 + priest.perks.enhanced_mind_flay->effectN( 1 ).percent();
    }
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.mental_anguish->ok() )
    {
      am *= 1.0 + priest.cache.mastery_value();
    }

    return am;
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );
    priest.buffs.glyph_of_mind_flay->trigger();

    if ( priest.active_items.mental_fatigue )
    {
      if ( d->state && result_is_hit( d->state->result ) )
      {
        // Assumes trigger on hit, not on damage
        priest_td_t& td = get_td( d->state->target );
        td.buffs.mental_fatigue->trigger();
      }
    }
  }
};

struct insanity_t : public mind_flay_base_t<true>
{
  typedef mind_flay_base_t<true> base_t;

  insanity_t( priest_t& p, const std::string& options_str )
    : base_t( p, options_str, "insanity" )
  {
  }

  double composite_persistent_multiplier(
      const action_state_t* s ) const override
  {
    double am = base_t::composite_persistent_multiplier( s );

    am *= 1.0 + priest.talents.insanity->effectN( 1 ).percent();

    return am;
  }

  bool ready() override
  {
    if ( !priest.buffs.insanity->up() )
      return false;

    return base_t::ready();
  }
};

// Shadow Word Pain Spell ===================================================

struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_pain", p,
                      p.find_class_spell( "Shadow Word: Pain" ) )
  {
    parse_options( options_str );

    may_crit  = true;
    tick_zero = false;

    base_multiplier *=
        1.0 + p.sets.set( SET_CASTER, T13, B4 )->effectN( 1 ).percent();

    base_crit += p.sets.set( SET_CASTER, T14, B2 )->effectN( 1 ).percent();

    dot_duration +=
        p.sets.set( SET_CASTER, T14, B4 )->effectN( 1 ).time_value();

    if ( priest.specs.shadowy_apparitions->ok() &&
         !priest.active_spells.shadowy_apparitions )
    {
      priest.active_spells.shadowy_apparitions =
          new shadowy_apparition_spell_t( p );
      if ( !priest.sets.has_set_bonus( SET_CASTER, T15, B4 ) )
      {
        // If SW:P is the only action having SA, then we can add it as a
        // child
        // stat
        add_child( priest.active_spells.shadowy_apparitions );
      }
    }
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
      if ( trigger_shadowy_insight() )
        priest.procs.shadowy_insight_from_shadow_word_pain->occur();
  }
};

// Vampiric Embrace Spell ===================================================

struct vampiric_embrace_t : public priest_spell_t
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

// Vampiric Touch Spell =====================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "vampiric_touch", p,
                      p.find_class_spell( "Vampiric Touch" ) )
  {
    parse_options( options_str );
    may_crit = false;

    dot_duration +=
        p.sets.set( SET_CASTER, T14, B4 )->effectN( 1 ).time_value();

    if ( priest.specs.shadowy_apparitions->ok() &&
         priest.sets.has_set_bonus( SET_CASTER, T15, B4 ) &&
         !priest.active_spells.shadowy_apparitions )
    {
      priest.active_spells.shadowy_apparitions =
          new shadowy_apparition_spell_t( p );
    }
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( trigger_surge_of_darkness() )
      priest.procs.surge_of_darkness_from_vampiric_touch->occur();

    if ( priest.sets.has_set_bonus( SET_CASTER, T15, B4 ) )
    {
      if ( priest.active_spells.shadowy_apparitions &&
           ( d->state->result_amount > 0 ) )
      {
        if ( rng().roll(
                 priest.sets.set( SET_CASTER, T15, B4 )->proc_chance() ) )
        {
          priest.procs.t15_4pc_caster->occur();

          priest.active_spells.shadowy_apparitions->trigger();
        }
      }
    }
  }
};

// Holy Fire Base =====================================================

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

    can_trigger_atonement = priest.specs.atonement->ok();

    range += priest.glyphs.holy_fire->effectN( 1 ).base_value();
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

  double cost() const override
  {
    double c = priest_spell_t::cost();

    if ( priest.buffs.chakra_chastise->check() )
      c *= 1.0 + priest.buffs.chakra_chastise->data().effectN( 3 ).percent();

    return c;
  }

  void impact( action_state_t* s ) override
  {
    if ( backlash && priest.glyphs.inquisitor->ok() &&
         ( s->result == RESULT_HIT || s->result == RESULT_CRIT ) )
    {
      backlash->spellpower = s->composite_spell_power();
      backlash->multiplier = s->da_multiplier;

      if ( s->result == RESULT_CRIT )
        backlash->critical = true;

      backlash->schedule_execute();
    }

    priest_spell_t::impact( s );
  }
};

// Power Word: Solace Spell =================================================

struct power_word_solace_t : public holy_fire_base_t
{
  power_word_solace_t( priest_t& player, const std::string& options_str )
    : holy_fire_base_t( "power_word_solace", player,
                        player.find_spell( 129250 ) )
  {
    parse_options( options_str );

    travel_speed = 0.0;  // DBC has default travel taking 54seconds.....

    can_trigger_atonement = true;  // works for both disc and holy
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

// Holy Fire Spell ==========================================================

struct holy_fire_t : public holy_fire_base_t
{
  holy_fire_t( priest_t& player, const std::string& options_str )
    : holy_fire_base_t( "holy_fire", player,
                        player.find_class_spell( "Holy Fire" ) )
  {
    parse_options( options_str );
  }
};

// Penance Spell ============================================================

struct penance_t : public priest_spell_t
{
  struct penance_tick_t : public priest_spell_t
  {
    penance_tick_t( priest_t& p, stats_t* stats )
      : priest_spell_t( "penance_tick", p, p.find_spell( 47666 ) )
    {
      background            = true;
      dual                  = true;
      direct_tick           = true;
      can_trigger_atonement = priest.specs.atonement->ok();

      this->stats = stats;
    }

    void init() override
    {
      priest_spell_t::init();
      if ( atonement )
        atonement->stats = player->get_stats( "atonement_penance" );
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

    may_crit               = false;
    may_miss               = false;
    channeled              = true;
    tick_zero              = true;
    dot_duration           = timespan_t::from_seconds( 2.0 );
    base_tick_time         = timespan_t::from_seconds( 1.0 );
    hasted_ticks           = false;
    castable_in_shadowform = false;

    // HACK: Set can_trigger here even though the tick spell actually
    // does the triggering. We want atonement_penance to be created in
    // priest_heal_t::init() so that it appears in the report.
    can_trigger_atonement = priest.specs.atonement->ok();

    cooldown->duration =
        data().cooldown() +
        p.sets.set( SET_HEALER, T14, B4 )->effectN( 1 ).time_value();

    dot_duration += priest.sets.set( PRIEST_DISCIPLINE, T17, B2 )
                        ->effectN( 1 )
                        .time_value();

    dynamic_tick_action = true;
    tick_action         = new penance_tick_t( p, stats );
  }

  void init() override
  {
    priest_spell_t::init();
    if ( atonement )
      atonement->channeled = true;
  }

  bool usable_moving() const override
  {
    return priest.glyphs.penance->ok();
  }

  double cost() const override
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + priest.glyphs.penance->effectN( 1 ).percent();

    return c;
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

// Smite Spell ==============================================================

struct smite_t : public priest_spell_t
{
  struct state_t : public action_state_t
  {
    bool glyph_benefit;
    state_t( action_t* a, player_t* t )
      : action_state_t( a, t ), glyph_benefit( false )
    {
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s ) << " glyph_benefit=" << std::boolalpha
                                     << glyph_benefit;
      return s;
    }

    void initialize() override
    {
      action_state_t::initialize();
      glyph_benefit = false;
    }

    void copy_state( const action_state_t* o ) override
    {
      action_state_t::copy_state( o );
      glyph_benefit = static_cast<const state_t&>( *o ).glyph_benefit;
    }
  };

  cooldown_t* hw_chastise;

  smite_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "smite", p, p.find_class_spell( "Smite" ) ),
      hw_chastise( p.get_cooldown( "holy_word_chastise" ) )
  {
    parse_options( options_str );

    procs_courageous_primal_diamond = false;

    can_trigger_atonement  = priest.specs.atonement->ok();
    castable_in_shadowform = false;

    range += priest.glyphs.holy_fire->effectN( 1 ).base_value();
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.holy_evangelism->trigger();
    priest.buffs.surge_of_light->trigger();

    trigger_surge_of_light();
  }

  void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );
    assert( execute_state );
    priest.benefits.smites_with_glyph_increase->update(
        static_cast<const state_t*>( execute_state )->glyph_benefit );
  }

  /* Check if Holy Fire or PW: Solace is up
   */
  bool glyph_benefit( player_t* t ) const
  {
    bool glyph_benefit = false;

    if ( priest.glyphs.smite->ok() )
    {
      const priest_td_t& td = get_td( t );

      const dot_t* dot = priest.talents.power_word_solace->ok()
                             ? td.dots.power_word_solace
                             : td.dots.holy_fire;

      glyph_benefit = dot->is_ticking();
    }

    return glyph_benefit;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = priest_spell_t::composite_target_multiplier( target );

    if ( glyph_benefit( target ) )
      m *= 1.0 + priest.glyphs.smite->effectN( 1 ).percent();

    return m;
  }

  action_state_t* new_state() override
  {
    return new state_t( this, target );
  }

  void snapshot_state( action_state_t* s, dmg_e type ) override
  {
    state_t& state = static_cast<state_t&>( *s );

    state.glyph_benefit = glyph_benefit( s->target );

    priest_spell_t::snapshot_state( s, type );
  }

  void trigger_atonement( dmg_e type, action_state_t* s )
  {
    double atonement_dmg = s->result_amount;  // Normal dmg

    // If HF/PW:S was up, remove the extra damage from glyph
    if ( static_cast<const state_t*>( s )->glyph_benefit )
      atonement_dmg /= 1.0 + priest.glyphs.smite->effectN( 1 ).percent();

    atonement->trigger( atonement_dmg, direct_tick ? DMG_OVER_TIME : type,
                        s->result );
  }

  double cost() const override
  {
    double c = priest_spell_t::cost();

    if ( priest.buffs.chakra_chastise->check() )
      c *= 1.0 + priest.buffs.chakra_chastise->data().effectN( 3 ).percent();

    return c;
  }
};

// Cascade Spell

// TODO: Have Cascade pick the next target per bounce based on current distance
// instead of feeding it a list of all available targets.
template <class Base>
struct cascade_base_t : public Base
{
private:
  typedef Base ab;  // typedef for the templated action type, priest_spell_t, or
                    // priest_heal_t
public:
  typedef cascade_base_t base_t;  // typedef for cascade_base_t<action_base_t>
  struct cascade_state_t : action_state_t
  {
    int jump_counter;
    player_t* source_target;

    cascade_state_t( action_t* a, player_t* t )
      : action_state_t( a, t ), jump_counter( 0 ), source_target( nullptr )
    {
    }
  };

  std::vector<player_t*> targets;  // List of available targets to jump to,
                                   // created once at execute() and static
                                   // during the jump process.

  cascade_base_t( const std::string& n, priest_t& p,
                  const std::string& options_str,
                  const spell_data_t* scaling_data )
    : ab( n, p, p.find_talent_spell( "Cascade" ) )
  {
    ab::parse_options( options_str );

    if ( p.specialization() != PRIEST_SHADOW )
      ab::parse_effect_data( scaling_data->effectN(
          1 ) );  // Parse damage or healing numbers from the scaling spell
    else
      ab::parse_effect_data( scaling_data->effectN( 2 ) );
    ab::school       = scaling_data->get_school_type();
    ab::travel_speed = scaling_data->missile_speed();
    ab::radius       = 40;
    ab::range        = 0;
  }

  action_state_t* new_state() override
  {
    return new cascade_state_t( this, ab::target );
  }

  timespan_t distance_targeting_travel_time( action_state_t* s ) const override
  {
    cascade_state_t* cs = debug_cast<cascade_state_t*>( s );
    if ( cs->source_target )
      return timespan_t::from_seconds(
          cs->source_target->get_player_distance( *s->target ) /
          ab::travel_speed );
    return timespan_t::zero();
  }

  player_t* get_next_player( player_t* currentTarget )
  {
    if ( targets.empty() )
    {
      return nullptr;
    }
    player_t* furthest       = nullptr;
    double furthest_distance = 0;

    // Get target at first position
    for ( auto it = targets.begin(); it != targets.end(); it++ )
    {
      player_t* t = *it;
      if ( t == currentTarget )
        continue;

      if ( currentTarget->sim->distance_targeting_enabled )
      {
        double current_distance = t->get_player_distance( *currentTarget );
        if ( current_distance <= 40 && current_distance > furthest_distance )
        {
          furthest_distance = current_distance;
          furthest          = t;
        }
        continue;
      }
      targets.erase( it );
      return t;
    }

    return furthest;
  }

  virtual void populate_target_list() = 0;

  void execute() override
  {
    // Clear and populate targets list
    targets.clear();
    populate_target_list();

    ab::execute();
  }

  void impact( action_state_t* q ) override
  {
    ab::impact( q );

    cascade_state_t* cs = static_cast<cascade_state_t*>( q );

    assert( ab::data().effectN( 1 ).base_value() < 5 );  // Safety limit
    assert( ab::data().effectN( 2 ).base_value() < 5 );  // Safety limit

    if ( cs->jump_counter < ab::data().effectN( 1 ).base_value() )
    {
      player_t* currentTarget = q->target;
      for ( int i = 0; i < ab::data().effectN( 2 ).base_value(); ++i )
      {
        player_t* t = get_next_player( currentTarget );

        if ( t )
        {
          if ( ab::sim->debug )
            ab::sim->out_debug.printf( "%s action %s jumps to player %s",
                                       ab::player->name(), ab::name(),
                                       t->name() );

          // Copy-Pasted action_t::execute() code. Additionally increasing
          // jump
          // counter by one.
          cascade_state_t* s = debug_cast<cascade_state_t*>( ab::get_state() );
          s->target        = t;
          s->source_target = currentTarget;
          s->n_targets     = 1;
          s->chain_target  = 0;
          s->jump_counter  = cs->jump_counter + 1;
          ab::snapshot_state(
              s, q->target->is_enemy() ? DMG_DIRECT : HEAL_DIRECT );
          s->result = ab::calculate_result( s );

          if ( ab::result_is_hit( s->result ) )
            s->result_amount = calculate_direct_amount( s );

          if ( ab::sim->debug )
            s->debug();

          ab::schedule_travel( s );
        }
      }
    }
    else
    {
      cs->jump_counter = 0;
    }
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    cascade_state_t* cs = debug_cast<cascade_state_t*>( s );
    double cda          = action_t::calculate_direct_amount( s );

    double distance;

    if ( cs->source_target == nullptr )  // Initial bounce
    {
      if ( cs->action->player->sim->distance_targeting_enabled )
        distance = cs->action->player->get_player_distance( *cs->target );
      else
        distance = std::fabs( cs->action->player->current.distance -
                              cs->target->current.distance );
    }
    else
    {
      if ( cs->action->player->sim->distance_targeting_enabled )
        distance = cs->source_target->get_player_distance( *cs->target );
      else
        distance = std::fabs( cs->source_target->current.distance -
                              cs->target->current.distance );
    }

    if ( distance >= 30.0 )
      return cda;

    // Source: Ghostcrawler 20/06/2012;
    // http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97
    // 40% damage at 0 yards, 100% at 30, scaling linearly
    return cda * ( 0.4 + 0.6 * distance / 30.0 );
  }
};

struct cascade_t : public cascade_base_t<priest_spell_t>
{
  cascade_t( priest_t& p, const std::string& options_str )
    : base_t( "cascade", p, options_str, get_spell_data( p ) )
  {
    instant_multistrike = 0;
  }

  void populate_target_list() override
  {
    for ( size_t i = 0; i < target_list().size(); ++i )
    {
      player_t* t;
      if ( priest.specialization() == PRIEST_SHADOW )
        t = target_list()[ i ];
      else
        t = sim->player_no_pet_list[ i ];

      if ( target_list().size() > 1 )
      {
        targets.push_back( t );
        targets.push_back( t );
      }
    }

    if ( priest.specialization() != PRIEST_SHADOW )
    {
      targets.push_back( target );
    }
  }

private:
  const spell_data_t* get_spell_data( priest_t& p ) const
  {
    unsigned id = p.specialization() == PRIEST_SHADOW ? 127628 : 121148;
    return p.find_spell( id );
  }
};

// Halo Spell

// This is the background halo spell which does the actual damage
// Templated so we can base it on priest_spell_t or priest_heal_t
template <class Base>
struct halo_base_t : public Base
{
private:
  typedef Base ab;  // typedef for the templated action type, priest_spell_t, or
                    // priest_heal_t
public:
  typedef halo_base_t base_t;  // typedef for halo_base_t<ab>

  halo_base_t( const std::string& n, priest_t& p, const spell_data_t* s )
    : ab( n, p, s )
  {
    ab::aoe        = -1;
    ab::background = true;

    if ( ab::data().ok() )
    {
      // Reparse the correct effect number, because we have two competing ones
      // (
      // were 2 > 1 always wins out )
      ab::parse_effect_data( ab::data().effectN( 1 ) );
    }
    ab::radius = 30;
    ab::range  = 0;
  }

  timespan_t distance_targeting_travel_time( action_state_t* s ) const override
  {
    return timespan_t::from_seconds(
        s->action->player->get_player_distance( *s->target ) /
        ab::travel_speed );
  }

  double calculate_direct_amount( action_state_t* s ) const override
  {
    double cda = action_t::calculate_direct_amount( s );

    // Source: Ghostcrawler 20/06/2012
    // http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97

    double distance;
    if ( s->action->player->sim->distance_targeting_enabled )
      distance = s->action->player->get_player_distance( *s->target );
    else
      distance =
          std::fabs( s->action->player->current.distance -
                     s->target->current
                         .distance );  // Distance from the caster to the target

    // double mult = 0.5 * pow( 1.01, -1 * pow( ( distance - 25 ) / 2, 4 ) ) +
    // 0.1 + 0.015 * distance;
    double mult = 0.5 * exp( -0.00995 * pow( distance / 2 - 12.5, 4 ) ) + 0.1 +
                  0.015 * distance;

    return cda * mult;
  }
};

struct halo_t : public priest_spell_t
{
  halo_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "halo", p, p.talents.halo ),
      _base_spell( get_base_spell( p ) )
  {
    parse_options( options_str );
    instant_multistrike = 0;

    add_child( _base_spell );
  }

  void execute() override
  {
    priest_spell_t::execute();

    _base_spell->execute();
  }

private:
  action_t* _base_spell;

  action_t* get_base_spell( priest_t& p ) const
  {
    if ( p.specialization() == PRIEST_SHADOW )
    {
      return new halo_base_t<priest_spell_t>( "halo_damage", p,
                                              p.find_spell( 120696 ) );
    }
    else
    {
      return new halo_base_t<priest_heal_t>( "halo_heal", p,
                                             p.find_spell( 120692 ) );
    }
  }
};

// Divine Star spell

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

struct divine_star_t : public priest_spell_t
{
  divine_star_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "divine_star", p, p.talents.divine_star ),
      _base_spell( get_base_spell( p ) )
  {
    parse_options( options_str );

    dot_duration = base_tick_time = timespan_t::zero();
    instant_multistrike = 0;

    add_child( _base_spell );
  }

  void execute() override
  {
    priest_spell_t::execute();

    _base_spell->execute();
  }

  bool usable_moving() const override
  {
    // Holy/Disc version is usable while moving, Shadow version is instant cast
    // anyway.
    return true;
  }

private:
  action_t* _base_spell;

  action_t* get_base_spell( priest_t& p ) const
  {
    if ( priest.specialization() == PRIEST_SHADOW )
    {
      return new divine_star_base_t<priest_spell_t>( "divine_star_damage", p,
                                                     p.find_spell( 122128 ) );
    }
    else
    {
      return new divine_star_base_t<priest_heal_t>( "divine_star_heal", p,
                                                    p.find_spell( 110745 ) );
    }
  }
};

// Void Entropy Spell =======================================================

struct void_entropy_t : public priest_spell_t
{
  void_entropy_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "void_entropy", p, p.talents.void_entropy )
  {
    parse_options( options_str );
    may_crit            = false;
    tick_zero           = false;
    instant_multistrike = 0;
  }

  void consume_resource() override
  {
    resource_consumed = cost();

    if ( execute_state->result != RESULT_MISS )
    {
      resource_consumed = shadow_orbs_to_consume();

      trigger_insanity( execute_state, static_cast<int>( resource_consumed ) );

      if ( priest.sets.has_set_bonus( PRIEST_SHADOW, T17, B2 ) )
      {
        if ( priest.cooldowns.mind_blast->remains() == timespan_t::zero() )
        {
          priest.procs.t17_2pc_caster_mind_blast_reset_overflow->occur();
        }
        else
        {
          priest.procs.t17_2pc_caster_mind_blast_reset->occur();
        }

        priest.cooldowns.mind_blast->adjust(
            -timespan_t::from_seconds( resource_consumed *
                                       priest.sets.set( PRIEST_SHADOW, T17, B2 )
                                           ->effectN( 1 )
                                           .time_value()
                                           .total_seconds() ),
            true );
      }
    }

    player->resource_loss( current_resource(), resource_consumed, nullptr,
                           this );

    if ( sim->log )
      sim->out_log.printf(
          "%s consumes %.1f %s for %s (%.0f)", player->name(),
          resource_consumed, util::resource_type_string( current_resource() ),
          name(), player->resources.current[ current_resource() ] );

    stats->consume_resource( current_resource(), resource_consumed );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
  }

  bool ready() override
  {
    // Only usable with 3 orbs, per Celestalon in Theorycraft thread - Twintop
    // 2014/07/27
    if ( priest.resources.current[ RESOURCE_SHADOW_ORB ] < 3.0 )
      return false;

    return priest_spell_t::ready();
  }
};

// Silence Spell =======================================================

struct silence_t : public priest_spell_t
{
  silence_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "silence", player, player.find_class_spell( "Silence" ) )
  {
    parse_options( options_str );
    may_miss = may_crit = false;
    ignore_false_positive = true;

    cooldown           = priest.cooldowns.silence;
    cooldown->duration = data().cooldown();

    // Glyph of Silence
    if ( priest.specialization() == PRIEST_SHADOW &&
         priest.glyphs.silence->ok() )
    {
      cooldown->duration += priest.glyphs.silence->effectN( 1 ).time_value();
    }
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

}  // NAMESPACE spells

// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================

namespace heals
{
// Binding Heal Spell =======================================================

struct binding_heal_t : public priest_heal_t
{
  binding_heal_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "binding_heal", p, p.find_class_spell( "Binding Heal" ) )
  {
    parse_options( options_str );

    aoe = 2;
  }

  void init() override
  {
    priest_heal_t::init();

    target_cache.list.clear();
    target_cache.list.push_back( target );
    target_cache.list.push_back( player );
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    assert( tl.size() == 2 );

    if ( tl[ 0 ] != target )
      tl[ 0 ] = target;

    return tl.size();
  }

  void execute() override
  {
    priest_heal_t::execute();

    trigger_serendipity();
    trigger_surge_of_light();
  }
};

// Circle of Healing ========================================================

struct circle_of_healing_t : public priest_heal_t
{
  circle_of_healing_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "circle_of_healing", p,
                     p.find_class_spell( "Circle of Healing" ) )
  {
    parse_options( options_str );

    base_costs[ current_resource() ] *=
        1.0 + p.glyphs.circle_of_healing->effectN( 2 ).percent();
    base_costs[ current_resource() ] =
        floor( base_costs[ current_resource() ] );
    aoe = p.glyphs.circle_of_healing->ok() ? 6 : 5;
  }

  void execute() override
  {
    cooldown->duration = data().cooldown();
    cooldown->duration +=
        priest.sets.set( SET_HEALER, T14, B4 )->effectN( 2 ).time_value();
    if ( priest.buffs.chakra_sanctuary->up() )
      cooldown->duration +=
          priest.buffs.chakra_sanctuary->data().effectN( 2 ).time_value();

    // Choose Heal Target
    target = find_lowest_player();

    priest_heal_t::execute();

    priest.buffs.absolution->trigger();
    trigger_surge_of_light();
  }

  double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary->up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary->data().effectN( 1 ).percent();

    return am;
  }
};

// Desperate Prayer =========================================================

// TODO: Check and see if Desperate Prayer can trigger Surge of Light for Holy
struct desperate_prayer_t : public priest_heal_t
{
  desperate_prayer_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "desperate_prayer", p, p.talents.desperate_prayer )
  {
    parse_options( options_str );

    target = &p;  // always targets the priest himself
  }
};

// Divine Hymn Spell ========================================================

struct divine_hymn_tick_t : public priest_heal_t
{
  divine_hymn_tick_t( priest_t& player, int nr_targets )
    : priest_heal_t( "divine_hymn_tick", player, player.find_spell( 64844 ) )
  {
    background = true;

    aoe = nr_targets;
  }
};

struct divine_hymn_t : public priest_heal_t
{
  divine_hymn_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "divine_hymn", p, p.find_class_spell( "Divine Hymn" ) )
  {
    parse_options( options_str );

    harmful             = false;
    channeled           = true;
    dynamic_tick_action = true;

    tick_action = new divine_hymn_tick_t( p, data().effectN( 2 ).base_value() );
    add_child( tick_action );
  }

  double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary->up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary->data().effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    priest_heal_t::execute();

    trigger_surge_of_light();
  }
};

// Echo of Light

struct echo_of_light_t
    : public residual_action::residual_periodic_action_t<priest_heal_t>
{
  echo_of_light_t( priest_t& p )
    : base_t( "echo_of_light", p, p.find_spell( 77489 ) )
  {
    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_duration   = data().duration();
  }
};

// Flash Heal Spell =========================================================

struct flash_heal_t : public priest_heal_t
{
  flash_heal_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "flash_heal", p, p.find_class_spell( "Flash Heal" ) )
  {
    parse_options( options_str );
    can_trigger_spirit_shell = true;

    castable_in_shadowform = false;
  }

  void execute() override
  {
    priest_heal_t::execute();

    trigger_serendipity();
    consume_surge_of_light();
    trigger_surge_of_light();
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    if ( !priest.buffs.spirit_shell->check() )
      trigger_strength_of_soul( s->target );

    trigger_naarus_discipline( s );
  }

  timespan_t execute_time() const override
  {
    if ( priest.buffs.surge_of_light->check() )
      return timespan_t::zero();

    return priest_heal_t::execute_time();
  }

  double cost() const override
  {
    if ( priest.buffs.surge_of_light->check() )
      return 0.0;

    double c = priest_heal_t::cost();

    c *= 1.0 + priest.sets.set( SET_HEALER, T14, B2 )->effectN( 1 ).percent();

    return c;
  }
};

// Guardian Spirit ==========================================================

struct guardian_spirit_t : public priest_heal_t
{
  guardian_spirit_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "guardian_spirit", p,
                     p.find_class_spell( "Guardian Spirit" ) )
  {
    parse_options( options_str );

    base_dd_min = base_dd_max = 0.0;  // The absorb listed isn't a real absorb
    harmful = false;
  }

  void execute() override
  {
    priest_heal_t::execute();
    target->buffs.guardian_spirit->trigger();
  }
};

// Heal Spell =======================================================

// starts with underscore because of name conflict with heal_t

struct _heal_t : public priest_heal_t
{
  _heal_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "heal", p, p.find_class_spell( "Heal" ) )
  {
    parse_options( options_str );
    can_trigger_spirit_shell = true;
  }

  void execute() override
  {
    priest_heal_t::execute();

    consume_serendipity();
    trigger_surge_of_light();
    trigger_divine_insight();
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    if ( !priest.buffs.spirit_shell->check() )
      trigger_strength_of_soul( s->target );

    trigger_naarus_discipline( s );
  }

  double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    am *= 1.0 +
          priest.sets.set( SET_HEALER, T16, B2 )->effectN( 1 ).percent() *
              priest.buffs.serendipity->check();

    if ( priest.active_items.complete_healing &&
         priest.buffs.chakra_sanctuary->check() )
    {
      const item_t* item = priest.active_items.complete_healing->item;
      am *=
          1.0 +
          priest.active_items.complete_healing->driver()->effectN( 3 ).average(
              item ) /
              100.0;
    }

    return am;
  }

  double cost() const override
  {
    double c = priest_heal_t::cost();

    if ( priest.buffs.serendipity->check() )
      c *= 1.0 +
           priest.buffs.serendipity->check() *
               priest.buffs.serendipity->data().effectN( 2 ).percent();

    if ( priest.active_items.complete_healing &&
         priest.buffs.chakra_sanctuary->check() )
    {
      const item_t* item = priest.active_items.complete_healing->item;
      c *= 1.0 +
           priest.active_items.complete_healing->driver()->effectN( 1 ).average(
               item ) /
               100.0;
    }

    return c;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = priest_heal_t::execute_time();

    if ( priest.buffs.serendipity->check() )
      et *= 1.0 +
            priest.buffs.serendipity->check() *
                priest.buffs.serendipity->data().effectN( 1 ).percent();

    if ( priest.active_items.complete_healing &&
         priest.buffs.chakra_sanctuary->check() )
    {
      const item_t* item = priest.active_items.complete_healing->item;
      et *=
          1.0 +
          priest.active_items.complete_healing->driver()->effectN( 1 ).average(
              item ) /
              100.0;
    }

    return et;
  }
};

// Holy Word Sanctuary ======================================================

struct holy_word_sanctuary_t : public priest_heal_t
{
  struct holy_word_sanctuary_tick_t : public priest_heal_t
  {
    holy_word_sanctuary_tick_t( priest_t& player )
      : priest_heal_t( "holy_word_sanctuary_tick", player,
                       player.find_spell( 88686 ) )
    {
      dual        = true;
      background  = true;
      direct_tick = true;
      can_trigger_EoL =
          false;  // http://us.battle.net/wow/en/forum/topic/5889309137?page=107#2137
    }

    double action_multiplier() const override
    {
      double am = priest_heal_t::action_multiplier();

      if ( priest.buffs.chakra_sanctuary->up() )
        am *=
            1.0 + priest.buffs.chakra_sanctuary->data().effectN( 1 ).percent();

      // Spell data from the buff ( 15% ) is correct, not the one in the
      // triggering spell ( 50% )
      am *= 1.0 +
            priest.buffs.absolution->check() *
                priest.buffs.absolution->data().effectN( 1 ).percent();

      return am;
    }
  };

  holy_word_sanctuary_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "holy_word_sanctuary", p, p.find_spell( 88685 ) )
  {
    parse_options( options_str );

    may_crit = false;

    base_tick_time = timespan_t::from_seconds( 2.0 );
    dot_duration   = timespan_t::from_seconds( 18.0 );

    tick_action = new holy_word_sanctuary_tick_t( p );

    // Needs testing
    cooldown->duration *=
        1.0 + p.sets.has_set_bonus( SET_HEALER, T13, B4 ) * -0.2;
  }

  void execute() override
  {
    priest_heal_t::execute();

    priest.buffs.absolution->expire();

    trigger_surge_of_light();
  }

  bool ready() override
  {
    if ( !priest.buffs.chakra_sanctuary->check() )
      return false;

    return priest_heal_t::ready();
  }
  // HW: Sanctuary is treated as a instant cast spell, both affected by Inner
  // Will and Mental Agility
};

// Holy Word Chastise =======================================================

struct holy_word_chastise_t : public priest_spell_t
{
  holy_word_chastise_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "holy_word_chastise", p,
                      p.find_class_spell( "Holy Word: Chastise" ) )
  {
    parse_options( options_str );

    base_costs[ current_resource() ] =
        floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown->duration *=
        1.0 + p.sets.has_set_bonus( SET_HEALER, T13, B4 ) * -0.2;

    castable_in_shadowform = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.absolution->expire();
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    // Spell data from the buff ( 15% ) is correct, not the one in the
    // triggering spell ( 50% )
    am *= 1.0 +
          priest.buffs.absolution->check() *
              priest.buffs.absolution->data().effectN( 1 ).percent();

    return am;
  }

  bool ready() override
  {
    if ( priest.buffs.chakra_sanctuary->check() )
      return false;

    if ( priest.buffs.chakra_serenity->check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Holy Word Serenity =======================================================

struct holy_word_serenity_t : public priest_heal_t
{
  holy_word_serenity_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "holy_word_serenity", p, p.find_spell( 88684 ) )
  {
    parse_options( options_str );

    base_costs[ current_resource() ] =
        floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown->duration =
        data().cooldown() *
        ( 1.0 + p.sets.has_set_bonus( SET_HEALER, T13, B4 ) * -0.2 );
  }

  void execute() override
  {
    priest_heal_t::execute();

    priest.buffs.absolution->expire();

    trigger_surge_of_light();
  }

  double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    // Spell data from the buff ( 15% ) is correct, not the one in the
    // triggering spell ( 50% )
    am *= 1.0 +
          priest.buffs.absolution->check() *
              priest.buffs.absolution->data().effectN( 1 ).percent();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    get_td( s->target ).buffs.holy_word_serenity->trigger();
  }

  bool ready() override
  {
    if ( !priest.buffs.chakra_serenity->check() )
      return false;

    return priest_heal_t::ready();
  }
};

// Holy Word ================================================================

struct holy_word_t : public priest_spell_t
{
  holy_word_sanctuary_t* hw_sanctuary;
  holy_word_chastise_t* hw_chastise;
  holy_word_serenity_t* hw_serenity;

  holy_word_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "holy_word", p, spell_data_t::nil() ),
      hw_sanctuary( new holy_word_sanctuary_t( p, options_str ) ),
      hw_chastise( new holy_word_chastise_t( p, options_str ) ),
      hw_serenity( new holy_word_serenity_t( p, options_str ) )
  {
    school = SCHOOL_HOLY;

    castable_in_shadowform = false;
  }

  void init() override
  {
    priest_spell_t::init();

    hw_sanctuary->action_list = action_list;
    hw_chastise->action_list  = action_list;
    hw_serenity->action_list  = action_list;
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    action_t* a;

    if ( priest.buffs.chakra_serenity->up() )
      a = hw_serenity;
    else if ( priest.buffs.chakra_sanctuary->up() )
      a = hw_sanctuary;
    else
      a = hw_chastise;

    player->last_foreground_action = a;
    a->schedule_execute( state );
  }

  void execute() override
  {
    assert( false );
  }

  bool ready() override
  {
    if ( priest.buffs.chakra_serenity->check() )
      return hw_serenity->ready();

    else if ( priest.buffs.chakra_sanctuary->check() )
      return hw_sanctuary->ready();

    else
      return hw_chastise->ready();
  }
};

/* Lightwell Spell
 * Create only if ( p.pets.lightwell )
 */
struct lightwell_t : public priest_spell_t
{
  timespan_t consume_interval;
  cooldown_t* lightwell_renew_cd;

  lightwell_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "lightwell", p, p.find_class_spell( "Lightwell" ) ),
      consume_interval( timespan_t::from_seconds( 10 ) ),
      lightwell_renew_cd( nullptr )
  {
    add_option( opt_timespan( "consume_interval", consume_interval ) );
    parse_options( options_str );

    harmful = false;

    castable_in_shadowform = false;

    assert( consume_interval > timespan_t::zero() &&
            consume_interval < cooldown->duration );
  }

  bool init_finished() override
  {
    lightwell_renew_cd =
        priest.pets.lightwell->get_cooldown( "lightwell_renew" );

    return priest_spell_t::init_finished();
  }

  void execute() override
  {
    priest_spell_t::execute();

    lightwell_renew_cd->duration = consume_interval;
    priest.pets.lightwell->summon( data().duration() );
  }
};

// Penance Heal Spell =======================================================

struct penance_heal_t : public priest_heal_t
{
  struct penance_heal_tick_t : public priest_heal_t
  {
    penance_heal_tick_t( priest_t& player )
      : priest_heal_t( "penance_heal_tick", player, player.find_spell( 47750 ) )
    {
      background  = true;
      may_crit    = true;
      dual        = true;
      direct_tick = true;

      school = SCHOOL_HOLY;
      stats  = player.get_stats( "penance_heal", this );
    }

    void init() override
    {
      priest_heal_t::init();

      snapshot_flags |= STATE_MUL_TA;
    }

    void impact( action_state_t* state ) override
    {
      priest_heal_t::impact( state );

      if ( result_is_hit( state->result ) )
      {
        // TODO: Check if this is the correct place & check for triggering.
        // 2015-04-14: Your Penance [...] each time it deals damage or heals.
        priest.buffs.reperation->trigger();
      }

      trigger_naarus_discipline( state );
    }
  };

  penance_heal_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "penance_heal", p, p.find_class_spell( "Penance" ) )
  {
    parse_options( options_str );

    may_crit            = false;
    channeled           = true;
    tick_zero           = true;
    dot_duration        = timespan_t::from_seconds( 2.0 );
    base_tick_time      = timespan_t::from_seconds( 1.0 );
    hasted_ticks        = false;
    dynamic_tick_action = true;

    cooldown = p.cooldowns.penance;
    cooldown->duration =
        data().cooldown() +
        p.sets.set( SET_HEALER, T14, B4 )->effectN( 1 ).time_value();

    tick_action = new penance_heal_tick_t( p );
  }

  double cost() const override
  {
    double c = priest_heal_t::cost();

    c *= 1.0 + priest.glyphs.penance->effectN( 1 ).percent();

    return c;
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    if ( !priest.buffs.spirit_shell->check() )
      trigger_strength_of_soul( s->target );
  }
};

// Power Word: Shield Spell =================================================

struct power_word_shield_t : public priest_absorb_t
{
  struct glyph_power_word_shield_t : public priest_heal_t
  {
    glyph_power_word_shield_t( priest_t& player )
      : priest_heal_t( "power_word_shield_glyph", player,
                       player.find_spell( 55672 ) )
    {
      school     = SCHOOL_HOLY;
      background = true;
      proc       = true;

      snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;
      castable_in_shadowform = true;
    }

    void trigger( action_state_t* s )
    {
      base_dd_min = base_dd_max =
          priest.glyphs.power_word_shield->effectN( 1 ).percent() *
          s->result_amount;
      target = s->target;
      execute();
    }
  };

  glyph_power_word_shield_t* glyph_pws;
  bool ignore_debuff;

  power_word_shield_t( priest_t& p, const std::string& options_str )
    : priest_absorb_t( "power_word_shield", p,
                       p.find_class_spell( "Power Word: Shield" ) ),
      glyph_pws( nullptr ),
      ignore_debuff( false )
  {
    add_option( opt_bool( "ignore_debuff", ignore_debuff ) );
    parse_options( options_str );

    spell_power_mod.direct = 4.59;  // last checked 2015/02/21

    if ( p.glyphs.power_word_shield->ok() )
    {
      glyph_pws = new glyph_power_word_shield_t( p );
      // add_child( glyph_pws );
    }

    castable_in_shadowform = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_absorb_t::impact( s );

    buffs::weakened_soul_t* weakened_soul =
        debug_cast<buffs::weakened_soul_t*>( s->target->buffs.weakened_soul );
    weakened_soul->trigger_weakened_souls( priest );
    priest.buffs.borrowed_time->trigger();

    // Talent
    if ( priest.talents.body_and_soul->ok() )
      s->target->buffs.body_and_soul->trigger();

    // Glyph
    if ( glyph_pws )
      glyph_pws->trigger( s );
  }

  bool ready() override
  {
    if ( !ignore_debuff && target->buffs.weakened_soul->check() )
      return false;

    return priest_absorb_t::ready();
  }
};

// Prayer of Healing Spell ==================================================

struct prayer_of_healing_t : public priest_heal_t
{
  prayer_of_healing_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "prayer_of_healing", p,
                     p.find_class_spell( "Prayer of Healing" ) )
  {
    parse_options( options_str );
    aoe                      = 5;
    group_only               = true;
    can_trigger_spirit_shell = true;
  }

  void execute() override
  {
    priest_heal_t::execute();

    consume_serendipity();
    trigger_surge_of_light();
    trigger_divine_insight();
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    trigger_naarus_discipline( s );
  }
  double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary->up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary->data().effectN( 1 ).percent();

    am *= 1.0 +
          priest.sets.set( SET_HEALER, T16, B2 )->effectN( 1 ).percent() *
              priest.buffs.serendipity->check();

    return am;
  }

  double cost() const override
  {
    double c = priest_heal_t::cost();

    if ( priest.buffs.clear_thoughts->check() )
      c *= 1.0 +
           priest.buffs.clear_thoughts->check() *
               priest.buffs.clear_thoughts->data().effectN( 1 ).percent();

    if ( priest.buffs.serendipity->check() )
      c *= 1.0 +
           priest.buffs.serendipity->check() *
               priest.buffs.serendipity->data().effectN( 2 ).percent();

    return c;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = priest_heal_t::execute_time();

    if ( priest.buffs.serendipity->check() )
      et *= 1.0 +
            priest.buffs.serendipity->check() *
                priest.buffs.serendipity->data().effectN( 1 ).percent();

    return et;
  }

  bool ready() override
  {
    if ( priest.talents.clarity_of_purpose->ok() )
      return false;  // Clarity of Purpose replaces Prayer of Healing

    return heal_t::ready();
  }
};

// Prayer of Mending Spell ==================================================

struct prayer_of_mending_t : public priest_heal_t
{
  bool single;

  prayer_of_mending_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "prayer_of_mending", p,
                     p.find_class_spell( "Prayer of Mending" ) ),
      single( false )
  {
    add_option( opt_bool( "single", single ) );
    parse_options( options_str );

    spell_power_mod.direct = 0.666;  // last checked 2015/02/21
    base_dd_min = base_dd_max = data().effectN( 1 ).min( &p );

    aoe = 5;

    if ( priest.sets.has_set_bonus( PRIEST_HOLY, T17, B2 ) )
      aoe +=
          (int)priest.sets.set( PRIEST_HOLY, T17, B2 )->effectN( 1 ).percent() *
          100;

    castable_in_shadowform = false;
  }

  void execute() override
  {
    priest_heal_t::execute();

    priest.buffs.absolution->trigger();
    trigger_surge_of_light();
    trigger_serendipity( true );
  }

  double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary->up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary->data().effectN( 1 ).percent();

    return am;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = priest_heal_t::composite_target_multiplier( t );

    if ( priest.glyphs.prayer_of_mending->ok() && t == target )
      ctm *= 1.0 + priest.glyphs.prayer_of_mending->effectN( 1 ).percent();

    return ctm;
  }

  void update_ready( timespan_t cd_duration ) override
  {
    // If divine insight holy is up, don't trigger a cooldown
    if ( !priest.buffs.divine_insight->up() )
    {
      priest_heal_t::update_ready( cd_duration );
    }
  }
};

// Renew Spell ==============================================================

struct renew_t : public priest_heal_t
{
  struct rapid_renewal_t : public priest_heal_t
  {
    rapid_renewal_t( priest_t& p )
      : priest_heal_t( "rapid_renewal", p, p.specs.rapid_renewal )
    {
      background = true;
      proc       = true;
    }

    void trigger( action_state_t* s, double /* amount */ )
    {
      target = s->target;
      execute();
      trigger_surge_of_light();
    }

    double composite_da_multiplier(
        const action_state_t* /* state */ ) const override
    {
      return 1.0;
    }
  };
  rapid_renewal_t* rr;

  renew_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "renew", p, p.find_class_spell( "Renew" ) ), rr( nullptr )
  {
    parse_options( options_str );

    may_crit = false;

    if ( p.specs.rapid_renewal->ok() )
    {
      rr = new rapid_renewal_t( p );
      add_child( rr );
      base_multiplier *= 1.0 + p.specs.rapid_renewal->effectN( 2 ).percent();
    }

    base_multiplier *= 1.0 + p.glyphs.renew->effectN( 1 ).percent();
    dot_duration += p.glyphs.renew->effectN( 2 ).time_value();

    dot_duration += priest.perks.enhanced_renew->effectN( 1 ).time_value();

    castable_in_shadowform = true;
  }

  double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary->up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary->data().effectN( 1 ).percent();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    if ( rr )
    {
      dot_t* d         = get_dot( s->target );
      result_e r       = d->state->result;
      d->state->result = RESULT_HIT;
      double tick_dmg  = calculate_tick_amount( d->state, 1.0 );
      d->state->result = r;
      tick_dmg *=
          d->ticks_left();  // Gets multiplied by the hasted amount of ticks
      rr->trigger( s, tick_dmg );
    }
  }
};

struct clarity_of_will_t : public priest_absorb_t
{
  clarity_of_will_t( priest_t& p, const std::string& options_str )
    : priest_absorb_t( "clarity_of_will", p, p.talents.clarity_of_will )
  {
    parse_options( options_str );

    spell_power_mod.direct = 6.6;  // Last checked 2015/02/21

    // TODO: implement buff value overflow of 75% of casting priest health.
    // 15/02/14
  }
};

struct clarity_of_purpose_t : public priest_heal_t
{
  clarity_of_purpose_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "clarity_of_purpose", p,
                     p.find_spell( 0 /*p.talents.divine_clarity */ ) )
  {
    parse_options( options_str );
    // can_trigger_spirit_shell = true; ?
    // TODO: implement mechanic
  }

  void execute() override
  {
    priest_heal_t::execute();

    trigger_surge_of_light();
  }
};

struct saving_grace_t : public priest_heal_t
{
  saving_grace_t( priest_t& p, const std::string& options_str )
    : priest_heal_t( "saving_grace", p,
                     p.find_spell( 0 /*p.talents.spiritual_guidance */ ) )
  {
    parse_options( options_str );
    // can_trigger_spirit_shell = true; ?
  }

  void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    priest.buffs.saving_grace_penalty->trigger();
  }

  timespan_t execute_time() const override
  {
    timespan_t et = priest_heal_t::execute_time();

    return et;
  }

  void execute() override
  {
    priest_heal_t::execute();

    trigger_surge_of_light();
  }
};

}  // NAMESPACE heals

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
  typedef priest_buff_t base_t;  // typedef for priest_buff_t<buff_base_t>

  priest_buff_t( priest_td_t& p, const buff_creator_basics_t& params )
    : Base( params ), priest( p.priest )
  {
  }

  priest_buff_t( priest_t& p, const buff_creator_basics_t& params )
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

/* Custom shadowform buff
 * trigger/cancels spell haste aura
 */
struct shadowform_t : public priest_buff_t<buff_t>
{
  shadowform_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "shadowform" )
                     .spell( p.find_class_spell( "Shadowform" ) )
                     .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {
  }

  bool trigger( int stacks, double value, double chance,
                timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    if ( !sim->overrides.haste )
      sim->auras.haste->trigger();

    if ( !sim->overrides.multistrike )
      sim->auras.multistrike->trigger();

    return r;
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    if ( !sim->overrides.haste )
      sim->auras.haste->decrement();

    if ( !sim->overrides.multistrike )
      sim->auras.multistrike->decrement();
  }
};

/* Custom archangel buff
 * snapshots evangelism stacks and expires it
 */
struct archangel_t : public priest_buff_t<buff_t>
{
  archangel_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "archangel" )
                     .spell( p.specs.archangel )
                     .max_stack( 5 ) )
  {
    default_value = data().effectN( 1 ).percent();

    if ( priest.sets.has_set_bonus( SET_HEALER, T16, B2 ) )
    {
      add_invalidate( CACHE_CRIT );
    }
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

struct dispersion_t : public priest_buff_t<buff_t>
{
  dispersion_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "dispersion" )
                     .spell( p.find_class_spell( "Dispersion" ) ) )
  {
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    priest.buffs.shadow_power->trigger();
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

struct spirit_shell_t : public priest_buff_t<buff_t>
{
  spirit_shell_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "spirit_shell" )
                     .spell( p.find_class_spell( "Spirit Shell" ) )
                     .cd( timespan_t::zero() ) )
  {
  }

  bool trigger( int stacks, double value, double chance,
                timespan_t duration ) override
  {
    bool success = base_t::trigger( stacks, value, chance, duration );

    if ( success )
    {
      priest.buffs.resolute_spirit->trigger();
    }

    return success;
  }
};

}  // end namespace buffs

namespace items
{
void do_trinket_init( priest_t* player, specialization_e spec,
                      const special_effect_t*& ptr,
                      const special_effect_t& effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from
  // working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player->find_spell( effect.spell_id )->ok() ||
       player->specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is
  // "enabled"
  ptr = &( effect );
}

void discipline_trinket( special_effect_t& effect )
{
  priest_t* s = debug_cast<priest_t*>( effect.player );
  do_trinket_init( s, PRIEST_DISCIPLINE, s->active_items.naarus_discipline,
                   effect );
}

void holy_trinket( special_effect_t& effect )
{
  priest_t* s = debug_cast<priest_t*>( effect.player );
  do_trinket_init( s, PRIEST_HOLY, s->active_items.complete_healing, effect );
}

void shadow_trinket( special_effect_t& effect )
{
  priest_t* s = debug_cast<priest_t*>( effect.player );
  do_trinket_init( s, PRIEST_SHADOW, s->active_items.mental_fatigue, effect );
}

void init()
{
  unique_gear::register_special_effect( 184912, discipline_trinket );
  unique_gear::register_special_effect( 184914, holy_trinket );
  unique_gear::register_special_effect( 184915, shadow_trinket );
}

}  // items

// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p )
  : actor_target_data_t( target, &p ),
    dots(),
    buffs(),
    glyph_of_mind_harvest_consumed( false ),
    priest( p )
{
  dots.holy_fire             = target->get_dot( "holy_fire", &p );
  dots.power_word_solace     = target->get_dot( "power_word_solace", &p );
  dots.devouring_plague_tick = target->get_dot( "devouring_plague_tick", &p );
  dots.shadow_word_pain      = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch        = target->get_dot( "vampiric_touch", &p );
  dots.void_entropy          = target->get_dot( "void_entropy", &p );
  dots.renew                 = target->get_dot( "renew", &p );

  buffs.divine_aegis =
      absorb_buff_creator_t( *this, "divine_aegis", p.find_spell( 47753 ) )
          .source( p.get_stats( "divine_aegis" ) );

  buffs.spirit_shell = absorb_buff_creator_t( *this, "spirit_shell_absorb" )
                           .spell( p.find_spell( 114908 ) )
                           .source( p.get_stats( "spirit_shell" ) );

  buffs.holy_word_serenity = buff_creator_t( *this, "holy_word_serenity" )
                                 .spell( p.find_spell( 88684 ) )
                                 .cd( timespan_t::zero() )
                                 .activated( false );

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

  target->callbacks_on_demise.push_back(
      std::bind( &priest_td_t::target_demise, this ) );
}

void priest_td_t::reset()
{
  glyph_of_mind_harvest_consumed = false;
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
    perks(),
    cooldowns(),
    gains(),
    benefits(),
    procs(),
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
  cooldowns.angelic_feather   = get_cooldown( "angelic_feather" );
  cooldowns.chakra            = get_cooldown( "chakra" );
  cooldowns.mindbender        = get_cooldown( "mindbender" );
  cooldowns.mind_blast        = get_cooldown( "mind_blast" );
  cooldowns.penance           = get_cooldown( "penance" );
  cooldowns.power_word_shield = get_cooldown( "power_word_shield" );
  cooldowns.shadowfiend       = get_cooldown( "shadowfiend" );
  cooldowns.silence           = get_cooldown( "silence" );

  cooldowns.angelic_feather->charges  = 3;
  cooldowns.angelic_feather->duration = timespan_t::from_seconds( 10.0 );

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
  gains.mindbender              = get_gain( "Mindbender Mana" );
  gains.power_word_solace       = get_gain( "Power Word: Solace Mana" );
  gains.devouring_plague_health = get_gain( "Devouring Plague Health" );
  gains.shadow_orb_auspicious_spirits =
      get_gain( "Shadow Orbs from Auspicious Spirits" );
  gains.shadow_orb_mind_blast = get_gain( "Shadow Orbs from Mind Blast" );
  gains.shadow_orb_mind_harvest =
      get_gain( "Shadow Orbs from Glyph of Mind Harvest" );
  gains.shadow_orb_shadow_word_death =
      get_gain( "Shadow Orbs from Shadow Word: Death" );
}

/* Construct priest procs
 *
 */
void priest_t::create_procs()
{
  procs.shadowy_apparition = get_proc( "Shadowy Apparition Procced" );
  procs.divine_insight = get_proc( "Divine Insight Instant Prayer of Mending" );
  procs.divine_insight_overflow =
      get_proc( "Divine Insight Instant Prayer of Mending lost to overflow" );
  procs.shadowy_insight = get_proc( "Shadowy Insight Mind Blast CD Reset" );
  procs.shadowy_insight_from_mind_spike =
      get_proc( "Shadowy Insight Mind Blast CD Reset from Mind Spike" );
  procs.shadowy_insight_from_shadow_word_pain =
      get_proc( "Shadowy Insight Mind Blast CD Reset from Shadow Word: Pain" );
  procs.shadowy_insight_overflow =
      get_proc( "Shadowy Insight Mind Blast CD Reset lost to overflow" );
  procs.surge_of_darkness = get_proc( "Surge of Darkness" );
  procs.surge_of_darkness_from_devouring_plague =
      get_proc( "Surge of Darkness from Devouring Plague" );
  procs.surge_of_darkness_from_vampiric_touch =
      get_proc( "Surge of Darkness from Vampiric Touch" );
  procs.surge_of_darkness_overflow =
      get_proc( "Surge of Darkness lost to overflow" );
  procs.surge_of_light          = get_proc( "Surge of Light" );
  procs.surge_of_light_overflow = get_proc( "Surge of Light lost to overflow" );
  procs.mind_spike_dot_removal  = get_proc( "Mind Spike removed DoTs" );
  procs.mind_spike_dot_removal_devouring_plague =
      get_proc( "Mind Spike removed Devouring Plague" );
  procs.mind_spike_dot_removal_shadow_word_pain =
      get_proc( "Mind Spike removed Shadow Word: Pain" );
  procs.mind_spike_dot_removal_vampiric_touch =
      get_proc( "Mind Spike removed Vampiric Touch" );
  procs.mind_spike_dot_removal_void_entropy =
      get_proc( "Mind Spike removed Void Entropy" );
  procs.mind_spike_dot_removal_devouring_plague_ticks =
      get_proc( "Devouring Plague ticks lost from Mind Spike removal" );
  procs.mind_spike_dot_removal_shadow_word_pain_ticks =
      get_proc( "Shadow Word: Pain ticks lost from Mind Spike removal" );
  procs.mind_spike_dot_removal_vampiric_touch_ticks =
      get_proc( "Vampiric Touch ticks lost from Mind Spike removal" );
  procs.mind_spike_dot_removal_void_entropy_ticks =
      get_proc( "Void Entropy ticks lost from Mind Spike removal" );
  procs.t15_2pc_caster = get_proc( "Tier15 2pc caster" );
  procs.t15_4pc_caster = get_proc( "Tier15 4pc caster" );
  procs.t15_2pc_caster_shadow_word_pain =
      get_proc( "Tier15 2pc caster Shadow Word: Pain Extra Tick" );
  procs.t15_2pc_caster_vampiric_touch =
      get_proc( "Tier15 2pc caster Vampiric Touch Extra Tick" );
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
  procs.void_entropy_extensions =
      get_proc( "Void Entropy extensions from Devouring Plague casts" );

  procs.sd_mind_spike_dot_removal_devouring_plague_ticks =
      get_sample_data( "Devouring Plague ticks lost from Mind Spike removal" );
}

/* Construct priest benefits
 *
 */
void priest_t::create_benefits()
{
  benefits.smites_with_glyph_increase =
      get_benefit( "Smites increased by Holy Fire" );
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
      if ( specialization() == PRIEST_DISCIPLINE ||
           specialization() == PRIEST_HOLY )
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
  if ( name_str == "mind_harvest" )
  {
    struct mind_harvest_expr_t : public expr_t
    {
      priest_t& player;
      action_t& action;
      mind_harvest_expr_t( priest_t& p, action_t& act )
        : expr_t( "mind_harvest" ), player( p ), action( act )
      {
      }

      double evaluate() override
      {
        priest_td_t& td = *player.get_target_data( action.target );

        return td.glyph_of_mind_harvest_consumed;
      }
    };
    return new mind_harvest_expr_t( *this, *a );
  }
  if ( name_str == "primary_target" )
  {
    struct primary_target_t : public expr_t
    {
      priest_t& player;
      action_t& action;
      primary_target_t( priest_t& p, action_t& act )
        : expr_t( "primary_target" ), player( p ), action( act )
      {
      }

      double evaluate() override
      {
        if ( player.target == action.target )
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    };
    return new primary_target_t( *this, *a );
  }
  if ( name_str == "shadowy_apparitions_in_flight" )
  {
    struct shadowy_apparitions_in_flight_t : public expr_t
    {
      priest_t& priest;
      shadowy_apparitions_in_flight_t( priest_t& p )
        : expr_t( "shadowy_apparitions_in_flight" ), priest( p )
      {
      }

      double evaluate() override
      {
        if ( !priest.active_spells.shadowy_apparitions )
          return 0.0;

        return static_cast<double>(
            priest.active_spells.shadowy_apparitions->get_num_travel_events() );
      }
    };
    return new shadowy_apparitions_in_flight_t( *this );
  }

  return player_t::create_expression( a, name_str );
}

void priest_t::init_resources( bool force )
{
  base_t::init_resources( force );
  resources.current[ RESOURCE_SHADOW_ORB ] =
      resources.initial[ RESOURCE_SHADOW_ORB ] =
          clamp( as<double>( options.initial_shadow_orbs ), 0.0,
                 resources.base[ RESOURCE_SHADOW_ORB ] );
}

// priest_t::composite_armor ================================================

double priest_t::composite_armor() const
{
  double a = base_t::composite_armor();

  if ( buffs.shadowform->check() )
    a *= 1.0 + buffs.shadowform->data().effectN( 3 ).percent();

  return std::floor( a );
}

// priest_t::spirit ==========================================

double priest_t::spirit() const
{
  double s = player_t::spirit();

  return s;
}

// priest_t::composite_spell_haste ==========================================

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.power_infusion->check() )
    h /= 1.0 + buffs.power_infusion->data().effectN( 1 ).percent();

  if ( buffs.resolute_spirit->check() )
    h /= 1.0 + buffs.resolute_spirit->data().effectN( 1 ).percent();

  if ( buffs.mental_instinct->check() )
    h /= 1.0 +
         buffs.mental_instinct->data().effectN( 1 ).percent() *
             buffs.mental_instinct->check();

  return h;
}

// priest_t::composite_melee_haste ==========================================

double priest_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.power_infusion->check() )
    h /= 1.0 + buffs.power_infusion->data().effectN( 1 ).percent();

  if ( buffs.resolute_spirit->check() )
    h /= 1.0 + buffs.resolute_spirit->data().effectN( 1 ).percent();

  if ( buffs.mental_instinct->check() )
    h /= 1.0 +
         buffs.mental_instinct->data().effectN( 1 ).percent() *
             buffs.mental_instinct->check();

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

// priest_t::composite_spell_power ===============================

double priest_t::composite_spell_power( school_e school ) const
{
  return base_t::composite_spell_power( school );
}

// priest_t::composite_spell_power_multiplier ===============================

double priest_t::composite_spell_power_multiplier() const
{
  return base_t::composite_spell_power_multiplier();
}

// priest_t::composite_spell_crit ===============================

double priest_t::composite_spell_crit() const
{
  double csc = base_t::composite_spell_crit();

  if ( buffs.archangel->check() )
    csc *= 1.0 + sets.set( SET_HEALER, T16, B2 )->effectN( 2 ).percent();

  return csc;
}

// priest_t::composite_melee_crit ===============================

double priest_t::composite_melee_crit() const
{
  double cmc = base_t::composite_melee_crit();

  if ( buffs.archangel->check() )
    cmc *= 1.0 + sets.set( SET_HEALER, T16, B2 )->effectN( 2 ).percent();

  return cmc;
}

double priest_t::composite_multistrike() const
{
  double cm = base_t::composite_multistrike();

  if ( buffs.premonition->check() )
  {
    cm += buffs.premonition->data().effectN( 1 ).percent();
  }

  return cm;
}

// Multistrike Effect Multipliers ====================

double priest_t::composite_player_multistrike_damage_multiplier() const
{
  double m = player_t::composite_player_multistrike_damage_multiplier();
  m *= 1.0 + specs.divine_providence->effectN( 2 ).percent();
  return m;
}

double priest_t::composite_player_multistrike_healing_multiplier() const
{
  double m = player_t::composite_player_multistrike_healing_multiplier();
  m *= 1.0 + specs.divine_providence->effectN( 2 ).percent();
  return m;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  if ( specs.shadowform->ok() && dbc::is_school( SCHOOL_SHADOWFROST, school ) )
  {
    m *= 1.0 +
         buffs.shadowform->check() * specs.shadowform->effectN( 2 ).percent();
  }

  if ( dbc::is_school( SCHOOL_SHADOWLIGHT, school ) )
  {
    if ( buffs.chakra_chastise->check() )
    {
      m *= 1.0 + buffs.chakra_chastise->data().effectN( 1 ).percent();
    }
  }

  if ( buffs.twist_of_fate->check() )
  {
    m *= 1.0 + buffs.twist_of_fate->current_value;
  }

  if ( perks.enhanced_focused_will->ok() && buffs.focused_will->check() )
  {
    m *= 1.0 +
         buffs.focused_will->check() *
             perks.enhanced_focused_will->effectN( 1 ).percent();
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

  if ( mastery_spells.shield_discipline->ok() )
  {
    m *= 1.0 +
         cache.mastery() *
             mastery_spells.shield_discipline->effectN( 3 ).mastery_value();
  }
  if ( specs.grace->ok() )
    m *= 1.0 + specs.grace->effectN( 1 ).percent();

  if ( buffs.saving_grace_penalty->check() )
  {
    m *= 1.0 +
         buffs.saving_grace_penalty->check() *
             buffs.saving_grace_penalty->data().effectN( 1 ).percent();
  }

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

  if ( specs.grace->ok() )
    m *= 1.0 + specs.grace->effectN( 2 ).percent();

  if ( buffs.saving_grace_penalty->check() )
  {
    m *= 1.0 +
         buffs.saving_grace_penalty->check() *
             buffs.saving_grace_penalty->data().effectN( 2 ).percent();
  }

  if ( buffs.reperation->check() )
  {
    // TODO: check assumption that it increases absorb as well.
    m *= 1.0 +
         buffs.reperation->data().effectN( 1 ).percent() *
             buffs.reperation->check();
  }

  return m;
}
// priest_t::temporary_movement_modifier =======================================

double priest_t::temporary_movement_modifier() const
{
  double speed = player_t::temporary_movement_modifier();

  if ( glyphs.free_action->ok() && buffs.dispersion->check() )
    speed = std::max( speed, glyphs.free_action->effectN( 1 ).percent() );

  if ( buffs.glyph_of_levitate->check() )
    speed = std::max( speed, buffs.glyph_of_levitate->default_value );

  if ( buffs.glyph_of_mind_flay->check() )
    speed = std::max(
        speed, ( buffs.glyph_of_mind_flay->data().effectN( 1 ).percent() *
                 buffs.glyph_of_mind_flay->check() ) );

  return speed;
}

// priest_t::composite_attribute_multiplier =================================

double priest_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = base_t::composite_attribute_multiplier( attr );

  return m;
}

// priest_t::composite_rating_multiplier ====================================

double priest_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
    case RATING_SPELL_HASTE:  // Shadow
    case RATING_MELEE_HASTE:  // Shadow, for Shadowfiend/Mindbender
      if ( specs.mastermind->ok() )
        m *= 1.0 + specs.mastermind->effectN( 1 ).percent();
      break;
    case RATING_SPELL_CRIT:  // Discipline
    case RATING_MELEE_CRIT:  // Discipline, for Shadowfiend/Mindbender
      if ( specs.enlightenment->ok() )
        m *= 1.0 + specs.enlightenment->effectN( 2 ).percent();
      break;
    case RATING_MULTISTRIKE:  // Holy
      if ( specs.divine_providence->ok() )
        m *= 1.0 + specs.divine_providence->effectN( 1 ).percent();
      break;
    default:
      break;
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
  if ( name == "chakra_chastise" )
    return new chakra_chastise_t( *this, options_str );
  if ( name == "chakra_sanctuary" )
    return new chakra_sanctuary_t( *this, options_str );
  if ( name == "chakra_serenity" )
    return new chakra_serenity_t( *this, options_str );
  if ( name == "dispersion" )
    return new dispersion_t( *this, options_str );
  if ( name == "levitate" )
    return new levitate_t( *this, options_str );
  if ( name == "power_word_fortitude" )
    return new fortitude_t( *this, options_str );
  if ( name == "pain_suppression" )
    return new pain_suppression_t( *this, options_str );
  if ( name == "power_infusion" )
    return new power_infusion_t( *this, options_str );
  if ( name == "shadowform" )
    return new shadowform_t( *this, options_str );
  if ( name == "silence" )
    return new silence_t( *this, options_str );
  if ( name == "vampiric_embrace" )
    return new vampiric_embrace_t( *this, options_str );
  if ( name == "spirit_shell" )
    return new spirit_shell_t( *this, options_str );

  // Damage
  if ( name == "devouring_plague" )
    return new devouring_plague_t( *this, options_str );
  if ( name == "mind_blast" )
    return new mind_blast_t( *this, options_str );
  if ( name == "mind_flay" )
    return new mind_flay_base_t<>( *this, options_str );
  if ( name == "insanity" )
    return new insanity_t( *this, options_str );
  if ( name == "mind_spike" )
    return new mind_spike_t( *this, options_str );
  if ( name == "mind_sear" )
    return new mind_sear_base_t<>( *this, options_str );
  if ( name == "penance" )
    return new penance_t( *this, options_str );
  if ( name == "searing_insanity" )
    return new searing_insanity_t( *this, options_str );
  if ( name == "shadow_word_death" )
    return new shadow_word_death_t( *this, options_str );
  if ( name == "shadow_word_pain" )
    return new shadow_word_pain_t( *this, options_str );
  if ( name == "smite" )
    return new smite_t( *this, options_str );
  if ( ( name == "shadowfiend" ) || ( name == "mindbender" ) )
  {
    if ( talents.mindbender->ok() )
      return new summon_mindbender_t( *this, options_str );
    else
      return new summon_shadowfiend_t( *this, options_str );
  }
  if ( ( name == "holy_fire" ) || ( name == "power_word_solace" ) )
  {
    if ( talents.power_word_solace->ok() )
      return new power_word_solace_t( *this, options_str );
    else
      return new holy_fire_t( *this, options_str );
  }
  if ( name == "vampiric_touch" )
    return new vampiric_touch_t( *this, options_str );
  if ( name == "cascade" )
    return new cascade_t( *this, options_str );
  if ( name == "halo" )
    return new halo_t( *this, options_str );
  if ( name == "divine_star" )
    return new divine_star_t( *this, options_str );
  if ( name == "void_entropy" )
    return new void_entropy_t( *this, options_str );

  // Heals
  if ( name == "binding_heal" )
    return new binding_heal_t( *this, options_str );
  if ( name == "circle_of_healing" )
    return new circle_of_healing_t( *this, options_str );
  if ( name == "divine_hymn" )
    return new divine_hymn_t( *this, options_str );
  if ( name == "flash_heal" )
    return new flash_heal_t( *this, options_str );
  if ( name == "heal" )
    return new _heal_t( *this, options_str );
  if ( name == "guardian_spirit" )
    return new guardian_spirit_t( *this, options_str );
  if ( name == "holy_word" )
    return new holy_word_t( *this, options_str );
  if ( name == "penance_heal" )
    return new penance_heal_t( *this, options_str );
  if ( name == "power_word_shield" )
    return new power_word_shield_t( *this, options_str );
  if ( name == "prayer_of_healing" )
    return new prayer_of_healing_t( *this, options_str );
  if ( name == "prayer_of_mending" )
    return new prayer_of_mending_t( *this, options_str );
  if ( name == "renew" )
    return new renew_t( *this, options_str );
  if ( name == "clarity_of_will" )
    return new clarity_of_will_t( *this, options_str );
  // if ( name == "clarity_of_purpose"     ) return new clarity_of_purpose_t
  // ( *this, options_str );
  if ( name == "saving_grace" )
    return new saving_grace_t( *this, options_str );

  if ( find_class_spell( "Lightwell" )->ok() )
    if ( name == "lightwell" )
      return new lightwell_t( *this, options_str );

  return base_t::create_action( name, options_str );
}

// priest_t::create_pet =====================================================

pet_t* priest_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  if ( pet_name == "shadowfiend" )
    return new pets::shadowfiend_pet_t( sim, *this );
  if ( pet_name == "mindbender" )
    return new pets::mindbender_pet_t( sim, *this );
  if ( pet_name == "lightwell" )
    return new pets::lightwell_pet_t( sim, *this );

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

  if ( find_class_spell( "Lightwell" )->ok() )
    pets.lightwell = create_pet( "lightwell" );
}

// priest_t::init_base ======================================================

void priest_t::init_base_stats()
{
  base_t::init_base_stats();

  if ( specs.shadow_orbs->ok() )
  {
    resources.base[ RESOURCE_SHADOW_ORB ] =
        specs.shadow_orbs->effectN( 1 ).base_value();

    resources.base[ RESOURCE_SHADOW_ORB ] +=
        perks.enhanced_shadow_orbs->effectN( 1 ).base_value();
  }

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  // Discipline/Holy
  base.mana_regen_from_spirit_multiplier =
      specs.meditation_disc->ok()
          ? specs.meditation_disc->effectN( 1 ).percent()
          : specs.meditation_holy->effectN( 1 ).percent();
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
  // Level 15 / Tier 1
  talents.desperate_prayer = find_talent_spell( "Desperate Prayer" );
  talents.spectral_guise   = find_talent_spell( "Spectral Guise" );
  talents.angelic_bulwark  = find_talent_spell( "Angelic Bulwark" );

  // Level 30 / Tier 2
  talents.body_and_soul   = find_talent_spell( "Body and Soul" );
  talents.angelic_feather = find_talent_spell( "Angelic Feather" );
  talents.phantasm        = find_talent_spell( "Phantasm" );

  // Level 45 / Tier 3
  talents.surge_of_light    = find_talent_spell( "Surge of Light" );
  talents.surge_of_darkness = find_talent_spell( "Surge of Darkness" );
  talents.mindbender        = find_talent_spell( "Mindbender" );
  talents.power_word_solace = find_talent_spell( "Power Word: Solace" );
  talents.insanity          = find_talent_spell( "Insanity" );

  // Level 60 / Tier 4
  talents.void_tendrils  = find_talent_spell( "Void Tendrils" );
  talents.psychic_scream = find_talent_spell( "Psychic Scream" );
  talents.dominate_mind  = find_talent_spell( "Dominate Mind" );

  // Level 75 / Tier 5
  talents.twist_of_fate   = find_talent_spell( "Twist of Fate" );
  talents.power_infusion  = find_talent_spell( "Power Infusion" );
  talents.divine_insight  = find_talent_spell( "Divine Insight" );
  talents.spirit_shell    = find_talent_spell( "Spirit Shell" );
  talents.shadowy_insight = find_talent_spell( "Shadowy Insight" );

  // Level 90 / Tier 6
  talents.cascade     = find_talent_spell( "Cascade" );
  talents.divine_star = find_talent_spell( "Divine Star" );
  talents.halo        = find_talent_spell( "Halo" );

  // Level 100 / Tier 7
  talents.clarity_of_power   = find_talent_spell( "Clarity of Power" );
  talents.clarity_of_purpose = find_talent_spell( "Clarity of Purpose" );
  talents.clarity_of_will    = find_talent_spell( "Clarity of Will" );
  talents.void_entropy       = find_talent_spell( "Void Entropy" );
  talents.words_of_mending   = find_talent_spell( "Words of Mending" );
  talents.auspicious_spirits = find_talent_spell( "Auspicious Spirits" );
  talents.saving_grace       = find_talent_spell( "Saving Grace" );

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
  specs.mysticism        = find_specialization_spell( "Mysticism" );
  specs.spirit_shell     = find_specialization_spell( "Spirit Shell" );
  specs.strength_of_soul = find_specialization_spell( "Strength of Soul" );
  specs.enlightenment    = find_specialization_spell( "Enlightenment" );

  // Holy
  specs.meditation_holy =
      find_specialization_spell( "Meditation", "meditation_holy", PRIEST_HOLY );
  specs.serendipity       = find_specialization_spell( "Serendipity" );
  specs.rapid_renewal     = find_specialization_spell( "Rapid Renewal" );
  specs.divine_providence = find_specialization_spell( "Divine Providence" );
  specs.focused_will      = find_specialization_spell( "Focused Will" );

  // Shadow
  specs.shadowform = find_class_spell( "Shadowform" );
  specs.shadowy_apparitions =
      find_specialization_spell( "Shadowy Apparitions" );
  specs.shadow_orbs = find_specialization_spell( "Shadow Orbs" );
  specs.mastermind  = find_specialization_spell( "Mastermind" );

  // Mastery Spells
  mastery_spells.shield_discipline = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light     = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.mental_anguish    = find_mastery_spell( PRIEST_SHADOW );

  // Perk Spells
  // General
  perks.enhanced_power_word_shield =
      find_perk_spell( "Enhanced Power Word: Shield" );

  // Healing
  perks.enhanced_focused_will = find_perk_spell( "Enhanced Focused Will" );
  perks.enhanced_leap_of_faith =
      find_perk_spell( "Enhanced Leap of Faith" );  // NYI

  // Discipline

  // Holy
  perks.enhanced_chakras = find_perk_spell( "Enhanced Chakras" );
  perks.enhanced_renew   = find_perk_spell( "Enhanced Renew" );

  // Shadow
  perks.enhanced_mind_flay   = find_perk_spell( "Enhanced Mind Flay" );
  perks.enhanced_shadow_orbs = find_perk_spell( "Enhanced Shadow Orbs" );
  perks.enhanced_shadow_word_death =
      find_perk_spell( "Enhanced Shadow Word: Death" );

  // Glyphs
  // glyphs.dispel_magic                 = find_glyph_spell( "Glyph of Dispel
  // Magic" );            //NYI
  // glyphs.fade                         = find_glyph_spell( "Glyph of Fade" );
  // //NYI
  // glyphs.fear_ward                    = find_glyph_spell( "Glyph of Fear
  // Ward" );               //NYI
  // glyphs.leap_of_faith                = find_glyph_spell( "Glyph of Leap of
  // Faith" );           //NYI
  glyphs.levitate = find_glyph_spell( "Glyph of Levitate" );
  // glyphs.mass_dispel                  = find_glyph_spell( "Glyph of Mass
  // Dispel" );             //NYI
  glyphs.power_word_shield = find_glyph_spell( "Glyph of Power Word: Shield" );
  glyphs.prayer_of_mending = find_glyph_spell( "Glyph of Prayer of Mending" );
  // glyphs.psychic_scream               = find_glyph_spell( "Glyph of Psychic
  // Scream" );          //NYI
  // glyphs.reflective_shield            = find_glyph_spell( "Glyph of
  // Reflective Shield" );       //NYI
  // glyphs.restored_faith               = find_glyph_spell( "Glyph of Restored
  // Faith" );          //NYI
  // glyphs.scourge_imprisonment         = find_glyph_spell( "Glyph of Scourge
  // Imprisonment" );    //NYI
  // glyphs.weakened_soul                = find_glyph_spell( "Glyph of Weakened
  // Soul" );           //NYI

  // Healing Specs
  glyphs.holy_fire  = find_glyph_spell( "Glyph of Holy Fire" );
  glyphs.inquisitor = find_glyph_spell( "Glyph of the Inquisitor" );
  // glyphs.purify                       = find_glyph_spell( "Glyph of Purify"
  // );                  //NYI
  // glyphs.shadow_magic                 = find_glyph_spell( "Glyph of Shadow
  // Magic" );            //NYI
  glyphs.smite = find_glyph_spell( "Glyph of Smite" );

  // Discipline
  glyphs.borrowed_time = find_glyph_spell( "Glyph of Borrowed Time" );
  glyphs.penance       = find_glyph_spell( "Glyph of Penance" );

  // Holy
  // glyphs.binding_heal                 = find_glyph_spell( "Glyph of Binding
  // Heal" );            //NYI
  glyphs.circle_of_healing = find_glyph_spell( "Glyph of Circle of Healing" );
  glyphs.deep_wells        = find_glyph_spell( "Glyph of Deep Wells" );
  // glyphs.guardian_spirit              = find_glyph_spell( "Glyph of Guardian
  // Spirit" );         //NYI
  // glyphs.lightwell                    = find_glyph_spell( "Glyph of
  // Lightwell" );               //NYI
  // glyphs.redeemer                     = find_glyph_spell( "Glyph of the
  // Redeemer" );            //NYI
  glyphs.renew = find_glyph_spell( "Glyph of Renew" );
  // glyphs.spirit_of_redemption         = find_glyph_spell( "Glyph of Spirit of
  // Redemption" );    //NYI

  // Shadow
  glyphs.delayed_coalescence =
      find_glyph_spell( "Glyph of Delayed Coalescence" );
  glyphs.dispersion = find_glyph_spell( "Glyph of Dispersion" );
  // glyphs.focused_mending              = find_glyph_spell( "Glyph of Focused
  // Mending" );         //NYI
  glyphs.free_action  = find_glyph_spell( "Glyph of Free Action" );
  glyphs.mind_blast   = find_glyph_spell( "Glyph of Mind Blast" );
  glyphs.mind_flay    = find_glyph_spell( "Glyph of Mind Flay" );
  glyphs.mind_harvest = find_glyph_spell( "Glyph of Mind Harvest" );
  glyphs.mind_spike   = find_glyph_spell( "Glyph of Mind Spike" );
  // glyphs.miraculous_dispelling        = find_glyph_spell( "Glyph of
  // Miraculous Dispelling" );   //NYI
  // glyphs.psychic_horror               = find_glyph_spell( "Glyph of Psychic
  // Horror" );          //NYI
  glyphs.shadow_word_death = find_glyph_spell( "Glyph of Shadow Word: Death" );
  glyphs.silence           = find_glyph_spell( "Glyph of Silence" );  // NYI
  glyphs.vampiric_embrace  = find_glyph_spell( "Glyph of Vampiric Embrace" );

  if ( mastery_spells.echo_of_light->ok() )
    active_spells.echo_of_light = new actions::heals::echo_of_light_t( *this );

  active_spells.surge_of_darkness = talents.surge_of_darkness->ok()
                                        ? find_spell( 87160 )
                                        : spell_data_t::not_found();

  // Range Based on Talents
  if ( talents.cascade->ok() )
    base.distance = 30.0;
  else if ( talents.divine_star->ok() )
    base.distance = 24.0;
  else if ( talents.halo->ok() )
    base.distance = 27.0;
  else
    base.distance = 27.0;
}

void priest_t::assess_damage_imminent( school_e, dmg_e, action_state_t* s )
{
  if ( s->result_amount > 0.0 )
  {
    buffs.focused_will->trigger();
  }
}

// priest_t::init_buffs =====================================================

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Talents
  buffs.power_infusion = buff_creator_t( this, "power_infusion" )
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

  buffs.surge_of_light =
      buff_creator_t( this, "surge_of_light" )
          .spell( find_spell( 109186 ) )
          .chance( talents.surge_of_light->effectN( 1 ).percent() );

  buffs.surge_of_darkness =
      buff_creator_t( this, "surge_of_darkness" )
          .spell( find_spell( 87160 ) )
          .chance( talents.surge_of_darkness->effectN( 1 ).percent() );

  buffs.divine_insight =
      buff_creator_t( this, "divine_insight" )
          .spell( talents.divine_insight )
          .chance( talents.divine_insight->effectN( 1 ).percent() );

  buffs.shadowy_insight =
      buff_creator_t( this, "shadowy_insight" )
          .spell( talents.shadowy_insight )
          .chance( talents.shadowy_insight->effectN( 4 ).percent() );

  buffs.insanity = buff_creator_t( this, "insanity" )
                       .spell( find_spell( 132573 ) )
                       .default_value( talents.insanity->max_stacks() );

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

  buffs.spirit_shell = new buffs::spirit_shell_t( *this );

  buffs.saving_grace_penalty =
      buff_creator_t( this, "saving_grace_penalty" )
          .spell( talents.saving_grace->effectN( 2 ).trigger() )
          .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  // Holy
  buffs.chakra_chastise =
      buff_creator_t( this, "chakra_chastise" )
          .spell( find_spell( 81209 ) )
          .chance( specialization() == PRIEST_HOLY ? 1.0 : 0.0 )
          .cd( timespan_t::from_seconds( 30 ) )
          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.chakra_sanctuary =
      buff_creator_t( this, "chakra_sanctuary" )
          .spell( find_spell( 81206 ) )
          .chance( specialization() == PRIEST_HOLY ? 1.0 : 0.0 )
          .cd( timespan_t::from_seconds( 30 ) );

  buffs.chakra_serenity =
      buff_creator_t( this, "chakra_serenity" )
          .spell( find_spell( 81208 ) )
          .chance( specialization() == PRIEST_HOLY ? 1.0 : 0.0 )
          .cd( timespan_t::from_seconds( 30 ) );

  buffs.serendipity = buff_creator_t( this, "serendipity" )
                          .spell( find_spell( specs.serendipity->effectN( 1 )
                                                  .trigger_spell_id() ) );

  buffs.focused_will =
      buff_creator_t( this, "focused_will" )
          .spell( specs.focused_will->ok()
                      ? specs.focused_will->effectN( 1 ).trigger()
                      : spell_data_t::not_found() );

  if ( perks.enhanced_focused_will->ok() )
    buffs.focused_will->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Shadow

  buffs.shadowform = new buffs::shadowform_t( *this );

  buffs.vampiric_embrace =
      buff_creator_t( this, "vampiric_embrace",
                      find_class_spell( "Vampiric Embrace" ) )
          .duration( find_class_spell( "Vampiric Embrace" )->duration() +
                     glyphs.vampiric_embrace->effectN( 1 ).time_value() );

  buffs.glyph_of_mind_spike =
      buff_creator_t( this, "glyph_mind_spike" )
          .spell( glyphs.mind_spike->effectN( 1 ).trigger() )
          .chance( glyphs.mind_spike->proc_chance() );

  buffs.shadow_word_death_reset_cooldown =
      buff_creator_t( this, "shadow_word_death_reset_cooldown" )
          .max_stack( 1 )
          .duration( timespan_t::from_seconds( 9.0 ) );  // data in the old
  // deprecated glyph. Leave
  // hardcoded for now,
  // 3/12/2012; 9.0sec ICD in
  // 5.4 (2013/08/18)

  buffs.mind_sear_on_hit_reset =
      buff_creator_t( this, "mind_sear_on_hit_reset" )
          .max_stack( 2 )
          .duration( timespan_t::from_seconds( 5.0 ) );

  buffs.dispersion = new buffs::dispersion_t( *this );

  // Set Bonuses
  buffs.empowered_shadows =
      buff_creator_t( this, "empowered_shadows" )
          .spell( sets.set( SET_CASTER, T16, B4 )->effectN( 1 ).trigger() )
          .chance( sets.has_set_bonus( SET_CASTER, T16, B4 ) )
          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.absolution = buff_creator_t( this, "absolution" )
                         .spell( find_spell( 145336 ) )
                         .chance( sets.has_set_bonus( SET_HEALER, T16, B4 ) );

  buffs.resolute_spirit =
      stat_buff_creator_t( this, "resolute_spirit" )
          .spell( find_spell( 145374 ) )
          .chance( sets.has_set_bonus( SET_HEALER, T16, B4 ) )
          .add_invalidate( CACHE_SPELL_HASTE )
          .add_invalidate( CACHE_HASTE );

  buffs.mental_instinct =
      buff_creator_t( this, "mental_instinct" )
          .spell( sets.set( PRIEST_SHADOW, T17, B4 )->effectN( 1 ).trigger() )
          .chance( sets.has_set_bonus( PRIEST_SHADOW, T17, B4 ) )
          .add_invalidate( CACHE_SPELL_HASTE )
          .add_invalidate( CACHE_HASTE );

  buffs.clear_thoughts =
      buff_creator_t( this, "clear_thoughts" )
          .spell(
              sets.set( PRIEST_DISCIPLINE, T17, B4 )->effectN( 1 ).trigger() )
          .chance( sets.set( PRIEST_DISCIPLINE, T17, B4 )->proc_chance() );

  buffs.shadow_power =
      buff_creator_t( this, "shadow_power" )
          .spell( find_spell( 171150 ) )
          .chance( sets.has_set_bonus( PRIEST_SHADOW, PVP, B2 ) )
          .add_invalidate( CACHE_VERSATILITY );

  buffs.reperation =
      buff_creator_t( this, "reperation" )
          .spell( find_spell( 186478 ) )
          .chance( sets.has_set_bonus( PRIEST_DISCIPLINE, T18, B2 ) )
          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buffs.premonition =
      buff_creator_t( this, "premonition" )
          .spell( find_spell( 188779 ) )
          .chance( sets.has_set_bonus( PRIEST_SHADOW, T18, B4 ) )
          .add_invalidate( CACHE_MULTISTRIKE );

  // Glyphs
  buffs.glyph_of_levitate =
      buff_creator_t( this, "glyph_of_levitate", glyphs.levitate )
          .default_value( glyphs.levitate->effectN( 1 ).percent() );

  buffs.glyph_of_mind_flay =
      buff_creator_t( this, "glyph_of_mind_flay", find_spell( 120587 ) )
          .chance( glyphs.mind_flay->ok() ? 1.0 : 0.0 );
}

// ALL Spec Pre-Combat Action Priority List

void priest_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  if ( sim->allow_flasks && true_level >= 80 )
  {
    std::string flask_action = "flask,type=";

    if ( true_level > 90 )
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
          if ( talents.clarity_of_power->ok() )
            flask_action += "greater_draenic_intellect_flask";
          else if ( talents.auspicious_spirits->ok() )
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

    if ( level() > 90 )
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
          if ( talents.clarity_of_power->ok() )
            food_action += "sleeper_sushi";
          else if ( talents.auspicious_spirits->ok() )
            food_action += "pickled_eel";
          else
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

  precombat->add_action( this, "Power Word: Fortitude", "if=!aura.stamina.up" );

  // Chakra / Shadowform
  switch ( specialization() )
  {
    case PRIEST_HOLY:
      if ( primary_role() != ROLE_HEAL )
        precombat->add_action( this, "Chakra: Chastise" );
      else
        precombat->add_action( this, "Chakra: Serenity" );
      break;
    case PRIEST_SHADOW:
      precombat->add_action( this, "Shadowform", "if=!buff.shadowform.up" );
      break;
    case PRIEST_DISCIPLINE:
    default:
      break;
  }

  // Snapshot stats
  precombat->add_action( "snapshot_stats",
                         "Snapshot raid buffed stats before combat begins and "
                         "pre-potting is done." );

  if ( sim->allow_potions && true_level >= 80 )
  {
    if ( true_level > 90 )
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
      if ( primary_role() != ROLE_HEAL )
        precombat->add_action( "smite" );
      else
        precombat->add_action( "prayer_of_mending" );
      break;
    case PRIEST_HOLY:
      if ( primary_role() != ROLE_HEAL )
        precombat->add_action( "smite" );
      else
        precombat->add_action( "prayer_of_mending" );
      break;
    case PRIEST_SHADOW:
    default:
      if ( talents.clarity_of_power->ok() )
        precombat->add_action( "mind_spike" );
      else
        precombat->add_action( "vampiric_touch" );
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
  def->add_action( this, "Holy Fire" );
  def->add_action( this, "Shadow Word: Pain",
                   ",if=remains<tick_time|!ticking" );
  def->add_action( this, "Smite" );
}

// Shadow Combat Action Priority List

void priest_t::apl_shadow()
{
  action_priority_list_t* default_list           = get_action_priority_list( "default" );
  action_priority_list_t* decision               = get_action_priority_list( "decision" );
  action_priority_list_t* pvp_dispersion         = get_action_priority_list( "pvp_dispersion" );
  action_priority_list_t* main                   = get_action_priority_list( "main" );
  action_priority_list_t* vent                   = get_action_priority_list( "vent" );
  action_priority_list_t* cop_dotweave           = get_action_priority_list( "cop_dotweave" );
  action_priority_list_t* cop_insanity           = get_action_priority_list( "cop_insanity" );
  action_priority_list_t* cop_row_insanity_spend = get_action_priority_list( "cop_row_insanity_spend" );
  action_priority_list_t* cop_row_insanity_build = get_action_priority_list( "cop_row_insanity_build" );
  action_priority_list_t* cop_row_insanity_death = get_action_priority_list( "cop_row_insanity_death" );
  action_priority_list_t* cop                    = get_action_priority_list( "cop" );

  default_list->add_action( this, "Shadowform", "if=!buff.shadowform.up" );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list->add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    default_list->add_action( profession_actions[ i ] );

  // Potions
  if ( sim->allow_potions && true_level >= 80 )
  {
    if ( true_level > 90 )
      default_list->add_action( "potion,name=draenic_intellect,if=buff.bloodlust.react|target.time_to_die<=40" );
    else if ( true_level > 85 )
      default_list->add_action( "potion,name=jade_serpent,if=buff.bloodlust.react|target.time_to_die<=40" );
    else
      default_list->add_action( "potion,name=volcanic,if=buff.bloodlust.react|target.time_to_die<=40" );
  }

  if ( find_item( "nithramus_the_allseer" ) )
  {
    default_list->add_action( "use_item,name=nithramus_the_allseer" );
  }

  default_list->add_action( "power_infusion,if=talent.power_infusion.enabled" );
  default_list->add_action( "silence,if=target.debuff.casting.react" );

  // Racials
  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list->add_action( racial_actions[ i ] );

  default_list->add_action( "call_action_list,name=pvp_dispersion,if=set_bonus.pvp_2pc" );
  default_list->add_action( "call_action_list,name=decision" );

  // Choose which APL to use based on talents and fight conditions.
  decision->add_action( "call_action_list,name=main,if=(!talent.clarity_of_power.enabled&!talent.void_entropy.enabled)|(talent.clarity_of_power.enabled&!t18_class_trinket&buff.bloodlust.up&buff.power_infusion.up)" );
  decision->add_action( "call_action_list,name=vent,if=talent.void_entropy.enabled&!talent.clarity_of_power.enabled&!talent.auspicious_spirits.enabled" );
  decision->add_action( "call_action_list,name=cop_row_insanity_spend,if=active_enemies=1&talent.clarity_of_power.enabled&talent.insanity.enabled&t18_class_trinket&target.health.pct>20&shadow_orb=5" );
  decision->add_action( "call_action_list,name=cop_row_insanity_build,if=active_enemies=1&talent.clarity_of_power.enabled&talent.insanity.enabled&t18_class_trinket&target.health.pct>20&shadow_orb<5" );
  decision->add_action( "call_action_list,name=cop_row_insanity_death,if=active_enemies=1&talent.clarity_of_power.enabled&talent.insanity.enabled&t18_class_trinket&target.health.pct<=20" );
  decision->add_action( "call_action_list,name=cop,if=talent.clarity_of_power.enabled&!talent.insanity.enabled" );
  decision->add_action( "call_action_list,name=cop_dotweave,if=talent.clarity_of_power.enabled&talent.insanity.enabled&target.health.pct>20&active_enemies<=6" );
  decision->add_action( "call_action_list,name=cop_insanity,if=talent.clarity_of_power.enabled&talent.insanity.enabled" );

  // Use the PVP 2P to get a boost.
  pvp_dispersion->add_action( "call_action_list,name=decision,if=cooldown.dispersion.remains>0" );
  pvp_dispersion->add_action( "dispersion,interrupt=1" );
  pvp_dispersion->add_action( "call_action_list,name=decision" );

  // Auspicious Spirits + Main APL if you don't have a L100 Talent selected
  main->add_action( "mindbender,if=talent.mindbender.enabled" );
  main->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  main->add_action( "shadow_word_death,if=target.health.pct<20&shadow_orb<=4,cycle_targets=1" );
  main->add_action( "mind_blast,if=glyph.mind_harvest.enabled&shadow_orb<=2&active_enemies<=5&cooldown_react" );
  main->add_action( "devouring_plague,if=shadow_orb=5&!target.dot.devouring_plague_dot.ticking&(talent.surge_of_darkness.enabled|set_bonus.tier17_4pc),cycle_targets=1" );
  main->add_action( "devouring_plague,if=shadow_orb=5" );
  main->add_action( "devouring_plague,if=shadow_orb>=3&talent.auspicious_spirits.enabled&shadowy_apparitions_in_flight>=3" );
  main->add_action( "devouring_plague,if=shadow_orb>=4&talent.auspicious_spirits.enabled&shadowy_apparitions_in_flight>=2" );
  main->add_action( "devouring_plague,if=shadow_orb>=3&buff.mental_instinct.remains<gcd&buff.mental_instinct.remains>(gcd*0.7)&buff.mental_instinct.remains" );
  main->add_action( "devouring_plague,if=shadow_orb>=4&talent.auspicious_spirits.enabled&((cooldown.mind_blast.remains<gcd&!set_bonus.tier17_2pc&(!set_bonus.tier18_4pc&!talent.mindbender.enabled))|(target.health.pct<20&cooldown.shadow_word_death.remains<gcd))&!target.dot.devouring_plague_tick.ticking&talent.surge_of_darkness.enabled,cycle_targets=1" );
  main->add_action( "devouring_plague,if=shadow_orb>=4&talent.auspicious_spirits.enabled&((cooldown.mind_blast.remains<gcd&!set_bonus.tier17_2pc&(!set_bonus.tier18_4pc&!talent.mindbender.enabled))|(target.health.pct<20&cooldown.shadow_word_death.remains<gcd))" );
  main->add_action( "devouring_plague,if=shadow_orb>=3&!talent.auspicious_spirits.enabled&((cooldown.mind_blast.remains<gcd&!set_bonus.tier17_2pc&(!set_bonus.tier18_4pc&!talent.mindbender.enabled))|(target.health.pct<20&cooldown.shadow_word_death.remains<gcd))&!target.dot.devouring_plague_tick.ticking&talent.surge_of_darkness.enabled,cycle_targets=1" );
  main->add_action( "devouring_plague,if=shadow_orb>=3&!talent.auspicious_spirits.enabled&((cooldown.mind_blast.remains<gcd&!set_bonus.tier17_2pc&(!set_bonus.tier18_4pc&!talent.mindbender.enabled))|(target.health.pct<20&cooldown.shadow_word_death.remains<gcd))" );
  main->add_action( "devouring_plague,if=shadow_orb>=3&talent.auspicious_spirits.enabled&set_bonus.tier18_4pc&talent.mindbender.enabled&buff.premonition.up" );
  main->add_action( "mind_blast,if=glyph.mind_harvest.enabled&mind_harvest=0,cycle_targets=1" );
  main->add_action( "mind_blast,if=talent.auspicious_spirits.enabled&active_enemies<=4&cooldown_react" );
  main->add_action( "shadow_word_pain,if=talent.auspicious_spirits.enabled&remains<(18*0.3)&target.time_to_die>(18*0.75)&miss_react,cycle_targets=1,max_cycle_targets=7" );
  main->add_action( "mind_blast,if=cooldown_react" );
  main->add_action( "insanity,if=t18_class_trinket&target.debuff.mental_fatigue.remains<gcd,interrupt_if=target.debuff.mental_fatigue.remains>gcd" );
  main->add_action( "mind_flay,if=t18_class_trinket&(target.debuff.mental_fatigue.remains<gcd|(cooldown.mind_blast.remains<2*gcd&target.debuff.mental_fatigue.remains<2*gcd)),interrupt_if=target.debuff.mental_fatigue.remains>gcd" );
  main->add_action( "searing_insanity,if=buff.insanity.remains<0.5*gcd&active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  main->add_action( "searing_insanity,if=active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  main->add_action( "insanity,if=buff.insanity.remains<0.5*gcd&active_enemies<=2,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1|shadow_orb=5)" );
  main->add_action( "insanity,chain=1,if=active_enemies<=2,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1|shadow_orb=5)" );
  main->add_action( "halo,if=talent.halo.enabled&target.distance<=30&active_enemies>2" );
  main->add_action( "cascade,if=talent.cascade.enabled&set_bonus.tier18_4pc&buff.premonition.up&active_enemies>2&target.distance<=40" );
  main->add_action( "divine_star,if=talent.divine_star.enabled&set_bonus.tier18_4pc&buff.premonition.up&active_enemies>4&target.distance<=24" );
  main->add_action( "cascade,if=talent.cascade.enabled&!set_bonus.tier18_4pc&active_enemies>2&target.distance<=40" );
  main->add_action( "divine_star,if=talent.divine_star.enabled&!set_bonus.tier18_4pc&active_enemies>4&target.distance<=24" );
  main->add_action( "shadow_word_pain,if=!talent.auspicious_spirits.enabled&remains<(18*0.3)&target.time_to_die>(18*0.75)&miss_react&active_enemies<=5,cycle_targets=1,max_cycle_targets=5" );
  main->add_action( "vampiric_touch,if=remains<(15*0.3+cast_time)&target.time_to_die>(15*0.75+cast_time)&miss_react&active_enemies<=5,cycle_targets=1,max_cycle_targets=5" );
  main->add_action( "devouring_plague,if=!talent.void_entropy.enabled&shadow_orb>=3&ticks_remain<=1" );
  main->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&set_bonus.tier18_4pc&buff.premonition.up" );
  main->add_action( "mind_spike,if=active_enemies<=5&buff.surge_of_darkness.react=3" );
  main->add_action( "halo,if=talent.halo.enabled&target.distance<=30&target.distance>=17" );
  main->add_action( "cascade,if=talent.cascade.enabled&set_bonus.tier18_4pc&buff.premonition.up&(active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11" );
  main->add_action( "divine_star,if=talent.divine_star.enabled&set_bonus.tier18_4pc&buff.premonition.up&target.distance<=24" );
  main->add_action( "cascade,if=talent.cascade.enabled&!set_bonus.tier18_4pc&(active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11" );
  main->add_action( "divine_star,if=talent.divine_star.enabled&!set_bonus.tier18_4pc&target.distance<=24" );
  main->add_action( "wait,sec=cooldown.shadow_word_death.remains,if=target.health.pct<20&cooldown.shadow_word_death.remains<0.5&active_enemies<=1,cycle_targets=1" );
  main->add_action( "wait,sec=cooldown.mind_blast.remains,if=cooldown.mind_blast.remains<0.5&cooldown.mind_blast.remains&active_enemies<=1" );
  main->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&!set_bonus.tier18_4pc" );
  main->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&set_bonus.tier18_4pc&(buff.premonition.up|talent.mindbender.enabled)" );
  main->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&set_bonus.tier18_4pc&!talent.mindbender.enabled&cooldown.shadowfiend.remains>13&buff.surge_of_darkness.remains<(1.1*gcd*buff.surge_of_darkness.react)" );
  main->add_action( "divine_star,if=talent.divine_star.enabled&target.distance<=28&active_enemies>1" );
  main->add_action( "mind_sear,chain=1,if=active_enemies>=4,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1|shadow_orb=5),target_if=max:spell_targets.mind_sear_tick" );
  main->add_action( "shadow_word_pain,if=talent.auspicious_spirits.enabled&remains<(18*0.9)&target.time_to_die>(18*0.75)&active_enemies>=3&miss_react,cycle_targets=1,max_cycle_targets=7" );
  main->add_action( "shadow_word_pain,if=shadow_orb>=2&ticks_remain<=3&target.time_to_die>(18*0.75)&talent.insanity.enabled" );
  main->add_action( "vampiric_touch,if=shadow_orb>=2&ticks_remain<=3.5&target.time_to_die>(15*0.75+cast_time)&talent.insanity.enabled" );
  main->add_action( "mind_flay,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1|shadow_orb=5)" );
  main->add_action( "shadow_word_death,moving=1,if=movement.remains>=1*gcd" );
  main->add_action( "power_word_shield,moving=1,if=talent.body_and_soul.enabled&movement.distance>=25" );
  main->add_action( "halo,if=talent.halo.enabled&target.distance<=30,moving=1" );
  main->add_action( "divine_star,moving=1,if=talent.divine_star.enabled&target.distance<=28" );
  main->add_action( "cascade,moving=1,if=talent.cascade.enabled&target.distance<=40" );
  main->add_action( "shadow_word_pain,moving=1,cycle_targets=1" );

  // Void Entropy
  vent->add_action( "mindbender,if=set_bonus.tier18_2pc&talent.mindbender.enabled" );
  vent->add_action( "shadowfiend,if=set_bonus.tier18_2pc&!talent.mindbender.enabled" );
  vent->add_action( "void_entropy,if=shadow_orb=3&!ticking&target.time_to_die>60&active_enemies=1" );
  vent->add_action( "void_entropy,if=!dot.void_entropy.ticking&shadow_orb=5&active_enemies>=1&target.time_to_die>60,cycle_targets=1,max_cycle_targets=6" );
  vent->add_action( "devouring_plague,if=shadow_orb=5&dot.void_entropy.ticking&dot.void_entropy.remains<=gcd*2&cooldown_react&active_enemies=1" );
  vent->add_action( "devouring_plague,if=dot.void_entropy.ticking&dot.void_entropy.remains<=gcd*2&cooldown_react&active_enemies>1,cycle_targets=1" );
  vent->add_action( "devouring_plague,if=shadow_orb=5&dot.void_entropy.remains<5&active_enemies>1,cycle_targets=1" );
  vent->add_action( "devouring_plague,if=shadow_orb=5&dot.void_entropy.remains<10&active_enemies>2,cycle_targets=1" );
  vent->add_action( "devouring_plague,if=shadow_orb=5&dot.void_entropy.remains<15&active_enemies>3,cycle_targets=1" );
  vent->add_action( "devouring_plague,if=shadow_orb=5&dot.void_entropy.remains<20&active_enemies>4,cycle_targets=1" );
  vent->add_action( "devouring_plague,if=shadow_orb=5&(dot.void_entropy.remains|target.time_to_die<=60)&(cooldown.mind_blast.remains<=gcd*2|(target.health.pct<20&cooldown.shadow_word_death.remains<=gcd*2))&active_enemies=1" );
  vent->add_action( "devouring_plague,if=shadow_orb=5&(dot.void_entropy.remains|target.time_to_die<=60)&(cooldown.mind_blast.remains<=gcd*2|(target.health.pct<20&cooldown.shadow_word_death.remains<=gcd*2))&active_enemies>1,cycle_targets=1" );
  vent->add_action( "devouring_plague,if=shadow_orb>=3&dot.void_entropy.ticking&active_enemies=1&buff.mental_instinct.remains<(gcd*1.4)&buff.mental_instinct.remains>(gcd*0.7)&buff.mental_instinct.remains" );
  vent->add_action( "devouring_plague,if=shadow_orb>=3&target.time_to_die<=gcd*4&active_enemies=1" );
  vent->add_action( "mindbender,if=!set_bonus.tier18_2pc&talent.mindbender.enabled&cooldown.mind_blast.remains>=gcd" );
  vent->add_action( "shadowfiend,if=!set_bonus.tier18_2pc&!talent.mindbender.enabled&cooldown.mind_blast.remains>=gcd" );
  vent->add_action( "halo,if=talent.halo.enabled&&target.distance<=30&active_enemies>=4" );
  vent->add_action( "cascade,if=talent.cascade.enabled&!set_bonus.tier18_4pc&active_enemies>=3&target.distance<=40" );
  vent->add_action( "divine_star,if=talent.divine_star.enabled&!set_bonus.tier18_4pc&active_enemies>6&target.distance<=24" );
  vent->add_action( "cascade,if=talent.cascade.enabled&set_bonus.tier18_4pc&buff.premonition.up&active_enemies>=3&target.distance<=40" );
  vent->add_action( "divine_star,if=talent.divine_star.enabled&set_bonus.tier18_4pc&buff.premonition.up&active_enemies>6&target.distance<=24" );
  vent->add_action( "mind_blast,if=glyph.mind_harvest.enabled&mind_harvest=0&shadow_orb<=2,cycle_targets=1" );
  vent->add_action( "devouring_plague,if=glyph.mind_harvest.enabled&mind_harvest=0&shadow_orb>=3,cycle_targets=1" );
  vent->add_action( "mind_blast,if=active_enemies<=10&cooldown_react&shadow_orb<=4" );
  vent->add_action( "shadow_word_death,if=target.health.pct<20&cooldown_react&shadow_orb<=4,cycle_targets=1" );
  vent->add_action( "insanity,if=t18_class_trinket&target.debuff.mental_fatigue.remains<gcd,interrupt_if=target.debuff.mental_fatigue.remains>gcd" );
  vent->add_action( "mind_flay,if=t18_class_trinket&(target.debuff.mental_fatigue.remains<gcd|(cooldown.mind_blast.remains<2*gcd&target.debuff.mental_fatigue.remains<2*gcd)),interrupt_if=target.debuff.mental_fatigue.remains>gcd" );
  vent->add_action( "searing_insanity,if=buff.insanity.remains<0.5*gcd&active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  vent->add_action( "searing_insanity,if=active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  vent->add_action( "shadow_word_pain,if=shadow_orb=4&remains<(18*0.50)&set_bonus.tier17_2pc&cooldown.mind_blast.remains<1.2*gcd&cooldown.mind_blast.remains>0.2*gcd" );
  vent->add_action( "insanity,if=buff.insanity.remains<0.5*gcd&active_enemies<=3&cooldown.mind_blast.remains>0.5*gcd,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1)" );
  vent->add_action( "insanity,chain=1,if=active_enemies<=3&cooldown.mind_blast.remains>0.5*gcd,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1)" );
  vent->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&set_bonus.tier18_4pc&buff.premonition.up" );
  vent->add_action( "mind_spike,if=active_enemies<=5&buff.surge_of_darkness.react=3" );
  vent->add_action( "shadow_word_pain,if=remains<(18*0.3)&target.time_to_die>(18*0.75)&miss_react,cycle_targets=1,max_cycle_targets=5" );
  vent->add_action( "vampiric_touch,if=remains<(15*0.3+cast_time)&target.time_to_die>(15*0.75+cast_time)&miss_react,cycle_targets=1,max_cycle_targets=5" );
  vent->add_action( "halo,if=talent.halo.enabled&target.distance<=30&cooldown.mind_blast.remains>0.5*gcd" );
  vent->add_action( "cascade,if=talent.cascade.enabled&!set_bonus.tier18_4pc&target.distance<=40&cooldown.mind_blast.remains>0.5*gcd" );
  vent->add_action( "divine_star,if=talent.divine_star.enabled&!set_bonus.tier18_4pc&target.distance<=24&cooldown.mind_blast.remains>0.5*gcd" );
  vent->add_action( "cascade,if=talent.cascade.enabled&set_bonus.tier18_4pc&buff.premonition.up&target.distance<=40&cooldown.mind_blast.remains>0.5*gcd" );
  vent->add_action( "divine_star,if=talent.divine_star.enabled&set_bonus.tier18_4pc&buff.premonition.up&target.distance<=24&cooldown.mind_blast.remains>0.5*gcd" );
  vent->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&!set_bonus.tier18_4pc" );
  vent->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&set_bonus.tier18_4pc&(buff.premonition.up|talent.mindbender.enabled)" );
  vent->add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5&set_bonus.tier18_4pc&!talent.mindbender.enabled&cooldown.shadowfiend.remains>13&buff.surge_of_darkness.remains<(1.1*gcd*buff.surge_of_darkness.react)" );
  vent->add_action( "mind_spike,if=active_enemies<=5&buff.surge_of_darkness.react&cooldown.mind_blast.remains>0.5*gcd" );
  vent->add_action( "mind_sear,chain=1,if=active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  vent->add_action( "mind_flay,if=cooldown.mind_blast.remains>0.5*gcd,interrupt=1,chain=1" );
  vent->add_action( "shadow_word_death,moving=1,if=movement.remains>=1*gcd" );
  vent->add_action( "power_word_shield,moving=1,if=talent.body_and_soul.enabled&movement.distance>=25" );
  vent->add_action( "halo,if=talent.halo.enabled&target.distance<=30,moving=1" );
  vent->add_action( "divine_star,moving=1,if=talent.divine_star.enabled&target.distance<=28" );
  vent->add_action( "cascade,moving=1,if=talent.cascade.enabled&target.distance<=40" );
  vent->add_action( "shadow_word_pain,moving=1,cycle_targets=1" );

  // Clarity of Power DoT Weaving, for when you have Insanity and the target is
  // above 20%
  cop_dotweave->add_action( "devouring_plague,if=target.dot.vampiric_touch.ticking&target.dot.shadow_word_pain.ticking&shadow_orb=5&cooldown_react" );
  cop_dotweave->add_action( "devouring_plague,if=buff.mental_instinct.remains<gcd&buff.mental_instinct.remains>(gcd*0.7)&buff.mental_instinct.remains" );
  cop_dotweave->add_action( "devouring_plague,if=(target.dot.vampiric_touch.ticking&target.dot.shadow_word_pain.ticking&!buff.insanity.remains&cooldown.mind_blast.remains>0.4*gcd)" );
  cop_dotweave->add_action( "shadow_word_death,if=target.health.pct<20&!target.dot.shadow_word_pain.ticking&!target.dot.vampiric_touch.ticking,cycle_targets=1" );
  cop_dotweave->add_action( "shadow_word_death,if=target.health.pct<20,cycle_targets=1" );
  cop_dotweave->add_action( "mind_blast,if=glyph.mind_harvest.enabled&mind_harvest=0&shadow_orb<=2,cycle_targets=1" );
  cop_dotweave->add_action( "mind_blast,if=shadow_orb<=4&cooldown_react" );
  cop_dotweave->add_action( "shadowfiend,if=!talent.mindbender.enabled&set_bonus.tier18_2pc" );
  cop_dotweave->add_action( "mindbender,if=talent.mindbender.enabled&set_bonus.tier18_2pc" );
  cop_dotweave->add_action( "searing_insanity,if=buff.insanity.remains<0.5*gcd&active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  cop_dotweave->add_action( "searing_insanity,if=active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  cop_dotweave->add_action( "shadowfiend,if=!talent.mindbender.enabled&!buff.insanity.remains" );
  cop_dotweave->add_action( "mindbender,if=talent.mindbender.enabled&!buff.insanity.remains" );
  cop_dotweave->add_action( "shadow_word_pain,if=shadow_orb=4&set_bonus.tier17_2pc&!target.dot.shadow_word_pain.ticking&!target.dot.devouring_plague.ticking&cooldown.mind_blast.remains<gcd&cooldown.mind_blast.remains>0" );
  cop_dotweave->add_action( "shadow_word_pain,if=shadow_orb=5&!target.dot.devouring_plague.ticking&!target.dot.shadow_word_pain.ticking" );
  cop_dotweave->add_action( "vampiric_touch,if=shadow_orb=5&!target.dot.devouring_plague.ticking&!target.dot.vampiric_touch.ticking" );
  cop_dotweave->add_action( "insanity,if=buff.insanity.remains,chain=1,interrupt_if=cooldown.mind_blast.remains<=0.1" );
  cop_dotweave->add_action( "shadow_word_pain,if=shadow_orb>=2&target.dot.shadow_word_pain.remains>=6&cooldown.mind_blast.remains>0.5*gcd&target.dot.vampiric_touch.remains&buff.bloodlust.up&!set_bonus.tier17_2pc" );
  cop_dotweave->add_action( "vampiric_touch,if=shadow_orb>=2&target.dot.vampiric_touch.remains>=5&cooldown.mind_blast.remains>0.5*gcd&buff.bloodlust.up&!set_bonus.tier17_2pc" );
  cop_dotweave->add_action( "halo,if=talent.halo.enabled&!set_bonus.tier18_4pc&cooldown.mind_blast.remains>0.5*gcd&target.distance<=30&target.distance>=17" );
  cop_dotweave->add_action( "cascade,if=talent.cascade.enabled&!set_bonus.tier18_4pc&cooldown.mind_blast.remains>0.5*gcd&((active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11)" );
  cop_dotweave->add_action( "divine_star,if=talent.divine_star.enabled&!set_bonus.tier18_4pc&cooldown.mind_blast.remains>0.5*gcd&active_enemies>3&target.distance<=24" );
  cop_dotweave->add_action( "halo,if=talent.halo.enabled&set_bonus.tier18_4pc&buff.premonition.up&cooldown.mind_blast.remains>0.5*gcd&target.distance<=30&target.distance>=17" );
  cop_dotweave->add_action( "cascade,if=talent.cascade.enabled&set_bonus.tier18_4pc&buff.premonition.up&cooldown.mind_blast.remains>0.5*gcd&((active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11)" );
  cop_dotweave->add_action( "divine_star,if=talent.divine_star.enabled&set_bonus.tier18_4pc&buff.premonition.up&cooldown.mind_blast.remains>0.5*gcd&active_enemies>3&target.distance<=24" );
  cop_dotweave->add_action( "shadow_word_pain,if=primary_target=0&!ticking,cycle_targets=1,max_cycle_targets=5" );
  cop_dotweave->add_action( "vampiric_touch,if=primary_target=0&!ticking,cycle_targets=1,max_cycle_targets=5" );
  cop_dotweave->add_action( "divine_star,if=talent.divine_star.enabled&cooldown.mind_blast.remains>0.5*gcd&active_enemies=3&target.distance<=24" );
  cop_dotweave->add_action( "shadow_word_pain,if=primary_target=0&(!ticking|remains<=18*0.3)&target.time_to_die>(18*0.75),cycle_targets=1,max_cycle_targets=5" );
  cop_dotweave->add_action( "vampiric_touch,if=primary_target=0&(!ticking|remains<=15*0.3+cast_time)&target.time_to_die>(15*0.75+cast_time),cycle_targets=1,max_cycle_targets=5" );
  cop_dotweave->add_action( "mind_sear,if=active_enemies>=8,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  cop_dotweave->add_action( "mind_spike" );
  cop_dotweave->add_action( "shadow_word_death,moving=1,if=!target.dot.shadow_word_pain.ticking&!target.dot.vampiric_touch.ticking,cycle_targets=1" );
  cop_dotweave->add_action( "shadow_word_death,moving=1,if=movement.remains>=1*gcd" );
  cop_dotweave->add_action( "power_word_shield,moving=1,if=talent.body_and_soul.enabled&movement.distance>=25" );
  cop_dotweave->add_action( "halo,if=talent.halo.enabled&target.distance<=30,moving=1" );
  cop_dotweave->add_action( "divine_star,if=talent.divine_star.enabled&target.distance<=28,moving=1" );
  cop_dotweave->add_action( "cascade,if=talent.cascade.enabled&target.distance<=40,moving=1" );
  cop_dotweave->add_action( "devouring_plague,moving=1" );
  cop_dotweave->add_action( "shadow_word_pain,if=primary_target=0,moving=1,cycle_targets=1" );

  // Clarity of Power with Insanity
  cop_insanity->add_action( "devouring_plague,if=shadow_orb=5|(active_enemies>=5&!buff.insanity.remains)" );
  cop_insanity->add_action( "devouring_plague,if=buff.mental_instinct.remains<(gcd*1.7)&buff.mental_instinct.remains>(gcd*0.7)&buff.mental_instinct.remains" );
  cop_insanity->add_action( "mind_blast,if=glyph.mind_harvest.enabled&mind_harvest=0,cycle_targets=1" );
  cop_insanity->add_action( "mind_blast,if=active_enemies<=5&cooldown_react" );
  cop_insanity->add_action( "shadow_word_death,if=target.health.pct<20&!target.dot.shadow_word_pain.ticking&!target.dot.vampiric_touch.ticking,cycle_targets=1" );
  cop_insanity->add_action( "shadow_word_death,if=target.health.pct<20,cycle_targets=1" );
  cop_insanity->add_action( "devouring_plague,if=shadow_orb>=3&!set_bonus.tier17_2pc&!set_bonus.tier17_4pc&(cooldown.mind_blast.remains<gcd|(target.health.pct<20&cooldown.shadow_word_death.remains<gcd)),cycle_targets=1" );
  cop_insanity->add_action( "devouring_plague,if=shadow_orb>=3&set_bonus.tier17_2pc&!set_bonus.tier17_4pc&(cooldown.mind_blast.remains<=2|(target.health.pct<20&cooldown.shadow_word_death.remains<gcd)),cycle_targets=1" );
  cop_insanity->add_action( "shadowfiend,if=!talent.mindbender.enabled&set_bonus.tier18_2pc" );
  cop_insanity->add_action( "mindbender,if=talent.mindbender.enabled&set_bonus.tier18_2pc" );
  cop_insanity->add_action( "searing_insanity,if=buff.insanity.remains<0.5*gcd&active_enemies>=3&cooldown.mind_blast.remains>0.5*gcd,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  cop_insanity->add_action( "searing_insanity,if=active_enemies>=5,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  cop_insanity->add_action( "mindbender,if=talent.mindbender.enabled" );
  cop_insanity->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  cop_insanity->add_action( "shadow_word_pain,if=remains<(18*0.3)&target.time_to_die>(18*0.75)&miss_react&active_enemies<=5&primary_target=0,cycle_targets=1,max_cycle_targets=5" );
  cop_insanity->add_action( "vampiric_touch,if=remains<(15*0.3+cast_time)&target.time_to_die>(15*0.75+cast_time)&miss_react&active_enemies<=5&primary_target=0,cycle_targets=1,max_cycle_targets=5" );
  cop_insanity->add_action( "insanity,if=buff.insanity.remains<0.5*gcd&active_enemies<=2,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|(cooldown.shadow_word_death.remains<=0.1&target.health.pct<20))" );
  cop_insanity->add_action( "insanity,if=active_enemies<=2,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|(cooldown.shadow_word_death.remains<=0.1&target.health.pct<20))" );
  cop_insanity->add_action( "halo,if=talent.halo.enabled&!set_bonus.tier18_4pc&target.distance<=30&target.distance>=17" );
  cop_insanity->add_action( "cascade,if=talent.cascade.enabled&!set_bonus.tier18_4pc&((active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11)" );
  cop_insanity->add_action( "divine_star,if=talent.divine_star.enabled&!set_bonus.tier18_4pc&active_enemies>2&target.distance<=24" );
  cop_insanity->add_action( "halo,if=talent.halo.enabled&set_bonus.tier18_4pc&buff.premonition.up&target.distance<=30&target.distance>=17" );
  cop_insanity->add_action( "cascade,if=talent.cascade.enabled&set_bonus.tier18_4pc&buff.premonition.up&((active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11)" );
  cop_insanity->add_action( "divine_star,if=talent.divine_star.enabled&set_bonus.tier18_4pc&buff.premonition.up&active_enemies>2&target.distance<=24" );
  cop_insanity->add_action( "mind_sear,if=active_enemies>=8,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  cop_insanity->add_action( "mind_spike" );
  cop_insanity->add_action( "shadow_word_death,moving=1,if=!target.dot.shadow_word_pain.ticking&!target.dot.vampiric_touch.ticking,cycle_targets=1" );
  cop_insanity->add_action( "shadow_word_death,moving=1,if=movement.remains>=1*gcd" );
  cop_insanity->add_action( "power_word_shield,moving=1,if=talent.body_and_soul.enabled&movement.distance>=25" );
  cop_insanity->add_action( "halo,if=talent.halo.enabled&target.distance<=30,moving=1" );
  cop_insanity->add_action( "divine_star,if=talent.divine_star.enabled&target.distance<=28,moving=1" );
  cop_insanity->add_action( "cascade,if=talent.cascade.enabled&target.distance<=40,moving=1" );
  cop_insanity->add_action( "devouring_plague,moving=1" );
  cop_insanity->add_action( "shadow_word_pain,if=primary_target=0,moving=1,cycle_targets=1" );

  // Clarity of Power
  cop->add_action( "shadowfiend,if=!talent.mindbender.enabled&set_bonus.tier18_2pc" );
  cop->add_action( "mindbender,if=talent.mindbender.enabled&set_bonus.tier18_2pc" );
  cop->add_action( "devouring_plague,if=shadow_orb=5&primary_target=0&!target.dot.devouring_plague_dot.ticking&target.time_to_die>=(gcd*4*7%6),cycle_targets=1" );
  cop->add_action( "devouring_plague,if=shadow_orb=5&primary_target=0&target.time_to_die>=(gcd*4*7%6)&(cooldown.mind_blast.remains<=gcd|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20)),cycle_targets=1" );
  cop->add_action( "devouring_plague,if=shadow_orb=5&!set_bonus.tier17_2pc&(cooldown.mind_blast.remains<=gcd|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))" );
  cop->add_action( "devouring_plague,if=shadow_orb=5&set_bonus.tier17_2pc&(cooldown.mind_blast.remains<=gcd*2|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))" );
  cop->add_action( "devouring_plague,if=primary_target=0&buff.mental_instinct.remains<gcd&buff.mental_instinct.remains>(gcd*0.7)&buff.mental_instinct.remains&active_enemies>1,cycle_targets=1" );
  cop->add_action( "devouring_plague,if=buff.mental_instinct.remains<gcd&buff.mental_instinct.remains>(gcd*0.7)&buff.mental_instinct.remains&active_enemies>1" );
  cop->add_action( "devouring_plague,if=shadow_orb>=3&!set_bonus.tier17_2pc&!set_bonus.tier17_4pc&(cooldown.mind_blast.remains<=gcd|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))&primary_target=0&target.time_to_die>=(gcd*4*7%6),cycle_targets=1" );
  cop->add_action( "devouring_plague,if=shadow_orb>=3&!set_bonus.tier17_2pc&!set_bonus.tier17_4pc&(cooldown.mind_blast.remains<=gcd|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))" );
  cop->add_action( "devouring_plague,if=shadow_orb>=3&set_bonus.tier17_2pc&!set_bonus.tier17_4pc&(cooldown.mind_blast.remains<=gcd*2|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))&primary_target=0&target.time_to_die>=(gcd*4*7%6)&active_enemies>1,cycle_targets=1" );
  cop->add_action( "devouring_plague,if=shadow_orb>=3&set_bonus.tier17_2pc&!set_bonus.tier17_4pc&(cooldown.mind_blast.remains<=gcd*2|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))&active_enemies>1" );
  cop->add_action( "devouring_plague,if=shadow_orb>=3&set_bonus.tier17_2pc&talent.mindbender.enabled&!target.dot.devouring_plague_dot.ticking&(cooldown.mind_blast.remains<=gcd*2|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))&primary_target=0&target.time_to_die>=(gcd*4*7%6)&active_enemies=1,cycle_targets=1" );
  cop->add_action( "devouring_plague,if=shadow_orb>=3&set_bonus.tier17_2pc&talent.mindbender.enabled&!target.dot.devouring_plague_dot.ticking&(cooldown.mind_blast.remains<=gcd*2|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))&active_enemies=1" );
  cop->add_action( "devouring_plague,if=shadow_orb>=3&set_bonus.tier17_2pc&talent.surge_of_darkness.enabled&buff.mental_instinct.remains<(gcd*1.4)&buff.mental_instinct.remains>(gcd*0.7)&buff.mental_instinct.remains&(cooldown.mind_blast.remains<=gcd*2|(cooldown.shadow_word_death.remains<=gcd&target.health.pct<20))&primary_target=0&target.time_to_die>=(gcd*4*7%6)&active_enemies=1,cycle_targets=1" );
  cop->add_action( "mind_blast,if=mind_harvest=0,cycle_targets=1" );
  cop->add_action( "mind_blast,if=cooldown_react" );
  cop->add_action( "shadow_word_death,if=target.health.pct<20&!target.dot.shadow_word_pain.ticking&!target.dot.vampiric_touch.ticking,cycle_targets=1" );
  cop->add_action( "shadow_word_death,if=target.health.pct<20,cycle_targets=1" );
  cop->add_action( "mindbender,if=talent.mindbender.enabled" );
  cop->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  cop->add_action( "halo,if=talent.halo.enabled&!set_bonus.tier18_4pc&target.distance<=30&target.distance>=17" );
  cop->add_action( "cascade,if=talent.cascade.enabled&!set_bonus.tier18_4pc&(active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11" );
  cop->add_action( "divine_star,if=talent.divine_star.enabled&!set_bonus.tier18_4pc&active_enemies>3&target.distance<=24" );
  cop->add_action( "halo,if=talent.halo.enabled&set_bonus.tier18_4pc&buff.premonition.up&target.distance<=30&target.distance>=17" );
  cop->add_action( "cascade,if=talent.cascade.enabled&set_bonus.tier18_4pc&buff.premonition.up&(active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11" );
  cop->add_action( "divine_star,if=talent.divine_star.enabled&set_bonus.tier18_4pc&buff.premonition.up&active_enemies>3&target.distance<=24" );
  cop->add_action( "shadow_word_pain,if=remains<(18*0.3)&target.time_to_die>(18*0.75)&miss_react&!ticking&active_enemies<=5&primary_target=0,cycle_targets=1,max_cycle_targets=5" );
  cop->add_action( "vampiric_touch,if=remains<(15*0.3+cast_time)&target.time_to_die>(15*0.75+cast_time)&miss_react&active_enemies<=5&primary_target=0,cycle_targets=1,max_cycle_targets=5" );
  cop->add_action( "divine_star,if=talent.divine_star.enabled&active_enemies=3&target.distance<=24" );
  cop->add_action( "mind_spike,if=active_enemies<=4&buff.surge_of_darkness.react" );
  cop->add_action( "mind_sear,if=active_enemies>=8,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1),target_if=max:spell_targets.mind_sear_tick" );
  cop->add_action( "mind_spike,if=target.dot.devouring_plague_tick.remains&target.dot.devouring_plague_tick.remains<cast_time" );
  cop->add_action( "mind_flay,if=target.dot.devouring_plague_tick.ticks_remain>1&active_enemies>1,chain=1,interrupt_if=(cooldown.mind_blast.remains<=0.1|cooldown.shadow_word_death.remains<=0.1)" );
  cop->add_action( "mind_spike" );
  cop->add_action( "shadow_word_death,moving=1,if=!target.dot.shadow_word_pain.ticking&!target.dot.vampiric_touch.ticking,cycle_targets=1" );
  cop->add_action( "shadow_word_death,moving=1,if=movement.remains>=1*gcd" );
  cop->add_action( "power_word_shield,moving=1,if=talent.body_and_soul.enabled&movement.distance>=25" );
  cop->add_action( "halo,moving=1,if=talent.halo.enabled&target.distance<=30" );
  cop->add_action( "divine_star,if=talent.divine_star.enabled&target.distance<=28,moving=1" );
  cop->add_action( "cascade,if=talent.cascade.enabled&target.distance<=40,moving=1" );
  cop->add_action( "devouring_plague,moving=1" );

  //CoP RoW APLs from Kilee
  cop_row_insanity_spend->add_action( "wait,sec=cooldown.mind_blast.remains,if=cooldown.mind_blast.remains<0.3&cooldown.mind_blast.remains" );
  cop_row_insanity_spend->add_action( "mind_blast" );
  cop_row_insanity_spend->add_action( "insanity,interrupt=1" );
  cop_row_insanity_spend->add_action( "devouring_plague,if=cooldown.mind_blast.remains<=gcd*2&!target.debuff.mental_fatigue.remains<=gcd" );
  cop_row_insanity_spend->add_action( "shadowfiend,if=!(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd)))" );
  cop_row_insanity_spend->add_action( "mind_spike,if=!(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd)))" );
  cop_row_insanity_spend->add_action( "mind_flay,if=(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd))),interrupt=1,chain=1" );

  cop_row_insanity_build->add_action( "wait,sec=cooldown.mind_blast.remains,if=cooldown.mind_blast.remains<0.3&cooldown.mind_blast.remains" );
  cop_row_insanity_build->add_action( "mind_blast" );
  cop_row_insanity_build->add_action( "insanity,interrupt=1" );
  cop_row_insanity_build->add_action( "shadowfiend,if=!(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd)))" );
  cop_row_insanity_build->add_action( "mind_spike,if=!(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd)))" );
  cop_row_insanity_build->add_action( "mind_flay,if=(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd))),interrupt=1,chain=1" );

  cop_row_insanity_death->add_action( "wait,sec=cooldown.mind_blast.remains,if=cooldown.mind_blast.remains<0.3&cooldown.mind_blast.remains" );
  cop_row_insanity_death->add_action( "mind_blast" );
  cop_row_insanity_death->add_action( "insanity,interrupt=1" );
  cop_row_insanity_death->add_action( "devouring_plague,if=cooldown.mind_blast.remains<=gcd*2&!target.debuff.mental_fatigue.remains<=gcd" );
  cop_row_insanity_death->add_action( "shadowfiend,if=!(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd)))" );
  cop_row_insanity_death->add_action( "mind_spike,if=!(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd)))" );
  cop_row_insanity_death->add_action( "mind_flay,if=(!target.debuff.mental_fatigue.up|target.debuff.mental_fatigue.stack<5|(target.debuff.mental_fatigue.remains<=gcd|(target.debuff.mental_fatigue.remains<=gcd*2&cooldown.mindblast.remains<=gcd))),interrupt=1,chain=1" );
}

// Discipline Heal Combat Action Priority List

void priest_t::apl_disc_heal()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def->add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def->add_action( profession_actions[ i ] );

  // Potions
  if ( sim->allow_potions )
  {
    std::string a = "mana_potion,if=mana.pct<=75";
    def->add_action( a );
  }

  if ( race == RACE_BLOOD_ELF )
    def->add_action( "arcane_torrent,if=mana.pct<95" );

  if ( race != RACE_BLOOD_ELF )
  {
    std::vector<std::string> racial_actions = get_racial_actions();
    for ( size_t i = 0; i < racial_actions.size(); i++ )
      def->add_action( racial_actions[ i ] );
  }

  def->add_action( "power_infusion,if=talent.power_infusion.enabled" );
  def->add_action( "power_word_solace,if=talent.power_word_solace.enabled" );
  def->add_action( "mindbender,if=talent.mindbender.enabled&mana.pct<80" );
  def->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  def->add_action( this, "Power Word: Shield" );
  def->add_action( "penance_heal,if=buff.borrowed_time.up" );
  def->add_action( "penance_heal" );
  def->add_action( this, "Flash Heal", "if=buff.surge_of_light.react" );
  def->add_action( "prayer_of_mending" );
  def->add_talent( this, "Clarity of Will" );
  def->add_action( this, "Heal", "if=buff.power_infusion.up|mana.pct>20" );
  def->add_action( "heal" );
}

// Discipline Damage Combat Action Priority List

void priest_t::apl_disc_dmg()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def->add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def->add_action( profession_actions[ i ] );

  // Potions
  if ( sim->allow_potions && true_level >= 80 )
  {
    if ( true_level > 90 )
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
    std::vector<std::string> racial_actions = get_racial_actions();
    for ( size_t i = 0; i < racial_actions.size(); i++ )
      def->add_action( racial_actions[ i ] );
  }

  def->add_talent( this, "Power Infusion", "if=talent.power_infusion.enabled" );

  def->add_action( this, "Shadow Word: Pain", "if=!ticking" );

  def->add_action( this, "Penance" );

  def->add_talent( this, "Power Word: Solace",
                   "if=talent.power_word_solace.enabled" );
  def->add_action( this, "Holy Fire", "if=!talent.power_word_solace.enabled" );

  def->add_action( this, "Smite",
                   "if=glyph.smite.enabled&(dot.power_word_solace.remains+dot."
                   "holy_fire.remains)>cast_time" );
  def->add_action( this, "Shadow Word: Pain", "if=remains<(duration*0.3)" );
  def->add_action( this, "Smite" );
  def->add_action( this, "Shadow Word: Pain" );
}

// Holy Heal Combat Action Priority List

void priest_t::apl_holy_heal()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def->add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def->add_action( profession_actions[ i ] );

  // Potions
  if ( sim->allow_potions )
    def->add_action( "mana_potion,if=mana.pct<=75" );

  std::string racial_condition;
  if ( race == RACE_BLOOD_ELF )
    racial_condition = ",if=mana.pct<=90";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def->add_action( racial_actions[ i ], racial_condition );

  def->add_action( "power_infusion,if=talent.power_infusion.enabled" );
  def->add_action( this, "Lightwell" );

  def->add_action( "power_word_solace,if=talent.power_word_solace.enabled" );
  def->add_action( "mindbender,if=talent.mindbender.enabled&mana.pct<80" );
  def->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  def->add_action( "prayer_of_mending,if=buff.divine_insight.up" );
  def->add_action( "flash_heal,if=buff.surge_of_light.up" );
  def->add_action( "circle_of_healing" );
  def->add_action( "holy_word" );
  def->add_action( "halo,if=talent.halo.enabled" );
  def->add_action( "cascade,if=talent.cascade.enabled" );
  def->add_action( "divine_star,if=talent.divine_star.enabled" );
  def->add_action( "renew,if=!ticking" );
  def->add_action( "heal,if=buff.serendipity.react>=2&mana.pct>40" );
  def->add_action( "prayer_of_mending" );
  def->add_action( "heal" );
}

// Holy Damage Combat Action Priority List

void priest_t::apl_holy_dmg()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def->add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def->add_action( profession_actions[ i ] );

  // Potions
  if ( sim->allow_potions )
  {
    if ( true_level > 90 )
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

  // Racials
  std::string racial_condition;
  if ( race == RACE_BLOOD_ELF )
    racial_condition = ",if=mana.pct<=90";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def->add_action( racial_actions[ i ], racial_condition );

  def->add_action( "power_infusion,if=talent.power_infusion.enabled" );
  def->add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  def->add_action( "mindbender,if=talent.mindbender.enabled" );
  def->add_action(
      "shadow_word_pain,cycle_targets=1,max_cycle_targets=5,if=miss_react&!"
      "ticking" );
  def->add_action( "power_word_solace" );
  def->add_action( "mind_sear,if=spell_targets.mind_sear_tick>=4" );
  def->add_action( "holy_fire" );
  def->add_action( "smite" );
  def->add_action( "holy_word,moving=1" );
  def->add_action( "shadow_word_pain,moving=1" );
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
      if ( primary_role() != ROLE_HEAL )
        apl_disc_dmg();  // DISCIPLINE DAMAGE
      else
        apl_disc_heal();  // DISCIPLINE HEAL
      break;
    case PRIEST_HOLY:
      if ( primary_role() != ROLE_HEAL )
        apl_holy_dmg();  // HOLY DAMAGE
      else
        apl_holy_heal();  // HOLY HEAL
      break;
    default:
      apl_default();  // DEFAULT
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  base_t::reset();

  // Reset Target Data
  for ( size_t i = 0; i < sim->target_list.size(); ++i )
  {
    if ( priest_td_t* td = find_target_data( sim->target_list[ i ] ) )
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
    double extraReduction = 0;

    if ( glyphs.delayed_coalescence->ok() &&
         buffs.dispersion->remains().total_seconds() <= 2.0 )
    {
      extraReduction =
          ( 2.0 - floor( buffs.dispersion->remains().total_seconds() ) ) *
          glyphs.delayed_coalescence->effectN( 2 ).percent();
    }

    s->result_amount *=
        1.0 +
        ( buffs.dispersion->data().effectN( 1 ).percent() + extraReduction );
  }

  if ( buffs.focused_will->check() )
  {
    s->result_amount *= 1.0 +
                        buffs.focused_will->data().effectN( 1 ).percent() *
                            buffs.focused_will->check();
  }
}

// priest_t::invalidate_cache ===============================================

void priest_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_MASTERY && mastery_spells.shield_discipline->ok() )
  {
    player_t::invalidate_cache( CACHE_PLAYER_HEAL_MULTIPLIER );
  }
}

// priest_t::create_options =================================================

void priest_t::create_options()
{
  base_t::create_options();

  add_option( opt_string( "atonement_target", options.atonement_target_str ) );
  add_option( opt_deprecated( "double_dot", "action_list=double_dot" ) );
  add_option(
      opt_int( "initial_shadow_orbs", options.initial_shadow_orbs, 0, 5 ) );
  add_option( opt_bool( "autounshift", options.autoUnshift ) );
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

    if ( !options.atonement_target_str.empty() )
      profile_str += "atonement_target=" + options.atonement_target_str + "\n";

    if ( options.initial_shadow_orbs != 0 )
    {
      profile_str += "initial_shadow_orbs=";
      profile_str += util::to_string( options.initial_shadow_orbs );
      profile_str += "\n";
    }
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
class priest_report_t : public player_report_extension_t
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

struct priest_module_t : public module_t
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
    p->buffs.weakened_soul = new buffs::weakened_soul_t( p );
  }
  void static_init() const override
  {
    items::init();
  }
  void register_hotfixes() const override
  {
    // Missing DBC Values
    hotfix::register_spell(
        "Priest", "missing",
        "Auspicious Spirit travel speed, in yards per second.", 78203,
        hotfix::HOTFIX_FLAG_DEFAULT | hotfix::HOTFIX_FLAG_QUIET )
        .field( "prj_speed" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 6.0 )
        .verification_value( 0.0 );

    hotfix::register_effect(
        "Priest", "missing", "Shadow Orbs base number.", 101429,
        hotfix::HOTFIX_FLAG_DEFAULT | hotfix::HOTFIX_FLAG_QUIET )
        .field( "base_value" )
        .operation( hotfix::HOTFIX_SET )
        .modifier( 3.0 )
        .verification_value( 0.0 );
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
