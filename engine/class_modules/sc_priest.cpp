// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

/* Forward declarations
 */
struct priest_t;
namespace actions {
namespace spells {
struct shadowy_apparition_spell_t;
}
}

/* Priest target data
 * Contains target specific things
 */
struct priest_td_t : public actor_pair_t
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

  priest_td_t( player_t* target, priest_t& p );
};

/* Priest class definition
 *
 * Derived from player_t. Contains everything that defines the priest class.
 */
struct priest_t : public player_t
{
public:
  typedef player_t base_t;

  struct buffs_t
  {
    // Talents
    buff_t* power_infusion;
    buff_t* twist_of_fate;
    buff_t* surge_of_light;

    // Discipline
    buff_t* archangel;
    buff_t* borrowed_time;
    buff_t* holy_evangelism;
    buff_t* inner_fire;
    buff_t* inner_focus;
    buff_t* inner_will;
    buff_t* spirit_shell;

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
  } buffs;

  // Talents
  struct talents_t
  {
    const spell_data_t* void_tendrils;
    const spell_data_t* psyfiend;
    const spell_data_t* dominate_mind;

    const spell_data_t* body_and_soul;
    const spell_data_t* angelic_feather;
    const spell_data_t* phantasm;

    const spell_data_t* from_darkness_comes_light;
    const spell_data_t* mindbender;
    const spell_data_t* power_word_solace;

    const spell_data_t* desperate_prayer;
    const spell_data_t* spectral_guise;
    const spell_data_t* angelic_bulwark;

    const spell_data_t* twist_of_fate;
    const spell_data_t* power_infusion;
    const spell_data_t* divine_insight;

    const spell_data_t* cascade;
    const spell_data_t* divine_star;
    const spell_data_t* halo;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    // General

    // Discipline
    const spell_data_t* archangel;
    const spell_data_t* atonement;
    const spell_data_t* borrowed_time;
    const spell_data_t* divine_aegis;
    const spell_data_t* divine_fury;
    const spell_data_t* evangelism;
    const spell_data_t* grace;
    const spell_data_t* meditation_disc;
    const spell_data_t* mysticism;
    const spell_data_t* rapture;
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
    const spell_data_t* spiritual_precision;
  } specs;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* shield_discipline;
    const spell_data_t* echo_of_light;
    const spell_data_t* shadowy_recall;
  } mastery_spells;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* chakra;
    cooldown_t* inner_focus;
    cooldown_t* mindbender;
    cooldown_t* mind_blast;
    cooldown_t* penance;
    cooldown_t* rapture;
    cooldown_t* shadowfiend;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* devouring_plague_health;
    gain_t* dispersion;
    gain_t* hymn_of_hope;
    gain_t* mindbender;
    gain_t* power_word_solace;
    gain_t* rapture;
    gain_t* shadowfiend;
    gain_t* shadow_orb_mb;
    gain_t* shadow_orb_swd;
    gain_t* vampiric_touch_mana;
    gain_t* vampiric_touch_mastery_mana;
  } gains;

  // Benefits
  struct benefits_t
  {
    benefit_t* smites_with_glyph_increase;
  } benefits;

  // Procs
  struct procs_t
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

  struct shadowy_apparitions_t
  {
  private:
    typedef actions::spells::shadowy_apparition_spell_t sa_spell;
    priest_t& priest;
    std::vector<sa_spell*> apparitions_free;
    std::vector<player_t*> targets_queued;
    std::vector<sa_spell*> apparitions_active;

  public:
    shadowy_apparitions_t( priest_t& p ) : priest( p )
  { /* Do NOT access p or this -> priest here! */ }

    void trigger( action_state_t& d );
    void tidy_up( actions::spells::shadowy_apparition_spell_t& );
    void start_from_queue();
    void reset();
    void add_more( size_t num );

  } shadowy_apparitions;

  // Special
  struct active_spells_t
  {
    const spell_data_t* surge_of_darkness;
    action_t* echo_of_light;

    active_spells_t() :
      surge_of_darkness( nullptr ),
      echo_of_light( nullptr ) {}
  } active_spells;

  // Random Number Generators
  struct rngs_t
  {
    rng_t* mastery_extra_tick;
    rng_t* shadowy_apparitions;
  } rngs;

  // Pets
  struct pets_t
  {
    pet_t* shadowfiend;
    pet_t* mindbender;
    pet_t* lightwell;
  } pets;

  // Options
  int initial_shadow_orbs;
  std::string atonement_target_str;
  std::vector<player_t*> party_list;
  bool autoUnshift; // Shift automatically out of stance/form

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
    const spell_data_t* inner_fire;
    const spell_data_t* inner_sanctum;
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
  } glyphs;

private:
  target_specific_t<priest_td_t> target_data;

public:
  priest_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, PRIEST, name, r ),
    // initialize containers. For POD containers this sets all elements to 0.
    // use eg. buffs( buffs_t() ) instead of buffs() to help certain old compilers circumvent their bugs
    buffs( buffs_t() ),
    talents( talents_t() ),
    specs( specs_t() ),
    mastery_spells( mastery_spells_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    benefits( benefits_t() ),
    procs( procs_t() ),
    shadowy_apparitions( shadowy_apparitions_t( *this ) ),
    active_spells( active_spells_t() ),
    rngs( rngs_t() ),
    pets( pets_t() ),
    initial_shadow_orbs( 0 ),
    autoUnshift( true ),
    glyphs( glyphs_t() )
  {
    target_data.init( "target_data", this );

    initial.distance = 27.0;

    create_cooldowns();
    create_gains();
    create_procs();
    create_benefits();
  }

  // Function Definitions
  virtual void      init_base();
  virtual void      init_spells();
  virtual void      create_buffs();
  virtual void      init_values();
  virtual void      init_actions();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      reset();
  virtual void      init_party();
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, save_e=SAVE_ALL, bool save_html=false );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role();
  virtual void      combat_begin();
  virtual double    composite_armor();
  virtual double    composite_spell_haste();
  virtual double    composite_spell_power_multiplier();
  virtual double    composite_spell_hit();
  virtual double    composite_attack_hit();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    composite_player_heal_multiplier( school_e school );
  virtual double    composite_movement_speed();
  virtual double    composite_attribute_multiplier( attribute_e attr );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual void pre_analyze_hook();
  virtual priest_td_t* get_target_data( player_t* target )
  {
    priest_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new priest_td_t( target, *this );
    }
    return td;
  }

  void fixup_atonement_stats( const std::string& trigger_spell_name, const std::string& atonement_spell_name );
  double shadowy_recall_chance();

private:
  // Construction helper functions for priest_t members
  //static void create_buffs();
  void create_cooldowns();
  void create_gains();
  void create_procs();
  void create_benefits();
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
    initial.distance = 3;
  }

  struct _stat_list_t
  {
    int level;
    std::array<double,ATTRIBUTE_MAX> stats;
  };

  virtual void init_base()
  {
    pet_t::init_base();

    owner_coeff.ap_from_sp = 1.0;
    owner_coeff.sp_from_sp = 1.0;

    // Base Stats, same for all pets. Depend on level
    static const _stat_list_t pet_base_stats[] =
    {
      //   none, str, agi, sta, int, spi
      {  0, { { 0,   0,   0,   0,   0,   0 } } },
      { 85, { { 0, 453, 883, 353, 159, 225 } } },
    };

    assert( sizeof_array( pet_base_stats ) > 0 );
    assert( pet_base_stats[ 0 ].level <= 1 );

    // Loop from end to beginning to get the data for the highest available level equal or lower than the player level
    int i = sizeof_array( pet_base_stats );
    while ( --i > 0 )
    {
      if ( pet_base_stats[ i ].level <= level )
        break;
    }
    base.attribute = pet_base_stats[ i ].stats;
  }

  virtual void schedule_ready( timespan_t delta_time, bool waiting )
  {
    if ( main_hand_attack && ! main_hand_attack -> execute_event )
    {
      main_hand_attack -> schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  virtual double composite_player_multiplier( school_e school, action_t* a )
  {
    double m = pet_t::composite_player_multiplier( school, a );

    // Orc racial
    if ( owner -> race == RACE_ORC )
      m *= 1.0 + find_spell( 21563 ) -> effectN( 1 ).percent();

    return m;
  }

  virtual resource_e primary_resource()
  { return RESOURCE_ENERGY; }

  priest_t* o() const
  { return static_cast<priest_t*>( owner ); }
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
    buffs( buffs_t() ),
    gains( gains_t() ),
    shadowcrawl_action( nullptr ),
    direct_power_mod( 0.0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );

    owner_coeff.health = 0.3;
  }

  virtual double mana_return_percent() const = 0;

  virtual void init_actions()
  {
    if ( action_list_str.empty() )
    {
      action_list_str += "/snapshot_stats";
      action_list_str += "/shadowcrawl";
      action_list_str += "/wait_for_shadowcrawl";
    }

    priest_pet_t::init_actions();
  }

  virtual void create_buffs()
  {
    priest_pet_t::create_buffs();

    buffs.shadowcrawl = buff_creator_t( this, "shadowcrawl", find_pet_spell( "Shadowcrawl" ) );
  }

  virtual void init_gains()
  {
    priest_pet_t::init_gains();

    if      ( pet_type == PET_SHADOWFIEND )
      gains.fiend = o() -> gains.shadowfiend;
    else if ( pet_type == PET_MINDBENDER  )
      gains.fiend = o() -> gains.mindbender;
    else
      gains.fiend = get_gain( "basefiend" );
  }

  virtual void init_resources( bool force )
  {
    priest_pet_t::init_resources( force );

    resources.initial[ RESOURCE_MANA ] = owner -> resources.max[ RESOURCE_MANA ];
    resources.current = resources.max = resources.initial;
  }

  virtual void summon( timespan_t duration )
  {
    dismiss();

    duration += timespan_t::from_seconds( 0.01 );

    priest_pet_t::summon( duration );

    if ( shadowcrawl_action )
    {
      // Ensure that it gets used after the first melee strike. In the combat logs that happen at the same time, but the melee comes first.
      shadowcrawl_action -> cooldown -> ready = sim -> current_time + timespan_t::from_seconds( 0.001 );
    }
  }

  virtual void init_base();
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str );
};

// ==========================================================================
// Pet Shadowfiend
// ==========================================================================

struct shadowfiend_pet_t : public base_fiend_pet_t
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

  virtual double mana_return_percent() const
  { return mana_leech -> effectN( 1 ).percent(); }
};

// ==========================================================================
// Pet Mindbender
// ==========================================================================

struct mindbender_pet_t : public base_fiend_pet_t
{
  const spell_data_t* mindbender_spell;

  mindbender_pet_t( sim_t* sim, priest_t& owner, const std::string& name = "mindbender" ) :
    base_fiend_pet_t( sim, owner, PET_MINDBENDER, name ),
    mindbender_spell( owner.find_talent_spell( "Mindbender" ) )
  {
    direct_power_mod = 0.80;

    main_hand_weapon.min_dmg = owner.dbc.spell_scaling( owner.type, owner.level ) * 1.6;
    main_hand_weapon.max_dmg = owner.dbc.spell_scaling( owner.type, owner.level ) * 1.6;

    main_hand_weapon.damage  = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  }

  virtual double mana_return_percent() const
  {
    double m  = mindbender_spell -> effectN( 2 ).base_value();
           m /= mindbender_spell -> effectN( 3 ).base_value();
    return m / 100;
  }
};

struct lightwell_pet_t : public priest_pet_t
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
                                   const std::string& options_str );

  virtual void summon( timespan_t duration )
  {
    charges = 10 + o() -> glyphs.lightwell -> effectN( 1 ).base_value();

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

  virtual void reset()
  {
    melee_attack_t::reset();
    first_swing = true;
  }

  priest_pet_t* p() const
  { return static_cast<priest_pet_t*>( player ); }

  virtual timespan_t execute_time()
  {
    // First swing comes instantly after summoning the pet
    if ( first_swing )
      return timespan_t::zero();

    return melee_attack_t::execute_time();
  }

  virtual void schedule_execute( action_state_t* state = 0 )
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

  priest_pet_t& p() const
  { return static_cast<priest_pet_t&>( *player ); }
};

struct shadowcrawl_t : public priest_pet_spell_t
{
  shadowcrawl_t( base_fiend_pet_t& p ) :
    priest_pet_spell_t( p, "Shadowcrawl" )
  {
    may_miss  = false;
    harmful   = false;
  }

  base_fiend_pet_t& p() const
  { return static_cast<base_fiend_pet_t&>( *player ); }

  virtual void execute()
  {
    priest_pet_spell_t::execute();

    p().buffs.shadowcrawl -> trigger();
  }
};

struct fiend_melee_t : public priest_pet_melee_t
{
  fiend_melee_t( base_fiend_pet_t& p ) :
    priest_pet_melee_t( p, "melee" )
  {
    weapon = &( p.main_hand_weapon );
    weapon_multiplier = 0.0;
    base_dd_min       = weapon -> min_dmg;
    base_dd_max       = weapon -> max_dmg;
    direct_power_mod  = p.direct_power_mod;
  }

  base_fiend_pet_t& p() const
  { return static_cast<base_fiend_pet_t&>( *player ); }

  virtual double action_multiplier()
  {
    double am = priest_pet_melee_t::action_multiplier();

    am *= 1.0 + p().buffs.shadowcrawl -> up() * p().buffs.shadowcrawl -> data().effectN( 2 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    priest_pet_melee_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p().o() -> resource_gain( RESOURCE_MANA, p().o() -> resources.max[ RESOURCE_MANA ] *
                                p().mana_return_percent(),
                                p().gains.fiend );
    }
  }
};

struct lightwell_renew_t : public heal_t
{
  lightwell_renew_t( lightwell_pet_t& p ) :
    heal_t( "lightwell_renew", &p, p.find_spell( 7001 ) )
  {
    may_crit = false;
    tick_may_crit = true;

    tick_power_mod = 0.308;
  }

  lightwell_pet_t& p() const
  { return static_cast<lightwell_pet_t&>( *player ); }

  virtual void execute()
  {
    p().charges--;

    target = find_lowest_player();

    heal_t::execute();
  }

  virtual void last_tick( dot_t* d )
  {
    heal_t::last_tick( d );

    if ( p().charges <= 0 )
      p().dismiss();
  }

  virtual bool ready()
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

void base_fiend_pet_t::init_base()
{
  priest_pet_t::init_base();

  main_hand_attack = new actions::fiend_melee_t( *this );
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
private:
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
public:
private:
  typedef Base ab; // typedef for the templated action type, eg. spell_t, attack_t, heal_t
public:
  typedef priest_action_t base_t; // typedef for priest_action_t<action_base_t>

  virtual ~priest_action_t() {}
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


    can_cancel_shadowform = p.autoUnshift;
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

  priest_td_t& td( player_t* t = nullptr )
  { return *( priest.get_target_data( t ? t : this -> target ) ); }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    cancel_shadowform();

    ab::schedule_execute( state );
  }

  virtual bool ready()
  {
    if ( ! ab::ready() )
      return false;

    if ( ! check_shadowform() )
      return false;

    return ( min_interval -> remains() <= timespan_t::zero() );
  }

  virtual double cost()
  {
    double c = ab::cost();

    if ( ( this -> base_execute_time <= timespan_t::zero() ) && ! this -> channeled )
    {
      c *= 1.0 + priest.buffs.inner_will -> check() * priest.buffs.inner_will -> data().effectN( 1 ).percent();
      c  = floor( c );
    }


    if ( priest.buffs.power_infusion -> check() )
      c *= 1.0 + priest.buffs.power_infusion -> data().effectN( 2 ).percent();

    return c;
  }

  virtual void consume_resource()
  {
    ab::consume_resource();

    if ( this -> base_execute_time <= timespan_t::zero() )
      priest.buffs.inner_will -> up();
    else if ( this -> base_execute_time > timespan_t::zero() && ! this -> channeled )
      priest.buffs.borrowed_time -> expire();
  }

  void parse_options( option_t*          options,
                      const std::string& options_str )
  {
    const option_t base_options[] =
    {
      opt_timespan( "min_interval", ( min_interval -> duration ) ),
      opt_null()
    };

    std::vector<option_t> merged_options;
    ab::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }

  void update_ready( timespan_t cd_duration )
  {
    ab::update_ready( cd_duration );

    if ( min_interval -> duration > timespan_t::zero() && ! this -> dual )
    {
      min_interval -> start( timespan_t::min(), timespan_t::zero() );

      if ( this -> sim -> debug )
        this -> sim -> output( "%s starts min_interval for %s (%s). Will be ready at %.4f",
                               priest.name(), this -> name(), min_interval -> name(), min_interval -> ready.total_seconds() );
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
                   const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, player, s )
  {
    may_crit          = false;
    tick_may_crit     = false;
    may_miss          = false;
  }

  virtual double action_multiplier()
  {
    return base_t::action_multiplier() *
           ( 1.0 + ( priest.composite_mastery() *
                     priest.mastery_spells.shield_discipline -> effectN( 1 ).coeff() / 100.0 ) );
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
      direct_power_mod = 0.0;
    }

    void impact( action_state_t* s ) // override
    {
      absorb_buff_t& buff = *td( s -> target ).buffs.divine_aegis;
      // Divine Aegis caps absorbs at 40% of target's health
      double old_amount = td( s -> target ).buffs.divine_aegis -> value();
      double new_amount = std::max( 0.0, std::min( s -> target -> resources.current[ RESOURCE_HEALTH ] * 0.4 - old_amount, s -> result_amount ) );
      buff.trigger( 1, old_amount + new_amount );
      stats -> add_result( 0.0, new_amount, ABSORB, s -> result );
      buff.absorb_source -> add_result( 0.0, new_amount, ABSORB, s -> result );
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

    double action_multiplier() // override
    {
      double am;

      am = absorb_t::action_multiplier(); // ( 1 + 0 ) *

      return am *
             ( 1 + trigger_crit_multiplier ) *           // ( 1 + crit ) *
             ( 1 + trigger_crit_multiplier * 0.3 );      // ( 1 + crit * 30% "DA factor" )
    }

    void trigger( action_state_t* s )
    {
      assert( s -> result != RESULT_CRIT );
      base_dd_min = base_dd_max = s -> result_amount;
      target = s -> target;
      trigger_crit_multiplier = s -> composite_crit();
      execute();
    }

    void impact( action_state_t* s ) // override
    {
      // Spirit Shell caps absorbs at 60% of target's health
      double old_amount = td( s -> target ).buffs.spirit_shell -> value();
      double new_amount = std::max( 0.0, std::min( s -> target -> resources.current[ RESOURCE_HEALTH ] * 0.6 - old_amount,
                                                   s -> result_amount ) );
      td( s -> target ).buffs.spirit_shell -> trigger( 1, old_amount + new_amount );
      stats -> add_result( 0.0, new_amount, ABSORB, s -> result );
    }
  };

  divine_aegis_t* da;
  spirit_shell_absorb_t* ss;
  unsigned divine_aegis_trigger_mask;
  bool can_trigger_EoL, can_trigger_spirit_shell;

  virtual void init()
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

  virtual double composite_crit()
  {
    double cc = base_t::composite_crit();

    if ( priest.buffs.chakra_serenity -> up() )
      cc += priest.buffs.chakra_serenity -> data().effectN( 1 ).percent();

    return cc;
  }

  virtual double composite_target_crit( player_t* t )
  {
    double ctc = base_t::composite_target_crit( t );

    if ( td( t ).buffs.holy_word_serenity -> up() )
      ctc += td( t ).buffs.holy_word_serenity -> data().effectN( 2 ).percent();

    return ctc;
  }

  virtual double action_multiplier()
  {
    return base_t::action_multiplier() * ( 1.0 + priest.buffs.archangel -> value() );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = base_t::composite_target_multiplier( t );

    if ( priest.specs.grace -> ok() )
      ctm *= 1.0 + t -> buffs.grace -> check() * t -> buffs.grace -> value();

    return ctm;
  }

  virtual void execute()
  {
    if ( can_trigger_spirit_shell )
      may_crit = priest.buffs.spirit_shell -> check() == 0;

    base_t::execute();

    may_crit = true;
  }

  virtual void impact( action_state_t* s )
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

        if ( priest.buffs.chakra_serenity -> up() && td( s -> target ).dots.renew -> ticking )
          td( s -> target ).dots.renew -> refresh_duration();

        if ( priest.talents.twist_of_fate -> ok() && ( save_health_percentage < priest.talents.twist_of_fate -> effectN( 1 ).base_value() ) )
        {
          priest.buffs.twist_of_fate -> trigger();
        }
      }
    }
  }

  virtual void tick( dot_t* d ) /* override */
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
      s -> result_amount * p.composite_mastery() * p.mastery_spells.echo_of_light -> effectN( 1 ).mastery_value() ); // ignite damage
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
  struct atonement_heal_t : public priest_heal_t
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

      if ( ! p.atonement_target_str.empty() )
        target = sim -> find_player( p.atonement_target_str );
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

    virtual double composite_target_multiplier( player_t* target )
    {
      double m = priest_heal_t::composite_target_multiplier( target );
      if ( target == player )
        m *= 0.5; // Hardcoded in the tooltip
      return m;
    }

    virtual double total_crit_bonus()
    { return 0; }

    virtual void execute()
    {
      player_t* saved_target = target;
      if ( ! target )
        target = find_lowest_player();

      priest_heal_t::execute();

      target = saved_target;
    }

    virtual void tick( dot_t* d )
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

  virtual void init()
  {
    base_t::init();

    if ( can_trigger_atonement && priest.specs.atonement -> ok() )
      atonement = new atonement_heal_t( "atonement_" + name_str, priest );
  }

  virtual void impact( action_state_t* s )
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
                              action_state_t* s )
  {
    base_t::assess_damage( type, s );

    if ( aoe == 0 && priest.buffs.vampiric_embrace -> up() && result_is_hit( s -> result ) )
    {
      double a = s -> result_amount * ( priest.buffs.vampiric_embrace -> data().effectN( 1 ).percent() + priest.glyphs.vampiric_embrace -> effectN( 2 ).percent() ) ;

      // Split amongst number of people in raid.
      a /= 1.0 + priest.party_list.size();

      // Priest Heal
      priest.resource_gain( RESOURCE_HEALTH, a, player -> gains.vampiric_embrace );

      // Pet Heal
      // Pet's get a full share without counting against the number in the raid.
      for ( size_t i = 0; i < player -> pet_list.size(); ++i )
      {
        pet_t* r = player -> pet_list[ i ];
        r -> resource_gain( RESOURCE_HEALTH, a, r -> gains.vampiric_embrace );
      }

      // Party Heal
      for ( size_t i = 0; i < priest.party_list.size(); i++ )
      {
        player_t* q = priest.party_list[ i ];

        q -> resource_gain( RESOURCE_HEALTH, a, q -> gains.vampiric_embrace );

        for ( size_t j = 0; j < player -> pet_list.size(); ++j )
        {
          pet_t* r = player -> pet_list[ j ];
          r -> resource_gain( RESOURCE_HEALTH, a, r -> gains.vampiric_embrace );
        }
      }
    }

    if ( atonement )
      atonement -> trigger( s -> result_amount, direct_tick ? DMG_OVER_TIME : type, s -> result );
  }

  void generate_shadow_orb( gain_t* g, unsigned number = 1 )
  {
    if ( priest.specs.shadow_orbs -> ok() )
      priest.resource_gain( RESOURCE_SHADOW_ORB, number, g, this );
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

struct archangel_t : public priest_spell_t
{
  archangel_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "archangel", p, p.specs.archangel )
  {
    parse_options( NULL, options_str );
    harmful = may_hit = may_crit = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    priest.buffs.archangel -> trigger();
  }

  virtual bool ready()
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
    parse_options( NULL, options_str );

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

struct chakra_chastise_t : public chakra_base_t
{
  chakra_chastise_t( priest_t& p, const std::string& options_str ) :
    chakra_base_t( p, p.find_class_spell( "Chakra: Chastise" ), options_str )
  { }

  virtual void execute()
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_chastise );
  }
};

struct chakra_sanctuary_t : public chakra_base_t
{
  chakra_sanctuary_t( priest_t& p, const std::string& options_str ) :
    chakra_base_t( p, p.find_class_spell( "Chakra: Sanctuary" ), options_str )
  { }

  virtual void execute()
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_sanctuary );
  }
};

struct chakra_serenity_t : public chakra_base_t
{
  chakra_serenity_t( priest_t& p, const std::string& options_str ) :
    chakra_base_t( p, p.find_class_spell( "Chakra: Serenity" ), options_str )
  { }

  virtual void execute()
  {
    chakra_base_t::execute();
    switch_to( priest.buffs.chakra_serenity );
  }
};

// Dispersion Spell =========================================================

struct dispersion_t : public priest_spell_t
{
  dispersion_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "dispersion", player, player.find_class_spell( "Dispersion" ) )
  {
    parse_options( NULL, options_str );

    base_tick_time    = timespan_t::from_seconds( 1.0 );
    num_ticks         = 6;

    channeled         = true;
    harmful           = false;
    tick_may_crit     = false;

    cooldown -> duration += priest.glyphs.dispersion -> effectN( 1 ).time_value();
  }

  virtual void tick( dot_t* d )
  {
    double regen_amount = priest.find_spell( 49766 ) -> effectN( 1 ).percent() * priest.resources.max[ RESOURCE_MANA ];

    priest.resource_gain( RESOURCE_MANA, regen_amount, priest.gains.dispersion );

    priest_spell_t::tick( d );
  }
};

// Fortitude Spell ==========================================================

struct fortitude_t : public priest_spell_t
{
  fortitude_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "fortitude", player, player.find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.stamina != 0 );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger();
  }
};

// Hymn of Hope Spell =======================================================

struct hymn_of_hope_tick_t : public priest_spell_t
{
  hymn_of_hope_tick_t( priest_t& player ) :
    priest_spell_t( "hymn_of_hope_tick", player, player.find_spell( 64904 ) )
  {
    dual        = true;
    background  = true;
    may_crit    = true;
    direct_tick = true;

    harmful     = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest.resource_gain( RESOURCE_MANA, data().effectN( 1 ).percent() * priest.resources.max[ RESOURCE_MANA ], priest.gains.hymn_of_hope );

    // Hymn of Hope only adds +x% of the current_max mana, it doesn't change if afterwards max_mana changes.
    player -> buffs.hymn_of_hope -> trigger();
  }
};

struct hymn_of_hope_t : public priest_spell_t
{
  hymn_of_hope_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "hymn_of_hope", p, p.find_class_spell( "Hymn of Hope" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    channeled = true;
    dynamic_tick_action = true;

    tick_action = new hymn_of_hope_tick_t( p );
  }

  virtual void init()
  {
    priest_spell_t::init();

    tick_action -> stats = stats;
  }
};

// Inner Focus Spell ========================================================

struct inner_focus_t : public priest_spell_t
{
  inner_focus_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "inner_focus", p, p.find_class_spell( "Inner Focus" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    cooldown -> duration = timespan_t::from_seconds( sim -> wheel_seconds );

    priest_spell_t::execute();

    priest.buffs.inner_focus -> trigger();
  }
};

// Inner Fire Spell =========================================================

struct inner_fire_t : public priest_spell_t
{
  inner_fire_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "inner_fire", player, player.find_class_spell( "Inner Fire" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest.buffs.inner_will -> expire ();
    priest.buffs.inner_fire -> trigger();
  }

  virtual bool ready()
  {
    if ( priest.buffs.inner_fire -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Inner Will Spell =========================================================

struct inner_will_t : public priest_spell_t
{
  inner_will_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "inner_will", player, player.find_class_spell( "Inner Will" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest.buffs.inner_fire -> expire();

    priest.buffs.inner_will -> trigger();
  }

  virtual bool ready()
  {
    if ( priest.buffs.inner_will -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Pain Supression ==========================================================

struct pain_suppression_t : public priest_spell_t
{
  pain_suppression_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "pain_suppression", p, p.find_class_spell( "Pain Suppression" ) )
  {
    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = &p;
    }

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    target -> buffs.pain_supression -> trigger();
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
{
  power_infusion_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "power_infusion", p, p.talents.power_infusion )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    priest.buffs.power_infusion -> trigger();
  }
};

// Shadow Form Spell ========================================================

struct shadowform_t : public priest_spell_t
{
  shadowform_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "shadowform", p, p.find_class_spell( "Shadowform" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest.buffs.shadowform -> trigger();
  }

  virtual bool ready()
  {
    if (  priest.buffs.shadowform -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Spirit Shell Spell =======================================================

struct spirit_shell_t : public priest_spell_t
{
  spirit_shell_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "spirit_shell", p, p.specs.spirit_shell )
  {
    parse_options( NULL, options_str );
    harmful = may_hit = may_crit = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();
    priest.buffs.spirit_shell -> trigger();
  }
};

// PET SPELLS

struct summon_pet_t : public priest_spell_t
{
  timespan_t summoning_duration;
  pet_t* pet;

public:
  summon_pet_t( const std::string& n, priest_t& p, const spell_data_t* sd = spell_data_t::nil() ) :
    priest_spell_t( n, p, sd ),
    summoning_duration ( timespan_t::zero() ),
    pet( nullptr )
  {
    harmful = false;

    pet = p.find_pet( n );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), n.c_str() );
    }
  }

  virtual void execute()
  {
    pet -> summon( summoning_duration );

    priest_spell_t::execute();
  }
};


struct summon_shadowfiend_t : public summon_pet_t
{
  summon_shadowfiend_t( priest_t& p, const std::string& options_str ) :
    summon_pet_t( "shadowfiend", p, p.find_class_spell( "Shadowfiend" ) )
  {
    parse_options( NULL, options_str );
    harmful    = false;
    summoning_duration = data().duration();
    cooldown = p.cooldowns.shadowfiend;
    cooldown -> duration = data().cooldown();
  }
};


struct summon_mindbender_t : public summon_pet_t
{
  summon_mindbender_t( priest_t& p, const std::string& options_str ) :
    summon_pet_t( "mindbender", p, p.find_talent_spell( "Mindbender" ) )
  {
    parse_options( NULL, options_str );
    harmful    = false;
    summoning_duration = data().duration();
    cooldown = p.cooldowns.mindbender;
    cooldown -> duration = data().cooldown();
  }
};

// ==========================================================================
// Priest Damage Spells
// ==========================================================================

struct priest_procced_mastery_spell_t : public priest_spell_t
{
  priest_procced_mastery_spell_t( const std::string& n, priest_t& p,
                                  const spell_data_t* s = spell_data_t::nil() ) :
    priest_spell_t( n, p, p.mastery_spells.shadowy_recall -> ok() ? s : spell_data_t::not_found() )
  {
    stormlash_da_multiplier = 0.0;
    background              = true;
    proc                    = true;
    base_execute_time       = timespan_t::zero();
  }


  virtual timespan_t travel_time()
  {
    return sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );
  }

  virtual void impact( action_state_t* s )
  {
    priest_spell_t::impact( s );

    priest.procs.mastery_extra_tick -> occur();
  }
};

// Shadowy Apparition Spell =================================================

struct shadowy_apparition_spell_t : public priest_spell_t
{
  rng_t* t15_2pc;

  shadowy_apparition_spell_t( priest_t& p ) :
    priest_spell_t( "shadowy_apparition", p, p.specs.shadowy_apparitions -> ok() ? p.find_spell( 87532 ) : spell_data_t::not_found() ),
    t15_2pc( nullptr )
  {
    background        = true;
    proc              = true;
    callbacks         = false;

    trigger_gcd       = timespan_t::zero();
    travel_speed      = 3.5;
    direct_power_mod  = 0.375;

    // Create this for everyone, as when we make shadowy_apparition_spell_t, we haven't run init_items() yet
    t15_2pc = player -> get_rng( "Tier15 2pc caster" );
  }

  virtual void impact( action_state_t* s )
  {
    priest_spell_t::impact( s );

    if ( priest.set_bonus.tier15_2pc_caster() )
    {
      if ( t15_2pc && t15_2pc -> roll( priest.sets -> set( SET_T15_2PC_CASTER ) -> effectN( 1 ).percent() ) )
      {
        priest_td_t& td = this -> td( s -> target );
        priest.procs.t15_2pc_caster -> occur();

        if ( td.dots.shadow_word_pain -> ticking )
        {
          td.dots.shadow_word_pain -> extend_duration( 1 );
          priest.procs.t15_2pc_caster_shadow_word_pain -> occur();
        }

        if ( td.dots.vampiric_touch -> ticking )
        {
          td.dots.vampiric_touch   -> extend_duration( 1 );
          priest.procs.t15_2pc_caster_vampiric_touch -> occur();
        }
      }
    }

    priest.shadowy_apparitions.tidy_up( *this );

    priest.shadowy_apparitions.start_from_queue();
  }
};

// Mind Blast Spell =========================================================

struct mind_blast_t : public priest_spell_t
{
  bool casted_with_divine_insight;

  mind_blast_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "mind_blast", player, player.find_class_spell( "Mind Blast" ) ),
    casted_with_divine_insight( false )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    casted_with_divine_insight = false;
  }

