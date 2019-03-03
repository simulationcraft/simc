// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Demon Hunter
// ==========================================================================

/* ==========================================================================
// Battle for Azeroth To-Do
// ==========================================================================

  * Possibly add per-race Nemesis buffs, if any fight styles are configured in a way it matters
  * Add option for Greater Soul spawns on add demise (% chance?) simulating adds in M+/dungeon style situations
  * Azerite Traits
  ** Implement Vengeance traits
  ** Test all Havoc traits when available

*/

// Forward Declarations
class demon_hunter_t;
struct soul_fragment_t;

namespace buffs
{}
namespace actions
{
  struct demon_hunter_attack_t;
  namespace attacks
  {
    struct auto_attack_damage_t;
  }
}
namespace items
{
}

// Target Data
class demon_hunter_td_t : public actor_target_data_t
{
public:
  struct dots_t
  {
    // Havoc
    dot_t* trail_of_ruin;

    // Vengeance
    dot_t* fiery_brand;
    dot_t* sigil_of_flame;
  } dots;

  struct debuffs_t
  {
    // Havoc
    buff_t* nemesis;
    buff_t* dark_slash;

    // Vengeance
    buff_t* frailty;
    buff_t* void_reaver;
  } debuffs;

  demon_hunter_td_t( player_t* target, demon_hunter_t& p );

  demon_hunter_t& dh()
  {
    return *debug_cast<demon_hunter_t*>( source );
  }
  const demon_hunter_t& dh() const
  {
    return *debug_cast<demon_hunter_t*>( source );
  }

  void target_demise();
};

constexpr unsigned MAX_SOUL_FRAGMENTS = 5;
constexpr timespan_t DEMONIC_EXTEND_DURATION = timespan_t::from_seconds(8);
//constexpr timespan_t MAX_FIERY_BRAND_DURATION = timespan_t::from_seconds(10);
constexpr double VENGEFUL_RETREAT_DISTANCE = 20.0;

enum class soul_fragment
{
  ALL,
  GREATER,
  LESSER,
};

const char* get_soul_fragment_str( soul_fragment type )
{
  switch ( type )
  {
    case soul_fragment::ALL:
      return "soul fragment";
    case soul_fragment::GREATER:
      return "greater soul fragment";
    case soul_fragment::LESSER:
      return "lesser soul fragment";
    default:
      return "";
  }
}

struct movement_buff_t : public buff_t
{
  double yards_from_melee;
  double distance_moved;
  demon_hunter_t* dh;

  movement_buff_t( demon_hunter_t* p, const buff_creator_basics_t& b )
    : buff_t( b ), yards_from_melee( 0.0 ), distance_moved( 0.0 ), dh( p )
  {
  }

  bool trigger( int s = 1, double v = DEFAULT_VALUE(), double c = -1.0,
                timespan_t d = timespan_t::min() ) override;
};

typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;

struct counter_t
{
  const sim_t* sim;

  double value, interval;
  timespan_t last;

  counter_t( demon_hunter_t* p );

  void add( double val )
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim->current_iteration == 0 && sim->iterations > sim->threads && !sim->debug && !sim->log )
      return;

    value += val;
    if ( last > timespan_t::min() )
      interval += ( sim->current_time() - last ).total_seconds();
    last = sim->current_time();
  }

  void reset()
  {
    last = timespan_t::min();
  }

  double divisor() const
  {
    if ( !sim->debug && !sim->log && sim->iterations > sim->threads )
      return sim->iterations - sim->threads;
    else
      return std::min( sim->iterations, sim->threads );
  }

  double mean() const
  {
    return value / divisor();
  }

  double interval_mean() const
  {
    return interval / divisor();
  }

  void merge( const counter_t& other )
  {
    value += other.value;
    interval += other.interval;
  }
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

  // Data collection for cooldown waste
  std::vector<counter_t*> counters;
  auto_dispose<std::vector<data_t*>> cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*>> cd_waste_iter;

  // Autoattacks
  actions::attacks::auto_attack_damage_t* melee_main_hand;
  actions::attacks::auto_attack_damage_t* melee_off_hand;

  double metamorphosis_health;    // Vengeance temp health from meta;
  double expected_max_health;

  // Soul Fragments
  unsigned next_fragment_spawn;  // determines whether the next fragment spawn
                                 // on the left or right
  auto_dispose<std::vector<soul_fragment_t*>> soul_fragments;
  event_t* soul_fragment_pick_up;

  double spirit_bomb_accumulator;  // Spirit Bomb healing accumulator
  event_t* spirit_bomb_driver;

  // Override for target's hitbox size, relevant for Fel Rush and Vengeful
  // Retreat.
  double target_reach;
  event_t* exit_melee_event;  // Event to disable melee abilities mid-VR.
  double initial_fury;

  timespan_t sigil_delay; // The amount of time it takes for a sigil to activate.
  timespan_t sigil_of_flame_activates; // When sigil of flame will next activate.

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* demon_soul;
    buff_t* immolation_aura;
    buff_t* metamorphosis;

    // Havoc
    buff_t* blade_dance;
    buff_t* blind_fury;
    buff_t* blur;
    buff_t* death_sweep;
    buff_t* momentum;
    buff_t* nemesis;
    buff_t* out_of_range;
    buff_t* prepared;

    movement_buff_t* fel_rush_move;
    movement_buff_t* vengeful_retreat_move;

    // Vengeance
    buff_t* demon_spikes;
    absorb_buff_t* soul_barrier;

    // Azerite
    buff_t* furious_gaze;
    buff_t* revolving_blades;
    buff_t* seething_power;
    buff_t* thirsting_blades;
    buff_t* thirsting_blades_driver;
  } buff;

  // Talents
  struct talents_t
  {
    // Havoc
    const spell_data_t* blind_fury;
    const spell_data_t* demonic_appetite;
    const spell_data_t* felblade;

    const spell_data_t* insatiable_hunger;
    const spell_data_t* demon_blades;
    const spell_data_t* immolation_aura;

    const spell_data_t* trail_of_ruin;
    const spell_data_t* fel_mastery;
    const spell_data_t* fel_barrage;

    const spell_data_t* soul_rending;
    const spell_data_t* desperate_instincts;
    const spell_data_t* netherwalk;
    
    const spell_data_t* cycle_of_hatred;
    const spell_data_t* first_blood;
    const spell_data_t* dark_slash;

    const spell_data_t* unleashed_power;
    const spell_data_t* master_of_the_glaive;
    const spell_data_t* fel_eruption;

    const spell_data_t* demonic;
    const spell_data_t* momentum;
    const spell_data_t* nemesis;

    // Vengeance
    const spell_data_t* abyssal_strike;
    const spell_data_t* agonizing_flames;
    const spell_data_t* razor_spikes;

    const spell_data_t* feast_of_souls;
    const spell_data_t* fallout;
    const spell_data_t* burning_alive;

    const spell_data_t* flame_crash;
    const spell_data_t* charred_flesh;
    //                  felblade

    //                  soul_rending
    const spell_data_t* feed_the_demon;
    const spell_data_t* fracture;

    const spell_data_t* concentrated_sigils;
    const spell_data_t* sigil_of_chains;
    const spell_data_t* quickened_sigils;

    const spell_data_t* gluttony;
    const spell_data_t* fel_devastation;
    const spell_data_t* spirit_bomb;

    const spell_data_t* last_resort;        // NYI
    const spell_data_t* void_reaver;
    const spell_data_t* soul_barrier;
  } talent;

  // Specialization Spells
  struct specializations_t
  {
    // General
    const spell_data_t* demon_hunter;
    const spell_data_t* chaos_brand;
    const spell_data_t* consume_magic;
    const spell_data_t* consume_soul_greater;
    const spell_data_t* consume_soul_lesser;
    const spell_data_t* critical_strikes;
    const spell_data_t* demonic_wards;
    const spell_data_t* disrupt;
    const spell_data_t* leather_specialization;
    const spell_data_t* metamorphosis;
    const spell_data_t* metamorphosis_buff;
    const spell_data_t* soul_fragment;

    // Havoc
    const spell_data_t* havoc;
    const spell_data_t* annihilation;
    const spell_data_t* blade_dance;
    const spell_data_t* blur;
    const spell_data_t* chaos_nova;
    const spell_data_t* chaos_strike;
    const spell_data_t* chaos_strike_refund;
    const spell_data_t* chaos_strike_fury;
    const spell_data_t* death_sweep;
    const spell_data_t* demonic_appetite_fury;
    const spell_data_t* eye_beam;
    const spell_data_t* fel_rush_damage;
    const spell_data_t* vengeful_retreat;
    const spell_data_t* momentum_buff;

    // Vengeance
    const spell_data_t* vengeance;
    const spell_data_t* demon_spikes;
    const spell_data_t* fiery_brand_dr;
    const spell_data_t* immolation_aura;
    const spell_data_t* riposte;
    const spell_data_t* soul_cleave;
  } spec;

  struct azerite_t
  {
    // Havoc
    azerite_power_t chaotic_transformation;
    azerite_power_t eyes_of_rage;
    azerite_power_t furious_gaze;
    azerite_power_t revolving_blades;
    azerite_power_t seething_power;
    azerite_power_t thirsting_blades;
  } azerite;

  // Mastery Spells
  struct mastery_t
  {
    const spell_data_t* demonic_presence;  // Havoc
    const spell_data_t* fel_blood;         // Vengeance
  } mastery;

  // Cooldowns
  struct cooldowns_t
  {
    // General
    cooldown_t* consume_magic;
    cooldown_t* disrupt;
    cooldown_t* felblade;
    cooldown_t* fel_eruption;

    // Havoc
    cooldown_t* blade_dance;
    cooldown_t* blur;
    cooldown_t* chaos_nova;
    cooldown_t* dark_slash;
    cooldown_t* demonic_appetite;
    cooldown_t* eye_beam;
    cooldown_t* fel_barrage;
    cooldown_t* fel_rush;
    cooldown_t* metamorphosis;
    cooldown_t* nemesis;
    cooldown_t* netherwalk;
    cooldown_t* throw_glaive;
    cooldown_t* vengeful_retreat;
    cooldown_t* movement_shared;

    // Vengeance
    cooldown_t* demon_spikes;
    cooldown_t* fiery_brand;
    cooldown_t* sigil_of_chains;
    cooldown_t* sigil_of_flame;
    cooldown_t* sigil_of_misery;
    cooldown_t* sigil_of_silence;
  } cooldown;

  // Gains
  struct gains_t
  {
    // General
    gain_t* miss_refund;
    gain_t* immolation_aura;

    // Havoc
    gain_t* blind_fury;
    gain_t* demonic_appetite;
    gain_t* prepared;

    // Vengeance
    gain_t* metamorphosis;

    // Azerite
    gain_t* thirsting_blades;
    gain_t* revolving_blades;
  } gain;

  // Benefits
  struct benefits_t
  {
  } benefits;

  // Procs
  struct procs_t
  {
    // General
    proc_t* delayed_aa_range;
    proc_t* delayed_aa_channel;
    proc_t* soul_fragment_greater;
    proc_t* soul_fragment_lesser;

    // Havoc
    proc_t* demon_blades_wasted;
    proc_t* demonic_appetite;
    proc_t* demons_bite_in_meta;
    proc_t* chaos_strike_in_dark_slash;
    proc_t* annihilation_in_dark_slash;
    proc_t* felblade_reset;

    // Vengeance
    proc_t* gluttony;
    proc_t* soul_fragment_expire;
    proc_t* soul_fragment_overflow;
    proc_t* soul_fragment_from_shear;
    proc_t* soul_fragment_from_fracture;
    proc_t* soul_fragment_from_fallout;
  } proc;

  // RPPM objects
  struct rppms_t
  {
    // Havoc
    real_ppm_t* felblade_havoc;
    real_ppm_t* demonic_appetite;

    // Vengeance
    real_ppm_t* felblade;
    real_ppm_t* gluttony;
  } rppm;

  // Shuffled proc objects
  struct shuffled_rngs_t
  {
  } shuffled_rngs;

  // Special
  struct actives_t
  {
    // General
    heal_t* consume_soul_greater;
    heal_t* consume_soul_lesser;

    // Havoc
    attack_t* demon_blades;

    // Vengeance
    spell_t* immolation_aura;
    heal_t* spirit_bomb_heal;
  } active;

  // Pets
  struct pets_t
  {
  } pets;

  // Options
  struct demon_hunter_options_t
  {
  } options;

  demon_hunter_t( sim_t* sim, const std::string& name, race_e r );

  // overridden player_t init functions
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void copy_from( player_t* source ) override;
  action_t* create_action( const std::string& name,
                           const std::string& options ) override;
  void create_buffs() override;
  expr_t* create_expression( const std::string& ) override;
  void create_options() override;
  pet_t* create_pet( const std::string& name,
                     const std::string& type = std::string() ) override;
  std::string create_profile( save_e ) override;
  void init_absorb_priority() override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_procs() override;
  void init_resources( bool force ) override;
  void init_rng() override;
  void init_scaling() override;
  void init_spells() override;
  void invalidate_cache( cache_e ) override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;

  // custom demon_hunter_t init functions
private:
  void apl_precombat();
  void apl_default();
  void apl_havoc();
  void apl_vengeance();
  void create_cooldowns();
  void create_gains();
  void create_benefits();

public:
  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  // overridden player_t stat functions
  double composite_armor() const override;
  double composite_armor_multiplier() const override;
  double composite_attack_power_multiplier() const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_crit_avoidance() const override;
  double composite_dodge() const override;
  double composite_melee_haste() const override;
  double composite_spell_haste() const override;
  double composite_leech() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_expertise( const weapon_t* ) const override;
  double composite_parry() const override;
  double composite_parry_rating() const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_dd_multiplier(school_e, const action_t* a) const override;
  double composite_player_td_multiplier( school_e, const action_t* a ) const override;
  double composite_player_target_multiplier(player_t* target, school_e school) const override;
  double composite_spell_crit_chance() const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double passive_movement_modifier() const override;
  double temporary_movement_modifier() const override;

  // overridden player_t combat functions
  void assess_damage( school_e, dmg_e, action_state_t* s ) override;
  void combat_begin() override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  void interrupt() override;
  void recalculate_resource_max( resource_e ) override;
  void reset() override;
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void target_mitigation( school_e, dmg_e, action_state_t* ) override;

  // custom demon_hunter_t functions
  void set_out_of_range( timespan_t duration );
  void adjust_movement();
  double calculate_expected_max_health() const;
  unsigned consume_soul_fragments( soul_fragment = soul_fragment::ALL, bool heal = true, unsigned max = MAX_SOUL_FRAGMENTS );
  unsigned get_active_soul_fragments( soul_fragment = soul_fragment::ALL ) const;
  unsigned get_total_soul_fragments( soul_fragment = soul_fragment::ALL ) const;
  void activate_soul_fragment( soul_fragment_t* );
  void spawn_soul_fragment( soul_fragment, unsigned = 1 );
  double get_target_reach() const
  {
    return target_reach >= 0 ? target_reach : sim->target->combat_reach;
  }
  expr_t* create_sigil_expression( const std::string& );

  // Cooldown Tracking
  template <typename T_CONTAINER, typename T_DATA>
  T_CONTAINER* get_data_entry( const std::string& name, std::vector<T_DATA*>& entries )
  {
    for ( size_t i = 0; i < entries.size(); i++ )
    {
      if ( entries[ i ]->first == name )
      {
        return &( entries[ i ]->second );
      }
    }

    entries.push_back( new T_DATA( name, T_CONTAINER() ) );
    return &( entries.back()->second );
  }

  virtual ~demon_hunter_t();

private:
  target_specific_t<demon_hunter_td_t> _target_data;
};

demon_hunter_t::~demon_hunter_t()
{
  range::dispose( counters );
}

counter_t::counter_t( demon_hunter_t* p ) : sim( p->sim ), value( 0 ), interval( 0 ), last( timespan_t::min() )
{
  p->counters.push_back( this );
}

// Delayed Execute Event ====================================================

struct delayed_execute_event_t : public event_t
{
  action_t* action;
  player_t* target;

  delayed_execute_event_t( demon_hunter_t* p, action_t* a, player_t* t, timespan_t delay )
    : event_t( *p->sim, delay ), action( a ), target( t )
  {
    assert( action->background );
  }

  const char* name() const override
  {
    return action->name();
  }

  void execute() override
  {
    if ( !target->is_sleeping() )
    {
      action->set_target( target );
      action->execute();
    }
  }
};

// Movement Buff definition =================================================

struct exit_melee_event_t : public event_t
{
  demon_hunter_t& dh;
  movement_buff_t* trigger_buff;

  exit_melee_event_t( demon_hunter_t* p, timespan_t delay, movement_buff_t* trigger_buff )
    : event_t( *p, delay ), dh( *p ),
    trigger_buff(trigger_buff)
  {
    assert(delay > timespan_t::zero());
  }

  const char* name() const override
  {
    return "exit_melee_event";
  }

  void execute() override
  {
    // Trigger an out of range buff based on the distance to return plus remaining movement aura time
    if (trigger_buff && trigger_buff->yards_from_melee > 0.0)
    {
      const timespan_t base_duration = timespan_t::from_seconds(trigger_buff->yards_from_melee / dh.cache.run_speed());
      dh.set_out_of_range(base_duration + trigger_buff->remains());
    }

    dh.exit_melee_event = nullptr;
  }
};

bool movement_buff_t::trigger( int s, double v, double c, timespan_t d )
{
  assert( distance_moved > 0 );
  assert( buff_duration > timespan_t::zero() );

  // Check if we're already moving away from the target, if so we will now be moving towards it
  if ( dh->current.distance_to_move || dh->buff.out_of_range->check() ||
       dh->buff.vengeful_retreat_move->check() )
  {
    dh->set_out_of_range(timespan_t::zero());
    yards_from_melee = 0.0;
  }
  // Since we're not yet moving, we should be moving away from the target
  else
  {
    // Calculate the number of yards away from melee this will send us.
    // This is equal to twice the reach + melee range, assuming we are moving "across" the target
    yards_from_melee = std::max(0.0, distance_moved - (dh->get_target_reach() + 5.0) * 2.0);
  }

  if ( yards_from_melee > 0.0 )
  {
    assert(!dh->exit_melee_event);

    // Calculate the amount of time it will take for the movement to carry us out of range
    const timespan_t delay = buff_duration * (1.0 - (yards_from_melee / distance_moved));

    assert(delay > timespan_t::zero() );

    // Schedule event to set us out of melee.
    dh->exit_melee_event = make_event<exit_melee_event_t>(*sim, dh, delay, this);
  }

  return buff_t::trigger( s, v, c, d );
}

// Soul Fragment definition =================================================

struct soul_fragment_t
{
  struct fragment_expiration_t : public event_t
  {
    soul_fragment_t* frag;

    fragment_expiration_t( soul_fragment_t* s )
      : event_t( *s->dh, s->dh->spec.soul_fragment->duration() ), frag( s )
    {
    }

    const char* name() const override
    {
      return "Soul Fragment expiration";
    }

    void execute() override
    {
      if ( sim().debug )
      {
        sim().out_debug.printf(
          "%s %s expires. active=%u total=%u", frag->dh->name(),
          get_soul_fragment_str( frag->type ),
          frag->dh->get_active_soul_fragments( frag->type ),
          frag->dh->get_total_soul_fragments( frag->type ) );
      }

      frag->expiration = nullptr;
      frag->remove();
    }
  };

  struct fragment_activate_t : public event_t
  {
    soul_fragment_t* frag;

    fragment_activate_t( soul_fragment_t* s ) : event_t( *s->dh ), frag( s )
    {
      schedule( travel_time() );
    }

    const char* name() const override
    {
      return "Soul Fragment activate";
    }

    timespan_t travel_time() const
    {
      double distance = frag->get_distance( frag->dh );
      double velocity = frag->dh->spec.consume_soul_greater->missile_speed();
      return timespan_t::from_seconds( distance / velocity );
    }

    void execute() override
    {
      if (sim().debug)
      {
        sim().out_debug.printf(
          "%s %s becomes active. active=%u total=%u", frag->dh->name(),
          get_soul_fragment_str(frag->type),
          frag->dh->get_active_soul_fragments(frag->type) + 1,
          frag->dh->get_total_soul_fragments(frag->type));
      }

      frag->activate = nullptr;
      frag->expiration = make_event<fragment_expiration_t>(sim(), frag);
      frag->dh->activate_soul_fragment(frag);
    }
  };

  demon_hunter_t* dh;
  double x, y;
  event_t* activate;
  event_t* expiration;
  const soul_fragment type;

  soul_fragment_t( demon_hunter_t* p, soul_fragment t ) : dh( p ), type( t )
  {
    activate = expiration = nullptr;

    schedule_activate();
  }

  ~soul_fragment_t()
  {
    event_t::cancel( activate );
    event_t::cancel( expiration );
  }

  double get_distance( demon_hunter_t* p ) const
  {
    return p->get_position_distance( x, y );
  }

  bool active() const
  {
    return expiration != nullptr;
  }

