// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
/*
TODO - Updated 2016/04/03 by Twintop:

Disc / Holy
 Everything

Shadow
 Artifacts Traits
 Shadowmend
 Update Mind Spike implementation to match changes from March 30th

 Setbonuses - Several Setbonuses have to be updated by Blizzard: T17, T16 no
longer work.

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
    dot_t* shadow_word_pain;
    dot_t* vampiric_touch;
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
    buff_t* mind_spike;
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
struct priest_t : public player_t
{
public:
  typedef player_t base_t;

  // Buffs
  struct
  {
    // Talents
    buff_t* power_infusion;
    buff_t* twist_of_fate;
    buff_t* surge_of_light;

    // Discipline
    buff_t* archangel;
    buff_t* borrowed_time;
    buff_t* holy_evangelism;
    buff_t* inner_focus;
    buff_t* spirit_shell;

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
    buff_t* surrender_to_madness;
    buff_t* void_ray;
    buff_t* mind_sear_on_hit_reset;
    buff_t* voidform;
    buff_t* vampiric_embrace;
    buff_t* surge_of_darkness;
    buff_t* dispersion;
    buff_t* lingering_insanity;

    // Set Bonus
    buff_t* empowered_shadows;  // t16 4pc caster
    buff_t* mental_instinct;    // t17 4pc caster
    buff_t* absolution;         // t16 4pc heal holy word
    buff_t* resolute_spirit;    // t16 4pc heal spirit shell
    buff_t* clear_thoughts;     // t17 4pc disc
    buff_t* reperation;         // T18 Disc 2p
    buff_t* premonition;        // T18 Shadow 4pc
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
    const spell_data_t* voidform;
    const spell_data_t* shadowy_apparitions;
  } specs;

  // Mastery Spells
  struct
  {
    const spell_data_t* shield_discipline;
    const spell_data_t* echo_of_light;
    const spell_data_t* madness;
  } mastery_spells;

  // Cooldowns
  struct
  {
    cooldown_t* angelic_feather;
    cooldown_t* chakra;
    cooldown_t* mindbender;
    cooldown_t* penance;
    cooldown_t* power_word_shield;
    cooldown_t* shadowfiend;
    cooldown_t* silence;

	  cooldown_t* mind_blast;
	  cooldown_t* shadow_word_death;
	  cooldown_t* shadow_word_void;
  } cooldowns;

  // Gains
  struct
  {
    gain_t* mindbender;
    gain_t* power_word_solace;
    gain_t* insanity_auspicious_spirits;
    gain_t* insanity_drain;
	  gain_t* insanity_mind_blast;
	  gain_t* insanity_mind_flay;
	  gain_t* insanity_mind_sear;
    gain_t* insanity_mindbender;
    gain_t* insanity_power_infusion;
    gain_t* insanity_shadow_crash;
    gain_t* insanity_shadow_word_death;
    gain_t* insanity_shadow_word_pain_onhit;
    gain_t* insanity_shadow_word_void;
    gain_t* insanity_surrender_to_madness;
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
    proc_t* divine_insight;
    proc_t* divine_insight_overflow;
    proc_t* shadowy_insight;
    proc_t* shadowy_insight_overflow;
    proc_t* serendipity;
    proc_t* serendipity_overflow;
    proc_t* shadowy_apparition;
    proc_t* shadowy_apparition_overflow;
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

  } procs;

  // Special
  struct
  {
    const spell_data_t* surge_of_darkness;
    action_t* echo_of_light;
    actions::spells::shadowy_apparition_spell_t* shadowy_apparitions;
    spell_t* voidform;
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
    std::string atonement_target_str;
    bool autoUnshift;  // Shift automatically out of stance/form
    priest_options_t() : autoUnshift( true )
    {
    }
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
  double composite_melee_haste() const override;
  double composite_melee_speed() const override;
  double composite_spell_haste() const override;
  double composite_spell_speed() const override;
  double composite_spell_power( school_e school ) const override;
  double composite_spell_power_multiplier() const override;
  double composite_spell_crit() const override;
  double composite_melee_crit() const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_absorb_multiplier(
      const action_state_t* s ) const override;
  double composite_player_heal_multiplier(
      const action_state_t* s ) const override;
  double temporary_movement_modifier() const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
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
        if (o().specialization() == PRIEST_SHADOW)
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
      if ( p().o().specialization() == PRIEST_SHADOW )
      {
        p().o().resource_gain(
            RESOURCE_INSANITY,
            p().o().talents.mindbender->effectN( 3 ).base_value(),
            p().gains.fiend );
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
  bool trigger_weakened_souls( priest_t& )
  {
    return buff_t::trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, buff_duration );
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
  typedef priest_action_t base_t;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  typedef Base ab;
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
      divine_aegis_trigger_mask( RESULT_CRIT_MASK ),
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

  timespan_t gcd() const override
  {
    timespan_t g = priest_action_t<spell_t>::gcd();
    timespan_t m = priest_action_t<spell_t>::min_gcd;

    if (g == timespan_t::zero())
    {
      return timespan_t::zero();
    }

    if (priest.specialization() == PRIEST_SHADOW && (priest.buffs.voidform->up() || priest.buffs.lingering_insanity->up()))
    {
      m = m - (m * 0.33); //TODO: Fix when spelldata is playing nice (1.0 + priest.specs.voidform->effectN(4).percent());
    }

    g *= priest.cache.spell_haste();

    if (g < m)
    {
      g = m;
    }

    return g;
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
    amount *= priest.buffs.vampiric_embrace->data().effectN( 1 ).percent() *
              ( 1.0 +
                priest.talents.sanlayn->effectN( 2 )
                    .percent() );  // FIXME additive or multiplicate?

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

  void generate_insanity( double num_amount, gain_t* g = nullptr )
  {
    if (priest.specialization() == PRIEST_SHADOW)
    {
      double amount = num_amount;
      double amount_from_power_infusion = 0.0;
      double amount_from_surrender_to_madness = 0.0;

      if (priest.buffs.power_infusion->check())
      {
        amount_from_power_infusion = (amount * (1.0 + priest.buffs.power_infusion->data().effectN(3).percent())) - amount;
      }

      if (priest.buffs.surrender_to_madness->check())
      {
        amount_from_surrender_to_madness = (amount * (1.0 + priest.talents.surrender_to_madness->effectN(1).percent())) - amount;
      }

      priest.resource_gain(RESOURCE_INSANITY, amount, g, this);

      if (amount_from_power_infusion > 0.0)
      {
        priest.resource_gain(RESOURCE_INSANITY, amount_from_power_infusion, priest.gains.insanity_power_infusion, this);
      }

      if (amount_from_surrender_to_madness > 0.0)
      {
        priest.resource_gain(RESOURCE_INSANITY, amount_from_surrender_to_madness, priest.gains.insanity_surrender_to_madness, this);
      }    
    }
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
    dot_duration   = timespan_t::from_seconds( 6.0 );

    ignore_false_positive = true;
    channeled             = true;
    harmful               = false;
    tick_may_crit         = false;
    // hasted_ticks      = false;
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
    return timespan_t::from_seconds( 6.0 );
  }

  void last_tick( dot_t* d ) override
  {
    priest_spell_t::last_tick( d );
    priest.buffs.dispersion->expire();
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

// Voidform Spell ========================================================

struct voidform_t : public priest_spell_t
{
  voidform_t(priest_t& p, const std::string& options_str)
    : priest_spell_t("voidform", p, p.specs.voidform)
  {
    base_costs[RESOURCE_INSANITY] = 0.0;
  }

  void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.voidform->trigger();

    if (priest.talents.void_lord->ok() && priest.buffs.lingering_insanity->up())
    {
      timespan_t time = priest.buffs.lingering_insanity->remains() - (priest.talents.void_lord->effectN(1).time_value() * 1000);
      priest.buffs.lingering_insanity->extend_duration(player, -time);
    }
    else
    {
      priest.buffs.lingering_insanity->expire();
    }
  }

  bool ready() override
  {
    if (!priest.buffs.voidform->check() && 
        (priest.resources.current[RESOURCE_INSANITY] >= priest.resources.max[RESOURCE_INSANITY] ||
          (priest.talents.legacy_of_the_void->ok() && (priest.resources.current[RESOURCE_INSANITY] >= priest.resources.max[RESOURCE_INSANITY] + priest.talents.legacy_of_the_void->effectN(2).base_value()))))
    {
      return priest_spell_t::ready();
    }
    else
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

// Surrender to Madness =======================================================

struct surrender_to_madness_t : public priest_spell_t
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

  virtual bool ready() override
  {
    if ( !priest.buffs.voidform->check() )
      return false;

    return priest_spell_t::ready();
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
  double insanity_gain;

  shadowy_apparition_spell_t( priest_t& p )
    : priest_spell_t("shadowy_apparitions", p, p.find_spell(78203)),
    insanity_gain(0)
  {
    background          = true;
    proc                = false;
    callbacks           = true;
    may_miss            = false;

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

    if (priest.talents.auspicious_spirits->ok())
    {
      insanity_gain += 4.0; // * (1.0 + priest.talents.auspicious_spirits->effectN(1).percent())
      generate_insanity(insanity_gain, priest.gains.insanity_auspicious_spirits);
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
  double insanity_gain;

  mind_blast_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "mind_blast", player,
                      player.find_class_spell( "Mind Blast" ) ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) * ( 1.0 + player.talents.fortress_of_the_mind->effectN( 2 ).percent() ) )
  {
    parse_options( options_str );
    is_mind_spell       = true;

	  cooldown->charges = data().charges();
	  cooldown->duration = data().charge_cooldown();

    spell_power_mod.direct *= 1.0 + player.talents.fortress_of_the_mind->effectN( 4 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );
    generate_insanity( insanity_gain, priest.gains.insanity_mind_blast );

    if ( priest.talents.mind_spike->ok() )
    {
      priest_td_t& td = get_td( s->target );

      td.buffs.mind_spike->up();  // benefit tracking
      td.buffs.mind_spike->expire();
    }
  }

  double cooldown_reduction() const override
  {
    return spell_t::cooldown_reduction() * composite_haste();
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
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
    cd_duration = ( cooldown->duration + priest.specs.voidform->effectN( 4 ).time_value() * priest.buffs.voidform->up() ) * composite_haste();

    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows->up();  // benefit tracking
    priest.buffs.empowered_shadows->expire();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double am = base_t::composite_target_multiplier( t );

    priest_td_t& td = get_td( t );
    if ( priest.talents.mind_spike->ok() && td.buffs.mind_spike->check() )
    {
      am *= 1.0 + td.buffs.mind_spike->check_stack_value();
    }

    return am;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

    if ( priest.buffs.empowered_shadows->check() )
      d *= 1.0 +
           priest.buffs.empowered_shadows->current_value *
               priest.buffs.empowered_shadows->check();

    d *= 1.0 + priest.sets.set( SET_CASTER, T16, B2 )->effectN( 1 ).percent();

    return d;
  }
};

// Mind Spike Spell =========================================================

struct mind_spike_t : public priest_spell_t
{
  double insanity_gain;

  mind_spike_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "mind_spike", p, p.talents.mind_spike ),
      insanity_gain( data().effectN( 3 ).resource( RESOURCE_INSANITY ) * (1.0 + p.talents.fortress_of_the_mind->effectN(1).percent() ))
  {
    parse_options( options_str );
    is_mind_spell       = true;
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    generate_insanity( insanity_gain, priest.gains.insanity_mind_flay );

    if ( result_is_hit( s->result ) )
    {
      priest_td_t& td = get_td( s->target );
      td.buffs.mind_spike->trigger();
    }
  }

  void execute() override
  {
    priest_spell_t::execute();

    if (priest.talents.void_ray->ok())
      priest.buffs.void_ray->trigger();
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    if (priest.talents.void_ray->ok() && priest.buffs.void_ray->check())
      am *= 1.0 +
      priest.buffs.void_ray->check() *
      priest.buffs.void_ray->data().effectN(1).percent();

    return am;
  }

  virtual bool ready() override
  {
    if ( !priest.talents.mind_spike->ok() )
      return false;

    return priest_spell_t::ready();
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t : public priest_spell_t
{
  double insanity_gain;

  //TODO: Mind Sear is missing damage information in spell data
  mind_sear_tick_t( priest_t& p, const spell_data_t* mind_sear )
    : priest_spell_t( "mind_sear_tick", p, mind_sear->effectN( 1 ).trigger() ),
    insanity_gain(1) //TODO: Missing from spell data
  {
    radius              = data().effectN( 1 ).radius();
    background          = true;
    dual                = true;
    aoe                 = -1;
    callbacks           = false;
    direct_tick         = true;
    use_off_gcd         = true;
  }

  void impact(action_state_t* s) override
  {
    generate_insanity(insanity_gain, priest.gains.insanity_mind_sear);
  }
};

struct mind_sear_t : public priest_spell_t
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

    tick_action = new mind_sear_tick_t( p, p.find_class_spell( "Mind Sear" ) );
  }

  double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

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

// Shadow Word Death Spell ==================================================

struct shadow_word_death_t : public priest_spell_t
{
  double insanity_gain;

  shadow_word_death_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_death", p,
                      p.find_class_spell( "Shadow Word: Death" ) ),
	  insanity_gain( p.find_spell( 190714 )->effectN( 1 ).resource(RESOURCE_INSANITY) )
  {
    parse_options( options_str );

	// FIXME remove when spelldata is fixed
	if (p.talents.reaper_of_souls->ok())
	{
	  base_multiplier *= 1.5;
	}
	else
	{
	  base_multiplier *= 4.0;
	}

    base_multiplier *= 1.0 + p.sets.set( SET_CASTER, T13, B2 )->effectN( 1 ).percent();
  }

  void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows->up();  // benefit tracking
    priest.buffs.empowered_shadows->expire();
  }

  void impact( action_state_t* s ) override
  {
	double save_health_percentage = s->target->health_percentage();

    priest_spell_t::impact( s );

	if (result_is_hit(s->result))
	{
		if (priest.talents.reaper_of_souls->ok() ||
			((save_health_percentage > 0.0) && (s->target->health_percentage() <= 0.0)))
		{
			generate_insanity(insanity_gain, priest.gains.insanity_shadow_word_death);
		}
	}
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double d = priest_spell_t::composite_da_multiplier( state );

	if ( priest.buffs.empowered_shadows->check() )
	{
	  d *= 1.0 + priest.buffs.empowered_shadows->current_value * priest.buffs.empowered_shadows->check();
	}

    return d;
  }

  double cooldown_reduction() const override
  {
    return spell_t::cooldown_reduction() * composite_haste();
  }

  bool ready() override
  {
    if ( !priest_spell_t::ready() )
      return false;

	if ( priest.talents.reaper_of_souls->ok() && target->health_percentage() < 35.0 
		|| target->health_percentage() < 20.0 )  // FIXME Spelldata effectN(2)?
	{
	  return true;
	}

    return false;
  }
};

// Mind Flay Spell ==========================================================
struct mind_flay_t : public priest_spell_t
{
  double insanity_gain;

  mind_flay_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "mind_flay", p,
                      p.find_specialization_spell( "Mind Flay" ) ),
                      insanity_gain(data().effectN(3).resource(RESOURCE_INSANITY) * (1.0 + p.talents.fortress_of_the_mind->effectN(1).percent()))
  {
    parse_options( options_str );

    may_crit      = false;
    channeled     = true;
    hasted_ticks  = false;
    use_off_gcd   = true;
    is_mind_spell = true;

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
      priest.buffs.void_ray->trigger();

    generate_insanity( insanity_gain, priest.gains.insanity_mind_flay );
  }

  virtual bool ready() override
  {
    if ( priest.talents.mind_spike->ok() )
      return false;

    return priest_spell_t::ready();
  }
};

// Shadow Crash Spell ===================================================

struct shadow_crash_t : public priest_spell_t
{
  double insanity_gain;

  shadow_crash_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_crash", p, p.talents.shadow_crash ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    aoe = -1;
  }

  void execute() override
  {
    priest_spell_t::execute();

    generate_insanity( insanity_gain, priest.gains.insanity_shadow_crash );
  }
};

// Shadow Word Pain Spell ===================================================

struct shadow_word_pain_t : public priest_spell_t
{
  double insanity_gain;

  shadow_word_pain_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "shadow_word_pain", p,
                      p.find_class_spell("Shadow Word: Pain")),
                      insanity_gain(data().effectN(3).resource(RESOURCE_INSANITY))
  {
    parse_options( options_str );

    may_crit  = true;
    tick_zero = false;

    base_multiplier *= 1.0 + p.sets.set( SET_CASTER, T13, B4 )->effectN( 1 ).percent();

    base_crit += p.sets.set( SET_CASTER, T14, B2 )->effectN( 1 ).percent();

    dot_duration += p.sets.set( SET_CASTER, T14, B4 )->effectN( 1 ).time_value();

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

  void impact(action_state_t* s) override
  {
    priest_spell_t::impact(s);

    generate_insanity(insanity_gain, priest.gains.insanity_shadow_word_pain_onhit);
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

    if (d->state->result_amount > 0)
    {
      if (trigger_shadowy_insight())
      {
        priest.procs.shadowy_insight->occur();
      }
    }
  }

  double cost() const override
  {
	  double c = priest_spell_t::cost();

	  if (priest.specialization() == PRIEST_SHADOW)
		  return 0.0;

	  return c;
  }

  virtual double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.madness->ok() )
      m *= 1.0 + priest.cache.mastery_value();

    return m;
  }
};

// Shadow Word Void Spell ==================================================

struct shadow_word_void_t : public priest_spell_t
{
  double insanity_gain;

  shadow_word_void_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "shadow_word_void", player,
                      player.talents.shadow_word_void ),
      insanity_gain( data().effectN( 2 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options_str );

    cooldown->charges = data().charges();
    cooldown->duration = data().charge_cooldown();
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
  double insanity_gain;

  vampiric_touch_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "vampiric_touch", p,
                      p.find_class_spell("Vampiric Touch")),
                      insanity_gain(data().effectN(3).resource(RESOURCE_INSANITY))
  {
    parse_options( options_str );
    may_crit = false;

    dot_duration +=
        p.sets.set( SET_CASTER, T14, B4 )->effectN( 1 ).time_value();

    spell_power_mod.tick *= 1.0 + p.talents.sanlayn->effectN( 1 ).percent();

    if ( priest.specs.shadowy_apparitions->ok() &&
         priest.sets.has_set_bonus( SET_CASTER, T15, B4 ) &&
         !priest.active_spells.shadowy_apparitions )
    {
      priest.active_spells.shadowy_apparitions =
          new shadowy_apparition_spell_t( p );
    }
  }
  
  void impact(action_state_t* s) override
  {
    priest_spell_t::impact(s);

    generate_insanity(insanity_gain, priest.gains.insanity_vampiric_touch_onhit);
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    priest.resource_gain( RESOURCE_HEALTH, d->state->result_amount,
                          priest.gains.vampiric_touch_health );

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

  virtual double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.madness->ok() )
      m *= 1.0 + priest.cache.mastery_value();

    return m;
  }
};

// Void Bolt Spell =========================================================

struct void_bolt_t : public priest_spell_t
{
  double insanity_gain;

  void_bolt_t( priest_t& player, const std::string& options_str )
    : priest_spell_t( "void_bolt", player,
                      player.find_specialization_spell( "Void Bolt" ) ),
	  insanity_gain(data().effectN(3).resource(RESOURCE_INSANITY))
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    priest_td_t& td = get_td( s->target );
    timespan_t extend_duration =
        timespan_t::from_seconds( data().effectN( 2 ).base_value() );
    // extend_duration *= composite_haste(); FIXME Check is it is reduced by
    // haste or not.
    if ( td.dots.shadow_word_pain->is_ticking() )
    {
      td.dots.shadow_word_pain->refresh_duration();
    }

    if ( td.dots.vampiric_touch->is_ticking() )
    {
      td.dots.vampiric_touch->refresh_duration();
    }

    generate_insanity(insanity_gain, priest.gains.insanity_void_bolt);
  }

  double cooldown_reduction() const override
  {
    return spell_t::cooldown_reduction() * composite_haste();
  }

  bool ready() override
  {
    if ( !priest.buffs.voidform->check() )
      return false;

    return priest_spell_t::ready();
  }

  virtual double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.madness->ok() )
      m *= 1.0 + priest.cache.mastery_value();

    return m;
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

    may_crit       = false;
    may_miss       = false;
    channeled      = true;
    tick_zero      = true;
    dot_duration   = timespan_t::from_seconds( 2.0 );
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

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
  cooldown_t* hw_chastise;

  smite_t( priest_t& p, const std::string& options_str )
    : priest_spell_t( "smite", p, p.find_class_spell( "Smite" ) ),
      hw_chastise( p.get_cooldown( "holy_word_chastise" ) )
  {
    parse_options( options_str );

    procs_courageous_primal_diamond = false;

    can_trigger_atonement = priest.specs.atonement->ok();
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
  }

  void trigger_atonement( dmg_e type, action_state_t* s )
  {
    double atonement_dmg = s->result_amount;  // Normal dmg

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

    aoe = 5;
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

    buffs::weakened_soul_t* weakened_soul =
        debug_cast<buffs::weakened_soul_t*>( s->target->buffs.weakened_soul );
    weakened_soul->trigger_weakened_souls( priest );
    priest.buffs.borrowed_time->trigger();

    // Talent
    if ( priest.talents.body_and_soul->ok() )
      s->target->buffs.body_and_soul->trigger();
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
      double tick_dmg  = calculate_tick_amount( d->state, d->current_stack() );
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

/* Custom voidform buff
 */
