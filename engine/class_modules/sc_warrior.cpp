// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace
{  // UNNAMED NAMESPACE
// ==========================================================================
// Warrior
// To Do: Clean up green text
// Fury - Gathering Storm tick behavior - Fury needs 2 more
// Arms - Clean up Crushing Assault (Whirlwind Fervor)
// ==========================================================================

struct warrior_t;

struct warrior_td_t : public actor_target_data_t
{
  dot_t* dots_deep_wounds;
  dot_t* dots_gushing_wound;
  dot_t* dots_ravager;
  dot_t* dots_rend;
  buff_t* debuffs_colossus_smash;
  buff_t* debuffs_siegebreaker;
  buff_t* debuffs_demoralizing_shout;
  buff_t* debuffs_taunt;
  buff_t* debuffs_punish;
  buff_t* debuffs_callous_reprisal;

  warrior_t& warrior;
  warrior_td_t( player_t* target, warrior_t& p );
};

using data_t        = std::pair<std::string, simple_sample_data_with_min_max_t>;
using simple_data_t = std::pair<std::string, simple_sample_data_t>;

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

struct warrior_t : public player_t
{
public:
  event_t *heroic_charge, *rampage_driver;
  std::vector<attack_t*> rampage_attacks;
  bool non_dps_mechanics, warrior_fixed_time;
  int into_the_fray_friends;
  int never_surrender_percentage;
  double expected_max_health;

  auto_dispose<std::vector<data_t*> > cd_waste_exec, cd_waste_cumulative;
  auto_dispose<std::vector<simple_data_t*> > cd_waste_iter;

  // Active
  struct active_t
  {
    action_t* deep_wounds_ARMS;
    action_t* deep_wounds_PROT;
    action_t* charge;
    action_t* slaughter;  // Fury T21 2PC
    action_t* iron_fortress; // Prot azerite trait                             
    action_t* bastion_of_might_ip; // 0 rage IP from Bastion of Might azerite trait
  } active;

  // Buffs
  struct buffs_t
  {
    buff_t* avatar;
    buff_t* ayalas_stone_heart;
    buff_t* deadly_calm;
    buff_t* bastion_of_might;     // the mastery buff
    buff_t* berserker_rage;
    buff_t* bladestorm;
    buff_t* bounding_stride;
    buff_t* charge_movement;
    buff_t* defensive_stance;
    buff_t* die_by_the_sword;
    buff_t* enrage;
    buff_t* frothing_berserker;
    buff_t* furious_slash;
    buff_t* furious_charge;
    buff_t* heroic_leap_movement;
    buff_t* ignore_pain;
    buff_t* intercept_movement;
    buff_t* intervene_movement;
    buff_t* into_the_fray;
    buff_t* last_stand;
    buff_t* meat_cleaver;
    buff_t* overpower;
    buff_t* rallying_cry;
    buff_t* ravager;
    buff_t* ravager_protection;
    buff_t* recklessness;
    buff_t* revenge;
    buff_t* shield_block;
    buff_t* shield_wall;
    buff_t* spell_reflection;
    buff_t* sudden_death;
    buff_t* sweeping_strikes;
    buff_t* vengeance_revenge;
    buff_t* vengeance_ignore_pain;
    buff_t* whirlwind;
    buff_t* tornados_eye;
    buff_t* protection_rage;

    buff_t* raging_thirst;
    buff_t* t20_fury_4p;

    // Legendary Items
    buff_t* fujiedas_fury;
    buff_t* xavarics_magnum_opus;
    buff_t* sephuzs_secret;
    buff_t* in_for_the_kill;
    buff_t* war_veteran;     // Arms T21 2PC
    buff_t* weighted_blade;  // Arms T21 4PC
    // Azerite Traits
    buff_t* bloodcraze;
    buff_t* bloodcraze_driver;
    buff_t* bloodsport;
    buff_t* brace_for_impact;
    buff_t* crushing_assault;
    buff_t* executioners_precision;
    buff_t* gathering_storm;
    buff_t* infinite_fury;
    buff_t* pulverizing_blows;
    buff_t* striking_the_anvil;
    buff_t* test_of_might_tracker;  // Used to track rage gain from test of might.
    stat_buff_t* test_of_might;
    buff_t* trample_the_weak;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* avatar;
    cooldown_t* recklessness;
    cooldown_t* berserker_rage;
    cooldown_t* bladestorm;
    cooldown_t* bloodthirst;
    cooldown_t* charge;
    cooldown_t* colossus_smash;
    cooldown_t* deadly_calm;
    cooldown_t* demoralizing_shout;
    cooldown_t* dragon_roar;
    cooldown_t* enraged_regeneration;
    cooldown_t* execute;
    cooldown_t* heroic_leap;
    cooldown_t* iron_fortress_icd;
    cooldown_t* last_stand;
    cooldown_t* mortal_strike;
    cooldown_t* overpower;
    cooldown_t* rage_from_auto_attack;
    cooldown_t* rage_from_crit_block;
    cooldown_t* rage_of_the_valarjar_icd;
    cooldown_t* raging_blow;
    cooldown_t* ravager;
    cooldown_t* revenge_reset;
    cooldown_t* shield_slam;
    cooldown_t* shield_wall;
    cooldown_t* shockwave;
    cooldown_t* siegebreaker;
    cooldown_t* skullsplitter;
    cooldown_t* storm_bolt;
    cooldown_t* warbreaker;
  } cooldown;

  // Gains
  struct gains_t
  {
    gain_t* archavons_heavy_hand;
    gain_t* avoided_attacks;
    gain_t* critical_block;
    gain_t* meat_cleaver;
    gain_t* melee_crit;
    gain_t* melee_main_hand;
    gain_t* melee_off_hand;
    gain_t* raging_blow;
    gain_t* revenge;
    gain_t* shield_slam;
    gain_t* whirlwind;
    gain_t* booming_voice;
    gain_t* thunder_clap;
    gain_t* protection_t20_2p;
    gain_t* endless_rage;
    gain_t* collateral_damage;

    // Legendarys, Azerite, and Special Effects
    gain_t* execute_refund;
    gain_t* rage_from_damage_taken;
    gain_t* ravager;
    gain_t* ceannar_rage;
    gain_t* cold_steel_hot_blood;
    gain_t* valarjar_berserking;
    gain_t* lord_of_war;
    gain_t* simmering_rage;
  } gain;

  // Spells
  struct spells_t
  {
    const spell_data_t* battle_shout;
    const spell_data_t* charge;
    const spell_data_t* colossus_smash_debuff;
    const spell_data_t* headlong_rush;
    const spell_data_t* heroic_leap;
    const spell_data_t* intervene;
    const spell_data_t* siegebreaker_debuff;
    const spell_data_t* whirlwind_buff;
    const spell_data_t* ravager_protection;
    const spell_data_t* shield_block_buff;
    const spell_data_t* riposte;
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
    proc_t* delayed_auto_attack;
    proc_t* tactician;

    // Tier bonuses
    proc_t* t19_2pc_arms;
  } proc;

  // Spec Passives
  struct spec_t
  {
    const spell_data_t* spell_reflection;
    const spell_data_t* recklessness;
    const spell_data_t* bladestorm;
    const spell_data_t* bloodthirst;
    const spell_data_t* colossus_smash;
    const spell_data_t* deep_wounds_ARMS;
    const spell_data_t* deep_wounds_PROT;
    const spell_data_t* demoralizing_shout;
    const spell_data_t* devastate;
    const spell_data_t* die_by_the_sword;
    const spell_data_t* enrage;
    const spell_data_t* enraged_regeneration;
    const spell_data_t* execute;
    const spell_data_t* execute_2;
    const spell_data_t* hamstring;
    const spell_data_t* ignore_pain;
    const spell_data_t* intercept;
    const spell_data_t* last_stand;
    const spell_data_t* mortal_strike;
    const spell_data_t* overpower;
    const spell_data_t* piercing_howl;
    const spell_data_t* raging_blow;
    const spell_data_t* rallying_cry;
    const spell_data_t* rampage;
    const spell_data_t* revenge;
    const spell_data_t* seasoned_soldier;
    const spell_data_t* shield_block;
    const spell_data_t* shield_block_2;
    const spell_data_t* shield_slam;
    const spell_data_t* shield_slam_2;
    const spell_data_t* shield_wall;
    const spell_data_t* slam;
    const spell_data_t* shockwave;
    const spell_data_t* sweeping_strikes;
    const spell_data_t* tactician;
    const spell_data_t* thunder_clap;
    const spell_data_t* whirlwind;
    const spell_data_t* whirlwind_2;
    const spell_data_t* revenge_trigger;
    const spell_data_t* berserker_rage;
    const spell_data_t* victory_rush;
    const spell_data_t* arms_warrior;
    const spell_data_t* fury_warrior;
    const spell_data_t* prot_warrior;
    const spell_data_t* avatar;
    const spell_data_t* vanguard;
  } spec;

  // Talents
  struct talents_t
  {
    // Arms
    const spell_data_t* war_machine;
    const spell_data_t* sudden_death;
    const spell_data_t* skullsplitter;

    const spell_data_t* double_time;
    const spell_data_t* impending_victory;
    const spell_data_t* storm_bolt;

    const spell_data_t* massacre;
    const spell_data_t* fervor_of_battle;
    const spell_data_t* rend;

    const spell_data_t* second_wind;  // NYI
    const spell_data_t* bounding_stride;
    const spell_data_t* defensive_stance;

    const spell_data_t* collateral_damage;
    const spell_data_t* warbreaker;
    const spell_data_t* cleave;

    const spell_data_t* in_for_the_kill;
    const spell_data_t* avatar;
    const spell_data_t* deadly_calm;

    const spell_data_t* anger_management;
    const spell_data_t* dreadnaught;
    const spell_data_t* ravager;

    // Fury
    // const spell_data_t* war_machine; see arms
    const spell_data_t* endless_rage;
    const spell_data_t* fresh_meat;
    
    // skipping t30 identital to arms

    const spell_data_t* inner_rage;
    // const spell_data_t* sudden_death; see arms
    const spell_data_t* furious_slash;

    const spell_data_t* furious_charge;
    // const spell_data_t* bounding_stride; see arms
    const spell_data_t* warpaint;

    const spell_data_t* carnage;
    // const spell_data_t* massacre; see arms
    const spell_data_t* frothing_berserker;

    const spell_data_t* meat_cleaver;
    const spell_data_t* dragon_roar;
    const spell_data_t* bladestorm;

    const spell_data_t* reckless_abandon;
    // const spell_data_t* anger_management; see arms
    const spell_data_t* siegebreaker;

    // Protection
    const spell_data_t* into_the_fray;
    const spell_data_t* punish;
    // const spell_data_t* impending_victory; see arms
    
    const spell_data_t* crackling_thunder;
    // const spell_data_t* bounding_stride; see arms
    const spell_data_t* safeguard; // NYI

    const spell_data_t* best_served_cold;
    const spell_data_t* unstoppable_force;
    // const spell_data_t* dragon_roar; see fury

    const spell_data_t* indomitable;
    const spell_data_t* never_surrender;  // SORTA NYI
    const spell_data_t* bolster;

    const spell_data_t* menace; // NYI
    const spell_data_t* rumbling_earth;
    // const spell_data_t* storm_bolt; see arms

    const spell_data_t* booming_voice;
    const spell_data_t* vengeance;
    const spell_data_t* devastator;

    // const spell_data_t* anger_management; see arms
    const spell_data_t* heavy_repercussions;
    // const spell_data_t* ravager; see arms
  } talents;

  struct legendary_t
  {
    const spell_data_t* sephuzs_secret;
    const spell_data_t* valarjar_berserkers;
    const spell_data_t* ceannar_charger;
    const spell_data_t* archavons_heavy_hand;
    const spell_data_t* kazzalax_fujiedas_fury;
    const spell_data_t* prydaz_xavarics_magnum_opus;
    const spell_data_t* najentuss_vertebrae;
    const spell_data_t* ayalas_stone_heart;
    const spell_data_t* weight_of_the_earth;
    const spell_data_t* raging_fury;
    const spell_data_t* the_great_storms_eye;

    legendary_t()
      : sephuzs_secret( spell_data_t::not_found() ),
        valarjar_berserkers( spell_data_t::not_found() ),
        ceannar_charger( spell_data_t::not_found() ),
        archavons_heavy_hand( spell_data_t::not_found() ),
        kazzalax_fujiedas_fury( spell_data_t::not_found() ),
        prydaz_xavarics_magnum_opus( spell_data_t::not_found() ),
        najentuss_vertebrae( spell_data_t::not_found() ),
        ayalas_stone_heart( spell_data_t::not_found() ),
        weight_of_the_earth( spell_data_t::not_found() ),
        raging_fury( spell_data_t::not_found() ),
        the_great_storms_eye( spell_data_t::not_found() )
    {
    }

  } legendary;
  
  // Azerite traits
  struct
  {
    // All
    azerite_power_t breach;
    azerite_power_t moment_of_glory;
    azerite_power_t bury_the_hatchet;
    // Prot
    azerite_power_t iron_fortress;
    azerite_power_t deafening_crash;
    azerite_power_t callous_reprisal;
    azerite_power_t brace_for_impact;
    azerite_power_t bloodsport;
    azerite_power_t bastion_of_might;
    // Arms
    azerite_power_t test_of_might;
    azerite_power_t seismic_wave;
    azerite_power_t lord_of_war;
    azerite_power_t gathering_storm;
    azerite_power_t executioners_precision;
    azerite_power_t crushing_assault;
    azerite_power_t striking_the_anvil;
    // fury
    azerite_power_t trample_the_weak;
    azerite_power_t simmering_rage;
    azerite_power_t reckless_flurry;
    azerite_power_t pulverizing_blows;
    azerite_power_t infinite_fury;
    azerite_power_t bloodcraze;
    azerite_power_t cold_steel_hot_blood;
    azerite_power_t unbridled_ferocity;
  } azerite;

  // Default consumables
  std::string default_potion() const override;
  std::string default_flask() const override;
  std::string default_food() const override;
  std::string default_rune() const override;

  warrior_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF )
    : player_t( sim, WARRIOR, name, r ),
      heroic_charge( nullptr ),
      rampage_driver( nullptr ),
      rampage_attacks( 0 ),
      active(),
      buff(),
      cooldown(),
      gain(),
      spell(),
      mastery(),
      proc(),
      spec(),
      talents(),
      legendary(),
      azerite()
  {
    non_dps_mechanics =
        false;  // When set to false, disables stuff that isn't important, such as second wind, bloodthirst heal, etc.
    warrior_fixed_time    = true;
    into_the_fray_friends = -1;
    never_surrender_percentage = 70;
    expected_max_health   = 0;

    regen_type = REGEN_DISABLED;

    talent_points.register_validity_fn( [this]( const spell_data_t* spell ) {
      // Soul of the Battlelord
      if ( find_item_by_id( 151650 ) )
      {
        switch ( specialization() )
        {
          case WARRIOR_FURY:
            return spell->id() == 206315;  // Massacre
          case WARRIOR_ARMS:
            return spell->id() == 152278;  // Anger Management
          case WARRIOR_PROTECTION:
            return spell->id() == 202572;  // Vengeance
          default:
            // This shouldn't happen
            break;
        }
      }
      return false;
    } );
  }

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void init_scaling() override;
  void create_buffs() override;
  void init_gains() override;
  void init_position() override;
  void init_procs() override;
  void init_resources( bool ) override;
  void arise() override;
  void combat_begin() override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_rating_multiplier( rating_e rating ) const override;
  double composite_player_multiplier( school_e school ) const override;
  // double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_melee_haste() const override;
  // double composite_armor_multiplier() const override;
  double composite_bonus_armor() const override;
  double composite_block() const override;
  double composite_block_reduction( action_state_t* s ) const override;
  double composite_parry_rating() const override;
  double composite_parry() const override;
  double composite_melee_expertise( const weapon_t* ) const override;
  double composite_attack_power_multiplier() const override;
  // double composite_melee_attack_power() const override;
  double composite_mastery() const override;
  double composite_crit_block() const override;
  double composite_crit_avoidance() const override;
  // double composite_melee_speed() const override;
  double composite_melee_crit_chance() const override;
  double composite_melee_crit_rating() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  // double composite_leech() const override;
  double resource_gain( resource_e, double, gain_t* = nullptr, action_t* = nullptr ) override;
  void teleport( double yards, timespan_t duration ) override;
  void trigger_movement( double distance, movement_direction_e direction ) override;
  void interrupt() override;
  void reset() override;
  void moving() override;
  void create_options() override;
  std::string create_profile( save_e type ) override;
  void invalidate_cache( cache_e ) override;
  double temporary_movement_modifier() const override;

  void default_apl_dps_precombat();
  void apl_default();
  void apl_fury();
  void apl_arms();
  void apl_prot();
  void init_action_list() override;

  action_t* create_action( const std::string& name, const std::string& options ) override;
  void activate() override;
  resource_e primary_resource() const override
  {
    return RESOURCE_RAGE;
  }
  role_e primary_role() const override;
  stat_e convert_hybrid_stat( stat_e s ) const override;
  // void assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* s ) override;
  // void assess_damage_imminent( school_e, dmg_e, action_state_t* s ) override;
  void assess_damage( school_e, dmg_e, action_state_t* ) override;
  void target_mitigation( school_e, dmg_e, action_state_t* ) override;
  void copy_from( player_t* ) override;
  void merge( player_t& ) override;

  void datacollection_begin() override;
  void datacollection_end() override;

  target_specific_t<warrior_td_t> target_data;

  warrior_td_t* get_target_data( player_t* target ) const override
  {
    warrior_td_t*& td = target_data[ target ];

    if ( !td )
    {
      td = new warrior_td_t( target, const_cast<warrior_t&>( *this ) );
    }
    return td;
  }

  void enrage()
  {
    buff.enrage->trigger();
    if ( legendary.ceannar_charger->found() )
    {
      resource_gain( RESOURCE_RAGE,
                     legendary.ceannar_charger->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ),
                     gain.ceannar_rage );
    }

    if ( talents.endless_rage->ok() )
    {
      double gain_amount = talents.endless_rage->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE );
      resource_gain( RESOURCE_RAGE, gain_amount, gain.endless_rage );
    }
  }
};

namespace
{  // UNNAMED NAMESPACE
// Template for common warrior action code. See priest_action_t.
template <class Base>
struct warrior_action_t : public Base
{
  struct affected_by_t
  {
    // mastery/buff damage increase.
    bool fury_mastery_direct, fury_mastery_dot;
    // talents
    bool avatar, frothing_direct, frothing_dot, sweeping_strikes, deadly_calm, booming_voice;
    // azerite
    bool crushing_assault;

    affected_by_t()
      : fury_mastery_direct( false ),
        fury_mastery_dot( false ),
        avatar( false ),
        frothing_direct( false ),
        frothing_dot( false ),
        sweeping_strikes( false ),
        deadly_calm( false ),
        crushing_assault( false ),
        booming_voice( false )
    {
    }
  } affected_by;

  double tactician_per_rage;

private:
  typedef Base ab;  // action base, eg. spell_t
public:
  typedef warrior_action_t base_t;
  bool track_cd_waste;
  simple_sample_data_with_min_max_t *cd_wasted_exec, *cd_wasted_cumulative;
  simple_sample_data_t* cd_wasted_iter;
  bool initialized;
  warrior_action_t( const std::string& n, warrior_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      tactician_per_rage( 0 ),
      track_cd_waste( s->cooldown() > timespan_t::zero() || s->charge_cooldown() > timespan_t::zero() ),
      cd_wasted_exec( nullptr ),
      cd_wasted_cumulative( nullptr ),
      cd_wasted_iter( nullptr ),
      initialized( false )
  {
    ab::may_crit = true;
    tactician_per_rage += ( player->spec.tactician->effectN( 1 ).percent() / 100 );
  }

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

    if ( ab::data().affected_by( p()->spec.fury_warrior->effectN( 1 ) ) )
      ab::base_dd_multiplier *= 1.0 + p()->spec.fury_warrior->effectN( 1 ).percent();
    if ( ab::data().affected_by( p()->spec.fury_warrior->effectN( 2 ) ) )
      ab::base_td_multiplier *= 1.0 + p()->spec.fury_warrior->effectN( 2 ).percent();

    if ( ab::data().affected_by( p()->spec.arms_warrior->effectN( 2 ) ) )
      ab::base_dd_multiplier *= 1.0 + p()->spec.arms_warrior->effectN( 2 ).percent();
    if ( ab::data().affected_by( p()->spec.arms_warrior->effectN( 3 ) ) )
      ab::base_td_multiplier *= 1.0 + p()->spec.arms_warrior->effectN( 3 ).percent();

    if ( ab::data().affected_by( p()->spec.prot_warrior->effectN( 1 ) ) )
      ab::base_dd_multiplier *= 1.0 + p()->spec.prot_warrior->effectN( 1 ).percent();
    if ( ab::data().affected_by( p()->spec.prot_warrior->effectN( 2 ) ) )
      ab::base_td_multiplier *= 1.0 + p()->spec.prot_warrior->effectN( 2 ).percent();

    ab::cooldown->hasted = ab::data().affected_by( p()->spell.headlong_rush->effectN( 1 ) );
    if ( ab::cooldown->hasted == false )  // Shield block is on a different effect for some reason.
      ab::cooldown->hasted = ab::data().affected_by( p()->spell.headlong_rush->effectN( 3 ) );
    if ( ab::data().affected_by( p()->spell.headlong_rush->effectN( 2 ) ) )
      ab::gcd_haste = HASTE_ATTACK;

    affected_by.sweeping_strikes    = ab::data().affected_by( p()->spec.sweeping_strikes->effectN( 1 ) );
    affected_by.deadly_calm         = ab::data().affected_by( p()->talents.deadly_calm->effectN( 1 ) );
    affected_by.fury_mastery_direct = ab::data().affected_by( p()->mastery.unshackled_fury->effectN( 1 ) );
    affected_by.fury_mastery_dot    = ab::data().affected_by( p()->mastery.unshackled_fury->effectN( 2 ) );
    affected_by.booming_voice       = ab::data().affected_by( p()->spec.demoralizing_shout->effectN( 3 ) );

    if ( p()->specialization() == WARRIOR_PROTECTION )
      affected_by.avatar              = ab::data().affected_by( p()->spec.avatar->effectN( 1 ) );
    else
      affected_by.avatar              = ab::data().affected_by( p()->talents.avatar->effectN( 1 ) );

    affected_by.frothing_direct =
        ab::data().affected_by( p()->talents.frothing_berserker->effectN( 1 ).trigger()->effectN( 1 ) );
    affected_by.frothing_dot =
        ab::data().affected_by( p()->talents.frothing_berserker->effectN( 1 ).trigger()->effectN( 3 ) );

    initialized = true;
  }

  virtual ~warrior_action_t()
  {
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
    if ( ab::sim->log )
    {
      ab::sim->out_debug.printf(
          "Rage used to calculate tactician chance from ability %s: %4.4f, actual rage used: %4.4f", ab::name(),
          ab::cost(), base_t::cost() );
    }
    return ab::cost();
  }