  void remove() const
  {
    std::vector<soul_fragment_t*>::iterator it =
      std::find( dh->soul_fragments.begin(), dh->soul_fragments.end(), this );

    assert( it != dh->soul_fragments.end() );

    dh->soul_fragments.erase( it );
    delete this;
  }

  timespan_t remains() const
  {
    if ( expiration )
    {
      return expiration->remains();
    }
    else
    {
      return dh->spec.soul_fragment->duration();
    }
  }

  bool is_type( soul_fragment t ) const
  {
    return t == soul_fragment::ALL || type == t;
  }

  void set_position()
  {
    // Set base position: 15 yards to the front right or front left.
    x = dh->x_position + ( dh->next_fragment_spawn % 2 ? -10.6066 : 10.6066 );
    y = dh->y_position + 10.6066;

    // Calculate random offset, 2-5 yards from the base position.
    double r_min = 2.0, r_max = 5.0;
    // Nornmalizing factor
    double a = 2.0 / ( r_max * r_max - r_min * r_min );
    // Calculate distance from origin using power-law distribution for
    // uniformity.
    double dist = sqrt( 2.0 * dh->rng().real() / a + r_min * r_min );
    // Pick a random angle.
    double theta = dh->rng().range( 0.0, 2.0 * m_pi );
    // And finally, apply the offsets to x and y;
    x += dist * cos( theta );
    y += dist * sin( theta );

    dh->next_fragment_spawn++;
  }

  void schedule_activate()
  {
    set_position();

    activate = make_event<fragment_activate_t>( *dh->sim, this );
  }

  void consume( bool heal = true, bool instant = false )
  {
    assert( active() );

    if ( heal )
    {
      action_t* consume_action = type == soul_fragment::GREATER ?
        dh->active.consume_soul_greater : dh->active.consume_soul_lesser;

      if ( instant )
      {
        consume_action->execute();
      }
      else
      {
        double velocity = dh->spec.consume_soul_greater->missile_speed();
        timespan_t delay = timespan_t::from_seconds( get_distance( dh ) / velocity );
        make_event<delayed_execute_event_t>( *dh->sim, dh, consume_action, dh, delay );
      }
    }

    if ( dh->talent.feed_the_demon->ok() )
    {
      timespan_t duration = timespan_t::from_seconds( dh->talent.feed_the_demon->effectN( 1 ).base_value() ) / 10;
      dh->cooldown.demon_spikes->adjust( -duration );
    }

    if ( dh->talent.gluttony->ok() && dh->rppm.gluttony->trigger() )
    {
      dh->proc.gluttony->occur();

      timespan_t duration = timespan_t::from_seconds( dh->talent.gluttony->effectN( 1 ).base_value() );
      if ( dh->buff.metamorphosis->check() )
        dh->buff.metamorphosis->extend_duration( dh, duration );
      else
        dh->buff.metamorphosis->trigger( 1, dh->buff.metamorphosis->default_value, -1.0, duration );
    }

    if ( dh->azerite.eyes_of_rage.ok() )
    {
      timespan_t duration = dh->azerite.eyes_of_rage.spell_ref().effectN( 1 ).time_value();
      dh->cooldown.eye_beam->adjust( -duration );
    }

    if (type == soul_fragment::GREATER && dh->sim->target->race == RACE_DEMON)
    {
      dh->buff.demon_soul->trigger();
    }

    remove();
  }
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
  }

  struct _stat_list_t
  {
    int level;
    std::array<double, ATTRIBUTE_MAX> stats;
  };

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    base.position = POSITION_BACK;
    base.distance = 3;

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
  bool hasted_gcd;
  double energize_delta;

  // Cooldown tracking
  bool track_cd_waste;
  simple_sample_data_with_min_max_t *cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;

  // Affect flags for various dynamic effects
  struct
  {
    // Havoc
    bool demonic_presence_da;
    bool demonic_presence_ta;
    bool momentum_da;
    bool momentum_ta;

    // Vengeance
    bool charred_flesh;
  } affected_by;

  demon_hunter_action_t( const std::string& n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
                         const std::string& o = std::string() )
    : ab( n, p, s ),
      hasted_gcd( false ),
      energize_delta( 0.0 ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr )
  {
    ab::parse_options( o );
    ab::may_crit = true;
    ab::tick_may_crit = true;

    memset( &affected_by, 0, sizeof( affected_by ) );

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      // Class Damage Multiplier
      if ( ab::data().affected_by( p->spec.havoc->effectN( 1 ) ) )
        ab::base_dd_multiplier *= 1.0 + p->spec.havoc->effectN( 1 ).percent();

      if ( ab::data().affected_by( p->spec.havoc->effectN( 2 ) ) )
        ab::base_td_multiplier *= 1.0 + p->spec.havoc->effectN( 2 ).percent();

      // Hasted GCD
      hasted_gcd = ab::data().affected_by( p->spec.havoc->effectN( 3 ) );

      // Hasted Cooldowns
      if ( ab::data().affected_by( p->spec.havoc->effectN( 4 ) ) )
        ab::cooldown->hasted = true;

      // Hasted Category Cooldowns
      if ( as<int>( ab::data().category() ) == p->spec.havoc->effectN( 6 ).misc_value1() )
        ab::cooldown->hasted = true;

      // Mastery
      if ( ab::data().affected_by( p->mastery.demonic_presence->effectN( 1 ) ) )
        affected_by.demonic_presence_da = true;

      if ( ab::data().affected_by( p->mastery.demonic_presence->effectN( 3 ) ) )
        affected_by.demonic_presence_ta = true;

      // Momemtum
      if ( p->talent.momentum->ok() )
      {
        if ( ab::data().affected_by( p->spec.momentum_buff->effectN( 1 ) ) )
          affected_by.momentum_da = true;

        if ( ab::data().affected_by( p->spec.momentum_buff->effectN( 2 ) ) )
          affected_by.momentum_ta = true;
      }
    }
    else // DEMON_HUNTER_VENGEANCE
    {
      // Class Damage Multiplier
      if ( ab::data().affected_by( p->spec.vengeance->effectN( 2 ) ) )
        ab::base_dd_multiplier *= 1.0 + p->spec.vengeance->effectN( 2 ).percent();

      if ( ab::data().affected_by( p->spec.vengeance->effectN( 3 ) ) )
        ab::base_td_multiplier *= 1.0 + p->spec.vengeance->effectN( 3 ).percent();

      // Hasted GCD
      hasted_gcd = ab::data().affected_by( p->spec.vengeance->effectN( 4 ) );

      // Normal Cooldowns
      if ( ab::data().affected_by( p->spec.vengeance->effectN( 5 ) ) )
        ab::cooldown->hasted = true;

      // Category Cooldowns
      if ( as<int>( ab::data().category() ) == p->spec.vengeance->effectN( 6 ).misc_value1() )
        ab::cooldown->hasted = true;

      if ( as<int>( ab::data().category() ) == p->spec.vengeance->effectN( 7 ).misc_value1() )
        ab::cooldown->hasted = true;
      
      // Ability-Specific Damage Modifiers
      if ( ab::data().affected_by( p->spec.vengeance->effectN( 8 ) ) )
        ab::base_multiplier *= 1.0 + p->spec.vengeance->effectN( 8 ).percent();

      if ( ab::data().affected_by( p->spec.vengeance->effectN( 9 ) ) )
        ab::base_multiplier *= 1.0 + p->spec.vengeance->effectN( 9 ).percent();

      // Charred Flesh Whitelist
      if ( ab::data().affected_by( p->spec.fiery_brand_dr->effectN( 2 ) ) )
        affected_by.charred_flesh = true;
    }
  }

  demon_hunter_t* p()
  {
    return static_cast<demon_hunter_t*>( ab::player );
  }

  const demon_hunter_t* p() const
  {
    return static_cast<const demon_hunter_t*>( ab::player );
  }

  demon_hunter_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  virtual timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
      return g;

    if ( hasted_gcd )
    {
      g *= p()->cache.attack_haste();
    }

    if ( g < ab::min_gcd )
      g = ab::min_gcd;

    return g;
  }

  void init() override
  {
    ab::init();

    if ( track_cd_waste )
    {
      cd_wasted_exec =
        p()-> template get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_exec );
      cd_wasted_cumulative = p()-> template get_data_entry<simple_sample_data_with_min_max_t, data_t>(
        ab::name_str, p()->cd_waste_cumulative );
      cd_wasted_iter =
        p()-> template get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p()->cd_waste_iter );
    }
  }

  void init_finished() override
  {
    // For reporting purposes only, as the game displays this as SCHOOL_CHAOS
    if ( ab::stats->school == SCHOOL_CHROMATIC )
      ab::stats->school = SCHOOL_CHAOS;

    ab::init_finished();
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    if ( p()->talent.charred_flesh->ok() && affected_by.charred_flesh )
    {
      demon_hunter_td_t* td = p()->get_target_data( target );
      if ( td->dots.fiery_brand && td->dots.fiery_brand->is_ticking() )
      {
        m *= 1.0 + p()->talent.charred_flesh->effectN( 1 ).percent();
      }
    }

    return m;
  }

  virtual double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_da_multiplier( s );

    if ( affected_by.demonic_presence_da )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.momentum_da )
    {
      m *= 1.0 + p()->buff.momentum->check_value();
    }

    return m;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_ta_multiplier( s );

    if ( affected_by.demonic_presence_ta )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.momentum_ta )
    {
      m *= 1.0 + p()->buff.momentum->check_value();
    }

    return m;
  }

  virtual double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = ab::composite_energize_amount( s );

    if ( energize_delta > 0 )
    {
      ea += static_cast<int>( p()->rng().range( 0, 1 + energize_delta ) - ( energize_delta / 2.0 ) );
    }

    return ea;
  }

  void track_benefits( action_state_t* s )
  {
    if ( ab::snapshot_flags & STATE_TGT_MUL_TA )
    {
      if ( p()->specialization() == DEMON_HUNTER_HAVOC )
      {
        td( s->target )->debuffs.nemesis->up();
      }
    }

    if ( ab::snapshot_flags & STATE_MUL_TA )
    {
      p()->buff.demon_soul->up();
    }

    if ( affected_by.momentum_ta || affected_by.momentum_da )
    {
      p()->buff.momentum->up();
    }
  } 

  virtual void tick( dot_t* d ) override
  {
    ab::tick( d );

    accumulate_spirit_bomb( d->state );

    if ( d->state->result_amount > 0 )
    {
      track_benefits( d->state );
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s->result ) )
    {
      accumulate_spirit_bomb( s );
      trigger_chaos_brand( s );

      if ( s->result_amount > 0 )
      {
        track_benefits( s );
      }
    }
  }

  virtual void execute() override
  {
    ab::execute();

    if ( !ab::hit_any_target && ab::last_resource_cost > 0 )
    {
      trigger_refund();
    }
  }

  virtual bool ready() override
  {
    if ( ( ab::execute_time() > timespan_t::zero() || ab::channeled ) &&
      ( p()->buff.out_of_range->check() || p()->soul_fragment_pick_up ) )
    {
      return false;
    }

    if ( p()->buff.out_of_range->check() && ab::range <= 5.0 )
    {
      return false;
    }

    if ( p()->buff.fel_rush_move->check() )
    {
      return false;
    }

    return ab::ready();
  }

  virtual void update_ready( timespan_t cd ) override
  {
    if ( cd_wasted_exec &&
      ( cd > timespan_t::zero() || ( cd <= timespan_t::zero() && ab::cooldown->duration > timespan_t::zero() ) ) &&
         ab::cooldown->current_charge == ab::cooldown->charges && ab::cooldown->last_charged > timespan_t::zero() &&
         ab::cooldown->last_charged < ab::sim->current_time() )
    {
      double time_ = ( ab::sim->current_time() - ab::cooldown->last_charged ).total_seconds();
      if ( p()->sim->debug )
      {
        p()->sim->out_debug.printf( "%s %s cooldown waste tracking waste=%.3f exec_time=%.3f", p()->name(), ab::name(),
                                    time_, ab::time_to_execute.total_seconds() );
      }
      time_ -= ab::time_to_execute.total_seconds();

      if ( time_ > 0 )
      {
        cd_wasted_exec->add( time_ );
        cd_wasted_iter->add( time_ );
      }
    }

    ab::update_ready( cd );
  }

  void trigger_refund()
  {
    if ( ab::resource_current == RESOURCE_FURY || ab::resource_current == RESOURCE_PAIN )
    {
      p()->resource_gain( ab::resource_current, ab::last_resource_cost * 0.80, p()->gain.miss_refund );
    }
  }

  void accumulate_spirit_bomb( action_state_t* s )
  {
    if ( !p()->talent.spirit_bomb->ok() )
    {
      return;
    }

    if ( !( ab::harmful && s->result_amount > 0 ) )
    {
      return;
    }

    const demon_hunter_td_t* target_data = td( s->target );

    if ( !target_data->debuffs.frailty->check() )
    {
      return;
    }

    const double multiplier = target_data->debuffs.frailty->stack_value();
    p()->spirit_bomb_accumulator += s->result_amount * multiplier;
  }

  void trigger_felblade( action_state_t* s )
  {
    if ( ab::result_is_miss( s->result ) )
      return;

    if ( p()->talent.felblade->ok() && p()->rppm.felblade->trigger() )
    {
      p()->proc.felblade_reset->occur();
      p()->cooldown.felblade->reset( true );
    }
  }

  void trigger_chaos_brand( action_state_t* s )
  {
    if ( ab::sim->overrides.chaos_brand )
    {
      return;
    }

    if ( ab::result_is_miss( s->result ) )
    {
      return;
    }

    if ( s->result_amount == 0.0 )
    {
      return;
    }

    if ( s->target->debuffs.chaos_brand && p()->spec.chaos_brand->ok() )
    {
      s->target->debuffs.chaos_brand->trigger();
    }
  }

protected:
  /// typedef for demon_hunter_action_t<action_base_t>
  typedef demon_hunter_action_t base_t;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  typedef Base ab;
};

// ==========================================================================
// Demon Hunter Ability Classes
// ==========================================================================

struct demon_hunter_heal_t : public demon_hunter_action_t<heal_t>
{
  demon_hunter_heal_t( const std::string& n, demon_hunter_t* p,
                       const spell_data_t* s = spell_data_t::nil(),
                       const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {
    harmful = false;
    set_target( p );
  }
};

struct demon_hunter_spell_t : public demon_hunter_action_t<spell_t>
{
  demon_hunter_spell_t( const std::string& n, demon_hunter_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {}
};

struct demon_hunter_attack_t : public demon_hunter_action_t<melee_attack_t>
{
  demon_hunter_attack_t( const std::string& n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
                         const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {
    special = true;
  }
};

// ==========================================================================
// Demon Hunter heals
// ==========================================================================

namespace heals
{

// Consume Soul =============================================================

struct consume_soul_t : public demon_hunter_heal_t
{
  struct demonic_appetite_energize_t : public demon_hunter_spell_t
  {
    demonic_appetite_energize_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "demonic_appetite_fury", p, p->spec.demonic_appetite_fury )
    {
      may_miss = may_block = may_dodge = may_parry = may_crit = callbacks = false;
      background = quiet = dual = true;
      energize_type = ENERGIZE_ON_CAST;
    }
  };

  const soul_fragment type;
  const spell_data_t* vengeance_heal;
  const timespan_t vengeance_heal_interval;

  consume_soul_t( demon_hunter_t* p, const std::string& n, const spell_data_t* s, soul_fragment t )
    : demon_hunter_heal_t( n, p, s ), 
    type( t ),
    vengeance_heal( p->find_specialization_spell( 203783 ) ),
    vengeance_heal_interval( timespan_t::from_seconds( vengeance_heal->effectN( 4 ).base_value() ) )
  {
    may_miss = may_crit = false;
    background = true;

    if ( p->talent.demonic_appetite->ok() )
    {
      execute_action = new demonic_appetite_energize_t( p );
    }
  }

  double calculate_heal( const action_state_t* ) const
  {
    if ( p()->specialization() == DEMON_HUNTER_HAVOC )
    {
      // Havoc always heals for the same percentage of HP, regardless of the soul type consumed
      return player->resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();
    }
    else
    {
      if ( type == soul_fragment::LESSER )
      {
        // Vengeance-Specific Healing Logic
        // This is not in the heal data and they use SpellId 203783 to control the healing parameters
        return std::max( player->resources.max[ RESOURCE_HEALTH ] * vengeance_heal->effectN( 3 ).percent(),
                         player->compute_incoming_damage( vengeance_heal_interval ) * vengeance_heal->effectN( 2 ).percent() );
      }
      // SOUL_FRAGMENT_GREATER for Vengeance uses AP mod calculations
    }

    return 0.0;
  }

  double base_da_min( const action_state_t* s ) const override
  {
    return calculate_heal( s );
  }

  double base_da_max( const action_state_t* s ) const override
  {
    return calculate_heal( s );
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_heal_t : public demon_hunter_heal_t
{
  fel_devastation_heal_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "fel_devastation_heal", p, p->find_spell( 212106 ) )
  {
    background = true;
  }
};

// Soul Barrier =============================================================

struct soul_barrier_t : public demon_hunter_action_t<absorb_t>
{
  unsigned souls_consumed;

  soul_barrier_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_action_t( "soul_barrier", p, p->talent.soul_barrier, options_str ),
    souls_consumed( 0 )
  {
    // Doesn't get populated from spell data correctly since this is not actually an AP coefficient
    base_dd_min = base_dd_max = 0;
    attack_power_mod.direct = data().effectN( 1 ).percent();
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    double c = demon_hunter_action_t<absorb_t>::attack_direct_power_coefficient( s );

    c += souls_consumed * data().effectN( 2 ).percent();

    return c;
  }

  void execute() override
  {
    souls_consumed = p()->consume_soul_fragments();
    demon_hunter_action_t<absorb_t>::execute();
  }

  void impact( action_state_t* s ) override
  {
    p()->buff.soul_barrier->trigger( 1, s->result_amount );
  }
};

// Soul Cleave ==============================================================

struct soul_cleave_heal_t : public demon_hunter_heal_t
{
  struct feast_of_souls_heal_t : public demon_hunter_heal_t
  {
    feast_of_souls_heal_t(demon_hunter_t* p)
      : demon_hunter_heal_t( "feast_of_souls", p, p->find_spell( 207693 ) )
    {
      background = dual = true;
      hasted_ticks = false;
      tick_zero = true;
    }
  };

  soul_cleave_heal_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "soul_cleave_heal", p, p->spec.soul_cleave )
  {
    background = dual = true;

    // Clear out the costs since this is just a copy of the damage spell
    base_costs.fill( 0 );
    secondary_costs.fill( 0 );

    if ( p->talent.feast_of_souls->ok() )
    {
      execute_action = new feast_of_souls_heal_t( p );
    }
  }
};

// Spirit Bomb ==============================================================

struct spirit_bomb_heal_t : public demon_hunter_heal_t
{
  spirit_bomb_heal_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "spirit_bomb_heal", p, p->find_spell( 227255 ) )
  {
    background = true;
    may_crit   = false;
  }
};

} // end namespace spells

// ==========================================================================
// Demon Hunter spells
// ==========================================================================

namespace spells
{

// Blur =====================================================================

struct blur_t : public demon_hunter_spell_t
{
  blur_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "blur", p, p->spec.blur, options_str )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p()->buff.blur->trigger();
  }
};

// Chaos Nova ===============================================================

struct chaos_nova_t : public demon_hunter_spell_t
{
  chaos_nova_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "chaos_nova", p, p->spec.chaos_nova, options_str )
  {
    aoe = -1;
    cooldown->duration += p->talent.unleashed_power->effectN( 1 ).time_value();
    base_costs[ RESOURCE_FURY ] *= 1.0 + p->talent.unleashed_power->effectN( 2 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( s->target->type == ENEMY_ADD )
    {
      if ( p()->rng().roll( data().effectN( 3 ).percent() ) )
      {
        p()->spawn_soul_fragment( soul_fragment::LESSER );
      }
    }
  }
};

// Consume Magic ============================================================

struct consume_magic_t : public demon_hunter_spell_t
{
  resource_e resource;
  double resource_amount;

  consume_magic_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "consume_magic", p, p->spec.consume_magic, options_str )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;

    const spelleffect_data_t effect = data().effectN(
      p->specialization() == DEMON_HUNTER_HAVOC ? 2 : 3);

    energize_type = ENERGIZE_ON_CAST;
    energize_resource = effect.resource_gain_type();
    energize_amount = effect.resource(energize_resource);
  }

  bool ready() override
  {
    // Currently no support for magic debuffs on bosses, just return FALSE
    return false;
  }
};

// Demon Spikes =============================================================

struct demon_spikes_t : public demon_hunter_spell_t
{
  demon_spikes_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t("demon_spikes", p, p->spec.demon_spikes, options_str)
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;
    use_off_gcd = true;
    harmful = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    p()->buff.demon_spikes->trigger();
  }
};

// Disrupt ==================================================================

