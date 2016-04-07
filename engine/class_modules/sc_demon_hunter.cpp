// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace  // UNNAMED NAMESPACE
{
/* Forward declarations
 */
class demon_hunter_t;
namespace actions
{
  namespace attacks
  {
    struct melee_t;
    struct chaos_blade_t;
  }
}

/**
 * Demon Hunter target data
 * Contains target specific things
 */
class demon_hunter_td_t : public actor_target_data_t
{
public:
  struct dots_t
  {
  } dots;
  
  struct debuffs_t
  {
    buff_t* nemesis;
  } debuffs;

  demon_hunter_td_t( player_t* target, demon_hunter_t& p );
};

/* Demon Hunter class definition
 *
 * Derived from player_t. Contains everything that defines the Demon Hunter
 * class.
 */
class demon_hunter_t : public player_t
{
public:
  typedef player_t base_t;

  event_t* blade_dance_driver;
  std::vector<attack_t*> blade_dance_attacks;
  std::vector<attack_t*> death_sweep_attacks;

  // Autoattacks
  actions::attacks::melee_t* melee_main_hand;
  actions::attacks::melee_t* melee_off_hand;
  actions::attacks::chaos_blade_t* chaos_blade_main_hand;
  actions::attacks::chaos_blade_t* chaos_blade_off_hand;

  // Buffs
  struct
  {
    buff_t* blade_dance;
    buff_t* blur;
    buff_t* chaos_blades;
    buff_t* death_sweep;
    buff_t* metamorphosis;
    buff_t* momentum;
    buff_t* nemesis;
    buff_t* prepared;
    buff_t* vengeful_retreat_jump_cancel;
  } buff;

  // Talents
  struct
  {
    // Havoc
    const spell_data_t* fel_mastery;
    const spell_data_t* demonic_appetite;
    const spell_data_t* blind_fury;

    const spell_data_t* prepared;
    const spell_data_t* demon_blades;
    const spell_data_t* fel_eruption;

    const spell_data_t* felblade;
    const spell_data_t* first_blood;
    const spell_data_t* demon_reborn;
    
    const spell_data_t* momentum;
    const spell_data_t* bloodlet;
    const spell_data_t* nemesis;
    
    const spell_data_t* master_of_the_glaive;
    const spell_data_t* unleashed_power;
    const spell_data_t* chaos_cleave;
    
    const spell_data_t* chaos_blades;
    const spell_data_t* demonic;

    // NYI
    const spell_data_t* netherwalk;
    const spell_data_t* desperate_instincts;
    const spell_data_t* soul_rending;

    const spell_data_t* fel_barrage;
  } talent;

  // Specialization Spells
  struct
  {
    const spell_data_t* demon_hunter;

    const spell_data_t* havoc;
    const spell_data_t* annihilation;
    const spell_data_t* blade_dance;
    const spell_data_t* blur;
    const spell_data_t* chaos_nova;
    const spell_data_t* chaos_strike;
    const spell_data_t* consume_magic;
    const spell_data_t* death_sweep;
    const spell_data_t* demonic_instincts;
    const spell_data_t* metamorphosis_buff;

    const spell_data_t* vengeance;
  } spec;

  // Mastery Spells
  struct
  {
    const spell_data_t* demonic_presence;
    const spell_data_t* fel_blood;
  } mastery_spell;

  // Cooldowns
  struct
  {
    cooldown_t* blade_dance;
    cooldown_t* blur;
    cooldown_t* death_sweep;
    cooldown_t* chaos_nova;
    cooldown_t* chaos_blades;
    cooldown_t* consume_magic;
    cooldown_t* demonic_appetite;
    cooldown_t* eye_beam;
    cooldown_t* felblade;
    cooldown_t* fel_barrage;
    cooldown_t* fel_eruption;
    cooldown_t* fel_rush;
    cooldown_t* fury_of_the_illidari;
    cooldown_t* nemesis;
    cooldown_t* netherwalk;
    cooldown_t* throw_glaive;
    cooldown_t* vengeful_retreat;
  } cooldown;

  // Gains
  struct
  {
    gain_t* fury_refund;
    gain_t* demonic_appetite;
    gain_t* prepared;
  } gain;

  // Benefits
  struct
  {
  } benefits;

  // Procs
  struct
  {
    proc_t* delayed_aa_range;
    proc_t* delayed_aa_channel;
    proc_t* demonic_appetite;
    proc_t* felblade_reset;
    proc_t* demon_blade_supp;
  } proc;

  // RPPM objects
  struct rppms_t
  {
    // Havoc
    real_ppm_t felblade;
  } rppm;

  // Special
  struct
  {
  } active_spells;

  // Pets
  struct
  {
  } pets;

  // Options
  struct demon_hunter_options_t
  {
  } options;

  // Glyphs
  struct
  {
  } glyphs;

  demon_hunter_t( sim_t* sim, const std::string& name, race_e r );

  // player_t overrides
  void init() override;
  void init_base_stats() override;
  void init_procs() override;
  void init_resources( bool force ) override;
  void init_rng() override;
  void init_spells() override;
  void create_buffs() override;
  void init_scaling() override;
  void invalidate_cache( cache_e ) override;
  void reset() override;
  void interrupt() override;
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
    return RESOURCE_FURY;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_dodge() const override;
  double composite_player_multiplier( school_e ) const override;
  double passive_movement_modifier() const override;
  void target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void init_action_list() override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  bool has_t18_class_trinket() const override;

private:
  void create_cooldowns();
  void create_gains();
  void create_benefits();
  void apl_precombat();
  void apl_default();
  void apl_havoc();
  void apl_vengeance();
  demon_hunter_td_t* find_target_data( player_t* target ) const;

  target_specific_t<demon_hunter_td_t> _target_data;
};

namespace pets
{
// ==========================================================================
// Demon Hunter Pets
// ==========================================================================

/* Demon Hunter pet base
 *
 * defines characteristics common to ALL Demon Hunter pets
 */
struct demon_hunter_pet_t : public pet_t
{
  demon_hunter_pet_t( sim_t* sim, demon_hunter_t& owner,
                      const std::string& pet_name, pet_e pt,
                      bool guardian = false )
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

  demon_hunter_t& o() const
  {
    return static_cast<demon_hunter_t&>( *owner );
  }
};

namespace actions
{  // namespace for pet actions

}  // end namespace actions ( for pets )

}  // END pets NAMESPACE

namespace actions
{
/* This is a template for common code for all Demon Hunter actions.
 * The template is instantiated with either spell_t, heal_t or absorb_t as the
 * 'Base' class.
 * Make sure you keep the inheritance hierarchy and use base_t in the derived
 * class,
 * don't skip it and call spell_t/heal_t or absorb_t directly.
 */
template <typename Base>
class demon_hunter_action_t : public Base
{
public:
  gain_t* action_gain;
  bool metamorphosis_gcd;
  bool hasted_gcd, hasted_cd;
  
   
  demon_hunter_action_t( const std::string& n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil() )
    : ab( n, p, s ), action_gain( p -> get_gain( n ) ),
    metamorphosis_gcd( Base::data().affected_by( p -> spec.metamorphosis_buff -> effectN( 7 ) ) )
  {
    ab::may_crit         = true;
    ab::tick_may_crit    = true;

    switch ( p -> specialization() )
    {
    case DEMON_HUNTER_HAVOC:
      hasted_gcd = ab::data().affected_by( p -> spec.havoc -> effectN( 1 ) );
      hasted_cd  = ab::data().affected_by( p -> spec.havoc -> effectN( 2 ) );
      break;
    case DEMON_HUNTER_VENGEANCE:
      hasted_gcd = ab::data().affected_by( p -> spec.vengeance -> effectN( 1 ) );
      hasted_cd  = ab::data().affected_by( p -> spec.vengeance -> effectN( 2 ) );
      break;
    }
  }

  demon_hunter_t* p()
  { return static_cast<demon_hunter_t*>( ab::player ); }

  const demon_hunter_t* p() const
  { return static_cast<const demon_hunter_t*>( ab::player ); }

  demon_hunter_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
      return g;

    if ( metamorphosis_gcd && p() -> buff.metamorphosis -> up() )
    {
      g += p() -> spec.metamorphosis_buff -> effectN( 7 ).time_value();
    }

    if ( hasted_gcd )
    {
      g *= p() -> cache.attack_haste();
    }

    if ( g < ab::min_gcd )
      g = ab::min_gcd;

    return g;
  }

  double cooldown_reduction() const override
  {
    double cdr = ab::cooldown_reduction();

    if ( hasted_cd )
    {
      cdr *= p() -> cache.attack_haste();
    }

    return cdr;
  }

protected:
  /// typedef for demon_hunter_action_t<action_base_t>
  typedef demon_hunter_action_t base_t;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  typedef Base ab;
};

// ==========================================================================
// Demon Hunter heals
// ==========================================================================

struct demon_hunter_heal_t: public demon_hunter_action_t < heal_t >
{
  demon_hunter_heal_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    target = p;
  }
};