  int n_targets() const override
  {
    if ( affected_by.sweeping_strikes && p()->buff.sweeping_strikes->check() )
    {
      return static_cast<int>( 1 + p()->spec.sweeping_strikes->effectN( 1 ).base_value() );
    }

    return ab::n_targets();
  }

  double cost() const override
  {
    double c = ab::cost();

    if ( p()->buff.deadly_calm->check() && affected_by.deadly_calm )
    {
      c *= 1.0 + p()->talents.deadly_calm->effectN( 1 ).percent();
    }
    return c;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = ab::composite_target_multiplier( target );

    warrior_td_t* td = p()->get_target_data( target );

    if ( td->debuffs_colossus_smash->check() )
    {
      m *= 1.0 + ( td->debuffs_colossus_smash->value() );
    }

    if ( td->debuffs_siegebreaker->check() )
    {
      m *= 1.0 + ( td->debuffs_siegebreaker->value() );
    }

    if ( td -> debuffs_demoralizing_shout -> up() &&
         p() -> talents.booming_voice -> ok() && 
         affected_by.booming_voice )
    {
      m *= 1.0 + p() -> talents.booming_voice->effectN( 2 ).percent();
    }

    return m;
  }

  virtual double composite_da_multiplier( const action_state_t* s ) const override
  {
    double dm = ab::composite_da_multiplier( s );

    if ( affected_by.fury_mastery_direct && p()->buff.enrage->up() )
    {
      dm *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.avatar && p()->buff.avatar->up() )
    {
      dm *= 1.0 + p()->buff.avatar->data().effectN( 1 ).percent();
    }

    if ( affected_by.frothing_direct && p()->buff.frothing_berserker->up() )
    {
      dm *= 1.0 + p()->buff.frothing_berserker->data().effectN( 1 ).percent();
    }

    if ( affected_by.sweeping_strikes && s->chain_target > 0 )
    {
      dm *= p()->spec.sweeping_strikes->effectN( 2 ).percent();
    }

    return dm;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double tm = ab::composite_ta_multiplier( s );

    if ( affected_by.fury_mastery_dot && p()->buff.enrage->up() )
    {
      tm *= 1.0 + p()->cache.mastery_value();
    }

    if ( affected_by.frothing_dot && p()->buff.frothing_berserker->up() )
    {
      tm *= 1.0 + p()->buff.frothing_berserker->data().effectN( 3 ).percent();
    }

    return tm;
  }

  void execute() override
  {
    ab::execute();
  }

  bool ready() override
  {
    if ( !ab::ready() )
    {
      return false;
    }
    if ( p()->current.distance_to_move > ab::range && ab::range != -1 )
    {
      // -1 melee range implies that the ability can be used at any distance from the target.
      return false;
    }
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

    if ( p()->talents.collateral_damage->ok() && affected_by.sweeping_strikes && p()->buff.sweeping_strikes->up() &&
         s->chain_target == 1 )
    {
      p()->resource_gain( RESOURCE_RAGE,
                          ( ab::last_resource_cost * p()->talents.collateral_damage->effectN( 1 ).percent() ),
                          p()->gain.collateral_damage );
    }
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
    if ( tactician_per_rage )
    {
      tactician();
    }
   
    ab::consume_resource();

    double rage = ab::last_resource_cost;

    if ( p()->buff.test_of_might_tracker->check() )
      p()->buff.test_of_might_tracker->current_value +=
          ab::cost();  // Uses rage cost before deadly calm makes it cheaper.

    if ( p()->talents.anger_management->ok() )
    {
      anger_management( rage );
    }
    if ( ab::result_is_miss( ab::execute_state->result ) && rage > 0 && !ab::aoe )
    {
      p()->resource_gain( RESOURCE_RAGE, rage * 0.8, p()->gain.avoided_attacks );
    }
  }

  virtual void tactician()
  {
    double tact_rage = tactician_cost();  // Tactician resets based on cost before things make it cost less.
    if ( ab::rng().roll( tactician_per_rage * tact_rage ) )
    {
      p()->cooldown.overpower->reset( true );
      p()->proc.tactician->occur();
      if ( p()->azerite.striking_the_anvil.ok() )
      {
        p()->buff.striking_the_anvil->trigger();
      }
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
        cd_time_reduction /= p()->talents.anger_management->effectN( 3 ).base_value();
        p()->cooldown.recklessness->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }

      else if ( p()->specialization() == WARRIOR_ARMS )
      {
        cd_time_reduction /= p()->talents.anger_management->effectN( 1 ).base_value();  // Doesn't actually affect
                                                                                        // Ravager, although they can be
                                                                                        // used together with Soul of
                                                                                        // the Battelord
        p()->cooldown.colossus_smash->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.warbreaker->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.bladestorm->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }

      else if ( p()->specialization() == WARRIOR_PROTECTION )
      {
        cd_time_reduction /= p()->talents.anger_management->effectN( 2 ).base_value();
        p()->cooldown.last_stand->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.shield_wall->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.demoralizing_shout->adjust( timespan_t::from_seconds( cd_time_reduction ) );
        p()->cooldown.avatar->adjust( timespan_t::from_seconds( cd_time_reduction ) );
      }
    }
  }
};

struct warrior_heal_t : public warrior_action_t<heal_t>
{  // Main Warrior Heal Class
  warrior_heal_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
    may_crit = tick_may_crit = hasted_ticks = false;
    target                                  = p;
  }
};

struct warrior_spell_t : public warrior_action_t<spell_t>
{  // Main Warrior Spell Class - Used for spells that deal no damage, usually buffs.
  warrior_spell_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, p, s )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }
};

struct warrior_attack_t : public warrior_action_t<melee_attack_t>
{  // Main Warrior Attack Class
  warrior_attack_t( const std::string& n, warrior_t* p, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, p, s )
  {
    special = true;
  }

  virtual void execute()
  {
    base_t::execute();
    if ( !special )  // Procs below only trigger on special attacks, not autos
      return;

    if ( p()->talents.sudden_death->ok() && p()->buff.sudden_death->trigger() )
    {
      p()->cooldown.execute->reset( true );
    }

    if ( p()->buff.ayalas_stone_heart->trigger() )
    {
      p()->cooldown.execute->reset( true );
    }

    if ( affected_by.crushing_assault )
    {
      p()->buff.crushing_assault->expire();
    }
    p()->buff.crushing_assault->trigger();
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
      return targets.size() ? targets[ random_idx ] : nullptr;
    }
    else
    {
      auto random_idx = rng().range( size_t(), sim->target_non_sleeping_list.size() );
      return sim->target_non_sleeping_list[ random_idx ];
    }
  }
};

// Devastate ================================================================

struct devastate_t : public warrior_attack_t
{
  double shield_slam_reset;
  devastate_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "devastate", p, p->spec.devastate ),
      shield_slam_reset( p->spec.shield_slam_2->effectN( 1 ).percent() )
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
    }
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() || p() -> talents.devastator -> ok() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Melee Attack =============================================================

struct reckless_flurry_t : warrior_attack_t
{
  reckless_flurry_t( warrior_t* p )
    : warrior_attack_t( "reckless_flurry", p, p->find_spell( 283810 ) )
  {
    background  = true;
    base_dd_min = base_dd_max = p->azerite.reckless_flurry.value( 2 );
  }
};

struct devastator_t : warrior_attack_t
{
  double shield_slam_reset;

  devastator_t( warrior_t* p ) :
    warrior_attack_t( "devastator", p, p -> talents.devastator -> effectN( 1 ).trigger() ),
    shield_slam_reset( p -> talents.devastator -> effectN( 2 ).percent() )
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
  }
};

struct melee_t : public warrior_attack_t
{
  warrior_attack_t* reckless_flurry;
  bool mh_lost_melee_contact, oh_lost_melee_contact;
  double base_rage_generation, arms_rage_multiplier, fury_rage_multiplier, seasoned_soldier_crit_mult;
  devastator_t* devastator;
  melee_t( const std::string& name, warrior_t* p )
    : warrior_attack_t( name, p, spell_data_t::nil() ), reckless_flurry( nullptr ),
      mh_lost_melee_contact( true ),
      oh_lost_melee_contact( true ),
      // arms and fury multipliers are both 1, adjusted by the spec scaling auras (x4 for Arms and x1 for Fury)
      base_rage_generation( 1.75 ),
      arms_rage_multiplier( 4.00 ),
      fury_rage_multiplier( 1.00 ),
      seasoned_soldier_crit_mult( p->spec.seasoned_soldier->effectN( 1 ).percent() ),
      devastator( nullptr )
  {
    school     = SCHOOL_PHYSICAL;
    special    = false;
    background = repeating = may_glance = true;
    trigger_gcd                         = timespan_t::zero();
    weapon_multiplier                   = 1.0;
    if ( p->dual_wield() )
    {
      base_hit -= 0.19;
    }
    if ( p->talents.devastator->ok() )
    {
      devastator = new devastator_t( p );
      add_child( devastator );
    }
    if ( p->azerite.reckless_flurry.ok() )
    {
      reckless_flurry = new reckless_flurry_t( p );
    }
  }

  void init() override
  {
    warrior_attack_t::init();
    affected_by.fury_mastery_direct = p()->mastery.unshackled_fury->ok();
    affected_by.avatar              = p()->talents.avatar->ok();
    if ( p()->specialization() == WARRIOR_PROTECTION )
      affected_by.avatar              = p()->spec.avatar->ok();
    else
      affected_by.avatar              = p()->talents.avatar->ok();
    affected_by.frothing_direct     = p()->talents.frothing_berserker->ok();
    affected_by.booming_voice       = p() -> talents.booming_voice -> ok();
  }

  void reset() override
  {
    warrior_attack_t::reset();
    mh_lost_melee_contact = oh_lost_melee_contact = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = warrior_attack_t::execute_time();
    if ( weapon->slot == SLOT_MAIN_HAND && mh_lost_melee_contact )
    {
      return timespan_t::zero();  // If contact is lost, the attack is instant.
    }
    else if ( weapon->slot == SLOT_OFF_HAND && oh_lost_melee_contact )  // Also used for the first attack.
    {
      return timespan_t::zero();
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

  void execute() override
  {
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
        if ( p()->talents.devastator->ok() )
        {
          devastator->target = execute_state->target;
          devastator->schedule_execute();
        }
        trigger_rage_gain( execute_state );
        if ( p()->azerite.reckless_flurry.ok() )  // does this need a spec = fury check?
        {
          p()->cooldown.recklessness->adjust(
              ( -1 * p()->azerite.reckless_flurry.spell_ref().effectN( 1 ).time_value() ) );
        }
        if ( p()->buff.infinite_fury->check() )  // does this need a spec = fury check?
        {
          ( p()->buff.infinite_fury->trigger() ); // auto attacks refresh the buff but do not trigger it initially
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( reckless_flurry && result_is_hit( s->result ) )
    {
      reckless_flurry->set_target( s->target );
      reckless_flurry->execute();
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
      rage_gain *= 1.0 + p()->talents.war_machine->effectN( 2 ).percent();
    }
    else if ( p() -> specialization() == WARRIOR_FURY )
    {
      rage_gain *= fury_rage_multiplier;
      if ( weapon->slot == SLOT_OFF_HAND )
      {
        rage_gain *= 0.5;
      }
      rage_gain *= 1.0 + p()->talents.war_machine->effectN( 2 ).percent();
    }
    else 
    {
      // Protection generates a static 2 rage per successful auto attack landed
      rage_gain = 2.0;
    }

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
  auto_attack_t( warrior_t* p, const std::string& options_str ) : warrior_attack_t( "auto_attack", p )
  {
    parse_options( options_str );
    assert( p->main_hand_weapon.type != WEAPON_NONE );
    ignore_false_positive = true;
    range                 = 5;

    if ( p->main_hand_weapon.type == WEAPON_2H && p->has_shield_equipped() && p->specialization() != WARRIOR_FURY )
    {
      sim->errorf( "Player %s is using a 2 hander + shield while specced as Protection or Arms. Disabling autoattacks.",
                   name() );
    }
    else
    {
      p->main_hand_attack                    = new melee_t( "auto_attack_mh", p );
      p->main_hand_attack->weapon            = &( p->main_hand_weapon );
      p->main_hand_attack->base_execute_time = p->main_hand_weapon.swing_time;
    }

    if ( p->off_hand_weapon.type != WEAPON_NONE && p->specialization() == WARRIOR_FURY )
    {
      p->off_hand_attack                    = new melee_t( "auto_attack_oh", p );
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

// Mortal Strike ============================================================

struct mortal_strike_t20_t : public warrior_attack_t
{
  mortal_strike_t20_t( warrior_t* p, const std::string& name ) : warrior_attack_t( name, p, p->spec.mortal_strike )
  {
    cooldown->duration = timespan_t::zero();
    weapon             = &( p->main_hand_weapon );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.overpower->check_stack_value();

    return am;
  }

  double cost() const override
  {
    return 0;
  }

  double tactician_cost() const override
  {
    return 0;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( !sim->overrides.mortal_wounds && execute_state->target->debuffs.mortal_wounds )
      {
        execute_state->target->debuffs.mortal_wounds->trigger();
      }
    }
    p()->buff.overpower->expire();
    p()->buff.executioners_precision->expire();
  }
};

struct mortal_strike_t : public warrior_attack_t
{
  mortal_strike_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "mortal_strike", p, p->spec.mortal_strike )
  {
    parse_options( options_str );

    weapon           = &( p->main_hand_weapon );
    cooldown->hasted = true;  // Doesn't show up in spelldata for some reason.
    impact_action    = p->active.deep_wounds_ARMS;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.overpower->check_stack_value();

    return am;
  }

  double cost() const override
  {
    double c = warrior_attack_t::cost();

    c += p()->legendary.archavons_heavy_hand->effectN( 1 ).resource( RESOURCE_RAGE );

    return c;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->buff.executioners_precision->stack_value();
    return b;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      if ( !sim->overrides.mortal_wounds && execute_state->target->debuffs.mortal_wounds )
      {
        execute_state->target->debuffs.mortal_wounds->trigger();
      }
    }
    p()->buff.overpower->expire();
    p()->buff.executioners_precision->expire();
    p()->buff.weighted_blade->trigger( 1 );
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
  bladestorm_tick_t( warrior_t* p, const std::string& name )
    : warrior_attack_t( name, p,
                        p->specialization() == WARRIOR_FURY ? p->talents.bladestorm->effectN( 1 ).trigger()
                                                            : p->spec.bladestorm->effectN( 1 ).trigger() )
  {
    dual = true;
    aoe  = -1;
    if ( p->specialization() == WARRIOR_ARMS )
    {
      base_multiplier *= 1.0 + p->spec.arms_warrior->effectN( 4 ).percent();
      impact_action = p->active.deep_wounds_ARMS;
    }
  }
};

struct bladestorm_t : public warrior_attack_t
{
  attack_t *bladestorm_mh, *bladestorm_oh;
  mortal_strike_t20_t* mortal_strike;
  bladestorm_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "bladestorm", p,
                        p->specialization() == WARRIOR_FURY ? p->talents.bladestorm : p->spec.bladestorm ),
      bladestorm_mh( new bladestorm_tick_t( p, "bladestorm_mh" ) ),
      bladestorm_oh( nullptr ),
      mortal_strike( nullptr )
  {
    if ( p->talents.ravager->ok() )
    {
      background = true;  // Ravager replaces bladestorm for arms.
    }
    else
    {
      parse_options( options_str );
      channeled = tick_zero = true;
      callbacks = interrupt_auto_attack = false;
      travel_speed                      = 0;
      bladestorm_mh->weapon             = &( player->main_hand_weapon );
      add_child( bladestorm_mh );
      if ( player->off_hand_weapon.type != WEAPON_NONE && player->specialization() == WARRIOR_FURY )
      {
        bladestorm_oh         = new bladestorm_tick_t( p, "bladestorm_oh" );
        bladestorm_oh->weapon = &( player->off_hand_weapon );
        add_child( bladestorm_oh );
      }
      if ( p->specialization() == WARRIOR_FURY )
      {
        energize_type     = ENERGIZE_PER_TICK;
        energize_resource = RESOURCE_RAGE;
        energize_amount   = data().effectN( 4 ).resource( energize_resource );
      }
      if ( p->sets->has_set_bonus( WARRIOR_ARMS, T20, B4 ) )
      {
        mortal_strike = new mortal_strike_t20_t( p, "bladestorm_mortal_strike" );
        add_child( mortal_strike );
      }
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p()->buff.bladestorm->trigger();
  }

  void tick( dot_t* d ) override
  {
    if ( d->ticks_left() )
    {
      p()->buff.tornados_eye->trigger();
      p()->buff.gathering_storm->trigger();
    }
    else
    {
      // only duration is refreshed on last tick
      p()->buff.tornados_eye->trigger( 0 );
      p()->buff.gathering_storm->trigger( 0 );
    }

    warrior_attack_t::tick( d );
    bladestorm_mh->execute();

    if ( bladestorm_mh->result_is_hit( execute_state->result ) && bladestorm_oh )
    {
      bladestorm_oh->execute();
    }
    // Hotfix as of 2014-12-05: 3 -> 2 ticks with an additional MS. Seems to be ticks 1 and 3 (zero-indexed).
    if ( mortal_strike && ( d->current_tick == 1 || d->current_tick == 3 ) )
    {
      auto t = select_random_target();

      if ( t )
      {
        mortal_strike->target = t;
        mortal_strike->execute();
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );
    p()->buff.bladestorm->expire();
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

  double action_multiplier() const override
  {
    double am = warrior_heal_t::action_multiplier();

    am *= 1.0 + p()->buff.furious_charge->check_value();
    am *= 1.0 + p()->buff.t20_fury_4p->check_stack_value();

    return am;
  }
  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

// Bloodthirst ==============================================================

struct gushing_wound_dot_t : public warrior_attack_t
{
  gushing_wound_dot_t( warrior_t* p ) : warrior_attack_t( "gushing_wound", p, p->find_spell( 288091 ) ) //288080 & 4 is CB
  {
    background = tick_may_crit = true;
    hasted_ticks               = false;
    base_td = p->azerite.cold_steel_hot_blood.value( 2 );
  }
};

struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_heal_t* bloodthirst_heal;
  warrior_attack_t* gushing_wound;
  int aoe_targets;
  double enrage_chance;
  double rage_from_cold_steel_hot_blood;
  bloodthirst_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "bloodthirst", p, p->spec.bloodthirst ),
      bloodthirst_heal( nullptr ),
      gushing_wound( nullptr ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
      enrage_chance( p->spec.enrage->effectN( 2 ).percent() ),
      //rage_from_cold_steel_hot_blood( (p->azerite.cold_steel_hot_blood.spell()->effectN( 1 ).base_value() ) / 10.0 )
      rage_from_cold_steel_hot_blood( p->find_spell( 288087 )->effectN( 1 ).base_value() / 10.0 )
  {
    parse_options( options_str );

    weapon = &( p->main_hand_weapon );
    radius = 5;
    if ( p->non_dps_mechanics )
    {
      bloodthirst_heal = new bloodthirst_heal_t( p );
    }
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();

    if ( p->talents.fresh_meat->ok() )
    {
      enrage_chance += p->talents.fresh_meat->effectN( 1 ).percent();
    }

    if ( p->azerite.cold_steel_hot_blood.ok() )
    {
      gushing_wound = new gushing_wound_dot_t( p );
      add_child( gushing_wound );
    }
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.t20_fury_4p->check_stack_value();

    return am;
  }

  int n_targets() const override
  {
    if ( p()->buff.meat_cleaver->check() )
    {
      return aoe_targets + 1;
    }
    return warrior_attack_t::n_targets();
  }

  double composite_crit_chance() const override
  {
    double c = warrior_attack_t::composite_crit_chance();
    if ( p()->buff.bloodcraze->check() )
      c += p()->buff.bloodcraze->check_stack_value() / p()->current.rating.attack_crit;
    return c;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( gushing_wound && execute_state->result == RESULT_CRIT )
    {
      gushing_wound->execute();
    }

    if ( p()->azerite.cold_steel_hot_blood.ok() && execute_state->result == RESULT_CRIT )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_cold_steel_hot_blood, p()->gain.cold_steel_hot_blood );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.meat_cleaver->decrement();

    if ( result_is_hit( execute_state->result ) )
    {
      p()->buff.fujiedas_fury->trigger( 1 );
      if ( bloodthirst_heal )
      {
        bloodthirst_heal->execute();
        p()->buff.furious_charge->expire();
      }
      p()->buff.t20_fury_4p->expire();

      if ( rng().roll( enrage_chance ) )
      {
        p()->enrage();
      }
    }
    if ( execute_state->result == RESULT_CRIT )
    {
      p()->buff.bloodcraze->expire();
    }
  }
};

// Furious Slash ==============================================================

struct furious_slash_t : public warrior_attack_t
{
  int aoe_targets;
  furious_slash_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "furious_slash", p, p->talents.furious_slash ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 2 ).base_value() ) )
  {
    parse_options( options_str );
    weapon              = &( p->off_hand_weapon );
    radius              = 5;
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

    p()->buff.meat_cleaver->decrement();
    p()->buff.furious_slash->trigger();
  }

  bool ready() override
  {
    if ( p()->off_hand_weapon.type == WEAPON_NONE )
      return false;
    return warrior_attack_t::ready();
  }
};

// Charge ===================================================================

struct charge_t : public warrior_attack_t
{
  const spell_data_t* charge_damage;
  bool first_charge;
  double movement_speed_increase, min_range;
  charge_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "charge", p, p->spell.charge ),
      charge_damage( p->find_spell( 126664 ) ),
      first_charge( true ),
      movement_speed_increase( 5.0 ),
      min_range( data().min_range() )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    movement_directionality = MOVEMENT_OMNI;
    energize_resource       = RESOURCE_RAGE;
    energize_type           = ENERGIZE_ON_CAST;
    attack_power_mod.direct = charge_damage->effectN( 2 ).ap_coeff();

    if ( p->talents.double_time->ok() )
    {
      cooldown->charges += as<int>( p->talents.double_time->effectN( 1 ).base_value() );
      cooldown->duration += p->talents.double_time->effectN( 2 ).time_value();
    }
    p->cooldown.charge = cooldown;
    p->active.charge   = this;
  }

  void execute() override
  {
    if ( p()->current.distance_to_move > 5 )
    {
      p()->buff.charge_movement->trigger(
          1, movement_speed_increase, 1,
          timespan_t::from_seconds(
              p()->current.distance_to_move /
              ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) ) );
      p()->current.moving_away = 0;
    }

    warrior_attack_t::execute();

    p()->buff.furious_charge->trigger();

    if ( p()->legendary.sephuzs_secret != spell_data_t::not_found() && execute_state->target->type == ENEMY_ADD )
    {
      p()->buff.sephuzs_secret->trigger();
    }
    if ( first_charge )
    {
      first_charge = !first_charge;
    }
  }

