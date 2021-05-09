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
// Shadowlands To-Do
// ==========================================================================

  Existing Issues
  ---------------

  * Add option for Greater Soul spawns on add demise (% chance?) simulating adds in M+/dungeon style situations

  New Issues
  ----------

  * Implement Vengeance Legendaries

  Vengeance:
  * Add AoE component to Sinful Brand Meta
  * Add Revel in Pain passive
  ** Ruinous Bulwark Absorb

  Maybe:
  ** Darkness (?)
  ** Sigil of Silence (?)
  ** Sigil of Misery (?)
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

    // Covenant
    dot_t* sinful_brand;

    // Legendary
    dot_t* burning_wound;
  } dots;

  struct debuffs_t
  {
    // Havoc
    buff_t* essence_break;

    // Vengeance
    buff_t* frailty;
    buff_t* void_reaver;

    // Covenant
    buff_t* serrated_glaive;

    // Legendary
    buff_t* burning_wound;
    buff_t* fel_bombardment;
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
constexpr double VENGEFUL_RETREAT_DISTANCE = 20.0;

enum class soul_fragment : unsigned
{
  GREATER = 0x01,
  LESSER = 0x02,
  GREATER_DEMON = 0x04,
  EMPOWERED_DEMON = 0x08,

  ANY_GREATER = ( GREATER | GREATER_DEMON | EMPOWERED_DEMON ),
  ANY_DEMON = ( GREATER_DEMON | EMPOWERED_DEMON ),
  ANY = 0xFF
};

soul_fragment operator&( soul_fragment l, soul_fragment r )
{
  return static_cast<soul_fragment> ( static_cast<unsigned>( l ) & static_cast<unsigned>( r ) );
}

soul_fragment operator|( soul_fragment l, soul_fragment r )
{
  return static_cast<soul_fragment> ( static_cast<unsigned>( l ) | static_cast<unsigned>( r ) );
}

const char* get_soul_fragment_str( soul_fragment type )
{
  switch ( type )
  {
    case soul_fragment::ANY:
      return "soul fragment";
    case soul_fragment::GREATER:
      return "greater soul fragment";
    case soul_fragment::LESSER:
      return "lesser soul fragment";
    case soul_fragment::GREATER_DEMON:
      return "greater demon fragment";
    case soul_fragment::EMPOWERED_DEMON:
      return "empowered demon fragment";
    default:
      return "";
  }
}

struct movement_buff_t : public buff_t
{
  double yards_from_melee;
  double distance_moved;
  demon_hunter_t* dh;

  movement_buff_t( demon_hunter_t* p, const std::string& name, const spell_data_t* spell_data = spell_data_t::nil(), const item_t* item = nullptr );

  bool trigger( int s = 1, double v = DEFAULT_VALUE(), double c = -1.0, timespan_t d = timespan_t::min() ) override;
};

typedef std::pair<std::string, simple_sample_data_with_min_max_t> data_t;
typedef std::pair<std::string, simple_sample_data_t> simple_data_t;

/* Demon Hunter class definition
 *
 * Derived from player_t. Contains everything that defines the Demon Hunter
 * class.
 */
class demon_hunter_t : public player_t
{
public:
  using base_t = player_t;

  // Data collection for cooldown waste
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

  event_t* exit_melee_event;  // Event to disable melee abilities mid-VR.

  // Buffs
  struct buffs_t
  {
    // General
    damage_buff_t* demon_soul;
    damage_buff_t* empowered_demon_soul;
    buff_t* immolation_aura;
    buff_t* metamorphosis;

    // Havoc
    buff_t* blind_fury;
    buff_t* blur;
    buff_t* death_sweep;
    buff_t* eye_beam_rank_3;
    buff_t* momentum;
    buff_t* out_of_range;
    buff_t* prepared;
    buff_t* unbound_chaos;

    movement_buff_t* fel_rush_move;
    movement_buff_t* vengeful_retreat_move;

    // Vengeance
    buff_t* demon_spikes;
    absorb_buff_t* soul_barrier;

    // Covenant
    buff_t* growing_inferno;
    buff_t* soul_furance;

    // Legendary
    buff_t* chaos_theory;
    buff_t* fel_bombardment;
  } buff;

  // Talents
  struct talents_t
  {
    // Havoc
    const spell_data_t* blind_fury;
    const spell_data_t* demonic_appetite;
    const spell_data_t* felblade;

    const spell_data_t* insatiable_hunger;
    const spell_data_t* burning_hatred;
    const spell_data_t* demon_blades;

    const spell_data_t* trail_of_ruin;
    const spell_data_t* unbound_chaos;
    const spell_data_t* glaive_tempest;

    const spell_data_t* soul_rending;
    const spell_data_t* desperate_instincts;
    const spell_data_t* netherwalk;

    const spell_data_t* cycle_of_hatred;
    const spell_data_t* first_blood;
    const spell_data_t* essence_break;

    const spell_data_t* unleashed_power;
    const spell_data_t* master_of_the_glaive;
    const spell_data_t* fel_eruption;

    const spell_data_t* demonic;
    const spell_data_t* momentum;
    const spell_data_t* fel_barrage;

    // Vengeance
    const spell_data_t* abyssal_strike;
    const spell_data_t* agonizing_flames;
    //                  felblade

    const spell_data_t* feast_of_souls;
    const spell_data_t* fallout;
    const spell_data_t* burning_alive;

    const spell_data_t* infernal_armor;
    const spell_data_t* charred_flesh;
    const spell_data_t* spirit_bomb;

    //                  soul_rending
    const spell_data_t* feed_the_demon;
    const spell_data_t* fracture;

    const spell_data_t* concentrated_sigils;
    const spell_data_t* quickened_sigils;
    const spell_data_t* sigil_of_chains;

    const spell_data_t* void_reaver;
    //                  demonic
    const spell_data_t* soul_barrier;

    const spell_data_t* last_resort;        // NYI
    const spell_data_t* ruinous_bulwark;
    const spell_data_t* bulk_extraction;

    // PvP Talents
    const spell_data_t* demonic_origins;
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
    const spell_data_t* demonic_wards_rank_2;
    const spell_data_t* demonic_wards_rank_3;
    const spell_data_t* demon_soul;
    const spell_data_t* disrupt;
    const spell_data_t* disrupt_rank_2;
    const spell_data_t* disrupt_rank_3;
    const spell_data_t* empowered_demon_soul;
    const spell_data_t* leather_specialization;
    const spell_data_t* immolation_aura;
    const spell_data_t* immolation_aura_rank_2;
    const spell_data_t* immolation_aura_rank_3;
    const spell_data_t* metamorphosis;
    const spell_data_t* metamorphosis_rank_2;
    const spell_data_t* metamorphosis_rank_3;
    const spell_data_t* metamorphosis_rank_4;
    const spell_data_t* metamorphosis_buff;
    const spell_data_t* soul_fragment;
    const spell_data_t* throw_glaive;
    const spell_data_t* throw_glaive_rank_2;

    // Havoc
    const spell_data_t* havoc;
    const spell_data_t* annihilation;
    const spell_data_t* blade_dance;
    const spell_data_t* blade_dance_rank_2;
    const spell_data_t* blur;
    const spell_data_t* chaos_nova;
    const spell_data_t* chaos_nova_rank_2;
    const spell_data_t* chaos_strike;
    const spell_data_t* chaos_strike_rank_2;
    const spell_data_t* chaos_strike_rank_3;
    const spell_data_t* chaos_strike_refund;
    const spell_data_t* chaos_strike_fury;
    const spell_data_t* death_sweep;
    const spell_data_t* demonic_appetite_fury;
    const spell_data_t* demons_bite;
    const spell_data_t* eye_beam;
    const spell_data_t* eye_beam_rank_2;
    const spell_data_t* eye_beam_rank_3;
    const spell_data_t* fel_rush;
    const spell_data_t* fel_rush_rank_2;
    const spell_data_t* fel_rush_rank_3;
    const spell_data_t* fel_rush_damage;
    const spell_data_t* unrestrained_fury;
    const spell_data_t* unrestrained_fury_2;
    const spell_data_t* vengeful_retreat;
    const spell_data_t* momentum_buff;

    // Vengeance
    const spell_data_t* vengeance;
    const spell_data_t* demon_spikes;
    const spell_data_t* demon_spikes_rank_2;
    const spell_data_t* fel_devastation;
    const spell_data_t* fel_devastation_rank_2;
    const spell_data_t* fiery_brand;
    const spell_data_t* fiery_brand_rank_2;
    const spell_data_t* fiery_brand_rank_3;
    const spell_data_t* fiery_brand_dr;
    const spell_data_t* infernal_strike;
    const spell_data_t* infernal_strike_rank_2;
    const spell_data_t* infernal_strike_rank_3;
    const spell_data_t* riposte;
    const spell_data_t* sigil_of_flame;
    const spell_data_t* sigil_of_flame_rank_2;
    const spell_data_t* soul_cleave;
    const spell_data_t* soul_cleave_rank_2;
    const spell_data_t* soul_cleave_rank_3;
    const spell_data_t* thick_skin;
    const spell_data_t* throw_glaive_rank_3;

    // Legendary
    const spell_data_t* darkglare_boon_refund;
  } spec;

  struct covenant_t
  {
    const spell_data_t* elysian_decree;
    const spell_data_t* fodder_to_the_flame;
    const spell_data_t* sinful_brand;
    const spell_data_t* the_hunt;
  } covenant;

  // Conduits
  struct conduit_t
  {
    conduit_data_t dancing_with_fate;
    conduit_data_t growing_inferno;
    conduit_data_t relentless_onslaught;
    conduit_data_t serrated_glaive;

    conduit_data_t soul_furnace;
    conduit_data_t fel_defender;

    conduit_data_t repeat_decree;
    conduit_data_t increased_scrutiny;
    conduit_data_t brooding_pool;
    conduit_data_t unnatural_malice;
  } conduit;

  struct legendary_t
  {
    // General
    item_runeforge_t darkglare_boon;
    item_runeforge_t collective_anguish;
    item_runeforge_t fel_bombardment;
    item_runeforge_t darkest_hour;                  // NYI

    // Havoc
    item_runeforge_t chaos_theory;
    item_runeforge_t erratic_fel_core;
    item_runeforge_t darker_nature;                 // NYI
    item_runeforge_t burning_wound;

    // Vengeance
    item_runeforge_t fiery_soul;
    item_runeforge_t razelikhs_defilement;
    item_runeforge_t fel_flame_fortification;       // NYI
    item_runeforge_t spirit_of_the_darkness_flame;  // NYI
  } legendary;

  // Mastery Spells
  struct mastery_t
  {
    // Havoc
    const spell_data_t* demonic_presence;
    const spell_data_t* demonic_presence_rank_2;
    // Vengeance
    const spell_data_t* fel_blood;
    const spell_data_t* fel_blood_rank_2;
  } mastery;

  // Cooldowns
  struct cooldowns_t
  {
    // General
    cooldown_t* consume_magic;
    cooldown_t* disrupt;
    cooldown_t* elysian_decree;
    cooldown_t* felblade;
    cooldown_t* fel_eruption;

    // Havoc
    cooldown_t* blade_dance;
    cooldown_t* blur;
    cooldown_t* chaos_nova;
    cooldown_t* essence_break;
    cooldown_t* demonic_appetite;
    cooldown_t* eye_beam;
    cooldown_t* fel_barrage;
    cooldown_t* fel_rush;
    cooldown_t* metamorphosis;
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

    // Legendary
    gain_t* darkglare_boon_refund;
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
    proc_t* soul_fragment_greater_demon;
    proc_t* soul_fragment_empowered_demon;
    proc_t* soul_fragment_lesser;

    // Havoc
    proc_t* demon_blades_wasted;
    proc_t* demonic_appetite;
    proc_t* demons_bite_in_meta;
    proc_t* chaos_strike_in_essence_break;
    proc_t* annihilation_in_essence_break;
    proc_t* blade_dance_in_essence_break;
    proc_t* death_sweep_in_essence_break;
    proc_t* felblade_reset;

    // Vengeance
    proc_t* soul_fragment_expire;
    proc_t* soul_fragment_overflow;
    proc_t* soul_fragment_from_shear;
    proc_t* soul_fragment_from_fracture;
    proc_t* soul_fragment_from_fallout;
    proc_t* soul_fragment_from_meta;

    // Legendary
    proc_t* darkglare_boon_resets;
  } proc;

  // RPPM objects
  struct rppms_t
  {
    real_ppm_t* felblade;

    // Havoc
    real_ppm_t* demonic_appetite;
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
    spell_t* immolation_aura = nullptr;
    spell_t* collective_anguish = nullptr;

    // Havoc
    attack_t* demon_blades = nullptr;
    attack_t* relentless_onslaught = nullptr;
    attack_t* relentless_onslaught_annihilation = nullptr;

    // Vengeance
    spell_t* infernal_armor = nullptr;
    heal_t* spirit_bomb_heal = nullptr;
  } active;

  // Pets
  struct pets_t
  {
  } pets;

  // Options
  struct demon_hunter_options_t
  {
    // Override for target's hitbox size, relevant for Fel Rush and Vengeful Retreat.
    double target_reach = -1.0;
    double initial_fury = 0;
    double memory_of_lucid_dreams_proc_chance = 0.15;
    int fodder_to_the_flame_kill_seconds = 4;
    double razelikhs_defilement_utility_pct = 0;
  } options;

  demon_hunter_t( sim_t* sim, util::string_view name, race_e r );

  // overridden player_t init functions
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void copy_from( player_t* source ) override;
  action_t* create_action( util::string_view name, const std::string& options ) override;
  void create_buffs() override;
  std::unique_ptr<expr_t> create_expression( util::string_view ) override;
  void create_options() override;
  pet_t* create_pet( util::string_view name, util::string_view type ) override;
  std::string create_profile( save_e ) override;
  void init_absorb_priority() override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_procs() override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
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
  std::string default_temporary_enchant() const override;

  // overridden player_t stat functions
  double composite_armor() const override;
  double composite_base_armor_multiplier() const override;
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
  void assess_damage( school_e, result_amount_type, action_state_t* s ) override;
  void combat_begin() override;
  const demon_hunter_td_t* find_target_data( const player_t* target ) const override;
  demon_hunter_td_t* get_target_data( player_t* target ) const override;
  void interrupt() override;
  void regen( timespan_t periodicity ) override;
  double resource_gain( resource_e, double, gain_t* source = nullptr, action_t* action = nullptr ) override;
  void recalculate_resource_max( resource_e, gain_t* source = nullptr ) override;
  void reset() override;
  void merge( player_t& other ) override;
  void datacollection_begin() override;
  void datacollection_end() override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void apply_affecting_auras( action_t& action ) override;

  // custom demon_hunter_t functions
  void set_out_of_range( timespan_t duration );
  void adjust_movement();
  double calculate_expected_max_health() const;
  unsigned consume_soul_fragments( soul_fragment = soul_fragment::ANY, bool heal = true, unsigned max = MAX_SOUL_FRAGMENTS );
  unsigned get_active_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  unsigned get_total_soul_fragments( soul_fragment = soul_fragment::ANY ) const;
  void activate_soul_fragment( soul_fragment_t* );
  void spawn_soul_fragment( soul_fragment, unsigned = 1, bool = false );
  void spawn_soul_fragment( soul_fragment, unsigned, player_t* target, bool = false );
  void trigger_demonic();
  double get_target_reach() const
  {
    return options.target_reach >= 0 ? options.target_reach : sim->target->combat_reach;
  }

  // Secondary Action Tracking
  std::vector<action_t*> background_actions;

  template <typename T, typename... Ts>
  T* get_background_action( util::string_view n, Ts&&... args )
  {
    auto it = range::find( background_actions, n, &action_t::name_str );
    if ( it != background_actions.cend() )
    {
      return dynamic_cast<T*>( *it );
    }

    auto action = new T( n, this, std::forward<Ts>( args )... );
    action->background = true;
    background_actions.push_back( action );
    return action;
  }

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

private:
  target_specific_t<demon_hunter_td_t> _target_data;
};


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
  assert( buff_duration() > timespan_t::zero() );

  // Check if we're already moving away from the target, if so we will now be moving towards it
  if ( dh->current.distance_to_move || dh->buff.out_of_range->check() || dh->buff.vengeful_retreat_move->check() )
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
    const timespan_t delay = buff_duration() * (1.0 - (yards_from_melee / distance_moved));

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
        sim().out_debug.printf( "%s %s expires. active=%u total=%u", frag->dh->name(),
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
      double velocity = frag->dh->spec.consume_soul_greater->missile_speed();
      if ( velocity == 0 || frag->consume_on_activation )
        return timespan_t::zero();
      double distance = frag->get_distance( frag->dh );
      return timespan_t::from_seconds( distance / velocity );
    }

    void execute() override
    {
      if (sim().debug)
      {
        sim().out_debug.printf( "%s %s becomes active. active=%u total=%u", frag->dh->name(),
                                get_soul_fragment_str( frag->type ),
                                frag->dh->get_active_soul_fragments( frag->type ) + 1,
                                frag->dh->get_total_soul_fragments( frag->type ) );
      }

      frag->activate = nullptr;
      frag->expiration = make_event<fragment_expiration_t>( sim(), frag );
      frag->dh->activate_soul_fragment( frag );
    }
  };

  demon_hunter_t* dh;
  double x, y;
  event_t* activate;
  event_t* expiration;
  const soul_fragment type;
  bool consume_on_activation;

  soul_fragment_t( demon_hunter_t* p, soul_fragment t, bool consume_on_activation )
    : dh( p ), type( t ), consume_on_activation( consume_on_activation )
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

  bool is_type( soul_fragment type_mask ) const
  {
    return ( type_mask & type ) == type;
  }

  void set_position()
  {
    // Set base position: 15 yards to the front right or front left.
    x = dh->x_position + ( dh->next_fragment_spawn % 2 ? -10.6066 : 10.6066 );
    y = dh->y_position + 10.6066;

    // Calculate random offset, 2-5 yards from the base position.
    double r_min = 2.0;
    double r_max = 5.0;
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
      action_t* consume_action = is_type( soul_fragment::ANY_GREATER ) ?
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

    if ( dh->conduit.soul_furnace.ok() )
    {
      dh->buff.soul_furance->trigger();
    }

    if ( is_type( soul_fragment::EMPOWERED_DEMON ) )
    {
      dh->buff.empowered_demon_soul->trigger();
    }
    else if ( is_type( soul_fragment::GREATER_DEMON ) )
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
    int i = as<int>( range::size( pet_base_stats ) );
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
  double energize_delta;

  // Cooldown tracking
  bool track_cd_waste;
  simple_sample_data_with_min_max_t *cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;

  // Affect flags for various dynamic effects
  struct affect_flags
  {
    bool direct = false;
    bool periodic = false;
  };

  struct
  {
    affect_flags demon_soul;
    affect_flags empowered_demon_soul;

    // Havoc
    affect_flags demonic_presence;
    affect_flags momentum;
    bool essence_break = false;
    bool burning_wound = false;
  } affected_by;

  void parse_affect_flags( const spell_data_t* spell, affect_flags& flags )
  {
    for ( const spelleffect_data_t& effect : spell->effects() )
    {
      if ( !effect.ok() || effect.type() != E_APPLY_AURA || effect.subtype() != A_ADD_PCT_MODIFIER )
        continue;

      if ( ab::data().affected_by( effect ) )
      {
        switch ( effect.misc_value1() )
        {
          case P_GENERIC:
            flags.direct = true;
            break;
          case P_TICK_DAMAGE:
            flags.periodic = true;
            break;
        }
      }
    }
  }

  demon_hunter_action_t( util::string_view n, demon_hunter_t* p,
                         const spell_data_t* s = spell_data_t::nil(),
                         const std::string& o = std::string() )
    : ab( n, p, s ),
      energize_delta( 0.0 ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr )
  {
    ab::parse_options( o );

    // Rank Passives
    ab::apply_affecting_aura( p->spec.disrupt_rank_3 );
    ab::apply_affecting_aura( p->spec.throw_glaive_rank_2 );

    // Conduit Passives
    ab::apply_affecting_conduit( p->conduit.dancing_with_fate );
    ab::apply_affecting_conduit( p->conduit.fel_defender );
    ab::apply_affecting_conduit( p->conduit.increased_scrutiny );
    ab::apply_affecting_conduit( p->conduit.unnatural_malice );

    // Generic Affect Flags
    parse_affect_flags( p->spec.demon_soul, affected_by.demon_soul );
    parse_affect_flags( p->spec.empowered_demon_soul, affected_by.empowered_demon_soul );

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      // Rank Passives
      ab::apply_affecting_aura( p->spec.chaos_strike_rank_3 );
      ab::apply_affecting_aura( p->spec.eye_beam_rank_2 );
      ab::apply_affecting_aura( p->spec.fel_rush_rank_2 );
      ab::apply_affecting_aura( p->spec.fel_rush_rank_3 );
      ab::apply_affecting_aura( p->spec.metamorphosis_rank_3 );

      // Talent Passives
      ab::apply_affecting_aura( p->talent.blind_fury );
      ab::apply_affecting_aura( p->talent.first_blood );
      ab::apply_affecting_aura( p->talent.insatiable_hunger );
      ab::apply_affecting_aura( p->talent.master_of_the_glaive );
      ab::apply_affecting_aura( p->talent.momentum );
      ab::apply_affecting_aura( p->talent.unleashed_power );

      // Legendary Passives
      ab::apply_affecting_aura( p->legendary.erratic_fel_core );

      // Affect Flags
      parse_affect_flags( p->mastery.demonic_presence, affected_by.demonic_presence );
      if ( p->talent.momentum->ok() )
      {
        parse_affect_flags( p->spec.momentum_buff, affected_by.momentum );
      }
      if ( p->talent.essence_break->ok() )
      {
        affected_by.essence_break = ab::data().affected_by( p->find_spell( 320338 )->effectN( 1 ) );
      }
      if ( p->legendary.burning_wound->ok() )
      {
        affected_by.burning_wound = ab::data().affected_by( p->legendary.burning_wound->effectN( 1 ).trigger()->effectN( 2 ) );
      }
    }
    else // DEMON_HUNTER_VENGEANCE
    {
      // Rank Passives
      ab::apply_affecting_aura( p->spec.fiery_brand_rank_3 );
      ab::apply_affecting_aura( p->spec.immolation_aura_rank_3 );
      ab::apply_affecting_aura( p->spec.infernal_strike_rank_2 );
      ab::apply_affecting_aura( p->spec.infernal_strike_rank_3 );
      ab::apply_affecting_aura( p->spec.metamorphosis_rank_2 );
      ab::apply_affecting_aura( p->spec.metamorphosis_rank_3 );
      ab::apply_affecting_aura( p->spec.soul_cleave_rank_3 );
      ab::apply_affecting_aura( p->spec.throw_glaive_rank_3 );

      // Talent Passives
      ab::apply_affecting_aura( p->talent.abyssal_strike );
      ab::apply_affecting_aura( p->talent.agonizing_flames );
      ab::apply_affecting_aura( p->talent.concentrated_sigils );
      ab::apply_affecting_aura( p->talent.quickened_sigils );
      ab::apply_affecting_aura( p->talent.ruinous_bulwark );
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

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    if ( affected_by.essence_break )
    {
      m *= 1.0 + td( target )->debuffs.essence_break->check_value();
    }

    if ( affected_by.burning_wound )
    {
      m *= 1.0 + td( target )->debuffs.burning_wound->check_value();
    }

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_da_multiplier( s );

    if ( affected_by.demonic_presence.direct )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.momentum.direct )
    {
      m *= 1.0 + p()->buff.momentum->check_value();
    }

    if ( affected_by.demon_soul.direct )
    {
      m *= p()->buff.demon_soul->check_value_direct();
    }

    if ( affected_by.empowered_demon_soul.direct )
    {
      m *= p()->buff.empowered_demon_soul->check_value_direct();
    }

    return m;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = ab::composite_ta_multiplier( s );

    if ( affected_by.demonic_presence.periodic )
    {
      m *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.momentum.periodic )
    {
      m *= 1.0 + p()->buff.momentum->check_value();
    }

    if ( affected_by.demon_soul.periodic )
    {
      m *= p()->buff.demon_soul->check_value_periodic();
    }

    if ( affected_by.empowered_demon_soul.periodic )
    {
      m *= p()->buff.empowered_demon_soul->check_value_periodic();
    }

    return m;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = ab::composite_energize_amount( s );

    if ( energize_delta > 0 )
    {
      ea += static_cast<int>( p()->rng().range( 0, 1 + energize_delta ) - ( energize_delta / 2.0 ) );
    }

    return ea;
  }

  void track_benefits( action_state_t* /* s */ )
  {
    if ( ab::snapshot_flags & STATE_MUL_TA )
    {
      p()->buff.demon_soul->up();
      p()->buff.empowered_demon_soul->up();
    }

    if ( affected_by.momentum.direct || affected_by.momentum.periodic )
    {
      p()->buff.momentum->up();
    }
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    accumulate_spirit_bomb( d->state );

    if ( d->state->result_amount > 0 )
    {
      track_benefits( d->state );
    }
  }

  void impact( action_state_t* s ) override
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

  void execute() override
  {
    ab::execute();

    if ( !ab::hit_any_target && ab::last_resource_cost > 0 )
    {
      trigger_refund();
    }
  }

  bool ready() override
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

  void update_ready( timespan_t cd ) override
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
    if ( ab::resource_current == RESOURCE_FURY )
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
  using base_t = demon_hunter_action_t<Base>;

