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
#include <vector>

#include "simulationcraft.hpp"

namespace monk
{
struct monk_t;
struct monk_td_t;

namespace pets
{
struct storm_earth_and_fire_pet_t;
struct xuen_pet_t;
struct niuzao_pet_t;
struct call_to_arms_niuzao_pet_t;
struct chiji_pet_t;
struct yulon_pet_t;
struct white_tiger_statue_t;
struct fury_of_xuen_pet_t;

enum class sef_pet_e
{
  SEF_FIRE = 0,
  SEF_EARTH,
  SEF_PET_MAX
};
}  // namespace pets

namespace actions
{
enum class sef_ability_e
{
  SEF_NONE = -1,
  // Attacks begin here
  SEF_TIGER_PALM,
  SEF_BLACKOUT_KICK,
  SEF_BLACKOUT_KICK_TOTM,
  SEF_RISING_SUN_KICK,
  SEF_GLORY_OF_THE_DAWN,
  SEF_FISTS_OF_FURY,
  SEF_SPINNING_CRANE_KICK,
  SEF_WHIRLING_DRAGON_PUNCH,
  SEF_STRIKE_OF_THE_WINDLORD,
  SEF_STRIKE_OF_THE_WINDLORD_OH,
  SEF_CELESTIAL_CONDUIT,
  SEF_ATTACK_MAX,
  // Attacks end here

  // Spells begin here
  SEF_CHI_WAVE,
  SEF_CRACKLING_JADE_LIGHTNING,
  SEF_SPELL_MAX,
  // Spells end here

  // Misc
  SEF_SPELL_MIN  = SEF_CHI_WAVE,
  SEF_ATTACK_MIN = SEF_TIGER_PALM,
  SEF_MAX
};

template <class Base>
struct monk_action_t : public parse_action_effects_t<Base>
{
  sef_ability_e sef_ability;
  bool ww_mastery;
  bool may_combo_strike;
  bool trigger_chiji;
  bool cast_during_sck;
  bool track_cd_waste;

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
  template <typename... Ts>
  void apply_affecting_aura( Ts &&...args )
  {
    base_t::apply_affecting_aura( std::forward<Ts>( args )... );
  }

  const spelleffect_data_t *find_spelleffect( const spell_data_t *spell, effect_subtype_t subtype, int misc_value,
                                              const spell_data_t *affected, effect_type_t type );
  const spell_data_t *find_spell_override( const spell_data_t *base, const spell_data_t *passive );

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
  void tick( dot_t *dot ) override;
  void trigger_storm_earth_and_fire( const action_t *action );
  void trigger_mystic_touch( action_state_t *state );
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
  double action_multiplier() const override;
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

struct monk_buff_t : public buff_t
{
  monk_buff_t( monk_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil(),
               const item_t *item = nullptr );
  monk_buff_t( monk_td_t *player, std::string_view name, const spell_data_t *spell_data = spell_data_t::nil(),
               const item_t *item = nullptr );
  monk_td_t &get_td( player_t *target );
  const monk_td_t *find_td( player_t *target ) const;
  monk_t &p();
  const monk_t &p() const;
};

struct summon_pet_t : public monk_spell_t
{
  timespan_t summoning_duration;
  std::string_view pet_name;
  pet_t *pet;

  summon_pet_t( monk_t *player, std::string_view name, std::string_view pet_name,
                const spell_data_t *spell_data = spell_data_t::nil() );
  void init_finished() override;
  void execute() override;
  bool ready() override;
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
  void adjust( timespan_t reduction );
};
}  // namespace actions

namespace buffs
{
struct shuffle_t : actions::monk_buff_t
{
  timespan_t accumulator;
  const timespan_t max_duration;

  using monk_buff_t::trigger;
  shuffle_t( monk_t *monk );
  void trigger( timespan_t duration );
};

struct gift_of_the_ox_t : actions::monk_buff_t
{
  /*
   * TODO:
   *  - Check which spell id is triggered by expire and by trigger orb
   */
  struct orb_t : actions::monk_heal_t
  {
    orb_t( monk_t *player, std::string_view name, const spell_data_t *spell_data );

    double action_multiplier() const override;
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

  struct accumulator_t : actions::monk_buff_t
  {
    aspect_of_harmony_t *aspect_of_harmony;
    accumulator_t( monk_t *player, aspect_of_harmony_t *aspect_of_harmony );
    void trigger_with_state( action_state_t *state );
  };

  struct spender_t : actions::monk_buff_t
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
};
}  // namespace buffs

inline int sef_spell_index( int x )
{
  return x - static_cast<int>( actions::sef_ability_e::SEF_SPELL_MIN );
}

struct monk_td_t : public actor_target_data_t
{
public:
  struct dots_t
  {
    propagate_const<dot_t *> breath_of_fire;
    propagate_const<dot_t *> crackling_jade_lightning_aoe;
    propagate_const<dot_t *> crackling_jade_lightning_sef;
    propagate_const<dot_t *> crackling_jade_lightning_sef_aoe;
    propagate_const<dot_t *> enveloping_mist;
    propagate_const<dot_t *> renewing_mist;
    propagate_const<dot_t *> soothing_mist;
    propagate_const<dot_t *> touch_of_karma;

    // Master of Harmony
    propagate_const<dot_t *> aspect_of_harmony;
  } dot;

  struct debuff_t
  {
    // Brewmaster
    propagate_const<buff_t *> keg_smash;
    propagate_const<buff_t *> exploding_keg;