  void reset() override
  {
    action_t::reset();
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
         p()->buff.intervene_movement->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Intercept ===========================================================================
// FIXME: Min range on bad dudes, no min range on good dudes.
struct intercept_t : public warrior_attack_t
{
  double movement_speed_increase;
  intercept_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "intercept", p, p->spec.intercept ), movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    movement_directionality = MOVEMENT_OMNI;
    energize_type           = ENERGIZE_ON_CAST;
    energize_resource       = RESOURCE_RAGE;

    cooldown-> charges += as<int>( p-> spec.prot_warrior -> effectN( 3 ).base_value() );
    cooldown-> duration += timespan_t::from_millis( p -> spec.prot_warrior -> effectN( 4 ).base_value() );
  }

  void execute() override
  {
    if ( p()->current.distance_to_move > 5 )
    {
      p()->buff.intercept_movement->trigger(
          1, movement_speed_increase, 1,
          timespan_t::from_seconds(
              p()->current.distance_to_move /
              ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) ) );
      p()->current.moving_away = 0;
    }
    warrior_attack_t::execute();
  }

  bool ready() override
  {
    if ( p()->buff.heroic_leap_movement->check() )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Cleave ===================================================================

struct cleave_t : public warrior_attack_t
{
  cleave_t( warrior_t* p, const std::string& options_str ) : warrior_attack_t( "cleave", p, p->talents.cleave )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
    aoe    = -1;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    if ( execute_state->n_targets >= 3 )
    {
      p()->active.deep_wounds_ARMS->set_target( s->target );
      p()->active.deep_wounds_ARMS->execute();
    }
  }
};

// Colossus Smash ===========================================================

struct colossus_smash_t : public warrior_attack_t
{
  bool lord_of_war;
  double rage_from_lord_of_war;
  colossus_smash_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "colossus_smash", p, p->spec.colossus_smash ),
      lord_of_war( false ),
      rage_from_lord_of_war(
        //( p->azerite.lord_of_war.spell()->effectN( 1 ).base_value() * p->azerite.lord_of_war.n_items() ) / 10.0 )
          ( p->azerite.lord_of_war.spell()->effectN( 1 ).base_value() ) / 10.0 ) // 8.1 no extra rage with rank
  {
    if ( p->talents.warbreaker->ok() )
    {
      background = true;  // Warbreaker replaces Colossus Smash for Arms
    }
    else
    {
      parse_options( options_str );
      weapon = &( player->main_hand_weapon );
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->azerite.lord_of_war.value( 2 );
    return b;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->azerite.lord_of_war.ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_lord_of_war, p()->gain.lord_of_war );
    }

    if ( result_is_hit( execute_state->result ) )
    {
      td( execute_state->target )->debuffs_colossus_smash->trigger();
      p()->buff.test_of_might_tracker->trigger();

      p()->buff.war_veteran->trigger();

      if ( p()->talents.in_for_the_kill->ok() )
        p()->buff.in_for_the_kill->trigger();
    }
  }
};

// Deep Wounds ARMS ==============================================================

struct deep_wounds_ARMS_t : public warrior_attack_t
{
  deep_wounds_ARMS_t( warrior_t* p ) : warrior_attack_t( "deep_wounds", p, p->find_spell( 262115 ) )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->cache.mastery_value();

    return am;
  }
};

// Deep Wounds PROT ==============================================================

struct deep_wounds_PROT_t : public warrior_attack_t
{
  deep_wounds_PROT_t( warrior_t* p )
    : warrior_attack_t( "deep_wounds", p, p->spec.deep_wounds_PROT->effectN( 2 ).trigger() )
  {
    background = tick_may_crit = true;
    hasted_ticks               = true;
  }
};

// Demoralizing Shout =======================================================

struct demoralizing_shout : public warrior_attack_t
{
  double rage_gain;
  demoralizing_shout( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "demoralizing_shout", p, p->spec.demoralizing_shout ), rage_gain( 0 )
  {
    parse_options( options_str );
    rage_gain += p->talents.booming_voice->effectN( 1 ).resource( RESOURCE_RAGE );
    aoe = -1;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( rage_gain > 0 )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_gain, p()->gain.booming_voice );
    }
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );
    td( s->target ) -> debuffs_demoralizing_shout -> trigger( 
      1, data().effectN( 1 ).percent(), 1.0, p()->spec.demoralizing_shout->duration()  );
  }
};

// Dragon Roar ==============================================================

struct dragon_roar_t : public warrior_attack_t
{
  dragon_roar_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "dragon_roar", p, p->talents.dragon_roar )
  {
    parse_options( options_str );
    aoe       = -1;
    may_dodge = may_parry = may_block = false;
  }
};

// Arms Execute ==================================================================

struct execute_damage_t : public warrior_attack_t
{
  double max_rage;
  double cost_rage;
  execute_damage_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "execute", p, p->spec.execute->effectN( 1 ).trigger() ), max_rage( 40 )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
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
};

struct execute_arms_t : public warrior_attack_t
{
  execute_damage_t* trigger_attack;
  double max_rage;
  double execute_pct;
  execute_arms_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "execute", p, p->spec.execute ), max_rage( 40 ), execute_pct( 20 )
  {
    parse_options( options_str );
    weapon        = &( p->main_hand_weapon );
    impact_action = p->active.deep_wounds_ARMS;

    trigger_attack = new execute_damage_t( p, options_str );

    if ( p->talents.massacre->ok() )
    {
      execute_pct = p->talents.massacre->effectN( 2 )._base_value;
    }
  }

  double tactician_cost() const override
  {
    double c = max_rage;

    if ( !p()->buff.ayalas_stone_heart->check() && !p()->buff.deadly_calm->check() && !p()->buff.sudden_death->check() )
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

    if ( p()->buff.ayalas_stone_heart->check() )
    {
      return c *= 1.0 + p()->buff.ayalas_stone_heart->data().effectN( 2 ).percent();
    }
    if ( p()->buff.sudden_death->check() )
    {
      return 0;  // The cost reduction isn't in the spell data
    }
    if ( p()->buff.deadly_calm->check() )
    {
      c *= 1.0 + p()->talents.deadly_calm->effectN( 1 ).percent();
    }
    return c;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    trigger_attack->cost_rage = last_resource_cost;
    trigger_attack->execute();
    p()->resource_gain( RESOURCE_RAGE, last_resource_cost * 0.2,
                        p()->gain.execute_refund );  // Not worth the trouble to check if the target died.

    p()->buff.ayalas_stone_heart->expire();
    p()->buff.sudden_death->expire();

    if ( p()->azerite.executioners_precision.ok() )
    {
      p()->buff.executioners_precision->trigger();
    }
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Ayala's Stone Heart and Sudden Death allow execution on any target
    bool always = p()->buff.ayalas_stone_heart->check() || p()->buff.sudden_death->check();

    if ( ! always && candidate_target->health_percentage() > execute_pct )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
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

// Fury Execute ======================================================================
struct execute_main_hand_t : public warrior_attack_t
{
  int aoe_targets;
  execute_main_hand_t( warrior_t* p, const char* name, const spell_data_t* s )
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    dual   = true;
    weapon = &( p->main_hand_weapon );
    base_multiplier *= 1.0 + p->spec.execute_2->effectN( 1 ).percent();
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
};

struct execute_off_hand_t : public warrior_attack_t
{
  int aoe_targets;
  execute_off_hand_t( warrior_t* p, const char* name, const spell_data_t* s ) 
    : warrior_attack_t( name, p, s ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) )
  {
    dual     = true;
    may_miss = may_dodge = may_parry = may_block = false;
    weapon                                       = &( p->off_hand_weapon );
    base_multiplier *= 1.0 + p->spec.execute_2->effectN( 1 ).percent();
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
};

struct fury_execute_parent_t : public warrior_attack_t
{
  execute_main_hand_t* mh_attack;
  execute_off_hand_t* oh_attack;
  double execute_pct;
  fury_execute_parent_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "execute", p, p->spec.execute ), execute_pct( 20 )
  {
    parse_options( options_str );
    mh_attack = new execute_main_hand_t( p, "execute_mainhand", p->spec.execute->effectN( 1 ).trigger() );
    oh_attack = new execute_off_hand_t( p, "execute_offhand", p->spec.execute->effectN( 2 ).trigger() );
    add_child( mh_attack );
    add_child( oh_attack );

    if ( p->talents.massacre->ok() )
    {
      execute_pct = p->talents.massacre->effectN( 2 )._base_value;
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();

    mh_attack->execute();

    if ( p()->specialization() == WARRIOR_FURY && result_is_hit( execute_state->result ) &&
         p()->off_hand_weapon.type != WEAPON_NONE )
      // If MH fails to land, or if there is no OH weapon for Fury, oh attack does not execute.
      oh_attack->execute();

    p()->buff.meat_cleaver->decrement();
    p()->buff.sudden_death->expire();
    p()->buff.ayalas_stone_heart->expire();
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Ayala's Stone Heart and Sudden Death allow execution on any target
    bool always = p()->buff.ayalas_stone_heart->check() || p()->buff.sudden_death->check();

    if ( ! always && candidate_target->health_percentage() > execute_pct )
    {
      return false;
    }

    return warrior_attack_t::target_ready( candidate_target );
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

// Hamstring ==============================================================

struct hamstring_t : public warrior_attack_t
{
  hamstring_t( warrior_t* p, const std::string& options_str ) : warrior_attack_t( "hamstring", p, p->spec.hamstring )
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
  }
};

// Piercing Howl ==============================================================

struct piercing_howl_t : public warrior_attack_t
{
  piercing_howl_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "piercing_howl", p, p->spec.piercing_howl )
  {
    parse_options( options_str );
  }
};

// Heroic Throw ==============================================================

struct heroic_throw_t : public warrior_attack_t
{
  heroic_throw_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "heroic_throw", p, p->find_class_spell( "Heroic Throw" ) )
  {
    parse_options( options_str );

    weapon    = &( player->main_hand_weapon );
    may_dodge = may_parry = may_block = false;

    cooldown -> duration *= 1.0 + p -> spec.prot_warrior -> effectN( 5 ).percent();
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
  bool weight_of_the_earth;
  heroic_leap_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "heroic_leap", p, p->spell.heroic_leap ),
      heroic_leap_damage( p->find_spell( 52174 ) ),
      weight_of_the_earth( false )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    aoe                   = -1;
    may_dodge = may_parry = may_miss = may_block = false;
    movement_directionality                      = MOVEMENT_OMNI;
    base_teleport_distance                       = data().max_range();
    range                                        = -1;
    attack_power_mod.direct                      = heroic_leap_damage->effectN( 1 ).ap_coeff();
    radius                                       = heroic_leap_damage->effectN( 1 ).radius();

    cooldown->duration = data().charge_cooldown();  // Fixes bug in spelldata for now.
    cooldown->duration += p->talents.bounding_stride->effectN( 1 ).time_value();
  }

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
                     ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() ) ) /
                     travel_time().total_seconds();
      p()->buff.heroic_leap_movement->trigger( 1, speed, 1, travel_time() );
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( p()->current.distance_to_move <= radius && p()->current.moving_away <= radius &&
         ( p()->heroic_charge != nullptr || weight_of_the_earth ) )
    {
      warrior_attack_t::impact( s );
      if ( weight_of_the_earth && p()->specialization() == WARRIOR_ARMS )
      {
        td( s->target )->debuffs_colossus_smash->trigger();
      }
    }
  }

  bool ready() override
  {
    if ( p()->buff.intervene_movement->check() || p()->buff.charge_movement->check() ||
         p()->buff.intercept_movement->check() )
    {
      return false;
    }

    return warrior_attack_t::ready();
  }
};

// Impending Victory ========================================================

struct impending_victory_heal_t : public warrior_heal_t
{
  impending_victory_heal_t( warrior_t* p ) : warrior_heal_t( "impending_victory_heal", p, p->find_spell( 118340 ) )
  {
    base_pct_heal = data().effectN( 1 ).percent();
    background    = true;
  }

  resource_e current_resource() const override
  {
    return RESOURCE_NONE;
  }
};

struct impending_victory_t : public warrior_attack_t
{
  impending_victory_heal_t* impending_victory_heal;
  impending_victory_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "impending_victory", p ), impending_victory_heal( nullptr )
  {
    parse_options( options_str );
    if ( p->non_dps_mechanics )
    {
      impending_victory_heal = new impending_victory_heal_t( p );
    }
    parse_effect_data( data().effectN( 2 ) );  // Both spell effects list an ap coefficient, #2 is correct.
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( impending_victory_heal )
    {
      impending_victory_heal->execute();
    }
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
  intervene_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "intervene", p, p->spell.intervene ), movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    movement_directionality = MOVEMENT_OMNI;
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
              ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() + movement_speed_increase ) ) ) );
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

// Iron Fortress azerite trait ==============================================

struct iron_fortress_t : public warrior_attack_t
{
  timespan_t internal_cd;
  bool crit_blocked;

  iron_fortress_t( warrior_t* p ) :
    warrior_attack_t( "iron_fortress", p, p -> azerite.iron_fortress.spell() -> effectN( 1 ).trigger() ), // TODO: change to 279142 once it's pulled
    internal_cd( p -> azerite.iron_fortress.spell() -> effectN( 1 ).trigger() -> internal_cooldown() ),
    crit_blocked( false )
  {
    base_dd_min = base_dd_max = p -> azerite.iron_fortress.value( 1 );
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    // If the hit was critically blocked, deals twice as much damage (no spelldata for it)
    if ( crit_blocked )
    {
      am *= 2.0;
    }

    return am;
  }

  void do_execute( block_result_e b )
  {
    crit_blocked = b == BLOCK_RESULT_CRIT_BLOCKED;

    warrior_attack_t::execute();
    p() -> cooldown.iron_fortress_icd -> start( internal_cd );
  }
};

// Heroic Charge ============================================================

struct heroic_charge_movement_ticker_t : public event_t
{
  timespan_t duration;
  warrior_t* warrior;

  heroic_charge_movement_ticker_t( sim_t&, warrior_t* p, timespan_t d = timespan_t::zero() )
    : event_t( *p ), warrior( p )
  {
    if ( d > timespan_t::zero() )
    {
      duration = d;
    }
    else
    {
      duration = next_execute();
    }

    if ( sim().debug )
      sim().out_debug.printf( "New movement event" );

    schedule( duration );
  }

  timespan_t next_execute() const
  {
    timespan_t min_time       = timespan_t::max();
    bool any_movement         = false;
    timespan_t time_to_finish = warrior->time_to_move();
    if ( time_to_finish != timespan_t::zero() )
    {
      any_movement = true;

      if ( time_to_finish < min_time )
      {
        min_time = time_to_finish;
      }
    }

    if ( min_time >
         timespan_t::from_seconds( 0.05 ) )  // Update a little more than usual, since we're moving a lot faster.
    {
      min_time = timespan_t::from_seconds( 0.05 );
    }

    if ( !any_movement )
    {
      return timespan_t::zero();
    }
    else
    {
      return min_time;
    }
  }

  void execute() override
  {
    if ( warrior->time_to_move() > timespan_t::zero() )
    {
      warrior->update_movement( duration );
    }

    timespan_t next = next_execute();
    if ( next > timespan_t::zero() )
    {
      warrior->heroic_charge = make_event<heroic_charge_movement_ticker_t>( sim(), sim(), warrior, next );
    }
    else
    {
      warrior->heroic_charge = nullptr;
      warrior->buff.heroic_leap_movement->expire();
    }
  }
};

struct heroic_charge_t : public warrior_attack_t
{
  heroic_leap_t* leap;
  bool disable_leap;
  heroic_charge_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "heroic_charge", p, spell_data_t::nil() ), leap( nullptr ), disable_leap( false )
  {
    add_option( opt_bool( "disable_heroic_leap", disable_leap ) );
    parse_options( options_str );

    leap                  = new heroic_leap_t( p, "" );
    trigger_gcd           = timespan_t::zero();
    ignore_false_positive = true;
    callbacks = may_crit = may_hit = false;
    p->active.charge->use_off_gcd  = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( !disable_leap && p()->cooldown.heroic_leap->up() )
    {  // We are moving 10 yards, and heroic leap always executes in 0.5 seconds.
      // Do some hacky math to ensure it will only take 0.5 seconds, since it will certainly
      // be the highest temporary movement speed buff.
      double speed;
      speed = 10 / ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() ) ) / 0.5;
      p()->buff.heroic_leap_movement->trigger( 1, speed, 1, timespan_t::from_millis( 500 ) );
      leap->execute();
      p()->trigger_movement(
          10.0, MOVEMENT_AWAY );  // Leap 10 yards out, because it's impossible to precisely land 8 yards away.
      p()->heroic_charge = make_event<heroic_charge_movement_ticker_t>( *sim, *sim, p() );
    }
    else
    {
      p()->trigger_movement( 9.0, MOVEMENT_AWAY );
      p()->heroic_charge = make_event<heroic_charge_movement_ticker_t>( *sim, *sim, p() );
    }
  }

  bool ready() override
  {
    if ( p()->cooldown.charge->up() && !p()->buffs.movement->check() && p()->heroic_charge == nullptr )
    {
      return warrior_attack_t::ready();
    }
    else
    {
      return false;
    }
  }
};

// Pummel ===================================================================

struct pummel_t : public warrior_attack_t
{
  pummel_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "pummel", p, p->find_class_spell( "Pummel" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->legendary.sephuzs_secret != spell_data_t::not_found() )
    {
      p()->buff.sephuzs_secret->trigger();
    }
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

    base_multiplier *= 1.0 + p->talents.inner_rage->effectN( 2 ).percent();
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.raging_thirst->check_value();

    return am;
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

struct raging_blow_t : public warrior_attack_t
{
  raging_blow_attack_t* mh_attack;
  raging_blow_attack_t* oh_attack;
  double cd_reset_chance;
  raging_blow_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "raging_blow", p, p->spec.raging_blow ),
      mh_attack( nullptr ),
      oh_attack( nullptr ),
      cd_reset_chance( p->spec.raging_blow->effectN( 1 ).percent() )
  {
    parse_options( options_str );

    oh_attack         = new raging_blow_attack_t( p, "raging_blow_oh", p->spec.raging_blow->effectN( 4 ).trigger() );
    oh_attack->weapon = &( p->off_hand_weapon );
    add_child( oh_attack );
    mh_attack         = new raging_blow_attack_t( p, "raging_blow_mh", p->spec.raging_blow->effectN( 3 ).trigger() );
    mh_attack->weapon = &( p->main_hand_weapon );
    add_child( mh_attack );
    cooldown->reset( false );
    track_cd_waste = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      mh_attack->execute();
      oh_attack->execute();
    }
    if ( rng().roll( cd_reset_chance ) )
    {
      cooldown->reset( true );
    }
    p()->buff.t20_fury_4p->trigger( 1 );
    p()->buff.meat_cleaver->decrement();

    if ( p()->azerite.pulverizing_blows.ok() )
    {
      p()->buff.pulverizing_blows->trigger();
    }
  }

  double recharge_multiplier() const override
  {
    auto m = warrior_attack_t::recharge_multiplier();

    if ( p()->talents.inner_rage->ok() )
    {
      m *= 1.0 + p()->talents.inner_rage->effectN( 1 ).percent();
    }
    return m;
  }

  bool ready() override
  {
    // Needs weapons in both hands
    if ( p()->main_hand_weapon.type == WEAPON_NONE || p()->off_hand_weapon.type == WEAPON_NONE )
    {
      return false;
    }
    return warrior_attack_t::ready();
  }
};

// Siegebreaker ===========================================================

struct siegebreaker_t : public warrior_attack_t
{
  siegebreaker_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "siegebreaker", p, p->talents.siegebreaker )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( result_is_hit( execute_state->result ) )
    {
      td( execute_state->target )->debuffs_siegebreaker->trigger();
    }
  }
};

// Skullsplitter ===========================================================

struct skullsplitter_t : public warrior_attack_t
{
  skullsplitter_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "skullsplitter", p, p->talents.skullsplitter )
  {
    parse_options( options_str );
    weapon = &( player->main_hand_weapon );
  }
};

// Sweeping Strikes ===================================================================

struct sweeping_strikes_t : public warrior_spell_t
{
  sweeping_strikes_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "sweeping_strikes", p, p->spec.sweeping_strikes )
  {
    parse_options( options_str );
    callbacks = false;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.sweeping_strikes->trigger();
  }
};

// Overpower ============================================================