  virtual void impact( action_state_t* s )
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      generate_shadow_orb( priest.gains.shadow_orb_mb );

      priest.buffs.glyph_mind_spike -> expire();
    }
  }

  void consume_resource()
  {
    if ( casted_with_divine_insight )
      resource_consumed = 0.0;
    else
      resource_consumed = cost();

    player -> resource_loss( current_resource(), resource_consumed, 0, this );

    if ( sim -> log )
      sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double cost()
  {
    if ( priest.buffs.divine_insight_shadow -> check() )
      return 0.0;

    return priest_spell_t::cost();
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    if ( priest.buffs.divine_insight_shadow -> up() )
    {
      casted_with_divine_insight = true;
    }

    priest_spell_t::schedule_execute( state );

    priest.buffs.divine_insight_shadow -> expire();
  }

  virtual void update_ready( timespan_t cd_duration )
  {
    if ( priest.specs.mind_surge -> ok() && ! priest.buffs.divine_insight_shadow -> check() )
    {
      cd_duration = cooldown -> duration * composite_haste();
    }
    priest_spell_t::update_ready( cd_duration );
  }

  virtual timespan_t execute_time()
  {
    timespan_t a = priest_spell_t::execute_time();

    if ( priest.buffs.divine_insight_shadow -> check() )
    {
      return timespan_t::zero();
    }

    a *= 1 + ( priest.buffs.glyph_mind_spike -> stack() * priest.glyphs.mind_spike -> effectN( 1 ).percent() );

    return a;
  }

  virtual void reset()
  {
    priest_spell_t::reset();

    casted_with_divine_insight = false;
  }
};

// Mind Spike Spell =========================================================

struct mind_spike_t : public priest_spell_t
{
  struct mind_spike_state_t : public action_state_t
  {
    bool surge_of_darkness;

    mind_spike_state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      surge_of_darkness ( false )
    {
    }

    virtual void copy_state( const action_state_t* o )
    {
      if ( this == o || o == 0 ) return;

      action_state_t::copy_state( o );

      const mind_spike_state_t* dps_t = static_cast< const mind_spike_state_t* >( o );
      surge_of_darkness = dps_t -> surge_of_darkness;
    }
  };

  bool casted_with_surge_of_darkness;

  mind_spike_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "mind_spike", player, player.find_class_spell( "Mind Spike" ) ),
    casted_with_surge_of_darkness( false )
  {
    parse_options( NULL, options_str );
  }

  virtual action_state_t* new_state()
  {
    return new mind_spike_state_t( this, target );
  }

  virtual action_state_t* get_state( const action_state_t* s )
  {
    action_state_t* s_ = priest_spell_t::get_state( s );

    if ( s == 0 )
    {
      mind_spike_state_t* ds_ = static_cast< mind_spike_state_t* >( s_ );
      ds_ -> surge_of_darkness = false;
    }

    return s_;
  }

  virtual void snapshot_state( action_state_t* state, uint32_t flags, dmg_e type )
  {
    mind_spike_state_t* dps_t = static_cast< mind_spike_state_t* >( state );

    dps_t -> surge_of_darkness = casted_with_surge_of_darkness;

    priest_spell_t::snapshot_state( state, flags, type );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    casted_with_surge_of_darkness = false;
  }

  virtual void impact( action_state_t* s )
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      mind_spike_state_t* dps_t = static_cast< mind_spike_state_t* >( s );
      if ( ! dps_t -> surge_of_darkness )
      {
        cancel_dot( *td( s -> target ).dots.shadow_word_pain );
        cancel_dot( *td( s -> target ).dots.vampiric_touch );
        cancel_dot( *td( s -> target ).dots.devouring_plague_tick );
        priest.procs.mind_spike_dot_removal -> occur();

        priest.buffs.glyph_mind_spike -> trigger();
      }
    }
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    if ( priest.buffs.surge_of_darkness -> up() )
    {
      casted_with_surge_of_darkness = true;
    }

    priest_spell_t::schedule_execute( state );

    priest.buffs.surge_of_darkness -> decrement();
  }

  void consume_resource()
  {
    if ( casted_with_surge_of_darkness )
      resource_consumed = 0.0;
    else
      resource_consumed = cost();

    player -> resource_loss( current_resource(), resource_consumed, 0, this );

    if ( sim -> log )
      sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double composite_da_multiplier()
  {
    double d = priest_spell_t::composite_da_multiplier();

    if ( casted_with_surge_of_darkness )
    {
      d *= 1.0 + priest.active_spells.surge_of_darkness -> effectN( 4 ).percent();
    }

    return d;
  }

  virtual double cost()
  {
    if ( priest.buffs.surge_of_darkness -> check() )
      return 0.0;

    return priest_spell_t::cost();
  }


  virtual timespan_t execute_time()
  {
    if ( priest.buffs.surge_of_darkness -> check() )
    {
      return timespan_t::zero();
    }

    return priest_spell_t::execute_time();
  }

  virtual void reset()
  {
    priest_spell_t::reset();

    casted_with_surge_of_darkness = false;
  }
};

// Mind Sear Mastery Proc ===========================================

struct mind_sear_mastery_t : public priest_procced_mastery_spell_t
{
  mind_sear_mastery_t( priest_t& p ) :
    priest_procced_mastery_spell_t( "mind_sear_mastery", p,
                                    p.find_class_spell( "Mind Sear" ) -> ok() ? p.find_spell( 124469 ) : spell_data_t::not_found() )
  {
  }

  virtual void impact( action_state_t* s )
  {
    priest_procced_mastery_spell_t::impact( s );
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t : public priest_spell_t
{
  mind_sear_mastery_t* proc_spell;

  mind_sear_tick_t( priest_t& player ) :
    priest_spell_t( "mind_sear_tick", player, player.find_class_spell( "Mind Sear" ) -> effectN( 1 ).trigger() ),
    proc_spell( nullptr )
  {
    background  = true;
    dual        = true;
    aoe         = -1;
    callbacks   = false;
    direct_tick = true;

    if ( player.mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new mind_sear_mastery_t( player );
      add_child( proc_spell );
    }
  }

  virtual void impact( action_state_t* s )
  {
    priest_spell_t::impact( s );

    if ( proc_spell && priest.rngs.mastery_extra_tick -> roll( priest.shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }
  }
};

struct mind_sear_t : public priest_spell_t
{
  mind_sear_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "mind_sear", p, p.find_class_spell( "Mind Sear" ) )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    may_crit     = false;
    hasted_ticks = false;
    dynamic_tick_action = true;

    tick_action = new mind_sear_tick_t( p );
  }

  virtual void init()
  {
    priest_spell_t::init();

    tick_action -> stats = stats;
  }
};

// Shadow Word Death Spell ==================================================

struct shadow_word_death_t : public priest_spell_t
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
      base_dd_min = base_dd_max = 0.533 * p.dbc.spell_scaling( data().scaling_class(), p.level );
      direct_power_mod = 0.599;

      target = &p;
    }

    virtual void init()
    {
      priest_spell_t::init();

      stats -> type = STATS_NEUTRAL;
    }

    virtual timespan_t execute_time()
    {
      return sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );
    }

    virtual double composite_spell_power()
    {
      return spellpower;
    }

    virtual double composite_spell_power_multiplier()
    {
      return 1.0;
    }

    virtual double composite_da_multiplier()
    {
      double d = multiplier;
      if ( priest.set_bonus.tier13_2pc_caster() )
        d *= 0.663587;

      return d;
    }
  };

  shadow_word_death_backlash_t* backlash;

  shadow_word_death_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "shadow_word_death", p, p.find_class_spell( "Shadow Word: Death" ) ),
    backlash( new shadow_word_death_backlash_t( p ) )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p.sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    bool below_20 = ( target -> health_percentage() < 20.0 );

    priest_spell_t::execute();

    if ( below_20 && ! priest.buffs.shadow_word_death_reset_cooldown -> up() && priest.specialization() == PRIEST_SHADOW )
    {
      cooldown -> reset( false );
      priest.buffs.shadow_word_death_reset_cooldown -> trigger();
    }
  }

  virtual void impact( action_state_t* s )
  {
    s -> result_amount = floor( s -> result_amount );

    if ( backlash && s -> target -> health_percentage() >= 20.0 )
    {
      backlash -> spellpower = s -> composite_spell_power();
      backlash -> multiplier = s -> da_multiplier;
      backlash -> schedule_execute();
    }

    if ( result_is_hit() )
    {
      if ( s -> target -> health_percentage() >= 20.0 )
        s -> result_amount /= 4.0;
      else if ( ! priest.buffs.shadow_word_death_reset_cooldown -> check() )
        generate_shadow_orb( priest.gains.shadow_orb_swd );
    }

    priest_spell_t::impact( s );
  }

  virtual bool ready()
  {
    if ( !priest_spell_t::ready() )
      return false;

    if ( !priest.glyphs.shadow_word_death -> ok() && target -> health_percentage() >= 20.0 )
      return false;

    return true;

  }
};

// Devouring Plague Mastery Proc ===========================================

struct devouring_plague_mastery_t : public priest_procced_mastery_spell_t
{
  int orbs_used;
  double heal_pct;

  devouring_plague_mastery_t( priest_t& p ) :
    priest_procced_mastery_spell_t( "devouring_plague_mastery", p,
                                    p.find_class_spell( "Devouring Plague" ) -> ok() ? p.find_spell( 124467 ) : spell_data_t::not_found() ),
    orbs_used( 0 ),
    heal_pct( p.find_class_spell( "Devouring Plague" ) -> effectN( 3 ).percent() / 100.0 )
  {
  }

  virtual double action_da_multiplier()
  {
    double m = priest_spell_t::action_da_multiplier();

    m *= orbs_used;

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    priest_procced_mastery_spell_t::impact( s );

    double a = heal_pct * orbs_used * priest.resources.max[ RESOURCE_HEALTH ];

    priest.resource_gain( RESOURCE_HEALTH, a, priest.gains.devouring_plague_health );
  }
};

// Devouring Plague State ===================================================

struct devouring_plague_state_t : public action_state_t
{
  int orbs_used;

  devouring_plague_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ),
    orbs_used ( 0 )
  { }

  virtual void copy_state( const action_state_t* o )
  {
    if ( this == o || o == 0 ) return;

    action_state_t::copy_state( o );

    const devouring_plague_state_t* dps_t = debug_cast< const devouring_plague_state_t* >( o );
    orbs_used = dps_t -> orbs_used;
  }
};

// Devouring Plague Spell ===================================================

struct devouring_plague_t : public priest_spell_t
{
  struct devouring_plague_dot_t : public priest_spell_t
  {
    devouring_plague_mastery_t* proc_spell;
    double special_tick_dmg;

    devouring_plague_dot_t( priest_t& p, priest_spell_t* pa ) :
      priest_spell_t( "devouring_plague_tick", p, p.find_class_spell( "Devouring Plague" ) ),
      proc_spell( nullptr ),
      special_tick_dmg( 0.0 )
    {
      parse_effect_data( data().effectN( 5 ) );

      tick_power_mod = direct_power_mod;

      base_dd_min = base_dd_max = direct_power_mod = 0.0;

      background = true;

      if ( p.mastery_spells.shadowy_recall -> ok() )
      {
        proc_spell = new devouring_plague_mastery_t( p );
        if ( pa )
          pa -> add_child( proc_spell );
      }
    }

    virtual void reset()
    {
      priest_spell_t::reset();

      special_tick_dmg = 0;
    }

    virtual double calculate_tick_damage( result_e r, double power, double multiplier, player_t* t )
    {
      special_tick_dmg = action_t::calculate_tick_damage( r, power, 1.0, t );

      return action_t::calculate_tick_damage( r, power, multiplier, t );
    }

    virtual action_state_t* new_state()
    {
      return new devouring_plague_state_t( this, target );
    }

    virtual void init()
    {
      priest_spell_t::init();

      // Override snapshot flags
      snapshot_flags |= STATE_USER_1;
    }

    virtual action_state_t* get_state( const action_state_t* s = nullptr )
    {
      action_state_t* s_ = priest_spell_t::get_state( s );

      if ( s == 0 )
      {
        devouring_plague_state_t* ds_ = static_cast< devouring_plague_state_t* >( s_ );
        ds_ -> orbs_used = 0;
      }

      return s_;
    }

    virtual void snapshot_state( action_state_t* state, uint32_t flags, dmg_e type )
    {
      devouring_plague_state_t* dps_t = static_cast< devouring_plague_state_t* >( state );

      if ( flags & STATE_USER_1 )
        dps_t -> orbs_used = ( int ) priest.resources.current[ current_resource() ];

      priest_spell_t::snapshot_state( state, flags, type );
    }

    virtual double action_ta_multiplier()
    {
      double m = priest_spell_t::action_ta_multiplier();

      m *= priest.resources.current[ current_resource() ];

      return m;
    }

    virtual void impact( action_state_t* s )
    {
      double saved_impact_dmg = s -> result_amount;
      s -> result_amount = 0;
      priest_spell_t::impact( s );

      dot_t* dot = get_dot();
      base_ta_adder = saved_impact_dmg / dot -> ticks();
    }

    virtual void tick( dot_t* d )
    {
      priest_spell_t::tick( d );

      devouring_plague_state_t* dps_state = debug_cast< devouring_plague_state_t* >( d -> state );

      double a = data().effectN( 3 ).percent() / 100.0 * dps_state -> orbs_used * priest.resources.max[ RESOURCE_HEALTH ];
      priest.resource_gain( RESOURCE_HEALTH, a, priest.gains.devouring_plague_health );

      if ( proc_spell && dps_state -> orbs_used && priest.rngs.mastery_extra_tick -> roll( priest.shadowy_recall_chance() ) )
      {
        devouring_plague_state_t* dps_t = static_cast< devouring_plague_state_t* >( d -> state );
        proc_spell -> orbs_used = dps_t -> orbs_used;
        proc_spell -> schedule_execute();
      }
    }
  };

  devouring_plague_dot_t* dot_spell;

  devouring_plague_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "devouring_plague", p, p.find_class_spell( "Devouring Plague" ) ),
    dot_spell( nullptr )
  {
    parse_options( NULL, options_str );

    parse_effect_data( data().effectN( 4 ) );

    base_td = num_ticks = 0;
    base_tick_time = timespan_t::zero();

    dot_spell = new devouring_plague_dot_t( p, this );
    add_child( dot_spell );
  }

  virtual void consume_resource()
  {
    resource_consumed = cost();

    if ( execute_state -> result != RESULT_MISS )
    {
      resource_consumed = priest.resources.current[ current_resource() ];
    }

    player -> resource_loss( current_resource(), resource_consumed, nullptr, this );

    if ( sim -> log )
      sim -> output( "%s consumes %.1f %s for %s (%.0f)", player -> name(),
                     resource_consumed, util::resource_type_string( current_resource() ),
                     name(), player -> resources.current[ current_resource() ] );

    stats -> consume_resource( current_resource(), resource_consumed );
  }

  virtual double action_da_multiplier()
  {
    double m = priest_spell_t::action_da_multiplier();

    m *= priest.resources.current[ current_resource() ];

    return m;
  }

  // Shortened and modified version of the ignite mechanic
  // Damage from the old dot is added to the new one.
  // Important here that the combined damage then will get multiplicated by the new orb amount.
  void trigger_dot( player_t* t )
  {
    dot_t* dot = dot_spell -> get_dot( t );

    double new_total_ignite_dmg = 0;
    assert( dot_spell -> num_ticks > 0 );

    if ( dot -> ticking )
    {
      new_total_ignite_dmg += dot_spell -> special_tick_dmg * dot -> ticks();
    }


    // Pass total amount of damage to the ignite dot_spell, and let it divide it by the correct number of ticks!

    action_state_t* s = dot_spell -> get_state();
    s -> target = t;
    dot_spell -> snapshot_state( s, dot_spell -> snapshot_flags, DMG_OVER_TIME );
    s -> result = RESULT_HIT;
    s -> result_amount = new_total_ignite_dmg;
    dot_spell -> schedule_travel( s );
    dot_spell -> stats -> add_execute( timespan_t::zero() );
  }

  virtual void impact( action_state_t* s )
  {
    priest_spell_t::impact( s );

    trigger_dot( s -> target );
  }
};