private:
  /// typedef for the templated action type, eg. spell_t, attack_t, heal_t
  using ab = Base;
};

// ==========================================================================
// Demon Hunter Ability Classes
// ==========================================================================

struct demon_hunter_heal_t : public demon_hunter_action_t<heal_t>
{
  demon_hunter_heal_t( util::string_view n, demon_hunter_t* p,
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
  demon_hunter_spell_t( util::string_view n, demon_hunter_t* p,
                        const spell_data_t* s = spell_data_t::nil(),
                        const std::string& o = std::string() )
    : base_t( n, p, s, o )
  {}
};

struct demon_hunter_sigil_t : public demon_hunter_spell_t
{
  timespan_t sigil_delay;
  timespan_t sigil_activates;

  demon_hunter_sigil_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
    : demon_hunter_spell_t( n, p, s ),
    sigil_delay( delay ),
    sigil_activates( timespan_t::zero() )
  {
    aoe = -1;
    background = dual = ground_aoe = true;
    assert( delay > timespan_t::zero() );
  }

  void place_sigil( player_t* target )
  {
    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( target )
      .x( p()->talent.concentrated_sigils->ok() ? p()->x_position : target->x_position )
      .y( p()->talent.concentrated_sigils->ok() ? p()->y_position : target->y_position )
      .pulse_time( sigil_delay )
      .duration( sigil_delay )
      .action( this ) );

    sigil_activates = sim->current_time() + sigil_delay;
  }

  std::unique_ptr<expr_t> create_sigil_expression( util::string_view name );
};

struct demon_hunter_attack_t : public demon_hunter_action_t<melee_attack_t>
{
  demon_hunter_attack_t( util::string_view n, demon_hunter_t* p,
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
    demonic_appetite_energize_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.demonic_appetite_fury )
    {
      may_miss = may_block = may_dodge = may_parry = callbacks = false;
      background = quiet = dual = true;
      energize_type = action_energize::ON_CAST;
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
    may_miss = false;
    background = true;

    if ( p->talent.demonic_appetite->ok() )
    {
      execute_action = p->get_background_action<demonic_appetite_energize_t>( "demonic_appetite_fury" );
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
  fel_devastation_heal_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_heal_t( name, p, p->find_spell( 212106 ) )
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
    feast_of_souls_heal_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_heal_t( name, p, p->find_spell( 207693 ) )
    {
      dual = true;
    }
  };

  soul_cleave_heal_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_heal_t( name, p, p->spec.soul_cleave )
  {
    background = dual = true;

    // Clear out the costs since this is just a copy of the damage spell
    base_costs.fill( 0 );
    secondary_costs.fill( 0 );

    if ( p->talent.feast_of_souls->ok() )
    {
      execute_action = p->get_background_action<feast_of_souls_heal_t>( "feast_of_souls" );
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
    may_miss = false;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    p()->buff.blur->trigger();
  }
};

// Bulk Extraction ==========================================================

struct bulk_extraction_t : public demon_hunter_spell_t
{
  bulk_extraction_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "bulk_extraction", p, p->talent.bulk_extraction, options_str )
  {
    aoe = -1;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    unsigned num_souls = std::min( execute_state->n_targets, as<unsigned>( data().effectN( 2 ).base_value() ) );
    p()->spawn_soul_fragment( soul_fragment::LESSER, num_souls, true );
  }
};

// Chaos Nova ===============================================================

struct chaos_nova_t : public demon_hunter_spell_t
{
  chaos_nova_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "chaos_nova", p, p->spec.chaos_nova, options_str )
  {
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( s->target->type == ENEMY_ADD && p()->spec.chaos_nova_rank_2->ok() )
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
    may_miss = false;

    const spelleffect_data_t& effect = data().effectN( 1 );
    energize_type = action_energize::ON_CAST;
    energize_resource = effect.resource_gain_type();
    energize_amount = effect.resource( energize_resource );
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
    may_miss = harmful = false;
    use_off_gcd = true;
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
  disrupt_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "disrupt", p, p->spec.disrupt, options_str )
  {
    may_miss = false;
    is_interrupt = true;

    const spelleffect_data_t& effect = p->spec.disrupt_rank_2->effectN( 1 ).trigger()->effectN( 1 );
    energize_type = action_energize::ON_CAST;
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
    eye_beam_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 198030 ) )
    {
      background = dual = reduced_aoe_damage = true;
      aoe = -1;
    }

    double composite_target_multiplier( player_t* target ) const override
    {
      double m = demon_hunter_spell_t::composite_target_multiplier( target );

      m *= 1.0 + td( target )->debuffs.serrated_glaive->value();

      return m;
    }
  };

  eye_beam_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "eye_beam", p, p->spec.eye_beam, options_str )
  {
    may_miss = false;
    channeled = true;

    // 6/6/2020 - Override the lag handling for Eye Beam so that it doesn't use channeled ready behavior
    //            In-game tests have shown it is possible to cast after faster than the 250ms channel_lag using a nochannel macro
    ability_lag         = p->world_lag;
    ability_lag_stddev  = p->world_lag_stddev;

    tick_action = p->get_background_action<eye_beam_tick_t>( "eye_beam_tick" );

    if ( p->active.collective_anguish )
    {
      add_child( p->active.collective_anguish );
    }
  }

  void last_tick( dot_t* d ) override
  {
    demon_hunter_spell_t::last_tick( d );

    if ( p()->spec.eye_beam_rank_3->ok() )
      p()->buff.eye_beam_rank_3->trigger();
  }

  void execute() override
  {
    // Trigger Meta before the execute so that the channel duration is affected by Meta haste
    p()->trigger_demonic();

    demon_hunter_spell_t::execute();
    timespan_t duration = composite_dot_duration( execute_state );

    // Since Demonic triggers Meta with 8s + hasted duration, need to extend by the hasted duration after have an execute_state
    if ( p()->talent.demonic->ok() )
    {
      p()->buff.metamorphosis->extend_duration( p(), duration );
    }

    if ( p()->talent.blind_fury->ok() )
    {
      // Blind Fury gains scale with the duration of the channel
      p()->buff.blind_fury->set_duration( duration );
      p()->buff.blind_fury->set_period( timespan_t::from_millis( 100 ) * ( duration / dot_duration ) );
      p()->buff.blind_fury->trigger();
    }

    // Darkglare Boon Legendary
    if ( p()->legendary.darkglare_boon->ok() && p()->rng().roll( p()->legendary.darkglare_boon->effectN( 1 ).percent() ) )
    {
      cooldown->reset( true );
      p()->proc.darkglare_boon_resets->occur();
      // 02/18/2021 -- Added in PTR build
      if ( p()->spec.darkglare_boon_refund->ok() )
      {
        p()->resource_gain( RESOURCE_FURY, p()->spec.darkglare_boon_refund->effectN( 1 ).resource( RESOURCE_FURY ),
                            p()->gain.darkglare_boon_refund );
      }
    }

    // Collective Anguish Legendary
    if ( p()->active.collective_anguish )
    {
      p()->active.collective_anguish->set_target( target );
      p()->active.collective_anguish->execute();
    }
  }
};

// Fel Barrage ==============================================================

struct fel_barrage_t : public demon_hunter_spell_t
{
  struct fel_barrage_tick_t : public demon_hunter_spell_t
  {
    fel_barrage_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.fel_barrage->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = as<int>( data().effectN( 2 ).base_value() );
    }
  };

  fel_barrage_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t("fel_barrage", p, p->talent.fel_barrage, options_str)
  {
    may_miss = false;
    channeled = true;
    tick_action = p->get_background_action<fel_barrage_tick_t>( "fel_barrage_tick" );
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
    fel_devastation_tick_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.fel_devastation->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe = -1;
    }
  };

  heals::fel_devastation_heal_t* heal;

  fel_devastation_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fel_devastation", p, p->spec.fel_devastation, options_str ),
    heal( nullptr )
  {
    may_miss = false;
    channeled = true;

    tick_action = p->get_background_action<fel_devastation_tick_t>( "fel_devastation_tick" );

    if ( p->spec.fel_devastation_rank_2->ok() )
    {
      heal = p->get_background_action<heals::fel_devastation_heal_t>( "fel_devastation_heal" );
    }

    if ( p->active.collective_anguish )
    {
      add_child( p->active.collective_anguish );
    }
  }

  void execute() override
  {
    p()->trigger_demonic();
    demon_hunter_spell_t::execute();

    // Since Demonic triggers Meta with 8s + hasted duration, need to extend by the hasted duration after have an execute_state
    if ( p()->talent.demonic->ok() )
    {
      p()->buff.metamorphosis->extend_duration( p(), composite_dot_duration( execute_state ) );
    }

    // Darkglare Boon Legendary
    double darkglare_percent = p()->legendary.darkglare_boon->effectN( 2 ).percent();
    if ( p()->legendary.darkglare_boon->ok() && p()->rng().roll( darkglare_percent ) )
    {
      cooldown->reset( true );
      p()->proc.darkglare_boon_resets->occur();
      // 02/18/2021 -- Added in PTR build
      if ( p()->spec.darkglare_boon_refund->ok() )
      {
        p()->resource_gain( RESOURCE_FURY, p()->spec.darkglare_boon_refund->effectN( 2 ).resource( RESOURCE_FURY ),
                            p()->gain.darkglare_boon_refund );
      }
    }

    // Collective Anquish Legendary
    if ( p()->active.collective_anguish )
    {
      p()->active.collective_anguish->set_target( target );
      p()->active.collective_anguish->execute();
    }
  }

  void tick( dot_t* d ) override
  {
    if ( heal )
    {
      heal->execute(); // Heal happens before damage
    }

    demon_hunter_spell_t::tick( d );
  }
};

// Fel Eruption =============================================================

