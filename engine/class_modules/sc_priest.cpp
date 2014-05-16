// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

/* WOD TODO (when new dbc is up):
 *
 * - Greater Heal has been renamed Heal.
 * - Cascade, Divine Star, and Halo no longer heal allies but are instant cast
 *   for Shadow Priests, or damage enemies for Discipline or Holy Priests.
 */
namespace { // UNNAMED NAMESPACE

/* Forward declarations
 */
struct priest_t;

/* Priest target data
 * Contains target specific things
 */
struct priest_td_t final : public actor_pair_t
{
public:
  struct dots_t
  {
    dot_t* devouring_plague_tick;
    dot_t* shadow_word_pain;
    dot_t* vampiric_touch;
    dot_t* holy_fire;
    dot_t* power_word_solace;
    dot_t* renew;
  } dots;

  struct buffs_t
  {
    absorb_buff_t* divine_aegis;
    absorb_buff_t* power_word_shield;
    absorb_buff_t* spirit_shell;
    buff_t* holy_word_serenity;
  } buffs;
  priest_t& priest;

  priest_td_t( player_t* target, priest_t& p );
};

/* Priest class definition
 *
 * Derived from player_t. Contains everything that defines the priest class.
 */
struct priest_t final : public player_t
{
public:
  typedef player_t base_t;

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
    buff_t* saving_grace_penalty;

    // Holy
    buff_t* chakra_chastise;
    buff_t* chakra_sanctuary;
    buff_t* chakra_serenity;
    buff_t* serendipity;

    // Shadow
    buff_t* divine_insight_shadow;
    buff_t* shadow_word_death_reset_cooldown;
    buff_t* glyph_mind_spike;
    buff_t* shadowform;
    buff_t* vampiric_embrace;
    buff_t* surge_of_darkness;
    buff_t* dispersion;

    // Set Bonus
    buff_t* empowered_shadows; // t16 4pc caster
    buff_t* absolution; // t16 4pc heal holy word
    buff_t* resolute_spirit; // t16 4pc heal spirit shell
  } buffs;

  // Talents
  struct
  {
    const spell_data_t* void_tendrils;
    const spell_data_t* psychic_scream;
    const spell_data_t* dominate_mind;

    const spell_data_t* body_and_soul;
    const spell_data_t* angelic_feather;
    const spell_data_t* phantasm;

    const spell_data_t* from_darkness_comes_light;
    const spell_data_t* mindbender;
    const spell_data_t* solace_and_insanity;

    const spell_data_t* desperate_prayer;
    const spell_data_t* spectral_guise;
    const spell_data_t* angelic_bulwark;

    const spell_data_t* twist_of_fate;
    const spell_data_t* power_infusion;
    const spell_data_t* divine_insight;

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
    // General

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
    const spell_data_t* train_of_thought;

    // Holy
    const spell_data_t* meditation_holy;
    const spell_data_t* rapid_renewal;
    const spell_data_t* serendipity;

    // Shadow
    const spell_data_t* devouring_plague;
    const spell_data_t* mind_surge;
    const spell_data_t* shadowform;
    const spell_data_t* shadowy_apparitions;
    const spell_data_t* shadow_orbs;
  } specs;

  // Mastery Spells
  struct
  {
    const spell_data_t* shield_discipline;
    const spell_data_t* echo_of_light;
    const spell_data_t* mental_anguish;
  } mastery_spells;

  struct perk_spells_t
  {
    const spell_data_t* enhanced_mind_flay;
    const spell_data_t* enhanced_shadow_orbs;
    const spell_data_t* enhanced_shadow_word_death;
    const spell_data_t* improved_shadow_word_pain;
    const spell_data_t* improved_vampiric_touch;
  } perks;

  // Cooldowns
  struct
  {
    cooldown_t* chakra;
    cooldown_t* inner_focus;
    cooldown_t* mindbender;
    cooldown_t* mind_blast;
    cooldown_t* penance;
    cooldown_t* shadowfiend;
  } cooldowns;

  // Gains
  struct
  {
    gain_t* devouring_plague_health;
    gain_t* dispersion;
    gain_t* mindbender;
    gain_t* power_word_solace;
    gain_t* shadowfiend;
    gain_t* shadow_orb_mb;
    gain_t* shadow_orb_swd;
    gain_t* vampiric_touch_mana;
    gain_t* vampiric_touch_mastery_mana;
    gain_t* auspicious_spirits;
    gain_t* clarity_of_power_mind_spike;
  } gains;

  // Benefits
  struct
  {
    benefit_t* smites_with_glyph_increase;
  } benefits;

  // Procs
  struct
  {
    proc_t* divine_insight_shadow;
    proc_t* mastery_extra_tick;
    proc_t* mind_spike_dot_removal;
    proc_t* shadowy_apparition;
    proc_t* surge_of_darkness;
    proc_t* surge_of_light;
    proc_t* surge_of_light_overflow;
    proc_t* t15_2pc_caster;
    proc_t* t15_4pc_caster;
    proc_t* t15_2pc_caster_shadow_word_pain;
    proc_t* t15_2pc_caster_vampiric_touch;
  } procs;

  // Special
  struct
  {
    const spell_data_t* surge_of_darkness;
    action_t* echo_of_light;
  } active_spells;

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
    bool autoUnshift; // Shift automatically out of stance/form
    priest_options_t() : initial_shadow_orbs( 0 ), autoUnshift( true ) {}
  } options;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* circle_of_healing;
    const spell_data_t* dark_binding;
    const spell_data_t* devouring_plague;
    const spell_data_t* dispersion;
    const spell_data_t* borrowed_time;
    const spell_data_t* holy_fire;
    const spell_data_t* holy_nova;
    const spell_data_t* lightwell;
    const spell_data_t* mind_blast;
    const spell_data_t* mind_flay;
    const spell_data_t* mind_spike;
    const spell_data_t* penance;
    const spell_data_t* power_word_shield;
    const spell_data_t* prayer_of_mending;
    const spell_data_t* renew;
    const spell_data_t* shadow_word_death;
    const spell_data_t* smite;
    const spell_data_t* vampiric_embrace;

    // WoD
    const spell_data_t* free_action;
  } glyphs;

  priest_t( sim_t* sim, const std::string& name, race_e r ) :
    player_t( sim, PRIEST, name, r ),
    // initialize containers. For POD containers this sets all elements to 0.
    // use eg. buffs( buffs_t() ) instead of buffs() to help certain old compilers circumvent their bugs
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
    pets(),
    options(),
    glyphs()
  {
    base.distance = 27.0; //Halo

    create_cooldowns();
    create_gains();
    create_procs();
    create_benefits();
  }

  // Function Definitions
  virtual void      init_base_stats() override;
  virtual void      init_spells() override;
  virtual void      create_buffs() override;
  virtual void      init_scaling() override;
  virtual void      reset() override;
  virtual void      create_options() override;
  virtual bool      create_profile( std::string& profile_str, save_e = SAVE_ALL, bool save_html = false ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual void      copy_from( player_t* source ) override;
  virtual set_e       decode_set( const item_t& ) const override;
  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual void      combat_begin() override;
  virtual double    composite_armor() const override;
  virtual double    composite_spell_haste() const override;
  virtual double    composite_spell_speed() const override;
  virtual double    composite_spell_power_multiplier() const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_heal_multiplier( school_e school ) const override;
  virtual double    temporary_movement_modifier() const;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* ) override;
  virtual void pre_analyze_hook() override;
  virtual void      init_action_list() override;
  virtual priest_td_t* get_target_data( player_t* target ) const override;

private:
  // Construction helper functions for priest_t members
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
  void fixup_atonement_stats( const std::string& trigger_spell_name, const std::string& atonement_spell_name );

  target_specific_t<priest_td_t*> target_data;
};

namespace pets {

// ==========================================================================
// Priest Pets
// ==========================================================================

/* priest pet base
 *
 * defines characteristics common to ALL priest pets
 */
struct priest_pet_t : public pet_t
{
  priest_pet_t( sim_t* sim, priest_t& owner, const std::string& pet_name, pet_e pt, bool guardian = false ) :
    pet_t( sim, &owner, pet_name, pt, guardian )
  {
    base.position = POSITION_BACK;
    base.distance = 3;
  }

  struct _stat_list_t
  {
    int level;
    std::array<double, ATTRIBUTE_MAX> stats;
  };


  virtual void init_base_stats() override
  {
    pet_t::init_base_stats();

    owner_coeff.ap_from_sp = 1.0;
    owner_coeff.sp_from_sp = 1.0;

    // Base Stats, same for all pets. Depend on level
    static const _stat_list_t pet_base_stats[] =
    {
      //   none, str, agi, sta, int, spi
      { 85, { { 0, 453, 883, 353, 159, 225 } } },
    };

    // Loop from end to beginning to get the data for the highest available level equal or lower than the player level
    int i = as<int>( sizeof_array( pet_base_stats ) );
    while ( --i > 0 )
    {
      if ( pet_base_stats[ i ].level <= level )
        break;
    }
    if ( i >= 0 )
      base.stats.attribute = pet_base_stats[ i ].stats;
  }