// Mind Flay Mastery Proc ===========================================
template <bool insanity>
struct mind_flay_mastery_t : public priest_procced_mastery_spell_t
{
  mind_flay_mastery_t( priest_t& p ) :
    priest_procced_mastery_spell_t( insanity ? "mind_flay_insanity_mastery" : "mind_flay_mastery", p,
                                    p.find_spell( insanity ? 124468 : 124468 ) ) // FIXME: Check if MF:I mastery has a different spell or not
  {
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = priest_spell_t::composite_target_multiplier( t );

    if ( insanity )
    {
      if ( priest.talents.power_word_solace -> ok() && td( t ).dots.devouring_plague_tick -> ticking )
      {
        const devouring_plague_state_t* dp_state = debug_cast<const devouring_plague_state_t*>( td( t ).dots.devouring_plague_tick -> state );
        m *= 1.0 + dp_state -> orbs_used / 3.0;
      }
    }

    return m;
  }
};

// Mind Flay Spell ==========================================================
template <bool insanity = false>
struct mind_flay_base_t : public priest_spell_t
{
  mind_flay_mastery_t<insanity>* proc_spell;

  mind_flay_base_t( priest_t& p, const std::string& options_str, const std::string& name = "mind_flay" ) :
    priest_spell_t( name, p, p.find_class_spell( insanity ? "Mind Flay (Insanity)" : "Mind Flay" ) ), // FIXME: adjust once spell data is available
    proc_spell( nullptr )
  {
    parse_options( NULL, options_str );

    may_crit     = false;
    channeled    = true;
    hasted_ticks = false;
    stormlash_da_multiplier = 0.0;
    stormlash_ta_multiplier = 1.0;

    if ( p.mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new mind_flay_mastery_t<insanity>( p );
      add_child( proc_spell );
    }
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    if ( proc_spell && priest.rngs.mastery_extra_tick -> roll( priest.shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }
  }
};

struct mind_flay_insanity_t : public mind_flay_base_t<true>
{
  typedef mind_flay_base_t<true> base_t;

  mind_flay_insanity_t( priest_t& p, const std::string& options_str ) :
    base_t( p, options_str, "mind_flay_insanity" )
    {
    }

  virtual double composite_target_multiplier( player_t* t )
  {
    double m = priest_spell_t::composite_target_multiplier( t );

    if ( priest.talents.power_word_solace -> ok() && td( t ).dots.devouring_plague_tick -> ticking )
    {
      const devouring_plague_state_t* dp_state = debug_cast<const devouring_plague_state_t*>( td( t ).dots.devouring_plague_tick -> state );
      m *= 1.0 + dp_state -> orbs_used / 3.0;
    }

    return m;
  }

  virtual bool ready()
  {
    if ( !priest.talents.power_word_solace -> ok() || ! td( target ).dots.devouring_plague_tick -> ticking )
      return false;

    return base_t::ready();
  }
};

// Shadow Word: Pain Mastery Proc ===========================================

struct shadow_word_pain_mastery_t : public priest_procced_mastery_spell_t
{
  shadow_word_pain_mastery_t( priest_t& p ) :
    priest_procced_mastery_spell_t( "shadow_word_pain_mastery", p,
                                    p.find_class_spell( "Shadow Word: Pain" ) -> ok() ? p.find_spell( 124464 ) : spell_data_t::not_found() )
  {
    // TO-DO: Confirm this applies
    base_crit   += p.sets -> set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    priest_procced_mastery_spell_t::impact( s );

    if ( ( s -> result == RESULT_CRIT ) && ( priest.specs.shadowy_apparitions -> ok() ) )
    {
      priest.shadowy_apparitions.trigger( *s );
    }
    if ( s -> result_amount > 0 )
    {
      priest.buffs.divine_insight_shadow -> trigger();
    }
  }
};

// Shadow Word Pain Spell ===================================================

struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_mastery_t* proc_spell;

  shadow_word_pain_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "shadow_word_pain", p, p.find_class_spell( "Shadow Word: Pain" ) ),
    proc_spell( nullptr )
  {
    parse_options( NULL, options_str );

    may_crit   = false;
    tick_zero  = true;

    base_multiplier *= 1.0 + p.sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();

    base_crit   += p.sets -> set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();

    num_ticks += ( int ) ( ( p.sets -> set( SET_T14_4PC_CASTER ) -> effectN( 1 ).base_value() / 1000.0 ) / base_tick_time.total_seconds() );

    if ( p.mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new shadow_word_pain_mastery_t( p );
      add_child( proc_spell );
    }
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    if ( ( d -> state -> result_amount > 0 ) && ( priest.specs.shadowy_apparitions -> ok() ) )
    {
      if ( d -> state -> result == RESULT_CRIT )
      {
        priest.shadowy_apparitions.trigger( *d -> state );
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
    if ( proc_spell && priest.rngs.mastery_extra_tick -> roll( priest.shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }
  }
};

// Vampiric Embrace Spell ===================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "vampiric_embrace", p, p.find_class_spell( "Vampiric Embrace" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest.buffs.vampiric_embrace -> trigger();
  }

  virtual bool ready()
  {
    if ( priest.buffs.vampiric_embrace -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Vampiric Touch Mastery Proc ===========================================

struct vampiric_touch_mastery_t : public priest_procced_mastery_spell_t
{
  rng_t* t15_4pc;

  vampiric_touch_mastery_t( priest_t& p ) :
    priest_procced_mastery_spell_t( "vampiric_touch_mastery", p,
                                    p.find_class_spell( "Vampiric Touch" ) -> ok() ? p.find_spell( 124465 ) : spell_data_t::not_found() )
  {
    if ( priest.set_bonus.tier15_4pc_caster() )
    {
      t15_4pc = player -> get_rng( "Tier15 4pc caster" );
    }
  }

  virtual void impact( action_state_t* s )
  {
    priest_procced_mastery_spell_t::impact( s );

    double m = player->resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();
    player -> resource_gain( RESOURCE_MANA, m, priest.gains.vampiric_touch_mana, this );

    if ( priest.buffs.surge_of_darkness -> trigger() )
    {
      priest.procs.surge_of_darkness -> occur();
    }

    if ( priest.set_bonus.tier15_4pc_caster() )
    {
      if ( ( s -> result_amount > 0 ) && ( priest.specs.shadowy_apparitions -> ok() ) )
      {
        if ( t15_4pc -> roll( priest.sets -> set( SET_T15_4PC_CASTER ) -> proc_chance() ) )
        {
          priest.procs.t15_4pc_caster -> occur();
          priest.shadowy_apparitions.trigger( *s );
        }
      }
    }
  }
};

// Vampiric Touch Spell =====================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_mastery_t* proc_spell;
  rng_t* t15_4pc;

  vampiric_touch_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "vampiric_touch", p, p.find_class_spell( "Vampiric Touch" ) ),
    proc_spell( nullptr )
  {
    parse_options( NULL, options_str );
    may_crit   = false;

    num_ticks += ( int ) ( ( p.sets -> set( SET_T14_4PC_CASTER ) -> effectN( 1 ).base_value() / 1000.0 ) / base_tick_time.total_seconds() );

    if ( priest.set_bonus.tier15_4pc_caster() )
    {
      t15_4pc = player -> get_rng( "Tier15 4pc caster" );
    }

    if ( p.mastery_spells.shadowy_recall -> ok() )
    {
      proc_spell = new vampiric_touch_mastery_t( p );
      add_child( proc_spell );
    }

  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    double m = player->resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();
    player -> resource_gain( RESOURCE_MANA, m, priest.gains.vampiric_touch_mana, this );

    if ( priest.buffs.surge_of_darkness -> trigger() )
    {
      priest.procs.surge_of_darkness -> occur();
    }

    if ( proc_spell && priest.rngs.mastery_extra_tick -> roll( priest.shadowy_recall_chance() ) )
    {
      proc_spell -> schedule_execute();
    }

    if ( priest.set_bonus.tier15_4pc_caster() )
    {
      if ( ( d -> state -> result_amount > 0 ) && ( priest.specs.shadowy_apparitions -> ok() ) )
      {
        if ( t15_4pc -> roll( priest.sets -> set( SET_T15_4PC_CASTER ) -> proc_chance() ) )
        {
          priest.procs.t15_4pc_caster -> occur();
          priest.shadowy_apparitions.trigger( *d -> state );
        }
      }
    }
  }
};

// Power Word: Solace Spell ==========================================================

struct power_word_solace_t : public priest_spell_t
{
  power_word_solace_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "power_word_solace", p, p.talents.power_word_solace  )
  {
    parse_options( NULL, options_str );

    can_cancel_shadowform = false; // FIXME: check in 5.2+
    castable_in_shadowform = false;

    can_trigger_atonement = true; // FIXME: check in 5.2+

    range += priest.glyphs.holy_fire -> effectN( 1 ).base_value();
  }

  virtual void execute()
  {
    priest.buffs.holy_evangelism -> up();

    priest_spell_t::execute();

    priest.buffs.holy_evangelism -> trigger();
  }

  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    m *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return m;
  }

  virtual double cost()
  {
    double c = priest_spell_t::cost();

    if ( priest.buffs.chakra_chastise -> check() )
      c *= 1.0 + priest.buffs.chakra_chastise -> data().effectN( 3 ).percent();

    c *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }

  virtual void impact( action_state_t* s )
  {
    priest_spell_t::impact( s );

    double amount = data().effectN( 2 ).percent() / 100.0 * priest.resources.max[ RESOURCE_MANA ];
    priest.resource_gain( RESOURCE_MANA, amount, priest.gains.power_word_solace );
  }
};

// Holy Fire Spell ==========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( priest_t& player, const std::string& options_str ) :
    priest_spell_t( "holy_fire", player, player.find_class_spell( "Holy Fire" ) )
  {
    parse_options( NULL, options_str );

    can_trigger_atonement = true;
    castable_in_shadowform = false;

    range += priest.glyphs.holy_fire -> effectN( 1 ).base_value();

    if ( priest.talents.power_word_solace -> ok() )
    {
      sim -> errorf( "Player %s: Power Word: Solace overrides Holy Fire if the talent is picked.\n"
                     "Please use it instead of Holy Fire.\n", player.name() );
      background = true;
    }
  }

  virtual void execute()
  {
    priest.buffs.holy_evangelism -> up();
    priest_spell_t::execute();
    priest.buffs.holy_evangelism -> trigger();
  }

  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    m *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return m;
  }

  virtual double cost()
  {
    double c = priest_spell_t::cost();

    if ( priest.buffs.chakra_chastise -> check() )
      c *= 1.0 + priest.buffs.chakra_chastise -> data().effectN( 3 ).percent();

    c *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }
};

// Penance Spell ============================================================

struct penance_t : public priest_spell_t
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

    void init() // override
    {
      priest_spell_t::init();
      if ( atonement )
        atonement -> stats = player -> get_stats( "atonement_penance" );
    }
  };

  penance_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "penance", p, p.find_class_spell( "Penance" ) )
  {
    parse_options( NULL, options_str );

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

    cooldown -> duration = data().cooldown() + p.sets -> set( SET_T14_4PC_HEAL ) -> effectN( 1 ).time_value();

    dynamic_tick_action = true;
    tick_action = new penance_tick_t( p, stats );
  }

  virtual void init()
  {
    priest_spell_t::init();
    if ( atonement )
      atonement -> channeled = true;
  }

  virtual bool usable_moving() { return priest.glyphs.penance -> ok(); }

  virtual double cost()
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    c *= 1.0 + priest.glyphs.penance -> effectN( 1 ).percent();

    return c;
  }

  virtual double action_multiplier()
  {
    double m = priest_spell_t::action_multiplier();

    m *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return m;
  }

  virtual void execute()
  {
    priest.buffs.holy_evangelism -> up();
    priest_spell_t::execute();
    priest.buffs.holy_evangelism -> trigger();
  }
};

// Smite Spell ==============================================================

struct smite_t : public priest_spell_t
{
  bool glyph_benefit;

  smite_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "smite", p, p.find_class_spell( "Smite" ) ),
    glyph_benefit( false )
  {
    parse_options( NULL, options_str );

    can_trigger_atonement = true;
    castable_in_shadowform = false;

    range += priest.glyphs.holy_fire -> effectN( 1 ).base_value();
  }

  virtual void execute()
  {
    priest.buffs.holy_evangelism -> up();
    priest.benefits.smites_with_glyph_increase -> update( glyph_benefit );

    priest_spell_t::execute();

    priest.buffs.holy_evangelism -> trigger();
    priest.buffs.surge_of_light -> trigger();

    // Train of Thought
    if ( priest.specs.train_of_thought -> ok() )
      priest.cooldowns.penance -> adjust ( - priest.specs.train_of_thought -> effectN( 2 ).time_value() );
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double m = priest_spell_t::composite_target_multiplier( target );

    bool glyph_benefit;

    if ( priest.talents.power_word_solace )
      glyph_benefit = priest.glyphs.smite -> ok() && td( target ).dots.power_word_solace -> ticking;
    else
      glyph_benefit = priest.glyphs.smite -> ok() && td( target ).dots.holy_fire -> ticking;

    if ( glyph_benefit )
      m *= 1.0 + priest.glyphs.smite -> effectN( 1 ).percent();

    return m;
  }

  virtual double action_multiplier()
  {
    double am = priest_spell_t::action_multiplier();

    am *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return am;
  }

  virtual double cost()
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
    ab::stormlash_da_multiplier = 0.0;

    ab::parse_options( nullptr, options_str );

    ab::parse_effect_data( scaling_data -> effectN( 1 ) ); // Parse damage or healing numbers from the scaling spell
    ab::school       = scaling_data -> get_school_type();
    ab::travel_speed = scaling_data -> missile_speed();
  }

  virtual action_state_t* new_state()
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

  virtual void execute()
  {
    // Clear and populate targets list
    targets.clear();
    populate_target_list();

    ab::execute();
  }

  virtual void impact( action_state_t* q )
  {
    ab::impact( q );

    cascade_state_t* cs = debug_cast<cascade_state_t*>( q );

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
            ab::sim -> output( "%s action %s jumps to player %s",
                               ab::player -> name(), ab::name(), t -> name() );


          // Copy-Pasted action_t::execute() code. Additionally increasing jump counter by one.
          cascade_state_t* s = debug_cast<cascade_state_t*>( ab::get_state() );
          s -> target = t;
          s -> jump_counter = cs -> jump_counter + 1;
          ab::snapshot_state( s, ab::snapshot_flags, q -> target -> is_enemy() ? DMG_DIRECT : HEAL_DIRECT );
          s -> result = ab::calculate_result( s );

          if ( ab:: result_is_hit( s -> result ) )
            s -> result_amount = ab::calculate_direct_damage( s -> result, 0, s -> attack_power, s -> spell_power, s -> composite_da_multiplier(), s -> target );

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

  virtual double composite_target_da_multiplier( player_t* t )
  {
    double ctdm = ab::composite_target_da_multiplier( t );

    // Source: Ghostcrawler 20/06/2012
    // http://us.battle.net/wow/en/forum/topic/5889309137?page=5#97

    double distance = ab::player -> current.distance; // Replace with whatever we measure distance
    if ( distance >= 30.0 )
      return ctdm;

    // 40% damage at 0 yards, 100% at 30, scaling linearly
    return ctdm * ( 0.4 + 0.6 * distance / 30.0 );
  }
};

