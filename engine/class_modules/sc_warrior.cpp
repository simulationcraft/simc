// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc/specialization.hpp"
#include "simulationcraft.hpp"
#include "player/player_talent_points.hpp"
#include "class_modules/apl/apl_warrior.hpp"
#include "action/parse_effects.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Warrior
// To Do: Clean up green text
// Fury - Gathering Storm tick behavior - Fury needs 2 more
// Arms - 
// ==========================================================================

struct warrior_t;

// Finds an action with the given name. If no action exists, a new one will
// be created.
//
// Use this with secondary background actions to ensure the player only has
// one copy of the action.
// Shamelessly borrowed from the mage module
template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

template <typename V>
  static const spell_data_t* resolve_spell_data( V data )
  {
    if constexpr (std::is_invocable_v<decltype( &spell_data_t::ok ), V>)
      return data;
    else if constexpr (std::is_invocable_v<decltype( &buff_t::data ), V>)
      return &data->data();
    else if constexpr (std::is_invocable_v<decltype( &action_t::data ), V>)
      return &data->data();

    assert( false && "Could not resolve find_effect argument to spell data." );
    return nullptr;
  }

  // finds a spell effect
  // 1) first argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
  // 2) if the second argument is player_talent_t, spell_data_t*, buff_t*, or action_t* then only effects that affect it are returned
  // 3) if the third (or second if the above does not apply) argument is effect subtype, then the type is assumed to be E_APPLY_AURA
  // further arguments can be given to filter for full type + subtype + property
  template <typename T, typename U, typename... Ts>
  static const spelleffect_data_t& find_effect( T val, U type, Ts&&... args )
  {
    const spell_data_t* data = resolve_spell_data<T>( val );

    if constexpr (std::is_same_v<U, effect_subtype_t>)
      return spell_data_t::find_spelleffect( *data, E_APPLY_AURA, type, std::forward<Ts>( args )... );
    else if constexpr (std::is_same_v<U, effect_type_t>)
      return spell_data_t::find_spelleffect( *data, type, std::forward<Ts>( args )... );
    else
    {
      const spell_data_t* affected = resolve_spell_data<U>( type );

      if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_subtype_t>)
        return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA, std::forward<Ts>( args )... );
      else if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Ts...>>, effect_type_t>)
        return spell_data_t::find_spelleffect( *data, *affected, std::forward<Ts>( args )... );
      else
        return spell_data_t::find_spelleffect( *data, *affected, E_APPLY_AURA );
    }

    assert( false && "Could not resolve find_effect argument to type/subtype." );
    return spelleffect_data_t::nil();
  }

  template <typename T, typename U, typename... Ts>
  static size_t find_effect_index( T val, U type, Ts&&... args )
  {
    return find_effect( val, type, std::forward<Ts>( args )... ).index() + 1;
  }

  // finds the first effect with a trigger spell
  // argument can be either player_talent_t, spell_data_t*, buff_t*, action_t*
  template <typename T>
  static const spelleffect_data_t& find_trigger( T val )
  {
    const spell_data_t* data = resolve_spell_data<T>( val );

    for (const auto& eff : data->effects())
    {
      switch (eff.type())
      {
        case E_TRIGGER_SPELL:
        case E_TRIGGER_SPELL_WITH_VALUE:
          return eff;
        case E_APPLY_AURA:
        case E_APPLY_AREA_AURA_PARTY:
          switch (eff.subtype())
          {
            case A_PROC_TRIGGER_SPELL:
            case A_PROC_TRIGGER_SPELL_WITH_VALUE:
            case A_PERIODIC_TRIGGER_SPELL:
            case A_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
              return eff;
            default:
              break;
          }
          break;
        default:
          break;
      }
    }

    return spelleffect_data_t::nil();
  }

struct warrior_td_t : public actor_target_data_t
{
  dot_t* dots_deep_wounds;
  dot_t* dots_gushing_wound;
  dot_t* dots_ravager;
  dot_t* dots_rend;
  dot_t* dots_thunderous_roar;
  buff_t* debuffs_colossus_smash;
  buff_t* debuffs_concussive_blows;
  buff_t* debuffs_champions_might;
  buff_t* debuffs_executioners_precision;
  buff_t* debuffs_fatal_mark;
  buff_t* debuffs_skullsplitter;
  buff_t* debuffs_demoralizing_shout;
  buff_t* debuffs_taunt;
  buff_t* debuffs_punish;
  buff_t* debuffs_callous_reprisal;
  buff_t* debuffs_marked_for_execution;
  buff_t* debuffs_overwhelmed;
  buff_t* debuffs_wrecked;  // Dominance of the Colossus
  bool hit_by_fresh_meat;

  warrior_t& warrior;
  warrior_td_t( player_t* target, warrior_t& p );

  void target_demise();
};

// utility to create target_effect_t compatible functions from warrior_td_t member references
template <typename T>
static std::function<int( actor_target_data_t* )> d_fn( T d, bool stack = true )
{
  if constexpr ( std::is_invocable_v<decltype( &buff_t::check ), std::invoke_result_t<T, warrior_td_t>> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<warrior_td_t*>( t ) )->check();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<warrior_td_t*>( t ) )->check() > 0;
      };
  }
  else if constexpr ( std::is_invocable_v<decltype( &dot_t::is_ticking ), std::invoke_result_t<T, warrior_td_t>> )
  {
    if ( stack )
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<warrior_td_t*>( t ) )->current_stack();
      };
    else
      return [ d ]( actor_target_data_t* t ) {
        return std::invoke( d, static_cast<warrior_td_t*>( t ) )->is_ticking();
      };
  }
  else
  {
    static_assert( static_false<T>, "Not a valid member of warrior_td_t" );
    return nullptr;
  }
}

using data_t        = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

template <typename T_CONTAINER, typename T_DATA>
T_CONTAINER* get_data_entry( util::string_view name, std::vector<T_DATA*>& entries )
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

struct warrior_t : public parse_player_effects_t
{
public:
  std::vector<attack_t*> rampage_attacks;
  bool non_dps_mechanics, warrior_fixed_time;
  int into_the_fray_friends;
  int never_surrender_percentage;
  bool first_rampage_attack_missed;

  auto_dispose<std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*> > cd_waste_iter;

  // Active
  struct active_t
  {
    action_t* deep_wounds_ARMS;
    action_t* deep_wounds_PROT;
    action_t* fatality;
    action_t* torment_avatar;
    action_t* torment_odyns_fury;
    action_t* torment_recklessness;
    action_t* tough_as_nails;
    action_t* slayers_strike;
  } active;

  // Buffs
  struct buffs_t
  {
    buff_t* ashen_juggernaut;
    buff_t* avatar;
    buff_t* battle_stance;
    buff_t* battering_ram;
    buff_t* berserker_rage;
    buff_t* berserker_stance;
    buff_t* bladestorm;
    buff_t* bloodbath;
    buff_t* bloodcraze;
    buff_t* bounding_stride;
    buff_t* brace_for_impact;
    buff_t* charge_movement;
    buff_t* collateral_damage;
    buff_t* concussive_blows;
    buff_t* crushing_blow;
    buff_t* dance_of_death_bladestorm;
    buff_t* dance_of_death_ravager;
    buff_t* dancing_blades;
    buff_t* defensive_stance;
    buff_t* die_by_the_sword;
    buff_t* enrage;
    buff_t* frenzy;
    buff_t* heroic_leap_movement;
    buff_t* ignore_pain;
    buff_t* in_for_the_kill;
    buff_t* intercept_movement;
    buff_t* intervene_movement;
    buff_t* into_the_fray;
    buff_t* juggernaut;
    buff_t* juggernaut_prot;
    buff_t* last_stand;
    buff_t* meat_cleaver;
    buff_t* martial_prowess;
    buff_t* merciless_bonegrinder;
    buff_t* ravager;
    buff_t* recklessness;
    buff_t* recklessness_warlords_torment;
    buff_t* revenge;
    buff_t* seismic_reverberation_revenge;
    buff_t* shield_block;
    buff_t* shield_charge_movement;
    buff_t* shield_wall;
    buff_t* show_of_force;
    buff_t* slaughtering_strikes;
    buff_t* spell_reflection;
    buff_t* storm_of_swords;
    buff_t* sudden_death;
    buff_t* sweeping_strikes;
    buff_t* test_of_might_tracker;  // Used to track rage gain from test of might.
    buff_t* test_of_might;
    buff_t* unnerving_focus;
    buff_t* whirlwind;
    buff_t* wild_strikes;

    buff_t* seeing_red;
    buff_t* seeing_red_tracking;
    buff_t* violent_outburst;

    // Covenant
    buff_t* conquerors_banner;
    buff_t* conquerors_frenzy;
    buff_t* conquerors_mastery;

    // Colossus
    buff_t* colossal_might;

    // Slayer
    buff_t* imminent_demise;
    buff_t* brutal_finish;
    buff_t* fierce_followthrough;
    buff_t* opportunist;

    // Mountain Thane
    buff_t* thunder_blast;
    buff_t* steadfast_as_the_peaks;
    buff_t* burst_of_power;

    // DF Tier
    buff_t* strike_vulnerabilities;
    buff_t* vanguards_determination;
    buff_t* crushing_advance;
    buff_t* merciless_assault;
    buff_t* earthen_tenacity;    // T30 Protection 4PC
    buff_t* furious_bloodthirst; // T31 Fury 2PC
    buff_t* fervid;              // T31 Prot 2pc proc
    buff_t* fervid_opposition;   // T31 2pc DR buff

    // TWW1 Tier
    buff_t* overpowering_might; // Arms 2pc
    buff_t* lethal_blows;       // Arms 4pc
    buff_t* bloody_rampage;     // Fury 2pc
    buff_t* deep_thirst;        // Fury 4pc
    buff_t* expert_strategist;  // Prot 2pc
    buff_t* brutal_followup;    // Prot 4pc
  } buff;

  struct rppm_t
  {
    real_ppm_t* fatal_mark;
    real_ppm_t* revenge;
    real_ppm_t* sudden_death;
    real_ppm_t* t31_sudden_death;
    real_ppm_t* slayers_dominance;
  } rppm;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* avatar;
    cooldown_t* recklessness;
    cooldown_t* berserker_rage;
    cooldown_t* bladestorm;
    cooldown_t* bloodthirst;
    cooldown_t* bloodbath;
    cooldown_t* cleave;
    cooldown_t* colossus_smash;
    cooldown_t* demoralizing_shout;
    cooldown_t* thunderous_roar;
    cooldown_t* enraged_regeneration;
    cooldown_t* execute;
    cooldown_t* heroic_leap;
    cooldown_t* iron_fortress_icd;
    cooldown_t* impending_victory;
    cooldown_t* last_stand;
    cooldown_t* mortal_strike;
    cooldown_t* odyns_fury;
    cooldown_t* onslaught;
    cooldown_t* overpower;
    cooldown_t* pummel;
    cooldown_t* rage_from_auto_attack;
    cooldown_t* rage_from_crit_block;
    cooldown_t* rage_of_the_valarjar_icd;
    cooldown_t* raging_blow;
    cooldown_t* crushing_blow;
    cooldown_t* ravager;
    cooldown_t* shield_slam;
    cooldown_t* shield_wall;
    cooldown_t* single_minded_fury_icd;
    cooldown_t* shockwave;
    cooldown_t* skullsplitter;
    cooldown_t* storm_bolt;
    cooldown_t* sudden_death_icd;
    cooldown_t* tough_as_nails_icd;
    cooldown_t* thunder_clap;
    cooldown_t* warbreaker;
    cooldown_t* conquerors_banner;
    cooldown_t* champions_spear;
    cooldown_t* berserkers_torment;
    cooldown_t* cold_steel_hot_blood_icd;
    cooldown_t* reap_the_storm_icd;
    cooldown_t* demolish;
    cooldown_t* t31_fury_4pc_icd;
    cooldown_t* burst_of_power_icd;
  } cooldown;

  // Gains
  struct gains_t
  {
    gain_t* archavons_heavy_hand;
    gain_t* avatar;
    gain_t* avatar_torment;
    gain_t* avoided_attacks;
    gain_t* battlelord;
    gain_t* bloodsurge;
    gain_t* charge;
    gain_t* critical_block;
    gain_t* execute;
    gain_t* frothing_berserker;
    gain_t* meat_cleaver;
    gain_t* melee_crit;
    gain_t* melee_main_hand;
    gain_t* melee_off_hand;
    gain_t* raging_blow;
    gain_t* ravager;
    gain_t* revenge;
    gain_t* shield_charge;
    gain_t* shield_slam;
    gain_t* storm_of_steel;
    gain_t* champions_spear;
    gain_t* finishing_blows;
    gain_t* whirlwind;
    gain_t* booming_voice;
    gain_t* thunder_blast;
    gain_t* thunder_clap;
    gain_t* endless_rage;
    gain_t* instigate;
    gain_t* war_machine_demise;
    gain_t* merciless_assault;
    gain_t* thorims_might;
    gain_t* burst_of_power;

    // Legendarys, Azerite, and Special Effects
    gain_t* execute_refund;
    gain_t* rage_from_damage_taken;
    gain_t* ceannar_rage;
    gain_t* cold_steel_hot_blood;
    gain_t* valarjar_berserking;
    gain_t* lord_of_war;
    gain_t* simmering_rage;
    gain_t* conquerors_banner;
  } gain;

  // Spells
  struct spells_t
  {
    // Core Class Spells
    const spell_data_t* battle_shout;
    const spell_data_t* berserker_rage;
    const spell_data_t* charge;
    const spell_data_t* execute;
    const spell_data_t* execute_rage_refund;
    const spell_data_t* hamstring;
    const spell_data_t* heroic_throw;
    const spell_data_t* pummel;
    const spell_data_t* shield_block;
    const spell_data_t* shield_slam;
    const spell_data_t* slam;
    const spell_data_t* taunt;
    const spell_data_t* victory_rush;
    const spell_data_t* whirlwind;

    // Arms
    const spell_data_t* deep_wounds_arms;

    // Fury

    // Protection
    const spell_data_t* seismic_reverberation_revenge;

    // Extra Spells To Make Things Work

    const spell_data_t* colossus_smash_debuff;
    const spell_data_t* dance_of_death;
    const spell_data_t* dance_of_death_bs_buff; // Bladestorm
    const spell_data_t* fatal_mark_debuff;
    const spell_data_t* concussive_blows_debuff;
    const spell_data_t* recklessness_buff;
    const spell_data_t* shield_block_buff;
    const spell_data_t* whirlwind_buff;
    const spell_data_t* aftershock_duration;
    const spell_data_t* shield_wall;
    const spell_data_t* sudden_death_arms;
    const spell_data_t* sudden_death_fury;

    // DF Tier
    // T31
    const spell_data_t* furious_bloodthirst;
    const spell_data_t* t31_fury_4pc;

    // Colossus
    const spell_data_t* wrecked_debuff;

    // Slayer
    const spell_data_t* marked_for_execution_debuff;
    const spell_data_t* slayers_strike;
    const spell_data_t* overwhelmed_debuff;
    const spell_data_t* reap_the_storm;

    // Mountain Thane
    const spell_data_t* lightning_strike;
  } spell;

  // Mastery
  struct mastery_t
  {
    const spell_data_t* deep_wounds_ARMS;  // Arms
    const spell_data_t* critical_block;    // Protection
    const spell_data_t* unshackled_fury;   // Fury
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* battlelord;
    proc_t* battlelord_wasted;
    proc_t* delayed_auto_attack;
    proc_t* tactician;
  } proc;

  // Spec Passives
  struct spec_t
  {
    // Class Aura
    const spell_data_t* warrior;
    const spell_data_t* warrior_2;

    // Arms Spells
    const spell_data_t* arms_warrior;
    const spell_data_t* arms_warrior_2;
    const spell_data_t* seasoned_soldier;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* deep_wounds_ARMS;

    // Fury Spells
    const spell_data_t* fury_warrior;
    const spell_data_t* fury_warrior_2;
    const spell_data_t* enrage;
    const spell_data_t* execute;
    const spell_data_t* whirlwind;

    const spell_data_t* bloodbath; // BT replacement
    const spell_data_t* crushing_blow; // RB replacement

    // Protection Spells
    const spell_data_t* protection_warrior;
    const spell_data_t* protection_warrior_2;
    const spell_data_t* devastate;
    const spell_data_t* riposte;
    const spell_data_t* vanguard;

    const spell_data_t* deep_wounds_PROT;
    const spell_data_t* revenge_trigger;
    const spell_data_t* shield_block_2;

  } spec;

  // Talents
  struct talents_t
  {
    struct class_talents_t
    {
      player_talent_t battle_stance;
      player_talent_t berserker_stance;
      player_talent_t defensive_stance;

      player_talent_t impending_victory;
      player_talent_t war_machine;
      player_talent_t intervene;
      player_talent_t rallying_cry;

      player_talent_t berserker_shout;
      player_talent_t piercing_howl;
      player_talent_t fast_footwork;
      player_talent_t spell_reflection;
      player_talent_t leeching_strikes;
      player_talent_t inspiring_presence;
      player_talent_t second_wind;

      player_talent_t frothing_berserker;
      player_talent_t heroic_leap;
      player_talent_t intimidating_shout;
      player_talent_t thunder_clap;

      player_talent_t wrecking_throw;
      player_talent_t shattering_throw;
      player_talent_t crushing_force;
      player_talent_t pain_and_gain;
      player_talent_t cacophonous_roar;
      player_talent_t menace;
      player_talent_t storm_bolt;
      player_talent_t overwhelming_rage;
      player_talent_t barbaric_training;
      player_talent_t concussive_blows;

      player_talent_t reinforced_plates;
      player_talent_t bounding_stride;
      player_talent_t crackling_thunder;
      player_talent_t sidearm;

      player_talent_t honed_reflexes;
      player_talent_t bitter_immunity;
      player_talent_t double_time;
      player_talent_t seismic_reverberation;

      player_talent_t armored_to_the_teeth;
      player_talent_t wild_strikes;
      player_talent_t one_handed_weapon_specialization;
      player_talent_t two_handed_weapon_specialization;
      player_talent_t dual_wield_specialization;
      player_talent_t cruel_strikes;
      player_talent_t endurance_training;

      player_talent_t avatar;
      player_talent_t thunderous_roar;
      player_talent_t champions_spear;
      player_talent_t shockwave;

      player_talent_t immovable_object;
      player_talent_t unstoppable_force;
      player_talent_t blademasters_torment;
      player_talent_t warlords_torment;
      player_talent_t berserkers_torment;
      player_talent_t titans_torment;
      player_talent_t uproar;
      player_talent_t thunderous_words;
      player_talent_t piercing_challenge;
      player_talent_t champions_might;
      player_talent_t rumbling_earth;

    } warrior;

    struct arms_talents_t
    {
      player_talent_t mortal_strike;

      player_talent_t overpower;

      player_talent_t martial_prowess;
      player_talent_t die_by_the_sword;
      player_talent_t improved_execute;

      player_talent_t improved_overpower;
      player_talent_t bloodsurge;
      player_talent_t fueled_by_violence;
      player_talent_t storm_wall;
      player_talent_t ignore_pain;
      player_talent_t sudden_death;
      player_talent_t fervor_of_battle;

      player_talent_t tactician;
      player_talent_t colossus_smash;
      player_talent_t impale;

      player_talent_t skullsplitter;
      player_talent_t rend;
      player_talent_t finishing_blows;
      player_talent_t anger_management;
      player_talent_t spiteful_serenity;
      player_talent_t exhilarating_blows;
      player_talent_t improved_sweeping_strikes;
      player_talent_t collateral_damage;
      player_talent_t cleave;

      player_talent_t bloodborne;
      player_talent_t dreadnaught;
      player_talent_t strength_of_arms;
      player_talent_t in_for_the_kill;
      player_talent_t test_of_might;
      player_talent_t blunt_instruments;
      player_talent_t warbreaker;
      player_talent_t massacre;
      player_talent_t storm_of_swords;

      player_talent_t deft_experience;
      player_talent_t valor_in_victory;
      player_talent_t critical_thinking;

      player_talent_t battlelord;
      player_talent_t bloodletting;
      player_talent_t bladestorm;
      player_talent_t ravager;
      player_talent_t sharpened_blades;
      player_talent_t juggernaut;

      player_talent_t fatality;
      player_talent_t dance_of_death;
      player_talent_t unhinged;
      player_talent_t merciless_bonegrinder;
      player_talent_t executioners_precision;
    } arms;

    struct fury_talents_t
    {
      player_talent_t bloodthirst;

      player_talent_t raging_blow;

      player_talent_t frenzied_enrage;
      player_talent_t powerful_enrage;
      player_talent_t enraged_regeneration;
      player_talent_t improved_execute;

      player_talent_t improved_bloodthirst;
      player_talent_t fresh_meat;
      player_talent_t warpaint;
      player_talent_t invigorating_fury;
      player_talent_t sudden_death;
      player_talent_t cruelty;

      player_talent_t focus_in_chaos;
      player_talent_t rampage;
      player_talent_t improved_raging_blow;

      player_talent_t single_minded_fury;
      player_talent_t cold_steel_hot_blood;
      player_talent_t vicious_contempt;
      player_talent_t frenzy;
      player_talent_t hack_and_slash;
      player_talent_t slaughtering_strikes;
      player_talent_t ashen_juggernaut;
      player_talent_t improved_whirlwind;

      player_talent_t bloodborne;
      player_talent_t bloodcraze;
      player_talent_t recklessness;
      player_talent_t massacre;
      player_talent_t wrath_and_fury;
      player_talent_t meat_cleaver;

      player_talent_t deft_experience;
      player_talent_t swift_strikes;
      player_talent_t critical_thinking;

      player_talent_t odyns_fury;
      player_talent_t anger_management;
      player_talent_t reckless_abandon;
      player_talent_t onslaught;
      player_talent_t ravager;
      player_talent_t bladestorm;

      player_talent_t dancing_blades;
      player_talent_t titanic_rage;
      player_talent_t unbridled_ferocity;
      player_talent_t depths_of_insanity;
      player_talent_t tenderize;
      player_talent_t storm_of_steel;
      player_talent_t unhinged;
    } fury;

    struct protection_talents_t
    {
      player_talent_t ignore_pain;

      player_talent_t revenge;

      player_talent_t demoralizing_shout;
      player_talent_t devastator;
      player_talent_t last_stand;

      player_talent_t fight_through_the_flames;
      player_talent_t best_served_cold;
      player_talent_t strategist;
      player_talent_t brace_for_impact;
      player_talent_t unnerving_focus;

      player_talent_t challenging_shout;
      player_talent_t instigate;
      player_talent_t rend;
      player_talent_t bloodsurge;
      player_talent_t fueled_by_violence;
      player_talent_t brutal_vitality;

      player_talent_t disrupting_shout;
      player_talent_t show_of_force;
      player_talent_t sudden_death;
      player_talent_t thunderlord;
      player_talent_t shield_wall;
      player_talent_t bolster;
      player_talent_t tough_as_nails;
      player_talent_t spell_block;
      player_talent_t bloodborne;

      player_talent_t heavy_repercussions;
      player_talent_t into_the_fray;
      player_talent_t enduring_defenses;
      player_talent_t massacre;
      player_talent_t anger_management;
      player_talent_t defenders_aegis;
      player_talent_t impenetrable_wall;
      player_talent_t punish;
      player_talent_t juggernaut;

      player_talent_t focused_vigor;
      player_talent_t shield_specialization;
      player_talent_t enduring_alacrity;

      player_talent_t shield_charge;
      player_talent_t booming_voice;
      player_talent_t indomitable;
      player_talent_t violent_outburst;
      player_talent_t ravager;

      player_talent_t battering_ram;
      player_talent_t champions_bulwark;
      player_talent_t battle_scarred_veteran;
      player_talent_t dance_of_death;
      player_talent_t storm_of_steel;

    } protection;

    struct colossus_talents_t
    {
      player_talent_t demolish;
      player_talent_t martial_expert;
      player_talent_t colossal_might;
      player_talent_t boneshaker; // NYI
      player_talent_t earthquaker;
      player_talent_t one_against_many;
      player_talent_t arterial_bleed;
      player_talent_t tide_of_battle;
      player_talent_t no_stranger_to_pain;
      player_talent_t veteran_vitality; // NYI
      player_talent_t practiced_strikes;
      player_talent_t precise_might;
      player_talent_t mountain_of_muscle_and_scars;
      player_talent_t dominance_of_the_colossus;
    } colossus;

    struct slayer_talents_t
    {
      player_talent_t slayers_dominance;
      player_talent_t imminent_demise;
      player_talent_t overwhelming_blades;
      player_talent_t relentless_pursuit; // NYI
      player_talent_t vicious_agility; // NYI
      player_talent_t death_drive; // NYI
      player_talent_t culling_cyclone;
      player_talent_t brutal_finish;
      player_talent_t fierce_followthrough;
      player_talent_t opportunist;
      player_talent_t show_no_mercy;
      player_talent_t reap_the_storm;
      player_talent_t slayers_malice;
      player_talent_t unrelenting_onslaught;
    } slayer;

    struct mountain_thane_talents_t
    {
      player_talent_t lightning_strikes;
      player_talent_t crashing_thunder;
      player_talent_t ground_current;
      player_talent_t strength_of_the_mountain;
      player_talent_t thunder_blast;
      player_talent_t storm_bolts;
      player_talent_t storm_shield; // NYI
      player_talent_t keep_your_feet_on_the_ground; // NYI
      player_talent_t steadfast_as_the_peaks;
      player_talent_t flashing_skies;
      player_talent_t snap_induction;
      player_talent_t gathering_clouds;
      player_talent_t thorims_might;
      player_talent_t burst_of_power;
      player_talent_t avatar_of_the_storm;
    } mountain_thane;

    struct shared_talents_t
    {
      player_talent_t ravager;
      player_talent_t rend;
      player_talent_t bloodsurge;
      player_talent_t dance_of_death;
    } shared;

  } talents;

  struct tier_set_t
  {
    const spell_data_t* t29_arms_2pc;
    const spell_data_t* t29_arms_4pc;
    const spell_data_t* t29_fury_2pc;
    const spell_data_t* t29_fury_4pc;
    const spell_data_t* t29_prot_2pc;
    const spell_data_t* t29_prot_4pc;
    const spell_data_t* t30_arms_2pc;
    const spell_data_t* t30_arms_4pc;
    const spell_data_t* t30_fury_2pc;
    const spell_data_t* t30_fury_4pc;
    const spell_data_t* t30_prot_2pc;
    const spell_data_t* t30_prot_4pc;
    const spell_data_t* t31_arms_2pc;
    const spell_data_t* t31_arms_4pc;
    const spell_data_t* t31_fury_2pc;
    const spell_data_t* t31_fury_4pc;
  } tier_set;

  // Covenant Powers
  struct covenant_t
  {
    const spell_data_t* conquerors_banner;
  } covenant;

  struct warrior_options_t
  {
  } options;

  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;
  std::string default_temporary_enchant() const override;

  warrior_t( sim_t* sim, util::string_view name, race_e r = RACE_NIGHT_ELF )
    : parse_player_effects_t( sim, WARRIOR, name, r ),
      rampage_attacks( 0 ),
      first_rampage_attack_missed( false ),
      active(),
      buff(),
      cooldown(),
      gain(),
      spell(),
      mastery(),
      proc(),
      spec(),
      talents()
  {
    non_dps_mechanics =
        true;  // When set to false, disables stuff that isn't important, such as second wind, bloodthirst heal, etc.
    warrior_fixed_time    = true;
    into_the_fray_friends = -1;
    never_surrender_percentage = 70;

    resource_regeneration = regen_type::DISABLED;
  }

  // Character Definition
  void init_spells() override;
  void init_items() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_finished() override;
  void init_gains() override;
  void init_position() override;
  void init_procs() override;
  void init_resources( bool ) override;
  void arise() override;
  void combat_begin() override;
  void init_rng() override;
  bool validate_fight_style( fight_style_e style ) const override;
  double composite_attribute( attribute_e attr ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_target_crit_chance( player_t* target ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_armor_multiplier() const override;
  double composite_bonus_armor() const override;
  double composite_base_armor_multiplier() const override;
  double composite_block() const override;
  double composite_block_reduction( action_state_t* s ) const override;
  double composite_parry_rating() const override;
  double composite_parry() const override;
  double composite_attack_power_multiplier() const override;
  // double composite_melee_attack_power() const override;
  double composite_mastery() const override;
  double composite_damage_versatility() const override;
  double composite_heal_versatility() const override;
  double composite_mitigation_versatility() const override;
  double composite_crit_block() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_crit_rating() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double composite_spell_crit_chance() const override;
  double composite_leech() const override;
  double resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  void teleport( double yards, timespan_t duration ) override;
  void trigger_movement( double distance, movement_direction_type direction ) override;
  void interrupt() override;
  void reset() override;
  void moving() override;
  void create_actions() override;
  void create_options() override;
  std::string create_profile( save_e type ) override;
  void invalidate_cache( cache_e ) override;
  double non_stacking_movement_modifier() const override;

  void apl_default();
  void init_action_list() override;

  action_t* create_action( util::string_view name, util::string_view options ) override;
  void activate() override;
  resource_e primary_resource() const override
  {
    return RESOURCE_RAGE;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  // void assess_damage_imminent_pre_absorb( school_e, result_amount_type, action_state_t* s ) override;
  // void assess_damage_imminent( school_e, result_amount_type, action_state_t* s ) override;
  void assess_damage( school_e, result_amount_type, action_state_t* ) override;
  void target_mitigation( school_e, result_amount_type, action_state_t* ) override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;
  void apply_affecting_auras( action_t& action ) override;
  void parse_player_effects();

  void datacollection_begin() override;
  void datacollection_end() override;

  target_specific_t<warrior_td_t> target_data;

  const warrior_td_t* find_target_data( const player_t* target ) const override
  {
    return target_data[ target ];
  }

  warrior_td_t* get_target_data( player_t* target ) const override
  {
    warrior_td_t*& td = target_data[ target ];

    if ( !td )
    {
      td = new warrior_td_t( target, const_cast<warrior_t&>( *this ) );
    }
    return td;
  }

  // Secondary Action Tracking - From Rogue
private:
  std::vector<action_t*> background_actions;

public:
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


  void enrage()
  {
    buff.enrage->trigger();
  }
};

namespace
{  // UNNAMED NAMESPACE
// Template for common warrior action code. See priest_action_t.
template <class Base>
struct warrior_action_t : public parse_action_effects_t<Base>
{
  struct affected_by_t
  {
    // talents
    bool sweeping_strikes;

    affected_by_t()
      : sweeping_strikes( false )
    {
    }
  } affected_by;

  bool usable_while_channeling;
  double tactician_per_rage;

private:
  using ab = parse_action_effects_t<Base>;
public:
  using base_t = warrior_action_t<Base>;
  bool track_cd_waste;
  simple_sample_data_with_min_max_t *cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;
  bool initialized;
  warrior_action_t( util::string_view n, warrior_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      usable_while_channeling( false ),
      tactician_per_rage( 0 ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr ),
      initialized( false )
  {
    ab::may_crit = true;
    if ( p()->talents.arms.deft_experience->ok() )
    {
      tactician_per_rage += ( ( player->talents.arms.tactician->effectN( 1 ).percent() +
                                player->talents.arms.deft_experience->effectN( 2 ).percent() ) / 100 );
    }
    else
    {
      tactician_per_rage += ( player->talents.arms.tactician->effectN( 1 ).percent() / 100 );
    }

    if ( this->data().ok() )
    {
      apply_buff_effects();

      if ( this->type == action_e::ACTION_SPELL || this->type == action_e::ACTION_ATTACK )
      {
        apply_debuff_effects();
      }

      if ( this->data().flags( spell_attribute::SX_ABILITY ) || this->trigger_gcd > 0_ms )
      {
        this->not_a_proc = true;
      }
    }
  }

  void apply_buff_effects()
  {
    // Shared
    parse_effects( p()->buff.avatar, effect_mask_t( true ).disable( 8 ), p()->talents.arms.spiteful_serenity, p()->talents.warrior.unstoppable_force );

    if ( p()->specialization() == WARRIOR_ARMS )
    {
      // Add Flat Modifier (107): Spell Cooldown (11) isn't yet supported by parse_effects.
      // This one is for Blademaster's Torment, effect 8 is dynamically enabled
      // parse_effects( p()->buff.avatar, effect_mask_t( false ).enable( 8 ), p()->talents.arms.spiteful_serenity, p()->talents.warrior.unstoppable_force,  [ this ] { return p()->talents.warrior.blademasters_torment->ok(); } );

      parse_effects( p()->buff.dance_of_death_bladestorm );
      parse_effects( p()->buff.juggernaut );
      parse_effects( p()->buff.merciless_bonegrinder );
      parse_effects( p()->buff.storm_of_swords );
      parse_effects( p()->buff.recklessness_warlords_torment );

      parse_effects( p()->buff.strike_vulnerabilities ); // T29 arms
      parse_effects( p()->buff.crushing_advance ); // T30 Arms 4pc

      // TWW1 Tier
      parse_effects( p()->buff.overpowering_might );  // Arms 2pc
      parse_effects( p()->buff.lethal_blows );        // Arms 4pc
    }
    else if ( p()->specialization() == WARRIOR_FURY )
    {
      parse_effects( p()->mastery.unshackled_fury, [ this ] { return p()->buff.enrage->check(); } );
      parse_effects( p()->buff.ashen_juggernaut );
      parse_effects( p()->buff.berserker_stance );
      parse_effects( p()->buff.bloodcraze, p()->talents.fury.bloodcraze );
      parse_effects( p()->buff.dancing_blades );
      // Action-scoped Enrage effects(#4, #5) only apply with Powerful Enrage
      if ( p()->talents.fury.powerful_enrage->ok() )
        parse_effects( p()->buff.enrage, effect_mask_t( false ).enable( 4, 5 ) );
      parse_effects( p()->buff.recklessness );
      parse_effects( p()->buff.slaughtering_strikes );
      parse_effects( p()->talents.fury.wrath_and_fury, effect_mask_t( false ).enable( 2 ), [ this ] { return p()->buff.enrage->check(); } );

      parse_effects( p()->buff.merciless_assault );

      // TWW1 Tier
      parse_effects( p()->buff.bloody_rampage );      // Fury 2pc
      parse_effects( p()->buff.deep_thirst );         // Fury 4pc
    }
    else if ( p()->specialization() == WARRIOR_PROTECTION )
    {
      parse_effects( p()->buff.battering_ram );
      parse_effects( p()->buff.juggernaut_prot );
      parse_effects( p()->buff.seismic_reverberation_revenge );
      parse_effects( p()->buff.vanguards_determination );

      // TWW1 Tier
      parse_effects( p()->buff.expert_strategist );   // Prot 2pc
      parse_effects( p()->buff.brutal_followup );     // Prot 4pc
    }

    // Colossus
    if ( p()->talents.colossus.demolish->ok() )
    {
      parse_effects( p()->buff.colossal_might, effect_mask_t( false ).enable( 1 ), p()->spec.protection_warrior );
      if ( p()->talents.colossus.arterial_bleed->ok() )
      {
        parse_effects( p()->buff.colossal_might, effect_mask_t( false ).enable( 2 ), p()->spec.protection_warrior );
      }
      if ( p()->talents.colossus.tide_of_battle->ok() )
      {
        parse_effects( p()->buff.colossal_might, effect_mask_t( false ).enable( 3, 4 ), p()->spec.protection_warrior );
      }
      // Effect 3 is the auto attack mod
      parse_effects( p()->talents.colossus.mountain_of_muscle_and_scars, effect_mask_t( false ).enable( 3 ) );
      parse_effects( p()->talents.colossus.practiced_strikes );
    }

    // Slayer
    if ( p()->talents.slayer.slayers_dominance->ok() )
    {
      parse_effects( p()->buff.brutal_finish );
      parse_effects( p()->buff.fierce_followthrough );
      parse_effects( p()->buff.opportunist );
    }

    // Mountain Thane
    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      // Crashing Thunder
      // Damage amps
      parse_effects( p()->talents.mountain_thane.crashing_thunder, effect_mask_t( false ).enable( 1, 2, 3, 9 ) );
      // Reduce TC rage cost by 100%
      parse_effects( p()->talents.mountain_thane.crashing_thunder, effect_mask_t( false ).enable( 5 ) );
      if ( p()->specialization() == WARRIOR_FURY )
      {
        // Add 5 rage gain to TC for Fury
        parse_effects( p()->talents.mountain_thane.crashing_thunder, effect_mask_t( false ).enable( 4 ) );
        // Update various talents
        parse_effects( p()->talents.warrior.barbaric_training, effect_mask_t( false ).enable( 3, 4 ), p()->talents.mountain_thane.crashing_thunder );
        parse_effects( p()->talents.fury.meat_cleaver, effect_mask_t( false ).enable( 4 ), p()->talents.mountain_thane.crashing_thunder );
      }
      parse_effects( p()->buff.burst_of_power, effect_mask_t( false ).enable( 2 ) );
    }
  }

  void apply_debuff_effects()
  {
    // Shared
    parse_target_effects( d_fn( &warrior_td_t::debuffs_champions_might ),
                            p()->talents.warrior.champions_spear->effectN( 1 ).trigger() );

    // Arms
    // Arms deep wounds spell data contains T30 2pc bonus, which is disabled/enabled via script.
    // To account for this, we parse the data twice, first ignoring effects #4 & #5, then if the T30 2pc is active only
    // parse #4 & #5.
    parse_target_effects( d_fn( &warrior_td_t::dots_deep_wounds ),
                          p()->spell.deep_wounds_arms, effect_mask_t( true ).disable( 4, 5 ),
                          p()->mastery.deep_wounds_ARMS );
    if ( p()->sets->has_set_bonus( WARRIOR_ARMS, T30, B2 ) )
    {
      parse_target_effects( d_fn( &warrior_td_t::dots_deep_wounds ),
                            p()->spell.deep_wounds_arms, effect_mask_t( false ).enable( 4, 5 ) );
    }

    if ( p()->talents.warrior.thunderous_words->ok() )
    {
      // Action-scoped Thunderous Roar effect(#3) only apply with Thunderous Words
      parse_target_effects( d_fn( &warrior_td_t::dots_thunderous_roar ),
                            p()->talents.warrior.thunderous_roar->effectN( 2 ).trigger(),
                            effect_mask_t( false ).enable( 3 ) );
    }

    parse_target_effects( d_fn( &warrior_td_t::debuffs_colossus_smash ),
                          p()->spell.colossus_smash_debuff,
                          p()->talents.arms.blunt_instruments, p()->talents.arms.spiteful_serenity );

    parse_target_effects( d_fn( &warrior_td_t::debuffs_executioners_precision ),
                          p()->talents.arms.executioners_precision->effectN( 1 ).trigger(),
                          p()->talents.arms.executioners_precision );
    // Fury

    // Protection
    parse_target_effects( d_fn( &warrior_td_t::debuffs_demoralizing_shout ),
                          p()->talents.protection.demoralizing_shout,
                          p()->talents.protection.booming_voice );

    // Colossus
    if ( p()->talents.colossus.demolish->ok() )
    {
      // Wrecked has a value of 10 in spelldata, but it needs to be interpreted as 1% per stack
      parse_target_effects( d_fn( &warrior_td_t::debuffs_wrecked ),
                            p()->spell.wrecked_debuff, effect_mask_t( false ).enable( 2 ), p()->spell.wrecked_debuff->effectN( 2 ).base_value() / 1000, p()->spec.protection_warrior );
    }

    // Slayer
    if ( p()->talents.slayer.slayers_dominance->ok() )
    {
      parse_target_effects( d_fn( &warrior_td_t::debuffs_marked_for_execution ),
                            p()->spell.marked_for_execution_debuff,
                            effect_mask_t( false ).enable( 1 ) );

      if ( p()->talents.slayer.show_no_mercy->ok() )
      {
        parse_target_effects( d_fn( &warrior_td_t::debuffs_marked_for_execution ),
                            p()->spell.marked_for_execution_debuff,
                            effect_mask_t( false ).enable( 2, 3 ) );
      }

      parse_target_effects( d_fn( &warrior_td_t::debuffs_overwhelmed ),
                            p()->spell.overwhelmed_debuff );
    }

    // Mountain Thane
  }

  template <typename... Ts>
  void parse_effects( Ts&&... args ) { ab::parse_effects( std::forward<Ts>( args )... ); }
  template <typename... Ts>
  void parse_target_effects( Ts&&... args ) { ab::parse_target_effects( std::forward<Ts>( args )... ); }

  void init() override
  {
    if ( initialized )
      return;

    ab::init();

    if ( track_cd_waste )
    {
      cd_wasted_exec = get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_exec );
      cd_wasted_cumulative =
          get_data_entry<simple_sample_data_with_min_max_t, data_t>( ab::name_str, p()->cd_waste_cumulative );
      cd_wasted_iter = get_data_entry<simple_sample_data_t, simple_data_t>( ab::name_str, p()->cd_waste_iter );
    }

    affected_by.sweeping_strikes         = ab::data().affected_by( p()->spec.sweeping_strikes->effectN( 1 ) );

    initialized = true;
  }


  warrior_t* p()
  {
    return debug_cast<warrior_t*>( ab::player );
  }

  const warrior_t* p() const
  {
    return debug_cast<warrior_t*>( ab::player );
  }

  warrior_td_t* td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  virtual double tactician_cost() const
  {
    double base = ab::base_cost();

    if ( ab::sim->log )
    {
      ab::sim->out_debug.printf(
          "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f", ab::name(),
          base, this->cost() );
    }

    return base;
  }

  int n_targets() const override
  {
    if ( affected_by.sweeping_strikes && p()->buff.sweeping_strikes->check() )
    {
      return static_cast<int>( 1 + p()->spec.sweeping_strikes->effectN( 1 ).base_value() );
    }

    return ab::n_targets();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = ab::composite_da_multiplier( s );

    if ( affected_by.sweeping_strikes && s->chain_target > 0 )
    {
      dm *= p()->spec.sweeping_strikes->effectN( 2 ).percent();
    }

    return dm;
  }

  void execute() override
  {
    ab::execute();

    if ( affected_by.sweeping_strikes && p()->talents.arms.collateral_damage.ok() && p()->buff.sweeping_strikes -> up() && ab::num_targets_hit >= 2 )
    {
      p() -> buff.collateral_damage -> trigger();
    }
  }

  bool ready() override
  {
    if ( !ab::ready() )
      return false;

    // -1 melee range implies that the ability can be used at any distance from the target.
    if ( p()->current.distance_to_move > ab::range && ab::range != -1 )
      return false;

    if ( ( p()->channeling || p()->buff.bladestorm->check() ) && !usable_while_channeling )
      return false;

    return true;
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

  bool usable_moving() const override
  {  // All warrior abilities are usable while moving, the issue is being in range.
    return true;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( ab::sim->log )
    {
      ab::sim->out_debug.printf(
          "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Haste: %4.4f%%, "
          "Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action "
          "Multiplier: %4.4f",
          p()->cache.strength(), p()->cache.attack_power() * p()->composite_attack_power_multiplier(),
          p()->cache.attack_crit_chance() * 100, p()->composite_player_critical_damage_multiplier( s ),
          p()->cache.mastery_value() * 100, ( 1 / p()->cache.attack_haste() - 1 ) * 100,
          p()->cache.damage_versatility() * 100, p()->cache.bonus_armor(), s->composite_ta_multiplier(),
          s->composite_da_multiplier(), s->action->action_multiplier() );
    }
  }

  void tick( dot_t* d ) override
  {
    ab::tick( d );

    if ( ab::sim->log )
    {
      ab::sim->out_debug.printf(
          "Strength: %4.4f, AP: %4.4f, Crit: %4.4f%%, Crit Dmg Mult: %4.4f,  Mastery: %4.4f%%, Haste: %4.4f%%, "
          "Versatility: %4.4f%%, Bonus Armor: %4.4f, Tick Multiplier: %4.4f, Direct Multiplier: %4.4f, Action "
          "Multiplier: %4.4f",
          p()->cache.strength(), p()->cache.attack_power() * p()->composite_attack_power_multiplier(),
          p()->cache.attack_crit_chance() * 100, p()->composite_player_critical_damage_multiplier( d->state ),
          p()->cache.mastery_value() * 100, ( 1 / p()->cache.attack_haste() - 1 ) * 100,
          p()->cache.damage_versatility() * 100, p()->cache.bonus_armor(), d->state->composite_ta_multiplier(),
          d->state->composite_da_multiplier(), d->state->action->action_ta_multiplier() );
    }
  }

  void consume_resource() override
  {
    //td = find_target_data( target );

    if ( tactician_per_rage )
    {
      tactician();
    }

    ab::consume_resource();

    double rage = ab::last_resource_cost;

    if ( p()->buff.test_of_might_tracker->check() )
    {
        if ( ab::id != 190456)  // Test of might ignores rage used for ignore pain
          p()->buff.test_of_might_tracker->current_value += rage;  // Uses rage cost before anything makes it cheaper.
    }

    if ( p()->talents.arms.anger_management->ok() || p()->talents.fury.anger_management->ok() || p()->talents.protection.anger_management->ok() )
    {
      anger_management( rage );
    }
    if ( rage > 0 && !ab::aoe && ab::execute_state && ab::result_is_miss( ab::execute_state->result ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage * 0.8, p()->gain.avoided_attacks );
    }

    // Protection Warrior Violent Outburst Seeing Red Tracking
    if ( p()->specialization() == WARRIOR_PROTECTION && p()->talents.protection.violent_outburst.ok() && rage > 0 )
    {
      // Trigger the buff if this is the first rage consumption of the iteration
      if ( !p()->buff.seeing_red_tracking->check() )
      {
        p()->buff.seeing_red_tracking->trigger();
      }

      double original_value = p()->buff.seeing_red_tracking->current_value;
      double rage_per_stack = p()->buff.seeing_red_tracking->data().effectN( 1 ).base_value();
      p()->buff.seeing_red_tracking->current_value += rage;
      p()->sim->print_debug( "{} increments seeing_red_tracking by {}. Old={} New={}", p()->name(), rage,
                             original_value, p()->buff.seeing_red_tracking->current_value );

      while ( p()->buff.seeing_red_tracking->current_value >= rage_per_stack )
      {
        p()->buff.seeing_red_tracking->current_value -= rage_per_stack;
        p()->sim->print_debug(
            "{} reaches seeing_red_tracking threshold, triggering seeing_red buff. New seeing_red_tracking value is {}",
            p()->name(), p()->buff.seeing_red_tracking->current_value );

        p()->buff.seeing_red->trigger();

        if( p()->buff.seeing_red->at_max_stacks() )
        {
          p()->buff.seeing_red->expire();
          p()->buff.violent_outburst->trigger();
        }

      }
    }
  }

  virtual void tactician()
  {
    if ( p()->specialization() == WARRIOR_ARMS && ab::id == 190456 ) // Ignore pain can not trigger tactician for arms
      return;

    double tact_rage = tactician_cost();  // Tactician resets based on cost before things make it cost less.
    double tactician_chance = tactician_per_rage;

    if ( ab::rng().roll( tactician_chance * tact_rage ) )
    {
      p()->cooldown.overpower->reset( true );
      p()->proc.tactician->occur();
      if ( p()->talents.slayer.opportunist->ok() )
        p()->buff.opportunist->trigger();
      if ( p()->sets->has_set_bonus( WARRIOR_ARMS, TWW1, B4 ) )
        p()->buff.lethal_blows->trigger();
    }
  }

  void anger_management( double rage )
  {
    if ( rage > 0 )
    {
      // Anger management reduces some cds by 1 sec per n rage spent (different per spec)
      double cd_time_reduction = -1.0 * rage;

      if ( p()->specialization() == WARRIOR_FURY )
      {
        cd_time_reduction /= p()->talents.fury.anger_management->effectN( 3 ).base_value();
        p()->cooldown.recklessness->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.bladestorm->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.ravager->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }

      else if ( p()->specialization() == WARRIOR_ARMS )
      {
        if ( ab::id == 190456 )  // Ignore pain can not trigger anger management for arms
          return;

        cd_time_reduction /= p()->talents.arms.anger_management->effectN( 1 ).base_value();  
                                                                                                                                                                       
        p()->cooldown.colossus_smash->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.warbreaker->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.bladestorm->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.ravager->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }

      else if ( p()->specialization() == WARRIOR_PROTECTION )
      {
        cd_time_reduction /= p()->talents.protection.anger_management->effectN( 2 ).base_value();
        p()->cooldown.avatar->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.shield_wall->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }
    }
  }

  // Delayed Execute Event ====================================================

  struct delayed_execute_event_t : public event_t
  {
    action_t* action;
    player_t* target;

    delayed_execute_event_t( warrior_t* p, action_t* a, player_t* t, timespan_t delay )
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
};

struct warrior_heal_t : public warrior_action_t<heal_t>
{  // Main Warrior Heal Class
  warrior_heal_t( util::string_view n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
    may_crit = tick_may_crit = hasted_ticks = false;
    target                                  = p;
  }
};

struct warrior_spell_t : public warrior_action_t<spell_t>
{  // Main Warrior Spell Class - Used for spells that deal no damage, usually buffs.
  warrior_spell_t( util::string_view n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

struct warrior_attack_t : public warrior_action_t<melee_attack_t>
{  // Main Warrior Attack Class
  warrior_attack_t( util::string_view n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, p, s )
  {
    special = true;
  }

  void impact( action_state_t* s ) override
  {
    warrior_action_t::impact( s );

    if ( !special )  // Procs below only trigger on special attacks, not autos
      return;

    if ( p()->talents.slayer.slayers_dominance->ok() && s->target == p()->target && p()->rppm.slayers_dominance->trigger() )
    {
      p()->active.slayers_strike->execute_on_target(s->target);
    }
  }


  void execute() override
  {
    base_t::execute();
    if ( !special )  // Procs below only trigger on special attacks, not autos
      return;

    if ( ( p()->talents.arms.sudden_death->ok() || p()->talents.fury.sudden_death->ok() || p()->talents.protection.sudden_death->ok() ) 
           && p()->cooldown.sudden_death_icd->up() && p()->rppm.sudden_death->trigger() )
    {
      p()->buff.sudden_death->trigger();
      p()->cooldown.sudden_death_icd->start();
      p()->cooldown.execute->reset( true );
    }
  }

  player_t* select_random_target() const
  {
    if ( sim->distance_targeting_enabled )
    {
      std::vector<player_t*> targets;
      range::for_each( sim->target_non_sleeping_list, [&targets, this]( player_t* t ) {
        if ( p()->get_player_distance( *t ) <= radius + t->combat_reach )
        {
          targets.push_back( t );
        }
      } );

      auto random_idx = rng().range( targets.size() );
      return !targets.empty() ? targets[ random_idx ] : nullptr;
    }
    else
    {
      auto random_idx = rng().range( size_t(), sim->target_non_sleeping_list.size() );
      return sim->target_non_sleeping_list[ random_idx ];
    }
  }
};

// Reap the Storm ===========================================================

struct reap_the_storm_t : public warrior_attack_t
{
  reap_the_storm_t( util::string_view name, warrior_t* p ) : warrior_attack_t( name, p, p->spell.reap_the_storm )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = p->talents.slayer.reap_the_storm->effectN( 1 ).base_value();
    weapon = &( p->main_hand_weapon );
  }
};

// Devastate ================================================================

struct devastate_t : public warrior_attack_t
{
  double shield_slam_reset;
  devastate_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "devastate", p, p->spec.devastate ),
      shield_slam_reset( p->talents.protection.strategist->effectN( 1 ).percent() )
  {
    weapon        = &( p->main_hand_weapon );
    impact_action = p->active.deep_wounds_PROT;
    parse_options( options_str );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) && rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
      if ( p()->sets->has_set_bonus( WARRIOR_PROTECTION, TWW1, B2 ) )
        p()->buff.expert_strategist->trigger();
    }

    if ( p() -> talents.protection.instigate.ok() )
      p() -> resource_gain( RESOURCE_RAGE, p() -> talents.protection.instigate -> effectN( 2 ).resource( RESOURCE_RAGE ), p() -> gain.instigate );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p() -> talents.protection.instigate -> effectN( 1 ).percent();

    return am;
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() || p()->talents.protection.devastator->ok() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Melee Attack =============================================================

struct sidearm_t : warrior_attack_t
{
  sidearm_t( warrior_t* p ) : warrior_attack_t( "sidearm", p, p->find_spell( 384391 ) )
  {
    background = true;
  }
};

struct devastator_t : warrior_attack_t
{
  double shield_slam_reset;

  devastator_t( warrior_t* p ) : warrior_attack_t( "devastator", p, p->talents.protection.devastator->effectN( 1 ).trigger() ),
      shield_slam_reset( p->talents.protection.devastator->effectN( 2 ).percent() )
  {
    background = true;
    impact_action = p->active.deep_wounds_PROT;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( rng().roll( shield_slam_reset ) )
    {
      p() -> cooldown.shield_slam -> reset( true );
    }

    if ( p() -> talents.protection.instigate.ok() )
      p() -> resource_gain( RESOURCE_RAGE, p() -> talents.protection.instigate -> effectN( 4 ).resource( RESOURCE_RAGE ), p() -> gain.instigate );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p() -> talents.protection.instigate -> effectN( 3 ).percent();

    return am;
  }
};

struct melee_t : public warrior_attack_t
{
  int sync_weapons;
  bool first;
  warrior_attack_t* sidearm;
  bool mh_lost_melee_contact, oh_lost_melee_contact;
  double base_rage_generation, arms_rage_multiplier, fury_rage_multiplier, prot_rage_multiplier, seasoned_soldier_crit_mult;
  double sidearm_chance, enrage_chance;
  devastator_t* devastator;
  melee_t( util::string_view name, warrior_t* p, int sw )
    : warrior_attack_t( name, p, spell_data_t::nil() ),
      sync_weapons( sw ),
      first( true ),
      sidearm( nullptr),
      mh_lost_melee_contact( true ),
      oh_lost_melee_contact( true ),
      base_rage_generation( 1.75 ),
      arms_rage_multiplier( 1.0 + p->spec.arms_warrior->effectN( 1 ).percent() ),
      fury_rage_multiplier( 1.0 + p->spec.fury_warrior->effectN( 3 ).percent() ),
      prot_rage_multiplier( 1.0 + p->spec.protection_warrior->effectN( 5 ).percent() ),
      seasoned_soldier_crit_mult( p->spec.seasoned_soldier->effectN( 1 ).percent() ),
      sidearm_chance( p->talents.warrior.sidearm->proc_chance() ),
      devastator( nullptr )
  {
    school                    = SCHOOL_PHYSICAL;
    may_crit                  = true;
    may_glance                = true;
    background                = true;
    allow_class_ability_procs = true;
    not_a_proc                = true;
    repeating                 = true;
    trigger_gcd               = timespan_t::zero();
    special                   = false;

    usable_while_channeling   = true;

    weapon_multiplier = 1.0;
    if ( p->dual_wield() )
    {
      base_hit -= 0.19;
    }
    if ( p->talents.protection.devastator->ok() )
    {
      devastator = new devastator_t( p );
      add_child( devastator );
    }
    if ( p->talents.warrior.sidearm->ok() )
    {
      sidearm = new sidearm_t( p );
    }

    // explicitly apply here as the calls in warrior_action_t require valid spell data
    apply_buff_effects();
    apply_debuff_effects();
  }

  void reset() override
  {
    warrior_attack_t::reset();

    first = true;
    mh_lost_melee_contact = oh_lost_melee_contact = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = warrior_attack_t::execute_time();
    if ( weapon->slot == SLOT_MAIN_HAND && mh_lost_melee_contact )
    {
      return timespan_t::zero();  // If contact is lost, the attack is instant.
    }
    else if ( weapon->slot == SLOT_OFF_HAND && ( oh_lost_melee_contact || first ) )  // Also used for the first attack.
    {
      if ( sync_weapons )
        return timespan_t::zero();

      // From testing and log analysis, when you charge in, around 50ms is a very common offset
      // for the off hand attack.  If you walk up and auto the boss, it's typically 50% of your swing time offset.
      return timespan_t::from_millis( rng().gauss_ab( 50, 25, 10, static_cast<double>( (t * 0.5).total_millis() ) ) );
    }
    else
    {
      return t;
    }
  }

  void schedule_execute( action_state_t* s ) override
  {
    warrior_attack_t::schedule_execute( s );
    if ( weapon->slot == SLOT_MAIN_HAND )
    {
      mh_lost_melee_contact = false;
    }
    else if ( weapon->slot == SLOT_OFF_HAND )
    {
      oh_lost_melee_contact = false;
    }
  }

  double composite_hit() const override
  {
    if ( p()->talents.fury.focus_in_chaos.ok() && p()->buff.enrage->check() )
    {
      return 1.0;
    }
    return warrior_attack_t::composite_hit();
  }

  void execute() override
  {
    if ( first )
      first = false;

    if ( p()->current.distance_to_move > 5 )
    {  // Cancel autoattacks, auto_attack_t will restart them when we're back in range.
      if ( weapon->slot == SLOT_MAIN_HAND )
      {
        p()->proc.delayed_auto_attack->occur();
        mh_lost_melee_contact = true;
        player->main_hand_attack->cancel();
      }
      else
      {
        p()->proc.delayed_auto_attack->occur();
        oh_lost_melee_contact = true;
        player->off_hand_attack->cancel();
      }
    }
    else
    {
      warrior_attack_t::execute();

      if ( result_is_hit( execute_state->result ) )
      {
        if ( p()->talents.protection.devastator->ok() )
        {
          devastator->target = execute_state->target;
          devastator->schedule_execute();
        }
        trigger_rage_gain( execute_state );
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( sidearm && result_is_hit( s->result ) && rng().roll( sidearm_chance ) )
    {
      sidearm->set_target( s->target );
      sidearm->execute();
    }

    if ( p()->talents.warrior.wild_strikes->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.wild_strikes->trigger();
    }

    if ( p() -> specialization() == WARRIOR_FURY &&
          p() -> main_hand_weapon.group() == WEAPON_1H &&
          p() -> off_hand_weapon.group() == WEAPON_1H &&
          p() -> talents.fury.single_minded_fury->ok() &&
          p() -> cooldown.single_minded_fury_icd->up() &&
          s -> result == RESULT_CRIT )
    {
      if ( rng().roll( p() -> talents.fury.single_minded_fury->proc_chance() ) )
      {
        p()->cooldown.single_minded_fury_icd->start();
        p()->buff.enrage->trigger();
      }
    }

  }

  void trigger_rage_gain( const action_state_t* s )
  {
    double rage_gain = weapon->swing_time.total_seconds() * base_rage_generation;

    if ( p()->specialization() == WARRIOR_ARMS )
    {
      rage_gain *= arms_rage_multiplier;
      if ( s->result == RESULT_CRIT )
      {
        rage_gain *= 1.0 + seasoned_soldier_crit_mult;
      }
    }
    else if ( p() -> specialization() == WARRIOR_FURY )
    {
      rage_gain *= fury_rage_multiplier;
      if ( weapon->slot == SLOT_OFF_HAND )
      {
        rage_gain *= 0.5;
      }
    }
    else
    {
      rage_gain *= prot_rage_multiplier;
    }
    rage_gain *= 1.0 + p()->talents.warrior.war_machine->effectN( 2 ).percent();

    rage_gain = util::round( rage_gain, 1 );

    if ( p()->specialization() == WARRIOR_ARMS && s->result == RESULT_CRIT )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_gain, p()->gain.melee_crit );
    }
    else
    {
      p()->resource_gain( RESOURCE_RAGE, rage_gain,
                          weapon->slot == SLOT_OFF_HAND ? p()->gain.melee_off_hand : p()->gain.melee_main_hand );
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public warrior_attack_t
{
  int sync_weapons;
  auto_attack_t( warrior_t* p, util::string_view options_str ) : warrior_attack_t( "auto_attack", p )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );
    assert( p->main_hand_weapon.type != WEAPON_NONE );
    ignore_false_positive = usable_while_channeling = true;
    range = 5;

    if ( p->main_hand_weapon.type == WEAPON_2H && p->has_shield_equipped() && p->specialization() != WARRIOR_FURY )
    {
      sim->errorf( "Player %s is using a 2 hander + shield while specced as Protection or Arms. Disabling autoattacks.",
                   name() );
    }
    else
    {
      p->main_hand_attack                    = new melee_t( "auto_attack_mh", p, sync_weapons );
      p->main_hand_attack->weapon            = &( p->main_hand_weapon );
      p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;
    }

    if ( p->off_hand_weapon.type != WEAPON_NONE && p->specialization() == WARRIOR_FURY )
    {
      p->off_hand_attack                    = new melee_t( "auto_attack_oh", p, sync_weapons );
      p->off_hand_attack->weapon            = &( p->off_hand_weapon );
      p->off_hand_attack->base_execute_time = p->off_hand_weapon.swing_time;
      p->off_hand_attack->id                = 1;
    }
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    if ( p()->main_hand_attack->execute_event == nullptr )
    {
      p()->main_hand_attack->schedule_execute();
    }
    if ( p()->off_hand_attack && p()->off_hand_attack->execute_event == nullptr )
    {
      p()->off_hand_attack->schedule_execute();
    }
  }

  bool ready() override
  {
    bool ready = warrior_attack_t::ready();
    if ( ready )  // Range check
    {
      if ( p()->main_hand_attack->execute_event == nullptr )
      {
        return ready;
      }
      if ( p()->off_hand_attack && p()->off_hand_attack->execute_event == nullptr )
      {
        // Don't check for execute_event if we don't have an offhand.
        return ready;
      }
    }
    return false;
  }
};

// Rend ==============================================================

struct rend_dot_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  rend_dot_t( warrior_t* p )
    : warrior_attack_t( "rend", p, p->find_spell( 388539 ) ),
      bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
      rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
    if ( p()->tier_set.t31_arms_2pc->ok() && p()->rppm.t31_sudden_death->trigger() )
    {
      p()->buff.sudden_death->trigger();
      p()->cooldown.execute->reset( true );
    }
  }

  timespan_t tick_time ( const action_state_t* s ) const override
  {
    auto base_tick_time = warrior_attack_t::tick_time( s );

      auto td = p() -> get_target_data( s -> target );
      if ( td -> debuffs_skullsplitter -> up() )
        base_tick_time *= 1 / ( 1.0 + td -> debuffs_skullsplitter -> value() );

    return base_tick_time;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto dot_duration = warrior_attack_t::composite_dot_duration( s );

      auto td = p() -> get_target_data( s -> target );
      if ( td -> debuffs_skullsplitter -> up() )
        dot_duration *= 1 / ( 1.0 + td -> debuffs_skullsplitter -> value() );

    return dot_duration;
  }
};

struct rend_t : public warrior_attack_t
{
  warrior_attack_t* rend_dot;
  int aoe_targets;
  rend_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "rend", p, p->talents.arms.rend ),
      rend_dot( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    tick_may_crit = true;
    hasted_ticks  = true;
    rend_dot      = new rend_dot_t( p );
    radius = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    rend_dot->set_target( s->target );
    rend_dot->execute();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.meat_cleaver->decrement();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Prot Rend ==============================================================

struct rend_dot_prot_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  rend_dot_prot_t( warrior_t* p )
    : warrior_attack_t( "rend", p, p->find_spell( 394063 ) ),
      bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
      rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
  }
};

struct rend_prot_t : public warrior_attack_t
{
  warrior_attack_t* rend_dot;
  rend_prot_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "rend", p, p->talents.protection.rend ), rend_dot( nullptr )
  {
    parse_options( options_str );
    tick_may_crit = true;
    hasted_ticks  = true;
    // Arma: 2022 Nov 4th.  The trigger spell triggers the arms version of rend dot, even though the tooltip references
    // the prot version.
    if ( p->bugs )
      rend_dot = new rend_dot_t( p );
    else
      rend_dot = new rend_dot_prot_t( p );
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    rend_dot->set_target( s->target );
    rend_dot->execute();
  }

  void execute() override
  {
    warrior_attack_t::execute();
    // 25% proc chance found via testing
    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) )
    {
      p() -> buff.fervid -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.25 );
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Lightning Strike =========================================================

struct ground_current_t : public warrior_attack_t
{
  ground_current_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 460670 ) )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    warrior_attack_t::available_targets( tl );

    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }
};

struct lightning_strike_t : public warrior_attack_t
{
  action_t* ground_current;
  double rage_from_thorims_might;
  lightning_strike_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->spell.lightning_strike ),
    ground_current( nullptr ),
    rage_from_thorims_might( 0 )
  {
    background = true;
    if ( p->talents.mountain_thane.ground_current->ok() )
    {
      std::string s = "ground_current_";
      s += name;
      ground_current = get_action<ground_current_t>( s, p );
      add_child( ground_current );
    }

    if ( p->talents.mountain_thane.thorims_might->ok() )
    {
      rage_from_thorims_might = p->talents.mountain_thane.thorims_might->effectN( 1 ).resource( RESOURCE_RAGE );
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( ground_current )
    {
      ground_current->execute_on_target( s->target );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->talents.mountain_thane.burst_of_power->ok() )
      p()->buff.burst_of_power->trigger();

    if ( p()->talents.mountain_thane.thorims_might->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_thorims_might, p()->gain.thorims_might );
    }

    if ( p()->talents.mountain_thane.avatar_of_the_storm->ok() && !p()->buff.avatar->check() )
    {
      if ( rng().roll( p()->talents.mountain_thane.avatar_of_the_storm->effectN( 2 ).percent() ) )
        p()->buff.avatar->extend_duration_or_trigger( timespan_t::from_seconds( p()->talents.mountain_thane.avatar_of_the_storm->effectN( 3 ).base_value() ) );
    }
  }
};

// Bloodthirst Heal =========================================================

struct bloodthirst_heal_t : public warrior_heal_t
{
  bloodthirst_heal_t( warrior_t* p ) : warrior_heal_t( "bloodthirst_heal", p, p->find_spell( 117313 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background    = true;
  }

  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

// Bloodthirst ==============================================================

struct gushing_wound_dot_t : public warrior_attack_t
{
  gushing_wound_dot_t( warrior_t* p ) : warrior_attack_t( "gushing_wound", p, p->find_spell( 385042 ) ) //288080 & 4 is CB
  {
    background = tick_may_crit = true;
    hasted_ticks               = false;
    //base_td = p->talents.fury.cold_steel_hot_blood.value( 2 );
  }
};

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  warrior_attack_t* gushing_wound;
  int aoe_targets;
  double enrage_chance;
  double rage_from_cold_steel_hot_blood;
  double rage_from_merciless_assault;
  double rage_from_burst_of_power;
  action_t* reap_the_storm;
  bool unhinged;
  bloodthirst_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "bloodthirst", p, p->talents.fury.bloodthirst ),
      bloodthirst_heal( nullptr ),
      gushing_wound( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      enrage_chance( p->spec.enrage->effectN( 2 ).percent() ),
      rage_from_cold_steel_hot_blood( p->find_spell( 383978 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_merciless_assault( p->find_spell( 409983 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_burst_of_power( 0 ),
      reap_the_storm( nullptr ),
      unhinged( false )
  {
    parse_options( options_str );

    weapon = &( p->main_hand_weapon );
    radius = 5;
    if ( p->non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();

    if ( p->talents.fury.fresh_meat->ok() )
    {
      enrage_chance += p->talents.fury.fresh_meat->effectN( 1 ).percent();
    }

    if ( p->talents.fury.cold_steel_hot_blood.ok() )
    {
      gushing_wound = new gushing_wound_dot_t( p );
    }

    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }

    if ( p->talents.slayer.reap_the_storm->ok() )
    {
      reap_the_storm = get_action<reap_the_storm_t>( "reap_the_storm_bloodthirst", p );
      add_child( reap_the_storm );
    }

    if ( p->talents.mountain_thane.burst_of_power->ok() )
    {
      rage_from_burst_of_power = p->talents.mountain_thane.burst_of_power->effectN( 1 ).trigger()->effectN( 3 ).resource( RESOURCE_RAGE );
    }
  }

  // Background version for use with Unhinged
  bloodthirst_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->talents.fury.bloodthirst ),
      bloodthirst_heal( nullptr ),
      gushing_wound( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      enrage_chance( p->spec.enrage->effectN( 2 ).percent() ),
      rage_from_cold_steel_hot_blood( p->find_spell( 383978 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_burst_of_power( 0 ),
      unhinged( false )
  {
    background = true;

    weapon = &( p->main_hand_weapon );
    radius = 5;
    if ( p->non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();

    if ( p->talents.fury.fresh_meat->ok() )
    {
      enrage_chance += p->talents.fury.fresh_meat->effectN( 1 ).percent();
    }

    if ( p->talents.fury.cold_steel_hot_blood.ok() )
    {
      gushing_wound = new gushing_wound_dot_t( p );
    }

    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }

    if ( p->talents.slayer.reap_the_storm->ok() )
    {
      std::string s = "reap_the_storm_";
      s += name;
      reap_the_storm = get_action<reap_the_storm_t>( s, p );
      add_child( reap_the_storm );
    }

    if ( p->talents.mountain_thane.burst_of_power->ok() )
    {
      rage_from_burst_of_power = p->talents.mountain_thane.burst_of_power->effectN( 1 ).trigger()->effectN( 3 ).resource( RESOURCE_RAGE );
    }
  }

  // Constructor for unhinged specfic version, so we can disable sweeping strikes, as well as have a bool to check
  bloodthirst_t( util::string_view name, warrior_t* p, bool unhinged )
    : bloodthirst_t( name, p )
  {
    this->unhinged = unhinged;
  }

  int n_targets() const override
  {
    if ( !unhinged && p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.fury.vicious_contempt->ok() && ( target->health_percentage() < 35 ) )
    {
      am *= 1.0 + ( p()->talents.fury.vicious_contempt->effectN( 1 ).percent() );
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    // Delayed event to cancel the stack on any crit results
    if ( p()->talents.fury.bloodcraze->ok() && s->result == RESULT_CRIT )
      make_event( *p()->sim, [ this ] { p()->buff.bloodcraze->expire(); } );

    if ( gushing_wound && s->result == RESULT_CRIT )
    {
      gushing_wound->execute_on_target( s->target );
    }

    if ( p()->talents.fury.cold_steel_hot_blood.ok() && execute_state->result == RESULT_CRIT &&
         p()->cooldown.cold_steel_hot_blood_icd->up() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_cold_steel_hot_blood, p()->gain.cold_steel_hot_blood );
      p() -> cooldown.cold_steel_hot_blood_icd->start();
    }

    if ( p()->tier_set.t30_fury_4pc->ok() && target == s->target )
    {
      p()->resource_gain( RESOURCE_RAGE, p()->buff.merciless_assault->stack() * rage_from_merciless_assault,
                          p()->gain.merciless_assault );
    }

    if ( p()->tier_set.t31_fury_4pc->ok() && s->result == RESULT_CRIT && p()->cooldown.t31_fury_4pc_icd->up() )
    {
      p()->cooldown.odyns_fury->adjust( - timespan_t::from_millis( p()->spell.t31_fury_4pc->effectN( 3 ).base_value() ) );
      p()->cooldown.t31_fury_4pc_icd->start();
    }

    // We schedule this one to trigger after the action fully resolves, as we need to expire the buff if it already exists
    if ( p()->talents.slayer.fierce_followthrough->ok() && s->result == RESULT_CRIT && s->chain_target == 0 )
      make_event( sim, [ this ] { p()->buff.fierce_followthrough->trigger(); } );

    if ( p()->talents.mountain_thane.burst_of_power->ok() && p()->buff.burst_of_power->up() && p()->cooldown.burst_of_power_icd->up() )
    {
      p()->cooldown.burst_of_power_icd->start();
      p()->buff.burst_of_power->decrement();
      p()->resource_gain( RESOURCE_RAGE, rage_from_burst_of_power, p()->gain.burst_of_power );
      // Reset CD after everything resolves
      make_event( *p()->sim, [ this ] { p()->cooldown.bloodbath->reset( true );
                                        p()->cooldown.bloodthirst->reset( true ); } );
    }

    if ( p()->talents.slayer.reap_the_storm->ok() )
    {
      if ( p()->cooldown.reap_the_storm_icd->is_ready() && rng().roll( p()->talents.slayer.reap_the_storm->proc_chance() ) )
      {
        reap_the_storm->execute();
        p()->cooldown.reap_the_storm_icd->start();
      }
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = warrior_attack_t::composite_da_multiplier( s );

    if ( p()->tier_set.t31_fury_2pc->ok() && p()->buff.furious_bloodthirst->up() && s->chain_target == 0 )
    {
      da *= 1 + p()->spell.furious_bloodthirst->effectN( 1 ).percent();
    }

    return da;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = warrior_attack_t::composite_target_crit_chance( target );

    if ( p()->tier_set.t31_fury_2pc->ok() && p()->buff.furious_bloodthirst->up() && target == p()->target )
      c += p() -> spell.furious_bloodthirst -> effectN( 2 ).percent();

    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( !unhinged )
      p()->buff.meat_cleaver->decrement();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( bloodthirst_heal )
      {
        bloodthirst_heal->execute();
      }

      if ( rng().roll( enrage_chance ) )
      {
        p()->enrage();
      }
    }
    if( !td( execute_state->target )->hit_by_fresh_meat )
    {
      p()->buff.enrage->trigger();
      td( execute_state->target )->hit_by_fresh_meat = true;
    }

    if ( p()->buff.enrage->up() )
    {
      p()->buff.enrage->extend_duration( p(), p()->talents.fury.deft_experience->effectN( 2 ).time_value() );
    }

    p()->buff.fierce_followthrough->expire();

    if ( p()->sets->has_set_bonus( WARRIOR_FURY, TWW1, B2 ) )
      p()->buff.bloody_rampage->trigger();

    p()->buff.deep_thirst->expire();

    p()->buff.furious_bloodthirst->decrement();
    p()->buff.merciless_assault->expire();

    if ( p()->talents.mountain_thane.thunder_blast->ok() && rng().roll( p()->talents.mountain_thane.thunder_blast->effectN( 1 ).percent() ) )
    {
      p()->buff.thunder_blast->trigger();
    }
  }

  bool ready() override
  {
    if ( p()->buff.bloodbath->check() && !background )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Bloodbath ==============================================================

struct bloodbath_t : public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  warrior_attack_t* gushing_wound;
  int aoe_targets;
  double enrage_chance;
  double rage_from_cold_steel_hot_blood;
  double rage_from_merciless_assault;
  double rage_from_burst_of_power;
  action_t* reap_the_storm;
  bool unhinged;
  bloodbath_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "bloodbath", p, p->spec.bloodbath ),
      bloodthirst_heal( nullptr ),
      gushing_wound( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      enrage_chance( p->spec.enrage->effectN( 2 ).percent() ),
      rage_from_cold_steel_hot_blood( p->find_spell( 383978 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_merciless_assault( p->find_spell( 409983 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_burst_of_power( 0 ),
      reap_the_storm( nullptr ),
      unhinged( false )
  {
    parse_options( options_str );

    weapon = &( p->main_hand_weapon );
    radius = 5;
    cooldown = p->cooldown.bloodthirst;
    track_cd_waste = true;
    if ( p->non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();

    if ( p->talents.fury.deft_experience->ok() )
    {
      enrage_chance += p->talents.fury.deft_experience->effectN( 2 ).percent();
    }

    if ( p->talents.fury.fresh_meat->ok() )
    {
      enrage_chance += p->talents.fury.fresh_meat->effectN( 1 ).percent();
    }

    if ( p->talents.fury.cold_steel_hot_blood.ok() )
    {
      gushing_wound = new gushing_wound_dot_t( p );
    }

    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }

    if ( p->talents.slayer.reap_the_storm->ok() )
    {
      reap_the_storm = get_action<reap_the_storm_t>( "reap_the_storm_bloodbath", p );
      add_child( reap_the_storm );
    }

    if ( p->talents.mountain_thane.burst_of_power->ok() )
    {
      rage_from_burst_of_power = p->talents.mountain_thane.burst_of_power->effectN( 1 ).trigger()->effectN( 3 ).resource( RESOURCE_RAGE );
    }
  }

  // Background version for use with unhinged
  bloodbath_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->spec.bloodbath ),
      bloodthirst_heal( nullptr ),
      gushing_wound( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      enrage_chance( p->spec.enrage->effectN( 2 ).percent() ),
      rage_from_cold_steel_hot_blood( p->find_spell( 383978 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_burst_of_power( 0 ),
      unhinged( false )
  {
    background = true;

    weapon = &( p->main_hand_weapon );
    radius = 5;
    if ( p->non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();

    if ( p->talents.fury.deft_experience->ok() )
    {
      enrage_chance += p->talents.fury.deft_experience->effectN( 2 ).percent();
    }

    if ( p->talents.fury.fresh_meat->ok() )
    {
      enrage_chance += p->talents.fury.fresh_meat->effectN( 1 ).percent();
    }

    if ( p->talents.fury.cold_steel_hot_blood.ok() )
    {
      gushing_wound = new gushing_wound_dot_t( p );
    }

    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }

    if ( p->talents.slayer.reap_the_storm->ok() )
    {
      std::string s = "reap_the_storm_";
      s += name;
      reap_the_storm = get_action<reap_the_storm_t>( s, p );
      add_child( reap_the_storm );
    }

    if ( p->talents.mountain_thane.burst_of_power->ok() )
    {
      rage_from_burst_of_power = p->talents.mountain_thane.burst_of_power->effectN( 1 ).trigger()->effectN( 3 ).resource( RESOURCE_RAGE );
    }
  }

  // Constructor for unhinged specfic version, so we can disable sweeping strikes, as well as have a bool to check
  bloodbath_t( util::string_view name, warrior_t* p, bool unhinged )
    : bloodbath_t( name, p )
  {
    this->unhinged = unhinged;
  }

  int n_targets() const override
  {
    if ( !unhinged && p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.fury.vicious_contempt->ok() && ( target->health_percentage() < 35 ) )
    {
      am *= 1.0 + ( p()->talents.fury.vicious_contempt->effectN( 1 ).percent() );
    }

    return am;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    // Delayed event to cancel the stack on any crit results
    if ( p()->talents.fury.bloodcraze->ok() && s->result == RESULT_CRIT )
      make_event( *p()->sim, [ this ] { p()->buff.bloodcraze->expire(); } );


    if ( gushing_wound && s->result == RESULT_CRIT )
    {
      gushing_wound->set_target( s->target );
      gushing_wound->execute();
    }

    if ( p()->talents.fury.cold_steel_hot_blood.ok() && execute_state->result == RESULT_CRIT &&
         p()->cooldown.cold_steel_hot_blood_icd->up() )
    {
      p()->cooldown.cold_steel_hot_blood_icd->start();
    }

    if ( p()->tier_set.t30_fury_4pc->ok() && target == s->target )
    {
      p()->resource_gain( RESOURCE_RAGE, p()->buff.merciless_assault->stack() * rage_from_merciless_assault,
                          p()->gain.merciless_assault );
    }

    if ( p()->tier_set.t31_fury_4pc->ok() && s->result == RESULT_CRIT && p()->cooldown.t31_fury_4pc_icd->up() )
    {
      p()->cooldown.odyns_fury->adjust( - timespan_t::from_millis( p()->spell.t31_fury_4pc->effectN( 3 ).base_value() ) );
      p()->cooldown.t31_fury_4pc_icd->start();
    }

    // We schedule this one to trigger after the action fully resolves, as we need to expire the buff if it already exists
    if ( p()->talents.slayer.fierce_followthrough->ok() && s->result == RESULT_CRIT && s->chain_target == 0 )
      make_event( sim, [ this ] { p()->buff.fierce_followthrough->trigger(); } );

    if ( p()->talents.mountain_thane.burst_of_power->ok() && p()->buff.burst_of_power->up() && p()->cooldown.burst_of_power_icd->up() )
    {
      p()->cooldown.burst_of_power_icd->start();
      p()->buff.burst_of_power->decrement();
      p()->resource_gain( RESOURCE_RAGE, rage_from_burst_of_power, p()->gain.burst_of_power );
      make_event( *p()->sim, [ this ] { p()->cooldown.bloodbath->reset( true );
                                        p()->cooldown.bloodthirst->reset( true ); } );
    }

    if ( p()->talents.slayer.reap_the_storm->ok() )
    {
      if ( p()->cooldown.reap_the_storm_icd->is_ready() && rng().roll( p()->talents.slayer.reap_the_storm->proc_chance() ) )
      {
        reap_the_storm->execute();
        p()->cooldown.reap_the_storm_icd->start();
      }
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = warrior_attack_t::composite_da_multiplier( s );

    if ( p()->tier_set.t31_fury_2pc->ok() && p()->buff.furious_bloodthirst->up() && s->chain_target == 0 )
    {
      da *= 1 + p()->spell.furious_bloodthirst->effectN( 1 ).percent();
    }

    return da;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = warrior_attack_t::composite_target_crit_chance( target );

    if ( p()->tier_set.t31_fury_2pc->ok() && p()->buff.furious_bloodthirst->up() && target == p()->target )
      c += p() -> spell.furious_bloodthirst -> effectN( 2 ).percent();

    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.bloodbath->decrement();
    if ( !unhinged )
      p()->buff.meat_cleaver->decrement();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( bloodthirst_heal )
      {
        bloodthirst_heal->execute();
      }

      if ( rng().roll( enrage_chance ) )
      {
        p()->enrage();
      }

      p()->buff.deep_thirst->expire();
    }

    p()->buff.fierce_followthrough->expire();

    if ( p()->sets->has_set_bonus( WARRIOR_FURY, TWW1, B2 ) )
      p()->buff.bloody_rampage->trigger();

    p()->buff.furious_bloodthirst->decrement();
    p()->buff.merciless_assault->expire();

    if ( p()->talents.mountain_thane.thunder_blast->ok() && rng().roll( p()->talents.mountain_thane.thunder_blast->effectN( 1 ).percent() ) )
    {
      p()->buff.thunder_blast->trigger();
    }
  }

  bool ready() override
  {
    if ( !p()->buff.bloodbath->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Mortal Strike ============================================================
struct crushing_advance_t : warrior_attack_t
{
  crushing_advance_t( util::string_view name, warrior_t* p ) : warrior_attack_t( name, p, p->find_spell( 411703 ) )
  {
    aoe                 = -1;
    reduced_aoe_targets = 5.0;
    background          = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->buff.crushing_advance->stack() > 1 )
    {
      am *= 1.0 + ( p()->buff.crushing_advance->stack() - 1 ) * 0.5;
    }
    // gains a 50% damage bonus for each stack beyond the first
    // 1 stack = base damage, 2 stack = +50%, 3 stack = +100%
    // Not in spell data

    return am;
  }
};

struct mortal_strike_t : public warrior_attack_t
{
  double cost_rage;
  double exhilarating_blows_chance;
  double frothing_berserker_chance;
  double rage_from_frothing_berserker;
  warrior_attack_t* rend_dot;
  warrior_attack_t* crushing_advance;
  action_t* reap_the_storm;
  bool unhinged;
  mortal_strike_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "mortal_strike", p, p->talents.arms.mortal_strike ),
      exhilarating_blows_chance( p->talents.arms.exhilarating_blows->proc_chance() ),
      frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
      rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).percent() ),
      rend_dot( nullptr ),
      crushing_advance( nullptr ),
      reap_the_storm( nullptr ),
      unhinged( false )
  {
    parse_options( options_str );

    weapon           = &( p->main_hand_weapon );
    cooldown->hasted = true;  // Doesn't show up in spelldata for some reason.
    impact_action    = p->active.deep_wounds_ARMS;
    rend_dot = new rend_dot_t( p );
    if ( p->talents.slayer.reap_the_storm->ok() )
    {
      reap_the_storm = get_action<reap_the_storm_t>( "reap_the_storm_mortal_strike", p );
      add_child( reap_the_storm );
    }

    if ( p->tier_set.t30_arms_4pc->ok() )
    {
      crushing_advance = new crushing_advance_t( "crushing_advance", p );
    }
  }

  // This version is used for unhinged and other background actions
  mortal_strike_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->talents.arms.mortal_strike ),
      exhilarating_blows_chance( p->talents.arms.exhilarating_blows->proc_chance() ),
      frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
      rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).percent() ),
      rend_dot( nullptr ),
      unhinged( true )
  {
    background = true;
    impact_action = p->active.deep_wounds_ARMS;
    rend_dot = new rend_dot_t( p );
    internal_cooldown->duration = 0_s;
    if ( p->talents.slayer.reap_the_storm->ok() )
    {
      std::string s = "reap_the_storm_";
      s += name;
      reap_the_storm = get_action<reap_the_storm_t>( s, p );
      add_child( reap_the_storm );
    }
    if ( p->tier_set.t30_arms_4pc->ok() )
    {
      crushing_advance = new crushing_advance_t( "crushing_advance_unhinged", p );
    }
  }

  // This version is used for unhinged, to set the variable, as unhinged does not cleave
  mortal_strike_t( util::string_view name, warrior_t* p, bool unhinged )
    : mortal_strike_t( name, p )
  {
    this->unhinged = unhinged;
  }

  void init() override
  {
    warrior_attack_t::init();
    if ( unhinged )
    {
      affected_by.sweeping_strikes = false;
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.martial_prowess->check_stack_value();

    return am;
  }

  double cost() const override
  {
    if ( background )
      return 0;
    return warrior_attack_t::cost();
  }

  double tactician_cost() const override
  {
    if ( background )
      return 0;
    return warrior_attack_t::tactician_cost();
  }

  void execute() override
  {
    warrior_attack_t::execute();

    cost_rage = last_resource_cost;
    if ( !background && p()->talents.warrior.frothing_berserker->ok() && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }

    if ( result_is_hit( execute_state->result ) )
    {
      if ( !sim->overrides.mortal_wounds && execute_state->target->debuffs.mortal_wounds )
      {
        execute_state->target->debuffs.mortal_wounds->trigger();
      }
    }

    if ( p()->talents.arms.exhilarating_blows->ok() && rng().roll( exhilarating_blows_chance ) )
    {
      p()->cooldown.mortal_strike->reset( true );
      p()->cooldown.cleave->reset( true );
    }

    if ( crushing_advance && p()->buff.crushing_advance->check() )
    {
      // crushing_advance->set_target( s->target );
      crushing_advance->execute();
    }

    p()->buff.crushing_advance->expire();

    p()->buff.martial_prowess->expire();

    p()->buff.brutal_finish->expire();

    p()->buff.fierce_followthrough->expire();

    if ( p()->sets->has_set_bonus( WARRIOR_ARMS, TWW1, B2 ) )
    {
      p()->buff.overpowering_might->trigger();
    }

    p()->buff.lethal_blows->expire();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( p()->talents.arms.fatality->ok() && p()->rppm.fatal_mark->trigger() && target->health_percentage() > 30 )
    { // does this eat RPPM when switching from low -> high health target?
      td( s->target )->debuffs_fatal_mark->trigger();
    }

    if ( td( s->target )->debuffs_executioners_precision->up() )
    {
      td( s->target )->debuffs_executioners_precision->expire();
    }

    if ( p()->talents.arms.bloodletting->ok() && ( target->health_percentage() < 35 ) )
    {
      rend_dot->set_target( s->target );
      rend_dot->execute();
    }

    // We schedule this one to trigger after the action fully resolves, as we need to expire the buff if it already exists
    if ( p()->talents.slayer.fierce_followthrough->ok() && s->result == RESULT_CRIT && s->chain_target == 0 )
      make_event( sim, [ this ] { p()->buff.fierce_followthrough->trigger(); } );

    if ( p()->talents.colossus.colossal_might->ok() )
    {
      // Gain 2 stacks on a crit with precise might, 1 otherwise.
      if ( p()->talents.colossus.precise_might->ok() && s->result == RESULT_CRIT )
      {
        if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
        {
          p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() * 2 ) );
        }
        p()->buff.colossal_might->trigger( 2 );
      }
      else
      {
        if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
        {
          p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() ) );
        }
        p()->buff.colossal_might->trigger();
      }
      // If this is an unhinged MS, and we have at least 2 targets, and sweeping strikes is up, grant an extra stack.  This is a bug.
      if ( p()->bugs && this->unhinged && p()->buff.sweeping_strikes->up() && p()->sim->target_non_sleeping_list.size() > 1 )
      {
        if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
        {
          p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() ) );
        }
        p()->buff.colossal_might->trigger();
      }
    }

    if ( p()->tier_set.t29_arms_4pc->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.strike_vulnerabilities->trigger();
    }

    if ( p()->talents.slayer.reap_the_storm->ok() )
    {
      if ( p()->cooldown.reap_the_storm_icd->is_ready() && rng().roll( p()->talents.slayer.reap_the_storm->proc_chance() ) )
      {
        reap_the_storm->execute();
        p()->cooldown.reap_the_storm_icd->start();
      }
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Bladestorm ===============================================================

struct bladestorm_tick_t : public warrior_attack_t
{
  double rage_from_storm_of_steel;
  bladestorm_tick_t( warrior_t* p, util::string_view name, const spell_data_t* spell )
    : warrior_attack_t( name, p, spell ),
      rage_from_storm_of_steel( 0.0 )
  {
    dual = true;
    aoe = -1;
    reduced_aoe_targets = 8.0;
    background = true;
    if ( p->specialization() == WARRIOR_ARMS )
    {
      impact_action = p->active.deep_wounds_ARMS;
      // Arms does not generate rage when bladestorming
      energize_amount = 0;
      energize_resource = RESOURCE_NONE;
    }
    rage_from_storm_of_steel += p->talents.fury.storm_of_steel -> effectN( 6 ).resource( RESOURCE_RAGE );
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = warrior_attack_t::composite_da_multiplier( state );

    if ( p()->talents.slayer.culling_cyclone->ok() )
      m *= 1.0 + ( p()->talents.slayer.culling_cyclone->effectN( 1 ).percent() / p()->sim->target_non_sleeping_list.size() );

    return m;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p()->talents.slayer.overwhelming_blades->ok() && data().id() == 50622 ) // 50622 is MH bladestorm attack.  We only proc overwhelmed debuff from MH hits
    {
      td( state->target )->debuffs_overwhelmed->trigger();
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( execute_state->n_targets > 0 )
      p()->resource_gain( RESOURCE_RAGE, rage_from_storm_of_steel, p()->gain.storm_of_steel );
  }
};

struct bladestorm_t : public warrior_attack_t
{
  attack_t *bladestorm_mh, *bladestorm_oh;
  mortal_strike_t* mortal_strike;
  bloodthirst_t* bloodthirst;
  bloodbath_t* bloodbath;

  bladestorm_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_attack_t( n, p, spell ),
    bladestorm_mh( new bladestorm_tick_t( p, fmt::format( "{}_mh", n ), spell->effectN( 1 ).trigger() ) ),
      bladestorm_oh( nullptr ),
      mortal_strike( nullptr ),
      bloodthirst( nullptr ),
      bloodbath( nullptr )
  {
    parse_options( options_str );
    channeled = false;
    tick_zero = true;
    interrupt_auto_attack = false;
    travel_speed                      = 0;
    internal_cooldown->duration = 0_s; // allow Anger Management to reduce the cd properly due to having both charges and cooldown entries

    bladestorm_mh->weapon             = &( player->main_hand_weapon );
    add_child( bladestorm_mh );
    if ( player->off_hand_weapon.type != WEAPON_NONE && player->specialization() == WARRIOR_FURY )
    {
      bladestorm_oh         = new bladestorm_tick_t( p, fmt::format( "{}_oh", n ), spell->effectN( 1 ).trigger()->effectN( 2 ).trigger() );
      bladestorm_oh->weapon = &( player->off_hand_weapon );
      add_child( bladestorm_oh );
    }

    // Unhinged DOES work w/ Torment and Signet
    if ( p->talents.arms.unhinged->ok() )
    {
      mortal_strike = new mortal_strike_t( "mortal_strike_bladestorm_unhinged", p, true );
      add_child( mortal_strike );
    }

    if ( p->talents.fury.unhinged->ok() )
    {
      bloodthirst = new bloodthirst_t( "bloodthirst_bladestorm_unhinged", p, true );
      add_child( bloodthirst );

      if ( p->talents.fury.reckless_abandon->ok() )
      {
        bloodbath = new bloodbath_t( "bloodbath_bladestorm_unhinged", p, true );
        add_child( bloodbath );
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.bladestorm->trigger();
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto new_dot_duration = warrior_attack_t::composite_dot_duration( s );

    if ( p() -> talents.slayer.imminent_demise -> ok() )
    {
      new_dot_duration = tick_time( s ) * ( dot_duration.total_seconds() + p() -> buff.imminent_demise -> stack() );
    }

    return new_dot_duration;
  }

  timespan_t tick_time ( const action_state_t* s ) const override
  {
    auto new_base_tick_time = warrior_attack_t::tick_time( s );

    // Normally we get 6 ticks of bladestorm, but with imminent demise, we get 1-3 extra ticks, in the same amount of time
    if ( p() -> talents.slayer.imminent_demise->ok() )
    {
      new_base_tick_time *= ( dot_duration.total_seconds() / ( dot_duration.total_seconds() + p() -> buff.imminent_demise -> stack() ) );
    }

    return new_base_tick_time;
  }

  void tick( dot_t* d ) override
  {
    // dont tick if BS buff not up
    // since first tick is instant the buff won't be up yet
    if ( d->ticks_left() < d->num_ticks() && !p()->buff.bladestorm->up() )
    {
      make_event( sim, [ d ] { d->cancel(); } );
      return;
    }

    warrior_attack_t::tick( d );
    // As of TWW, since bladestorm has an initial tick, unhinged procs on odd ticks
    if ( ( mortal_strike || bloodthirst || bloodbath ) && ( d->current_tick % 2 == 1 ) )
    {
      auto t = p() -> target;
      if ( ! p() -> target || p() -> target->is_sleeping() )
        t = select_random_target();

      if ( t )
      {
        if ( mortal_strike )
          mortal_strike->execute_on_target( t );
        if ( bloodthirst || bloodbath )
        {
          if ( bloodbath && p()->buff.bloodbath->check() )
            bloodbath->execute_on_target( t );
          else
            bloodthirst->execute_on_target( t );
        }
      }
    }

    bladestorm_mh->execute();

    if ( bladestorm_mh->result_is_hit( execute_state->result ) && bladestorm_oh )
    {
      bladestorm_oh->execute();
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );
    p()->buff.bladestorm->expire();

    if ( p()->talents.arms.merciless_bonegrinder->ok() )
    {
      p()->buff.merciless_bonegrinder->trigger();
    }

    if ( p() -> talents.shared.dance_of_death->ok() && p()->buff.dance_of_death_bladestorm->up() )
    {
      p()->buff.dance_of_death_bladestorm -> trigger( -1, p() -> spell.dance_of_death_bs_buff->duration() );
    }

    if ( p()->talents.slayer.imminent_demise->ok() )
    {
      p()->buff.imminent_demise->expire();
    }

    if ( p()->talents.slayer.brutal_finish->ok() )
    {
      p()->buff.brutal_finish->trigger();
    }
  }
};

// Onslaught ==============================================================

struct onslaught_t : public warrior_attack_t
{
  double unbridled_chance;
  const spell_data_t* damage_spell;
  int aoe_targets;
  onslaught_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "onslaught", p, p->talents.fury.onslaught ),
      unbridled_chance( p->talents.fury.unbridled_ferocity->effectN( 1 ).base_value() / 100.0 ),
      damage_spell( p->find_spell( 396718U ) ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    weapon              = &( p->main_hand_weapon );
    radius              = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
    attack_power_mod.direct = damage_spell->effectN( 1 ).ap_coeff();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void execute() override
  {
    if ( p()->talents.fury.tenderize->ok() )
    {
      p()->enrage();
      if ( p()->talents.fury.slaughtering_strikes->ok() )
        p()->buff.slaughtering_strikes->trigger( as<int>( p()->talents.fury.tenderize->effectN( 1 ).base_value() ) );
    }

    if ( p()->talents.fury.unbridled_ferocity.ok() && rng().roll( unbridled_chance ) )
    {
      const timespan_t trigger_duration = p()->talents.fury.unbridled_ferocity->effectN( 2 ).time_value();
      p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
    }

    warrior_attack_t::execute();

    p()->buff.meat_cleaver->decrement();

  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
      return false;
    return warrior_attack_t::ready();
  }
};

// Charge ===================================================================

struct charge_t : public warrior_attack_t
{
  // Split the damage, resource gen and other triggered effects from the movement part of Charge
  struct charge_impact_t : public warrior_attack_t
  {
    const spell_data_t* damage_spell;

    charge_impact_t( warrior_t* p ) :
      warrior_attack_t( "charge_impact", p, p -> spell.charge ),
      damage_spell( p->find_spell( 126664 ) )
    {
      background = true;

      energize_resource       = RESOURCE_RAGE;
      energize_type           = action_energize::ON_CAST;
      attack_power_mod.direct = damage_spell->effectN( 1 ).ap_coeff();
    }
  };

  bool first_charge;
  double movement_speed_increase, min_range;
  charge_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "charge", p, p->spell.charge ),
      first_charge( true ),
      movement_speed_increase( 5.0 ),
      min_range( data().min_range() )
  {
    parse_options( options_str );
    movement_directionality = movement_direction_type::OMNI;

    impact_action = new charge_impact_t( p );

    // Rage gen is handled in the impact action
    energize_amount = 0;
    energize_resource = RESOURCE_NONE;

    if ( p->talents.warrior.double_time->ok() )
    {
      cooldown->charges += as<int>( p->talents.warrior.double_time->effectN( 1 ).base_value() );
      cooldown->duration += p->talents.warrior.double_time->effectN( 2 ).time_value();
    }
  }

  timespan_t calc_charge_time( double distance )
  {
    return timespan_t::from_seconds( distance /
      ( p()->base_movement_speed * ( 1 + p()->stacking_movement_modifier() + movement_speed_increase ) ) );
  }

  void execute() override
  {
    if ( p()->current.distance_to_move > 5 )
    {
      p()->buff.charge_movement->trigger( 1, movement_speed_increase, 1, calc_charge_time( p()->current.distance_to_move ) );
      p()->current.moving_away = 0;
    }

    warrior_attack_t::execute();

  }

  void reset() override
  {
    warrior_attack_t::reset();
    first_charge = true;
  }

  bool ready() override
  {
    if ( first_charge )  // Assumes that we charge into combat, instead of setting initial distance to 20 yards.
    {
      return warrior_attack_t::ready();
    }
    if ( p()->current.distance_to_move < min_range )  // Cannot charge if too close to the target.
    {
      return false;
    }
    if ( p()->buff.charge_movement->check() || p()->buff.heroic_leap_movement->check() ||
         p()->buff.intervene_movement->check() || p() -> buff.shield_charge_movement->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Slam =====================================================================

struct slam_t : public warrior_attack_t
{
  bool from_Fervor;
  int aoe_targets;
  slam_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "slam", p, p->spell.slam ), from_Fervor( false ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    weapon                       = &( p->main_hand_weapon );
    radius = 5;
    if ( player->specialization() == WARRIOR_FURY )
    {
      base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
    }
  }

  slam_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->spell.slam ), from_Fervor( false ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    background = true;
    weapon                       = &( p->main_hand_weapon );
    radius = 5;
    if ( player->specialization() == WARRIOR_FURY )
    {
      base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
    }
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double cost() const override
  {
    if ( from_Fervor )
      return 0;

    return warrior_attack_t::cost();
  }

  double tactician_cost() const override
  {
    if ( from_Fervor )
      return 0;
    return warrior_attack_t::cost();
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.meat_cleaver->decrement();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Cleave ===================================================================
struct cleave_seismic_reverberation_t : public warrior_attack_t
{
  cleave_seismic_reverberation_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 458459 ) )
  {
    weapon = &( player->main_hand_weapon );
    aoe = -1;
    reduced_aoe_targets = 5.0;
    background = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.martial_prowess->check_stack_value();

    if ( !p()->buff.sweeping_strikes->up() && p()->buff.collateral_damage->up() )
    {
      am *= 1.0 + p()->buff.collateral_damage->stack_value();
    }

    am *= 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 3 ).percent();

    return am;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = warrior_attack_t::composite_da_multiplier( state );

    if ( p()->talents.colossus.one_against_many->ok() )
    {
      m *= 1.0 + ( p()->talents.colossus.one_against_many->effectN( 1 ).percent() * std::min( state -> n_targets,  as<unsigned int>( p()->talents.colossus.one_against_many->effectN( 2 ).base_value() ) ) );
    }

    return m;
  }

};

struct cleave_t : public warrior_attack_t
{
  slam_t* fervor_slam;
  double cost_rage;
  double frothing_berserker_chance;
  double rage_from_frothing_berserker;
  action_t* reap_the_storm;
  action_t* seismic_action;
  cleave_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "cleave", p, p->talents.arms.cleave ),
    fervor_slam( nullptr ),
    frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
    rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).percent() ),
    reap_the_storm( nullptr ),
    seismic_action( nullptr )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
    aoe = -1;
    reduced_aoe_targets = 5.0;

    if ( p->talents.arms.fervor_of_battle->ok() )
    {
      fervor_slam                               = new slam_t( "slam_cleave_fervor_of_battle", p );
      fervor_slam->from_Fervor                  = true;
      add_child( fervor_slam );
    }
    if ( p->talents.slayer.reap_the_storm->ok() )
    {
      reap_the_storm = get_action<reap_the_storm_t>( "reap_the_storm_cleave", p );
      add_child( reap_the_storm );
    }
    if ( p->talents.warrior.seismic_reverberation->ok() )
    {
      seismic_action = new cleave_seismic_reverberation_t( "cleave_seismic_reverberation", p );
      add_child( seismic_action );
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.martial_prowess->check_stack_value();

    if ( !p()->buff.sweeping_strikes->up() && p()->buff.collateral_damage->up() )
    {
      am *= 1.0 + p()->buff.collateral_damage->stack_value();
    }

    return am;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = warrior_attack_t::composite_da_multiplier( state );

    if ( p()->talents.colossus.one_against_many->ok() )
    {
      m *= 1.0 + ( p()->talents.colossus.one_against_many->effectN( 1 ).percent() * std::min( state -> n_targets,  as<unsigned int>( p()->talents.colossus.one_against_many->effectN( 2 ).base_value() ) ) );
    }

    return m;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    if ( execute_state->n_targets >= 1 )
    {
      p()->active.deep_wounds_ARMS->set_target( s->target );
      p()->active.deep_wounds_ARMS->execute();
    }
    if ( p()->talents.arms.fatality->ok() && p()->rppm.fatal_mark->trigger() && target->health_percentage() > 30 )
    {  // does this eat RPPM when switching from low -> high health target?
      td( s->target )->debuffs_fatal_mark->trigger();
    }
    if ( p()->tier_set.t29_arms_4pc->ok() && s->result == RESULT_CRIT )
    {
      p()->buff.strike_vulnerabilities->trigger();
    }

    if ( p()->talents.slayer.reap_the_storm->ok() )
    {
      if ( p()->cooldown.reap_the_storm_icd->is_ready() && rng().roll( p()->talents.slayer.reap_the_storm->proc_chance() ) )
      {
        reap_the_storm->execute();
        p()->cooldown.reap_the_storm_icd->start();
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.warrior.seismic_reverberation->ok() &&
          execute_state->n_targets >= p()->talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      seismic_action->execute_on_target( target );
    }

    cost_rage = last_resource_cost;
    if ( p()->talents.warrior.frothing_berserker->ok() && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }
    p()->buff.martial_prowess->expire();

    if ( p()->talents.arms.collateral_damage.ok() && !p()->buff.sweeping_strikes->up() && p()->buff.collateral_damage->up()  )
    {
      p() -> buff.collateral_damage -> expire();
    }

    if ( p() -> talents.arms.fervor_of_battle.ok() && num_targets_hit >= p() -> talents.arms.fervor_of_battle -> effectN( 1 ).base_value() )
      fervor_slam->execute_on_target( target );

    if ( p()->talents.colossus.colossal_might->ok() && execute_state -> n_targets >= p()->talents.colossus.colossal_might->effectN( 1 ).base_value() )
    {
      if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
      {
        p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() ) );
      }
      p()->buff.colossal_might->trigger();
    }

    if ( p()->sets->has_set_bonus( WARRIOR_ARMS, TWW1, B2 ) )
    {
      p()->buff.overpowering_might->trigger();
    }

    p()->buff.lethal_blows->expire();
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t : public warrior_attack_t
{
  colossus_smash_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "colossus_smash", p, p->talents.arms.colossus_smash )
  {
    if ( p->talents.arms.warbreaker->ok() )
    {
      background = true;  // Warbreaker replaces Colossus Smash for Arms
    }
    else
    {
      parse_options( options_str );
      weapon = &( player->main_hand_weapon );
      impact_action    = p->active.deep_wounds_ARMS;
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_colossus_smash->trigger();
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      p()->buff.test_of_might_tracker->trigger();

      if ( p()->talents.arms.in_for_the_kill->ok() )
        p()->buff.in_for_the_kill->trigger();
    }
  }
};

// Deep Wounds ARMS ==============================================================

struct deep_wounds_ARMS_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  deep_wounds_ARMS_t( warrior_t* p ) : warrior_attack_t( "deep_wounds", p, p->find_spell( 262115 ) ),
    bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
    rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }

    if ( p()->tier_set.t30_arms_4pc->ok() && d->state->result == RESULT_CRIT )
    {
      p()->buff.crushing_advance->trigger();
    }
  }

  timespan_t tick_time ( const action_state_t* s ) const override
  {
    auto base_tick_time = warrior_attack_t::tick_time( s );

    auto td = p() -> get_target_data( s -> target );
    if ( td -> debuffs_skullsplitter -> up() )
      base_tick_time *= 1 / ( 1.0 + td -> debuffs_skullsplitter -> value() );

    return base_tick_time;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto dot_duration = warrior_attack_t::composite_dot_duration( s );

    auto td = p() -> get_target_data( s -> target );
    if ( td -> debuffs_skullsplitter -> up() )
      dot_duration *= 1 / ( 1.0 + td -> debuffs_skullsplitter -> value() );

    return dot_duration;
  }
};

// Deep Wounds PROT ==============================================================

struct deep_wounds_PROT_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  deep_wounds_PROT_t( warrior_t* p )
    : warrior_attack_t( "deep_wounds", p, p->spec.deep_wounds_PROT->effectN( 1 ).trigger() ),
    bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
    rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
  }
};

// Demolish =================================================================

struct demolish_damage_t : public warrior_attack_t
{
  demolish_damage_t( util::string_view name, warrior_t* p,  const spell_data_t* demolish )
    : warrior_attack_t( name, p, demolish )
  {
    background = true;
    dual = true;
    // Third attack is aoe
    if ( data().id() == 440888 )
    {
      aoe = -1;
      reduced_aoe_targets = p->talents.colossus.demolish->effectN( 1 ).base_value();
    }
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->up() )
    {
      td( state->target )->debuffs_wrecked->trigger( p()->buff.colossal_might->stack() );
    }
  }
};

struct demolish_t : public warrior_attack_t
{
  demolish_damage_t* demolish_first_attack;
  demolish_damage_t* demolish_second_attack;
  demolish_damage_t* demolish_third_attack;
  demolish_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "demolish", p, p->talents.colossus.demolish ),
      demolish_first_attack( nullptr ),
      demolish_second_attack( nullptr ),
      demolish_third_attack( nullptr )
    {
      parse_options( options_str );
      channeled = true;
      hasted_ticks = true;
      weapon = &( player->main_hand_weapon );
      demolish_first_attack = new demolish_damage_t( "demolish_first_attack", p, p->find_spell( 440884 ) );
      demolish_second_attack = new demolish_damage_t( "demolish_second_attack", p, p->find_spell( 440886 ) );
      demolish_third_attack = new demolish_damage_t( "demolish_third_attack", p, p->find_spell( 440888 ) );
      add_child( demolish_first_attack );
      add_child( demolish_second_attack );
      add_child( demolish_third_attack );
    }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( d->current_tick == 1 )
    {
      demolish_first_attack->execute();
    }
    else if ( d->current_tick == 3 )
    {
      demolish_second_attack->execute();
    }
    else if ( d->current_tick == 8 )
    {
      demolish_third_attack->execute();
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );
    p()->buff.colossal_might->expire();
  }
};

// Demoralizing Shout =======================================================

struct demoralizing_shout_t : public warrior_attack_t
{
  double rage_gain;
  demoralizing_shout_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "demoralizing_shout", p, p->talents.protection.demoralizing_shout ), rage_gain( 0 )
  {
    parse_options( options_str );
    usable_while_channeling = true;
    rage_gain += p->talents.protection.booming_voice->effectN( 1 ).resource( RESOURCE_RAGE );
    aoe = -1;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( rage_gain > 0 )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_gain, p()->gain.booming_voice );
    }

    if ( p()->talents.mountain_thane.snap_induction->ok() )
    {
      p()->buff.thunder_blast->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    td( s->target ) -> debuffs_demoralizing_shout -> trigger(
      1, data().effectN( 1 ).percent(), 1.0, p()->talents.protection.demoralizing_shout->duration()  );
  }
};

// Thunderous Roar ==============================================================

struct thunderous_roar_dot_t : public warrior_attack_t
{
  double bloodsurge_chance, rage_from_bloodsurge;
  thunderous_roar_dot_t( warrior_t* p )
    : warrior_attack_t( "thunderous_roar_dot", p, p->find_spell( 397364 ) ),
      bloodsurge_chance( p->talents.shared.bloodsurge->proc_chance() ),
      rage_from_bloodsurge( p->talents.shared.bloodsurge->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    //hasted_ticks               = false; //currently hasted in game - likely unintended
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    if ( p()->talents.shared.bloodsurge->ok() && rng().roll( bloodsurge_chance ) )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_bloodsurge, p()->gain.bloodsurge );
    }
  }
};

struct thunderous_roar_t : public warrior_attack_t
{
  warrior_attack_t* thunderous_roar_dot;
  thunderous_roar_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "thunderous_roar", p, p->talents.warrior.thunderous_roar ),
      thunderous_roar_dot( nullptr )
  {
    parse_options( options_str );
    aoe       = -1;
    reduced_aoe_targets = p->talents.warrior.thunderous_roar->effectN( 3 ).base_value();
    may_dodge = may_parry = may_block = false;

    thunderous_roar_dot   = new thunderous_roar_dot_t( p );
    add_child( thunderous_roar_dot );

  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    thunderous_roar_dot->set_target( s->target );
    thunderous_roar_dot->execute();
  }
};

// Thunder Clap =============================================================

struct thunder_blast_seismic_reverberation_t : public warrior_attack_t
{
  thunder_blast_seismic_reverberation_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 436793 ) )
  {
    background = true;
    may_dodge = may_parry = may_block = false;
    may_crit = false;
    tick_may_crit = false;

    energize_type = action_energize::NONE;
  }
};

struct thunder_blast_t : public warrior_attack_t
{
  double rage_gain;
  double shield_slam_reset;
  warrior_attack_t* rend;
  action_t* lightning_strike;
  action_t* seismic_action;
  double rend_target_cap;
  double rend_targets_hit;
  thunder_blast_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "thunder_blast", p, p->find_spell( 435222 ) ),
      rage_gain( data().effectN( 4 ).resource( RESOURCE_RAGE ) ),
      shield_slam_reset( p->talents.protection.strategist->effectN( 1 ).percent() ),
      rend( nullptr ),
      lightning_strike( nullptr ),
      seismic_action( nullptr ),
      rend_target_cap( 0 ),
      rend_targets_hit( 0 )
  {
    parse_options( options_str );
    aoe       = -1;
    may_dodge = may_parry = may_block = false;

    // Shared cooldown with thunder clap
    cooldown = p->cooldown.thunder_clap;

    energize_type = action_energize::NONE;

    if ( p->spec.protection_warrior->ok() )
    {
      rage_gain += p->spec.protection_warrior->effectN( 20 ).resource( RESOURCE_RAGE );
      rage_gain += p->talents.mountain_thane.thunder_blast->effectN( 3 ).resource( RESOURCE_RAGE );
    }

    if ( p->talents.mountain_thane.crashing_thunder->ok() && p->specialization() == WARRIOR_FURY )
      rage_gain += p->talents.mountain_thane.crashing_thunder->effectN( 4 ).resource( RESOURCE_RAGE );

    if ( p->talents.shared.rend.ok() )
    {
      rend_target_cap = p->talents.warrior.thunder_clap->effectN( 5 ).base_value();
      if ( p->talents.arms.rend->ok() )
        rend = new rend_dot_t( p );
      if ( p->talents.protection.rend->ok() )
      {
        // Arma: 2022 Nov 4th.  Even if you are prot, the arms rend dot is being applied.
        if ( p->bugs )
          rend = new rend_dot_t( p );
        else
          rend = new rend_dot_prot_t( p );
      }
    }

    if ( p->talents.mountain_thane.lightning_strikes->ok() )
    {
      lightning_strike = get_action<lightning_strike_t>( "lightning_strike_thunder_blast", p );
      add_child( lightning_strike );
    }

    if ( p->talents.mountain_thane.crashing_thunder->ok() && p->talents.warrior.seismic_reverberation->ok() )
    {
      seismic_action = new thunder_blast_seismic_reverberation_t( "thunder_blast_seismic_reverberation", p );
      add_child( seismic_action );
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->buff.show_of_force->check() )
    {
      am *= 1.0 + ( p()->buff.show_of_force->stack_value() );
    }

    if ( p()->buff.violent_outburst->check() )
    {
      am *= 1.0 + p()->buff.violent_outburst->data().effectN( 1 ).percent();
    }

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = warrior_attack_t::bonus_da( s );

    return da;
  }

  void execute() override
  {
    rend_targets_hit = 0;

    warrior_attack_t::execute();

    if ( p()->buff.show_of_force->up() )
    {
      p()->buff.show_of_force->expire();
    }

    if ( rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
      if ( p()->sets->has_set_bonus( WARRIOR_PROTECTION, TWW1, B2 ) )
        p()->buff.expert_strategist->trigger();
    }

    if ( p()->talents.protection.thunderlord.ok() )
    {
      p()->cooldown.demoralizing_shout->adjust(
          -p()->talents.protection.thunderlord->effectN( 1 ).time_value() *
          std::min( execute_state->n_targets,
                    as<unsigned int>( p()->talents.protection.thunderlord->effectN( 2 ).base_value() ) ) );
    }

    auto total_rage_gain = rage_gain;

    if ( p()->buff.violent_outburst->check() )
    {
      p()->buff.ignore_pain->trigger();
      p()->buff.violent_outburst->expire();
      total_rage_gain *= 1.0 + p()->buff.violent_outburst->data().effectN( 4 ).percent();
    }

    p()->resource_gain( RESOURCE_RAGE, total_rage_gain, p()->gain.thunder_blast );

    if ( p()->talents.mountain_thane.crashing_thunder->ok() )
    {
      if ( p()->talents.fury.improved_whirlwind->ok() )
      {
        p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
      }
    }

    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      auto chance = p()->talents.mountain_thane.lightning_strikes->effectN( 1 ).percent();
      if ( p()->buff.avatar->check() )
        chance *= 1.0 + p()->talents.mountain_thane.lightning_strikes->effectN( 2 ).percent();
      if ( p()->talents.mountain_thane.gathering_clouds->ok() )
        chance *= 1.0 + p()->talents.mountain_thane.gathering_clouds->effectN( 1 ).percent();
      if ( rng().roll( chance ) )
      {
        lightning_strike->execute();
      }
    }

    if ( p()->talents.mountain_thane.flashing_skies->ok() )
    {
      lightning_strike->execute();
    }

    p()->buff.thunder_blast->decrement();
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p()->talents.shared.rend.ok() )
    {
      if ( rend_targets_hit < rend_target_cap )
      {
        rend->execute_on_target( state->target );
        rend_targets_hit++;
      }
    }

    if ( p()->talents.mountain_thane.crashing_thunder->ok() &&
          p()->talents.warrior.seismic_reverberation->ok() &&
          state->n_targets >= p()->talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      seismic_action->base_dd_min = seismic_action->base_dd_max = state->result_amount * ( 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 4 ).percent() );
      seismic_action->execute_on_target( target );
    }
  }

  bool ready() override
  {
    if ( ! p()->buff.thunder_blast->check() && !background )
      return false;
    return warrior_attack_t::ready();
  }
};

struct thunder_clap_seismic_reverberation_t : public warrior_attack_t
{
  thunder_clap_seismic_reverberation_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 436792 ) )
  {
    background = true;
    may_dodge = may_parry = may_block = false;
    may_crit = false;
    tick_may_crit = false;

    energize_type = action_energize::NONE;
  }
};

struct thunder_clap_t : public warrior_attack_t
{
  double rage_gain;
  double shield_slam_reset;
  warrior_attack_t* rend;
  action_t* lightning_strike;
  action_t* seismic_action;
  double rend_target_cap;
  double rend_targets_hit;
  thunder_clap_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "thunder_clap", p, p->talents.warrior.thunder_clap ),
      rage_gain( data().effectN( 4 ).resource( RESOURCE_RAGE ) ),
      shield_slam_reset( p->talents.protection.strategist->effectN( 1 ).percent() ),
      rend( nullptr ),
      lightning_strike( nullptr ),
      seismic_action( nullptr ),
      rend_target_cap( 0 ),
      rend_targets_hit( 0 )
  {
    parse_options( options_str );
    aoe       = -1;
    may_dodge = may_parry = may_block = false;

    energize_type = action_energize::NONE;

    if ( p->spec.protection_warrior->ok() )
      rage_gain += p->spec.protection_warrior->effectN( 20 ).resource( RESOURCE_RAGE );

    if ( p->talents.mountain_thane.crashing_thunder->ok() && p->specialization() == WARRIOR_FURY )
      rage_gain += p->talents.mountain_thane.crashing_thunder->effectN( 4 ).resource( RESOURCE_RAGE );

    if ( p->talents.shared.rend.ok() )
    {
      rend_target_cap = p->talents.warrior.thunder_clap->effectN( 5 ).base_value();
      if ( p->talents.arms.rend->ok() )
        rend = new rend_dot_t( p );
      if ( p->talents.protection.rend->ok() )
      {
        // Arma: 2022 Nov 4th.  Even if you are prot, the arms rend dot is being applied.
        if ( p->bugs )
          rend = new rend_dot_t( p );
        else
          rend = new rend_dot_prot_t( p );
      }
    }

    if ( p->talents.mountain_thane.lightning_strikes->ok() )
    {
      lightning_strike = get_action<lightning_strike_t>( "lightning_strike_thunder_clap", p );
      add_child( lightning_strike );
    }

    if ( p->talents.mountain_thane.crashing_thunder->ok() && p->talents.warrior.seismic_reverberation->ok() )
    {
      seismic_action = new thunder_clap_seismic_reverberation_t( "thunder_clap_seismic_reverberation", p );
      add_child( seismic_action );
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->buff.show_of_force->check() )
    {
      am *= 1.0 + ( p()->buff.show_of_force->stack_value() );
    }

    if ( p()->buff.violent_outburst->check() )
    {
      am *= 1.0 + p()->buff.violent_outburst->data().effectN( 1 ).percent();
    }

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = warrior_attack_t::bonus_da( s );

    return da;
  }

  void execute() override
  {
    rend_targets_hit = 0;

    warrior_attack_t::execute();

    if ( p()->buff.show_of_force->up() )
    {
      p()->buff.show_of_force->expire();
    }

    if ( rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
      if ( p()->sets->has_set_bonus( WARRIOR_PROTECTION, TWW1, B2 ) )
        p()->buff.expert_strategist->trigger();
    }

    if ( p()->talents.protection.thunderlord.ok() )
    {
      p()->cooldown.demoralizing_shout->adjust(
          -p()->talents.protection.thunderlord->effectN( 1 ).time_value() *
          std::min( execute_state->n_targets,
                    as<unsigned int>( p()->talents.protection.thunderlord->effectN( 2 ).base_value() ) ) );
    }

    auto total_rage_gain = rage_gain;

    if ( p()->buff.violent_outburst->check() )
    {
      p()->buff.ignore_pain->trigger();
      p()->buff.violent_outburst->expire();
      total_rage_gain *= 1.0 + p()->buff.violent_outburst->data().effectN( 4 ).percent();
    }

    p()->resource_gain( RESOURCE_RAGE, total_rage_gain, p()->gain.thunder_clap );

    if ( p()->talents.mountain_thane.crashing_thunder->ok() )
    {
      if ( p()->talents.fury.improved_whirlwind->ok() )
      {
        p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
      }
    }

    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      auto chance = p()->talents.mountain_thane.lightning_strikes->effectN( 1 ).percent();
      if ( p()->buff.avatar->check() )
        chance *= 1.0 + p()->talents.mountain_thane.lightning_strikes->effectN( 2 ).percent();
      if ( p()->talents.mountain_thane.gathering_clouds->ok() )
        chance *= 1.0 + p()->talents.mountain_thane.gathering_clouds->effectN( 1 ).percent();
      if ( rng().roll( chance ) )
      {
        lightning_strike->execute();
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p()->talents.shared.rend.ok() )
    {
      if ( rend_targets_hit < rend_target_cap )
      {
        rend->execute_on_target( state->target );
        rend_targets_hit++;
      }
    }

    if ( p()->talents.mountain_thane.crashing_thunder->ok() &&
          p()->talents.warrior.seismic_reverberation->ok() &&
          state->n_targets >= p()->talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      seismic_action->base_dd_min = seismic_action->base_dd_max = state->result_amount * ( 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 4 ).percent() );
      seismic_action->execute_on_target( target );
    }
  }

  bool ready() override
  {
    if ( p()->buff.thunder_blast->check() && !background )
      return false;
    return warrior_attack_t::ready();
  }
};

// Arms Execute ==================================================================

struct finishing_wound_t : public residual_action::residual_periodic_action_t<warrior_attack_t>
{
  finishing_wound_t( util::string_view name, warrior_t* p ) :
    base_t( name, p, p->find_spell( 426284 ) )
  {
    dual = true;
  }
};

struct execute_damage_t : public warrior_attack_t
{
  double max_rage;
  double cost_rage;
  finishing_wound_t* finishing_wound;
  execute_damage_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "execute_damage", p, p->spell.execute->effectN( 1 ).trigger() ), max_rage( 40 ),
    finishing_wound( nullptr )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
    background = true;
    finishing_wound = new finishing_wound_t( "finishing_wound", p);
    add_child( finishing_wound );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( cost_rage == 0 )  // If it was free, it's a full damage execute.
      am *= 2.0;
    else
      am *= 2.0 * ( std::min( max_rage, cost_rage ) / max_rage );
    return am;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p()->talents.arms.executioners_precision->ok() && ( result_is_hit( state->result ) ) )
    {
      td( state->target )->debuffs_executioners_precision->trigger();
    }

    if ( td( state->target )->debuffs_marked_for_execution->up() )
    {
      if ( p()->talents.slayer.unrelenting_onslaught->ok() )
      {
        p()->cooldown.bladestorm->adjust( - ( timespan_t::from_seconds( p()->talents.slayer.unrelenting_onslaught->effectN( 1 ).base_value() * td( state->target )->debuffs_marked_for_execution->stack() ) ) );
        td( state->target )->debuffs_overwhelmed->trigger( as<int>( p()->talents.slayer.unrelenting_onslaught->effectN( 2 ).base_value() ) * td( state->target )->debuffs_marked_for_execution->stack() );
      }
      td( state->target )->debuffs_marked_for_execution->expire();
    }
  }
};

struct execute_arms_t : public warrior_attack_t
{
  execute_damage_t* trigger_attack;
  action_t* lightning_strike;
  double max_rage;
  double execute_pct;
  double shield_slam_reset;
  execute_arms_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "execute", p, p->spell.execute ),
    trigger_attack( nullptr ),
    lightning_strike( nullptr ),
    max_rage( 40 ),
    execute_pct( 20 ),
    shield_slam_reset( p -> talents.protection.strategist -> effectN( 1 ).percent() )
  {
    parse_options( options_str );
    weapon        = &( p->main_hand_weapon );

    trigger_attack = new execute_damage_t( p, options_str );
    add_child( trigger_attack );

    if ( p->talents.arms.massacre->ok() )
    {
      execute_pct = p->talents.arms.massacre->effectN( 2 )._base_value;
    }
    if ( p->talents.protection.massacre->ok() )
    {
      execute_pct = p->talents.protection.massacre->effectN( 2 ).base_value();
    }

    if ( p->talents.mountain_thane.lightning_strikes->ok() )
    {
      lightning_strike = get_action<lightning_strike_t>( "lightning_strike_execute", p );
      add_child( lightning_strike );
    }
  }

  double tactician_cost() const override
  {
    double c;

    if ( p()->buff.sudden_death->check() )
    {
      c = 0;
    }
    else
    {
      c = std::min( max_rage, p()->resources.current[ RESOURCE_RAGE ] );
      c = ( c / max_rage ) * 40;
    }
    if ( sim->log )
    {
      sim->out_debug.printf( "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f",
                             name(), c, cost() );
    }
    return c;
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();
    c        = std::min( max_rage, std::max( p()->resources.current[ RESOURCE_RAGE ], c ) );

    if ( p()->buff.sudden_death->check() )
    {
      return 0;  // The cost reduction isn't in the spell data
    }
    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    trigger_attack->cost_rage = last_resource_cost;
    trigger_attack->execute();
    if ( p()->talents.arms.improved_execute->ok() && !p()->talents.arms.critical_thinking->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, last_resource_cost * p()->spell.execute_rage_refund->effectN( 2 ).percent(),
                          p()->gain.execute_refund );  // Not worth the trouble to check if the target died.
    }
    if ( p()->talents.arms.improved_execute->ok() && p()->talents.arms.critical_thinking->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, last_resource_cost * ( p()->talents.arms.critical_thinking->effectN( 2 ).percent() +
                                                 p()->spell.execute_rage_refund->effectN( 2 ).percent() ),
                          p()->gain.execute_refund );  // Not worth the trouble to check if the target died.
    }

    if ( p()->buff.sudden_death->up() )
    {
      p()->buff.sudden_death->decrement();
      if ( p()->talents.slayer.imminent_demise->ok() )
      {
        p()->buff.imminent_demise->trigger();
      }
    }
    if ( p()->talents.arms.juggernaut.ok() )
    {
      p()->buff.juggernaut->trigger();
    }
    if ( p()->talents.protection.juggernaut.ok() )
    {
      p()->buff.juggernaut_prot->trigger();
    }

    if ( rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
      if ( p()->sets->has_set_bonus( WARRIOR_PROTECTION, TWW1, B2 ) )
        p()->buff.expert_strategist->trigger();
    }

    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      auto chance = p()->talents.mountain_thane.lightning_strikes->effectN( 1 ).percent();
      if ( p()->buff.avatar->check() )
        chance *= 1.0 + p()->talents.mountain_thane.lightning_strikes->effectN( 2 ).percent();
      if ( p()->talents.mountain_thane.gathering_clouds->ok() )
        chance *= 1.0 + p()->talents.mountain_thane.gathering_clouds->effectN( 1 ).percent();
      if ( rng().roll( chance ) )
      {
        lightning_strike->execute();
      }
    }
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if( state->target->health_percentage() < 30 && td( state->target )->debuffs_fatal_mark->check() )
    {
      p()->active.fatality->set_target( state->target );
      p()->active.fatality->execute();
    }

    if ( p()->tier_set.t29_arms_4pc->ok() && state->result == RESULT_CRIT )
    {
      p()->buff.strike_vulnerabilities->trigger();
    }

    // 25% proc chance found via testing
    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) )
    {
      p() -> buff.fervid -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.25 );
    }

    if ( p()->talents.colossus.colossal_might->ok() )
    {
      if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
      {
        p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() ) );
      }
      p()->buff.colossal_might->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Sudden Death allow execution on any target
    bool always = p()->buff.sudden_death->check();

    if ( ! always && candidate_target->health_percentage() > execute_pct )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
  }

};

// Fatality ===============================================================================
struct fatality_t : public warrior_attack_t
{
  fatality_t( warrior_t* p ) : warrior_attack_t( "fatality", p, p->find_spell( 383706 ) )
  {
    background = true;
    may_crit   = false;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warrior_attack_t::composite_target_multiplier( target );
    m *= td( target )->debuffs_fatal_mark->stack();
    return m;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );
    td( state->target )->debuffs_fatal_mark->expire();
  }
};

// Fury Execute ======================================================================
struct execute_main_hand_t : public warrior_attack_t
{
  int aoe_targets;
  execute_main_hand_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    background = true;
    dual   = true;
    weapon = &( p->main_hand_weapon );
    radius = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );
    if ( p()->talents.slayer.unrelenting_onslaught->ok() && td( state->target )->debuffs_marked_for_execution->up() )
    {
      p()->cooldown.bladestorm->adjust( - ( timespan_t::from_seconds( p()->talents.slayer.unrelenting_onslaught->effectN( 1 ).base_value() * td( state->target )->debuffs_marked_for_execution->stack() ) ) );
      td( state->target )->debuffs_overwhelmed->trigger( as<int>( p()->talents.slayer.unrelenting_onslaught->effectN( 2 ).base_value() ) * td( state->target )->debuffs_marked_for_execution->stack() );
    }
  }
};

struct execute_off_hand_t : public warrior_attack_t
{
  int aoe_targets;
  execute_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    background = true;
    dual     = true;
    may_miss = may_dodge = may_parry = may_block = false;
    weapon                                       = &( p->off_hand_weapon );
    radius = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );
    // We only remove the debuff in the off hand, if the target dies from the MH attack, we don't need to worry about expiring it.
    if ( td( state->target )->debuffs_marked_for_execution->up() )
      td( state->target )->debuffs_marked_for_execution->expire();
  }
};

struct execute_fury_t : public warrior_attack_t
{
  execute_main_hand_t* mh_attack;
  execute_off_hand_t* oh_attack;
  action_t* lightning_strike;
  bool improved_execute;
  double execute_pct;
  //double cost_rage;
  double max_rage;
  double rage_from_improved_execute;
  execute_fury_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "execute", p, p->spec.execute ),
      mh_attack( nullptr ),
      oh_attack( nullptr ),
      lightning_strike( nullptr ),
      improved_execute( false ),
      execute_pct( 20 ),
      max_rage( 40 ),
      rage_from_improved_execute(
      ( p->talents.fury.improved_execute->effectN( 3 ).base_value() ) / 10.0 )
  {
    parse_options( options_str );
    mh_attack = new execute_main_hand_t( p, "execute_mainhand", p->spec.execute->effectN( 1 ).trigger() );
    oh_attack = new execute_off_hand_t( p, "execute_offhand", p->spec.execute->effectN( 2 ).trigger() );
    add_child( mh_attack );
    add_child( oh_attack );

    if ( p->talents.fury.massacre->ok() )
    {
      execute_pct = p->talents.fury.massacre->effectN( 2 ).base_value();
      if ( cooldown->action == this )
      {
        cooldown -> duration -= timespan_t::from_millis( p -> talents.fury.massacre -> effectN( 3 ).base_value() );
      }
    }

    if ( p->talents.mountain_thane.lightning_strikes->ok() )
    {
      lightning_strike = get_action<lightning_strike_t>( "lightning_strike_execute", p );
      add_child( lightning_strike );
    }
  }

  double cost_pct_multiplier() const override
  {
    auto c = warrior_attack_t::cost_pct_multiplier();

    if ( p()->talents.fury.improved_execute->ok() )
    {
      c *= 1.0 + p()->talents.fury.improved_execute->effectN( 1 ).percent();
    }

    return c;
  }

//  double cost() const override
//  {
//    double c = warrior_attack_t::cost();

//    c = std::min( max_rage, std::max( p()->resources.current[ RESOURCE_RAGE ], c ) );

//    return c;
//  }

  void execute() override
  {
    warrior_attack_t::execute();

    mh_attack->execute();

    if ( p()->specialization() == WARRIOR_FURY && result_is_hit( execute_state->result ) &&
         p()->off_hand_weapon.type != WEAPON_NONE )
      // If MH fails to land, or if there is no OH weapon for Fury, oh attack does not execute.
      oh_attack->execute();

    p()->buff.meat_cleaver->decrement();
    if ( p() -> buff.sudden_death -> up() )
    {
      p()->buff.sudden_death->decrement();
      if ( p()->talents.slayer.imminent_demise->ok() )
      {
        p()->buff.imminent_demise->trigger();
      }
    }

    if ( p()->talents.fury.improved_execute->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_improved_execute, p()->gain.execute );
    }

    if ( p()->talents.fury.ashen_juggernaut->ok() )
    {
      p()->buff.ashen_juggernaut->trigger();
    }

    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      auto chance = p()->talents.mountain_thane.lightning_strikes->effectN( 1 ).percent();
      if ( p()->buff.avatar->check() )
        chance *= 1.0 + p()->talents.mountain_thane.lightning_strikes->effectN( 2 ).percent();
      if ( p()->talents.mountain_thane.gathering_clouds->ok() )
        chance *= 1.0 + p()->talents.mountain_thane.gathering_clouds->effectN( 1 ).percent();
      if ( rng().roll( chance ) )
      {
        lightning_strike->execute();
      }
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Sudden Death allow execution on any target
    bool always = p()->buff.sudden_death->check();

    if ( ! always && candidate_target->health_percentage() > execute_pct )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
  }
};

// Hamstring ==============================================================

struct hamstring_t : public warrior_attack_t
{
  int aoe_targets;
  hamstring_t( warrior_t* p, util::string_view options_str ) : warrior_attack_t( "hamstring", p, p->spell.hamstring ),
  aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );

    radius = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.meat_cleaver->decrement();
  }
};

// Piercing Howl ==============================================================

struct piercing_howl_t : public warrior_attack_t
{
  piercing_howl_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "piercing_howl", p, p->talents.warrior.piercing_howl )
  {
    parse_options( options_str );
  }
};

// Heroic Throw ==============================================================

struct heroic_throw_t : public warrior_attack_t
{
  heroic_throw_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "heroic_throw", p, p->find_class_spell( "Heroic Throw" ) )
  {
    parse_options( options_str );

    weapon    = &( player->main_hand_weapon );
    may_dodge = may_parry = may_block = false;
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move > range ||
         p()->current.distance_to_move < data().min_range() )  // Cannot heroic throw unless target is in range.
    {
      return false;
    }
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Heroic Leap ==============================================================

struct heroic_leap_t : public warrior_attack_t
{
  const spell_data_t* heroic_leap_damage;
  heroic_leap_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "heroic_leap", p, p->talents.warrior.heroic_leap ),
      heroic_leap_damage( p->find_spell( 52174 ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    aoe                   = -1;
    may_dodge = may_parry = may_miss = may_block = false;
    movement_directionality                      = movement_direction_type::OMNI;
    base_teleport_distance                       = data().max_range();
    range                                        = -1;
    attack_power_mod.direct                      = heroic_leap_damage->effectN( 1 ).ap_coeff();
    radius                                       = heroic_leap_damage->effectN( 1 ).radius();

    cooldown->duration = data().charge_cooldown();  // Fixes bug in spelldata for now.
    cooldown->duration += p->talents.warrior.bounding_stride->effectN( 1 ).time_value();
  }

  // TOCHECK: Shouldn't this scale with distance to travel?
  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( 0.5 );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->current.distance_to_move > 0 && !p()->buff.heroic_leap_movement->check() )
    {
      double speed = std::min( p()->current.distance_to_move, base_teleport_distance ) /
                     ( p()->base_movement_speed * ( 1 + p()->stacking_movement_modifier() ) ) /
                     travel_time().total_seconds();
      p()->buff.heroic_leap_movement->trigger( 1, speed, 1, travel_time() );
    }
  }

  bool ready() override
  {
    if ( p()->buff.intervene_movement->check() || p()->buff.charge_movement->check() ||
         p()->buff.intercept_movement->check() || p()->buff.shield_charge_movement->check() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Impending Victory Heal ========================================================

struct impending_victory_heal_t : public warrior_heal_t
{
  impending_victory_heal_t( warrior_t* p ) : warrior_heal_t( "impending_victory_heal", p, p->find_spell( 202166 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background    = true;
  }

  proc_types proc_type() const override
  {
    return PROC1_NONE_HEAL;
  }

  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

// Impending Victory ==============================================================

struct impending_victory_t : public warrior_attack_t
{
  impending_victory_heal_t* impending_victory_heal;
  int aoe_targets;
  impending_victory_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "impending_victory", p, p->talents.warrior.impending_victory ), impending_victory_heal( nullptr ),
    aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    if ( p->non_dps_mechanics )
    {
      impending_victory_heal = new impending_victory_heal_t( p );
    }
    radius = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( impending_victory_heal )
    {
      impending_victory_heal->execute();
    }

    // 25% proc chance found via testing
    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) )
    {
      p() -> buff.fervid -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.25 );
    }

    p() -> buff.meat_cleaver -> decrement();
  }
};

// Intervene ===============================================================
// Note: Conveniently ignores that you can only intervene a friendly target.
// For the time being, we're just going to assume that there is a friendly near the target
// that we can intervene to. Maybe in the future with a more complete movement system, we will
// fix this to work in a raid simulation that includes multiple melee.

struct intervene_t : public warrior_attack_t
{
  double movement_speed_increase;
  intervene_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "intervene", p, p->talents.warrior.intervene ), movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    movement_directionality = movement_direction_type::OMNI;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->current.distance_to_move > 0 )
    {
      p()->buff.intervene_movement->trigger(
          1, movement_speed_increase, 1,
          timespan_t::from_seconds(
              p()->current.distance_to_move /
              ( p()->base_movement_speed * ( 1 + p()->stacking_movement_modifier() + movement_speed_increase ) ) ) );
      p()->current.moving_away = 0;
    }
  }

  bool ready() override
  {
    if ( p()->buff.heroic_leap_movement->check() || p()->buff.charge_movement->check() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Heroic Charge ============================================================

struct heroic_charge_t : public warrior_attack_t
{
  //TODO: do actual movement for distance targeting sims
  warrior_attack_t* heroic_leap;
  charge_t* charge;
  int heroic_leap_distance;

  heroic_charge_t( warrior_t* p, util::string_view options_str ) :
    warrior_attack_t( "heroic_charge", p, spell_data_t::nil() ),
    heroic_leap( new heroic_leap_t( p, "" ) ),
    charge( new charge_t( p, "" ) ),
    heroic_leap_distance( 10 )
  {
    // The user can set the distance that they want to travel with heroic leap before charging if they want
    add_option( opt_int( "distance", heroic_leap_distance ) );
    parse_options( options_str );

    if ( heroic_leap_distance > heroic_leap -> data().max_range() ||
         heroic_leap_distance < heroic_leap -> data().min_range() )
    {
      sim -> error( "{} has an invalid heroic leap distance of {}, defaulting to 10", p -> name(), heroic_leap_distance );
      heroic_leap_distance = 10;
    }
    callbacks = false;
  }

  timespan_t execute_time() const override
  {
    // Execute time is equal to the sum of heroic leap's travel time (a fixed 0.5s atm)
    // and charge's time to travel based on heroic leap's distance travelled
    return heroic_leap -> travel_time() + charge -> calc_charge_time( heroic_leap_distance );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    // At the end of the execution, start cooldowns, adjusted for execute time
    heroic_leap -> cooldown -> start( heroic_leap -> cooldown_duration() - execute_time() );
    charge -> cooldown -> start( charge -> cooldown_duration() - charge -> calc_charge_time( heroic_leap_distance ) );

    // Execute charge damage and whatever else it's supposed to proc
    charge -> impact_action -> execute();
  }

  bool ready() override
  {
    // TODO: Enable heroic leaping mid-movement while making sure it doesn't break anything
    if ( ! heroic_leap -> cooldown -> is_ready() || ! charge -> cooldown -> is_ready() ||
         p()->buffs.movement->check() || p() -> buff.heroic_leap_movement -> check() || p() -> buff.charge_movement -> check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Pummel ===================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "pummel", p, p->find_class_spell( "Pummel" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
    is_interrupt = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.warrior.concussive_blows.ok() && result_is_hit( execute_state->result ) )
    {
      td( execute_state->target )->debuffs_concussive_blows->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
      return false;

    return warrior_attack_t::target_ready( candidate_target );
  }
};

// Raging Blow ==============================================================

struct raging_blow_attack_t : public warrior_attack_t
{
  int aoe_targets;
  raging_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
    aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    dual                                         = true;
    background = true;

    //base_multiplier *= 1.0 + p->talents.cruelty->effectN( 1 ).percent();
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.fury.cruelty->ok() && p()->buff.enrage->check() )
    {
      am *= 1.0 + p()->talents.fury.cruelty->effectN( 1 ).percent();
    }

    return am;
  }
};

struct raging_blow_t : public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;
  action_t* lightning_strike;
  double cd_reset_chance;
  double wrath_and_fury_reset_chance;
  bool opportunist_up;
  raging_blow_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "raging_blow", p, p->talents.fury.raging_blow ),
      mh_attack( nullptr ),
      oh_attack( nullptr ),
      lightning_strike( nullptr ),
      cd_reset_chance( p->talents.fury.raging_blow->effectN( 1 ).percent() ),
      wrath_and_fury_reset_chance( p->talents.fury.wrath_and_fury->effectN( 1 ).percent() ),
      opportunist_up( false )
  {
    parse_options( options_str );

    oh_attack         = new raging_blow_attack_t( p, "raging_blow_oh", p->talents.fury.raging_blow->effectN( 4 ).trigger() );
    oh_attack->weapon = &( p->off_hand_weapon );
    add_child( oh_attack );
    mh_attack         = new raging_blow_attack_t( p, "raging_blow_mh", p->talents.fury.raging_blow->effectN( 3 ).trigger() );
    mh_attack->weapon = &( p->main_hand_weapon );
    add_child( mh_attack );
    cooldown->reset( false );
    track_cd_waste = true;
    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }
    if ( p->talents.mountain_thane.lightning_strikes->ok() )
    {
      lightning_strike = get_action<lightning_strike_t>( "lightning_strike_raging_blow", p );
      add_child( lightning_strike );
    }
  }

  void init() override
  {
    warrior_attack_t::init();
    cooldown->hasted = true;
  }

  void execute() override
  {
    opportunist_up = p()->buff.opportunist->check();

    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      mh_attack->execute();
      oh_attack->execute();
    }

    p()->buff.opportunist->decrement();

    if ( p()->talents.fury.improved_raging_blow->ok() && p()->talents.fury.wrath_and_fury->ok() &&
         p()->buff.enrage->check() )
    {
      if ( rng().roll( cd_reset_chance + wrath_and_fury_reset_chance ) )
      {
        cooldown->reset( true );
        // If opportunist was up when we used the skill, in game it refreshes, then fades.  For simc purposes, do not proc if we
        // cast with opportunist up.
        if ( p()->talents.slayer.opportunist->ok() && !opportunist_up )
          p()->buff.opportunist->trigger();

        if ( p()->sets->has_set_bonus( WARRIOR_FURY, TWW1, B4 ) )
          p()->buff.deep_thirst->trigger();
      }
    }
    else if ( p()->talents.fury.improved_raging_blow->ok() )
    {
      if ( rng().roll( cd_reset_chance ) )
      {
        cooldown->reset( true );
        // If opportunist was up when we used the skill, in game it refreshes, then fades.  For simc purposes, do not proc if we
        // cast with opportunist up.
        if ( p()->talents.slayer.opportunist->ok() && !opportunist_up )
          p()->buff.opportunist->trigger();

        if ( p()->sets->has_set_bonus( WARRIOR_FURY, TWW1, B4 ) )
          p()->buff.deep_thirst->trigger();
      }
    }
    p()->buff.meat_cleaver->decrement();

    if ( p()->talents.fury.slaughtering_strikes->ok() )
    {
      p()->buff.slaughtering_strikes->trigger();
    }

    if ( p()->talents.fury.bloodcraze->ok() )
    {
      p()->buff.bloodcraze->trigger();
    }

    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      auto chance = p()->talents.mountain_thane.lightning_strikes->effectN( 1 ).percent();
      if ( p()->buff.avatar->check() )
        chance *= 1.0 + p()->talents.mountain_thane.lightning_strikes->effectN( 2 ).percent();
      if ( p()->talents.mountain_thane.gathering_clouds->ok() )
        chance *= 1.0 + p()->talents.mountain_thane.gathering_clouds->effectN( 1 ).percent();
      if ( rng().roll( chance ) )
      {
        lightning_strike->execute();
      }
    }
  }

  bool ready() override
  {
    // Needs weapons in both hands
    if ( p()->main_hand_weapon.type == WEAPON_NONE || p()->off_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    if ( p()->buff.crushing_blow->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Crushing Blow ==============================================================

struct crushing_blow_attack_t : public warrior_attack_t
{
  int aoe_targets;
  crushing_blow_attack_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
    aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    dual                                         = true;
    background = true;

    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.fury.cruelty->ok() && p()->buff.enrage->check() )
    {
      am *= 1.0 + p()->talents.fury.cruelty->effectN( 1 ).percent();
    }

    return am;
  }
};

struct crushing_blow_t : public warrior_attack_t
{
  crushing_blow_attack_t* mh_attack;
  crushing_blow_attack_t* oh_attack;
  action_t* lightning_strike;
  double cd_reset_chance, wrath_and_fury_reset_chance;
  bool opportunist_up;
  crushing_blow_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "crushing_blow", p, p->spec.crushing_blow ),
      mh_attack( nullptr ),
      oh_attack( nullptr ),
      lightning_strike( nullptr ),
      cd_reset_chance( p->spec.crushing_blow->effectN( 1 ).percent() ),
      wrath_and_fury_reset_chance( p->talents.fury.wrath_and_fury->effectN( 1 ).percent() ),
      opportunist_up( false )
  {
    parse_options( options_str );

    oh_attack         = new crushing_blow_attack_t( p, "crushing_blow_oh", p->spec.crushing_blow->effectN( 4 ).trigger() );
    oh_attack->weapon = &( p->off_hand_weapon );
    add_child( oh_attack );
    mh_attack         = new crushing_blow_attack_t( p, "crushing_blow_mh", p->spec.crushing_blow->effectN( 3 ).trigger() );
    mh_attack->weapon = &( p->main_hand_weapon );
    add_child( mh_attack );
    cooldown->reset( false );
    cooldown = p -> cooldown.raging_blow;
    track_cd_waste = true;

    if ( p->talents.fury.swift_strikes->ok() )
    {
      energize_amount += p->talents.fury.swift_strikes->effectN( 2 ).resource( RESOURCE_RAGE );
    }

    if ( p->talents.mountain_thane.lightning_strikes->ok() )
    {
      lightning_strike = get_action<lightning_strike_t>( "lightning_strike_crushing_blow", p );
      add_child( lightning_strike );
    }
  }

  void init() override
  {
    warrior_attack_t::init();
    cooldown->hasted = true;
  }

  void execute() override
  {
    opportunist_up = p()->buff.opportunist->check();

    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      mh_attack->execute();
      oh_attack->execute();
    }

    p()->buff.opportunist->decrement();

    if ( p()->talents.fury.improved_raging_blow->ok() && p()->talents.fury.wrath_and_fury->ok() &&
         p()->buff.enrage->check() )
    {
      if ( rng().roll( cd_reset_chance + wrath_and_fury_reset_chance ) )
      {
        cooldown->reset( true );
        // If opportunist was up when we used the skill, in game it refreshes, then fades.  For simc purposes, do not proc if we
        // cast with opportunist up.
        if ( p()->talents.slayer.opportunist->ok() && !opportunist_up )
          p()->buff.opportunist->trigger();

        if ( p()->sets->has_set_bonus( WARRIOR_FURY, TWW1, B4 ) )
          p()->buff.deep_thirst->trigger();
      }
    }
    else if ( p()->talents.fury.improved_raging_blow->ok() && rng().roll( cd_reset_chance ) )
    {
      cooldown->reset( true );
      // If opportunist was up when we used the skill, in game it refreshes, then fades.  For simc purposes, do not proc if we
      // cast with opportunist up.
      if ( p()->talents.slayer.opportunist->ok() && !opportunist_up )
        p()->buff.opportunist->trigger();

      if ( p()->sets->has_set_bonus( WARRIOR_FURY, TWW1, B4 ) )
          p()->buff.deep_thirst->trigger();
    }

    p()->buff.crushing_blow->decrement();
    p()->buff.meat_cleaver->decrement();

    if ( p()->talents.fury.slaughtering_strikes->ok() )
    {
      p()->buff.slaughtering_strikes->trigger();
    }

    if ( p()->talents.fury.bloodcraze->ok() )
    {
      p()->buff.bloodcraze->trigger();
    }

    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      auto chance = p()->talents.mountain_thane.lightning_strikes->effectN( 1 ).percent();
      if ( p()->buff.avatar->check() )
        chance *= 1.0 + p()->talents.mountain_thane.lightning_strikes->effectN( 2 ).percent();
      if ( p()->talents.mountain_thane.gathering_clouds->ok() )
        chance *= 1.0 + p()->talents.mountain_thane.gathering_clouds->effectN( 1 ).percent();
      if ( rng().roll( chance ) )
      {
        lightning_strike->execute();
      }
    }
  }

  bool ready() override
  {
    // Needs weapons in both hands
    if ( p()->main_hand_weapon.type == WEAPON_NONE || p()->off_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    if ( !p()->buff.crushing_blow->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Shattering Throw ========================================================

struct shattering_throw_t : public warrior_attack_t
{
  shattering_throw_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shattering_throw", p, p->talents.warrior.shattering_throw )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
    attack_power_mod.direct = 1.0;
  }
  //add absorb shield bonus (are those even in SimC?), add cast time?
};

// Skullsplitter ===========================================================

struct skullsplitter_t : public warrior_attack_t
{
  skullsplitter_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "skullsplitter", p, p->talents.arms.skullsplitter )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_skullsplitter->trigger();
    }
  }
};

// Sweeping Strikes ===================================================================

struct sweeping_strikes_t : public warrior_spell_t
{
  sweeping_strikes_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "sweeping_strikes", p, p->spec.sweeping_strikes )
  {
    parse_options( options_str );
    callbacks = false;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.sweeping_strikes->extend_duration_or_trigger();
  }
};

// Odyn's Fury ==========================================================

struct odyns_fury_off_hand_t : public warrior_attack_t
{
  odyns_fury_off_hand_t( warrior_t* p, util::string_view name, const spell_data_t* spell )
    : warrior_attack_t( name, p, spell )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = p->talents.fury.odyns_fury->effectN( 6 ).base_value();
  }
};

struct odyns_fury_main_hand_t : public warrior_attack_t
{
  int num_targets;
  odyns_fury_main_hand_t( warrior_t* p, util::string_view name, const spell_data_t* spell )
    : warrior_attack_t( name, p, spell ),
    num_targets( 0 )
  {
    background = true;
    aoe        = -1;
    reduced_aoe_targets = p->talents.fury.odyns_fury->effectN( 6 ).base_value();
  }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double ta = warrior_attack_t::composite_ta_multiplier( state );

    if ( num_targets > reduced_aoe_targets )
    {
      ta *= std::sqrt( reduced_aoe_targets / std::min<int>( sim->max_aoe_enemies, num_targets ) );
    }

    return ta;
  }

  void execute() override
  {
    // sqrt is snapshot on execute for the number of targets hit
    num_targets = as<int>( sim->target_non_sleeping_list.size() );

    warrior_attack_t::execute();
  }
};

struct odyns_fury_t : warrior_attack_t
{
  odyns_fury_main_hand_t* mh_attack;
  odyns_fury_main_hand_t* mh_attack2;
  odyns_fury_off_hand_t* oh_attack;
  odyns_fury_off_hand_t* oh_attack2;
  odyns_fury_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_attack_t( n, p, spell ),
      mh_attack( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 1 ).trigger() ) ),
      mh_attack2( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 3 ).trigger() ) ),
      oh_attack( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 2 ).trigger() ) ),
      oh_attack2( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 4 ).trigger() ) )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack->weapon = &( p->main_hand_weapon );
      add_child( mh_attack );
      mh_attack->radius = radius;
      mh_attack2->weapon = &( p->main_hand_weapon );
      mh_attack2->radius = radius;
      add_child( mh_attack2 );
      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_attack->weapon = &( p->off_hand_weapon );
        add_child( oh_attack );
        oh_attack->radius = radius;
        oh_attack2->weapon = &( p->off_hand_weapon );
        add_child( oh_attack2 );
        oh_attack2->radius = radius;
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.dancing_blades->ok() )
    {
      p()->buff.dancing_blades->trigger();
    } 

    if ( p()->talents.fury.titanic_rage->ok() )
    {
      p()->enrage();
      p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
    }

    mh_attack->execute();
    oh_attack->execute();
    mh_attack2->execute();
    oh_attack2->execute();

    if ( p()->talents.warrior.titans_torment->ok() )
    {
      action_t* torment_ability = p()->active.torment_avatar;
      torment_ability->schedule_execute();
    }

    if ( p()->tier_set.t31_fury_2pc->ok() )
    {
      // Triggers 3 stacks on cast (not in data), stacking up to 6 max
      p()->buff.furious_bloodthirst->trigger( 3 );
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Torment Odyn's Fury ==========================================================

struct torment_odyns_fury_t : warrior_attack_t
{
  odyns_fury_main_hand_t* mh_attack;
  odyns_fury_main_hand_t* mh_attack2;
  odyns_fury_off_hand_t* oh_attack;
  odyns_fury_off_hand_t* oh_attack2;
  torment_odyns_fury_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_attack_t( n, p, spell ),
      mh_attack( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 1 ).trigger() ) ),
      mh_attack2( new odyns_fury_main_hand_t( p, fmt::format( "{}_mh", n ), spell->effectN( 3 ).trigger() ) ),
      oh_attack( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 2 ).trigger() ) ),
      oh_attack2( new odyns_fury_off_hand_t( p, fmt::format( "{}_oh", n ), spell->effectN( 4 ).trigger() ) )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack->weapon = &( p->main_hand_weapon );
      add_child( mh_attack );
      mh_attack->radius  = radius;
      mh_attack2->weapon = &( p->main_hand_weapon );
      mh_attack2->radius = radius;
      add_child( mh_attack2 );
      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_attack->weapon = &( p->off_hand_weapon );
        add_child( oh_attack );
        oh_attack->radius  = radius;
        oh_attack2->weapon = &( p->off_hand_weapon );
        add_child( oh_attack2 );
        oh_attack2->radius = radius;
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.dancing_blades->ok() )
    {
      p()->buff.dancing_blades->trigger();
    }

    if ( p()->talents.fury.titanic_rage->ok() )
    {
      p()->enrage();
      p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
    }

    mh_attack->execute();
    oh_attack->execute();
    mh_attack2->execute();
    oh_attack2->execute();

    if ( p()->tier_set.t31_fury_2pc->ok() )
    {
      // Triggers 3 stacks on cast (not in data), stacking up to 6 max
      p()->buff.furious_bloodthirst->trigger( 3 );
    }
}

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Overpower ============================================================

struct dreadnaught_t : warrior_attack_t
{
  dreadnaught_t( warrior_t* p )
    : warrior_attack_t( "dreadnaught", p, p->find_spell( 315961 ) )
  {
    aoe = -1;
    reduced_aoe_targets = 5.0;
    background  = true;
  }
};

struct overpower_t : public warrior_attack_t
{
  double battlelord_chance;
  double rage_from_finishing_blows;
  double rage_from_battlelord;
  warrior_attack_t* dreadnaught;

  overpower_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "overpower", p, p->talents.arms.overpower ),
      battlelord_chance( p->talents.arms.battlelord->proc_chance() ),
      rage_from_finishing_blows( p->find_spell( 400806 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_battlelord( p->talents.arms.battlelord->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ) ),
      dreadnaught( nullptr )
  {
    parse_options( options_str );
    may_block = may_parry = may_dodge = false;
    weapon                            = &( p->main_hand_weapon );

    if ( p->talents.arms.dreadnaught->ok() )
    {
      dreadnaught = new dreadnaught_t( p );
      add_child( dreadnaught );
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( dreadnaught && result_is_hit( s->result ) )
    {
      dreadnaught->execute_on_target( s->target );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->talents.arms.battlelord->ok() && rng().roll( battlelord_chance ) )
    {
      if ( !p()->cooldown.mortal_strike->up() )
      {
        p()->proc.battlelord_wasted->occur();
      }
      else
      {
        p()->proc.battlelord->occur();
      }
      p()->cooldown.mortal_strike->reset( true );
      p()->resource_gain( RESOURCE_RAGE, rage_from_battlelord, p()->gain.battlelord );
    }

    if ( p()->talents.arms.martial_prowess->ok() )
    {
    p()->buff.martial_prowess->trigger();
    }

    if ( p()->talents.arms.finishing_blows->ok() && target->health_percentage() < 35 )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_finishing_blows, p()->gain.finishing_blows );
    }

    p()->buff.overpowering_might->expire();
    p()->buff.opportunist->decrement();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Warbreaker ==============================================================================

struct warbreaker_t : public warrior_attack_t
{
  warbreaker_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "warbreaker", p, p->talents.arms.warbreaker )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
    aoe    = -1;
    // warbreaker reduced target count is not in spelldata yet
    reduced_aoe_targets = 5;

    impact_action    = p->active.deep_wounds_ARMS;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( hit_any_target )
    {
      p()->buff.test_of_might_tracker->trigger();

      if ( p()->talents.arms.in_for_the_kill->ok() )
        p()->buff.in_for_the_kill->trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_colossus_smash->trigger();
    }
  }
};

// Rampage ================================================================

struct rampage_attack_t : public warrior_attack_t
{
  int aoe_targets;
  bool first_attack, valarjar_berserking, simmering_rage;
  double rage_from_valarjar_berserking;
  double hack_and_slash_chance;
  rampage_attack_t( warrior_t* p, const spell_data_t* rampage, util::string_view name )
    : warrior_attack_t( name, p, rampage ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      first_attack( false ),
      valarjar_berserking( false ),
      simmering_rage( false ),
      rage_from_valarjar_berserking( p->find_spell( 248179 )->effectN( 1 ).base_value() / 10.0 ),
      hack_and_slash_chance( p->talents.fury.hack_and_slash->proc_chance() )
  {
    background = true;
    dual = true;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
    if ( p->talents.fury.rampage->effectN( 2 ).trigger() == rampage )
      first_attack = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( first_attack && result_is_miss( execute_state->result ) )
      p()->first_rampage_attack_missed = true;
    else if ( first_attack )
      p()->first_rampage_attack_missed = false;

    if ( p()->talents.fury.hack_and_slash->ok() && rng().roll( hack_and_slash_chance ) )
    {
      p()->cooldown.raging_blow->reset( true );
      p()->cooldown.crushing_blow->reset( true );
    }

    // Expire buffs after the fourth attack triggers
    if ( p()->talents.fury.rampage->effectN( 5 ).trigger()->id() == data().id() )
    {
      p()->buff.meat_cleaver->decrement();
      p()->buff.slaughtering_strikes->expire();
      p()->buff.brutal_finish->expire();
      p()->buff.bloody_rampage->expire();
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( !p()->first_rampage_attack_missed )
    {  // If the first attack misses, all of the rest do as well. However, if any other attack misses, the attacks after
       // continue. The animations and timing of everything else still occur, so we can't just cancel rampage.
      warrior_attack_t::impact( s );
    }
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }
};

struct rampage_parent_t : public warrior_attack_t
{
  double cost_rage;
  double unbridled_chance;
  double frothing_berserker_chance;
  double rage_from_frothing_berserker;
  rampage_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "rampage", p, p->talents.fury.rampage ),
    unbridled_chance( p->talents.fury.unbridled_ferocity->effectN( 1 ).base_value() / 100.0 ),
    frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
    rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).percent())
  {
    parse_options( options_str );
    for ( auto* rampage_attack : p->rampage_attacks )
    {
      add_child( rampage_attack );
    }
    track_cd_waste = false;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    cost_rage = last_resource_cost;
    if ( p()->talents.warrior.frothing_berserker->ok() && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }
    if ( p()->talents.fury.frenzy->ok() )
    {
      p()->buff.frenzy->trigger();
    }
    if ( p()->talents.fury.unbridled_ferocity.ok() && rng().roll( unbridled_chance ) )
    {
      const timespan_t trigger_duration = p()->talents.fury.unbridled_ferocity->effectN( 2 ).time_value();
      p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
    }

    if ( p()->talents.fury.reckless_abandon->ok() )
    {
      p()->buff.bloodbath->trigger();
      p()->buff.crushing_blow->trigger();
    }

    if ( p()->tier_set.t30_fury_4pc->ok() )
    {
      p()->buff.merciless_assault->trigger();
    }

    p()->enrage();

    make_event<delayed_execute_event_t>( *sim, p(), p()->rampage_attacks[0], p()->target, timespan_t::from_millis(p()->talents.fury.rampage->effectN( 2 ).misc_value1()) );
    make_event<delayed_execute_event_t>( *sim, p(), p()->rampage_attacks[1], p()->target, timespan_t::from_millis(p()->talents.fury.rampage->effectN( 3 ).misc_value1()) );
    make_event<delayed_execute_event_t>( *sim, p(), p()->rampage_attacks[2], p()->target, timespan_t::from_millis(p()->talents.fury.rampage->effectN( 4 ).misc_value1()) );
    make_event<delayed_execute_event_t>( *sim, p(), p()->rampage_attacks[3], p()->target, timespan_t::from_millis(p()->talents.fury.rampage->effectN( 5 ).misc_value1()) );
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE || p()->off_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Ravager ==============================================================

struct ravager_tick_t : public warrior_attack_t
{
  double rage_from_storm_of_steel;
  double rage_from_ravager;
  ravager_tick_t( warrior_t* p, util::string_view name )
    : warrior_attack_t( name, p, p->find_spell( 156287 ) ),
    rage_from_storm_of_steel( 0.0 ),
    rage_from_ravager( 0.0 )
  {
    aoe = -1;
    reduced_aoe_targets = data().effectN( 2 ).base_value();
    dual = ground_aoe = true;
    rage_from_ravager = p->find_spell( 334934 )->effectN( 1 ).resource( RESOURCE_RAGE );
    rage_from_storm_of_steel += p->talents.fury.storm_of_steel->effectN( 5 ).resource( RESOURCE_RAGE );
    rage_from_storm_of_steel += p->talents.protection.storm_of_steel->effectN( 5 ).resource( RESOURCE_RAGE );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( execute_state->n_targets > 0 )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_ravager, p()->gain.ravager );
      p()->resource_gain( RESOURCE_RAGE, rage_from_storm_of_steel, p()->gain.storm_of_steel );
    }
  }
};

struct ravager_t : public warrior_attack_t
{
  ravager_tick_t* ravager;
  mortal_strike_t* mortal_strike;
  bloodthirst_t* bloodthirst;
  bloodbath_t* bloodbath;
  ravager_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "ravager", p, p->talents.shared.ravager ),
      ravager( new ravager_tick_t( p, "ravager_tick" ) ),
      mortal_strike( nullptr ),
      bloodthirst( nullptr ),
      bloodbath( nullptr )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    hasted_ticks            = true;
    internal_cooldown->duration = 0_s; // allow Anger Management to reduce the cd properly due to having both charges and cooldown entries
    attack_power_mod.direct = attack_power_mod.tick = 0;
    add_child( ravager );

    if ( p->talents.arms.unhinged->ok() )
    {
      mortal_strike = new mortal_strike_t( "mortal_strike_ravager_unhinged", p, true );
      add_child( mortal_strike );
    }

    if ( p->talents.fury.unhinged->ok() )
    {
      bloodthirst = new bloodthirst_t( "bloodthirst_ravager_unhinged", p, true );
      add_child( bloodthirst );

      if ( p->talents.fury.reckless_abandon->ok() )
      {
        bloodbath = new bloodbath_t( "bloodbath_ravager_unhinged", p, true );
        add_child( bloodbath );
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    // Make sure the buff is expired on fresh cast
    if ( p()->talents.shared.dance_of_death->ok() && p()->buff.dance_of_death_ravager->check() )
      p()->buff.dance_of_death_ravager->expire();

    if ( p()->talents.arms.merciless_bonegrinder->ok() )
    {
      // Set a 30s time for the buff, normally it would be either 12, or 15 seconds, but duration is hasted, expiry is tied to expiry of ravager
      p()->buff.merciless_bonegrinder->trigger(30_s);
    }
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );
    ravager->execute();

    // As of TWW Unhinged procs on the even ticks
    if ( ( mortal_strike || bloodthirst || bloodbath ) && ( d->current_tick % 2 == 0 ) )
    {
      // Select main target for unhinged, if no target, or target is dead, select a random target
      auto t = p() -> target;
      if ( ! p() -> target || p() -> target->is_sleeping() )
        t = select_random_target();

      if ( t )
      {
        if ( mortal_strike )
          mortal_strike->execute_on_target( t );
        if ( bloodthirst || bloodbath )
        {
          if ( bloodbath && p()->buff.bloodbath->check() )
            bloodbath->execute_on_target( t );
          else
            bloodthirst->execute_on_target( t );
        }
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );
    p()->buff.merciless_bonegrinder->expire();
  }
};

// Revenge ==================================================================

struct revenge_t : public warrior_attack_t
{
  double shield_slam_reset;
  action_t* seismic_action;
  action_t* lightning_strike;
  double frothing_berserker_chance;
  double rage_from_frothing_berserker;
  revenge_t( warrior_t* p, util::string_view options_str, bool seismic = false )
    : warrior_attack_t( seismic ? "revenge_seismic_reverberation" : "revenge", p, p->talents.protection.revenge ),
      shield_slam_reset( p->talents.protection.strategist->effectN( 1 ).percent() ),
      seismic_action( nullptr ),
      lightning_strike( nullptr ),
      frothing_berserker_chance( p->talents.warrior.frothing_berserker->proc_chance() ),
      rage_from_frothing_berserker( p->talents.warrior.frothing_berserker->effectN( 1 ).percent() )
    {
      parse_options( options_str );
      aoe           = -1;
      impact_action = p->active.deep_wounds_PROT;
      base_multiplier *= 1.0 + p -> talents.protection.best_served_cold -> effectN( 1 ).percent();
      if ( seismic )
      {
        background = proc = true;
        base_multiplier *= 1.0 + p -> find_spell( 382956 ) -> effectN( 3 ).percent();
      }
      else if ( p -> talents.warrior.seismic_reverberation -> ok() )
      {
        seismic_action = new revenge_t( p, "", true );
        add_child( seismic_action );
      }

      if ( p->talents.mountain_thane.lightning_strikes->ok() )
      {
        if ( seismic )
          lightning_strike = get_action<lightning_strike_t>( "lightning_strike_revenge_seismic_reverberation", p );
        else
          lightning_strike = get_action<lightning_strike_t>( "lightning_strike_revenge", p );
        add_child( lightning_strike );
      }
  }

  double cost_pct_multiplier() const override
  {
    double cost = warrior_attack_t::cost_pct_multiplier();
    cost *= 1.0 + p()->buff.revenge->check_value();
    //cost *= 1.0 + p()->buff.vengeance_revenge->check_value();
    return cost;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p()->buff.revenge->expire();

    if ( p()->talents.warrior.seismic_reverberation->ok() && !background &&
    execute_state->n_targets >= p()->talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      p()->buff.seismic_reverberation_revenge->trigger();
      seismic_action->execute_on_target( target );
    }

    if ( rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
      if ( p()->sets->has_set_bonus( WARRIOR_PROTECTION, TWW1, B2 ) )
        p()->buff.expert_strategist->trigger();
    }

    if ( p()->talents.protection.show_of_force->ok() )
    {
      p()->buff.show_of_force->trigger();
    }

    if ( p()->talents.warrior.frothing_berserker->ok() && !background && rng().roll( frothing_berserker_chance ) )
    {
      p()->resource_gain(RESOURCE_RAGE, last_resource_cost * rage_from_frothing_berserker, p()->gain.frothing_berserker);
    }

    if ( !background && p()->talents.colossus.colossal_might->ok() && execute_state -> n_targets >= p()->talents.colossus.colossal_might->effectN( 1 ).base_value() )
    {
      if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
      {
        p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() ) );
      }
      p()->buff.colossal_might->trigger();
    }

    if ( p()->talents.mountain_thane.lightning_strikes->ok() )
    {
      auto chance = p()->talents.mountain_thane.lightning_strikes->effectN( 1 ).percent();
      if ( p()->buff.avatar->check() )
        chance *= 1.0 + p()->talents.mountain_thane.lightning_strikes->effectN( 2 ).percent();
      if ( p()->talents.mountain_thane.gathering_clouds->ok() )
        chance *= 1.0 + p()->talents.mountain_thane.gathering_clouds->effectN( 1 ).percent();
      if ( rng().roll( chance ) )
      {
        lightning_strike->execute();
      }
    }

    if ( p() -> sets->has_set_bonus( WARRIOR_PROTECTION, T29, B2 ) )
      p()->buff.vanguards_determination->trigger();

    // 25% proc chance found via testing
    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) )
    {
      p() -> buff.fervid -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.25 );
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();
    if( p() -> buff.revenge -> up() && p() -> talents.protection.best_served_cold -> ok() )
    {
      am /= 1.0 + p()->talents.protection.best_served_cold->effectN( 1 ).percent();
      am *= 1.0 + p()->talents.protection.best_served_cold->effectN( 1 ).percent() +
            p()->buff.revenge->data().effectN( 2 ).percent();
    }

    am *= 1.0 + p() -> talents.protection.show_of_force -> effectN( 2 ).percent();

    return am;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = warrior_attack_t::composite_da_multiplier( state );

    if ( p()->talents.colossus.one_against_many->ok() )
    {
      m *= 1.0 + ( p()->talents.colossus.one_against_many->effectN( 1 ).percent() * std::min( state -> n_targets,  as<unsigned int>( p()->talents.colossus.one_against_many->effectN( 2 ).base_value() ) ) );
    }

    return m;
  }
};

// Enraged Regeneration ===============================================

struct enraged_regeneration_t : public warrior_heal_t
{
  enraged_regeneration_t( warrior_t* p, util::string_view options_str )
    : warrior_heal_t( "enraged_regeneration", p, p->talents.fury.enraged_regeneration )
  {
    parse_options( options_str );
    range         = -1;
    base_pct_heal = data().effectN( 1 ).percent();
  }
};

// Shield Charge ============================================================

struct shield_charge_damage_t : public warrior_attack_t
{
  double rage_gain;
  shield_charge_damage_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 385954 ) ),
    rage_gain( p->find_spell( 385954 )->effectN( 4 ).resource( RESOURCE_RAGE ) )
  {
    background = true;
    may_miss = false;
    energize_type = action_energize::NONE;

    aoe = 0;
    // this spell has both coefficients in it, force #1
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    rage_gain += p->talents.protection.champions_bulwark->effectN( 2 ).resource( RESOURCE_RAGE );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.protection.champions_bulwark->ok() )
    {
      am *= 1.0 + p()->talents.protection.champions_bulwark->effectN( 3 ).percent();
    }
    return am;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.protection.champions_bulwark->ok() )
    {
    if ( p()->buff.shield_block->check() )
    {
      p()->buff.shield_block->extend_duration( p(), p() -> buff.shield_block->buff_duration() );
    }
    else
    {
      p()->buff.shield_block->trigger();
    }
      p()->buff.revenge->trigger();
    }

    if ( p()->talents.protection.battering_ram->ok() )
    {
      p()->buff.battering_ram->trigger();
    }

    p()->resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.shield_charge );
  }
};

struct shield_charge_damage_aoe_t : public warrior_attack_t
{
  shield_charge_damage_aoe_t( util::string_view name, warrior_t* p )
    : warrior_attack_t( name, p, p->find_spell( 385954 ) )
  {
    background = true;
    may_miss = false;
    energize_type = action_energize::NONE;

    aoe = -1;

    // this spell has both coefficients in it, force #2
    attack_power_mod.direct = data().effectN( 2 ).ap_coeff();
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
  {
    warrior_attack_t::available_targets( tl );
    // Remove our main target from the target list, as aoe hits all other mobs
    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.protection.champions_bulwark->ok() )
    {
      am *= 1.0 + p()->talents.protection.champions_bulwark->effectN( 3 ).percent();
    }
    return am;
  }
};

struct shield_charge_t : public warrior_attack_t
{
  double movement_speed_increase;
  action_t* shield_charge_damage;
  action_t* shield_charge_damage_aoe;
  shield_charge_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shield_charge", p, p->talents.protection.shield_charge ),
      movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    energize_type = action_energize::NONE;
    movement_directionality = movement_direction_type::OMNI;

    shield_charge_damage = get_action<shield_charge_damage_t>( "shield_charge_main", p );
    shield_charge_damage_aoe = get_action<shield_charge_damage_aoe_t>( "shield_charge_aoe", p );
    add_child( shield_charge_damage );
    add_child( shield_charge_damage_aoe );
  }

  timespan_t calc_charge_time( double distance )
  {
    return timespan_t::from_seconds( distance /
      ( p()->base_movement_speed * ( 1 + p()->stacking_movement_modifier() + movement_speed_increase ) ) );
  }

  void execute() override
  {
    if ( p()->current.distance_to_move > 0 )
    {
      p()->buff.shield_charge_movement->trigger( 1, movement_speed_increase, 1, calc_charge_time( p()->current.distance_to_move ) );
      p()->current.moving_away = 0;
    }
    warrior_attack_t::execute();

    shield_charge_damage->execute_on_target( target );
    // If we have more than one target, trigger aoe as well
    if ( sim -> target_non_sleeping_list.size() > 1 )
      shield_charge_damage_aoe->execute_on_target( target );
  }

  bool ready() override
  {
    if ( p()->buff.charge_movement->check() || p()->buff.heroic_leap_movement->check() ||
         p()->buff.intervene_movement->check() || p()->buff.shield_charge_movement->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Shield Slam ==============================================================

// Linked action for shield slam aoe with T30 Protection
struct earthen_smash_t : public warrior_attack_t
{
  earthen_smash_t( util::string_view name, warrior_t* p )
  : warrior_attack_t( name, p, p->find_spell( 410219 ) )
  {
    background = true;
    aoe = -1;
  }
};

// Linked action for shield slam fervid bite T31 Protection
struct fervid_bite_t : public warrior_attack_t
{
  fervid_bite_t( util::string_view name, warrior_t* p )
  : warrior_attack_t( name, p, p->find_spell( 425534 ) )
  {
    background = true;
    ignores_armor = true;
  }
};

struct shield_slam_t : public warrior_attack_t
{
  double rage_gain;
  action_t* earthen_smash;
  action_t* fervid_bite;
  int aoe_targets;
  shield_slam_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shield_slam", p, p->spell.shield_slam ),
    rage_gain( p->spell.shield_slam->effectN( 3 ).resource( RESOURCE_RAGE ) ),
    earthen_smash( get_action<earthen_smash_t>( "earthen_smash", p ) ),
    fervid_bite( get_action<fervid_bite_t>( "fervid_bite", p ) ),
    aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    energize_type = action_energize::NONE;
    rage_gain += p->talents.protection.heavy_repercussions->effectN( 2 ).resource( RESOURCE_RAGE );
    rage_gain += p->talents.protection.impenetrable_wall->effectN( 2 ).resource( RESOURCE_RAGE );

    if ( p -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B2 ) )
        base_multiplier *= 1.0 + p -> sets -> set( WARRIOR_PROTECTION, T30, B2 ) -> effectN( 1 ).percent();

    radius = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.shield_block->up() )
    {
      double sb_increase = p() -> spell.shield_block_buff -> effectN( 2 ).percent();
      am *= 1.0 + sb_increase;
    }

    if ( p()->talents.protection.punish.ok() )
    {
      am *= 1.0 + p()->talents.protection.punish->effectN( 1 ).percent();
    }

    if ( p()->buff.violent_outburst->check() )
    {
      am *= 1.0 + p()->buff.violent_outburst->data().effectN( 1 ).percent();
    }

    if ( p() -> buff.brace_for_impact -> up() )
    {
      am *= 1.0 + p()->buff.brace_for_impact -> stack_value();
    }

    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B2 ) && p() -> buff.last_stand -> up() )
    {
        am *= 1.0 + p() -> talents.protection.last_stand -> effectN( 3 ).percent();
    }

    return am;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->buff.shield_block->up() && p()->talents.protection.heavy_repercussions->ok() )
    {
      p () -> buff.shield_block -> extend_duration( p(),
          timespan_t::from_seconds( p() -> talents.protection.heavy_repercussions -> effectN( 1 ).percent() ) );
    }

    if ( p()->talents.protection.impenetrable_wall->ok() )
    {
      p()->cooldown.shield_wall->adjust( - timespan_t::from_seconds( p()->talents.protection.impenetrable_wall->effectN( 1 ).base_value() ) );
    }

    auto total_rage_gain = rage_gain;

    if ( p() -> talents.protection.brace_for_impact->ok() )
      p() -> buff.brace_for_impact -> trigger();

    if ( p()->buff.violent_outburst->check() )
    {
      p()->buff.ignore_pain->trigger();
      p()->buff.violent_outburst->expire();
      total_rage_gain *= 1.0 + p() -> buff.violent_outburst->data().effectN( 3 ).percent();
    }

    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B2 ) ) 
    {
      p()->cooldown.last_stand->adjust( - timespan_t::from_seconds( p() -> sets -> set(WARRIOR_PROTECTION, T30, B2 ) -> effectN( 2 ).base_value() ) );
      // Value is doubled with last stand up, so we apply the same effect twice.
      if ( p() -> buff.last_stand -> up() )
      {
        p()->cooldown.last_stand->adjust( - timespan_t::from_seconds( p() -> sets -> set(WARRIOR_PROTECTION, T30, B2 ) -> effectN( 2 ).base_value() ) );
      }
    }

    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B4 ) && p() -> buff.earthen_tenacity -> up() )
    {
      earthen_smash -> execute_on_target( target );
    }

    p() -> buff.meat_cleaver->decrement();

    p()->resource_gain( RESOURCE_RAGE, total_rage_gain, p() -> gain.shield_slam );

    if ( p()->talents.mountain_thane.thunder_blast->ok() && rng().roll( p()->talents.mountain_thane.thunder_blast->effectN( 1 ).percent() ) )
    {
      p()->buff.thunder_blast->trigger();
    }
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    warrior_td_t* td = p() -> get_target_data( state -> target );

    if ( p()->talents.protection.punish.ok() )
    {
      td -> debuffs_punish -> trigger();
    }

    if ( p()->talents.colossus.colossal_might->ok() )
    {
      // Gain 2 stacks on a crit with precise might, 1 otherwise.
      if ( p()->talents.colossus.precise_might->ok() && state->result == RESULT_CRIT )
      {
        if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
        {
          p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() * 2 ) );
        }
        p()->buff.colossal_might->trigger( 2 );
      }
      else
      {
        if ( p()->talents.colossus.dominance_of_the_colossus->ok() && p()->buff.colossal_might->at_max_stacks() )
        {
          p()->cooldown.demolish->adjust( - timespan_t::from_seconds( p()->talents.colossus.dominance_of_the_colossus->effectN( 2 ).base_value() ) );
        }
        p()->buff.colossal_might->trigger();
      }
    }

    if ( state->result == RESULT_CRIT && p()->sets->has_set_bonus( WARRIOR_PROTECTION, TWW1, B4 ) )
    {
      p()->buff.brutal_followup->trigger();
    }

    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) && p() -> buff.fervid -> up() )
    {
      double total_amount = 0;
      if ( td->dots_deep_wounds->is_ticking() )
      {
        td->dots_deep_wounds->current_action->calculate_tick_amount( td->dots_deep_wounds->state, td->dots_deep_wounds->get_tick_factor() * td->dots_deep_wounds->current_stack() );
        auto amount = td->dots_deep_wounds->state->result_raw * td->dots_deep_wounds->ticks_left_fractional();
        // Damage reduction
        amount *= p() -> sets -> set( WARRIOR_PROTECTION, T31, B2 ) -> effectN( 1 ).percent();
        total_amount += amount;
        td->dots_deep_wounds->cancel();
      }

      if ( td->dots_rend->is_ticking() )
      {
        td->dots_rend->current_action->calculate_tick_amount( td->dots_rend->state, td->dots_rend->get_tick_factor() * td->dots_rend->current_stack() );
        auto amount = td->dots_rend->state->result_raw * td->dots_rend->ticks_left_fractional();
        // Damage reduction
        amount *= p() -> sets -> set( WARRIOR_PROTECTION, T31, B2 ) -> effectN( 1 ).percent();
        total_amount += amount;
        td->dots_rend->cancel();
      }

      if ( td->dots_thunderous_roar->is_ticking() )
      {
        td->dots_thunderous_roar->current_action->calculate_tick_amount( td->dots_thunderous_roar->state, td->dots_thunderous_roar->get_tick_factor() * td->dots_thunderous_roar->current_stack() );
        auto amount = td->dots_thunderous_roar->state->result_raw * td->dots_thunderous_roar->ticks_left_fractional();
        // Damage reduction, Thunderous Roar uses effect4, instead of effect1
        amount *= p() -> sets -> set( WARRIOR_PROTECTION, T31, B2 ) -> effectN( 4 ).percent();
        total_amount += amount;
        td->dots_thunderous_roar->cancel();
      }

      if ( total_amount > 0 )
      {
        fervid_bite->execute_on_target( state->target, total_amount );
      }

      if( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B4 ) )
      {
        p() -> cooldown.thunderous_roar -> adjust ( -1.0 * p() -> sets -> set( WARRIOR_PROTECTION, T31, B4 ) -> effectN( 2 ).time_value() );
        p() -> cooldown.thunder_clap -> reset( true );
      }

      p() -> buff.fervid -> expire();
      p() -> buff.fervid_opposition -> trigger();
    }

    if ( p()->talents.mountain_thane.burst_of_power->ok() && p()->buff.burst_of_power->up() && p()->cooldown.burst_of_power_icd->up() )
    {
      p()->cooldown.burst_of_power_icd->start();
      p()->buff.burst_of_power->decrement();
      // Reset CD after everything resolves
      make_event( *p()->sim, [ this ] { p()->cooldown.shield_slam->reset( true ); } );
    }
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Shockwave ================================================================

struct shockwave_t : public warrior_attack_t
{
  shockwave_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "shockwave", p, p->talents.warrior.shockwave )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    aoe                               = -1;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( state -> n_targets >= as<size_t>( p() -> talents.warrior.rumbling_earth->effectN( 1 ).base_value() ) )
    {
      p()->cooldown.shockwave->adjust(
          timespan_t::from_seconds( p()->talents.warrior.rumbling_earth->effectN( 2 ).base_value() ) );
    }
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = warrior_attack_t::composite_da_multiplier( state );

    if ( p()->talents.colossus.one_against_many->ok() )
    {
      m *= 1.0 + ( p()->talents.colossus.one_against_many->effectN( 1 ).percent() * std::min( state -> n_targets,  as<unsigned int>( p()->talents.colossus.one_against_many->effectN( 2 ).base_value() ) ) );
    }

    return m;
  }
};

// Slayer's Strike ==========================================================
struct slayers_strike_t : public warrior_attack_t
{
  int imminent_demise_tracker;
  int imminent_demise_trigger_threshold;
  slayers_strike_t( warrior_t* p )
    : warrior_attack_t( "slayers_strike", p, p->spell.slayers_strike ),
    imminent_demise_tracker( 0 ),
    imminent_demise_trigger_threshold( 0 )
  {
    special = true;
    background = true;

    if( p->talents.slayer.imminent_demise -> ok() )
      imminent_demise_trigger_threshold = as<int>( p->talents.slayer.imminent_demise -> effectN( 1 ).base_value() );
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      td( state -> target ) -> debuffs_marked_for_execution->trigger();
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p() -> talents.slayer.imminent_demise -> ok() )
    {
      imminent_demise_tracker++;
      if ( imminent_demise_tracker == imminent_demise_trigger_threshold )
      {
        imminent_demise_tracker = 0;
        p() -> buff.sudden_death -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0 );
        p()->cooldown.execute->reset( true );
      }
    }
  }

  void reset() override
  {
    warrior_attack_t::reset();
    imminent_demise_tracker = 0;
  }
};

// Storm Bolt ===============================================================

struct storm_bolt_t : public warrior_attack_t
{
  storm_bolt_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "storm_bolt", p, p->talents.warrior.storm_bolt )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    if ( p->talents.mountain_thane.storm_bolts->ok() )
      aoe = 1 + as<int>( p->talents.mountain_thane.storm_bolts->effectN( 1 ).base_value() );
    if ( p->talents.slayer.unrelenting_onslaught->ok() )
      usable_while_channeling = true;
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Tough as Nails ===========================================================

struct tough_as_nails_t : public warrior_attack_t
{
  tough_as_nails_t( warrior_t* p ) :
    warrior_attack_t( "tough_as_nails", p, p -> find_spell( 385890 ) )
  {
    may_crit = false;
    ignores_armor = true;

    background = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p() -> cooldown.tough_as_nails_icd -> start();
  }
};

// Victory Rush =============================================================

struct victory_rush_heal_t : public warrior_heal_t
{
  victory_rush_heal_t( warrior_t* p ) : warrior_heal_t( "victory_rush_heal", p, p->find_spell( 118779 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background    = true;
  }
  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

struct victory_rush_t : public warrior_attack_t
{
  victory_rush_heal_t* victory_rush_heal;
  int aoe_targets;

  victory_rush_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "victory_rush", p, p->spell.victory_rush ), victory_rush_heal( new victory_rush_heal_t( p ) ),
    aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    parse_options( options_str );
    if ( p->non_dps_mechanics )
    {
      // With imp ww while you do hit every target, you still only get the single MT heal
      execute_action = victory_rush_heal;
    }
    cooldown->duration = timespan_t::from_seconds( 1000.0 );

    radius = 5;
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p() -> buff.meat_cleaver->decrement();
  }
};

// Whirlwind ================================================================

struct whirlwind_fury_damage_t : public warrior_attack_t
{
  bool seismic_reverberation;
  whirlwind_fury_damage_t( util::string_view name, warrior_t* p, const spell_data_t* whirlwind, bool seismic ) : warrior_attack_t( name, p, whirlwind ),
  seismic_reverberation( seismic )
  {
    background = true;
    aoe = -1;
    reduced_aoe_targets = 5.0;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.warrior.seismic_reverberation->ok() && seismic_reverberation )
    {
      am *= 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 3 ).percent();
    }
    return am;
  }
};

struct fury_whirlwind_parent_t : public warrior_attack_t
{
  whirlwind_fury_damage_t* mh_first_attack;
  whirlwind_fury_damage_t* oh_first_attack;
  whirlwind_fury_damage_t* mh_other_attack;
  whirlwind_fury_damage_t* oh_other_attack;
  whirlwind_fury_damage_t* mh_seismic_reverberation_attack;
  whirlwind_fury_damage_t* oh_seismic_reverberation_attack;
  double base_rage_gain;
  double additional_rage_gain_per_target;
  fury_whirlwind_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "whirlwind", p, p->spec.whirlwind ),
      mh_first_attack( nullptr ),
      oh_first_attack( nullptr ),
      mh_other_attack( nullptr ),
      oh_other_attack( nullptr ),
      mh_seismic_reverberation_attack( nullptr ),
      oh_seismic_reverberation_attack( nullptr ),
      base_rage_gain( p->spec.whirlwind->effectN( 1 ).base_value() ),
      additional_rage_gain_per_target( p->spec.whirlwind->effectN( 2 ).base_value() )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_first_attack         = new whirlwind_fury_damage_t( "whirlwind_mh_first", p, data().effectN( 4 ).trigger(), false );
      mh_first_attack->weapon = &( p->main_hand_weapon );
      mh_first_attack->radius = radius;
      add_child( mh_first_attack );

      mh_other_attack         = new whirlwind_fury_damage_t( "whirlwind_mh_others", p, data().effectN( 6 ).trigger(), false );
      mh_other_attack->weapon = &( p->main_hand_weapon );
      mh_other_attack->radius = radius;
      add_child( mh_other_attack );

      if ( p->talents.warrior.seismic_reverberation.ok() )
      {
        mh_seismic_reverberation_attack = new whirlwind_fury_damage_t( "whirlwind_mh_seismic_reverberation", p, p->find_spell( 385233 ), true );
        mh_seismic_reverberation_attack->weapon = &( p->main_hand_weapon );
        mh_seismic_reverberation_attack->radius = radius;
        add_child( mh_seismic_reverberation_attack );
      }

      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_first_attack         = new whirlwind_fury_damage_t( "whirlwind_oh_first", p, data().effectN( 5 ).trigger(), false );
        oh_first_attack->weapon = &( p->off_hand_weapon );
        oh_first_attack->radius = radius;
        add_child( oh_first_attack );

        oh_other_attack         = new whirlwind_fury_damage_t( "whirlwind_oh_others", p, data().effectN( 7 ).trigger(), false );
        oh_other_attack->weapon = &( p->off_hand_weapon );
        oh_other_attack->radius = radius;
        add_child( oh_other_attack );

        if ( p->talents.warrior.seismic_reverberation.ok() )
        {
          oh_seismic_reverberation_attack         = new whirlwind_fury_damage_t( "whirlwind_oh_seismic_reverberation", p, p->find_spell( 385234 ), true );
          oh_seismic_reverberation_attack->weapon = &( p->off_hand_weapon );
          oh_seismic_reverberation_attack->radius = radius;
          add_child( oh_seismic_reverberation_attack );
        }
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.fury.improved_whirlwind->ok() )
    {
      const int num_available_targets = std::min( 5, as<int>( target_list().size() ));  // Capped to 5 targets
      p()->resource_gain( RESOURCE_RAGE, ( base_rage_gain + additional_rage_gain_per_target * num_available_targets ),
                        p()->gain.whirlwind );

      p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->max_stack() );
    }

    mh_first_attack->execute_on_target( target );
    if ( oh_first_attack )
      oh_first_attack->execute_on_target( target );

    if ( p() -> talents.warrior.seismic_reverberation.ok() && mh_first_attack->num_targets_hit >= p() -> talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      mh_seismic_reverberation_attack->execute_on_target( target );
      if ( oh_seismic_reverberation_attack )
        oh_seismic_reverberation_attack->execute_on_target( target );
    }

    make_event( *sim, timespan_t::from_millis(data().effectN( 6 ).misc_value1()), [ this ]() { mh_other_attack->execute_on_target( target ); } );
    make_event( *sim, timespan_t::from_millis(data().effectN( 8 ).misc_value1()), [ this ]() { mh_other_attack->execute_on_target( target ); } );
    if ( oh_other_attack )
    {
      make_event( *sim, timespan_t::from_millis(data().effectN( 7 ).misc_value1()), [ this ]() { oh_other_attack->execute_on_target( target ); } );
      make_event( *sim, timespan_t::from_millis(data().effectN( 9 ).misc_value1()), [ this ]() { oh_other_attack->execute_on_target( target ); } );
    }
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Arms Whirlwind ========================================================

struct whirlwind_arms_damage_t : public warrior_attack_t
{
  bool seismic_reverberation;
  whirlwind_arms_damage_t( util::string_view name, warrior_t* p, const spell_data_t* whirlwind, bool seismic ) : warrior_attack_t( name, p, whirlwind ),
  seismic_reverberation( seismic )
  {
    aoe = -1;
    reduced_aoe_targets = 5.0;
    background = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p()->talents.warrior.seismic_reverberation->ok() && seismic_reverberation )
    {
      am *= 1.0 + p()->talents.warrior.seismic_reverberation->effectN( 3 ).percent();
    }

    if ( !p()->buff.sweeping_strikes->up() && p()->buff.collateral_damage->up() )
    {
      am *= 1.0 + p()->buff.collateral_damage->stack_value();
    }

    return am;
  }

  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = warrior_attack_t::composite_da_multiplier( state );

    if ( p()->talents.colossus.one_against_many->ok() )
    {
      m *= 1.0 + ( p()->talents.colossus.one_against_many->effectN( 1 ).percent() * std::min( state -> n_targets,  as<unsigned int>( p()->talents.colossus.one_against_many->effectN( 2 ).base_value() ) ) );
    }

    return m;
  }

  double tactician_cost() const override
  {
    return 0;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->talents.arms.collateral_damage.ok() && !p()->buff.sweeping_strikes->up() && p()->buff.collateral_damage->up() && data().id() == 411547 )
    {
      p() -> buff.collateral_damage -> expire();
    }
  }
};

struct arms_whirlwind_parent_t : public warrior_attack_t
{
  double max_rage;
  slam_t* fervor_slam;
  whirlwind_arms_damage_t* first_attack;
  whirlwind_arms_damage_t* second_attack;
  whirlwind_arms_damage_t* third_attack;
  whirlwind_arms_damage_t* seismic_reverberation_attack;

  arms_whirlwind_parent_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "whirlwind", p, p->spell.whirlwind ),
      fervor_slam( nullptr ),
      first_attack( nullptr ),
      second_attack( nullptr ),
      third_attack( nullptr ),
      seismic_reverberation_attack( nullptr )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      first_attack         = new whirlwind_arms_damage_t( "whirlwind_1", p, data().effectN( 1 ).trigger(), false );
      first_attack->weapon = &( p->main_hand_weapon );
      first_attack->radius = radius;
      add_child( first_attack );

      second_attack         = new whirlwind_arms_damage_t( "whirlwind_2", p, data().effectN( 2 ).trigger(), false );
      second_attack->weapon = &( p->main_hand_weapon );
      second_attack->radius = radius;
      add_child( second_attack );

      third_attack         = new whirlwind_arms_damage_t( "whirlwind_3", p, data().effectN( 3 ).trigger(), false );
      third_attack->weapon = &( p->main_hand_weapon );
      third_attack->radius = radius;
      add_child( third_attack );

      if ( p->talents.warrior.seismic_reverberation.ok() )
      {
        seismic_reverberation_attack         = new whirlwind_arms_damage_t( "whirlwind_seismic_reverberation", p, p->find_spell( 385228 ), true );
        seismic_reverberation_attack->weapon = &( p->main_hand_weapon );
        seismic_reverberation_attack->radius = radius;
        add_child( seismic_reverberation_attack );
      }

      if ( p->talents.arms.fervor_of_battle->ok() )
      {
        fervor_slam                               = new slam_t( "slam_whirlwind_fervor_of_battle", p );
        fervor_slam->from_Fervor                  = true;
        add_child( fervor_slam );
      }
    }
  }

  double tactician_cost() const override
  {
    double c = warrior_attack_t::cost();

    if ( sim->log )
    {
      sim->out_debug.printf( "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f",
                             name(), c, cost() );
    }
    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->buff.storm_of_swords->up() )
    {
      p()->buff.storm_of_swords->expire();
    }

    p()->buff.storm_of_swords->trigger();

    first_attack->execute_on_target( target );

    if ( p() -> talents.arms.fervor_of_battle.ok() && first_attack->num_targets_hit >= p() -> talents.arms.fervor_of_battle -> effectN( 1 ).base_value() )
      fervor_slam->execute_on_target( target );

    if ( p() -> talents.warrior.seismic_reverberation.ok() && first_attack->num_targets_hit >= p() -> talents.warrior.seismic_reverberation->effectN( 1 ).base_value() )
    {
      seismic_reverberation_attack->execute_on_target( target );
    }

    make_event( *sim, timespan_t::from_millis(data().effectN( 2 ).misc_value1()), [ this ]() { second_attack->execute_on_target( target ); } );
    make_event( *sim, timespan_t::from_millis(data().effectN( 3 ).misc_value1()), [ this ]() { third_attack->execute_on_target( target ); } );
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Wrecking Throw ========================================================

struct wrecking_throw_t : public warrior_attack_t
{
  wrecking_throw_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "wrecking_throw", p, p->talents.warrior.wrecking_throw )
  {
    parse_options( options_str );
    may_crit = may_parry = may_dodge = may_block = false;
    weapon = &( player->main_hand_weapon );
    attack_power_mod.direct = 1.0;
  }
  // add absorb shield bonus (are those even in SimC?)
};

// ==========================================================================
// Covenant Abilities
// ==========================================================================

// Conquerors Banner=========================================================

struct conquerors_banner_t : public warrior_spell_t
{
  conquerors_banner_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "conquerors_banner", p, p->covenant.conquerors_banner )
  {
    parse_options( options_str );
    energize_resource       = RESOURCE_NONE;
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.conquerors_banner->trigger();
    p()->buff.conquerors_mastery->trigger();
  }
};

// Champion's Spear==========================================================

struct champions_spear_damage_t : public warrior_attack_t
{
  double rage_gain;
  champions_spear_damage_t( util::string_view name, warrior_t* p ) : warrior_attack_t( name, p, p->find_spell( 376080 ) ),
  rage_gain( p->find_spell( 376080 )->effectN( 3 ).resource( RESOURCE_RAGE ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
    aoe                        = -1;
    reduced_aoe_targets        = 5.0;
    dual                       = true;
    allow_class_ability_procs  = true;
    energize_type     = action_energize::NONE;

    rage_gain *= 1.0 + p->talents.warrior.piercing_challenge->effectN( 2 ).percent();
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.champions_spear );
  }
};

struct champions_spear_t : public warrior_attack_t
{
  action_t* champions_spear_attack;
  champions_spear_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "champions_spear", p, p->talents.warrior.champions_spear ),
    champions_spear_attack( get_action<champions_spear_damage_t>( "champions_spear_damage", p ) )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    aoe = -1;
    reduced_aoe_targets               = 5.0;

    execute_action = champions_spear_attack;
    add_child( execute_action );

    energize_type     = action_energize::NONE;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p()->talents.warrior.champions_might->ok() )
    {
      if ( result_is_hit( state->result ) )
      {
        td( state->target )->debuffs_champions_might->trigger();
      }
    }
  }
};

// ==========================================================================
// Warrior Spells
// ==========================================================================

// Avatar ===================================================================

struct avatar_t : public warrior_spell_t
{
  avatar_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_spell_t( n, p, spell )
  {

    parse_options( options_str );
    callbacks = false;
    harmful   = false;
    target    = p;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->talents.warrior.immovable_object->ok() )
      p()->buff.shield_wall->trigger( p()->talents.warrior.immovable_object->effectN( 2 ).time_value() );

    p()->buff.avatar->extend_duration_or_trigger();

    if ( p()->talents.warrior.berserkers_torment.ok() )
    {
      action_t* torment_ability = p()->active.torment_recklessness;
      torment_ability->schedule_execute();
    }
    if ( p()->talents.warrior.blademasters_torment.ok() )
    {
      p()->buff.sweeping_strikes->extend_duration_or_trigger( p()->talents.warrior.blademasters_torment->effectN( 1 ).time_value() );
    }
    if ( p()->talents.warrior.titans_torment->ok() )
    {
      action_t* torment_ability = p()->active.torment_odyns_fury;
      torment_ability->schedule_execute();
    }

    if ( p()->talents.warrior.warlords_torment->ok() )
    {
      const timespan_t trigger_duration = p()->talents.warrior.warlords_torment->effectN( 1 ).time_value();
      p()->buff.recklessness_warlords_torment->extend_duration_or_trigger( trigger_duration );
    }

    if ( p()->talents.mountain_thane.avatar_of_the_storm->ok() )
    {
      p()->buff.thunder_blast->trigger( as<int> ( p()->talents.mountain_thane.avatar_of_the_storm->effectN( 1 ).base_value() ) );
      p()->cooldown.thunder_clap->reset( true );
    }
  }

  bool verify_actor_spec() const override // no longer needed ?
  {
    // Do not check spec if Arms talent avatar is available, so that spec check on the spell (required: protection) does not fail.
    if ( p()->talents.warrior.avatar->ok() && p()->specialization() == WARRIOR_ARMS )
      return true;

    return warrior_spell_t::verify_actor_spec();
  }
};

// Torment Avatar ===================================================================

struct torment_avatar_t : public warrior_spell_t
{
  torment_avatar_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_spell_t( n, p, spell )
  {
    parse_options( options_str );
    callbacks = false;
    target    = p;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->talents.warrior.berserkers_torment->ok() )
    {
      const timespan_t trigger_duration = p()->talents.warrior.berserkers_torment->effectN( 2 ).time_value();
      p()->buff.avatar->extend_duration_or_trigger( trigger_duration );
    }
    if ( p()->talents.warrior.titans_torment->ok() )
    {
      const timespan_t trigger_duration = p()->talents.warrior.titans_torment->effectN( 1 ).time_value();
      p()->buff.avatar->extend_duration_or_trigger( trigger_duration );   
    }
  }
};

// Battle Shout ===================================================================

struct battle_shout_t : public warrior_spell_t
{
  battle_shout_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "battle_shout", p, p->spell.battle_shout )
  {
    parse_options( options_str );
    usable_while_channeling = true;
    harmful = false;
    target  = p;

    background = sim->overrides.battle_shout != 0;
  }

  void execute() override
  {
    sim->auras.battle_shout->trigger();
  }

  bool ready() override
  {
    return warrior_spell_t::ready() && !sim->auras.battle_shout->check();
  }
};

// Berserker Rage ===========================================================

struct berserker_rage_t : public warrior_spell_t
{
  berserker_rage_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "berserker_rage", p, p->spell.berserker_rage )
  {
    parse_options( options_str );
    callbacks   = false;
    use_off_gcd = true;
    range       = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.berserker_rage->trigger();
  }
};

// Battle Stance ===============================================================

struct battle_stance_t : public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  battle_stance_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "battle_stance", p, p->talents.warrior.battle_stance ), onoff(), onoffbool( false )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );
    harmful = false;
    target  = p;

    if ( onoff == "on" )
    {
      onoffbool = true;
    }
    else if ( onoff == "off" )
    {
      onoffbool = false;
    }
    else
    {
      sim->errorf( "Battle stance must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }

    use_off_gcd = true;

    //background = sim->overrides.battle_stance != 0;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    if ( onoffbool )
    {
      p()->buff.battle_stance->trigger();
    }
    else
    {
      p()->buff.battle_stance->expire();
    }
  }

  bool ready() override
  {
    if ( onoffbool && p()->buff.battle_stance->check() )
    {
      return false;
    }
    else if ( !onoffbool && !p()->buff.battle_stance->check() )
    {
      return false;
    }

    return warrior_spell_t::ready(); // && !sim->auras.battle_stance->check();
  }
};

// Berserker Stance ===============================================================

struct berserker_stance_t : public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  berserker_stance_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "berserker_stance", p, p->talents.warrior.berserker_stance ), onoff(), onoffbool( false )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );
    harmful = false;
    target  = p;

    if ( onoff == "on" )
    {
      onoffbool = true;
    }
    else if ( onoff == "off" )
    {
      onoffbool = false;
    }
    else
    {
      sim->errorf( "Berserker stance must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }

    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    if ( onoffbool )
    {
      p()->buff.berserker_stance->trigger();
    }
    else
    {
      p()->buff.berserker_stance->expire();
    }
  }

  bool ready() override
  {
    if ( onoffbool && p()->buff.berserker_stance->check() )
    {
      return false;
    }
    else if ( !onoffbool && !p()->buff.berserker_stance->check() )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// Defensive Stance ===============================================================

struct defensive_stance_t : public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  defensive_stance_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "defensive_stance", p, p->talents.warrior.defensive_stance ), onoff(), onoffbool( false )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );
    harmful = false;
    target = p;

    if ( onoff == "on" )
    {
      onoffbool = true;
    }
    else if ( onoff == "off" )
    {
      onoffbool = false;
    }
    else
    {
      sim->errorf( "Defensive stance must use the option 'toggle=on' or 'toggle=off'" );
      background = true;
    }

    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    if ( onoffbool )
    {
      p()->buff.defensive_stance->trigger();
    }
    else
    {
      p()->buff.defensive_stance->expire();
    }
  }

  bool ready() override
  {
    if ( onoffbool && p()->buff.defensive_stance->check() )
    {
      return false;
    }
    else if ( !onoffbool && !p()->buff.defensive_stance->check() )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// Die By the Sword  ==============================================================

struct die_by_the_sword_t : public warrior_spell_t
{
  die_by_the_sword_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "die_by_the_sword", p, p->talents.arms.die_by_the_sword )
  {
    parse_options( options_str );
    callbacks = false;
    range     = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.die_by_the_sword->trigger();
  }

  bool ready() override
  {
    if ( p()->main_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// In for the Kill ===================================================================

struct in_for_the_kill_t : public buff_t
{
  double base_value;
  double below_pct_increase_amount;
  double below_pct_increase;

  in_for_the_kill_t( warrior_t& p, util::string_view n, const spell_data_t* s )
    : buff_t( &p, n, s ),
      base_value( p.talents.arms.in_for_the_kill->effectN( 1 ).percent() ),
      below_pct_increase_amount( p.talents.arms.in_for_the_kill->effectN( 2 ).percent() ),
      below_pct_increase( p.talents.arms.in_for_the_kill->effectN( 3 ).base_value() )

  {
    add_invalidate( CACHE_ATTACK_HASTE );
    set_duration( s->duration() +
                  p.talents.arms.spiteful_serenity->effectN( 8 ).time_value() +
                  p.talents.arms.blunt_instruments->effectN( 2 ).time_value() );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    if ( source->target->health_percentage() <= below_pct_increase )
    {
      default_value = below_pct_increase_amount;
    }
    else
    {
      default_value = base_value;
    }
    return buff_t::trigger( stacks, value, chance, duration );
  }
};

// Last Stand ===============================================================

struct last_stand_t : public warrior_spell_t
{
  last_stand_t( warrior_t* p, util::string_view options_str ) : warrior_spell_t( "last_stand", p, p->talents.protection.last_stand )
  {
    parse_options( options_str );
    range              = -1;
    cooldown->duration = data().cooldown();
    cooldown -> duration += p -> talents.protection.bolster -> effectN( 1 ).time_value();
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->talents.protection.unnerving_focus->ok() )
    {
      p()->buff.unnerving_focus->trigger();
    }

    if ( p() -> talents.protection.bolster -> ok() )
    {
      if ( p()->buff.shield_block->check() )
      {
        p()->buff.shield_block->extend_duration( p(), data().duration() );
      }
      else
      {
        p()->buff.shield_block->trigger( data().duration() ) ;
      }
    }
    p()->buff.last_stand->trigger();
  }
};

// Rallying Cry ===============================================================

struct rallying_cry_t : public warrior_spell_t
{
  rallying_cry_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "rallying_cry", p, p->talents.warrior.rallying_cry )
  {
    parse_options( options_str );
    callbacks = false;
    range     = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    player->buffs.rallying_cry->trigger();
  }
};

// Recklessness =============================================================

struct recklessness_t : public warrior_spell_t
{
  recklessness_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_spell_t( n, p, spell )
  {
    parse_options( options_str );
    callbacks  = false;
    harmful    = false;
    target     = p;

    if ( p->talents.fury.reckless_abandon->ok() )
    {
      energize_amount   = p->talents.fury.reckless_abandon->effectN( 1 ).base_value() / 10.0;
      energize_type     = action_energize::ON_CAST;
      energize_resource = RESOURCE_RAGE;
    }
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.recklessness->extend_duration_or_trigger();

    if ( p()->talents.warrior.berserkers_torment.ok() )
    {
      action_t* torment_ability = p()->active.torment_avatar;
      torment_ability->schedule_execute();
    }

    if ( p()->talents.mountain_thane.snap_induction->ok() )
    {
      p()->buff.thunder_blast->trigger();
    }
  }

  bool verify_actor_spec() const override
  {
    if ( p()->talents.warrior.warlords_torment->ok() )
      return true;

    return warrior_spell_t::verify_actor_spec();
  }
};

// Torment Recklessness =============================================================

struct torment_recklessness_t : public warrior_spell_t
{
  torment_recklessness_t( warrior_t* p, util::string_view options_str, util::string_view n, const spell_data_t* spell )
    : warrior_spell_t( n, p, spell )
  {
    parse_options( options_str );
    callbacks  = false;
    harmful    = false;
    target     = p;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    const timespan_t trigger_duration = p()->talents.warrior.berserkers_torment->effectN( 2 ).time_value();
    p()->buff.recklessness->extend_duration_or_trigger( trigger_duration );
  }
};

// Ignore Pain =============================================================

struct ignore_pain_buff_t : public absorb_buff_t
{
  ignore_pain_buff_t( warrior_t* player ) : absorb_buff_t( player, "ignore_pain", player->talents.protection.ignore_pain )
  {
    cooldown->duration = 0_ms;
    set_absorb_source( player->get_stats( "ignore_pain" ) );
    set_absorb_gain( player->get_gain( "ignore_pain" ) );
  }

  // Custom consume implementation to allow minimum absorb amount.
  double consume( double amount, action_state_t* ) override
  {
    // IP only absorbs up to 55% of the damage taken
    amount *= debug_cast< warrior_t* >( player ) -> talents.protection.ignore_pain -> effectN( 2 ).percent();
    double absorbed = absorb_buff_t::consume( amount );

    return absorbed;
  }
};

struct ignore_pain_t : public warrior_spell_t
{
  ignore_pain_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "ignore_pain", p, p->talents.protection.ignore_pain )
  {
    parse_options( options_str );
    may_crit     = false;
    use_off_gcd  = true;
    range        = -1;
    target       = player;
    base_costs[ RESOURCE_RAGE ] = ( p->specialization() == WARRIOR_FURY ? 60 : p->specialization() == WARRIOR_ARMS ? 20 : 35);

    base_dd_max = base_dd_min = 0;
    resource_current = RESOURCE_RAGE;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    // 87.5% proc chance found via testing
    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) )
    {
      p() -> buff.fervid -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.875 );
    }
  }

  void impact( action_state_t* s ) override
  {
    // With the buff to warrior on Jan 23 2024
    // the amount gained is +5%.  Need to check cap as well.  This buff was stored in a dummy effect
    // in the protection aura
    // p() -> effectN( 23 ).percent();

    double new_ip = s -> result_amount;

    if ( p()->talents.colossus.no_stranger_to_pain->ok() )
    {
      new_ip *= 1.0 + p()->talents.colossus.no_stranger_to_pain->effectN( 1 ).percent();
    }

    double previous_ip = p() -> buff.ignore_pain -> current_value;

    // IP is capped to 30% of max health
    double ip_max_health_cap = p() -> max_health() * 0.3;

    if ( previous_ip + new_ip > ip_max_health_cap )
    {
      new_ip = ip_max_health_cap;
    }

    if ( new_ip > 0.0 )
    {
      p()->buff.ignore_pain->trigger( 1, new_ip );
    }
  }
};

// Shield Block =============================================================

struct shield_block_t : public warrior_spell_t
{
  shield_block_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "shield_block", p, p->spell.shield_block )
  {
    parse_options( options_str );
    cooldown->hasted = true;
    cooldown->charges += as<int>( p->spec.shield_block_2->effectN( 1 ).base_value() );
  }

  void execute() override
  {
    warrior_spell_t::execute();

    if ( p()->buff.shield_block->check() )
    {
      p()->buff.shield_block->extend_duration( p(), p() -> buff.shield_block->buff_duration() );
    }
    else
    {
      p()->buff.shield_block->trigger();
    }

    // 25% proc chance found via testing
    if ( p() -> sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) )
    {
      p() -> buff.fervid -> trigger( 1, buff_t::DEFAULT_VALUE(), 0.25 );
    }
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() )
    {
      return false;
    }

    return warrior_spell_t::ready();
  }
};

// Shield Wall ==============================================================

struct shield_wall_t : public warrior_spell_t
{
  shield_wall_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "shield_wall", p, p->talents.protection.shield_wall )
  {
    parse_options( options_str );
    harmful = false;
    range   = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.shield_wall->trigger( 1, p()->buff.shield_wall->data().effectN( 1 ).percent() );

    if ( p()->talents.warrior.immovable_object->ok() )
      p()->buff.avatar->trigger( p()->talents.warrior.immovable_object->effectN( 2 ).time_value() );
  }
};

// Spell Reflection  ==============================================================

struct spell_reflection_t : public warrior_spell_t
{
  spell_reflection_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "spell_reflection", p, p->talents.warrior.spell_reflection )
  {
    parse_options( options_str );
    use_off_gcd = usable_while_channeling = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.spell_reflection->trigger();
  }
};

// Taunt =======================================================================

struct taunt_t : public warrior_spell_t
{
  taunt_t( warrior_t* p, util::string_view options_str )
    : warrior_spell_t( "taunt", p, p->find_class_spell( "Taunt" ) )
  {
    parse_options( options_str );
    use_off_gcd = ignore_false_positive = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
    {
      target->taunt( player );
    }

    warrior_spell_t::impact( s );
  }
};

}  // UNNAMED NAMESPACE

// warrior_t::create_action  ================================================

action_t* warrior_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "avatar" )
    return new avatar_t( this, options_str, name, talents.warrior.avatar );
  if ( name == "battle_shout" )
    return new battle_shout_t( this, options_str );
  if ( name == "recklessness" )
    return new recklessness_t( this, options_str, name, specialization() == WARRIOR_FURY ? talents.fury.recklessness : find_spell( 1719 ) );
  if ( name == "battle_stance" )
    return new battle_stance_t( this, options_str );
  if ( name == "berserker_rage" )
    return new berserker_rage_t( this, options_str );
  if ( name == "berserker_stance" )
    return new berserker_stance_t( this, options_str );
  if ( name == "bladestorm" )
    return new bladestorm_t( this, options_str, name, specialization() == WARRIOR_FURY ? talents.fury.bladestorm : talents.arms.bladestorm );
  if ( name == "bloodthirst" )
    return new bloodthirst_t( this, options_str );
  if ( name == "bloodbath" )
    return new bloodbath_t( this, options_str );
  if ( name == "charge" )
    return new charge_t( this, options_str );
  if ( name == "cleave" )
    return new cleave_t( this, options_str );
  if ( name == "colossus_smash" )
    return new colossus_smash_t( this, options_str );
  if ( name == "conquerors_banner" )
    return new conquerors_banner_t( this, options_str );
  if ( name == "defensive_stance" )
    return new defensive_stance_t( this, options_str );
  if ( name == "demoralizing_shout" )
    return new demoralizing_shout_t( this, options_str );
  if ( name == "devastate" )
    return new devastate_t( this, options_str );
  if ( name == "die_by_the_sword" )
    return new die_by_the_sword_t( this, options_str );
  if ( name == "thunderous_roar" )
    return new thunderous_roar_t( this, options_str );
  if ( name == "enraged_regeneration" )
    return new enraged_regeneration_t( this, options_str );
  if ( name == "execute" )
  {
    if ( specialization() == WARRIOR_FURY )
    {
      return new execute_fury_t( this, options_str );
    }
    else
    {
      return new execute_arms_t( this, options_str );
    }
  }
  if ( name == "hamstring" )
    return new hamstring_t( this, options_str );
  if ( name == "heroic_charge" )
    return new heroic_charge_t( this, options_str );
  if ( name == "heroic_leap" )
    return new heroic_leap_t( this, options_str );
  if ( name == "heroic_throw" )
    return new heroic_throw_t( this, options_str );
  if ( name == "impending_victory" )
    return new impending_victory_t( this, options_str );
  if ( name == "intervene" )
    return new intervene_t( this, options_str );
  if ( name == "last_stand" )
    return new last_stand_t( this, options_str );
  if ( name == "mortal_strike" )
    return new mortal_strike_t( this, options_str );
  if ( name == "odyns_fury" )
    return new odyns_fury_t( this, options_str, name, talents.fury.odyns_fury );
  if ( name == "onslaught" )
    return new onslaught_t( this, options_str );
  if ( name == "overpower" )
    return new overpower_t( this, options_str );
  if ( name == "piercing_howl" )
    return new piercing_howl_t( this, options_str );
  if ( name == "pummel" )
    return new pummel_t( this, options_str );
  if ( name == "raging_blow" )
    return new raging_blow_t( this, options_str );
  if ( name == "crushing_blow" )
    return new crushing_blow_t( this, options_str );
  if ( name == "rallying_cry" )
    return new rallying_cry_t( this, options_str );
  if ( name == "rampage" )
    return new rampage_parent_t( this, options_str );
  if ( name == "ravager" )
    return new ravager_t( this, options_str );
  if ( name == "rend" )
  {
    if ( specialization() == WARRIOR_PROTECTION )
      return new rend_prot_t( this, options_str );
    else
      return new rend_t( this, options_str );
  }
  if ( name == "revenge" )
    return new revenge_t( this, options_str );
  if ( name == "shattering_throw" )
    return new shattering_throw_t( this, options_str );
  if ( name == "shield_block" )
    return new shield_block_t( this, options_str );
  if ( name == "shield_charge" )
    return new shield_charge_t( this, options_str );
  if ( name == "shield_slam" )
    return new shield_slam_t( this, options_str );
  if ( name == "shield_wall" )
    return new shield_wall_t( this, options_str );
  if ( name == "shockwave" )
    return new shockwave_t( this, options_str );
  if ( name == "skullsplitter" )
    return new skullsplitter_t( this, options_str );
  if ( name == "slam" )
    return new slam_t( this, options_str );
  if ( name == "champions_spear" )
    return new champions_spear_t( this, options_str );
  if ( name == "spell_reflection" )
    return new spell_reflection_t( this, options_str );
  if ( name == "storm_bolt" )
    return new storm_bolt_t( this, options_str );
  if ( name == "sweeping_strikes" )
    return new sweeping_strikes_t( this, options_str );
  if ( name == "taunt" )
    return new taunt_t( this, options_str );
  if ( name == "thunder_clap" )
    return new thunder_clap_t( this, options_str );
  if ( name == "thunder_blast" )
    return new thunder_blast_t( this, options_str );
  if ( name == "victory_rush" )
    return new victory_rush_t( this, options_str );
  if ( name == "warbreaker" )
    return new warbreaker_t( this, options_str );
  if ( name == "ignore_pain" )
    return new ignore_pain_t( this, options_str );
  if ( name == "whirlwind" )
  {
    if ( specialization() == WARRIOR_FURY )
    {
      return new fury_whirlwind_parent_t( this, options_str );
    }
    else if ( specialization() == WARRIOR_ARMS )
    {
      return new arms_whirlwind_parent_t( this, options_str );
    }
  }
  if ( name == "wrecking_throw" )
    return new wrecking_throw_t( this, options_str );
  if ( name == "demolish" )
    return new demolish_t( this, options_str );

  return parse_player_effects_t::create_action( name, options_str );
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  parse_player_effects_t::init_spells();

  // Core Class Spells
  spell.battle_shout            = find_class_spell( "Battle Shout" );
  spell.berserker_rage          = find_class_spell( "Berserker Rage" );
  spell.charge                  = find_class_spell( "Charge" );
  spell.execute                 = find_class_spell( "Execute" );
  spell.execute_rage_refund     = find_spell( 163201 );
  spell.hamstring               = find_class_spell( "Hamstring" );
  spell.heroic_throw            = find_class_spell( "Heroic Throw" );
  spell.pummel                  = find_class_spell( "Pummel" );
  spell.shield_block            = find_class_spell( "Shield Block" );
  spell.shield_slam             = find_class_spell( "Shield Slam" );
  spell.slam                    = find_class_spell( "Slam" );
  spell.taunt                   = find_class_spell( "Taunt" );
  spell.victory_rush            = find_class_spell( "Victory Rush" );
  spell.whirlwind               = find_class_spell( "Whirlwind" );
  spell.shield_block_buff       = find_spell( 132404 );
  spell.concussive_blows_debuff = find_spell( 383116 ); 
  spell.recklessness_buff       = find_spell( 1719 ); // lookup to allow Warlord to use Reck

  // Class Passives
  spec.warrior                  = find_spell( 137047 );
  spec.warrior_2                = find_spell( 462116 );

  // Arms Spells
  mastery.deep_wounds_ARMS      = find_mastery_spell( WARRIOR_ARMS );
  spec.arms_warrior             = find_specialization_spell( "Arms Warrior" );
  spec.arms_warrior_2           = find_spell( 462115 ); // Trinket aura
  spec.seasoned_soldier         = find_specialization_spell( "Seasoned Soldier" );
  spec.sweeping_strikes         = find_specialization_spell( "Sweeping Strikes" );
  spec.deep_wounds_ARMS         = find_specialization_spell("Mastery: Deep Wounds", WARRIOR_ARMS);
  spell.colossus_smash_debuff   = find_spell( 208086 );
  spell.dance_of_death          = find_spell( 390713 );
  spell.dance_of_death_bs_buff  = find_spell( 459572 );
  spell.deep_wounds_arms        = find_spell( 262115 );
  spell.fatal_mark_debuff       = find_spell( 383704 );
  spell.sudden_death_arms       = find_spell( 52437 );

  // Fury Spells
  mastery.unshackled_fury       = find_mastery_spell( WARRIOR_FURY );
  spec.fury_warrior             = find_specialization_spell( "Fury Warrior" );
  spec.fury_warrior_2           = find_spell( 462117 );  // Trinket aura
  spec.enrage                   = find_specialization_spell( "Enrage" );
  spec.execute                  = find_specialization_spell( "Execute" );
  spec.whirlwind                = find_specialization_spell( "Whirlwind" );
  spec.bloodbath                = find_spell(335096);
  spec.crushing_blow            = find_spell(335097);
  spell.whirlwind_buff          = find_spell( 85739, WARRIOR_FURY );  // Used to be called Meat Cleaver
  spell.sudden_death_fury       = find_spell( 280776 );

  spell.furious_bloodthirst     = find_spell( 423211 );
  spell.t31_fury_4pc            = find_spell( 422926 );

  // Protection Spells
  mastery.critical_block        = find_mastery_spell( WARRIOR_PROTECTION );
  spec.protection_warrior       = find_specialization_spell( "Protection Warrior" );
  spec.protection_warrior_2     = find_spell( 462119 );  // Trinket aura
  spec.devastate                = find_specialization_spell( "Devastate" );
  spec.riposte                  = find_specialization_spell( "Riposte" );
  spec.vanguard                 = find_specialization_spell( "Vanguard" );
  spec.deep_wounds_PROT         = find_specialization_spell("Deep Wounds", WARRIOR_PROTECTION);
  spec.revenge_trigger          = find_specialization_spell("Revenge Trigger");
  spec.shield_block_2           = find_specialization_spell( 231847 ); // extra charge
  spell.shield_wall             = find_spell( 871 );
  spell.seismic_reverberation_revenge = find_spell( 384730 );

  // Colossus Spells
  spell.wrecked_debuff              = find_spell( 447513 );

  // Slayer Spells
  spell.marked_for_execution_debuff = find_spell( 445584 );
  spell.slayers_strike              = find_spell( 445579 );
  spell.overwhelmed_debuff          = find_spell( 445836 );
  spell.reap_the_storm              = find_spell( 446005 );

  // Mountain Thane Spells
  spell.lightning_strike            = find_spell( 435791 );

  // Class Talents
  talents.warrior.battle_stance                    = find_talent_spell( talent_tree::CLASS, "Battle Stance" );
  talents.warrior.berserker_stance                 = find_talent_spell( talent_tree::CLASS, "Berserker Stance" );
  talents.warrior.defensive_stance                 = find_talent_spell( talent_tree::CLASS, "Defensive Stance" );

  talents.warrior.second_wind                      = find_talent_spell( talent_tree::CLASS, "Second Wind" );
  talents.warrior.war_machine                      = find_talent_spell( talent_tree::CLASS, "War Machine", specialization() );
  talents.warrior.fast_footwork                    = find_talent_spell( talent_tree::CLASS, "Fast Footwork" );
  talents.warrior.leeching_strikes                 = find_talent_spell( talent_tree::CLASS, "Leeching Strikes" );

  talents.warrior.impending_victory                = find_talent_spell( talent_tree::CLASS, "Impending Victory" );
  talents.warrior.heroic_leap                      = find_talent_spell( talent_tree::CLASS, "Heroic Leap" );
  talents.warrior.storm_bolt                       = find_talent_spell( talent_tree::CLASS, "Storm Bolt" );
  talents.warrior.intervene                        = find_talent_spell( talent_tree::CLASS, "Intervene" );

  talents.warrior.intimidating_shout               = find_talent_spell( talent_tree::CLASS, "Intimidating Shout" );
  talents.warrior.frothing_berserker               = find_talent_spell( talent_tree::CLASS, "Frothing Berserker", specialization() );
  talents.warrior.bounding_stride                  = find_talent_spell( talent_tree::CLASS, "Bounding Stride" );
  talents.warrior.pain_and_gain                    = find_talent_spell( talent_tree::CLASS, "Pain and Gain" );
  talents.warrior.thunder_clap                     = find_talent_spell( talent_tree::CLASS, "Thunder Clap", specialization() );

  talents.warrior.cacophonous_roar                 = find_talent_spell( talent_tree::CLASS, "Cacophonous Roar" );
  talents.warrior.menace                           = find_talent_spell( talent_tree::CLASS, "Menace" );
  talents.warrior.spell_reflection                 = find_talent_spell( talent_tree::CLASS, "Spell Reflection" );
  talents.warrior.rallying_cry                     = find_talent_spell( talent_tree::CLASS, "Rallying Cry" );
  talents.warrior.shockwave                        = find_talent_spell( talent_tree::CLASS, "Shockwave" );
  talents.warrior.crackling_thunder                = find_talent_spell( talent_tree::CLASS, "Crackling Thunder" );

  talents.warrior.honed_reflexes                   = find_talent_spell( talent_tree::CLASS, "Honed Reflexes" );
  talents.warrior.crushing_force                   = find_talent_spell( talent_tree::CLASS, "Crushing Force", specialization() );
  talents.warrior.bitter_immunity                  = find_talent_spell( talent_tree::CLASS, "Bitter Immunity" );
  talents.warrior.overwhelming_rage                = find_talent_spell( talent_tree::CLASS, "Overwhelming Rage" );
  talents.warrior.rumbling_earth                   = find_talent_spell( talent_tree::CLASS, "Rumbling Earth" );
  talents.warrior.reinforced_plates                = find_talent_spell( talent_tree::CLASS, "Reinforced Plates" );

  talents.warrior.wrecking_throw                   = find_talent_spell( talent_tree::CLASS, "Wrecking Throw" );
  talents.warrior.shattering_throw                 = find_talent_spell( talent_tree::CLASS, "Shattering Throw" );
  talents.warrior.barbaric_training                = find_talent_spell( talent_tree::CLASS, "Barbaric Training" );
  talents.warrior.sidearm                          = find_talent_spell( talent_tree::CLASS, "Sidearm" );
  talents.warrior.double_time                      = find_talent_spell( talent_tree::CLASS, "Double Time" );
  talents.warrior.seismic_reverberation            = find_talent_spell( talent_tree::CLASS, "Seismic Reverberation" );
  talents.warrior.concussive_blows                 = find_talent_spell( talent_tree::CLASS, "Concussive Blows" );
  talents.warrior.berserker_shout                  = find_talent_spell( talent_tree::CLASS, "Berserker Shout" );
  talents.warrior.piercing_howl                    = find_talent_spell( talent_tree::CLASS, "Piercing Howl" );

  talents.warrior.cruel_strikes                    = find_talent_spell( talent_tree::CLASS, "Cruel Strikes" );
  talents.warrior.wild_strikes                     = find_talent_spell( talent_tree::CLASS, "Wild Strikes" );
  talents.warrior.two_handed_weapon_specialization = find_talent_spell( talent_tree::CLASS, "Two-Handed Weapon Specialization" );
  talents.warrior.dual_wield_specialization        = find_talent_spell( talent_tree::CLASS, "Dual Wield Specialization" );
  talents.warrior.one_handed_weapon_specialization = find_talent_spell( talent_tree::CLASS, "One-Handed Weapon Specialization" );
  talents.warrior.endurance_training               = find_talent_spell( talent_tree::CLASS, "Endurance Training", specialization() );
  talents.warrior.armored_to_the_teeth             = find_talent_spell( talent_tree::CLASS, "Armored to the Teeth" );

  talents.warrior.thunderous_roar                  = find_talent_spell( talent_tree::CLASS, "Thunderous Roar" );
  talents.warrior.avatar                           = find_talent_spell( talent_tree::CLASS, "Avatar" );
  talents.warrior.champions_spear                  = find_talent_spell( talent_tree::CLASS, "Champion's Spear" );

  talents.warrior.uproar                           = find_talent_spell( talent_tree::CLASS, "Uproar" );
  talents.warrior.thunderous_words                 = find_talent_spell( talent_tree::CLASS, "Thunderous Words" );
  talents.warrior.blademasters_torment             = find_talent_spell( talent_tree::CLASS, "Blademaster's Torment" );
  talents.warrior.warlords_torment                 = find_talent_spell( talent_tree::CLASS, "Warlord's Torment" );
  talents.warrior.berserkers_torment               = find_talent_spell( talent_tree::CLASS, "Berserker's Torment" );
  talents.warrior.titans_torment                   = find_talent_spell( talent_tree::CLASS, "Titan's Torment" );
  talents.warrior.immovable_object                 = find_talent_spell( talent_tree::CLASS, "Immovable Object" );
  talents.warrior.unstoppable_force                = find_talent_spell( talent_tree::CLASS, "Unstoppable Force" );
  talents.warrior.piercing_challenge                = find_talent_spell( talent_tree::CLASS, "Piercing Challenge" );
  talents.warrior.champions_might                  = find_talent_spell( talent_tree::CLASS, "Champion's Might" );

  // Arms Talents
  talents.arms.mortal_strike                       = find_talent_spell( talent_tree::SPECIALIZATION, "Mortal Strike" );

  talents.arms.overpower                           = find_talent_spell( talent_tree::SPECIALIZATION, "Overpower" );

  talents.arms.martial_prowess                     = find_talent_spell( talent_tree::SPECIALIZATION, "Martial Prowess" );
  talents.arms.die_by_the_sword                    = find_talent_spell( talent_tree::SPECIALIZATION, "Die by the Sword" );
  talents.arms.improved_execute                    = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Execute", WARRIOR_ARMS );

  talents.arms.improved_overpower                  = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Overpower" );
  talents.arms.bloodsurge                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodsurge", WARRIOR_ARMS );
  talents.arms.fueled_by_violence                  = find_talent_spell( talent_tree::SPECIALIZATION, "Fueled by Violence", WARRIOR_ARMS );
  talents.arms.storm_wall                          = find_talent_spell( talent_tree::SPECIALIZATION, "Storm Wall" );
  talents.arms.ignore_pain                         = find_talent_spell( talent_tree::SPECIALIZATION, "Ignore Pain", WARRIOR_ARMS );
  talents.arms.sudden_death                        = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Death", WARRIOR_ARMS );
  talents.arms.fervor_of_battle                    = find_talent_spell( talent_tree::SPECIALIZATION, "Fervor of Battle" );

  talents.arms.tactician                           = find_talent_spell( talent_tree::SPECIALIZATION, "Tactician" );
  talents.arms.colossus_smash                      = find_talent_spell( talent_tree::SPECIALIZATION, "Colossus Smash" );
  talents.arms.impale                              = find_talent_spell( talent_tree::SPECIALIZATION, "Impale" );

  talents.arms.skullsplitter                       = find_talent_spell( talent_tree::SPECIALIZATION, "Skullsplitter" );
  talents.arms.rend                                = find_talent_spell( talent_tree::SPECIALIZATION, "Rend", WARRIOR_ARMS );
  talents.arms.finishing_blows                     = find_talent_spell( talent_tree::SPECIALIZATION, "Finishing Blows" );
  talents.arms.anger_management                    = find_talent_spell( talent_tree::SPECIALIZATION, "Anger Management" );
  talents.arms.spiteful_serenity                   = find_talent_spell( talent_tree::SPECIALIZATION, "Spiteful Serenity", WARRIOR_ARMS );
  talents.arms.exhilarating_blows                  = find_talent_spell( talent_tree::SPECIALIZATION, "Exhilarating Blows" );
  talents.arms.improved_sweeping_strikes           = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Sweeping Strikes", WARRIOR_ARMS );
  talents.arms.collateral_damage                   = find_talent_spell( talent_tree::SPECIALIZATION, "Collateral Damage" );
  talents.arms.cleave                              = find_talent_spell( talent_tree::SPECIALIZATION, "Cleave" );

  talents.arms.bloodborne                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodborne", WARRIOR_ARMS );
  talents.arms.dreadnaught                         = find_talent_spell( talent_tree::SPECIALIZATION, "Dreadnaught" );
  talents.arms.strength_of_arms                    = find_talent_spell( talent_tree::SPECIALIZATION, "Strength of Arms", WARRIOR_ARMS );
  talents.arms.in_for_the_kill                     = find_talent_spell( talent_tree::SPECIALIZATION, "In for the Kill" );
  talents.arms.test_of_might                       = find_talent_spell( talent_tree::SPECIALIZATION, "Test of Might" );
  talents.arms.blunt_instruments                   = find_talent_spell( talent_tree::SPECIALIZATION, "Blunt Instruments" );
  talents.arms.warbreaker                          = find_talent_spell( talent_tree::SPECIALIZATION, "Warbreaker" );
  talents.arms.massacre                            = find_talent_spell( talent_tree::SPECIALIZATION, "Massacre", WARRIOR_ARMS );
  talents.arms.storm_of_swords                     = find_talent_spell( talent_tree::SPECIALIZATION, "Storm of Swords", WARRIOR_ARMS );

  talents.arms.deft_experience                     = find_talent_spell( talent_tree::SPECIALIZATION, "Deft Experience", WARRIOR_ARMS );
  talents.arms.valor_in_victory                    = find_talent_spell( talent_tree::SPECIALIZATION, "Valor in Victory" );
  talents.arms.critical_thinking                   = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Thinking", WARRIOR_ARMS );

  talents.arms.battlelord                          = find_talent_spell( talent_tree::SPECIALIZATION, "Battlelord" );
  talents.arms.bloodletting                        = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodletting" );
  talents.arms.bladestorm                          = find_talent_spell( talent_tree::SPECIALIZATION, "Bladestorm", WARRIOR_ARMS );
  talents.arms.ravager                             = find_talent_spell( talent_tree::SPECIALIZATION, "Ravager", WARRIOR_ARMS );
  talents.arms.sharpened_blades                    = find_talent_spell( talent_tree::SPECIALIZATION, "Sharpened Blades" );
  talents.arms.juggernaut                          = find_talent_spell( talent_tree::SPECIALIZATION, "Juggernaut", WARRIOR_ARMS );

  talents.arms.fatality                            = find_talent_spell( talent_tree::SPECIALIZATION, "Fatality" );
  talents.arms.dance_of_death                      = find_talent_spell( talent_tree::SPECIALIZATION, "Dance of Death", WARRIOR_ARMS );
  talents.arms.unhinged                            = find_talent_spell( talent_tree::SPECIALIZATION, "Unhinged", WARRIOR_ARMS );
  talents.arms.merciless_bonegrinder               = find_talent_spell( talent_tree::SPECIALIZATION, "Merciless Bonegrinder" );
  talents.arms.executioners_precision              = find_talent_spell( talent_tree::SPECIALIZATION, "Executioner's Precision" );

  // Fury Talents
  talents.fury.bloodthirst          = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodthirst" );

  talents.fury.raging_blow          = find_talent_spell( talent_tree::SPECIALIZATION, "Raging Blow" );

  talents.fury.frenzied_enrage      = find_talent_spell( talent_tree::SPECIALIZATION, "Frenzied Enrage" );
  talents.fury.powerful_enrage      = find_talent_spell( talent_tree::SPECIALIZATION, "Powerful Enrage" );
  talents.fury.enraged_regeneration = find_talent_spell( talent_tree::SPECIALIZATION, "Enraged Regeneration" );
  talents.fury.improved_execute     = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Execute", WARRIOR_FURY );

  talents.fury.improved_bloodthirst = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Bloodthirst" );
  talents.fury.fresh_meat           = find_talent_spell( talent_tree::SPECIALIZATION, "Fresh Meat" );
  talents.fury.warpaint             = find_talent_spell( talent_tree::SPECIALIZATION, "Warpaint" );
  talents.fury.invigorating_fury    = find_talent_spell( talent_tree::SPECIALIZATION, "Invigorating Fury" );
  talents.fury.sudden_death         = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Death", WARRIOR_FURY );
  talents.fury.cruelty              = find_talent_spell( talent_tree::SPECIALIZATION, "Cruelty" );

  talents.fury.focus_in_chaos       = find_talent_spell( talent_tree::SPECIALIZATION, "Focus in Chaos" );
  talents.fury.rampage              = find_talent_spell( talent_tree::SPECIALIZATION, "Rampage" );
  talents.fury.improved_raging_blow = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Raging Blow" );

  talents.fury.single_minded_fury   = find_talent_spell( talent_tree::SPECIALIZATION, "Single-Minded Fury" );
  talents.fury.cold_steel_hot_blood = find_talent_spell( talent_tree::SPECIALIZATION, "Cold Steel, Hot Blood" );
  talents.fury.vicious_contempt     = find_talent_spell( talent_tree::SPECIALIZATION, "Vicious Contempt" );
  talents.fury.frenzy               = find_talent_spell( talent_tree::SPECIALIZATION, "Frenzy" );
  talents.fury.hack_and_slash       = find_talent_spell( talent_tree::SPECIALIZATION, "Hack and Slash" );
  talents.fury.slaughtering_strikes = find_talent_spell( talent_tree::SPECIALIZATION, "Slaughtering Strikes" );
  talents.fury.ashen_juggernaut     = find_talent_spell( talent_tree::SPECIALIZATION, "Ashen Juggernaut" );
  talents.fury.improved_whirlwind   = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Whirlwind" );

  talents.fury.bloodborne           = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodborne", WARRIOR_FURY );
  talents.fury.bloodcraze           = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodcraze" );
  talents.fury.recklessness         = find_talent_spell( talent_tree::SPECIALIZATION, "Recklessness" );
  talents.fury.massacre             = find_talent_spell( talent_tree::SPECIALIZATION, "Massacre", WARRIOR_FURY );
  talents.fury.wrath_and_fury       = find_talent_spell( talent_tree::SPECIALIZATION, "Wrath and Fury" );
  talents.fury.meat_cleaver         = find_talent_spell( talent_tree::SPECIALIZATION, "Meat Cleaver" );

  talents.fury.deft_experience      = find_talent_spell( talent_tree::SPECIALIZATION, "Deft Experience", WARRIOR_FURY );
  talents.fury.swift_strikes        = find_talent_spell( talent_tree::SPECIALIZATION, "Swift Strikes" );
  talents.fury.critical_thinking    = find_talent_spell( talent_tree::SPECIALIZATION, "Critical Thinking", WARRIOR_FURY );

  talents.fury.odyns_fury           = find_talent_spell( talent_tree::SPECIALIZATION, "Odyn's Fury" );
  talents.fury.anger_management     = find_talent_spell( talent_tree::SPECIALIZATION, "Anger Management" );
  talents.fury.reckless_abandon     = find_talent_spell( talent_tree::SPECIALIZATION, "Reckless Abandon" );
  talents.fury.onslaught            = find_talent_spell( talent_tree::SPECIALIZATION, "Onslaught" );
  talents.fury.ravager              = find_talent_spell( talent_tree::SPECIALIZATION, "Ravager", WARRIOR_FURY );
  talents.fury.bladestorm           = find_talent_spell( talent_tree::SPECIALIZATION, "Bladestorm", WARRIOR_FURY );

  talents.fury.dancing_blades       = find_talent_spell( talent_tree::SPECIALIZATION, "Dancing Blades" );
  talents.fury.titanic_rage         = find_talent_spell( talent_tree::SPECIALIZATION, "Titanic Rage" );
  talents.fury.unbridled_ferocity   = find_talent_spell( talent_tree::SPECIALIZATION, "Unbridled Ferocity" );
  talents.fury.depths_of_insanity   = find_talent_spell( talent_tree::SPECIALIZATION, "Depths of Insanity" );
  talents.fury.tenderize            = find_talent_spell( talent_tree::SPECIALIZATION, "Tenderize" );
  talents.fury.storm_of_steel       = find_talent_spell( talent_tree::SPECIALIZATION, "Storm of Steel", WARRIOR_FURY );
  talents.fury.unhinged             = find_talent_spell( talent_tree::SPECIALIZATION, "Unhinged", WARRIOR_FURY );

  // Protection Talents
  talents.protection.ignore_pain            = find_talent_spell( talent_tree::SPECIALIZATION, "Ignore Pain" );

  talents.protection.revenge                = find_talent_spell( talent_tree::SPECIALIZATION, "Revenge" );

  talents.protection.demoralizing_shout     = find_talent_spell( talent_tree::SPECIALIZATION, "Demoralizing Shout" );
  talents.protection.devastator             = find_talent_spell( talent_tree::SPECIALIZATION, "Devastator" );
  talents.protection.last_stand             = find_talent_spell( talent_tree::SPECIALIZATION, "Last Stand" );

  talents.protection.fight_through_the_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Fight Through the Flames" );
  talents.protection.best_served_cold       = find_talent_spell( talent_tree::SPECIALIZATION, "Best Served Cold" );
  talents.protection.strategist             = find_talent_spell( talent_tree::SPECIALIZATION, "Strategist" );
  talents.protection.brace_for_impact       = find_talent_spell( talent_tree::SPECIALIZATION, "Brace for Impact" );
  talents.protection.unnerving_focus        = find_talent_spell( talent_tree::SPECIALIZATION, "Unnerving Focus" );

  talents.protection.challenging_shout      = find_talent_spell( talent_tree::SPECIALIZATION, "Challenging Shout" );
  talents.protection.instigate              = find_talent_spell( talent_tree::SPECIALIZATION, "Instigate" );
  talents.protection.rend                   = find_talent_spell( talent_tree::SPECIALIZATION, "Rend", WARRIOR_PROTECTION );
  talents.protection.bloodsurge             = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodsurge", WARRIOR_PROTECTION );
  talents.protection.fueled_by_violence     = find_talent_spell( talent_tree::SPECIALIZATION, "Fueled by Violence", WARRIOR_PROTECTION );
  talents.protection.brutal_vitality        = find_talent_spell( talent_tree::SPECIALIZATION, "Brutal Vitality" ); // NYI

  talents.protection.disrupting_shout       = find_talent_spell( talent_tree::SPECIALIZATION, "Disrupting Shout" );
  talents.protection.show_of_force          = find_talent_spell( talent_tree::SPECIALIZATION, "Show of Force" );
  talents.protection.sudden_death           = find_talent_spell( talent_tree::SPECIALIZATION, "Sudden Death", WARRIOR_PROTECTION );
  talents.protection.thunderlord            = find_talent_spell( talent_tree::SPECIALIZATION, "Thunderlord" );
  talents.protection.shield_wall            = find_talent_spell( talent_tree::SPECIALIZATION, "Shield Wall" );
  talents.protection.bolster                = find_talent_spell( talent_tree::SPECIALIZATION, "Bolster" );
  talents.protection.tough_as_nails         = find_talent_spell( talent_tree::SPECIALIZATION, "Tough as Nails" );
  talents.protection.spell_block            = find_talent_spell( talent_tree::SPECIALIZATION, "Spell Block" );
  talents.protection.bloodborne             = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodborne", WARRIOR_PROTECTION );

  talents.protection.heavy_repercussions    = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Repercussions" );
  talents.protection.into_the_fray          = find_talent_spell( talent_tree::SPECIALIZATION, "Into the Fray" );
  talents.protection.enduring_defenses      = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Defenses" );
  talents.protection.massacre               = find_talent_spell( talent_tree::SPECIALIZATION, "Massacre", WARRIOR_PROTECTION );
  talents.protection.anger_management       = find_talent_spell( talent_tree::SPECIALIZATION, "Anger Management" );
  talents.protection.defenders_aegis        = find_talent_spell( talent_tree::SPECIALIZATION, "Defender's Aegis" );
  talents.protection.impenetrable_wall      = find_talent_spell( talent_tree::SPECIALIZATION, "Impenetrable Wall" );
  talents.protection.punish                 = find_talent_spell( talent_tree::SPECIALIZATION, "Punish" );
  talents.protection.juggernaut             = find_talent_spell( talent_tree::SPECIALIZATION, "Juggernaut", WARRIOR_PROTECTION );

  talents.protection.focused_vigor          = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Vigor" );
  talents.protection.shield_specialization  = find_talent_spell( talent_tree::SPECIALIZATION, "Shield Specialization" );
  talents.protection.enduring_alacrity      = find_talent_spell( talent_tree::SPECIALIZATION, "Enduring Alacrity" );

  talents.protection.shield_charge          = find_talent_spell( talent_tree::SPECIALIZATION, "Shield Charge" );
  talents.protection.booming_voice          = find_talent_spell( talent_tree::SPECIALIZATION, "Booming Voice" );
  talents.protection.indomitable            = find_talent_spell( talent_tree::SPECIALIZATION, "Indomitable" );
  talents.protection.violent_outburst       = find_talent_spell( talent_tree::SPECIALIZATION, "Violent Outburst" );
  talents.protection.ravager                = find_talent_spell( talent_tree::SPECIALIZATION, "Ravager", WARRIOR_PROTECTION );

  talents.protection.battering_ram          = find_talent_spell( talent_tree::SPECIALIZATION, "Battering Ram" );
  talents.protection.champions_bulwark      = find_talent_spell( talent_tree::SPECIALIZATION, "Champion's Bulwark" );
  talents.protection.battle_scarred_veteran = find_talent_spell( talent_tree::SPECIALIZATION, "Battle-Scarred Veteran" );
  talents.protection.dance_of_death         = find_talent_spell( talent_tree::SPECIALIZATION, "Dance of Death", WARRIOR_PROTECTION );
  talents.protection.storm_of_steel         = find_talent_spell( talent_tree::SPECIALIZATION, "Storm of Steel", WARRIOR_PROTECTION );

  // Colossus Hero Talents
  talents.colossus.demolish                     = find_talent_spell( talent_tree::HERO, "Demolish" );
  talents.colossus.martial_expert               = find_talent_spell( talent_tree::HERO, "Martial Expert" );
  talents.colossus.colossal_might               = find_talent_spell( talent_tree::HERO, "Colossal Might" );
  talents.colossus.boneshaker                   = find_talent_spell( talent_tree::HERO, "Boneshaker" );
  talents.colossus.earthquaker                  = find_talent_spell( talent_tree::HERO, "Earthquaker" );
  talents.colossus.one_against_many             = find_talent_spell( talent_tree::HERO, "One Against Many" );
  talents.colossus.arterial_bleed               = find_talent_spell( talent_tree::HERO, "Arterial Bleed" );
  talents.colossus.tide_of_battle               = find_talent_spell( talent_tree::HERO, "Tide of Battle" );
  talents.colossus.no_stranger_to_pain          = find_talent_spell( talent_tree::HERO, "No Stranger to Pain" );
  talents.colossus.veteran_vitality             = find_talent_spell( talent_tree::HERO, "Veteran Vitality" );
  talents.colossus.practiced_strikes            = find_talent_spell( talent_tree::HERO, "Practiced Strikes" );
  talents.colossus.precise_might                = find_talent_spell( talent_tree::HERO, "Precise Might" );
  talents.colossus.mountain_of_muscle_and_scars = find_talent_spell( talent_tree::HERO, "Mountain of Muscle and Scars" );
  talents.colossus.dominance_of_the_colossus    = find_talent_spell( talent_tree::HERO, "Dominance of the Colossus" );

  // Slayer Hero Talents
  talents.slayer.slayers_dominance     = find_talent_spell( talent_tree::HERO, "Slayer's Dominance" );
  talents.slayer.imminent_demise       = find_talent_spell( talent_tree::HERO, "Imminent Demise" );
  talents.slayer.overwhelming_blades   = find_talent_spell( talent_tree::HERO, "Overwhelming Blades" );
  talents.slayer.relentless_pursuit    = find_talent_spell( talent_tree::HERO, "Relentless Pursuit" );
  talents.slayer.vicious_agility       = find_talent_spell( talent_tree::HERO, "Vicious Agility" );
  talents.slayer.death_drive           = find_talent_spell( talent_tree::HERO, "Death Drive" );
  talents.slayer.culling_cyclone       = find_talent_spell( talent_tree::HERO, "Culling Cyclone" );
  talents.slayer.brutal_finish         = find_talent_spell( talent_tree::HERO, "Brutal Finish" );
  talents.slayer.fierce_followthrough  = find_talent_spell( talent_tree::HERO, "Fierce Followthrough" );
  talents.slayer.opportunist           = find_talent_spell( talent_tree::HERO, "Opportunist" );
  talents.slayer.show_no_mercy         = find_talent_spell( talent_tree::HERO, "Show No Mercy" );
  talents.slayer.reap_the_storm        = find_talent_spell( talent_tree::HERO, "Reap the Storm" );
  talents.slayer.slayers_malice        = find_talent_spell( talent_tree::HERO, "Slayer's Malice" );
  talents.slayer.unrelenting_onslaught = find_talent_spell( talent_tree::HERO, "Unrelenting Onslaught" );

  // Mountain Thane Hero Talents
  talents.mountain_thane.lightning_strikes            = find_talent_spell( talent_tree::HERO, "Lightning Strikes" );
  talents.mountain_thane.crashing_thunder             = find_talent_spell( talent_tree::HERO, "Crashing Thunder" );
  talents.mountain_thane.ground_current               = find_talent_spell( talent_tree::HERO, "Ground Current" );
  talents.mountain_thane.strength_of_the_mountain     = find_talent_spell( talent_tree::HERO, "Strength of the Mountain" );
  talents.mountain_thane.thunder_blast                = find_talent_spell( talent_tree::HERO, "Thunder Blast" );
  talents.mountain_thane.storm_bolts                  = find_talent_spell( talent_tree::HERO, "Storm Bolts" );
  talents.mountain_thane.storm_shield                 = find_talent_spell( talent_tree::HERO, "Storm Shield" );
  talents.mountain_thane.keep_your_feet_on_the_ground = find_talent_spell( talent_tree::HERO, "Keep Your Feet on the Ground" );
  talents.mountain_thane.steadfast_as_the_peaks       = find_talent_spell( talent_tree::HERO, "Steadfast as the Peaks" );
  talents.mountain_thane.flashing_skies               = find_talent_spell( talent_tree::HERO, "Flashing Skies" );
  talents.mountain_thane.snap_induction               = find_talent_spell( talent_tree::HERO, "Snap Induction" );
  talents.mountain_thane.gathering_clouds             = find_talent_spell( talent_tree::HERO, "Gathering Clouds" );
  talents.mountain_thane.thorims_might                = find_talent_spell( talent_tree::HERO, "Thorim's Might" );
  talents.mountain_thane.burst_of_power               = find_talent_spell( talent_tree::HERO, "Burst of Power" );
  talents.mountain_thane.avatar_of_the_storm          = find_talent_spell( talent_tree::HERO, "Avatar of the Storm" );

  // Shared Talents - needed when using the same spell data with a spec check (ravager)

  auto find_shared_talent = []( std::vector<player_talent_t*> talents ) {
    for ( const auto t : talents )
    {
      if ( t->ok() )
      {
        return *t;
      }
    }
    return *talents[ 0 ];
  };

  talents.shared.ravager = find_shared_talent( { &talents.arms.ravager, &talents.fury.ravager, &talents.protection.ravager } );
  talents.shared.rend = find_shared_talent( { &talents.arms.rend, &talents.protection.rend } );
  talents.shared.bloodsurge = find_shared_talent( { &talents.arms.bloodsurge, &talents.protection.bloodsurge } );
  talents.shared.dance_of_death = find_shared_talent( { &talents.arms.dance_of_death, &talents.protection.dance_of_death } );

  // Convenant Abilities
  covenant.conquerors_banner     = find_covenant_spell( "Conqueror's Banner" );

  // Tier Sets
  tier_set.t29_arms_2pc               = sets->set( WARRIOR_ARMS, T29, B2 );
  tier_set.t29_arms_4pc               = sets->set( WARRIOR_ARMS, T29, B4 );
  tier_set.t29_fury_2pc               = sets->set( WARRIOR_FURY, T29, B2 );
  tier_set.t29_fury_4pc               = sets->set( WARRIOR_FURY, T29, B4 );
  tier_set.t29_prot_2pc               = sets->set( WARRIOR_PROTECTION, T29, B2 );
  tier_set.t29_prot_4pc               = sets->set( WARRIOR_PROTECTION, T29, B4 );
  tier_set.t30_arms_2pc               = sets->set( WARRIOR_ARMS, T30, B2 );
  tier_set.t30_arms_4pc               = sets->set( WARRIOR_ARMS, T30, B4 );
  tier_set.t30_fury_2pc               = sets->set( WARRIOR_FURY, T30, B2 );
  tier_set.t30_fury_4pc               = sets->set( WARRIOR_FURY, T30, B4 );
  tier_set.t30_prot_2pc               = sets->set( WARRIOR_PROTECTION, T30, B2 );
  tier_set.t30_prot_4pc               = sets->set( WARRIOR_PROTECTION, T30, B4 );
  tier_set.t31_arms_2pc               = sets->set( WARRIOR_ARMS, T31, B2 );
  tier_set.t31_arms_4pc               = sets->set( WARRIOR_ARMS, T31, B4 );
  tier_set.t31_fury_2pc               = sets->set( WARRIOR_FURY, T31, B2 );
  tier_set.t31_fury_4pc               = sets->set( WARRIOR_FURY, T31, B4 );

  // Active spells
  active.deep_wounds_ARMS = nullptr;
  active.deep_wounds_PROT = nullptr;
  active.fatality         = nullptr;
  active.slayers_strike   = nullptr;

  // AA Mods Not Handled by affecting_aura
  if ( specialization() == WARRIOR_FURY )
  {
  auto_attack_multiplier *= 1.0 + spec.fury_warrior->effectN( 4 ).percent();
  }
  if ( specialization() == WARRIOR_ARMS )
  {
    auto_attack_multiplier *= 1.0 + spec.arms_warrior->effectN( 6 ).percent();
  }
  if ( specialization() == WARRIOR_FURY && main_hand_weapon.group() == WEAPON_1H &&
             off_hand_weapon.group() == WEAPON_1H && talents.fury.single_minded_fury->ok() )
  {
    auto_attack_multiplier *= 1.0 + talents.fury.single_minded_fury->effectN( 4 ).percent();  // TODO double check this in game
    auto_attack_multiplier *= 1.0 + talents.fury.single_minded_fury->effectN( 5 ).percent();
  }
  if ( specialization() == WARRIOR_FURY && talents.warrior.dual_wield_specialization->ok() )
  {
  auto_attack_multiplier *= 1.0 + talents.warrior.dual_wield_specialization->effectN( 3 ).percent();
  }
  if ( specialization() == WARRIOR_ARMS && talents.warrior.two_handed_weapon_specialization->ok() ) 
  {
    auto_attack_multiplier *= 1.0 + talents.warrior.two_handed_weapon_specialization->effectN( 3 ).percent();
  }
  if ( specialization() == WARRIOR_PROTECTION && talents.warrior.one_handed_weapon_specialization->ok() )
  {
    auto_attack_multiplier *= 1.0 + talents.warrior.one_handed_weapon_specialization->effectN( 3 ).percent();
  }

  // Cooldowns
  cooldown.avatar         = get_cooldown( "avatar" );
  cooldown.recklessness   = get_cooldown( "recklessness" );
  cooldown.berserker_rage = get_cooldown( "berserker_rage" );
  cooldown.bladestorm     = get_cooldown( "bladestorm" );
  cooldown.bloodthirst    = get_cooldown( "bloodthirst" );
  cooldown.bloodbath      = get_cooldown( "bloodbath" );

  cooldown.cleave                           = get_cooldown( "cleave" );
  cooldown.colossus_smash                   = get_cooldown( "colossus_smash" );
  cooldown.conquerors_banner                = get_cooldown( "conquerors_banner" );
  cooldown.demoralizing_shout               = get_cooldown( "demoralizing_shout" );
  cooldown.thunderous_roar                  = get_cooldown( "thunderous_roar" );
  cooldown.enraged_regeneration             = get_cooldown( "enraged_regeneration" );
  cooldown.execute                          = get_cooldown( "execute" );
  cooldown.heroic_leap                      = get_cooldown( "heroic_leap" );
  cooldown.last_stand                       = get_cooldown( "last_stand" );
  cooldown.mortal_strike                    = get_cooldown( "mortal_strike" );
  cooldown.odyns_fury                       = get_cooldown( "odyns_fury" );
  cooldown.onslaught                        = get_cooldown( "onslaught" );
  cooldown.overpower                        = get_cooldown( "overpower" );
  cooldown.pummel                           = get_cooldown( "pummel" );
  cooldown.rage_from_auto_attack            = get_cooldown( "rage_from_auto_attack" );
  cooldown.rage_from_auto_attack->duration  = timespan_t::from_seconds ( 1.0 );
  cooldown.rage_from_crit_block             = get_cooldown( "rage_from_crit_block" );
  cooldown.rage_from_crit_block->duration   = timespan_t::from_seconds( 3.0 );
  cooldown.raging_blow                      = get_cooldown( "raging_blow" );
  cooldown.crushing_blow                    = get_cooldown( "raging_blow" );
  cooldown.ravager                          = get_cooldown( "ravager" );
  cooldown.shield_slam                      = get_cooldown( "shield_slam" );
  cooldown.shield_wall                      = get_cooldown( "shield_wall" );
  cooldown.single_minded_fury_icd           = get_cooldown( "single_minded_fury" );
  cooldown.single_minded_fury_icd->duration = talents.fury.single_minded_fury->internal_cooldown();
  cooldown.skullsplitter                    = get_cooldown( "skullsplitter" );
  cooldown.shockwave                        = get_cooldown( "shockwave" );
  cooldown.storm_bolt                       = get_cooldown( "storm_bolt" );
  cooldown.sudden_death_icd                 = get_cooldown( "sudden_death" );
  cooldown.sudden_death_icd->duration =
      specialization() == WARRIOR_FURY   ? talents.fury.sudden_death->internal_cooldown(): 
      specialization() == WARRIOR_ARMS ? talents.arms.sudden_death->internal_cooldown() : 
      talents.protection.sudden_death->internal_cooldown();
  cooldown.sudden_death_icd->duration       = talents.arms.sudden_death->internal_cooldown();
  cooldown.tough_as_nails_icd               = get_cooldown( "tough_as_nails" );
  cooldown.tough_as_nails_icd -> duration   = talents.protection.tough_as_nails->effectN( 1 ).trigger() -> internal_cooldown();
  cooldown.thunder_clap                     = get_cooldown( "thunder_clap" );
  cooldown.warbreaker                       = get_cooldown( "warbreaker" );
  cooldown.cold_steel_hot_blood_icd         = get_cooldown( "cold_steel_hot_blood" );
  cooldown.cold_steel_hot_blood_icd -> duration = talents.fury.cold_steel_hot_blood->effectN( 2 ).trigger() -> internal_cooldown();
  cooldown.t31_fury_4pc_icd                 = get_cooldown( "t31_fury_4pc_icd" );
  cooldown.t31_fury_4pc_icd->duration = find_spell( 422926 )->internal_cooldown();
  cooldown.reap_the_storm_icd               = get_cooldown( "reap_the_storm" );
  cooldown.reap_the_storm_icd -> duration   = talents.slayer.reap_the_storm->internal_cooldown();
  cooldown.demolish                         = get_cooldown( "demolish" );
  cooldown.burst_of_power_icd               = get_cooldown( "burst_of_power" );
  cooldown.burst_of_power_icd -> duration = find_spell( 437121 )->internal_cooldown();
}

// warrior_t::init_items ===============================================

void warrior_t::init_items()
{
  parse_player_effects_t::init_items();

  set_bonus_type_e tier_to_enable;
  switch ( specialization() )
  {
    case WARRIOR_ARMS:
      tier_to_enable = T29;
      break;
    case WARRIOR_FURY:
      tier_to_enable = T30;
      break;
    case WARRIOR_PROTECTION:
      tier_to_enable = T31;
      break;
    default:
      return;
  }

  if ( sets->has_set_bonus( specialization(), DF4, B2 ) )
    sets->enable_set_bonus( specialization(), tier_to_enable, B2 );

  if ( sets->has_set_bonus( specialization(), DF4, B4 ) )
    sets->enable_set_bonus( specialization(), tier_to_enable, B4 );
}


// warrior_t::init_base =====================================================

void warrior_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5.0;

  parse_player_effects_t::init_base_stats();

  resources.base[ RESOURCE_RAGE ] = 100;
  if ( talents.warrior.overwhelming_rage->ok() )
  {
    resources.base[ RESOURCE_RAGE ] += find_spell( 382767 )->effectN( 1 ).base_value() / 10.0;
  }
  resources.max[ RESOURCE_RAGE ]  = resources.base[ RESOURCE_RAGE ];

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in parse_player_effects_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in parse_player_effects_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base_gcd = timespan_t::from_seconds( 1.5 );

  resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1 + talents.protection.indomitable -> effectN( 3 ).percent();

  // Warriors gets +7% block from their class aura
  base.block += spec.warrior -> effectN( 7 ).percent();

  // Protection Warriors have a +8% block chance in their spec aura
  base.block += spec.protection_warrior -> effectN( 9 ).percent();
}

// warrior_t::merge ==========================================================

void warrior_t::merge( player_t& other )
{
  parse_player_effects_t::merge( other );

  const warrior_t& s = static_cast<warrior_t&>( other );

  for ( size_t i = 0, end = cd_waste_exec.size(); i < end; i++ )
  {
    cd_waste_exec[ i ]->second.merge( s.cd_waste_exec[ i ]->second );
    cd_waste_cumulative[ i ]->second.merge( s.cd_waste_cumulative[ i ]->second );
  }
}

// warrior_t::datacollection_begin ===========================================

void warrior_t::datacollection_begin()
{
  if ( active_during_iteration )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_iter[ i ]->second.reset();
    }
  }

  parse_player_effects_t::datacollection_begin();
}

// warrior_t::datacollection_end =============================================

void warrior_t::datacollection_end()
{
  if ( requires_data_collection() )
  {
    for ( size_t i = 0, end = cd_waste_iter.size(); i < end; ++i )
    {
      cd_waste_cumulative[ i ]->second.add( cd_waste_iter[ i ]->second.sum() );
    }
  }

  parse_player_effects_t::datacollection_end();
}

// NO Spec Combat Action Priority List

void warrior_t::apl_default()
{
  action_priority_list_t* default_list = get_action_priority_list( "default" );

  default_list->add_action( this, "Heroic Throw" );
}

// ==========================================================================
// Warrior Buffs
// ==========================================================================

namespace buffs
{
template <typename Base>
struct warrior_buff_t : public Base
{
public:
  using base_t = warrior_buff_t;


  warrior_buff_t( warrior_td_t& td, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( td, name, s, item )
  {
  }

  warrior_buff_t( warrior_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( &p, name, s, item )
  {
  }

protected:
  warrior_t& warrior()
  {
    return *debug_cast<warrior_t*>( Base::source );
  }
  const warrior_t& warrior() const
  {
    return *debug_cast<warrior_t*>( Base::source );
  }
};


// Rallying Cry ==============================================================

struct rallying_cry_t : public buff_t
{
  double health_change;
  rallying_cry_t( player_t* p ) :
    buff_t( p, "rallying_cry", p->find_spell( 97463 ) ), health_change( p->find_spell( 97462 )->effectN( 1 ).percent() )
  { }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );

    double old_health = player -> resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );
    player -> resources.current[ RESOURCE_HEALTH ] *= 1.0 + health_change; // Update health after the maximum is increased

    sim -> print_debug( "{} gains Rallying Cry: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];
    double old_health = player -> resources.current[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player -> resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change; // Update health before the maximum is reduced
    player -> recalculate_resource_max( RESOURCE_HEALTH );

    sim -> print_debug( "{} loses Rallying Cry: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }
};

// Steadfast as the Peaks =====================================================

struct steadfast_as_the_peaks_buff_t : public warrior_buff_t<buff_t>
{
  double health_change;
  steadfast_as_the_peaks_buff_t( warrior_t& p, util::string_view n, const spell_data_t* s ) :
    base_t( p, n, s ), health_change( data().effectN( 3 ).percent() )
  {
    add_invalidate( CACHE_BLOCK );
    set_cooldown( timespan_t::zero() );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    warrior_buff_t<buff_t>::start( stacks, value, duration );

    double old_health = player -> resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );
    player -> resources.current[ RESOURCE_HEALTH ] *= 1.0 + health_change; // Update health after the maximum is increased

    sim -> print_debug( "{} gains Steadfast as the Peaks: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];
    double old_health = player -> resources.current[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player -> resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );

    sim -> print_debug( "{} loses Steadfast as the Peaks: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }
};

// Last Stand ======================================================================

struct last_stand_buff_t : public warrior_buff_t<buff_t>
{
  double health_change;
  last_stand_buff_t( warrior_t& p, util::string_view n, const spell_data_t* s ) :
    base_t( p, n, s ), health_change( data().effectN( 1 ).percent() )
  {
    add_invalidate( CACHE_BLOCK );
    set_cooldown( timespan_t::zero() );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    warrior_buff_t<buff_t>::start( stacks, value, duration );

    double old_health = player -> resources.current[ RESOURCE_HEALTH ];
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] *= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );
    player -> resources.current[ RESOURCE_HEALTH ] *= 1.0 + health_change; // Update health after the maximum is increased

    sim -> print_debug( "{} gains Last Stand: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );
  }

  void expire_override( int, timespan_t ) override
  {
    double old_max_health = player -> resources.max[ RESOURCE_HEALTH ];
    double old_health = player -> resources.current[ RESOURCE_HEALTH ];

    player -> resources.initial_multiplier[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    // Last Stand isn't like other health buffs, the player doesn't lose health when it expires (unless it's over the cap)
    // player -> resources.current[ RESOURCE_HEALTH ] /= 1.0 + health_change;
    player -> recalculate_resource_max( RESOURCE_HEALTH );

    sim -> print_debug( "{} loses Last Stand: health pct change {}%, current health: {} -> {}, max: {} -> {}",
                        player -> name(), health_change * 100.0,
                        old_health, player -> resources.current[ RESOURCE_HEALTH ],
                        old_max_health, player -> resources.max[ RESOURCE_HEALTH ] );

    warrior_t* p = debug_cast< warrior_t* >( player );
    if ( ! p -> sim -> event_mgr.canceled && p -> sets -> has_set_bonus( WARRIOR_PROTECTION, T30, B4 ) )
      p -> buff.earthen_tenacity -> trigger();
  }
};

// Demoralizing Shout ================================================================

struct debuff_demo_shout_t : public warrior_buff_t<buff_t>
{
  int extended;
  debuff_demo_shout_t( warrior_td_t& p, warrior_t* w )
    : base_t( p, "demoralizing_shout_debuff", w -> find_spell( 1160 ) ),
      extended( 0 )
  {
    cooldown -> duration = timespan_t::zero(); // Cooldown handled by the action
    default_value = data().effectN( 1 ).percent();
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    extended = 0; // counts extra seconds gained on each debuff past the initial trigger
    return base_t::trigger( stacks, value, chance, duration );
  }
};

// Test of Might Tracker ===================================================================

struct test_of_might_t : public warrior_buff_t<buff_t>
{
  test_of_might_t( warrior_t& p, util::string_view n, const spell_data_t* s )
    : base_t( p, n, s )
  {
    quiet = true;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t* test_of_might = warrior().buff.test_of_might;
    test_of_might->expire();
    double strength = ( current_value / 10 ) * ( warrior().talents.arms.test_of_might->effectN( 1 ).percent() );
    test_of_might->trigger( 1, strength );
    current_value = 0;
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

}  // end namespace buffs

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

warrior_td_t::warrior_td_t( player_t* target, warrior_t& p ) : actor_target_data_t( target, &p ), warrior( p )
{
  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
  using namespace buffs;

  hit_by_fresh_meat = false;
  dots_deep_wounds = target->get_dot( "deep_wounds", &p );
  dots_ravager     = target->get_dot( "ravager", &p );
  dots_rend        = target->get_dot( "rend", &p );
  dots_gushing_wound = target->get_dot( "gushing_wound", &p );
  dots_thunderous_roar = target->get_dot( "thunderous_roar_dot", &p );

  debuffs_champions_might = make_buff( *this, "champions_might", p.talents.warrior.champions_spear->effectN( 1 ).trigger() )
                              ->apply_affecting_aura( p.talents.warrior.champions_might );

  debuffs_colossus_smash = make_buff( *this , "colossus_smash", p.spell.colossus_smash_debuff )
                            ->apply_affecting_aura( p.talents.arms.spiteful_serenity )
                            ->apply_affecting_aura( p.talents.arms.blunt_instruments );

  debuffs_concussive_blows = make_buff( *this, "concussive_blows", p.spell.concussive_blows_debuff )
                                 ->set_default_value_from_effect_type( A_MOD_DAMAGE_FROM_CASTER );

  debuffs_executioners_precision = make_buff( *this, "executioners_precision", p.talents.arms.executioners_precision->effectN( 1 ).trigger() );

  debuffs_fatal_mark = make_buff( *this, "fatal_mark" ) 
          ->set_duration( p.spell.fatal_mark_debuff->duration() )
          ->set_max_stack( p.spell.fatal_mark_debuff->max_stacks() );

  debuffs_skullsplitter = make_buff( *this, "skullsplitter",  p.find_spell( 427040 ) )
                            ->set_default_value( p.find_spell( 427040 ) -> effectN( 1 ).percent() )
                            ->set_stack_change_callback( [ this ]( buff_t* buff_, int old_, int new_ )
                            {
                              // Dot ticks twice as fast, for half the time with skullsplitter up
                              if ( old_ == 0 )
                              {
                                auto coeff = 1.0 / ( 1.0 + buff_ -> default_value );
                                dots_deep_wounds -> adjust( coeff );
                                dots_rend -> adjust( coeff );
                              }
                              else if ( new_ == 0 )
                              {
                                auto coeff = 1.0 + buff_ -> default_value;
                                dots_deep_wounds -> adjust( coeff );
                                dots_rend -> adjust( coeff );
                              }
                            } );

  debuffs_demoralizing_shout = new buffs::debuff_demo_shout_t( *this, &p );

  debuffs_punish = make_buff( *this, "punish", p.talents.protection.punish -> effectN( 2 ).trigger() )
    ->set_default_value( p.talents.protection.punish -> effectN( 2 ).trigger() -> effectN( 1 ).percent() );

  debuffs_taunt = make_buff( *this, "taunt", p.find_class_spell( "Taunt" ) );

  // Colossus
  debuffs_wrecked              = make_buff( *this, "wrecked", p.spell.wrecked_debuff )
                                    ->set_refresh_behavior( buff_refresh_behavior::DURATION );

  // Slayer
  debuffs_marked_for_execution = make_buff( *this, "marked_for_execution", p.spell.marked_for_execution_debuff );
  debuffs_overwhelmed          = make_buff( *this, "overwhelmed", p.spell.overwhelmed_debuff );

  // Mountain Thane
}

void warrior_td_t::target_demise()
{
  // Don't pollute results at the end-of-iteration deaths of everyone
  if ( source->sim->event_mgr.canceled )
    return;

  warrior_t* p = debug_cast<warrior_t*>( source );

  if ( p -> talents.shared.dance_of_death.ok() && p -> buff.bladestorm -> up() )
  {
    if ( ! p -> buff.dance_of_death_bladestorm -> at_max_stacks() )
    {
      p -> buff.dance_of_death_bladestorm -> trigger();
    }
  }

  if ( p -> talents.warrior.war_machine->ok() )
  {
    p->resource_gain( RESOURCE_RAGE, p -> talents.warrior.war_machine -> effectN( 1 ).trigger() -> effectN( 1 ).resource( RESOURCE_RAGE ),
                          p->gain.war_machine_demise );
  }
}

// warrior_t::init_buffs ====================================================

void warrior_t::create_buffs()
{
  parse_player_effects_t::create_buffs();

  using namespace buffs;

  buff.ashen_juggernaut = make_buff( this, "ashen_juggernaut", talents.fury.ashen_juggernaut->effectN( 1 ).trigger() )
                            ->set_cooldown( talents.fury.ashen_juggernaut->internal_cooldown() )
                            ->set_default_value_from_effect( 1 );

  buff.revenge =
      make_buff( this, "revenge", find_spell( 5302 ) )
      ->set_default_value( find_spell( 5302 )->effectN( 1 ).percent() )
      ->set_cooldown( spec.revenge_trigger -> internal_cooldown() );

  buff.avatar = make_buff( this, "avatar", talents.warrior.avatar )
      ->set_cooldown( timespan_t::zero() )
      ->apply_affecting_aura( talents.arms.spiteful_serenity )
      -> set_stack_change_callback(
        [ this ]( buff_t*, int old_, int new_ ) {
          if ( talents.warrior.blademasters_torment->ok() )
          {
            if ( old_ == 0 )  // Gained Avatar
            {
              cooldown.cleave->duration += talents.warrior.avatar->effectN( 8 ).time_value();
            }
            else if ( new_ == 0 )  // Lost Avatar
            {
              cooldown.cleave->duration -= talents.warrior.avatar->effectN( 8 ).time_value();
            }
          }
        } );

  buff.collateral_damage = make_buff( this, "collateral_damage", find_spell( 334783 ) )
      -> set_default_value_from_effect( 1 );

  buff.wild_strikes = make_buff( this, "wild_strikes", talents.warrior.wild_strikes->effectN( 2 ).trigger() )
      ->set_cooldown( talents.warrior.wild_strikes->internal_cooldown() )
      ->set_trigger_spell( talents.warrior.wild_strikes );

  buff.dancing_blades = make_buff( this, "dancing_blades", find_spell( 391688 ) )
      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC);

  buff.battering_ram = make_buff( this, "battering_ram", find_spell( 394313 ) );

  buff.berserker_rage = make_buff( this, "berserker_rage", spell.berserker_rage )
      ->set_cooldown( timespan_t::zero() );

  buff.bounding_stride = make_buff( this, "bounding_stride", find_spell( 202164 ) )
    ->set_chance( talents.warrior.bounding_stride->ok() )
    ->set_default_value( find_spell( 202164 )->effectN( 1 ).percent() );

  buff.bladestorm =
      make_buff( this, "bladestorm", specialization() == WARRIOR_FURY ? find_spell( 46924 ) : talents.arms.bladestorm )
      ->set_period( timespan_t::zero() )
      ->set_cooldown( timespan_t::zero() );

  buff.battle_stance = make_buff( this, "battle_stance", talents.warrior.battle_stance )
    ->set_activated( true )
    ->set_default_value( talents.warrior.battle_stance->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.berserker_stance = make_buff( this, "berserker_stance", talents.warrior.berserker_stance )
    ->set_activated( true )
    ->set_default_value( talents.warrior.berserker_stance->effectN( 1 ).percent() );

  // Reckless Abandon
  buff.bloodbath = make_buff( this, "bloodbath", talents.fury.reckless_abandon->effectN( 3 ).trigger() )
                      ->apply_affecting_aura( talents.fury.depths_of_insanity );
  buff.crushing_blow = make_buff( this, "crushing_blow", talents.fury.reckless_abandon->effectN( 2 ).trigger() )
                      ->apply_affecting_aura( talents.fury.depths_of_insanity );

  buff.defensive_stance = make_buff( this, "defensive_stance", talents.warrior.defensive_stance )
    ->set_activated( true )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.die_by_the_sword = make_buff( this, "die_by_the_sword", talents.arms.die_by_the_sword )
    ->set_default_value( talents.arms.die_by_the_sword->effectN( 2 ).percent() )
    ->set_cooldown( timespan_t::zero() )
    ->add_invalidate( CACHE_PARRY );

  buff.enrage = make_buff( this, "enrage", find_spell( 184362 ) )
     ->add_invalidate( CACHE_ATTACK_HASTE )
     ->add_invalidate( CACHE_RUN_SPEED )
     ->set_default_value( find_spell( 184362 )->effectN( 1 ).percent() )
     ->set_duration( find_spell( 184362 )->duration() )
     ->apply_affecting_aura( talents.fury.powerful_enrage );

  buff.frenzy = make_buff( this, "frenzy", find_spell(335082) );

  buff.heroic_leap_movement   = make_buff( this, "heroic_leap_movement" );
  buff.charge_movement        = make_buff( this, "charge_movement" );
  buff.intervene_movement     = make_buff( this, "intervene_movement" );
  buff.intercept_movement     = make_buff( this, "intercept_movement" );
  buff.shield_charge_movement = make_buff( this, "shield_charge_movement" );

  buff.into_the_fray = make_buff( this, "into_the_fray", find_spell( 202602 ) )
    ->set_chance( talents.protection.into_the_fray->ok() )
    ->set_default_value( find_spell( 202602 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_HASTE );

  buff.juggernaut = make_buff( this, "juggernaut", talents.arms.juggernaut->effectN( 1 ).trigger() )
    ->set_default_value( talents.arms.juggernaut->effectN( 1 ).trigger()->effectN( 1 ).percent() )
    ->set_duration( talents.arms.juggernaut->effectN( 1 ).trigger()->duration() )
    ->set_cooldown( talents.arms.juggernaut->internal_cooldown() );

  buff.juggernaut_prot = make_buff( this, "juggernaut_prot", talents.protection.juggernaut->effectN( 1 ).trigger() )
    ->set_default_value( talents.protection.juggernaut->effectN( 1 ).trigger()->effectN( 1 ).percent() )
    ->set_duration( talents.protection.juggernaut->effectN( 1 ).trigger()->duration() )
    ->set_cooldown( talents.protection.juggernaut->internal_cooldown() );

  buff.last_stand = new buffs::last_stand_buff_t( *this, "last_stand", talents.protection.last_stand );

  buff.meat_cleaver = make_buff( this, "meat_cleaver", spell.whirlwind_buff )
                        ->apply_affecting_aura( talents.fury.meat_cleaver );

  buff.martial_prowess =
    make_buff(this, "martial_prowess", talents.arms.martial_prowess)
    ->set_default_value(talents.arms.overpower->effectN(2).percent() );
  buff.martial_prowess->set_max_stack(buff.martial_prowess->max_stack() + as<int>( talents.arms.martial_prowess->effectN(2).base_value() ) );

  buff.merciless_bonegrinder = make_buff( this, "merciless_bonegrinder", find_spell( 383316 ) )
    ->set_default_value( find_spell( 383316 )->effectN( 1 ).percent() )
    ->set_duration( find_spell( 383316 )->duration() );

  buff.spell_reflection = make_buff( this, "spell_reflection", talents.warrior.spell_reflection )
    -> set_cooldown( 0_ms ); // handled by the ability

  buff.storm_of_swords = make_buff( this, "storm_of_swords", talents.arms.storm_of_swords->effectN( 1 ).trigger() )
                                  ->set_chance( talents.arms.storm_of_swords->proc_chance() );

  buff.sweeping_strikes = make_buff(this, "sweeping_strikes", spec.sweeping_strikes)
    ->set_duration(spec.sweeping_strikes->duration() + talents.arms.improved_sweeping_strikes->effectN( 1 ).time_value() )
    ->set_cooldown(timespan_t::zero());

  buff.ignore_pain = new ignore_pain_buff_t( this );

  buff.recklessness = make_buff( this, "recklessness", spell.recklessness_buff )
    ->set_cooldown( timespan_t::zero() )
    ->apply_affecting_aura( talents.fury.depths_of_insanity );

  buff.recklessness_warlords_torment = make_buff( this, "recklessness_warlords_torment", spell.recklessness_buff )
    ->set_cooldown( timespan_t::zero() );

  buff.sudden_death = make_buff( this, "sudden_death", specialization() == WARRIOR_FURY ? spell.sudden_death_fury : specialization() == WARRIOR_ARMS ? spell.sudden_death_arms : spell.sudden_death_arms );
  if ( tier_set.t29_fury_4pc->ok() )
    buff.sudden_death->set_rppm( RPPM_NONE, -1, 2.5 ); // hardcode unsupported type 8 modifier

  buff.seismic_reverberation_revenge = make_buff( this, "seismic_reverberation_revenge", spell.seismic_reverberation_revenge );

  buff.shield_block = make_buff( this, "shield_block", spell.shield_block_buff )
    ->set_duration( spell.shield_block_buff->duration() + talents.protection.enduring_defenses->effectN( 1 ).time_value() )
    ->set_cooldown( timespan_t::zero() )
    ->add_invalidate( CACHE_BLOCK );

  buff.shield_wall = make_buff( this, "shield_wall", spell.shield_wall )
    ->set_default_value( spell.shield_wall->effectN( 1 ).percent() )
    ->set_cooldown( timespan_t::zero() );

  buff.slaughtering_strikes = make_buff( this, "slaughtering_strikes", find_spell( 393931 ) )
                                  ->set_default_value_from_effect( 1 );

  const spell_data_t* test_of_might_tracker = talents.arms.test_of_might.spell()->effectN( 1 ).trigger()->effectN( 1 ).trigger();
  buff.test_of_might_tracker = new test_of_might_t( *this, "test_of_might_tracker", test_of_might_tracker );
  buff.test_of_might_tracker->set_duration( spell.colossus_smash_debuff->duration() + talents.arms.spiteful_serenity->effectN( 8 ).time_value() 
                                            + talents.arms.blunt_instruments->effectN( 2 ).time_value() );
  buff.test_of_might = make_buff( this, "test_of_might", find_spell( 385013 ) )
                              ->set_default_value( talents.arms.test_of_might->effectN( 1 ).percent() )
                              ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
                              ->set_trigger_spell( test_of_might_tracker );

  buff.bloodcraze = make_buff( this, "bloodcraze", talents.fury.bloodcraze->effectN( 1 ).trigger() );

  //buff.vengeance_ignore_pain = make_buff( this, "vengeance_ignore_pain", find_spell( 202574 ) )
    //->set_chance( talents.vengeance->ok() )
    //->set_default_value( find_spell( 202574 )->effectN( 1 ).percent() );

  //buff.vengeance_revenge = make_buff( this, "vengeance_revenge", find_spell( 202573 ) )
    //->set_chance( talents.vengeance->ok() )
    //->set_default_value( find_spell( 202573 )->effectN( 1 ).percent() );

  buff.in_for_the_kill = new in_for_the_kill_t( *this, "in_for_the_kill", find_spell( 248622 ) );

  buff.dance_of_death_ravager = make_buff( this, "dance_of_death_ravager", find_spell( 459567 ) )
      ->set_duration( 0_s ) // Handled by the ravager action
      ->set_max_stack( as<int>(spell.dance_of_death->effectN( 2 ).base_value()) );

  buff.dance_of_death_bladestorm = make_buff( this, "dance_of_death_bladestorm", spell.dance_of_death_bs_buff )
      ->set_duration( 20_s ); // Slightly longer than max extension;

  buff.seeing_red = make_buff( this, "seeing_red", find_spell( 386486 ) );

  buff.seeing_red_tracking =
      make_buff( this, "seeing_red_tracking", find_spell( 386477 ) )
          ->set_quiet( true )
          ->set_duration( timespan_t::zero() )
          ->set_max_stack( 100 )
          ->set_default_value( 0 );

  buff.violent_outburst = make_buff( this, "violent_outburst", find_spell( 386478 ) );

  buff.brace_for_impact = make_buff( this, "brace_for_impact", talents.protection.brace_for_impact->effectN( 1 ).trigger() )
                         -> set_default_value( talents.protection.brace_for_impact->effectN( 1 ).trigger()->effectN( 1 ).percent() )
                         -> set_initial_stack( 1 );

  // Covenant Abilities====================================================================================================

  buff.conquerors_banner = make_buff( this, "conquerors_banner", covenant.conquerors_banner )
    ->set_default_value_from_effect_type( A_PERIODIC_ENERGIZE )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
      resource_gain( RESOURCE_RAGE, (specialization() == WARRIOR_FURY ? 6 : 4), gain.conquerors_banner );
    } );

  buff.conquerors_frenzy    = make_buff( this, "conquerors_frenzy", find_spell( 325862 ) )
                               ->set_default_value( find_spell( 325862 )->effectN( 2 ).percent() )
                               ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.conquerors_mastery = make_buff<stat_buff_t>( this, "conquerors_mastery", find_spell( 325862 ) );

  buff.show_of_force = make_buff( this, "show_of_force", talents.protection.show_of_force -> effectN( 1 ).trigger() )
                           ->set_default_value( talents.protection.show_of_force -> effectN( 1 ).percent() );

  // Arma: 2022 Nov 4.  Unnerving focus seems to get the value from the parent, not the value set in the buff
  buff.unnerving_focus = make_buff( this, "unnerving_focus", talents.protection.unnerving_focus -> effectN( 1 ).trigger() )
                           ->set_default_value( talents.protection.unnerving_focus -> effectN( 1 ).percent() );

   // T29 Tier Effects ===============================================================================================================

  buff.strike_vulnerabilities = make_buff( this, "strike_vulnerabilities", tier_set.t29_arms_4pc->ok() ?
                                           find_spell( 394173 ) : spell_data_t::not_found() )
                               ->set_default_value( find_spell( 394173 )->effectN( 1 ).percent() )
                               ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.vanguards_determination = make_buff( this, "vanguards_determination", tier_set.t29_prot_2pc->ok() ?
                                            find_spell( 394056 ) : spell_data_t::not_found() )
                                    ->set_default_value( find_spell( 394056 )->effectN( 1 ).percent());

  // T30 Tier Effects ===============================================================================================================
  buff.crushing_advance = make_buff( this, "crushing_advance", tier_set.t30_arms_4pc->ok() ?
                               find_spell( 410138 ) : spell_data_t::not_found() )
                          ->set_default_value( find_spell( 410138 )->effectN( 1 ).percent() );

  buff.merciless_assault = make_buff( this, "merciless_assault", tier_set.t30_fury_4pc->ok() ? 
                                find_spell( 409983 ) : spell_data_t::not_found() )
                           ->set_default_value( find_spell( 409983 )->effectN( 2 ).percent() )
                           ->set_duration( find_spell( 409983 )->duration() );

  buff.earthen_tenacity = make_buff( this, "earthen_tenacity", tier_set.t30_prot_4pc -> ok() ?
                                find_spell( 410218 ) : spell_data_t::not_found() );

  // T31 Tier Effects ===============================================================================================================

  buff.furious_bloodthirst = make_buff( this, "furious_bloodthirst", tier_set.t31_fury_2pc->ok() ?
                                   find_spell( 423211 ) : spell_data_t::not_found() )
                                   ->set_cooldown( 0_ms ); // used for buff consumption, not application

  buff.fervid = make_buff( this, "fervid", sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2) ? find_spell( 425517 ) : spell_data_t::not_found() );

  buff.fervid_opposition = make_buff( this, "fervid_opposition", sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2) ? find_spell( 427413 ) : spell_data_t::not_found() );


  // Colossus
  buff.colossal_might       = make_buff( this, "colossal_might", find_spell( 440989 ) )
                                ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                                ->apply_affecting_aura( talents.colossus.dominance_of_the_colossus )
                                ->apply_affecting_aura( spec.protection_warrior );

  // Slayer
  buff.imminent_demise      = make_buff( this, "imminent_demise", find_spell( 445606 ) );
  buff.brutal_finish        = make_buff( this, "brutal_finish", find_spell( 446918 ) );
  buff.fierce_followthrough = make_buff( this, "fierce_followthrough", find_spell( 458689 ) );
  buff.opportunist          = make_buff( this, "opportunist", find_spell( 456120 ) );

  // Mountain Thane
  buff.thunder_blast          = make_buff( this, "thunder_blast", find_spell( 435615 ) );
  buff.steadfast_as_the_peaks = new buffs::steadfast_as_the_peaks_buff_t( *this, "steadfast_as_the_peaks", find_spell( 437152 ) );
  buff.burst_of_power         = make_buff( this, "burst_of_power", find_spell( 437121 ) )
                                  ->set_initial_stack( find_spell( 437121 )->max_stacks() )
                                  ->set_cooldown( talents.mountain_thane.burst_of_power -> internal_cooldown() )
                                  ->set_chance( talents.mountain_thane.burst_of_power->proc_chance() );

  // TWW1 Tier
  buff.overpowering_might = make_buff( this, "overpowering_might", find_spell( 455483 ) );  // Arms 2pc
  buff.lethal_blows       = make_buff( this, "lethal_blows", find_spell( 455485 ) );        // Arms 4pc
  buff.bloody_rampage     = make_buff( this, "bloody_rampage", find_spell( 455490 ) );      // Fury 2pc
  buff.deep_thirst        = make_buff( this, "deep_thirst", find_spell( 455495 ) );         // Fury 4pc
  buff.expert_strategist  = make_buff( this, "expert_strategist", find_spell( 455499 ) );   // Prot 2pc
  buff.brutal_followup    = make_buff( this, "brutal_followup", find_spell( 455501 ) );     // Prot 4pc
}

// warrior_t::init_finished =============================================
void warrior_t::init_finished()
{
  parse_player_effects();
  parse_player_effects_t::init_finished();
}

// warrior_t::init_rng ==================================================
void warrior_t::init_rng()
{
  parse_player_effects_t::init_rng();
  rppm.fatal_mark       = get_rppm( "fatal_mark", talents.arms.fatality );
  rppm.revenge          = get_rppm( "revenge_trigger", spec.revenge_trigger );
  rppm.sudden_death     = get_rppm( "sudden death", specialization() == WARRIOR_FURY ? talents.fury.sudden_death : 
                                                    specialization() == WARRIOR_ARMS ? talents.arms.sudden_death : 
                                                    talents.protection.sudden_death );
  rppm.t31_sudden_death = get_rppm( "t31_sudden_death", find_spell( 422923 ) );
  rppm.slayers_dominance = get_rppm( "slayers_dominance", talents.slayer.slayers_dominance );
}

// warrior_t::validate_fight_style ==========================================
bool warrior_t::validate_fight_style( fight_style_e style ) const
{
  if ( specialization() == WARRIOR_PROTECTION )
  {
    switch ( style )
    {
      case FIGHT_STYLE_DUNGEON_ROUTE:
      case FIGHT_STYLE_DUNGEON_SLICE:
        return false;
      default:
        return true;
    }
  }
  return true;
}

// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  parse_player_effects_t::init_scaling();

  if ( specialization() == WARRIOR_FURY )
  {
    scaling->enable( STAT_WEAPON_OFFHAND_DPS );
  }

  if ( specialization() == WARRIOR_PROTECTION )
  {
    scaling->enable( STAT_BONUS_ARMOR );
  }

  scaling->disable( STAT_AGILITY );
}

// warrior_t::init_gains ====================================================

void warrior_t::init_gains()
{
  parse_player_effects_t::init_gains();

  gain.archavons_heavy_hand             = get_gain( "archavons_heavy_hand" );
  gain.avatar                           = get_gain( "avatar" );
  gain.avatar_torment                   = get_gain( "avatar_torment" );
  gain.avoided_attacks                  = get_gain( "avoided_attacks" );
  gain.battlelord                       = get_gain( "battlelord" );
  gain.bloodsurge                       = get_gain( "bloodsurge" );
  gain.charge                           = get_gain( "charge" );
  gain.conquerors_banner                = get_gain( "conquerors_banner" );
  gain.critical_block                   = get_gain( "critical_block" );
  gain.execute                          = get_gain( "execute" );
  gain.frothing_berserker               = get_gain( "frothing_berserker" );
  gain.melee_crit                       = get_gain( "melee_crit" );
  gain.melee_main_hand                  = get_gain( "melee_main_hand" );
  gain.melee_off_hand                   = get_gain( "melee_off_hand" );
  gain.revenge                          = get_gain( "revenge" );
  gain.shield_charge                    = get_gain( "shield_charge" );
  gain.shield_slam                      = get_gain( "shield_slam" );
  gain.champions_spear                  = get_gain( "champions_spear" );
  gain.finishing_blows                  = get_gain( "finishing_blows" );
  gain.booming_voice                    = get_gain( "booming_voice" );
  gain.thunder_blast                    = get_gain( "thunder_blast" );
  gain.thunder_clap                     = get_gain( "thunder_clap" );
  gain.whirlwind                        = get_gain( "whirlwind" );
  gain.instigate                        = get_gain( "instigate" );
  gain.war_machine_demise               = get_gain( "war_machine_demise" );

  gain.ceannar_rage           = get_gain( "ceannar_rage" );
  gain.cold_steel_hot_blood   = get_gain( "cold_steel_hot_blood" );
  gain.endless_rage           = get_gain( "endless_rage" );
  gain.lord_of_war            = get_gain( "lord_of_war" );
  gain.meat_cleaver           = get_gain( "meat_cleaver" );
  gain.valarjar_berserking    = get_gain( "valarjar_berserking" );
  gain.rage_from_damage_taken = get_gain( "rage_from_damage_taken" );
  gain.ravager                = get_gain( "ravager" );
  gain.simmering_rage         = get_gain( "simmering_rage" );
  gain.storm_of_steel         = get_gain( "storm_of_steel" );
  gain.execute_refund         = get_gain( "execute_refund" );
  gain.merciless_assault      = get_gain( "merciless_assault" );
  gain.thorims_might          = get_gain( "thorims_might" );
  gain.burst_of_power         = get_gain( "burst_of_power" );
}

// warrior_t::init_position ====================================================

void warrior_t::init_position()
{
  parse_player_effects_t::init_position();
}

// warrior_t::init_procs ======================================================

void warrior_t::init_procs()
{
  parse_player_effects_t::init_procs();
  proc.battlelord          = get_proc( "Battlelord Mortal Strike reset");
  proc.battlelord_wasted   = get_proc( "Battlelord Mortal Strike reset wasted" );
  proc.delayed_auto_attack = get_proc( "delayed_auto_attack" );
  proc.tactician           = get_proc( "tactician" );
}

// warrior_t::init_resources ================================================

void warrior_t::init_resources( bool force )
{
  parse_player_effects_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ] = 0;  // By default, simc sets all resources to full. However, Warriors cannot
                                           // reliably start combat with more than 0 rage. This will also ensure that
                                           // the 20-40 rage from Charge is not overwritten.
}

// warrior_t::default_potion ================================================

std::string warrior_t::default_potion() const
{
  std::string fury_pot =
      ( true_level > 70 )
          ? "tempered_potion_3"
          : ( true_level > 60 )
                ? "elemental_potion_of_ultimate_power_3"
                : "disabled";

  std::string arms_pot =
      ( true_level > 70 )
          ? "tempered_potion_3"
          : ( true_level > 60 )
                ? "elemental_potion_of_ultimate_power_3"
                : "disabled";

  std::string protection_pot =
      ( true_level > 70 )
          ? "tempered_potion_3"
          : ( true_level > 60 )
                ? "elemental_potion_of_ultimate_power_3"
                : "disabled";

  switch ( specialization() )
  {
    case WARRIOR_FURY:
      return fury_pot;
    case WARRIOR_ARMS:
      return arms_pot;
    case WARRIOR_PROTECTION:
      return protection_pot;
    default:
      return "disabled";
  }
}

// warrior_t::default_flask =================================================

std::string warrior_t::default_flask() const
{
  if ( specialization() == WARRIOR_PROTECTION && true_level > 70 )
    return "flask_of_alchemical_chaos_3";

  return ( true_level > 70 )
             ? "flask_of_alchemical_chaos_3"
             : ( true_level > 50 )
                   ? "spectral_flask_of_power"
                   : "disabled";
}

// warrior_t::default_food ==================================================

std::string warrior_t::default_food() const
{
  std::string fury_food = ( true_level > 70 )
                              ? "the_sushi_special"
                              : ( true_level > 60 )
                                    ? "thousandbone_tongueslicer"
                                    : "disabled";

  std::string arms_food = ( true_level > 70 )
                              ? "the_sushi_special"
                              : ( true_level > 60 )
                                    ? "feisty_fish_sticks"
                                    : "disabled";

  std::string protection_food = ( true_level > 70 )
                              ? "feast_of_the_midnight_masquerade"
                              : ( true_level > 60 )
                                    ? "feisty_fish_sticks"
                                    : "disabled";

  switch ( specialization() )
  {
    case WARRIOR_FURY:
      return fury_food;
    case WARRIOR_ARMS:
      return arms_food;
    case WARRIOR_PROTECTION:
      return protection_food;
    default:
      return "disabled";
  }
}

// warrior_t::default_rune ==================================================

std::string warrior_t::default_rune() const
{
  return ( true_level >= 70 ) ? "crystallized"
                               : ( true_level >= 60 ) ? "draconic" : "disabled";
}

// warrior_t::default_temporary_enchant =====================================

std::string warrior_t::default_temporary_enchant() const
{
  std::string fury_temporary_enchant = ( true_level >= 60 )
                              ? "main_hand:algari_mana_oil_3/off_hand:algari_mana_oil_3"
                              : "disabled";

  std::string arms_temporary_enchant = ( true_level >= 60 )
                              ? "main_hand:algari_mana_oil_3"
                              : "disabled";

  std::string protection_temporary_enchant = ( true_level >= 60 )
                              ? "main_hand:ironclaw_whetstone_3"
                              : "disabled";
  switch ( specialization() )
  {
    case WARRIOR_FURY:
      return fury_temporary_enchant;
    case WARRIOR_ARMS:
      return arms_temporary_enchant;
    case WARRIOR_PROTECTION:
      return protection_temporary_enchant;
    default:
      return "disabled";
  }
}

// warrior_t::init_actions ==================================================

void warrior_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    parse_player_effects_t::init_action_list();
    return;
  }

  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
    {
      sim->errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    }

    quiet = true;
    return;
  }

  clear_action_priority_lists();

  switch ( specialization() )
  {
    case WARRIOR_FURY:
        warrior_apl::fury( this );
      break;
    case WARRIOR_ARMS:
        warrior_apl::arms( this );
      break;
    case WARRIOR_PROTECTION:
      warrior_apl::protection( this );
      break;
    default:
      apl_default();  // DEFAULT
      break;
  }

  // Default
  use_default_action_list = true;
  parse_player_effects_t::init_action_list();
}

// warrior_t::arise() ======================================================

void warrior_t::arise()
{
  parse_player_effects_t::arise();
}

// warrior_t::combat_begin ==================================================

void warrior_t::combat_begin()
{
  if ( !sim->fixed_time )
  {
    if ( warrior_fixed_time )
    {
      for ( auto* p : sim->player_list )
      {
         if ( p->specialization() != WARRIOR_FURY && p->specialization() != WARRIOR_ARMS )
        {
          warrior_fixed_time = false;
          break;
        }
      }
      if ( warrior_fixed_time )
      {
        sim->fixed_time = true;
        sim->error(
            "To fix issues with the target exploding <20% range due to execute, fixed_time=1 has been enabled. This "
            "gives similar results" );
        sim->error(
            "to execute's usage in a raid sim, without taking an eternity to simulate. To disable this option, add "
            "warrior_fixed_time=0 to your sim." );
      }
    }
  }
  parse_player_effects_t::combat_begin();
  buff.into_the_fray -> trigger( into_the_fray_friends < 0 ? buff.into_the_fray -> max_stack() : into_the_fray_friends + 1 );
}

// Into the fray

struct into_the_fray_callback_t
{
  warrior_t* w;
  double fray_distance;
  into_the_fray_callback_t( warrior_t* p ) : w( p ), fray_distance( 0 )
  {
    fray_distance = p->talents.protection.into_the_fray->effectN( 1 ).base_value();
  }

  void operator()( player_t* )
  {
    size_t i            = w->sim->target_non_sleeping_list.size();
    size_t buff_stacks_ = w->into_the_fray_friends;
    while ( i > 0 && buff_stacks_ < w->buff.into_the_fray->data().max_stacks() )
    {
      i--;
      player_t* target_ = w->sim->target_non_sleeping_list[ i ];
      if ( w->get_player_distance( *target_ ) <= fray_distance )
      {
        buff_stacks_++;
      }
    }
    if ( w->buff.into_the_fray->current_stack != as<int>( buff_stacks_ ) )
    {
      w->buff.into_the_fray->expire();
      w->buff.into_the_fray->trigger( static_cast<int>( buff_stacks_ ) );
    }
  }
};

// warrior_t::create_actions ================================================

void warrior_t::create_actions()
{
  if ( talents.fury.rampage->ok() )
  {
    // rampage now hits 4 times instead of 5 and effect indexes shifted
    rampage_attack_t* first  = new rampage_attack_t( this, talents.fury.rampage->effectN( 2 ).trigger(), "rampage1" );
    rampage_attack_t* second = new rampage_attack_t( this, talents.fury.rampage->effectN( 3 ).trigger(), "rampage2" );
    rampage_attack_t* third  = new rampage_attack_t( this, talents.fury.rampage->effectN( 4 ).trigger(), "rampage3" );
    rampage_attack_t* fourth = new rampage_attack_t( this, talents.fury.rampage->effectN( 5 ).trigger(), "rampage4" );

    // the order for hits is now OH MH OH MH
    first->weapon  = &( this->off_hand_weapon );
    second->weapon = &( this->main_hand_weapon );
    third->weapon  = &( this->off_hand_weapon );
    fourth->weapon = &( this->main_hand_weapon );

    this->rampage_attacks.push_back( first );
    this->rampage_attacks.push_back( second );
    this->rampage_attacks.push_back( third );
    this->rampage_attacks.push_back( fourth );
  }

  if ( spec.deep_wounds_ARMS->ok() )
    active.deep_wounds_ARMS = new deep_wounds_ARMS_t( this );
  if ( spec.deep_wounds_PROT->ok() )
    active.deep_wounds_PROT = new deep_wounds_PROT_t( this );
  if ( talents.arms.fatality->ok() )
    active.fatality = new fatality_t( this );

  if ( talents.protection.tough_as_nails->ok() )
  {
    active.tough_as_nails = new tough_as_nails_t( this );
  }
  if ( talents.warrior.berserkers_torment->ok() )
  {
    active.torment_recklessness = new torment_recklessness_t( this, "", "recklessness_torment", find_spell( 1719 ) );
    active.torment_avatar       = new torment_avatar_t( this, "", "avatar_torment", find_spell( 107574 ) );
    for ( action_t* action : { active.torment_recklessness, active.torment_avatar } )
    {
      action->background  = true;
      action->trigger_gcd = timespan_t::zero();
    }
  }
  if ( talents.warrior.titans_torment->ok() )
  {
    active.torment_avatar       = new torment_avatar_t( this, "", "avatar_torment", find_spell( 107574 ) );
    active.torment_odyns_fury   = new torment_odyns_fury_t( this, "", "odyns_fury_torment", find_spell( 385059 ) );
    for ( action_t* action : { active.torment_avatar, active.torment_odyns_fury } )
    {
      action->background  = true;
      action->trigger_gcd = timespan_t::zero();
    }
  }
  if( talents.slayer.slayers_dominance->ok() )
  {
    active.slayers_strike = new slayers_strike_t( this );
  }

  parse_player_effects_t::create_actions();
}

void warrior_t::activate()
{
  parse_player_effects_t::activate();

  // If the value is equal to -1, don't use the callback and just assume max stacks all the time
  if ( talents.protection.into_the_fray->ok() && into_the_fray_friends >= 0 )
  {
    sim->target_non_sleeping_list.register_callback( into_the_fray_callback_t( this ) );
  }
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  parse_player_effects_t::reset();
  first_rampage_attack_missed = false;
}

// Movement related overrides. =============================================

void warrior_t::moving()
{
}

void warrior_t::interrupt()
{
  buff.charge_movement->expire();
  buff.heroic_leap_movement->expire();
  buff.intervene_movement->expire();
  buff.intercept_movement->expire();
  buff.shield_charge_movement->expire();

  parse_player_effects_t::interrupt();
}

void warrior_t::teleport( double, timespan_t )
{
  // All movement "teleports" are modeled.
}

void warrior_t::trigger_movement( double distance, movement_direction_type direction )
{
  if ( talents.protection.into_the_fray->ok() && into_the_fray_friends >= 0 )
  {
    into_the_fray_callback_t( this );
  }

  else
  {
    parse_player_effects_t::trigger_movement( distance, direction );
  }
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( school_e school ) const
{
  double m = parse_player_effects_t::composite_player_multiplier( school );

  if ( buff.defensive_stance->check() )
  {
    m *= 1.0 + talents.warrior.defensive_stance->effectN( 2 ).percent() + spec.protection_warrior->effectN( 18 ).percent();
  }

  return m;
}

// warrior_t::composite_player_target_multiplier ==============================
double warrior_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = parse_player_effects_t::composite_player_target_multiplier( target, school );

  if ( talents.warrior.concussive_blows.ok() )
  {
    auto debuff = get_target_data( target )->debuffs_concussive_blows;

    if ( debuff->check() && debuff->has_common_school( school ) )
    {
      m *= 1.0 + debuff->value();
    }
  }

  return m;
}

// warrior_t::composite_player_target_crit_chance =============================

double warrior_t::composite_player_target_crit_chance( player_t* target ) const
{
  double c = player_t::composite_player_target_crit_chance( target );

  auto td = get_target_data( target );

  // crit chance bonus is not currently whitelisted in data
  if ( sets->has_set_bonus( WARRIOR_ARMS, T30, B2 ) && td->dots_deep_wounds->is_ticking() )
    c += spell.deep_wounds_arms->effectN( 4 ).percent();

  return c;
}

// warrior_t::composite_mastery =============================================

double warrior_t::composite_mastery() const
{
  double y = parse_player_effects_t::composite_mastery();

  if ( specialization() == WARRIOR_ARMS )
  {
    y += talents.arms.deft_experience->effectN( 1 ).base_value();
  }
  else
  {
    y += talents.fury.deft_experience->effectN( 1 ).base_value();
  }

  return y;
}

// warrior_t::composite_damage_versatility =============================================

double warrior_t::composite_damage_versatility() const
{
  double cdv = parse_player_effects_t::composite_damage_versatility();

  cdv += talents.arms.valor_in_victory->effectN( 1 ).percent();

  return cdv;
}

// warrior_t::composite_heal_versatility ==============================

double warrior_t::composite_heal_versatility() const
{
  double chv = parse_player_effects_t::composite_heal_versatility();

  chv += talents.arms.valor_in_victory->effectN( 1 ).percent();

  return chv;
}

// warrior_t::composite_mitigation_versatility ========================

double warrior_t::composite_mitigation_versatility() const
{
  double cmv = parse_player_effects_t::composite_mitigation_versatility();

  cmv += talents.arms.valor_in_victory->effectN( 1 ).percent();

  return cmv;
}

// warrior_t::composite_attribute ================================

double warrior_t::composite_attribute( attribute_e attr ) const
{
  double p = parse_player_effects_t::composite_attribute( attr );

  if ( attr == ATTR_STRENGTH )
  {
    // Arma 2022 Nov 13 Note that unless we handle str manually in the armor calcs Attt for prot ends up looping here.  
    // As Str and armor are both looping in the stat cache
    // As we have it implemented properly in the armor calcs, this can be globally enabled.
    // get_attribute -> composite_attribute -> bonus_armor -> composite_bonus_armor -> strength -> get_attribute
    //if ( specialization() != WARRIOR_PROTECTION )
    p += ( talents.warrior.armored_to_the_teeth->effectN( 2 ).percent() * cache.armor() );
  }

  return p;
}

// warrior_t::composite_attribute_multiplier ================================

double warrior_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = parse_player_effects_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + talents.protection.focused_vigor->effectN( 1 ).percent();
  }

  // Protection has increased stamina from vanguard
  if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + spec.vanguard -> effectN( 2 ).percent();
    m *= 1.0 + talents.warrior.endurance_training -> effectN( 1 ).percent();
  }

  return m;
}

// warrior_t::composite_rating_multiplier ===================================

double warrior_t::composite_rating_multiplier( rating_e rating ) const
{
  return parse_player_effects_t::composite_rating_multiplier( rating );
}

// warrior_t::matching_gear_multiplier ======================================

double warrior_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( ( attr == ATTR_STRENGTH ) && ( specialization() != WARRIOR_PROTECTION ) )
  {
    return 0.05;
  }

  if ( ( attr == ATTR_STAMINA ) && ( specialization() == WARRIOR_PROTECTION ) )
  {
    return 0.05;
  }

  return 0.0;
}

// warrior_t::composite_armor_multiplier ==========================================

double warrior_t::composite_armor_multiplier() const
{
  double ar = parse_player_effects_t::composite_armor_multiplier();

  // Arma 2022 Nov 10.  To avoid an infinite loop, we manually calculate the str benefit of armored to the teeth here, and apply the armor we would gain from it
  if ( talents.warrior.armored_to_the_teeth->ok() && specialization() == WARRIOR_PROTECTION )
  {
    auto q = spec.vanguard -> effectN( 1 ).percent() *
              talents.warrior.armored_to_the_teeth -> effectN( 2 ).percent() *
              ( 1+talents.warrior.reinforced_plates->effectN( 1 ).percent()) *
              ( 1+talents.protection.focused_vigor->effectN( 3 ).percent() ) *
              ( 1+talents.protection.enduring_alacrity->effectN( 3 ).percent() );

    ar *= 1 + ( 1+talents.protection.focused_vigor->effectN( 3 ).percent()) * ( q/(1 - q) );
  }

 // Generally Modify Armor% (101)

  ar *= 1.0 + talents.warrior.reinforced_plates->effectN( 1 ).percent();
  ar *= 1.0 + talents.protection.enduring_alacrity -> effectN( 3 ).percent();
  ar *= 1.0 + talents.protection.focused_vigor -> effectN( 3 ).percent();

  return ar;
}

// warrior_t::composite_bonus_armor ==========================================

double warrior_t::composite_bonus_armor() const
{
  double ba = parse_player_effects_t::composite_bonus_armor();

  // Arma 2022 Nov 10.
  // We have to use base strength here, and then multiply by all the various STR buffs, while avoiding multiplying by Armored to the teeth.
  // This is to prevent an infinite loop between Attt using the cache.armor() and composite_bonus_armor using cache.strength()
  // We have to add all str buffs to this, if we want to avoid vanguard missing anything.  Not ideal.

  if ( specialization() == WARRIOR_PROTECTION )
  {
    // Pulls strength using the base player_t functions.  We call these directly to avoid using the warrior_t versions, as armor contribution from 
    // attt is calculated during composite_armor_multiplier
    auto current_str = util::floor( parse_player_effects_t::composite_attribute( ATTR_STRENGTH ) * parse_player_effects_t::composite_attribute_multiplier( ATTR_STRENGTH ) );
    // if there is anything else in warrior_t::composite_attribute_multiplier that applies to str, like focused_vigor for instance
    // it needs to be added here as well
    ba += spec.vanguard -> effectN( 1 ).percent() * current_str * (1.0 + talents.protection.focused_vigor->effectN( 1 ).percent());
  }

  // If in the future if blizz changes behavior, and we want to go back to using the two caches, we can use the below code
  //if( specialization() == WARRIOR_PROTECTION )
  //  ba += spec.vanguard -> effectN( 1 ).percent() * cache.strength();

  return ba;
}

// warrior_t::composite_base_armor_multiplier ================================
double warrior_t::composite_base_armor_multiplier() const
{
  // Generally Modify Base Resistance (142)
  double a = parse_player_effects_t::composite_base_armor_multiplier();

  return a;
}

// warrior_t::composite_block ================================================

double warrior_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * mastery.critical_block->effectN( 2 ).mastery_value();
  double b                   = parse_player_effects_t::composite_block_dr( block_subject_to_dr );

  // shield block adds 100% block chance
  if ( buff.shield_block -> up() )
  {
    b += spell.shield_block_buff -> effectN( 1 ).percent();
  }

  // Not affected by DR
  if ( talents.protection.shield_specialization->ok() )
    b += talents.protection.shield_specialization->effectN( 1 ).percent();

  return b;
}

// warrior_t::composite_block_reduction =====================================

double warrior_t::composite_block_reduction( action_state_t* s ) const
{
  double br = parse_player_effects_t::composite_block_reduction( s );

  if ( buff.brace_for_impact -> check() )
  {
    br *= 1.0 + buff.brace_for_impact -> check() * talents.protection.brace_for_impact -> effectN( 2 ).percent();
  }

  if ( talents.protection.shield_specialization->ok() )
  {
      br += talents.protection.shield_specialization->effectN( 2 ).percent();
  }

  return br;
}

// warrior_t::composite_parry_rating() ========================================

double warrior_t::composite_parry_rating() const
{
  double p = parse_player_effects_t::composite_parry_rating();

  // TODO: remove the spec check once riposte is pulled from spelldata
  if ( spec.riposte -> ok() || specialization() == WARRIOR_PROTECTION )
  {
    p += composite_melee_crit_rating();
  }
  return p;
}

// warrior_t::composite_parry =================================================

double warrior_t::composite_parry() const
{
  double parry = parse_player_effects_t::composite_parry();

  if ( buff.die_by_the_sword->check() )
  {
    parry += talents.arms.die_by_the_sword->effectN( 1 ).percent();
  }
  return parry;
}

// warrior_t::composite_attack_power_multiplier ==============================

double warrior_t::composite_attack_power_multiplier() const
{
  double ap = parse_player_effects_t::composite_attack_power_multiplier();

  if ( mastery.critical_block -> ok() )
  {
    ap *= 1.0 + mastery.critical_block -> effectN( 5 ).mastery_value() * cache.mastery();
  }
  return ap;
}

// warrior_t::composite_crit_block =====================================

double warrior_t::composite_crit_block() const
{

  double b = parse_player_effects_t::composite_crit_block();

  if ( mastery.critical_block->ok() )
  {
    b += cache.mastery() * mastery.critical_block->effectN( 1 ).mastery_value();
  }

  return b;
}

// warrior_t::composite_melee_crit_chance =========================================

double warrior_t::composite_melee_crit_chance() const
{
  double c = parse_player_effects_t::composite_melee_crit_chance();

  c += buff.conquerors_frenzy->check_value();
  c += talents.warrior.cruel_strikes->effectN( 1 ).percent();
  c += buff.battle_stance->check_value();

  c += buff.strike_vulnerabilities->check_value();

  if ( specialization() == WARRIOR_ARMS )
  {
    c += talents.arms.critical_thinking->effectN( 1 ).percent();
  }
  else if ( specialization() == WARRIOR_FURY )
  {
    c += talents.fury.critical_thinking->effectN( 1 ).percent();
  }

  c += talents.protection.focused_vigor->effectN( 2 ).percent();

  return c;
}

// warrior_t::composite_melee_crit_rating =========================================

double warrior_t::composite_melee_crit_rating() const
{
  double c = parse_player_effects_t::composite_melee_crit_rating();

  return c;
}

// warrior_t::composite_player_critical_damage_multiplier ==================
double warrior_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double cdm = parse_player_effects_t::composite_player_critical_damage_multiplier( s );

  return cdm;
}

// warrior_t::composite_spell_crit_chance =========================================
double warrior_t::composite_spell_crit_chance() const
{
  double c = parse_player_effects_t::composite_spell_crit_chance();

  c += talents.warrior.cruel_strikes->effectN( 1 ).percent();
  c += buff.battle_stance->check_value();

  if ( specialization() == WARRIOR_ARMS )
  {
    c += talents.arms.critical_thinking->effectN( 1 ).percent();
  }
  else if ( specialization() == WARRIOR_FURY )
  {
    c += talents.fury.critical_thinking->effectN( 1 ).percent();
  }

  c += talents.protection.focused_vigor->effectN( 2 ).percent();

  return c;
}

// warrior_t::composite_leech ==============================================

double warrior_t::composite_leech() const
{
  double m = parse_player_effects_t::composite_leech();

  m += talents.warrior.leeching_strikes->effectN( 1 ).percent();

  return m;
}


// warrior_t::resource_gain =================================================

double warrior_t::resource_gain( resource_e r, double a, gain_t* g, action_t* action )
{
  if ( ( buff.recklessness->check() || buff.recklessness_warlords_torment->check() ) && r == RESOURCE_RAGE )
  {
    bool do_not_double_rage = false;

    do_not_double_rage      = ( g == gain.ceannar_rage || g == gain.valarjar_berserking || g == gain.simmering_rage || 
                                  g == gain.frothing_berserker || g == gain.thorims_might );

    if ( !do_not_double_rage )  // FIXME: remove this horror after BFA launches, keep Simmering Rage
    {
      if ( buff.recklessness->check() )
        a *= 1.0 + spell.recklessness_buff->effectN( 1 ).percent();
      else if ( buff.recklessness_warlords_torment->check() )
        a *= 1.0 + talents.warrior.warlords_torment->effectN( 2 ).percent();
    }
  }

  if ( buff.unnerving_focus->up() )
  {
    a *= 1.0 + buff.unnerving_focus->stack_value();//Spell data lists all the abilities it provides rage gain to separately - currently it is all of our abilities.
  }
  return parse_player_effects_t::resource_gain( r, a, g, action );
}

// warrior_t::non_stacking_movement_modifier ================================

double warrior_t::non_stacking_movement_modifier() const
{
  double ms = parse_player_effects_t::non_stacking_movement_modifier();

  // These are ordered in the highest speed movement increase to the lowest, there's no reason to check the rest as they
  // will just be overridden. Also gives correct benefit numbers.
  if ( buff.heroic_leap_movement->up() )
  {
    ms = std::max( buff.heroic_leap_movement->value(), ms );
  }
  else if ( buff.charge_movement->up() )
  {
    ms = std::max( buff.charge_movement->value(), ms );
  }
  else if ( buff.intervene_movement->up() )
  {
    ms = std::max( buff.intervene_movement->value(), ms );
  }
  else if ( buff.intercept_movement->up() )
  {
    ms = std::max( buff.intercept_movement->value(), ms );
  }
  else if ( buff.shield_charge_movement->up() )
  {
    ms = std::max( buff.shield_charge_movement->value(), ms );
  }
  else if ( buff.bounding_stride->up() )
  {
    ms = std::max( buff.bounding_stride->value(), ms );
  }
  return ms;
}

// warrior_t::invalidate_cache ==============================================

void warrior_t::invalidate_cache( cache_e c )
{
  parse_player_effects_t::invalidate_cache( c );

  if ( mastery.critical_block->ok() )
  {
    if ( c == CACHE_MASTERY )
    {
      parse_player_effects_t::invalidate_cache( CACHE_BLOCK );
      parse_player_effects_t::invalidate_cache( CACHE_CRIT_BLOCK );
      parse_player_effects_t::invalidate_cache( CACHE_ATTACK_POWER );
    }
    if ( c == CACHE_CRIT_CHANCE )
    {
      parse_player_effects_t::invalidate_cache( CACHE_PARRY );
    }
  }
  if ( c == CACHE_MASTERY && mastery.unshackled_fury->ok() )
  {
    parse_player_effects_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
  if ( c == CACHE_STRENGTH && spec.vanguard -> ok() )
  {
    parse_player_effects_t::invalidate_cache( CACHE_BONUS_ARMOR );
  }
  if ( c == CACHE_ARMOR && talents.warrior.armored_to_the_teeth->ok() && spec.vanguard->ok() )
  {
    parse_player_effects_t::invalidate_cache( CACHE_STRENGTH );
  }
  if ( c == CACHE_BONUS_ARMOR && talents.warrior.armored_to_the_teeth->ok() && spec.vanguard->ok() )
  {
    parse_player_effects_t::invalidate_cache( CACHE_STRENGTH );
  }
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role() const
{
  if ( parse_player_effects_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( parse_player_effects_t::primary_role() == ROLE_DPS || parse_player_effects_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( specialization() == WARRIOR_PROTECTION )
  {
    return ROLE_TANK;
  }
  return ROLE_ATTACK;
}

// warrior_t::convert_hybrid_stat ==============================================

stat_e warrior_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    case STAT_AGI_INT:
      return STAT_NONE;
    case STAT_STR_AGI_INT:
    case STAT_STR_AGI:
    case STAT_STR_INT:
      return STAT_STRENGTH;
    case STAT_SPIRIT:
      return STAT_NONE;
    case STAT_BONUS_ARMOR:
        return s;
    default:
      return s;
  }
}

void warrior_t::assess_damage( school_e school, result_amount_type type, action_state_t* s )
{
  parse_player_effects_t::assess_damage( school, type, s );

  if ( s->result == RESULT_DODGE || s->result == RESULT_PARRY )
  {
   if ( rppm.revenge->trigger() )
    {
      buff.revenge->trigger();
    }
  }

  // Generate 3 Rage on auto-attack taken.
  // TODO: Update with spelldata once it actually exists
  // Arma Feb 1 2023 - We check that damage was actually done, as otherwise, flasks that did not do damage were
  // giving us rage
  else if ( ! s -> action -> special && cooldown.rage_from_auto_attack->up() && s->result_raw > 0.0 )
  {
    resource_gain( RESOURCE_RAGE, 3.0, gain.rage_from_damage_taken, s -> action );
    cooldown.rage_from_auto_attack->start( cooldown.rage_from_auto_attack->duration );
  }

  if ( talents.protection.tough_as_nails->ok() && cooldown.tough_as_nails_icd -> up() &&
    ( s -> block_result == BLOCK_RESULT_BLOCKED || s -> block_result == BLOCK_RESULT_CRIT_BLOCKED ) &&
    s -> action -> player -> is_enemy() )
  {
    active.tough_as_nails -> execute_on_target( s -> action -> player );
  }
}

// warrior_t::target_mitigation ============================================

void warrior_t::target_mitigation( school_e school, result_amount_type dtype, action_state_t* s )
{
  parse_player_effects_t::target_mitigation( school, dtype, s );

  if ( s->result == RESULT_HIT || s->result == RESULT_CRIT || s->result == RESULT_GLANCE )
  {
    if ( buff.defensive_stance->up() )
    {
      s->result_amount *= 1.0 + buff.defensive_stance->data().effectN( 1 ).percent() + spec.protection_warrior->effectN( 17 ).percent();
    }

    if ( buff.defensive_stance->up() && talents.protection.fight_through_the_flames->ok() && talents.warrior.defensive_stance->effectN( 3 ).affected_schools() & school )
    {
      s->result_amount *= 1.0 + talents.protection.fight_through_the_flames->effectN( 1 ).percent();
    }

    warrior_td_t* td = get_target_data( s->action->player );

    if ( td->debuffs_demoralizing_shout->up() )
    {
      s->result_amount *= 1.0 + td->debuffs_demoralizing_shout->value();
    }

    if ( td -> debuffs_punish -> up() )
    {
      s -> result_amount *= 1.0 + td -> debuffs_punish -> value();
    }

    if ( school != SCHOOL_PHYSICAL && buff.spell_reflection->up() )
    {
      s -> result_amount *= 1.0 + buff.spell_reflection->data().effectN( 2 ).percent();
      buff.spell_reflection->expire();
    }
    // take care of dmg reduction CDs
    if ( buff.shield_wall->up() )
    {
      s->result_amount *= 1.0 + buff.shield_wall->value();
    }
    else if ( buff.die_by_the_sword->up() )
    {
      s->result_amount *= 1.0 + buff.die_by_the_sword->default_value;
    }

    if ( sets -> has_set_bonus( WARRIOR_PROTECTION, T31, B2 ) && buff.fervid_opposition -> up() )
    {
      s->result_amount *= 1.0 - sets -> set( WARRIOR_PROTECTION, T31, B2 )->effectN( 2 ).percent();
    }

    if ( specialization() == WARRIOR_PROTECTION )
      s->result_amount *= 1.0 + spec.vanguard -> effectN( 3 ).percent();

    if ( buff.vanguards_determination->up() )
      s->result_amount *= 1.0 + sets->set( WARRIOR_PROTECTION, T29, B2 )->effectN( 1 ).trigger()->effectN( 2 ).percent();
  }
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  parse_player_effects_t::create_options();

  add_option( opt_bool( "non_dps_mechanics", non_dps_mechanics ) );
  add_option( opt_bool( "warrior_fixed_time", warrior_fixed_time ) );
  add_option( opt_int( "into_the_fray_friends", into_the_fray_friends ) );
  add_option( opt_int( "never_surrender_percentage", never_surrender_percentage ) );
}

// warrior_t::create_profile ================================================

std::string warrior_t::create_profile( save_e type )
{
  if ( specialization() == WARRIOR_PROTECTION && primary_role() == ROLE_TANK )
  {
    position_str = "front";
  }

  return parse_player_effects_t::create_profile( type );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  parse_player_effects_t::copy_from( source );

  warrior_t* p = debug_cast<warrior_t*>( source );

  non_dps_mechanics     = p->non_dps_mechanics;
  warrior_fixed_time    = p->warrior_fixed_time;
  into_the_fray_friends = p->into_the_fray_friends;
  never_surrender_percentage = p -> never_surrender_percentage;
}

void warrior_t::parse_player_effects()
{
  parse_effects( spec.warrior );
  parse_effects( talents.warrior.wild_strikes );
  parse_effects( buff.wild_strikes, talents.warrior.wild_strikes );

  if ( specialization() == WARRIOR_ARMS )
  {
    parse_effects( spec.arms_warrior );
    parse_effects( buff.in_for_the_kill, USE_CURRENT );
  }
  else if ( specialization() == WARRIOR_FURY )
  {
    parse_effects( spec.fury_warrior );
    parse_effects( buff.dancing_blades );
    parse_effects( buff.frenzy );
    parse_effects( talents.fury.swift_strikes, effect_mask_t( false ).enable( 1 ) );

    if ( talents.fury.frenzied_enrage->ok() )
      parse_effects( buff.enrage, effect_mask_t( false ).enable( 1, 2 ) );
  }
  else if ( specialization() == WARRIOR_PROTECTION )
  {
    parse_effects( spec.protection_warrior );
    parse_effects( buff.battering_ram );
    parse_effects( talents.protection.enduring_alacrity, effect_mask_t( false ).enable( 1, 2 ) );
    parse_effects( buff.into_the_fray );
  }

  // Colossus

  // Slayer

  // Mountain Thane
  parse_effects( talents.mountain_thane.steadfast_as_the_peaks );
}

void warrior_t::apply_affecting_auras( action_t& action )
{
  parse_player_effects_t::apply_affecting_auras( action );

  // Spec Auras
  action.apply_affecting_aura( spec.warrior );
  action.apply_affecting_aura( spec.arms_warrior );
  action.apply_affecting_aura( spec.fury_warrior );
  action.apply_affecting_aura( spec.protection_warrior );

  // Arms Auras
  action.apply_affecting_aura( talents.arms.bloodborne );
  action.apply_affecting_aura( talents.arms.bloodletting );
  action.apply_affecting_aura( talents.arms.blunt_instruments ); // damage only
  action.apply_affecting_aura( talents.arms.impale );
  action.apply_affecting_aura( talents.arms.improved_overpower );
  action.apply_affecting_aura( talents.arms.improved_execute );
  action.apply_affecting_aura( talents.arms.sharpened_blades );
  action.apply_affecting_aura( talents.arms.strength_of_arms );
  action.apply_affecting_aura( talents.arms.valor_in_victory );

  // Fury Auras
  action.apply_affecting_aura( talents.fury.bloodborne );
  action.apply_affecting_aura( talents.fury.critical_thinking );
  action.apply_affecting_aura( talents.fury.improved_bloodthirst );
  action.apply_affecting_aura( talents.fury.improved_raging_blow );
  action.apply_affecting_aura( talents.fury.meat_cleaver );
  action.apply_affecting_aura( talents.fury.storm_of_steel );
  action.apply_affecting_aura( talents.fury.titanic_rage );

  // Protection Auras
  action.apply_affecting_aura( talents.protection.storm_of_steel );
  action.apply_affecting_aura( talents.protection.bloodborne );
  action.apply_affecting_aura( talents.protection.defenders_aegis );
  action.apply_affecting_aura( talents.protection.battering_ram );

  // Shared Auras
  action.apply_affecting_aura( talents.warrior.barbaric_training );
  action.apply_affecting_aura( talents.warrior.champions_might );
  action.apply_affecting_aura( talents.warrior.concussive_blows );
  action.apply_affecting_aura( talents.warrior.crackling_thunder );
  action.apply_affecting_aura( talents.warrior.cruel_strikes );
  action.apply_affecting_aura( talents.warrior.crushing_force );
  action.apply_affecting_aura( talents.warrior.piercing_challenge );
  action.apply_affecting_aura( talents.warrior.honed_reflexes );
  action.apply_affecting_aura( talents.warrior.thunderous_words );
  action.apply_affecting_aura( talents.warrior.uproar );

  // set bonus
  action.apply_affecting_aura( tier_set.t29_arms_2pc );
  action.apply_affecting_aura( tier_set.t29_fury_2pc );
  action.apply_affecting_aura( tier_set.t30_fury_2pc );
  action.apply_affecting_aura( tier_set.t31_arms_2pc );
  action.apply_affecting_aura( tier_set.t31_fury_2pc );

  if ( specialization() == WARRIOR_FURY && main_hand_weapon.group() == WEAPON_1H &&
             off_hand_weapon.group() == WEAPON_1H && talents.fury.single_minded_fury->ok() )
  {
    action.apply_affecting_aura( talents.fury.single_minded_fury );
  }
  if ( specialization() == WARRIOR_FURY && talents.warrior.dual_wield_specialization->ok() )
  {
    action.apply_affecting_aura( talents.warrior.dual_wield_specialization );
  }
  if ( specialization() == WARRIOR_ARMS && talents.warrior.two_handed_weapon_specialization->ok() )
  {
    action.apply_affecting_aura( talents.warrior.two_handed_weapon_specialization );
  }
  if ( specialization() == WARRIOR_PROTECTION && talents.warrior.one_handed_weapon_specialization->ok() )
  {
    action.apply_affecting_aura( talents.warrior.one_handed_weapon_specialization );
  }

  // Colossus
  action.apply_affecting_aura( talents.colossus.martial_expert );
  action.apply_affecting_aura( talents.colossus.earthquaker );
  action.apply_affecting_aura( talents.colossus.mountain_of_muscle_and_scars );

  // Slayer
  action.apply_affecting_aura( talents.slayer.slayers_malice );

  // Mountain Thane
  action.apply_affecting_aura( talents.mountain_thane.strength_of_the_mountain );
  action.apply_affecting_aura( talents.mountain_thane.storm_bolts );
  action.apply_affecting_aura( talents.mountain_thane.thorims_might );
  if ( specialization() == WARRIOR_PROTECTION )
  {
    action.apply_affecting_aura( talents.mountain_thane.thunder_blast );
    // Effect 2 is not properly flagged as Protection only in Spell Data. Effect 1 & 3 are manually handled elsewhere.
  }
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warrior_report_t : public player_report_extension_t
{
public:
  warrior_report_t( warrior_t& player ) : p( player )
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

      action_t* a          = p.find_action( entry->first );
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
    // Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
    if ( !p.cd_waste_exec.empty() )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown waste details</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      cdwaste_table_header( os );
      cdwaste_table_contents( os );
      cdwaste_table_footer( os );

      os << "\t\t\t\t\t</div>\n";

      os << "<div class=\"clear\"></div>\n";
    }
    p.parsed_effects_html( os );
    os << "\t\t\t\t\t</div>\n";
  }

private:
  warrior_t& p;
};

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p              = new warrior_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new warrior_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_hotfixes() const override
  {
  }

  void init( player_t* p ) const override
  {
        p->buffs.conquerors_banner = make_buff<stat_buff_t>( p, "conquerors_banner_external", p->find_spell( 325862 ) );
        p->buffs.rallying_cry = make_buff<buffs::rallying_cry_t>( p );
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};
}  // UNNAMED NAMESPACE

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}