  virtual void schedule_ready( timespan_t delta_time, bool waiting ) override
  {
    if ( main_hand_attack && ! main_hand_attack -> execute_event )
    {
      main_hand_attack -> schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  virtual double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Orc racial
    m *= 1.0 + o().racials.command -> effectN( 1 ).percent();

    return m;
  }

  virtual resource_e primary_resource() const override
  { return RESOURCE_ENERGY; }

  priest_t& o() const
  { return static_cast<priest_t&>( *owner ); }
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

  base_fiend_pet_t( sim_t* sim, priest_t& owner, pet_e pt, const std::string& name ) :
    priest_pet_t( sim, owner, name, pt ),
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

  virtual void init_action_list() override;

  virtual void create_buffs() override
  {
    priest_pet_t::create_buffs();

    buffs.shadowcrawl = buff_creator_t( this, "shadowcrawl", find_pet_spell( "Shadowcrawl" ) );
  }

  virtual void init_gains() override
  {
    priest_pet_t::init_gains();

    switch ( pet_type )
    {
    case PET_SHADOWFIEND:
      gains.fiend = o().gains.shadowfiend;
      break;
    case PET_MINDBENDER:
      gains.fiend = o().gains.mindbender;
      break;
    default:
      gains.fiend = get_gain( "basefiend" );
      break;
    }
  }

  virtual void init_resources( bool force ) override
  {
    priest_pet_t::init_resources( force );

    resources.initial[ RESOURCE_MANA ] = owner -> resources.max[ RESOURCE_MANA ];
    resources.current = resources.max = resources.initial;
  }

  virtual void summon( timespan_t duration ) override
  {
    dismiss();

    duration += timespan_t::from_seconds( 0.01 );

    priest_pet_t::summon( duration );

    if ( shadowcrawl_action )
    {
      /* Ensure that it gets used after the first melee strike.
       * In the combat logs that happen at the same time, but the melee comes first.
       */
      shadowcrawl_action -> cooldown -> ready = sim -> current_time + timespan_t::from_seconds( 0.001 );
    }
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override;
};

// ==========================================================================
// Pet Shadowfiend
// ==========================================================================

struct shadowfiend_pet_t final : public base_fiend_pet_t
{
  const spell_data_t* mana_leech;

  shadowfiend_pet_t( sim_t* sim, priest_t& owner, const std::string& name = "shadowfiend" ) :
    base_fiend_pet_t( sim, owner, PET_SHADOWFIEND, name ),
    mana_leech( find_spell( 34650, "mana_leech" ) )
  {
    direct_power_mod = 1.0;

    main_hand_weapon.min_dmg = owner.dbc.spell_scaling( owner.type, owner.level ) * 2;
    main_hand_weapon.max_dmg = owner.dbc.spell_scaling( owner.type, owner.level ) * 2;

    main_hand_weapon.damage  = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  }

  virtual double mana_return_percent() const override
  { return mana_leech -> effectN( 1 ).percent(); }
};

// ==========================================================================
// Pet Mindbender
// ==========================================================================

struct mindbender_pet_t final : public base_fiend_pet_t
{
  const spell_data_t* mindbender_spell;

  mindbender_pet_t( sim_t* sim, priest_t& owner, const std::string& name = "mindbender" ) :
    base_fiend_pet_t( sim, owner, PET_MINDBENDER, name ),
    mindbender_spell( owner.find_talent_spell( "Mindbender" ) )
  {
    direct_power_mod = 0.88;

    main_hand_weapon.min_dmg = owner.dbc.spell_scaling( owner.type, owner.level ) * 1.76;
    main_hand_weapon.max_dmg = owner.dbc.spell_scaling( owner.type, owner.level ) * 1.76;
    main_hand_weapon.damage  = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  }

  virtual double mana_return_percent() const override
  {
    double m  = mindbender_spell -> effectN( 2 ).base_value();
           m /= mindbender_spell -> effectN( 3 ).base_value();
    return m / 100;
  }
};

// ==========================================================================
// Pet Lightwell
// ==========================================================================

struct lightwell_pet_t final : public priest_pet_t
{
public:
  int charges;

  lightwell_pet_t( sim_t* sim, priest_t& p ) :
    priest_pet_t( sim, p, "lightwell", PET_NONE, true ),
    charges( 0 )
  {
    role = ROLE_HEAL;

    action_list_str  = "/snapshot_stats";
    action_list_str += "/lightwell_renew";
    action_list_str += "/wait,sec=cooldown.lightwell_renew.remains";

    owner_coeff.sp_from_sp = 1.0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override;

  virtual void summon( timespan_t duration ) override
  {
    charges = 10 + o().glyphs.lightwell -> effectN( 1 ).base_value();

    priest_pet_t::summon( duration );
  }
};

namespace actions { // namespace for pet actions

// ==========================================================================
// Priest Pet Melee
// ==========================================================================

struct priest_pet_melee_t : public melee_attack_t
{
  bool first_swing;

  priest_pet_melee_t( priest_pet_t& p, const char* name ) :
    melee_attack_t( name, &p, spell_data_t::nil() ),
    first_swing( true )
  {
    school = SCHOOL_SHADOW;
    weapon = &( p.main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit    = true;
    background  = true;
    repeating   = true;
  }

  virtual void reset() override
  {
    melee_attack_t::reset();
    first_swing = true;
  }

  virtual timespan_t execute_time() const override
  {
    // First swing comes instantly after summoning the pet
    if ( first_swing )
      return timespan_t::zero();

    return melee_attack_t::execute_time();
  }

  virtual void schedule_execute( action_state_t* state = 0 ) override
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
  priest_pet_spell_t( priest_pet_t& p, const std::string& n ) :
    spell_t( n, &p, p.find_pet_spell( n ) )
  {
    may_crit = true;
  }

  priest_pet_spell_t( const std::string& token, priest_pet_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    spell_t( token, p, s )
  {
    may_crit = true;
  }

  priest_pet_t& p()
  { return static_cast<priest_pet_t&>( *player ); }
  const priest_pet_t& p() const
  { return static_cast<priest_pet_t&>( *player ); }
};

struct shadowcrawl_t final : public priest_pet_spell_t
{
  shadowcrawl_t( base_fiend_pet_t& p ) :
    priest_pet_spell_t( p, "Shadowcrawl" )
  {
    may_miss  = false;
    harmful   = false;
  }

  base_fiend_pet_t& p()
  { return static_cast<base_fiend_pet_t&>( *player ); }
  const base_fiend_pet_t& p() const
  { return static_cast<base_fiend_pet_t&>( *player ); }

  virtual void execute() override
  {
    priest_pet_spell_t::execute();

    p().buffs.shadowcrawl -> trigger();
  }
};

struct fiend_melee_t final : public priest_pet_melee_t
{
  fiend_melee_t( base_fiend_pet_t& p ) :
    priest_pet_melee_t( p, "melee" )
  {
    weapon = &( p.main_hand_weapon );
    weapon_multiplier = 0.0;
    base_dd_min       = weapon -> min_dmg;
    base_dd_max       = weapon -> max_dmg;
    attack_power_mod.direct  = p.direct_power_mod;
  }

  base_fiend_pet_t& p()
  { return static_cast<base_fiend_pet_t&>( *player ); }
  const base_fiend_pet_t& p() const
  { return static_cast<base_fiend_pet_t&>( *player ); }

  virtual double action_multiplier() const override
  {
    double am = priest_pet_melee_t::action_multiplier();

    am *= 1.0 + p().buffs.shadowcrawl -> up() * p().buffs.shadowcrawl -> data().effectN( 2 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_pet_melee_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p().o().resource_gain( RESOURCE_MANA,
                             p().o().resources.max[ RESOURCE_MANA ] * p().mana_return_percent(),
                             p().gains.fiend );
    }
  }
};

struct lightwell_renew_t final : public heal_t
{
  lightwell_renew_t( lightwell_pet_t& p ) :
    heal_t( "lightwell_renew", &p, p.find_spell( 7001 ) )
  {
    may_crit = false;
    tick_may_crit = true;

    spell_power_mod.direct = 0.308;
  }

  lightwell_pet_t& p()
  { return static_cast<lightwell_pet_t&>( *player ); }
  const lightwell_pet_t& p() const
  { return static_cast<lightwell_pet_t&>( *player ); }

  virtual void execute() override
  {
    p().charges--;

    target = find_lowest_player();

    heal_t::execute();
  }

  virtual void last_tick( dot_t* d ) override
  {
    heal_t::last_tick( d );

    if ( p().charges <= 0 )
      p().dismiss();
  }

  virtual bool ready() override
  {
    if ( p().charges <= 0 )
      return false;

    return heal_t::ready();
  }
};

} // end namespace actions ( for pets )

// ==========================================================================
// Pet Shadowfiend/Mindbender Base
// ==========================================================================

void base_fiend_pet_t::init_action_list()
{
  main_hand_attack = new actions::fiend_melee_t( *this );

  if ( action_list_str.empty() )
  {
    action_list_str += "/snapshot_stats";
    action_list_str += "/shadowcrawl";
    action_list_str += "/wait_for_shadowcrawl";
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

  if ( name == "wait_for_shadowcrawl" ) return new wait_for_cooldown_t( this, "shadowcrawl" );

  return priest_pet_t::create_action( name, options_str );
}

action_t* lightwell_pet_t::create_action( const std::string& name,
                                          const std::string& options_str )
{
  if ( name == "lightwell_renew" ) return new actions::lightwell_renew_t( *this );

  return priest_pet_t::create_action( name, options_str );
}

} // END pets NAMESPACE

namespace actions {

/* This is a template for common code between priest_spell_t, priest_heal_t and priest_absorb_t.
 * The template is instantiated with either spell_t, heal_t or absorb_t as the 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived class,
 * don't skip it and call spell_t/heal_t or absorb_t directly.
 */
template <typename Base>
struct priest_action_t : public Base
{
public:
  priest_action_t( const std::string& n, priest_t& p,
                   const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, &p, s ),
    sform( p.buffs.shadowform ),
    min_interval( p.get_cooldown( "min_interval_" + ab::name_str ) ),
    priest( p )
  {
    ab::may_crit          = true;
    ab::tick_may_crit     = true;

    ab::dot_behavior      = DOT_REFRESH;
    ab::weapon_multiplier = 0.0;


    can_cancel_shadowform = p.options.autoUnshift;
    castable_in_shadowform = true;
  }

  bool check_shadowform() const
  {
    return ( castable_in_shadowform || can_cancel_shadowform || ( sform -> current_stack == 0 ) );
  }

  void cancel_shadowform()
  {
    if ( ! castable_in_shadowform )
    {
      // FIX-ME: Needs to drop haste aura too.
      sform  -> expire();
    }
  }

  double shadow_orbs_to_consume() const
  {
    return std::min( 3.0, priest.resources.current[ RESOURCE_SHADOW_ORB ] );
  }

  priest_td_t& get_td( player_t* t ) const
  { return *( priest.get_target_data( t ) ); }

  virtual void schedule_execute( action_state_t* state = 0 ) override
  {
    cancel_shadowform();

    ab::schedule_execute( state );
  }

  virtual bool ready() override
  {
    if ( ! ab::ready() )
      return false;

    if ( ! check_shadowform() )
      return false;

    return ( min_interval -> remains() <= timespan_t::zero() );
  }

  virtual double cost() const override
  {
    double c = ab::cost();

    if ( priest.buffs.power_infusion -> check() )
    {
      c *= 1.0 + priest.buffs.power_infusion -> data().effectN( 2 ).percent();
      c  = std::floor( c );
    }

    return c;
  }

  virtual void consume_resource() override
  {
    ab::consume_resource();

    if ( ab::base_execute_time > timespan_t::zero() && ! this -> channeled )
      priest.buffs.borrowed_time -> expire();
  }

  virtual void parse_options( option_t*          options,
                      const std::string& options_str ) override
  {
    const option_t base_options[] =
    {
      opt_timespan( "min_interval", ( min_interval -> duration ) ),
      opt_null()
    };

    std::vector<option_t> merged_options;
    ab::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }

  virtual void update_ready( timespan_t cd_duration ) override
  {
    ab::update_ready( cd_duration );

    if ( min_interval -> duration > timespan_t::zero() && ! this -> dual )
    {
      min_interval -> start( timespan_t::min(), timespan_t::zero() );

      if ( ab::sim -> debug )
        ab::sim -> out_debug.printf( "%s starts min_interval for %s (%s). Will be ready at %.4f",
                               priest.name(), this -> name(), min_interval -> name(), min_interval -> ready.total_seconds() );
    }
  }
private:
  typedef Base ab; // typedef for the templated action type, eg. spell_t, attack_t, heal_t
  buff_t* sform;
  cooldown_t* min_interval; // Minimal interval / Forced cooldown of the action. Specifiable through option
protected:
  bool castable_in_shadowform;
  bool can_cancel_shadowform;

  /* keep reference to the priest. We are sure this will always resolve
   * to the same player as the action_t::player; pointer, and is always valid
   * because it owns the action
   */
  priest_t& priest;

  typedef priest_action_t base_t; // typedef for priest_action_t<action_base_t>
};

// ==========================================================================
// Priest Absorb
// ==========================================================================

struct priest_absorb_t : public priest_action_t<absorb_t>
{
public:
  priest_absorb_t( const std::string& n, priest_t& player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
    may_crit          = false;
    tick_may_crit     = false;
    may_miss          = false;
  }

  virtual double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( priest.mastery_spells.shield_discipline -> ok() )
      am += 1.0 + priest.cache.mastery_value();

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
    divine_aegis_t( const std::string& n, priest_t& p ) :
      priest_absorb_t( n + "_divine_aegis", p, p.find_spell( 47753 ) )
    {
      check_spell( p.specs.divine_aegis );
      proc             = true;
      background       = true;
      spell_power_mod.direct = 0.0;
    }

    virtual void impact( action_state_t* s ) override
    {
      absorb_buff_t& buff = *get_td( s -> target ).buffs.divine_aegis;
      // Divine Aegis caps absorbs at 40% of target's health
      double old_amount = buff.value();
      double new_amount = clamp( s -> result_amount, 0.0, s -> target -> resources.current[ RESOURCE_HEALTH ] * 0.4 - old_amount );
      buff.trigger( 1, old_amount + new_amount );
      stats -> add_result( 0.0, new_amount, ABSORB, s -> result, s -> block_result, s -> target );
      buff.absorb_source -> add_result( 0.0, new_amount, ABSORB, s -> result, s -> block_result, s -> target );
    }

    void trigger( const action_state_t* s )
    {
      base_dd_min = base_dd_max = s -> result_amount * priest.specs.divine_aegis -> effectN( 1 ).percent();
      target = s -> target;
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

    spirit_shell_absorb_t( priest_heal_t& trigger ) :
      priest_absorb_t( trigger.name_str + "_shell", trigger.priest, trigger.priest.specs.spirit_shell ),
      trigger_crit_multiplier( 0.0 )
    {
      background = true;
      proc = true;
      may_crit = false;
      snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;
    }

    virtual double action_multiplier() const override // override
    {
      double am;

      am = absorb_t::action_multiplier();

      return am *                                        // ( am ) *
             ( 1 + trigger_crit_multiplier ) *           // ( 1 + crit ) *
             ( 1 + trigger_crit_multiplier * 0.3 );      // ( 1 + crit * 30% "DA factor" )
    }

    virtual void impact( action_state_t* s ) override
    {
      // Spirit Shell caps absorbs at 60% of target's health
      buff_t& spirit_shell = *get_td( s -> target ).buffs.spirit_shell;
      double old_amount = spirit_shell.value();
      double new_amount = clamp( s -> result_amount, 0.0, s -> target -> resources.current[ RESOURCE_HEALTH ] * 0.6 - old_amount );

      spirit_shell.trigger( 1, old_amount + new_amount );
      stats -> add_result( 0.0, new_amount, ABSORB, s -> result, s -> block_result, s -> target );
    }

    void trigger( action_state_t* s )
    {
      assert( s -> result != RESULT_CRIT );
      base_dd_min = base_dd_max = s -> result_amount;
      target = s -> target;
      trigger_crit_multiplier = s -> composite_crit();
      execute();
    }
  };

  divine_aegis_t* da;
  spirit_shell_absorb_t* ss;
  unsigned divine_aegis_trigger_mask;
  bool can_trigger_EoL, can_trigger_spirit_shell;

  virtual void init() override
  {
    base_t::init();

    if ( divine_aegis_trigger_mask && priest.specs.divine_aegis -> ok() )
    {
      da = new divine_aegis_t( name_str, priest );
      // add_child( da );
    }

    if ( can_trigger_spirit_shell )
      ss = new spirit_shell_absorb_t( *this );
  }

  priest_heal_t( const std::string& n, priest_t& player,
                 const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    da( nullptr ), ss( nullptr ),
    divine_aegis_trigger_mask( RESULT_CRIT_MASK ),
    can_trigger_EoL( true ), can_trigger_spirit_shell( false )
  {}

  virtual double composite_crit() const override
  {
    double cc = base_t::composite_crit();

    if ( priest.buffs.chakra_serenity -> up() )
      cc += priest.buffs.chakra_serenity -> data().effectN( 1 ).percent();

    return cc;
  }

  virtual double composite_target_crit( player_t* t ) const override
  {
    double ctc = base_t::composite_target_crit( t );

    if ( get_td( t ).buffs.holy_word_serenity -> check() )
      ctc += get_td( t ).buffs.holy_word_serenity -> data().effectN( 2 ).percent();

    return ctc;
  }

  virtual double action_multiplier() const override
  {
    return base_t::action_multiplier() * ( 1.0 + priest.buffs.archangel -> value() );
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( priest.specs.grace -> ok() )
      ctm *= 1.0 + t -> buffs.grace -> check() * t -> buffs.grace -> value();

    return ctm;
  }

  virtual void execute() override
  {
    if ( can_trigger_spirit_shell )
      may_crit = priest.buffs.spirit_shell -> check() == 0;

    base_t::execute();

    may_crit = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    if ( ss && priest.buffs.spirit_shell -> up() )
    {
      ss -> trigger( s );
    }
    else
    {
      double save_health_percentage = s -> target -> health_percentage();

      base_t::impact( s );

      if ( s -> result_amount > 0 )
      {
        trigger_divine_aegis( s );
        trigger_echo_of_light( this, s );

        if ( priest.buffs.chakra_serenity -> up() && get_td( target ).dots.renew -> ticking )
        {
          get_td( target ).dots.renew -> refresh_duration();
        }

        if ( priest.talents.twist_of_fate -> ok() && ( save_health_percentage < priest.talents.twist_of_fate -> effectN( 1 ).base_value() ) )
        {
          priest.buffs.twist_of_fate -> trigger();
        }
      }
    }
  }

  virtual void tick( dot_t* d ) override /* override */
  {
    base_t::tick( d );
    trigger_divine_aegis( d -> state );
  }

  void trigger_divine_aegis( action_state_t* s )
  {
    if ( da && ( ( 1 << s -> result ) & divine_aegis_trigger_mask ) != 0 )
      da -> trigger( s );
  }

  // Priest Echo of Light, Ignite-Mechanic specialization
  void trigger_echo_of_light( priest_heal_t* h, action_state_t* s )
  {
    priest_t& p = h -> priest;
    if ( ! p.mastery_spells.echo_of_light -> ok() || ! can_trigger_EoL )
      return;

    ignite::trigger_pct_based(
      p.active_spells.echo_of_light, // ignite spell
      s -> target, // target
      s -> result_amount * p.cache.mastery_value() ); // ignite damage
  }

  void trigger_grace( player_t* t )
  {
    if ( priest.specs.grace -> ok() )
      t -> buffs.grace -> trigger( 1, priest.specs.grace -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  }

  void trigger_serendipity()
  { priest.buffs.serendipity -> trigger(); }

  void trigger_strength_of_soul( player_t* t )
  {
    if ( priest.specs.strength_of_soul -> ok() && t -> buffs.weakened_soul -> up() )
      t -> buffs.weakened_soul -> extend_duration( player, timespan_t::from_seconds( -1 * priest.specs.strength_of_soul -> effectN( 1 ).base_value() ) );
  }

  void trigger_surge_of_light()
  {
    int stack = priest.buffs.surge_of_light -> current_stack;
    if ( priest.buffs.surge_of_light -> trigger() )
    {
      if ( priest.buffs.surge_of_light -> current_stack == stack )
        priest.procs.surge_of_light_overflow -> occur();
      priest.procs.surge_of_light -> occur();
    }
  }

  void consume_inner_focus()
  {
    if ( priest.buffs.inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      priest.buffs.inner_focus -> expire();
      priest.cooldowns.inner_focus -> start( priest.buffs.inner_focus -> data().cooldown() );
    }
  }

  void consume_serendipity()
  {
    priest.buffs.serendipity -> up();
    priest.buffs.serendipity -> expire();
  }

  void consume_surge_of_light()
  {
    priest.buffs.surge_of_light -> up();
    priest.buffs.surge_of_light -> expire();
  }
};

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public priest_action_t<spell_t>
{
  // Atonement heal =========================================================
  struct atonement_heal_t final : public priest_heal_t
  {
    atonement_heal_t( const std::string& n, priest_t& p ) :
      priest_heal_t( n, p, p.specs.atonement )
    {
      background     = true;
      round_base_dmg = false;
      hasted_ticks   = false;

      // We set the base crit chance to 100%, so that simply setting
      // may_crit = true (in trigger()) will force a crit. When the trigger
      // spell crits, the atonement crits as well and procs Divine Aegis.
      base_crit = 1.0;

      snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;

      if ( ! p.options.atonement_target_str.empty() )
        target = sim -> find_player( p.options.atonement_target_str );
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

      base_dd_min = base_dd_max = std::min( cap, damage * data().effectN( 1 ).percent() );

      direct_tick = dual = ( dmg_type == DMG_OVER_TIME );
      may_crit = ( result == RESULT_CRIT );

      execute();
    }

    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = priest_heal_t::composite_target_multiplier( target );
      if ( target == player )
        m *= 0.5; // Hardcoded in the tooltip
      return m;
    }

    virtual double total_crit_bonus() const override
    { return 0; }

    virtual void execute() override
    {
      player_t* saved_target = target;
      if ( ! target )
        target = find_lowest_player();

      priest_heal_t::execute();

      target = saved_target;
    }

    virtual void tick( dot_t* d ) override
    {
      player_t* saved_target = target;
      if ( ! target )
        target = find_lowest_player();

      priest_heal_t::tick( d );

      target = saved_target;
    }
  };

  atonement_heal_t* atonement;
  bool can_trigger_atonement;

  priest_spell_t( const std::string& n, priest_t& player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s ),
    atonement( nullptr ), can_trigger_atonement( false )
  {
    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;

    can_trigger_atonement = false;
  }

  virtual void init() override
  {
    base_t::init();

    if ( can_trigger_atonement && priest.specs.atonement -> ok() )
      atonement = new atonement_heal_t( "atonement_" + name_str, priest );
  }

  virtual void impact( action_state_t* s ) override
  {
    double save_health_percentage = s -> target -> health_percentage();

    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( priest.talents.twist_of_fate -> ok() && ( save_health_percentage < priest.talents.twist_of_fate -> effectN( 1 ).base_value() ) )
      {
        priest.buffs.twist_of_fate -> trigger();
      }
    }
  }

  virtual void assess_damage( dmg_e type,
                              action_state_t* s ) override
  {
    base_t::assess_damage( type, s );

    if ( aoe == 0 && result_is_hit( s -> result ) && priest.buffs.vampiric_embrace -> up() )
      trigger_vampiric_embrace( s );

    if ( atonement )
      trigger_atonment( type, s );
  }

  /* Based on previous implementation ( pets don't count but get full heal )
   * and http://www.wowhead.com/spell=15286#comments:id=1796701
   * Last checked 2013/05/25
   */
  void trigger_vampiric_embrace( action_state_t* s )
  {
    double amount = s -> result_amount;
    amount *= ( priest.buffs.vampiric_embrace -> data().effectN( 1 ).percent() + priest.glyphs.vampiric_embrace -> effectN( 2 ).percent() ) ;

    // Get all non-pet, non-sleeping players
    std::vector<player_t*> ally_list;;
    range::remove_copy_if( sim -> player_no_pet_list.data(), back_inserter( ally_list ), player_t::_is_sleeping );

    // Split amongst number of people in raid.
    // Pet's get a full share without counting against the number in the raid.
    // Amount is split up evenly, no smart heal mechanic involved
    amount /= 1.0 + ally_list.size();

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

  virtual void trigger_atonment( dmg_e type, action_state_t* s )
  {
    atonement -> trigger( s -> result_amount, direct_tick ? DMG_OVER_TIME : type, s -> result );
  }

  void generate_shadow_orb( unsigned num_orbs, gain_t* g = nullptr )
  {
    if ( priest.specs.shadow_orbs -> ok() )
      priest.resource_gain( RESOURCE_SHADOW_ORB, num_orbs, g, this );
  }
};

namespace spells {

void cancel_dot( dot_t& dot )
{
  if ( dot.ticking )
  {
    dot.cancel();
    dot.reset();
  }
}

// ==========================================================================
// Priest Abilities
// ==========================================================================

// ==========================================================================
// Priest Non-Harmful Spells
// ==========================================================================

// Archangel Spell ==========================================================

struct archangel_t final : public priest_spell_t
{
  archangel_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "archangel", p, p.specs.archangel )
  {
    parse_options( nullptr, options_str );
    harmful = may_hit = may_crit = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();
    priest.buffs.archangel -> trigger();
  }

  virtual bool ready() override
  {
    if ( ! priest.buffs.holy_evangelism -> check() )
      return false;
    else
      return priest_spell_t::ready();
  }
};

// Chakra_Pre Spell =========================================================

struct chakra_base_t : public priest_spell_t
{
  chakra_base_t( priest_t& p, const spell_data_t* s, const std::string& options_str ) :
    priest_spell_t( "chakra", p, s )
  {
    parse_options( nullptr, options_str );

    harmful = false;

    p.cooldowns.chakra -> duration = cooldown -> duration;
    cooldown = p.cooldowns.chakra;
  }

  void switch_to( buff_t* b )
  {
    if ( priest.buffs.chakra_sanctuary != b )
      priest.buffs.chakra_sanctuary -> expire();
    if ( priest.buffs.chakra_serenity != b )
      priest.buffs.chakra_serenity -> expire();
    if ( priest.buffs.chakra_chastise != b )
      priest.buffs.chakra_chastise -> expire();

    b -> trigger();
  }
};

struct chakra_chastise_t final : public chakra_base_t
{
  chakra_chastise_t( priest_t& p, const std::string& options_str ) :
    chakra_base_t( p, p.find_class_spell( "Chakra: Chastise" ), options_str )
  { }

  virtual void execute() override
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_chastise );
  }
};

struct chakra_sanctuary_t final : public chakra_base_t
{
  chakra_sanctuary_t( priest_t& p, const std::string& options_str ) :
    chakra_base_t( p, p.find_class_spell( "Chakra: Sanctuary" ), options_str )
  { }

  virtual void execute() override
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_sanctuary );
  }
};

struct chakra_serenity_t final : public chakra_base_t
{
  chakra_serenity_t( priest_t& p, const std::string& options_str ) :
    chakra_base_t( p, p.find_class_spell( "Chakra: Serenity" ), options_str )
  { }

  virtual void execute() override
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_serenity );
  }
};

// Dispersion Spell =========================================================

struct dispersion_t final : public priest_spell_t
{
  double regen_multiplier;

  dispersion_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "dispersion", player, player.find_class_spell( "Dispersion" ) ),
    regen_multiplier( priest.find_spell( 49766 ) -> effectN( 1 ).percent() )
  {
    parse_options( nullptr, options_str );

    base_tick_time    = timespan_t::from_seconds( 1.0 );
    num_ticks         = 6;

    channeled         = true;
    harmful           = false;
    tick_may_crit     = false;

    cooldown -> duration = data().cooldown() + priest.glyphs.dispersion -> effectN( 1 ).time_value();
  }

  virtual void tick( dot_t* d ) override
  {
    priest.resource_gain( RESOURCE_MANA,
                          regen_multiplier * priest.resources.max[ RESOURCE_MANA ],
                          priest.gains.dispersion );

    priest_spell_t::tick( d );
  }
};

// Fortitude Spell ==========================================================

struct fortitude_t final : public priest_spell_t
{
  fortitude_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "fortitude", player, player.find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;

    background = ( sim -> overrides.stamina != 0 );
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger();
  }
};

// Inner Focus Spell ========================================================

struct inner_focus_t final : public priest_spell_t
{
  inner_focus_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "inner_focus", p, p.find_class_spell( "Inner Focus" ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    cooldown -> duration = sim -> max_time;

    priest_spell_t::execute();

    priest.buffs.inner_focus -> trigger();
  }
};

// Pain Supression ==========================================================

struct pain_suppression_t final : public priest_spell_t
{
  pain_suppression_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "pain_suppression", p, p.find_class_spell( "Pain Suppression" ) )
  {
    parse_options( nullptr, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = &p;
    }

    harmful = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    target -> buffs.pain_supression -> trigger();
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t final : public priest_spell_t
{
  power_infusion_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "power_infusion", p, p.talents.power_infusion )
  {
    parse_options( nullptr, options_str );
    harmful = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();
    priest.buffs.power_infusion -> trigger();
  }
};

// Shadow Form Spell ========================================================

struct shadowform_t final : public priest_spell_t
{
  shadowform_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "shadowform", p, p.find_class_spell( "Shadowform" ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.shadowform -> trigger();
  }