struct cascade_damage_t : public cascade_base_t<priest_spell_t>
{
  cascade_damage_t( priest_t& p, const std::string& options_str ) :
    base_t( "cascade_damage", p, options_str, p.find_spell( p.specialization() == PRIEST_SHADOW ? 127628 : 120785 ) )
  {}

  virtual void populate_target_list()
  {
    for ( size_t i = 0; i < sim -> target_list.size(); ++i )
    {
      player_t* t = sim -> target_list[ i ];
      if ( t != target ) targets.push_back( t );
    }
  }
};

struct cascade_heal_t : public cascade_base_t<priest_heal_t>
{
  cascade_heal_t( priest_t& p, const std::string& options_str ) :
    base_t( "cascade_heal", p, options_str, p.find_spell( p.specialization() == PRIEST_SHADOW ? 127629 : 121148 ) )
  { }

  virtual void populate_target_list()
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
template <class Base, int scaling_effect_index>
struct halo_base_t : public Base
{
private:
  typedef Base ab; // typedef for the templated action type, priest_spell_t, or priest_heal_t
public:
  typedef halo_base_t base_t; // typedef for halo_base_t<ab>

  halo_base_t( const std::string& n, priest_t& p ) :
    ab( n, p, p.find_spell( p.specialization() == PRIEST_SHADOW ? 120696 : 120692 ) )
  {
    ab::aoe = -1;
    ab::background = true;

    if ( ab::data().ok() )
    {
      // Reparse the correct effect number, because we have two competing ones ( were 2 > 1 always wins out )
      ab::parse_effect_data( ab::data().effectN( scaling_effect_index ) );
    }
  }
  virtual ~halo_base_t() {}

  virtual double composite_target_da_multiplier( player_t* t )
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

struct halo_t : public priest_spell_t
{
  typedef halo_base_t<priest_spell_t, 2> halo_damage_t;
  typedef halo_base_t<priest_heal_t, 1> halo_heal_t;

  halo_damage_t* damage_spell;
  halo_heal_t* heal_spell;

  halo_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "halo", p, p.talents.halo -> ok() ? p.find_spell( p.specialization() == PRIEST_SHADOW ? 120644 : 120517 ) : spell_data_t::not_found() ),
    damage_spell( new halo_damage_t( "halo_damage", p ) ),
    heal_spell( new halo_heal_t( "halo_heal", p ) )
  {
    parse_options( nullptr, options_str );

    // Have the primary halo spell take on the stats that are most appropriate to the player's role
    // so it shows up nicely in the DPET chart.
    if ( p.primary_role() == ROLE_HEAL )
      add_child( heal_spell );
    else
      add_child( damage_spell );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    damage_spell -> execute();
    heal_spell -> execute();
  }
};

// Divine Star spell

template <class Base, int scaling_effect_index>
struct divine_star_base_t : public Base
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
    ab::stormlash_da_multiplier = 0.0;

    if ( ab::data().ok() )
    {
      // Reparse the correct effect number, because we have two competing ones ( were 2 > 1 always wins out )
      ab::parse_effect_data( ab::data().effectN( scaling_effect_index ) );
    }

    ab::proc = ab::background = true;
  }

  void execute() // override
  {
    ab::execute();
    if ( return_spell )
      return_spell -> execute();
  }

};

struct divine_star_t : public priest_spell_t
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

  virtual void execute()
  {
    priest_spell_t::execute();

    // Divine Star will damage and heal targets twice, once on the way out and
    // again on the way back. This is determined by distance from the target.
    // If we are too far away, it misses completely. If we are at the very
    // edge distance wise, it will only hit once. If we are within range (and
    // aren't moving such that it would miss the target on the way out and/or
    // back), it will hit twice. Threshold is 30 yards, per tooltip and tests
    // done by Spinalcrack for his HaloPro addon, for 2 hits. 35 yards is the
    // threshold for 1 hit.

    double distance = priest_spell_t::player -> current.distance;

    if ( distance <= 35 )
    {
      damage_spell -> execute();
      heal_spell -> execute();

      if ( distance <= 30 )
      {
        damage_spell -> execute();
        heal_spell -> execute();
      }
    }
  }
};

} // NAMESPACE spells

// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================

namespace heals {

// Binding Heal Spell =======================================================

struct binding_heal_t : public priest_heal_t
{
  binding_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "binding_heal", p, p.find_class_spell( "Binding Heal" ) )
  {
    parse_options( NULL, options_str );

    // FIXME: Should heal target & caster.
    aoe = 2;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    consume_inner_focus();
    trigger_serendipity();
    trigger_surge_of_light();
  }
};

// Circle of Healing ========================================================

struct circle_of_healing_t : public priest_heal_t
{
  circle_of_healing_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "circle_of_healing", p, p.find_class_spell( "Circle of Healing" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p.glyphs.circle_of_healing -> effectN( 2 ).percent();
    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );
    aoe = p.glyphs.circle_of_healing -> ok() ? 6 : 5;
  }

  virtual void execute()
  {
    cooldown -> duration = data().cooldown();
    cooldown -> duration += priest.sets -> set( SET_T14_4PC_HEAL ) -> effectN( 2 ).time_value();
    if ( priest.buffs.chakra_sanctuary -> up() )
      cooldown -> duration += priest.buffs.chakra_sanctuary -> data().effectN( 2 ).time_value();

    // Choose Heal Target
    target = find_lowest_player();

    priest_heal_t::execute();
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

// Desperate Prayer ===================

struct desperate_prayer_t : public priest_heal_t
{
  desperate_prayer_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "desperate_prayer", p, p.talents.desperate_prayer )
  {
    parse_options( 0, options_str );

    target = &p; // always targets the priest himself
  }
};

// Divine Hymn Spell ========================================================

struct divine_hymn_tick_t : public priest_heal_t
{
  divine_hymn_tick_t( priest_t& player, int nr_targets ) :
    priest_heal_t( "divine_hymn_tick", player, player.find_spell( 64844 ) )
  {
    background  = true;

    aoe = nr_targets;
  }
};

struct divine_hymn_t : public priest_heal_t
{
  divine_hymn_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "divine_hymn", p, p.find_class_spell( "Divine Hymn" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    channeled = true;
    dynamic_tick_action = true;

    tick_action = new divine_hymn_tick_t( p, data().effectN( 2 ).base_value() );
    add_child( tick_action );
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

// Echo of Light

struct echo_of_light_t : public ignite::pct_based_action_t<priest_heal_t>
{
  echo_of_light_t( priest_t& p ) :
    base_t( "echo_of_light", p, p.find_spell( 77489 ) )
  {
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks      = as<int>( data().duration() / base_tick_time );
  }
};

// Flash Heal Spell =========================================================

struct flash_heal_t : public priest_heal_t
{
  flash_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "flash_heal", p, p.find_class_spell( "Flash Heal" ) )
  {
    parse_options( NULL, options_str );
    can_trigger_spirit_shell = true;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    consume_inner_focus();
    trigger_serendipity();
    consume_surge_of_light();
    trigger_surge_of_light();
  }

  virtual void impact( action_state_t* s )
  {
    priest_heal_t::impact( s );

    trigger_grace( s -> target );

    if ( ! priest.buffs.spirit_shell -> check() )
      trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit()
  {
    double cc = priest_heal_t::composite_crit();

    if ( priest.buffs.inner_focus -> check() )
      cc += priest.buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual timespan_t execute_time()
  {
    if ( priest.buffs.surge_of_light -> check() )
      return timespan_t::zero();

    return priest_heal_t::execute_time();
  }

  virtual double cost()
  {
    if ( priest.buffs.surge_of_light -> check() )
      return 0;

    double c = priest_heal_t::cost();

    if ( priest.buffs.inner_focus -> check() )
      c *= 1.0 + priest.buffs.inner_focus -> data().effectN( 1 ).percent();

    c *= 1.0 + priest.sets -> set( SET_T14_2PC_HEAL ) -> effectN( 1 ).percent();

    return c;
  }
};

// Guardian Spirit ==========================================================

struct guardian_spirit_t : public priest_heal_t
{
  guardian_spirit_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "guardian_spirit", p, p.find_class_spell( "Guardian Spirit" ) )
  {
    parse_options( NULL, options_str );

    // The absorb listed isn't a real absorb
    base_dd_min = base_dd_max = 0;

    harmful = false;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    target -> buffs.guardian_spirit -> trigger();
  }
};

// Greater Heal Spell =======================================================

struct greater_heal_t : public priest_heal_t
{
  greater_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "greater_heal", p, p.find_class_spell( "Greater Heal" ) )
  {
    parse_options( NULL, options_str );
    can_trigger_spirit_shell = true;
  }

  virtual void execute()
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

  virtual void impact( action_state_t* s )
  {
    priest_heal_t::impact( s );

    trigger_grace( s -> target );
    if ( ! priest.buffs.spirit_shell -> check() )
      trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit()
  {
    double cc = priest_heal_t::composite_crit();

    if ( priest.buffs.inner_focus -> check() )
      cc += priest.buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    if ( priest.buffs.inner_focus -> check() )
      c *= 1.0 + priest.buffs.inner_focus -> data().effectN( 1 ).percent();

    if ( priest.buffs.serendipity -> check() )
      c *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 2 ).percent();

    return c;
  }

  virtual timespan_t execute_time()
  {
    timespan_t et = priest_heal_t::execute_time();

    if ( priest.buffs.serendipity -> check() )
      et *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 1 ).percent();

    return et;
  }
};

// Heal Spell ===============================================================

struct _heal_t : public priest_heal_t
{
  _heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "heal", p, p.find_class_spell( "Heal" ) )
  {
    parse_options( NULL, options_str );
    can_trigger_spirit_shell = true;
  }

  virtual void execute()
  {
    priest_heal_t::execute();
    trigger_surge_of_light();
  }

  virtual void impact( action_state_t* s )
  {
    priest_heal_t::impact( s );

    trigger_grace( s -> target );
    if ( ! priest.buffs.spirit_shell -> check() )
      trigger_strength_of_soul( s -> target );
  }
};

// Holy Word Sanctuary ======================================================

struct holy_word_sanctuary_t : public priest_heal_t
{
  struct holy_word_sanctuary_tick_t : public priest_heal_t
  {
    holy_word_sanctuary_tick_t( priest_t& player ) :
      priest_heal_t( "holy_word_sanctuary_tick", player, player.find_spell( 88686 ) )
    {
      dual        = true;
      background  = true;
      direct_tick = true;
      can_trigger_EoL = false; // http://us.battle.net/wow/en/forum/topic/5889309137?page=107#2137
    }

    virtual double action_multiplier()
    {
      double am = priest_heal_t::action_multiplier();

      if ( priest.buffs.chakra_sanctuary -> up() )
        am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

      return am;
    }
  };

  holy_word_sanctuary_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "holy_word_sanctuary", p, p.find_spell( 88685 ) )
  {
    parse_options( NULL, options_str );

    may_crit     = false;

    base_tick_time = timespan_t::from_seconds( 2.0 );
    num_ticks = 9;

    tick_action = new holy_word_sanctuary_tick_t( p );

    // Needs testing
    cooldown -> duration *= 1.0 + p.set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual bool ready()
  {
    if ( ! priest.buffs.chakra_sanctuary -> check() )
      return false;

    return priest_heal_t::ready();
  }

  // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility
    // Implemented 06/12/2011 ( Patch 4.3 ),
    // see Issue1023 and http://elitistjerks.com/f77/t110245-cataclysm_holy_priest_compendium/p25/#post2054467

    c *= 1.0 + priest.buffs.inner_will -> check() * priest.buffs.inner_will -> data().effectN( 1 ).percent();
    c  = floor( c );

    return c;
  }

  virtual void consume_resource()
  {
    priest_heal_t::consume_resource();

    priest.buffs.inner_will -> up();
  }
};

// Holy Word Chastise =======================================================

struct holy_word_chastise_t : public priest_spell_t
{
  holy_word_chastise_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "holy_word_chastise", p, p.find_class_spell( "Holy Word: Chastise" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p.set_bonus.tier13_4pc_heal() * -0.2;

    castable_in_shadowform = false;
  }