    // Windwalker
    propagate_const<buff_t *> acclamation;
    propagate_const<buff_t *> flying_serpent_kick;
    propagate_const<buff_t *> gale_force;
    propagate_const<buff_t *> empowered_tiger_lightning;
    propagate_const<buff_t *> fury_of_xuen_empowered_tiger_lightning;
    propagate_const<buff_t *> mark_of_the_crane;
    propagate_const<buff_t *> storm_earth_and_fire;
    propagate_const<buff_t *> touch_of_karma;

    // Shado-Pan
    propagate_const<buff_t *> high_impact;
    propagate_const<buff_t *> veterans_eye;

    // Covenant Abilities
    propagate_const<buff_t *> jadefire_stomp;
    propagate_const<buff_t *> weapons_of_order;

    // Shadowland Legendaries
    propagate_const<buff_t *> jadefire_brand;
  } debuff;

  monk_t &monk;
  monk_td_t( player_t *target, monk_t *p );
};

// utility to create target_effect_t compatible functions from monk_td_t member references
// adapted from sc_death_knight.cpp
template <typename T>
static std::function<int( actor_target_data_t * )> td_fn( T effect, bool stack = true )
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
  // Active
  action_t *windwalking_aura;

  // Special Auto-Attacks
  action_t *dual_threat_kick;

  // For Debug reporting, used by create_proc_callback in init_special_effects
  std::map<std::string, std::vector<action_t *>> proc_tracking;

  struct active_actions_t
  {
    // General
    propagate_const<action_t *> chi_wave;

    // Conduit of the Celestials
    propagate_const<action_t *> courage_of_the_white_tiger;
    propagate_const<action_t *> flight_of_the_red_crane_damage;
    propagate_const<action_t *> flight_of_the_red_crane_heal;
    propagate_const<action_t *> flight_of_the_red_crane_celestial_damage;
    propagate_const<action_t *> flight_of_the_red_crane_celestial_heal;
    propagate_const<action_t *> strength_of_the_black_ox_dmg;
    propagate_const<action_t *> strength_of_the_black_ox_absorb;

    // Shado-Pan
    propagate_const<action_t *> flurry_strikes;

    // Brewmaster
    propagate_const<action_t *> special_delivery;
    propagate_const<action_t *> breath_of_fire;
    propagate_const<heal_t *> celestial_fortune;
    propagate_const<action_t *> exploding_keg;
    propagate_const<action_t *> niuzao_call_to_arms_summon;
    propagate_const<action_t *> chi_surge;

    // Windwalker
    propagate_const<action_t *> empowered_tiger_lightning;
    propagate_const<action_t *> flurry_of_xuen;
    propagate_const<action_t *> fury_of_xuen_summon;
    propagate_const<action_t *> fury_of_xuen_empowered_tiger_lightning;
    propagate_const<action_t *> gale_force;
  } active_actions;

  struct passive_actions_t
  {
    action_t *combat_wisdom_eh;
    action_t *thunderfist;
    action_t *press_the_advantage;
  } passive_actions;

  std::vector<action_t *> combo_strike_actions;
  double squirm_timer;
  double spiritual_focus_count;

  int efficient_training_energy;
  int flurry_strikes_energy;
  double flurry_strikes_damage;

  //==============================================
  // Monk Movement
  //==============================================

  struct monk_movement_t : public buff_t
  {
  private:
    monk_t *p;

    propagate_const<buff_t *> base_movement;
    const spell_data_t *data;

  public:
    double distance_moved;

    monk_movement_t( monk_t *player, util::string_view n, const spell_data_t *s = spell_data_t::not_found() )
      : buff_t( player, n ), p( player ), base_movement( find( player, "movement" ) ), data( s )
    {
      set_chance( 1 );
      set_max_stack( 1 );
      set_duration( timespan_t::from_seconds( 1 ) );

      if ( base_movement == nullptr )
        base_movement = new movement_buff_t( p );

      if ( s->ok() )
      {
        set_trigger_spell( s );
        set_duration( s->duration() );
      }
    };

    void set_distance( double distance )
    {
      distance_moved = distance;
    }

    bool trigger( int stacks = 1, double value = -std::numeric_limits<double>::min(), double chance = -1.0,
                  timespan_t /*duration*/ = timespan_t::min() ) override
    {
      if ( distance_moved > 0 )
      {
        // Check if we're already moving away from the target, if so we will now be moving towards it
        if ( p->current.distance_to_move )
        {
          // TODO: Movement speed increase based on distance_moved for roll / fsk style abilities
          if ( data->ok() )
          {
          }
          p->current.distance_to_move   = distance_moved;
          p->current.movement_direction = movement_direction_type::TOWARDS;
        }
        else
        {
          // TODO: Set melee out of range
          p->current.moving_away        = distance_moved;
          p->current.movement_direction = movement_direction_type::AWAY;
        }

        return base_movement->trigger( stacks, value, chance, this->buff_duration() );
      }

      return false;
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      base_movement->expire_override( expiration_stacks, remaining_duration );
    }
  };

  struct movement_t
  {
    propagate_const<monk_movement_t *> chi_torpedo;
    propagate_const<monk_movement_t *> flying_serpent_kick;
    propagate_const<monk_movement_t *> melee_squirm;
    propagate_const<monk_movement_t *> roll;
    propagate_const<monk_movement_t *> whirling_dragon_punch;
  } movement;

  //=================================================

