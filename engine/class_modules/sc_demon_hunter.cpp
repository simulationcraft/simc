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
// Legion To-Do
// ==========================================================================

   General ------------------------------------------------------------------
   Soul Fragments from Shattered Souls
   Demon Soul buff
   Felblade movement mechanics
   Darkness

   Havoc --------------------------------------------------------------------
   Defensive talent tier
   Second look at Momentum skills' timings
   Fury of the Illidari distance targeting support
   Overwhelming Power artifact trait
   Defensive artifact traits
   More thorough caching on blade_dance_expr_t
   Fix Nemesis

   Vengeance ----------------------------------------------------------------
   Torment
   Last Resort, Nether Bond talents
   True proc chance for Fallout
   Artificial Stamina

   Needs Documenting --------------------------------------------------------
   "activation_time" / "delay" sigil expression
   "sigil_placed" sigil expression
   "jump_cancel" -> "animation_cancel" fel rush option
*/

/* Forward declarations
 */
class demon_hunter_t;
struct soul_fragment_t;
struct damage_calc_helper_t;

namespace actions
{
struct demon_hunter_attack_t;
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
    // Vengeance
    dot_t* fiery_brand;
    dot_t* sigil_of_flame;
  } dots;

  struct debuffs_t
  {
    // Havoc
    buff_t* anguish;
    buff_t* nemesis;

    // Vengeance
    buff_t* frailty;
  } debuffs;

  demon_hunter_td_t( player_t* target, demon_hunter_t& p );
};

const unsigned MAX_SOUL_FRAGMENTS = 5;

enum soul_fragment_e
{
  SOUL_FRAGMENT_ALL = 0,
  SOUL_FRAGMENT_GREATER,
  SOUL_FRAGMENT_LESSER,
};

const char* get_soul_fragment_str( soul_fragment_e type )
{
  switch ( type )
  {
    case SOUL_FRAGMENT_ALL:
      return "soul fragment";
    case SOUL_FRAGMENT_GREATER:
      return "greater soul fragment";
    case SOUL_FRAGMENT_LESSER:
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

  void expire_override( int, timespan_t ) override;
};

const double VENGEFUL_RETREAT_DISTANCE = 20.0;

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
  std::vector<actions::demon_hunter_attack_t*> blade_dance_attacks;
  std::vector<actions::demon_hunter_attack_t*> death_sweep_attacks;
  std::vector<actions::demon_hunter_attack_t*> chaos_strike_attacks;
  std::vector<actions::demon_hunter_attack_t*> annihilation_attacks;

  std::vector<damage_calc_helper_t*> damage_calcs;
  damage_calc_helper_t* blade_dance_dmg;
  damage_calc_helper_t* death_sweep_dmg;
  damage_calc_helper_t* chaos_strike_dmg;
  damage_calc_helper_t* annihilation_dmg;

  // Autoattacks
  actions::attacks::melee_t* melee_main_hand;
  actions::attacks::melee_t* melee_off_hand;
  actions::attacks::chaos_blade_t* chaos_blade_main_hand;
  actions::attacks::chaos_blade_t* chaos_blade_off_hand;

  unsigned shear_counter;         // # of failed Shears since last proc
  double metamorphosis_health;    // Vengeance temp health from meta;
  double expected_max_health;

  // Soul Fragments
  unsigned next_fragment_spawn;  // determines whether the next fragment spawn
                                 // on the left or right
  auto_dispose<std::vector<soul_fragment_t*>> soul_fragments;
  event_t* soul_fragment_pick_up;

  std::vector<cooldown_t*>
    sigil_cooldowns;  // For Defiler's Lost Vambraces legendary

  double spirit_bomb;  // Spirit Bomb healing accumulator
  event_t* spirit_bomb_driver;

  // Override for target's hitbox size, relevant for Fel Rush and Vengeful
  // Retreat.
  double target_reach;
  event_t* exiting_melee;  // Event to disable melee abilities mid-VR.
  double initial_fury;

  timespan_t sigil_delay; // The amount of time it takes for a sigil to activate.
  timespan_t sigil_of_flame_activates; // When sigil of flame will next activate.

  // Buffs
  struct
  {
    // General
    buff_t* demon_soul;
    buff_t* metamorphosis;

    // Havoc
    buff_t* blade_dance;
    buff_t* blur;
    buff_t* chaos_blades;
    buff_t* death_sweep;
    movement_buff_t* fel_rush_move;
    buff_t* momentum;
    buff_t* out_of_range;
    buff_t* nemesis;
    buff_t* prepared;
    buff_t* rage_of_the_illidari;
    movement_buff_t* vengeful_retreat_move;

    // Vengeance
    buff_t* blade_turning;
    buff_t* defensive_spikes;
    buff_t* demon_spikes;
    buff_t* empower_wards;
    buff_t* immolation_aura;
    buff_t* painbringer;
    buff_t* siphon_power;
    absorb_buff_t* soul_barrier;
  } buff;

  // Talents
  struct
  {
    // General
    const spell_data_t* fel_eruption;

    const spell_data_t* felblade;

    // Havoc
    const spell_data_t* fel_mastery;
    const spell_data_t* demonic_appetite;
    const spell_data_t* blind_fury;

    const spell_data_t* prepared;
    const spell_data_t* demon_blades;

    const spell_data_t* first_blood;
    const spell_data_t* demon_reborn;

    const spell_data_t* momentum;
    const spell_data_t* bloodlet;
    const spell_data_t* nemesis;

    const spell_data_t* master_of_the_glaive;
    const spell_data_t* unleashed_power;
    const spell_data_t* chaos_cleave;

    const spell_data_t* chaos_blades;
    const spell_data_t* fel_barrage;
    const spell_data_t* demonic;

    // NYI
    const spell_data_t* netherwalk;
    const spell_data_t* desperate_instincts;

    // Vengeance
    const spell_data_t* abyssal_strike;  // NYI
    const spell_data_t* agonizing_flames;
    const spell_data_t* razor_spikes;

    const spell_data_t* feast_of_souls;
    const spell_data_t* fallout;
    const spell_data_t* burning_alive;

    //                  felblade
    const spell_data_t* flame_crash;  // NYI
    //                  fel_eruption

    const spell_data_t* feed_the_demon;
    const spell_data_t* fracture;
    const spell_data_t* soul_rending;

    const spell_data_t* concentrated_sigils;
    const spell_data_t* sigil_of_chains;
    const spell_data_t* quickened_sigils;

    const spell_data_t* fel_devastation;
    const spell_data_t* blade_turning;
    const spell_data_t* spirit_bomb;

    const spell_data_t* soul_barrier;
    const spell_data_t* last_resort;  // NYI
    const spell_data_t* nether_bond;  // NYI
  } talent;

  // Specialization Spells
  struct
  {
    // General
    const spell_data_t* demon_hunter;
    const spell_data_t* consume_magic;
    const spell_data_t* consume_soul;
    const spell_data_t* consume_soul_lesser;
    const spell_data_t* critical_strikes;
    const spell_data_t* leather_specialization;
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
    const spell_data_t* death_sweep;
    const spell_data_t* demonic_appetite_fury;
    const spell_data_t* fel_barrage_proc;
    const spell_data_t* fel_rush_damage;
    const spell_data_t* vengeful_retreat;

    // Vengeance
    const spell_data_t* vengeance;
    const spell_data_t* demonic_wards;
    const spell_data_t* fiery_brand_dr;
    const spell_data_t* immolation_aura;
    const spell_data_t* soul_cleave;
    const spell_data_t* riposte;
  } spec;

  // Mastery Spells
  struct
  {
    const spell_data_t* demonic_presence;  // Havoc
    const spell_data_t* fel_blood;         // Vengeance
  } mastery_spell;

  // Artifacts
  struct artifact_spell_data_t
  {
    // Havoc -- Twinblades of the Deceiver
    artifact_power_t anguish_of_the_deceiver;
    artifact_power_t balanced_blades;
    artifact_power_t chaos_vision;
    artifact_power_t contained_fury;
    artifact_power_t critical_chaos;
    artifact_power_t demon_rage;
    artifact_power_t demon_speed;
    artifact_power_t feast_on_the_souls;
    artifact_power_t fury_of_the_illidari;
    artifact_power_t inner_demons;
    artifact_power_t rage_of_the_illidari;
    artifact_power_t sharpened_glaives;
    artifact_power_t unleashed_demons;
    artifact_power_t warglaives_of_chaos;

    // NYI
    artifact_power_t deceivers_fury;
    artifact_power_t illidari_knowledge;
    artifact_power_t overwhelming_power;

    // Vengeance -- The Aldrachi Warblades
    artifact_power_t aldrachi_design;
    artifact_power_t aura_of_pain;
    artifact_power_t charred_warblades;
    artifact_power_t defensive_spikes;
    artifact_power_t demonic_flames;
    artifact_power_t devour_souls;
    artifact_power_t embrace_the_pain;
    artifact_power_t fiery_demise;
    artifact_power_t fueled_by_pain;
    artifact_power_t honed_warblades;
    artifact_power_t painbringer;
    artifact_power_t shatter_the_souls;
    artifact_power_t soul_carver;
    artifact_power_t tormented_souls;
    artifact_power_t will_of_the_illidari;

    // NYI
    artifact_power_t infernal_force;
    artifact_power_t siphon_power;
  } artifact;

  // Cooldowns
  struct
  {
    // General
    cooldown_t* consume_magic;
    cooldown_t* felblade;
    cooldown_t* fel_eruption;

    // Havoc
    cooldown_t* blade_dance;
    cooldown_t* blur;
    cooldown_t* death_sweep;
    cooldown_t* chaos_nova;
    cooldown_t* chaos_blades;
    cooldown_t* demonic_appetite;
    cooldown_t* eye_beam;
    cooldown_t* fel_barrage;
    cooldown_t* fel_barrage_proc;
    cooldown_t* fel_rush;
    cooldown_t* fel_rush_secondary;
    cooldown_t* fury_of_the_illidari;
    cooldown_t* nemesis;
    cooldown_t* netherwalk;
    cooldown_t* throw_glaive;
    cooldown_t* vengeful_retreat;
    cooldown_t* vengeful_retreat_secondary;

    // Vengeance
    cooldown_t* demon_spikes;
    cooldown_t* fiery_brand;
    cooldown_t* sigil_of_chains;
    cooldown_t* sigil_of_flame;
    cooldown_t* sigil_of_misery;
    cooldown_t* sigil_of_silence;
  } cooldown;

  // Gains
  struct
  {
    // General
    gain_t* miss_refund;

    // Havoc
    gain_t* demonic_appetite;
    gain_t* prepared;
	gain_t* blind_fury;

    // Vengeance
    gain_t* damage_taken;
    gain_t* immolation_aura;
    gain_t* metamorphosis;
  } gain;

  // Benefits
  struct
  {
  } benefits;

  // Procs
  struct
  {
    // General
    proc_t* delayed_aa_range;
    proc_t* delayed_aa_channel;
    proc_t* soul_fragment;
    proc_t* soul_fragment_lesser;

    // Havoc
    proc_t* demon_blades_wasted;
    proc_t* demonic_appetite;
    proc_t* demons_bite_in_meta;
    proc_t* felblade_reset;
    proc_t* fel_barrage;

    // Vengeance
    proc_t* fueled_by_pain;
    proc_t* soul_fragment_expire;
    proc_t* soul_fragment_overflow;
  } proc;

  // RPPM objects
  struct rppms_t
  {
    // General
    real_ppm_t* felblade;

    // Havoc
    real_ppm_t* inner_demons;

    // Vengeance
    real_ppm_t* fueled_by_pain;
  } rppm;

  // Special
  struct
  {
    // General
    heal_t* consume_soul_greater;
    heal_t* consume_soul_lesser;

    // Havoc
    spell_t* anguish;
    attack_t* demon_blades;
    spell_t* inner_demons;

    // Vengeance
    heal_t* charred_warblades;
    spell_t* immolation_aura;
    heal_t* spirit_bomb_heal;
  } active;

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

  // Legion Legendaries
  struct
  {
    // General

    // Havoc
    double eternal_hunger;
    timespan_t raddons_cascading_eyes;

    // Vengeance
    double cloak_of_fel_flames;
    double fragment_of_the_betrayers_prison;
    timespan_t kirel_narak;
    bool runemasters_pauldrons;
    timespan_t the_defilers_lost_vambraces;
  } legendary;

  demon_hunter_t( sim_t* sim, const std::string& name, race_e r );
  ~demon_hunter_t();

  // overridden player_t init functions
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void copy_from( player_t* source ) override;
  action_t* create_action( const std::string& name,
                           const std::string& options ) override;
  bool create_actions() override;
  void create_buffs() override;
  expr_t* create_expression( action_t*, const std::string& ) override;
  void create_options() override;
  pet_t* create_pet( const std::string& name,
                     const std::string& type = std::string() ) override;
  std::string create_profile( save_e = SAVE_ALL ) override;
  bool has_t18_class_trinket() const override;
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
  // overridden player_t stat functions
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
  double composite_spell_crit_chance() const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double passive_movement_modifier() const override;
  double temporary_movement_modifier() const override;

  // overridden player_t combat functions
  void assess_damage( school_e, dmg_e, action_state_t* s ) override;
  void assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* ) override;
  void combat_begin() override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  void interrupt() override;
  void recalculate_resource_max( resource_e ) override;
  void reset() override;
  void target_mitigation( school_e, dmg_e, action_state_t* ) override;

  // custom demon_hunter_t functions
  void adjust_movement();
  double calculate_expected_max_health() const;
  unsigned consume_soul_fragments( soul_fragment_e = SOUL_FRAGMENT_ALL,
                                   bool heal = true );
  unsigned get_active_soul_fragments(
    soul_fragment_e = SOUL_FRAGMENT_ALL ) const;
  unsigned get_total_soul_fragments(
    soul_fragment_e = SOUL_FRAGMENT_ALL ) const;
  void spawn_soul_fragment( soul_fragment_e, unsigned = 1 );
  void invalidate_damage_calcs();
  double get_target_reach() const
  {
    return target_reach >= 0 ? target_reach : sim -> target -> combat_reach;
  }
  expr_t* create_sigil_expression( const std::string& );

private:
  target_specific_t<demon_hunter_td_t> _target_data;
};

// Movement Buff definition =================================================

struct exit_melee_event_t : public event_t
{
  demon_hunter_t& dh;

  exit_melee_event_t( demon_hunter_t* p, timespan_t d )
    : event_t( *p, d ), dh( *p )
  {
    assert( d > timespan_t::zero() );
  }

  const char* name() const override
  {
    return "exit_melee_event";
  }

  void execute() override
  {
    if ( !dh.buff.out_of_range -> check() )
    {
      // Trigger out of range with no duration. This will get overridden once
      // the movement completes.
      dh.buff.out_of_range -> trigger( 1, dh.cache.run_speed() );
    }

    dh.exiting_melee = nullptr;
  }
};

bool movement_buff_t::trigger( int s, double v, double c, timespan_t d )
{
  assert( distance_moved > 0 );
  assert( buff_duration > timespan_t::zero() );

  // Check if we're moving towards or away from the target.
  if ( dh -> current.distance_to_move || dh -> buff.out_of_range -> check() ||
       dh -> buff.vengeful_retreat_move -> check() )
  { // Towards
    // Cancel any in progress movement.
    dh -> buff.fel_rush_move -> expire();
    dh -> buff.vengeful_retreat_move -> expire();
    event_t::cancel( dh -> exiting_melee );

    // "Move" back into melee range.
    dh -> buff.out_of_range -> expire();

    // We're not moving out, so don't trigger out of range at the end of this
    // movement.
    yards_from_melee = 0.0;
  }
  else
  { // Away
    // Calculate the number of yards away from melee this will send us.
    yards_from_melee = std::max(
                         0.0, distance_moved - ( dh -> get_target_reach() + 5.0 ) * 2.0 );
  }

  if ( yards_from_melee > 0.0 )
  {
    assert( !dh -> exiting_melee );

    // Calculate the amount of time it will take for the movement to carry us
    // out of melee range.
    timespan_t time =
      buff_duration * ( 1.0 - ( yards_from_melee / distance_moved ) );

    assert( time > timespan_t::zero() );

    // Schedule event to set us out of melee.
    dh -> exiting_melee = make_event<exit_melee_event_t>( *sim, dh, time );
  }

  return buff_t::trigger( s, v, c, d );
}

void movement_buff_t::expire_override( int s, timespan_t d )
{
  buff_t::expire_override( s, d );

  if ( sim -> debug )
    sim -> out_debug.printf( "%s debug, d=%f y=%.3f", name_str.c_str(),
                           d.total_millis(), yards_from_melee );

  // If we reached the end of the movement without it being canceled...
  if ( d == timespan_t::zero() && yards_from_melee > 0.0 )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s triggers out of range value=%.3f dur=%.3f",
                             name_str.c_str(), dh -> cache.run_speed(),
                             yards_from_melee / dh -> cache.run_speed() );

    // Then trigger out_of_range with a duration equal to the time it will take
    // us to get back.
    bool s = dh -> buff.out_of_range -> trigger(
               1, dh -> cache.run_speed(), -1.0,
               timespan_t::from_seconds( yards_from_melee / dh -> cache.run_speed() ) );

    if ( !s && sim -> debug )
      sim -> out_debug.printf( "%s fails to trigger out of range",
                             name_str.c_str() );
  }
}

// Delayed Execute Event ====================================================

struct delayed_execute_event_t : public event_t
{
  action_t* action;
  player_t* target;

  delayed_execute_event_t( demon_hunter_t* p, action_t* a, player_t* t,
                           timespan_t delay )
    : event_t( *p -> sim, delay ), action( a ), target( t )
  {
    assert( action -> background );
  }

  const char* name() const override
  {
    return "Delayed Execute";
  }