  virtual bool ready()
  {
    if ( priest.buffs.chakra_sanctuary -> check() )
      return false;

    if ( priest.buffs.chakra_serenity -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Holy Word Serenity =======================================================

struct holy_word_serenity_t : public priest_heal_t
{
  holy_word_serenity_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "holy_word_serenity", p, p.find_spell( 88684 ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p.set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual void impact( action_state_t* s )
  {
    priest_heal_t::impact( s );

    td( s -> target ).buffs.holy_word_serenity -> trigger();
  }

  virtual bool ready()
  {
    if ( ! priest.buffs.chakra_serenity -> check() )
      return false;

    return priest_heal_t::ready();
  }
};

// Holy Word ================================================================

struct holy_word_t : public priest_spell_t
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

  virtual void init()
  {
    priest_spell_t::init();

    hw_sanctuary -> action_list = action_list;
    hw_chastise -> action_list = action_list;
    hw_serenity -> action_list = action_list;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
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

  virtual void execute()
  {
    assert( false );
  }

  virtual bool ready()
  {
    if ( priest.buffs.chakra_serenity -> check() )
      return hw_serenity -> ready();

    else if ( priest.buffs.chakra_sanctuary -> check() )
      return hw_sanctuary -> ready();

    else
      return hw_chastise -> ready();
  }
};

// Lightwell Spell ==========================================================

struct lightwell_t : public priest_spell_t
{
  timespan_t consume_interval;

  lightwell_t( priest_t& p, const std::string& options_str ) :
    priest_spell_t( "lightwell", p, p.find_class_spell( "Lightwell" ) ),
    consume_interval( timespan_t::from_seconds( 10 ) )
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

  virtual void execute()
  {
    priest_spell_t::execute();

    priest.pets.lightwell -> get_cooldown( "lightwell_renew" ) -> duration = consume_interval;
    priest.pets.lightwell -> summon( data().duration() );
  }
};

// Penance Heal Spell =======================================================

struct penance_heal_t : public priest_heal_t
{
  struct penance_heal_tick_t : public priest_heal_t
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

    virtual void impact( action_state_t* s )
    {
      priest_heal_t::impact( s );

      trigger_grace( s -> target );
    }
  };

  penance_heal_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "penance_heal", p, p.find_class_spell( "Penance" ) )
  {
    parse_options( NULL, options_str );

    may_crit       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;
    dynamic_tick_action = true;

    cooldown = p.cooldowns.penance;
    cooldown -> duration = data().cooldown() + p.sets -> set( SET_T14_4PC_HEAL ) -> effectN( 1 ).time_value();

    tick_action = new penance_heal_tick_t( p );
  }

  virtual void init()
  {
    priest_heal_t::init();

    tick_action -> stats = stats;
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    c *= 1.0 + ( priest.buffs.holy_evangelism -> check() * priest.buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    c *= 1.0 + priest.glyphs.penance -> effectN( 1 ).percent();

    return c;
  }
};

// Power Word: Shield Spell =================================================

struct power_word_shield_t : public priest_absorb_t
{
  struct glyph_power_word_shield_t : public priest_heal_t
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
  int ignore_debuff;

  power_word_shield_t( priest_t& p, const std::string& options_str ) :
    priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ),
    glyph_pws( nullptr ),
    ignore_debuff( 0 )
  {
    option_t options[] =
    {
      opt_bool( "ignore_debuff", ignore_debuff ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( p.specs.rapture -> ok() )
      cooldown -> duration = timespan_t::zero();

    // Tooltip is wrong.
    // direct_power_mod = 0.87; // hardcoded into tooltip
    direct_power_mod = 1.8709; // matches in-game actual value

    if ( p.glyphs.power_word_shield -> ok() )
    {
      glyph_pws = new glyph_power_word_shield_t( p );
      //add_child( glyph_pws );
    }

    castable_in_shadowform = true;
  }

  virtual void impact( action_state_t* s )
  {
    s -> target -> buffs.weakened_soul -> trigger();
    priest.buffs.borrowed_time -> trigger();

    // Glyph
    if ( glyph_pws )
      glyph_pws -> trigger( s );

    td( s -> target ).buffs.power_word_shield -> trigger( 1, s -> result_amount );
    stats -> add_result( 0, s -> result_amount, ABSORB, s -> result );
  }

  virtual void consume_resource()
  {
    priest_absorb_t::consume_resource();

    // Rapture
    if ( priest.cooldowns.rapture -> up() && priest.specs.rapture -> ok() )
    {
      player -> resource_gain( RESOURCE_MANA, player -> spirit() * priest.specs.rapture -> effectN( 1 ).percent(), priest.gains.rapture );
      priest.cooldowns.rapture -> start();
    }
  }

  virtual bool ready()
  {
    if ( ! ignore_debuff && target -> buffs.weakened_soul -> check() )
      return false;

    return priest_absorb_t::ready();
  }
};

// Prayer of Healing Spell ==================================================

struct prayer_of_healing_t : public priest_heal_t
{
  prayer_of_healing_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_healing", p, p.find_class_spell( "Prayer of Healing" ) )
  {
    parse_options( NULL, options_str );
    aoe = 5;
    group_only = true;
    divine_aegis_trigger_mask = RESULT_HIT_MASK;
    can_trigger_spirit_shell = true;
  }

  virtual void execute()
  {
    priest_heal_t::execute();
    consume_inner_focus();
    consume_serendipity();
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual double composite_crit()
  {
    double cc = priest_heal_t::composite_crit();

    if ( priest.buffs.inner_focus -> check() )
      cc += priest.buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double cost()
  {
    double c = priest_heal_t::cost();

    if ( priest.buffs.inner_focus -> check() )
      c *= 1.0 + priest.buffs.inner_focus -> data().effectN( 1 ).percent();

    if ( priest.buffs.serendipity -> check() )
      c *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 2 ).percent();

    return c;
  }

  virtual timespan_t execute_time()
  {
    timespan_t et = priest_heal_t::execute_time();

    if ( priest.buffs.serendipity -> check() )
      et *= 1.0 + priest.buffs.serendipity -> check() * priest.buffs.serendipity -> data().effectN( 1 ).percent();

    return et;
  }
};

// Prayer of Mending Spell ==================================================

struct prayer_of_mending_t : public priest_heal_t
{
  int single;

  prayer_of_mending_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_mending", p, p.find_class_spell( "Prayer of Mending" ) ),
    single( 0 )
  {
    option_t options[] =
    {
      opt_bool( "single", single ),
      opt_null()
    };
    parse_options( options, options_str );

    direct_power_mod = data().effectN( 1 ).coeff();
    base_dd_min = base_dd_max = data().effectN( 1 ).min( &p );

    divine_aegis_trigger_mask = 0;

    aoe = 5;

    castable_in_shadowform = p.glyphs.dark_binding -> ok();
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = priest_heal_t::composite_target_multiplier( t );

    if ( priest.glyphs.prayer_of_mending -> ok() && t == target )
      ctm *= 1.0 + priest.glyphs.prayer_of_mending -> effectN( 1 ).percent();

    return ctm;
  }
};

// Renew Spell ==============================================================

struct renew_t : public priest_heal_t
{
  struct rapid_renewal_t : public priest_heal_t
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

    virtual double composite_da_multiplier()
    { return 1.0; }
  };
  rapid_renewal_t* rr;

  renew_t( priest_t& p, const std::string& options_str ) :
    priest_heal_t( "renew", p, p.find_class_spell( "Renew" ) ),
    rr( nullptr )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    if ( p.specs.rapid_renewal -> ok() )
    {
      rr = new rapid_renewal_t( p );
      add_child( rr );
      trigger_gcd += p.specs.rapid_renewal -> effectN( 1 ).time_value();
      base_multiplier *= 1.0 + p.specs.rapid_renewal -> effectN( 2 ).percent();
    }

    base_multiplier *= 1.0 + p.glyphs.renew -> effectN( 1 ).percent();
    num_ticks       += ( int ) ( p.glyphs.renew -> effectN( 2 ).time_value() / base_tick_time );

    castable_in_shadowform = p.glyphs.dark_binding -> ok();
  }

  virtual double action_multiplier()
  {
    double am = priest_heal_t::action_multiplier();

    if ( priest.buffs.chakra_sanctuary -> up() )
      am *= 1.0 + priest.buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    priest_heal_t::impact( s );

    if ( rr )
    {
      dot_t* d = get_dot( s -> target );
      double tick_dmg = calculate_tick_damage( RESULT_HIT, d -> state -> composite_power(), d -> state -> composite_ta_multiplier(), d -> state -> target );
      tick_dmg *= d -> ticks(); // Gets multiplied by the hasted amount of ticks
      rr -> trigger( s, tick_dmg );
    }
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
protected:
  priest_t& priest;
public:
  typedef priest_buff_t base_t; // typedef for priest_buff_t<buff_base_t>

  virtual ~priest_buff_t() {}
  priest_buff_t( priest_t& p, const buff_creator_basics_t& params ) :
    Base( params ), priest( p )
  { }
};

/* Custom shadowform buff
 * trigger/cancels spell haste aura
 */
struct shadowform_t : public priest_buff_t<buff_t>
{
  shadowform_t( priest_t& p ) :
    base_t( p, buff_creator_t( &p, "shadowform" ).spell( p.find_class_spell( "Shadowform" ) ) )
  { }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    bool r = base_t::trigger( stacks, value, chance, duration );

    if ( ! sim -> overrides.spell_haste )
      sim -> auras.spell_haste -> trigger();

    return r;
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    if ( ! sim -> overrides.spell_haste )
      sim -> auras.spell_haste -> expire();
  }
};

/* Custom archangel buff
 * snapshots evangelism stacks and expires it
 */
struct archangel_t : public priest_buff_t<buff_t>
{
  archangel_t( priest_t& p ) :
    base_t( p, buff_creator_t( &p, "archangel" ).spell( p.specs.archangel ).max_stack( 5 ) )
  {
    default_value = data().effectN( 1 ).percent();
  }

  virtual bool trigger( int stacks, double /* value */, double chance, timespan_t duration )
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
struct divine_insight_shadow_t : public priest_buff_t<buff_t>
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

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
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

} // end namespace buffs


// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p ) :
  actor_pair_t( target, &p ),
  dots( dots_t() ),
  buffs( buffs_t() )
{
  dots.holy_fire             = target -> get_dot( "holy_fire",             &p );
  dots.power_word_solace     = target -> get_dot( "power_word_solace",     &p );
  dots.devouring_plague_tick = target -> get_dot( "devouring_plague_tick", &p );
  dots.shadow_word_pain      = target -> get_dot( "shadow_word_pain",      &p );
  dots.vampiric_touch        = target -> get_dot( "vampiric_touch",        &p );

  dots.renew = target -> get_dot( "renew", &p );

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
// Priest Shadowy Apparition Definitions
// ==========================================================================

/* Trigger a new Shadowy Apparition
 *
 */
void priest_t::shadowy_apparitions_t::trigger( action_state_t& d )
{
  if ( ! priest.specs.shadowy_apparitions -> ok() )
    return;

  // Check if there are already 4 active SA
  if ( apparitions_active.size() >= 4 )
  {
    // If there are already 4 SA's active, they will get added to a queue
    // Added 11. March 2013, see http://howtopriest.com/viewtopic.php?f=8&t=3242
    targets_queued.push_back( d.target );

    if ( priest.sim -> debug )
      priest.sim -> output( "%s added shadowy apparition to the queue. Active SA: %d, Queued SA: %d",
          priest.name(), as<unsigned>( apparitions_active.size() ), as<unsigned>( targets_queued.size() ) );
  }
  else if ( ! apparitions_free.empty() ) // less than 4 active SA
  {
    // Get SA spell, set target and remove it from free list
    sa_spell* s = apparitions_free.back();
    s -> target = d.target;
    apparitions_free.pop_back();

    // Add to active SA spell list
    apparitions_active.push_back( s );

    if ( priest.sim -> debug )
      priest.sim -> output( "%s triggered shadowy apparition. Active SA: %d, Queued SA: %d",
          priest.name(), as<unsigned>( apparitions_active.size() ), as<unsigned>( targets_queued.size() ) );

    // Execute
    priest.procs.shadowy_apparition -> occur();
    s -> execute();
  }
  else
  {
    assert( false && "not enough free shadowy apparition spells available" );
  }
}

/* Cleanup executed SA action.
 * - Remove from active list.
 * - Add to free list.
 */
void priest_t::shadowy_apparitions_t::tidy_up( actions::spells::shadowy_apparition_spell_t& s )
{
  std::vector<sa_spell*>::iterator it = range::find( apparitions_active, &s );
  assert( it != apparitions_active.end() );
  apparitions_active.erase( it );
  apparitions_free.push_back( &s );
}

void priest_t::shadowy_apparitions_t::add_more( size_t num )
{
  for ( size_t i = 0; i < num; i++ )
  {
    apparitions_free.push_back( new sa_spell( priest ) );
  }

  if ( priest.sim -> debug )
    priest.sim -> output( "%s created %d shadowy apparitions. %d free shadowy apparitions available.",
        priest.name(), num, as<unsigned>( apparitions_free.size() ) );
}

/* Start SA from queue
 * Added 11. March 2013, see http://howtopriest.com/viewtopic.php?f=8&t=3242
 */
void priest_t::shadowy_apparitions_t::start_from_queue()
{
  if ( ! targets_queued.empty() )
  {
    if ( ! apparitions_free.empty() )
    {
      // Get SA spell from free list
      sa_spell* s = apparitions_free.back();
      apparitions_free.pop_back();

      // Set SA spell target to the target saved in the queue
      s -> target = targets_queued.front();
      targets_queued.erase( targets_queued.begin() );

      // Add to active SA spell list
      apparitions_active.push_back( s );

      if ( priest.sim -> debug )
        priest.sim -> output( "%s triggered shadowy apparition from the queue. Active SA: %d, Queued SA: %d",
          priest.name(), as<unsigned>( apparitions_active.size() ), as<unsigned>( targets_queued.size() ) );

      // Execute
      priest.procs.shadowy_apparition -> occur();
      s -> execute();
    }
    else
    {
      assert( false && "not enough free shadowy apparition spells available" );
    }
  }
}

/* Reset Shadowy Apparition Lists
 * - Move contents of "active" list to the "free" list
 * - Clear queue
 */
void priest_t::shadowy_apparitions_t::reset()
{
  apparitions_free.insert( apparitions_free.end(), apparitions_active.begin(), apparitions_active.end() );
  apparitions_active.clear();

  targets_queued.clear();
}

// ==========================================================================
// Priest Definitions
// ==========================================================================

/* Construct priest cooldowns
 *
 */
void priest_t::create_cooldowns()
{
  cooldowns.mind_blast  = get_cooldown( "mind_blast" );
  cooldowns.shadowfiend = get_cooldown( "shadowfiend" );
  cooldowns.mindbender  = get_cooldown( "mindbender" );
  cooldowns.chakra      = get_cooldown( "chakra" );
  cooldowns.inner_focus = get_cooldown( "inner_focus" );
  cooldowns.penance     = get_cooldown( "penance" );
  cooldowns.rapture     = get_cooldown( "rapture" );
}

/* Construct priest gains
 *
 */
void priest_t::create_gains()
{
  gains.dispersion                    = get_gain( "dispersion" );
  gains.shadowfiend                   = get_gain( "shadowfiend" );
  gains.mindbender                    = get_gain( "mindbender" );
  gains.hymn_of_hope                  = get_gain( "hymn_of_hope" );
  gains.power_word_solace             = get_gain( "Power Word: Solace Mana" );
  gains.shadow_orb_mb                 = get_gain( "Shadow Orbs from Mind Blast" );
  gains.shadow_orb_swd                = get_gain( "Shadow Orbs from Shadow Word: Death" );
  gains.devouring_plague_health       = get_gain( "Devouring Plague Health" );
  gains.vampiric_touch_mana           = get_gain( "Vampiric Touch Mana" );
  gains.vampiric_touch_mastery_mana   = get_gain( "Vampiric Touch Mastery Mana" );
  gains.rapture                       = get_gain( "Rapture" );
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

void priest_t::create_benefits()
{
  benefits.smites_with_glyph_increase = get_benefit( "Smites increased by Holy Fire" );
}

// priest_t::primary_role ===================================================

role_e priest_t::primary_role()
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

// priest_t::combat_begin ===================================================

void priest_t::combat_begin()
{
  base_t::combat_begin();

  if ( initial_shadow_orbs > 0 )
    resources.current[ RESOURCE_SHADOW_ORB ] = std::min( ( double ) initial_shadow_orbs, resources.base[ RESOURCE_SHADOW_ORB ] );
}

// priest_t::composite_armor ================================================

double priest_t::composite_armor()
{
  double a = base_t::composite_armor();

  if ( buffs.inner_fire -> up() )
  {
    a *= 1.0 + buffs.inner_fire -> data().effectN( 1 ).percent();
    a *= 1.0 + glyphs.inner_fire -> effectN( 1 ).percent();
  }

  return floor( a );
}

// priest_t::composite_spell_haste ==========================================

double priest_t::composite_spell_haste()
{
  double h = player_t::composite_spell_haste();

  if ( buffs.power_infusion -> up() )
    h /= 1.0 + buffs.power_infusion -> data().effectN( 1 ).percent();

  if ( buffs.borrowed_time -> up() )
    h /= 1.0 + buffs.borrowed_time -> data().effectN( 1 ).percent();

  return h;
}

// priest_t::composite_spell_power_multiplier ===============================

double priest_t::composite_spell_power_multiplier()
{
  double m = 1.0;

  if ( buffs.inner_fire -> up() )
    m += buffs.inner_fire -> data().effectN( 2 ).percent();

  if ( sim -> auras.spell_power_multiplier -> check() )
    m += sim -> auras.spell_power_multiplier -> value();

  m *= current.spell_power_multiplier;

  return m;
}

// priest_t::composite_spell_hit ============================================

double priest_t::composite_spell_hit()
{
  double hit = base_t::composite_spell_hit();

  hit += specs.divine_fury -> effectN( 1 ).percent();
  hit += ( ( spirit() - base.attribute[ ATTR_SPIRIT ] ) * specs.spiritual_precision -> effectN( 1 ).percent() ) / rating.spell_hit;

  return hit;
}

// priest_t::composite_attack_hit ============================================

double priest_t::composite_attack_hit()
{
  double hit = base_t::composite_attack_hit();

  hit += ( ( spirit() - base.attribute[ ATTR_SPIRIT ] ) * specs.spiritual_precision -> effectN( 1 ).percent() ) / rating.spell_hit;

  return hit;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = base_t::composite_player_multiplier( school, a );

  if ( dbc::is_school( SCHOOL_SHADOW, school ) )
  {
    m *= 1.0 + buffs.shadowform -> check() * specs.shadowform -> effectN( 2 ).percent();
  }

  if ( dbc::is_school( SCHOOL_SHADOWLIGHT, school ) )
  {
    if ( buffs.chakra_chastise -> up() )
    {
      m *= 1.0 + buffs.chakra_chastise -> data().effectN( 1 ).percent();
    }
  }

  if ( buffs.power_infusion -> up() )
  {
    m *= 1.0 + buffs.power_infusion -> data().effectN( 3 ).percent();
  }

  if ( buffs.twist_of_fate -> check() )
  {
    m *= 1.0 + buffs.twist_of_fate -> value();
  }

  return m;
}

// priest_t::composite_player_heal_multiplier ===============================

double priest_t::composite_player_heal_multiplier( school_e s )
{
  double m = player_t::composite_player_heal_multiplier( s );

  if ( buffs.twist_of_fate -> check() )
  {
    m *= 1.0 + buffs.twist_of_fate -> value();
  }

  // FIXME: Nothing in {heal_t|absorb_t} is actually using composite_player_{heal|absorb}_multiplier().

  return m;
}

// priest_t::composite_movement_speed =======================================

double priest_t::composite_movement_speed()
{
  double speed = base_t::composite_movement_speed();

  if ( buffs.inner_will -> up() )
    speed *= 1.0 + buffs.inner_will -> data().effectN( 2 ).percent() + glyphs.inner_sanctum -> effectN( 2 ).percent();

  return speed;
}

// priest_t::composite_attribute_multiplier =================================

double priest_t::composite_attribute_multiplier( attribute_e attr )
{
  double m = base_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_INTELLECT )
    m *= 1.0 + specs.mysticism -> effectN( 1 ).percent();

  return m;
}

// priest_t::matching_gear_multiplier =======================================

double priest_t::matching_gear_multiplier( attribute_e attr )
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
  if ( name == "hymn_of_hope"           ) return new hymn_of_hope_t          ( *this, options_str );
  if ( name == "inner_fire"             ) return new inner_fire_t            ( *this, options_str );
  if ( name == "inner_focus"            ) return new inner_focus_t           ( *this, options_str );
  if ( name == "inner_will"             ) return new inner_will_t            ( *this, options_str );
  if ( name == "pain_suppression"       ) return new pain_suppression_t      ( *this, options_str );
  if ( name == "power_infusion"         ) return new power_infusion_t        ( *this, options_str );
  if ( name == "shadowform"             ) return new shadowform_t            ( *this, options_str );
  if ( name == "vampiric_embrace"       ) return new vampiric_embrace_t      ( *this, options_str );
  if ( name == "spirit_shell"           ) return new spirit_shell_t          ( *this, options_str );

  // Damage
  if ( name == "devouring_plague"       ) return new devouring_plague_t      ( *this, options_str );
  if ( name == "holy_fire"              ) return new holy_fire_t             ( *this, options_str );
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
  if ( name == "vampiric_touch"         ) return new vampiric_touch_t        ( *this, options_str );
  if ( name == "power_word_solace"      ) return new power_word_solace_t     ( *this, options_str );
  if ( name == "cascade_damage"         ) return new cascade_damage_t        ( *this, options_str );
  if ( name == "halo"                   ) return new halo_t                  ( *this, options_str );
  if ( name == "divine_star"            ) return new divine_star_t           ( *this, options_str );

  // Heals
  if ( name == "binding_heal"           ) return new binding_heal_t          ( *this, options_str );
  if ( name == "circle_of_healing"      ) return new circle_of_healing_t     ( *this, options_str );
  if ( name == "divine_hymn"            ) return new divine_hymn_t           ( *this, options_str );
  if ( name == "flash_heal"             ) return new flash_heal_t            ( *this, options_str );
  if ( name == "greater_heal"           ) return new greater_heal_t          ( *this, options_str );
  if ( name == "guardian_spirit"        ) return new guardian_spirit_t       ( *this, options_str );
  if ( name == "heal"                   ) return new _heal_t                 ( *this, options_str );
  if ( name == "holy_word"              ) return new holy_word_t             ( *this, options_str );
  if ( name == "lightwell"              ) return new lightwell_t             ( *this, options_str );
  if ( name == "penance_heal"           ) return new penance_heal_t          ( *this, options_str );
  if ( name == "power_word_shield"      ) return new power_word_shield_t     ( *this, options_str );
  if ( name == "prayer_of_healing"      ) return new prayer_of_healing_t     ( *this, options_str );
  if ( name == "prayer_of_mending"      ) return new prayer_of_mending_t     ( *this, options_str );
  if ( name == "renew"                  ) return new renew_t                 ( *this, options_str );
  if ( name == "cascade_heal"           ) return new cascade_heal_t          ( *this, options_str );

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
  pets.lightwell        = create_pet( "lightwell"   );
}

// priest_t::init_base ======================================================

void priest_t::init_base()
{
  base_t::init_base();

  base.attack_power = 0.0;

  if ( specs.shadow_orbs -> ok() )
    resources.base[ RESOURCE_SHADOW_ORB ] = 3;

  initial.attack_power_per_strength = 1.0;
  initial.spell_power_per_intellect = 1.0;

  diminished_kfactor   = 0.009830;
  diminished_dodge_cap = 0.006650;
  diminished_parry_cap = 0.006650;

#if 0
// New Item Testing

  std::string item_opt = "myitem,stats=50int,id=81692,gem_id2=76694";
  new_item_stuff::item_t* foo = new new_item_stuff::item_t( *this, item_opt );
  std::cout << "\n name " << foo -> name() << " id=" << foo -> id() << "\n";
  std::cout << "stat int = " << foo -> get_stat( STAT_INTELLECT ) << "\n";
  delete foo;
  ( void ) foo;

  std::cout << "sizeof(new_item_stuff::item_t): " << sizeof( new_item_stuff::base_item_t ) << "\n";

// End New Item Testing
#endif
}

// priest_t::init_scaling ===================================================

void priest_t::init_scaling()
{
  base_t::init_scaling();

  // Atonement heals are capped at a percentage of the Priest's health,
  // so there may be scaling with stamina.
  if ( specs.atonement -> ok() && primary_role() == ROLE_HEAL )
    scales_with[ STAT_STAMINA ] = true;

  // Disc/Holy are hitcapped vs. raid bosses by Divine Fury
  if ( specs.divine_fury -> ok() )
    scales_with[ STAT_HIT_RATING ] = false;

  // For a Shadow Priest Spirit is the same as Hit Rating so invert it.
  if ( ( specs.spiritual_precision -> ok() ) && ( sim -> scaling -> scale_stat == STAT_SPIRIT ) )
  {
    double v = sim -> scaling -> scale_value;

    if ( ! sim -> scaling -> positive_scale_delta )
    {
      invert_scaling = 1;
      initial.attribute[ ATTR_SPIRIT ] -= v * 2;
    }
  }
}

// priest_t::init_spells ===================

void priest_t::init_spells()
{
  base_t::init_spells();

  // Talents
  talents.void_tendrils               = find_talent_spell( "Void Tendrils" );
  talents.psyfiend                    = find_talent_spell( "Psyfiend" );
  talents.dominate_mind               = find_talent_spell( "Dominate Mind" );
  talents.body_and_soul               = find_talent_spell( "Body and Soul" );
  talents.angelic_feather             = find_talent_spell( "Angelic Feather" );
  talents.phantasm                    = find_talent_spell( "Phantasm" );
  talents.from_darkness_comes_light   = find_talent_spell( "From Darkness, Comes Light" );
  talents.mindbender                  = find_talent_spell( "Mindbender" );

  talents.power_word_solace           = find_talent_spell( "Solace and Insanity" );

  talents.desperate_prayer            = find_talent_spell( "Desperate Prayer" );
  talents.spectral_guise              = find_talent_spell( "Spectral Guise" );
  talents.angelic_bulwark             = find_talent_spell( "Angelic Bulwark" );
  talents.twist_of_fate               = find_talent_spell( "Twist of Fate" );
  talents.power_infusion              = find_talent_spell( "Power Infusion" );
  talents.divine_insight              = find_talent_spell( "Divine Insight" );
  talents.cascade                     = find_talent_spell( "Cascade" );
  talents.divine_star                 = find_talent_spell( "Divine Star" );
  talents.halo                        = find_talent_spell( "Halo" );

  // Passive Spells

  // General Spells

  // Discipline
  specs.atonement                      = find_specialization_spell( "Atonement" );
  specs.archangel                      = find_specialization_spell( "Archangel" );
  specs.borrowed_time                  = find_specialization_spell( "Borrowed Time" );
  specs.divine_aegis                   = find_specialization_spell( "Divine Aegis" );
  specs.divine_fury                    = find_specialization_spell( "Divine Fury" );
  specs.evangelism                     = find_specialization_spell( "Evangelism" );
  specs.grace                          = find_specialization_spell( "Grace" );
  specs.meditation_disc                = find_specialization_spell( "Meditation", "meditation_disc", PRIEST_DISCIPLINE );
  specs.mysticism                      = find_specialization_spell( "Mysticism" );
  specs.rapture                        = find_specialization_spell( "Rapture" );
  specs.spirit_shell                   = find_specialization_spell( "Spirit Shell" );
  specs.strength_of_soul               = find_specialization_spell( "Strength of Soul" );
  specs.train_of_thought               = find_specialization_spell( "Train of Thought" );

  // Holy
  specs.meditation_holy                = find_specialization_spell( "Meditation", "meditation_holy", PRIEST_HOLY );
  specs.serendipity                    = find_specialization_spell( "Serendipity" );
  specs.rapid_renewal                  = find_specialization_spell( "Rapid Renewal" );

  // Shadow
  specs.mind_surge                     = find_specialization_spell( "Mind Surge" );
  specs.spiritual_precision            = find_specialization_spell( "Spiritual Precision" );
  specs.shadowform                     = find_class_spell( "Shadowform" );
  specs.shadowy_apparitions            = find_specialization_spell( "Shadowy Apparitions" );
  specs.shadow_orbs                    = find_specialization_spell( "Shadow Orbs" );

  // Mastery Spells
  mastery_spells.shield_discipline    = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light        = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.shadowy_recall       = find_mastery_spell( PRIEST_SHADOW );

  // Glyphs
  glyphs.circle_of_healing            = find_glyph_spell( "Glyph of Circle of Healing" );
  glyphs.dispersion                   = find_glyph_spell( "Glyph of Dispersion" );
  glyphs.holy_nova                    = find_glyph_spell( "Glyph of Holy Nova" );
  glyphs.inner_fire                   = find_glyph_spell( "Glyph of Inner Fire" );
  glyphs.lightwell                    = find_glyph_spell( "Glyph of Lightwell" );
  glyphs.penance                      = find_glyph_spell( "Glyph of Penance" );
  glyphs.power_word_shield            = find_glyph_spell( "Glyph of Power Word: Shield" );
  glyphs.prayer_of_mending            = find_glyph_spell( "Glyph of Prayer of Mending" );
  glyphs.renew                        = find_glyph_spell( "Glyph of Renew" );
  glyphs.smite                        = find_glyph_spell( "Glyph of Smite" );
  glyphs.holy_fire                    = find_glyph_spell( "Glyph of Holy Fire" );
  glyphs.dark_binding                 = find_glyph_spell( "Glyph of Dark Binding" );
  glyphs.mind_spike                   = find_glyph_spell( "Glyph of Mind Spike" );
  glyphs.inner_sanctum                = find_glyph_spell( "Glyph of Inner Sanctum" );
  glyphs.mind_flay                    = find_glyph_spell( "Glyph of Mind Flay" );
  glyphs.mind_blast                   = find_glyph_spell( "Glyph of Mind Blast" );
  glyphs.devouring_plague             = find_glyph_spell( "Glyph of Devouring Plague" );
  glyphs.vampiric_embrace             = find_glyph_spell( "Glyph of Vampiric Embrace" );
  glyphs.borrowed_time                = find_glyph_spell( "Glyph of Borrowed Time" );
  glyphs.shadow_word_death            = find_glyph_spell( "Glyph of Shadow Word: Death" );

  if ( mastery_spells.echo_of_light -> ok() )
    active_spells.echo_of_light = new actions::heals::echo_of_light_t( *this );
  else
    active_spells.echo_of_light = nullptr;

  if ( specs.shadowy_apparitions -> ok() )
  {
    shadowy_apparitions.add_more( 5 );
  }

  active_spells.surge_of_darkness  = talents.from_darkness_comes_light -> ok() ? find_spell( 87160 ) : spell_data_t::not_found();

  // Set Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P    M2P    M4P    T2P    T4P     H2P     H4P
    { 105843, 105844,     0,     0,     0,     0, 105827, 105832 }, // Tier13
    { 123114, 123115,     0,     0,     0,     0, 123111, 123113 }, // Tier14
    { 138156, 138158,     0,     0,     0,     0, 138293, 138301 }, // Tier15
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// priest_t::init_buffs =====================================================

void priest_t::create_buffs()
{
  base_t::create_buffs();

  // Talents
  buffs.power_infusion = buff_creator_t( this, "power_infusion" )
                         .spell( talents.power_infusion );

  buffs.twist_of_fate = buff_creator_t( this, "twist_of_fate" )
                        .spell( talents.twist_of_fate )
                        .duration( talents.twist_of_fate -> effectN( 1 ).trigger() -> duration() )
                        .default_value( talents.twist_of_fate -> effectN( 1 ).trigger() -> effectN( 2 ).percent() );

  buffs.surge_of_light = buff_creator_t( this, "surge_of_light" )
                         .spell( find_spell( 114255 ) )
                         .chance( talents.from_darkness_comes_light -> effectN( 1 ).percent() );

  // Discipline
  buffs.archangel = new buffs::archangel_t( *this );

  buffs.borrowed_time = buff_creator_t( this, "borrowed_time" )
                        .spell( find_spell( 59889 ) )
                        .chance( specs.borrowed_time -> ok() )
                        .default_value( find_spell( 59889 ) -> effectN( 1 ).percent() );

  buffs.holy_evangelism = buff_creator_t( this, "holy_evangelism" )
                          .spell( find_spell( 81661 ) )
                          .chance( specs.evangelism -> ok() )
                          .activated( false );

  buffs.inner_fire = buff_creator_t( this, "inner_fire" )
                     .spell( find_class_spell( "Inner Fire" ) );

  buffs.inner_focus = buff_creator_t( this, "inner_focus" )
                      .spell( find_class_spell( "Inner Focus" ) )
                      .cd( timespan_t::zero() );

  buffs.inner_will = buff_creator_t( this, "inner_will" )
                     .spell( find_class_spell( "Inner Will" ) );

  buffs.spirit_shell = buff_creator_t( this, "spirit_shell" )
                       .spell( find_class_spell( "Spirit Shell" ) )
                       .cd( timespan_t::zero() );

  // Holy
  buffs.chakra_chastise = buff_creator_t( this, "chakra_chastise" )
                          .spell( find_spell( 81209 ) )
                          .chance( specialization() == PRIEST_HOLY ? 1.0 : 0.0 )
                          .cd( timespan_t::zero() );

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
                                           .duration( timespan_t::from_seconds( 6.0 ) ); // data in the old deprecated glyph. Leave hardcoded for now, 3/12/2012

  buffs.surge_of_darkness                = buff_creator_t( this, "surge_of_darkness", active_spells.surge_of_darkness )
                                           .chance( active_spells.surge_of_darkness -> ok() ? 0.15 : 0.0 ); // hardcoded into tooltip, 3/12/2012
}

// priest_t::init_actions ===================================================

void priest_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;

    if ( sim -> allow_flasks )
    {
      // Flask
      if ( level > 85 )
        precombat_list += "/flask,type=warm_sun";
      else if ( level >= 80 )
        precombat_list += "/flask,type=draconic_mind";
    }

    if ( sim -> allow_food )
    {
      // Food
      if ( level > 85 )
        precombat_list += "/food,type=mogu_fish_stew";
      else if ( level >= 80 )
        precombat_list += "/food,type=seafood_magnifique_feast";
    }

    add_action( "Power Word: Fortitude", "if=!aura.stamina.up", "precombat" );
    add_action( "Inner Fire", "", "precombat" );
    add_action( "Shadowform", "", "precombat" );

    precombat_list += "/snapshot_stats";

    if ( sim -> allow_potions )
    {
      // Potion
      if ( level > 85 )
        precombat_list += "/jade_serpent_potion";
      else if ( level >= 80 )
        precombat_list += "/volcanic_potion";
    }

    // End precombat list

    add_action( "Shadowform" );

    action_list_str += init_use_item_actions();

    action_list_str += init_use_profession_actions();

    // ======================================================================

    switch ( specialization() )
    {
      // SHADOW =============================================================
    case PRIEST_SHADOW:

      if ( sim -> allow_potions )
      {
        // Infight Potion
        if ( level > 85 )
          action_list_str += "/jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=40";
        else if ( level >= 80 )
          action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      }

      action_list_str += "/mindbender,if=talent.mindbender.enabled";
      action_list_str += "/shadowfiend,if=!talent.mindbender.enabled";
      action_list_str += "/power_infusion,if=talent.power_infusion.enabled";

      action_list_str += init_use_racial_actions();

      add_action( "Devouring Plague", "if=shadow_orb=3&(cooldown.mind_blast.remains<1.5|target.health.pct<20&cooldown.shadow_word_death.remains<1.5)" );
      add_action( "Shadow Word: Death", "if=active_enemies<=5" );
      add_action( "Mind Blast", "if=active_enemies<=6&cooldown_react" );
      action_list_str += "/mind_flay_insanity,if=target.dot.devouring_plague_tick.ticks_remain=1,chain=1";
      action_list_str += "/mind_flay_insanity,interrupt=1,chain=1";
      add_action( "Shadow Word: Pain", "cycle_targets=1,max_cycle_targets=8,if=miss_react&!ticking" );
      add_action( "Vampiric Touch", "cycle_targets=1,max_cycle_targets=8,if=remains<cast_time&miss_react" );
      add_action( "Mind Spike", "if=active_enemies<=6&buff.surge_of_darkness.react=2" );
      add_action( "Shadow Word: Pain", "cycle_targets=1,max_cycle_targets=8,if=miss_react&ticks_remain<=1" );
      add_action( "Vampiric Touch", "cycle_targets=1,max_cycle_targets=8,if=remains<cast_time+tick_time&miss_react" );
      add_action( "Vampiric Embrace", "if=shadow_orb=3&health.pct<=40" );
      add_action( "Devouring Plague", "if=shadow_orb=3&ticks_remain<=1" );
      action_list_str += "/halo,if=talent.halo.enabled";
      action_list_str += "/cascade_damage,if=talent.cascade.enabled";
      action_list_str += "/divine_star,if=talent.divine_star.enabled";
      action_list_str += "/wait,sec=cooldown.shadow_word_death.remains,if=target.health.pct<20&cooldown.shadow_word_death.remains<0.5&active_enemies<=1";
      action_list_str += "/wait,sec=cooldown.mind_blast.remains,if=cooldown.mind_blast.remains<0.5&active_enemies<=1";
      action_list_str += "/mind_spike,if=buff.surge_of_darkness.react&active_enemies<=6";
      add_action( "Mind Sear", "chain=1,interrupt=1,if=active_enemies>=3" );
      add_action( "Mind Flay", "chain=1,interrupt=1" );
      add_action( "Shadow Word: Death", "moving=1" );
      add_action( "Mind Blast", "moving=1,if=buff.divine_insight_shadow.react&cooldown_react" );
      add_action( "Shadow Word: Pain", "moving=1" );
      add_action( "Dispersion" );

      break;
      // SHADOW END =========================================================


      // DISCIPLINE =========================================================
    case PRIEST_DISCIPLINE:

      // DAMAGE DISCIPLINE ==================================================
      if ( primary_role() != ROLE_HEAL )
      {
        if ( sim -> allow_potions )
        {
          // Infight Potion
          if ( level > 85 )
            action_list_str += "/jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=40";
          else if ( level >= 80 )
            action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
        }

        if ( race == RACE_BLOOD_ELF )
          action_list_str += "/arcane_torrent,if=mana.pct<=95";

        action_list_str += "/mindbender,if=talent.mindbender.enabled";
        if ( level >= 42 )
          action_list_str += "/shadowfiend,if=!talent.mindbender.enabled";

        if ( find_class_spell( "Hymn of Hope" ) -> ok() )
        {
          action_list_str += "/hymn_of_hope,if=mana.pct<55";
          if ( level >= 42 )
            action_list_str += "&target.time_to_die>30&(pet.mindbender.active|pet.shadowfiend.active)";
        }

        if ( race != RACE_BLOOD_ELF )
          action_list_str += init_use_racial_actions();

        action_list_str += "/power_infusion,if=talent.power_infusion.enabled";

        add_action( "Archangel", "if=buff.holy_evangelism.react=5" );
        add_action( "Penance" );
        add_action( "Shadow Word: Death" );

        if ( find_class_spell( "Holy Fire" ) -> ok() )
        {
          action_list_str += "/power_word_solace,if=talent.power_word_solace.enabled";
          action_list_str += "/holy_fire,if=!talent.power_word_solace.enabled";
        }

        action_list_str += "/halo,if=talent.halo.enabled&active_enemies>3";
        action_list_str += "/divine_star,if=talent.divine_star.enabled&active_enemies>2";
        action_list_str += "/cascade_damage,if=talent.cascade.enabled&active_enemies>3";
        add_action( "Smite", "if=glyph.smite.enabled&dot.power_word_solace.remains>cast_time" );
        add_action( "Smite", "if=!talent.twist_of_fate.enabled&mana.pct>65" );
        add_action( "Smite", "if=talent.twist_of_fate.enabled&target.health.pct<20&mana.pct>target.health.pct" );
      }
      // DAMAGE DISCIPLINE END ==============================================

      // HEALER DISCIPLINE ==================================================
      else
      {
        // DEFAULT
        if ( sim -> allow_potions )
          action_list_str += "/mana_potion,if=mana.pct<=75";
        if ( find_class_spell( "Hymn of Hope" ) )
          action_list_str += "&cooldown.hymn_of_hope.remains";

        if ( race == RACE_BLOOD_ELF )
          action_list_str  += "/arcane_torrent,if=mana.pct<95";

        action_list_str += "/mindbender,if=talent.mindbender.enabled";
        if ( level >= 42 )
        {
          action_list_str += "/shadowfiend,if=!talent.mindbender.enabled&";
          if ( find_class_spell( "Hymn of Hope" ) )
            action_list_str += "(mana.pct<45|(mana.pct<70&cooldown.hymn_of_hope.remains>60))";
          else
            action_list_str += "mana.pct<70";
        }

        add_action( "Hymn of Hope", "if=mana.pct<60" );
        if ( level >= 42 )
          action_list_str += "&(pet.mindbender.active|pet.shadowfiend.active)";

        if ( race != RACE_BLOOD_ELF )
          action_list_str += init_use_racial_actions();

        add_action( "Inner Focus" );
        action_list_str += "/power_infusion,if=talent.power_infusion.enabled";
        add_action( "Power Word: Shield", "if=!cooldown.rapture.remains" );
        add_action( "Renew", "if=buff.borrowed_time.up&(!ticking|remains<tick_time)" );
        action_list_str += "/penance_heal,if=buff.borrowed_time.up|target.buff.grace.stack<3";
        add_action( "Greater Heal", "if=buff.inner_focus.up" );
        action_list_str += "/penance_heal";
        add_action( "Flash Heal", "if=buff.surge_of_light.react" );
        add_action( "Greater Heal", "if=buff.power_infusion.up|mana.pct>20" );
        action_list_str += "/power_word_solace,if=talent.power_word_solace.enabled";
        action_list_str += "/heal";
        // DEFAULT END

/*
        // PWS
        list_pws += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )
          list_pws  += "/arcane_torrent,if=mana_pct<=90";
        if ( level >= 66 )
          list_pws += "/shadowfiend,if=mana_pct<=20";
        if ( level >= 64 )
          list_pws += "/hymn_of_hope";
        if ( level >= 66 )
          list_pws += ",if=pet.shadowfiend.active";
        if ( race == RACE_TROLL )
          list_pws += "/berserking";
        list_pws += "/inner_focus";
        list_pws += "/power_infusion";

        list_pws += "/power_word_shield,ignore_debuff=1";
*/
        // PWS END
      }
      break;
      // HEALER DISCIPLINE END ==============================================

      // HOLY
    case PRIEST_HOLY:
      // DAMAGE DEALER
      if ( primary_role() != ROLE_HEAL )
      {
        if ( sim -> allow_potions )
        {
          // Infight Potion
          if ( level > 85 )
            action_list_str += "/jade_serpent_potion,if=buff.bloodlust.react|target.time_to_die<=40";
          else if ( level >= 80 )
            action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
        }

        std::string racial_condition;
        if ( race == RACE_BLOOD_ELF )
          racial_condition = ",if=mana.pct<=90";
        action_list_str += init_use_racial_actions( racial_condition );

        action_list_str += "/mindbender,if=talent.mindbender.enabled";
        if ( level >= 42 )
          action_list_str += "/shadowfiend,if=!talent.mindbender.enabled";

        add_action( "Hymn of Hope", level >= 42 ? ",if=(pet.mindbender.active|pet.shadowfiend.active)&mana.pct<=20" : ",if=mana.pct<30" );

        add_action( "Chakra: Chastise", ",if=buff.chakra_chastise.down" );
        if ( find_specialization_spell( "Holy Word: Chastise" ) -> ok() )
          action_list_str += "/holy_word";

        if ( find_class_spell( "Holy Fire" ) -> ok() )
        {
          action_list_str += "/power_word_solace,if=talent.power_word_solace.enabled";
          action_list_str += "/holy_fire,if=!talent.power_word_solace.enabled";
        }

        add_action( "Shadow Word: Pain", "if=remains<tick_time|!ticking" );
        add_action( "Smite" );
      }
      // HEALER
      else
      {
        if ( sim -> allow_potions )
          action_list_str += "/mana_potion,if=mana.pct<=75";

        action_list_str += "/mindbender,if=talent.mindbender.enabled";
        if ( level >= 42 )
          action_list_str += "/shadowfiend,if=!talent.mindbender.enabled";

        if ( add_action( "Hymn of Hope" ) )
        {
          if ( find_class_spell( "Shadowfiend" ) -> ok() )
            action_list_str += ",if=(pet.mindbender.active|pet.shadowfiend.active)&mana.pct<=20";
          else
            action_list_str += ",if=mana.pct<30";
        }

        std::string racial_condition;
        if ( race == RACE_BLOOD_ELF )
          racial_condition = ",if=mana.pct<=90";
        action_list_str += init_use_racial_actions( racial_condition );

        add_action( "Chakra: Serenity" );
        add_action( "Renew", ",if=!ticking" );
        add_action( "Holy Word", ",if=buff.chakra_serenity.up" );
        add_action( "Greater Heal", ",if=buff.serendipity.react>=2&mana.pct>40" );
        add_action( "Flash Heal", ",if=buff.surge_of_light.up" );
        add_action( "Heal" );

      }
      break;
    default:
      if ( sim -> allow_potions )                        action_list_str += "/mana_potion,if=mana.pct<=75";
      add_action( "Shadowfiend", ",if=mana_pct<50" );
      add_action( "Hymn of Hope", ",if=pet.shadowfiend.active&time>200" );
      if ( race == RACE_TROLL )                          action_list_str += "/berserking";
      if ( race == RACE_BLOOD_ELF )                      action_list_str += "/arcane_torrent,if=mana.pct<=90";
      add_action( "Holy Fire" );
      add_action( "Shadow Word: Pain",",if=remains<tick_time|!ticking" );
      add_action( "Smite" );
      break;
    }
  }

  base_t::init_actions();
}

// priest_t::init_party =====================================================

void priest_t::init_party()
{
  party_list.clear();

  if ( party == 0 )
    return;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( p && ( p != this ) && ( ! p -> quiet ) && ( ! p -> is_pet() ) )
    {
      party_list.push_back( p );
    }
  }
}

// priest_t::init_rng ====================================================

void priest_t::init_rng()
{
  base_t::init_rng();

  rngs.mastery_extra_tick  = get_rng( "shadowy_recall_extra_tick" );
  rngs.shadowy_apparitions = get_rng( "shadowy_apparitions" );
}

// priest_t::init_values ====================================================

void priest_t::init_values()
{
  base_t::init_values();

  // Discipline/Holy
  initial.mana_regen_from_spirit_multiplier = specs.meditation_disc -> ok() ?
                                              specs.meditation_disc -> effectN( 1 ).percent() :
                                              specs.meditation_holy -> effectN( 1 ).percent();
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  base_t::reset();

  if ( specs.shadowy_apparitions -> ok() )
  {
    shadowy_apparitions.reset();
  }

  init_party();
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

// Fixup Atonement Stats HPE, HPET and HPR

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

  if ( buffs.shadowform -> check() )
  { s -> result_amount *= 1.0 + buffs.shadowform -> data().effectN( 3 ).percent(); }

  if ( buffs.inner_fire -> check() && glyphs.inner_sanctum -> ok() && ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK ) )
  { s -> result_amount *= 1.0 - glyphs.inner_sanctum -> effectN( 1 ).percent(); }
}

