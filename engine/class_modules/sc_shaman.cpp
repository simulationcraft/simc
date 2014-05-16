// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// MoP TODO
// ==========================================================================
//
// Code:
// - Talents Totemic Restoration, Conductivity, Healing Tide Totem
//
// Restoration:
// - Greating Healing Wave, Healing Wave, Chain Heal, Healing Rain
// - Mana Tide Totem
// - Mastery
// - ROLE_HEAL action list
//
// General:
// - Class base stats for 87..90
// - Searing totem base damage scaling
// - Healing Stream Totem
// - Empower to Primal Elementals
// - Echo of the Elements functionality with heals
//
// ==========================================================================
// BUGS
// ==========================================================================
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Shaman
// ==========================================================================

namespace { // UNNAMED NAMESPACE

struct shaman_t;

enum totem_e { TOTEM_NONE = 0, TOTEM_AIR, TOTEM_EARTH, TOTEM_FIRE, TOTEM_WATER, TOTEM_MAX };
enum imbue_e { IMBUE_NONE = 0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE, FROSTBRAND_IMBUE, EARTHLIVING_IMBUE };

#define MAX_MAELSTROM_STACK ( 5 )

struct shaman_attack_t;
struct shaman_spell_t;
struct shaman_heal_t;
struct shaman_totem_pet_t;
struct totem_pulse_event_t;
struct totem_pulse_action_t;
struct maelstrom_weapon_buff_t;

struct shaman_td_t : public actor_pair_t
{
  struct dots
  {
    dot_t* flame_shock;
  } dot;

  struct debuffs
  {
    buff_t* stormstrike;
    buff_t* unleashed_fury;
    buff_t* t16_2pc_caster;
  } debuff;

  struct heals
  {
    dot_t* riptide;
    dot_t* earthliving;
  } heal;

  shaman_td_t( player_t* target, shaman_t* p );
};

struct counter_t
{
  const sim_t* sim;

  double value, interval;
  timespan_t last;

  counter_t( shaman_t* p );

  void add( double val )
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    value += val;
    if ( last > timespan_t::min() )
      interval += ( sim -> current_time - last ).total_seconds();
    last = sim -> current_time;
  }

  void reset()
  { last = timespan_t::min(); }

  double divisor() const
  {
    if ( ! sim -> debug && ! sim -> log && sim -> iterations > sim -> threads )
      return sim -> iterations - sim -> threads;
    else
      return std::min( sim -> iterations, sim -> threads );
  }

  double mean() const
  { return value / divisor(); }

  double interval_mean() const
  { return interval / divisor(); }

  void merge( const counter_t& other )
  {
    value += other.value;
    interval += other.interval;
  }
};

struct shaman_t : public player_t
{
public:
  // Misc
  timespan_t ls_reset;
  int        active_flame_shocks;
  bool       lava_surge_during_lvb;
  std::vector<counter_t*> counters;

  // Options
  timespan_t wf_delay;
  timespan_t wf_delay_stddev;
  timespan_t uf_expiration_delay;
  timespan_t uf_expiration_delay_stddev;

  // Active
  action_t* active_lightning_charge;

  // Cached actions
  action_t* action_ancestral_awakening;
  action_t* action_improved_lava_lash;
  action_t* action_lightning_strike;

  // Pets
  pet_t* pet_feral_spirit[4];
  pet_t* pet_fire_elemental;
  pet_t* guardian_fire_elemental;
  pet_t* pet_earth_elemental;
  pet_t* guardian_earth_elemental;
  pet_t* guardian_lightning_elemental[10];

  // Totems
  shaman_totem_pet_t* totems[ TOTEM_MAX ];

  // Constants
  struct
  {
    double matching_gear_multiplier;
    double flurry_rating_multiplier;
    double speed_attack_ancestral_swiftness;
    double haste_ancestral_swiftness;
    double haste_elemental_mastery;
    double attack_speed_flurry;
    double attack_speed_unleash_wind;
  } constant;

  // Buffs
  struct
  {
    buff_t* ancestral_swiftness;
    buff_t* ascendance;
    buff_t* elemental_focus;
    buff_t* echo_of_the_elements;
    buff_t* lava_surge;
    buff_t* spew_lava;
    buff_t* lightning_shield;
    maelstrom_weapon_buff_t* maelstrom_weapon;
    buff_t* shamanistic_rage;
    buff_t* shocking_lava;
    buff_t* spirit_walk;
    buff_t* spiritwalkers_grace;
    buff_t* tier16_2pc_melee;
    buff_t* tier16_2pc_caster;
    buff_t* unleash_flame;
    buff_t* unleashed_fury_wf;
    buff_t* tidal_waves;
    buff_t* water_shield;

    haste_buff_t* elemental_mastery;
    haste_buff_t* flurry;
    haste_buff_t* tier13_4pc_healer;
    haste_buff_t* unleash_wind;

    stat_buff_t* elemental_blast_agility;
    stat_buff_t* elemental_blast_crit;
    stat_buff_t* elemental_blast_haste;
    stat_buff_t* elemental_blast_mastery;
    stat_buff_t* elemental_blast_multistrike;
    stat_buff_t* tier13_2pc_caster;
    stat_buff_t* tier13_4pc_caster;

    buff_t* improved_chain_lightning;
  } buff;

  // Cooldowns
  struct
  {
    cooldown_t* ancestral_swiftness;
    cooldown_t* ascendance;
    cooldown_t* fire_elemental_totem;
    cooldown_t* earth_elemental_totem;
    cooldown_t* feral_spirits;
    cooldown_t* lava_burst;
    cooldown_t* lava_lash;
    cooldown_t* shock;
    cooldown_t* strike;
    cooldown_t* t16_4pc_caster;
    cooldown_t* t16_4pc_melee;
    cooldown_t* windfury_weapon;
  } cooldown;

  // Gains
  struct
  {
    gain_t* primal_wisdom;
    gain_t* resurgence;
    gain_t* rolling_thunder;
    gain_t* telluric_currents;
    gain_t* thunderstorm;
    gain_t* water_shield;
  } gain;

  // Tracked Procs
  struct
  {
    proc_t* elemental_overload;
    proc_t* lava_surge;
    proc_t* ls_fast;
    proc_t* maelstrom_weapon;
    proc_t* rolling_thunder;
    proc_t* swings_clipped_mh;
    proc_t* swings_clipped_oh;
    proc_t* swings_reset_mh;
    proc_t* swings_reset_oh;
    proc_t* t15_2pc_melee;
    proc_t* t16_2pc_melee;
    proc_t* t16_4pc_caster;
    proc_t* t16_4pc_melee;
    proc_t* wasted_t15_2pc_melee;
    proc_t* wasted_lava_surge;
    proc_t* wasted_ls;
    proc_t* wasted_ls_shock_cd;
    proc_t* wasted_mw;
    proc_t* windfury;

    proc_t* fulmination[7];

    proc_t* uf_flame_shock;
    proc_t* uf_fire_nova;
    proc_t* uf_lava_burst;
    proc_t* uf_elemental_blast;
    proc_t* uf_wasted;

    proc_t* surge_during_lvb;
  } proc;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;

    // Elemental / Restoration
    const spell_data_t* spiritual_insight;

    // Elemental
    const spell_data_t* elemental_focus;
    const spell_data_t* elemental_fury;
    const spell_data_t* fulmination;
    const spell_data_t* lava_surge;
    const spell_data_t* readiness_elemental;
    const spell_data_t* rolling_thunder;
    const spell_data_t* shamanism;

    // Enhancement
    const spell_data_t* critical_strikes;
    const spell_data_t* dual_wield;
    const spell_data_t* flurry;
    const spell_data_t* mental_quickness;
    const spell_data_t* primal_wisdom;
    const spell_data_t* readiness_enhancement;
    const spell_data_t* shamanistic_rage;
    const spell_data_t* spirit_walk;
    const spell_data_t* maelstrom_weapon;

    // Restoration
    const spell_data_t* ancestral_awakening;
    const spell_data_t* ancestral_focus;
    const spell_data_t* earth_shield;
    const spell_data_t* meditation;
    const spell_data_t* purification;
    const spell_data_t* resurgence;
    const spell_data_t* riptide;
    const spell_data_t* tidal_waves;
  } spec;

  // Masteries
  struct
  {
    const spell_data_t* elemental_overload;
    const spell_data_t* enhanced_elements;
    const spell_data_t* deep_healing;
  } mastery;

  // Talents
  struct
  {
    const spell_data_t* call_of_the_elements;
    const spell_data_t* totemic_restoration;

    const spell_data_t* elemental_mastery;
    const spell_data_t* ancestral_swiftness;
    const spell_data_t* echo_of_the_elements;

    const spell_data_t* unleashed_fury;
    const spell_data_t* primal_elementalist;
    const spell_data_t* elemental_blast;

    const spell_data_t* shocking_lava;
    const spell_data_t* storm_elemental_totem;
    const spell_data_t* spew_lava;
  } talent;

  // Perks
  struct
  {
    // Elemental

    // Enhancement
    const spell_data_t* improved_searing_totem;
    const spell_data_t* improved_frost_shock;
    const spell_data_t* improved_lava_lash;
    const spell_data_t* improved_flame_shock;
    const spell_data_t* improved_maelstrom_weapon;
    const spell_data_t* improved_stormstrike;
    const spell_data_t* improved_lava_lash_2;
    const spell_data_t* improved_feral_spirits;

    // Elemental
    const spell_data_t* improved_chain_lightning;
    const spell_data_t* improved_lightning_shield;
    const spell_data_t* improved_elemental_fury;
    const spell_data_t* improved_lightning_bolt;
    const spell_data_t* improved_lava_burst;
    const spell_data_t* improved_shocks;
  } perk;

  // Glyphs
  struct
  {
    const spell_data_t* chain_lightning;
    const spell_data_t* fire_elemental_totem;
    const spell_data_t* flame_shock;
    const spell_data_t* frost_shock;
    const spell_data_t* healing_storm;
    const spell_data_t* lava_lash;
    const spell_data_t* spiritwalkers_grace;
    const spell_data_t* telluric_currents;
    const spell_data_t* thunder;
    const spell_data_t* thunderstorm;
    const spell_data_t* water_shield;
    const spell_data_t* lightning_shield;
  } glyph;

  // Misc Spells
  struct
  {
    const spell_data_t* primal_wisdom;
    const spell_data_t* resurgence;
    const spell_data_t* ancestral_swiftness;
    const spell_data_t* flame_shock;
    const spell_data_t* windfury_driver;
  } spell;

  // RNGs
  real_ppm_t rppm_echo_of_the_elements;

  // Cached pointer for ascendance / normal white melee
  shaman_attack_t* melee_mh;
  shaman_attack_t* melee_oh;
  shaman_attack_t* ascendance_mh;
  shaman_attack_t* ascendance_oh;

  // Weapon Enchants
  shaman_attack_t* windfury_mh;
  shaman_attack_t* windfury_oh;
  shaman_spell_t*  flametongue_mh;
  shaman_spell_t*  flametongue_oh;

  // Tier16 random imbues
  action_t* t16_wind;
  action_t* t16_flame;

  shaman_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, SHAMAN, name, r ),
    ls_reset( timespan_t::zero() ), active_flame_shocks( 0 ), lava_surge_during_lvb( false ),
    wf_delay( timespan_t::from_seconds( 0.95 ) ), wf_delay_stddev( timespan_t::from_seconds( 0.25 ) ),
    uf_expiration_delay( timespan_t::from_seconds( 0.3 ) ), uf_expiration_delay_stddev( timespan_t::from_seconds( 0.05 ) ),
    active_lightning_charge( nullptr ),
    action_ancestral_awakening( nullptr ),
    action_improved_lava_lash( nullptr ),
    action_lightning_strike( nullptr ),
    pet_fire_elemental( nullptr ),
    guardian_fire_elemental( nullptr ),
    pet_earth_elemental( nullptr ),
    guardian_earth_elemental( nullptr ),
    constant(),
    buff(),
    cooldown(),
    gain(),
    proc(),
    spec(),
    mastery(),
    talent(),
    glyph(),
    spell(),
    rppm_echo_of_the_elements( *this, 0, RPPM_HASTE )
  {
    // Active
    active_lightning_charge   = 0;

    action_improved_lava_lash = 0;
    action_lightning_strike = 0;

    for ( int i = 0; i < 4; i++ ) pet_feral_spirit[ i ] = 0;
    range::fill( guardian_lightning_elemental, 0 );

    // Totem tracking
    for ( int i = 0; i < TOTEM_MAX; i++ ) totems[ i ] = 0;

    // Cooldowns
    cooldown.ancestral_swiftness  = get_cooldown( "ancestral_swiftness"   );
    cooldown.ascendance           = get_cooldown( "ascendance"            );
    cooldown.earth_elemental_totem= get_cooldown( "earth_elemental_totem" );
    cooldown.fire_elemental_totem = get_cooldown( "fire_elemental_totem"  );
    cooldown.feral_spirits        = get_cooldown( "feral_spirit"          );
    cooldown.lava_burst           = get_cooldown( "lava_burst"            );
    cooldown.lava_lash            = get_cooldown( "lava_lash"             );
    cooldown.shock                = get_cooldown( "shock"                 );
    cooldown.strike               = get_cooldown( "strike"                );
    cooldown.t16_4pc_caster       = get_cooldown( "t16_4pc_caster"        );
    cooldown.t16_4pc_melee        = get_cooldown( "t16_4pc_melee"         );
    cooldown.windfury_weapon      = get_cooldown( "windfury_weapon"       );

    melee_mh = 0;
    melee_oh = 0;
    ascendance_mh = 0;
    ascendance_oh = 0;

    // Weapon Enchants
    windfury_mh    = 0;
    windfury_oh    = 0;
    flametongue_mh = 0;
    flametongue_oh = 0;

    t16_wind = 0;
    t16_flame = 0;
  }

  virtual           ~shaman_t();

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      init_scaling();
  virtual void      create_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_action_list();
  virtual void      moving();
  virtual void      invalidate_cache( cache_e c );
  virtual double    temporary_movement_modifier() const;
  virtual double    composite_melee_haste() const;
  virtual double    composite_melee_speed() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_spell_haste() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_power( school_e school ) const;
  virtual double    composite_spell_power_multiplier() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual double    composite_multistrike() const;
  virtual void      target_mitigation( school_e, dmg_e, action_state_t* );
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual action_t* create_proc_action( const std::string& name );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual expr_t* create_expression( action_t*, const std::string& name );
  virtual set_e       decode_set( const item_t& ) const;
  virtual resource_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_e primary_role() const;
  virtual stat_e convert_hybrid_stat( stat_e s ) const;
  virtual void      arise();
  virtual void      reset();
  virtual void      merge( player_t& other );

  target_specific_t<shaman_td_t*> target_data;

  virtual shaman_td_t* get_target_data( player_t* target ) const
  {
    shaman_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new shaman_td_t( target, const_cast<shaman_t*>(this) );
    }
    return td;
  }

  // Event Tracking
  virtual void regen( timespan_t periodicity );
};

shaman_t::~shaman_t()
{
  range::dispose( counters );
}

counter_t::counter_t( shaman_t* p ) :
  sim( p -> sim ), value( 0 ), interval( 0 ), last( timespan_t::min() )
{
  p -> counters.push_back( this );
}

shaman_td_t::shaman_td_t( player_t* target, shaman_t* p ) :
  actor_pair_t( target, p )
{
  dot.flame_shock       = target -> get_dot( "flame_shock", p );

  debuff.stormstrike    = buff_creator_t( *this, "stormstrike", p -> find_specialization_spell( "Stormstrike" ) );
  debuff.unleashed_fury = buff_creator_t( *this, "unleashed_fury_ft", p -> find_spell( 118470 ) );
  debuff.t16_2pc_caster = buff_creator_t( *this, "tier16_2pc_caster", p -> sets.set( SET_T16_2PC_CASTER ) -> effectN( 1 ).trigger() )
                          .chance( static_cast< double >( p -> sets.has_set_bonus( SET_T16_2PC_CASTER ) ) );
}

// ==========================================================================
// Shaman Custom Buff Declaration
// ==========================================================================

struct maelstrom_weapon_buff_t : public buff_t
{
  std::vector<shaman_attack_t*> trigger_actions;

  maelstrom_weapon_buff_t( shaman_t* player ) :
    buff_t( buff_creator_t( player, 53817, "maelstrom_weapon" ) )
  { activated = false; }

  using buff_t::trigger;

  bool trigger( shaman_attack_t* action, int stacks, double chance );
  void execute( int stacks, double value, timespan_t duration );
  void reset();
};

struct ascendance_buff_t : public buff_t
{
  action_t* lava_burst;

  ascendance_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, 114051, "ascendance" ) ),
    lava_burst( 0 )
  { }

  void ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown );
  bool trigger( int stacks, double value, double chance, timespan_t duration );
  void expire_override();
};

struct unleash_flame_buff_t : public buff_t
{
  core_event_t* expiration_delay;

  unleash_flame_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, 73683, "unleash_flame" ) ),
    expiration_delay( 0 )
  { }

  void expire_override();
  void reset();
};

// ==========================================================================
// Shaman Action Base Template
// ==========================================================================

