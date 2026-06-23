// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "action/action_callback.hpp"
#include "action/parse_effects.hpp"
#include "action/residual_action.hpp"
#include "buff/buff.hpp"
#include "dbc/data_enums.hh"
#include "dbc/spell_data.hpp"
#include "player/pet_spawner.hpp"
#include "player/player.hpp"
#include "sc_enums.hpp"
#include "sc_stagger.hpp"
#include "sim/proc.hpp"
#include "util/timeline.hpp"

#include <array>
#include <memory>
#include <queue>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "simulationcraft.hpp"

namespace monk
{
struct monk_t;
struct monk_td_t;

namespace pets
{
struct monk_pet_t : public pet_t
{
  monk_pet_t( monk_t *owner, std::string_view name, pet_e pet_type, bool guardian, bool dynamic );
  monk_t *o();
  const monk_t *o() const;
  void init_assessors() override;
};
struct xuen_pet_t;
namespace niuzao
{
struct niuzao_pet_t : public monk_pet_t
{
  action_t *stomp;
  niuzao_pet_t( std::string_view name, monk_t *player );
  void init_action_list() override;
  action_t *create_action( std::string_view name, std::string_view options_str ) override;
  void init_spells() override;
};
}  // namespace niuzao
}  // namespace pets

namespace actions
{
template <class Base>
struct monk_action_t : public parse_action_effects_t<Base>
{
  bool ww_mastery;
  bool may_combo_strike;
  bool cast_during_sck;
  bool track_cd_waste;
  std::vector<player_effect_t> persistent_multiplier_effects;

private:
  std::array<resource_e, MONK_MISTWEAVER + 1> _resource_by_stance;

public:
  using base_t = parse_action_effects_t<Base>;

  template <typename... Args>
  monk_action_t( Args &&...args );
  std::string full_name() const;
  monk_t *p();
  const monk_t *p() const;
  monk_td_t *get_td( player_t *target ) const;
  const monk_td_t *find_td( player_t *target ) const;
  void apply_buff_effects();
  void apply_debuff_effects();

  template <typename... Ts>
  void parse_effects( Ts &&...args )
  {
    base_t::parse_effects( std::forward<Ts>( args )... );
  }
  template <typename... Ts>
  void parse_target_effects( Ts &&...args )
  {
    base_t::parse_target_effects( std::forward<Ts>( args )... );
  }

  std::unique_ptr<expr_t> create_expression( std::string_view name_str ) override;

  bool usable_moving() const override;
  bool ready() override;
  void init() override;
  void init_finished() override;
  void reset_swing();
  resource_e current_resource() const override;
  bool is_combo_strike();
  bool is_combo_break();
  void combo_strikes_trigger();
  void consume_resource() override;
  void execute() override;
  void impact( action_state_t *state ) override;
  void trigger_mystic_touch( action_state_t *state );

  double composite_persistent_multiplier( const action_state_t *state ) const override;
  size_t total_effects_count() const override;
  void print_parsed_custom_type( report::sc_html_stream &os ) const override;
};

struct monk_spell_t : public monk_action_t<spell_t>
{
  using base_t = monk_action_t<spell_t>;
  monk_spell_t( monk_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil() );
};

struct monk_heal_t : public monk_action_t<heal_t>
{
  using base_t = monk_action_t<heal_t>;
  monk_heal_t( monk_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil() );
};

struct monk_absorb_t : public monk_action_t<absorb_t>
{
  using base_t = monk_action_t<absorb_t>;
  monk_absorb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil() );
};

struct monk_melee_attack_t : public monk_action_t<melee_attack_t>
{
  using base_t = monk_action_t<melee_attack_t>;
  monk_melee_attack_t( monk_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil() );
  result_amount_type amount_type( const action_state_t *state, bool periodic ) const override;
};

template <class base_action_t>
struct brew_t : base_action_t
{
  template <typename... Args>
  brew_t( monk_t *player, Args &&...args );
  void execute() override;
};

struct brews_t
{
  std::unordered_map<unsigned, cooldown_t *> cooldowns;

  void insert_cooldown( action_t *action );
  bool contains( action_t *action ) const;
  void adjust( timespan_t reduction );
};

struct conduit_of_the_celestials_container_t
{
  action_t *base;
  action_t *celestial;

  conduit_of_the_celestials_container_t() : base( nullptr ), celestial( nullptr )
  {
  }

  conduit_of_the_celestials_container_t( monk_t * ) : base( nullptr ), celestial( nullptr )
  {
  }
};

struct flurry_strikes_t : public monk_spell_t
{
  struct flurry_strike_t : public monk_spell_t
  {
    flurry_strike_t( monk_t *player, std::string_view name );
    void impact( action_state_t *state ) override;
  };

  enum source_e
  {
    NONE,
    FLURRY_STRIKES,
    STAND_READY,
    WISDOM_OF_THE_WALL
  };

  bool fallback;
  action_t *high_impact;
  action_t *shado_over_the_battlefield;
  std::unordered_map<source_e, flurry_strike_t *> flurry_strike_variants;
  cooldown_t *wisdom_of_the_wall;

  flurry_strikes_t( bool, monk_t * );
  using monk_spell_t::execute;
  void execute( source_e );
};
}  // namespace actions

namespace buffs
{
template <typename Base = buff_t>
struct monk_buff_t : public Base
{
  using base_t = Base;

  monk_buff_t( monk_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil(),
               const item_t *item = nullptr );
  monk_buff_t( monk_td_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil(),
               const item_t *item = nullptr );
  monk_td_t &get_td( player_t *target );
  const monk_td_t *find_td( player_t *target ) const;
  monk_t &p();
  const monk_t &p() const;
};

struct gift_of_the_ox_t : monk_buff_t<>
{
  /*
   * TODO:
   *  - Check which spell id is triggered by expire and by trigger orb
   */
  struct orb_t : actions::monk_heal_t
  {
    orb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data );