struct seismic_wave_t : warrior_attack_t
{
  seismic_wave_t( warrior_t* p )
    : warrior_attack_t( "seismic_wave", p, p->find_spell( 278497 ) )
  {
    aoe         = -1;
    background  = true;
    base_dd_min = base_dd_max = p->azerite.seismic_wave.value( 1 );
  }
};
struct overpower_t : public warrior_attack_t
{
  warrior_attack_t* seismic_wave;
  overpower_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "overpower", p, p->spec.overpower ), seismic_wave( nullptr )
  {
    parse_options( options_str );
    may_block = may_parry = may_dodge = false;
    weapon                            = &( p->main_hand_weapon );

    if ( p->talents.dreadnaught->ok() )
    {
      cooldown->charges += as<int>( p->talents.dreadnaught->effectN( 1 ).base_value() );
    }
    p->cooldown.charge = cooldown;
    p->active.charge   = this;

    if ( p->azerite.seismic_wave.ok() )
    {
      seismic_wave = new seismic_wave_t( p );
      add_child( seismic_wave );
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    if ( p()->buff.striking_the_anvil->check() )
    {
      b += p()->buff.striking_the_anvil->value();
    }
    return b;
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( seismic_wave && result_is_hit( s->result ) )
    {
      seismic_wave->set_target( s->target );
      seismic_wave->execute();
    }
    if ( p()->buff.striking_the_anvil->check() )
    {
      p()->cooldown.mortal_strike->adjust( - timespan_t::from_millis( 
      p()->azerite.striking_the_anvil.spell()->effectN( 2 ).base_value() ) );
    }
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p()->buff.overpower->trigger();
    p()->buff.striking_the_anvil->expire();
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
  bool lord_of_war;
  double rage_from_lord_of_war;
  warbreaker_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "warbreaker", p, p->talents.warbreaker ),
      lord_of_war( false ),
      rage_from_lord_of_war(
          //( p->azerite.lord_of_war.spell()->effectN( 1 ).base_value() * p->azerite.lord_of_war.n_items() ) / 10.0 )
          ( p->azerite.lord_of_war.spell()->effectN( 1 ).base_value() ) / 10.0 ) // 8.1 no extra rage with rank
  {
    parse_options( options_str );
    weapon = &( p->main_hand_weapon );
    aoe    = -1;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->azerite.lord_of_war.value( 2 );
    return b;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->azerite.lord_of_war.ok() )
    {
      p()->resource_gain( RESOURCE_RAGE, rage_from_lord_of_war, p()->gain.lord_of_war );
    }

    if ( hit_any_target )
    {
      p()->buff.war_veteran->trigger();
      p()->buff.test_of_might_tracker->trigger();

      if ( p()->talents.in_for_the_kill->ok() )
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
  bool first_attack, first_attack_missed, valarjar_berserking, simmering_rage;
//  double unbridled_chance; //unbridled ferocity azerite trait
  double rage_from_valarjar_berserking;
  double rage_from_simmering_rage;
  rampage_attack_t( warrior_t* p, const spell_data_t* rampage, const std::string& name )
    : warrior_attack_t( name, p, rampage ),
      aoe_targets( as<int>( p->spell.whirlwind_buff->effectN( 1 ).base_value() ) ),
//      unbridled_chance( p->azerite.unbridled_ferocity.spell()->effectN( 2 ).percent() ),
      first_attack( false ),
      first_attack_missed( false ),
      valarjar_berserking( false ),
      simmering_rage( false ),
      rage_from_valarjar_berserking( p->find_spell( 248179 )->effectN( 1 ).base_value() / 10.0 ),
      rage_from_simmering_rage(
          ( p->azerite.simmering_rage.spell()->effectN( 1 ).base_value() ) / 10.0 )
  {
    dual = true;
    if ( p->sets->has_set_bonus( WARRIOR_FURY, T21, B4 ) )
    {
      base_multiplier *= ( 1.0 + p->find_spell( 200853 )->effectN( 1 ).percent() );
    }
    base_multiplier *= 1.0 + p->talents.carnage->effectN( 4 ).percent();
    base_aoe_multiplier = p->spell.whirlwind_buff->effectN( 3 ).percent();
    if ( p->spec.rampage->effectN( 3 ).trigger() == rampage )
      first_attack = true;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( first_attack && result_is_miss( execute_state->result ) )
      first_attack_missed = true;
    else if ( first_attack )
      first_attack_missed = false;
  }

  void impact( action_state_t* s ) override
  {
    if ( !first_attack_missed )
    {  // If the first attack misses, all of the rest do as well. However, if any other attack misses, the attacks after
       // continue. The animations and timing of everything else still occur, so we can't just cancel rampage.
      warrior_attack_t::impact( s );
      if ( p()->legendary.valarjar_berserkers != spell_data_t::not_found() && s->result == RESULT_CRIT &&
           target == s->target )
      {
        p()->resource_gain( RESOURCE_RAGE, rage_from_valarjar_berserking, p()->gain.valarjar_berserking );
      }
      if ( p()->azerite.simmering_rage.ok() && target == s->target )
      {
        p()->resource_gain( RESOURCE_RAGE, rage_from_simmering_rage, p()->gain.simmering_rage );
      }

      if ( result_is_hit( s->result ) && p()->sets->has_set_bonus( WARRIOR_FURY, T21, B2 ) )
      {
        double amount = s->result_amount * p()->sets->set( WARRIOR_FURY, T21, B2 )->effectN( 1 ).percent();

        residual_action::trigger( p()->active.slaughter, s->target, amount );
      }
    }
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->buff.pulverizing_blows->stack_value();
    b += p()->azerite.simmering_rage.value( 2 );
    b += p()->azerite.unbridled_ferocity.value( 2 );
    return b;
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

struct rampage_event_t : public event_t
{
  timespan_t duration;
  warrior_t* warrior;
  size_t attacks;
  rampage_event_t( warrior_t* p, size_t current_attack ) : event_t( *p->sim ), warrior( p ), attacks( current_attack )
  {
    duration = next_execute();
    schedule( duration );
    if ( sim().debug )
      sim().out_debug.printf( "New rampage event" );
  }

  timespan_t next_execute() const
  {
    timespan_t time_till_next_attack = timespan_t::zero();
    switch ( attacks )
    {
      case 0:
        break;  // First attack is instant.
      case 1:
        time_till_next_attack = timespan_t::from_millis( warrior->spec.rampage->effectN( 3 ).misc_value1() );
        break;
      case 2:
        time_till_next_attack = timespan_t::from_millis( warrior->spec.rampage->effectN( 4 ).misc_value1() -
                                                         warrior->spec.rampage->effectN( 3 ).misc_value1() );
        break;
      case 3:
        time_till_next_attack = timespan_t::from_millis( warrior->spec.rampage->effectN( 5 ).misc_value1() -
                                                         warrior->spec.rampage->effectN( 4 ).misc_value1() );
        break;
    }
    return time_till_next_attack;
  }

  void execute() override
  {
    warrior->rampage_attacks[ attacks ]->execute();
    if ( attacks == 0 )
    {
      // Enrage/Frothing go here if not benefiting the first hit
    }
    attacks++;
    if ( attacks < warrior->rampage_attacks.size() )
    {
      warrior->rampage_driver = make_event<rampage_event_t>( sim(), warrior, attacks );
    }
    else
    {
      warrior->rampage_driver = nullptr;
      warrior->buff.meat_cleaver->decrement();
      warrior->buff.pulverizing_blows->expire();

    }
  }
};

struct rampage_parent_t : public warrior_attack_t
{
  double unbridled_chance;  // unbridled ferocity azerite trait
  rampage_parent_t( warrior_t* p, const std::string& options_str ) : warrior_attack_t( "rampage", p, p->spec.rampage ),
  unbridled_chance( p->find_spell( 288060 )->proc_chance() )
  {
    parse_options( options_str );
    for ( size_t i = 0; i < p->rampage_attacks.size(); i++ )
    {
      add_child( p->rampage_attacks[ i ] );
    }
    track_cd_waste = false;
    base_costs[ RESOURCE_RAGE ] += p->talents.carnage->effectN( 1 ).resource( RESOURCE_RAGE );
    base_costs[ RESOURCE_RAGE ] += p->talents.frothing_berserker->effectN( 2 ).resource( RESOURCE_RAGE );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( p()->talents.frothing_berserker->ok() )
    {
      p()->buff.frothing_berserker->trigger();
    }
    if ( p()->azerite.trample_the_weak.ok() )
    {
      p()->buff.trample_the_weak->trigger();
    }
    if ( p()->azerite.unbridled_ferocity.ok() && rng().roll( unbridled_chance ) )
    {
      if ( p()->buff.recklessness->check() )
      {
        p()->buff.recklessness->extend_duration( p(), timespan_t::from_seconds( 4 ) );
      }
      else
      {
      p()->buff.recklessness->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, timespan_t::from_seconds( 4 ) );
      }
    }
    p()->enrage();
    p()->rampage_driver = make_event<rampage_event_t>( *sim, p(), 0 );
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
  double rage_from_ravager;
  ravager_tick_t( warrior_t* p, const std::string& name )
    : warrior_attack_t( name, p, p->find_spell( 156287 ) ), rage_from_ravager( 0.0 )
  {
    aoe           = -1;
    impact_action = p->active.deep_wounds_ARMS;
    dual = ground_aoe = true;
    // Protection's Ravager deals less damage
    attack_power_mod.direct *= 1.0 + p->spec.prot_warrior->effectN( 8 ).percent();
    rage_from_ravager = p->find_spell( 248439 )->effectN( 1 ).resource( RESOURCE_RAGE );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    if ( execute_state->n_targets > 0 )
      p()->resource_gain( RESOURCE_RAGE, rage_from_ravager, p()->gain.ravager );
  }
};

struct ravager_t : public warrior_attack_t
{
  ravager_tick_t* ravager;
  mortal_strike_t20_t* mortal_strike;
  ravager_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "ravager", p, p->talents.ravager ),
      ravager( new ravager_tick_t( p, "ravager_tick" ) ),
      mortal_strike( nullptr )
  {
    parse_options( options_str );
    ignore_false_positive   = true;
    hasted_ticks            = true;
    callbacks               = false;
    attack_power_mod.direct = attack_power_mod.tick = 0;
    add_child( ravager );
    if ( p->sets->has_set_bonus( WARRIOR_ARMS, T20, B4 ) )
    {
      mortal_strike = new mortal_strike_t20_t( p, "ravager_mortal_strike" );
      add_child( mortal_strike );
    }
  }

  void execute() override
  {
    if ( p()->specialization() == WARRIOR_PROTECTION )
    {
      p()->buff.ravager_protection->trigger();
    }
    else
    {
      p()->buff.ravager->trigger();
    }

    warrior_attack_t::execute();
  }

  void tick( dot_t* d ) override
  {
    // the ticks do scale with haste so I turned hasted_ticks on
    // however this made it tick more than 7 times
    if ( d->current_tick > 7 )
      return;

    // the helm buff occurs before each tick
    // it refreshes and adds one stack on the first 6 ticks
    // only duration is refreshed on last tick, no stack is added
    if ( d->current_tick <= 6 )
    {
      p()->buff.tornados_eye->trigger();
      p()->buff.gathering_storm->trigger();
    }
    if ( d->current_tick == 7 )
    {
      p()->buff.tornados_eye->trigger( 0 );
      p()->buff.gathering_storm->trigger( 0 );
    }
    warrior_attack_t::tick( d );
    ravager->execute();
    // the 4pc occurs on the first and 4th tick
    // if (mortal_strike && (d->current_tick == 1 || d->current_tick == 4))
    // As of 2017-12-05, this was hotfixed to only one tick total. Seems to be third one.
    if ( mortal_strike && d->current_tick == 3 )
    {
      auto t = select_random_target();

      if ( t )
      {
        mortal_strike->target = t;
        mortal_strike->execute();
      }
    }
  }
};

// Revenge ==================================================================

struct revenge_t : public warrior_attack_t
{
  double shield_slam_reset;
  revenge_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "revenge", p, p->spec.revenge ),
      shield_slam_reset( p->spec.shield_slam_2->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    aoe           = -1;
    impact_action = p->active.deep_wounds_PROT;
  }

  double cost() const override
  {
    double cost = warrior_attack_t::cost();
    cost *= 1.0 + p()->buff.revenge->check_value();
    cost *= 1.0 + p()->buff.vengeance_revenge->check_value();
    return cost;
  }

  void execute() override
  {
    warrior_attack_t::execute();
    p()->buff.revenge->expire();
    p()->buff.vengeance_revenge->expire();
    p()->buff.vengeance_ignore_pain->trigger();

    if ( rng().roll( shield_slam_reset ) )
      p()->cooldown.shield_slam->reset( true );
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );
    if ( p() -> azerite.callous_reprisal.enabled() )
    {
      td( state -> target ) -> debuffs_callous_reprisal -> trigger();
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

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    b += p()->azerite.callous_reprisal.value( 2 );
    return b;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + ( p()->talents.best_served_cold->effectN( 1 ).percent() *
                  std::min( target_list().size(),
                            static_cast<size_t>( p()->talents.best_served_cold->effectN( 2 ).base_value() ) ) );

    return am;
  }
};

// Enraged Regeneration ===============================================

struct enraged_regeneration_t : public warrior_heal_t
{
  enraged_regeneration_t( warrior_t* p, const std::string& options_str )
    : warrior_heal_t( "enraged_regeneration", p, p->spec.enraged_regeneration )
  {
    parse_options( options_str );
    range         = -1;
    base_pct_heal = data().effectN( 1 ).percent();
  }
};

// Rend ==============================================================

struct rend_t : public warrior_attack_t
{
  rend_t( warrior_t* p, const std::string& options_str ) : warrior_attack_t( "rend", p, p->talents.rend )
  {
    parse_options( options_str );
    tick_may_crit = true;
    hasted_ticks  = true;
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

// Shield Slam ==============================================================
struct shield_slam_t : public warrior_attack_t
{
  double rage_gain;
  shield_slam_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "shield_slam", p, p->spec.shield_slam ),
      rage_gain( data().effectN( 3 ).resource( RESOURCE_RAGE ) )
  {
    parse_options( options_str );
    energize_type = ENERGIZE_NONE;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.shield_block->up() )
    {
      double sb_increase = p() -> spell.shield_block_buff -> effectN( 2 ).percent();
      sb_increase += p() -> talents.heavy_repercussions -> effectN( 2 ).percent();
      am *= 1.0 + sb_increase;
    }

    if ( p() -> talents.punish -> ok() )
    {
      am *= 1.0 + p() -> talents.punish -> effectN( 1 ).percent();
    }

    return am;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double da = warrior_attack_t::bonus_da( state );

    da += p() -> buff.brace_for_impact -> stack() * p()->azerite.brace_for_impact.value(2);

    return da;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( p()->buff.shield_block->up() && p()->talents.heavy_repercussions->ok() )
    {
      p () -> buff.shield_block -> extend_duration( p(), 
          timespan_t::from_seconds( p() -> talents.heavy_repercussions -> effectN( 1 ).percent() ) );
    }
    
    p()->resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.shield_slam );

    if ( p()->sets->has_set_bonus( WARRIOR_PROTECTION, T20, B4 ) )
    {
      p()->cooldown.berserker_rage->adjust(
          timespan_t::from_seconds( -0.1 * p()->sets->set( WARRIOR_PROTECTION, T20, B4 )->effectN( 1 ).base_value() ),
          false );
    }
    if ( p() -> azerite.brace_for_impact.enabled() )
    {
      p() -> buff.brace_for_impact -> trigger();
    }
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    warrior_td_t* td = p() -> get_target_data( state -> target );

    if ( p() -> talents.punish -> ok() )
    {
      td -> debuffs_punish -> trigger();
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

// Slam =====================================================================

struct slam_t : public warrior_attack_t
{
  bool from_Fervor;
  slam_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "slam", p, p->spec.slam ), from_Fervor( false )
  {
    parse_options( options_str );
    weapon                       = &( p->main_hand_weapon );
    affected_by.crushing_assault = true;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double b = warrior_attack_t::bonus_da( s );
    if ( p()->buff.crushing_assault->check() )
    {
      b += p()->buff.crushing_assault->value();
    }
    return b;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.weighted_blade->stack_value();

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = warrior_attack_t::composite_crit_chance();

    cc += p()->buff.weighted_blade->stack_value();

    return cc;
  }

  double cost() const override
  {
    if ( from_Fervor )
      return 0;

    if ( p()->buff.crushing_assault->check() && !from_Fervor )
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
    p()->buff.weighted_blade->expire();
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

// Slaughter (T21 Fury Set Bonus) Dot
struct slaughter_dot_t : public residual_action::residual_periodic_action_t<warrior_spell_t>
{
  slaughter_dot_t( warrior_t* p ) : base_t( "T21 2PC", p, p->find_spell( 253384 ) )
  {
    dual          = true;
    tick_may_crit = false;
  }
};

// Shockwave ================================================================

struct shockwave_t : public warrior_attack_t
{
  shockwave_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "shockwave", p, p->spec.shockwave )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
    aoe                               = -1;
    
    base_multiplier *= 1.0 + p -> spec.prot_warrior -> effectN( 9 ).percent();
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( state -> n_targets >= as<size_t>( p() -> talents.rumbling_earth->effectN( 1 ).base_value() ) )
    {
      p() -> cooldown.shockwave -> adjust( timespan_t::from_seconds( p() -> talents.rumbling_earth -> effectN( 2 ).base_value() ) );
    }
  }
};

// Storm Bolt ===============================================================

struct storm_bolt_t : public warrior_attack_t
{
  storm_bolt_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "storm_bolt", p, p->talents.storm_bolt )
  {
    parse_options( options_str );
    may_dodge = may_parry = may_block = false;
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

// Thunder Clap =============================================================

struct thunder_clap_t : public warrior_attack_t
{
  double rage_gain;
  double shield_slam_reset;
  thunder_clap_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "thunder_clap", p, p->spec.thunder_clap ),
      rage_gain( data().effectN( 4 ).resource( RESOURCE_RAGE ) ),
      shield_slam_reset( p->spec.shield_slam_2->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    aoe       = -1;
    may_dodge = may_parry = may_block = false;

    radius *= 1.0 + p->talents.crackling_thunder->effectN( 1 ).percent();
    energize_type = ENERGIZE_NONE;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    if ( p() -> buff.avatar -> up() && p() -> talents.unstoppable_force -> ok() )
    {
      am *= 1.0 + p() -> talents.unstoppable_force -> effectN( 1 ).percent();
    }

    return am;
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = warrior_attack_t::bonus_da( s );

    if ( p() -> azerite.deafening_crash.enabled() )
    {
      da += p() -> azerite.deafening_crash.value( 2 );
    }

    return da;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( rng().roll( shield_slam_reset ) )
    {
      p()->cooldown.shield_slam->reset( true );
    }

    p()->resource_gain( RESOURCE_RAGE, rage_gain, p() -> gain.thunder_clap );    
  }

  double recharge_multiplier() const override
  {
    double rm = warrior_attack_t::recharge_multiplier();
    if ( p() -> buff.avatar -> up() && p() -> talents.unstoppable_force -> ok() )
    {
      rm *= 1.0 + ( p() -> talents.unstoppable_force -> effectN( 2 ).percent() );
    }
    return rm;
  }

  void impact( action_state_t* state ) override
  {
    warrior_attack_t::impact( state );

    if ( p() -> azerite.deafening_crash.enabled() && td( state -> target ) -> debuffs_demoralizing_shout -> up() )
    {
      td( state -> target ) -> debuffs_demoralizing_shout -> extend_duration( p(), timespan_t::from_millis( p() -> azerite.deafening_crash.spell() -> effectN( 1 ).base_value() ) );
    }
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

  victory_rush_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "victory_rush", p, p->spec.victory_rush ), victory_rush_heal( new victory_rush_heal_t( p ) )
  {
    parse_options( options_str );
    if ( p->non_dps_mechanics )
    {
      execute_action = victory_rush_heal;
    }
    cooldown->duration = timespan_t::from_seconds( 1000.0 );
    // Prot's victory rush deals less damage
    base_multiplier *= 1.0 + p -> spec.prot_warrior -> effectN( 9 ).percent();
  }
};

// Whirlwind ================================================================

struct whirlwind_off_hand_t : public warrior_attack_t
{
  whirlwind_off_hand_t( warrior_t* p, const spell_data_t* whirlwind ) : warrior_attack_t( "whirlwind_oh", p, whirlwind )
  {
    aoe = -1;
  }
};

struct fury_whirlwind_mh_t : public warrior_attack_t
{
  fury_whirlwind_mh_t( warrior_t* p, const spell_data_t* whirlwind ) : warrior_attack_t( "whirlwind_mh", p, whirlwind )
  {
    aoe = -1;
  }
};

struct fury_whirlwind_parent_t : public warrior_attack_t
{
  whirlwind_off_hand_t* oh_attack;
  fury_whirlwind_mh_t* mh_attack;
  timespan_t spin_time;
  double base_rage_gain;
  double additional_rage_gain_per_target;
  double enrage_chance;
  fury_whirlwind_parent_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "whirlwind", p, p->spec.whirlwind ),
      oh_attack( nullptr ),
      mh_attack( nullptr ),
      spin_time( timespan_t::from_millis( p->spec.whirlwind->effectN( 3 ).misc_value1() ) ),
      base_rage_gain( p->spec.whirlwind->effectN( 8 ).base_value() ),
      additional_rage_gain_per_target( p->spec.whirlwind->effectN( 9 ).base_value() ),
      enrage_chance( p->talents.meat_cleaver->effectN( 3 ).percent() )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack         = new fury_whirlwind_mh_t( p, data().effectN( 1 ).trigger() );
      mh_attack->weapon = &( p->main_hand_weapon );
      mh_attack->radius = radius;
      add_child( mh_attack );
      if ( p->off_hand_weapon.type != WEAPON_NONE )
      {
        oh_attack         = new whirlwind_off_hand_t( p, data().effectN( 2 ).trigger() );
        oh_attack->weapon = &( p->off_hand_weapon );
        oh_attack->radius = radius;
        add_child( oh_attack );
      }
    }
    tick_zero = true;
    callbacks = hasted_ticks = false;
    base_tick_time           = spin_time;
    dot_duration             = base_tick_time * 2;
  }

  timespan_t composite_dot_duration( const action_state_t* /* s */ ) const override
  {
    if ( p()->legendary.najentuss_vertebrae != spell_data_t::not_found() &&
         as<int>( target_list().size() ) >= p()->legendary.najentuss_vertebrae->effectN( 1 ).base_value() )
    {
      return dot_duration + ( base_tick_time * p()->legendary.najentuss_vertebrae->effectN( 2 ).base_value() );
    }

    return dot_duration;
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );

    if ( mh_attack )
    {
      mh_attack->execute();
      if ( oh_attack )
      {
        oh_attack->execute();
      }
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );

    p()->buff.meat_cleaver->trigger( p()->buff.meat_cleaver->data().max_stacks() );
  }

  void execute() override
  {
    warrior_attack_t::execute();
    const int num_available_targets = as<int>( target_list().size() );

    p()->resource_gain( RESOURCE_RAGE, ( base_rage_gain + additional_rage_gain_per_target * num_available_targets ),
                        p()->gain.whirlwind );

    if ( p()->talents.meat_cleaver->ok() )
    {
      p()->resource_gain( RESOURCE_RAGE,
                          std::min( p()->talents.meat_cleaver->effectN( 2 ).base_value(),
                                    p()->talents.meat_cleaver->effectN( 1 ).base_value() * num_available_targets ),
                          p()->gain.meat_cleaver );
    }
    if ( p()->talents.meat_cleaver->ok() )
    {
      if ( rng().roll( enrage_chance ) )
      {
        p()->enrage();
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

// Arms Whirlwind ========================================================

struct arms_whirlwind_mh_t : public warrior_attack_t
{
  arms_whirlwind_mh_t( warrior_t* p, const spell_data_t* whirlwind ) : warrior_attack_t( "whirlwind_mh", p, whirlwind )
  {
    aoe = -1;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.weighted_blade->stack_value();

    if ( p()->talents.fervor_of_battle->ok() )
    {
      am *= 1.0 + p()->talents.fervor_of_battle->effectN( 1 ).percent();
    }

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = warrior_attack_t::composite_crit_chance();

    cc += p()->buff.weighted_blade->stack_value();

    return cc;
  }
};

struct first_arms_whirlwind_mh_t : public warrior_attack_t
{
  first_arms_whirlwind_mh_t( warrior_t* p, const spell_data_t* whirlwind )
    : warrior_attack_t( "whirlwind_mh", p, whirlwind )
  {
    aoe = -1;
  }

  double action_multiplier() const override
  {
    double am = warrior_attack_t::action_multiplier();

    am *= 1.0 + p()->buff.weighted_blade->stack_value();

    if ( p()->talents.fervor_of_battle->ok() )
    {
      am *= 1.0 + p()->talents.fervor_of_battle->effectN( 1 ).percent();
    }

    return am;
  }

  double composite_crit_chance() const override
  {
    double cc = warrior_attack_t::composite_crit_chance();

    cc += p()->buff.weighted_blade->stack_value();

    return cc;
  }
};

struct arms_whirlwind_parent_t : public warrior_attack_t
{
  slam_t* fervor_slam;
  first_arms_whirlwind_mh_t* first_mh_attack;
  arms_whirlwind_mh_t* mh_attack;
  timespan_t spin_time;
  arms_whirlwind_parent_t( warrior_t* p, const std::string& options_str )
    : warrior_attack_t( "whirlwind", p, p->spec.whirlwind ),
      fervor_slam( nullptr ),
      first_mh_attack( nullptr ),
      mh_attack( nullptr ),
      spin_time( timespan_t::from_millis( p->spec.whirlwind->effectN( 2 ).misc_value1() ) )
  {
    parse_options( options_str );
    radius = data().effectN( 1 ).trigger()->effectN( 1 ).radius_max();

    if ( p->talents.fervor_of_battle->ok() )
    {
      fervor_slam                               = new slam_t( p, options_str );
      fervor_slam->from_Fervor                  = true;
      fervor_slam->affected_by.crushing_assault = true;
    }

    if ( p->main_hand_weapon.type != WEAPON_NONE )
    {
      mh_attack         = new arms_whirlwind_mh_t( p, data().effectN( 2 ).trigger() );
      mh_attack->weapon = &( p->main_hand_weapon );
      mh_attack->radius = radius;
      add_child( mh_attack );
      first_mh_attack         = new first_arms_whirlwind_mh_t( p, data().effectN( 1 ).trigger() );
      first_mh_attack->weapon = &( p->main_hand_weapon );
      first_mh_attack->radius = radius;
      add_child( first_mh_attack );
    }
    tick_zero = true;
    callbacks = hasted_ticks = false;
    base_tick_time           = spin_time;
    dot_duration             = base_tick_time * 2;
  }

  timespan_t composite_dot_duration( const action_state_t* /* s */ ) const override
  {
    if ( p()->legendary.najentuss_vertebrae != spell_data_t::not_found() &&
         as<int>( target_list().size() ) >= p()->legendary.najentuss_vertebrae->effectN( 1 ).base_value() )
    {
      return dot_duration + ( base_tick_time * p()->legendary.najentuss_vertebrae->effectN( 2 ).base_value() );
    }

    return dot_duration;
  }

  double cost() const override
  {
    if ( p()->talents.fervor_of_battle->ok() && p()->buff.crushing_assault->check() )
      return 10;
    return warrior_attack_t::cost();
  }

  void tick( dot_t* d ) override
  {
    warrior_attack_t::tick( d );

    if ( d->current_tick == 1 )
    {
      if ( p()->talents.fervor_of_battle->ok() )
      {
        fervor_slam->execute();
      }
      first_mh_attack->execute();
    }
    else
    {
      mh_attack->execute();
    }
  }

  void last_tick( dot_t* d ) override
  {
    warrior_attack_t::last_tick( d );

    p()->buff.weighted_blade->expire();
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

// ==========================================================================
// Warrior Spells
// ==========================================================================

// Avatar ===================================================================

struct avatar_t : public warrior_spell_t
{
  avatar_t( warrior_t* p, const std::string& options_str ) :
    warrior_spell_t( "avatar", p, p -> specialization() == WARRIOR_PROTECTION ? p -> spec.avatar : p -> talents.avatar )
  {
    parse_options( options_str );
    callbacks = false;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.avatar->trigger();

    if ( p()->azerite.bastion_of_might.enabled() )
    {
      p()->buff.bastion_of_might->trigger();

      // TODO: check if Arms warriors get an ignore pain too
      if ( p() -> specialization() == WARRIOR_PROTECTION )
        p() -> active.bastion_of_might_ip -> execute( );
    }
  }

  bool verify_actor_spec() const override
  {
    if ( p()->talents.avatar->ok() && p() -> specialization() == WARRIOR_ARMS )
    {
      // Do not check spec if Arms talent avatar is available, so that spec check on the spell (required: protection)
      // does not fail.
      return true;
    }
    else
    {
      return warrior_spell_t::verify_actor_spec();
    }
  }
};

// Battle Shout ===================================================================

struct battle_shout_t : public warrior_spell_t
{
  battle_shout_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "battle_shout", p, p->spell.battle_shout )
  {
    parse_options( options_str );
    harmful = false;

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
  berserker_rage_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "berserker_rage", p, p->spec.berserker_rage )
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

    if ( p()->sets->has_set_bonus( WARRIOR_PROTECTION, T20, B2 ) )
    {
      p()->resource_gain(
          RESOURCE_RAGE,
          p()->sets->set( WARRIOR_PROTECTION, T20, B2 )->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ),
          p()->gain.protection_t20_2p );
      p()->buff.protection_rage->trigger();
    }
  }
};

// Deadly Calm ===================================================================

struct deadly_calm_t : public warrior_spell_t
{
  deadly_calm_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "deadly_calm", p, p->talents.deadly_calm )
  {
    parse_options( options_str );
    callbacks   = false;
    use_off_gcd = true;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.deadly_calm->trigger();
  }
};

// Defensive Stance ===============================================================

struct defensive_stance_t : public warrior_spell_t
{
  std::string onoff;
  bool onoffbool;
  defensive_stance_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "defensive_stance", p, p->talents.defensive_stance ), onoff( nullptr ), onoffbool( 0 )
  {
    add_option( opt_string( "toggle", onoff ) );
    parse_options( options_str );

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
  die_by_the_sword_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "die_by_the_sword", p, p->spec.die_by_the_sword )
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

  in_for_the_kill_t( warrior_t& p, const std::string& n, const spell_data_t* s )
    : buff_t( &p, n, s ),
      base_value( p.talents.in_for_the_kill->effectN( 1 ).percent() ),
      below_pct_increase_amount( p.talents.in_for_the_kill->effectN( 2 ).percent() ),
      below_pct_increase( p.talents.in_for_the_kill->effectN( 3 ).base_value() )

  {
    add_invalidate( CACHE_ATTACK_HASTE );
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
  last_stand_t( warrior_t* p, const std::string& options_str ) : warrior_spell_t( "last_stand", p, p->spec.last_stand )
  {
    parse_options( options_str );
    range              = -1;
    cooldown->duration = data().cooldown();
    cooldown->duration *= 1.0 + p->sets->set( WARRIOR_PROTECTION, T18, B2 )->effectN( 1 ).percent();
    cooldown -> duration += timespan_t::from_millis( p -> talents.bolster -> effectN( 1 ).base_value() );
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.last_stand->trigger();
  }
};

// Rallying Cry ===============================================================

struct rallying_cry_t : public warrior_spell_t
{
  rallying_cry_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "rallying_cry", p, p->spec.rallying_cry )
  {
    parse_options( options_str );
    callbacks = false;
    range     = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.rallying_cry->trigger();
  }
};