struct disrupt_t : public demon_hunter_spell_t
{
  resource_e resource;
  double resource_amount;

  disrupt_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "disrupt", p, p->spec.disrupt, options_str )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;

    const spelleffect_data_t effect = p->find_spell( 218903 )
      ->effectN( p->specialization() == DEMON_HUNTER_HAVOC ? 1 : 2 );

    energize_type = ENERGIZE_ON_CAST;
    energize_resource = effect.resource_gain_type();
    energize_amount = effect.resource( energize_resource );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return demon_hunter_spell_t::target_ready( candidate_target );
  }
};

// Eye Beam =================================================================

struct eye_beam_t : public demon_hunter_spell_t
{
  struct eye_beam_tick_t : public demon_hunter_spell_t
  {
    eye_beam_tick_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "eye_beam_tick", p, p->find_spell( 198030 ) )
    {
      background = dual = true;
      aoe = -1;
      base_crit += p->spec.havoc->effectN( 3 ).percent();
    }

    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = demon_hunter_spell_t::composite_target_multiplier( target );

      if ( target == this->target )
      {
        m *= 1.0 + p()->spec.eye_beam->effectN( 3 ).percent();
      }

      return m;
    }

    virtual double bonus_da( const action_state_t* s ) const override
    {
      double b = demon_hunter_spell_t::bonus_da( s );

      b += p()->azerite.eyes_of_rage.value( 2 );

      return b;
    }
  };

  eye_beam_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "eye_beam", p, p->spec.eye_beam, options_str )
  {
    may_miss = may_crit = false;
    channeled = true;

    dot_duration *= 1.0 + p->talent.blind_fury->effectN( 1 ).percent();

    tick_action = new eye_beam_tick_t( p );
  }

  void last_tick( dot_t* d ) override
  {
    demon_hunter_spell_t::last_tick( d );

    p()->buff.furious_gaze->trigger();
  }

  void execute() override
  {
    bool extend_meta = false;

    if (p()->talent.demonic->ok())
    {
      if (p()->buff.metamorphosis->check())
      {
        p()->buff.metamorphosis->extend_duration(p(), DEMONIC_EXTEND_DURATION );
      }
      else
      {
        // Trigger Meta before the execute so that the duration is affected by Meta haste
        extend_meta = p()->buff.metamorphosis->trigger( 1, p()->buff.metamorphosis->default_value, -1.0, DEMONIC_EXTEND_DURATION );
      }
    }

    demon_hunter_spell_t::execute();
    timespan_t duration = composite_dot_duration( execute_state );

    // Since Demonic triggers Meta with 8s + hasted duration, need to extend by the hasted duration after have an execute_state
    // 8/30/2018 - Demonic Meta always gets the increased channel time
    if (true) 
    {
      // 8/31/2018 - The channel time extend during meta is erroneously double-reduced by meta haste
      if ( p()->bugs && !extend_meta )
      {
        duration /= 1.0 + p()->buff.metamorphosis->default_value;
      }
      p()->buff.metamorphosis->extend_duration(p(), duration);
    }

    if (p()->talent.blind_fury->ok())
    {
      // Blind Fury gains scale with the duration of the channel
      p()->buff.blind_fury->buff_duration = duration;
      p()->buff.blind_fury->buff_period = timespan_t::from_millis(100) * (duration / dot_duration);
      p()->buff.blind_fury->trigger();
    }
  }
};

// Fel Barrage ==============================================================

struct fel_barrage_t : public demon_hunter_spell_t
{
  struct fel_barrage_tick_t : public demon_hunter_spell_t
  {
    fel_barrage_tick_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "fel_barrage_tick", p, p->talent.fel_barrage->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  fel_barrage_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t("fel_barrage", p, p->talent.fel_barrage, options_str)
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    channeled = tick_zero = hasted_ticks = true;

    tick_action = new fel_barrage_tick_t( p );
  }

  timespan_t composite_dot_duration( const action_state_t* /* s */ ) const override
  {
    // 6/20/2018 -- Channel duration is currently not affected by Haste, although tick rate is
    return dot_duration;
  }

  bool usable_moving() const override
  {
    return true;
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_t : public demon_hunter_spell_t
{
  struct fel_devastation_tick_t : public demon_hunter_spell_t
  {
    fel_devastation_tick_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "fel_devastation_tick", p, p->talent.fel_devastation->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  action_t* heal;

  fel_devastation_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fel_devastation", p, p->talent.fel_devastation, options_str )
  {
    may_miss = may_crit = false;
    channeled = true;

    heal = p->find_action( "fel_devastation_heal" );
    if ( !heal )
    {
      heal = new heals::fel_devastation_heal_t( p );
    }

    tick_action = p->find_action( "fel_devastation_tick" );
    if ( !tick_action )
    {
      tick_action = new fel_devastation_tick_t( p );
    }
  }

  void tick( dot_t* d ) override
  {
    heal->execute(); // Heal happens first.
    demon_hunter_spell_t::tick( d );
  }
};

// Fel Eruption =============================================================

struct fel_eruption_t : public demon_hunter_spell_t
{
  fel_eruption_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fel_eruption", p, p->talent.fel_eruption, options_str )
  {
    may_crit         = false;
    resource_current = p->specialization() == DEMON_HUNTER_HAVOC 
      ? RESOURCE_FURY : RESOURCE_PAIN;
  }
};

// Fiery Brand ==============================================================

struct fiery_brand_t : public demon_hunter_spell_t
{
  struct fiery_brand_state_t : public action_state_t
  {
    bool primary;

    fiery_brand_state_t( action_t* a, player_t* target )
      : action_state_t( a, target ), primary( false )
    {
    }

    void initialize() override
    {
      action_state_t::initialize();

      primary = false;
    }

    void copy_state( const action_state_t* s ) override
    {
      action_state_t::copy_state( s );

      primary = debug_cast<const fiery_brand_state_t*>( s )->primary;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s );

      s << " primary=" << primary;

      return s;
    }
  };

  struct fiery_brand_dot_t : public demon_hunter_spell_t
  {
    fiery_brand_dot_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "fiery_brand_dot", p, p->find_spell( 207771 ) )
    {
      background = dual = true;
      hasted_ticks = may_crit = false;

      if ( p->talent.burning_alive->ok() )
      {
        // Spread radius used for Burning Alive.
        radius = p->find_spell( 207760 )->effectN( 1 ).radius_max();
      }
      else
      {
        attack_power_mod.tick = 0.0;
      }
    }

    action_state_t* new_state() override
    {
      return new fiery_brand_state_t( this, target );
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( !t ) t = target;
      if ( !t ) return nullptr;

      return td( t )->dots.fiery_brand;
    }

    void tick( dot_t* d ) override
    {
      demon_hunter_spell_t::tick( d );

      trigger_burning_alive( d );
    }

    void trigger_burning_alive( dot_t* d )
    {
      if ( !p()->talent.burning_alive->ok() )
      {
        return;
      }

      if ( !debug_cast<fiery_brand_state_t*>( d->state )->primary )
      {
        return;
      }

      // Invalidate and retrieve the new target list
      target_cache.is_valid = false;
      std::vector<player_t*> targets = target_list();
      if ( targets.size() == 1 )
      {
        return;
      }

      // Remove all the targets with existing Fiery Brand DoTs
      auto it = std::remove_if( targets.begin(), targets.end(), [ this ]( player_t* target ) { 
        return this->td( target )->dots.fiery_brand->is_ticking(); 
      } );
      targets.erase( it, targets.end() );

      if ( targets.size() == 0 )
      {
        return;
      }

      // Execute a dot on a random target
      player_t* target = targets[ static_cast<int>( p()->rng().range( 0, static_cast<double>( targets.size() ) ) ) ];
      this->set_target( target );
      schedule_execute();
    }
  };

  action_t* dot;

  fiery_brand_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fiery_brand", p, p->find_specialization_spell( "Fiery Brand" ), options_str ),
    dot( nullptr )
  {
    use_off_gcd = true;

    dot = p->find_action( "fiery_brand_dot" );
    if ( !dot )
    {
      dot = new fiery_brand_dot_t( p );
    }
    dot->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if (result_is_miss(s->result))
      return;

    /* Set this DoT as the primary DoT, enabling its ticks to spread the DoT
       to nearby targets when Burning Alive is talented. */
    fiery_brand_state_t* fb_state = debug_cast<fiery_brand_state_t*>( dot->get_state() );
    fb_state->target = s->target;
    dot->snapshot_state( fb_state, DMG_DIRECT );
    fb_state->primary = true;
    dot->schedule_execute( fb_state );
  }
};

// Sigil of Flame ===========================================================

struct sigil_of_flame_damage_t : public demon_hunter_spell_t
{
  sigil_of_flame_damage_t(demon_hunter_t* p)
    : demon_hunter_spell_t( "sigil_of_flame_damage", p, p->find_spell( 204598 ) )
  {
    aoe = -1;
    background = dual = ground_aoe = true;
    hasted_ticks = false;

    if (p->talent.concentrated_sigils->ok())
    {
      dot_duration +=
        p->talent.concentrated_sigils->effectN(5).time_value();
    }
  }

  dot_t* get_dot(player_t* t)
  {
    if (!t) t = target;
    if (!t) return nullptr;

    return td(t)->dots.sigil_of_flame;
  }

  void make_ground_aoe_event(demon_hunter_t* dh, action_state_t* s)
  {
    make_event<ground_aoe_event_t>(*sim, dh, ground_aoe_params_t()
      .target(s->target)
      .x(dh->talent.concentrated_sigils->ok() ? dh->x_position : s->target->x_position)
      .y(dh->talent.concentrated_sigils->ok() ? dh->y_position : s->target->y_position)
      .pulse_time(dh->sigil_delay)
      .duration(dh->sigil_delay)
      .start_time(sim->current_time())
      .action(this));
  }
};

struct sigil_of_flame_t : public demon_hunter_spell_t
{
  sigil_of_flame_damage_t* damage;

  sigil_of_flame_t(demon_hunter_t* p, const std::string& options_str)
    : demon_hunter_spell_t("sigil_of_flame", p, p->find_specialization_spell("Sigil of Flame"),
      options_str)
  {
    may_miss = may_crit = false;
    damage = new sigil_of_flame_damage_t(p);
    damage->stats = stats;

    cooldown->duration *= 1.0 + p->talent.quickened_sigils->effectN(2).percent();

    // Add damage modifiers in sigil_of_flame_damage_t, not here.
  }

  // Don't record data for this action.
  void record_data(action_state_t* s) override
  {
    (void)s;
    assert(s->result_amount == 0.0);
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    p()->sigil_of_flame_activates = sim->current_time() + p()->sigil_delay;
    damage->make_ground_aoe_event( p(), execute_state );
  }

  expr_t* create_expression(const std::string& name) override
  {
    if (expr_t* e = p()->create_sigil_expression(name))
      return e;

    return demon_hunter_spell_t::create_expression(name);
  }
};

// Infernal Strike ==========================================================

struct infernal_strike_t : public demon_hunter_spell_t
{
  struct infernal_strike_impact_t : public demon_hunter_spell_t
  {
    sigil_of_flame_damage_t* sigil;

    infernal_strike_impact_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "infernal_strike_impact", p, p->find_spell( 189112 ) ),
      sigil( nullptr )
    {
      background = dual = true;
      aoe = -1;

      if ( p->talent.flame_crash->ok() )
      {
        sigil = new sigil_of_flame_damage_t( p );
      }
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      if ( sigil )
      {
        p()->sigil_of_flame_activates = sim->current_time() + p()->sigil_delay;
        sigil->make_ground_aoe_event( p(), execute_state );
      }
    }
  };

  action_t* damage;

  infernal_strike_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "infernal_strike", p, p->find_specialization_spell( "Infernal Strike" ), 
                            options_str )
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
    travel_speed = 1.0;  // allows use precombat

    damage = p->find_action( "infernal_strike_impact" );

    if ( !damage )
    {
      infernal_strike_impact_t* a = new infernal_strike_impact_t( p );
      a->stats = stats;
      if ( a->sigil )
      {
        add_child( a->sigil );
      }

      damage = a;
    }

    base_teleport_distance += p->talent.abyssal_strike->effectN( 1 ).base_value();
    cooldown->duration += p->talent.abyssal_strike->effectN( 2 ).time_value();
  }

  // leap travel time, independent of distance
  // TOCHECK: Just assuming this is the same as metamorphosis.
  timespan_t travel_time() const override
  { return timespan_t::from_seconds( 1.0 ); }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    damage->set_target(s->target);
    damage->execute();
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( ! t ) t = target;
    if ( ! t ) return nullptr;

    return td( t )->dots.sigil_of_flame;
  }

  expr_t* create_expression( const std::string& name ) override
  {
    if ( expr_t* e = p()->create_sigil_expression( name ) )
      return e;

    return demon_hunter_spell_t::create_expression( name );
  }
};

// Immolation Aura ==========================================================

struct immolation_aura_t : public demon_hunter_spell_t
{
  struct immolation_aura_damage_t : public demon_hunter_spell_t
  {
    bool initial;

    // TOCHECK: Direct, over time, or both?
    immolation_aura_damage_t( demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_spell_t( "immolation_aura_tick", p, s ), initial( false )
    {
      background = dual = true;
      aoe = -1;

      // Rename gain for periodic energizes. Initial hit action doesn't energize.
      gain = p->get_gain( "immolation_aura_tick" );

      base_multiplier *= 1.0 + p->talent.agonizing_flames->effectN( 1 ).percent();
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        // FIXME: placeholder proc chance, lack of info on real proc chance.
        if ( initial && p()->talent.fallout->ok() && rng().roll( 0.60 ) )
        {
          p()->spawn_soul_fragment( soul_fragment::LESSER, 1 );
          p()->proc.soul_fragment_from_fallout->occur();
        }
      }
    }

    dmg_e amount_type( const action_state_t*, bool ) const override
    {
      return initial ? DMG_DIRECT : DMG_OVER_TIME; // TOCHECK
    }
  };

  immolation_aura_damage_t* initial_damage;

  immolation_aura_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "immolation_aura", p, p->spec.immolation_aura, options_str )
  {
    may_miss = may_crit = false;
    dot_duration = timespan_t::zero(); 

    if ( !p->active.immolation_aura )
    {
      p->active.immolation_aura = new immolation_aura_damage_t( p, data().effectN( 1 ).trigger() );
      p->active.immolation_aura->stats = stats;
    }

    initial_damage = new immolation_aura_damage_t( p, data().effectN( 2 ).trigger() );
    initial_damage->initial = true;
    initial_damage->stats = stats;

    // Add damage modifiers in immolation_aura_damage_t, not here.
  }

  void execute() override
  {
    p()->buff.immolation_aura->trigger();

    demon_hunter_spell_t::execute();

    initial_damage->set_target( target );
    initial_damage->execute();
  }
};

// Metamorphosis ============================================================

struct metamorphosis_t : public demon_hunter_spell_t
{
  struct metamorphosis_impact_t : public demon_hunter_spell_t
  {
    metamorphosis_impact_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "metamorphosis_impact", p, p->find_spell( 200166 ) )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  metamorphosis_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "metamorphosis", p, p->spec.metamorphosis, options_str )
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    dot_duration = timespan_t::zero();

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      base_teleport_distance  = data().max_range();
      movement_directionality = MOVEMENT_OMNI;
      min_gcd                 = timespan_t::from_seconds( 1.0 );  // Cannot use skills during travel time
      travel_speed            = 1.0;                              // Allows use in the precombat list

      impact_action = new metamorphosis_impact_t( p );
      // Don't assign the stats here because we don't want Meta to show up in the DPET chart
    }
  }

  // leap travel time, independent of distance
  timespan_t travel_time() const override
  {
    if ( p()->specialization() == DEMON_HUNTER_HAVOC )
      return timespan_t::from_seconds( 1.0 );
    else // DEMON_HUNTER_VENGEANCE
      return timespan_t::zero();
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if (p()->specialization() == DEMON_HUNTER_HAVOC)
    {
      // Buff is gained at the start of the leap.
      if ( p()->buff.metamorphosis->check() )
      {
        p()->buff.metamorphosis->extend_duration( p(), p()->buff.metamorphosis->buff_duration );
      }
      else
      {
        p()->buff.metamorphosis->trigger();
      }

      if ( p()->azerite.chaotic_transformation.ok() )
      {
        p()->cooldown.eye_beam->reset( false );
        p()->cooldown.blade_dance->reset( false );
      }
    }
    else // DEMON_HUNTER_VENGEANCE
    {
      p()->buff.metamorphosis->trigger();
    }
  }
};

// Nemesis ==================================================================

struct nemesis_t : public demon_hunter_spell_t
{
  nemesis_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "nemesis", p, p->talent.nemesis, options_str )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    td( s->target )->debuffs.nemesis->trigger();
  }
};

// Pick up Soul Fragment ====================================================

struct pick_up_fragment_t : public demon_hunter_spell_t
{
  struct pick_up_event_t : public event_t
  {
    demon_hunter_t* dh;
    soul_fragment_t* frag;
    expr_t* expr;

    pick_up_event_t( soul_fragment_t* f, timespan_t time, expr_t* e )
      : event_t( *f->dh, time ), 
      dh( f->dh ), 
      frag( f ), 
      expr( e )
    {
    }

    const char* name() const override
    {
      return "Soul Fragment pick up";
    }

    void execute() override
    {
      // Evaluate if_expr to make sure the actor still wants to consume.
      if ( frag && frag->active() && ( !expr || expr->eval() ) && dh->active.consume_soul_greater )
      {
        frag->consume( true, true );
      }

      dh->soul_fragment_pick_up = nullptr;
    }
  };

  std::vector<soul_fragment_t*>::iterator it;
  enum soul_fragment_select_e
  {
    SOUL_FRAGMENT_SELECT_NEAREST,
    SOUL_FRAGMENT_SELECT_NEWEST,
    SOUL_FRAGMENT_SELECT_OLDEST,
  };
  soul_fragment_select_e select_mode;
  soul_fragment type;

  pick_up_fragment_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "pick_up_fragment", p, spell_data_t::nil() ),
      select_mode( SOUL_FRAGMENT_SELECT_OLDEST ),
      type( soul_fragment::ALL )
  {
    std::string type_str, mode_str;
    add_option( opt_string( "type", type_str ) );
    add_option( opt_string( "mode", mode_str ) );
    parse_mode( mode_str );
    parse_type( mode_str );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    // use_off_gcd = true;
    may_miss = may_crit = callbacks = harmful = false;
    range = 5.0;  // Disallow use outside of melee.
  }

  void parse_mode( const std::string& value )
  {
    if ( value == "close" || value == "near" || value == "closest" ||
         value == "nearest" )
    {
      select_mode = SOUL_FRAGMENT_SELECT_NEAREST;
    }
    else if ( value == "new" || value == "newest" )
    {
      select_mode = SOUL_FRAGMENT_SELECT_NEWEST;
    }
    else if ( value == "old" || value == "oldest" )
    {
      select_mode = SOUL_FRAGMENT_SELECT_OLDEST;
    }
    else if ( value != "" )
    {
      sim->errorf(
        "%s uses bad parameter for pick_up_soul_fragment option "
        "\"mode\". Valid options: closest, newest, oldest",
        sim->active_player->name() );
    }
  }

  void parse_type( const std::string& value )
  {
    if ( value == "greater" )
    {
      type = soul_fragment::GREATER;
    }
    else if ( value == "lesser" )
    {
      type = soul_fragment::LESSER;
    }
    else if ( value == "all" || value == "any" )
    {
      type = soul_fragment::ALL;
    }
    else if ( value != "" )
    {
      sim->errorf(
        "%s uses bad parameter for pick_up_soul_fragment option "
        "\"type\". Valid options: greater, lesser, any",
        sim->active_player->name() );
    }
  }

  timespan_t calculate_movement_time( soul_fragment_t* frag )
  {
    // Fragments have a 6 yard trigger radius
    double dtm = std::max( 0.0, frag->get_distance( p() ) - 6.0 );

    return timespan_t::from_seconds( dtm / p()->cache.run_speed() );
  }

  soul_fragment_t* select_fragment()
  {
    switch ( select_mode )
    {
      case SOUL_FRAGMENT_SELECT_NEAREST:
      {
        double dist                = std::numeric_limits<double>::max();
        soul_fragment_t* candidate = nullptr;

        for ( it = p()->soul_fragments.begin(); it != p()->soul_fragments.end(); it++ )
        {
          soul_fragment_t* frag = *it;

          if ( frag->is_type( type ) && frag->active() &&
               frag->remains() < calculate_movement_time( frag ) )
          {
            double this_distance = frag->get_distance( p() );

            if ( this_distance < dist )
            {
              dist = this_distance;
              candidate = frag;
            }
          }
        }

        return candidate;
      }
      break;
      case SOUL_FRAGMENT_SELECT_NEWEST:
        for ( it = p()->soul_fragments.end(); it != p()->soul_fragments.begin(); it-- )
        {
          soul_fragment_t* frag = *it;

          if ( frag->is_type( type ) &&
               frag->remains() > calculate_movement_time( frag ) )
          {
            return frag;
          }
        }
        break;
      case SOUL_FRAGMENT_SELECT_OLDEST:
      default:
        for ( it = p()->soul_fragments.begin(); it != p()->soul_fragments.end(); it++ )
        {
          soul_fragment_t* frag = *it;

          if ( frag->is_type( type ) &&
               frag->remains() > calculate_movement_time( frag ) )
          {
            return frag;
          }
        }
        break;
    }

    return nullptr;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    soul_fragment_t* frag = select_fragment();
    assert( frag );
    timespan_t time = calculate_movement_time( frag );

    assert( p()->soul_fragment_pick_up == nullptr );
    p()->soul_fragment_pick_up = make_event<pick_up_event_t>( *sim, frag, time, if_expr );
  }

  bool ready() override
  {
    if ( p()->soul_fragment_pick_up )
    {
      return false;
    }

    if ( !p()->get_active_soul_fragments( type ) )
    {
      return false;
    }

    if ( !demon_hunter_spell_t::ready() )
    {
      return false;
    }

    // Catch edge case where a fragment exists but we can't pick it up in time.
    return select_fragment() != nullptr;
  }
};