template <class Base>
struct shaman_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef shaman_action_t base_t;

  // Misc stuff
  bool        totem;
  bool        shock;

  // Echo of Elements functionality
  bool        may_proc_eoe;
  bool        uses_eoe;
  proc_t*     used_eoe;
  proc_t*     generated_eoe;

  shaman_action_t( const std::string& n, shaman_t* player,
                   const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    totem( false ), shock( false ),
    may_proc_eoe( false ), uses_eoe( false )
  {
    ab::may_crit = true;
  }

  void init()
  {
    ab::init();

    if ( ! ab::background && ab::has_amount_result() )
      may_proc_eoe = true;

    if ( uses_eoe && ab::s_data )
      used_eoe = p() -> get_proc( "Echo of the Elements: " + std::string( ab::s_data -> name_cstr() ) + " (consume)" );

    if ( may_proc_eoe && ab::s_data )
      generated_eoe = p() -> get_proc( "Echo of the Elements: " + std::string( ab::s_data -> name_cstr() ) + " (generate)" );
  }

  shaman_t* p()
  { return debug_cast< shaman_t* >( ab::player ); }
  const shaman_t* p() const
  { return debug_cast< shaman_t* >( ab::player ); }

  shaman_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  double cost() const
  {
    if ( ab::player -> resources.infinite_resource[ RESOURCE_MANA ] == 1 )
      return 0;

    double c = ab::cost();
    if ( c == 0 )
      return c;
    c *= 1.0 + cost_reduction();
    if ( c < 0 ) c = 0;
    return c;
  }

  virtual double cost_reduction() const
  {
    double c = 0.0;

    if ( ab::base_execute_time == timespan_t::zero() ||
         ( ( ab::data().school_mask() & SCHOOL_MASK_NATURE ) != 0 &&
           p() -> buff.maelstrom_weapon -> check() == p() -> buff.maelstrom_weapon -> max_stack() ) )
      c += p() -> spec.mental_quickness -> effectN( ! shock ? 2 : 3 ).percent();

    if ( p() -> buff.shamanistic_rage -> up() && ( ab::harmful || totem ) )
      c += p() -> buff.shamanistic_rage -> data().effectN( 1 ).percent();

    return c;
  }

  // Both Ancestral Swiftness and Maelstrom Weapon share this conditional for use
  bool instant_eligibility() const
  { return ab::data().school_mask() & SCHOOL_MASK_NATURE && ab::base_execute_time != timespan_t::zero(); }

  void execute()
  {
    ab::execute();

    if ( p() -> talent.echo_of_the_elements -> ok() &&
         may_proc_eoe && 
         ab::result_is_hit( ab::execute_state -> result ) && 
         p() -> rppm_echo_of_the_elements.trigger() )
    {
      p() -> buff.echo_of_the_elements -> trigger();
      generated_eoe -> occur();
    }
  }

  void update_ready( timespan_t cd )
  {
    if ( uses_eoe && p() -> buff.echo_of_the_elements -> up() )
    {
      cd = timespan_t::zero();
      p() -> buff.echo_of_the_elements -> expire();
      used_eoe -> occur();
    }

    ab::update_ready( cd );
  }

  virtual expr_t* create_expression( const std::string& name )
  {
    if ( ! util::str_compare_ci( name, "cooldown.higher_priority.min_remains" ) )
      return ab::create_expression( name );

    struct hprio_cd_min_remains_expr_t : public expr_t
    {
      action_t* action_;
      std::vector<cooldown_t*> cd_;

      // TODO: Line_cd support
      hprio_cd_min_remains_expr_t( action_t* a ) :
        expr_t( "min_remains" ), action_( a )
      {
        action_priority_list_t* list = a -> player -> get_action_priority_list( a -> action_list );
        for ( size_t i = 0, end = list -> foreground_action_list.size(); i < end; i++ )
        {
          action_t* list_action = list -> foreground_action_list[ i ];
          // Jump out when we reach this action
          if ( list_action == action_ )
            break;

          // Skip if this action's cooldown is the same as the list action's cooldown
          if ( list_action -> cooldown == action_ -> cooldown )
            continue;

          // Skip actions with no cooldown
          if ( list_action -> cooldown && list_action -> cooldown -> duration == timespan_t::zero() )
            continue;

          // Skip cooldowns that are already accounted for
          if ( std::find( cd_.begin(), cd_.end(), list_action -> cooldown ) != cd_.end() )
            continue;

          //std::cout << "Appending " << list_action -> name() << " to check list" << std::endl;
          cd_.push_back( list_action -> cooldown );
        }
      }

      double evaluate()
      {
        if ( cd_.size() == 0 )
          return 0;

        timespan_t min_cd = cd_[ 0 ] -> remains();
        for ( size_t i = 1, end = cd_.size(); i < end; i++ )
        {
          timespan_t remains = cd_[ i ] -> remains();
          //std::cout << "cooldown.higher_priority.min_remains " << cd_[ i ] -> name_str << " remains=" << remains.total_seconds() << std::endl;
          if ( remains < min_cd )
            min_cd = remains;
        }

        //std::cout << "cooldown.higher_priority.min_remains=" << min_cd.total_seconds() << std::endl;
        return min_cd.total_seconds();
      }
    };

    return new hprio_cd_min_remains_expr_t( this );
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public shaman_action_t<melee_attack_t>
{
private:
  typedef shaman_action_t<melee_attack_t> ab;
public:
  bool may_proc_windfury;
  bool may_proc_flametongue;
  bool may_proc_primal_wisdom;

  // Maelstrom Weapon functionality
  bool        may_proc_maelstrom;
  counter_t*  maelstrom_procs;
  counter_t*  maelstrom_procs_wasted;

  shaman_attack_t( const std::string& token, shaman_t* p, const spell_data_t* s ) :
    base_t( token, p, s ),
    may_proc_windfury( false ), may_proc_flametongue( true ), may_proc_primal_wisdom( true ),
    may_proc_maelstrom( p -> spec.maelstrom_weapon -> ok() ),
    maelstrom_procs( 0 ), maelstrom_procs_wasted( 0 )
  {
    special = may_crit = true;
    may_glance = false;
  }

  void init()
  {
    ab::init();

    if ( may_proc_maelstrom )
    {
      maelstrom_procs = new counter_t( p() );
      maelstrom_procs_wasted = new counter_t( p() );
    }
  }

  void impact( action_state_t* );
};

// ==========================================================================
// Shaman Base Spell
// ==========================================================================

template <class Base>
struct shaman_spell_base_t : public shaman_action_t<Base>
{
private:
  typedef shaman_action_t<Base> ab;
public:
  typedef shaman_spell_base_t<Base> base_t;

  std::vector<counter_t*> maelstrom_weapon_cast, maelstrom_weapon_executed;

  shaman_spell_base_t( const std::string& n, shaman_t* player,
                       const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  { }

  void init()
  {
    ab::init();

    for ( size_t i = 0; i < MAX_MAELSTROM_STACK + 2; i++ )
    {
      maelstrom_weapon_cast.push_back( new counter_t( ab::p() ) );
      maelstrom_weapon_executed.push_back( new counter_t( ab::p() ) );
    }
  }

  void execute();
  void schedule_execute( action_state_t* state = 0 );

  double cost_reduction() const
  {
    double c = ab::cost_reduction();
    const shaman_t* p = ab::p();

    if ( c > -1.0 && ab::instant_eligibility() )
      c += p -> buff.maelstrom_weapon -> check() * p -> buff.maelstrom_weapon -> data().effectN( 2 ).percent();

    return c;
  }

  timespan_t execute_time() const
  {
    timespan_t t = ab::execute_time();

    if ( ab::instant_eligibility() )
    {
      const shaman_t* p = ab::p();

      if ( p -> buff.ancestral_swiftness -> up() )
        t *= 1.0 + p -> buff.ancestral_swiftness -> data().effectN( 1 ).percent();
      else
        t *= 1.0 + p -> buff.maelstrom_weapon -> stack() * p -> buff.maelstrom_weapon -> data().effectN( 1 ).percent();
    }

    return t;
  }

  double action_multiplier() const
  {
    double m = ab::action_multiplier();

    const shaman_t* p = ab::p();
    if ( p -> buff.maelstrom_weapon -> up() && ab::instant_eligibility() )
      m *= 1.0 + p -> buff.maelstrom_weapon -> check() * p -> perk.improved_maelstrom_weapon -> effectN( 1 ).percent();

    return m;
  }

  bool use_ancestral_swiftness()
  {
    shaman_t* p = ab::p();
    return ab::instant_eligibility() && p -> buff.ancestral_swiftness -> check() &&
           p -> buff.maelstrom_weapon -> check() * p -> buff.maelstrom_weapon -> data().effectN( 1 ).percent() < 1.0;
  }
};

// ==========================================================================
// Shaman Offensive Spell
// ==========================================================================

struct shaman_spell_t : public shaman_spell_base_t<spell_t>
{
  // Elemental overload stuff
  bool     overload;
  shaman_spell_t* overload_spell;
  double   overload_chance_multiplier;

  shaman_spell_t( const std::string& token, shaman_t* p,
                  const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    base_t( token, p, s ),
    overload( false ), overload_spell( 0 ), overload_chance_multiplier( 1.0 )
  {
    parse_options( 0, options );

    crit_bonus_multiplier *= 1.0 + p -> spec.elemental_fury -> effectN( 1 ).percent() + p -> perk.improved_elemental_fury -> effectN( 1 ).percent();
  }

  virtual void   execute();

  virtual void consume_resource()
  {
    base_t::consume_resource();

    if ( harmful && callbacks && ! proc && resource_consumed > 0 && p() -> buff.elemental_focus -> up() )
      p() -> buff.elemental_focus -> decrement();
  }

  virtual void impact( action_state_t* state )
  {
    base_t::impact( state );

    if ( overload_spell && result_is_hit( state -> result ) )
    {
      double overload_chance = 0.0;
      if ( p() -> mastery.elemental_overload -> ok() )
        overload_chance += p() -> cache.mastery_value() * overload_chance_multiplier;

      if ( overload_chance && rng().roll( overload_chance ) )
      {
        overload_spell -> execute();
        if ( p() -> sets.has_set_bonus( SET_T13_4PC_CASTER ) )
          p() -> buff.tier13_4pc_caster -> trigger();
      }
    }

    // Overloads dont trigger elemental focus
    if ( ! overload && ! proc && p() -> specialization() == SHAMAN_ELEMENTAL &&
         spell_power_mod.direct > 0 && state -> result == RESULT_CRIT )
      p() -> buff.elemental_focus -> trigger( p() -> buff.elemental_focus -> data().initial_stacks() );
  }

  virtual double cost_reduction() const
  {
    double c = base_t::cost_reduction();

    if ( harmful && callbacks && ! proc && p() -> buff.elemental_focus -> up() )
      c += p() -> buff.elemental_focus -> data().effectN( 1 ).percent();

    return c;
  }

  virtual bool usable_moving() const
  {
    if ( p() -> buff.spiritwalkers_grace -> check() || execute_time() == timespan_t::zero() )
      return true;

    return base_t::usable_moving();
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = base_t::composite_target_multiplier( target );


    if ( td( target ) -> debuff.t16_2pc_caster -> check() && ( dbc::is_school( school, SCHOOL_FIRE )
    || dbc::is_school( school, SCHOOL_NATURE ) ) )
      m *= 1.0 + td( target ) -> debuff.t16_2pc_caster -> data().effectN( 1 ).percent();


    return m;
  }

  virtual double composite_da_multiplier() const
  {
    double m = base_t::composite_da_multiplier();

    if ( instant_eligibility() && p() -> buff.maelstrom_weapon -> stack() > 0 )
      m *= 1.0 + p() -> sets.set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent();

    if ( p() -> buff.elemental_focus -> up() )
      m *= 1.0 + p() -> buff.elemental_focus -> data().effectN( 2 ).percent();

    return m;
  }
};

// ==========================================================================
// Shaman Heal
// ==========================================================================

struct shaman_heal_t : public shaman_spell_base_t<heal_t>
{
  double elw_proc_high,
         elw_proc_low,
         resurgence_gain;

  bool proc_tidal_waves,
       consume_tidal_waves;

  shaman_heal_t( const std::string& token, shaman_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( 0, options );
  }

  shaman_heal_t( shaman_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( "", p, s ),
    elw_proc_high( .2 ), elw_proc_low( 1.0 ),
    resurgence_gain( 0 ),
    proc_tidal_waves( false ), consume_tidal_waves( false )
  {
    parse_options( 0, options );
  }

  double composite_spell_power() const
  {
    double sp = base_t::composite_spell_power();

    if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
      sp += p() -> main_hand_weapon.buff_value;

    return sp;
  }

  double composite_da_multiplier() const
  {
    double m = base_t::composite_da_multiplier();
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
    m *= 1.0 + p() -> buff.maelstrom_weapon -> stack() * p() -> glyph.healing_storm -> effectN( 1 ).percent();
    return m;
  }

  double composite_ta_multiplier() const
  {
    double m = base_t::composite_da_multiplier();
    m *= 1.0 + p() -> spec.purification -> effectN( 1 ).percent();
    m *= 1.0 + p() -> buff.maelstrom_weapon -> stack() * p() -> glyph.healing_storm -> effectN( 1 ).percent();
    return m;
  }

  double composite_target_multiplier( player_t* target ) const
  {
    double m = base_t::composite_target_multiplier( target );
    if ( target -> buffs.earth_shield -> up() )
      m *= 1.0 + p() -> spec.earth_shield -> effectN( 2 ).percent();
    return m;
  }

  void impact( action_state_t* s );

  void execute()
  {
    base_t::execute();

    if ( consume_tidal_waves )
      p() -> buff.tidal_waves -> decrement();
  }

  virtual double deep_healing( const action_state_t* s )
  {
    if ( ! p() -> mastery.deep_healing -> ok() )
      return 0.0;

    double hpp = ( 1.0 - s -> target -> health_percentage() / 100.0 );

    return 1.0 + hpp * p() -> cache.mastery_value();
  }
};

// ==========================================================================
// Pet Spirit Wolf
// ==========================================================================

struct feral_spirit_pet_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    melee_t( feral_spirit_pet_t* player ) :
      melee_attack_t( "melee", player, spell_data_t::nil() )
    {
      auto_attack = true;
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating = true;
      may_crit = true;
      school      = SCHOOL_PHYSICAL;
    }

    feral_spirit_pet_t* p() { return static_cast<feral_spirit_pet_t*>( player ); }

    void init()
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_feral_spirit[ 0 ] )
        stats = p() -> o() -> pet_feral_spirit[ 0 ] -> get_stats( name(), this );
    }

    virtual void impact( action_state_t* state )
    {
      melee_attack_t::impact( state );

      if ( result_is_hit( state -> result ) )
      {
        shaman_t* o = p() -> o();

        if ( rng().roll( o -> sets.set( SET_T13_4PC_MELEE ) -> effectN( 1 ).percent() ) )
        {
          int mwstack = o -> buff.maelstrom_weapon -> total_stack();
          o -> buff.maelstrom_weapon -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
          o -> proc.maelstrom_weapon -> occur();

          if ( mwstack == o -> buff.maelstrom_weapon -> max_stack() )
            o -> proc.wasted_mw -> occur();
        }
      }
    }
  };

  melee_t* melee;
  const spell_data_t* command;

  feral_spirit_pet_t( shaman_t* owner ) :
    pet_t( owner -> sim, owner, "spirit_wolf", true, true ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );

    owner_coeff.ap_from_ap = 0.50;

    command = owner -> find_spell( 65222 );
  }

  shaman_t* o() const { return static_cast<shaman_t*>( owner ); }

  void arise()
  {
    pet_t::arise();
    schedule_ready();
  }

  void init_action_list()
  {
    pet_t::init_action_list();

    melee = new melee_t( this );
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    if ( ! melee -> execute_event )
      melee -> schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school ) const
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    m *= 1.0 + o() -> perk.improved_feral_spirits -> effectN( 1 ).percent();

    // TODO-WOD: Figure out values for reals, put 100% scaling here for now
    m *= 2.0;

    return m;
  }
};

struct earth_elemental_pet_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> current.distance = 1; }
    virtual timespan_t execute_time() const { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    virtual bool ready() { return ( player -> current.distance > 1 ); }
    virtual bool usable_moving() const { return true; }
  };

  struct auto_melee_attack_t : public melee_attack_t
  {
    auto_melee_attack_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "auto_attack", player )
    {
      assert( player -> main_hand_weapon.type != WEAPON_NONE );
      player -> main_hand_attack = new melee_t( player );

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player -> is_moving() ) return false;
      return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct melee_t : public melee_attack_t
  {
    melee_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "earth_melee", player, spell_data_t::nil() )
    {
      auto_attack = true;
      school            = SCHOOL_PHYSICAL;
      may_crit          = true;
      background        = true;
      repeating         = true;
      weapon            = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
    }
  };

  struct pulverize_t : public melee_attack_t
  {
    pulverize_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "pulverize", player, player -> find_spell( 118345 ) )
    {
      school            = SCHOOL_PHYSICAL;
      may_crit          = true;
      special           = true;
      weapon            = &( player -> main_hand_weapon );
      weapon_multiplier = 0;
    }
  };

  const spell_data_t* command;

  earth_elemental_pet_t( sim_t* sim, shaman_t* owner, bool guardian ) :
    pet_t( sim, owner, ( ! guardian ) ? "primal_earth_elemental" : "greater_earth_elemental", guardian /*GUARDIAN*/ )
  {
    stamina_per_owner   = 1.0;

    command = owner -> find_spell( 65222 );
  }

  double composite_player_multiplier( school_e school ) const
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    m *= 0.1;

    return m;
  }

  shaman_t* o() { return static_cast< shaman_t* >( owner ); }
  //timespan_t available() { return sim -> max_time; }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    // Approximated from lvl 85, 86 Earth Elementals
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 1.3;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 1.3;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );

    if ( o() -> talent.primal_elementalist -> ok() )
    {
      main_hand_weapon.min_dmg *= 1.5;
      main_hand_weapon.max_dmg *= 1.5;
    }

    resources.base[ RESOURCE_HEALTH ] = 8000; // Approximated from lvl85 earth elemental in game
    resources.base[ RESOURCE_MANA   ] = 0; //

    // Simple as it gets, travel to target, kick off melee
    action_list_str = "travel/auto_attack,moving=0";

    if ( o() -> talent.primal_elementalist -> ok() )
      action_list_str += "/pulverize";

    owner_coeff.ap_from_sp = 1.3;
    if ( o() -> talent.primal_elementalist -> ok() )
      owner_coeff.ap_from_sp *= 1.0 + o() -> talent.primal_elementalist -> effectN( 1 ).percent();
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t( this );
    if ( name == "auto_attack" ) return new auto_melee_attack_t ( this );
    if ( name == "pulverize"   ) return new pulverize_t( this );

    return pet_t::create_action( name, options_str );
  }
};

struct fire_elemental_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> current.distance = 1; }
    virtual timespan_t execute_time() const { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    virtual bool ready() { return ( player -> current.distance > 1 ); }
    virtual timespan_t gcd() const { return timespan_t::zero(); }
    virtual bool usable_moving() const { return true; }
  };

  struct fire_elemental_spell_t : public spell_t
  {
    fire_elemental_t* p;

    fire_elemental_spell_t( const std::string& t, fire_elemental_t* p, const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
      spell_t( t, p, s ), p( p )
    {
      parse_options( 0, options );

      school                      = SCHOOL_FIRE;
      may_crit                    = true;
      base_costs[ RESOURCE_MANA ] = 0;
      crit_bonus_multiplier      *= 1.0 + p -> o() -> spec.elemental_fury -> effectN( 1 ).percent() + p -> o() -> perk.improved_elemental_fury -> effectN( 1 ).percent();
    }

    virtual double composite_da_multiplier() const
    {
      double m = spell_t::composite_da_multiplier();

      if ( p -> o() -> mastery.enhanced_elements -> ok() )
        m *= 1.0 + p -> o() -> cache.mastery_value();

      return m;
    }

    virtual double composite_ta_multiplier() const
    {
      double m = spell_t::composite_ta_multiplier();

      if ( p -> o() -> mastery.enhanced_elements -> ok() )
        m *= 1.0 + p -> o() -> cache.mastery_value();

      return m;
    }
  };

  struct fire_nova_t  : public fire_elemental_spell_t
  {
    fire_nova_t( fire_elemental_t* player, const std::string& options ) :
      fire_elemental_spell_t( "fire_nova", player, player -> find_spell( 117588 ), options )
    {
      aoe = -1;
      base_dd_min        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) * .65;
      base_dd_max        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) * .65;
    }
  };

  struct fire_blast_t : public fire_elemental_spell_t
  {
    fire_blast_t( fire_elemental_t* player, const std::string& options ) :
      fire_elemental_spell_t( "fire_blast", player, player -> find_spell( 57984 ), options )
    {
      base_dd_min        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) / 1 / p -> level;
      base_dd_max        = p -> dbc.spell_scaling( p -> o() -> type, p -> level ) / 1 / p -> level;
    }

    virtual void execute()
    {
      fire_elemental_spell_t::execute();

      // Reset swing timer
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
        player -> main_hand_attack -> execute_event -> reschedule( player -> main_hand_attack -> execute_time() );
    }

    virtual bool usable_moving() const
    {
      return true;
    }
  };

  struct immolate_t : public fire_elemental_spell_t
  {
    immolate_t( fire_elemental_t* player, const std::string& options ) :
      fire_elemental_spell_t( "immolate", player, player -> find_spell( 118297 ), options )
    {
      hasted_ticks  = true;
      tick_may_crit = true;
      base_costs[ RESOURCE_MANA ] = 0;
    }
  };

  struct fire_melee_t : public melee_attack_t
  {
    fire_melee_t( fire_elemental_t* player ) :
      melee_attack_t( "fire_melee", player, spell_data_t::nil() )
    {
      auto_attack = true;
      special                      = false;
      may_crit                     = true;
      background                   = true;
      repeating                    = true;
      spell_power_mod.direct       = 1.0;
      school                       = SCHOOL_FIRE;
      crit_bonus_multiplier       *= 1.0 + player -> o() -> spec.elemental_fury -> effectN( 1 ).percent() + player -> o() -> perk.improved_elemental_fury -> effectN( 1 ).percent();
      weapon_power_mod             = 0;
      base_dd_min                  = player -> dbc.spell_scaling( player -> o() -> type, player -> level );
      base_dd_max                  = player -> dbc.spell_scaling( player -> o() -> type, player -> level );
      if ( player -> o() -> talent.primal_elementalist -> ok() )
      {
        base_dd_min *= 1.5 * 1.2;
        base_dd_max *= 1.5 * 1.2;
      }
    }

    virtual void execute()
    {
      // If we're casting Immolate, we should clip a swing
      if ( time_to_execute > timespan_t::zero() && player -> executing )
        schedule_execute();
      else
        melee_attack_t::execute();
    }

    virtual double composite_da_multiplier() const
    {
      double m = melee_attack_t::composite_da_multiplier();

      fire_elemental_t* p = static_cast< fire_elemental_t* >( player );
      if ( p -> o() -> mastery.enhanced_elements -> ok() )
        m *= 1.0 + p -> o() -> cache.mastery_value();

      return m;
    }
  };

  struct auto_melee_attack_t : public melee_attack_t
  {
    auto_melee_attack_t( fire_elemental_t* player ) :
      melee_attack_t( "auto_attack", player )
    {
      player -> main_hand_attack = new fire_melee_t( player );
      player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
      player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player  -> is_moving() ) return false;
      return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  shaman_t* o() { return static_cast< shaman_t* >( owner ); }

  const spell_data_t* command;

  fire_elemental_t( sim_t* sim, shaman_t* owner, bool guardian ) :
    pet_t( sim, owner, ( ! guardian ) ? "primal_fire_elemental" : "greater_fire_elemental", guardian /*GUARDIAN*/ )
  {
    stamina_per_owner      = 1.0;
    command = owner -> find_spell( 65222 );
  }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = 32268; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 8908; // Level 85 value

    //mana_per_intellect               = 4.5;
    //hp_per_stamina = 10.5;

    // FE Swings with a weapon, however the damage is "magical"
    main_hand_weapon.type            = WEAPON_BEAST;
    main_hand_weapon.min_dmg         = 0;
    main_hand_weapon.max_dmg         = 0;
    main_hand_weapon.damage          = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time      = timespan_t::from_seconds( 1.4 );

    owner_coeff.sp_from_sp = 0.36;

    if ( o() -> talent.primal_elementalist -> ok() )
      owner_coeff.sp_from_sp *= 1.5 * 1.2;
  }

  void init_action_list()
  {
    action_list_str = "travel/auto_attack/fire_blast/fire_nova,if=active_enemies>=3";
    if ( type == PLAYER_PET )
      action_list_str += "/immolate,if=!ticking";

    pet_t::init_action_list();
  }

  virtual resource_e primary_resource() const { return RESOURCE_MANA; }

  double composite_player_multiplier( school_e school ) const
  {
    double m = pet_t::composite_player_multiplier( school );

    if ( owner -> race == RACE_ORC )
      m *= 1.0 + command -> effectN( 1 ).percent();

    return m;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t( this );
    if ( name == "fire_blast"  ) return new fire_blast_t( this, options_str );
    if ( name == "fire_nova"   ) return new fire_nova_t( this, options_str );
    if ( name == "auto_attack" ) return new auto_melee_attack_t( this );
    if ( name == "immolate"    ) return new immolate_t( this, options_str );

    return pet_t::create_action( name, options_str );
  }
};

// TODO: Orc racial

struct lightning_elemental_t : public pet_t
{
  struct lightning_blast_t : public spell_t
  {
    lightning_blast_t( lightning_elemental_t* p ) :
      spell_t( "lightning_blast", p, p -> find_spell( 145002 ) )
    {
      base_costs[ RESOURCE_MANA ] = 0;
      may_crit = true;
    }

    double composite_haste() const
    { return 1.0; }

    void init()
    {
      spell_t::init();

      shaman_t* s = debug_cast< shaman_t* >( static_cast<lightning_elemental_t*>(player) -> owner );
      if ( ! player -> sim -> report_pets_separately && player != s -> guardian_lightning_elemental[ 0 ] )
        stats = s -> guardian_lightning_elemental[ 0 ] -> get_stats( name() );
    }
  };

  lightning_elemental_t( shaman_t* owner ) :
    pet_t( owner -> sim, owner, "lightning_elemental", true, true )
  { stamina_per_owner      = 1.0; }

  void init_base_stats()
  {
    pet_t::init_base_stats();
    owner_coeff.sp_from_sp = 1.0;
  }

  action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "lightning_blast" ) return new lightning_blast_t( this );
    return pet_t::create_action( name, options_str );
  }

  void init_action_list()
  {
    action_list_str = "lightning_blast";

    pet_t::init_action_list();
  }

  resource_e primary_resource() const
  { return RESOURCE_MANA; }
};

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

// trigger_maelstrom_weapon =================================================

static bool trigger_maelstrom_weapon( shaman_attack_t* a )
{
  assert( a -> weapon );

  if ( a -> player -> specialization() != SHAMAN_ENHANCEMENT )
    return false;

  double chance = a -> weapon -> proc_chance_on_swing( 10.0 );

  if ( a -> p() -> sets.has_set_bonus( SET_PVP_2PC_MELEE ) )
    chance *= 1.2;

  return a -> p() -> buff.maelstrom_weapon -> trigger( a, 1, chance );
}

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( shaman_attack_t* a )
{
  shaman_t* p = a -> p();

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    p -> flametongue_mh -> execute();
  else
    p -> flametongue_oh -> execute();
}

// trigger_windfury_weapon ==================================================

struct windfury_delay_event_t : public event_t
{
  shaman_attack_t* wf;

  windfury_delay_event_t( shaman_attack_t* wf, timespan_t delay ) :
    event_t( *wf -> p(), "windfury_delay_event" ), wf( wf )
  {
    add_event( delay );
  }

  virtual void execute()
  {
    shaman_t* p = wf -> p();

    p -> proc.windfury -> occur();
    wf -> execute();
    wf -> execute();
    wf -> execute();
  }
};

static bool trigger_windfury_weapon( shaman_attack_t* a )
{
  shaman_t* p = a -> p();
  shaman_attack_t* wf = 0;

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    wf = p -> windfury_mh;
  else
    wf = p -> windfury_oh;

  if ( p -> rng().roll( p -> spell.windfury_driver -> proc_chance() ) )
  {
    p -> cooldown.feral_spirits -> ready -= timespan_t::from_seconds( p -> sets.set( SET_T15_4PC_MELEE ) -> effectN( 1 ).base_value() );

    // Delay windfury by some time, up to about a second
    new ( *p -> sim ) windfury_delay_event_t( wf, p -> rng().gauss( p -> wf_delay, p -> wf_delay_stddev ) );
    return true;
  }
  return false;
}

// trigger_rolling_thunder ==================================================

static bool trigger_rolling_thunder( shaman_spell_t* s )
{
  shaman_t* p = s -> p();

  if ( ! p -> buff.lightning_shield -> check() )
    return false;

  if ( s -> rng().roll( p -> spec.rolling_thunder -> proc_chance() ) )
  {
    if ( p -> buff.lightning_shield -> check() == 1 )
      p -> ls_reset = s -> sim -> current_time;

    p -> resource_gain( RESOURCE_MANA,
                        p -> dbc.spell( 88765 ) -> effectN( 1 ).percent() * p -> resources.max[ RESOURCE_MANA ],
                        p -> gain.rolling_thunder );


    int stacks = ( p -> sets.has_set_bonus( SET_T14_4PC_CASTER ) ) ? 2 : 1;
    int wasted_stacks = ( p -> buff.lightning_shield -> check() + stacks ) - p -> buff.lightning_shield -> max_stack();

    for ( int i = 0; i < wasted_stacks; i++ )
    {
      if ( s -> sim -> current_time - p -> ls_reset >= p -> cooldown.shock -> duration )
        p -> proc.wasted_ls -> occur();
      else
        p -> proc.wasted_ls_shock_cd -> occur();
    }

    if ( wasted_stacks > 0 )
    {
      if ( s -> sim -> current_time - p -> ls_reset < p -> cooldown.shock -> duration )
        p -> proc.ls_fast -> occur();
      p -> ls_reset = timespan_t::zero();
    }

    p -> buff.lightning_shield -> trigger( stacks );
    p -> proc.rolling_thunder  -> occur();
    return true;
  }

  return false;
}

// trigger_improved_lava_lash ===============================================