// ==========================================================================
// Demon Hunter spells -- For spells that deal no damage, usually buffs.
// ==========================================================================

struct demon_hunter_spell_t: public demon_hunter_action_t < spell_t >
{
  demon_hunter_spell_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

namespace spells
{

// Blur =====================================================================

struct blur_t : public demon_hunter_spell_t
{
  blur_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_spell_t( "blur", p, p -> spec.blur )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> buff.blur -> trigger();
  }
};

// Chaos Blades =============================================================

struct chaos_blades_t : public demon_hunter_spell_t
{
  chaos_blades_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_spell_t( "chaos_blades", p, p -> talent.chaos_blades )
  {
    parse_options( options_str );

    harmful = may_miss = may_crit = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> buff.chaos_blades -> trigger();
  }
};

// Consume Magic ============================================================

struct consume_magic_t : public demon_hunter_spell_t
{
  consume_magic_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_spell_t( "consume_magic", p, p -> spec.consume_magic )
  {
    parse_options( options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( p() -> specialization() == DEMON_HUNTER_HAVOC )
    {
      p() -> resource_gain( RESOURCE_FURY, p() -> resources.max[ RESOURCE_FURY ], action_gain );
    }
  }

  bool ready() override
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return demon_hunter_spell_t::ready();
  }
};

// Nemesis ==================================================================

struct nemesis_t : public demon_hunter_spell_t
{
  nemesis_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_spell_t( "nemesis", p, p -> talent.nemesis )
  {
    parse_options( options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    td( target ) -> debuffs.nemesis -> trigger();
  }
};

} // end namespace spells

// ==========================================================================
// Demon Hunter attacks
// ==========================================================================

struct demon_hunter_attack_t: public demon_hunter_action_t < melee_attack_t >
{
  bool demonic_presence, chaos_blades;

  demon_hunter_attack_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ),
    demonic_presence( data().affected_by( p -> mastery_spell.demonic_presence -> effectN( 1 ) ) ),
    chaos_blades( data().affected_by( p -> talent.chaos_blades -> effectN( 2 ) ) )
  {
    special = true;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( demonic_presence )
    {
      am *= 1.0 + p() -> cache.mastery_value();
    }

    if ( chaos_blades && p() -> buff.chaos_blades -> check() )
    {
      am *= 1.0 + p() -> cache.mastery_value();
    }

    return am;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double tm = base_t::composite_target_multiplier( t );

    tm *= 1.0 + p() -> get_target_data( t ) -> debuffs.nemesis -> value();

    return tm;
  }
  
  virtual void execute() override
  {
    base_t::execute();

    if ( ! result_is_hit( execute_state -> result ) && resource_consumed > 0 )
    {
      trigger_fury_refund();
    }
  }

  void trigger_fury_refund()
  {
    player -> resource_gain( RESOURCE_FURY, resource_consumed * 0.80, p() -> gain.fury_refund );
  }

  virtual bool usable_moving() const override
  {
    if ( execute_time() > timespan_t::zero() )
      return false;

    if ( channeled )
      return false;

    if ( p() -> buff.vengeful_retreat_jump_cancel -> check() )
      return true;

    return base_t::usable_moving();
  }
};

namespace attacks
{
  
// Melee Attack =============================================================

struct melee_t : public demon_hunter_attack_t
{
  struct demon_blade_t : public demon_hunter_attack_t
  {
    struct {
      double base, range;
    } fury;

    // FIXME: Need spell data to replace hardcoded values.
    demon_blade_t( demon_hunter_t* p ) :
      demon_hunter_attack_t( "demon_blade", p, /* p -> find_spell( 203796 ) */ spell_data_t::nil() )
    {
      cooldown -> duration = timespan_t::from_millis( 200 ); // Rough value from testing.

      weapon_multiplier = 1.0;
      normalize_weapon_speed = true;
      weapon = &( p -> main_hand_weapon );
      school = SCHOOL_SHADOW;

      fury.base  = 20;
      fury.range = 10;
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        p() -> resource_gain( RESOURCE_FURY, fury.base + rng().real() * fury.range, action_gain );
      }

      if ( p() -> talent.felblade -> ok() && p() -> rppm.felblade.trigger() )
      {
        p() -> proc.felblade_reset -> occur();
        p() -> cooldown.felblade -> reset( true );
      }
    }
  };

  enum status_e {
    MELEE_CONTACT,
    LOST_CONTACT_CHANNEL,
    LOST_CONTACT_RANGE,
  };

  struct {
    status_e main_hand, off_hand;
  } status;

  demon_blade_t* demon_blade;

  melee_t( const std::string& name, demon_hunter_t* p ) :
    demon_hunter_attack_t( name, p, spell_data_t::nil() )
  {
    school = SCHOOL_PHYSICAL;
    special = false;
    background = repeating = auto_attack = may_glance = true;
    trigger_gcd = timespan_t::zero();

    status.main_hand = status.off_hand = LOST_CONTACT_RANGE;

    if ( p -> dual_wield() )
      base_hit -= 0.19;

    if ( p -> talent.demon_blades -> ok() )
    {
      demon_blade = new demon_blade_t( p );
      add_child( demon_blade );
    }
  }

  void reset() override
  {
    demon_hunter_attack_t::reset();
    
    status.main_hand = status.off_hand = LOST_CONTACT_RANGE;
  }