  struct buffs_t
  {
    // General
    propagate_const<buff_t *> chi_torpedo;
    propagate_const<buff_t *> chi_wave;
    propagate_const<buff_t *> dampen_harm;
    propagate_const<buff_t *> diffuse_magic;
    propagate_const<buff_t *> fatal_touch;
    propagate_const<buff_t *> jadefire_stomp;
    propagate_const<buff_t *> invokers_delight;
    propagate_const<buff_t *> rushing_jade_wind;
    propagate_const<buff_t *> spinning_crane_kick;
    propagate_const<buff_t *> windwalking_driver;
    propagate_const<absorb_buff_t *> yulons_grace;
    propagate_const<buff_t *> windwalking_movement_aura;
    propagate_const<buff_t *> chi_burst;

    // Brewmaster
    propagate_const<buff_t *> bladed_armor;
    propagate_const<buff_t *> blackout_combo;
    propagate_const<buff_t *> call_to_arms_invoke_niuzao;
    propagate_const<absorb_buff_t *> celestial_brew;
    propagate_const<buff_t *> celestial_flames;
    propagate_const<buff_t *> charred_passions;
    propagate_const<buff_t *> counterstrike;
    propagate_const<buff_t *> elusive_brawler;
    propagate_const<buff_t *> exploding_keg;
    propagate_const<buff_t *> fortifying_brew;
    propagate_const<buffs::gift_of_the_ox_t *> gift_of_the_ox;
    propagate_const<buff_t *> expel_harm_accumulator;
    propagate_const<buff_t *> hit_scheme;
    propagate_const<buff_t *> invoke_niuzao;
    propagate_const<buff_t *> press_the_advantage;
    propagate_const<buff_t *> pretense_of_instability;
    propagate_const<buff_t *> purified_chi;
    propagate_const<buffs::shuffle_t *> shuffle;
    propagate_const<buff_t *> training_of_niuzao;
    propagate_const<buff_t *> weapons_of_order;
    propagate_const<buff_t *> zen_meditation;
    // niuzao r2 recent purifies fake buff
    propagate_const<buff_t *> recent_purifies;
    propagate_const<buff_t *> ox_stance;

    // Mistweaver
    propagate_const<absorb_buff_t *> life_cocoon;
    propagate_const<buff_t *> channeling_soothing_mist;
    propagate_const<buff_t *> invoke_chiji;
    propagate_const<buff_t *> invoke_chiji_evm;
    propagate_const<buff_t *> lifecycles_enveloping_mist;
    propagate_const<buff_t *> lifecycles_vivify;
    propagate_const<buff_t *> mana_tea;
    propagate_const<buff_t *> refreshing_jade_wind;
    propagate_const<buff_t *> teachings_of_the_monastery;
    propagate_const<buff_t *> thunder_focus_tea;

    // Windwalker
    propagate_const<buff_t *> bok_proc;
    propagate_const<buff_t *> chi_energy;
    propagate_const<buff_t *> combat_wisdom;
    propagate_const<buff_t *> combo_strikes;
    propagate_const<buff_t *> cyclone_strikes;
    propagate_const<buff_t *> dance_of_chiji;
    propagate_const<buff_t *> dance_of_chiji_hidden;  // Used for trigger DoCJ ticks
    propagate_const<buff_t *> darting_hurricane;
    propagate_const<buff_t *> dizzying_kicks;
    propagate_const<buff_t *> dual_threat;
    propagate_const<buff_t *> ferociousness;
    propagate_const<buff_t *> flying_serpent_kick_movement;
    propagate_const<buff_t *> fury_of_xuen_stacks;
    propagate_const<buff_t *> fury_of_xuen;
    propagate_const<buff_t *> hidden_masters_forbidden_touch;
    propagate_const<buff_t *> hit_combo;
    propagate_const<buff_t *> flurry_of_xuen;
    propagate_const<buff_t *> invoke_xuen;
    propagate_const<buff_t *> jadefire_brand;
    propagate_const<buff_t *> martial_mixture;
    propagate_const<buff_t *> memory_of_the_monastery;
    propagate_const<buff_t *> momentum_boost_damage;
    propagate_const<buff_t *> momentum_boost_speed;
    propagate_const<buff_t *> ordered_elements;
    propagate_const<buff_t *> pressure_point;
    propagate_const<buff_t *> storm_earth_and_fire;
    propagate_const<buff_t *> the_emperors_capacitor;
    propagate_const<buff_t *> thunderfist;
    propagate_const<buff_t *> touch_of_death_ww;
    propagate_const<buff_t *> touch_of_karma;
    propagate_const<buff_t *> transfer_the_power;
    propagate_const<buff_t *> whirling_dragon_punch;

    // Conduit of the Celestials
    propagate_const<buff_t *> august_dynasty;
    propagate_const<buff_t *> celestial_conduit;
    propagate_const<buff_t *> chi_jis_swiftness;
    propagate_const<buff_t *> courage_of_the_white_tiger;
    propagate_const<buff_t *> flight_of_the_red_crane;
    propagate_const<buff_t *> heart_of_the_jade_serpent_stack_mw;
    propagate_const<buff_t *> heart_of_the_jade_serpent_stack_ww;
    propagate_const<buff_t *> heart_of_the_jade_serpent;
    propagate_const<buff_t *> heart_of_the_jade_serpent_cdr_celestial;
    propagate_const<buff_t *> heart_of_the_jade_serpent_cdr;
    propagate_const<buff_t *> inner_compass_crane_stance;
    propagate_const<buff_t *> inner_compass_ox_stance;
    propagate_const<buff_t *> inner_compass_serpent_stance;
    propagate_const<buff_t *> inner_compass_tiger_stance;
    propagate_const<buff_t *> jade_sanctuary;
    propagate_const<buff_t *> strength_of_the_black_ox;
    propagate_const<buff_t *> unity_within;