// Recklessness =============================================================

struct recklessness_t : public warrior_spell_t
{
  double bonus_crit;
  recklessness_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "recklessness", p, p->spec.recklessness ), bonus_crit( 0.0 )
  {
    parse_options( options_str );
    bonus_crit = data().effectN( 1 ).percent();
    callbacks  = false;

    harmful = false;

    if ( p->talents.reckless_abandon->ok() )
    {
      energize_amount   = p->talents.reckless_abandon->effectN( 2 ).base_value() / 10.0;
      energize_type     = ENERGIZE_ON_CAST;
      energize_resource = RESOURCE_RAGE;
    }
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.recklessness->trigger();
  }
};

// Ignore Pain =============================================================

struct ignore_pain_buff_t : public absorb_buff_t
{
  ignore_pain_buff_t( warrior_t* player ) : absorb_buff_t( player, "ignore_pain", player->spec.ignore_pain )
  {
    set_absorb_source( player->get_stats( "ignore_pain" ) );
    set_absorb_gain( player->get_gain( "ignore_pain" ) );
  }

  // Custom consume implementation to allow minimum absorb amount.
  double consume( double amount ) override
  {
    // IP only absorbs up to 50% of the damage taken
    amount *= debug_cast< warrior_t* >( player ) -> spec.ignore_pain -> effectN( 2 ).percent();
    double absorbed = absorb_buff_t::consume( amount );

    return absorbed;
  }
};

struct ignore_pain_t : public warrior_spell_t
{
  ignore_pain_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "ignore_pain", p, p->spec.ignore_pain )
  {
    parse_options( options_str );
    may_crit     = false;
    range        = -1;
    target       = player;

    base_dd_max = base_dd_min = 0;
  }

  double bonus_da( const action_state_t* state ) const override
  {
    double da = warrior_spell_t::bonus_da( state );

    if ( p() -> azerite.bloodsport.enabled() )
    {
      da += p() -> azerite.bloodsport.value( 3 );
    }

    return da;
  }

  double cost() const override
  {
    double c = warrior_spell_t::cost();

    c *= 1.0 + p() -> buff.vengeance_ignore_pain -> value();

    return c;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.vengeance_ignore_pain->expire();
    p()->buff.vengeance_revenge->trigger();
    if ( p() -> azerite.bloodsport.enabled() )
    {
      p() -> buff.bloodsport -> trigger();
    }
  }

  void impact( action_state_t* s ) override
  {
    double new_ip = s -> result_amount;

    if ( p() -> talents.never_surrender -> ok() )
    { 
      double percent_health;
      if ( p() -> never_surrender_percentage < 0 )
      {
        percent_health = p() -> health_percentage() / 100;
      }
      else
      {
        percent_health = ( rng().gauss( p() -> never_surrender_percentage / 100, 0.2 ) );
      }

      new_ip *= 1.0 + percent_health * p() -> talents.never_surrender -> effectN( 1 ).percent();
    }

    double previous_ip = p() -> buff.ignore_pain -> current_value;

    // IP is capped at 1.3 times the amount of the current IP
    double ip_cap_ratio = 1.3;

    if ( previous_ip + new_ip > new_ip * ip_cap_ratio )
    {
      new_ip *= ip_cap_ratio;
    }

    if ( new_ip > 0.0 )
    {
      p()->buff.ignore_pain->trigger( 1, new_ip );
    }
  }

  bool ready() override
  {
    if ( !p()->has_shield_equipped() )
      return false;

    return warrior_spell_t::ready();
  }
};

// A free Ignore Pain from Bastion of Might when Avatar is cast
struct ignore_pain_bom_t : public ignore_pain_t
{
  ignore_pain_bom_t( warrior_t* p )
    : ignore_pain_t( p, "" )
  {
    may_crit     = false;
    range        = -1;
    target       = player;
  }

  void execute() override
  {
    warrior_spell_t::execute();
    p()->buff.vengeance_revenge->trigger(); // this IP does trigger vengeance but doesnt consume it
  }

  double cost( ) const override
  {
    return 0.0;
  }

};

// Shield Block =============================================================

struct shield_block_t : public warrior_spell_t
{
  shield_block_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "shield_block", p, p->spec.shield_block )
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
      p()->buff.shield_block->extend_duration( p(), p()->buff.shield_block->data().duration() );
    }
    else
    {
      p()->buff.shield_block->trigger();
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
  shield_wall_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "shield_wall", p, p->spec.shield_wall )
  {
    parse_options( options_str );
    harmful = false;
    range   = -1;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.shield_wall->trigger( 1, p()->buff.shield_wall->data().effectN( 1 ).percent() );
  }
};

// Spell Reflection  ==============================================================

struct spell_reflection_t : public warrior_spell_t
{
  spell_reflection_t( warrior_t* p, const std::string& options_str )
    : warrior_spell_t( "spell_reflection", p, p->spec.spell_reflection )
  {
    parse_options( options_str );
    use_off_gcd = true;
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
  taunt_t( warrior_t* p, const std::string& options_str )
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

action_t* warrior_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "avatar" )
    return new avatar_t( this, options_str );
  if ( name == "battle_shout" )
    return new battle_shout_t( this, options_str );
  if ( name == "recklessness" )
    return new recklessness_t( this, options_str );
  if ( name == "berserker_rage" )
    return new berserker_rage_t( this, options_str );
  if ( name == "bladestorm" )
    return new bladestorm_t( this, options_str );
  if ( name == "bloodthirst" )
    return new bloodthirst_t( this, options_str );
  if ( name == "charge" )
    return new charge_t( this, options_str );
  if ( name == "cleave" )
    return new cleave_t( this, options_str );
  if ( name == "colossus_smash" )
    return new colossus_smash_t( this, options_str );
  if ( name == "deadly_calm" )
    return new deadly_calm_t( this, options_str );
  if ( name == "defensive_stance" )
    return new defensive_stance_t( this, options_str );
  if ( name == "demoralizing_shout" )
    return new demoralizing_shout( this, options_str );
  if ( name == "devastate" )
    return new devastate_t( this, options_str );
  if ( name == "die_by_the_sword" )
    return new die_by_the_sword_t( this, options_str );
  if ( name == "dragon_roar" )
    return new dragon_roar_t( this, options_str );
  if ( name == "enraged_regeneration" )
    return new enraged_regeneration_t( this, options_str );
  if ( name == "execute" )
  {
    if ( specialization() == WARRIOR_ARMS )
    {
      return new execute_arms_t( this, options_str );
    }
    else
    {
      return new fury_execute_parent_t( this, options_str );
    }
  }
  if ( name == "furious_slash" )
    return new furious_slash_t( this, options_str );
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
  if ( name == "overpower" )
    return new overpower_t( this, options_str );
  if ( name == "piercing_howl" )
    return new piercing_howl_t( this, options_str );
  if ( name == "pummel" )
    return new pummel_t( this, options_str );
  if ( name == "raging_blow" )
    return new raging_blow_t( this, options_str );
  if ( name == "rallying_cry" )
    return new rallying_cry_t( this, options_str );
  if ( name == "rampage" )
    return new rampage_parent_t( this, options_str );
  if ( name == "ravager" )
    return new ravager_t( this, options_str );
  if ( name == "rend" )
    return new rend_t( this, options_str );
  if ( name == "revenge" )
    return new revenge_t( this, options_str );
  if ( name == "shield_block" )
    return new shield_block_t( this, options_str );
  if ( name == "shield_slam" )
    return new shield_slam_t( this, options_str );
  if ( name == "shield_wall" )
    return new shield_wall_t( this, options_str );
  if ( name == "shockwave" )
    return new shockwave_t( this, options_str );
  if ( name == "siegebreaker" )
    return new siegebreaker_t( this, options_str );
  if ( name == "skullsplitter" )
    return new skullsplitter_t( this, options_str );
  if ( name == "slam" )
    return new slam_t( this, options_str );
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
  if ( name == "victory_rush" )
    return new victory_rush_t( this, options_str );
  if ( name == "warbreaker" )
    return new warbreaker_t( this, options_str );
  if ( name == "ignore_pain" )
    return new ignore_pain_t( this, options_str );
  if ( name == "intercept" )
    return new intercept_t( this, options_str );
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

  return player_t::create_action( name, options_str );
}

// warrior_t::init_spells ===================================================

void warrior_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.critical_block   = find_mastery_spell( WARRIOR_PROTECTION );
  mastery.deep_wounds_ARMS = find_mastery_spell( WARRIOR_ARMS );
  mastery.unshackled_fury  = find_mastery_spell( WARRIOR_FURY );

  // Spec Passives
  spec.berserker_rage       = find_specialization_spell( "Berserker Rage" );
  spec.bladestorm           = find_specialization_spell( "Bladestorm" );
  spec.bloodthirst          = find_specialization_spell( "Bloodthirst" );
  spec.colossus_smash       = find_specialization_spell( "Colossus Smash" );
  spec.deep_wounds_ARMS     = find_specialization_spell( "Mastery: Deep Wounds", WARRIOR_ARMS );
  spec.deep_wounds_PROT     = find_specialization_spell( "Deep Wounds", WARRIOR_PROTECTION );
  spec.demoralizing_shout   = find_specialization_spell( "Demoralizing Shout" );
  spec.devastate            = find_specialization_spell( "Devastate" );
  spec.die_by_the_sword     = find_specialization_spell( "Die By the Sword" );
  spec.enrage               = find_specialization_spell( "Enrage" );
  spec.enraged_regeneration = find_specialization_spell( "Enraged Regeneration" );
  spec.execute              = find_specialization_spell( "Execute" );
  if ( specialization() == WARRIOR_FURY )
  {
    spec.execute_2 = find_specialization_spell( 231827 );
  }
  else
  {
    spec.execute_2 = find_specialization_spell( 231830 );
  }
  spec.hamstring        = find_specialization_spell( "Hamstring" );
  spec.ignore_pain      = find_specialization_spell( "Ignore Pain" );
  spec.intercept        = find_specialization_spell( "Intercept" );
  spec.last_stand       = find_specialization_spell( "Last Stand" );
  spec.mortal_strike    = find_specialization_spell( "Mortal Strike" );
  spec.overpower        = find_specialization_spell( "Overpower" );
  spec.piercing_howl    = find_specialization_spell( "Piercing Howl" );
  spec.raging_blow      = find_specialization_spell( "Raging Blow" );
  spec.rampage          = find_specialization_spell( "Rampage" );
  spec.rallying_cry     = find_specialization_spell( "Rallying Cry" );
  spec.recklessness     = find_specialization_spell( "Recklessness" );
  spec.revenge          = find_specialization_spell( "Revenge" );
  spec.revenge_trigger  = find_specialization_spell( "Revenge Trigger" );
  spec.seasoned_soldier = find_specialization_spell( "Seasoned Soldier" );
  spec.shield_block     = find_specialization_spell( "Shield Block" );
  spec.shield_block_2   = find_specialization_spell( 231847 );
  spec.shield_slam      = find_specialization_spell( "Shield Slam" );
  spec.shield_slam_2    = find_specialization_spell( 231834 );
  spec.shield_wall      = find_specialization_spell( "Shield Wall" );
  spec.shockwave        = find_specialization_spell( "Shockwave" );
  spec.slam             = find_specialization_spell( "Slam" );
  spec.spell_reflection = find_specialization_spell( "Spell Reflection" );
  spec.sweeping_strikes = find_specialization_spell( "Sweeping Strikes" );
  spec.tactician        = find_specialization_spell( "Tactician" );
  spec.thunder_clap     = find_specialization_spell( "Thunder Clap" );
  spec.victory_rush     = find_specialization_spell( "Victory Rush" );
  // Only for Protection, the arms talent is under talents.avatar
  spec.avatar           = find_specialization_spell( "Avatar" );
  spec.vanguard         = find_specialization_spell( "Vanguard" );
  if ( specialization() == WARRIOR_FURY )
  {
    spec.whirlwind   = find_specialization_spell( 190411 );
    spec.whirlwind_2 = find_specialization_spell( "Whirlwind" );
  }
  else
  {
    spec.whirlwind = find_specialization_spell( "Whirlwind" );
  }
  spec.fury_warrior = find_specialization_spell( "Fury Warrior" );
  spec.arms_warrior = find_specialization_spell( "Arms Warrior" );
  spec.prot_warrior = find_specialization_spell( "Protection Warrior" );

  // Talents
  talents.anger_management    = find_talent_spell( "Anger Management" );
  // Only for Arms, the Protection baseline spell is under spec.avatar
  talents.avatar              = find_talent_spell( "Avatar" );
  talents.best_served_cold    = find_talent_spell( "Best Served Cold" );
  talents.bladestorm          = find_talent_spell( "Bladestorm" );
  talents.bolster             = find_talent_spell( "Bolster" );
  talents.booming_voice       = find_talent_spell( "Booming Voice" );
  talents.bounding_stride     = find_talent_spell( "Bounding Stride" );
  talents.carnage             = find_talent_spell( "Carnage" );
  talents.cleave              = find_talent_spell( "Cleave" );
  talents.collateral_damage   = find_talent_spell( "Collateral Damage" );
  talents.crackling_thunder   = find_talent_spell( "Crackling Thunder" );
  talents.deadly_calm         = find_talent_spell( "Deadly Calm" );
  talents.defensive_stance    = find_talent_spell( "Defensive Stance" );
  talents.devastator          = find_talent_spell( "Devastator" );
  talents.double_time         = find_talent_spell( "Double Time" );
  talents.dragon_roar         = find_talent_spell( "Dragon Roar" );
  talents.dreadnaught         = find_talent_spell( "Dreadnaught" );
  talents.endless_rage        = find_talent_spell( "Endless Rage" );
  talents.fervor_of_battle    = find_talent_spell( "Fervor of Battle" );
  talents.fresh_meat          = find_talent_spell( "Fresh Meat" );
  talents.frothing_berserker  = find_talent_spell( "Frothing Berserker" );
  talents.furious_charge      = find_talent_spell( "Furious Charge" );
  talents.furious_slash       = find_talent_spell( "Furious Slash" );
  talents.heavy_repercussions = find_talent_spell( "Heavy Repercussions" );
  talents.impending_victory   = find_talent_spell( "Impending Victory" );
  talents.in_for_the_kill     = find_talent_spell( "In For The Kill" );
  talents.indomitable         = find_talent_spell( "Indomitable" );
  talents.inner_rage          = find_talent_spell( "Inner Rage" );
  talents.into_the_fray       = find_talent_spell( "Into the Fray" );
  talents.massacre            = find_talent_spell( "Massacre" );
  talents.meat_cleaver        = find_talent_spell( "Meat Cleaver" );
  talents.menace              = find_talent_spell( "Menace" );
  talents.never_surrender     = find_talent_spell( "Never Surrender" );
  talents.punish              = find_talent_spell( "Punish" );
  talents.ravager             = find_talent_spell( "Ravager" );
  talents.reckless_abandon    = find_talent_spell( "Reckless Abandon" );
  talents.rend                = find_talent_spell( "Rend" );
  talents.rumbling_earth      = find_talent_spell( "Rumbling Earth" );
  talents.safeguard           = find_talent_spell( "Safeguard" );
  talents.second_wind         = find_talent_spell( "Second Wind" );
  talents.siegebreaker        = find_talent_spell( "Siegebreaker" );
  talents.skullsplitter       = find_talent_spell( "Skullsplitter" );
  talents.storm_bolt          = find_talent_spell( "Storm Bolt" );
  talents.sudden_death        = find_talent_spell( "Sudden Death" );
  talents.unstoppable_force   = find_talent_spell( "Unstoppable Force" );
  talents.vengeance           = find_talent_spell( "Vengeance" );
  talents.war_machine         = find_talent_spell( "War Machine" );
  talents.warbreaker          = find_talent_spell( "Warbreaker" );
  talents.warpaint            = find_talent_spell( "Warpaint" );

  // All
  azerite.breach           = find_azerite_spell( "Breach" );
  azerite.moment_of_glory  = find_azerite_spell( "Moment of Glory" );
  azerite.bury_the_hatchet = find_azerite_spell( "Bury the Hatchet" );
  // Prot
  azerite.iron_fortress      = find_azerite_spell( "Iron Fortress" );
  azerite.deafening_crash    = find_azerite_spell( "Deafening Crash" );
  azerite.callous_reprisal   = find_azerite_spell( "Callous Reprisal" );
  azerite.brace_for_impact   = find_azerite_spell( "Brace for Impact" );
  azerite.bloodsport         = find_azerite_spell( "Bloodsport" );
  azerite.bastion_of_might   = find_azerite_spell( "Bastion of Might" );
  // Arms
  azerite.test_of_might          = find_azerite_spell( "Test of Might" );
  azerite.seismic_wave           = find_azerite_spell( "Seismic Wave" );
  azerite.lord_of_war            = find_azerite_spell( "Lord of War" );
  azerite.gathering_storm        = find_azerite_spell( "Gathering Storm" );
  azerite.executioners_precision = find_azerite_spell( "Executioner's Precision" );
  azerite.crushing_assault       = find_azerite_spell( "Crushing Assault" );
  azerite.striking_the_anvil     = find_azerite_spell( "Striking the Anvil" );
  // Fury
  azerite.trample_the_weak  = find_azerite_spell( "Trample the Weak" );
  azerite.simmering_rage    = find_azerite_spell( "Simmering Rage" );
  azerite.reckless_flurry   = find_azerite_spell( "Reckless Flurry" );
  azerite.pulverizing_blows = find_azerite_spell( "Pulverizing Blows" );
  azerite.infinite_fury     = find_azerite_spell( "Infinite fury" );
  azerite.bloodcraze        = find_azerite_spell( "Bloodcraze" );
  azerite.cold_steel_hot_blood   = find_azerite_spell( "Cold Steel, Hot Blood" );
  azerite.unbridled_ferocity     = find_azerite_spell( "Unbridled Ferocity" );

  // Generic spells
  spell.battle_shout          = find_class_spell( "Battle Shout" );
  spell.charge                = find_class_spell( "Charge" );
  spell.colossus_smash_debuff = find_spell( 208086 );
  spell.intervene             = find_spell( 147833 );
  spell.headlong_rush         = find_spell( 137047 );  // Also may be used for other crap in the future.
  spell.heroic_leap           = find_class_spell( "Heroic Leap" );
  spell.siegebreaker_debuff   = find_spell( 280773 );
  spell.whirlwind_buff        = find_spell( 85739, WARRIOR_FURY );  // Used to be called Meat Cleaver
  spell.ravager_protection    = find_spell( 227744 );
  spell.shield_block_buff     = find_spell( 132404 );
  spell.riposte               = find_class_spell( "Riposte" );


  // Active spells
  active.deep_wounds_ARMS = nullptr;
  active.deep_wounds_PROT = nullptr;
  active.charge           = nullptr;
  active.slaughter        = nullptr;

  auto_attack_multiplier *= 1.0 + spec.fury_warrior->effectN( 4 ).percent();

  if ( spec.deep_wounds_ARMS->ok() )
    active.deep_wounds_ARMS = new deep_wounds_ARMS_t( this );
  if ( spec.deep_wounds_PROT->ok() )
    active.deep_wounds_PROT = new deep_wounds_PROT_t( this );
  if ( sets->has_set_bonus( WARRIOR_FURY, T21, B2 ) )
    active.slaughter = new slaughter_dot_t( this );
  if ( spec.rampage->ok() )
  {
    // rampage now hits 4 times instead of 5 and effect indexes shifted
    rampage_attack_t* first  = new rampage_attack_t( this, spec.rampage->effectN( 2 ).trigger(), "rampage1" );
    rampage_attack_t* second = new rampage_attack_t( this, spec.rampage->effectN( 3 ).trigger(), "rampage2" );
    rampage_attack_t* third  = new rampage_attack_t( this, spec.rampage->effectN( 4 ).trigger(), "rampage3" );
    rampage_attack_t* fourth = new rampage_attack_t( this, spec.rampage->effectN( 5 ).trigger(), "rampage4" );

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
  if ( azerite.iron_fortress.enabled() )
  {
    active.iron_fortress = new iron_fortress_t( this );
  }
  if ( azerite.bastion_of_might.enabled() && specialization() == WARRIOR_PROTECTION )
  {
    active.bastion_of_might_ip = new ignore_pain_bom_t( this );
  }

  // Cooldowns
  cooldown.avatar         = get_cooldown( "avatar" );
  cooldown.recklessness   = get_cooldown( "recklessness" );
  cooldown.berserker_rage = get_cooldown( "berserker_rage" );
  cooldown.bladestorm     = get_cooldown( "bladestorm" );
  cooldown.bloodthirst    = get_cooldown( "bloodthirst" );

  cooldown.charge                           = get_cooldown( "charge" );
  cooldown.colossus_smash                   = get_cooldown( "colossus_smash" );
  cooldown.deadly_calm                      = get_cooldown( "deadly_calm" );
  cooldown.demoralizing_shout               = get_cooldown( "demoralizing_shout" );
  cooldown.dragon_roar                      = get_cooldown( "dragon_roar" );
  cooldown.enraged_regeneration             = get_cooldown( "enraged_regeneration" );
  cooldown.execute                          = get_cooldown( "execute" );
  cooldown.heroic_leap                      = get_cooldown( "heroic_leap" );
  cooldown.iron_fortress_icd                = get_cooldown( "iron_fortress" );
  cooldown.last_stand                       = get_cooldown( "last_stand" );
  cooldown.mortal_strike                    = get_cooldown( "mortal_strike" );
  cooldown.overpower                        = get_cooldown( "overpower" );
  cooldown.rage_from_auto_attack            = get_cooldown( "rage_from_auto_attack" );
  cooldown.rage_from_auto_attack->duration  = timespan_t::from_seconds ( 1.0 );
  cooldown.rage_from_crit_block             = get_cooldown( "rage_from_crit_block" );
  cooldown.rage_from_crit_block->duration   = timespan_t::from_seconds( 3.0 );
  cooldown.raging_blow                      = get_cooldown( "raging_blow" );
  cooldown.ravager                          = get_cooldown( "ravager" );
  cooldown.revenge_reset                    = get_cooldown( "revenge_reset" );
  cooldown.revenge_reset->duration          = spec.revenge_trigger->internal_cooldown();
  cooldown.shield_slam                      = get_cooldown( "shield_slam" );
  cooldown.shield_wall                      = get_cooldown( "shield_wall" );
  cooldown.siegebreaker                     = get_cooldown( "siegebreaker" );
  cooldown.skullsplitter                    = get_cooldown( "skullsplitter" );
  cooldown.shockwave                        = get_cooldown( "shockwave" );
  cooldown.storm_bolt                       = get_cooldown( "storm_bolt" );
  cooldown.warbreaker                       = get_cooldown( "warbreaker" );
}