struct voidform_t : public priest_buff_t<buff_t>
{
  struct insanity_loss_event_t : player_event_t
  {
    voidform_t* vf;

    insanity_loss_event_t( voidform_t* s )
      : player_event_t( *s->player ), vf( s )
    {
      add_event( timespan_t::from_seconds(
          0.1 ) );  // FIXME Is there spelldata for tick intervall ?
    }

    virtual const char* name() const override
    {
      return "voidform_insanity_loss";
    }

    virtual void execute() override
    {
      // http://howtopriest.com/viewtopic.php?f=95&t=8069&start=20#p69794
      // ----
      // Updated 2016-04-03 by Twintop:
      // Drain starts at 8 over 1 second and increases by 1 over 2 seconds per stack of Voidform.
      // Ticks happen ever 0.1 sec.
      // CHECK ME: Triggering Voidform in-game and not using any abilities results in 11 stacks. The implementation below results in 11 stacks as well by subtracting 1 from the current stackcount.
      // TODO: Use Spelldata
      auto priest = debug_cast<priest_t*>( player() );
      double insanity_loss = (8 * 0.1) + (( priest->buffs.voidform->check() - 1) / 2 ) * 0.1;
      priest->resource_loss( RESOURCE_INSANITY, insanity_loss,
                             priest->gains.insanity_drain );

      if ( priest->resources.current[ RESOURCE_INSANITY ] <= 0.0 )
      {
        if ( sim().debug )
        {
          sim().out_debug << "Insanity-loss event cancels voidform because priest is out of insanity.";
        }
        vf->insanity_loss = nullptr; // avoid double-canceling
        priest->buffs.voidform->expire();
        return;
      }

      vf->insanity_loss = new ( sim() ) insanity_loss_event_t( vf );
    }
  };