static bool trigger_improved_lava_lash( shaman_attack_t* a )
{
  struct improved_lava_lash_t : public shaman_spell_t
  {
    improved_lava_lash_t( shaman_t* p ) :
      shaman_spell_t( "improved_lava_lash", p )
    {
      const spell_data_t* lava_lash = p -> find_specialization_spell( "Lava Lash" );

      may_miss = may_crit = false;
      proc = true;
      callbacks = false;
      background = true;
      dual = true;

      aoe = lava_lash -> effectN( 4 ).base_value() +
            p -> perk.improved_lava_lash -> effectN( 1 ).base_value();
    }

    // Exclude targets with your flame shock on
    size_t available_targets( std::vector< player_t* >& tl ) const
    {
      tl.clear();

      for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
      {
        if ( sim -> actor_list[ i ] -> is_sleeping() )
          continue;

        if ( ! sim -> actor_list[ i ] -> is_enemy() )
          continue;

        if ( sim -> actor_list[ i ] == target )
          continue;

        shaman_td_t* main_target_td = this -> td( target );
        shaman_td_t* td = this -> td( sim -> actor_list[ i ] );
        dot_t* target_dot = main_target_td -> dot.flame_shock;
        dot_t* spread_dot = td -> dot.flame_shock;

        if ( spread_dot -> remains() > target_dot -> remains() )
          continue;

        tl.push_back( sim -> actor_list[ i ] );
      }

      return tl.size();
    }

    std::vector< player_t* >& target_list() const
    {
      size_t total_targets = available_targets( target_cache.list );

      // Reduce targets to aoe amount by removing random entries from the
      // target list until it's at aoe amount
      while ( total_targets > static_cast< size_t >( aoe ) )
      {
        bool removed = false;

        // Remove targets that have a flame shock first
        // TODO: The flame shocked targets should be randomly removed too, but
        // this will have to do for now
        for ( size_t i = 0; i < target_cache.list.size(); i++ )
        {
          shaman_td_t* td = this -> td( target_cache.list[ i ] );
          if ( td -> dot.flame_shock -> ticking )
          {
            target_cache.list.erase( target_cache.list.begin() + i );
            removed = true;
            break;
          }
        }

        // There's no flame shocked targets to remove, eliminate a random target
        if ( ! removed )
          target_cache.list.erase( target_cache.list.begin() + static_cast< size_t >( const_cast<improved_lava_lash_t*>(this) -> rng().range( 0, as<double>( target_cache.list.size() ) ) ) );

        total_targets--;
      }

      return target_cache.list;
    }

    // A simple impact method that triggers the proxy flame shock application
    // on the selected target of the lava lash spread driver
    void impact( action_state_t* state )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "%s spreads Flame Shock (off of %s) on %s",
                       player -> name(),
                       target -> name(),
                       state -> target -> name() );

      dot_t* dot = td( target ) -> dot.flame_shock;
      if ( dot -> ticking )
      {
        if ( ! td( state -> target ) -> dot.flame_shock -> ticking )
          p() -> active_flame_shocks++;
        dot -> copy( state -> target );
      }
    }
  };

  // Do not spread the love when there is only poor Fluffy Pillow against you
  if ( a -> sim -> num_enemies == 1 )
    return false;

  if ( ! a -> td( a -> target ) -> dot.flame_shock -> ticking )
    return false;

  shaman_t* p = a -> p();

  if ( p -> glyph.lava_lash -> ok() )
    return false;

  if ( ! p -> action_improved_lava_lash )
  {
    p -> action_improved_lava_lash = new improved_lava_lash_t( p );
    p -> action_improved_lava_lash -> init();
  }

  // Splash from the action's target
  p -> action_improved_lava_lash -> target = a -> target;
  p -> action_improved_lava_lash -> execute();

  return true;
}

static bool trigger_lightning_strike( const action_state_t* s )
{
  struct lightning_strike_t : public shaman_spell_t
  {
    lightning_strike_t( shaman_t* player ) :
      shaman_spell_t( "t15_lightning_strike", player,
                      player -> sets.set( SET_T15_2PC_CASTER ) -> effectN( 1 ).trigger() )
    {
      proc = background = true;
      callbacks = false;
      aoe = -1;
    }

    double composite_da_multiplier() const
    {
      double m = shaman_spell_t::composite_da_multiplier();
      m *= 1 / static_cast< double >( target_cache.list.size() );
      return m;
    }
  };

  shaman_t* p = debug_cast< shaman_t* >( s -> action -> player );

  if ( !  p -> sets.has_set_bonus( SET_T15_2PC_CASTER ) )
    return false;

  if ( ! p -> action_lightning_strike )
  {
    p -> action_lightning_strike = new lightning_strike_t( p );
    p -> action_lightning_strike -> init();
  }

  // Do Echo of the Elements procs trigger this?
  if ( p -> rng().roll( p -> sets.set( SET_T15_2PC_CASTER ) -> proc_chance() ) )
  {
    p -> action_lightning_strike -> execute();
    return true;
  }

  return false;
}

// trigger_tier16_2pc_melee =================================================

static bool trigger_tier16_2pc_melee( const action_state_t* s )
{
  shaman_t* p = debug_cast< shaman_t* >( s -> action -> player );

  if ( s -> action -> proc )
    return false;

  if ( ! p -> rng().roll( p -> buff.tier16_2pc_melee -> data().proc_chance() ) )
    return false;

  p -> proc.t16_2pc_melee -> occur();

  switch ( static_cast< int >( p -> rng().range( 0, 2 ) ) )
  {
    // Windfury
    case 0:
      p -> t16_wind -> execute();
      break;
    // Flametongue
    case 1:
      p -> t16_flame -> execute();
      break;
    default:
      assert( false );
      break;
  }

  return true;
}

// trigger_tier16_4pc_melee ================================================

static bool trigger_tier16_4pc_melee( const action_state_t* s )
{
  shaman_t* p = debug_cast< shaman_t* >( s -> action -> player );

  if ( p -> cooldown.t16_4pc_melee -> down() )
    return false;

  if ( ! p -> rng().roll( p -> sets.set( SET_T16_4PC_MELEE ) -> proc_chance() ) )
    return false;

  p -> cooldown.t16_4pc_melee -> start( p -> sets.set( SET_T16_4PC_MELEE ) -> internal_cooldown() );

  p -> proc.t16_4pc_melee -> occur();
  p -> cooldown.lava_lash -> reset( true );

  return true;
}
// trigger_tier16_4pc_caster ================================================

static bool trigger_tier16_4pc_caster( const action_state_t* s )
{
  shaman_t* p = debug_cast< shaman_t* >( s -> action -> player );

  if ( p -> cooldown.t16_4pc_caster -> down() )
    return false;

  if ( ! p -> rng().roll( p -> sets.set( SET_T16_4PC_CASTER ) -> proc_chance() ) )
    return false;

  p -> cooldown.t16_4pc_caster -> start( p -> sets.set( SET_T16_4PC_CASTER ) -> internal_cooldown() );

  for ( size_t i = 0; i < sizeof_array( p -> guardian_lightning_elemental ); i++ )
  {
    pet_t* pet = p -> guardian_lightning_elemental[ i ];

    if ( ! pet -> is_sleeping() )
      continue;

    pet -> summon( p -> sets.set( SET_T16_4PC_CASTER ) -> effectN( 1 ).trigger() -> duration() );
    p -> proc.t16_4pc_caster -> occur();
    break;
  }

  return true;
}

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct lava_burst_overload_t : public shaman_spell_t
{
  lava_burst_overload_t( shaman_t* player ) :
    shaman_spell_t( "lava_burst_overload", player, player -> dbc.spell( 77451 ) )
  {
    base_multiplier     *= 1.0 + player -> perk.improved_lava_burst -> effectN( 1 ).percent();
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> check() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    shaman_td_t* td = this -> td( target );
    if ( td -> debuff.unleashed_fury -> check() )
      m *= 1.0 + td -> debuff.unleashed_fury -> data().effectN( 2 ).percent();

    //if ( td -> dot.flame_shock -> ticking )
     //m *= 1.0 + p() -> spell.flame_shock -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* ) const
  { return 1.0; }

  void execute()
  {
    shaman_spell_t::execute();

    // FIXME: DBC Value modified in dbc_t::apply_hotfixes()
    p() -> cooldown.ascendance -> ready -= p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).time_value();
  }
};

struct lightning_bolt_overload_t : public shaman_spell_t
{
  lightning_bolt_overload_t( shaman_t* player ) :
    shaman_spell_t( "lightning_bolt_overload", player, player -> dbc.spell( 45284 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
    base_multiplier     += player -> spec.shamanism -> effectN( 1 ).percent();
    base_multiplier     *= 1.0 + player -> perk.improved_lightning_bolt -> effectN( 1 ).percent();
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    if ( td( target ) -> debuff.unleashed_fury -> up() )
      m *= 1.0 + td( target ) -> debuff.unleashed_fury -> data().effectN( 1 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );
      trigger_lightning_strike( state );
    }
  }
};

struct chain_lightning_overload_t : public shaman_spell_t
{
  chain_lightning_overload_t( shaman_t* player ) :
    shaman_spell_t( "chain_lightning_overload", player, player -> dbc.spell( 45297 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
    base_multiplier     += player -> spec.shamanism -> effectN( 2 ).percent();
    aoe                  = 3 + player -> glyph.chain_lightning -> effectN( 1 ).base_value();
    base_add_multiplier  = data().effectN( 1 ).chain_multiplier();
  }

  double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    // Glyph of Chain Lightning is bugged on live, and does not reduce the 
    // damage Chain Lightning overloads do. It does provide the extra targets,
    // though.
    if ( ! p() -> bugs )
      m *= 1.0 + p() -> glyph.chain_lightning -> effectN( 2 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      trigger_lightning_strike( state );
    }
  }
};

struct lava_beam_overload_t : public shaman_spell_t
{
  lava_beam_overload_t( shaman_t* player ) :
    shaman_spell_t( "lava_beam_overload", player, player -> dbc.spell( 114738 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
    base_costs[ RESOURCE_MANA ] = 0;
    base_multiplier     += p() -> spec.shamanism -> effectN( 2 ).percent();
    aoe                  = 5;
    base_add_multiplier  = data().effectN( 1 ).chain_multiplier();
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );
      trigger_lightning_strike( state );
    }
  }
};

struct elemental_blast_overload_t : public shaman_spell_t
{
  elemental_blast_overload_t( shaman_t* player ) :
    shaman_spell_t( "elemental_blast_overload", player, player -> dbc.spell( 120588 ) )
  {
    overload             = true;
    background           = true;
    base_execute_time    = timespan_t::zero();
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> check() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }
};

struct lightning_charge_t : public shaman_spell_t
{
  int threshold;

  lightning_charge_t( const std::string& n, shaman_t* player ) :
    shaman_spell_t( n, player, player -> dbc.spell( 26364 ) ),
    threshold( static_cast< int >( player -> spec.fulmination -> effectN( 1 ).base_value() ) )
  {
    // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    background       = true;
    may_crit         = true;

    if ( player -> spec.fulmination -> ok() )
      id = player -> spec.fulmination -> id();
  }

  double composite_target_crit( player_t* target ) const
  {
    double c = shaman_spell_t::composite_target_crit( target );


    if ( td( target ) -> debuff.stormstrike -> check() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( threshold > 0 )
    {
      int consuming_stack =  p() -> buff.lightning_shield -> check() - threshold;
      if ( consuming_stack > 0 )
        m *= consuming_stack;
    }

    return m;
  }
};

// TODO: Does unleash flame benefit from Unleash Flame +firedamage bonus?
struct unleash_flame_t : public shaman_spell_t
{
  unleash_flame_t( const std::string& name, shaman_t* player ) :
    shaman_spell_t( name, player, player -> dbc.spell( 73683 ) )
  {
    harmful              = true;
    background           = true;
    //proc                 = true;

    // Don't cooldown here, unleash elements ability will handle it
    cooldown -> duration = timespan_t::zero();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    p() -> buff.unleash_flame -> trigger();
    if ( result_is_hit( execute_state -> result ) && p() -> talent.unleashed_fury -> ok() )
      td( execute_state -> target ) -> debuff.unleashed_fury -> trigger();
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  flametongue_weapon_spell_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_spell_t( n, player, player -> find_spell( 10444 ) )
  {
    may_crit = background = true;

    if ( player -> specialization() == SHAMAN_ENHANCEMENT )
    {
      snapshot_flags          = STATE_AP;
      attack_power_mod.direct = w -> swing_time.total_seconds() / 2.6 * 0.15;
    }
  }
};

struct ancestral_awakening_t : public shaman_heal_t
{
  ancestral_awakening_t( shaman_t* player ) :
    shaman_heal_t( "ancestral_awakening", player, player -> find_spell( 52752 ) )
  {
    background = proc = true;
  }

  double composite_da_multiplier() const
  {
    double m = shaman_heal_t::composite_da_multiplier();
    m *= p() -> spec.ancestral_awakening -> effectN( 1 ).percent();
    return m;
  }

  void execute()
  {
    target = find_lowest_player();
    shaman_heal_t::execute();
  }
};

struct windfury_weapon_melee_attack_t : public shaman_attack_t
{
  windfury_weapon_melee_attack_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_attack_t( n, player, player -> dbc.spell( 25504 ) )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    background       = true;
    may_proc_windfury = false;
  }
};

struct unleash_wind_t : public shaman_attack_t
{
  unleash_wind_t( const std::string& name, shaman_t* player ) :
    shaman_attack_t( name, player, player -> dbc.spell( 73681 ) )
  {
    background = true;
    may_proc_maelstrom = may_proc_primal_wisdom = may_dodge = may_parry = false;
    normalize_weapon_speed = true;

    // Unleash wind implicitly uses main hand weapon of the player to perform
    // the damaging attack
    weapon = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    // Don't cooldown here, unleash elements will handle it
    cooldown -> duration = timespan_t::zero();
  }

  void execute()
  {
    shaman_attack_t::execute();
    p() -> buff.unleash_wind -> trigger( p() -> buff.unleash_wind -> data().initial_stacks() );
    if ( result_is_hit( execute_state -> result ) && p() -> talent.unleashed_fury -> ok() )
      p() -> buff.unleashed_fury_wf -> trigger();
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  const spell_data_t* lightning_shield;

  stormstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    shaman_attack_t( n, player, s ),
    lightning_shield( player -> find_spell( 324 ) )
  {
    may_proc_windfury = background = true;
    may_miss = may_dodge = may_parry = false;
    weapon = w;
    base_multiplier *= 1.0 + p() -> perk.improved_stormstrike -> effectN( 1 ).percent();
  }

  double action_multiplier() const
  {
    double m = shaman_attack_t::action_multiplier();

    if ( p() -> buff.lightning_shield -> up() )
      m *= 1.0 + lightning_shield -> effectN( 3 ).percent();

    return m;
  }
};

struct windstrike_attack_t : public stormstrike_attack_t
{
  windstrike_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    stormstrike_attack_t( n, player, s, w )
  { }

  double target_armor( player_t* ) const
  { return 0.0; }
};

struct windlash_t : public shaman_attack_t
{
  double swing_timer_variance;

  windlash_t( const std::string& n, const spell_data_t* s, shaman_t* player, weapon_t* w, double stv ) :
    shaman_attack_t( n, player, s ), swing_timer_variance( stv )
  {
    may_proc_windfury = background = repeating = may_miss = may_dodge = may_parry = true;
    may_glance = special = false;
    weapon            = w;
    base_execute_time = w -> swing_time;
    trigger_gcd       = timespan_t::zero();
  }

  double target_armor( player_t* ) const
  { return 0.0; }

  timespan_t execute_time() const
  {
    timespan_t t = shaman_attack_t::execute_time();

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds( const_cast<windlash_t*>(this) -> rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );

      return st;
    }
    else
      return t;
  }

  void execute()
  {
    if ( time_to_execute > timespan_t::zero() && p() -> executing )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "Executing '%s' during melee (%s).", p() -> executing -> name(), util::slot_type_string( weapon -> slot ) );

      if ( weapon -> slot == SLOT_OFF_HAND )
        p() -> proc.swings_clipped_oh -> occur();
      else
        p() -> proc.swings_clipped_mh -> occur();

      schedule_execute();
    }
    else
    {
      p() -> buff.flurry -> decrement();
      p() -> buff.unleash_wind -> decrement();

      shaman_attack_t::execute();
    }
  }
};

// ==========================================================================
// Shaman Action / Spell Base
// ==========================================================================

// shaman_spell_base_t::schedule_execute ====================================

template <class Base>
void shaman_spell_base_t<Base>::schedule_execute( action_state_t* state )
{
  shaman_t* p = ab::p();

  if ( p -> sim -> log )
    p -> sim -> out_log.printf( "%s schedules execute for %s", p -> name(), ab::name() );

  ab::time_to_execute = execute_time();

  ab::execute_event = ab::start_action_execute_event( ab::time_to_execute, state );

  if ( ! ab::background )
  {
    p -> executing = this;
    p -> gcd_ready = p -> sim -> current_time + ab::gcd();
    if ( p -> action_queued && p -> sim -> strict_gcd_queue )
      p -> gcd_ready -= p -> sim -> queue_gcd_reduction;
  }

  if ( ab::instant_eligibility() && ! use_ancestral_swiftness() && p -> specialization() == SHAMAN_ENHANCEMENT )
    maelstrom_weapon_cast[ p -> buff.maelstrom_weapon -> check() ] -> add( 1 );
}

// shaman_spell_base_t::execute =============================================

template <class Base>
void shaman_spell_base_t<Base>::execute()
{
  ab::execute();

  bool as_cast = use_ancestral_swiftness();
  bool eligible_for_instant = ab::instant_eligibility();
  shaman_t* p = ab::p();

  // Ancestral swiftness handling, note that MW / AS share the same "conditions" for use
  // Maelstromable nature spell with less than 5 stacks is going to consume ancestral swiftness
  if ( as_cast )
  {
    p -> buff.ancestral_swiftness -> expire();
    p -> cooldown.ancestral_swiftness -> start();
  }

  // Shamans have specialized swing timer reset system, where every cast time spell
  // resets the swing timers, _IF_ the spell is not maelstromable, or the maelstrom
  // weapon stack is zero, or ancient swiftness was not used to cast the spell
  if ( ( ! eligible_for_instant || p -> buff.maelstrom_weapon -> check() == 0 ) &&
       ! as_cast && execute_time() > timespan_t::zero() )
  {
    if ( ab::sim -> debug )
    {
      ab::sim -> out_debug.printf( "Resetting swing timers for '%s', instant_eligibility=%d mw_stacks=%d",
                         ab::name_str.c_str(), eligible_for_instant, p -> buff.maelstrom_weapon -> check() );
    }

    timespan_t time_to_next_hit;

    if ( ab::player -> main_hand_attack && ab::player -> main_hand_attack -> execute_event )
    {
      time_to_next_hit = ab::player -> main_hand_attack -> execute_time();
      ab::player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      p -> proc.swings_reset_mh -> occur();
    }

    if ( ab::player -> off_hand_attack && ab::player -> off_hand_attack -> execute_event )
    {
      time_to_next_hit = ab::player -> off_hand_attack -> execute_time();
      ab::player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      p -> proc.swings_reset_oh -> occur();
    }
  }

  if ( eligible_for_instant && ! as_cast && p -> specialization() == SHAMAN_ENHANCEMENT )
  {
    maelstrom_weapon_executed[ p -> buff.maelstrom_weapon -> check() ] -> add( 1 );
    p -> buff.maelstrom_weapon -> expire();
  }

  p -> buff.spiritwalkers_grace -> up();
}

// shaman_heal_t::impact ====================================================

void shaman_heal_t::impact( action_state_t* s )
{
  // Todo deep healing to adjust s -> result_amount by x% before impacting
  if ( sim -> debug && p() -> mastery.deep_healing -> ok() )
  {
    sim -> out_debug.printf( "%s Deep Heals %s@%.2f%% mul=%.3f %.0f -> %.0f",
                   player -> name(), s -> target -> name(),
                   s -> target -> health_percentage(), deep_healing( s ),
                   s -> result_amount, s -> result_amount * deep_healing( s ) );
  }

  s -> result_amount *= deep_healing( s );

  base_t::impact( s );

  if ( proc_tidal_waves )
    p() -> buff.tidal_waves -> trigger( p() -> buff.tidal_waves -> data().initial_stacks() );

  if ( s -> result == RESULT_CRIT )
  {
    if ( resurgence_gain > 0 )
      p() -> resource_gain( RESOURCE_MANA, resurgence_gain, p() -> gain.resurgence );

    if ( p() -> spec.ancestral_awakening -> ok() )
    {
      if ( ! p() -> action_ancestral_awakening )
      {
        p() -> action_ancestral_awakening = new ancestral_awakening_t( p() );
        p() -> action_ancestral_awakening -> init();
      }

      p() -> action_ancestral_awakening -> base_dd_min = s -> result_total;
      p() -> action_ancestral_awakening -> base_dd_max = s -> result_total;
    }
  }

  if ( p() -> main_hand_weapon.buff_type == EARTHLIVING_IMBUE )
  {
    double chance = ( s -> target -> resources.pct( RESOURCE_HEALTH ) > .35 ) ? elw_proc_high : elw_proc_low;

    if ( rng().roll( chance ) )
    {
      // Todo proc earthliving on target
    }
  }
}

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_attack_t::impact ============================================

void shaman_attack_t::impact( action_state_t* state )
{
  base_t::impact( state );

  // Bail out early if the result is a miss/dodge/parry
  if ( ! result_is_hit( state -> result ) )
    return;

  if ( may_proc_maelstrom )
    trigger_maelstrom_weapon( this );

  if ( may_proc_windfury && weapon && weapon -> buff_type == WINDFURY_IMBUE )
    trigger_windfury_weapon( this );

  if ( may_proc_flametongue && weapon && weapon -> buff_type == FLAMETONGUE_IMBUE )
    trigger_flametongue_weapon( this );

  if ( may_proc_primal_wisdom && rng().roll( p() -> spec.primal_wisdom -> proc_chance() ) )
  {
    double amount = p() -> spell.primal_wisdom -> effectN( 1 ).percent() * p() -> resources.base[ RESOURCE_MANA ];
    p() -> resource_gain( RESOURCE_MANA, amount, p() -> gain.primal_wisdom );
  }

  if ( state -> result == RESULT_CRIT )
    p() -> buff.flurry -> trigger( p() -> buff.flurry -> data().initial_stacks() );

  if ( p() -> buff.tier16_2pc_melee -> up() )
    trigger_tier16_2pc_melee( state );
}

// Melee Attack =============================================================

struct melee_t : public shaman_attack_t
{
  int sync_weapons;
  bool first;
  double swing_timer_variance;

  melee_t( const std::string& name, const spell_data_t* s, shaman_t* player, weapon_t* w, int sw, double stv ) :
    shaman_attack_t( name, player, s ), sync_weapons( sw ),
    first( true ), swing_timer_variance( stv )
  {
    auto_attack = true;
    may_proc_windfury = background = repeating = may_glance = true;
    special           = false;
    trigger_gcd       = timespan_t::zero();
    weapon            = w;
    base_execute_time = w -> swing_time;

    if ( p() -> specialization() == SHAMAN_ENHANCEMENT && p() -> dual_wield() )
      base_hit -= 0.19;
  }