// Spirit Bomb ==============================================================

struct spirit_bomb_t : public demon_hunter_spell_t
{
  struct spirit_bomb_damage_t : public demon_hunter_spell_t
  {
    spirit_bomb_damage_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "spirit_bomb_damage", p, p->find_spell( 247455 ) )
    {
      background = dual = true;
      aoe = -1;
    }

    void impact(action_state_t* s) override
    {
      // Spirit Bomb benefits from its own Frailty debuff
      if (result_is_hit(s->result))
      {
        td(s->target)->debuffs.frailty->trigger();
      }

      demon_hunter_spell_t::impact(s);
    }
  };

  spirit_bomb_damage_t* damage;
  unsigned max_fragments_consumed;

  spirit_bomb_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "spirit_bomb", p, p->talent.spirit_bomb, options_str ),
    damage( new spirit_bomb_damage_t( p ) ),
    max_fragments_consumed( static_cast<unsigned>( data().effectN( 2 ).base_value() ) )
  {
    may_miss = may_crit = proc = callbacks = may_dodge = may_parry = may_block = false;

    damage->stats = stats;

    if (!p->active.spirit_bomb_heal)
    {
      p->active.spirit_bomb_heal = new heals::spirit_bomb_heal_t(p);
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    // Soul fragments consumed are capped for Spirit Bomb
    const int fragments_consumed =
      p()->consume_soul_fragments( soul_fragment::ALL, true, max_fragments_consumed );

    damage->set_target( target );
    action_state_t* damage_state = damage->get_state();
    damage->snapshot_state( damage_state, DMG_DIRECT );
    damage_state->da_multiplier *= fragments_consumed;
    damage->schedule_execute( damage_state );
    damage->execute_event->reschedule( timespan_t::from_seconds( 1.0 ) );
  }

  bool ready() override
  {
    if ( p()->get_active_soul_fragments() < 1 )
    {
      return false;
    }

    return demon_hunter_spell_t::ready();
  }
};

}  // end namespace spells

// ==========================================================================
// Demon Hunter attacks
// ==========================================================================

namespace attacks
{

// Auto Attack ==============================================================

  struct auto_attack_damage_t : public demon_hunter_attack_t
  {
    enum status_e
    {
      MELEE_CONTACT,
      LOST_CONTACT_CHANNEL,
      LOST_CONTACT_RANGE,
    };

    struct status_t
    {
      status_e main_hand, off_hand;
    } status;

    auto_attack_damage_t( const std::string& name, demon_hunter_t* p, weapon_t* w, const spell_data_t* s = spell_data_t::nil() )
      : demon_hunter_attack_t( name, p, s )
    {
      school = SCHOOL_PHYSICAL;
      special = false;
      background = repeating = may_glance = true;
      trigger_gcd = timespan_t::zero();
      weapon = w;
      weapon_multiplier = 1.0;
      base_execute_time = weapon->swing_time;
      
      affected_by.momentum_da = true;

      status.main_hand = status.off_hand = LOST_CONTACT_RANGE;

      if ( p->dual_wield() )
      {
        base_hit -= 0.19;
      }
    }

    void reset() override
    {
      demon_hunter_attack_t::reset();

      status.main_hand = status.off_hand = LOST_CONTACT_RANGE;
    }

    timespan_t execute_time() const override
    {
      status_e s =
        weapon->slot == SLOT_MAIN_HAND ? status.main_hand : status.off_hand;

      switch ( s )
      {
        // Start 500ms polling for being "back in range".
        case LOST_CONTACT_CHANNEL:
        case LOST_CONTACT_RANGE:
          return timespan_t::from_millis( 500 );
        default:
          return demon_hunter_attack_t::execute_time();
      }
    }

    void trigger_demon_blades( action_state_t* s )
    {
      if ( !p()->talent.demon_blades->ok() )
        return;

      if ( !result_is_hit( s->result ) )
        return;

      if ( !rng().roll( p()->talent.demon_blades->effectN( 1 ).percent() ) )
        return;

      p()->active.demon_blades->set_target( s->target );
      p()->active.demon_blades->execute();
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      trigger_demon_blades( s );
    }

    void schedule_execute( action_state_t* s ) override
    {
      demon_hunter_attack_t::schedule_execute( s );

      if ( weapon->slot == SLOT_MAIN_HAND )
        status.main_hand = MELEE_CONTACT;
      else if ( weapon->slot == SLOT_OFF_HAND )
        status.off_hand = MELEE_CONTACT;
    }

    void execute() override
    {
      if ( p()->current.distance_to_move > 5 || p()->buff.out_of_range->check() ||
        ( p()->channeling && p()->channeling->interrupt_auto_attack ) )
      {
        status_e s;

        // Cancel autoattacks, auto_attack_t will restart them when we're able to
        // attack again.
        if ( p()->channeling && p()->channeling->interrupt_auto_attack )
        {
          p()->proc.delayed_aa_channel->occur();
          s = LOST_CONTACT_CHANNEL;
        }
        else
        {
          p()->proc.delayed_aa_range->occur();
          s = LOST_CONTACT_RANGE;
        }

        if ( weapon->slot == SLOT_MAIN_HAND )
        {
          status.main_hand = s;
          player->main_hand_attack->cancel();
        }
        else
        {
          status.off_hand = s;
          player->off_hand_attack->cancel();
        }
        return;
      }

      demon_hunter_attack_t::execute();
    }
  };

struct auto_attack_t : public demon_hunter_attack_t
{
  auto_attack_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "auto_attack", p, spell_data_t::nil(),
                             options_str )
  {
    range                   = 5;
    trigger_gcd             = timespan_t::zero();

    p->melee_main_hand      = new auto_attack_damage_t( "auto_attack_mh", p, &( p->main_hand_weapon ) );
    p->main_hand_attack     = p->melee_main_hand;

    p->melee_off_hand       = new auto_attack_damage_t( "auto_attack_oh", p, &( p->off_hand_weapon ) );
    p->off_hand_attack      = p->melee_off_hand;
    p->off_hand_attack->id  = 1;
  }

  void execute() override
  {
    if ( p()->main_hand_attack->execute_event == nullptr )
    {
      p()->main_hand_attack->schedule_execute();
    }

    if ( p()->off_hand_attack->execute_event == nullptr )
    {
      p()->off_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    bool ready = demon_hunter_attack_t::ready();

    if ( ready )  // Range check
    {
      if ( p()->main_hand_attack->execute_event == nullptr )
        return ready;

      if ( p()->off_hand_attack->execute_event == nullptr )
        return ready;
    }

    return false;
  }
};

// Blade Dance =============================================================

struct blade_dance_base_t : public demon_hunter_attack_t
{
  struct trail_of_ruin_dot_t : public demon_hunter_spell_t
  {
    trail_of_ruin_dot_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "trail_of_ruin", p, p->talent.trail_of_ruin->effectN( 1 ).trigger() )
    {
      background = dual = true;
      hasted_ticks = false;
    }
  };

  struct blade_dance_damage_t : public demon_hunter_attack_t
  {
    timespan_t delay;
    action_t* trail_of_ruin_dot;
    bool last_attack;

    blade_dance_damage_t( demon_hunter_t* p, const spelleffect_data_t& eff, const std::string& name )
      : demon_hunter_attack_t( name, p, eff.trigger() ),
      delay( timespan_t::from_millis( eff.misc_value1() ) ),
      trail_of_ruin_dot( nullptr ),
      last_attack( false )
    {
      background = dual = true;
      aoe = -1;
    }

    virtual double composite_da_multiplier( const action_state_t* s ) const override
    {
      double dm = demon_hunter_attack_t::composite_da_multiplier( s );

      if ( s->chain_target == 0 )
      {
        dm *= 1.0 + p()->talent.first_blood->effectN( 1 ).percent();
      }

      return dm;
    }

    virtual double bonus_da( const action_state_t* s ) const override
    {
      double b = demon_hunter_attack_t::bonus_da( s );

      b += p()->azerite.revolving_blades.value( 2 );

      return b;
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      if ( last_attack )
      {
        if ( trail_of_ruin_dot )
        {
          trail_of_ruin_dot->set_target( s->target );
          trail_of_ruin_dot->execute();
        }

        p()->buff.revolving_blades->trigger();
      }
    }
  };

  std::vector<blade_dance_damage_t*> attacks;
  buff_t* dodge_buff;
  action_t* trail_of_ruin_dot;

  blade_dance_base_t( const std::string& n, demon_hunter_t* p,
                      const spell_data_t* s, const std::string& options_str, buff_t* dodge_buff )
    : demon_hunter_attack_t( n, p, s, options_str ), dodge_buff( dodge_buff ), trail_of_ruin_dot ( nullptr )
  {
    may_miss = may_crit = may_parry = may_block = may_dodge = false;
    cooldown = p->cooldown.blade_dance;  // Blade Dance/Death Sweep Shared Cooldown
    range = 5.0; // Disallow use outside of melee range.

    base_costs[ RESOURCE_FURY ] += p->talent.first_blood->effectN( 2 ).resource( RESOURCE_FURY );

    if ( p->talent.trail_of_ruin->ok() )
    {
      trail_of_ruin_dot = p->find_action( "trail_of_ruin" );
      if ( !trail_of_ruin_dot )
      {
        trail_of_ruin_dot = new trail_of_ruin_dot_t( p );
      }
    }
  }

  virtual double cost() const override
  {
    double c = demon_hunter_attack_t::cost();

    c = std::max( 0.0, c + p()->buff.revolving_blades->check_stack_value() );

    return c;
  }

  void init_finished() override
  {
    demon_hunter_attack_t::init_finished();

    for ( auto& attack : attacks )
    {
      attack->stats = stats;
    }

    if( attacks.back() )
    {
      attacks.back()->last_attack = true;

      // Trail of Ruin is added to the final hit in the attack list
      if ( p()->talent.trail_of_ruin->ok() && trail_of_ruin_dot )
      {
        attacks.back()->trail_of_ruin_dot = trail_of_ruin_dot;
      }
    }
  }

  virtual void execute() override
  {
    // Blade Dance/Death Sweep Shared Cooldown
    cooldown->duration = data().cooldown();

    demon_hunter_attack_t::execute();

    // Benefit Tracking and Consume Buff
    if ( p()->buff.revolving_blades->up() )
    {
      const double adjusted_cost = cost();
      p()->buff.revolving_blades->expire();
      p()->gain.revolving_blades->add( RESOURCE_FURY, cost() - adjusted_cost );
    }

    // Create Strike Events
    for ( auto& attack : attacks )
    {
      make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
    }

    dodge_buff->trigger();
  }
};

struct blade_dance_t : public blade_dance_base_t
{
  blade_dance_t( demon_hunter_t* p, const std::string& options_str )
    : blade_dance_base_t( "blade_dance", p, p->spec.blade_dance, options_str, p->buff.blade_dance )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 4 ), "blade_dance1" ) );
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 5 ), "blade_dance2" ) );
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 6 ), "blade_dance3" ) );
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 7 ), "blade_dance4" ) );
    }
  }

  bool ready() override
  {
    if ( !blade_dance_base_t::ready() )
      return false;

    return !p()->buff.metamorphosis->check();
  }
};

// Death Sweep ==============================================================

struct death_sweep_t : public blade_dance_base_t
{
  death_sweep_t( demon_hunter_t* p, const std::string& options_str )
    : blade_dance_base_t( "death_sweep", p, p->spec.death_sweep, options_str, p->buff.death_sweep )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 4 ), "death_sweep1" ) );
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 5 ), "death_sweep2" ) );
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 6 ), "death_sweep3" ) );
      attacks.push_back( new blade_dance_damage_t( p, data().effectN( 7 ), "death_sweep4" ) );
    }
  }

  void execute() override
  {
    blade_dance_base_t::execute();

    assert( p()->buff.metamorphosis->check() );

    // If Metamorphosis has less than 1s remaining, it gets extended so the whole Death Sweep happens during Meta.
    if ( p()->buff.metamorphosis->remains_lt( p()->buff.death_sweep->buff_duration ) )
    {
      p()->buff.metamorphosis->extend_duration( p(), p()->buff.death_sweep->buff_duration - p()->buff.metamorphosis->remains() );
    }
  }

  bool ready() override
  {
    if ( !blade_dance_base_t::ready() )
      return false;

    // Death Sweep can be queued in the last 250ms, so need to ensure meta is still up after that.
    return ( p()->buff.metamorphosis->remains() > cooldown->queue_delay() );
  }
};

// Chaos Strike =============================================================

struct chaos_strike_base_t : public demon_hunter_attack_t
{
  struct chaos_strike_damage_t: public demon_hunter_attack_t {
    timespan_t delay;
    chaos_strike_base_t* parent;
    bool may_refund;
    double thirsting_blades_bonus_da;

    chaos_strike_damage_t( demon_hunter_t* p, const spelleffect_data_t& eff, 
                           chaos_strike_base_t* a, const std::string& name )
      : demon_hunter_attack_t( name, p, eff.trigger() ), 
      delay( timespan_t::from_millis( eff.misc_value1() ) ), 
      parent( a ),
      thirsting_blades_bonus_da( 0.0 )
    {
      assert( eff.type() == E_TRIGGER_SPELL );
      background = dual = true;
      may_refund = ( weapon == &( p->off_hand_weapon ) );
      school = SCHOOL_CHAOS; // 7/16/2018 -- Manually setting since Blizzard broke the spell school data for Annihilation_2 (201428)
    }
    
    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = demon_hunter_attack_t::composite_target_multiplier( target );
      
      if ( p()->specialization() == DEMON_HUNTER_HAVOC )
      {
        m *= 1.0 + td( target )->debuffs.dark_slash->check_value();
      }

      return m;
    }

    virtual double bonus_da( const action_state_t* s ) const override
    {
      double b = demon_hunter_attack_t::bonus_da( s );

      b += thirsting_blades_bonus_da;

      return b;
    }

    virtual void execute() override
    {
      demon_hunter_attack_t::execute();

      // Technically this appears to have a 0.5s GCD, but this is not relevant at the moment
      if ( may_refund && p()->rng().roll( p()->spec.chaos_strike_refund->proc_chance() ) )
      {
        p()->resource_gain( RESOURCE_FURY, p()->spec.chaos_strike_fury->effectN( 1 ).resource( RESOURCE_FURY ), parent->gain );

        if ( p()->talent.cycle_of_hatred->ok() && p()->cooldown.metamorphosis->down() )
        {
          const double adjust_seconds = p()->talent.cycle_of_hatred->effectN( 1 ).base_value();
          p()->cooldown.metamorphosis->adjust( timespan_t::from_seconds( -adjust_seconds ) );
        }
      }
    }
  };

  std::vector<chaos_strike_damage_t*> attacks;

  chaos_strike_base_t( const std::string& n, demon_hunter_t* p,
                       const spell_data_t* s, const std::string& options_str = std::string())
    : demon_hunter_attack_t( n, p, s, options_str )
  {
    // Don't put damage modifiers here, they should go in chaos_strike_damage_t.
  }

  virtual double cost() const override
  {
    double c = demon_hunter_attack_t::cost();

    c = std::max( 0.0, c - p()->buff.thirsting_blades->check() );

    return c;
  }

  void init_finished() override
  {
    demon_hunter_attack_t::init_finished();

    // Use one stats object for all parts of the attack.
    for ( auto& attack : attacks )
    {
      attack->stats = stats;
    }
  }

  virtual void execute() override
  {
    demon_hunter_attack_t::execute();

    // Thirsting Blades Benefit Tracking and Consume Buff
    double thirsting_blades_bonus_da = p()->buff.thirsting_blades->stack_value();
    if ( thirsting_blades_bonus_da > 0 )
    {
      p()->gain.thirsting_blades->add( RESOURCE_FURY, p()->buff.thirsting_blades->check() );
      p()->buff.thirsting_blades->expire();
    }

    // Create Strike Events
    for ( auto& attack : attacks )
    {
      // Snapshot Thirsting Blades manually as we can't snapshot bonus damage with action states right now
      attack->thirsting_blades_bonus_da = thirsting_blades_bonus_da;
      make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
    }

    // Metamorphosis benefit and Dark Slash stats tracking
    if ( p()->buff.metamorphosis->up() )
    {
      if ( td( target )->debuffs.dark_slash->up() )
        p()->proc.annihilation_in_dark_slash->occur();
    }
    else
    {
      if ( td( target )->debuffs.dark_slash->up() )
        p()->proc.chaos_strike_in_dark_slash->occur();
    }

    // Demonic Appetite
    if ( p()->talent.demonic_appetite->ok() && p()->rppm.demonic_appetite->trigger() )
    {
      p()->proc.demonic_appetite->occur();
      p()->spawn_soul_fragment( soul_fragment::LESSER );
    }

    // Seething Power
    p()->buff.seething_power->trigger();
  }
};

struct chaos_strike_t : public chaos_strike_base_t
{
  chaos_strike_t( demon_hunter_t* p, const std::string& options_str)
    : chaos_strike_base_t( "chaos_strike", p, p->spec.chaos_strike, options_str )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( new chaos_strike_damage_t( p, data().effectN( 2 ), this, "chaos_strike_dmg_1" ) );
      attacks.push_back( new chaos_strike_damage_t( p, data().effectN( 3 ), this, "chaos_strike_dmg_2" ) );
    }
  }

  bool ready() override
  {
    if ( p()->buff.metamorphosis->check() )
    {
      return false;
    }

    return chaos_strike_base_t::ready();
  }
};

// Annihilation =============================================================

struct annihilation_t : public chaos_strike_base_t
{
  annihilation_t( demon_hunter_t* p, const std::string& options_str )
    : chaos_strike_base_t( "annihilation", p, p->spec.annihilation, options_str )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( new chaos_strike_damage_t( p, data().effectN( 2 ), this, "annihilation_dmg_1" ) );
      attacks.push_back( new chaos_strike_damage_t( p, data().effectN( 3 ), this, "annihilation_dmg_2" ) );
    }
  }

  bool ready() override
  {
    if ( !p()->buff.metamorphosis->check() )
    {
      return false;
    }

    return chaos_strike_base_t::ready();
  }
};

// Demon's Bite =============================================================

struct demons_bite_t : public demon_hunter_attack_t
{
  demons_bite_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "demons_bite", p, p->find_class_spell( "Demon's Bite" ), options_str )
  {
    energize_delta = energize_amount * data().effectN( 3 ).m_delta();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );
    
    if ( p()->talent.insatiable_hunger->ok() )
    {
      ea += static_cast<int>( p()->rng().range( 0, 1 + p()->talent.insatiable_hunger->effectN( 1 ).base_value() ) );
    }

    return ea;
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double b = demon_hunter_attack_t::bonus_da( s );

    b += p()->azerite.chaotic_transformation.value( 2 );

    return b;
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( p()->buff.metamorphosis->check() )
    {
      p()->proc.demons_bite_in_meta->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade(s);
  }

  bool ready() override
  {
    if ( p()->talent.demon_blades->ok() )
    {
      return false;
    }

    return demon_hunter_attack_t::ready();
  }
};

// Demon Blades =============================================================

struct demon_blades_t : public demon_hunter_attack_t
{
  demon_blades_t( demon_hunter_t* p )
    : demon_hunter_attack_t( "demon_blades", p, p->find_spell( 203796 ) )
  {
    background = true;
    energize_delta = energize_amount * data().effectN( 2 ).m_delta();
  }

  virtual double bonus_da( const action_state_t* s ) const override
  {
    double b = demon_hunter_attack_t::bonus_da( s );

    b += p()->azerite.chaotic_transformation.value( 2 );

    return b;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade(s);
  }
};

// Dark Slash ===============================================================

struct dark_slash_t : public demon_hunter_attack_t
{
  dark_slash_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "dark_slash", p, p->find_talent_spell( "Dark Slash" ), options_str )
  {
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs.dark_slash->trigger();
    }
  }
};

// Felblade =================================================================
// TODO: Real movement stuff.