    // Master of Harmony
    buffs::aspect_of_harmony_t aspect_of_harmony;
    propagate_const<buff_t *> balanced_stratagem_physical;
    propagate_const<buff_t *> balanced_stratagem_magic;

    // Shado-Pan
    propagate_const<buff_t *> against_all_odds;
    propagate_const<buff_t *> flurry_charge;
    propagate_const<buff_t *> veterans_eye;
    propagate_const<buff_t *> vigilant_watch;
    propagate_const<buff_t *> wisdom_of_the_wall_crit;
    propagate_const<buff_t *> wisdom_of_the_wall_dodge;
    propagate_const<buff_t *> wisdom_of_the_wall_flurry;
    propagate_const<buff_t *> wisdom_of_the_wall_mastery;

    // TWW1 Set Bonus
    propagate_const<buff_t *> tiger_strikes;
    propagate_const<buff_t *> tigers_ferocity;
    propagate_const<buff_t *> flow_of_battle_damage;
    propagate_const<buff_t *> flow_of_battle_free_keg_smash;
  } buff;

  struct gains_t
  {
    propagate_const<gain_t *> black_ox_brew_energy;
    propagate_const<gain_t *> chi_refund;
    propagate_const<gain_t *> bok_proc;
    propagate_const<gain_t *> chi_burst;
    propagate_const<gain_t *> crackling_jade_lightning;
    propagate_const<gain_t *> energy_burst;
    propagate_const<gain_t *> energy_refund;
    propagate_const<gain_t *> energizing_elixir_chi;
    propagate_const<gain_t *> energizing_elixir_energy;
    propagate_const<gain_t *> expel_harm;
    propagate_const<gain_t *> focus_of_xuen;
    propagate_const<gain_t *> fortuitous_spheres;
    propagate_const<gain_t *> gift_of_the_ox;
    propagate_const<gain_t *> glory_of_the_dawn;
    propagate_const<gain_t *> healing_elixir;
    propagate_const<gain_t *> open_palm_strikes;
    propagate_const<gain_t *> ordered_elements;
    propagate_const<gain_t *> power_strikes;
    propagate_const<gain_t *> tiger_palm;
    propagate_const<gain_t *> touch_of_death_ww;
    propagate_const<gain_t *> weapons_of_order;
  } gain;

  struct procs_t
  {
    propagate_const<proc_t *> anvil__stave;
    propagate_const<proc_t *> blackout_combo_tiger_palm;
    propagate_const<proc_t *> blackout_combo_breath_of_fire;
    propagate_const<proc_t *> blackout_combo_keg_smash;
    propagate_const<proc_t *> blackout_combo_celestial_brew;
    propagate_const<proc_t *> blackout_combo_purifying_brew;
    propagate_const<proc_t *> blackout_combo_rising_sun_kick;
    propagate_const<proc_t *> blackout_kick_cdr_oe;
    propagate_const<proc_t *> blackout_kick_cdr;
    propagate_const<proc_t *> bountiful_brew_proc;
    propagate_const<proc_t *> charred_passions;
    propagate_const<proc_t *> chi_surge;
    propagate_const<proc_t *> counterstrike_tp;
    propagate_const<proc_t *> counterstrike_sck;
    propagate_const<proc_t *> elusive_footwork_proc;
    propagate_const<proc_t *> face_palm;
    propagate_const<proc_t *> glory_of_the_dawn;
    propagate_const<proc_t *> keg_smash_scalding_brew;
    propagate_const<proc_t *> quick_sip;
    propagate_const<proc_t *> rsk_reset_totm;
    propagate_const<proc_t *> salsalabims_strength;
    propagate_const<proc_t *> tranquil_spirit_expel_harm;
    propagate_const<proc_t *> tranquil_spirit_goto;
    propagate_const<proc_t *> xuens_battlegear_reduction;
    propagate_const<proc_t *> elusive_brawler_preserved;
  } proc;

  struct cooldowns_t
  {
    propagate_const<cooldown_t *> anvil__stave;
    propagate_const<cooldown_t *> blackout_kick;
    propagate_const<cooldown_t *> breath_of_fire;
    propagate_const<cooldown_t *> chi_torpedo;
    propagate_const<cooldown_t *> drinking_horn_cover;
    propagate_const<cooldown_t *> expel_harm;
    propagate_const<cooldown_t *> jadefire_stomp;
    propagate_const<cooldown_t *> fists_of_fury;
    propagate_const<cooldown_t *> flying_serpent_kick;
    propagate_const<cooldown_t *> healing_elixir;
    propagate_const<cooldown_t *> invoke_niuzao;
    propagate_const<cooldown_t *> invoke_xuen;
    propagate_const<cooldown_t *> invoke_yulon;
    propagate_const<cooldown_t *> keg_smash;
    propagate_const<cooldown_t *> rising_sun_kick;
    propagate_const<cooldown_t *> refreshing_jade_wind;
    propagate_const<cooldown_t *> roll;
    propagate_const<cooldown_t *> storm_earth_and_fire;
    propagate_const<cooldown_t *> strike_of_the_windlord;
    propagate_const<cooldown_t *> thunder_focus_tea;
    propagate_const<cooldown_t *> touch_of_death;
    propagate_const<cooldown_t *> weapons_of_order;
    propagate_const<cooldown_t *> whirling_dragon_punch;
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
      const spell_data_t *brewmasters_balance;
      const spell_data_t *celestial_fortune;
      const spell_data_t *celestial_fortune_heal;
      const spell_data_t *expel_harm_rank_2;
      const spell_data_t *blackout_kick;
      const spell_data_t *spinning_crane_kick;
      const spell_data_t *spinning_crane_kick_rank_2;
      const spell_data_t *touch_of_death_rank_3;