  void reset()
  {
    shaman_attack_t::reset();

    first = true;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = shaman_attack_t::execute_time();
    if ( first )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 ) : timespan_t::zero();
    }

    if ( swing_timer_variance > 0 )
    {
      timespan_t st = timespan_t::from_seconds(const_cast<melee_t*>(this) ->  rng().gauss( t.total_seconds(), t.total_seconds() * swing_timer_variance ) );
      if ( sim -> debug )
        sim -> out_debug.printf( "Swing timer variance for %s, real_time=%.3f swing_timer=%.3f", name(), t.total_seconds(), st.total_seconds() );
      return st;
    }
    else
      return t;
  }

  void execute()
  {
    if ( first )
    {
      first = false;
    }
    if ( time_to_execute > timespan_t::zero() && p() -> executing )
    {
      if ( sim -> debug )
        sim -> out_debug.printf( "Executing '%s' during melee (%s).", p() -> executing -> name(), util::slot_type_string( weapon -> slot ) );

      if ( weapon -> slot == SLOT_OFF_HAND )
        p() -> proc.swings_clipped_oh -> occur();
      else
        p() -> proc.swings_clipped_mh -> occur();

      schedule_execute();
    }
    else
    {
      p() -> buff.flurry -> decrement();
      p() -> buff.unleash_wind -> decrement();

      shaman_attack_t::execute();
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;
  double swing_timer_variance;

  auto_attack_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 ), swing_timer_variance( 0.00 )
  {
    option_t options[] =
    {
      opt_bool( "sync_weapons", sync_weapons ),
      opt_float( "swing_timer_variance", swing_timer_variance ),
      opt_null()
    };
    parse_options( options, options_str );

    assert( p() -> main_hand_weapon.type != WEAPON_NONE );

    p() -> melee_mh      = new melee_t( "auto_attack_mh", spell_data_t::nil(), player, &( p() -> main_hand_weapon ), sync_weapons, swing_timer_variance );
    p() -> melee_mh      -> school = SCHOOL_PHYSICAL;
    p() -> ascendance_mh = new windlash_t( "wind_lash_mh", player -> find_spell( 114089 ), player, &( p() -> main_hand_weapon ), swing_timer_variance );

    p() -> main_hand_attack = p() -> melee_mh;

    if ( p() -> off_hand_weapon.type != WEAPON_NONE && p() -> specialization() == SHAMAN_ENHANCEMENT )
    {
      if ( ! p() -> dual_wield() ) return;

      p() -> melee_oh = new melee_t( "auto_attack_oh", spell_data_t::nil(), player, &( p() -> off_hand_weapon ), sync_weapons, swing_timer_variance );
      p() -> melee_oh -> school = SCHOOL_PHYSICAL;
      p() -> ascendance_oh = new windlash_t( "wind_lash_offhand", player -> find_spell( 114093 ), player, &( p() -> off_hand_weapon ), swing_timer_variance );

      p() -> off_hand_attack = p() -> melee_oh;

      p() -> off_hand_attack -> id = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();
    if ( p() -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( p() -> is_moving() ) return false;
    return ( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  double ft_bonus;

  lava_lash_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "lava_lash", player, player -> find_class_spell( "Lava Lash" ) ),
    ft_bonus( data().effectN( 2 ).percent() )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    uses_eoe = may_proc_windfury = true;
    school = SCHOOL_FIRE;

    base_multiplier += player -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
    base_multiplier *= 1.0 + player -> perk.improved_lava_lash_2 -> effectN( 1 ).percent();

    parse_options( NULL, options_str );

    weapon              = &( player -> off_hand_weapon );

    if ( weapon -> type == WEAPON_NONE )
      background        = true; // Do not allow execution.
  }

  // Lava Lash multiplier calculation from
  // http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p11/#post1935780
  // MoP: Vastly simplified, most bonuses are gone
  virtual double action_da_multiplier() const
  {
    double m = shaman_attack_t::action_da_multiplier();

    m *= 1.0 + ( weapon -> buff_type == FLAMETONGUE_IMBUE ) * ft_bonus;

    return m;
  }

  void impact( action_state_t* state )
  {
    shaman_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( td( state -> target ) -> dot.flame_shock -> ticking )
        trigger_improved_lava_lash( this );

      p() -> buff.shocking_lava -> trigger();
    }
  }
};

// Primal Strike Attack =====================================================

struct primal_strike_t : public shaman_attack_t
{
  primal_strike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "primal_strike", player, player -> find_class_spell( "Primal Strike" ) )
  {
    parse_options( NULL, options_str );
    may_proc_windfury    = true;
    weapon               = &( p() -> main_hand_weapon );
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_attack_t * stormstrike_mh;
  stormstrike_attack_t * stormstrike_oh;

  stormstrike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "stormstrike", player, player -> find_class_spell( "Stormstrike" ) ),
    stormstrike_mh( 0 ), stormstrike_oh( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    parse_options( NULL, options_str );

    may_proc_eoe         = true;
    weapon               = &( p() -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    uses_eoe             = true;
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_attack_t
    // stormstrike_attack_t( std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    stormstrike_mh = new stormstrike_attack_t( "stormstrike_mh", player, data().effectN( 2 ).trigger(), &( player -> main_hand_weapon ) );
    add_child( stormstrike_mh );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      stormstrike_oh = new stormstrike_attack_t( "stormstrike_offhand", player, data().effectN( 3 ).trigger(), &( player -> off_hand_weapon ) );
      add_child( stormstrike_oh );
    }
  }

  void execute()
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
    {
      int bonus = p() -> sets.set( SET_T15_2PC_MELEE ) -> effectN( 1 ).base_value();

      p() -> buff.maelstrom_weapon -> trigger( this, bonus, 1.0 );
    }
  }

  void impact( action_state_t* state )
  {
    // Bypass shaman-specific attack based procs and co for this spell, the relevant ones
    // are handled by stormstrike_attack_t
    melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      td( state -> target ) -> debuff.stormstrike -> trigger();

      stormstrike_mh -> execute();
      if ( stormstrike_oh ) stormstrike_oh -> execute();
    }
  }

  bool ready()
  {
    if ( p() -> buff.ascendance -> check() )
      return false;

    return shaman_attack_t::ready();
  }
};

// Windstrike Attack ========================================================

struct windstrike_t : public shaman_attack_t
{
  windstrike_attack_t * windstrike_mh;
  windstrike_attack_t * windstrike_oh;

  windstrike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "windstrike", player, player -> find_spell( 115356 ) ),
    windstrike_mh( 0 ), windstrike_oh( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    parse_options( NULL, options_str );

    may_proc_eoe         = true;
    school               = SCHOOL_PHYSICAL;
    weapon               = &( p() -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    uses_eoe             = true;
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_attack_t
    windstrike_mh = new windstrike_attack_t( "windstrike_mh", player, data().effectN( 2 ).trigger(), &( player -> main_hand_weapon ) );
    windstrike_mh -> normalize_weapon_speed = true;
    windstrike_mh -> school = SCHOOL_PHYSICAL;
    add_child( windstrike_mh );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      windstrike_oh = new windstrike_attack_t( "windstrike_offhand", player, data().effectN( 3 ).trigger(), &( player -> off_hand_weapon ) );
      windstrike_oh -> normalize_weapon_speed = true;
      windstrike_oh -> school = SCHOOL_PHYSICAL;
      add_child( windstrike_oh );
    }
  }

  void execute()
  {
    shaman_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) && p() -> sets.has_set_bonus( SET_T15_2PC_MELEE ) )
    {
      int bonus = p() -> sets.set( SET_T15_2PC_MELEE ) -> effectN( 1 ).base_value();

      p() -> buff.maelstrom_weapon -> trigger( this, bonus, 1.0 );
    }
  }

  void impact( action_state_t* state )
  {
    // Bypass shaman-specific attack based procs and co for this spell, the relevant ones
    // are handled by stormstrike_attack_t
    melee_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      td( state -> target ) -> debuff.stormstrike -> trigger();

      windstrike_mh -> execute();
      if ( windstrike_oh ) windstrike_oh -> execute();
    }
  }

  bool ready()
  {
    if ( ! p() -> buff.ascendance -> check() )
      return false;

    return shaman_attack_t::ready();
  }
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

// shaman_spell_t::execute ==================================================

void shaman_spell_t::execute()
{
  base_t::execute();

  if ( ! totem && ! background && ! proc && ( data().school_mask() & SCHOOL_MASK_FIRE ) )
    p() -> buff.unleash_flame -> expire();

}

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "bloodlust", player, player -> find_class_spell( "Bloodlust" ), options_str )
  {
    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if ( p -> buffs.exhaustion -> check() || p -> is_pet() || p -> is_enemy() )
        continue;
      p -> buffs.bloodlust -> trigger();
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( sim -> overrides.bloodlust )
      return false;

    if ( p() -> buffs.exhaustion -> check() )
      return false;

    if (  p() -> buffs.bloodlust -> cooldown -> down() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning Spell ====================================================

struct chain_lightning_t : public shaman_spell_t
{
  chain_lightning_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "chain_lightning", player, player -> find_class_spell( "Chain Lightning" ), options_str )
  {
    cooldown -> duration += player -> spec.shamanism        -> effectN( 4 ).time_value();
    base_multiplier      += player -> spec.shamanism        -> effectN( 2 ).percent();
    aoe                   = player -> glyph.chain_lightning -> effectN( 1 ).base_value() + 3;
    base_add_multiplier   = data().effectN( 1 ).chain_multiplier();

    overload_spell        = new chain_lightning_overload_t( player );
    overload_chance_multiplier = 0.3;
    add_child( overload_spell );
  }

  double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    m *= 1.0 + p() -> glyph.chain_lightning -> effectN( 2 ).percent();

    return m;
  }

  double composite_target_crit( player_t* target ) const
  {
    double c = shaman_spell_t::composite_target_crit( target );


    if ( td( target ) -> debuff.stormstrike -> check() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.improved_chain_lightning -> trigger( execute_state -> n_targets );
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );
      trigger_lightning_strike( state );
    }

    trigger_tier16_4pc_caster( state );
  }

  bool ready()
  {
    if ( p() -> specialization() == SHAMAN_ELEMENTAL && p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Lava Beam Spell ==========================================================

struct lava_beam_t : public shaman_spell_t
{
  lava_beam_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lava_beam", player, player -> find_spell( 114074 ), options_str )
  {
    check_spec( SHAMAN_ELEMENTAL );

    base_execute_time    += p() -> spec.shamanism        -> effectN( 3 ).time_value();
    base_multiplier      += p() -> spec.shamanism        -> effectN( 2 ).percent();
    aoe                   = 5;
    base_add_multiplier   = data().effectN( 1 ).chain_multiplier();

    overload_spell        = new lava_beam_overload_t( player );
    overload_chance_multiplier = 0.3;
    add_child( overload_spell );
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> check() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.improved_chain_lightning -> trigger( execute_state -> n_targets );
  }

  void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );
      trigger_lightning_strike( state );
    }
  }

  bool ready()
  {
    if ( ! p() -> buff.ascendance -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_mastery", player, player -> talent.elemental_mastery, options_str )
  {
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.elemental_mastery -> trigger();
    if ( p() -> sets.has_set_bonus( SET_T13_2PC_CASTER ) )
      p() -> buff.tier13_2pc_caster -> trigger();
  }
};

// Ancestral Swiftness Spell ================================================

struct ancestral_swiftness_t : public shaman_spell_t
{
  ancestral_swiftness_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "ancestral_swiftness", player, player -> talent.ancestral_swiftness, options_str )
  {
    harmful = may_crit = may_miss = callbacks = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.ancestral_swiftness -> trigger();
  }
};

// Call of the Elements Spell ===============================================

struct call_of_the_elements_t : public shaman_spell_t
{
  call_of_the_elements_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "call_of_the_elements", player, player -> find_talent_spell( "Call of the Elements" ), options_str )
  {
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
  }
};

// Fire Nova Spell ==========================================================

struct fire_nova_explosion_t : public shaman_spell_t
{
  fire_nova_explosion_t( shaman_t* player ) :
    shaman_spell_t( "fire_nova_explosion", player, player -> find_spell( 8349 ) )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    aoe        = -1;
    background = true;
    dual = true;
  }

  void init()
  {
    shaman_spell_t::init();

    stats = player -> get_stats( "fire_nova" );
  }

  double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> check() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  // Fire nova does not damage the main target.
  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      if ( ! sim -> actor_list[ i ] -> is_sleeping() &&
             sim -> actor_list[ i ] -> is_enemy() &&
             sim -> actor_list[ i ] != target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }
};

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "fire_nova", player, player -> find_specialization_spell( "Fire Nova" ), options_str )
  {
    may_crit = may_miss = callbacks = false;
    uses_eoe  = true;
    aoe       = -1;

    impact_action = new fire_nova_explosion_t( player );
  }

  // Override assess_damage, as fire_nova_explosion is going to do all the
  // damage for us.
  void assess_damage( dmg_e type, action_state_t* s )
  { if ( s -> result_amount > 0 ) shaman_spell_t::assess_damage( type, s ); }

  bool ready()
  {
    if ( ! td( target ) -> dot.flame_shock -> ticking )
      return false;

    return shaman_spell_t::ready();
  }

  void execute()
  {
    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_fire_nova -> occur();

    shaman_spell_t::execute();
  }

  // Fire nova is emitted on all targets with a flame shock from us .. so
  std::vector< player_t* >& target_list() const
  {
    target_cache.list.clear();

    for ( size_t i = 0; i < sim -> target_non_sleeping_list.size(); ++i )
    {
      player_t* e = sim -> target_non_sleeping_list[ i ];
      if ( ! e -> is_enemy() )
        continue;

      if ( td( e ) -> dot.flame_shock -> ticking )
        target_cache.list.push_back( e );
    }

    return target_cache.list;
  }
};

// Earthquake Spell =========================================================

struct earthquake_rumble_t : public shaman_spell_t
{
  earthquake_rumble_t( shaman_t* player ) :
    shaman_spell_t( "earthquake_rumble", player, player -> find_spell( 77478 ) )
  {
    harmful = background = true;
    aoe = -1;
    school = SCHOOL_PHYSICAL;
  }

  void init()
  {
    shaman_spell_t::init();

    stats = player -> get_stats( "earthquake" );
  }

  virtual double composite_spell_power() const
  {
    double sp = shaman_spell_t::composite_spell_power();

    sp += player -> cache.spell_power( SCHOOL_NATURE );

    return sp;
  }

  virtual double target_armor( player_t* ) const
  {
    return 0;
  }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "earthquake", player, player -> find_specialization_spell( "Earthquake" ), options_str )
  {
    uses_eoe = player -> specialization() == SHAMAN_ELEMENTAL;
    hasted_ticks = may_miss = may_crit = false;

    base_td = base_dd_min = base_dd_max = attack_power_mod.direct = spell_power_mod.direct = 0;
    harmful = true;
    num_ticks = ( int ) data().duration().total_seconds();
    base_tick_time = data().effectN( 2 ).period();

    dynamic_tick_action = true;
    tick_action = new earthquake_rumble_t( player );
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  lava_burst_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lava_burst", player, player -> find_class_spell( "Lava Burst" ), options_str )
  {
    base_multiplier     *= 1.0 + player -> perk.improved_lava_burst -> effectN( 1 ).percent();
    uses_eoe = player -> specialization() == SHAMAN_ELEMENTAL;
    overload_spell          = new lava_burst_overload_t( player );
    add_child( overload_spell );
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> check() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  virtual double composite_hit() const
  {
    double m = shaman_spell_t::composite_hit();

    if ( p() -> specialization() == SHAMAN_RESTORATION )
      m += p() -> spec.spiritual_insight -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = shaman_spell_t::composite_target_multiplier( target );

    shaman_td_t* td = this -> td( target );
    if ( td -> debuff.unleashed_fury -> check() )
      m *= 1.0 + td -> debuff.unleashed_fury -> data().effectN( 2 ).percent();

    //if ( td -> dot.flame_shock -> ticking )
      //m *= 1.0 + p() -> spell.flame_shock -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* ) const
  { return 1.0; }

  virtual void update_ready( timespan_t /* cd_duration */ )
  {
    timespan_t d = cooldown -> duration;

    // Lava Surge has procced during the cast of Lava Burst, the cooldown 
    // reset is deferred to the finished cast, instead of "eating" it.
    if ( p() -> lava_surge_during_lvb )
      d = timespan_t::zero();

    shaman_spell_t::update_ready( d );
  }

  virtual void execute()
  {
    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_lava_burst -> occur();

    shaman_spell_t::execute();

    // FIXME: DBC Value modified in dbc_t::apply_hotfixes()
    p() -> cooldown.ascendance -> ready -= p() -> sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).time_value();

    // Lava Surge buff does not get eaten, if the Lava Surge proc happened
    // during the Lava Burst cast
    if ( ! p() -> lava_surge_during_lvb && p() -> buff.lava_surge -> check() )
      p() -> buff.lava_surge -> expire();

    p() -> lava_surge_during_lvb = false;
  }

  virtual void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      p() -> buff.shocking_lava -> trigger();
      trigger_rolling_thunder( this );
    }
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.lava_surge -> up() )
      return timespan_t::zero();

    return shaman_spell_t::execute_time();
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  lightning_bolt_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_bolt", player, player -> find_class_spell( "Lightning Bolt" ), options_str )
  {
    base_multiplier   += player -> spec.shamanism -> effectN( 1 ).percent();
    base_multiplier   *= 1.0 + player -> perk.improved_lightning_bolt -> effectN( 1 ).percent();
    base_execute_time += player -> spec.shamanism -> effectN( 3 ).time_value();
    overload_spell     = new lightning_bolt_overload_t( player );
    add_child( overload_spell );
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    m *= 1.0 + p() -> sets.set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();

    return m;
  }

  virtual double composite_hit() const
  {
    double m = shaman_spell_t::composite_hit();

    if ( p() -> specialization() == SHAMAN_RESTORATION )
      m += p() -> spec.spiritual_insight -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* target ) const
  {
    double c = shaman_spell_t::composite_target_crit( target );


    if ( td( target ) -> debuff.stormstrike -> check() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double m = shaman_spell_t::composite_target_multiplier( target );


    if ( td( target ) -> debuff.unleashed_fury -> check() )
      m *= 1.0 + td( target ) -> debuff.unleashed_fury -> data().effectN( 1 ).percent();

    return m;
  }

  virtual void impact( action_state_t* state )
  {
    shaman_spell_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      if ( p() -> glyph.telluric_currents -> ok() )
      {
        double mana_gain = p() -> glyph.telluric_currents -> effectN( 2 ).percent();
        p() -> resource_gain( RESOURCE_MANA,
                              p() -> resources.base[RESOURCE_MANA] * mana_gain,
                              p() -> gain.telluric_currents );
      }
    }

    if ( result_is_hit( state -> result ) )
      trigger_lightning_strike( state );

    trigger_tier16_4pc_caster( state );
  }
};

// Elemental Blast Spell ====================================================

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_blast", player, player -> talent.elemental_blast, options_str )
  {
    overload_spell = new elemental_blast_overload_t( player );
    add_child( overload_spell );

  }

  virtual void execute()
  {
    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_elemental_blast -> occur();

    shaman_spell_t::execute();
  }

  result_e calculate_result( action_state_t* s )
  {
    if ( ! s -> target ) return RESULT_NONE;

    int delta_level = s -> target -> level - player -> level;

    result_e result = RESULT_NONE;

    if ( ( result == RESULT_NONE ) && may_miss )
    {
      if ( rng().roll( miss_chance( composite_hit(), s -> target ) ) )
        result = RESULT_MISS;
    }

    if ( result == RESULT_NONE )
    {
      result = RESULT_HIT;
      unsigned max_buffs = 4 + ( p() -> specialization() == SHAMAN_ENHANCEMENT ? 1 : 0 );

      unsigned b = static_cast< unsigned >( rng().range( 0, max_buffs ) );
      assert( b < max_buffs );

      p() -> buff.elemental_blast_agility -> expire();
      p() -> buff.elemental_blast_crit -> expire();
      p() -> buff.elemental_blast_haste -> expire();
      p() -> buff.elemental_blast_mastery -> expire();
      p() -> buff.elemental_blast_multistrike -> expire();

      if ( b == 0 )
        p() -> buff.elemental_blast_crit -> trigger();
      else if ( b == 1 )
        p() -> buff.elemental_blast_haste -> trigger();
      else if ( b == 2 )
        p() -> buff.elemental_blast_mastery -> trigger();
      else if ( b == 3 )
        p() -> buff.elemental_blast_multistrike -> trigger();
      else
        p() -> buff.elemental_blast_agility -> trigger();

      if ( rng().roll( crit_chance( composite_crit() + composite_target_crit( s -> target ), delta_level ) ) )
        result = RESULT_CRIT;
    }

    if ( sim -> debug ) sim -> out_debug.printf( "%s result for %s is %s", player -> name(), name(), util::result_type_string( result ) );

    // Re-snapshot state here, after we have procced a buff spell. The new state
    // is going to be used to calculate the damage of this spell already
    snapshot_state( s, DMG_DIRECT );
    if ( sim -> debug ) s -> debug();

    return result;
  }

  virtual double composite_da_multiplier() const
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( p() -> buff.unleash_flame -> check() )
      m *= 1.0 + p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  virtual double composite_hit() const
  {
    double m = shaman_spell_t::composite_hit();

    if ( p() -> specialization() == SHAMAN_RESTORATION )
      m += p() -> spec.spiritual_insight -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* target ) const
  {
    double c = shaman_spell_t::composite_target_crit( target );

    if ( td( target ) -> debuff.stormstrike -> check() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }
};


// Shamanistic Rage Spell ===================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "shamanistic_rage", player, player -> find_class_spell( "Shamanistic Rage" ), options_str )
  {
    harmful   = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.shamanistic_rage -> trigger();
  }
};

// Spirit Wolf Spell ========================================================

struct feral_spirit_spell_t : public shaman_spell_t
{
  feral_spirit_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "feral_spirit", player, player -> find_class_spell( "Feral Spirit" ), options_str )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    harmful   = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    int n = 0;
    for ( size_t i = 0; i < sizeof_array( p() -> pet_feral_spirit ) && n < data().effectN( 1 ).base_value(); i++ )
    {
      if ( ! p() -> pet_feral_spirit[ i ] -> is_sleeping() )
        continue;

      p() -> pet_feral_spirit[ i ] -> summon( data().duration() );
      n++;
    }
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  double bonus;

  thunderstorm_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "thunderstorm", player, player -> find_class_spell( "Thunderstorm" ), options_str ), bonus( 0 )
  {
    check_spec( SHAMAN_ELEMENTAL );

    aoe = -1;
    cooldown -> duration += player -> glyph.thunder -> effectN( 1 ).time_value();
    bonus                 = data().effectN( 2 ).percent() +
                            player -> glyph.thunderstorm -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    player -> resource_gain( data().effectN( 2 ).resource_gain_type(),
                             player -> resources.max[ data().effectN( 2 ).resource_gain_type() ] * bonus,
                             p() -> gain.thunderstorm );
  }
};

// Unleash Elements Spell ===================================================

struct unleash_elements_t : public shaman_spell_t
{
  unleash_wind_t*   wind;
  unleash_flame_t* flame;