  timespan_t execute_time() const override
  {
    status_e s = weapon -> slot == SLOT_MAIN_HAND ? status.main_hand : status.off_hand;

    switch( s )
    {
    /* Autoattacks after a channel occur around 350ms later. channel_lag
       takes care of the first 250, but add an extra 100 here. */
    case LOST_CONTACT_CHANNEL:
      return timespan_t::from_millis( 100 );
    // Position lag stuff
    case LOST_CONTACT_RANGE:
      return rng().gauss( timespan_t::from_millis( 250 ), timespan_t::from_millis( 62 ) );
    default:
      return demon_hunter_attack_t::execute_time();
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talent.demon_blades -> ok() && ! demon_blade -> cooldown -> down() )
      {
        if ( p() -> in_gcd() )
        {
          p() -> proc.demon_blade_supp -> occur();
        }
        else
        {
          demon_blade -> target = target;
          demon_blade -> execute();
        }
      }
    }
  }

  void schedule_execute( action_state_t* s ) override
  {
    demon_hunter_attack_t::schedule_execute( s );

    if ( weapon -> slot == SLOT_MAIN_HAND )
      status.main_hand = MELEE_CONTACT;
    else if ( weapon -> slot == SLOT_OFF_HAND )
      status.off_hand = MELEE_CONTACT;
  }

  void execute() override
  {
    if ( p() -> current.distance_to_move > 5 || p() -> channeling
      || ( p() -> buffs.self_movement -> check() && ! p() -> buff.vengeful_retreat_jump_cancel -> check() ) )
    {
      status_e s;

      // Cancel autoattacks, auto_attack_t will restart them when we're able to attack again.
      if ( p() -> channeling )
      {
        p() -> proc.delayed_aa_channel -> occur();
        s = LOST_CONTACT_CHANNEL;
      }
      else
      {
        p() -> proc.delayed_aa_range -> occur();
        s = LOST_CONTACT_RANGE;
      }

      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        status.main_hand = s;
        player -> main_hand_attack -> cancel();
      }
      else
      {
        status.off_hand = s;
        player -> off_hand_attack -> cancel();
      }
      return;
    }

    demon_hunter_attack_t::execute();
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public demon_hunter_attack_t
{
  auto_attack_t( demon_hunter_t* p, const std::string& options_str ):
    demon_hunter_attack_t( "auto_attack", p )
  {
    parse_options( options_str );

    ignore_false_positive = true;
    range = 5;
    trigger_gcd = timespan_t::zero();

    p -> melee_main_hand = new melee_t( "auto_attack_mh", p );
    p -> main_hand_attack = p -> melee_main_hand;
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    p -> melee_off_hand = new melee_t( "auto_attack_oh", p );
    p -> off_hand_attack = p -> melee_off_hand;
    p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
    p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    p -> off_hand_attack -> id = 1;
  }

  void execute() override
  {
    if ( p() -> main_hand_attack -> execute_event == nullptr )
    {
      p() -> main_hand_attack -> schedule_execute();
    }

    if ( p() -> off_hand_attack -> execute_event == nullptr )
    {
      p() -> off_hand_attack -> schedule_execute();
    }
  }

  bool ready() override
  {
    bool ready = demon_hunter_attack_t::ready();

    if ( ready ) // Range check
    {
      if ( p() -> main_hand_attack -> execute_event == nullptr )
        return ready;

      if ( p() -> off_hand_attack -> execute_event == nullptr )
        return ready;
    }

    return false;
  }
};

// Blade Dance =============================================================

struct blade_dance_attack_t: public demon_hunter_attack_t
{
  blade_dance_attack_t( demon_hunter_t* p, const spell_data_t* s, const std::string& name ):
    demon_hunter_attack_t( name, p, s )
  {
    dual = background = true;
    aoe = -1;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = demon_hunter_attack_t::composite_da_multiplier( s );

    if ( p() -> talent.first_blood -> ok() && s -> chain_target == 0 )
    {
      dm *= 1.0 + p() -> talent.first_blood -> effectN( 1 ).percent();
    }

    return dm;
  }
};

struct blade_dance_event_t: public event_t
{
  demon_hunter_t* dh;
  size_t current;
  bool in_metamorphosis;
  const spell_data_t* timing_passive;
  std::vector<attack_t*>* attacks;

  blade_dance_event_t( demon_hunter_t* p, size_t ca, bool meta = false ):
    event_t( *p -> sim ), dh( p ), current( ca ), in_metamorphosis( meta )
  {
    timing_passive = in_metamorphosis ? p -> spec.death_sweep : p -> spec.blade_dance;
    attacks = in_metamorphosis ? &p -> death_sweep_attacks : &p -> blade_dance_attacks;

    add_event( next_execute() );
  }

  const char* name() const override
  { return "Blade Dance"; }

  timespan_t next_execute() const
  { return timespan_t::from_millis( timing_passive -> effectN( current + 5 ).misc_value1() - ( current > 0 ? timing_passive -> effectN( current + 4 ).misc_value1() : 0 ) ); }

  void execute() override
  {
    attacks -> at( current ) -> execute();
    current++;

    if ( current < attacks -> size() )
    {
      dh -> blade_dance_driver = new ( sim() ) blade_dance_event_t( dh, current, in_metamorphosis );
    }
  }
};

struct blade_dance_template_t: public demon_hunter_attack_t
{
  std::vector<attack_t*> attacks;
  buff_t* dodge_buff;

  blade_dance_template_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s ):
    demon_hunter_attack_t( n, p, s )
  {
    aoe = -1;

    for ( size_t i = 0; i < attacks.size(); i++ )
    {
      add_child( attacks[ i ] );
    }
  }

  bool init_finished() override
  {
    bool f = demon_hunter_attack_t::init_finished();
    
    for ( size_t i = 0; i < attacks.size(); i++ )
      attacks[ i ] -> stats = stats;

    return f;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = demon_hunter_attack_t::composite_da_multiplier( s );

    if ( p() -> talent.first_blood -> ok() && s -> chain_target == 0 )
    {
      dm *= 1.0 + p() -> talent.first_blood -> effectN( 1 ).percent();
    }

    return dm;
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    p() -> blade_dance_driver = new ( *sim ) blade_dance_event_t( p(), 0, p() -> buff.metamorphosis -> up() );
    dodge_buff -> trigger();
  }
};

struct blade_dance_t: public blade_dance_template_t
{
  blade_dance_t( demon_hunter_t* p, const std::string& options_str ):
    blade_dance_template_t( "blade_dance", p, p -> spec.blade_dance )
  {
    parse_options( options_str );

    attacks = p -> blade_dance_attacks;
    dodge_buff = p -> buff.blade_dance;
  }

  bool ready() override
  {
    if ( p() -> buff.metamorphosis -> check() )
      return false;

    return blade_dance_template_t::ready();
  }
};

// Chaos Blades ============================================================

struct chaos_blade_t : public demon_hunter_attack_t
{
  chaos_blade_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s, weapon_t* w ) :
    demon_hunter_attack_t( n, p, s )
  {
    special = may_glance = false; // TOCHECK may_glance
    repeating = background = true;
    base_execute_time = w -> swing_time;
  }
};

// Chaos Nova ===============================================================

struct chaos_nova_t : public demon_hunter_attack_t
{
  chaos_nova_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "chaos_nova", p, p -> spec.chaos_nova )
  {
    parse_options( options_str );

    cooldown -> duration -= p -> talent.unleashed_power -> effectN( 1 ).time_value();
    base_costs[ RESOURCE_FURY ] *= 1.0 + p -> talent.unleashed_power -> effectN( 2 ).percent();

    aoe = -1;
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( p() -> talent.demonic -> ok() )
    {
      p() -> buff.metamorphosis -> trigger( 1, p() -> buff.metamorphosis -> default_value, -1.0, timespan_t::from_seconds( 5.0 ) + gcd() );
    }
  }
};

// Chaos Strike =============================================================

struct chaos_strike_template_t : public demon_hunter_attack_t
{
  struct chaos_strike_oh_t : public demon_hunter_attack_t
  {
    chaos_strike_template_t* parent;

    chaos_strike_oh_t( demon_hunter_t* p, const spell_data_t* s, chaos_strike_template_t* mh ) :
      demon_hunter_attack_t( mh -> name_str + "_oh", p, s ),
      parent( mh )
    {
      dual = background = true;
      may_miss = may_parry = may_dodge = false;
      weapon = &( p -> off_hand_weapon );

      if ( p -> talent.chaos_cleave -> ok() )
        aoe = 1 + p -> talent.chaos_cleave -> effectN( 1 ).base_value(); // Bugged as of build 21287
    }