// warrior_t::init_base =====================================================

void warrior_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 5.0;

  player_t::init_base_stats();

  resources.base[ RESOURCE_RAGE ] = 100;
  resources.max[ RESOURCE_RAGE ]  = resources.base[ RESOURCE_RAGE ];

  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 0.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here.

  base_gcd = timespan_t::from_seconds( 1.5 );

  resources.base_multiplier[ RESOURCE_HEALTH ] *= 1 + talents.indomitable->effectN( 1 ).percent();

  if ( specialization() == WARRIOR_PROTECTION )
  {
    if ( items.size() > 0 )
    {
      double totalweight         = 0;
      double avg_weighted_ilevel = 0;
      size_t divisor             = 0;
      for ( size_t i = 0; i < items.size(); i++ )
      {
        if ( items[ i ].slot == SLOT_SHIRT || items[ i ].slot == SLOT_TABARD || !items[ i ].active() )
        {
          continue;
        }
        const auto& data = dbc.random_property( items[ i ].item_level() );
        if ( data.p_epic[ 0 ] == 0 )
        {
          continue;
        }

        double ratio = data.p_epic[ item_database::random_suffix_type( items[ i ] ) ] / data.p_epic[ 0 ];
        totalweight += ratio;
        divisor++;
      }
      for ( size_t i = 0; i < items.size(); i++ )
      {
        if ( items[ i ].slot == SLOT_SHIRT || items[ i ].slot == SLOT_TABARD || !items[ i ].active() )
        {
          continue;
        }
        const auto& data = dbc.random_property( items[ i ].item_level() );
        if ( data.p_epic[ 0 ] == 0 )
        {
          continue;
        }

        double ratio = data.p_epic[ item_database::random_suffix_type( items[ i ] ) ] / data.p_epic[ 0 ];
        avg_weighted_ilevel += ( ratio * static_cast<double>( items[ i ].item_level() ) / totalweight * divisor );
      }


      int average_itemlevel = static_cast<int>( divisor ? avg_weighted_ilevel / divisor : 0.0 );

      if (average_itemlevel > 0 )
      {

        const auto& data = dbc.random_property( static_cast<int>( average_itemlevel ) );

        expected_max_health = data.p_epic[ 0 ] * 8.484262;
        expected_max_health += base.stats.attribute[ ATTR_STAMINA ];
        expected_max_health *= 1.0 + matching_gear_multiplier( ATTR_STAMINA );
        expected_max_health *= 60;

      }
    }
  }
}

// warrior_t::merge ==========================================================

void warrior_t::merge( player_t& other )
{
  player_t::merge( other );

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

  player_t::datacollection_begin();
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

  player_t::datacollection_end();
}

// Pre-combat Action Priority List============================================

void warrior_t::default_apl_dps_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );

  precombat->add_action( "food" );

  precombat->add_action( "augmentation" );

  precombat->add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  precombat->add_action( "potion" );
  
  if ( specialization() == WARRIOR_FURY )
  {
    precombat->add_action( this, "Recklessness", "if=!talent.furious_slash.enabled&!talent.reckless_abandon.enabled" );
  }
}

// Fury Warrior Action Priority List ========================================

void warrior_t::apl_fury()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  default_apl_dps_precombat();
  action_priority_list_t* default_list  = get_action_priority_list( "default" );
  action_priority_list_t* movement      = get_action_priority_list( "movement" );
  action_priority_list_t* single_target = get_action_priority_list( "single_target" );

  default_list->add_action( "auto_attack" );
  default_list->add_action( this, "Charge" );
  default_list->add_action( "run_action_list,name=movement,if=movement.distance>5",
                            "This is mostly to prevent cooldowns from being accidentally used during movement." );
  default_list->add_action(
      this, "Heroic Leap",
      "if=(raid_event.movement.distance>25&raid_event.movement.in>45)|!raid_event.movement.exists" );

  if ( sim->allow_potions && true_level >= 80 )
  {
    default_list->add_action( "potion" );
  }

  default_list->add_talent( this, "Furious Slash",
                            "if=talent.furious_slash.enabled&(buff.furious_slash.stack<3|buff.furious_slash.remains<3|("
                            "cooldown.recklessness.remains<3&buff.furious_slash.remains<9))" );
  default_list->add_action( this, "Rampage", "if=cooldown.recklessness.remains<3" );
  default_list->add_action( this, "Recklessness", "if=!talent.siegebreaker.enabled|(cooldown.siegebreaker.remains<1|"
                            "cooldown.siegebreaker.remains>5)" );
  default_list->add_action( this, "Whirlwind", "if=spell_targets.whirlwind>1&!buff.meat_cleaver.up" );

  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].name_str == "grongs_primal_rage" )
    {
      default_list->add_action( "use_item,name=" + items[ i ].name_str +
                                ",if=equipped.grongs_primal_rage&buff.enrage.up&!buff.recklessness.up" );
    }
    else if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      if ( items[ i ].slot != SLOT_WAIST )
        default_list->add_action( "use_item,name=" + items[ i ].name_str );
    }
  }

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
    {
      // While it's on the GCD, arcane torrent wasting a global
      // is a dps decrease.
      // default_list->add_action( racial_actions[ i ] + ",if=rage<40&!buff.recklessness.up" );
    }
    else if ( racial_actions[ i ] == "lights_judgment" )
    {
      default_list->add_action( racial_actions[ i ] + ",if=buff.recklessness.down" );
    }
    else
    {
      default_list->add_action( racial_actions[ i ] );
    }
  }
  default_list->add_action( "run_action_list,name=single_target" );

  movement->add_action( this, "Heroic Leap" );

  single_target->add_talent( this, "Siegebreaker" );
  single_target->add_action( this, "Rampage",
                             "if=buff.recklessness.up|(talent.frothing_berserker.enabled|talent.carnage.enabled&(buff."
                             "enrage.remains<gcd|rage>90)|talent.massacre.enabled&(buff.enrage.remains<gcd|rage>90))" );
  single_target->add_action( this, "Execute" );
  single_target->add_talent( this, "Bladestorm",  "if=prev_gcd.1.rampage" );
  single_target->add_action( this, "Bloodthirst", "if=buff.enrage.down|azerite.cold_steel_hot_blood.rank>1" );
  single_target->add_action( this, "Raging Blow", "if=charges=2" );
  single_target->add_action( this, "Bloodthirst" );
  single_target->add_talent( this, "Dragon Roar", "if=buff.enrage.up" );
  single_target->add_action(
      this, "Raging Blow",
      "if=talent.carnage.enabled|(talent.massacre.enabled&rage<80)|(talent.frothing_berserker.enabled&rage<90)" );
  single_target->add_talent( this, "Furious Slash", "if=talent.furious_slash.enabled" );
  single_target->add_action( this, "Whirlwind" );
}

// Arms Warrior Action Priority List ========================================

void warrior_t::apl_arms()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  default_apl_dps_precombat();
  action_priority_list_t* default_list  = get_action_priority_list( "default" );
  action_priority_list_t* hac           = get_action_priority_list( "hac" );
  action_priority_list_t* five_target   = get_action_priority_list( "five_target" );
  action_priority_list_t* execute       = get_action_priority_list( "execute" );
  action_priority_list_t* single_target = get_action_priority_list( "single_target" );

  default_list->add_action( this, "Charge" );
  default_list->add_action( "auto_attack" );

  if ( sim->allow_potions && true_level >= 80 )
  {
    default_list->add_action( "potion" );
  //default_list->add_action( "potion","if=(talent.massacre.enabled&target.health.pct<35|target.health.pct<20)&debuff.colossus_smash.up );
  }

  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    if ( racial_actions[ i ] == "arcane_torrent" )
    {
      default_list->add_action( racial_actions[ i ] +
                                ",if=debuff.colossus_smash.down&cooldown.mortal_strike.remains>1.5&rage<50" );
    }
    else if ( racial_actions[ i ] == "lights_judgment" )
    {
      default_list->add_action( racial_actions[ i ] + ",if=debuff.colossus_smash.down" );
    }
    else
    {
      default_list->add_action( racial_actions[ i ] + ",if=debuff.colossus_smash.up" );
    }
  }

  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].name_str == "grongs_primal_rage" )
    {
      default_list->add_action( "use_item,name=" + items[ i ].name_str +
                                ",if=equipped.grongs_primal_rage&!debuff.colossus_smash.up&!buff.test_of_might.up" );
    }
    else if ( items[ i ].has_special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE ) )
    {
      if ( items[ i ].slot != SLOT_WAIST )
        default_list->add_action( "use_item,name=" + items[ i ].name_str );
    }
  }

  default_list->add_talent(
      this, "Avatar",
      "if=cooldown.colossus_smash.remains<8|(talent.warbreaker.enabled&cooldown.warbreaker.remains<8)" );
  default_list->add_action( this, "Sweeping Strikes", "if=spell_targets.whirlwind>1&(cooldown.bladestorm.remains>10"
                            "|cooldown.colossus_smash.remains>8|azerite.test_of_might.enabled)" );

  default_list->add_action( "run_action_list,name=hac,if=raid_event.adds.exists" );
  default_list->add_action( "run_action_list,name=five_target,if=spell_targets.whirlwind>4" );
  default_list->add_action( "run_action_list,name=execute,if=(talent.massacre.enabled&target.health.pct<35)|target.health.pct<20" );
  default_list->add_action( "run_action_list,name=single_target" );

  hac->add_talent( this, "Rend",
                           "if=remains<=duration*0.3&(!raid_event.adds.up|buff.sweeping_strikes.up)" );
  hac->add_talent( this, "Skullsplitter",
                           "if=rage<60&(cooldown.deadly_calm.remains>3|!talent.deadly_calm.enabled)" );
  hac->add_talent( this, "Deadly Calm",
                           "if=(cooldown.bladestorm.remains>6|talent.ravager.enabled&cooldown.ravager.remains>6)&"
                           "(cooldown.colossus_smash.remains<2|(talent.warbreaker.enabled&cooldown.warbreaker.remains<2))" );
  hac->add_talent( this, "Ravager",
                           "if=(raid_event.adds.up|raid_event.adds.in>target.time_to_die)&(cooldown.colossus_smash.remains<2|"
                           "(talent.warbreaker.enabled&cooldown.warbreaker.remains<2))" );
  hac->add_action( this, "Colossus Smash",
                           "if=raid_event.adds.up|raid_event.adds.in>40|(raid_event.adds.in>20&talent.anger_management.enabled)" );
  hac->add_talent( this, "Warbreaker",
                           "if=raid_event.adds.up|raid_event.adds.in>40|(raid_event.adds.in>20&talent.anger_management.enabled)" );
  hac->add_action( this, "Bladestorm",
                           "if=(debuff.colossus_smash.up&raid_event.adds.in>target.time_to_die)|raid_event.adds.up&"
                           "((debuff.colossus_smash.remains>4.5&!azerite.test_of_might.enabled)|buff.test_of_might.up)" );
  hac->add_action( this, "Overpower",
                           "if=!raid_event.adds.up|(raid_event.adds.up&azerite.seismic_wave.enabled)" );
  hac->add_talent( this, "Cleave",
                           "if=spell_targets.whirlwind>2" );
  hac->add_action( this, "Execute",
                           "if=!raid_event.adds.up|(!talent.cleave.enabled&dot.deep_wounds.remains<2)|buff.sudden_death.react" );
  hac->add_action( this, "Mortal Strike",
                           "if=!raid_event.adds.up|(!talent.cleave.enabled&dot.deep_wounds.remains<2)" );
  hac->add_action( this, "Whirlwind",
                           "if=raid_event.adds.up" );
  hac->add_action( this, "Overpower" );
  hac->add_action( this, "Whirlwind",
                           "if=talent.fervor_of_battle.enabled" );
  hac->add_action( this, "Slam",
                           "if=!talent.fervor_of_battle.enabled&!raid_event.adds.up" );

  five_target->add_talent( this, "Skullsplitter",
                           "if=rage<60&(!talent.deadly_calm.enabled|buff.deadly_calm.down)" );

  five_target->add_talent( this, "Ravager",
                           "if=(!talent.warbreaker.enabled|cooldown.warbreaker.remains<2)" );
  five_target->add_action( this, "Colossus Smash", "if=debuff.colossus_smash.down" );
  five_target->add_talent( this, "Warbreaker", "if=debuff.colossus_smash.down" );
  five_target->add_action( this, "Bladestorm",
                           "if=buff.sweeping_strikes.down&(!talent.deadly_calm.enabled|buff.deadly_calm.down)&"
                            "((debuff.colossus_smash.remains>4.5&!azerite.test_of_might.enabled)|buff.test_of_might.up)" );
  five_target->add_talent( this, "Deadly Calm" );
  five_target->add_talent( this, "Cleave" );
  five_target->add_action( this, "Execute",
                           "if=(!talent.cleave.enabled&dot.deep_wounds.remains<2)|(buff.sudden_death.react|buff.stone_"
                           "heart.react)&(buff.sweeping_strikes.up|cooldown.sweeping_strikes.remains>8)" );
  five_target->add_action( this, "Mortal Strike",
                           "if=(!talent.cleave.enabled&dot.deep_wounds.remains<2)|buff.sweeping_strikes.up&buff."
                           "overpower.stack=2&(talent.dreadnaught.enabled|buff.executioners_precision.stack=2)" );
  five_target->add_action( this, "Whirlwind", "if=debuff.colossus_smash.up|(buff.crushing_assault.up&talent.fervor_of_"
                                 "battle.enabled)" );
  five_target->add_action( this, "Whirlwind", "if=buff.deadly_calm.up|rage>60" );
  five_target->add_action( this, "Overpower" );
  five_target->add_action( this, "Whirlwind" );

//execute->add_talent( this, "Rend", "if=remains<=duration*0.3&debuff.colossus_smash.down" ); Not worth casting at the moment
  execute->add_talent( this, "Skullsplitter",
                       "if=rage<60&(!talent.deadly_calm.enabled|buff.deadly_calm.down)" );
  execute->add_talent( this, "Ravager",
                       "if=!buff.deadly_calm.up&(cooldown.colossus_smash.remains<2|(talent.warbreaker.enabled&cooldown."
                       "warbreaker.remains<2))" );
  execute->add_action( this, "Colossus Smash", "if=debuff.colossus_smash.down" );
  execute->add_talent( this, "Warbreaker", "if=debuff.colossus_smash.down" );
  execute->add_talent( this, "Deadly Calm");
  execute->add_action( this, "Bladestorm", "if=rage<30&!buff.deadly_calm.up" );
  execute->add_talent( this, "Cleave", "if=spell_targets.whirlwind>2" );
  execute->add_action( this, "Slam", "if=buff.crushing_assault.up" );
  execute->add_action( this, "Mortal Strike","if=buff.overpower.stack=2&talent.dreadnaught."
                             "enabled|buff.executioners_precision.stack=2" );
  execute->add_action( this, "Execute" , "if=buff.deadly_calm.up");
  execute->add_action( this, "Overpower" );
  execute->add_action( this, "Execute" );

  single_target->add_talent( this, "Rend", "if=remains<=duration*0.3&debuff.colossus_smash.down" );
  single_target->add_talent( this, "Skullsplitter",
                             "if=rage<60&(!talent.deadly_calm.enabled|buff.deadly_calm.down)" );
  single_target->add_talent( this, "Ravager", "if=!buff.deadly_calm.up&(cooldown.colossus_smash.remains<2|(talent."
                             "warbreaker.enabled&cooldown.warbreaker.remains<2))");
  single_target->add_action( this, "Colossus Smash", "if=debuff.colossus_smash.down" );
  single_target->add_talent( this, "Warbreaker", "if=debuff.colossus_smash.down" );
  single_target->add_talent( this, "Deadly Calm" );
  single_target->add_action( this, "Execute", "if=buff.sudden_death.react" );
  single_target->add_action( this, "Bladestorm",
                             "if=cooldown.mortal_strike.remains&(!talent.deadly_calm.enabled|buff.deadly_calm.down)&"
                             "((debuff.colossus_smash.up&!azerite.test_of_might.enabled)|buff.test_of_might.up)" );
  single_target->add_talent( this, "Cleave", "if=spell_targets.whirlwind>2" );
  single_target->add_action( this, "Overpower", "if=azerite.seismic_wave.rank=3" );
  single_target->add_action( this, "Mortal Strike" );
  single_target->add_action( this, "Whirlwind", "if=talent.fervor_of_battle.enabled&(buff.deadly_calm.up|rage>=60)" );
  single_target->add_action( this, "Overpower" );
  single_target->add_action( this, "Whirlwind",
                             "if=talent.fervor_of_battle.enabled&(!azerite.test_of_might.enabled|"
                             "debuff.colossus_smash.up)" );
  single_target->add_action( this, "Slam",
                             "if=!talent.fervor_of_battle.enabled&(!azerite.test_of_might.enabled|"
                             "debuff.colossus_smash.up|buff.deadly_calm.up|rage>=60)");
}

// Protection Warrior Action Priority List ========================================