  unleash_elements_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "unleash_elements", player, player -> find_class_spell( "Unleash Elements" ), options_str ),
    wind( 0 ), flame( 0 )
  {
    may_crit     = false;
    may_miss     = false;
    may_proc_eoe = true;

    wind = new unleash_wind_t( "unleash_wind", player );
    flame = new unleash_flame_t( "unleash_flame", player );

    add_child( wind );
    add_child( flame );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( player -> main_hand_weapon.buff_type == WINDFURY_IMBUE ||
         player -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
      wind -> execute();
    if ( player -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE ||
         player -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      flame -> execute();

    p() -> buff.tier16_2pc_melee -> trigger();
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "spiritwalkers_grace", player, player -> find_class_spell( "Spiritwalker's Grace" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.spiritwalkers_grace -> trigger();

    if ( p() -> sets.has_set_bonus( SET_T13_4PC_HEAL ) )
      p() -> buff.tier13_4pc_healer -> trigger();
  }
};

struct spirit_walk_t : public shaman_spell_t
{
  spirit_walk_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "spirit_walk", player, player -> find_specialization_spell( "Spirit Walk" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.spirit_walk -> trigger();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================

struct earth_shock_t : public shaman_spell_t
{
  struct lightning_charge_delay_t : public event_t
  {
    buff_t* buff;
    int consume_stacks;
    int consume_threshold;

    lightning_charge_delay_t( shaman_t& p, buff_t* b, int consume, int consume_threshold ) :
      event_t( p, "lightning_charge_delay_t" ), buff( b ),
      consume_stacks( consume ), consume_threshold( consume_threshold )
    {
      add_event( timespan_t::from_seconds( 0.001 ) );
    }

    void execute()
    {
      if ( ( buff -> check() - consume_threshold ) > 0 )
        buff -> decrement( consume_stacks );
    }
  };

  int consume_threshold;

  earth_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "earth_shock", player, player -> find_class_spell( "Earth Shock" ), options_str ),
    consume_threshold( ( int ) player -> spec.fulmination -> effectN( 1 ).base_value() )
  {
    base_multiplier      *= 1.0 + player -> perk.improved_shocks -> effectN( 1 ).percent();
    cooldown             = player -> cooldown.shock;
    cooldown -> duration = data().cooldown() + player -> spec.spiritual_insight -> effectN( 3 ).time_value();
    shock                = true;

    stats -> add_child ( player -> get_stats( "fulmination" ) );
  }

  double action_multiplier() const
  {
    double m = shaman_spell_t::action_multiplier();
    
    m *= 1.0 + p() -> buff.shocking_lava -> stack() * p() -> buff.shocking_lava -> data().effectN( 1 ).percent();

    return m;
  }

  double composite_target_crit( player_t* target ) const
  {
    double c = shaman_spell_t::composite_target_crit( target );

    if ( td( target ) -> debuff.stormstrike -> check() )
    {
      c += td( target ) -> debuff.stormstrike -> data().effectN( 1 ).percent();
      c += player -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).percent();
    }

    return c;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.shocking_lava -> expire();

    if ( consume_threshold == 0 )
      return;

    if ( result_is_hit( execute_state -> result ) )
    {
      int consuming_stacks = p() -> buff.lightning_shield -> stack() - consume_threshold;
      if ( consuming_stacks > 0 )
      {
        p() -> active_lightning_charge -> execute();
        new ( *p() -> sim ) lightning_charge_delay_t( *p(), p() -> buff.lightning_shield, consuming_stacks, consume_threshold );
        p() -> proc.fulmination[ consuming_stacks ] -> occur();

        shaman_td_t* tdata = td( execute_state -> target );
        tdata -> debuff.t16_2pc_caster -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
            consuming_stacks * tdata -> debuff.t16_2pc_caster -> data().duration() );
      }
    }
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "flame_shock", player, player -> find_class_spell( "Flame Shock" ), options_str )
  {
    // TODO-WOD: Separate to tick and direct amount to be safe
    base_multiplier      *= 1.0 + player -> perk.improved_shocks -> effectN( 1 ).percent();
    tick_may_crit         = true;
    dot_behavior          = DOT_REFRESH;
    cooldown              = player -> cooldown.shock;
    cooldown -> duration  = data().cooldown() + player -> spec.spiritual_insight -> effectN( 3 ).time_value();
    shock                 = true;
  }

  // Override assess_damage, so we can prevent 0 damage hits from reports, when
  // the flame_shock_t object is used with lava lash to spread flame shocks
  void assess_damage( dmg_e type, action_state_t* s )
  { if ( s -> result_amount > 0 ) shaman_spell_t::assess_damage( type, s ); }

  double action_multiplier() const
  {
    double m = shaman_spell_t::action_multiplier();

    m *= 1.0 + p() -> buff.unleash_flame -> stack() * p() -> buff.unleash_flame -> data().effectN( 2 ).percent();
    m *= 1.0 + p() -> buff.shocking_lava -> stack() * p() -> buff.shocking_lava -> data().effectN( 1 ).percent();

    return m;
  }

  void execute()
  {
    if ( ! td( target ) -> dot.flame_shock -> ticking )
      p() -> active_flame_shocks++;

    if ( p() -> buff.unleash_flame -> check() )
      p() -> proc.uf_flame_shock -> occur();

    shaman_spell_t::execute();

    p() -> buff.shocking_lava -> expire();
  }

  virtual void tick( dot_t* d )
  {
    shaman_spell_t::tick( d );

    if ( rng().roll ( p() -> spec.lava_surge -> proc_chance() ) )
    {
      if ( p() -> buff.lava_surge -> check() )
        p() -> proc.wasted_lava_surge -> occur();

      p() -> proc.lava_surge -> occur();
      if ( ! p() -> executing || p() -> executing -> id != 51505 )
        p() -> cooldown.lava_burst -> reset( true );
      else
      {
        p() -> proc.surge_during_lvb -> occur();
        p() -> lava_surge_during_lvb = true;
      }

      p() -> buff.lava_surge -> trigger();
    }

    if ( rng().roll( p() -> perk.improved_flame_shock -> proc_chance() ) )
      p() -> cooldown.lava_lash -> reset( true );

    trigger_tier16_4pc_melee( d -> state );
  }

  void last_tick( dot_t* d )
  {
    shaman_spell_t::last_tick( d );

    p() -> active_flame_shocks--;
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "frost_shock", player, player -> find_class_spell( "Frost Shock" ), options_str )
  {
    base_multiplier      *= 1.0 + player -> perk.improved_shocks -> effectN( 1 ).percent();
    uses_eoe = player -> specialization() == SHAMAN_ELEMENTAL || player -> specialization() == SHAMAN_ENHANCEMENT;
    if ( ! player -> perk.improved_frost_shock -> ok() )
      cooldown = player -> cooldown.shock;

    shock    = true;
  }

  void execute()
  {
    timespan_t tmp_cd = cooldown -> duration;
    cooldown -> duration = data().cooldown() + p() -> glyph.frost_shock -> effectN( 1 ).time_value();

    base_t::execute();

    cooldown -> duration = tmp_cd;
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "wind_shear", player, player -> find_class_spell( "Wind Shear" ), options_str )
  {
    may_miss = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// Ascendancy Spell =========================================================

struct ascendance_t : public shaman_spell_t
{
  cooldown_t* strike_cd;

  ascendance_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "ascendance", player, player -> find_class_spell( "Ascendance" ), options_str )
  {
    harmful = false;

    strike_cd = p() -> cooldown.strike;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    strike_cd -> reset( false );

    p() -> buff.ascendance -> trigger();
  }
};

// Healing Surge Spell ======================================================

struct healing_surge_t : public shaman_heal_t
{
  healing_surge_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_class_spell( "Healing Surge" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_crit() const
  {
    double c = shaman_heal_t::composite_crit();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c += p() -> spec.tidal_waves -> effectN( 1 ).percent() +
           p() -> sets.set( SET_T14_4PC_HEAL ) -> effectN( 1 ).percent();
    }

    return c;
  }
};

// Healing Wave Spell =======================================================

struct healing_wave_t : public shaman_heal_t
{
  healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - ( p() -> spec.tidal_waves -> effectN( 1 ).percent() +
                   p() -> sets.set( SET_T14_4PC_HEAL ) -> effectN( 1 ).percent() );
    }

    return c;
  }
};

// Greater Healing Wave Spell ===============================================

struct greater_healing_wave_t : public shaman_heal_t
{
  greater_healing_wave_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Greater Healing Wave" ), options_str )
  {
    resurgence_gain = p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  timespan_t execute_time() const
  {
    timespan_t c = shaman_heal_t::execute_time();

    if ( p() -> buff.tidal_waves -> up() )
    {
      c *= 1.0 - ( p() -> spec.tidal_waves -> effectN( 1 ).percent() +
                   p() -> sets.set( SET_T14_4PC_HEAL ) -> effectN( 1 ).percent() );
    }

    return c;
  }
};

// Riptide Spell ============================================================

struct riptide_t : public shaman_heal_t
{
  riptide_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_specialization_spell( "Riptide" ), options_str )
  {
    resurgence_gain = 0.6 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }
};

// Chain Heal Spell =========================================================

struct chain_heal_t : public shaman_heal_t
{
  chain_heal_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_class_spell( "Chain Heal" ), options_str )
  {
    resurgence_gain = 0.333 * p() -> spell.resurgence -> effectN( 1 ).average( player ) * p() -> spec.resurgence -> effectN( 1 ).percent();
  }

  double composite_target_da_multiplier( player_t* t) const
  {
    double m = shaman_heal_t::composite_target_da_multiplier( t );

    if ( td( t ) -> heal.riptide -> ticking )
      m *= 1.0 + p() -> spec.riptide -> effectN( 3 ).percent();

    return m;
  }
};

// Healing Rain Spell =======================================================

struct healing_rain_t : public shaman_heal_t
{
  struct healing_rain_aoe_tick_t : public shaman_heal_t
  {
    healing_rain_aoe_tick_t( shaman_t* player ) :
      shaman_heal_t( "healing_rain_tick", player, player -> find_spell( 73921 ) )
    {
      background = true;
      aoe = -1;
      // Can proc Echo of the Elements?
    }
  };

  healing_rain_t( shaman_t* player, const std::string& options_str ) :
    shaman_heal_t( player, player -> find_class_spell( "Healing Rain" ), options_str )
  {
    base_tick_time = data().effectN( 2 ).period();
    num_ticks = ( int ) ( data().duration() / data().effectN( 2 ).period() );
    hasted_ticks = false;
    tick_action = new healing_rain_aoe_tick_t( player );
  }
};

// ==========================================================================
// Shaman Totem System
// ==========================================================================

struct totem_active_expr_t : public expr_t
{
  pet_t& pet;
  totem_active_expr_t( pet_t& p ) :
    expr_t( "totem_active" ), pet( p ) { }
  virtual double evaluate() { return ! pet.is_sleeping(); }
};

struct totem_remains_expr_t : public expr_t
{
  pet_t& pet;
  totem_remains_expr_t( pet_t& p ) :
    expr_t( "totem_remains" ), pet( p ) { }
  virtual double evaluate()
  {
    if ( pet.is_sleeping() || ! pet.expiration )
      return 0.0;

    return pet.expiration -> remains().total_seconds();
  }
};

struct shaman_totem_pet_t : public pet_t
{
  totem_e               totem_type;

  // Pulse related functionality
  totem_pulse_action_t* pulse_action;
  core_event_t*         pulse_event;
  timespan_t            pulse_amplitude;

  // Summon related functionality
  pet_t*                summon_pet;

  // Spew Lava
  buff_t*               spew_lava;
  action_t*             spew_lava_action;

  shaman_totem_pet_t( shaman_t* p, const std::string& n, totem_e tt ) :
    pet_t( p -> sim, p, n, true ),
    totem_type( tt ),
    pulse_action( 0 ), pulse_event( 0 ), pulse_amplitude( timespan_t::zero() ),
    summon_pet( 0 ), spew_lava( 0 ), spew_lava_action( 0 )
  { }

  virtual void summon( timespan_t = timespan_t::zero() );
  virtual void dismiss();

  shaman_t* o()
  { return debug_cast< shaman_t* >( owner ); }

  virtual double composite_player_multiplier( school_e school ) const
  { return owner -> cache.player_multiplier( school ); }

  virtual double composite_spell_hit() const
  { return owner -> cache.spell_hit(); }

  virtual double composite_spell_crit() const
  { return owner -> cache.spell_crit(); }

  virtual double composite_spell_power( school_e school ) const
  { return owner -> cache.spell_power( school ); }

  virtual double composite_spell_power_multiplier() const
  { return owner -> composite_spell_power_multiplier(); }

  virtual double composite_spell_speed() const
  { return 1.0; }

  virtual expr_t* create_expression( action_t* a, const std::string& name )
  {
    if ( util::str_compare_ci( name, "active" ) )
      return new totem_active_expr_t( *this );
    else if ( util::str_compare_ci( name, "remains" ) )
      return new totem_remains_expr_t( *this );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, duration );

    return pet_t::create_expression( a, name );
  }

  virtual void init_spells();

  virtual void create_buffs()
  {
    pet_t::create_buffs();

    if ( totem_type != TOTEM_FIRE )
      return;

    spew_lava = buff_creator_t( this, "spew_lava", o() -> find_talent_spell( "Spew Lava" ) );
  }
};

struct shaman_totem_t : public shaman_spell_t
{
  shaman_totem_pet_t* totem_pet;
  timespan_t totem_duration;

  shaman_totem_t( const std::string& totem_name, shaman_t* player, const std::string& options_str, const spell_data_t* spell_data ) :
    shaman_spell_t( totem_name, player, spell_data, options_str ),
    totem_duration( data().duration() )
  {
    totem = true;
    harmful = callbacks = may_miss = may_crit = false;
    totem_pet      = dynamic_cast< shaman_totem_pet_t* >( player -> find_pet( name() ) );
    assert( totem_pet != 0 );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    totem_pet -> summon( totem_duration );
  }

  virtual expr_t* create_expression( const std::string& name )
  {
    if ( util::str_compare_ci( name, "active" ) )
      return new totem_active_expr_t( *totem_pet );
    else if ( util::str_compare_ci( name, "remains" ) )
      return new totem_remains_expr_t( *totem_pet );
    else if ( util::str_compare_ci( name, "duration" ) )
      return make_ref_expr( name, totem_duration );

    return shaman_spell_t::create_expression( name );
  }
};

struct totem_pulse_action_t : public spell_t
{
  shaman_totem_pet_t* totem;

  totem_pulse_action_t( const std::string& token, shaman_totem_pet_t* p, const spell_data_t* s ) :
    spell_t( token, p, s ), totem( p )
  {
    may_crit = harmful = background = true;
    callbacks = false;
    crit_bonus_multiplier *= 1.0 + totem -> o() -> spec.elemental_fury -> effectN( 1 ).percent() + totem -> o() -> perk.improved_elemental_fury -> effectN( 1 ).percent();
  }

  void init()
  {
    spell_t::init();

    // Hacky, but constructor wont work.
    crit_multiplier *= util::crit_multiplier( totem -> o() -> meta_gem );
  }

  double composite_da_multiplier() const
  {
    double m = spell_t::composite_da_multiplier();

    if ( totem -> o() -> buff.elemental_focus -> up() )
      m *= 1.0 + totem -> o() -> buff.elemental_focus -> data().effectN( 2 ).percent();

    return m;
  }
};

struct spew_lava_action_t : public totem_pulse_action_t
{
  spew_lava_action_t( shaman_totem_pet_t* p ) :
    totem_pulse_action_t( "spew_lava", p, p -> find_spell( 157501 ) )
  {
    may_miss = may_crit = false;
    tick_may_crit = true;
    aoe = -1;
    travel_speed = 0;
    spell_power_mod.tick = data().effectN( 1 ).coeff();
    spell_power_mod.direct = 0;
    base_tick_time = p -> find_spell( 152255 ) -> effectN( 1 ).period();
    num_ticks = p -> find_spell( 152255 ) -> duration().total_seconds() / base_tick_time.total_seconds();
    base_dd_min = base_dd_max = 0;
    hasted_ticks = false;
  }
};

struct totem_pulse_event_t : public event_t
{
  shaman_totem_pet_t* totem;

  totem_pulse_event_t( shaman_totem_pet_t& t, timespan_t amplitude ) :
    event_t( t, "totem_pulse" ),
    totem( &t )
  {
    add_event( amplitude );
  }

  virtual void execute()
  {
    if ( totem -> pulse_action )
      totem -> pulse_action -> execute();

    totem -> pulse_event = new ( sim() ) totem_pulse_event_t( *totem, totem -> pulse_amplitude );
  }
};

void shaman_totem_pet_t::summon( timespan_t duration )
{
  if ( o() -> totems[ totem_type ] )
    o() -> totems[ totem_type ] -> dismiss();

  pet_t::summon( duration );

  o() -> totems[ totem_type ] = this;

  if ( pulse_action )
    pulse_event = new ( *sim ) totem_pulse_event_t( *this, pulse_amplitude );

  if ( summon_pet )
    summon_pet -> summon();
}

void shaman_totem_pet_t::dismiss()
{
  pet_t::dismiss();
  core_event_t::cancel( pulse_event );

  if ( summon_pet )
    summon_pet -> dismiss();

  if ( o() -> totems[ totem_type ] == this )
    o() -> totems[ totem_type ] = 0;
}

void shaman_totem_pet_t::init_spells()
{
  pet_t::init_spells();

  if ( totem_type != TOTEM_FIRE )
    return;

  spew_lava_action = new spew_lava_action_t( this );
}

// Spew Lava Spell =========================================================

struct spew_lava_t: public shaman_spell_t
{
  spew_lava_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "spew_lava", player, player -> find_talent_spell( "Spew Lava" ), options_str )
  {
    harmful = false;
    num_ticks = 0;
    base_tick_time = timespan_t::zero();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> totems[ TOTEM_FIRE ] -> spew_lava -> trigger();
    p() -> totems[ TOTEM_FIRE ] -> spew_lava_action -> schedule_execute();
  }

  bool ready()
  {
    if ( ! p() -> totems[ TOTEM_FIRE ] )
      return false;

    return shaman_spell_t::ready();
  }
};

// Earth Elemental Totem Spell ==============================================

struct earth_elemental_totem_t : public shaman_totem_pet_t
{
  earth_elemental_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "earth_elemental_totem", TOTEM_EARTH )
  { }

  void init_spells()
  {
    if ( o() -> talent.primal_elementalist -> ok() )
      summon_pet = o() -> find_pet( "primal_earth_elemental" );
    else
      summon_pet = o() -> find_pet( "greater_earth_elemental" );
    assert( summon_pet != 0 );

    shaman_totem_pet_t::init_spells();
  }
};

struct earth_elemental_totem_spell_t : public shaman_totem_t
{
  earth_elemental_totem_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "earth_elemental_totem", player, options_str, player -> find_class_spell( "Earth Elemental Totem" ) )
  { }

  void execute()
  {
    shaman_totem_t::execute();

    if ( p() -> cooldown.fire_elemental_totem -> up() )
      p() -> cooldown.fire_elemental_totem -> start( data().duration() );
  }
};

// Fire Elemental Totem Spell ===============================================

struct fire_elemental_totem_t : public shaman_totem_pet_t
{
  fire_elemental_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "fire_elemental_totem", TOTEM_FIRE )
  { }

  void init_spells()
  {
    if ( o() -> talent.primal_elementalist -> ok() )
      summon_pet = o() -> find_pet( "primal_fire_elemental" );
    else
      summon_pet = o() -> find_pet( "greater_fire_elemental" );
    assert( summon_pet != 0 );

    shaman_totem_pet_t::init_spells();
  }
};

struct fire_elemental_totem_spell_t : public shaman_totem_t
{
  fire_elemental_totem_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "fire_elemental_totem", player, options_str, player -> find_class_spell( "Fire Elemental Totem" ) )
  {
    cooldown -> duration *= 1.0 + p() -> glyph.fire_elemental_totem -> effectN( 1 ).percent();
    totem_duration       *= 1.0 + p() -> glyph.fire_elemental_totem -> effectN( 2 ).percent();
  }

  void execute()
  {
    shaman_totem_t::execute();

    if ( p() -> cooldown.earth_elemental_totem -> up() )
      p() -> cooldown.earth_elemental_totem -> start( data().duration() );
  }
};

// Magma Totem Spell ========================================================

struct magma_totem_pulse_t : public totem_pulse_action_t
{
  magma_totem_pulse_t( shaman_totem_pet_t* p ) :
    totem_pulse_action_t( "magma_totem", p, p -> find_spell( 8187 ) )
  {
    aoe = -1;
  }
};

struct magma_totem_t : public shaman_totem_pet_t
{
  magma_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "magma_totem", TOTEM_FIRE )
  {
    pulse_amplitude = p -> dbc.spell( 8188 ) -> effectN( 1 ).period();
  }

  void init_spells()
  {
    if ( find_spell( 8187 ) -> ok() )
      pulse_action = new magma_totem_pulse_t( this );

    shaman_totem_pet_t::init_spells();
  }
};

// Searing Totem Spell ======================================================

struct searing_totem_pulse_t : public totem_pulse_action_t
{
  searing_totem_pulse_t( shaman_totem_pet_t* p ) :
    totem_pulse_action_t( "searing_bolt", p, p -> find_spell( 3606 ) )
  {
    base_execute_time = timespan_t::zero();
    // 15851 dbc base min/max damage is approx. 10 points too low, compared
    // to in-game damage ranges
    base_dd_min += 10;
    base_dd_max += 10;

    base_multiplier *= 1.0 + totem -> o() -> perk.improved_searing_totem -> effectN( 1 ).percent();
  }
};

struct searing_totem_t : public shaman_totem_pet_t
{
  searing_totem_t( shaman_t* p ) :
    shaman_totem_pet_t( p, "searing_totem", TOTEM_FIRE )
  {
    pulse_amplitude = p -> find_spell( 3606 ) -> cast_time( p -> level );
  }

  void init_spells()
  {
    if ( find_spell( 3606 ) -> ok() )
      pulse_action = new searing_totem_pulse_t( this );

    shaman_totem_pet_t::init_spells();
  }
};

// ==========================================================================
// Shaman Weapon Imbues
// ==========================================================================

// Flametongue Weapon Spell =================================================