      const spell_data_t *stagger;
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
      const spell_data_t *expel_harm_rank_2;
    } mistweaver;

    struct
    {
      const spell_data_t *mastery;
      const spell_data_t *aura;
      const spell_data_t *aura_2;
      const spell_data_t *blackout_kick_rank_2;
      const spell_data_t *blackout_kick_rank_3;
      const spell_data_t *combo_breaker;
      const spell_data_t *combat_conditioning;
      const spell_data_t *empowered_tiger_lightning;
      const spell_data_t *flying_serpent_kick;
      const spell_data_t *mark_of_the_crane;
      const spell_data_t *touch_of_death_rank_3;
      const spell_data_t *touch_of_karma;
    } windwalker;
  } baseline;

  struct talents_t
  {
    struct
    {
      // Row 1
      player_talent_t soothing_mist;
      player_talent_t paralysis;
      player_talent_t rising_sun_kick;
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
      player_talent_t improved_detox;
      // Row 4
      player_talent_t vivacious_vivification;
      player_talent_t jade_walk;
      player_talent_t pressure_points;
      player_talent_t spear_hand_strike;
      player_talent_t ancient_arts;
      // Row 5
      player_talent_t chi_wave;
      const spell_data_t *chi_wave_buff;
      const spell_data_t *chi_wave_driver;
      const spell_data_t *chi_wave_damage;
      const spell_data_t *chi_wave_heal;
      player_talent_t chi_burst;
      const spell_data_t *chi_burst_buff;
      const spell_data_t *chi_burst_projectile;
      const spell_data_t *chi_burst_damage;
      const spell_data_t *chi_burst_heal;
      player_talent_t transcendence;
      player_talent_t energy_transfer;
      player_talent_t celerity;
      player_talent_t chi_torpedo;
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
      player_talent_t diffuse_magic;
      player_talent_t peace_and_prosperity;
      player_talent_t fortifying_brew;
      const spell_data_t *fortifying_brew_buff;
      player_talent_t dance_of_the_wind;
      player_talent_t dampen_harm;
      // Row 8
      player_talent_t save_them_all;
      player_talent_t swift_art;
      player_talent_t strength_of_spirit;
      player_talent_t profound_rebuttal;
      player_talent_t summon_black_ox_statue;
      player_talent_t summon_jade_serpent_statue;
      player_talent_t summon_white_tiger_statue;
      const spell_data_t *claw_of_the_white_tiger;
      player_talent_t ironshell_brew;
      player_talent_t expeditious_fortification;
      player_talent_t celestial_determination;
      // Row 9
      player_talent_t chi_proficiency;
      player_talent_t healing_winds;
      player_talent_t windwalking;
      player_talent_t bounce_back;
      player_talent_t martial_instincts;
      // Row 10
      player_talent_t lighter_than_air;
      player_talent_t flow_of_chi;
      player_talent_t escape_from_reality;
      player_talent_t transcendence_linked_spirits;
      player_talent_t fatal_touch;
      player_talent_t rushing_reflexes;
      player_talent_t clash;
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
      player_talent_t staggering_strikes;
      player_talent_t gift_of_the_ox;
      player_talent_t spirit_of_the_ox;
      const spell_data_t *gift_of_the_ox_buff;
      const spell_data_t *gift_of_the_ox_heal_trigger;
      const spell_data_t *gift_of_the_ox_heal_expire;
      player_talent_t quick_sip;
      // row 4
      player_talent_t hit_scheme;
      player_talent_t elixir_of_determination;
      player_talent_t special_delivery;
      const spell_data_t *special_delivery_missile;
      player_talent_t rushing_jade_wind;
      // row 5
      player_talent_t celestial_flames;
      player_talent_t celestial_brew;
      const spell_data_t *purified_chi;
      player_talent_t autumn_blessing;
      player_talent_t one_with_the_wind;
      player_talent_t zen_meditation;
      player_talent_t strike_at_dawn;
      // row 6
      player_talent_t breath_of_fire;
      const spell_data_t *breath_of_fire_dot;
      player_talent_t gai_plins_imperial_brew;
      const spell_data_t *gai_plins_imperial_brew_heal;
      player_talent_t invoke_niuzao_the_black_ox;
      const spell_data_t *invoke_niuzao_the_black_ox_stomp;
      player_talent_t tranquil_spirit;
      player_talent_t shadowboxing_treads;
      player_talent_t fluidity_of_motion;
      // row 7
      player_talent_t scalding_brew;
      player_talent_t salsalabims_strength;
      player_talent_t fortifying_brew_determination;
      player_talent_t bob_and_weave;
      player_talent_t black_ox_brew;
      player_talent_t walk_with_the_ox;
      player_talent_t light_brewing;
      player_talent_t training_of_niuzao;
      player_talent_t pretense_of_instability;
      player_talent_t counterstrike;
      // row 8
      player_talent_t dragonfire_brew;
      const spell_data_t *dragonfire_brew_hit;
      player_talent_t charred_passions;
      player_talent_t high_tolerance;
      player_talent_t exploding_keg;
      player_talent_t improved_invoke_niuzao_the_black_ox;
      player_talent_t elusive_footwork;
      player_talent_t anvil__stave;
      player_talent_t face_palm;
      // row 9
      player_talent_t ox_stance;
      const spell_data_t *ox_stance_buff;
      player_talent_t stormstouts_last_keg;
      player_talent_t blackout_combo;
      player_talent_t press_the_advantage;
      player_talent_t weapons_of_order;
      const spell_data_t *weapons_of_order_debuff;
      // row 10
      player_talent_t black_ox_adept;
      player_talent_t heightened_guard;
      player_talent_t call_to_arms;
      const spell_data_t *call_to_arms_buff;
      player_talent_t chi_surge;
    } brewmaster;