  void execute() override
  {
    if ( !target -> is_sleeping() )
    {
      action -> target = target;
      action -> schedule_execute();
    }
  }
};

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
          "%s %s expires. active=%u total=%u", frag -> dh -> name(),
          get_soul_fragment_str( frag -> type ),
          frag -> dh -> get_active_soul_fragments( frag -> type ),
          frag -> dh -> get_total_soul_fragments( frag -> type ) );
      }

      frag -> expiration = nullptr;
      frag -> remove();
    }
  };

  struct fragment_activate_t : public event_t
  {
    soul_fragment_t* frag;

    fragment_activate_t( soul_fragment_t* s ) : event_t( *s -> dh ), frag( s )
    {
      schedule( travel_time() );
    }

    const char* name() const override
    {
      return "Soul Fragment activate";
    }

    timespan_t travel_time() const
    {
      double distance = frag -> get_distance( frag -> dh );
      double velocity = frag -> dh -> spec.consume_soul -> missile_speed();
      return timespan_t::from_seconds( distance / velocity );
    }

    void execute() override
    {
      frag -> activate   = nullptr;
      frag -> expiration = make_event<fragment_expiration_t>( sim(), frag );

      if ( sim().debug )
      {
        sim().out_debug.printf(
          "%s %s becomes active. active=%u total=%u", frag -> dh -> name(),
          get_soul_fragment_str( frag -> type ),
          frag -> dh -> get_active_soul_fragments( frag -> type ),
          frag -> dh -> get_total_soul_fragments( frag -> type ) );
      }
    }
  };

  demon_hunter_t* dh;
  double x, y;
  event_t* activate;
  event_t* expiration;
  const soul_fragment_e type;

  soul_fragment_t( demon_hunter_t* p, soul_fragment_e t ) : dh( p ), type( t )
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
    return p -> get_position_distance( x, y );
  }

  bool active() const
  {
    return expiration != nullptr;
  }

  void remove() const
  {
    std::vector<soul_fragment_t*>::iterator it =
      std::find( dh -> soul_fragments.begin(), dh -> soul_fragments.end(), this );

    assert( it != dh -> soul_fragments.end() );

    dh -> soul_fragments.erase( it );
    delete this;
  }

  timespan_t remains() const
  {
    if ( expiration )
    {
      return expiration -> remains();
    }
    else
    {
      return dh -> spec.soul_fragment -> duration();
    }
  }

  bool is_type( soul_fragment_e t ) const
  {
    return t == SOUL_FRAGMENT_ALL || type == t;
  }

  void set_position()
  {
    // Set base position: 15 yards to the front right or front left.
    x = dh -> x_position + ( dh -> next_fragment_spawn % 2 ? -10.6066 : 10.6066 );
    y = dh -> y_position + 10.6066;

    // Calculate random offset, 2-5 yards from the base position.
    double r_min = 2.0, r_max = 5.0;
    // Nornmalizing factor
    double a = 2.0 / ( r_max * r_max - r_min * r_min );
    // Calculate distance from origin using power-law distribution for
    // uniformity.
    double dist = sqrt( 2.0 * dh -> rng().real() / a + r_min * r_min );
    // Pick a random angle.
    double theta = dh -> rng().range( 0.0, 2.0 * M_PI );
    // And finally, apply the offsets to x and y;
    x += dist * cos( theta );
    y += dist * sin( theta );

    dh -> next_fragment_spawn++;
  }

  void schedule_activate()
  {
    set_position();

    activate = make_event<fragment_activate_t>( *dh -> sim, this );
  }

  void consume( bool heal = true, bool instant = false )
  {
    assert( active() );

    if ( heal )
    {
      action_t* a = type == SOUL_FRAGMENT_GREATER
                    ? dh -> active.consume_soul_greater
                    : dh -> active.consume_soul_lesser;

      if ( instant )
      {
        a -> schedule_execute();
      }
      else
      {
        // FIXME: There's probably a more elegant travel time solution than
        // this.
        double velocity = dh -> spec.consume_soul -> missile_speed();
        timespan_t delay =
          timespan_t::from_seconds( get_distance( dh ) / velocity );

        make_event<delayed_execute_event_t>( *dh -> sim, dh, a, dh, delay );
      }
    }

    // Oct 3 2016: Feast on the Souls procs from both major and lesser fragments.
    if ( dh -> artifact.feast_on_the_souls.rank() )
    {
      timespan_t t = -dh -> artifact.feast_on_the_souls.time_value();

      dh -> cooldown.eye_beam -> adjust( t );
      dh -> cooldown.chaos_nova -> adjust( t );
    }

    if ( dh -> talent.feed_the_demon -> ok() )
    {
      // Not in spell data.
      dh -> cooldown.demon_spikes -> adjust( timespan_t::from_seconds( -1.0 ) );
    }

    if ( dh -> artifact.fueled_by_pain.rank() &&
         dh -> rppm.fueled_by_pain -> trigger() )
    {
      dh -> proc.fueled_by_pain -> occur();

      timespan_t duration = timespan_t::from_seconds(
                              dh -> artifact.fueled_by_pain.data().effectN( 1 ).base_value() );

      dh -> buff.metamorphosis -> trigger( 1, dh -> buff.metamorphosis -> default_value,
                                       -1.0, duration );
    }

    dh -> buff.painbringer -> trigger();

    if ( dh -> sim -> target -> race == RACE_DEMON )
    {
      dh -> buff.demon_soul -> trigger();
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
    if ( main_hand_attack && !main_hand_attack -> execute_event )
    {
      main_hand_attack -> schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    // Orc racial
    m *= 1.0 + o().racials.command -> effectN( 1 ).percent();

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
  bool demonic_presence;
  bool havoc_t19_2pc;
  bool hasted_gcd;
  bool may_proc_fel_barrage;

  demon_hunter_action_t( const std::string& n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
                         const std::string& o = std::string() )
    : ab( n, p, s ),
      demonic_presence( ab::data().affected_by(
                          p -> mastery_spell.demonic_presence -> effectN( 1 ) ) ),
      havoc_t19_2pc( ab::data().affected_by(
                       p -> sets.set( DEMON_HUNTER_HAVOC, T19, B2 ) ) ),
      hasted_gcd( false ),
      may_proc_fel_barrage( false )
  {
    ab::parse_options( o );
    ab::may_crit      = true;
    ab::tick_may_crit = true;

    switch ( p -> specialization() )
    {
      case DEMON_HUNTER_HAVOC:
        hasted_gcd = ab::data().affected_by( p -> spec.havoc -> effectN( 1 ) );
        ab::cooldown -> hasted =
          ab::data().affected_by( p -> spec.havoc -> effectN( 2 ) );
        break;
      case DEMON_HUNTER_VENGEANCE:
        hasted_gcd = ab::data().affected_by( p -> spec.vengeance -> effectN( 1 ) );
        ab::cooldown -> hasted =
          ab::data().affected_by( p -> spec.vengeance -> effectN( 2 ) );
        break;
      default:
        break;
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
    return p() -> get_target_data( t );
  }

  virtual timespan_t gcd() const override
  {
    timespan_t g = ab::gcd();

    if ( g == timespan_t::zero() )
      return g;

    if ( hasted_gcd )
    {
      g *= p() -> cache.attack_haste();
    }

    if ( g < ab::min_gcd )
      g = ab::min_gcd;

    return g;
  }

  virtual double action_multiplier() const override
  {
    double am = ab::action_multiplier();

    if ( demonic_presence )
    {
      am *= 1.0 + p() -> cache.mastery_value();
    }

    return am;
  }

  virtual double composite_energize_amount(
    const action_state_t* s ) const override
  {
    double ea = ab::composite_energize_amount( s );

    if ( havoc_t19_2pc && ab::energize_resource == RESOURCE_FURY )
    {
      ea *=
        1.0 +
        p() -> sets.set( DEMON_HUNTER_HAVOC, T19, B2 ) -> effectN( 1 ).percent();
    }

    return ea;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = ab::composite_target_multiplier( t );

    tm *= 1.0 + td( t ) -> debuffs.nemesis -> current_value;

    if ( dbc::is_school( ab::school, SCHOOL_FIRE ) )
    {
      tm *= 1.0 +
            td( t ) -> dots.fiery_brand -> is_ticking() *
            p() -> artifact.fiery_demise.percent();
    }

    return tm;
  }

  virtual void tick( dot_t* d ) override
  {
    ab::tick( d );

    trigger_fel_barrage( d -> state );
    trigger_charred_warblades( d -> state );
    accumulate_spirit_bomb( d -> state );

    // Benefit tracking
    if ( d -> state -> result_amount > 0 )
    {
      if ( ab::snapshot_flags & STATE_TGT_MUL_TA )
      {
        td( d -> state -> target ) -> debuffs.nemesis -> up();
      }
      if ( ab::snapshot_flags & STATE_MUL_TA )
      {
        p() -> buff.momentum -> up();
        p() -> buff.demon_soul -> up();
        p() -> buff.chaos_blades -> up();
      }
    }
  }

  virtual void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::result_is_hit( s -> result ) )
    {
      trigger_fel_barrage( s );
      trigger_charred_warblades( s );
      accumulate_spirit_bomb( s );

      // Benefit tracking
      if ( s -> result_amount > 0 )
      {
        if ( ab::snapshot_flags & STATE_TGT_MUL_DA )
        {
          td( s -> target ) -> debuffs.nemesis -> up();
        }
        if ( ab::snapshot_flags & STATE_MUL_DA )
        {
          p() -> buff.momentum -> up();
          p() -> buff.demon_soul -> up();
          p() -> buff.chaos_blades -> up();
        }
      }
    }
  }

  virtual void execute() override
  {
    ab::execute();

    if ( !ab::hit_any_target && ab::resource_consumed > 0 )
    {
      trigger_refund();
    }
  }

  virtual bool ready() override
  {
    if ( ( ab::execute_time() > timespan_t::zero() || ab::channeled ) &&
         ( p() -> buff.out_of_range -> check() || p() -> soul_fragment_pick_up ) )
    {
      return false;
    }

    if ( p() -> buff.out_of_range -> check() && ab::range <= 5.0 )
    {
      return false;
    }

    if ( p() -> buff.fel_rush_move -> check() )
    {
      return false;
    }

    return ab::ready();
  }

  void trigger_refund()
  {
    if ( ab::resource_current == RESOURCE_FURY ||
         ab::resource_current == RESOURCE_PAIN )
    {
      p() -> resource_gain( ab::resource_current, ab::resource_consumed * 0.80,
                          p() -> gain.miss_refund );
    }
  }

  void trigger_charred_warblades( action_state_t* s )
  {
    if ( !p() -> artifact.charred_warblades.rank() )
    {
      return;
    }

    if ( !dbc::is_school( ab::school, SCHOOL_FIRE ) )
    {
      return;
    }

    if ( !( ab::harmful && s -> result_amount > 0 ) )
    {
      return;
    }

    heal_t* action      = p() -> active.charred_warblades;
    action -> base_dd_min = action -> base_dd_max = s -> result_amount;
    action -> schedule_execute();
  }

  void accumulate_spirit_bomb( action_state_t* s )
  {
    if ( !p() -> talent.spirit_bomb -> ok() )
    {
      return;
    }

    if ( !( ab::harmful && s -> result_amount > 0 ) )
    {
      return;
    }

    p() -> spirit_bomb +=
      s -> result_amount * td( s -> target ) -> debuffs.frailty -> value();
  }

  void trigger_fel_barrage( action_state_t* s )
  {
    if ( ! p() -> talent.fel_barrage -> ok() )
      return;

    if ( ! may_proc_fel_barrage )
      return;

    if ( ! ab::result_is_hit( s -> result ) )
      return;

    if ( s -> result_amount <= 0 )
      return;

    if ( p() -> cooldown.fel_barrage_proc -> down() )
      return;

    /* Sep 23 2016: Via hotfix, this ability was changed in some mysterious
    unknown manner. Proc mechanic is now totally serverside.
    http://blue.mmo-champion.com/topic/767333-hotfixes-september-16/

    Assuming proc chance and ICD are unchanged and that ICD starts on-attempt
    instead of on-success now.

    As a result, chance and ICD are now hardcoded (they are no longer in the
    spell data).*/

    if ( p() -> rng().roll( 0.10 ) )
    {
      p() -> proc.fel_barrage -> occur();
      p() -> cooldown.fel_barrage -> reset( true );
    }

    p() -> cooldown.fel_barrage_proc -> start();
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

struct demon_hunter_heal_t : public demon_hunter_action_t<heal_t>
{
  demon_hunter_heal_t( const std::string& n, demon_hunter_t* p,
                       const spell_data_t* s = spell_data_t::nil(),
                       const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {
    harmful = false;
    target  = p;
  }
};

// ==========================================================================
// Demon Hunter spells
// ==========================================================================

struct demon_hunter_spell_t : public demon_hunter_action_t<spell_t>
{
  demon_hunter_spell_t( const std::string& n, demon_hunter_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {
  }
};

namespace heals
{
// Consume Soul =============================================================

struct consume_soul_t : public demon_hunter_heal_t
{
  struct demonic_appetite_t : public demon_hunter_spell_t
  {
    demonic_appetite_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "demonic_appetite_fury", p,
                              p -> find_spell( 210041 ) )
    {
      may_miss = may_crit = callbacks = false;
      background = quiet = true;
      energize_type = ENERGIZE_ON_CAST;
    }
  };

  soul_fragment_e type;

  consume_soul_t( demon_hunter_t* p, const std::string& n,
                  const spell_data_t* s, soul_fragment_e t )
    : demon_hunter_heal_t( n, p, s ), type( t )
  {
    background = true;

    if ( p -> talent.demonic_appetite -> ok() )
    {
      execute_action = new demonic_appetite_t( p );
    }
  }
};

// Charred Warblades ========================================================

struct charred_warblades_t : public demon_hunter_heal_t
{
  charred_warblades_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "charred_warblades", p, p -> find_spell( 213011 ) )
  {
    background = true;
    may_crit   = false;
    base_multiplier *=
      p -> artifact.charred_warblades.data().effectN( 1 ).percent();
  }

  void init() override
  {
    demon_hunter_heal_t::init();

    snapshot_flags &= ~STATE_VERSATILITY;  // Not affected by Versatility.
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_heal_t : public demon_hunter_heal_t
{
  fel_devastation_heal_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "fel_devastation_heal", p, p -> find_spell( 212106 ) )
  {
    background = true;
    may_crit   = false;
  }
};

// Soul Barrier =============================================================

struct soul_barrier_t : public demon_hunter_action_t<absorb_t>
{
  double soul_coefficient;

  soul_barrier_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_action_t( "soul_barrier", p, p -> talent.soul_barrier,
                             options_str )
  {
    soul_coefficient = data().effectN( 2 ).ap_coeff();
  }

  double attack_direct_power_coefficient(
    const action_state_t* s ) const override
  {
    double c =
      demon_hunter_action_t<absorb_t>::attack_direct_power_coefficient( s );

    c += p() -> get_active_soul_fragments() * soul_coefficient;

    return c;
  }

  void execute() override
  {
    demon_hunter_action_t<absorb_t>::execute();

    p() -> consume_soul_fragments( SOUL_FRAGMENT_ALL, false );
  }

  void impact( action_state_t* s ) override
  {
    p() -> buff.soul_barrier -> trigger( 1, s -> result_amount );
  }
};

// Soul Cleave ==============================================================

struct soul_cleave_heal_t : public demon_hunter_heal_t
{
  double pct_heal;

  soul_cleave_heal_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "soul_cleave_heal", p, p -> spec.soul_cleave ),
      pct_heal( 0.0 )
  {
    background = true;
    // This action is free; the parent pays the cost.
    base_costs.fill( 0 );
    secondary_costs.fill( 0 );
    attack_power_mod.direct = 4.50; // From tooltip.

    if ( p -> talent.feast_of_souls -> ok() )
    {
      const spell_data_t* s = p -> find_spell( 207693 );
      dot_duration          = s -> duration();
      base_tick_time        = s -> effectN( 1 ).period();
      attack_power_mod.tick = 1.30;  // Not in spell data.
      hasted_ticks          = false;
    }

    base_multiplier *= 1.0 + p -> artifact.devour_souls.percent();
  }

  void execute() override
  {
    base_dd_min = base_dd_max = p() -> max_health() * pct_heal;

    demon_hunter_heal_t::execute();
  }
};

// Spirit Bomb ==============================================================

struct spirit_bomb_heal_t : public demon_hunter_heal_t
{
  spirit_bomb_heal_t( demon_hunter_t* p )
    : demon_hunter_heal_t( "spirit_bomb_heal", p, p -> find_spell( 227255 ) )
  {
    background = true;
    may_crit   = false;
  }

  void init() override
  {
    demon_hunter_heal_t::init();

    // Jun 26 2016: *Is* affected by versatility.
    // snapshot_flags &= ~STATE_VERSATILITY;
  }
};
}

namespace spells
{
// Anguish ==================================================================

struct anguish_t : public demon_hunter_spell_t
{
  anguish_t( demon_hunter_t* p )
    : demon_hunter_spell_t( "anguish", p, p -> find_spell( 202446 ) )
  {
    background = dual = true;
    base_dd_min = base_dd_max = 0;
  }
};

// Blur =====================================================================

struct blur_t : public demon_hunter_spell_t
{
  blur_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "blur", p, p -> spec.blur, options_str )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> buff.blur -> trigger();

    if ( p() -> artifact.demon_speed.rank() )
    {
		if (maybe_ptr(p()->dbc.ptr)){
			return;
		}

      p() -> cooldown.fel_rush -> reset( false, true );
    }
  }
};

// Chaos Blades =============================================================

struct chaos_blades_t : public demon_hunter_spell_t
{
  chaos_blades_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "chaos_blades", p, p -> talent.chaos_blades,
                            options_str )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = harmful = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> buff.chaos_blades -> trigger( 1, p() -> cache.mastery_value() );
  }
};

// Chaos Nova ===============================================================

struct chaos_nova_t : public demon_hunter_spell_t
{
  chaos_nova_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "chaos_nova", p, p -> spec.chaos_nova, options_str )
  {
    aoe = -1;
    cooldown -> duration += p -> talent.unleashed_power -> effectN( 1 ).time_value();
    base_costs[ RESOURCE_FURY ] *=
      1.0 + p -> talent.unleashed_power -> effectN( 2 ).percent();
    school = SCHOOL_CHAOS;  // Jun 27 2016: Spell data states Chromatic damage,
    // just override it.
    may_proc_fel_barrage = true;  // Jul 12 2016
  }
};

// Consume Magic ============================================================

struct consume_magic_t : public demon_hunter_spell_t
{
  resource_e resource;
  double resource_amount;

  consume_magic_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "consume_magic", p, p -> spec.consume_magic,
                            options_str )
  {
    /* use_off_gcd = */ ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = may_crit = false;

    const spelleffect_data_t effect = p -> find_spell( 218903 ) -> effectN(
                                        p -> specialization() == DEMON_HUNTER_HAVOC ? 1 : 2 );
    resource        = effect.resource_gain_type();
    resource_amount = effect.resource( resource );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> resource_gain( resource, resource_amount, gain );
  }

  bool ready() override
  {
    if ( !target -> debuffs.casting -> check() )
      return false;

    return demon_hunter_spell_t::ready();
  }
};

// Demon Spikes =============================================================

struct demon_spikes_t : public demon_hunter_spell_t
{
  demon_spikes_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "demon_spikes", p,
                            p -> find_specialization_spell( "Demon Spikes" ),
                            options_str )
  {
    use_off_gcd = true;
    cooldown -> hasted = true; // Doesn't show up in spelldata
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> buff.demon_spikes -> trigger();
    p() -> buff.defensive_spikes -> trigger();
  }
};

// Empower Wards ===========================================================

struct empower_wards_t : public demon_hunter_spell_t
{
  empower_wards_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "empower_wards", p,
                            p -> find_specialization_spell( "Empower Wards" ),
                            options_str )
  {
    use_off_gcd = true;
    base_dd_min = base_dd_max = 0;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> buff.empower_wards -> trigger();
  }
};

// Eye Beam =================================================================

struct eye_beam_t : public demon_hunter_spell_t
{
  struct eye_beam_tick_t : public demon_hunter_spell_t
  {
    eye_beam_tick_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "eye_beam_tick", p, p -> find_spell( 198030 ) )
    {
      aoe  = -1;
      dual = background = true;
      base_crit += p -> spec.havoc -> effectN( 3 ).percent();
      may_proc_fel_barrage = true;  // Jul 12 2016

      school = SCHOOL_CHAOS;  // Jun 27 2016: Spell data states Chromatic
      // damage, just override it.

      base_multiplier *= 1.0 + p -> artifact.chaos_vision.percent();
    }

    dmg_e amount_type( const action_state_t*, bool ) const override
    {
      return DMG_OVER_TIME;
    }  // TOCHECK

    void impact( action_state_t* s ) override
    {
      demon_hunter_spell_t::impact( s );

      if ( result_is_hit( s -> result ) &&
           p() -> artifact.anguish_of_the_deceiver.rank() )
      {
        td( s -> target ) -> debuffs.anguish -> trigger();
      }

      if ( p() -> legendary.raddons_cascading_eyes < timespan_t::zero() &&
           s -> result == RESULT_CRIT )
      {
        p() -> cooldown.eye_beam -> adjust( p() -> legendary.raddons_cascading_eyes );
      }

	  if (p()->talent.blind_fury->ok()){
		  p()->resource_gain(RESOURCE_FURY, 7, p()->gain.blind_fury);
	  }
    }
  };

  eye_beam_tick_t* beam;

  eye_beam_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "eye_beam", p, p -> find_class_spell( "Eye Beam" ),
                            options_str ),
      beam( new eye_beam_tick_t( p ) )
  {
    may_miss = may_crit = false;
    harmful     = false;  // Disables bleeding on the target.
    channeled   = true;
    beam -> stats = stats;

    school = SCHOOL_CHAOS;  // Jun 27 2016: Spell data states Chromatic damage,
    // just override it.

    dot_duration *= 1.0 + p -> talent.blind_fury -> effectN( 1 ).percent();

    if ( p -> artifact.anguish_of_the_deceiver.rank() )
    {
      add_child( p -> active.anguish );
    }
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void tick( dot_t* d ) override
  {
    demon_hunter_spell_t::tick( d );

    beam -> schedule_execute();
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( p() -> talent.demonic -> ok() )
    {
		timespan_t demonicTime;
		if (maybe_ptr(p()->dbc.ptr)){
			demonicTime = timespan_t::from_seconds(8);
		}
		else{
			demonicTime = timespan_t::from_seconds(5);
		}

      /* Buff starts when the channel does and lasts for 5 seconds +
      base channel duration. No duration data for demonic. */
      p() -> buff.metamorphosis -> trigger(
        1, p() -> buff.metamorphosis -> default_value, -1.0,
		demonicTime + dot_duration);
    }

    // If Metamorphosis has less than channel s remaining, it gets extended so the whole Eye Beam happens during Meta.
    // TOCHECK: Base channel duration or hasted channel duration?
    if ( p() -> buff.metamorphosis -> remains_lt( dot_duration ) )
    {
      p() -> buff.metamorphosis -> trigger( 1, p() -> buff.metamorphosis -> current_value, -1.0,
        dot_duration );
    }
  }
};

// Fel Barrage ==============================================================

struct fel_barrage_t : public demon_hunter_spell_t
{
  struct fel_barrage_tick_t : public demon_hunter_spell_t
  {
    fel_barrage_tick_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "fel_barrage_tick", p, p -> find_spell( 211052 ) )
    {
      background = dual = true;
      aoe                  = -1;
      may_proc_fel_barrage = false;  // Can't proc itself, that would be silly!
      school = SCHOOL_CHAOS; // override to chaos, spell data is wrong
    }
  };

  action_t* damage;

  fel_barrage_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fel_barrage", p, p -> talent.fel_barrage,
                            options_str )
  {
    channeled = tick_zero = true;
    may_proc_fel_barrage = false;
    dot_duration   = data().duration();
    base_tick_time = data().effectN( 1 ).period();
    school = SCHOOL_CHAOS; // override to chaos, spell data is wrong

    damage = p -> find_action( "fel_barrage_tick" );

    if ( ! damage )
    {
      damage = new fel_barrage_tick_t( p );
    }

    damage -> stats = stats;
  }

  // Hide direct results in report.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  timespan_t travel_time() const override
  { return timespan_t::zero(); }

  // Channel is not hasted. TOCHECK
  timespan_t tick_time( const action_state_t* ) const override
  { return base_tick_time; }

  double composite_persistent_multiplier( const action_state_t* ) const override
  {
    /* Override persistent multiplier and just return the charge multiplier.
    This value will be used to modify the tick_state. */
    return ( double )cooldown -> current_charge / cooldown -> charges;
  }

  void update_ready( timespan_t cd_duration ) override
  {
    assert( cooldown -> current_charge > 0 );

    /* A bit of a dirty hack to consume all charges. Just consume all but one
       and let action_t::update_ready() take it from there. */
    cooldown -> current_charge = 1;

    demon_hunter_spell_t::update_ready( cd_duration );
  }

  bool usable_moving() const override
  { return true; }

  void tick( dot_t* d ) override
  {
    demon_hunter_spell_t::tick( d );

    action_state_t* tick_state = damage -> get_state();
    damage -> target = d -> target;
    damage -> snapshot_state( tick_state, DMG_DIRECT );
    // Multiply the damage by the number of charges.
    tick_state -> persistent_multiplier *= d -> state -> persistent_multiplier;
    damage-> schedule_execute( tick_state );
  }
};

// Fel Devastation ==========================================================

struct fel_devastation_t : public demon_hunter_spell_t
{
  struct fel_devastation_tick_t : public demon_hunter_spell_t
  {
    fel_devastation_tick_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "fel_devastation_tick", p,
                              p -> find_spell( 212105 ) )
    {
      aoe        = -1;
      background = dual = true;
    }

    dmg_e amount_type( const action_state_t*, bool ) const override
    {
      return DMG_OVER_TIME;
    }  // TOCHECK
  };

  heals::fel_devastation_heal_t* heal;
  fel_devastation_tick_t* damage;

  fel_devastation_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fel_devastation", p, p -> talent.fel_devastation,
                            options_str )
  {
    channeled = true;
    may_miss = may_crit = false;
    harmful       = false;  // Disables bleeding on the target.
    heal          = new heals::fel_devastation_heal_t( p );
    damage        = new fel_devastation_tick_t( p );
    damage -> stats = stats;
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  // Channel is not hasted.
  timespan_t tick_time( const action_state_t* ) const override
  {
    return base_tick_time;
  }

  void tick( dot_t* d ) override
  {
    demon_hunter_spell_t::tick( d );

    // Heal happens first.
    heal -> schedule_execute();
    damage -> schedule_execute();
  }
};

// Fel Eruption =============================================================