struct flametongue_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  flametongue_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "flametongue_weapon", player, player -> find_specialization_spell( "Flametongue Weapon" ) ),
    bonus_power( 0 ), main( 0 ), off( 0 )
  {
    std::string weapon_str;

    option_t options[] =
    {
      opt_string( "weapon", weapon_str ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: flametongue_weapon: weapon option must be one of main/off/both\n", p() -> name() );
      sim -> cancel();
    }

    // Spell damage scaling is defined in "Flametongue Weapon (Passive), id 10400"
    bonus_power  = player -> dbc.spell( 10400 ) -> effectN( 2 ).percent();
    harmful      = false;
    may_miss     = false;

    if ( main )
      player -> flametongue_mh = new flametongue_weapon_spell_t( "flametongue_attack_mh", player, &( player -> main_hand_weapon ) );

    if ( off )
      player -> flametongue_oh = new flametongue_weapon_spell_t( "flametongue_attack_oh", player, &( player -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      p() -> main_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      p() -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      p() -> off_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      p() -> off_hand_weapon.buff_value = bonus_power;
    }
    p() -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  };

  virtual bool ready()
  {
    if ( main && ( p() -> main_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    if ( off && ( p() -> off_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    return false;
  }
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  windfury_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "windfury_weapon", player, player -> find_specialization_spell( "Windfury Weapon" ) ),
    bonus_power( 0 ), main( 0 ), off( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    std::string weapon_str;

    option_t options[] =
    {
      opt_string( "weapon", weapon_str ),
      opt_null()
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: windfury_weapon: weapon option must be one of main/off/both\n", p() -> name() );
      sim -> cancel();
    }

    bonus_power  = 0;
    harmful      = false;
    may_miss     = false;

    if ( main )
      player -> windfury_mh = new windfury_weapon_melee_attack_t( "windfury_attack_mh", player, &( player -> main_hand_weapon ) );

    if ( off )
      player -> windfury_oh = new windfury_weapon_melee_attack_t( "windfury_attack_oh", player, &( player -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      p() -> main_hand_weapon.buff_type  = WINDFURY_IMBUE;
      p() -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      p() -> off_hand_weapon.buff_type  = WINDFURY_IMBUE;
      p() -> off_hand_weapon.buff_value = bonus_power;
    }
    p() -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  };

  virtual bool ready()
  {
    if ( main && ( p() -> main_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    if ( off && ( p() -> off_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    return false;
  }
};

struct earthliving_weapon_t : public shaman_spell_t
{
  double bonus_power;

  earthliving_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "earthliving_weapon", player, player -> find_specialization_spell( "Earthliving Weapon" ), options_str ),
    bonus_power( 0 )
  {
    check_spec( SHAMAN_RESTORATION );

    bonus_power  = player -> find_spell( 52007 ) -> effectN( 2 ).average( player );
    harmful      = false;
    may_miss     = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> main_hand_weapon.buff_type  = EARTHLIVING_IMBUE;
    p() -> main_hand_weapon.buff_value = bonus_power;
    p() -> invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  };

  virtual bool ready()
  {
    if ( p() -> main_hand_weapon.buff_type != EARTHLIVING_IMBUE )
      return true;

    return false;
  }
};

// ==========================================================================
// Shaman Shields
// ==========================================================================

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_shield", player, player -> find_class_spell( "Lightning Shield" ), options_str )
  {
    harmful = false;

    player -> active_lightning_charge = new lightning_charge_t( player -> specialization() == SHAMAN_ELEMENTAL ? "fulmination" : "lightning_shield", player );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.water_shield     -> expire();
    p() -> buff.lightning_shield -> trigger( data().initial_stacks() );
  }
};

// Water Shield Spell =======================================================

struct water_shield_t : public shaman_spell_t
{
  double bonus;

  water_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "water_shield", player, player -> find_class_spell( "Water Shield" ), options_str ),
    bonus( 0.0 )
  {
    harmful      = false;
    bonus        = data().effectN( 2 ).average( player );
    bonus       *= 1.0 + player -> glyph.water_shield -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.lightning_shield -> expire();
    p() -> buff.water_shield -> trigger( data().initial_stacks(), bonus );
  }
};

// ==========================================================================
// Shaman Custom Buff implementation
// ==========================================================================

inline bool maelstrom_weapon_buff_t::trigger( shaman_attack_t* action, int stacks, double chance )
{
  int cur_stack = current_stack;

  if ( ! buff_t::trigger( stacks, buff_t::DEFAULT_VALUE(), chance, timespan_t::min() ) )
    return false;

  if ( delay )
    for ( int i = 0; i < stacks; i++ )
      trigger_actions.push_back( action );
  else
  {
    for ( int i = 0; i < stacks; i++ )
    {
      if ( action -> maelstrom_procs )
        action -> maelstrom_procs -> add( 1 );

      if ( ( cur_stack + i ) >= max_stack() && action -> maelstrom_procs_wasted )
        action -> maelstrom_procs_wasted -> add( 1 );
    }
  }

  return true;
}

inline void maelstrom_weapon_buff_t::execute( int stacks, double value, timespan_t duration )
{
  int cur_stack = current_stack;

  buff_t::execute( stacks, value, duration );

  for ( size_t i = 0, end = trigger_actions.size(); i < end; i++ )
  {
    shaman_attack_t* a = trigger_actions[ i ];
    if ( a -> maelstrom_procs )
      a -> maelstrom_procs -> add( 1 );

    if ( static_cast<int>( cur_stack + i ) >= max_stack() )
      a -> maelstrom_procs_wasted -> add( 1 );
  }

  trigger_actions.clear();
}

inline void maelstrom_weapon_buff_t::reset() 
{
  buff_t::reset();
  trigger_actions.clear();
}

struct unleash_flame_expiration_delay_t : public event_t
{
  unleash_flame_buff_t* buff;

  unleash_flame_expiration_delay_t( shaman_t& player, unleash_flame_buff_t* b ) :
    event_t( player, "unleash_flame_expiration_delay" ), buff( b )
  {
    add_event( sim().rng().gauss( player.uf_expiration_delay, player.uf_expiration_delay_stddev ) );
  }

  virtual void execute()
  {
    // Call real expire after a delay
    buff -> buff_t::expire();
    buff -> expiration_delay = 0;
  }
};

inline void unleash_flame_buff_t::expire_override()
{
  if ( current_stack == 1 && ! expiration && ! expiration_delay && ! player -> is_sleeping() )
  {
    shaman_t* p = debug_cast< shaman_t* >( player );
    p -> proc.uf_wasted -> occur();
  }

  // Active player's Unleash Flame buff has a short aura expiration delay, which allows
  // "Double Casting" with a single buff
  if ( ! player -> is_sleeping() )
  {
    if ( current_stack <= 0 ) return;
    if ( expiration_delay ) return;
    // If there's an expiration event going on, we are prematurely canceling the buff, so delay expiration
    if ( expiration )
      expiration_delay = new ( *sim ) unleash_flame_expiration_delay_t( *debug_cast< shaman_t* >( player ), this );
    else
      buff_t::expire_override();
  }
  // If the p() is sleeping, make sure the existing buff behavior works (i.e., a call to
  // expire) and additionally, make _absolutely sure_ that any pending expiration delay
  // is canceled
  else
  {
    buff_t::expire_override();
    core_event_t::cancel( expiration_delay );
  }
}

inline void unleash_flame_buff_t::reset()
{
  buff_t::reset();
  core_event_t::cancel( expiration_delay );
}

void ascendance_buff_t::ascendance( attack_t* mh, attack_t* oh, timespan_t lvb_cooldown )
{
  // Presume that ascendance trigger and expiration will not reset the swing
  // timer, so we need to cancel and reschedule autoattack with the
  // remaining swing time of main/off hands
  if ( player -> specialization() == SHAMAN_ENHANCEMENT )
  {
    bool executing = false;
    timespan_t time_to_hit = timespan_t::zero();
    if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
    {
      executing = true;
      time_to_hit = player -> main_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
      if ( time_to_hit < timespan_t::zero() )
      {
        sim -> out_error.printf( "Ascendance %s time_to_hit=%f", player -> main_hand_attack -> name(), time_to_hit.total_seconds() );
        assert( 0 );
      }
#endif
      core_event_t::cancel( player -> main_hand_attack -> execute_event );
    }

    player -> main_hand_attack = mh;
    if ( executing )
    {
      // Kick off the new main hand attack, by instantly scheduling
      // and rescheduling it to the remaining time to hit. We cannot use
      // normal reschedule mechanism here (i.e., simply use
      // event_t::reschedule() and leave it be), because the rescheduled
      // event would be triggered before the full swing time (of the new
      // auto attack) in most cases.
      player -> main_hand_attack -> base_execute_time = timespan_t::zero();
      player -> main_hand_attack -> schedule_execute();
      player -> main_hand_attack -> base_execute_time = player -> main_hand_attack -> weapon -> swing_time;
      player -> main_hand_attack -> execute_event -> reschedule( time_to_hit );
    }

    if ( player -> off_hand_attack )
    {
      time_to_hit = timespan_t::zero();
      executing = false;

      if ( player -> off_hand_attack -> execute_event )
      {
        executing = true;
        time_to_hit = player -> off_hand_attack -> execute_event -> remains();
#ifndef NDEBUG
        if ( time_to_hit < timespan_t::zero() )
        {
          sim -> out_error.printf( "Ascendance %s time_to_hit=%f", player -> off_hand_attack -> name(), time_to_hit.total_seconds() );
          assert( 0 );
        }
#endif
        core_event_t::cancel( player -> off_hand_attack -> execute_event );
      }

      player -> off_hand_attack = oh;
      if ( executing )
      {
        // Kick off the new off hand attack, by instantly scheduling
        // and rescheduling it to the remaining time to hit. We cannot use
        // normal reschedule mechanism here (i.e., simply use
        // event_t::reschedule() and leave it be), because the rescheduled
        // event would be triggered before the full swing time (of the new
        // auto attack) in most cases.
        player -> off_hand_attack -> base_execute_time = timespan_t::zero();
        player -> off_hand_attack -> schedule_execute();
        player -> off_hand_attack -> base_execute_time = player -> off_hand_attack -> weapon -> swing_time;
        player -> off_hand_attack -> execute_event -> reschedule( time_to_hit );
      }
    }
  }
  // Elemental simply changes the Lava Burst cooldown, Lava Beam replacement
  // will be handled by action list and ready() in Chain Lightning / Lava
  // Beam
  else if ( player -> specialization() == SHAMAN_ELEMENTAL )
  {
    if ( lava_burst )
    {
      lava_burst -> cooldown -> duration = lvb_cooldown;
      lava_burst -> cooldown -> reset( false );
    }
  }
}

inline bool ascendance_buff_t::trigger( int stacks, double value, double chance, timespan_t duration )
{
  shaman_t* p = debug_cast< shaman_t* >( player );

  if ( player -> specialization() == SHAMAN_ELEMENTAL && ! lava_burst )
  {
    lava_burst = player -> find_action( "lava_burst" );
  }

  ascendance( p -> ascendance_mh, p -> ascendance_oh, timespan_t::zero() );
  return buff_t::trigger( stacks, value, chance, duration );
}

inline void ascendance_buff_t::expire_override()
{
  shaman_t* p = debug_cast< shaman_t* >( player );

  ascendance( p -> melee_mh, p -> melee_oh, lava_burst ? lava_burst -> data().cooldown() : timespan_t::zero() );
  buff_t::expire_override();
}

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();

  option_t shaman_options[] =
  {
    opt_timespan( "wf_delay",                   wf_delay                   ),
    opt_timespan( "wf_delay_stddev",            wf_delay_stddev            ),
    opt_timespan( "uf_expiration_delay",        uf_expiration_delay        ),
    opt_timespan( "uf_expiration_delay_stddev", uf_expiration_delay_stddev ),
    opt_null()
  };

  option_t::copy( options, shaman_options );
}

// ==========================================================================
// Shaman Specific Proc actions
// ==========================================================================

struct shaman_lightning_strike_t : public shaman_attack_t
{
  shaman_lightning_strike_t( shaman_t* p ) :
    shaman_attack_t( "lightning_strike", p, p -> find_spell( 137597 ) )
  {
    may_proc_windfury = false;
    may_proc_maelstrom = false;
    may_proc_primal_wisdom = false;
    may_proc_flametongue = false;
    background = true;
    may_dodge = may_parry = false;
  }
};
 
struct shaman_flurry_of_xuen_t : public shaman_attack_t
{
  shaman_flurry_of_xuen_t( shaman_t* p ) :
    shaman_attack_t( "flurry_of_xuen", p, p -> find_spell( 147891 ) )
  {
    may_proc_windfury = false;
    may_proc_maelstrom = false;
    may_proc_primal_wisdom = false;
    may_proc_flametongue = false;
    background = true;
    aoe = 5;
  }
};

// shaman_t::create_proc_action =============================================

action_t* shaman_t::create_proc_action( const std::string& name )
{
  if ( name == "flurry_of_xuen" ) return new shaman_flurry_of_xuen_t( this );
  if ( name == "ligtning_strike" ) return new shaman_lightning_strike_t( this );

  return 0;
}

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "ancestral_swiftness"     ) return new      ancestral_swiftness_t( this, options_str );
  if ( name == "ascendance"              ) return new               ascendance_t( this, options_str );
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if ( name == "call_of_the_elements"    ) return new     call_of_the_elements_t( this, options_str );
  if ( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "earthliving_weapon"      ) return new       earthliving_weapon_t( this, options_str );
  if ( name == "earthquake"              ) return new               earthquake_t( this, options_str );
  if ( name == "elemental_blast"         ) return new          elemental_blast_t( this, options_str );
  if ( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options_str );
  if ( name == "fire_nova"               ) return new                fire_nova_t( this, options_str );
  if ( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if ( name == "flametongue_weapon"      ) return new       flametongue_weapon_t( this, options_str );
  if ( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if ( name == "lava_beam"               ) return new                lava_beam_t( this, options_str );
  if ( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if ( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if ( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if ( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if ( name == "primal_strike"           ) return new            primal_strike_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "spew_lava"               ) return new               spew_lava_t( this, options_str );
  if ( name == "windstrike"              ) return new               windstrike_t( this, options_str );
  if ( name == "feral_spirit"            ) return new       feral_spirit_spell_t( this, options_str );
  if ( name == "spirit_walk"             ) return new              spirit_walk_t( this, options_str );
  if ( name == "spiritwalkers_grace"     ) return new      spiritwalkers_grace_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if ( name == "unleash_elements"        ) return new         unleash_elements_t( this, options_str );
  if ( name == "water_shield"            ) return new             water_shield_t( this, options_str );
  if ( name == "wind_shear"              ) return new               wind_shear_t( this, options_str );
  if ( name == "windfury_weapon"         ) return new          windfury_weapon_t( this, options_str );

  if ( name == "chain_heal"              ) return new               chain_heal_t( this, options_str );
  if ( name == "greater_healing_wave"    ) return new     greater_healing_wave_t( this, options_str );
  if ( name == "healing_rain"            ) return new             healing_rain_t( this, options_str );
  if ( name == "healing_surge"           ) return new            healing_surge_t( this, options_str );
  if ( name == "healing_wave"            ) return new             healing_wave_t( this, options_str );
  if ( name == "riptide"                 ) return new                  riptide_t( this, options_str );

  if ( name == "earth_elemental_totem"   ) return new earth_elemental_totem_spell_t( this, options_str );
  if ( name == "fire_elemental_totem"    ) return new  fire_elemental_totem_spell_t( this, options_str );
  if ( name == "magma_totem"             ) return new                shaman_totem_t( "magma_totem", this, options_str, find_specialization_spell( "Magma Totem" ) );
  if ( name == "searing_totem"           ) return new                shaman_totem_t( "searing_totem", this, options_str, find_class_spell( "Searing Totem" ) );

  return player_t::create_action( name, options_str );
}

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "fire_elemental_pet"       ) return new fire_elemental_t( sim, this, false );
  if ( pet_name == "fire_elemental_guardian"  ) return new fire_elemental_t( sim, this, true );
  if ( pet_name == "earth_elemental_pet"      ) return new earth_elemental_pet_t( sim, this, false );
  if ( pet_name == "earth_elemental_guardian" ) return new earth_elemental_pet_t( sim, this, true );

  if ( pet_name == "earth_elemental_totem"   ) return new earth_elemental_totem_t( this );
  if ( pet_name == "fire_elemental_totem"    ) return new fire_elemental_totem_t( this );
  if ( pet_name == "magma_totem"             ) return new magma_totem_t( this );
  if ( pet_name == "searing_totem"           ) return new searing_totem_t( this );

  return 0;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  pet_fire_elemental       = create_pet( "fire_elemental_pet"       );
  guardian_fire_elemental  = create_pet( "fire_elemental_guardian"  );
  pet_earth_elemental      = create_pet( "earth_elemental_pet"      );
  guardian_earth_elemental = create_pet( "earth_elemental_guardian" );

  if ( specialization() == SHAMAN_ELEMENTAL )
  {
    for ( size_t i = 0; i < sizeof_array( guardian_lightning_elemental ); i++ )
      guardian_lightning_elemental[ i ] = new lightning_elemental_t( this );
  }

  if ( specialization() == SHAMAN_ENHANCEMENT )
  {
    for ( size_t i = 0; i < sizeof_array( pet_feral_spirit ); i++ )
      pet_feral_spirit[ i ] = new feral_spirit_pet_t( this );
  }

  create_pet( "earth_elemental_totem" );
  create_pet( "fire_elemental_totem"  );
  create_pet( "magma_totem"           );
  create_pet( "searing_totem"         );
}

// shaman_t::create_expression ==============================================

expr_t* shaman_t::create_expression( action_t* a, const std::string& name )
{

  std::vector<std::string> splits = util::string_split( name, "." );

  // totem.<kind>.<op>
  if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "totem" ) )
  {
    totem_e totem_type = TOTEM_NONE;
    shaman_totem_pet_t* totem = 0;

    if ( util::str_compare_ci( splits[ 1 ], "earth" ) )
      totem_type = TOTEM_EARTH;
    else if ( util::str_compare_ci( splits[ 1 ], "fire" ) )
      totem_type = TOTEM_FIRE;
    else if ( util::str_compare_ci( splits[ 1 ], "water" ) )
      totem_type = TOTEM_WATER;
    else if ( util::str_compare_ci( splits[ 1 ], "air" ) )
      totem_type = TOTEM_AIR;
    // None of the elements, then a specific totem name?
    else
      totem = static_cast< shaman_totem_pet_t* >( find_pet( splits[ 1 ] ) );

    // Nothing found
    if ( totem_type == TOTEM_NONE && totem == 0 )
      return player_t::create_expression( a, name );
    // A specific totem name given, and found
    else if ( totem != 0 )
      return totem -> create_expression( a, splits[ 2 ] );

    // Otherwise generic totem school, make custom expressions
    if ( util::str_compare_ci( splits[ 2 ], "active" ) )
    {
      struct totem_active_expr_t : public expr_t
      {
        shaman_t& p;
        totem_e totem_type;

        totem_active_expr_t( shaman_t& p, totem_e tt ) :
          expr_t( "totem_active" ), p( p ), totem_type( tt )
        { }

        virtual double evaluate()
        {
          return p.totems[ totem_type ] && ! p.totems[ totem_type ] -> is_sleeping();
        }
      };
      return new totem_active_expr_t( *this, totem_type );
    }
  }

  if ( util::str_compare_ci( name, "active_flame_shock" ) )
  {
    return make_ref_expr( name, active_flame_shocks );
  }

  return player_t::create_expression( a, name );
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  // New set bonus system
  static const set_bonus_description_t set_bonuses =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    { 105780, 105816, 105866, 105872,     0,     0, 105764, 105876 }, // Tier13
    { 123123, 123124, 123132, 123133,     0,     0, 123134, 123135 }, // Tier14
    { 138145, 138144, 138136, 138141,     0,     0, 138303, 138305 }, // Tier15
    { 144998, 145003, 144962, 144966,     0,     0, 145378, 145380 }, // Tier16
  };
  sets.register_spelldata( set_bonuses );

  // Generic
  spec.mail_specialization   = find_specialization_spell( "Mail Specialization" );
  constant.matching_gear_multiplier = spec.mail_specialization -> effectN( 1 ).percent();

  // Elemental / Restoration
  spec.spiritual_insight     = find_specialization_spell( "Spiritual Insight" );

  // Elemental
  spec.elemental_focus       = find_specialization_spell( "Elemental Focus" );
  spec.elemental_fury        = find_specialization_spell( "Elemental Fury" );
  spec.fulmination           = find_specialization_spell( "Fulmination" );
  spec.lava_surge            = find_specialization_spell( "Lava Surge" );
  spec.readiness_elemental   = find_specialization_spell( "Readiness: Elemental" );
  spec.rolling_thunder       = find_specialization_spell( "Rolling Thunder" );
  spec.shamanism             = find_specialization_spell( "Shamanism" );

  // Enhancement
  spec.critical_strikes      = find_specialization_spell( "Critical Strikes" );
  spec.dual_wield            = find_specialization_spell( "Dual Wield" );
  spec.flurry                = find_specialization_spell( "Flurry" );
  spec.maelstrom_weapon      = find_specialization_spell( "Maelstrom Weapon" );
  spec.mental_quickness      = find_specialization_spell( "Mental Quickness" );
  spec.primal_wisdom         = find_specialization_spell( "Primal Wisdom" );
  spec.readiness_enhancement = find_specialization_spell( "Readiness: Enhancement" );
  spec.shamanistic_rage      = find_specialization_spell( "Shamanistic Rage" );
  spec.spirit_walk           = find_specialization_spell( "Spirit Walk" );

  // Restoration
  spec.ancestral_awakening   = find_specialization_spell( "Ancestral Awakening" );
  spec.ancestral_focus       = find_specialization_spell( "Ancestral Focus" );
  spec.earth_shield          = find_specialization_spell( "Earth Shield" );
  spec.meditation            = find_specialization_spell( "Meditation" );
  spec.purification          = find_specialization_spell( "Purification" );
  spec.resurgence            = find_specialization_spell( "Resurgence" );
  spec.riptide               = find_specialization_spell( "Riptide" );
  spec.tidal_waves           = find_specialization_spell( "Tidal Waves" );

  // Masteries
  mastery.elemental_overload         = find_mastery_spell( SHAMAN_ELEMENTAL   );
  mastery.enhanced_elements          = find_mastery_spell( SHAMAN_ENHANCEMENT );
  mastery.deep_healing               = find_mastery_spell( SHAMAN_RESTORATION );

  // Talents
  talent.call_of_the_elements        = find_talent_spell( "Call of the Elements" );
  talent.totemic_restoration         = find_talent_spell( "Totemic Restoration"  );

  talent.elemental_mastery           = find_talent_spell( "Elemental Mastery"    );
  talent.ancestral_swiftness         = find_talent_spell( "Ancestral Swiftness"  );

  talent.echo_of_the_elements        = find_talent_spell( "Echo of the Elements" );
  rppm_echo_of_the_elements.set_frequency( talent.echo_of_the_elements -> real_ppm() );

  talent.unleashed_fury              = find_talent_spell( "Unleashed Fury"       );
  talent.primal_elementalist         = find_talent_spell( "Primal Elementalist"  );
  talent.elemental_blast             = find_talent_spell( "Elemental Blast"      );

  talent.shocking_lava               = find_talent_spell( "Shocking Lava" );
  talent.storm_elemental_totem       = find_talent_spell( "Storm Elemental Totem" );
  talent.spew_lava                   = find_talent_spell( "Spew Lava" );

  // Perks - Shared
  perk.improved_searing_totem        = find_perk_spell( "Improved Searing Totem" );
  perk.improved_frost_shock          = find_perk_spell( "Improved Frost Shock" );

  // Perks - Enhancement
  perk.improved_lava_lash            = find_perk_spell( "Improved Lava Lash" );
  perk.improved_flame_shock          = find_perk_spell( "Improved Flame Shock" );
  perk.improved_maelstrom_weapon     = find_perk_spell( "Improved Maelstrom Weapon" );
  perk.improved_stormstrike          = find_perk_spell( "Improved Stormstrike" );
  perk.improved_lava_lash_2          = find_perk_spell( 7 );
  perk.improved_feral_spirits        = find_perk_spell( "Improved Feral Spirits" );

  // Perks - Elemental
  perk.improved_chain_lightning      = find_perk_spell( "Improved Chain Lightning" );
  perk.improved_lightning_shield     = find_perk_spell( "Improved Lightning Shield" );
  perk.improved_elemental_fury       = find_perk_spell( "Improved Elemental Fury" );
  perk.improved_lightning_bolt       = find_perk_spell( "Improved Lightning Bolt" );
  perk.improved_lava_burst           = find_perk_spell( "Improved Lava Burst" );
  perk.improved_shocks               = find_perk_spell( "Improved Shocks" );

  // Glyphs
  glyph.chain_lightning              = find_glyph_spell( "Glyph of Chain Lightning" );
  glyph.fire_elemental_totem         = find_glyph_spell( "Glyph of Fire Elemental Totem" );
  glyph.flame_shock                  = find_glyph_spell( "Glyph of Flame Shock" );
  glyph.frost_shock                  = find_glyph_spell( "Glyph of Frost Shock" );
  glyph.healing_storm                = find_glyph_spell( "Glyph of Healing Storm" );
  glyph.lava_lash                    = find_glyph_spell( "Glyph of Lava Lash" );
  glyph.spiritwalkers_grace          = find_glyph_spell( "Glyph of Spiritwalker's Grace" );
  glyph.telluric_currents            = find_glyph_spell( "Glyph of Telluric Currents" );
  glyph.thunder                      = find_glyph_spell( "Glyph of Thunder" );
  glyph.thunderstorm                 = find_glyph_spell( "Glyph of Thunderstorm" );
  glyph.water_shield                 = find_glyph_spell( "Glyph of Water Shield" );
  glyph.lightning_shield             = find_glyph_spell( "Glyph of Lightning Shield" );

  // Misc spells
  spell.ancestral_swiftness          = find_spell( 121617 );
  spell.primal_wisdom                = find_spell( 63375 );
  spell.resurgence                   = find_spell( 101033 );
  spell.flame_shock                  = find_class_spell( "Flame Shock" );
  spell.windfury_driver              = find_spell( 33757 );

  // Constants
  constant.speed_attack_ancestral_swiftness = 1.0 / ( 1.0 + spell.ancestral_swiftness -> effectN( 2 ).percent() );
  constant.haste_ancestral_swiftness  = 1.0 / ( 1.0 + spell.ancestral_swiftness -> effectN( 1 ).percent() );

  // Tier16 2PC Enhancement bonus actions, these need to bypass imbue checks
  // presumably, so we cannot just re-use our actual imbued ones
  if ( sets.has_set_bonus( SET_T16_2PC_MELEE ) )
  {
    t16_wind = new unleash_wind_t( "t16_unleash_wind", this );
    t16_flame = new unleash_flame_t( "t16_unleash_flame", this );
  }

  player_t::init_spells();
}

// shaman_t::init_base ======================================================

void shaman_t::init_base_stats()
{
  player_t::init_base_stats();

  //base.stats.attack_power = ( level * 2 ) - 30; Gone in WoD, double check later.
  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  resources.initial_multiplier[ RESOURCE_MANA ] = 1.0 + spec.spiritual_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.spiritual_insight -> effectN( 1 ).percent();

  base.distance = ( specialization() == SHAMAN_ENHANCEMENT ) ? 3 : 30;
  base.mana_regen_from_spirit_multiplier = spec.meditation -> effectN( 1 ).percent();

  diminished_kfactor    = 0.009880;
  diminished_dodge_cap = 0.006870;
  diminished_parry_cap = 0.006870;

  //if ( specialization() == SHAMAN_ENHANCEMENT )
  //  ready_type = READY_TRIGGER;

  if ( specialization() == SHAMAN_ENHANCEMENT )
    resources.infinite_resource[ RESOURCE_MANA ] = 1;
}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  switch ( specialization() )
  {
    case SHAMAN_ENHANCEMENT:
      scales_with[ STAT_STRENGTH              ] = false;
      scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
      scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
      scales_with[ STAT_SPIRIT                ] = false;
      scales_with[ STAT_SPELL_POWER           ] = false;
      scales_with[ STAT_INTELLECT             ] = false;
      break;
    case SHAMAN_ELEMENTAL:
      scales_with[ STAT_SPIRIT                ] = false;
      break;
    case SHAMAN_RESTORATION:
      scales_with[ STAT_MASTERY_RATING ] = false;
      break;
    default:
      break;
  }
}

// shaman_t::init_buffs =====================================================

void shaman_t::create_buffs()
{
  player_t::create_buffs();

  buff.ancestral_swiftness     = buff_creator_t( this, "ancestral_swiftness", talent.ancestral_swiftness )
                                 .cd( timespan_t::zero() );
  buff.ascendance              = new ascendance_buff_t( this );
  buff.elemental_focus         = buff_creator_t( this, "elemental_focus",   spec.elemental_focus -> effectN( 1 ).trigger() )
                                 .activated( false )
                                 .cd( spec.elemental_focus -> internal_cooldown() );
  buff.echo_of_the_elements    = buff_creator_t( this, "echo_of_the_elements", talent.echo_of_the_elements )
                                 .chance( talent.echo_of_the_elements -> ok() );
  buff.spew_lava               = buff_creator_t( this, "spew_lava", talent.spew_lava )
                                 .chance( talent.spew_lava -> ok() );
  buff.lava_surge              = buff_creator_t( this, "lava_surge",        spec.lava_surge )
                                 .activated( false )
                                 .chance( 1.0 ); // Proc chance is handled externally
  buff.lightning_shield        = buff_creator_t( this, "lightning_shield", find_class_spell( "Lightning Shield" ) )
                                 .max_stack( ( specialization() == SHAMAN_ELEMENTAL )
                                             ? static_cast< int >( spec.rolling_thunder -> effectN( 1 ).base_value() + perk.improved_lightning_shield -> effectN( 1 ).base_value() )
                                             : find_class_spell( "Lightning Shield" ) -> initial_stacks() )
                                 .cd( timespan_t::zero() );
  //buff.maelstrom_weapon        = buff_creator_t( this, "maelstrom_weapon",  spec.maelstrom_weapon -> effectN( 1 ).trigger() )
  //                               .chance( spec.maelstrom_weapon -> proc_chance() )
  //                               .activated( false );
  buff.maelstrom_weapon        = new maelstrom_weapon_buff_t( this );
  buff.shamanistic_rage        = buff_creator_t( this, "shamanistic_rage",  spec.shamanistic_rage );
  buff.shocking_lava           = buff_creator_t( this, "shocking_lava", find_spell( 157174 ) )
                                 .chance( talent.shocking_lava -> ok() );
  buff.spirit_walk             = buff_creator_t( this, "spirit_walk", spec.spirit_walk );
  buff.spiritwalkers_grace     = buff_creator_t( this, "spiritwalkers_grace", find_class_spell( "Spiritwalker's Grace" ) )
                                 .chance( 1.0 )
                                 .duration( find_class_spell( "Spiritwalker's Grace" ) -> duration() +
                                            glyph.spiritwalkers_grace -> effectN( 1 ).time_value() +
                                            sets.set( SET_T13_4PC_HEAL ) -> effectN( 1 ).time_value() );
  buff.tidal_waves             = buff_creator_t( this, "tidal_waves", spec.tidal_waves -> ok() ? find_spell( 53390 ) : spell_data_t::not_found() );
  buff.unleash_flame           = new unleash_flame_buff_t( this );
  buff.unleashed_fury_wf       = buff_creator_t( this, "unleashed_fury_wf", find_spell( 118472 ) )
                                 .add_invalidate( CACHE_MULTISTRIKE );
  buff.water_shield            = buff_creator_t( this, "water_shield", find_class_spell( "Water Shield" ) );

  // Haste buffs
  buff.elemental_mastery       = haste_buff_creator_t( this, "elemental_mastery", talent.elemental_mastery )
                                 .chance( 1.0 )
                                 .add_invalidate( CACHE_HASTE );
  constant.haste_elemental_mastery = 1.0 / ( 1.0 + buff.elemental_mastery -> data().effectN( 1 ).percent() );

  buff.flurry                  = haste_buff_creator_t( this, "flurry", spec.flurry -> effectN( 1 ).trigger() )
                                 .chance( spec.flurry -> proc_chance() )
                                 .activated( false )
                                 .add_invalidate( CACHE_HASTE );
  constant.flurry_rating_multiplier = spec.flurry -> effectN( 1 ).trigger() -> effectN( 2 ).percent();
  constant.attack_speed_flurry = 1.0 / ( 1.0 + spec.flurry -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  buff.unleash_wind            = haste_buff_creator_t( this, "unleash_wind", find_spell( 73681 ) ).add_invalidate( CACHE_ATTACK_SPEED );
  constant.attack_speed_unleash_wind = 1.0 / ( 1.0 + buff.unleash_wind -> data().effectN( 1 ).percent() );

  buff.tier13_4pc_healer       = haste_buff_creator_t( this, "tier13_4pc_healer", find_spell( 105877 ) ).add_invalidate( CACHE_HASTE );

  // Stat buffs
  buff.elemental_blast_crit    = stat_buff_creator_t( this, "elemental_blast_crit", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_CRIT_RATING, find_spell( 118522 ) -> effectN( 1 ).average( this ) );
  buff.elemental_blast_haste   = stat_buff_creator_t( this, "elemental_blast_haste", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_HASTE_RATING, find_spell( 118522 ) -> effectN( 2 ).average( this ) );
  buff.elemental_blast_mastery = stat_buff_creator_t( this, "elemental_blast_mastery", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_MASTERY_RATING, find_spell( 118522 ) -> effectN( 3 ).average( this ) );
  buff.elemental_blast_multistrike = stat_buff_creator_t( this, "elemental_blast_multistrike", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_MULTISTRIKE_RATING, find_spell( 118522 ) -> effectN( 4 ).average( this ) );
  buff.elemental_blast_agility = stat_buff_creator_t( this, "elemental_blast_agility", find_spell( 118522 ) )
                                 .max_stack( 1 )
                                 .add_stat( STAT_AGILITY, find_spell( 118522 ) -> effectN( 5 ).average( this ) );
  buff.tier13_2pc_caster        = stat_buff_creator_t( this, "tier13_2pc_caster", find_spell( 105779 ) );
  buff.tier13_4pc_caster        = stat_buff_creator_t( this, "tier13_4pc_caster", find_spell( 105821 ) );
  buff.tier16_2pc_melee         = buff_creator_t( this, "tier16_2pc_melee", sets.set( SET_T16_2PC_MELEE ) -> effectN( 1 ).trigger() )
                                  .chance( static_cast< double >( sets.has_set_bonus( SET_T16_2PC_MELEE ) ) );


  buff.improved_chain_lightning = buff_creator_t( this, "improved_chain_lightning", find_perk_spell( "Improved Chain Lightning" ) )
                                  .max_stack( 5 ) // TODO-WOD: Figure this out from spell data
                                  .chance( perk.improved_chain_lightning -> ok() );
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.primal_wisdom        = get_gain( "primal_wisdom"     );
  gain.resurgence           = get_gain( "resurgence"        );
  gain.rolling_thunder      = get_gain( "rolling_thunder"   );
  gain.telluric_currents    = get_gain( "telluric_currents" );
  gain.thunderstorm         = get_gain( "thunderstorm"      );
  gain.water_shield         = get_gain( "water_shield"      );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.elemental_overload = get_proc( "elemental_overload"      );
  proc.lava_surge         = get_proc( "lava_surge"              );
  proc.ls_fast            = get_proc( "lightning_shield_too_fast_fill" );
  proc.maelstrom_weapon   = get_proc( "maelstrom_weapon"        );
  proc.swings_clipped_mh  = get_proc( "swings_clipped_mh"       );
  proc.swings_clipped_oh  = get_proc( "swings_clipped_oh"       );
  proc.swings_reset_mh    = get_proc( "swings_reset_mh"         );
  proc.swings_reset_oh    = get_proc( "swings_reset_oh"         );
  proc.rolling_thunder    = get_proc( "rolling_thunder"         );
  proc.uf_flame_shock     = get_proc( "uf_flame_shock"          );
  proc.uf_fire_nova       = get_proc( "uf_fire_nova"            );
  proc.uf_lava_burst      = get_proc( "uf_lava_burst"           );
  proc.uf_elemental_blast = get_proc( "uf_elemental_blast"      );
  proc.uf_wasted          = get_proc( "uf_wasted"               );
  proc.t15_2pc_melee      = get_proc( "t15_2pc_melee"           );
  proc.t16_2pc_melee      = get_proc( "t16_2pc_melee"           );
  proc.t16_4pc_caster     = get_proc( "t16_4pc_caster"          );
  proc.t16_4pc_melee      = get_proc( "t16_4pc_melee"           );
  proc.wasted_t15_2pc_melee = get_proc( "wasted_t15_2pc_melee"  );
  proc.wasted_lava_surge  = get_proc( "wasted_lava_surge"       );
  proc.wasted_ls          = get_proc( "wasted_lightning_shield" );
  proc.wasted_ls_shock_cd = get_proc( "wasted_lightning_shield_shock_cd" );
  proc.wasted_mw          = get_proc( "wasted_maelstrom_weapon" );
  proc.windfury           = get_proc( "windfury"                );
  proc.surge_during_lvb   = get_proc( "lava_surge_during_lvb"   );

  for ( int i = 0; i < 7; i++ )
  {
    proc.fulmination[ i ] = get_proc( "fulmination_" + util::to_string( i ) );
  }
}

// shaman_t::init_actions ===================================================

void shaman_t::init_action_list()
{
  if ( ! ( primary_role() == ROLE_ATTACK && specialization() == SHAMAN_ENHANCEMENT ) &&
       ! ( primary_role() == ROLE_SPELL  && specialization() == SHAMAN_ELEMENTAL   ) )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role (%s) or spec(%s) isn't supported yet.",
                     name(), util::role_type_string( primary_role() ), dbc::specialization_string( specialization() ).c_str() );
    quiet = true;
    return;
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  // Restoration isn't supported atm
  if ( specialization() == SHAMAN_RESTORATION && primary_role() == ROLE_HEAL )
  {
    if ( ! quiet )
      sim -> errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }

  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  int hand_addon = -1;

  for ( int i = 0; i < ( int ) items.size(); i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) && items[ i ].slot == SLOT_HANDS )
    {
      hand_addon = i;
      break;
    }
  }

  clear_action_priority_lists();

  std::vector<std::string> use_items;
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) && 
         ( specialization() != SHAMAN_ELEMENTAL || items[ i ].slot != SLOT_HANDS ) )
      use_items.push_back( "use_item,name=" + items[ i ].name_str );
  }

  action_priority_list_t* precombat = get_action_priority_list( "precombat" );
  action_priority_list_t* def       = get_action_priority_list( "default"   );
  action_priority_list_t* single    = get_action_priority_list( "single", "Single target action priority list" );
  action_priority_list_t* aoe       = get_action_priority_list( "aoe", "Multi target action priority list" );

  // Flask
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask_action = "flask,type=";
    if ( primary_role() == ROLE_ATTACK )
      flask_action += ( ( level > 85 ) ? "spring_blossoms" : ( level >= 80 ) ? "winds" : "" );
    else
      flask_action += ( ( level > 85 ) ? "warm_sun" : ( level >= 80 ) ? "draconic_mind" : "" );

    precombat -> add_action( flask_action );
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    if ( specialization() == SHAMAN_ENHANCEMENT )
      food_action += ( level > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
    else
      food_action += ( level > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";

    precombat -> add_action( food_action );
  }

  // Weapon Enchants
  if ( specialization() == SHAMAN_ENHANCEMENT && primary_role() == ROLE_ATTACK )
  {
    precombat -> add_action( this, "Windfury Weapon", "weapon=main" );
    if ( off_hand_weapon.type != WEAPON_NONE )
      precombat -> add_action( this, "Flametongue Weapon", "weapon=off" );
  }
  else
  {
    precombat -> add_action( this, "Flametongue Weapon", "weapon=main" );
    if ( specialization() == SHAMAN_ENHANCEMENT && off_hand_weapon.type != WEAPON_NONE )
      precombat -> add_action( this, "Flametongue Weapon", "weapon=off" );
  }

  // Active Shield, presume any non-restoration / healer wants lightning shield
  if ( specialization() != SHAMAN_RESTORATION || primary_role() != ROLE_HEAL )
    precombat -> add_action( this, "Lightning Shield", "if=!buff.lightning_shield.up" );
  else
    precombat -> add_action( this, "Water Shield", "if=!buff.water_shield.up" );

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( sim -> allow_potions && level >= 80 )
  {
    // Prepotion (work around for now, until snapshot_stats stop putting things into combat)
    if ( primary_role() == ROLE_ATTACK )
      precombat -> add_action( level > 85 ? "virmens_bite_potion" : "tolvir_potion" );
    else
      precombat -> add_action( level > 85 ? "jade_serpent_potion" : "volcanic_potion" );
  }

  // All Shamans Bloodlust and Wind Shear by default
  def -> add_action( this, "Wind Shear" );

  std::string bloodlust_options = "if=";

  if ( sim -> bloodlust_percent > 0 )
    bloodlust_options += "target.health.pct<" + util::to_string( sim -> bloodlust_percent ) + "|";

  if ( sim -> bloodlust_time < timespan_t::zero() )
    bloodlust_options += "target.time_to_die<" + util::to_string( - sim -> bloodlust_time.total_seconds() ) + "|";

  if ( sim -> bloodlust_time > timespan_t::zero() )
    bloodlust_options += "time>" + util::to_string( sim -> bloodlust_time.total_seconds() ) + "|";
  bloodlust_options.erase( bloodlust_options.end() - 1 );

  if ( action_priority_t* a = def -> add_action( this, "Bloodlust", bloodlust_options ) )
    a -> comment( "Bloodlust casting behavior mirrors the simulator settings for proxy bloodlust. See options 'bloodlust_percent', and 'bloodlust_time'. " );

  // Melee turns on auto attack
  if ( primary_role() == ROLE_ATTACK )
    def -> add_action( "auto_attack" );

  // On use stuff and profession abilities
  for ( size_t i = 0; i < use_items.size(); i++ )
    def -> add_action( use_items[ i ] );

  // In-combat potion
  if ( sim -> allow_potions && level >= 80  )
  {
    std::string potion_action = "";
    if ( primary_role() == ROLE_ATTACK )
      potion_action = level > 85 ? "virmens_bite_potion" : "tolvir_potion";
    else
      potion_action = level > 85 ? "jade_serpent_potion" : "volcanic_potion";

    potion_action += ",if=time>60&(pet.primal_fire_elemental.active|pet.greater_fire_elemental.active|target.time_to_die<=60)";

    def -> add_action( potion_action, "In-combat potion is linked to Primal or Greater Fire Elemental Totem, after the first 60 seconds of combat." );
  }

  if ( specialization() == SHAMAN_ENHANCEMENT && primary_role() == ROLE_ATTACK )
  {
    def -> add_action( "blood_fury" );
    def -> add_action( "arcane_torrent" );
    def -> add_action( "berserking" );

    def -> add_talent( this, "Elemental Mastery", "if=talent.primal_elementalist.enabled&glyph.fire_elemental_totem.enabled&(cooldown.fire_elemental_totem.remains=0|cooldown.fire_elemental_totem.remains>=80)" );
    def -> add_talent( this, "Elemental Mastery", "if=talent.primal_elementalist.enabled&!glyph.fire_elemental_totem.enabled&(cooldown.fire_elemental_totem.remains=0|cooldown.fire_elemental_totem.remains>=50)" );
    def -> add_talent( this, "Elemental Mastery", "if=!talent.primal_elementalist.enabled" );
    def -> add_action( this, "Fire Elemental Totem", "if=!active" );
    def -> add_action( this, "Ascendance", "if=cooldown.strike.remains>=3" );

    // Need to remove the "/" in front of the profession action(s) for the new default action priority list stuff :/
    def -> add_action( init_use_profession_actions( ",if=(glyph.fire_elemental_totem.enabled&(pet.primal_fire_elemental.active|pet.greater_fire_elemental.active))|!glyph.fire_elemental_totem.enabled" ).erase( 0, 1 ) );

    def -> add_action( "run_action_list,name=single,if=active_enemies=1", "If only one enemy, priority follows the 'single' action list." );
    def -> add_action( "run_action_list,name=aoe,if=active_enemies>1", "On multiple enemies, the priority follows the 'aoe' action list." );

    single -> add_action( this, "Searing Totem", "if=!totem.fire.active" );
    single -> add_action( this, "Unleash Elements", "if=(talent.unleashed_fury.enabled|set_bonus.tier16_2pc_melee=1)" );
    single -> add_talent( this, "Elemental Blast", "if=buff.maelstrom_weapon.react>=1" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react=5" );
    single -> add_action( this, "Feral Spirit", "if=set_bonus.tier15_4pc_melee=1" );
    single -> add_action( this, find_class_spell( "Ascendance" ), "windstrike" );
    single -> add_action( this, "Stormstrike" );
    single -> add_action( this, "Primal Strike" );
    single -> add_action( this, "Flame Shock", "if=buff.unleash_flame.up&!ticking" );
    single -> add_action( this, "Lava Lash" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=set_bonus.tier15_2pc_melee=1&buff.maelstrom_weapon.react>=4&!buff.ascendance.up" );
    single -> add_action( this, "Flame Shock", "if=(buff.unleash_flame.up&(dot.flame_shock.remains<10|action.flame_shock.tick_damage>dot.flame_shock.tick_dmg))|!ticking" );
    single -> add_action( this, "Unleash Elements" );
    single -> add_action( this, "Frost Shock", "if=glyph.frost_shock.enabled&set_bonus.tier14_4pc_melee=0" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react>=3&!buff.ascendance.up" );
    single -> add_talent( this, "Ancestral Swiftness", "if=buff.maelstrom_weapon.react<2" ) ;
    single -> add_action( this, "Lightning Bolt", "if=buff.ancestral_swiftness.up" );
    single -> add_action( this, "Earth Shock", "if=(!glyph.frost_shock.enabled|set_bonus.tier14_4pc_melee=1)" );
    single -> add_action( this, "Feral Spirit" );
    single -> add_action( this, "Earth Elemental Totem", "if=!active" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    single -> add_action( this, spec.maelstrom_weapon, "lightning_bolt", "if=buff.maelstrom_weapon.react>1&!buff.ascendance.up" );

    // AoE
    aoe -> add_action( this, "Fire Nova", "if=active_flame_shock>=4" );
    aoe -> add_action( "wait,sec=cooldown.fire_nova.remains,if=active_flame_shock>=4&cooldown.fire_nova.remains<0.67" );
    aoe -> add_action( this, "Magma Totem", "if=active_enemies>5&!totem.fire.active" );
    aoe -> add_action( this, "Searing Totem", "if=active_enemies<=5&!totem.fire.active" );
    aoe -> add_action( this, "Lava Lash", "if=dot.flame_shock.ticking" );
    aoe -> add_talent( this, "Elemental Blast", "if=buff.maelstrom_weapon.react>=1" );
    aoe -> add_action( this, spec.maelstrom_weapon, "chain_lightning", "if=active_enemies>=2&buff.maelstrom_weapon.react>=3" );
    aoe -> add_action( this, "Unleash Elements" );
    aoe -> add_action( this, "Flame Shock", "cycle_targets=1,if=!ticking" );
    aoe -> add_action( this, find_class_spell( "Ascendance" ), "windstrike" );
    aoe -> add_action( this, "Fire Nova", "if=active_flame_shock>=3" );
    aoe -> add_action( this, spec.maelstrom_weapon, "chain_lightning", "if=active_enemies>=2&buff.maelstrom_weapon.react>=1" );
    aoe -> add_action( this, "Stormstrike" );
    aoe -> add_action( this, "Primal Strike" );
    aoe -> add_action( this, "Earth Shock", "if=active_enemies<4" );
    aoe -> add_action( this, "Feral Spirit" );
    aoe -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=50" );
    aoe -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    aoe -> add_action( this, "Fire Nova", "if=active_flame_shock>=1" );
  }
  else if ( specialization() == SHAMAN_ELEMENTAL && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_DPS ) )
  {
    // Sync berserking with ascendance as they share a cooldown, but making sure
    // that no two haste cooldowns overlap, within reason
    def -> add_action( "berserking,if=!buff.bloodlust.up&!buff.elemental_mastery.up&(set_bonus.tier15_4pc_caster=1|(buff.ascendance.cooldown_remains=0&(dot.flame_shock.remains>buff.ascendance.duration|level<87)))" );
    // Sync blood fury with ascendance or fire elemental as long as one is ready
    // soon after blood fury is.
    def -> add_action( "blood_fury,if=buff.bloodlust.up|buff.ascendance.up|((cooldown.ascendance.remains>10|level<87)&cooldown.fire_elemental_totem.remains>10)" );
    def -> add_action( "arcane_torrent" );

    // Use Elemental Mastery after initial Bloodlust ends. Also make sure that
    // Elemental Mastery is not used during Ascendance, if Berserking is up.
    // Finally, after the second Ascendance (time 200+ seconds), start using
    // Elemental Mastery on cooldown.
    def -> add_talent( this, "Elemental Mastery", "if=time>15&((!buff.bloodlust.up&time<120)|(!buff.berserking.up&!buff.bloodlust.up&buff.ascendance.up)|(time>=200&(cooldown.ascendance.remains>30|level<87)))" );

    def -> add_talent( this, "Ancestral Swiftness", "if=!buff.ascendance.up" );
    def -> add_action( this, "Fire Elemental Totem", "if=!active" );

    // Use Ascendance preferably with a haste CD up, but dont overdo the
    // delaying. Make absolutely sure that Ascendance can be used so that
    // only Lava Bursts need to be cast during it's duration
    std::string ascendance_opts = "if=active_enemies>1|(dot.flame_shock.remains>buff.ascendance.duration&(target.time_to_die<20|buff.bloodlust.up";
    if ( race == RACE_TROLL )
      ascendance_opts += "|buff.berserking.up|set_bonus.tier15_4pc_caster=1";
    else
      ascendance_opts += "|time>=60";
    ascendance_opts += ")&cooldown.lava_burst.remains>0)";

    def -> add_action( this, "Ascendance", ascendance_opts );

    // Need to remove the "/" in front of the profession action(s) for the new default action priority list stuff :/
    def -> add_action( init_use_profession_actions().erase( 0, 1 ) );

    def -> add_action( "run_action_list,name=single,if=active_enemies=1", "If only one enemy, priority follows the 'single' action list." );
    def -> add_action( "run_action_list,name=aoe,if=active_enemies>1", "On multiple enemies, the priority follows the 'aoe' action list." );

    if ( hand_addon > -1 )
      single -> add_action( "use_item,name=" + items[ hand_addon ].name_str + ",if=((cooldown.ascendance.remains>10|level<87)&cooldown.fire_elemental_totem.remains>10)|buff.ascendance.up|buff.bloodlust.up|totem.fire_elemental_totem.active" );

    single -> add_action( this, "Unleash Elements", "if=talent.unleashed_fury.enabled&!buff.ascendance.up" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1,if=buff.ascendance.up" );
    if ( find_item( "unerring_vision_of_lei_shen" ) )
      single -> add_action( this, "Flame Shock", "if=buff.perfect_aim.react&crit_pct<100" );
    single -> add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time&(buff.ascendance.up|cooldown_react)" );
    single -> add_action( this, "Flame Shock", "if=ticks_remain<2" );
    single -> add_talent( this, "Elemental Blast" );
    single -> add_action( this, spec.fulmination, "earth_shock", "if=buff.lightning_shield.react=buff.lightning_shield.max_stack",
                          "Use Earth Shock if Lightning Shield is at max (7) charges" );
    single -> add_action( this, spec.fulmination, "earth_shock", "if=buff.lightning_shield.react>3&dot.flame_shock.remains>cooldown&dot.flame_shock.remains<cooldown+action.flame_shock.tick_time",
                          "Use Earth Shock if Lightning Shield is above 3 charges and the Flame Shock remaining duration is longer than the shock cooldown but shorter than shock cooldown + tick time interval" );
    single -> add_action( this, "Flame Shock", "if=time>60&remains<=buff.ascendance.duration&cooldown.ascendance.remains+buff.ascendance.duration<duration",
                          "After the initial Ascendance, use Flame Shock pre-emptively just before Ascendance to guarantee Flame Shock staying up for the full duration of the Ascendance buff" );
    single -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=60" );
    single -> add_action( this, "Searing Totem", "if=cooldown.fire_elemental_totem.remains>20&!totem.fire.active",
                          "Keep Searing Totem up, unless Fire Elemental Totem is coming off cooldown in the next 20 seconds" );
    single -> add_action( this, "Spiritwalker's Grace", "moving=1,if=((talent.elemental_blast.enabled&cooldown.elemental_blast.remains=0)|(cooldown.lava_burst.remains=0&!buff.lava_surge.react))|(buff.raid_movement.duration>=action.unleash_elements.gcd+action.earth_shock.gcd)" );
//  single -> add_action( this, "Unleash Elements", "moving=1,if=!glyph.unleashed_lightning.enabled" );
//  single -> add_action( this, "Earth Shock", "moving=1,if=!glyph.unleashed_lightning.enabled&dot.flame_shock.remains>cooldown",
//                        "Use Earth Shock when moving if Glyph of Unleashed Lightning is not equipped and there's at least shock cooldown time of Flame Shock duration left" );
    single -> add_action( this, "Lightning Bolt" );

    // AoE
    aoe -> add_action( this, find_class_spell( "Ascendance" ), "lava_beam" );
    aoe -> add_action( this, "Magma Totem", "if=active_enemies>2&!totem.fire.active" );
    aoe -> add_action( this, "Searing Totem", "if=active_enemies<=2&!totem.fire.active" );
    aoe -> add_action( this, "Lava Burst", "if=active_enemies<3&dot.flame_shock.remains>cast_time&cooldown_react" );
    aoe -> add_action( this, "Flame Shock", "cycle_targets=1,if=!ticking&active_enemies<3" );
    aoe -> add_action( this, "Earthquake", "if=active_enemies>4" );
    aoe -> add_action( this, "Thunderstorm", "if=mana.pct_nonproc<80" );
    aoe -> add_action( this, "Chain Lightning", "if=mana.pct_nonproc>10" );
    aoe -> add_action( this, "Lightning Bolt" );
  }
  else if ( primary_role() == ROLE_SPELL )
  {
    def -> add_action( this, "Fire Elemental Totem", "if=!active" );
    def -> add_action( this, "Searing Totem", "if=!totem.fire.active" );
    def -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=50" );
    def -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    def -> add_talent( this, "Elemental Mastery" );
    def -> add_talent( this, "Elemental Blast" );
    def -> add_talent( this, "Ancestral Swiftness" );
    def -> add_action( this, "Flame Shock", "if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)" );
    def -> add_action( this, "Lava Burst", "if=dot.flame_shock.remains>cast_time" );
    def -> add_action( this, "Chain Lightning", "if=target.adds>2&mana.pct>25" );
    def -> add_action( this, "Lightning Bolt" );
  }
  else if ( primary_role() == ROLE_ATTACK )
  {
    def -> add_action( this, "Fire Elemental Totem", "if=!active" );
    def -> add_action( this, "Searing Totem", "if=!totem.fire.active" );
    def -> add_action( this, "Earth Elemental Totem", "if=!active&cooldown.fire_elemental_totem.remains>=50" );
    def -> add_action( this, "Spiritwalker's Grace", "moving=1" );
    def -> add_talent( this, "Elemental Mastery" );
    def -> add_talent( this, "Elemental Blast" );
    def -> add_talent( this, "Ancestral Swiftness" );
    def -> add_talent( this, "Primal Strike" );
    def -> add_action( this, "Flame Shock", "if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)" );
    def -> add_action( this, "Earth Shock" );
    def -> add_action( this, "Lightning Bolt", "moving=1" );
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// shaman_t::moving =========================================================

void shaman_t::moving()
{
  // Spiritwalker's Grace complicates things, as you can cast it while casting
  // anything. So, to model that, if a raid move event comes, we need to check
  // if we can trigger Spiritwalker's Grace. If so, conditionally execute it, to
  // allow the currently executing cast to finish.
  if ( level >= 85 )
  {
    action_t* swg = find_action( "spiritwalkers_grace" );

    // We need to bypass swg -> ready() check here, so whip up a special
    // readiness check that only checks for player skill, cooldown and resource
    // availability
    if ( swg && executing && swg -> ready() )
    {
      // Shaman executes SWG mid-cast during a movement event, if
      // 1) The profile does not have Glyph of Unleashed Lightning and is
      //    casting a Lighting Bolt (non-instant cast)
      // 2) The profile is casting Lava Burst (without Lava Surge)
      // 3) The profile is casting Chain Lightning
      // 4) The profile is casting Elemental Blast
      if ( ( executing -> id == 51505 ) ||
           ( executing -> id == 421 ) ||
           ( executing -> id == 117014 ) )
      {
        if ( sim -> log )
          sim -> out_log.printf( "%s spiritwalkers_grace during spell cast, next cast (%s) should finish",
                         name(), executing -> name() );
        swg -> execute();
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack ) main_hand_attack -> cancel();
    if (  off_hand_attack )  off_hand_attack -> cancel();
  }
  else
  {
    halt();
  }
}

// shaman_t::matching_gear_multiplier =======================================

double shaman_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_AGILITY || attr == ATTR_INTELLECT )
    return constant.matching_gear_multiplier;

  return 0.0;
}

// shaman_t::composite_spell_haste ==========================================

double shaman_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_ancestral_swiftness;

  if ( buff.elemental_mastery -> up() )
    h *= constant.haste_elemental_mastery;

  if ( buff.tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + buff.tier13_4pc_healer -> data().effectN( 1 ).percent() );
  return h;
}

// shaman_t::composite_spell_crit =========================================

double shaman_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// shaman_t::temporary_movement_modifier =======================================

double shaman_t::temporary_movement_modifier() const
{
  double ms = player_t::temporary_movement_modifier();

  if ( buff.spirit_walk -> up() )
    ms = std::max( buff.spirit_walk -> data().effectN( 1 ).percent(), ms );

  return ms;
}

// shaman_t::composite_attack_haste =========================================

double shaman_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buff.elemental_mastery -> up() )
    h *= constant.haste_elemental_mastery;

  if ( talent.ancestral_swiftness -> ok() )
    h *= constant.haste_ancestral_swiftness;

  if ( buff.tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + buff.tier13_4pc_healer -> data().effectN( 1 ).percent() );

  return h;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_melee_speed() const
{
  double speed = player_t::composite_melee_speed();

  if ( buff.flurry -> up() )
    speed *= constant.attack_speed_flurry;

  if ( buff.unleash_wind -> up() )
    speed *= constant.attack_speed_unleash_wind;

  if ( talent.ancestral_swiftness -> ok() )
    speed *= constant.speed_attack_ancestral_swiftness;

  return speed;
}

// shaman_t::composite_melee_crit =========================================

double shaman_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( school_e school ) const
{
  double sp = 0;

  if ( specialization() == SHAMAN_ENHANCEMENT )
    sp = composite_attack_power_multiplier() * cache.attack_power() * spec.mental_quickness -> effectN( 1 ).percent();
  else
    sp = player_t::composite_spell_power( school );

  return sp;
}

// shaman_t::composite_spell_power_multiplier ===============================

double shaman_t::composite_spell_power_multiplier() const
{
  if ( specialization() == SHAMAN_ENHANCEMENT )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// shaman_t::composite_player_multiplier ====================================

double shaman_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( school != SCHOOL_PHYSICAL )
  {
    if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + main_hand_weapon.buff_value;
    else if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + off_hand_weapon.buff_value;
  }

  if ( mastery.enhanced_elements -> ok() &&
       ( dbc::is_school( school, SCHOOL_FIRE   ) ||
         dbc::is_school( school, SCHOOL_FROST  ) ||
         dbc::is_school( school, SCHOOL_NATURE ) ) )
  {
    m *= 1.0 + cache.mastery_value();
  }

  return m;
}

// shaman_t::composite_rating_multiplier ====================================

double shaman_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      if ( buff.flurry -> up() )
        m *= 1.0 + constant.flurry_rating_multiplier;
      break;
    default: break;
  }

  return m;
}

// shaman_t::composite_multistrike ==========================================

double shaman_t::composite_multistrike() const
{
  double m = player_t::composite_multistrike();

  // TODO-WOD: Flat or multiplicative bonus?
  if ( buff.unleashed_fury_wf -> up() )
    m += buff.unleashed_fury_wf -> data().effectN( 1 ).percent();

  return m;
}

// shaman_t::target_mitigation ==============================================

void shaman_t::target_mitigation( school_e school, dmg_e type, action_state_t* state )
{
  player_t::target_mitigation( school, type, state );

  if ( buff.lightning_shield -> check() )
  {
    state -> result_amount *= 1.0 + glyph.lightning_shield -> effectN( 1 ).percent();
  }
}

// shaman_t::invalidate_cache ===============================================

void shaman_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
    case CACHE_STRENGTH:
    case CACHE_ATTACK_POWER:
      if ( specialization() == SHAMAN_ENHANCEMENT )
        player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_MASTERY:
      if ( mastery.enhanced_elements -> ok() )
      {
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      }
      break;
    default: break;
  }
}