  virtual bool ready() override
  {
    if (  priest.buffs.shadowform -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Spirit Shell Spell =======================================================

struct spirit_shell_t final : public priest_spell_t
{
  spirit_shell_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "spirit_shell", p, p.specs.spirit_shell )
  {
    parse_options( nullptr, options_str );
    harmful = may_hit = may_crit = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();
    priest.buffs.spirit_shell -> trigger();
  }
};

// Per Summon Base Class

struct summon_pet_t : public priest_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, priest_t& p, const spell_data_t* sd = spell_data_t::nil() ) :
    priest_spell_t( n, p, sd ),
    summoning_duration ( timespan_t::zero() ),
    pet( p.find_pet( n ) )
  {
    harmful = false;

    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), n.c_str() );
      background = true;
    }
  }

  virtual void execute() override
  {
    pet -> summon( summoning_duration );

    priest_spell_t::execute();
  }
};


struct summon_shadowfiend_t final : public summon_pet_t
{
  summon_shadowfiend_t( priest_t& p, const std::string& options_str ) :
    summon_pet_t( "shadowfiend", p, p.find_class_spell( "Shadowfiend" ) )
  {
    parse_options( nullptr, options_str );
    harmful    = false;
    summoning_duration = data().duration();
    cooldown = p.cooldowns.shadowfiend;
    cooldown -> duration = data().cooldown();
  }
};


struct summon_mindbender_t final : public summon_pet_t
{
  summon_mindbender_t( priest_t& p, const std::string& options_str ) :
    summon_pet_t( "mindbender", p, p.find_talent_spell( "Mindbender" ) )
  {
    parse_options( nullptr, options_str );
    harmful    = false;
    summoning_duration = data().duration();
    cooldown = p.cooldowns.mindbender;
    cooldown -> duration = data().cooldown();
  }
};

// ==========================================================================
// Priest Damage Spells
// ==========================================================================

// Shadowy Apparition Spell =================================================

struct shadowy_apparition_spell_t final : public priest_spell_t
{
  shadowy_apparition_spell_t( priest_t& p ) :
    priest_spell_t( "shadowy_apparitions",
                    p,
                    p.find_spell( 78203 ) )
  {
    background        = true;
    proc              = false;
    callbacks         = true;
    may_miss          = false;

    trigger_gcd       = timespan_t::zero();
    travel_speed      = 6.0;
    if ( ! priest.talents.auspicious_spirits -> ok() )
    {
      spell_power_mod.direct  = 0.375;
      base_dd_min = base_dd_max = 393;
    }
    school            = SCHOOL_SHADOW;
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( priest.talents.auspicious_spirits -> ok() )
    {
      generate_shadow_orb( 1, priest.gains.auspicious_spirits );
    }

    if ( rng().roll( priest.sets.set( SET_T15_2PC_CASTER ) -> effectN( 1 ).percent() ) )
    {
      priest_td_t& td = get_td( s -> target);
      priest.procs.t15_2pc_caster -> occur();

      if ( td.dots.shadow_word_pain -> ticking )
      {
        td.dots.shadow_word_pain -> extend_duration( 1 );
        priest.procs.t15_2pc_caster_shadow_word_pain -> occur();
      }

      if ( td.dots.vampiric_touch -> ticking )
      {
        td.dots.vampiric_touch -> extend_duration( 1 );
        priest.procs.t15_2pc_caster_vampiric_touch -> occur();
      }

    }
  }

  /* Trigger a shadowy apparition
   */
  void trigger()
  {
    if ( priest.sim -> debug )
      priest.sim -> out_debug << priest.name() << " triggered shadowy apparition.";

    priest.procs.shadowy_apparition -> occur();
    schedule_execute();
  }
};

// Mind Blast Spell =========================================================

struct mind_blast_t final : public priest_spell_t
{
  bool casted_with_divine_insight;

  mind_blast_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "mind_blast", player, player.find_class_spell( "Mind Blast" ) ),
    casted_with_divine_insight( false )
  {
    parse_options( nullptr, options_str );

  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    casted_with_divine_insight = false;
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      generate_shadow_orb( 1, priest.gains.shadow_orb_mb );

      priest.buffs.glyph_mind_spike -> expire();
    }
  }

  void consume_resource() override
  {
    if ( casted_with_divine_insight )
      resource_consumed = 0.0;
    else
      resource_consumed = cost();

    player -> resource_loss( current_resource(), resource_consumed, nullptr, this );

    if ( sim -> log )
      sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double cost() const override
  {
    if ( priest.buffs.divine_insight_shadow -> check() )
      return 0.0;

    return priest_spell_t::cost();
  }

  virtual void schedule_execute( action_state_t* state = 0 ) override
  {
    if ( priest.buffs.divine_insight_shadow -> up() )
    {
      casted_with_divine_insight = true;
    }

    priest_spell_t::schedule_execute( state );

    priest.buffs.divine_insight_shadow -> expire();
  }

  virtual void update_ready( timespan_t cd_duration ) override
  {
    if ( cd_duration < timespan_t::zero() )
      cd_duration = cooldown -> duration;

    if ( priest.specs.mind_surge -> ok() && ! priest.buffs.divine_insight_shadow -> check() )
    {
      cd_duration = cooldown -> duration * composite_haste();
    }


    cd_duration -= timespan_t::from_seconds( priest.talents.clarity_of_power -> effectN( 2 ).base_value() );

    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows -> up(); // benefit tracking
    priest.buffs.empowered_shadows -> expire();
  }

  virtual timespan_t execute_time() const override
  {
    if ( priest.buffs.divine_insight_shadow -> check() )
    {
      return timespan_t::zero();
    }

    timespan_t et = priest_spell_t::execute_time();
    et *= 1 + priest.buffs.glyph_mind_spike -> stack() * priest.glyphs.mind_spike -> effectN( 1 ).percent();
    return et;
  }

  virtual void reset() override
  {
    priest_spell_t::reset();

    casted_with_divine_insight = false;
  }

  virtual double composite_da_multiplier() const override
  {
    double d = priest_spell_t::composite_da_multiplier();

    if ( priest.buffs.empowered_shadows -> check() )
      d *= 1.0 + priest.buffs.empowered_shadows->current_value *  priest.buffs.empowered_shadows -> check();

    if ( priest.mastery_spells.mental_anguish -> ok() )
    {
      d *= 1.0 + priest.cache.mastery_value();
    }

    return d;
  }
};

// Mind Spike Spell =========================================================

struct mind_spike_t final : public priest_spell_t
{
  struct mind_spike_state_t : public action_state_t
  {
    bool surge_of_darkness;

    mind_spike_state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      surge_of_darkness ( false )
    { }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    { action_state_t::debug_str( s ) << " surge_of_darkness=" << surge_of_darkness; return s; }

    void copy_state( const action_state_t* o ) override
    {
      action_state_t::copy_state( o );
      const mind_spike_state_t* dps_t = static_cast<const mind_spike_state_t*>( o );
      surge_of_darkness = dps_t -> surge_of_darkness;
    }

    void initialize() override
    { action_state_t::initialize(); surge_of_darkness = false; }
  };

  bool casted_with_surge_of_darkness;

  mind_spike_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "mind_spike", player, player.find_class_spell( "Mind Spike" ) ),
    casted_with_surge_of_darkness( false )
  {
    parse_options( nullptr, options_str );
  }

  virtual action_state_t* new_state() override
  {
    return new mind_spike_state_t( this, target );
  }

  virtual action_state_t* get_state( const action_state_t* s ) override
  {
    action_state_t* s_ = priest_spell_t::get_state( s );

    if ( !s )
    {
      mind_spike_state_t* ds_ = static_cast<mind_spike_state_t*>( s_ );
      ds_ -> surge_of_darkness = false;
    }

    return s_;
  }

  virtual void snapshot_state( action_state_t* state, dmg_e type ) override
  {
    mind_spike_state_t* ms_s = static_cast<mind_spike_state_t*>( state );

    ms_s -> surge_of_darkness = casted_with_surge_of_darkness;

    priest_spell_t::snapshot_state( state, type );
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    casted_with_surge_of_darkness = false;
  }

  virtual void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows -> up(); // benefit tracking
    priest.buffs.empowered_shadows -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      const mind_spike_state_t* ms_s = static_cast<const mind_spike_state_t*>( s );
      if ( ! ms_s -> surge_of_darkness )
      {
        priest_td_t& td = get_td( s -> target );
        cancel_dot( *td.dots.shadow_word_pain );
        cancel_dot( *td.dots.vampiric_touch );
        cancel_dot( *td.dots.devouring_plague_tick );
        priest.procs.mind_spike_dot_removal -> occur();

        priest.buffs.glyph_mind_spike -> trigger();
      }

      if ( priest.talents.clarity_of_power -> ok() )
      {
        priest.resource_gain( RESOURCE_MANA, priest.resources.base[ RESOURCE_MANA ] * priest.talents.clarity_of_power -> effectN( 3 ).percent(), priest.gains.clarity_of_power_mind_spike );
      }
    }
  }

  virtual void schedule_execute( action_state_t* state ) override
  {
    if ( priest.buffs.surge_of_darkness -> up() )
    {
      casted_with_surge_of_darkness = true;
    }

    priest_spell_t::schedule_execute( state );

    priest.buffs.surge_of_darkness -> decrement();
  }

  void consume_resource() override
  {
    if ( casted_with_surge_of_darkness )
      resource_consumed = 0.0;
    else
      resource_consumed = cost();

    player -> resource_loss( current_resource(), resource_consumed, nullptr, this );

    if ( sim -> log )
      sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double composite_da_multiplier() const override
  {
    double d = priest_spell_t::composite_da_multiplier();

    if ( casted_with_surge_of_darkness )
    {
      d *= 1.0 + priest.active_spells.surge_of_darkness -> effectN( 4 ).percent();
    }

    if ( priest.mastery_spells.mental_anguish -> ok() )
    {
      d *= 1.0 + priest.cache.mastery_value();
    }

    if ( priest.buffs.empowered_shadows -> check() )
      d *= 1.0 + priest.buffs.empowered_shadows->current_value *  priest.buffs.empowered_shadows -> check();

    return d;
  }

  virtual double cost() const override
  {
    if ( priest.buffs.surge_of_darkness -> check() )
      return 0.0;

    return priest_spell_t::cost();
  }


  virtual timespan_t execute_time() const override
  {
    if ( priest.buffs.surge_of_darkness -> check() )
    {
      return timespan_t::zero();
    }

    return priest_spell_t::execute_time();
  }

  virtual void reset() override
  {
    priest_spell_t::reset();
    casted_with_surge_of_darkness = false;
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t final : public priest_spell_t
{
  mind_sear_tick_t( priest_t& p ) :
    priest_spell_t( "mind_sear_tick", p, p.find_class_spell( "Mind Sear" ) -> effectN( 1 ).trigger() )
  {
    background  = true;
    dual        = true;
    aoe         = -1;
    callbacks   = false;
    direct_tick = true;

  }
};

struct mind_sear_t final : public priest_spell_t
{
  mind_sear_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "mind_sear", p, p.find_class_spell( "Mind Sear" ) )
  {
    parse_options( nullptr, options_str );

    channeled    = true;
    may_crit     = false;
    hasted_ticks = false;
    dynamic_tick_action = true;

    tick_action = new mind_sear_tick_t( p );
  }

  virtual double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.mental_anguish -> ok() )
    {
      am *= 1.0 + priest.cache.mastery_value();
    }

    return am;
  }
};

// Shadow Word Death Spell ==================================================

struct shadow_word_death_t final : public priest_spell_t
{
  struct shadow_word_death_backlash_t : public priest_spell_t
  {
    double spellpower;
    double multiplier;

    shadow_word_death_backlash_t( priest_t& p ) :
      priest_spell_t( "shadow_word_death_backlash", p, p.find_class_spell( "Shadow Word: Death" ) ),
      spellpower( 0.0 ), multiplier( 1.0 )
    {
      background = true;
      harmful    = false;
      proc       = true;
      may_crit   = false;
      callbacks  = false;

      // Hard-coded values as nothing in DBC
      base_dd_min = base_dd_max = 0.533 * priest.dbc.spell_scaling( data().scaling_class(), priest.level );
      spell_power_mod.direct = 0.599;

      target = &priest;

      ability_lag = sim -> default_aura_delay;
      ability_lag_stddev = sim -> default_aura_delay_stddev;
    }

    virtual void init() override
    {
      priest_spell_t::init();

      stats -> type = STATS_NEUTRAL;
    }

    virtual double composite_spell_power() const override
    { return spellpower; }

    virtual double composite_spell_power_multiplier()
    { return 1.0; }

    virtual double composite_da_multiplier() const override
    {
      double d = multiplier;

      if ( priest.sets.has_set_bonus( SET_T13_2PC_CASTER ) )
        d *= 0.663587;

      return d;
    }
  };

  shadow_word_death_backlash_t* backlash;

  shadow_word_death_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "shadow_word_death", p, p.find_class_spell( "Shadow Word: Death" ) ),
    backlash( new shadow_word_death_backlash_t( p ) )
  {
    parse_options( nullptr, options_str );

    base_multiplier *= 1.0 + p.sets.set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual void execute() override
  {
    bool below_20 = ( target -> health_percentage() < 20.0 );

    priest_spell_t::execute();

    if ( below_20 && priest.specialization() == PRIEST_SHADOW && ! priest.buffs.shadow_word_death_reset_cooldown -> up() )
    {
      cooldown -> reset( false );
      priest.buffs.shadow_word_death_reset_cooldown -> trigger();
    }
  }

  virtual void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );

    priest.buffs.empowered_shadows -> up(); // benefit tracking
    priest.buffs.empowered_shadows -> expire();
  }

  virtual void impact( action_state_t* s ) override
  {
    s -> result_amount = floor( s -> result_amount );

    bool over_20 = ( s -> target -> health_percentage() >= 20.0 );
    if ( backlash && over_20 )
    {
      backlash -> spellpower = s -> composite_spell_power();
      backlash -> multiplier = s -> da_multiplier;
      backlash -> schedule_execute();
    }

    if ( result_is_hit( s -> result ) )
    {
      if ( over_20 )
      {
        s -> result_amount /= 4.0;
      }
      // Assume from the wording of perk 'Enhanced Shadow Word: Death' that it's a 100% chance.
      else if ( ! priest.buffs.shadow_word_death_reset_cooldown -> check() || priest.perks.enhanced_shadow_word_death -> ok() )
      {
        generate_shadow_orb( 1, priest.gains.shadow_orb_swd );
      }
    }

    priest_spell_t::impact( s );
  }

  virtual double composite_da_multiplier() const override
  {
    double d = priest_spell_t::composite_da_multiplier();

    if ( priest.buffs.empowered_shadows -> check() )
        d *= 1.0 + priest.buffs.empowered_shadows -> current_value *  priest.buffs.empowered_shadows -> check();

    return d;
  }

  virtual bool ready() override
  {
    if ( !priest_spell_t::ready() )
      return false;

    if ( !priest.glyphs.shadow_word_death -> ok() && target -> health_percentage() >= 20.0 )
      return false;

    return true;
  }
};

// Devouring Plague State ===================================================

struct shadow_orb_state_t final : public action_state_t
{
  int orbs_used;

  shadow_orb_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ),
    orbs_used ( 0 )
  { }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  { action_state_t::debug_str( s ) << " orbs_used=" << orbs_used; return s; }

  void initialize() override
  { action_state_t::initialize(); orbs_used = 0; }

  void copy_state( const action_state_t* o ) override
  {
    action_state_t::copy_state( o );
    const shadow_orb_state_t* dps_t = static_cast<const shadow_orb_state_t*>( o );
    orbs_used = dps_t -> orbs_used;
  }
};

// Devouring Plague Spell ===================================================

struct devouring_plague_t final : public priest_spell_t
{
  struct devouring_plague_dot_t : public priest_spell_t
  {
    devouring_plague_dot_t( priest_t& p, priest_spell_t* ) :
      priest_spell_t( "devouring_plague_tick", p, p.find_class_spell( "Devouring Plague" ) )
    {
      parse_effect_data( data().effectN( 2 ) );

      spell_power_mod.tick = spell_power_mod.direct;

      base_dd_min = base_dd_max = spell_power_mod.direct = 0.0;

      background = true;
    }

    virtual void reset() override
    { priest_spell_t::reset(); base_ta_adder = 0; }

    virtual action_state_t* new_state() override
    { return new shadow_orb_state_t( this, target ); }

    virtual action_state_t* get_state( const action_state_t* s = nullptr ) override
    {
      action_state_t* s_ = priest_spell_t::get_state( s );

      if ( !s )
      {
        shadow_orb_state_t* ds_ = static_cast<shadow_orb_state_t*>( s_ );
        ds_ -> orbs_used = 0;
      }

      return s_;
    }

    virtual void snapshot_state( action_state_t* state, dmg_e type ) override
    {
      shadow_orb_state_t& dp_state = static_cast<shadow_orb_state_t&>( *state );

      dp_state.orbs_used = as<int>( shadow_orbs_to_consume() );

      priest_spell_t::snapshot_state( state, type );
    }

    virtual double action_ta_multiplier() const override
    {
      double m = priest_spell_t::action_ta_multiplier();

      m *= shadow_orbs_to_consume();

      return m;
    }

    virtual void impact( action_state_t* s ) override
    {
      double saved_impact_dmg = s -> result_amount; // catch previous remaining dp damage
      s -> result_amount = 0;
      priest_spell_t::impact( s );

      if ( saved_impact_dmg > 0.0 )
      {
        dot_t* dot = get_dot();
        base_ta_adder = saved_impact_dmg / dot -> ticks();
        if ( sim -> debug )
          sim -> out_debug.printf( "%s DP still ticking. Added %.2f damage / %.2f per tick to new dot", player -> name(), saved_impact_dmg, base_ta_adder );
      }
    }

    virtual void tick( dot_t* d ) override
    {
      priest_spell_t::tick( d );

      const shadow_orb_state_t& dp_state = static_cast<const shadow_orb_state_t&>( *d -> state );

      double a = data().effectN( 2 ).percent() / 100.0 * dp_state.orbs_used * priest.resources.max[ RESOURCE_HEALTH ];
      priest.resource_gain( RESOURCE_HEALTH, a, priest.gains.devouring_plague_health );
    }
  };

  devouring_plague_dot_t* dot_spell;

  devouring_plague_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "devouring_plague", p, p.find_class_spell( "Devouring Plague" ) ),
    dot_spell( new devouring_plague_dot_t( p, this ) )
  {
    parse_options( nullptr, options_str );

    base_td = num_ticks = 0;
    base_tick_time = timespan_t::zero();

    add_child( dot_spell );
  }

  virtual void consume_resource() override
  {
    resource_consumed = cost();

    if ( execute_state -> result != RESULT_MISS )
    {
      resource_consumed = shadow_orbs_to_consume();
    }

    player -> resource_loss( current_resource(), resource_consumed, nullptr, this );

    if ( sim -> log )
      sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );

    // Grants 20% per stack, 60% in total. Overriding DBC data as it is a flat 50% no matter stack count. Updated 2013/08/11
    priest.buffs.empowered_shadows -> trigger(1, resource_consumed * 0.2);
  }

  virtual double action_da_multiplier() const override
  {
    double m = priest_spell_t::action_da_multiplier();

    m *= priest.resources.current[ current_resource() ];

    return m;
  }

  // Shortened and modified version of the ignite mechanic
  // Damage from the old dot is added to the new one.
  // Important here that the combined damage then will get multiplicated by the new orb amount.
  void trigger_dp_dot( player_t* t )
  {
    dot_t* dot = dot_spell -> get_dot( t );

    double previous_dp_dmg = 0;

    if ( dot -> ticking )
    {
      const shadow_orb_state_t& state = static_cast<const shadow_orb_state_t&>( *dot -> state );
      // Take the old damage without the orb multiplier
      previous_dp_dmg += state.result_amount / state.orbs_used * dot -> ticks();

      if ( sim -> debug )
        sim -> out_debug.printf( "%s DP still ticking. Added %.2f damage to new dot", player -> name(), previous_dp_dmg );
    }


    // Pass total amount of damage to the ignite dot_spell, and let it divide it by the correct number of ticks!

    action_state_t* s = dot_spell -> get_state();
    s -> target = t;
    dot_spell -> snapshot_state( s, DMG_OVER_TIME );
    s -> result = RESULT_HIT;
    s -> result_amount = previous_dp_dmg; // pass the old remaining dp damage to the dot_spell state, which will be catched in its impact method.
    dot_spell -> schedule_travel( s );
    dot_spell -> stats -> add_execute( timespan_t::zero(), s -> target );
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    trigger_dp_dot( s -> target );
  }
};