    void impact( action_state_t *state ) override;
  };

  struct orb_event_t : event_t
  {
    std::queue<orb_event_t *> *queue;
    std::function<void()> expire_cb;

    orb_event_t( monk_t *player, timespan_t duration, std::queue<orb_event_t *> *queue,
                 std::function<void()> expire_cb );
    void execute() override;
    const char *name() const override;
  };

  monk_t *player;
  orb_t *heal_trigger;
  orb_t *heal_expire;
  std::queue<orb_event_t *> queue;
  double accumulator;
  proc_data_t proc_data;

  // just using the first orb spawner.
  // 124503 also exists, but it just spawns an orb on the opposite side, so no
  // impact in simc
  gift_of_the_ox_t( monk_t *player );
  void spawn_orb( int count );
  void trigger_from_damage( double amount );
  int consume( int count );
  void remove();
  void reset();
};

struct empowered_tiger_lightning_t : monk_buff_t<>
{
  empowered_tiger_lightning_t( monk_td_t & );

  using monk_buff_t<>::trigger;
  bool trigger( const action_state_t * );
};

struct aspect_of_harmony_t
{
private:
  template <class base_action_t>
  friend struct purified_spirit_t;

  struct accumulator_t;
  struct spender_t;
  propagate_const<accumulator_t *> accumulator;  // accumulator buff
  propagate_const<spender_t *> spender;          // spender buff
  propagate_const<action_t *> damage;            // spender damage periodic action
  propagate_const<action_t *> heal;              // spender heal periodic action
  propagate_const<action_t *> purified_spirit;   // purified spirit damage / heal
  propagate_const<buff_t *> path_of_resurgence;  // path of resurgence buff

  bool fallback;

  struct accumulator_t : monk_buff_t<>
  {
    aspect_of_harmony_t *aspect_of_harmony;
    sc_timeline_t pool_size_percent;  // pool as a fraction of current maximum hp
    accumulator_t( monk_t *player, aspect_of_harmony_t *aspect_of_harmony );
    void trigger_with_state( action_state_t *state );
    void adjust( double amount );
  };

  struct spender_t : monk_buff_t<>
  {
    template <class base_action_t>
    struct purified_spirit_t : base_action_t
    {
      aspect_of_harmony_t *aspect_of_harmony;

      purified_spirit_t( monk_t *player, const spell_data_t *spell_data, aspect_of_harmony_t *aspect_of_harmony );
      void init() override;
      void execute() override;
    };

    template <class base_action_t>
    struct tick_t : residual_action::residual_periodic_action_t<base_action_t>
    {
      tick_t( monk_t *player, std::string_view name, const spell_data_t *spell_data );
    };

    aspect_of_harmony_t *aspect_of_harmony;
    double pool;

    spender_t( monk_t *player, aspect_of_harmony_t *aspect_of_harmony );
    void reset() override;
    bool trigger( int stacks = -1, double = DEFAULT_VALUE(), double chance = -1.0,
                  timespan_t duration = timespan_t::min() ) override;
    void trigger_with_state( action_state_t *state );
  };

public:
  aspect_of_harmony_t();
  void construct_buffs( monk_t *player );
  void construct_actions( monk_t *player );

  void trigger( action_state_t *state );
  void trigger_flat( double amount );
  void trigger_spend();
  void trigger_path_of_resurgence();

  bool heal_ticking();

  const sc_timeline_t &pool_size_percent() const
  {
    return accumulator->pool_size_percent;
  }
};

struct balanced_stratagem_t : monk_buff_t<>
{
  std::unordered_set<unsigned> allowlist;

  balanced_stratagem_t( monk_t *player, std::string_view name, const spell_data_t *spell_data,
                        std::unordered_set<unsigned> allowlist );

  using monk_buff_t<>::trigger;
  bool trigger( const spell_data_t * );
};

struct fractional_absorb_t : public monk_buff_t<absorb_buff_t>
{
  double absorb_fraction;

  fractional_absorb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data );

  double consume( double amount, action_state_t *state = nullptr ) override;
  absorb_buff_t *set_absorb_fraction( double fraction );
};

struct flurry_charge_t : monk_buff_t<>
{
  flurry_charge_t( monk_t *player );

  using monk_buff_t<>::trigger;
  bool trigger( action_state_t *state, weapon_t *weapon );
};
}  // namespace buffs

struct monk_td_t : public actor_target_data_t
{
  struct dots_t
  {
    propagate_const<dot_t *> breath_of_fire;
    propagate_const<dot_t *> crackling_jade_lightning_aoe;

    // Master of Harmony
    propagate_const<dot_t *> aspect_of_harmony;
  } dot;

  struct debuff_t
  {
    // Brewmaster
    propagate_const<buff_t *> keg_smash;
    propagate_const<buff_t *> exploding_keg;

    // Windwalker
    propagate_const<buff_t *> empowered_tiger_lightning;

    // Shado-Pan
    propagate_const<buff_t *> high_impact;
  } debuff;

  monk_t &monk;
  monk_td_t( player_t *target, monk_t *p );
};

struct monk_effect_callback_t : dbc_proc_callback_t
{
  monk_t *player;

  using post_init_callback_fn_t = std::function<void( monk_effect_callback_t * )>;

private:
  std::vector<post_init_callback_fn_t> post_init_callbacks;

public:
  monk_effect_callback_t( const special_effect_t &effect, monk_t *player );
  void trigger( const proc_data_t &data, player_t *target, action_state_t *state, proc_trigger_type_e type ) override;
  void execute( const spell_data_t *spell, player_t *target, action_state_t *state ) override;
  void initialize() override;