    // Use crit roll of the primary hit.
    double composite_crit() const override
    { return parent -> is_critical; }

    void execute() override
    {
      if ( parent -> is_critical )
      {
        p() -> resource_gain( RESOURCE_FURY, parent -> base_costs[ RESOURCE_FURY ], parent -> action_gain );
      }

      demon_hunter_attack_t::execute();
    }
  };

  struct chaos_strike_event_t: public event_t
  {
    chaos_strike_oh_t* off_hand;

    chaos_strike_event_t( demon_hunter_t* p, chaos_strike_oh_t* oh, timespan_t delay ) :
      event_t( *p -> sim ), off_hand( oh )
    {
      add_event( delay );
    }

    const char* name() const override
    { return "Chaos Strike"; }

    void execute() override
    { off_hand -> execute(); }
  };

  chaos_strike_oh_t* off_hand;
  bool is_critical;

  chaos_strike_template_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s ) :
    demon_hunter_attack_t( n, p, s )
  {
    off_hand = new chaos_strike_oh_t( p, data().effectN( 3 ).trigger(), this );
    add_child( off_hand );
  }

  bool init_finished() override
  {
    bool f = demon_hunter_attack_t::init_finished();

    // Use 1 stats object for both actions.
    off_hand -> stats = stats;

    return f;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      is_critical = true;
  }

  void execute() override
  {
    is_critical = false;

    demon_hunter_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      new ( *sim ) chaos_strike_event_t( p(), off_hand, timespan_t::from_millis( data().effectN( 3 ).misc_value1() ) );

      // TODO: Travel time
      if ( p() -> talent.demonic_appetite -> ok() && ! p() -> cooldown.demonic_appetite -> down() &&
        p() -> rng().roll( p() -> talent.demonic_appetite -> proc_chance() ) )
      {
        p() -> cooldown.demonic_appetite -> start();
        p() -> proc.demonic_appetite -> occur();
        p() -> resource_gain( RESOURCE_FURY, 30, p() -> gain.demonic_appetite ); // FIXME
      }
    }
  }
};

struct chaos_strike_t : public chaos_strike_template_t
{
  chaos_strike_t( demon_hunter_t* p, const std::string& options_str ) :
    chaos_strike_template_t( "chaos_strike", p, p -> spec.chaos_strike )
  {
    parse_options( options_str );

    aoe += p -> talent.chaos_cleave -> effectN( 1 ).base_value();
  }

  bool ready() override
  {
    if ( p() -> buff.metamorphosis -> check() )
      return false;

    return chaos_strike_template_t::ready();
  }
};

// Annihilation =============================================================
// TOCHECK: Refund mechanics on multiple targets.

struct annihilation_t : public chaos_strike_template_t
{
  annihilation_t( demon_hunter_t* p, const std::string& options_str ) :
    chaos_strike_template_t( "annihilation", p, p -> spec.annihilation )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    if ( ! p() -> buff.metamorphosis -> check() )
      return false;

    return chaos_strike_template_t::ready();
  }
};

// Death Sweep ==============================================================

struct death_sweep_t: public blade_dance_template_t
{
  death_sweep_t( demon_hunter_t* p, const std::string& options_str ):
    blade_dance_template_t( "death_sweep", p, p -> spec.death_sweep )
  {
    parse_options( options_str );

    attacks = p -> death_sweep_attacks;
    dodge_buff = p -> buff.death_sweep;
  }

  bool ready() override
  {
    if ( ! p() -> buff.metamorphosis -> check() )
      return false;

    return blade_dance_template_t::ready();
  }
};

// Demon's Bite =============================================================

struct demons_bite_t : public demon_hunter_attack_t
{
  struct {
    double base, range;
  } fury;

  demons_bite_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "demons_bite", p, p -> find_class_spell( "Demon's Bite" ) )
  {
    parse_options( options_str );

    fury.base  = data().effectN( 3 ).resource( RESOURCE_FURY ) + 1.0;
    fury.range = data().effectN( 3 ).die_sides() - 1.0;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_FURY, fury.base + rng().real() * fury.range, action_gain );
    }

    if ( p() -> talent.felblade -> ok() && p() -> rppm.felblade.trigger() )
    {
      p() -> proc.felblade_reset -> occur();
      p() -> cooldown.felblade -> reset( true );
    }
  }

  bool ready() override
  {
    if ( p() -> talent.demon_blades -> ok() )
    {
      return false;
    }

    return demon_hunter_attack_t::ready();
  }
};

// Eye Beam =================================================================
// FIXME: Eye Beam stats show +1 hits/execute.

struct eye_beam_t : public demon_hunter_attack_t
{
  // TOCHECK: Direct or tick?
  struct eye_beam_tick_t : public demon_hunter_attack_t
  {
    eye_beam_tick_t( demon_hunter_t* p ) :
      demon_hunter_attack_t( "eye_beam_tick", p, p -> find_spell( 198030 ) )
    {
      aoe = -1;
      dual = background = true;
    }
  };

  eye_beam_tick_t* beam;

  eye_beam_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "eye_beam", p, p -> find_class_spell( "Eye Beam" ) )
  {
    parse_options( options_str );
    
    channeled = true;
    beam = new eye_beam_tick_t( p );
    beam -> stats = stats;

    dot_duration *= 1.0 + p -> talent.blind_fury -> effectN( 1 ).percent();
  }

  // Channel is not hasted.
  timespan_t tick_time( double ) const override
  { return base_tick_time; }

  void tick( dot_t* d ) override
  {
    demon_hunter_attack_t::tick( d );

    // Until 400ms through the channel, it's just animation and doesn't actually deal damage.
    if ( d -> current_tick >= 2 )
    {
      beam -> target = target;
      beam -> execute();
    }
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( p() -> talent.demonic -> ok() )
    {
      p() -> buff.metamorphosis -> trigger( 1, p() -> buff.metamorphosis -> default_value, -1.0, timespan_t::from_seconds( 5.0 ) + execute_time() );
    }
  }
};

// Felblade =============================================================
// TODO: Real movement stuff.

struct felblade_t : public demon_hunter_attack_t
{
  const spell_data_t* damage_spell;

  felblade_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "felblade", p, p -> talent.felblade )
  {
    parse_options( options_str );

    damage_spell = p -> find_spell( 213243 );
    school = damage_spell -> get_school_type();
    normalize_weapon_speed = true;
    weapon_multiplier = damage_spell -> effectN( 2 ).percent();
    weapon = &( p -> main_hand_weapon );

    // Special handling for Chaos Blades damage modifier.
    chaos_blades = damage_spell -> affected_by( p -> talent.chaos_blades -> effectN( 2 ) );

    movement_directionality = MOVEMENT_TOWARDS;
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> specialization() == DEMON_HUNTER_HAVOC )
      {
        p() -> resource_gain( RESOURCE_FURY, damage_spell -> effectN( 4 ).resource( RESOURCE_FURY ), action_gain );
      }
    }
  }
};

// Fel Eruption =============================================================

struct fel_eruption_t : public demon_hunter_attack_t
{
  fel_eruption_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "fel_eruption", p, p -> talent.fel_eruption )
  {
    parse_options( options_str );

    resource_current = RESOURCE_FURY; // FIXME: Have to force it to use Fury for now.
  }
};

// Fel Rush =================================================================