// Mind Flay Spell ==========================================================
template <bool insanity = false>
struct mind_flay_base_t : public priest_spell_t
{
  mind_flay_base_t( priest_t& p, const std::string& options_str, const std::string& name = "mind_flay" ) :
    priest_spell_t( name, p, p.find_class_spell( insanity ? "Mind Flay (Insanity)" : "Mind Flay" ) )
  {
    parse_options( nullptr, options_str );

    may_crit     = false;
    channeled    = true;
    hasted_ticks = false;

    if ( priest.perks.enhanced_mind_flay -> ok() )
    {
      num_ticks += 1;
      base_tick_time *= 1.0 + priest.perks.enhanced_mind_flay -> effectN( 1 ).percent();
    }
  }

  virtual double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    if ( priest.mastery_spells.mental_anguish -> ok() )
    {
      am *= 1.0 + priest.cache.mastery_value();
    }

    return am;
  }
};

struct mind_flay_insanity_t final : public mind_flay_base_t<true>
{
  typedef mind_flay_base_t<true> base_t;
  int orbs_used;

  mind_flay_insanity_t( priest_t& p, const std::string& options_str ) :
    base_t( p, options_str, "mind_flay_insanity" ),
      orbs_used( 0 )
  {
  }

  virtual double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    am *= 1.0 + orbs_used / 3.0;
    return am;
  }

  virtual bool ready() override
  {
    priest_td_t& td = get_td( target );
    if (!( priest.talents.solace_and_insanity -> ok() && td.dots.devouring_plague_tick -> ticking ))
      return false;

    const shadow_orb_state_t* dp_state = debug_cast<const shadow_orb_state_t*>( td.dots.devouring_plague_tick -> state );
    orbs_used = dp_state -> orbs_used;

    return base_t::ready();
  }
};

// Shadow Word Pain Spell ===================================================

struct shadow_word_pain_t final : public priest_spell_t
{
  shadowy_apparition_spell_t* proc_shadowy_apparition;

  shadow_word_pain_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "shadow_word_pain", p, p.find_class_spell( "Shadow Word: Pain" ) ),
    proc_shadowy_apparition( nullptr )
  {
    parse_options( nullptr, options_str );

    may_crit   = false;
    tick_zero  = true;

    base_multiplier *= 1.0 + p.sets.set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();

    base_multiplier *= 1.0 + p.perks.improved_shadow_word_pain -> effectN( 1 ).percent();

    base_crit   += p.sets.set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();

    num_ticks += ( int ) ( ( p.sets.set( SET_T14_4PC_CASTER ) -> effectN( 1 ).base_value() / 1000.0 ) / base_tick_time.total_seconds() );

    if ( priest.specs.shadowy_apparitions -> ok() )
    {
      proc_shadowy_apparition = new shadowy_apparition_spell_t( p );
      add_child( proc_shadowy_apparition );
    }
  }

  virtual void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( proc_shadowy_apparition && ( d -> state -> result_amount > 0 ) )
    {
      if ( d -> state -> result == RESULT_CRIT )
      {
        proc_shadowy_apparition -> trigger();
      }
    }
    if ( ( d -> state -> result_amount > 0 ) && ( priest.talents.divine_insight -> ok() ) )
    {
      if ( priest.buffs.divine_insight_shadow -> trigger() )
      {
        priest.cooldowns.mind_blast -> reset( true );
        priest.procs.divine_insight_shadow -> occur();
      }
    }
  }
};

// Vampiric Embrace Spell ===================================================

struct vampiric_embrace_t final : public priest_spell_t
{
  vampiric_embrace_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "vampiric_embrace", p, p.find_class_spell( "Vampiric Embrace" ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();
    priest.buffs.vampiric_embrace -> trigger();
  }

  virtual bool ready() override
  {
    if ( priest.buffs.vampiric_embrace -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Vampiric Touch Spell =====================================================

struct vampiric_touch_t final : public priest_spell_t
{
  shadowy_apparition_spell_t* proc_shadowy_apparition;

  vampiric_touch_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "vampiric_touch", p, p.find_class_spell( "Vampiric Touch" ) ),
    proc_shadowy_apparition( nullptr )
  {
    parse_options( nullptr, options_str );
    may_crit   = false;

    base_multiplier *= 1.0 + p.perks.improved_vampiric_touch -> effectN( 1 ).percent();

    num_ticks += ( int ) ( ( p.sets.set( SET_T14_4PC_CASTER ) -> effectN( 1 ).base_value() / 1000.0 ) / base_tick_time.total_seconds() );

    if ( priest.specs.shadowy_apparitions -> ok() )
      proc_shadowy_apparition = new shadowy_apparition_spell_t( p );
  }

  virtual void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    double m = player -> resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();
    player -> resource_gain( RESOURCE_MANA, m, priest.gains.vampiric_touch_mana, this );

    if ( priest.buffs.surge_of_darkness -> trigger() )
    {
      priest.procs.surge_of_darkness -> occur();
    }

    if ( priest.sets.has_set_bonus( SET_T15_4PC_CASTER ) )
    {
      if ( proc_shadowy_apparition && ( d -> state -> result_amount > 0 )  )
      {
        if ( rng().roll( priest.sets.set( SET_T15_4PC_CASTER ) -> proc_chance() ) )
        {
          priest.procs.t15_4pc_caster -> occur();

          proc_shadowy_apparition -> trigger();
        }
      }
    }
  }
};

struct holy_fire_base_t : public priest_spell_t
{
  holy_fire_base_t( const std::string& name, priest_t& p, const spell_data_t* sd ) :
    priest_spell_t( name, p, sd )
  {
    procs_courageous_primal_diamond = false;

    can_trigger_atonement = true;

    range += priest.glyphs.holy_fire -> effectN( 1 ).base_value();
  }

  virtual void execute() override
  {
    priest.buffs.holy_evangelism -> up();

    priest_spell_t::execute();

    priest.buffs.holy_evangelism -> trigger();
  }

  virtual double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    m *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return m;
  }

  virtual double cost() const override
  {
    double c = priest_spell_t::cost();

    if ( priest.buffs.chakra_chastise -> check() )
      c *= 1.0 + priest.buffs.chakra_chastise -> data().effectN( 3 ).percent();

    c *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }
};
// Power Word: Solace Spell =================================================

struct power_word_solace_t final : public holy_fire_base_t
{
  power_word_solace_t( priest_t& p, const std::string& options_str ) :
    holy_fire_base_t( "power_word_solace", p, p.find_spell( p.talents.solace_and_insanity -> effectN( 1 ).base_value() ) )
  {
    parse_options( nullptr, options_str );
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    double amount = data().effectN( 3 ).percent() / 100.0 * priest.resources.max[ RESOURCE_MANA ];
    priest.resource_gain( RESOURCE_MANA, amount, priest.gains.power_word_solace );
  }
};

// Holy Fire Spell ==========================================================

struct holy_fire_t final : public holy_fire_base_t
{
  holy_fire_t( priest_t& player, const std::string& options_str ) :
    holy_fire_base_t( "holy_fire", player, player.find_class_spell( "Holy Fire" ) )
  {
    parse_options( nullptr, options_str );
  }
};

// Penance Spell ============================================================

struct penance_t final : public priest_spell_t
{
  struct penance_tick_t : public priest_spell_t
  {
    penance_tick_t( priest_t& p, stats_t* stats ) :
      priest_spell_t( "penance_tick", p, p.find_spell( 47666 ) )
    {
      background  = true;
      dual        = true;
      direct_tick = true;
      can_trigger_atonement = true;

      this -> stats = stats;
    }

    void init() override
    {
      priest_spell_t::init();
      if ( atonement )
        atonement -> stats = player -> get_stats( "atonement_penance" );
    }
  };

  penance_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "penance", p, p.find_class_spell( "Penance" ) )
  {
    parse_options( nullptr, options_str );

    may_crit       = false;
    may_miss       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;
    castable_in_shadowform = false;

    // HACK: Set can_trigger here even though the tick spell actually
    // does the triggering. We want atonement_penance to be created in
    // priest_heal_t::init() so that it appears in the report.
    can_trigger_atonement = true;

    cooldown -> duration = data().cooldown() + p.sets.set( SET_T14_4PC_HEAL ) -> effectN( 1 ).time_value();

    dynamic_tick_action = true;
    tick_action = new penance_tick_t( p, stats );
  }

  virtual void init() override
  {
    priest_spell_t::init();
    if ( atonement )
      atonement -> channeled = true;
  }

  virtual bool usable_moving() const override
  { return priest.glyphs.penance -> ok(); }

  virtual double cost() const override
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    c *= 1.0 + priest.glyphs.penance -> effectN( 1 ).percent();

    return c;
  }

  virtual double action_multiplier() const override
  {
    double m = priest_spell_t::action_multiplier();

    m *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return m;
  }

  virtual void execute() override
  {
    priest.buffs.holy_evangelism -> up();
    priest_spell_t::execute();
    priest.buffs.holy_evangelism -> trigger();
  }
};

// Smite Spell ==============================================================

struct smite_t final : public priest_spell_t
{
  struct state_t : public action_state_t
  {
    bool glyph_benefit;
    state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      glyph_benefit( false ) { }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    { action_state_t::debug_str( s ) << " glyph_benefit=" << std::boolalpha << glyph_benefit; return s; }

    void initialize() override
    { action_state_t::initialize(); glyph_benefit = false; }

    void copy_state( const action_state_t* o ) override
    {
      action_state_t::copy_state( o );
      glyph_benefit = static_cast<const state_t&>( *o ).glyph_benefit;
    }
  };

  cooldown_t* hw_chastise;

  smite_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "smite", p, p.find_class_spell( "Smite" ) ),
    hw_chastise( p.get_cooldown( "holy_word_chastise" ) )
  {
    parse_options( nullptr, options_str );

    procs_courageous_primal_diamond = false;

    can_trigger_atonement = true;
    castable_in_shadowform = false;

    range += priest.glyphs.holy_fire -> effectN( 1 ).base_value();
  }

  virtual void execute() override
  {
    priest.buffs.holy_evangelism -> up();

    priest_spell_t::execute();

    priest.buffs.holy_evangelism -> trigger();
    priest.buffs.surge_of_light -> trigger();

    // Train of Thought
    if ( priest.specs.train_of_thought -> ok() )
      priest.cooldowns.penance -> adjust ( - priest.specs.train_of_thought -> effectN( 2 ).time_value() );

    if ( priest.buffs.chakra_chastise -> check() )
    {
      if ( rng().roll( priest.buffs.chakra_chastise -> data().proc_chance() ) )
        hw_chastise -> reset( true );
    }
  }

  virtual void update_ready( timespan_t cd_duration ) override
  {
    priest_spell_t::update_ready( cd_duration );
    assert( execute_state );
    priest.benefits.smites_with_glyph_increase -> update( static_cast<const state_t*>( execute_state ) -> glyph_benefit );
  }

  /* Check if Holy Fire or PW: Solace is up
   */
  bool glyph_benefit( player_t* t ) const
  {
    bool glyph_benefit = false;

    priest_td_t& td = get_td( t );
    if ( priest.talents.solace_and_insanity -> ok() )
      glyph_benefit = priest.glyphs.smite -> ok() && td. dots.power_word_solace -> ticking;
    else
      glyph_benefit = priest.glyphs.smite -> ok() && td.dots.holy_fire -> ticking;

    return glyph_benefit;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = priest_spell_t::composite_target_multiplier( target );

    if ( glyph_benefit( target ) )
      m *= 1.0 + priest.glyphs.smite -> effectN( 1 ).percent();

    return m;
  }

  virtual action_state_t* new_state() override
  { return new state_t( this, target ); }

  virtual void snapshot_state( action_state_t* s, dmg_e type ) override
  {
    state_t& state = static_cast<state_t&>( *s );

    state.glyph_benefit = glyph_benefit( s -> target );

    priest_spell_t::snapshot_state( s, type );
  }

  virtual void trigger_atonment( dmg_e type, action_state_t* s ) override
  {
    double atonement_dmg = s -> result_amount; // Normal dmg

    // If HF/PW:S was up, remove the extra damage from glyph
    if ( static_cast<const state_t*>( s ) -> glyph_benefit )
      atonement_dmg /= 1.0 + priest.glyphs.smite -> effectN( 1 ).percent();

    atonement -> trigger( atonement_dmg, direct_tick ? DMG_OVER_TIME : type, s -> result );
  }

  virtual double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    am *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return am;
  }

  virtual double cost() const override
  {
    double c = priest_spell_t::cost();

    if ( priest.buffs.chakra_chastise -> check() )
      c *= 1.0 + priest.buffs.chakra_chastise -> data().effectN( 3 ).percent();
    c *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }
};

// Cascade Spell

template <class Base>
struct cascade_base_t : public Base
{
private:
  typedef Base ab; // typedef for the templated action type, priest_spell_t, or priest_heal_t
public:
  typedef cascade_base_t base_t; // typedef for cascade_base_t<action_base_t>

  struct cascade_state_t : action_state_t
  {
    int jump_counter;

    cascade_state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      jump_counter( 0 )
    { }
  };

  std::vector<player_t*> targets; // List of available targets to jump to, created once at execute() and static during the jump process.

  virtual ~cascade_base_t() {}

  cascade_base_t( const std::string& n, priest_t& p, const std::string& options_str, const spell_data_t* scaling_data ) :
    ab( n, p, p.find_talent_spell( "Cascade" ) )
  {
    ab::parse_options( nullptr, options_str );

    ab::parse_effect_data( scaling_data -> effectN( 1 ) ); // Parse damage or healing numbers from the scaling spell
    ab::school       = scaling_data -> get_school_type();
    ab::travel_speed = scaling_data -> missile_speed();
  }

  virtual action_state_t* new_state() override
  {
    return new cascade_state_t( this, ab::target );
  }

  player_t* get_next_player()
  {
    player_t* t = nullptr;

    // Get target at first position
    if ( ! targets.empty() )
    {
      t = targets.back();
      targets.pop_back(); // Remove chosen target from the list.
    }

    return t;
  }

  virtual void populate_target_list() = 0;

  virtual void execute() override
  {
    // Clear and populate targets list
    targets.clear();
    populate_target_list();

    ab::execute();
  }

  virtual void impact( action_state_t* q ) override
  {
    ab::impact( q );

    cascade_state_t* cs = static_cast<cascade_state_t*>( q );

    assert( ab::data().effectN( 1 ).base_value() < 5 ); // Safety limit
    assert( ab::data().effectN( 2 ).base_value() < 5 ); // Safety limit

    if ( cs -> jump_counter < ab::data().effectN( 1 ).base_value() )
    {
      for ( int i = 0; i < ab::data().effectN( 2 ).base_value(); ++i )
      {
        player_t* t = get_next_player();

        if ( t )
        {
          if ( ab::sim -> debug )
            ab::sim -> out_debug.printf( "%s action %s jumps to player %s",
                               ab::player -> name(), ab::name(), t -> name() );


          // Copy-Pasted action_t::execute() code. Additionally increasing jump counter by one.
          cascade_state_t* s = debug_cast<cascade_state_t*>( ab::get_state() );
          s -> target = t;
          s -> n_targets = 1;
          s -> chain_target = 0;
          s -> jump_counter = cs -> jump_counter + 1;
          ab::snapshot_state( s, q -> target -> is_enemy() ? DMG_DIRECT : HEAL_DIRECT );
          s -> result = ab::calculate_result( s );

          if ( ab:: result_is_hit( s -> result ) )
            s -> result_amount = ab::calculate_direct_amount( s );

          if ( ab::sim -> debug )
            s -> debug();

          ab::schedule_travel( s );
        }
      }
    }
    else
    {
      cs -> jump_counter = 0;
    }
  }

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double ctdm = ab::composite_target_da_multiplier( t );

    double distance = ab::player -> current.distance;
    if ( distance >= 30.0 )
      return ctdm;

    // Source: Ghostcrawler 20/06/2012; http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97
    // 40% damage at 0 yards, 100% at 30, scaling linearly
    return ctdm * ( 0.4 + 0.6 * distance / 30.0 );
  }
};

struct cascade_damage_t final : public cascade_base_t<priest_spell_t>
{
  cascade_damage_t( priest_t& p, const std::string& options_str ) :
    base_t( "cascade_damage", p, options_str, p.find_spell( p.specialization() == PRIEST_SHADOW ? 127628 : 120785 ) )
  {}

  virtual void populate_target_list() override
  {
    for ( size_t i = 0; i < sim -> target_list.size(); ++i )
    {
      player_t* t = sim -> target_list[ i ];
      if ( t != target ) targets.push_back( t );
    }
  }
};

struct cascade_heal_t final : public cascade_base_t<priest_heal_t>
{
  cascade_heal_t( priest_t& p, const std::string& options_str ) :
    base_t( "cascade_heal", p, options_str, p.find_spell( p.specialization() == PRIEST_SHADOW ? 127629 : 121148 ) )
  { }

  virtual void populate_target_list() override
  {
    for ( size_t i = 0; i < sim -> player_list.size(); ++i )
    {
      player_t* t = sim -> player_list[ i ];
      if ( t != target ) targets.push_back( t );
    }
  }
};

// Halo Spell

// This is the background halo spell which does the actual damage
// Templated so we can base it on priest_spell_t or priest_heal_t
template <class Base, int spell_nr>
struct halo_base_t final : public Base
{
private:
  typedef Base ab; // typedef for the templated action type, priest_spell_t, or priest_heal_t
public:
  typedef halo_base_t base_t; // typedef for halo_base_t<ab>

  halo_base_t( const std::string& n, priest_t& p ) :
    ab( n, p, p.find_spell( spell_nr ) )
  {
    ab::aoe = -1;
    ab::background = true;

    if ( ab::data().ok() )
    {
      // Reparse the correct effect number, because we have two competing ones ( were 2 > 1 always wins out )
      ab::parse_effect_data( ab::data().effectN( 1 ) );
    }
  }
  virtual ~halo_base_t() {}

  virtual double composite_target_da_multiplier( player_t* t ) const override
  {
    double ctdm = ab::composite_target_da_multiplier( t );

    // Source: Ghostcrawler 20/06/2012
    // http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97

    double distance = ab::player -> current.distance; // Replace with whatever we measure distance

    //double mult = 0.5 * pow( 1.01, -1 * pow( ( distance - 25 ) / 2, 4 ) ) + 0.1 + 0.015 * distance;
    double mult = 0.5 * exp( -0.00995 * pow( distance / 2 - 12.5, 4 ) ) + 0.1 + 0.015 * distance;

    return ctdm * mult;
  }
};