struct fel_eruption_t : public demon_hunter_spell_t
{
  struct fel_eruption_damage_t : public demon_hunter_spell_t
  {
    fel_eruption_damage_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "fel_eruption_dmg", p, p -> find_spell( 225102 ) )
    {
      background = dual = true;
      may_miss = false;
      // Assume the target is stun immune.
      base_multiplier *= 1.0 + p -> talent.fel_eruption -> effectN( 1 ).percent();
      school = SCHOOL_CHAOS;        // Jun 27 2016: Spell data states Chromatic
      // damage, just override it.
      may_proc_fel_barrage = true;  // Jul 12 2016

      // Damage penalty for Vengeance DH
      base_multiplier *= 1.0 + p -> spec.vengeance -> effectN( 4 ).percent();
    }
  };

  fel_eruption_damage_t* damage;

  fel_eruption_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fel_eruption", p, p -> talent.fel_eruption,
                            options_str )
  {
    may_crit         = false;
    resource_current = p -> specialization() == DEMON_HUNTER_HAVOC
                       ? RESOURCE_FURY
                       : RESOURCE_PAIN;

    damage        = new fel_eruption_damage_t( p );
    damage -> stats = stats;

    // Add damage modifiers in fel_eruption_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( hit_any_target )
    {
      damage -> target = execute_state -> target;
      damage -> schedule_execute();
    }
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
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

      primary = debug_cast<const fiery_brand_state_t*>( s ) -> primary;
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
      : demon_hunter_spell_t( "fiery_brand_dot", p, p -> find_spell( 204022 ) )
    {
      background = dual = true;
      hasted_ticks = may_crit = false;
      school = p -> find_specialization_spell( "Fiery Brand" ) -> get_school_type();
      base_dd_min = base_dd_max = 0;

      dot_duration += p -> artifact.demonic_flames.time_value();

      if ( p -> talent.burning_alive -> ok() )
      {
        // Not in spell data.
        attack_power_mod.tick = 1.0;
        // Spread radius used for Burning Alive.
        radius = p -> find_spell( 207760 ) -> effectN( 1 ).radius();
      }
      else
      {
        /* If Burning alive isn't talented this isn't really a DoT, so let's
        turn off DoT callbacks and minimize the number of events generated.*/
        base_tick_time = dot_duration;
        tick_may_crit = callbacks = false;
      }
    }

    action_state_t* new_state() override
    {
      return new fiery_brand_state_t( this, target );
    }

    dot_t* get_dot( player_t* t ) override
    {
      if ( ! t ) t = target;
      if ( ! t ) return nullptr;

      return td( t ) -> dots.fiery_brand;
    }

    void record_data( action_state_t* s ) override
    {
      // Don't record data direct hits for this action.
      if ( s -> result_type != DMG_DIRECT )
      {
        demon_hunter_spell_t::record_data( s );
      }
#ifndef NDEBUG
      else
      {
        assert( s -> result_amount == 0.0 );
      }
#endif
    }

    void tick( dot_t* d ) override
    {
      demon_hunter_spell_t::tick( d );

      trigger_burning_alive( d );
    }

    void trigger_burning_alive( dot_t* d )
    {
      if ( ! p() -> talent.burning_alive -> ok() )
      {
        return;
      }

      if ( ! debug_cast<fiery_brand_state_t*>( d -> state ) -> primary )
      {
        return;
      }

      // Retrieve target list, checking for distance if necessary.
      std::vector<player_t*> targets = target_list();
      if ( sim -> distance_targeting_enabled )
      {
        targets = check_distance_targeting( targets );
      }

      if ( targets.size() == 1 )
      {
        return;
      }

      // Filter target list down to targets that are not already branded.
      std::vector<player_t*> candidates;

      for ( size_t i = 0; i < targets.size(); i++ )
      {
        if ( ! td( targets[ i ] ) -> dots.fiery_brand -> is_ticking() )
        {
          candidates.push_back( targets[ i ] );
        }
      }

      if ( candidates.size() == 0 )
      {
        return;
      }

      // Pick a random target.
      player_t* target =
        candidates[ ( int ) p() -> rng().range( 0, candidates.size() ) ];

      // Execute a dot on that target.
      this -> target = target;
      schedule_execute();
    }
  };

  action_t* dot;

  fiery_brand_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fiery_brand", p,
      p -> find_specialization_spell( "Fiery Brand" ), options_str )
  {
    use_off_gcd = true;

    dot = p -> find_action( "fiery_brand_dot" );

    if ( ! dot )
    {
      dot = new fiery_brand_dot_t( p );
    }

    dot -> stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    /* Set this DoT as the primary DoT, enabling its ticks to spread the DoT
       to nearby targets when Burning Alive is talented. */
    fiery_brand_state_t* fb_state =
      debug_cast<fiery_brand_state_t*>( dot -> get_state() );
    fb_state -> target = s -> target;
    dot -> snapshot_state( fb_state, DMG_DIRECT );
    fb_state -> primary = true;
    dot -> schedule_execute( fb_state );
  }
};

// Metamorphosis ============================================================

struct sigil_of_flame_damage_t : public demon_hunter_spell_t
{
  sigil_of_flame_damage_t( demon_hunter_t* );

  dot_t* get_dot( player_t* ) override;
};

struct infernal_strike_t : public demon_hunter_spell_t
{
  struct infernal_strike_impact_t : public demon_hunter_spell_t
  {
    action_t* sigil;

    infernal_strike_impact_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "infernal_strike_impact", p,
        p -> find_spell( 189112 ) ), sigil( nullptr )
    {
      background = dual = true;
      aoe = -1;

      base_multiplier *= 1.0 + p -> artifact.infernal_force.percent();

      if ( p -> talent.flame_crash -> ok() )
      {
        sigil = new sigil_of_flame_damage_t( p );
      }
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      if ( sigil )
      {
        p() -> sigil_of_flame_activates = sim -> current_time() + p() -> sigil_delay;

        make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
            .target( execute_state -> target )
            .x( p() -> x_position )
            .y( p() -> y_position )
            .pulse_time( p() -> sigil_delay )
            .duration( p() -> sigil_delay )
            .start_time( sim -> current_time() )
            .action( sigil ) );
      }
    }
  };

  action_t* damage;

  infernal_strike_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "infernal_strike", p,
      p -> find_specialization_spell( "Infernal Strike" ), options_str )
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
    travel_speed = 1.0;  // allows use precombat

    damage = p -> find_action( "infernal_strike_impact" );

    if ( ! damage )
    {
      infernal_strike_impact_t* a = new infernal_strike_impact_t( p );
      a -> stats = stats;
      if ( a -> sigil )
      {
        add_child( a -> sigil );
      }

      damage = a;
    }

    base_teleport_distance += p -> talent.abyssal_strike -> effectN( 1 ).base_value();
    cooldown -> duration += p -> talent.abyssal_strike -> effectN( 2 ).time_value();
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  // leap travel time, independent of distance
  // TOCHECK: Just assuming this is the same as metamorphosis.
  timespan_t travel_time() const override
  { return timespan_t::from_seconds( 1.0 ); }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    damage -> target = s -> target;
    damage -> schedule_execute();
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( ! t ) t = target;
    if ( ! t ) return nullptr;

    return td( t ) -> dots.sigil_of_flame;
  }

  expr_t* create_expression( const std::string& name ) override
  {
    if ( expr_t* e = p() -> create_sigil_expression( name ) )
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
    immolation_aura_damage_t( demon_hunter_t* p, spell_data_t* s )
      : demon_hunter_spell_t( "immolation_aura_dmg", p, s ), initial( false )
    {
      aoe        = -1;
      background = dual = true;
      // Rename gain for periodic energizes. Initial hit action doesn't
      // energize.
      gain = p -> get_gain( "immolation_aura_tick" );

      base_multiplier *=
        1.0 + p -> talent.agonizing_flames -> effectN( 1 ).percent();
      base_multiplier *= 1.0 + p -> artifact.aura_of_pain.percent();
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        // FIXME: placeholder proc chance, lack of info on real proc chance.
        if ( initial && p() -> talent.fallout -> ok() && rng().roll( 0.60 ) )
        {
          p() -> spawn_soul_fragment( SOUL_FRAGMENT_LESSER );
        }

        if ( p() -> legendary.kirel_narak < timespan_t::zero() )
        {
          p() -> cooldown.fiery_brand -> adjust( p() -> legendary.kirel_narak );
        }
      }
    }
  };

  immolation_aura_damage_t* initial;

  immolation_aura_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "immolation_aura", p, p -> spec.immolation_aura,
                            options_str )
  {
    may_miss = may_crit = false;
    dot_duration = timespan_t::zero();  // damage is dealt by ticking buff

    if ( !p -> active.immolation_aura )
    {
      p -> active.immolation_aura =
        new immolation_aura_damage_t( p, data().effectN( 1 ).trigger() );
      p -> active.immolation_aura -> stats = stats;
    }

    initial = new immolation_aura_damage_t( p, data().effectN( 2 ).trigger() );
    initial -> initial = true;
    initial -> stats   = stats;

    // Add damage modifiers in immolation_aura_damage_t, not here.
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void execute() override
  {
    p() -> buff.immolation_aura -> trigger();

    demon_hunter_spell_t::execute();

    initial -> schedule_execute();
  }
};

// Inner Demons =============================================================

struct inner_demons_t : public demon_hunter_spell_t
{
  inner_demons_t( demon_hunter_t* p )
    : demon_hunter_spell_t( "inner_demons", p, p -> find_spell( 202388 ) )
  {
    background = true;
    aoe        = -1;
  }

  timespan_t travel_time() const override
  {
    return p() -> artifact.inner_demons.data().effectN( 1 ).trigger() -> duration();
  }
};

// Metamorphosis ============================================================

struct metamorphosis_t : public demon_hunter_spell_t
{
  struct metamorphosis_impact_t : public demon_hunter_spell_t
  {
    metamorphosis_impact_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "metamorphosis_impact", p,
                              p -> find_spell( 200166 ) )
    {
      background   = true;
      aoe          = -1;
      dot_duration = timespan_t::zero();
      school       = SCHOOL_CHAOS;  // Jun 27 2016: Spell data states Chromatic
      // damage, just override it.
      may_proc_fel_barrage = true;  // Jul 12 2016
    }
  };

  metamorphosis_impact_t* damage;

  metamorphosis_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "metamorphosis", p,
                            p -> find_class_spell( "Metamorphosis" ),
                            options_str )
  {
    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    dot_duration            = timespan_t::zero();
    base_teleport_distance  = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
    damage                  = new metamorphosis_impact_t( p );
    min_gcd                 = timespan_t::from_seconds(
                                1.0 );           // cannot use skills during travel time
    travel_speed = 1.0;  // allows use precombat

    cooldown -> duration += p -> artifact.unleashed_demons.time_value();
  }

  // leap travel time, independent of distance
  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 1.0 );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    damage -> schedule_execute();

    // Buff is gained at the start of the leap.
    p() -> buff.metamorphosis -> trigger();

    if ( p() -> talent.demon_reborn -> ok() )
    {
      p() -> cooldown.blur -> reset( false, true );
      p() -> cooldown.chaos_nova -> reset( false, true );
      p() -> cooldown.eye_beam -> reset( false, true );
    }

    if ( p() -> legendary.runemasters_pauldrons )
    {
      p() -> cooldown.sigil_of_chains -> reset( false, true );
      p() -> cooldown.sigil_of_flame -> reset( false, true );
      p() -> cooldown.sigil_of_misery -> reset( false, true );
      p() -> cooldown.sigil_of_silence -> reset( false, true );
    }
  }
};

// Nemesis ==================================================================

struct nemesis_t : public demon_hunter_spell_t
{
  nemesis_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "nemesis", p, p -> talent.nemesis, options_str )
  {
    may_miss = may_block = may_dodge = may_parry = may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    td( s -> target ) -> debuffs.nemesis -> trigger();
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
      : event_t( *f -> dh, time ), dh( f -> dh ), frag( f ), expr( e )
    {
    }

    const char* name() const override
    {
      return "Soul Fragment pick up";
    }

    void execute() override
    {
      // Evaluate if_expr to make sure the actor still wants to consume.
      if ( frag && expr -> eval() )
      {
        frag -> consume( true, true );
      }

      dh -> soul_fragment_pick_up = nullptr;
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
  soul_fragment_e type;

  pick_up_fragment_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "pick_up_fragment", p, spell_data_t::nil() ),
      select_mode( SOUL_FRAGMENT_SELECT_OLDEST ),
      type( SOUL_FRAGMENT_ALL )
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
      sim -> errorf(
        "%s uses bad parameter for pick_up_soul_fragment option "
        "\"mode\". Valid options: closest, newest, oldest",
        sim -> active_player -> name() );
    }
  }

  void parse_type( const std::string& value )
  {
    if ( value == "greater" )
    {
      type = SOUL_FRAGMENT_GREATER;
    }
    else if ( value == "lesser" )
    {
      type = SOUL_FRAGMENT_LESSER;
    }
    else if ( value == "all" || value == "any" )
    {
      type = SOUL_FRAGMENT_ALL;
    }
    else if ( value != "" )
    {
      sim -> errorf(
        "%s uses bad parameter for pick_up_soul_fragment option "
        "\"type\". Valid options: greater, lesser, any",
        sim -> active_player -> name() );
    }
  }

  timespan_t calculate_movement_time( soul_fragment_t* frag )
  {
    // Fragments have a 6 yard trigger radius
    double dtm = std::max( 0.0, frag -> get_distance( p() ) - 6.0 );

    return timespan_t::from_seconds( dtm / p() -> cache.run_speed() );
  }

  soul_fragment_t* select_fragment()
  {
    switch ( select_mode )
    {
      case SOUL_FRAGMENT_SELECT_NEAREST:
      {
        double dist                = std::numeric_limits<double>::max();
        soul_fragment_t* candidate = nullptr;

        for ( it = p() -> soul_fragments.begin(); it != p() -> soul_fragments.end();
              it++ )
        {
          soul_fragment_t* frag = *it;

          if ( frag -> is_type( type ) && frag -> active() &&
               frag -> remains() < calculate_movement_time( frag ) )
          {
            double this_distance = frag -> get_distance( p() );

            if ( this_distance < dist )
            {
              dist      = this_distance;
              candidate = frag;
            }
          }
        }

        return candidate;
      }
      case SOUL_FRAGMENT_SELECT_NEWEST:
        for ( it = p() -> soul_fragments.end(); it != p() -> soul_fragments.begin();
              it-- )
        {
          soul_fragment_t* frag = *it;

          if ( frag -> is_type( type ) &&
               frag -> remains() > calculate_movement_time( frag ) )
          {
            return frag;
          }
        }
      case SOUL_FRAGMENT_SELECT_OLDEST:
      default:
        for ( it = p() -> soul_fragments.begin(); it != p() -> soul_fragments.end();
              it++ )
        {
          soul_fragment_t* frag = *it;

          if ( frag -> is_type( type ) &&
               frag -> remains() > calculate_movement_time( frag ) )
          {
            return frag;
          }
        }
    }

    return nullptr;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    soul_fragment_t* frag = select_fragment();
    assert( frag );
    timespan_t time = calculate_movement_time( frag );

    assert( p() -> soul_fragment_pick_up == nullptr );
    p() -> soul_fragment_pick_up = make_event<pick_up_event_t>( *sim, frag, time, if_expr );
  }

  bool ready() override
  {
    if ( p() -> soul_fragment_pick_up )
    {
      return false;
    }

    if ( !p() -> get_active_soul_fragments( type ) )
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

// Sigil of Flame ===========================================================

inline sigil_of_flame_damage_t::sigil_of_flame_damage_t( demon_hunter_t* p )
  : demon_hunter_spell_t( "sigil_of_flame_dmg", p, p -> find_spell( 204598 ) )
{
  aoe = -1;
  background = dual = ground_aoe = true;
  hasted_ticks = false;

  if ( p -> talent.concentrated_sigils -> ok() )
  {
    dot_duration +=
      p -> talent.concentrated_sigils -> effectN( 5 ).time_value();
  }
}

inline dot_t* sigil_of_flame_damage_t::get_dot( player_t* t )
{
  if ( ! t ) t = target;
  if ( ! t ) return nullptr;

  return td( t ) -> dots.sigil_of_flame;
}

struct sigil_of_flame_t : public demon_hunter_spell_t
{
  sigil_of_flame_damage_t* damage;

  sigil_of_flame_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "sigil_of_flame", p,
                            p -> find_specialization_spell( "Sigil of Flame" ),
                            options_str )
  {
    may_miss = may_crit = false;
    damage = new sigil_of_flame_damage_t( p );
    damage -> stats = stats;

    cooldown -> duration *=
      1.0 + p -> talent.quickened_sigils -> effectN( 2 ).percent();

    // Add damage modifiers in sigil_of_flame_damage_t, not here.
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p() -> sigil_of_flame_activates = sim -> current_time() + p() -> sigil_delay;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
        .target( execute_state -> target )
        .x( p() -> talent.concentrated_sigils -> ok() ?
          p() -> x_position : execute_state -> target -> x_position )
        .y( p() -> talent.concentrated_sigils -> ok() ?
          p() -> y_position : execute_state -> target -> y_position )
        .pulse_time( p() -> sigil_delay )
        .duration( p() -> sigil_delay )
        .start_time( sim -> current_time() )
        .action( damage ) );
  }

  expr_t* create_expression( const std::string& name ) override
  {
    if ( expr_t* e = p() -> create_sigil_expression( name ) )
      return e;

    return demon_hunter_spell_t::create_expression( name );
  }
};

// Spirit Bomb ==============================================================

struct spirit_bomb_t : public demon_hunter_spell_t
{
  struct spirit_bomb_damage_t : public demon_hunter_spell_t
  {
    spirit_bomb_damage_t( demon_hunter_t* p )
      : demon_hunter_spell_t( "spirit_bomb_dmg", p, p -> find_spell( 218677 ) )
    {
      background = true;
      aoe        = -1;
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_spell_t::impact( s );

      if ( result_is_hit( s -> result ) )
      {
        td( s -> target ) -> debuffs.frailty -> trigger();
      }
    }
  };

  spirit_bomb_damage_t* dmg;

  spirit_bomb_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "spirit_bomb", p, p -> talent.spirit_bomb,
                            options_str ),
      dmg( new spirit_bomb_damage_t( p ) )
  {
    dmg -> stats = stats;

    if ( !p -> active.spirit_bomb_heal )
    {
      p -> active.spirit_bomb_heal = new heals::spirit_bomb_heal_t( p );
    }
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void consume_resource() override
  {
    demon_hunter_spell_t::consume_resource();

    assert( p() -> soul_fragments.size() > 0 );

    // Spend the oldest soul fragment. TOCHECK: What happens in-game? Lesser vs
    // Major?
    p() -> soul_fragments[ 0 ] -> consume( false );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    dmg -> target = execute_state -> target;
    dmg -> schedule_execute();
  }

  bool ready() override
  {
    return p() -> get_active_soul_fragments() && demon_hunter_spell_t::ready();
  }
};

}  // end namespace spells

// ==========================================================================
// Demon Hunter attacks
// ==========================================================================

struct demon_hunter_attack_t : public demon_hunter_action_t<melee_attack_t>
{
  demon_hunter_attack_t( const std::string& n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
                         const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {
    special = true;

    parse_special_effect_data( data() );
  }

  void parse_special_effect_data( const spell_data_t& spell )
  {
    /* Only set weapon if the attack deals weapon damage. weapon_multiplier
    defaults to 1.0 so the way we check that is if weapon is defined. */
    if ( weapon && spell.flags( SPELL_ATTR_EX3_REQ_OFFHAND ) )
    {
      weapon = &( p() -> off_hand_weapon );
    }
  }

  // Mechanics described at
  // http://us.battle.net/wow/en/forum/topic/20743504316?page=26#520
  void trigger_demon_blades( action_state_t* s )
  {
    if ( !p() -> talent.demon_blades -> ok() )
      return;

    if ( !result_is_hit( s -> result ) )
      return;

    // All hits have an x% chance to generate 1 charge.
    if ( !rng().roll( p() -> talent.demon_blades -> effectN( 1 ).percent() ) )
        return;

    p() -> active.demon_blades -> target = s -> target;
    p() -> active.demon_blades -> schedule_execute();
  }
};

namespace attacks
{
// Melee Attack =============================================================

struct melee_t : public demon_hunter_attack_t
{
  enum status_e
  {
    MELEE_CONTACT,
    LOST_CONTACT_CHANNEL,
    LOST_CONTACT_RANGE,
  };

  struct
  {
    status_e main_hand, off_hand;
  } status;

  melee_t( const std::string& name, demon_hunter_t* p )
    : demon_hunter_attack_t( name, p, spell_data_t::nil() )
  {
    school     = SCHOOL_PHYSICAL;
    special    = false;
    background = repeating = may_glance = true;
    trigger_gcd = timespan_t::zero();

    status.main_hand = status.off_hand = LOST_CONTACT_RANGE;

    if ( p -> dual_wield() )
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
      weapon -> slot == SLOT_MAIN_HAND ? status.main_hand : status.off_hand;

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

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    trigger_demon_blades( s );
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
    if ( p() -> current.distance_to_move > 5 || p() -> channeling ||
         p() -> buff.out_of_range -> check() )
    {
      status_e s;

      // Cancel autoattacks, auto_attack_t will restart them when we're able to
      // attack again.
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
  auto_attack_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "auto_attack", p, spell_data_t::nil(),
                             options_str )
  {
    ignore_false_positive = true;
    range                 = 5;
    trigger_gcd           = timespan_t::zero();

    p -> melee_main_hand                     = new melee_t( "auto_attack_mh", p );
    p -> main_hand_attack                    = p -> melee_main_hand;
    p -> main_hand_attack -> weapon            = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    p -> melee_off_hand                     = new melee_t( "auto_attack_oh", p );
    p -> off_hand_attack                    = p -> melee_off_hand;
    p -> off_hand_attack -> weapon            = &( p -> off_hand_weapon );
    p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    p -> off_hand_attack -> id                = 1;
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

    if ( ready )  // Range check
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

struct blade_dance_attack_t : public demon_hunter_attack_t
{
  double first_blood_multiplier;  // Used by damage_calc_helper_t

  blade_dance_attack_t( demon_hunter_t* p, const spell_data_t* s,
                        const std::string& name )
    : demon_hunter_attack_t( name, p, s )
  {
    dual = background = true;
    aoe = -1;
    first_blood_multiplier =
      1.0 + p -> talent.first_blood -> effectN( 1 ).percent();
  }

  double action_multiplier() const override
  {
    double am = demon_hunter_attack_t::action_multiplier();

    am *= 1.0 + target_list().size() * p() -> artifact.balanced_blades.percent();

    return am;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = demon_hunter_attack_t::composite_da_multiplier( s );

    if ( s -> chain_target == 0 )
    {
      dm *= first_blood_multiplier;
    }

    return dm;
  }
};

struct blade_dance_event_t : public event_t
{
  demon_hunter_t* dh;
  size_t current;
  bool in_metamorphosis;
  const spell_data_t* timing_passive;
  std::vector<demon_hunter_attack_t*>* attacks;

  blade_dance_event_t( demon_hunter_t* p, size_t ca, bool meta = false )
    : event_t( *p -> sim ), dh( p ), current( ca ), in_metamorphosis( meta )
  {
    timing_passive =
      in_metamorphosis ? p -> spec.death_sweep : p -> spec.blade_dance;
    attacks =
      in_metamorphosis ? &p -> death_sweep_attacks : &p -> blade_dance_attacks;

    schedule( next_execute() );
  }

  const char* name() const override
  {
    return "Blade Dance";
  }

  timespan_t next_execute() const
  {
    if ( current == 0 )
    {
      return timespan_t::zero();
    }
    else
    {
      return timespan_t::from_millis(
               timing_passive -> effectN( current + 4 ).misc_value1() -
               timing_passive -> effectN( current + 3 ).misc_value1() );
    }
  }

  void execute() override
  {
    attacks -> at( current ) -> schedule_execute();
    current++;

    if ( current < attacks -> size() )
    {
      dh -> blade_dance_driver =
          make_event<blade_dance_event_t>( sim(), dh, current, in_metamorphosis );
    }
    else
    {
      dh -> blade_dance_driver = nullptr;
    }
  }
};

struct blade_dance_base_t : public demon_hunter_attack_t
{
  std::vector<demon_hunter_attack_t*> attacks;
  buff_t* dodge_buff;

  blade_dance_base_t( const std::string& n, demon_hunter_t* p,
                      const spell_data_t* s, const std::string& options_str )
    : demon_hunter_attack_t( n, p, s, options_str ), dodge_buff( nullptr )
  {
    may_miss = may_crit = may_parry = may_block = may_dodge = false;
    cooldown = p -> get_cooldown( "blade_dance" );  // shared cooldown
    // Disallow use outside of melee range.
    range = 5.0;
    ignore_false_positive = true;

    base_costs[ RESOURCE_FURY ] +=
      p -> talent.first_blood -> effectN( 2 ).resource( RESOURCE_FURY );
  }

  virtual bool init_finished() override
  {
    bool f = demon_hunter_attack_t::init_finished();

    for ( size_t i = 0; i < attacks.size(); i++ )
    {
      attacks[ i ] -> stats = stats;
    }

    return f;
  }

  // Don't record data for this action.
  virtual void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  virtual double action_multiplier() const override
  {
    double am = demon_hunter_attack_t::action_multiplier();

    am *= 1.0 + target_list().size() * p() -> artifact.balanced_blades.percent();

    return am;
  }

  virtual double composite_da_multiplier(
    const action_state_t* s ) const override
  {
    double dm = demon_hunter_attack_t::composite_da_multiplier( s );

    if ( p() -> talent.first_blood -> ok() && s -> chain_target == 0 )
    {
      dm *= 1.0 + p() -> talent.first_blood -> effectN( 1 ).percent();
    }

    return dm;
  }

  virtual void execute() override
  {
    cooldown -> duration = data().cooldown();

    demon_hunter_attack_t::execute();

    assert( !p() -> blade_dance_driver );
    p() -> blade_dance_driver = make_event<blade_dance_event_t>( *sim, p(), 0, p() -> buff.metamorphosis -> up() );

    assert( dodge_buff );
    dodge_buff -> trigger();
  }
};

struct blade_dance_t : public blade_dance_base_t
{
  blade_dance_t( demon_hunter_t* p, const std::string& options_str )
    : blade_dance_base_t( "blade_dance", p, p -> spec.blade_dance, options_str )
  {
    if ( p -> blade_dance_attacks.empty() )
    {
      p -> blade_dance_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 4 ).trigger(), "blade_dance1" ) );
      p -> blade_dance_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 5 ).trigger(), "blade_dance2" ) );
      p -> blade_dance_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 6 ).trigger(), "blade_dance3" ) );
      p -> blade_dance_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 7 ).trigger(), "blade_dance4" ) );

      // Jul 12 2016: Only final attack procs Fel Barrage.
      p -> blade_dance_attacks.back() -> may_proc_fel_barrage = true;
    }

    attacks    = p -> blade_dance_attacks;
    dodge_buff = p -> buff.blade_dance;
  }

  bool ready() override
  {
    if ( p() -> buff.metamorphosis -> check() )
    {
      return false;
    }

    return blade_dance_base_t::ready();
  }
};