struct fel_rush_t : public demon_hunter_attack_t
{
  fel_rush_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "fel_rush", p, p -> find_class_spell( "Fel Rush" ) )
  {
    parse_options( options_str );

    cooldown -> charges  = data().charges();
    cooldown -> duration = data().charge_cooldown();

    const spell_data_t* damage_spell = p -> find_spell( 192611 );
    school = damage_spell -> get_school_type();
    attack_power_mod.direct = damage_spell -> effectN( 1 ).ap_coeff();
    aoe = -1;

    // Special handling for Chaos Blades damage modifier.
    chaos_blades = damage_spell -> affected_by( p -> talent.chaos_blades -> effectN( 2 ) );
    
    base_teleport_distance = damage_spell -> effectN( 1 ).radius();
    movement_directionality = MOVEMENT_OMNI;
    ignore_false_positive = true;
    
    base_crit += p -> talent.fel_mastery -> effectN( 2 ).percent();
  }

  // Fel Rush's loss of control causes a GCD lag after the loss ends.
  timespan_t gcd() const override
  { return data().gcd() + rng().gauss( sim -> gcd_lag, sim -> gcd_lag_stddev ); }

  void execute() override
  {
    double dtm = p() -> current.distance_to_move;

    demon_hunter_attack_t::execute();
    
    // Buff to track the rush's movement. This lets us delay autoattacks.
    p() -> buffs.self_movement -> trigger( 1, 0, -1.0, data().gcd() );

    p() -> buff.momentum -> trigger();

    // If we're not moving to cover distance, let's assume we're Fel Rushing for damage.
    if ( dtm == 0.0 )
    {
      p() -> current.distance = std::abs( p() -> current.distance - composite_teleport_distance( execute_state ) );

      // If new distance after rushing is too far away to melee from, then trigger movement back into melee range.
      if ( p() -> current.distance > 5.0 )
      {
        p() -> trigger_movement( p() -> current.distance - 5.0, MOVEMENT_TOWARDS );
      }
    }

    if ( p() -> talent.fel_mastery -> ok() && result_is_hit( execute_state -> result ) )
    {
      p() -> resource_gain( RESOURCE_FURY, p() -> talent.fel_mastery -> effectN( 1 ).resource( RESOURCE_FURY ), action_gain );
    }
  }

  bool ready() override
  {
    if ( p() -> buffs.self_movement -> check() )
      return false;

    return demon_hunter_attack_t::ready();
  }
};

// Metamorphosis ============================================================

struct metamorphosis_t : public demon_hunter_attack_t
{
  metamorphosis_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "metamorphosis", p, p -> find_class_spell( "Metamorphosis" ) )
  {
    parse_options( options_str );

    const spell_data_t* damage_spell = p -> find_spell( 200166 );
    school                  = damage_spell -> get_school_type();
    attack_power_mod.direct = damage_spell -> effectN( 2 ).ap_coeff();
    radius                  = damage_spell -> effectN( 2 ).radius();
    aoe                     = -1;

    // Special handling for Chaos Blades damage modifier.
    chaos_blades = damage_spell -> affected_by( p -> talent.chaos_blades -> effectN( 2 ) );
    
    base_teleport_distance  = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
  }

  // leap travel time, indepedent of distance
  timespan_t travel_time() const override
  { return timespan_t::from_seconds( 1.0 ); }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    // Buff is gained at the start of the leap.
    p() -> buff.metamorphosis -> trigger();

    if ( p() -> talent.demon_reborn -> ok() )
    {
      p() -> cooldown.blade_dance          -> reset( false );
      p() -> cooldown.blur                 -> reset( false );
      p() -> cooldown.chaos_blades         -> reset( false );
      p() -> cooldown.chaos_nova           -> reset( false );
      p() -> cooldown.consume_magic        -> reset( false );
      p() -> cooldown.death_sweep          -> reset( false );
      p() -> cooldown.eye_beam             -> reset( false );
      p() -> cooldown.felblade             -> reset( false );
      p() -> cooldown.fel_barrage          -> reset( false );
      p() -> cooldown.fel_eruption         -> reset( false );
      p() -> cooldown.fel_rush             -> reset( false ); // TOCHECK
      p() -> cooldown.nemesis              -> reset( false );
      p() -> cooldown.netherwalk           -> reset( false );
      p() -> cooldown.fury_of_the_illidari -> reset( false );
      p() -> cooldown.throw_glaive         -> reset( false ); // TOCHECK
      p() -> cooldown.vengeful_retreat     -> reset( false );
    }
    
    // Buff to track the actual leap. This lets us delay autoattacks.
    p() -> buffs.self_movement -> trigger( 1, 0, -1.0, travel_time() ); // TOCHECK
  }
};

// Throw Glaive =============================================================

struct throw_glaive_t : public demon_hunter_attack_t
{
  struct bloodlet_t : public residual_action::residual_periodic_action_t<demon_hunter_attack_t>
  {
    bloodlet_t( demon_hunter_t* p ) :
      residual_action::residual_periodic_action_t<demon_hunter_attack_t>( "bloodlet", p, p -> find_spell( 207690 ) )
    {
      background = dual = proc = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  bloodlet_t* bloodlet;

  throw_glaive_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "throw_glaive", p, p -> find_class_spell( "Throw Glaive" ) )
  {
    parse_options( options_str );

    cooldown -> charges  = data().charges() + p -> talent.master_of_the_glaive -> effectN( 2 ).base_value();
    cooldown -> duration = data().charge_cooldown();

    aoe = 3; // Ricochets to 2 additional enemies
    radius = 10.0; // with 10 yards.

    if ( p -> talent.bloodlet -> ok() )
    {
      bloodlet = new bloodlet_t( p );
      add_child( bloodlet );
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( p() -> talent.bloodlet -> ok() && result_is_hit( s -> result ) )
    {
      residual_action::trigger( bloodlet, s -> target, 
        s -> result_amount * p() -> talent.bloodlet -> effectN( 1 ).percent() );
    }
  }
};

// Vengeful Retreat =================================================================

struct vengeful_retreat_t : public demon_hunter_attack_t
{
  bool jump_cancel;

  vengeful_retreat_t( demon_hunter_t* p, const std::string& options_str ) :
    demon_hunter_attack_t( "vengeful_retreat", p, p -> find_class_spell( "Vengeful Retreat" ) )
  {
    add_option( opt_bool( "jump_cancel", jump_cancel ) );
    parse_options( options_str );

    const spell_data_t* damage_spell = p -> find_spell( 198813 );
    school = damage_spell -> get_school_type();
    normalize_weapon_speed = damage_spell -> effectN( 2 ).type() == E_NORMALIZED_WEAPON_DMG;
    weapon_multiplier = damage_spell -> effectN( 3 ).percent();
    weapon = &( p -> main_hand_weapon );
    radius = damage_spell -> effectN( 3 ).radius();
    aoe = -1;

    // Special handling for Chaos Blades damage modifier.
    chaos_blades = damage_spell -> affected_by( p -> talent.chaos_blades -> effectN( 2 ) );
    
    if ( ! jump_cancel )
    {
      base_teleport_distance = 20.0;
      movement_directionality = MOVEMENT_OMNI;
    }
    ignore_false_positive = true;
    use_off_gcd = true;

    cooldown -> duration += p -> talent.prepared -> effectN( 2 ).time_value();
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    p() -> buff.prepared -> trigger();
    p() -> buff.momentum -> trigger();
    
    // Buff to track the movement. This lets us delay autoattacks and other things.
    if ( jump_cancel )
    {
      p() -> buff.vengeful_retreat_jump_cancel -> trigger();
      p() -> buffs.self_movement -> trigger( 1, 0, -1.0, p() -> buff.vengeful_retreat_jump_cancel -> buff_duration );
    }
    else
    {
      p() -> buffs.self_movement -> trigger( 1, 0, -1.0, timespan_t::from_seconds( 1.0 ) );
    }
  }