struct halo_t final : public priest_spell_t
{
  /* Go through some hoops to dynamically choose the correct halo sub-spell
   * dynamically depending on the talent spell, without having to create
   * two separate halo_t versions
   */
  typedef halo_base_t<priest_spell_t, 120696> halo_damage_t;
  typedef halo_base_t<priest_heal_t, 120692> halo_heal_t;

  halo_damage_t* damage_spell;
  halo_heal_t* heal_spell;

  halo_damage_t* get_damage_spell( const spell_data_t* talent_spell, priest_t& p ) const
  {
    if ( talent_spell->id() == 120644 ) { // shadow version
      return new halo_damage_t( "halo_dmg", p );
    }
    else
      return nullptr;
  }
  halo_heal_t* get_heal_spell( const spell_data_t* talent_spell, priest_t& p ) const
  {
    if ( talent_spell -> id() == 120517 ) { // holy/disc
      return new halo_heal_t( "halo_heal", p );
    }
    else
      return nullptr;
  }
  halo_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "halo", p, p.talents.halo ),
    damage_spell( get_damage_spell( p.talents.halo, p ) ),
    heal_spell( get_heal_spell( p.talents.halo, p ) )
  {
    parse_options( nullptr, options_str );

    if ( heal_spell )
      add_child( heal_spell );

    if ( damage_spell )
      add_child( damage_spell );
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    if ( damage_spell )
      damage_spell -> execute();

    if ( heal_spell )
      heal_spell -> execute();
  }
};

// Divine Star spell

template <class Base, int scaling_effect_index>
struct divine_star_base_t final : public Base
{
private:
  typedef Base ab; // the action base ("ab") type (priest_spell_t or priest_heal_t)
protected:
  typedef divine_star_base_t base_t;

  divine_star_base_t* return_spell;

public:
  divine_star_base_t( const std::string& n, priest_t& p, const spell_data_t* spell_data, bool is_return_spell = false ) :
    ab( n, p, spell_data ),
    return_spell( ( is_return_spell ? nullptr : new divine_star_base_t( n, p, spell_data, true ) ) )
  {
    ab::aoe = -1;

    if ( ab::data().ok() )
    {
      // Reparse the correct effect number, because we have two competing ones ( were 2 > 1 always wins out )

      // Commented out for now, was causing asserts. Doesn't look like this
      // is needed any more, but don't want to totally remove it just in case.
      // -- Twintop / 2014-05-16
      //ab::parse_effect_data( ab::data().effectN( scaling_effect_index ) );
    }

    ab::proc = ab::background = true;
  }

  // Divine Star will damage and heal targets twice, once on the way out and
  // again on the way back. This is determined by distance from the target.
  // If we are too far away, it misses completely. If we are at the very
  // edge distance wise, it will only hit once. If we are within range (and
  // aren't moving such that it would miss the target on the way out and/or
  // back), it will hit twice. Threshold is 24 yards, per tooltip and tests
  // for 2 hits. 28 yards is the threshold for 1 hit.
  virtual void execute() override
  {
    double distance = ab::player -> current.distance;

    if ( distance <= 28 )
    {
      ab::execute();

      if ( return_spell && distance <= 24 )
        return_spell -> execute();
    }
  }
};

struct divine_star_t final : public priest_spell_t
{
  typedef divine_star_base_t<priest_spell_t, 1> ds_damage_t;
  typedef divine_star_base_t<priest_heal_t, 2> ds_heal_t;

  ds_damage_t* damage_spell;
  ds_heal_t* heal_spell;

  divine_star_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "divine_star", p, p.talents.divine_star -> ok() ? p.find_spell( p.specialization() == PRIEST_SHADOW ? 122121 : 110744 ) : spell_data_t::not_found() ),
    damage_spell( new ds_damage_t( "divine_star_damage", p, data().effectN( 1 ).trigger() ) ),
    heal_spell( new ds_heal_t( "divine_star_heal", p, data().effectN( 1 ).trigger() ) )
  {
    parse_options( 0, options_str );

    // Disable ticking (There is a periodic effect described in the base spell
    // that does no damage. I assume the Star checks every 250 milliseconds for
    // new targets coming into range).

    //num_ticks = 0;
    //base_tick_time = timespan_t::zero();

    // Have the primary DS spell take on the stats that are most appropriate to the player's role
    // so it shows up nicely in the DPET chart.
    if ( p.primary_role() == ROLE_HEAL )
      add_child( heal_spell );
    else
      add_child( damage_spell );
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    if ( damage_spell )
      damage_spell -> execute();

    if ( heal_spell )
      heal_spell -> execute();
  }
};

struct void_entropy_t : public priest_spell_t
 {
  void_entropy_t( priest_t& p, const std::string& options_str ) :
     priest_spell_t( "void_entropy", p, p.talents.void_entropy )
   {
     parse_options( nullptr, options_str );

   }

  virtual void consume_resource() override
  {
    resource_consumed = cost();

    if ( execute_state -> result != RESULT_MISS )
    {
      resource_consumed = shadow_orbs_to_consume();
    }

    player -> resource_loss( current_resource(), resource_consumed, nullptr, this );

    if ( sim -> log )
      sim -> out_log.printf( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

   virtual double action_ta_multiplier() const override
   {
     double m = priest_spell_t::action_ta_multiplier();

     m *= shadow_orbs_to_consume();

     return m;
   }
 };
} // NAMESPACE spells

// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================

namespace heals {

// Binding Heal Spell =======================================================

struct binding_heal_t final : public priest_heal_t
{
  binding_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "binding_heal", p, p.find_class_spell( "Binding Heal" ) )
  {
    parse_options( nullptr, options_str );

    aoe = 2;
  }

  virtual void init() override
  {
    priest_heal_t::init();

    target_cache.list.clear();
    target_cache.list.push_back( target );
    target_cache.list.push_back( player );
  }

  virtual size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    assert( tl.size() == 2 );

    if ( tl[ 0 ] != target )
      tl[ 0 ] = target;

    return tl.size();
  }

  virtual void execute() override
  {
    priest_heal_t::execute();

    consume_inner_focus();
    trigger_serendipity();
    trigger_surge_of_light();
  }
};

// Circle of Healing ========================================================

struct circle_of_healing_t final : public priest_heal_t
{
  circle_of_healing_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "circle_of_healing", p, p.find_class_spell( "Circle of Healing" ) )
  {
    parse_options( nullptr, options_str );

    base_costs[ current_resource() ] *= 1.0 + p.glyphs.circle_of_healing -> effectN( 2 ).percent();
    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );
    aoe = p.glyphs.circle_of_healing -> ok() ? 6 : 5;
  }

  virtual void execute() override
  {
    cooldown -> duration = data().cooldown();
    cooldown -> duration += priest.sets.set( SET_T14_4PC_HEAL ) -> effectN( 2 ).time_value();
    if ( priest.buffs.chakra_sanctuary -> up() )
      cooldown -> duration += priest.buffs.chakra_sanctuary -> data().effectN( 2 ).time_value();

    // Choose Heal Target
    target = find_lowest_player();

    priest_heal_t::execute();

    priest.buffs.absolution -> trigger();
  }

  virtual double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

// Desperate Prayer =========================================================

struct desperate_prayer_t final : public priest_heal_t
{
  desperate_prayer_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "desperate_prayer", p, p.talents.desperate_prayer )
  {
    parse_options( nullptr, options_str );

    target = &p; // always targets the priest himself
  }
};

// Divine Hymn Spell ========================================================

struct divine_hymn_tick_t final : public priest_heal_t
{
  divine_hymn_tick_t( priest_t& player, int nr_targets ) :
    priest_heal_t( "divine_hymn_tick", player, player.find_spell( 64844 ) )
  {
    background  = true;

    aoe = nr_targets;
  }
};

struct divine_hymn_t final : public priest_heal_t
{
  divine_hymn_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "divine_hymn", p, p.find_class_spell( "Divine Hymn" ) )
  {
    parse_options( nullptr, options_str );

    harmful = false;
    channeled = true;
    dynamic_tick_action = true;

    tick_action = new divine_hymn_tick_t( p, data().effectN( 2 ).base_value() );
    add_child( tick_action );
  }

  virtual double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

// Echo of Light

struct echo_of_light_t final : public ignite::pct_based_action_t<priest_heal_t>
{
  echo_of_light_t( priest_t& p ) :
    base_t( "echo_of_light", p, p.find_spell( 77489 ) )
  {
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks      = as<int>( data().duration() / base_tick_time );
  }
};

// Flash Heal Spell =========================================================

struct flash_heal_t final : public priest_heal_t
{
  flash_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "flash_heal", p, p.find_class_spell( "Flash Heal" ) )
  {
    parse_options( nullptr, options_str );
    can_trigger_spirit_shell = true;
  }

  virtual void execute() override
  {
    priest_heal_t::execute();

    consume_inner_focus();
    trigger_serendipity();
    consume_surge_of_light();
    trigger_surge_of_light();
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    trigger_grace( s -> target );

    if ( ! priest.buffs.spirit_shell -> check() )
      trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit() const override
  {
    double cc = priest_heal_t::composite_crit();

    if ( priest.buffs.inner_focus -> check() )
      cc += priest.buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual timespan_t execute_time() const override
  {
    if ( priest.buffs.surge_of_light -> check() )
      return timespan_t::zero();

    return priest_heal_t::execute_time();
  }

  virtual double cost() const override
  {
    if ( priest.buffs.surge_of_light -> check() )
      return 0.0;

    double c = priest_heal_t::cost();

    c *= 1.0 + priest.sets.set( SET_T14_2PC_HEAL ) -> effectN( 1 ).percent();

    return c;
  }
};

// Guardian Spirit ==========================================================

struct guardian_spirit_t final : public priest_heal_t
{
  guardian_spirit_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "guardian_spirit", p, p.find_class_spell( "Guardian Spirit" ) )
  {
    parse_options( nullptr, options_str );

    base_dd_min = base_dd_max = 0.0; // The absorb listed isn't a real absorb
    harmful = false;
  }

  virtual void execute() override
  {
    priest_heal_t::execute();
    target -> buffs.guardian_spirit -> trigger();
  }
};

// Greater Heal Spell =======================================================

struct greater_heal_t final : public priest_heal_t
{
  greater_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "greater_heal", p, p.find_class_spell( "Greater Heal" ) )
  {
    parse_options( nullptr, options_str );
    can_trigger_spirit_shell = true;
  }

  virtual void execute() override
  {
    priest_heal_t::execute();

    // Train of Thought
    // NOTE: Process Train of Thought _before_ Inner Focus: the GH that consumes Inner Focus does not
    //       reduce the cooldown, since Inner Focus doesn't go on cooldown until after it is consumed.
    if ( priest.specs.train_of_thought -> ok() )
      priest.cooldowns.inner_focus -> adjust( timespan_t::from_seconds( - priest.specs.train_of_thought -> effectN( 1 ).base_value() ) );

    consume_inner_focus();
    consume_serendipity();
    trigger_surge_of_light();
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    trigger_grace( s -> target );
    if ( ! priest.buffs.spirit_shell -> check() )
      trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit() const override
  {
    double cc = priest_heal_t::composite_crit();

    if ( priest.buffs.inner_focus -> check() )
      cc += priest.buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    am *= 1.0 + priest.sets.set( SET_T16_2PC_HEAL ) -> effectN( 1 ).percent() * priest.buffs.serendipity -> check();

    return am;
  }

  virtual double cost() const override
  {
    double c = priest_heal_t::cost();

    if ( priest.buffs.serendipity -> check() )
      c *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 2 ).percent();

    return c;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t et = priest_heal_t::execute_time();

    if ( priest.buffs.serendipity -> check() )
      et *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 1 ).percent();

    return et;
  }
};

// Holy Word Sanctuary ======================================================

struct holy_word_sanctuary_t final : public priest_heal_t
{
  struct holy_word_sanctuary_tick_t final : public priest_heal_t
  {
    holy_word_sanctuary_tick_t( priest_t& player ) :
      priest_heal_t( "holy_word_sanctuary_tick", player, player.find_spell( 88686 ) )
    {
      dual        = true;
      background  = true;
      direct_tick = true;
      can_trigger_EoL = false; // http://us.battle.net/wow/en/forum/topic/5889309137?page=107#2137
    }

    virtual double action_multiplier() const override
    {
      double am = priest_heal_t::action_multiplier();

      if ( priest.buffs.chakra_sanctuary -> up() )
        am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

      // Spell data from the buff ( 15% ) is correct, not the one in the triggering spell ( 50% )
      am *= 1.0 + priest.buffs.absolution -> check() * priest.buffs.absolution -> data().effectN( 1 ).percent();

      return am;
    }
  };

  holy_word_sanctuary_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "holy_word_sanctuary", p, p.find_spell( 88685 ) )
  {
    parse_options( nullptr, options_str );

    may_crit     = false;

    base_tick_time = timespan_t::from_seconds( 2.0 );
    num_ticks = 9;

    tick_action = new holy_word_sanctuary_tick_t( p );

    // Needs testing
    cooldown -> duration *= 1.0 + p.sets.has_set_bonus( SET_T13_4PC_HEAL ) * -0.2;
  }

  virtual void execute() override
  {
    priest_heal_t::execute();

    priest.buffs.absolution -> expire();
  }

  virtual bool ready() override
  {
    if ( ! priest.buffs.chakra_sanctuary -> check() )
      return false;

    return priest_heal_t::ready();
  }

  // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility

};

// Holy Word Chastise =======================================================

struct holy_word_chastise_t final : public priest_spell_t
{
  holy_word_chastise_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "holy_word_chastise", p, p.find_class_spell( "Holy Word: Chastise" ) )
  {
    parse_options( nullptr, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p.sets.has_set_bonus( SET_T13_4PC_HEAL ) * -0.2;

    castable_in_shadowform = false;
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    priest.buffs.absolution -> expire();
  }

  virtual double action_multiplier() const override
  {
    double am = priest_spell_t::action_multiplier();

    // Spell data from the buff ( 15% ) is correct, not the one in the triggering spell ( 50% )
    am *= 1.0 + priest.buffs.absolution -> check() * priest.buffs.absolution -> data().effectN( 1 ).percent();

    return am;
  }

  virtual bool ready() override
  {
    if ( priest.buffs.chakra_sanctuary -> check() )
      return false;

    if ( priest.buffs.chakra_serenity -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Holy Word Serenity =======================================================

struct holy_word_serenity_t final : public priest_heal_t
{
  holy_word_serenity_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "holy_word_serenity", p, p.find_spell( 88684 ) )
  {
    parse_options( nullptr, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration = data().cooldown() * ( 1.0 + p.sets.has_set_bonus( SET_T13_4PC_HEAL ) * -0.2 );
  }

  virtual void execute() override
  {
    priest_heal_t::execute();

    priest.buffs.absolution -> expire();
  }

  virtual double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    // Spell data from the buff ( 15% ) is correct, not the one in the triggering spell ( 50% )
    am *= 1.0 + priest.buffs.absolution -> check() * priest.buffs.absolution -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    get_td( s -> target ).buffs.holy_word_serenity -> trigger();
  }

  virtual bool ready() override
  {
    if ( ! priest.buffs.chakra_serenity -> check() )
      return false;

    return priest_heal_t::ready();
  }
};

// Holy Word ================================================================

struct holy_word_t final : public priest_spell_t
{
  holy_word_sanctuary_t* hw_sanctuary;
  holy_word_chastise_t*  hw_chastise;
  holy_word_serenity_t*  hw_serenity;

  holy_word_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "holy_word", p, spell_data_t::nil() ),
    hw_sanctuary( new holy_word_sanctuary_t( p, options_str ) ),
    hw_chastise(  new holy_word_chastise_t ( p, options_str ) ),
    hw_serenity(  new holy_word_serenity_t ( p, options_str ) )
  {
    school = SCHOOL_HOLY;

    castable_in_shadowform = false;
  }

  virtual void init() override
  {
    priest_spell_t::init();

    hw_sanctuary -> action_list = action_list;
    hw_chastise -> action_list = action_list;
    hw_serenity -> action_list = action_list;
  }

  virtual void schedule_execute( action_state_t* state = 0 ) override
  {
    action_t* a;

    if ( priest.buffs.chakra_serenity -> up() )
      a = hw_serenity;
    else if ( priest.buffs.chakra_sanctuary -> up() )
      a = hw_sanctuary;
    else
      a = hw_chastise;

    player -> last_foreground_action = a;
    a -> schedule_execute( state );
  }

  virtual void execute() override
  {
    assert( false );
  }

  virtual bool ready() override
  {
    if ( priest.buffs.chakra_serenity -> check() )
      return hw_serenity -> ready();

    else if ( priest.buffs.chakra_sanctuary -> check() )
      return hw_sanctuary -> ready();

    else
      return hw_chastise -> ready();
  }
};

/* Lightwell Spell
 * Create only if ( p.pets.lightwell )
 */
struct lightwell_t final : public priest_spell_t
{
  timespan_t consume_interval;
  cooldown_t* lightwell_renew_cd;

  lightwell_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "lightwell", p, p.find_class_spell( "Lightwell" ) ),
    consume_interval( timespan_t::from_seconds( 10 ) ),
    lightwell_renew_cd( priest.pets.lightwell -> get_cooldown( "lightwell_renew" ) )
  {
    option_t options[] =
    {
      opt_timespan( "consume_interval", consume_interval ),
      opt_null()
    };
    parse_options( options, options_str );

    harmful = false;

    castable_in_shadowform = false;

    assert( consume_interval > timespan_t::zero() && consume_interval < cooldown -> duration );
  }

  virtual void execute() override
  {
    priest_spell_t::execute();

    lightwell_renew_cd -> duration = consume_interval;
    priest.pets.lightwell -> summon( data().duration() );
  }
};

// Penance Heal Spell =======================================================

struct penance_heal_t final : public priest_heal_t
{
  struct penance_heal_tick_t final : public priest_heal_t
  {
    penance_heal_tick_t( priest_t& player ) :
      priest_heal_t( "penance_heal_tick", player, player.find_spell( 47666 ) )
    {
      background  = true;
      may_crit    = true;
      dual        = true;
      direct_tick = true;

      school = SCHOOL_HOLY;
      stats = player.get_stats( "penance_heal", this );
    }

    virtual void impact( action_state_t* s ) override
    {
      priest_heal_t::impact( s );

      trigger_grace( s -> target );
    }
  };

  penance_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "penance_heal", p, p.find_class_spell( "Penance" ) )
  {
    parse_options( nullptr, options_str );

    may_crit       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;
    dynamic_tick_action = true;

    cooldown = p.cooldowns.penance;
    cooldown -> duration = data().cooldown() + p.sets.set( SET_T14_4PC_HEAL ) -> effectN( 1 ).time_value();

    tick_action = new penance_heal_tick_t( p );
  }

  virtual double cost() const override
  {
    double c = priest_heal_t::cost();

    c *= 1.0 + priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent();

    c *= 1.0 + priest.glyphs.penance -> effectN( 1 ).percent();

    return c;
  }
};

// Power Word: Shield Spell =================================================

struct power_word_shield_t final : public priest_absorb_t
{
  struct glyph_power_word_shield_t final : public priest_heal_t
  {
    glyph_power_word_shield_t( priest_t& player ) :
      priest_heal_t( "power_word_shield_glyph", player, player.find_spell( 55672 ) )
    {
      school          = SCHOOL_HOLY;

      background = true;
      proc       = true;

      snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;

      castable_in_shadowform = true;
    }