    // Mistweaver
    struct
    {
      // Row 1
      player_talent_t enveloping_mist;
      // Row 2
      player_talent_t thunder_focus_tea;
      player_talent_t renewing_mist;
      // Row 3
      player_talent_t life_cocoon;
      player_talent_t mana_tea;
      player_talent_t healing_elixir;
      // Row 4
      player_talent_t teachings_of_the_monastery;
      player_talent_t crane_style;
      player_talent_t revival;
      player_talent_t restoral;
      player_talent_t invigorating_mists;
      // 8 Required
      // Row 5
      player_talent_t nourishing_chi;
      player_talent_t calming_coalescence;
      player_talent_t uplifting_spirits;
      player_talent_t energizing_brew;
      player_talent_t lifecycles;
      player_talent_t zen_pulse;
      // Row 6
      player_talent_t mists_of_life;
      player_talent_t overflowing_mists;
      player_talent_t invoke_yulon_the_jade_serpent;
      player_talent_t invoke_chi_ji_the_red_crane;
      player_talent_t deep_clarity;
      player_talent_t rapid_diffusion;
      // Row 7
      player_talent_t chrysalis;
      player_talent_t burst_of_life;
      player_talent_t yulons_whisper;
      player_talent_t mist_wrap;
      player_talent_t refreshing_jade_wind;
      const spell_data_t *refreshing_jade_wind_tick;
      player_talent_t celestial_harmony;
      player_talent_t dancing_mists;
      player_talent_t chi_harmony;
      // 20 Required
      // Row 8
      player_talent_t jadefire_stomp;
      player_talent_t peer_into_peace;
      player_talent_t jade_bond;
      player_talent_t gift_of_the_celestials;
      player_talent_t focused_thunder;
      player_talent_t sheiluns_gift;
      // Row 9
      player_talent_t ancient_concordance;
      player_talent_t ancient_teachings;
      player_talent_t resplendent_mist;
      player_talent_t secret_infusion;
      player_talent_t misty_peaks;
      player_talent_t peaceful_mending;
      player_talent_t veil_of_pride;
      player_talent_t shaohaos_lessons;
      // Row 10
      player_talent_t awakened_jadefire;
      player_talent_t dance_of_chiji;
      player_talent_t tea_of_serenity;
      player_talent_t tea_of_plenty;
      player_talent_t unison;
      player_talent_t mending_proliferation;
      player_talent_t invokers_delight;
      player_talent_t tear_of_morning;
      player_talent_t rising_mist;
      player_talent_t legacy_of_wisdom;
    } mistweaver;

    // Windwalker
    struct
    {
      // Row 1
      player_talent_t fists_of_fury;
      // Row 2
      player_talent_t momentum_boost;
      player_talent_t combat_wisdom;
      const spell_data_t *combat_wisdom_expel_harm;
      player_talent_t acclamation;
      // Row 3
      player_talent_t touch_of_the_tiger;
      player_talent_t hardened_soles;
      player_talent_t ascension;
      player_talent_t ferociousness;
      // Row 4
      player_talent_t crane_vortex;
      player_talent_t teachings_of_the_monastery;
      const spell_data_t *teachings_of_the_monastery_blackout_kick;
      player_talent_t glory_of_the_dawn;
      // 8 Required
      // Row 5
      player_talent_t jade_ignition;
      player_talent_t courageous_impulse;
      player_talent_t storm_earth_and_fire;
      player_talent_t flurry_of_xuen;
      player_talent_t hit_combo;
      player_talent_t brawlers_intensity;
      player_talent_t meridian_strikes;
      // Row 6
      player_talent_t dance_of_chiji;
      player_talent_t drinking_horn_cover;
      player_talent_t spiritual_focus;
      player_talent_t ordered_elements;
      player_talent_t strike_of_the_windlord;
      // Row 7
      player_talent_t martial_mixture;
      player_talent_t energy_burst;
      player_talent_t shadowboxing_treads;
      player_talent_t invoke_xuen_the_white_tiger;
      player_talent_t inner_peace;
      player_talent_t rushing_jade_wind;
      player_talent_t thunderfist;
      // 20 Required
      // Row 8
      player_talent_t sequenced_strikes;
      player_talent_t rising_star;
      player_talent_t invokers_delight;
      player_talent_t dual_threat;
      player_talent_t gale_force;
      const spell_data_t *gale_force_damage;
      // Row 9
      player_talent_t last_emperors_capacitor;
      player_talent_t whirling_dragon_punch;
      const spell_data_t *whirling_dragon_punch_buff;
      player_talent_t xuens_bond;
      player_talent_t xuens_battlegear;
      player_talent_t transfer_the_power;
      player_talent_t jadefire_fists;
      player_talent_t jadefire_stomp;
      const spell_data_t *jadefire_stomp_damage;
      const spell_data_t *jadefire_stomp_ww_damage;
      player_talent_t communion_with_wind;
      // Row 10
      player_talent_t power_of_the_thunder_king;
      player_talent_t revolving_whirl;
      player_talent_t knowledge_of_the_broken_temple;
      player_talent_t memory_of_the_monastery;
      player_talent_t fury_of_xuen;
      player_talent_t path_of_jade;
      player_talent_t singularly_focused_jade;
      player_talent_t jadefire_harmony;
      const spell_data_t *jadefire_brand_dmg;
      const spell_data_t *jadefire_brand_heal;
      player_talent_t darting_hurricane;
    } windwalker;

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
      // Row 3
      player_talent_t tigers_vigor;
      player_talent_t roar_from_the_heavens;
      player_talent_t endless_draught;
      player_talent_t mantra_of_purity;
      player_talent_t mantra_of_tenacity;
      // Row 4
      player_talent_t overwhelming_force;
      const spell_data_t *overwhelming_force_damage;
      player_talent_t path_of_resurgence;
      player_talent_t way_of_a_thousand_strikes;
      player_talent_t clarity_of_purpose;
      // Row 5
      player_talent_t coalescence;
    } master_of_harmony;