/* Helper function to get the shadowy recall proc chance
 *
 */
double priest_t::shadowy_recall_chance()
{
  return mastery_spells.shadowy_recall -> effectN( 1 ).mastery_value() * composite_mastery();
}

// priest_t::create_options =================================================

void priest_t::create_options()
{
  base_t::create_options();

  option_t priest_options[] =
  {
    opt_string( "atonement_target", atonement_target_str ),
    opt_deprecated( "double_dot", "action_list=double_dot" ),
    opt_int( "initial_shadow_orbs", initial_shadow_orbs ),
    opt_bool( "autounshift", autoUnshift ),
    opt_null()
  };

  option_t::copy( options, priest_options );
}

// priest_t::create_profile =================================================

bool priest_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  base_t::create_profile( profile_str, type, save_html );

  if ( type == SAVE_ALL )
  {
    if ( ! atonement_target_str.empty() )
      profile_str += "atonement_target=" + atonement_target_str + "\n";

    if ( initial_shadow_orbs != 0 )
    {
      profile_str += "initial_shadow_orbs=";
      profile_str += util::to_string( initial_shadow_orbs );
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

  atonement_target_str = source_p -> atonement_target_str;
  initial_shadow_orbs  = source_p -> initial_shadow_orbs;
  autoUnshift          = source_p -> autoUnshift;
}

// priest_t::decode_set =====================================================

int priest_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  bool is_caster = false;
  bool is_healer = false;

  if ( strstr( s, "_of_dying_light" ) )
  {
    is_caster = ( strstr( s, "hood"          ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "vestment"      ) ||
                  strstr( s, "gloves"        ) ||
                  strstr( s, "leggings"      ) );

    /* FIX-ME: Kludge due to caster shoulders and chests have the wrong name */
    const char* t = item.encoded_stats_str.c_str();

    if ( ( ( item.slot == SLOT_SHOULDERS ) && strstr( t, "crit"  ) ) ||
         ( ( item.slot == SLOT_CHEST     ) && strstr( t, "haste" ) ) )
    {
      is_caster = 1;
    }

    if ( is_caster ) return SET_T13_CASTER;

    is_healer = ( strstr( s, "cowl"          ) ||
                  strstr( s, "mantle"        ) ||
                  strstr( s, "robes"         ) ||
                  strstr( s, "handwraps"     ) ||
                  strstr( s, "legwraps"      ) );
    if ( is_healer ) return SET_T13_HEAL;
  }

  if ( strstr( s, "guardian_serpent" ) )
  {
    is_caster = ( strstr( s, "hood"           ) ||
                  strstr( s, "shoulderguards" ) ||
                  strstr( s, "raiment"        ) ||
                  strstr( s, "gloves"         ) ||
                  strstr( s, "leggings"       ) );

    if ( is_caster ) return SET_T14_CASTER;

    is_healer = ( strstr( s, "cowl"          ) ||
                  strstr( s, "mantle"        ) ||
                  strstr( s, "robes"         ) ||
                  strstr( s, "handwraps"     ) ||
                  strstr( s, "legwraps"      ) );
    if ( is_healer ) return SET_T14_HEAL;
  }

  if ( strstr( s, "_of_the_exorcist" ) )
  {
    is_caster = ( strstr( s, "hood"           ) ||
                  strstr( s, "shoulderguards" ) ||
                  strstr( s, "raiment"        ) ||
                  strstr( s, "gloves"         ) ||
                  strstr( s, "leggings"       ) );

    if ( is_caster ) return SET_T15_CASTER;

    is_healer = ( strstr( s, "cowl"          ) ||
                  strstr( s, "mantle"        ) ||
                  strstr( s, "robes"         ) ||
                  strstr( s, "handwraps"     ) ||
                  strstr( s, "legwraps"      ) );
    if ( is_healer ) return SET_T15_HEAL;
  }

  if ( strstr( s, "_gladiators_mooncloth_" ) ) return SET_PVP_HEAL;
  if ( strstr( s, "_gladiators_satin_"     ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

// PRIEST MODULE INTERFACE ================================================

struct priest_module_t : public module_t
{
  priest_module_t() : module_t( PRIEST ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new priest_t( sim, name, r );
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> buffs.guardian_spirit  = buff_creator_t( p, "guardian_spirit", p -> find_spell( 47788 ) ); // Let the ability handle the CD
      p -> buffs.pain_supression  = buff_creator_t( p, "pain_supression", p -> find_spell( 33206 ) ); // Let the ability handle the CD
      p -> buffs.weakened_soul    = buff_creator_t( p, "weakened_soul",   p -> find_spell(  6788 ) );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t& module_t::priest_()
{
  static priest_module_t m = priest_module_t();
  return m;
}