    void trigger( action_state_t* s )
    {
      base_dd_min = base_dd_max = priest.glyphs.power_word_shield -> effectN( 1 ).percent() * s -> result_amount;
      target = s -> target;
      execute();
    }
  };

  glyph_power_word_shield_t* glyph_pws;
  bool ignore_debuff;

  power_word_shield_t( priest_t& p, const std::string& options_str ) :
    priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ),
    glyph_pws( nullptr ),
    ignore_debuff( false )
  {
    option_t options[] =
    {
      opt_bool( "ignore_debuff", ignore_debuff ),
      opt_null()
    };
    parse_options( options, options_str );

    // Tooltip is wrong.
    // direct_power_mod = 0.87; // hardcoded into tooltip
    spell_power_mod.direct = 1.8709; // matches in-game actual value

    if ( p.glyphs.power_word_shield -> ok() )
    {
      glyph_pws = new glyph_power_word_shield_t( p );
      //add_child( glyph_pws );
    }

    castable_in_shadowform = true;
  }

  virtual void impact( action_state_t* s ) override
  {
    s -> target -> buffs.weakened_soul -> trigger();
    priest.buffs.borrowed_time -> trigger();

    // Glyph
    if ( glyph_pws )
      glyph_pws -> trigger( s );

    get_td( s -> target ).buffs.power_word_shield -> trigger( 1, s -> result_amount );
    stats -> add_result( 0, s -> result_amount, ABSORB, s -> result, s -> block_result, s -> target );
  }

  virtual bool ready() override
  {
    if ( ! ignore_debuff && target -> buffs.weakened_soul -> check() )
      return false;

    return priest_absorb_t::ready();
  }
};

// Prayer of Healing Spell ==================================================

struct prayer_of_healing_t final : public priest_heal_t
{
  prayer_of_healing_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_healing", p, p.find_class_spell( "Prayer of Healing" ) )
  {
    parse_options( nullptr, options_str );
    aoe = 5;
    group_only = true;
    divine_aegis_trigger_mask = RESULT_HIT_MASK;
    can_trigger_spirit_shell = true;
  }

  virtual void execute() override
  {
    priest_heal_t::execute();
    consume_inner_focus();
    consume_serendipity();
  }

  virtual double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    am *= 1.0 + priest.sets.set( SET_T16_2PC_HEAL ) -> effectN( 1 ).percent() * priest.buffs.serendipity -> check();

    return am;
  }

  virtual double composite_crit() const override
  {
    double cc = priest_heal_t::composite_crit();

    if ( priest.buffs.inner_focus -> check() )
      cc += priest.buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double cost() const override
  {
    double c = priest_heal_t::cost();

    if ( priest.buffs.serendipity -> check() )
      c *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 2 ).percent();

    return c;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t et = priest_heal_t::execute_time();

    if ( priest.buffs.serendipity -> check() )
      et *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 1 ).percent();

    return et;
  }
};

// Prayer of Mending Spell ==================================================

struct prayer_of_mending_t final : public priest_heal_t
{
  bool single;

  prayer_of_mending_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_mending", p, p.find_class_spell( "Prayer of Mending" ) ),
    single( false )
  {
    option_t options[] =
    {
      opt_bool( "single", single ),
      opt_null()
    };
    parse_options( options, options_str );

    spell_power_mod.direct = data().effectN( 1 ).coeff();
    base_dd_min = base_dd_max = data().effectN( 1 ).min( &p );

    divine_aegis_trigger_mask = 0;

    aoe = 5;

    castable_in_shadowform = true;
  }

  virtual void execute() override
  {
    priest_heal_t::execute();

    priest.buffs.absolution -> trigger();
  }

  virtual double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double ctm = priest_heal_t::composite_target_multiplier( t );

    if ( priest.glyphs.prayer_of_mending -> ok() && t == target )
      ctm *= 1.0 + priest.glyphs.prayer_of_mending -> effectN( 1 ).percent();

    return ctm;
  }
};

// Renew Spell ==============================================================

struct renew_t final : public priest_heal_t
{
  struct rapid_renewal_t final : public priest_heal_t
  {
    rapid_renewal_t( priest_t& p ) :
      priest_heal_t( "rapid_renewal", p, p.specs.rapid_renewal )
    {
      background = true;
      proc       = true;
    }

    void trigger( action_state_t* s, double amount )
    {
      base_dd_min = base_dd_max = amount * data().effectN( 3 ).percent();
      target = s -> target;
      execute();
    }

    virtual double composite_da_multiplier() const override
    { return 1.0; }
  };
  rapid_renewal_t* rr;

  renew_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "renew", p, p.find_class_spell( "Renew" ) ),
    rr( nullptr )
  {
    parse_options( nullptr, options_str );

    may_crit = false;

    if ( p.specs.rapid_renewal -> ok() )
    {
      rr = new rapid_renewal_t( p );
      add_child( rr );
      base_multiplier *= 1.0 + p.specs.rapid_renewal -> effectN( 2 ).percent();
    }

    base_multiplier *= 1.0 + p.glyphs.renew -> effectN( 1 ).percent();
    num_ticks       += ( int ) ( p.glyphs.renew -> effectN( 2 ).time_value() / base_tick_time );

    castable_in_shadowform = true;
  }

  virtual double action_multiplier() const override
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s ) override
  {
    priest_heal_t::impact( s );

    if ( rr )
    {
      dot_t* d = get_dot( s -> target );
      result_e r = d -> state -> result;
      d -> state -> result = RESULT_HIT;
      double tick_dmg = calculate_tick_amount( d -> state );
      d -> state -> result = r;
      tick_dmg *= d -> ticks(); // Gets multiplied by the hasted amount of ticks
      rr -> trigger( s, tick_dmg );
    }
  }
};

struct clarity_of_will_t final : public priest_heal_t
{
  clarity_of_will_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "clarity_of_will", p, p.find_spell( 0 /*p.talents.divine_clarity */ ) )
  {
    parse_options( nullptr, options_str );
    // TODO: implement mechanic
  }
};

struct clarity_of_purpose_t final : public priest_heal_t
{
  clarity_of_purpose_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "clarity_of_purpose", p, p.find_spell( 0 /*p.talents.divine_clarity */ ) )
  {
    parse_options( nullptr, options_str );
    // can_trigger_spirit_shell = true; ?
    // TODO: implement mechanic
  }
};

struct saving_grace_t final : public priest_heal_t
{
  saving_grace_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "saving_grace", p, p.find_spell( 0 /*p.talents.spiritual_guidance */ ) )
  {
    parse_options( nullptr, options_str );
    // can_trigger_spirit_shell = true; ?

    pct_heal = 0.50; // TODO: spelldata
  }

  virtual void impact( action_state_t* s )
  {
    priest_heal_t::impact( s );

    priest.buffs.saving_grace_penalty -> trigger();
  }
};



} // NAMESPACE heals

} // NAMESPACE actions

namespace buffs { // namespace buffs

/* This is a template for common code between priest buffs.
 * The template is instantiated with any type of buff ( buff_t, debuff_t, absorb_buff_t, etc. ) as the 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived class,
 * don't skip it and call buff_t/absorb_buff_t/etc. directly.
 */
template <typename Base>
struct priest_buff_t : public Base
{
public:
  typedef priest_buff_t base_t; // typedef for priest_buff_t<buff_base_t>

  priest_buff_t( priest_td_t& p, const buff_creator_basics_t& params ) :
    Base( params ), priest( p.priest )
  { }

  priest_buff_t( priest_t& p, const buff_creator_basics_t& params ) :
    Base( params ), priest( p )
  { }

  priest_td_t& get_td( player_t* t ) const
  { return *( priest.get_target_data( t ) ); }

protected:
  priest_t& priest;
};

/* Custom shadowform buff
 * trigger/cancels spell haste aura
 */
struct shadowform_t final : public priest_buff_t<buff_t>
{
  shadowform_t( priest_t& p ) :
    base_t( p, buff_creator_t( &p, "shadowform" )
               .spell( p.find_class_spell( "Shadowform" ) )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  { }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    if ( ! sim -> overrides.haste )
      sim -> auras.haste -> trigger();

    return r;
  }

  virtual void expire_override() override
  {
    base_t::expire_override();

    if ( ! sim -> overrides.haste )
      sim -> auras.haste -> decrement();
  }
};

/* Custom archangel buff
 * snapshots evangelism stacks and expires it
 */
struct archangel_t final : public priest_buff_t<buff_t>
{
  archangel_t( priest_t& p ) :
    base_t( p, buff_creator_t( &p, "archangel" ).spell( p.specs.archangel ).max_stack( 5 ) )
  {
    default_value = data().effectN( 1 ).percent();

    if ( priest.sets.has_set_bonus( SET_T16_2PC_HEAL ) )
    {
      add_invalidate( CACHE_CRIT );

    }
  }

  virtual bool trigger( int stacks, double /* value */, double chance, timespan_t duration ) override
  {
    stacks = priest.buffs.holy_evangelism -> stack();
    double archangel_value = default_value * stacks;
    bool success = base_t::trigger( stacks, archangel_value, chance, duration );

    priest.buffs.holy_evangelism -> expire();

    return success;
  }
};

/* Custom divine insight buff
 */
struct divine_insight_shadow_t final : public priest_buff_t<buff_t>
{
  // Get correct spell data
  static const spell_data_t* sd( priest_t& p )
  {
    if ( p.talents.divine_insight -> ok() && ( p.specialization() == PRIEST_SHADOW ) )
      return p.talents.divine_insight -> effectN( 2 ).trigger();
    return spell_data_t::not_found();
  }

  divine_insight_shadow_t( priest_t& p ) :
    base_t( p, buff_creator_t( &p, "divine_insight_shadow" )
            .spell( sd( p ) )
          )
  {
    default_chance = data().ok() ? 0.05 : 0.0; // 5% hardcoded into tooltip, 3/12/2012
  }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool success = base_t::trigger( stacks, value, chance, duration );

    if ( success )
    {
      priest.cooldowns.mind_blast -> reset( true );
      priest.procs.divine_insight_shadow -> occur();
    }

    return success;
  }
};

struct spirit_shell_t final : public priest_buff_t<buff_t>
{
  spirit_shell_t( priest_t& p ) :
    base_t( p, buff_creator_t( &p, "spirit_shell" ).spell( p.find_class_spell( "Spirit Shell" ) ).cd( timespan_t::zero() ) )
  { }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool success = base_t::trigger( stacks, value, chance, duration );

    if ( success )
    {
      priest.buffs.resolute_spirit -> trigger();
    }

    return success;
  }
};
} // end namespace buffs


// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p ) :
  actor_pair_t( target, &p ),
  dots(),
  buffs(),
  priest( p )
{
  dots.holy_fire             = target -> get_dot( "holy_fire",             &p );
  dots.power_word_solace     = target -> get_dot( "power_word_solace",     &p );
  dots.devouring_plague_tick = target -> get_dot( "devouring_plague_tick", &p );
  dots.shadow_word_pain      = target -> get_dot( "shadow_word_pain",      &p );
  dots.vampiric_touch        = target -> get_dot( "vampiric_touch",        &p );
  dots.renew                 = target -> get_dot( "renew",                 &p );

  buffs.divine_aegis = absorb_buff_creator_t( *this, "divine_aegis", p.find_spell( 47753 ) )
                       .source( p.get_stats( "divine_aegis" ) );

  buffs.power_word_shield = absorb_buff_creator_t( *this, "power_word_shield", p.find_spell( 17 ) )
                            .source( p.get_stats( "power_word_shield" ) )
                            .cd( timespan_t::zero() );

  buffs.spirit_shell = absorb_buff_creator_t( *this, "spirit_shell_absorb" )
                       .spell( p.find_spell( 114908 ) )
                       .source( p.get_stats( "spirit_shell" ) );

  buffs.holy_word_serenity = buff_creator_t( *this, "holy_word_serenity" )
                             .spell( p.find_spell( 88684 ) )
                             .cd( timespan_t::zero() )
                             .activated( false );
}

// ==========================================================================
// Priest Definitions
// ==========================================================================

/* Construct priest cooldowns
 *
 */
void priest_t::create_cooldowns()
{
  cooldowns.mind_blast   = get_cooldown( "mind_blast" );
  cooldowns.shadowfiend  = get_cooldown( "shadowfiend" );
  cooldowns.mindbender   = get_cooldown( "mindbender" );
  cooldowns.chakra       = get_cooldown( "chakra" );
  cooldowns.inner_focus  = get_cooldown( "inner_focus" );
  cooldowns.penance      = get_cooldown( "penance" );
}

/* Construct priest gains
 *
 */
void priest_t::create_gains()
{
  gains.dispersion                    = get_gain( "dispersion" );
  gains.shadowfiend                   = get_gain( "shadowfiend" );
  gains.mindbender                    = get_gain( "mindbender" );
  gains.power_word_solace             = get_gain( "Power Word: Solace Mana" );
  gains.shadow_orb_mb                 = get_gain( "Shadow Orbs from Mind Blast" );
  gains.shadow_orb_swd                = get_gain( "Shadow Orbs from Shadow Word: Death" );
  gains.auspicious_spirits            = get_gain( "Shadow Orbs from Auspicious Spirits" );
  gains.devouring_plague_health       = get_gain( "Devouring Plague Health" );
  gains.vampiric_touch_mana           = get_gain( "Vampiric Touch Mana" );
  gains.vampiric_touch_mastery_mana   = get_gain( "Vampiric Touch Mastery Mana" );
  gains.clarity_of_power_mind_spike   = get_gain( "Clarity of Power Mind Spike" );
}

/* Construct priest procs
 *
 */
void priest_t::create_procs()
{
  procs.mastery_extra_tick               = get_proc( "Shadowy Recall Extra Tick"                      );
  procs.shadowy_apparition               = get_proc( "Shadowy Apparition Procced"                     );
  procs.divine_insight_shadow            = get_proc( "Divine Insight Mind Blast CD Reset"             );
  procs.surge_of_darkness                = get_proc( "FDCL Mind Spike proc"                           );
  procs.surge_of_light                   = get_proc( "Surge of Light"                                 );
  procs.surge_of_light_overflow          = get_proc( "Surge of Light lost to overflow"                );
  procs.mind_spike_dot_removal           = get_proc( "Mind Spike removed DoTs"                        );
  procs.t15_2pc_caster                   = get_proc( "Tier15 2pc caster"                              );
  procs.t15_4pc_caster                   = get_proc( "Tier15 4pc caster"                              );
  procs.t15_2pc_caster_shadow_word_pain  = get_proc( "Tier15 2pc caster Shadow Word: Pain Extra Tick" );
  procs.t15_2pc_caster_vampiric_touch    = get_proc( "Tier15 2pc caster Vampiric Touch Extra Tick"    );
}

/* Construct priest benefits
 *
 */
void priest_t::create_benefits()
{
  benefits.smites_with_glyph_increase = get_benefit( "Smites increased by Holy Fire" );
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
      if ( specialization() == PRIEST_DISCIPLINE || specialization() == PRIEST_HOLY )
        return ROLE_HEAL;
      break;
  }

  return ROLE_SPELL;
}

// priest_t::convert_hybrid_stat ==============================================

stat_e priest_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  // This is all a guess at how the hybrid primaries will work, since they
  // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_AGI_INT: 
    return STAT_INTELLECT; 
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;     
  default: return s; 
  }
}

// priest_t::combat_begin ===================================================

void priest_t::combat_begin()
{
  base_t::combat_begin();

  resources.current[ RESOURCE_SHADOW_ORB ] = clamp( as<double>( options.initial_shadow_orbs ), 0.0, resources.base[ RESOURCE_SHADOW_ORB ] );
}

// priest_t::composite_armor ================================================

double priest_t::composite_armor() const
{
  double a = base_t::composite_armor();

  if ( buffs.shadowform -> check() )
    a *= 1.0 + buffs.shadowform -> data().effectN( 3 ).percent();

  return std::floor( a );
}

// priest_t::composite_spell_haste ==========================================

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.power_infusion -> check() )
    h /= 1.0 + buffs.power_infusion -> data().effectN( 1 ).percent();

  if ( buffs.resolute_spirit -> check() )
    h /= 1.0 + buffs.resolute_spirit -> data().effectN( 1 ).percent(); // FIXME: check whether to use set bonus data ( 10% ) or buff data ( 15% ). 2013/06/13

  return h;
}

// priest_t::composite_spell_speed ==========================================

double priest_t::composite_spell_speed() const
{
  double h = player_t::composite_spell_speed();

  if ( buffs.borrowed_time -> check() )
    h /= 1.0 + buffs.borrowed_time -> data().effectN( 1 ).percent();

  return h;
}

// priest_t::composite_spell_power_multiplier ===============================

double priest_t::composite_spell_power_multiplier() const
{
  double m = 1.0;

  if ( sim -> auras.spell_power_multiplier -> check() )
    m += sim -> auras.spell_power_multiplier -> current_value;

  m *= current.spell_power_multiplier;

  return m;
}

double priest_t::composite_spell_crit() const
{
  double csc = base_t::composite_spell_crit();

  if ( buffs.archangel -> check() )
    csc *= 1.0 + sets.set( SET_T16_2PC_HEAL ) -> effectN( 2 ).percent();

  return csc;
}

double priest_t::composite_melee_crit() const
{
  double cmc = base_t::composite_melee_crit();

  if ( buffs.archangel -> check() )
    cmc *= 1.0 + sets.set( SET_T16_2PC_HEAL ) -> effectN( 2 ).percent();

  return cmc;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( school_e school ) const
{
  double m = base_t::composite_player_multiplier( school );

  if ( specs.shadowform -> ok() && dbc::is_school( SCHOOL_SHADOW, school ) )
  {
    m *= 1.0 + buffs.shadowform -> check() * specs.shadowform -> effectN( 2 ).percent();
  }

  if ( dbc::is_school( SCHOOL_SHADOWLIGHT, school ) )
  {
    if ( buffs.chakra_chastise -> check() )
    {
      m *= 1.0 + buffs.chakra_chastise -> data().effectN( 1 ).percent();
    }
  }

  if ( buffs.power_infusion -> check() )
  {
    m *= 1.0 + buffs.power_infusion -> data().effectN( 3 ).percent();
  }

  if ( buffs.twist_of_fate -> check() )
  {
    m *= 1.0 + buffs.twist_of_fate -> current_value;
  }

  return m;
}

// priest_t::composite_player_heal_multiplier ===============================

double priest_t::composite_player_heal_multiplier( school_e s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buffs.twist_of_fate -> check() )
  {
    m *= 1.0 + buffs.twist_of_fate -> current_value;
  }

  if ( buffs.saving_grace_penalty -> check() )
  {
    m *= 1.0 + buffs.saving_grace_penalty -> check() * -0.10; // TODO: spelldata
  }

  return m;
}

// priest_t::temporary_movement_modifier =======================================

double priest_t::temporary_movement_modifier() const
{
  double speed = player_t::temporary_movement_modifier();

  if ( glyphs.free_action -> ok() && buffs.dispersion -> check() ) {
    speed = std::max( speed, glyphs.free_action -> effectN( 1 ).percent() );
  }

  return speed;
}

// priest_t::composite_attribute_multiplier =================================