struct fel_eruption_t : public demon_hunter_spell_t
{
  fel_eruption_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fel_eruption", p, p->talent.fel_eruption, options_str )
  {
    may_crit = false;
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
    fiery_brand_dot_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 207771 ) )
    {
      background = dual = true;

      // Spread radius used for Burning Alive.
      if ( p->talent.burning_alive->ok() )
      {
        radius = p->find_spell( 207760 )->effectN( 1 ).radius_max();
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
        return;

      if ( !debug_cast<fiery_brand_state_t*>( d->state )->primary )
        return;

      // Invalidate and retrieve the new target list
      target_cache.is_valid = false;
      std::vector<player_t*> targets = target_list();
      if ( targets.size() == 1 )
        return;

      // Remove all the targets with existing Fiery Brand DoTs
      auto it = std::remove_if( targets.begin(), targets.end(), [ this ]( player_t* target ) {
        return this->td( target )->dots.fiery_brand->is_ticking();
      } );
      targets.erase( it, targets.end() );

      if ( targets.empty() )
        return;

      // Execute a dot on a random target
      player_t* target = targets[ static_cast<int>( p()->rng().range( 0, static_cast<double>( targets.size() ) ) ) ];
      this->set_target( target );
      this->schedule_execute();
    }
  };

  fiery_brand_dot_t* dot_action;

  fiery_brand_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "fiery_brand", p, p->spec.fiery_brand, options_str ),
    dot_action( nullptr )
  {
    use_off_gcd = true;

    dot_action = p->get_background_action<fiery_brand_dot_t>( "fiery_brand_dot" );
    dot_action->stats = stats;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );

    if ( result_is_miss( s->result ) )
      return;

    // Trigger the initial DoT action and set the primary flag for use with Burning Alive
    dot_action->set_target( s->target );
    fiery_brand_state_t* fb_state = debug_cast<fiery_brand_state_t*>( dot_action->get_state() );
    dot_action->snapshot_state( fb_state, result_amount_type::DMG_OVER_TIME );
    fb_state->primary = true;
    dot_action->schedule_execute( fb_state );
  }
};

// Glaive Tempest ===========================================================

struct glaive_tempest_t : public demon_hunter_spell_t
{
  struct glaive_tempest_damage_t : public demon_hunter_attack_t
  {
    glaive_tempest_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->find_spell( 342857 ) )
    {
      background = dual = ground_aoe = true;
    }
  };

  glaive_tempest_damage_t* glaive_tempest_mh;
  glaive_tempest_damage_t* glaive_tempest_oh;

  glaive_tempest_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "glaive_tempest", p, p->talent.glaive_tempest, options_str )
  {
    school = SCHOOL_CHAOS; // Reporting purposes only
    glaive_tempest_mh = p->get_background_action<glaive_tempest_damage_t>( "glaive_tempest_mh" );
    glaive_tempest_oh = p->get_background_action<glaive_tempest_damage_t>( "glaive_tempest_oh" );
    add_child( glaive_tempest_mh );
    add_child( glaive_tempest_oh );
  }

  void make_ground_aoe_event( glaive_tempest_damage_t* action )
  {
    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( execute_state->target )
      .x( p()->x_position )
      .y( p()->y_position )
      .pulse_time( 500_ms ) // Not in spell data
      .duration( data().duration() )
      .action( action ), true );
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    make_ground_aoe_event( glaive_tempest_mh );
    make_ground_aoe_event( glaive_tempest_oh );
  }
};

// Sigil of Flame ===========================================================

struct sigil_of_flame_damage_t : public demon_hunter_sigil_t
{
  sigil_of_flame_damage_t( util::string_view name, demon_hunter_t* p, timespan_t delay )
    : demon_hunter_sigil_t( name, p, p->find_spell( 204598 ), delay )
  {
  }

  dot_t* get_dot( player_t* t ) override
  {
    if ( !t ) t = target;
    if ( !t ) return nullptr;
    return td( t )->dots.sigil_of_flame;
  }
};

struct sigil_of_flame_t : public demon_hunter_spell_t
{
  sigil_of_flame_damage_t* sigil;

  sigil_of_flame_t(demon_hunter_t* p, const std::string& options_str)
    : demon_hunter_spell_t( "sigil_of_flame", p, p->spec.sigil_of_flame, options_str ),
    sigil( nullptr )
  {
    may_miss = false;

    if ( p->spec.sigil_of_flame_rank_2->ok() )
    {
      sigil = p->get_background_action<sigil_of_flame_damage_t>( "sigil_of_flame_damage", ground_aoe_duration );
      sigil->stats = stats;
    }

    // Add damage modifiers in sigil_of_flame_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

    if ( sigil )
    {
      sigil->place_sigil( execute_state->target );
    }
  }

  std::unique_ptr<expr_t> create_expression(util::string_view name) override
  {
    if ( auto e = sigil->create_sigil_expression( name ) )
      return e;

    return demon_hunter_spell_t::create_expression(name);
  }
};

// Sigil of the Illidari Legendary ==========================================

struct collective_anguish_t : public demon_hunter_spell_t
{
  struct collective_anguish_tick_t : public demon_hunter_spell_t
  {
    collective_anguish_tick_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_spell_t( name, p, s )
    {
      // TOCHECK: Currently does not use split damage on beta but probably will at some point
      background = dual = true;
      aoe = -1;
    }

    double composite_crit_chance() const override
    {
      // 03/15/2020 -- 100% crit rate as of 9.0.5, does not appear to be in any spell data
      if ( p()->specialization() == DEMON_HUNTER_VENGEANCE )
        return 1.0;

      return demon_hunter_spell_t::composite_crit_chance();
    }
  };

  collective_anguish_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s )
    : demon_hunter_spell_t( name, p, s )
  {
    may_miss = false;
    background = dual = hasted_ticks = tick_on_application = true;

    if( p->specialization() == DEMON_HUNTER_HAVOC )
      tick_action = p->get_background_action<collective_anguish_tick_t>( "collective_anguish_tick", data().effectN( 1 ).trigger() );
    else // Trigger data not set up correctly for Vengeance
      tick_action = p->get_background_action<collective_anguish_tick_t>( "collective_anguish_tick", p->find_spell( 333389 ) );
  }

  // Behaves as a channeled spell, although we can't set channeled = true since it is background
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }
};

// Infernal Strike ==========================================================

struct infernal_strike_t : public demon_hunter_spell_t
{
  struct infernal_strike_impact_t : public demon_hunter_spell_t
  {
    sigil_of_flame_damage_t* sigil;

    infernal_strike_impact_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 189112 ) ),
      sigil( nullptr )
    {
      background = dual = true;
      aoe = -1;

      if ( p->talent.abyssal_strike->ok() )
      {
        timespan_t sigil_delay = p->spec.sigil_of_flame->duration() - p->talent.quickened_sigils->effectN( 1 ).time_value();
        sigil = p->get_background_action<sigil_of_flame_damage_t>( "abyssal_strike", sigil_delay );
      }
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      if ( sigil )
      {
        sigil->place_sigil( execute_state->target );
      }
    }
  };

  infernal_strike_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "infernal_strike", p, p->spec.infernal_strike, options_str )
  {
    may_miss = false;
    base_teleport_distance  = data().max_range();
    movement_directionality = movement_direction_type::OMNI;
    travel_speed = 1.0;  // allows use precombat

    impact_action = p->get_background_action<infernal_strike_impact_t>( "infernal_strike_impact" );
    impact_action->stats = stats;
  }

  // leap travel time, independent of distance
  timespan_t travel_time() const override
  { return 1_s; }
};

// Immolation Aura ==========================================================

struct immolation_aura_t : public demon_hunter_spell_t
{
  struct infernal_armor_damage_t : public demon_hunter_spell_t
  {
    infernal_armor_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 320334 ) )
    {
    }
  };

  struct immolation_aura_damage_t : public demon_hunter_spell_t
  {
    bool initial;

    immolation_aura_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, bool initial )
      : demon_hunter_spell_t( name, p, s ), initial( initial )
    {
      background = dual = true;
      aoe = -1;

      // Rename gain for periodic energizes. Initial hit action doesn't energize.
      // Gains are encoded in the 258922 spell data differently for Havoc vs. Vengeance
      gain = p->get_gain( "immolation_aura_tick" );
      if ( !initial )
      {
        if ( p->specialization() == DEMON_HUNTER_VENGEANCE )
        {
          energize_amount = data().effectN( 3 ).base_value();
        }
        else // DEMON_HUNTER_HAVOC
        {
          energize_amount = p->talent.burning_hatred->ok() ? data().effectN( 2 ).base_value() : 0;
        }
      }
    }

    double action_multiplier() const override
    {
      double am = demon_hunter_spell_t::action_multiplier();

      if ( p()->conduit.growing_inferno.ok() )
      {
        am *= 1.0 + p()->buff.growing_inferno->stack_value();
      }

      return am;
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

        if ( p()->talent.charred_flesh->ok() )
        {
          demon_hunter_td_t* target_data = td( s->target );
          if ( target_data->dots.fiery_brand->is_ticking() )
          {
            target_data->dots.fiery_brand->adjust_duration( p()->talent.charred_flesh->effectN( 1 ).time_value() );
          }
        }

        // Fel Bombardment Legendary
        // Based on beta testing, does not appear to trigger from initial damage, only ticks
        if ( !initial )
        {
          p()->buff.fel_bombardment->trigger();
        }
      }
    }

    result_amount_type amount_type( const action_state_t*, bool ) const override
    {
      return initial ? result_amount_type::DMG_DIRECT : result_amount_type::DMG_OVER_TIME;
    }
  };

  immolation_aura_damage_t* initial_damage;

  immolation_aura_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "immolation_aura", p, p->spec.immolation_aura, options_str )
  {
    may_miss = false;
    dot_duration = timespan_t::zero();

    if ( !p->active.immolation_aura )
    {
      p->active.immolation_aura = p->get_background_action<immolation_aura_damage_t>( "immolation_aura_tick", data().effectN( 1 ).trigger(), false );
      p->active.immolation_aura->stats = stats;
    }

    // Initial damage is referenced indirectly in Immolation Aura (Rank 2) (id=320364) tooltip
    initial_damage = p->get_background_action<immolation_aura_damage_t>( "immolation_aura_initial", p->find_spell( 258921 ), true );
    initial_damage->stats = stats;

    if ( p->talent.infernal_armor->ok() && !p->active.infernal_armor )
    {
      p->active.infernal_armor = p->get_background_action<infernal_armor_damage_t>( "infernal_armor" );
      add_child( p->active.infernal_armor );
    }

    // Add damage modifiers in immolation_aura_damage_t, not here.
  }

  void execute() override
  {
    p()->buff.immolation_aura->trigger();

    demon_hunter_spell_t::execute();

    initial_damage->set_target( target );
    initial_damage->execute();

    if ( p()->talent.unbound_chaos->ok() )
    {
      p()->buff.unbound_chaos->trigger();
    }
  }
};

// Metamorphosis ============================================================

struct metamorphosis_t : public demon_hunter_spell_t
{
  struct metamorphosis_impact_t : public demon_hunter_spell_t
  {
    struct sinful_brand_meta_t : public demon_hunter_spell_t
    {
      sinful_brand_meta_t( util::string_view name, demon_hunter_t* p )
        : demon_hunter_spell_t( name, p, p->covenant.sinful_brand )
      {
      }
    };

    metamorphosis_impact_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 200166 ) )
    {
      background = dual = true;
      aoe = -1;

      if ( p->covenant.sinful_brand->ok() )
      {
        impact_action = p->get_background_action<sinful_brand_meta_t>( "sinful_brand" );
      }
    }
  };

  metamorphosis_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "metamorphosis", p, p->spec.metamorphosis, options_str )
  {
    may_miss = false;
    dot_duration = timespan_t::zero();

    if ( p->specialization() == DEMON_HUNTER_HAVOC )
    {
      base_teleport_distance  = data().max_range();
      movement_directionality = movement_direction_type::OMNI;
      min_gcd                 = timespan_t::from_seconds( 1.0 );  // Cannot use skills during travel time
      travel_speed            = 1.0;                              // Allows use in the precombat list

      impact_action = p->get_background_action<metamorphosis_impact_t>( "metamorphosis_impact" );
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
      p()->buff.metamorphosis->extend_duration_or_trigger();

      if ( p()->spec.metamorphosis_rank_4->ok() )
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
      type( soul_fragment::ANY )
  {
    std::string type_str;
    std::string mode_str;
    add_option( opt_string( "type", type_str ) );
    add_option( opt_string( "mode", mode_str ) );
    parse_options( options_str );
    
    parse_mode( mode_str );
    parse_type( mode_str );

    trigger_gcd = timespan_t::zero();
    // use_off_gcd = true;
    may_miss = callbacks = harmful = false;
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
    else if ( !value.empty() )
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
      type = soul_fragment::ANY_GREATER;
    }
    else if ( value == "lesser" )
    {
      type = soul_fragment::LESSER;
    }
    else if ( value == "all" || value == "any" )
    {
      type = soul_fragment::ANY;
    }
    else if ( value == "demon" )
    {
      type = soul_fragment::ANY_DEMON;
    }
    else if ( !value.empty() )
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
    p()->soul_fragment_pick_up = make_event<pick_up_event_t>( *sim, frag, time, if_expr.get() );
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
    spirit_bomb_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 247455 ) )
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
    max_fragments_consumed( static_cast<unsigned>( data().effectN( 2 ).base_value() ) )
  {
    may_miss = proc = callbacks = false;

    damage = p->get_background_action<spirit_bomb_damage_t>( "spirit_bomb_damage" );
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
    const int fragments_consumed = p()->consume_soul_fragments( soul_fragment::ANY, true, max_fragments_consumed );
    if ( fragments_consumed > 0 )
    {
      damage->set_target( target );
      action_state_t* damage_state = damage->get_state();
      damage->snapshot_state( damage_state, result_amount_type::DMG_DIRECT );
      damage_state->da_multiplier *= fragments_consumed;
      damage->schedule_execute( damage_state );
      damage->execute_event->reschedule( timespan_t::from_seconds( 1.0 ) );
    }
  }

  bool ready() override
  {
    if ( p()->get_active_soul_fragments() < 1 )
      return false;

    return demon_hunter_spell_t::ready();
  }
};

// Covenant Abilities =======================================================
// ==========================================================================

// Elysian Decree ===========================================================

struct elysian_decree_t : public demon_hunter_spell_t
{
  struct elysian_decree_sigil_t : public demon_hunter_sigil_t
  {
    elysian_decree_sigil_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s, timespan_t delay )
      : demon_hunter_sigil_t( name, p, s, delay )
    {
    }

    void execute() override
    {
      demon_hunter_sigil_t::execute();
      p()->spawn_soul_fragment( soul_fragment::LESSER, 3 );
    }
  };

  elysian_decree_sigil_t* sigil;
  elysian_decree_sigil_t* repeat_decree_sigil;

  elysian_decree_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "elysian_decree", p, p->covenant.elysian_decree, options_str ),
    sigil( nullptr ), repeat_decree_sigil( nullptr )
  {
    if ( p->covenant.elysian_decree->ok() )
    {
      sigil = p->get_background_action<elysian_decree_sigil_t>( "elysian_decree_sigil", p->find_spell( 307046 ), ground_aoe_duration );
      sigil->stats = stats;
    }

    if ( p->conduit.repeat_decree.ok() )
    {
      const spell_data_t* repeat_decree_driver = p->find_spell( 307046 )->effectN( 2 ).trigger();
      timespan_t repeat_decree_delay = timespan_t::from_millis( repeat_decree_driver->effectN( 2 ).misc_value1() );
      repeat_decree_sigil = p->get_background_action<elysian_decree_sigil_t>(
        "elysian_decree_repeat_decree", repeat_decree_driver->effectN( 2 ).trigger(), ground_aoe_duration + repeat_decree_delay );
      repeat_decree_sigil->base_multiplier = 1.0 + p->conduit.repeat_decree.percent();
      add_child( repeat_decree_sigil );
    }
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    sigil->place_sigil( target );
    if ( repeat_decree_sigil )
      repeat_decree_sigil->place_sigil( target );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name ) override
  {
    if ( sigil )
    {
      if ( auto e = sigil->create_sigil_expression( name ) )
        return e;
    }

    return demon_hunter_spell_t::create_expression( name );
  }
};

// Fodder to the Flame ======================================================

struct fodder_to_the_flame_cb_t : public dbc_proc_callback_t
{
  struct fodder_to_the_flame_damage_t : public demon_hunter_spell_t
  {
    double target_soft_cap;

    fodder_to_the_flame_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 350631 ) ),
      target_soft_cap( p->find_spell( 350570 )->effectN( 1 ).base_value() )
    {
      background = true;
      aoe = -1;
    }

    double composite_aoe_multiplier( const action_state_t* state ) const override
    {
      double cam = demon_hunter_spell_t::composite_aoe_multiplier( state );

      if ( state->n_targets > target_soft_cap )
      {
        cam *= std::sqrt( target_soft_cap / state->n_targets );
      }

      return cam;
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();
      p()->spawn_soul_fragment( soul_fragment::EMPOWERED_DEMON );
    }
  };

  // Dummy trigger spell in order to trigger Covenant cast callbacks
  struct fodder_to_the_flame_spawn_trigger_t : public demon_hunter_spell_t
  {
    struct fodder_to_the_flame_state_t : public action_state_t
    {
      fodder_to_the_flame_state_t( action_t* a, player_t* target ) : action_state_t( a, target ) {}

      proc_types2 cast_proc_type2() const override
      { return PROC2_CAST; }
    };

    fodder_to_the_flame_spawn_trigger_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 350570 ) )
    {
      quiet = true;
    }

    proc_types proc_type() const override
    { return PROC1_NONE_SPELL; }

    action_state_t* new_state() override
    { return new fodder_to_the_flame_state_t( this, target ); }
  };

  fodder_to_the_flame_damage_t* damage;
  fodder_to_the_flame_spawn_trigger_t* spawn_trigger;
  timespan_t trigger_delay;
  demon_hunter_t* dh;

  fodder_to_the_flame_cb_t( demon_hunter_t* p, const special_effect_t& e )
    : dbc_proc_callback_t( p, e ), dh( p )
  {
    damage = dh->get_background_action<fodder_to_the_flame_damage_t>( "fodder_to_the_flame" );
    spawn_trigger = dh->get_background_action<fodder_to_the_flame_spawn_trigger_t>( "fodder_to_the_flame_spawn" );
    trigger_delay = timespan_t::from_seconds( p->options.fodder_to_the_flame_kill_seconds );
  }

  void execute( action_t* a, action_state_t* s ) override
  {
    dbc_proc_callback_t::execute( a, s );
    make_event<delayed_execute_event_t>( *dh->sim, dh, damage, s->target, trigger_delay );
    spawn_trigger->execute();
  }
}; 