    // Shado-Pan
    struct
    {
      // Row 1
      player_talent_t flurry_strikes;
      const spell_data_t *flurry_strikes_hit;
      // Row 2
      player_talent_t pride_of_pandaria;
      player_talent_t high_impact;
      const spell_data_t *high_impact_debuff;
      player_talent_t veterans_eye;
      player_talent_t martial_precision;
      // Row 3
      player_talent_t protect_and_serve;
      player_talent_t lead_from_the_front;
      player_talent_t one_versus_many;
      player_talent_t whirling_steel;
      player_talent_t predictive_training;
      // Row 4
      player_talent_t against_all_odds;
      player_talent_t efficient_training;
      player_talent_t vigilant_watch;
      // Row 5
      player_talent_t wisdom_of_the_wall;
      const spell_data_t *wisdom_of_the_wall_flurry;
    } shado_pan;

    // Conduit of the Celestials
    struct
    {
      // Row 1
      player_talent_t celestial_conduit;
      const spell_data_t *celestial_conduit_dmg;
      const spell_data_t *celestial_conduit_heal;
      // Row 2
      player_talent_t temple_training;
      player_talent_t xuens_guidance;
      player_talent_t courage_of_the_white_tiger;
      const spell_data_t *courage_of_the_white_tiger_damage;
      const spell_data_t *courage_of_the_white_tiger_heal;
      player_talent_t restore_balance;
      player_talent_t yulons_knowledge;
      // Row 3
      player_talent_t heart_of_the_jade_serpent;
      player_talent_t strength_of_the_black_ox;
      const spell_data_t *strength_of_the_black_ox_absorb;
      const spell_data_t *strength_of_the_black_ox_damage;
      player_talent_t flight_of_the_red_crane;
      const spell_data_t *flight_of_the_red_crane_dmg;
      const spell_data_t *flight_of_the_red_crane_heal;
      const spell_data_t *flight_of_the_red_crane_celestial_dmg;
      const spell_data_t *flight_of_the_red_crane_celestial_heal;
      // Row 4
      player_talent_t niuzaos_protection;
      player_talent_t jade_sanctuary;
      player_talent_t chi_jis_swiftness;
      player_talent_t inner_compass;
      player_talent_t august_dynasty;
      // Row 5
      player_talent_t unity_within;
      const spell_data_t *unity_within_dmg_mult;
    } conduit_of_the_celestials;
  } talent;

  struct
  {
    struct
    {
      const spell_data_t *ww_4pc;
      const spell_data_t *ww_4pc_dmg;
      const spell_data_t *brm_4pc_damage_buff;
      const spell_data_t *brm_4pc_free_keg_smash_buff;
    } tww1;

    struct
    {
      const spell_data_t *ww_2pc;
      const spell_data_t *ww_2pc_winning_streak;
      const spell_data_t *ww_4pc;
      const spell_data_t *ww_4pc_cashout;
      propagate_const<buff_t *> winning_streak;
      propagate_const<buff_t *> cashout;

      const spell_data_t *brm_2pc;
      const spell_data_t *brm_2pc_luck_of_the_draw;
      const spell_data_t *brm_4pc;
      const spell_data_t *brm_4pc_opportunistic_strike;
      propagate_const<buff_t *> luck_of_the_draw;
      propagate_const<buff_t *> opportunistic_strike;
    } tww2;
  } tier;

  struct pets_t
  {
    std::array<pets::storm_earth_and_fire_pet_t *, (int)pets::sef_pet_e::SEF_PET_MAX> sef;
    spawner::pet_spawner_t<pet_t, monk_t> xuen;
    spawner::pet_spawner_t<pet_t, monk_t> niuzao;
    spawner::pet_spawner_t<pet_t, monk_t> yulon;
    spawner::pet_spawner_t<pet_t, monk_t> chiji;
    spawner::pet_spawner_t<pet_t, monk_t> white_tiger_statue;
    spawner::pet_spawner_t<pet_t, monk_t> fury_of_xuen_tiger;
    spawner::pet_spawner_t<pet_t, monk_t> call_to_arms_niuzao;

    pet_t *bron;

    pets_t( monk_t *p );
  } pets;

  // Options
  struct options_t
  {
    int initial_chi;
    double expel_harm_effectiveness;
    double jadefire_stomp_uptime;
    int chi_burst_healing_targets;
    int motc_override;
    double squirm_frequency;

    int shado_pan_initial_charge_accumulator;
  } user_options;

  // exterminate these structs
  struct
  {
    const spell_data_t *jadefire_stomp;
    const spell_data_t *healing_elixir;
    const spell_data_t *invokers_delight;
    const spell_data_t *rushing_jade_wind;
    const spell_data_t *teachings_of_the_monastery;
  } shared;