// Chaos Blades ============================================================

struct chaos_blade_t : public demon_hunter_attack_t
{
  chaos_blade_t( const std::string& n, demon_hunter_t* p,
                 const spell_data_t* s )
    : demon_hunter_attack_t( n, p, s )
  {
    base_execute_time = weapon -> swing_time;
    special           = true;  // Apr 12 2016: Cannot miss.
    repeating = background = true;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    trigger_demon_blades( s );
  }
};

// Chaos Strike =============================================================

struct chaos_strike_state_t : public action_state_t
{
  bool is_critical;
  player_t* dh;

  chaos_strike_state_t( action_t* a, player_t* target )
    : action_state_t( a, target ), is_critical(false), dh( a -> player )
  {
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );

    is_critical = debug_cast<const chaos_strike_state_t*>( s ) -> is_critical;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s );

    s << " is_critical=" << is_critical;

    return s;
  }
};

struct chaos_strike_base_t : public demon_hunter_attack_t
{
  struct chaos_strike_damage_t : public demon_hunter_attack_t
  {
    timespan_t delay;
    chaos_strike_base_t* parent;
    bool may_refund;

    chaos_strike_damage_t( demon_hunter_t* p, const spelleffect_data_t& eff,
                           chaos_strike_base_t* a )
      : demon_hunter_attack_t( "chaos_strike_dmg", p, eff.trigger() ),
        delay( timespan_t::from_millis( eff.misc_value1() ) ),
        parent( a )
    {
      assert( eff.type() == E_TRIGGER_SPELL );

      dual = background = true;
      aoe = data().effectN( 1 ).chain_target() +
            p -> talent.chaos_cleave -> effectN( 1 ).base_value();
      chain_multiplier = data().effectN( 1 ).chain_multiplier();
      may_refund       = weapon == &( p -> off_hand_weapon );

      // Do not put crit chance modifiers here!
      base_multiplier *= 1.0 + p -> artifact.warglaives_of_chaos.percent();
      crit_bonus_multiplier *= 1.0 + p -> artifact.critical_chaos.percent();
    }

    action_state_t* new_state() override
    {
      return new chaos_strike_state_t( this, target );
    }

    result_e calculate_result( action_state_t* s ) const override
    {
      result_e r = demon_hunter_attack_t::calculate_result( s );

      if ( result_is_miss( r ) )
      {
        return r;
      }

      /* Use the crit roll from our custom state instead of the roll done in
         attack_t::calculate_result. */
      return debug_cast<chaos_strike_state_t*>( s ) -> is_critical ? RESULT_CRIT
             : RESULT_HIT;
    }

    void schedule_execute( action_state_t* s = nullptr ) override
    {
      demon_hunter_attack_t::schedule_execute( s );

      /* Refund occurs prior to the offhand hit, and since we need to check the
         execute state
         we'll replicate that by doing it here instead of in execute(). */
      chaos_strike_state_t* cs = debug_cast<chaos_strike_state_t*>( s );

      if ( weapon == &( p() -> off_hand_weapon ) && parent -> roll_refund( cs ) )
      {
        p() -> resource_gain( RESOURCE_FURY,
                            parent -> composite_energize_amount( s ),
                            parent -> gain );
      }
    }
  };

  struct chaos_strike_event_t : public event_t
  {
    action_t* action;
    player_t* target;
    chaos_strike_state_t* state;

    chaos_strike_event_t( action_t* a, player_t* t, chaos_strike_state_t* s )
      : event_t( *a->player->sim,
                 debug_cast<chaos_strike_damage_t*>( a )->delay ),
        action( a ),
        target( t ),
        state( s )
    {
      assert( action->background );
    }

    const char* name() const override
    {
      return "chaos_strike_event";
    }

    void execute() override
    {
      if ( !target -> is_sleeping() )
      {
        chaos_strike_state_t* s =
          debug_cast<chaos_strike_state_t*>( action -> get_state() );
        s -> target = target;
        action -> snapshot_state( s, DMG_DIRECT );
        s -> is_critical = state -> is_critical;
        action -> schedule_execute( s );
      }
    }
  };

  std::vector<demon_hunter_attack_t*> attacks;

  chaos_strike_base_t( const std::string& n, demon_hunter_t* p,
                       const spell_data_t* s, const std::string& options_str )
    : demon_hunter_attack_t( n, p, s, options_str )
  {
    energize_amount =
      p -> spec.chaos_strike_refund -> effectN( 1 ).resource( RESOURCE_FURY );
    aoe = s -> effectN( 1 ).chain_target() +
          p -> talent.chaos_cleave -> effectN( 1 ).base_value();

    // Don't put damage modifiers here, they should go in chaos_strike_damage_t.
    // Crit chance modifiers need to be in here, not chaos_strike_damage_t.
    base_crit +=
      p -> sets.set( DEMON_HUNTER_HAVOC, T19, B4 ) -> effectN( 1 ).percent();
  }

  virtual bool init_finished() override
  {
    bool f = demon_hunter_attack_t::init_finished();

    // Use one stats object for all parts of the attack.
    for ( size_t i = 0; i < attacks.size(); i++ )
    {
      attacks[ i ] -> stats = stats;
    }

    return f;
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  /* Determines whether the OH hit should trigger a refund. Leaving this as a
  virtual
  function in case the two skills have different refund mechanics again.*/
  // Jun 27 2016: Annihilation refunds half on crit despite what tooltip states.
  virtual bool roll_refund( chaos_strike_state_t* s )
  {
    return s -> is_critical;
  }

  action_state_t* new_state() override
  {
    return new chaos_strike_state_t( this, target );
  }

  void snapshot_state( action_state_t* s, dmg_e rt ) override
  {
    demon_hunter_attack_t::snapshot_state( s, rt );

    chaos_strike_state_t* cs = debug_cast<chaos_strike_state_t*>( s );

    /* If this is the primary target then roll for crit, otherwise copy the
       result from the previous chain_target's state. */
    if ( cs -> chain_target == 0 )
    {
      cs -> is_critical = p() -> rng().roll( cs -> composite_crit_chance() );
    }
    else if ( execute_state )
    {
      cs -> is_critical =
        debug_cast<chaos_strike_state_t*>( execute_state ) -> is_critical;
    }
  }

  result_e calculate_result( action_state_t* s ) const override
  {
    result_e r = demon_hunter_attack_t::calculate_result( s );

    if ( result_is_miss( r ) )
    {
      return r;
    }

    /* Use the crit roll from our custom state instead of the roll done in
       attack_t::calculate_result. */
    return debug_cast<chaos_strike_state_t*>( s ) -> is_critical ? RESULT_CRIT
           : RESULT_HIT;
  }

  virtual void execute() override
  {
    demon_hunter_attack_t::execute();

    /* Trigger child actions. For Annihilation this is the MH and OH damage,
    and for Chaos Strike it's just the OH (this action is the MH action). */
    for ( size_t i = 0; i < attacks.size(); i++ )
    {
      make_event<chaos_strike_event_t>( *sim,
        attacks[ i ], target,
        debug_cast<chaos_strike_state_t*>( execute_state ) );
    }

    // Metamorphosis benefit
    p() -> buff.metamorphosis -> up();

    if ( hit_any_target )
    {
      // TODO: Travel time
      if ( p() -> talent.demonic_appetite -> ok() &&
           !p() -> cooldown.demonic_appetite -> down() &&
           p() -> rng().roll( p() -> talent.demonic_appetite -> proc_chance() ) )
      {
        p() -> cooldown.demonic_appetite -> start();
        p() -> proc.demonic_appetite -> occur();

        p() -> spawn_soul_fragment( SOUL_FRAGMENT_LESSER );
        // FIXME
      }

      // Inner Demons procs on cast
      if ( p() -> rppm.inner_demons -> trigger() )
      {
        assert( p() -> active.inner_demons );
        p() -> active.inner_demons -> target = target;
        p() -> active.inner_demons -> schedule_execute();
      }
    }
  }
};

struct chaos_strike_t : public chaos_strike_base_t
{
  chaos_strike_t( demon_hunter_t* p, const std::string& options_str )
    : chaos_strike_base_t( "chaos_strike", p, p -> spec.chaos_strike,
                           options_str )
  {
    if ( p -> chaos_strike_attacks.empty() )
    {
      p -> chaos_strike_attacks.push_back(
        new chaos_strike_damage_t( p, data().effectN( 2 ), this ) );
      p -> chaos_strike_attacks.push_back(
        new chaos_strike_damage_t( p, data().effectN( 3 ), this ) );

      // Jul 12 2016: Only first attack procs Fel Barrage.
      p -> chaos_strike_attacks.front() -> may_proc_fel_barrage = true;
    }

    attacks = p -> chaos_strike_attacks;
  }

  bool ready() override
  {
    if ( p() -> buff.metamorphosis -> check() )
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
    : chaos_strike_base_t( "annihilation", p, p -> spec.annihilation,
                           options_str )
  {
    if ( p -> annihilation_attacks.empty() )
    {
      p -> annihilation_attacks.push_back(
        new chaos_strike_damage_t( p, data().effectN( 2 ), this ) );
      p -> annihilation_attacks.push_back(
        new chaos_strike_damage_t( p, data().effectN( 3 ), this ) );

      // Jul 12 2016: Only first attack procs Fel Barrage.
      p -> annihilation_attacks.front() -> may_proc_fel_barrage = true;
    }

    attacks = p -> annihilation_attacks;
  }

  bool ready() override
  {
    if ( !p() -> buff.metamorphosis -> check() )
    {
      return false;
    }

    return chaos_strike_base_t::ready();
  }
};

// Death Sweep ==============================================================

struct death_sweep_t : public blade_dance_base_t
{
  death_sweep_t( demon_hunter_t* p, const std::string& options_str )
    : blade_dance_base_t( "death_sweep", p, p -> spec.death_sweep, options_str )
  {
    if ( p -> death_sweep_attacks.empty() )
    {
      p -> death_sweep_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 4 ).trigger(), "death_sweep1" ) );
      p -> death_sweep_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 5 ).trigger(), "death_sweep2" ) );
      p -> death_sweep_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 6 ).trigger(), "death_sweep3" ) );
      p -> death_sweep_attacks.push_back( new blade_dance_attack_t(
                                          p, data().effectN( 7 ).trigger(), "death_sweep4" ) );

      // Jul 12 2016: Only final attack procs Fel Barrage.
      p -> death_sweep_attacks.back() -> may_proc_fel_barrage = true;
    }

    attacks    = p -> death_sweep_attacks;
    dodge_buff = p -> buff.death_sweep;
  }

  void execute() override
  {
    blade_dance_base_t::execute();

    assert( p() -> buff.metamorphosis -> check() );

    // If Metamorphosis has less than 1s remaining, it gets extended so the whole Death Sweep happens during Meta.
    if ( p() -> buff.metamorphosis -> remains_lt( p() -> buff.death_sweep -> buff_duration ) )
    {
      p() -> buff.metamorphosis -> trigger( 1, p() -> buff.metamorphosis -> current_value, -1.0,
        p() -> buff.death_sweep -> buff_duration );
    }
  }

  bool ready() override
  {
    // Death Sweep can be queued in the last 250ms, so need to ensure meta is still up after that.
    if ( p() -> buff.metamorphosis -> remains() <= cooldown -> queue_delay() )
    {
      return false;
    }

    return blade_dance_base_t::ready();
  }
};

// Demon's Bite =============================================================

struct demons_bite_t : public demon_hunter_attack_t
{
  unsigned energize_die_sides;

  demons_bite_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t(
        "demons_bite", p, p -> find_class_spell( "Demon's Bite" ), options_str )
  {
    energize_die_sides   = data().effectN( 3 ).die_sides();
    may_proc_fel_barrage = true;  // Jul 12 2016

    base_multiplier *= 1.0 + p -> artifact.demon_rage.percent();
  }

  void execute() override
  {
    // Modify base amount so it properly benefits from multipliers.
    double old_amount = energize_amount;
    energize_amount += ( int )rng().range( 1, 1 + energize_die_sides );

    demon_hunter_attack_t::execute();

    energize_amount = old_amount;

    if ( p() -> buff.metamorphosis -> check() )
    {
      p() -> proc.demons_bite_in_meta -> occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> talent.felblade -> ok() &&
         p() -> rppm.felblade -> trigger() )
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

// Demon Blade ==============================================================

struct demon_blades_t : public demon_hunter_attack_t
{
  unsigned energize_die_sides;

  demon_blades_t( demon_hunter_t* p )
    : demon_hunter_attack_t( "demon_blades", p, p -> find_spell( 203796 ) )
  {
    background           = true;
    cooldown -> duration   = p -> talent.demon_blades -> internal_cooldown();
    energize_die_sides   = data().effectN( 3 ).die_sides();
    may_proc_fel_barrage = true;  // Jul 12 2016

    base_multiplier *= 1.0 + p -> artifact.demon_rage.percent();
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    ea += ( int )rng().range( 1, 1 + energize_die_sides );

    return ea;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> talent.felblade -> ok() &&
         p() -> rppm.felblade -> trigger() )
    {
      p() -> proc.felblade_reset -> occur();
      p() -> cooldown.felblade -> reset( true );
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
      : demon_hunter_attack_t( "felblade_dmg", p, p -> find_spell( 213243 ) )
    {
      dual = background = true;
      may_miss = may_dodge = may_parry = false;
      may_proc_fel_barrage = true;  // Jul 12 2016

      // Clear energize and then manually pick which effect to parse.
      energize_type = ENERGIZE_NONE;
      parse_effect_data( data().effectN( p -> specialization() == DEMON_HUNTER_HAVOC ? 4 : 3 ) );

      // Damage penalty for Vengeance DH
      base_multiplier *= 1.0 + p -> spec.vengeance -> effectN( 3 ).percent();
    }
  };

  felblade_damage_t* damage;

  felblade_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "felblade", p, p -> talent.felblade, options_str )
  {
    may_crit = may_block = false;
    damage        = new felblade_damage_t( p );
    damage -> stats = stats;

    movement_directionality = MOVEMENT_TOWARDS;

    // Add damage modifiers in felblade_damage_t, not here.
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( hit_any_target )
    {
      damage -> target = execute_state -> target;
      damage -> schedule_execute();
    }

    // Cancel Vengeful Retreat movement.
    p() -> buff.vengeful_retreat_move -> expire();
    p() -> buff.out_of_range -> expire();
    event_t::cancel( p() -> exiting_melee );
  }
};

// Fel Rush =================================================================

struct fel_rush_t : public demon_hunter_attack_t
{
  struct fel_rush_damage_t : public demon_hunter_attack_t
  {
    fel_rush_damage_t( demon_hunter_t* p )
      : demon_hunter_attack_t( "fel_rush_dmg", p, p -> spec.fel_rush_damage )
    {
      aoe  = -1;
      dual = background = true;
      may_proc_fel_barrage = true;  // Jul 12 2016

      base_multiplier *= 1.0 + p -> talent.fel_mastery -> effectN( 2 ).percent();

      if ( p -> talent.fel_mastery -> ok() )
      {
        energize_type     = ENERGIZE_ON_HIT;
        energize_resource = RESOURCE_FURY;
        energize_amount =
          p -> talent.fel_mastery -> effectN( 1 ).resource( RESOURCE_FURY );
      }
    }
  };

  fel_rush_damage_t* damage;
  bool a_cancel;

  fel_rush_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "fel_rush", p, p -> find_class_spell( "Fel Rush" ) ),
      a_cancel( false )
  {
    add_option( opt_bool( "animation_cancel", a_cancel ) );
    parse_options( options_str );

    may_miss = may_dodge = may_parry = may_block = may_crit = false;
    min_gcd = trigger_gcd;

    damage = new fel_rush_damage_t( p );
    damage -> stats = stats;

    if ( !a_cancel )
    {
      base_teleport_distance  = damage -> radius;
      movement_directionality = MOVEMENT_OMNI;
      ignore_false_positive   = true;

      p -> buff.fel_rush_move -> distance_moved = base_teleport_distance;
    }

    // Add damage modifiers in fel_rush_damage_t, not here.
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  timespan_t gcd() const override
  {
    timespan_t g = demon_hunter_attack_t::gcd();

    // Fel Rush's loss of control causes a GCD lag after the loss ends.
    // TOCHECK: Does this delay happen when jump cancelling?
    g += rng().gauss( sim -> gcd_lag, sim -> gcd_lag_stddev );

    return g;
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    // Does not benefit from momentum, so snapshot damage now.
    action_state_t* s = damage -> get_state();
    s -> target = target;
    damage -> snapshot_state( s, DMG_DIRECT );
    damage -> schedule_execute( s );

    // Aug 04 2016: Using Fel Rush puts VR on cooldown for 1 second.
    p() -> cooldown.vengeful_retreat_secondary -> start( timespan_t::from_seconds( 1.0 ) );

    if ( !a_cancel )
    {
      p() -> buff.fel_rush_move -> trigger();
    }

    p() -> buff.momentum -> trigger();
  }

  bool ready() override
  {
    // Aug 04 2016: Using VR puts Fel Rush on cooldown for 1 second.
    if ( p() -> cooldown.fel_rush_secondary -> down() )
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
    fracture_damage_t( const std::string& n, demon_hunter_t* p,
                       const spell_data_t* s )
      : demon_hunter_attack_t( n, p, s )
    {
      background = dual = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  fracture_damage_t* mh, *oh;

  fracture_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "fracture", p, p -> talent.fracture, options_str )
  {
    may_crit          = false;
    weapon_multiplier = 0;

    mh = new fracture_damage_t( "fracture_mh", p,
                                data().effectN( 2 ).trigger() );
    oh = new fracture_damage_t( "fracture_oh", p,
                                data().effectN( 3 ).trigger() );
    mh -> stats = oh -> stats = stats;
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      mh -> target = oh -> target = s -> target;
      mh -> schedule_execute();
      oh -> schedule_execute();

      p() -> spawn_soul_fragment( SOUL_FRAGMENT_LESSER, 2 );
    }
  }
};

// Fury of the Illidari =====================================================
// TODO: Distance targeting support.

struct fury_of_the_illidari_t : public demon_hunter_attack_t
{
  struct rage_of_the_illidari_t : public demon_hunter_attack_t
  {
    rage_of_the_illidari_t( demon_hunter_t* p )
      : demon_hunter_attack_t( "rage_of_the_illidari", p,
                               p -> find_spell( 217070 ) )
    {
      may_crit   = false;
      aoe        = -1;
      background = split_aoe_damage = true;
      base_multiplier =
        p -> artifact.rage_of_the_illidari.data().effectN( 1 ).percent();
    }

    void init() override
    {
      demon_hunter_attack_t::init();

      snapshot_flags = update_flags = 0;
    }

    void execute() override
    {
      // Manually apply base_multiplier since the flag it uses is disabled.
      base_dd_min = base_dd_max =
                      base_multiplier * p() -> buff.rage_of_the_illidari -> check_value();

      demon_hunter_attack_t::execute();

      p() -> buff.rage_of_the_illidari -> expire();
    }
  };

  struct fury_of_the_illidari_tick_t : public demon_hunter_attack_t
  {
    fury_of_the_illidari_tick_t( demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_attack_t( "fury_of_the_illidari_tick", p, s )
    {
      background = dual = ground_aoe = true;
      aoe                  = -1;
      may_proc_fel_barrage = true;  // Jul 12 2016
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      if ( result_is_hit( s -> result ) &&
           p() -> artifact.rage_of_the_illidari.rank() )
      {
        p() -> buff.rage_of_the_illidari -> trigger(
          1,
          p() -> buff.rage_of_the_illidari -> current_value + s -> result_amount );
      }
    }

    dmg_e amount_type( const action_state_t*, bool ) const override
    {
      return DMG_OVER_TIME;
    }  // TOCHECK
  };

  action_t* mh;
  action_t* oh;
  rage_of_the_illidari_t* rage;

  fury_of_the_illidari_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "fury_of_the_illidari", p,
                             p -> artifact.fury_of_the_illidari, options_str ),
      mh( new fury_of_the_illidari_tick_t( p, p -> find_spell( 201628 ) ) ),
      oh( new fury_of_the_illidari_tick_t( p, p -> find_spell( 201789 ) ) ),
      rage( nullptr )
  {
    may_miss = may_crit = may_parry = may_block = may_dodge = false;
    dot_duration   = data().duration();
    base_tick_time = timespan_t::from_millis( 500 );
    tick_zero      = true;

    // Disallow use outside of melee range.
    range = 5.0;

    // Silly reporting things
    school = mh -> school;
    mh -> stats = oh -> stats = stats;

    if ( p -> artifact.rage_of_the_illidari.rank() )
    {
      rage = new rage_of_the_illidari_t( p );
      add_child( rage );
    }
  }

  timespan_t travel_time() const override
  { return timespan_t::zero(); }

  void tick( dot_t* d ) override
  {
    demon_hunter_attack_t::tick( d );

    mh -> schedule_execute();
    oh -> schedule_execute();
  }

  void last_tick( dot_t* d ) override
  {
    demon_hunter_attack_t::last_tick( d );

    if ( p() -> artifact.rage_of_the_illidari.rank() )
    {
      rage -> schedule_execute();
    }
  }
};

// Shear ====================================================================

struct shear_t : public demon_hunter_attack_t
{
  std::array<double, 8> shatter_chance;