double priest_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = base_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_INTELLECT )
    m *= 1.0 + specs.mysticism -> effectN( 1 ).percent();

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
  if ( name == "archangel"              ) return new archangel_t             ( *this, options_str );
  if ( name == "chakra_chastise"        ) return new chakra_chastise_t       ( *this, options_str );
  if ( name == "chakra_sanctuary"       ) return new chakra_sanctuary_t      ( *this, options_str );
  if ( name == "chakra_serenity"        ) return new chakra_serenity_t       ( *this, options_str );
  if ( name == "dispersion"             ) return new dispersion_t            ( *this, options_str );
  if ( name == "power_word_fortitude"   ) return new fortitude_t             ( *this, options_str );
  if ( name == "inner_focus"            ) return new inner_focus_t           ( *this, options_str );
  if ( name == "pain_suppression"       ) return new pain_suppression_t      ( *this, options_str );
  if ( name == "power_infusion"         ) return new power_infusion_t        ( *this, options_str );
  if ( name == "shadowform"             ) return new shadowform_t            ( *this, options_str );
  if ( name == "vampiric_embrace"       ) return new vampiric_embrace_t      ( *this, options_str );
  if ( name == "spirit_shell"           ) return new spirit_shell_t          ( *this, options_str );

  // Damage
  if ( name == "devouring_plague"       ) return new devouring_plague_t      ( *this, options_str );
  if ( name == "mind_blast"             ) return new mind_blast_t            ( *this, options_str );
  if ( name == "mind_flay"              ) return new mind_flay_base_t<>      ( *this, options_str );
  if ( name == "mind_flay_insanity"     ) return new mind_flay_insanity_t    ( *this, options_str );
  if ( name == "mind_spike"             ) return new mind_spike_t            ( *this, options_str );
  if ( name == "mind_sear"              ) return new mind_sear_t             ( *this, options_str );
  if ( name == "penance"                ) return new penance_t               ( *this, options_str );
  if ( name == "shadow_word_death"      ) return new shadow_word_death_t     ( *this, options_str );
  if ( name == "shadow_word_pain"       ) return new shadow_word_pain_t      ( *this, options_str );
  if ( name == "smite"                  ) return new smite_t                 ( *this, options_str );
  if ( ( name == "shadowfiend"          ) || ( name == "mindbender" ) )
  {
    if ( talents.mindbender -> ok() )
      return new summon_mindbender_t    ( *this, options_str );
    else
      return new summon_shadowfiend_t   ( *this, options_str );
  }
  if ( ( name == "holy_fire" ) || ( name == "power_word_solace" ) )
  {
    if ( talents.solace_and_insanity -> ok() )
      return new power_word_solace_t ( *this, options_str );
    else
      return new holy_fire_t  ( *this, options_str );
  }
  if ( name == "vampiric_touch"         ) return new vampiric_touch_t        ( *this, options_str );
  if ( name == "cascade_damage"         ) return new cascade_damage_t        ( *this, options_str );
  if ( name == "halo"                   ) return new halo_t                  ( *this, options_str );
  if ( name == "divine_star"            ) return new divine_star_t           ( *this, options_str );
  if ( name == "void_entropy"           ) return new void_entropy_t          ( *this, options_str );

  // Heals
  if ( name == "binding_heal"           ) return new binding_heal_t          ( *this, options_str );
  if ( name == "circle_of_healing"      ) return new circle_of_healing_t     ( *this, options_str );
  if ( name == "divine_hymn"            ) return new divine_hymn_t           ( *this, options_str );
  if ( name == "flash_heal"             ) return new flash_heal_t            ( *this, options_str );
  if ( name == "greater_heal"           ) return new greater_heal_t          ( *this, options_str );
  if ( name == "guardian_spirit"        ) return new guardian_spirit_t       ( *this, options_str );
  if ( name == "holy_word"              ) return new holy_word_t             ( *this, options_str );
  if ( name == "penance_heal"           ) return new penance_heal_t          ( *this, options_str );
  if ( name == "power_word_shield"      ) return new power_word_shield_t     ( *this, options_str );
  if ( name == "prayer_of_healing"      ) return new prayer_of_healing_t     ( *this, options_str );
  if ( name == "prayer_of_mending"      ) return new prayer_of_mending_t     ( *this, options_str );
  if ( name == "renew"                  ) return new renew_t                 ( *this, options_str );
  if ( name == "cascade_heal"           ) return new cascade_heal_t          ( *this, options_str );
  if ( name == "clarity_of_will"        ) return new clarity_of_will_t       ( *this, options_str );
  if ( name == "clarity_of_purpose"     ) return new clarity_of_purpose_t    ( *this, options_str );
  if ( name == "saving_grace"           ) return new saving_grace_t          ( *this, options_str );

  if ( find_class_spell( "Lightwell" ) -> ok() )
    if ( name == "lightwell"              ) return new lightwell_t             ( *this, options_str );

  return base_t::create_action( name, options_str );
}

// priest_t::create_pet =====================================================

pet_t* priest_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "shadowfiend" ) return new pets::shadowfiend_pet_t( sim, *this );
  if ( pet_name == "mindbender"  ) return new pets::mindbender_pet_t ( sim, *this );
  if ( pet_name == "lightwell"   ) return new pets::lightwell_pet_t  ( sim, *this );

  sim -> errorf( "Tried to create priest pet %s.", pet_name.c_str() );

  return nullptr;
}

// priest_t::create_pets ====================================================

void priest_t::create_pets()
{
  base_t::create_pets();

  pets.shadowfiend      = create_pet( "shadowfiend" );
  pets.mindbender       = create_pet( "mindbender"  );

  if ( find_class_spell( "Lightwell" ) -> ok() )
    pets.lightwell        = create_pet( "lightwell"   );
}

// priest_t::init_base ======================================================

void priest_t::init_base_stats()
{
  base_t::init_base_stats();

  base.stats.attack_power = 0.0;

  if ( specs.shadow_orbs -> ok() )
  {
    resources.base[ RESOURCE_SHADOW_ORB ] = 3.0;

    resources.base[ RESOURCE_SHADOW_ORB ] += perks.enhanced_shadow_orbs -> effectN( 1 ).base_value();
  }

  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Discipline/Holy
  base.mana_regen_from_spirit_multiplier = specs.meditation_disc -> ok() ?
      specs.meditation_disc -> effectN( 1 ).percent() :
      specs.meditation_holy -> effectN( 1 ).percent();

  diminished_kfactor   = 0.009830;
  diminished_dodge_cap = 0.006650;
  diminished_parry_cap = 0.006650;
}

// priest_t::init_scaling ===================================================

void priest_t::init_scaling()
{
  base_t::init_scaling();

  // Atonement heals are capped at a percentage of the Priest's health,
  // so there may be scaling with stamina.
  if ( specs.atonement -> ok() && primary_role() == ROLE_HEAL )
    scales_with[ STAT_STAMINA ] = true;

}

// priest_t::init_spells ====================================================

void priest_t::init_spells()
{
  base_t::init_spells();

  // Talents
  talents.void_tendrils               = find_talent_spell( "Void Tendrils" );
  talents.psychic_scream               = find_talent_spell( "Psychic Scream" );
  talents.dominate_mind               = find_talent_spell( "Dominate Mind" );

  talents.body_and_soul               = find_talent_spell( "Body and Soul" );
  talents.angelic_feather             = find_talent_spell( "Angelic Feather" );
  talents.phantasm                    = find_talent_spell( "Phantasm" );

  talents.from_darkness_comes_light   = find_talent_spell( "From Darkness, Comes Light" );
  talents.mindbender                  = find_talent_spell( "Mindbender" );
  talents.solace_and_insanity         = find_talent_spell( "Solace and Insanity" );

  talents.desperate_prayer            = find_talent_spell( "Desperate Prayer" );
  talents.spectral_guise              = find_talent_spell( "Spectral Guise" );
  talents.angelic_bulwark             = find_talent_spell( "Angelic Bulwark" );

  talents.twist_of_fate               = find_talent_spell( "Twist of Fate" );
  talents.power_infusion              = find_talent_spell( "Power Infusion" );
  talents.divine_insight              = find_talent_spell( "Divine Insight" );

  talents.cascade                     = find_talent_spell( "Cascade" );
  talents.divine_star                 = find_talent_spell( "Divine Star" );
  talents.halo                        = find_talent_spell( "Halo" );

  talents.clarity_of_power            = find_talent_spell( "Clarity of Power" );
  talents.clarity_of_purpose          = find_talent_spell( "Clarity of Purpose" );
  talents.clarity_of_will             = find_talent_spell( "Clarity of Will" );
  talents.void_entropy                = find_talent_spell( "Void Entropy" );
  talents.auspicious_spirits          = find_talent_spell( "Auspicious Spirits" );
  talents.saving_grace                = find_talent_spell( "Saving Grace" );
  talents.words_of_mending            = find_talent_spell( "Words of Mending" );
  //std::cerr << talents.auspicious_spirits -> id() << "\n\n";


  // General Spells

  // Discipline
  specs.atonement                      = find_specialization_spell( "Atonement" );
  specs.archangel                      = find_specialization_spell( "Archangel" );
  specs.borrowed_time                  = find_specialization_spell( "Borrowed Time" );
  specs.divine_aegis                   = find_specialization_spell( "Divine Aegis" );
  specs.evangelism                     = find_specialization_spell( "Evangelism" );
  specs.grace                          = find_specialization_spell( "Grace" );
  specs.meditation_disc                = find_specialization_spell( "Meditation", "meditation_disc", PRIEST_DISCIPLINE );
  specs.mysticism                      = find_specialization_spell( "Mysticism" );
  specs.spirit_shell                   = find_specialization_spell( "Spirit Shell" );
  specs.strength_of_soul               = find_specialization_spell( "Strength of Soul" );
  specs.train_of_thought               = find_specialization_spell( "Train of Thought" );

  // Holy
  specs.meditation_holy                = find_specialization_spell( "Meditation", "meditation_holy", PRIEST_HOLY );
  specs.serendipity                    = find_specialization_spell( "Serendipity" );
  specs.rapid_renewal                  = find_specialization_spell( "Rapid Renewal" );

  // Shadow
  specs.mind_surge                     = find_specialization_spell( "Mind Surge" );
  specs.shadowform                     = find_class_spell( "Shadowform" );
  specs.shadowy_apparitions            = find_specialization_spell( "Shadowy Apparitions" );
  specs.shadow_orbs                    = find_specialization_spell( "Shadow Orbs" );

  // Mastery Spells
  mastery_spells.shield_discipline    = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light        = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.mental_anguish       = find_mastery_spell( PRIEST_SHADOW );

  // Perk Spells
  perks.enhanced_mind_flay            = find_perk_spell( "Enhanced Mind Flay" );
  perks.enhanced_shadow_orbs          = find_perk_spell( "Enhanced Shadow Orbs" );
  perks.enhanced_shadow_word_death    = find_perk_spell( "Enhanced Shadow Word: Death" );
  perks.improved_shadow_word_pain     = find_perk_spell( "Improved Shadow Word: Pain" );
  perks.improved_vampiric_touch       = find_perk_spell( "Improved Vampiric Touch" );

  // Glyphs
  glyphs.circle_of_healing            = find_glyph_spell( "Glyph of Circle of Healing" );
  glyphs.dispersion                   = find_glyph_spell( "Glyph of Dispersion" );
  glyphs.holy_nova                    = find_glyph_spell( "Glyph of Holy Nova" );
  glyphs.lightwell                    = find_glyph_spell( "Glyph of Lightwell" );
  glyphs.penance                      = find_glyph_spell( "Glyph of Penance" );
  glyphs.power_word_shield            = find_glyph_spell( "Glyph of Power Word: Shield" );
  glyphs.prayer_of_mending            = find_glyph_spell( "Glyph of Prayer of Mending" );
  glyphs.renew                        = find_glyph_spell( "Glyph of Renew" );
  glyphs.smite                        = find_glyph_spell( "Glyph of Smite" );
  glyphs.holy_fire                    = find_glyph_spell( "Glyph of Holy Fire" );
  glyphs.dark_binding                 = find_glyph_spell( "Glyph of Dark Binding" );
  glyphs.mind_spike                   = find_glyph_spell( "Glyph of Mind Spike" );
  glyphs.mind_flay                    = find_glyph_spell( "Glyph of Mind Flay" );
  glyphs.mind_blast                   = find_glyph_spell( "Glyph of Mind Blast" );
  glyphs.devouring_plague             = find_glyph_spell( "Glyph of Devouring Plague" );
  glyphs.vampiric_embrace             = find_glyph_spell( "Glyph of Vampiric Embrace" );
  glyphs.borrowed_time                = find_glyph_spell( "Glyph of Borrowed Time" );
  glyphs.shadow_word_death            = find_glyph_spell( "Glyph of Shadow Word: Death" );
  // WoD
  glyphs.free_action                  = find_glyph_spell( "Glyph of Free Action" );

  if ( mastery_spells.echo_of_light -> ok() )
    active_spells.echo_of_light = new actions::heals::echo_of_light_t( *this );

  active_spells.surge_of_darkness = talents.from_darkness_comes_light -> ok() ? find_spell( 87160 ) : spell_data_t::not_found();

  // Range Based on Talents
  if (talents.cascade -> ok())
    base.distance = 30.0;
  else if (talents.divine_star -> ok())
    base.distance = 24.0;
  else if (talents.halo -> ok())
    base.distance = 27.0;
  else
    base.distance = 27.0;

  // Set Bonuses
  static const set_bonus_description_t set_bonuses =
  {
    //   C2P     C4P    M2P    M4P    T2P    T4P     H2P     H4P
    { 105843, 105844,     0,     0,     0,     0, 105827, 105832 }, // Tier13
    { 123114, 123115,     0,     0,     0,     0, 123111, 123113 }, // Tier14
    { 138156, 138158,     0,     0,     0,     0, 138293, 138301 }, // Tier15
    { 145174, 145179,     0,     0,     0,     0, 145306, 145334 }, // Tier16
  };
  sets.register_spelldata( set_bonuses );
}

// priest_t::init_buffs =====================================================

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Talents
  buffs.power_infusion = buff_creator_t( this, "power_infusion" )
                         .spell( talents.power_infusion )
                         .add_invalidate( CACHE_SPELL_HASTE )
                         .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.twist_of_fate = buff_creator_t( this, "twist_of_fate" )
                        .spell( talents.twist_of_fate )
                        .duration( talents.twist_of_fate -> effectN( 1 ).trigger() -> duration() )
                        .default_value( talents.twist_of_fate -> effectN( 1 ).trigger() -> effectN( 2 ).percent() )
                        .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                        .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buffs.surge_of_light = buff_creator_t( this, "surge_of_light" )
                         .spell( find_spell( 114255 ) )
                         .chance( talents.from_darkness_comes_light -> effectN( 1 ).percent() );

  // Discipline
  buffs.archangel = new buffs::archangel_t( *this );

  buffs.borrowed_time = buff_creator_t( this, "borrowed_time" )
                        .spell( find_spell( 59889 ) )
                        .chance( specs.borrowed_time -> ok() )
                        .default_value( find_spell( 59889 ) -> effectN( 1 ).percent() )
                        .add_invalidate( CACHE_SPELL_HASTE );

  buffs.holy_evangelism = buff_creator_t( this, "holy_evangelism" )
                          .spell( find_spell( 81661 ) )
                          .chance( specs.evangelism -> ok() )
                          .activated( false );

  buffs.inner_focus = buff_creator_t( this, "inner_focus" )
                      .spell( find_class_spell( "Inner Focus" ) )
                      .cd( timespan_t::zero() );

  buffs.spirit_shell = new buffs::spirit_shell_t( *this );

  buffs.saving_grace_penalty = buff_creator_t( this, "saving_grace_penalty" )
                            .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER ); // TODO: spelldata

  // Holy
  buffs.chakra_chastise = buff_creator_t( this, "chakra_chastise" )
                          .spell( find_spell( 81209 ) )
                          .chance( specialization() == PRIEST_HOLY ? 1.0 : 0.0 )
                          .cd( timespan_t::zero() )
                          .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buffs.chakra_sanctuary = buff_creator_t( this, "chakra_sanctuary" )
                           .spell( find_spell( 81206 ) )
                           .chance( specialization() == PRIEST_HOLY ? 1.0 : 0.0 )
                           .cd( timespan_t::zero() );

  buffs.chakra_serenity = buff_creator_t( this, "chakra_serenity" )
                          .spell( find_spell( 81208 ) )
                          .chance( specialization() == PRIEST_HOLY ? 1.0 : 0.0 )
                          .cd( timespan_t::zero() );

  buffs.serendipity = buff_creator_t( this, "serendipity" )
                      .spell( find_spell( specs.serendipity -> effectN( 1 ).trigger_spell_id( ) ) );

  // Shadow
  buffs.divine_insight_shadow = new buffs::divine_insight_shadow_t( *this );

  buffs.shadowform = new buffs::shadowform_t( *this );

  buffs.vampiric_embrace = buff_creator_t( this, "vampiric_embrace", find_class_spell( "Vampiric Embrace" ) )
                           .duration( find_class_spell( "Vampiric Embrace" ) -> duration() + glyphs.vampiric_embrace -> effectN( 1 ).time_value() );

  buffs.glyph_mind_spike = buff_creator_t( this, "glyph_mind_spike" )
                           .spell( glyphs.mind_spike -> effectN( 1 ).trigger() )
                           .chance( glyphs.mind_spike -> proc_chance() );

  buffs.shadow_word_death_reset_cooldown = buff_creator_t( this, "shadow_word_death_reset_cooldown" )
                                              .max_stack( 1 )
                                              .duration( timespan_t::from_seconds( 9.0 ) ); // data in the old deprecated glyph. Leave hardcoded for now, 3/12/2012; 9.0sec ICD in 5.4 (2013/08/18)

  buffs.empowered_shadows = buff_creator_t( this, "empowered_shadows" )
                            .spell( sets.set( SET_T16_4PC_CASTER ) -> effectN( 1 ).trigger() )
                            .chance( sets.has_set_bonus( SET_T16_4PC_CASTER ) ? 1.0 : 0.0 );

  buffs.absolution = buff_creator_t( this, "absolution" )
                      .spell( find_spell( 145336 ) )
                      .chance( sets.has_set_bonus( SET_T16_4PC_HEAL ) ? 1.0 : 0.0 );

  buffs.resolute_spirit = stat_buff_creator_t( this, "resolute_spirit" )
                          .spell( find_spell( 145374 ) )
                          .chance( sets.has_set_bonus( SET_T16_4PC_HEAL ) ? 1.0 : 0.0 )
                          .add_invalidate( CACHE_HASTE );

  buffs.surge_of_darkness                = buff_creator_t( this, "surge_of_darkness", active_spells.surge_of_darkness )
                                            .chance( active_spells.surge_of_darkness -> ok() ? 0.20 : 0.0 ); // Updated 5.4 PTR value, 6/20/2013

  buffs.dispersion = buff_creator_t( this, "dispersion" ).spell( find_class_spell( "Dispersion" ) );
}

// ALL Spec Pre-Combat Action Priority List

void priest_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    flask_action += ( level > 85 ) ? "warm_sun" : "draconic_mind";
    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    food_action += ( level > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  precombat -> add_action( this, "Power Word: Fortitude", "if=!aura.stamina.up" );
  precombat -> add_action( this, "Shadowform" );
  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && level >= 80 )
    precombat -> add_action( ( level > 85 ) ? "jade_serpent_potion" : "volcanic_potion" );
}