  struct
  {
    // General
    const spell_data_t *rushing_jade_wind;
    const spell_data_t *rushing_jade_wind_tick;

    // Windwalker
    const spell_data_t *bok_proc;
    const spell_data_t *chi_explosion;
    const spell_data_t *crackling_tiger_lightning;
    const spell_data_t *crackling_tiger_lightning_driver;
    const spell_data_t *combat_wisdom_expel_harm;
    const spell_data_t *cyclone_strikes;
    const spell_data_t *dance_of_chiji;
    const spell_data_t *dance_of_chiji_bug;
    const spell_data_t *dual_threat_kick;
    const spell_data_t *dizzying_kicks;
    const spell_data_t *empowered_tiger_lightning;
    const spell_data_t *jadefire_brand_dmg;
    const spell_data_t *jadefire_brand_heal;
    const spell_data_t *fists_of_fury_tick;
    const spell_data_t *flurry_of_xuen_driver;
    const spell_data_t *flying_serpent_kick_damage;
    const spell_data_t *focus_of_xuen;
    const spell_data_t *fury_of_xuen_stacking_buff;
    const spell_data_t *fury_of_xuen;
    const spell_data_t *glory_of_the_dawn_damage;
    const spell_data_t *hidden_masters_forbidden_touch;
    const spell_data_t *hit_combo;
    const spell_data_t *improved_touch_of_death;
    const spell_data_t *mark_of_the_crane;
    const spell_data_t *summon_white_tiger_statue;
    const spell_data_t *power_strikes_chi;
    const spell_data_t *thunderfist;
    const spell_data_t *touch_of_karma_tick;
    const spell_data_t *whirling_dragon_punch_aoe_tick;
    const spell_data_t *whirling_dragon_punch_st_tick;
  } passives;

public:
  monk_t( sim_t *sim, util::string_view name, race_e r );

  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;
  action_t *create_action( util::string_view name, util::string_view options ) override;
  double composite_attack_power_multiplier() const override;
  double composite_dodge() const override;
  double non_stacking_movement_modifier() const override;
  double composite_player_target_armor( player_t *target ) const override;
  void create_pets() override;
  void init_spells() override;
  void init_background_actions() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void create_actions() override;
  void init_gains() override;
  void init_procs() override;
  void init_assessors() override;
  void init_special_effects() override;
  void init_special_effect( special_effect_t &effect ) override;
  void init_finished() override;
  void reset() override;
  void create_options() override;
  void copy_from( player_t * ) override;
  resource_e primary_resource() const override;
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  void combat_begin() override;
  void target_mitigation( school_e, result_amount_type, action_state_t * ) override;
  void assess_damage( school_e, result_amount_type, action_state_t *s ) override;
  void assess_heal( school_e, result_amount_type, action_state_t *s ) override;
  void invalidate_cache( cache_e ) override;
  void init_action_list() override;
  void activate() override;
  void collect_resource_timeline_information() override;
  bool validate_fight_style( fight_style_e style ) const override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
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

  // Custom Monk Functions
  void parse_player_effects();
  void create_proc_callback( const spell_data_t *effect_driver,
                             bool ( *trigger )( monk_t *player, action_state_t *state ), proc_flag PF_OVERRIDE,
                             proc_flag2 PF2_OVERRIDE, action_t *proc_action_override = nullptr );
  void create_proc_callback( const spell_data_t *effect_driver,
                             bool ( *trigger )( monk_t *player, action_state_t *state ),
                             action_t *proc_action_override = nullptr );
  void create_proc_callback( const spell_data_t *effect_driver,
                             bool ( *trigger )( monk_t *player, action_state_t *state ), proc_flag PF_OVERRIDE,
                             action_t *proc_action_override = nullptr );
  void create_proc_callback( const spell_data_t *effect_driver,
                             bool ( *trigger )( monk_t *player, action_state_t *state ), proc_flag2 PF2_OVERRIDE,
                             action_t *proc_action_override = nullptr );
  void trigger_celestial_fortune( action_state_t * );
  void trigger_mark_of_the_crane( action_state_t * );
  void trigger_empowered_tiger_lightning( action_state_t * );
  player_t *next_mark_of_the_crane_target( action_state_t * );
  int mark_of_the_crane_counter();
  bool mark_of_the_crane_max();
  double sck_modifier();
  double calculate_last_stagger_tick_damage( int n ) const;
  bool affected_by_sef( spell_data_t data ) const;  // Custom handler for SEF bugs

  // Storm Earth and Fire targeting logic
  std::vector<player_t *> create_storm_earth_and_fire_target_list() const;
  void summon_storm_earth_and_fire( timespan_t duration );
  void retarget_storm_earth_and_fire( pet_t *pet, std::vector<player_t *> &targets ) const;
  void retarget_storm_earth_and_fire_pets() const;

  void trigger_storm_earth_and_fire( const action_t *a, actions::sef_ability_e sef_ability, bool combo_strike );
  void storm_earth_and_fire_fixate( player_t *target );
  bool storm_earth_and_fire_fixate_ready( player_t *target );
  player_t *storm_earth_and_fire_fixate_target( pets::sef_pet_e sef_pet );
  void trigger_storm_earth_and_fire_bok_proc( pets::sef_pet_e sef_pet );
};

struct sef_despawn_cb_t
{
  monk_t *monk;

  sef_despawn_cb_t( monk_t *m ) : monk( m )
  {
  }

  void operator()( player_t * );
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
}  // namespace events

}  // namespace monk