struct felblade_t : public demon_hunter_attack_t
{
  struct felblade_damage_t : public demon_hunter_attack_t
  {
    felblade_damage_t( demon_hunter_t* p )
      : demon_hunter_attack_t( "felblade_damage", p, p->find_spell( 213243 ) )
    {
      background = dual = true;
      may_miss = may_dodge = may_parry = false;

      // Clear energize and then manually pick which effect to parse.
      energize_type = ENERGIZE_NONE;
      parse_effect_data( data().effectN( p->specialization() == DEMON_HUNTER_HAVOC ? 4 : 3 ) );
      gain = p->get_gain( "felblade" );
    }
  };

  felblade_damage_t* damage;

  felblade_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "felblade", p, p->talent.felblade, options_str )
  {
    may_crit = may_block = false;
    movement_directionality = MOVEMENT_TOWARDS;

    execute_action = new felblade_damage_t( p );
    execute_action->stats = stats;

    // Add damage modifiers in felblade_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    // Cancel all other movement
    p()->set_out_of_range(timespan_t::zero());
  }
};

// Fel Rush =================================================================

struct fel_rush_t : public demon_hunter_attack_t
{

  struct fel_rush_damage_t : public demon_hunter_attack_t
  {
    fel_rush_damage_t( demon_hunter_t* p )
      : demon_hunter_attack_t( "fel_rush_damage", p, p->spec.fel_rush_damage )
    {
      background = dual = true;
      aoe  = -1;
      base_multiplier *= 1.0 + p->talent.fel_mastery->effectN( 1 ).percent();
    }
  };

  bool a_cancel;
  timespan_t gcd_lag;

  fel_rush_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "fel_rush", p, p->find_class_spell( "Fel Rush" ) ),
      a_cancel( false )
  {
    add_option( opt_bool( "animation_cancel", a_cancel ) );
    parse_options( options_str );

    may_miss = may_dodge = may_parry = may_block = may_crit = false;
    min_gcd = trigger_gcd;

    execute_action = new fel_rush_damage_t( p );
    execute_action->stats = stats;

    if ( !a_cancel )
    {
      // Fel Rush does damage in a further line than it moves you
      base_teleport_distance = execute_action->radius - 5;
      movement_directionality = MOVEMENT_OMNI;
      p->buff.fel_rush_move->distance_moved = base_teleport_distance;
    }

    // Add damage modifiers in fel_rush_damage_t, not here.
  }

  void execute() override
  {
    // 07/14/2018 -- As of the latest build, Momentum is now triggered before the damage
    p()->buff.momentum->trigger();

    demon_hunter_attack_t::execute();

    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    p()->cooldown.movement_shared->start( timespan_t::from_seconds( 1.0 ) );

    if ( !a_cancel )
    {
      p()->buff.fel_rush_move->trigger();
    }

  }

  void schedule_execute( action_state_t* s ) override
  {
    // Fel Rush's loss of control causes a GCD lag after the loss ends.
    // Calculate this once on schedule_execute since gcd() is called multiple times
    gcd_lag = rng().gauss( sim->gcd_lag, sim->gcd_lag_stddev );

    demon_hunter_attack_t::schedule_execute( s );
  }

  timespan_t gcd() const override
  {
    return demon_hunter_attack_t::gcd() + gcd_lag;
  }

  bool ready() override
  {
    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    if ( p()->cooldown.movement_shared->down() )
    {
      return false;
    }

    return demon_hunter_attack_t::ready();
  }
};

// Fracture =================================================================

struct fracture_t : public demon_hunter_attack_t
{
  struct fracture_damage_t : public demon_hunter_attack_t
  {
    fracture_damage_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_attack_t( n, p, s )
    {
      background = dual = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  fracture_damage_t* mh, *oh;

  fracture_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "fracture", p, p->talent.fracture, options_str )
  {
    may_crit = false;

    mh = new fracture_damage_t( "fracture_mh", p, data().effectN( 2 ).trigger() );
    oh = new fracture_damage_t( "fracture_oh", p, data().effectN( 3 ).trigger() );
    mh->stats = oh->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade( s );

    if ( result_is_hit( s->result ) )
    {
      mh->set_target( s->target );
      oh->set_target( s->target );
      mh->execute();
      oh->execute();

      p()->spawn_soul_fragment( soul_fragment::LESSER, 2 );
      p()->proc.soul_fragment_from_fracture->occur();
      p()->proc.soul_fragment_from_fracture->occur();
    }
  }
};

// Shear ====================================================================

struct shear_t : public demon_hunter_attack_t
{
  shear_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t("shear", p, p->find_specialization_spell( "Shear" ), options_str )
  {
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade(s);

    if ( result_is_hit( s->result ) )
    {
      p()->spawn_soul_fragment( soul_fragment::LESSER );
      p()->proc.soul_fragment_from_shear->occur();
    }
  }

  bool ready() override
  {
    if ( p()->talent.fracture->ok() )
    {
      return false;
    }

    return demon_hunter_attack_t::ready();
  }
};

// Soul Cleave ==============================================================

struct soul_cleave_t : public demon_hunter_attack_t
{
  struct soul_cleave_damage_t : public demon_hunter_attack_t
  {
    soul_cleave_damage_t( const std::string& n, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_attack_t( n, p, s )
    {
      background = dual = true;
      aoe = -1;
    }
    
    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      if ( result_is_hit( s->result ) && p()->talent.void_reaver->ok() )
      {
        td( s->target )->debuffs.void_reaver->trigger();
      }
    }
  };

  heals::soul_cleave_heal_t* heal;

  soul_cleave_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "soul_cleave", p, p->spec.soul_cleave, options_str ),
      heal(new heals::soul_cleave_heal_t(p))
  {
    may_miss = may_dodge = may_parry = may_block = may_crit = false;
    attack_power_mod.direct = 0;  // This parent action deals no damage, parsed data is for the heal

    execute_action = new soul_cleave_damage_t( "soul_cleave_damage", p, data().effectN( 2 ).trigger() );
    
    // Add damage modifiers in soul_cleave_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    heal->set_target( player );
    heal->execute();

    // Soul fragments consumed are capped for Soul Cleave
    p()->consume_soul_fragments( soul_fragment::ALL, true, (unsigned)data().effectN( 3 ).base_value() );
  }
};

// Throw Glaive =============================================================

struct throw_glaive_t : public demon_hunter_attack_t
{
  throw_glaive_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t("throw_glaive", p, p->find_class_spell("Throw Glaive"), options_str)
  {
    radius = 10.0;
    cooldown->charges += static_cast<int>( p->talent.master_of_the_glaive->effectN( 2 ).base_value() );
  }
};

// Vengeful Retreat =========================================================

struct vengeful_retreat_t : public demon_hunter_attack_t
{
  struct vengeful_retreat_damage_t : public demon_hunter_attack_t
  {
    vengeful_retreat_damage_t(demon_hunter_t* p)
      : demon_hunter_attack_t("vengeful_retreat_damage", p, p->find_spell(198813))
    {
      background = dual = true;
      aoe = -1;
    }

    void execute() override
    {
      demon_hunter_attack_t::execute();

      if ( hit_any_target )
      {
        p()->buff.prepared->trigger();
      }
    }
  };

  vengeful_retreat_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "vengeful_retreat", p, p->spec.vengeful_retreat, options_str )
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    // use_off_gcd = true;

    execute_action = new vengeful_retreat_damage_t( p );
    execute_action->stats = stats;

    base_teleport_distance = VENGEFUL_RETREAT_DISTANCE;
    movement_directionality = MOVEMENT_OMNI;
    p->buff.vengeful_retreat_move->distance_moved = base_teleport_distance;
    
    cooldown->duration += p->talent.momentum->effectN( 1 ).time_value();

    // Add damage modifiers in vengeful_retreat_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    p()->cooldown.movement_shared->start( timespan_t::from_seconds( 1.0 ) );
    p()->buff.vengeful_retreat_move->trigger();
  }

  bool ready() override
  {
    // Fel Rush and VR shared a 1 second GCD when one or the other is triggered
    if (p()->cooldown.movement_shared->down())
    {
      return false;
    }

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
  using base_t = demon_hunter_buff_t;

  demon_hunter_buff_t( demon_hunter_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr )
    : BuffBase( &p, name, s, item )
  { }
  demon_hunter_buff_t( demon_hunter_td_t& td, const std::string& name, const spell_data_t* s = spell_data_t::nil(), const item_t* item = nullptr )
    : BuffBase( td, name, s, item )
  { }

protected:

  demon_hunter_t& p()
  {
    return *debug_cast<demon_hunter_t*>( BuffBase::source );
  }
  const demon_hunter_t& p() const
  {
    return *debug_cast<demon_hunter_t*>( BuffBase::source );
  }

private:
  typedef BuffBase bb;
};

// Nemesis ==================================================================

struct nemesis_debuff_t : public demon_hunter_buff_t<buff_t>
{
  nemesis_debuff_t( demon_hunter_td_t& td )
    : base_t( td, "nemesis", td.dh().talent.nemesis )
  {
    set_default_value( p().talent.nemesis->effectN( 1 ).percent() );
    set_cooldown( timespan_t::zero() );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );

    if ( remaining_duration > timespan_t::zero() )
    {
      p().buff.nemesis->trigger( 1, player->race, -1.0, remaining_duration );
    }
  }
};

// Metamorphosis Buff =======================================================

struct metamorphosis_buff_t : public demon_hunter_buff_t<buff_t>
{
  bool extended_by_demonic = false;

  static void vengeance_callback(buff_t* b, int, const timespan_t&)
  {
    demon_hunter_t* p = debug_cast<demon_hunter_t*>( b->player );
    double energize = p->spec.metamorphosis_buff->effectN( 4 ).resource( RESOURCE_PAIN );

    p->resource_gain(RESOURCE_PAIN, energize, p->gain.metamorphosis);
  }

  metamorphosis_buff_t(demon_hunter_t* p)
    : base_t(*p, "metamorphosis", p->spec.metamorphosis_buff)
  {
    set_cooldown(timespan_t::zero());
    if (p->specialization() == DEMON_HUNTER_HAVOC)
    {
      default_value = p->spec.metamorphosis_buff->effectN( 6 ).percent();
      buff_period = timespan_t::zero();
      tick_behavior = buff_tick_behavior::NONE;
      add_invalidate( CACHE_HASTE );
      add_invalidate( CACHE_LEECH );
    }
    else // DEMON_HUNTER_VENGEANCE
    {
      default_value = p->spec.metamorphosis_buff->effectN( 2 ).percent();
      buff_period = p->spec.metamorphosis_buff->effectN( 4 ).period();
      tick_callback = vengeance_callback;
      add_invalidate( CACHE_ARMOR );
      if ( p->talent.soul_rending->ok() )
      {
        add_invalidate( CACHE_LEECH );
      }
    }
  }

  bool trigger(int stacks, double value, double chance, timespan_t duration) override
  {
    if (!buff_t::trigger(stacks, value, chance, duration))
    {
      return false;
    }

    // 8/30/2018 currently you can extend demonic inside meta any # of times
    //if ( p().specialization() == DEMON_HUNTER_HAVOC )
    //{
    //  // If we have an initial trigger from Eye Beam, set the flag for future validation
    //  if ( p().executing && p().executing->id == p().spec.eye_beam->id() )
    //  {
    //    extended_by_demonic = true;
    //  }
    //  else
    //  {
    //    extended_by_demonic = false;
    //  }
    //}

    return true;
  }

  void extend_duration(player_t* p, timespan_t extra_seconds) override
  {

    // 8/30/2018 currently you can extend demonic inside meta any # of times
    //if ( this->p().specialization() == DEMON_HUNTER_HAVOC && p->executing )
    //{
    //  // If we extend the duration with a proper Meta cast, we can clear the flag as successive Eye Beams can extend again
    //  if ( p->executing->id == this->p().spec.metamorphosis->id() )
    //  {
    //    extended_by_demonic = false;
    //  }
    //  // If we are triggering from Eye Beam, we should disallow any additional full Demonic extensions
    //  else if ( p->executing->id == this->p().spec.eye_beam->id() && extra_seconds == DEMONIC_EXTEND_DURATION )
    //  {
    //    if ( extended_by_demonic )
    //      return;
	  //
    //    extended_by_demonic = true;
    //  }
    //}

    buff_t::extend_duration(p, extra_seconds);
  }

  void start(int stacks, double value, timespan_t duration) override
  {
    demon_hunter_buff_t<buff_t>::start(stacks, value, duration);

    if ( p().specialization() == DEMON_HUNTER_VENGEANCE )
    {
      p().metamorphosis_health = p().max_health() * value;
      p().stat_gain( STAT_MAX_HEALTH, p().metamorphosis_health, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    }
  }

  void expire_override(int expiration_stacks, timespan_t remaining_duration) override
  {
    demon_hunter_buff_t<buff_t>::expire_override(expiration_stacks, remaining_duration);

    if (p().specialization() == DEMON_HUNTER_VENGEANCE)
    {
      p().stat_loss(STAT_MAX_HEALTH, p().metamorphosis_health, (gain_t*)nullptr, (action_t*)nullptr, true);
      p().metamorphosis_health = 0;
    }
    else
    {
      extended_by_demonic = false;
    }
  }
};

// Demon Spikes buff ========================================================

struct demon_spikes_t : public demon_hunter_buff_t<buff_t>
{
  const timespan_t max_duration;

  demon_spikes_t(demon_hunter_t* p)
    : base_t( *p, "demon_spikes", p->find_spell( 203819 ) ),
      max_duration( buff_duration * 3 ) // Demon Spikes can only be extended to 3x its base duration
  {
    set_default_value( p->find_spell( 203819 )->effectN( 1 ).percent() );
    set_refresh_behavior( buff_refresh_behavior::EXTEND );
    add_invalidate( CACHE_PARRY );
    add_invalidate( CACHE_ARMOR );
    if ( p->talent.razor_spikes->ok() )
    {
      add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    }
  }

  virtual bool trigger(int stacks, double value, double chance, timespan_t duration) override
  {
    if (duration == timespan_t::min())
    {
      duration = buff_duration;
    }

    if (remains() + buff_duration > max_duration)
    {
      duration = max_duration - remains();
    }

    if (duration <= timespan_t::zero())
    {
      return false;
    }

    return demon_hunter_buff_t::trigger(stacks, value, chance, duration);
  }
};

}  // end namespace buffs

// ==========================================================================
// Misc. Events and Structs
// ==========================================================================

// Spirit Bomb event ========================================================

struct spirit_bomb_event_t : public event_t
{
  demon_hunter_t* dh;

  spirit_bomb_event_t( demon_hunter_t* p, bool initial = false )
    : event_t( *p), dh( p )
  {
    timespan_t delta_time = timespan_t::from_seconds( 1.0 );
    if ( initial )
    {
      delta_time *= rng().real();
    }
    schedule( delta_time );
  }

  const char* name() const override
  {
    return "spirit_bomb_driver";
  }

  void execute() override
  {
    assert( dh ->spirit_bomb_accumulator >= 0.0 );

    if ( dh->spirit_bomb_accumulator > 0 )
    {
      action_t* a = dh->active.spirit_bomb_heal;
      a->base_dd_min = a->base_dd_max = dh->spirit_bomb_accumulator;
      a->execute();

      dh->spirit_bomb_accumulator = 0.0;
    }

    dh->spirit_bomb_driver = make_event<spirit_bomb_event_t>( sim(), dh );
  }
};

// ==========================================================================
// Targetdata Definitions
// ==========================================================================

demon_hunter_td_t::demon_hunter_td_t( player_t* target, demon_hunter_t& p )
  : actor_target_data_t( target, &p ), dots( dots_t() ), debuffs( debuffs_t() )
{
  if (p.specialization() == DEMON_HUNTER_HAVOC)
  {
    debuffs.dark_slash = make_buff( *this, "dark_slash", p.talent.dark_slash )
      ->set_default_value( p.talent.dark_slash->effectN( 3 ).percent() );
    debuffs.nemesis = new buffs::nemesis_debuff_t(*this);
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    dots.fiery_brand = target->get_dot("fiery_brand", &p);
    dots.sigil_of_flame = target->get_dot("sigil_of_flame", &p);
    debuffs.frailty = make_buff( *this, "frailty", p.find_spell( 247456 ) )
      ->set_default_value( p.find_spell( 247456 )->effectN( 1 ).percent() );
    debuffs.void_reaver = make_buff( *this, "void_reaver", p.find_spell( 268178 ) )
      ->set_default_value( p.find_spell( 268178 )->effectN( 1 ).percent() );
  }

  target->callbacks_on_demise.push_back( [this]( player_t* ) { target_demise(); } );
}

void demon_hunter_td_t::target_demise()
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source->sim->event_mgr.canceled )
    return;


  // TODO: Make an option to register this for testing M+/dungeon scenarios
  //demon_hunter_t* p = static_cast<demon_hunter_t*>( source );
  //p->spawn_soul_fragment( soul_fragment::GREATER );
}

// ==========================================================================
// Demon Hunter Definitions
// ==========================================================================

demon_hunter_t::demon_hunter_t(sim_t* sim, const std::string& name, race_e r)
  : player_t(sim, DEMON_HUNTER, name, r),
  melee_main_hand(nullptr),
  melee_off_hand(nullptr),
  next_fragment_spawn(0),
  soul_fragments(),
  spirit_bomb_accumulator(0.0),
  spirit_bomb_driver(nullptr),
  target_reach(-1.0),
  exit_melee_event(nullptr),
  initial_fury(0),
  sigil_of_flame_activates(timespan_t::zero()),
  buff(),
  talent(),
  spec(),
  mastery(),
  cooldown(),
  gain(),
  benefits(),
  proc(),
  active(),
  pets(),
  options()
{
  create_cooldowns();
  create_gains();
  create_benefits();

  regen_type = REGEN_DISABLED;
}

// ==========================================================================
// overridden player_t init functions
// ==========================================================================

// demon_hunter_t::convert_hybrid_stat ======================================

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
    default:
      return s;
  }
}

// demon_hunter_t::copy_from ================================================

void demon_hunter_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  auto source_p = debug_cast<demon_hunter_t*>( source );

  options = source_p->options;
}

// demon_hunter_t::create_action ============================================

action_t* demon_hunter_t::create_action( const std::string& name,
    const std::string& options_str )
{
  using namespace actions::heals;

  if ( name == "soul_barrier" )
    return new soul_barrier_t( this, options_str );

  using namespace actions::spells;

  if ( name == "blur" )
    return new blur_t( this, options_str );
  if ( name == "chaos_nova" )
    return new chaos_nova_t( this, options_str );
  if ( name == "consume_magic" )
    return new consume_magic_t( this, options_str );
  if ( name == "demon_spikes" )
    return new demon_spikes_t( this, options_str );
  if ( name == "disrupt" )
    return new disrupt_t( this, options_str );
  if ( name == "eye_beam" )
    return new eye_beam_t( this, options_str );
  if ( name == "fel_barrage" )
    return new fel_barrage_t( this, options_str );
  if ( name == "fel_eruption" )
    return new fel_eruption_t( this, options_str );
  if ( name == "fel_devastation" )
    return new fel_devastation_t( this, options_str );
  if ( name == "fiery_brand" )
    return new fiery_brand_t( this, options_str );
  if ( name == "infernal_strike" )
    return new infernal_strike_t( this, options_str );
  if ( name == "immolation_aura" )
    return new immolation_aura_t( this, options_str );
  if ( name == "metamorphosis" || name == "meta" )
    return new metamorphosis_t( this, options_str );
  if ( name == "nemesis" )
    return new nemesis_t( this, options_str );
  if ( name == "pick_up_soul_fragment" || name == "pick_up_fragment" ||
       name == "pick_up_soul" )
  {
    return new pick_up_fragment_t( this, options_str );
  }
  if ( name == "sigil_of_flame" )
    return new sigil_of_flame_t( this, options_str );
  if ( name == "spirit_bomb" )
    return new spirit_bomb_t( this, options_str );

  using namespace actions::attacks;

  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "annihilation" )
    return new annihilation_t( this, options_str );
  if ( name == "blade_dance" )
    return new blade_dance_t( this, options_str );
  if ( name == "chaos_strike" )
    return new chaos_strike_t( this, options_str );
  if ( name == "dark_slash" )
    return new dark_slash_t( this, options_str );
  if ( name == "death_sweep" )
    return new death_sweep_t( this, options_str );
  if ( name == "demons_bite" )
    return new demons_bite_t( this, options_str );
  if ( name == "felblade" )
    return new felblade_t( this, options_str );
  if ( name == "fel_rush" )
    return new fel_rush_t( this, options_str );
  if ( name == "fracture" )
    return new fracture_t( this, options_str );
  if ( name == "shear" )
    return new shear_t( this, options_str );
  if ( name == "soul_cleave" )
    return new soul_cleave_t( this, options_str );
  if ( name == "throw_glaive" )
    return new throw_glaive_t( this, options_str );
  if ( name == "vengeful_retreat" )
    return new vengeful_retreat_t( this, options_str );

  return base_t::create_action( name, options_str );
}