  monk_effect_callback_t *register_post_init_callback( const post_init_callback_fn_t &fn );
  monk_effect_callback_t *register_callback_trigger_function( dbc_proc_callback_t::trigger_fn_type t,
                                                              const dbc_proc_callback_t::trigger_fn_t &fn );
  monk_effect_callback_t *register_callback_execute_function( const dbc_proc_callback_t::execute_fn_t &fn );
};

struct monk_callback_init_t
{
  const spell_data_t *effect_driver;
  proc_flag pf_override;
  proc_flag2 pf2_override;
  action_t *action_override;
  double ppm;

  monk_callback_init_t( const spell_data_t *sd = nullptr, proc_flag pf = static_cast<proc_flag>( 0ull ),
                        proc_flag2 pf2 = static_cast<proc_flag2>( 0ull ), action_t *ac = nullptr, double ppm = 0.0 )
    : effect_driver( sd ), pf_override( pf ), pf2_override( pf2 ), action_override( ac ), ppm( ppm ) {};
};

// utility to create target_effect_t compatible functions from monk_td_t member references
// adapted from sc_death_knight.cpp
template <typename T>
static std::function<double( actor_target_data_t * )> td_fn( T effect, bool stack = true )
{
  if constexpr ( std::is_invocable_v<T, monk_td_t::debuff_t> )
  {
    if ( stack )
      return [ effect ]( actor_target_data_t *target_data ) {
        return std::invoke( effect, static_cast<monk_td_t *>( target_data )->debuff )->check();
      };
    else
      return [ effect ]( actor_target_data_t *target_data ) {
        return std::invoke( effect, static_cast<monk_td_t *>( target_data )->debuff )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<T, monk_td_t::dots_t> )
  {
    if ( stack )
      return [ effect ]( actor_target_data_t *target_data ) {
        return std::invoke( effect, static_cast<monk_td_t *>( target_data )->dot )->current_stack();
      };
    else
      return [ effect ]( actor_target_data_t *target_data ) {
        return std::invoke( effect, static_cast<monk_td_t *>( target_data )->dot )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of monk_td_t" );
    return nullptr;
  }
}

struct monk_t : public stagger_t<parse_player_effects_t, monk_t>
{
  using base_t = stagger_t<parse_player_effects_t, monk_t>;

private:
  target_specific_t<monk_td_t> target_data;

public:
  // For Debug reporting, used by create_proc_callback in init_special_effects
  std::map<std::string, std::vector<action_t *>> proc_tracking;

  struct
  {
    // Monk
    propagate_const<action_t *> chi_wave;

    // Brewmaster
    propagate_const<action_t *> special_delivery;
    propagate_const<heal_t *> celestial_fortune;
    propagate_const<action_t *> exploding_keg;
    propagate_const<heal_t *> refreshing_drink;
    propagate_const<action_t *> vital_flame;
    propagate_const<action_t *> walk_with_the_ox;
    propagate_const<accumulated_rng_t *> walk_with_the_ox_rng;

    // Windwalker
    propagate_const<action_t *> empowered_tiger_lightning;
    propagate_const<action_t *> flurry_of_xuen;
    propagate_const<action_t *> combat_wisdom_eh;

    // Conduit of the Celestials
    actions::conduit_of_the_celestials_container_t courage_of_the_white_tiger;
    actions::conduit_of_the_celestials_container_t strength_of_the_black_ox;
    actions::conduit_of_the_celestials_container_t flight_of_the_red_crane;

    // Master of Harmony
    propagate_const<action_t *> harmonic_surge;

    // Shado-Pan
    actions::flurry_strikes_t *flurry_strikes;
  } action;

  std::vector<action_t *> combo_strike_actions;

  struct
  {
    // General
    propagate_const<buff_t *> chi_wave;
    propagate_const<buff_t *> rushing_jade_wind;
    propagate_const<buff_t *> spinning_crane_kick;  // TODO: is this necessary?
    propagate_const<buff_t *> yulons_grace;

    // Brewmaster
    propagate_const<buff_t *> blackout_combo;
    propagate_const<buff_t *> celestial_flames;
    propagate_const<buff_t *> charred_passions;
    propagate_const<buff_t *> counterstrike;
    propagate_const<buff_t *> elixir_of_determination;
    propagate_const<buff_t *> elusive_brawler;
    propagate_const<buff_t *> empty_barrel;
    propagate_const<buff_t *> empty_the_cellar;
    propagate_const<buff_t *> exploding_keg;
    propagate_const<buff_t *> fortifying_brew;
    propagate_const<buff_t *> fuel_on_the_fire;
    propagate_const<buffs::gift_of_the_ox_t *> gift_of_the_ox;
    propagate_const<buff_t *> expel_harm_accumulator;
    propagate_const<buff_t *> invoke_niuzao;
    propagate_const<buff_t *> niuzaos_resolve;
    propagate_const<buff_t *> press_the_advantage;
    propagate_const<buff_t *> pretense_of_instability;
    propagate_const<buff_t *> refreshing_drink;
    propagate_const<buff_t *> shuffle;
    propagate_const<buff_t *> swift_as_a_coursing_river;
    propagate_const<buff_t *> training_of_niuzao;
    propagate_const<buff_t *> ox_stance;

    // Windwalker
    propagate_const<buff_t *> teachings_of_the_monastery;
    propagate_const<buff_t *> combo_breaker;
    propagate_const<buff_t *> combat_wisdom;
    propagate_const<buff_t *> combo_strikes;
    propagate_const<buff_t *> dance_of_chiji;
    propagate_const<buff_t *> hit_combo;
    propagate_const<buff_t *> flurry_of_xuen;
    propagate_const<buff_t *> invoke_xuen;
    propagate_const<buff_t *> memory_of_the_monastery;
    propagate_const<buff_t *> momentum_boost_damage;
    propagate_const<buff_t *> momentum_boost_speed;
    propagate_const<buff_t *> thunderfist;
    propagate_const<buff_t *> touch_of_death_ww;
    propagate_const<buff_t *> touch_of_karma;
    propagate_const<buff_t *> whirling_dragon_punch;
    propagate_const<buff_t *> zenith;
    propagate_const<buff_t *> zenith_stomp;
    propagate_const<buff_t *> rushing_wind_kick;
    propagate_const<buff_t *> tigereye_brew_1;
    propagate_const<buff_t *> tigereye_brew_1_accumulator;
    propagate_const<buff_t *> tigereye_brew_3;

    // Conduit of the Celestials
    propagate_const<buff_t *> celestial_conduit;
    propagate_const<buff_t *> courage_of_the_white_tiger;
    propagate_const<buff_t *> heart_of_the_jade_serpent;
    propagate_const<buff_t *> heart_of_the_jade_serpent_yulons_avatar;
    propagate_const<buff_t *> heart_of_the_jade_serpent_unity_within;
    propagate_const<buff_t *> inner_compass_crane_stance;
    propagate_const<buff_t *> inner_compass_ox_stance;
    propagate_const<buff_t *> inner_compass_serpent_stance;
    propagate_const<buff_t *> inner_compass_tiger_stance;
    propagate_const<buff_t *> jade_sanctuary;
    propagate_const<buff_t *> strength_of_the_black_ox;
    propagate_const<buff_t *> unity_within;

    // Master of Harmony
    buffs::aspect_of_harmony_t aspect_of_harmony;
    propagate_const<buffs::balanced_stratagem_t *> balanced_stratagem_physical;
    propagate_const<buffs::balanced_stratagem_t *> balanced_stratagem_magic;
    propagate_const<buff_t *> harmonic_surge;

    // Shado-Pan
    propagate_const<buffs::flurry_charge_t *> flurry_charge;
    propagate_const<buff_t *> predictive_training;
    propagate_const<buff_t *> stand_ready;
    propagate_const<buff_t *> whirling_steel;

    // Tier
    propagate_const<buff_t *> mid2_ww_4pc;
  } buff;

  struct
  {
    propagate_const<gain_t *> black_ox_brew_energy;
    propagate_const<gain_t *> chi_refund;
    propagate_const<gain_t *> combo_breaker;
    propagate_const<gain_t *> energy_burst;
    propagate_const<gain_t *> energy_refund;
    propagate_const<gain_t *> touch_of_death_ww;
    propagate_const<gain_t *> zenith;
  } gain;

  struct
  {
    propagate_const<proc_t *> anvil_and_stave;
    propagate_const<proc_t *> blackout_combo_tiger_palm;
    propagate_const<proc_t *> blackout_combo_keg_smash;
    propagate_const<proc_t *> charred_passions;
    propagate_const<proc_t *> elusive_footwork_proc;
    propagate_const<proc_t *> salsalabims_strength;
    propagate_const<proc_t *> tranquil_spirit_expel_harm;
    propagate_const<proc_t *> tranquil_spirit_goto;
    propagate_const<proc_t *> xuens_battlegear_sck_reduction;
    propagate_const<proc_t *> elusive_brawler_preserved;
  } proc;

  struct
  {
    propagate_const<cooldown_t *> anvil_and_stave;
    propagate_const<cooldown_t *> blackout_kick;
    propagate_const<cooldown_t *> expel_harm;
    propagate_const<cooldown_t *> fists_of_fury;
    propagate_const<cooldown_t *> rising_sun_kick;
  } cooldown;

  struct
  {
    struct
    {
      const spell_data_t *aura;
      const spell_data_t *critical_strikes;
      const spell_data_t *two_hand_adjustment;
      const spell_data_t *leather_specialization;
      const spell_data_t *expel_harm;
      const spell_data_t *expel_harm_damage;
      const spell_data_t *blackout_kick;
      const spell_data_t *crackling_jade_lightning;
      const spell_data_t *leg_sweep;
      const spell_data_t *mystic_touch;
      const spell_data_t *roll;
      const spell_data_t *spinning_crane_kick;
      const spell_data_t *tiger_palm;
      const spell_data_t *touch_of_death;
      const spell_data_t *vivify;
      const spell_data_t *provoke;
    } monk;

    struct
    {
      const spell_data_t *mastery;
      const spell_data_t *aura;
      const spell_data_t *aura_2;
      const spell_data_t *aura_3;
      const spell_data_t *aura_4;
      const spell_data_t *brewmasters_balance;
      const spell_data_t *celestial_fortune;
      const spell_data_t *celestial_fortune_heal;
      const spell_data_t *expel_harm_rank_2;
      const spell_data_t *blackout_kick;
      const spell_data_t *spinning_crane_kick;
      const spell_data_t *spinning_crane_kick_rank_2;
      const spell_data_t *touch_of_death_rank_3;

      const spell_data_t *stagger_self_damage;
      const spell_data_t *light_stagger;
      const spell_data_t *moderate_stagger;
      const spell_data_t *heavy_stagger;

      actions::brews_t brews;
    } brewmaster;

    struct
    {
      const spell_data_t *mastery;
      const spell_data_t *aura;
      const spell_data_t *aura_2;
      const spell_data_t *aura_3;
      const spell_data_t *aura_4;
      const spell_data_t *blackout_kick_rank_2;
      const spell_data_t *blackout_kick_rank_3;
      const spell_data_t *combat_conditioning;
      const spell_data_t *flying_serpent_kick;
      const spell_data_t *touch_of_death_rank_3;
      const spell_data_t *touch_of_karma;
      const spell_data_t *touch_of_karma_buff;
      const spell_data_t *touch_of_karma_tick;
      const spell_data_t *empowered_tiger_lightning;
      const spell_data_t *empowered_tiger_lightning_damage;
    } windwalker;
  } baseline;

  struct
  {
    struct
    {
      // Row 1
      player_talent_t soothing_mist;
      player_talent_t paralysis;
      player_talent_t rising_sun_kick;  // Windwalker
      player_talent_t stagger;          // Brewmaster
      // Row 2
      player_talent_t elusive_mists;
      player_talent_t tigers_lust;
      player_talent_t crashing_momentum;
      player_talent_t disable;
      player_talent_t fast_feet;
      // Row 3
      player_talent_t grace_of_the_crane;
      player_talent_t bounding_agility;
      player_talent_t calming_presence;
      player_talent_t winds_reach;
      player_talent_t detox;
      // Row 4
      player_talent_t vivacious_vivification;
      player_talent_t jade_walk;
      player_talent_t pressure_points;
      player_talent_t spear_hand_strike;
      player_talent_t ancient_arts;
      // Row 5
      player_talent_t chi_wave;  // Brewmaster
      const spell_data_t *chi_wave_buff;
      const spell_data_t *chi_wave_driver;
      const spell_data_t *chi_wave_damage;
      const spell_data_t *chi_wave_heal;
      player_talent_t chi_burst;  // Brewmaster
      const spell_data_t *chi_burst_buff;
      const spell_data_t *chi_burst_projectile;
      const spell_data_t *chi_burst_damage;
      const spell_data_t *chi_burst_heal;
      player_talent_t tiger_fang;  // Windwalker
      player_talent_t transcendence;
      player_talent_t energy_transfer;
      player_talent_t celerity;
      player_talent_t chi_torpedo;
      player_talent_t stillstep_coil;
      // Row 6
      player_talent_t quick_footed;
      player_talent_t hasty_provocation;
      player_talent_t ferocity_of_xuen;
      player_talent_t ring_of_peace;
      player_talent_t song_of_chi_ji;
      player_talent_t spirits_essence;
      player_talent_t tiger_tail_sweep;
      player_talent_t improved_touch_of_death;
      // Row 7
      player_talent_t vigorous_expulsion;
      player_talent_t yulons_grace;
      const spell_data_t *yulons_grace_buff;
      player_talent_t peace_and_prosperity;
      player_talent_t fortifying_brew;
      const spell_data_t *fortifying_brew_buff;
      player_talent_t dance_of_the_wind;
      // Row 8
      player_talent_t save_them_all;
      player_talent_t swift_art;
      player_talent_t strength_of_spirit;
      player_talent_t profound_rebuttal;
      player_talent_t summon_black_ox_statue;  // Brewmaster
      player_talent_t zenith_stomp;            // Windwalker
      const spell_data_t *zenith_stomp_damage;
      const spell_data_t *zenith_stomp_buff;
      player_talent_t ironshell_brew;
      player_talent_t expeditious_fortification;
      player_talent_t diffuse_magic;
      player_talent_t celestial_determination;
      // Row 9
      player_talent_t chi_proficiency;
      player_talent_t healing_winds;
      player_talent_t windwalking;
      player_talent_t chi_transfer;
      player_talent_t martial_instincts;
      // Row 10
      player_talent_t lighter_than_air;
      player_talent_t flow_of_chi;
      player_talent_t escape_from_reality;
      player_talent_t transcendence_linked_spirits;
      player_talent_t fatal_touch;
      player_talent_t rushing_reflexes;
    } monk;

    // Brewmaster
    struct
    {
      // row 1
      player_talent_t keg_smash;
      // row 2
      player_talent_t purifying_brew;
      player_talent_t shuffle;
      const spell_data_t *shuffle_buff;
      // row 3
      player_talent_t august_blessing;
      player_talent_t staggering_strikes;
      player_talent_t quick_sip;
      player_talent_t elixir_of_determination;
      const spell_data_t *elixir_of_determination_cooldown;
      player_talent_t improved_blackout_kick;
      player_talent_t swift_as_a_coursing_river;
      // row 4
      player_talent_t gift_of_the_ox;
      const spell_data_t *gift_of_the_ox_buff;
      const spell_data_t *gift_of_the_ox_heal_trigger;
      const spell_data_t *gift_of_the_ox_heal_expire;
      player_talent_t special_delivery;
      const spell_data_t *special_delivery_missile;
      player_talent_t rushing_jade_wind;
      player_talent_t spirit_of_the_ox;
      // row 5
      player_talent_t jade_flash;
      player_talent_t celestial_brew;
      player_talent_t celestial_infusion;
      player_talent_t niuzaos_resolve;
      const spell_data_t *niuzaos_resolve_hot;
      player_talent_t celestial_flames;
      const spell_data_t *celestial_flames_damage;
      player_talent_t shadowboxing_treads;
      player_talent_t fluidity_of_motion;
      player_talent_t elusive_footwork;
      player_talent_t one_with_the_wind;
      // row 6
      player_talent_t breath_of_fire;
      const spell_data_t *breath_of_fire_dot;
      player_talent_t gai_plins_imperial_brew;
      const spell_data_t *gai_plins_imperial_brew_heal;
      player_talent_t light_brewing;
      player_talent_t training_of_niuzao;
      player_talent_t pretense_of_instability;
      // row 7
      player_talent_t scalding_brew;
      player_talent_t salsalabims_strength;
      player_talent_t fortifying_brew_determination;
      player_talent_t bob_and_weave;
      player_talent_t black_ox_brew;
      player_talent_t walk_with_the_ox;
      const spell_data_t *walk_with_the_ox_stomp;
      player_talent_t zen_state;
      player_talent_t tranquil_spirit;
      player_talent_t face_palm;
      // row 8
      player_talent_t dragonfire_brew;
      const spell_data_t *dragonfire_brew_hit;
      player_talent_t charred_passions;
      const spell_data_t *charred_passions_damage;
      player_talent_t high_tolerance;
      player_talent_t press_the_advantage;
      const spell_data_t *press_the_advantage_damage;
      const spell_data_t *press_the_advantage_tiger_palm;
      player_talent_t blackout_combo;
      player_talent_t anvil_and_stave;
      player_talent_t counterstrike;
      // row 9
      player_talent_t exploding_keg;
      player_talent_t ox_stance;
      const spell_data_t *ox_stance_buff;
      player_talent_t awakening_spirit;
      player_talent_t vital_flame;
      const spell_data_t *vital_flame_heal;
      player_talent_t invoke_niuzao_the_black_ox;
      const spell_data_t *invoke_niuzao_the_black_ox_npc;
      const spell_data_t *invoke_niuzao_the_black_ox_stomp;
      // row 10
      player_talent_t fuel_on_the_fire;
      const spell_data_t *fuel_on_the_fire_buff;
      const spell_data_t *fuel_on_the_fire_damage;
      player_talent_t empty_the_cellar;
      const spell_data_t *empty_the_cellar_buff;
      const spell_data_t *empty_the_cellar_driver;
      const spell_data_t *empty_the_cellar_damage;
      player_talent_t keg_volley;
      player_talent_t stormstouts_last_keg;
      player_talent_t heart_of_the_ox;
      player_talent_t mighty_stomp;
      // apex
      player_talent_t bring_me_another_1;
      player_talent_t bring_me_another_2;
      player_talent_t bring_me_another_3;
      const spell_data_t *empty_barrel_damage;
      const spell_data_t *refreshing_drink_buff;
      const spell_data_t *refreshing_drink_hot;
    } brewmaster;

    // Windwalker
    struct
    {
      // Row 1
      player_talent_t fists_of_fury;
      // Row 2
      player_talent_t momentum_boost;
      const spell_data_t *momentum_boost_speed;
      player_talent_t combat_wisdom;
      const spell_data_t *combat_wisdom_buff;
      const spell_data_t *combat_wisdom_expel_harm;
      player_talent_t sharp_reflexes;
      // Row 3
      player_talent_t touch_of_the_tiger;
      player_talent_t ferociousness;
      player_talent_t hardened_soles;
      player_talent_t ascension;
      // Row 4
      player_talent_t dual_threat;
      const spell_data_t *dual_threat_damage;
      player_talent_t teachings_of_the_monastery;
      const spell_data_t *teachings_of_the_monastery_buff;
      const spell_data_t *teachings_of_the_monastery_blackout_kick;
      player_talent_t glory_of_the_dawn;
      const spell_data_t *glory_of_the_dawn_damage;
      // Row 5
      player_talent_t crane_vortex;
      player_talent_t meridian_strikes;
      player_talent_t rising_star;
      player_talent_t zenith;
      player_talent_t hit_combo;
      const spell_data_t *hit_combo_buff;
      player_talent_t brawlers_intensity;
      // Row 6
      player_talent_t jade_ignition;
      player_talent_t cyclones_drift;
      player_talent_t crashing_fists;
      player_talent_t drinking_horn_cover;
      player_talent_t spiritual_focus;
      player_talent_t obsidian_spiral;
      const spell_data_t *obsidian_spiral_energize;
      player_talent_t combo_breaker;
      const spell_data_t *combo_breaker_buff;
      // Row 7
      player_talent_t dance_of_chiji;
      const spell_data_t *dance_of_chiji_buff;
      player_talent_t shadowboxing_treads;
      player_talent_t strike_of_the_windlord;
      player_talent_t whirling_dragon_punch;
      const spell_data_t *whirling_dragon_punch_aoe_tick;
      const spell_data_t *whirling_dragon_punch_st_tick;
      const spell_data_t *whirling_dragon_punch_buff;
      player_talent_t energy_burst;
      player_talent_t inner_peace;
      // Row 8
      player_talent_t sequenced_strikes;
      player_talent_t sunfire_spiral;
      player_talent_t communion_with_wind;
      player_talent_t revolving_whirl;
      player_talent_t echo_technique;
      player_talent_t universal_energy;
      player_talent_t memory_of_the_monastery;
      const spell_data_t *memory_of_the_monastery_buff;
      // Row 9
      player_talent_t rushing_wind_kick;
      const spell_data_t *rushing_wind_kick_buff;
      const spell_data_t *rushing_wind_kick_action;
      player_talent_t xuens_battlegear;
      player_talent_t thunderfist;
      const spell_data_t *thunderfist_buff;
      player_talent_t weapon_of_wind;
      player_talent_t knowledge_of_the_broken_temple;
      player_talent_t slicing_winds;
      const spell_data_t *slicing_winds_damage;
      player_talent_t jadefire_stomp;
      const spell_data_t *jadefire_stomp_damage;
      const spell_data_t *jadefire_stomp_targeting;
      // Row 10
      player_talent_t skyfire_heel;
      const spell_data_t *skyfire_heel_damage;
      const spell_data_t *skyfire_heel_buff;
      player_talent_t harmonic_combo;
      player_talent_t flurry_of_xuen;
      const spell_data_t *flurry_of_xuen_driver;
      player_talent_t martial_agility;
      player_talent_t airborne_rhythm;
      const spell_data_t *airborne_rhythm_energize;
      player_talent_t hurricanes_vault;
      player_talent_t path_of_jade;
      player_talent_t singularly_focused_jade;
      // Apex
      player_talent_t tigereye_brew_1;
      const spell_data_t *tigereye_brew_1_buff;
      player_talent_t tigereye_brew_2;
      player_talent_t tigereye_brew_3;
      const spell_data_t *tigereye_brew_3_buff;
    } windwalker;

    // Conduit of the Celestials
    struct
    {
      // Row 1
      player_talent_t invoke_xuen_the_white_tiger;
      const spell_data_t *invoke_xuen_the_white_tiger_npc;
      const spell_data_t *crackling_tiger_lightning_driver;
      // Row 2
      player_talent_t temple_training;
      player_talent_t xuens_guidance;
      player_talent_t courage_of_the_white_tiger;
      const spell_data_t *courage_of_the_white_tiger_buff;
      const spell_data_t *courage_of_the_white_tiger_damage;
      const spell_data_t *courage_of_the_white_tiger_heal;
      player_talent_t restore_balance;
      player_talent_t xuens_bond;
      player_talent_t heart_of_the_jade_serpent;
      const spell_data_t *heart_of_the_jade_serpent_buff;
      // Row 3
      player_talent_t chijis_swiftness;
      player_talent_t strength_of_the_black_ox;
      const spell_data_t *strength_of_the_black_ox_buff;
      const spell_data_t *strength_of_the_black_ox_absorb;
      const spell_data_t *strength_of_the_black_ox_damage;
      player_talent_t path_of_the_falling_star;
      player_talent_t yulons_avatar;
      const spell_data_t *yulons_avatar_buff;
      // Row 4
      player_talent_t niuzaos_protection;
      player_talent_t jade_sanctuary;
      const spell_data_t *jade_sanctuary_buff;
      player_talent_t celestial_conduit;
      const spell_data_t *celestial_conduit_action;
      const spell_data_t *celestial_conduit_damage;
      const spell_data_t *celestial_conduit_heal;
      player_talent_t inner_compass;
      const spell_data_t *inner_compass_crane_stance_buff;
      const spell_data_t *inner_compass_ox_stance_buff;
      const spell_data_t *inner_compass_tiger_stance_buff;
      const spell_data_t *inner_compass_serpent_stance_buff;
      player_talent_t flowing_wisdom;
      // Row 5
      player_talent_t unity_within;
      const spell_data_t *unity_within_buff;
      const spell_data_t *unity_within_heart_of_the_jade_serpent_buff;
      const spell_data_t *unity_within_dmg_mult;
      // Row Crackbird
      const spell_data_t *flight_of_the_red_crane_damage;
    } conduit_of_the_celestials;

    // Master of Harmony
    struct
    {
      // Row 1
      player_talent_t aspect_of_harmony;
      const spell_data_t *aspect_of_harmony_driver;
      const spell_data_t *aspect_of_harmony_accumulator;
      const spell_data_t *aspect_of_harmony_spender;
      const spell_data_t *aspect_of_harmony_damage;
      const spell_data_t *aspect_of_harmony_heal;
      // Row 2
      player_talent_t manifestation;
      player_talent_t purified_spirit;
      const spell_data_t *purified_spirit_damage;
      const spell_data_t *purified_spirit_heal;
      player_talent_t harmonic_gambit;
      player_talent_t balanced_stratagem;
      const spell_data_t *balanced_stratagem_physical;
      const spell_data_t *balanced_stratagem_magic;
      player_talent_t harmonic_surge;
      const spell_data_t *harmonic_surge_buff;
      const spell_data_t *harmonic_surge_damage;
      const spell_data_t *harmonic_surge_heal;
      // Row 3
      player_talent_t tigers_vigor;
      player_talent_t roar_from_the_heavens;
      player_talent_t endless_draught;
      player_talent_t mantra_of_purity;
      player_talent_t mantra_of_tenacity;
      player_talent_t potential_energy;
      // Row 4
      player_talent_t overwhelming_force;
      const spell_data_t *overwhelming_force_damage;
      player_talent_t path_of_resurgence;
      player_talent_t way_of_a_thousand_strikes;
      player_talent_t clarity_of_purpose;
      player_talent_t meditative_focus;
      // Row 5
      player_talent_t coalescence;
    } master_of_harmony;

    // Shado-Pan
    struct
    {
      // Row 1
      player_talent_t flurry_strikes;
      const spell_data_t *flurry_strike;
      const spell_data_t *flurry_charge;
      // Row 2
      player_talent_t pride_of_pandaria;
      player_talent_t high_impact;
      const spell_data_t *high_impact_debuff;
      player_talent_t veterans_eye;
      player_talent_t martial_precision;
      player_talent_t shado_over_the_battlefield;
      const spell_data_t *flurry_strike_shado_over_the_battlefield;
      // Row 3
      player_talent_t combat_stance;
      player_talent_t initiators_edge;
      player_talent_t one_versus_many;
      player_talent_t whirling_steel;
      player_talent_t predictive_training;
      player_talent_t stand_ready;
      const spell_data_t *stand_ready_buff;
      // Row 4
      player_talent_t against_all_odds;
      player_talent_t efficient_training;
      player_talent_t vigilant_watch;
      player_talent_t weapons_of_the_wall;
      // Row 5
      player_talent_t wisdom_of_the_wall;
    } shado_pan;
  } talent;

  struct
  {
    struct
    {
      const spell_data_t *brm_2pc;
      const spell_data_t *brm_4pc;
      const spell_data_t *brm_4pc_extra_kick;
    } mid1;

    struct
    {
      const spell_data_t *ww_4pc_buff;
    } mid2;

    struct
    {
    } mid3;
  } tier;

  struct pets_t
  {
    spawner::pet_spawner_t<pet_t, monk_t> xuen;
    spawner::pet_spawner_t<pets::niuzao::niuzao_pet_t, monk_t> niuzao;

    pets_t( monk_t *p );
  } pets;

  // Options
  struct options_t
  {
    int initial_chi;
    double expel_harm_effectiveness;
    int chi_burst_healing_targets;
    int shado_pan_initial_charge_accumulator;
  } user_options;

public:
  monk_t( sim_t *sim, std::string_view name, race_e r );

  // APL
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;
  void init_action_list() override;
  void init_blizzard_action_list() override;
  void parse_assisted_combat_step( const assisted_combat_step_data_t &step,
                                   action_priority_list_t *assisted_combat ) override;
  std::vector<std::string> action_names_from_spell_id( unsigned int spell_id ) const override;
  std::string aura_expr_from_spell_id( unsigned int spell_id, bool on_self = true ) const override;
  parsed_assisted_combat_rule_t parse_assisted_combat_rule( const assisted_combat_rule_data_t &rule,
                                                            const assisted_combat_step_data_t &step ) const override;
  bool validate_actor() override;
  bool validate_fight_style( fight_style_e style ) const override;

  // Init / Reset
  void init_spells() override;
  void init_background_actions() override;
  void init_base_stats() override;
  void init_scaling() override;
  void init_gains() override;
  void init_procs() override;
  void init_special_effects() override;
  void init_assessors() override;
  void init_finished() override;
  void create_buffs() override;
  action_t *create_action( std::string_view name, std::string_view options ) override;
  void create_actions() override;
  void create_options() override;
  std::unique_ptr<expr_t> create_expression( std::string_view name_str ) override;
  void reset() override;
  void copy_from( player_t * ) override;

  // Combat
  void combat_begin() override;
  void target_mitigation( school_e, result_amount_type, action_state_t * ) override;
  void assess_damage( school_e, result_amount_type, action_state_t *s ) override;
  void assess_heal( school_e, result_amount_type, action_state_t *s ) override;
  void invalidate_cache( cache_e ) override;

  // Stats
  role_e primary_role() const override;
  resource_e primary_resource() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double composite_attack_power_multiplier() const override;
  double composite_dodge() const override;
  double composite_player_target_armor( player_t *target ) const override;

  // Other
  bool wowv_l( wowv_t value ) const;
  bool wowv_ge( wowv_t value ) const;
  const monk_td_t *find_target_data( const player_t *target ) const override
  {
    return target_data[ target ];
  }
  monk_td_t *get_target_data( player_t *target ) const override
  {
    monk_td_t *&td = target_data[ target ];
    if ( !td )
    {
      td = new monk_td_t( target, const_cast<monk_t *>( this ) );
    }
    return td;
  }
  void parse_player_effects();
  monk_effect_callback_t *create_proc_callback( monk_callback_init_t params );

  // Actions
  void trigger_celestial_fortune( action_state_t * );
};

namespace events
{
// based on implementation from sc_demon_hunter.cpp
struct delayed_execute_event_t : event_t
{
  action_t *action;
  player_t *target;

  delayed_execute_event_t( monk_t *player, action_t *action, player_t *target, timespan_t delay )
    : event_t( *player->sim, delay ), action( action ), target( target )
  {
    assert( action->background && "Delayed Execute actions must be background!" );
  }

  const char *name() const override
  {
    return action->name();
  }

  void execute() override
  {
    if ( target->is_sleeping() )
      return;
    action->execute_on_target( target );
  }
};

struct delayed_buff_trigger_event_t : event_t
{
  buff_t *buff;

  delayed_buff_trigger_event_t( monk_t *player, buff_t *buff, timespan_t delay )
    : event_t( *player->sim, delay ), buff( buff )
  {
  }

  const char *name() const override
  {
    return buff->name();
  }

  void execute() override
  {
    buff->trigger();
  }
};

struct repeating_dynamic_period_cb_event_data_t
{
  std::function<timespan_t( monk_t * )> period_fn;
  std::function<void( monk_t * )> callback;

  repeating_dynamic_period_cb_event_data_t( std::function<timespan_t( monk_t * )> period_fn,
                                            std::function<void( monk_t * )> callback )
    : period_fn( std::move( period_fn ) ), callback( std::move( callback ) )
  {
  }
};

struct repeating_dynamic_period_cb_event_t : event_t
{
  monk_t *player;
  std::unique_ptr<repeating_dynamic_period_cb_event_data_t> data;

  repeating_dynamic_period_cb_event_t( monk_t *player, std::function<timespan_t( monk_t * )> period_fn,
                                       std::function<void( monk_t * )> callback )
    : event_t( *player->sim, period_fn( player ) ),
      player( player ),
      data( std::make_unique<repeating_dynamic_period_cb_event_data_t>( period_fn, callback ) )
  {
  }

  repeating_dynamic_period_cb_event_t( monk_t *player, std::unique_ptr<repeating_dynamic_period_cb_event_data_t> data )
    : event_t( *player->sim, data->period_fn( player ) ), player( player ), data( std::move( data ) )
  {
  }

  const char *name() const override
  {
    return "repeating_dynamic_period_cb_event_t";
  }

  void execute() override
  {
    data->callback( player );
    make_event<repeating_dynamic_period_cb_event_t>( *player->sim, player, std::move( data ) );
  }
};

struct delayed_cb_event_t : event_t
{
  std::function<void()> cb;

  delayed_cb_event_t( monk_t *player, timespan_t delay, std::function<void()> cb )
    : event_t( *player->sim, delay ), cb( std::move( cb ) )
  {
  }

  const char *name() const override
  {
    return "delayed_cb_event_t";
  }

  void execute() override
  {
    cb();
  }
};
}  // namespace events

}  // namespace monk