  bool ready() override
  {
    if ( p() -> buffs.self_movement -> check() )
      return false;
    // Don't let the player try to jump cancel while out of range.
    if ( jump_cancel && p() -> current.distance_to_move > 5 )
      return false;

    return demon_hunter_attack_t::ready();
  }
};

}  // end namespace attacks

}  // end namespace actions

namespace buffs
{
  
template <typename BuffBase>
struct demon_hunter_buff_t : public BuffBase
{
protected:
  typedef demon_hunter_buff_t base_t;
  demon_hunter_t& dh;

public:
  demon_hunter_buff_t( demon_hunter_t& p, const buff_creator_basics_t& params ) :
    BuffBase( params ), dh( p )
  {}

  demon_hunter_t& p() const { return dh; }
};

struct chaos_blades_t : public demon_hunter_buff_t<buff_t>
{
  chaos_blades_t( demon_hunter_t* p ) :
    demon_hunter_buff_t<buff_t>( *p, buff_creator_t( p, "chaos_blades", p -> talent.chaos_blades )
        .cd( timespan_t::zero() ) )
  {}

  void change_auto_attack( attack_t*& hand, attack_t* a )
  {
    if ( hand == 0 )
      return;

    bool executing = hand -> execute_event != 0;
    timespan_t time_to_hit = timespan_t::zero();

    if ( executing )
    {
      time_to_hit = hand -> execute_event -> occurs() - sim -> current_time();
      event_t::cancel( hand -> execute_event );
    }

    hand = a;

    // Kick off the new attack, by instantly scheduling and rescheduling it to
    // the remaining time to hit. We cannot use normal reschedule mechanism
    // here (i.e., simply use event_t::reschedule() and leave it be), because
    // the rescheduled event would be triggered before the full swing time
    // (of the new auto attack) in most cases.
    if ( executing )
    {
      timespan_t old_swing_time = hand -> base_execute_time;
      hand -> base_execute_time = timespan_t::zero();
      hand -> schedule_execute();
      hand -> base_execute_time = old_swing_time;
      hand -> execute_event -> reschedule( time_to_hit );
    }
  }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(), timespan_t duration = timespan_t::min() )
  {
    buff_t::execute( stacks, value, duration );

    change_auto_attack( p().main_hand_attack, p().chaos_blade_main_hand );
    if ( p().off_hand_attack )
      change_auto_attack( p().off_hand_attack, p().chaos_blade_off_hand );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    change_auto_attack( p().main_hand_attack, p().melee_main_hand );
    if ( p().off_hand_attack )
      change_auto_attack( p().off_hand_attack, p().melee_off_hand );
  }
};

// Nemesis ==================================================================

struct nemesis_buff_t : public demon_hunter_buff_t<buff_t>
{
  nemesis_buff_t( demon_hunter_t* p, player_t* target ) :
    demon_hunter_buff_t<buff_t>( *p, buff_creator_t( target, "nemesis", p -> talent.nemesis )
      .default_value( p -> talent.nemesis -> effectN( 1 ).percent() )
      .cd( timespan_t::zero() ) )
  {}

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    if ( remaining_duration > timespan_t::zero() )
    {
      p().buff.nemesis -> trigger( 1, player -> race, -1.0, remaining_duration );
    }
  }
};

}  // end namespace buffs

// ==========================================================================
// Targetdata Definitions
// ==========================================================================

demon_hunter_td_t::demon_hunter_td_t( player_t* target, demon_hunter_t& p )
  : actor_target_data_t( target, &p ), dots( dots_t() ), debuffs( debuffs_t() )
{
  debuffs.nemesis = new buffs::nemesis_buff_t( &p, target );
}

// ==========================================================================
// Demon Hunter Definitions
// ==========================================================================

demon_hunter_t::demon_hunter_t( sim_t* sim, const std::string& name, race_e r ) :
  player_t( sim, DEMON_HUNTER, name, r ),
  buff(),
  talent(),
  spec(),
  mastery_spell(),
  cooldown(),
  gain(),
  benefits(),
  proc(),
  active_spells(),
  pets(),
  options(),
  glyphs(),
  blade_dance_driver( nullptr ),
  blade_dance_attacks( 0 ),
  death_sweep_attacks( 0 ),
  melee_main_hand( nullptr ),
  melee_off_hand( nullptr ),
  chaos_blade_main_hand( nullptr ),
  chaos_blade_off_hand( nullptr )
{
  base.distance = 5.0;

  create_cooldowns();
  create_gains();
  create_benefits();

  regen_type = REGEN_DISABLED;
}

/* Construct cooldowns
 *
 */
void demon_hunter_t::create_cooldowns()
{
  cooldown.blade_dance          = get_cooldown( "blade_dance" );
  cooldown.blur                 = get_cooldown( "blur" );
  cooldown.chaos_blades         = get_cooldown( "chaos_blades" );
  cooldown.chaos_nova           = get_cooldown( "chaos_nova" );
  cooldown.consume_magic        = get_cooldown( "consume_magic" );
  cooldown.death_sweep          = get_cooldown( "death_sweep" );
  cooldown.demonic_appetite     = get_cooldown( "demonic_appetite" );
  cooldown.eye_beam             = get_cooldown( "eye_beam" );
  cooldown.fel_barrage          = get_cooldown( "fel_barrage" );
  cooldown.felblade             = get_cooldown( "felblade" );
  cooldown.fel_eruption         = get_cooldown( "fel_eruption" );
  cooldown.fel_rush             = get_cooldown( "fel_rush" );
  cooldown.fury_of_the_illidari = get_cooldown( "fury_of_the_illidari" );
  cooldown.nemesis              = get_cooldown( "nemesis" );
  cooldown.netherwalk           = get_cooldown( "netherwalk" );
  cooldown.throw_glaive         = get_cooldown( "throw_glaive" );
  cooldown.vengeful_retreat     = get_cooldown( "vengeful_retreat" );
}

/* Construct gains
 *
 */
void demon_hunter_t::create_gains()
{
  gain.fury_refund      = get_gain( "fury_refund" );
  gain.demonic_appetite = get_gain( "demonic_appetite" );
  gain.prepared         = get_gain( "prepared" );
}

/* Construct benefits
 *
 */
void demon_hunter_t::create_benefits()
{
}

/* Define the acting role of the Demon Hunter
 */
role_e demon_hunter_t::primary_role() const
{
  switch ( specialization() )
  {
  case DEMON_HUNTER_HAVOC:
    return ROLE_ATTACK;
  case DEMON_HUNTER_VENGEANCE:
    return ROLE_TANK;
  default:
    return ROLE_NONE;
  }
}

/**
 * @brief Convert hybrid stats
 *
 *  Converts hybrid stats that either morph based on spec or only work
 *  for certain specs into the appropriate "basic" stats
 */
stat_e demon_hunter_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT: 
  case STAT_STR_AGI:
    return STAT_AGILITY;
  case STAT_STR_INT:
    return STAT_NONE;
  case STAT_SPIRIT:
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
      return specialization() == DEMON_HUNTER_VENGEANCE ? s : STAT_NONE;  
  default: return s; 
  }
}

// demon_hunter_t::matching_gear_multiplier =================================

double demon_hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  // TODO: Find in spell data... somewhere.
  if ( attr == STAT_AGILITY && specialization() == DEMON_HUNTER_HAVOC )
    return 0.05;

  if ( attr == STAT_STAMINA && specialization() == DEMON_HUNTER_VENGEANCE ) // TOCHECK
    return 0.05;

  return 0.0;
}