void warrior_t::apl_prot()
{
  std::vector<std::string> racial_actions = get_racial_actions();

  default_apl_dps_precombat();

  action_priority_list_t* default_list = get_action_priority_list( "default" );
  action_priority_list_t* st           = get_action_priority_list( "st" );
  action_priority_list_t* aoe          = get_action_priority_list( "aoe" );

  default_list -> add_action( "auto_attack" );
  default_list -> add_action( this, "Intercept", "if=time=0" );
  default_list -> add_action( "use_items,if=cooldown.avatar.remains>20" );
  default_list -> add_action( "use_item,name=grongs_primal_rage,if=buff.avatar.down" );
  
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[ i ] );

  default_list -> add_action( "potion,if=buff.avatar.up|target.time_to_die<25" );
  default_list -> add_action( this, "Ignore Pain", "if=rage.deficit<25+20*talent.booming_voice.enabled*cooldown.demoralizing_shout.ready", "use Ignore Pain to avoid rage capping" );
  default_list -> add_action( this, "Avatar" );
  default_list -> add_action( "run_action_list,name=aoe,if=spell_targets.thunder_clap>=3" );
  default_list -> add_action( "call_action_list,name=st" );

  st -> add_action( this, "Thunder Clap", "if=spell_targets.thunder_clap=2&talent.unstoppable_force.enabled&buff.avatar.up" );
  st -> add_action( this, "Shield Block", "if=cooldown.shield_slam.ready&buff.shield_block.down&azerite.brace_for_impact.rank>azerite.deafening_crash.rank&buff.avatar.up" );
  st -> add_action( this, "Shield Slam", "if=azerite.brace_for_impact.rank>azerite.deafening_crash.rank&buff.avatar.up&buff.shield_block.up" );
  st -> add_action( this, "Thunder Clap", "if=(talent.unstoppable_force.enabled&buff.avatar.up)" );
  st -> add_action( this, "Demoralizing Shout", "if=talent.booming_voice.enabled" );
  st -> add_action( this, "Shield Block", "if=cooldown.shield_slam.ready&buff.shield_block.down" );
  st -> add_action( this, "Shield Slam" );
  st -> add_talent( this, "Dragon Roar" );
  st -> add_action( this, "Thunder Clap" );
  st -> add_action( this, "Revenge" );
  st -> add_talent( this, "Ravager" );
  st -> add_action( this, "Devastate" );

  aoe -> add_action( this, "Thunder Clap" );
  aoe -> add_action( this, "Demoralizing Shout", "if=talent.booming_voice.enabled" );
  aoe -> add_talent( this, "Dragon Roar" );
  aoe -> add_action( this, "Revenge" );
  aoe -> add_talent( this, "Ravager" );
  aoe -> add_action( this, "Shield Block", "if=cooldown.shield_slam.ready&buff.shield_block.down" );
  aoe -> add_action( this, "Shield Slam" );

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


  warrior_buff_t( warrior_td_t& td, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
                 const item_t* item = nullptr )
    : Base( td, name, s, item )
  {
  }

  warrior_buff_t( warrior_t& p, const std::string& name, const spell_data_t* s = spell_data_t::nil(),
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

struct rallying_cry_t : public warrior_buff_t<buff_t>
{
  int health_gain;
  rallying_cry_t( warrior_t& p, const std::string& n, const spell_data_t* s )
    : base_t( p, n, s ), health_gain( 0 )
  {
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    health_gain = static_cast<int>(
        util::floor( warrior().resources.max[ RESOURCE_HEALTH ] * warrior().spec.rallying_cry->effectN( 1 ).percent() ) );
    warrior().stat_gain( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    return base_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    warrior().stat_loss( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Last Stand ======================================================================

struct last_stand_buff_t : public warrior_buff_t<buff_t>
{
  int health_gain;
  last_stand_buff_t( warrior_t& p, const std::string& n, const spell_data_t* s )
    : base_t( p, n, s ), health_gain( 0 )
  {
    add_invalidate( CACHE_BLOCK );
    set_cooldown( timespan_t::zero() );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    health_gain = static_cast<int>(
        util::floor( warrior().resources.max[ RESOURCE_HEALTH ] * ( warrior().spec.last_stand->effectN( 1 ).percent() ) ) );
    warrior().stat_gain( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    return base_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    warrior().stat_loss( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// Demoralizing Shout ================================================================

struct debuff_demo_shout_t : public warrior_buff_t<buff_t>
{
  int extended;
  const int deafening_crash_cap;
  debuff_demo_shout_t( warrior_td_t& p, warrior_t* w )
    : base_t( p, "demoralizing_shout_debuff", w -> find_specialization_spell( "Demoralizing Shout" ) ),
      extended( 0 ), deafening_crash_cap( as<int>( w -> azerite.deafening_crash.spell() -> effectN( 3 ).base_value() ) )
  {
    cooldown -> duration = timespan_t::zero(); // Cooldown handled by the action
    default_value = data().effectN( 1 ).percent();
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    extended = 0; // counts extra seconds gained on each debuff past the initial trigger
    return base_t::trigger( stacks, value, chance, duration );
  }
 
  // TODO? Change how the extension is coded with deafening crash cap
  // if another mechanic extends demo shout duration after the initial trigger
  void extend_duration( player_t* p, timespan_t extra_seconds ) override
  {
    if ( extended < deafening_crash_cap )
    {
      base_t::extend_duration( p, extra_seconds );
      extended += as<int>(extra_seconds.total_seconds() );
    }
  }
};

// Protection T20 2P buff that generates rage over time ==================================

struct protection_rage_t : public warrior_buff_t<buff_t>
{
  protection_rage_t( warrior_t& p, const std::string& n, const spell_data_t* s )
    : base_t( p, n, s )
  {
    set_tick_callback( [&p]( buff_t*, int, const timespan_t& ) {
            p.resource_gain(
                RESOURCE_RAGE,
                p.sets->set( WARRIOR_PROTECTION, T20, B2 )->effectN( 1 ).trigger()->effectN( 2 ).resource( RESOURCE_RAGE ),
                p.gain.protection_t20_2p );
          } );
    // The initial tick generates 20 rage and is done in Berserker's Rage execute
    tick_zero = false;
  }
};

// Test of Might Tracker ===================================================================

struct test_of_might_t : public warrior_buff_t<buff_t>
{
  test_of_might_t( warrior_t& p, const std::string& n, const spell_data_t* s )
    : base_t( p, n, s )
  {
    quiet = true;
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stat_buff_t* test_of_might = warrior().buff.test_of_might;
    const int strength = static_cast<int>( current_value / 10 ) * as<int>( warrior().azerite.test_of_might.value( 1 ) );
    test_of_might->manual_stats_added = false;
    test_of_might->add_stat( STAT_STRENGTH, strength );
    test_of_might->trigger();
    current_value = 0;
    base_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// That legendary crap ring ===========================================================

struct sephuzs_secret_buff_t : public buff_t
{
  cooldown_t* icd;
  sephuzs_secret_buff_t( warrior_t* p ) : buff_t( p, "sephuzs_secret", p->find_spell( 208052 ) )
  {
    set_default_value( p->find_spell( 208052 )->effectN( 2 ).percent() );
    add_invalidate( CACHE_ATTACK_HASTE );
    icd           = p->get_cooldown( "sephuzs_secret_cooldown" );
    icd->duration = p->find_spell( 226262 )->duration();
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    if ( icd->down() )
      return;
    buff_t::execute( stacks, value, duration );
    icd->start();
  }
};

}  // end namespace buffs

// ==========================================================================
// Warrior Character Definition
// ==========================================================================

warrior_td_t::warrior_td_t( player_t* target, warrior_t& p ) : actor_target_data_t( target, &p ), warrior( p )
{
  using namespace buffs;

  dots_deep_wounds = target->get_dot( "deep_wounds", &p );
  dots_ravager     = target->get_dot( "ravager", &p );
  dots_rend        = target->get_dot( "rend", &p );
  dots_gushing_wound = target->get_dot( "gushing_wound", &p );

  debuffs_colossus_smash = buff_creator_t( static_cast<actor_pair_t>( *this ), "colossus_smash" )
                               .default_value( p.spell.colossus_smash_debuff->effectN( 2 ).percent() )
                               .duration( p.spell.colossus_smash_debuff->duration() )
                               .cd( timespan_t::zero() );

  debuffs_siegebreaker = buff_creator_t( static_cast<actor_pair_t>( *this ), "siegebreaker" )
                             .default_value( p.spell.siegebreaker_debuff->effectN( 2 ).percent() )
                             .duration( p.spell.siegebreaker_debuff->duration() )
                             .cd( timespan_t::zero() );

  debuffs_demoralizing_shout = new buffs::debuff_demo_shout_t( *this, &p );
  debuffs_punish = buff_creator_t( static_cast<actor_pair_t>( *this ), "punish", p.talents.punish -> effectN( 2 ).trigger() )
                   .default_value( p.talents.punish -> effectN( 2 ).trigger() -> effectN( 1 ).percent() );
  debuffs_callous_reprisal = buff_creator_t( static_cast<actor_pair_t>( *this ), "callous_reprisal", 
                                             p.azerite.callous_reprisal.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
                              .default_value( p.azerite.callous_reprisal.spell() -> effectN( 1 ).percent() );
  debuffs_taunt = buff_creator_t( static_cast<actor_pair_t>( *this ), "taunt", p.find_class_spell( "Taunt" ) );
}

// warrior_t::init_buffs ====================================================

void warrior_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  buff.furious_charge = buff_creator_t( this, "furious_charge", find_spell( 202225 ) )
                            .chance( talents.furious_charge->ok() )
                            .default_value( find_spell( 202225 )->effectN( 1 ).percent() );

  buff.revenge =
      buff_creator_t( this, "revenge", find_spell( 5302 ) ).default_value( find_spell( 5302 )->effectN( 1 ).percent() );

  buff.avatar = buff_creator_t( this, "avatar", specialization() == WARRIOR_PROTECTION ? spec.avatar : talents.avatar ).cd( timespan_t::zero() );


  buff.berserker_rage = buff_creator_t( this, "berserker_rage", spec.berserker_rage ).cd( timespan_t::zero() );

  buff.frothing_berserker = make_buff( this, "frothing_berserker", find_spell( 215572 ) )
                                ->set_default_value( find_spell( 215572 )->effectN( 2 ).percent() )
                                ->add_invalidate( CACHE_ATTACK_HASTE );

  buff.bounding_stride = buff_creator_t( this, "bounding_stride", find_spell( 202164 ) )
                             .chance( talents.bounding_stride->ok() )
                             .default_value( find_spell( 202164 )->effectN( 1 ).percent() );

  buff.bladestorm =
      buff_creator_t( this, "bladestorm", talents.bladestorm->ok() ? talents.bladestorm : spec.bladestorm )
          .period( timespan_t::zero() )
          .cd( timespan_t::zero() );

  buff.defensive_stance = buff_creator_t( this, "defensive_stance", talents.defensive_stance )
                              .activated( true )
                              .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  
  buff.die_by_the_sword = buff_creator_t( this, "die_by_the_sword", spec.die_by_the_sword )
                              .default_value( spec.die_by_the_sword->effectN( 2 ).percent() )
                              .cd( timespan_t::zero() )
                              .add_invalidate( CACHE_PARRY );

  buff.enrage = make_buff( this, "enrage", find_spell( 184362 ) )
                    ->add_invalidate( CACHE_ATTACK_HASTE )
                    ->add_invalidate( CACHE_RUN_SPEED )
                    ->set_default_value( find_spell( 184362 )->effectN( 1 ).percent() );

  buff.furious_slash = make_buff( this, "furious_slash", find_spell( 202539 ) )
                           ->set_default_value( find_spell( 202539 )->effectN( 1 ).percent() )
                           ->add_invalidate( CACHE_ATTACK_HASTE );

  buff.heroic_leap_movement = buff_creator_t( this, "heroic_leap_movement" );
  buff.charge_movement      = buff_creator_t( this, "charge_movement" );
  buff.intervene_movement   = buff_creator_t( this, "intervene_movement" );
  buff.intercept_movement   = buff_creator_t( this, "intercept_movement" );

  buff.into_the_fray = buff_creator_t( this, "into_the_fray", find_spell( 202602 ) )
                           .chance( talents.into_the_fray->ok() )
                           .default_value( find_spell( 202602 )->effectN( 1 ).percent() )
                           .add_invalidate( CACHE_HASTE );

  buff.last_stand = new buffs::last_stand_buff_t( *this, "last_stand", spec.last_stand );

  buff.meat_cleaver = buff_creator_t( this, "meat_cleaver", spell.whirlwind_buff );

  buff.rallying_cry = new buffs::rallying_cry_t( *this, "rallying_cry", find_spell( 97463 ) );

  buff.overpower =
      buff_creator_t( this, "overpower", spec.overpower )
          .default_value( spec.overpower->effectN( 2 ).percent() + talents.dreadnaught->effectN( 2 ).percent() );

  buff.ravager = buff_creator_t( this, "ravager", talents.ravager );

  buff.ravager_protection = buff_creator_t( this, "ravager_protection", spell.ravager_protection )
                            .add_invalidate( CACHE_PARRY );

  buff.spell_reflection = buff_creator_t( this, "spell_reflection", spec.spell_reflection );

  buff.sweeping_strikes = buff_creator_t( this, "sweeping_strikes", spec.sweeping_strikes ).cd( timespan_t::zero() );

  buff.ignore_pain = new ignore_pain_buff_t( this );

  buff.recklessness = make_buff( this, "recklessness", spec.recklessness )
    ->set_duration( spec.recklessness->duration() + talents.reckless_abandon->effectN( 1 ).time_value() )
    ->add_invalidate( CACHE_CRIT_CHANCE )
    ->set_cooldown( timespan_t::zero() )
    ->set_default_value( spec.recklessness->effectN( 1 ).percent() )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int after ) { if ( after == 0 ) buff.infinite_fury->trigger(); });

  buff.sudden_death = buff_creator_t( this, "sudden_death", talents.sudden_death );

  buff.deadly_calm = buff_creator_t( this, "deadly_calm", talents.deadly_calm ).cd( timespan_t::zero() );

  buff.shield_block = buff_creator_t( this, "shield_block", spell.shield_block_buff )
                          .cd( timespan_t::zero() )
                          .add_invalidate( CACHE_BLOCK );

  buff.shield_wall = buff_creator_t( this, "shield_wall", spec.shield_wall )
                         .default_value( spec.shield_wall->effectN( 1 ).percent() )
                         .cd( timespan_t::zero() );

  buff.vengeance_ignore_pain = buff_creator_t( this, "vengeance_ignore_pain", find_spell( 202574 ) )
                                   .chance( talents.vengeance->ok() )
                                   .default_value( find_spell( 202574 )->effectN( 1 ).percent() );

  buff.vengeance_revenge = buff_creator_t( this, "vengeance_revenge", find_spell( 202573 ) )
                               .chance( talents.vengeance->ok() )
                               .default_value( find_spell( 202573 )->effectN( 1 ).percent() );

  buff.raging_thirst =
      buff_creator_t( this, "raging_thirst", sets->set( WARRIOR_FURY, T20, B2 )->effectN( 1 ).trigger() )
          .default_value( sets->set( WARRIOR_FURY, T20, B2 )->effectN( 1 ).trigger()->effectN( 1 ).percent() )
          .chance( sets->set( WARRIOR_FURY, T20, B2 )->proc_chance() );

  buff.t20_fury_4p =
      buff_creator_t( this, "t20_fury_4p", talents.inner_rage->ok() ? find_spell( 242953 ) : find_spell( 242952 ) )
          .default_value( talents.inner_rage->ok() ? find_spell( 242953 )->effectN( 1 ).percent()
                                                   : find_spell( 242952 )->effectN( 1 ).percent() )
          .chance( sets->has_set_bonus( WARRIOR_FURY, T20, B4 ) );

  buff.sephuzs_secret = new buffs::sephuzs_secret_buff_t( this );

  buff.in_for_the_kill = new in_for_the_kill_t( *this, "in_for_the_kill", find_spell( 248622 ) );

  buff.war_veteran =
      buff_creator_t( this, "war_veteran", sets->set( WARRIOR_ARMS, T21, B2 )->effectN( 1 ).trigger() )
          .default_value( sets->set( WARRIOR_ARMS, T21, B2 )->effectN( 1 ).trigger()->effectN( 1 ).percent() )
          .chance( sets->has_set_bonus( WARRIOR_ARMS, T21, B2 ) );

  buff.weighted_blade =
      buff_creator_t( this, "weighted_blade", sets->set( WARRIOR_ARMS, T21, B4 )->effectN( 1 ).trigger() )
          .default_value( sets->set( WARRIOR_ARMS, T21, B4 )->effectN( 1 ).trigger()->effectN( 1 ).percent() )
          .chance( sets->has_set_bonus( WARRIOR_ARMS, T21, B4 ) );

  buff.protection_rage = new protection_rage_t( *this, "protection_rage", find_spell( 242303 ) );

  buff.whirlwind = buff_creator_t( this, "whirlwind", find_spell( 85739 ) );

  // Azerite
  const spell_data_t* bloodcraze_trigger = azerite.bloodcraze.spell()->effectN( 1 ).trigger();
  const spell_data_t* bloodcraze_buff    = bloodcraze_trigger->effectN( 1 ).trigger();
  buff.bloodcraze_driver =
      make_buff<buff_t>( this, "bloodcraze_driver", bloodcraze_trigger )
          ->set_trigger_spell( azerite.bloodcraze )
          ->set_quiet( true )
          ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
          ->set_tick_callback( [this]( buff_t*, int, const timespan_t& ) { buff.bloodcraze->trigger(); } );

  buff.bloodcraze = make_buff<buff_t>( this, "bloodcraze", bloodcraze_buff )
                        ->set_trigger_spell( bloodcraze_trigger )
                        ->set_default_value( azerite.bloodcraze.value( 1 ) );

  const spell_data_t* crushing_assault_trigger = azerite.crushing_assault.spell()->effectN( 1 ).trigger();
  const spell_data_t* crushing_assault_buff    = crushing_assault_trigger->effectN( 1 ).trigger();
  buff.crushing_assault                        = make_buff<buff_t>( this, "crushing_assault", crushing_assault_buff )
                              ->set_default_value( azerite.crushing_assault.value( 1 ) )
                              ->set_trigger_spell( crushing_assault_trigger );

  buff.executioners_precision =
      make_buff( this, "executioners_precision", find_spell( 272870 ) )
          ->set_trigger_spell( azerite.executioners_precision.spell_ref().effectN( 1 ).trigger() )
          ->set_default_value( azerite.executioners_precision.value() );

  const spell_data_t* gathering_storm_trigger = azerite.gathering_storm.spell()->effectN( 1 ).trigger();
  //const spell_data_t* gathering_storm_buff    = gathering_storm_trigger->effectN( 1 ).trigger();
  buff.gathering_storm                        = make_buff<stat_buff_t>( this, "gathering_storm", find_spell( 273415 ) )
                           ->add_stat( STAT_STRENGTH, azerite.gathering_storm.value( 1 ) )
                           ->set_trigger_spell( gathering_storm_trigger );

  const spell_data_t* infinite_fury_trigger = azerite.infinite_fury.spell()->effectN( 1 ).trigger();
  const spell_data_t* infinite_fury_buff    = infinite_fury_trigger ->effectN( 1 ).trigger();
  buff.infinite_fury = make_buff( this, "infinite_fury", infinite_fury_buff    )
                               ->set_trigger_spell( infinite_fury_trigger  )
                               ->set_default_value( azerite.infinite_fury.value() )
                               ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.pulverizing_blows = make_buff( this, "pulverizing_blows", find_spell( 275672 ) )
                               ->set_trigger_spell( azerite.pulverizing_blows.spell_ref().effectN( 1 ).trigger() )
                               ->set_default_value( azerite.pulverizing_blows.value() );

  const spell_data_t* test_of_might_tracker = azerite.test_of_might.spell()->effectN( 1 ).trigger()->effectN( 1 ).trigger();
  buff.test_of_might_tracker = new test_of_might_t( *this, "test_of_might_tracker", test_of_might_tracker );
  buff.test_of_might = make_buff<stat_buff_t>( this, "test_of_might", find_spell( 275540 ) );
  buff.test_of_might->set_trigger_spell( test_of_might_tracker );

  const spell_data_t* trample_the_weak_trigger = azerite.trample_the_weak.spell()->effectN( 1 ).trigger();
  const spell_data_t* trample_the_weak_buff    = trample_the_weak_trigger->effectN( 1 ).trigger();
  buff.trample_the_weak = make_buff<stat_buff_t>( this, "trample_the_weak", trample_the_weak_buff )
                              ->add_stat( STAT_STRENGTH, azerite.trample_the_weak.value( 1 ) )
                              ->add_stat( STAT_STAMINA, azerite.trample_the_weak.value( 1 ) )
                              ->set_trigger_spell( trample_the_weak_trigger );
  buff.bloodsport = make_buff<stat_buff_t>( this, "bloodsport", azerite.bloodsport.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
                   -> add_stat( STAT_LEECH_RATING, azerite.bloodsport.value( 2 ) );
  buff.brace_for_impact = make_buff( this, "brace_for_impact", azerite.brace_for_impact.spell() -> effectN( 1 ).trigger() -> effectN( 1 ).trigger() )
                         -> set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
                         -> set_default_value( azerite.brace_for_impact.value( 1 ) );
  buff.striking_the_anvil = make_buff( this, "striking_the_anvil", find_spell( 288452 ) )
                               ->set_trigger_spell( azerite.striking_the_anvil.spell_ref().effectN( 1 ).trigger() )
                               ->set_default_value( azerite.striking_the_anvil.value() );

  const spell_data_t* bastion_of_might_trigger = azerite.bastion_of_might.spell()->effectN( 1 ).trigger();
  const spell_data_t* bastion_of_might_buff = bastion_of_might_trigger->effectN( 1 ).trigger();
  buff.bastion_of_might = make_buff<stat_buff_t>( this, "bastion_of_might", bastion_of_might_buff)
                               ->add_stat( STAT_MASTERY_RATING, azerite.bastion_of_might.value( 1 ) )
                               ->set_trigger_spell( bastion_of_might_trigger );

}

// warrior_t::init_scaling ==================================================

void warrior_t::init_scaling()
{
  player_t::init_scaling();

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
  player_t::init_gains();

  gain.archavons_heavy_hand             = get_gain( "archavons_heavy_hand" );
  gain.avoided_attacks                  = get_gain( "avoided_attacks" );
  gain.critical_block                   = get_gain( "critical_block" );
  gain.melee_crit                       = get_gain( "melee_crit" );
  gain.melee_main_hand                  = get_gain( "melee_main_hand" );
  gain.melee_off_hand                   = get_gain( "melee_off_hand" );
  gain.revenge                          = get_gain( "revenge" );
  gain.shield_slam                      = get_gain( "shield_slam" );
  gain.booming_voice                    = get_gain( "booming_voice" );
  gain.thunder_clap                     = get_gain( "thunder_clap" );
  gain.protection_t20_2p                = get_gain( "t20_2p" );
  gain.whirlwind                        = get_gain( "whirlwind" );
  gain.collateral_damage                = get_gain( "collateral_damage" );

  gain.ceannar_rage           = get_gain( "ceannar_rage" );
  gain.cold_steel_hot_blood   = get_gain( "cold_steel_hot_blood" );
  gain.endless_rage           = get_gain( "endless_rage" );
  gain.lord_of_war            = get_gain( "lord_of_war" );
  gain.meat_cleaver           = get_gain( "meat_cleaver" );
  gain.valarjar_berserking    = get_gain( "valarjar_berserking" );
  gain.ravager                = get_gain( "ravager" );
  gain.rage_from_damage_taken = get_gain( "rage_from_damage_taken" );
  gain.simmering_rage         = get_gain( "simmering_rage" );
  gain.execute_refund         = get_gain( "execute_refund" );
}

// warrior_t::init_position ====================================================

void warrior_t::init_position()
{
  player_t::init_position();
}

// warrior_t::init_procs ======================================================

void warrior_t::init_procs()
{
  player_t::init_procs();
  proc.delayed_auto_attack = get_proc( "delayed_auto_attack" );

  proc.tactician = get_proc( "tactician" );
}

// warrior_t::init_resources ================================================

void warrior_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_RAGE ] = 0;  // By default, simc sets all resources to full. However, Warriors cannot
                                           // reliably start combat with more than 0 rage. This will also ensure that
                                           // the 20-35 rage from Charge is not overwritten.
}

// warrior_t::default_potion ================================================

std::string warrior_t::default_potion() const
{
  std::string fury_pot =
      ( true_level > 110 )
          ? "bursting_blood"
          : ( true_level > 100 )
                ? "old_war"
                : ( true_level >= 90 )
                      ? "draenic_strength"
                      : ( true_level >= 85 ) ? "mogu_power" : ( true_level >= 80 ) ? "golemblood_potion" : "disabled";

  std::string arms_pot =
      ( true_level > 110 )
          ? "bursting_blood"
          : ( true_level > 100 )
                ? "old_war"
                : ( true_level >= 90 )
                      ? "draenic_strength"
                      : ( true_level >= 85 ) ? "mogu_power" : ( true_level >= 80 ) ? "golemblood_potion" : "disabled";

  std::string protection_pot =
      ( true_level > 110 )
          ? "battle_potion_of_strength"
          : ( true_level > 100 )
                ? "old_war"
                : ( true_level >= 90 )
                      ? "draenic_strength"
                      : ( true_level >= 85 ) ? "mogu_power" : ( true_level >= 80 ) ? "golemblood_potion" : "disabled";

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
  return ( true_level > 110 )
             ? "undertow"
             : ( true_level > 100 )
                   ? "flask_of_the_countless_armies"
                   : ( true_level >= 90 )
                         ? "greater_draenic_strength_flask"
                         : ( true_level >= 85 ) ? "winters_bite"
                                                : ( true_level >= 80 ) ? "titanic_strength" : "disabled";
}

// warrior_t::default_food ==================================================

std::string warrior_t::default_food() const
{
  std::string fury_food = ( true_level > 110 )
                              ? "bountiful_captains_feast"
                              : ( true_level > 100 )
                                    ? "the_hungry_magister"
                                    : ( true_level > 90 )
                                          ? "buttered_sturgeon"
                                          : ( true_level >= 85 )
                                                ? "sea_mist_rice_noodles"
                                                : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

  std::string arms_food = ( true_level > 110 )
                              ? "bountiful_captains_feast"
                              : ( true_level > 100 )
                                    ? "the_hungry_magister"
                                    : ( true_level > 90 )
                                          ? "buttered_sturgeon"
                                          : ( true_level >= 85 )
                                                ? "sea_mist_rice_noodles"
                                                : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

  std::string protection_food = ( true_level > 110 )
                              ? "bountiful_captains_feast"
                              : ( true_level > 100 )
                                    ? "the_hungry_magister"
                                    : ( true_level > 90 )
                                          ? "buttered_sturgeon"
                                          : ( true_level >= 85 )
                                                ? "sea_mist_rice_noodles"
                                                : ( true_level >= 80 ) ? "seafood_magnifique_feast" : "disabled";

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
  return ( true_level >= 120 ) ? "battle_scarred"
                               : ( true_level >= 110 ) ? "defiled" : ( true_level >= 100 ) ? "hyper" : "disabled";
}

// warrior_t::init_actions ==================================================

void warrior_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
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
      apl_fury();
      break;
    case WARRIOR_ARMS:
      apl_arms();
      break;
    case WARRIOR_PROTECTION:
      apl_prot();
      break;
    default:
      apl_default();  // DEFAULT
      break;
  }

  // Default
  use_default_action_list = true;
  player_t::init_action_list();
}

// warrior_t::arise() ======================================================

void warrior_t::arise()
{
  player_t::arise();
}

// warrior_t::combat_begin ==================================================

void warrior_t::combat_begin()
{
  if ( !sim->fixed_time )
  {
    if ( warrior_fixed_time )
    {
      for ( size_t i = 0; i < sim->player_list.size(); ++i )
      {
        player_t* p = sim->player_list[ i ];
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
  player_t::combat_begin();
  buff.into_the_fray -> trigger( into_the_fray_friends < 0 ? buff.into_the_fray -> max_stack() : into_the_fray_friends + 1 );
  buff.bloodcraze->trigger( buff.bloodcraze->data().max_stacks() );
  buff.bloodcraze_driver->trigger();
}

// Into the fray

struct into_the_fray_callback_t
{
  warrior_t* w;
  double fray_distance;
  into_the_fray_callback_t( warrior_t* p ) : w( p ), fray_distance( 0 )
  {
    fray_distance = p->talents.into_the_fray->effectN( 1 ).base_value();
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

void warrior_t::activate()
{
  player_t::activate();

  // If the value is equal to -1, don't use the callback and just assume max stacks all the time
  if ( talents.into_the_fray->ok() && into_the_fray_friends >= 0 )
  {
    sim->target_non_sleeping_list.register_callback( into_the_fray_callback_t( this ) );
  }
}

// warrior_t::reset =========================================================

void warrior_t::reset()
{
  player_t::reset();

  heroic_charge  = nullptr;
  rampage_driver = nullptr;
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
  if ( heroic_charge )
  {
    event_t::cancel( heroic_charge );
  }
  player_t::interrupt();
}

void warrior_t::teleport( double, timespan_t )
{
  // All movement "teleports" are modeled.
}

void warrior_t::trigger_movement( double distance, movement_direction_e direction )
{
  if ( talents.into_the_fray->ok() && into_the_fray_friends >= 0 )
  {
    into_the_fray_callback_t( this );
  }
  if ( heroic_charge )
  {
    event_t::cancel( heroic_charge );  // Cancel heroic leap if it's running to make sure nothing weird happens when
                                       // movement from another source is attempted.
    player_t::trigger_movement( distance, direction );
  }
  else
  {
    player_t::trigger_movement( distance, direction );
  }
}

// warrior_t::composite_player_multiplier ===================================

double warrior_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buff.tornados_eye->check() )
  {
    m *= 1.0 + ( buff.tornados_eye->current_stack * buff.tornados_eye->data().effectN( 2 ).percent() );
  }

  if ( buff.defensive_stance->check() )
  {
    m *= 1.0 + talents.defensive_stance->effectN( 2 ).percent();
  }

  m *= 1.0 + buff.fujiedas_fury->check_stack_value();

  return m;
}

// warrior_t::composite_melee_haste ===========================================

double warrior_t::composite_melee_haste() const
{
  double a = player_t::composite_melee_haste();

  a *= 1.0 / ( 1.0 + buff.enrage->check_value() );

  a *= 1.0 / ( 1.0 + buff.frothing_berserker->check_value() );

  a *= 1.0 / ( 1.0 + buff.furious_slash->check_stack_value() );

  a *= 1.0 / ( 1.0 + buff.into_the_fray->check_stack_value() );

  a *= 1.0 / ( 1.0 + buff.sephuzs_secret->check_value() );

  a *= 1.0 / ( 1.0 + buff.in_for_the_kill->check_value() );

  return a;
}

// warrior_t::composite_melee_expertise =====================================

double warrior_t::composite_melee_expertise( const weapon_t* ) const
{
  double e = player_t::composite_melee_expertise();

  e += spec.prot_warrior->effectN( 11 ).percent();

  return e;
}

// warrior_t::composite_mastery =============================================

double warrior_t::composite_mastery() const
{
  return player_t::composite_mastery();
}

// warrior_t::composite_attribute_multiplier ================================

double warrior_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // Protection has increased stamina from vanguard
  if ( attr == ATTR_STAMINA )
  {
    m *= 1.0 + spec.vanguard -> effectN( 2 ).percent();
  }

  return m;
}

// warrior_t::composite_rating_multiplier ===================================

double warrior_t::composite_rating_multiplier( rating_e rating ) const
{
  return player_t::composite_rating_multiplier( rating );
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

// warrior_t::composite_bonus_armor ==========================================

double warrior_t::composite_bonus_armor() const
{
  double ba = player_t::composite_bonus_armor();

  ba += spec.vanguard -> effectN( 1 ).percent() * cache.strength();

  return ba;
}

// warrior_t::composite_block ================================================

double warrior_t::composite_block() const
{
  // this handles base block and and all block subject to diminishing returns
  double block_subject_to_dr = cache.mastery() * mastery.critical_block->effectN( 2 ).mastery_value();
  double b                   = player_t::composite_block_dr( block_subject_to_dr );

  // Protection Warriors have a +8% block chance in their spec aura
  b += spec.prot_warrior -> effectN( 13 ).percent();

  // shield block adds 100% block chance
  if ( buff.shield_block -> check() )
  {
    b += spell.shield_block_buff -> effectN( 1 ).percent();
  }

  if ( buff.last_stand && talents.bolster -> ok() )
  {
    b+= talents.bolster -> effectN( 2 ).percent();
  }

  return b;
}

// warrior_t::composite_block_reduction =====================================

double warrior_t::composite_block_reduction( action_state_t* s ) const
{
  double br = player_t::composite_block_reduction( s );

  if ( buff.brace_for_impact -> up() )
  {
    br += buff.brace_for_impact -> stack_value();
  }

  if ( azerite.iron_fortress.enabled() )
  {
    br += azerite.iron_fortress.value( 2 );
  }
  
  return br;
}

// warrior_t::composite_parry_rating() ========================================

double warrior_t::composite_parry_rating() const
{
  double p = player_t::composite_parry_rating();

  // TODO: remove the spec check once riposte is pulled from spelldata
  if ( spell.riposte -> ok() || specialization() == WARRIOR_PROTECTION )
  {
    p += composite_melee_crit_rating();
  }
  return p;
}

// warrior_t::composite_parry =================================================

double warrior_t::composite_parry() const
{
  double parry = player_t::composite_parry();

  if ( buff.ravager_protection->check() )
  {
    parry += spell.ravager_protection -> effectN( 1 ).percent();
  }
  else if ( buff.die_by_the_sword->check() )
  {
    parry += spec.die_by_the_sword->effectN( 1 ).percent();
  }
  return parry;
}

// warrior_t::composite_attack_power_multiplier ==============================

double warrior_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.critical_block -> ok() )
  {
    ap *= 1.0 + mastery.critical_block -> effectN( 5 ).mastery_value() * cache.mastery();
  }
  return ap;
}

// warrior_t::composite_crit_block =====================================

double warrior_t::composite_crit_block() const
{

  double b = player_t::composite_crit_block();

  if ( mastery.critical_block->ok() )
  {
    b += cache.mastery() * mastery.critical_block->effectN( 1 ).mastery_value();
  }

  if ( buff.shield_block->check() && sets->has_set_bonus( WARRIOR_PROTECTION, T19, B2 ) )
    b += find_spell( 212236 )->effectN( 1 ).percent();

  return b;
}

// warrior_t::composite_crit_avoidance ===========================================

double warrior_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();
  c += spec.prot_warrior->effectN( 10 ).percent();
  return c;
}

// warrior_t::composite_melee_speed ========================================
/*
double warrior_t::composite_melee_speed() const
{
  double s = player_t::composite_melee_speed();

  return s;
}
*/

// warrior_t::composite_melee_crit_chance =========================================

double warrior_t::composite_melee_crit_chance() const
{
  double c = player_t::composite_melee_crit_chance();

  c += buff.recklessness->check_value();

  return c;
}

// warrior_t::composite_melee_crit_rating =========================================

double warrior_t::composite_melee_crit_rating() const
{
  double c = player_t::composite_melee_crit_rating();

  c += buff.infinite_fury->check_value();

  return c;
}

// warrior_t::composite_player_critical_damage_multiplier ==================

double warrior_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double cdm = player_t::composite_player_critical_damage_multiplier( s );

  if ( buff.war_veteran->check() )
  {
    cdm *= 1.0 + buff.war_veteran->value();
  }

  return cdm;
}

// warrior_t::composite_spell_crit_chance =========================================
/*
double warrior_t::composite_spell_crit_chance() const
{
  return composite_melee_crit_chance();
}
*/

// warrior_t::composite_leech ==============================================
/*
double warrior_t::composite_leech() const
{
  return player_t::composite_leech();
}
*/

// warrior_t::resource_gain =================================================

double warrior_t::resource_gain( resource_e r, double a, gain_t* g, action_t* action )
{
  if ( buff.recklessness->check() && r == RESOURCE_RAGE )
  {
    bool do_not_double_rage = false;
    do_not_double_rage      = ( g == gain.ceannar_rage || g == gain.valarjar_berserking || g == gain.simmering_rage );

    if ( !do_not_double_rage )  // FIXME: remove this horror after BFA launches, keep Simmering Rage
      a *= 1.0 + spec.recklessness->effectN( 4 ).percent();
  }
  return player_t::resource_gain( r, a, g, action );
}

// warrior_t::temporary_movement_modifier ==================================

double warrior_t::temporary_movement_modifier() const
{
  double temporary = player_t::temporary_movement_modifier();

  // These are ordered in the highest speed movement increase to the lowest, there's no reason to check the rest as they
  // will just be overridden. Also gives correct benefit numbers.
  if ( buff.heroic_leap_movement->up() )
  {
    temporary = std::max( buff.heroic_leap_movement->value(), temporary );
  }
  else if ( buff.charge_movement->up() )
  {
    temporary = std::max( buff.charge_movement->value(), temporary );
  }
  else if ( buff.intervene_movement->up() )
  {
    temporary = std::max( buff.intervene_movement->value(), temporary );
  }
  else if ( buff.intercept_movement->up() )
  {
    temporary = std::max( buff.intercept_movement->value(), temporary );
  }
  else if ( buff.bounding_stride->up() )
  {
    temporary = std::max( buff.bounding_stride->value(), temporary );
  }
  else if ( buff.tornados_eye->up() )
  {
    temporary =
        std::max( buff.tornados_eye->current_stack * buff.tornados_eye->data().effectN( 1 ).percent(), temporary );
  }
  return temporary;
}

// warrior_t::invalidate_cache ==============================================

void warrior_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( mastery.critical_block->ok() )
  {
    if ( c == CACHE_MASTERY )
    {
      player_t::invalidate_cache( CACHE_BLOCK );
      player_t::invalidate_cache( CACHE_CRIT_BLOCK );
      player_t::invalidate_cache( CACHE_ATTACK_POWER );
    }
    if ( c == CACHE_CRIT_CHANCE )
    {
      player_t::invalidate_cache( CACHE_PARRY );
    }
  }
  if ( c == CACHE_MASTERY && mastery.unshackled_fury->ok() )
  {
    player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
  if ( c == CACHE_STRENGTH && spec.vanguard -> ok() )
  {
    player_t::invalidate_cache( CACHE_BONUS_ARMOR );
  }
}

// warrior_t::primary_role() ================================================

role_e warrior_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
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
      if ( specialization() == WARRIOR_PROTECTION )
      {
        return s;
      }
      else
      {
        return STAT_NONE;
      }
    default:
      return s;
  }
}

void warrior_t::assess_damage( school_e school, dmg_e type, action_state_t* s )
{
  player_t::assess_damage( school, type, s );

  if ( s->result == RESULT_DODGE || s->result == RESULT_PARRY )
  {
    if ( cooldown.revenge_reset->up() )
    {
      buff.revenge->trigger();
      cooldown.revenge_reset->start();
    }
  }

  // Generate 3 Rage on auto-attack taken. 
  // TODO: Update with spelldata once it actually exists
  else if ( ! s -> action -> special && cooldown.rage_from_auto_attack->up() )
  {
    resource_gain( RESOURCE_RAGE, 3.0, gain.rage_from_damage_taken, s -> action );
    cooldown.rage_from_auto_attack->start( cooldown.rage_from_auto_attack->duration );
  }

  if ( s -> block_result == BLOCK_RESULT_BLOCKED || s -> block_result == BLOCK_RESULT_CRIT_BLOCKED )
  {
    if ( azerite.iron_fortress.enabled() && cooldown.iron_fortress_icd -> up() )
    {
      // sanity check - no friendly-fire
      if ( s -> action -> player -> is_enemy() )
      {
        iron_fortress_t* iron_fortress_active = debug_cast<iron_fortress_t*>( active.iron_fortress );

        iron_fortress_active -> target = s -> action -> player;
        iron_fortress_active -> do_execute( s -> block_result );
      }
    }
  }
}

// warrior_t::target_mitigation ============================================

void warrior_t::target_mitigation( school_e school, dmg_e dtype, action_state_t* s )
{
  player_t::target_mitigation( school, dtype, s );

  if ( s->result == RESULT_HIT || s->result == RESULT_CRIT || s->result == RESULT_GLANCE )
  {
    if ( buff.defensive_stance->up() )
    {
      s->result_amount *= 1.0 + buff.defensive_stance->data().effectN( 1 ).percent();
    }

    warrior_td_t* td = get_target_data( s->action->player );

    if ( td->debuffs_demoralizing_shout->up() )
    {
      s->result_amount *= 1.0 + td->debuffs_demoralizing_shout->value();
    }

    // 2018-10-21: Callous Reprisal's damage reduction is 0.5% per stack per equipped item with the trait selected
    if ( td -> debuffs_callous_reprisal -> up() )
    {
      s -> result_amount *= 1.0 + td -> debuffs_callous_reprisal -> stack_value() * azerite.callous_reprisal.n_items();
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
  }

  if ( action_t::result_is_block( s->block_result ) )
  {
    if ( s->block_result == BLOCK_RESULT_CRIT_BLOCKED )
    {
      resource_gain(
          RESOURCE_RAGE,
          sets->set( WARRIOR_PROTECTION, T19, B4 )->effectN( 1 ).trigger()->effectN( 1 ).resource( RESOURCE_RAGE ),
          gain.critical_block );
    }
  }
}

// warrior_t::create_options ================================================

void warrior_t::create_options()
{
  player_t::create_options();

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

  return player_t::create_profile( type );
}

// warrior_t::copy_from =====================================================

void warrior_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warrior_t* p = debug_cast<warrior_t*>( source );

  non_dps_mechanics     = p->non_dps_mechanics;
  warrior_fixed_time    = p->warrior_fixed_time;
  into_the_fray_friends = p->into_the_fray_friends;
  never_surrender_percentage = p -> never_surrender_percentage;
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

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n";
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
    if ( p.cd_waste_exec.size() > 0 )
    {
      os << "\t\t\t\t\t<h3 class=\"toggle open\">Cooldown waste details</h3>\n"
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
  warrior_t& p;
};

namespace items
{
struct raging_fury_t : public unique_gear::scoped_action_callback_t<charge_t>
{
  raging_fury_t() : super( WARRIOR, "charge" )
  {
  }

  void manipulate( charge_t* action, const special_effect_t& e ) override
  {
    action->energize_amount *= 1.0 + e.driver()->effectN( 1 ).percent();
  }
};

struct raging_fury2_t : public unique_gear::scoped_action_callback_t<intercept_t>
{
  raging_fury2_t() : super( WARRIOR, "intercept" )
  {
  }

  void manipulate( intercept_t* action, const special_effect_t& e ) override
  {
    action->energize_amount *= 1.0 + e.driver()->effectN( 1 ).percent();
  }
};

struct weight_of_the_earth_t : public unique_gear::scoped_action_callback_t<heroic_leap_t>
{
  weight_of_the_earth_t() : super( WARRIOR, "heroic_leap" )
  {
  }

  void manipulate( heroic_leap_t* action, const special_effect_t& e ) override
  {
    action->radius *= 1.0 + e.driver()->effectN( 1 ).percent();
    action->weight_of_the_earth = true;
  }
};

struct ayalas_stone_heart_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  ayalas_stone_heart_t() : super( WARRIOR, "stone_heart" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.ayalas_stone_heart;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.driver()->effectN( 1 ).trigger() )
        ->set_trigger_spell( e.driver() )
        ->set_rppm( RPPM_NONE );
  }
};

struct the_great_storms_eye_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  the_great_storms_eye_t() : super( WARRIOR, "tornados_eye" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.tornados_eye;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    if ( e.player->talent_points.has_row_col( 6, 3 ) )
      return make_buff( e.player, buff_name, e.player->find_spell( 248145 ) )
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          ->add_invalidate( CACHE_RUN_SPEED );
    else
      return make_buff( e.player, buff_name, e.player->find_spell( 248142 ) )
          ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
          ->add_invalidate( CACHE_RUN_SPEED );
  }
};

struct kazzalax_fujiedas_fury_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  kazzalax_fujiedas_fury_t() : super( WARRIOR, "fujiedas_fury" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.fujiedas_fury;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.driver()->effectN( 1 ).trigger() )
        ->set_default_value( e.driver()->effectN( 1 ).trigger()->effectN( 1 ).percent() )
        ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct prydaz_xavarics_magnum_opus_t : public unique_gear::class_buff_cb_t<warrior_t>
{
  prydaz_xavarics_magnum_opus_t() : super( WARRIOR, "xavarics_magnum_opus" )
  {
  }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buff.xavarics_magnum_opus;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.player->find_spell( 207472 ) );
  }
};