  shear_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t(
        "shear", p, p -> find_specialization_spell( "Shear" ), options_str )
  {
    /* Proc chance increases on each consecutive failed attempt, rates
       from http://us.battle.net/wow/en/forum/topic/20743504316?page=4#75 */
    shatter_chance = {{0.04, 0.12, 0.25, 0.40, 0.60, 0.80, 0.90, 1.00}};

    base_multiplier *= 1.0 + p -> artifact.honed_warblades.percent();
  }

  void shatter( action_state_t* /* s */ )
  {
    assert( p() -> shear_counter < 8 );

    double chance = shatter_chance[ p() -> shear_counter ];

    if ( p() -> health_percentage() < 50.0 )
    {
      chance += p() -> artifact.shatter_the_souls.percent();
    }

    if ( p() -> rng().roll( chance ) )
    {
      p() -> spawn_soul_fragment( SOUL_FRAGMENT_LESSER );
      p() -> shear_counter = 0;
    }
    else
    {
      p() -> shear_counter++;
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    ea *= 1.0 + p() -> buff.blade_turning -> check_value();

    return ea;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talent.felblade -> ok() && p() -> rppm.felblade -> trigger() )
      {
        p() -> proc.felblade_reset -> occur();
        p() -> cooldown.felblade -> reset( true );
      }
    }
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( hit_any_target )
    {
      shatter( execute_state );
    }

    p() -> buff.blade_turning -> up();
    p() -> buff.blade_turning -> expire();
  }
};

// Soul Carver ==============================================================

struct soul_carver_t : public demon_hunter_attack_t
{
  struct soul_carver_oh_t : public demon_hunter_attack_t
  {
    soul_carver_oh_t( demon_hunter_t* p )
      : demon_hunter_attack_t(
          "soul_carver_oh", p,
          p -> artifact.soul_carver.data().effectN( 4 ).trigger() )
    {
      background = dual = true;
      may_miss = may_parry = may_dodge = false;  // TOCHECK
    }
  };

  soul_carver_oh_t* oh;

  soul_carver_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "soul_carver", p, p -> artifact.soul_carver,
                             options_str )
  {
    oh        = new soul_carver_oh_t( p );
    oh -> stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      oh -> target = s -> target;
      oh -> schedule_execute();

      p() -> spawn_soul_fragment( SOUL_FRAGMENT_LESSER, 2 );
    }
  }

  void tick( dot_t* d ) override
  {
    demon_hunter_attack_t::tick( d );

    p() -> spawn_soul_fragment( SOUL_FRAGMENT_LESSER );
  }
};

// Soul Cleave ==============================================================

struct soul_cleave_t : public demon_hunter_attack_t
{
  struct soul_cleave_damage_t : public demon_hunter_attack_t
  {
    soul_cleave_damage_t( const std::string& n, demon_hunter_t* p,
                          spell_data_t* s )
      : demon_hunter_attack_t( n, p, s )
    {
      aoe        = 1;
      background = dual = true;

      base_multiplier *= 1.0 + p -> artifact.tormented_souls.percent();
    }
  };

  heals::soul_cleave_heal_t* heal;
  soul_cleave_damage_t* mh;

  soul_cleave_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "soul_cleave", p, p -> spec.soul_cleave,
                             options_str ),
      heal( new heals::soul_cleave_heal_t( p ) )
  {
    may_miss = may_dodge = may_parry = may_block = may_crit = false;
    attack_power_mod.direct = 0;  // This parent action deals no damage;
    base_dd_min = base_dd_max = 0;

    mh = new soul_cleave_damage_t( "soul_cleave_mh", p,
                                   data().effectN( 2 ).trigger() );
    mh -> stats = stats;

    // TOCHECK: How does this really work with regards to the heal multiplier?
    double cost_mod = p -> sets.set( DEMON_HUNTER_VENGEANCE, T19, B2 ) -> effectN( 1 ).resource( RESOURCE_PAIN );

    base_costs[ RESOURCE_PAIN ]      += cost_mod;
    secondary_costs[ RESOURCE_PAIN ] += cost_mod;

    // Add damage modifiers in soul_cleave_damage_t, not here.
  }

  double cost() const override
  {
    resource_e cr = current_resource();

    // Consume pain between min and max cost.
    return clamp( p() -> resources.current[ cr ], base_costs[ cr ], base_cost() );
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    double pain_multiplier = resource_consumed / base_costs[ current_resource() ];

    // Heal happens first.
    action_state_t* heal_state = heal -> get_state();
    heal -> target    = player;
    heal -> snapshot_state( heal_state, HEAL_DIRECT );
    heal_state -> da_multiplier *= pain_multiplier;
    heal -> schedule_execute( heal_state );

    // Damage
    action_state_t* mh_state = mh -> get_state();
    mh -> target      = execute_state -> target;
    mh -> snapshot_state( mh_state, DMG_DIRECT );
    mh_state -> da_multiplier *= pain_multiplier;
    mh -> schedule_execute( mh_state );

    p() -> consume_soul_fragments();

    if ( p() -> legendary.the_defilers_lost_vambraces < timespan_t::zero() )
    {
      unsigned roll =
        as<unsigned>( p() -> rng().range( 0, p() -> sigil_cooldowns.size() ) );
      p() -> sigil_cooldowns[ roll ] -> adjust(
        p() -> legendary.the_defilers_lost_vambraces );
    }

    p() -> cooldown.demon_spikes -> adjust(
      -p() -> sets.set( DEMON_HUNTER_VENGEANCE, T19, B4 )
       -> effectN( 1 )
      .time_value() );
  }
};

// Throw Glaive =============================================================

struct throw_glaive_t : public demon_hunter_attack_t
{
  struct bloodlet_t : public residual_action::residual_periodic_action_t <
    demon_hunter_attack_t >
  {
    bloodlet_t( demon_hunter_t* p )
      : residual_action::residual_periodic_action_t<demon_hunter_attack_t>(
          "bloodlet", p, p -> find_spell( 207690 ) )
    {
      background = dual = proc = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  bloodlet_t* bloodlet;

  throw_glaive_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "throw_glaive", p,
                             p -> find_class_spell( "Throw Glaive" ),
                             options_str ),
      bloodlet( nullptr )
  {
    cooldown -> charges +=
      p -> talent.master_of_the_glaive -> effectN( 2 ).base_value();
    aoe                  = 3;     // Ricochets to 2 additional enemies
    radius               = 10.0;  // with 10 yards.
    may_proc_fel_barrage = true;  // Jun 2 2016

    if ( p -> talent.bloodlet -> ok() )
    {
      bloodlet = new bloodlet_t( p );
      add_child( bloodlet );
    }

    base_multiplier *= 1.0 + p -> artifact.sharpened_glaives.percent();
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( p() -> talent.bloodlet -> ok() && result_is_hit( s -> result ) )
    {
      residual_action::trigger(
        bloodlet, s -> target,
        s -> result_amount * p() -> talent.bloodlet -> effectN( 1 ).percent() );
    }
  }
};

// Vengeful Retreat =========================================================

struct vengeful_retreat_t : public demon_hunter_attack_t
{
  struct vengeful_retreat_damage_t : public demon_hunter_attack_t
  {
    vengeful_retreat_damage_t( demon_hunter_t* p )
      : demon_hunter_attack_t( "vengeful_retreat_dmg", p,
                               p -> find_spell( 198813 ) )
    {
      background = dual = true;
      aoe                  = -1;
      may_proc_fel_barrage = true;  // Jul 12 2016
    }
  };

  vengeful_retreat_damage_t* damage;

  vengeful_retreat_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "vengeful_retreat", p, p -> spec.vengeful_retreat )
  {
    parse_options( options_str );

    may_miss = may_dodge = may_parry = may_crit = may_block = false;
    damage                = new vengeful_retreat_damage_t( p );
    damage -> stats         = stats;
    ignore_false_positive = true;
    // use_off_gcd           = true;
    base_teleport_distance                        = VENGEFUL_RETREAT_DISTANCE;
    p -> buff.vengeful_retreat_move -> distance_moved = base_teleport_distance;
    movement_directionality                       = MOVEMENT_OMNI;

    cooldown -> duration += p -> talent.prepared -> effectN( 2 ).time_value();

    // Add damage modifiers in vengeful_retreat_damage_t, not here.
  }

  // Don't record data for this action.
  void record_data( action_state_t* s ) override
  {
    ( void )s;
    assert( s -> result_amount == 0.0 );
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    p() -> cooldown.fel_rush_secondary -> start( timespan_t::from_seconds( 1.0 ) );

    p() -> buff.vengeful_retreat_move -> trigger();

    if ( hit_any_target )
    {
      // Jul 11 2016: Does not benefit from its own momentum, so snapshot now.
      action_state_t* s = damage -> get_state();
      s -> target         = target;
      damage -> snapshot_state( s, DMG_DIRECT );
      damage -> schedule_execute( s );

      p() -> buff.prepared -> trigger();
    }

    p() -> buff.momentum -> trigger();
  }

  bool ready() override
  {
    if ( p() -> cooldown.vengeful_retreat_secondary -> down() )
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
  demon_hunter_t& dh;
  bool invalidates_damage_calcs;

  demon_hunter_buff_t( demon_hunter_t& p, const buff_creator_basics_t& params )
    : BuffBase( params ), dh( p ), invalidates_damage_calcs( false )
  {
  }

  demon_hunter_buff_t( demon_hunter_t& p, const absorb_buff_creator_t& params )
    : BuffBase( params ), dh( p ), invalidates_damage_calcs( false )
  {
  }

  demon_hunter_t& p() const
  {
    return dh;
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    bb::start( stacks, value, duration );

    if ( invalidates_damage_calcs )
    {
      p().invalidate_damage_calcs();
    }
  }

  virtual void expire_override( int expiration_stacks,
                                timespan_t remaining_duration ) override
  {
    bb::expire_override( expiration_stacks, remaining_duration );

    if ( invalidates_damage_calcs )
    {
      p().invalidate_damage_calcs();
    }
  }

protected:
  typedef demon_hunter_buff_t base_t;

private:
  typedef BuffBase bb;
};

// Anguish ==================================================================

struct anguish_debuff_t : public demon_hunter_buff_t<debuff_t>
{
  action_t* anguish;

  anguish_debuff_t( demon_hunter_t* p, player_t* target )
    : demon_hunter_buff_t<debuff_t>(
        *p, buff_creator_t( actor_pair_t( target, p ), "anguish",
                            p -> artifact.anguish_of_the_deceiver.data()
                            .effectN( 1 )
                            .trigger() ) )
  {
    anguish = p -> find_action( "anguish" );
  }

  virtual void expire_override( int expiration_stacks,
                                timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<debuff_t>::expire_override( expiration_stacks,
        remaining_duration );

    // Only if the debuff expires naturally; if the target dies it doesn't deal
    // damage.
    if ( remaining_duration == timespan_t::zero() )
    {
      // Schedule an execute, but snapshot state right now so we can apply the
      // stack multiplier.
      action_state_t* s = anguish -> get_state();
      s -> target         = player;
      anguish -> snapshot_state( s, DMG_DIRECT );
      s -> target_da_multiplier *= expiration_stacks;
      anguish -> schedule_execute( s );
    }
  }
};

// Chaos Blades =============================================================

struct chaos_blades_t : public demon_hunter_buff_t<buff_t>
{
  chaos_blades_t( demon_hunter_t* p )
    : demon_hunter_buff_t<buff_t>(
        *p, buff_creator_t( p, "chaos_blades", p -> talent.chaos_blades )
        .cd( timespan_t::zero() )
        .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {
  }

  void change_auto_attack( attack_t*& hand, attack_t* a )
  {
    if ( hand == 0 )
      return;

    bool executing         = hand -> execute_event != 0;
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
      hand -> base_execute_time   = timespan_t::zero();
      hand -> schedule_execute();
      hand -> base_execute_time = old_swing_time;
      hand -> execute_event -> reschedule( time_to_hit );
    }
  }

  void execute( int stacks = 1, double value = buff_t::DEFAULT_VALUE(),
                timespan_t duration = timespan_t::min() ) override
  {
    buff_t::execute( stacks, value, duration );

    change_auto_attack( p().main_hand_attack, p().chaos_blade_main_hand );
    if ( p().off_hand_attack )
      change_auto_attack( p().off_hand_attack, p().chaos_blade_off_hand );
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    change_auto_attack( p().main_hand_attack, p().melee_main_hand );
    if ( p().off_hand_attack )
      change_auto_attack( p().off_hand_attack, p().melee_off_hand );
  }
};

// Nemesis ==================================================================

struct nemesis_debuff_t : public demon_hunter_buff_t<debuff_t>
{
  nemesis_debuff_t( demon_hunter_t* p, player_t* target )
    : demon_hunter_buff_t<debuff_t>(
        *p, buff_creator_t( actor_pair_t( target, p ), "nemesis",
                            p -> talent.nemesis )
        .default_value( p -> talent.nemesis -> effectN( 1 ).percent() )
        .cd( timespan_t::zero() ) )
  {
    invalidates_damage_calcs = true;
  }

  virtual void expire_override( int expiration_stacks,
                                timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<debuff_t>::expire_override( expiration_stacks,
        remaining_duration );

    if ( remaining_duration > timespan_t::zero() )
    {
      p().buff.nemesis -> trigger( 1, player -> race, -1.0, remaining_duration );
    }
  }
};

// Metamorphosis Buff =======================================================

struct metamorphosis_buff_t : public demon_hunter_buff_t<buff_t>
{
  static void callback( buff_t* b, int, const timespan_t& )
  {
    demon_hunter_t* p = debug_cast<demon_hunter_t*>( b -> player );
    p -> resource_gain( RESOURCE_PAIN, p -> spec.metamorphosis_buff -> effectN( 4 )
                      .resource( RESOURCE_PAIN ),
                      p -> gain.metamorphosis );
  }

  metamorphosis_buff_t( demon_hunter_t* p )
    : demon_hunter_buff_t<buff_t>(
        *p, buff_creator_t( p, "metamorphosis", p -> spec.metamorphosis_buff )
        .cd( timespan_t::zero() )
        .default_value(
          p -> spec.metamorphosis_buff -> effectN( 2 ).percent() +
          p -> artifact.embrace_the_pain.percent() )
        .period( p -> spec.metamorphosis_buff -> effectN( 4 ).period() )
        .tick_callback( &callback )
        .add_invalidate( CACHE_LEECH ) )
  {
    assert( p -> specialization() == DEMON_HUNTER_VENGEANCE );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    demon_hunter_buff_t<buff_t>::start( stacks, value, duration );

    p().metamorphosis_health = p().max_health() * value;
    p().stat_gain( STAT_MAX_HEALTH, p().metamorphosis_health, ( gain_t* )nullptr,
                   ( action_t* )nullptr, true );
  }

  void expire_override( int expiration_stacks,
                        timespan_t remaining_duration ) override
  {
    demon_hunter_buff_t<buff_t>::expire_override( expiration_stacks,
        remaining_duration );

    p().stat_loss( STAT_MAX_HEALTH, p().metamorphosis_health, ( gain_t* )nullptr,
                   ( action_t* )nullptr, true );
    p().metamorphosis_health = 0;
  }
};

// Soul Barrier buff ========================================================

struct soul_barrier_t : public demon_hunter_buff_t<absorb_buff_t>
{
  soul_barrier_t( demon_hunter_t* p )
    : demon_hunter_buff_t<absorb_buff_t>(
        *p, absorb_buff_creator_t( p, "soul_barrier", p -> talent.soul_barrier )
        .source( p -> get_stats( "soul_barrier" ) )
        .gain( p -> get_gain( "soul_barrier" ) )
        .high_priority( true )  // TOCHECK
        .cd( timespan_t::zero() ) )
  {
  }

  double minimum_absorb() const
  {
    return data().effectN( 3 ).ap_coeff() * p().cache.attack_power();
  }

  // Custom consume implementation to allow minimum absorb amount.
  double consume( double amount ) override
  {
    // Limit the consumption to the current size of the buff.
    amount = std::min( amount, current_value );

    if ( absorb_source )
    {
      absorb_source -> add_result( amount, 0, ABSORB, RESULT_HIT,
                                 BLOCK_RESULT_UNBLOCKED, player );
    }

    if ( absorb_gain )
    {
      absorb_gain -> add( RESOURCE_HEALTH, amount, 0 );
    }

    current_value = std::max( current_value - amount, minimum_absorb() );

    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s %s absorbs %.2f (remaining: %.2f)",
                             player -> name(), name(), amount, current_value );
    }

    absorb_used( amount );

    return amount;
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
    assert( dh -> spirit_bomb >= 0.0 );

    if ( dh -> spirit_bomb > 0 )
    {
      action_t* a    = dh -> active.spirit_bomb_heal;
      a -> base_dd_min = a -> base_dd_max = dh -> spirit_bomb;
      a -> schedule_execute();

      dh -> spirit_bomb = 0.0;
    }

    dh -> spirit_bomb_driver = make_event<spirit_bomb_event_t>( sim(), dh );
  }
};

// Damage Calculation Helper ================================================

struct damage_calc_helper_t
{
  // TODO: Balanced Blades handling.

private:
  bool valid;
  action_t* action;
  action_state_t* state;
  double cached_amount;
  double weapon_damage_multiplier;
  double crit_bonus_damage;
  // First Blood talent multiplier, if relevant to the action.
  double first_blood;
#ifndef NDEBUG
  std::vector<actions::demon_hunter_attack_t*> attacks_;
#endif

public:
  damage_calc_helper_t( std::vector<actions::demon_hunter_attack_t*> attacks )
    : valid( false ),
      action( attacks.at( 0 ) ),
      state( action -> get_state() ),
      first_blood( 1.0 )
  {
    debug_cast<demon_hunter_t*>( action -> player )
     -> damage_calcs.push_back( this );

    // Always calculate the damage of a hit.
    state -> result = RESULT_HIT;

    // Calculate total weapon damage for the whole attack.
    double total_wdmg = 0;

    for ( unsigned i = 0; i < attacks.size(); i++ )
    {
      attack_t* a = attacks.at( i );
      double w    = a -> weapon_multiplier;

      // Off hand damage penalty
      if ( a -> weapon == &( a -> player -> off_hand_weapon ) )
      {
        w *= 0.5;
      }

      total_wdmg += w;
    }

    /* Set multiplier as a ratio of the total damage to our tester action's
    damage. Given
    that attack benefits from all the same modifiers this saves us a lot of
    calculation. */
    weapon_damage_multiplier = total_wdmg / action -> weapon_multiplier;

    crit_bonus_damage = action -> total_crit_bonus( state );

    // Check if the action benefits from First Blood.
    if ( util::str_prefix_ci( action -> name_str, "blade_dance" ) ||
         util::str_prefix_ci( action -> name_str, "death_sweep" ) )
    {
      first_blood =
        debug_cast<actions::attacks::blade_dance_attack_t*>( action )
         -> first_blood_multiplier;
    }

#ifndef NDEBUG
    range::for_each( attacks, [this]( actions::demon_hunter_attack_t* a ) {
      attacks_.push_back( a );
    } );
#endif
  }

  ~damage_calc_helper_t()
  {
    delete state;
  }

  double total()
  {
    if ( !valid )
    {
      cached_amount = calculate();

      valid = true;
    }

#ifndef NDEBUG
    double c = calculate();
    assert( cached_amount == c );
#endif

    return cached_amount;
  }

  void invalidate()
  {
    valid = false;
  }

private:
  double calculate()
  {
    assert( crit_bonus_damage == action -> total_crit_bonus( state ) );

    return get_raw_amount() * calculate_target_multiplier();
  }

  double get_raw_amount()
  {
    action -> snapshot_state( state, DMG_DIRECT );

    // Negate target multipliers.
    state -> target_da_multiplier = state -> target_mitigation_da_multiplier = 1.0;

    double amount = action -> calculate_direct_amount( state );

    // Negate First Blood multiplier.
    if ( state -> chain_target == 0 )
      amount /= first_blood;

    return amount * weapon_damage_multiplier;
  }

  double get_target_mitigation( player_t* t )
  {
    state -> result_amount = 1.0;
    t -> target_mitigation( action -> get_school(), DMG_DIRECT, state );

    return state -> result_amount;
  }

  double get_target_crit_modifier( player_t* t )
  {
    double crit_chance =
      clamp( state -> crit_chance + action -> composite_target_crit_chance( t ),
             0.0, 1.0 );

    crit_chance *= action -> composite_crit_chance_multiplier();

    return 1.0 + ( crit_chance * crit_bonus_damage );
  }

  double calculate_target_multiplier()
  {
    double tm                  = 0;
    std::vector<player_t*>& tl = action -> target_list();
    int size                   = ( int )tl.size();

    for ( int i = 0; i < size && ( action -> aoe < 0 || i < action -> aoe ); i++ )
    {
      player_t* t = tl.at( i );

      double m = get_target_mitigation( t ) *
                 action -> composite_target_multiplier( t ) *
                 get_target_crit_modifier( t );

      if ( action -> chain_multiplier != 1.0 )
      {
        m *= pow( action -> chain_multiplier, i );
      }

      if ( i == 0 )
      {
        m *= first_blood;
      }

      tm += m;
    }

    return tm;
  }
};

struct damage_calc_invalidate_callback_t
{
  demon_hunter_t* dh;

  damage_calc_invalidate_callback_t( demon_hunter_t* p ) : dh( p ) {}

  void operator()( player_t* )
  {
    dh -> invalidate_damage_calcs();
  }
};

// ==========================================================================
// Targetdata Definitions
// ==========================================================================

demon_hunter_td_t::demon_hunter_td_t( player_t* target, demon_hunter_t& p )
  : actor_target_data_t( target, &p ), dots( dots_t() ), debuffs( debuffs_t() )
{
  // Havoc
  debuffs.anguish = new buffs::anguish_debuff_t( &p, target );
  debuffs.nemesis = new buffs::nemesis_debuff_t( &p, target );

  // Vengeance
  dots.fiery_brand    = target -> get_dot( "fiery_brand", &p );
  dots.sigil_of_flame = target -> get_dot( "sigil_of_flame", &p );
  debuffs.frailty =
    buff_creator_t( target, "frailty", p.find_spell( 224509 ) )
    .default_value( p.find_spell( 224509 ) -> effectN( 1 ).percent() );
}

// ==========================================================================
// Demon Hunter Definitions
// ==========================================================================

demon_hunter_t::demon_hunter_t( sim_t* sim, const std::string& name, race_e r )
  : player_t( sim, DEMON_HUNTER, name, r ),
    blade_dance_driver( nullptr ),
    blade_dance_attacks( 0 ),
    death_sweep_attacks( 0 ),
    chaos_strike_attacks( 0 ),
    annihilation_attacks( 0 ),
    damage_calcs( 0 ),
    blade_dance_dmg( nullptr ),
    death_sweep_dmg( nullptr ),
    chaos_strike_dmg( nullptr ),
    annihilation_dmg( nullptr ),
    melee_main_hand( nullptr ),
    melee_off_hand( nullptr ),
    chaos_blade_main_hand( nullptr ),
    chaos_blade_off_hand( nullptr ),
    next_fragment_spawn( 0 ),
    soul_fragments(),
    sigil_cooldowns( 0 ),
    spirit_bomb( 0.0 ),
    spirit_bomb_driver( nullptr ),
    target_reach( -1.0 ),
    exiting_melee( nullptr ),
    initial_fury( 0 ),
    sigil_of_flame_activates( timespan_t::zero() ),
    buff(),
    talent(),
    spec(),
    mastery_spell(),
    cooldown(),
    gain(),
    benefits(),
    proc(),
    active(),
    pets(),
    options(),
    glyphs(),
    legendary()
{
  base.distance = 5.0;

  create_cooldowns();
  create_gains();
  create_benefits();

  regen_type = REGEN_DISABLED;
}