// Sinful Brand =============================================================

struct sinful_brand_t: public demon_hunter_spell_t
{
  sinful_brand_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "sinful_brand", p, p->covenant.sinful_brand, options_str )
  {
  }
};

// The Hunt =================================================================

struct the_hunt_t : public demon_hunter_spell_t
{
  struct the_hunt_damage_t : public demon_hunter_spell_t
  {
    struct the_hunt_dot_t : public demon_hunter_spell_t
    {
      the_hunt_dot_t( util::string_view name, demon_hunter_t* p )
        : demon_hunter_spell_t( name, p, p->covenant.the_hunt->effectN( 4 ).trigger() )
      {
        dual = true;
        aoe = as<int>( p->covenant.the_hunt->effectN( 2 ).trigger()->effectN( 1 ).base_value() );
      }
    };

    the_hunt_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->covenant.the_hunt->effectN( 1 ).trigger() )
    {
      dual = true;
      impact_action = p->get_background_action<the_hunt_dot_t>( "the_hunt_dot" );
    }
  };

  the_hunt_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "the_hunt", p, p->covenant.the_hunt, options_str )
  {
    movement_directionality = movement_direction_type::TOWARDS;
    impact_action = p->get_background_action<the_hunt_damage_t>( "the_hunt_damage" );
    impact_action->stats = stats;
    impact_action->impact_action->stats = stats;
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();
    p()->set_out_of_range( timespan_t::zero() ); // Cancel all other movement
  }

  timespan_t travel_time() const override
  { return 100_ms; }
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
      background = repeating = may_glance = may_crit = true;
      trigger_gcd = timespan_t::zero();
      weapon = w;
      weapon_multiplier = 1.0;
      base_execute_time = weapon->swing_time;

      affected_by.momentum.direct = true;

      status.main_hand = status.off_hand = LOST_CONTACT_RANGE;

      if ( p->dual_wield() )
      {
        base_hit -= 0.19;
      }
    }

    double action_multiplier() const override
    {
      double m = demon_hunter_attack_t::action_multiplier();

      m *= p()->buff.demon_soul->stack_value_auto_attack();
      m *= p()->buff.empowered_demon_soul->stack_value_auto_attack();

      return m;
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
      if ( p()->current.distance_to_move > 5 || p()->buff.out_of_range->check() )
      {
        status_e s = LOST_CONTACT_RANGE;
        p()->proc.delayed_aa_range->occur();

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
    p()->main_hand_attack->set_target( target );
    if ( p()->main_hand_attack->execute_event == nullptr )
    {
      p()->main_hand_attack->schedule_execute();
    }

    p()->off_hand_attack->set_target( target );
    if ( p()->off_hand_attack->execute_event == nullptr )
    {
      p()->off_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    // Range check
    if ( !demon_hunter_attack_t::ready() )
      return false;
    
    if ( p()->main_hand_attack->execute_event == nullptr || p()->off_hand_attack->execute_event == nullptr )
      return true;

    return false;
  }
};

// Blade Dance =============================================================

struct blade_dance_base_t : public demon_hunter_attack_t
{
  struct trail_of_ruin_dot_t : public demon_hunter_spell_t
  {
    trail_of_ruin_dot_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->talent.trail_of_ruin->effectN( 1 ).trigger() )
    {
      background = dual = true;
    }
  };

  struct blade_dance_damage_t : public demon_hunter_attack_t
  {
    timespan_t delay;
    action_t* trail_of_ruin_dot;
    bool last_attack;

    blade_dance_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff )
      : demon_hunter_attack_t( name, p, eff.trigger() ),
      delay( timespan_t::from_millis( eff.misc_value1() ) ),
      trail_of_ruin_dot( nullptr ),
      last_attack( false )
    {
      background = dual = true;
      aoe = as<int>( p->find_spell( 199552 )->effectN( 1 ).base_value() );
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double dm = demon_hunter_attack_t::composite_da_multiplier( s );

      if ( s->chain_target == 0 )
      {
        dm *= 1.0 + p()->talent.first_blood->effectN( 1 ).percent();
      }

      return dm;
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
      }
    }
  };

  std::vector<blade_dance_damage_t*> attacks;
  buff_t* dodge_buff;
  trail_of_ruin_dot_t* trail_of_ruin_dot;
  timespan_t ability_cooldown;

  blade_dance_base_t( const std::string& n, demon_hunter_t* p,
                      const spell_data_t* s, const std::string& options_str, buff_t* dodge_buff )
    : demon_hunter_attack_t( n, p, s, options_str ), dodge_buff( dodge_buff ), trail_of_ruin_dot ( nullptr )
  {
    may_miss = false;
    cooldown = p->cooldown.blade_dance;  // Blade Dance/Death Sweep Category Cooldown
    range = 5.0; // Disallow use outside of melee range.

    ability_cooldown = data().cooldown();
    if ( data().affected_by( p->spec.blade_dance_rank_2->effectN( 1 ) ) )
    {
      ability_cooldown += p->spec.blade_dance_rank_2->effectN( 1 ).time_value();
    }

    if ( p->talent.trail_of_ruin->ok() )
    {
      trail_of_ruin_dot = p->get_background_action<trail_of_ruin_dot_t>( "trail_of_ruin" );
    }
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

  void execute() override
  {
    // Blade Dance/Death Sweep Shared Category Cooldown
    cooldown->duration = ability_cooldown;

    demon_hunter_attack_t::execute();

    // Chaos Theory Legendary
    // This has a 0.5s ICD in the spell data, so just trigger once in the execute and not on impacts
    p()->buff.chaos_theory->trigger();

    // Metamorphosis benefit and Essence Break stats tracking
    if ( p()->buff.metamorphosis->up() )
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.death_sweep_in_essence_break->occur();
    }
    else
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.blade_dance_in_essence_break->occur();
    }

    // Create Strike Events
    for ( auto& attack : attacks )
    {
      make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
    }

    if ( dodge_buff )
    {
      dodge_buff->trigger();
    }
  }

  bool has_amount_result() const override
  { return true; }
};

struct blade_dance_t : public blade_dance_base_t
{
  blade_dance_t( demon_hunter_t* p, const std::string& options_str )
    : blade_dance_base_t( "blade_dance", p, p->spec.blade_dance, options_str, nullptr )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance1", data().effectN( 2 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance2", data().effectN( 3 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance3", data().effectN( 4 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "blade_dance4", data().effectN( 5 ) ) );
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
    : blade_dance_base_t( "death_sweep", p, p->spec.death_sweep, options_str, nullptr )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep1", data().effectN( 2 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep2", data().effectN( 3 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep3", data().effectN( 4 ) ) );
      attacks.push_back( p->get_background_action<blade_dance_damage_t>( "death_sweep4", data().effectN( 5 ) ) );
    }
  }

  void execute() override
  {
    blade_dance_base_t::execute();

    assert( p()->buff.metamorphosis->check() );

    // If Metamorphosis has less than 1s remaining, it gets extended so the whole Death Sweep happens during Meta.
    if ( p()->buff.metamorphosis->remains_lt( 1_s ) )
    {
      p()->buff.metamorphosis->extend_duration( p(), 1_s - p()->buff.metamorphosis->remains() );
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
    double refund_proc_chance;

    chaos_strike_damage_t( util::string_view name, demon_hunter_t* p, const spelleffect_data_t& eff, chaos_strike_base_t* a )
      : demon_hunter_attack_t( name, p, eff.trigger() ),
      delay( timespan_t::from_millis( eff.misc_value1() ) ),
      parent( a ),
      refund_proc_chance( 0.0 )
    {
      assert( eff.type() == E_TRIGGER_SPELL );
      background = dual = true;

      // Disable refunds on secondary procs such as Relentless Onslaught
      may_refund = ( weapon == &( p->off_hand_weapon ) ) && !a->from_onslaught;
      if ( may_refund )
      {
        refund_proc_chance = p->spec.chaos_strike_refund->proc_chance();
        refund_proc_chance += p->spec.chaos_strike_rank_2->effectN( 1 ).percent();
      }
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = demon_hunter_attack_t::composite_da_multiplier( s );

      m *= 1.0 + p()->buff.chaos_theory->stack_value();

      return m;
    }

    double get_refund_proc_chance()
    {
      double chance = refund_proc_chance;

      if ( p()->buff.chaos_theory->check() )
      {
        chance += p()->buff.chaos_theory->data().effectN( 2 ).percent();
      }

      return chance;
    }

    void execute() override
    {
      demon_hunter_attack_t::execute();

      // Technically this appears to have a 0.5s ICD, but this is handled elsewhere
      if ( may_refund && p()->rng().roll( this->get_refund_proc_chance() ) )
      {
        p()->resource_gain( RESOURCE_FURY, p()->spec.chaos_strike_fury->effectN( 1 ).resource( RESOURCE_FURY ), parent->gain );

        if ( p()->talent.cycle_of_hatred->ok() && p()->cooldown.eye_beam->down() )
        {
          const double adjust_seconds = p()->talent.cycle_of_hatred->effectN( 1 ).base_value();
          p()->cooldown.eye_beam->adjust( timespan_t::from_seconds( -adjust_seconds ) );
        }
      }
    }

    void impact( action_state_t* s ) override
    {
      demon_hunter_attack_t::impact( s );

      // 02/09/2021 -- Confirmed that due to timing changes, Onslaught can't proc refunds
      if ( result_is_hit( s->result ) && may_refund && p()->conduit.relentless_onslaught->ok() )
      {
        if ( p()->rng().roll( p()->conduit.relentless_onslaught.percent() ) )
        {
          attack_t* const relentless_onslaught = p()->buff.metamorphosis->check() ?
            p()->active.relentless_onslaught_annihilation : p()->active.relentless_onslaught;
          relentless_onslaught->set_target( s->target );
          relentless_onslaught->schedule_execute();
        }
      }
    }
  };

  std::vector<chaos_strike_damage_t*> attacks;
  bool from_onslaught;

  chaos_strike_base_t( util::string_view n, demon_hunter_t* p, const spell_data_t* s, const std::string& options_str = "", bool from_onslaught = false )
    : demon_hunter_attack_t( n, p, s, options_str ),
    from_onslaught( from_onslaught )
  {
  }

  double cost() const override
  {
    if ( from_onslaught )
      return 0.0;

    return demon_hunter_attack_t::cost();
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

  void execute() override
  {
    demon_hunter_attack_t::execute();

    // Create Strike Events
    for ( auto& attack : attacks )
    {
      make_event<delayed_execute_event_t>( *sim, p(), attack, target, attack->delay );
    }

    // Metamorphosis benefit and Essence Break stats tracking
    if ( p()->buff.metamorphosis->up() )
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.annihilation_in_essence_break->occur();
    }
    else
    {
      if ( td( target )->debuffs.essence_break->up() )
        p()->proc.chaos_strike_in_essence_break->occur();
    }

    // Demonic Appetite
    if ( p()->talent.demonic_appetite->ok() && p()->rppm.demonic_appetite->trigger() )
    {
      p()->proc.demonic_appetite->occur();
      p()->spawn_soul_fragment( soul_fragment::LESSER );
    }
  }

  bool has_amount_result() const override
  { return true; }
};

struct chaos_strike_t : public chaos_strike_base_t
{
  chaos_strike_t( util::string_view name, demon_hunter_t* p, const std::string& options_str = "", bool from_onslaught = false )
    : chaos_strike_base_t( name, p, p->spec.chaos_strike, options_str, from_onslaught )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_1", name ), data().effectN( 2 ), this ) );
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_2", name ), data().effectN( 3 ), this ) );
    }

    if ( !from_onslaught && p->active.relentless_onslaught )
    {
      add_child( p->active.relentless_onslaught );
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
  annihilation_t( util::string_view name, demon_hunter_t* p, const std::string& options_str = "", bool from_onslaught = false )
    : chaos_strike_base_t( name, p, p->spec.annihilation, options_str, from_onslaught )
  {
    if ( attacks.empty() )
    {
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_1", name ), data().effectN( 2 ), this ) );
      attacks.push_back( p->get_background_action<chaos_strike_damage_t>( fmt::format( "{}_damage_2", name ), data().effectN( 3 ), this ) );
    }

    if ( !from_onslaught && p->active.relentless_onslaught_annihilation )
    {
      add_child( p->active.relentless_onslaught_annihilation );
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

// Burning Wound Legendary ==================================================

struct burning_wound_t : public demon_hunter_spell_t
{
  burning_wound_t( util::string_view name, demon_hunter_t* p )
    : demon_hunter_spell_t( name, p, p->legendary.burning_wound->effectN( 1 ).trigger() )
  {
    dual = true;
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_spell_t::impact( s );
    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs.burning_wound->trigger();
    }
  }
};

// Demon's Bite =============================================================

struct demons_bite_t : public demon_hunter_attack_t
{
  demons_bite_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "demons_bite", p, p->spec.demons_bite, options_str )
  {
    energize_delta = energize_amount * data().effectN( 3 ).m_delta();

    if ( p->legendary.burning_wound->ok() && !p->talent.demon_blades->ok() )
    {
      impact_action = p->get_background_action<burning_wound_t>( "burning_wound" );
      add_child( impact_action );
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    if ( p()->talent.insatiable_hunger->ok() )
    {
      ea += static_cast<int>( p()->rng().range( p()->talent.insatiable_hunger->effectN( 3 ).base_value(),
                                                1 + p()->talent.insatiable_hunger->effectN( 4 ).base_value() ) );
    }

    return ea;
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

    if ( p->legendary.burning_wound->ok() && p->talent.demon_blades->ok() )
    {
      impact_action = p->get_background_action<burning_wound_t>( "burning_wound" );
      add_child( impact_action );
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );
    trigger_felblade( s );
  }
};

// Essence Break ============================================================

struct essence_break_t : public demon_hunter_attack_t
{
  essence_break_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "essence_break", p, p->talent.essence_break, options_str )
  {
    aoe = -1;
    school = SCHOOL_CHAOS; // Manually setting for now since the spell data is broken
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs.essence_break->trigger();
    }
  }
};

// Felblade =================================================================
// TODO: Real movement stuff.

struct felblade_t : public demon_hunter_attack_t
{
  struct felblade_damage_t : public demon_hunter_attack_t
  {
    felblade_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->find_spell( 213243 ) )
    {
      background = dual = true;
      gain = p->get_gain( "felblade" );
    }
  };

  felblade_damage_t* damage;

  felblade_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "felblade", p, p->talent.felblade, options_str )
  {
    may_block = false;
    movement_directionality = movement_direction_type::TOWARDS;

    execute_action = p->get_background_action<felblade_damage_t>( "felblade_damage" );
    execute_action->stats = stats;

    // Add damage modifiers in felblade_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();
    p()->set_out_of_range( timespan_t::zero() );// Cancel all other movement
  }
};

// Fel Rush =================================================================

struct fel_rush_t : public demon_hunter_attack_t
{
  struct fel_rush_damage_t : public demon_hunter_spell_t
  {
    fel_rush_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->spec.fel_rush_damage )
    {
      background = dual = true;
      aoe = -1;
    }