// demon_hunter_t::composite_dodge ==========================================

double demon_hunter_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  d += buff.blade_dance -> check_value();
  d += buff.death_sweep -> check_value();
  
  d += buff.blur -> check() * buff.blur -> data().effectN( 2 ).percent();

  return d;
}

// demon_hunter_t::composite_player_multiplier =====================================

double demon_hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= 1.0 + buff.momentum -> check_value();

  // TODO: Figure out how to access target's race.
  m *= 1.0 + buff.nemesis -> check() * buff.nemesis -> data().effectN( 1 ).percent();

  return m;
}

// demon_hunter_t::passive_movement_modifier ================================

double demon_hunter_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( mastery_spell.demonic_presence -> ok() )
    ms += cache.mastery() * mastery_spell.demonic_presence -> effectN( 2 ).mastery_value();

  return ms;
}

// demon_hunter_t::create_action ============================================

action_t* demon_hunter_t::create_action( const std::string& name,
                                         const std::string& options_str )
{
  using namespace actions::spells;
  if ( name == "blur"             ) return new blur_t              ( this, options_str );
  if ( name == "consume_magic"    ) return new consume_magic_t     ( this, options_str );
  using namespace actions::attacks;
  if ( name == "auto_attack"      ) return new auto_attack_t       ( this, options_str );
  if ( name == "annihilation"     ) return new annihilation_t      ( this, options_str );
  if ( name == "blade_dance"      ) return new blade_dance_t       ( this, options_str );
  if ( name == "chaos_blades"     ) return new chaos_blades_t      ( this, options_str );
  if ( name == "chaos_nova"       ) return new chaos_nova_t        ( this, options_str );
  if ( name == "chaos_strike"     ) return new chaos_strike_t      ( this, options_str );
  if ( name == "death_sweep"      ) return new death_sweep_t       ( this, options_str );
  if ( name == "demons_bite"      ) return new demons_bite_t       ( this, options_str );
  if ( name == "eye_beam"         ) return new eye_beam_t          ( this, options_str );
  if ( name == "felblade"         ) return new felblade_t          ( this, options_str );
  if ( name == "fel_eruption"     ) return new fel_eruption_t      ( this, options_str );
  if ( name == "fel_rush"         ) return new fel_rush_t          ( this, options_str );
  if ( name == "metamorphosis" ||
       name == "meta"             ) return new metamorphosis_t     ( this, options_str );
  if ( name == "nemesis"          ) return new nemesis_t           ( this, options_str );
  if ( name == "throw_glaive"     ) return new throw_glaive_t      ( this, options_str );
  if ( name == "vengeful_retreat" ) return new vengeful_retreat_t  ( this, options_str );

  return base_t::create_action( name, options_str );
}

pet_t* demon_hunter_t::create_pet( const std::string& pet_name,
                                   const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  // Add pets here

  sim -> errorf( "No demon hunter pet with name '%s' available.",
               pet_name.c_str() );

  return nullptr;
}

// demon_hunter_t::create_pets ==============================================

void demon_hunter_t::create_pets()
{
  base_t::create_pets();
}

// demon_hunter_t::init ============================================================

void demon_hunter_t::init()
{
  player_t::init();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    sim -> errorf( "%s is using an unsupported spec.", name() );
  }
}

// demon_hunter_t::init_base_stats ==========================================

void demon_hunter_t::init_base_stats()
{
  base_t::init_base_stats();

  resources.base[ RESOURCE_FURY ] = 100;

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// demon_hunter_t::init_resources ===========================================

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.current[ RESOURCE_FURY ] = 0;
}

// demon_hunter_t::init_rng ========================================================

void demon_hunter_t::init_rng()
{
  // RPPM objects
  rppm.felblade = real_ppm_t( *this, find_spell( 203557 ) -> real_ppm(), 1.0, RPPM_HASTE );

  player_t::init_rng();
}

// demon_hunter_t::init_procs ===============================================

void demon_hunter_t::init_procs()
{
  base_t::init_procs();

  proc.delayed_aa_range   = get_proc( "delayed_swing__out_of_range" );
  proc.delayed_aa_channel = get_proc( "delayed_swing__channeling" );
  proc.demonic_appetite   = get_proc( "demonic_appetite" );
  proc.felblade_reset     = get_proc( "felblade_reset" );
  proc.demon_blade_supp   = get_proc( "demon_blade_suppressed" );
}

// demon_hunter_t::init_scaling =============================================

void demon_hunter_t::init_scaling()
{
  base_t::init_scaling();

  scales_with[ STAT_WEAPON_OFFHAND_DPS ] = true;

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
    scales_with[ STAT_BONUS_ARMOR ] = true;

  scales_with[ STAT_STRENGTH ] = false;
}

// demon_hunter_t::init_spells ==============================================

void demon_hunter_t::init_spells()
{
  base_t::init_spells();

  // General ================================================================

  spec.demon_hunter        = find_class_spell( "Demon Hunter" );

  spec.annihilation        = find_spell( 201427 );
  spec.blade_dance         = find_class_spell( "Blade Dance" );
  spec.blur                = find_class_spell( "Blur" );
  spec.chaos_nova          = find_class_spell( "Chaos Nova" );
  spec.chaos_strike        = find_class_spell( "Chaos Strike" );
  spec.consume_magic       = find_class_spell( "Consume Magic" );
  spec.death_sweep         = find_spell( 210152 );
  spec.demonic_instincts   = find_spell( 203569 ); // not a class spell
  spec.havoc               = find_specialization_spell( "Havoc Demon Hunter" );
  spec.metamorphosis_buff  = find_spell( 162264 );

  spec.vengeance           = find_specialization_spell( "Vengeance Demon Hunter" );

  // Masteries ==============================================================

  mastery_spell.demonic_presence = find_mastery_spell( DEMON_HUNTER_HAVOC );
  mastery_spell.fel_blood        = find_mastery_spell( DEMON_HUNTER_VENGEANCE );

  // Talents ================================================================

  talent.fel_mastery          = find_talent_spell( "Fel Mastery" );
  talent.demonic_appetite     = find_talent_spell( "Demonic Appetite" );
  talent.blind_fury           = find_talent_spell( "Blind Fury" );
    
  talent.prepared             = find_talent_spell( "Prepared" );
  talent.chaos_cleave         = find_talent_spell( "Chaos Cleave" );
  talent.fel_eruption         = find_talent_spell( "Fel Eruption" );

  talent.felblade             = find_talent_spell( "Felblade" );
  talent.first_blood          = find_talent_spell( "First Blood" );
  talent.bloodlet             = find_talent_spell( "Bloodlet" );

  talent.netherwalk           = find_talent_spell( "Netherwalk" );
  talent.desperate_instincts  = find_talent_spell( "Desperate Instincts" );
  talent.soul_rending         = find_talent_spell( "Soul Rending" );

  talent.momentum             = find_talent_spell( "Momentum" );
  talent.demonic              = find_talent_spell( "Demonic" );
  talent.nemesis              = find_talent_spell( "Nemesis" );

  talent.master_of_the_glaive = find_talent_spell( "Master of the Glaive" );
  talent.unleashed_power      = find_talent_spell( "Unleashed Power" );
  talent.demon_blades         = find_talent_spell( "Demon Blades" );

  talent.chaos_blades         = find_talent_spell( "Chaos Blades" );
  talent.fel_barrage          = find_talent_spell( "Fel Barrage (NYI)" );
  talent.demon_reborn         = find_talent_spell( "Demon Reborn" );

  if ( talent.demonic_appetite -> ok() )
  {
    cooldown.demonic_appetite -> duration = talent.demonic_appetite -> internal_cooldown();
  }
  
  using namespace actions::attacks;

  if ( spec.blade_dance -> ok() )
  {
    blade_dance_attacks.push_back( new blade_dance_attack_t( this, spec.blade_dance -> effectN( 5 ).trigger(), "blade_dance2" ) );
    blade_dance_attacks.push_back( new blade_dance_attack_t( this, spec.blade_dance -> effectN( 6 ).trigger(), "blade_dance3" ) );
    blade_dance_attacks.push_back( new blade_dance_attack_t( this, spec.blade_dance -> effectN( 7 ).trigger(), "blade_dance4" ) );
  }

  if ( spec.death_sweep -> ok() )
  {
    death_sweep_attacks.push_back( new blade_dance_attack_t( this, spec.death_sweep -> effectN( 5 ).trigger(), "death_sweep2" ) );
    death_sweep_attacks.push_back( new blade_dance_attack_t( this, spec.death_sweep -> effectN( 6 ).trigger(), "death_sweep3" ) );
    death_sweep_attacks.push_back( new blade_dance_attack_t( this, spec.death_sweep -> effectN( 7 ).trigger(), "death_sweep4" ) );
  }
  
  if ( talent.chaos_blades -> ok() )
  {
    chaos_blade_main_hand = new chaos_blade_t( "chaos_blade_mh",
      this, find_spell( talent.chaos_blades -> effectN( 1 ).misc_value1() ), &( main_hand_weapon ) );

    chaos_blade_off_hand = new chaos_blade_t( "chaos_blade_oh",
      this, talent.chaos_blades -> effectN( 1 ).trigger(), &( off_hand_weapon ) );
  }
}