  insanity_loss_event_t* insanity_loss;

  voidform_t( priest_t& p )
    : base_t( p, buff_creator_t( &p, "voidform" )
                     .spell( p.find_spell( 194249 ) )
                     .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                     .add_invalidate( CACHE_HASTE )
                     .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER ) ),
      insanity_loss( nullptr )
  {
  }

  bool trigger( int stacks, double value, double chance,
                timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    assert( insanity_loss == nullptr );
    insanity_loss = new ( *sim ) insanity_loss_event_t( this );

    return r;
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    if ( priest.buffs.lingering_insanity->check() )
      priest.buffs.lingering_insanity->expire();

    priest.buffs.lingering_insanity->trigger( expiration_stacks );
    priest.active_spells.voidform->cancel();

    if ( priest.buffs.surrender_to_madness->check() )
    {
      priest.demise();
      // FIXME Add some waiting time here
      priest.arise();
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
  : actor_target_data_t( target, &p ), dots(), buffs(), priest( p )
{
  dots.holy_fire         = target->get_dot( "holy_fire", &p );
  dots.power_word_solace = target->get_dot( "power_word_solace", &p );
  dots.shadow_word_pain  = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch    = target->get_dot( "vampiric_touch", &p );
  dots.renew             = target->get_dot( "renew", &p );

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

  if ( priest.talents.mind_spike->ok() )
    buffs.mind_spike = buff_creator_t( *this, "mind_spike" )
                           .spell( p.talents.mind_spike )
                           .default_value( 0.5 );  // FIXME no value in Data?

  target->callbacks_on_demise.push_back(
      std::bind( &priest_td_t::target_demise, this ) );
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
  cooldowns.penance           = get_cooldown( "penance" );
  cooldowns.power_word_shield = get_cooldown( "power_word_shield" );
  cooldowns.shadowfiend       = get_cooldown( "shadowfiend" );
  cooldowns.silence           = get_cooldown( "silence" );

  cooldowns.mind_blast = get_cooldown("mind_blast");
  cooldowns.shadow_word_death = get_cooldown("shadow_word_death");
  cooldowns.shadow_word_void  = get_cooldown( "shadow_word_void" );

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
  gains.mindbender = get_gain("Mana Gained from Mindbender");
  gains.power_word_solace = get_gain("Mana Gained from Power Word: Solace");
  gains.insanity_auspicious_spirits = get_gain("Insanity Gained from Auspicious Spirits");
  gains.insanity_drain = get_gain("Insanity Drained by Voidform");
  gains.insanity_mind_blast = get_gain("Insanity Gained from Mind Blast");
  gains.insanity_mind_flay = get_gain("Insanity Gained from Mind Flay or Mind Spike");
  gains.insanity_mind_sear = get_gain("Insanity Gained from Mind Sear");
  gains.insanity_mindbender = get_gain("Insanity Gained from Mindbender");
  gains.insanity_power_infusion = get_gain("Insanity Gained from Power Infusion");
  gains.insanity_shadow_crash = get_gain("Insanity Gained from Shadow Crash");
  gains.insanity_shadow_word_death = get_gain("Insanity Gained from Shadow Word: Death");
  gains.insanity_shadow_word_pain_onhit = get_gain("Insanity Gained from Shadow Word: Pain Casts");
  gains.insanity_shadow_word_void = get_gain("Insanity Gained from Shadow Word: Void");
  gains.insanity_surrender_to_madness = get_gain("Insanity Gained from Surrender to Madness");
  gains.insanity_vampiric_touch_onhit = get_gain("Insanity Gained from Vampiric Touch Casts");
  gains.insanity_void_bolt = get_gain("Insanity Gained from Void Bolt");
  gains.vampiric_touch_health = get_gain("Health from Vampiric Touch Ticks");
}

/* Construct priest procs
 *
 */
void priest_t::create_procs()
{
  procs.shadowy_apparition = get_proc("Shadowy Apparition Procced");
  procs.shadowy_apparition = get_proc("Shadowy Apparition Insanity lost to overflow");
  procs.divine_insight = get_proc("Divine Insight Instant Prayer of Mending");
  procs.divine_insight_overflow = get_proc("Divine Insight Instant Prayer of Mending lost to overflow");
  procs.shadowy_insight = get_proc("Shadowy Insight Mind Blast CD Reset from Shadow Word: Pain");
  procs.shadowy_insight_overflow = get_proc("Shadowy Insight Mind Blast CD Reset lost to overflow");
  procs.surge_of_light = get_proc("Surge of Light");
  procs.surge_of_light_overflow = get_proc("Surge of Light lost to overflow");
  procs.t15_2pc_caster = get_proc("Tier15 2pc caster");
  procs.t15_4pc_caster = get_proc("Tier15 4pc caster");
  procs.t15_2pc_caster_shadow_word_pain = get_proc("Tier15 2pc caster Shadow Word: Pain Extra Tick");
  procs.t15_2pc_caster_vampiric_touch = get_proc("Tier15 2pc caster Vampiric Touch Extra Tick");
  procs.t17_2pc_caster_mind_blast_reset = get_proc("Tier17 2pc Mind Blast CD Reduction occurances");
  procs.t17_2pc_caster_mind_blast_reset_overflow = get_proc("Tier17 2pc Mind Blast CD Reduction occurances lost to overflow");
  procs.t17_2pc_caster_mind_blast_reset_overflow_seconds = get_proc("Tier17 2pc Mind Blast CD Reduction in seconds (total)");
  procs.serendipity = get_proc("Serendipity (Non-Tier 17 4pc)");
  procs.serendipity_overflow = get_proc("Serendipity lost to overflow (Non-Tier 17 4pc)");
  procs.t17_4pc_holy = get_proc("Tier17 4pc Serendipity");
  procs.t17_4pc_holy_overflow = get_proc("Tier17 4pc Serendipity lost to overflow");
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

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  if ( specs.voidform->ok() && dbc::is_school( SCHOOL_SHADOWFROST, school ) &&
       buffs.voidform->check() )
  {
    if ( talents.void_lord->ok() )
      m *= 1.0 + talents.void_lord->effectN( 1 ).percent();
    else
      m *= 1.0 + buffs.voidform->data().effectN( 1 ).percent();
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

  if ( mastery_spells.shield_discipline->ok() )
  {
    m *= 1.0 +
         cache.mastery() *
             mastery_spells.shield_discipline->effectN( 3 ).mastery_value();
  }
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
// priest_t::temporary_movement_modifier =======================================

double priest_t::temporary_movement_modifier() const
{
  double speed = player_t::temporary_movement_modifier();

  return speed;
}

// priest_t::composite_attribute_multiplier =================================

double priest_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = base_t::composite_attribute_multiplier( attr );

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
  if ( name == "spirit_shell" )
    return new spirit_shell_t( *this, options_str );

  // Shadow
  if ( name == "mind_blast" )
    return new mind_blast_t( *this, options_str );
  if ( name == "mind_flay" )
    return new mind_flay_t( *this, options_str );
  if ( name == "mind_spike" )
    return new mind_spike_t( *this, options_str );
  if ( name == "mind_sear" )
    return new mind_sear_t( *this, options_str );
  if ( name == "shadow_crash" )
    return new shadow_crash_t( *this, options_str );
  if ( name == "shadow_word_death" )
    return new shadow_word_death_t( *this, options_str );
  if ( name == "shadow_word_pain" )
    return new shadow_word_pain_t( *this, options_str );
  if ( name == "shadow_word_void" )
    return new shadow_word_void_t(*this, options_str);
  if (name == "vampiric_touch")
    return new vampiric_touch_t(*this, options_str);
  if ( name == "void_bolt" )
    return new void_bolt_t( *this, options_str );
  if (name == "voidform")
    return new voidform_t(*this, options_str);

  if ((name == "shadowfiend") || (name == "mindbender"))
  {
    if (talents.mindbender->ok())
      return new summon_mindbender_t(*this, options_str);
    else
      return new summon_shadowfiend_t(*this, options_str);
  }

  //Disc+Holy
  if (name == "penance")
    return new penance_t(*this, options_str);
  if ( name == "smite" )
    return new smite_t( *this, options_str );
  if ( ( name == "holy_fire" ) || ( name == "power_word_solace" ) )
  {
    if ( talents.power_word_solace->ok() )
      return new power_word_solace_t( *this, options_str );
    else
      return new holy_fire_t( *this, options_str );
  }
  if ( name == "cascade" )
    return new cascade_t( *this, options_str );
  if ( name == "halo" )
    return new halo_t( *this, options_str );
  if ( name == "divine_star" )
    return new divine_star_t( *this, options_str );

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
  specs.voidform = find_specialization_spell( "Voidform" );
  specs.shadowy_apparitions =
      find_specialization_spell( "Shadowy Apparitions" );

  // Mastery Spells
  mastery_spells.shield_discipline = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light     = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.madness           = find_mastery_spell( PRIEST_SHADOW );

  ///////////////
  // Glyphs    //
  ///////////////

  // All Specs
  glyphs.angels            = find_glyph_spell( "Glyph of Angels" );
  glyphs.confession        = find_glyph_spell( "Glyph of Confession" );
  glyphs.holy_resurrection = find_glyph_spell( "Glyph of Holy Resurrection" );
  glyphs.shackle_undead    = find_glyph_spell( "Glyph of Shackle Undead" );
  glyphs.the_heavens       = find_glyph_spell( "Glyph of the Heavens" );
  glyphs.the_sha           = find_glyph_spell( "Glyph of the Sha" );

  // Discipline
  glyphs.borrowed_time = find_glyph_spell( "Glyph of Borrowed Time" );

  // Holy
  glyphs.inspired_hymns = find_glyph_spell( "Glyph of Inspired Hymns" );
  glyphs.the_valkyr     = find_glyph_spell( "Glyph of the Val'kyr" );

  // Shadow
  glyphs.dark_archangel  = find_glyph_spell( "Glyph of Dark Archangel" );
  glyphs.shadow          = find_glyph_spell( "Glyph of Shadow" );
  glyphs.shadow_ravens   = find_glyph_spell( "Glyph of Shadow Ravens" );
  glyphs.shadowy_friends = find_glyph_spell( "Glyph of Shadowy Friends" );

  if ( mastery_spells.echo_of_light->ok() )
    active_spells.echo_of_light = new actions::heals::echo_of_light_t( *this );

  if ( specs.voidform->ok() )
    active_spells.voidform = new actions::spells::voidform_t( *this, "" );

  // Range Based on Talents
  if ( talents.divine_star->ok() )
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

  buffs.shadowy_insight =
      buff_creator_t( this, "shadowy_insight" )
          .spell( talents.shadowy_insight )
          .chance( talents.shadowy_insight->effectN( 4 ).percent() );

  buffs.void_ray = buff_creator_t( this, "void_ray" )
                       .spell( talents.void_ray->effectN( 1 ).trigger() );

  buffs.surrender_to_madness = buff_creator_t( this, "surrender_to_madness" )
                                   .spell( talents.surrender_to_madness );

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

  // Shadow

  buffs.voidform = new buffs::voidform_t( *this );

  buffs.lingering_insanity = buff_creator_t( this, "lingering_insanity" )
                                 .spell( find_spell( 197937 ) )
                                 .add_invalidate( CACHE_HASTE );

  buffs.vampiric_embrace =
      buff_creator_t( this, "vampiric_embrace",
                      find_class_spell( "Vampiric Embrace" ) )
          .duration( find_class_spell( "Vampiric Embrace" )->duration() );

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
          if ( talents.auspicious_spirits->ok() )
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

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list->add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    default_list->add_action( profession_actions[ i ] );

  // Potions

  // Racials

  // Choose which APL to use based on talents and fight conditions.

  default_list->add_action( "call_action_list,name=main" );

  main->add_action("voidform");
  main->add_action("power_infusion,if=talent.power_infusion.enabled");
  main->add_action("void_bolt");
  main->add_action("mind_blast");
  main->add_action("shadow_word_death");
  main->add_action("shadow_word_void,if=talent.shadow_word_void.enabled");
  main->add_action("mindbender,if=talent.mindbender.enabled");
  main->add_action("shadow_word_pain,if=!ticking");
  main->add_action("vampiric_touch,if=!ticking");
  main->add_action("shadow_crash,if=talent.shadow_crash.enabled");
  main->add_action("shadowfiend,if=!talent.mindbender.enabled");
  main->add_action("mind_flay,if=!talent.mind_spike.enabled");
  main->add_action("mind_spike,if=talent.mind_spike.enabled");
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
    /*case PRIEST_DISCIPLINE:
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
      break;*/
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