    double action_multiplier() const override
    {
      double am = demon_hunter_spell_t::action_multiplier();

      am *= 1.0 + p()->buff.unbound_chaos->value();

      return am;
    }
  };

  bool a_cancel;
  timespan_t gcd_lag;

  fel_rush_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "fel_rush", p, p->spec.fel_rush ),
      a_cancel( false )
  {
    add_option( opt_bool( "animation_cancel", a_cancel ) );
    parse_options( options_str );

    may_miss = may_dodge = may_parry = may_block = false;
    min_gcd = trigger_gcd;

    execute_action = p->get_background_action<fel_rush_damage_t>( "fel_rush_damage" );
    execute_action->stats = stats;

    if ( !a_cancel )
    {
      // Fel Rush does damage in a further line than it moves you
      base_teleport_distance = execute_action->radius - 5;
      movement_directionality = movement_direction_type::OMNI;
      p->buff.fel_rush_move->distance_moved = base_teleport_distance;
    }

    // Add damage modifiers in fel_rush_damage_t, not here.
  }

  void execute() override
  {
    p()->buff.momentum->trigger();

    demon_hunter_attack_t::execute();

    p()->buff.unbound_chaos->expire();

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
    fracture_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_attack_t( name, p, s )
    {
      dual = true;
      may_miss = may_dodge = may_parry = false;
    }
  };

  fracture_damage_t* mh, *oh;

  fracture_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "fracture", p, p->talent.fracture, options_str )
  {
    mh = p->get_background_action<fracture_damage_t>( "fracture_mh", data().effectN( 2 ).trigger() );
    oh = p->get_background_action<fracture_damage_t>( "fracture_oh", data().effectN( 3 ).trigger() );
    mh->stats = oh->stats = stats;
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    if ( p()->buff.metamorphosis->check() )
    {
      ea += p()->spec.metamorphosis_buff->effectN( 9 ).resource( RESOURCE_FURY );
    }

    return ea;
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

      p()->spawn_soul_fragment( soul_fragment::LESSER, as<int>( data().effectN( 1 ).base_value() ) );
      p()->proc.soul_fragment_from_fracture->occur();
      p()->proc.soul_fragment_from_fracture->occur();

      if ( p()->buff.metamorphosis->check() )
      {
        p()->spawn_soul_fragment( soul_fragment::LESSER );
        p()->proc.soul_fragment_from_meta->occur();
      }
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
    trigger_felblade( s );

    if ( result_is_hit( s->result ) )
    {
      p()->spawn_soul_fragment( soul_fragment::LESSER );
      p()->proc.soul_fragment_from_shear->occur();

      if ( p()->buff.metamorphosis->check() )
      {
        p()->spawn_soul_fragment( soul_fragment::LESSER );
        p()->proc.soul_fragment_from_meta->occur();
      }
    }
  }

  double composite_energize_amount( const action_state_t* s ) const override
  {
    double ea = demon_hunter_attack_t::composite_energize_amount( s );

    if ( p()->buff.metamorphosis->check() )
    {
      ea += p()->spec.metamorphosis_buff->effectN( 4 ).resource( RESOURCE_FURY );
    }

    return ea;
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
    soul_cleave_damage_t( util::string_view name, demon_hunter_t* p, const spell_data_t* s )
      : demon_hunter_attack_t( name, p, s )
    {
      dual = true;
      aoe = as<int>( data().effectN( 2 ).base_value() );
    }

    double action_multiplier() const override
    {
      double am = demon_hunter_attack_t::action_multiplier();

      if ( p()->buff.soul_furance->at_max_stacks() )
      {
        am *= 1.0 + p()->buff.soul_furance->check_value();
      }

      return am;
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
  std::vector<cooldown_t*> sigil_cooldowns;
  timespan_t sigil_cooldown_adjust;
  timespan_t decree_cooldown_adjust;

  soul_cleave_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "soul_cleave", p, p->spec.soul_cleave, options_str ),
    heal( nullptr )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    attack_power_mod.direct = 0;  // This parent action deals no damage, parsed data is for the heal

    execute_action = p->get_background_action<soul_cleave_damage_t>( "soul_cleave_damage", data().effectN( 2 ).trigger() );
    execute_action->stats = stats;

    if ( p->spec.soul_cleave_rank_2->ok() )
    {
      heal = p->get_background_action<heals::soul_cleave_heal_t>( "soul_cleave_heal" );
    }

    if ( p->legendary.razelikhs_defilement->ok() )
    {
      sigil_cooldowns = { p->cooldown.sigil_of_flame, p->cooldown.elysian_decree };
      sigil_cooldown_adjust = -timespan_t::from_seconds( p->legendary.razelikhs_defilement->effectN( 1 ).base_value() );
      decree_cooldown_adjust = -timespan_t::from_seconds( p->legendary.razelikhs_defilement->effectN( 2 ).base_value() );
    }

    // Add damage modifiers in soul_cleave_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_attack_t::execute();

    if ( heal )
    {
      heal->set_target( player );
      heal->execute();
    }

    if ( p()->buff.soul_furance->at_max_stacks() )
    {
      p()->buff.soul_furance->expire();
    }

    if ( p()->legendary.razelikhs_defilement->ok() && !p()->rng().roll( p()->options.razelikhs_defilement_utility_pct ) )
    {
      std::vector<cooldown_t*> sigils_on_cooldown;
      range::copy_if( sigil_cooldowns, std::back_inserter( sigils_on_cooldown ), []( cooldown_t* c ) { return c->down(); } );
      if ( !sigils_on_cooldown.empty() )
      {
        cooldown_t* sigil_cooldown = sigils_on_cooldown[ rng().range( sigils_on_cooldown.size() ) ];
        if ( sigil_cooldown == p()->cooldown.elysian_decree )
          sigil_cooldown->adjust( decree_cooldown_adjust );
        else
          sigil_cooldown->adjust( sigil_cooldown_adjust );
      }
    }

    // Soul fragments consumed are capped for Soul Cleave
    unsigned consumed = p()->consume_soul_fragments( soul_fragment::ANY, true, (unsigned)data().effectN( 3 ).base_value() );

    if ( p()->legendary.fiery_soul->ok() )
    {
      const timespan_t adjust_duration = -timespan_t::from_seconds( p()->legendary.fiery_soul->effectN( 1 ).base_value() );
      p()->cooldown.fiery_brand->adjust( adjust_duration * consumed );
    }
  }
};

// Throw Glaive =============================================================

struct throw_glaive_t : public demon_hunter_attack_t
{
  struct throw_glaive_damage_t : public demon_hunter_attack_t
  {
    throw_glaive_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_attack_t( name, p, p->spec.throw_glaive->effectN( 1 ).trigger() )
    {
      background = dual = true;
      radius = 10.0;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = demon_hunter_attack_t::composite_da_multiplier( s );

      m *= 1.0 + td( s->target )->debuffs.fel_bombardment->stack_value();

      return m;
    }
  };

  throw_glaive_damage_t* fel_bombardment;

  throw_glaive_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_attack_t( "throw_glaive", p, p->spec.throw_glaive, options_str ),
    fel_bombardment( nullptr )
  {
    execute_action = p->get_background_action<throw_glaive_damage_t>( "throw_glaive" );
    execute_action->stats = stats;

    if ( p->legendary.fel_bombardment->ok() )
    {
      fel_bombardment = p->get_background_action<throw_glaive_damage_t>( "fel_bombardment" );
      add_child( fel_bombardment );
    }
  }

  void execute() override
  {
    int bombardment_stacks = p()->buff.fel_bombardment->check();
    if ( bombardment_stacks > 0 )
    {
      // Apply hidden damage debuff to the primary target before execute
      td( target )->debuffs.fel_bombardment->trigger( bombardment_stacks );
      p()->buff.fel_bombardment->expire();
    }

    demon_hunter_attack_t::execute();

    // Fel Bombardment Legendary
    // In-game, this directly just triggers additional procs of the Throw Glaive spell
    // For SimC purposes, using a cloned spell for better stats tracking
    if ( hit_any_target && fel_bombardment && bombardment_stacks > 0 )
    {
      // 12/03/2020 - Apparently hotfixed to work correctly, although it still doesn't work how it did on beta
      // For each stack, pick a new target and trigger a glaive until we run out of targets
      const auto targets_in_range = targets_in_range_list( target_list() );
      for ( auto bombardment_target : targets_in_range )
      {
        // Does not throw an additional glaive at the primary target
        if ( bombardment_target == target )
          continue;

        if ( bombardment_stacks < 1 )
          break;

        fel_bombardment->set_target( bombardment_target );
        fel_bombardment->execute();
        bombardment_stacks--;
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    demon_hunter_attack_t::impact( s );

    if ( p()->conduit.serrated_glaive.ok() )
    {
      td( s->target )->debuffs.serrated_glaive->trigger();
    }
  }
};

// Vengeful Retreat =========================================================

struct vengeful_retreat_t : public demon_hunter_spell_t
{
  struct vengeful_retreat_damage_t : public demon_hunter_spell_t
  {
    vengeful_retreat_damage_t( util::string_view name, demon_hunter_t* p )
      : demon_hunter_spell_t( name, p, p->find_spell( 198813 ) )
    {
      background = dual = true;
      aoe = -1;
    }

    void execute() override
    {
      demon_hunter_spell_t::execute();

      if ( hit_any_target )
      {
        p()->buff.prepared->trigger();
      }
    }
  };

  vengeful_retreat_t( demon_hunter_t* p, const std::string& options_str )
    : demon_hunter_spell_t( "vengeful_retreat", p, p->spec.vengeful_retreat, options_str )
  {
    execute_action = p->get_background_action<vengeful_retreat_damage_t>( "vengeful_retreat_damage" );
    execute_action->stats = stats;

    base_teleport_distance = VENGEFUL_RETREAT_DISTANCE;
    movement_directionality = movement_direction_type::OMNI;
    p->buff.vengeful_retreat_move->distance_moved = base_teleport_distance;

    // Add damage modifiers in vengeful_retreat_damage_t, not here.
  }

  void execute() override
  {
    demon_hunter_spell_t::execute();

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

    return demon_hunter_spell_t::ready();
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

  demon_hunter_t* p()
  {
    return static_cast<demon_hunter_t*>( BuffBase::source );
  }

  const demon_hunter_t* p() const
  {
    return static_cast<const demon_hunter_t*>( BuffBase::source );
  }

private:
  using bb = BuffBase;
};

// Immolation Aura ==========================================================

struct immolation_aura_buff_t : public demon_hunter_buff_t<buff_t>
{
  immolation_aura_buff_t( demon_hunter_t* p )
    : base_t( *p, "immolation_aura", p->spec.immolation_aura )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value_from_effect_type( A_MOD_SPEED_ALWAYS );
    apply_affecting_aura( p->spec.immolation_aura_rank_3 );
    apply_affecting_aura( p->talent.agonizing_flames );

    set_tick_callback( [ p ]( buff_t*, int, timespan_t ) {
      p->active.immolation_aura->execute();
      p->buff.growing_inferno->trigger();
    } );

    if ( p->conduit.growing_inferno.ok() )
    {
      set_stack_change_callback( [ p ]( buff_t*, int, int ) {
        p->buff.growing_inferno->expire();
      } );
    }

    if ( p->talent.agonizing_flames->ok() )
    {
      add_invalidate( CACHE_RUN_SPEED );
    }

    if ( p->talent.infernal_armor->ok() )
    {
      add_invalidate( CACHE_ARMOR );
    }
  }
};

// Metamorphosis Buff =======================================================

struct metamorphosis_buff_t : public demon_hunter_buff_t<buff_t>
{
  metamorphosis_buff_t(demon_hunter_t* p)
    : base_t(*p, "metamorphosis", p->spec.metamorphosis_buff)
  {
    set_cooldown( timespan_t::zero() );
    buff_period = timespan_t::zero();
    tick_behavior = buff_tick_behavior::NONE;

    if (p->specialization() == DEMON_HUNTER_HAVOC)
    {
      set_default_value_from_effect_type( A_HASTE_ALL );
      apply_affecting_aura( p->spec.metamorphosis_rank_2 );
      add_invalidate( CACHE_HASTE );
      add_invalidate( CACHE_LEECH );
    }
    else // DEMON_HUNTER_VENGEANCE
    {
      set_default_value_from_effect_type( A_MOD_INCREASE_HEALTH_PERCENT );
      add_invalidate( CACHE_ARMOR );
      if ( p->talent.soul_rending->ok() )
      {
        add_invalidate( CACHE_LEECH );
      }
    }
  }

  void trigger_demonic()
  {
    const timespan_t extend_duration = p()->talent.demonic->effectN( 1 ).time_value();
    p()->buff.metamorphosis->extend_duration_or_trigger( extend_duration );
  }

  void start(int stacks, double value, timespan_t duration) override
  {
    demon_hunter_buff_t<buff_t>::start(stacks, value, duration);

    if ( p()->specialization() == DEMON_HUNTER_VENGEANCE )
    {
      p()->metamorphosis_health = p()->max_health() * value;
      p()->stat_gain( STAT_MAX_HEALTH, p()->metamorphosis_health, ( gain_t* )nullptr, ( action_t* )nullptr, true );
    }
  }

  void expire_override(int expiration_stacks, timespan_t remaining_duration) override
  {
    demon_hunter_buff_t<buff_t>::expire_override(expiration_stacks, remaining_duration);

    if (p()->specialization() == DEMON_HUNTER_VENGEANCE)
    {
      p()->stat_loss(STAT_MAX_HEALTH, p()->metamorphosis_health, (gain_t*)nullptr, (action_t*)nullptr, true);
      p()->metamorphosis_health = 0;
    }
  }
};

// Demon Spikes buff ========================================================

struct demon_spikes_t : public demon_hunter_buff_t<buff_t>
{
  const timespan_t max_duration;

  demon_spikes_t(demon_hunter_t* p)
    : base_t( *p, "demon_spikes", p->find_spell( 203819 ) ),
      max_duration( base_buff_duration * 3 ) // Demon Spikes can only be extended to 3x its base duration
  {
    set_default_value_from_effect_type( A_MOD_PARRY_PERCENT );
    set_refresh_behavior( buff_refresh_behavior::EXTEND );
    add_invalidate( CACHE_PARRY );
    add_invalidate( CACHE_ARMOR );
  }

  bool trigger(int stacks, double value, double chance, timespan_t duration) override
  {
    if (duration == timespan_t::min())
    {
      duration = buff_duration();
    }

    if (remains() + buff_duration() > max_duration)
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

movement_buff_t::movement_buff_t( demon_hunter_t* p, const std::string& name, const spell_data_t* spell_data, const item_t* item )
  : buff_t( p, name, spell_data, item ), yards_from_melee( 0.0 ), distance_moved( 0.0 ), dh( p )
{
}

// ==========================================================================
// Targetdata Definitions
// ==========================================================================

demon_hunter_td_t::demon_hunter_td_t( player_t* target, demon_hunter_t& p )
  : actor_target_data_t( target, &p ), dots( dots_t() ), debuffs( debuffs_t() )
{
  if ( p.specialization() == DEMON_HUNTER_HAVOC )
  {
    debuffs.essence_break = make_buff( *this, "essence_break", p.find_spell( 320338 ) )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_cooldown( timespan_t::zero() );

    debuffs.burning_wound = make_buff( *this, "burning_wound", p.legendary.burning_wound->effectN( 1 ).trigger() )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS );
    dots.burning_wound = target->get_dot( "burning_wound", &p );
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    dots.fiery_brand = target->get_dot("fiery_brand", &p);
    dots.sigil_of_flame = target->get_dot("sigil_of_flame", &p);
    debuffs.frailty = make_buff( *this, "frailty", p.find_spell( 247456 ) )
      ->set_default_value_from_effect( 1 );
    debuffs.void_reaver = make_buff( *this, "void_reaver", p.find_spell( 268178 ) )
      ->set_default_value_from_effect_type( A_MOD_DAMAGE_TO_CASTER );
  }

  dots.sinful_brand = target->get_dot( "sinful_brand", &p );
  debuffs.serrated_glaive = make_buff( *this, "exposed_wound", p.conduit.serrated_glaive->effectN( 1 ).trigger() )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER_SPELLS );

  debuffs.fel_bombardment = make_buff( *this, "fel_bombardment_debuff", p.find_spell( 337849 ) )
    ->set_default_value_from_effect( 1, 0.01 )
    ->set_duration( 0.5_s );

  target->register_on_demise_callback( &p, [this]( player_t* ) { target_demise(); } );
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

demon_hunter_t::demon_hunter_t(sim_t* sim, util::string_view name, race_e r)
  : player_t(sim, DEMON_HUNTER, name, r),
  melee_main_hand(nullptr),
  melee_off_hand(nullptr),
  next_fragment_spawn(0),
  soul_fragments(),
  spirit_bomb_accumulator(0.0),
  spirit_bomb_driver(nullptr),
  exit_melee_event(nullptr),
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

  resource_regeneration = regen_type::DISABLED;
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

action_t* demon_hunter_t::create_action( util::string_view name, const std::string& options_str )
{
  using namespace actions::heals;

  if ( name == "soul_barrier" )       return new soul_barrier_t( this, options_str );

  using namespace actions::spells;

  if ( name == "blur" )               return new blur_t( this, options_str );
  if ( name == "bulk_extraction" )    return new bulk_extraction_t( this, options_str );
  if ( name == "chaos_nova" )         return new chaos_nova_t( this, options_str );
  if ( name == "consume_magic" )      return new consume_magic_t( this, options_str );
  if ( name == "demon_spikes" )       return new demon_spikes_t( this, options_str );
  if ( name == "disrupt" )            return new disrupt_t( this, options_str );
  if ( name == "eye_beam" )           return new eye_beam_t( this, options_str );
  if ( name == "fel_barrage" )        return new fel_barrage_t( this, options_str );
  if ( name == "fel_eruption" )       return new fel_eruption_t( this, options_str );
  if ( name == "fel_devastation" )    return new fel_devastation_t( this, options_str );
  if ( name == "fiery_brand" )        return new fiery_brand_t( this, options_str );
  if ( name == "glaive_tempest" )     return new glaive_tempest_t( this, options_str );
  if ( name == "infernal_strike" )    return new infernal_strike_t( this, options_str );
  if ( name == "immolation_aura" )    return new immolation_aura_t( this, options_str );
  if ( name == "metamorphosis" )      return new metamorphosis_t( this, options_str );
  if ( name == "pick_up_fragment" )   return new pick_up_fragment_t( this, options_str );
  if ( name == "sigil_of_flame" )     return new sigil_of_flame_t( this, options_str );
  if ( name == "spirit_bomb" )        return new spirit_bomb_t( this, options_str );

  if ( name == "elysian_decree" )     return new elysian_decree_t( this, options_str );
  if ( name == "sinful_brand" )       return new sinful_brand_t( this, options_str );
  if ( name == "the_hunt" )           return new the_hunt_t( this, options_str );

  using namespace actions::attacks;

  if ( name == "auto_attack" )        return new auto_attack_t( this, options_str );
  if ( name == "annihilation" )       return new annihilation_t( "annihilation", this, options_str );
  if ( name == "blade_dance" )        return new blade_dance_t( this, options_str );
  if ( name == "chaos_strike" )       return new chaos_strike_t( "chaos_strike", this, options_str );
  if ( name == "essence_break" )      return new essence_break_t( this, options_str );
  if ( name == "death_sweep" )        return new death_sweep_t( this, options_str );
  if ( name == "demons_bite" )        return new demons_bite_t( this, options_str );
  if ( name == "felblade" )           return new felblade_t( this, options_str );
  if ( name == "fel_rush" )           return new fel_rush_t( this, options_str );
  if ( name == "fracture" )           return new fracture_t( this, options_str );
  if ( name == "shear" )              return new shear_t( this, options_str );
  if ( name == "soul_cleave" )        return new soul_cleave_t( this, options_str );
  if ( name == "throw_glaive" )       return new throw_glaive_t( this, options_str );
  if ( name == "vengeful_retreat" )   return new vengeful_retreat_t( this, options_str );

  return base_t::create_action( name, options_str );
}

// demon_hunter_t::create_buffs =============================================

void demon_hunter_t::create_buffs()
{
  base_t::create_buffs();

  using namespace buffs;

  // General ================================================================

  buff.demon_soul = make_buff<damage_buff_t>( this, "demon_soul", spec.demon_soul );
  buff.empowered_demon_soul = make_buff<damage_buff_t>( this, "empowered_demon_soul", spec.empowered_demon_soul )
    ->apply_affecting_conduit( conduit.brooding_pool );
  buff.immolation_aura = new buffs::immolation_aura_buff_t( this );
  buff.metamorphosis = new buffs::metamorphosis_buff_t( this );

  // Havoc ==================================================================

  buff.blur = make_buff( this, "blur", spec.blur->effectN( 1 ).trigger() )
    ->set_default_value_from_effect_type( A_MOD_DAMAGE_PERCENT_TAKEN )
    ->apply_affecting_aura( talent.desperate_instincts )
    ->set_cooldown( timespan_t::zero() )
    ->add_invalidate( CACHE_LEECH )
    ->add_invalidate( CACHE_DODGE );

  buff.eye_beam_rank_3 = make_buff( this, "furious_gaze", find_spell( 343312 ) )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_default_value_from_effect_type( A_HASTE_ALL )
    ->add_invalidate( CACHE_HASTE );

  buff.fel_rush_move = new movement_buff_t( this, "fel_rush_movement", spell_data_t::nil() );
  buff.fel_rush_move->set_chance( 1.0 )
    ->set_duration( spec.fel_rush->gcd() );

  buff.momentum = make_buff( this, "momentum", spec.momentum_buff )
    ->set_trigger_spell( talent.momentum )
    ->set_default_value_from_effect( 1 )
    ->set_refresh_behavior( buff_refresh_behavior::EXTEND )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.out_of_range = make_buff( this, "out_of_range", spell_data_t::nil() )
    ->set_chance( 1.0 );

  buff.prepared = make_buff( this, "prepared", find_spell( 203650, DEMON_HUNTER_HAVOC ) )
    ->set_trigger_spell( talent.momentum )
    ->set_default_value_from_effect_type( A_RESTORE_POWER )
    ->set_affects_regen( true );

  buff.blind_fury = make_buff( this, "blind_fury", spec.eye_beam )
    ->set_default_value( talent.blind_fury->effectN( 3 ).resource( RESOURCE_FURY ) / 50 )
    ->set_cooldown( timespan_t::zero() )
    ->set_period( timespan_t::from_millis( 100 ) ) // Overridden on cast
    ->set_tick_zero( true )
    ->set_tick_callback( [ this ]( buff_t* b, int, timespan_t ) {
      resource_gain( RESOURCE_FURY, b->check_value(), gain.blind_fury );
    } );

  buff.unbound_chaos = make_buff( this, "unbound_chaos", find_spell( 347462 ) )
    ->set_default_value_from_effect( 1 );

  buff.vengeful_retreat_move = new movement_buff_t(this, "vengeful_retreat_movement", spell_data_t::nil() );
  buff.vengeful_retreat_move
    ->set_chance( 1.0 )
    ->set_duration( spec.vengeful_retreat->duration() );

  // Vengeance ==============================================================

  buff.demon_spikes = new buffs::demon_spikes_t(this);

  buff.soul_barrier = make_buff<absorb_buff_t>( this, "soul_barrier", talent.soul_barrier );
  buff.soul_barrier->set_absorb_source( get_stats( "soul_barrier" ) )
    ->set_absorb_gain( get_gain( "soul_barrier" ) )
    ->set_absorb_high_priority( true )  // TOCHECK
    ->set_cooldown( timespan_t::zero() );

  // Covenant ===============================================================

  // Fake Growing Inferno buff for tracking purposes
  buff.growing_inferno = make_buff<buff_t>( this, "growing_inferno", conduit.growing_inferno )
    ->set_default_value( conduit.growing_inferno.percent() )
    // 12/02/2020 - Manual hotfix, not in spell data
    ->set_max_stack( conduit.growing_inferno->ok() ? (int)( 10 / conduit.growing_inferno.percent() ) : 1 )
    ->set_duration( 20_s );

  buff.soul_furance = make_buff<buff_t>( this, "soul_furance", conduit.soul_furnace->effectN( 1 ).trigger() )
    ->set_default_value( conduit.soul_furnace.percent() );

  // Legendary ==============================================================

  const spell_data_t* chaos_theory_buff = legendary.chaos_theory->ok() ? find_spell( 337567 ) : spell_data_t::not_found();
  buff.chaos_theory = make_buff<buff_t>( this, "chaos_theory", chaos_theory_buff )
    ->set_default_value_from_effect( 1 )
    ->set_chance( legendary.chaos_theory->effectN( 1 ).percent() )
    ->set_cooldown( timespan_t::zero() );

  // TOCHECK: 20% proc rate was removed from the primary spell, need to confirm the actual proc rate
  const spell_data_t* fel_bombardment_buff = legendary.fel_bombardment->ok() ? find_spell( 337849 ) : spell_data_t::not_found();
  buff.fel_bombardment = make_buff<buff_t>( this, "fel_bombardment", fel_bombardment_buff );
  if ( legendary.fel_bombardment->ok() )
  {
    buff.fel_bombardment->set_chance( 0.2 );
  }
}

struct metamorphosis_adjusted_cooldown_expr_t : public expr_t
{
  demon_hunter_t* dh;
  double cooldown_multiplier;

  metamorphosis_adjusted_cooldown_expr_t( demon_hunter_t* p, const std::string& name_str )
    : expr_t( name_str ), dh( p ), cooldown_multiplier( 1.0 )
  {
  }

  void calculate_multiplier()
  {
    double reduction_per_second = 0.0;
    cooldown_multiplier = 1.0 / ( 1.0 + reduction_per_second );
  }

  double evaluate() override
  {
    return dh->cooldown.metamorphosis->remains().total_seconds() * cooldown_multiplier;
  }
};

struct eye_beam_adjusted_cooldown_expr_t : public expr_t
{
  demon_hunter_t* dh;
  double cooldown_multiplier;

  eye_beam_adjusted_cooldown_expr_t( demon_hunter_t* p, util::string_view name_str )
    : expr_t( name_str ), dh( p ), cooldown_multiplier( 1.0 )
  {}

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

    return dh->cooldown.eye_beam->remains().total_seconds() * cooldown_multiplier;
  }
};

// demon_hunter_t::create_expression ========================================

std::unique_ptr<expr_t> demon_hunter_t::create_expression( util::string_view name_str )
{
  if ( name_str == "greater_soul_fragments" || name_str == "lesser_soul_fragments" ||
       name_str == "soul_fragments" || name_str == "demon_soul_fragments" )
  {
    struct soul_fragments_expr_t : public expr_t
    {
      demon_hunter_t* dh;
      soul_fragment type;

      soul_fragments_expr_t( demon_hunter_t* p, util::string_view n, soul_fragment t )
        : expr_t( n ), dh( p ), type( t )
      {
      }

      double evaluate() override
      {
        return dh->get_active_soul_fragments( type );
      }
    };

    soul_fragment type = soul_fragment::LESSER;

    if ( name_str == "soul_fragments" )
    {
      type = soul_fragment::ANY;
    }
    else if ( name_str == "greater_soul_fragments" )
    {
      type = soul_fragment::ANY_GREATER;
    }
    else if ( name_str == "demon_soul_fragments" )
    {
      type = soul_fragment::ANY_DEMON;
    }

    return std::make_unique<soul_fragments_expr_t>( this, name_str, type );
  }
  else if ( name_str == "cooldown.metamorphosis.adjusted_remains" )
  {
    return this->cooldown.metamorphosis->create_expression( "remains" );
  }
  else if ( name_str == "cooldown.eye_beam.adjusted_remains" )
  {
    if ( this->talent.cycle_of_hatred->ok() )
    {
      return std::make_unique<eye_beam_adjusted_cooldown_expr_t>( this, name_str );
    }
    else
    {
      return this->cooldown.eye_beam->create_expression( "remains" );
    }
  }

  return player_t::create_expression( name_str );
}

// demon_hunter_t::create_options
// ==================================================

void demon_hunter_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "target_reach", options.target_reach ) );
  add_option( opt_float( "initial_fury", options.initial_fury, 0.0, 120 ) );
  add_option( opt_float( "memory_of_lucid_dreams_proc_chance", options.memory_of_lucid_dreams_proc_chance, 0.0, 1.0 ) );
  add_option( opt_int( "fodder_to_the_flame_kill_seconds", options.fodder_to_the_flame_kill_seconds, 0, 10 ) );
  add_option( opt_float( "razelikhs_defilement_utility_pct", options.razelikhs_defilement_utility_pct, 0.0, 1.0 ) );
}