demon_hunter_t::~demon_hunter_t()
{
  delete blade_dance_dmg;
  delete death_sweep_dmg;
  delete chaos_strike_dmg;
  delete annihilation_dmg;
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

  options = source_p -> options;
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
  if ( name == "chaos_blades" )
    return new chaos_blades_t( this, options_str );
  if ( name == "chaos_nova" )
    return new chaos_nova_t( this, options_str );
  if ( name == "consume_magic" )
    return new consume_magic_t( this, options_str );
  if ( name == "demon_spikes" )
    return new demon_spikes_t( this, options_str );
  if ( name == "eye_beam" )
    return new eye_beam_t( this, options_str );
  if ( name == "empower_wards" )
    return new empower_wards_t( this, options_str );
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
  if ( name == "fury_of_the_illidari" )
    return new fury_of_the_illidari_t( this, options_str );
  if ( name == "shear" )
    return new shear_t( this, options_str );
  if ( name == "soul_carver" )
    return new soul_carver_t( this, options_str );
  if ( name == "soul_cleave" )
    return new soul_cleave_t( this, options_str );
  if ( name == "throw_glaive" )
    return new throw_glaive_t( this, options_str );
  if ( name == "vengeful_retreat" )
    return new vengeful_retreat_t( this, options_str );

  return base_t::create_action( name, options_str );
}

// demon_hunter_t::create_actions ===========================================

bool demon_hunter_t::create_actions()
{
  bool ca = player_t::create_actions();

  if ( !blade_dance_attacks.empty() )
  {
    blade_dance_dmg = new damage_calc_helper_t( blade_dance_attacks );
  }

  if ( !death_sweep_attacks.empty() )
  {
    death_sweep_dmg = new damage_calc_helper_t( death_sweep_attacks );
  }

  if ( !chaos_strike_attacks.empty() )
  {
    chaos_strike_dmg = new damage_calc_helper_t( chaos_strike_attacks );
  }

  if ( !annihilation_attacks.empty() )
  {
    annihilation_dmg = new damage_calc_helper_t( annihilation_attacks );
  }

  if ( blade_dance_dmg || death_sweep_dmg || chaos_strike_dmg ||
       annihilation_dmg )
  {
    sim -> target_non_sleeping_list.register_callback(
      damage_calc_invalidate_callback_t( this ) );
  }

  return ca;
}

// demon_hunter_t::create_buffs =============================================

void demon_hunter_t::create_buffs()
{
  base_t::create_buffs();

  using namespace buffs;

  // General

  buff.demon_soul =
    buff_creator_t( this, "demon_soul", find_spell( 208195 ) )
    .default_value( find_spell( 208195 ) -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    buff.metamorphosis =
      buff_creator_t( this, "metamorphosis", spec.metamorphosis_buff )
      .cd( timespan_t::zero() )
      .add_invalidate( CACHE_LEECH )
      .add_invalidate( CACHE_HASTE )
      .tick_behavior( BUFF_TICK_NONE )
      .period( timespan_t::zero() )
      .default_value( spec.metamorphosis_buff -> effectN( 7 ).percent() );
  }
  else
  {
    buff.metamorphosis = new metamorphosis_buff_t( this );
  }

  // Havoc

  buff.blade_dance =
    buff_creator_t( this, "blade_dance", spec.blade_dance )
    .default_value( spec.blade_dance -> effectN( 2 ).percent() )
    .add_invalidate( CACHE_DODGE )
    .cd( timespan_t::zero() );

  buff.blur =
    buff_creator_t( this, "blur", spec.blur -> effectN( 1 ).trigger() )
    .default_value(
      spec.blur -> effectN( 1 ).trigger() -> effectN( 3 ).percent() )
    .cd( timespan_t::zero() )
    .add_invalidate( CACHE_LEECH );

  buff.chaos_blades = new buffs::chaos_blades_t( this );

  buff.death_sweep =
    buff_creator_t( this, "death_sweep", spec.death_sweep )
    .default_value( spec.death_sweep -> effectN( 2 ).percent() )
    .add_invalidate( CACHE_DODGE )
    .cd( timespan_t::zero() );

  buff.fel_rush_move = new movement_buff_t(
    this, buff_creator_t( this, "fel_rush_movement", spell_data_t::nil() )
    .chance( 1.0 )
    .duration( find_class_spell( "Fel Rush" ) -> gcd() ) );

  buff.momentum =
    buff_creator_t( this, "momentum", find_spell( 208628 ) )
    .default_value( find_spell( 208628 ) -> effectN( 1 ).percent() )
    .trigger_spell( talent.momentum )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.out_of_range =
    buff_creator_t( this, "out_of_range", spell_data_t::nil() ).chance( 1.0 );

  // TODO: Buffs for each race?
  buff.nemesis = buff_creator_t( this, "nemesis_buff", find_spell( 208605 ) )
                 .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.prepared =
    buff_creator_t( this, "prepared",
                    talent.prepared -> effectN( 1 ).trigger() )
    .trigger_spell( talent.prepared )
    .default_value(
      talent.prepared -> effectN( 1 ).trigger() -> effectN( 1 ).resource(
        RESOURCE_FURY ) *
      ( 1.0 +
        sets.set( DEMON_HUNTER_HAVOC, T19, B2 )
         -> effectN( 1 )
        .percent() ) )
  .tick_callback( [this]( buff_t* b, int, const timespan_t& ) {
    resource_gain( RESOURCE_FURY, b -> check_value(), gain.prepared );
  } );

  buff.rage_of_the_illidari =
    buff_creator_t( this, "rage_of_the_illidari", find_spell( 217060 ) );

  buff.vengeful_retreat_move = new movement_buff_t(
    this,
    buff_creator_t( this, "vengeful_retreat_movement", spell_data_t::nil() )
    .chance( 1.0 )
    .duration( spec.vengeful_retreat -> duration() ) );

  // Vengeance
  buff.blade_turning =
    buff_creator_t( this, "blade_turning",
                    talent.blade_turning -> effectN( 1 ).trigger() )
    .default_value( talent.blade_turning -> effectN( 1 )
                    .trigger()
                     -> effectN( 1 )
                    .percent() )
    .trigger_spell( talent.blade_turning );

  buff.defensive_spikes =
    buff_creator_t( this, "defensive_spikes",
                    artifact.defensive_spikes.data().effectN( 1 ).trigger() )
    .trigger_spell( artifact.defensive_spikes )
    .default_value(artifact.defensive_spikes.data().effectN( 1 ).trigger()
      -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PARRY );

  buff.demon_spikes =
    buff_creator_t( this, "demon_spikes", find_spell( 203819 ) )
    .default_value( find_spell( 203819 ) -> effectN( 1 ).percent() )
    .refresh_behavior( BUFF_REFRESH_EXTEND )
    .add_invalidate( CACHE_PARRY )
    .add_invalidate( CACHE_LEECH )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.empower_wards =
    buff_creator_t( this, "empower_wards",
                    find_specialization_spell( "Empower Wards" ) )
    .default_value( find_specialization_spell( "Empower Wards" )
                     -> effectN( 1 )
                    .percent() );

  buff.immolation_aura =
    buff_creator_t( this, "immolation_aura", spec.immolation_aura )
  .tick_callback( [this]( buff_t*, int, const timespan_t& ) {
    active.immolation_aura -> schedule_execute();
  } )
  .default_value( talent.agonizing_flames -> effectN( 2 ).percent() )
  .add_invalidate( CACHE_RUN_SPEED )
  .cd( timespan_t::zero() );

  buff.painbringer =
    buff_creator_t( this, "painbringer", find_spell( 212988 ) )
    .trigger_spell( artifact.painbringer )
    .default_value( find_spell( 212988 ) -> effectN( 1 ).percent() );

  buff.siphon_power =
    buff_creator_t( this, "siphoned_power", find_spell( 218561 ) )
    .trigger_spell( artifact.siphon_power )
    .add_invalidate( CACHE_AGILITY )
    .max_stack( artifact.siphon_power.value() ? artifact.siphon_power.value() : 1 );

  buff.soul_barrier = new buffs::soul_barrier_t( this );
}

std::string parse_abbreviation( const std::string& s )
{
  if ( s == "cs" )
    return "chaos_strike";
  if ( s == "bd" || s == "dance" )
    return "blade_dance";
  if ( s == "sweep" )
    return "death_sweep";
  if ( s == "annihilate" )
    return "annihilation";

  return s;
}

struct db_ratio_expr_t : public expr_t
{
  action_t* action;
  actions::attacks::demons_bite_t* db;
  bool reduced_by_crit;

  db_ratio_expr_t( demon_hunter_t* p, const std::string& n, action_t* a )
    : expr_t( n ), action( a )
  {
    db = debug_cast<actions::attacks::demons_bite_t*>(
           p -> find_action( "demons_bite" ) );
    reduced_by_crit = util::str_compare_ci( a -> name_str, "chaos_strike" ) ||
                      util::str_compare_ci( a -> name_str, "annihilation" );
  }

  double evaluate() override
  {
    double eff_cost = action -> base_costs[ RESOURCE_FURY ];

    if ( reduced_by_crit )
    {
      eff_cost *= 1.0 - action -> composite_crit_chance();
    }

    double db_gain = db -> energize_amount + ( 1 + db -> energize_die_sides ) / 2.0;
    return eff_cost / db_gain;
  }
};

struct blade_dance_expr_t : public expr_t
{
  demon_hunter_t& dh;
  actions::attacks::demons_bite_t* demons_bite;
  action_state_t* db_state;
  double db_fury;  // Average amount of fury generated by a use of Demon's Bite.
  double dbs_per_use;  // Average number of DBs needed to fund a Blade Dance.
  action_t* fury_spender;  // Form-appropriate fury spending action.
  damage_calc_helper_t* dance_damage, *spender_damage;

  blade_dance_expr_t( action_t* action )
    : expr_t( "" ),
      dh( *debug_cast<demon_hunter_t*>( action -> player ) ),
      demons_bite( debug_cast<actions::attacks::demons_bite_t*>(
                     dh.find_action( "demons_bite" ) ) ),
      db_state( demons_bite -> get_state() )
  {
    db_fury = demons_bite -> energize_amount +
              ( 1.0 + demons_bite -> energize_die_sides ) / 2.0;
    dbs_per_use = action -> base_costs[ RESOURCE_FURY ] / db_fury;

    if ( util::str_compare_ci( action -> name_str, "blade_dance" ) )
    {
      dance_damage   = dh.blade_dance_dmg;
      fury_spender   = dh.find_action( "chaos_strike" );
      spender_damage = dh.chaos_strike_dmg;
    }
    else
    {
      dance_damage   = dh.death_sweep_dmg;
      fury_spender   = dh.find_action( "annihilation" );
      spender_damage = dh.annihilation_dmg;
    }
  }

  ~blade_dance_expr_t()
  {
    delete db_state;
  }

  double dbs_per_spender()
  {
    double eff_cost = fury_spender -> base_costs[ RESOURCE_FURY ];
    eff_cost -=
      dh.spec.chaos_strike_refund -> effectN( 1 ).resource( RESOURCE_FURY ) *
      fury_spender -> composite_crit_chance();

    return eff_cost / db_fury;
  }

  double demons_bite_damage()
  {
    demons_bite -> snapshot_state( db_state, DMG_DIRECT );
    db_state -> result_amount = demons_bite -> calculate_direct_amount( db_state );
    db_state -> target -> target_mitigation( demons_bite -> get_school(), DMG_DIRECT,
                                         db_state );
    double amount = db_state -> result_amount;

    amount *= 1.0 +
              clamp( db_state -> crit_chance + db_state -> target_crit_chance, 0.0,
                     1.0 ) *
              demons_bite -> composite_player_critical_multiplier( db_state );

    return amount;
  }

  double evaluate() override
  {
    double db_damage       = demons_bite_damage();
    double dbs_per_spender = this -> dbs_per_spender();

    // Evaluate total damage per GCD of both sides of the equation.

    // Sum Blade Dance damage and Demon's Bite damage, divide by GCDs.
    double left = ( dance_damage -> total() + dbs_per_use * db_damage ) /
                  ( 1 + dbs_per_use );

    // Sum Fury Spender and Demon's Bite damage, divide by GCDs.
    double right = ( spender_damage -> total() + dbs_per_spender * db_damage ) /
                   ( 1 + dbs_per_spender );

    return left > right;
  }
};

struct damage_calc_expr_t : public expr_t
{
  damage_calc_helper_t* calc;

  damage_calc_expr_t( const std::string& name_str, damage_calc_helper_t* c )
    : expr_t( name_str ), calc( c )
  {
  }

  double evaluate() override
  {
    return calc -> total();
  }
};

// demon_hunter_t::create_expression ========================================

expr_t* demon_hunter_t::create_expression( action_t* a,
    const std::string& name_str )
{
  if ( name_str == "greater_soul_fragments" ||
       name_str == "lesser_soul_fragments" || name_str == "soul_fragments" )
  {
    struct soul_fragments_expr_t : public expr_t
    {
      demon_hunter_t* dh;
      soul_fragment_e type;

      soul_fragments_expr_t( demon_hunter_t* p, const std::string& n,
                             soul_fragment_e t )
        : expr_t( n ), dh( p ), type( t )
      {
      }

      virtual double evaluate() override
      {
        return dh -> get_active_soul_fragments( type );
      }
    };

    soul_fragment_e type;
    if ( name_str == "soul_fragments" )
    {
      type = SOUL_FRAGMENT_ALL;
    }
    else if ( name_str == "greater_soul_fragments" )
    {
      type = SOUL_FRAGMENT_GREATER;
    }
    else
    {
      type = SOUL_FRAGMENT_LESSER;
    }

    return new soul_fragments_expr_t( this, name_str, type );
  }
  else if ( util::str_prefix_ci( name_str, "db_per_" ) ||
            util::str_prefix_ci( name_str, "demons_bite_per_" ) )
  {
    unsigned ofs = util::str_prefix_ci( name_str, "db_per_" ) ? 7 : 16;
    const std::string& action_str = parse_abbreviation(
                                      std::string( name_str.begin() + ofs, name_str.end() ) );

    action_t* action = find_action( action_str );

    if ( action && action -> base_costs[ RESOURCE_FURY ] > 0 )
    {
      return new db_ratio_expr_t( this, name_str, action );
    }
  }
  else if ( util::str_in_str_ci( name_str, "_worth_using" ) )
  {
    const std::string& action_str = parse_abbreviation(
                                      std::string( name_str.begin(), name_str.end() - 12 ) );

    if ( util::str_compare_ci( action_str, "blade_dance" ) ||
         util::str_compare_ci( action_str, "death_sweep" ) )
    {
      action_t* action = find_action( action_str );

      if ( action )
      {
        return new blade_dance_expr_t( action );
      }
    }
  }
  else if ( util::str_in_str_ci( name_str, "_damage" ) )
  {
    const std::string& action_str = parse_abbreviation(
                                      std::string( name_str.begin(), name_str.end() - 7 ) );

    action_t* action = find_action( action_str );

    if ( action )
    {
      if ( util::str_compare_ci( action_str, "blade_dance" ) )
      {
        return new damage_calc_expr_t( name_str, blade_dance_dmg );
      }
      else if ( util::str_compare_ci( action_str, "death_sweep" ) )
      {
        return new damage_calc_expr_t( name_str, death_sweep_dmg );
      }
      else if ( util::str_compare_ci( action_str, "chaos_strike" ) )
      {
        return new damage_calc_expr_t( name_str, chaos_strike_dmg );
      }
      else if ( util::str_compare_ci( action_str, "annihilation" ) )
      {
        return new damage_calc_expr_t( name_str, annihilation_dmg );
      }
    }
  }

  return player_t::create_expression( a, name_str );
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

  sim -> errorf( "No demon hunter pet with name '%s' available.",
               pet_name.c_str() );

  return nullptr;
}

// demon_hunter_t::create_profile ===========================================

std::string demon_hunter_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  // Log all options here

  return profile_str;
}

// demon_hunter_t::has_t18_class_trinket ====================================

bool demon_hunter_t::has_t18_class_trinket() const
{
  return false;
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
      sim -> errorf(
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
  base_t::init_base_stats();

  switch ( specialization() )
  {
    case DEMON_HUNTER_HAVOC:
      resources.base[ RESOURCE_FURY ] = 100 + artifact.contained_fury.value();
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
  proc.delayed_aa_range     = get_proc( "delayed_swing__out_of_range" );
  proc.delayed_aa_channel   = get_proc( "delayed_swing__channeling" );
  proc.soul_fragment        = get_proc( "soul_fragment" );
  proc.soul_fragment_lesser = get_proc( "soul_fragment_lesser" );
  proc.felblade_reset       = get_proc( "felblade_reset" );

  // Havoc
  proc.demon_blades_wasted = get_proc( "demon_blades_wasted" );
  proc.demonic_appetite    = get_proc( "demonic_appetite" );
  proc.demons_bite_in_meta = get_proc( "demons_bite_in_meta" );
  proc.fel_barrage         = get_proc( "fel_barrage" );

  // Vengeance
  proc.fueled_by_pain         = get_proc( "fueled_by_pain" );
  proc.soul_fragment_expire   = get_proc( "soul_fragment_expire" );
  proc.soul_fragment_overflow = get_proc( "soul_fragment_overflow" );
}

// demon_hunter_t::init_resources ===========================================

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );

  if ( artifact.will_of_the_illidari.rank() )
  {
    recalculate_resource_max( RESOURCE_HEALTH );
    resources.initial[ RESOURCE_HEALTH ] =
      resources.current[ RESOURCE_HEALTH ] = resources.max[ RESOURCE_HEALTH ];
  }

  resources.current[ RESOURCE_FURY ] = initial_fury;
  resources.current[ RESOURCE_PAIN ] = 0;
  expected_max_health = calculate_expected_max_health();
}

// demon_hunter_t::init_rng =================================================