// demon_hunter_t::create_buffs =============================================

void demon_hunter_t::create_buffs()
{
  base_t::create_buffs();

  using namespace buffs;

  // General ================================================================

  buff.demon_soul =
      make_buff( this, "demon_soul", find_spell( 163073 ) )
    ->set_default_value( find_spell( 163073 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.metamorphosis = new buffs::metamorphosis_buff_t( this );

  if(specialization() == DEMON_HUNTER_HAVOC )
  {
    buff.immolation_aura =
        make_buff( this, "immolation_aura", spec.immolation_aura )
      ->set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
        active.immolation_aura->execute();
      } )
      ->set_cooldown( timespan_t::zero() );
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    buff.immolation_aura =
        make_buff( this, "immolation_aura", spec.immolation_aura )
      ->set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) {
        active.immolation_aura->execute();
      } )
      ->set_default_value( talent.agonizing_flames->effectN( 2 ).percent() )
      ->add_invalidate( CACHE_RUN_SPEED )
      ->set_cooldown( timespan_t::zero() );
  }

  // Havoc ==================================================================

  buff.blade_dance =
    buff_creator_t( this, "blade_dance", spec.blade_dance )
    .default_value( spec.blade_dance->effectN( 2 ).percent() )
    .add_invalidate( CACHE_DODGE )
    .cd( timespan_t::zero() );

  buff.blur =
    buff_creator_t(this, "blur", spec.blur->effectN(1).trigger())
    .default_value(spec.blur->effectN(1).trigger()->effectN(3).percent()
      + (talent.desperate_instincts->ok() ? talent.desperate_instincts->effectN(3).percent() : 0))
    .cd(timespan_t::zero())
    .add_invalidate(CACHE_LEECH)
    .add_invalidate(CACHE_DODGE);

  buff.death_sweep =
    buff_creator_t( this, "death_sweep", spec.death_sweep )
    .default_value( spec.death_sweep->effectN( 2 ).percent() )
    .add_invalidate( CACHE_DODGE )
    .cd( timespan_t::zero() );

  buff.fel_rush_move = new movement_buff_t(
    this, buff_creator_t( this, "fel_rush_movement", spell_data_t::nil() )
    .chance( 1.0 )
    .duration( find_class_spell( "Fel Rush" )->gcd() ) );

  buff.momentum =
    buff_creator_t( this, "momentum", spec.momentum_buff )
    .default_value( spec.momentum_buff->effectN( 1 ).percent() )
    .trigger_spell( talent.momentum )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.out_of_range =
    buff_creator_t( this, "out_of_range", spell_data_t::nil() ).chance( 1.0 );

  // TODO: Buffs for each race?
  buff.nemesis = buff_creator_t( this, "nemesis_buff", find_spell( 208605, DEMON_HUNTER_HAVOC ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  const double prepared_value = ( find_spell( 203650 )->effectN( 1 ).resource( RESOURCE_FURY ) / 50 );
  buff.prepared =
    buff_creator_t( this, "prepared", find_spell( 203650, DEMON_HUNTER_HAVOC ) )
    .default_value( prepared_value )
    .trigger_spell( talent.momentum )
    .period( timespan_t::from_millis( 100 ) )
    .tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
      resource_gain( RESOURCE_FURY, b->check_value(), gain.prepared );
    } );

  buff.blind_fury =
    buff_creator_t( this, "blind_fury", spec.eye_beam )
    .cd( timespan_t::zero() )
    .default_value( talent.blind_fury->effectN( 3 ).resource( RESOURCE_FURY ) / 50 )
    .duration( spec.eye_beam->duration() * ( 1.0 + talent.blind_fury->effectN( 1 ).percent() ) )
    .period( timespan_t::from_millis( 100 ) )
    .tick_zero( true )
    .tick_callback( [ this ]( buff_t* b, int, const timespan_t& ) {
      resource_gain( RESOURCE_FURY, b->check_value(), gain.blind_fury );
    } );

  buff.vengeful_retreat_move = new movement_buff_t(this,
    buff_creator_t( this, "vengeful_retreat_movement", spell_data_t::nil() )
    .chance( 1.0 )
    .duration( spec.vengeful_retreat->duration() ) );

  // Vengeance ==============================================================

  buff.demon_spikes = new buffs::demon_spikes_t(this);

  buff.soul_barrier = make_buff<absorb_buff_t>( this, "soul_barrier", talent.soul_barrier );
  buff.soul_barrier->set_absorb_source( get_stats( "soul_barrier" ) )
      ->set_absorb_gain( get_gain( "soul_barrier" ) )
      ->set_absorb_high_priority( true )  // TOCHECK
      ->set_cooldown( timespan_t::zero() );

  // Azerite ================================================================
  
  // Furious Gaze doesn't have a trigger reference in the Azerite trait for some reason...
  const spell_data_t* furious_gaze_buff = azerite.furious_gaze.spell()->ok() ? find_spell( 273232 ) : spell_data_t::not_found();
  buff.furious_gaze = make_buff<stat_buff_t>( this, "furious_gaze", furious_gaze_buff )
    ->add_stat( STAT_HASTE_RATING, azerite.furious_gaze.value( 1 ) )
    ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
    ->set_trigger_spell( azerite.furious_gaze );

  const spell_data_t* revolving_blades_trigger = azerite.revolving_blades.spell()->effectN( 1 ).trigger();
  const spell_data_t* revolving_blades_buff = revolving_blades_trigger->effectN( 1 ).trigger();
  buff.revolving_blades = make_buff<buff_t>( this, "revolving_blades", revolving_blades_buff )
    ->set_default_value( revolving_blades_buff->effectN( 1 ).resource( RESOURCE_FURY ) )
    ->set_trigger_spell( revolving_blades_trigger );

  const spell_data_t* seething_power_trigger = azerite.seething_power.spell()->effectN( 1 ).trigger();
  const spell_data_t* seething_power_buff = seething_power_trigger->effectN( 1 ).trigger();
  buff.seething_power = make_buff<stat_buff_t>( this, "seething_power", seething_power_buff )
    ->add_stat( STAT_AGILITY, azerite.seething_power.value( 1 ) )
    ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
    ->set_trigger_spell( seething_power_trigger );

  const spell_data_t* thirsting_blades_trigger = azerite.thirsting_blades.spell()->effectN( 1 ).trigger();
  const spell_data_t* thirsting_blades_buff = thirsting_blades_trigger->effectN( 1 ).trigger();
  buff.thirsting_blades_driver = make_buff<buff_t>( this, "thirsting_blades_driver", thirsting_blades_trigger )
    ->set_trigger_spell( azerite.thirsting_blades )
    ->set_quiet( true )
    ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
    ->set_tick_callback( [ this ]( buff_t*, int, const timespan_t& ) { buff.thirsting_blades->trigger(); } );

  buff.thirsting_blades = make_buff<buff_t>( this, "thirsting_blades", thirsting_blades_buff )
    ->set_trigger_spell( thirsting_blades_trigger )
    ->set_default_value( azerite.thirsting_blades.value( 1 ) );
}

struct metamorphosis_buff_demonic_expr_t : public expr_t
{
  demon_hunter_t* dh;

  metamorphosis_buff_demonic_expr_t(demon_hunter_t* p, const std::string& name_str)
    : expr_t(name_str), dh(p)
  {
  }

  double evaluate() override
  {
    buffs::metamorphosis_buff_t* metamorphosis = debug_cast<buffs::metamorphosis_buff_t*>(dh->buff.metamorphosis);
    if (metamorphosis && metamorphosis->check() && metamorphosis->extended_by_demonic)
      return true;

    return false;
  }
};

struct metamorphosis_adjusted_cooldown_expr_t : public expr_t
{
  demon_hunter_t* dh;
  double cooldown_multiplier;

  metamorphosis_adjusted_cooldown_expr_t( demon_hunter_t* p,
                                          const std::string& name_str )
    : expr_t( name_str ),
      dh( p ),
      cooldown_multiplier( 1.0 )
  {
  }

  void calculate_multiplier()
  {
    double reduction_per_second = 0.0;

    if ( dh->talent.cycle_of_hatred->ok() )
    {
      action_t* chaos_strike = dh->find_action( "chaos_strike" );
      assert( chaos_strike );

      // Fury estimates are on the conservative end, intended to be rough approximation only
      double approx_fury_per_second = 15.5;

      // Use base haste only for approximation, don't want to calculate with temp buffs
      const double base_haste = 1.0 / dh->collected_data.buffed_stats_snapshot.attack_haste;
      approx_fury_per_second *= base_haste;

      // Assume 90% of Fury used on Chaos Strike/Annihilation
      const double approx_seconds_per_refund = ( chaos_strike->cost() / ( approx_fury_per_second * 0.9 ) )
        / dh->spec.chaos_strike_refund->proc_chance();

      if ( dh->talent.cycle_of_hatred->ok() )
        reduction_per_second += dh->talent.cycle_of_hatred->effectN( 1 ).base_value() / approx_seconds_per_refund;
    }

    cooldown_multiplier = 1.0 / ( 1.0 + reduction_per_second );
  }

  double evaluate() override
  {
    // Need to calculate shoulders on first evaluation because we don't have crit/haste values on init
    if ( cooldown_multiplier == 1 && dh->talent.cycle_of_hatred->ok() )
    {
      calculate_multiplier();
    }

    return dh->cooldown.metamorphosis->remains().total_seconds() * cooldown_multiplier;
  }
};

// demon_hunter_t::create_expression ========================================

expr_t* demon_hunter_t::create_expression( const std::string& name_str )
{
  if ( name_str == "greater_soul_fragments" ||
       name_str == "lesser_soul_fragments" || name_str == "soul_fragments" )
  {
    struct soul_fragments_expr_t : public expr_t
    {
      demon_hunter_t* dh;
      soul_fragment type;

      soul_fragments_expr_t( demon_hunter_t* p, const std::string& n,
                             soul_fragment t )
        : expr_t( n ), dh( p ), type( t )
      {
      }

      virtual double evaluate() override
      {
        return dh->get_active_soul_fragments( type );
      }
    };

    soul_fragment type;
    if ( name_str == "soul_fragments" )
    {
      type = soul_fragment::ALL;
    }
    else if ( name_str == "greater_soul_fragments" )
    {
      type = soul_fragment::GREATER;
    }
    else
    {
      type = soul_fragment::LESSER;
    }

    return new soul_fragments_expr_t( this, name_str, type );
  }
  else if ( name_str == "cooldown.metamorphosis.adjusted_remains" )
  {
    if ( this->talent.cycle_of_hatred->ok() )
    {
      return new metamorphosis_adjusted_cooldown_expr_t( this, name_str );
    }
    else
    {
      return this->cooldown.metamorphosis->create_expression( "remains" );
    }
  }
  else if ( name_str == "buff.metamorphosis.extended_by_demonic" )
  {
    return new metamorphosis_buff_demonic_expr_t( this, name_str );
  }

  return player_t::create_expression( name_str );
}

// demon_hunter_t::create_options
// ==================================================

void demon_hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "target_reach", target_reach ) );
  add_option( opt_float( "initial_fury", initial_fury ) );
}

// demon_hunter_t::create_pet ===============================================

pet_t* demon_hunter_t::create_pet( const std::string& pet_name,
                                   const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  // Add pets here

  return nullptr;
}

// demon_hunter_t::create_profile ===========================================

std::string demon_hunter_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  // Log all options here

  return profile_str;
}

// demon_hunter_t::init_absorb_priority =====================================

void demon_hunter_t::init_absorb_priority()
{
  player_t::init_absorb_priority();

  absorb_priority.push_back( 227225 );  // Soul Barrier
}

// demon_hunter_t::init_action_list =========================================

void demon_hunter_t::init_action_list()
{
  if ( main_hand_weapon.type == WEAPON_NONE ||
       off_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
    {
      sim->errorf(
        "Player %s does not have a valid main-hand and off-hand weapon.",
        name() );
    }
    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat();  // PRE-COMBAT

  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      apl_havoc();
      break;
    case DEMON_HUNTER_VENGEANCE:
      apl_vengeance();
      break;
    default:
      apl_default();  // DEFAULT
      break;
  }

  use_default_action_list = true;

  base_t::init_action_list();
}

// demon_hunter_t::init_base_stats ==========================================

void demon_hunter_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5.0;

  base_t::init_base_stats();

  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      resources.base[ RESOURCE_FURY ] = 120;
      break;
    case DEMON_HUNTER_VENGEANCE:
      resources.base[ RESOURCE_PAIN ] = 100;
      break;
    default:
      break;
  }

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in
  // player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// demon_hunter_t::init_procs ===============================================

void demon_hunter_t::init_procs()
{
  base_t::init_procs();

  // General
  proc.delayed_aa_range       = get_proc( "delayed_swing__out_of_range" );
  proc.delayed_aa_channel     = get_proc( "delayed_swing__channeling" );
  proc.soul_fragment_greater  = get_proc( "soul_fragment_greater" );
  proc.soul_fragment_lesser   = get_proc( "soul_fragment_lesser" );
  proc.felblade_reset         = get_proc( "felblade_reset" );

  // Havoc
  proc.demon_blades_wasted        = get_proc( "demon_blades_wasted" );
  proc.demonic_appetite           = get_proc( "demonic_appetite" );
  proc.demons_bite_in_meta        = get_proc( "demons_bite_in_meta" );
  proc.chaos_strike_in_dark_slash = get_proc( "chaos_strike_in_dark_slash" );
  proc.annihilation_in_dark_slash = get_proc( "annihilation_in_dark_slash" );

  // Vengeance
  proc.gluttony                     = get_proc( "gluttony" );
  proc.soul_fragment_expire         = get_proc( "soul_fragment_expire" );
  proc.soul_fragment_overflow       = get_proc( "soul_fragment_overflow" );
  proc.soul_fragment_from_shear     = get_proc( "soul_fragment_from_shear" );
  proc.soul_fragment_from_fracture  = get_proc( "soul_fragment_from_fracture" );
  proc.soul_fragment_from_fallout   = get_proc( "soul_fragment_from_fallout" );
}

// demon_hunter_t::init_resources ===========================================

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.current[ RESOURCE_FURY ] = initial_fury;
  resources.current[ RESOURCE_PAIN ] = 0;
  expected_max_health = calculate_expected_max_health();
}

// demon_hunter_t::init_rng =================================================

void demon_hunter_t::init_rng()
{
  // RPPM objects

  // General
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    rppm.felblade = get_rppm( "felblade", find_spell( 236167 ) );
    rppm.demonic_appetite = get_rppm( "demonic_appetite ", talent.demonic_appetite );
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    rppm.felblade = get_rppm( "felblade", find_spell( 203557 ) );
    rppm.gluttony = get_rppm( "gluttony", talent.gluttony );
    // 6/27/2018 -- Removed from spell data, but still seems to be RPPM
    if ( rppm.gluttony->get_frequency() == 0 )
      rppm.gluttony->set_frequency( 1.0 );
  }

  player_t::init_rng();
}

// demon_hunter_t::init_scaling =============================================

void demon_hunter_t::init_scaling()
{
  base_t::init_scaling();

  scaling->enable( STAT_WEAPON_OFFHAND_DPS );

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
    scaling->enable( STAT_BONUS_ARMOR );

  scaling->disable( STAT_STRENGTH );
}

// demon_hunter_t::init_spells ==============================================

void demon_hunter_t::init_spells()
{
  base_t::init_spells();

  // Specialization =========================================================

  // General
  spec.demon_hunter           = find_class_spell( "Demon Hunter" );
  spec.consume_magic          = find_class_spell( "Consume Magic" );
  spec.chaos_brand            = find_spell( 255260 );
  spec.critical_strikes       = find_spell( 221351 );
  spec.demonic_wards          = find_specialization_spell( "Demonic Wards" ); // Two different spells with the same name
  spec.disrupt                = find_class_spell( "Disrupt" );
  spec.soul_fragment          = find_spell( 204255 );

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    spec.consume_soul_greater   = find_spell( 178963 );
    spec.consume_soul_lesser    = spec.consume_soul_greater;
    spec.immolation_aura        = find_talent_spell( "Immolation Aura" );
    spec.leather_specialization = find_spell( 178976 );
    spec.metamorphosis          = find_class_spell( "Metamorphosis" );
    spec.metamorphosis_buff     = spec.metamorphosis->effectN( 2 ).trigger();
  }
  else
  {
    spec.consume_soul_greater   = find_spell( 210042 );
    spec.consume_soul_lesser    = find_spell( 203794 );
    spec.immolation_aura        = find_specialization_spell( "Immolation Aura" );
    spec.leather_specialization = find_spell( 226359 );
    spec.metamorphosis          = find_specialization_spell( "Metamorphosis" );
    spec.metamorphosis_buff     = spec.metamorphosis;
  }

  // Havoc
  spec.havoc                  = find_specialization_spell( "Havoc Demon Hunter" );
  spec.blade_dance            = find_class_spell( "Blade Dance",      DEMON_HUNTER_HAVOC );
  spec.chaos_nova             = find_class_spell( "Chaos Nova",       DEMON_HUNTER_HAVOC );
  spec.chaos_strike           = find_class_spell( "Chaos Strike",     DEMON_HUNTER_HAVOC );
  spec.eye_beam               = find_class_spell( "Eye Beam",         DEMON_HUNTER_HAVOC );
  spec.vengeful_retreat       = find_class_spell( "Vengeful Retreat", DEMON_HUNTER_HAVOC );
  spec.annihilation           = find_spell( 201427, DEMON_HUNTER_HAVOC );
  spec.blur                   = find_spell( 198589, DEMON_HUNTER_HAVOC );
  spec.chaos_strike_refund    = find_spell( 197125, DEMON_HUNTER_HAVOC );
  spec.chaos_strike_fury      = find_spell( 193840, DEMON_HUNTER_HAVOC );
  spec.death_sweep            = find_spell( 210152, DEMON_HUNTER_HAVOC );
  spec.demonic_appetite_fury  = find_spell( 210041, DEMON_HUNTER_HAVOC );
  spec.fel_rush_damage        = find_spell( 192611, DEMON_HUNTER_HAVOC );  
  spec.momentum_buff          = find_spell( 208628, DEMON_HUNTER_HAVOC );

  // Vengeance
  spec.vengeance              = find_specialization_spell( "Vengeance Demon Hunter" );
  spec.demon_spikes           = find_specialization_spell( "Demon Spikes" );
  spec.riposte                = find_specialization_spell( "Riposte" );
  spec.soul_cleave            = find_specialization_spell( "Soul Cleave" );
  spec.fiery_brand_dr         = find_spell( 207744, DEMON_HUNTER_VENGEANCE );

  // Masteries ==============================================================

  mastery.demonic_presence    = find_mastery_spell( DEMON_HUNTER_HAVOC );
  mastery.fel_blood           = find_mastery_spell( DEMON_HUNTER_VENGEANCE );

  // Talents ================================================================

  // General
  talent.felblade             = find_talent_spell( "Felblade" );
  talent.soul_rending         = find_talent_spell( "Soul Rending" );

  // Havoc
  talent.blind_fury           = find_talent_spell( "Blind Fury" );
  talent.demonic_appetite     = find_talent_spell( "Demonic Appetite" );
  // talent.felblade

  talent.insatiable_hunger    = find_talent_spell( "Insatiable Hunger" );
  talent.demon_blades         = find_talent_spell( "Demon Blades" );
  talent.immolation_aura      = find_talent_spell( "Immolation Aura" );

  talent.trail_of_ruin        = find_talent_spell( "Trail of Ruin" );
  talent.fel_mastery          = find_talent_spell( "Fel Mastery" );
  talent.fel_barrage          = find_talent_spell( "Fel Barrage" );

  // talent.soul_rending
  talent.desperate_instincts  = find_talent_spell( "Desperate Instincts" );
  talent.netherwalk           = find_talent_spell( "Netherwalk" );

  talent.cycle_of_hatred      = find_talent_spell( "Cycle of Hatred" );
  talent.first_blood          = find_talent_spell( "First Blood" );
  talent.dark_slash           = find_talent_spell( "Dark Slash" );

  talent.unleashed_power      = find_talent_spell( "Unleashed Power" );
  talent.master_of_the_glaive = find_talent_spell( "Master of the Glaive" );
  talent.fel_eruption         = find_talent_spell( "Fel Eruption" );

  talent.demonic              = find_talent_spell( "Demonic" );
  talent.momentum             = find_talent_spell( "Momentum" );
  talent.nemesis              = find_talent_spell( "Nemesis" );

  // Vengeance
  talent.abyssal_strike       = find_talent_spell( "Abyssal Strike" );
  talent.agonizing_flames     = find_talent_spell( "Agonizing Flames" );
  talent.razor_spikes         = find_talent_spell( "Razor Spikes" );

  talent.feast_of_souls       = find_talent_spell( "Feast of Souls" );
  talent.fallout              = find_talent_spell( "Fallout" );
  talent.burning_alive        = find_talent_spell( "Burning Alive" );
 
  talent.flame_crash          = find_talent_spell( "Flame Crash" );
  talent.charred_flesh        = find_talent_spell( "Charred Flesh" );
  // talent.felblade

  // talent.soul_rending
  talent.feed_the_demon       = find_talent_spell( "Feed the Demon" );
  talent.fracture             = find_talent_spell( "Fracture" );

  talent.concentrated_sigils  = find_talent_spell( "Concentrated Sigils" );
  talent.sigil_of_chains      = find_talent_spell( "Sigil of Chains" );
  talent.quickened_sigils     = find_talent_spell( "Quickened Sigils" );

  talent.gluttony             = find_talent_spell( "Gluttony" );
  talent.fel_devastation      = find_talent_spell( "Fel Devastation" );
  talent.spirit_bomb          = find_talent_spell( "Spirit Bomb" );

  talent.last_resort          = find_talent_spell( "Last Resort" );
  talent.void_reaver          = find_talent_spell( "Void Reaver" );
  talent.soul_barrier         = find_talent_spell( "Soul Barrier" );

  // Azerite ================================================================

  // Havoc
  azerite.chaotic_transformation  = find_azerite_spell( "Chaotic Transformation" );
  azerite.eyes_of_rage            = find_azerite_spell( "Eyes of Rage" );
  azerite.furious_gaze            = find_azerite_spell( "Furious Gaze" );
  azerite.revolving_blades        = find_azerite_spell( "Revolving Blades" );
  azerite.seething_power          = find_azerite_spell( "Seething Power" );
  azerite.thirsting_blades        = find_azerite_spell( "Thirsting Blades" );

  // Spell Initialization ===================================================

  using namespace actions::attacks;
  using namespace actions::spells;
  using namespace actions::heals;

  active.consume_soul_greater 
    = new consume_soul_t(this, "consume_soul_greater", spec.consume_soul_greater, soul_fragment::GREATER);
  active.consume_soul_lesser 
    = new consume_soul_t(this, "consume_soul_lesser", spec.consume_soul_lesser, soul_fragment::LESSER);

  if ( talent.demon_blades->ok() )
  {
    active.demon_blades = new demon_blades_t( this );
  }

  sigil_delay = find_specialization_spell( "Sigil of Flame" )->duration();

  if ( talent.quickened_sigils->ok() )
  {
    sigil_delay += talent.quickened_sigils->effectN( 1 ).time_value();
  }
}