// demon_hunter_t::create_buffs =============================================

void demon_hunter_t::create_buffs()
{
  base_t::create_buffs();

  using namespace buffs;

  // General

  buff.blade_dance   = buff_creator_t( this, "blade_dance", spec.blade_dance )
                       .default_value( spec.blade_dance -> effectN( 3 ).percent() )
                       .add_invalidate( CACHE_DODGE );

  buff.blur          = buff_creator_t( this, "blur", spec.blur -> effectN( 1 ).trigger() )
                       .default_value( spec.blur -> effectN( 1 ).trigger() -> effectN( 3 ).percent() )
                       .cd( timespan_t::zero() );

  buff.death_sweep   = buff_creator_t( this, "death_sweep", spec.death_sweep )
                       .default_value( spec.death_sweep -> effectN( 3 ).percent() )
                       .add_invalidate( CACHE_DODGE );

  buff.metamorphosis = buff_creator_t( this, "metamorphosis", spec.metamorphosis_buff )
                       .cd( timespan_t::zero() );

  buff.momentum      = buff_creator_t( this, "momentum", find_spell( 208628 ) )
                       .default_value( find_spell( 208628 ) -> effectN( 1 ).percent() )
                       .chance( talent.momentum -> ok() )
                       .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // TODO: Buffs for each race?
  buff.nemesis       = buff_creator_t( this, "nemesis", find_spell( 208605 ) )
                       .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.prepared      = buff_creator_t( this, "prepared", talent.prepared -> effectN( 1 ).trigger() )
                       .chance( talent.prepared -> ok() )
                       .tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
                         resource_gain( RESOURCE_FURY, b -> data().effectN( 1 ).resource( RESOURCE_FURY ),
                         gain.prepared ); } );

  buff.chaos_blades  = new chaos_blades_t( this );

  buff.vengeful_retreat_jump_cancel = buff_creator_t( this, "vengeful_retreat_jump_cancel", spell_data_t::nil() )
                       .chance( 1.0 )
                       .duration( timespan_t::from_seconds( 1.25 ) );

  // Havoc

  // Vengeance
}

// ALL Spec Pre-Combat Action Priority List

void demon_hunter_t::apl_precombat()
{
  //action_priority_list_t* precombat = get_action_priority_list( "precombat" );
}

bool demon_hunter_t::has_t18_class_trinket() const
{
  return false;
}

// NO Spec Combat Action Priority List

void demon_hunter_t::apl_default()
{
  //action_priority_list_t* def = get_action_priority_list( "default" );
}

// Havoc Combat Action Priority List

void demon_hunter_t::apl_havoc()
{
  //action_priority_list_t* def = get_action_priority_list( "default" );
}

// Vengeance Combat Action Priority List

void demon_hunter_t::apl_vengeance()
{
  //action_priority_list_t* def = get_action_priority_list( "default" );
}

/* Always returns non-null targetdata pointer
 */
demon_hunter_td_t* demon_hunter_t::get_target_data( player_t* target ) const
{
  auto& td = _target_data[ target ];
  if ( !td )
  {
    td = new demon_hunter_td_t( target, const_cast<demon_hunter_t&>( *this ) );
  }
  return td;
}

/* Returns targetdata if found, nullptr otherwise
 */
demon_hunter_td_t* demon_hunter_t::find_target_data( player_t* target ) const
{
  return _target_data[ target ];
}

void demon_hunter_t::init_action_list()
{
  // FIXME
  /* if ( main_hand_weapon.type == WEAPON_NONE || off_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
    {
      sim -> errorf( "Player %s does not have a valid main-hand and off-hand weapon.", name() );
    }
    quiet = true;
    return;
  } */

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat(); // PRE-COMBAT

  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      apl_havoc();
      break;
    case DEMON_HUNTER_VENGEANCE:
      apl_vengeance();
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

void demon_hunter_t::reset()
{
  base_t::reset();

  blade_dance_driver = nullptr;
  rppm.felblade.reset();
}

void demon_hunter_t::interrupt()
{
  if ( blade_dance_driver )
  {
    event_t::cancel( blade_dance_driver );
  }

  base_t::interrupt();
}

void demon_hunter_t::target_mitigation( school_e school, dmg_e dt,
                                        action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  s -> result_amount *= 1.0 + buff.blur -> value();
}

// demon_hunter_t::invalidate_cache =========================================

void demon_hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery_spell.demonic_presence -> ok() )
    {
      player_t::invalidate_cache( CACHE_RUN_SPEED );
    }
    break;
  default:
    break;
  }
}

void demon_hunter_t::create_options()
{
  base_t::create_options();
}

std::string demon_hunter_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  // Log all options here

  return profile_str;
}

void demon_hunter_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  auto source_p = debug_cast<demon_hunter_t*>( source );

  options = source_p -> options;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class demon_hunter_report_t : public player_report_extension_t
{
public:
  demon_hunter_report_t( demon_hunter_t& player ) : p( player )
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
  demon_hunter_t& p;
};

// MODULE INTERFACE ==================================================

class demon_hunter_module_t : public module_t
{
public:
  demon_hunter_module_t() : module_t( DEMON_HUNTER )
  {
  }

  player_t* create_player( sim_t* sim, const std::string& name,
                           race_e r = RACE_NONE ) const override
  {
    auto p              = new demon_hunter_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>(
        new demon_hunter_report_t( *p ) );
    return p;
  }

  bool valid() const override { return true; }

  void init( player_t* ) const override {}
  void static_init() const override {}

  void register_hotfixes() const override {}

  void combat_begin( sim_t* ) const override {}

  void combat_end( sim_t* ) const override {}
};

}  // UNNAMED NAMESPACE

const module_t* module_t::demon_hunter()
{
  static demon_hunter_module_t m;
  return &m;
}