void demon_hunter_t::init_rng()
{
  // RPPM objects

  // General
  rppm.felblade = get_rppm( "felblade", find_spell( 203557 ) );

  // Havoc
  rppm.inner_demons = get_rppm( "inner_demons", artifact.inner_demons );

  // Vengeance
  rppm.fueled_by_pain = get_rppm( "fueled_by_pain", artifact.fueled_by_pain );

  player_t::init_rng();
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

  // Specialization =========================================================

  // General
  spec.demon_hunter           = find_class_spell( "Demon Hunter" );
  spec.consume_magic          = find_class_spell( "Consume Magic" );
  spec.consume_soul           = find_spell( 210042 );
  spec.consume_soul_lesser    = find_spell( 203794 );
  spec.critical_strikes       = find_spell( 221351 );  // not a class spell
  spec.leather_specialization = specialization() == DEMON_HUNTER_HAVOC
                                ? find_spell( 178976 )
                                : find_spell( 226359 );
  spec.metamorphosis_buff = specialization() == DEMON_HUNTER_HAVOC
                            ? find_spell( 162264 )
                            : find_spell( 187827 );
  spec.soul_fragment = find_spell( 204255 );

  // Havoc
  spec.havoc               = find_specialization_spell( "Havoc Demon Hunter" );
  spec.annihilation        = find_spell( 201427 );
  spec.blade_dance         = find_class_spell( "Blade Dance" );
  spec.blur                = find_class_spell( "Blur" );
  spec.chaos_nova          = find_class_spell( "Chaos Nova" );
  spec.chaos_strike        = find_class_spell( "Chaos Strike" );
  spec.chaos_strike_refund = find_spell( 197125 );
  spec.death_sweep         = find_spell( 210152 );
  spec.fel_rush_damage     = find_spell( 192611 );
  spec.vengeful_retreat    = find_class_spell( "Vengeful Retreat" );

  // Vengeance
  spec.vengeance       = find_specialization_spell( "Vengeance Demon Hunter" );
  spec.demonic_wards   = find_specialization_spell( "Demonic Wards" );
  spec.fiery_brand_dr  = find_spell( 209245 );
  spec.immolation_aura = find_specialization_spell( "Immolation Aura" );
  spec.riposte         = find_specialization_spell( "Riposte" );
  spec.soul_cleave     = find_specialization_spell( "Soul Cleave" );

  // Masteries ==============================================================

  mastery_spell.demonic_presence = find_mastery_spell( DEMON_HUNTER_HAVOC );
  mastery_spell.fel_blood        = find_mastery_spell( DEMON_HUNTER_VENGEANCE );

  // Talents ================================================================

  // General
  talent.fel_eruption = find_talent_spell( "Fel Eruption" );

  talent.felblade = find_talent_spell( "Felblade" );

  talent.soul_rending = find_talent_spell( "Soul Rending" );

  // Havoc
  talent.fel_mastery      = find_talent_spell( "Fel Mastery" );
  talent.demonic_appetite = find_talent_spell( "Demonic Appetite" );
  talent.blind_fury       = find_talent_spell( "Blind Fury" );

  talent.prepared     = find_talent_spell( "Prepared" );
  talent.chaos_cleave = find_talent_spell( "Chaos Cleave" );

  talent.first_blood = find_talent_spell( "First Blood" );
  talent.bloodlet    = find_talent_spell( "Bloodlet" );

  talent.netherwalk          = find_talent_spell( "Netherwalk" );
  talent.desperate_instincts = find_talent_spell( "Desperate Instincts" );

  talent.momentum = find_talent_spell( "Momentum" );
  talent.demonic  = find_talent_spell( "Demonic" );
  talent.nemesis  = find_talent_spell( "Nemesis" );

  talent.master_of_the_glaive = find_talent_spell( "Master of the Glaive" );
  talent.unleashed_power      = find_talent_spell( "Unleashed Power" );
  talent.demon_blades         = find_talent_spell( "Demon Blades" );

  talent.chaos_blades = find_talent_spell( "Chaos Blades" );
  talent.fel_barrage  = find_talent_spell( "Fel Barrage" );
  talent.demon_reborn = find_talent_spell( "Demon Reborn" );

  // Vengeance
  talent.abyssal_strike   = find_talent_spell( "Abyssal Strike" );
  talent.agonizing_flames = find_talent_spell( "Agonizing Flames" );
  talent.razor_spikes     = find_talent_spell( "Razor Spikes" );

  talent.feast_of_souls = find_talent_spell( "Feast of Souls" );
  talent.fallout        = find_talent_spell( "Fallout" );
  talent.burning_alive  = find_talent_spell( "Burning Alive" );

  // talent.felblade
  talent.flame_crash = find_talent_spell( "Flame Crash" );
  // talent.fel_eruption

  talent.feed_the_demon = find_talent_spell( "Feed the Demon" );
  talent.fracture       = find_talent_spell( "Fracture" );
  // talent.soul_rending

  talent.concentrated_sigils = find_talent_spell( "Concentrated Sigils" );
  talent.sigil_of_chains     = find_talent_spell( "Sigil of Chains" );
  talent.quickened_sigils    = find_talent_spell( "Quickened Sigils" );

  talent.fel_devastation = find_talent_spell( "Fel Devastation" );
  talent.blade_turning   = find_talent_spell( "Blade Turning" );
  talent.spirit_bomb     = find_talent_spell( "Spirit Bomb" );

  talent.last_resort  = find_talent_spell( "Last Resort" );
  talent.nether_bond  = find_talent_spell( "Nether Bond" );
  talent.soul_barrier = find_talent_spell( "Soul Barrier" );

  // Artifacts ==============================================================

  // Havoc -- Twinblades of the Deceiver
  artifact.anguish_of_the_deceiver =
    find_artifact_spell( "Anguish of the Deceiver" );
  artifact.balanced_blades      = find_artifact_spell( "Balanced Blades" );
  artifact.chaos_vision         = find_artifact_spell( "Chaos Vision" );
  artifact.contained_fury       = find_artifact_spell( "Contained Fury" );
  artifact.critical_chaos       = find_artifact_spell( "Critical Chaos" );
  artifact.deceivers_fury       = find_artifact_spell( "Deceiver's Fury" );
  artifact.demon_rage           = find_artifact_spell( "Demon Rage" );
  artifact.demon_speed          = find_artifact_spell( "Demon Speed" );
  artifact.feast_on_the_souls   = find_artifact_spell( "Feast on the Souls" );
  artifact.fury_of_the_illidari = find_artifact_spell( "Fury of the Illidari" );
  artifact.illidari_knowledge   = find_artifact_spell( "Illidari Knowledge" );
  artifact.inner_demons         = find_artifact_spell( "Inner Demons" );
  artifact.overwhelming_power   = find_artifact_spell( "Overwhelming Power" );
  artifact.rage_of_the_illidari = find_artifact_spell( "Rage of the Illidari" );
  artifact.sharpened_glaives    = find_artifact_spell( "Sharpened Glaives" );
  artifact.unleashed_demons     = find_artifact_spell( "Unleashed Demons" );
  artifact.warglaives_of_chaos  = find_artifact_spell( "Warglaives of Chaos" );

  // Vengeance -- The Aldrachi Warblades
  artifact.aldrachi_design      = find_artifact_spell( "Aldrachi Design" );
  artifact.aura_of_pain         = find_artifact_spell( "Aura of Pain" );
  artifact.charred_warblades    = find_artifact_spell( "Charred Warblades" );
  artifact.defensive_spikes     = find_artifact_spell( "Defensive Spikes" );
  artifact.demonic_flames       = find_artifact_spell( "Demonic Flames" );
  artifact.devour_souls         = find_artifact_spell( "Devour Souls" );
  artifact.embrace_the_pain     = find_artifact_spell( "Embrace the Pain" );
  artifact.fiery_demise         = find_artifact_spell( "Fiery Demise" );
  artifact.fueled_by_pain       = find_artifact_spell( "Fueled by Pain" );
  artifact.honed_warblades      = find_artifact_spell( "Honed Warblades" );
  artifact.infernal_force       = find_artifact_spell( "Infernal Force" );
  artifact.painbringer          = find_artifact_spell( "Painbringer" );
  artifact.shatter_the_souls    = find_artifact_spell( "Shatter the Souls" );
  artifact.siphon_power         = find_artifact_spell( "Siphon Power" );
  artifact.soul_carver          = find_artifact_spell( "Soul Carver" );
  artifact.tormented_souls      = find_artifact_spell( "Tormented Souls" );
  artifact.will_of_the_illidari = find_artifact_spell( "Will of the Illidari" );

  // Spell Initialization ===================================================

  if ( talent.demonic_appetite -> ok() )
  {
    spec.demonic_appetite_fury = find_spell( 210041 );
    cooldown.demonic_appetite -> duration =
      talent.demonic_appetite -> internal_cooldown();
  }

  if ( talent.fel_barrage -> ok() )
  {
    spec.fel_barrage_proc = find_spell( 222703 );

    /* Sep 23 2016: Via hotfix, this ability was changed in some mysterious
    unknown manner. Proc mechanic is now totally serverside.
    http://blue.mmo-champion.com/topic/767333-hotfixes-september-16/

    Assuming proc chance and ICD are unchanged and that ICD starts on-attempt
    instead of on-success now.

    As a result, chance and ICD are now hardcoded (they are no longer in the
    spell data).*/

    /* Subsequent Fury of the Illidari ticks (500ms interval) cannot proc,
    so use 501ms instead of 500. */
    cooldown.fel_barrage_proc -> duration = timespan_t::from_millis( 501 );
  }

  using namespace actions::attacks;
  using namespace actions::spells;
  using namespace actions::heals;

  active.consume_soul_greater = new consume_soul_t(
    this, "consume_soul", spec.consume_soul, SOUL_FRAGMENT_GREATER );
  active.consume_soul_lesser =
    new consume_soul_t( this, "consume_soul_lesser", spec.consume_soul_lesser,
                        SOUL_FRAGMENT_LESSER );

  if ( talent.chaos_blades -> ok() )
  {
    const spell_data_t* mh_spell =
      find_spell( talent.chaos_blades -> effectN( 1 ).misc_value1() );

    // Not linked via a trigger, so assert that the spell is there.
    assert( mh_spell -> ok() );

    chaos_blade_main_hand =
      new chaos_blade_t( "chaos_blade_mh", this, mh_spell );

    chaos_blade_off_hand = new chaos_blade_t(
      "chaos_blade_oh", this, talent.chaos_blades -> effectN( 1 ).trigger() );
  }

  if ( talent.demon_blades -> ok() )
  {
    active.demon_blades = new demon_blades_t( this );
  }

  if ( artifact.inner_demons.rank() )
  {
    active.inner_demons = new inner_demons_t( this );
  }

  if ( artifact.anguish_of_the_deceiver.rank() )
  {
    active.anguish = new anguish_t( this );
  }

  if ( artifact.charred_warblades.rank() )
  {
    active.charred_warblades = new charred_warblades_t( this );
  }

  if ( talent.sigil_of_chains -> ok() )
  {
    sigil_cooldowns.push_back( cooldown.sigil_of_chains );
  }

  sigil_delay = find_specialization_spell( "Sigil of Flame" ) -> duration();

  if ( talent.quickened_sigils -> ok() )
  {
    sigil_delay += talent.quickened_sigils -> effectN( 1 ).time_value();
  }
}

// demon_hunter_t::invalidate_cache =========================================

void demon_hunter_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( mastery_spell.demonic_presence -> ok() )
        invalidate_cache( CACHE_RUN_SPEED );
      if ( buff.chaos_blades -> check() )
        invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;
    case CACHE_CRIT_CHANCE:
      if ( spec.riposte -> ok() )
        invalidate_cache( CACHE_PARRY );
      break;
    case CACHE_RUN_SPEED:
      adjust_movement();
      break;
    default:
      break;
  }

  // Expression stuff
  switch ( c )
  {
    case CACHE_ATTACK_POWER:
    case CACHE_CRIT_CHANCE:
    case CACHE_DAMAGE_VERSATILITY:
    case CACHE_PLAYER_DAMAGE_MULTIPLIER:
      if ( blade_dance_dmg )
        blade_dance_dmg -> invalidate();
      if ( death_sweep_dmg )
        death_sweep_dmg -> invalidate();
    case CACHE_MASTERY:
      if ( chaos_strike_dmg )
        chaos_strike_dmg -> invalidate();
      if ( annihilation_dmg )
        annihilation_dmg -> invalidate();
      break;
    default:
      break;
  }
}

// demon_hunter_t::primary_resource =========================================

resource_e demon_hunter_t::primary_resource() const
{
  switch ( specialization() )
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

// ==========================================================================
// custom demon_hunter_t init functions
// ==========================================================================

// demon_hunter_t::apl_precombat ============================================

void demon_hunter_t::apl_precombat()
{
  action_priority_list_t* pre = get_action_priority_list( "precombat" );

  // Flask or Elixir
  if ( sim -> allow_flasks )
  {
    if ( true_level > 100 )
      pre -> add_action( "flask,type=flask_of_the_seventh_demon" );
    else
      pre -> add_action( "flask,type=greater_draenic_agility_flask" );
  }

  // Food
  if ( sim -> allow_food )
  {
    if ( true_level > 100 )
      pre -> add_action( "food,type=the_hungry_magister" );
    else
      pre -> add_action( "food,type=pickled_eel" );
  }

  // Augmentation Rune
  if ( true_level > 100 )
    pre -> add_action( "augmentation,type=defiled" );

  // Snapshot Stats
  pre -> add_action( "snapshot_stats",
                   "Snapshot raid buffed stats before combat begins and "
                   "pre-potting is done." );

  // Pre-Potion
  if ( sim -> allow_potions )
  {
    std::string potion;

    if ( specialization() == DEMON_HUNTER_HAVOC )
      potion = true_level > 100 ? "old_war" : "draenic_agility";
    else
      potion = true_level > 100 ? "unbending_potion" : "draenic_versatility";

    pre -> add_action( "potion,name=" + potion );

    if ( specialization() == DEMON_HUNTER_HAVOC )
    {
      pre -> add_action( this, "Metamorphosis" );
    }
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
  for ( size_t i = 0; i < p -> items.size(); i++ )
  {
    if ( p -> items[ i ].has_use_special_effect() &&
      p -> items[ i ].special_effect().source != SPECIAL_EFFECT_SOURCE_ADDON )
    {
      std::string line = std::string( "use_item,slot=" ) + p -> items[ i ].slot_name();
      if ( util::str_compare_ci( p -> items[ i ].name_str,
                                 "tiny_oozeling_in_a_jar" ) )
      {
        line += ",if=buff.congealing_goo.react=6|(buff.chaos_blades.up&buff.chaos_blades.remains<5&"
          "cooldown.chaos_blades.remains&buff.congealing_goo.up)";
      }
      else
      {
        timespan_t use_cd = p -> items[ i ].special_effect().cooldown();

        if ( use_cd > timespan_t::zero() &&
          p -> talent.chaos_blades -> cooldown() % use_cd == timespan_t::zero() )
        {
          line += ",if=buff.chaos_blades.up|!talent.chaos_blades.enabled";

          if ( use_cd < p -> talent.chaos_blades -> cooldown() )
          {
            char buffer[ 8 ];
            snprintf( buffer, sizeof( buffer ), "%.0f", use_cd.total_seconds() );
            line += "|cooldown.chaos_blades.remains>=" + std::string( buffer );
          }
        }
      }

      apl -> add_action( line );
    }
  }
}

void demon_hunter_t::apl_havoc()
{
  talent_overrides_str +=
    "/fel_barrage,if=active_enemies>1|raid_event.adds.exists";

  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "auto_attack" );
  def -> add_action( "variable,name=pooling_for_meta,value=cooldown.metamorphosis.ready&"
    "buff.metamorphosis.down&(!talent.demonic.enabled|!cooldown.eye_beam.ready)&"
    "(!talent.chaos_blades.enabled|cooldown.chaos_blades.ready)&(!talent.nemesis.enabled|"
    "debuff.nemesis.up|cooldown.nemesis.ready)",
    "\"Getting ready to use meta\" conditions, this is used in a few places." );
  def -> add_action( "variable,name=blade_dance,value=talent.first_blood.enabled|"
    "spell_targets.blade_dance1>=2+talent.chaos_cleave.enabled",
    "Blade Dance conditions. Always if First Blood is talented, otherwise 3+ targets with Chaos "
    "Cleave or 2+ targets without." );
  def -> add_action( "variable,name=pooling_for_blade_dance,value=variable.blade_dance&"
    "fury-40<35-talent.first_blood.enabled*20&spell_targets.blade_dance1>=2",
    "Blade Dance pooling condition, so we don't spend too much fury when we need it soon. No need "
    "to pool on\n# single target since First Blood already makes it cheap enough and delaying it a"
    " tiny bit isn't a big deal." );
  def -> add_action( this, "Blur", "if=artifact.demon_speed.enabled&"
    "cooldown.fel_rush.charges_fractional<0.5&"
    "cooldown.vengeful_retreat.remains-buff.momentum.remains>4" );
  def -> add_action( "call_action_list,name=cooldown" );
  def -> add_action( this, "Fel Rush", "animation_cancel=1,if=time=0",
    "Fel Rush in at the start of combat." );
  def -> add_action(
    "pick_up_fragment,if=talent.demonic_appetite.enabled&fury.deficit>=30" );
  def -> add_action( this, "Consume Magic" );
  def -> add_action(
    this, "Vengeful Retreat", "if=(talent.prepared.enabled|talent.momentum.enabled)&"
    "buff.prepared.down&buff.momentum.down",
    "Vengeful Retreat backwards through the target to minimize downtime." );
  def -> add_action( this, "Fel Rush", "animation_cancel=1,if=(talent.momentum.enabled|"
    "talent.fel_mastery.enabled)&(!talent.momentum.enabled|(charges=2|"
    "cooldown.vengeful_retreat.remains>4)&buff.momentum.down)&(!talent.fel_mastery.enabled|"
    "fury.deficit>=25)&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))",
    "Fel Rush for Momentum and for fury from Fel Mastery." );
  def -> add_talent( this, "Fel Barrage", "if=charges>=5&(buff.momentum.up|"
    "!talent.momentum.enabled)&(active_enemies>desired_targets|raid_event.adds.in>30)",
    "Use Fel Barrage at max charges, saving it for Momentum and adds if possible." );
  def -> add_action( this, "Throw Glaive", "if=talent.bloodlet.enabled&(!talent.momentum.enabled|"
    "buff.momentum.up)&charges=2" );
  def -> add_action( this, artifact.fury_of_the_illidari, "fury_of_the_illidari",
    "if=active_enemies>desired_targets|raid_event.adds.in>55&(!talent.momentum.enabled|"
    "buff.momentum.up)" );
  def -> add_action( this, "Eye Beam", "if=talent.demonic.enabled&buff.metamorphosis.down&"
    "fury.deficit<30" );
  def -> add_action( this, spec.death_sweep, "death_sweep", "if=variable.blade_dance" );
  def -> add_action( this, "Blade Dance", "if=variable.blade_dance" );
  def -> add_action( this, "Throw Glaive", "if=talent.bloodlet.enabled&"
    "spell_targets>=2+talent.chaos_cleave.enabled&(!talent.master_of_the_glaive.enabled|"
    "!talent.momentum.enabled|buff.momentum.up)&(spell_targets>=3|raid_event.adds.in>recharge_time+cooldown)" );
  def -> add_talent( this, "Fel Eruption" );
  def -> add_talent( this, "Felblade", "if=fury.deficit>=30+buff.prepared.up*8" );
  def -> add_action( this, spec.annihilation, "annihilation", "if=(talent.demon_blades.enabled|"
    "!talent.momentum.enabled|buff.momentum.up|fury.deficit<30+buff.prepared.up*8|"
    "buff.metamorphosis.remains<5)&!variable.pooling_for_blade_dance" );
  def -> add_action( this, "Throw Glaive", "if=talent.bloodlet.enabled&"
    "(!talent.master_of_the_glaive.enabled|!talent.momentum.enabled|buff.momentum.up)&raid_event.adds.in>recharge_time+cooldown" );
  def -> add_action( this, "Eye Beam", "if=!talent.demonic.enabled&"
    "((spell_targets.eye_beam_tick>desired_targets&active_enemies>1)|(raid_event.adds.in>45&"
    "!variable.pooling_for_meta&buff.metamorphosis.down&(artifact.anguish_of_the_deceiver.enabled|"
    "active_enemies>1)))" );
  def -> add_action( this, "Demon's Bite", "if=talent.demonic.enabled&buff.metamorphosis.down&"
    "cooldown.eye_beam.remains<gcd&fury.deficit>=20",
    "If Demonic is talented, pool fury as Eye Beam is coming off cooldown." );
  def -> add_action( this, "Demon's Bite", "if=talent.demonic.enabled&buff.metamorphosis.down&"
    "cooldown.eye_beam.remains<2*gcd&fury.deficit>=45" );
  def -> add_action( this, "Throw Glaive", "if=buff.metamorphosis.down&spell_targets>=2" );
  def -> add_action( this, "Chaos Strike", "if=(talent.demon_blades.enabled|"
    "!talent.momentum.enabled|buff.momentum.up|fury.deficit<30+buff.prepared.up*8)&"
    "!variable.pooling_for_meta&!variable.pooling_for_blade_dance&(!talent.demonic.enabled|"
    "!cooldown.eye_beam.ready)" );
  def -> add_talent( this, "Fel Barrage", "if=charges=4&buff.metamorphosis.down&(buff.momentum.up|"
    "!talent.momentum.enabled)&(active_enemies>desired_targets|raid_event.adds.in>30)",
    "Use Fel Barrage if its nearing max charges, saving it for Momentum and adds if possible." );
  def -> add_action( this, "Fel Rush", "animation_cancel=1,if=!talent.momentum.enabled&"
    "raid_event.movement.in>charges*10" );
  def -> add_action( this, "Demon's Bite" );
  def -> add_action( this, "Throw Glaive", "if=buff.out_of_range.up|buff.raid_movement.up" );
  def -> add_talent( this, "Felblade", "if=movement.distance|buff.out_of_range.up" );
  def -> add_action( this, "Fel Rush", "if=movement.distance>15|(buff.out_of_range.up&"
    "!talent.momentum.enabled)" );
  def -> add_action( this, "Vengeful Retreat", "if=movement.distance>15" );
  def -> add_action( this, "Throw Glaive", "if=talent.felblade.enabled" );

  action_priority_list_t* cd = get_action_priority_list( "cooldown" );

  add_havoc_use_items( this, cd );
  cd -> add_talent( this, "Nemesis", "target_if=min:target.time_to_die,if=raid_event.adds.exists&"
    "debuff.nemesis.down&(active_enemies>desired_targets|raid_event.adds.in>60)" );
  cd -> add_talent( this, "Nemesis", "if=!raid_event.adds.exists&"
    "(cooldown.metamorphosis.remains>100|target.time_to_die<70)" );
  cd -> add_talent( this, "Nemesis", "sync=metamorphosis,if=!raid_event.adds.exists" );
  cd -> add_talent( this, "Chaos Blades", "if=buff.metamorphosis.up|"
    "cooldown.metamorphosis.remains>100|target.time_to_die<20" );
  cd -> add_action( this, "Metamorphosis", "if=variable.pooling_for_meta&fury.deficit<30&"
    "(talent.chaos_blades.enabled|!cooldown.fury_of_the_illidari.ready)" );

  // Pre-Potion
  if ( sim -> allow_potions )
  {
    if ( true_level > 100 )
      cd -> add_action( "potion,name=old_war,if=buff.metamorphosis.remains>25|target.time_to_die<30" );
    else
      cd -> add_action( "potion,name=draenic_agility_potion,if=buff.metamorphosis.remains>25|target.time_to_die<30" );
  }
}

// demon_hunter_t::apl_vengeance ============================================

void demon_hunter_t::apl_vengeance()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  def -> add_action( "auto_attack" );
  def -> add_action( this, "Fiery Brand",
    "if=buff.demon_spikes.down&buff.metamorphosis.down" );
  def -> add_action( this, "Demon Spikes", "if=charges=2|buff.demon_spikes.d"
    "own&!dot.fiery_brand.ticking&buff.metamorphosis.down" );
  def -> add_action( this, "Empower Wards", "if=debuff.casting.up" );
  def -> add_action( this, "Infernal Strike", "if=!sigil_placed&!in_flight&rema"
    "ins-travel_time-delay<0.3*duration&artifact.fiery_demise.enabled&dot.fi"
    "ery_brand.ticking" );
  def -> add_action( this, "Infernal Strike", "if=!sigil_placed&!in_flight&rema"
    "ins-travel_time-delay<0.3*duration&(!artifact.fiery_demise.enabled|(max"
    "_charges-charges_fractional)*recharge_time<cooldown.fiery_brand.remains"
    "+5)&(cooldown.sigil_of_flame.remains>7|charges=2)" );
  def -> add_talent( this, "Spirit Bomb", "if=debuff.frailty.down" );
  def -> add_action( this, artifact.soul_carver, "soul_carver",
    "if=dot.fiery_brand.ticking" );
  def -> add_action( this, "Immolation Aura", "if=pain<=80" );
  def -> add_talent( this, "Felblade", "if=pain<=70" );
  def -> add_talent( this, "Soul Barrier" );
  def -> add_action( this, "Soul Cleave", "if=soul_fragments=5" );
  def -> add_action( this, "Metamorphosis", "if=buff.demon_spikes.down&!dot."
    "fiery_brand.ticking&buff.metamorphosis.down&incoming_damage_5s>health.m"
    "ax*0.70" );
  def -> add_talent( this, "Fel Devastation",
    "if=incoming_damage_5s>health.max*0.70" );
  def -> add_action( this, "Soul Cleave",
    "if=incoming_damage_5s>=health.max*0.70" );
  def -> add_talent( this, "Fel Eruption" );
  def -> add_action( this, "Sigil of Flame",
    "if=remains-delay<=0.3*duration" );
  def -> add_talent( this, "Fracture",
    "if=pain>=80&soul_fragments<4&incoming_damage_4s<=health.max*0.20" );
  def -> add_action( this, "Soul Cleave", "if=pain>=80" );
  def -> add_action( this, "Shear" );
}

// demon_hunter_t::create_cooldowns =========================================

void demon_hunter_t::create_cooldowns()
{
  // General
  cooldown.consume_magic = get_cooldown( "consume_magic" );
  cooldown.felblade      = get_cooldown( "felblade" );
  cooldown.fel_eruption  = get_cooldown( "fel_eruption" );

  // Havoc
  cooldown.blade_dance          = get_cooldown( "blade_dance" );
  cooldown.blur                 = get_cooldown( "blur" );
  cooldown.chaos_blades         = get_cooldown( "chaos_blades" );
  cooldown.chaos_nova           = get_cooldown( "chaos_nova" );
  cooldown.death_sweep          = get_cooldown( "death_sweep" );
  cooldown.demonic_appetite     = get_cooldown( "demonic_appetite" );
  cooldown.eye_beam             = get_cooldown( "eye_beam" );
  cooldown.fel_barrage          = get_cooldown( "fel_barrage" );
  cooldown.fel_barrage_proc     = get_cooldown( "fel_barrage_proc" );
  cooldown.fel_rush             = get_cooldown( "fel_rush" );
  cooldown.fel_rush_secondary   = get_cooldown( "fel_rush_secondary" );
  cooldown.fury_of_the_illidari = get_cooldown( "fury_of_the_illidari" );
  cooldown.nemesis              = get_cooldown( "nemesis" );
  cooldown.netherwalk           = get_cooldown( "netherwalk" );
  cooldown.throw_glaive         = get_cooldown( "throw_glaive" );
  cooldown.vengeful_retreat     = get_cooldown( "vengeful_retreat" );
  cooldown.vengeful_retreat_secondary = get_cooldown( "vengeful_retreat_secondary" );

  // Vengeance
  cooldown.demon_spikes     = get_cooldown( "demon_spikes" );
  cooldown.fiery_brand      = get_cooldown( "fiery_brand" );
  cooldown.sigil_of_chains  = get_cooldown( "sigil_of_chains" );
  cooldown.sigil_of_flame   = get_cooldown( "sigil_of_flame" );
  cooldown.sigil_of_misery  = get_cooldown( "sigil_of_misery" );
  cooldown.sigil_of_silence = get_cooldown( "sigil_of_silence" );

  sigil_cooldowns.push_back( cooldown.sigil_of_flame );
  sigil_cooldowns.push_back( cooldown.sigil_of_silence );
  sigil_cooldowns.push_back( cooldown.sigil_of_misery );
}