// demon_hunter_t::invalidate_cache =========================================

void demon_hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( mastery.demonic_presence->ok() )
        invalidate_cache( CACHE_RUN_SPEED );
      if ( mastery.fel_blood->ok() )
        invalidate_cache( CACHE_ARMOR );
      break;
    case CACHE_CRIT_CHANCE:
      if ( spec.riposte->ok() )
        invalidate_cache( CACHE_PARRY );
      break;
    case CACHE_RUN_SPEED:
      adjust_movement();
      break;
    case CACHE_AGILITY:
      if ( buff.demon_spikes->check() )
        invalidate_cache( CACHE_ARMOR );
      break;
    default:
      break;
  }
}

// demon_hunter_t::primary_resource =========================================

resource_e demon_hunter_t::primary_resource() const
{
  switch (specialization())
  {
    case DEMON_HUNTER_HAVOC:
      return RESOURCE_FURY;
    case DEMON_HUNTER_VENGEANCE:
      return RESOURCE_PAIN;
    default:
      return RESOURCE_NONE;
  }
}

// demon_hunter_t::primary_role =============================================

role_e demon_hunter_t::primary_role() const
{
  switch (specialization())
  {
    case DEMON_HUNTER_HAVOC:
      return ROLE_ATTACK;
    case DEMON_HUNTER_VENGEANCE:
      return ROLE_TANK;
    default:
      return ROLE_NONE;
  }
}

// demon_hunter_t::default_flask ===================================================

std::string demon_hunter_t::default_flask() const
{
  return (true_level >  110) ? "currents" :
         (true_level >  100) ? "seventh_demon" :
         (true_level >= 90) ? "greater_draenic_agility_flask" :
         (true_level >= 85) ? "spring_blossoms" :
         (true_level >= 80) ? "winds" : "disabled";
}

// demon_hunter_t::default_potion ==================================================

std::string demon_hunter_t::default_potion() const
{
  return (true_level > 110) ? (specialization() == DEMON_HUNTER_HAVOC ? "battle_potion_of_agility" : "steelskin_potion") :
         (true_level > 100) ? (specialization() == DEMON_HUNTER_HAVOC ? "prolonged_power" : "unbending_potion") :
         (true_level >= 90) ? (specialization() == DEMON_HUNTER_HAVOC ? "draenic_agility" : "draenic_versatility") :
         (true_level >= 85) ? "virmens_bite" :
         (true_level >= 80) ? "tolvir" : "disabled";
}

// demon_hunter_t::default_food ====================================================

std::string demon_hunter_t::default_food() const
{
  return (true_level >  110) ? "bountiful_captains_feast" :
         (true_level >  100) ? "lavish_suramar_feast" :
         (true_level >  90) ? "pickled_eel" :
         (true_level >= 90) ? "sea_mist_rice_noodles" :
         (true_level >= 80) ? "seafood_magnifique_feast" : "disabled";
}

// demon_hunter_t::default_rune ====================================================

std::string demon_hunter_t::default_rune() const
{
  return (true_level >= 120) ? "battle_scarred" :
         (true_level >= 110) ? "defiled" :
         (true_level >= 100) ? "hyper" : "disabled";
}

// ==========================================================================
// custom demon_hunter_t init functions
// ==========================================================================

// demon_hunter_t::apl_precombat ============================================

void demon_hunter_t::apl_precombat()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  pre->add_action( "flask" );
  pre->add_action( "augmentation" );
  pre->add_action( "food" );

  // Snapshot Stats
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  pre->add_action( "potion" );

  if (specialization() == DEMON_HUNTER_HAVOC)
  {
    pre->add_action( this, "Metamorphosis", "if=!azerite.chaotic_transformation.enabled" );
  }
}

// demon_hunter_t::apl_default ==============================================

void demon_hunter_t::apl_default()
{
  // action_priority_list_t* def = get_action_priority_list( "default" );
}

// demon_hunter_t::apl_havoc ================================================

void add_havoc_use_items( demon_hunter_t* p, action_priority_list_t* apl )
{
  // On-Use Items
  for ( auto& item : p->items )
  {
    auto effect = item.special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
    if ( effect && effect->source != SPECIAL_EFFECT_SOURCE_ADDON )
    {
      if ( util::str_compare_ci( item.name_str, "galecallers_boon" ) )
      {
        std::string line1 = std::string( "use_item,name=" ) + item.name_str + std::string( ",sync=fel_barrage" );
        std::string line2 =
            std::string( "use_item,name=" ) + item.name_str + std::string( ",if=!talent.fel_barrage.enabled" );

        apl->add_action( line1 );
        apl->add_action( line2 );
      }
      else 
      {
        std::string line = std::string( "use_item,name=" ) + item.name_str;
        apl->add_action( line );
      }
    }
  }
}

void demon_hunter_t::apl_havoc()
{
  action_priority_list_t* apl_default = get_action_priority_list( "default" );
  apl_default->add_action( "auto_attack" );
  apl_default->add_action( "variable,name=blade_dance,value=talent.first_blood.enabled|spell_targets.blade_dance1>=(3-talent.trail_of_ruin.enabled)" );
  apl_default->add_action( "variable,name=waiting_for_nemesis,value=!(!talent.nemesis.enabled|cooldown.nemesis.ready|cooldown.nemesis.remains>target.time_to_die|cooldown.nemesis.remains>60)" );
  apl_default->add_action( "variable,name=pooling_for_meta,value=!talent.demonic.enabled&cooldown.metamorphosis.remains<6&fury.deficit>30&(!variable.waiting_for_nemesis|cooldown.nemesis.remains<10)" );
  apl_default->add_action( "variable,name=pooling_for_blade_dance,value=variable.blade_dance&(fury<75-talent.first_blood.enabled*20)" );
  apl_default->add_action( "variable,name=pooling_for_eye_beam,value=talent.demonic.enabled&!talent.blind_fury.enabled&cooldown.eye_beam.remains<(gcd.max*2)&fury.deficit>20" );
  apl_default->add_action( "variable,name=waiting_for_dark_slash,value=talent.dark_slash.enabled&!variable.pooling_for_blade_dance&!variable.pooling_for_meta&cooldown.dark_slash.up" );
  apl_default->add_action( "variable,name=waiting_for_momentum,value=talent.momentum.enabled&!buff.momentum.up" );
  apl_default->add_action( this, "Disrupt" );
  apl_default->add_action( "call_action_list,name=cooldown,if=gcd.remains=0" );
  apl_default->add_action( "pick_up_fragment,if=fury.deficit>=35" );
  apl_default->add_action( "call_action_list,name=dark_slash,if=talent.dark_slash.enabled&(variable.waiting_for_dark_slash|debuff.dark_slash.up)" );
  apl_default->add_action( "run_action_list,name=demonic,if=talent.demonic.enabled" );
  apl_default->add_action( "run_action_list,name=normal" );
  
  action_priority_list_t* apl_cooldown = get_action_priority_list( "cooldown" );
  apl_cooldown->add_action( this, "Metamorphosis", "if=!(talent.demonic.enabled|variable.pooling_for_meta|variable.waiting_for_nemesis)|target.time_to_die<25" );
  apl_cooldown->add_action( this, "Metamorphosis", "if=talent.demonic.enabled&(!azerite.chaotic_transformation.enabled|(cooldown.eye_beam.remains>20&cooldown.blade_dance.remains>gcd.max))" );
  apl_cooldown->add_talent( this, "Nemesis", "target_if=min:target.time_to_die,if=raid_event.adds.exists&debuff.nemesis.down&(active_enemies>desired_targets|raid_event.adds.in>60)" );
  apl_cooldown->add_talent( this, "Nemesis", "if=!raid_event.adds.exists" );
  apl_cooldown->add_action( "potion,if=buff.metamorphosis.remains>25|target.time_to_die<60" );
  add_havoc_use_items(this, apl_cooldown);

  action_priority_list_t* apl_normal = get_action_priority_list( "normal" );
  apl_normal->add_action( this, "Vengeful Retreat", "if=talent.momentum.enabled&buff.prepared.down&time>1" );
  apl_normal->add_action( this, "Fel Rush", "if=(variable.waiting_for_momentum|talent.fel_mastery.enabled)&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  apl_normal->add_talent( this, "Fel Barrage", "if=!variable.waiting_for_momentum&(active_enemies>desired_targets|raid_event.adds.in>30)" );
  apl_normal->add_action( this, spec.death_sweep, "death_sweep", "if=variable.blade_dance" );
  apl_normal->add_talent( this, "Immolation Aura" );
  apl_normal->add_action( this, "Eye Beam", "if=active_enemies>1&(!raid_event.adds.exists|raid_event.adds.up)&!variable.waiting_for_momentum" );
  apl_normal->add_action( this, "Blade Dance", "if=variable.blade_dance" );
  apl_normal->add_talent( this, "Felblade", "if=fury.deficit>=40" );
  apl_normal->add_action( this, "Eye Beam", "if=!talent.blind_fury.enabled&!variable.waiting_for_dark_slash&raid_event.adds.in>cooldown" );
  apl_normal->add_action( this, spec.annihilation, "annihilation", "if=(talent.demon_blades.enabled|!variable.waiting_for_momentum|fury.deficit<30|buff.metamorphosis.remains<5)"
                                                                   "&!variable.pooling_for_blade_dance&!variable.waiting_for_dark_slash" );
  apl_normal->add_action( this, "Chaos Strike", "if=(talent.demon_blades.enabled|!variable.waiting_for_momentum|fury.deficit<30)"
                                                "&!variable.pooling_for_meta&!variable.pooling_for_blade_dance&!variable.waiting_for_dark_slash" );
  apl_normal->add_action( this, "Eye Beam", "if=talent.blind_fury.enabled&raid_event.adds.in>cooldown" );
  apl_normal->add_action( this, "Demon's Bite" );
  apl_normal->add_action( this, "Fel Rush", "if=!talent.momentum.enabled&raid_event.movement.in>charges*10&talent.demon_blades.enabled" );
  apl_normal->add_talent( this, "Felblade", "if=movement.distance>15|buff.out_of_range.up" );
  apl_normal->add_action( this, "Fel Rush", "if=movement.distance>15|(buff.out_of_range.up&!talent.momentum.enabled)" );
  apl_normal->add_action( this, "Vengeful Retreat", "if=movement.distance>15" );
  apl_normal->add_action( this, "Throw Glaive", "if=talent.demon_blades.enabled" );

  action_priority_list_t* apl_demonic = get_action_priority_list( "demonic" );
  apl_demonic->add_action( this, spec.death_sweep, "death_sweep", "if=variable.blade_dance" );
  apl_demonic->add_action( this, "Eye Beam", "if=raid_event.adds.up|raid_event.adds.in>25" );
  apl_demonic->add_talent( this, "Fel Barrage", "if=((!cooldown.eye_beam.up|buff.metamorphosis.up)&raid_event.adds.in>30)|active_enemies>desired_targets" );
  apl_demonic->add_action( this, "Blade Dance", "if=variable.blade_dance&!cooldown.metamorphosis.ready"
                                                "&(cooldown.eye_beam.remains>(5-azerite.revolving_blades.rank*3)|(raid_event.adds.in>cooldown&raid_event.adds.in<25))" );
  apl_demonic->add_talent( this, "Immolation Aura" );
  apl_demonic->add_action( this, spec.annihilation, "annihilation", "if=!variable.pooling_for_blade_dance" );
  apl_demonic->add_talent( this, "Felblade", "if=fury.deficit>=40" );
  apl_demonic->add_action( this, "Chaos Strike", "if=!variable.pooling_for_blade_dance&!variable.pooling_for_eye_beam" );
  apl_demonic->add_action( this, "Fel Rush", "if=talent.demon_blades.enabled&!cooldown.eye_beam.ready&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  apl_demonic->add_action( this, "Demon's Bite" );
  apl_demonic->add_action( this, "Throw Glaive", "if=buff.out_of_range.up" );
  apl_demonic->add_action( this, "Fel Rush", "if=movement.distance>15|buff.out_of_range.up" );
  apl_demonic->add_action( this, "Vengeful Retreat", "if=movement.distance>15" );
  apl_demonic->add_action( this, "Throw Glaive", "if=talent.demon_blades.enabled" );

  action_priority_list_t* apl_dark_slash = get_action_priority_list( "dark_slash" );
  apl_dark_slash->add_talent( this, "Dark Slash", "if=fury>=80&(!variable.blade_dance|!cooldown.blade_dance.ready)" );
  apl_dark_slash->add_action( this, spec.annihilation, "annihilation", "if=debuff.dark_slash.up" );
  apl_dark_slash->add_action( this, "Chaos Strike", "if=debuff.dark_slash.up" );
}

// demon_hunter_t::apl_vengeance ============================================

void demon_hunter_t::apl_vengeance()
{
  action_priority_list_t* apl_default = get_action_priority_list( "default" );

  apl_default->add_action( "auto_attack" );
  apl_default->add_action( this, "Consume Magic" );
  
  // On-use items
  for ( auto& item : items )
  {
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      apl_default->add_action( "use_item,slot=" + std::string( item.slot_name() ), ",if=!raid_event.adds.exists|active_enemies>1" );
    }
  }

  apl_default->add_action( "call_action_list,name=brand,if=talent.charred_flesh.enabled" );
  apl_default->add_action( "call_action_list,name=defensives" );
  apl_default->add_action( "call_action_list,name=normal" );

  action_priority_list_t* apl_defensives = get_action_priority_list( "defensives", "Defensives" );
  apl_defensives->add_action( this, "Demon Spikes" );
  // apl_defensives->add_talent( this, "Soul Barrier" );
  apl_defensives->add_action( this, "Metamorphosis" );
  apl_defensives->add_action( this, "Fiery Brand" );

  action_priority_list_t* apl_brand = get_action_priority_list( "brand", "Fiery Brand Rotation" );
  apl_brand->add_action( this, "Sigil of Flame", "if=cooldown.fiery_brand.remains<2" );
  apl_brand->add_action( this, "Infernal Strike", "if=cooldown.fiery_brand.remains=0" );
  apl_brand->add_action( this, "Fiery Brand" );
  apl_brand->add_action( this, "Immolation Aura", "if=dot.fiery_brand.ticking" );
  apl_brand->add_talent( this, "Fel Devastation", "if=dot.fiery_brand.ticking" );
  apl_brand->add_action( this, "Infernal Strike", "if=dot.fiery_brand.ticking" );
  apl_brand->add_action( this, "Sigil of Flame", "if=dot.fiery_brand.ticking" );

  action_priority_list_t* apl_normal = get_action_priority_list( "normal", "Normal Rotation" );
  apl_normal->add_action( this, "Infernal Strike" );
  apl_normal->add_talent( this, "Spirit Bomb", "if=soul_fragments>=4" );
  apl_normal->add_action( this, "Soul Cleave", "if=!talent.spirit_bomb.enabled" );
  apl_normal->add_action( this, "Soul Cleave", "if=talent.spirit_bomb.enabled&soul_fragments=0" );
  apl_normal->add_action( this, "Immolation Aura", "if=pain<=90" );
  apl_normal->add_talent( this, "Felblade", "if=pain<=70" );
  apl_normal->add_talent( this, "Fracture", "if=soul_fragments<=3" );
  apl_normal->add_talent( this, "Fel Devastation" );
  apl_normal->add_action( this, "Sigil of Flame" );
  apl_normal->add_action( this, "Shear" );
  apl_normal->add_action( this, "Throw Glaive" );
}

// demon_hunter_t::create_cooldowns =========================================

void demon_hunter_t::create_cooldowns()
{
  // General
  cooldown.consume_magic        = get_cooldown( "consume_magic" );
  cooldown.disrupt              = get_cooldown( "disrupt" );
  cooldown.felblade             = get_cooldown( "felblade" );
  cooldown.fel_eruption         = get_cooldown( "fel_eruption" );

  // Havoc
  cooldown.blade_dance          = get_cooldown( "blade_dance" );
  cooldown.blur                 = get_cooldown( "blur" );
  cooldown.chaos_nova           = get_cooldown( "chaos_nova" );
  cooldown.dark_slash           = get_cooldown( "dark_slash" );
  cooldown.eye_beam             = get_cooldown( "eye_beam" );
  cooldown.fel_barrage          = get_cooldown( "fel_barrage" );
  cooldown.fel_rush             = get_cooldown( "fel_rush" );
  cooldown.metamorphosis        = get_cooldown( "metamorphosis" );
  cooldown.nemesis              = get_cooldown( "nemesis" );
  cooldown.netherwalk           = get_cooldown( "netherwalk" );
  cooldown.throw_glaive         = get_cooldown( "throw_glaive" );
  cooldown.vengeful_retreat     = get_cooldown( "vengeful_retreat" );
  cooldown.movement_shared      = get_cooldown( "movement_shared" );

  // Vengeance
  cooldown.demon_spikes         = get_cooldown( "demon_spikes" );
  cooldown.fiery_brand          = get_cooldown( "fiery_brand" );
  cooldown.sigil_of_chains      = get_cooldown( "sigil_of_chains" );
  cooldown.sigil_of_flame       = get_cooldown( "sigil_of_flame" );
  cooldown.sigil_of_misery      = get_cooldown( "sigil_of_misery" );
  cooldown.sigil_of_silence     = get_cooldown( "sigil_of_silence" );
}

// demon_hunter_t::create_gains =============================================

void demon_hunter_t::create_gains()
{
  // General
  gain.miss_refund              = get_gain( "miss_refund" );

  // Havoc
  gain.blind_fury               = get_gain("blind_fury");
  gain.demonic_appetite         = get_gain("demonic_appetite");
  gain.prepared                 = get_gain("prepared");
  
  // Vengeance
  gain.metamorphosis            = get_gain("metamorphosis");

  // Azerite
  gain.thirsting_blades         = get_gain( "thirsting_blades" );
  gain.revolving_blades         = get_gain( "revolving_blades" );
}

// demon_hunter_t::create_benefits ==========================================

void demon_hunter_t::create_benefits()
{
}

// ==========================================================================
// overridden player_t stat functions
// ==========================================================================

// demon_hunter_t::composite_armor ==========================================

double demon_hunter_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buff.demon_spikes->up() )
  {
    const double mastery_value = cache.mastery() * mastery.fel_blood ->effectN( 1 ).mastery_value();
    a += ( buff.demon_spikes->data().effectN( 2 ).percent() + mastery_value ) * cache.agility();
  }

  return a;
}

// demon_hunter_t::composite_armor_multiplier ===============================

double demon_hunter_t::composite_armor_multiplier() const
{
  double am = player_t::composite_armor_multiplier();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    am *= 1.0 + spec.demonic_wards->effectN( 5 ).percent();

    if ( buff.metamorphosis->check() )
    {
      am *= 1.0 + spec.metamorphosis_buff->effectN( 7 ).percent();
    }
  }

  return am;
}