// NO Spec Combat Action Priority List

void priest_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // DEFAULT
  if ( sim -> allow_potions )
    def -> add_action( "mana_potion,if=mana.pct<=75" );

  if ( find_class_spell( "Shadowfiend" ) -> ok() )
  {
    def -> add_action( this, "Shadowfiend", ",if=mana.pct<50" );
  }
  if ( race == RACE_TROLL )  def -> add_action( "berserking" );
  if ( race == RACE_BLOOD_ELF ) def -> add_action( "arcane_torrent,if=mana.pct<=90" );
  def -> add_action( this, "Holy Fire" );
  def -> add_action( this, "Shadow Word: Pain", ",if=remains<tick_time|!ticking" );
  def -> add_action( this, "Smite" );
}

// Shadow Combat Action Priority List

void priest_t::apl_shadow()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( this, "Shadowform" );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def -> add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // Potions
  if ( sim -> allow_potions && level >= 80 )
  {
    if ( level > 85 )
      def -> add_action( "jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=40" );
    else
      def -> add_action( "volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40" );
  }

  def -> add_action( "mindbender,if=talent.mindbender.enabled" );
  def -> add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  def -> add_action( "power_infusion,if=talent.power_infusion.enabled" );

  // Racials
  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] );

  def -> add_talent( this, "Void Entropy", "if=shadow_orb=3&miss_react&!ticking" );
  def -> add_action( "shadow_word_death,if=buff.shadow_word_death_reset_cooldown.stack=1&active_enemies<=5" );
  def -> add_action( this, "Devouring Plague", "if=shadow_orb=3&(cooldown.mind_blast.remains<1.5|target.health.pct<20&cooldown.shadow_word_death.remains<1.5)" );
  if ( find_item( "unerring_vision_of_lei_shen" ) )
  {
    def -> add_action( this, "Shadow Word: Pain", "cycle_targets=1,max_cycle_targets=8,if=miss_react&buff.perfect_aim.react&crit_pct<100" );
  }
  def -> add_action( this, "Mind Blast", "if=active_enemies<=5&cooldown_react" );
  def -> add_action( "shadow_word_death,if=buff.shadow_word_death_reset_cooldown.stack=0&active_enemies<=5" );
  def -> add_action( "mind_flay_insanity,if=target.dot.devouring_plague_tick.ticks_remain=1,chain=1" );
  def -> add_action( "mind_flay_insanity,interrupt=1,chain=1,if=active_enemies<=5" );
  def -> add_action( this, "Shadow Word: Pain", "cycle_targets=1,max_cycle_targets=5,if=miss_react&!ticking" );
  def -> add_action( this, "Vampiric Touch", "cycle_targets=1,max_cycle_targets=5,if=remains<cast_time&miss_react" );
  def -> add_action( this, "Shadow Word: Pain", "cycle_targets=1,max_cycle_targets=5,if=miss_react&ticks_remain<=1" );
  def -> add_action( this, "Vampiric Touch", "cycle_targets=1,max_cycle_targets=5,if=remains<cast_time+tick_time&miss_react" );
  def -> add_action( this, "Vampiric Embrace", "if=shadow_orb=3&health.pct<=40" );
  def -> add_action( this, "Devouring Plague", "if=shadow_orb=3&ticks_remain<=1" );
  def -> add_action( this, "Mind Spike", "if=active_enemies<=5&buff.surge_of_darkness.react=2" );
  def -> add_action( "halo,if=talent.halo.enabled&target.distance<=30&target.distance>=17" ); //When coefficients change, update minimum distance!
  def -> add_action( "cascade_damage,if=talent.cascade.enabled&(active_enemies>1|target.distance>=28)&target.distance<=40&target.distance>=11" ); //When coefficients change, update minimum distance!
  def -> add_action( "divine_star,if=talent.divine_star.enabled&(active_enemies>1|target.distance<=24)" );
  def -> add_action( "wait,sec=cooldown.shadow_word_death.remains,if=target.health.pct<20&cooldown.shadow_word_death.remains&cooldown.shadow_word_death.remains<0.5&active_enemies<=1" );
  def -> add_action( "wait,sec=cooldown.mind_blast.remains,if=cooldown.mind_blast.remains<0.5&cooldown.mind_blast.remains&active_enemies<=1" );
  def -> add_action( "mind_spike,if=buff.surge_of_darkness.react&active_enemies<=5" );
  def -> add_action( "divine_star,if=talent.divine_star.enabled&target.distance<=28&active_enemies>1" );
  def -> add_action( this, "Mind Sear", "chain=1,interrupt=1,if=active_enemies>=3" );
  def -> add_action( this, "Mind Flay", "chain=1,interrupt=1" );
  def -> add_action( this, "Shadow Word: Death", "moving=1" );
  def -> add_action( this, "Mind Blast", "moving=1,if=buff.divine_insight_shadow.react&cooldown_react" );
  def -> add_action( "divine_star,moving=1,if=talent.divine_star.enabled&target.distance<=28" );
  def -> add_action( "cascade_damage,moving=1,if=talent.cascade.enabled&target.distance<=40" );
  def -> add_action( this, "Shadow Word: Pain", "moving=1" );
  def -> add_action( this, "Dispersion" );

}

// Discipline Heal Combat Action Priority List

void priest_t::apl_disc_heal()
{
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def -> add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // Potions
  if ( sim -> allow_potions )
  {
    std::string a = "mana_potion,if=mana.pct<=75";
    def -> add_action( a );
  }

  if ( race == RACE_BLOOD_ELF )
    def -> add_action( "arcane_torrent,if=mana.pct<95" );

  if ( find_class_spell( "Shadowfiend" ) -> ok() )
  {
    def -> add_action( "mindbender,if=talent.mindbender.enabled" );

    std::string a = "shadowfiend,if=!talent.mindbender.enabled&";
    a += "mana.pct<70";
    def -> add_action( a );
  }

  if ( race != RACE_BLOOD_ELF )
  {
    std::vector<std::string> racial_actions = get_racial_actions();
    for ( size_t i = 0; i < racial_actions.size(); i++ )
      def -> add_action( racial_actions[ i ] );
  }

  def -> add_action( this, "Inner Focus" );
  def -> add_action( "power_infusion,if=talent.power_infusion.enabled" );
  def -> add_action( this, "Power Word: Shield" );
  def -> add_action( this, "Renew", "if=buff.borrowed_time.up&(!ticking|remains<tick_time)" );
  def -> add_action( "penance_heal,if=buff.borrowed_time.up|target.buff.grace.stack<3" );
  def -> add_action( this, "Greater Heal", "if=buff.inner_focus.up" );
  def -> add_action( "penance_heal" );
  def -> add_action( this, "Flash Heal", "if=buff.surge_of_light.react" );
  def -> add_action( this, "Greater Heal", "if=buff.power_infusion.up|mana.pct>20" );
  def -> add_action( "power_word_solace,if=talent.power_word_solace.enabled" );
  // DEFAULT END
}

// Discipline Damage Combat Action Priority List

void priest_t::apl_disc_dmg()
{
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def -> add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // Potions
  if ( sim -> allow_potions && level >= 80 )
  {
    if ( level > 85 )
      def -> add_action( "jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=40" );
    else
      def -> add_action( "volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40" );
  }

  if ( race == RACE_BLOOD_ELF )
    def -> add_action( "arcane_torrent,if=mana.pct<=95" );

  if ( find_class_spell( "Shadowfiend" ) -> ok() )
  {
    def -> add_action( "mindbender,if=talent.mindbender.enabled" );
    def -> add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  }

  if ( race != RACE_BLOOD_ELF )
  {
    std::vector<std::string> racial_actions = get_racial_actions();
    for ( size_t i = 0; i < racial_actions.size(); i++ )
      def -> add_action( racial_actions[ i ] );
  }

  def -> add_action( "power_infusion,if=talent.power_infusion.enabled" );

  def -> add_action( this, "Archangel", "if=buff.holy_evangelism.react=5" );
  def -> add_action( this, "Penance" );
  def -> add_action( this, "Shadow Word: Death" );

  if ( find_class_spell( "Holy Fire" ) -> ok() )
  {
    if ( talents.solace_and_insanity -> ok() ) // This is just for a nice-looking action list. Both strings resolve to the correct current action
      def -> add_action( "power_word_solace" );
    else
      def -> add_action( "holy_fire" );
  }

  def -> add_action( "halo,if=talent.halo.enabled&active_enemies>3" );
  def -> add_action( "divine_star,if=talent.divine_star.enabled&active_enemies>2" );
  def -> add_action( "cascade_damage,if=talent.cascade.enabled&active_enemies>3" );
  def -> add_action( this, "Smite", "if=glyph.smite.enabled&dot.power_word_solace.remains>cast_time" );
  def -> add_action( this, "Smite", "if=!talent.twist_of_fate.enabled&mana.pct>15" );
  def -> add_action( this, "Smite", "if=talent.twist_of_fate.enabled&target.health.pct<35&mana.pct>target.health.pct" );
}

// Holy Heal Combat Action Priority List

void priest_t::apl_holy_heal()
{
  action_priority_list_t* def       = get_action_priority_list( "default"   );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def -> add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // Potions
  if ( sim -> allow_potions )
    def -> add_action( "mana_potion,if=mana.pct<=75" );

  if ( find_class_spell( "Shadowfiend" ) -> ok() )
  {
    def -> add_action( "mindbender,if=talent.mindbender.enabled" );
    def -> add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  }

  std::string fiend_cond = ",if=mana.pct<30";
  if ( find_class_spell( "Shadowfiend" ) -> ok() )
    fiend_cond = ",if=(pet.mindbender.active|pet.shadowfiend.active)&mana.pct<=20";


  std::string racial_condition;
  if ( race == RACE_BLOOD_ELF )
    racial_condition = ",if=mana.pct<=90";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ], racial_condition );

  def -> add_action( this, "Chakra: Serenity" );
  def -> add_action( this, "Renew", ",if=!ticking" );
  def -> add_action( this, "Holy Word", ",if=buff.chakra_serenity.up" );
  def -> add_action( this, "Greater Heal", ",if=buff.serendipity.react>=2&mana.pct>40" );
  def -> add_action( this, "Flash Heal", ",if=buff.surge_of_light.up" );
}

// Holy Damage Combat Action Priority List

void priest_t::apl_holy_dmg()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // On-Use Items
  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def -> add_action( item_actions[ i ] );

  // Professions
  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    def -> add_action( profession_actions[ i ] );

  // Potions
  if ( sim -> allow_potions )
  {
    if ( level > 85 )
      def -> add_action( "jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=40" );
    else if ( level >= 80 )
      def -> add_action( "volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40" );
  }

  // Racials
  std::string racial_condition;
  if ( race == RACE_BLOOD_ELF )
    racial_condition = ",if=mana.pct<=90";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ], racial_condition );


  if ( find_class_spell( "Shadowfiend" ) -> ok() )
  {
    def -> add_action( "mindbender,if=talent.mindbender.enabled" );
    def -> add_action( "shadowfiend,if=!talent.mindbender.enabled" );
  }

  def -> add_action( this, "Chakra: Chastise", ",if=buff.chakra_chastise.down" );

  if ( find_specialization_spell( "Holy Word: Chastise" ) -> ok() )
    def -> add_action( "holy_word" );

  if ( find_class_spell( "Holy Fire" ) -> ok() )
  {
    if ( talents.solace_and_insanity -> ok() ) // This is just for a nice-looking action list. Both strings resolve to the correct current action
      def -> add_action( "power_word_solace" );
    else
      def -> add_action( "holy_fire" );
  }

  def -> add_action( this, "Shadow Word: Pain", "if=remains<tick_time|!ticking" );
  def -> add_action( this, "Smite" );
}

/* Always returns non-null targetdata pointer
 */
priest_td_t* priest_t::get_target_data( player_t* target ) const
{
  priest_td_t*& td = target_data[ target ];
  if ( ! td )
  {
    td = new priest_td_t( target, const_cast<priest_t&>(*this) );
  }
  return td;
}

// priest_t::init_actions ===================================================

void priest_t::init_action_list()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat(); // PRE-COMBAT

  switch ( specialization() )
  {
    case PRIEST_SHADOW:
      apl_shadow(); // SHADOW
      break;
    case PRIEST_DISCIPLINE:
      if ( primary_role() != ROLE_HEAL )
        apl_disc_dmg();  // DISCIPLINE DAMAGE
      else
        apl_disc_heal(); // DISCIPLINE HEAL
      break;
    case PRIEST_HOLY:
      if ( primary_role() != ROLE_HEAL )
        apl_holy_dmg();  // HOLY DAMAGE
      else
        apl_holy_heal(); // HOLY HEAL
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  base_t::reset();
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
      atonement -> resource_gain.merge( trigger -> resource_gain );
      atonement -> total_execute_time = trigger -> total_execute_time;
      atonement -> total_tick_time = trigger -> total_tick_time;
      atonement -> num_ticks = trigger -> num_ticks;
    }
  }
}

/* Fixup Atonement Stats HPE, HPET and HPR
 */

void priest_t::pre_analyze_hook()
{
  base_t::pre_analyze_hook();

  if ( specs.atonement -> ok() )
  {
    fixup_atonement_stats( "smite", "atonement_smite" );
    fixup_atonement_stats( "holy_fire", "atonement_holy_fire" );
    fixup_atonement_stats( "penance", "atonement_penance" );
  }
}

// priest_t::target_mitigation ==============================================

void priest_t::target_mitigation( school_e school,
                                  dmg_e    dt,
                                  action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( buffs.dispersion -> check() )
  {
    s -> result_amount *= 1.0 + buffs.dispersion -> data().effectN( 1 ).percent();
  }
}

// priest_t::create_options =================================================

void priest_t::create_options()
{
  base_t::create_options();

  option_t priest_options[] =
  {
    opt_string( "atonement_target", options.atonement_target_str ),
    opt_deprecated( "double_dot", "action_list=double_dot" ),
    opt_int( "initial_shadow_orbs", options.initial_shadow_orbs ),
    opt_bool( "autounshift", options.autoUnshift ),
    opt_null()
  };

  option_t::copy( base_t::options, priest_options );
}

// priest_t::create_profile =================================================

bool priest_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  base_t::create_profile( profile_str, type, save_html );

  if ( type == SAVE_ALL )
  {
    if ( ! options.autoUnshift )
      profile_str += "autounshift=" + util::to_string( options.autoUnshift ) + "\n";

    if ( ! options.atonement_target_str.empty() )
      profile_str += "atonement_target=" + options.atonement_target_str + "\n";

    if ( options.initial_shadow_orbs != 0 )
    {
      profile_str += "initial_shadow_orbs=";
      profile_str += util::to_string( options.initial_shadow_orbs );
      profile_str += "\n";
    }
  }

  return true;
}

// priest_t::copy_from ======================================================

void priest_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  priest_t* source_p = debug_cast<priest_t*>( source );

  options = source_p -> options;
}

// priest_t::decode_set =====================================================

set_e priest_t::decode_set( const item_t& item ) const
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const std::string s = item.name();

  // Tier 13
  if ( s.find( "_of_dying_light" ) != std::string::npos )
  {
    if ( s.find( "hood"          ) != std::string::npos ||
         s.find( "shoulderwraps" ) != std::string::npos ||
         s.find( "vestment"      ) != std::string::npos ||
         s.find( "gloves"        ) != std::string::npos ||
         s.find( "leggings"      ) != std::string::npos ||
         ( item.slot == SLOT_SHOULDERS && item.has_item_stat( STAT_CRIT_RATING ) ) ||
         ( item.slot == SLOT_CHEST     && item.has_item_stat( STAT_HASTE_RATING ) ) )
    {
      return SET_T13_CASTER;
    }

    if ( s.find( "cowl"          ) != std::string::npos ||
         s.find( "mantle"        ) != std::string::npos ||
         s.find( "robes"         ) != std::string::npos ||
         s.find( "handwraps"     ) != std::string::npos ||
         s.find( "legwraps"      ) != std::string::npos  )
    {
      return SET_T13_HEAL;
    }
  }

  // Tier 14
  if ( s.find( "guardian_serpent" ) != std::string::npos  )
  {
    if ( s.find( "hood"           ) != std::string::npos ||
         s.find( "shoulderguards" ) != std::string::npos ||
         s.find( "raiment"        ) != std::string::npos ||
         s.find( "gloves"         ) != std::string::npos ||
         s.find( "leggings"       ) != std::string::npos  )
    {
      return SET_T14_CASTER;
    }

    if ( s.find( "cowl"          ) != std::string::npos ||
         s.find( "mantle"        ) != std::string::npos ||
         s.find( "robes"         ) != std::string::npos ||
         s.find( "handwraps"     ) != std::string::npos ||
         s.find( "legwraps"      ) != std::string::npos  )
    {
      return SET_T14_HEAL;
    }
  }

  // Tier 15
  if ( s.find( "_of_the_exorcist" ) != std::string::npos  )
  {
    if ( s.find( "hood"           ) != std::string::npos ||
         s.find( "shoulderguards" ) != std::string::npos ||
         s.find( "raiment"        ) != std::string::npos ||
         s.find( "gloves"         ) != std::string::npos ||
         s.find( "leggings"       ) != std::string::npos  )
    {
      return SET_T15_CASTER;
    }

    if ( s.find( "cowl"          ) != std::string::npos ||
         s.find( "mantle"        ) != std::string::npos ||
         s.find( "robes"         ) != std::string::npos ||
         s.find( "handwraps"     ) != std::string::npos ||
         s.find( "legwraps"      ) != std::string::npos  )
    {
      return SET_T15_HEAL;
    }
  }

  // Tier 16
  if ( s.find( "_of_the_ternion_glory" ) != std::string::npos  )
  {
    if ( s.find( "hood"           ) != std::string::npos ||
         s.find( "shoulderguards" ) != std::string::npos ||
         s.find( "raiment"        ) != std::string::npos ||
         s.find( "gloves"         ) != std::string::npos ||
         s.find( "leggings"       ) != std::string::npos  )
    {
      return SET_T16_CASTER;
    }

    if ( s.find( "cowl"          ) != std::string::npos ||
         s.find( "mantle"        ) != std::string::npos ||
         s.find( "robes"         ) != std::string::npos ||
         s.find( "handwraps"     ) != std::string::npos ||
         s.find( "legwraps"      ) != std::string::npos  )
    {
      return SET_T16_HEAL;
    }
  }

  // PvP
  if ( s.find( "_gladiators_mooncloth_" ) != std::string::npos  ) return SET_PVP_HEAL;
  if ( s.find( "_gladiators_satin_"     ) != std::string::npos  ) return SET_PVP_CASTER;

  return SET_NONE;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class priest_report_t : public player_report_extension_t
{
public:
  priest_report_t( priest_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
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
  priest_module_t() : module_t( PRIEST ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    priest_t* p = new priest_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new priest_report_t( *p ) );
    return p;
  }
  virtual bool valid() const override { return true; }
  virtual void init( sim_t* sim ) const override
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> buffs.guardian_spirit  = buff_creator_t( p, "guardian_spirit", p -> find_spell( 47788 ) ); // Let the ability handle the CD
      p -> buffs.pain_supression  = buff_creator_t( p, "pain_supression", p -> find_spell( 33206 ) ); // Let the ability handle the CD
      p -> buffs.weakened_soul    = buff_creator_t( p, "weakened_soul",   p -> find_spell(  6788 ) );
    }
  }
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::priest()
{
  static priest_module_t m;
  return &m;
}