// shaman_t::regen  =========================================================

void shaman_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buff.water_shield -> up() )
  {
    double water_shield_regen = periodicity.total_seconds() * buff.water_shield -> value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gain.water_shield );
  }
}

// shaman_t::arise() ========================================================

void shaman_t::arise()
{
  player_t::arise();

  assert( main_hand_attack == melee_mh && off_hand_attack == melee_oh );

  if ( ! sim -> overrides.mastery && dbc.spell( 116956 ) -> is_level( level ) )
  {
    double mastery_rating = dbc.spell( 116956 ) -> effectN( 1 ).average( this );
    if ( ! sim -> auras.mastery -> check() || sim -> auras.mastery -> current_value < mastery_rating )
      sim -> auras.mastery -> trigger( 1, mastery_rating );
  }

  if ( ! sim -> overrides.spell_power_multiplier && dbc.spell( 77747 ) -> is_level( level ) )
    sim -> auras.spell_power_multiplier -> trigger();

  if ( specialization() == SHAMAN_ENHANCEMENT && ! sim -> overrides.haste && dbc.spell( 30809 ) -> is_level( level ) )
    sim -> auras.haste -> trigger();

  if ( specialization() == SHAMAN_ELEMENTAL && ! sim -> overrides.haste && dbc.spell( 51470 ) -> is_level( level ) )
    sim -> auras.haste  -> trigger();
}