// demon_hunter_t::composite_attack_power_multiplier ========================

double demon_hunter_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  ap *= 1.0 + cache.mastery() * mastery.fel_blood->effectN( 2 ).mastery_value();

  return ap;
}

// demon_hunter_t::composite_attribute_multiplier ===========================

double demon_hunter_t::composite_attribute_multiplier( attribute_e a ) const
{
  double am = player_t::composite_attribute_multiplier( a );

  switch ( a )
  {
    case ATTR_STAMINA:
      if ( specialization() == DEMON_HUNTER_VENGEANCE )
      {
        am *= 1.0 + spec.demonic_wards->effectN( 4 ).percent();
      }
      break;
    default:
      break;
  }

  return am;
}

// demon_hunter_t::composite_crit_avoidance =================================

double demon_hunter_t::composite_crit_avoidance() const
{
  double ca = player_t::composite_crit_avoidance();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    ca += spec.demonic_wards->effectN( 2 ).percent();
  }

  return ca;
}

// demon_hunter_t::composite_dodge ==========================================

double demon_hunter_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  d += buff.blade_dance->check_value();
  d += buff.death_sweep->check_value();

  d += buff.blur->check() * buff.blur->data().effectN( 2 ).percent();

  return d;
}

// demon_hunter_t::composite_melee_haste  ===================================

double demon_hunter_t::composite_melee_haste() const
{
  double mh = player_t::composite_melee_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    mh /= 1.0 + buff.metamorphosis->check_value();
  }

  return mh;
}

// demon_hunter_t::composite_spell_haste ====================================

double demon_hunter_t::composite_spell_haste() const
{
  double sh = player_t::composite_spell_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    sh /= 1.0 + buff.metamorphosis->check_value();
  }

  return sh;
}

// demon_hunter_t::composite_leech ==========================================

double demon_hunter_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( buff.metamorphosis->check() )
  {
    if ( specialization() == DEMON_HUNTER_HAVOC )
    {
      l += spec.metamorphosis_buff->effectN( 3 ).percent();
    }

    l += talent.soul_rending->effectN( 2 ).percent();
  }

  if ( talent.soul_rending->ok() )
  {
    l += talent.soul_rending->effectN( 1 ).percent();
  }

  return l;
}

// demon_hunter_t::composite_melee_crit_chance ==============================

double demon_hunter_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  mc += spec.critical_strikes->effectN( 1 ).percent();

  return mc;
}

// demon_hunter_t::composite_melee_expertise ================================

double demon_hunter_t::composite_melee_expertise( const weapon_t* w ) const
{
  double me = player_t::composite_melee_expertise( w );

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    me += spec.demonic_wards->effectN( 3 ).base_value();
  }

  return me;
}

// demon_hunter_t::composite_parry ==========================================

double demon_hunter_t::composite_parry() const
{
  double cp = player_t::composite_parry();

  cp += buff.demon_spikes->check_value();

  return cp;
}

// demon_hunter_t::composite_parry_rating() =================================

double demon_hunter_t::composite_parry_rating() const
{
  double pr = player_t::composite_parry_rating();

  if ( spec.riposte->ok() )
  {
    pr += composite_melee_crit_rating();
  }

  return pr;
}

// demon_hunter_t::composite_player_multiplier ==============================

double demon_hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && buff.demon_spikes->check() )
  {
    m *= 1.0 + talent.razor_spikes->effectN( 1 ).percent();
  }

  return m;
}

// demon_hunter_t::composite_player_dd_multiplier ===========================

double demon_hunter_t::composite_player_dd_multiplier( school_e school, const action_t * a ) const
{
  double m = player_t::composite_player_dd_multiplier(school, a);

  if ( buff.nemesis->check() && a->target->race == buff.nemesis->current_value )
  {
    m *= 1.0 + buff.nemesis->data().effectN( 1 ).percent();
  }

  if ( a->target->race == RACE_DEMON && buff.demon_soul->check() )
  {
    m *= 1.0 + buff.demon_soul->value();
  }

  return m;
}

// demon_hunter_t::composite_player_td_multiplier ===========================

double demon_hunter_t::composite_player_td_multiplier( school_e school, const action_t * a ) const
{
  double m = player_t::composite_player_td_multiplier( school, a );

  if ( buff.nemesis->check() && a->target->race == buff.nemesis->current_value )
  {
    m *= 1.0 + buff.nemesis->data().effectN( 1 ).percent();
  }

  if ( a->target->race == RACE_DEMON && buff.demon_soul->check() )
  {
    m *= 1.0 + buff.demon_soul->value();
  }

  return m;
}

// demon_hunter_t::composite_player_target_multiplier =======================

double demon_hunter_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    m *= 1.0 + get_target_data( target )->debuffs.nemesis->check_value();
  }

  return m;
}

// demon_hunter_t::composite_spell_crit_chance ==============================

double demon_hunter_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  sc += spec.critical_strikes->effectN( 1 ).percent();

  return sc;
}

// demon_hunter_t::matching_gear_multiplier =================================

double demon_hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( specialization() == DEMON_HUNTER_HAVOC && attr == ATTR_AGILITY ) ||
       ( specialization() == DEMON_HUNTER_VENGEANCE && attr == ATTR_STAMINA ) )
  {
    return spec.leather_specialization->effectN( 1 ).percent();
  }

  return 0.0;
}

// demon_hunter_t::passive_movement_modifier ================================

double demon_hunter_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( mastery.demonic_presence->ok() )
  {
    ms += cache.mastery() * mastery.demonic_presence->effectN( 2 ).mastery_value();
  }

  return ms;
}

// demon_hunter_t::temporary_movement_modifier ==============================

double demon_hunter_t::temporary_movement_modifier() const
{
  double ms = player_t::temporary_movement_modifier();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    ms = std::max( ms, buff.immolation_aura->value() );
  }

  return ms;
}

// ==========================================================================
// overridden player_t combat functions
// ==========================================================================

// demon_hunter_t::expected_max_health() ====================================

double demon_hunter_t::calculate_expected_max_health() const
{
  double slot_weights = 0;
  double prop_values = 0;
  for ( auto& item : items )
  {
    if ( item.slot == SLOT_SHIRT || item.slot == SLOT_RANGED ||
         item.slot == SLOT_TABARD || item.item_level() <= 0 )
    {
      continue;
    }

    const random_prop_data_t item_data = dbc.random_property(item.item_level());
    int index = item_database::random_suffix_type(&item.parsed.data);
    if ( item_data.p_epic[0] == 0 )
    {
      continue;
    }
    slot_weights += item_data.p_epic[index] / item_data.p_epic[0];

    if (!item.active())
    {
      continue;
    }

    prop_values += item_data.p_epic[index];
  }

  double expected_health = (prop_values / slot_weights) * 8.318556;
  expected_health += base.stats.attribute[STAT_STAMINA];
  expected_health *= 1 + matching_gear_multiplier(ATTR_STAMINA);
  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    expected_health *= 1 + spec.demonic_wards->effectN( 4 ).percent();
  }
  expected_health *= current.health_per_stamina;
  return expected_health;
}

// demon_hunter_t::assess_damage ============================================

void demon_hunter_t::assess_damage( school_e school, dmg_e dt,
                                    action_state_t* s )
{
  player_t::assess_damage( school, dt, s );

  // Benefit tracking
  if ( s->action->may_parry )
  {
    buff.demon_spikes->up();
  }
}

// demon_hunter_t::combat_begin =============================================

void demon_hunter_t::combat_begin()
{
  player_t::combat_begin();

  // Start event drivers
  if ( talent.spirit_bomb->ok() )
  {
    spirit_bomb_driver = make_event<spirit_bomb_event_t>( *sim, this, true );
  }

  buff.thirsting_blades->trigger( buff.thirsting_blades->data().max_stacks() );
  buff.thirsting_blades_driver->trigger();
}

// demon_hunter_t::interrupt ================================================

void demon_hunter_t::interrupt()
{
  event_t::cancel( soul_fragment_pick_up );
  base_t::interrupt();
}

// demon_hunter_t::recalculate_resource_max =================================

void demon_hunter_t::recalculate_resource_max( resource_e r )
{
  player_t::recalculate_resource_max( r );

  if ( r == RESOURCE_HEALTH )
  {
    // Update Metamorphosis' value for the new health amount.
    if ( specialization() == DEMON_HUNTER_VENGEANCE && buff.metamorphosis->check() )
    {
      assert( metamorphosis_health > 0 );

      double base_health = max_health() - metamorphosis_health;
      double new_health  = base_health * buff.metamorphosis->check_value();
      double diff        = new_health - metamorphosis_health;

      if ( diff != 0.0 )
      {
        if ( sim->debug )
        {
          sim->out_debug.printf(
            "%s adjusts %s temporary health. old=%.0f new=%.0f diff=%.0f",
            name(), buff.metamorphosis->name(), metamorphosis_health,
            new_health, diff );
        }

        resources.max[ RESOURCE_HEALTH ] += diff;
        resources.temporary[ RESOURCE_HEALTH ] += diff;
        if ( diff > 0 )
        {
          resource_gain( RESOURCE_HEALTH, diff );
        }
        else if ( diff < 0 )
        {
          resource_loss( RESOURCE_HEALTH, -diff );
        }

        metamorphosis_health += diff;
      }
    }
  }
}

// demon_hunter_t::target_mitigation ========================================

void demon_hunter_t::target_mitigation( school_e school, dmg_e dt, action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( s->result_amount <= 0 )
  {
    return;
  }

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    s->result_amount *= 1.0 + buff.blur->value();

    if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
    {
      s->result_amount *= 1.0 + spec.demonic_wards->effectN( 1 ).percent();
    }
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    s->result_amount *= 1.0 + spec.demonic_wards->effectN( 1 ).percent();

    const demon_hunter_td_t* td = get_target_data( s->action->player );
    if ( td->dots.fiery_brand && td->dots.fiery_brand->is_ticking() )
    {
      s->result_amount *= 1.0 + spec.fiery_brand_dr->effectN( 1 ).percent();
    }

    if ( td->debuffs.void_reaver )
    {
      s->result_amount *= 1.0 + td->debuffs.void_reaver->stack_value();
    }
  }
}

// demon_hunter_t::reset ====================================================

void demon_hunter_t::reset()
{
  base_t::reset();

  soul_fragment_pick_up   = nullptr;
  spirit_bomb_driver      = nullptr;
  exit_melee_event        = nullptr;
  next_fragment_spawn     = 0;
  metamorphosis_health    = 0;
  spirit_bomb_accumulator = 0.0;
  sigil_of_flame_activates = timespan_t::zero();

  for ( size_t i = 0; i < soul_fragments.size(); i++ )
  {
    delete soul_fragments[ i ];
  }

  soul_fragments.clear();
}

// demon_hunter_t::merge ==========================================================

void demon_hunter_t::merge( player_t& other )
{
  player_t::merge( other );

  const demon_hunter_t& s = static_cast<demon_hunter_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ]->merge( *s.counters[ i ] );

  for ( size_t i = 0, end = cd_waste_exec.size(); i < end; i++ )
  {
    cd_waste_exec[ i ]->second.merge( s.cd_waste_exec[ i ]->second );
    cd_waste_cumulative[ i ]->second.merge( s.cd_waste_cumulative[ i ]->second );
  }
}

// demon_hunter_t::datacollection_begin ===========================================

void demon_hunter_t::datacollection_begin()
{
  if ( active_during_iteration )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_iter[ i ]->second.reset();
    }
  }

  player_t::datacollection_begin();
}

// shaman_t::datacollection_end =============================================

void demon_hunter_t::datacollection_end()
{
  if ( requires_data_collection() )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_cumulative[ i ]->second.add( cd_waste_iter[ i ]->second.sum() );
    }
  }

  player_t::datacollection_end();
}

// ==========================================================================
// custom demon_hunter_t functions
// ==========================================================================

// demon_hunter_t::set_out_of_range =========================================

void demon_hunter_t::set_out_of_range( timespan_t duration )
{
  if ( duration <= timespan_t::zero() )
  {
    // Cancel all movement buffs and events
    buff.out_of_range->expire();
    buff.fel_rush_move->expire();
    buff.vengeful_retreat_move->expire();
    event_t::cancel( exit_melee_event );
  }
  else
  {
    if ( buff.out_of_range->check() )
    {
      buff.out_of_range->extend_duration( this, duration - buff.out_of_range->remains() );
      buff.out_of_range->default_value = cache.run_speed();
    }
    else
    {
      buff.out_of_range->trigger( 1, cache.run_speed(), -1.0, duration );
    }
  }
}

// demon_hunter_t::adjust_movement ==========================================

void demon_hunter_t::adjust_movement()
{
  if ( buff.out_of_range->check() &&
       buff.out_of_range->remains() > timespan_t::zero() )
  {
    // Recalculate movement duration.
    assert( buff.out_of_range->value() > 0 );
    assert( buff.out_of_range->expiration.size() );

    timespan_t remains = buff.out_of_range->remains();
    remains *= buff.out_of_range->check_value() / cache.run_speed();

    set_out_of_range(remains);
  }
}

// demon_hunter_t::consume_soul_fragments ===================================

unsigned demon_hunter_t::consume_soul_fragments( soul_fragment type, bool heal, unsigned max )
{
  unsigned souls_consumed = 0;
  std::vector<soul_fragment_t*> candidates;

  // Look through the list of active soul fragments to populate the vector of fragments to remove.
  // We need to use a new list as to not change the underlying vector as we are iterating over it
  // since the consume() method calls erase() on the fragment being consumed
  for ( auto& it : soul_fragments )
  {
    if ( it->is_type( type ) && it->active() )
    {
      candidates.push_back( it );
    }
  }

  for ( auto& it : candidates )
  {
    it->consume( heal );
    souls_consumed++;
    if ( souls_consumed >= max )
      break;
  }

  if ( sim->debug )
  {
    sim->out_debug.printf( "%s consumes %u %ss. remaining=%u", name(), souls_consumed,
                           get_soul_fragment_str( type ), 0,
                           get_total_soul_fragments( type ) );
  }

  return souls_consumed;
}

// demon_hunter_t::get_active_soul_fragments ================================

unsigned demon_hunter_t::get_active_soul_fragments( soul_fragment type ) const
{
  switch ( type )
  {
    case soul_fragment::GREATER:
    case soul_fragment::LESSER:
      return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                              [ &type ]( unsigned acc, soul_fragment_t* frag ) {
        return acc + ( frag->is_type( type ) && frag->active() );
      } );
    case soul_fragment::ALL:
    default:
      return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                              []( unsigned acc, soul_fragment_t* frag ) {
        return acc + frag->active();
      } );
  }
}

// demon_hunter_t::get_total_soul_fragments =================================

unsigned demon_hunter_t::get_total_soul_fragments( soul_fragment type ) const
{
  switch ( type )
  {
    case soul_fragment::GREATER:
    case soul_fragment::LESSER:
      return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                              [ &type ]( unsigned acc, soul_fragment_t* frag ) {
        return acc + frag->is_type( type );
      } );
    case soul_fragment::ALL:
    default:
      return (unsigned)soul_fragments.size();
  }
}

// demon_hunter_t::activate_soul_fragment ===================================

void demon_hunter_t::activate_soul_fragment( soul_fragment_t* frag )
{
  if ( frag->type == soul_fragment::LESSER )
  {
    unsigned active_fragments = get_active_soul_fragments( frag->type );
    if ( active_fragments > MAX_SOUL_FRAGMENTS )
    {
      // Find and delete the oldest active fragment of this type.
      for ( auto& it : soul_fragments )
      {
        if ( it->is_type( soul_fragment::LESSER ) && it->active() )
        {
          if ( specialization() == DEMON_HUNTER_HAVOC )
          {
            it->remove();
          }
          else // DEMON_HUNTER_VENGEANCE
          {
            // 7.2.5 -- When Soul Fragments are created that exceed the cap of 5 active fragments, 
            // the oldest fragment is now automatically consumed if it is within 60 yards of the Demon Hunter.
            // If it is more than 60 yds from the Demon Hunter, it despawns.
            it->consume( true );

            if ( sim->debug )
            {
              sim->out_debug.printf( "%s consumes overflow fragment %ss. remaining=%u", name(),
                                     get_soul_fragment_str( soul_fragment::LESSER ), get_total_soul_fragments( soul_fragment::LESSER ) );
            }
          }

          proc.soul_fragment_overflow->occur();
          event_t::cancel( soul_fragment_pick_up );
          break;
        }
      }
    }

    assert( get_active_soul_fragments( soul_fragment::LESSER ) <= MAX_SOUL_FRAGMENTS );
  }
}

// demon_hunter_t::spawn_soul_fragment ======================================

void demon_hunter_t::spawn_soul_fragment( soul_fragment type, unsigned n )
{
  proc_t* soul_proc = type == soul_fragment::GREATER ? proc.soul_fragment_greater : proc.soul_fragment_lesser;
  for ( unsigned i = 0; i < n; i++ )
  {
    soul_fragments.push_back( new soul_fragment_t( this, type ) );
    soul_proc->occur();
  }

  if ( sim->debug )
  {
    sim->out_debug.printf( "%s creates %u %ss. active=%u total=%u", name(), n,
                           get_soul_fragment_str( type ),
                           get_active_soul_fragments( type ),
                           get_total_soul_fragments( type ) );
  }
}

// demon_hunter_t::create_sigil_expression ==================================

expr_t* demon_hunter_t::create_sigil_expression( const std::string& name )
{
  if ( util::str_compare_ci( name, "activation_time" ) || util::str_compare_ci( name, "delay" ) )
  {
    if ( sim->optimize_expressions )
    {
      return expr_t::create_constant( name, sigil_delay.total_seconds() );
    }
    else
    {
      return make_ref_expr( name, sigil_delay );
    }
  }
  else if ( util::str_compare_ci( name, "sigil_placed" ) || util::str_compare_ci( name, "placed" ) )
  {
    struct sigil_placed_expr_t : public expr_t
    {
      demon_hunter_t* dh;

      sigil_placed_expr_t( demon_hunter_t* p, const std::string& n )
        : expr_t( n ), dh( p )
      {}

      double evaluate() override
      {
        if ( dh->sim->current_time() < dh->sigil_of_flame_activates )
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
    };

    return new sigil_placed_expr_t( this, name );
  }

  return nullptr;
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

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class demon_hunter_report_t : public player_report_extension_t
{
public:
  demon_hunter_report_t( demon_hunter_t& player ) : p( player )
  {
  }

  void cdwaste_table_header( report::sc_html_stream& os )
  {
    os << "<table class=\"sc\" style=\"float: left;margin-right: 10px;\">\n"
      << "<tr>\n"
      << "<th></th>\n"
      << "<th colspan=\"3\">Seconds per Execute</th>\n"
      << "<th colspan=\"3\">Seconds per Iteration</th>\n"
      << "</tr>\n"
      << "<tr>\n"
      << "<th>Ability</th>\n"
      << "<th>Average</th>\n"
      << "<th>Minimum</th>\n"
      << "<th>Maximum</th>\n"
      << "<th>Average</th>\n"
      << "<th>Minimum</th>\n"
      << "<th>Maximum</th>\n"
      << "</tr>\n";
  }

  void cdwaste_table_footer( report::sc_html_stream& os )
  {
    os << "</table>\n";
  }

  void cdwaste_table_contents( report::sc_html_stream& os )
  {
    size_t n = 0;
    for ( size_t i = 0; i < p.cd_waste_exec.size(); i++ )
    {
      const data_t* entry = p.cd_waste_exec[ i ];
      if ( entry->second.count() == 0 )
      {
        continue;
      }

      const data_t* iter_entry = p.cd_waste_cumulative[ i ];

      action_t* a = p.find_action( entry->first );
      std::string name_str = entry->first;
      if ( a )
      {
        name_str = report::action_decorator_t( a ).decorate();
      }

      std::string row_class_str = "";
      if ( ++n & 1 )
        row_class_str = " class=\"odd\"";

      os.printf( "<tr%s>", row_class_str.c_str() );
      os << "<td class=\"left\">" << name_str << "</td>";
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", entry->second.max() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.mean() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.min() );
      os.printf( "<td class=\"right\">%.3f</td>", iter_entry->second.max() );
      os << "</tr>\n";
    }
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    (void)p;
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
    if ( p.cd_waste_exec.size() > 0 )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown Waste Details</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "\t\t\t\t\t</div>\n";
      os << "<div class=\"clear\"></div>\n";
    }
    os << "\t\t\t\t\t</div>\n";
  }

private:
  demon_hunter_t& p;
};

using namespace unique_gear;
using namespace actions::spells;
using namespace actions::attacks;

namespace items
{
} // end namespace items

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
    auto p = new demon_hunter_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new demon_hunter_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void init( player_t* ) const override
  {
  }

  void static_init() const override
  {
    using namespace items;
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

const module_t* module_t::demon_hunter()
{
  static demon_hunter_module_t m;
  return &m;
}