// demon_hunter_t::create_gains =============================================

void demon_hunter_t::create_gains()
{
  // General
  gain.miss_refund = get_gain( "miss_refund" );

  // Havoc
  gain.demonic_appetite = get_gain( "demonic_appetite" );
  gain.prepared         = get_gain( "prepared" );
  gain.blind_fury		= get_gain("blind_fury");

  // Vengeance
  gain.damage_taken    = get_gain( "damage_taken" );
  gain.immolation_aura = get_gain( "immolation_aura" );
  gain.metamorphosis   = get_gain( "metamorphosis" );
}

// demon_hunter_t::create_benefits ==========================================

void demon_hunter_t::create_benefits()
{
}

// ==========================================================================
// overridden player_t stat functions
// ==========================================================================

// demon_hunter_t::composite_armor_multiplier ===============================

double demon_hunter_t::composite_armor_multiplier() const
{
  double am = player_t::composite_armor_multiplier();

  am *= 1.0 + spec.demonic_wards -> effectN( 5 ).percent();

  return am;
}

// demon_hunter_t::composite_attack_power_multiplier ========================

double demon_hunter_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  ap *= 1.0 +
        cache.mastery() * mastery_spell.fel_blood -> effectN( 2 ).mastery_value();

  return ap;
}

// demon_hunter_t::composite_attribute_multiplier ===========================

double demon_hunter_t::composite_attribute_multiplier( attribute_e a ) const
{
  double am = player_t::composite_attribute_multiplier( a );

  switch ( a )
  {
    case ATTR_STAMINA:
      am *= 1.0 + spec.demonic_wards -> effectN( 4 ).percent();
      break;
    case ATTR_AGILITY:
    // Deliberately ignore stacks.
      am *= 1.0 + buff.siphon_power -> check_value();
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

  ca += spec.demonic_wards -> effectN( 2 ).percent();

  return ca;
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

// demon_hunter_t::composite_melee_haste  ===================================

double demon_hunter_t::composite_melee_haste() const
{
  double mh = player_t::composite_melee_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    mh /= 1.0 + buff.metamorphosis -> check_value();
  }

  return mh;
}

// demon_hunter_t::composite_spell_haste ====================================

double demon_hunter_t::composite_spell_haste() const
{
  double sh = player_t::composite_spell_haste();

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    sh /= 1.0 + buff.metamorphosis -> check_value();
  }

  return sh;
}

// demon_hunter_t::composite_leech ==========================================

double demon_hunter_t::composite_leech() const
{
  double l = player_t::composite_leech();

  if ( talent.soul_rending -> ok() && buff.metamorphosis -> check() )
  {
    l += talent.soul_rending -> effectN( 1 ).percent();
  }

  if ( buff.blur -> check() )
  {
    l += legendary.eternal_hunger;
  }

  if ( buff.demon_spikes -> check() )  // legendary effect
  {
    l += legendary.fragment_of_the_betrayers_prison;
  }

  return l;
}

// demon_hunter_t::composite_melee_crit_chance ==============================

double demon_hunter_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  mc += spec.critical_strikes -> effectN( 1 ).percent();

  return mc;
}

// demon_hunter_t::composite_melee_expertise ================================

double demon_hunter_t::composite_melee_expertise( const weapon_t* w ) const
{
  double me = player_t::composite_melee_expertise( w );

  me += spec.demonic_wards -> effectN( 3 ).base_value();

  return me;
}

// demon_hunter_t::composite_parry ==========================================

double demon_hunter_t::composite_parry() const
{
  double cp = player_t::composite_parry();

  cp += buff.demon_spikes -> check_value();

  cp += artifact.aldrachi_design.percent();

  cp += buff.defensive_spikes -> check_value();

  return cp;
}

// demon_hunter_t::composite_parry_rating() =================================

double demon_hunter_t::composite_parry_rating() const
{
  double pr = player_t::composite_parry_rating();

  if ( spec.riposte -> ok() )
    pr += composite_melee_crit_rating();

  return pr;
}

// demon_hunter_t::composite_player_multiplier ==============================

double demon_hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  m *= 1.0 + buff.demon_soul -> check_value();

  m *= 1.0 + buff.momentum -> check_value();

  // TODO: Figure out how to access target's race.
  m *=
    1.0 + buff.nemesis -> check() * buff.nemesis -> data().effectN( 1 ).percent();

  m *= 1.0 + buff.chaos_blades -> check_value();

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && buff.demon_spikes -> check() )
    m *= 1.0 + talent.razor_spikes -> effectN( 1 ).percent();

  return m;
}

// demon_hunter_t::composite_spell_crit_chance ==============================

double demon_hunter_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  sc += spec.critical_strikes -> effectN( 1 ).percent();

  return sc;
}

// demon_hunter_t::matching_gear_multiplier =================================

double demon_hunter_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( specialization() == DEMON_HUNTER_HAVOC && attr == ATTR_AGILITY ) ||
       ( specialization() == DEMON_HUNTER_VENGEANCE && attr == ATTR_STAMINA ) )
  {
    return spec.leather_specialization -> effectN( 1 ).percent();
  }

  return 0.0;
}

// demon_hunter_t::passive_movement_modifier ================================

double demon_hunter_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( mastery_spell.demonic_presence -> ok() )
    ms += cache.mastery() *
          mastery_spell.demonic_presence -> effectN( 2 ).mastery_value();

  return ms;
}

// demon_hunter_t::temporary_movement_modifier ==============================

double demon_hunter_t::temporary_movement_modifier() const
{
  double ms = player_t::temporary_movement_modifier();

  ms += std::max( ms, buff.immolation_aura -> value() );

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
  for (size_t i = 0; i < items.size(); i++)
  {
    const item_t* item = &items[i];
    if ( ! item || item -> slot == SLOT_SHIRT || item -> slot == SLOT_RANGED ||
      item -> slot == SLOT_TABARD || item -> item_level() <= 0 )
    {
      continue;
    }

    const random_prop_data_t item_data = dbc.random_property(item->item_level());
    int index = item_database::random_suffix_type(&item->parsed.data);
    slot_weights += item_data.p_epic[index] / item_data.p_epic[0];

    if (!item->active())
    {
      continue;
    }

    prop_values += item_data.p_epic[index];
  }

  double expected_health = (prop_values / slot_weights) * 8.318556;
  expected_health += base.stats.attribute[STAT_STAMINA];
  expected_health *= 1 + matching_gear_multiplier(ATTR_STAMINA);
  expected_health *= 1 + spec.demonic_wards->effectN(4).percent();
  expected_health *= 1 + artifact.will_of_the_illidari.percent();
  expected_health *= current.health_per_stamina;
  return expected_health;
}

// demon_hunter_t::assess_damage ============================================

void demon_hunter_t::assess_damage( school_e school, dmg_e dt,
                                    action_state_t* s )
{
  player_t::assess_damage( school, dt, s );

  if ( s -> result == RESULT_PARRY )
  {
    buff.blade_turning -> trigger();
  }

  // Benefit tracking
  if ( s -> action -> may_parry )
  {
    buff.demon_spikes -> up();
    buff.defensive_spikes -> up();
  }
}

// demon_hunter_t::damage_imminent_pre_absorb ===============================

void demon_hunter_t::assess_damage_imminent_pre_absorb( school_e school,
                                                        dmg_e rt,
                                                        action_state_t* s )
{
  player_t::assess_damage_imminent_pre_absorb( school, rt, s );

  if ( s -> result_amount <= 0 )
  {
    return;
  }

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    resource_gain( RESOURCE_PAIN,
      s -> result_amount / expected_max_health * 50.0,
      gain.damage_taken );
  }

  if ( artifact.siphon_power.rank() && buff.empower_wards -> check() &&
    dbc::get_school_mask( s -> action -> school ) & SCHOOL_MAGIC_MASK )
  {
    double cv = 0;

    if ( buff.siphon_power -> check() )
    {
      cv = buff.siphon_power -> check_value();
    }

    assert( buff.siphon_power -> check() || buff.siphon_power -> current_stack == 0 );

    cv += s -> result_amount / resources.max[ RESOURCE_HEALTH ];

    cv = std::min( cv, artifact.siphon_power.percent() );

    // Trigger with stacks just for reporting convenience;
    // the stack count doesn't actually do anything.
    unsigned new_stack =
      std::max( static_cast<unsigned>( cv * 100.0 ), ( unsigned ) 1 );

    buff.siphon_power -> trigger(
      new_stack - buff.siphon_power -> current_stack, cv );
  }
}

// demon_hunter_t::combat_begin =============================================

void demon_hunter_t::combat_begin()
{
  player_t::combat_begin();

  // Start event drivers
  if ( talent.spirit_bomb -> ok() )
    spirit_bomb_driver = make_event<spirit_bomb_event_t>( *sim, this, true );
}

// demon_hunter_t::interrupt ================================================

void demon_hunter_t::interrupt()
{
  event_t::cancel( blade_dance_driver );
  event_t::cancel( soul_fragment_pick_up );

  base_t::interrupt();
}

// demon_hunter_t::recalculate_resource_max =================================

void demon_hunter_t::recalculate_resource_max( resource_e r )
{
  player_t::recalculate_resource_max( r );

  if ( r == RESOURCE_HEALTH )
  {
    resources.max[ r ] *= 1.0 + artifact.will_of_the_illidari.percent();

    // Update Metamorphosis' value for the new health amount.
    if ( buff.metamorphosis -> check() )
    {
      assert( metamorphosis_health > 0 );

      double base_health = max_health() - metamorphosis_health;
      double new_health  = base_health * buff.metamorphosis -> check_value();
      double diff        = new_health - metamorphosis_health;

      if ( diff != 0.0 )
      {
        if ( sim -> debug )
        {
          sim -> out_debug.printf(
            "%s adjusts %s temporary health. old=%.0f new=%.0f diff=%.0f",
            name(), buff.metamorphosis -> name(), metamorphosis_health,
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

// demon_hunter_t::reset ====================================================

void demon_hunter_t::reset()
{
  base_t::reset();

  blade_dance_driver    = nullptr;
  soul_fragment_pick_up = nullptr;
  spirit_bomb_driver    = nullptr;
  exiting_melee         = nullptr;
  next_fragment_spawn   = 0;
  shear_counter         = 0;
  metamorphosis_health  = 0;
  spirit_bomb           = 0.0;
  sigil_of_flame_activates = timespan_t::zero();

  for ( size_t i = 0; i < soul_fragments.size(); i++ )
  {
    delete soul_fragments[ i ];
  }

  soul_fragments.clear();

  invalidate_damage_calcs();
}

// demon_hunter_t::target_mitigation ========================================

void demon_hunter_t::target_mitigation( school_e school, dmg_e dt,
                                        action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );

  if ( s -> result_amount <= 0 )
  {
    return;
  }

  s -> result_amount *= 1.0 + buff.blur -> value();

  s -> result_amount *= 1.0 + buff.painbringer -> stack_value();

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && buff.demon_spikes -> up() )
  {
    s -> result_amount *= 1.0 + buff.demon_spikes -> data().effectN( 2 ).percent() -
                        cache.mastery_value();
  }

  if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
  {
    // TOCHECK: Tooltip says magical damage but spell data says all damage.
    s -> result_amount *= 1.0 + spec.demonic_wards -> effectN( 1 ).percent();

    s -> result_amount *= 1.0 + buff.empower_wards -> value();

    if ( buff.immolation_aura -> check() )
    {
      s -> result_amount *= 1.0 - legendary.cloak_of_fel_flames;
    }
  }

  if ( get_target_data( s -> action -> player ) -> dots.fiery_brand -> is_ticking() )
  {
    s -> result_amount *= 1.0 + spec.fiery_brand_dr -> effectN( 2 ).percent();
  }
}

// ==========================================================================
// custom demon_hunter_t functions
// ==========================================================================

// demon_hunter_t::adjust_movement ==========================================

void demon_hunter_t::adjust_movement()
{
  if ( buff.out_of_range -> check() &&
       buff.out_of_range -> remains() > timespan_t::zero() )
  {
    // Recalculate movement duration.
    assert( buff.out_of_range -> value() > 0 );
    assert( buff.out_of_range -> expiration.size() );

    timespan_t remains = buff.out_of_range -> remains();
    remains *= buff.out_of_range -> check_value() / cache.run_speed();

    /* Adjust the remaining duration on movement for the new run speed.
    Expire and re-trigger because we can't reschedule backwards. */
    buff.out_of_range -> expire();

    // Catch edge case where new duration rounds to zero and makes buff
    // infinite.
    if ( remains > timespan_t::zero() )
    {
      buff.out_of_range -> trigger( 1, cache.run_speed(), -1.0, remains );
    }
  }
}

// demon_hunter_t::consume_soul_fragments ===================================

unsigned demon_hunter_t::consume_soul_fragments( soul_fragment_e type,
    bool heal )
{
  unsigned a = get_active_soul_fragments( type );

  if ( a == 0 )
    return 0;

  unsigned consumed = 0;

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s consumes %u %ss. active=%u total=%u", name(), a,
                           get_soul_fragment_str( type ), 0,
                           get_total_soul_fragments( type ) - a );
  }

  for ( size_t i = 0; i < soul_fragments.size(); i++ )
  {
    if ( soul_fragments[ i ] -> is_type( type ) && soul_fragments[ i ] -> active() )
    {
      soul_fragments[ i ] -> consume( heal );
      consumed++;
    }
  }

  return consumed;
}

// demon_hunter_t::get_active_soul_fragments ================================

unsigned demon_hunter_t::get_active_soul_fragments( soul_fragment_e type ) const
{
  switch ( type )
  {
    case SOUL_FRAGMENT_GREATER:
    case SOUL_FRAGMENT_LESSER:
      return std::accumulate(
               soul_fragments.begin(), soul_fragments.end(), 0,
      [&type]( unsigned acc, soul_fragment_t* frag ) {
        return acc + ( frag -> is_type( type ) && frag -> active() );
      } );
    case SOUL_FRAGMENT_ALL:
    default:
      return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
      []( unsigned acc, soul_fragment_t* frag ) {
        return acc + frag -> active();
      } );
  }
}

// demon_hunter_t::get_total_soul_fragments =================================

unsigned demon_hunter_t::get_total_soul_fragments( soul_fragment_e type ) const
{
  switch ( type )
  {
    case SOUL_FRAGMENT_GREATER:
    case SOUL_FRAGMENT_LESSER:
      return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
      [&type]( unsigned acc, soul_fragment_t* frag ) {
        return acc + frag -> is_type( type );
      } );
    case SOUL_FRAGMENT_ALL:
    default:
      return ( unsigned )soul_fragments.size();
  }
}

// demon_hunter_t::spawn_soul_fragment ======================================

void demon_hunter_t::spawn_soul_fragment( soul_fragment_e type, unsigned n )
{
  proc_t* soul_proc = type == SOUL_FRAGMENT_GREATER ? proc.soul_fragment
                      : proc.soul_fragment_lesser;

  for ( unsigned i = 0; i < n; i++ )
  {
    if ( get_total_soul_fragments( type ) == MAX_SOUL_FRAGMENTS )
    {
      // Find and delete the oldest fragment of this type.
      std::vector<soul_fragment_t*>::iterator it;
      for ( it = soul_fragments.begin(); it != soul_fragments.end(); it++ )
      {
        soul_fragment_t& frag = **it;

        if ( frag.is_type( type ) )
        {
          frag.remove();
          proc.soul_fragment_overflow -> occur();
          break;
        }
      }

      assert( get_total_soul_fragments( type ) < MAX_SOUL_FRAGMENTS );
    }

    soul_fragments.push_back( new soul_fragment_t( this, type ) );
    soul_proc -> occur();
  }

  if ( sim -> debug )
  {
    sim -> out_debug.printf( "%s creates %u %ss. active=%u total=%u", name(), n,
                           get_soul_fragment_str( type ),
                           get_active_soul_fragments( type ),
                           get_total_soul_fragments( type ) );
  }
}

// demon_hunter_t::invalidate_damage_calcs ==================================

void demon_hunter_t::invalidate_damage_calcs()
{
  for ( size_t i = 0; i < damage_calcs.size(); i++ )
  {
    damage_calcs[ i ] -> invalidate();
  }
}

// demon_hunter_t::create_sigil_expression ==================================

expr_t* demon_hunter_t::create_sigil_expression( const std::string& name )
{
  if ( util::str_compare_ci( name, "activation_time" ) ||
    util::str_compare_ci( name, "delay" ) )
  {
    if ( sim -> optimize_expressions )
    {
      return expr_t::create_constant( name, sigil_delay.total_seconds() );
    }
    else
    {
      return make_ref_expr( name, sigil_delay );
    }
  }
  else if ( util::str_compare_ci( name, "sigil_placed" ) ||
    util::str_compare_ci( name, "placed" ) )
  {
    struct sigil_placed_expr_t : public expr_t
    {
      demon_hunter_t* dh;

      sigil_placed_expr_t( demon_hunter_t* p, const std::string& n )
        : expr_t( n ), dh( p )
      {}

      double evaluate() override
      {
        if ( dh -> sim -> current_time() < dh -> sigil_of_flame_activates )
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

  void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    ( void )p;
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

using namespace unique_gear;
using namespace actions::spells;
using namespace actions::attacks;

// Generic legendary items

template <typename T>
struct anger_of_the_halfgiants_t : public scoped_action_callback_t<T>
{
  anger_of_the_halfgiants_t( const std::string& name )
    : scoped_action_callback_t<T>( DEMON_HUNTER, name )
  {
  }

  void manipulate( T* action, const special_effect_t& e ) override
  {
    action -> energize_die_sides += e.driver() -> effectN( 1 ).base_value();

    // Demon Blades modifies the effect of the legendary.
    demon_hunter_t* p = debug_cast<demon_hunter_t*>( action -> player );
    assert( ! p -> talent.demon_blades -> ok() || e.driver() -> affected_by( p -> talent.demon_blades ) );
    action -> energize_die_sides += p -> talent.demon_blades -> effectN( 2 ).base_value();
  }
};

struct moarg_bionic_stabilizers_t
  : public scoped_action_callback_t<throw_glaive_t>
{
  moarg_bionic_stabilizers_t() : super( DEMON_HUNTER, "throw_glaive" )
  {
  }

  void manipulate( throw_glaive_t* action, const special_effect_t& e ) override
  {
    action -> chain_bonus_damage += e.driver() -> effectN( 1 ).percent();
  }  // TOCHECK
};

// Vengeance-specific legendary items

struct cloak_of_fel_flames_t : public scoped_actor_callback_t<demon_hunter_t>
{
  cloak_of_fel_flames_t() : super( DEMON_HUNTER_VENGEANCE )
  {
  }

  void manipulate( demon_hunter_t* actor, const special_effect_t& e ) override
  {
    actor -> legendary.cloak_of_fel_flames = e.driver() -> effectN( 1 ).percent();
  }
};

struct fragment_of_the_betrayers_prison_t
  : public scoped_actor_callback_t<demon_hunter_t>
{
  fragment_of_the_betrayers_prison_t() : super( DEMON_HUNTER_VENGEANCE )
  {
  }

  void manipulate( demon_hunter_t* actor,
                   const special_effect_t& /* e */ ) override
  {
    actor -> legendary.fragment_of_the_betrayers_prison =
      actor -> find_spell( 217500 ) -> effectN( 1 ).percent();
  }
};

struct the_defilers_lost_vambraces_t
  : public scoped_actor_callback_t<demon_hunter_t>
{
  the_defilers_lost_vambraces_t() : super( DEMON_HUNTER_VENGEANCE )
  {
  }

  void manipulate( demon_hunter_t* actor, const special_effect_t& e ) override
  {
    actor -> legendary.the_defilers_lost_vambraces =
      timespan_t::from_seconds( -e.driver() -> effectN( 1 ).base_value() );
  }
};

struct kirel_narak_t : public scoped_actor_callback_t<demon_hunter_t>
{
  kirel_narak_t() : super( DEMON_HUNTER_VENGEANCE )
  {
  }

  void manipulate( demon_hunter_t* actor, const special_effect_t& e ) override
  {
    actor -> legendary.kirel_narak =
      timespan_t::from_seconds( -e.driver() -> effectN( 1 ).base_value() );
  }
};

struct runemasters_pauldrons_t : public scoped_actor_callback_t<demon_hunter_t>
{
  runemasters_pauldrons_t() : super( DEMON_HUNTER_VENGEANCE )
  {
  }

  void manipulate( demon_hunter_t* actor,
                   const special_effect_t& /* e */ ) override
  {
    actor -> legendary.runemasters_pauldrons = true;
  }
};

// Havoc-specific legendary items

struct eternal_hunger_t : public scoped_actor_callback_t<demon_hunter_t>
{
  eternal_hunger_t() : super( DEMON_HUNTER_HAVOC )
  {
  }

  void manipulate( demon_hunter_t* actor, const special_effect_t& e ) override
  {
    actor -> legendary.eternal_hunger = e.driver() -> effectN( 1 ).percent();
  }
};

struct raddons_cascading_eyes_t : public scoped_actor_callback_t<demon_hunter_t>
{
  raddons_cascading_eyes_t() : super( DEMON_HUNTER_HAVOC )
  {
  }

  void manipulate( demon_hunter_t* actor, const special_effect_t& e ) override
  {
    actor -> legendary.raddons_cascading_eyes =
      -e.driver() -> effectN( 1 ).time_value();
  }
};

struct loramus_thalipedes_sacrifice_t
  : public scoped_action_callback_t<fel_rush_t::fel_rush_damage_t>
{
  loramus_thalipedes_sacrifice_t() : super( DEMON_HUNTER_HAVOC, "fel_rush_dmg" )
  {
  }

  void manipulate( fel_rush_t::fel_rush_damage_t* action,
                   const special_effect_t& e ) override
  {
    action -> chain_bonus_damage += e.driver() -> effectN( 1 ).percent();
  }
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
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new demon_hunter_report_t( *p ) );
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
    register_special_effect(
      208827, anger_of_the_halfgiants_t<demon_blades_t>( "demon_blades" ) );
    register_special_effect(
      208827, anger_of_the_halfgiants_t<demons_bite_t>( "demons_bite" ) );
    register_special_effect( 217735, cloak_of_fel_flames_t() );
    register_special_effect( 208985, eternal_hunger_t() );
    register_special_effect( 217496, fragment_of_the_betrayers_prison_t() );
    register_special_effect( 209002, loramus_thalipedes_sacrifice_t() );
    register_special_effect( 210970, kirel_narak_t() );
    register_special_effect( 208826, moarg_bionic_stabilizers_t() );
    register_special_effect( 215149, raddons_cascading_eyes_t() );
    register_special_effect( 210867, runemasters_pauldrons_t() );
    register_special_effect( 210840, the_defilers_lost_vambraces_t() );
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