// shaman_t::reset ==========================================================

void shaman_t::reset()
{
  player_t::reset();

  ls_reset = timespan_t::zero();
  active_flame_shocks = 0;
  lava_surge_during_lvb = false;
  rppm_echo_of_the_elements.reset();
  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> reset();
}

// shaman_t::merge ==========================================================

void shaman_t::merge( player_t& other )
{
  player_t::merge( other );

  shaman_t& s = static_cast<shaman_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> merge( *s.counters[ i ] );
}

// shaman_t::decode_set =====================================================

set_e shaman_t::decode_set( const item_t& item ) const
{
  bool is_caster = false, is_melee = false, is_heal = false;

  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "spiritwalkers" ) )
  {
    is_caster = ( strstr( s, "headpiece"     ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "hauberk"       ) ||
                  strstr( s, "kilt"          ) ||
                  strstr( s, "gloves"        ) );

    is_melee = ( strstr( s, "helmet"         ) ||
                 strstr( s, "spaulders"      ) ||
                 strstr( s, "cuirass"        ) ||
                 strstr( s, "legguards"      ) ||
                 strstr( s, "grips"          ) );

    is_heal  = ( strstr( s, "faceguard"      ) ||
                 strstr( s, "mantle"         ) ||
                 strstr( s, "tunic"          ) ||
                 strstr( s, "legwraps"       ) ||
                 strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T13_CASTER;
    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_heal   ) return SET_T13_HEAL;
  }

  if ( strstr( s, "firebirds_" ) )
  {
    is_caster = ( strstr( s, "headpiece"     ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "hauberk"       ) ||
                  strstr( s, "kilt"          ) ||
                  strstr( s, "gloves"        ) );

    is_melee = ( strstr( s, "helmet"         ) ||
                 strstr( s, "spaulders"      ) ||
                 strstr( s, "cuirass"        ) ||
                 strstr( s, "legguards"      ) ||
                 strstr( s, "grips"          ) );

    is_heal  = ( strstr( s, "faceguard"      ) ||
                 strstr( s, "mantle"         ) ||
                 strstr( s, "tunic"          ) ||
                 strstr( s, "legwraps"       ) ||
                 strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T14_CASTER;
    if ( is_melee  ) return SET_T14_MELEE;
    if ( is_heal   ) return SET_T14_HEAL;
  }

  if ( strstr( s, "_of_the_witch_doctor" ) )
  {
    is_caster = ( strstr( s, "headpiece"     ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "hauberk"       ) ||
                  strstr( s, "kilt"          ) ||
                  strstr( s, "gloves"        ) );

    is_melee = ( strstr( s, "helmet"         ) ||
                 strstr( s, "spaulders"      ) ||
                 strstr( s, "cuirass"        ) ||
                 strstr( s, "legguards"      ) ||
                 strstr( s, "grips"          ) );

    is_heal  = ( strstr( s, "faceguard"      ) ||
                 strstr( s, "mantle"         ) ||
                 strstr( s, "tunic"          ) ||
                 strstr( s, "legwraps"       ) ||
                 strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T15_CASTER;
    if ( is_melee  ) return SET_T15_MELEE;
    if ( is_heal   ) return SET_T15_HEAL;
  }

  if ( util::str_in_str_ci( s, "_of_celestial_harmony" ) )
  {
    is_caster = ( util::str_in_str_ci( s, "hauberk" ) ||
                  util::str_in_str_ci( s, "gloves" ) ||
                  util::str_in_str_ci( s, "headpiece" ) ||
                  util::str_in_str_ci( s, "leggings" ) ||
                  util::str_in_str_ci( s, "shoulderwraps" ) );

    is_melee = ( util::str_in_str_ci( s, "cuirass" ) ||
                 util::str_in_str_ci( s, "grips" ) ||
                 util::str_in_str_ci( s, "helmet" ) ||
                 util::str_in_str_ci( s, "legguards" ) ||
                 util::str_in_str_ci( s, "spaulders" ) );

    is_heal = ( util::str_in_str_ci( s, "tunic" ) ||
                util::str_in_str_ci( s, "handwraps" ) ||
                util::str_in_str_ci( s, "faceguard" ) ||
                util::str_in_str_ci( s, "legwraps" ) ||
                util::str_in_str_ci( s, "mantle" ) );

    if ( is_caster ) return SET_T16_CASTER;
    if ( is_melee  ) return SET_T16_MELEE;
    if ( is_heal   ) return SET_T16_HEAL;
  }

  if ( strstr( s, "_gladiators_linked_"   ) )     return SET_PVP_MELEE;
  if ( strstr( s, "_gladiators_mail_"     ) )     return SET_PVP_CASTER;
  if ( strstr( s, "_gladiators_ringmail_" ) )     return SET_PVP_MELEE;

  return SET_NONE;
}

// shaman_t::primary_role ===================================================

role_e shaman_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HYBRID;//To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == SHAMAN_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_SPELL;
  }

  else if ( specialization() == SHAMAN_ENHANCEMENT )
    return ROLE_ATTACK;

  else if ( specialization() == SHAMAN_ELEMENTAL )
    return ROLE_SPELL;

  return player_t::primary_role();
}

// shaman_t::convert_hybrid_stat ===========================================
stat_e shaman_t::convert_hybrid_stat( stat_e s ) const
{  
  switch ( s )
  {
  case STAT_AGI_INT: 
    if ( specialization() == SHAMAN_ENHANCEMENT )
      return STAT_AGILITY;
    else
      return STAT_INTELLECT; 
  // This is a guess at how AGI/STR gear will work for Resto/Elemental, TODO: confirm  
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Enhance, TODO: confirm  
  // this should probably never come up since shamans can't equip plate, but....
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == SHAMAN_RESTORATION )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return STAT_NONE;     
  default: return s; 
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class shaman_report_t : public player_report_extension_t
{
public:
  shaman_report_t( shaman_t& player ) :
      p( player )
  { }

  void mwgen_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
         << "<tr>\n"
           << "<th>Ability</th>\n"
           << "<th>Generated</th>\n"
           << "<th>Wasted</th>\n"
         << "</tr>\n";
  }

  void mwuse_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;\">\n"
         << "<tr style=\"vertical-align: bottom;\">\n"
           << "<th rowspan=\"2\">Ability</th>\n"
           << "<th rowspan=\"2\">Event</th>\n"
           << "<th rowspan=\"2\">0</th rowspan=\"2\"><th rowspan=\"2\">1</th><th rowspan=\"2\">2</th><th rowspan=\"2\">3</th><th rowspan=\"2\">4</th><th rowspan=\"2\">5</th><th colspan=\"2\">Total</th>\n"
         << "</tr>\n"
         << "<tr><th>casts</th><th>charges</th></tr>\n";
  }

  void mwgen_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mwuse_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void mwgen_table_contents( report::sc_html_stream& os )
  {
    double total_generated = 0, total_wasted = 0;
    int n = 0;

    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      double n_generated = 0, n_wasted = 0;

      for ( size_t j = 0, end2 = stats -> action_list.size(); j < end2; j++ )
      {
        shaman_attack_t* a = dynamic_cast<shaman_attack_t*>( stats -> action_list[ j ] );
        if ( ! a )
          continue;

        if ( ! a -> may_proc_maelstrom )
          continue;

        n_generated += a -> maelstrom_procs -> mean();
        total_generated += a -> maelstrom_procs -> mean();
        n_wasted += a -> maelstrom_procs_wasted -> mean();
        total_wasted += a -> maelstrom_procs_wasted -> mean();
      }

      if ( n_generated > 0 || n_wasted > 0 )
      {
        wowhead::wowhead_e domain = SC_BETA ? wowhead::BETA : wowhead::LIVE;
        if ( ! SC_BETA && p.dbc.ptr )
          domain = wowhead::PTR;

        std::string name_str = wowhead::decorated_action_name( stats -> name_str, 
                                                               stats -> action_list[ 0 ],
                                                               domain );
        std::string row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        os.printf("<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f (%.2f%%)</td></tr>\n",
            row_class_str.c_str(),
            name_str.c_str(),
            util::round( n_generated, 2 ),
            util::round( n_wasted, 2 ), util::round( 100.0 * n_wasted / n_generated, 2 ) );
      }
    }

    os.printf("<tr><td class=\"left\">Total</td><td class=\"right\">%.2f</td><td class=\"right\">%.2f (%.2f%%)</td></tr>\n",
        total_generated, total_wasted, 100.0 * total_wasted / total_generated );
  }

  void mwuse_table_contents( report::sc_html_stream& os )
  {
    std::vector<double> total_mw_cast( MAX_MAELSTROM_STACK + 2 );
    std::vector<double> total_mw_executed( MAX_MAELSTROM_STACK + 2 );
    int n = 0;
    std::string row_class_str = "";

    for ( size_t i = 0, end = p.action_list.size(); i < end; i++ )
    {
      if ( shaman_spell_t* s = dynamic_cast<shaman_spell_t*>( p.action_list[ i ] ) )
      {
        for ( size_t j = 0, end2 = s -> maelstrom_weapon_cast.size() - 1; j < end2; j++ )
        {
          total_mw_cast[ j ] += s -> maelstrom_weapon_cast[ j ] -> mean();
          total_mw_cast[ MAX_MAELSTROM_STACK + 1 ] += s -> maelstrom_weapon_cast[ j ] -> mean();

          total_mw_executed[ j ] += s -> maelstrom_weapon_executed[ j ] -> mean();
          total_mw_executed[ MAX_MAELSTROM_STACK + 1 ] += s -> maelstrom_weapon_executed[ j ] -> mean();
        }
      }
    }

    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      std::vector<double> n_cast( MAX_MAELSTROM_STACK + 2 );
      std::vector<double> n_executed( MAX_MAELSTROM_STACK + 2 );
      double n_cast_charges = 0, n_executed_charges = 0;
      bool has_data = false;

      for ( size_t j = 0, end2 = stats -> action_list.size(); j < end2; j++ )
      {
        if ( shaman_spell_t* s = dynamic_cast<shaman_spell_t*>( stats -> action_list[ j ] ) )
        {
          for ( size_t k = 0, end3 = s -> maelstrom_weapon_cast.size() - 1; k < end3; k++ )
          {
            if ( s -> maelstrom_weapon_cast[ k ] -> mean() > 0 || s -> maelstrom_weapon_executed[ k ] -> mean() > 0 )
              has_data = true;

            n_cast[ k ] += s -> maelstrom_weapon_cast[ k ] -> mean();
            n_cast[ MAX_MAELSTROM_STACK + 1 ] += s -> maelstrom_weapon_cast[ k ] -> mean();

            n_cast_charges += s -> maelstrom_weapon_cast[ k ] -> mean() * k;

            n_executed[ k ] += s -> maelstrom_weapon_executed[ k ] -> mean();
            n_executed[ MAX_MAELSTROM_STACK + 1 ] += s -> maelstrom_weapon_executed[ k ] -> mean();

            n_executed_charges += s -> maelstrom_weapon_executed[ k ] -> mean() * k;
          }
        }
      }

      if ( has_data )
      {
        row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        wowhead::wowhead_e domain = SC_BETA ? wowhead::BETA : wowhead::LIVE;
        if ( ! SC_BETA && p.dbc.ptr )
          domain = wowhead::PTR;

        std::string name_str = wowhead::decorated_action_name( stats -> name_str.c_str(), 
                                                               stats -> action_list[ 0 ],
                                                               domain );

        os.printf("<tr%s><td rowspan=\"2\" class=\"left\" style=\"vertical-align: top;\">%s</td>",
            row_class_str.c_str(), name_str.c_str() );

        os << "<td class=\"left\">Cast</td>";

        for ( size_t j = 0, end2 = n_cast.size(); j < end2; j++ )
        {
          double pct = 0;
          if ( total_mw_cast[ j ] > 0 )
            pct = 100.0 * n_cast[ j ] / n_cast[ MAX_MAELSTROM_STACK + 1 ];

          if ( j < end2 - 1 )
            os.printf("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_cast[ j ], 1 ), util::round( pct, 1 ) );
          else
          {
            os.printf("<td class=\"right\">%.1f</td>", util::round( n_cast[ j ], 1 ) );
            os.printf("<td class=\"right\">%.1f</td>", util::round( n_cast_charges, 1 ) );
          }
        }

        os << "</tr>\n";

        os.printf("<tr%s>", row_class_str.c_str() );

        os << "<td class=\"left\">Execute</td>";

        for ( size_t j = 0, end2 = n_executed.size(); j < end2; j++ )
        {
          double pct = 0;
          if ( total_mw_executed[ j ] > 0 )
            pct = 100.0 * n_executed[ j ] / n_executed[ MAX_MAELSTROM_STACK + 1 ];

          if ( j < end2 - 1 )
            os.printf("<td class=\"right\">%.1f (%.1f%%)</td>", util::round( n_executed[ j ], 1 ), util::round( pct, 1 ) );
          else
          {
            os.printf("<td class=\"right\">%.1f</td>", util::round( n_executed[ j ], 1 ) );
            os.printf("<td class=\"right\">%.1f</td>", util::round( n_executed_charges, 1 ) );
          }
        }

        os << "</tr>\n";
      }
    }
  }

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.specialization() != SHAMAN_ENHANCEMENT )
      return;

    // Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Maelstrom Weapon details</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    
    mwgen_table_header( os );
    mwgen_table_contents( os );
    mwgen_table_footer( os );

    mwuse_table_header( os );
    mwuse_table_contents( os );
    mwuse_table_footer( os );

    os << "<div class=\"clear\"></div>\n";

    os << "\t\t\t\t\t\t</div>\n"
       << "\t\t\t\t\t</div>\n";
  }
private:
  shaman_t& p;
};

// SHAMAN MODULE INTERFACE ==================================================

struct shaman_module_t : public module_t
{
  shaman_module_t() : module_t( SHAMAN ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    shaman_t* p = new shaman_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new shaman_report_t( *p ) );
    return p;
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[i];
      p -> buffs.bloodlust  = haste_buff_creator_t( p, "bloodlust", p -> find_spell( 2825 ) )
                              .max_stack( 1 );

      p -> buffs.earth_shield = buff_creator_t( p, "earth_shield", p -> find_spell( 974 ) )
                                .cd( timespan_t::from_seconds( 2.0 ) );

      p -> buffs.exhaustion = buff_creator_t( p, "exhaustion", p -> find_spell( 57723 ) )
                              .max_stack( 1 )
                              .quiet( true );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::shaman()
{
  static shaman_module_t m;
  return &m;
}