// demon_hunter_t::create_pet ===============================================

pet_t* demon_hunter_t::create_pet( util::string_view pet_name,
                                   util::string_view /* pet_type */ )
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

  resources.base[ RESOURCE_FURY ] = 100;
  resources.base[ RESOURCE_FURY ] += spec.unrestrained_fury->effectN( 1 ).base_value();
  resources.base[ RESOURCE_FURY ] += spec.unrestrained_fury_2->effectN( 1 ).base_value();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

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
  proc.delayed_aa_range               = get_proc( "delayed_aa_out_of_range" );
  proc.soul_fragment_greater          = get_proc( "soul_fragment_greater" );
  proc.soul_fragment_greater_demon    = get_proc( "soul_fragment_greater_demon" );
  proc.soul_fragment_empowered_demon  = get_proc( "soul_fragment_empowered_demon" );
  proc.soul_fragment_lesser           = get_proc( "soul_fragment_lesser" );
  proc.felblade_reset                 = get_proc( "felblade_reset" );

  // Havoc
  proc.demon_blades_wasted            = get_proc( "demon_blades_wasted" );
  proc.demonic_appetite               = get_proc( "demonic_appetite" );
  proc.demons_bite_in_meta            = get_proc( "demons_bite_in_meta" );
  proc.chaos_strike_in_essence_break  = get_proc( "chaos_strike_in_essence_break" );
  proc.annihilation_in_essence_break  = get_proc( "annihilation_in_essence_break" );
  proc.blade_dance_in_essence_break   = get_proc( "blade_dance_in_essence_break" );
  proc.death_sweep_in_essence_break   = get_proc( "death_sweep_in_essence_break" );

  // Vengeance
  proc.soul_fragment_expire           = get_proc( "soul_fragment_expire" );
  proc.soul_fragment_overflow         = get_proc( "soul_fragment_overflow" );
  proc.soul_fragment_from_shear       = get_proc( "soul_fragment_from_shear" );
  proc.soul_fragment_from_fracture    = get_proc( "soul_fragment_from_fracture" );
  proc.soul_fragment_from_fallout     = get_proc( "soul_fragment_from_fallout" );
  proc.soul_fragment_from_meta        = get_proc( "soul_fragment_from_meta" );

  // Legendary
  proc.darkglare_boon_resets          = get_proc( "darkglare_boon_resets" );
}

// demon_hunter_t::init_resources ===========================================

void demon_hunter_t::init_resources( bool force )
{
  base_t::init_resources( force );

  resources.current[ RESOURCE_FURY ] = options.initial_fury;
  expected_max_health = calculate_expected_max_health();
}

// demon_hunter_t::init_special_effects =====================================

void demon_hunter_t::init_special_effects()
{
  base_t::init_special_effects();

  if ( covenant.fodder_to_the_flame->ok() )
  {
    auto const fodder_to_the_flame_driver = new special_effect_t( this );
    fodder_to_the_flame_driver->name_str = "fodder_to_the_flame_driver";
    fodder_to_the_flame_driver->spell_id = 350570;
    special_effects.push_back( fodder_to_the_flame_driver );

    auto cb = new actions::spells::fodder_to_the_flame_cb_t( this, *fodder_to_the_flame_driver );
    cb->initialize();
  }
}

// demon_hunter_t::init_rng =================================================