struct sephuzs_secret_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  sephuzs_secret_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.sephuzs_secret = e.driver();
  }
};

struct valarjar_berserkers_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  valarjar_berserkers_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.valarjar_berserkers = e.driver();
  }
};

struct archavons_heavy_hand_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  archavons_heavy_hand_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.archavons_heavy_hand = e.driver();
  }
};

struct ceannar_charger_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  ceannar_charger_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.ceannar_charger = e.driver();
  }
};

struct najentuss_vertebrae_t : public unique_gear::scoped_actor_callback_t<warrior_t>
{
  najentuss_vertebrae_t() : super( WARRIOR )
  {
  }

  void manipulate( warrior_t* warrior, const special_effect_t& e ) override
  {
    warrior->legendary.najentuss_vertebrae = e.driver();
  }
};

void init()
{
  unique_gear::register_special_effect( 205144, archavons_heavy_hand_t() );
  unique_gear::register_special_effect( 207779, ceannar_charger_t() );
  unique_gear::register_special_effect( 207775, kazzalax_fujiedas_fury_t(), true );
  unique_gear::register_special_effect( 207428, prydaz_xavarics_magnum_opus_t(), true );  // Not finished
  unique_gear::register_special_effect( 215096, najentuss_vertebrae_t() );
  unique_gear::register_special_effect( 207767, ayalas_stone_heart_t(), true );
  unique_gear::register_special_effect( 208177, weight_of_the_earth_t() );
  unique_gear::register_special_effect( 222266, raging_fury_t() );
  unique_gear::register_special_effect( 222266, raging_fury2_t() );
  unique_gear::register_special_effect( 208051, sephuzs_secret_t() );
  unique_gear::register_special_effect( 248118, the_great_storms_eye_t(), true );
  unique_gear::register_special_effect( 248120, valarjar_berserkers_t() );
}

}  // items

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR )
  {
  }

  player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto p              = new warrior_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new warrior_report_t( *p ) );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void static_init() const override
  {
    items::init();
  }

  void register_hotfixes() const override
  {
  }

  void init( player_t* ) const override
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

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}