void demon_hunter_t::init_rng()
{
  // RPPM objects

  // General
  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    rppm.felblade = get_rppm( "felblade", find_spell( 236167 ) );
    rppm.demonic_appetite = get_rppm( "demonic_appetite", talent.demonic_appetite );
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    rppm.felblade = get_rppm( "felblade", find_spell( 203557 ) );
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

  // General Passives
  spec.demon_hunter           = find_specialization_spell( "Demon Hunter" );
  spec.consume_magic          = find_class_spell( "Consume Magic" );
  spec.chaos_brand            = find_spell( 255260 );
  spec.critical_strikes       = find_spell( 221351 );
  spec.demonic_wards          = find_specialization_spell( "Demonic Wards" );
  spec.demonic_wards_rank_2   = find_rank_spell( "Demonic Wards", "Rank 2" );
  spec.demonic_wards_rank_3   = find_rank_spell( "Demonic Wards", "Rank 3" );
  spec.demon_soul             = find_spell( 163073 );
  spec.empowered_demon_soul   = find_spell( 347765 );
  spec.thick_skin             = find_specialization_spell( "Thick Skin" );
  spec.soul_fragment          = find_spell( 204255 );
  spec.leather_specialization = find_specialization_spell( "Leather Specialization", "Passive" );

  // Shared Abilities
  spec.disrupt                = find_class_spell( "Disrupt" );
  spec.disrupt_rank_2         = find_rank_spell( "Disrupt", "Rank 2" );
  spec.disrupt_rank_3         = find_rank_spell( "Disrupt", "Rank 3" );
  spec.immolation_aura        = find_class_spell( "Immolation Aura" );
  spec.immolation_aura_rank_2 = find_rank_spell( "Immolation Aura", "Rank 2" );
  spec.immolation_aura_rank_3 = find_rank_spell( "Immolation Aura", "Rank 3" );
  spec.throw_glaive           = find_class_spell( "Throw Glaive" );
  spec.throw_glaive_rank_2    = find_rank_spell( "Throw Glaive", "Rank 2" );
  spec.throw_glaive_rank_3    = find_rank_spell( "Throw Glaive", "Rank 3" );
  spec.metamorphosis_rank_2   = find_rank_spell( "Metamorphosis", "Rank 2" );
  spec.metamorphosis_rank_3   = find_rank_spell( "Metamorphosis", "Rank 3" );
  spec.metamorphosis_rank_4   = find_rank_spell( "Metamorphosis", "Rank 4" );

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    spec.consume_soul_greater   = find_spell( 178963 );
    spec.consume_soul_lesser    = spec.consume_soul_greater;
    spec.metamorphosis          = find_class_spell( "Metamorphosis" );
    spec.metamorphosis_buff     = spec.metamorphosis->effectN( 2 ).trigger();
  }
  else
  {
    spec.consume_soul_greater   = find_spell( 210042 );
    spec.consume_soul_lesser    = find_spell( 203794 );
    spec.metamorphosis          = find_specialization_spell( "Metamorphosis" );
    spec.metamorphosis_buff     = spec.metamorphosis;
  }

  // Havoc
  spec.havoc                  = find_specialization_spell( "Havoc Demon Hunter" );
  spec.annihilation           = find_spell( 201427, DEMON_HUNTER_HAVOC );
  spec.blade_dance            = find_class_spell( "Blade Dance", DEMON_HUNTER_HAVOC );
  spec.blade_dance_rank_2     = find_rank_spell( "Blade Dance", "Rank 2" );
  spec.blur                   = find_specialization_spell( "Blur" );
  spec.chaos_nova             = find_class_spell( "Chaos Nova", DEMON_HUNTER_HAVOC );
  spec.chaos_nova_rank_2      = find_rank_spell( "Chaos Nova", "Rank 2" );
  spec.chaos_strike           = find_class_spell( "Chaos Strike", DEMON_HUNTER_HAVOC );
  spec.chaos_strike_rank_2    = find_rank_spell( "Chaos Strike", "Rank 2" );
  spec.chaos_strike_rank_3    = find_rank_spell( "Chaos Strike", "Rank 3" );
  spec.chaos_strike_refund    = find_spell( 197125, DEMON_HUNTER_HAVOC );
  spec.chaos_strike_fury      = find_spell( 193840, DEMON_HUNTER_HAVOC );
  spec.death_sweep            = find_spell( 210152, DEMON_HUNTER_HAVOC );
  spec.demonic_appetite_fury  = find_spell( 210041, DEMON_HUNTER_HAVOC );
  spec.demons_bite            = find_class_spell( "Demon's Bite", DEMON_HUNTER_HAVOC );
  spec.eye_beam               = find_class_spell( "Eye Beam", DEMON_HUNTER_HAVOC );
  spec.eye_beam_rank_2        = find_rank_spell( "Eye Beam", "Rank 2" );
  spec.eye_beam_rank_3        = find_rank_spell( "Eye Beam", "Rank 3" );
  spec.fel_rush               = find_class_spell( "Fel Rush", DEMON_HUNTER_HAVOC );
  spec.fel_rush_rank_2        = find_rank_spell( "Fel Rush", "Rank 2" );
  spec.fel_rush_rank_3        = find_rank_spell( "Fel Rush", "Rank 3" );
  spec.fel_rush_damage        = find_spell( 192611, DEMON_HUNTER_HAVOC );
  spec.momentum_buff          = find_spell( 208628, DEMON_HUNTER_HAVOC );
  spec.unrestrained_fury      = find_rank_spell( "Unrestrained Fury", "Rank 1" );
  spec.unrestrained_fury_2    = find_rank_spell( "Unrestrained Fury", "Rank 2" );
  spec.vengeful_retreat       = find_class_spell( "Vengeful Retreat", DEMON_HUNTER_HAVOC );

  // Vengeance
  spec.vengeance              = find_specialization_spell( "Vengeance Demon Hunter" );
  spec.demon_spikes           = find_specialization_spell( "Demon Spikes" );
  spec.demon_spikes_rank_2    = find_rank_spell( "Demon Spikes", "Rank 2" );
  spec.fel_devastation        = find_specialization_spell( "Fel Devastation" );
  spec.fel_devastation_rank_2 = find_rank_spell( "Fel Devastation", "Rank 2" );
  spec.fiery_brand            = find_specialization_spell( "Fiery Brand" );
  spec.fiery_brand_rank_2     = find_rank_spell( "Fiery Brand", "Rank 2" );
  spec.fiery_brand_rank_3     = find_rank_spell( "Fiery Brand", "Rank 3" );
  spec.fiery_brand_dr         = find_spell( 207744, DEMON_HUNTER_VENGEANCE );
  spec.infernal_strike        = find_specialization_spell( "Infernal Strike" );
  spec.infernal_strike_rank_2 = find_rank_spell( "Infernal Strike", "Rank 2" );
  spec.infernal_strike_rank_3 = find_rank_spell( "Infernal Strike", "Rank 3" );
  spec.riposte                = find_specialization_spell( "Riposte" );
  spec.sigil_of_flame         = find_specialization_spell( "Sigil of Flame" );
  spec.sigil_of_flame_rank_2  = find_rank_spell( "Sigil of Flame", "Rank 2" );
  spec.soul_cleave            = find_specialization_spell( "Soul Cleave" );
  spec.soul_cleave_rank_2     = find_rank_spell( "Soul Cleave", "Rank 2" );
  spec.soul_cleave_rank_3     = find_rank_spell( "Soul Cleave", "Rank 3" );

  // Masteries ==============================================================

  mastery.demonic_presence        = find_mastery_spell( DEMON_HUNTER_HAVOC );
  mastery.demonic_presence_rank_2 = find_rank_spell( "Mastery: Demonic Presence", "Rank 2" );
  mastery.fel_blood               = find_mastery_spell( DEMON_HUNTER_VENGEANCE );
  mastery.fel_blood_rank_2        = find_rank_spell( "Mastery: Fel Blood", "Rank 2" );

  // Talents ================================================================

  // General
  talent.felblade             = find_talent_spell( "Felblade" );
  talent.soul_rending         = find_talent_spell( "Soul Rending" );

  // Havoc
  talent.blind_fury           = find_talent_spell( "Blind Fury" );
  talent.demonic_appetite     = find_talent_spell( "Demonic Appetite" );
  // talent.felblade

  talent.insatiable_hunger    = find_talent_spell( "Insatiable Hunger" );
  talent.burning_hatred       = find_talent_spell( "Burning Hatred" );
  talent.demon_blades         = find_talent_spell( "Demon Blades" );

  talent.trail_of_ruin        = find_talent_spell( "Trail of Ruin" );
  talent.unbound_chaos        = find_talent_spell( "Unbound Chaos" );
  talent.glaive_tempest       = find_talent_spell( "Glaive Tempest" );

  // talent.soul_rending
  talent.desperate_instincts  = find_talent_spell( "Desperate Instincts" );
  talent.netherwalk           = find_talent_spell( "Netherwalk" );

  talent.cycle_of_hatred      = find_talent_spell( "Cycle of Hatred" );
  talent.first_blood          = find_talent_spell( "First Blood" );
  talent.essence_break        = find_talent_spell( "Essence Break" );

  talent.unleashed_power      = find_talent_spell( "Unleashed Power" );
  talent.master_of_the_glaive = find_talent_spell( "Master of the Glaive" );
  talent.fel_eruption         = find_talent_spell( "Fel Eruption" );

  talent.demonic              = find_talent_spell( "Demonic" );
  talent.momentum             = find_talent_spell( "Momentum" );
  talent.fel_barrage          = find_talent_spell( "Fel Barrage" );

  // Vengeance
  talent.abyssal_strike       = find_talent_spell( "Abyssal Strike" );
  talent.agonizing_flames     = find_talent_spell( "Agonizing Flames" );

  talent.feast_of_souls       = find_talent_spell( "Feast of Souls" );
  talent.fallout              = find_talent_spell( "Fallout" );
  talent.burning_alive        = find_talent_spell( "Burning Alive" );

  talent.infernal_armor       = find_talent_spell( "Infernal Armor" );
  talent.charred_flesh        = find_talent_spell( "Charred Flesh" );
  talent.spirit_bomb          = find_talent_spell( "Spirit Bomb" );

  talent.feed_the_demon       = find_talent_spell( "Feed the Demon" );
  talent.fracture             = find_talent_spell( "Fracture" );

  talent.concentrated_sigils  = find_talent_spell( "Concentrated Sigils" );
  talent.quickened_sigils     = find_talent_spell( "Quickened Sigils" );
  talent.sigil_of_chains      = find_talent_spell( "Sigil of Chains" );

  talent.void_reaver          = find_talent_spell( "Void Reaver" );
  talent.soul_barrier         = find_talent_spell( "Soul Barrier" );

  talent.last_resort          = find_talent_spell( "Last Resort" );
  talent.ruinous_bulwark      = find_talent_spell( "Ruinous Bulwark" );
  talent.bulk_extraction      = find_talent_spell( "Bulk Extraction" );

  // PvP Talents
  talent.demonic_origins      = find_spell( 235893, DEMON_HUNTER_HAVOC );

  // Covenant Abilities =====================================================

  covenant.elysian_decree       = find_covenant_spell( "Elysian Decree" );
  covenant.fodder_to_the_flame  = find_covenant_spell( "Fodder to the Flame" );
  covenant.sinful_brand         = find_covenant_spell( "Sinful Brand" );
  covenant.the_hunt             = find_covenant_spell( "The Hunt" );

  // Conduits ===============================================================

  conduit.dancing_with_fate     = find_conduit_spell( "Dancing with Fate" );
  conduit.growing_inferno       = find_conduit_spell( "Growing Inferno" );
  conduit.relentless_onslaught  = find_conduit_spell( "Relentless Onslaught" );
  conduit.serrated_glaive       = find_conduit_spell( "Serrated Glaive" );

  conduit.fel_defender          = find_conduit_spell( "Fel Defender" );
  conduit.soul_furnace          = find_conduit_spell( "Soul Furnace" );

  conduit.repeat_decree         = find_conduit_spell( "Repeat Decree" );
  conduit.increased_scrutiny    = find_conduit_spell( "Increased Scrutiny" );
  conduit.brooding_pool         = find_conduit_spell( "Brooding Pool" );
  conduit.unnatural_malice      = find_conduit_spell( "Unnatural Malice" );

  // Legendary Items ========================================================

  legendary.darkglare_boon                = find_runeforge_legendary( "Darkglare Medallion" ); // Note: Blizzard typo in item runeforge data
  legendary.darkest_hour                  = find_runeforge_legendary( "Darkest Hour" );
  legendary.collective_anguish            = find_runeforge_legendary( "Collective Anguish" );
  legendary.fel_bombardment               = find_runeforge_legendary( "Fel Bombardment" );

  legendary.burning_wound                 = find_runeforge_legendary( "Burning Wound" );
  legendary.chaos_theory                  = find_runeforge_legendary( "Chaos Theory" );
  legendary.darker_nature                 = find_runeforge_legendary( "Darker Nature" );
  legendary.erratic_fel_core              = find_runeforge_legendary( "Erratic Fel Core" );

  legendary.fiery_soul                    = find_runeforge_legendary( "Fiery Soul" );
  legendary.razelikhs_defilement          = find_runeforge_legendary( "Razelikh's Defilement" );
  legendary.fel_flame_fortification       = find_runeforge_legendary( "Fel Flame Fortification" );
  legendary.spirit_of_the_darkness_flame  = find_runeforge_legendary( "Spirit of the Darkness Flame" );

  spec.darkglare_boon_refund = ( legendary.darkglare_boon->ok() ) ? find_spell( 350726 ) : spell_data_t::not_found();

  // Spell Initialization ===================================================

  using namespace actions::attacks;
  using namespace actions::spells;
  using namespace actions::heals;

  active.consume_soul_greater = new consume_soul_t( this, "consume_soul_greater", spec.consume_soul_greater, soul_fragment::GREATER );
  active.consume_soul_lesser = new consume_soul_t( this, "consume_soul_lesser", spec.consume_soul_lesser, soul_fragment::LESSER );

  if ( talent.demon_blades->ok() )
  {
    active.demon_blades = new demon_blades_t( this );
  }

  if ( legendary.collective_anguish.ok() )
  {
    const spell_data_t* driver = ( specialization() == DEMON_HUNTER_HAVOC ) ? find_spell( 333105 ) : find_spell( 333386 );
    active.collective_anguish = get_background_action<collective_anguish_t>( "collective_anguish", driver );
  }

  if ( conduit.relentless_onslaught.ok() && specialization() == DEMON_HUNTER_HAVOC )
  {
    active.relentless_onslaught = get_background_action<chaos_strike_t>( "chaos_strike_onslaught", "", true );
    active.relentless_onslaught_annihilation = get_background_action<annihilation_t>( "annihilation_onslaught", "", true );
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
      {
        invalidate_cache( CACHE_RUN_SPEED );
      }
      if ( mastery.fel_blood->ok() )
      {
        invalidate_cache( CACHE_ARMOR );
        invalidate_cache( CACHE_ATTACK_POWER );
      }
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
    case DEMON_HUNTER_VENGEANCE:
      return RESOURCE_FURY;
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
  return ( true_level >= 51 ) ? "spectral_flask_of_power" :
         ( true_level >= 40 ) ? "greater_flask_of_the_currents" :
         ( true_level >= 35 ) ? "greater_draenic_agility_flask" :
         "disabled";
}

// demon_hunter_t::default_potion ==================================================

std::string demon_hunter_t::default_potion() const
{
  return ( true_level >= 51 ) ? "phantom_fire" :
         ( true_level >= 40 ) ? "potion_of_unbridled_fury" :
         ( true_level >= 35 ) ? "draenic_agility" :
         "disabled";
}

// demon_hunter_t::default_food ====================================================

std::string demon_hunter_t::default_food() const
{
  return ( true_level >= 51 ) ? "feast_of_gluttonous_hedonism" :
         ( true_level >= 45 ) ? "famine_evaluator_and_snack_table" :
         ( true_level >= 40 ) ? "lavish_suramar_feast" :
         "disabled";
}

// demon_hunter_t::default_rune ====================================================

std::string demon_hunter_t::default_rune() const
{
  return ( true_level >= 60 ) ? "veiled" :
         ( true_level >= 50 ) ? "battle_scarred" :
         ( true_level >= 45 ) ? "defiled" :
         ( true_level >= 40 ) ? "hyper" :
         "disabled";
}

// demon_hunter_t::default_temporary_enchant =======================================

std::string demon_hunter_t::default_temporary_enchant() const
{
  return ( true_level >= 60 ) ? "main_hand:shaded_sharpening_stone/off_hand:shaded_sharpening_stone" :
         "disabled";
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
  pre->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  if ( specialization() == DEMON_HUNTER_HAVOC )
  {
    pre->add_action( "variable,name=trinket_sync_slot,value=1,if=trinket.1.has_stat.any_dps&(!trinket.2.has_stat.any_dps|trinket.1.cooldown.duration>=trinket.2.cooldown.duration)" );
    pre->add_action( "variable,name=trinket_sync_slot,value=2,if=trinket.2.has_stat.any_dps&(!trinket.1.has_stat.any_dps|trinket.2.cooldown.duration>trinket.1.cooldown.duration)" );
    pre->add_action( "variable,name=use_eye_beam_fury_condition,value=talent.blind_fury.enabled&(runeforge.darkglare_medallion|talent.demon_blades.enabled)", "Use Eye Beam at low Fury with Blind Fury combined with either Darkglare or Demon Blades" );
  }
}

// demon_hunter_t::apl_default ==============================================

void demon_hunter_t::apl_default()
{
  // action_priority_list_t* def = get_action_priority_list( "default" );
}

// demon_hunter_t::apl_havoc ================================================

void add_havoc_use_items( demon_hunter_t*, action_priority_list_t* apl )
{
  apl->add_action( "use_items,slots=trinket1,if=variable.trinket_sync_slot=1&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.1.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=2&!trinket.2.cooldown.ready)|!variable.trinket_sync_slot", "Default use item logic" );
  apl->add_action( "use_items,slots=trinket2,if=variable.trinket_sync_slot=2&(buff.metamorphosis.up|(!talent.demonic.enabled&cooldown.metamorphosis.remains>(fight_remains>?trinket.2.cooldown.duration%2))|fight_remains<=20)|(variable.trinket_sync_slot=1&!trinket.1.cooldown.ready)|!variable.trinket_sync_slot" );
}

void demon_hunter_t::apl_havoc()
{
  action_priority_list_t* apl_default = get_action_priority_list( "default" );
  apl_default->add_action( "auto_attack" );
  apl_default->add_action( "retarget_auto_attack,line_cd=1,target_if=min:debuff.burning_wound.remains,if=runeforge.burning_wound&talent.demon_blades.enabled" );
  apl_default->add_action( "variable,name=blade_dance,if=!runeforge.chaos_theory&!runeforge.darkglare_medallion,value=talent.first_blood.enabled|spell_targets.blade_dance1>=(3-talent.trail_of_ruin.enabled)", "Without Chaos Theory or Darkglare, Blade Dance with First Blood or at 3+ (2+ with Trail of Ruin) targets" );
  apl_default->add_action( "variable,name=blade_dance,if=runeforge.chaos_theory,value=buff.chaos_theory.down|talent.first_blood.enabled&spell_targets.blade_dance1>=(2-talent.trail_of_ruin.enabled)|!talent.cycle_of_hatred.enabled&spell_targets.blade_dance1>=(4-talent.trail_of_ruin.enabled)", "With Chaos Theory, Blade Dance when the buff is down, with First Blood at 2+ (1+ with Trail of Ruin) or with Essence Break at 4+ (3+ with Trail of Ruin) targets" );
  apl_default->add_action( "variable,name=blade_dance,if=runeforge.darkglare_medallion,value=talent.first_blood.enabled|(buff.metamorphosis.up|talent.trail_of_ruin.enabled|debuff.essence_break.up)&spell_targets.blade_dance1>=(3-talent.trail_of_ruin.enabled)|!talent.demonic.enabled&spell_targets.blade_dance1>=4", "With Darkglare, Blade Dance at normal target count when buffed by a secondary effect, or always at 4T+ for non-Demonic" );
  apl_default->add_action( "variable,name=blade_dance,op=reset,if=talent.essence_break.enabled&cooldown.essence_break.ready", "Use Essence Break before Blade Dance if it is available and off cooldown" );
  apl_default->add_action( "variable,name=pooling_for_meta,value=!talent.demonic.enabled&cooldown.metamorphosis.remains<6&fury.deficit>30" );
  apl_default->add_action( "variable,name=pooling_for_blade_dance,value=variable.blade_dance&(fury<75-talent.first_blood.enabled*20)" );
  apl_default->add_action( "variable,name=pooling_for_eye_beam,value=talent.demonic.enabled&!talent.blind_fury.enabled&cooldown.eye_beam.remains<(gcd.max*2)&fury.deficit>20" );
  apl_default->add_action( "variable,name=waiting_for_momentum,value=talent.momentum.enabled&!buff.momentum.up" );
  apl_default->add_action( this, "Disrupt" );
  apl_default->add_action( "call_action_list,name=cooldown,if=gcd.remains=0" );
  apl_default->add_action( "pick_up_fragment,type=demon,if=demon_soul_fragments>0" );
  apl_default->add_action( "pick_up_fragment,if=fury.deficit>=35" );
  apl_default->add_action( this, "Throw Glaive", "if=buff.fel_bombardment.stack=5&(buff.immolation_aura.up|!buff.metamorphosis.up)" );
  apl_default->add_action( "run_action_list,name=demonic,if=talent.demonic.enabled" );
  apl_default->add_action( "run_action_list,name=normal" );

  action_priority_list_t* apl_cooldown = get_action_priority_list( "cooldown" );
  apl_cooldown->add_action( this, "Metamorphosis", "if=!talent.demonic.enabled&cooldown.eye_beam.remains>20&(!covenant.venthyr.enabled|!dot.sinful_brand.ticking)|fight_remains<25" );
  apl_cooldown->add_action( this, "Metamorphosis", "if=talent.demonic.enabled&(cooldown.eye_beam.remains>20&(!variable.blade_dance|cooldown.blade_dance.remains>gcd.max))&(!covenant.venthyr.enabled|!dot.sinful_brand.ticking)|fight_remains<25" );
  apl_cooldown->add_action( "potion,if=buff.metamorphosis.remains>25|fight_remains<60" );
  add_havoc_use_items( this, apl_cooldown );
  apl_cooldown->add_action( "sinful_brand,if=!dot.sinful_brand.ticking" );
  apl_cooldown->add_action( "the_hunt,if=!talent.demonic.enabled&!variable.waiting_for_momentum&!variable.pooling_for_meta|buff.furious_gaze.up" );
  apl_cooldown->add_action( "elysian_decree,if=(active_enemies>desired_targets|raid_event.adds.in>30)" );

  action_priority_list_t* apl_normal = get_action_priority_list( "normal" );
  apl_normal->add_action( this, "Vengeful Retreat", "if=talent.momentum.enabled&buff.prepared.down&time>1" );
  apl_normal->add_action( this, "Fel Rush", "if=(buff.unbound_chaos.up|variable.waiting_for_momentum&(!talent.unbound_chaos.enabled|!cooldown.immolation_aura.ready))&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  apl_normal->add_talent( this, "Fel Barrage", "if=active_enemies>desired_targets|raid_event.adds.in>30" );
  apl_normal->add_action( this, spec.death_sweep, "death_sweep", "if=variable.blade_dance" );
  apl_normal->add_action( this, "Immolation Aura" );
  apl_normal->add_talent( this, "Glaive Tempest", "if=!variable.waiting_for_momentum&(active_enemies>desired_targets|raid_event.adds.in>10)" );
  apl_normal->add_action( this, "Throw Glaive", "if=conduit.serrated_glaive.enabled&cooldown.eye_beam.remains<6&!buff.metamorphosis.up&!debuff.exposed_wound.up" );
  apl_normal->add_action( this, "Eye Beam", "if=!variable.waiting_for_momentum&(active_enemies>desired_targets|raid_event.adds.in>15&(!variable.use_eye_beam_fury_condition|spell_targets>1|fury<70))" );
  apl_normal->add_action( this, "Blade Dance", "if=variable.blade_dance" );
  apl_normal->add_talent( this, "Felblade", "if=fury.deficit>=40" );
  apl_normal->add_talent( this, "Essence Break" );
  apl_normal->add_action( this, spec.annihilation, "annihilation", "if=(talent.demon_blades.enabled|!variable.waiting_for_momentum|fury.deficit<30|buff.metamorphosis.remains<5)&!variable.pooling_for_blade_dance" );
  apl_normal->add_action( this, "Chaos Strike", "if=(talent.demon_blades.enabled|!variable.waiting_for_momentum|fury.deficit<30)&!variable.pooling_for_meta&!variable.pooling_for_blade_dance" );
  apl_normal->add_action( this, "Eye Beam", "if=talent.blind_fury.enabled&raid_event.adds.in>cooldown" );
  apl_normal->add_action( this, "Demon's Bite", "target_if=min:debuff.burning_wound.remains,if=runeforge.burning_wound&debuff.burning_wound.remains<4" );
  apl_normal->add_action( this, "Demon's Bite" );
  apl_normal->add_action( this, "Fel Rush", "if=!talent.momentum.enabled&raid_event.movement.in>charges*10&talent.demon_blades.enabled" );
  apl_normal->add_talent( this, "Felblade", "if=movement.distance>15|buff.out_of_range.up" );
  apl_normal->add_action( this, "Fel Rush", "if=movement.distance>15|(buff.out_of_range.up&!talent.momentum.enabled)" );
  apl_normal->add_action( this, "Vengeful Retreat", "if=movement.distance>15" );
  apl_normal->add_action( this, "Throw Glaive", "if=talent.demon_blades.enabled" );

  action_priority_list_t* apl_demonic = get_action_priority_list( "demonic" );
  apl_demonic->add_action( this, "Fel Rush", "if=talent.unbound_chaos.enabled&buff.unbound_chaos.up&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  apl_demonic->add_action( this, spec.death_sweep, "death_sweep", "if=variable.blade_dance" );
  apl_demonic->add_talent( this, "Glaive Tempest", "if=active_enemies>desired_targets|raid_event.adds.in>10" );
  apl_demonic->add_action( this, "Throw Glaive", "if=conduit.serrated_glaive.enabled&cooldown.eye_beam.remains<6&!buff.metamorphosis.up&!debuff.exposed_wound.up" );
  apl_demonic->add_action( this, "Eye Beam", "if=active_enemies>desired_targets|raid_event.adds.in>25&(!variable.use_eye_beam_fury_condition|spell_targets>1|fury<70)" );
  apl_demonic->add_action( this, "Blade Dance", "if=variable.blade_dance&!cooldown.metamorphosis.ready"
                                                "&(cooldown.eye_beam.remains>5|(raid_event.adds.in>cooldown&raid_event.adds.in<25))" );
  apl_demonic->add_action( this, "Immolation Aura" );
  apl_demonic->add_action( this, spec.annihilation, "annihilation", "if=!variable.pooling_for_blade_dance" );
  apl_demonic->add_talent( this, "Felblade", "if=fury.deficit>=40" );
  apl_demonic->add_talent( this, "Essence Break" );
  apl_demonic->add_action( this, "Chaos Strike", "if=!variable.pooling_for_blade_dance&!variable.pooling_for_eye_beam" );
  apl_demonic->add_action( this, "Fel Rush", "if=talent.demon_blades.enabled&!cooldown.eye_beam.ready&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  apl_demonic->add_action( this, "Demon's Bite", "target_if=min:debuff.burning_wound.remains,if=runeforge.burning_wound&debuff.burning_wound.remains<4" );
  apl_demonic->add_action( this, "Fel Rush", "if=!talent.demon_blades.enabled&spell_targets>1&(charges=2|(raid_event.movement.in>10&raid_event.adds.in>10))" );
  apl_demonic->add_action( this, "Demon's Bite" );
  apl_demonic->add_action( this, "Throw Glaive", "if=buff.out_of_range.up" );
  apl_demonic->add_action( this, "Fel Rush", "if=movement.distance>15|buff.out_of_range.up" );
  apl_demonic->add_action( this, "Vengeful Retreat", "if=movement.distance>15" );
  apl_demonic->add_action( this, "Throw Glaive", "if=talent.demon_blades.enabled" );
}

// demon_hunter_t::apl_vengeance ============================================

void demon_hunter_t::apl_vengeance()
{
  action_priority_list_t* apl_default = get_action_priority_list( "default" );

  apl_default->add_action( "auto_attack" );
  apl_default->add_action( "variable,name=brand_build,value=talent.agonizing_flames.enabled&talent.burning_alive.enabled&talent.charred_flesh.enabled" );
  apl_default->add_action( this, "Disrupt" );
  // Only triggers if there is something to steal
  apl_default->add_action( this, "Consume Magic" );
  apl_default->add_action( this, "Throw Glaive", "if=buff.fel_bombardment.stack=5&(buff.immolation_aura.up|!buff.metamorphosis.up)" );
  apl_default->add_action( "call_action_list,name=brand,if=variable.brand_build" );
  apl_default->add_action( "call_action_list,name=defensives" );
  apl_default->add_action( "call_action_list,name=cooldowns" );
  apl_default->add_action( "call_action_list,name=normal" );

  action_priority_list_t* apl_defensives = get_action_priority_list( "defensives", "Defensives" );
  apl_defensives->add_action( this, "Demon Spikes" );
  apl_defensives->add_action( this, "Metamorphosis", "if=!buff.metamorphosis.up&(!covenant.venthyr.enabled|!dot.sinful_brand.ticking)|target.time_to_die<15" );
  apl_defensives->add_action( this, "Fiery Brand" );

  action_priority_list_t* cooldowns = get_action_priority_list( "cooldowns" );
  cooldowns->add_action( "potion" );
  cooldowns->add_action( "use_items", "Default fallback for usable items." );
  cooldowns->add_action( "sinful_brand,if=!dot.sinful_brand.ticking" );
  cooldowns->add_action( "the_hunt" );
  cooldowns->add_action( "elysian_decree" );

  action_priority_list_t* apl_brand = get_action_priority_list( "brand", "Fiery Brand Rotation" );
  apl_brand->add_action( this, "Fiery Brand" );
  apl_brand->add_action( this, "Immolation Aura", "if=dot.fiery_brand.ticking" );

  action_priority_list_t* apl_normal = get_action_priority_list( "normal", "Normal Rotation" );
  apl_normal->add_action( this, "Infernal Strike" );
  apl_normal->add_talent( this, "Bulk Extraction" );
  apl_normal->add_talent( this, "Spirit Bomb", "if=((buff.metamorphosis.up&talent.fracture.enabled&soul_fragments>=3)|soul_fragments>=4)" );
  apl_normal->add_action( this, "Fel Devastation" );
  apl_normal->add_action( this, "Soul Cleave", "if=((talent.spirit_bomb.enabled&soul_fragments=0)|!talent.spirit_bomb.enabled)&((talent.fracture.enabled&fury>=55)|(!talent.fracture.enabled&fury>=70)|cooldown.fel_devastation.remains>target.time_to_die|(buff.metamorphosis.up&((talent.fracture.enabled&fury>=35)|(!talent.fracture.enabled&fury>=50))))" );
  apl_normal->add_action( this, "Immolation Aura", "if=((variable.brand_build&cooldown.fiery_brand.remains>10)|!variable.brand_build)&fury<=90" );
  apl_normal->add_talent( this, "Felblade", "if=fury<=60" );
  apl_normal->add_talent( this, "Fracture", "if=((talent.spirit_bomb.enabled&soul_fragments<=3)|(!talent.spirit_bomb.enabled&((buff.metamorphosis.up&fury<=55)|(buff.metamorphosis.down&fury<=70))))" );
  apl_normal->add_action( this, "Sigil of Flame", "if=!(covenant.kyrian.enabled&runeforge.razelikhs_defilement)" );
  apl_normal->add_action( this, "Shear" );
  apl_normal->add_action( this, "Throw Glaive" );
}

// demon_hunter_t::create_cooldowns =========================================

void demon_hunter_t::create_cooldowns()
{
  // General
  cooldown.consume_magic        = get_cooldown( "consume_magic" );
  cooldown.disrupt              = get_cooldown( "disrupt" );
  cooldown.elysian_decree       = get_cooldown( "elysian_decree" );
  cooldown.felblade             = get_cooldown( "felblade" );
  cooldown.fel_eruption         = get_cooldown( "fel_eruption" );

  // Havoc
  cooldown.blade_dance          = get_cooldown( "blade_dance" );
  cooldown.blur                 = get_cooldown( "blur" );
  cooldown.chaos_nova           = get_cooldown( "chaos_nova" );
  cooldown.essence_break        = get_cooldown( "essence_break" );
  cooldown.eye_beam             = get_cooldown( "eye_beam" );
  cooldown.fel_barrage          = get_cooldown( "fel_barrage" );
  cooldown.fel_rush             = get_cooldown( "fel_rush" );
  cooldown.metamorphosis        = get_cooldown( "metamorphosis" );
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

  // Legendary
  gain.darkglare_boon_refund    = get_gain( "darkglare_boon_refund" );
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

// demon_hunter_t::composite_base_armor_multiplier ==========================

double demon_hunter_t::composite_base_armor_multiplier() const
{
  double am = player_t::composite_base_armor_multiplier();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    am *= 1.0 + spec.thick_skin->effectN( 2 ).percent();

    if ( buff.metamorphosis->check() )
    {
      am *= 1.0 + spec.metamorphosis_buff->effectN( 7 ).percent();
    }
  }

  return am;
}

// demon_hunter_t::composite_armor_multiplier ===============================

double demon_hunter_t::composite_armor_multiplier() const
{
  double am = player_t::composite_armor_multiplier();

  if ( specialization() == DEMON_HUNTER_VENGEANCE )
  {
    if ( buff.immolation_aura->check() )
    {
      am *= 1.0 + talent.infernal_armor->effectN( 1 ).percent();
    }
  }

  return am;
}

// demon_hunter_t::composite_attack_power_multiplier ========================

double demon_hunter_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  ap *= 1.0 + cache.mastery() * mastery.fel_blood_rank_2->effectN( 1 ).mastery_value();

  return ap;
}

// demon_hunter_t::composite_attribute_multiplier ===========================

double demon_hunter_t::composite_attribute_multiplier( attribute_e a ) const
{
  double am = player_t::composite_attribute_multiplier( a );

  switch ( a )
  {
    case ATTR_STAMINA:
      am *= 1.0 + spec.thick_skin->effectN( 1 ).percent();
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

  ca += spec.thick_skin->effectN( 4 ).percent();

  return ca;
}

// demon_hunter_t::composite_dodge ==========================================

double demon_hunter_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

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

  mh /= 1.0 + buff.eye_beam_rank_3->check_value();

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

  sh /= 1.0 + buff.eye_beam_rank_3->check_value();

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

  me += spec.thick_skin->effectN( 3 ).percent();

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
    ms += cache.mastery() * mastery.demonic_presence_rank_2->effectN( 1 ).mastery_value();
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

    const random_prop_data_t item_data = dbc->random_property(item.item_level());
    int index = item_database::random_suffix_type(item.parsed.data);
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
  expected_health *= 1 + spec.thick_skin->effectN( 1 ).percent();
  expected_health *= current.health_per_stamina;
  return expected_health;
}

// demon_hunter_t::assess_damage ============================================

void demon_hunter_t::assess_damage( school_e school, result_amount_type dt, action_state_t* s )
{
  player_t::assess_damage( school, dt, s );

  // Benefit tracking
  if ( s->action->may_parry )
  {
    buff.demon_spikes->up();
  }

  if ( active.infernal_armor && buff.immolation_aura->check() && s->action->player->is_enemy() &&
       dbc::is_school( school, SCHOOL_PHYSICAL ) && dt == result_amount_type::DMG_DIRECT && s->action->result_is_hit( s->result ) )
  {
    active.infernal_armor->set_target( s->action->player );
    active.infernal_armor->execute();
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

  // Momentum triggers periodicy-based Restore Power
  if ( talent.momentum->ok() )
  {
    resource_regeneration = regen_type::DYNAMIC;
  }
}

// demon_hunter_t::interrupt ================================================

void demon_hunter_t::interrupt()
{
  event_t::cancel( soul_fragment_pick_up );
  base_t::interrupt();
}

// demon_hunter_t::regen ====================================================

void demon_hunter_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buff.prepared->up() )
  {
    resource_gain( RESOURCE_FURY, buff.prepared->check_value() * periodicity.total_seconds(), gain.prepared );
  }
}

// demon_hunter_t::resource_gain ============================================

double demon_hunter_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  // Memory of Lucid Dreams
  if ( ( resource_type == RESOURCE_FURY ) &&
       buffs.memory_of_lucid_dreams->up() )
  {
    amount *= 1.0 + buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();
  }

  return player_t::resource_gain( resource_type, amount, source, action );
}

// demon_hunter_t::recalculate_resource_max =================================

void demon_hunter_t::recalculate_resource_max( resource_e r, gain_t* source )
{
  player_t::recalculate_resource_max( r, source );

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

void demon_hunter_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
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
      s->result_amount *= 1.0 + spec.demonic_wards->effectN( 1 ).percent()
        + spec.demonic_wards_rank_2->effectN( 1 ).percent();
    }
  }
  else // DEMON_HUNTER_VENGEANCE
  {
    s->result_amount *= 1.0 + spec.demonic_wards->effectN( 1 ).percent()
      + spec.demonic_wards_rank_2->effectN( 1 ).percent()
      + spec.demonic_wards_rank_3->effectN( 1 ).percent();

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
    assert( !buff.out_of_range->expiration.empty() );

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

unsigned demon_hunter_t::get_active_soul_fragments( soul_fragment type_mask ) const
{
    return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                            [ &type_mask ]( unsigned acc, soul_fragment_t* frag ) {
                              return acc + ( frag->is_type( type_mask ) && frag->active() );
                            } );
}

// demon_hunter_t::get_total_soul_fragments =================================

unsigned demon_hunter_t::get_total_soul_fragments( soul_fragment type_mask ) const
{
  if ( type_mask == soul_fragment::ANY )
  {
    return (unsigned)soul_fragments.size();
  }

  return std::accumulate( soul_fragments.begin(), soul_fragments.end(), 0,
                          [ &type_mask ]( unsigned acc, soul_fragment_t* frag ) {
                            return acc + frag->is_type( type_mask );
                          } );
}

// demon_hunter_t::activate_soul_fragment ===================================

void demon_hunter_t::activate_soul_fragment( soul_fragment_t* frag )
{
  // If we spawn a fragment with this flag, instantly consume it
  if ( frag->consume_on_activation )
  {
    frag->consume( true, true );
    return;
  }

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

void demon_hunter_t::spawn_soul_fragment( soul_fragment type, unsigned n, bool consume_on_activation )
{
  if ( type == soul_fragment::GREATER && sim->target->race == RACE_DEMON )
  {
    type = soul_fragment::GREATER_DEMON;
  }

  proc_t* soul_proc;
  switch ( type )
  {
    case soul_fragment::GREATER:          soul_proc = proc.soul_fragment_greater; break;
    case soul_fragment::GREATER_DEMON:    soul_proc = proc.soul_fragment_greater_demon; break;
    case soul_fragment::EMPOWERED_DEMON:  soul_proc = proc.soul_fragment_empowered_demon; break;
    default:                              soul_proc = proc.soul_fragment_lesser;
  }

  for ( unsigned i = 0; i < n; i++ )
  {
    soul_fragments.push_back( new soul_fragment_t( this, type, consume_on_activation ) );
    soul_proc->occur();
  }

  if ( sim->debug )
  {
    sim->out_debug.printf( "%s creates %u %ss. active=%u total=%u", name(), n, get_soul_fragment_str( type ),
                           get_active_soul_fragments( type ), get_total_soul_fragments( type ) );
  }
}

void demon_hunter_t::spawn_soul_fragment( soul_fragment type, unsigned n, player_t* target, bool consume_on_activation )
{
  if ( type == soul_fragment::GREATER && target->race == RACE_DEMON )
  {
    type = soul_fragment::GREATER_DEMON;
  }

  demon_hunter_t::spawn_soul_fragment( type, n, consume_on_activation );
}

// demon_hunter_t::trigger_demonic ==========================================

void demon_hunter_t::trigger_demonic()
{
  if ( !talent.demonic->ok() )
    return;

  debug_cast<buffs::metamorphosis_buff_t*>( buff.metamorphosis )->trigger_demonic();
}

// demon_hunter_sigil_t::create_sigil_expression ==================================

std::unique_ptr<expr_t> actions::demon_hunter_sigil_t::create_sigil_expression( util::string_view name )
{
  if ( util::str_compare_ci( name, "activation_time" ) || util::str_compare_ci( name, "delay" ) )
  {
    return make_ref_expr( name, sigil_delay );
  }
  else if ( util::str_compare_ci( name, "sigil_placed" ) || util::str_compare_ci( name, "placed" ) )
  {
    return make_fn_expr( name, [ this ] {
      if ( p()->sim->current_time() < sigil_activates )
        return 1;
      else
        return 0;
    });
  }

  return {};
}

const demon_hunter_td_t* demon_hunter_t::find_target_data( const player_t* target ) const
{
  return _target_data[ target ];
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

void demon_hunter_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  action.apply_affecting_aura( spec.demon_hunter );
  action.apply_affecting_aura( spec.havoc );
  action.apply_affecting_aura( spec.vengeance );
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
        name_str = report_decorators::decorated_action(*a);
      }
      else
      {
        name_str = util::encode_html( name_str );
      }

      std::string row_class_str;
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
    if ( !p.cd_waste_exec.empty() )
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

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
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
    /*
    hotfix::register_effect( "Demon Hunter", "2020-10-29", "Prepatch Unbound Chaos Damage", 728000 )
      .field( "ap_coefficient" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 2.75 )
      .verification_value( 2.3375 );
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

const module_t* module_t::demon_hunter()
{
  static demon_hunter_module_t m;
  return &m;
}
